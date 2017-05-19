#!/bin/bash

REPO_URL_BLOB=$(cat /opt/config/repo_url_blob.txt)
REPO_URL_ARTIFACTS=$(cat /opt/config/repo_url_artifacts.txt)
DEMO_ARTIFACTS_VERSION=$(cat /opt/config/demo_artifacts_version.txt)
INSTALL_SCRIPT_VERSION=$(cat /opt/config/install_script_version.txt)
CLOUD_ENV=$(cat /opt/config/cloud_env.txt)

# OpenStack network configuration
if [[ $CLOUD_ENV == "openstack" ]]
then
	echo 127.0.0.1 $(hostname) >> /etc/hosts
	MTU=$(/sbin/ifconfig | grep MTU | sed 's/.*MTU://' | sed 's/ .*//' | sort -n | head -1)

	VFW_PRIVATE_IP_O=$(cat /opt/config/vfw_private_ip_0.txt)
	echo "auto eth1" >> /etc/network/interfaces
	echo "iface eth1 inet static" >> /etc/network/interfaces
	echo "    address $VFW_PRIVATE_IP_O" >> /etc/network/interfaces
	echo "    netmask 255.255.255.0" >> /etc/network/interfaces
	echo "    mtu $MTU" >> /etc/network/interfaces

	VFW_PRIVATE_IP_1=$(cat /opt/config/vfw_private_ip_1.txt)
	echo "auto eth2" >> /etc/network/interfaces
	echo "iface eth2 inet static" >> /etc/network/interfaces
	echo "    address $VFW_PRIVATE_IP_1" >> /etc/network/interfaces
	echo "    netmask 255.255.255.0" >> /etc/network/interfaces
	echo "    mtu $MTU" >> /etc/network/interfaces

	VFW_PRIVATE_IP_2=$(cat /opt/config/vfw_private_ip_2.txt)
	echo "auto eth3" >> /etc/network/interfaces
	echo "iface eth3 inet static" >> /etc/network/interfaces
	echo "    address $VFW_PRIVATE_IP_2" >> /etc/network/interfaces
	echo "    netmask 255.255.255.0" >> /etc/network/interfaces
	echo "    mtu $MTU" >> /etc/network/interfaces

	ifup eth1
	ifup eth2
	ifup eth3
fi

# Download required dependencies
add-apt-repository -y ppa:openjdk-r/ppa
apt-get update
apt-get install -y make wget openjdk-8-jdk gcc libcurl4-openssl-dev python-pip bridge-utils apt-transport-https ca-certificates
pip install jsonschema

# Download artifacts for virtual firewall
mkdir /opt/honeycomb
cd /opt

wget $REPO_URL_BLOB/org.openecomp.demo/vnfs/vfw/$INSTALL_SCRIPT_VERSION/v_firewall_init.sh
wget $REPO_URL_BLOB/org.openecomp.demo/vnfs/vfw/$INSTALL_SCRIPT_VERSION/vfirewall.sh
wget $REPO_URL_ARTIFACTS/org/openecomp/demo/vnf/sample-distribution/$DEMO_ARTIFACTS_VERSION/sample-distribution-$DEMO_ARTIFACTS_VERSION-hc.tar.gz
wget $REPO_URL_ARTIFACTS/org/openecomp/demo/vnf/ves/ves/$DEMO_ARTIFACTS_VERSION/ves-$DEMO_ARTIFACTS_VERSION-demo.tar.gz
wget $REPO_URL_ARTIFACTS/org/openecomp/demo/vnf/ves/ves_vfw_reporting/$DEMO_ARTIFACTS_VERSION/ves_vfw_reporting-$DEMO_ARTIFACTS_VERSION-demo.tar.gz

tar -zxvf ves-$DEMO_ARTIFACTS_VERSION-demo.tar.gz
mv ves-$DEMO_ARTIFACTS_VERSION VES
tar -zxvf ves_vfw_reporting-$DEMO_ARTIFACTS_VERSION-demo.tar.gz
mv ves_vfw_reporting-$DEMO_ARTIFACTS_VERSION VESreporting_vFW
tar -zxvf sample-distribution-$DEMO_ARTIFACTS_VERSION-hc.tar.gz
mv sample-distribution-$DEMO_ARTIFACTS_VERSION honeycomb
sed -i 's/"restconf-binding-address": "127.0.0.1",/"restconf-binding-address": "0.0.0.0",/g' honeycomb/sample-distribution-$DEMO_ARTIFACTS_VERSION/config/honeycomb.json
mv VESreporting_vFW /opt/VES/code/evel_training/VESreporting
rm *.tar.gz
chmod +x v_firewall_init.sh
chmod +x vfirewall.sh

# Install VPP
export UBUNTU="trusty"
export RELEASE=".stable.1609"
rm /etc/apt/sources.list.d/99fd.io.list
echo "deb [trusted=yes] https://nexus.fd.io/content/repositories/fd.io$RELEASE.ubuntu.$UBUNTU.main/ ./" | sudo tee -a /etc/apt/sources.list.d/99fd.io.list
apt-get update
apt-get install -y vpp vpp-dpdk-dkms vpp-lib vpp-dbg vpp-plugins vpp-dev
sleep 1

# Install VES
cd /opt/VES/bldjobs/
make clean
make
sleep 1

# Run instantiation script
cd /opt
mv vfirewall.sh /etc/init.d
update-rc.d vfirewall.sh defaults
./v_firewall_init.sh