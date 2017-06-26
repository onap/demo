/**************************************************************************//**
 * @file
 * Implementation of EVEL functions relating to the Measurement.
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
#include "evel_throttle.h"

/**************************************************************************//**
 * Create a new Measurement event.
 *
 * @note    The mandatory fields on the Measurement must be supplied to this
 *          factory function and are immutable once set.  Optional fields have
 *          explicit setter functions, but again values may only be set once so
 *          that the Measurement has immutable properties.
 *
 * @param   measurement_interval
 *
 * @returns pointer to the newly manufactured ::EVENT_MEASUREMENT.  If the
 *          event is not used (i.e. posted) it must be released using
 *          ::evel_free_event.
 * @retval  NULL  Failed to create the event.
 *****************************************************************************/
EVENT_MEASUREMENT * evel_new_measurement(double measurement_interval)
{
  EVENT_MEASUREMENT * measurement = NULL;

  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(measurement_interval >= 0.0);

  /***************************************************************************/
  /* Allocate the measurement.                                               */
  /***************************************************************************/
  measurement = malloc(sizeof(EVENT_MEASUREMENT));
  if (measurement == NULL)
  {
    log_error_state("Out of memory for Measurement");
    goto exit_label;
  }
  memset(measurement, 0, sizeof(EVENT_MEASUREMENT));
  EVEL_DEBUG("New measurement is at %lp", measurement);

  /***************************************************************************/
  /* Initialize the header & the measurement fields.                         */
  /***************************************************************************/
  evel_init_header(&measurement->header);
  measurement->header.event_domain = EVEL_DOMAIN_MEASUREMENT;
  measurement->measurement_interval = measurement_interval;
  dlist_initialize(&measurement->cpu_usage);
  dlist_initialize(&measurement->filesystem_usage);
  dlist_initialize(&measurement->latency_distribution);
  dlist_initialize(&measurement->vnic_usage);
  dlist_initialize(&measurement->codec_usage);
  dlist_initialize(&measurement->feature_usage);
  dlist_initialize(&measurement->additional_measurements);
  evel_init_option_double(&measurement->aggregate_cpu_usage);
  evel_init_option_double(&measurement->mean_request_latency);
  evel_init_option_double(&measurement->memory_configured);
  evel_init_option_double(&measurement->memory_used);
  evel_init_option_double(&measurement->vnfc_scaling_metric);
  evel_init_option_int(&measurement->concurrent_sessions);
  evel_init_option_int(&measurement->configured_entities);
  evel_init_option_int(&measurement->media_ports_in_use);
  evel_init_option_int(&measurement->request_rate);
  measurement->major_version = EVEL_MEASUREMENT_MAJOR_VERSION;
  measurement->minor_version = EVEL_MEASUREMENT_MINOR_VERSION;

exit_label:
  EVEL_EXIT();
  return measurement;
}

/**************************************************************************//**
 * Set the Event Type property of the Measurement.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param measurement Pointer to the Measurement.
 * @param type        The Event Type to be set. ASCIIZ string. The caller
 *                    does not need to preserve the value once the function
 *                    returns.
 *****************************************************************************/
void evel_measurement_type_set(EVENT_MEASUREMENT * measurement,
                               const char * const type)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions and call evel_header_type_set.                      */
  /***************************************************************************/
  assert(measurement != NULL);
  assert(measurement->header.event_domain == EVEL_DOMAIN_MEASUREMENT);
  evel_header_type_set(&measurement->header, type);

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Concurrent Sessions property of the Measurement.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param measurement         Pointer to the Measurement.
 * @param concurrent_sessions The Concurrent Sessions to be set.
 *****************************************************************************/
void evel_measurement_conc_sess_set(EVENT_MEASUREMENT * measurement,
                                    int concurrent_sessions)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(measurement != NULL);
  assert(measurement->header.event_domain == EVEL_DOMAIN_MEASUREMENT);
  assert(concurrent_sessions >= 0);

  evel_set_option_int(&measurement->concurrent_sessions,
                      concurrent_sessions,
                      "Concurrent Sessions");
  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Configured Entities property of the Measurement.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param measurement         Pointer to the Measurement.
 * @param configured_entities The Configured Entities to be set.
 *****************************************************************************/
void evel_measurement_cfg_ents_set(EVENT_MEASUREMENT * measurement,
                                   int configured_entities)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(measurement != NULL);
  assert(measurement->header.event_domain == EVEL_DOMAIN_MEASUREMENT);
  assert(configured_entities >= 0);

  evel_set_option_int(&measurement->configured_entities,
                      configured_entities,
                      "Configured Entities");
  EVEL_EXIT();
}

/**************************************************************************//**
 * Add an additional set of Errors to the Measurement.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param measurement       Pointer to the measurement.
 * @param receive_discards  The number of receive discards.
 * @param receive_errors    The number of receive errors.
 * @param transmit_discards The number of transmit discards.
 * @param transmit_errors   The number of transmit errors.
 *****************************************************************************/
