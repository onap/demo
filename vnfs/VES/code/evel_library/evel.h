#ifndef EVEL_INCLUDED
#define EVEL_INCLUDED
/**************************************************************************//**
 * @file
 * Header for EVEL library
 *
 * This file implements the EVEL library which is intended to provide a
 * simple wrapper around the complexity of AT&T's Vendor Event Listener API so
 * that VNFs can use it without worrying about details of the API transport.
 *
 * Zero return value is success (::EVEL_SUCCESS), non-zero is failure and will
 * be one of ::EVEL_ERR_CODES.
 *
 * License
 * -------
 *
 * Copyright © 2017 AT&T Intellectual Property. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *****************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>

#include "double_list.h"

/*****************************************************************************/
/* Supported API version.                                                    */
/*****************************************************************************/
#define EVEL_API_MAJOR_VERSION 3
#define EVEL_API_MINOR_VERSION 0

/**************************************************************************//**
 * Error codes
 *
 * Error codes for EVEL low level interface
 *****************************************************************************/
typedef enum {
  EVEL_SUCCESS,                   /** The operation was successful.          */
  EVEL_ERR_GEN_FAIL,              /** Non-specific failure.                  */
  EVEL_CURL_LIBRARY_FAIL,         /** A cURL library operation failed.       */
  EVEL_PTHREAD_LIBRARY_FAIL,      /** A Posix threads operation failed.      */
  EVEL_OUT_OF_MEMORY,             /** A memory allocation failure occurred.  */
  EVEL_EVENT_BUFFER_FULL,         /** Too many events in the ring-buffer.    */
  EVEL_EVENT_HANDLER_INACTIVE,    /** Attempt to raise event when inactive.  */
  EVEL_NO_METADATA,               /** Failed to retrieve OpenStack metadata. */
  EVEL_BAD_METADATA,              /** OpenStack metadata invalid format.     */
  EVEL_BAD_JSON_FORMAT,           /** JSON failed to parse correctly.        */
  EVEL_JSON_KEY_NOT_FOUND,        /** Failed to find the specified JSON key. */
  EVEL_MAX_ERROR_CODES            /** Maximum number of valid error codes.   */
} EVEL_ERR_CODES;

/**************************************************************************//**
 * Logging levels
 *
 * Variable levels of verbosity in the logging functions.
 *****************************************************************************/
typedef enum {
  EVEL_LOG_MIN               = 0,
  EVEL_LOG_SPAMMY            = 30,
  EVEL_LOG_DEBUG             = 40,
  EVEL_LOG_INFO              = 50,
  EVEL_LOG_ERROR             = 60,
  EVEL_LOG_MAX               = 101
} EVEL_LOG_LEVELS;

/*****************************************************************************/
/* Maximum string lengths.                                                   */
/*****************************************************************************/
#define EVEL_MAX_STRING_LEN          4096
#define EVEL_MAX_JSON_BODY           16000
#define EVEL_MAX_ERROR_STRING_LEN    255
#define EVEL_MAX_URL_LEN             511

/**************************************************************************//**
 * This value represents there being no restriction on the reporting interval.
 *****************************************************************************/
static const int EVEL_MEASUREMENT_INTERVAL_UKNOWN = 0;

/**************************************************************************//**
 * How many events can be backed-up before we start dropping events on the
 * floor.
 *
 * @note  This value should be tuned in accordance with expected burstiness of
 *        the event load and the expected response time of the ECOMP event
 *        listener so that the probability of the buffer filling is suitably
 *        low.
 *****************************************************************************/
static const int EVEL_EVENT_BUFFER_DEPTH = 100;

/*****************************************************************************/
/* How many different IP Types-of-Service are supported.                     */
/*****************************************************************************/
#define EVEL_TOS_SUPPORTED      256

/**************************************************************************//**
 * Event domains for the various events we support.
 * JSON equivalent field: domain
 *****************************************************************************/
typedef enum {
  EVEL_DOMAIN_INTERNAL,       /** Internal event, not for external routing.  */
  EVEL_DOMAIN_HEARTBEAT,      /** A Heartbeat event (event header only).     */
  EVEL_DOMAIN_FAULT,          /** A Fault event.                             */
  EVEL_DOMAIN_MEASUREMENT,    /** A Measurement for VF Scaling event.        */
  EVEL_DOMAIN_MOBILE_FLOW,    /** A Mobile Flow event.                       */
  EVEL_DOMAIN_REPORT,         /** A Measurement for VF Reporting event.      */
  EVEL_DOMAIN_SERVICE,        /** A Service event.                           */
  EVEL_DOMAIN_SIGNALING,      /** A Signaling event.                         */
  EVEL_DOMAIN_STATE_CHANGE,   /** A State Change event.                      */
  EVEL_DOMAIN_SYSLOG,         /** A Syslog event.                            */
  EVEL_DOMAIN_OTHER,          /** Another event.                             */
  EVEL_MAX_DOMAINS            /** Maximum number of recognized Event types.  */
} EVEL_EVENT_DOMAINS;

/**************************************************************************//**
 * Event priorities.
 * JSON equivalent field: priority
 *****************************************************************************/
typedef enum {
  EVEL_PRIORITY_HIGH,
  EVEL_PRIORITY_MEDIUM,
  EVEL_PRIORITY_NORMAL,
  EVEL_PRIORITY_LOW,
  EVEL_MAX_PRIORITIES
} EVEL_EVENT_PRIORITIES;

/**************************************************************************//**
 * Fault / Threshold severities.
 * JSON equivalent field: eventSeverity
 *****************************************************************************/
typedef enum {
  EVEL_SEVERITY_CRITICAL,
  EVEL_SEVERITY_MAJOR,
  EVEL_SEVERITY_MINOR,
  EVEL_SEVERITY_WARNING,
  EVEL_SEVERITY_NORMAL,
  EVEL_MAX_SEVERITIES
} EVEL_SEVERITIES;

/**************************************************************************//**
 * Fault source types.
 * JSON equivalent field: eventSourceType
 *****************************************************************************/
typedef enum {
  EVEL_SOURCE_OTHER,
  EVEL_SOURCE_ROUTER,
  EVEL_SOURCE_SWITCH,
  EVEL_SOURCE_HOST,
  EVEL_SOURCE_CARD,
  EVEL_SOURCE_PORT,
  EVEL_SOURCE_SLOT_THRESHOLD,
  EVEL_SOURCE_PORT_THRESHOLD,
  EVEL_SOURCE_VIRTUAL_MACHINE,
  EVEL_SOURCE_VIRTUAL_NETWORK_FUNCTION,
  /***************************************************************************/
  /* START OF VENDOR-SPECIFIC VALUES                                         */
  /*                                                                         */
  /* Vendor-specific values should be added here, and handled appropriately  */
  /* in evel_event.c.                                                        */
  /***************************************************************************/

  /***************************************************************************/
  /* END OF VENDOR-SPECIFIC VALUES                                           */
  /***************************************************************************/
  EVEL_MAX_SOURCE_TYPES
} EVEL_SOURCE_TYPES;

/**************************************************************************//**
 * Fault VNF Status.
 * JSON equivalent field: vfStatus
 *****************************************************************************/
typedef enum {
  EVEL_VF_STATUS_ACTIVE,
  EVEL_VF_STATUS_IDLE,
  EVEL_VF_STATUS_PREP_TERMINATE,
  EVEL_VF_STATUS_READY_TERMINATE,
  EVEL_VF_STATUS_REQ_TERMINATE,
  EVEL_MAX_VF_STATUSES
} EVEL_VF_STATUSES;

/**************************************************************************//**
 * Counter criticalities.
 * JSON equivalent field: criticality
 *****************************************************************************/
typedef enum {
  EVEL_COUNTER_CRITICALITY_CRIT,
  EVEL_COUNTER_CRITICALITY_MAJ,
  EVEL_MAX_COUNTER_CRITICALITIES
} EVEL_COUNTER_CRITICALITIES;

/**************************************************************************//**
 * Alert actions.
 * JSON equivalent field: alertAction
 *****************************************************************************/
typedef enum {
  EVEL_ALERT_ACTION_CLEAR,
  EVEL_ALERT_ACTION_CONT,
  EVEL_ALERT_ACTION_SET,
  EVEL_MAX_ALERT_ACTIONS
} EVEL_ALERT_ACTIONS;

/**************************************************************************//**
 * Alert types.
 * JSON equivalent field: alertType
 *****************************************************************************/
typedef enum {
  EVEL_ALERT_TYPE_CARD,
  EVEL_ALERT_TYPE_ELEMENT,
  EVEL_ALERT_TYPE_INTERFACE,
  EVEL_ALERT_TYPE_SERVICE,
  EVEL_MAX_ALERT_TYPES
} EVEL_ALERT_TYPES;

/**************************************************************************//**
 * Alert types.
 * JSON equivalent fields: newState, oldState
 *****************************************************************************/
typedef enum {
  EVEL_ENTITY_STATE_IN_SERVICE,
  EVEL_ENTITY_STATE_MAINTENANCE,
  EVEL_ENTITY_STATE_OUT_OF_SERVICE,
  EVEL_MAX_ENTITY_STATES
} EVEL_ENTITY_STATE;

/**************************************************************************//**
 * Syslog facilities.
 * JSON equivalent field: syslogFacility
 *****************************************************************************/
typedef enum {
  EVEL_SYSLOG_FACILITY_KERNEL,
  EVEL_SYSLOG_FACILITY_USER,
  EVEL_SYSLOG_FACILITY_MAIL,
  EVEL_SYSLOG_FACILITY_SYSTEM_DAEMON,
  EVEL_SYSLOG_FACILITY_SECURITY_AUTH,
  EVEL_SYSLOG_FACILITY_INTERNAL,
  EVEL_SYSLOG_FACILITY_LINE_PRINTER,
  EVEL_SYSLOG_FACILITY_NETWORK_NEWS,
  EVEL_SYSLOG_FACILITY_UUCP,
  EVEL_SYSLOG_FACILITY_CLOCK_DAEMON,
  EVEL_SYSLOG_FACILITY_SECURITY_AUTH2,
  EVEL_SYSLOG_FACILITY_FTP_DAEMON,
  EVEL_SYSLOG_FACILITY_NTP,
  EVEL_SYSLOG_FACILITY_LOG_AUDIT,
  EVEL_SYSLOG_FACILITY_LOG_ALERT,
  EVEL_SYSLOG_FACILITY_CLOCK_DAEMON2,
  EVEL_SYSLOG_FACILITY_LOCAL0,
  EVEL_SYSLOG_FACILITY_LOCAL1,
  EVEL_SYSLOG_FACILITY_LOCAL2,
  EVEL_SYSLOG_FACILITY_LOCAL3,
  EVEL_SYSLOG_FACILITY_LOCAL4,
  EVEL_SYSLOG_FACILITY_LOCAL5,
  EVEL_SYSLOG_FACILITY_LOCAL6,
  EVEL_SYSLOG_FACILITY_LOCAL7,
  EVEL_MAX_SYSLOG_FACILITIES
} EVEL_SYSLOG_FACILITIES;

/**************************************************************************//**
 * TCP flags.
 * JSON equivalent fields: tcpFlagCountList, tcpFlagList
 *****************************************************************************/
typedef enum {
  EVEL_TCP_NS,
  EVEL_TCP_CWR,
  EVEL_TCP_ECE,
  EVEL_TCP_URG,
  EVEL_TCP_ACK,
  EVEL_TCP_PSH,
  EVEL_TCP_RST,
  EVEL_TCP_SYN,
  EVEL_TCP_FIN,
  EVEL_MAX_TCP_FLAGS
} EVEL_TCP_FLAGS;

/**************************************************************************//**
 * Mobile QCI Classes of Service.
 * JSON equivalent fields: mobileQciCosCountList, mobileQciCosList
 *****************************************************************************/
typedef enum {

  /***************************************************************************/
  /* UMTS Classes of Service.                                                */
  /***************************************************************************/
  EVEL_QCI_COS_UMTS_CONVERSATIONAL,
  EVEL_QCI_COS_UMTS_STREAMING,
  EVEL_QCI_COS_UMTS_INTERACTIVE,
  EVEL_QCI_COS_UMTS_BACKGROUND,

  /***************************************************************************/
  /* LTE Classes of Service.                                                 */
  /***************************************************************************/
  EVEL_QCI_COS_LTE_1,
  EVEL_QCI_COS_LTE_2,
  EVEL_QCI_COS_LTE_3,
  EVEL_QCI_COS_LTE_4,
  EVEL_QCI_COS_LTE_65,
  EVEL_QCI_COS_LTE_66,
  EVEL_QCI_COS_LTE_5,
  EVEL_QCI_COS_LTE_6,
  EVEL_QCI_COS_LTE_7,
  EVEL_QCI_COS_LTE_8,
  EVEL_QCI_COS_LTE_9,
  EVEL_QCI_COS_LTE_69,
  EVEL_QCI_COS_LTE_70,
  EVEL_MAX_QCI_COS_TYPES
} EVEL_QCI_COS_TYPES;

/**************************************************************************//**
 * Service Event endpoint description
 * JSON equivalent field: endpointDesc
 *****************************************************************************/
typedef enum {
  EVEL_SERVICE_ENDPOINT_CALLEE,
  EVEL_SERVICE_ENDPOINT_CALLER,
  EVEL_MAX_SERVICE_ENDPOINT_DESC
} EVEL_SERVICE_ENDPOINT_DESC;

/**************************************************************************//**
 * Boolean type for EVEL library.
 *****************************************************************************/
typedef enum {
  EVEL_FALSE,
  EVEL_TRUE
} EVEL_BOOLEAN;

/**************************************************************************//**
 * Optional parameter holder for double.
 *****************************************************************************/
typedef struct evel_option_double
{
  double value;
  EVEL_BOOLEAN is_set;
} EVEL_OPTION_DOUBLE;

/**************************************************************************//**
 * Optional parameter holder for string.
 *****************************************************************************/
typedef struct evel_option_string
{
  char * value;
  EVEL_BOOLEAN is_set;
} EVEL_OPTION_STRING;

/**************************************************************************//**
 * Optional parameter holder for int.
 *****************************************************************************/
typedef struct evel_option_int
{
  int value;
  EVEL_BOOLEAN is_set;
} EVEL_OPTION_INT;

/**************************************************************************//**
 * Optional parameter holder for unsigned long long.
 *****************************************************************************/
typedef struct evel_option_ull
{
  unsigned long long value;
  EVEL_BOOLEAN is_set;
} EVEL_OPTION_ULL;

/**************************************************************************//**
 * Optional parameter holder for time_t.
 *****************************************************************************/
