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
	BITS=$(cat /opt/config/vdhcp_private_net_cidr.txt | cut -d"/" -f2)
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
apt-get install -y wget openjdk-8-jdk apt-transport-https ca-certificates kea-dhcp4-server g++ libcurl4-gnutls-dev libboost-dev kea-dev
sleep 1

#  download the kea hook 
cd /opt
wget $REPO_URL_ARTIFACTS/org/onap/demo/vnf/vcpe/vdhcp/$DEMO_ARTIFACTS_VERSION/kea-sdnc-notify-mod.tar.gz 
tar -zxvf kea-sdnc-notify-mod.tar.gz 
mv kea-sdnc-notify-mod  VDHCP
rm *.tar.gz
cd VDHCP
# build.sh takes a minute or two to run
./build.sh
mv kea-sdnc-notify.so /usr/local/lib


# Download vDHCP demo code for DHCP Server
#cd /opt

#wget $REPO_URL_BLOB/org.onap.demo/vnfs/vCPE/$INSTALL_SCRIPT_VERSION/v_dhcp_init.sh
#wget $REPO_URL_BLOB/org.onap.demo/vnfs/vCPE/$INSTALL_SCRIPT_VERSION/vdhcp.sh

#chmod +x v_dhcp_init.sh
#chmod +x vdhcp.sh

# Download DHCP config files
cd /opt/config
wget $REPO_URL_BLOB/org.onap.demo/vnfs/vCPE/$INSTALL_SCRIPT_VERSION/kea-dhcp4.conf
wget $REPO_URL_BLOB/org.onap.demo/vnfs/vCPE/$INSTALL_SCRIPT_VERSION/kea-sdnc-notify.conf

# Configure DHCP
mv kea-dhcp4.conf /etc/kea/kea-dhcp4.conf
mv kea-sdnc-notify.conf /etc/kea/kea-sdnc-notify.conf
sleep 1

######################################################################################
### KEA install automatically puts kea-dhcp4-server in /etc/init.d and does update.rc
# rc0.d/K01kea-dhcp4-server
#rc1.d/K01kea-dhcp4-server
#rc2.d/S03kea-dhcp4-server
#rc3.d/S03kea-dhcp4-server
#rc4.d/S03kea-dhcp4-server
#rc5.d/S03kea-dhcp4-server
#rc6.d/K01kea-dhcp4-server
######################################################################################
# Run instantiation script
#cd /opt
#mv vdhcp.sh /etc/init.d
#update-rc.d vdhcp.sh defaults
#./v_dhcp_init.sh
