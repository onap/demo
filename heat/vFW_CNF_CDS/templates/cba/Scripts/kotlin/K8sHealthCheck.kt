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

import com.fasterxml.jackson.annotation.JsonIgnoreProperties
import com.fasterxml.jackson.annotation.JsonProperty
import com.google.gson.Gson
import org.onap.ccsdk.cds.blueprintsprocessor.core.BlueprintPropertiesService
import org.onap.ccsdk.cds.blueprintsprocessor.core.api.data.ExecutionServiceInput
import org.onap.ccsdk.cds.blueprintsprocessor.functions.k8s.K8sConnectionPluginConfiguration
import org.onap.ccsdk.cds.blueprintsprocessor.functions.k8s.instance.K8sPluginInstanceApi
import org.onap.ccsdk.cds.blueprintsprocessor.functions.k8s.instance.K8sRbInstanceResourceStatus
import org.onap.ccsdk.cds.blueprintsprocessor.functions.k8s.instance.K8sRbInstanceStatus
import org.onap.ccsdk.cds.blueprintsprocessor.services.execution.AbstractScriptComponentFunction
import org.onap.ccsdk.cds.controllerblueprints.core.BlueprintProcessorException
import org.onap.ccsdk.cds.controllerblueprints.core.utils.JacksonUtils
import org.slf4j.LoggerFactory

/*
 * Performs a status check against all helm packages (vf-modules) for a CNF to check
 * if they are healthy or not. The input (instance ids valid for the helm packages) is
 * resolved already  in the resource resolution phase of the cba and stored in
 * "helm_instances".
 * If one of the packages is not healthy (parameter phase!="Running" the status check is
 * failed. The failure leads to throwing an exception, otherwise the code just returns.
 * In the end the CBA will return an http (500) or http (200) for the health-check flow
 * depending of the result of status check.
 *
*/
open class K8sHealthCheck : AbstractScriptComponentFunction() {

    private val log = LoggerFactory.getLogger(K8sHealthCheck::class.java)!!

    override fun getName(): String {
        return "K8sHealthCheckKt"
    }

    override suspend fun processNB(executionRequest: ExecutionServiceInput) {
        log.info("Health check script execution Started")

        val bluePrintPropertiesService: BlueprintPropertiesService = this.functionDependencyInstanceAsType("blueprintPropertiesService")!!

        val k8sConfiguration = K8sConnectionPluginConfiguration(bluePrintPropertiesService)

        val instanceApi = K8sPluginInstanceApi(k8sConfiguration)
        // Get status for each instance belonging to this specific CNF. The instance Ids are retrieved in the
        // Resource assignment phase
        val helmInstances = getDynamicProperties("helm_instances")
        log.info("Helm instances: $helmInstances")
        for (instance in helmInstances) {
            val instanceStatus: K8sRbInstanceStatus = instanceApi.getInstanceStatus(instance.get("k8s-instance-id").asText())!!
            val resourcesStatus: List<K8sRbInstanceResourceStatus>? = instanceStatus.resourcesStatus
            if (resourcesStatus != null) {
                for (status in resourcesStatus) {
                    if (!instanceStatusIsOk(status)) {
                        throw BlueprintProcessorException("Health check status failed for ${status.name}!")
                    }
                }
            }
            log.info(instance.asText())
        }
        log.info("Health check Script execution Ended")
    }

    override suspend fun recoverNB(runtimeException: RuntimeException, executionRequest: ExecutionServiceInput) {
        log.info("Executing Recovery")
        bluePrintRuntimeService.getBlueprintError().addError(
            "${runtimeException.message}", getName()
        )
    }

    private fun instanceStatusIsOk(resourceStatus: K8sRbInstanceResourceStatus): Boolean {
        log.info("instanceStatusIsOk start")
        if (resourceStatus.status != null) {
            if (resourceStatus.status!!.containsKey("status")) {
                val gson = Gson()
                val innerStatusData = gson.toJson(resourceStatus.status!!["status"]).toString()
                val innerStatusJson: StatusPhase? = JacksonUtils.readValue(innerStatusData, StatusPhase::class.java)
                if (innerStatusJson!!.phase != null && !innerStatusJson.phase.equals("Running")) {
                    log.info("instanceStatusIsOk return false")
                    return false
                }
            }
        }
        log.info("instanceStatusIsOk return true")
        return true
    }

    @JsonIgnoreProperties(ignoreUnknown = true)
    class StatusPhase {

        @get:JsonProperty("phase")
        var phase: String? = null

        override fun equals(other: Any?): Boolean {
            if (this === other) return true
            if (javaClass != other?.javaClass) return false
            return true
        }

        override fun hashCode(): Int {
            return javaClass.hashCode()
        }
    }
}
