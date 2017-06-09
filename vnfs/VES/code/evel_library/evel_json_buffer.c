/**************************************************************************//**
 * @file
 * Source module relating to internal EVEL_JSON_BUFFER manipulation functions.
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

#include <assert.h>
#include <string.h>

#include "evel_throttle.h"

/*****************************************************************************/
/* Local prototypes.                                                         */
/*****************************************************************************/
static char * evel_json_kv_comma(EVEL_JSON_BUFFER * jbuf);

/**************************************************************************//**
 * Initialize a ::EVEL_JSON_BUFFER.
 *
 * @param jbuf          Pointer to the ::EVEL_JSON_BUFFER to initialise.
 * @param json          Pointer to the underlying working buffer to use.
 * @param max_size      Size of storage available in the JSON buffer.
 * @param throttle_spec Pointer to throttle specification. Can be NULL.
 *****************************************************************************/
void evel_json_buffer_init(EVEL_JSON_BUFFER * jbuf,
                           char * const json,
                           const int max_size,
                           EVEL_THROTTLE_SPEC * throttle_spec)
{
  EVEL_ENTER();

  assert(jbuf != NULL);
  assert(json != NULL);
  jbuf->json = json;
  jbuf->max_size = max_size;
  jbuf->offset = 0;
  jbuf->throttle_spec = throttle_spec;
  jbuf->depth = 0;
  jbuf->checkpoint = -1;

  EVEL_EXIT();
}

/**************************************************************************//**
 * Encode an integer value to a JSON buffer.
 *
 * @param jbuf          Pointer to working ::EVEL_JSON_BUFFER.
 * @param value         The integer to add to it.
 *****************************************************************************/
void evel_enc_int(EVEL_JSON_BUFFER * jbuf,
                  const int value)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(jbuf != NULL);

  jbuf->offset += snprintf(jbuf->json + jbuf->offset,
                           jbuf->max_size - jbuf->offset,
                           "%d", value);

  EVEL_EXIT();
}

/**************************************************************************//**
 * Encode a string key and string value to a ::EVEL_JSON_BUFFER.
 *
 * @param jbuf          Pointer to working ::EVEL_JSON_BUFFER.
 * @param key           Pointer to the key to encode.
 * @param option        Pointer to holder of the corresponding value to encode.
 * @return true if the key, value was added, false if it was suppressed.
 *****************************************************************************/
bool evel_enc_kv_opt_string(EVEL_JSON_BUFFER * jbuf,
                            const char * const key,
                            const EVEL_OPTION_STRING * const option)
{
  bool added = false;

  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(option != NULL);

  if (option->is_set)
  {
    if ((jbuf->depth == EVEL_THROTTLE_FIELD_DEPTH) &&
        (jbuf->throttle_spec != NULL) &&
        evel_throttle_suppress_field(jbuf->throttle_spec, key))
    {
      EVEL_INFO("Suppressed: %s, %s", key, option->value);
    }
    else
    {
      EVEL_DEBUG("Encoded: %s, %s", key, option->value);
      evel_enc_kv_string(jbuf, key, option->value);
      added = true;
    }
  }

  EVEL_EXIT();

  return added;
}

/**************************************************************************//**
 * Encode a string key and string value to a ::EVEL_JSON_BUFFER.
 *
 * @param jbuf          Pointer to working ::EVEL_JSON_BUFFER.
 * @param key           Pointer to the key to encode.
 * @param value         Pointer to the corresponding value to encode.
 *****************************************************************************/
void evel_enc_kv_string(EVEL_JSON_BUFFER * jbuf,
                        const char * const key,
                        const char * const value)
{
  int index;
  int length;

  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(jbuf != NULL);
  assert(key != NULL);

  jbuf->offset += snprintf(jbuf->json + jbuf->offset,
                           jbuf->max_size - jbuf->offset,
                           "%s\"%s\": \"",
                           evel_json_kv_comma(jbuf),
                           key);

  /***************************************************************************/
  /* We need to escape quotation marks and backslashes in the value.         */
  /***************************************************************************/
  length = strlen(value);

  for (index = 0; index < length; index++)
  {
    /*************************************************************************/
    /* Drop out if no more space.                                            */
    /*************************************************************************/
    if (jbuf->max_size - jbuf->offset < 2)
    {
      break;
    }

    /*************************************************************************/
    /* Add an escape character if necessary, then write the character        */
    /* itself.                                                               */
    /*************************************************************************/
    if ((value[index] == '\"') || (value[index] == '\\'))
    {
      jbuf->json[jbuf->offset] = '\\';
      jbuf->offset++;
    }

    jbuf->json[jbuf->offset] = value[index];
    jbuf->offset++;
  }

  jbuf->offset += snprintf(jbuf->json + jbuf->offset,
                           jbuf->max_size - jbuf->offset,
                           "\"");

  EVEL_EXIT();
}

