/**************************************************************************//**
 * @file
 * Wrap the OpenStack metadata service.
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
#include <malloc.h>

#include <curl/curl.h>

#include "evel.h"
#include "evel_internal.h"
#include "jsmn.h"
#include "metadata.h"

/**************************************************************************//**
 * URL on the link-local IP address where we can get the metadata in
 * machine-friendly format.
 *****************************************************************************/
static const char * OPENSTACK_METADATA_URL =
                      "http://169.254.169.254/openstack/latest/meta_data.json";

/**************************************************************************//**
 * How long we're prepared to wait for the metadata service to respond in
 * seconds.
 *****************************************************************************/
static const int OPENSTACK_METADATA_TIMEOUT = 2;

/**************************************************************************//**
 * Size of fields extracted from metadata service.
 *****************************************************************************/
#define MAX_METADATA_STRING  64

/**************************************************************************//**
 * UUID of the VM extracted from the OpenStack metadata service.
 *****************************************************************************/
static char vm_uuid[MAX_METADATA_STRING+1] = {0};

/**************************************************************************//**
 * Name of the VM extracted from the OpenStack metadata service.
 *****************************************************************************/
static char vm_name[MAX_METADATA_STRING+1] = {0};

/**************************************************************************//**
 * How many metadata elements we allow for in the retrieved JSON.
 *****************************************************************************/
static const int MAX_METADATA_TOKENS = 128;

/*****************************************************************************/
/* Local prototypes.                                                         */
/*****************************************************************************/
static EVEL_ERR_CODES json_get_top_level_string(const char * json_string,
                                                const jsmntok_t *tokens,
                                                int json_token_count,
                                                const char * key,
                                                char * value);
static EVEL_ERR_CODES json_get_string(const char * json_string,
                                      const jsmntok_t *tokens,
                                      int json_token_count,
                                      const char * key,
                                      char * value);
static int jsoneq(const char *json, const jsmntok_t *tok, const char *s);

/**************************************************************************//**
 * Download metadata from the OpenStack metadata service.
 *
 * @param verbosity   Controls whether to generate debug to stdout.  Zero:
 *                    none.  Non-zero: generate debug.
 * @returns Status code
 * @retval  EVEL_SUCCESS      On success
 * @retval  ::EVEL_ERR_CODES  On failure.
 *****************************************************************************/
