/**************************************************************************//**
 * @file
 * Implementation of EVEL functions relating to Event Headers - since
 * Heartbeats only contain the Event Header, the Heartbeat factory function is
 * here too.
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
#include <sys/time.h>

#include "evel.h"
#include "evel_internal.h"
#include "evel_throttle.h"
#include "metadata.h"

/**************************************************************************//**
 * Unique sequence number for events from this VNF.
 *****************************************************************************/
static int event_sequence = 1;

/**************************************************************************//**
 * Set the next event_sequence to use.
 *
 * @param sequence      The next sequence number to use.
 *****************************************************************************/
void evel_set_next_event_sequence(const int sequence)
{
  EVEL_ENTER();

  EVEL_INFO("Setting event sequence to %d, was %d ", sequence, event_sequence);
  event_sequence = sequence;

  EVEL_EXIT();
}

/**************************************************************************//**
 * Create a new heartbeat event.
 *
 * @note that the heartbeat is just a "naked" commonEventHeader!
 *
 * @returns pointer to the newly manufactured ::EVENT_HEADER.  If the event is
 *          not used it must be released using ::evel_free_event
 * @retval  NULL  Failed to create the event.
 *****************************************************************************/
EVENT_HEADER * evel_new_heartbeat()
{
  EVENT_HEADER * heartbeat = NULL;
  EVEL_ENTER();

  /***************************************************************************/
  /* Allocate the header.                                                    */
  /***************************************************************************/
  heartbeat = malloc(sizeof(EVENT_HEADER));
  if (heartbeat == NULL)
  {
    log_error_state("Out of memory");
    goto exit_label;
  }
  memset(heartbeat, 0, sizeof(EVENT_HEADER));

  /***************************************************************************/
  /* Initialize the header.  Get a new event sequence number.  Note that if  */
  /* any memory allocation fails in here we will fail gracefully because     */
  /* everything downstream can cope with NULLs.                              */
  /***************************************************************************/
  evel_init_header(heartbeat);
  evel_force_option_string(&heartbeat->event_type, "Autonomous heartbeat");

exit_label:
  EVEL_EXIT();
  return heartbeat;
}

/**************************************************************************//**
 * Initialize a newly created event header.
 *
 * @param header  Pointer to the header being initialized.
 *****************************************************************************/
