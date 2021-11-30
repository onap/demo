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

import org.onap.ccsdk.cds.blueprintsprocessor.core.api.data.ExecutionServiceInput
import org.onap.ccsdk.cds.blueprintsprocessor.services.execution.AbstractScriptComponentFunction
import org.slf4j.LoggerFactory

open class SimpleErrorCheck : AbstractScriptComponentFunction() {

    private val log = LoggerFactory.getLogger(SimpleErrorCheck::class.java)!!

    override fun getName(): String {
        return "SimpleErrorCheck"
    }

    override suspend fun processNB(executionRequest: ExecutionServiceInput) {
        log.info("SIMPLE ERROR CHECK - START")

        log.info("SIMPLE ERROR CHECK - END")
    }

    override suspend fun recoverNB(runtimeException: RuntimeException, executionRequest: ExecutionServiceInput) {
        log.info("Executing Recovery")
        this.addError("${runtimeException.message}")
    }
}
