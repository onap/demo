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
 * ECOMP is a trademark and service mark of AT&T Intellectual Property.
 ****************************************************************************/

/**************************************************************************//**
 * @file
 * Event Manager
 *
 * Simple event manager that is responsible for taking events (Heartbeats,
 * Faults and Measurements) from the ring-buffer and posting them to the API.
 *
 ****************************************************************************/

#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <pthread.h>

#include <curl/curl.h>

#include "evel.h"
#include "evel_internal.h"
#include "ring_buffer.h"
#include "evel_throttle.h"

/**************************************************************************//**
 * How long we're prepared to wait for the API service to respond in
 * seconds.
 *****************************************************************************/
static const int EVEL_API_TIMEOUT = 5;

/*****************************************************************************/
/* Prototypes of locally scoped functions.                                   */
/*****************************************************************************/
static size_t read_callback(void *ptr, size_t size, size_t nmemb, void *userp);
static void * event_handler(void *arg);
static bool evel_handle_response_tokens(const MEMORY_CHUNK * const chunk,
                                        const jsmntok_t * const json_tokens,
                                        const int num_tokens,
                                        MEMORY_CHUNK * const post);
static bool evel_tokens_match_command_list(const MEMORY_CHUNK * const chunk,
                                           const jsmntok_t * const json_token,
                                           const int num_tokens);
static bool evel_token_equals_string(const MEMORY_CHUNK * const chunk,
                                     const jsmntok_t * const json_token,
                                     const char * check_string);

/**************************************************************************//**
 * Buffers for error strings from libcurl.
 *****************************************************************************/
static char curl_err_string[CURL_ERROR_SIZE] = "<NULL>";

/**************************************************************************//**
 * Handle for the API into libcurl.
 *****************************************************************************/
static CURL * curl_handle = NULL;

/**************************************************************************//**
 * Special headers that we send.
 *****************************************************************************/
static struct curl_slist * hdr_chunk = NULL;

/**************************************************************************//**
 * Message queue for sending events to the API.
 *****************************************************************************/
static ring_buffer event_buffer;

/**************************************************************************//**
 * Single pending priority post, which can be generated as a result of a
 * response to an event.  Currently only used to respond to a commandList.
 *****************************************************************************/
static MEMORY_CHUNK priority_post;

/**************************************************************************//**
 * The thread which is responsible for handling events off of the ring-buffer
 * and posting them to the Event Handler API.
 *****************************************************************************/
static pthread_t evt_handler_thread;

/**************************************************************************//**
 * Variable to convey to the event handler thread what the foreground wants it
 * to do.
 *****************************************************************************/
static EVT_HANDLER_STATE evt_handler_state = EVT_HANDLER_UNINITIALIZED;

/**************************************************************************//**
 * The configured API URL for event and throttling.
 *****************************************************************************/
static char * evel_event_api_url;
static char * evel_throt_api_url;

/**************************************************************************//**
 * Initialize the event handler.
 *
 * Primarily responsible for getting CURL ready for use.
 *
 * @param[in] event_api_url
 *                      The URL where the Vendor Event Listener API is expected
 *                      to be.
 * @param[in] throt_api_url
 *                      The URL where the Throttling API is expected to be.
 * @param[in] username  The username for the Basic Authentication of requests.
 * @param[in] password  The password for the Basic Authentication of requests.
 * @param     verbosity 0 for normal operation, positive values for chattier
 *                        logs.
 *****************************************************************************/
