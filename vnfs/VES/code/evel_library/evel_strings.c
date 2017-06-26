/**************************************************************************//**
 * @file
 * Implementation of EVEL functions to convert common enum types to strings.
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

#include "evel_internal.h"

/**************************************************************************//**
 * Map an ::EVEL_COUNTER_CRITICALITIES enum value to the equivalent string.
 *
 * @param criticality   The criticality to convert.
 * @returns The equivalent string.
 *****************************************************************************/
char * evel_criticality(const EVEL_COUNTER_CRITICALITIES criticality)
{
  char * result;

  EVEL_ENTER();

  switch (criticality)
  {
    case EVEL_COUNTER_CRITICALITY_CRIT:
      result = "CRIT";
      break;

    case EVEL_COUNTER_CRITICALITY_MAJ:
      result = "MAJ";
      break;

    default:
      EVEL_ERROR("Unexpected counter criticality %d", criticality);
      assert(0);
  }

  EVEL_EXIT();

  return result;
}

/**************************************************************************//**
 * Map an ::EVEL_SEVERITIES enum value to the equivalent string.
 *
 * @param severity      The severity to convert.
 * @returns The equivalent string.
 *****************************************************************************/
char * evel_severity(const EVEL_SEVERITIES severity)
{
  char * result;

  EVEL_ENTER();

  switch (severity)
  {
    case EVEL_SEVERITY_CRITICAL:
      result = "CRITICAL";
      break;

    case EVEL_SEVERITY_MAJOR:
      result = "MAJOR";
      break;

    case EVEL_SEVERITY_MINOR:
      result = "MINOR";
      break;

    case EVEL_SEVERITY_WARNING:
      result = "WARNING";
      break;

    case EVEL_SEVERITY_NORMAL:
      result = "NORMAL";
      break;

    default:
      EVEL_ERROR("Unexpected event severity %d", severity);
      assert(0);
  }

  EVEL_EXIT();

  return result;
}

/**************************************************************************//**
 * Map an ::EVEL_ALERT_ACTIONS enum value to the equivalent string.
 *
 * @param alert_action  The alert_action to convert.
 * @returns The equivalent string.
 *****************************************************************************/
char * evel_alert_action(const EVEL_ALERT_ACTIONS alert_action)
{
  char * result;

  EVEL_ENTER();

  switch (alert_action)
  {
    case EVEL_ALERT_ACTION_CLEAR:
      result = "CLEAR";
      break;

    case EVEL_ALERT_ACTION_CONT:
      result = "CONT";
      break;

    case EVEL_ALERT_ACTION_SET:
      result = "SET";
      break;

    default:
      EVEL_ERROR("Unexpected alert action %d", alert_action);
      assert(0);
  }

  EVEL_EXIT();

  return result;
}

/**************************************************************************//**
 * Map an ::EVEL_ALERT_TYPES enum value to the equivalent string.
 *
 * @param alert_type    The alert_type to convert.
 * @returns The equivalent string.
 *****************************************************************************/
char * evel_alert_type(const EVEL_ALERT_TYPES alert_type)
{
  char * result;

  EVEL_ENTER();

  switch (alert_type)
  {
    case EVEL_ALERT_TYPE_CARD:
      result = "CARD-ANOMALY";
      break;

    case EVEL_ALERT_TYPE_ELEMENT:
      result = "ELEMENT-ANOMALY";
      break;

    case EVEL_ALERT_TYPE_INTERFACE:
      result = "INTERFACE-ANOMALY";
      break;

    case EVEL_ALERT_TYPE_SERVICE:
      result = "SERVICE-ANOMALY";
      break;

    default:
      EVEL_ERROR("Unexpected alert type %d", alert_type);
      assert(0);
  }

  EVEL_EXIT();

  return result;
}

/**************************************************************************//**
 * Map an ::EVEL_EVENT_DOMAINS enum value to the equivalent string.
 *
 * @param domain        The domain to convert.
 * @returns The equivalent string.
 *****************************************************************************/
char * evel_event_domain(const EVEL_EVENT_DOMAINS domain)
{
  char * result;

  EVEL_ENTER();

  switch (domain)
  {
    case EVEL_DOMAIN_HEARTBEAT:
      result = "heartbeat";
      break;

    case EVEL_DOMAIN_FAULT:
      result = "fault";
      break;

    case EVEL_DOMAIN_MEASUREMENT:
      result = "measurementsForVfScaling";
      break;

    case EVEL_DOMAIN_REPORT:
      result = "measurementsForVfReporting";
      break;

    case EVEL_DOMAIN_MOBILE_FLOW:
      result = "mobileFlow";
      break;

    case EVEL_DOMAIN_SERVICE:
      result = "serviceEvents";
      break;

    case EVEL_DOMAIN_SIGNALING:
      result = "signaling";
      break;

    case EVEL_DOMAIN_STATE_CHANGE:
      result = "stateChange";
      break;

    case EVEL_DOMAIN_SYSLOG:
      result = "syslog";
      break;

    case EVEL_DOMAIN_OTHER:
      result = "other";
      break;

    default:
      result = NULL;
      EVEL_ERROR("Unexpected domain %d", domain);
      assert(0);
  }

  EVEL_EXIT();

  return result;
}

