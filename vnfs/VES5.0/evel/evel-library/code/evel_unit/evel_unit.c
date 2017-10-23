/*************************************************************************//**
 *
 * Copyright © 2017 AT&T Intellectual Property. All rights reserved.
 *
 * Unless otherwise specified, all software contained herein is
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
 * ECOMP is a trademark and service mark of AT&T Intellectual Property.
 ****************************************************************************/
/**************************************************************************//**
 * @file
 * Unit tests for JSON encoding and throttling.
 *
 * This software is intended to show the essential elements of the library's
 * use.
 *
 ****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/time.h>

#include "evel.h"
#include "evel_internal.h"
#include "evel_throttle.h"
#include "metadata.h"

typedef enum {
  SERVICE_NONE,
  SERVICE_CODEC,
  SERVICE_TRANSCODING,
  SERVICE_RTCP,
  SERVICE_EOC_VQM,
  SERVICE_MARKER
} SERVICE_TEST;

/*****************************************************************************/
/* Local prototypes.                                                         */
/*****************************************************************************/
static void test_encode_heartbeat();
static void test_encode_header_overrides();
static void test_encode_fault();
static void test_encode_fault_with_escaping();
static void test_encode_measurement();
static void test_encode_mobile_mand();
static void test_encode_mobile_opts();
static void test_encode_other();
static void test_encode_report();
static void test_encode_service();
static void test_encode_service_subset(const SERVICE_TEST service_test);
static void test_encode_signaling();
static void test_encode_state_change();
static void test_encode_syslog();
static void test_json_response_junk();
static void test_json_provide_throttle_state();
static void test_json_measurement_interval();
static void test_json_throttle_spec_field();
static void test_json_throttle_spec_nv_pair();
static void test_json_throttle_spec_two_domains();
static void test_json_throttle_spec_bad_command_type();
static void test_encode_fault_throttled();
static void test_encode_measurement_throttled();
static void test_encode_mobile_throttled();
static void test_encode_other_throttled();
static void test_encode_report_throttled();
static void test_encode_service_throttled();
static void test_encode_signaling_throttled();
static void test_encode_state_change_throttled();
static void test_encode_syslog_throttled();
static void compare_strings(char * expected,
                            char * actual,
                            int max_size,
                            char * description);

/**************************************************************************//**
 * Main function.
 *
 * Runs all unit test cases, and fails hard on the first failure.
 *
 * @param[in] argc  Argument count.
 * @param[in] argv  Argument vector - for usage see usage_text.
 *****************************************************************************/
int main(int argc, char ** argv)
{
  assert(argc >= 0);
  assert(argv != NULL);

  /***************************************************************************/
  /* Fix our timezone to UTC.                                                */
  /***************************************************************************/
  putenv("TZ=UTC");

  /***************************************************************************/
  /* Initialize metadata.                                                    */
  /***************************************************************************/
  openstack_metadata_initialize();

  /***************************************************************************/
  /* Minimal initialisation to exercise the encoders.                        */
  /***************************************************************************/
  functional_role = "UNIT TEST";
  log_initialize(EVEL_LOG_DEBUG, "EVEL");

  /***************************************************************************/
  /* Test each encoder.                                                      */
  /***************************************************************************/
  test_encode_heartbeat();
  test_encode_header_overrides();
  test_encode_fault();
  test_encode_measurement();
  test_encode_mobile_mand();
  test_encode_mobile_opts();
  test_encode_other();
  test_encode_report();
  test_encode_service();
  test_encode_signaling();
  test_encode_state_change();
  test_encode_syslog();

  /***************************************************************************/
  /* Test JSON Throttle.                                                     */
  /***************************************************************************/
  test_json_response_junk();
  test_json_provide_throttle_state();
  test_json_measurement_interval();
  test_json_throttle_spec_field();
  test_json_throttle_spec_nv_pair();
  test_json_throttle_spec_two_domains();
  test_json_throttle_spec_bad_command_type();

  /***************************************************************************/
  /* Test each encoder with throttling applied.                              */
  /***************************************************************************/
  test_encode_fault_throttled();
  test_encode_measurement_throttled();
  test_encode_mobile_throttled();
  test_encode_other_throttled();
  test_encode_report_throttled();
  test_encode_service_throttled();
  test_encode_signaling_throttled();
  test_encode_state_change_throttled();
  test_encode_syslog_throttled();

  /***************************************************************************/
  /* Test character escaping.                                                */
  /***************************************************************************/
  test_encode_fault_with_escaping();

  printf ("\nAll Tests Passed\n");

  return 0;
}

/*****************************************************************************/
/* We link with this gettimeofday so that we get a fixed result              */
/*****************************************************************************/
int gettimeofday(struct timeval *tv,
                 struct timezone *tz __attribute__((unused)))
{
  tv->tv_sec = 1;
  tv->tv_usec = 2;
  return 0;
}

void test_encode_heartbeat()
{
  char * expected =
    "{\"event\": {"
    "\"commonEventHeader\": {"
    "\"domain\": \"heartbeat\", "
    "\"eventId\": \"121\", "
    "\"functionalRole\": \"UNIT TEST\", "
    "\"lastEpochMicrosec\": 1000002, "
    "\"priority\": \"Normal\", "
    "\"reportingEntityName\": \"Dummy VM name - No Metadata available\", "
    "\"sequence\": 121, "
    "\"sourceName\": \"Dummy VM name - No Metadata available\", "
    "\"startEpochMicrosec\": 1000002, "
    "\"version\": 1.2, "
    "\"eventType\": \"Autonomous heartbeat\", "
    "\"reportingEntityId\": \"Dummy VM UUID - No Metadata available\", "
    "\"sourceId\": \"Dummy VM UUID - No Metadata available\""
    "}}}";

  size_t json_size = 0;
  char json_body[EVEL_MAX_JSON_BODY];

  /***************************************************************************/
  /* Test the VM name/uuid once.                                             */
  /***************************************************************************/
  evel_set_next_event_sequence(121);

  EVENT_HEADER * heartbeat = evel_new_heartbeat();
  assert(heartbeat != NULL);

  json_size = evel_json_encode_event(
    json_body, EVEL_MAX_JSON_BODY, (EVENT_HEADER *) heartbeat);
  compare_strings(expected, json_body, EVEL_MAX_JSON_BODY, "Heartbeat");
  assert((json_size == strlen(json_body)) && "Bad size returned");

  evel_free_event(heartbeat);
}

void test_encode_header_overrides()
{
  char * expected =
    "{\"event\": {"
    "\"commonEventHeader\": {"
    "\"domain\": \"heartbeat\", "
    "\"eventId\": \"121\", "
    "\"functionalRole\": \"UNIT TEST\", "
    "\"lastEpochMicrosec\": 1000, "
    "\"priority\": \"Normal\", "
    "\"reportingEntityName\": \"entity_name_override\", "
    "\"sequence\": 121, "
    "\"sourceName\": \"Dummy VM name - No Metadata available\", "
    "\"startEpochMicrosec\": 1001, "
    "\"version\": 1.2, "
    "\"eventType\": \"Autonomous heartbeat\", "
    "\"reportingEntityId\": \"entity_id_override\", "
    "\"sourceId\": \"Dummy VM UUID - No Metadata available\""
    "}}}";

  size_t json_size = 0;
  char json_body[EVEL_MAX_JSON_BODY];

  /***************************************************************************/
  /* Test the VM name/uuid once.                                             */
  /***************************************************************************/
  evel_set_next_event_sequence(121);

  EVENT_HEADER * heartbeat = evel_new_heartbeat();
  assert(heartbeat != NULL);

  evel_start_epoch_set(heartbeat, 1001);
  evel_last_epoch_set(heartbeat, 1000);
  evel_reporting_entity_name_set(heartbeat, "entity_name_override");
  evel_reporting_entity_id_set(heartbeat, "entity_id_override");

  json_size = evel_json_encode_event(
    json_body, EVEL_MAX_JSON_BODY, (EVENT_HEADER *) heartbeat);
  compare_strings(expected, json_body, EVEL_MAX_JSON_BODY, "Heartbeat");
  assert((json_size == strlen(json_body)) && "Bad size returned");

  evel_free_event(heartbeat);
}

void test_encode_fault()
{
  char * expected =
    "{\"event\": {"
    "\"commonEventHeader\": {"
    "\"domain\": \"fault\", "
    "\"eventId\": \"122\", "
    "\"functionalRole\": \"UNIT TEST\", "
    "\"lastEpochMicrosec\": 1000002, "
    "\"priority\": \"Normal\", "
    "\"reportingEntityName\": \"Dummy VM name - No Metadata available\", "
    "\"sequence\": 122, "
    "\"sourceName\": \"Dummy VM name - No Metadata available\", "
    "\"startEpochMicrosec\": 1000002, "
    "\"version\": 1.2, "
    "\"eventType\": \"Bad things happen...\", "
    "\"reportingEntityId\": \"Dummy VM UUID - No Metadata available\", "
    "\"sourceId\": \"Dummy VM UUID - No Metadata available\""
    "}, "
    "\"faultFields\": {"
    "\"alarmCondition\": \"My alarm condition\", "
    "\"eventSeverity\": \"MAJOR\", "
    "\"eventSourceType\": \"other\", "
    "\"specificProblem\": \"It broke very badly\", "
    "\"eventCategory\": \"link\", "
    "\"vfStatus\": \"Active\", "
    "\"faultFieldsVersion\": 1.1, "
    "\"alarmAdditionalInformation\": ["
    "{\"name\": \"name1\", "
    "\"value\": \"value1\"}, "
    "{\"name\": \"name2\", "
    "\"value\": \"value2\"}], "
    "\"alarmInterfaceA\": \"My Interface Card\""
    "}}}";

  size_t json_size = 0;
  char json_body[EVEL_MAX_JSON_BODY];
  evel_set_next_event_sequence(122);
  EVENT_FAULT * fault = evel_new_fault("My alarm condition",
                                       "It broke very badly",
                                       EVEL_PRIORITY_NORMAL,
                                       EVEL_SEVERITY_MAJOR,
					EVEL_SOURCE_HOST,
                             EVEL_VF_STATUS_PREP_TERMINATE);
  assert(fault != NULL);
  evel_fault_type_set(fault, "Bad things happen...");
  evel_fault_interface_set(fault, "My Interface Card");
  evel_fault_addl_info_add(fault, "name1", "value1");
  evel_fault_addl_info_add(fault, "name2", "value2");

  json_size = evel_json_encode_event(
    json_body, EVEL_MAX_JSON_BODY, (EVENT_HEADER *) fault);
  compare_strings(expected, json_body, EVEL_MAX_JSON_BODY, "Fault");
  assert((json_size == strlen(json_body)) && "Bad size returned");

  evel_free_event(fault);
}

void test_encode_measurement()
{
  char * expected =
    "{\"event\": "
    "{\"commonEventHeader\": {"
    "\"domain\": \"measurementsForVfScaling\", "
    "\"eventId\": \"123\", "
    "\"functionalRole\": \"UNIT TEST\", "
    "\"lastEpochMicrosec\": 3000, "
    "\"priority\": \"Normal\", "
    "\"reportingEntityName\": \"entity_name\", "
    "\"sequence\": 123, "
    "\"sourceName\": \"Dummy VM name - No Metadata available\", "
    "\"startEpochMicrosec\": 2000, "
    "\"version\": 1.2, "
    "\"eventType\": \"Perf management...\", "
    "\"reportingEntityId\": \"entity_id\", "
    "\"sourceId\": \"Dummy VM UUID - No Metadata available\""
    "}, "
    "\"measurementsForVfScalingFields\": "
    "{"
    "\"measurementInterval\": 5.500000, "
    "\"concurrentSessions\": 1, "
    "\"configuredEntities\": 2, "
    "\"cpuUsageArray\": ["
    "{\"cpuIdentifier\": \"cpu1\", "
    "\"percentUsage\": 11.110000}, "
    "{\"cpuIdentifier\": \"cpu2\", "
    "\"percentUsage\": 22.220000}], "
    "\"filesystemUsageArray\": ["
    "{\"blockConfigured\": 100.110000, "
    "\"blockIops\": 33, "
    "\"blockUsed\": 100.220000, "
    "\"ephemeralConfigured\": 100.110000, "
    "\"ephemeralIops\": 44, "
    "\"ephemeralUsed\": 200.220000, "
    "\"filesystemName\": \"00-11-22\"}, "
    "{\"blockConfigured\": 300.110000, "
    "\"blockIops\": 55, "
    "\"blockUsed\": 300.220000, "
    "\"ephemeralConfigured\": 300.110000, "
    "\"ephemeralIops\": 66, "
    "\"ephemeralUsed\": 400.220000, "
    "\"filesystemName\": \"33-44-55\"}], "
    "\"latencyDistribution\": ["
    "{\"countsInTheBucket\": 20}, "
    "{\"lowEndOfLatencyBucket\": 10.000000, "
    "\"highEndOfLatencyBucket\": 20.000000, "
    "\"countsInTheBucket\": 30}], "
    "\"meanRequestLatency\": 4.400000, "
    "\"memoryConfigured\": 6.600000, "
    "\"memoryUsed\": 3.300000, "
    "\"requestRate\": 7, "
    "\"vNicUsageArray\": ["
    "{"
    "\"bytesIn\": 3, "
    "\"bytesOut\": 4, "
    "\"packetsIn\": 100, "
    "\"packetsOut\": 200, "
    "\"vNicIdentifier\": \"eth0\""
    "}, "
    "{"
    "\"bytesIn\": 13, "
    "\"bytesOut\": 14, "
    "\"packetsIn\": 110, "
    "\"packetsOut\": 240, "
    "\"vNicIdentifier\": \"eth1\", "
    "\"broadcastPacketsIn\": 11, "
    "\"broadcastPacketsOut\": 12, "
    "\"multicastPacketsIn\": 15, "
    "\"multicastPacketsOut\": 16, "
    "\"unicastPacketsIn\": 17, "
    "\"unicastPacketsOut\": 18"
    "}"
    "], "
    "\"aggregateCpuUsage\": 8.800000, "
    "\"numberOfMediaPortsInUse\": 1234, "
    "\"vnfcScalingMetric\": 1234.567800, "
    "\"errors\": {"
    "\"receiveDiscards\": 1, "
    "\"receiveErrors\": 0, "
    "\"transmitDiscards\": 2, "
    "\"transmitErrors\": 1}, "
    "\"featureUsageArray\": ["
    "{\"featureIdentifier\": \"FeatureA\", "
    "\"featureUtilization\": 123}, "
    "{\"featureIdentifier\": \"FeatureB\", "
    "\"featureUtilization\": 567}], "
    "\"codecUsageArray\": ["
    "{\"codecIdentifier\": \"G711a\", "
    "\"numberInUse\": 91}, "
    "{\"codecIdentifier\": \"G729ab\", "
    "\"numberInUse\": 92}], "
    "\"additionalMeasurements\": ["
    "{\"name\": \"Group1\", "
    "\"measurements\": ["
    "{\"name\": \"Name1\", "
    "\"value\": \"Value1\"}]}, "
    "{\"name\": \"Group2\", "
    "\"measurements\": ["
    "{\"name\": \"Name1\", "
    "\"value\": \"Value1\"}, "
    "{\"name\": \"Name2\", "
    "\"value\": \"Value2\"}]}], "
    "\"measurementsForVfScalingVersion\": 1.1}}}";

  size_t json_size = 0;
  char json_body[EVEL_MAX_JSON_BODY];
  EVENT_MEASUREMENT * measurement = NULL;
  MEASUREMENT_LATENCY_BUCKET * bucket = NULL;
  MEASUREMENT_VNIC_PERFORMANCE * vnic_use = NULL;
  MEASUREMENT_CPU_USE *cpu_use;

  /***************************************************************************/
  /* Measurement.                                                            */
  /***************************************************************************/
  evel_set_next_event_sequence(123);
  measurement = evel_new_measurement(5.5);
  assert(measurement != NULL);
  evel_measurement_type_set(measurement, "Perf management...");
  evel_measurement_conc_sess_set(measurement, 1);
  evel_measurement_cfg_ents_set(measurement, 2);
  evel_measurement_mean_req_lat_set(measurement, 4.4);
  evel_measurement_request_rate_set(measurement, 7);

  cpu_use = evel_measurement_new_cpu_use_add(measurement, "cpu1", 11.11);
  evel_measurement_cpu_use_idle_set(cpu_use,22.22);
  evel_measurement_cpu_use_interrupt_set(cpu_use,33.33);
  evel_measurement_cpu_use_nice_set(cpu_use,44.44);
  evel_measurement_cpu_use_softirq_set(cpu_use,55.55);
  evel_measurement_cpu_use_steal_set(cpu_use,66.66);
  evel_measurement_cpu_use_system_set(cpu_use,77.77);
  evel_measurement_cpu_use_usageuser_set(cpu_use,88.88);
  evel_measurement_cpu_use_wait_set(cpu_use,99.99);

  cpu_use = evel_measurement_new_cpu_use_add(measurement, "cpu2", 22.22);
  evel_measurement_cpu_use_idle_set(cpu_use,12.22);
  evel_measurement_cpu_use_interrupt_set(cpu_use,33.33);
  evel_measurement_cpu_use_nice_set(cpu_use,44.44);
  evel_measurement_cpu_use_softirq_set(cpu_use,55.55);
  evel_measurement_cpu_use_steal_set(cpu_use,66.66);
  evel_measurement_cpu_use_system_set(cpu_use,77.77);
  evel_measurement_cpu_use_usageuser_set(cpu_use,88.88);
  evel_measurement_cpu_use_wait_set(cpu_use,19.99);


  evel_measurement_fsys_use_add(measurement,"00-11-22",100.11, 100.22, 33,
                                200.11, 200.22, 44);
  evel_measurement_fsys_use_add(measurement,"33-44-55",300.11, 300.22, 55,
                                400.11, 400.22, 66);
  evel_start_epoch_set(&measurement->header, 2000);
  evel_last_epoch_set(&measurement->header, 3000);
  evel_reporting_entity_name_set(&measurement->header, "entity_name");
  evel_reporting_entity_id_set(&measurement->header, "entity_id");

  /***************************************************************************/
  /* Latency Bucket with no optional parameters.                             */
  /***************************************************************************/
  bucket = evel_new_meas_latency_bucket(20);
  evel_meas_latency_bucket_add(measurement, bucket);

  /***************************************************************************/
  /* Latency Bucket with all optional parameters.                            */
  /***************************************************************************/
  bucket = evel_new_meas_latency_bucket(30);
  evel_meas_latency_bucket_low_end_set(bucket, 10.0);
  evel_meas_latency_bucket_high_end_set(bucket, 20.0);
  evel_meas_latency_bucket_add(measurement, bucket);

  /***************************************************************************/
  /* vNIC Use with no optional parameters.                                   */
  /***************************************************************************/
  vnic_use = evel_new_measurement_vnic_use("eth0", 100, 200, 3, 4);
  evel_meas_vnic_use_add(measurement, vnic_use);

  /***************************************************************************/
  /* vNIC Use with all optional parameters.                                  */
  /***************************************************************************/
  vnic_use = evel_new_measurement_vnic_use("eth1", 110, 240, 13, 14);
  evel_vnic_use_bcast_pkt_in_set(vnic_use, 11);
  evel_vnic_use_bcast_pkt_out_set(vnic_use, 12);
  evel_vnic_use_mcast_pkt_in_set(vnic_use, 15);
  evel_vnic_use_mcast_pkt_out_set(vnic_use, 16);
  evel_vnic_use_ucast_pkt_in_set(vnic_use, 17);
  evel_vnic_use_ucast_pkt_out_set(vnic_use, 18);
  evel_meas_vnic_use_add(measurement, vnic_use);

  evel_measurement_errors_set(measurement, 1, 0, 2, 1);

  evel_measurement_feature_use_add(measurement, "FeatureA", 123);
  evel_measurement_feature_use_add(measurement, "FeatureB", 567);

  evel_measurement_codec_use_add(measurement, "G711a", 91);
  evel_measurement_codec_use_add(measurement, "G729ab", 92);

  evel_measurement_media_port_use_set(measurement, 1234);

  evel_measurement_vnfc_scaling_metric_set(measurement, 1234.5678);

  evel_measurement_custom_measurement_add(measurement,
                                          "Group1", "Name1", "Value1");
  evel_measurement_custom_measurement_add(measurement,
                                          "Group2", "Name1", "Value1");
  evel_measurement_custom_measurement_add(measurement,
                                          "Group2", "Name2", "Value2");

  json_size = evel_json_encode_event(
    json_body, EVEL_MAX_JSON_BODY, (EVENT_HEADER *) measurement);
  compare_strings(expected, json_body, EVEL_MAX_JSON_BODY, "Measurement");
  assert((json_size == strlen(json_body)) && "Bad size returned");

  evel_free_event(measurement);
}