EVEL_ERR_CODES event_handler_initialize(const char * const event_api_url,
                                        const char * const throt_api_url,
                                        const char * const username,
                                        const char * const password,
                                        int verbosity)
{
  int rc = EVEL_SUCCESS;
  CURLcode curl_rc = CURLE_OK;

  EVEL_ENTER();

  /***************************************************************************/
  /* Check assumptions.                                                      */
  /***************************************************************************/
  assert(event_api_url != NULL);
  assert(throt_api_url != NULL);
  assert(username != NULL);
  assert(password != NULL);

  /***************************************************************************/
  /* Store the API URLs.                                                     */
  /***************************************************************************/
  evel_event_api_url = strdup(event_api_url);
  assert(evel_event_api_url != NULL);
  evel_throt_api_url = strdup(throt_api_url);
  assert(evel_throt_api_url != NULL);


  curl_version_info_data *d = curl_version_info(CURLVERSION_NOW);
  /* compare with the 24 bit hex number in 8 bit fields */
  if(d->version_num >= 0x072100) {
     /* this is libcurl 7.33.0 or later */
     EVEL_INFO("7.33 or later Curl version %x.",d->version_num);
  }
  else {
     EVEL_INFO("Old Curl version.");
  }
  /***************************************************************************/
  /* Start the CURL library. Note that this initialization is not threadsafe */
  /* which imposes a constraint that the EVEL library is initialized before  */
  /* any threads are started.                                                */
  /***************************************************************************/
  curl_rc = curl_global_init(CURL_GLOBAL_SSL);
  if (curl_rc != CURLE_OK)
  {
    rc = EVEL_CURL_LIBRARY_FAIL;
    log_error_state("Failed to initialize libCURL. Error code=%d", curl_rc);
    goto exit_label;
  }

  /***************************************************************************/
  /* Get a curl handle which we'll use for all of our output.                */
  /***************************************************************************/
  curl_handle = curl_easy_init();
  if (curl_handle == NULL)
  {
    rc = EVEL_CURL_LIBRARY_FAIL;
    log_error_state("Failed to get libCURL handle");
    goto exit_label;
  }

  /***************************************************************************/
  /* Prime the library to give friendly error codes.                         */
  /***************************************************************************/
  curl_rc = curl_easy_setopt(curl_handle,
                             CURLOPT_ERRORBUFFER,
                             curl_err_string);
  if (curl_rc != CURLE_OK)
  {
    rc = EVEL_CURL_LIBRARY_FAIL;
    log_error_state("Failed to initialize libCURL to provide friendly errors. "
                    "Error code=%d", curl_rc);
    goto exit_label;
  }

  /***************************************************************************/
  /* If running in verbose mode generate more output.                        */
  /***************************************************************************/
  if (verbosity > 0)
  {
    curl_rc = curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 1L);
    if (curl_rc != CURLE_OK)
    {
      rc = EVEL_CURL_LIBRARY_FAIL;
      log_error_state("Failed to initialize libCURL to be verbose. "
                      "Error code=%d", curl_rc);
      goto exit_label;
    }
  }

  /***************************************************************************/
  /* Set the URL for the API.                                                */
  /***************************************************************************/
  curl_rc = curl_easy_setopt(curl_handle, CURLOPT_URL, event_api_url);
  if (curl_rc != CURLE_OK)
  {
    rc = EVEL_CURL_LIBRARY_FAIL;
    log_error_state("Failed to initialize libCURL with the API URL. "
                    "Error code=%d (%s)", curl_rc, curl_err_string);
    goto exit_label;
  }
  EVEL_INFO("Initializing CURL to send events to: %s", event_api_url);

  /***************************************************************************/
  /* send all data to this function.                                         */
  /***************************************************************************/
  curl_rc = curl_easy_setopt(curl_handle,
                             CURLOPT_WRITEFUNCTION,
                             evel_write_callback);
  if (curl_rc != CURLE_OK)
  {
    rc = EVEL_CURL_LIBRARY_FAIL;
    log_error_state("Failed to initialize libCURL with the write callback. "
                    "Error code=%d (%s)", curl_rc, curl_err_string);
    goto exit_label;
  }

  /***************************************************************************/
  /* some servers don't like requests that are made without a user-agent     */
  /* field, so we provide one.                                               */
  /***************************************************************************/
  curl_rc = curl_easy_setopt(curl_handle,
                             CURLOPT_USERAGENT,
                             "libcurl-agent/1.0");
  if (curl_rc != CURLE_OK)
  {
    rc = EVEL_CURL_LIBRARY_FAIL;
    log_error_state("Failed to initialize libCURL to upload. "
                    "Error code=%d (%s)", curl_rc, curl_err_string);
    goto exit_label;
  }

  /***************************************************************************/
  /* Specify that we are going to POST data.                                 */
  /***************************************************************************/
  curl_rc = curl_easy_setopt(curl_handle, CURLOPT_POST, 1L);
  if (curl_rc != CURLE_OK)
  {
    rc = EVEL_CURL_LIBRARY_FAIL;
    log_error_state("Failed to initialize libCURL to upload. "
                    "Error code=%d (%s)", curl_rc, curl_err_string);
    goto exit_label;
  }

  /***************************************************************************/
  /* we want to use our own read function.                                   */
  /***************************************************************************/
  curl_rc = curl_easy_setopt(curl_handle, CURLOPT_READFUNCTION, read_callback);
  if (curl_rc != CURLE_OK)
  {
    rc = EVEL_CURL_LIBRARY_FAIL;
    log_error_state("Failed to initialize libCURL to upload using read "
                    "function. Error code=%d (%s)", curl_rc, curl_err_string);
    goto exit_label;
  }

  /***************************************************************************/
  /* All of our events are JSON encoded.  We also suppress the               */
  /* Expect: 100-continue   header that we would otherwise get since it      */
  /* confuses some servers.                                                  */
  /*                                                                         */
  /* @TODO: do AT&T want this behavior?                                      */
  /***************************************************************************/
  hdr_chunk = curl_slist_append(hdr_chunk, "Content-type: application/json");
  hdr_chunk = curl_slist_append(hdr_chunk, "Expect:");

  /***************************************************************************/
  /* set our custom set of headers.                                         */
  /***************************************************************************/
  curl_rc = curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, hdr_chunk);
  if (curl_rc != CURLE_OK)
  {
    rc = EVEL_CURL_LIBRARY_FAIL;
    log_error_state("Failed to initialize libCURL to use custom headers. "
                    "Error code=%d (%s)", curl_rc, curl_err_string);
    goto exit_label;
  }

  /***************************************************************************/
  /* Set the timeout for the operation.                                      */
  /***************************************************************************/
  curl_rc = curl_easy_setopt(curl_handle,
                             CURLOPT_TIMEOUT,
                             EVEL_API_TIMEOUT);
  if (curl_rc != CURLE_OK)
  {
    rc = EVEL_CURL_LIBRARY_FAIL;
    log_error_state("Failed to initialize libCURL for API timeout. "
                    "Error code=%d (%s)", curl_rc, curl_err_string);
    goto exit_label;
  }

  /***************************************************************************/
  /* Set that we want Basic authentication with username:password Base-64    */
  /* encoded for the operation.                                              */
  /***************************************************************************/
  curl_rc = curl_easy_setopt(curl_handle, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
  if (curl_rc != CURLE_OK)
  {
    rc = EVEL_CURL_LIBRARY_FAIL;
    log_error_state("Failed to initialize libCURL for Basic Authentication. "
                    "Error code=%d (%s)", curl_rc, curl_err_string);
    goto exit_label;
  }
  curl_rc = curl_easy_setopt(curl_handle, CURLOPT_USERNAME, username);
  if (curl_rc != CURLE_OK)
  {
    rc = EVEL_CURL_LIBRARY_FAIL;
    log_error_state("Failed to initialize libCURL with username. "
                    "Error code=%d (%s)", curl_rc, curl_err_string);
    goto exit_label;
  }
  curl_rc = curl_easy_setopt(curl_handle, CURLOPT_PASSWORD, password);
  if (curl_rc != CURLE_OK)
  {
    rc = EVEL_CURL_LIBRARY_FAIL;
    log_error_state("Failed to initialize libCURL with password. "
                    "Error code=%d (%s)", curl_rc, curl_err_string);
    goto exit_label;
  }

  /***************************************************************************/
  /* Initialize a message ring-buffer to be used between the foreground and  */
  /* the thread which sends the messages.  This can't fail.                  */
  /***************************************************************************/
  ring_buffer_initialize(&event_buffer, EVEL_EVENT_BUFFER_DEPTH);

  /***************************************************************************/
  /* Initialize the priority post buffer to empty.                           */
  /***************************************************************************/
  priority_post.memory = NULL;

exit_label:
  EVEL_EXIT();

  return(rc);
}