void evel_measurement_errors_set(EVENT_MEASUREMENT * measurement,
                                 int receive_discards,
                                 int receive_errors,
                                 int transmit_discards,
                                 int transmit_errors)
{
  MEASUREMENT_ERRORS * errors = NULL;
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                      */
  /***************************************************************************/
  assert(measurement != NULL);
  assert(measurement->header.event_domain == EVEL_DOMAIN_MEASUREMENT);
  assert(receive_discards >= 0);
  assert(receive_errors >= 0);
  assert(transmit_discards >= 0);
  assert(transmit_errors >= 0);

  if (measurement->errors == NULL)
  {
    EVEL_DEBUG("Adding Errors: %d, %d; %d, %d",
               receive_discards,
               receive_errors,
               transmit_discards,
               transmit_errors);
    errors = malloc(sizeof(MEASUREMENT_ERRORS));
    assert(errors != NULL);
    memset(errors, 0, sizeof(MEASUREMENT_ERRORS));
    errors->receive_discards = receive_discards;
    errors->receive_errors = receive_errors;
    errors->transmit_discards = transmit_discards;
    errors->transmit_errors = transmit_errors;
    measurement->errors = errors;
  }
  else
  {
    errors = measurement->errors;
    EVEL_DEBUG("Ignoring attempt to add Errors: %d, %d; %d, %d\n"
               "Errors already set: %d, %d; %d, %d",
               receive_discards,
               receive_errors,
               transmit_discards,
               transmit_errors,
               errors->receive_discards,
               errors->receive_errors,
               errors->transmit_discards,
               errors->transmit_errors);
  }

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Mean Request Latency property of the Measurement.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param measurement          Pointer to the Measurement.
 * @param mean_request_latency The Mean Request Latency to be set.
 *****************************************************************************/
void evel_measurement_mean_req_lat_set(EVENT_MEASUREMENT * measurement,
                                       double mean_request_latency)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(measurement != NULL);
  assert(measurement->header.event_domain == EVEL_DOMAIN_MEASUREMENT);
  assert(mean_request_latency >= 0.0);

  evel_set_option_double(&measurement->mean_request_latency,
                         mean_request_latency,
                         "Mean Request Latency");
  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Memory Configured property of the Measurement.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param measurement       Pointer to the Measurement.
 * @param memory_configured The Memory Configured to be set.
 *****************************************************************************/
void evel_measurement_mem_cfg_set(EVENT_MEASUREMENT * measurement,
                                  double memory_configured)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(measurement != NULL);
  assert(measurement->header.event_domain == EVEL_DOMAIN_MEASUREMENT);
  assert(memory_configured >= 0.0);

  evel_set_option_double(&measurement->memory_configured,
                         memory_configured,
                         "Memory Configured");
  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Memory Used property of the Measurement.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param measurement Pointer to the Measurement.
 * @param memory_used The Memory Used to be set.
 *****************************************************************************/
void evel_measurement_mem_used_set(EVENT_MEASUREMENT * measurement,
                                   double memory_used)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(measurement != NULL);
  assert(measurement->header.event_domain == EVEL_DOMAIN_MEASUREMENT);
  assert(memory_used >= 0.0);

  evel_set_option_double(&measurement->memory_used,
                         memory_used,
                         "Memory Used");
  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Request Rate property of the Measurement.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param measurement  Pointer to the Measurement.
 * @param request_rate The Request Rate to be set.
 *****************************************************************************/
void evel_measurement_request_rate_set(EVENT_MEASUREMENT * measurement,
                                       int request_rate)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(measurement != NULL);
  assert(measurement->header.event_domain == EVEL_DOMAIN_MEASUREMENT);
  assert(request_rate >= 0);

  evel_set_option_int(&measurement->request_rate,
                      request_rate,
                      "Request Rate");
  EVEL_EXIT();
}

/**************************************************************************//**
 * Add an additional CPU usage value name/value pair to the Measurement.
 *
 * The name and value are null delimited ASCII strings.  The library takes
 * a copy so the caller does not have to preserve values after the function
 * returns.
 *
 * @param measurement   Pointer to the measurement.
 * @param id            ASCIIZ string with the CPU's identifier.
 * @param usage         CPU utilization.
 *****************************************************************************/
void evel_measurement_cpu_use_add(EVENT_MEASUREMENT * measurement,
                                 char * id, double usage)
{
  MEASUREMENT_CPU_USE * cpu_use = NULL;
  EVEL_ENTER();

  /***************************************************************************/
  /* Check assumptions.                                                      */
  /***************************************************************************/
  assert(measurement != NULL);
  assert(measurement->header.event_domain == EVEL_DOMAIN_MEASUREMENT);
  assert(id != NULL);
  assert(usage >= 0.0);

  /***************************************************************************/
  /* Allocate a container for the value and push onto the list.              */
  /***************************************************************************/
  EVEL_DEBUG("Adding id=%s usage=%lf", id, usage);
  cpu_use = malloc(sizeof(MEASUREMENT_CPU_USE));
  assert(cpu_use != NULL);
  memset(cpu_use, 0, sizeof(MEASUREMENT_CPU_USE));
  cpu_use->id = strdup(id);
  cpu_use->usage = usage;
  assert(cpu_use->id != NULL);

  dlist_push_last(&measurement->cpu_usage, cpu_use);

  EVEL_EXIT();
}

/**************************************************************************//**
 * Add an additional File System usage value name/value pair to the
 * Measurement.
 *
 * The filesystem_name is null delimited ASCII string.  The library takes a
 * copy so the caller does not have to preserve values after the function
 * returns.
 *
 * @param measurement     Pointer to the measurement.
 * @param filesystem_name   ASCIIZ string with the file-system's UUID.
 * @param block_configured  Block storage configured.
 * @param block_used        Block storage in use.
 * @param block_iops        Block storage IOPS.
 * @param ephemeral_configured  Ephemeral storage configured.
 * @param ephemeral_used        Ephemeral storage in use.
 * @param ephemeral_iops        Ephemeral storage IOPS.
 *****************************************************************************/
void evel_measurement_fsys_use_add(EVENT_MEASUREMENT * measurement,
                                   char * filesystem_name,
                                   double block_configured,
                                   double block_used,
                                   int block_iops,
                                   double ephemeral_configured,
                                   double ephemeral_used,
                                   int ephemeral_iops)
{
  MEASUREMENT_FSYS_USE * fsys_use = NULL;
  EVEL_ENTER();

  /***************************************************************************/
  /* Check assumptions.                                                      */
  /***************************************************************************/
  assert(measurement != NULL);
  assert(measurement->header.event_domain == EVEL_DOMAIN_MEASUREMENT);
  assert(filesystem_name != NULL);
  assert(block_configured >= 0.0);
  assert(block_used >= 0.0);
  assert(block_iops >= 0);
  assert(ephemeral_configured >= 0.0);
  assert(ephemeral_used >= 0.0);
  assert(ephemeral_iops >= 0);

  /***************************************************************************/
  /* Allocate a container for the value and push onto the list.              */
  /***************************************************************************/
  EVEL_DEBUG("Adding filesystem_name=%s", filesystem_name);
  fsys_use = malloc(sizeof(MEASUREMENT_FSYS_USE));
  assert(fsys_use != NULL);
  memset(fsys_use, 0, sizeof(MEASUREMENT_FSYS_USE));
  fsys_use->filesystem_name = strdup(filesystem_name);
  fsys_use->block_configured = block_configured;
  fsys_use->block_used = block_used;
  fsys_use->block_iops = block_iops;
  fsys_use->ephemeral_configured = block_configured;
  fsys_use->ephemeral_used = ephemeral_used;
  fsys_use->ephemeral_iops = ephemeral_iops;

  dlist_push_last(&measurement->filesystem_usage, fsys_use);

  EVEL_EXIT();
}

