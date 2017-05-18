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

	VDNS_PRIVATE_IP_O=$(cat /opt/config/local_private_ipaddr.txt)
	echo "auto eth1" >> /etc/network/interfaces
	echo "iface eth1 inet static" >> /etc/network/interfaces
	echo "    address $VDNS_PRIVATE_IP_O" >> /etc/network/interfaces
	echo "    netmask 255.255.255.0" >> /etc/network/interfaces

	VDNS_PRIVATE_IP_1=$(cat /opt/config/oam_private_ipaddr.txt)
	echo "auto eth2" >> /etc/network/interfaces
	echo "iface eth2 inet static" >> /etc/network/interfaces
	echo "    address $VDNS_PRIVATE_IP_1" >> /etc/network/interfaces
	echo "    netmask 255.255.255.0" >> /etc/network/interfaces

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