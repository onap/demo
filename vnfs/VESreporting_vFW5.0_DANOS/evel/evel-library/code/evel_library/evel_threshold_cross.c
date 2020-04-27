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
 * Implementation of EVEL functions relating to the Threshold Cross Alerts.
 *
 *****************************************************************************/
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include "evel.h"	
#include "evel_internal.h"	
#include "evel_throttle.h"	


/**************************************************************************//**
 * Create a new Threshold Crossing Alert event.
 *
 * @note    The mandatory fields on the TCA must be supplied to this factory
 *          function and are immutable once set.  Optional fields have explicit
 *          setter functions, but again values may only be set once so that the
 *          TCA has immutable properties.
 *
 * @param event_name  Unique Event Name confirming Domain AsdcVnfModel Description
 * @param event_id    A universal identifier of the event for: troubleshooting correlation, analysis, etc
 * @param char* tcriticality   Performance Counter Criticality MAJ MIN,
 * @param char* tname          Performance Counter Threshold name
 * @param char* tthresholdCrossed  Counter Threshold crossed value
 * @param char* tvalue             Counter actual value
 * @param EVEL_EVENT_ACTION talertAction   Alert set continue or clear
 * @param char*  talertDescription
 * @param EVEL_ALERT_TYPE     talertType    Kind of anamoly
 * @param unsigned long long  tcollectionTimestamp time at which alert was collected
 * @param EVEL_SEVERITIES     teventSeverity  Severity of Alert
 * @param unsigned long long  teventStartTimestamp Time when this alert started
 *
 * @returns pointer to the newly manufactured ::EVENT_THRESHOLD_CROSS.  If the
 *          event is not used it must be released using
 *          ::evel_free_threshold_cross
 * @retval  NULL  Failed to create the event.
 *****************************************************************************/
EVENT_THRESHOLD_CROSS * evel_new_threshold_cross(const char * ev_name, const char * ev_id,
			   char *  tcriticality,
                           char *  tname,
                           char *  tthresholdCrossed,
                           char *  tvalue,
                           EVEL_EVENT_ACTION  talertAction,
                           char *             talertDescription, 
                           EVEL_ALERT_TYPE    talertType,
                           unsigned long long  tcollectionTimestamp, 
                           EVEL_SEVERITIES     teventSeverity,
                           unsigned long long  teventStartTimestamp )
{
        EVENT_THRESHOLD_CROSS * event = NULL;
        EVEL_ENTER();

	assert( tcriticality!= NULL );
	assert( tname!= NULL );
	assert( tthresholdCrossed != NULL );
	assert( tvalue!= NULL );
	assert( talertDescription != NULL );
		

	/***************************************************************************/
	/* Allocate the Threshold crossing event.                                  */
	/***************************************************************************/
	event = malloc(sizeof(EVENT_THRESHOLD_CROSS));
	if (event == NULL)
	{
	    log_error_state("Out of memory");
	    goto exit_label;
	}
	memset(event, 0, sizeof(EVENT_THRESHOLD_CROSS));
	EVEL_DEBUG("New Threshold Cross event is at %lp", event);

  /***************************************************************************/
  /* Initialize the header & the threshold crossing fields.                  */
  /***************************************************************************/
  evel_init_header_nameid(&event->header,ev_name,ev_id);
  event->header.event_domain = EVEL_DOMAIN_THRESHOLD_CROSS;
  event->major_version = EVEL_THRESHOLD_CROSS_MAJOR_VERSION;
  event->minor_version = EVEL_THRESHOLD_CROSS_MINOR_VERSION;


  event->additionalParameters.criticality = strdup(tcriticality);
  event->additionalParameters.name = strdup(tname);
  event->additionalParameters.thresholdCrossed = strdup(tthresholdCrossed);
  event->additionalParameters.value = strdup(tvalue);
  event->alertAction      =  talertAction;
  event->alertDescription =  strdup(talertDescription); 
  event->alertType        =  talertType;
  event->collectionTimestamp =   tcollectionTimestamp; 
  event->eventSeverity       =   teventSeverity;
  event->eventStartTimestamp =   teventStartTimestamp;

  evel_init_option_string(&event->alertValue);
  evel_init_option_string(&event->dataCollector);
  evel_init_option_string(&event->elementType);
  evel_init_option_string(&event->interfaceName);
  evel_init_option_string(&event->networkService);
  evel_init_option_string(&event->possibleRootCause);
  dlist_initialize(&event->additional_info);
  dlist_initialize(&event->alertidList);

exit_label:

  EVEL_EXIT();
  return event;

}


