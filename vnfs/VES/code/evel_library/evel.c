/**************************************************************************//**
 * @file
 * Source module isolating the ECOMP Vendor Event Listener (EVEL) API.
 *
 * This file implements the EVEL library which is intended to provide a
 * simple wrapper around the complexity of AT&T's Vendor Event Listener API so
 * that VNFs can use it without worrying about details of:
 *
 *  *   The API's encoding into JSON.
 *  *   The API's transport over HTTP/HTTPS.
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

#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/time.h>
#include <curl/curl.h>

#include "evel.h"
#include "evel_internal.h"
#include "evel_throttle.h"
#include "metadata.h"

/**************************************************************************//**
 * The type of equipment represented by this VNF.
 *****************************************************************************/
EVEL_SOURCE_TYPES event_source_type = EVEL_SOURCE_OTHER;

/**************************************************************************//**
 * The Functional Role of the equipment represented by this VNF.
 *****************************************************************************/
char *functional_role = NULL;

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
 * @param   secure  Whether to use HTTPS (0=HTTP, 1=HTTPS)
 * @param   username  Username for Basic Authentication of requests.
 * @param   password  Password for Basic Authentication of requests.
 * @param   source_type The kind of node we represent.
 * @param   role    The role this node undertakes.
 * @param   verbosity  0 for normal operation, positive values for chattier
 *                        logs.
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
                               )
{
  EVEL_ERR_CODES rc = EVEL_SUCCESS;
  char base_api_url[EVEL_MAX_URL_LEN + 1] = {0};
  char event_api_url[EVEL_MAX_URL_LEN + 1] = {0};
  char throt_api_url[EVEL_MAX_URL_LEN + 1] = {0};
  char path_url[EVEL_MAX_URL_LEN + 1] = {0};
  char topic_url[EVEL_MAX_URL_LEN + 1] = {0};
  char version_string[10] = {0};
  int offset;

  /***************************************************************************/
  /* Check assumptions.                                                      */
  /***************************************************************************/
  assert(fqdn != NULL);
  assert(port > 0 && port <= 65535);
  assert(source_type < EVEL_MAX_SOURCE_TYPES);
  assert(role != NULL);

  /***************************************************************************/
  /* Start logging so we can report on progress.                             */
  /***************************************************************************/
  log_initialize(verbosity == 0 ? EVEL_LOG_INFO : EVEL_LOG_DEBUG, "EVEL");
  EVEL_INFO("EVEL started");
  EVEL_INFO("API server is: %s", fqdn);
  EVEL_INFO("API port is: %d", port);

  if (path != NULL)
  {
    EVEL_INFO("API path is: %s", path);
  }
  else
  {
    EVEL_INFO("No API path");
  }

  if (topic != NULL)
  {
    EVEL_INFO("API topic is: %s", topic);
  }
  else
  {
    EVEL_INFO("No API topic");
  }

  EVEL_INFO("API transport is: %s", secure ? "HTTPS" : "HTTP");
  EVEL_INFO("Event Source Type is: %d", source_type);
  EVEL_INFO("Functional Role is: %s", role);
  EVEL_INFO("Log verbosity is: %d", verbosity);

  /***************************************************************************/
  /* Initialize event throttling to the default state.                       */
  /***************************************************************************/
  evel_throttle_initialize();

  /***************************************************************************/
  /* Save values we will need during operation.                              */
  /***************************************************************************/
  event_source_type = source_type;
  functional_role = strdup(role);

  /***************************************************************************/
  /* Ensure there are no trailing zeroes and unnecessary decimal points in   */
  /* the version.                                                            */
  /***************************************************************************/
  offset = sprintf(version_string, "%d", EVEL_API_MAJOR_VERSION);

  if (EVEL_API_MINOR_VERSION != 0)
  {
    sprintf(version_string + offset, ".%d", EVEL_API_MINOR_VERSION);
  }

  /***************************************************************************/
  /* Build a common base of the API URLs.                                    */
  /***************************************************************************/
  strcpy(path_url, "/");
  snprintf(base_api_url,
           EVEL_MAX_URL_LEN,
           "%s://%s:%d%s/eventListener/v%s",
           secure ? "https" : "http",
           fqdn,
           port,
           (((path != NULL) && (strlen(path) > 0)) ?
            strncat(path_url, path, EVEL_MAX_URL_LEN) : ""),
           version_string);

  /***************************************************************************/
  /* Build the URL to the event API.                                         */
  /***************************************************************************/
  strcpy(topic_url, "/");
  snprintf(event_api_url,
           EVEL_MAX_URL_LEN,
           "%s%s",
           base_api_url,
           (((topic != NULL) && (strlen(topic) > 0)) ?
            strncat(topic_url, topic, EVEL_MAX_URL_LEN) : ""));
  EVEL_INFO("Vendor Event Listener API is located at: %s", event_api_url);

  /***************************************************************************/
  /* Build the URL to the throttling API.                                    */
  /***************************************************************************/
  snprintf(throt_api_url,
           EVEL_MAX_URL_LEN,
           "%s/clientThrottlingState",
           base_api_url);
  EVEL_INFO("Vendor Event Throttling API is located at: %s", throt_api_url);

  /***************************************************************************/
  /* Spin-up the event-handler, which gets cURL readied for use.             */
  /***************************************************************************/
  rc = event_handler_initialize(event_api_url,
                                throt_api_url,
                                username,
                                password,
                                verbosity);
  if (rc != EVEL_SUCCESS)
  {
    log_error_state("Failed to initialize event handler (including cURL)");
    goto exit_label;
  }

  /***************************************************************************/
  /* Extract the metadata from OpenStack. If we fail to extract it, we       */
  /* record that in the logs, but carry on, assuming we're in a test         */
  /* without a metadata service.                                             */
  /***************************************************************************/
  rc = openstack_metadata(verbosity);
  if (rc != EVEL_SUCCESS)
  {
    EVEL_INFO("Failed to load OpenStack metadata - assuming test environment");
    rc = EVEL_SUCCESS;
  }

  /***************************************************************************/
  /* Start the event handler thread.                                         */
  /***************************************************************************/
  rc = event_handler_run();
  if (rc != EVEL_SUCCESS)
  {
    log_error_state("Failed to start event handler thread. "
                    "Error code=%d", rc);
    goto exit_label;
  }

exit_label:
  return(rc);
}

