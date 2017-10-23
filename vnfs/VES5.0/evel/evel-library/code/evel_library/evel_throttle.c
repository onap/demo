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

#define _GNU_SOURCE
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <limits.h>
#include <pthread.h>
#include <search.h>

#include "evel_throttle.h"

/*****************************************************************************/
/* The Event Throttling State for all domains, indexed by                    */
/* ::EVEL_EVENT_DOMAINS, corresponding to JSON eventDomain.                  */
/*                                                                           */
/* A given domain is in a throttled state if ::evel_throttle_spec is         */
/* non-NULL.                                                                 */
/*****************************************************************************/
static EVEL_THROTTLE_SPEC * evel_throttle_spec[EVEL_MAX_DOMAINS];

/*****************************************************************************/
/* The current measurement interval.  Default: MEASUREMENT_INTERVAL_UKNOWN.  */
/* Must be protected by evel_measurement_interval_mutex.                     */
/*****************************************************************************/
static int evel_measurement_interval;

/*****************************************************************************/
/* Mutex protecting evel_measurement_interval from contention between an     */
/* EVEL client reading it, and the EVEL event handler updating it.           */
/*****************************************************************************/
static pthread_mutex_t evel_measurement_interval_mutex;

/*****************************************************************************/
/* Flag stating that we have received a "provideThrottlingState" command.    */
/* Set during JSON processing and cleared on sending the throttling state.   */
/*****************************************************************************/
static bool evel_provide_throttling_state;

/*****************************************************************************/
/* Holder for the "commandType" value during JSON processing.                */
/*****************************************************************************/
static char * evel_command_type_value;

/*****************************************************************************/
/* Holder for the "measurementInterval" value during JSON processing.        */
/*****************************************************************************/
static char * evel_measurement_interval_value;

/*****************************************************************************/
/* Holder for the "eventDomain" value during JSON processing.                */
/*****************************************************************************/
static char * evel_throttle_spec_domain_value;

/*****************************************************************************/
/* Decoded version of ::evel_throttle_spec_domain_value.                     */
/*****************************************************************************/
static EVEL_EVENT_DOMAINS evel_throttle_spec_domain;

/*****************************************************************************/
/* During JSON processing of a single throttling specification, we collect   */
/* parameters in this working ::EVEL_THROTTLE_SPEC                           */
/*****************************************************************************/
static EVEL_THROTTLE_SPEC * evel_temp_throttle;

/*****************************************************************************/
/* State tracking our progress through the command list                      */
/*****************************************************************************/
EVEL_JSON_COMMAND_STATE evel_json_command_state;

/*****************************************************************************/
/* Debug strings for ::EVEL_JSON_COMMAND_STATE.                              */
/*****************************************************************************/
static const char * const evel_jcs_strings[EVEL_JCS_MAX] = {
  "EVEL_JCS_START",
  "EVEL_JCS_COMMAND_LIST",
  "EVEL_JCS_COMMAND_LIST_ENTRY",
  "EVEL_JCS_COMMAND",
  "EVEL_JCS_SPEC",
  "EVEL_JCS_FIELD_NAMES",
  "EVEL_JCS_PAIRS_LIST",
  "EVEL_JCS_PAIRS_LIST_ENTRY",
  "EVEL_JCS_NV_PAIR_NAMES"
};

/*****************************************************************************/
/* Debug strings for JSON token type.                                        */
/*****************************************************************************/
#define JSON_TOKEN_TYPES                (JSMN_PRIMITIVE + 1)
static const char * const evel_json_token_strings[JSON_TOKEN_TYPES] = {
  "JSMN_UNDEFINED",
  "JSMN_OBJECT",
  "JSMN_ARRAY",
  "JSMN_STRING",
  "JSMN_PRIMITIVE"
};

/*****************************************************************************/
/* Debug strings for JSON domains.                                           */
/*****************************************************************************/
static const char * evel_domain_strings[EVEL_MAX_DOMAINS] = {
  "internal",
  "heartbeat",
  "fault",
  "measurementsForVfScaling",
  "mobileFlow",
  "report",
  "serviceEvents",
  "signaling",
  "stateChange",
  "syslog",
  "other"
  "voiceQuality",
  "maxDomain"
};

/*****************************************************************************/
/* Local prototypes.                                                         */
/*****************************************************************************/
static void evel_throttle_finalize(EVEL_THROTTLE_SPEC * throttle_spec);
static struct hsearch_data * evel_throttle_hash_create(DLIST * hash_keys);
static void evel_throttle_free(EVEL_THROTTLE_SPEC * throttle_spec);
static void evel_throttle_free_nv_pair(EVEL_SUPPRESSED_NV_PAIRS * nv_pairs);
static void evel_init_json_stack(EVEL_JSON_STACK * json_stack,
                                 const MEMORY_CHUNK * const chunk);
static bool evel_stack_push(EVEL_JSON_STACK * const json_stack,
                            const int num_required,
                            const EVEL_JSON_STATE new_state);
static void evel_stack_pop(EVEL_JSON_STACK * const json_stack);
static void evel_stack_cleanup(EVEL_JSON_STACK * const json_stack);
static char * evel_stack_strdup(const MEMORY_CHUNK * const chunk,
                                const jsmntok_t * const token);
static void evel_stack_store_key(EVEL_JSON_STACK * const json_stack,
                                 const jsmntok_t * const token);
static void evel_stack_store_value(EVEL_JSON_STACK * const json_stack,
                                   const jsmntok_t * const token);
static void evel_stack_store_item(EVEL_JSON_STACK * const json_stack,
                                  const jsmntok_t * const token);
static void evel_set_command_state(const EVEL_JSON_COMMAND_STATE new_state);
static void evel_debug_token(const MEMORY_CHUNK * const chunk,
                             const jsmntok_t * const token);
static void evel_command_list_response(MEMORY_CHUNK * const post);
static int evel_json_encode_throttle(char * const json, const int max_size);
static int evel_json_encode_throttle_spec(char * const json,
                                          const int max_size,
                                          const EVEL_EVENT_DOMAINS domain);
static int evel_json_encode_nv_pairs(char * const json,
                                     const int max_size,
                                     EVEL_SUPPRESSED_NV_PAIRS * nv_pairs);
static void evel_close_command();
static void evel_open_command();
static void evel_set_throttling_spec();
static void evel_set_measurement_interval();
static void evel_open_throttle_spec();
static void evel_close_throttle_spec();
static EVEL_EVENT_DOMAINS evel_decode_domain(char * domain_value);
static void evel_open_nv_pairs_list_entry();
static void evel_close_nv_pairs_list_entry();
static void evel_store_nv_pair_field_name(char * const value);
static void evel_store_nv_pair_name(char * const item);
static void evel_store_suppressed_field_name(char * const item);
static EVEL_SUPPRESSED_NV_PAIRS * evel_get_last_nv_pairs();

/**************************************************************************//**
 * Return the current measurement interval provided by the Event Listener.
 *
 * @returns The current measurement interval
 * @retval  EVEL_MEASUREMENT_INTERVAL_UKNOWN (0) - interval has not been
 *          specified
 *****************************************************************************/
int evel_get_measurement_interval()
{
  int result;

  EVEL_ENTER();

  /***************************************************************************/
  /* Lock, read, unlock.                                                     */
  /***************************************************************************/
  pthread_mutex_lock(&evel_measurement_interval_mutex);
  result = evel_measurement_interval;
  pthread_mutex_unlock(&evel_measurement_interval_mutex);

  EVEL_EXIT();

  return result;
}

/**************************************************************************//**
 * Return the ::EVEL_THROTTLE_SPEC for a given domain.
 *
 * @param domain        The domain for which to return state.
 *****************************************************************************/
