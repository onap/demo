#ifndef EVEL_INCLUDED
#define EVEL_INCLUDED
/*************************************************************************//**
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
 *
 ****************************************************************************/

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
 *****************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>

#include "jsmn.h"
#include "double_list.h"
#include "hashtable.h"

/*****************************************************************************/
/* Supported API version.                                                    */
/*****************************************************************************/
#define EVEL_API_MAJOR_VERSION 5
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
  EVEL_DOMAIN_HEARTBEAT_FIELD,/** A Heartbeat field event.                   */
  EVEL_DOMAIN_SIPSIGNALING,   /** A Signaling event.                         */
  EVEL_DOMAIN_STATE_CHANGE,   /** A State Change event.                      */
  EVEL_DOMAIN_SYSLOG,         /** A Syslog event.                            */
  EVEL_DOMAIN_OTHER,          /** Another event.                             */
  EVEL_DOMAIN_THRESHOLD_CROSS,  /** A Threshold Crossing  Event		     */
  EVEL_DOMAIN_VOICE_QUALITY,  /** A Voice Quality Event		 	     */
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

/**************************************************************************//**
 * enrichment fields for internal VES Event Listener service use only,
 * not supplied by event sources
 *****************************************************************************/
typedef struct internal_header_fields
{
  void *object;
  EVEL_BOOLEAN is_set;
} EVEL_OPTION_INTHEADER_FIELDS;

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
  char * event_name;
  char * source_name;
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
  EVEL_OPTION_INTHEADER_FIELDS internal_field;
  EVEL_OPTION_STRING nfcnaming_code;
  EVEL_OPTION_STRING nfnaming_code;

} EVENT_HEADER;

/*****************************************************************************/
/* Supported Fault version.                                                  */
/*****************************************************************************/
#define EVEL_FAULT_MAJOR_VERSION 2
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
  EVEL_OPTION_STRING category;
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


/**************************************************************************//**
 * optional field block for fields specific to heartbeat events
 *****************************************************************************/
typedef struct event_heartbeat_fields
{
  /***************************************************************************/
  /* Header and version                                                      */
  /***************************************************************************/
  EVENT_HEADER header;
  int major_version;
  int minor_version;

  /***************************************************************************/
  /* Mandatory fields                                                        */
  /***************************************************************************/
  double heartbeat_version;
  int    heartbeat_interval;

  /***************************************************************************/
  /* Optional fields                                                         */
  /***************************************************************************/
  DLIST additional_info;

} EVENT_HEARTBEAT_FIELD;

/**************************************************************************//**
 * tuple which provides the name of a key along with its value and
 * relative order
 *****************************************************************************/
typedef struct internal_key
{
  char                *keyname;
  EVEL_OPTION_INT      keyorder;
  EVEL_OPTION_STRING   keyvalue;
} EVEL_INTERNAL_KEY;

/**************************************************************************//**
 * meta-information about an instance of a jsonObject along with
 * the actual object instance
 *****************************************************************************/
typedef struct json_object_instance
{

  char *jsonstring;
  unsigned long long objinst_epoch_microsec;
  DLIST object_keys; /*EVEL_INTERNAL_KEY list */

} EVEL_JSON_OBJECT_INSTANCE;
#define MAX_JSON_TOKENS 128
/**************************************************************************//**
 * Create a new json object instance.
 *
 * @note    The mandatory fields on the Other must be supplied to this factory
 *          function and are immutable once set.  Optional fields have explicit
 *          setter functions, but again values may only be set once so that the
 *          Other has immutable properties.
 * @param   yourjson       json string.
 * @returns pointer to the newly manufactured ::EVEL_JSON_OBJECT_INSTANCE.
 *          not used (i.e. posted) it must be released using ::evel_free_jsonobjectinstance.
 * @retval  NULL  Failed to create the json object instance.
 *****************************************************************************/
EVEL_JSON_OBJECT_INSTANCE * evel_new_jsonobjinstance(const char *const yourjson);
/**************************************************************************//**
 * Free an json object instance.
 *
 * Free off the json object instance supplied.
 *  Will free all the contained allocated memory.
 *
 *****************************************************************************/
void evel_free_jsonobjinst(EVEL_JSON_OBJECT_INSTANCE * objinst);

/**************************************************************************//**
 * enrichment fields for internal VES Event Listener service use only,
 * not supplied by event sources
 *****************************************************************************/
typedef struct json_object
{

  char *object_name;
  EVEL_OPTION_STRING objectschema;
  EVEL_OPTION_STRING objectschemaurl;
  EVEL_OPTION_STRING nfsubscribedobjname;
  EVEL_OPTION_STRING nfsubscriptionid;
  DLIST jsonobjectinstances;  /* EVEL_JSON_OBJECT_INSTANCE list */

} EVEL_JSON_OBJECT;

/**************************************************************************//**
 * Create a new json object.
 *
 * @note    The mandatory fields on the Other must be supplied to this factory
 *          function and are immutable once set.  Optional fields have explicit
 *          setter functions, but again values may only be set once so that the
 *          Other has immutable properties.
 * @param name       name of the object.
 * @returns pointer to the newly manufactured ::EVEL_JSON_OBJECT.
 *          not used (i.e. posted) it must be released using ::evel_free_jsonobject.
 * @retval  NULL  Failed to create the json object.
 *****************************************************************************/
EVEL_JSON_OBJECT * evel_new_jsonobject(const char *const name);
/**************************************************************************//**
 * Free an json object.
 *
 * Free off the json object instance supplied.
 *  Will free all the contained allocated memory.
 *
 *****************************************************************************/
void evel_free_jsonobject(EVEL_JSON_OBJECT * jsobj);
/*****************************************************************************/
/* Supported Measurement version.                                            */
/*****************************************************************************/
#define EVEL_MEASUREMENT_MAJOR_VERSION 2
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
  DLIST additional_info;
  DLIST additional_measurements;
  DLIST additional_objects;
  DLIST codec_usage;
  EVEL_OPTION_INT concurrent_sessions;
  EVEL_OPTION_INT configured_entities;
  DLIST cpu_usage;
  DLIST disk_usage;
  MEASUREMENT_ERRORS * errors;
  DLIST feature_usage;
  DLIST filesystem_usage;
  DLIST latency_distribution;
  EVEL_OPTION_DOUBLE mean_request_latency;
  DLIST mem_usage;
  EVEL_OPTION_INT media_ports_in_use;
  EVEL_OPTION_INT request_rate;
  EVEL_OPTION_INT vnfc_scaling_metric;
  DLIST vnic_usage;

} EVENT_MEASUREMENT;

/**************************************************************************//**
 * CPU Usage.
 * JSON equivalent field: cpuUsage
 *****************************************************************************/
typedef struct measurement_cpu_use {
  char * id;
  double usage;
  EVEL_OPTION_DOUBLE idle;
  EVEL_OPTION_DOUBLE intrpt;
  EVEL_OPTION_DOUBLE nice;
  EVEL_OPTION_DOUBLE softirq;
  EVEL_OPTION_DOUBLE steal;
  EVEL_OPTION_DOUBLE sys;
  EVEL_OPTION_DOUBLE user;
  EVEL_OPTION_DOUBLE wait;
} MEASUREMENT_CPU_USE;


/**************************************************************************//**
 * Disk Usage.
 * JSON equivalent field: diskUsage
 *****************************************************************************/
typedef struct measurement_disk_use {
  char * id;
  EVEL_OPTION_DOUBLE iotimeavg;
  EVEL_OPTION_DOUBLE iotimelast;
  EVEL_OPTION_DOUBLE iotimemax;
  EVEL_OPTION_DOUBLE iotimemin;
  EVEL_OPTION_DOUBLE mergereadavg;
  EVEL_OPTION_DOUBLE mergereadlast;
  EVEL_OPTION_DOUBLE mergereadmax;
  EVEL_OPTION_DOUBLE mergereadmin;
  EVEL_OPTION_DOUBLE mergewriteavg;
  EVEL_OPTION_DOUBLE mergewritelast;
  EVEL_OPTION_DOUBLE mergewritemax;
  EVEL_OPTION_DOUBLE mergewritemin;
  EVEL_OPTION_DOUBLE octetsreadavg;
  EVEL_OPTION_DOUBLE octetsreadlast;
  EVEL_OPTION_DOUBLE octetsreadmax;
  EVEL_OPTION_DOUBLE octetsreadmin;
  EVEL_OPTION_DOUBLE octetswriteavg;
  EVEL_OPTION_DOUBLE octetswritelast;
  EVEL_OPTION_DOUBLE octetswritemax;
  EVEL_OPTION_DOUBLE octetswritemin;
  EVEL_OPTION_DOUBLE opsreadavg;
  EVEL_OPTION_DOUBLE opsreadlast;
  EVEL_OPTION_DOUBLE opsreadmax;
  EVEL_OPTION_DOUBLE opsreadmin;
  EVEL_OPTION_DOUBLE opswriteavg;
  EVEL_OPTION_DOUBLE opswritelast;
  EVEL_OPTION_DOUBLE opswritemax;
  EVEL_OPTION_DOUBLE opswritemin;
  EVEL_OPTION_DOUBLE pendingopsavg;
  EVEL_OPTION_DOUBLE pendingopslast;
  EVEL_OPTION_DOUBLE pendingopsmax;
  EVEL_OPTION_DOUBLE pendingopsmin;
  EVEL_OPTION_DOUBLE timereadavg;
  EVEL_OPTION_DOUBLE timereadlast;
  EVEL_OPTION_DOUBLE timereadmax;
  EVEL_OPTION_DOUBLE timereadmin;
  EVEL_OPTION_DOUBLE timewriteavg;
  EVEL_OPTION_DOUBLE timewritelast;
  EVEL_OPTION_DOUBLE timewritemax;
  EVEL_OPTION_DOUBLE timewritemin;

} MEASUREMENT_DISK_USE;

