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
 * EVEL internal definitions.
 *
 * These are internal definitions which need to be shared between modules
 * within the library but are not intended for external consumption.
 *
 ****************************************************************************/

#ifndef EVEL_INTERNAL_INCLUDED
#define EVEL_INTERNAL_INCLUDED

#include "evel.h"

/*****************************************************************************/
/* Define some type-safe min/max macros.                                     */
/*****************************************************************************/
#define max(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

#define min(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })


/**************************************************************************//**
 * Compile-time assertion.
 *****************************************************************************/
#define EVEL_CT_ASSERT(X) switch (0) {case 0: case (X):;}

/**************************************************************************//**
 * The Functional Role of the equipment represented by this VNF.
 *****************************************************************************/
extern char * functional_role;

/**************************************************************************//**
 * The type of equipment represented by this VNF.
 *****************************************************************************/
extern EVEL_SOURCE_TYPES event_source_type;

/**************************************************************************//**
 * A chunk of memory used in the cURL functions.
 *****************************************************************************/
typedef struct memory_chunk {
  char * memory;
  size_t size;
} MEMORY_CHUNK;

/**************************************************************************//**
 * Global commands that may be sent to the Event Handler thread.
 *****************************************************************************/
typedef enum {
  EVT_CMD_TERMINATE,
  EVT_CMD_MAX_COMMANDS
} EVT_HANDLER_COMMAND;

/**************************************************************************//**
 * State of the Event Handler thread.
 *****************************************************************************/
typedef enum {
  EVT_HANDLER_UNINITIALIZED,      /** The library cannot handle events.      */
  EVT_HANDLER_INACTIVE,           /** The event handler thread not started.  */
  EVT_HANDLER_ACTIVE,             /** The event handler thread is started.   */
  EVT_HANDLER_REQUEST_TERMINATE,  /** Initial stages of shutdown.            */
  EVT_HANDLER_TERMINATING,        /** The ring-buffer is being depleted.     */
  EVT_HANDLER_TERMINATED,         /** The library is exited.                 */
  EVT_HANDLER_MAX_STATES          /** Maximum number of valid states.        */
} EVT_HANDLER_STATE;

/**************************************************************************//**
 * Internal event.
 * Pseudo-event used for routing internal commands.
 *****************************************************************************/
typedef struct event_internal {
  EVENT_HEADER header;
  EVT_HANDLER_COMMAND command;
} EVENT_INTERNAL;

/**************************************************************************//**
 * Suppressed NV pairs list entry.
 * JSON equivalent field: suppressedNvPairs
 *****************************************************************************/
typedef struct evel_suppressed_nv_pairs {

  /***************************************************************************/
  /* Mandatory fields                                                        */
  /* JSON equivalent field: nvPairFieldName                                  */
  /***************************************************************************/
  char * nv_pair_field_name;

  /***************************************************************************/
  /* Optional fields                                                         */
  /* JSON equivalent field: suppressedNvPairNames                            */
  /* Type of each list entry: char *                                         */
  /***************************************************************************/
  DLIST suppressed_nv_pair_names;

  /***************************************************************************/
  /* Hash table containing suppressed_nv_pair_names as keys.                 */
  /***************************************************************************/
  struct hsearch_data * hash_nv_pair_names;

} EVEL_SUPPRESSED_NV_PAIRS;

/**************************************************************************//**
 * Event Throttling Specification for a domain which is in a throttled state.
 * JSON equivalent object: eventThrottlingState
 *****************************************************************************/