EVEL_THROTTLE_SPEC * evel_get_throttle_spec(EVEL_EVENT_DOMAINS domain)
{
  EVEL_THROTTLE_SPEC * result;

  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(domain < EVEL_MAX_DOMAINS);

  result = evel_throttle_spec[domain];

  EVEL_EXIT();

  return result;
}

/**************************************************************************//**
 * Determine whether a field_name should be suppressed.
 *
 * @param throttle_spec Throttle specification for the domain being encoded.
 * @param field_name    The field name to encoded or suppress.
 * @return true if the field_name should be suppressed, false otherwise.
 *****************************************************************************/
bool evel_throttle_suppress_field(EVEL_THROTTLE_SPEC * throttle_spec,
                                  const char * const field_name)
{
  bool suppress = false;

  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(field_name != NULL);

  /***************************************************************************/
  /* If the throttle spec and hash table exist, query the field_names table. */
  /***************************************************************************/
  if ((throttle_spec != NULL) && (throttle_spec->hash_field_names != NULL))
  {
    ENTRY hash_query;
    ENTRY * hash_result;
    hash_query.key = (char * const) field_name;
    suppress = (hsearch_r(hash_query,
                          FIND,
                          &hash_result,
                          throttle_spec->hash_field_names) != 0);
  }

  EVEL_EXIT();

  return suppress;
}

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
                                    const char * const name)
{
  EVEL_SUPPRESSED_NV_PAIRS * nv_pairs;
  bool hit = false;
  bool suppress = false;

  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(field_name != NULL);
  assert(name != NULL);

  /***************************************************************************/
  /* If the throttle spec and hash table exist, query the nv_pairs table.    */
  /***************************************************************************/
  if ((throttle_spec != NULL) && (throttle_spec->hash_nv_pairs_list != NULL))
  {
    ENTRY hash_query;
    ENTRY * hash_result;
    hash_query.key = (char * const) field_name;
    hit = (hsearch_r(hash_query,
                     FIND,
                     &hash_result,
                     throttle_spec->hash_nv_pairs_list) != 0);
    if (hit)
    {
      nv_pairs = hash_result->data;
    }
  }

  /***************************************************************************/
  /* If we got a hit, and the nv_pairs and hash table exist, query the       */
  /* nv_pairs table.                                                         */
  /***************************************************************************/
  if (hit && (nv_pairs != NULL) && (nv_pairs->hash_nv_pair_names != NULL))
  {
    ENTRY hash_query;
    ENTRY * hash_result;
    hash_query.key = (char * const) name;
    suppress = (hsearch_r(hash_query,
                          FIND,
                          &hash_result,
                          nv_pairs->hash_nv_pair_names) != 0);
  }

  EVEL_EXIT();

  return suppress;
}

/**************************************************************************//**
 * Initialize event throttling to the default state.
 *
 * Called from ::evel_initialize.
 *****************************************************************************/
void evel_throttle_initialize()
{
  int pthread_rc;
  int ii;

  EVEL_ENTER();

  for (ii = 0; ii < EVEL_MAX_DOMAINS; ii++)
  {
    evel_throttle_spec[ii] = NULL;
  }

  pthread_rc = pthread_mutex_init(&evel_measurement_interval_mutex, NULL);
  assert(pthread_rc == 0);

  evel_measurement_interval = EVEL_MEASUREMENT_INTERVAL_UKNOWN;

  EVEL_EXIT();
}

/**************************************************************************//**
 * Clean up event throttling.
 *
 * Called from ::evel_terminate.
 *****************************************************************************/
void evel_throttle_terminate()
{
  int pthread_rc;
  int ii;

  EVEL_ENTER();

  for (ii = 0; ii < EVEL_MAX_DOMAINS; ii++)
  {
    if (evel_throttle_spec[ii] != NULL)
    {
      evel_throttle_free(evel_throttle_spec[ii]);
      evel_throttle_spec[ii] = NULL;
    }
  }

  pthread_rc = pthread_mutex_destroy(&evel_measurement_interval_mutex);
  assert(pthread_rc == 0);

  EVEL_EXIT();
}

/**************************************************************************//**
 * Finalize a single ::EVEL_THROTTLE_SPEC.
 *
 * Now that the specification is collected, build hash tables to simplify the
 * throttling itself.
 *
 * @param throttle_spec The ::EVEL_THROTTLE_SPEC to finalize.
 *****************************************************************************/
void evel_throttle_finalize(EVEL_THROTTLE_SPEC * throttle_spec)
{
  int nv_pairs_count;
  DLIST_ITEM * dlist_item;
  ENTRY * add_result;

  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(throttle_spec != NULL);

  /***************************************************************************/
  /* Populate the hash table for suppressed field names.                     */
  /***************************************************************************/
  throttle_spec->hash_field_names =
             evel_throttle_hash_create(&throttle_spec->suppressed_field_names);

  /***************************************************************************/
  /* Create the hash table for suppressed nv pairs.                          */
  /***************************************************************************/
  nv_pairs_count = dlist_count(&throttle_spec->suppressed_nv_pairs_list);
  if (nv_pairs_count > 0)
  {
    throttle_spec->hash_nv_pairs_list = calloc(1, sizeof(struct hsearch_data));
    assert(throttle_spec->hash_nv_pairs_list != NULL);

    /*************************************************************************/
    /* Provide plenty of space in the table - see hcreate_r notes.           */
    /*************************************************************************/
    if (hcreate_r(nv_pairs_count * 2, throttle_spec->hash_nv_pairs_list) == 0)
    {
      EVEL_ERROR("Failed to create hash table");
      free(throttle_spec->hash_nv_pairs_list);
      throttle_spec->hash_nv_pairs_list = NULL;
    }
  }

  /***************************************************************************/
  /* Populate the hash tables under suppressed field names.                  */
  /***************************************************************************/
  dlist_item = dlist_get_first(&throttle_spec->suppressed_nv_pairs_list);
  while (dlist_item != NULL)
  {
    EVEL_SUPPRESSED_NV_PAIRS * nv_pairs = dlist_item->item;
    ENTRY hash_add;

    /*************************************************************************/
    /* Set the key to the string, and the item to the nv_pairs.              */
    /*************************************************************************/
    assert(nv_pairs != NULL);
    hash_add.key = nv_pairs->nv_pair_field_name;
    hash_add.data = nv_pairs;
    hsearch_r(hash_add, ENTER, &add_result, throttle_spec->hash_nv_pairs_list);

    /*************************************************************************/
    /* Create the nv_pair_names hash since we're in here.                    */
    /*************************************************************************/
    nv_pairs->hash_nv_pair_names =
      evel_throttle_hash_create(&nv_pairs->suppressed_nv_pair_names);

    dlist_item = dlist_get_next(dlist_item);
  }

  EVEL_EXIT();
}

/**************************************************************************//**
 * Create and populate a hash table from a DLIST of keys.
 *
 * @param hash_keys     Pointer to a DLIST of hash table keys.
 * @return Pointer to the created hash-table, or NULL on failure.
 *****************************************************************************/
struct hsearch_data * evel_throttle_hash_create(DLIST * hash_keys)
{
  int key_count;
  struct hsearch_data * hash_table;
  ENTRY * add_result;

  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(hash_keys != NULL);

