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
 * ECOMP is a trademark and service mark of AT&T Intellectual Property.
 ****************************************************************************/
/**************************************************************************//**
 * @file
 * Implementation of EVEL functions relating to Signaling.
 *
 ****************************************************************************/

#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include "evel_throttle.h"

/**************************************************************************//**
 * Create a new Signaling event.
 *
 * @note    The mandatory fields on the Signaling must be supplied to
 *          this factory function and are immutable once set.  Optional fields
 *          have explicit setter functions, but again values may only be set
 *          once so that the event has immutable properties.
 * @param event_name  Unique Event Name confirming Domain AsdcModel Description
 * @param event_id    A universal identifier of the event for: troubleshooting correlation, analysis, etc
 * @param vendor_name   The vendor id to encode in the event vnf field.
 * @param module        The module to encode in the event.
 * @param vnfname       The Virtual network function to encode in the event.
 * @returns pointer to the newly manufactured ::EVENT_SIGNALING.  If the event
 *          is not used (i.e. posted) it must be released using
 *          ::evel_free_signaling.
 * @retval  NULL  Failed to create the event.
 *****************************************************************************/
EVENT_SIGNALING * evel_new_signaling(const char* ev_name, const char *ev_id,
				     const char * const vendor_name,
                                     const char * const correlator,
                                     const char * const local_ip_address,
                                     const char * const local_port,
                                     const char * const remote_ip_address,
                                     const char * const remote_port)
{
  EVENT_SIGNALING * event = NULL;

  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(vendor_name != NULL);

  /***************************************************************************/
  /* Allocate the Signaling event.                                           */
  /***************************************************************************/
  event = malloc(sizeof(EVENT_SIGNALING));
  if (event == NULL)
  {
    log_error_state("Out of memory");
    goto exit_label;
  }
  memset(event, 0, sizeof(EVENT_SIGNALING));
  EVEL_DEBUG("New Signaling event is at %lp", event);

  /***************************************************************************/
  /* Initialize the header & the Signaling fields.                           */
  /***************************************************************************/
  evel_init_header_nameid(&event->header,ev_name,ev_id);
  event->header.event_domain = EVEL_DOMAIN_SIPSIGNALING;
  event->major_version = EVEL_SIGNALING_MAJOR_VERSION;
  event->minor_version = EVEL_SIGNALING_MINOR_VERSION;
  evel_init_vendor_field(&event->vnfname_field, vendor_name);
  evel_set_option_string(&event->correlator,correlator,"Init correlator");
  evel_set_option_string(&event->local_ip_address,local_ip_address,"Init correlator");
  evel_set_option_string(&event->local_port,local_port,"Init local port");
  evel_set_option_string(&event->remote_ip_address,remote_ip_address,"Init remote ip");
  evel_set_option_string(&event->remote_port,remote_port,"Init remote port");
  evel_init_option_string(&event->compressed_sip);
  evel_init_option_string(&event->summary_sip);
  dlist_initialize(&event->additional_info);

exit_label:

  EVEL_EXIT();
  return event;
}

/**************************************************************************//**
 * Add an additional value name/value pair to the SIP signaling.
 *
 * The name and value are null delimited ASCII strings.  The library takes
 * a copy so the caller does not have to preserve values after the function
 * returns.
 *
 * @param event     Pointer to the fault.
 * @param name      ASCIIZ string with the attribute's name.  The caller
 *                  does not need to preserve the value once the function
 *                  returns.
 * @param value     ASCIIZ string with the attribute's value.  The caller
 *                  does not need to preserve the value once the function
 *                  returns.
 *****************************************************************************/
void evel_signaling_addl_info_add(EVENT_SIGNALING * event, char * name, char * value)
{
  FAULT_ADDL_INFO * addl_info = NULL;
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(event != NULL);
  assert(event->header.event_domain == EVEL_DOMAIN_SIPSIGNALING);
  assert(name != NULL);
  assert(value != NULL);

  EVEL_DEBUG("Adding name=%s value=%s", name, value);
  addl_info = malloc(sizeof(SIGNALING_ADDL_FIELD));
  assert(addl_info != NULL);
  memset(addl_info, 0, sizeof(SIGNALING_ADDL_FIELD));
  addl_info->name = strdup(name);
  addl_info->value = strdup(value);
  assert(addl_info->name != NULL);
  assert(addl_info->value != NULL);

  dlist_push_last(&event->additional_info, addl_info);

  EVEL_EXIT();
}


/**************************************************************************//**
 * Set the Event Type property of the Signaling event.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param event         Pointer to the Signaling event.
 * @param type          The Event Type to be set. ASCIIZ string. The caller
 *                      does not need to preserve the value once the function
 *                      returns.
 *****************************************************************************/