typedef struct evel_option_time
{
  time_t value;
  EVEL_BOOLEAN is_set;
} EVEL_OPTION_TIME;

/*****************************************************************************/
/* Supported Common Event Header version.                                    */
/*****************************************************************************/
#define EVEL_HEADER_MAJOR_VERSION 1
#define EVEL_HEADER_MINOR_VERSION 2

/**************************************************************************//**
 * Event header.
 * JSON equivalent field: commonEventHeader
 *****************************************************************************/
typedef struct event_header {
  /***************************************************************************/
  /* Version                                                                 */
  /***************************************************************************/
  int major_version;
  int minor_version;

  /***************************************************************************/
  /* Mandatory fields                                                        */
  /***************************************************************************/
  EVEL_EVENT_DOMAINS event_domain;
  char * event_id;
  char * source_name;
  char * functional_role;
  char * reporting_entity_name;
  EVEL_EVENT_PRIORITIES priority;
  unsigned long long start_epoch_microsec;
  unsigned long long last_epoch_microsec;
  int sequence;

  /***************************************************************************/
  /* Optional fields                                                         */
  /***************************************************************************/
  EVEL_OPTION_STRING event_type;
  EVEL_OPTION_STRING source_id;
  EVEL_OPTION_STRING reporting_entity_id;

} EVENT_HEADER;

/*****************************************************************************/
/* Supported Fault version.                                                  */
/*****************************************************************************/
#define EVEL_FAULT_MAJOR_VERSION 1
#define EVEL_FAULT_MINOR_VERSION 1

/**************************************************************************//**
 * Fault.
 * JSON equivalent field: faultFields
 *****************************************************************************/
typedef struct event_fault {
  /***************************************************************************/
  /* Header and version                                                      */
  /***************************************************************************/
  EVENT_HEADER header;
  int major_version;
  int minor_version;

  /***************************************************************************/
  /* Mandatory fields                                                        */
  /***************************************************************************/
  EVEL_SEVERITIES event_severity;
  EVEL_SOURCE_TYPES event_source_type;
  char * alarm_condition;
  char * specific_problem;
  EVEL_VF_STATUSES vf_status;

  /***************************************************************************/
  /* Optional fields                                                         */
  /***************************************************************************/
  EVEL_OPTION_STRING alarm_interface_a;
  DLIST additional_info;

} EVENT_FAULT;

/**************************************************************************//**
 * Fault Additional Info.
 * JSON equivalent field: alarmAdditionalInformation
 *****************************************************************************/
typedef struct fault_additional_info {
  char * name;
  char * value;
} FAULT_ADDL_INFO;

/*****************************************************************************/
/* Supported Measurement version.                                            */
/*****************************************************************************/
#define EVEL_MEASUREMENT_MAJOR_VERSION 1
#define EVEL_MEASUREMENT_MINOR_VERSION 1

/**************************************************************************//**
 * Errors.
 * JSON equivalent field: errors
 *****************************************************************************/
typedef struct measurement_errors {
  int receive_discards;
  int receive_errors;
  int transmit_discards;
  int transmit_errors;
} MEASUREMENT_ERRORS;

/**************************************************************************//**
 * Measurement.
 * JSON equivalent field: measurementsForVfScalingFields
 *****************************************************************************/
typedef struct event_measurement {
  /***************************************************************************/
  /* Header and version                                                      */
  /***************************************************************************/
  EVENT_HEADER header;
  int major_version;
  int minor_version;

  /***************************************************************************/
  /* Mandatory fields                                                        */
  /***************************************************************************/
  double measurement_interval;

  /***************************************************************************/
  /* Optional fields                                                         */
  /***************************************************************************/
  DLIST additional_measurements;
  EVEL_OPTION_DOUBLE aggregate_cpu_usage;
  DLIST codec_usage;
  EVEL_OPTION_INT concurrent_sessions;
  EVEL_OPTION_INT configured_entities;
  DLIST cpu_usage;
  MEASUREMENT_ERRORS * errors;
  DLIST feature_usage;
  DLIST filesystem_usage;
  DLIST latency_distribution;
  EVEL_OPTION_DOUBLE mean_request_latency;
  EVEL_OPTION_DOUBLE memory_configured;
  EVEL_OPTION_DOUBLE memory_used;
  EVEL_OPTION_INT media_ports_in_use;
  EVEL_OPTION_INT request_rate;
  EVEL_OPTION_DOUBLE vnfc_scaling_metric;
  DLIST vnic_usage;

} EVENT_MEASUREMENT;

/**************************************************************************//**
 * CPU Usage.
 * JSON equivalent field: cpuUsage
 *****************************************************************************/
typedef struct measurement_cpu_use {
  char * id;
  double usage;
} MEASUREMENT_CPU_USE;

/**************************************************************************//**
 * Filesystem Usage.
 * JSON equivalent field: filesystemUsage
 *****************************************************************************/
typedef struct measurement_fsys_use {
  char * filesystem_name;
  double block_configured;
  int block_iops;
  double block_used;
  double ephemeral_configured;
  int ephemeral_iops;
  double ephemeral_used;
} MEASUREMENT_FSYS_USE;

/**************************************************************************//**
 * Latency Bucket.
 * JSON equivalent field: latencyBucketMeasure
 *****************************************************************************/
typedef struct measurement_latency_bucket {
  int count;

  /***************************************************************************/
  /* Optional fields                                                         */
  /***************************************************************************/
  EVEL_OPTION_DOUBLE high_end;
  EVEL_OPTION_DOUBLE low_end;

} MEASUREMENT_LATENCY_BUCKET;

/**************************************************************************//**
 * Virtual NIC usage.
 * JSON equivalent field: vNicUsage
 *****************************************************************************/
typedef struct measurement_vnic_use {
  int bytes_in;
  int bytes_out;
  int packets_in;
  int packets_out;
  char * vnic_id;

  /***************************************************************************/
  /* Optional fields                                                         */
  /***************************************************************************/
  EVEL_OPTION_INT broadcast_packets_in;
  EVEL_OPTION_INT broadcast_packets_out;
  EVEL_OPTION_INT multicast_packets_in;
  EVEL_OPTION_INT multicast_packets_out;
  EVEL_OPTION_INT unicast_packets_in;
  EVEL_OPTION_INT unicast_packets_out;

} MEASUREMENT_VNIC_USE;

/**************************************************************************//**
 * Codec Usage.
 * JSON equivalent field: codecsInUse
 *****************************************************************************/
typedef struct measurement_codec_use {
  char * codec_id;
  int number_in_use;
} MEASUREMENT_CODEC_USE;

/**************************************************************************//**
 * Feature Usage.
 * JSON equivalent field: featuresInUse
 *****************************************************************************/
typedef struct measurement_feature_use {
  char * feature_id;
  int feature_utilization;
} MEASUREMENT_FEATURE_USE;

/**************************************************************************//**
 * Measurement Group.
 * JSON equivalent field: additionalMeasurements
 *****************************************************************************/
typedef struct measurement_group {
  char * name;
  DLIST measurements;
} MEASUREMENT_GROUP;

/**************************************************************************//**
 * Custom Defined Measurement.
 * JSON equivalent field: measurements
 *****************************************************************************/
typedef struct custom_measurement {
  char * name;
  char * value;
} CUSTOM_MEASUREMENT;

/*****************************************************************************/
/* Supported Report version.                                                 */
/*****************************************************************************/
#define EVEL_REPORT_MAJOR_VERSION 1
#define EVEL_REPORT_MINOR_VERSION 1

/**************************************************************************//**
 * Report.
 * JSON equivalent field: measurementsForVfReportingFields
 *
 * @note  This is an experimental event type and is not currently a formal part
 *        of AT&T's specification.
 *****************************************************************************/
typedef struct event_report {
  /***************************************************************************/
  /* Header and version                                                      */
  /***************************************************************************/
  EVENT_HEADER header;
  int major_version;
  int minor_version;

  /***************************************************************************/
  /* Mandatory fields                                                        */
  /***************************************************************************/
  double measurement_interval;

  /***************************************************************************/
  /* Optional fields                                                         */
  /***************************************************************************/
  DLIST feature_usage;
  DLIST measurement_groups;

} EVENT_REPORT;

/**************************************************************************//**
 * Mobile GTP Per Flow Metrics.
 * JSON equivalent field: gtpPerFlowMetrics
 *****************************************************************************/
typedef struct mobile_gtp_per_flow_metrics {
  double avg_bit_error_rate;
  double avg_packet_delay_variation;
  int avg_packet_latency;
  int avg_receive_throughput;
  int avg_transmit_throughput;
  int flow_activation_epoch;
  int flow_activation_microsec;
  int flow_deactivation_epoch;
  int flow_deactivation_microsec;
  time_t flow_deactivation_time;
  char * flow_status;
  int max_packet_delay_variation;
  int num_activation_failures;
  int num_bit_errors;
  int num_bytes_received;
  int num_bytes_transmitted;
  int num_dropped_packets;
  int num_l7_bytes_received;
  int num_l7_bytes_transmitted;
  int num_lost_packets;
  int num_out_of_order_packets;
  int num_packet_errors;
  int num_packets_received_excl_retrans;
  int num_packets_received_incl_retrans;
  int num_packets_transmitted_incl_retrans;
  int num_retries;
  int num_timeouts;
  int num_tunneled_l7_bytes_received;
  int round_trip_time;
  int time_to_first_byte;

  /***************************************************************************/
  /* Optional fields                                                         */
  /***************************************************************************/
  EVEL_OPTION_INT ip_tos_counts[EVEL_TOS_SUPPORTED];
  EVEL_OPTION_INT tcp_flag_counts[EVEL_MAX_TCP_FLAGS];
  EVEL_OPTION_INT qci_cos_counts[EVEL_MAX_QCI_COS_TYPES];
  EVEL_OPTION_INT dur_connection_failed_status;
  EVEL_OPTION_INT dur_tunnel_failed_status;
  EVEL_OPTION_STRING flow_activated_by;
  EVEL_OPTION_TIME flow_activation_time;
  EVEL_OPTION_STRING flow_deactivated_by;
  EVEL_OPTION_STRING gtp_connection_status;
  EVEL_OPTION_STRING gtp_tunnel_status;
  EVEL_OPTION_INT large_packet_rtt;
  EVEL_OPTION_DOUBLE large_packet_threshold;
  EVEL_OPTION_INT max_receive_bit_rate;
  EVEL_OPTION_INT max_transmit_bit_rate;
  EVEL_OPTION_INT num_gtp_echo_failures;
  EVEL_OPTION_INT num_gtp_tunnel_errors;
  EVEL_OPTION_INT num_http_errors;

} MOBILE_GTP_PER_FLOW_METRICS;

/*****************************************************************************/
/* Supported Mobile Flow version.                                            */
/*****************************************************************************/
#define EVEL_MOBILE_FLOW_MAJOR_VERSION 1
#define EVEL_MOBILE_FLOW_MINOR_VERSION 1

/**************************************************************************//**
 * Mobile Flow.
 * JSON equivalent field: mobileFlow
 *****************************************************************************/
typedef struct event_mobile_flow {
  /***************************************************************************/
  /* Header and version                                                      */
  /***************************************************************************/
  EVENT_HEADER header;
  int major_version;
  int minor_version;

  /***************************************************************************/
  /* Mandatory fields                                                        */
  /***************************************************************************/
  char * flow_direction;
  MOBILE_GTP_PER_FLOW_METRICS * gtp_per_flow_metrics;
  char * ip_protocol_type;
  char * ip_version;
  char * other_endpoint_ip_address;
  int other_endpoint_port;
  char * reporting_endpoint_ip_addr;
  int reporting_endpoint_port;

  /***************************************************************************/
  /* Optional fields                                                         */
  /***************************************************************************/
  EVEL_OPTION_STRING application_type;
  EVEL_OPTION_STRING app_protocol_type;
  EVEL_OPTION_STRING app_protocol_version;
  EVEL_OPTION_STRING cid;
  EVEL_OPTION_STRING connection_type;
  EVEL_OPTION_STRING ecgi;
  EVEL_OPTION_STRING gtp_protocol_type;
  EVEL_OPTION_STRING gtp_version;
  EVEL_OPTION_STRING http_header;
  EVEL_OPTION_STRING imei;
  EVEL_OPTION_STRING imsi;
  EVEL_OPTION_STRING lac;
  EVEL_OPTION_STRING mcc;
  EVEL_OPTION_STRING mnc;
  EVEL_OPTION_STRING msisdn;
  EVEL_OPTION_STRING other_functional_role;
  EVEL_OPTION_STRING rac;
  EVEL_OPTION_STRING radio_access_technology;
  EVEL_OPTION_STRING sac;
  EVEL_OPTION_INT sampling_algorithm;
  EVEL_OPTION_STRING tac;
  EVEL_OPTION_STRING tunnel_id;
  EVEL_OPTION_STRING vlan_id;

} EVENT_MOBILE_FLOW;

/**************************************************************************//**
 * Other.
 * JSON equivalent field: otherFields
 *****************************************************************************/
typedef struct event_other {
  EVENT_HEADER header;
  DLIST other_fields;

} EVENT_OTHER;

/**************************************************************************//**
 * Other Field.
 * JSON equivalent field: otherFields
 *****************************************************************************/
typedef struct other_field {
  char * name;
  char * value;
} OTHER_FIELD;

/**************************************************************************//**
 * Event Instance Identifier
 * JSON equivalent field: eventInstanceIdentifier
 *****************************************************************************/
typedef struct evel_event_instance_id {

  /***************************************************************************/
  /* Mandatory fields                                                        */
  /***************************************************************************/
  char * vendor_id;                                        /* JSON: vendorId */
  char * event_id;                                          /* JSON: eventId */

  /***************************************************************************/
  /* Optional fields                                                         */
  /***************************************************************************/
  EVEL_OPTION_STRING product_id;                          /* JSON: productId */
  EVEL_OPTION_STRING subsystem_id;                      /* JSON: subsystemId */
  EVEL_OPTION_STRING event_friendly_name;         /* JSON: eventFriendlyName */

} EVEL_EVENT_INSTANCE_ID;

/*****************************************************************************/
/* Supported Service Events version.                                         */
/*****************************************************************************/
#define EVEL_SERVICE_MAJOR_VERSION 1
#define EVEL_SERVICE_MINOR_VERSION 1

/**************************************************************************//**
 * Service Events.
 * JSON equivalent field: serviceEventsFields
 *****************************************************************************/