/**************************************************************************//**
 * Encode a string key and integer value to a ::EVEL_JSON_BUFFER.
 *
 * @param jbuf          Pointer to working ::EVEL_JSON_BUFFER.
 * @param key           Pointer to the key to encode.
 * @param option        Pointer to holder of the corresponding value to encode.
 * @return true if the key, value was added, false if it was suppressed.
 *****************************************************************************/
bool evel_enc_kv_opt_int(EVEL_JSON_BUFFER * jbuf,
                         const char * const key,
                         const EVEL_OPTION_INT * const option)
{
  bool added = false;

  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(option != NULL);

  if (option->is_set)
  {
    if ((jbuf->depth == EVEL_THROTTLE_FIELD_DEPTH) &&
        (jbuf->throttle_spec != NULL) &&
        evel_throttle_suppress_field(jbuf->throttle_spec, key))
    {
      EVEL_INFO("Suppressed: %s, %d", key, option->value);
    }
    else
    {
      EVEL_DEBUG("Encoded: %s, %d", key, option->value);
      evel_enc_kv_int(jbuf, key, option->value);
      added = true;
    }
  }

  EVEL_EXIT();

  return added;
}

/**************************************************************************//**
 * Encode a string key and integer value to a ::EVEL_JSON_BUFFER.
 *
 * @param jbuf          Pointer to working ::EVEL_JSON_BUFFER.
 * @param key           Pointer to the key to encode.
 * @param value         The corresponding value to encode.
 *****************************************************************************/
void evel_enc_kv_int(EVEL_JSON_BUFFER * jbuf,
                     const char * const key,
                     const int value)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(jbuf != NULL);
  assert(key != NULL);

  jbuf->offset += snprintf(jbuf->json + jbuf->offset,
                           jbuf->max_size - jbuf->offset,
                           "%s\"%s\": %d",
                           evel_json_kv_comma(jbuf),
                           key,
                           value);

  EVEL_EXIT();
}

/**************************************************************************//**
 * Encode a string key and double value to a ::EVEL_JSON_BUFFER.
 *
 * @param jbuf          Pointer to working ::EVEL_JSON_BUFFER.
 * @param key           Pointer to the key to encode.
 * @param option        Pointer to holder of the corresponding value to encode.
 * @return true if the key, value was added, false if it was suppressed.
 *****************************************************************************/
bool evel_enc_kv_opt_double(EVEL_JSON_BUFFER * jbuf,
                            const char * const key,
                            const EVEL_OPTION_DOUBLE * const option)
{
  bool added = false;

  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(option != NULL);

  if (option->is_set)
  {
    if ((jbuf->depth == EVEL_THROTTLE_FIELD_DEPTH) &&
        (jbuf->throttle_spec != NULL) &&
        evel_throttle_suppress_field(jbuf->throttle_spec, key))
    {
      EVEL_INFO("Suppressed: %s, %1f", key, option->value);
    }
    else
    {
      EVEL_DEBUG("Encoded: %s, %1f", key, option->value);
      evel_enc_kv_double(jbuf, key, option->value);
      added = true;
    }
  }

  EVEL_EXIT();

  return added;
}

/**************************************************************************//**
 * Encode a string key and double value to a ::EVEL_JSON_BUFFER.
 *
 * @param jbuf          Pointer to working ::EVEL_JSON_BUFFER.
 * @param key           Pointer to the key to encode.
 * @param value         The corresponding value to encode.
 *****************************************************************************/
void evel_enc_kv_double(EVEL_JSON_BUFFER * jbuf,
                        const char * const key,
                        const double value)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(jbuf != NULL);
  assert(key != NULL);

  jbuf->offset += snprintf(jbuf->json + jbuf->offset,
                           jbuf->max_size - jbuf->offset,
                           "%s\"%s\": %1f",
                           evel_json_kv_comma(jbuf),
                           key,
                           value);

  EVEL_EXIT();
}

