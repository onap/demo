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
import com.fasterxml.jackson.databind.ObjectMapper
import io.mockk.coEvery
import io.mockk.coVerify
import io.mockk.mockk
import io.mockk.mockkObject
import kotlinx.coroutines.runBlocking
import org.junit.Before
import org.junit.Test
import org.onap.ccsdk.cds.blueprintsprocessor.functions.resource.resolution.ResourceAssignmentRuntimeService
import org.onap.ccsdk.cds.blueprintsprocessor.functions.resource.resolution.utils.ResourceAssignmentUtils
import org.onap.ccsdk.cds.controllerblueprints.resource.dict.ResourceAssignment

class HealthCheckSetupTest {

    private val resourceAssignment = mockk<ResourceAssignment>()
    private var raRuntimeService = mockk<ResourceAssignmentRuntimeService>()
    private val systemUnderTest = HealthCheckSetup()
    private val vfModuleListInput: JsonNode = ObjectMapper().readTree("{\"vf-modules\":[{\"vf-module-id\":\"b906db15-3471-4c04-ade3-00481c30d5a4\",\"vf-module-name\":\"helm_Service_40612439-vfw\",\"heat-stack-id\":\"helm_Service_40612439-vfw/instance-id-test1\",\"orchestration-status\":\"Active\",\"is-base-vf-module\":true,\"automated-assignment\":false,\"resource-version\":\"1616517395747\",\"model-invariant-id\":\"10522355-6d2a-47b5-9c1e-f293ec2077be\",\"model-version-id\":\"93a09b17-e0a1-4981-a4ad-c549a7ef66ff\",\"model-customization-id\":\"954f6087-cd0f-4b66-870a-a6013c5d9ab1\",\"module-index\":0,\"relationship-list\":{\"relationship\":[{\"related-to\":\"vserver\",\"relationship-label\":\"org.onap.relationships.inventory.Uses\",\"related-link\":\"/aai/v19/cloud-infrastructure/cloud-regions/cloud-region/CloudOwner/RegionOne/tenants/tenant/b1ce7742d956463999923ceaed71786e/vservers/vserver/f647cad8-c749-4ad2-aeb0-3198a4d09298\",\"relationship-data\":[{\"relationship-key\":\"cloud-region.cloud-owner\",\"relationship-value\":\"CloudOwner\"},{\"relationship-key\":\"cloud-region.cloud-region-id\",\"relationship-value\":\"RegionOne\"},{\"relationship-key\":\"tenant.tenant-id\",\"relationship-value\":\"b1ce7742d956463999923ceaed71786e\"},{\"relationship-key\":\"vserver.vserver-id\",\"relationship-value\":\"f647cad8-c749-4ad2-aeb0-3198a4d09298\"}],\"related-to-property\":[{\"property-key\":\"vserver.vserver-name\",\"property-value\":\"ORAN_Service_40612439-vfw-node-mkpmfaviot6b\"}]}]}},{\"vf-module-id\":\"b906db15-3471-4c04-ade3-00481c30d5a4\",\"vf-module-name\":\"nohelm_Service_40612439-vfw\",\"heat-stack-id\":\"nohelm_Service_40612439-vfw/aba1be74-2c55-4f62-88cd-d4e2cf7e3be0\",\"orchestration-status\":\"Active\",\"is-base-vf-module\":true,\"automated-assignment\":false,\"resource-version\":\"1616517395747\",\"model-invariant-id\":\"10522355-6d2a-47b5-9c1e-f293ec2077be\",\"model-version-id\":\"93a09b17-e0a1-4981-a4ad-c549a7ef66ff\",\"model-customization-id\":\"954f6087-cd0f-4b66-870a-a6013c5d9ab1\",\"module-index\":0},{\"vf-module-id\":\"b906db15-3471-4c04-ade3-00481c30d5a4\",\"vf-module-name\":\"helm_Service_40612439-vfw\",\"heat-stack-id\":\"helm_Service_40612439-vfw/instance-id-test2\",\"orchestration-status\":\"Active\",\"is-base-vf-module\":true,\"automated-assignment\":false,\"resource-version\":\"1616517395747\",\"model-invariant-id\":\"10522355-6d2a-47b5-9c1e-f293ec2077be\",\"model-version-id\":\"93a09b17-e0a1-4981-a4ad-c549a7ef66ff\",\"model-customization-id\":\"954f6087-cd0f-4b66-870a-a6013c5d9ab1\",\"module-index\":0}]}")
    private val instanceIdListResult: JsonNode = ObjectMapper().readTree("{\"helm_instances\":[{\"instance-id\":\"instance-id-test1\"},{\"instance-id\":\"instance-id-test2\"}]}")

    @Before
    fun setup() {
        systemUnderTest.raRuntimeService = raRuntimeService
        mockkObject(ResourceAssignmentUtils.Companion)
        coEvery { ResourceAssignmentUtils.setResourceDataValue(resourceAssignment, raRuntimeService, any())} returns Unit
        coEvery { resourceAssignment.name  } returns ("health-check-setup")
        coEvery { raRuntimeService.getResolutionStore("vf-modules-list")} returns vfModuleListInput
    }

    @Test
    fun `Shall store valid instance-id list that can be input for K8sPlugin API to get status`() {
        runBlocking {
            systemUnderTest.processNB(resourceAssignment) }

        coVerify {
            ResourceAssignmentUtils.setResourceDataValue(resourceAssignment, raRuntimeService, instanceIdListResult)
        }
    }

}