/**************************************************************************//**
 * Add a Feature usage value name/value pair to the Measurement.
 *
 * The name is null delimited ASCII string.  The library takes
 * a copy so the caller does not have to preserve values after the function
 * returns.
 *
 * @param measurement     Pointer to the measurement.
 * @param feature         ASCIIZ string with the feature's name.
 * @param utilization     Utilization of the feature.
 *****************************************************************************/
void evel_measurement_feature_use_add(EVENT_MEASUREMENT * measurement,
                                      char * feature,
                                      int utilization)
{
  MEASUREMENT_FEATURE_USE * feature_use = NULL;
  EVEL_ENTER();

  /***************************************************************************/
  /* Check assumptions.                                                      */
  /***************************************************************************/
  assert(measurement != NULL);
  assert(measurement->header.event_domain == EVEL_DOMAIN_MEASUREMENT);
  assert(feature != NULL);
  assert(utilization >= 0);

  /***************************************************************************/
  /* Allocate a container for the value and push onto the list.              */
  /***************************************************************************/
  EVEL_DEBUG("Adding Feature=%s Use=%d", feature, utilization);
  feature_use = malloc(sizeof(MEASUREMENT_FEATURE_USE));
  assert(feature_use != NULL);
  memset(feature_use, 0, sizeof(MEASUREMENT_FEATURE_USE));
  feature_use->feature_id = strdup(feature);
  assert(feature_use->feature_id != NULL);
  feature_use->feature_utilization = utilization;

  dlist_push_last(&measurement->feature_usage, feature_use);

  EVEL_EXIT();
}

/**************************************************************************//**
 * Add a Additional Measurement value name/value pair to the Report.
 *
 * The name is null delimited ASCII string.  The library takes
 * a copy so the caller does not have to preserve values after the function
 * returns.
 *
 * @param measurement   Pointer to the Measaurement.
 * @param group    ASCIIZ string with the measurement group's name.
 * @param name     ASCIIZ string containing the measurement's name.
 * @param value    ASCIIZ string containing the measurement's value.
 *****************************************************************************/
void evel_measurement_custom_measurement_add(EVENT_MEASUREMENT * measurement,
                                             const char * const group,
                                             const char * const name,
                                             const char * const value)
{
  MEASUREMENT_GROUP * measurement_group = NULL;
  CUSTOM_MEASUREMENT * custom_measurement = NULL;
  DLIST_ITEM * item = NULL;
  EVEL_ENTER();

  /***************************************************************************/
  /* Check assumptions.                                                      */
  /***************************************************************************/
  assert(measurement != NULL);
  assert(measurement->header.event_domain == EVEL_DOMAIN_MEASUREMENT);
  assert(group != NULL);
  assert(name != NULL);
  assert(value != NULL);

  /***************************************************************************/
  /* Allocate a container for the name/value pair.                           */
  /***************************************************************************/
  EVEL_DEBUG("Adding Measurement Group=%s Name=%s Value=%s",
              group, name, value);
  custom_measurement = malloc(sizeof(CUSTOM_MEASUREMENT));
  assert(custom_measurement != NULL);
  memset(custom_measurement, 0, sizeof(CUSTOM_MEASUREMENT));
  custom_measurement->name = strdup(name);
  assert(custom_measurement->name != NULL);
  custom_measurement->value = strdup(value);
  assert(custom_measurement->value != NULL);

  /***************************************************************************/
  /* See if we have that group already.                                      */
  /***************************************************************************/
  item = dlist_get_first(&measurement->additional_measurements);
  while (item != NULL)
  {
    measurement_group = (MEASUREMENT_GROUP *) item->item;
    assert(measurement_group != NULL);

    EVEL_DEBUG("Got measurement group %s", measurement_group->name);
    if (strcmp(group, measurement_group->name) == 0)
    {
      EVEL_DEBUG("Found existing Measurement Group");
      break;
    }
    item = dlist_get_next(item);
  }

  /***************************************************************************/
  /* If we didn't have the group already, create it.                         */
  /***************************************************************************/
  if (item == NULL)
  {
    EVEL_DEBUG("Creating new Measurement Group");
    measurement_group = malloc(sizeof(MEASUREMENT_GROUP));
    assert(measurement_group != NULL);
    memset(measurement_group, 0, sizeof(MEASUREMENT_GROUP));
    measurement_group->name = strdup(group);
    assert(measurement_group->name != NULL);
    dlist_initialize(&measurement_group->measurements);
    dlist_push_last(&measurement->additional_measurements, measurement_group);
  }

  /***************************************************************************/
  /* If we didn't have the group already, create it.                         */
  /***************************************************************************/
  dlist_push_last(&measurement_group->measurements, custom_measurement);

  EVEL_EXIT();
}

/**************************************************************************//**
 * Add a Codec usage value name/value pair to the Measurement.
 *
 * The name is null delimited ASCII string.  The library takes
 * a copy so the caller does not have to preserve values after the function
 * returns.
 *
 * @param measurement     Pointer to the measurement.
 * @param codec           ASCIIZ string with the codec's name.
 * @param utilization     Number of codecs in use.
 *****************************************************************************/