/**************************************************************************//**
 * Run the event handler.
 *
 * Spawns the thread responsible for handling events and sending them to the
 * API.
 *
 *  @return Status code.
 *  @retval ::EVEL_SUCCESS if everything OK.
 *  @retval One of ::EVEL_ERR_CODES if there was a problem.
 *****************************************************************************/
EVEL_ERR_CODES event_handler_run()
{
  EVEL_ERR_CODES rc = EVEL_SUCCESS;
  int pthread_rc = 0;

  EVEL_ENTER();

  /***************************************************************************/
  /* Start the event handler thread.                                         */
  /***************************************************************************/
  evt_handler_state = EVT_HANDLER_INACTIVE;
  pthread_rc = pthread_create(&evt_handler_thread, NULL, event_handler, NULL);
  if (pthread_rc != 0)
  {
    rc = EVEL_PTHREAD_LIBRARY_FAIL;
    log_error_state("Failed to start event handler thread. "
                    "Error code=%d", pthread_rc);
  }

  EVEL_EXIT()
  return rc;
}

/**************************************************************************//**
 * Terminate the event handler.
 *
 * Shuts down the event handler thread in as clean a way as possible. Sets the
 * global exit flag and then signals the thread to interrupt it since it's
 * most likely waiting on the ring-buffer.
 *
 * Having achieved an orderly shutdown of the event handler thread, clean up
 * the cURL library's resources cleanly.
 *
 *  @return Status code.
 *  @retval ::EVEL_SUCCESS if everything OK.
 *  @retval One of ::EVEL_ERR_CODES if there was a problem.
 *****************************************************************************/
