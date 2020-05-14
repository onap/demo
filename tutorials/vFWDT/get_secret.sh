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

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

`$DIR/yq > /dev/null 2>&1`

if [ $? -ne 0 ]; then
	echo "Install yq"
	wget -qcO $DIR/yq https://github.com/mikefarah/yq/releases/download/2.4.0/yq_linux_amd64 
	chmod 755 $DIR/yq
fi

echo `kubectl get secret $1 -o jsonpath="{.data.password}" | base64 --decode`
