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
 * Implementation of EVEL functions relating to the Voice Quality.
 *****************************************************************************/

#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include "evel.h"
#include "evel_internal.h"
#include "evel_throttle.h"

/**************************************************************************//**
 * Create a new voice quality event.
 *
 * @note    The mandatory fields on the Voice Quality must be supplied to this 
 *          factory function and are immutable once set.  Optional fields have 
 *          explicit setter functions, but again values may only be set once 
 *          so that the Voice Quality has immutable properties.
 * @param event_name    Unique Event Name
 * @param event_id    A universal identifier of the event for analysis etc.
 * @param   calleeSideCodec         Callee codec for the call.
 * @param   callerSideCodec         Caller codec for the call.
 * @param   correlator              Constant across all events on this call.
 * @param   midCallRtcp             Base64 encoding of the binary RTCP data
 *                                  (excluding Eth/IP/UDP headers).
 * @param   vendorVnfNameFields     Vendor, VNF and VfModule names.
 * @returns pointer to the newly manufactured ::EVENT_VOICE_QUALITY.  If the 
 *          event is not used (i.e. posted) it must be released using
            ::evel_free_voice_quality.
 * @retval  NULL  Failed to create the event.
 *****************************************************************************/
EVENT_VOICE_QUALITY * evel_new_voice_quality(const char* ev_name, const char *ev_id,
					const char * const calleeSideCodec,
    const char * const callerSideCodec, const char * const correlator,
    const char * const midCallRtcp, const char * const vendorName) {
    
    bool inError = false;
    EVENT_VOICE_QUALITY *voiceQuality = NULL;
    EVEL_ENTER();

    /***************************************************************************/
    /* Check preconditions.                                                    */
    /***************************************************************************/
    assert(calleeSideCodec != NULL);
    assert(callerSideCodec != NULL);
    assert(correlator != NULL);
    assert(midCallRtcp != NULL);
    assert(vendorName != NULL);

    /***************************************************************************/
    /* Allocate the Voice Quality.                                                     */
    /***************************************************************************/
    voiceQuality = malloc(sizeof(EVENT_VOICE_QUALITY));
    
    if (voiceQuality == NULL)
    {
        log_error_state("Out of memory");
        inError = true;
    }

    //Only in case of successful allocation initialize data.
    if (inError == false) {
        memset(voiceQuality, 0, sizeof(EVENT_VOICE_QUALITY));
        EVEL_DEBUG("New Voice Quality is at %lp", voiceQuality);

        /***************************************************************************/
        /* Initialize the header & the fault fields.  Optional integer values are   */
        /* initialized as 0.                                                        */
        /***************************************************************************/
        evel_init_header_nameid(&voiceQuality->header,ev_name,ev_id);
        voiceQuality->header.event_domain = EVEL_DOMAIN_VOICE_QUALITY;
        voiceQuality->major_version = EVEL_VOICEQ_MAJOR_VERSION;
        voiceQuality->minor_version = EVEL_VOICEQ_MINOR_VERSION;

        voiceQuality->calleeSideCodec = strdup(calleeSideCodec);
        voiceQuality->callerSideCodec = strdup(callerSideCodec);
        voiceQuality->correlator = strdup(correlator);
        voiceQuality->midCallRtcp = strdup(midCallRtcp);
        evel_init_vendor_field(&voiceQuality->vendorVnfNameFields, vendorName);
        dlist_initialize(&voiceQuality->additionalInformation);
        voiceQuality->endOfCallVqmSummaries = NULL;
        evel_init_option_string(&voiceQuality->phoneNumber);
    }

    EVEL_EXIT();
    return voiceQuality;

}