void test_encode_mobile_mand()
{
  char * expected =
    "{\"event\": "
    "{\"commonEventHeader\": {"
    "\"domain\": \"mobileFlow\", "
    "\"eventId\": \"1241\", "
    "\"functionalRole\": \"UNIT TEST\", "
    "\"lastEpochMicrosec\": 1000002, "
    "\"priority\": \"Normal\", "
    "\"reportingEntityName\": \"Dummy VM name - No Metadata available\", "
    "\"sequence\": 1241, "
    "\"sourceName\": \"Dummy VM name - No Metadata available\", "
    "\"startEpochMicrosec\": 1000002, "
    "\"version\": 1.2, "
    "\"reportingEntityId\": \"Dummy VM UUID - No Metadata available\", "
    "\"sourceId\": \"Dummy VM UUID - No Metadata available\""
    "}, "
    "\"mobileFlowFields\": {"
    "\"flowDirection\": \"Outbound\", "
    "\"gtpPerFlowMetrics\": {"
    "\"avgBitErrorRate\": 12.300000, "
    "\"avgPacketDelayVariation\": 3.120000, "
    "\"avgPacketLatency\": 100, "
    "\"avgReceiveThroughput\": 2100, "
    "\"avgTransmitThroughput\": 500, "
    "\"flowActivationEpoch\": 1470409421, "
    "\"flowActivationMicrosec\": 987, "
    "\"flowDeactivationEpoch\": 1470409431, "
    "\"flowDeactivationMicrosec\": 11, "
    "\"flowDeactivationTime\": \"Fri, 05 Aug 2016 15:03:51 +0000\", "
    "\"flowStatus\": \"Working\", "
    "\"maxPacketDelayVariation\": 87, "
    "\"numActivationFailures\": 3, "
    "\"numBitErrors\": 17, "
    "\"numBytesReceived\": 123654, "
    "\"numBytesTransmitted\": 4561, "
    "\"numDroppedPackets\": 0, "
    "\"numL7BytesReceived\": 12, "
    "\"numL7BytesTransmitted\": 10, "
    "\"numLostPackets\": 1, "
    "\"numOutOfOrderPackets\": 3, "
    "\"numPacketErrors\": 7, "
    "\"numPacketsReceivedExclRetrans\": 899, "
    "\"numPacketsReceivedInclRetrans\": 901, "
    "\"numPacketsTransmittedInclRetrans\": 302, "
    "\"numRetries\": 6, "
    "\"numTimeouts\": 2, "
    "\"numTunneledL7BytesReceived\": 0, "
    "\"roundTripTime\": 110, "
    "\"timeToFirstByte\": 225"
    "}, "
    "\"ipProtocolType\": \"TCP\", "
    "\"ipVersion\": \"IPv4\", "
    "\"otherEndpointIpAddress\": \"2.3.4.1\", "
    "\"otherEndpointPort\": 2341, "
    "\"reportingEndpointIpAddr\": \"4.2.3.1\", "
    "\"reportingEndpointPort\": 4321"
    "}}}";

  size_t json_size = 0;
  char json_body[EVEL_MAX_JSON_BODY];
  MOBILE_GTP_PER_FLOW_METRICS * metrics = NULL;
  EVENT_MOBILE_FLOW * mobile_flow = NULL;

  /***************************************************************************/
  /* Mobile.                                                                 */
  /***************************************************************************/
  evel_set_next_event_sequence(1241);

  metrics = evel_new_mobile_gtp_flow_metrics(12.3,
                                             3.12,
                                             100,
                                             2100,
                                             500,
                                             1470409421,
                                             987,
                                             1470409431,
                                             11,
                                             (time_t)1470409431,
                                             "Working",
                                             87,
                                             3,
                                             17,
                                             123654,
                                             4561,
                                             0,
                                             12,
                                             10,
                                             1,
                                             3,
                                             7,
                                             899,
                                             901,
                                             302,
                                             6,
                                             2,
                                             0,
                                             110,
                                             225);
  assert(metrics != NULL);
  mobile_flow = evel_new_mobile_flow("Outbound",
                                     metrics,
                                     "TCP",
                                     "IPv4",
                                     "2.3.4.1",
                                     2341,
                                     "4.2.3.1",
                                     4321);
  assert(mobile_flow != NULL);

  json_size = evel_json_encode_event(
    json_body, EVEL_MAX_JSON_BODY, (EVENT_HEADER *) mobile_flow);
  compare_strings(expected, json_body, EVEL_MAX_JSON_BODY, "Mobile");
  assert((json_size == strlen(json_body)) && "Bad size returned");

  evel_free_event(mobile_flow);
}

void test_encode_mobile_opts()
{
  char * expected =
    "{\"event\": "
    "{\"commonEventHeader\": {"
    "\"domain\": \"mobileFlow\", "
    "\"eventId\": \"1242\", "
    "\"functionalRole\": \"UNIT TEST\", "
    "\"lastEpochMicrosec\": 1000002, "
    "\"priority\": \"Normal\", "
    "\"reportingEntityName\": \"Dummy VM name - No Metadata available\", "
    "\"sequence\": 1242, "
    "\"sourceName\": \"Dummy VM name - No Metadata available\", "
    "\"startEpochMicrosec\": 1000002, "
    "\"version\": 1.2, "
    "\"eventType\": \"Mobile flow...\", "
    "\"reportingEntityId\": \"Dummy VM UUID - No Metadata available\", "
    "\"sourceId\": \"Dummy VM UUID - No Metadata available\""
    "}, "
    "\"mobileFlowFields\": {"
    "\"flowDirection\": \"Inbound\", "
    "\"gtpPerFlowMetrics\": {"
    "\"avgBitErrorRate\": 132.000100, "
    "\"avgPacketDelayVariation\": 31.200000, "
    "\"avgPacketLatency\": 101, "
    "\"avgReceiveThroughput\": 2101, "
    "\"avgTransmitThroughput\": 501, "
    "\"flowActivationEpoch\": 1470409422, "
    "\"flowActivationMicrosec\": 988, "
    "\"flowDeactivationEpoch\": 1470409432, "
    "\"flowDeactivationMicrosec\": 12, "
    "\"flowDeactivationTime\": \"Fri, 05 Aug 2016 15:03:52 +0000\", "
    "\"flowStatus\": \"Inactive\", "
    "\"maxPacketDelayVariation\": 88, "
    "\"numActivationFailures\": 4, "
    "\"numBitErrors\": 18, "
    "\"numBytesReceived\": 123655, "
    "\"numBytesTransmitted\": 4562, "
    "\"numDroppedPackets\": 1, "
    "\"numL7BytesReceived\": 13, "
    "\"numL7BytesTransmitted\": 11, "
    "\"numLostPackets\": 2, "
    "\"numOutOfOrderPackets\": 4, "
    "\"numPacketErrors\": 8, "
    "\"numPacketsReceivedExclRetrans\": 900, "
    "\"numPacketsReceivedInclRetrans\": 902, "
    "\"numPacketsTransmittedInclRetrans\": 303, "
    "\"numRetries\": 7, "
    "\"numTimeouts\": 3, "
    "\"numTunneledL7BytesReceived\": 1, "
    "\"roundTripTime\": 111, "
    "\"timeToFirstByte\": 226, "
    "\"ipTosCountList\": ["
    "[\"1\", 13], "
    "[\"4\", 99], "
    "[\"17\", 1]], "
    "\"ipTosList\": [\"1\", \"4\", \"17\"], "
    "\"tcpFlagList\": [\"CWR\", \"URG\"], "
    "\"tcpFlagCountList\": [[\"CWR\", 10], [\"URG\", 121]], "
    "\"mobileQciCosList\": [\"conversational\", \"65\"], "
    "\"mobileQciCosCountList\": [[\"conversational\", 11], [\"65\", 122]], "
    "\"durConnectionFailedStatus\": 12, "
    "\"durTunnelFailedStatus\": 13, "
    "\"flowActivatedBy\": \"Remote\", "
    "\"flowActivationTime\": \"Fri, 05 Aug 2016 15:03:43 +0000\", "
    "\"flowDeactivatedBy\": \"Remote\", "
    "\"gtpConnectionStatus\": \"Connected\", "
    "\"gtpTunnelStatus\": \"Not tunneling\", "
    "\"largePacketRtt\": 80, "
    "\"largePacketThreshold\": 600.000000, "
    "\"maxReceiveBitRate\": 1357924680, "
    "\"maxTransmitBitRate\": 235711, "
    "\"numGtpEchoFailures\": 1, "
    "\"numGtpTunnelErrors\": 4, "
    "\"numHttpErrors\": 2"
    "}, "
    "\"ipProtocolType\": \"UDP\", "
    "\"ipVersion\": \"IPv6\", "
    "\"otherEndpointIpAddress\": \"2.3.4.2\", "
    "\"otherEndpointPort\": 2342, "
    "\"reportingEndpointIpAddr\": \"4.2.3.2\", "
    "\"reportingEndpointPort\": 4322, "
    "\"applicationType\": \"Demo application\", "
    "\"appProtocolType\": \"GSM\", "
    "\"appProtocolVersion\": \"1\", "
    "\"cid\": \"65535\", "
    "\"connectionType\": \"S1-U\", "
    "\"ecgi\": \"e65535\", "
    "\"gtpProtocolType\": \"GTP-U\", "
    "\"gtpVersion\": \"1\", "
    "\"httpHeader\": \"http://www.something.com\", "
    "\"imei\": \"209917614823\", "
    "\"imsi\": \"355251/05/850925/8\", "
    "\"lac\": \"1\", "
    "\"mcc\": \"410\", "
    "\"mnc\": \"04\", "
    "\"msisdn\": \"6017123456789\", "
    "\"otherFunctionalRole\": \"MME\", "
    "\"rac\": \"514\", "
    "\"radioAccessTechnology\": \"LTE\", "
    "\"sac\": \"1\", "
    "\"samplingAlgorithm\": 1, "
    "\"tac\": \"2099\", "
    "\"tunnelId\": \"Tunnel 1\", "
    "\"vlanId\": \"15\""
    "}}}";

  size_t json_size = 0;
  char json_body[EVEL_MAX_JSON_BODY];
  MOBILE_GTP_PER_FLOW_METRICS * metrics = NULL;
  EVENT_MOBILE_FLOW * mobile_flow = NULL;

  /***************************************************************************/
  /* Mobile.                                                                 */
  /***************************************************************************/
  evel_set_next_event_sequence(1242);

  metrics = evel_new_mobile_gtp_flow_metrics(132.0001,
                                             31.2,
                                             101,
                                             2101,
                                             501,
                                             1470409422,
                                             988,
                                             1470409432,
                                             12,
                                             (time_t)1470409432,
                                             "Inactive",
                                             88,
                                             4,
                                             18,
                                             123655,
                                             4562,
                                             1,
                                             13,
                                             11,
                                             2,
                                             4,
                                             8,
                                             900,
                                             902,
                                             303,
                                             7,
                                             3,
                                             1,
                                             111,
                                             226);
  assert(metrics != NULL);

  evel_mobile_gtp_metrics_dur_con_fail_set(metrics, 12);
  evel_mobile_gtp_metrics_dur_tun_fail_set(metrics, 13);
  evel_mobile_gtp_metrics_act_by_set(metrics, "Remote");
  evel_mobile_gtp_metrics_act_time_set(metrics, (time_t)1470409423);
  evel_mobile_gtp_metrics_deact_by_set(metrics, "Remote");
  evel_mobile_gtp_metrics_con_status_set(metrics, "Connected");
  evel_mobile_gtp_metrics_tun_status_set(metrics, "Not tunneling");
  evel_mobile_gtp_metrics_iptos_set(metrics, 1, 13);
  evel_mobile_gtp_metrics_iptos_set(metrics, 17, 1);
  evel_mobile_gtp_metrics_iptos_set(metrics, 4, 99);
  evel_mobile_gtp_metrics_large_pkt_rtt_set(metrics, 80);
  evel_mobile_gtp_metrics_large_pkt_thresh_set(metrics, 600.0);
  evel_mobile_gtp_metrics_max_rcv_bit_rate_set(metrics, 1357924680);
  evel_mobile_gtp_metrics_max_trx_bit_rate_set(metrics, 235711);
  evel_mobile_gtp_metrics_num_echo_fail_set(metrics, 1);
  evel_mobile_gtp_metrics_num_tun_fail_set(metrics, 4);
  evel_mobile_gtp_metrics_num_http_errors_set(metrics, 2);
  evel_mobile_gtp_metrics_tcp_flag_count_add(metrics, EVEL_TCP_CWR, 10);
  evel_mobile_gtp_metrics_tcp_flag_count_add(metrics, EVEL_TCP_URG, 121);
  evel_mobile_gtp_metrics_qci_cos_count_add(
                                metrics, EVEL_QCI_COS_UMTS_CONVERSATIONAL, 11);
  evel_mobile_gtp_metrics_qci_cos_count_add(
                                            metrics, EVEL_QCI_COS_LTE_65, 122);

  mobile_flow = evel_new_mobile_flow("Inbound",
                                     metrics,
                                     "UDP",
                                     "IPv6",
                                     "2.3.4.2",
                                     2342,
                                     "4.2.3.2",
                                     4322);
  assert(mobile_flow != NULL);

  evel_mobile_flow_type_set(mobile_flow, "Mobile flow...");
  evel_mobile_flow_app_type_set(mobile_flow, "Demo application");
  evel_mobile_flow_app_prot_type_set(mobile_flow, "GSM");
  evel_mobile_flow_app_prot_ver_set(mobile_flow, "1");
  evel_mobile_flow_cid_set(mobile_flow, "65535");
  evel_mobile_flow_con_type_set(mobile_flow, "S1-U");
  evel_mobile_flow_ecgi_set(mobile_flow, "e65535");
  evel_mobile_flow_gtp_prot_type_set(mobile_flow, "GTP-U");
  evel_mobile_flow_gtp_prot_ver_set(mobile_flow, "1");
  evel_mobile_flow_http_header_set(mobile_flow,
                                   "http://www.something.com");
  evel_mobile_flow_imei_set(mobile_flow, "209917614823");
  evel_mobile_flow_imsi_set(mobile_flow, "355251/05/850925/8");
  evel_mobile_flow_lac_set(mobile_flow, "1");
  evel_mobile_flow_mcc_set(mobile_flow, "410");
  evel_mobile_flow_mnc_set(mobile_flow, "04");
  evel_mobile_flow_msisdn_set(mobile_flow, "6017123456789");
  evel_mobile_flow_other_func_role_set(mobile_flow, "MME");
  evel_mobile_flow_rac_set(mobile_flow, "514");
  evel_mobile_flow_radio_acc_tech_set(mobile_flow, "LTE");
  evel_mobile_flow_sac_set(mobile_flow, "1");
  evel_mobile_flow_samp_alg_set(mobile_flow, 1);
  evel_mobile_flow_tac_set(mobile_flow, "2099");
  evel_mobile_flow_tunnel_id_set(mobile_flow, "Tunnel 1");
  evel_mobile_flow_vlan_id_set(mobile_flow, "15");

  json_size = evel_json_encode_event(
    json_body, EVEL_MAX_JSON_BODY, (EVENT_HEADER *) mobile_flow);
  compare_strings(expected, json_body, EVEL_MAX_JSON_BODY, "Mobile");
  assert((json_size == strlen(json_body)) && "Bad size returned");

  evel_free_event(mobile_flow);
}

