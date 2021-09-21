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

import com.fasterxml.jackson.databind.node.ObjectNode
import kotlinx.coroutines.Job
import kotlinx.coroutines.cancel
import kotlinx.coroutines.delay
import kotlinx.coroutines.joinAll
import kotlinx.coroutines.launch
import kotlinx.coroutines.runBlocking
import org.onap.ccsdk.cds.blueprintsprocessor.core.BluePrintPropertiesService
import org.onap.ccsdk.cds.blueprintsprocessor.core.api.data.ExecutionServiceInput
import org.onap.ccsdk.cds.blueprintsprocessor.functions.k8s.K8sConnectionPluginConfiguration
import org.onap.ccsdk.cds.blueprintsprocessor.functions.k8s.instance.K8sPluginInstanceApi
import org.onap.ccsdk.cds.blueprintsprocessor.functions.k8s.instance.healthcheck.K8sRbInstanceHealthCheck
import org.onap.ccsdk.cds.blueprintsprocessor.functions.k8s.instance.healthcheck.K8sRbInstanceHealthCheckSimple
import org.onap.ccsdk.cds.blueprintsprocessor.services.execution.AbstractScriptComponentFunction
import org.slf4j.LoggerFactory

open class K8sHealthCheck : AbstractScriptComponentFunction() {

    private val log = LoggerFactory.getLogger(K8sHealthCheck::class.java)!!

    override fun getName(): String {
        return "K8sHealthCheck"
    }

    private fun initPluginApi(): K8sPluginInstanceApi {
        val bluePrintPropertiesService: BluePrintPropertiesService = this.functionDependencyInstanceAsType("bluePrintPropertiesService")!!
        val k8sConfiguration = K8sConnectionPluginConfiguration(bluePrintPropertiesService)

        return K8sPluginInstanceApi(k8sConfiguration)
    }

    override suspend fun processNB(executionRequest: ExecutionServiceInput) {
        val instanceApi = initPluginApi()

        log.info("Health check script execution - START")
        val configValueSetup: ObjectNode = getDynamicProperties("config-deploy-setup") as ObjectNode
        log.info("Config Value Setup: $configValueSetup")

        val instanceHealthCheckList = startInstanceHealthCheck(configValueSetup, instanceApi)
        val statuses = getStatuses(instanceHealthCheckList, instanceApi)
        log.info("Health check script execution - END")
    }

    private fun startInstanceHealthCheck(configValueSetup: ObjectNode, instanceApi: K8sPluginInstanceApi): List<HealthCheckInstance> {
        val healthCheckInstanceList = arrayListOf<HealthCheckInstance>()

        configValueSetup.fields().forEach {
            val instanceName = it.value.get("k8s-instance-id").asText()
            val response: K8sRbInstanceHealthCheckSimple? = instanceApi.startInstanceHealthCheck(instanceName)
            log.debug("K8sRbInstanceHealthCheckSimple response: $$response")
            healthCheckInstanceList.add(HealthCheckInstance(instanceName, response?.id))
        }
        log.info("healthCheckInstanceList: $healthCheckInstanceList")

        return healthCheckInstanceList
    }

    private fun getStatuses(instanceHealthCheckList: List<HealthCheckInstance>, instanceApi: K8sPluginInstanceApi): Map<String, String> {
        val statuses = hashMapOf<String, String>()
        runBlocking {
            val jobs: List<Job> = instanceHealthCheckList.map {
                launch {
                    log.info("Thread started: ${Thread.currentThread().name} for $it")
                    // WAIT APPROX 5 MINUTES
                    repeat(30) { _ ->
                        val response: K8sRbInstanceHealthCheck = instanceApi.getInstanceHealthCheck(it.heatStackId, it.healthCheckInstance!!)!!
                        log.debug("Response for $it: $response")
                        val status = response.status!!
                        if (!"RUNNING".equals(status, true)) {
                            statuses[it.heatStackId] = status
                            log.info("Poll status: $status for $it")
                            instanceApi.deleteInstanceHealthCheck(it.heatStackId, it.healthCheckInstance)
                            cancel()
                        }
                        delay(10_000L)
                    }
                    statuses[it.heatStackId] = "Timeout"
                    log.warn("Send delete hc request")
                    instanceApi.deleteInstanceHealthCheck(it.heatStackId, it.healthCheckInstance!!)
                }
            }
            jobs.joinAll()
        }
        var success = true
        statuses?.forEach { it ->
            if (it.value != "Succeeded") {
                success = false
            }
        }
        log.info("---")
        if (success) {
            log.info("Healthcheck finished successfully")
        } else {
            log.info("Healthcheck finished with failure")
        }
        log.info("Detailed results: $statuses")
        log.info("---")
        return statuses
    }

    data class HealthCheckInstance(val heatStackId: String, val healthCheckInstance: String?) {
        override fun toString(): String {
            return "HealthCheckInstance(heatStackId='$heatStackId', healthCheckInstance='$healthCheckInstance')"
        }
    }

    override suspend fun recoverNB(runtimeException: RuntimeException, executionRequest: ExecutionServiceInput) {
        log.info("Executing Recovery")
        bluePrintRuntimeService.getBluePrintError().addError("${runtimeException.message}", getName())
    }
}
