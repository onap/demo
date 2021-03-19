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

import com.fasterxml.jackson.databind.JsonNode
import com.fasterxml.jackson.databind.node.ObjectNode
import com.fasterxml.jackson.module.kotlin.jacksonObjectMapper
import org.onap.ccsdk.cds.blueprintsprocessor.functions.resource.resolution.processor.ResourceAssignmentProcessor
import org.onap.ccsdk.cds.blueprintsprocessor.functions.resource.resolution.utils.ResourceAssignmentUtils
import org.onap.ccsdk.cds.controllerblueprints.core.BlueprintProcessorException
import org.onap.ccsdk.cds.controllerblueprints.resource.dict.ResourceAssignment
import org.slf4j.LoggerFactory

open class ConfigDeploySetup() : ResourceAssignmentProcessor() {

    private val log = LoggerFactory.getLogger(ConfigDeploySetup::class.java)!!

    override fun getName(): String {
        return "ConfigDeploySetup"
    }

    override suspend fun processNB(resourceAssignment: ResourceAssignment) {

        var retValue: ObjectNode? = null

        try {
            if (resourceAssignment.name == "config-deploy-setup") {
                val modulesSdnc = raRuntimeService.getResolutionStore("vf-modules-list-sdnc")["vf-modules"]
                val modulesAai = raRuntimeService.getResolutionStore("vf-modules-list-aai")["vf-modules"]
                val objectMapper = jacksonObjectMapper()
                val result: ObjectNode = objectMapper.createObjectNode()
                for (module in modulesSdnc) {
                    val modelTopology = module.at("/vf-module-data/vf-module-topology")
                    val moduleParameters = modelTopology.at("/vf-module-parameters/param")
                    val label: String? = getParamValueByName(moduleParameters,"vf_module_label")
                    if (label != null) {
                        val modelInfo = modelTopology["onap-model-information"]
                        val moduleData: ObjectNode = objectMapper.createObjectNode()
                        result.put(label, moduleData)
                        moduleData.put("k8s-rb-definition-name", modelInfo["model-invariant-uuid"].asText())
                        moduleData.put("k8s-rb-definition-version", modelInfo["model-uuid"].asText())
                        val templateName: String? = getParamValueByName(moduleParameters,"k8s-rb-config-template-name")
                        val templateSource: String? = getParamValueByName(moduleParameters,"k8s-rb-config-template-source")
                        if (templateName != null)
                            moduleData.put("k8s-rb-config-template-name", templateName)
                        if (templateSource != null)
                            moduleData.put("k8s-rb-config-template-source", templateSource)
                        for (aaiModule in modulesAai) {
                            if (aaiModule["vf-module-id"].asText() == module["vf-module-id"].asText()) {
                                moduleData.put("k8s-instance-id", aaiModule["heat-stack-id"].asText())
                                break
                            }
                        }
                    }
                }
                retValue = result
            }
            ResourceAssignmentUtils.setResourceDataValue(resourceAssignment, raRuntimeService, retValue)
        } catch (e: Exception) {
            log.error(e.message, e)
            ResourceAssignmentUtils.setResourceDataValue(resourceAssignment, raRuntimeService, "ERROR")

            throw BlueprintProcessorException("Failed in template key ($resourceAssignment) assignments, cause: ${e.message}", e)
        }
    }

    private fun getParamValueByName(params: JsonNode, paramName: String): String? {
        for (param in params) {
            if (param["name"].asText() == paramName) {
                return param["value"].asText()
            }
        }
        return null
    }

    override suspend fun recoverNB(runtimeException: RuntimeException, resourceAssignment: ResourceAssignment) {
        this.addError("${runtimeException.message}")
    }
}