EVEL_ERR_CODES event_handler_terminate()
{
  EVEL_ERR_CODES rc = EVEL_SUCCESS;

  EVEL_ENTER();
  EVENT_INTERNAL *event = NULL;

  /***************************************************************************/
  /* Make sure that we were initialized before trying to terminate the       */
  /* event handler thread.                                                   */
  /***************************************************************************/
  if (evt_handler_state != EVT_HANDLER_UNINITIALIZED)
  {
    /*************************************************************************/
    /* Make sure that the event handler knows it's time to die.              */
    /*************************************************************************/
    event = evel_new_internal_event(EVT_CMD_TERMINATE,"EVELinternal","EVELid");
    if (event == NULL)
    {
      /***********************************************************************/
      /* We failed to get an event, but we don't bail out - we will just     */
      /* clean up what we can and continue on our way, since we're exiting   */
      /* anyway.                                                             */
      /***********************************************************************/
      EVEL_ERROR("Failed to get internal event - perform dirty exit instead!");
    }
    else
    {
      /***********************************************************************/
      /* Post the event then wait for the Event Handler to exit.  Set the    */
      /* global command, too, in case the ring-buffer is full.               */
      /***********************************************************************/
      EVEL_DEBUG("Sending event to Event Hander to request it to exit.");
      evt_handler_state = EVT_HANDLER_REQUEST_TERMINATE;
      evel_post_event((EVENT_HEADER *) event);
      pthread_join(evt_handler_thread, NULL);
      EVEL_DEBUG("Event Handler thread has exited.");
    }
  }
  else
  {
    EVEL_DEBUG("Event handler was not initialized, so no need to kill it");
  }

  /***************************************************************************/
  /* Clean-up the cURL library.                                              */
  /***************************************************************************/
  if (curl_handle != NULL)
  {
    curl_easy_cleanup(curl_handle);
    curl_handle = NULL;
  }
  if (hdr_chunk != NULL)
  {
    curl_slist_free_all(hdr_chunk);
    hdr_chunk = NULL;
  }

  /***************************************************************************/
  /* Free off the stored API URL strings.                                    */
  /***************************************************************************/
  if (evel_event_api_url != NULL)
  {
    free(evel_event_api_url);
    evel_event_api_url = NULL;
  }
  if (evel_throt_api_url != NULL)
  {
    free(evel_throt_api_url);
    evel_throt_api_url = NULL;
  }

  EVEL_EXIT();
  return rc;
}

/**************************************************************************//**
 * Post an event.
 *
 * @note  So far as the caller is concerned, successfully posting the event
 * relinquishes all responsibility for the event - the library will take care
 * of freeing the event in due course.

 * @param event   The event to be posted.
 *
 * @returns Status code
 * @retval  EVEL_SUCCESS On success
 * @retval  "One of ::EVEL_ERR_CODES" On failure.
 *****************************************************************************/
