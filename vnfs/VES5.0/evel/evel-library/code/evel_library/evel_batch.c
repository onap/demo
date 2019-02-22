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
 * Source module implementing EVEL Batch API.
 *
 * This file implements the EVEL Batch API which is intended to provide a
 * simple wrapper around packaging multiple EVEL messages into single HTTP(S) package
 * This is implemented per VES 5.3 standards. Currently max size of package is 160K
 *
 ****************************************************************************/

#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include "evel.h"
#include "evel_internal.h"

/**************************************************************************//**
 * Create a new empty Batch event.
 *
 * @note    The mandatory fields on the Batch must be supplied to this factory
 *          function and are immutable once set.  Optional fields have explicit
 *          setter functions, but again values may only be set once so that the
 *          Batch has immutable properties.
 * @params  Event name and Event id are dummy strings. Not encoded into JSON
 * @returns pointer to the newly manufactured ::EVENT_HEADER.  If the event is
 *          not used (i.e. posted) it must be released using ::evel_free_batch.
 * @retval  NULL  Failed to create the event.
 *****************************************************************************/
EVENT_HEADER * evel_new_batch(const char* ev_name, const char *ev_id)
{
  EVENT_HEADER * other = NULL;
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/

  /***************************************************************************/
  /* Allocate the Batch.                                                     */
  /***************************************************************************/
  other = malloc(sizeof(EVENT_HEADER));
  if (other == NULL)
  {
    log_error_state("Out of memory");
    goto exit_label;
  }
  memset(other, 0, sizeof(EVENT_HEADER));
  EVEL_DEBUG("New Batch is at %lp", other);

  /***************************************************************************/
  /* Initialize the header & the Batch fields.  Optional string values are   */
  /* uninitialized (NULL).                                                   */
  /***************************************************************************/
  evel_init_header_nameid(other,ev_name,ev_id);
  other->event_domain = EVEL_DOMAIN_BATCH;
  other->major_version = EVEL_BATCH_MAJOR_VERSION;
  other->minor_version = EVEL_BATCH_MINOR_VERSION;

  dlist_initialize(&other->batch_events);

exit_label:
  EVEL_EXIT();
  return other;
}


/**************************************************************************//**
 * Add an additional VES Message into Batch Event
 *
 * The function may be called as many times without reaching 160K max json size
 * limit.
 * The max limit is only checked at encoding time and error generated
 *
 * @param batchev     Pointer to  already created new Batch Event.
 * @param child       Pointer to  additional VES Event
 *****************************************************************************/
void evel_batch_add_event(EVENT_HEADER * batchev, EVENT_HEADER *child)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(batchev != NULL);
  assert(batchev->event_domain == EVEL_DOMAIN_BATCH);
  assert(child != NULL);

  EVEL_DEBUG("Adding Batch event");

  dlist_push_last(&batchev->batch_events, child);

  EVEL_EXIT();
}


/**************************************************************************//**
 * Free a Batch Event.
 *
 * Free off the Batch supplied.  Will free all the contained VES message memory.
 *
 * @note It does not free the Batch itself, since that may be part of a
 * larger structure.
 *****************************************************************************/
void evel_free_batch(EVENT_HEADER * event)
{
  EVENT_HEADER * batch_field = NULL;

  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.  As an internal API we don't allow freeing NULL    */
  /* events as we do on the public API.                                      */
  /***************************************************************************/
  assert(event != NULL);
  assert(event->event_domain == EVEL_DOMAIN_BATCH);

  /***************************************************************************/
  /* Free all internal strings then the header itself.                       */
  /***************************************************************************/
  batch_field = dlist_pop_last(&event->batch_events);
  while (batch_field != NULL)
  {
    EVEL_DEBUG("Freeing Batch Event (%s, %s)",
               batch_field->event_name,
               batch_field->event_id);
    evel_free_event(batch_field);
    batch_field = dlist_pop_last(&event->batch_events);
  }
  evel_free_header(event); 

  EVEL_EXIT();
}
