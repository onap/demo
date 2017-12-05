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
 * Implementation of EVEL functions relating to the Measurement for VF
 * Reporting event.
 *
 * @note  This is an experimental event tytpe and does not form part of the
 *        currently approved AT&T event schema.  It is intended to allow a
 *        less-onerous event reporting mechanism because it avoids having to
 *        return all the platform statistics which are mandatory in the
 *        **measurementsForVfScaling** event.
 ****************************************************************************/

#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include "evel.h"
#include "evel_internal.h"
#include "evel_throttle.h"

/**************************************************************************//**
 * Create a new Report event.
 *
 * @note    The mandatory fields on the Report must be supplied to this
 *          factory function and are immutable once set.  Optional fields have
 *          explicit setter functions, but again values may only be set once so
 *          that the Report has immutable properties.
 *
 * @param   measurement_interval
 * @param event_name    Unique Event Name
 * @param event_id    A universal identifier of the event for analysis etc
 * @returns pointer to the newly manufactured ::EVENT_REPORT.  If the event is
 *          not used (i.e. posted) it must be released using ::evel_free_event.
 * @retval  NULL  Failed to create the event.
 *****************************************************************************/
EVENT_REPORT * evel_new_report(double measurement_interval,const char *ev_name, const char *ev_id)
{
  EVENT_REPORT * report = NULL;

  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(measurement_interval >= 0.0);

  /***************************************************************************/
  /* Allocate the report.                                                    */
  /***************************************************************************/
  report = malloc(sizeof(EVENT_REPORT));
  if (report == NULL)
  {
    log_error_state("Out of memory for Report");
    goto exit_label;
  }
  memset(report, 0, sizeof(EVENT_REPORT));
  EVEL_DEBUG("New report is at %lp", report);

  /***************************************************************************/
  /* Initialize the header & the report fields.                              */
  /***************************************************************************/
  evel_init_header_nameid(&report->header,ev_name,ev_id);
  report->header.event_domain = EVEL_DOMAIN_REPORT;
  report->measurement_interval = measurement_interval;

  dlist_initialize(&report->feature_usage);
  dlist_initialize(&report->measurement_groups);
  report->major_version = EVEL_REPORT_MAJOR_VERSION;
  report->minor_version = EVEL_REPORT_MINOR_VERSION;

exit_label:
  EVEL_EXIT();
  return report;
}

/**************************************************************************//**
 * Set the Event Type property of the Report.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param report Pointer to the Report.
 * @param type        The Event Type to be set. ASCIIZ string. The caller
 *                    does not need to preserve the value once the function
 *                    returns.
 *****************************************************************************/
void evel_report_type_set(EVENT_REPORT * report,
                          const char * const type)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions and call evel_header_type_set.                      */
  /***************************************************************************/
  assert(report != NULL);
  assert(report->header.event_domain == EVEL_DOMAIN_REPORT);
  evel_header_type_set(&report->header, type);

  EVEL_EXIT();
}

/**************************************************************************//**
 * Add a Feature usage value name/value pair to the Report.
 *
 * The name is null delimited ASCII string.  The library takes
 * a copy so the caller does not have to preserve values after the function
 * returns.
 *
 * @param report          Pointer to the report.
 * @param feature         ASCIIZ string with the feature's name.
 * @param utilization     Utilization of the feature.
 *****************************************************************************/
void evel_report_feature_use_add(EVENT_REPORT * report,
                                 char * feature,
                                 int utilization)
{
  MEASUREMENT_FEATURE_USE * feature_use = NULL;
  EVEL_ENTER();

  /***************************************************************************/
  /* Check assumptions.                                                      */
  /***************************************************************************/
  assert(report != NULL);
  assert(report->header.event_domain == EVEL_DOMAIN_REPORT);
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

  dlist_push_last(&report->feature_usage, feature_use);

  EVEL_EXIT();
}

/**************************************************************************//**
 * Add a Additional Measurement value name/value pair to the Report.
 *
 * The name is null delimited ASCII string.  The library takes
 * a copy so the caller does not have to preserve values after the function
 * returns.
 *
 * @param report   Pointer to the report.
 * @param group    ASCIIZ string with the measurement group's name.
 * @param name     ASCIIZ string containing the measurement's name.
 * @param value    ASCIIZ string containing the measurement's value.
 *****************************************************************************/
