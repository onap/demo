/**************************************************************************//**
 * @file
 * Implementation of EVEL functions relating to the Mobile Flow.
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
#include <time.h>

#include "evel.h"
#include "evel_internal.h"

/*****************************************************************************/
/* Array of strings to use when encoding TCP flags.                          */
/*****************************************************************************/
static char * evel_tcp_flag_strings[EVEL_MAX_TCP_FLAGS] = {
  "NS",
  "CWR",
  "ECE",
  "URG",
  "ACK",
  "PSH",
  "RST",
  "SYN",
  "FIN"
};

/*****************************************************************************/
/* Array of strings to use when encoding QCI COS.                            */
/*****************************************************************************/
static char * evel_qci_cos_strings[EVEL_MAX_QCI_COS_TYPES] = {
  "conversational",
  "streaming",
  "interactive",
  "background",
  "1",
  "2",
  "3",
  "4",
  "65",
  "66",
  "5",
  "6",
  "7",
  "8",
  "9",
  "69",
  "70"
};

/*****************************************************************************/
/* Local prototypes                                                          */
/*****************************************************************************/
void evel_json_encode_mobile_flow_gtp_flow_metrics(
                                        EVEL_JSON_BUFFER * jbuf,
                                        MOBILE_GTP_PER_FLOW_METRICS * metrics);

/**************************************************************************//**
 * Create a new Mobile Flow event.
 *
 * @note    The mandatory fields on the Mobile Flow must be supplied to this
 *          factory function and are immutable once set.  Optional fields have
 *          explicit setter functions, but again values may only be set once so
 *          that the Mobile Flow has immutable properties.
 * @param   flow_direction              Flow direction.
 * @param   gtp_per_flow_metrics        GTP per-flow metrics.
 * @param   ip_protocol_type            IP protocol type.
 * @param   ip_version                  IP protocol version.
 * @param   other_endpoint_ip_address   IP address of the other endpoint.
 * @param   other_endpoint_port         IP port of the other endpoint.
 * @param   reporting_endpoint_ip_addr  IP address of the reporting endpoint.
 * @param   reporting_endpoint_port     IP port of the reporting endpoint.
 * @returns pointer to the newly manufactured ::EVENT_MOBILE_FLOW.  If the
 *          event is not used (i.e. posted) it must be released using
 *          ::evel_free_mobile_flow.
 * @retval  NULL  Failed to create the event.
 *****************************************************************************/
EVENT_MOBILE_FLOW * evel_new_mobile_flow(
                            const char * const flow_direction,
                            MOBILE_GTP_PER_FLOW_METRICS * gtp_per_flow_metrics,
                            const char * const ip_protocol_type,
                            const char * const ip_version,
                            const char * const other_endpoint_ip_address,
                            int other_endpoint_port,
                            const char * const reporting_endpoint_ip_addr,
                            int reporting_endpoint_port)
{
  EVENT_MOBILE_FLOW * mobile_flow = NULL;
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(flow_direction != NULL);
  assert(gtp_per_flow_metrics != NULL);
  assert(ip_protocol_type != NULL);
  assert(ip_version != NULL);
  assert(other_endpoint_ip_address != NULL);
  assert(other_endpoint_port > 0);
  assert(reporting_endpoint_ip_addr != NULL);
  assert(reporting_endpoint_port > 0);

  /***************************************************************************/
  /* Allocate the Mobile Flow.                                               */
  /***************************************************************************/
  mobile_flow = malloc(sizeof(EVENT_MOBILE_FLOW));
  if (mobile_flow == NULL)
  {
    log_error_state("Out of memory");
    goto exit_label;
  }
  memset(mobile_flow, 0, sizeof(EVENT_MOBILE_FLOW));
  EVEL_DEBUG("New Mobile Flow is at %lp", mobile_flow);

  /***************************************************************************/
  /* Initialize the header & the Mobile Flow fields.  Optional string values */
  /* are uninitialized (NULL).                                               */
  /***************************************************************************/
  evel_init_header(&mobile_flow->header);
  mobile_flow->header.event_domain = EVEL_DOMAIN_MOBILE_FLOW;
  mobile_flow->major_version = EVEL_MOBILE_FLOW_MAJOR_VERSION;
  mobile_flow->minor_version = EVEL_MOBILE_FLOW_MINOR_VERSION;
  mobile_flow->flow_direction = strdup(flow_direction);
  mobile_flow->gtp_per_flow_metrics = gtp_per_flow_metrics;
  mobile_flow->ip_protocol_type = strdup(ip_protocol_type);
  mobile_flow->ip_version = strdup(ip_version);
  mobile_flow->other_endpoint_ip_address = strdup(other_endpoint_ip_address);
  mobile_flow->other_endpoint_port = other_endpoint_port;
  mobile_flow->reporting_endpoint_ip_addr = strdup(reporting_endpoint_ip_addr);
  mobile_flow->reporting_endpoint_port = reporting_endpoint_port;
  evel_init_option_string(&mobile_flow->application_type);
  evel_init_option_string(&mobile_flow->app_protocol_type);
  evel_init_option_string(&mobile_flow->app_protocol_version);
  evel_init_option_string(&mobile_flow->cid);
  evel_init_option_string(&mobile_flow->connection_type);
  evel_init_option_string(&mobile_flow->ecgi);
  evel_init_option_string(&mobile_flow->gtp_protocol_type);
  evel_init_option_string(&mobile_flow->gtp_version);
  evel_init_option_string(&mobile_flow->http_header);
  evel_init_option_string(&mobile_flow->imei);
  evel_init_option_string(&mobile_flow->imsi);
  evel_init_option_string(&mobile_flow->lac);
  evel_init_option_string(&mobile_flow->mcc);
  evel_init_option_string(&mobile_flow->mnc);
  evel_init_option_string(&mobile_flow->msisdn);
  evel_init_option_string(&mobile_flow->other_functional_role);
  evel_init_option_string(&mobile_flow->rac);
  evel_init_option_string(&mobile_flow->radio_access_technology);
  evel_init_option_string(&mobile_flow->sac);
  evel_init_option_int(&mobile_flow->sampling_algorithm);
  evel_init_option_string(&mobile_flow->tac);
  evel_init_option_string(&mobile_flow->tunnel_id);
  evel_init_option_string(&mobile_flow->vlan_id);

exit_label:
  EVEL_EXIT();
  return mobile_flow;
}

/**************************************************************************//**
 * Set the Event Type property of the Mobile Flow.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param mobile_flow Pointer to the Mobile Flow.
 * @param type        The Event Type to be set. ASCIIZ string. The caller
 *                    does not need to preserve the value once the function
 *                    returns.
 *****************************************************************************/
void evel_mobile_flow_type_set(EVENT_MOBILE_FLOW * mobile_flow,
                               const char * const type)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions and call evel_header_type_set.                      */
  /***************************************************************************/
  assert(mobile_flow != NULL);
  assert(mobile_flow->header.event_domain == EVEL_DOMAIN_MOBILE_FLOW);
  evel_header_type_set(&mobile_flow->header, type);

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Application Type property of the Mobile Flow.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param mobile_flow Pointer to the Mobile Flow.
 * @param type        The Application Type to be set. ASCIIZ string. The caller
 *                    does not need to preserve the value once the function
 *                    returns.
 *****************************************************************************/
void evel_mobile_flow_app_type_set(EVENT_MOBILE_FLOW * mobile_flow,
                                   const char * const type)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(mobile_flow != NULL);
  assert(mobile_flow->header.event_domain == EVEL_DOMAIN_MOBILE_FLOW);
  assert(type != NULL);

  evel_set_option_string(&mobile_flow->application_type,
                         type,
                         "Application Type");
  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Application Protocol Type property of the Mobile Flow.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param mobile_flow Pointer to the Mobile Flow.
 * @param type        The Application Protocol Type to be set. ASCIIZ string.
 *                    The caller does not need to preserve the value once the
 *                    function returns.
 *****************************************************************************/
