#!/bin/bash

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

echo -e '\n\nUploading Affinity Policy\n'
curl --silent -i -k -X POST --user 'healthcheck:zb!XztG34' "https://policy-api:6969/policy/api/v1/policytypes/onap.policies.optimization.resource.AffinityPolicy/versions/1.0.0/policies" -H "X-ONAP-RequestID: e1763e61-9eef-4911-b952-1be1edd9812b" -H 'Content-Type: application/json' -d @/tmp/affinity_vFW_TD.json
sleep 3
echo -e '\n\nPushing Affinity Policy\n'
curl --silent -k -i -X POST --user 'healthcheck:zb!XztG34' "https://policy-pap:6969/policy/pap/v1/pdps/policies" -H 'Content-Type: application/json' -H "Accept: application/json" -d @/tmp/affinity_vFW_TD_push.json
sleep 3

echo -e '\n\nUploading Query Policy\n'
curl --silent -i -k -X POST --user 'healthcheck:zb!XztG34' "https://policy-api:6969/policy/api/v1/policytypes/onap.policies.optimization.service.QueryPolicy/versions/1.0.0/policies" -H "X-ONAP-RequestID: e1763e61-9eef-4911-b952-1be1edd9812b" -H 'Content-Type: application/json' -d @/tmp/QueryPolicy_vFW_TD.json
sleep 3
echo -e '\n\nPushing Query Policy\n'
curl --silent -k -i -X POST --user 'healthcheck:zb!XztG34' "https://policy-pap:6969/policy/pap/v1/pdps/policies" -H 'Content-Type: application/json' -H "Accept: application/json" -d @/tmp/queryPolicy_vFW_TD_push.json
sleep 3

echo -e '\n\nUploading vFW VNF Policy\n'
curl --silent -i -k -X POST --user 'healthcheck:zb!XztG34' "https://policy-api:6969/policy/api/v1/policytypes/onap.policies.optimization.resource.VnfPolicy/versions/1.0.0/policies" -H "X-ONAP-RequestID: e1763e61-9eef-4911-b952-1be1edd9812b" -H 'Content-Type: application/json' -d @/tmp/vnfPolicy_vFW_TD.json
sleep 3
echo -e '\n\nPushing vFW VNF Policy\n'
curl --silent -k -i -X POST --user 'healthcheck:zb!XztG34' "https://policy-pap:6969/policy/pap/v1/pdps/policies" -H 'Content-Type: application/json' -H "Accept: application/json" -d @/tmp/vnfPolicy_vFW_TD_push.json
sleep 3

echo -e '\n\nUploading vPGN VNF Policy\n'
curl --silent -i -k -X POST --user 'healthcheck:zb!XztG34' "https://policy-api:6969/policy/api/v1/policytypes/onap.policies.optimization.resource.VnfPolicy/versions/1.0.0/policies" -H "X-ONAP-RequestID: e1763e61-9eef-4911-b952-1be1edd9812b" -H 'Content-Type: application/json' -d @/tmp/vnfPolicy_vPGN_TD.json
sleep 3
echo -e '\n\nPushing vPGN VNF Policy\n'
curl --silent -k -i -X POST --user 'healthcheck:zb!XztG34' "https://policy-pap:6969/policy/pap/v1/pdps/policies" -H 'Content-Type: application/json' -H "Accept: application/json" -d @/tmp/vnfPolicy_vPGN_TD_push.json

