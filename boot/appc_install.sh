#!/bin/bash

# Read configuration files
NEXUS_REPO=$(cat /opt/config/nexus_repo.txt)
ARTIFACT_VERSION=$(cat /opt/config/artifact_version.txt)
DOCKER_KEY=$(cat /opt/config/docker_key.txt)
DNS_IP_ADDR=$(cat /opt/config/dns_ip_addr.txt)
CLOUD_ENV=$(cat /opt/config/cloud_env.txt)
GERRIT_BRANCH=$(cat /opt/config/gerrit_branch.txt)

# Add host name to /etc/host to avoid warnings in openstack images
if [[ $CLOUD_ENV == "openstack" ]]
then
	echo 127.0.0.1 $(hostname) >> /etc/hosts
fi

# Download dependencies
add-apt-repository -y ppa:openjdk-r/ppa
apt-get update
apt-get install -y apt-transport-https ca-certificates wget openjdk-8-jdk git ntp ntpdate

# Download scripts from Nexus
curl -k $NEXUS_REPO/org.openecomp.demo/boot/$ARTIFACT_VERSION/docker_key.txt -o /opt/config/docker_key.txt
curl -k $NEXUS_REPO/org.openecomp.demo/boot/$ARTIFACT_VERSION/appc_vm_init.sh -o /opt/appc_vm_init.sh
curl -k $NEXUS_REPO/org.openecomp.demo/boot/$ARTIFACT_VERSION/appc_serv.sh -o /opt/appc_serv.sh
chmod +x /opt/appc_vm_init.sh
chmod +x /opt/appc_serv.sh
mv /opt/appc_serv.sh /etc/init.d
update-rc.d appc_serv.sh defaults

# Download and install docker-engine and docker-compose
apt-key adv --keyserver hkp://ha.pool.sks-keyservers.net:80 --recv-keys $DOCKER_KEY
echo "deb https://apt.dockerproject.org/repo ubuntu-trusty main" | sudo tee /etc/apt/sources.list.d/docker.list
apt-get update
apt-get install -y linux-image-extra-$(uname -r) linux-image-extra-virtual
apt-get install -y docker-engine

mkdir /opt/docker
curl -L https://github.com/docker/compose/releases/download/1.9.0/docker-compose-`uname -s`-`uname -m` > /opt/docker/docker-compose
chmod +x /opt/docker/docker-compose

# DNS IP address configuration
echo "nameserver "$DNS_IP_ADDR >> /etc/resolvconf/resolv.conf.d/head
resolvconf -u

# Clone Gerrit repository and run docker containers
cd /opt
git clone -b $GERRIT_BRANCH --single-branch http://gerrit.onap.org/r/appc/deployment.git appc
./appc_vm_init.sh