EVEL_ERR_CODES openstack_metadata(int verbosity)
{
  int rc = EVEL_SUCCESS;
  CURLcode curl_rc = CURLE_OK;
  CURL * curl_handle = NULL;
  MEMORY_CHUNK rx_chunk;
  char curl_err_string[CURL_ERROR_SIZE] = "<NULL>";
  jsmn_parser json_parser;
  jsmntok_t tokens[MAX_METADATA_TOKENS];
  int json_token_count = 0;

  EVEL_ENTER();

  /***************************************************************************/
  /* Initialize dummy values for the metadata - needed for test              */
  /* environments.                                                           */
  /***************************************************************************/
  openstack_metadata_initialize();

  /***************************************************************************/
  /* Get a curl handle which we'll use for accessing the metadata service.   */
  /***************************************************************************/
  curl_handle = curl_easy_init();
  if (curl_handle == NULL)
  {
    rc = EVEL_CURL_LIBRARY_FAIL;
    EVEL_ERROR("Failed to get libcurl handle");
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
    EVEL_ERROR("Failed to initialize libcurl to provide friendly errors. "
               "Error code=%d", curl_rc);
    goto exit_label;
  }

  /***************************************************************************/
  /* Set the URL for the metadata API.                                       */
  /***************************************************************************/
  curl_rc = curl_easy_setopt(curl_handle, CURLOPT_URL, OPENSTACK_METADATA_URL);
  if (curl_rc != CURLE_OK)
  {
    rc = EVEL_CURL_LIBRARY_FAIL;
    EVEL_ERROR("Failed to initialize libcurl with the API URL. "
               "Error code=%d (%s)", curl_rc, curl_err_string);
    goto exit_label;
  }

  /***************************************************************************/
  /* send all data to this function.                                         */
  /***************************************************************************/
  curl_rc = curl_easy_setopt(curl_handle,
                             CURLOPT_WRITEFUNCTION,
                             evel_write_callback);
  if (curl_rc != CURLE_OK)
  {
    rc = EVEL_CURL_LIBRARY_FAIL;
    EVEL_ERROR("Failed to initialize libcurl with the write callback. "
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
    EVEL_ERROR("Failed to initialize libcurl to upload.  Error code=%d (%s)",
               curl_rc, curl_err_string);
    goto exit_label;
  }

  /***************************************************************************/
  /* Set the timeout for the operation.                                      */
  /***************************************************************************/
  curl_rc = curl_easy_setopt(curl_handle,
                             CURLOPT_TIMEOUT,
                             OPENSTACK_METADATA_TIMEOUT);
  if (curl_rc != CURLE_OK)
  {
    rc = EVEL_NO_METADATA;
    EVEL_ERROR("Failed to initialize libcurl to set timeout. "
               "Error code=%d (%s)", curl_rc, curl_err_string);
    goto exit_label;
  }

  /***************************************************************************/
  /* Create the memory chunk to be used for the response to the post.  The   */
  /* will be realloced.                                                      */
  /***************************************************************************/
  rx_chunk.memory = malloc(1);
  assert(rx_chunk.memory != NULL);
  rx_chunk.size = 0;

  /***************************************************************************/
  /* Point to the data to be received.                                       */
  /***************************************************************************/
  curl_rc = curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&rx_chunk);
  if (curl_rc != CURLE_OK)
  {
    rc = EVEL_CURL_LIBRARY_FAIL;
    EVEL_ERROR("Failed to initialize libcurl to receive metadata. "
               "Error code=%d (%s)", curl_rc, curl_err_string);
    goto exit_label;
  }
  EVEL_DEBUG("Initialized data to receive");

  /***************************************************************************/
  /* If running in verbose mode generate more output.                        */
  /***************************************************************************/
  if (verbosity > 0)
  {
    curl_rc = curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 1L);
    if (curl_rc != CURLE_OK)
    {
      rc = EVEL_CURL_LIBRARY_FAIL;
      log_error_state("Failed to initialize libcurl to be verbose. "
                      "Error code=%d", curl_rc);
      goto exit_label;
    }
  }

  /***************************************************************************/
  /* Now run off and do what you've been told!                               */
  /***************************************************************************/
  curl_rc = curl_easy_perform(curl_handle);
  if (curl_rc != CURLE_OK)
  {
    rc = EVEL_CURL_LIBRARY_FAIL;
    EVEL_ERROR("Failed to transfer the data from metadata service. "
               "Error code=%d (%s)", curl_rc, curl_err_string);
  }
  else
  {
    /*************************************************************************/
    /* We have some metadata available, so break it out into tokens.         */
    /*************************************************************************/
    EVEL_DEBUG("Received metadata size = %d", rx_chunk.size);
    EVEL_INFO("Received metadata = %s", rx_chunk.memory);
    jsmn_init(&json_parser);
    json_token_count = jsmn_parse(&json_parser,
                                  rx_chunk.memory, rx_chunk.size,
                                  tokens, MAX_METADATA_TOKENS);

    /*************************************************************************/
    /* Check that we parsed some data and that the top level is as expected. */
    /*************************************************************************/
    if (json_token_count < 0 || tokens[0].type != JSMN_OBJECT)
    {
      rc = EVEL_BAD_METADATA;
      EVEL_ERROR("Failed to parse received JSON OpenStack metadata.  "
                 "Error code=%d", json_token_count);
      goto exit_label;
    }
    else
    {
      EVEL_DEBUG("Extracted %d tokens from the JSON OpenStack metadata.  ",
                                                             json_token_count);
    }

    /*************************************************************************/
    /* Find the keys we want from the metadata.                              */
    /*************************************************************************/
    if (json_get_string(rx_chunk.memory,
                        tokens,
                        json_token_count,
                        "uuid",
                        vm_uuid) != EVEL_SUCCESS)
    {
      rc = EVEL_BAD_METADATA;
      EVEL_ERROR("Failed to extract UUID from OpenStack metadata");
    }
    else
    {
      EVEL_DEBUG("UUID: %s", vm_uuid);
    }
    if (json_get_top_level_string(rx_chunk.memory,
                                  tokens,
                                  json_token_count,
                                  "name",
                                  vm_name) != EVEL_SUCCESS)
    {
      rc = EVEL_BAD_METADATA;
      EVEL_ERROR("Failed to extract VM Name from OpenStack metadata");
    }
    else
    {
      EVEL_DEBUG("VM Name: %s", vm_name);
    }
  }