void evel_measurement_codec_use_add(EVENT_MEASUREMENT * measurement,
                                    char * codec,
                                    int utilization)
{
  MEASUREMENT_CODEC_USE * codec_use = NULL;
  EVEL_ENTER();

  /***************************************************************************/
  /* Check assumptions.                                                      */
  /***************************************************************************/
  assert(measurement != NULL);
  assert(measurement->header.event_domain == EVEL_DOMAIN_MEASUREMENT);
  assert(codec != NULL);
  assert(utilization >= 0.0);

  /***************************************************************************/
  /* Allocate a container for the value and push onto the list.              */
  /***************************************************************************/
  EVEL_DEBUG("Adding Codec=%s Use=%d", codec, utilization);
  codec_use = malloc(sizeof(MEASUREMENT_CODEC_USE));
  assert(codec_use != NULL);
  memset(codec_use, 0, sizeof(MEASUREMENT_CODEC_USE));
  codec_use->codec_id = strdup(codec);
  assert(codec_use->codec_id != NULL);
  codec_use->number_in_use = utilization;

  dlist_push_last(&measurement->codec_usage, codec_use);

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Aggregate CPU Use property of the Measurement.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param measurement   Pointer to the measurement.
 * @param cpu_use       The CPU use to set.
 *****************************************************************************/
void evel_measurement_agg_cpu_use_set(EVENT_MEASUREMENT * measurement,
                                      double cpu_use)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(measurement != NULL);
  assert(measurement->header.event_domain == EVEL_DOMAIN_MEASUREMENT);
  assert(cpu_use >= 0.0);

  evel_set_option_double(&measurement->aggregate_cpu_usage,
                         cpu_use,
                         "CPU Use");
  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Media Ports in Use property of the Measurement.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param measurement         Pointer to the measurement.
 * @param media_ports_in_use  The media port usage to set.
 *****************************************************************************/
void evel_measurement_media_port_use_set(EVENT_MEASUREMENT * measurement,
                                         int media_ports_in_use)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(measurement != NULL);
  assert(measurement->header.event_domain == EVEL_DOMAIN_MEASUREMENT);
  assert(media_ports_in_use >= 0);

  evel_set_option_int(&measurement->media_ports_in_use,
                      media_ports_in_use,
                      "Media Ports In Use");
  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the VNFC Scaling Metric property of the Measurement.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param measurement     Pointer to the measurement.
 * @param scaling_metric  The scaling metric to set.
 *****************************************************************************/
void evel_measurement_vnfc_scaling_metric_set(EVENT_MEASUREMENT * measurement,
                                              double scaling_metric)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(measurement != NULL);
  assert(measurement->header.event_domain == EVEL_DOMAIN_MEASUREMENT);
  assert(scaling_metric >= 0.0);

  evel_set_option_double(&measurement->vnfc_scaling_metric,
                         scaling_metric,
                         "VNFC Scaling Metric");
  EVEL_EXIT();
}

/**************************************************************************//**
 * Create a new Latency Bucket to be added to a Measurement event.
 *
 * @note    The mandatory fields on the ::MEASUREMENT_LATENCY_BUCKET must be
 *          supplied to this factory function and are immutable once set.
 *          Optional fields have explicit setter functions, but again values
 *          may only be set once so that the ::MEASUREMENT_LATENCY_BUCKET has
 *          immutable properties.
 *
 * @param count         Count of events in this bucket.
 *
 * @returns pointer to the newly manufactured ::MEASUREMENT_LATENCY_BUCKET.
 *          If the structure is not used it must be released using free.
 * @retval  NULL  Failed to create the Latency Bucket.
 *****************************************************************************/
MEASUREMENT_LATENCY_BUCKET * evel_new_meas_latency_bucket(const int count)
{
  MEASUREMENT_LATENCY_BUCKET * bucket;

  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(count >= 0);

  /***************************************************************************/
  /* Allocate, then set Mandatory Parameters.                                */
  /***************************************************************************/
  EVEL_DEBUG("Creating bucket, count = %d", count);
  bucket = malloc(sizeof(MEASUREMENT_LATENCY_BUCKET));
  assert(bucket != NULL);

  /***************************************************************************/
  /* Set Mandatory Parameters.                                               */
  /***************************************************************************/
  bucket->count = count;

  /***************************************************************************/
  /* Initialize Optional Parameters.                                         */
  /***************************************************************************/
  evel_init_option_double(&bucket->high_end);
  evel_init_option_double(&bucket->low_end);

  EVEL_EXIT();

  return bucket;
}

/**************************************************************************//**
 * Set the High End property of the Measurement Latency Bucket.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param bucket        Pointer to the Measurement Latency Bucket.
 * @param high_end      High end of the bucket's range.
 *****************************************************************************/
void evel_meas_latency_bucket_high_end_set(
                                     MEASUREMENT_LATENCY_BUCKET * const bucket,
                                     const double high_end)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(high_end >= 0.0);
  evel_set_option_double(&bucket->high_end, high_end, "High End");

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Low End property of the Measurement Latency Bucket.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param bucket        Pointer to the Measurement Latency Bucket.
 * @param low_end       Low end of the bucket's range.
 *****************************************************************************/
void evel_meas_latency_bucket_low_end_set(
                                     MEASUREMENT_LATENCY_BUCKET * const bucket,
                                     const double low_end)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(low_end >= 0.0);
  evel_set_option_double(&bucket->low_end, low_end, "Low End");
  EVEL_EXIT();
}

/**************************************************************************//**
 * Add an additional Measurement Latency Bucket to the specified event.
 *
 * @param measurement   Pointer to the Measurement event.
 * @param bucket        Pointer to the Measurement Latency Bucket to add.
 *****************************************************************************/
void evel_meas_latency_bucket_add(EVENT_MEASUREMENT * const measurement,
                                  MEASUREMENT_LATENCY_BUCKET * const bucket)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(measurement != NULL);
  assert(measurement->header.event_domain == EVEL_DOMAIN_MEASUREMENT);
  assert(bucket != NULL);
  dlist_push_last(&measurement->latency_distribution, bucket);

  EVEL_EXIT();
}

/**************************************************************************//**
 * Add an additional Latency Distribution bucket to the Measurement.
 *
 * This function implements the previous API, purely for convenience.
 *
 * @param measurement   Pointer to the measurement.
 * @param low_end       Low end of the bucket's range.
 * @param high_end      High end of the bucket's range.
 * @param count         Count of events in this bucket.
 *****************************************************************************/
void evel_measurement_latency_add(EVENT_MEASUREMENT * const measurement,
                                  const double low_end,
                                  const double high_end,
                                  const int count)
{
  MEASUREMENT_LATENCY_BUCKET * bucket = NULL;

  EVEL_ENTER();

  /***************************************************************************/
  /* Trust the assertions in the underlying methods.                         */
  /***************************************************************************/
  bucket = evel_new_meas_latency_bucket(count);
  evel_meas_latency_bucket_low_end_set(bucket, low_end);
  evel_meas_latency_bucket_high_end_set(bucket, high_end);
  evel_meas_latency_bucket_add(measurement, bucket);

  EVEL_EXIT();
}

/**************************************************************************//**
 * Create a new vNIC Use to be added to a Measurement event.
 *
 * @note    The mandatory fields on the ::MEASUREMENT_VNIC_USE must be supplied
 *          to this factory function and are immutable once set. Optional
 *          fields have explicit setter functions, but again values may only be
 *          set once so that the ::MEASUREMENT_VNIC_USE has immutable
 *          properties.
 *
 * @param vnic_id               ASCIIZ string with the vNIC's ID.
 * @param packets_in            Total packets received.
 * @param packets_out           Total packets transmitted.
 * @param bytes_in              Total bytes received.
 * @param bytes_out             Total bytes transmitted.
 *
 * @returns pointer to the newly manufactured ::MEASUREMENT_VNIC_USE.
 *          If the structure is not used it must be released using
 *          ::evel_free_measurement_vnic_use.
 * @retval  NULL  Failed to create the vNIC Use.
 *****************************************************************************/