void evel_mobile_flow_app_prot_type_set(EVENT_MOBILE_FLOW * mobile_flow,
                                        const char * const type)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(mobile_flow != NULL);
  assert(mobile_flow->header.event_domain == EVEL_DOMAIN_MOBILE_FLOW);
  assert(type != NULL);

  evel_set_option_string(&mobile_flow->app_protocol_type,
                         type,
                         "Application Protocol Type");
  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Application Protocol Version property of the Mobile Flow.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param mobile_flow Pointer to the Mobile Flow.
 * @param version     The Application Protocol Version to be set. ASCIIZ
 *                    string.  The caller does not need to preserve the value
 *                    once the function returns.
 *****************************************************************************/
void evel_mobile_flow_app_prot_ver_set(EVENT_MOBILE_FLOW * mobile_flow,
                                       const char * const version)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(mobile_flow != NULL);
  assert(mobile_flow->header.event_domain == EVEL_DOMAIN_MOBILE_FLOW);
  assert(version != NULL);

  evel_set_option_string(&mobile_flow->app_protocol_version,
                         version,
                         "Application Protocol Version");
  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the CID property of the Mobile Flow.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param mobile_flow Pointer to the Mobile Flow.
 * @param cid         The CID to be set. ASCIIZ string.  The caller does not
 *                    need to preserve the value once the function returns.
 *****************************************************************************/
void evel_mobile_flow_cid_set(EVENT_MOBILE_FLOW * mobile_flow,
                              const char * const cid)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(mobile_flow != NULL);
  assert(mobile_flow->header.event_domain == EVEL_DOMAIN_MOBILE_FLOW);
  assert(cid != NULL);

  evel_set_option_string(&mobile_flow->cid,
                         cid,
                         "CID");
  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Connection Type property of the Mobile Flow.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param mobile_flow Pointer to the Mobile Flow.
 * @param type        The Connection Type to be set. ASCIIZ string. The caller
 *                    does not need to preserve the value once the function
 *                    returns.
 *****************************************************************************/
void evel_mobile_flow_con_type_set(EVENT_MOBILE_FLOW * mobile_flow,
                                   const char * const type)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(mobile_flow != NULL);
  assert(mobile_flow->header.event_domain == EVEL_DOMAIN_MOBILE_FLOW);
  assert(type != NULL);

  evel_set_option_string(&mobile_flow->connection_type,
                         type,
                         "Connection Type");
  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the ECGI property of the Mobile Flow.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param mobile_flow Pointer to the Mobile Flow.
 * @param ecgi        The ECGI to be set. ASCIIZ string.  The caller does not
 *                    need to preserve the value once the function returns.
 *****************************************************************************/
void evel_mobile_flow_ecgi_set(EVENT_MOBILE_FLOW * mobile_flow,
                               const char * const ecgi)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(mobile_flow != NULL);
  assert(mobile_flow->header.event_domain == EVEL_DOMAIN_MOBILE_FLOW);
  assert(ecgi != NULL);

  evel_set_option_string(&mobile_flow->ecgi,
                         ecgi,
                         "ECGI");
  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the GTP Protocol Type property of the Mobile Flow.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param mobile_flow Pointer to the Mobile Flow.
 * @param type        The GTP Protocol Type to be set. ASCIIZ string.  The
 *                    caller does not need to preserve the value once the
 *                    function returns.
 *****************************************************************************/
void evel_mobile_flow_gtp_prot_type_set(EVENT_MOBILE_FLOW * mobile_flow,
                                        const char * const type)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(mobile_flow != NULL);
  assert(mobile_flow->header.event_domain == EVEL_DOMAIN_MOBILE_FLOW);
  assert(type != NULL);

  evel_set_option_string(&mobile_flow->gtp_protocol_type,
                         type,
                         "GTP Protocol Type");
  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the GTP Protocol Version property of the Mobile Flow.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param mobile_flow Pointer to the Mobile Flow.
 * @param version     The GTP Protocol Version to be set. ASCIIZ string.  The
 *                    caller does not need to preserve the value once the
 *                    function returns.
 *****************************************************************************/
void evel_mobile_flow_gtp_prot_ver_set(EVENT_MOBILE_FLOW * mobile_flow,
                                       const char * const version)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(mobile_flow != NULL);
  assert(mobile_flow->header.event_domain == EVEL_DOMAIN_MOBILE_FLOW);
  assert(version != NULL);

  evel_set_option_string(&mobile_flow->gtp_version,
                         version,
                         "GTP Protocol Version");
  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the HTTP Header property of the Mobile Flow.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param mobile_flow Pointer to the Mobile Flow.
 * @param header      The HTTP header to be set. ASCIIZ string. The caller does
 *                    not need to preserve the value once the function returns.
 *****************************************************************************/
void evel_mobile_flow_http_header_set(EVENT_MOBILE_FLOW * mobile_flow,
                                      const char * const header)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(mobile_flow != NULL);
  assert(mobile_flow->header.event_domain == EVEL_DOMAIN_MOBILE_FLOW);
  assert(header != NULL);

  evel_set_option_string(&mobile_flow->http_header,
                         header,
                         "HTTP Header");
  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the IMEI property of the Mobile Flow.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param mobile_flow Pointer to the Mobile Flow.
 * @param imei        The IMEI to be set. ASCIIZ string.  The caller does not
 *                    need to preserve the value once the function returns.
 *****************************************************************************/
void evel_mobile_flow_imei_set(EVENT_MOBILE_FLOW * mobile_flow,
                               const char * const imei)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(mobile_flow != NULL);
  assert(mobile_flow->header.event_domain == EVEL_DOMAIN_MOBILE_FLOW);
  assert(imei != NULL);

  evel_set_option_string(&mobile_flow->imei,
                         imei,
                         "IMEI");
  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the IMSI property of the Mobile Flow.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param mobile_flow Pointer to the Mobile Flow.
 * @param imsi        The IMSI to be set. ASCIIZ string.  The caller does not
 *                    need to preserve the value once the function returns.
 *****************************************************************************/
void evel_mobile_flow_imsi_set(EVENT_MOBILE_FLOW * mobile_flow,
                               const char * const imsi)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(mobile_flow != NULL);
  assert(mobile_flow->header.event_domain == EVEL_DOMAIN_MOBILE_FLOW);
  assert(imsi != NULL);

  evel_set_option_string(&mobile_flow->imsi,
                         imsi,
                         "IMSI");
  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the LAC property of the Mobile Flow.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param mobile_flow Pointer to the Mobile Flow.
 * @param lac         The LAC to be set. ASCIIZ string.  The caller does not
 *                    need to preserve the value once the function returns.
 *****************************************************************************/
void evel_mobile_flow_lac_set(EVENT_MOBILE_FLOW * mobile_flow,
                              const char * const lac)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(mobile_flow != NULL);
  assert(mobile_flow->header.event_domain == EVEL_DOMAIN_MOBILE_FLOW);
  assert(lac != NULL);

  evel_set_option_string(&mobile_flow->lac,
                         lac,
                         "LAC");
  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the MCC property of the Mobile Flow.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param mobile_flow Pointer to the Mobile Flow.
 * @param mcc         The MCC to be set. ASCIIZ string.  The caller does not
 *                    need to preserve the value once the function returns.
 *****************************************************************************/
void evel_mobile_flow_mcc_set(EVENT_MOBILE_FLOW * mobile_flow,
                              const char * const mcc)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(mobile_flow != NULL);
  assert(mobile_flow->header.event_domain == EVEL_DOMAIN_MOBILE_FLOW);
  assert(mcc != NULL);

  evel_set_option_string(&mobile_flow->mcc,
                         mcc,
                         "MCC");
  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the MNC property of the Mobile Flow.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param mobile_flow Pointer to the Mobile Flow.
 * @param mnc         The MNC to be set. ASCIIZ string.  The caller does not
 *                    need to preserve the value once the function returns.
 *****************************************************************************/
