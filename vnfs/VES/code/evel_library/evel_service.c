/**************************************************************************//**
 * @file
 * Implementation of EVEL functions relating to Service.
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
 * Create a new Service event.
 *
 * @note    The mandatory fields on the Service must be supplied to
 *          this factory function and are immutable once set.  Optional fields
 *          have explicit setter functions, but again values may only be set
 *          once so that the event has immutable properties.
 * @param vendor_id     The vendor id to encode in the event instance id.
 * @param event_id      The vendor event id to encode in the event instance id.
 * @returns pointer to the newly manufactured ::EVENT_SERVICE.  If the event
 *          is not used (i.e. posted) it must be released using
 *          ::evel_free_service.
 * @retval  NULL  Failed to create the event.
 *****************************************************************************/
EVENT_SERVICE * evel_new_service(const char * const vendor_id,
                                 const char * const event_id)
{
  EVENT_SERVICE * event = NULL;

  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(vendor_id != NULL);
  assert(event_id != NULL);

  /***************************************************************************/
  /* Allocate the Service event.                                           */
  /***************************************************************************/
  event = malloc(sizeof(EVENT_SERVICE));
  if (event == NULL)
  {
    log_error_state("Out of memory");
    goto exit_label;
  }
  memset(event, 0, sizeof(EVENT_SERVICE));
  EVEL_DEBUG("New Service event is at %lp", event);

  /***************************************************************************/
  /* Initialize the header & the Service fields.                           */
  /***************************************************************************/
  evel_init_header(&event->header);
  event->header.event_domain = EVEL_DOMAIN_SERVICE;
  event->major_version = EVEL_SERVICE_MAJOR_VERSION;
  event->minor_version = EVEL_SERVICE_MINOR_VERSION;
  evel_init_event_instance_id(&event->instance_id, vendor_id, event_id);
  evel_init_option_string(&event->correlator);
  dlist_initialize(&event->additional_fields);
  evel_init_option_string(&event->codec);
  evel_init_option_string(&event->callee_side_codec);
  evel_init_option_string(&event->caller_side_codec);
  evel_init_option_string(&event->rtcp_data);
  evel_init_option_string(&event->adjacency_name);
  evel_init_option_string(&event->endpoint_description);
  evel_init_option_int(&event->endpoint_jitter);
  evel_init_option_int(&event->endpoint_rtp_oct_disc);
  evel_init_option_int(&event->endpoint_rtp_oct_recv);
  evel_init_option_int(&event->endpoint_rtp_oct_sent);
  evel_init_option_int(&event->endpoint_rtp_pkt_disc);
  evel_init_option_int(&event->endpoint_rtp_pkt_recv);
  evel_init_option_int(&event->endpoint_rtp_pkt_sent);
  evel_init_option_int(&event->local_jitter);
  evel_init_option_int(&event->local_rtp_oct_disc);
  evel_init_option_int(&event->local_rtp_oct_recv);
  evel_init_option_int(&event->local_rtp_oct_sent);
  evel_init_option_int(&event->local_rtp_pkt_disc);
  evel_init_option_int(&event->local_rtp_pkt_recv);
  evel_init_option_int(&event->local_rtp_pkt_sent);
  evel_init_option_double(&event->mos_cqe);
  evel_init_option_int(&event->packets_lost);
  evel_init_option_double(&event->packet_loss_percent);
  evel_init_option_int(&event->r_factor);
  evel_init_option_int(&event->round_trip_delay);
  evel_init_option_string(&event->phone_number);

exit_label:

  EVEL_EXIT();
  return event;
}

/**************************************************************************//**
 * Set the Event Type property of the Service event.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param event         Pointer to the Service event.
 * @param type          The Event Type to be set. ASCIIZ string. The caller
 *                      does not need to preserve the value once the function
 *                      returns.
 *****************************************************************************/
void evel_service_type_set(EVENT_SERVICE * const event,
                           const char * const type)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions and call evel_header_type_set.                      */
  /***************************************************************************/
  assert(event != NULL);
  assert(event->header.event_domain == EVEL_DOMAIN_SERVICE);
  evel_header_type_set(&event->header, type);

  EVEL_EXIT();
}

