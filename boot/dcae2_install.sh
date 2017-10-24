#!/bin/bash

set -ex 

# Read configuration files
NEXUS_REPO=$(cat /opt/config/nexus_repo.txt)
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
echo "deb http://ppa.launchpad.net/openjdk-r/ppa/ubuntu $(lsb_release -c -s) main" >>  /etc/apt/sources.list.d/java.list
echo "deb-src http://ppa.launchpad.net/openjdk-r/ppa/ubuntu $(lsb_release -c -s) main" >>  /etc/apt/sources.list.d/java.list
apt-get update
apt-get install --allow-unauthenticated -y apt-transport-https ca-certificates wget make openjdk-8-jdk git ntp ntpdate python python-pip

# Download scripts from Nexus
curl -k "$NEXUS_REPO/org.onap.demo/boot/$ARTIFACTS_VERSION/dcae2_vm_init.sh" -o /opt/dcae2_vm_init.sh
curl -k "$NEXUS_REPO/org.onap.demo/boot/$ARTIFACTS_VERSION/dcae2_serv.sh" -o /opt/dcae2_serv.sh
chmod +x /opt/dcae2_vm_init.sh
chmod +x /opt/dcae2_serv.sh
mv /opt/dcae2_serv.sh /etc/init.d
update-rc.d dcae2_serv.sh defaults

# Download and install docker-engine and docker-compose
echo "deb https://apt.dockerproject.org/repo ubuntu-xenial main" | sudo tee /etc/apt/sources.list.d/docker.list
apt-get update
apt-get install -y "linux-image-extra-$(uname -r)" linux-image-extra-virtual
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
echo "DOCKER_OPTS=\"$DNS_FLAG--mtu=$MTU\"" >> /etc/default/docker

cp /lib/systemd/system/docker.service /etc/systemd/system
sed -i "/ExecStart/s/$/ --mtu=$MTU/g" /etc/systemd/system/docker.service
service docker restart


# DNS IP address configuration
echo "nameserver $DNS_IP_ADDR" >> /etc/resolvconf/resolv.conf.d/head
resolvconf -u


# prepare the configurations needed by DCAEGEN2 installer
rm -rf /opt/app/config
mkdir -p /opt/app/config


# private key
cp /opt/config/priv_key /opt/app/config/key
chmod 777 /opt/app/config/key

# move keystone url file
cp /opt/config/keystone_url.txt /opt/app/config/keystone_url.txt

# download blueprint input template files
rm -rf /opt/app/inputs-templates
mkdir -p /opt/app/inputs-templates
wget -P /opt/app/inputs-templates https://nexus.onap.org/service/local/repositories/raw/content/org.onap.dcaegen2.platform.blueprints/releases/input-templates/inputs.yaml
wget -P /opt/app/inputs-templates https://nexus.onap.org/service/local/repositories/raw/content/org.onap.dcaegen2.platform.blueprints/releases/input-templates/phinputs.yaml
wget -P /opt/app/inputs-templates https://nexus.onap.org/service/local/repositories/raw/content/org.onap.dcaegen2.platform.blueprints/releases/input-templates/dhinputs.yaml
wget -P /opt/app/inputs-templates https://nexus.onap.org/service/local/repositories/raw/content/org.onap.dcaegen2.platform.blueprints/releases/input-templates/invinputs.yaml


# generate blueprint input files
pip install jinja2
wget https://nexus.onap.org/service/local/repositories/raw/content/org.onap.dcaegen2.deployments/releases/scripts/detemplate-bpinputs.py && (python detemplate-bpinputs.py /opt/config /opt/app/inputs-templates /opt/app/config; rm detemplate-bpinputs.py)


# Run docker containers
cd /opt
./dcae2_vm_init.sh