EVEL_ERR_CODES evel_post_event(EVENT_HEADER * event)
{
  int rc = EVEL_SUCCESS;

  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(event != NULL);

  /***************************************************************************/
  /* We need to make sure that we are either initializing or running         */
  /* normally before writing the event into the buffer so that we can        */
  /* guarantee that the ring-buffer empties  properly on exit.               */
  /***************************************************************************/
  if ((evt_handler_state == EVT_HANDLER_ACTIVE) ||
      (evt_handler_state == EVT_HANDLER_INACTIVE) ||
      (evt_handler_state == EVT_HANDLER_REQUEST_TERMINATE))
  {
    if (ring_buffer_write(&event_buffer, event) == 0)
    {
      log_error_state("Failed to write event to buffer - event dropped!");
      rc = EVEL_EVENT_BUFFER_FULL;
      evel_free_event(event);
    }
  }
  else
  {
    /*************************************************************************/
    /* System is not in active operation, so reject the event.               */
    /*************************************************************************/
    log_error_state("Event Handler system not active - event dropped!");
    rc = EVEL_EVENT_HANDLER_INACTIVE;
    evel_free_event(event);
  }

  EVEL_EXIT();
  return (rc);
}

/**************************************************************************//**
 * Post an event to the Vendor Event Listener API.
 *
 * @returns Status code
 * @retval  EVEL_SUCCESS On success
 * @retval  "One of ::EVEL_ERR_CODES" On failure.
 *****************************************************************************/
static EVEL_ERR_CODES evel_post_api(char * msg, size_t size)
{
  int rc = EVEL_SUCCESS;
  CURLcode curl_rc = CURLE_OK;
  MEMORY_CHUNK rx_chunk;
  MEMORY_CHUNK tx_chunk;
  int http_response_code = 0;

  EVEL_ENTER();

  /***************************************************************************/
  /* Create the memory chunk to be used for the response to the post.  The   */
  /* will be realloced.                                                      */
  /***************************************************************************/
  rx_chunk.memory = malloc(1);
  assert(rx_chunk.memory != NULL);
  rx_chunk.size = 0;

  /***************************************************************************/
  /* Create the memory chunk to be sent as the body of the post.             */
  /***************************************************************************/
  tx_chunk.memory = msg;
  tx_chunk.size = size;
  EVEL_DEBUG("Sending chunk of size %d", tx_chunk.size);

  /***************************************************************************/
  /* Point to the data to be received.                                       */
  /***************************************************************************/
  curl_rc = curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &rx_chunk);
  if (curl_rc != CURLE_OK)
  {
    rc = EVEL_CURL_LIBRARY_FAIL;
    log_error_state("Failed to initialize libCURL to upload. "
                    "Error code=%d (%s)", curl_rc, curl_err_string);
    goto exit_label;
  }
  EVEL_DEBUG("Initialized data to receive");

  /***************************************************************************/
  /* Pointer to pass to our read function                                    */
  /***************************************************************************/
  curl_rc = curl_easy_setopt(curl_handle, CURLOPT_READDATA, &tx_chunk);
  if (curl_rc != CURLE_OK)
  {
    rc = EVEL_CURL_LIBRARY_FAIL;
    log_error_state("Failed to set upload data for libCURL to upload. "
                    "Error code=%d (%s)", curl_rc, curl_err_string);
    goto exit_label;
  }
  EVEL_DEBUG("Initialized data to send");

  /***************************************************************************/
  /* Size of the data to transmit.                                           */
  /***************************************************************************/
  curl_rc = curl_easy_setopt(curl_handle,
                             CURLOPT_POSTFIELDSIZE,
                             tx_chunk.size);
  if (curl_rc != CURLE_OK)
  {
    rc = EVEL_CURL_LIBRARY_FAIL;
    log_error_state("Failed to set length of upload data for libCURL to "
                    "upload.  Error code=%d (%s)", curl_rc, curl_err_string);
    goto exit_label;
  }
  EVEL_DEBUG("Initialized length of data to send");

  /***************************************************************************/
  /* Now run off and do what you've been told!                               */
  /***************************************************************************/
  curl_rc = curl_easy_perform(curl_handle);
  if (curl_rc != CURLE_OK)
  {
    rc = EVEL_CURL_LIBRARY_FAIL;
    log_error_state("Failed to transfer an event to Vendor Event Listener! "
                    "Error code=%d (%s)", curl_rc, curl_err_string);
    EVEL_ERROR("Dropped event: %s", msg);
    goto exit_label;
  }

  /***************************************************************************/
  /* See what response we got - any 2XX response is good.                    */
  /***************************************************************************/
  curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &http_response_code);
  EVEL_DEBUG("HTTP response code: %d", http_response_code);
  if ((http_response_code / 100) == 2)
  {
    /*************************************************************************/
    /* If the server responded with data it may be interesting but not a     */
    /* problem.                                                              */
    /*************************************************************************/
    if ((rx_chunk.size > 0) && (rx_chunk.memory != NULL))
    {
      EVEL_DEBUG("Server returned data = %d (%s)",
                 rx_chunk.size,
                 rx_chunk.memory);

      /***********************************************************************/
      /* If this is a response to priority post, then we're not interested.  */
      /***********************************************************************/
      if (priority_post.memory != NULL)
      {
        EVEL_ERROR("Ignoring priority post response");
      }
      else
      {
        evel_handle_event_response(&rx_chunk, &priority_post);
      }
    }
  }
  else
  {
    EVEL_ERROR("Unexpected HTTP response code: %d with data size %d (%s)",
                http_response_code,
                rx_chunk.size,
                rx_chunk.size > 0 ? rx_chunk.memory : "NONE");
    EVEL_ERROR("Potentially dropped event: %s", msg);
  }