/**************************************************************************//**
 * Map an ::EVEL_EVENT_PRIORITIES enum value to the equivalent string.
 *
 * @param priority      The priority to convert.
 * @returns The equivalent string.
 *****************************************************************************/
char * evel_event_priority(const EVEL_EVENT_PRIORITIES priority)
{
  char * result;

  EVEL_ENTER();

  switch (priority)
  {
    case EVEL_PRIORITY_HIGH:
      result = "High";
      break;

    case EVEL_PRIORITY_MEDIUM:
      result = "Medium";
      break;

    case EVEL_PRIORITY_NORMAL:
      result = "Normal";
      break;

    case EVEL_PRIORITY_LOW:
      result = "Low";
      break;

    default:
      result = NULL;
      EVEL_ERROR("Unexpected priority %d", priority);
      assert(0);
  }

  EVEL_EXIT();

  return result;
}

/**************************************************************************//**
 * Map an ::EVEL_SOURCE_TYPES enum value to the equivalent string.
 *
 * @param source_type   The source type to convert.
 * @returns The equivalent string.
 *****************************************************************************/
char * evel_source_type(const EVEL_SOURCE_TYPES source_type)
{
  char * result;

  EVEL_ENTER();

  switch (source_type)
  {
    case EVEL_SOURCE_OTHER:
      result = "other";
      break;

    case EVEL_SOURCE_ROUTER:
      result = "router";
      break;

    case EVEL_SOURCE_SWITCH:
      result = "switch";
      break;

    case EVEL_SOURCE_HOST:
      result = "host";
      break;

    case EVEL_SOURCE_CARD:
      result = "card";
      break;

    case EVEL_SOURCE_PORT:
      result = "port";
      break;

    case EVEL_SOURCE_SLOT_THRESHOLD:
      result = "slotThreshold";
      break;

    case EVEL_SOURCE_PORT_THRESHOLD:
      result = "portThreshold";
      break;

    case EVEL_SOURCE_VIRTUAL_MACHINE:
      result = "virtualMachine";
      break;

    case EVEL_SOURCE_VIRTUAL_NETWORK_FUNCTION:
      result = "virtualNetworkFunction";
      break;

    default:
      result = NULL;
      EVEL_ERROR("Unexpected Event Source Type %d", (int) source_type);
      assert(0);
  }

  EVEL_EXIT();

  return result;
}

/**************************************************************************//**
 * Map an ::EVEL_VF_STATUSES enum value to the equivalent string.
 *
 * @param vf_status     The vf_status to convert.
 * @returns The equivalent string.
 *****************************************************************************/
char * evel_vf_status(const EVEL_VF_STATUSES vf_status)
{
  char * result;

  EVEL_ENTER();

  switch (vf_status)
  {
    case EVEL_VF_STATUS_ACTIVE:
      result = "Active";
      break;

    case EVEL_VF_STATUS_IDLE:
      result = "Idle";
      break;

    case EVEL_VF_STATUS_PREP_TERMINATE:
      result = "Preparing to terminate";
      break;

    case EVEL_VF_STATUS_READY_TERMINATE:
      result = "Ready to terminate";
      break;

    case EVEL_VF_STATUS_REQ_TERMINATE:
      result = "Requesting termination";
      break;

    default:
      result = NULL;
      EVEL_ERROR("Unexpected VF Status %d", vf_status);
      assert(0);
  }

  EVEL_EXIT();

  return result;
}

/**************************************************************************//**
 * Convert a ::EVEL_ENTITY_STATE to it's string form for JSON encoding.
 *
 * @param state         The entity state to encode.
 *
 * @returns the corresponding string
 *****************************************************************************/
char * evel_entity_state(const EVEL_ENTITY_STATE state)
{
  char * result;

  EVEL_ENTER();

  switch (state)
  {
    case EVEL_ENTITY_STATE_IN_SERVICE:
      result = "inService";
      break;

    case EVEL_ENTITY_STATE_MAINTENANCE:
      result = "maintenance";
      break;

    case EVEL_ENTITY_STATE_OUT_OF_SERVICE:
      result = "outOfService";
      break;

    default:
      EVEL_ERROR("Unexpected entity state %d", state);
      assert(0);
  }

  EVEL_EXIT();

  return result;
}

/**************************************************************************//**
 * Convert a ::EVEL_SERVICE_ENDPOINT_DESC to string form for JSON encoding.
 *
 * @param endpoint_desc endpoint description to encode.
 *
 * @returns the corresponding string
 *****************************************************************************/
char * evel_service_endpoint_desc(const EVEL_ENTITY_STATE endpoint_desc)
{
  char * result;

  EVEL_ENTER();

  switch (endpoint_desc)
  {
    case EVEL_SERVICE_ENDPOINT_CALLEE:
      result = "Callee";
      break;

    case EVEL_SERVICE_ENDPOINT_CALLER:
      result = "Caller";
      break;

    default:
      EVEL_ERROR("Unexpected endpoint description %d", endpoint_desc);
      assert(0);
  }

  EVEL_EXIT();

  return result;
}