/**************************************************************************//**
 * Set the Event Type property of the TC Alert.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param type        The Event Type to be set. ASCIIZ string. The caller
 *                    does not need to preserve the value once the function
 *                    returns.
 *****************************************************************************/
void evel_threshold_cross_type_set(EVENT_THRESHOLD_CROSS * const event,char *  type)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions and call evel_header_type_set.                      */
  /***************************************************************************/
  assert(type!=NULL);
  assert(event->header.event_domain == EVEL_DOMAIN_THRESHOLD_CROSS);
  evel_header_type_set(&event->header, type);

  EVEL_EXIT();
}

/**************************************************************************//**
 * Add an optional additional alertid value to Alert.
 *
 *****************************************************************************/
void evel_threshold_cross_alertid_add(EVENT_THRESHOLD_CROSS * const event,char *  alertid)
{
  char *alid=NULL;
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(event != NULL);
  assert(event->header.event_domain == EVEL_DOMAIN_THRESHOLD_CROSS);
  assert(alertid != NULL);

  EVEL_DEBUG("Adding AlertId=%s", alertid);
  alid = strdup(alertid);
  assert(alid != NULL);

  dlist_push_last(&event->alertidList, alid);

  EVEL_EXIT();
}
	
/**************************************************************************//**
 * Add an optional additional value name/value pair to the Alert.
 *
 * The name and value are NULL delimited ASCII strings.  The library takes
 * a copy so the caller does not have to preserve values after the function
 * returns.
 * @param name      ASCIIZ string with the attribute's name.  The caller
 *                  does not need to preserve the value once the function
 *                  returns.
 * @param value     ASCIIZ string with the attribute's value.  The caller
 *                  does not need to preserve the value once the function
 *                  returns.
 *****************************************************************************/
