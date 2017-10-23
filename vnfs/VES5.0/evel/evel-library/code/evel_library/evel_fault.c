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
 * Implementation of EVEL functions relating to the Fault.
 *
 ****************************************************************************/

#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include "evel.h"
#include "evel_internal.h"
#include "evel_throttle.h"

/**************************************************************************//**
 * Create a new fault event.
 *
 * @note    The mandatory fields on the Fault must be supplied to this factory
 *          function and are immutable once set.  Optional fields have explicit
 *          setter functions, but again values may only be set once so that the
 *          Fault has immutable properties.
 * @param event_name  Unique Event Name confirming Domain AsdcModel Description
 * @param event_id    A universal identifier of the event for: troubleshooting correlation, analysis, etc
 * @param   condition   The condition indicated by the Fault.
 * @param   specific_problem  The specific problem triggering the fault.
 * @param   priority    The priority of the event.
 * @param   severity    The severity of the Fault.
 * @param   ev_source_type    Source of Alarm event
 * @param   version     fault version
 * @param   status      status of Virtual Function
 * @returns pointer to the newly manufactured ::EVENT_FAULT.  If the event is
 *          not used (i.e. posted) it must be released using ::evel_free_fault.
 * @retval  NULL  Failed to create the event.
 *****************************************************************************/
EVENT_FAULT * evel_new_fault(const char * ev_name,
			     const char * ev_id,
			     const char * const condition,
                             const char * const specific_problem,
			     EVEL_EVENT_PRIORITIES priority,
                             EVEL_SEVERITIES severity,
                             EVEL_SOURCE_TYPES ev_source_type,
			     EVEL_VF_STATUSES status)
{
  EVENT_FAULT * fault = NULL;
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(condition != NULL);
  assert(specific_problem != NULL);
  assert(priority < EVEL_MAX_PRIORITIES);
  assert(severity < EVEL_MAX_SEVERITIES);

  /***************************************************************************/
  /* Allocate the fault.                                                     */
  /***************************************************************************/
  fault = malloc(sizeof(EVENT_FAULT));
  if (fault == NULL)
  {
    log_error_state("Out of memory");
    goto exit_label;
  }
  memset(fault, 0, sizeof(EVENT_FAULT));
  EVEL_DEBUG("New fault is at %lp", fault);

  /***************************************************************************/
  /* Initialize the header & the fault fields.  Optional string values are   */
  /* uninitialized (NULL).                                                   */
  /***************************************************************************/
  evel_init_header_nameid(&fault->header,ev_name,ev_id);
  fault->header.event_domain = EVEL_DOMAIN_FAULT;
  fault->header.priority = priority;
  fault->major_version = EVEL_FAULT_MAJOR_VERSION;
  fault->minor_version = EVEL_FAULT_MINOR_VERSION;
  fault->event_severity = severity;
  fault->event_source_type = ev_source_type;
  fault->vf_status = status;
  fault->alarm_condition = strdup(condition);
  fault->specific_problem = strdup(specific_problem);
  evel_init_option_string(&fault->category);
  evel_init_option_string(&fault->alarm_interface_a);
  dlist_initialize(&fault->additional_info);

exit_label:
  EVEL_EXIT();
  return fault;
}

/**************************************************************************//**
 * Add an additional value name/value pair to the Fault.
 *
 * The name and value are null delimited ASCII strings.  The library takes
 * a copy so the caller does not have to preserve values after the function
 * returns.
 *
 * @param fault     Pointer to the fault.
 * @param name      ASCIIZ string with the attribute's name.  The caller
 *                  does not need to preserve the value once the function
 *                  returns.
 * @param value     ASCIIZ string with the attribute's value.  The caller
 *                  does not need to preserve the value once the function
 *                  returns.
 *****************************************************************************/
void evel_fault_addl_info_add(EVENT_FAULT * fault, char * name, char * value)
{
  FAULT_ADDL_INFO * addl_info = NULL;
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(fault != NULL);
  assert(fault->header.event_domain == EVEL_DOMAIN_FAULT);
  assert(name != NULL);
  assert(value != NULL);

  EVEL_DEBUG("Adding name=%s value=%s", name, value);
  addl_info = malloc(sizeof(FAULT_ADDL_INFO));
  assert(addl_info != NULL);
  memset(addl_info, 0, sizeof(FAULT_ADDL_INFO));
  addl_info->name = strdup(name);
  addl_info->value = strdup(value);
  assert(addl_info->name != NULL);
  assert(addl_info->value != NULL);

  dlist_push_last(&fault->additional_info, addl_info);

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Fault Category property of the Fault.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param fault      Pointer to the fault.
 * @param category   Category : license, link, routing, security, signaling.
 *			 ASCIIZ string. The caller
 *                   does not need to preserve the value once the function
 *                   returns.
 *****************************************************************************/
void evel_fault_category_set(EVENT_FAULT * fault,
                              const char * const category)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(fault != NULL);
  assert(fault->header.event_domain == EVEL_DOMAIN_FAULT);
  assert(category != NULL);

  evel_set_option_string(&fault->category,
                         category,
                         "Fault Category set");
  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Alarm Interface A property of the Fault.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param fault      Pointer to the fault.
 * @param interface  The Alarm Interface A to be set. ASCIIZ string. The caller
 *                   does not need to preserve the value once the function
 *                   returns.
 *****************************************************************************/
void evel_fault_interface_set(EVENT_FAULT * fault,
                              const char * const interface)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(fault != NULL);
  assert(fault->header.event_domain == EVEL_DOMAIN_FAULT);
  assert(interface != NULL);

  evel_set_option_string(&fault->alarm_interface_a,
                         interface,
                         "Alarm Interface A");
  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Event Type property of the Fault.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param fault      Pointer to the fault.
 * @param type       The Event Type to be set. ASCIIZ string. The caller
 *                   does not need to preserve the value once the function
 *                   returns.
 *****************************************************************************/
void evel_fault_type_set(EVENT_FAULT * fault, const char * const type)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions and call evel_header_type_set.                      */
  /***************************************************************************/
  assert(fault != NULL);
  assert(fault->header.event_domain == EVEL_DOMAIN_FAULT);
  evel_header_type_set(&fault->header, type);

  EVEL_EXIT();
}

