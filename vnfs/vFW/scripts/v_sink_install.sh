#!/bin/bash

REPO_URL_BLOB=$(cat /opt/config/repo_url_blob.txt)
INSTALL_SCRIPT_VERSION=$(cat /opt/config/install_script_version.txt)
CLOUD_ENV=$(cat /opt/config/cloud_env.txt)

# OpenStack network configuration
if [[ $CLOUD_ENV == "openstack" ]]
then
	echo 127.0.0.1 $(hostname) >> /etc/hosts

	# Allow remote login as root
	mv /root/.ssh/authorized_keys /root/.ssh/authorized_keys.bk
	cp /home/ubuntu/.ssh/authorized_keys /root/.ssh

	MTU=$(/sbin/ifconfig | grep MTU | sed 's/.*MTU://' | sed 's/ .*//' | sort -n | head -1)

	VSN_PRIVATE_IP_O=$(cat /opt/config/vsn_private_ip_0.txt)
	echo "auto eth1" >> /etc/network/interfaces
	echo "iface eth1 inet static" >> /etc/network/interfaces
	echo "    address $VSN_PRIVATE_IP_O" >> /etc/network/interfaces
	echo "    netmask 255.255.255.0" >> /etc/network/interfaces
	echo "    mtu $MTU" >> /etc/network/interfaces

	VSN_PRIVATE_IP_1=$(cat /opt/config/vsn_private_ip_1.txt)
	echo "auto eth2" >> /etc/network/interfaces
	echo "iface eth2 inet static" >> /etc/network/interfaces
	echo "    address $VSN_PRIVATE_IP_1" >> /etc/network/interfaces
	echo "    netmask 255.255.255.0" >> /etc/network/interfaces
	echo "    mtu $MTU" >> /etc/network/interfaces

	ifup eth1
	ifup eth2
fi

# Download required dependencies
add-apt-repository -y ppa:openjdk-r/ppa
apt-get update
apt-get install -y make wget openjdk-8-jdk apt-transport-https ca-certificates darkstat

# Configure and run Darkstat
sed -i "s/START_DARKSTAT=.*/START_DARKSTAT=yes/g" /etc/darkstat/init.cfg
sed -i "s/INTERFACE=.*/INTERFACE=\"-i eth1\"/g" /etc/darkstat/init.cfg
/etc/init.d/darkstat start

# Download scripts for virtual sink
cd /opt
wget $REPO_URL_BLOB/org.openecomp.demo/vnfs/vfw/$INSTALL_SCRIPT_VERSION/v_sink_init.sh
wget $REPO_URL_BLOB/org.openecomp.demo/vnfs/vfw/$INSTALL_SCRIPT_VERSION/vsink.sh
chmod +x v_sink_init.sh
chmod +x vsink.sh

# Run instantiation script
mv vsink.sh /etc/init.d
update-rc.d vsink.sh defaults
./v_sink_init.sh