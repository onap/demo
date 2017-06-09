/**************************************************************************//**
 * @file
 * Implementation of EVEL functions relating to the Syslog.
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

#include "evel_throttle.h"

/**************************************************************************//**
 * Create a new Syslog event.
 *
 * @note    The mandatory fields on the Syslog must be supplied to this factory
 *          function and are immutable once set.  Optional fields have explicit
 *          setter functions, but again values may only be set once so that the
 *          Syslog has immutable properties.
 * @param   event_source_type  The type of Syslog event source.
 * @param   syslog_msg         The Syslog event message.
 * @param   syslog_tag         The messgaeId identifying the type of message.
 * @returns pointer to the newly manufactured ::EVENT_SYSLOG.  If the event is
 *          not used (i.e. posted) it must be released using
 *          ::evel_free_syslog.
 * @retval  NULL  Failed to create the event.
 *****************************************************************************/
EVENT_SYSLOG * evel_new_syslog(EVEL_SOURCE_TYPES event_source_type,
                               const char * const syslog_msg,
                               const char * const syslog_tag)
{
  EVENT_SYSLOG * syslog = NULL;
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(event_source_type < EVEL_MAX_SOURCE_TYPES);
  assert(syslog_msg != NULL);
  assert(syslog_tag != NULL);

  /***************************************************************************/
  /* Allocate the Syslog.                                                    */
  /***************************************************************************/
  syslog = malloc(sizeof(EVENT_SYSLOG));
  if (syslog == NULL)
  {
    log_error_state("Out of memory");
    goto exit_label;
  }
  memset(syslog, 0, sizeof(EVENT_SYSLOG));
  EVEL_DEBUG("New Syslog is at %lp", syslog);

  /***************************************************************************/
  /* Initialize the header & the Syslog fields.  Optional string values are  */
  /* uninitialized (NULL).                                                   */
  /***************************************************************************/
  evel_init_header(&syslog->header);
  syslog->header.event_domain = EVEL_DOMAIN_SYSLOG;
  syslog->major_version = EVEL_SYSLOG_MAJOR_VERSION;
  syslog->minor_version = EVEL_SYSLOG_MINOR_VERSION;
  syslog->event_source_type = event_source_type;
  syslog->syslog_msg = strdup(syslog_msg);
  syslog->syslog_tag = strdup(syslog_tag);
  dlist_initialize(&syslog->additional_fields);
  evel_init_option_int(&syslog->syslog_facility);
  evel_init_option_int(&syslog->syslog_proc_id);
  evel_init_option_int(&syslog->syslog_ver);
  evel_init_option_string(&syslog->event_source_host);
  evel_init_option_string(&syslog->syslog_proc);
  evel_init_option_string(&syslog->syslog_s_data);

exit_label:
  EVEL_EXIT();
  return syslog;
}

/**************************************************************************//**
 * Set the Event Type property of the Syslog.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param syslog      Pointer to the syslog.
 * @param type        The Event Type to be set. ASCIIZ string. The caller
 *                    does not need to preserve the value once the function
 *                    returns.
 *****************************************************************************/
void evel_syslog_type_set(EVENT_SYSLOG * syslog,
                          const char * const type)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions and call evel_header_type_set.                      */
  /***************************************************************************/
  assert(syslog != NULL);
  assert(syslog->header.event_domain == EVEL_DOMAIN_SYSLOG);
  evel_header_type_set(&syslog->header, type);

  EVEL_EXIT();
}

/**************************************************************************//**
 * Add an additional value name/value pair to the Syslog.
 *
 * The name and value are null delimited ASCII strings.  The library takes
 * a copy so the caller does not have to preserve values after the function
 * returns.
 *
 * @param syslog    Pointer to the syslog.
 * @param name      ASCIIZ string with the attribute's name.  The caller
 *                  does not need to preserve the value once the function
 *                  returns.
 * @param value     ASCIIZ string with the attribute's value.  The caller
 *                  does not need to preserve the value once the function
 *                  returns.
 *****************************************************************************/
