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
 ****************************************************************************/
/**************************************************************************//**
 * @file
 * Implementation of EVEL functions relating to the State Change.
 *
 ****************************************************************************/

#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include "evel_throttle.h"

/**************************************************************************//**
 * Create a new State Change event.
 *
 * @note    The mandatory fields on the State Change must be supplied to this
 *          factory function and are immutable once set.  Optional fields have
 *          explicit setter functions, but again values may only be set once
 *          so that the State Change has immutable properties.
 *
 * @param event_name  Unique Event Name confirming Domain AsdcModel Description
 * @param event_id    A universal identifier of the event for: troubleshooting correlation, analysis, etc
 * @param new_state     The new state of the reporting entity.
 * @param old_state     The old state of the reporting entity.
 * @param interface     The card or port name of the reporting entity.
 *
 * @returns pointer to the newly manufactured ::EVENT_STATE_CHANGE.  If the
 *          event is not used it must be released using
 *          ::evel_free_state_change
 * @retval  NULL  Failed to create the event.
 *****************************************************************************/
EVENT_STATE_CHANGE * evel_new_state_change(const char* ev_name,
                                           const char *ev_id,
					   const EVEL_ENTITY_STATE new_state,
                                           const EVEL_ENTITY_STATE old_state,
                                           const char * const interface)
{
  EVENT_STATE_CHANGE * state_change = NULL;
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(new_state < EVEL_MAX_ENTITY_STATES);
  assert(old_state < EVEL_MAX_ENTITY_STATES);
  assert(interface != NULL);

  /***************************************************************************/
  /* Allocate the State Change.                                              */
  /***************************************************************************/
  state_change = malloc(sizeof(EVENT_STATE_CHANGE));
  if (state_change == NULL)
  {
    log_error_state("Out of memory");
    goto exit_label;
  }
  memset(state_change, 0, sizeof(EVENT_STATE_CHANGE));
  EVEL_DEBUG("New State Change is at %lp", state_change);

  /***************************************************************************/
  /* Initialize the header & the State Change fields.  Optional string       */
  /* values are uninitialized (NULL).                                        */
  /***************************************************************************/
  evel_init_header_nameid(&state_change->header,ev_name,ev_id);
  state_change->header.event_domain = EVEL_DOMAIN_STATE_CHANGE;
  state_change->major_version = EVEL_STATE_CHANGE_MAJOR_VERSION;
  state_change->minor_version = EVEL_STATE_CHANGE_MINOR_VERSION;
  state_change->new_state = new_state;
  state_change->old_state = old_state;
  state_change->state_interface = strdup(interface);
  dlist_initialize(&state_change->additional_fields);

exit_label:
  EVEL_EXIT();
  return state_change;
}

/**************************************************************************//**
 * Free a State Change.
 *
 * Free off the State Change supplied.  Will free all the contained allocated
 * memory.
 *
 * @note It does not free the State Change itself, since that may be part of a
 * larger structure.
 *****************************************************************************/
