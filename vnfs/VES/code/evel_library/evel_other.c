/**************************************************************************//**
 * @file
 * Implementation of EVEL functions relating to Other.
 *
 * License
 * -------
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
 * Create a new Other event.
 *
 * @note    The mandatory fields on the Other must be supplied to this factory
 *          function and are immutable once set.  Optional fields have explicit
 *          setter functions, but again values may only be set once so that the
 *          Other has immutable properties.
 * @returns pointer to the newly manufactured ::EVENT_OTHER.  If the event is
 *          not used (i.e. posted) it must be released using ::evel_free_other.
 * @retval  NULL  Failed to create the event.
 *****************************************************************************/
EVENT_OTHER * evel_new_other()
{
  EVENT_OTHER * other = NULL;
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/

  /***************************************************************************/
  /* Allocate the Other.                                                     */
  /***************************************************************************/
  other = malloc(sizeof(EVENT_OTHER));
  if (other == NULL)
  {
    log_error_state("Out of memory");
    goto exit_label;
  }
  memset(other, 0, sizeof(EVENT_OTHER));
  EVEL_DEBUG("New Other is at %lp", other);

  /***************************************************************************/
  /* Initialize the header & the Other fields.  Optional string values are   */
  /* uninitialized (NULL).                                                   */
  /***************************************************************************/
  evel_init_header(&other->header);
  other->header.event_domain = EVEL_DOMAIN_OTHER;
  dlist_initialize(&other->other_fields);

exit_label:
  EVEL_EXIT();
  return other;
}

/**************************************************************************//**
 * Set the Event Type property of the Other.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param other       Pointer to the Other.
 * @param type        The Event Type to be set. ASCIIZ string. The caller
 *                    does not need to preserve the value once the function
 *                    returns.
 *****************************************************************************/
void evel_other_type_set(EVENT_OTHER * other,
                         const char * const type)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions and call evel_header_type_set.                      */
  /***************************************************************************/
  assert(other != NULL);
  assert(other->header.event_domain == EVEL_DOMAIN_OTHER);
  evel_header_type_set(&other->header, type);

  EVEL_EXIT();
}

/**************************************************************************//**
 * Add a field name/value pair to the Other.
 *
 * The name and value are null delimited ASCII strings.  The library takes
 * a copy so the caller does not have to preserve values after the function
 * returns.
 *
 * @param other     Pointer to the Other.
 * @param name      ASCIIZ string with the field's name.  The caller does not
 *                  need to preserve the value once the function returns.
 * @param value     ASCIIZ string with the field's value.  The caller does not
 *                  need to preserve the value once the function returns.
 *****************************************************************************/
void evel_other_field_add(EVENT_OTHER * other, char * name, char * value)
{
  OTHER_FIELD * other_field = NULL;
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(other != NULL);
  assert(other->header.event_domain == EVEL_DOMAIN_OTHER);
  assert(name != NULL);
  assert(value != NULL);

  EVEL_DEBUG("Adding name=%s value=%s", name, value);
  other_field = malloc(sizeof(OTHER_FIELD));
  assert(other_field != NULL);
  memset(other_field, 0, sizeof(OTHER_FIELD));
  other_field->name = strdup(name);
  other_field->value = strdup(value);
  assert(other_field->name != NULL);
  assert(other_field->value != NULL);

  dlist_push_last(&other->other_fields, other_field);

  EVEL_EXIT();
}

/**************************************************************************//**
 * Encode the Other in JSON according to AT&T's schema for the event type.
 *
 * @param jbuf          Pointer to the ::EVEL_JSON_BUFFER to encode into.
 * @param event         Pointer to the ::EVENT_HEADER to encode.
 *****************************************************************************/
void evel_json_encode_other(EVEL_JSON_BUFFER * jbuf,
                            EVENT_OTHER * event)
{
  OTHER_FIELD * other_field = NULL;
  DLIST_ITEM * other_field_item = NULL;

  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(event != NULL);
  assert(event->header.event_domain == EVEL_DOMAIN_OTHER);

  evel_json_encode_header(jbuf, &event->header);
  evel_json_open_named_list(jbuf, "otherFields");
  other_field_item = dlist_get_first(&event->other_fields);
  while (other_field_item != NULL)
  {
    other_field = (OTHER_FIELD *) other_field_item->item;
    assert(other_field != NULL);

    evel_json_open_object(jbuf);
    evel_enc_kv_string(jbuf, "name", other_field->name);
    evel_enc_kv_string(jbuf, "value", other_field->value);
    evel_json_close_object(jbuf);
    other_field_item = dlist_get_next(other_field_item);
  }
  evel_json_close_list(jbuf);

  EVEL_EXIT();
}

/**************************************************************************//**
 * Free an Other.
 *
 * Free off the Other supplied.  Will free all the contained allocated memory.
 *
 * @note It does not free the Other itself, since that may be part of a
 * larger structure.
 *****************************************************************************/
void evel_free_other(EVENT_OTHER * event)
{
  OTHER_FIELD * other_field = NULL;

  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.  As an internal API we don't allow freeing NULL    */
  /* events as we do on the public API.                                      */
  /***************************************************************************/
  assert(event != NULL);
  assert(event->header.event_domain == EVEL_DOMAIN_OTHER);

  /***************************************************************************/
  /* Free all internal strings then the header itself.                       */
  /***************************************************************************/
  other_field = dlist_pop_last(&event->other_fields);
  while (other_field != NULL)
  {
    EVEL_DEBUG("Freeing Other Field (%s, %s)",
               other_field->name,
               other_field->value);
    free(other_field->name);
    free(other_field->value);
    free(other_field);
    other_field = dlist_pop_last(&event->other_fields);
  }
  evel_free_header(&event->header);

  EVEL_EXIT();
}