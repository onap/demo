/**************************************************************************//**
 * @file
 * Utility to post test control commands to the test_collector testControl API.
 *
 * This software is a test capability, allowing test cases to trigger
 * expected throttling behaviours at the test collector, for automated test
 * capabilty.
 *
 * License
 * -------
 *
 * Copyright(c) <2016>, AT&T Intellectual Property.  All other rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:  This product includes software
 * developed by the AT&T.
 * 4. Neither the name of AT&T nor the names of its contributors may be used to
 * endorse or promote products derived from this software without specific
 * prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY AT&T INTELLECTUAL PROPERTY ''AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL AT&T INTELLECTUAL PROPERTY BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *****************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <assert.h>

#include "evel_test_control.h"
#include "evel_internal.h"                               /* For MEMORY_CHUNK */

/*****************************************************************************/
/* Local prototypes.                                                         */
/*****************************************************************************/
static size_t read_callback(void *ptr, size_t size, size_t nmemb, void *userp);

/**************************************************************************//**
 * POST provide JSON to the test_collector testControl API.
 *
 * This function does not take ownership of the json_buffer passed in.
 *
 * @param json_buffer   Pointer to the JSON to POST
 * @param json_size     The number of bytes to POST
 * @param secure  Whether to use HTTPS (0=HTTP, 1=HTTPS)
 * @param fqdn          The test control API FQDN or IP address.
 * @param port          The test control API port.
 *****************************************************************************/
void evel_test_control(char * const json_buffer,
                       const int json_size,
                       const int secure,
                       const char * fqdn,
                       const int port)
{
  CURLcode curl_rc = CURLE_OK;
  char curl_err_string[CURL_ERROR_SIZE] = "<NULL>";

  /***************************************************************************/
  /* Get a curl handle.                                                      */
  /***************************************************************************/
  CURL * curl_handle = curl_easy_init();
  assert(curl_handle != NULL);

  /***************************************************************************/
  /* Prime the library to give friendly error codes.                         */
  /***************************************************************************/
  curl_rc = curl_easy_setopt(curl_handle,
                             CURLOPT_ERRORBUFFER,
                             curl_err_string);
  assert(curl_rc == CURLE_OK);

  /***************************************************************************/
  /* Build and set the testControl API URL.                                  */
  /***************************************************************************/
  char version_string[10] = {0};
  int offset = sprintf(version_string, "%d", EVEL_API_MAJOR_VERSION);
  if (EVEL_API_MINOR_VERSION != 0)
  {
    sprintf(version_string + offset, ".%d", EVEL_API_MINOR_VERSION);
  }
  char test_control_url[EVEL_MAX_URL_LEN + 1] = {0};
  snprintf(test_control_url,
           EVEL_MAX_URL_LEN,
           "%s://%s:%d/testControl/v%s/commandList",
           secure ? "https" : "http",
           fqdn,
           port,
           version_string);
  curl_rc = curl_easy_setopt(curl_handle, CURLOPT_URL, test_control_url);
  assert(curl_rc == CURLE_OK);

  /***************************************************************************/
  /* Some servers don't like requests that are made without a user-agent     */
  /* field, so we provide one.                                               */
  /***************************************************************************/
  curl_rc = curl_easy_setopt(curl_handle,
                             CURLOPT_USERAGENT,
                             "libcurl-agent/1.0");
  assert(curl_rc == CURLE_OK);

  /***************************************************************************/
  /* Specify that we are going to POST data.                                 */
  /***************************************************************************/
  curl_rc = curl_easy_setopt(curl_handle, CURLOPT_POST, 1L);
  assert(curl_rc == CURLE_OK);

  /***************************************************************************/
  /* We want to use our own read function.                                   */
  /***************************************************************************/
  curl_rc = curl_easy_setopt(curl_handle, CURLOPT_READFUNCTION, read_callback);
  assert(curl_rc == CURLE_OK);

  /***************************************************************************/
  /* All of our events are JSON encoded.  We also suppress the               */
  /* Expect: 100-continue   header that we would otherwise get since it      */
  /* confuses some servers.                                                  */
  /***************************************************************************/
  static struct curl_slist * hdr_chunk = NULL;
  hdr_chunk = curl_slist_append(hdr_chunk, "Content-type: application/json");
  hdr_chunk = curl_slist_append(hdr_chunk, "Expect:");

  /***************************************************************************/
  /* Set our custom set of headers.                                         */
  /***************************************************************************/
  curl_rc = curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, hdr_chunk);
  assert(curl_rc == CURLE_OK);

  /***************************************************************************/
  /* Set the timeout for the operation.                                      */
  /***************************************************************************/
  const int TEST_CTRL_TIMEOUT = 2;
  curl_rc = curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, TEST_CTRL_TIMEOUT);
  assert(curl_rc == CURLE_OK);

  /***************************************************************************/
  /* Create a common pointer to pass to our read function, on stack.         */
  /***************************************************************************/
  MEMORY_CHUNK tx_chunk = {json_buffer, json_size};
  curl_rc = curl_easy_setopt(curl_handle, CURLOPT_READDATA, &tx_chunk);
  assert(curl_rc == CURLE_OK);

  /***************************************************************************/
  /* Set transmit size.                                                      */
  /***************************************************************************/
  curl_rc = curl_easy_setopt(curl_handle,
                             CURLOPT_POSTFIELDSIZE,
                             tx_chunk.size);
  assert(curl_rc == CURLE_OK);

  /***************************************************************************/
  /* Perform the POST.                                                       */
  /***************************************************************************/
  curl_rc = curl_easy_perform(curl_handle);
  assert(curl_rc == CURLE_OK);

  /***************************************************************************/
  /* Shut down the cURL library in a tidy manner.                            */
  /***************************************************************************/
  curl_easy_cleanup(curl_handle);

  EVEL_EXIT();
}