/**************************************************************************//**
 * Clean up the EVEL library.
 *
 * @note that at present don't expect Init/Term cycling not to leak memory!
 *
 * @returns Status code
 * @retval  EVEL_SUCCESS On success
 * @retval  "One of ::EVEL_ERR_CODES" On failure.
 *****************************************************************************/
EVEL_ERR_CODES evel_terminate(void)
{
  int rc = EVEL_SUCCESS;

  /***************************************************************************/
  /* First terminate any pending transactions in the event-posting thread.   */
  /***************************************************************************/
  rc = event_handler_terminate();
  if (rc != EVEL_SUCCESS)
  {
    log_error_state("Failed to terminate EVEL library cleanly!");
  }

  /***************************************************************************/
  /* Shut down the Event Handler library in a tidy manner.                   */
  /***************************************************************************/
  curl_global_cleanup();

  /***************************************************************************/
  /* Clean up allocated memory.                                              */
  /***************************************************************************/
  free(functional_role);

  /***************************************************************************/
  /* Clean up event throttling.                                              */
  /***************************************************************************/
  evel_throttle_terminate();

  EVEL_INFO("EVEL stopped");
  return(rc);
}

/**************************************************************************//**
 * Free an event.
 *
 * Free off the event supplied.  Will free all the contained allocated memory.
 *
 * @note  It is safe to free a NULL pointer.
 *****************************************************************************/
