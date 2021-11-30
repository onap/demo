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
import org.onap.ccsdk.cds.blueprintsprocessor.functions.k8s.definition.template.K8sConfigTemplateComponent
import org.onap.ccsdk.cds.blueprintsprocessor.functions.k8s.definition.template.K8sConfigValueComponent
import org.onap.ccsdk.cds.blueprintsprocessor.functions.resource.resolution.processor.ResourceAssignmentProcessor
import org.onap.ccsdk.cds.blueprintsprocessor.functions.resource.resolution.utils.ResourceAssignmentUtils
import org.onap.ccsdk.cds.controllerblueprints.core.BluePrintProcessorException
import org.onap.ccsdk.cds.controllerblueprints.resource.dict.ResourceAssignment
import org.slf4j.LoggerFactory

open class ConfigDeploySetup() : ResourceAssignmentProcessor() {

    private val log = LoggerFactory.getLogger(ConfigDeploySetup::class.java)!!

    override fun getName(): String {
        return "ConfigDeploySetup"
    }

    override suspend fun processNB(executionRequest: ResourceAssignment) {

        try {
            if (executionRequest.name == "registration-reconfiguration-parameters"){
                val objectMapper = jacksonObjectMapper()
                var retValue: ObjectNode? = null
                val result: ObjectNode = objectMapper.createObjectNode()

                val pnfDetailsAai = raRuntimeService.getResolutionStore("pnf-details-aai")
                val serviceInstanceId = raRuntimeService.getResolutionStore("service-instance-id")
                val vnfId = raRuntimeService.getResolutionStore("vnf-id")

                if (pnfDetailsAai.get("pnf").get(0).get("orchestration-status").asText() == "Active") { //to sie dzieje w CORE, na potrzeby rekonfiguracji UERANSIM, gdy PNF nie jest aktywny, to znaczy ze nie został zarejestrowany
                    val reconfigParam: ObjectNode = objectMapper.createObjectNode()
                    reconfigParam.put("service-instance-id", pnfDetailsAai.get("pnf").get(0).get("serial-number"))
                    reconfigParam.put("vnf-id", pnfDetailsAai.get("pnf").get(0).get("equip-model"))
                    result.put("reconfiguration-params", reconfigParam)
                }
                else { //to sie dzieje w UERANSIM, na potrzeby eventu rejestracyjnego
                    val regiParam: ObjectNode = objectMapper.createObjectNode()
                    regiParam.put("service-instance-id", serviceInstanceId)
                    regiParam.put("vnf-id", vnfId)
                    result.put("registration-params", regiParam)
                }

                retValue = result
                ResourceAssignmentUtils.setResourceDataValue(executionRequest, raRuntimeService, retValue)
            }
            else if (executionRequest.name == "pnf-instance-name") {
                var retValue: String? = null

                val modulesSdnc = raRuntimeService.getResolutionStore("vf-modules-list-sdnc")["vf-modules"]
                val serviceInstanceName = raRuntimeService.getResolutionStore("service-instance-name").asText()

                var result: String = serviceInstanceName
                for (module in modulesSdnc) {
                    val modelTopology = module.at("/vf-module-data/vf-module-topology")
                    val moduleParameters = modelTopology.at("/vf-module-parameters/param")
                    val label: String? = getParamValueByName(moduleParameters, "vf_module_label")
                    if (label == "helm_ueransim") {
                        val coreInstanceName: String? = getParamValueByName(moduleParameters, "core-instance-name")
                        if (coreInstanceName != null)
                            result = coreInstanceName
                    }
                }

                retValue = result
                ResourceAssignmentUtils.setResourceDataValue(executionRequest, raRuntimeService, retValue)
            }
            else if (executionRequest.name == "config-deploy-setup") {
                val objectMapper = jacksonObjectMapper()
                var retValue: ObjectNode? = null
                val result: ObjectNode = objectMapper.createObjectNode()

                val modulesSdnc = raRuntimeService.getResolutionStore("vf-modules-list-sdnc")["vf-modules"]
                val modulesAai = raRuntimeService.getResolutionStore("vf-modules-list-aai")["vf-modules"]

                for (module in modulesSdnc) {
                    val modelTopology = module.at("/vf-module-data/vf-module-topology")
                    val moduleParameters = modelTopology.at("/vf-module-parameters/param")
                    val label: String? = getParamValueByName(moduleParameters, "vf_module_label")
                    if (label != null) {
                        val modelInfo = modelTopology["onap-model-information"]
                        val moduleData: ObjectNode = objectMapper.createObjectNode()
                        result.put(label, moduleData)
                        moduleData.put(K8sConfigTemplateComponent.INPUT_K8S_DEFINITION_NAME, modelInfo["model-invariant-uuid"].asText())
                        moduleData.put(K8sConfigTemplateComponent.INPUT_K8S_DEFINITION_VERSION, modelInfo["model-customization-uuid"].asText())
                        val templateName: String? = getParamValueByName(moduleParameters, K8sConfigTemplateComponent.INPUT_K8S_TEMPLATE_NAME)
                        val templateSource: String? = getParamValueByName(moduleParameters, K8sConfigTemplateComponent.INPUT_K8S_TEMPLATE_SOURCE)
                        val configValueSource: String? = getParamValueByName(moduleParameters, K8sConfigValueComponent.INPUT_K8S_CONFIG_VALUE_SOURCE)
                        val configName: String? = getParamValueByName(moduleParameters, K8sConfigValueComponent.INPUT_K8S_RB_CONFIG_NAME)
                        val coreInstanceName: String? = getParamValueByName(moduleParameters, "core-instance-name")

                        if (templateName != null)
                            moduleData.put(K8sConfigTemplateComponent.INPUT_K8S_TEMPLATE_NAME, templateName)
                        if (templateSource != null)
                            moduleData.put(K8sConfigTemplateComponent.INPUT_K8S_TEMPLATE_SOURCE, templateSource)
                        if (configValueSource != null)
                            moduleData.put(K8sConfigValueComponent.INPUT_K8S_CONFIG_VALUE_SOURCE, configValueSource)
                        if (configName != null)
                            moduleData.put(K8sConfigValueComponent.INPUT_K8S_RB_CONFIG_NAME, configName)
                        if (coreInstanceName != null)
                            moduleData.put("core-instance-name", coreInstanceName)

                        for (aaiModule in modulesAai) {
                            if (aaiModule["vf-module-id"].asText() == module["vf-module-id"].asText() && aaiModule["heat-stack-id"] != null) {
                                moduleData.put(K8sConfigValueComponent.INPUT_K8S_INSTANCE_ID, aaiModule["heat-stack-id"].asText())
                                break
                            }
                        }
                    }
                }
                retValue = result
                ResourceAssignmentUtils.setResourceDataValue(executionRequest, raRuntimeService, retValue)
            }

        } catch (e: Exception) {
            log.error(e.message, e)
            ResourceAssignmentUtils.setResourceDataValue(executionRequest, raRuntimeService, "ERROR")

            throw BluePrintProcessorException("Failed in template key ($executionRequest) assignments, cause: ${e.message}", e)
        }
    }

    private fun getParamValueByName(params: JsonNode, paramName: String): String? {
        for (param in params) {
            if (param["name"].asText() == paramName && param["value"].asText() != "null") {
                return param["value"].asText()
            }
        }
        return null
    }

    override suspend fun recoverNB(runtimeException: RuntimeException, executionRequest: ResourceAssignment) {
        this.addError("${runtimeException.message}")
    }
}