void evel_mobile_flow_mnc_set(EVENT_MOBILE_FLOW * mobile_flow,
                              const char * const mnc)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(mobile_flow != NULL);
  assert(mobile_flow->header.event_domain == EVEL_DOMAIN_MOBILE_FLOW);
  assert(mnc != NULL);

  evel_set_option_string(&mobile_flow->mnc,
                         mnc,
                         "MNC");
  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the MSISDN property of the Mobile Flow.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param mobile_flow Pointer to the Mobile Flow.
 * @param msisdn      The MSISDN to be set. ASCIIZ string.  The caller does not
 *                    need to preserve the value once the function returns.
 *****************************************************************************/
void evel_mobile_flow_msisdn_set(EVENT_MOBILE_FLOW * mobile_flow,
                                 const char * const msisdn)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(mobile_flow != NULL);
  assert(mobile_flow->header.event_domain == EVEL_DOMAIN_MOBILE_FLOW);
  assert(msisdn != NULL);

  evel_set_option_string(&mobile_flow->msisdn,
                         msisdn,
                         "MSISDN");
  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Other Functional Role property of the Mobile Flow.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param mobile_flow Pointer to the Mobile Flow.
 * @param role        The Other Functional Role to be set. ASCIIZ string. The
 *                    caller does not need to preserve the value once the
 *                    function returns.
 *****************************************************************************/
void evel_mobile_flow_other_func_role_set(EVENT_MOBILE_FLOW * mobile_flow,
                                          const char * const role)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(mobile_flow != NULL);
  assert(mobile_flow->header.event_domain == EVEL_DOMAIN_MOBILE_FLOW);
  assert(role != NULL);

  evel_set_option_string(&mobile_flow->other_functional_role,
                         role,
                         "Other Functional Role");
  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the RAC property of the Mobile Flow.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param mobile_flow Pointer to the Mobile Flow.
 * @param rac         The RAC to be set. ASCIIZ string.  The caller does not
 *                    need to preserve the value once the function returns.
 *****************************************************************************/
void evel_mobile_flow_rac_set(EVENT_MOBILE_FLOW * mobile_flow,
                              const char * const rac)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(mobile_flow != NULL);
  assert(mobile_flow->header.event_domain == EVEL_DOMAIN_MOBILE_FLOW);
  assert(rac != NULL);

  evel_set_option_string(&mobile_flow->rac,
                         rac,
                         "RAC");
  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Radio Access Technology property of the Mobile Flow.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param mobile_flow Pointer to the Mobile Flow.
 * @param tech        The Radio Access Technology to be set. ASCIIZ string. The
 *                    caller does not need to preserve the value once the
 *                    function returns.
 *****************************************************************************/
void evel_mobile_flow_radio_acc_tech_set(EVENT_MOBILE_FLOW * mobile_flow,
                                         const char * const tech)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(mobile_flow != NULL);
  assert(mobile_flow->header.event_domain == EVEL_DOMAIN_MOBILE_FLOW);
  assert(tech != NULL);

  evel_set_option_string(&mobile_flow->radio_access_technology,
                         tech,
                         "Radio Access Technology");
  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the SAC property of the Mobile Flow.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param mobile_flow Pointer to the Mobile Flow.
 * @param sac         The SAC to be set. ASCIIZ string.  The caller does not
 *                    need to preserve the value once the function returns.
 *****************************************************************************/
void evel_mobile_flow_sac_set(EVENT_MOBILE_FLOW * mobile_flow,
                              const char * const sac)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(mobile_flow != NULL);
  assert(mobile_flow->header.event_domain == EVEL_DOMAIN_MOBILE_FLOW);
  assert(sac != NULL);

  evel_set_option_string(&mobile_flow->sac,
                         sac,
                         "SAC");
  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Sampling Algorithm property of the Mobile Flow.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param mobile_flow Pointer to the Mobile Flow.
 * @param algorithm   The Sampling Algorithm to be set.
 *****************************************************************************/
void evel_mobile_flow_samp_alg_set(EVENT_MOBILE_FLOW * mobile_flow,
                                   int algorithm)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(mobile_flow != NULL);
  assert(mobile_flow->header.event_domain == EVEL_DOMAIN_MOBILE_FLOW);
  assert(algorithm >= 0);

  evel_set_option_int(&mobile_flow->sampling_algorithm,
                      algorithm,
                      "Sampling Algorithm");
  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the TAC property of the Mobile Flow.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param mobile_flow Pointer to the Mobile Flow.
 * @param tac         The TAC to be set. ASCIIZ string.  The caller does not
 *                    need to preserve the value once the function returns.
 *****************************************************************************/
void evel_mobile_flow_tac_set(EVENT_MOBILE_FLOW * mobile_flow,
                              const char * const tac)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(mobile_flow != NULL);
  assert(mobile_flow->header.event_domain == EVEL_DOMAIN_MOBILE_FLOW);
  assert(tac != NULL);

  evel_set_option_string(&mobile_flow->tac,
                         tac,
                         "TAC");
  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Tunnel ID property of the Mobile Flow.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param mobile_flow Pointer to the Mobile Flow.
 * @param tunnel_id   The Tunnel ID to be set. ASCIIZ string.  The caller does
 *                    not need to preserve the value once the function returns.
 *****************************************************************************/
void evel_mobile_flow_tunnel_id_set(EVENT_MOBILE_FLOW * mobile_flow,
                                    const char * const tunnel_id)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(mobile_flow != NULL);
  assert(mobile_flow->header.event_domain == EVEL_DOMAIN_MOBILE_FLOW);
  assert(tunnel_id != NULL);

  evel_set_option_string(&mobile_flow->tunnel_id,
                         tunnel_id,
                         "Tunnel ID");
  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the VLAN ID property of the Mobile Flow.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param mobile_flow Pointer to the Mobile Flow.
 * @param vlan_id     The VLAN ID to be set. ASCIIZ string.  The caller does
 *                    not need to preserve the value once the function returns.
 *****************************************************************************/
void evel_mobile_flow_vlan_id_set(EVENT_MOBILE_FLOW * mobile_flow,
                                  const char * const vlan_id)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(mobile_flow != NULL);
  assert(mobile_flow->header.event_domain == EVEL_DOMAIN_MOBILE_FLOW);
  assert(vlan_id != NULL);

  evel_set_option_string(&mobile_flow->vlan_id,
                         vlan_id,
                         "VLAN ID");
  EVEL_EXIT();
}

/**************************************************************************//**
 * Encode the Mobile Flow in JSON according to AT&T's schema for the event
 * type.
 *
 * @param jbuf          Pointer to the ::EVEL_JSON_BUFFER to encode into.
 * @param event         Pointer to the ::EVENT_HEADER to encode.
 *****************************************************************************/