exit_label:

  /***************************************************************************/
  /* Shut down the cURL library in a tidy manner.                            */
  /***************************************************************************/
  if (curl_handle != NULL)
  {
    curl_easy_cleanup(curl_handle);
    curl_handle = NULL;
  }
  free(rx_chunk.memory);

  EVEL_EXIT();
  return rc;
}

/**************************************************************************//**
 * Initialize default values for vm_name and vm_uuid - for testing purposes.
 *****************************************************************************/
void openstack_metadata_initialize()
{
  strncpy(vm_uuid,
          "Dummy VM UUID - No Metadata available",
          MAX_METADATA_STRING);
  strncpy(vm_name,
          "Dummy VM name - No Metadata available",
          MAX_METADATA_STRING);
}

/**************************************************************************//**
 * Get a string value from supplied JSON by matching the key.
 *
 * As the structure of the metadata we're looking at is pretty straightforward
 * we don't do anything complex (a la XPath) to extract nested keys with the
 * same leaf name, for example.  Simply walk the structure until we find a
 * string with the correct value.
 *
 * @param[in] json_string   The string which contains the JSON and has already
 *                          been parsed.
 * @param[in] tokens        The tokens which the JSON parser found in the JSON.
 * @param[in] json_token_count  How many tokens were found.
 * @param[in] key           The key we're looking for.
 * @param[out] value        The string we found at @p key.
 *
 * @returns Status code
 * @retval  EVEL_SUCCESS      On success - contents of @p value updated.
 * @retval  EVEL_JSON_KEY_NOT_FOUND  Key not found - @p value not updated.
 * @retval  EVEL_BAD_JSON     Parser hit unexpected data - @p value not
 *                            updated.
 *****************************************************************************/
static EVEL_ERR_CODES json_get_string(const char * json_string,
                                      const jsmntok_t * tokens,
                                      int json_token_count,
                                      const char * key,
                                      char * value)
{
  EVEL_ERR_CODES rc = EVEL_JSON_KEY_NOT_FOUND;
  int token_num = 0;
  int token_len = 0;

  EVEL_ENTER();

  /***************************************************************************/
  /* Check assumptions.                                                      */
  /***************************************************************************/
  assert(json_string != NULL);
  assert(tokens != NULL);
  assert(json_token_count >= 0);
  assert(key != NULL);
  assert(value != NULL);

  for (token_num = 0; token_num < json_token_count; token_num++)
  {
    switch(tokens[token_num].type)
    {
    case JSMN_OBJECT:
      EVEL_DEBUG("Skipping object");
      break;

    case JSMN_ARRAY:
      EVEL_DEBUG("Skipping array");
      break;

    case JSMN_STRING:
      /***********************************************************************/
      /* This is a string, so may be what we want.  Compare keys.            */
      /***********************************************************************/
      if (jsoneq(json_string, &tokens[token_num], key) == 0)
      {
        token_len = tokens[token_num + 1].end - tokens[token_num + 1].start;
        EVEL_DEBUG("Token %d len %d matches at %d to %d", token_num,
                                                   tokens[token_num + 1].start,
                                                   tokens[token_num + 1].end);
        strncpy(value, json_string + tokens[token_num + 1].start, token_len);
        value[token_len] = '\0';
        EVEL_DEBUG("Extracted key: \"%s\" Value: \"%s\"", key, value);
        rc = EVEL_SUCCESS;
        goto exit_label;
      }
      else
      {
        EVEL_DEBUG("String key did not match");
      }

      /***********************************************************************/
      /* Step over the value, whether we used it or not.                     */
      /***********************************************************************/
      token_num++;
      break;

    case JSMN_PRIMITIVE:
      EVEL_INFO("Skipping primitive");
      break;

    case JSMN_UNDEFINED:
    default:
      rc = EVEL_BAD_JSON_FORMAT;
      EVEL_ERROR("Unexpected JSON format at token %d (%d)",
                  token_num,
                  tokens[token_num].type);
      goto exit_label;
    }
  }

exit_label:
  EVEL_EXIT();
  return rc;
}

/**************************************************************************//**
 * Get a top-level string value from supplied JSON by matching the key.
 *
 * Unlike json_get_string, this only returns a value that is in the top-level
 * JSON object.
 *
 * @param[in] json_string   The string which contains the JSON and has already
 *                          been parsed.
 * @param[in] tokens        The tokens which the JSON parser found in the JSON.
 * @param[in] json_token_count  How many tokens were found.
 * @param[in] key           The key we're looking for.
 * @param[out] value        The string we found at @p key.
 *
 * @returns Status code
 * @retval  EVEL_SUCCESS      On success - contents of @p value updated.
 * @retval  EVEL_JSON_KEY_NOT_FOUND  Key not found - @p value not updated.
 * @retval  EVEL_BAD_JSON     Parser hit unexpected data - @p value not
 *                            updated.
 *****************************************************************************/