typedef struct evel_throttle_spec {

  /***************************************************************************/
  /* List of field names to be suppressed.                                   */
  /* JSON equivalent field: suppressedFieldNames                             */
  /* Type of each list entry: char *                                         */
  /***************************************************************************/
  DLIST suppressed_field_names;

  /***************************************************************************/
  /* List of name-value pairs to be suppressed.                              */
  /* JSON equivalent field: suppressedNvPairsList                            */
  /* Type of each list entry: EVEL_SUPPRESSED_NV_PAIRS *                     */
  /***************************************************************************/
  DLIST suppressed_nv_pairs_list;

  /***************************************************************************/
  /* Hash table containing suppressed_nv_pair_names as keys.                 */
  /***************************************************************************/
  struct hsearch_data * hash_field_names;

  /***************************************************************************/
  /* Hash table containing nv_pair_field_name as keys, and                   */
  /* suppressed_nv_pairs_list as values.                                     */
  /***************************************************************************/
  struct hsearch_data * hash_nv_pairs_list;

} EVEL_THROTTLE_SPEC;

/*****************************************************************************/
/* RFC2822 format string for strftime.                                       */
/*****************************************************************************/
#define EVEL_RFC2822_STRFTIME_FORMAT "%a, %d %b %Y %T %z"

/*****************************************************************************/
/* EVEL_JSON_BUFFER depth at which we throttle fields.                       */
/*****************************************************************************/
#define EVEL_THROTTLE_FIELD_DEPTH 3

/**************************************************************************//**
 * Initialize the event handler.
 *
 * Primarily responsible for getting cURL ready for use.
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
                                        int verbosity);

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
EVEL_ERR_CODES event_handler_terminate();

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
EVEL_ERR_CODES event_handler_run();

/**************************************************************************//**
 * Create a new internal event.
 *
 * @note    The mandatory fields on the Fault must be supplied to this factory
 *          function and are immutable once set.  Optional fields have explicit
 *          setter functions, but again values may only be set once so that the
 *          Fault has immutable properties.
 * @param   command   The condition indicated by the event.
 * @returns pointer to the newly manufactured ::EVENT_INTERNAL.  If the event
 *          is not used (i.e. posted) it must be released using
 *          ::evel_free_event.
 * @retval  NULL  Failed to create the event.
 *****************************************************************************/
EVENT_INTERNAL * evel_new_internal_event(EVT_HANDLER_COMMAND command,const char* ev_name, const char *ev_id);

/**************************************************************************//**
 * Free an internal event.
 *
 * Free off the event supplied.  Will free all the contained* allocated memory.
 *
 * @note It does not free the internal event itself, since that may be part of
 * a larger structure.
 *****************************************************************************/
void evel_free_internal_event(EVENT_INTERNAL * event);

/*****************************************************************************/
/* Structure to hold JSON buffer and associated tracking, as it is written.  */
/*****************************************************************************/
typedef struct evel_json_buffer
{
  char * json;
  int offset;
  int max_size;

  /***************************************************************************/
  /* The working throttle specification, which can be NULL.                  */
  /***************************************************************************/
  EVEL_THROTTLE_SPEC * throttle_spec;

  /***************************************************************************/
  /* Current object/list nesting depth.                                      */
  /***************************************************************************/
  int depth;

  /***************************************************************************/
  /* The checkpoint.                                                         */
  /***************************************************************************/
  int checkpoint;

} EVEL_JSON_BUFFER;

/**************************************************************************//**
 * Encode the event as a JSON event object according to AT&T's schema.
 *
 * @param jbuf          Pointer to the ::EVEL_JSON_BUFFER to encode into.
 * @param event         Pointer to the ::EVENT_HEADER to encode.
 *****************************************************************************/
void evel_json_encode_header(EVEL_JSON_BUFFER * jbuf,
                             EVENT_HEADER * event);

/**************************************************************************//**
 * Encode the fault in JSON according to AT&T's schema for the fault type.
 *
 * @param jbuf          Pointer to the ::EVEL_JSON_BUFFER to encode into.
 * @param event         Pointer to the ::EVENT_HEADER to encode.
 *****************************************************************************/
void evel_json_encode_fault(EVEL_JSON_BUFFER * jbuf,
                            EVENT_FAULT * event);

/**************************************************************************//**
 * Encode the measurement as a JSON measurement.
 *
 * @param jbuf          Pointer to the ::EVEL_JSON_BUFFER to encode into.
 * @param event         Pointer to the ::EVENT_HEADER to encode.
 *****************************************************************************/