typedef struct event_service {
  /***************************************************************************/
  /* Header and version                                                      */
  /***************************************************************************/
  EVENT_HEADER header;
  int major_version;
  int minor_version;

  /***************************************************************************/
  /* Mandatory fields                                                        */
  /***************************************************************************/
  EVEL_EVENT_INSTANCE_ID instance_id;       /* JSON: eventInstanceIdentifier */

  /***************************************************************************/
  /* Optional fields.                                                        */
  /***************************************************************************/
  EVEL_OPTION_STRING correlator;                         /* JSON: correlator */
  DLIST additional_fields;                         /* JSON: additionalFields */

  /***************************************************************************/
  /* Optional fields within JSON equivalent object: codecSelected            */
  /***************************************************************************/
  EVEL_OPTION_STRING codec;                                   /* JSON: codec */

  /***************************************************************************/
  /* Optional fields within JSON equivalent object: codecSelectedTranscoding */
  /***************************************************************************/
  EVEL_OPTION_STRING callee_side_codec;             /* JSON: calleeSideCodec */
  EVEL_OPTION_STRING caller_side_codec;             /* JSON: callerSideCodec */

  /***************************************************************************/
  /* Optional fields within JSON equivalent object: midCallRtcp              */
  /***************************************************************************/
  EVEL_OPTION_STRING rtcp_data;                            /* JSON: rtcpData */

  /***************************************************************************/
  /* Optional fields within JSON equivalent object: endOfCallVqmSummaries    */
  /***************************************************************************/
  EVEL_OPTION_STRING adjacency_name;                  /* JSON: adjacencyName */
  EVEL_OPTION_STRING endpoint_description;      /* JSON: endpointDescription */
  EVEL_OPTION_INT endpoint_jitter;                   /* JSON: endpointJitter */
  EVEL_OPTION_INT endpoint_rtp_oct_disc; /* JSON: endpointRtpOctetsDiscarded */
  EVEL_OPTION_INT endpoint_rtp_oct_recv;  /* JSON: endpointRtpOctetsReceived */
  EVEL_OPTION_INT endpoint_rtp_oct_sent;      /* JSON: endpointRtpOctetsSent */
  EVEL_OPTION_INT endpoint_rtp_pkt_disc;/* JSON: endpointRtpPacketsDiscarded */
  EVEL_OPTION_INT endpoint_rtp_pkt_recv; /* JSON: endpointRtpPacketsReceived */
  EVEL_OPTION_INT endpoint_rtp_pkt_sent;     /* JSON: endpointRtpPacketsSent */
  EVEL_OPTION_INT local_jitter;                         /* JSON: localJitter */
  EVEL_OPTION_INT local_rtp_oct_disc;       /* JSON: localRtpOctetsDiscarded */
  EVEL_OPTION_INT local_rtp_oct_recv;        /* JSON: localRtpOctetsReceived */
  EVEL_OPTION_INT local_rtp_oct_sent;            /* JSON: localRtpOctetsSent */
  EVEL_OPTION_INT local_rtp_pkt_disc;      /* JSON: localRtpPacketsDiscarded */
  EVEL_OPTION_INT local_rtp_pkt_recv;       /* JSON: localRtpPacketsReceived */
  EVEL_OPTION_INT local_rtp_pkt_sent;           /* JSON: localRtpPacketsSent */
  EVEL_OPTION_DOUBLE mos_cqe;                                /* JSON: mosCqe */
  EVEL_OPTION_INT packets_lost;                         /* JSON: packetsLost */
  EVEL_OPTION_DOUBLE packet_loss_percent;         /* JSON: packetLossPercent */
  EVEL_OPTION_INT r_factor;                                 /* JSON: rFactor */
  EVEL_OPTION_INT round_trip_delay;                  /* JSON: roundTripDelay */

  /***************************************************************************/
  /* Optional fields within JSON equivalent object: marker                   */
  /***************************************************************************/
  EVEL_OPTION_STRING phone_number;                      /* JSON: phoneNumber */

} EVENT_SERVICE;

/*****************************************************************************/
/* Supported Signaling version.                                              */
/*****************************************************************************/
#define EVEL_SIGNALING_MAJOR_VERSION 1
#define EVEL_SIGNALING_MINOR_VERSION 1

/**************************************************************************//**
 * Signaling.
 * JSON equivalent field: signalingFields
 *****************************************************************************/
typedef struct event_signaling {
  /***************************************************************************/
  /* Header and version                                                      */
  /***************************************************************************/
  EVENT_HEADER header;
  int major_version;
  int minor_version;

  /***************************************************************************/
  /* Mandatory fields                                                        */
  /***************************************************************************/
  EVEL_EVENT_INSTANCE_ID instance_id;       /* JSON: eventInstanceIdentifier */

  /***************************************************************************/
  /* Optional fields                                                         */
  /***************************************************************************/
  EVEL_OPTION_STRING correlator;                         /* JSON: correlator */
  EVEL_OPTION_STRING local_ip_address;               /* JSON: localIpAddress */
  EVEL_OPTION_STRING local_port;                          /* JSON: localPort */
  EVEL_OPTION_STRING remote_ip_address;             /* JSON: remoteIpAddress */
  EVEL_OPTION_STRING remote_port;                        /* JSON: remotePort */
  EVEL_OPTION_STRING compressed_sip;                  /* JSON: compressedSip */
  EVEL_OPTION_STRING summary_sip;                        /* JSON: summarySip */

} EVENT_SIGNALING;

/*****************************************************************************/
/* Supported State Change version.                                           */
/*****************************************************************************/
#define EVEL_STATE_CHANGE_MAJOR_VERSION 1
#define EVEL_STATE_CHANGE_MINOR_VERSION 1

/**************************************************************************//**
 * State Change.
 * JSON equivalent field: stateChangeFields
 *****************************************************************************/
typedef struct event_state_change {
  /***************************************************************************/
  /* Header and version                                                      */
  /***************************************************************************/
  EVENT_HEADER header;
  int major_version;
  int minor_version;

  /***************************************************************************/
  /* Mandatory fields                                                        */
  /***************************************************************************/
  EVEL_ENTITY_STATE new_state;
  EVEL_ENTITY_STATE old_state;
  char * state_interface;

  /***************************************************************************/
  /* Optional fields                                                         */
  /***************************************************************************/
  DLIST additional_fields;

} EVENT_STATE_CHANGE;

/**************************************************************************//**
 * State Change Additional Field.
 * JSON equivalent field: additionalFields
 *****************************************************************************/
typedef struct state_change_additional_field {
  char * name;
  char * value;
} STATE_CHANGE_ADDL_FIELD;

/*****************************************************************************/
/* Supported Syslog version.                                                 */
/*****************************************************************************/
#define EVEL_SYSLOG_MAJOR_VERSION 1
#define EVEL_SYSLOG_MINOR_VERSION 1

/**************************************************************************//**
 * Syslog.
 * JSON equivalent field: syslogFields
 *****************************************************************************/
typedef struct event_syslog {
  /***************************************************************************/
  /* Header and version                                                      */
  /***************************************************************************/
  EVENT_HEADER header;
  int major_version;
  int minor_version;

  /***************************************************************************/
  /* Mandatory fields                                                        */
  /***************************************************************************/
  EVEL_SOURCE_TYPES event_source_type;
  char * syslog_msg;
  char * syslog_tag;

  /***************************************************************************/
  /* Optional fields                                                         */
  /***************************************************************************/
  DLIST additional_fields;
  EVEL_OPTION_STRING event_source_host;
  EVEL_OPTION_INT syslog_facility;
  EVEL_OPTION_STRING syslog_proc;
  EVEL_OPTION_INT syslog_proc_id;
  EVEL_OPTION_STRING syslog_s_data;
  EVEL_OPTION_INT syslog_ver;

} EVENT_SYSLOG;

/**************************************************************************//**
 * Syslog Additional Field.
 * JSON equivalent field: additionalFields
 *****************************************************************************/
typedef struct syslog_additional_field {
  char * name;
  char * value;
} SYSLOG_ADDL_FIELD;

/**************************************************************************//**
 * Copyright.
 * JSON equivalent object: attCopyrightNotice
 *****************************************************************************/
typedef struct copyright {
  char * useAndRedistribution;
  char * condition1;
  char * condition2;
  char * condition3;
  char * condition4;
  char * disclaimerLine1;
  char * disclaimerLine2;
  char * disclaimerLine3;
  char * disclaimerLine4;
} COPYRIGHT;

/**************************************************************************//**
 * Library initialization.
 *
 * Initialize the EVEL library.
 *
 * @note  This function initializes the cURL library.  Applications making use
 *        of libcurl may need to pull the initialization out of here.  Note
 *        also that this function is not threadsafe as a result - refer to
 *        libcurl's API documentation for relevant warnings.
 *
 * @sa  Matching Term function.
 *
 * @param   fqdn    The API's FQDN or IP address.
 * @param   port    The API's port.
 * @param   path    The optional path (may be NULL).
 * @param   topic   The optional topic part of the URL (may be NULL).
 * @param   secure  Whether to use HTTPS (0=HTTP, 1=HTTPS).
 * @param   username  Username for Basic Authentication of requests.
 * @param   password  Password for Basic Authentication of requests.
 * @param   source_type The kind of node we represent.
 * @param   role    The role this node undertakes.
 * @param   verbosity  0 for normal operation, positive values for chattier
 *                     logs.
 *
 * @returns Status code
 * @retval  EVEL_SUCCESS      On success
 * @retval  ::EVEL_ERR_CODES  On failure.
 *****************************************************************************/
EVEL_ERR_CODES evel_initialize(const char * const fqdn,
                               int port,
                               const char * const path,
                               const char * const topic,
                               int secure,
                               const char * const username,
                               const char * const password,
                               EVEL_SOURCE_TYPES source_type,
                               const char * const role,
                               int verbosity
                               );

/**************************************************************************//**
 * Clean up the EVEL library.
 *
 * @note that at present don't expect Init/Term cycling not to leak memory!
 *
 * @returns Status code
 * @retval  EVEL_SUCCESS On success
 * @retval  "One of ::EVEL_ERR_CODES" On failure.
 *****************************************************************************/
EVEL_ERR_CODES evel_terminate(void);

EVEL_ERR_CODES evel_post_event(EVENT_HEADER * event);
const char * evel_error_string(void);


/**************************************************************************//**
 * Free an event.
 *
 * Free off the event supplied.  Will free all the contained allocated memory.
 *
 * @note  It is safe to free a NULL pointer.
 *****************************************************************************/
void evel_free_event(void * event);

/**************************************************************************//**
 * Encode the event as a JSON event object according to AT&T's schema.
 *
 * @param json      Pointer to where to store the JSON encoded data.
 * @param max_size  Size of storage available in json_body.
 * @param event     Pointer to the ::EVENT_HEADER to encode.
 * @returns Number of bytes actually written.
 *****************************************************************************/
int evel_json_encode_event(char * json,
                           int max_size,
                           EVENT_HEADER * event);

/**************************************************************************//**
 * Callback function to provide returned data.
 *
 * Copy data into the supplied buffer, write_callback::ptr, checking size
 * limits.
 *
 * @returns   Number of bytes placed into write_callback::ptr. 0 for EOF.
 *****************************************************************************/
size_t evel_write_callback(void *contents,
                           size_t size,
                           size_t nmemb,
                           void *userp);

/*****************************************************************************/
/*****************************************************************************/
/*                                                                           */
/*   HEARTBEAT - (includes common header, too)                               */
/*                                                                           */
/*****************************************************************************/
/*****************************************************************************/

/**************************************************************************//**
 * Create a new heartbeat event.
 *
 * @note that the heartbeat is just a "naked" commonEventHeader!
 *
 * @returns pointer to the newly manufactured ::EVENT_HEADER.  If the event is
 *          not used it must be released using ::evel_free_event
 * @retval  NULL  Failed to create the event.
 *****************************************************************************/
EVENT_HEADER * evel_new_heartbeat(void);

/**************************************************************************//**
 * Free an event header.
 *
 * Free off the event header supplied.  Will free all the contained allocated
 * memory.
 *
 * @note It does not free the header itself, since that may be part of a
 * larger structure.
 *****************************************************************************/
void evel_free_header(EVENT_HEADER * const event);

/**************************************************************************//**
 * Initialize a newly created event header.
 *
 * @param header  Pointer to the header being initialized.
 *****************************************************************************/
void evel_init_header(EVENT_HEADER * const header);

/**************************************************************************//**
 * Set the Event Type property of the event header.
 *
 * @param header        Pointer to the ::EVENT_HEADER.
 * @param type          The Event Type to be set. ASCIIZ string. The caller
 *                      does not need to preserve the value once the function
 *                      returns.
 *****************************************************************************/
void evel_header_type_set(EVENT_HEADER * const header,
                          const char * const type);

/**************************************************************************//**
 * Set the Start Epoch property of the event header.
 *
 * @note The Start Epoch defaults to the time of event creation.
 *
 * @param header        Pointer to the ::EVENT_HEADER.
 * @param start_epoch_microsec
 *                      The start epoch to set, in microseconds.
 *****************************************************************************/
void evel_start_epoch_set(EVENT_HEADER * const header,
                          const unsigned long long start_epoch_microsec);

/**************************************************************************//**
 * Set the Last Epoch property of the event header.
 *
 * @note The Last Epoch defaults to the time of event creation.
 *
 * @param header        Pointer to the ::EVENT_HEADER.
 * @param last_epoch_microsec
 *                      The last epoch to set, in microseconds.
 *****************************************************************************/
void evel_last_epoch_set(EVENT_HEADER * const header,
                         const unsigned long long last_epoch_microsec);

/**************************************************************************//**
 * Set the Reporting Entity Name property of the event header.
 *
 * @note The Reporting Entity Name defaults to the OpenStack VM Name.
 *
 * @param header        Pointer to the ::EVENT_HEADER.
 * @param entity_name   The entity name to set.
 *****************************************************************************/
void evel_reporting_entity_name_set(EVENT_HEADER * const header,
                                    const char * const entity_name);

/**************************************************************************//**
 * Set the Reporting Entity Id property of the event header.
 *
 * @note The Reporting Entity Id defaults to the OpenStack VM UUID.
 *
 * @param header        Pointer to the ::EVENT_HEADER.
 * @param entity_id     The entity id to set.
 *****************************************************************************/
void evel_reporting_entity_id_set(EVENT_HEADER * const header,
                                  const char * const entity_id);

/*****************************************************************************/
/*****************************************************************************/
/*                                                                           */
/*   FAULT                                                                   */
/*                                                                           */
/*****************************************************************************/
/*****************************************************************************/

/**************************************************************************//**
 * Create a new fault event.
 *
 *
 * @returns pointer to the newly manufactured ::EVENT_FAULT.  If the event is
 *          not used it must be released using ::evel_free_fault
 * @retval  NULL  Failed to create the event.
 *****************************************************************************/
