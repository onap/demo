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
import com.google.common.util.concurrent.ThreadFactoryBuilder
import org.onap.ccsdk.cds.blueprintsprocessor.core.BlueprintPropertiesService
import org.onap.ccsdk.cds.blueprintsprocessor.core.api.data.ExecutionServiceInput
import org.onap.ccsdk.cds.blueprintsprocessor.functions.k8s.K8sConnectionPluginConfiguration
import org.onap.ccsdk.cds.blueprintsprocessor.functions.k8s.instance.K8sPluginInstanceApi
import org.onap.ccsdk.cds.blueprintsprocessor.functions.k8s.instance.healthcheck.K8sRbInstanceHealthCheck
import org.onap.ccsdk.cds.blueprintsprocessor.functions.k8s.instance.healthcheck.K8sRbInstanceHealthCheckSimple
import org.onap.ccsdk.cds.blueprintsprocessor.services.execution.AbstractScriptComponentFunction
import org.slf4j.LoggerFactory
import java.util.Timer
import java.util.TimerTask
import java.util.concurrent.Callable
import java.util.concurrent.Executors
import java.util.concurrent.TimeUnit

open class K8sHealthCheck : AbstractScriptComponentFunction() {

    private val log = LoggerFactory.getLogger(K8sHealthCheck::class.java)!!

    override fun getName(): String {
        return "K8sHealthCheck"
    }

    private fun initPluginApi(): K8sPluginInstanceApi {
        val bluePrintPropertiesService: BlueprintPropertiesService = this.functionDependencyInstanceAsType("blueprintPropertiesService")!!
        val k8sConfiguration = K8sConnectionPluginConfiguration(bluePrintPropertiesService)

        return K8sPluginInstanceApi(k8sConfiguration)
    }

    override suspend fun processNB(executionRequest: ExecutionServiceInput) {
        val instanceApi = initPluginApi()

        log.info("Health check script execution - START")
        val configValueSetup: ObjectNode = getDynamicProperties("config-deploy-setup") as ObjectNode
        log.info("Config Value Setup: $configValueSetup")

        val instanceHealthCheckList = startInstanceHealthCheck(configValueSetup, instanceApi)
        getStatuses(instanceHealthCheckList, instanceApi)

        instanceHealthCheckList.forEach {
            instanceApi.deleteInstanceHealthCheck(it.heatStackId, it.healthCheckInstance!!)
        }

        log.info("Health check script execution - END")
    }

    private fun getStatuses(instanceHealthCheckList: List<HealthCheckInstance>, instanceApi: K8sPluginInstanceApi) {
        val processors = Runtime.getRuntime().availableProcessors()
        val threadFactory = ThreadFactoryBuilder().setNameFormat("Poll status-%d").build()
        val executorService = Executors.newFixedThreadPool(processors, threadFactory)

        val tasksList: List<GetInstanceHealthCheckTask> = instanceHealthCheckList.map { hc ->
            GetInstanceHealthCheckTask(hc, instanceApi)
        }

        val statuses: List<Status> = tasksList.map { task -> executorService.submit(task).get(5, TimeUnit.MINUTES) }

        // TODO: shutdown executor service

        log.info("Instances status: $statuses")
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

    class GetInstanceHealthCheckTask(
        private val hcInstance: HealthCheckInstance,
        private val instanceApi: K8sPluginInstanceApi
    ) : Callable<Status> {

        private val log = LoggerFactory.getLogger(K8sHealthCheck::class.java)!!

        override fun call(): Status {
            return pollStatusUntilNotRunning()
        }

        private fun pollStatusUntilNotRunning(): Status {
            log.info("TASK STARTED FOR: $hcInstance")
            val pollTimerTask = PollGetInstanceHealthCheck(hcInstance, instanceApi)
            Timer(false).schedule(pollTimerTask, 0, 10_000)

            return Status(hcInstance.heatStackId, pollTimerTask.status)
        }

        class PollGetInstanceHealthCheck(
            private val hcInstance: HealthCheckInstance,
            private val instanceApi: K8sPluginInstanceApi
        ) : TimerTask() {
            private val log = LoggerFactory.getLogger(PollGetInstanceHealthCheck::class.java)!!
            internal var status: String = ""

            override fun run() {
                poll()
            }

            private fun poll() {
                val response: K8sRbInstanceHealthCheck? = instanceApi.getInstanceHealthCheck(hcInstance.heatStackId, hcInstance.healthCheckInstance!!)
                status = response?.status!!
                if (!status.equals("RUNNING", true)) {
                    log.info("Poll status: $status")
                    cancel()
                }
            }
        }
    }

    data class HealthCheckInstance(val heatStackId: String, val healthCheckInstance: String?) {
        override fun toString(): String {
            return "HealthCheckInstance(heatStackId='$heatStackId', healthCheckInstance='$healthCheckInstance')"
        }
    }

    data class Status(val heatStackId: String, val status: String) {
        override fun toString(): String {
            return "Status(heatStackId='$heatStackId', status='$status')"
        }
    }

    override suspend fun recoverNB(runtimeException: RuntimeException, executionRequest: ExecutionServiceInput) {
        log.info("Executing Recovery")
        bluePrintRuntimeService.getBlueprintError().addError("${runtimeException.message}", getName())
    }
}
