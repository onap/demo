#!/bin/sh

# ============LICENSE_START=======================================================
# Copyright (C) 2022 Deutche Telekom AG
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
        "blueprintName": "APACHE",
        "blueprintVersion": "1.0.0",
        "actionName": "'config-upgrade-$ACTION'",
        "mode": "sync"
    },
    "payload": {
       "'config-upgrade-$ACTION-request'": {
           "resolution-key": "VF_apache_k8s_demo_CNF",
           "'config-upgrade-$ACTION-properties'": {
               "service-instance-id": "5e6c49cc-e569-4eb9-bc9e-f7ce0ffb798f",
               "service-model-uuid": "0997c40e-63b7-419d-a13b-6a5ce97334bb",
               "vnf-id": "935fe87e-9bbd-4828-add2-d90196f5f382",
               "vnf-name": "Instance_apache-3_0",
               "vnf-customization-uuid": "6b6ff775-a170-4ce4-bcd0-85645d738390"
           }
        }
    }
}' | jq '.payload | .["'config-upgrade-$ACTION-response'"]'