/**************************************************************************//**
 * Add an additional Disk usage value name/value pair to the Measurement.
 *
 * The name and value are null delimited ASCII strings.  The library takes
 * a copy so the caller does not have to preserve values after the function
 * returns.
 *
 * @param measurement   Pointer to the measurement.
 * @param id            ASCIIZ string with the CPU's identifier.
 * @param usage         Disk utilization.
 *****************************************************************************/
MEASUREMENT_DISK_USE * evel_measurement_new_disk_use_add(EVENT_MEASUREMENT * measurement, char * id);

/**************************************************************************//**
 * Filesystem Usage.
 * JSON equivalent field: filesystemUsage
 *****************************************************************************/
typedef struct measurement_fsys_use {
  char * filesystem_name;
  double block_configured;
  double block_iops;
  double block_used;
  double ephemeral_configured;
  double ephemeral_iops;
  double ephemeral_used;
} MEASUREMENT_FSYS_USE;

/**************************************************************************//**
 * Memory Usage.
 * JSON equivalent field: memoryUsage
 *****************************************************************************/
typedef struct measurement_mem_use {
  char * id;
  char * vmid;
  double membuffsz;
  EVEL_OPTION_DOUBLE memcache;
  EVEL_OPTION_DOUBLE memconfig;
  EVEL_OPTION_DOUBLE memfree;
  EVEL_OPTION_DOUBLE slabrecl;
  EVEL_OPTION_DOUBLE slabunrecl;
  EVEL_OPTION_DOUBLE memused;
} MEASUREMENT_MEM_USE;

/**************************************************************************//**
 * Add an additional Memory usage value name/value pair to the Measurement.
 *
 * The name and value are null delimited ASCII strings.  The library takes
 * a copy so the caller does not have to preserve values after the function
 * returns.
 *
 * @param measurement   Pointer to the measurement.
 * @param id            ASCIIZ string with the Memory identifier.
 * @param vmidentifier  ASCIIZ string with the VM's identifier.
 * @param membuffsz     Memory Size.
 *
 * @return  Returns pointer to memory use structure in measurements
 *****************************************************************************/
MEASUREMENT_MEM_USE * evel_measurement_new_mem_use_add(EVENT_MEASUREMENT * measurement,
                                 char * id,  char *vmidentifier,  double membuffsz);

/**************************************************************************//**
 * Set kilobytes of memory used for cache
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param mem_use      Pointer to the Memory Use.
 * @param val          double
 *****************************************************************************/
void evel_measurement_mem_use_memcache_set(MEASUREMENT_MEM_USE * const mem_use,
                                    const double val);
/**************************************************************************//**
 * Set kilobytes of memory configured in the virtual machine on which the VNFC reporting
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param mem_use      Pointer to the Memory Use.
 * @param val          double
 *****************************************************************************/
void evel_measurement_mem_use_memconfig_set(MEASUREMENT_MEM_USE * const mem_use,
                                    const double val);
/**************************************************************************//**
 * Set kilobytes of physical RAM left unused by the system
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param mem_use      Pointer to the Memory Use.
 * @param val          double
 *****************************************************************************/
void evel_measurement_mem_use_memfree_set(MEASUREMENT_MEM_USE * const mem_use,
                                    const double val);
/**************************************************************************//**
 * Set the part of the slab that can be reclaimed such as caches measured in kilobytes
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param mem_use      Pointer to the Memory Use.
 * @param val          double
 *****************************************************************************/
void evel_measurement_mem_use_slab_reclaimed_set(MEASUREMENT_MEM_USE * const mem_use,
                                    const double val);
/**************************************************************************//**
 * Set the part of the slab that cannot be reclaimed such as caches measured in kilobytes
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param mem_use      Pointer to the Memory Use.
 * @param val          double
 *****************************************************************************/
void evel_measurement_mem_use_slab_unreclaimable_set(MEASUREMENT_MEM_USE * const mem_use,
                                    const double val);
/**************************************************************************//**
 * Set the total memory minus the sum of free, buffered, cached and slab memory in kilobytes
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param mem_use      Pointer to the Memory Use.
 * @param val          double
 *****************************************************************************/
void evel_measurement_mem_use_usedup_set(MEASUREMENT_MEM_USE * const mem_use,
                                    const double val);
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
typedef struct measurement_vnic_performance {
  /***************************************************************************/
  /* Optional fields                                                         */
  /***************************************************************************/
  /*Cumulative count of broadcast packets received as read at the end of
   the measurement interval*/
  EVEL_OPTION_DOUBLE recvd_bcast_packets_acc;
  /*Count of broadcast packets received within the measurement interval*/
  EVEL_OPTION_DOUBLE recvd_bcast_packets_delta;
  /*Cumulative count of discarded packets received as read at the end of
   the measurement interval*/
  EVEL_OPTION_DOUBLE recvd_discarded_packets_acc;
  /*Count of discarded packets received within the measurement interval*/
  EVEL_OPTION_DOUBLE recvd_discarded_packets_delta;
  /*Cumulative count of error packets received as read at the end of
   the measurement interval*/
  EVEL_OPTION_DOUBLE recvd_error_packets_acc;
  /*Count of error packets received within the measurement interval*/
  EVEL_OPTION_DOUBLE recvd_error_packets_delta;
  /*Cumulative count of multicast packets received as read at the end of
   the measurement interval*/
  EVEL_OPTION_DOUBLE recvd_mcast_packets_acc;
  /*Count of mcast packets received within the measurement interval*/
  EVEL_OPTION_DOUBLE recvd_mcast_packets_delta;
  /*Cumulative count of octets received as read at the end of
   the measurement interval*/
  EVEL_OPTION_DOUBLE recvd_octets_acc;
  /*Count of octets received within the measurement interval*/
  EVEL_OPTION_DOUBLE recvd_octets_delta;
  /*Cumulative count of all packets received as read at the end of
   the measurement interval*/
  EVEL_OPTION_DOUBLE recvd_total_packets_acc;
  /*Count of all packets received within the measurement interval*/
  EVEL_OPTION_DOUBLE recvd_total_packets_delta;
  /*Cumulative count of unicast packets received as read at the end of
   the measurement interval*/
  EVEL_OPTION_DOUBLE recvd_ucast_packets_acc;
  /*Count of unicast packets received within the measurement interval*/
  EVEL_OPTION_DOUBLE recvd_ucast_packets_delta;
  /*Cumulative count of transmitted broadcast packets at the end of
   the measurement interval*/
  EVEL_OPTION_DOUBLE tx_bcast_packets_acc;
  /*Count of transmitted broadcast packets within the measurement interval*/
  EVEL_OPTION_DOUBLE tx_bcast_packets_delta;
  /*Cumulative count of transmit discarded packets at the end of
   the measurement interval*/
  EVEL_OPTION_DOUBLE tx_discarded_packets_acc;
  /*Count of transmit discarded packets within the measurement interval*/
  EVEL_OPTION_DOUBLE tx_discarded_packets_delta;
  /*Cumulative count of transmit error packets at the end of
   the measurement interval*/
  EVEL_OPTION_DOUBLE tx_error_packets_acc;
  /*Count of transmit error packets within the measurement interval*/
  EVEL_OPTION_DOUBLE tx_error_packets_delta;
  /*Cumulative count of transmit multicast packets at the end of
   the measurement interval*/
  EVEL_OPTION_DOUBLE tx_mcast_packets_acc;
  /*Count of transmit multicast packets within the measurement interval*/
  EVEL_OPTION_DOUBLE tx_mcast_packets_delta;
  /*Cumulative count of transmit octets at the end of
   the measurement interval*/
  EVEL_OPTION_DOUBLE tx_octets_acc;
  /*Count of transmit octets received within the measurement interval*/
  EVEL_OPTION_DOUBLE tx_octets_delta;
  /*Cumulative count of all transmit packets at the end of
   the measurement interval*/
  EVEL_OPTION_DOUBLE tx_total_packets_acc;
  /*Count of transmit packets within the measurement interval*/
  EVEL_OPTION_DOUBLE tx_total_packets_delta;
  /*Cumulative count of all transmit unicast packets at the end of
   the measurement interval*/
  EVEL_OPTION_DOUBLE tx_ucast_packets_acc;
  /*Count of transmit unicast packets within the measurement interval*/
  EVEL_OPTION_DOUBLE tx_ucast_packets_delta;
  /* Indicates whether vNicPerformance values are likely inaccurate
           due to counter overflow or other condtions*/
  char *valuesaresuspect;
  char *vnic_id;

} MEASUREMENT_VNIC_PERFORMANCE;

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
#define EVEL_MOBILE_FLOW_MINOR_VERSION 2

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
  DLIST additional_info;                         /* JSON: additionalFields */

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