void evel_json_encode_mobile_flow(EVEL_JSON_BUFFER * jbuf,
                                  EVENT_MOBILE_FLOW * event)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(event != NULL);
  assert(event->header.event_domain == EVEL_DOMAIN_MOBILE_FLOW);

  evel_json_encode_header(jbuf, &event->header);
  evel_json_open_named_object(jbuf, "mobileFlowFields");

  /***************************************************************************/
  /* Mandatory parameters.                                                   */
  /***************************************************************************/
  evel_enc_kv_string(jbuf, "flowDirection", event->flow_direction);
  evel_json_encode_mobile_flow_gtp_flow_metrics(
    jbuf, event->gtp_per_flow_metrics);
  evel_enc_kv_string(jbuf, "ipProtocolType", event->ip_protocol_type);
  evel_enc_kv_string(jbuf, "ipVersion", event->ip_version);
  evel_enc_kv_string(
    jbuf, "otherEndpointIpAddress", event->other_endpoint_ip_address);
  evel_enc_kv_int(jbuf, "otherEndpointPort", event->other_endpoint_port);
  evel_enc_kv_string(
    jbuf, "reportingEndpointIpAddr", event->reporting_endpoint_ip_addr);
  evel_enc_kv_int(
    jbuf, "reportingEndpointPort", event->reporting_endpoint_port);

  /***************************************************************************/
  /* Optional parameters.                                                    */
  /***************************************************************************/
  evel_enc_kv_opt_string(jbuf, "applicationType", &event->application_type);
  evel_enc_kv_opt_string(jbuf, "appProtocolType", &event->app_protocol_type);
  evel_enc_kv_opt_string(
    jbuf, "appProtocolVersion", &event->app_protocol_version);
  evel_enc_kv_opt_string(jbuf, "cid", &event->cid);
  evel_enc_kv_opt_string(jbuf, "connectionType", &event->connection_type);
  evel_enc_kv_opt_string(jbuf, "ecgi", &event->ecgi);
  evel_enc_kv_opt_string(jbuf, "gtpProtocolType", &event->gtp_protocol_type);
  evel_enc_kv_opt_string(jbuf, "gtpVersion", &event->gtp_version);
  evel_enc_kv_opt_string(jbuf, "httpHeader", &event->http_header);
  evel_enc_kv_opt_string(jbuf, "imei", &event->imei);
  evel_enc_kv_opt_string(jbuf, "imsi", &event->imsi);
  evel_enc_kv_opt_string(jbuf, "lac", &event->lac);
  evel_enc_kv_opt_string(jbuf, "mcc", &event->mcc);
  evel_enc_kv_opt_string(jbuf, "mnc", &event->mnc);
  evel_enc_kv_opt_string(jbuf, "msisdn", &event->msisdn);
  evel_enc_kv_opt_string(
    jbuf, "otherFunctionalRole", &event->other_functional_role);
  evel_enc_kv_opt_string(jbuf, "rac", &event->rac);
  evel_enc_kv_opt_string(
    jbuf, "radioAccessTechnology", &event->radio_access_technology);
  evel_enc_kv_opt_string(jbuf, "sac", &event->sac);
  evel_enc_kv_opt_int(jbuf, "samplingAlgorithm", &event->sampling_algorithm);
  evel_enc_kv_opt_string(jbuf, "tac", &event->tac);
  evel_enc_kv_opt_string(jbuf, "tunnelId", &event->tunnel_id);
  evel_enc_kv_opt_string(jbuf, "vlanId", &event->vlan_id);
#if 0
  /***************************************************************************/
  /* Not in schema.                                                          */
  /***************************************************************************/
  evel_enc_version(jbuf,
                   "mobileFlowFieldsVersion",
                   event->major_version,
                   event->minor_version);
#endif
  evel_json_close_object(jbuf);

  EVEL_EXIT();
}

/**************************************************************************//**
 * Free a Mobile Flow.
 *
 * Free off the Mobile Flow supplied.  Will free all the contained allocated
 * memory.
 *
 * @note It does not free the Mobile Flow itself, since that may be part of a
 * larger structure.
 *****************************************************************************/
void evel_free_mobile_flow(EVENT_MOBILE_FLOW * event)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.  As an internal API we don't allow freeing NULL    */
  /* events as we do on the public API.                                      */
  /***************************************************************************/
  assert(event != NULL);
  assert(event->header.event_domain == EVEL_DOMAIN_MOBILE_FLOW);

  /***************************************************************************/
  /* Free all internal strings then the header itself.                       */
  /***************************************************************************/
  free(event->flow_direction);

  evel_free_mobile_gtp_flow_metrics(event->gtp_per_flow_metrics);
  free(event->gtp_per_flow_metrics);
  free(event->ip_protocol_type);
  free(event->ip_version);
  free(event->other_endpoint_ip_address);
  free(event->reporting_endpoint_ip_addr);
  evel_free_option_string(&event->application_type);
  evel_free_option_string(&event->app_protocol_type);
  evel_free_option_string(&event->app_protocol_version);
  evel_free_option_string(&event->cid);
  evel_free_option_string(&event->connection_type);
  evel_free_option_string(&event->ecgi);
  evel_free_option_string(&event->gtp_protocol_type);
  evel_free_option_string(&event->gtp_version);
  evel_free_option_string(&event->http_header);
  evel_free_option_string(&event->imei);
  evel_free_option_string(&event->imsi);
  evel_free_option_string(&event->lac);
  evel_free_option_string(&event->mcc);
  evel_free_option_string(&event->mnc);
  evel_free_option_string(&event->msisdn);
  evel_free_option_string(&event->other_functional_role);
  evel_free_option_string(&event->rac);
  evel_free_option_string(&event->radio_access_technology);
  evel_free_option_string(&event->sac);
  evel_free_option_string(&event->tac);
  evel_free_option_string(&event->tunnel_id);
  evel_free_option_string(&event->vlan_id);

  evel_free_header(&event->header);

  EVEL_EXIT();
}

/**************************************************************************//**
 * Create a new Mobile GTP Per Flow Metrics.
 *
 * @note    The mandatory fields on the Mobile GTP Per Flow Metrics must be
 *          supplied to this factory function and are immutable once set.
 *          Optional fields have explicit setter functions, but again values
 *          may only be set once so that the Mobile GTP Per Flow Metrics has
 *          immutable properties.
 *
 * @param   avg_bit_error_rate          Average bit error rate.
 * @param   avg_packet_delay_variation  Average delay or jitter in ms.
 * @param   avg_packet_latency          Average delivery latency.
 * @param   avg_receive_throughput      Average receive throughput.
 * @param   avg_transmit_throughput     Average transmit throughput.
 * @param   flow_activation_epoch       Time the connection is activated.
 * @param   flow_activation_microsec    Microseconds for the start of the flow
 *                                      connection.
 * @param   flow_deactivation_epoch     Time for the end of the connection.
 * @param   flow_deactivation_microsec  Microseconds for the end of the flow
 *                                      connection.
 * @param   flow_deactivation_time      Transmission time of the first packet.
 * @param   flow_status                 Connection status.
 * @param   max_packet_delay_variation  Maximum packet delay or jitter in ms.
 * @param   num_activation_failures     Number of failed activation requests.
 * @param   num_bit_errors              Number of errored bits.
 * @param   num_bytes_received          Number of bytes received.
 * @param   num_bytes_transmitted       Number of bytes transmitted.
 * @param   num_dropped_packets         Number of received packets dropped.
 * @param   num_l7_bytes_received       Number of tunneled Layer 7 bytes
 *                                      received.
 * @param   num_l7_bytes_transmitted    Number of tunneled Layer 7 bytes
 *                                      transmitted.
 * @param   num_lost_packets            Number of lost packets.
 * @param   num_out_of_order_packets    Number of out-of-order packets.
 * @param   num_packet_errors           Number of errored packets.
 * @param   num_packets_received_excl_retrans  Number of packets received,
 *                                             excluding retransmits.
 * @param   num_packets_received_incl_retrans  Number of packets received.
 * @param   num_packets_transmitted_incl_retrans  Number of packets
 *                                                transmitted.
 * @param   num_retries                 Number of packet retries.
 * @param   num_timeouts                Number of packet timeouts.
 * @param   num_tunneled_l7_bytes_received  Number of tunneled Layer 7 bytes
 *                                          received, excluding retransmits.
 * @param   round_trip_time             Round trip time.
 * @param   time_to_first_byte          Time in ms between connection
 *                                      activation and first byte received.
 *
 * @returns pointer to the newly manufactured ::MOBILE_GTP_PER_FLOW_METRICS.
 *          If the structure is not used it must be released using
 *          ::evel_free_mobile_gtp_flow_metrics.
 * @retval  NULL  Failed to create the event.
 *****************************************************************************/