exit_label:
  free(rx_chunk.memory);
  EVEL_EXIT();
  return(rc);
}

/**************************************************************************//**
 * Callback function to provide data to send.
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
  MEMORY_CHUNK *tx_chunk = (MEMORY_CHUNK *)userp;

  EVEL_ENTER();

  bytes_to_write = min(size*nmemb, tx_chunk->size);

  if (bytes_to_write > 0)
  {
    EVEL_DEBUG("Going to try to write %d bytes", bytes_to_write);
    strncpy((char *)ptr, tx_chunk->memory, bytes_to_write);
    tx_chunk->memory += bytes_to_write;
    tx_chunk->size -= bytes_to_write;
    rtn = bytes_to_write;
  }
  else
  {
    EVEL_DEBUG("Reached EOF");
  }

  EVEL_EXIT();
  return rtn;
}

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
                             void *userp)
{
  size_t realsize = size * nmemb;
  MEMORY_CHUNK * rx_chunk = (MEMORY_CHUNK *)userp;

  EVEL_ENTER();

  EVEL_DEBUG("Called with %d chunks of %d size = %d", nmemb, size, realsize);
  EVEL_DEBUG("rx chunk size is %d", rx_chunk->size);

  rx_chunk->memory = realloc(rx_chunk->memory, rx_chunk->size + realsize + 1);
  if(rx_chunk->memory == NULL) {
    /* out of memory! */
    printf("not enough memory (realloc returned NULL)\n");
    return 0;
  }

  memcpy(&(rx_chunk->memory[rx_chunk->size]), contents, realsize);
  rx_chunk->size += realsize;
  rx_chunk->memory[rx_chunk->size] = 0;

  EVEL_DEBUG("Rx data: %s", rx_chunk->memory);
  EVEL_DEBUG("Returning: %d", realsize);

  EVEL_EXIT();
  return realsize;
}

/**************************************************************************//**
 * Event Handler.
 *
 * Watch for messages coming on the internal queue and send them to the
 * listener.
 *
 * param[in]  arg  Argument - unused.
 *****************************************************************************/
