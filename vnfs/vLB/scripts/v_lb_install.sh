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

	IP=$(cat /opt/config/local_private_ipaddr.txt)
	BITS=$(cat /opt/config/vlb_private_net_cidr.txt | cut -d"/" -f2)
	NETMASK=$(cdr2mask $BITS)
	echo "auto eth1" >> /etc/network/interfaces
	echo "iface eth1 inet static" >> /etc/network/interfaces
	echo "    address $IP" >> /etc/network/interfaces
	echo "    netmask $NETMASK" >> /etc/network/interfaces
	echo "    mtu $MTU" >> /etc/network/interfaces

	IP=$(cat /opt/config/oam_private_ipaddr.txt)
	BITS=$(cat /opt/config/onap_private_net_cidr.txt | cut -d"/" -f2)
	NETMASK=$(cdr2mask $BITS)
	echo "auto eth2" >> /etc/network/interfaces
	echo "iface eth2 inet static" >> /etc/network/interfaces
	echo "    address $IP" >> /etc/network/interfaces
	echo "    netmask $NETMASK" >> /etc/network/interfaces
	echo "    mtu $MTU" >> /etc/network/interfaces

	ifup eth1
	ifup eth2
fi

# Download required dependencies
add-apt-repository -y ppa:openjdk-r/ppa
apt-get update
apt-get install -y make gcc wget openjdk-8-jdk bridge-utils libcurl4-openssl-dev apt-transport-https ca-certificates
sleep 1

# Download vLB demo code for load balancer
mkdir /opt/config
mkdir /opt/FDserver
cd /opt

wget $REPO_URL_BLOB/org.openecomp.demo/vnfs/vlb/$INSTALL_SCRIPT_VERSION/v_lb_init.sh
wget $REPO_URL_BLOB/org.openecomp.demo/vnfs/vlb/$INSTALL_SCRIPT_VERSION/vlb.sh
wget $REPO_URL_BLOB/org.openecomp.demo/vnfs/vlb/$INSTALL_SCRIPT_VERSION/dnsmembership.sh
wget $REPO_URL_BLOB/org.openecomp.demo/vnfs/vlb/$INSTALL_SCRIPT_VERSION/add_dns.sh
wget $REPO_URL_BLOB/org.openecomp.demo/vnfs/vlb/$INSTALL_SCRIPT_VERSION/remove_dns.sh
wget $REPO_URL_ARTIFACTS/org/openecomp/demo/vnf/vlb/dns-manager/$DEMO_ARTIFACTS_VERSION/dns-manager-$DEMO_ARTIFACTS_VERSION.jar
wget $REPO_URL_ARTIFACTS/org/openecomp/demo/vnf/ves/ves/$DEMO_ARTIFACTS_VERSION/ves-$DEMO_ARTIFACTS_VERSION-demo.tar.gz
wget $REPO_URL_ARTIFACTS/org/openecomp/demo/vnf/ves/ves_vlb_reporting/$DEMO_ARTIFACTS_VERSION/ves_vlb_reporting-$DEMO_ARTIFACTS_VERSION-demo.tar.gz

tar -zxvf ves-$DEMO_ARTIFACTS_VERSION-demo.tar.gz
mv ves-$DEMO_ARTIFACTS_VERSION VES
tar -zxvf ves_vlb_reporting-$DEMO_ARTIFACTS_VERSION-demo.tar.gz
mv ves_vlb_reporting-$DEMO_ARTIFACTS_VERSION VESreporting_vLB

mv VESreporting_vLB /opt/VES/code/evel_training/VESreporting
mv dns-manager-$DEMO_ARTIFACTS_VERSION.jar /opt/FDserver/dns-manager-$DEMO_ARTIFACTS_VERSION.jar
mv dnsmembership.sh /opt/FDserver/dnsmembership.sh
mv add_dns.sh /opt/FDserver/add_dns.sh
mv remove_dns.sh /opt/FDserver/remove_dns.sh
rm *.tar.gz

chmod +x v_lb_init.sh
chmod +x vlb.sh
chmod +x /opt/VES/code/evel_training/VESreporting/go-client.sh
chmod +x /opt/FDserver/dnsmembership.sh
chmod +x /opt/FDserver/add_dns.sh
chmod +x /opt/FDserver/remove_dns.sh

# Create a file with public IP of the VM if it doesn't exist. This is for VMs directly attached to the external network.
if [ ! -e /opt/config/local_public_ipaddr.txt ]
then
	IP_ADDRESS=$(ifconfig eth0 | grep "inet addr" | tr -s ' ' | cut -d' ' -f3 | cut -d':' -f2)
	echo $IP_ADDRESS > /opt/config/local_public_ipaddr.txt
fi

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
mv vlb.sh /etc/init.d
update-rc.d vlb.sh defaults
./v_lb_init.sh