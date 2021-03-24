#!/bin/sh

# ============LICENSE_START=======================================================
# Copyright (C) 2020 Orange
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
TEMPLATE_NAME="helm_base_template"

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
        "actionName": "resource-assignment",
        "mode": "sync"
    },
    "payload": {
        "resource-assignment-request": {
            "template-prefix": [
                "'$TEMPLATE_NAME'"
            ],
            "resolution-key": "ra-test-resolution",
            "resource-assignment-properties": {
                "vpg-management-port": 100,
                "aic-cloud-region": "RegionOne",
                "vnf-model-customization-uuid": "d73864db-1f6e-4e54-a533-a96773c926a4",
                "service-instance-id": "2afee7c4-8b16-4f2f-a567-48fb7948abcf",
                "vnf-id": "21dcbbd2-3ec2-4a9c-bb0d-599cafc16a1f",
                "vnf_name": "sample-vnf-name",
                "vf-module-name": "vf-module-name",
                "vf-module-label": "'$TEMPLATE_NAME'",
                "vf-module-type": "vf-module-type",
                "vf-module-model-customization-uuid": "d3ae2df9-95d4-48cc-a466-9f12dee80458",
                "vf-module-model-invariant-uuid": "564e55dc-3b90-4c9c-9e97-42f2c97d8f11",
                "vf-module-model-version": "3d55e2a6-7634-4ceb-98e9-2852d621a544",
                "vf-module-id": "3e6a0375-4b92-4bf5-9910-b0b893448a9c",
                "vf-naming-policy" : "SDNC_Policy.ONAP_NF_NAMING_TIMESTAMP",
                "k8s-rb-profile-name": "vfw-cnf-cds-base-profile",
                "management-prefix-id" : 3
            }
        }
    }
}' | jq '.payload | .["resource-assignment-response"] | .["meshed-template"] | .'$TEMPLATE_NAME' | fromjson | .["resource-accumulator-resolved-data"] '