/**************************************************************************//**
 * Callback function to provide POST data.
 *
 * Copy data into the supplied buffer, read_callback::ptr, checking size
 * limits.
 *
 * @returns   Number of bytes placed into read_callback::ptr. 0 for EOF.
 *****************************************************************************/
static size_t read_callback(void *ptr, size_t size, size_t nmemb, void *userp)
{
  size_t rtn = 0;
  size_t bytes_to_write = 0;
  MEMORY_CHUNK * tx_chunk = (MEMORY_CHUNK *)userp;

  EVEL_ENTER();

  bytes_to_write = min(size * nmemb, tx_chunk->size);

  if (bytes_to_write > 0)
  {
    strncpy((char *)ptr, tx_chunk->memory, bytes_to_write);
    tx_chunk->memory += bytes_to_write;
    tx_chunk->size -= bytes_to_write;
    rtn = bytes_to_write;
  }

  EVEL_EXIT();

  return rtn;
}

/**************************************************************************//**
 * POST a pre-set test scenario to the test_collector testControl API.
 *
 * This function provides various pre-configured scenarios, purely to avoid
 * duplicating them elsewhere.
 *
 * @param scenario      The scenario to POST.
 * @param secure        Whether to use HTTPS (0=HTTP, 1=HTTPS)
 * @param fqdn          The test control API FQDN or IP address.
 * @param port          The test control API port.
 *****************************************************************************/
