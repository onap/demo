#!/bin/bash

REPO_URL_BLOB=$(cat /opt/config/repo_url_blob.txt)
REPO_URL_ARTIFACTS=$(cat /opt/config/repo_url_artifacts.txt)
DEMO_ARTIFACTS_VERSION=$(cat /opt/config/demo_artifacts_version.txt)
INSTALL_SCRIPT_VERSION=$(cat /opt/config/install_script_version.txt)
CLOUD_ENV=$(cat /opt/config/cloud_env.txt)

# Convert Network CIDR to Netmask
cdr2mask () {
	# Number of args to shift, 255..255, first non-255 byte, zeroes
	set -- $(( 5 - ($1 / 8) )) 255 255 255 255 $(( (255 << (8 - ($1 % 8))) & 255 )) 0 0 0
	[ $1 -gt 1 ] && shift $1 || shift
	echo ${1-0}.${2-0}.${3-0}.${4-0}
}

# OpenStack network configuration
if [[ $CLOUD_ENV == "openstack" ]]
then
	echo 127.0.0.1 $(hostname) >> /etc/hosts

	# Allow remote login as root
	mv /root/.ssh/authorized_keys /root/.ssh/authorized_keys.bk
	cp /home/ubuntu/.ssh/authorized_keys /root/.ssh

	MTU=$(/sbin/ifconfig | grep MTU | sed 's/.*MTU://' | sed 's/ .*//' | sort -n | head -1)

	IP=$(cat /opt/config/vfw_private_ip_0.txt)
	BITS=$(cat /opt/config/unprotected_private_net_cidr.txt | cut -d"/" -f2)
	NETMASK=$(cdr2mask $BITS)
	echo "auto eth1" >> /etc/network/interfaces
	echo "iface eth1 inet static" >> /etc/network/interfaces
	echo "    address $IP" >> /etc/network/interfaces
	echo "    netmask $NETMASK" >> /etc/network/interfaces
	echo "    mtu $MTU" >> /etc/network/interfaces

	IP=$(cat /opt/config/vfw_private_ip_1.txt)
	BITS=$(cat /opt/config/protected_private_net_cidr.txt | cut -d"/" -f2)
	NETMASK=$(cdr2mask $BITS)
	echo "auto eth2" >> /etc/network/interfaces
	echo "iface eth2 inet static" >> /etc/network/interfaces
	echo "    address $IP" >> /etc/network/interfaces
	echo "    netmask $NETMASK" >> /etc/network/interfaces
	echo "    mtu $MTU" >> /etc/network/interfaces

	IP=$(cat /opt/config/vfw_private_ip_2.txt)
	BITS=$(cat /opt/config/onap_private_net_cidr.txt | cut -d"/" -f2)
	NETMASK=$(cdr2mask $BITS)
	echo "auto eth3" >> /etc/network/interfaces
	echo "iface eth3 inet static" >> /etc/network/interfaces
	echo "    address $IP" >> /etc/network/interfaces
	echo "    netmask $NETMASK" >> /etc/network/interfaces
	echo "    mtu $MTU" >> /etc/network/interfaces

	ifup eth1
	ifup eth2
	ifup eth3
fi

# Download required dependencies
echo "deb http://ppa.launchpad.net/openjdk-r/ppa/ubuntu $(lsb_release -c -s) main" >>  /etc/apt/sources.list.d/java.list
echo "deb-src http://ppa.launchpad.net/openjdk-r/ppa/ubuntu $(lsb_release -c -s) main" >>  /etc/apt/sources.list.d/java.list
apt-get update
apt-get install --allow-unauthenticated -y make wget openjdk-8-jdk gcc libcurl4-openssl-dev python-pip bridge-utils apt-transport-https ca-certificates
pip install jsonschema

# Download artifacts for virtual firewall
mkdir /opt/honeycomb
cd /opt

wget $REPO_URL_BLOB/org.onap.demo/vnfs/vfw/$INSTALL_SCRIPT_VERSION/v_firewall_init.sh
wget $REPO_URL_BLOB/org.onap.demo/vnfs/vfw/$INSTALL_SCRIPT_VERSION/vfirewall.sh
wget $REPO_URL_ARTIFACTS/org/onap/demo/vnf/sample-distribution/$DEMO_ARTIFACTS_VERSION/sample-distribution-$DEMO_ARTIFACTS_VERSION-hc.tar.gz
wget $REPO_URL_ARTIFACTS/org/onap/demo/vnf/ves5/ves/$DEMO_ARTIFACTS_VERSION/ves-$DEMO_ARTIFACTS_VERSION-demo.tar.gz
wget $REPO_URL_ARTIFACTS/org/onap/demo/vnf/ves5/ves_vfw_reporting/$DEMO_ARTIFACTS_VERSION/ves_vfw_reporting-$DEMO_ARTIFACTS_VERSION-demo.tar.gz

tar -zmxvf ves-$DEMO_ARTIFACTS_VERSION-demo.tar.gz
mv ves-$DEMO_ARTIFACTS_VERSION VES
tar -zmxvf ves_vfw_reporting-$DEMO_ARTIFACTS_VERSION-demo.tar.gz
mv ves_vfw_reporting-$DEMO_ARTIFACTS_VERSION VESreporting_vFW
tar -zmxvf sample-distribution-$DEMO_ARTIFACTS_VERSION-hc.tar.gz

mv sample-distribution-$DEMO_ARTIFACTS_VERSION honeycomb
sed -i 's/"restconf-binding-address": "127.0.0.1",/"restconf-binding-address": "0.0.0.0",/g' honeycomb/sample-distribution-$DEMO_ARTIFACTS_VERSION/config/honeycomb.json
mv VESreporting_vFW /opt/VES/evel/evel-library/code/VESreporting
rm *.tar.gz

chmod +x v_firewall_init.sh
chmod +x vfirewall.sh
chmod +x /opt/VES/evel/evel-library/code/VESreporting/go-client.sh

# Install VPP
export UBUNTU="trusty"
export RELEASE=".stable.1609"
rm /etc/apt/sources.list.d/99fd.io.list
echo "deb [trusted=yes] https://nexus.fd.io/content/repositories/fd.io$RELEASE.ubuntu.$UBUNTU.main/ ./" | sudo tee -a /etc/apt/sources.list.d/99fd.io.list
apt-get update
apt-get install -y vpp vpp-dpdk-dkms vpp-lib vpp-dbg vpp-plugins vpp-dev
sleep 1

# Install VES
cd /opt/VES/evel/evel-library/bldjobs/
make clean
make
sleep 1

# Run instantiation script
cd /opt
mv vfirewall.sh /etc/init.d
update-rc.d vfirewall.sh defaults
./v_firewall_init.sh