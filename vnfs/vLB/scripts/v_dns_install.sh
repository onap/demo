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
apt-get install -y wget openjdk-8-jdk bind9 bind9utils bind9-doc apt-transport-https ca-certificates
sleep 1

# Download vDNS demo code for DNS Server
mkdir /opt/config
mkdir /opt/FDclient
cd /opt

wget $REPO_URL_BLOB/org.openecomp.demo/vnfs/vlb/$INSTALL_SCRIPT_VERSION/v_dns_init.sh
wget $REPO_URL_BLOB/org.openecomp.demo/vnfs/vlb/$INSTALL_SCRIPT_VERSION/vdns.sh
wget $REPO_URL_BLOB/org.openecomp.demo/vnfs/vlb/$INSTALL_SCRIPT_VERSION/dnsclient.sh
wget $REPO_URL_BLOB/org.openecomp.demo/vnfs/vlb/$INSTALL_SCRIPT_VERSION/set_gre_tunnel.sh
wget $REPO_URL_ARTIFACTS/org/openecomp/demo/vnf/vlb/dns-client/$DEMO_ARTIFACTS_VERSION/dns-client-$DEMO_ARTIFACTS_VERSION.jar

mv dns-client-$DEMO_ARTIFACTS_VERSION.jar /opt/FDclient/
mv dnsclient.sh /opt/FDclient/
mv set_gre_tunnel.sh /opt/FDclient/

chmod +x v_dns_init.sh
chmod +x vdns.sh
chmod +x /opt/FDclient/dnsclient.sh
chmod +x /opt/FDclient/set_gre_tunnel.sh

# Download Bind config files
cd /opt/config
wget $REPO_URL_BLOB/org.openecomp.demo/vnfs/vlb/$INSTALL_SCRIPT_VERSION/db_dnsdemo_openecomp_org
wget $REPO_URL_BLOB/org.openecomp.demo/vnfs/vlb/$INSTALL_SCRIPT_VERSION/named.conf.options
wget $REPO_URL_BLOB/org.openecomp.demo/vnfs/vlb/$INSTALL_SCRIPT_VERSION/named.conf.local

# Configure Bind
modprobe ip_gre
mkdir /etc/bind/zones
sed -i "s/OPTIONS=.*/OPTIONS=\"-4 -u bind\"/g" /etc/default/bind9
mv db_dnsdemo_openecomp_org /etc/bind/zones/db.dnsdemo.openecomp.org
mv named.conf.options /etc/bind/
mv named.conf.local /etc/bind/
sleep 1

# Run instantiation script
cd /opt
mv vdns.sh /etc/init.d
update-rc.d vdns.sh defaults
./v_dns_init.sh