MEASUREMENT_VNIC_USE * evel_new_measurement_vnic_use(char * const vnic_id,
                                                     const int packets_in,
                                                     const int packets_out,
                                                     const int bytes_in,
                                                     const int bytes_out)
{
  MEASUREMENT_VNIC_USE * vnic_use;

  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(vnic_id != NULL);
  assert(packets_in >= 0);
  assert(packets_out >= 0);
  assert(bytes_in >= 0);
  assert(bytes_out >= 0);

  /***************************************************************************/
  /* Allocate, then set Mandatory Parameters.                                */
  /***************************************************************************/
  EVEL_DEBUG("Adding VNIC ID=%s", vnic_id);
  vnic_use = malloc(sizeof(MEASUREMENT_VNIC_USE));
  assert(vnic_use != NULL);
  vnic_use->vnic_id = strdup(vnic_id);
  vnic_use->packets_in = packets_in;
  vnic_use->packets_out = packets_out;
  vnic_use->bytes_in = bytes_in;
  vnic_use->bytes_out = bytes_out;

  /***************************************************************************/
  /* Initialize Optional Parameters.                                         */
  /***************************************************************************/
  evel_init_option_int(&vnic_use->broadcast_packets_in);
  evel_init_option_int(&vnic_use->broadcast_packets_out);
  evel_init_option_int(&vnic_use->multicast_packets_in);
  evel_init_option_int(&vnic_use->multicast_packets_out);
  evel_init_option_int(&vnic_use->unicast_packets_in);
  evel_init_option_int(&vnic_use->unicast_packets_out);

  EVEL_EXIT();

  return vnic_use;
}

/**************************************************************************//**
 * Free a vNIC Use.
 *
 * Free off the ::MEASUREMENT_VNIC_USE supplied.  Will free all the contained
 * allocated memory.
 *
 * @note It does not free the vNIC Use itself, since that may be part of a
 * larger structure.
 *****************************************************************************/
void evel_free_measurement_vnic_use(MEASUREMENT_VNIC_USE * const vnic_use)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(vnic_use != NULL);
  assert(vnic_use->vnic_id != NULL);

  /***************************************************************************/
  /* Free the duplicated string.                                             */
  /***************************************************************************/
  free(vnic_use->vnic_id);
  vnic_use->vnic_id = NULL;

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Broadcast Packets Received property of the vNIC Use.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param vnic_use      Pointer to the vNIC Use.
 * @param broadcast_packets_in
 *                      Broadcast packets received.
 *****************************************************************************/
void evel_vnic_use_bcast_pkt_in_set(MEASUREMENT_VNIC_USE * const vnic_use,
                                    const int broadcast_packets_in)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(broadcast_packets_in >= 0);

  evel_set_option_int(&vnic_use->broadcast_packets_in,
                      broadcast_packets_in,
                      "Broadcast Packets Received");

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Broadcast Packets Transmitted property of the vNIC Use.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param vnic_use      Pointer to the vNIC Use.
 * @param broadcast_packets_out
 *                      Broadcast packets transmitted.
 *****************************************************************************/
void evel_vnic_use_bcast_pkt_out_set(MEASUREMENT_VNIC_USE * const vnic_use,
                                     const int broadcast_packets_out)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(broadcast_packets_out >= 0);

  evel_set_option_int(&vnic_use->broadcast_packets_out,
                      broadcast_packets_out,
                      "Broadcast Packets Transmitted");

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Multicast Packets Received property of the vNIC Use.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param vnic_use      Pointer to the vNIC Use.
 * @param multicast_packets_in
 *                      Multicast packets received.
 *****************************************************************************/
void evel_vnic_use_mcast_pkt_in_set(MEASUREMENT_VNIC_USE * const vnic_use,
                                    const int multicast_packets_in)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(multicast_packets_in >= 0);

  evel_set_option_int(&vnic_use->multicast_packets_in,
                      multicast_packets_in,
                      "Multicast Packets Received");

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Multicast Packets Transmitted property of the vNIC Use.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param vnic_use      Pointer to the vNIC Use.
 * @param multicast_packets_out
 *                      Multicast packets transmitted.
 *****************************************************************************/
void evel_vnic_use_mcast_pkt_out_set(MEASUREMENT_VNIC_USE * const vnic_use,
                                     const int multicast_packets_out)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(multicast_packets_out >= 0);

  evel_set_option_int(&vnic_use->multicast_packets_out,
                      multicast_packets_out,
                      "Multicast Packets Transmitted");

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Unicast Packets Received property of the vNIC Use.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param vnic_use      Pointer to the vNIC Use.
 * @param unicast_packets_in
 *                      Unicast packets received.
 *****************************************************************************/
void evel_vnic_use_ucast_pkt_in_set(MEASUREMENT_VNIC_USE * const vnic_use,
                                    const int unicast_packets_in)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(unicast_packets_in >= 0);

  evel_set_option_int(&vnic_use->unicast_packets_in,
                      unicast_packets_in,
                      "Unicast Packets Received");

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Unicast Packets Transmitted property of the vNIC Use.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param vnic_use      Pointer to the vNIC Use.
 * @param unicast_packets_out
 *                      Unicast packets transmitted.
 *****************************************************************************/
void evel_vnic_use_ucast_pkt_out_set(MEASUREMENT_VNIC_USE * const vnic_use,
                                     const int unicast_packets_out)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(unicast_packets_out >= 0);

  evel_set_option_int(&vnic_use->unicast_packets_out,
                      unicast_packets_out,
                      "Unicast Packets Transmitted");

  EVEL_EXIT();
}

/**************************************************************************//**
 * Add an additional vNIC Use to the specified Measurement event.
 *
 * @param measurement   Pointer to the measurement.
 * @param vnic_use      Pointer to the vNIC Use to add.
 *****************************************************************************/
void evel_meas_vnic_use_add(EVENT_MEASUREMENT * const measurement,
                            MEASUREMENT_VNIC_USE * const vnic_use)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(measurement != NULL);
  assert(measurement->header.event_domain == EVEL_DOMAIN_MEASUREMENT);
  assert(vnic_use != NULL);

  dlist_push_last(&measurement->vnic_usage, vnic_use);

  EVEL_EXIT();
}