EVENT_FAULT * evel_new_fault(const char * const condition,
                             const char * const specific_problem,
                             EVEL_EVENT_PRIORITIES priority,
                             EVEL_SEVERITIES severity);

/**************************************************************************//**
 * Free a Fault.
 *
 * Free off the Fault supplied.  Will free all the contained allocated memory.
 *
 * @note It does not free the Fault itself, since that may be part of a
 * larger structure.
 *****************************************************************************/
void evel_free_fault(EVENT_FAULT * event);


/**************************************************************************//**
 * Set the Alarm Interface A property of the Fault.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param fault      Pointer to the fault.
 * @param interface  The Alarm Interface A to be set. ASCIIZ string. The caller
 *                   does not need to preserve the value once the function
 *                   returns.
 *****************************************************************************/
void evel_fault_interface_set(EVENT_FAULT * fault,
                              const char * const interface);

/**************************************************************************//**
 * Add an additional value name/value pair to the Fault.
 *
 * The name and value are null delimited ASCII strings.  The library takes
 * a copy so the caller does not have to preserve values after the function
 * returns.
 *
 * @param fault     Pointer to the fault.
 * @param name      ASCIIZ string with the attribute's name.
 * @param value     ASCIIZ string with the attribute's value.
 *****************************************************************************/
void evel_fault_addl_info_add(EVENT_FAULT * fault, char * name, char * value);

/**************************************************************************//**
 * Set the Event Type property of the Fault.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param fault      Pointer to the fault.
 * @param type       The Event Type to be set. ASCIIZ string. The caller
 *                   does not need to preserve the value once the function
 *                   returns.
 *****************************************************************************/
void evel_fault_type_set(EVENT_FAULT * fault, const char * const type);

/*****************************************************************************/
/*****************************************************************************/
/*                                                                           */
/*   MEASUREMENT                                                             */
/*                                                                           */
/*****************************************************************************/
/*****************************************************************************/

/**************************************************************************//**
 * Create a new Measurement event.
 *
 * @note    The mandatory fields on the Measurement must be supplied to this
 *          factory function and are immutable once set.  Optional fields have
 *          explicit setter functions, but again values may only be set once so
 *          that the Measurement has immutable properties.
 *
 * @param   measurement_interval
 *
 * @returns pointer to the newly manufactured ::EVENT_MEASUREMENT.  If the
 *          event is not used (i.e. posted) it must be released using
 *          ::evel_free_event.
 * @retval  NULL  Failed to create the event.
 *****************************************************************************/
EVENT_MEASUREMENT * evel_new_measurement(double measurement_interval);

/**************************************************************************//**
 * Free a Measurement.
 *
 * Free off the Measurement supplied.  Will free all the contained allocated
 * memory.
 *
 * @note It does not free the Measurement itself, since that may be part of a
 * larger structure.
 *****************************************************************************/
void evel_free_measurement(EVENT_MEASUREMENT * event);

/**************************************************************************//**
 * Set the Event Type property of the Measurement.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param measurement Pointer to the Measurement.
 * @param type        The Event Type to be set. ASCIIZ string. The caller
 *                    does not need to preserve the value once the function
 *                    returns.
 *****************************************************************************/
void evel_measurement_type_set(EVENT_MEASUREMENT * measurement,
                               const char * const type);

/**************************************************************************//**
 * Set the Concurrent Sessions property of the Measurement.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param measurement         Pointer to the Measurement.
 * @param concurrent_sessions The Concurrent Sessions to be set.
 *****************************************************************************/
void evel_measurement_conc_sess_set(EVENT_MEASUREMENT * measurement,
                                    int concurrent_sessions);

/**************************************************************************//**
 * Set the Configured Entities property of the Measurement.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param measurement         Pointer to the Measurement.
 * @param configured_entities The Configured Entities to be set.
 *****************************************************************************/
void evel_measurement_cfg_ents_set(EVENT_MEASUREMENT * measurement,
                                   int configured_entities);

/**************************************************************************//**
 * Add an additional set of Errors to the Measurement.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param measurement       Pointer to the measurement.
 * @param receive_discards  The number of receive discards.
 * @param receive_errors    The number of receive errors.
 * @param transmit_discards The number of transmit discards.
 * @param transmit_errors   The number of transmit errors.
 *****************************************************************************/
void evel_measurement_errors_set(EVENT_MEASUREMENT * measurement,
                                 int receive_discards,
                                 int receive_errors,
                                 int transmit_discards,
                                 int transmit_errors);

/**************************************************************************//**
 * Set the Mean Request Latency property of the Measurement.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param measurement          Pointer to the Measurement.
 * @param mean_request_latency The Mean Request Latency to be set.
 *****************************************************************************/
void evel_measurement_mean_req_lat_set(EVENT_MEASUREMENT * measurement,
                                       double mean_request_latency);

/**************************************************************************//**
 * Set the Memory Configured property of the Measurement.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param measurement       Pointer to the Measurement.
 * @param memory_configured The Memory Configured to be set.
 *****************************************************************************/
void evel_measurement_mem_cfg_set(EVENT_MEASUREMENT * measurement,
                                  double memory_configured);

/**************************************************************************//**
 * Set the Memory Used property of the Measurement.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param measurement       Pointer to the Measurement.
 * @param memory_used The Memory Used to be set.
 *****************************************************************************/
void evel_measurement_mem_used_set(EVENT_MEASUREMENT * measurement,
                                   double memory_used);

/**************************************************************************//**
 * Set the Request Rate property of the Measurement.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param measurement  Pointer to the Measurement.
 * @param request_rate The Request Rate to be set.
 *****************************************************************************/
void evel_measurement_request_rate_set(EVENT_MEASUREMENT * measurement,
                                       int request_rate);

/**************************************************************************//**
 * Add an additional CPU usage value name/value pair to the Measurement.
 *
 * The name and value are null delimited ASCII strings.  The library takes
 * a copy so the caller does not have to preserve values after the function
 * returns.
 *
 * @param measurement   Pointer to the measurement.
 * @param id            ASCIIZ string with the CPU's identifier.
 * @param usage         CPU utilization.
 *****************************************************************************/
void evel_measurement_cpu_use_add(EVENT_MEASUREMENT * measurement,
                                  char * id, double usage);

/**************************************************************************//**
 * Add an additional File System usage value name/value pair to the
 * Measurement.
 *
 * The filesystem_name is null delimited ASCII string.  The library takes a
 * copy so the caller does not have to preserve values after the function
 * returns.
 *
 * @param measurement     Pointer to the measurement.
 * @param filesystem_name   ASCIIZ string with the file-system's UUID.
 * @param block_configured  Block storage configured.
 * @param block_used        Block storage in use.
 * @param block_iops        Block storage IOPS.
 * @param ephemeral_configured  Ephemeral storage configured.
 * @param ephemeral_used        Ephemeral storage in use.
 * @param ephemeral_iops        Ephemeral storage IOPS.
 *****************************************************************************/
void evel_measurement_fsys_use_add(EVENT_MEASUREMENT * measurement,
                                   char * filesystem_name,
                                   double block_configured,
                                   double block_used,
                                   int block_iops,
                                   double ephemeral_configured,
                                   double ephemeral_used,
                                   int ephemeral_iops);

/**************************************************************************//**
 * Add a Feature usage value name/value pair to the Measurement.
 *
 * The name is null delimited ASCII string.  The library takes
 * a copy so the caller does not have to preserve values after the function
 * returns.
 *
 * @param measurement     Pointer to the measurement.
 * @param feature         ASCIIZ string with the feature's name.
 * @param utilization     Utilization of the feature.
 *****************************************************************************/
void evel_measurement_feature_use_add(EVENT_MEASUREMENT * measurement,
                                      char * feature,
                                      int utilization);

/**************************************************************************//**
 * Add a Additional Measurement value name/value pair to the Measurement.
 *
 * The name is null delimited ASCII string.  The library takes
 * a copy so the caller does not have to preserve values after the function
 * returns.
 *
 * @param measurement   Pointer to the Measurement.
 * @param group    ASCIIZ string with the measurement group's name.
 * @param name     ASCIIZ string containing the measurement's name.
 * @param name     ASCIIZ string containing the measurement's value.
 *****************************************************************************/
void evel_measurement_custom_measurement_add(EVENT_MEASUREMENT * measurement,
                                             const char * const group,
                                             const char * const name,
                                             const char * const value);

/**************************************************************************//**
 * Add a Codec usage value name/value pair to the Measurement.
 *
 * The name is null delimited ASCII string.  The library takes
 * a copy so the caller does not have to preserve values after the function
 * returns.
 *
 * @param measurement     Pointer to the measurement.
 * @param codec           ASCIIZ string with the codec's name.
 * @param utilization     Utilization of the feature.
 *****************************************************************************/
void evel_measurement_codec_use_add(EVENT_MEASUREMENT * measurement,
                                    char * codec,
                                    int utilization);

/**************************************************************************//**
}
 * Set the Aggregate CPU Use property of the Measurement.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param measurement   Pointer to the measurement.
 * @param cpu_use       The CPU use to set.
 *****************************************************************************/
void evel_measurement_agg_cpu_use_set(EVENT_MEASUREMENT * measurement,
                                      double cpu_use);

/**************************************************************************//**
 * Set the Media Ports in Use property of the Measurement.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param measurement         Pointer to the measurement.
 * @param media_ports_in_use  The media port usage to set.
 *****************************************************************************/
void evel_measurement_media_port_use_set(EVENT_MEASUREMENT * measurement,
                                         int media_ports_in_use);

/**************************************************************************//**
 * Set the VNFC Scaling Metric property of the Measurement.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param measurement     Pointer to the measurement.
 * @param scaling_metric  The scaling metric to set.
 *****************************************************************************/
void evel_measurement_vnfc_scaling_metric_set(EVENT_MEASUREMENT * measurement,
                                              double scaling_metric);

/**************************************************************************//**
 * Create a new Latency Bucket to be added to a Measurement event.
 *
 * @note    The mandatory fields on the ::MEASUREMENT_LATENCY_BUCKET must be
 *          supplied to this factory function and are immutable once set.
 *          Optional fields have explicit setter functions, but again values
 *          may only be set once so that the ::MEASUREMENT_LATENCY_BUCKET has
 *          immutable properties.
 *
 * @param count         Count of events in this bucket.
 *
 * @returns pointer to the newly manufactured ::MEASUREMENT_LATENCY_BUCKET.
 * @retval  NULL  Failed to create the Latency Bucket.
 *****************************************************************************/
MEASUREMENT_LATENCY_BUCKET * evel_new_meas_latency_bucket(const int count);

/**************************************************************************//**
 * Set the High End property of the Measurement Latency Bucket.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param bucket        Pointer to the Measurement Latency Bucket.
 * @param high_end      High end of the bucket's range.
 *****************************************************************************/
void evel_meas_latency_bucket_high_end_set(
                                     MEASUREMENT_LATENCY_BUCKET * const bucket,
                                     const double high_end);

/**************************************************************************//**
 * Set the Low End property of the Measurement Latency Bucket.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param bucket        Pointer to the Measurement Latency Bucket.
 * @param low_end       Low end of the bucket's range.
 *****************************************************************************/
void evel_meas_latency_bucket_low_end_set(
                                     MEASUREMENT_LATENCY_BUCKET * const bucket,
                                     const double low_end);

/**************************************************************************//**
 * Add an additional Measurement Latency Bucket to the specified event.
 *
 * @param measurement   Pointer to the Measurement event.
 * @param bucket        Pointer to the Measurement Latency Bucket to add.
 *****************************************************************************/
void evel_meas_latency_bucket_add(EVENT_MEASUREMENT * const measurement,
                                  MEASUREMENT_LATENCY_BUCKET * const bucket);

/**************************************************************************//**
 * Add an additional Latency Distribution bucket to the Measurement.
 *
 * This function implements the previous API, purely for convenience.
 *
 * @param measurement   Pointer to the measurement.
 * @param low_end       Low end of the bucket's range.
 * @param high_end      High end of the bucket's range.
 * @param count         Count of events in this bucket.
 *****************************************************************************/
void evel_measurement_latency_add(EVENT_MEASUREMENT * const measurement,
                                  const double low_end,
                                  const double high_end,
                                  const int count);

/**************************************************************************//**
 * Create a new vNIC Use to be added to a Measurement event.
 *
 * @note    The mandatory fields on the ::MEASUREMENT_VNIC_USE must be supplied
 *          to this factory function and are immutable once set. Optional
 *          fields have explicit setter functions, but again values may only be
 *          set once so that the ::MEASUREMENT_VNIC_USE has immutable
 *          properties.
 *
 * @param vnic_id               ASCIIZ string with the vNIC's ID.
 * @param packets_in            Total packets received.
 * @param packets_out           Total packets transmitted.
 * @param bytes_in              Total bytes received.
 * @param bytes_out             Total bytes transmitted.
 *
 * @returns pointer to the newly manufactured ::MEASUREMENT_VNIC_USE.
 *          If the structure is not used it must be released using
 *          ::evel_free_measurement_vnic_use.
 * @retval  NULL  Failed to create the vNIC Use.
 *****************************************************************************/
MEASUREMENT_VNIC_USE * evel_new_measurement_vnic_use(char * const vnic_id,
                                                     const int packets_in,
                                                     const int packets_out,
                                                     const int bytes_in,
                                                     const int bytes_out);

/**************************************************************************//**
 * Free a vNIC Use.
 *
 * Free off the ::MEASUREMENT_VNIC_USE supplied.  Will free all the contained
 * allocated memory.
 *
 * @note It does not free the vNIC Use itself, since that may be part of a
 * larger structure.
 *****************************************************************************/
void evel_free_measurement_vnic_use(MEASUREMENT_VNIC_USE * const vnic_use);

/**************************************************************************//**
 * Set the Broadcast Packets Received property of the vNIC Use.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param vnic_use      Pointer to the vNIC Use.
 * @param broadcast_packets_in
 *                      Broadcast packets received.
 *****************************************************************************/
void evel_vnic_use_bcast_pkt_in_set(MEASUREMENT_VNIC_USE * const vnic_use,
                                    const int broadcast_packets_in);

/**************************************************************************//**
 * Set the Broadcast Packets Transmitted property of the vNIC Use.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param vnic_use      Pointer to the vNIC Use.
 * @param broadcast_packets_out
 *                      Broadcast packets transmitted.
 *****************************************************************************/
void evel_vnic_use_bcast_pkt_out_set(MEASUREMENT_VNIC_USE * const vnic_use,
                                     const int broadcast_packets_out);