/**************************************************************************//**
 * Add a name/value pair to the Service, under the additionalFields array.
 *
 * The name and value are null delimited ASCII strings.  The library takes
 * a copy so the caller does not have to preserve values after the function
 * returns.
 *
 * @param event     Pointer to the Service event.
 * @param name      ASCIIZ string with the field's name.  The caller does not
 *                  need to preserve the value once the function returns.
 * @param value     ASCIIZ string with the field's value.  The caller does not
 *                  need to preserve the value once the function returns.
 *****************************************************************************/
void evel_service_addl_field_add(EVENT_SERVICE * const event,
                                 const char * const name,
                                 const char * const value)
{
  OTHER_FIELD * nv_pair = NULL;

  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(event != NULL);
  assert(event->header.event_domain == EVEL_DOMAIN_SERVICE);
  assert(name != NULL);
  assert(value != NULL);

  EVEL_DEBUG("Adding name=%s value=%s", name, value);
  nv_pair = malloc(sizeof(OTHER_FIELD));
  assert(nv_pair != NULL);
  nv_pair->name = strdup(name);
  nv_pair->value = strdup(value);
  assert(nv_pair->name != NULL);
  assert(nv_pair->value != NULL);

  dlist_push_last(&event->additional_fields, nv_pair);

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Product Id property of the Service event.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param event         Pointer to the Service event.
 * @param product_id    The vendor product id to be set. ASCIIZ string. The
 *                      caller does not need to preserve the value once the
 *                      function returns.
 *****************************************************************************/
void evel_service_product_id_set(EVENT_SERVICE * const event,
                                 const char * const product_id)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions and call evel_set_option_string.                    */
  /***************************************************************************/
  assert(event != NULL);
  assert(event->header.event_domain == EVEL_DOMAIN_SERVICE);
  evel_set_option_string(&event->instance_id.product_id,
                         product_id,
                         "Product Id");

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Subsystem Id property of the Service event.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param event         Pointer to the Service event.
 * @param subsystem_id  The vendor subsystem id to be set. ASCIIZ string. The
 *                      caller does not need to preserve the value once the
 *                      function returns.
 *****************************************************************************/
void evel_service_subsystem_id_set(EVENT_SERVICE * const event,
                                   const char * const subsystem_id)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions and call evel_set_option_string.                    */
  /***************************************************************************/
  assert(event != NULL);
  assert(event->header.event_domain == EVEL_DOMAIN_SERVICE);
  evel_set_option_string(&event->instance_id.subsystem_id,
                         subsystem_id,
                         "Subsystem Id");

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Friendly Name property of the Service event.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param event         Pointer to the Service event.
 * @param friendly_name The vendor friendly name to be set. ASCIIZ string. The
 *                      caller does not need to preserve the value once the
 *                      function returns.
 *****************************************************************************/
void evel_service_friendly_name_set(EVENT_SERVICE * const event,
                                    const char * const friendly_name)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions and call evel_set_option_string.                    */
  /***************************************************************************/
  assert(event != NULL);
  assert(event->header.event_domain == EVEL_DOMAIN_SERVICE);
  evel_set_option_string(&event->instance_id.event_friendly_name,
                         friendly_name,
                         "Friendly Name");

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Correlator property of the Service event.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param event         Pointer to the Service event.
 * @param correlator    The correlator to be set. ASCIIZ string. The caller
 *                      does not need to preserve the value once the function
 *                      returns.
 *****************************************************************************/
void evel_service_correlator_set(EVENT_SERVICE * const event,
                                 const char * const correlator)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions and call evel_set_option_string.                    */
  /***************************************************************************/
  assert(event != NULL);
  assert(event->header.event_domain == EVEL_DOMAIN_SERVICE);
  evel_set_option_string(&event->correlator,
                         correlator,
                         "Correlator");

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Codec property of the Service event.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param event         Pointer to the Service event.
 * @param codec         The codec to be set. ASCIIZ string. The caller does not
 *                      need to preserve the value once the function returns.
 *****************************************************************************/
void evel_service_codec_set(EVENT_SERVICE * const event,
                            const char * const codec)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions and call evel_set_option_string.                    */
  /***************************************************************************/
  assert(event != NULL);
  assert(event->header.event_domain == EVEL_DOMAIN_SERVICE);
  evel_set_option_string(&event->codec,
                         codec,
                         "Codec");

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Callee Side Codec property of the Service event.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param event         Pointer to the Service event.
 * @param codec         The codec to be set. ASCIIZ string. The caller does not
 *                      need to preserve the value once the function returns.
 *****************************************************************************/
void evel_service_callee_codec_set(EVENT_SERVICE * const event,
                                   const char * const codec)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions and call evel_set_option_string.                    */
  /***************************************************************************/
  assert(event != NULL);
  assert(event->header.event_domain == EVEL_DOMAIN_SERVICE);
  evel_set_option_string(&event->callee_side_codec,
                         codec,
                         "Callee Side Codec");

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Caller Side Codec property of the Service event.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param event         Pointer to the Service event.
 * @param codec         The codec to be set. ASCIIZ string. The caller does not
 *                      need to preserve the value once the function returns.
 *****************************************************************************/
void evel_service_caller_codec_set(EVENT_SERVICE * const event,
                                   const char * const codec)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions and call evel_set_option_string.                    */
  /***************************************************************************/
  assert(event != NULL);
  assert(event->header.event_domain == EVEL_DOMAIN_SERVICE);
  evel_set_option_string(&event->caller_side_codec,
                         codec,
                         "Caller Side Codec");

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the RTCP Data property of the Service event.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param event         Pointer to the Service event.
 * @param rtcp_data     The RTCP Data to be set. ASCIIZ string. The caller
 *                      does not need to preserve the value once the function
 *                      returns.
 *****************************************************************************/
void evel_service_rtcp_data_set(EVENT_SERVICE * const event,
                                const char * const rtcp_data)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions and call evel_set_option_string.                    */
  /***************************************************************************/
  assert(event != NULL);
  assert(event->header.event_domain == EVEL_DOMAIN_SERVICE);
  evel_set_option_string(&event->rtcp_data,
                         rtcp_data,
                         "RTCP Data");

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Adjacency Name property of the Service event.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param event         Pointer to the Service event.
 * @param adjacency_name
 *                      The adjacency name to be set. ASCIIZ string. The caller
 *                      does not need to preserve the value once the function
 *                      returns.
 *****************************************************************************/
void evel_service_adjacency_name_set(EVENT_SERVICE * const event,
                                     const char * const adjacency_name)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions and call evel_set_option_string.                    */
  /***************************************************************************/
  assert(event != NULL);
  assert(event->header.event_domain == EVEL_DOMAIN_SERVICE);
  evel_set_option_string(&event->adjacency_name,
                         adjacency_name,
                         "Adjacency Name");

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Endpoint Descriptor property of the Service event.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param event         Pointer to the Service event.
 * @param endpoint_desc The endpoint descriptor to be set.
 *****************************************************************************/
void evel_service_endpoint_desc_set(
                                EVENT_SERVICE * const event,
                                const EVEL_SERVICE_ENDPOINT_DESC endpoint_desc)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions and call evel_set_option_string.                    */
  /***************************************************************************/
  assert(event != NULL);
  assert(event->header.event_domain == EVEL_DOMAIN_SERVICE);
  evel_set_option_string(&event->endpoint_description,
                         evel_service_endpoint_desc(endpoint_desc),
                         "Endpoint Description");

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Endpoint Jitter property of the Service event.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param event         Pointer to the Service event.
 * @param jitter        The jitter to be set.
 *****************************************************************************/
void evel_service_endpoint_jitter_set(EVENT_SERVICE * const event,
                                      const int jitter)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions and call evel_set_option_string.                    */
  /***************************************************************************/
  assert(event != NULL);
  assert(event->header.event_domain == EVEL_DOMAIN_SERVICE);
  evel_set_option_int(&event->endpoint_jitter,
                      jitter,
                      "Endpoint Jitter");

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Endpoint Rtp Octets Discarded property of the Service event.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param event         Pointer to the Service event.
 * @param rtp_oct_disc  The discard count.
 *****************************************************************************/
void evel_service_endpoint_rtp_oct_disc_set(EVENT_SERVICE * const event,
                                            const int rtp_oct_disc)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions and call evel_set_option_string.                    */
  /***************************************************************************/
  assert(event != NULL);
  assert(event->header.event_domain == EVEL_DOMAIN_SERVICE);
  evel_set_option_int(&event->endpoint_rtp_oct_disc,
                      rtp_oct_disc,
                      "Endpoint Rtp Octets Discarded");

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Endpoint Rtp Octets Received property of the Service event.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param event         Pointer to the Service event.
 * @param rtp_oct_recv  The receive count.
 *****************************************************************************/
void evel_service_endpoint_rtp_oct_recv_set(EVENT_SERVICE * const event,
                                            const int rtp_oct_recv)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions and call evel_set_option_string.                    */
  /***************************************************************************/
  assert(event != NULL);
  assert(event->header.event_domain == EVEL_DOMAIN_SERVICE);
  evel_set_option_int(&event->endpoint_rtp_oct_recv,
                      rtp_oct_recv,
                      "Endpoint Rtp Octets Received");

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Endpoint Rtp Octets Sent property of the Service event.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param event         Pointer to the Service event.
 * @param rtp_oct_sent  The send count.
 *****************************************************************************/
void evel_service_endpoint_rtp_oct_sent_set(EVENT_SERVICE * const event,
                                            const int rtp_oct_sent)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions and call evel_set_option_string.                    */
  /***************************************************************************/
  assert(event != NULL);
  assert(event->header.event_domain == EVEL_DOMAIN_SERVICE);
  evel_set_option_int(&event->endpoint_rtp_oct_sent,
                      rtp_oct_sent,
                      "Endpoint Rtp Octets Sent");

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Endpoint Rtp Packets Discarded property of the Service event.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param event         Pointer to the Service event.
 * @param rtp_pkt_disc  The discard count.
 *****************************************************************************/
void evel_service_endpoint_rtp_pkt_disc_set(EVENT_SERVICE * const event,
                                            const int rtp_pkt_disc)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions and call evel_set_option_string.                    */
  /***************************************************************************/
  assert(event != NULL);
  assert(event->header.event_domain == EVEL_DOMAIN_SERVICE);
  evel_set_option_int(&event->endpoint_rtp_pkt_disc,
                      rtp_pkt_disc,
                      "Endpoint Rtp Packets Discarded");

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Endpoint Rtp Packets Received property of the Service event.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param event         Pointer to the Service event.
 * @param rtp_pkt_recv  The receive count.
 *****************************************************************************/
void evel_service_endpoint_rtp_pkt_recv_set(EVENT_SERVICE * const event,
                                            const int rtp_pkt_recv)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions and call evel_set_option_string.                    */
  /***************************************************************************/
  assert(event != NULL);
  assert(event->header.event_domain == EVEL_DOMAIN_SERVICE);
  evel_set_option_int(&event->endpoint_rtp_pkt_recv,
                      rtp_pkt_recv,
                      "Endpoint Rtp Packets Received");

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Endpoint Rtp Packets Sent property of the Service event.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param event         Pointer to the Service event.
 * @param rtp_pkt_sent  The send count.
 *****************************************************************************/
void evel_service_endpoint_rtp_pkt_sent_set(EVENT_SERVICE * const event,
                                            const int rtp_pkt_sent)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions and call evel_set_option_string.                    */
  /***************************************************************************/
  assert(event != NULL);
  assert(event->header.event_domain == EVEL_DOMAIN_SERVICE);
  evel_set_option_int(&event->endpoint_rtp_pkt_sent,
                      rtp_pkt_sent,
                      "Endpoint Rtp Packets Sent");

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Local Jitter property of the Service event.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param event         Pointer to the Service event.
 * @param jitter        The jitter to be set.
 *****************************************************************************/
void evel_service_local_jitter_set(EVENT_SERVICE * const event,
                                   const int jitter)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions and call evel_set_option_string.                    */
  /***************************************************************************/
  assert(event != NULL);
  assert(event->header.event_domain == EVEL_DOMAIN_SERVICE);
  evel_set_option_int(&event->local_jitter,
                      jitter,
                      "Local Jitter");

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Local Rtp Octets Discarded property of the Service event.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param event         Pointer to the Service event.
 * @param rtp_oct_disc  The discard count.
 *****************************************************************************/
void evel_service_local_rtp_oct_disc_set(EVENT_SERVICE * const event,
                                         const int rtp_oct_disc)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions and call evel_set_option_string.                    */
  /***************************************************************************/
  assert(event != NULL);
  assert(event->header.event_domain == EVEL_DOMAIN_SERVICE);
  evel_set_option_int(&event->local_rtp_oct_disc,
                      rtp_oct_disc,
                      "Local Rtp Octets Discarded");

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Local Rtp Octets Received property of the Service event.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param event         Pointer to the Service event.
 * @param rtp_oct_recv  The receive count.
 *****************************************************************************/
void evel_service_local_rtp_oct_recv_set(EVENT_SERVICE * const event,
                                         const int rtp_oct_recv)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions and call evel_set_option_string.                    */
  /***************************************************************************/
  assert(event != NULL);
  assert(event->header.event_domain == EVEL_DOMAIN_SERVICE);
  evel_set_option_int(&event->local_rtp_oct_recv,
                      rtp_oct_recv,
                      "Local Rtp Octets Received");

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Local Rtp Octets Sent property of the Service event.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param event         Pointer to the Service event.
 * @param rtp_oct_sent  The send count.
 *****************************************************************************/
void evel_service_local_rtp_oct_sent_set(EVENT_SERVICE * const event,
                                         const int rtp_oct_sent)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions and call evel_set_option_string.                    */
  /***************************************************************************/
  assert(event != NULL);
  assert(event->header.event_domain == EVEL_DOMAIN_SERVICE);
  evel_set_option_int(&event->local_rtp_oct_sent,
                      rtp_oct_sent,
                      "Local Rtp Octets Sent");

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Local Rtp Packets Discarded property of the Service event.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param event         Pointer to the Service event.
 * @param rtp_pkt_disc  The discard count.
 *****************************************************************************/
void evel_service_local_rtp_pkt_disc_set(EVENT_SERVICE * const event,
                                         const int rtp_pkt_disc)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions and call evel_set_option_string.                    */
  /***************************************************************************/
  assert(event != NULL);
  assert(event->header.event_domain == EVEL_DOMAIN_SERVICE);
  evel_set_option_int(&event->local_rtp_pkt_disc,
                      rtp_pkt_disc,
                      "Local Rtp Packets Discarded");

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Local Rtp Packets Received property of the Service event.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param event         Pointer to the Service event.
 * @param rtp_pkt_recv  The receive count.
 *****************************************************************************/
void evel_service_local_rtp_pkt_recv_set(EVENT_SERVICE * const event,
                                         const int rtp_pkt_recv)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions and call evel_set_option_string.                    */
  /***************************************************************************/
  assert(event != NULL);
  assert(event->header.event_domain == EVEL_DOMAIN_SERVICE);
  evel_set_option_int(&event->local_rtp_pkt_recv,
                      rtp_pkt_recv,
                      "Local Rtp Packets Received");

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Local Rtp Packets Sent property of the Service event.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param event         Pointer to the Service event.
 * @param rtp_pkt_sent  The send count.
 *****************************************************************************/
void evel_service_local_rtp_pkt_sent_set(EVENT_SERVICE * const event,
                                         const int rtp_pkt_sent)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions and call evel_set_option_string.                    */
  /***************************************************************************/
  assert(event != NULL);
  assert(event->header.event_domain == EVEL_DOMAIN_SERVICE);
  evel_set_option_int(&event->local_rtp_pkt_sent,
                      rtp_pkt_sent,
                      "Local Rtp Packets Sent");

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Mos Cqe property of the Service event.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param event         Pointer to the Service event.
 * @param mos_cqe       The mosCqe to be set.
 *****************************************************************************/
void evel_service_mos_cqe_set(EVENT_SERVICE * const event,
                              const double mos_cqe)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions and call evel_set_option_string.                    */
  /***************************************************************************/
  assert(event != NULL);
  assert(event->header.event_domain == EVEL_DOMAIN_SERVICE);
  evel_set_option_double(&event->mos_cqe,
                         mos_cqe,
                         "Mos Cqe");

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Packets Lost property of the Service event.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param event         Pointer to the Service event.
 * @param packets_lost  The number of packets lost to be set.
 *****************************************************************************/
void evel_service_packets_lost_set(EVENT_SERVICE * const event,
                                   const int packets_lost)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions and call evel_set_option_string.                    */
  /***************************************************************************/
  assert(event != NULL);
  assert(event->header.event_domain == EVEL_DOMAIN_SERVICE);
  evel_set_option_int(&event->packets_lost,
                      packets_lost,
                      "Packets Lost");

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the packet Loss Percent property of the Service event.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param event         Pointer to the Service event.
 * @param packet_loss_percent
 *                      The packet loss in percent.
 *****************************************************************************/
void evel_service_packet_loss_percent_set(EVENT_SERVICE * const event,
                                          const double packet_loss_percent)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions and call evel_set_option_string.                    */
  /***************************************************************************/
  assert(event != NULL);
  assert(event->header.event_domain == EVEL_DOMAIN_SERVICE);
  evel_set_option_double(&event->packet_loss_percent,
                         packet_loss_percent,
                         "Packet Loss Percent");

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the R Factor property of the Service event.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param event         Pointer to the Service event.
 * @param r_factor      The R Factor to be set.
 *****************************************************************************/
void evel_service_r_factor_set(EVENT_SERVICE * const event,
                               const int r_factor)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions and call evel_set_option_string.                    */
  /***************************************************************************/
  assert(event != NULL);
  assert(event->header.event_domain == EVEL_DOMAIN_SERVICE);
  evel_set_option_int(&event->r_factor,
                      r_factor,
                      "R Factor");

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Round Trip Delay property of the Service event.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param event         Pointer to the Service event.
 * @param round_trip_delay
 *                      The Round trip delay to be set.
 *****************************************************************************/
void evel_service_round_trip_delay_set(EVENT_SERVICE * const event,
                                       const int round_trip_delay)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions and call evel_set_option_string.                    */
  /***************************************************************************/
  assert(event != NULL);
  assert(event->header.event_domain == EVEL_DOMAIN_SERVICE);
  evel_set_option_int(&event->round_trip_delay,
                      round_trip_delay,
                      "Round Trip Delay");

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Phone Number property of the Service event.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param event         Pointer to the Service event.
 * @param phone_number  The Phone Number to be set. ASCIIZ string. The caller
 *                      does not need to preserve the value once the function
 *                      returns.
 *****************************************************************************/
void evel_service_phone_number_set(EVENT_SERVICE * const event,
                                   const char * const phone_number)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions and call evel_set_option_string.                    */
  /***************************************************************************/
  assert(event != NULL);
  assert(event->header.event_domain == EVEL_DOMAIN_SERVICE);
  evel_set_option_string(&event->phone_number,
                         phone_number,
                         "Phone Number");

  EVEL_EXIT();
}