/*****************************************************************************/
/* Supported Other field version.                                            */
/*****************************************************************************/
#define EVEL_OTHER_EVENT_MAJOR_VERSION 1
#define EVEL_OTHER_EVENT_MINOR_VERSION 1

/**************************************************************************//**
 * Other.
 * JSON equivalent field: otherFields
 *****************************************************************************/
typedef struct event_other {
  EVENT_HEADER header;
  int major_version;
  int minor_version;

  HASHTABLE_T *namedarrays; /* HASHTABLE_T */
  DLIST jsonobjects; /* DLIST of EVEL_JSON_OBJECT */
  DLIST namedvalues;
} EVENT_OTHER;

/**************************************************************************//**
 * Other Field.
 * JSON equivalent field: otherFields
 *****************************************************************************/
typedef struct other_field {
  char * name;
  char * value;
} OTHER_FIELD;


/*****************************************************************************/
/* Supported Service Events version.                                         */
/*****************************************************************************/
#define EVEL_HEARTBEAT_FIELD_MAJOR_VERSION 1
#define EVEL_HEARTBEAT_FIELD_MINOR_VERSION 1


/*****************************************************************************/
/* Supported Signaling version.                                              */
/*****************************************************************************/
#define EVEL_SIGNALING_MAJOR_VERSION 2
#define EVEL_SIGNALING_MINOR_VERSION 1

/**************************************************************************//**
 * Vendor VNF Name fields.
 * JSON equivalent field: vendorVnfNameFields
 *****************************************************************************/
typedef struct vendor_vnfname_field {
  char * vendorname;
  EVEL_OPTION_STRING vfmodule;
  EVEL_OPTION_STRING vnfname;
} VENDOR_VNFNAME_FIELD;

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
  VENDOR_VNFNAME_FIELD vnfname_field;
  EVEL_OPTION_STRING correlator;                         /* JSON: correlator */
  EVEL_OPTION_STRING local_ip_address;               /* JSON: localIpAddress */
  EVEL_OPTION_STRING local_port;                          /* JSON: localPort */
  EVEL_OPTION_STRING remote_ip_address;             /* JSON: remoteIpAddress */
  EVEL_OPTION_STRING remote_port;                        /* JSON: remotePort */

  /***************************************************************************/
  /* Optional fields                                                         */
  /***************************************************************************/
  EVEL_OPTION_STRING compressed_sip;                  /* JSON: compressedSip */
  EVEL_OPTION_STRING summary_sip;                        /* JSON: summarySip */
  DLIST additional_info;

} EVENT_SIGNALING;

/**************************************************************************//**
 * Sgnaling Additional Field.
 * JSON equivalent field: additionalFields
 *****************************************************************************/
typedef struct signaling_additional_field {
  char * name;
  char * value;
} SIGNALING_ADDL_FIELD;

/*****************************************************************************/
/* Supported State Change version.                                           */
/*****************************************************************************/
#define EVEL_STATE_CHANGE_MAJOR_VERSION 1
#define EVEL_STATE_CHANGE_MINOR_VERSION 2

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
  double version;

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
#define EVEL_SYSLOG_MINOR_VERSION 2

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
  EVEL_OPTION_STRING additional_filters;
  EVEL_OPTION_STRING event_source_host;
  EVEL_OPTION_INT syslog_facility;
  EVEL_OPTION_INT syslog_priority;
  EVEL_OPTION_STRING syslog_proc;
  EVEL_OPTION_INT syslog_proc_id;
  EVEL_OPTION_STRING syslog_s_data;
  EVEL_OPTION_STRING syslog_sdid;
  EVEL_OPTION_STRING syslog_severity;
  double syslog_fver;
  EVEL_OPTION_INT syslog_ver;

} EVENT_SYSLOG;

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
 * Initialize an event instance id.
 *
 * @param vfield        Pointer to the event vnfname field being initialized.
 * @param vendor_id     The vendor id to encode in the event instance id.
 * @param event_id      The event id to encode in the event instance id.
 *****************************************************************************/
void evel_init_vendor_field(VENDOR_VNFNAME_FIELD * const vfield,
                                 const char * const vendor_name);

/**************************************************************************//**
 * Set the Vendor module property of the Vendor.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param vfield        Pointer to the Vendor field.
 * @param module_name   The module name to be set. ASCIIZ string. The caller
 *                      does not need to preserve the value once the function
 *                      returns.
 *****************************************************************************/
void evel_vendor_field_module_set(VENDOR_VNFNAME_FIELD * const vfield,
                                    const char * const module_name);
/**************************************************************************//**
 * Set the Vendor module property of the Vendor.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param vfield        Pointer to the Vendor field.
 * @param module_name   The module name to be set. ASCIIZ string. The caller
 *                      does not need to preserve the value once the function
 *                      returns.
 *****************************************************************************/
void evel_vendor_field_vnfname_set(VENDOR_VNFNAME_FIELD * const vfield,
                                    const char * const vnfname);
/**************************************************************************//**
 * Free an event instance id.
 *
 * @param vfield   Pointer to the event vnfname_field being freed.
 *****************************************************************************/
void evel_free_event_vendor_field(VENDOR_VNFNAME_FIELD * const vfield);

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
 * Create a new heartbeat event of given name and type.
 *
 * @note that the heartbeat is just a "naked" commonEventHeader!
 *
 * @param event_name  Unique Event Name confirming Domain AsdcModel Description
 * @param event_id    A universal identifier of the event for: troubleshooting correlation, analysis, etc
 *
 * @returns pointer to the newly manufactured ::EVENT_HEADER.  If the event is
 *          not used it must be released using ::evel_free_event
 * @retval  NULL  Failed to create the event.
 *****************************************************************************/
EVENT_HEADER * evel_new_heartbeat_nameid(const char* ev_name, const char *ev_id);


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
void evel_init_header(EVENT_HEADER * const header,const char *const eventname);

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

/**************************************************************************//**
 * Set the NFC Naming code property of the event header.
 *
 * @param header        Pointer to the ::EVENT_HEADER.
 * @param nfcnamingcode String
 *****************************************************************************/
void evel_nfcnamingcode_set(EVENT_HEADER * const header,
                         const char * const nfcnam);
/**************************************************************************//**
 * Set the NF Naming code property of the event header.
 *
 * @param header        Pointer to the ::EVENT_HEADER.
 * @param nfnamingcode String
 *****************************************************************************/
void evel_nfnamingcode_set(EVENT_HEADER * const header,
                         const char * const nfnam);

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
 * @note    The mandatory fields on the Fault must be supplied to this factory
 *          function and are immutable once set.  Optional fields have explicit
 *          setter functions, but again values may only be set once so that the
 *          Fault has immutable properties.
 * @param event_name    Unique Event Name
 * @param event_id    A universal identifier of the event for analysis etc
 * @param   condition   The condition indicated by the Fault.
 * @param   specific_problem  The specific problem triggering the fault.
 * @param   priority    The priority of the event.
 * @param   severity    The severity of the Fault.
 * @param   ev_source_type    Source of Alarm event
 * @param   version     fault version
 * @param   status      status of Virtual Function
 * @returns pointer to the newly manufactured ::EVENT_FAULT.  If the event is
 *          not used (i.e. posted) it must be released using ::evel_free_fault.
 * @retval  NULL  Failed to create the event.
 *****************************************************************************/
EVENT_FAULT * evel_new_fault(const char* ev_name, const char *ev_id,
			     const char * const condition,
                             const char * const specific_problem,
                             EVEL_EVENT_PRIORITIES priority,
                             EVEL_SEVERITIES severity,
                             EVEL_SOURCE_TYPES ev_source_type,
                             EVEL_VF_STATUSES status);

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
 * Set the Fault Category property of the Fault.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param fault      Pointer to the fault.
 * @param category   Category : license, link, routing, security, signaling.
 *                       ASCIIZ string. The caller
 *                   does not need to preserve the value once the function
 *                   returns.
 *****************************************************************************/