void evel_test_control_scenario(const EVEL_TEST_CONTROL_SCENARIO scenario,
                                const int secure,
                                const char * fqdn,
                                const int port)
{
  const int MAX_JSON = 10000;
  char json_buffer[MAX_JSON];
  int json_size = 0;

  EVEL_ENTER();

  switch (scenario)
  {
    case TC_RESET_ALL_DOMAINS:
      json_size += snprintf(
        json_buffer + json_size,
        MAX_JSON - json_size,
        "{"
        "  \"commandList\": ["
        "    {"
        "      \"command\": {"
        "        \"commandType\": \"throttlingSpecification\","
        "        \"eventDomainThrottleSpecification\": {"
        "          \"eventDomain\": \"fault\""
        "        }"
        "      }"
        "    },"
        "    {"
        "      \"command\": {"
        "        \"commandType\": \"throttlingSpecification\","
        "        \"eventDomainThrottleSpecification\": {"
        "          \"eventDomain\": \"measurementsForVfScaling\""
        "        }"
        "      }"
        "    },"
        "    {"
        "      \"command\": {"
        "        \"commandType\": \"throttlingSpecification\","
        "        \"eventDomainThrottleSpecification\": {"
        "          \"eventDomain\": \"mobileFlow\""
        "        }"
        "      }"
        "    },"
        "    {"
        "      \"command\": {"
        "        \"commandType\": \"throttlingSpecification\","
        "        \"eventDomainThrottleSpecification\": {"
        "          \"eventDomain\": \"serviceEvents\""
        "        }"
        "      }"
        "    },"
        "    {"
        "      \"command\": {"
        "        \"commandType\": \"throttlingSpecification\","
        "        \"eventDomainThrottleSpecification\": {"
        "          \"eventDomain\": \"signaling\""
        "        }"
        "      }"
        "    },"
        "    {"
        "      \"command\": {"
        "        \"commandType\": \"throttlingSpecification\","
        "        \"eventDomainThrottleSpecification\": {"
        "          \"eventDomain\": \"stateChange\""
        "        }"
        "      }"
        "    },"
        "    {"
        "      \"command\": {"
        "        \"commandType\": \"throttlingSpecification\","
        "        \"eventDomainThrottleSpecification\": {"
        "          \"eventDomain\": \"syslog\""
        "        }"
        "      }"
        "    }"
        "  ]"
        "}");
      break;

    case TC_FAULT_SUPPRESS_FIELDS:
      json_size += snprintf(
        json_buffer + json_size,
        MAX_JSON - json_size,
        "{"
        "  \"commandList\": ["
        "    {"
        "      \"command\": {"
        "        \"commandType\": \"throttlingSpecification\","
        "        \"eventDomainThrottleSpecification\": {"
        "          \"suppressedFieldNames\": ["
        "            \"alarmInterfaceA\","
        "            \"alarmAdditionalInformation\""
        "          ],"
        "          \"eventDomain\": \"fault\""
        "        }"
        "      }"
        "    }"
        "  ]"
        "}");
      break;

    case TC_FAULT_SUPPRESS_FIELDS_AND_PAIRS:
      json_size += snprintf(
        json_buffer + json_size,
        MAX_JSON - json_size,
        "{"
        "  \"commandList\": ["
        "    {"
        "      \"command\": {"
        "        \"commandType\": \"throttlingSpecification\","
        "        \"eventDomainThrottleSpecification\": {"
        "          \"suppressedNvPairsList\": ["
        "            {"
        "              \"nvPairFieldName\": \"alarmAdditionalInformation\","
        "              \"suppressedNvPairNames\": ["
        "                \"name1\","
        "                \"name2\""
        "              ]"
        "            }"
        "          ],"
        "          \"suppressedFieldNames\": ["
        "            \"alarmInterfaceA\""
        "          ],"
        "          \"eventDomain\": \"fault\""
        "        }"
        "      }"
        "    }"
        "  ]"
        "}");
      break;

    case TC_FAULT_SUPPRESS_NOTHING:
      json_size += snprintf(
        json_buffer + json_size,
        MAX_JSON - json_size,
        "{"
        "  \"commandList\": ["
        "    {"
        "      \"command\": {"
        "        \"commandType\": \"throttlingSpecification\","
        "        \"eventDomainThrottleSpecification\": {"
        "          \"eventDomain\": \"fault\""
        "        }"
        "      }"
        "    }"
        "  ]"
        "}");
      break;

    case TC_FAULT_SUPPRESS_PAIRS:
      json_size += snprintf(
        json_buffer + json_size,
        MAX_JSON - json_size,
        "{"
        "  \"commandList\": ["
        "    {"
        "      \"command\": {"
        "        \"commandType\": \"throttlingSpecification\","
        "        \"eventDomainThrottleSpecification\": {"
        "          \"suppressedNvPairsList\": ["
        "            {"
        "              \"nvPairFieldName\": \"alarmAdditionalInformation\","
        "              \"suppressedNvPairNames\": ["
        "                \"name1\","
        "                \"name2\""
        "              ]"
        "            }"
        "          ],"
        "          \"eventDomain\": \"fault\""
        "        }"
        "      }"
        "    }"
        "  ]"
        "}");
      break;

    case TC_MEAS_SUPPRESS_FIELDS_AND_PAIRS:
      json_size += snprintf(
        json_buffer + json_size,
        MAX_JSON - json_size,
        "{"
        "  \"commandList\": ["
        "    {"
        "      \"command\": {"
        "        \"commandType\": \"throttlingSpecification\","
        "        \"eventDomainThrottleSpecification\": {"
        "          \"suppressedNvPairsList\": ["
        "            {"
        "              \"nvPairFieldName\": \"cpuUsageArray\","
        "              \"suppressedNvPairNames\": ["
        "                \"cpu1\","
        "                \"cpu3\""
        "              ]"
        "            }"
        "          ],"
        "          \"suppressedFieldNames\": ["
        "            \"numberOfMediaPortsInUse\""
        "          ],"
        "          \"eventDomain\": \"measurementsForVfScaling\""
        "        }"
        "      }"
        "    }"
        "  ]"
        "}");
      break;

    case TC_MOBILE_SUPPRESS_FIELDS_AND_PAIRS:
      json_size += snprintf(
        json_buffer + json_size,
        MAX_JSON - json_size,
        "{"
        "  \"commandList\": ["
        "    {"
        "      \"command\": {"
        "        \"commandType\": \"throttlingSpecification\","
        "        \"eventDomainThrottleSpecification\": {"
        "          \"suppressedFieldNames\": ["
        "            \"radioAccessTechnology\","
        "            \"samplingAlgorithm\""
        "          ],"
        "          \"eventDomain\": \"mobileFlow\""
        "        }"
        "      }"
        "    }"
        "  ]"
        "}");
      break;

    case TC_SERVICE_SUPPRESS_FIELDS_AND_PAIRS:
      json_size += snprintf(
        json_buffer + json_size,
        MAX_JSON - json_size,
        "{"
        "  \"commandList\": ["
        "    {"
        "      \"command\": {"
        "        \"commandType\": \"throttlingSpecification\","
        "        \"eventDomainThrottleSpecification\": {"
        "          \"suppressedNvPairsList\": ["
        "            {"
        "              \"nvPairFieldName\": \"additionalFields\","
        "              \"suppressedNvPairNames\": ["
        "                \"Name1\","
        "                \"Name3\""
        "              ]"
        "            }"
        "          ],"
        "          \"suppressedFieldNames\": ["
        "            \"reportingEntityId\","
        "            \"eventType\","
        "            \"sourceId\","
        "            \"midCallRtcp\","
        "            \"endOfCallVqmSummaries\","
        "            \"marker\""
        "          ],"
        "          \"eventDomain\": \"serviceEvents\""
        "        }"
        "      }"
        "    }"
        "  ]"
        "}");
      break;

    case TC_SIGNALING_SUPPRESS_FIELDS:
      json_size += snprintf(
        json_buffer + json_size,
        MAX_JSON - json_size,
        "{"
        "  \"commandList\": ["
        "    {"
        "      \"command\": {"
        "        \"commandType\": \"throttlingSpecification\","
        "        \"eventDomainThrottleSpecification\": {"
        "          \"suppressedFieldNames\": ["
        "            \"reportingEntityId\","
        "            \"eventType\","
        "            \"sourceId\","
        "            \"localIpAddress\","
        "            \"localIpPort\","
        "            \"remoteIpAddress\","
        "            \"remotePort\","
        "            \"compressedSip\""
        "          ],"
        "          \"eventDomain\": \"signaling\""
        "        }"
        "      }"
        "    }"
        "  ]"
        "}");
      break;

    case TC_STATE_SUPPRESS_FIELDS_AND_PAIRS:
      json_size += snprintf(
        json_buffer + json_size,
        MAX_JSON - json_size,
        "{"
        "  \"commandList\": ["
        "    {"
        "      \"command\": {"
        "        \"commandType\": \"throttlingSpecification\","
        "        \"eventDomainThrottleSpecification\": {"
        "          \"suppressedNvPairsList\": ["
        "            {"
        "              \"nvPairFieldName\": \"additionalFields\","
        "              \"suppressedNvPairNames\": ["
        "                \"Name1\""
        "              ]"
        "            }"
        "          ],"
        "          \"suppressedFieldNames\": ["
        "            \"reportingEntityId\","
        "            \"eventType\","
        "            \"sourceId\""
        "          ],"
        "          \"eventDomain\": \"stateChange\""
        "        }"
        "      }"
        "    }"
        "  ]"
        "}");
      break;

    case TC_SYSLOG_SUPPRESS_FIELDS_AND_PAIRS:
      json_size += snprintf(
        json_buffer + json_size,
        MAX_JSON - json_size,
        "{"
        "  \"commandList\": ["
        "    {"
        "      \"command\": {"
        "        \"commandType\": \"throttlingSpecification\","
        "        \"eventDomainThrottleSpecification\": {"
        "          \"suppressedNvPairsList\": ["
        "            {"
        "              \"nvPairFieldName\": \"additionalFields\","
        "              \"suppressedNvPairNames\": ["
        "                \"Name1\","
        "                \"Name4\""
        "              ]"
        "            }"
        "          ],"
        "          \"suppressedFieldNames\": ["
        "            \"syslogFacility\","
        "            \"syslogProc\","
        "            \"syslogProcId\""
        "          ],"
        "          \"eventDomain\": \"syslog\""
        "        }"
        "      }"
        "    }"
        "  ]"
        "}");
      break;

    case TC_PROVIDE_THROTTLING_SPEC:
      json_size += snprintf(
        json_buffer + json_size,
        MAX_JSON - json_size,
        "{"
        "  \"commandList\": ["
        "    {"
        "      \"command\": {"
        "        \"commandType\": \"provideThrottlingState\""
        "      }"
        "    }"
        "  ]"
        "}");
      break;

    default:
      break;
  }

  if (json_size != 0)
  {
    evel_test_control(json_buffer, json_size, secure, fqdn, port);
  }

  EVEL_EXIT();
}

/**************************************************************************//**
 * POST a measurement interval change to the test_collector testControl API.
 *
 * @param interval      The measurement interval.
 * @param secure        Whether to use HTTPS (0=HTTP, 1=HTTPS)
 * @param fqdn          The test control API FQDN or IP address.
 * @param port          The test control API port.
 *****************************************************************************/
void evel_test_control_meas_interval(const int interval,
                                     const int secure,
                                     const char * fqdn,
                                     const int port)
{
  const int MAX_JSON = 10000;
  char json_buffer[MAX_JSON];
  int json_size = 0;

  EVEL_ENTER();

  json_size += snprintf(
    json_buffer + json_size,
    MAX_JSON - json_size,
    "{\"commandList\": [{\"command\": "
    "{\"commandType\": \"measurementIntervalChange\", "
    "\"measurementInterval\": %d}}]}",
    interval);
  evel_test_control(json_buffer, json_size, secure, fqdn, port);

  EVEL_EXIT();
}