/**************************************************************************//**
 * Encode a string key and unsigned long long value to a ::EVEL_JSON_BUFFER.
 *
 * @param jbuf          Pointer to working ::EVEL_JSON_BUFFER.
 * @param key           Pointer to the key to encode.
 * @param option        Pointer to holder of the corresponding value to encode.
 * @return true if the key, value was added, false if it was suppressed.
 *****************************************************************************/
bool evel_enc_kv_opt_ull(EVEL_JSON_BUFFER * jbuf,
                         const char * const key,
                         const EVEL_OPTION_ULL * const option)
{
  bool added = false;

  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(option != NULL);

  if (option->is_set)
  {
    if ((jbuf->depth == EVEL_THROTTLE_FIELD_DEPTH) &&
        (jbuf->throttle_spec != NULL) &&
        evel_throttle_suppress_field(jbuf->throttle_spec, key))
    {
      EVEL_INFO("Suppressed: %s, %1lu", key, option->value);
    }
    else
    {
      EVEL_DEBUG("Encoded: %s, %1lu", key, option->value);
      evel_enc_kv_ull(jbuf, key, option->value);
      added = true;
    }
  }

  EVEL_EXIT();

  return added;
}

/**************************************************************************//**
 * Encode a string key and unsigned long long value to a ::EVEL_JSON_BUFFER.
 *
 * @param jbuf          Pointer to working ::EVEL_JSON_BUFFER.
 * @param key           Pointer to the key to encode.
 * @param value         The corresponding value to encode.
 *****************************************************************************/
void evel_enc_kv_ull(EVEL_JSON_BUFFER * jbuf,
                     const char * const key,
                     const unsigned long long value)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(jbuf != NULL);
  assert(key != NULL);

  jbuf->offset += snprintf(jbuf->json + jbuf->offset,
                           jbuf->max_size - jbuf->offset,
                           "%s\"%s\": %llu",
                           evel_json_kv_comma(jbuf),
                           key,
                           value);

  EVEL_EXIT();
}

/**************************************************************************//**
 * Encode a string key and time value to a ::EVEL_JSON_BUFFER.
 *
 * @param jbuf          Pointer to working ::EVEL_JSON_BUFFER.
 * @param key           Pointer to the key to encode.
 * @param option        Pointer to holder of the corresponding value to encode.
 * @return true if the key, value was added, false if it was suppressed.
 *****************************************************************************/
bool evel_enc_kv_opt_time(EVEL_JSON_BUFFER * jbuf,
                          const char * const key,
                          const EVEL_OPTION_TIME * const option)
{
  bool added = false;

  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(option != NULL);

  if (option->is_set)
  {
    if ((jbuf->depth == EVEL_THROTTLE_FIELD_DEPTH) &&
        (jbuf->throttle_spec != NULL) &&
        evel_throttle_suppress_field(jbuf->throttle_spec, key))
    {
      EVEL_INFO("Suppressed time: %s", key);
    }
    else
    {
      EVEL_DEBUG("Encoded time: %s", key);
      evel_enc_kv_time(jbuf, key, &option->value);
      added = true;
    }
  }

  EVEL_EXIT();

  return added;
}

/**************************************************************************//**
 * Encode a string key and time value to a ::EVEL_JSON_BUFFER.
 *
 * @param jbuf          Pointer to working ::EVEL_JSON_BUFFER.
 * @param key           Pointer to the key to encode.
 * @param time          Pointer to the time to encode.
 *****************************************************************************/
void evel_enc_kv_time(EVEL_JSON_BUFFER * jbuf,
                      const char * const key,
                      const time_t * time)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(jbuf != NULL);
  assert(key != NULL);
  assert(time != NULL);

  jbuf->offset += snprintf(jbuf->json + jbuf->offset,
                           jbuf->max_size - jbuf->offset,
                           "%s\"%s\": \"",
                           evel_json_kv_comma(jbuf),
                           key);
  jbuf->offset += strftime(jbuf->json + jbuf->offset,
                           jbuf->max_size - jbuf->offset,
                           EVEL_RFC2822_STRFTIME_FORMAT,
                           localtime(time));
  jbuf->offset += snprintf(jbuf->json + jbuf->offset,
                           jbuf->max_size - jbuf->offset,
                           "\"");
  EVEL_EXIT();
}