void evel_syslog_addl_field_add(EVENT_SYSLOG * syslog,
                                char * name,
                                char * value)
{
  SYSLOG_ADDL_FIELD * addl_field = NULL;
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(syslog != NULL);
  assert(syslog->header.event_domain == EVEL_DOMAIN_SYSLOG);
  assert(name != NULL);
  assert(value != NULL);

  EVEL_DEBUG("Adding name=%s value=%s", name, value);
  addl_field = malloc(sizeof(SYSLOG_ADDL_FIELD));
  assert(addl_field != NULL);
  memset(addl_field, 0, sizeof(SYSLOG_ADDL_FIELD));
  addl_field->name = strdup(name);
  addl_field->value = strdup(value);
  assert(addl_field->name != NULL);
  assert(addl_field->value != NULL);

  dlist_push_last(&syslog->additional_fields, addl_field);

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Event Source Host property of the Syslog.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param syslog     Pointer to the Syslog.
 * @param host       The Event Source Host to be set. ASCIIZ string. The caller
 *                   does not need to preserve the value once the function
 *                   returns.
 *****************************************************************************/
void evel_syslog_event_source_host_set(EVENT_SYSLOG * syslog,
                                       const char * const host)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(syslog != NULL);
  assert(syslog->header.event_domain == EVEL_DOMAIN_SYSLOG);
  assert(host != NULL);

  evel_set_option_string(&syslog->event_source_host,
                         host,
                         "Event Source Host");
  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Facility property of the Syslog.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param syslog      Pointer to the Syslog.
 * @param facility    The Syslog Facility to be set.  ASCIIZ string. The caller
 *                    does not need to preserve the value once the function
 *                    returns.
 *****************************************************************************/
void evel_syslog_facility_set(EVENT_SYSLOG * syslog,
                              EVEL_SYSLOG_FACILITIES facility)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(syslog != NULL);
  assert(syslog->header.event_domain == EVEL_DOMAIN_SYSLOG);
  assert(facility < EVEL_MAX_SYSLOG_FACILITIES);

  evel_set_option_int(&syslog->syslog_facility,
                      facility,
                      "Facility");
  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Process property of the Syslog.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param syslog     Pointer to the Syslog.
 * @param proc       The Process to be set. ASCIIZ string. The caller does not
 *                   need to preserve the value once the function returns.
 *****************************************************************************/
void evel_syslog_proc_set(EVENT_SYSLOG * syslog, const char * const proc)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(syslog != NULL);
  assert(syslog->header.event_domain == EVEL_DOMAIN_SYSLOG);
  assert(proc != NULL);

  evel_set_option_string(&syslog->syslog_proc, proc, "Process");
  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Process ID property of the Syslog.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param syslog     Pointer to the Syslog.
 * @param proc_id    The Process ID to be set. ASCIIZ string. The caller does
 *                   not need to preserve the value once the function returns.
 *****************************************************************************/
void evel_syslog_proc_id_set(EVENT_SYSLOG * syslog, int proc_id)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(syslog != NULL);
  assert(syslog->header.event_domain == EVEL_DOMAIN_SYSLOG);
  assert(proc_id > 0);

  evel_set_option_int(&syslog->syslog_proc_id,
                      proc_id,
                      "Process ID");
  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Version property of the Syslog.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param syslog     Pointer to the Syslog.
 * @param version    The Version to be set. ASCIIZ string. The caller does not
 *                   need to preserve the value once the function returns.
 *****************************************************************************/
void evel_syslog_version_set(EVENT_SYSLOG * syslog, int version)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(syslog != NULL);
  assert(syslog->header.event_domain == EVEL_DOMAIN_SYSLOG);
  assert(version >= 0);

  evel_set_option_int(&syslog->syslog_ver,
                      version,
                      "Version");
  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Structured Data property of the Syslog.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param syslog     Pointer to the Syslog.
 * @param s_data     The Structured Data to be set. ASCIIZ string. The caller
 *                   does not need to preserve the value once the function
 *                   returns.
 *****************************************************************************/
void evel_syslog_s_data_set(EVENT_SYSLOG * syslog, const char * const s_data)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(syslog != NULL);
  assert(syslog->header.event_domain == EVEL_DOMAIN_SYSLOG);
  assert(s_data != NULL);

  evel_set_option_string(&syslog->syslog_s_data,
                         s_data,
                         "Structured Data");
  EVEL_EXIT();
}

/**************************************************************************//**
 * Encode the Syslog in JSON according to AT&T's schema for the event type.
 *
 * @param jbuf          Pointer to the ::EVEL_JSON_BUFFER to encode into.
 * @param event         Pointer to the ::EVENT_HEADER to encode.
 *****************************************************************************/