/**************************************************************************//**
 * Add an additional vNIC usage record Measurement.
 *
 * This function implements the previous API, purely for convenience.
 *
 * The ID is null delimited ASCII string.  The library takes a copy so the
 * caller does not have to preserve values after the function returns.
 *
 * @param measurement           Pointer to the measurement.
 * @param vnic_id               ASCIIZ string with the vNIC's ID.
 * @param packets_in            Total packets received.
 * @param packets_out           Total packets transmitted.
 * @param broadcast_packets_in  Broadcast packets received.
 * @param broadcast_packets_out Broadcast packets transmitted.
 * @param bytes_in              Total bytes received.
 * @param bytes_out             Total bytes transmitted.
 * @param multicast_packets_in  Multicast packets received.
 * @param multicast_packets_out Multicast packets transmitted.
 * @param unicast_packets_in    Unicast packets received.
 * @param unicast_packets_out   Unicast packets transmitted.
 *****************************************************************************/
void evel_measurement_vnic_use_add(EVENT_MEASUREMENT * const measurement,
                                   char * const vnic_id,
                                   const int packets_in,
                                   const int packets_out,
                                   const int broadcast_packets_in,
                                   const int broadcast_packets_out,
                                   const int bytes_in,
                                   const int bytes_out,
                                   const int multicast_packets_in,
                                   const int multicast_packets_out,
                                   const int unicast_packets_in,
                                   const int unicast_packets_out)
{
  MEASUREMENT_VNIC_USE * vnic_use = NULL;
  EVEL_ENTER();

  /***************************************************************************/
  /* Trust the assertions in the underlying methods.                         */
  /***************************************************************************/
  vnic_use = evel_new_measurement_vnic_use(vnic_id,
                                           packets_in,
                                           packets_out,
                                           bytes_in,
                                           bytes_out);
  evel_vnic_use_bcast_pkt_in_set(vnic_use, broadcast_packets_in);
  evel_vnic_use_bcast_pkt_out_set(vnic_use, broadcast_packets_out);
  evel_vnic_use_mcast_pkt_in_set(vnic_use, multicast_packets_in);
  evel_vnic_use_mcast_pkt_out_set(vnic_use, multicast_packets_out);
  evel_vnic_use_ucast_pkt_in_set(vnic_use, unicast_packets_in);
  evel_vnic_use_ucast_pkt_out_set(vnic_use, unicast_packets_out);
  evel_meas_vnic_use_add(measurement, vnic_use);
}

/**************************************************************************//**
 * Encode the measurement as a JSON measurement.
 *
 * @param jbuf          Pointer to the ::EVEL_JSON_BUFFER to encode into.
 * @param event         Pointer to the ::EVENT_HEADER to encode.
 *****************************************************************************/
