#!/bin/bash

PROTECTED_NET_GW=$(cat /opt/config/protected_net_gw.txt)
UNPROTECTED_NET=$(cat /opt/config/unprotected_net.txt | cut -d'/' -f1)
REPO_URL_BLOB=$(cat /opt/config/repo_url_blob.txt)
REPO_URL_ARTIFACTS=$(cat /opt/config/repo_url_artifacts.txt)
DEMO_ARTIFACTS_VERSION=$(cat /opt/config/demo_artifacts_version.txt)
VSN_PRIVATE_IP_O=$(cat /opt/config/vsn_private_ip_0.txt)
VSN_PRIVATE_IP_1=$(cat /opt/config/vsn_private_ip_1.txt)
CLOUD_ENV=$(cat /opt/config/cloud_env.txt)


# Network configuration
if [[ $CLOUD_ENV == "openstack" ]]
then
	echo 127.0.0.1 $(hostname) >> /etc/hosts

	echo "auto eth1" >> /etc/network/interfaces
	echo "iface eth1 inet static" >> /etc/network/interfaces
	echo "    address $VSN_PRIVATE_IP_O" >> /etc/network/interfaces
	echo "    netmask 255.255.255.0" >> /etc/network/interfaces

	ifup eth1
fi

# Download required dependencies
add-apt-repository -y ppa:openjdk-r/ppa
apt-get update
apt-get install -y make wget openjdk-8-jdk apt-transport-https ca-certificates darkstat

# Configure and run darkstat
sed -i "s/START_DARKSTAT=.*/START_DARKSTAT=yes/g" /etc/darkstat/init.cfg
sed -i "s/INTERFACE=.*/INTERFACE=\"-i eth1\"/g" /etc/darkstat/init.cfg
/etc/init.d/darkstat start

# Download code for virtual sink
cd /opt
wget $REPO_URL_BLOB/org.openecomp.demo/vnfs/vfw/$DEMO_ARTIFACTS_VERSION/v_sink_init.sh
wget $REPO_URL_BLOB/org.openecomp.demo/vnfs/vfw/$DEMO_ARTIFACTS_VERSION/vsink.sh
chmod +x v_sink_init.sh
chmod +x vsink.sh

# Run instantiation script
mv vsink.sh /etc/init.d
update-rc.d vsink.sh defaults
./v_sink_init.sh