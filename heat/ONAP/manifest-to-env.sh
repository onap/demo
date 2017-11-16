#!/bin/bash
#==================LICENSE_START==========================================
#
# Copyright (c) 2017 Huawei Technologies Co., Ltd.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#        http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
#==================LICENSE_END============================================

# USAGE: Pipe in docker-manifest.csv from the integration repo.  This
# script converts it into a series of environment variable settings
# that can then be used with envsubst to set the docker versions in
# onap_openstack_template.env.
#
# EXAMPLE:
#   source <(./manifest-to-env.sh  < ~/Projects/onap/integration/version-manifest/src/main/resources/docker-manifest.csv)
#   envsubst < onap_openstack_template.env > onap_openstack.env

sed '1d' | awk -F , '{ v=$1; gsub(".*[./]","",$1); gsub("-","_",$1); print "export "  toupper($1) "_DOCKER=" $2 " # " v }'