/**************************************************************************//**
 * Encode the Service in JSON according to AT&T's schema for the
 * event type.
 *
 * @param jbuf          Pointer to the ::EVEL_JSON_BUFFER to encode into.
 * @param event         Pointer to the ::EVENT_HEADER to encode.
 *****************************************************************************/
void evel_json_encode_service(EVEL_JSON_BUFFER * const jbuf,
                                EVENT_SERVICE * const event)
{
  OTHER_FIELD * nv_pair = NULL;
  DLIST_ITEM * dlist_item = NULL;

  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(event != NULL);
  assert(event->header.event_domain == EVEL_DOMAIN_SERVICE);

  evel_json_encode_header(jbuf, &event->header);
  evel_json_open_named_object(jbuf, "serviceEventsFields");

  /***************************************************************************/
  /* Mandatory fields                                                        */
  /***************************************************************************/
  evel_json_encode_instance_id(jbuf, &event->instance_id);
  evel_enc_version(jbuf,
                   "serviceEventsFieldsVersion",
                   event->major_version,
                   event->minor_version);

  /***************************************************************************/
  /* Optional fields                                                         */
  /***************************************************************************/
  evel_enc_kv_opt_string(jbuf, "correlator", &event->correlator);

  /***************************************************************************/
  /* Checkpoint, so that we can wind back if all fields are suppressed.      */
  /***************************************************************************/
  evel_json_checkpoint(jbuf);
  if (evel_json_open_opt_named_list(jbuf, "additionalFields"))
  {
    bool added = false;

    dlist_item = dlist_get_first(&event->additional_fields);
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

  /***************************************************************************/
  /* Optional fields within JSON equivalent object: codecSelected            */
  /***************************************************************************/
  evel_json_checkpoint(jbuf);
  if (evel_json_open_opt_named_object(jbuf, "codecSelected"))
  {
    bool added = false;
    added |= evel_enc_kv_opt_string(jbuf,
                                    "codec",
                                    &event->codec);
    evel_json_close_object(jbuf);

    /*************************************************************************/
    /* If the object is empty, rewind to before we opened it.                */
    /*************************************************************************/
    if (!added)
    {
      evel_json_rewind(jbuf);
    }
  }

  /***************************************************************************/
  /* Optional fields within JSON equivalent object: codecSelectedTranscoding */
  /***************************************************************************/
  evel_json_checkpoint(jbuf);
  if (evel_json_open_opt_named_object(jbuf, "codecSelectedTranscoding"))
  {
    bool added = false;
    added |= evel_enc_kv_opt_string(jbuf,
                                    "calleeSideCodec",
                                    &event->callee_side_codec);
    added |= evel_enc_kv_opt_string(jbuf,
                                    "callerSideCodec",
                                    &event->caller_side_codec);
    evel_json_close_object(jbuf);

    /*************************************************************************/
    /* If the object is empty, rewind to before we opened it.                */
    /*************************************************************************/
    if (!added)
    {
      evel_json_rewind(jbuf);
    }
  }

  /***************************************************************************/
  /* Optional fields within JSON equivalent object: midCallRtcp              */
  /***************************************************************************/
  evel_json_checkpoint(jbuf);
  if (evel_json_open_opt_named_object(jbuf, "midCallRtcp"))
  {
    bool added = false;
    added |= evel_enc_kv_opt_string(jbuf,
                                    "rtcpData",
                                    &event->rtcp_data);
    evel_json_close_object(jbuf);

    /*************************************************************************/
    /* If the object is empty, rewind to before we opened it.                */
    /*************************************************************************/
    if (!added)
    {
      evel_json_rewind(jbuf);
    }
  }

  /***************************************************************************/
  /* Optional fields within JSON equivalent object: endOfCallVqmSummaries    */
  /***************************************************************************/
  evel_json_checkpoint(jbuf);
  if (evel_json_open_opt_named_object(jbuf, "endOfCallVqmSummaries"))
  {
    bool added = false;
    added |= evel_enc_kv_opt_string(jbuf,
                                    "adjacencyName",
                                    &event->adjacency_name);
    added |= evel_enc_kv_opt_string(jbuf,
                                    "endpointDescription",
                                    &event->endpoint_description);
    added |= evel_enc_kv_opt_int(jbuf,
                                 "endpointJitter",
                                 &event->endpoint_jitter);
    added |= evel_enc_kv_opt_int(jbuf,
                                 "endpointRtpOctetsDiscarded",
                                 &event->endpoint_rtp_oct_disc);
    added |= evel_enc_kv_opt_int(jbuf,
                                 "endpointRtpOctetsReceived",
                                 &event->endpoint_rtp_oct_recv);
    added |= evel_enc_kv_opt_int(jbuf,
                                 "endpointRtpOctetsSent",
                                 &event->endpoint_rtp_oct_sent);
    added |= evel_enc_kv_opt_int(jbuf,
                                 "endpointRtpPacketsDiscarded",
                                 &event->endpoint_rtp_pkt_disc);
    added |= evel_enc_kv_opt_int(jbuf,
                                 "endpointRtpPacketsReceived",
                                 &event->endpoint_rtp_pkt_recv);
    added |= evel_enc_kv_opt_int(jbuf,
                                 "endpointRtpPacketsSent",
                                 &event->endpoint_rtp_pkt_sent);
    added |= evel_enc_kv_opt_int(jbuf,
                                 "localJitter",
                                 &event->local_jitter);
    added |= evel_enc_kv_opt_int(jbuf,
                                 "localRtpOctetsDiscarded",
                                 &event->local_rtp_oct_disc);
    added |= evel_enc_kv_opt_int(jbuf,
                                 "localRtpOctetsReceived",
                                 &event->local_rtp_oct_recv);
    added |= evel_enc_kv_opt_int(jbuf,
                                 "localRtpOctetsSent",
                                 &event->local_rtp_oct_sent);
    added |= evel_enc_kv_opt_int(jbuf,
                                 "localRtpPacketsDiscarded",
                                 &event->local_rtp_pkt_disc);
    added |= evel_enc_kv_opt_int(jbuf,
                                 "localRtpPacketsReceived",
                                 &event->local_rtp_pkt_recv);
    added |= evel_enc_kv_opt_int(jbuf,
                                 "localRtpPacketsSent",
                                 &event->local_rtp_pkt_sent);
    added |= evel_enc_kv_opt_double(jbuf,
                                    "mosCqe",
                                    &event->mos_cqe);
    added |= evel_enc_kv_opt_int(jbuf,
                                 "packetsLost",
                                 &event->packets_lost);
    added |= evel_enc_kv_opt_double(jbuf,
                                    "packetLossPercent",
                                    &event->packet_loss_percent);
    added |= evel_enc_kv_opt_int(jbuf,
                                 "rFactor",
                                 &event->r_factor);
    added |= evel_enc_kv_opt_int(jbuf,
                                 "roundTripDelay",
                                 &event->round_trip_delay);
    evel_json_close_object(jbuf);

    /*************************************************************************/
    /* If the object is empty, rewind to before we opened it.                */
    /*************************************************************************/
    if (!added)
    {
      evel_json_rewind(jbuf);
    }
  }

  /***************************************************************************/
  /* Optional fields within JSON equivalent object: marker                   */
  /***************************************************************************/
  evel_json_checkpoint(jbuf);
  if (evel_json_open_opt_named_object(jbuf, "marker"))
  {
    bool added = false;
    added |= evel_enc_kv_opt_string(jbuf, "phoneNumber", &event->phone_number);
    evel_json_close_object(jbuf);

    /*************************************************************************/
    /* If the object is empty, rewind to before we opened it.                */
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
 * Free a Service event.
 *
 * Free off the event supplied.  Will free all the contained allocated memory.
 *
 * @note It does not free the event itself, since that may be part of a larger
 * structure.
 *****************************************************************************/
void evel_free_service(EVENT_SERVICE * const event)
{
  OTHER_FIELD * nv_pair = NULL;

  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(event != NULL);
  assert(event->header.event_domain == EVEL_DOMAIN_SERVICE);

  /***************************************************************************/
  /* Free all internal strings then the header itself.                       */
  /***************************************************************************/
  nv_pair = dlist_pop_last(&event->additional_fields);
  while (nv_pair != NULL)
  {
    EVEL_DEBUG("Freeing Other Field (%s, %s)", nv_pair->name, nv_pair->value);
    free(nv_pair->name);
    free(nv_pair->value);
    free(nv_pair);
    nv_pair = dlist_pop_last(&event->additional_fields);
  }
  evel_free_option_string(&event->correlator);
  evel_free_option_string(&event->codec);
  evel_free_option_string(&event->callee_side_codec);
  evel_free_option_string(&event->caller_side_codec);
  evel_free_option_string(&event->rtcp_data);
  evel_free_option_string(&event->adjacency_name);
  evel_free_option_string(&event->endpoint_description);
  evel_free_option_string(&event->phone_number);
  evel_free_event_instance_id(&event->instance_id);
  evel_free_header(&event->header);

  EVEL_EXIT();
}