  /***************************************************************************/
  /* Count the keys and if there are any, populate the hash table with them. */
  /***************************************************************************/
  key_count = dlist_count(hash_keys);
  if (key_count > 0)
  {
    EVEL_DEBUG("Populating table for %d keys", key_count);

    hash_table = calloc(1, sizeof(struct hsearch_data));
    assert(hash_table != NULL);

    /*************************************************************************/
    /* We need to leave plenty of space in the table - see hcreate_r notes.  */
    /*************************************************************************/
    if (hcreate_r(key_count * 2, hash_table) != 0)
    {
      DLIST_ITEM * dlist_item;
      dlist_item = dlist_get_first(hash_keys);
      while (dlist_item != NULL)
      {
        assert(dlist_item->item != NULL);

        /*********************************************************************/
        /* Set the key and data to the item, which is a string in this case. */
        /*********************************************************************/
        ENTRY hash_add;
        hash_add.key = dlist_item->item;
        hash_add.data = dlist_item->item;
        hsearch_r(hash_add, ENTER, &add_result, hash_table);
        dlist_item = dlist_get_next(dlist_item);
      }
    }
    else
    {
      EVEL_ERROR("Failed to create hash table");
      free(hash_table);
      hash_table = NULL;
    }
  }
  else
  {
    hash_table = NULL;
  }

  EVEL_EXIT();

  return hash_table;
}

/**************************************************************************//**
 * Free resources associated with a single ::EVEL_THROTTLE_SPEC.
 *
 * @param throttle_spec The ::EVEL_THROTTLE_SPEC to free.
 *****************************************************************************/
void evel_throttle_free(EVEL_THROTTLE_SPEC * throttle_spec)
{
  char * field_name;
  EVEL_SUPPRESSED_NV_PAIRS * nv_pairs;

  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(throttle_spec != NULL);

  /***************************************************************************/
  /* Free any hash tables.                                                   */
  /***************************************************************************/
  if (throttle_spec->hash_field_names != NULL)
  {
    hdestroy_r(throttle_spec->hash_field_names);
    free(throttle_spec->hash_field_names);
  }
  if (throttle_spec->hash_nv_pairs_list != NULL)
  {
    hdestroy_r(throttle_spec->hash_nv_pairs_list);
    free(throttle_spec->hash_nv_pairs_list);
  }

  /***************************************************************************/
  /* Iterate through the linked lists, freeing memory.                       */
  /***************************************************************************/
  field_name = dlist_pop_last(&throttle_spec->suppressed_field_names);
  while (field_name != NULL)
  {
    free(field_name);
    field_name = dlist_pop_last(&throttle_spec->suppressed_field_names);
  }

  nv_pairs = dlist_pop_last(&throttle_spec->suppressed_nv_pairs_list);
  while (nv_pairs != NULL)
  {
    evel_throttle_free_nv_pair(nv_pairs);
    nv_pairs = dlist_pop_last(&throttle_spec->suppressed_nv_pairs_list);
  }

  free(throttle_spec);

  EVEL_EXIT();
}

/**************************************************************************//**
 * Free resources associated with a single ::EVEL_SUPPRESSED_NV_PAIR.
 *
 * @param nv_pair       The ::EVEL_SUPPRESSED_NV_PAIR to free.
 *****************************************************************************/
void evel_throttle_free_nv_pair(EVEL_SUPPRESSED_NV_PAIRS * nv_pairs)
{
  char * suppressed_name;

  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(nv_pairs != NULL);

  /***************************************************************************/
  /* Free any hash tables.                                                   */
  /***************************************************************************/
  if (nv_pairs->hash_nv_pair_names != NULL)
  {
    hdestroy_r(nv_pairs->hash_nv_pair_names);
    free(nv_pairs->hash_nv_pair_names);
  }

  /***************************************************************************/
  /* Iterate through the linked lists, freeing memory.                       */
  /***************************************************************************/
  suppressed_name = dlist_pop_last(&nv_pairs->suppressed_nv_pair_names);
  while (suppressed_name != NULL)
  {
    free(suppressed_name);
    suppressed_name = dlist_pop_last(&nv_pairs->suppressed_nv_pair_names);
  }
  if (nv_pairs->nv_pair_field_name != NULL)
  {
    free(nv_pairs->nv_pair_field_name);
  }
  free(nv_pairs);

  EVEL_EXIT();
}

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
                              MEMORY_CHUNK * const post)
{
  EVEL_JSON_STACK stack;
  EVEL_JSON_STACK * json_stack = &stack;
  EVEL_JSON_STACK_ENTRY * entry;

  bool json_ok = true;
  int token_index = 0;

  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(chunk != NULL);
  assert(json_tokens != NULL);
  assert(num_tokens < EVEL_MAX_RESPONSE_TOKENS);

  /***************************************************************************/
  /* Collect one top-level item.                                             */
  /***************************************************************************/
  evel_init_json_stack(json_stack, chunk);

  /***************************************************************************/
  /* Initialize JSON processing variables.                                   */
  /***************************************************************************/
  evel_provide_throttling_state = false;
  evel_command_type_value = NULL;
  evel_measurement_interval_value = NULL;
  evel_throttle_spec_domain_value = NULL;
  evel_throttle_spec_domain = EVEL_MAX_DOMAINS;
  evel_temp_throttle = NULL;
  evel_json_command_state = EVEL_JCS_START;

  /***************************************************************************/
  /* Loop through the tokens, keeping a stack of state representing the      */
  /* nested JSON structure (see json_state). We also track our way through   */
  /* the ::EVEL_JSON_COMMAND_STATE as we go.                                 */
  /***************************************************************************/
  while (json_ok && (token_index < num_tokens))
  {
    const jsmntok_t * const token = &json_tokens[token_index];

    if (EVEL_DEBUG_ON())
    {
      evel_debug_token(chunk, token);
    }

    /*************************************************************************/
    /* We may have popped or pushed, so always re-evaluate the stack entry.  */
    /*************************************************************************/
    entry = &json_stack->entry[json_stack->level];

    switch(token->type)
    {
      case JSMN_OBJECT:
        if ((entry->json_state == EVEL_JSON_ITEM) ||
            (entry->json_state == EVEL_JSON_VALUE))
        {
          json_ok = evel_stack_push(json_stack, token->size, EVEL_JSON_KEY);
        }
        else
        {
          EVEL_ERROR("Unexpected JSON state %d at token %d (%d)",
                     entry->json_state, token_index, token->type);
          json_ok = false;
        }
        break;

      case JSMN_ARRAY:
        if ((entry->json_state == EVEL_JSON_ITEM) ||
            (entry->json_state == EVEL_JSON_VALUE))
        {
          json_ok = evel_stack_push(json_stack, token->size, EVEL_JSON_ITEM);
        }
        else
        {
          EVEL_ERROR("Unexpected JSON state %d at token %d (%d)",
                     entry->json_state, token_index, token->type);
          json_ok = false;
        }
        break;

      case JSMN_STRING:
        if (entry->json_state == EVEL_JSON_KEY)
        {
          evel_stack_store_key(json_stack, token);
        }
        else if (entry->json_state == EVEL_JSON_VALUE)
        {
          evel_stack_store_value(json_stack, token);
        }
        else if (entry->json_state == EVEL_JSON_ITEM)
        {
          evel_stack_store_item(json_stack, token);
        }
        else
        {
          EVEL_ERROR("Unexpected JSON state %d at token %d (%d)",
                     entry->json_state, token_index, token->type);
          json_ok = false;
        }
        break;

      case JSMN_PRIMITIVE:
        if (entry->json_state == EVEL_JSON_VALUE)
        {
          evel_stack_store_value(json_stack, token);
        }
        else if (entry->json_state == EVEL_JSON_ITEM)
        {
          evel_stack_store_item(json_stack, token);
        }
        else
        {
          EVEL_ERROR("Unexpected JSON state %d at token %d (%d)",
                     entry->json_state, token_index, token->type);
          json_ok = false;
        }
        break;

      case JSMN_UNDEFINED:
      default:
        EVEL_ERROR("Unexpected JSON format at token %d (%d)",
                   token_index, token->type);
        json_ok = false;
        break;
    }

    /*************************************************************************/
    /* Pop the stack if we're counted enough nested items.                   */
    /*************************************************************************/
    evel_stack_pop(json_stack);

    token_index++;
  }

  /***************************************************************************/
  /* Cleanup the stack - we may have exited without winding it back, if the  */
  /* input was not well formed.                                              */
  /***************************************************************************/
  evel_stack_cleanup(json_stack);

  /***************************************************************************/
  /* We may want to generate and POST a response to the command list.        */
  /***************************************************************************/
  if (json_ok)
  {
    evel_command_list_response(post);
  }

  /***************************************************************************/
  /* Make sure we're clean on exit.                                          */
  /***************************************************************************/
  assert(evel_command_type_value == NULL);
  assert(evel_measurement_interval_value == NULL);
  assert(evel_throttle_spec_domain_value == NULL);
  assert(evel_throttle_spec_domain == EVEL_MAX_DOMAINS);
  assert(evel_temp_throttle == NULL);

  EVEL_EXIT();

  return json_ok;
}

