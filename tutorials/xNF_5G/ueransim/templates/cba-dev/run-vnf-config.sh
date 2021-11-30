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
# ./run-vnf-config.sh assign/deploy
REQ_ID=`shuf -i 1-1000000 -n 1`
SUB_REQ_ID=$REQ_ID"-"`shuf -i 1-1000 -n 1`
ACTION=$1
curl --location --request POST 'http://10.254.184.164:30449/api/v1/execution-service/process' \
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
        "blueprintName": "free5GC",
        "blueprintVersion": "1.1.1",
        "actionName": "'config-$ACTION'",
        "mode": "sync"
    },
    "payload": {
       "'config-$ACTION-request'": {
           "resolution-key": "VF_ueransim_CNF_automated",
           "'config-$ACTION-properties'": {
               "service-instance-id": "6178e706-fbfc-42c9-a807-79cd8e4a1898",
               "vnf-id": "065ee10b-e56e-41bc-ae68-2c2ba8a25e8e",
               "vnf-name": "VF_ueransim_CNF_automated",
               "vnf-customization-uuid": "5e4fb0a9-e4ff-4c95-859e-4615fee88d3e"
           }
        }
    }
}' | jq '.payload | .["'config-$ACTION-response'"]'

