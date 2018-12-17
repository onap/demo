#!/bin/bash

NEXUS_ARTIFACT_REPO=$(cat /opt/config/nexus_artifact_repo.txt)
DEMO_ARTIFACTS_VERSION=$(cat /opt/config/demo_artifacts_version.txt)
if [[ "$DEMO_ARTIFACTS_VERSION" =~ "SNAPSHOT" ]]; then REPO=snapshots; else REPO=releases; fi
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

	IP=$(cat /opt/config/vpg_private_ip_0.txt)
	BITS=$(cat /opt/config/unprotected_private_net_cidr.txt | cut -d"/" -f2)
	NETMASK=$(cdr2mask $BITS)
	echo "auto eth1" >> /etc/network/interfaces
	echo "iface eth1 inet static" >> /etc/network/interfaces
	echo "    address $IP" >> /etc/network/interfaces
	echo "    netmask $NETMASK" >> /etc/network/interfaces
	echo "    mtu $MTU" >> /etc/network/interfaces

	IP=$(cat /opt/config/vpg_private_ip_1.txt)
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
echo "deb http://ppa.launchpad.net/openjdk-r/ppa/ubuntu $(lsb_release -c -s) main" >>  /etc/apt/sources.list.d/java.list
echo "deb-src http://ppa.launchpad.net/openjdk-r/ppa/ubuntu $(lsb_release -c -s) main" >>  /etc/apt/sources.list.d/java.list
apt-get update
apt-get install --allow-unauthenticated -y make wget openjdk-8-jdk gcc libcurl4-openssl-dev python-pip bridge-utils apt-transport-https ca-certificates
pip install jsonschema

# Download code for packet generator
mkdir /opt/honeycomb
cd /opt

unzip -p -j /opt/vfw-scripts-$INSTALL_SCRIPT_VERSION.zip v_packetgen_init.sh > /opt/v_packetgen_init.sh
unzip -p -j /opt/vfw-scripts-$INSTALL_SCRIPT_VERSION.zip vpacketgen.sh > /opt/vpacketgen.sh
unzip -p -j /opt/vfw-scripts-$INSTALL_SCRIPT_VERSION.zip run_traffic_fw_demo.sh > /opt/run_traffic_fw_demo.sh
wget -O sample-distribution-$DEMO_ARTIFACTS_VERSION-hc.tar.gz "${NEXUS_ARTIFACT_REPO}/service/local/artifact/maven/redirect?r=${REPO}&g=org.onap.demo.vnf&a=sample-distribution&c=hc&e=tar.gz&v=$DEMO_ARTIFACTS_VERSION"
wget -O vfw_pg_streams-$DEMO_ARTIFACTS_VERSION-demo.tar.gz "${NEXUS_ARTIFACT_REPO}/service/local/artifact/maven/redirect?r=${REPO}&g=org.onap.demo.vnf.vfw&a=vfw_pg_streams&c=demo&e=tar.gz&v=$DEMO_ARTIFACTS_VERSION"

tar -zmxvf sample-distribution-$DEMO_ARTIFACTS_VERSION-hc.tar.gz
tar -zmxvf vfw_pg_streams-$DEMO_ARTIFACTS_VERSION-demo.tar.gz
mv vfw_pg_streams-$DEMO_ARTIFACTS_VERSION pg_streams
mv sample-distribution-$DEMO_ARTIFACTS_VERSION honeycomb
sed -i 's/"restconf-binding-address": "127.0.0.1",/"restconf-binding-address": "0.0.0.0",/g' honeycomb/sample-distribution-$DEMO_ARTIFACTS_VERSION/config/honeycomb.json
rm *.tar.gz
chmod +x v_packetgen_init.sh
chmod +x vpacketgen.sh

# Install VPP
export UBUNTU="trusty"
export RELEASE=".stable.1609"
rm /etc/apt/sources.list.d/99fd.io.list
echo "deb [trusted=yes] https://nexus.fd.io/content/repositories/fd.io$RELEASE.ubuntu.$UBUNTU.main/ ./" | sudo tee -a /etc/apt/sources.list.d/99fd.io.list
apt-get update
apt-get install -y vpp vpp-dpdk-dkms vpp-lib vpp-dbg vpp-plugins vpp-dev
sleep 1

# Install honeycomb daily reboot script (workaround due to honeycomb file handle leak)
cat > /etc/cron.daily/honeycomb <<EOF
#!/bin/bash
VERSION=$(cat /opt/config/demo_artifacts_version.txt)
pkill java
/opt/honeycomb/sample-distribution-$VERSION/honeycomb &>/dev/null &disown
EOF
chmod +x /etc/cron.daily/honeycomb

# Run instantiation script
cd /opt
mv vpacketgen.sh /etc/init.d
update-rc.d vpacketgen.sh defaults
./v_packetgen_init.sh