void evel_report_custom_measurement_add(EVENT_REPORT * report,
                                        const char * const group,
                                        const char * const name,
                                        const char * const value)
{
  MEASUREMENT_GROUP * measurement_group = NULL;
  CUSTOM_MEASUREMENT * measurement = NULL;
  DLIST_ITEM * item = NULL;
  EVEL_ENTER();

  /***************************************************************************/
  /* Check assumptions.                                                      */
  /***************************************************************************/
  assert(report != NULL);
  assert(report->header.event_domain == EVEL_DOMAIN_REPORT);
  assert(group != NULL);
  assert(name != NULL);
  assert(value != NULL);

  /***************************************************************************/
  /* Allocate a container for the name/value pair.                           */
  /***************************************************************************/
  EVEL_DEBUG("Adding Measurement Group=%s Name=%s Value=%s",
              group, name, value);
  measurement = malloc(sizeof(CUSTOM_MEASUREMENT));
  assert(measurement != NULL);
  memset(measurement, 0, sizeof(CUSTOM_MEASUREMENT));
  measurement->name = strdup(name);
  assert(measurement->name != NULL);
  measurement->value = strdup(value);
  assert(measurement->value != NULL);

  /***************************************************************************/
  /* See if we have that group already.                                      */
  /***************************************************************************/
  item = dlist_get_first(&report->measurement_groups);
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
    dlist_push_last(&report->measurement_groups, measurement_group);
  }

  /***************************************************************************/
  /* If we didn't have the group already, create it.                         */
  /***************************************************************************/
  dlist_push_last(&measurement_group->measurements, measurement);

  EVEL_EXIT();
}

/**************************************************************************//**
 * Encode the report as a JSON report.
 *
 * @param jbuf          Pointer to the ::EVEL_JSON_BUFFER to encode into.
 * @param event         Pointer to the ::EVENT_HEADER to encode.
 *****************************************************************************/
void evel_json_encode_report(EVEL_JSON_BUFFER * jbuf,
                             EVENT_REPORT * event)
{
  MEASUREMENT_FEATURE_USE * feature_use = NULL;
  MEASUREMENT_GROUP * measurement_group = NULL;
  CUSTOM_MEASUREMENT * custom_measurement = NULL;
  DLIST_ITEM * item = NULL;
  DLIST_ITEM * nested_item = NULL;

  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(event != NULL);
  assert(event->header.event_domain == EVEL_DOMAIN_REPORT);

  evel_json_encode_header(jbuf, &event->header);
  evel_json_open_named_object(jbuf, "measurementsForVfReportingFields");
  evel_enc_kv_double(jbuf, "measurementInterval", event->measurement_interval);

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
  /* Additional Measurement Groups list.                                     */
  /***************************************************************************/
  evel_json_checkpoint(jbuf);
  if (evel_json_open_opt_named_list(jbuf, "additionalMeasurements"))
  {
    bool item_added = false;

    item = dlist_get_first(&event->measurement_groups);
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
        evel_json_open_named_list(jbuf, "arrayOfFields");

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
                   "measurementFieldsVersion",
                   event->major_version,
                   event->minor_version);
  evel_json_close_object(jbuf);

  EVEL_EXIT();
}

/**************************************************************************//**
 * Free a Report.
 *
 * Free off the Report supplied.  Will free all the contained allocated memory.
 *
 * @note It does not free the Report itself, since that may be part of a
 * larger structure.
 *****************************************************************************/
void evel_free_report(EVENT_REPORT * event)
{
  MEASUREMENT_FEATURE_USE * feature_use = NULL;
  MEASUREMENT_GROUP * measurement_group = NULL;
  CUSTOM_MEASUREMENT * custom_measurement = NULL;

  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.  As an internal API we don't allow freeing NULL    */
  /* events as we do on the public API.                                      */
  /***************************************************************************/
  assert(event != NULL);
  assert(event->header.event_domain == EVEL_DOMAIN_REPORT);

  /***************************************************************************/
  /* Free all internal strings then the header itself.                       */
  /***************************************************************************/
  feature_use = dlist_pop_last(&event->feature_usage);
  while (feature_use != NULL)
  {
    EVEL_DEBUG("Freeing Feature use Info (%s)", feature_use->feature_id);
    free(feature_use->feature_id);
    free(feature_use);
    feature_use = dlist_pop_last(&event->feature_usage);
  }
  measurement_group = dlist_pop_last(&event->measurement_groups);
  while (measurement_group != NULL)
  {
    EVEL_DEBUG("Freeing Measurement Group (%s)", measurement_group->name);

    custom_measurement = dlist_pop_last(&measurement_group->measurements);
    while (custom_measurement != NULL)
    {
      EVEL_DEBUG("Freeing mesaurement (%s)", custom_measurement->name);

      free(custom_measurement->name);
      free(custom_measurement->value);
      free(custom_measurement);
      custom_measurement = dlist_pop_last(&measurement_group->measurements);
    }

    free(measurement_group->name);
    free(measurement_group);
    measurement_group = dlist_pop_last(&event->measurement_groups);
  }

  evel_free_header(&event->header);

  EVEL_EXIT();
}