void evel_json_encode_measurement(EVEL_JSON_BUFFER * jbuf,
                                  EVENT_MEASUREMENT * event);

/**************************************************************************//**
 * Encode the Mobile Flow in JSON according to AT&T's schema for the event
 * type.
 *
 * @param jbuf          Pointer to the ::EVEL_JSON_BUFFER to encode into.
 * @param event         Pointer to the ::EVENT_HEADER to encode.
 *****************************************************************************/
void evel_json_encode_mobile_flow(EVEL_JSON_BUFFER * jbuf,
                                  EVENT_MOBILE_FLOW * event);

/**************************************************************************//**
 * Encode the report as a JSON report.
 *
 * @param jbuf          Pointer to the ::EVEL_JSON_BUFFER to encode into.
 * @param event         Pointer to the ::EVENT_HEADER to encode.
 *****************************************************************************/
void evel_json_encode_report(EVEL_JSON_BUFFER * jbuf,
                             EVENT_REPORT * event);

/**************************************************************************//**
 * Encode the Heartbeat fields in JSON according to AT&T's schema for the
 * event type.
 *
 * @param jbuf          Pointer to the ::EVEL_JSON_BUFFER to encode into.
 * @param event         Pointer to the ::EVENT_HEADER to encode.
 *****************************************************************************/
void evel_json_encode_hrtbt_field(EVEL_JSON_BUFFER * const jbuf,
                                EVENT_HEARTBEAT_FIELD * const event);

/**************************************************************************//**
 * Encode the Signaling in JSON according to AT&T's schema for the event type.
 *
 * @param jbuf          Pointer to the ::EVEL_JSON_BUFFER to encode into.
 * @param event         Pointer to the ::EVENT_HEADER to encode.
 *****************************************************************************/
void evel_json_encode_signaling(EVEL_JSON_BUFFER * const jbuf,
                                EVENT_SIGNALING * const event);

/**************************************************************************//**
 * Encode the state change as a JSON state change.
 *
 * @param jbuf          Pointer to the ::EVEL_JSON_BUFFER to encode into.
 * @param state_change  Pointer to the ::EVENT_STATE_CHANGE to encode.
 *****************************************************************************/
void evel_json_encode_state_change(EVEL_JSON_BUFFER * jbuf,
                                   EVENT_STATE_CHANGE * state_change);

/**************************************************************************//**
 * Encode the Syslog in JSON according to AT&T's schema for the event type.
 *
 * @param jbuf          Pointer to the ::EVEL_JSON_BUFFER to encode into.
 * @param event         Pointer to the ::EVENT_HEADER to encode.
 *****************************************************************************/
void evel_json_encode_syslog(EVEL_JSON_BUFFER * jbuf,
                             EVENT_SYSLOG * event);

/**************************************************************************//**
 * Encode the Other in JSON according to AT&T's schema for the event type.
 *
 * @param jbuf          Pointer to the ::EVEL_JSON_BUFFER to encode into.
 * @param event         Pointer to the ::EVENT_HEADER to encode.
 *****************************************************************************/
void evel_json_encode_other(EVEL_JSON_BUFFER * jbuf,
                            EVENT_OTHER * event);

/**************************************************************************//**
 * Set the next event_sequence to use.
 *
 * @param sequence      The next sequence number to use.
 *****************************************************************************/
void evel_set_next_event_sequence(const int sequence);

/**************************************************************************//**
 * Handle a JSON response from the listener, contained in a ::MEMORY_CHUNK.
 *
 * Tokenize the response, and decode any tokens found.
 *
 * @param chunk         The memory chunk containing the response.
 * @param post          The memory chunk in which to place any resulting POST.
 *****************************************************************************/
void evel_handle_event_response(const MEMORY_CHUNK * const chunk,
                                MEMORY_CHUNK * const post);

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
                           EVEL_THROTTLE_SPEC * throttle_spec);

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
                            const EVEL_OPTION_STRING * const option);