void test_encode_report()
{
  char * expected =
    "{\"event\": "
    "{\"commonEventHeader\": {"
    "\"domain\": \"measurementsForVfReporting\", "
    "\"eventId\": \"125\", "
    "\"functionalRole\": \"UNIT TEST\", "
    "\"lastEpochMicrosec\": 1000002, "
    "\"priority\": \"Normal\", "
    "\"reportingEntityName\": \"Dummy VM name - No Metadata available\", "
    "\"sequence\": 125, "
    "\"sourceName\": \"Dummy VM name - No Metadata available\", "
    "\"startEpochMicrosec\": 1000002, "
    "\"version\": 1.2, "
    "\"eventType\": \"Perf reporting...\", "
    "\"reportingEntityId\": \"Dummy VM UUID - No Metadata available\", "
    "\"sourceId\": \"Dummy VM UUID - No Metadata available\""
    "}, "
    "\"measurementsForVfReportingFields\": "
    "{\"measurementInterval\": 1.100000, "
    "\"featureUsageArray\": ["
    "{\"featureIdentifier\": \"FeatureA\", "
    "\"featureUtilization\": 123}, "
    "{\"featureIdentifier\": \"FeatureB\", "
    "\"featureUtilization\": 567}], "
    "\"additionalMeasurements\": ["
    "{\"name\": \"Group1\", "
    "\"measurements\": ["
    "{\"name\": \"Name1\", "
    "\"value\": \"Value1\"}]}, "
    "{\"name\": \"Group2\", "
    "\"measurements\": ["
    "{\"name\": \"Name1\", "
    "\"value\": \"Value1\"}, "
    "{\"name\": \"Name2\", "
    "\"value\": \"Value2\"}]}], "
    "\"measurementFieldsVersion\": 1.1}}}";

  size_t json_size = 0;
  char json_body[EVEL_MAX_JSON_BODY];
  EVENT_REPORT * report = NULL;

  /***************************************************************************/
  /* Report.                                                                 */
  /***************************************************************************/
  evel_set_next_event_sequence(125);
  report = evel_new_report(1.1);
  assert(report != NULL);
  evel_report_type_set(report, "Perf reporting...");
  evel_report_feature_use_add(report, "FeatureA", 123);
  evel_report_feature_use_add(report, "FeatureB", 567);
  evel_report_custom_measurement_add(report, "Group1", "Name1", "Value1");
  evel_report_custom_measurement_add(report, "Group2", "Name1", "Value1");
  evel_report_custom_measurement_add(report, "Group2", "Name2", "Value2");

  json_size = evel_json_encode_event(
    json_body, EVEL_MAX_JSON_BODY, (EVENT_HEADER *) report);
  compare_strings(expected, json_body, EVEL_MAX_JSON_BODY, "Report");
  assert((json_size == strlen(json_body)) && "Bad size returned");

  evel_free_event(report);
}

void test_encode_service()
{
  test_encode_service_subset(SERVICE_NONE);
  test_encode_service_subset(SERVICE_CODEC);
  test_encode_service_subset(SERVICE_TRANSCODING);
  test_encode_service_subset(SERVICE_RTCP);
  test_encode_service_subset(SERVICE_EOC_VQM);
  test_encode_service_subset(SERVICE_MARKER);
}

void test_encode_service_subset(const SERVICE_TEST service_test)
{
  char * expected_start =
    "{\"event\": "
    "{\"commonEventHeader\": {"
    "\"domain\": \"serviceEvents\", "
    "\"eventId\": \"2000\", "
    "\"functionalRole\": \"UNIT TEST\", "
    "\"lastEpochMicrosec\": 1000002, "
    "\"priority\": \"Normal\", "
    "\"reportingEntityName\": \"Dummy VM name - No Metadata available\", "
    "\"sequence\": 2000, "
    "\"sourceName\": \"Dummy VM name - No Metadata available\", "
    "\"startEpochMicrosec\": 1000002, "
    "\"version\": 1.2, "
    "\"eventType\": \"Service Event\", "
    "\"reportingEntityId\": \"Dummy VM UUID - No Metadata available\", "
    "\"sourceId\": \"Dummy VM UUID - No Metadata available\""
    "}, "
    "\"serviceEventsFields\": {"
    "\"eventInstanceIdentifier\": "
    "{"
    "\"vendorId\": \"vendor_x_id\", "
    "\"eventId\": \"vendor_x_event_id\", "
    "\"productId\": \"vendor_x_product_id\", "
    "\"subsystemId\": \"vendor_x_subsystem_id\", "
    "\"eventFriendlyName\": \"vendor_x_frieldly_name\""
    "}, "
    "\"serviceEventsFieldsVersion\": 1.1, "
    "\"correlator\": \"vendor_x_correlator\", "
    "\"additionalFields\": ["
    "{\"name\": \"Name1\", \"value\": \"Value1\"}, "
    "{\"name\": \"Name2\", \"value\": \"Value2\"}, "
    "{\"name\": \"Name3\", \"value\": \"Value3\"}, "
    "{\"name\": \"Name4\", \"value\": \"Value4\"}]";
  char * expected_codec =
    ", "
    "\"codecSelected\": {"
    "\"codec\": \"PCMA\""
    "}";
  char * expected_transcoding =
    ", "
    "\"codecSelectedTranscoding\": {"
    "\"calleeSideCodec\": \"PCMA\", "
    "\"callerSideCodec\": \"G729A\""
    "}";
  char * expected_rtcp =
    ", "
    "\"midCallRtcp\": {"
    "\"rtcpData\": \"some_rtcp_data\""
    "}";
  char * expected_eoc_vqm =
    ", "
    "\"endOfCallVqmSummaries\": {"
    "\"adjacencyName\": \"vendor_x_adjacency\", "
    "\"endpointDescription\": \"Caller\", "
    "\"endpointJitter\": 66, "
    "\"endpointRtpOctetsDiscarded\": 100, "
    "\"endpointRtpOctetsReceived\": 200, "
    "\"endpointRtpOctetsSent\": 300, "
    "\"endpointRtpPacketsDiscarded\": 400, "
    "\"endpointRtpPacketsReceived\": 500, "
    "\"endpointRtpPacketsSent\": 600, "
    "\"localJitter\": 99, "
    "\"localRtpOctetsDiscarded\": 150, "
    "\"localRtpOctetsReceived\": 250, "
    "\"localRtpOctetsSent\": 350, "
    "\"localRtpPacketsDiscarded\": 450, "
    "\"localRtpPacketsReceived\": 550, "
    "\"localRtpPacketsSent\": 650, "
    "\"mosCqe\": 12.255000, "
    "\"packetsLost\": 157, "
    "\"packetLossPercent\": 0.232000, "
    "\"rFactor\": 11, "
    "\"roundTripDelay\": 15"
    "}";
  char * expected_marker =
    ", "
    "\"marker\": {"
    "\"phoneNumber\": \"0888888888\""
    "}";
  char * expected_end =
    "}}}";

  char * expected_middle = NULL;
  switch (service_test)
  {
    case SERVICE_NONE:
      expected_middle = "";
      break;
    case SERVICE_CODEC:
      expected_middle = expected_codec;
      break;
    case SERVICE_TRANSCODING:
      expected_middle = expected_transcoding;
      break;
    case SERVICE_RTCP:
      expected_middle = expected_rtcp;
      break;
    case SERVICE_EOC_VQM:
      expected_middle = expected_eoc_vqm;
      break;
    case SERVICE_MARKER:
      expected_middle = expected_marker;
      break;
  }
  assert(expected_middle != NULL);

  int offset = 0;
  char expected[EVEL_MAX_JSON_BODY];
  offset = snprintf(expected + offset,
                    EVEL_MAX_JSON_BODY - offset,
                    "%s%s%s",
                    expected_start,
                    expected_middle,
                    expected_end);

  size_t json_size = 0;
  char json_body[EVEL_MAX_JSON_BODY];
  EVENT_SGNALING * event = NULL;
  evel_set_next_event_sequence(2000);
  event = evel_new_signaling("vendor_x_id",
           "correlator", "1.0.3.1", "1234", "192.168.1.3","3456");
  assert(event != NULL);
  evel_signaling_type_set(event, "Signaling");
  evel_signaling_correlator_set(event, "vendor_x_correlator");
  evel_signaling_vnfmodule_name_set(event, "vendor_x_module");
  evel_signaling_vnfname_set(event, "vendor_x_vnf");

  switch (service_test)
  {
    case SERVICE_NONE:
      break;
    case SERVICE_CODEC:
      evel_signaling_addl_info_add(event, "codec", "PCMA");
      break;
    case SERVICE_TRANSCODING:
      evel_signaling_addl_info_add(event, "calleecodec", "PCMA");
      evel_signaling_addl_info_add(event, "callercodec", "G729A");
      break;
    case SERVICE_RTCP:
      evel_signaling_addl_info_add(event, "rtcpdata", "abcdefgh");
      break;
    case SERVICE_EOC_VQM:
      evel_signaling_addl_info_add(event, "adjacency", "vendor_x");
      /*evel_service_adjacency_name_set(event, "vendor_x_adjacency");
      evel_service_endpoint_desc_set(event, EVEL_SERVICE_ENDPOINT_CALLER);
      evel_service_endpoint_jitter_set(event, 66);
      evel_service_endpoint_rtp_oct_disc_set(event, 100);
      evel_service_endpoint_rtp_oct_recv_set(event, 200);
      evel_service_endpoint_rtp_oct_sent_set(event, 300);
      evel_service_endpoint_rtp_pkt_disc_set(event, 400);
      evel_service_endpoint_rtp_pkt_recv_set(event, 500);
      evel_service_endpoint_rtp_pkt_sent_set(event, 600);
      evel_service_local_jitter_set(event, 99);
      evel_service_local_rtp_oct_disc_set(event, 150);
      evel_service_local_rtp_oct_recv_set(event, 250);
      evel_service_local_rtp_oct_sent_set(event, 350);
      evel_service_local_rtp_pkt_disc_set(event, 450);
      evel_service_local_rtp_pkt_recv_set(event, 550);
      evel_service_local_rtp_pkt_sent_set(event, 650);
      evel_service_mos_cqe_set(event, 12.255);
      evel_service_packets_lost_set(event, 157);
      evel_service_packet_loss_percent_set(event, 0.232);
      evel_service_r_factor_set(event, 11);
      evel_service_round_trip_delay_set(event, 15);*/
      break;
    case SERVICE_MARKER:
      evel_signaling_addl_info_add(event, "service_phone", "0888888888");
      break;
  }

  json_size = evel_json_encode_event(
    json_body, EVEL_MAX_JSON_BODY, (EVENT_HEADER *) event);
  compare_strings(expected, json_body, EVEL_MAX_JSON_BODY, "Service");
  assert((json_size == strlen(json_body)) && "Bad size returned");

  evel_free_event(event);
}

void test_encode_signaling()
{
  char * expected =
    "{\"event\": "
    "{\"commonEventHeader\": {"
    "\"domain\": \"signaling\", "
    "\"eventId\": \"2001\", "
    "\"functionalRole\": \"UNIT TEST\", "
    "\"lastEpochMicrosec\": 1000002, "
    "\"priority\": \"Normal\", "
    "\"reportingEntityName\": \"Dummy VM name - No Metadata available\", "
    "\"sequence\": 2001, "
    "\"sourceName\": \"Dummy VM name - No Metadata available\", "
    "\"startEpochMicrosec\": 1000002, "
    "\"version\": 1.2, "
    "\"eventType\": \"Signaling\", "
    "\"reportingEntityId\": \"Dummy VM UUID - No Metadata available\", "
    "\"sourceId\": \"Dummy VM UUID - No Metadata available\""
    "}, "
    "\"signalingFields\": {"
    "\"eventInstanceIdentifier\": "
    "{"
    "\"vendorId\": \"vendor_x_id\", "
    "\"eventId\": \"vendor_x_event_id\", "
    "\"productId\": \"vendor_x_product_id\", "
    "\"subsystemId\": \"vendor_x_subsystem_id\", "
    "\"eventFriendlyName\": \"vendor_x_frieldly_name\""
    "}, "
    "\"signalingFieldsVersion\": 1.1, "
    "\"correlator\": \"vendor_x_correlator\", "
    "\"localIpAddress\": \"1.0.3.1\", "
    "\"localPort\": \"1031\", "
    "\"remoteIpAddress\": \"5.3.3.0\", "
    "\"remotePort\": \"5330\", "
    "\"compressedSip\": \"compressed_sip\", "
    "\"summarySip\": \"summary_sip\""
    "}}}";

  size_t json_size = 0;
  char json_body[EVEL_MAX_JSON_BODY];
  EVENT_SIGNALING * event = NULL;
  evel_set_next_event_sequence(2001);
  event = evel_new_signaling("vendor_x_id",
           "correlator", "1.0.3.1", "1234", "192.168.1.3","3456");
  assert(event != NULL);
  evel_signaling_vnfmodule_name_set(event, "vendor_x_module");
  evel_signaling_vnfname_set(event, "vendor_x_vnf");
  evel_signaling_type_set(event, "Signaling");
  evel_signaling_product_id_set(event, "vendor_x_product_id");
  evel_signaling_subsystem_id_set(event, "vendor_x_subsystem_id");
  evel_signaling_friendly_name_set(event, "vendor_x_frieldly_name");
  evel_signaling_correlator_set(event, "vendor_x_correlator");
  evel_signaling_local_ip_address_set(event, "1.0.3.1");
  evel_signaling_local_port_set(event, "1031");
  evel_signaling_remote_ip_address_set(event, "5.3.3.0");
  evel_signaling_remote_port_set(event, "5330");
  evel_signaling_compressed_sip_set(event, "compressed_sip");
  evel_signaling_summary_sip_set(event, "summary_sip");
  json_size = evel_json_encode_event(
    json_body, EVEL_MAX_JSON_BODY, (EVENT_HEADER *) event);
  compare_strings(expected, json_body, EVEL_MAX_JSON_BODY, "Signaling");
  assert((json_size == strlen(json_body)) && "Bad size returned");

  evel_free_event(event);
}

void test_encode_state_change()
{
  char * expected =
    "{\"event\": "
    "{\"commonEventHeader\": {"
    "\"domain\": \"stateChange\", "
    "\"eventId\": \"128\", "
    "\"functionalRole\": \"UNIT TEST\", "
    "\"lastEpochMicrosec\": 1000002, "
    "\"priority\": \"Normal\", "
    "\"reportingEntityName\": \"Dummy VM name - No Metadata available\", "
    "\"sequence\": 128, "
    "\"sourceName\": \"Dummy VM name - No Metadata available\", "
    "\"startEpochMicrosec\": 1000002, "
    "\"version\": 1.2, "
    "\"eventType\": \"SC Type\", "
    "\"reportingEntityId\": \"Dummy VM UUID - No Metadata available\", "
    "\"sourceId\": \"Dummy VM UUID - No Metadata available\""
    "}, "
    "\"stateChangeFields\": {"
    "\"newState\": \"inService\", "
    "\"oldState\": \"outOfService\", "
    "\"stateInterface\": \"An Interface\", "
    "\"additionalFields\": ["
    "{\"name\": \"Name1\", "
    "\"value\": \"Value1\"}, "
    "{\"name\": \"Name2\", "
    "\"value\": \"Value2\"}"
    "], "
    "\"stateChangeFieldsVersion\": 1.1"
    "}}}";

  size_t json_size = 0;
  char json_body[EVEL_MAX_JSON_BODY];
  EVENT_STATE_CHANGE * state_change = NULL;
  evel_set_next_event_sequence(128);
  state_change = evel_new_state_change(EVEL_ENTITY_STATE_IN_SERVICE,
                                       EVEL_ENTITY_STATE_OUT_OF_SERVICE,
                                       "An Interface");
  assert(state_change != NULL);
  evel_state_change_type_set(state_change, "SC Type");
  evel_state_change_addl_field_add(state_change, "Name1", "Value1");
  evel_state_change_addl_field_add(state_change, "Name2", "Value2");

  json_size = evel_json_encode_event(
    json_body, EVEL_MAX_JSON_BODY, (EVENT_HEADER *) state_change);
  compare_strings(expected, json_body, EVEL_MAX_JSON_BODY, "StateChange");
  assert((json_size == strlen(json_body)) && "Bad size returned");

  evel_free_event(state_change);
}

