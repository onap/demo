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

cd ../cba/

mvn clean install

if [ $? -eq 1 ]
then
   echo "----------------"
   echo "CBA BUILD FAILED"
   echo "----------------"
   exit 1
fi

CBA_NAME=`ls target/*.zip`
cp $CBA_NAME ../cba-dev/cba.zip

echo "-----------------"
echo "CBA BUILD SUCCESS"
echo "-----------------"

cd ../cba-dev