/**************************************************************************//**
 * Encode a string key and string value to a ::EVEL_JSON_BUFFER.
 *
 * @param jbuf          Pointer to working ::EVEL_JSON_BUFFER.
 * @param key           Pointer to the key to encode.
 * @param value         Pointer to the corresponding value to encode.
 *****************************************************************************/
void evel_enc_kv_string(EVEL_JSON_BUFFER * jbuf,
                        const char * const key,
                        const char * const value);

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
                         const EVEL_OPTION_INT * const option);

/**************************************************************************//**
 * Encode a string key and integer value to a ::EVEL_JSON_BUFFER.
 *
 * @param jbuf          Pointer to working ::EVEL_JSON_BUFFER.
 * @param key           Pointer to the key to encode.
 * @param value         The corresponding value to encode.
 *****************************************************************************/
void evel_enc_kv_int(EVEL_JSON_BUFFER * jbuf,
                     const char * const key,
                     const int value);

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
                            const EVEL_OPTION_DOUBLE * const option);

/**************************************************************************//**
 * Encode a string key and double value to a ::EVEL_JSON_BUFFER.
 *
 * @param jbuf          Pointer to working ::EVEL_JSON_BUFFER.
 * @param key           Pointer to the key to encode.
 * @param value         The corresponding value to encode.
 *****************************************************************************/
void evel_enc_kv_double(EVEL_JSON_BUFFER * jbuf,
                        const char * const key,
                        const double value);

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
                         const EVEL_OPTION_ULL * const option);

/**************************************************************************//**
 * Encode a string key and unsigned long long value to a ::EVEL_JSON_BUFFER.
 *
 * @param jbuf          Pointer to working ::EVEL_JSON_BUFFER.
 * @param key           Pointer to the key to encode.
 * @param value         The corresponding value to encode.
 *****************************************************************************/
void evel_enc_kv_ull(EVEL_JSON_BUFFER * jbuf,
                     const char * const key,
                     const unsigned long long value);

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
                          const EVEL_OPTION_TIME * const option);

/**************************************************************************//**
 * Encode a string key and time value to a ::EVEL_JSON_BUFFER.
 *
 * @param jbuf          Pointer to working ::EVEL_JSON_BUFFER.
 * @param key           Pointer to the key to encode.
 * @param time          Pointer to the time to encode.
 *****************************************************************************/
void evel_enc_kv_time(EVEL_JSON_BUFFER * jbuf,
                      const char * const key,
                      const time_t * time);

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
                      const int minor_version);

/**************************************************************************//**
 * Add the key and opening bracket of an optional named list to a JSON buffer.
 *
 * @param jbuf          Pointer to working ::EVEL_JSON_BUFFER.
 * @param key           Pointer to the key to encode.
 * @return true if the list was opened, false if it was suppressed.
 *****************************************************************************/
bool evel_json_open_opt_named_list(EVEL_JSON_BUFFER * jbuf,
                                   const char * const key);

/**************************************************************************//**
 * Add the key and opening bracket of a named list to a JSON buffer.
 *
 * @param jbuf          Pointer to working ::EVEL_JSON_BUFFER.
 * @param key           Pointer to the key to encode.
 *****************************************************************************/
void evel_json_open_named_list(EVEL_JSON_BUFFER * jbuf,
                               const char * const key);

/**************************************************************************//**
 * Add the closing bracket of a list to a JSON buffer.
 *
 * @param jbuf          Pointer to working ::EVEL_JSON_BUFFER.
 *****************************************************************************/
void evel_json_close_list(EVEL_JSON_BUFFER * jbuf);

/**************************************************************************//**
 * Encode a list item with format and param list to a ::EVEL_JSON_BUFFER.
 *
 * @param jbuf          Pointer to working ::EVEL_JSON_BUFFER.
 * @param format        Format string in standard printf format.
 * @param ...           Variable parameters for format string.
 *****************************************************************************/