void test_encode_syslog()
{
  char * expected =
    "{\"event\": "
    "{\"commonEventHeader\": {"
    "\"domain\": \"syslog\", "
    "\"eventId\": \"126\", "
    "\"functionalRole\": \"UNIT TEST\", "
    "\"lastEpochMicrosec\": 1000002, "
    "\"priority\": \"Normal\", "
    "\"reportingEntityName\": \"Dummy VM name - No Metadata available\", "
    "\"sequence\": 126, "
    "\"sourceName\": \"Dummy VM name - No Metadata available\", "
    "\"startEpochMicrosec\": 1000002, "
    "\"version\": 1.2, "
    "\"eventType\": \"SL Type\", "
    "\"reportingEntityId\": \"Dummy VM UUID - No Metadata available\", "
    "\"sourceId\": \"Dummy VM UUID - No Metadata available\""
    "}, "
    "\"syslogFields\": {"
    "\"eventSourceType\": \"virtualNetworkFunction\", "
    "\"syslogMsg\": \"SL Message\", "
    "\"syslogTag\": \"SL Tag\", "
    "\"syslogFieldsVersion\": 1.1, "
    "\"eventSourceHost\": \"SL Host\", "
    "\"syslogFacility\": 6, "
    "\"syslogProc\": \"SL Proc\", "
    "\"syslogProcId\": 2, "
    "\"syslogSData\": \"SL SDATA\", "
    "\"syslogVer\": 1"
    "}}}";
  size_t json_size = 0;
  char json_body[EVEL_MAX_JSON_BODY];
  EVENT_SYSLOG * syslog = NULL;
  evel_set_next_event_sequence(126);
  syslog = evel_new_syslog(EVEL_SOURCE_VIRTUAL_NETWORK_FUNCTION,
                           "SL Message",
                           "SL Tag");
  assert(syslog != NULL);
  evel_syslog_type_set(syslog, "SL Type");
  evel_syslog_event_source_host_set(syslog, "SL Host");
  evel_syslog_facility_set(syslog, EVEL_SYSLOG_FACILITY_LINE_PRINTER);
  evel_syslog_proc_set(syslog, "SL Proc");
  evel_syslog_proc_id_set(syslog, 2);
  evel_syslog_version_set(syslog, 1);
  evel_syslog_s_data_set(syslog, "SL SDATA");

  json_size = evel_json_encode_event(
    json_body, EVEL_MAX_JSON_BODY, (EVENT_HEADER *) syslog);
  compare_strings(expected, json_body, EVEL_MAX_JSON_BODY, "Syslog");
  assert((json_size == strlen(json_body)) && "Bad size returned");

  evel_free_event(syslog);
}

void test_encode_other()
{
  char * expected =
    "{\"event\": "
    "{\"commonEventHeader\": {"
    "\"domain\": \"other\", "
    "\"eventId\": \"129\", "
    "\"functionalRole\": \"UNIT TEST\", "
    "\"lastEpochMicrosec\": 1000002, "
    "\"priority\": \"Normal\", "
    "\"reportingEntityName\": \"Dummy VM name - No Metadata available\", "
    "\"sequence\": 129, "
    "\"sourceName\": \"Dummy VM name - No Metadata available\", "
    "\"startEpochMicrosec\": 1000002, "
    "\"version\": 1.2, "
    "\"eventType\": \"Other Type\", "
    "\"reportingEntityId\": \"Dummy VM UUID - No Metadata available\", "
    "\"sourceId\": \"Dummy VM UUID - No Metadata available\""
    "}, "
    "\"otherFields\": ["
    "{\"name\": \"Other field 1\", "
    "\"value\": \"Other value 1\"}, "
    "{\"name\": \"Other field 2\", "
    "\"value\": \"Other value 2\"}"
    "]"
    "}}";

  size_t json_size = 0;
  char json_body[EVEL_MAX_JSON_BODY];
  EVENT_OTHER * other = NULL;
  evel_set_next_event_sequence(129);
  other = evel_new_other();
  assert(other != NULL);
  evel_other_type_set(other, "Other Type");
  evel_other_field_add(other,
                       "Other field 1",
                       "Other value 1");
  evel_other_field_add(other,
                       "Other field 2",
                       "Other value 2");

  json_size = evel_json_encode_event(
    json_body, EVEL_MAX_JSON_BODY, (EVENT_HEADER *) other);
  compare_strings(expected, json_body, EVEL_MAX_JSON_BODY, "Other");
  assert((json_size == strlen(json_body)) && "Bad size returned");

  evel_free_event(other);
}

void compare_strings(char * expected,
                     char * actual,
                     int max_size,
                     char * description)
{
  if (strncmp(expected, actual, max_size) != 0)
  {
    int diff = 0;
    while (diff < max_size)
    {
      if (expected[diff] != actual[diff])
      {
        break;
      }
      diff++;
    }

    printf("Comparison Failure at Offset %d\n\n", diff);
    printf("Expected:\n%s\n", expected);
    printf("Actual:\n%s\n", actual);
    printf("Description: %s\n", description);
    assert(0);
  }
}

/**************************************************************************//**
 * Copy a json string to a ::MEMORY_CHUNK for testing.
 *
 * @param chunk         The memory chunk.
 * @param string        The json string.
 *****************************************************************************/
void copy_string_to_chunk(MEMORY_CHUNK * chunk, char * string)
{
  int mem_size;

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(chunk != NULL);
  assert(string != NULL);

  mem_size = strlen(string) + 1;
  chunk->memory = malloc(mem_size);
  memcpy(chunk->memory, string, mem_size);
  chunk->size = mem_size;
}

/**************************************************************************//**
 * Copy a json string to a ::MEMORY_CHUNK for testing.
 *
 * @param json          The JSON string.
 * @param post          Memory chunk to post a response.
 *****************************************************************************/
void handle_json_response(char * json, MEMORY_CHUNK * post)
{
  MEMORY_CHUNK chunk;
  post->memory = NULL;
  post->size = 0;
  copy_string_to_chunk(&chunk, json);
  evel_handle_event_response(&chunk, post);
  free(chunk.memory);
}

/**************************************************************************//**
 * Test that a non-"commandList" JSON buffer leaves the throttle state off.
 *****************************************************************************/
void test_json_response_junk()
{
  MEMORY_CHUNK post;
  int domain;
  char * json_junk =
    "{"
    "\"junk1\": ["
    "\"1\", \"2\", \"3\"], "
    "\"junk2\": ["
    "\"1\", \"2\", \"3\"]"
    "}";

  evel_throttle_initialize();
  handle_json_response(json_junk, &post);

  /***************************************************************************/
  /* Check that all domains are not throttled.                               */
  /***************************************************************************/
  for (domain = EVEL_DOMAIN_FAULT; domain < EVEL_MAX_DOMAINS; domain++)
  {
    assert(evel_get_throttle_spec(domain) == NULL);
  }

  /***************************************************************************/
  /* Check that we generated no post.                                        */
  /***************************************************************************/
  assert(post.memory == NULL);

  evel_throttle_terminate();
}

char * json_command_list_provide =
  "{"
  "\"commandList\": ["
  "{"
  "\"command\": {"
  "\"commandType\": \"provideThrottlingState\""
  "}"
  "}"
  "]"
  "}";

char * json_command_list_fault_clear =
  "{"
  "\"commandList\": ["
  "{"
  "\"command\": {"
  "\"commandType\": \"throttlingSpecification\", "
  "\"eventDomainThrottleSpecification\": {"
  "\"eventDomain\": \"fault\""
  "}"
  "}"
  "}"
  "]"
  "}";

char * json_command_list_syslog_clear =
  "{"
  "\"commandList\": ["
  "{"
  "\"command\": {"
  "\"commandType\": \"throttlingSpecification\", "
  "\"eventDomainThrottleSpecification\": {"
  "\"eventDomain\": \"syslog\""
  "}"
  "}"
  "}"
  "]"
  "}";

char * expected_throttle_state_normal =
  "{"
  "\"eventThrottlingState\": {"
  "\"eventThrottlingMode\": \"normal\"}"
  "}";

/**************************************************************************//**
 * Test that we can return the default throttling state.
 *****************************************************************************/
void test_json_provide_throttle_state()
{
  MEMORY_CHUNK post;
  int domain;

  char * expected_post = expected_throttle_state_normal;

  evel_throttle_initialize();
  handle_json_response(json_command_list_provide, &post);

  /***************************************************************************/
  /* Check that all domains are not throttled.                               */
  /***************************************************************************/
  for (domain = EVEL_DOMAIN_FAULT; domain < EVEL_MAX_DOMAINS; domain++)
  {
    assert(evel_get_throttle_spec(domain) == NULL);
  }

  /***************************************************************************/
  /* Check that we generated a throttling specification post.                */
  /***************************************************************************/
  assert(post.memory != NULL);
  compare_strings(expected_post, post.memory, strlen(expected_post),
                  "Throttle State Normal");
  free(post.memory);

  evel_throttle_terminate();
}

/**************************************************************************//**
 * Test the measurement interval handling and API.
 *****************************************************************************/
void test_json_measurement_interval()
{
  MEMORY_CHUNK post;
  char * json_command_list_interval_only =
    "{"
    "\"commandList\": ["
    "{"
    "\"command\": {"
    "\"measurementInterval\": 60"
    "}"
    "}"
    "]"
    "}";

  char * json_command_list_interval_first =
    "{"
    "\"commandList\": ["
    "{"
    "\"command\": {"
    "\"measurementInterval\": 30, "
    "\"commandType\": \"measurementIntervalChange\""
    "}"
    "}"
    "]"
    "}";

  char * json_command_list_command_first =
    "{"
    "\"commandList\": ["
    "{"
    "\"command\": {"
    "\"commandType\": \"measurementIntervalChange\", "
    "\"measurementInterval\": 60"
    "}"
    "}"
    "]"
    "}";

  evel_throttle_initialize();
  assert(evel_get_measurement_interval() == EVEL_MEASUREMENT_INTERVAL_UKNOWN);

  /***************************************************************************/
  /* Check that we're not handling stuff when we shouldn't.                  */
  /***************************************************************************/
  handle_json_response(json_command_list_interval_only, &post);
  assert(post.memory == NULL);
  assert(evel_get_measurement_interval() == EVEL_MEASUREMENT_INTERVAL_UKNOWN);

  /***************************************************************************/
  /* Check that we're OK with the interval coming first.                     */
  /***************************************************************************/
  handle_json_response(json_command_list_interval_first, &post);
  assert(post.memory == NULL);
  assert(evel_get_measurement_interval() == 30);

  /***************************************************************************/
  /* Check that we're OK with the command type coming first.                 */
  /***************************************************************************/
  handle_json_response(json_command_list_command_first, &post);
  assert(post.memory == NULL);
  assert(evel_get_measurement_interval() == 60);

  evel_throttle_terminate();
}

/**************************************************************************//**
 * Test a single domain, single field suppression.
 *****************************************************************************/
void test_json_throttle_spec_field()
{
  MEMORY_CHUNK post;
  int domain;

  char * json_command_list_fault_single =
    "{"
    "\"commandList\": ["
    "{"
    "\"command\": {"
    "\"commandType\": \"throttlingSpecification\", "
    "\"eventDomainThrottleSpecification\": {"
    "\"eventDomain\": \"fault\", "
    "\"suppressedFieldNames\": [\"alarmInterfaceA\"]"
    "}"
    "}"
    "}"
    "]"
    "}";

  char * json_command_list_fault_double =
    "{"
    "\"commandList\": ["
    "{"
    "\"command\": {"
    "\"commandType\": \"throttlingSpecification\", "
    "\"eventDomainThrottleSpecification\": {"
    "\"eventDomain\": \"fault\", "
    "\"suppressedFieldNames\": ["
    "\"alarmInterfaceA\", \"alarmAdditionalInformation\"]"
    "}"
    "}"
    "}"
    "]"
    "}";

  char * expected_post_fault_single =
    "{"
    "\"eventThrottlingState\": {"
    "\"eventThrottlingMode\": \"throttled\", "
    "\"eventDomainThrottleSpecificationList\": ["
    "{"
    "\"eventDomain\": \"fault\", "
    "\"suppressedFieldNames\": [\"alarmInterfaceA\"]"
    "}"
    "]"
    "}"
    "}";

  char * expected_post_fault_double =
    "{"
    "\"eventThrottlingState\": {"
    "\"eventThrottlingMode\": \"throttled\", "
    "\"eventDomainThrottleSpecificationList\": ["
    "{"
    "\"eventDomain\": \"fault\", "
    "\"suppressedFieldNames\": ["
    "\"alarmInterfaceA\", \"alarmAdditionalInformation\"]"
    "}"
    "]"
    "}"
    "}";

  /***************************************************************************/
  /* Initialize and provide a specification with a single fault suppressed.  */
  /***************************************************************************/
  evel_throttle_initialize();
  handle_json_response(json_command_list_fault_single, &post);

  /***************************************************************************/
  /* Check that the FAULT domain is throttled.                               */
  /***************************************************************************/
  assert(evel_get_throttle_spec(EVEL_DOMAIN_FAULT) != NULL);
  for (domain = EVEL_DOMAIN_MEASUREMENT; domain < EVEL_MAX_DOMAINS; domain++)
  {
    if (domain != EVEL_DOMAIN_FAULT)
    {
      assert(evel_get_throttle_spec(domain) == NULL);
    }
  }
  assert(post.memory == NULL);

  /***************************************************************************/
  /* Request and verify the throttling state.                                */
  /***************************************************************************/
  handle_json_response(json_command_list_provide, &post);
  assert(post.memory != NULL);
  compare_strings(expected_post_fault_single,
                  post.memory,
                  strlen(expected_post_fault_single),
                  "Fault - Single Field");
  free(post.memory);
  post.memory = NULL;

  /***************************************************************************/
  /* Update a specification with two faults suppressed.                      */
  /***************************************************************************/
  handle_json_response(json_command_list_fault_double, &post);

  /***************************************************************************/
  /* Check that the FAULT domain is throttled.                               */
  /***************************************************************************/
  assert(evel_get_throttle_spec(EVEL_DOMAIN_FAULT) != NULL);
  for (domain = EVEL_DOMAIN_MEASUREMENT; domain < EVEL_MAX_DOMAINS; domain++)
  {
    if (domain != EVEL_DOMAIN_FAULT)
    {
      assert(evel_get_throttle_spec(domain) == NULL);
    }
  }
  assert(post.memory == NULL);

  /***************************************************************************/
  /* Request and verify the throttling state.                                */
  /***************************************************************************/
  handle_json_response(json_command_list_provide, &post);
  assert(post.memory != NULL);
  compare_strings(expected_post_fault_double,
                  post.memory,
                  strlen(expected_post_fault_double),
                  "Fault - Double Field");
  free(post.memory);
  post.memory = NULL;

  /***************************************************************************/
  /* Now clear the FAULT domain.                                             */
  /***************************************************************************/
  handle_json_response(json_command_list_fault_clear, &post);
  for (domain = EVEL_DOMAIN_FAULT; domain < EVEL_MAX_DOMAINS; domain++)
  {
    assert(evel_get_throttle_spec(domain) == NULL);
  }

  evel_throttle_terminate();
}

/**************************************************************************//**
 * Test a single domain, nv_pair suppression.
 *****************************************************************************/
