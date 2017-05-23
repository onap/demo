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

	# Allow remote login as root
	mv /root/.ssh/authorized_keys /root/.ssh/authorized_keys.bk
	cp /home/ubuntu/.ssh/authorized_keys /root/.ssh
fi

# Download required dependencies
add-apt-repository -y ppa:openjdk-r/ppa
apt-get update
apt-get install -y make wget openjdk-8-jdk gcc libcurl4-openssl-dev python-pip bridge-utils apt-transport-https ca-certificates
pip install jsonschema

# Download vFirewall demo code for packet generator
mkdir /opt/config
mkdir /opt/honeycomb
cd /opt
wget $REPO_URL_BLOB/org.openecomp.demo/vnfs/vlb/$INSTALL_SCRIPT_VERSION/v_packetgen_for_dns_demo_init.sh
wget $REPO_URL_BLOB/org.openecomp.demo/vnfs/vlb/$INSTALL_SCRIPT_VERSION/vpacketgenfordnsdemo.sh
wget $REPO_URL_BLOB/org.openecomp.demo/vnfs/vlb/$INSTALL_SCRIPT_VERSION/run_streams_dns.sh
wget $REPO_URL_BLOB/org.openecomp.demo/vnfs/vlb/$INSTALL_SCRIPT_VERSION/vdnspacketgen_change_streams_ports.sh
wget $REPO_URL_ARTIFACTS/org/openecomp/demo/vnf/sample-distribution/$DEMO_ARTIFACTS_VERSION/sample-distribution-$DEMO_ARTIFACTS_VERSION-hc.tar.gz
wget $REPO_URL_ARTIFACTS/org/openecomp/demo/vnf/vlb/vlb_dns_streams/$DEMO_ARTIFACTS_VERSION/vlb_dns_streams-$DEMO_ARTIFACTS_VERSION-demo.tar.gz 

tar -zxvf vpp.tar.gz
tar -zxvf sample-distribution-$DEMO_ARTIFACTS_VERSION-hc.tar.gz
mv sample-distribution-$DEMO_ARTIFACTS_VERSION honeycomb
sed -i 's/"restconf-binding-address": "127.0.0.1",/"restconf-binding-address": "0.0.0.0",/g' honeycomb/sample-distribution-$DEMO_ARTIFACTS_VERSION/config/honeycomb.json
tar -zxvf vlb_dns_streams-$DEMO_ARTIFACTS_VERSION-demo.tar.gz
mv vlb_dns_streams-$DEMO_ARTIFACTS_VERSION dns_streams
rm *.tar.gz
chmod +x v_packetgen_for_dns_demo_init.sh
chmod +x vpacketgenfordnsdemo.sh
chmod +x run_streams_dns.sh
chmod +x vdnspacketgen_change_streams_ports.sh

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
mv vpacketgenfordnsdemo.sh /etc/init.d
update-rc.d vpacketgenfordnsdemo.sh defaults
./v_packetgen_for_dns_demo_init.sh