void evel_enc_list_item(EVEL_JSON_BUFFER * jbuf,
                        const char * const format,
                        ...);

/**************************************************************************//**
 * Add the opening bracket of an optional named object to a JSON buffer.
 *
 * @param jbuf          Pointer to working ::EVEL_JSON_BUFFER.
 * @param key           Pointer to the key to encode.
 *****************************************************************************/
bool evel_json_open_opt_named_object(EVEL_JSON_BUFFER * jbuf,
                                     const char * const key);

/**************************************************************************//**
 * Add the opening bracket of an object to a JSON buffer.
 *
 * @param jbuf          Pointer to working ::EVEL_JSON_BUFFER.
 * @param key           Pointer to the key to encode.
 * @return true if the object was opened, false if it was suppressed.
 *****************************************************************************/
void evel_json_open_named_object(EVEL_JSON_BUFFER * jbuf,
                                 const char * const key);

/**************************************************************************//**
 * Add the opening bracket of an object to a JSON buffer.
 *
 * @param jbuf          Pointer to working ::EVEL_JSON_BUFFER.
 *****************************************************************************/
void evel_json_open_object(EVEL_JSON_BUFFER * jbuf);

/**************************************************************************//**
 * Add the closing bracket of an object to a JSON buffer.
 *
 * @param jbuf          Pointer to working ::EVEL_JSON_BUFFER.
 *****************************************************************************/
void evel_json_close_object(EVEL_JSON_BUFFER * jbuf);

/**************************************************************************//**
 * Add a checkpoint - a stake in the ground to which we can rewind.
 *
 * @param jbuf          Pointer to working ::EVEL_JSON_BUFFER.
 *****************************************************************************/
void evel_json_checkpoint(EVEL_JSON_BUFFER * jbuf);

/**************************************************************************//**
 * Rewind to the latest checkoint.
 *
 * @param jbuf          Pointer to working ::EVEL_JSON_BUFFER.
 *****************************************************************************/
void evel_json_rewind(EVEL_JSON_BUFFER * jbuf);

/**************************************************************************//**
 * Free the underlying resources of an ::EVEL_OPTION_STRING.
 *
 * @param option        Pointer to the ::EVEL_OPTION_STRING.
 *****************************************************************************/
void evel_free_option_string(EVEL_OPTION_STRING * const option);

/**************************************************************************//**
 * Initialize an ::EVEL_OPTION_STRING to a not-set state.
 *
 * @param option        Pointer to the ::EVEL_OPTION_STRING.
 *****************************************************************************/
void evel_init_option_string(EVEL_OPTION_STRING * const option);

/**************************************************************************//**
 * Set the value of an ::EVEL_OPTION_STRING.
 *
 * @param option        Pointer to the ::EVEL_OPTION_STRING.
 * @param value         The value to set.
 * @param description   Description to be used in logging.
 *****************************************************************************/
void evel_set_option_string(EVEL_OPTION_STRING * const option,
                            const char * const value,
                            const char * const description);

/**************************************************************************//**
 * Force the value of an ::EVEL_OPTION_STRING.
 *
 * @param option        Pointer to the ::EVEL_OPTION_STRING.
 * @param value         The value to set.
 *****************************************************************************/
void evel_force_option_string(EVEL_OPTION_STRING * const option,
                              const char * const value);

/**************************************************************************//**
 * Initialize an ::EVEL_OPTION_INT to a not-set state.
 *
 * @param option        Pointer to the ::EVEL_OPTION_INT.
 *****************************************************************************/
void evel_init_option_int(EVEL_OPTION_INT * const option);

/**************************************************************************//**
 * Force the value of an ::EVEL_OPTION_INT.
 *
 * @param option        Pointer to the ::EVEL_OPTION_INT.
 * @param value         The value to set.
 *****************************************************************************/
void evel_force_option_int(EVEL_OPTION_INT * const option,
                           const int value);