MOBILE_GTP_PER_FLOW_METRICS * evel_new_mobile_gtp_flow_metrics(
                                      double avg_bit_error_rate,
                                      double avg_packet_delay_variation,
                                      int avg_packet_latency,
                                      int avg_receive_throughput,
                                      int avg_transmit_throughput,
                                      int flow_activation_epoch,
                                      int flow_activation_microsec,
                                      int flow_deactivation_epoch,
                                      int flow_deactivation_microsec,
                                      time_t flow_deactivation_time,
                                      const char * const flow_status,
                                      int max_packet_delay_variation,
                                      int num_activation_failures,
                                      int num_bit_errors,
                                      int num_bytes_received,
                                      int num_bytes_transmitted,
                                      int num_dropped_packets,
                                      int num_l7_bytes_received,
                                      int num_l7_bytes_transmitted,
                                      int num_lost_packets,
                                      int num_out_of_order_packets,
                                      int num_packet_errors,
                                      int num_packets_received_excl_retrans,
                                      int num_packets_received_incl_retrans,
                                      int num_packets_transmitted_incl_retrans,
                                      int num_retries,
                                      int num_timeouts,
                                      int num_tunneled_l7_bytes_received,
                                      int round_trip_time,
                                      int time_to_first_byte)
{
  MOBILE_GTP_PER_FLOW_METRICS * metrics = NULL;
  int ii;

  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(avg_bit_error_rate >= 0.0);
  assert(avg_packet_delay_variation >= 0.0);
  assert(avg_packet_latency >= 0);
  assert(avg_receive_throughput >= 0);
  assert(avg_transmit_throughput >= 0);
  assert(flow_activation_epoch > 0);
  assert(flow_activation_microsec >= 0);
  assert(flow_deactivation_epoch > 0);
  assert(flow_deactivation_microsec >= 0);
  assert(flow_status != NULL);
  assert(max_packet_delay_variation >= 0);
  assert(num_activation_failures >= 0);
  assert(num_bit_errors >= 0);
  assert(num_bytes_received >= 0);
  assert(num_bytes_transmitted >= 0);
  assert(num_dropped_packets >= 0);
  assert(num_l7_bytes_received >= 0);
  assert(num_l7_bytes_transmitted >= 0);
  assert(num_lost_packets >= 0);
  assert(num_out_of_order_packets >= 0);
  assert(num_packet_errors >= 0);
  assert(num_packets_received_excl_retrans >= 0);
  assert(num_packets_received_incl_retrans >= 0);
  assert(num_packets_transmitted_incl_retrans >= 0);
  assert(num_retries >= 0);
  assert(num_timeouts >= 0);
  assert(num_tunneled_l7_bytes_received >= 0);
  assert(round_trip_time >= 0);
  assert(time_to_first_byte >= 0);

  /***************************************************************************/
  /* Allocate the Mobile Flow GTP Per Flow Metrics.                          */
  /***************************************************************************/
  metrics = malloc(sizeof(MOBILE_GTP_PER_FLOW_METRICS));
  if (metrics == NULL)
  {
    log_error_state("Out of memory");
    goto exit_label;
  }
  memset(metrics, 0, sizeof(MOBILE_GTP_PER_FLOW_METRICS));
  EVEL_DEBUG("New Mobile Flow GTP Per Flow Metrics is at %lp", metrics);

  /***************************************************************************/
  /* Initialize the Mobile Flow GTP Per Flow Metrics fields.  Optional       */
  /* string values are uninitialized (NULL).                                 */
  /***************************************************************************/
  metrics->avg_bit_error_rate = avg_bit_error_rate;
  metrics->avg_packet_delay_variation = avg_packet_delay_variation;
  metrics->avg_packet_latency = avg_packet_latency;
  metrics->avg_receive_throughput = avg_receive_throughput;
  metrics->avg_transmit_throughput = avg_transmit_throughput;
  metrics->flow_activation_epoch = flow_activation_epoch;
  metrics->flow_activation_microsec = flow_activation_microsec;
  metrics->flow_deactivation_epoch = flow_deactivation_epoch;
  metrics->flow_deactivation_microsec = flow_deactivation_microsec;
  metrics->flow_deactivation_time = flow_deactivation_time;
  metrics->flow_status = strdup(flow_status);
  metrics->max_packet_delay_variation = max_packet_delay_variation;
  metrics->num_activation_failures = num_activation_failures;
  metrics->num_bit_errors = num_bit_errors;
  metrics->num_bytes_received = num_bytes_received;
  metrics->num_bytes_transmitted = num_bytes_transmitted;
  metrics->num_dropped_packets = num_dropped_packets;
  metrics->num_l7_bytes_received = num_l7_bytes_received;
  metrics->num_l7_bytes_transmitted = num_l7_bytes_transmitted;
  metrics->num_lost_packets = num_lost_packets;
  metrics->num_out_of_order_packets = num_out_of_order_packets;
  metrics->num_packet_errors = num_packet_errors;
  metrics->num_packets_received_excl_retrans =
                                             num_packets_received_excl_retrans;
  metrics->num_packets_received_incl_retrans =
                                             num_packets_received_incl_retrans;
  metrics->num_packets_transmitted_incl_retrans =
                                          num_packets_transmitted_incl_retrans;
  metrics->num_retries = num_retries;
  metrics->num_timeouts = num_timeouts;
  metrics->num_tunneled_l7_bytes_received = num_tunneled_l7_bytes_received;
  metrics->round_trip_time = round_trip_time;
  metrics->time_to_first_byte = time_to_first_byte;
  for (ii = 0; ii < EVEL_TOS_SUPPORTED; ii++)
  {
    evel_init_option_int(&metrics->ip_tos_counts[ii]);
  }
  for (ii = 0; ii < EVEL_MAX_TCP_FLAGS; ii++)
  {
    evel_init_option_int(&metrics->tcp_flag_counts[ii]);
  }
  for (ii = 0; ii < EVEL_MAX_QCI_COS_TYPES; ii++)
  {
    evel_init_option_int(&metrics->qci_cos_counts[ii]);
  }
  evel_init_option_int(&metrics->dur_connection_failed_status);
  evel_init_option_int(&metrics->dur_tunnel_failed_status);
  evel_init_option_string(&metrics->flow_activated_by);
  evel_init_option_time(&metrics->flow_activation_time);
  evel_init_option_string(&metrics->flow_deactivated_by);
  evel_init_option_string(&metrics->gtp_connection_status);
  evel_init_option_string(&metrics->gtp_tunnel_status);
  evel_init_option_int(&metrics->large_packet_rtt);
  evel_init_option_double(&metrics->large_packet_threshold);
  evel_init_option_int(&metrics->max_receive_bit_rate);
  evel_init_option_int(&metrics->max_transmit_bit_rate);
  evel_init_option_int(&metrics->num_gtp_echo_failures);
  evel_init_option_int(&metrics->num_gtp_tunnel_errors);
  evel_init_option_int(&metrics->num_http_errors);

exit_label:
  EVEL_EXIT();
  return metrics;
}

/**************************************************************************//**
 * Set the Duration of Connection Failed Status property of the Mobile GTP Per
 * Flow Metrics.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param metrics     Pointer to the Mobile GTP Per Flow Metrics.
 * @param duration    The Duration of Connection Failed Status to be set.
 *****************************************************************************/
void evel_mobile_gtp_metrics_dur_con_fail_set(
                                         MOBILE_GTP_PER_FLOW_METRICS * metrics,
                                         int duration)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(metrics != NULL);
  assert(duration >= 0);

  evel_set_option_int(&metrics->dur_connection_failed_status,
                      duration,
                      "Duration of Connection Failed Status");
  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Duration of Tunnel Failed Status property of the Mobile GTP Per Flow
 * Metrics.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param metrics     Pointer to the Mobile GTP Per Flow Metrics.
 * @param duration    The Duration of Tunnel Failed Status to be set.
 *****************************************************************************/
