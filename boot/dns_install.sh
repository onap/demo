#!/bin/bash

# Read configuration files
NEXUS_REPO=$(cat /opt/config/nexus_repo.txt)
ARTIFACTS_VERSION=$(cat /opt/config/artifacts_version.txt)
CLOUD_ENV=$(cat /opt/config/cloud_env.txt)


if [[ $CLOUD_ENV != "rackspace" ]]
then
	# Add host name to /etc/host to avoid warnings in openstack images
	echo 127.0.0.1 $(hostname) >> /etc/hosts

	# Allow remote login as root
	mv /root/.ssh/authorized_keys /root/.ssh/authorized_keys.bk
	cp /home/ubuntu/.ssh/authorized_keys /root/.ssh

	# Set the Bind configuration file name based on the deployment environment
	ZONE_FILE="bind_zones"
	ZONE_ONAP="bind_zones_onap"
	OPTIONS_FILE="bind_options"
else
	ZONE_FILE="db_simpledemo_openecomp_org"
	ZONE_ONAP="db_simpledemo_onap_org"
	OPTIONS_FILE="named.conf.options"
fi

# Set private IP in /etc/network/interfaces manually in the presence of public interface
# Some VM images don't add the private interface automatically, we have to do it during the component installation
if [[ $CLOUD_ENV == "openstack_nofloat" ]]
then
	LOCAL_IP=$(cat /opt/config/dns_ip_addr.txt)
	CIDR=$(cat /opt/config/oam_network_cidr.txt)
	BITMASK=$(echo $CIDR | cut -d"/" -f2)

	# Compute the netmask based on the network cidr
	if [[ $BITMASK == "8" ]]
	then
		NETMASK=255.0.0.0
	elif [[ $BITMASK == "16" ]]
	then
		NETMASK=255.255.0.0
	elif [[ $BITMASK == "24" ]]
	then
		NETMASK=255.255.255.0
	fi

	echo "auto eth1" >> /etc/network/interfaces
	echo "iface eth1 inet static" >> /etc/network/interfaces
	echo "    address $LOCAL_IP" >> /etc/network/interfaces
	echo "    netmask $NETMASK" >> /etc/network/interfaces
	ifup eth1
fi

# Download dependencies
echo "deb http://ppa.launchpad.net/openjdk-r/ppa/ubuntu $(lsb_release -c -s) main" >>  /etc/apt/sources.list.d/java.list
echo "deb-src http://ppa.launchpad.net/openjdk-r/ppa/ubuntu $(lsb_release -c -s) main" >>  /etc/apt/sources.list.d/java.list
apt-get update
apt-get install --allow-unauthenticated -y apt-transport-https ca-certificates wget openjdk-8-jdk bind9 bind9utils bind9-doc ntp ntpdate make

# Download script
mkdir /etc/bind/zones
curl -k $NEXUS_REPO/org.onap.demo/boot/$ARTIFACTS_VERSION/$ZONE_FILE -o /etc/bind/zones/db.simpledemo.openecomp.org
curl -k $NEXUS_REPO/org.onap.demo/boot/$ARTIFACTS_VERSION/$ZONE_ONAP -o /etc/bind/zones/db.simpledemo.onap.org
curl -k $NEXUS_REPO/org.onap.demo/boot/$ARTIFACTS_VERSION/$OPTIONS_FILE -o /etc/bind/named.conf.options
curl -k $NEXUS_REPO/org.onap.demo/boot/$ARTIFACTS_VERSION/named.conf.local -o /etc/bind/named.conf.local