/**************************************************************************//**
 * Set the value of an ::EVEL_OPTION_INT.
 *
 * @param option        Pointer to the ::EVEL_OPTION_INT.
 * @param value         The value to set.
 * @param description   Description to be used in logging.
 *****************************************************************************/
void evel_set_option_int(EVEL_OPTION_INT * const option,
                         const int value,
                         const char * const description);

/**************************************************************************//**
 * Initialize an ::EVEL_OPTION_DOUBLE to a not-set state.
 *
 * @param option        Pointer to the ::EVEL_OPTION_DOUBLE.
 *****************************************************************************/
void evel_init_option_double(EVEL_OPTION_DOUBLE * const option);

/**************************************************************************//**
 * Force the value of an ::EVEL_OPTION_DOUBLE.
 *
 * @param option        Pointer to the ::EVEL_OPTION_DOUBLE.
 * @param value         The value to set.
 *****************************************************************************/
void evel_force_option_double(EVEL_OPTION_DOUBLE * const option,
                              const double value);

/**************************************************************************//**
 * Set the value of an ::EVEL_OPTION_DOUBLE.
 *
 * @param option        Pointer to the ::EVEL_OPTION_DOUBLE.
 * @param value         The value to set.
 * @param description   Description to be used in logging.
 *****************************************************************************/
void evel_set_option_double(EVEL_OPTION_DOUBLE * const option,
                            const double value,
                            const char * const description);

/**************************************************************************//**
 * Initialize an ::EVEL_OPTION_ULL to a not-set state.
 *
 * @param option        Pointer to the ::EVEL_OPTION_ULL.
 *****************************************************************************/
void evel_init_option_ull(EVEL_OPTION_ULL * const option);

/**************************************************************************//**
 * Force the value of an ::EVEL_OPTION_ULL.
 *
 * @param option        Pointer to the ::EVEL_OPTION_ULL.
 * @param value         The value to set.
 *****************************************************************************/
void evel_force_option_ull(EVEL_OPTION_ULL * const option,
                           const unsigned long long value);

/**************************************************************************//**
 * Set the value of an ::EVEL_OPTION_ULL.
 *
 * @param option        Pointer to the ::EVEL_OPTION_ULL.
 * @param value         The value to set.
 * @param description   Description to be used in logging.
 *****************************************************************************/
void evel_set_option_ull(EVEL_OPTION_ULL * const option,
                         const unsigned long long value,
                         const char * const description);

/**************************************************************************//**
 * Initialize an ::EVEL_OPTION_TIME to a not-set state.
 *
 * @param option        Pointer to the ::EVEL_OPTION_TIME.
 *****************************************************************************/
void evel_init_option_time(EVEL_OPTION_TIME * const option);

/**************************************************************************//**
 * Force the value of an ::EVEL_OPTION_TIME.
 *
 * @param option        Pointer to the ::EVEL_OPTION_TIME.
 * @param value         The value to set.
 *****************************************************************************/
void evel_force_option_time(EVEL_OPTION_TIME * const option,
                            const time_t value);

/**************************************************************************//**
 * Set the value of an ::EVEL_OPTION_TIME.
 *
 * @param option        Pointer to the ::EVEL_OPTION_TIME.
 * @param value         The value to set.
 * @param description   Description to be used in logging.
 *****************************************************************************/
void evel_set_option_time(EVEL_OPTION_TIME * const option,
                          const time_t value,
                          const char * const description);

/**************************************************************************//**
 * Map an ::EVEL_COUNTER_CRITICALITIES enum value to the equivalent string.
 *
 * @param criticality   The criticality to convert.
 * @returns The equivalent string.
 *****************************************************************************/
char * evel_criticality(const EVEL_COUNTER_CRITICALITIES criticality);

/**************************************************************************//**
 * Map an ::EVEL_SEVERITIES enum value to the equivalent string.
 *
 * @param severity      The severity to convert.
 * @returns The equivalent string.
 *****************************************************************************/
char * evel_severity(const EVEL_SEVERITIES severity);