void evel_mobile_gtp_metrics_dur_tun_fail_set(
                                         MOBILE_GTP_PER_FLOW_METRICS * metrics,
                                         int duration)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(metrics != NULL);
  assert(duration >= 0);

  evel_set_option_int(&metrics->dur_tunnel_failed_status,
                      duration,
                      "Duration of Tunnel Failed Status");
  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Activated By property of the Mobile GTP Per Flow metrics.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param metrics     Pointer to the Mobile GTP Per Flow Metrics.
 * @param act_by      The Activated By to be set.  ASCIIZ string. The caller
 *                    does not need to preserve the value once the function
 *                    returns.
 *****************************************************************************/
void evel_mobile_gtp_metrics_act_by_set(MOBILE_GTP_PER_FLOW_METRICS * metrics,
                                        const char * const act_by)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(metrics != NULL);
  assert(act_by != NULL);

  evel_set_option_string(&metrics->flow_activated_by,
                         act_by,
                         "Activated By");
  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Activation Time property of the Mobile GTP Per Flow metrics.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param metrics     Pointer to the Mobile GTP Per Flow Metrics.
 * @param act_time    The Activation Time to be set.  ASCIIZ string. The caller
 *                    does not need to preserve the value once the function
 *                    returns.
 *****************************************************************************/
void evel_mobile_gtp_metrics_act_time_set(
                                         MOBILE_GTP_PER_FLOW_METRICS * metrics,
                                         time_t act_time)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(metrics != NULL);
  assert(act_time > 0);

  evel_set_option_time(&metrics->flow_activation_time,
                       act_time,
                       "Activation Time");
  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Deactivated By property of the Mobile GTP Per Flow metrics.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param metrics     Pointer to the Mobile GTP Per Flow Metrics.
 * @param deact_by    The Deactivated By to be set.  ASCIIZ string. The caller
 *                    does not need to preserve the value once the function
 *                    returns.
 *****************************************************************************/
void evel_mobile_gtp_metrics_deact_by_set(
                                         MOBILE_GTP_PER_FLOW_METRICS * metrics,
                                         const char * const deact_by)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(metrics != NULL);
  assert(deact_by != NULL);

  evel_set_option_string(&metrics->flow_deactivated_by,
                         deact_by,
                         "Deactivated By");
  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the GTP Connection Status property of the Mobile GTP Per Flow metrics.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param metrics     Pointer to the Mobile GTP Per Flow Metrics.
 * @param status      The GTP Connection Status to be set.  ASCIIZ string. The
 *                    caller does not need to preserve the value once the
 *                    function returns.
 *****************************************************************************/
void evel_mobile_gtp_metrics_con_status_set(
                                         MOBILE_GTP_PER_FLOW_METRICS * metrics,
                                         const char * const status)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(metrics != NULL);
  assert(status != NULL);

  evel_set_option_string(&metrics->gtp_connection_status,
                         status,
                         "GTP Connection Status");
  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the GTP Tunnel Status property of the Mobile GTP Per Flow metrics.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param metrics     Pointer to the Mobile GTP Per Flow Metrics.
 * @param status      The GTP Tunnel Status to be set.  ASCIIZ string. The
 *                    caller does not need to preserve the value once the
 *                    function returns.
 *****************************************************************************/
void evel_mobile_gtp_metrics_tun_status_set(
                                         MOBILE_GTP_PER_FLOW_METRICS * metrics,
                                         const char * const status)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(metrics != NULL);
  assert(status != NULL);

  evel_set_option_string(&metrics->gtp_tunnel_status,
                         status,
                         "GTP Tunnel Status");
  EVEL_EXIT();
}

/**************************************************************************//**
 * Set an IP Type-of-Service count property of the Mobile GTP Per Flow metrics.
 *
 * @param metrics     Pointer to the Mobile GTP Per Flow Metrics.
 * @param index       The index of the IP Type-of-Service.
 * @param count       The count.
 *****************************************************************************/
void evel_mobile_gtp_metrics_iptos_set(MOBILE_GTP_PER_FLOW_METRICS * metrics,
                                       int index,
                                       int count)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(metrics != NULL);
  assert(index >= 0);
  assert(index < EVEL_TOS_SUPPORTED);
  assert(count >= 0);
  assert(count <= 255);

  EVEL_DEBUG("IP Type-of-Service %d", index);
  evel_set_option_int(&metrics->ip_tos_counts[index],
                      count,
                      "IP Type-of-Service");
  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Large Packet Round-Trip Time property of the Mobile GTP Per Flow
 * Metrics.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param metrics     Pointer to the Mobile GTP Per Flow Metrics.
 * @param rtt         The Large Packet Round-Trip Time to be set.
 *****************************************************************************/
void evel_mobile_gtp_metrics_large_pkt_rtt_set(
                                         MOBILE_GTP_PER_FLOW_METRICS * metrics,
                                         int rtt)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(metrics != NULL);
  assert(rtt >= 0);

  evel_set_option_int(&metrics->large_packet_rtt,
                      rtt,
                      "Large Packet Round-Trip Time");
  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Large Packet Threshold property of the Mobile GTP Per Flow Metrics.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param metrics     Pointer to the Mobile GTP Per Flow Metrics.
 * @param threshold   The Large Packet Threshold to be set.
 *****************************************************************************/
void evel_mobile_gtp_metrics_large_pkt_thresh_set(
                                         MOBILE_GTP_PER_FLOW_METRICS * metrics,
                                         double threshold)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(metrics != NULL);
  assert(threshold >= 0.0);

  evel_set_option_double(&metrics->large_packet_threshold,
                         threshold,
                         "Large Packet Threshold");
  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Max Receive Bit Rate property of the Mobile GTP Per Flow Metrics.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param metrics     Pointer to the Mobile GTP Per Flow Metrics.
 * @param rate        The Max Receive Bit Rate to be set.
 *****************************************************************************/
void evel_mobile_gtp_metrics_max_rcv_bit_rate_set(
                                         MOBILE_GTP_PER_FLOW_METRICS * metrics,
                                         int rate)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(metrics != NULL);
  assert(rate >= 0);

  evel_set_option_int(&metrics->max_receive_bit_rate,
                      rate,
                      "Max Receive Bit Rate");
  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Max Transmit Bit Rate property of the Mobile GTP Per Flow Metrics.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param metrics     Pointer to the Mobile GTP Per Flow Metrics.
 * @param rate        The Max Transmit Bit Rate to be set.
 *****************************************************************************/
void evel_mobile_gtp_metrics_max_trx_bit_rate_set(
                                         MOBILE_GTP_PER_FLOW_METRICS * metrics,
                                         int rate)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(metrics != NULL);
  assert(rate >= 0);

  evel_set_option_int(&metrics->max_transmit_bit_rate,
                      rate,
                      "Max Transmit Bit Rate");
  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Number of GTP Echo Failures property of the Mobile GTP Per Flow
 * Metrics.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param metrics     Pointer to the Mobile GTP Per Flow Metrics.
 * @param num         The Number of GTP Echo Failures to be set.
 *****************************************************************************/
void evel_mobile_gtp_metrics_num_echo_fail_set(
                                         MOBILE_GTP_PER_FLOW_METRICS * metrics,
                                         int num)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(metrics != NULL);
  assert(num >= 0);

  evel_set_option_int(&metrics->num_gtp_echo_failures,
                      num,
                      "Number of GTP Echo Failures");
  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Number of GTP Tunnel Errors property of the Mobile GTP Per Flow
 * Metrics.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param metrics     Pointer to the Mobile GTP Per Flow Metrics.
 * @param num         The Number of GTP Tunnel Errors to be set.
 *****************************************************************************/
void evel_mobile_gtp_metrics_num_tun_fail_set(
                                         MOBILE_GTP_PER_FLOW_METRICS * metrics,
                                         int num)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(metrics != NULL);
  assert(num >= 0);

  evel_set_option_int(&metrics->num_gtp_tunnel_errors,
                      num,
                      "Number of GTP Tunnel Errors");
  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Number of HTTP Errors property of the Mobile GTP Per Flow Metrics.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param metrics     Pointer to the Mobile GTP Per Flow Metrics.
 * @param num         The Number of HTTP Errors to be set.
 *****************************************************************************/