void evel_signaling_type_set(EVENT_SIGNALING * const event,
                             const char * const type)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions and call evel_header_type_set.                      */
  /***************************************************************************/
  assert(event != NULL);
  assert(event->header.event_domain == EVEL_DOMAIN_SIPSIGNALING);
  evel_header_type_set(&event->header, type);

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Local Ip Address property of the Signaling event.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param event         Pointer to the Signaling event.
 * @param local_ip_address
 *                      The Local Ip Address to be set. ASCIIZ string. The
 *                      caller does not need to preserve the value once the
 *                      function returns.
 *****************************************************************************/
void evel_signaling_local_ip_address_set(EVENT_SIGNALING * const event,
                                         const char * const local_ip_address)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(event != NULL);
  assert(event->header.event_domain == EVEL_DOMAIN_SIPSIGNALING);
  assert(local_ip_address != NULL);

  evel_set_option_string(&event->local_ip_address,
                         local_ip_address,
                         "Local Ip Address");

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Local Port property of the Signaling event.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param event         Pointer to the Signaling event.
 * @param local_port    The Local Port to be set. ASCIIZ string. The caller
 *                      does not need to preserve the value once the function
 *                      returns.
 *****************************************************************************/
void evel_signaling_local_port_set(EVENT_SIGNALING * const event,
                                   const char * const local_port)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(event != NULL);
  assert(event->header.event_domain == EVEL_DOMAIN_SIPSIGNALING);
  assert(local_port != NULL);

  evel_set_option_string(&event->local_port,
                         local_port,
                         "Local Port");

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Remote Ip Address property of the Signaling event.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param event         Pointer to the Signaling event.
 * @param remote_ip_address
 *                      The Remote Ip Address to be set. ASCIIZ string. The
 *                      caller does not need to preserve the value once the
 *                      function returns.
 *****************************************************************************/
void evel_signaling_remote_ip_address_set(EVENT_SIGNALING * const event,
                                          const char * const remote_ip_address)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(event != NULL);
  assert(event->header.event_domain == EVEL_DOMAIN_SIPSIGNALING);
  assert(remote_ip_address != NULL);

  evel_set_option_string(&event->remote_ip_address,
                         remote_ip_address,
                         "Remote Ip Address");

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Remote Port property of the Signaling event.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param event         Pointer to the Signaling event.
 * @param remote_port   The Remote Port to be set. ASCIIZ string. The caller
 *                      does not need to preserve the value once the function
 *                      returns.
 *****************************************************************************/
void evel_signaling_remote_port_set(EVENT_SIGNALING * const event,
                                    const char * const remote_port)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(event != NULL);
  assert(event->header.event_domain == EVEL_DOMAIN_SIPSIGNALING);
  assert(remote_port != NULL);

  evel_set_option_string(&event->remote_port,
                         remote_port,
                         "Remote Port");

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Vendor module property of the Signaling event.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param event         Pointer to the Signaling event.
 * @param modulename    The module name to be set. ASCIIZ string. The caller
 *                      does not need to preserve the value once the function
 *                      returns.
 *****************************************************************************/
void evel_signaling_vnfmodule_name_set(EVENT_SIGNALING * const event,
                                    const char * const module_name)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(event != NULL);
  assert(event->header.event_domain == EVEL_DOMAIN_SIPSIGNALING);
  assert(module_name != NULL);

  evel_vendor_field_module_set(&event->vnfname_field, module_name);

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Vendor module property of the Signaling event.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param event         Pointer to the Signaling event.
 * @param vnfname       The Virtual Network function to be set. ASCIIZ string.
 *                      The caller does not need to preserve the value once
 *			the function returns.
 *****************************************************************************/
void evel_signaling_vnfname_set(EVENT_SIGNALING * const event,
                                    const char * const vnfname)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(event != NULL);
  assert(event->header.event_domain == EVEL_DOMAIN_SIPSIGNALING);
  assert(vnfname != NULL);

  evel_vendor_field_vnfname_set(&event->vnfname_field, vnfname);

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Compressed SIP property of the Signaling event.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param event         Pointer to the Signaling event.
 * @param compressed_sip
 *                      The Compressed SIP to be set. ASCIIZ string. The caller
 *                      does not need to preserve the value once the function
 *                      returns.
 *****************************************************************************/
void evel_signaling_compressed_sip_set(EVENT_SIGNALING * const event,
                                       const char * const compressed_sip)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(event != NULL);
  assert(event->header.event_domain == EVEL_DOMAIN_SIPSIGNALING);
  assert(compressed_sip != NULL);

  evel_set_option_string(&event->compressed_sip,
                         compressed_sip,
                         "Compressed SIP");

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Summary SIP property of the Signaling event.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param event         Pointer to the Signaling event.
 * @param summary_sip   The Summary SIP to be set. ASCIIZ string. The caller
 *                      does not need to preserve the value once the function
 *                      returns.
 *****************************************************************************/