/**************************************************************************//**
 * Copy a copy of an element, in string form.
 *
 * The caller must manage memory allocated for the copied string.
 *
 * @param chunk         Memory chunk containing the JSON buffer.
 * @param token         The token to copy from.
 * @return the copy of the element.
 *****************************************************************************/
char * evel_stack_strdup(const MEMORY_CHUNK * const chunk,
                         const jsmntok_t * const token)
{
  char temp_char;
  char * result;

  EVEL_ENTER();

  /***************************************************************************/
  /* Call strdup to copy the string, inserting a temporary \0 for the call.  */
  /***************************************************************************/
  temp_char = chunk->memory[token->end];
  chunk->memory[token->end] = '\0';
  result = strdup(chunk->memory + token->start);
  assert(result != NULL);
  chunk->memory[token->end] = temp_char;

  EVEL_EXIT();

  return result;
}

/**************************************************************************//**
 * Copy a copy of an element, in string form.
 *
 * @param json_stack    The JSON stack to initialize.
 * @param chunk         The underlying memory chunk used for parsing.
 *****************************************************************************/
void evel_init_json_stack(EVEL_JSON_STACK * json_stack,
                          const MEMORY_CHUNK * const chunk)
{
  EVEL_JSON_STACK_ENTRY * entry;

  EVEL_ENTER();

  json_stack->level = 0;
  entry = json_stack->entry;
  entry->json_state = EVEL_JSON_ITEM;
  entry->json_count = 0;
  entry->num_required = 1;
  entry->json_key = NULL;
  json_stack->chunk = chunk;

  EVEL_EXIT();
}

/**************************************************************************//**
 * Push a new entry on the stack
 *
 * @param json_stack    The stack.
 * @param num_required  The number of elements required.
 * @param new_state     The state for the new entry.
 * @return false if we cannot push onto the stack.
 *****************************************************************************/
bool evel_stack_push(EVEL_JSON_STACK * const json_stack,
                     const int num_required,
                     const EVEL_JSON_STATE new_state)
{
  EVEL_JSON_STACK_ENTRY * entry;
  char * key;
  bool result;

  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(json_stack != NULL);
  assert(json_stack->level >= 0);
  assert(json_stack->level < EVEL_JSON_STACK_DEPTH);
  assert((new_state == EVEL_JSON_ITEM) || (new_state == EVEL_JSON_KEY));

  /***************************************************************************/
  /* Check nesting depth, and stop processing if we hit the limit.           */
  /***************************************************************************/
  if ((json_stack->level + 1) >= EVEL_JSON_STACK_DEPTH)
  {
    EVEL_ERROR("JSON Nesting is too deep - stop processing");
    result = false;
    goto exit_label;
  }

  /***************************************************************************/
  /* Evaluate cases where we recurse and are interested in the contents.     */
  /***************************************************************************/
  entry = &json_stack->entry[json_stack->level];
  key = entry->json_key;

  /***************************************************************************/
  /* Note that this is the key before we drop a level.                       */
  /***************************************************************************/
  if (key != NULL)
  {
    EVEL_DEBUG("Push with key: %s", key);

    switch (evel_json_command_state)
    {
      case EVEL_JCS_START:
        if (strcmp(key, "commandList") == 0)
        {
          evel_set_command_state(EVEL_JCS_COMMAND_LIST);
        }
        break;

      case EVEL_JCS_COMMAND_LIST_ENTRY:
        if (strcmp(key, "command") == 0)
        {
          evel_open_command();
          evel_set_command_state(EVEL_JCS_COMMAND);
        }
        break;

      case EVEL_JCS_COMMAND:
        if (strcmp(key, "eventDomainThrottleSpecification") == 0)
        {
          evel_open_throttle_spec();
          evel_set_command_state(EVEL_JCS_SPEC);
        }
        break;

      case EVEL_JCS_SPEC:
        if (strcmp(key, "suppressedFieldNames") == 0)
        {
          evel_set_command_state(EVEL_JCS_FIELD_NAMES);
        }
        else if (strcmp(key, "suppressedNvPairsList") == 0)
        {
          evel_set_command_state(EVEL_JCS_PAIRS_LIST);
        }
        break;

      case EVEL_JCS_PAIRS_LIST_ENTRY:
        if (strcmp(key, "suppressedNvPairNames") == 0)
        {
          evel_set_command_state(EVEL_JCS_NV_PAIR_NAMES);
        }
        break;

      case EVEL_JCS_FIELD_NAMES:
      case EVEL_JCS_PAIRS_LIST:
      case EVEL_JCS_NV_PAIR_NAMES:
      default:
        EVEL_ERROR("Unexpected JSON key %s in state %d",
                   key,
                   evel_json_command_state);
        break;
    }
  }
  else
  {
    EVEL_DEBUG("Push with no key");

    /*************************************************************************/
    /* If we're pushing without a key, then we're in an array.  We switch    */
    /* state based on the existing state and stack level.                    */
    /*************************************************************************/
    const int COMMAND_LIST_LEVEL = 2;
    const int NV_PAIRS_LIST_LEVEL = 6;

    if ((evel_json_command_state == EVEL_JCS_PAIRS_LIST) &&
        (json_stack->level == NV_PAIRS_LIST_LEVEL))
    {
      /***********************************************************************/
      /* We are entering an object within the "suppressedNvPairsList" array. */
      /***********************************************************************/
      evel_open_nv_pairs_list_entry();
      evel_set_command_state(EVEL_JCS_PAIRS_LIST_ENTRY);
    }

    if ((evel_json_command_state == EVEL_JCS_COMMAND_LIST) &&
        (json_stack->level == COMMAND_LIST_LEVEL))
    {
      /***********************************************************************/
      /* We are entering an object within the "commandList" array.           */
      /***********************************************************************/
      evel_set_command_state(EVEL_JCS_COMMAND_LIST_ENTRY);
    }
  }

  /***************************************************************************/
  /* Push the stack and initialize the entry.                                */
  /***************************************************************************/
  json_stack->level++;
  entry++;
  EVEL_DEBUG("Stack Push -> %d", json_stack->level);
  entry = &json_stack->entry[json_stack->level];
  entry->json_count = 0;
  entry->num_required = num_required;
  entry->json_state = new_state;
  entry->json_key = NULL;
  result = true;

exit_label:

  EVEL_EXIT();

  return result;
}

/**************************************************************************//**
 * Pop any stack entries which have collected the required number of items.
 *
 * @param json_stack    The stack.
 *****************************************************************************/
