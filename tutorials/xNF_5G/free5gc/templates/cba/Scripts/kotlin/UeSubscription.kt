/*
 * Copyright © 2021 Orange
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package org.onap.ccsdk.cds.blueprintsprocessor.services.execution.scripts

import com.fasterxml.jackson.databind.ObjectMapper
import com.fasterxml.jackson.databind.node.ObjectNode
import com.fasterxml.jackson.module.kotlin.KotlinModule
import com.google.gson.Gson
import kotlinx.coroutines.delay
import net.minidev.json.JSONObject
import org.onap.ccsdk.cds.blueprintsprocessor.core.api.data.ExecutionServiceInput
import org.onap.ccsdk.cds.blueprintsprocessor.rest.BasicAuthRestClientProperties
import org.onap.ccsdk.cds.blueprintsprocessor.rest.service.BasicAuthRestClientService
import org.onap.ccsdk.cds.blueprintsprocessor.rest.service.BlueprintWebClientService
import org.onap.ccsdk.cds.blueprintsprocessor.services.execution.AbstractScriptComponentFunction
import org.onap.ccsdk.cds.error.catalog.core.logger
import org.slf4j.LoggerFactory
import org.springframework.http.HttpMethod
import java.lang.Thread.sleep

open class UeSubscription : AbstractScriptComponentFunction() {

    private val log = LoggerFactory.getLogger(PnfRegistration::class.java)!!

    override fun getName(): String {
        return "UeSubscription"
    }

    override suspend fun recoverNB(runtimeException: RuntimeException, executionRequest: ExecutionServiceInput) {
        log.info("Executing Recovery")
        this.addError("${runtimeException.message}")
    }

    override suspend fun processNB(executionRequest: ExecutionServiceInput) {
        log.info("UE SUBSCRIPTION - START")
        val configValueSetup: ObjectNode = getDynamicProperties("config-deploy-setup") as ObjectNode

        configValueSetup.fields().forEach { it ->
            if (it.key == "helm_ueransim"){
                try {
                    log.info("Sending UE Subscription request")
                    subscribeUe()
                    log.info("UE Subscribed")
                }
                catch (e: Exception) {
                    log.error("Error during request sending.")
                }
            } else {
                log.info("Run UeSubscription only for helm_ueransim")
            }
        }
        log.info("UE SUBSCRIPTION - END")
    }

    private fun subscribeUe() {
        val restClientProperties: BasicAuthRestClientProperties = BasicAuthRestClientProperties()
        restClientProperties.url = "http://10.254.184.92:30500"
        restClientProperties.username=""
        restClientProperties.password=""
        val restClientService: BasicAuthRestClientService = BasicAuthRestClientService(restClientProperties)
        var result : BlueprintWebClientService.WebClientResponse<String>? = null

        val payload = prepareRequestBody()
        log.info("Request body: ${payload.toString()}")
        result  = restClientService.exchangeResource(HttpMethod.POST.name,
            "/api/subscriber/imsi-208930000000003/20893", payload.toString())
        log.info(payload.toString())
    }

    private fun prepareRequestBody(): String {
        val bodyString = "{\"plmnID\":\"20893\",\"ueId\":\"imsi-208930000000003\",\"AuthenticationSubscription\":{\"authenticationManagementField\":\"8000\",\"authenticationMethod\":\"5G_AKA\",\"milenage\":{\"op\":{\"encryptionAlgorithm\":0,\"encryptionKey\":0,\"opValue\":\"\"}},\"opc\":{\"encryptionAlgorithm\":0,\"encryptionKey\":0,\"opcValue\":\"8e27b6af0e692e750f32667a3b14605d\"},\"permanentKey\":{\"encryptionAlgorithm\":0,\"encryptionKey\":0,\"permanentKeyValue\":\"8baf473f2f8fd09487cccbd7097c6862\"},\"sequenceNumber\":\"16f3b3f70fc2\"},\"AccessAndMobilitySubscriptionData\":{\"gpsis\":[\"msisdn-0900000000\"],\"nssai\":{\"defaultSingleNssais\":[{\"sst\":1,\"sd\":\"010203\",\"isDefault\":true},{\"sst\":1,\"sd\":\"112233\",\"isDefault\":true}],\"singleNssais\":[]},\"subscribedUeAmbr\":{\"downlink\":\"2 Gbps\",\"uplink\":\"1 Gbps\"}},\"SessionManagementSubscriptionData\":[{\"singleNssai\":{\"sst\":1,\"sd\":\"010203\"},\"dnnConfigurations\":{\"internet\":{\"sscModes\":{\"defaultSscMode\":\"SSC_MODE_1\",\"allowedSscModes\":[\"SSC_MODE_2\",\"SSC_MODE_3\"]},\"pduSessionTypes\":{\"defaultSessionType\":\"IPV4\",\"allowedSessionTypes\":[\"IPV4\"]},\"sessionAmbr\":{\"uplink\":\"200 Mbps\",\"downlink\":\"100 Mbps\"},\"5gQosProfile\":{\"5qi\":9,\"arp\":{\"priorityLevel\":8},\"priorityLevel\":8}},\"internet2\":{\"sscModes\":{\"defaultSscMode\":\"SSC_MODE_1\",\"allowedSscModes\":[\"SSC_MODE_2\",\"SSC_MODE_3\"]},\"pduSessionTypes\":{\"defaultSessionType\":\"IPV4\",\"allowedSessionTypes\":[\"IPV4\"]},\"sessionAmbr\":{\"uplink\":\"200 Mbps\",\"downlink\":\"100 Mbps\"},\"5gQosProfile\":{\"5qi\":9,\"arp\":{\"priorityLevel\":8},\"priorityLevel\":8}}}},{\"singleNssai\":{\"sst\":1,\"sd\":\"112233\"},\"dnnConfigurations\":{\"internet\":{\"sscModes\":{\"defaultSscMode\":\"SSC_MODE_1\",\"allowedSscModes\":[\"SSC_MODE_2\",\"SSC_MODE_3\"]},\"pduSessionTypes\":{\"defaultSessionType\":\"IPV4\",\"allowedSessionTypes\":[\"IPV4\"]},\"sessionAmbr\":{\"uplink\":\"200 Mbps\",\"downlink\":\"100 Mbps\"},\"5gQosProfile\":{\"5qi\":9,\"arp\":{\"priorityLevel\":8},\"priorityLevel\":8}},\"internet2\":{\"sscModes\":{\"defaultSscMode\":\"SSC_MODE_1\",\"allowedSscModes\":[\"SSC_MODE_2\",\"SSC_MODE_3\"]},\"pduSessionTypes\":{\"defaultSessionType\":\"IPV4\",\"allowedSessionTypes\":[\"IPV4\"]},\"sessionAmbr\":{\"uplink\":\"200 Mbps\",\"downlink\":\"100 Mbps\"},\"5gQosProfile\":{\"5qi\":9,\"arp\":{\"priorityLevel\":8},\"priorityLevel\":8}}}}],\"SmfSelectionSubscriptionData\":{\"DnnInfosubscribedSnssaiInfos\":{\"01010203\":{\"dnnInfos\":[{\"dnn\":\"internet\"}]},\"01112233\":{\"dnnInfos\":[{\"dnn\":\"internet\"}]}}},\"AmPolicyData\":{\"subscCats\":[\"free5gc\"]},\"SmPolicyData\":{\"smPolicySnssaiData\":{\"01010203\":{\"snssai\":{\"sst\":1,\"sd\":\"010203\"},\"smPolicyDnnData\":{\"internet\":{\"dnn\":\"internet\"}}},\"01112233\":{\"snssai\":{\"sst\":1,\"sd\":\"112233\"},\"smPolicyDnnData\":{\"internet\":{\"dnn\":\"internet\"}}}}},\"FlowRules\":[]}"
//        val bodyMap = Gson().fromJson<Map<*, *>>(bodyString, MutableMap::class.java)
//        perform payload modifications if needed
//        val bodyJson = Gson().toJson(bodyMap)

//        return bodyJson
        return bodyString
    }

}
