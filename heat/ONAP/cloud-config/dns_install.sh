#!/bin/bash

# Read configuration files
CLOUD_ENV=$(cat /opt/config/cloud_env.txt)
HTTP_PROXY=$(cat /opt/config/http_proxy.txt)
HTTPS_PROXY=$(cat /opt/config/https_proxy.txt)

if [ $HTTP_PROXY != "no_proxy" ]
then
    export http_proxy=$HTTP_PROXY
    export https_proxy=$HTTPS_PROXY
fi


if [[ $CLOUD_ENV != "rackspace" ]]
then
       # Set the Bind configuration file name based on the deployment environment
       ZONE_FILE="bind_zones"
       ZONE_ONAP="bind_zones_onap"
       OPTIONS_FILE="bind_options"
else
       ZONE_FILE="db_simpledemo_openecomp_org"
       ZONE_ONAP="db_simpledemo_onap_org"
       OPTIONS_FILE="named.conf.options"
fi

apt-get install -y bind9 bind9utils bind9-doc

# Download script
mkdir /etc/bind/zones
cp /opt/boot/$ZONE_FILE /etc/bind/zones/db.simpledemo.openecomp.org
cp /opt/boot/$ZONE_ONAP /etc/bind/zones/db.simpledemo.onap.org
cp /opt/boot/$OPTIONS_FILE /etc/bind/named.conf.options
cp /opt/boot/named.conf.local /etc/bind/named.conf.local

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
	sed -i "s/music_ip_addr/"$(cat /opt/config/music_ip_addr.txt)"/g" /etc/bind/zones/db.simpledemo.openecomp.org
	sed -i "s/oof_ip_addr/"$(cat /opt/config/oof_ip_addr.txt)"/g" /etc/bind/zones/db.simpledemo.openecomp.org
	sed -i "s/aaf_ip_addr/"$(cat /opt/config/aaf_ip_addr.txt)"/g" /etc/bind/zones/db.simpledemo.openecomp.org
	sed -i "s/sms_ip_addr/"$(cat /opt/config/sms_ip_addr.txt)"/g" /etc/bind/zones/db.simpledemo.openecomp.org
	sed -i "s/nbi_ip_addr/"$(cat /opt/config/nbi_ip_addr.txt)"/g" /etc/bind/zones/db.simpledemo.openecomp.org

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
	sed -i "s/music_ip_addr/"$(cat /opt/config/music_ip_addr.txt)"/g" /etc/bind/zones/db.simpledemo.onap.org
	sed -i "s/oof_ip_addr/"$(cat /opt/config/oof_ip_addr.txt)"/g" /etc/bind/zones/db.simpledemo.onap.org
	sed -i "s/aaf_ip_addr/"$(cat /opt/config/aaf_ip_addr.txt)"/g" /etc/bind/zones/db.simpledemo.onap.org
	sed -i "s/sms_ip_addr/"$(cat /opt/config/sms_ip_addr.txt)"/g" /etc/bind/zones/db.simpledemo.onap.org
	sed -i "s/nbi_ip_addr/"$(cat /opt/config/nbi_ip_addr.txt)"/g" /etc/bind/zones/db.simpledemo.onap.org
fi

# Configure Bind
modprobe ip_gre
sed -i "s/OPTIONS=.*/OPTIONS=\"-4 -u bind\"/g" /etc/default/bind9
service bind9 restart