static void * event_handler(void * arg __attribute__ ((unused)))
{
  int old_type = 0;
  EVENT_HEADER * msg = NULL;
  EVENT_INTERNAL * internal_msg = NULL;
  int json_size = 0;
  char json_body[EVEL_MAX_JSON_BODY];
  int rc = EVEL_SUCCESS;
  CURLcode curl_rc;

  EVEL_INFO("Event handler thread started");

  /***************************************************************************/
  /* Set this thread to be cancellable immediately.                          */
  /***************************************************************************/
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &old_type);

  /***************************************************************************/
  /* Set the handler as active, defending against weird situations like      */
  /* immediately shutting down after initializing the library so the         */
  /* handler never gets started up properly.                                 */
  /***************************************************************************/
  if (evt_handler_state == EVT_HANDLER_INACTIVE)
  {
    evt_handler_state = EVT_HANDLER_ACTIVE;
  }
  else
  {
    EVEL_ERROR("Event Handler State was not INACTIVE at start-up - "
               "Handler will exit immediately!");
  }

  while (evt_handler_state == EVT_HANDLER_ACTIVE)
  {
    /*************************************************************************/
    /* Wait for a message to be received.                                    */
    /*************************************************************************/
    EVEL_DEBUG("Event handler getting any messages");
    msg = ring_buffer_read(&event_buffer);

    /*************************************************************************/
    /* Internal events get special treatment while regular events get posted */
    /* to the far side.                                                      */
    /*************************************************************************/
    if (msg->event_domain != EVEL_DOMAIN_INTERNAL)
    {
      EVEL_DEBUG("External event received");

      /***********************************************************************/
      /* Encode the event in JSON.                                           */
      /***********************************************************************/
      json_size = evel_json_encode_event(json_body, EVEL_MAX_JSON_BODY, msg);

      /***********************************************************************/
      /* Send the JSON across the API.                                       */
      /***********************************************************************/
      EVEL_DEBUG("Sending JSON of size %d is: %s", json_size, json_body);
      rc = evel_post_api(json_body, json_size);
      if (rc != EVEL_SUCCESS)
      {
        EVEL_ERROR("Failed to transfer the data. Error code=%d", rc);
      }
    }
    else
    {
      EVEL_DEBUG("Internal event received");
      internal_msg = (EVENT_INTERNAL *) msg;
      assert(internal_msg->command == EVT_CMD_TERMINATE);
      evt_handler_state = EVT_HANDLER_TERMINATING;
    }

    /*************************************************************************/
    /* We are responsible for freeing the memory.                            */
    /*************************************************************************/
    evel_free_event(msg);
    msg = NULL;

    /*************************************************************************/
    /* There may be a single priority post to be sent.                       */
    /*************************************************************************/
    if (priority_post.memory != NULL)
    {
      EVEL_DEBUG("Priority Post");

      /***********************************************************************/
      /* Set the URL for the throttling API.                                 */
      /***********************************************************************/
      curl_rc = curl_easy_setopt(curl_handle, CURLOPT_URL, evel_throt_api_url);
      if (curl_rc != CURLE_OK)
      {
        /*********************************************************************/
        /* This is only likely to happen with CURLE_OUT_OF_MEMORY, in which  */
        /* case we carry on regardless.                                      */
        /*********************************************************************/
        EVEL_ERROR("Failed to set throttling URL. Error code=%d", rc);
      }
      else
      {
        rc = evel_post_api(priority_post.memory, priority_post.size);
        if (rc != EVEL_SUCCESS)
        {
          EVEL_ERROR("Failed to transfer priority post. Error code=%d", rc);
        }
      }

      /***********************************************************************/
      /* Reinstate the URL for the event API.                                */
      /***********************************************************************/
      curl_rc = curl_easy_setopt(curl_handle, CURLOPT_URL, evel_event_api_url);
      if (curl_rc != CURLE_OK)
      {
        /*********************************************************************/
        /* This is only likely to happen with CURLE_OUT_OF_MEMORY, in which  */
        /* case we carry on regardless.                                      */
        /*********************************************************************/
        EVEL_ERROR("Failed to reinstate events URL. Error code=%d", rc);
      }

      /***********************************************************************/
      /* We are responsible for freeing the memory.                          */
      /***********************************************************************/
      free(priority_post.memory);
      priority_post.memory = NULL;
    }
  }

  /***************************************************************************/
  /* The event handler is now exiting. The ring-buffer could contain events  */
  /* which have not been processed, so deplete those.  Because we've been    */
  /* asked to exit we can be confident that the foreground will have stopped */
  /* sending events in so we know that this process will conclude!           */
  /***************************************************************************/
  evt_handler_state = EVT_HANDLER_TERMINATING;
  while (!ring_buffer_is_empty(&event_buffer))
  {
    EVEL_DEBUG("Reading event from buffer");
    msg = ring_buffer_read(&event_buffer);
    evel_free_event(msg);
  }
  evt_handler_state = EVT_HANDLER_TERMINATED;
  EVEL_INFO("Event handler thread stopped");

  return (NULL);
}

/**************************************************************************//**
 * Handle a JSON response from the listener, contained in a ::MEMORY_CHUNK.
 *
 * Tokenize the response, and decode any tokens found.
 *
 * @param chunk         The memory chunk containing the response.
 * @param post          The memory chunk in which to place any resulting POST.
 *****************************************************************************/
