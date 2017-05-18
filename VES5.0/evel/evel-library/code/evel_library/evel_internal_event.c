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
  evel_init_header(&event->header,NULL);
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