/**************************************************************************//**
 * Set the Multicast Packets Received property of the vNIC Use.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param vnic_use      Pointer to the vNIC Use.
 * @param multicast_packets_in
 *                      Multicast packets received.
 *****************************************************************************/
void evel_vnic_use_mcast_pkt_in_set(MEASUREMENT_VNIC_USE * const vnic_use,
                                    const int multicast_packets_in);

/**************************************************************************//**
 * Set the Multicast Packets Transmitted property of the vNIC Use.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param vnic_use      Pointer to the vNIC Use.
 * @param multicast_packets_out
 *                      Multicast packets transmitted.
 *****************************************************************************/
void evel_vnic_use_mcast_pkt_out_set(MEASUREMENT_VNIC_USE * const vnic_use,
                                     const int multicast_packets_out);

/**************************************************************************//**
 * Set the Unicast Packets Received property of the vNIC Use.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param vnic_use      Pointer to the vNIC Use.
 * @param unicast_packets_in
 *                      Unicast packets received.
 *****************************************************************************/
void evel_vnic_use_ucast_pkt_in_set(MEASUREMENT_VNIC_USE * const vnic_use,
                                    const int unicast_packets_in);

/**************************************************************************//**
 * Set the Unicast Packets Transmitted property of the vNIC Use.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param vnic_use      Pointer to the vNIC Use.
 * @param unicast_packets_out
 *                      Unicast packets transmitted.
 *****************************************************************************/
void evel_vnic_use_ucast_pkt_out_set(MEASUREMENT_VNIC_USE * const vnic_use,
                                     const int unicast_packets_out);

/**************************************************************************//**
 * Add an additional vNIC Use to the specified Measurement event.
 *
 * @param measurement   Pointer to the measurement.
 * @param vnic_use      Pointer to the vNIC Use to add.
 *****************************************************************************/
void evel_meas_vnic_use_add(EVENT_MEASUREMENT * const measurement,
                            MEASUREMENT_VNIC_USE * const vnic_use);

/**************************************************************************//**
 * Add an additional vNIC usage record Measurement.
 *
 * This function implements the previous API, purely for convenience.
 *
 * The ID is null delimited ASCII string.  The library takes a copy so the
 * caller does not have to preserve values after the function returns.
 *
 * @param measurement           Pointer to the measurement.
 * @param vnic_id               ASCIIZ string with the vNIC's ID.
 * @param packets_in            Total packets received.
 * @param packets_out           Total packets transmitted.
 * @param broadcast_packets_in  Broadcast packets received.
 * @param broadcast_packets_out Broadcast packets transmitted.
 * @param bytes_in              Total bytes received.
 * @param bytes_out             Total bytes transmitted.
 * @param multicast_packets_in  Multicast packets received.
 * @param multicast_packets_out Multicast packets transmitted.
 * @param unicast_packets_in    Unicast packets received.
 * @param unicast_packets_out   Unicast packets transmitted.
 *****************************************************************************/
void evel_measurement_vnic_use_add(EVENT_MEASUREMENT * const measurement,
                                   char * const vnic_id,
                                   const int packets_in,
                                   const int packets_out,
                                   const int broadcast_packets_in,
                                   const int broadcast_packets_out,
                                   const int bytes_in,
                                   const int bytes_out,
                                   const int multicast_packets_in,
                                   const int multicast_packets_out,
                                   const int unicast_packets_in,
                                   const int unicast_packets_out);

/*****************************************************************************/
/*****************************************************************************/
/*                                                                           */
/*   REPORT                                                                  */
/*                                                                           */
/*****************************************************************************/
/*****************************************************************************/

/**************************************************************************//**
 * Create a new Report event.
 *
 * @note    The mandatory fields on the Report must be supplied to this
 *          factory function and are immutable once set.  Optional fields have
 *          explicit setter functions, but again values may only be set once so
 *          that the Report has immutable properties.
 *
 * @param   measurement_interval
 *
 * @returns pointer to the newly manufactured ::EVENT_REPORT.  If the event is
 *          not used (i.e. posted) it must be released using
 *          ::evel_free_report.
 * @retval  NULL  Failed to create the event.
 *****************************************************************************/
EVENT_REPORT * evel_new_report(double measurement_interval);

/**************************************************************************//**
 * Free a Report.
 *
 * Free off the Report supplied.  Will free all the contained allocated memory.
 *
 * @note It does not free the Report itself, since that may be part of a
 * larger structure.
 *****************************************************************************/
void evel_free_report(EVENT_REPORT * event);

/**************************************************************************//**
 * Set the Event Type property of the Report.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param report Pointer to the Report.
 * @param type        The Event Type to be set. ASCIIZ string. The caller
 *                    does not need to preserve the value once the function
 *                    returns.
 *****************************************************************************/
void evel_report_type_set(EVENT_REPORT * report, const char * const type);

/**************************************************************************//**
 * Add a Feature usage value name/value pair to the Report.
 *
 * The name is null delimited ASCII string.  The library takes
 * a copy so the caller does not have to preserve values after the function
 * returns.
 *
 * @param report          Pointer to the report.
 * @param feature         ASCIIZ string with the feature's name.
 * @param utilization     Utilization of the feature.
 *****************************************************************************/
void evel_report_feature_use_add(EVENT_REPORT * report,
                                 char * feature,
                                 int utilization);

/**************************************************************************//**
 * Add a Additional Measurement value name/value pair to the Report.
 *
 * The name is null delimited ASCII string.  The library takes
 * a copy so the caller does not have to preserve values after the function
 * returns.
 *
 * @param report   Pointer to the report.
 * @param group    ASCIIZ string with the measurement group's name.
 * @param name     ASCIIZ string containing the measurement's name.
 * @param value    ASCIIZ string containing the measurement's value.
 *****************************************************************************/
void evel_report_custom_measurement_add(EVENT_REPORT * report,
                                        const char * const group,
                                        const char * const name,
                                        const char * const value);

/*****************************************************************************/
/*****************************************************************************/
/*                                                                           */
/*   MOBILE_FLOW                                                             */
/*                                                                           */
/*****************************************************************************/
/*****************************************************************************/

/**************************************************************************//**
 * Create a new Mobile Flow event.
 *
 * @note    The mandatory fields on the Mobile Flow must be supplied to this
 *          factory function and are immutable once set.  Optional fields have
 *          explicit setter functions, but again values may only be set once so
 *          that the Mobile Flow has immutable properties.
 *
 * @param   flow_direction
 * @param   gtp_per_flow_metrics
 * @param   ip_protocol_type
 * @param   ip_version
 * @param   other_endpoint_ip_address
 * @param   other_endpoint_port
 * @param   reporting_endpoint_ip_addr
 * @param   reporting_endpoint_port
 *
 * @returns pointer to the newly manufactured ::EVENT_MOBILE_FLOW.  If the
 *          event is not used (i.e. posted) it must be released using
 *          ::evel_free_mobile_flow.
 * @retval  NULL  Failed to create the event.
 *****************************************************************************/
EVENT_MOBILE_FLOW * evel_new_mobile_flow(
                      const char * const flow_direction,
                      MOBILE_GTP_PER_FLOW_METRICS * gtp_per_flow_metrics,
                      const char * const ip_protocol_type,
                      const char * const ip_version,
                      const char * const other_endpoint_ip_address,
                      int other_endpoint_port,
                      const char * const reporting_endpoint_ip_addr,
                      int reporting_endpoint_port);

/**************************************************************************//**
 * Free a Mobile Flow.
 *
 * Free off the Mobile Flow supplied.  Will free all the contained allocated
 * memory.
 *
 * @note It does not free the Mobile Flow itself, since that may be part of a
 * larger structure.
 *****************************************************************************/
void evel_free_mobile_flow(EVENT_MOBILE_FLOW * event);

/**************************************************************************//**
 * Set the Event Type property of the Mobile Flow.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param mobile_flow Pointer to the Mobile Flow.
 * @param type        The Event Type to be set. ASCIIZ string. The caller
 *                    does not need to preserve the value once the function
 *                    returns.
 *****************************************************************************/
void evel_mobile_flow_type_set(EVENT_MOBILE_FLOW * mobile_flow,
                               const char * const type);

/**************************************************************************//**
 * Set the Application Type property of the Mobile Flow.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param mobile_flow Pointer to the Mobile Flow.
 * @param type        The Application Type to be set. ASCIIZ string. The caller
 *                    does not need to preserve the value once the function
 *                    returns.
 *****************************************************************************/
void evel_mobile_flow_app_type_set(EVENT_MOBILE_FLOW * mobile_flow,
                                   const char * const type);

/**************************************************************************//**
 * Set the Application Protocol Type property of the Mobile Flow.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param mobile_flow Pointer to the Mobile Flow.
 * @param type        The Application Protocol Type to be set. ASCIIZ string.
 *                    The caller does not need to preserve the value once the
 *                    function returns.
 *****************************************************************************/
void evel_mobile_flow_app_prot_type_set(EVENT_MOBILE_FLOW * mobile_flow,
                                        const char * const type);

/**************************************************************************//**
 * Set the Application Protocol Version property of the Mobile Flow.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param mobile_flow Pointer to the Mobile Flow.
 * @param version     The Application Protocol Version to be set. ASCIIZ
 *                    string.  The caller does not need to preserve the value
 *                    once the function returns.
 *****************************************************************************/
void evel_mobile_flow_app_prot_ver_set(EVENT_MOBILE_FLOW * mobile_flow,
                                       const char * const version);

/**************************************************************************//**
 * Set the CID property of the Mobile Flow.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param mobile_flow Pointer to the Mobile Flow.
 * @param cid         The CID to be set. ASCIIZ string.  The caller does not
 *                    need to preserve the value once the function returns.
 *****************************************************************************/
void evel_mobile_flow_cid_set(EVENT_MOBILE_FLOW * mobile_flow,
                              const char * const cid);

/**************************************************************************//**
 * Set the Connection Type property of the Mobile Flow.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param mobile_flow Pointer to the Mobile Flow.
 * @param type        The Connection Type to be set. ASCIIZ string. The caller
 *                    does not need to preserve the value once the function
 *                    returns.
 *****************************************************************************/
void evel_mobile_flow_con_type_set(EVENT_MOBILE_FLOW * mobile_flow,
                                   const char * const type);

/**************************************************************************//**
 * Set the ECGI property of the Mobile Flow.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param mobile_flow Pointer to the Mobile Flow.
 * @param ecgi        The ECGI to be set. ASCIIZ string.  The caller does not
 *                    need to preserve the value once the function returns.
 *****************************************************************************/
void evel_mobile_flow_ecgi_set(EVENT_MOBILE_FLOW * mobile_flow,
                               const char * const ecgi);

/**************************************************************************//**
 * Set the GTP Protocol Type property of the Mobile Flow.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param mobile_flow Pointer to the Mobile Flow.
 * @param type        The GTP Protocol Type to be set. ASCIIZ string.  The
 *                    caller does not need to preserve the value once the
 *                    function returns.
 *****************************************************************************/
void evel_mobile_flow_gtp_prot_type_set(EVENT_MOBILE_FLOW * mobile_flow,
                                        const char * const type);

/**************************************************************************//**
 * Set the GTP Protocol Version property of the Mobile Flow.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param mobile_flow Pointer to the Mobile Flow.
 * @param version     The GTP Protocol Version to be set. ASCIIZ string.  The
 *                    caller does not need to preserve the value once the
 *                    function returns.
 *****************************************************************************/
void evel_mobile_flow_gtp_prot_ver_set(EVENT_MOBILE_FLOW * mobile_flow,
                                       const char * const version);

/**************************************************************************//**
 * Set the HTTP Header property of the Mobile Flow.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param mobile_flow Pointer to the Mobile Flow.
 * @param header      The HTTP header to be set. ASCIIZ string. The caller does
 *                    not need to preserve the value once the function returns.
 *****************************************************************************/
void evel_mobile_flow_http_header_set(EVENT_MOBILE_FLOW * mobile_flow,
                                      const char * const header);

/**************************************************************************//**
 * Set the IMEI property of the Mobile Flow.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param mobile_flow Pointer to the Mobile Flow.
 * @param imei        The IMEI to be set. ASCIIZ string.  The caller does not
 *                    need to preserve the value once the function returns.
 *****************************************************************************/
void evel_mobile_flow_imei_set(EVENT_MOBILE_FLOW * mobile_flow,
                               const char * const imei);

/**************************************************************************//**
 * Set the IMSI property of the Mobile Flow.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param mobile_flow Pointer to the Mobile Flow.
 * @param imsi        The IMSI to be set. ASCIIZ string.  The caller does not
 *                    need to preserve the value once the function returns.
 *****************************************************************************/
void evel_mobile_flow_imsi_set(EVENT_MOBILE_FLOW * mobile_flow,
                               const char * const imsi);

/**************************************************************************//**
 * Set the LAC property of the Mobile Flow.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param mobile_flow Pointer to the Mobile Flow.
 * @param lac         The LAC to be set. ASCIIZ string.  The caller does not
 *                    need to preserve the value once the function returns.
 *****************************************************************************/
void evel_mobile_flow_lac_set(EVENT_MOBILE_FLOW * mobile_flow,
                              const char * const lac);

/**************************************************************************//**
 * Set the MCC property of the Mobile Flow.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param mobile_flow Pointer to the Mobile Flow.
 * @param mcc         The MCC to be set. ASCIIZ string.  The caller does not
 *                    need to preserve the value once the function returns.
 *****************************************************************************/
void evel_mobile_flow_mcc_set(EVENT_MOBILE_FLOW * mobile_flow,
                              const char * const mcc);

/**************************************************************************//**
 * Set the MNC property of the Mobile Flow.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param mobile_flow Pointer to the Mobile Flow.
 * @param mnc         The MNC to be set. ASCIIZ string.  The caller does not
 *                    need to preserve the value once the function returns.
 *****************************************************************************/
void evel_mobile_flow_mnc_set(EVENT_MOBILE_FLOW * mobile_flow,
                              const char * const mnc);

/**************************************************************************//**
 * Set the MSISDN property of the Mobile Flow.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param mobile_flow Pointer to the Mobile Flow.
 * @param msisdn      The MSISDN to be set. ASCIIZ string.  The caller does not
 *                    need to preserve the value once the function returns.
 *****************************************************************************/
void evel_mobile_flow_msisdn_set(EVENT_MOBILE_FLOW * mobile_flow,
                                 const char * const msisdn);

/**************************************************************************//**
 * Set the Other Functional Role property of the Mobile Flow.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param mobile_flow Pointer to the Mobile Flow.
 * @param role        The Other Functional Role to be set. ASCIIZ string. The
 *                    caller does not need to preserve the value once the
 *                    function returns.
 *****************************************************************************/