void evel_init_header(EVENT_HEADER * const header)
{
  char scratchpad[EVEL_MAX_STRING_LEN + 1] = {0};
  struct timeval tv;

  EVEL_ENTER();

  assert(header != NULL);

  gettimeofday(&tv, NULL);

  /***************************************************************************/
  /* Initialize the header.  Get a new event sequence number.  Note that if  */
  /* any memory allocation fails in here we will fail gracefully because     */
  /* everything downstream can cope with NULLs.                              */
  /***************************************************************************/
  header->event_domain = EVEL_DOMAIN_HEARTBEAT;
  snprintf(scratchpad, EVEL_MAX_STRING_LEN, "%d", event_sequence);
  header->event_id = strdup(scratchpad);
  header->functional_role = strdup(functional_role);
  header->last_epoch_microsec = tv.tv_usec + 1000000 * tv.tv_sec;
  header->priority = EVEL_PRIORITY_NORMAL;
  header->reporting_entity_name = strdup(openstack_vm_name());
  header->source_name = strdup(openstack_vm_name());
  header->sequence = event_sequence;
  header->start_epoch_microsec = header->last_epoch_microsec;
  header->major_version = EVEL_HEADER_MAJOR_VERSION;
  header->minor_version = EVEL_HEADER_MINOR_VERSION;
  event_sequence++;

  /***************************************************************************/
  /* Optional parameters.                                                    */
  /***************************************************************************/
  evel_init_option_string(&header->event_type);
  evel_force_option_string(&header->reporting_entity_id, openstack_vm_uuid());
  evel_force_option_string(&header->source_id, openstack_vm_uuid());

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Event Type property of the event header.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param header        Pointer to the ::EVENT_HEADER.
 * @param type          The Event Type to be set. ASCIIZ string. The caller
 *                      does not need to preserve the value once the function
 *                      returns.
 *****************************************************************************/
void evel_header_type_set(EVENT_HEADER * const header,
                          const char * const type)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(header != NULL);
  assert(type != NULL);

  evel_set_option_string(&header->event_type, type, "Event Type");

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Start Epoch property of the event header.
 *
 * @note The Start Epoch defaults to the time of event creation.
 *
 * @param header        Pointer to the ::EVENT_HEADER.
 * @param start_epoch_microsec
 *                      The start epoch to set, in microseconds.
 *****************************************************************************/
void evel_start_epoch_set(EVENT_HEADER * const header,
                          const unsigned long long start_epoch_microsec)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions and assign the new value.                           */
  /***************************************************************************/
  assert(header != NULL);
  header->start_epoch_microsec = start_epoch_microsec;

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Last Epoch property of the event header.
 *
 * @note The Last Epoch defaults to the time of event creation.
 *
 * @param header        Pointer to the ::EVENT_HEADER.
 * @param last_epoch_microsec
 *                      The last epoch to set, in microseconds.
 *****************************************************************************/
void evel_last_epoch_set(EVENT_HEADER * const header,
                         const unsigned long long last_epoch_microsec)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions and assign the new value.                           */
  /***************************************************************************/
  assert(header != NULL);
  header->last_epoch_microsec = last_epoch_microsec;

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Reporting Entity Name property of the event header.
 *
 * @note The Reporting Entity Name defaults to the OpenStack VM Name.
 *
 * @param header        Pointer to the ::EVENT_HEADER.
 * @param entity_name   The entity name to set.
 *****************************************************************************/
void evel_reporting_entity_name_set(EVENT_HEADER * const header,
                                    const char * const entity_name)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions and assign the new value.                           */
  /***************************************************************************/
  assert(header != NULL);
  assert(entity_name != NULL);
  assert(header->reporting_entity_name != NULL);

  /***************************************************************************/
  /* Free the previously allocated memory and replace it with a copy of the  */
  /* provided one.                                                           */
  /***************************************************************************/
  free(header->reporting_entity_name);
  header->reporting_entity_name = strdup(entity_name);

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Reporting Entity Id property of the event header.
 *
 * @note The Reporting Entity Id defaults to the OpenStack VM UUID.
 *
 * @param header        Pointer to the ::EVENT_HEADER.
 * @param entity_id     The entity id to set.
 *****************************************************************************/
void evel_reporting_entity_id_set(EVENT_HEADER * const header,
                                  const char * const entity_id)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions and assign the new value.                           */
  /***************************************************************************/
  assert(header != NULL);
  assert(entity_id != NULL);

  /***************************************************************************/
  /* Free the previously allocated memory and replace it with a copy of the  */
  /* provided one.  Note that evel_force_option_string strdups entity_id.    */
  /***************************************************************************/
  evel_free_option_string(&header->reporting_entity_id);
  evel_force_option_string(&header->reporting_entity_id, entity_id);

  EVEL_EXIT();
}

/**************************************************************************//**
 * Encode the event as a JSON event object according to AT&T's schema.
 *
 * @param jbuf          Pointer to the ::EVEL_JSON_BUFFER to encode into.
 * @param event         Pointer to the ::EVENT_HEADER to encode.
 *****************************************************************************/
