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
import org.onap.ccsdk.cds.blueprintsprocessor.functions.k8s.definition.profile.K8sProfileUploadComponent
import org.onap.ccsdk.cds.blueprintsprocessor.functions.resource.resolution.processor.ResourceAssignmentProcessor
import org.onap.ccsdk.cds.blueprintsprocessor.functions.resource.resolution.utils.ResourceAssignmentUtils
import org.onap.ccsdk.cds.controllerblueprints.core.BluePrintProcessorException
import org.onap.ccsdk.cds.controllerblueprints.core.isNullOrMissing
import org.onap.ccsdk.cds.controllerblueprints.resource.dict.ResourceAssignment
import org.slf4j.LoggerFactory

open class ConfigDeploySetup() : ResourceAssignmentProcessor() {

    private val log = LoggerFactory.getLogger(ConfigDeploySetup::class.java)!!

    override fun getName(): String {
        return "ConfigDeploySetup"
    }

    override suspend fun processNB(executionRequest: ResourceAssignment) {

        var retValue: Any? = null

        try {
            if (executionRequest.name == "service-instance-id") {
                var value = raRuntimeService.getInputValue(executionRequest.name)
                if (!value.isNullOrMissing()) {
                    retValue = value.asText()
                } else {
                    val vnfRelationshipList = raRuntimeService.getResolutionStore("vnf-relationship-list")
                    if (!vnfRelationshipList.isNullOrMissing()) {
                        vnfRelationshipList["relationship-list"].forEach { relation ->
                            if (relation["related-to"].asText() == "service-instance") {
                                relation["relationship-data"].forEach { data ->
                                    if (data["relationship-key"].asText() == "service-instance.service-instance-id") {
                                        retValue = data["relationship-value"].asText()
                                    }
                                }
                            }
                        }
                    }
                }
            } else if (executionRequest.name == "vnf-id") {
                var value = raRuntimeService.getInputValue(executionRequest.name)
                if (!value.isNullOrMissing()) {
                    retValue = value.asText()
                } else {
                    value = raRuntimeService.getInputValue("generic-vnf.vnf-id")
                    if (!value.isNullOrMissing()) {
                        retValue = value.asText()
                    }
                }
            } else if (executionRequest.name == "replica-count") {
                retValue = "1"
                try {
                    var value = raRuntimeService.getInputValue(executionRequest.name)
                    if (!value.isNullOrMissing()) {
                        retValue = value.asText()
                    } else {
                        value = raRuntimeService.getInputValue("data")
                        if (!value.isNullOrMissing()) {
                            if (value["replicaCount"] != null) {
                                retValue = value["replicaCount"].asText()
                            }
                        }
                    }
                } catch (e: Exception) {
                    log.error(e.message, e)
                    log.info("Setting default replica count: 1")
                }
            } else if (executionRequest.name == "config-deploy-setup") {
                val modulesSdnc = raRuntimeService.getResolutionStore("vf-modules-list-sdnc")["vf-modules"]
                val modulesAai = raRuntimeService.getResolutionStore("vf-modules-list-aai")["vf-modules"]
                val objectMapper = jacksonObjectMapper()
                val result: ObjectNode = objectMapper.createObjectNode()
                for (module in modulesSdnc) {
                    val modelTopology = module.at("/vf-module-data/vf-module-topology")
                    val moduleParameters = modelTopology.at("/vf-module-parameters/param")
                    val label: String? = getParamValueByName(moduleParameters, "vf_module_label")
                    if (label != null) {
                        val modelInfo = modelTopology["onap-model-information"]
                        val moduleData: ObjectNode = objectMapper.createObjectNode()
                        result.put(label, moduleData)
                        val profileName: String? = getParamValueByName(moduleParameters, K8sProfileUploadComponent.INPUT_K8S_PROFILE_NAME)
                        val profileSource: String? = getParamValueByName(moduleParameters, K8sProfileUploadComponent.INPUT_K8S_PROFILE_SOURCE)
                        val profileNamespace: String? = getParamValueByName(moduleParameters, K8sProfileUploadComponent.INPUT_K8S_PROFILE_NAMESPACE)
                        val profileK8sVersion: String? = getParamValueByName(moduleParameters, K8sProfileUploadComponent.INPUT_K8S_PROFILE_K8S_VERSION)
                        val templateName: String? = getParamValueByName(moduleParameters, K8sConfigTemplateComponent.INPUT_K8S_TEMPLATE_NAME)
                        val templateSource: String? = getParamValueByName(moduleParameters, K8sConfigTemplateComponent.INPUT_K8S_TEMPLATE_SOURCE)
                        val configValueSource: String? = getParamValueByName(moduleParameters, K8sConfigValueComponent.INPUT_K8S_CONFIG_VALUE_SOURCE)
                        val configName: String? = getParamValueByName(moduleParameters, K8sConfigValueComponent.INPUT_K8S_RB_CONFIG_NAME)

                        if (profileName != null)
                            moduleData.put(K8sProfileUploadComponent.INPUT_K8S_PROFILE_NAME, profileName)
                        if (profileSource != null)
                            moduleData.put(K8sProfileUploadComponent.INPUT_K8S_PROFILE_SOURCE, profileSource)
                        if (profileNamespace != null)
                            moduleData.put(K8sProfileUploadComponent.INPUT_K8S_PROFILE_NAMESPACE, profileNamespace)
                        if (profileK8sVersion != null)
                            moduleData.put(K8sProfileUploadComponent.INPUT_K8S_PROFILE_K8S_VERSION, profileK8sVersion)
                        if (templateName != null)
                            moduleData.put(K8sConfigTemplateComponent.INPUT_K8S_TEMPLATE_NAME, templateName)
                        if (templateSource != null)
                            moduleData.put(K8sConfigTemplateComponent.INPUT_K8S_TEMPLATE_SOURCE, templateSource)
                        if (configValueSource != null)
                            moduleData.put(K8sConfigValueComponent.INPUT_K8S_CONFIG_VALUE_SOURCE, configValueSource)
                        if (configName != null)
                            moduleData.put(K8sConfigValueComponent.INPUT_K8S_RB_CONFIG_NAME, configName)

                        for (aaiModule in modulesAai) {
                            if (aaiModule["vf-module-id"].asText() == module["vf-module-id"].asText() && aaiModule["heat-stack-id"] != null) {
                                moduleData.put(K8sConfigValueComponent.INPUT_K8S_INSTANCE_ID, aaiModule["heat-stack-id"].asText())
                                moduleData.put(K8sConfigTemplateComponent.INPUT_K8S_DEFINITION_NAME, aaiModule["model-invariant-id"].asText())
                                moduleData.put(K8sConfigTemplateComponent.INPUT_K8S_DEFINITION_VERSION, aaiModule["model-customization-id"].asText())
                                break
                            }
                        }
                    }
                }
                retValue = result
            }
            ResourceAssignmentUtils.setResourceDataValue(executionRequest, raRuntimeService, retValue)
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