/**************************************************************************//**
 * Encode a key and version.
 *
 * @param jbuf          Pointer to working ::EVEL_JSON_BUFFER.
 * @param key           Pointer to the key to encode.
 * @param major_version The major version to encode.
 * @param minor_version The minor version to encode.
 *****************************************************************************/
void evel_enc_version(EVEL_JSON_BUFFER * jbuf,
                      const char * const key,
                      const int major_version,
                      const int minor_version)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(jbuf != NULL);
  assert(key != NULL);

  evel_enc_kv_int(jbuf, key, major_version);
  if (minor_version != 0)
  {
    jbuf->offset += snprintf(jbuf->json + jbuf->offset,
                             jbuf->max_size - jbuf->offset,
                             ".%d",
                             minor_version);
  }

  EVEL_EXIT();
}

/**************************************************************************//**
 * Add the key and opening bracket of an optional named list to a JSON buffer.
 *
 * @param jbuf          Pointer to working ::EVEL_JSON_BUFFER.
 * @param key           Pointer to the key to encode.
 * @return true if the list was opened, false if it was suppressed.
 *****************************************************************************/
bool evel_json_open_opt_named_list(EVEL_JSON_BUFFER * jbuf,
                                   const char * const key)
{
  bool opened = false;

  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(jbuf != NULL);
  assert(key != NULL);

  if ((jbuf->depth == EVEL_THROTTLE_FIELD_DEPTH) &&
      (jbuf->throttle_spec != NULL) &&
      evel_throttle_suppress_field(jbuf->throttle_spec, key))
  {
    EVEL_INFO("Suppressed: %s", key);
    opened = false;
  }
  else
  {
    evel_json_open_named_list(jbuf, key);
    opened = true;
  }

  EVEL_EXIT();

  return opened;
}

/**************************************************************************//**
 * Add the key and opening bracket of a named list to a JSON buffer.
 *
 * @param jbuf          Pointer to working ::EVEL_JSON_BUFFER.
 * @param key           Pointer to the key to encode.
 *****************************************************************************/
void evel_json_open_named_list(EVEL_JSON_BUFFER * jbuf,
                               const char * const key)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(jbuf != NULL);
  assert(key != NULL);

  jbuf->offset += snprintf(jbuf->json + jbuf->offset,
                           jbuf->max_size - jbuf->offset,
                           "%s\"%s\": [",
                           evel_json_kv_comma(jbuf),
                           key);
  jbuf->depth++;

  EVEL_EXIT();
}

/**************************************************************************//**
 * Add the closing bracket of a list to a JSON buffer.
 *
 * @param jbuf          Pointer to working ::EVEL_JSON_BUFFER.
 *****************************************************************************/
void evel_json_close_list(EVEL_JSON_BUFFER * jbuf)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(jbuf != NULL);

  jbuf->offset += snprintf(jbuf->json + jbuf->offset,
                           jbuf->max_size - jbuf->offset,
                           "]");
  jbuf->depth--;

  EVEL_EXIT();
}

/**************************************************************************//**
 * Encode a list item with format and param list to a ::EVEL_JSON_BUFFER.
 *
 * @param jbuf          Pointer to working ::EVEL_JSON_BUFFER.
 * @param format        Format string in standard printf format.
 * @param ...           Variable parameters for format string.
 *****************************************************************************/
void evel_enc_list_item(EVEL_JSON_BUFFER * jbuf,
                        const char * const format,
                        ...)
{
  va_list largs;

  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(jbuf != NULL);
  assert(format != NULL);

  /***************************************************************************/
  /* Add a comma unless we're at the start of the list.                      */
  /***************************************************************************/
  if (jbuf->json[jbuf->offset - 1] != '[')
  {
    jbuf->offset += snprintf(jbuf->json + jbuf->offset,
                             jbuf->max_size - jbuf->offset,
                             ", ");
  }

  va_start(largs, format);
  jbuf->offset += vsnprintf(jbuf->json + jbuf->offset,
                            jbuf->max_size - jbuf->offset,
                            format,
                            largs);
  va_end(largs);

  EVEL_EXIT();
}

/**************************************************************************//**
 * Add the opening bracket of an optional named object to a JSON buffer.
 *
 * @param jbuf          Pointer to working ::EVEL_JSON_BUFFER.
 * @param key           Pointer to the key to encode.
 *****************************************************************************/