void evel_mobile_flow_other_func_role_set(EVENT_MOBILE_FLOW * mobile_flow,
                                          const char * const role);

/**************************************************************************//**
 * Set the RAC property of the Mobile Flow.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param mobile_flow Pointer to the Mobile Flow.
 * @param rac         The RAC to be set. ASCIIZ string.  The caller does not
 *                    need to preserve the value once the function returns.
 *****************************************************************************/
void evel_mobile_flow_rac_set(EVENT_MOBILE_FLOW * mobile_flow,
                              const char * const rac);

/**************************************************************************//**
 * Set the Radio Access Technology property of the Mobile Flow.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param mobile_flow Pointer to the Mobile Flow.
 * @param tech        The Radio Access Technology to be set. ASCIIZ string. The
 *                    caller does not need to preserve the value once the
 *                    function returns.
 *****************************************************************************/
void evel_mobile_flow_radio_acc_tech_set(EVENT_MOBILE_FLOW * mobile_flow,
                                         const char * const tech);

/**************************************************************************//**
 * Set the SAC property of the Mobile Flow.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param mobile_flow Pointer to the Mobile Flow.
 * @param sac         The SAC to be set. ASCIIZ string.  The caller does not
 *                    need to preserve the value once the function returns.
 *****************************************************************************/
void evel_mobile_flow_sac_set(EVENT_MOBILE_FLOW * mobile_flow,
                              const char * const sac);

/**************************************************************************//**
 * Set the Sampling Algorithm property of the Mobile Flow.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param mobile_flow Pointer to the Mobile Flow.
 * @param algorithm   The Sampling Algorithm to be set.
 *****************************************************************************/
void evel_mobile_flow_samp_alg_set(EVENT_MOBILE_FLOW * mobile_flow,
                                   int algorithm);

/**************************************************************************//**
 * Set the TAC property of the Mobile Flow.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param mobile_flow Pointer to the Mobile Flow.
 * @param tac         The TAC to be set. ASCIIZ string.  The caller does not
 *                    need to preserve the value once the function returns.
 *****************************************************************************/
void evel_mobile_flow_tac_set(EVENT_MOBILE_FLOW * mobile_flow,
                              const char * const tac);

/**************************************************************************//**
 * Set the Tunnel ID property of the Mobile Flow.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param mobile_flow Pointer to the Mobile Flow.
 * @param tunnel_id   The Tunnel ID to be set. ASCIIZ string.  The caller does
 *                    not need to preserve the value once the function returns.
 *****************************************************************************/
void evel_mobile_flow_tunnel_id_set(EVENT_MOBILE_FLOW * mobile_flow,
                                    const char * const tunnel_id);

/**************************************************************************//**
 * Set the VLAN ID property of the Mobile Flow.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param mobile_flow Pointer to the Mobile Flow.
 * @param vlan_id     The VLAN ID to be set. ASCIIZ string.  The caller does
 *                    not need to preserve the value once the function returns.
 *****************************************************************************/
void evel_mobile_flow_vlan_id_set(EVENT_MOBILE_FLOW * mobile_flow,
                                  const char * const vlan_id);

/**************************************************************************//**
 * Create a new Mobile GTP Per Flow Metrics.
 *
 * @note    The mandatory fields on the Mobile GTP Per Flow Metrics must be
 *          supplied to this factory function and are immutable once set.
 *          Optional fields have explicit setter functions, but again values
 *          may only be set once so that the Mobile GTP Per Flow Metrics has
 *          immutable properties.
 *
 * @param   avg_bit_error_rate
 * @param   avg_packet_delay_variation
 * @param   avg_packet_latency
 * @param   avg_receive_throughput
 * @param   avg_transmit_throughput
 * @param   flow_activation_epoch
 * @param   flow_activation_microsec
 * @param   flow_deactivation_epoch
 * @param   flow_deactivation_microsec
 * @param   flow_deactivation_time
 * @param   flow_status
 * @param   max_packet_delay_variation
 * @param   num_activation_failures
 * @param   num_bit_errors
 * @param   num_bytes_received
 * @param   num_bytes_transmitted
 * @param   num_dropped_packets
 * @param   num_l7_bytes_received
 * @param   num_l7_bytes_transmitted
 * @param   num_lost_packets
 * @param   num_out_of_order_packets
 * @param   num_packet_errors
 * @param   num_packets_received_excl_retrans
 * @param   num_packets_received_incl_retrans
 * @param   num_packets_transmitted_incl_retrans
 * @param   num_retries
 * @param   num_timeouts
 * @param   num_tunneled_l7_bytes_received
 * @param   round_trip_time
 * @param   time_to_first_byte
 *
 * @returns pointer to the newly manufactured ::MOBILE_GTP_PER_FLOW_METRICS.
 *          If the structure is not used it must be released using
 *          ::evel_free_mobile_gtp_flow_metrics.
 * @retval  NULL  Failed to create the event.
 *****************************************************************************/
MOBILE_GTP_PER_FLOW_METRICS * evel_new_mobile_gtp_flow_metrics(
                                      double avg_bit_error_rate,
                                      double avg_packet_delay_variation,
                                      int avg_packet_latency,
                                      int avg_receive_throughput,
                                      int avg_transmit_throughput,
                                      int flow_activation_epoch,
                                      int flow_activation_microsec,
                                      int flow_deactivation_epoch,
                                      int flow_deactivation_microsec,
                                      time_t flow_deactivation_time,
                                      const char * const flow_status,
                                      int max_packet_delay_variation,
                                      int num_activation_failures,
                                      int num_bit_errors,
                                      int num_bytes_received,
                                      int num_bytes_transmitted,
                                      int num_dropped_packets,
                                      int num_l7_bytes_received,
                                      int num_l7_bytes_transmitted,
                                      int num_lost_packets,
                                      int num_out_of_order_packets,
                                      int num_packet_errors,
                                      int num_packets_received_excl_retrans,
                                      int num_packets_received_incl_retrans,
                                      int num_packets_transmitted_incl_retrans,
                                      int num_retries,
                                      int num_timeouts,
                                      int num_tunneled_l7_bytes_received,
                                      int round_trip_time,
                                      int time_to_first_byte);

/**************************************************************************//**
 * Free a Mobile GTP Per Flow Metrics.
 *
 * Free off the Mobile GTP Per Flow Metrics supplied.  Will free all the
 * contained allocated memory.
 *
 * @note It does not free the Mobile GTP Per Flow Metrics itself, since that
 * may be part of a larger structure.
 *****************************************************************************/
void evel_free_mobile_gtp_flow_metrics(MOBILE_GTP_PER_FLOW_METRICS * metrics);

/**************************************************************************//**
 * Set the Duration of Connection Failed Status property of the Mobile GTP Per
 * Flow Metrics.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param metrics     Pointer to the Mobile GTP Per Flow Metrics.
 * @param duration    The Duration of Connection Failed Status to be set.
 *****************************************************************************/
void evel_mobile_gtp_metrics_dur_con_fail_set(
                                         MOBILE_GTP_PER_FLOW_METRICS * metrics,
                                         int duration);

/**************************************************************************//**
 * Set the Duration of Tunnel Failed Status property of the Mobile GTP Per Flow
 * Metrics.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param metrics     Pointer to the Mobile GTP Per Flow Metrics.
 * @param duration    The Duration of Tunnel Failed Status to be set.
 *****************************************************************************/
void evel_mobile_gtp_metrics_dur_tun_fail_set(
                                         MOBILE_GTP_PER_FLOW_METRICS * metrics,
                                         int duration);

/**************************************************************************//**
 * Set the Activated By property of the Mobile GTP Per Flow metrics.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param metrics     Pointer to the Mobile GTP Per Flow Metrics.
 * @param act_by      The Activated By to be set.  ASCIIZ string. The caller
 *                    does not need to preserve the value once the function
 *                    returns.
 *****************************************************************************/
void evel_mobile_gtp_metrics_act_by_set(MOBILE_GTP_PER_FLOW_METRICS * metrics,
                                        const char * const act_by);

/**************************************************************************//**
 * Set the Activation Time property of the Mobile GTP Per Flow metrics.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param metrics     Pointer to the Mobile GTP Per Flow Metrics.
 * @param act_time    The Activation Time to be set.  ASCIIZ string. The caller
 *                    does not need to preserve the value once the function
 *                    returns.
 *****************************************************************************/
void evel_mobile_gtp_metrics_act_time_set(
                                         MOBILE_GTP_PER_FLOW_METRICS * metrics,
                                         time_t act_time);

/**************************************************************************//**
 * Set the Deactivated By property of the Mobile GTP Per Flow metrics.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param metrics     Pointer to the Mobile GTP Per Flow Metrics.
 * @param deact_by    The Deactivated By to be set.  ASCIIZ string. The caller
 *                    does not need to preserve the value once the function
 *                    returns.
 *****************************************************************************/
void evel_mobile_gtp_metrics_deact_by_set(
                                         MOBILE_GTP_PER_FLOW_METRICS * metrics,
                                         const char * const deact_by);

/**************************************************************************//**
 * Set the GTP Connection Status property of the Mobile GTP Per Flow metrics.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param metrics     Pointer to the Mobile GTP Per Flow Metrics.
 * @param status      The GTP Connection Status to be set.  ASCIIZ string. The
 *                    caller does not need to preserve the value once the
 *                    function returns.
 *****************************************************************************/
void evel_mobile_gtp_metrics_con_status_set(
                                         MOBILE_GTP_PER_FLOW_METRICS * metrics,
                                         const char * const status);

/**************************************************************************//**
 * Set the GTP Tunnel Status property of the Mobile GTP Per Flow metrics.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param metrics     Pointer to the Mobile GTP Per Flow Metrics.
 * @param status      The GTP Tunnel Status to be set.  ASCIIZ string. The
 *                    caller does not need to preserve the value once the
 *                    function returns.
 *****************************************************************************/
void evel_mobile_gtp_metrics_tun_status_set(
                                         MOBILE_GTP_PER_FLOW_METRICS * metrics,
                                         const char * const status);

/**************************************************************************//**
 * Set an IP Type-of-Service count property of the Mobile GTP Per Flow metrics.
 *
 * @param metrics     Pointer to the Mobile GTP Per Flow Metrics.
 * @param index       The index of the IP Type-of-Service.
 * @param count       The count.
 *****************************************************************************/
void evel_mobile_gtp_metrics_iptos_set(MOBILE_GTP_PER_FLOW_METRICS * metrics,
                                       int index,
                                       int count);

/**************************************************************************//**
 * Set the Large Packet Round-Trip Time property of the Mobile GTP Per Flow
 * Metrics.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param metrics     Pointer to the Mobile GTP Per Flow Metrics.
 * @param rtt         The Large Packet Round-Trip Time to be set.
 *****************************************************************************/
void evel_mobile_gtp_metrics_large_pkt_rtt_set(
                                         MOBILE_GTP_PER_FLOW_METRICS * metrics,
                                         int rtt);

/**************************************************************************//**
 * Set the Large Packet Threshold property of the Mobile GTP Per Flow Metrics.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param metrics     Pointer to the Mobile GTP Per Flow Metrics.
 * @param threshold   The Large Packet Threshold to be set.
 *****************************************************************************/
void evel_mobile_gtp_metrics_large_pkt_thresh_set(
                                         MOBILE_GTP_PER_FLOW_METRICS * metrics,
                                         double threshold);

/**************************************************************************//**
 * Set the Max Receive Bit Rate property of the Mobile GTP Per Flow Metrics.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param metrics     Pointer to the Mobile GTP Per Flow Metrics.
 * @param rate        The Max Receive Bit Rate to be set.
 *****************************************************************************/
void evel_mobile_gtp_metrics_max_rcv_bit_rate_set(
                                         MOBILE_GTP_PER_FLOW_METRICS * metrics,
                                         int rate);

/**************************************************************************//**
 * Set the Max Transmit Bit Rate property of the Mobile GTP Per Flow Metrics.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param metrics     Pointer to the Mobile GTP Per Flow Metrics.
 * @param rate        The Max Transmit Bit Rate to be set.
 *****************************************************************************/
void evel_mobile_gtp_metrics_max_trx_bit_rate_set(
                                         MOBILE_GTP_PER_FLOW_METRICS * metrics,
                                         int rate);

/**************************************************************************//**
 * Set the Number of GTP Echo Failures property of the Mobile GTP Per Flow
 * Metrics.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param metrics     Pointer to the Mobile GTP Per Flow Metrics.
 * @param num         The Number of GTP Echo Failures to be set.
 *****************************************************************************/
void evel_mobile_gtp_metrics_num_echo_fail_set(
                                         MOBILE_GTP_PER_FLOW_METRICS * metrics,
                                         int num);

/**************************************************************************//**
 * Set the Number of GTP Tunnel Errors property of the Mobile GTP Per Flow
 * Metrics.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param metrics     Pointer to the Mobile GTP Per Flow Metrics.
 * @param num         The Number of GTP Tunnel Errors to be set.
 *****************************************************************************/
void evel_mobile_gtp_metrics_num_tun_fail_set(
                                         MOBILE_GTP_PER_FLOW_METRICS * metrics,
                                         int num);

/**************************************************************************//**
 * Set the Number of HTTP Errors property of the Mobile GTP Per Flow Metrics.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param metrics     Pointer to the Mobile GTP Per Flow Metrics.
 * @param num         The Number of HTTP Errors to be set.
 *****************************************************************************/
void evel_mobile_gtp_metrics_num_http_errors_set(
                                         MOBILE_GTP_PER_FLOW_METRICS * metrics,
                                         int num);

/**************************************************************************//**
 * Add a TCP flag count to the metrics.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param metrics       Pointer to the Mobile GTP Per Flow Metrics.
 * @param tcp_flag      The TCP flag count to be updated.
 * @param count         The associated flag count.
 *****************************************************************************/
void evel_mobile_gtp_metrics_tcp_flag_count_add(
                                         MOBILE_GTP_PER_FLOW_METRICS * metrics,
                                         const EVEL_TCP_FLAGS tcp_flag,
                                         const int count);

/**************************************************************************//**
 * Add a QCI COS count to the metrics.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param metrics       Pointer to the Mobile GTP Per Flow Metrics.
 * @param qci_cos       The QCI COS count to be updated.
 * @param count         The associated QCI COS count.
 *****************************************************************************/
void evel_mobile_gtp_metrics_qci_cos_count_add(
                                         MOBILE_GTP_PER_FLOW_METRICS * metrics,
                                         const EVEL_QCI_COS_TYPES qci_cos,
                                         const int count);

/*****************************************************************************/
/*****************************************************************************/
/*                                                                           */
/*   SERVICE EVENTS                                                          */
/*                                                                           */
/*****************************************************************************/
/*****************************************************************************/

