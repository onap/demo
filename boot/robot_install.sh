#!/bin/bash

# Read configuration files
NEXUS_REPO=$(cat /opt/config/nexus_repo.txt)
ARTIFACTS_VERSION=$(cat /opt/config/artifacts_version.txt)
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
curl -k $NEXUS_REPO/org.openecomp.demo/boot/$ARTIFACTS_VERSION/docker_key.txt -o /opt/config/docker_key.txt
curl -k $NEXUS_REPO/org.openecomp.demo/boot/$ARTIFACTS_VERSION/robot_vm_init.sh -o /opt/robot_vm_init.sh
curl -k $NEXUS_REPO/org.openecomp.demo/boot/$ARTIFACTS_VERSION/robot_serv.sh -o /opt/robot_serv.sh
chmod +x /opt/robot_vm_init.sh
chmod +x /opt/robot_serv.sh
mv /opt/robot_serv.sh /etc/init.d
update-rc.d robot_serv.sh defaults

# Download and install docker-engine and docker-compose
DOCKER_KEY=$(cat /opt/config/docker_key.txt)
apt-key adv --keyserver hkp://ha.pool.sks-keyservers.net:80 --recv-keys $DOCKER_KEY
echo "deb https://apt.dockerproject.org/repo ubuntu-xenial main" | sudo tee /etc/apt/sources.list.d/docker.list
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
mkdir -p /opt/eteshare/logs
mkdir -p /opt/eteshare/config
cd /opt
git clone -b $GERRIT_BRANCH --single-branch http://gerrit.onap.org/r/testsuite/properties.git testsuite/properties

# Rename network interface in openstack Ubuntu 16.04 images. Then, reboot the VM to pick up changes
if [[ $CLOUD_ENV == "openstack" ]]
then
	sed -i "s/GRUB_CMDLINE_LINUX=.*/GRUB_CMDLINE_LINUX=\"net.ifnames=0 biosdevname=0\"/g" /etc/default/grub
	grub-mkconfig -o /boot/grub/grub.cfg
	sed -i "s/ens[0-9]*/eth0/g" /etc/network/interfaces.d/*.cfg
	echo 'network: {config: disabled}' >> /etc/cloud/cloud.cfg.d/99-disable-network-config.cfg
	echo "APT::Periodic::Unattended-Upgrade \"0\";" >> /etc/apt/apt.conf.d/10periodic
	reboot
fi

# Run docker containers. For openstack Ubuntu 16.04 images this will run as a service after the VM has restarted
./robot_vm_init.sh