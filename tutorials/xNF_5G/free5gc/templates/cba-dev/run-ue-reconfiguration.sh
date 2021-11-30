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
ACTION='ran-reconfiguration'

curl --location --request POST 'http://portal.api.simpledemo.onap.org:30449/api/v1/execution-service/process' \
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
        "actionName": "'$ACTION'",
        "mode": "sync"
    },
    "payload": {
       "'$ACTION-request'": {
           "resolution-key": "VF_ueransim_CNF",
           "'$ACTION-properties'": {
               "service-instance-id": "26ac11f6-2371-4e5e-98fc-885363312dff",
               "vnf-id": "73ce7403-d667-4804-ab80-eaf801792a47",
               "vnf-name": "VF_ueransim_CNF_210803",
               "vnf-customization-uuid": "822cbae8-d023-4047-95c3-4f834569e195"
           }
        }
    }
}' | jq '.payload | .["'$ACTION-response'"]'

