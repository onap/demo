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
import org.onap.ccsdk.cds.blueprintsprocessor.core.BluePrintPropertiesService
import org.onap.ccsdk.cds.blueprintsprocessor.core.api.data.ExecutionServiceInput
import org.onap.ccsdk.cds.blueprintsprocessor.functions.k8s.K8sConnectionPluginConfiguration
import org.onap.ccsdk.cds.blueprintsprocessor.functions.k8s.instance.K8sPluginInstanceApi
import org.onap.ccsdk.cds.blueprintsprocessor.functions.k8s.instance.K8sRbInstanceStatus
import org.onap.ccsdk.cds.blueprintsprocessor.services.execution.AbstractScriptComponentFunction
import org.onap.ccsdk.cds.controllerblueprints.core.BluePrintException
import org.slf4j.LoggerFactory

open class SimpleStatusCheck : AbstractScriptComponentFunction() {

    private val log = LoggerFactory.getLogger(SimpleStatusCheck::class.java)!!

    override fun getName(): String {
        return "SimpleStatusCheck"
    }

    override suspend fun processNB(executionRequest: ExecutionServiceInput) {
        log.info("SIMPLE STATUS CHECK - START")

        val configValueSetup: ObjectNode = getDynamicProperties("config-deploy-setup") as ObjectNode

        val bluePrintPropertiesService: BluePrintPropertiesService =
            this.functionDependencyInstanceAsType("bluePrintPropertiesService")

        val k8sConfiguration = K8sConnectionPluginConfiguration(bluePrintPropertiesService)

        val instanceApi = K8sPluginInstanceApi(k8sConfiguration)

        var checkCount: Int = 30 // in the future to be read in from the input
        while (checkCount > 0) {
            var continueCheck = false
            configValueSetup.fields().forEach { it ->
                val vfModuleName = it.key
                val instanceName = it.value.get("k8s-instance-id").asText()

                val instanceStatus: K8sRbInstanceStatus? = instanceApi.getInstanceStatus(instanceName)
                log.debug("Get status for $instanceName")
                var status = ""
                instanceStatus?.resourcesStatus?.forEach {
                    log.debug("Resource: name=$it.name kind=$it.gvk.kind group=$it.gvk.group version=$it.gvk.version")
                    if (it.gvk?.kind == "Pod") {
                        var version = it.gvk?.version!!
                        if (it.gvk?.group!! != "")
                            version = "${it.gvk?.group}/$version"
                        // val podStatus = instanceApi.queryInstanceStatus(instanceName, it.gvk?.kind!!, version, it.name, null)
                        // log.info(podStatus.toString())
                        val podState = it.status?.get("status") as Map<String, Object>
                        status = podState["phase"] as String
                        if (status != "Running") {
                            continueCheck = true
                            log.info("Pod ${it.name} [$vfModuleName] has INVALID state ${(podState["phase"])}")
                        } else {
                            log.info("Pod ${it.name} [$vfModuleName] has VALID state ${(podState["phase"])}")
                        }
                    }
                }
            }
            if (continueCheck) {
                checkCount--
                if (checkCount == 0)
                    throw BluePrintException("Pods State verification failed")
                Thread.sleep(10000L)
            } else
                checkCount = 0
        }

        log.info("SIMPLE STATUS CHECK - END SUCCESS")
    }

    override suspend fun recoverNB(runtimeException: RuntimeException, executionRequest: ExecutionServiceInput) {
        log.info("Executing Recovery")
        this.addError("${runtimeException.message}")
    }
}
