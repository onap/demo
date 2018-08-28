#!/bin/bash

NEXUS_ARTIFACT_REPO=$(cat /opt/config/nexus_artifact_repo.txt)
NB_API_VERSION=$(cat /opt/config/nb_api_version.txt)
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
fi

# Download required dependencies
echo "deb http://ppa.launchpad.net/openjdk-r/ppa/ubuntu $(lsb_release -c -s) main" >>  /etc/apt/sources.list.d/java.list
echo "deb-src http://ppa.launchpad.net/openjdk-r/ppa/ubuntu $(lsb_release -c -s) main" >>  /etc/apt/sources.list.d/java.list
apt-get update
apt-get install --allow-unauthenticated -y wget openjdk-8-jdk bind9 bind9utils bind9-doc apt-transport-https ca-certificates git maven
sleep 1

# Download vDNS demo code for DNS Server
cd /opt
unzip -p -j /opt/vlbms-scripts-$INSTALL_SCRIPT_VERSION.zip v_dns_init.sh > /opt/v_dns_init.sh
unzip -p -j /opt/vlbms-scripts-$INSTALL_SCRIPT_VERSION.zip vdns.sh > /opt/vdns.sh
unzip -p -j /opt/vlbms-scripts-$INSTALL_SCRIPT_VERSION.zip set_gre_tunnel.sh > /opt/set_gre_tunnel.sh
unzip -p -j /opt/vlbms-scripts-$INSTALL_SCRIPT_VERSION.zip properties.conf > /opt/config/properties.conf
unzip -p -j /opt/vlbms-scripts-$INSTALL_SCRIPT_VERSION.zip run_health_vdns.sh > /opt/run_health.sh

sed -i 's/primary=.*/primary=false/g' /opt/config/properties.conf
sed -i 's/vnfc=.*/vnfc=vDNS/g' /opt/config/properties.conf

# Clone Honeycomb interface for the VNF component
mkdir honeycomb-api
git init honeycomb-api
cd honeycomb-api
git remote add origin https://gerrit.onap.org/r/p/demo.git
git config core.sparsecheckout true
echo "vnfs/vLBMS/apis" >> .git/info/sparse-checkout
git pull --depth=1 origin beijing

mkdir ~/.m2
cat > ~/.m2/settings.xml << EOF
<?xml version="1.0" encoding="UTF-8"?>
<!-- vi: set et smarttab sw=2 tabstop=2: -->
<settings xmlns="http://maven.apache.org/SETTINGS/1.0.0"
  xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
  xsi:schemaLocation="http://maven.apache.org/SETTINGS/1.0.0 http://maven.apache.org/xsd/settings-1.0.0.xsd">
 
  <profiles>
    <profile>
      <id>fd.io-release</id>
      <repositories>
        <repository>
          <id>fd.io-mirror</id>
          <name>fd.io-mirror</name>
          <url>https://nexus.fd.io/content/groups/public/</url>
          <releases>
            <enabled>true</enabled>
            <updatePolicy>never</updatePolicy>
          </releases>
          <snapshots>
            <enabled>false</enabled>
          </snapshots>
        </repository>
      </repositories>
      <pluginRepositories>
        <pluginRepository>
          <id>fd.io-mirror</id>
          <name>fd.io-mirror</name>
          <url>https://nexus.fd.io/content/repositories/public/</url>
          <releases>
            <enabled>true</enabled>
            <updatePolicy>never</updatePolicy>
          </releases>
          <snapshots>
            <enabled>false</enabled>
          </snapshots>
        </pluginRepository>
      </pluginRepositories>
    </profile>
 
    <profile>
      <id>fd.io-snapshots</id>
      <repositories>
        <repository>
          <id>fd.io-snapshot</id>
          <name>fd.io-snapshot</name>
          <url>https://nexus.fd.io/content/repositories/fd.io.snapshot/</url>
          <releases>
            <enabled>false</enabled>
          </releases>
          <snapshots>
            <enabled>true</enabled>
          </snapshots>
        </repository>
      </repositories>
      <pluginRepositories>
        <pluginRepository>
          <id>fd.io-snapshot</id>
          <name>fd.io-snapshot</name>
          <url>https://nexus.fd.io/content/repositories/fd.io.snapshot/</url>
          <releases>
            <enabled>false</enabled>
          </releases>
          <snapshots>
            <enabled>true</enabled>
          </snapshots>
        </pluginRepository>
      </pluginRepositories>
    </profile>
    <profile>
      <id>opendaylight-snapshots</id>
      <repositories>
        <repository>
          <id>opendaylight-snapshot</id>
          <name>opendaylight-snapshot</name>
          <url>https://nexus.opendaylight.org/content/repositories/opendaylight.snapshot/</url>
          <releases>
            <enabled>false</enabled>
          </releases>
          <snapshots>
            <enabled>true</enabled>
          </snapshots>
        </repository>
      </repositories>
      <pluginRepositories>
        <pluginRepository>
          <id>opendaylight-shapshot</id>
          <name>opendaylight-snapshot</name>
          <url>https://nexus.opendaylight.org/content/repositories/opendaylight.snapshot/</url>
          <releases>
            <enabled>false</enabled>
          </releases>
          <snapshots>
            <enabled>true</enabled>
          </snapshots>
        </pluginRepository>
      </pluginRepositories>
    </profile>
  </profiles>
 
  <activeProfiles>
    <activeProfile>fd.io-release</activeProfile>
    <activeProfile>fd.io-snapshots</activeProfile>
    <activeProfile>opendaylight-snapshots</activeProfile>
  </activeProfiles>