void evel_free_state_change(EVENT_STATE_CHANGE * const state_change)
{
  STATE_CHANGE_ADDL_FIELD * addl_field = NULL;

  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.  As an internal API we don't allow freeing NULL    */
  /* events as we do on the public API.                                      */
  /***************************************************************************/
  assert(state_change != NULL);
  assert(state_change->header.event_domain == EVEL_DOMAIN_STATE_CHANGE);

  /***************************************************************************/
  /* Free all internal strings then the header itself.                       */
  /***************************************************************************/
  addl_field = dlist_pop_last(&state_change->additional_fields);
  while (addl_field != NULL)
  {
    EVEL_DEBUG("Freeing Additional Field (%s, %s)",
               addl_field->name,
               addl_field->value);
    free(addl_field->name);
    free(addl_field->value);
    free(addl_field);
    addl_field = dlist_pop_last(&state_change->additional_fields);
  }
  free(state_change->state_interface);
  evel_free_header(&state_change->header);

  EVEL_EXIT();
}

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
                                const char * const type)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions and call evel_header_type_set.                      */
  /***************************************************************************/
  assert(state_change != NULL);
  assert(state_change->header.event_domain == EVEL_DOMAIN_STATE_CHANGE);
  evel_header_type_set(&state_change->header, type);

  EVEL_EXIT();
}

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
                                      const char * const value)
{
  STATE_CHANGE_ADDL_FIELD * addl_field = NULL;
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(state_change != NULL);
  assert(state_change->header.event_domain == EVEL_DOMAIN_STATE_CHANGE);
  assert(name != NULL);
  assert(value != NULL);

  EVEL_DEBUG("Adding name=%s value=%s", name, value);
  addl_field = malloc(sizeof(STATE_CHANGE_ADDL_FIELD));
  assert(addl_field != NULL);
  memset(addl_field, 0, sizeof(STATE_CHANGE_ADDL_FIELD));
  addl_field->name = strdup(name);
  addl_field->value = strdup(value);
  assert(addl_field->name != NULL);
  assert(addl_field->value != NULL);

  dlist_push_last(&state_change->additional_fields, addl_field);

  EVEL_EXIT();
}

/**************************************************************************//**
 * Encode the state change as a JSON state change.
 *
 * @param jbuf          Pointer to the ::EVEL_JSON_BUFFER to encode into.
 * @param state_change  Pointer to the ::EVENT_STATE_CHANGE to encode.
 *****************************************************************************/
void evel_json_encode_state_change(EVEL_JSON_BUFFER * jbuf,
                                   EVENT_STATE_CHANGE * state_change)
{
  STATE_CHANGE_ADDL_FIELD * addl_field = NULL;
  DLIST_ITEM * addl_field_item = NULL;
  char * new_state;
  char * old_state;

  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(state_change != NULL);
  assert(state_change->header.event_domain == EVEL_DOMAIN_STATE_CHANGE);

  new_state = evel_entity_state(state_change->new_state);
  old_state = evel_entity_state(state_change->old_state);

  evel_json_encode_header(jbuf, &state_change->header);
  evel_json_open_named_object(jbuf, "stateChangeFields");

  /***************************************************************************/
  /* Mandatory fields.                                                       */
  /***************************************************************************/
  evel_enc_kv_string(jbuf, "newState", new_state);
  evel_enc_kv_string(jbuf, "oldState", old_state);
  evel_enc_kv_string(jbuf, "stateInterface", state_change->state_interface);

  /***************************************************************************/
  /* Optional fields.                                                        */
  /***************************************************************************/
  evel_json_checkpoint(jbuf);
  if (evel_json_open_opt_named_list(jbuf, "additionalFields"))
  {
    bool item_added = false;

    addl_field_item = dlist_get_first(&state_change->additional_fields);
    while (addl_field_item != NULL)
    {
      addl_field = (STATE_CHANGE_ADDL_FIELD *) addl_field_item->item;
      assert(addl_field != NULL);

      if (!evel_throttle_suppress_nv_pair(jbuf->throttle_spec,
                                          "additionalFields",
                                          addl_field->name))
      {
        evel_json_open_object(jbuf);
        evel_enc_kv_string(jbuf, "name", addl_field->name);
        evel_enc_kv_string(jbuf, "value", addl_field->value);
        evel_json_close_object(jbuf);
        item_added = true;
      }
      addl_field_item = dlist_get_next(addl_field_item);
    }
    evel_json_close_list(jbuf);

    /*************************************************************************/
    /* If we've not written anything, rewind to before we opened the list.   */
    /*************************************************************************/
    if (!item_added)
    {
      evel_json_rewind(jbuf);
    }
  }

  evel_enc_version(jbuf,
                   "stateChangeFieldsVersion",
                   state_change->major_version,state_change->minor_version);

  evel_json_close_object(jbuf);

  EVEL_EXIT();
}