void test_json_throttle_spec_nv_pair()
{
  MEMORY_CHUNK post;
  int domain;

  char * json_command_list_fault_pair_single =
    "{"
    "\"commandList\": ["
    "{"
    "\"command\": {"
    "\"commandType\": \"throttlingSpecification\", "
    "\"eventDomainThrottleSpecification\": {"
    "\"eventDomain\": \"fault\", "
    "\"suppressedNvPairsList\": ["
    "{"
    "\"nvPairFieldName\": \"alarmAdditionalInformation\", "
    "\"suppressedNvPairNames\": [\"name1\"]"
    "}"
    "]"
    "}"
    "}"
    "}"
    "]"
    "}";

  char * json_command_list_fault_pair_double =
    "{"
    "\"commandList\": ["
    "{"
    "\"command\": {"
    "\"commandType\": \"throttlingSpecification\", "
    "\"eventDomainThrottleSpecification\": {"
    "\"eventDomain\": \"fault\", "
    "\"suppressedNvPairsList\": ["
    "{"
    "\"nvPairFieldName\": \"alarmAdditionalInformation\", "
    "\"suppressedNvPairNames\": [\"name1\", \"name2\"]"
    "}"
    "]"
    "}"
    "}"
    "}"
    "]"
    "}";

  char * expected_post_fault_pair_single =
    "{"
    "\"eventThrottlingState\": {"
    "\"eventThrottlingMode\": \"throttled\", "
    "\"eventDomainThrottleSpecificationList\": ["
    "{"
    "\"eventDomain\": \"fault\", "
    "\"suppressedNvPairsList\": ["
    "{"
    "\"nvPairFieldName\": \"alarmAdditionalInformation\", "
    "\"suppressedNvPairNames\": [\"name1\"]"
    "}"
    "]"
    "}"
    "]"
    "}"
    "}";

  char * expected_post_fault_pair_double =
    "{"
    "\"eventThrottlingState\": {"
    "\"eventThrottlingMode\": \"throttled\", "
    "\"eventDomainThrottleSpecificationList\": ["
    "{"
    "\"eventDomain\": \"fault\", "
    "\"suppressedNvPairsList\": ["
    "{"
    "\"nvPairFieldName\": \"alarmAdditionalInformation\", "
    "\"suppressedNvPairNames\": [\"name1\", \"name2\"]"
    "}"
    "]"
    "}"
    "]"
    "}"
    "}";

  /***************************************************************************/
  /* Initialize and provide a specification with a single nvpair with a      */
  /* single sub-field suppressed.                                            */
  /***************************************************************************/
  evel_throttle_initialize();
  handle_json_response(json_command_list_fault_pair_single, &post);

  /***************************************************************************/
  /* Check that the FAULT domain is throttled.                               */
  /***************************************************************************/
  assert(evel_get_throttle_spec(EVEL_DOMAIN_FAULT) != NULL);
  for (domain = EVEL_DOMAIN_MEASUREMENT; domain < EVEL_MAX_DOMAINS; domain++)
  {
    if (domain != EVEL_DOMAIN_FAULT)
    {
      assert(evel_get_throttle_spec(domain) == NULL);
    }
  }
  assert(post.memory == NULL);

  /***************************************************************************/
  /* Request and verify the throttling state.                                */
  /***************************************************************************/
  handle_json_response(json_command_list_provide, &post);
  assert(post.memory != NULL);
  compare_strings(expected_post_fault_pair_single,
                  post.memory,
                  strlen(expected_post_fault_pair_single),
                  "Fault - Single Pair, Single Field");
  free(post.memory);
  post.memory = NULL;

  /***************************************************************************/
  /* Update a specification with a single nvpair with two sub-fields         */
  /* suppressed.                                                             */
  /***************************************************************************/
  handle_json_response(json_command_list_fault_pair_double, &post);

  /***************************************************************************/
  /* Check that the FAULT domain is throttled.                               */
  /***************************************************************************/
  assert(evel_get_throttle_spec(EVEL_DOMAIN_FAULT) != NULL);
  for (domain = EVEL_DOMAIN_MEASUREMENT; domain < EVEL_MAX_DOMAINS; domain++)
  {
    if (domain != EVEL_DOMAIN_FAULT)
    {
      assert(evel_get_throttle_spec(domain) == NULL);
    }
  }
  assert(post.memory == NULL);

  /***************************************************************************/
  /* Request and verify the throttling state.                                */
  /***************************************************************************/
  handle_json_response(json_command_list_provide, &post);
  assert(post.memory != NULL);
  compare_strings(expected_post_fault_pair_double,
                  post.memory,
                  strlen(expected_post_fault_pair_double),
                  "Fault - Double Field");
  free(post.memory);
  post.memory = NULL;

  /***************************************************************************/
  /* Now clear the FAULT domain.                                             */
  /***************************************************************************/
  handle_json_response(json_command_list_fault_clear, &post);
  for (domain = EVEL_DOMAIN_FAULT; domain < EVEL_MAX_DOMAINS; domain++)
  {
    assert(evel_get_throttle_spec(domain) == NULL);
  }

  evel_throttle_terminate();
}

/**************************************************************************//**
 * Test two domains, nv_pair suppression.
 *****************************************************************************/
void test_json_throttle_spec_two_domains()
{
  MEMORY_CHUNK post;
  int domain;

  char * json_command_list_two_domains =
    "{"
    "\"commandList\": ["
    "{"
    "\"command\": {"
    "\"commandType\": \"throttlingSpecification\", "
    "\"eventDomainThrottleSpecification\": {"
    "\"eventDomain\": \"fault\", "
    "\"suppressedFieldNames\": [\"alarmInterfaceA\"], "
    "\"suppressedNvPairsList\": ["
    "{"
    "\"nvPairFieldName\": \"alarmAdditionalInformation\", "
    "\"suppressedNvPairNames\": [\"name1\"]"
    "}]}}}, "
    "{"
    "\"command\": {"
    "\"commandType\": \"throttlingSpecification\", "
    "\"eventDomainThrottleSpecification\": {"
    "\"eventDomain\": \"syslog\", "
    "\"suppressedFieldNames\": [\"syslogProcId\"], "
    "\"suppressedNvPairsList\": ["
    "{"
    "\"nvPairFieldName\": \"additionalFields\", "
    "\"suppressedNvPairNames\": [\"name1\"]"
    "}]}}}"
    "]"
    "}";

  char * expected_post_two_domains =
    "{"
    "\"eventThrottlingState\": {"
    "\"eventThrottlingMode\": \"throttled\", "
    "\"eventDomainThrottleSpecificationList\": ["
    "{"
    "\"eventDomain\": \"fault\", "
    "\"suppressedFieldNames\": [\"alarmInterfaceA\"], "
    "\"suppressedNvPairsList\": ["
    "{"
    "\"nvPairFieldName\": \"alarmAdditionalInformation\", "
    "\"suppressedNvPairNames\": [\"name1\"]"
    "}]}, "
    "{"
    "\"eventDomain\": \"syslog\", "
    "\"suppressedFieldNames\": [\"syslogProcId\"], "
    "\"suppressedNvPairsList\": ["
    "{"
    "\"nvPairFieldName\": \"additionalFields\", "
    "\"suppressedNvPairNames\": [\"name1\"]"
    "}]}"
    "]"
    "}"
    "}";

  /***************************************************************************/
  /* Initialize and provide a specification with a single nvpair with a      */
  /* single sub-field suppressed.                                            */
  /***************************************************************************/
  evel_throttle_initialize();
  handle_json_response(json_command_list_two_domains, &post);

  /***************************************************************************/
  /* Check that the FAULT and SYSLOG domains are throttled.                  */
  /***************************************************************************/
  assert(evel_get_throttle_spec(EVEL_DOMAIN_FAULT) != NULL);
  assert(evel_get_throttle_spec(EVEL_DOMAIN_SYSLOG) != NULL);
  for (domain = EVEL_DOMAIN_MEASUREMENT; domain < EVEL_MAX_DOMAINS; domain++)
  {
    if ((domain != EVEL_DOMAIN_FAULT) && (domain != EVEL_DOMAIN_SYSLOG))
    {
      assert(evel_get_throttle_spec(domain) == NULL);
    }
  }
  assert(post.memory == NULL);

  /***************************************************************************/
  /* Request and verify the throttling state.                                */
  /***************************************************************************/
  handle_json_response(json_command_list_provide, &post);
  assert(post.memory != NULL);
  compare_strings(expected_post_two_domains,
                  post.memory,
                  strlen(expected_post_two_domains),
                  "Fault - Two Domains");
  free(post.memory);
  post.memory = NULL;

  /***************************************************************************/
  /* Now clear the FAULT and SYSLOG domains.                                 */
  /***************************************************************************/
  handle_json_response(json_command_list_fault_clear, &post);
  assert(evel_get_throttle_spec(EVEL_DOMAIN_FAULT) == NULL);
  assert(evel_get_throttle_spec(EVEL_DOMAIN_SYSLOG) != NULL);
  handle_json_response(json_command_list_syslog_clear, &post);
  for (domain = EVEL_DOMAIN_FAULT; domain < EVEL_MAX_DOMAINS; domain++)
  {
    assert(evel_get_throttle_spec(domain) == NULL);
  }

  evel_throttle_terminate();
}

/**************************************************************************//**
 * Test bad command type.
 *****************************************************************************/
void test_json_throttle_spec_bad_command_type()
{
  MEMORY_CHUNK post;
  int domain;

  /***************************************************************************/
  /* Search for "dodgy" in the JSON, and you will see the dodgy bits we're   */
  /* handling in these tests.                                                */
  /***************************************************************************/
  #define NUM_BAD_COMMANDS 8
  char * json_command_list_dodgy_command =
    "{"
    "\"commandList\": ["
    "{"
    "\"command\": {"
    "\"commandType\": \"dodgyCommand\", "
    "\"eventDomainThrottleSpecification\": {"
    "\"eventDomain\": \"fault\", "
    "\"suppressedFieldNames\": [\"alarmInterfaceA\"], "
    "\"suppressedNvPairsList\": ["
    "{"
    "\"nvPairFieldName\": \"alarmAdditionalInformation\", "
    "\"suppressedNvPairNames\": [\"name1\"]"
    "}]}}}"
    "]"
    "}";

  char * json_command_list_dodgy_spec =
    "{"
    "\"commandList\": ["
    "{"
    "\"command\": {"
    "\"commandType\": \"throttlingSpecification\", "
    "\"dodgyEventDomainThrottleSpecification\": {"
    "\"eventDomain\": \"fault\", "
    "\"suppressedFieldNames\": [\"alarmInterfaceA\"], "
    "\"suppressedNvPairsList\": ["
    "{"
    "\"nvPairFieldName\": \"alarmAdditionalInformation\", "
    "\"suppressedNvPairNames\": [\"name1\"]"
    "}]}}}"
    "]"
    "}";

  char * json_command_list_dodgy_event_domain_key =
    "{"
    "\"commandList\": ["
    "{"
    "\"command\": {"
    "\"commandType\": \"throttlingSpecification\", "
    "\"eventDomainThrottleSpecification\": {"
    "\"dodgyEventDomainKey\": \"fault\", "
    "\"suppressedFieldNames\": [\"alarmInterfaceA\"], "
    "\"suppressedNvPairsList\": ["
    "{"
    "\"nvPairFieldName\": \"alarmAdditionalInformation\", "
    "\"suppressedNvPairNames\": [\"name1\"]"
    "}]}}}"
    "]"
    "}";

  char * json_command_list_dodgy_event_domain =
    "{"
    "\"commandList\": ["
    "{"
    "\"command\": {"
    "\"commandType\": \"throttlingSpecification\", "
    "\"eventDomainThrottleSpecification\": {"
    "\"eventDomain\": \"dodgyEventDomain\", "
    "\"suppressedFieldNames\": [\"alarmInterfaceA\"], "
    "\"suppressedNvPairsList\": ["
    "{"
    "\"nvPairFieldName\": \"alarmAdditionalInformation\", "
    "\"suppressedNvPairNames\": [\"name1\"]"
    "}]}}}"
    "]"
    "}";

  char * json_command_list_dodgy_field_names_key =
    "{"
    "\"commandList\": ["
    "{"
    "\"command\": {"
    "\"commandType\": \"throttlingSpecification\", "
    "\"eventDomainThrottleSpecification\": {"
    "\"eventDomain\": \"fault\", "
    "\"suppressedNvPairsList\": ["
    "{"
    "\"nvPairFieldName\": \"alarmAdditionalInformation\", "
    "\"suppressedNvPairNames\": [\"name1\"]"
    "}]}}}"
    "]"
    "}";

  char * json_command_list_dodgy_pair_names_list_key =
    "{"
    "\"commandList\": ["
    "{"
    "\"command\": {"
    "\"commandType\": \"throttlingSpecification\", "
    "\"eventDomainThrottleSpecification\": {"
    "\"eventDomain\": \"fault\", "
    "\"suppressedFieldNames\": [\"alarmInterfaceA\"], "
    "\"dodgySuppressedNvPairsListKey\": ["
    "{"
    "\"nvPairFieldName\": \"alarmAdditionalInformation\", "
    "\"suppressedNvPairNames\": [\"name1\"]"
    "}]}}}"
    "]"
    "}";

  char * json_command_list_dodgy_pair_field_name_key =
    "{"
    "\"commandList\": ["
    "{"
    "\"command\": {"
    "\"commandType\": \"throttlingSpecification\", "
    "\"eventDomainThrottleSpecification\": {"
    "\"eventDomain\": \"fault\", "
    "\"suppressedFieldNames\": [\"alarmInterfaceA\"], "
    "\"suppressedNvPairsList\": ["
    "{"
    "\"dodgyNvPairFieldNameKey\": \"alarmAdditionalInformation\", "
    "\"suppressedNvPairNames\": [\"name1\"]"
    "}]}}}"
    "]"
    "}";

  char * json_command_list_dodgy_pair_names_key =
    "{"
    "\"commandList\": ["
    "{"
    "\"command\": {"
    "\"commandType\": \"throttlingSpecification\", "
    "\"eventDomainThrottleSpecification\": {"
    "\"eventDomain\": \"fault\", "
    "\"suppressedFieldNames\": [\"alarmInterfaceA\"], "
    "\"suppressedNvPairsList\": ["
    "{"
    "\"nvPairFieldName\": \"alarmAdditionalInformation\", "
    "\"dodgySuppressedNvPairNamesKey\": [\"name1\"]"
    "}]}}}"
    "]"
    "}";

  char * json_command_list_dodgy_depth =
    "{"
    "\"commandList\": ["
    "{"
    "\"command\": {"
    "\"commandType\": \"throttlingSpecification\", "
    "\"eventDomainThrottleSpecification\": {"
    "\"eventDomain\": \"fault\", "
    "\"suppressedFieldNames\": [\"alarmInterfaceA\"], "
    "\"suppressedNvPairsList\": ["
    "{"
    "\"nvPairFieldName\": \"alarmAdditionalInformation\", "
    "\"dodgySuppressedNvPairNamesKey\": "
    "[\"name1\", [[[[[[[[]]]]]]]]]"
    "}]}}}"
    "]"
    "}";

  char * expected_throttle_state_dodgy_field_names_key =
    "{"
    "\"eventThrottlingState\": {"
    "\"eventThrottlingMode\": \"throttled\", "
    "\"eventDomainThrottleSpecificationList\": ["
    "{"
    "\"eventDomain\": \"fault\", "
    "\"suppressedNvPairsList\": ["
    "{"
    "\"nvPairFieldName\": \"alarmAdditionalInformation\", "
    "\"suppressedNvPairNames\": [\"name1\"]"
    "}]}"
    "]"
    "}"
    "}";

  char * expected_throttle_state_dodgy_pair_names_list_key =
    "{"
    "\"eventThrottlingState\": {"
    "\"eventThrottlingMode\": \"throttled\", "
    "\"eventDomainThrottleSpecificationList\": ["
    "{"
    "\"eventDomain\": \"fault\", "
    "\"suppressedFieldNames\": [\"alarmInterfaceA\"]"
    "}"
    "]"
    "}"
    "}";

  char * expected_throttle_state_dodgy_pair_field_name_key =
    "{"
    "\"eventThrottlingState\": {"
    "\"eventThrottlingMode\": \"throttled\", "
    "\"eventDomainThrottleSpecificationList\": ["
    "{"
    "\"eventDomain\": \"fault\", "
    "\"suppressedFieldNames\": [\"alarmInterfaceA\"]"
    "}"
    "]"
    "}"
    "}";

  char * expected_throttle_state_dodgy_pair_names_key =
    "{"
    "\"eventThrottlingState\": {"
    "\"eventThrottlingMode\": \"throttled\", "
    "\"eventDomainThrottleSpecificationList\": ["
    "{"
    "\"eventDomain\": \"fault\", "
    "\"suppressedFieldNames\": [\"alarmInterfaceA\"]"
    "}"
    "]"
    "}"
    "}";

  char * json_command_lists[] = {
    json_command_list_dodgy_command,
    json_command_list_dodgy_spec,
    json_command_list_dodgy_event_domain_key,
    json_command_list_dodgy_event_domain,
    json_command_list_dodgy_depth,
    json_command_list_dodgy_field_names_key,
    json_command_list_dodgy_pair_names_list_key,
    json_command_list_dodgy_pair_field_name_key,
    json_command_list_dodgy_pair_names_key
  };

  char * expected_posts[] = {
    expected_throttle_state_normal,
    expected_throttle_state_normal,
    expected_throttle_state_normal,
    expected_throttle_state_normal,
    expected_throttle_state_normal,
    expected_throttle_state_dodgy_field_names_key,
    expected_throttle_state_dodgy_pair_names_list_key,
    expected_throttle_state_dodgy_pair_field_name_key,
    expected_throttle_state_dodgy_pair_names_key
   };

  const int num_commands =
    sizeof(json_command_lists) / sizeof(json_command_lists[0]);
  const int num_posts =
    sizeof(expected_posts) / sizeof(expected_posts[0]);
  assert(num_commands == num_posts);

  /***************************************************************************/
  /* Initialize and provide a specification with a single nvpair with a      */
  /* single sub-field suppressed.                                            */
  /***************************************************************************/
  evel_throttle_initialize();

  int ii;
  for (ii = 0; ii < num_commands; ii++)
  {
    EVEL_DEBUG("Testing commandList[%d] = %s\n", ii, json_command_lists[ii]);
    handle_json_response(json_command_lists[ii], &post);

    /*************************************************************************/
    /* Check that throttling is in a normal state - because we ignored the   */
    /* command / .....                                                       */
    /*************************************************************************/
    for (domain = EVEL_DOMAIN_MEASUREMENT; domain < EVEL_MAX_DOMAINS; domain++)
    {
      assert(evel_get_throttle_spec(domain) == NULL);
    }
    if (expected_posts[ii] == expected_throttle_state_normal)
    {
      assert(evel_get_throttle_spec(EVEL_DOMAIN_FAULT) == NULL);
    }
    else
    {
      assert(evel_get_throttle_spec(EVEL_DOMAIN_FAULT) != NULL);
    }
    assert(post.memory == NULL);

    /*************************************************************************/
    /* Request and verify the throttling state.                              */
    /*************************************************************************/
    handle_json_response(json_command_list_provide, &post);
    assert(post.memory != NULL);
    compare_strings(expected_posts[ii],
                    post.memory,
                    strlen(expected_posts[ii]),
                    "Throttle State Normal");
    free(post.memory);
    post.memory = NULL;
  }

  evel_throttle_terminate();
}