void evel_json_encode_header(EVEL_JSON_BUFFER * jbuf,
                             EVENT_HEADER * event)
{
  char * domain;
  char * priority;

  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(jbuf != NULL);
  assert(jbuf->json != NULL);
  assert(jbuf->max_size > 0);
  assert(event != NULL);

  domain = evel_event_domain(event->event_domain);
  priority = evel_event_priority(event->priority);
  evel_json_open_named_object(jbuf, "commonEventHeader");

  /***************************************************************************/
  /* Mandatory fields.                                                       */
  /***************************************************************************/
  evel_enc_kv_string(jbuf, "domain", domain);
  evel_enc_kv_string(jbuf, "eventId", event->event_id);
  evel_enc_kv_string(jbuf, "functionalRole", event->functional_role);
  evel_enc_kv_ull(jbuf, "lastEpochMicrosec", event->last_epoch_microsec);
  evel_enc_kv_string(jbuf, "priority", priority);
  evel_enc_kv_string(
    jbuf, "reportingEntityName", event->reporting_entity_name);
  evel_enc_kv_int(jbuf, "sequence", event->sequence);
  evel_enc_kv_string(jbuf, "sourceName", event->source_name);
  evel_enc_kv_ull(jbuf, "startEpochMicrosec", event->start_epoch_microsec);
  evel_enc_version(
    jbuf, "version", event->major_version, event->minor_version);

  /***************************************************************************/
  /* Optional fields.                                                        */
  /***************************************************************************/
  evel_enc_kv_opt_string(jbuf, "eventType", &event->event_type);
  evel_enc_kv_opt_string(
    jbuf, "reportingEntityId", &event->reporting_entity_id);
  evel_enc_kv_opt_string(jbuf, "sourceId", &event->source_id);

  evel_json_close_object(jbuf);

  EVEL_EXIT();
}

/**************************************************************************//**
 * Free an event header.
 *
 * Free off the event header supplied.  Will free all the contained allocated
 * memory.
 *
 * @note It does not free the header itself, since that may be part of a
 * larger structure.
 *****************************************************************************/
void evel_free_header(EVENT_HEADER * const event)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.  As an internal API we don't allow freeing NULL    */
  /* events as we do on the public API.                                      */
  /***************************************************************************/
  assert(event != NULL);

  /***************************************************************************/
  /* Free all internal strings.                                              */
  /***************************************************************************/
  free(event->event_id);
  evel_free_option_string(&event->event_type);
  free(event->functional_role);
  evel_free_option_string(&event->reporting_entity_id);
  free(event->reporting_entity_name);
  evel_free_option_string(&event->source_id);
  free(event->source_name);

  EVEL_EXIT();
}

/**************************************************************************//**
 * Encode the event as a JSON event object according to AT&T's schema.
 *
 * @param json      Pointer to where to store the JSON encoded data.
 * @param max_size  Size of storage available in json_body.
 * @param event     Pointer to the ::EVENT_HEADER to encode.
 * @returns Number of bytes actually written.
 *****************************************************************************/
int evel_json_encode_event(char * json,
                           int max_size,
                           EVENT_HEADER * event)
{
  EVEL_JSON_BUFFER json_buffer;
  EVEL_JSON_BUFFER * jbuf = &json_buffer;
  EVEL_THROTTLE_SPEC * throttle_spec;

  EVEL_ENTER();

  /***************************************************************************/
  /* Get the latest throttle specification for the domain.                   */
  /***************************************************************************/
  throttle_spec = evel_get_throttle_spec(event->event_domain);

  /***************************************************************************/
  /* Initialize the JSON_BUFFER and open the top-level objects.              */
  /***************************************************************************/
  evel_json_buffer_init(jbuf, json, max_size, throttle_spec);
  evel_json_open_object(jbuf);
  evel_json_open_named_object(jbuf, "event");

  switch (event->event_domain)
  {
    case EVEL_DOMAIN_HEARTBEAT:
      evel_json_encode_header(jbuf, event);
      break;

    case EVEL_DOMAIN_FAULT:
      evel_json_encode_fault(jbuf, (EVENT_FAULT *)event);
      break;

    case EVEL_DOMAIN_MEASUREMENT:
      evel_json_encode_measurement(jbuf, (EVENT_MEASUREMENT *)event);
      break;

    case EVEL_DOMAIN_MOBILE_FLOW:
      evel_json_encode_mobile_flow(jbuf, (EVENT_MOBILE_FLOW *)event);
      break;

    case EVEL_DOMAIN_REPORT:
      evel_json_encode_report(jbuf, (EVENT_REPORT *)event);
      break;

    case EVEL_DOMAIN_SERVICE:
      evel_json_encode_service(jbuf, (EVENT_SERVICE *)event);
      break;

    case EVEL_DOMAIN_SIGNALING:
      evel_json_encode_signaling(jbuf, (EVENT_SIGNALING *)event);
      break;

    case EVEL_DOMAIN_STATE_CHANGE:
      evel_json_encode_state_change(jbuf, (EVENT_STATE_CHANGE *)event);
      break;

    case EVEL_DOMAIN_SYSLOG:
      evel_json_encode_syslog(jbuf, (EVENT_SYSLOG *)event);
      break;

    case EVEL_DOMAIN_OTHER:
      evel_json_encode_other(jbuf, (EVENT_OTHER *)event);
      break;

    case EVEL_DOMAIN_INTERNAL:
    default:
      EVEL_ERROR("Unexpected domain %d", event->event_domain);
      assert(0);
  }

  evel_json_close_object(jbuf);
  evel_json_close_object(jbuf);

  /***************************************************************************/
  /* Sanity check.                                                           */
  /***************************************************************************/
  assert(jbuf->depth == 0);

  EVEL_EXIT();

  return jbuf->offset;
}