/**************************************************************************//**
 * Add an additional value name/value pair to the Voice Quality.
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
void evel_voice_quality_addl_info_add(EVENT_VOICE_QUALITY * voiceQ, char * name, char * value) {
    VOICE_QUALITY_ADDL_INFO * addlInfo = NULL;
    EVEL_ENTER();

    /***************************************************************************/
    /* Check preconditions.                                                    */
    /***************************************************************************/
    assert(voiceQ != NULL);
    assert(voiceQ->header.event_domain == EVEL_DOMAIN_VOICE_QUALITY);
    assert(name != NULL);
    assert(value != NULL);

    EVEL_DEBUG("Adding name=%s value=%s", name, value);
    addlInfo = malloc(sizeof(VOICE_QUALITY_ADDL_INFO));
    assert(addlInfo != NULL);
    memset(addlInfo, 0, sizeof(VOICE_QUALITY_ADDL_INFO));
    addlInfo->name = strdup(name);
    addlInfo->value = strdup(value);
    assert(addlInfo->name != NULL);
    assert(addlInfo->value != NULL);

    dlist_push_last(&voiceQ->additionalInformation, addlInfo);

    EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Callee side codec for Call for domain Voice Quality
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param voiceQuality              Pointer to the Voice Quality Event.
 * @param calleeCodecForCall        The Callee Side Codec to be set.  ASCIIZ 
 *                                  string. The caller does not need to 
 *                                  preserve the value once the function
 *                                  returns.
 *****************************************************************************/