void test_encode_fault_throttled()
{
  MEMORY_CHUNK post;

  /***************************************************************************/
  /* We also test suppression of the event header parameters here.           */
  /***************************************************************************/
  char * json_command_list =
    "{"
    "\"commandList\": ["
    "{"
    "\"command\": {"
    "\"commandType\": \"throttlingSpecification\", "
    "\"eventDomainThrottleSpecification\": {"
    "\"eventDomain\": \"fault\", "
    "\"suppressedFieldNames\": ["
    "\"alarmInterfaceA\", "
    "\"eventType\", "
    "\"reportingEntityId\", "
    "\"sourceId\"], "
    "\"suppressedNvPairsList\": ["
    "{"
    "\"nvPairFieldName\": \"alarmAdditionalInformation\", "
    "\"suppressedNvPairNames\": [\"name3\", \"name4\"]"
    "}]}}}"
    "]"
    "}";

  char * expected =
    "{\"event\": {"
    "\"commonEventHeader\": {"
    "\"domain\": \"fault\", "
    "\"eventId\": \"122\", "
    "\"functionalRole\": \"UNIT TEST\", "
    "\"lastEpochMicrosec\": 1000002, "
    "\"priority\": \"Normal\", "
    "\"reportingEntityName\": \"Dummy VM name - No Metadata available\", "
    "\"sequence\": 122, "
    "\"sourceName\": \"Dummy VM name - No Metadata available\", "
    "\"startEpochMicrosec\": 1000002, "
    "\"version\": 1.2"
    "}, "
    "\"faultFields\": {"
    "\"alarmCondition\": \"My alarm condition\", "
    "\"eventSeverity\": \"MAJOR\", "
    "\"eventSourceType\": \"other\", "
    "\"specificProblem\": \"It broke very badly\", "
    "\"vfStatus\": \"Active\", "
    "\"faultFieldsVersion\": 1.1, "
    "\"alarmAdditionalInformation\": ["
    "{\"name\": \"name1\", "
    "\"value\": \"value1\"}, "
    "{\"name\": \"name2\", "
    "\"value\": \"value2\"}]"
    "}}}";

  /***************************************************************************/
  /* Initialize and provide a specification with a single fault suppressed.  */
  /***************************************************************************/
  evel_throttle_initialize();
  handle_json_response(json_command_list, &post);

  /***************************************************************************/
  /* Check that the domain is throttled.                                     */
  /***************************************************************************/
  assert(evel_get_throttle_spec(EVEL_DOMAIN_FAULT) != NULL);
  assert(post.memory == NULL);

  size_t json_size = 0;
  char json_body[EVEL_MAX_JSON_BODY];
  evel_set_next_event_sequence(122);
  EVENT_FAULT * fault = evel_new_fault("My alarm condition",
                                       "It broke very badly",
                                       EVEL_PRIORITY_NORMAL,
                                       EVEL_SEVERITY_MAJOR,
					EVEL_SOURCE_HOST,
                          EVEL_VF_STATUS_PREP_TERMINATE);
  assert(fault != NULL);
  evel_fault_type_set(fault, "Bad things happen...");
  evel_fault_addl_info_add(fault, "name1", "value1");
  evel_fault_addl_info_add(fault, "name2", "value2");

  /***************************************************************************/
  /* Suppressed fields.                                                      */
  /***************************************************************************/
  evel_fault_interface_set(fault, "My Interface Card");
  evel_fault_addl_info_add(fault, "name3", "value3");
  evel_fault_addl_info_add(fault, "name4", "value4");

  json_size = evel_json_encode_event(
    json_body, EVEL_MAX_JSON_BODY, (EVENT_HEADER *) fault);
  compare_strings(expected, json_body, EVEL_MAX_JSON_BODY, "Fault");
  assert((json_size == strlen(json_body)) && "Bad size returned");

  evel_free_event(fault);
  evel_throttle_terminate();
}

void test_encode_measurement_throttled()
{
  MEMORY_CHUNK post;

  /***************************************************************************/
  /* We also test suppression of the event header parameters here.           */
  /***************************************************************************/
  char * json_command_list =
    "{"
    "\"commandList\": ["
    "{"
    "\"command\": {"
    "\"commandType\": \"throttlingSpecification\", "
    "\"eventDomainThrottleSpecification\": {"
    "\"eventDomain\": \"measurementsForVfScaling\", "
    "\"suppressedFieldNames\": ["
    "\"errors\", "
    "\"vnfcScalingMetric\", "
    "\"numberOfMediaPortsInUse\", "
    "\"aggregateCpuUsage\", "
    "\"requestRate\", "
    "\"memoryUsed\", "
    "\"memoryConfigured\", "
    "\"meanRequestLatency\", "
    "\"latencyDistribution\", "
    "\"concurrentSessions\", "
    "\"configuredEntities\", "
    "\"eventType\", "
    "\"reportingEntityId\", "
    "\"sourceId\"], "
    "\"suppressedNvPairsList\": ["
    "{"
    "\"nvPairFieldName\": \"cpuUsageArray\", "
    "\"suppressedNvPairNames\": [\"cpu3\", \"cpu4\"]"
    "}, "
    "{"
    "\"nvPairFieldName\": \"filesystemUsageArray\", "
    "\"suppressedNvPairNames\": [\"00-11-22\", \"33-44-55\"]"
    "}, "
    "{"
    "\"nvPairFieldName\": \"vNicUsageArray\", "
    "\"suppressedNvPairNames\": [\"eth1\", \"eth0\"]"
    "}, "
    "{"
    "\"nvPairFieldName\": \"featureUsageArray\", "
    "\"suppressedNvPairNames\": [\"FeatureB\", \"FeatureC\"]"
    "},"
    "{"
    "\"nvPairFieldName\": \"codecUsageArray\", "
    "\"suppressedNvPairNames\": [\"G729ab\"]"
    "},"
    "{"
    "\"nvPairFieldName\": \"additionalMeasurements\", "
    "\"suppressedNvPairNames\": [\"Group2\"]"
    "}"
    "]}}}"
    "]"
    "}";

  char * expected =
    "{\"event\": "
    "{\"commonEventHeader\": {"
    "\"domain\": \"measurementsForVfScaling\", "
    "\"eventId\": \"123\", "
    "\"functionalRole\": \"UNIT TEST\", "
    "\"lastEpochMicrosec\": 1000002, "
    "\"priority\": \"Normal\", "
    "\"reportingEntityName\": \"Dummy VM name - No Metadata available\", "
    "\"sequence\": 123, "
    "\"sourceName\": \"Dummy VM name - No Metadata available\", "
    "\"startEpochMicrosec\": 1000002, "
    "\"version\": 1.2"
    "}, "
    "\"measurementsForVfScalingFields\": "
    "{"
    "\"measurementInterval\": 5.500000, "
    "\"cpuUsageArray\": ["
    "{\"cpuIdentifier\": \"cpu1\", "
    "\"percentUsage\": 11.110000}, "
    "{\"cpuIdentifier\": \"cpu2\", "
    "\"percentUsage\": 22.220000}], "
    "\"filesystemUsageArray\": ["
    "{\"blockConfigured\": 500.110000, "
    "\"blockIops\": 77, "
    "\"blockUsed\": 500.220000, "
    "\"ephemeralConfigured\": 500.110000, "
    "\"ephemeralIops\": 88, "
    "\"ephemeralUsed\": 600.220000, "
    "\"filesystemName\": \"66-77-88\"}], "
    "\"featureUsageArray\": ["
    "{\"featureIdentifier\": \"FeatureA\", "
    "\"featureUtilization\": 123}], "
    "\"codecUsageArray\": ["
    "{\"codecIdentifier\": \"G711a\", "
    "\"numberInUse\": 91}], "
    "\"additionalMeasurements\": ["
    "{\"name\": \"Group1\", "
    "\"measurements\": ["
    "{\"name\": \"Name1\", "
    "\"value\": \"Value1\"}]}], "
    "\"measurementsForVfScalingVersion\": 1.1}}}";
     MEASUREMENT_CPU_USE *cpu_use;

  /***************************************************************************/
  /* Initialize and provide a specification with a single fault suppressed.  */
  /***************************************************************************/
  evel_throttle_initialize();
  handle_json_response(json_command_list, &post);

  /***************************************************************************/
  /* Check that the domain is throttled.                                     */
  /***************************************************************************/
  assert(evel_get_throttle_spec(EVEL_DOMAIN_MEASUREMENT) != NULL);
  assert(post.memory == NULL);

  size_t json_size = 0;
  char json_body[EVEL_MAX_JSON_BODY];
  evel_set_next_event_sequence(123);
  EVENT_MEASUREMENT * measurement = evel_new_measurement(5.5);
  MEASUREMENT_LATENCY_BUCKET * bucket = NULL;
  MEASUREMENT_VNIC_PERFORMANCE * vnic_use = NULL;
  assert(measurement != NULL);

  evel_measurement_type_set(measurement, "Perf management...");
  evel_measurement_conc_sess_set(measurement, 1);
  evel_measurement_cfg_ents_set(measurement, 2);
  evel_measurement_mean_req_lat_set(measurement, 4.4);
  evel_measurement_mem_cfg_set(measurement, 6.6);
  evel_measurement_mem_used_set(measurement, 3.3);
  evel_measurement_request_rate_set(measurement, 7);

  cpu_use = evel_measurement_new_cpu_use_add(measurement, "cpu1", 11.11);
  evel_measurement_cpu_use_idle_set(cpu_use,22.22);
  evel_measurement_cpu_use_interrupt_set(cpu_use,33.33);
  evel_measurement_cpu_use_nice_set(cpu_use,44.44);
  evel_measurement_cpu_use_softirq_set(cpu_use,55.55);
  evel_measurement_cpu_use_steal_set(cpu_use,66.66);
  evel_measurement_cpu_use_system_set(cpu_use,77.77);
  evel_measurement_cpu_use_usageuser_set(cpu_use,88.88);
  evel_measurement_cpu_use_wait_set(cpu_use,99.99);

  cpu_use = evel_measurement_new_cpu_use_add(measurement, "cpu2", 22.22);
  evel_measurement_cpu_use_idle_set(cpu_use,12.22);
  evel_measurement_cpu_use_interrupt_set(cpu_use,33.33);
  evel_measurement_cpu_use_nice_set(cpu_use,44.44);
  evel_measurement_cpu_use_softirq_set(cpu_use,55.55);
  evel_measurement_cpu_use_steal_set(cpu_use,66.66);
  evel_measurement_cpu_use_system_set(cpu_use,77.77);
  evel_measurement_cpu_use_usageuser_set(cpu_use,88.88);
  evel_measurement_cpu_use_wait_set(cpu_use,19.99);

  evel_measurement_fsys_use_add(measurement, "00-11-22",
                                100.11, 100.22, 33,
                                200.11, 200.22, 44);
  evel_measurement_fsys_use_add(measurement, "33-44-55",
                                300.11, 300.22, 55,
                                400.11, 400.22, 66);
  evel_measurement_fsys_use_add(measurement, "66-77-88",
                                500.11, 500.22, 77,
                                600.11, 600.22, 88);

  bucket = evel_new_meas_latency_bucket(20);
  evel_meas_latency_bucket_add(measurement, bucket);

  bucket = evel_new_meas_latency_bucket(30);
  evel_meas_latency_bucket_low_end_set(bucket, 10.0);
  evel_meas_latency_bucket_high_end_set(bucket, 20.0);
  evel_meas_latency_bucket_add(measurement, bucket);

  vnic_use = evel_new_measurement_vnic_use("eth0", 100, 200, 3, 4);
  evel_vnic_use_bcast_pkt_in_set(vnic_use, 1);
  evel_vnic_use_bcast_pkt_out_set(vnic_use, 2);
  evel_vnic_use_mcast_pkt_in_set(vnic_use, 5);
  evel_vnic_use_mcast_pkt_out_set(vnic_use, 6);
  evel_vnic_use_ucast_pkt_in_set(vnic_use, 7);
  evel_vnic_use_ucast_pkt_out_set(vnic_use, 8);
  evel_meas_vnic_use_add(measurement, vnic_use);

  vnic_use = evel_new_measurement_vnic_use("eth1", 110, 240, 13, 14);
  evel_vnic_use_bcast_pkt_in_set(vnic_use, 11);
  evel_vnic_use_bcast_pkt_out_set(vnic_use, 12);
  evel_vnic_use_mcast_pkt_in_set(vnic_use, 15);
  evel_vnic_use_mcast_pkt_out_set(vnic_use, 16);
  evel_vnic_use_ucast_pkt_in_set(vnic_use, 17);
  evel_vnic_use_ucast_pkt_out_set(vnic_use, 18);
  evel_meas_vnic_use_add(measurement, vnic_use);

  evel_measurement_errors_set(measurement, 1, 0, 2, 1);
  evel_measurement_feature_use_add(measurement, "FeatureA", 123);
  evel_measurement_feature_use_add(measurement, "FeatureB", 567);
  evel_measurement_codec_use_add(measurement, "G711a", 91);
  evel_measurement_codec_use_add(measurement, "G729ab", 92);
  evel_measurement_media_port_use_set(measurement, 1234);
  evel_measurement_vnfc_scaling_metric_set(measurement, 1234.5678);
  evel_measurement_custom_measurement_add(measurement,
                                          "Group1", "Name1", "Value1");
  evel_measurement_custom_measurement_add(measurement,
                                          "Group2", "Name1", "Value1");
  evel_measurement_custom_measurement_add(measurement,
                                          "Group2", "Name2", "Value2");

  json_size = evel_json_encode_event(
    json_body, EVEL_MAX_JSON_BODY, (EVENT_HEADER *) measurement);
  compare_strings(expected, json_body, EVEL_MAX_JSON_BODY, "Measurement");
  assert((json_size == strlen(json_body)) && "Bad size returned");

  evel_free_event(measurement);
  evel_throttle_terminate();
}