void evel_mobile_gtp_metrics_num_http_errors_set(
                                         MOBILE_GTP_PER_FLOW_METRICS * metrics,
                                         int num)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(metrics != NULL);
  assert(num >= 0);

  evel_set_option_int(&metrics->num_http_errors,
                      num,
                      "Number of HTTP Errors");
  EVEL_EXIT();
}

/**************************************************************************//**
 * Add a TCP flag count to the metrics.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param metrics       Pointer to the Mobile GTP Per Flow Metrics.
 * @param tcp_flag      The TCP flag to be updated.
 * @param count         The associated flag count, which must be nonzero.
 *****************************************************************************/
void evel_mobile_gtp_metrics_tcp_flag_count_add(
                                         MOBILE_GTP_PER_FLOW_METRICS * metrics,
                                         const EVEL_TCP_FLAGS tcp_flag,
                                         const int count)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(metrics != NULL);
  assert(tcp_flag >= 0);
  assert(tcp_flag < EVEL_MAX_TCP_FLAGS);
  assert(count >= 0);

  EVEL_DEBUG("TCP Flag: %d", tcp_flag);
  evel_set_option_int(&metrics->tcp_flag_counts[tcp_flag],
                      count,
                      "TCP flag");
  EVEL_EXIT();
}

/**************************************************************************//**
 * Add a QCI COS count to the metrics.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param metrics       Pointer to the Mobile GTP Per Flow Metrics.
 * @param qci_cos       The QCI COS count to be updated.
 * @param count         The associated QCI COS count.
 *****************************************************************************/
void evel_mobile_gtp_metrics_qci_cos_count_add(
                                         MOBILE_GTP_PER_FLOW_METRICS * metrics,
                                         const EVEL_QCI_COS_TYPES qci_cos,
                                         const int count)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(metrics != NULL);
  assert(qci_cos >= 0);
  assert(qci_cos < EVEL_MAX_QCI_COS_TYPES);
  assert(count >= 0);

  EVEL_DEBUG("QCI COS: %d", qci_cos);
  evel_set_option_int(&metrics->qci_cos_counts[qci_cos],
                      count,
                      "QCI COS");
  EVEL_EXIT();
}

/**************************************************************************//**
 * Encode the Mobile Flow GTP Per Flow Metrics as a JSON object.
 *
 * @param jbuf          Pointer to working ::EVEL_JSON_BUFFER.
 * @param metrics       Pointer to the ::EVENT_MOBILE_FLOW to encode.
 * @returns Number of bytes actually written.
 *****************************************************************************/