void evel_signaling_summary_sip_set(EVENT_SIGNALING * const event,
                                    const char * const summary_sip)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(event != NULL);
  assert(event->header.event_domain == EVEL_DOMAIN_SIPSIGNALING);
  assert(summary_sip != NULL);

  evel_set_option_string(&event->summary_sip,
                         summary_sip,
                         "Summary SIP");

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Correlator property of the Signaling event.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param event         Pointer to the Signaling event.
 * @param correlator    The correlator to be set. ASCIIZ string. The caller
 *                      does not need to preserve the value once the function
 *                      returns.
 *****************************************************************************/
void evel_signaling_correlator_set(EVENT_SIGNALING * const event,
                                   const char * const correlator)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions and call evel_header_type_set.                      */
  /***************************************************************************/
  assert(event != NULL);
  assert(event->header.event_domain == EVEL_DOMAIN_SIPSIGNALING);
  evel_set_option_string(&event->correlator,
                         correlator,
                         "Correlator");

  EVEL_EXIT();
}

/**************************************************************************//**
 * Encode the Signaling in JSON according to AT&T's schema for the
 * event type.
 *
 * @param jbuf          Pointer to the ::EVEL_JSON_BUFFER to encode into.
 * @param event         Pointer to the ::EVENT_HEADER to encode.
 *****************************************************************************/
void evel_json_encode_signaling(EVEL_JSON_BUFFER * const jbuf,
                                EVENT_SIGNALING * const event)
{
  SIGNALING_ADDL_FIELD * addl_info = NULL;
  DLIST_ITEM * addl_info_item = NULL;

  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(event != NULL);
  assert(event->header.event_domain == EVEL_DOMAIN_SIPSIGNALING);

  evel_json_encode_header(jbuf, &event->header);
  evel_json_open_named_object(jbuf, "signalingFields");

  /***************************************************************************/
  /* Mandatory fields                                                        */
  /***************************************************************************/

  /***************************************************************************/
  /* Optional fields                                                         */
  /***************************************************************************/
  evel_enc_kv_opt_string(jbuf, "compressedSip", &event->compressed_sip);
  evel_enc_kv_opt_string(jbuf, "correlator", &event->correlator);
  evel_enc_kv_opt_string(jbuf, "localIpAddress", &event->local_ip_address);
  evel_enc_kv_opt_string(jbuf, "localPort", &event->local_port);
  evel_enc_kv_opt_string(jbuf, "remoteIpAddress", &event->remote_ip_address);
  evel_enc_kv_opt_string(jbuf, "remotePort", &event->remote_port);
  evel_enc_version(jbuf, "signalingFieldsVersion", event->major_version,event->minor_version);
  evel_enc_kv_opt_string(jbuf, "summarySip", &event->summary_sip);
  evel_json_encode_vendor_field(jbuf, &event->vnfname_field);


  /***************************************************************************/
  /* Checkpoint, so that we can wind back if all fields are suppressed.      */
  /***************************************************************************/
  evel_json_checkpoint(jbuf);
  if (evel_json_open_opt_named_list(jbuf, "additionalInformation"))
  {
    bool item_added = false;

    addl_info_item = dlist_get_first(&event->additional_info);
    while (addl_info_item != NULL)
    { 
      addl_info = (SIGNALING_ADDL_FIELD*) addl_info_item->item;
      assert(addl_info != NULL);

      if (!evel_throttle_suppress_nv_pair(jbuf->throttle_spec,
                                          "additionalInformation",
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

  evel_json_close_object(jbuf);

  EVEL_EXIT();
}

/**************************************************************************//**
 * Free a Signaling event.
 *
 * Free off the event supplied.  Will free all the contained allocated memory.
 *
 * @note It does not free the event itself, since that may be part of a larger
 * structure.
 *****************************************************************************/
void evel_free_signaling(EVENT_SIGNALING * const event)
{
  SIGNALING_ADDL_FIELD * addl_info = NULL;
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.  As an internal API we don't allow freeing NULL    */
  /* events as we do on the public API.                                      */
  /***************************************************************************/
  assert(event != NULL);
  assert(event->header.event_domain == EVEL_DOMAIN_SIPSIGNALING);

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

  evel_free_event_vendor_field(&event->vnfname_field);
  evel_free_option_string(&event->correlator);
  evel_free_option_string(&event->local_ip_address);
  evel_free_option_string(&event->local_port);
  evel_free_option_string(&event->remote_ip_address);
  evel_free_option_string(&event->remote_port);
  evel_free_option_string(&event->compressed_sip);
  evel_free_option_string(&event->summary_sip);
  evel_free_header(&event->header);

  EVEL_EXIT();
}