void test_encode_mobile_throttled()
{
  MEMORY_CHUNK post;

  /***************************************************************************/
  /* We also test suppression of the event header parameters here.           */
  /***************************************************************************/
  char * json_command_list =
    "{"
    "\"commandList\": ["
    "{"
    "\"command\": {"
    "\"commandType\": \"throttlingSpecification\", "
    "\"eventDomainThrottleSpecification\": {"
    "\"eventDomain\": \"mobileFlow\", "
    "\"suppressedFieldNames\": ["
    "\"applicationType\", "
    "\"appProtocolType\", "
    "\"appProtocolVersion\", "
    "\"cid\", "
    "\"connectionType\", "
    "\"ecgi\", "
    "\"gtpProtocolType\", "
    "\"gtpVersion\", "
    "\"httpHeader\", "
    "\"imei\", "
    "\"imsi\", "
    "\"lac\", "
    "\"mcc\", "
    "\"mnc\", "
    "\"msisdn\", "
    "\"otherFunctionalRole\", "
    "\"rac\", "
    "\"radioAccessTechnology\", "
    "\"sac\", "
    "\"samplingAlgorithm\", "
    "\"tac\", "
    "\"tunnelId\", "
    "\"vlanId\", "
    "\"eventType\", "
    "\"reportingEntityId\", "
    "\"sourceId\"], "
    "\"suppressedNvPairsList\": ["
    "]}}}"
    "]"
    "}";

  char * expected =
    "{\"event\": "
    "{\"commonEventHeader\": {"
    "\"domain\": \"mobileFlow\", "
    "\"eventId\": \"1242\", "
    "\"functionalRole\": \"UNIT TEST\", "
    "\"lastEpochMicrosec\": 1000002, "
    "\"priority\": \"Normal\", "
    "\"reportingEntityName\": \"Dummy VM name - No Metadata available\", "
    "\"sequence\": 1242, "
    "\"sourceName\": \"Dummy VM name - No Metadata available\", "
    "\"startEpochMicrosec\": 1000002, "
    "\"version\": 1.2"
    "}, "
    "\"mobileFlowFields\": {"
    "\"flowDirection\": \"Inbound\", "
    "\"gtpPerFlowMetrics\": {"
    "\"avgBitErrorRate\": 132.000100, "
    "\"avgPacketDelayVariation\": 31.200000, "
    "\"avgPacketLatency\": 101, "
    "\"avgReceiveThroughput\": 2101, "
    "\"avgTransmitThroughput\": 501, "
    "\"flowActivationEpoch\": 1470409422, "
    "\"flowActivationMicrosec\": 988, "
    "\"flowDeactivationEpoch\": 1470409432, "
    "\"flowDeactivationMicrosec\": 12, "
    "\"flowDeactivationTime\": \"Fri, 05 Aug 2016 15:03:52 +0000\", "
    "\"flowStatus\": \"Inactive\", "
    "\"maxPacketDelayVariation\": 88, "
    "\"numActivationFailures\": 4, "
    "\"numBitErrors\": 18, "
    "\"numBytesReceived\": 123655, "
    "\"numBytesTransmitted\": 4562, "
    "\"numDroppedPackets\": 1, "
    "\"numL7BytesReceived\": 13, "
    "\"numL7BytesTransmitted\": 11, "
    "\"numLostPackets\": 2, "
    "\"numOutOfOrderPackets\": 4, "
    "\"numPacketErrors\": 8, "
    "\"numPacketsReceivedExclRetrans\": 900, "
    "\"numPacketsReceivedInclRetrans\": 902, "
    "\"numPacketsTransmittedInclRetrans\": 303, "
    "\"numRetries\": 7, "
    "\"numTimeouts\": 3, "
    "\"numTunneledL7BytesReceived\": 1, "
    "\"roundTripTime\": 111, "
    "\"timeToFirstByte\": 226, "
    "\"ipTosCountList\": ["
    "[\"1\", 13], "
    "[\"4\", 99], "
    "[\"17\", 1]], "
    "\"ipTosList\": [\"1\", \"4\", \"17\"], "
    "\"tcpFlagList\": [\"CWR\", \"URG\"], "
    "\"tcpFlagCountList\": [[\"CWR\", 10], [\"URG\", 121]], "
    "\"mobileQciCosList\": [\"conversational\", \"65\"], "
    "\"mobileQciCosCountList\": [[\"conversational\", 11], [\"65\", 122]], "
    "\"durConnectionFailedStatus\": 12, "
    "\"durTunnelFailedStatus\": 13, "
    "\"flowActivatedBy\": \"Remote\", "
    "\"flowActivationTime\": \"Fri, 05 Aug 2016 15:03:43 +0000\", "
    "\"flowDeactivatedBy\": \"Remote\", "
    "\"gtpConnectionStatus\": \"Connected\", "
    "\"gtpTunnelStatus\": \"Not tunneling\", "
    "\"largePacketRtt\": 80, "
    "\"largePacketThreshold\": 600.000000, "
    "\"maxReceiveBitRate\": 1357924680, "
    "\"maxTransmitBitRate\": 235711, "
    "\"numGtpEchoFailures\": 1, "
    "\"numGtpTunnelErrors\": 4, "
    "\"numHttpErrors\": 2"
    "}, "
    "\"ipProtocolType\": \"UDP\", "
    "\"ipVersion\": \"IPv6\", "
    "\"otherEndpointIpAddress\": \"2.3.4.2\", "
    "\"otherEndpointPort\": 2342, "
    "\"reportingEndpointIpAddr\": \"4.2.3.2\", "
    "\"reportingEndpointPort\": 4322"
    "}}}";

  /***************************************************************************/
  /* Initialize and provide a specification with a single fault suppressed.  */
  /***************************************************************************/
  evel_throttle_initialize();
  handle_json_response(json_command_list, &post);

  /***************************************************************************/
  /* Check that the domain is throttled.                                     */
  /***************************************************************************/
  assert(evel_get_throttle_spec(EVEL_DOMAIN_MOBILE_FLOW) != NULL);
  assert(post.memory == NULL);

  size_t json_size = 0;
  char json_body[EVEL_MAX_JSON_BODY];
  MOBILE_GTP_PER_FLOW_METRICS * metrics = NULL;
  EVENT_MOBILE_FLOW * mobile_flow = NULL;

  /***************************************************************************/
  /* Mobile.                                                                 */
  /***************************************************************************/
  evel_set_next_event_sequence(1242);

  metrics = evel_new_mobile_gtp_flow_metrics(132.0001,
                                             31.2,
                                             101,
                                             2101,
                                             501,
                                             1470409422,
                                             988,
                                             1470409432,
                                             12,
                                             (time_t)1470409432,
                                             "Inactive",
                                             88,
                                             4,
                                             18,
                                             123655,
                                             4562,
                                             1,
                                             13,
                                             11,
                                             2,
                                             4,
                                             8,
                                             900,
                                             902,
                                             303,
                                             7,
                                             3,
                                             1,
                                             111,
                                             226);
  assert(metrics != NULL);

  evel_mobile_gtp_metrics_dur_con_fail_set(metrics, 12);
  evel_mobile_gtp_metrics_dur_tun_fail_set(metrics, 13);
  evel_mobile_gtp_metrics_act_by_set(metrics, "Remote");
  evel_mobile_gtp_metrics_act_time_set(metrics, (time_t)1470409423);
  evel_mobile_gtp_metrics_deact_by_set(metrics, "Remote");
  evel_mobile_gtp_metrics_con_status_set(metrics, "Connected");
  evel_mobile_gtp_metrics_tun_status_set(metrics, "Not tunneling");
  evel_mobile_gtp_metrics_iptos_set(metrics, 1, 13);
  evel_mobile_gtp_metrics_iptos_set(metrics, 17, 1);
  evel_mobile_gtp_metrics_iptos_set(metrics, 4, 99);
  evel_mobile_gtp_metrics_large_pkt_rtt_set(metrics, 80);
  evel_mobile_gtp_metrics_large_pkt_thresh_set(metrics, 600.0);
  evel_mobile_gtp_metrics_max_rcv_bit_rate_set(metrics, 1357924680);
  evel_mobile_gtp_metrics_max_trx_bit_rate_set(metrics, 235711);
  evel_mobile_gtp_metrics_num_echo_fail_set(metrics, 1);
  evel_mobile_gtp_metrics_num_tun_fail_set(metrics, 4);
  evel_mobile_gtp_metrics_num_http_errors_set(metrics, 2);
  evel_mobile_gtp_metrics_tcp_flag_count_add(metrics, EVEL_TCP_CWR, 10);
  evel_mobile_gtp_metrics_tcp_flag_count_add(metrics, EVEL_TCP_URG, 121);
  evel_mobile_gtp_metrics_qci_cos_count_add(
                                metrics, EVEL_QCI_COS_UMTS_CONVERSATIONAL, 11);
  evel_mobile_gtp_metrics_qci_cos_count_add(
                                            metrics, EVEL_QCI_COS_LTE_65, 122);

  mobile_flow = evel_new_mobile_flow("Inbound",
                                     metrics,
                                     "UDP",
                                     "IPv6",
                                     "2.3.4.2",
                                     2342,
                                     "4.2.3.2",
                                     4322);
  assert(mobile_flow != NULL);

  evel_mobile_flow_type_set(mobile_flow, "Mobile flow...");
  evel_mobile_flow_app_type_set(mobile_flow, "Demo application");
  evel_mobile_flow_app_prot_type_set(mobile_flow, "GSM");
  evel_mobile_flow_app_prot_ver_set(mobile_flow, "1");
  evel_mobile_flow_cid_set(mobile_flow, "65535");
  evel_mobile_flow_con_type_set(mobile_flow, "S1-U");
  evel_mobile_flow_ecgi_set(mobile_flow, "e65535");
  evel_mobile_flow_gtp_prot_type_set(mobile_flow, "GTP-U");
  evel_mobile_flow_gtp_prot_ver_set(mobile_flow, "1");
  evel_mobile_flow_http_header_set(mobile_flow,
                                   "http://www.something.com");
  evel_mobile_flow_imei_set(mobile_flow, "209917614823");
  evel_mobile_flow_imsi_set(mobile_flow, "355251/05/850925/8");
  evel_mobile_flow_lac_set(mobile_flow, "1");
  evel_mobile_flow_mcc_set(mobile_flow, "410");
  evel_mobile_flow_mnc_set(mobile_flow, "04");
  evel_mobile_flow_msisdn_set(mobile_flow, "6017123456789");
  evel_mobile_flow_other_func_role_set(mobile_flow, "MME");
  evel_mobile_flow_rac_set(mobile_flow, "514");
  evel_mobile_flow_radio_acc_tech_set(mobile_flow, "LTE");
  evel_mobile_flow_sac_set(mobile_flow, "1");
  evel_mobile_flow_samp_alg_set(mobile_flow, 1);
  evel_mobile_flow_tac_set(mobile_flow, "2099");
  evel_mobile_flow_tunnel_id_set(mobile_flow, "Tunnel 1");
  evel_mobile_flow_vlan_id_set(mobile_flow, "15");

  json_size = evel_json_encode_event(
    json_body, EVEL_MAX_JSON_BODY, (EVENT_HEADER *) mobile_flow);
  compare_strings(expected, json_body, EVEL_MAX_JSON_BODY, "Mobile");
  assert((json_size == strlen(json_body)) && "Bad size returned");

  evel_free_event(mobile_flow);
  evel_throttle_terminate();
}

void test_encode_other_throttled()
{
  MEMORY_CHUNK post;

  /***************************************************************************/
  /* We also test suppression of the event header parameters here.           */
  /***************************************************************************/
  char * json_command_list =
    "{"
    "\"commandList\": ["
    "{"
    "\"command\": {"
    "\"commandType\": \"throttlingSpecification\", "
    "\"eventDomainThrottleSpecification\": {"
    "\"eventDomain\": \"other\", "
    "\"suppressedFieldNames\": ["
    "\"eventType\", "
    "\"reportingEntityId\", "
    "\"sourceId\"], "
    "\"suppressedNvPairsList\": ["
    "]}}}"
    "]"
    "}";

  char * expected =
    "{\"event\": "
    "{\"commonEventHeader\": {"
    "\"domain\": \"other\", "
    "\"eventId\": \"129\", "
    "\"functionalRole\": \"UNIT TEST\", "
    "\"lastEpochMicrosec\": 1000002, "
    "\"priority\": \"Normal\", "
    "\"reportingEntityName\": \"Dummy VM name - No Metadata available\", "
    "\"sequence\": 129, "
    "\"sourceName\": \"Dummy VM name - No Metadata available\", "
    "\"startEpochMicrosec\": 1000002, "
    "\"version\": 1.2"
    "}, "
    "\"otherFields\": ["
    "{\"name\": \"Other field 1\", "
    "\"value\": \"Other value 1\"}, "
    "{\"name\": \"Other field 2\", "
    "\"value\": \"Other value 2\"}"
    "]"
    "}}";

  /***************************************************************************/
  /* Initialize and provide a specification with a single fault suppressed.  */
  /***************************************************************************/
  evel_throttle_initialize();
  handle_json_response(json_command_list, &post);

  /***************************************************************************/
  /* Check that the domain is throttled.                                     */
  /***************************************************************************/
  assert(evel_get_throttle_spec(EVEL_DOMAIN_OTHER) != NULL);
  assert(post.memory == NULL);

  size_t json_size = 0;
  char json_body[EVEL_MAX_JSON_BODY];
  EVENT_OTHER * other = NULL;
  evel_set_next_event_sequence(129);
  other = evel_new_other();
  assert(other != NULL);
  evel_other_type_set(other, "Other Type");
  evel_other_field_add(other,
                       "Other field 1",
                       "Other value 1");
  evel_other_field_add(other,
                       "Other field 2",
                       "Other value 2");

  json_size = evel_json_encode_event(
    json_body, EVEL_MAX_JSON_BODY, (EVENT_HEADER *) other);
  compare_strings(expected, json_body, EVEL_MAX_JSON_BODY, "Other");
  assert((json_size == strlen(json_body)) && "Bad size returned");

  evel_free_event(other);
  evel_throttle_terminate();
}

void test_encode_report_throttled()
{
  MEMORY_CHUNK post;

  /***************************************************************************/
  /* We also test suppression of the event header parameters here.           */
  /***************************************************************************/
  char * json_command_list =
    "{"
    "\"commandList\": ["
    "{"
    "\"command\": {"
    "\"commandType\": \"throttlingSpecification\", "
    "\"eventDomainThrottleSpecification\": {"
    "\"eventDomain\": \"report\", "
    "\"suppressedFieldNames\": ["
    "\"eventType\", "
    "\"reportingEntityId\", "
    "\"sourceId\"], "
    "\"suppressedNvPairsList\": ["
    "{"
    "\"nvPairFieldName\": \"featureUsageArray\", "
    "\"suppressedNvPairNames\": [\"FeatureB\", \"FeatureC\"]"
    "},"
    "{"
    "\"nvPairFieldName\": \"additionalMeasurements\", "
    "\"suppressedNvPairNames\": [\"Group2\"]"
    "}"
    "]}}}"
    "]"
    "}";

  char * expected =
    "{\"event\": "
    "{\"commonEventHeader\": {"
    "\"domain\": \"measurementsForVfReporting\", "
    "\"eventId\": \"125\", "
    "\"functionalRole\": \"UNIT TEST\", "
    "\"lastEpochMicrosec\": 1000002, "
    "\"priority\": \"Normal\", "
    "\"reportingEntityName\": \"Dummy VM name - No Metadata available\", "
    "\"sequence\": 125, "
    "\"sourceName\": \"Dummy VM name - No Metadata available\", "
    "\"startEpochMicrosec\": 1000002, "
    "\"version\": 1.2"
    "}, "
    "\"measurementsForVfReportingFields\": "
    "{\"measurementInterval\": 1.100000, "
    "\"featureUsageArray\": ["
    "{\"featureIdentifier\": \"FeatureA\", "
    "\"featureUtilization\": 123}], "
    "\"additionalMeasurements\": ["
    "{\"name\": \"Group1\", "
    "\"measurements\": ["
    "{\"name\": \"Name1\", "
    "\"value\": \"Value1\"}]}], "
    "\"measurementFieldsVersion\": 1.1}}}";

  /***************************************************************************/
  /* Initialize and provide a specification with a single fault suppressed.  */
  /***************************************************************************/
  evel_throttle_initialize();
  handle_json_response(json_command_list, &post);

  /***************************************************************************/
  /* Check that the domain is throttled.                                     */
  /***************************************************************************/
  assert(evel_get_throttle_spec(EVEL_DOMAIN_REPORT) != NULL);
  assert(post.memory == NULL);

  size_t json_size = 0;
  char json_body[EVEL_MAX_JSON_BODY];
  EVENT_REPORT * report = NULL;

  /***************************************************************************/
  /* Report.                                                                 */
  /***************************************************************************/
  evel_set_next_event_sequence(125);
  report = evel_new_report(1.1);
  assert(report != NULL);
  evel_report_type_set(report, "Perf reporting...");
  evel_report_feature_use_add(report, "FeatureA", 123);
  evel_report_feature_use_add(report, "FeatureB", 567);
  evel_report_custom_measurement_add(report, "Group1", "Name1", "Value1");
  evel_report_custom_measurement_add(report, "Group2", "Name1", "Value1");
  evel_report_custom_measurement_add(report, "Group2", "Name2", "Value2");

  json_size = evel_json_encode_event(
    json_body, EVEL_MAX_JSON_BODY, (EVENT_HEADER *) report);
  compare_strings(expected, json_body, EVEL_MAX_JSON_BODY, "Report");
  assert((json_size == strlen(json_body)) && "Bad size returned");

  evel_free_event(report);
  evel_throttle_terminate();
}