/**************************************************************************//**
 * Encode the fault in JSON according to AT&T's schema for the fault type.
 *
 * @param jbuf          Pointer to the ::EVEL_JSON_BUFFER to encode into.
 * @param event         Pointer to the ::EVENT_HEADER to encode.
 *****************************************************************************/
void evel_json_encode_fault(EVEL_JSON_BUFFER * jbuf,
                            EVENT_FAULT * event)
{
  FAULT_ADDL_INFO * addl_info = NULL;
  DLIST_ITEM * addl_info_item = NULL;
  char * fault_severity;
  char * fault_source_type;
  char * fault_vf_status;

  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(event != NULL);
  assert(event->header.event_domain == EVEL_DOMAIN_FAULT);

  fault_severity = evel_severity(event->event_severity);
  fault_source_type = evel_source_type(event->event_source_type);
  fault_vf_status = evel_vf_status(event->vf_status);

  evel_json_encode_header(jbuf, &event->header);
  evel_json_open_named_object(jbuf, "faultFields");

  /***************************************************************************/
  /* Mandatory fields.                                                       */
  /***************************************************************************/
  evel_enc_kv_string(jbuf, "alarmCondition", event->alarm_condition);
  evel_enc_kv_opt_string(jbuf, "eventCategory", &event->category);
  evel_enc_kv_string(jbuf, "eventSeverity", fault_severity);
  evel_enc_kv_string(jbuf, "eventSourceType", fault_source_type);
  evel_enc_kv_string(jbuf, "specificProblem", event->specific_problem);
  evel_enc_kv_string(jbuf, "vfStatus", fault_vf_status);
  evel_enc_version(
    jbuf, "faultFieldsVersion", event->major_version, event->minor_version);

  /***************************************************************************/
  /* Optional fields.                                                        */
  /***************************************************************************/

  /***************************************************************************/
  /* Checkpoint, so that we can wind back if all fields are suppressed.      */
  /***************************************************************************/
  evel_json_checkpoint(jbuf);
  if (evel_json_open_opt_named_list(jbuf, "alarmAdditionalInformation"))
  {
    bool item_added = false;

    addl_info_item = dlist_get_first(&event->additional_info);
    while (addl_info_item != NULL)
    {
      addl_info = (FAULT_ADDL_INFO*) addl_info_item->item;
      assert(addl_info != NULL);

      if (!evel_throttle_suppress_nv_pair(jbuf->throttle_spec,
                                          "alarmAdditionalInformation",
                                          addl_info->name))
      {
        evel_json_open_object(jbuf);
        evel_enc_kv_string(jbuf, "name", addl_info->name);
        evel_enc_kv_string(jbuf, "value", addl_info->value);
        evel_json_close_object(jbuf);
        item_added = true;
      }
      addl_info_item = dlist_get_next(addl_info_item);
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
  evel_enc_kv_opt_string(jbuf, "alarmInterfaceA", &event->alarm_interface_a);

  evel_json_close_object(jbuf);

  EVEL_EXIT();
}

/**************************************************************************//**
 * Free a Fault.
 *
 * Free off the Fault supplied.  Will free all the contained allocated memory.
 *
 * @note It does not free the Fault itself, since that may be part of a
 * larger structure.
 *****************************************************************************/
void evel_free_fault(EVENT_FAULT * event)
{
  FAULT_ADDL_INFO * addl_info = NULL;

  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.  As an internal API we don't allow freeing NULL    */
  /* events as we do on the public API.                                      */
  /***************************************************************************/
  assert(event != NULL);
  assert(event->header.event_domain == EVEL_DOMAIN_FAULT);

  /***************************************************************************/
  /* Free all internal strings then the header itself.                       */
  /***************************************************************************/
  addl_info = dlist_pop_last(&event->additional_info);
  while (addl_info != NULL)
  {
    EVEL_DEBUG("Freeing Additional Info (%s, %s)",
               addl_info->name,
               addl_info->value);
    free(addl_info->name);
    free(addl_info->value);
    free(addl_info);
    addl_info = dlist_pop_last(&event->additional_info);
  }
  free(event->alarm_condition);
  free(event->specific_problem);
  evel_free_option_string(&event->category);
  evel_free_option_string(&event->alarm_interface_a);
  evel_free_header(&event->header);

  EVEL_EXIT();
}
