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
ARTIFACTS_VERSION=$(cat /opt/config/artifacts_version.txt)
DNS_IP_ADDR=$(cat /opt/config/dns_ip_addr.txt)
CLOUD_ENV=$(cat /opt/config/cloud_env.txt)
EXTERNAL_DNS=$(cat /opt/config/external_dns.txt)
MAC_ADDR=$(cat /opt/config/mac_addr.txt)

MTU=$(/sbin/ifconfig | grep MTU | sed 's/.*MTU://' | sed 's/ .*//' | sort -n | head -1)

if [[ $CLOUD_ENV != "rackspace" ]]
then
	# Add host name to /etc/host to avoid warnings in openstack images
	echo "127.0.0.1 $(hostname)" >> /etc/hosts

	# Allow remote login as root
	mv /root/.ssh/authorized_keys /root/.ssh/authorized_keys.bk
	cp /home/ubuntu/.ssh/authorized_keys /root/.ssh
fi

# Download dependencies
#echo "deb http://ppa.launchpad.net/openjdk-r/ppa/ubuntu $(lsb_release -c -s) main" >>  /etc/apt/sources.list.d/java.list
#echo "deb-src http://ppa.launchpad.net/openjdk-r/ppa/ubuntu $(lsb_release -c -s) main" >>  /etc/apt/sources.list.d/java.list
apt-get update
#apt-get install --allow-unauthenticated -y apt-transport-https ca-certificates wget make openjdk-8-jdk git ntp ntpdate python python-pip
apt-get install --allow-unauthenticated -y apt-transport-https ca-certificates wget git ntp ntpdate python python-pip

# Download scripts from Nexus
unzip -p -j /opt/boot-$ARTIFACTS_VERSION.zip dcae2_vm_init.sh > /opt/dcae2_vm_init.sh
unzip -p -j /opt/boot-$ARTIFACTS_VERSION.zip dcae2_serv.sh > /opt/dcae2_serv.sh
chmod +x /opt/dcae2_vm_init.sh
chmod +x /opt/dcae2_serv.sh
mv /opt/dcae2_serv.sh /etc/init.d
update-rc.d dcae2_serv.sh defaults

# Download and install docker-engine and docker-compose
echo "deb https://apt.dockerproject.org/repo ubuntu-xenial main" | sudo tee /etc/apt/sources.list.d/docker.list
apt-get update
apt-get install -y "linux-image-extra-$(uname -r)" linux-image-extra-virtual jq
apt-get install -y --allow-unauthenticated docker-engine

mkdir -p /opt/docker
curl -L "https://github.com/docker/compose/releases/download/1.9.0/docker-compose-$(uname -s)-$(uname -m)" > /opt/docker/docker-compose
chmod +x /opt/docker/docker-compose


# Set the MTU size of docker containers to the minimum MTU size supported by vNICs. OpenStack deployments may 
# need to know the external DNS IP
DNS_FLAG=""
if [ -s /opt/config/dns_ip_addr.txt ]
then
	DNS_FLAG=$DNS_FLAG"--dns $(cat /opt/config/dns_ip_addr.txt) "
fi
if [ -s /opt/config/external_dns.txt ]
then
	DNS_FLAG=$DNS_FLAG"--dns $(cat /opt/config/external_dns.txt) "
fi
echo "DOCKER_OPTS=\"$DNS_FLAG--mtu=$MTU --raw-logs -H tcp://0.0.0.0:2376 -H unix:///var/run/docker.sock\"" >> /etc/default/docker

cp /lib/systemd/system/docker.service /etc/systemd/system
sed -i "/ExecStart/s/$/ --mtu=$MTU/g" /etc/systemd/system/docker.service
sed -i "/ExecStart/s/$/ -H tcp:\/\/0.0.0.0:2376 --raw-logs/g" /etc/systemd/system/docker.service
systemctl daemon-reload
service docker restart

# add hostname aliases
echo "$(cat /opt/config/dcae_ip_addr.txt) consul" >>/etc/hosts
echo "$(cat /opt/config/dcae_ip_addr.txt) dockerhost" >>/etc/hosts

# DNS IP address configuration
echo "nameserver $DNS_IP_ADDR" >> /etc/resolvconf/resolv.conf.d/head
resolvconf -u


# prepare the configurations needed by DCAEGEN2 installer
rm -rf /opt/app/config
mkdir -p /opt/app/config


# private key
sed -e 's/\\n/\n/g' /opt/config/priv_key | sed -e 's/^[ \t]*//g; s/[ \t]*$//g' > /opt/app/config/key
chmod 777 /opt/app/config/key

# move keystone url file
#cp /opt/config/keystone_url.txt /opt/app/config/keystone_url.txt


cd /opt
./dcae2_vm_init.sh