void evel_threshold_cross_addl_info_add(EVENT_THRESHOLD_CROSS * const event, const char *  name, const char *  value)
{
  OTHER_FIELD * nv_pair = NULL;

  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(event != NULL);
  assert(event->header.event_domain == EVEL_DOMAIN_THRESHOLD_CROSS);
  assert(name != NULL);
  assert(value != NULL);

  EVEL_DEBUG("Adding name=%s value=%s", name, value);
  nv_pair = malloc(sizeof(OTHER_FIELD));
  assert(nv_pair != NULL);
  nv_pair->name = strdup(name);
  nv_pair->value = strdup(value);
  assert(nv_pair->name != NULL);
  assert(nv_pair->value != NULL);

  dlist_push_last(&event->additional_info, nv_pair);

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
void evel_free_threshold_cross(EVENT_THRESHOLD_CROSS * const event)
{
  OTHER_FIELD * addl_info = NULL;
  char *ptr;
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.  As an internal API we don't allow freeing NULL    */
  /* events as we do on the API.                                      */
  /***************************************************************************/
  assert(event != NULL);
  assert(event->header.event_domain == EVEL_DOMAIN_THRESHOLD_CROSS);

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
  ptr = dlist_pop_last(&event->alertidList);
  while (ptr != NULL)
  {
    free(ptr);
    ptr = dlist_pop_last(&event->alertidList);
  }

  free(event->additionalParameters.criticality);
  free(event->additionalParameters.name);
  free(event->additionalParameters.thresholdCrossed);
  free(event->additionalParameters.value);
  free(event->alertDescription); 

  evel_free_option_string(&event->alertValue);
  evel_free_option_string(&event->dataCollector);
  evel_free_option_string(&event->elementType);
  evel_free_option_string(&event->interfaceName);
  evel_free_option_string(&event->networkService);
  evel_free_option_string(&event->possibleRootCause);
  evel_free_header(&event->header);

  EVEL_EXIT();
}

  /**************************************************************************//**
   * Set the TCA probable Root cause.
   *
   * @param sheader     Possible root cause to Threshold
   *****************************************************************************/
  void evel_threshold_cross_possible_rootcause_set(EVENT_THRESHOLD_CROSS * const event, char *  sheader)
  {
    EVEL_ENTER();

    /***************************************************************************/
    /* Check preconditions.                                                    */
    /***************************************************************************/
    assert(event->header.event_domain == EVEL_DOMAIN_THRESHOLD_CROSS);
    assert(sheader != NULL);

    evel_set_option_string(&event->possibleRootCause,
                         sheader,
                         "Rootcause value");

    EVEL_EXIT();
  }
    
  /**************************************************************************//**
   * Set the TCA networking cause.
   *
   * @param sheader     Possible networking service value to Threshold
   *****************************************************************************/
  void evel_threshold_cross_networkservice_set(EVENT_THRESHOLD_CROSS * const event, char *  sheader)
  {
	    EVEL_ENTER();

    /***************************************************************************/
    /* Check preconditions.                                                    */
    /***************************************************************************/
    assert(event->header.event_domain == EVEL_DOMAIN_THRESHOLD_CROSS);
    assert(sheader != NULL);

    evel_set_option_string(&event->networkService,
                         sheader,
                         "Networking service value");

	    EVEL_EXIT();
  }
    
  /**************************************************************************//**
   * Set the TCA Interface name.
   *
   * @param sheader     Interface name to threshold
   *****************************************************************************/
  void evel_threshold_cross_interfacename_set(EVENT_THRESHOLD_CROSS * const event,char *  sheader)
  {
	    EVEL_ENTER();

	    /***************************************************************************/
	    /* Check preconditions.                                                    */
	    /***************************************************************************/
            assert(event->header.event_domain == EVEL_DOMAIN_THRESHOLD_CROSS);
	    assert(sheader != NULL);

	    evel_set_option_string(&event->interfaceName,
	                           sheader,
	                           "TCA Interface name");
	    EVEL_EXIT();
  }
    
  /**************************************************************************//**
   * Set the TCA Data element type.
   *
   * @param sheader     element type of Threshold
   *****************************************************************************/
  void evel_threshold_cross_data_elementtype_set(EVENT_THRESHOLD_CROSS * const event,char *  sheader)
  {
	    EVEL_ENTER();

	    /***************************************************************************/
	    /* Check preconditions.                                                    */
	    /***************************************************************************/
            assert(event->header.event_domain == EVEL_DOMAIN_THRESHOLD_CROSS);
	    assert(sheader != NULL);

	    evel_set_option_string(&event->elementType,
	                           sheader,
	                           "TCA Element type value");
	    EVEL_EXIT();
  }

  /**************************************************************************//**
   * Set the TCA Data collector value.
   *
   * @param sheader     Data collector value
   *****************************************************************************/
  void evel_threshold_cross_data_collector_set(EVENT_THRESHOLD_CROSS * const event,char *  sheader)
  {
	    EVEL_ENTER();

	    /***************************************************************************/
	    /* Check preconditions.                                                    */
	    /***************************************************************************/
            assert(event->header.event_domain == EVEL_DOMAIN_THRESHOLD_CROSS);
	    assert(sheader != NULL);

	    evel_set_option_string(&event->dataCollector,
	                           sheader,
	                           "Datacollector value");
	    EVEL_EXIT();
  }
    
    
    
  /**************************************************************************//**
   * Set the TCA alert value.
   *
   * @param sheader     Possible alert value
   *****************************************************************************/
  void evel_threshold_cross_alertvalue_set(EVENT_THRESHOLD_CROSS * const event,char *  sheader)
  {
	    EVEL_ENTER();

	    /***************************************************************************/
	    /* Check preconditions.                                                    */
	    /***************************************************************************/
            assert(event->header.event_domain == EVEL_DOMAIN_THRESHOLD_CROSS);
	    assert(sheader != NULL);

	    evel_set_option_string(&event->alertValue,
	                           sheader,
	                           "Alert value");
	    EVEL_EXIT();
  }

/**************************************************************************//**
 * Encode the Mobile Flow GTP Per Flow Metrics as a JSON object.
 *
 * @param jbuf          Pointer to working ::EVEL_JSON_BUFFER.
 * @param metrics       Pointer to the ::EVENT_MOBILE_FLOW to encode.
 * @returns Number of bytes actually written.
 *****************************************************************************/
void evel_json_encode_perf_counter( EVEL_JSON_BUFFER * jbuf, PERF_COUNTER *pcounter)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(jbuf != NULL);
  assert(pcounter != NULL);

  evel_json_open_named_object(jbuf, "additionalParameters");

  /***************************************************************************/
  /* Mandatory parameters.                                                   */
  /***************************************************************************/
  evel_enc_kv_string(jbuf, "criticality", pcounter->criticality);
  evel_enc_kv_string(jbuf, "name", pcounter->name);
  evel_enc_kv_string(jbuf, "thresholdCrossed", pcounter->name);
  evel_enc_kv_string(jbuf, "value", pcounter->value);

  evel_json_close_object(jbuf);

  EVEL_EXIT();
}