void test_encode_service_throttled()
{
  MEMORY_CHUNK post;

  /***************************************************************************/
  /* We also test suppression of the event header parameters here.           */
  /***************************************************************************/
  char * json_command_list =
    "{"
    "\"commandList\": ["
    "{"
    "\"command\": {"
    "\"commandType\": \"throttlingSpecification\", "
    "\"eventDomainThrottleSpecification\": {"
    "\"eventDomain\": \"serviceEvents\", "
    "\"suppressedFieldNames\": ["
    "\"eventType\", "
    "\"correlator\", "
    "\"codecSelected\", "
    "\"codecSelectedTranscoding\", "
    "\"endOfCallVqmSummaries\", "
    "\"midCallRtcp\", "
    "\"marker\", "
    "\"reportingEntityId\", "
    "\"sourceId\"], "
    "\"suppressedNvPairsList\": ["
    "{"
    "\"nvPairFieldName\": \"additionalFields\", "
    "\"suppressedNvPairNames\": [\"Name1\", \"Name3\"]"
    "}"
    "]}}}"
    "]"
    "}";

  char * expected =
    "{\"event\": "
    "{\"commonEventHeader\": {"
    "\"domain\": \"serviceEvents\", "
    "\"eventId\": \"2000\", "
    "\"functionalRole\": \"UNIT TEST\", "
    "\"lastEpochMicrosec\": 1000002, "
    "\"priority\": \"Normal\", "
    "\"reportingEntityName\": \"Dummy VM name - No Metadata available\", "
    "\"sequence\": 2000, "
    "\"sourceName\": \"Dummy VM name - No Metadata available\", "
    "\"startEpochMicrosec\": 1000002, "
    "\"version\": 1.2"
    "}, "
    "\"serviceEventsFields\": {"
    "\"eventInstanceIdentifier\": "
    "{"
    "\"vendorId\": \"vendor_x_id\", "
    "\"eventId\": \"vendor_x_event_id\", "
    "\"productId\": \"vendor_x_product_id\", "
    "\"subsystemId\": \"vendor_x_subsystem_id\", "
    "\"eventFriendlyName\": \"vendor_x_frieldly_name\""
    "}, "
    "\"serviceEventsFieldsVersion\": 1.1, "
    "\"additionalFields\": ["
    "{\"name\": \"Name2\", \"value\": \"Value2\"}, "
    "{\"name\": \"Name4\", \"value\": \"Value4\"}]"
    "}}}";

  /***************************************************************************/
  /* Initialize and provide a specification with a single fault suppressed.  */
  /***************************************************************************/
  evel_throttle_initialize();
  handle_json_response(json_command_list, &post);

  /***************************************************************************/
  /* Check that the domain is throttled.                                     */
  /***************************************************************************/
  assert(evel_get_throttle_spec(EVEL_DOMAIN_SERVICE) != NULL);
  assert(post.memory == NULL);

  size_t json_size = 0;
  char json_body[EVEL_MAX_JSON_BODY];
  EVENT_SERVICE * event = NULL;
  evel_set_next_event_sequence(2000);
  event = evel_new_service("vendor_x_id", "vendor_x_event_id");
  assert(event != NULL);
  evel_service_type_set(event, "Service Event");
  evel_service_product_id_set(event, "vendor_x_product_id");
  evel_service_subsystem_id_set(event, "vendor_x_subsystem_id");
  evel_service_friendly_name_set(event, "vendor_x_frieldly_name");
  evel_service_correlator_set(event, "vendor_x_correlator");
  evel_service_codec_set(event, "PCMA");
  evel_service_codec_set(event, "PCMA");
  evel_service_callee_codec_set(event, "PCMA");
  evel_service_caller_codec_set(event, "G729A");
  evel_service_rtcp_data_set(event, "some_rtcp_data");
  evel_service_adjacency_name_set(event, "vendor_x_adjacency");
  evel_service_endpoint_desc_set(event, EVEL_SERVICE_ENDPOINT_CALLER);
  evel_service_endpoint_jitter_set(event, 66);
  evel_service_endpoint_rtp_oct_disc_set(event, 100);
  evel_service_endpoint_rtp_oct_recv_set(event, 200);
  evel_service_endpoint_rtp_oct_sent_set(event, 300);
  evel_service_endpoint_rtp_pkt_disc_set(event, 400);
  evel_service_endpoint_rtp_pkt_recv_set(event, 500);
  evel_service_endpoint_rtp_pkt_sent_set(event, 600);
  evel_service_local_jitter_set(event, 99);
  evel_service_local_rtp_oct_disc_set(event, 150);
  evel_service_local_rtp_oct_recv_set(event, 250);
  evel_service_local_rtp_oct_sent_set(event, 350);
  evel_service_local_rtp_pkt_disc_set(event, 450);
  evel_service_local_rtp_pkt_recv_set(event, 550);
  evel_service_local_rtp_pkt_sent_set(event, 650);
  evel_service_mos_cqe_set(event, 12.255);
  evel_service_packets_lost_set(event, 157);
  evel_service_packet_loss_percent_set(event, 0.232);
  evel_service_r_factor_set(event, 11);
  evel_service_round_trip_delay_set(event, 15);
  evel_service_phone_number_set(event, "0888888888");
  evel_service_addl_field_add(event, "Name1", "Value1");
  evel_service_addl_field_add(event, "Name2", "Value2");
  evel_service_addl_field_add(event, "Name3", "Value3");
  evel_service_addl_field_add(event, "Name4", "Value4");
  json_size = evel_json_encode_event(
    json_body, EVEL_MAX_JSON_BODY, (EVENT_HEADER *) event);
  compare_strings(expected, json_body, EVEL_MAX_JSON_BODY, "Service");
  assert((json_size == strlen(json_body)) && "Bad size returned");

  evel_free_event(event);
  evel_throttle_terminate();
}

void test_encode_signaling_throttled()
{
  MEMORY_CHUNK post;

  /***************************************************************************/
  /* We also test suppression of the event header parameters here.           */
  /***************************************************************************/
  char * json_command_list =
    "{"
    "\"commandList\": ["
    "{"
    "\"command\": {"
    "\"commandType\": \"throttlingSpecification\", "
    "\"eventDomainThrottleSpecification\": {"
    "\"eventDomain\": \"signaling\", "
    "\"suppressedFieldNames\": ["
    "\"correlator\", "
    "\"eventType\", "
    "\"reportingEntityId\", "
    "\"sourceId\", "
    "\"localIpAddress\", "
    "\"localPort\", "
    "\"remoteIpAddress\", "
    "\"remotePort\", "
    "\"compressedSip\", "
    "\"summarySip\"], "
    "\"suppressedNvPairsList\": ["
    "]}}}"
    "]"
    "}";

  char * expected =
    "{\"event\": "
    "{\"commonEventHeader\": {"
    "\"domain\": \"signaling\", "
    "\"eventId\": \"2001\", "
    "\"functionalRole\": \"UNIT TEST\", "
    "\"lastEpochMicrosec\": 1000002, "
    "\"priority\": \"Normal\", "
    "\"reportingEntityName\": \"Dummy VM name - No Metadata available\", "
    "\"sequence\": 2001, "
    "\"sourceName\": \"Dummy VM name - No Metadata available\", "
    "\"startEpochMicrosec\": 1000002, "
    "\"version\": 1.2"
    "}, "
    "\"signalingFields\": {"
    "\"eventInstanceIdentifier\": "
    "{"
    "\"vendorId\": \"vendor_x_id\", "
    "\"eventId\": \"vendor_x_event_id\", "
    "\"productId\": \"vendor_x_product_id\", "
    "\"subsystemId\": \"vendor_x_subsystem_id\", "
    "\"eventFriendlyName\": \"vendor_x_frieldly_name\""
    "}, "
    "\"signalingFieldsVersion\": 1.1"
    "}}}";

  /***************************************************************************/
  /* Initialize and provide a specification with a single fault suppressed.  */
  /***************************************************************************/
  evel_throttle_initialize();
  handle_json_response(json_command_list, &post);

  /***************************************************************************/
  /* Check that the domain is throttled.                                     */
  /***************************************************************************/
  assert(evel_get_throttle_spec(EVEL_DOMAIN_SIGNALING) != NULL);
  assert(post.memory == NULL);

  size_t json_size = 0;
  char json_body[EVEL_MAX_JSON_BODY];
  EVENT_SIGNALING * event = NULL;
  evel_set_next_event_sequence(2001);
  event = evel_new_signaling("vendor_x_id",
           "correlator", "1.0.3.1", "1234", "192.168.1.3","3456");
  assert(event != NULL);
  evel_signaling_vnfmodule_name_set(event, "vendor_x_module");
  evel_signaling_vnfname_set(event, "vendor_x_vnf");
  evel_signaling_type_set(event, "Signaling");
  evel_signaling_product_id_set(event, "vendor_x_product_id");
  evel_signaling_subsystem_id_set(event, "vendor_x_subsystem_id");
  evel_signaling_friendly_name_set(event, "vendor_x_frieldly_name");
  evel_signaling_correlator_set(event, "vendor_x_correlator");
  evel_signaling_local_ip_address_set(event, "1.0.3.1");
  evel_signaling_local_port_set(event, "1031");
  evel_signaling_remote_ip_address_set(event, "5.3.3.0");
  evel_signaling_remote_port_set(event, "5330");
  evel_signaling_compressed_sip_set(event, "compressed_sip");
  evel_signaling_summary_sip_set(event, "summary_sip");
  json_size = evel_json_encode_event(
    json_body, EVEL_MAX_JSON_BODY, (EVENT_HEADER *) event);
  compare_strings(expected, json_body, EVEL_MAX_JSON_BODY, "Signaling");
  assert((json_size == strlen(json_body)) && "Bad size returned");

  evel_free_event(event);
  evel_throttle_terminate();
}

void test_encode_state_change_throttled()
{
  MEMORY_CHUNK post;

  /***************************************************************************/
  /* We also test suppression of the event header parameters here.           */
  /***************************************************************************/
  char * json_command_list =
    "{"
    "\"commandList\": ["
    "{"
    "\"command\": {"
    "\"commandType\": \"throttlingSpecification\", "
    "\"eventDomainThrottleSpecification\": {"
    "\"eventDomain\": \"stateChange\", "
    "\"suppressedFieldNames\": ["
    "\"eventType\", "
    "\"reportingEntityId\", "
    "\"sourceId\"], "
    "\"suppressedNvPairsList\": ["
    "{"
    "\"nvPairFieldName\": \"additionalFields\", "
    "\"suppressedNvPairNames\": [\"Name1\"]"
    "},"
    "]}}}"
    "]"
    "}";

  char * expected =
    "{\"event\": "
    "{\"commonEventHeader\": {"
    "\"domain\": \"stateChange\", "
    "\"eventId\": \"128\", "
    "\"functionalRole\": \"UNIT TEST\", "
    "\"lastEpochMicrosec\": 1000002, "
    "\"priority\": \"Normal\", "
    "\"reportingEntityName\": \"Dummy VM name - No Metadata available\", "
    "\"sequence\": 128, "
    "\"sourceName\": \"Dummy VM name - No Metadata available\", "
    "\"startEpochMicrosec\": 1000002, "
    "\"version\": 1.2"
    "}, "
    "\"stateChangeFields\": {"
    "\"newState\": \"inService\", "
    "\"oldState\": \"outOfService\", "
    "\"stateInterface\": \"An Interface\", "
    "\"additionalFields\": ["
    "{\"name\": \"Name2\", "
    "\"value\": \"Value2\"}"
    "], "
    "\"stateChangeFieldsVersion\": 1.1"
    "}}}";

  /***************************************************************************/
  /* Initialize and provide a specification with a single fault suppressed.  */
  /***************************************************************************/
  evel_throttle_initialize();
  handle_json_response(json_command_list, &post);

  /***************************************************************************/
  /* Check that the domain is throttled.                                     */
  /***************************************************************************/
  assert(evel_get_throttle_spec(EVEL_DOMAIN_STATE_CHANGE) != NULL);
  assert(post.memory == NULL);

  size_t json_size = 0;
  char json_body[EVEL_MAX_JSON_BODY];
  EVENT_STATE_CHANGE * state_change = NULL;
  evel_set_next_event_sequence(128);
  state_change = evel_new_state_change(EVEL_ENTITY_STATE_IN_SERVICE,
                                       EVEL_ENTITY_STATE_OUT_OF_SERVICE,
                                       "An Interface");
  assert(state_change != NULL);
  evel_state_change_type_set(state_change, "SC Type");
  evel_state_change_addl_field_add(state_change, "Name1", "Value1");
  evel_state_change_addl_field_add(state_change, "Name2", "Value2");

  json_size = evel_json_encode_event(
    json_body, EVEL_MAX_JSON_BODY, (EVENT_HEADER *) state_change);
  compare_strings(expected, json_body, EVEL_MAX_JSON_BODY, "StateChange");
  assert((json_size == strlen(json_body)) && "Bad size returned");

  evel_free_event(state_change);
  evel_throttle_terminate();
}

void test_encode_syslog_throttled()
{
  MEMORY_CHUNK post;

  /***************************************************************************/
  /* We also test suppression of the event header parameters here.           */
  /***************************************************************************/
  char * json_command_list =
    "{"
    "\"commandList\": ["
    "{"
    "\"command\": {"
    "\"commandType\": \"throttlingSpecification\", "
    "\"eventDomainThrottleSpecification\": {"
    "\"eventDomain\": \"syslog\", "
    "\"suppressedFieldNames\": ["
    "\"eventSourceHost\", "
    "\"syslogFacility\", "
    "\"syslogProc\", "
    "\"syslogProcId\", "
    "\"syslogSData\", "
    "\"syslogVer\", "
    "\"eventType\", "
    "\"reportingEntityId\", "
    "\"sourceId\"], "
    "\"suppressedNvPairsList\": ["
    "{"
    "\"nvPairFieldName\": \"additionalFields\", "
    "\"suppressedNvPairNames\": [\"Name2\"]"
    "},"
    "]}}}"
    "]"
    "}";

  char * expected =
    "{\"event\": "
    "{\"commonEventHeader\": {"
    "\"domain\": \"syslog\", "
    "\"eventId\": \"126\", "
    "\"functionalRole\": \"UNIT TEST\", "
    "\"lastEpochMicrosec\": 1000002, "
    "\"priority\": \"Normal\", "
    "\"reportingEntityName\": \"Dummy VM name - No Metadata available\", "
    "\"sequence\": 126, "
    "\"sourceName\": \"Dummy VM name - No Metadata available\", "
    "\"startEpochMicrosec\": 1000002, "
    "\"version\": 1.2"
    "}, "
    "\"syslogFields\": {"
    "\"eventSourceType\": \"virtualNetworkFunction\", "
    "\"syslogMsg\": \"SL Message\", "
    "\"syslogTag\": \"SL Tag\", "
    "\"syslogFieldsVersion\": 1.1, "
    "\"additionalFields\": ["
    "{\"name\": \"Name1\", "
    "\"value\": \"Value1\"}"
    "]"
    "}}}";

  /***************************************************************************/
  /* Initialize and provide a specification with a single fault suppressed.  */
  /***************************************************************************/
  evel_throttle_initialize();
  handle_json_response(json_command_list, &post);

  /***************************************************************************/
  /* Check that the domain is throttled.                                     */
  /***************************************************************************/
  assert(evel_get_throttle_spec(EVEL_DOMAIN_SYSLOG) != NULL);
  assert(post.memory == NULL);

  size_t json_size = 0;
  char json_body[EVEL_MAX_JSON_BODY];
  EVENT_SYSLOG * syslog = NULL;
  evel_set_next_event_sequence(126);
  syslog = evel_new_syslog(EVEL_SOURCE_VIRTUAL_NETWORK_FUNCTION,
                           "SL Message",
                           "SL Tag");
  assert(syslog != NULL);
  evel_syslog_type_set(syslog, "SL Type");
  evel_syslog_event_source_host_set(syslog, "SL Host");
  evel_syslog_facility_set(syslog, EVEL_SYSLOG_FACILITY_LINE_PRINTER);
  evel_syslog_proc_set(syslog, "SL Proc");
  evel_syslog_proc_id_set(syslog, 2);
  evel_syslog_version_set(syslog, 1);
  evel_syslog_s_data_set(syslog, "SL SDATA");
  evel_syslog_addl_field_add(syslog, "Name1", "Value1");
  evel_syslog_addl_field_add(syslog, "Name2", "Value2");

  json_size = evel_json_encode_event(
    json_body, EVEL_MAX_JSON_BODY, (EVENT_HEADER *) syslog);
  compare_strings(expected, json_body, EVEL_MAX_JSON_BODY, "Syslog");
  assert((json_size == strlen(json_body)) && "Bad size returned");

  evel_free_event(syslog);
  evel_throttle_terminate();
}

void test_encode_fault_with_escaping()
{
  char * expected =
    "{\"event\": {"
    "\"commonEventHeader\": {"
    "\"domain\": \"fault\", "
    "\"eventId\": \"122\", "
    "\"functionalRole\": \"UNIT TEST\", "
    "\"lastEpochMicrosec\": 1000002, "
    "\"priority\": \"Normal\", "
    "\"reportingEntityName\": \"Dummy VM name - No Metadata available\", "
    "\"sequence\": 122, "
    "\"sourceName\": \"Dummy VM name - No Metadata available\", "
    "\"startEpochMicrosec\": 1000002, "
    "\"version\": 1.2, "
    "\"eventType\": \"Bad things happen...\\\\\", "
    "\"reportingEntityId\": \"Dummy VM UUID - No Metadata available\", "
    "\"sourceId\": \"Dummy VM UUID - No Metadata available\""
    "}, "
    "\"faultFields\": {"
    "\"alarmCondition\": \"My alarm condition\", "
    "\"eventSeverity\": \"MAJOR\", "
    "\"eventSourceType\": \"other\", "
    "\"specificProblem\": \"It broke \\\"very\\\" badly\", "
    "\"vfStatus\": \"Active\", "
    "\"faultFieldsVersion\": 1.1, "
    "\"alarmAdditionalInformation\": ["
    "{\"name\": \"name1\", "
    "\"value\": \"value1\"}, "
    "{\"name\": \"name2\", "
    "\"value\": \"value2\"}], "
    "\"alarmInterfaceA\": \"My Interface Card\""
    "}}}";

  size_t json_size = 0;
  char json_body[EVEL_MAX_JSON_BODY];
  evel_set_next_event_sequence(122);
  EVENT_FAULT * fault = evel_new_fault("My alarm condition",
                                       "It broke \"very\" badly",
                                       EVEL_PRIORITY_NORMAL,
                                       EVEL_SEVERITY_MAJOR,
					EVEL_SOURCE_HOST,
                          EVEL_VF_STATUS_PREP_TERMINATE);
  assert(fault != NULL);
  evel_fault_type_set(fault, "Bad things happen...\\");
  evel_fault_interface_set(fault, "My Interface Card");
  evel_fault_addl_info_add(fault, "name1", "value1");
  evel_fault_addl_info_add(fault, "name2", "value2");

  json_size = evel_json_encode_event(
    json_body, EVEL_MAX_JSON_BODY, (EVENT_HEADER *) fault);
  compare_strings(expected, json_body, EVEL_MAX_JSON_BODY, "Fault");
  assert((json_size == strlen(json_body)) && "Bad size returned");

  evel_free_event(fault);
}
