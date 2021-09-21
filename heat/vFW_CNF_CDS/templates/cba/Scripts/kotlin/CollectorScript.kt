/*
 *  Copyright © 2021 Bell Canada.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

package org.onap.ccsdk.cds.blueprintsprocessor.services.execution.scripts

import com.fasterxml.jackson.databind.JsonNode
import org.onap.ccsdk.cds.blueprintsprocessor.core.api.data.ExecutionServiceInput
import org.onap.ccsdk.cds.blueprintsprocessor.services.execution.AbstractScriptComponentFunction
import org.onap.ccsdk.cds.blueprintsprocessor.services.execution.ComponentScriptExecutor
import org.onap.ccsdk.cds.controllerblueprints.core.BluePrintProcessorException
import org.onap.ccsdk.cds.controllerblueprints.core.asJsonNode
import org.onap.ccsdk.cds.controllerblueprints.core.logger

open class CollectorScript : AbstractScriptComponentFunction() {

    private val log = logger(CollectorScript::class)

    override suspend fun processNB(executionRequest: ExecutionServiceInput) {
        bluePrintRuntimeService.bluePrintContext()
                .serviceTemplate.topologyTemplate!!.nodeTemplates!!
                .keys.filter { it.startsWith("execute-script") }
                .associateWith { responseData(it) }
                .let { it.asJsonNode() }
                .also { log.info("Collected results: $it") }
                .let { setAttribute(ComponentScriptExecutor.ATTRIBUTE_RESPONSE_DATA, it) }
    }

    private fun responseData(nodeTemplateName: String): JsonNode? {
        return try {
            bluePrintRuntimeService.getNodeTemplateAttributeValue(nodeTemplateName,
                    ComponentScriptExecutor.ATTRIBUTE_RESPONSE_DATA)
        } catch (exception: BluePrintProcessorException) { null }
    }

    override suspend fun recoverNB(runtimeException: RuntimeException, executionRequest: ExecutionServiceInput) {
        addError(runtimeException.message ?: "Failed without error message")
    }
}