/**************************************************************************//**
 * Create a new Service event.
 *
 * @note    The mandatory fields on the Service must be supplied to
 *          this factory function and are immutable once set.  Optional fields
 *          have explicit setter functions, but again values may only be set
 *          once so that the event has immutable properties.
 * @param vendor_id     The vendor id to encode in the event instance id.
 * @param event_id      The vendor event id to encode in the event instance id.
 * @returns pointer to the newly manufactured ::EVENT_SERVICE.  If the event
 *          is not used (i.e. posted) it must be released using
 *          ::evel_free_service.
 * @retval  NULL  Failed to create the event.
 *****************************************************************************/
EVENT_SERVICE * evel_new_service(const char * const vendor_id,
                                 const char * const event_id);

/**************************************************************************//**
 * Free a Service Events event.
 *
 * Free off the event supplied.  Will free all the contained allocated memory.
 *
 * @note It does not free the event itself, since that may be part of a larger
 * structure.
 *****************************************************************************/
void evel_free_service(EVENT_SERVICE * const event);

/**************************************************************************//**
 * Set the Event Type property of the Service event.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param event         Pointer to the Service event.
 * @param type          The Event Type to be set. ASCIIZ string. The caller
 *                      does not need to preserve the value once the function
 *                      returns.
 *****************************************************************************/
void evel_service_type_set(EVENT_SERVICE * const event,
                           const char * const type);

/**************************************************************************//**
 * Set the Product Id property of the Service event.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param event         Pointer to the Service event.
 * @param product_id    The vendor product id to be set. ASCIIZ string. The
 *                      caller does not need to preserve the value once the
 *                      function returns.
 *****************************************************************************/
void evel_service_product_id_set(EVENT_SERVICE * const event,
                                 const char * const product_id);

/**************************************************************************//**
 * Set the Subsystem Id property of the Service event.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param event         Pointer to the Service event.
 * @param subsystem_id  The vendor subsystem id to be set. ASCIIZ string. The
 *                      caller does not need to preserve the value once the
 *                      function returns.
 *****************************************************************************/
void evel_service_subsystem_id_set(EVENT_SERVICE * const event,
                                   const char * const subsystem_id);

/**************************************************************************//**
 * Set the Friendly Name property of the Service event.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param event         Pointer to the Service event.
 * @param friendly_name The vendor friendly name to be set. ASCIIZ string. The
 *                      caller does not need to preserve the value once the
 *                      function returns.
 *****************************************************************************/
void evel_service_friendly_name_set(EVENT_SERVICE * const event,
                                    const char * const friendly_name);

/**************************************************************************//**
 * Set the correlator property of the Service event.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param event         Pointer to the Service event.
 * @param correlator    The correlator to be set. ASCIIZ string. The caller
 *                      does not need to preserve the value once the function
 *                      returns.
 *****************************************************************************/
void evel_service_correlator_set(EVENT_SERVICE * const event,
                                 const char * const correlator);

/**************************************************************************//**
 * Set the Codec property of the Service event.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param event         Pointer to the Service event.
 * @param codec         The codec to be set. ASCIIZ string. The caller does not
 *                      need to preserve the value once the function returns.
 *****************************************************************************/
void evel_service_codec_set(EVENT_SERVICE * const event,
                            const char * const codec);

/**************************************************************************//**
 * Set the Callee Side Codec property of the Service event.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param event         Pointer to the Service event.
 * @param codec         The codec to be set. ASCIIZ string. The caller does not
 *                      need to preserve the value once the function returns.
 *****************************************************************************/
void evel_service_callee_codec_set(EVENT_SERVICE * const event,
                                   const char * const codec);

/**************************************************************************//**
 * Set the Caller Side Codec property of the Service event.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param event         Pointer to the Service event.
 * @param codec         The codec to be set. ASCIIZ string. The caller does not
 *                      need to preserve the value once the function returns.
 *****************************************************************************/
void evel_service_caller_codec_set(EVENT_SERVICE * const event,
                                   const char * const codec);

/**************************************************************************//**
 * Set the RTCP Data property of the Service event.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param event         Pointer to the Service event.
 * @param rtcp_data     The RTCP Data to be set. ASCIIZ string. The caller
 *                      does not need to preserve the value once the function
 *                      returns.
 *****************************************************************************/
void evel_service_rtcp_data_set(EVENT_SERVICE * const event,
                                const char * const rtcp_data);

/**************************************************************************//**
 * Set the Adjacency Name property of the Service event.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param event         Pointer to the Service event.
 * @param adjacency_name
 *                      The adjacency name to be set. ASCIIZ string. The caller
 *                      does not need to preserve the value once the function
 *                      returns.
 *****************************************************************************/
void evel_service_adjacency_name_set(EVENT_SERVICE * const event,
                                     const char * const adjacency_name);

/**************************************************************************//**
 * Set the Endpoint Descriptor property of the Service event.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param event         Pointer to the Service event.
 * @param endpoint_desc The endpoint descriptor to be set.
 *****************************************************************************/
void evel_service_endpoint_desc_set(
                                EVENT_SERVICE * const event,
                                const EVEL_SERVICE_ENDPOINT_DESC endpoint_desc);

/**************************************************************************//**
 * Set the Endpoint Jitter property of the Service event.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param event         Pointer to the Service event.
 * @param jitter        The jitter to be set.
 *****************************************************************************/
void evel_service_endpoint_jitter_set(EVENT_SERVICE * const event,
                                      const int jitter);

/**************************************************************************//**
 * Set the Endpoint Rtp Octets Discarded property of the Service event.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param event         Pointer to the Service event.
 * @param rtp_oct_disc  The discard count.
 *****************************************************************************/
void evel_service_endpoint_rtp_oct_disc_set(EVENT_SERVICE * const event,
                                            const int rtp_oct_disc);

/**************************************************************************//**
 * Set the Endpoint Rtp Octets Received property of the Service event.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param event         Pointer to the Service event.
 * @param rtp_oct_recv  The receive count.
 *****************************************************************************/
void evel_service_endpoint_rtp_oct_recv_set(EVENT_SERVICE * const event,
                                            const int rtp_oct_recv);

/**************************************************************************//**
 * Set the Endpoint Rtp Octets Sent property of the Service event.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param event         Pointer to the Service event.
 * @param rtp_oct_sent  The send count.
 *****************************************************************************/
void evel_service_endpoint_rtp_oct_sent_set(EVENT_SERVICE * const event,
                                            const int rtp_oct_sent);

/**************************************************************************//**
 * Set the Endpoint Rtp Packets Discarded property of the Service event.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param event         Pointer to the Service event.
 * @param rtp_pkt_disc  The discard count.
 *****************************************************************************/
void evel_service_endpoint_rtp_pkt_disc_set(EVENT_SERVICE * const event,
                                            const int rtp_pkt_disc);

/**************************************************************************//**
 * Set the Endpoint Rtp Packets Received property of the Service event.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param event         Pointer to the Service event.
 * @param rtp_pkt_recv  The receive count.
 *****************************************************************************/
void evel_service_endpoint_rtp_pkt_recv_set(EVENT_SERVICE * const event,
                                            const int rtp_pkt_recv);

/**************************************************************************//**
 * Set the Endpoint Rtp Packets Sent property of the Service event.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param event         Pointer to the Service event.
 * @param rtp_pkt_sent  The send count.
 *****************************************************************************/
void evel_service_endpoint_rtp_pkt_sent_set(EVENT_SERVICE * const event,
                                            const int rtp_pkt_sent);

/**************************************************************************//**
 * Set the Local Jitter property of the Service event.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param event         Pointer to the Service event.
 * @param jitter        The jitter to be set.
 *****************************************************************************/
void evel_service_local_jitter_set(EVENT_SERVICE * const event,
                                   const int jitter);

/**************************************************************************//**
 * Set the Local Rtp Octets Discarded property of the Service event.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param event         Pointer to the Service event.
 * @param rtp_oct_disc  The discard count.
 *****************************************************************************/
void evel_service_local_rtp_oct_disc_set(EVENT_SERVICE * const event,
                                         const int rtp_oct_disc);

/**************************************************************************//**
 * Set the Local Rtp Octets Received property of the Service event.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param event         Pointer to the Service event.
 * @param rtp_oct_recv  The receive count.
 *****************************************************************************/
void evel_service_local_rtp_oct_recv_set(EVENT_SERVICE * const event,
                                         const int rtp_oct_recv);

/**************************************************************************//**
 * Set the Local Rtp Octets Sent property of the Service event.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param event         Pointer to the Service event.
 * @param rtp_oct_sent  The send count.
 *****************************************************************************/
void evel_service_local_rtp_oct_sent_set(EVENT_SERVICE * const event,
                                         const int rtp_oct_sent);

/**************************************************************************//**
 * Set the Local Rtp Packets Discarded property of the Service event.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param event         Pointer to the Service event.
 * @param rtp_pkt_disc  The discard count.
 *****************************************************************************/
void evel_service_local_rtp_pkt_disc_set(EVENT_SERVICE * const event,
                                         const int rtp_pkt_disc);

/**************************************************************************//**
 * Set the Local Rtp Packets Received property of the Service event.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param event         Pointer to the Service event.
 * @param rtp_pkt_recv  The receive count.
 *****************************************************************************/
void evel_service_local_rtp_pkt_recv_set(EVENT_SERVICE * const event,
                                         const int rtp_pkt_recv);

/**************************************************************************//**
 * Set the Local Rtp Packets Sent property of the Service event.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param event         Pointer to the Service event.
 * @param rtp_pkt_sent  The send count.
 *****************************************************************************/
void evel_service_local_rtp_pkt_sent_set(EVENT_SERVICE * const event,
                                         const int rtp_pkt_sent);

/**************************************************************************//**
 * Set the Mos Cqe property of the Service event.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param event         Pointer to the Service event.
 * @param mos_cqe       The mosCqe to be set.
 *****************************************************************************/
void evel_service_mos_cqe_set(EVENT_SERVICE * const event,
                              const double mos_cqe);

/**************************************************************************//**
 * Set the Packets Lost property of the Service event.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param event         Pointer to the Service event.
 * @param packets_lost  The number of packets lost to be set.
 *****************************************************************************/
void evel_service_packets_lost_set(EVENT_SERVICE * const event,
                                   const int packets_lost);

/**************************************************************************//**
 * Set the packet Loss Percent property of the Service event.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param event         Pointer to the Service event.
 * @param packet_loss_percent
 *                      The packet loss in percent.
 *****************************************************************************/
void evel_service_packet_loss_percent_set(EVENT_SERVICE * const event,
                                          const double packet_loss_percent);

/**************************************************************************//**
 * Set the R Factor property of the Service event.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param event         Pointer to the Service event.
 * @param r_factor      The R Factor to be set.
 *****************************************************************************/
void evel_service_r_factor_set(EVENT_SERVICE * const event,
                               const int r_factor);

/**************************************************************************//**
 * Set the Round Trip Delay property of the Service event.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param event         Pointer to the Service event.
 * @param round_trip_delay
 *                      The Round trip delay to be set.
 *****************************************************************************/
void evel_service_round_trip_delay_set(EVENT_SERVICE * const event,
                                       const int round_trip_delay);

/**************************************************************************//**
 * Set the Phone Number property of the Service event.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param event         Pointer to the Service event.
 * @param phone_number  The Phone Number to be set. ASCIIZ string. The caller
 *                      does not need to preserve the value once the function
 *                      returns.
 *****************************************************************************/
void evel_service_phone_number_set(EVENT_SERVICE * const event,
                                   const char * const phone_number);

/**************************************************************************//**
 * Add a name/value pair to the Service, under the additionalFields array.
 *
 * The name and value are null delimited ASCII strings.  The library takes
 * a copy so the caller does not have to preserve values after the function
 * returns.
 *
 * @param event     Pointer to the Service event.
 * @param name      ASCIIZ string with the field's name.  The caller does not
 *                  need to preserve the value once the function returns.
 * @param value     ASCIIZ string with the field's value.  The caller does not
 *                  need to preserve the value once the function returns.
 *****************************************************************************/
void evel_service_addl_field_add(EVENT_SERVICE * const event,
                                 const char * const name,
                                 const char * const value);

/*****************************************************************************/
/*****************************************************************************/
/*                                                                           */
/*   SIGNALING                                                               */
/*                                                                           */
/*****************************************************************************/
/*****************************************************************************/

/**************************************************************************//**
 * Create a new Signaling event.
 *
 * @note    The mandatory fields on the Signaling must be supplied to
 *          this factory function and are immutable once set.  Optional fields
 *          have explicit setter functions, but again values may only be set
 *          once so that the event has immutable properties.
 * @param vendor_id     The vendor id to encode in the event instance id.
 * @param event_id      The vendor event id to encode in the event instance id.
 * @returns pointer to the newly manufactured ::EVENT_SIGNALING.  If the event
 *          is not used (i.e. posted) it must be released using
 *          ::evel_free_signaling.
 * @retval  NULL  Failed to create the event.
 *****************************************************************************/
EVENT_SIGNALING * evel_new_signaling(const char * const vendor_id,
                                     const char * const event_id);

/**************************************************************************//**
 * Free a Signaling event.
 *
 * Free off the event supplied.  Will free all the contained allocated memory.
 *
 * @note It does not free the event itself, since that may be part of a larger
 * structure.
 *****************************************************************************/
void evel_free_signaling(EVENT_SIGNALING * const event);

/**************************************************************************//**
 * Set the Event Type property of the Signaling event.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param event         Pointer to the Signaling event.
 * @param type          The Event Type to be set. ASCIIZ string. The caller
 *                      does not need to preserve the value once the function
 *                      returns.
 *****************************************************************************/
void evel_signaling_type_set(EVENT_SIGNALING * const event,
                             const char * const type);

/**************************************************************************//**
 * Set the Product Id property of the Signaling event.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param event         Pointer to the Signaling event.
 * @param product_id    The vendor product id to be set. ASCIIZ string. The
 *                      caller does not need to preserve the value once the
 *                      function returns.
 *****************************************************************************/
void evel_signaling_product_id_set(EVENT_SIGNALING * const event,
                                   const char * const product_id);

/**************************************************************************//**
 * Set the Subsystem Id property of the Signaling event.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param event         Pointer to the Signaling event.
 * @param subsystem_id  The vendor subsystem id to be set. ASCIIZ string. The
 *                      caller does not need to preserve the value once the
 *                      function returns.
 *****************************************************************************/
void evel_signaling_subsystem_id_set(EVENT_SIGNALING * const event,
                                     const char * const subsystem_id);

