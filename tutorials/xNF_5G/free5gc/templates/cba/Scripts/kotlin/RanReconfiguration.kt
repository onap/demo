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
import net.minidev.json.JSONObject
import org.onap.ccsdk.cds.blueprintsprocessor.core.api.data.ActionIdentifiers
import org.onap.ccsdk.cds.blueprintsprocessor.core.api.data.CommonHeader
import org.onap.ccsdk.cds.blueprintsprocessor.core.api.data.ExecutionServiceInput
import org.onap.ccsdk.cds.blueprintsprocessor.selfservice.api.ExecutionServiceHandler
import org.onap.ccsdk.cds.blueprintsprocessor.services.execution.AbstractScriptComponentFunction
import org.onap.ccsdk.cds.controllerblueprints.core.asJsonString
import org.onap.ccsdk.cds.controllerblueprints.core.config.BluePrintLoadConfiguration
import org.slf4j.LoggerFactory

open class RanReconfiguration : AbstractScriptComponentFunction() {

    private val log = LoggerFactory.getLogger(RanReconfiguration::class.java)!!

    override fun getName(): String {
        return "RanReconfiguration"
    }

    override suspend fun recoverNB(runtimeException: RuntimeException, executionRequest: ExecutionServiceInput) {
        log.info("Executing Recovery")
        this.addError("${runtimeException.message}")
    }

    override suspend fun processNB(executionRequest: ExecutionServiceInput) {
        log.info("RAN RECONFIGURATION - START")
        val configValueSetup: ObjectNode = getDynamicProperties("config-deploy-setup") as ObjectNode
        val reconfigurationParams: ObjectNode = getDynamicProperties("registration-reconfiguration-parameters") as ObjectNode
        if (configValueSetup.has("helm_free5gc")) {
            val pnfDetailsServiceInstanceId = reconfigurationParams.get("reconfiguration-params").get("service-instance-id").asText()
            val pnfDetailsVnfId = reconfigurationParams.get("reconfiguration-params").get("vnf-id").asText()

            sendReconfigurationRequest(pnfDetailsServiceInstanceId, pnfDetailsVnfId)
        }
        else {
            log.info("Run RanReconfiguration only for helm_free5gc")
        }
        log.info("RAN RECONFIGURATION - END")
    }

    private suspend fun sendReconfigurationRequest(serviceInstanceId: String, vnfId: String){
        val executionServiceHandler: ExecutionServiceHandler =
            this.functionDependencyInstanceAsType("executionServiceHandler")

        val executionServiceInput = ExecutionServiceInput()
        //Common header
        executionServiceInput.commonHeader = CommonHeader()
        executionServiceInput.commonHeader.originatorId = "onap-me-cm-adapter"
        executionServiceInput.commonHeader.requestId = "1"
        executionServiceInput.commonHeader.subRequestId = "1"
        //action identifiers
        executionServiceInput.actionIdentifiers = ActionIdentifiers()
        executionServiceInput.actionIdentifiers.blueprintName = "free5GC"
        executionServiceInput.actionIdentifiers.blueprintVersion = "1.1.1"
        executionServiceInput.actionIdentifiers.actionName = "ue-reconfiguration"
        executionServiceInput.actionIdentifiers.mode = "sync"
        //payload
        val mapper = ObjectMapper()
        val ueReconfigurationRequest = mapper.createObjectNode()
        ueReconfigurationRequest.put("resolution-key","VF_ueransim_CNF")
        val properties = mapper.createObjectNode()
        properties.put("service-instance-id", serviceInstanceId)
        properties.put("vnf-id", vnfId)
        ueReconfigurationRequest.put("ue-reconfiguration-properties", properties)

        executionServiceInput.payload = mapper.createObjectNode()
        executionServiceInput.payload.put("ue-reconfiguration-request", ueReconfigurationRequest)
        log.info("Request body: ${executionServiceInput.asJsonString()}")

        val processResult = executionServiceHandler.doProcess(executionServiceInput)
        log.info("Request result: ${processResult.asJsonString()}")
    }
}