void evel_json_encode_mobile_flow_gtp_flow_metrics(
                                        EVEL_JSON_BUFFER * jbuf,
                                        MOBILE_GTP_PER_FLOW_METRICS * metrics)
{
  int index;
  bool found_ip_tos;
  bool found_tcp_flag;
  bool found_qci_cos;

  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(jbuf != NULL);
  assert(metrics != NULL);

  evel_json_open_named_object(jbuf, "gtpPerFlowMetrics");

  /***************************************************************************/
  /* Mandatory parameters.                                                   */
  /***************************************************************************/
  evel_enc_kv_double(jbuf, "avgBitErrorRate", metrics->avg_bit_error_rate);
  evel_enc_kv_double(
    jbuf, "avgPacketDelayVariation", metrics->avg_packet_delay_variation);
  evel_enc_kv_int(jbuf, "avgPacketLatency", metrics->avg_packet_latency);
  evel_enc_kv_int(
    jbuf, "avgReceiveThroughput", metrics->avg_receive_throughput);
  evel_enc_kv_int(
    jbuf, "avgTransmitThroughput", metrics->avg_transmit_throughput);
  evel_enc_kv_int(jbuf, "flowActivationEpoch", metrics->flow_activation_epoch);
  evel_enc_kv_int(
    jbuf, "flowActivationMicrosec", metrics->flow_activation_microsec);
  evel_enc_kv_int(
    jbuf, "flowDeactivationEpoch", metrics->flow_deactivation_epoch);
  evel_enc_kv_int(
    jbuf, "flowDeactivationMicrosec", metrics->flow_deactivation_microsec);
  evel_enc_kv_time(
    jbuf, "flowDeactivationTime", &metrics->flow_deactivation_time);
  evel_enc_kv_string(jbuf, "flowStatus", metrics->flow_status);
  evel_enc_kv_int(
    jbuf, "maxPacketDelayVariation", metrics->max_packet_delay_variation);
  evel_enc_kv_int(
    jbuf, "numActivationFailures", metrics->num_activation_failures);
  evel_enc_kv_int(jbuf, "numBitErrors", metrics->num_bit_errors);
  evel_enc_kv_int(jbuf, "numBytesReceived", metrics->num_bytes_received);
  evel_enc_kv_int(jbuf, "numBytesTransmitted", metrics->num_bytes_transmitted);
  evel_enc_kv_int(jbuf, "numDroppedPackets", metrics->num_dropped_packets);
  evel_enc_kv_int(jbuf, "numL7BytesReceived", metrics->num_l7_bytes_received);
  evel_enc_kv_int(
    jbuf, "numL7BytesTransmitted", metrics->num_l7_bytes_transmitted);
  evel_enc_kv_int(jbuf, "numLostPackets", metrics->num_lost_packets);
  evel_enc_kv_int(
    jbuf, "numOutOfOrderPackets", metrics->num_out_of_order_packets);
  evel_enc_kv_int(jbuf, "numPacketErrors", metrics->num_packet_errors);
  evel_enc_kv_int(jbuf,
                  "numPacketsReceivedExclRetrans",
                  metrics->num_packets_received_excl_retrans);
  evel_enc_kv_int(jbuf,
                  "numPacketsReceivedInclRetrans",
                  metrics->num_packets_received_incl_retrans);
  evel_enc_kv_int(jbuf,
                  "numPacketsTransmittedInclRetrans",
                  metrics->num_packets_transmitted_incl_retrans);
  evel_enc_kv_int(jbuf, "numRetries", metrics->num_retries);
  evel_enc_kv_int(jbuf, "numTimeouts", metrics->num_timeouts);
  evel_enc_kv_int(jbuf,
                  "numTunneledL7BytesReceived",
                  metrics->num_tunneled_l7_bytes_received);
  evel_enc_kv_int(jbuf, "roundTripTime", metrics->round_trip_time);
  evel_enc_kv_int(jbuf, "timeToFirstByte", metrics->time_to_first_byte);

  /***************************************************************************/
  /* Optional parameters.                                                    */
  /***************************************************************************/
  found_ip_tos = false;
  for (index = 0; index < EVEL_TOS_SUPPORTED; index++)
  {
    if (metrics->ip_tos_counts[index].is_set)
    {
      found_ip_tos = true;
      break;
    }
  }

  if (found_ip_tos)
  {
    evel_json_open_named_list(jbuf, "ipTosCountList");
    for (index = 0; index < EVEL_TOS_SUPPORTED; index++)
    {
      if (metrics->ip_tos_counts[index].is_set)
      {
        evel_enc_list_item(jbuf,
                           "[\"%d\", %d]",
                           index,
                           metrics->ip_tos_counts[index].value);
      }
    }
    evel_json_close_list(jbuf);
  }

  if (found_ip_tos)
  {
    evel_json_open_named_list(jbuf, "ipTosList");
    for (index = 0; index < EVEL_TOS_SUPPORTED; index++)
    {
      if (metrics->ip_tos_counts[index].is_set)
      {
        evel_enc_list_item(jbuf, "\"%d\"", index);
      }
    }
    evel_json_close_list(jbuf);
  }

  /***************************************************************************/
  /* Make some compile-time assertions about EVEL_TCP_FLAGS.  If you update  */
  /* these, make sure you update evel_tcp_flag_strings to match the enum.    */
  /***************************************************************************/
  EVEL_CT_ASSERT(EVEL_TCP_NS == 0);
  EVEL_CT_ASSERT(EVEL_TCP_CWR == 1);
  EVEL_CT_ASSERT(EVEL_TCP_ECE == 2);
  EVEL_CT_ASSERT(EVEL_TCP_URG == 3);
  EVEL_CT_ASSERT(EVEL_TCP_ACK == 4);
  EVEL_CT_ASSERT(EVEL_TCP_PSH == 5);
  EVEL_CT_ASSERT(EVEL_TCP_RST == 6);
  EVEL_CT_ASSERT(EVEL_TCP_SYN == 7);
  EVEL_CT_ASSERT(EVEL_TCP_FIN == 8);
  EVEL_CT_ASSERT(EVEL_MAX_TCP_FLAGS == 9);

  found_tcp_flag = false;
  for (index = 0; index < EVEL_MAX_TCP_FLAGS; index++)
  {
    if (metrics->tcp_flag_counts[index].is_set)
    {
      found_tcp_flag = true;
      break;
    }
  }

  if (found_tcp_flag)
  {
    evel_json_open_named_list(jbuf, "tcpFlagList");
    for (index = 0; index < EVEL_MAX_TCP_FLAGS; index++)
    {
      if (metrics->tcp_flag_counts[index].is_set)
      {
        evel_enc_list_item(jbuf,
                           "\"%s\"",
                           evel_tcp_flag_strings[index]);
      }
    }
    evel_json_close_list(jbuf);
  }

  if (found_tcp_flag)
  {
    evel_json_open_named_list(jbuf, "tcpFlagCountList");
    for (index = 0; index < EVEL_MAX_TCP_FLAGS; index++)
    {
      if (metrics->tcp_flag_counts[index].is_set)
      {
        evel_enc_list_item(jbuf,
                           "[\"%s\", %d]",
                           evel_tcp_flag_strings[index],
                           metrics->tcp_flag_counts[index].value);
      }
    }
    evel_json_close_list(jbuf);
  }

  /***************************************************************************/
  /* Make some compile-time assertions about EVEL_QCI_COS_TYPES.  If you     */
  /* update these, make sure you update evel_qci_cos_strings to match the    */
  /* enum.                                                                   */
  /***************************************************************************/
  EVEL_CT_ASSERT(EVEL_QCI_COS_UMTS_CONVERSATIONAL ==0);
  EVEL_CT_ASSERT(EVEL_QCI_COS_UMTS_STREAMING == 1);
  EVEL_CT_ASSERT(EVEL_QCI_COS_UMTS_INTERACTIVE == 2);
  EVEL_CT_ASSERT(EVEL_QCI_COS_UMTS_BACKGROUND == 3);
  EVEL_CT_ASSERT(EVEL_QCI_COS_LTE_1 == 4);
  EVEL_CT_ASSERT(EVEL_QCI_COS_LTE_2 == 5);
  EVEL_CT_ASSERT(EVEL_QCI_COS_LTE_3 == 6);
  EVEL_CT_ASSERT(EVEL_QCI_COS_LTE_4 == 7);
  EVEL_CT_ASSERT(EVEL_QCI_COS_LTE_65 == 8);
  EVEL_CT_ASSERT(EVEL_QCI_COS_LTE_66 == 9);
  EVEL_CT_ASSERT(EVEL_QCI_COS_LTE_5 == 10);
  EVEL_CT_ASSERT(EVEL_QCI_COS_LTE_6 == 11);
  EVEL_CT_ASSERT(EVEL_QCI_COS_LTE_7 == 12);
  EVEL_CT_ASSERT(EVEL_QCI_COS_LTE_8 == 13);
  EVEL_CT_ASSERT(EVEL_QCI_COS_LTE_9 == 14);
  EVEL_CT_ASSERT(EVEL_QCI_COS_LTE_69 == 15);
  EVEL_CT_ASSERT(EVEL_QCI_COS_LTE_70 == 16);
  EVEL_CT_ASSERT(EVEL_MAX_QCI_COS_TYPES == 17);

  found_qci_cos = false;
  for (index = 0; index < EVEL_MAX_QCI_COS_TYPES; index++)
  {
    if (metrics->qci_cos_counts[index].is_set)
    {
      found_qci_cos = true;
      break;
    }
  }

  if (found_qci_cos)
  {
    evel_json_open_named_list(jbuf, "mobileQciCosList");
    for (index = 0; index < EVEL_MAX_QCI_COS_TYPES; index++)
    {
      if (metrics->qci_cos_counts[index].is_set)
      {
        evel_enc_list_item(jbuf,
                           "\"%s\"",
                           evel_qci_cos_strings[index]);
      }
    }
    evel_json_close_list(jbuf);
  }

  if (found_qci_cos)
  {
    evel_json_open_named_list(jbuf, "mobileQciCosCountList");
    for (index = 0; index < EVEL_MAX_QCI_COS_TYPES; index++)
    {
      if (metrics->qci_cos_counts[index].is_set)
      {
        evel_enc_list_item(jbuf,
                           "[\"%s\", %d]",
                           evel_qci_cos_strings[index],
                           metrics->qci_cos_counts[index].value);
      }
    }
    evel_json_close_list(jbuf);
  }

  evel_enc_kv_opt_int(
    jbuf, "durConnectionFailedStatus", &metrics->dur_connection_failed_status);
  evel_enc_kv_opt_int(
    jbuf, "durTunnelFailedStatus", &metrics->dur_tunnel_failed_status);
  evel_enc_kv_opt_string(jbuf, "flowActivatedBy", &metrics->flow_activated_by);
  evel_enc_kv_opt_time(
    jbuf, "flowActivationTime", &metrics->flow_activation_time);
  evel_enc_kv_opt_string(
    jbuf, "flowDeactivatedBy", &metrics->flow_deactivated_by);
  evel_enc_kv_opt_string(
    jbuf, "gtpConnectionStatus", &metrics->gtp_connection_status);
  evel_enc_kv_opt_string(jbuf, "gtpTunnelStatus", &metrics->gtp_tunnel_status);
  evel_enc_kv_opt_int(jbuf, "largePacketRtt", &metrics->large_packet_rtt);
  evel_enc_kv_opt_double(
    jbuf, "largePacketThreshold", &metrics->large_packet_threshold);
  evel_enc_kv_opt_int(
    jbuf, "maxReceiveBitRate", &metrics->max_receive_bit_rate);
  evel_enc_kv_opt_int(
    jbuf, "maxTransmitBitRate", &metrics->max_transmit_bit_rate);
  evel_enc_kv_opt_int(
    jbuf, "numGtpEchoFailures", &metrics->num_gtp_echo_failures);
  evel_enc_kv_opt_int(
    jbuf, "numGtpTunnelErrors", &metrics->num_gtp_tunnel_errors);
  evel_enc_kv_opt_int(jbuf, "numHttpErrors", &metrics->num_http_errors);

  evel_json_close_object(jbuf);

  EVEL_EXIT();
}

/**************************************************************************//**
 * Free a Mobile GTP Per Flow Metrics.
 *
 * Free off the Mobile GTP Per Flow Metrics supplied.  Will free all the
 * contained allocated memory.
 *
 * @note It does not free the Mobile GTP Per Flow Metrics itself, since that
 * may be part of a larger structure.
 *****************************************************************************/
void evel_free_mobile_gtp_flow_metrics(MOBILE_GTP_PER_FLOW_METRICS * metrics)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(metrics != NULL);

  /***************************************************************************/
  /* Free all internal strings.                                              */
  /***************************************************************************/
  free(metrics->flow_status);

  evel_free_option_string(&metrics->flow_activated_by);
  evel_free_option_string(&metrics->flow_deactivated_by);
  evel_free_option_string(&metrics->gtp_connection_status);
  evel_free_option_string(&metrics->gtp_tunnel_status);

  EVEL_EXIT();
}