void evel_free_event(void * event)
{
  EVENT_HEADER * evt_ptr = event;
  EVEL_ENTER();

  if (event != NULL)
  {
    /*************************************************************************/
    /* Work out what kind of event we're dealing with so we can cast it      */
    /* appropriately.                                                        */
    /*************************************************************************/
    switch (evt_ptr->event_domain)
    {
    case EVEL_DOMAIN_INTERNAL:
      EVEL_DEBUG("Event is an Internal event at %lp", evt_ptr);
      evel_free_internal_event((EVENT_INTERNAL *) evt_ptr);
      memset(evt_ptr, 0, sizeof(EVENT_INTERNAL));
      free(evt_ptr);
      break;

    case EVEL_DOMAIN_HEARTBEAT:
      EVEL_DEBUG("Event is a Heartbeat at %lp", evt_ptr);
      evel_free_header(evt_ptr);
      memset(evt_ptr, 0, sizeof(EVENT_HEADER));
      free(evt_ptr);
      break;

    case EVEL_DOMAIN_FAULT:
      EVEL_DEBUG("Event is a Fault at %lp", evt_ptr);
      evel_free_fault((EVENT_FAULT *)evt_ptr);
      memset(evt_ptr, 0, sizeof(EVENT_FAULT));
      free(evt_ptr);
      break;

    case EVEL_DOMAIN_MEASUREMENT:
      EVEL_DEBUG("Event is a Measurement at %lp", evt_ptr);
      evel_free_measurement((EVENT_MEASUREMENT *)evt_ptr);
      memset(evt_ptr, 0, sizeof(EVENT_MEASUREMENT));
      free(evt_ptr);
      break;

    case EVEL_DOMAIN_MOBILE_FLOW:
      EVEL_DEBUG("Event is a Mobile Flow at %lp", evt_ptr);
      evel_free_mobile_flow((EVENT_MOBILE_FLOW *)evt_ptr);
      memset(evt_ptr, 0, sizeof(EVENT_MOBILE_FLOW));
      free(evt_ptr);
      break;

    case EVEL_DOMAIN_REPORT:
      EVEL_DEBUG("Event is a Report at %lp", evt_ptr);
      evel_free_report((EVENT_REPORT *)evt_ptr);
      memset(evt_ptr, 0, sizeof(EVENT_REPORT));
      free(evt_ptr);
      break;

    case EVEL_DOMAIN_SERVICE:
      EVEL_DEBUG("Event is a Service Event at %lp", evt_ptr);
      evel_free_service((EVENT_SERVICE *)evt_ptr);
      memset(evt_ptr, 0, sizeof(EVENT_SERVICE));
      free(evt_ptr);
      break;

    case EVEL_DOMAIN_SIGNALING:
      EVEL_DEBUG("Event is a Signaling at %lp", evt_ptr);
      evel_free_signaling((EVENT_SIGNALING *)evt_ptr);
      memset(evt_ptr, 0, sizeof(EVENT_SIGNALING));
      free(evt_ptr);
      break;

    case EVEL_DOMAIN_STATE_CHANGE:
      EVEL_DEBUG("Event is a State Change at %lp", evt_ptr);
      evel_free_state_change((EVENT_STATE_CHANGE *)evt_ptr);
      memset(evt_ptr, 0, sizeof(EVENT_STATE_CHANGE));
      free(evt_ptr);
      break;

    case EVEL_DOMAIN_SYSLOG:
      EVEL_DEBUG("Event is a Syslog at %lp", evt_ptr);
      evel_free_syslog((EVENT_SYSLOG *)evt_ptr);
      memset(evt_ptr, 0, sizeof(EVENT_SYSLOG));
      free(evt_ptr);
      break;

    case EVEL_DOMAIN_OTHER:
      EVEL_DEBUG("Event is an Other at %lp", evt_ptr);
      evel_free_other((EVENT_OTHER *)evt_ptr);
      memset(evt_ptr, 0, sizeof(EVENT_OTHER));
      free(evt_ptr);
      break;

    default:
      EVEL_ERROR("Unexpected event domain (%d)", evt_ptr->event_domain);
      assert(0);
    }
  }
  EVEL_EXIT();
}
