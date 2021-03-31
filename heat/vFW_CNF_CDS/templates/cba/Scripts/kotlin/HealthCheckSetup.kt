/*
 * Copyright (C) 2021 Samsung Electronics
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *     http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License
 */
package org.onap.ccsdk.cds.blueprintsprocessor.services.execution.scripts

import com.fasterxml.jackson.databind.JsonNode
import com.fasterxml.jackson.databind.node.ArrayNode
import com.fasterxml.jackson.databind.node.ObjectNode
import com.fasterxml.jackson.module.kotlin.jacksonObjectMapper
import org.onap.ccsdk.cds.blueprintsprocessor.functions.resource.resolution.processor.ResourceAssignmentProcessor
import org.onap.ccsdk.cds.blueprintsprocessor.functions.resource.resolution.utils.ResourceAssignmentUtils
import org.onap.ccsdk.cds.controllerblueprints.core.BlueprintProcessorException
import org.onap.ccsdk.cds.controllerblueprints.resource.dict.ResourceAssignment
import org.slf4j.LoggerFactory

/*
 * Finds all helm packages in the resolution store
 */
open class HealthCheckSetup : ResourceAssignmentProcessor() {

    private val log = LoggerFactory.getLogger(HealthCheckSetup::class.java)!!

    override fun getName(): String {
        return "HealthCheckSetup"
    }

    override suspend fun processNB(executionRequest: ResourceAssignment) {

        var retValue: ObjectNode? = null

        try {
            if (executionRequest.name == "health-check-setup") {
                val modules = raRuntimeService.getResolutionStore("vf-modules-list")["vf-modules"]
                val objectMapper = jacksonObjectMapper()
                val result: ObjectNode = objectMapper.createObjectNode()
                val moduleData: ArrayNode = objectMapper.createArrayNode()
                result.set<JsonNode>("helm_instances", moduleData)
                for (module in modules) {
                    if (module["vf-module-name"].asText().startsWith("helm_")) {
                        val instanceNode: ObjectNode = objectMapper.createObjectNode()
                        instanceNode.put("instance-id", module["heat-stack-id"].asText().split("/")[1])
                        moduleData.add(instanceNode)
                    }
                    log.info(module.asText())
                }
                retValue = result
            }
            ResourceAssignmentUtils.setResourceDataValue(executionRequest, raRuntimeService, retValue)
        } catch (e: Exception) {
            log.error(e.message, e)
            ResourceAssignmentUtils.setResourceDataValue(executionRequest, raRuntimeService, "ERROR")
            throw BlueprintProcessorException("Failed in template key ($executionRequest) assignments, cause: ${e.message}", e)
        }
    }

    override suspend fun recoverNB(runtimeException: RuntimeException, executionRequest: ResourceAssignment) {
        raRuntimeService.getBlueprintError().addError(
            "Failed in ConfigValuesSetup-ResourceAssignmentProcessor : ${runtimeException.message}",
            getName()
        )
    }
}
