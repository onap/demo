/**************************************************************************//**
 * @file
 * Implementation of EVEL functions relating to the internal events.
 *
 * Internal events are never expected to be sent to the JSON API but comply
 * with interfaces for regular event types.  The primary use-case is to enable
 * the foreground processing to communicate with the background event handling
 * processing in an orderly fashion.  At present the only use is to initiate an
 * orderly shutdown of the Event Handler thread.
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

#include "evel.h"
#include "evel_internal.h"


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
EVENT_INTERNAL * evel_new_internal_event(EVT_HANDLER_COMMAND command)
{
  EVENT_INTERNAL * event = NULL;
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(command < EVT_CMD_MAX_COMMANDS);

  /***************************************************************************/
  /* Allocate the fault.                                                     */
  /***************************************************************************/
  event = malloc(sizeof(EVENT_INTERNAL));
  if (event == NULL)
  {
    log_error_state("Out of memory");
    goto exit_label;
  }
  memset(event, 0, sizeof(EVENT_INTERNAL));
  EVEL_DEBUG("New internal event is at %lp", event);

  /***************************************************************************/
  /* Initialize the header & the event fields.                               */
  /***************************************************************************/
  evel_init_header(&event->header);
  event->header.event_domain = EVEL_DOMAIN_INTERNAL;
  event->command = command;

exit_label:
  EVEL_EXIT();
  return event;
}

/**************************************************************************//**
 * Free an internal event.
 *
 * Free off the event supplied.  Will free all the contained allocated memory.
 *
 * @note It does not free the internal event itself, since that may be part of
 * a larger structure.
 *****************************************************************************/
void evel_free_internal_event(EVENT_INTERNAL * event)
{

  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.  As an internal API we don't allow freeing NULL    */
  /* events as we do on the public API.                                      */
  /***************************************************************************/
  assert(event != NULL);
  assert(event->header.event_domain == EVEL_DOMAIN_INTERNAL);

  /***************************************************************************/
  /* Free the header itself.                                                 */
  /***************************************************************************/
  evel_free_header(&event->header);

  EVEL_EXIT();
}