void evel_fault_category_set(EVENT_FAULT * fault,
                              const char * const category);

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
 * @param event_name    Unique Event Name
 * @param event_id    A universal identifier of the event for analysis etc
 *
 * @returns pointer to the newly manufactured ::EVENT_MEASUREMENT.  If the
 *          event is not used (i.e. posted) it must be released using
 *          ::evel_free_event.
 * @retval  NULL  Failed to create the event.
 *****************************************************************************/
EVENT_MEASUREMENT * evel_new_measurement(double measurement_interval,const char* ev_name, const char *ev_id);

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
MEASUREMENT_CPU_USE * evel_measurement_new_cpu_use_add(EVENT_MEASUREMENT * measurement, char * id, double usage);

/**************************************************************************//**
 * Set the CPU Idle value in measurement interval
 *   percentage of CPU time spent in the idle task
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param cpu_use      Pointer to the CPU Use.
 * @param val          double
 *****************************************************************************/
void evel_measurement_cpu_use_idle_set(MEASUREMENT_CPU_USE *const cpu_use,
                                    const double val);

/**************************************************************************//**
 * Set the percentage of time spent servicing interrupts
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param cpu_use      Pointer to the CPU Use.
 * @param val          double
 *****************************************************************************/
void evel_measurement_cpu_use_interrupt_set(MEASUREMENT_CPU_USE * const cpu_use,
                                    const double val);

/**************************************************************************//**
 * Set the percentage of time spent running user space processes that have been niced
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param cpu_use      Pointer to the CPU Use.
 * @param val          double
 *****************************************************************************/
void evel_measurement_cpu_use_nice_set(MEASUREMENT_CPU_USE * const cpu_use,
                                    const double val);

/**************************************************************************//**
 * Set the percentage of time spent handling soft irq interrupts
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param cpu_use      Pointer to the CPU Use.
 * @param val          double
 *****************************************************************************/
void evel_measurement_cpu_use_softirq_set(MEASUREMENT_CPU_USE * const cpu_use,
                                    const double val);
/**************************************************************************//**
 * Set the percentage of time spent in involuntary wait
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param cpu_use      Pointer to the CPU Use.
 * @param val          double
 *****************************************************************************/
void evel_measurement_cpu_use_steal_set(MEASUREMENT_CPU_USE * const cpu_use,
                                    const double val);
/**************************************************************************//**
 * Set the percentage of time spent on system tasks running the kernel
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param cpu_use      Pointer to the CPU Use.
 * @param val          double
 *****************************************************************************/
void evel_measurement_cpu_use_system_set(MEASUREMENT_CPU_USE * const cpu_use,
                                    const double val);
/**************************************************************************//**
 * Set the percentage of time spent running un-niced user space processes
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param cpu_use      Pointer to the CPU Use.
 * @param val          double
 *****************************************************************************/
void evel_measurement_cpu_use_usageuser_set(MEASUREMENT_CPU_USE * const cpu_use,
                                    const double val);
/**************************************************************************//**
 * Set the percentage of CPU time spent waiting for I/O operations to complete
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param cpu_use      Pointer to the CPU Use.
 * @param val          double
 *****************************************************************************/
void evel_measurement_cpu_use_wait_set(MEASUREMENT_CPU_USE * const cpu_use,
                                    const double val);

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
                                   double block_iops,
                                   double ephemeral_configured,
                                   double ephemeral_used,
                                   double ephemeral_iops);

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
                                              int scaling_metric);

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
 * @note    The mandatory fields on the ::MEASUREMENT_VNIC_PERFORMANCE must be supplied
 *          to this factory function and are immutable once set. Optional
 *          fields have explicit setter functions, but again values may only be
 *          set once so that the ::MEASUREMENT_VNIC_PERFORMANCE has immutable
 *          properties.
 *
 * @param vnic_id               ASCIIZ string with the vNIC's ID.
 * @param val_suspect           True or false confidence in data.
 *
 * @returns pointer to the newly manufactured ::MEASUREMENT_VNIC_PERFORMANCE.
 *          If the structure is not used it must be released using
 *          ::evel_measurement_free_vnic_performance.
 * @retval  NULL  Failed to create the vNIC Use.
 *****************************************************************************/
MEASUREMENT_VNIC_PERFORMANCE * evel_measurement_new_vnic_performance(char * const vnic_id, char * const val_suspect);

/**************************************************************************//**
 * Free a vNIC Use.
 *
 * Free off the ::MEASUREMENT_VNIC_PERFORMANCE supplied.  Will free all the contained
 * allocated memory.
 *
 * @note It does not free the vNIC Use itself, since that may be part of a
 * larger structure.
 *****************************************************************************/
void evel_measurement_free_vnic_performance(MEASUREMENT_VNIC_PERFORMANCE * const vnic_performance);

/**************************************************************************//**
 * Set the Accumulated Broadcast Packets Received in measurement interval
 * property of the vNIC performance.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param vnic_performance      Pointer to the vNIC Use.
 * @param recvd_bcast_packets_acc
 *****************************************************************************/
void evel_vnic_performance_rx_bcast_pkt_acc_set(MEASUREMENT_VNIC_PERFORMANCE * const vnic_performance,
                                    const double recvd_bcast_packets_acc);
/**************************************************************************//**
 * Set the Delta Broadcast Packets Received in measurement interval
 * property of the vNIC performance.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param vnic_performance      Pointer to the vNIC Use.
 * @param recvd_bcast_packets_delta
 *****************************************************************************/
void evel_vnic_performance_rx_bcast_pkt_delta_set(MEASUREMENT_VNIC_PERFORMANCE * const vnic_performance,
                                    const double recvd_bcast_packets_delta);
/**************************************************************************//**
 * Set the Discarded Packets Received in measurement interval
 * property of the vNIC performance.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param vnic_performance      Pointer to the vNIC Use.
 * @param recvd_discard_packets_acc
 *****************************************************************************/
void evel_vnic_performance_rx_discard_pkt_acc_set(MEASUREMENT_VNIC_PERFORMANCE * const vnic_performance,
                                    const double recvd_discard_packets_acc);
/**************************************************************************//**
 * Set the Delta Discarded Packets Received in measurement interval
 * property of the vNIC performance.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param vnic_performance      Pointer to the vNIC Use.
 * @param recvd_discard_packets_delta
 *****************************************************************************/
void evel_vnic_performance_rx_discard_pkt_delta_set(MEASUREMENT_VNIC_PERFORMANCE * const vnic_performance,
                                    const double recvd_discard_packets_delta);
/**************************************************************************//**
 * Set the Error Packets Received in measurement interval
 * property of the vNIC performance.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param vnic_performance      Pointer to the vNIC Use.
 * @param recvd_error_packets_acc
 *****************************************************************************/
void evel_vnic_performance_rx_error_pkt_acc_set(MEASUREMENT_VNIC_PERFORMANCE * const vnic_performance,
                                    const double recvd_error_packets_acc);
/**************************************************************************//**
 * Set the Delta Error Packets Received in measurement interval
 * property of the vNIC performance.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param vnic_performance      Pointer to the vNIC Use.
 * @param recvd_error_packets_delta
 *****************************************************************************/
void evel_vnic_performance_rx_error_pkt_delta_set(MEASUREMENT_VNIC_PERFORMANCE * const vnic_performance,
                                    const double recvd_error_packets_delta);
/**************************************************************************//**
 * Set the Accumulated Multicast Packets Received in measurement interval
 * property of the vNIC performance.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param vnic_performance      Pointer to the vNIC Use.
 * @param recvd_mcast_packets_acc
 *****************************************************************************/
void evel_vnic_performance_rx_mcast_pkt_acc_set(MEASUREMENT_VNIC_PERFORMANCE * const vnic_performance,
                                    const double recvd_mcast_packets_acc);
/**************************************************************************//**
 * Set the Delta Multicast Packets Received in measurement interval
 * property of the vNIC performance.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param vnic_performance      Pointer to the vNIC Use.
 * @param recvd_mcast_packets_delta
 *****************************************************************************/
void evel_vnic_performance_rx_mcast_pkt_delta_set(MEASUREMENT_VNIC_PERFORMANCE * const vnic_performance,
                                    const double recvd_mcast_packets_delta);
/**************************************************************************//**
 * Set the Accumulated Octets Received in measurement interval
 * property of the vNIC performance.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param vnic_performance      Pointer to the vNIC Use.
 * @param recvd_octets_acc
 *****************************************************************************/
void evel_vnic_performance_rx_octets_acc_set(MEASUREMENT_VNIC_PERFORMANCE * const vnic_performance,
                                    const double recvd_octets_acc);