void evel_json_encode_syslog(EVEL_JSON_BUFFER * jbuf,
                             EVENT_SYSLOG * event)
{
  char * event_source_type;
  SYSLOG_ADDL_FIELD * addl_field = NULL;
  DLIST_ITEM * addl_field_item = NULL;

  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(event != NULL);
  assert(event->header.event_domain == EVEL_DOMAIN_SYSLOG);

  event_source_type = evel_source_type(event->event_source_type);

  evel_json_encode_header(jbuf, &event->header);
  evel_json_open_named_object(jbuf, "syslogFields");

  /***************************************************************************/
  /* Mandatory fields                                                        */
  /***************************************************************************/
  evel_enc_kv_string(jbuf, "eventSourceType", event_source_type);
  evel_enc_kv_string(jbuf, "syslogMsg", event->syslog_msg);
  evel_enc_kv_string(jbuf, "syslogTag", event->syslog_tag);
  evel_enc_version(
    jbuf, "syslogFieldsVersion", event->major_version, event->minor_version);

  /***************************************************************************/
  /* Optional fields                                                         */
  /***************************************************************************/
  evel_json_checkpoint(jbuf);
  if (evel_json_open_opt_named_list(jbuf, "additionalFields"))
  {
    bool item_added = false;

    addl_field_item = dlist_get_first(&event->additional_fields);
    while (addl_field_item != NULL)
    {
      addl_field = (SYSLOG_ADDL_FIELD *) addl_field_item->item;
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

  evel_enc_kv_opt_string(jbuf, "eventSourceHost", &event->event_source_host);
  evel_enc_kv_opt_int(jbuf, "syslogFacility", &event->syslog_facility);
  evel_enc_kv_opt_string(jbuf, "syslogProc", &event->syslog_proc);
  evel_enc_kv_opt_int(jbuf, "syslogProcId", &event->syslog_proc_id);
  evel_enc_kv_opt_string(jbuf, "syslogSData", &event->syslog_s_data);
  evel_enc_kv_opt_int(jbuf, "syslogVer", &event->syslog_ver);
  evel_json_close_object(jbuf);

  EVEL_CT_ASSERT(EVEL_SYSLOG_FACILITY_KERNEL == 0);
  EVEL_CT_ASSERT(EVEL_SYSLOG_FACILITY_USER == 1);
  EVEL_CT_ASSERT(EVEL_SYSLOG_FACILITY_MAIL == 2);
  EVEL_CT_ASSERT(EVEL_SYSLOG_FACILITY_SYSTEM_DAEMON == 3);
  EVEL_CT_ASSERT(EVEL_SYSLOG_FACILITY_SECURITY_AUTH == 4);
  EVEL_CT_ASSERT(EVEL_SYSLOG_FACILITY_INTERNAL == 5);
  EVEL_CT_ASSERT(EVEL_SYSLOG_FACILITY_LINE_PRINTER == 6);
  EVEL_CT_ASSERT(EVEL_SYSLOG_FACILITY_NETWORK_NEWS == 7);
  EVEL_CT_ASSERT(EVEL_SYSLOG_FACILITY_UUCP == 8);
  EVEL_CT_ASSERT(EVEL_SYSLOG_FACILITY_CLOCK_DAEMON == 9);
  EVEL_CT_ASSERT(EVEL_SYSLOG_FACILITY_SECURITY_AUTH2 == 10);
  EVEL_CT_ASSERT(EVEL_SYSLOG_FACILITY_FTP_DAEMON == 11);
  EVEL_CT_ASSERT(EVEL_SYSLOG_FACILITY_NTP == 12);
  EVEL_CT_ASSERT(EVEL_SYSLOG_FACILITY_LOG_AUDIT == 13);
  EVEL_CT_ASSERT(EVEL_SYSLOG_FACILITY_LOG_ALERT == 14);
  EVEL_CT_ASSERT(EVEL_SYSLOG_FACILITY_CLOCK_DAEMON2 == 15);
  EVEL_CT_ASSERT(EVEL_SYSLOG_FACILITY_LOCAL0 == 16);
  EVEL_CT_ASSERT(EVEL_SYSLOG_FACILITY_LOCAL1 == 17);
  EVEL_CT_ASSERT(EVEL_SYSLOG_FACILITY_LOCAL2 == 18);
  EVEL_CT_ASSERT(EVEL_SYSLOG_FACILITY_LOCAL3 == 19);
  EVEL_CT_ASSERT(EVEL_SYSLOG_FACILITY_LOCAL4 == 20);
  EVEL_CT_ASSERT(EVEL_SYSLOG_FACILITY_LOCAL5 == 21);
  EVEL_CT_ASSERT(EVEL_SYSLOG_FACILITY_LOCAL6 == 22);
  EVEL_CT_ASSERT(EVEL_SYSLOG_FACILITY_LOCAL7 == 23);

  EVEL_EXIT();
}

/**************************************************************************//**
 * Free a Syslog.
 *
 * Free off the Syslog supplied.  Will free all the contained allocated memory.
 *
 * @note It does not free the Syslog itself, since that may be part of a
 * larger structure.
 *****************************************************************************/
void evel_free_syslog(EVENT_SYSLOG * event)
{
  SYSLOG_ADDL_FIELD * addl_field = NULL;

  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.  As an internal API we don't allow freeing NULL    */
  /* events as we do on the public API.                                      */
  /***************************************************************************/
  assert(event != NULL);
  assert(event->header.event_domain == EVEL_DOMAIN_SYSLOG);

  /***************************************************************************/
  /* Free all internal strings then the header itself.                       */
  /***************************************************************************/
  addl_field = dlist_pop_last(&event->additional_fields);
  while (addl_field != NULL)
  {
    EVEL_DEBUG("Freeing Additional Field (%s, %s)",
               addl_field->name,
               addl_field->value);
    free(addl_field->name);
    free(addl_field->value);
    free(addl_field);
    addl_field = dlist_pop_last(&event->additional_fields);
  }

  evel_free_option_string(&event->event_source_host);
  free(event->syslog_msg);
  evel_free_option_string(&event->syslog_proc);
  evel_free_option_string(&event->syslog_s_data);
  free(event->syslog_tag);
  evel_free_header(&event->header);

  EVEL_EXIT();
}