bool evel_json_open_opt_named_object(EVEL_JSON_BUFFER * jbuf,
                                     const char * const key)
{
  bool opened = false;

  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(jbuf != NULL);
  assert(key != NULL);

  if ((jbuf->depth == EVEL_THROTTLE_FIELD_DEPTH) &&
      (jbuf->throttle_spec != NULL) &&
      evel_throttle_suppress_field(jbuf->throttle_spec, key))
  {
    EVEL_INFO("Suppressed: %s", key);
    opened = false;
  }
  else
  {
    evel_json_open_named_object(jbuf, key);
    opened = true;
  }

  EVEL_EXIT();

  return opened;
}

/**************************************************************************//**
 * Add the opening bracket of an object to a JSON buffer.
 *
 * @param jbuf          Pointer to working ::EVEL_JSON_BUFFER.
 * @param key           Pointer to the key to encode.
 * @return true if the object was opened, false if it was suppressed.
 *****************************************************************************/
void evel_json_open_named_object(EVEL_JSON_BUFFER * jbuf,
                                 const char * const key)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(jbuf != NULL);
  assert(key != NULL);

  jbuf->offset += snprintf(jbuf->json + jbuf->offset,
                           jbuf->max_size - jbuf->offset,
                           "%s\"%s\": {",
                           evel_json_kv_comma(jbuf),
                           key);
  jbuf->depth++;

  EVEL_EXIT();
}

/**************************************************************************//**
 * Add the opening bracket of an object to a JSON buffer.
 *
 * @param jbuf          Pointer to working ::EVEL_JSON_BUFFER.
 *****************************************************************************/
void evel_json_open_object(EVEL_JSON_BUFFER * jbuf)
{
  char * comma;

  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(jbuf != NULL);

  if ((jbuf->offset != 0) && (jbuf->json[jbuf->offset-1] == '}'))
  {
    comma = ", ";
  }
  else
  {
    comma = "";
  }

  jbuf->offset += snprintf(jbuf->json + jbuf->offset,
                           jbuf->max_size - jbuf->offset,
                           "%s{",
                           comma);
  jbuf->depth++;

  EVEL_EXIT();
}

/**************************************************************************//**
 * Add the closing bracket of an object to a JSON buffer.
 *
 * @param jbuf          Pointer to working ::EVEL_JSON_BUFFER.
 *****************************************************************************/
void evel_json_close_object(EVEL_JSON_BUFFER * jbuf)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(jbuf != NULL);

  jbuf->offset += snprintf(jbuf->json + jbuf->offset,
                           jbuf->max_size - jbuf->offset,
                           "}");
  jbuf->depth--;

  EVEL_EXIT();
}

/**************************************************************************//**
 * Determine whether to add a comma when adding a key-value pair.
 *
 * @param jbuf          Pointer to working ::EVEL_JSON_BUFFER.
 * @returns A string containing the comma if it is required.
 *****************************************************************************/
char * evel_json_kv_comma(EVEL_JSON_BUFFER * jbuf)
{
  char * result;

  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(jbuf != NULL);

  if ((jbuf->offset == 0) ||
      (jbuf->json[jbuf->offset-1] == '{') ||
      (jbuf->json[jbuf->offset-1] == '['))
  {
    result = "";
  }
  else
  {
    result = ", ";
  }

  EVEL_EXIT();

  return result;
}

/**************************************************************************//**
 * Add a checkpoint - a stake in the ground to which we can rewind.
 *
 * @param jbuf          Pointer to working ::EVEL_JSON_BUFFER.
 *****************************************************************************/
void evel_json_checkpoint(EVEL_JSON_BUFFER * jbuf)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(jbuf != NULL);

  /***************************************************************************/
  /* Store the current offset.                                               */
  /***************************************************************************/
  jbuf->checkpoint = jbuf->offset;

  EVEL_EXIT();
}

/**************************************************************************//**
 * Rewind to the latest checkoint.
 *
 * @param jbuf          Pointer to working ::EVEL_JSON_BUFFER.
 *****************************************************************************/
void evel_json_rewind(EVEL_JSON_BUFFER * jbuf)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(jbuf != NULL);
  assert(jbuf->checkpoint >= 0);
  assert(jbuf->checkpoint <= jbuf->offset);

  /***************************************************************************/
  /* Reinstate the offset from the last checkpoint.                          */
  /***************************************************************************/
  jbuf->offset = jbuf->checkpoint;
  jbuf->checkpoint = -1;

  EVEL_EXIT();
}