void evel_stack_pop(EVEL_JSON_STACK * const json_stack)
{
  EVEL_JSON_STACK_ENTRY * entry;
  char * key;

  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(json_stack != NULL);
  assert(json_stack->level >= 0);
  assert(json_stack->level < EVEL_JSON_STACK_DEPTH);

  entry = &json_stack->entry[json_stack->level];
  while ((json_stack->level > 0) && (entry->json_count == entry->num_required))
  {
    key = entry->json_key;

    switch (evel_json_command_state)
    {
      case EVEL_JCS_COMMAND_LIST:
        evel_set_command_state(EVEL_JCS_START);
        break;

      case EVEL_JCS_COMMAND_LIST_ENTRY:
        evel_set_command_state(EVEL_JCS_COMMAND_LIST);
        break;

      case EVEL_JCS_COMMAND:
        evel_close_command();
        evel_set_command_state(EVEL_JCS_COMMAND_LIST_ENTRY);
        break;

      case EVEL_JCS_SPEC:
        evel_close_throttle_spec();
        evel_set_command_state(EVEL_JCS_COMMAND);
        break;

      case EVEL_JCS_FIELD_NAMES:
        evel_set_command_state(EVEL_JCS_SPEC);
        break;

      case EVEL_JCS_PAIRS_LIST:
        evel_set_command_state(EVEL_JCS_SPEC);
        break;

      case EVEL_JCS_PAIRS_LIST_ENTRY:
        evel_close_nv_pairs_list_entry();
        evel_set_command_state(EVEL_JCS_PAIRS_LIST);
        break;

      case EVEL_JCS_NV_PAIR_NAMES:
        evel_set_command_state(EVEL_JCS_PAIRS_LIST_ENTRY);
        break;

      default:
        break;
    }

    /*************************************************************************/
    /* Free off any key that was duplicated and stored.                      */
    /*************************************************************************/
    if (key != NULL)
    {
      free(key);
      entry->json_key = NULL;
    }

    /*************************************************************************/
    /* We just reached the required number of key-value pairs or items, so   */
    /* pop the stack.                                                        */
    /*************************************************************************/
    json_stack->level--;
    entry--;

    EVEL_DEBUG("Stack Pop  -> %d", json_stack->level);

    /*************************************************************************/
    /* We just completed collection of an ITEM (within an ARRAY) or a VALUE  */
    /* (within an OBJECT).  Either way, we need to count it.                 */
    /*************************************************************************/
    entry->json_count++;

    /*************************************************************************/
    /* If we just completed a VALUE, then we expect the next element to be a */
    /* key, if there is a next element.                                      */
    /*************************************************************************/
    if (entry->json_state == EVEL_JSON_VALUE)
    {
      entry->json_state = EVEL_JSON_KEY;
    }
  }

  EVEL_EXIT();
}

/**************************************************************************//**
 * Pop all stack entries, freeing any memory as we go.
 *
 * @param json_stack    The stack.
 *****************************************************************************/
void evel_stack_cleanup(EVEL_JSON_STACK * const json_stack)
{
  EVEL_JSON_STACK_ENTRY * entry;

  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(json_stack != NULL);
  assert(json_stack->level >= 0);
  assert(json_stack->level < EVEL_JSON_STACK_DEPTH);

  entry = &json_stack->entry[json_stack->level];
  while ((json_stack->level > 0))
  {
    /*************************************************************************/
    /* Free off any key that was duplicated and stored.                      */
    /*************************************************************************/
    if (entry->json_key != NULL)
    {
      free(entry->json_key);
      entry->json_key = NULL;
    }

    /*************************************************************************/
    /* We just reached the required number of key-value pairs or items, so   */
    /* pop the stack.                                                        */
    /*************************************************************************/
    json_stack->level--;
    entry--;
  }

  /***************************************************************************/
  /* If we hit EVEL_JSON_STACK_DEPTH, we exit the loop and can leave these   */
  /* values hanging - so clean them up.                                      */
  /***************************************************************************/
  if (evel_command_type_value != NULL)
  {
    free(evel_command_type_value);
    evel_command_type_value = NULL;
  }
  if (evel_measurement_interval_value != NULL)
  {
    free(evel_measurement_interval_value);
    evel_measurement_interval_value = NULL;
  }
  if (evel_throttle_spec_domain_value != NULL)
  {
    free(evel_throttle_spec_domain_value);
    evel_throttle_spec_domain_value = NULL;
  }
  evel_throttle_spec_domain = EVEL_MAX_DOMAINS;
  if (evel_temp_throttle != NULL)
  {
    evel_throttle_free(evel_temp_throttle);
    evel_temp_throttle = NULL;
  }

  EVEL_EXIT();
}

/**************************************************************************//**
 * Store a key in the JSON stack.
 *
 * We always store the most recent key at each level in the stack.
 *
 * @param json_stack    The stack.
 * @param token         The token holding the key.
 *****************************************************************************/
void evel_stack_store_key(EVEL_JSON_STACK * const json_stack,
                          const jsmntok_t * const token)
{
  EVEL_JSON_STACK_ENTRY * entry;

  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(json_stack != NULL);
  assert(json_stack->level >= 0);
  assert(json_stack->level < EVEL_JSON_STACK_DEPTH);

  /***************************************************************************/
  /* Free any previously stored key, replacing it with the new one.          */
  /***************************************************************************/
  entry = &json_stack->entry[json_stack->level];
  if (entry->json_key != NULL)
  {
    free(entry->json_key);
  }
  entry->json_key = evel_stack_strdup(json_stack->chunk, token);

  /***************************************************************************/
  /* Switch state to collecting the corresponding value.                     */
  /***************************************************************************/
  entry->json_state = EVEL_JSON_VALUE;

  EVEL_DEBUG("Stored key: %s", entry->json_key);
  EVEL_EXIT();
}

/**************************************************************************//**
 * Store a value in the JSON stack.
 *
 * @param json_stack    The stack.
 * @param token         The token holding the value.
 *****************************************************************************/
void evel_stack_store_value(EVEL_JSON_STACK * const json_stack,
                            const jsmntok_t * const token)
{
  EVEL_JSON_STACK_ENTRY * entry;
  char * value;
  bool stored;

  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(json_stack != NULL);
  assert(json_stack->level >= 0);
  assert(json_stack->level < EVEL_JSON_STACK_DEPTH);

  /***************************************************************************/
  /* Based on the (key, state), work out whether we're expecting a value,    */
  /* then store or ignore it as required.                                    */
  /***************************************************************************/
  entry = &json_stack->entry[json_stack->level];
  value = evel_stack_strdup(json_stack->chunk, token);
  stored = false;
  EVEL_DEBUG("Store value: %s", value);

  switch (evel_json_command_state)
  {
    case EVEL_JCS_COMMAND:
      if (strcmp(entry->json_key, "commandType") == 0)
      {
        evel_command_type_value = value;
        stored = true;
      }
      else if (strcmp(entry->json_key, "measurementInterval") == 0)
      {
        evel_measurement_interval_value = value;
        stored = true;
      }
      break;

    case EVEL_JCS_SPEC:
      if (strcmp(entry->json_key, "eventDomain") == 0)
      {
        evel_throttle_spec_domain_value = value;
        stored = true;
      }
      break;

    case EVEL_JCS_PAIRS_LIST_ENTRY:
      if (strcmp(entry->json_key, "nvPairFieldName") == 0)
      {
        evel_store_nv_pair_field_name(value);
        stored = true;
      }
      break;

    default:
      EVEL_DEBUG("Ignoring value in state: %s",
                 evel_jcs_strings[evel_json_command_state]);
      break;
  }

  if (!stored)
  {
    EVEL_DEBUG("Ignored value: %s", value);
    free(value);
  }

  /***************************************************************************/
  /* Switch state to another key.                                            */
  /***************************************************************************/
  entry->json_state = EVEL_JSON_KEY;

  /***************************************************************************/
  /* Count the key-value pair.                                               */
  /***************************************************************************/
  entry->json_count++;

  EVEL_EXIT();
}