/**************************************************************************//**
 * Encode the Signaling in JSON according to AT&T's schema for the
 * event type.
 *
 * @param jbuf          Pointer to the ::EVEL_JSON_BUFFER to encode into.
 * @param event         Pointer to the ::EVENT_HEADER to encode.
 *****************************************************************************/
void evel_json_encode_threshold_cross(EVEL_JSON_BUFFER * const jbuf,
                                EVENT_THRESHOLD_CROSS * const event)
{
  OTHER_FIELD * nv_pair = NULL;
  DLIST_ITEM * dlist_item = NULL;

  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(event != NULL);
  assert(event->header.event_domain == EVEL_DOMAIN_THRESHOLD_CROSS);

  evel_json_encode_header(jbuf, &event->header);
  evel_json_open_named_object(jbuf, "thresholdCrossingAlert");

  /***************************************************************************/
  /* Mandatory fields                                                        */
  /***************************************************************************/
  evel_json_encode_perf_counter(jbuf, &event->additionalParameters);
  evel_enc_kv_int(jbuf, "alertAction", event->alertAction);
  evel_enc_kv_string(jbuf, "alertDescription", event->alertDescription);
  evel_enc_kv_int(jbuf, "alertType", event->alertType);
  evel_enc_kv_ull(
    jbuf, "collectionTimestamp", event->collectionTimestamp);
  evel_enc_kv_int(jbuf, "eventSeverity", event->eventSeverity);
  evel_enc_kv_ull(
    jbuf, "eventStartTimestamp", event->eventStartTimestamp);

  /***************************************************************************/
  /* Optional fields                                                         */
  /***************************************************************************/
  evel_enc_kv_opt_string(jbuf, "alertValue", &event->alertValue);
  evel_enc_kv_opt_string(jbuf, "dataCollector", &event->dataCollector);
  evel_enc_kv_opt_string(jbuf, "elementType", &event->elementType);
  evel_enc_kv_opt_string(jbuf, "interfaceName", &event->interfaceName);
  evel_enc_kv_opt_string(jbuf, "networkService", &event->networkService);
  evel_enc_kv_opt_string(jbuf, "possibleRootCause", &event->possibleRootCause);

  /***************************************************************************/
  /* Checkpoint, so that we can wind back if all fields are suppressed.      */
  /***************************************************************************/
  evel_json_checkpoint(jbuf);
  if (evel_json_open_opt_named_list(jbuf, "additionalFields"))
  {
    bool added = false;

    dlist_item = dlist_get_first(&event->additional_info);
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
  evel_enc_version(jbuf,
                   "thresholdCrossingFieldsVersion",
                   event->major_version,
                   event->minor_version);

  evel_json_close_object(jbuf);

  EVEL_EXIT();
}