void evel_handle_event_response(const MEMORY_CHUNK * const chunk,
                                MEMORY_CHUNK * const post)
{
  jsmn_parser json_parser;
  jsmntok_t json_tokens[EVEL_MAX_RESPONSE_TOKENS];
  int num_tokens = 0;

  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(chunk != NULL);
  assert(priority_post.memory == NULL);

  EVEL_DEBUG("Response size = %d", chunk->size);
  EVEL_DEBUG("Response = %s", chunk->memory);

  /***************************************************************************/
  /* Initialize the parser and tokenize the response.                        */
  /***************************************************************************/
  jsmn_init(&json_parser);
  num_tokens = jsmn_parse(&json_parser,
                          chunk->memory,
                          chunk->size,
                          json_tokens,
                          EVEL_MAX_RESPONSE_TOKENS);

  if (num_tokens < 0)
  {
    EVEL_ERROR("Failed to parse JSON response.  "
               "Error code=%d", num_tokens);
  }
  else if (num_tokens == 0)
  {
    EVEL_DEBUG("No tokens found in JSON response");
  }
  else
  {
    EVEL_DEBUG("Decode JSON response tokens");
    if (!evel_handle_response_tokens(chunk, json_tokens, num_tokens, post))
    {
      EVEL_ERROR("Failed to handle JSON response.");
    }
  }

  EVEL_EXIT();
}

/**************************************************************************//**
 * Handle a JSON response from the listener, as a list of tokens from JSMN.
 *
 * @param chunk         Memory chunk containing the JSON buffer.
 * @param json_tokens   Array of tokens to handle.
 * @param num_tokens    The number of tokens to handle.
 * @param post          The memory chunk in which to place any resulting POST.
 * @return true if we handled the response, false otherwise.
 *****************************************************************************/
bool evel_handle_response_tokens(const MEMORY_CHUNK * const chunk,
                                 const jsmntok_t * const json_tokens,
                                 const int num_tokens,
                                 MEMORY_CHUNK * const post)
{
  bool json_ok = false;

  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(chunk != NULL);
  assert(json_tokens != NULL);
  assert(num_tokens < EVEL_MAX_RESPONSE_TOKENS);

  /***************************************************************************/
  /* Peek at the tokens to decide what the response it, then call the        */
  /* appropriate handler to handle it.  There is only one handler at this    */
  /* point.                                                                  */
  /***************************************************************************/
  if (evel_tokens_match_command_list(chunk, json_tokens, num_tokens))
  {
    json_ok = evel_handle_command_list(chunk, json_tokens, num_tokens, post);
  }

  EVEL_EXIT();

  return json_ok;
}

/**************************************************************************//**
 * Determine whether a list of tokens looks like a "commandList" response.
 *
 * @param chunk         Memory chunk containing the JSON buffer.
 * @param json_tokens   Token to check.
 * @param num_tokens    The number of tokens to handle.
 * @return true if the tokens look like a "commandList" match, or false.
 *****************************************************************************/
bool evel_tokens_match_command_list(const MEMORY_CHUNK * const chunk,
                                    const jsmntok_t * const json_tokens,
                                    const int num_tokens)
{
  bool result = false;

  EVEL_ENTER();

  /***************************************************************************/
  /* Make some checks on the basic layout of the commandList.                */
  /***************************************************************************/
  if ((num_tokens > 3) &&
      (json_tokens[0].type == JSMN_OBJECT) &&
      (json_tokens[1].type == JSMN_STRING) &&
      (json_tokens[2].type == JSMN_ARRAY) &&
      (evel_token_equals_string(chunk, &json_tokens[1], "commandList")))
  {
    result = true;
  }

  EVEL_EXIT();

  return result;
}

/**************************************************************************//**
 * Check that a string token matches a given input string.
 *
 * @param chunk         Memory chunk containing the JSON buffer.
 * @param json_token    Token to check.
 * @param check_string  String to check it against.
 * @return true if the strings match, or false.
 *****************************************************************************/
bool evel_token_equals_string(const MEMORY_CHUNK * const chunk,
                              const jsmntok_t * json_token,
                              const char * check_string)
{
  bool result = false;

  EVEL_ENTER();

  const int token_length = json_token->end - json_token->start;
  const char * const token_string = chunk->memory + json_token->start;

  if (token_length == (int)strlen(check_string))
  {
    result = (strncmp(token_string, check_string, token_length) == 0);
  }

  EVEL_EXIT();

  return result;
}