/**************************************************************************//**
 * Store an item in the JSON stack - a string or primitive in an array.
 *
 * @param json_stack    The stack.
 * @param token         The token holding the item.
 *****************************************************************************/
void evel_stack_store_item(EVEL_JSON_STACK * const json_stack,
                           const jsmntok_t * const token)
{
  EVEL_JSON_STACK_ENTRY * entry;
  char * item;
  bool stored;

  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(json_stack != NULL);
  assert(json_stack->level >= 0);
  assert(json_stack->level < EVEL_JSON_STACK_DEPTH);

  /***************************************************************************/
  /* Based on the state, work out whether we're expecting an item, then      */
  /* store or ignore it as required.                                         */
  /***************************************************************************/
  entry = &json_stack->entry[json_stack->level];
  item = evel_stack_strdup(json_stack->chunk, token);
  stored = false;
  EVEL_DEBUG("Store item: %s", item);

  switch (evel_json_command_state)
  {
    case EVEL_JCS_NV_PAIR_NAMES:
      evel_store_nv_pair_name(item);
      stored = true;
      break;

    case EVEL_JCS_FIELD_NAMES:
      evel_store_suppressed_field_name(item);
      stored = true;
      break;

    default:
      EVEL_DEBUG("Ignoring item in state: %s",
                 evel_jcs_strings[evel_json_command_state]);
      break;
  }

  if (!stored)
  {
    EVEL_DEBUG("Ignored item: %s", item);
    free(item);
  }

  /***************************************************************************/
  /* We need another item.  This is purely defensive.                        */
  /***************************************************************************/
  entry->json_state = EVEL_JSON_ITEM;

  /***************************************************************************/
  /* Count the item.                                                         */
  /***************************************************************************/
  entry->json_count++;
}

/**************************************************************************//**
 * Set the JSON command state to a new value.
 *
 * @param new_state     The new state to set.
 *****************************************************************************/
void evel_set_command_state(const EVEL_JSON_COMMAND_STATE new_state)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(evel_json_command_state < EVEL_JCS_MAX);
  assert(new_state < EVEL_JCS_MAX);

  /***************************************************************************/
  /* Provide common debug, and set the new state.                            */
  /***************************************************************************/
  EVEL_DEBUG("Command State: %s -> %s",
             evel_jcs_strings[evel_json_command_state],
             evel_jcs_strings[new_state]);
  evel_json_command_state = new_state;

  EVEL_EXIT();
}

/**************************************************************************//**
 * Produce debug output from a JSON token.
 *
 * @param chunk         Memory chunk containing the JSON buffer.
 * @param token         Token to dump.
 *****************************************************************************/
void evel_debug_token(const MEMORY_CHUNK * const chunk,
                      const jsmntok_t * const token)
{
  char temp_char;

  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(token->type > 0);
  assert(token->type < JSON_TOKEN_TYPES);

  /***************************************************************************/
  /* Log the token, leaving it in the state in which it started.             */
  /***************************************************************************/
  temp_char = chunk->memory[token->end];
  chunk->memory[token->end] = '\0';
  EVEL_DEBUG("JSON token type: %s", evel_json_token_strings[token->type]);
  EVEL_DEBUG("JSON token: %s", chunk->memory + token->start);
  chunk->memory[token->end] = temp_char;

  EVEL_EXIT();
}

/**************************************************************************//**
 * Post a response to the commandList.
 *
 * @param post          Memory chunk in which to post a response.
 *****************************************************************************/
void evel_command_list_response(MEMORY_CHUNK * const post)
{
  char * json_post;

  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(post != NULL);
  assert(post->memory == NULL);

  if (evel_provide_throttling_state)
  {
    EVEL_DEBUG("Provide throttling state");

    /*************************************************************************/
    /* Encode the response, making it printf-able for debug.                 */
    /*************************************************************************/
    json_post = malloc(EVEL_MAX_JSON_BODY);
    assert(json_post != NULL);
    post->size = evel_json_encode_throttle(json_post, EVEL_MAX_JSON_BODY - 1);
    post->memory = json_post;
    post->memory[post->size] = '\0';
    evel_provide_throttling_state = false;
  }

  EVEL_EXIT();
}

/**************************************************************************//**
 * Encode the full throttling specification according to AT&T's schema.
 *
 * @param json          Pointer to where to store the JSON encoded data.
 * @param max_size      Size of storage available in json_body.
 * @returns Number of bytes actually written.
 *****************************************************************************/
int evel_json_encode_throttle(char * const json, const int max_size)
{
  bool throttled;
  int domain;
  int offset;
  bool domain_added;

  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(json != NULL);
  assert(max_size > 0);

  /***************************************************************************/
  /* Work out if we're throttled.                                            */
  /***************************************************************************/
  throttled = false;
  for (domain = EVEL_DOMAIN_FAULT; domain < EVEL_MAX_DOMAINS; domain++)
  {
    if (evel_throttle_spec[domain] != NULL)
    {
      throttled = true;
    }
  }

  /***************************************************************************/
  /* Encode the response.                                                    */
  /***************************************************************************/
  offset = 0;
  offset += snprintf(json + offset, max_size - offset,
                     "{\"eventThrottlingState\": {");
  offset += snprintf(json + offset, max_size - offset,
                     "\"eventThrottlingMode\": \"%s\"",
                     throttled ? "throttled" : "normal");
  if (throttled)
  {
    offset += snprintf(json + offset, max_size - offset,
                       ", \"eventDomainThrottleSpecificationList\": [");

    domain_added = false;
    for (domain = EVEL_DOMAIN_FAULT; domain < EVEL_MAX_DOMAINS; domain++)
    {
      if (evel_throttle_spec[domain] != NULL)
      {
        if (domain_added)
        {
          offset += snprintf(json + offset, max_size - offset, ", ");
        }

        offset += evel_json_encode_throttle_spec(json + offset,
                                                 max_size - offset,
                                                 domain);
        domain_added = true;
      }
    }

    offset += snprintf(json + offset, max_size - offset, "]");
  }

  offset += snprintf(json + offset, max_size - offset, "}}");

  EVEL_EXIT();

  return offset;
}

/**************************************************************************//**
 * Encode a throttling specification for a domain.
 *
 * @param json          Pointer to where to store the JSON encoded data.
 * @param max_size      Size of storage available in json_body.
 * @returns Number of bytes actually written.
 *****************************************************************************/
int evel_json_encode_throttle_spec(char * const json,
                                   const int max_size,
                                   const EVEL_EVENT_DOMAINS domain)
{
  int offset;
  EVEL_THROTTLE_SPEC * throttle_spec;
  DLIST_ITEM * dlist_item;
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(domain >= EVEL_DOMAIN_FAULT);
  assert(domain < EVEL_MAX_DOMAINS);
  assert(evel_throttle_spec[domain] != NULL);

  throttle_spec = evel_throttle_spec[domain];

  /***************************************************************************/
  /* Encode the domain.                                                      */
  /***************************************************************************/
  offset = 0;
  offset += snprintf(json + offset, max_size - offset,
                     "{");
  offset += snprintf(json + offset, max_size - offset,
                     "\"eventDomain\": \"%s\"",
                     evel_domain_strings[domain]);

  /***************************************************************************/
  /* Encode "suppressedFieldNames".                                          */
  /***************************************************************************/
  dlist_item = dlist_get_first(&throttle_spec->suppressed_field_names);
  if (dlist_item != NULL)
  {
    offset += snprintf(json + offset, max_size - offset,
                       ", \"suppressedFieldNames\": [");
    while (dlist_item != NULL)
    {
      char * suppressed_field = dlist_item->item;
      assert(suppressed_field != NULL);

      offset += snprintf(json + offset, max_size - offset,
                         "\"%s\"", suppressed_field);
      dlist_item = dlist_get_next(dlist_item);
      if (dlist_item != NULL)
      {
        offset += snprintf(json + offset, max_size - offset, ", ");
      }
    }

    offset += snprintf(json + offset, max_size - offset, "]");
  }

  /***************************************************************************/
  /* Encode "suppressedNvPairsList".                                         */
  /***************************************************************************/
  dlist_item = dlist_get_first(&throttle_spec->suppressed_nv_pairs_list);
  if (dlist_item != NULL)
  {
    offset += snprintf(json + offset, max_size - offset,
                       ", \"suppressedNvPairsList\": [");
    while (dlist_item != NULL)
    {
      offset += evel_json_encode_nv_pairs(json + offset,
                                          max_size - offset,
                                          dlist_item->item);
      dlist_item = dlist_get_next(dlist_item);
      if (dlist_item != NULL)
      {
        offset += snprintf(json + offset, max_size - offset, ", ");
      }
    }

    offset += snprintf(json + offset, max_size - offset, "]");
  }

  offset += snprintf(json + offset, max_size - offset, "}");

  EVEL_EXIT();

  return offset;
}