/**************************************************************************//**
 * Initialize an event instance id.
 *
 * @param instance_id   Pointer to the event instance id being initialized.
 * @param vendor_id     The vendor id to encode in the event instance id.
 * @param event_id      The event id to encode in the event instance id.
 *****************************************************************************/
void evel_init_event_instance_id(EVEL_EVENT_INSTANCE_ID * const instance_id,
                                 const char * const vendor_id,
                                 const char * const event_id)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(instance_id != NULL);
  assert(vendor_id != NULL);
  assert(event_id != NULL);

  /***************************************************************************/
  /* Store the mandatory parts.                                              */
  /***************************************************************************/
  instance_id->vendor_id = strdup(vendor_id);
  instance_id->event_id = strdup(event_id);

  /***************************************************************************/
  /* Initialize the optional parts.                                          */
  /***************************************************************************/
  evel_init_option_string(&instance_id->product_id);
  evel_init_option_string(&instance_id->subsystem_id);
  evel_init_option_string(&instance_id->event_friendly_name);

  EVEL_EXIT();
}

/**************************************************************************//**
 * Free an event instance id.
 *
 * @param instance_id   Pointer to the event instance id being initialized.
 *****************************************************************************/
void evel_free_event_instance_id(EVEL_EVENT_INSTANCE_ID * const instance_id)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(instance_id != NULL);
  assert(instance_id->vendor_id != NULL);
  assert(instance_id->event_id != NULL);

  /***************************************************************************/
  /* Free everything.                                                        */
  /***************************************************************************/
  free(instance_id->vendor_id);
  free(instance_id->event_id);
  evel_free_option_string(&instance_id->product_id);
  evel_free_option_string(&instance_id->subsystem_id);
  evel_free_option_string(&instance_id->event_friendly_name);

  EVEL_EXIT();
}

/**************************************************************************//**
 * Encode the instance id as a JSON object according to AT&T's schema.
 *
 * @param jbuf          Pointer to the ::EVEL_JSON_BUFFER to encode into.
 * @param instance_id   Pointer to the ::EVEL_EVENT_INSTANCE_ID to encode.
 *****************************************************************************/
void evel_json_encode_instance_id(EVEL_JSON_BUFFER * jbuf,
                                  EVEL_EVENT_INSTANCE_ID * instance_id)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(jbuf != NULL);
  assert(jbuf->json != NULL);
  assert(jbuf->max_size > 0);
  assert(instance_id != NULL);
  assert(instance_id->vendor_id != NULL);
  assert(instance_id->event_id != NULL);

  evel_json_open_named_object(jbuf, "eventInstanceIdentifier");

  /***************************************************************************/
  /* Mandatory fields.                                                       */
  /***************************************************************************/
  evel_enc_kv_string(jbuf, "vendorId", instance_id->vendor_id);
  evel_enc_kv_string(jbuf, "eventId", instance_id->event_id);

  /***************************************************************************/
  /* Optional fields.                                                        */
  /***************************************************************************/
  evel_enc_kv_opt_string(jbuf, "productId", &instance_id->product_id);
  evel_enc_kv_opt_string(jbuf, "subsystemId", &instance_id->subsystem_id);
  evel_enc_kv_opt_string(
    jbuf, "eventFriendlyName", &instance_id->event_friendly_name);

  evel_json_close_object(jbuf);

  EVEL_EXIT();
}
