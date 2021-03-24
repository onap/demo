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
                "vnf"
            ],
            "resolution-key": "ra-test-resolution",
            "resource-assignment-properties": {
                "vpg-management-port": 100,
                "aic-cloud-region": "RegionOne",
                "vnf-model-customization-uuid": "d73864db-1f6e-4e54-a533-a96773c926a4",
                "service-instance-id": "2afee7c4-8b16-4f2f-a567-48fb7948abcf",
                "vnf-id": "51274ece-55ca-4cbc-b7c4-0da0dcc65d38",
                "vnf_name": "sample-vnf-name",
                "k8s-rb-profile-namespace": "vfw-namespace",
                "k8s-rb-profile-k8s-version": "1.18.9",
                "int_private1_net_cidr" : "192.168.10.0/24",
                "int_private2_net_cidr" : "192.168.20.0/24",
                "onap_private_net_cidr" : "10.0.0.0/16",
                "private1-prefix-id" : 2,
                "private2-prefix-id" : 1
            }
        }
    }
}' | jq '.payload | .["resource-assignment-response"] | .["meshed-template"] | .vnf | fromjson | .["resource-accumulator-resolved-data"] '