void evel_voice_quality_callee_codec_set(EVENT_VOICE_QUALITY * voiceQuality,
    const char * const calleeCodecForCall) {
    EVEL_ENTER();

    /***************************************************************************/
    /* Check preconditions.                                                    */
    /***************************************************************************/
    assert(voiceQuality != NULL);
    assert(voiceQuality->header.event_domain == EVEL_DOMAIN_VOICE_QUALITY);
    assert(calleeCodecForCall != NULL);

    voiceQuality->calleeSideCodec = strdup(calleeCodecForCall);

    EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Caller side codec for Call for domain Voice Quality
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param voiceQuality              Pointer to the Voice Quality Event.
 * @param callerCodecForCall        The Caller Side Codec to be set.  ASCIIZ 
 *                                  string. The caller does not need to 
 *                                  preserve the value once the function
 *                                  returns.
 *****************************************************************************/
void evel_voice_quality_caller_codec_set(EVENT_VOICE_QUALITY * voiceQuality,
    const char * const callerCodecForCall) {
    EVEL_ENTER();

    /***************************************************************************/
    /* Check preconditions.                                                    */
    /***************************************************************************/
    assert(voiceQuality != NULL);
    assert(voiceQuality->header.event_domain == EVEL_DOMAIN_VOICE_QUALITY);
    assert(callerCodecForCall != NULL);

    voiceQuality->calleeSideCodec = strdup(callerCodecForCall);

    EVEL_EXIT();
}

/**************************************************************************//**
 * Set the correlator for domain Voice Quality
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param voiceQuality              Pointer to the Voice Quality Event.
 * @param correlator                The correlator value to be set.  ASCIIZ 
 *                                  string. The caller does not need to 
 *                                  preserve the value once the function
 *                                  returns.
 *****************************************************************************/
void evel_voice_quality_correlator_set(EVENT_VOICE_QUALITY * voiceQuality,
    const char * const vCorrelator) {
    EVEL_ENTER();

    /***************************************************************************/
    /* Check preconditions.                                                    */
    /***************************************************************************/
    assert(voiceQuality != NULL);
    assert(voiceQuality->header.event_domain == EVEL_DOMAIN_VOICE_QUALITY);
    assert(vCorrelator != NULL);

    voiceQuality->correlator = strdup(vCorrelator);

    EVEL_EXIT();
}

/**************************************************************************//**
 * Set the RTCP Call Data for domain Voice Quality
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param voiceQuality              Pointer to the Voice Quality Event.
 * @param rtcpCallData              The RTCP Call Data to be set.  ASCIIZ 
 *                                  string. The caller does not need to 
 *                                  preserve the value once the function
 *                                  returns.
 *****************************************************************************/
void evel_voice_quality_rtcp_data_set(EVENT_VOICE_QUALITY * voiceQuality,
    const char * const rtcpCallData) {
    EVEL_ENTER();

    /***************************************************************************/
    /* Check preconditions.                                                    */
    /***************************************************************************/
    assert(voiceQuality != NULL);
    assert(voiceQuality->header.event_domain == EVEL_DOMAIN_VOICE_QUALITY);
    assert(rtcpCallData != NULL);

    voiceQuality->midCallRtcp = strdup(rtcpCallData);

    EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Vendor VNF Name fields for domain Voice Quality
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param voiceQuality              Pointer to the Voice Quality Event.
 * @param modulename                The Vendor, VNF and VfModule names to be set.   
 *                                  ASCIIZ string. The caller does not need to 
 *                                  preserve the value once the function
 *                                  returns.
 *****************************************************************************/
void evel_voice_quality_vnfmodule_name_set(EVENT_VOICE_QUALITY * voiceQuality,
    const char * const module_name) {
    EVEL_ENTER();

    /***************************************************************************/
    /* Check preconditions.                                                    */
    /***************************************************************************/
    assert(voiceQuality != NULL);
    assert(voiceQuality->header.event_domain == EVEL_DOMAIN_VOICE_QUALITY);
    assert(module_name != NULL);

    evel_vendor_field_module_set(&voiceQuality->vendorVnfNameFields, module_name);

    EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Vendor VNF Name fields for domain Voice Quality
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param voiceQuality              Pointer to the Voice Quality Event.
 * @param modulename                The Vendor, VNF and VfModule names to be set.   
 *                                  ASCIIZ string. The caller does not need to 
 *                                  preserve the value once the function
 *                                  returns.
 *****************************************************************************/
void evel_voice_quality_vnfname_set(EVENT_VOICE_QUALITY * voiceQuality,
    const char * const vnfname) {
    EVEL_ENTER();

    /***************************************************************************/
    /* Check preconditions.                                                    */
    /***************************************************************************/
    assert(voiceQuality != NULL);
    assert(voiceQuality->header.event_domain == EVEL_DOMAIN_VOICE_QUALITY);
    assert(vnfname != NULL);

    evel_vendor_field_vnfname_set(&voiceQuality->vendorVnfNameFields, vnfname);

    EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Phone Number associated with the Correlator for domain Voice Quality
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param voiceQuality              Pointer to the Voice Quality Event.
 * @param calleeCodecForCall        The Phone Number to be set.  ASCIIZ 
 *                                  string. The caller does not need to 
 *                                  preserve the value once the function
 *                                  returns.
 *****************************************************************************/
void evel_voice_quality_phone_number_set(EVENT_VOICE_QUALITY * voiceQuality,
    const char * const phoneNumber) {
    EVEL_ENTER();

    /***************************************************************************/
    /* Check preconditions.                                                    */
    /***************************************************************************/
    assert(voiceQuality != NULL);
    assert(voiceQuality->header.event_domain == EVEL_DOMAIN_VOICE_QUALITY);
    assert(phoneNumber != NULL);

    evel_set_option_string(&voiceQuality->phoneNumber, phoneNumber, "Phone_Number");

    EVEL_EXIT();
}

/**************************************************************************//**
 * Add an End of Call Voice Quality Metrices

 * The adjacencyName and endpointDescription is null delimited ASCII string.  
 * The library takes a copy so the caller does not have to preserve values
 * after the function returns.
 *
 * @param voiceQuality     Pointer to the measurement.
 * @param adjacencyName                     Adjacency name
 * @param endpointDescription               Enumeration: ‘Caller’, ‘Callee’.
 * @param endpointJitter                    Endpoint jitter
 * @param endpointRtpOctetsDiscarded        Endpoint RTP octets discarded.
 * @param endpointRtpOctetsReceived         Endpoint RTP octets received.
 * @param endpointRtpOctetsSent             Endpoint RTP octets sent
 * @param endpointRtpPacketsDiscarded       Endpoint RTP packets discarded.
 * @param endpointRtpPacketsReceived        Endpoint RTP packets received.
 * @param endpointRtpPacketsSent            Endpoint RTP packets sent.
 * @param localJitter                       Local jitter.
 * @param localRtpOctetsDiscarded           Local RTP octets discarded.
 * @param localRtpOctetsReceived            Local RTP octets received.
 * @param localRtpOctetsSent                Local RTP octets sent.
 * @param localRtpPacketsDiscarded          Local RTP packets discarded.
 * @param localRtpPacketsReceived           Local RTP packets received.
 * @param localRtpPacketsSent               Local RTP packets sent.
 * @param mosCqe                            Decimal range from 1 to 5
 *                                          (1 decimal place)
 * @param packetsLost                       No  Packets lost
 * @param packetLossPercent                 Calculated percentage packet loss 
 * @param rFactor                           rFactor from 0 to 100
 * @param roundTripDelay                    Round trip delay in milliseconds
 *****************************************************************************/
void evel_voice_quality_end_metrics_add(EVENT_VOICE_QUALITY * voiceQuality,
    const char * adjacencyName, EVEL_SERVICE_ENDPOINT_DESC endpointDescription,
    int endpointJitter,
    int endpointRtpOctetsDiscarded,
    int endpointRtpOctetsReceived,
    int endpointRtpOctetsSent,
    int endpointRtpPacketsDiscarded,
    int endpointRtpPacketsReceived,
    int endpointRtpPacketsSent,
    int localJitter,
    int localRtpOctetsDiscarded,
    int localRtpOctetsReceived,
    int localRtpOctetsSent,
    int localRtpPacketsDiscarded,
    int localRtpPacketsReceived,
    int localRtpPacketsSent,
    int mosCqe,
    int packetsLost,
    int packetLossPercent,
    int rFactor,
    int roundTripDelay) {
    
    END_OF_CALL_VOICE_QUALITY_METRICS * vQMetrices = NULL;
    EVEL_ENTER();

    /***************************************************************************/
    /* Check assumptions.                                                      */
    /***************************************************************************/
    assert(voiceQuality != NULL);
    assert(voiceQuality->header.event_domain == EVEL_DOMAIN_VOICE_QUALITY);
    assert(adjacencyName != NULL);
    assert(endpointDescription >= 0);
    assert(mosCqe >= 1 && mosCqe <= 5);
    assert(rFactor >= 0 && rFactor <= 100);
    assert(voiceQuality->endOfCallVqmSummaries == NULL);
    
    /***************************************************************************/
    /* Allocate a container for the value and push onto the list.              */
    /***************************************************************************/
    EVEL_DEBUG("Adding adjacencyName=%s endpointDescription=%d", adjacencyName, endpointDescription);
    vQMetrices = malloc(sizeof(END_OF_CALL_VOICE_QUALITY_METRICS));
    assert(vQMetrices != NULL);
    memset(vQMetrices, 0, sizeof(END_OF_CALL_VOICE_QUALITY_METRICS));

    vQMetrices->adjacencyName = strdup(adjacencyName);
    vQMetrices->endpointDescription = evel_service_endpoint_desc(endpointDescription);

    evel_set_option_int(&vQMetrices->endpointJitter, endpointJitter, "Endpoint jitter");
    evel_set_option_int(&vQMetrices->endpointRtpOctetsDiscarded, endpointRtpOctetsDiscarded, "Endpoint RTP octets discarded");
    evel_set_option_int(&vQMetrices->endpointRtpOctetsReceived, endpointRtpOctetsReceived, "Endpoint RTP octets received");
    evel_set_option_int(&vQMetrices->endpointRtpOctetsSent, endpointRtpOctetsSent, "Endpoint RTP octets sent");
    evel_set_option_int(&vQMetrices->endpointRtpPacketsDiscarded, endpointRtpPacketsDiscarded, "Endpoint RTP packets discarded");
    evel_set_option_int(&vQMetrices->endpointRtpPacketsReceived, endpointRtpPacketsReceived, "Endpoint RTP packets received");
    evel_set_option_int(&vQMetrices->endpointRtpPacketsSent, endpointRtpPacketsSent, "Endpoint RTP packets sent");
    evel_set_option_int(&vQMetrices->localJitter, localJitter, "Local jitter");
    evel_set_option_int(&vQMetrices->localRtpOctetsDiscarded, localRtpOctetsDiscarded, "Local RTP octets discarded");
    evel_set_option_int(&vQMetrices->localRtpOctetsReceived, localRtpOctetsReceived, "Local RTP octets received");
    evel_set_option_int(&vQMetrices->localRtpOctetsSent, localRtpOctetsSent, "Local RTP octets sent");
    evel_set_option_int(&vQMetrices->localRtpPacketsDiscarded, localRtpPacketsDiscarded, "Local RTP packets discarded");
    evel_set_option_int(&vQMetrices->localRtpPacketsReceived, localRtpPacketsReceived, "Local RTP packets received");
    evel_set_option_int(&vQMetrices->localRtpPacketsSent, localRtpPacketsSent, "Local RTP packets sent");
    evel_set_option_int(&vQMetrices->mosCqe, mosCqe, "Decimal range from 1 to 5 (1 decimal place)");
    evel_set_option_int(&vQMetrices->packetsLost, packetsLost, "Packets lost");
    evel_set_option_int(&vQMetrices->packetLossPercent, packetLossPercent, "Calculated percentage packet loss");
    evel_set_option_int(&vQMetrices->rFactor, rFactor, "rFactor ");
    evel_set_option_int(&vQMetrices->roundTripDelay, roundTripDelay, "Round trip delay in milliseconds ");

    voiceQuality->endOfCallVqmSummaries = vQMetrices;

    EVEL_EXIT();
}

/**************************************************************************//**
 * Encode the Voce Quality in JSON according to AT&T's schema for the voice
 * quality type.
 *
 * @param jbuf          Pointer to the ::EVEL_JSON_BUFFER to encode into.
 * @param event         Pointer to the ::EVENT_HEADER to encode.
 *****************************************************************************/
void evel_json_encode_voice_quality(EVEL_JSON_BUFFER * jbuf,
                            EVENT_VOICE_QUALITY * event)
{
  VOICE_QUALITY_ADDL_INFO * addlInfo = NULL;
  DLIST_ITEM * addlInfoItem = NULL;

  END_OF_CALL_VOICE_QUALITY_METRICS * vQMetrics = NULL;
  DLIST_ITEM * vQMetricsItem = NULL;

  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(event != NULL);
  assert(event->header.event_domain == EVEL_DOMAIN_VOICE_QUALITY);

  evel_json_encode_header(jbuf, &event->header);
  evel_json_open_named_object(jbuf, "voiceQualityFields");

  /***************************************************************************/
  /* Mandatory fields.                                                       */
  /***************************************************************************/
  evel_enc_kv_string(jbuf, "calleeSideCodec", event->calleeSideCodec);
  evel_enc_kv_string(jbuf, "callerSideCodec", event->callerSideCodec);
  evel_enc_kv_string(jbuf, "correlator", event->correlator);
  evel_enc_kv_string(jbuf, "midCallRtcp", event->midCallRtcp);
  evel_json_encode_vendor_field(jbuf, &event->vendorVnfNameFields);
  evel_enc_version(
    jbuf, "voiceQualityFieldsVersion", event->major_version, event->minor_version);

  /***************************************************************************/
  /* Optional fields.                                                        */
  /***************************************************************************/
  evel_enc_kv_opt_string(jbuf, "phoneNumber", &event->phoneNumber);
  /***************************************************************************/
  /* Checkpoint, so that we can wind back if all fields are suppressed.      */
  /***************************************************************************/
  //additionalInformation for Voice Quality
  bool item_added = false;
 
  evel_json_checkpoint(jbuf);
  if (evel_json_open_opt_named_list(jbuf, "additionalInformation"))
  {

    addlInfoItem = dlist_get_first(&event->additionalInformation);
    while (addlInfoItem != NULL)
    {
      addlInfo = (VOICE_QUALITY_ADDL_INFO*)addlInfoItem->item;
      assert(addlInfo != NULL);

      if (!evel_throttle_suppress_nv_pair(jbuf->throttle_spec,
                                          "additionalInformation",
                                          addlInfo->name))
      {
        evel_json_open_object(jbuf);
        evel_enc_kv_string(jbuf, "name", addlInfo->name);
        evel_enc_kv_string(jbuf, "value", addlInfo->value);
        evel_json_close_object(jbuf);
        item_added = true;
      }
      addlInfoItem = dlist_get_next(addlInfoItem);
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

    //endOfCallVqmSummaries
  if( event->endOfCallVqmSummaries != NULL )
  {
     evel_json_open_named_object(jbuf, "endOfCallVqmSummaries");
     vQMetrics = event->endOfCallVqmSummaries;
     assert(vQMetrics != NULL);

            if (!evel_throttle_suppress_nv_pair(jbuf->throttle_spec,
                "endOfCallVqmSummaries", vQMetrics->adjacencyName))
            {
                evel_enc_kv_string(jbuf, "adjacencyName", vQMetrics->adjacencyName);
                evel_enc_kv_string(jbuf, "endpointDescription", vQMetrics->endpointDescription);
                evel_enc_kv_opt_int(jbuf, "endpointJitter", &vQMetrics->endpointJitter);
                evel_enc_kv_opt_int(jbuf, "endpointRtpOctetsDiscarded", &vQMetrics->endpointRtpOctetsDiscarded);
                evel_enc_kv_opt_int(jbuf, "endpointRtpOctetsReceived", &vQMetrics->endpointRtpOctetsReceived);
                evel_enc_kv_opt_int(jbuf, "endpointRtpOctetsSent", &vQMetrics->endpointRtpOctetsSent);
                evel_enc_kv_opt_int(jbuf, "endpointRtpPacketsDiscarded", &vQMetrics->endpointRtpPacketsDiscarded);
                evel_enc_kv_opt_int(jbuf, "endpointRtpPacketsReceived", &vQMetrics->endpointRtpPacketsReceived);
                evel_enc_kv_opt_int(jbuf, "endpointRtpPacketsSent", &vQMetrics->endpointRtpPacketsSent);
                evel_enc_kv_opt_int(jbuf, "localJitter", &vQMetrics->localJitter);
                evel_enc_kv_opt_int(jbuf, "localRtpOctetsDiscarded", &vQMetrics->localRtpOctetsDiscarded);
                evel_enc_kv_opt_int(jbuf, "localRtpOctetsReceived", &vQMetrics->localRtpOctetsReceived);
                evel_enc_kv_opt_int(jbuf, "localRtpOctetsSent", &vQMetrics->localRtpOctetsSent);
                evel_enc_kv_opt_int(jbuf, "localRtpPacketsDiscarded", &vQMetrics->localRtpPacketsDiscarded);
                evel_enc_kv_opt_int(jbuf, "localRtpPacketsReceived", &vQMetrics->localRtpPacketsReceived);
                evel_enc_kv_opt_int(jbuf, "localRtpPacketsSent", &vQMetrics->localRtpPacketsSent);
                evel_enc_kv_opt_int(jbuf, "mosCqe", &vQMetrics->mosCqe);
                evel_enc_kv_opt_int(jbuf, "packetsLost", &vQMetrics->packetsLost);
                evel_enc_kv_opt_int(jbuf, "packetLossPercent", &vQMetrics->packetLossPercent);
                evel_enc_kv_opt_int(jbuf, "rFactor", &vQMetrics->rFactor);
                evel_enc_kv_opt_int(jbuf, "roundTripDelay", &vQMetrics->roundTripDelay);

            }

    evel_json_close_object(jbuf);
  }

  evel_json_close_object(jbuf);

  EVEL_EXIT();
}

/**************************************************************************//**
 * Free a Voice Quality.
 *
 * Free off the Voce Quality supplied.  Will free all the contained allocated
 * memory.
 *
 * @note It does not free the Voice Quality itself, since that may be part of a
 * larger structure.
 *****************************************************************************/
void evel_free_voice_quality(EVENT_VOICE_QUALITY * voiceQuality) {
    END_OF_CALL_VOICE_QUALITY_METRICS * vQMetrices = NULL;
    VOICE_QUALITY_ADDL_INFO * addlInfo = NULL;

    EVEL_ENTER();

    /***************************************************************************/
    /* Check preconditions.  As an internal API we don't allow freeing NULL    */
    /* events as we do on the public API.                                      */
    /***************************************************************************/
    assert(voiceQuality != NULL);
    assert(voiceQuality->header.event_domain == EVEL_DOMAIN_VOICE_QUALITY);

    /***************************************************************************/
    /* Free all internal strings then the header itself.                       */
    /***************************************************************************/
    
    //Additional Information
    addlInfo = dlist_pop_last(&voiceQuality->additionalInformation);
    while (addlInfo != NULL)
    {
        EVEL_DEBUG("Freeing Additional Info (%s, %s)",
            addlInfo->name,
            addlInfo->value);
        free(addlInfo->name);
        free(addlInfo->value);
        free(addlInfo);
        addlInfo = dlist_pop_last(&voiceQuality->additionalInformation);
    }

    //Summary Information
    if(voiceQuality->endOfCallVqmSummaries != NULL)
    {
        vQMetrices = voiceQuality->endOfCallVqmSummaries;
        EVEL_DEBUG("Freeing End of Call Voice Measurements Info (%s, %s)",
            vQMetrices->adjacencyName,
            vQMetrices->endpointDescription);
        free(vQMetrices->adjacencyName);
        free(vQMetrices);
    }

    //Members
    free(voiceQuality->calleeSideCodec);
    free(voiceQuality->callerSideCodec);
    free(voiceQuality->correlator);
    free(voiceQuality->midCallRtcp);
    evel_free_option_string(&voiceQuality->phoneNumber);
    evel_free_event_vendor_field(&voiceQuality->vendorVnfNameFields);

    //header
    evel_free_header(&voiceQuality->header);

    EVEL_EXIT();
}