/**************************************************************************//**
 * Set the Delta Octets Received in measurement interval
 * property of the vNIC performance.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param vnic_performance      Pointer to the vNIC Use.
 * @param recvd_octets_delta
 *****************************************************************************/
void evel_vnic_performance_rx_octets_delta_set(MEASUREMENT_VNIC_PERFORMANCE * const vnic_performance,
                                    const double recvd_octets_delta);
/**************************************************************************//**
 * Set the Accumulated Total Packets Received in measurement interval
 * property of the vNIC performance.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param vnic_performance      Pointer to the vNIC Use.
 * @param recvd_total_packets_acc
 *****************************************************************************/
void evel_vnic_performance_rx_total_pkt_acc_set(MEASUREMENT_VNIC_PERFORMANCE * const vnic_performance,
                                    const double recvd_total_packets_acc);
/**************************************************************************//**
 * Set the Delta Total Packets Received in measurement interval
 * property of the vNIC performance.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param vnic_performance      Pointer to the vNIC Use.
 * @param recvd_total_packets_delta
 *****************************************************************************/
void evel_vnic_performance_rx_total_pkt_delta_set(MEASUREMENT_VNIC_PERFORMANCE * const vnic_performance,
                                    const double recvd_total_packets_delta);
/**************************************************************************//**
 * Set the Accumulated Unicast Packets Received in measurement interval
 * property of the vNIC performance.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param vnic_performance      Pointer to the vNIC Use.
 * @param recvd_ucast_packets_acc
 *****************************************************************************/
void evel_vnic_performance_rx_ucast_pkt_acc_set(MEASUREMENT_VNIC_PERFORMANCE * const vnic_performance,
                                    const double recvd_ucast_packets_acc);
/**************************************************************************//**
 * Set the Delta Unicast packets Received in measurement interval
 * property of the vNIC performance.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param vnic_performance      Pointer to the vNIC Use.
 * @param recvd_ucast_packets_delta
 *****************************************************************************/
void evel_vnic_performance_rx_ucast_pkt_delta_set(MEASUREMENT_VNIC_PERFORMANCE * const vnic_performance,
                                    const double recvd_ucast_packets_delta);
/**************************************************************************//**
 * Set the Transmitted Broadcast Packets in measurement interval
 * property of the vNIC performance.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param vnic_performance      Pointer to the vNIC Use.
 * @param tx_bcast_packets_acc
 *****************************************************************************/
void evel_vnic_performance_tx_bcast_pkt_acc_set(MEASUREMENT_VNIC_PERFORMANCE * const vnic_performance,
                                    const double tx_bcast_packets_acc);
/**************************************************************************//**
 * Set the Delta Broadcast packets Transmitted in measurement interval
 * property of the vNIC performance.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param vnic_performance      Pointer to the vNIC Use.
 * @param tx_bcast_packets_delta
 *****************************************************************************/
void evel_vnic_performance_tx_bcast_pkt_delta_set(MEASUREMENT_VNIC_PERFORMANCE * const vnic_performance,
                                    const double tx_bcast_packets_delta);
/**************************************************************************//**
 * Set the Transmitted Discarded Packets in measurement interval
 * property of the vNIC performance.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param vnic_performance      Pointer to the vNIC Use.
 * @param tx_discarded_packets_acc
 *****************************************************************************/
void evel_vnic_performance_tx_discarded_pkt_acc_set(MEASUREMENT_VNIC_PERFORMANCE * const vnic_performance,
                                    const double tx_discarded_packets_acc);
/**************************************************************************//**
 * Set the Delta Discarded packets Transmitted in measurement interval
 * property of the vNIC performance.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param vnic_performance      Pointer to the vNIC Use.
 * @param tx_discarded_packets_delta
 *****************************************************************************/
void evel_vnic_performance_tx_discarded_pkt_delta_set(MEASUREMENT_VNIC_PERFORMANCE * const vnic_performance,
                                    const double tx_discarded_packets_delta);
/**************************************************************************//**
 * Set the Transmitted Errored Packets in measurement interval
 * property of the vNIC performance.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param vnic_performance      Pointer to the vNIC Use.
 * @param tx_error_packets_acc
 *****************************************************************************/
void evel_vnic_performance_tx_error_pkt_acc_set(MEASUREMENT_VNIC_PERFORMANCE * const vnic_performance,
                                    const double tx_error_packets_acc);
/**************************************************************************//**
 * Set the Delta Errored packets Transmitted in measurement interval
 * property of the vNIC performance.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param vnic_performance      Pointer to the vNIC Use.
 * @param tx_error_packets_delta
 *****************************************************************************/
void evel_vnic_performance_tx_error_pkt_delta_set(MEASUREMENT_VNIC_PERFORMANCE * const vnic_performance,
                                    const double tx_error_packets_delta);
/**************************************************************************//**
 * Set the Transmitted Multicast Packets in measurement interval
 * property of the vNIC performance.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param vnic_performance      Pointer to the vNIC Use.
 * @param tx_mcast_packets_acc
 *****************************************************************************/
void evel_vnic_performance_tx_mcast_pkt_acc_set(MEASUREMENT_VNIC_PERFORMANCE * const vnic_performance,
                                    const double tx_mcast_packets_acc);
/**************************************************************************//**
 * Set the Delta Multicast packets Transmitted in measurement interval
 * property of the vNIC performance.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param vnic_performance      Pointer to the vNIC Use.
 * @param tx_mcast_packets_delta
 *****************************************************************************/
void evel_vnic_performance_tx_mcast_pkt_delta_set(MEASUREMENT_VNIC_PERFORMANCE * const vnic_performance,
                                    const double tx_mcast_packets_delta);
/**************************************************************************//**
 * Set the Transmitted Octets in measurement interval
 * property of the vNIC performance.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param vnic_performance      Pointer to the vNIC Use.
 * @param tx_octets_acc
 *****************************************************************************/
void evel_vnic_performance_tx_octets_acc_set(MEASUREMENT_VNIC_PERFORMANCE * const vnic_performance,
                                    const double tx_octets_acc);
/**************************************************************************//**
 * Set the Delta Octets Transmitted in measurement interval
 * property of the vNIC performance.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param vnic_performance      Pointer to the vNIC Use.
 * @param tx_octets_delta
 *****************************************************************************/
void evel_vnic_performance_tx_octets_delta_set(MEASUREMENT_VNIC_PERFORMANCE * const vnic_performance,
                                    const double tx_octets_delta);
/**************************************************************************//**
 * Set the Transmitted Total Packets in measurement interval
 * property of the vNIC performance.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param vnic_performance      Pointer to the vNIC Use.
 * @param tx_total_packets_acc
 *****************************************************************************/
void evel_vnic_performance_tx_total_pkt_acc_set(MEASUREMENT_VNIC_PERFORMANCE * const vnic_performance,
                                    const double tx_total_packets_acc);
/**************************************************************************//**
 * Set the Delta Total Packets Transmitted in measurement interval
 * property of the vNIC performance.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param vnic_performance      Pointer to the vNIC Use.
 * @param tx_total_packets_delta
 *****************************************************************************/
void evel_vnic_performance_tx_total_pkt_delta_set(MEASUREMENT_VNIC_PERFORMANCE * const vnic_performance,
                                    const double tx_total_packets_delta);
/**************************************************************************//**
 * Set the Transmitted Unicast Packets in measurement interval
 * property of the vNIC performance.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param vnic_performance      Pointer to the vNIC Use.
 * @param tx_ucast_packets_acc
 *****************************************************************************/
void evel_vnic_performance_tx_ucast_packets_acc_set(MEASUREMENT_VNIC_PERFORMANCE * const vnic_performance,
                                    const double tx_ucast_packets_acc);
/**************************************************************************//**
 * Set the Delta Octets Transmitted in measurement interval
 * property of the vNIC performance.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param vnic_performance      Pointer to the vNIC Use.
 * @param tx_ucast_packets_delta
 *****************************************************************************/
void evel_vnic_performance_tx_ucast_pkt_delta_set(MEASUREMENT_VNIC_PERFORMANCE * const vnic_performance,
                                    const double tx_ucast_packets_delta);

/**************************************************************************//**
 * Add an additional vNIC Use to the specified Measurement event.
 *
 * @param measurement   Pointer to the measurement.
 * @param vnic_performance      Pointer to the vNIC Use to add.
 *****************************************************************************/
