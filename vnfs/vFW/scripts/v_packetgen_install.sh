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

	VPG_PRIVATE_IP_O=$(cat /opt/config/vpg_private_ip_0.txt)
	echo "auto eth1" >> /etc/network/interfaces
	echo "iface eth1 inet static" >> /etc/network/interfaces
	echo "    address $VPG_PRIVATE_IP_O" >> /etc/network/interfaces
	echo "    netmask 255.255.255.0" >> /etc/network/interfaces
	echo "    mtu $MTU" >> /etc/network/interfaces

	VPG_PRIVATE_IP_1=$(cat /opt/config/vpg_private_ip_1.txt)
	echo "auto eth2" >> /etc/network/interfaces
	echo "iface eth2 inet static" >> /etc/network/interfaces
	echo "    address $VPG_PRIVATE_IP_1" >> /etc/network/interfaces
	echo "    netmask 255.255.255.0" >> /etc/network/interfaces
	echo "    mtu $MTU" >> /etc/network/interfaces

	ifup eth1
	ifup eth2
fi

# Download required dependencies
add-apt-repository -y ppa:openjdk-r/ppa
apt-get update
apt-get install -y make wget openjdk-8-jdk gcc libcurl4-openssl-dev python-pip bridge-utils apt-transport-https ca-certificates
pip install jsonschema

# Download code for packet generator
mkdir /opt/honeycomb
cd /opt

wget $REPO_URL_BLOB/org.openecomp.demo/vnfs/vfw/$INSTALL_SCRIPT_VERSION/v_packetgen_init.sh
wget $REPO_URL_BLOB/org.openecomp.demo/vnfs/vfw/$INSTALL_SCRIPT_VERSION/vpacketgen.sh
wget $REPO_URL_BLOB/org.openecomp.demo/vnfs/vfw/$INSTALL_SCRIPT_VERSION/run_traffic_fw_demo.sh
wget $REPO_URL_ARTIFACTS/org/openecomp/demo/vnf/sample-distribution/$DEMO_ARTIFACTS_VERSION/sample-distribution-$DEMO_ARTIFACTS_VERSION-hc.tar.gz
wget $REPO_URL_ARTIFACTS/org/openecomp/demo/vnf/vfw/vfw_pg_streams/$DEMO_ARTIFACTS_VERSION/vfw_pg_streams-$DEMO_ARTIFACTS_VERSION-demo.tar.gz 

tar -zxvf sample-distribution-$DEMO_ARTIFACTS_VERSION-hc.tar.gz
tar -zxvf vfw_pg_streams-$DEMO_ARTIFACTS_VERSION-demo.tar.gz
mv vfw_pg_streams-$DEMO_ARTIFACTS_VERSION pg_streams
mv sample-distribution-$DEMO_ARTIFACTS_VERSION honeycomb
sed -i 's/"restconf-binding-address": "127.0.0.1",/"restconf-binding-address": "0.0.0.0",/g' honeycomb/sample-distribution-$DEMO_ARTIFACTS_VERSION/config/honeycomb.json
rm *.tar.gz
chmod +x v_packetgen_init.sh
chmod +x vpacketgen.sh

# Install VPP
export UBUNTU="trusty"
export RELEASE=".stable.1609"
rm /etc/apt/sources.list.d/99fd.io.list
echo "deb [trusted=yes] https://nexus.fd.io/content/repositories/fd.io$RELEASE.ubuntu.$UBUNTU.main/ ./" | sudo tee -a /etc/apt/sources.list.d/99fd.io.list
apt-get update
apt-get install -y vpp vpp-dpdk-dkms vpp-lib vpp-dbg vpp-plugins vpp-dev
sleep 1

# Run instantiation script
cd /opt
mv vpacketgen.sh /etc/init.d
update-rc.d vpacketgen.sh defaults
./v_packetgen_init.sh