#!/bin/bash
#############################################################################
#
# Copyright (c) 2017 AT&T Intellectual Property. All rights reserved.
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
#############################################################################

set -ex 

# Read configuration files
EXTERNAL_DNS=$(cat /opt/config/external_dns.txt)
MAC_ADDR=$(cat /opt/config/mac_addr.txt)
HTTP_PROXY=$(cat /opt/config/http_proxy.txt)
HTTPS_PROXY=$(cat /opt/config/https_proxy.txt)

if [ $HTTP_PROXY != "no_proxy" ]
then
    export http_proxy=$HTTP_PROXY
    export https_proxy=$HTTPS_PROXY
fi


# Download dependencies
apt-get update
apt-get install -y python python-pip

# Download scripts from Nexus
cp /opt/boot/dcae2_vm_init.sh /opt/dcae2_vm_init.sh
chmod +x /opt/dcae2_vm_init.sh

echo "DOCKER_OPTS=\"\$DOCKER_OPTS --raw-logs -H tcp://0.0.0.0:2376 -H unix:///var/run/docker.sock\"" >> /etc/default/docker
sed -i "/ExecStart/s/$/ -H tcp:\/\/0.0.0.0:2376 --raw-logs/g" /etc/systemd/system/docker.service
systemctl daemon-reload
service docker restart

# add hostname aliases
echo "$(cat /opt/config/dcae_ip_addr.txt) consul" >>/etc/hosts
echo "$(cat /opt/config/dcae_ip_addr.txt) dockerhost" >>/etc/hosts

# prepare the configurations needed by DCAEGEN2 installer
rm -rf /opt/app/config
mkdir -p /opt/app/config

# private key
sed -e 's/\\n/\n/g' /opt/config/priv_key | sed -e 's/^[ \t]*//g; s/[ \t]*$//g' > /opt/app/config/key
chmod 777 /opt/app/config/key

cd /opt
./dcae2_vm_init.sh
