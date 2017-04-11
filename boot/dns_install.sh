#!/bin/bash

# Read configuration files
NEXUS_REPO=$(cat /opt/config/nexus_repo.txt)
ARTIFACT_VERSION=$(cat /opt/config/artifact_version.txt)
CLOUD_ENV=$(cat /opt/config/cloud_env.txt)

# Add host name to /etc/host to avoid warnings in openstack images
if [[ $CLOUD_ENV == "openstack" ]]
then
	echo 127.0.0.1 $(hostname) >> /etc/hosts
fi

# Download dependencies
add-apt-repository -y ppa:openjdk-r/ppa
apt-get update
apt-get install -y apt-transport-https ca-certificates wget openjdk-8-jdk bind9 bind9utils bind9-doc ntp ntpdate

# Set the Bind configuration file name based on the deployment environment
if [[ $CLOUD_ENV == "openstack" ]]
then
	ZONE_FILE="bind_zones"
	OPTIONS_FILE="bind_options"
else
	ZONE_FILE="db_simpledemo_openecomp_org"
	OPTIONS_FILE="named.conf.options"
fi

# Download script
mkdir /etc/bind/zones
curl -k $NEXUS_REPO/org.openecomp.demo/boot/$ARTIFACT_VERSION/$ZONE_FILE -o /etc/bind/zones/db.simpledemo.openecomp.org
curl -k $NEXUS_REPO/org.openecomp.demo/boot/$ARTIFACT_VERSION/$OPTIONS_FILE -o /etc/bind/named.conf.options
curl -k $NEXUS_REPO/org.openecomp.demo/boot/$ARTIFACT_VERSION/named.conf.local -o /etc/bind/named.conf.local

# Set the private IP address of each ONAP VM in the Bind configuration in OpenStack deployments
if [[ $CLOUD_ENV == "openstack" ]]
then
	A=$(cat /opt/config/dcae_ip_addr.txt | cut -d"." -f1)
	B=$(cat /opt/config/dcae_ip_addr.txt | cut -d"." -f2)
	C=$(cat /opt/config/dcae_ip_addr.txt | cut -d"." -f3)
	sed -i "s/aai_ip_addr/"$(cat /opt/config/aai_ip_addr.txt)"/g" /etc/bind/zones/db.simpledemo.openecomp.org
	sed -i "s/appc_ip_addr/"$(cat /opt/config/appc_ip_addr.txt)"/g" /etc/bind/zones/db.simpledemo.openecomp.org
	sed -i "s/dcae_ip_addr/"$(cat /opt/config/dcae_ip_addr.txt)"/g" /etc/bind/zones/db.simpledemo.openecomp.org
	sed -i "s/dns_ip_addr/"$(cat /opt/config/dns_ip_addr.txt)"/g" /etc/bind/zones/db.simpledemo.openecomp.org
	sed -i "s/mso_ip_addr/"$(cat /opt/config/mso_ip_addr.txt)"/g" /etc/bind/zones/db.simpledemo.openecomp.org
	sed -i "s/mr_ip_addr/"$(cat /opt/config/mr_ip_addr.txt)"/g" /etc/bind/zones/db.simpledemo.openecomp.org
	sed -i "s/policy_ip_addr/"$(cat /opt/config/policy_ip_addr.txt)"/g" /etc/bind/zones/db.simpledemo.openecomp.org
	sed -i "s/portal_ip_addr/"$(cat /opt/config/portal_ip_addr.txt)"/g" /etc/bind/zones/db.simpledemo.openecomp.org
	sed -i "s/robot_ip_addr/"$(cat /opt/config/robot_ip_addr.txt)"/g" /etc/bind/zones/db.simpledemo.openecomp.org
	sed -i "s/sdc_ip_addr/"$(cat /opt/config/sdc_ip_addr.txt)"/g" /etc/bind/zones/db.simpledemo.openecomp.org
	sed -i "s/sdnc_ip_addr/"$(cat /opt/config/sdnc_ip_addr.txt)"/g" /etc/bind/zones/db.simpledemo.openecomp.org
	sed -i "s/vid_ip_addr/"$(cat /opt/config/vid_ip_addr.txt)"/g" /etc/bind/zones/db.simpledemo.openecomp.org
	sed -i "s/dcae_coll_ip_addr/"$A.$B.$C"/g" /etc/bind/zones/db.simpledemo.openecomp.org
fi

# Configure Bind
modprobe ip_gre
sed -i "s/OPTIONS=.*/OPTIONS=\"-4 -u bind\"/g" /etc/default/bind9
service bind9 restart