/**************************************************************************//**
 * Set the Friendly Name property of the Signaling event.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param event         Pointer to the Signaling event.
 * @param friendly_name The vendor friendly name to be set. ASCIIZ string. The
 *                      caller does not need to preserve the value once the
 *                      function returns.
 *****************************************************************************/
void evel_signaling_friendly_name_set(EVENT_SIGNALING * const event,
                                      const char * const friendly_name);

/**************************************************************************//**
 * Set the Correlator property of the Signaling event.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param event         Pointer to the Signaling event.
 * @param correlator    The correlator to be set. ASCIIZ string. The caller
 *                      does not need to preserve the value once the function
 *                      returns.
 *****************************************************************************/
void evel_signaling_correlator_set(EVENT_SIGNALING * const event,
                                   const char * const correlator);

/**************************************************************************//**
 * Set the Local Ip Address property of the Signaling event.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param event         Pointer to the Signaling event.
 * @param local_ip_address
 *                      The Local Ip Address to be set. ASCIIZ string. The
 *                      caller does not need to preserve the value once the
 *                      function returns.
 *****************************************************************************/
void evel_signaling_local_ip_address_set(EVENT_SIGNALING * const event,
                                         const char * const local_ip_address);

/**************************************************************************//**
 * Set the Local Port property of the Signaling event.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param event         Pointer to the Signaling event.
 * @param local_port    The Local Port to be set. ASCIIZ string. The caller
 *                      does not need to preserve the value once the function
 *                      returns.
 *****************************************************************************/
void evel_signaling_local_port_set(EVENT_SIGNALING * const event,
                                   const char * const local_port);

/**************************************************************************//**
 * Set the Remote Ip Address property of the Signaling event.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param event         Pointer to the Signaling event.
 * @param remote_ip_address
 *                      The Remote Ip Address to be set. ASCIIZ string. The
 *                      caller does not need to preserve the value once the
 *                      function returns.
 *****************************************************************************/
void evel_signaling_remote_ip_address_set(EVENT_SIGNALING * const event,
                                         const char * const remote_ip_address);

/**************************************************************************//**
 * Set the Remote Port property of the Signaling event.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param event         Pointer to the Signaling event.
 * @param remote_port   The Remote Port to be set. ASCIIZ string. The caller
 *                      does not need to preserve the value once the function
 *                      returns.
 *****************************************************************************/
void evel_signaling_remote_port_set(EVENT_SIGNALING * const event,
                                    const char * const remote_port);

/**************************************************************************//**
 * Set the Compressed SIP property of the Signaling event.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param event         Pointer to the Signaling event.
 * @param compressed_sip
 *                      The Compressed SIP to be set. ASCIIZ string. The caller
 *                      does not need to preserve the value once the function
 *                      returns.
 *****************************************************************************/
void evel_signaling_compressed_sip_set(EVENT_SIGNALING * const event,
                                       const char * const compressed_sip);

/**************************************************************************//**
 * Set the Summary SIP property of the Signaling event.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param event         Pointer to the Signaling event.
 * @param summary_sip   The Summary SIP to be set. ASCIIZ string. The caller
 *                      does not need to preserve the value once the function
 *                      returns.
 *****************************************************************************/
void evel_signaling_summary_sip_set(EVENT_SIGNALING * const event,
                                    const char * const summary_sip);

/*****************************************************************************/
/*****************************************************************************/
/*                                                                           */
/*   STATE CHANGE                                                            */
/*                                                                           */
/*****************************************************************************/
/*****************************************************************************/

/**************************************************************************//**
 * Create a new State Change event.
 *
 * @note    The mandatory fields on the Syslog must be supplied to this factory
 *          function and are immutable once set.  Optional fields have explicit
 *          setter functions, but again values may only be set once so that the
 *          Syslog has immutable properties.
 *
 * @param new_state     The new state of the reporting entity.
 * @param old_state     The old state of the reporting entity.
 * @param interface     The card or port name of the reporting entity.
 *
 * @returns pointer to the newly manufactured ::EVENT_STATE_CHANGE.  If the
 *          event is not used it must be released using
 *          ::evel_free_state_change
 * @retval  NULL  Failed to create the event.
 *****************************************************************************/
EVENT_STATE_CHANGE * evel_new_state_change(const EVEL_ENTITY_STATE new_state,
                                           const EVEL_ENTITY_STATE old_state,
                                           const char * const interface);

/**************************************************************************//**
 * Free a State Change.
 *
 * Free off the State Change supplied.  Will free all the contained allocated
 * memory.
 *
 * @note It does not free the State Change itself, since that may be part of a
 * larger structure.
 *****************************************************************************/
void evel_free_state_change(EVENT_STATE_CHANGE * const state_change);

/**************************************************************************//**
 * Set the Event Type property of the State Change.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param state_change  Pointer to the ::EVENT_STATE_CHANGE.
 * @param type          The Event Type to be set. ASCIIZ string. The caller
 *                      does not need to preserve the value once the function
 *                      returns.
 *****************************************************************************/
void evel_state_change_type_set(EVENT_STATE_CHANGE * const state_change,
                                const char * const type);

/**************************************************************************//**
 * Add an additional field name/value pair to the State Change.
 *
 * The name and value are null delimited ASCII strings.  The library takes
 * a copy so the caller does not have to preserve values after the function
 * returns.
 *
 * @param state_change  Pointer to the ::EVENT_STATE_CHANGE.
 * @param name          ASCIIZ string with the attribute's name.  The caller
 *                      does not need to preserve the value once the function
 *                      returns.
 * @param value         ASCIIZ string with the attribute's value.  The caller
 *                      does not need to preserve the value once the function
 *                      returns.
 *****************************************************************************/
void evel_state_change_addl_field_add(EVENT_STATE_CHANGE * const state_change,
                                      const char * const name,
                                      const char * const value);

/*****************************************************************************/
/*****************************************************************************/
/*                                                                           */
/*   SYSLOG                                                                  */
/*                                                                           */
/*****************************************************************************/
/*****************************************************************************/

/**************************************************************************//**
 * Create a new syslog event.
 *
 * @note    The mandatory fields on the Syslog must be supplied to this factory
 *          function and are immutable once set.  Optional fields have explicit
 *          setter functions, but again values may only be set once so that the
 *          Syslog has immutable properties.
 *
 * @param   event_source_type
 * @param   syslog_msg
 * @param   syslog_tag
 *
 * @returns pointer to the newly manufactured ::EVENT_SYSLOG.  If the event is
 *          not used it must be released using ::evel_free_syslog
 * @retval  NULL  Failed to create the event.
 *****************************************************************************/
EVENT_SYSLOG * evel_new_syslog(EVEL_SOURCE_TYPES event_source_type,
                               const char * const syslog_msg,
                               const char * const syslog_tag);

/**************************************************************************//**
 * Set the Event Type property of the Syslog.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param syslog      Pointer to the syslog.
 * @param type        The Event Type to be set. ASCIIZ string. The caller
 *                    does not need to preserve the value once the function
 *                    returns.
 *****************************************************************************/
void evel_syslog_type_set(EVENT_SYSLOG * syslog,
                          const char * const type);

/**************************************************************************//**
 * Free a Syslog.
 *
 * Free off the Syslog supplied.  Will free all the contained allocated memory.
 *
 * @note It does not free the Syslog itself, since that may be part of a
 * larger structure.
 *****************************************************************************/
void evel_free_syslog(EVENT_SYSLOG * event);

/**************************************************************************//**
 * Add an additional field name/value pair to the Syslog.
 *
 * The name and value are null delimited ASCII strings.  The library takes
 * a copy so the caller does not have to preserve values after the function
 * returns.
 *
 * @param syslog    Pointer to the syslog.
 * @param name      ASCIIZ string with the attribute's name.  The caller
 *                  does not need to preserve the value once the function
 *                  returns.
 * @param value     ASCIIZ string with the attribute's value.  The caller
 *                  does not need to preserve the value once the function
 *                  returns.
 *****************************************************************************/
void evel_syslog_addl_field_add(EVENT_SYSLOG * syslog,
                                char * name,
                                char * value);

/**************************************************************************//**
 * Set the Event Source Host property of the Syslog.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param syslog      Pointer to the Syslog.
 * @param host        The Event Source Host to be set.  ASCIIZ string. The
 *                    caller does not need to preserve the value once the
 *                    function returns.
 *****************************************************************************/
void evel_syslog_event_source_host_set(EVENT_SYSLOG * syslog,
                                       const char * const host);

/**************************************************************************//**
 * Set the Syslog Facility property of the Syslog.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param syslog      Pointer to the Syslog.
 * @param facility    The Syslog Facility to be set.  ASCIIZ string. The caller
 *                    does not need to preserve the value once the function
 *                    returns.
 *****************************************************************************/
void evel_syslog_facility_set(EVENT_SYSLOG * syslog,
                              EVEL_SYSLOG_FACILITIES facility);

/**************************************************************************//**
 * Set the Process property of the Syslog.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param syslog      Pointer to the Syslog.
 * @param proc        The Process to be set.  ASCIIZ string. The caller does
 *                    not need to preserve the value once the function returns.
 *****************************************************************************/
void evel_syslog_proc_set(EVENT_SYSLOG * syslog, const char * const proc);

/**************************************************************************//**
 * Set the Process ID property of the Syslog.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param syslog      Pointer to the Syslog.
 * @param proc_id     The Process ID to be set.
 *****************************************************************************/
void evel_syslog_proc_id_set(EVENT_SYSLOG * syslog, int proc_id);

/**************************************************************************//**
 * Set the Version property of the Syslog.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param syslog      Pointer to the Syslog.
 * @param version     The Version to be set.
 *****************************************************************************/
void evel_syslog_version_set(EVENT_SYSLOG * syslog, int version);

/**************************************************************************//**
 * Set the Structured Data property of the Syslog.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param syslog      Pointer to the Syslog.
 * @param s_data      The Structured Data to be set.  ASCIIZ string. The caller
 *                    does not need to preserve the value once the function
 *                    returns.
 *****************************************************************************/
void evel_syslog_s_data_set(EVENT_SYSLOG * syslog, const char * const s_data);

/*****************************************************************************/
/*****************************************************************************/
/*                                                                           */
/*   OTHER                                                                   */
/*                                                                           */
/*****************************************************************************/
/*****************************************************************************/

/**************************************************************************//**
 * Create a new other event.
 *
 *
 * @returns pointer to the newly manufactured ::EVENT_OTHER.  If the event is
 *          not used it must be released using ::evel_free_other.
 * @retval  NULL  Failed to create the event.
 *****************************************************************************/
EVENT_OTHER * evel_new_other(void);

/**************************************************************************//**
 * Free an Other.
 *
 * Free off the Other supplied.  Will free all the contained allocated memory.
 *
 * @note It does not free the Other itself, since that may be part of a
 * larger structure.
 *****************************************************************************/
void evel_free_other(EVENT_OTHER * event);

/**************************************************************************//**
 * Set the Event Type property of the Other.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param other       Pointer to the Other.
 * @param type        The Event Type to be set. ASCIIZ string. The caller
 *                    does not need to preserve the value once the function
 *                    returns.
 *****************************************************************************/
void evel_other_type_set(EVENT_OTHER * other,
                         const char * const type);

/**************************************************************************//**
 * Add a value name/value pair to the Other.
 *
 * The name and value are null delimited ASCII strings.  The library takes
 * a copy so the caller does not have to preserve values after the function
 * returns.
 *
 * @param other     Pointer to the Other.
 * @param name      ASCIIZ string with the attribute's name.
 * @param value     ASCIIZ string with the attribute's value.
 *****************************************************************************/
void evel_other_field_add(EVENT_OTHER * other,
                          char * name,
                          char * value);

/*****************************************************************************/
/*****************************************************************************/
/*                                                                           */
/*   THROTTLING                                                              */
/*                                                                           */
/*****************************************************************************/
/*****************************************************************************/

/**************************************************************************//**
 * Return the current measurement interval provided by the Event Listener.
 *
 * @returns The current measurement interval
 * @retval  EVEL_MEASUREMENT_INTERVAL_UKNOWN (0) - interval has not been
 *          specified
 *****************************************************************************/
int evel_get_measurement_interval();

/*****************************************************************************/
/*****************************************************************************/
/*                                                                           */
/*   LOGGING                                                                 */
/*                                                                           */
/*****************************************************************************/
/*****************************************************************************/

/*****************************************************************************/
/* Debug macros.                                                             */
/*****************************************************************************/
#define EVEL_DEBUG(FMT, ...)   log_debug(EVEL_LOG_DEBUG, (FMT), ##__VA_ARGS__)
#define EVEL_INFO(FMT, ...)    log_debug(EVEL_LOG_INFO, (FMT), ##__VA_ARGS__)
#define EVEL_SPAMMY(FMT, ...)  log_debug(EVEL_LOG_SPAMMY, (FMT), ##__VA_ARGS__)
#define EVEL_ERROR(FMT, ...)   log_debug(EVEL_LOG_ERROR, "ERROR: " FMT, \
                                         ##__VA_ARGS__)
#define EVEL_ENTER()                                                          \
        {                                                                     \
          log_debug(EVEL_LOG_DEBUG, "Enter %s {", __FUNCTION__);              \
          debug_indent += 2;                                                  \
        }
#define EVEL_EXIT()                                                           \
        {                                                                     \
          debug_indent -= 2;                                                  \
          log_debug(EVEL_LOG_DEBUG, "Exit %s }", __FUNCTION__);               \
        }

#define INDENT_SEPARATORS                                                     \
        "| | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | "

extern EVEL_LOG_LEVELS debug_level;
extern int debug_indent;
extern FILE * fout;

#define EVEL_DEBUG_ON() ((debug_level) >= EVEL_LOG_DEBUG)

/**************************************************************************//**
 * Initialize logging
 *
 * @param[in] level  The debugging level - one of ::EVEL_LOG_LEVELS.
 * @param[in] ident  The identifier for our logs.
 *****************************************************************************/
void log_initialize(EVEL_LOG_LEVELS level, const char * ident);

/**************************************************************************//**
 * Log debug information
 *
 * Logs debugging information in a platform independent manner.
 *
 * @param[in] level   The debugging level - one of ::EVEL_LOG_LEVELS.
 * @param[in] format  Log formatting string in printf format.
 * @param[in] ...     Variable argument list.
 *****************************************************************************/
void log_debug(EVEL_LOG_LEVELS level, char * format, ...);

/***************************************************************************//*
 * Store the formatted string into the static error string and log the error.
 *
 * @param format  Error string in standard printf format.
 * @param ...     Variable parameters to be substituted into the format string.
 *****************************************************************************/
void log_error_state(char * format, ...);

#ifdef __cplusplus
}
#endif

#endif