/**************************************************************************//**
 * Encode a single "suppressedNvPairsListEntry".
 *
 * @param json          Pointer to where to store the JSON encoded data.
 * @param max_size      Size of storage available in json_body.
 * @returns Number of bytes actually written.
 *****************************************************************************/
int evel_json_encode_nv_pairs(char * const json,
                              const int max_size,
                              EVEL_SUPPRESSED_NV_PAIRS * nv_pairs)
{
  DLIST_ITEM * dlist_item;
  char * name;
  int offset;

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(nv_pairs != NULL);
  assert(nv_pairs->nv_pair_field_name != NULL);
  assert(!dlist_is_empty(&nv_pairs->suppressed_nv_pair_names));

  /***************************************************************************/
  /* Encode it.                                                              */
  /***************************************************************************/
  offset = 0;
  offset += snprintf(json + offset, max_size - offset, "{");
  offset += snprintf(json + offset, max_size - offset,
                     "\"nvPairFieldName\": \"%s\"",
                     nv_pairs->nv_pair_field_name);
  dlist_item = dlist_get_first(&nv_pairs->suppressed_nv_pair_names);
  offset += snprintf(json + offset, max_size - offset,
                     ", \"suppressedNvPairNames\": [");
  while (dlist_item != NULL)
  {
    name = dlist_item->item;
    assert(name != NULL);
    offset += snprintf(json + offset, max_size - offset, "\"%s\"", name);
    dlist_item = dlist_get_next(dlist_item);
    if (dlist_item != NULL)
    {
      offset += snprintf(json + offset, max_size - offset, ", ");
    }
  }
  offset += snprintf(json + offset, max_size - offset, "]");
  offset += snprintf(json + offset, max_size - offset, "}");

  EVEL_EXIT();

  return offset;
}

/**************************************************************************//**
 * Method called when we open a "command" object.
 *****************************************************************************/
void evel_open_command()
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Make some assertions.                                                   */
  /***************************************************************************/
  assert(evel_command_type_value == NULL);
  assert(evel_measurement_interval_value == NULL);

  EVEL_EXIT();
}

/**************************************************************************//**
 * Method called when we close a "command" object.
 *****************************************************************************/
void evel_close_command()
{
  EVEL_ENTER();

  /***************************************************************************/
  /* If a commandType was provided, fan out and handle it now what we have   */
  /* fathered all related information.                                       */
  /*                                                                         */
  /* Note that we handle throttling specification and measurement interval   */
  /* updates immediately on closing the command (not the list). We could     */
  /* reject *all* commands in a list if any of them are invalid, but we are  */
  /* take a best-effort strategy here - any valid-looking command gets       */
  /* implemented regardless of what follows.                                 */
  /***************************************************************************/
  if (evel_command_type_value != NULL)
  {
    EVEL_DEBUG("Closing command %s", evel_command_type_value);

    if (strcmp(evel_command_type_value, "provideThrottlingState") == 0)
    {
      evel_provide_throttling_state = true;
    }
    else if (strcmp(evel_command_type_value, "throttlingSpecification") == 0)
    {
      evel_set_throttling_spec();
    }
    else if (strcmp(evel_command_type_value, "measurementIntervalChange") == 0)
    {
      evel_set_measurement_interval();
    }
    else
    {
      EVEL_ERROR("Ignoring unknown commandType: %s\n",
                 evel_command_type_value);
    }

    /*************************************************************************/
    /* Free the captured "commandType" value.                                */
    /*************************************************************************/
    free(evel_command_type_value);
    evel_command_type_value = NULL;
  }

  /***************************************************************************/
  /* There could be an unused working throttle spec at this point - if the   */
  /* "throttlingSpecification" commandType was not provided, or an invalid   */
  /* domain was provided, or was not provided at all.                        */
  /***************************************************************************/
  if (evel_temp_throttle != NULL)
  {
    evel_throttle_free(evel_temp_throttle);
    evel_temp_throttle = NULL;
  }

  /***************************************************************************/
  /* Similarly, the domain could be set.                                     */
  /***************************************************************************/
  evel_throttle_spec_domain = EVEL_MAX_DOMAINS;

  /***************************************************************************/
  /* There could be an unused measurement interval value at this point - if  */
  /* the "measurementIntervalChange" command was not provided.               */
  /***************************************************************************/
  if (evel_measurement_interval_value != NULL)
  {
    free(evel_measurement_interval_value);
    evel_measurement_interval_value = NULL;
  }

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the provided throttling specification, when the command closes.
 *****************************************************************************/
void evel_set_throttling_spec()
{
  EVEL_ENTER();

  if ((evel_throttle_spec_domain >= 0) &&
      (evel_throttle_spec_domain < EVEL_MAX_DOMAINS))
  {
    EVEL_DEBUG("Updating throttle spec for domain: %s",
               evel_domain_strings[evel_throttle_spec_domain]);

    /*************************************************************************/
    /* Free off the previous throttle specification for the domain, if there */
    /* is one.                                                               */
    /*************************************************************************/
    if (evel_throttle_spec[evel_throttle_spec_domain] != NULL)
    {
      evel_throttle_free(evel_throttle_spec[evel_throttle_spec_domain]);
    }

    /*************************************************************************/
    /* Finalize the working throttling spec, if there is one.                */
    /*************************************************************************/
    if (evel_temp_throttle != NULL)
    {
      evel_throttle_finalize(evel_temp_throttle);
    }

    /*************************************************************************/
    /* Replace the throttle specification for the domain with the working    */
    /* throttle specification.  This could be NULL, if an empty throttle     */
    /* specification has been received for a domain.                         */
    /*************************************************************************/
    evel_throttle_spec[evel_throttle_spec_domain] = evel_temp_throttle;
    evel_temp_throttle = NULL;
  }

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the provided measurement interval, when the command closes.
 *****************************************************************************/
void evel_set_measurement_interval()
{
  EVEL_ENTER();

  if (evel_measurement_interval_value != NULL)
  {
    const long int value = strtol(evel_measurement_interval_value, NULL, 10);

    if ((value >= 0) && (value <= INT_MAX))
    {
      /***********************************************************************/
      /* Lock, update, unlock.                                               */
      /***********************************************************************/
      EVEL_DEBUG("Updating measurement interval to %d\n", value);

      pthread_mutex_lock(&evel_measurement_interval_mutex);
      evel_measurement_interval = value;
      pthread_mutex_unlock(&evel_measurement_interval_mutex);
    }
    else
    {
      EVEL_ERROR("Ignoring invalid measurement interval: %s",
                 evel_measurement_interval_value);
    }
  }

  EVEL_EXIT();
}

/**************************************************************************//**
 * Method called when we open an "eventDomainThrottleSpecification" object.
 *****************************************************************************/
void evel_open_throttle_spec()
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(evel_throttle_spec_domain_value == NULL);
  assert(evel_throttle_spec_domain == EVEL_MAX_DOMAINS);
  assert(evel_temp_throttle == NULL);

  /***************************************************************************/
  /* Allocate and initialize an ::EVEL_THROTTLE_SPEC in which to hold        */
  /* captured JSON elements.                                                 */
  /***************************************************************************/
  evel_temp_throttle = malloc(sizeof(EVEL_THROTTLE_SPEC));
  assert(evel_temp_throttle != NULL);
  dlist_initialize(&evel_temp_throttle->suppressed_field_names);
  dlist_initialize(&evel_temp_throttle->suppressed_nv_pairs_list);
  evel_temp_throttle->hash_field_names = NULL;
  evel_temp_throttle->hash_nv_pairs_list = NULL;

  EVEL_EXIT();
}

/**************************************************************************//**
 * Method called when we close an "eventDomainThrottleSpecification" object.
 *****************************************************************************/
void evel_close_throttle_spec()
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Decode, free and blank a captured event domain value.                   */
  /***************************************************************************/
  if (evel_throttle_spec_domain_value != NULL)
  {
    evel_throttle_spec_domain =
                           evel_decode_domain(evel_throttle_spec_domain_value);
    free(evel_throttle_spec_domain_value);
    evel_throttle_spec_domain_value = NULL;
  }

  /***************************************************************************/
  /* Free off an empty working throttle spec, to stop it being used.  This   */
  /* state should be represented by a NULL pointer for the domain.           */
  /***************************************************************************/
  if (evel_temp_throttle != NULL)
  {
    if (dlist_is_empty(&evel_temp_throttle->suppressed_field_names) &&
        dlist_is_empty(&evel_temp_throttle->suppressed_nv_pairs_list))
    {
      free(evel_temp_throttle);
      evel_temp_throttle = NULL;
    }
  }

  EVEL_EXIT();
}

