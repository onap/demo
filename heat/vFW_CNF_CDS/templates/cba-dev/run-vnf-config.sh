#!/bin/sh

# ============LICENSE_START=======================================================
# Copyright (C) 2021 Orange
# ================================================================================
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# ============LICENSE_END=========================================================

REQ_ID=`shuf -i 1-1000000 -n 1`
SUB_REQ_ID=$REQ_ID"-"`shuf -i 1-1000 -n 1`
ACTION=$1

curl --location --request POST 'http://localhost:8081/api/v1/execution-service/process' \
--header 'Authorization: Basic Y2NzZGthcHBzOmNjc2RrYXBwcw==' \
--header 'Content-Type: application/json' \
--data-raw '{
    "commonHeader": {
        "originatorId": "onap-me-cm-adapter",
        "requestId": "'$REQ_ID'",
        "subRequestId": "'$SUB_REQ_ID'"
    },
    "actionIdentifiers": {
        "blueprintName": "vFW_CNF_CDS",
        "blueprintVersion": "8.0.0",
        "actionName": "'config-$ACTION'",
        "mode": "sync"
    },
    "payload": {
       "'config-$ACTION-request'": {
           "resolution-key": "VF_vfw_k8s_demo_CNF_KUD-6",
           "'config-$ACTION-properties'": {
               "service-instance-id": "889670f7-ed49-41b0-a251-b43e9a035811",
               "service-model-uuid": "bea61c93-1a90-426b-9fbe-6024bde48419",
               "vnf-id": "317f28f3-37b4-40c8-8062-e93fda15db99",
               "vnf-name": "VF_vfw_k8s_demo_CNF_KUD",
               "vnf-customization-uuid": "2793ba6f-332d-4694-8f8e-0b1f2ec3a732"
           }
        }
    }
}' | jq '.payload | .["'config-$ACTION-response'"]'