/**************************************************************************//**
 * Map an ::EVEL_ALERT_ACTIONS enum value to the equivalent string.
 *
 * @param alert_action  The alert_action to convert.
 * @returns The equivalent string.
 *****************************************************************************/
char * evel_alert_action(const EVEL_ALERT_ACTIONS alert_action);

/**************************************************************************//**
 * Map an ::EVEL_ALERT_TYPES enum value to the equivalent string.
 *
 * @param alert_type    The alert_type to convert.
 * @returns The equivalent string.
 *****************************************************************************/
char * evel_alert_type(const EVEL_ALERT_TYPES alert_type);

/**************************************************************************//**
 * Map an ::EVEL_EVENT_DOMAINS enum value to the equivalent string.
 *
 * @param domain        The domain to convert.
 * @returns The equivalent string.
 *****************************************************************************/
char * evel_event_domain(const EVEL_EVENT_DOMAINS domain);

/**************************************************************************//**
 * Map an ::EVEL_EVENT_PRIORITIES enum value to the equivalent string.
 *
 * @param priority      The priority to convert.
 * @returns The equivalent string.
 *****************************************************************************/
char * evel_event_priority(const EVEL_EVENT_PRIORITIES priority);

/**************************************************************************//**
 * Map an ::EVEL_SOURCE_TYPES enum value to the equivalent string.
 *
 * @param source_type   The source type to convert.
 * @returns The equivalent string.
 *****************************************************************************/
char * evel_source_type(const EVEL_SOURCE_TYPES source_type);

/**************************************************************************//**
 * Map an ::EVEL_VF_STATUSES enum value to the equivalent string.
 *
 * @param vf_status     The vf_status to convert.
 * @returns The equivalent string.
 *****************************************************************************/
char * evel_vf_status(const EVEL_VF_STATUSES vf_status);

/**************************************************************************//**
 * Convert a ::EVEL_ENTITY_STATE to it's string form for JSON encoding.
 *
 * @param state         The entity state to encode.
 *
 * @returns the corresponding string
 *****************************************************************************/
char * evel_entity_state(const EVEL_ENTITY_STATE state);

/**************************************************************************//**
 * Convert a ::EVEL_SERVICE_ENDPOINT_DESC to string form for JSON encoding.
 *
 * @param endpoint_desc endpoint description to encode.
 *
 * @returns the corresponding string
 *****************************************************************************/
char * evel_service_endpoint_desc(const EVEL_ENTITY_STATE endpoint_desc);


/**************************************************************************//**
 * Initialize an ::EVEL_OPTION_INTHEADER_FIELDS to a not-set state.
 *
 * @param option        Pointer to the ::EVEL_OPTION_INTHEADER_FIELDS.
 *****************************************************************************/
void evel_init_option_intheader(EVEL_OPTION_INTHEADER_FIELDS * const option);
/**************************************************************************//**
 * Force the value of an ::EVEL_OPTION_INTHEADER_FIELDS.
 *
 * @param option        Pointer to the ::EVEL_OPTION_INTHEADER_FIELDS.
 * @param value         The value to set.
 *****************************************************************************/
void evel_force_option_intheader(EVEL_OPTION_INTHEADER_FIELDS * const option,
                           const void* value);
/**************************************************************************//**
 * Set the value of an ::EVEL_OPTION_INTHEADER_FIELDS.
 *
 * @param option        Pointer to the ::EVEL_OPTION_INTHEADER_FIELDS.
 * @param value         The value to set.
 * @param description   Description to be used in logging.
 *****************************************************************************/
void evel_set_option_intheader(EVEL_OPTION_INTHEADER_FIELDS * const option,
                         const void * value,
                         const char * const description);
/**************************************************************************//**
 * Free the underlying resources of an ::EVEL_OPTION_INTHEADER_FIELDS.
 *
 * @param option        Pointer to the ::EVEL_OPTION_INTHEADER_FIELDS.
 *****************************************************************************/
void evel_free_option_intheader(EVEL_OPTION_INTHEADER_FIELDS * const option);

#endif