static EVEL_ERR_CODES json_get_top_level_string(const char * json_string,
                                                const jsmntok_t * tokens,
                                                int json_token_count,
                                                const char * key,
                                                char * value)
{
  EVEL_ERR_CODES rc = EVEL_JSON_KEY_NOT_FOUND;
  int token_num = 0;
  int token_len = 0;
  int bracket_count = 0;
  int string_index = 0;
  int increment = 0;

  EVEL_ENTER();

  /***************************************************************************/
  /* Check assumptions.                                                      */
  /***************************************************************************/
  assert(json_string != NULL);
  assert(tokens != NULL);
  assert(json_token_count >= 0);
  assert(key != NULL);
  assert(value != NULL);

  for (token_num = 0; token_num < json_token_count; token_num++)
  {
    switch(tokens[token_num].type)
    {
    case JSMN_OBJECT:
      EVEL_DEBUG("Skipping object");
      break;

    case JSMN_ARRAY:
      EVEL_DEBUG("Skipping array");
      break;

    case JSMN_STRING:
      /***********************************************************************/
      /* This is a string, so may be what we want.  Compare keys.            */
      /***********************************************************************/
      if (jsoneq(json_string, &tokens[token_num], key) == 0)
      {
        /*********************************************************************/
        /* Count the difference in the number of opening and closing         */
        /* brackets up to this token.  This needs to be 1 for a top-level    */
        /* string.  Let's just hope we don't have any strings containing     */
        /* brackets.                                                         */
        /*********************************************************************/
        increment = ((string_index < tokens[token_num].start) ? 1 : -1);

        while (string_index != tokens[token_num].start)
        {
          if (json_string[string_index] == '{')
          {
            bracket_count += increment;
          }
          else if (json_string[string_index] == '}')
          {
            bracket_count -= increment;
          }

          string_index += increment;
        }

        if (bracket_count == 1)
        {
          token_len = tokens[token_num + 1].end - tokens[token_num + 1].start;
          EVEL_DEBUG("Token %d len %d matches at top level at %d to %d",
                     token_num,
                     tokens[token_num + 1].start,
                     tokens[token_num + 1].end);
          strncpy(value, json_string + tokens[token_num + 1].start, token_len);
          value[token_len] = '\0';
          EVEL_DEBUG("Extracted key: \"%s\" Value: \"%s\"", key, value);
          rc = EVEL_SUCCESS;
          goto exit_label;
        }
        else
        {
          EVEL_DEBUG("String key did match, but not at top level");
        }
      }
      else
      {
        EVEL_DEBUG("String key did not match");
      }

      /***********************************************************************/
      /* Step over the value, whether we used it or not.                     */
      /***********************************************************************/
      token_num++;
      break;

    case JSMN_PRIMITIVE:
      EVEL_INFO("Skipping primitive");
      break;

    case JSMN_UNDEFINED:
    default:
      rc = EVEL_BAD_JSON_FORMAT;
      EVEL_ERROR("Unexpected JSON format at token %d (%d)",
                  token_num,
                  tokens[token_num].type);
      goto exit_label;
    }
  }

exit_label:
  EVEL_EXIT();
  return rc;
}

/**************************************************************************//**
 * Compare a JSON string token with a value.
 *
 * @param[in] json The string which contains the JSON and has already been
 *                 parsed.
 * @param[in] tok  The token which the JSON parser found in the JSON.
 * @param[in] s    The string we're looking for.
 *
 * @returns        Whether the token matches the string or not.
 * @retval  0      Value matches
 * @retval  -1     Value does not match.
 *****************************************************************************/
static int jsoneq(const char *json, const jsmntok_t *tok, const char *s) {
  if (tok->type == JSMN_STRING && (int) strlen(s) == tok->end - tok->start &&
      strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
    return 0;
  }
  return -1;
}

/**************************************************************************//**
 * Get the VM name provided by the metadata service.
 *
 * @returns VM name
 *****************************************************************************/
const char *openstack_vm_name()
{
  return vm_name;
}

/**************************************************************************//**
 * Get the VM UUID provided by the metadata service.
 *
 * @returns VM UUID
 *****************************************************************************/
const char *openstack_vm_uuid()
{
  return vm_uuid;
}
