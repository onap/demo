#!/bin/bash

#*******************************************************************************
# Copyright 2017 Huawei Technologies Co., Ltd.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#*******************************************************************************

CLI_LATEST_BINARY="https://nexus.onap.org/content/repositories/releases/org/onap/cli/cli-zip/1.1.0/cli-zip-1.1.0.zip"
CLI_INSTALL_DIR=/opt/onap/cli
CLI_ZIP=cli.zip
CLI_BIN=/usr/bin/onap
export ONAP_CLI_HOME=$CLI_INSTALL_DIR
export CLI_PRODUCT_VERSION=onap-1.1

#create install dir
if [ -d $CLI_INSTALL_DIR ]
then
    mv $CLI_INSTALL_DIR $CLI_INSTALL_DIR/../cli_`date +"%m-%d-%y-%H-%M-%S"`
    rm $CLI_BIN
fi

mkdir -p $CLI_INSTALL_DIR
cd $CLI_INSTALL_DIR

#Download and unzip CLI
apt-get install -y wget unzip

#check for java
java -version
if [ $? == 127 ]
then
    apt-get install -y openjdk-8-jre
fi

wget -O $CLI_ZIP $CLI_LATEST_BINARY

unzip $CLI_ZIP
if [ ! -d ./data ]; then mkdir ./data; fi
if [ ! -d ./onap-cli-schema ]; then mkdir ./onap-cli-schema; fi
chmod +x ./bin/onap.sh

#Make onap available in path
ln ./bin/onap.sh $CLI_BIN

#Print the version
onap -v

cd -