# Set the private IP address of each ONAP VM in the Bind configuration in OpenStack deployments
if [[ $CLOUD_ENV != "rackspace" ]]
then
        sed -i "s/dns_forwarder/"$(cat /opt/config/dns_forwarder.txt)"/g" /etc/bind/named.conf.options
	sed -i "s/dns_ip_addr/"$(cat /opt/config/dns_ip_addr.txt)"/g" /etc/bind/named.conf.options
	sed -i "s/external_dns/"$(cat /opt/config/external_dns.txt)"/g" /etc/bind/named.conf.options
	sed -i "s/aai1_ip_addr/"$(cat /opt/config/aai1_ip_addr.txt)"/g" /etc/bind/zones/db.simpledemo.openecomp.org
	sed -i "s/aai2_ip_addr/"$(cat /opt/config/aai2_ip_addr.txt)"/g" /etc/bind/zones/db.simpledemo.openecomp.org
	sed -i "s/appc_ip_addr/"$(cat /opt/config/appc_ip_addr.txt)"/g" /etc/bind/zones/db.simpledemo.openecomp.org
	sed -i "s/dcae_ip_addr/"$(cat /opt/config/dcae_ip_addr.txt)"/g" /etc/bind/zones/db.simpledemo.openecomp.org
	sed -i "s/dns_ip_addr/"$(cat /opt/config/dns_ip_addr.txt)"/g" /etc/bind/zones/db.simpledemo.openecomp.org
	sed -i "s/so_ip_addr/"$(cat /opt/config/so_ip_addr.txt)"/g" /etc/bind/zones/db.simpledemo.openecomp.org
	sed -i "s/mr_ip_addr/"$(cat /opt/config/mr_ip_addr.txt)"/g" /etc/bind/zones/db.simpledemo.openecomp.org
	sed -i "s/policy_ip_addr/"$(cat /opt/config/policy_ip_addr.txt)"/g" /etc/bind/zones/db.simpledemo.openecomp.org
	sed -i "s/portal_ip_addr/"$(cat /opt/config/portal_ip_addr.txt)"/g" /etc/bind/zones/db.simpledemo.openecomp.org
	sed -i "s/robot_ip_addr/"$(cat /opt/config/robot_ip_addr.txt)"/g" /etc/bind/zones/db.simpledemo.openecomp.org
	sed -i "s/sdc_ip_addr/"$(cat /opt/config/sdc_ip_addr.txt)"/g" /etc/bind/zones/db.simpledemo.openecomp.org
	sed -i "s/sdnc_ip_addr/"$(cat /opt/config/sdnc_ip_addr.txt)"/g" /etc/bind/zones/db.simpledemo.openecomp.org
	sed -i "s/vid_ip_addr/"$(cat /opt/config/vid_ip_addr.txt)"/g" /etc/bind/zones/db.simpledemo.openecomp.org
	sed -i "s/clamp_ip_addr/"$(cat /opt/config/clamp_ip_addr.txt)"/g" /etc/bind/zones/db.simpledemo.openecomp.org
	sed -i "s/openo_ip_addr/"$(cat /opt/config/openo_ip_addr.txt)"/g" /etc/bind/zones/db.simpledemo.openecomp.org

	sed -i "s/aai1_ip_addr/"$(cat /opt/config/aai1_ip_addr.txt)"/g" /etc/bind/zones/db.simpledemo.onap.org
	sed -i "s/aai2_ip_addr/"$(cat /opt/config/aai2_ip_addr.txt)"/g" /etc/bind/zones/db.simpledemo.onap.org
	sed -i "s/appc_ip_addr/"$(cat /opt/config/appc_ip_addr.txt)"/g" /etc/bind/zones/db.simpledemo.onap.org
	sed -i "s/dcae_ip_addr/"$(cat /opt/config/dcae_ip_addr.txt)"/g" /etc/bind/zones/db.simpledemo.onap.org
	sed -i "s/dns_ip_addr/"$(cat /opt/config/dns_ip_addr.txt)"/g" /etc/bind/zones/db.simpledemo.onap.org
	sed -i "s/so_ip_addr/"$(cat /opt/config/so_ip_addr.txt)"/g" /etc/bind/zones/db.simpledemo.onap.org
	sed -i "s/mr_ip_addr/"$(cat /opt/config/mr_ip_addr.txt)"/g" /etc/bind/zones/db.simpledemo.onap.org
	sed -i "s/policy_ip_addr/"$(cat /opt/config/policy_ip_addr.txt)"/g" /etc/bind/zones/db.simpledemo.onap.org
	sed -i "s/portal_ip_addr/"$(cat /opt/config/portal_ip_addr.txt)"/g" /etc/bind/zones/db.simpledemo.onap.org
	sed -i "s/robot_ip_addr/"$(cat /opt/config/robot_ip_addr.txt)"/g" /etc/bind/zones/db.simpledemo.onap.org
	sed -i "s/sdc_ip_addr/"$(cat /opt/config/sdc_ip_addr.txt)"/g" /etc/bind/zones/db.simpledemo.onap.org
	sed -i "s/sdnc_ip_addr/"$(cat /opt/config/sdnc_ip_addr.txt)"/g" /etc/bind/zones/db.simpledemo.onap.org
	sed -i "s/vid_ip_addr/"$(cat /opt/config/vid_ip_addr.txt)"/g" /etc/bind/zones/db.simpledemo.onap.org
	sed -i "s/clamp_ip_addr/"$(cat /opt/config/clamp_ip_addr.txt)"/g" /etc/bind/zones/db.simpledemo.onap.org
	sed -i "s/openo_ip_addr/"$(cat /opt/config/openo_ip_addr.txt)"/g" /etc/bind/zones/db.simpledemo.onap.org
fi

# Configure Bind
modprobe ip_gre
sed -i "s/OPTIONS=.*/OPTIONS=\"-4 -u bind\"/g" /etc/default/bind9
service bind9 restart