void evel_meas_vnic_performance_add(EVENT_MEASUREMENT * const measurement,
                            MEASUREMENT_VNIC_PERFORMANCE * const vnic_performance);

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
 * @param valset                true or false confidence level
 * @param recvd_bcast_packets_acc         Recieved broadcast packets
 * @param recvd_bcast_packets_delta       Received delta broadcast packets
 * @param recvd_discarded_packets_acc     Recieved discarded packets
 * @param recvd_discarded_packets_delta   Received discarded delta packets
 * @param recvd_error_packets_acc         Received error packets
 * @param recvd_error_packets_delta,      Received delta error packets
 * @param recvd_mcast_packets_acc         Received multicast packets
 * @param recvd_mcast_packets_delta       Received delta multicast packets
 * @param recvd_octets_acc                Received octets
 * @param recvd_octets_delta              Received delta octets
 * @param recvd_total_packets_acc         Received total packets
 * @param recvd_total_packets_delta       Received delta total packets
 * @param recvd_ucast_packets_acc         Received Unicast packets
 * @param recvd_ucast_packets_delta       Received delta unicast packets
 * @param tx_bcast_packets_acc            Transmitted broadcast packets
 * @param tx_bcast_packets_delta          Transmitted delta broadcast packets
 * @param tx_discarded_packets_acc        Transmitted packets discarded
 * @param tx_discarded_packets_delta      Transmitted delta discarded packets
 * @param tx_error_packets_acc            Transmitted error packets
 * @param tx_error_packets_delta          Transmitted delta error packets
 * @param tx_mcast_packets_acc            Transmitted multicast packets accumulated
 * @param tx_mcast_packets_delta          Transmitted delta multicast packets
 * @param tx_octets_acc                   Transmitted octets
 * @param tx_octets_delta                 Transmitted delta octets
 * @param tx_total_packets_acc            Transmitted total packets
 * @param tx_total_packets_delta          Transmitted delta total packets
 * @param tx_ucast_packets_acc            Transmitted Unicast packets
 * @param tx_ucast_packets_delta          Transmitted delta Unicast packets
 *****************************************************************************/
void evel_measurement_vnic_performance_add(EVENT_MEASUREMENT * const measurement,
                               char * const vnic_id,
                               char * valset,
                               double recvd_bcast_packets_acc,
                               double recvd_bcast_packets_delta,
                               double recvd_discarded_packets_acc,
                               double recvd_discarded_packets_delta,
                               double recvd_error_packets_acc,
                               double recvd_error_packets_delta,
                               double recvd_mcast_packets_acc,
                               double recvd_mcast_packets_delta,
                               double recvd_octets_acc,
                               double recvd_octets_delta,
                               double recvd_total_packets_acc,
                               double recvd_total_packets_delta,
                               double recvd_ucast_packets_acc,
                               double recvd_ucast_packets_delta,
                               double tx_bcast_packets_acc,
                               double tx_bcast_packets_delta,
                               double tx_discarded_packets_acc,
                               double tx_discarded_packets_delta,
                               double tx_error_packets_acc,
                               double tx_error_packets_delta,
                               double tx_mcast_packets_acc,
                               double tx_mcast_packets_delta,
                               double tx_octets_acc,
                               double tx_octets_delta,
                               double tx_total_packets_acc,
                               double tx_total_packets_delta,
                               double tx_ucast_packets_acc,
                               double tx_ucast_packets_delta);

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
 * @param event_name    Unique Event Name
 * @param event_id    A universal identifier of the event for analysis etc
 *
 * @returns pointer to the newly manufactured ::EVENT_REPORT.  If the event is
 *          not used (i.e. posted) it must be released using
 *          ::evel_free_report.
 * @retval  NULL  Failed to create the event.
 *****************************************************************************/
EVENT_REPORT * evel_new_report(double measurement_interval,const char* ev_name, const char *ev_id);

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
 * @param event_name    Unique Event Name
 * @param event_id    A universal identifier of the event for analysis etc
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
		      const char* ev_name, const char *ev_id,
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
 * @param event_name    Unique Event Name
 * @param event_id    A universal identifier of the event for analysis etc
 * @param vendor_name   The vendor id to encode in the event vnf field.
 * @param module        The module to encode in the event.
 * @param vnfname       The Virtual network function to encode in the event.
 * @returns pointer to the newly manufactured ::EVENT_SIGNALING.  If the event
 *          is not used (i.e. posted) it must be released using
 *          ::evel_free_signaling.
 * @retval  NULL  Failed to create the event.
 *****************************************************************************/
EVENT_SIGNALING * evel_new_signaling(const char* ev_name, const char *ev_id,
				     const char * const vendor_name,
                                     const char * const correlator,
				     const char * const local_ip_address,
				     const char * const local_port,
				     const char * const remote_ip_address,
				     const char * const remote_port);

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
 * Add an additional value name/value pair to the SIP signaling.
 *
 * The name and value are null delimited ASCII strings.  The library takes
 * a copy so the caller does not have to preserve values after the function
 * returns.
 *
 * @param event     Pointer to the fault.
 * @param name      ASCIIZ string with the attribute's name.  The caller
 *                  does not need to preserve the value once the function
 *                  returns.
 * @param value     ASCIIZ string with the attribute's value.  The caller
 *                  does not need to preserve the value once the function
 *                  returns.
 *****************************************************************************/
void evel_signaling_addl_info_add(EVENT_SIGNALING * event, char * name, char * value);

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
 * Set the Vendor module property of the Signaling event.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param event         Pointer to the Signaling event.
 * @param modulename    The module name to be set. ASCIIZ string. The caller
 *                      does not need to preserve the value once the function
 *                      returns.
 *****************************************************************************/
void evel_signaling_vnfmodule_name_set(EVENT_SIGNALING * const event,
                                    const char * const module_name);
/**************************************************************************//**
 * Set the Vendor module property of the Signaling event.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param event         Pointer to the Signaling event.
 * @param vnfname       The Virtual Network function to be set. ASCIIZ string.
 *                      The caller does not need to preserve the value once
 *                      the function returns.
 *****************************************************************************/
void evel_signaling_vnfname_set(EVENT_SIGNALING * const event,
                                    const char * const vnfname);

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
 * @param event_name    Unique Event Name
 * @param event_id    A universal identifier of the event for analysis etc
 * @param new_state     The new state of the reporting entity.
 * @param old_state     The old state of the reporting entity.
 * @param interface     The card or port name of the reporting entity.
 *
 * @returns pointer to the newly manufactured ::EVENT_STATE_CHANGE.  If the
 *          event is not used it must be released using
 *          ::evel_free_state_change
 * @retval  NULL  Failed to create the event.
 *****************************************************************************/
