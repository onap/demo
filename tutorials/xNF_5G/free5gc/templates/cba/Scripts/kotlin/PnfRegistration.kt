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

import com.fasterxml.jackson.databind.node.ObjectNode
import com.google.gson.Gson
import kotlinx.coroutines.delay
import kotlinx.coroutines.withTimeout
import net.minidev.json.JSONObject
import org.onap.ccsdk.cds.blueprintsprocessor.core.api.data.ExecutionServiceInput
import org.onap.ccsdk.cds.blueprintsprocessor.rest.BasicAuthRestClientProperties
import org.onap.ccsdk.cds.blueprintsprocessor.rest.service.BasicAuthRestClientService
import org.onap.ccsdk.cds.blueprintsprocessor.rest.service.BlueprintWebClientService
import org.onap.ccsdk.cds.blueprintsprocessor.services.execution.AbstractScriptComponentFunction
import org.slf4j.LoggerFactory
import org.springframework.http.HttpMethod

open class PnfRegistration : AbstractScriptComponentFunction() {

    private val log = LoggerFactory.getLogger(PnfRegistration::class.java)!!

    override fun getName(): String {
        return "PnfRegistration"
    }

    override suspend fun recoverNB(runtimeException: RuntimeException, executionRequest: ExecutionServiceInput) {
        log.info("Executing Recovery")
        this.addError("${runtimeException.message}")
    }

    override suspend fun processNB(executionRequest: ExecutionServiceInput) {
        log.info("PNF REGISTRATION - START")
        val configValueSetup: ObjectNode = getDynamicProperties("config-deploy-setup") as ObjectNode
        val registrationParams: ObjectNode = getDynamicProperties("registration-reconfiguration-parameters") as ObjectNode
        val vesIpAddress = "dcae-ves-collector"
        val vesPort = "8443"
        val aaiIpAddress = "aai"
        val aaiPort = "8443"
//        val vesIpAddress = "10.254.184.164"
//        val vesPort = "8443"
//        val aaiIpAddress = "10.254.184.164"
//        val aaiPort = "30233"
        configValueSetup.fields().forEach { it ->
            if (it.key == "helm_ueransim"){
                if (registrationParams.has("registration-params")){
                    val coreInstanceName = it.value["core-instance-name"].asText()
                    val serviceInstanceId = registrationParams.get("registration-params").get("service-instance-id").asText()
                    val vnfId = registrationParams.get("registration-params").get("vnf-id").asText()
                    try {
                        withTimeout(60500) {
                            while(!isPnfRegistered(coreInstanceName, aaiIpAddress, aaiPort))
                            {
                                log.info("PNF not registered, sending registration event.")
                                sendPnfRegistrationEvent(coreInstanceName, vesIpAddress, vesPort, serviceInstanceId, vnfId)
                                delay(15000)
                            }
                            log.info("PNF registered")
                        }
                    }
                    catch (e: Exception){
                        log.error("Error during request sending.")
                    }
                }
                else {
                    log.error("No need to register PNF.") //See ConfigDeploySetup - status "Active" means PNF is registered
                }
            }
            else {
                log.info("Run PnfRegistration only for helm_ueransim")
            }
        }
        log.info("PNF REGISTRATION - END")
    }

    private fun isPnfRegistered(sourceName: String, ipAddress: String, port: String): Boolean {
        val restClientProperties: BasicAuthRestClientProperties = BasicAuthRestClientProperties()
        restClientProperties.url = "https://$ipAddress:$port"
        restClientProperties.password = "AAI"
        restClientProperties.username = "AAI"
        val additionalHeaders = hashMapOf<String,String>()
        additionalHeaders["X-TransactionId"] = "1234"
        additionalHeaders["X-FromAppId"] = "CDS"
        restClientProperties.additionalHeaders = additionalHeaders
        val restClientService: BasicAuthRestClientService = BasicAuthRestClientService(restClientProperties)
        var result : BlueprintWebClientService.WebClientResponse<String>? = null
        result  = restClientService.exchangeResource(HttpMethod.GET.name, "/aai/v16/network/pnfs/pnf/$sourceName", "")
        val bodyString = result.body.toString()
        val bodyMap = Gson().fromJson<Map<*, *>>(bodyString, MutableMap::class.java)

        return bodyMap["orchestration-status"] == "Active"
    }

