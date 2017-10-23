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
 * Implementation of EVEL functions relating to Heartbeat fields.
 *
 ****************************************************************************/

#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include "evel.h"
#include "evel_throttle.h"

/**************************************************************************//**
 * Create a new Heartbeat fields event.
 *
 * @note    The mandatory fields on the Heartbeat fields must be supplied to
 *          this factory function and are immutable once set.  Optional fields
 *          have explicit setter functions, but again values may only be set
 *          once so that the event has immutable properties.
 * @param event_name  Unique Event Name confirming Domain AsdcModel Description
 * @param event_id    A universal identifier of the event for: troubleshooting correlation, analysis, etc
 * @param vendor_id     The vendor id to encode in the event instance id.
 * @param event_id      The vendor event id to encode in the event instance id.
 * @returns pointer to the newly manufactured ::EVENT_HEARTBEAT_FIELD.  If the event
 *          is not used (i.e. posted) it must be released using
 *          ::evel_free_hrtbt_field.
 * @retval  NULL  Failed to create the event.
 *****************************************************************************/
EVENT_HEARTBEAT_FIELD * evel_new_heartbeat_field(int interval,const char* ev_name, const char *ev_id)
{
  EVENT_HEARTBEAT_FIELD * event = NULL;

  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(interval > 0);

  /***************************************************************************/
  /* Allocate the Heartbeat fields event.                                           */
  /***************************************************************************/
  event = malloc(sizeof(EVENT_HEARTBEAT_FIELD));
  if (event == NULL)
  {
    log_error_state("Out of memory");
    goto exit_label;
  }
  memset(event, 0, sizeof(EVENT_HEARTBEAT_FIELD));
  EVEL_DEBUG("New Heartbeat fields event is at %lp", event);

  /***************************************************************************/
  /* Initialize the header & the Heartbeat fields fields.                           */
  /***************************************************************************/
  evel_init_header_nameid(&event->header,ev_name,ev_id);
  event->header.event_domain = EVEL_DOMAIN_HEARTBEAT_FIELD;
  event->major_version = EVEL_HEARTBEAT_FIELD_MAJOR_VERSION;
  event->minor_version = EVEL_HEARTBEAT_FIELD_MINOR_VERSION;

  event->heartbeat_interval = interval;
  dlist_initialize(&event->additional_info);

exit_label:

  EVEL_EXIT();
  return event;
}

/**************************************************************************//**
 * Add a name/value pair to the Heartbeat fields, under the additionalFields array.
 *
 * The name and value are null delimited ASCII strings.  The library takes
 * a copy so the caller does not have to preserve values after the function
 * returns.
 *
 * @param event     Pointer to the Heartbeat fields event.
 * @param name      ASCIIZ string with the field's name.  The caller does not
 *                  need to preserve the value once the function returns.
 * @param value     ASCIIZ string with the field's value.  The caller does not
 *                  need to preserve the value once the function returns.
 *****************************************************************************/
void evel_hrtbt_field_addl_field_add(EVENT_HEARTBEAT_FIELD * const event,
                                 const char * const name,
                                 const char * const value)
{
  OTHER_FIELD * nv_pair = NULL;

  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(event != NULL);
  assert(event->header.event_domain == EVEL_DOMAIN_HEARTBEAT_FIELD);
  assert(name != NULL);
  assert(value != NULL);

  EVEL_DEBUG("Adding name=%s value=%s", name, value);
  nv_pair = malloc(sizeof(OTHER_FIELD));
  assert(nv_pair != NULL);
  nv_pair->name = strdup(name);
  nv_pair->value = strdup(value);
  assert(nv_pair->name != NULL);
  assert(nv_pair->value != NULL);

  dlist_push_last(&event->additional_info, nv_pair);

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Interval property of the Heartbeat fields event.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param event         Pointer to the Heartbeat fields event.
 * @param product_id    The vendor product id to be set. ASCIIZ string. The
 *                      caller does not need to preserve the value once the
 *                      function returns.
 *****************************************************************************/
void evel_hrtbt_interval_set(EVENT_HEARTBEAT_FIELD * const event,
                                 const int interval)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions and call evel_set_option_string.                    */
  /***************************************************************************/
  assert(event != NULL);
  assert(event->header.event_domain == EVEL_DOMAIN_HEARTBEAT_FIELD);
  assert(interval > 0);

  event->heartbeat_interval = interval;
  EVEL_EXIT();
}