</settings>
EOF

cd /opt/honeycomb-api/vnfs/vLBMS/apis
mvn clean install

sed -i 's/"restconf-binding-address": "127.0.0.1",/"restconf-binding-address": "0.0.0.0",/g' /opt/honeycomb-api/vnfs/vLBMS/apis/vlb-vnf-onap-distribution/target/vlb-vnf-onap-distribution-$NB_API_VERSION-hc/vlb-vnf-onap-distribution-$NB_API_VERSION/config/honeycomb.json
sed -i 's/"netconf-tcp-binding-address": "127.0.0.1",/"netconf-tcp-binding-address": "0.0.0.0",/g' /opt/honeycomb-api/vnfs/vLBMS/apis/vlb-vnf-onap-distribution/target/vlb-vnf-onap-distribution-$NB_API_VERSION-hc/vlb-vnf-onap-distribution-$NB_API_VERSION/config/honeycomb.json

cd /opt
chmod +x v_dns_init.sh
chmod +x vdns.sh
chmod +x set_gre_tunnel.sh
chmod +x run_health.sh

# Download Bind config files
cd /opt/config
unzip -p -j /opt/vlbms-scripts-$INSTALL_SCRIPT_VERSION.zip db_dnsdemo_onap_org > /opt/db_dnsdemo_onap_org
unzip -p -j /opt/vlbms-scripts-$INSTALL_SCRIPT_VERSION.zip named.conf.options > /opt/named.conf.options
unzip -p -j /opt/vlbms-scripts-$INSTALL_SCRIPT_VERSION.zip named.conf.local > /opt/named.conf.local

echo "bind9" > service.txt

# Configure Bind
modprobe ip_gre
mkdir /etc/bind/zones
sed -i "s/OPTIONS=.*/OPTIONS=\"-4 -u bind\"/g" /etc/default/bind9
mv db_dnsdemo_onap_org /etc/bind/zones/db.dnsdemo.onap.org
mv named.conf.options /etc/bind/
mv named.conf.local /etc/bind/
sleep 1

# Run instantiation script
cd /opt
mv vdns.sh /etc/init.d
update-rc.d vdns.sh defaults

# Rename network interface in openstack Ubuntu 16.04 images. Then, reboot the VM to pick up changes
if [[ $CLOUD_ENV != "rackspace" ]]
then
	sed -i "s/GRUB_CMDLINE_LINUX=.*/GRUB_CMDLINE_LINUX=\"net.ifnames=0 biosdevname=0\"/g" /etc/default/grub
	grub-mkconfig -o /boot/grub/grub.cfg
	sed -i "s/ens[0-9]*/eth0/g" /etc/network/interfaces.d/*.cfg
	sed -i "s/ens[0-9]*/eth0/g" /etc/udev/rules.d/70-persistent-net.rules
	echo 'network: {config: disabled}' >> /etc/cloud/cloud.cfg.d/99-disable-network-config.cfg
	echo "APT::Periodic::Unattended-Upgrade \"0\";" >> /etc/apt/apt.conf.d/10periodic
	reboot
fi

./v_dns_init.sh