/**************************************************************************//**
 * Convert a value for an "eventDomain" into an ::EVEL_EVENT_DOMAINS.
 *
 * @param domain_value  The domain string value to decode.
 * @returns The matching ::EVEL_EVENT_DOMAINS, or ::EVEL_MAX_DOMAINS on error.
 *****************************************************************************/
EVEL_EVENT_DOMAINS evel_decode_domain(char * domain_value)
{
  EVEL_EVENT_DOMAINS result;
  int ii;

  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(domain_value != NULL);

  result = EVEL_MAX_DOMAINS;
  for (ii = EVEL_DOMAIN_FAULT; ii < EVEL_MAX_DOMAINS; ii++)
  {
    assert(evel_domain_strings[ii] != NULL);
    if (strcmp(evel_domain_strings[ii], domain_value) == 0)
    {
      result = ii;
    }
  }

  EVEL_EXIT();

  return result;
}

/**************************************************************************//**
 * Method called when we open a "suppressedNvPairsListEntry" object.
 *****************************************************************************/
void evel_open_nv_pairs_list_entry()
{
  EVEL_SUPPRESSED_NV_PAIRS * nv_pairs;

  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(evel_temp_throttle != NULL);

  /***************************************************************************/
  /* Allocate and initialize an ::EVEL_SUPPRESSED_NV_PAIRS, and add it to    */
  /* the list.                                                               */
  /***************************************************************************/
  nv_pairs = malloc(sizeof(EVEL_SUPPRESSED_NV_PAIRS));
  assert(nv_pairs != NULL);
  nv_pairs->nv_pair_field_name = NULL;
  dlist_initialize(&nv_pairs->suppressed_nv_pair_names);
  nv_pairs->hash_nv_pair_names = NULL;
  dlist_push_last(&evel_temp_throttle->suppressed_nv_pairs_list, nv_pairs);

  EVEL_EXIT();
}

/**************************************************************************//**
 * Method called when we close a "suppressedNvPairsListEntry" object.
 *****************************************************************************/
void evel_close_nv_pairs_list_entry()
{
  EVEL_SUPPRESSED_NV_PAIRS * nv_pairs;
  EVEL_SUPPRESSED_NV_PAIRS * popped;

  EVEL_ENTER();

  /***************************************************************************/
  /* Get the latest nv pairs.  This also performs the required checks.       */
  /***************************************************************************/
  nv_pairs = evel_get_last_nv_pairs();

  /***************************************************************************/
  /* For a "suppressedNvPairsListEntry" to have any meaning, we need both    */
  /* "nvPairFieldName" and "suppressedNvPairNames".  If we don't, then pop   */
  /* and free whatever we just collected.                                    */
  /***************************************************************************/
  if ((nv_pairs->nv_pair_field_name == NULL) ||
      dlist_is_empty(&nv_pairs->suppressed_nv_pair_names))
  {
    popped = dlist_pop_last(&evel_temp_throttle->suppressed_nv_pairs_list);
    assert(popped == nv_pairs);
    evel_throttle_free_nv_pair(popped);
  }

  EVEL_EXIT();
}

/**************************************************************************//**
 * Store an "nvPairFieldName" value in the working throttle spec.
 *
 * @param value         The value to store.
 *****************************************************************************/
void evel_store_nv_pair_field_name(char * const value)
{
  EVEL_SUPPRESSED_NV_PAIRS * nv_pairs;

  EVEL_ENTER();

  /***************************************************************************/
  /* Get the latest nv pairs.  This also performs the required checks.       */
  /***************************************************************************/
  nv_pairs = evel_get_last_nv_pairs();

  /***************************************************************************/
  /* Store the value.                                                        */
  /***************************************************************************/
  nv_pairs->nv_pair_field_name = value;

  EVEL_EXIT();
}

/**************************************************************************//**
 * Store a "suppressedNvPairNames" item in the working throttle spec.
 *
 * @param item          The item to store.
 *****************************************************************************/
void evel_store_nv_pair_name(char * const item)
{
  EVEL_SUPPRESSED_NV_PAIRS * nv_pairs;

  EVEL_ENTER();

  /***************************************************************************/
  /* Get the latest nv pairs.  This also performs the required checks.       */
  /***************************************************************************/
  nv_pairs = evel_get_last_nv_pairs();

  /***************************************************************************/
  /* Store the item.                                                         */
  /***************************************************************************/
  dlist_push_last(&nv_pairs->suppressed_nv_pair_names, item);

  EVEL_EXIT();
}

/**************************************************************************//**
 * Store a "suppressedFieldNames" item in the working throttle spec.
 *
 * @param item          The item to store.
 *****************************************************************************/
void evel_store_suppressed_field_name(char * const item)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(evel_temp_throttle != NULL);

  /***************************************************************************/
  /* Store the item.                                                         */
  /***************************************************************************/
  dlist_push_last(&evel_temp_throttle->suppressed_field_names, item);

  EVEL_EXIT();
}

/**************************************************************************//**
 * Get the last added suppressed nv pairs list entry in the working spec.
 *
 * @returns The last entry.
 *****************************************************************************/
EVEL_SUPPRESSED_NV_PAIRS * evel_get_last_nv_pairs()
{
  DLIST_ITEM * dlist_item;
  EVEL_SUPPRESSED_NV_PAIRS * nv_pairs;

  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(evel_temp_throttle != NULL);

  /***************************************************************************/
  /* Get the pair that was added when we opened the list entry.              */
  /***************************************************************************/
  dlist_item = dlist_get_last(&evel_temp_throttle->suppressed_nv_pairs_list);
  assert(dlist_item != NULL);
  nv_pairs = dlist_item->item;
  assert(nv_pairs != NULL);

  EVEL_EXIT();

  return nv_pairs;
}