/**************************************************************************//**
 * Encode the Heartbeat fields in JSON according to AT&T's schema for the
 * event type.
 *
 * @param jbuf          Pointer to the ::EVEL_JSON_BUFFER to encode into.
 * @param event         Pointer to the ::EVENT_HEADER to encode.
 *****************************************************************************/
void evel_json_encode_hrtbt_field(EVEL_JSON_BUFFER * const jbuf,
                                EVENT_HEARTBEAT_FIELD * const event)
{
  OTHER_FIELD * nv_pair = NULL;
  DLIST_ITEM * dlist_item = NULL;

  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(event != NULL);
  assert(event->header.event_domain == EVEL_DOMAIN_HEARTBEAT_FIELD);

  evel_json_encode_header(jbuf, &event->header);
  evel_json_open_named_object(jbuf, "heartbeatField");

  /***************************************************************************/
  /* Mandatory fields                                                        */
  /***************************************************************************/
  evel_enc_version(jbuf, "heartbeatFieldsVersion", event->major_version,event->minor_version);
  evel_enc_kv_int(jbuf, "heartbeatInterval", event->heartbeat_interval);

  /***************************************************************************/
  /* Optional fields                                                         */
  /***************************************************************************/

  /***************************************************************************/
  /* Checkpoint, so that we can wind back if all fields are suppressed.      */
  /***************************************************************************/
  evel_json_checkpoint(jbuf);
  if (evel_json_open_opt_named_list(jbuf, "additionalFields"))
  {
    bool added = false;

    dlist_item = dlist_get_first(&event->additional_info);
    while (dlist_item != NULL)
    {
      nv_pair = (OTHER_FIELD *) dlist_item->item;
      assert(nv_pair != NULL);

      if (!evel_throttle_suppress_nv_pair(jbuf->throttle_spec,
                                          "additionalFields",
                                          nv_pair->name))
      {
        evel_json_open_object(jbuf);
        evel_enc_kv_string(jbuf, "name", nv_pair->name);
        evel_enc_kv_string(jbuf, "value", nv_pair->value);
        evel_json_close_object(jbuf);
        added = true;
      }
      dlist_item = dlist_get_next(dlist_item);
    }
    evel_json_close_list(jbuf);

    /*************************************************************************/
    /* If we've not written anything, rewind to before we opened the list.   */
    /*************************************************************************/
    if (!added)
    {
      evel_json_rewind(jbuf);
    }
  }

  evel_json_close_object(jbuf);

  EVEL_EXIT();
}

/**************************************************************************//**
 * Free a Heartbeat fields event.
 *
 * Free off the event supplied.  Will free all the contained allocated memory.
 *
 * @note It does not free the event itself, since that may be part of a larger
 * structure.
 *****************************************************************************/
void evel_free_hrtbt_field(EVENT_HEARTBEAT_FIELD * const event)
{
  OTHER_FIELD * nv_pair = NULL;

  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(event != NULL);
  assert(event->header.event_domain == EVEL_DOMAIN_HEARTBEAT_FIELD);

  /***************************************************************************/
  /* Free all internal strings then the header itself.                       */
  /***************************************************************************/
  nv_pair = dlist_pop_last(&event->additional_info);
  while (nv_pair != NULL)
  {
    EVEL_DEBUG("Freeing Other Field (%s, %s)", nv_pair->name, nv_pair->value);
    free(nv_pair->name);
    free(nv_pair->value);
    free(nv_pair);
    nv_pair = dlist_pop_last(&event->additional_info);
  }
  evel_free_header(&event->header);

  EVEL_EXIT();
}
