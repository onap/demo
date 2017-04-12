#!/bin/bash

# Read configuration files
NEXUS_REPO=$(cat /opt/config/nexus_repo.txt)
ARTIFACTS_VERSION=$(cat /opt/config/artifacts_version.txt)
DNS_IP_ADDR=$(cat /opt/config/dns_ip_addr.txt)
CLOUD_ENV=$(cat /opt/config/cloud_env.txt)
GERRIT_BRANCH=$(cat /opt/config/gerrit_branch.txt)

ZONE=$(cat /opt/config/dcae_zone.txt)
STATE=$(cat /opt/config/dcae_state.txt)
HORIZON_URL=$(cat /opt/config/horizon_url.txt)
OPENSTACK_USER=$(cat /opt/config/openstack_user.txt)
OPENSTACK_PASSWORD=$(cat /opt/config/openstack_password.txt)
OPENSTACK_KEYNAME=$(cat /opt/config/key_name.txt)$(cat /opt/config/rand_str.txt)"_dcae"
OPENSTACK_PUBKEY=$(cat /opt/config/pub_key.txt)
KEYSTONE_URL=$(cat /opt/config/keystone_url.txt)
OPENSTACK_TENANT_ID=$(cat /opt/config/tenant_id.txt)
OPENSTACK_TENANT_NAME=OPEN-ECOMP
OPENSTACK_REGION=$(cat /opt/config/openstack_region.txt)
OPENSTACK_PRIVATE_NETWORK=$(cat /opt/config/openstack_private_network_name.txt)

NEXUS_URL_ROOT=$(cat /opt/config/nexus_repo_root.txt)
NEXUS_USER=$(cat /opt/config/nexus_username.txt)
NEXUS_PASSWORD=$(cat /opt/config/nexus_password.txt)
NEXUS_URL_SNAPSHOTS=$(cat /opt/config/nexus_url_snapshots.txt)
DOCKER_REGISTRY=$(cat /opt/config/nexus_docker_repo.txt)

# Add host name to /etc/host to avoid warnings in openstack images
if [[ $CLOUD_ENV == "openstack" ]]
then
	echo 127.0.0.1 $(hostname) >> /etc/hosts
fi

# Download dependencies
add-apt-repository -y ppa:openjdk-r/ppa
apt-get update
apt-get install -y apt-transport-https ca-certificates wget make openjdk-8-jdk git ntp ntpdate

# Download scripts from Nexus
curl -k $NEXUS_REPO/org.openecomp.demo/boot/$ARTIFACTS_VERSION/docker_key.txt -o /opt/config/docker_key.txt
curl -k $NEXUS_REPO/org.openecomp.demo/boot/$ARTIFACTS_VERSION/dcae_vm_init.sh -o /opt/dcae_vm_init.sh
curl -k $NEXUS_REPO/org.openecomp.demo/boot/$ARTIFACTS_VERSION/dcae_serv.sh -o /opt/dcae_serv.sh
chmod +x /opt/dcae_vm_init.sh
chmod +x /opt/dcae_serv.sh
mv /opt/dcae_serv.sh /etc/init.d
update-rc.d dcae_serv.sh defaults

# Download and install docker-engine and docker-compose
DOCKER_KEY=$(cat /opt/config/docker_key.txt)
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

# Clone Gerrit repository
cd /opt
git clone -b $GERRIT_BRANCH --single-branch http://gerrit.onap.org/r/dcae/demo/startup/controller.git dcae-startup-vm-controller

# Build a configuration file for the DCAE Controller
cd /opt/dcae-startup-vm-controller
mkdir -p /opt/app/dcae-controller
cat > /opt/app/dcae-controller/config.yaml << EOF_CONFIG
ZONE: $ZONE
STATE: $STATE
DCAE-VERSION: $ARTIFACTS_VERSION
HORIZON-URL: $HORIZON_URL/$OPENSTACK_USER
KEYSTONE-URL: $KEYSTONE_URL
OPENSTACK-TENANT-ID: $OPENSTACK_TENANT_ID
OPENSTACK-TENANT-NAME: $OPENSTACK_TENANT_NAME
OPENSTACK-REGION: $OPENSTACK_REGION
OPENSTACK-PRIVATE-NETWORK: $OPENSTACK_PRIVATE_NETWORK
OPENSTACK-USER: $OPENSTACK_USER
OPENSTACK-PASSWORD: $OPENSTACK_PASSWORD
OPENSTACK-KEYNAME: $OPENSTACK_KEYNAME
OPENSTACK-PUBKEY: $OPENSTACK_PUBKEY

NEXUS-URL-ROOT: $NEXUS_URL_ROOT
NEXUS-USER: $NEXUS_USER
NEXUS-PASSWORD: $NEXUS_PASSWORD
NEXUS-URL-SNAPSHOTS: $NEXUS_URL_SNAPSHOTS
NEXUS-RAWURL: $NEXUS_REPO

DOCKER-REGISTRY: $DOCKER_REGISTRY

GIT-MR-REPO: http://gerrit.onap.org/r/dcae/demo/startup/message-router.git
EOF_CONFIG

# Run docker containers
cd /opt
./dcae_vm_init.sh