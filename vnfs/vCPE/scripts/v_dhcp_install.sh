#!/bin/bash

NEXUS_ARTIFACT_REPO=$(cat /opt/config/nexus_artifact_repo.txt)
DEMO_ARTIFACTS_VERSION=$(cat /opt/config/demo_artifacts_version.txt)
if [[ "$DEMO_ARTIFACTS_VERSION" =~ "SNAPSHOT" ]]; then REPO=snapshots; else REPO=releases; fi
INSTALL_SCRIPT_VERSION=$(cat /opt/config/install_script_version.txt)
CLOUD_ENV=$(cat /opt/config/cloud_env.txt)
MR_IP_ADDR=$(cat /opt/config/mr_ip_addr.txt)
MR_IP_PORT=$(cat /opt/config/mr_ip_port.txt)

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

	IP=$(cat /opt/config/cpe_signal_ipaddr.txt)
	BITS=$(cat /opt/config/cpe_signal_net_cidr.txt | cut -d"/" -f2)
	NETMASK=$(cdr2mask $BITS)
	echo "auto eth1" >> /etc/network/interfaces
	echo "iface eth1 inet static" >> /etc/network/interfaces
	echo "    address $IP" >> /etc/network/interfaces
	echo "    netmask $NETMASK" >> /etc/network/interfaces
	echo "    mtu $MTU" >> /etc/network/interfaces

	IP=$(cat /opt/config/oam_ipaddr.txt)
	BITS=$(cat /opt/config/oam_cidr.txt | cut -d"/" -f2)
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
apt-get install --allow-unauthenticated -y wget openjdk-8-jdk apt-transport-https ca-certificates kea-dhcp4-server g++ libcurl4-gnutls-dev libboost-dev kea-dev
sleep 1

# Download the kea hook
cd /opt
wget -O kea-sdnc-notify-mod-$DEMO_ARTIFACTS_VERSION-demo.tar.gz "${NEXUS_ARTIFACT_REPO}/service/local/artifact/maven/redirect?r=${REPO}&g=org.onap.demo.vnf.vcpe&a=kea-sdnc-notify-mod&c=demo&e=tar.gz&v=$DEMO_ARTIFACTS_VERSION"
tar -zxvf kea-sdnc-notify-mod-$DEMO_ARTIFACTS_VERSION-demo.tar.gz
mv kea-sdnc-notify-mod-$DEMO_ARTIFACTS_VERSION VDHCP
rm *.tar.gz
cd VDHCP
# build.sh takes a minute or two to run
./build.sh
mv build/kea-sdnc-notify.so /usr/local/lib

# Download DHCP config files
cd /opt
unzip -p -j /opt/vcpe-scripts-$INSTALL_SCRIPT_VERSION.zip kea-dhcp4.conf > /opt/kea-dhcp4.conf
unzip -p -j /opt/vcpe-scripts-$INSTALL_SCRIPT_VERSION.zip kea-sdnc-notify.conf > /opt/kea-sdnc-notify.conf
unzip -p -j /opt/vcpe-scripts-$INSTALL_SCRIPT_VERSION.zip v_dhcp_init.sh > /opt/v_dhcp_init.sh
unzip -p -j /opt/vcpe-scripts-$INSTALL_SCRIPT_VERSION.zip v_dhcp.sh > /opt/v_dhcp.sh
chmod +x v_dhcp_init.sh
chmod +x v_dhcp.sh
mv v_dhcp.sh /etc/init.d
update-rc.d v_dhcp.sh defaults

# Configure DHCP
cp kea-dhcp4.conf /etc/kea-dhcp4-server.conf
mv kea-dhcp4.conf /etc/kea/kea-dhcp4.conf
sed -i "s/DMAAP_IPADDR/"$MR_IP_ADDR"/g" kea-sdnc-notify.conf
sed -i "s/DMAAP_IPPORT/"$MR_IP_PORT"/g" kea-sdnc-notify.conf
mv kea-sdnc-notify.conf /etc/kea/kea-sdnc-notify.conf
sleep 1

######################################################################################
### KEA install automatically puts kea-dhcp4-server in /etc/init.d and does update.rc
# rc0.d/K01kea-dhcp4-server
#rc1.d/K01kea-dhcp4-server
#rc2.d/S03kea-dhcp4-server
#rc3.d/S03kea-dhcp4-server
#rc4.d/S03kea-dhcp4-server
#rc5.d/S03kea-dhcp4-server
#rc6.d/K01kea-dhcp4-server
######################################################################################

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

./v_dhcp_init.sh