EVENT_STATE_CHANGE * evel_new_state_change(const char* ev_name, const char *ev_id,
					   const EVEL_ENTITY_STATE new_state,
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
 * @param event_name    Unique Event Name
 * @param event_id    A universal identifier of the event for analysis etc
 * @param   event_source_type
 * @param   syslog_msg
 * @param   syslog_tag
 * @param   version
 *
 * @returns pointer to the newly manufactured ::EVENT_SYSLOG.  If the event is
 *          not used it must be released using ::evel_free_syslog
 * @retval  NULL  Failed to create the event.
 *****************************************************************************/
EVENT_SYSLOG * evel_new_syslog(const char* ev_name, const char *ev_id,
				EVEL_SOURCE_TYPES event_source_type,
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

/**************************************************************************//**
 * Set the Structured SDID property of the Syslog.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param syslog     Pointer to the Syslog.
 * @param sdid     The Structured Data to be set. ASCIIZ string. name@number
 *                 Caller does not need to preserve the value once the function
 *                   returns.
 *****************************************************************************/
void evel_syslog_sdid_set(EVENT_SYSLOG * syslog, const char * const sdid);

/**************************************************************************//**
 * Set the Structured Severity property of the Syslog.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param syslog     Pointer to the Syslog.
 * @param sdid     The Structured Data to be set. ASCIIZ string. 
 *                 Caller does not need to preserve the value once the function
 *                   returns.
 *****************************************************************************/
void evel_syslog_severity_set(EVENT_SYSLOG * syslog, const char * const severty);


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
 * @param event_name    Unique Event Name
 * @param event_id    A universal identifier of the event for analysis etc
 *
 * @returns pointer to the newly manufactured ::EVENT_OTHER.  If the event is
 *          not used it must be released using ::evel_free_other.
 * @retval  NULL  Failed to create the event.
 *****************************************************************************/
EVENT_OTHER * evel_new_other(const char* ev_name, const char *ev_id);

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
/* Supported Report version.                                                 */
/*****************************************************************************/
#define EVEL_VOICEQ_MAJOR_VERSION 1
#define EVEL_VOICEQ_MINOR_VERSION 1

/**************************************************************************//**
 * End of Call Voice Quality Metrices
 * JSON equivalent field: endOfCallVqmSummaries
 *****************************************************************************/
typedef struct end_of_call_vqm_summaries {
	/***************************************************************************/
	/* Mandatory fields                                                        */
	/***************************************************************************/
	char* adjacencyName;
	char* endpointDescription;

	/***************************************************************************/
	/* Optional fields                                                         */
	/***************************************************************************/
	EVEL_OPTION_INT endpointJitter;
	EVEL_OPTION_INT endpointRtpOctetsDiscarded;
	EVEL_OPTION_INT endpointRtpOctetsReceived;
	EVEL_OPTION_INT endpointRtpOctetsSent;
	EVEL_OPTION_INT endpointRtpPacketsDiscarded;
	EVEL_OPTION_INT endpointRtpPacketsReceived;
	EVEL_OPTION_INT endpointRtpPacketsSent;
	EVEL_OPTION_INT localJitter;
	EVEL_OPTION_INT localRtpOctetsDiscarded;
	EVEL_OPTION_INT localRtpOctetsReceived;
	EVEL_OPTION_INT localRtpOctetsSent;
	EVEL_OPTION_INT localRtpPacketsDiscarded;
	EVEL_OPTION_INT localRtpPacketsReceived;
	EVEL_OPTION_INT localRtpPacketsSent;
	EVEL_OPTION_INT mosCqe;
	EVEL_OPTION_INT packetsLost;
	EVEL_OPTION_INT packetLossPercent;
	EVEL_OPTION_INT rFactor;
	EVEL_OPTION_INT roundTripDelay;

} END_OF_CALL_VOICE_QUALITY_METRICS;

/**************************************************************************//**
* Voice QUality.
* JSON equivalent field: voiceQualityFields
*****************************************************************************/

typedef struct event_voiceQuality {
	/***************************************************************************/
	/* Header and version                                                      */
	/***************************************************************************/
	EVENT_HEADER header;
	int major_version;
	int minor_version;

	/***************************************************************************/
	/* Mandatory fields                                                        */
	/***************************************************************************/
	
	char *calleeSideCodec;
	char *callerSideCodec;
	char *correlator;
	char *midCallRtcp;
	VENDOR_VNFNAME_FIELD vendorVnfNameFields;
	END_OF_CALL_VOICE_QUALITY_METRICS *endOfCallVqmSummaries;

	/***************************************************************************/
	/* Optional fields                                                         */
	/***************************************************************************/
	EVEL_OPTION_STRING phoneNumber;
	DLIST additionalInformation;

} EVENT_VOICE_QUALITY;
/**************************************************************************//**
 * Voice Quality Additional Info.
 * JSON equivalent field: additionalInformation
 *****************************************************************************/
typedef struct voice_quality_additional_info {
  char * name;
  char * value;
} VOICE_QUALITY_ADDL_INFO;

/**************************************************************************//**
 * Create a new voice quality event.
 *
 * @note    The mandatory fields on the Voice Quality must be supplied to this 
 *          factory function and are immutable once set.  Optional fields have 
 *          explicit setter functions, but again values may only be set once 
 *          so that the Voice Quality has immutable properties.
 * @param event_name    Unique Event Name
 * @param event_id    A universal identifier of the event for analysis etc
 * @param   calleeSideCodec			Callee codec for the call.
 * @param   callerSideCodec			Caller codec for the call.
 * @param   correlator				Constant across all events on this call.
 * @param   midCallRtcp				Base64 encoding of the binary RTCP data
 *									(excluding Eth/IP/UDP headers).
 * @param   vendorVnfNameFields		Vendor, VNF and VfModule names.
 * @returns pointer to the newly manufactured ::EVENT_VOICE_QUALITY.  If the 
 *          event is not used (i.e. posted) it must be released using
			::evel_free_voice_quality.
 * @retval  NULL  Failed to create the event.
 *****************************************************************************/
EVENT_VOICE_QUALITY * evel_new_voice_quality(const char* ev_name, const char *ev_id,
	const char * const calleeSideCodec,
	const char * const callerSideCodec, const char * const correlator,
	const char * const midCallRtcp, const char * const vendorVnfNameFields);

/**************************************************************************//**
 * Set the Callee side codec for Call for domain Voice Quality
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param voiceQuality				Pointer to the Voice Quality Event.
 * @param calleeCodecForCall		The Callee Side Codec to be set.  ASCIIZ 
 *									string. The caller does not need to 
 *									preserve the value once the function
 *									returns.
 *****************************************************************************/
void evel_voice_quality_callee_codec_set(EVENT_VOICE_QUALITY * voiceQuality,
									const char * const calleeCodecForCall);

/**************************************************************************//**
 * Set the Caller side codec for Call for domain Voice Quality
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param voiceQuality				Pointer to the Voice Quality Event.
 * @param callerCodecForCall		The Caller Side Codec to be set.  ASCIIZ 
 *									string. The caller does not need to 
 *									preserve the value once the function
 *									returns.
 *****************************************************************************/
void evel_voice_quality_caller_codec_set(EVENT_VOICE_QUALITY * voiceQuality,
									const char * const callerCodecForCall);

/**************************************************************************//**
 * Set the correlator for domain Voice Quality
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param voiceQuality				Pointer to the Voice Quality Event.
 * @param correlator				The correlator value to be set.  ASCIIZ 
 *									string. The caller does not need to 
 *									preserve the value once the function
 *									returns.
 *****************************************************************************/
void evel_voice_quality_correlator_set(EVENT_VOICE_QUALITY * voiceQuality,
									const char * const vCorrelator);

/**************************************************************************//**
 * Set the RTCP Call Data for domain Voice Quality
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param voiceQuality				Pointer to the Voice Quality Event.
 * @param rtcpCallData		        The RTCP Call Data to be set.  ASCIIZ 
 *									string. The caller does not need to 
 *									preserve the value once the function
 *									returns.
 *****************************************************************************/
void evel_voice_quality_rtcp_data_set(EVENT_VOICE_QUALITY * voiceQuality,
									const char * const rtcpCallData);

/**************************************************************************//**
 * Set the Vendor VNF Name fields for domain Voice Quality
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param voiceQuality				Pointer to the Voice Quality Event.
 * @param nameFields		        The Vendor, VNF and VfModule names to be set.   
 *									ASCIIZ string. The caller does not need to 
 *									preserve the value once the function
 *									returns.
 *****************************************************************************/
void evel_voice_quality_name_fields_set(EVENT_VOICE_QUALITY * voiceQuality,
									const char * const nameFields);

/**************************************************************************//**
 * Add an End of Call Voice Quality Metrices

 * The adjacencyName and endpointDescription is null delimited ASCII string.  
 * The library takes a copy so the caller does not have to preserve values
 * after the function returns.
 *
 * @param voiceQuality     Pointer to the measurement.
 * @param adjacencyName						Adjacency name
 * @param endpointDescription				Enumeration: ‘Caller’, ‘Callee’.
 * @param endpointJitter					Endpoint jitter
 * @param endpointRtpOctetsDiscarded        Endpoint RTP octets discarded.
 * @param endpointRtpOctetsReceived			Endpoint RTP octets received.
 * @param endpointRtpOctetsSent				Endpoint RTP octets sent
 * @param endpointRtpPacketsDiscarded		Endpoint RTP packets discarded.
 * @param endpointRtpPacketsReceived		Endpoint RTP packets received.
 * @param endpointRtpPacketsSent			Endpoint RTP packets sent.
 * @param localJitter						Local jitter.
 * @param localRtpOctetsDiscarded			Local RTP octets discarded.
 * @param localRtpOctetsReceived			Local RTP octets received.
 * @param localRtpOctetsSent				Local RTP octets sent.
 * @param localRtpPacketsDiscarded			Local RTP packets discarded.
 * @param localRtpPacketsReceived			Local RTP packets received.
 * @param localRtpPacketsSent				Local RTP packets sent.
 * @param mosCqe							Decimal range from 1 to 5
 *											(1 decimal place)
 * @param packetsLost						No	Packets lost
 * @param packetLossPercent					Calculated percentage packet loss 
 * @param rFactor							rFactor from 0 to 100
 * @param roundTripDelay					Round trip delay in milliseconds
 *****************************************************************************/
void evel_voice_quality_end_metrics_add(EVENT_VOICE_QUALITY * voiceQuality,
	const char * adjacencyName, EVEL_SERVICE_ENDPOINT_DESC endpointDescription,
	int endpointJitter,
	int endpointRtpOctetsDiscarded,
	int endpointRtpOctetsReceived,
	int endpointRtpOctetsSent,
	int endpointRtpPacketsDiscarded,
	int endpointRtpPacketsReceived,
	int endpointRtpPacketsSent,
	int localJitter,
	int localRtpOctetsDiscarded,
	int localRtpOctetsReceived,
	int localRtpOctetsSent,
	int localRtpPacketsDiscarded,
	int localRtpPacketsReceived,
	int localRtpPacketsSent,
	int mosCqe,
	int packetsLost,
	int packetLossPercent,
	int rFactor,
	int roundTripDelay);

/**************************************************************************//**
 * Free a Voice Quality.
 *
 * Free off the Voce Quality supplied.  Will free all the contained allocated
 * memory.
 *
 * @note It does not free the Voice Quality itself, since that may be part of a
 * larger structure.
 *****************************************************************************/
void evel_free_voice_quality(EVENT_VOICE_QUALITY * voiceQuality);

/**************************************************************************//**
 * Add an additional value name/value pair to the Voice Quality.
 *
 * The name and value are null delimited ASCII strings.  The library takes
 * a copy so the caller does not have to preserve values after the function
 * returns.
 *
 * @param fault     Pointer to the fault.
 * @param name      ASCIIZ string with the attribute's name.  The caller
 *                  does not need to preserve the value once the function
 *                  returns.
 * @param value     ASCIIZ string with the attribute's value.  The caller
 *                  does not need to preserve the value once the function
 *                  returns.
 *****************************************************************************/
void evel_voice_quality_addl_info_add(EVENT_VOICE_QUALITY * voiceQuality, char * name, char * value);


/*****************************************************************************/
/*****************************************************************************/
/*                                                                           */
/*   THRESHOLD CROSSING ALERT                                                */
/*                                                                           */
/*****************************************************************************/
/*****************************************************************************/

typedef enum evel_event_action {
	  EVEL_EVENT_ACTION_CLEAR,
	  EVEL_EVENT_ACTION_CONTINUE,
	  EVEL_EVENT_ACTION_SET,
	  EVEL_MAX_EVENT_ACTION
}EVEL_EVENT_ACTION;
	
typedef enum evel_alert_type {
         EVEL_CARD_ANOMALY, 
   	 EVEL_ELEMENT_ANOMALY, 
    	 EVEL_INTERFACE_ANOMALY, 
    	 EVEL_SERVICE_ANOMALY,
         EVEL_MAX_ANOMALY
}EVEL_ALERT_TYPE;


typedef struct perf_counter {
	char * criticality;
	char * name;
	char * thresholdCrossed;
	char * value;
}PERF_COUNTER;


/*****************************************************************************/
/* Supported Threshold Crossing version.                                     */
/*****************************************************************************/
#define EVEL_THRESHOLD_CROSS_MAJOR_VERSION 1
#define EVEL_THRESHOLD_CROSS_MINOR_VERSION 1

/**************************************************************************//**
 * Threshold Crossing.
 * JSON equivalent field: Threshold Cross Fields
 *****************************************************************************/
typedef struct event_threshold_cross {
  /***************************************************************************/
  /* Header and version                                                      */
  /***************************************************************************/
  EVENT_HEADER header;
  int major_version;
  int minor_version;

  /***************************************************************************/
  /* Mandatory fields                                                        */
  /***************************************************************************/
  PERF_COUNTER additionalParameters;
  EVEL_EVENT_ACTION  alertAction;
  char *             alertDescription; 
  EVEL_ALERT_TYPE    alertType;
  unsigned long long collectionTimestamp; 
  EVEL_SEVERITIES    eventSeverity;
  unsigned long long eventStartTimestamp;

  /***************************************************************************/
  /* Optional fields                                                         */
  /***************************************************************************/
  DLIST additional_info;
  EVEL_OPTION_STRING    alertValue;
  DLIST     alertidList;
  EVEL_OPTION_STRING    dataCollector;
  EVEL_OPTION_STRING    elementType;
  EVEL_OPTION_STRING    interfaceName;
  EVEL_OPTION_STRING    networkService;
  EVEL_OPTION_STRING    possibleRootCause;

} EVENT_THRESHOLD_CROSS;


/**************************************************************************//**
 * Create a new Threshold Crossing Alert event.
 *
 * @note    The mandatory fields on the TCA must be supplied to this factory
 *          function and are immutable once set.  Optional fields have explicit
 *          setter functions, but again values may only be set once so that the
 *          TCA has immutable properties.
 *
 * @param event_name    Unique Event Name
 * @param event_id    A universal identifier of the event for analysis etc
 * @param char* tcriticality   Performance Counter Criticality MAJ MIN,
 * @param char* tname          Performance Counter Threshold name
 * @param char* tthresholdCrossed  Counter Threshold crossed value
 * @param char* tvalue             Counter actual value
 * @param EVEL_EVENT_ACTION talertAction   Alert set continue or clear
 * @param char*  talertDescription
 * @param EVEL_ALERT_TYPE     talertType    Kind of anamoly
 * @param unsigned long long  tcollectionTimestamp time at which alert was collected
 * @param EVEL_SEVERITIES     teventSeverity  Severity of Alert
 * @param unsigned long long  teventStartTimestamp Time when this alert started
 *
 * @returns pointer to the newly manufactured ::EVENT_THRESHOLD_CROSS.  If the
 *          event is not used it must be released using
 *          ::evel_free_threshold_cross
 * @retval  NULL  Failed to create the event.
 *****************************************************************************/
EVENT_THRESHOLD_CROSS * evel_new_threshold_cross(
				const char* ev_name, const char *ev_id,
                               char * tcriticality,
	                       char * tname,
	                       char * tthresholdCrossed,
	                       char * tvalue,
                               EVEL_EVENT_ACTION  talertAction,
                               char *             talertDescription, 
                               EVEL_ALERT_TYPE    talertType,
                               unsigned long long tcollectionTimestamp, 
                               EVEL_SEVERITIES    teventSeverity,
                               unsigned long long teventStartTimestamp);

/**************************************************************************//**
 * Free a Threshold cross event.
 *
 * Free off the Threshold crossing event supplied.  Will free all the contained allocated
 * memory.
 *
 * @note It does not free the Threshold Cross itself, since that may be part of a
 * larger structure.
 *****************************************************************************/
void evel_free_threshold_cross(EVENT_THRESHOLD_CROSS * const tcp);

/**************************************************************************//**
 * Set the Event Type property of the Threshold Cross.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param tcp  Pointer to the ::EVENT_THRESHOLD_CROSS.
 * @param type          The Event Type to be set. ASCIIZ string. The caller
 *                      does not need to preserve the value once the function
 *                      returns.
 *****************************************************************************/
void evel_threshold_cross_type_set(EVENT_THRESHOLD_CROSS * const tcp, char * type);

/**************************************************************************//**
 * Add an optional additional alertid value to Alert.
 *
 * @param alertid  Adds Alert ID
 *****************************************************************************/
void evel_threshold_cross_alertid_add(EVENT_THRESHOLD_CROSS * const event,char *  alertid);

  /**************************************************************************//**
   * Set the TCA probable Root cause.
   *
   * @param sheader     Possible root cause to Threshold
   *****************************************************************************/
  void evel_threshold_cross_possible_rootcause_set(EVENT_THRESHOLD_CROSS * const event, char *  sheader);
  /**************************************************************************//**
   * Set the TCA networking cause.
   *
   * @param sheader     Possible networking service value to Threshold
   *****************************************************************************/
  void evel_threshold_cross_networkservice_set(EVENT_THRESHOLD_CROSS * const event, char *  sheader);
  /**************************************************************************//**
   * Set the TCA Interface name.
   *
   * @param sheader     Interface name to threshold
   *****************************************************************************/
  void evel_threshold_cross_interfacename_set(EVENT_THRESHOLD_CROSS * const event,char *  sheader);
  /**************************************************************************//**
   * Set the TCA Data element type.
   *
   * @param sheader     element type of Threshold
   *****************************************************************************/
  void evel_threshold_cross_data_elementtype_set(EVENT_THRESHOLD_CROSS * const event,char *  sheader);
  /**************************************************************************//**
   * Set the TCA Data collector value.
   *
   * @param sheader     Data collector value
   *****************************************************************************/
  void evel_threshold_cross_data_collector_set(EVENT_THRESHOLD_CROSS * const event,char *  sheader);
  /**************************************************************************//**
   * Set the TCA alert value.
   *
   * @param sheader     Possible alert value
   *****************************************************************************/
  void evel_threshold_cross_alertvalue_set(EVENT_THRESHOLD_CROSS * const event,char *  sheader);

/**************************************************************************//**
 * Add an additional field name/value pair to the THRESHOLD CROSS event.
 *
 * The name and value are null delimited ASCII strings.  The library takes
 * a copy so the caller does not have to preserve values after the function
 * returns.
 *
 * @param state_change  Pointer to the ::EVENT_THRESHOLD_CROSS.
 * @param name          ASCIIZ string with the attribute's name.  The caller
 *                      does not need to preserve the value once the function
 *                      returns.
 * @param value         ASCIIZ string with the attribute's value.  The caller
 *                      does not need to preserve the value once the function
 *                      returns.
 *****************************************************************************/
void evel_threshold_cross_addl_info_add(EVENT_THRESHOLD_CROSS * const tcp,
                                      const char * const name,
                                      const char * const value);

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