    private fun sendPnfRegistrationEvent(sourceName: String, vesIpAddress: String, vesPort: String, serviceInstanceId: String, vnfId: String){
        val restClientProperties: BasicAuthRestClientProperties = BasicAuthRestClientProperties()
        restClientProperties.url = "https://$vesIpAddress:$vesPort"
        restClientProperties.password = "sample1"
        restClientProperties.username = "sample1"
        val restClientService: BasicAuthRestClientService = BasicAuthRestClientService(restClientProperties)
        var result : BlueprintWebClientService.WebClientResponse<String>? = null

        val payload = prepareRequestBody(sourceName, serviceInstanceId, vnfId)

        result  = restClientService.exchangeResource(HttpMethod.POST.name, "/eventListener/v7", payload.toString())

        if (result.status == 202 && result.body == "Accepted") {
            log.info("PNF Registration event sent successfully")
        } else {
            log.error("Something went wrong with PNF Registrtation")
        }
    }

    private fun prepareRequestBody(sourceName: String, serviceInstanceId: String, vnfId: String): JSONObject {
        val commonEventHeaderData = JSONObject()
        commonEventHeaderData.appendField("startEpochMicrosec", 1538407540940)
        commonEventHeaderData.appendField("sourceId", "val13")
        commonEventHeaderData.appendField("eventId", "registration_38407540")
        commonEventHeaderData.appendField("nfcNamingCode", "oam")
        commonEventHeaderData.appendField("internalHeaderFields", JSONObject())
        commonEventHeaderData.appendField("eventType", "pnfRegistration")
        commonEventHeaderData.appendField("priority", "Normal")
        commonEventHeaderData.appendField("version", "4.0.1")
        commonEventHeaderData.appendField("reportingEntityName", "NOK6061ZW3")
        commonEventHeaderData.appendField("sequence", 0)
        commonEventHeaderData.appendField("domain", "pnfRegistration")
        commonEventHeaderData.appendField("lastEpochMicrosec",1538407540940)
        commonEventHeaderData.appendField("eventName", "test")
        commonEventHeaderData.appendField("vesEventListenerVersion", "7.0.1")
        commonEventHeaderData.appendField("sourceName", sourceName)
        commonEventHeaderData.appendField("nfNamingCode", "pnfservice")

        val pnfRegistrationFieldsData = JSONObject()
        pnfRegistrationFieldsData.appendField("unitType", "val8")
        pnfRegistrationFieldsData.appendField("serialNumber", serviceInstanceId)
        pnfRegistrationFieldsData.appendField("pnfRegistrationFieldsVersion", "2.0")
        pnfRegistrationFieldsData.appendField("manufactureDate", "1538407540942")
        pnfRegistrationFieldsData.appendField("modelNumber", vnfId)
        pnfRegistrationFieldsData.appendField("lastServiceDate", "1538407540942")
        pnfRegistrationFieldsData.appendField("unitFamily", "BBU")
        pnfRegistrationFieldsData.appendField("vendorName", "Juniper")
        pnfRegistrationFieldsData.appendField("oamV4IpAddress", "10.10.10.10")
        pnfRegistrationFieldsData.appendField("oamV6IpAddress", "ff02::02")
        pnfRegistrationFieldsData.appendField("softwareVersion", "1")

        val eventData = JSONObject()
        eventData.appendField("pnfRegistrationFields", pnfRegistrationFieldsData)
        eventData.appendField("commonEventHeader", commonEventHeaderData)

        val payload = JSONObject()
        payload.appendField("event", eventData)

        val payloadString = payload.toString()
        log.info("Request body: $payloadString")

        return payload
    }
}
