#ifndef EVEL_THROTTLE_INCLUDED
#define EVEL_THROTTLE_INCLUDED

/**************************************************************************//**
 * @file
 * EVEL throttle definitions.
 *
 * These are internal definitions related to throttling specicications, which
 * are required within the library but are not intended for external
 * consumption.
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
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:  This product includes
 *    software developed by the AT&T.
 * 4. Neither the name of AT&T nor the names of its contributors may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
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

#include "evel_internal.h"
#include "jsmn.h"

/*****************************************************************************/
/* Maximum depth of JSON response that we can handle.                        */
/*****************************************************************************/
#define EVEL_JSON_STACK_DEPTH           10

/**************************************************************************//**
 * Maximum number of tokens that we allow for in a JSON response.
 *****************************************************************************/
#define EVEL_MAX_RESPONSE_TOKENS        1024

/**************************************************************************//**
 * The nature of the next token that we are iterating through.  Within an
 * object, we alternate between collecting keys and values.  Within an array,
 * we only collect items.
 *****************************************************************************/
typedef enum {
  EVEL_JSON_KEY,
  EVEL_JSON_VALUE,
  EVEL_JSON_ITEM
} EVEL_JSON_STATE;

/**************************************************************************//**
 * States which we move through during JSON processing, tracking our way
 * through the supported JSON structure.
 *****************************************************************************/
typedef enum
{
  /***************************************************************************/
  /* Initial state.                                                          */
  /***************************************************************************/
  EVEL_JCS_START,

  /***************************************************************************/
  /* {"commandList": [                                                       */
  /***************************************************************************/
  EVEL_JCS_COMMAND_LIST,

  /***************************************************************************/
  /* {"commandList": [{                                                      */
  /***************************************************************************/
  EVEL_JCS_COMMAND_LIST_ENTRY,

  /***************************************************************************/
  /* {"commandList": [{"command": {                                          */
  /***************************************************************************/
  EVEL_JCS_COMMAND,

  /***************************************************************************/
  /* ... "eventDomainThrottleSpecification": {                               */
  /***************************************************************************/
  EVEL_JCS_SPEC,

  /***************************************************************************/
  /* ... "suppressedFieldNames": [                                           */
  /***************************************************************************/
  EVEL_JCS_FIELD_NAMES,

  /***************************************************************************/
  /* ... "suppressedNvPairsList": [                                          */
  /***************************************************************************/
  EVEL_JCS_PAIRS_LIST,

  /***************************************************************************/
  /* ... "suppressedNvPairsList": [{                                         */
  /***************************************************************************/
  EVEL_JCS_PAIRS_LIST_ENTRY,

  /***************************************************************************/
  /* ... "suppressedNvPairNames": [                                          */
  /***************************************************************************/
  EVEL_JCS_NV_PAIR_NAMES,

  EVEL_JCS_MAX
} EVEL_JSON_COMMAND_STATE;

/**************************************************************************//**
 * An entry in the JSON stack.
 *****************************************************************************/
typedef struct evel_json_stack_entry {

  /***************************************************************************/
  /* The number of elements required at this level.                          */
  /***************************************************************************/
  int num_required;

  /***************************************************************************/
  /* The number of elements collected at this level.                         */
  /***************************************************************************/
  int json_count;

  /***************************************************************************/
  /* The collection state at this level in the JSON stack.                   */
  /***************************************************************************/
  EVEL_JSON_STATE json_state;

  /***************************************************************************/
  /* The key being collected (if json_state is EVEL_JSON_VALUE), or NULL.    */
  /***************************************************************************/
  char * json_key;

} EVEL_JSON_STACK_ENTRY;

/**************************************************************************//**
 * The JSON stack.
 *****************************************************************************/
typedef struct evel_json_stack {

  /***************************************************************************/
  /* The current position of the stack - starting at zero.                   */
  /***************************************************************************/
  int level;

  /***************************************************************************/
  /* The stack itself.                                                       */
  /***************************************************************************/
  EVEL_JSON_STACK_ENTRY entry[EVEL_JSON_STACK_DEPTH];

  /***************************************************************************/
  /* The underlying memory chunk.                                            */
  /***************************************************************************/
  const MEMORY_CHUNK * chunk;

} EVEL_JSON_STACK;

/**************************************************************************//**
 * Initialize event throttling to the default state.
 *
 * Called from ::evel_initialize.
 *****************************************************************************/
void evel_throttle_initialize();

/**************************************************************************//**
 * Clean up event throttling.
 *
 * Called from ::evel_terminate.
 *****************************************************************************/
void evel_throttle_terminate();

/**************************************************************************//**
 * Handle a JSON response from the listener, as a list of tokens from JSMN.
 *
 * @param chunk         Memory chunk containing the JSON buffer.
 * @param json_tokens   Array of tokens to handle.
 * @param num_tokens    The number of tokens to handle.
 * @param post          The memory chunk in which to place any resulting POST.
 * @return true if the command was handled, false otherwise.
 *****************************************************************************/
bool evel_handle_command_list(const MEMORY_CHUNK * const chunk,
                              const jsmntok_t * const json_tokens,
                              const int num_tokens,
                              MEMORY_CHUNK * const post);

/**************************************************************************//**
 * Return the ::EVEL_THROTTLE_SPEC for a given domain.
 *
 * @param domain        The domain for which to return state.
 *****************************************************************************/
EVEL_THROTTLE_SPEC * evel_get_throttle_spec(EVEL_EVENT_DOMAINS domain);

/**************************************************************************//**
 * Determine whether a field_name should be suppressed.
 *
 * @param throttle_spec Throttle specification for the domain being encoded.
 * @param field_name    The field name to encoded or suppress.
 * @return true if the field_name should be suppressed, false otherwise.
 *****************************************************************************/
bool evel_throttle_suppress_field(EVEL_THROTTLE_SPEC * throttle_spec,
                                  const char * const field_name);

/**************************************************************************//**
 * Determine whether a name-value pair should be allowed (not suppressed).
 *
 * @param throttle_spec Throttle specification for the domain being encoded.
 * @param field_name    The field name holding the name-value pairs.
 * @param name          The name of the name-value pair to encoded or suppress.
 * @return true if the name-value pair should be suppressed, false otherwise.
 *****************************************************************************/
bool evel_throttle_suppress_nv_pair(EVEL_THROTTLE_SPEC * throttle_spec,
                                    const char * const field_name,
                                    const char * const name);

#endif
