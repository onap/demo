#!/bin/bash

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

	IP=$(cat /opt/config/vsn_private_ip_0.txt)
	BITS=$(cat /opt/config/protected_private_net_cidr.txt | cut -d"/" -f2)
	NETMASK=$(cdr2mask $BITS)
	echo "auto enp2s0" >> /etc/network/interfaces
	echo "iface enp2s0 inet static" >> /etc/network/interfaces
	echo "    address $IP" >> /etc/network/interfaces
	echo "    netmask $NETMASK" >> /etc/network/interfaces
	echo "    mtu $MTU" >> /etc/network/interfaces

	IP=$(cat /opt/config/vsn_private_ip_1.txt)
	BITS=$(cat /opt/config/onap_private_net_cidr.txt | cut -d"/" -f2)
	NETMASK=$(cdr2mask $BITS)
	echo "auto enp3s0" >> /etc/network/interfaces
	echo "iface enp3s0 inet static" >> /etc/network/interfaces
	echo "    address $IP" >> /etc/network/interfaces
	echo "    netmask $NETMASK" >> /etc/network/interfaces
	echo "    mtu $MTU" >> /etc/network/interfaces

	ifup enp2s0
	ifup enp3s0
fi

# Download required dependencies
echo "deb http://ppa.launchpad.net/openjdk-r/ppa/ubuntu $(lsb_release -c -s) main" >>  /etc/apt/sources.list.d/java.list
echo "deb-src http://ppa.launchpad.net/openjdk-r/ppa/ubuntu $(lsb_release -c -s) main" >>  /etc/apt/sources.list.d/java.list
apt-get update
apt-get install --allow-unauthenticated -y make wget openjdk-8-jdk apt-transport-https ca-certificates darkstat

# Configure and run Darkstat
sed -i "s/START_DARKSTAT=.*/START_DARKSTAT=yes/g" /etc/darkstat/init.cfg
sed -i "s/INTERFACE=.*/INTERFACE=\"-i enp2s0\"/g" /etc/darkstat/init.cfg
/etc/init.d/darkstat start

# Download scripts for virtual sink
cd /opt
unzip -p -j /opt/vfw-scripts-$INSTALL_SCRIPT_VERSION.zip v_sink_init.sh > /opt/v_sink_init.sh
unzip -p -j /opt/vfw-scripts-$INSTALL_SCRIPT_VERSION.zip vsink.sh > /opt/vsink.sh
chmod +x v_sink_init.sh
chmod +x vsink.sh

# Run instantiation script
mv vsink.sh /etc/init.d
update-rc.d vsink.sh defaults
./v_sink_init.sh