void evel_json_encode_measurement(EVEL_JSON_BUFFER * jbuf,
                                  EVENT_MEASUREMENT * event)
{
  MEASUREMENT_CPU_USE * cpu_use = NULL;
  MEASUREMENT_FSYS_USE * fsys_use = NULL;
  MEASUREMENT_LATENCY_BUCKET * bucket = NULL;
  MEASUREMENT_VNIC_USE * vnic_use = NULL;
  MEASUREMENT_ERRORS * errors = NULL;
  MEASUREMENT_FEATURE_USE * feature_use = NULL;
  MEASUREMENT_CODEC_USE * codec_use = NULL;
  MEASUREMENT_GROUP * measurement_group = NULL;
  CUSTOM_MEASUREMENT * custom_measurement = NULL;
  DLIST_ITEM * item = NULL;
  DLIST_ITEM * nested_item = NULL;

  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(event != NULL);
  assert(event->header.event_domain == EVEL_DOMAIN_MEASUREMENT);

  evel_json_encode_header(jbuf, &event->header);
  evel_json_open_named_object(jbuf, "measurementsForVfScalingFields");

  /***************************************************************************/
  /* Mandatory fields.                                                       */
  /***************************************************************************/
  evel_enc_kv_double(jbuf, "measurementInterval", event->measurement_interval);

  /***************************************************************************/
  /* Optional fields.                                                        */
  /***************************************************************************/
  evel_enc_kv_opt_int(jbuf, "concurrentSessions", &event->concurrent_sessions);
  evel_enc_kv_opt_int(jbuf, "configuredEntities", &event->configured_entities);

  /***************************************************************************/
  /* CPU Use list.                                                           */
  /***************************************************************************/
  evel_json_checkpoint(jbuf);
  if (evel_json_open_opt_named_list(jbuf, "cpuUsageArray"))
  {
    bool item_added = false;

    item = dlist_get_first(&event->cpu_usage);
    while (item != NULL)
    {
      cpu_use = (MEASUREMENT_CPU_USE*) item->item;
      assert(cpu_use != NULL);

      if (!evel_throttle_suppress_nv_pair(jbuf->throttle_spec,
                                          "cpuUsageArray",
                                          cpu_use->id))
      {
        evel_json_open_object(jbuf);
        evel_enc_kv_string(jbuf, "cpuIdentifier", cpu_use->id);
        evel_enc_kv_double(jbuf, "percentUsage", cpu_use->usage);
        evel_json_close_object(jbuf);
        item_added = true;
      }
      item = dlist_get_next(item);
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

  /***************************************************************************/
  /* Filesystem Usage list.                                                  */
  /***************************************************************************/
  evel_json_checkpoint(jbuf);
  if (evel_json_open_opt_named_list(jbuf, "filesystemUsageArray"))
  {
    bool item_added = false;

    item = dlist_get_first(&event->filesystem_usage);
    while (item != NULL)
    {
      fsys_use = (MEASUREMENT_FSYS_USE *) item->item;
      assert(fsys_use != NULL);

      if (!evel_throttle_suppress_nv_pair(jbuf->throttle_spec,
                                          "filesystemUsageArray",
                                          fsys_use->filesystem_name))
      {
        evel_json_open_object(jbuf);
        evel_enc_kv_double(
          jbuf, "blockConfigured", fsys_use->block_configured);
        evel_enc_kv_int(jbuf, "blockIops", fsys_use->block_iops);
        evel_enc_kv_double(jbuf, "blockUsed", fsys_use->block_used);
        evel_enc_kv_double(
          jbuf, "ephemeralConfigured", fsys_use->ephemeral_configured);
        evel_enc_kv_int(jbuf, "ephemeralIops", fsys_use->ephemeral_iops);
        evel_enc_kv_double(jbuf, "ephemeralUsed", fsys_use->ephemeral_used);
        evel_enc_kv_string(jbuf, "filesystemName", fsys_use->filesystem_name);
        evel_json_close_object(jbuf);
        item_added = true;
      }
      item = dlist_get_next(item);
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

  /***************************************************************************/
  /* Latency distribution.                                                   */
  /***************************************************************************/
  item = dlist_get_first(&event->latency_distribution);
  if ((item != NULL) &&
      evel_json_open_opt_named_list(jbuf, "latencyDistribution"))
  {
    while (item != NULL)
    {
      bucket = (MEASUREMENT_LATENCY_BUCKET*) item->item;
      assert(bucket != NULL);

      evel_json_open_object(jbuf);
      evel_enc_kv_opt_double(
        jbuf, "lowEndOfLatencyBucket", &bucket->low_end);
      evel_enc_kv_opt_double(
        jbuf, "highEndOfLatencyBucket", &bucket->high_end);
      evel_enc_kv_int(jbuf, "countsInTheBucket", bucket->count);
      evel_json_close_object(jbuf);
      item = dlist_get_next(item);
    }
    evel_json_close_list(jbuf);
  }

  evel_enc_kv_opt_double(
    jbuf, "meanRequestLatency", &event->mean_request_latency);
  evel_enc_kv_opt_double(jbuf, "memoryConfigured", &event->memory_configured);
  evel_enc_kv_opt_double(jbuf, "memoryUsed", &event->memory_used);
  evel_enc_kv_opt_int(jbuf, "requestRate", &event->request_rate);

  /***************************************************************************/
  /* vNIC Usage                                                              */
  /***************************************************************************/
  evel_json_checkpoint(jbuf);
  if (evel_json_open_opt_named_list(jbuf, "vNicUsageArray"))
  {
    bool item_added = false;

    item = dlist_get_first(&event->vnic_usage);
    while (item != NULL)
    {
      vnic_use = (MEASUREMENT_VNIC_USE *) item->item;
      assert(vnic_use != NULL);

      if (!evel_throttle_suppress_nv_pair(jbuf->throttle_spec,
                                          "vNicUsageArray",
                                          vnic_use->vnic_id))
      {
        evel_json_open_object(jbuf);

        /*********************************************************************/
        /* Mandatory fields.                                                 */
        /*********************************************************************/
        evel_enc_kv_int(jbuf, "bytesIn", vnic_use->bytes_in);
        evel_enc_kv_int(jbuf, "bytesOut", vnic_use->bytes_out);
        evel_enc_kv_int(jbuf, "packetsIn", vnic_use->packets_in);
        evel_enc_kv_int(jbuf, "packetsOut", vnic_use->packets_out);
        evel_enc_kv_string(jbuf, "vNicIdentifier", vnic_use->vnic_id);

        /*********************************************************************/
        /* Optional fields.                                                  */
        /*********************************************************************/
        evel_enc_kv_opt_int(
          jbuf, "broadcastPacketsIn", &vnic_use->broadcast_packets_in);
        evel_enc_kv_opt_int(
          jbuf, "broadcastPacketsOut", &vnic_use->broadcast_packets_out);
        evel_enc_kv_opt_int(
          jbuf, "multicastPacketsIn", &vnic_use->multicast_packets_in);
        evel_enc_kv_opt_int(
          jbuf, "multicastPacketsOut", &vnic_use->multicast_packets_out);
        evel_enc_kv_opt_int(
          jbuf, "unicastPacketsIn", &vnic_use->unicast_packets_in);
        evel_enc_kv_opt_int(
          jbuf, "unicastPacketsOut", &vnic_use->unicast_packets_out);

        evel_json_close_object(jbuf);
        item_added = true;
      }
      item = dlist_get_next(item);
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

  evel_enc_kv_opt_double(
    jbuf, "aggregateCpuUsage", &event->aggregate_cpu_usage);
  evel_enc_kv_opt_int(
    jbuf, "numberOfMediaPortsInUse", &event->media_ports_in_use);
  evel_enc_kv_opt_double(
    jbuf, "vnfcScalingMetric", &event->vnfc_scaling_metric);

  /***************************************************************************/
  /* Errors list.                                                            */
  /***************************************************************************/
  if ((event->errors != NULL) &&
      evel_json_open_opt_named_object(jbuf, "errors"))
  {
    errors = event->errors;
    evel_enc_kv_int(jbuf, "receiveDiscards", errors->receive_discards);
    evel_enc_kv_int(jbuf, "receiveErrors", errors->receive_errors);
    evel_enc_kv_int(jbuf, "transmitDiscards", errors->transmit_discards);
    evel_enc_kv_int(jbuf, "transmitErrors", errors->transmit_errors);
    evel_json_close_object(jbuf);
  }

  /***************************************************************************/
  /* Feature Utilization list.                                               */
  /***************************************************************************/
  evel_json_checkpoint(jbuf);
  if (evel_json_open_opt_named_list(jbuf, "featureUsageArray"))
  {
    bool item_added = false;

    item = dlist_get_first(&event->feature_usage);
    while (item != NULL)
    {
      feature_use = (MEASUREMENT_FEATURE_USE*) item->item;
      assert(feature_use != NULL);

      if (!evel_throttle_suppress_nv_pair(jbuf->throttle_spec,
                                          "featureUsageArray",
                                          feature_use->feature_id))
      {
        evel_json_open_object(jbuf);
        evel_enc_kv_string(jbuf, "featureIdentifier", feature_use->feature_id);
        evel_enc_kv_int(
          jbuf, "featureUtilization", feature_use->feature_utilization);
        evel_json_close_object(jbuf);
        item_added = true;
      }
      item = dlist_get_next(item);
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

  /***************************************************************************/
  /* Codec Utilization list.                                                 */
  /***************************************************************************/
  evel_json_checkpoint(jbuf);
  if (evel_json_open_opt_named_list(jbuf, "codecUsageArray"))
  {
    bool item_added = false;

    item = dlist_get_first(&event->codec_usage);
    while (item != NULL)
    {
      codec_use = (MEASUREMENT_CODEC_USE*) item->item;
      assert(codec_use != NULL);

      if (!evel_throttle_suppress_nv_pair(jbuf->throttle_spec,
                                          "codecUsageArray",
                                          codec_use->codec_id))
      {
        evel_json_open_object(jbuf);
        evel_enc_kv_string(jbuf, "codecIdentifier", codec_use->codec_id);
        evel_enc_kv_int(jbuf, "numberInUse", codec_use->number_in_use);
        evel_json_close_object(jbuf);
        item_added = true;
      }
      item = dlist_get_next(item);
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

  /***************************************************************************/
  /* Additional Measurement Groups list.                                     */
  /***************************************************************************/
  evel_json_checkpoint(jbuf);
  if (evel_json_open_opt_named_list(jbuf, "additionalMeasurements"))
  {
    bool item_added = false;

    item = dlist_get_first(&event->additional_measurements);
    while (item != NULL)
    {
      measurement_group = (MEASUREMENT_GROUP *) item->item;
      assert(measurement_group != NULL);

      if (!evel_throttle_suppress_nv_pair(jbuf->throttle_spec,
                                          "additionalMeasurements",
                                          measurement_group->name))
      {
        evel_json_open_object(jbuf);
        evel_enc_kv_string(jbuf, "name", measurement_group->name);
        evel_json_open_opt_named_list(jbuf, "measurements");

        /*********************************************************************/
        /* Measurements list.                                                */
        /*********************************************************************/
        nested_item = dlist_get_first(&measurement_group->measurements);
        while (nested_item != NULL)
        {
          custom_measurement = (CUSTOM_MEASUREMENT *) nested_item->item;
          assert(custom_measurement != NULL);

          evel_json_open_object(jbuf);
          evel_enc_kv_string(jbuf, "name", custom_measurement->name);
          evel_enc_kv_string(jbuf, "value", custom_measurement->value);
          evel_json_close_object(jbuf);
          nested_item = dlist_get_next(nested_item);
        }
        evel_json_close_list(jbuf);
        evel_json_close_object(jbuf);
        item_added = true;
      }
      item = dlist_get_next(item);
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

  /***************************************************************************/
  /* Although optional, we always generate the version.  Note that this      */
  /* closes the object, too.                                                 */
  /***************************************************************************/
  evel_enc_version(jbuf,
                   "measurementsForVfScalingVersion",
                   event->major_version,
                   event->major_version);
  evel_json_close_object(jbuf);

  EVEL_EXIT();
}

/**************************************************************************//**
 * Free a Measurement.
 *
 * Free off the Measurement supplied.  Will free all the contained allocated
 * memory.
 *
 * @note It does not free the Measurement itself, since that may be part of a
 * larger structure.
 *****************************************************************************/
void evel_free_measurement(EVENT_MEASUREMENT * event)
{
  MEASUREMENT_CPU_USE * cpu_use = NULL;
  MEASUREMENT_FSYS_USE * fsys_use = NULL;
  MEASUREMENT_LATENCY_BUCKET * bucket = NULL;
  MEASUREMENT_VNIC_USE * vnic_use = NULL;
  MEASUREMENT_FEATURE_USE * feature_use = NULL;
  MEASUREMENT_CODEC_USE * codec_use = NULL;
  MEASUREMENT_GROUP * measurement_group = NULL;
  CUSTOM_MEASUREMENT * measurement = NULL;

  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.  As an internal API we don't allow freeing NULL    */
  /* events as we do on the public API.                                      */
  /***************************************************************************/
  assert(event != NULL);
  assert(event->header.event_domain == EVEL_DOMAIN_MEASUREMENT);

  /***************************************************************************/
  /* Free all internal strings then the header itself.                       */
  /***************************************************************************/
  cpu_use = dlist_pop_last(&event->cpu_usage);
  while (cpu_use != NULL)
  {
    EVEL_DEBUG("Freeing CPU use Info (%s)", cpu_use->id);
    free(cpu_use->id);
    free(cpu_use);
    cpu_use = dlist_pop_last(&event->cpu_usage);
  }

  fsys_use = dlist_pop_last(&event->filesystem_usage);
  while (fsys_use != NULL)
  {
    EVEL_DEBUG("Freeing Filesystem Use info (%s)", fsys_use->filesystem_name);
    free(fsys_use->filesystem_name);
    free(fsys_use);
    fsys_use = dlist_pop_last(&event->filesystem_usage);
  }

  bucket = dlist_pop_last(&event->latency_distribution);
  while (bucket != NULL)
  {
    EVEL_DEBUG("Freeing Latency Bucket");
    free(bucket);
    bucket = dlist_pop_last(&event->latency_distribution);
  }

  vnic_use = dlist_pop_last(&event->vnic_usage);
  while (vnic_use != NULL)
  {
    EVEL_DEBUG("Freeing vNIC use Info (%s)", vnic_use->vnic_id);
    evel_free_measurement_vnic_use(vnic_use);
    free(vnic_use);
    vnic_use = dlist_pop_last(&event->vnic_usage);
  }

  codec_use = dlist_pop_last(&event->codec_usage);
  while (codec_use != NULL)
  {
    EVEL_DEBUG("Freeing Codec use Info (%s)", codec_use->codec_id);
    free(codec_use->codec_id);
    free(codec_use);
    codec_use = dlist_pop_last(&event->codec_usage);
  }

  if (event->errors != NULL)
  {
    EVEL_DEBUG("Freeing Errors");
    free(event->errors);
  }

  feature_use = dlist_pop_last(&event->feature_usage);
  while (feature_use != NULL)
  {
    EVEL_DEBUG("Freeing Feature use Info (%s)", feature_use->feature_id);
    free(feature_use->feature_id);
    free(feature_use);
    feature_use = dlist_pop_last(&event->feature_usage);
  }

  measurement_group = dlist_pop_last(&event->additional_measurements);
  while (measurement_group != NULL)
  {
    EVEL_DEBUG("Freeing Measurement Group (%s)", measurement_group->name);

    measurement = dlist_pop_last(&measurement_group->measurements);
    while (measurement != NULL)
    {
      EVEL_DEBUG("Freeing Measurement (%s)", measurement->name);
      free(measurement->name);
      free(measurement->value);
      free(measurement);
      measurement = dlist_pop_last(&measurement_group->measurements);
    }
    free(measurement_group->name);
    free(measurement_group);
    measurement_group = dlist_pop_last(&event->additional_measurements);
  }

  evel_free_header(&event->header);

  EVEL_EXIT();
}
