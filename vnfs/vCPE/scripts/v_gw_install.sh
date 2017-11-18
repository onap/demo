#!/bin/bash

REPO_URL_BLOB=$(cat /opt/config/repo_url_blob.txt)
REPO_URL_ARTIFACTS=$(cat /opt/config/repo_url_artifacts.txt)
DEMO_ARTIFACTS_VERSION=$(cat /opt/config/demo_artifacts_version.txt)
INSTALL_SCRIPT_VERSION=$(cat /opt/config/install_script_version.txt)
VPP_SOURCE_REPO_URL=$(cat /opt/config/vpp_source_repo_url.txt)
VPP_SOURCE_REPO_BRANCH=$(cat /opt/config/vpp_source_repo_branch.txt)
HC2VPP_SOURCE_REPO_URL=$(cat /opt/config/hc2vpp_source_repo_url.txt)
HC2VPP_SOURCE_REPO_BRANCH=$(cat /opt/config/hc2vpp_source_repo_branch.txt)
CLOUD_ENV=$(cat /opt/config/cloud_env.txt)
MUX_GW_IP=$(cat /opt/config/mux_gw_private_net_ipaddr.txt)
MUX_GW_CIDR=$(cat /opt/config/mux_gw_private_net_cidr.txt)
MUX_IP_ADDR=$(cat /opt/config/mux_ip_addr.txt)
VG_VGMUX_TUNNEL_VNI=$(cat /opt/config/vg_vgmux_tunnel_vni.txt)

# Build states are:
# 'build' - just build the code
# 'done' - code is build, install and setup
# 'auto' - bulid, install and setup
BUILD_STATE="auto"
if [[ -f /opt/config/compile_state.txt ]]
then
    BUILD_STATE=$(cat /opt/config/compile_state.txt)
fi

# Convert Network CIDR to Netmask
cdr2mask () {
	# Number of args to shift, 255..255, first non-255 byte, zeroes
	set -- $(( 5 - ($1 / 8) )) 255 255 255 255 $(( (255 << (8 - ($1 % 8))) & 255 )) 0 0 0
	[ $1 -gt 1 ] && shift $1 || shift
	echo ${1-0}.${2-0}.${3-0}.${4-0}
}

# OpenStack network configuration
if [[ $BUILD_STATE != "build" ]]
then
    if [[ $CLOUD_ENV == "openstack" ]]
    then
	echo 127.0.0.1 $(hostname) >> /etc/hosts

	# Allow remote login as root
	mv /root/.ssh/authorized_keys /root/.ssh/authorized_keys.bk
	cp /home/ubuntu/.ssh/authorized_keys /root/.ssh

	MTU=$(/sbin/ifconfig | grep MTU | sed 's/.*MTU://' | sed 's/ .*//' | sort -n | head -1)

	IP=$(cat /opt/config/oam_ipaddr.txt)
	BITS=$(cat /opt/config/oam_cidr.txt | cut -d"/" -f2)
	NETMASK=$(cdr2mask $BITS)
	echo "auto eth2" >> /etc/network/interfaces
	echo "iface eth2 inet static" >> /etc/network/interfaces
	echo "    address $IP" >> /etc/network/interfaces
	echo "    netmask $NETMASK" >> /etc/network/interfaces
	echo "    mtu $MTU" >> /etc/network/interfaces

	ifup eth2
    fi
fi  # endif BUILD_STATE != "build"

if [[ $BUILD_STATE != "done" ]]
then
    # Download required dependencies
    echo "deb http://ppa.launchpad.net/openjdk-r/ppa/ubuntu $(lsb_release -c -s) main" >>  /etc/apt/sources.list.d/java.list
    echo "deb-src http://ppa.launchpad.net/openjdk-r/ppa/ubuntu $(lsb_release -c -s) main" >>  /etc/apt/sources.list.d/java.list
    apt-get --allow-unauthenticated update
    apt-get install --allow-unauthenticated -y wget openjdk-8-jdk apt-transport-https ca-certificates g++ libcurl4-gnutls-dev
    sleep 1

    # Install the tools required for download codes
    apt-get --allow-unauthenticated install -y expect git make linux-image-extra-`uname -r`

    #Download and build the VPP codes
    cd /opt
    git clone ${VPP_SOURCE_REPO_URL} -b ${VPP_SOURCE_REPO_BRANCH} vpp

    cd vpp
    make install-dep

    cd build-root
    ./bootstrap.sh
    make V=0 PLATFORM=vpp TAG=vpp install-deb

    # Install the VPP package
    dpkg -i *.deb
    systemctl stop vpp
fi  # endif BUILD_STATE != "done"

if [[ $BUILD_STATE != "build" ]]
then
    # Auto-start configuration for the VPP
    cat > /etc/vpp/startup.conf << EOF

unix {
  nodaemon
  log /tmp/vpp.log
  full-coredump
  cli-listen localhost:5002
  startup-config /etc/vpp/setup.gate
}

api-trace {
  on
}

api-segment {
  gid vpp
}

cpu {
	## In the VPP there is one main thread and optionally the user can create worker(s)
	## The main thread and worker thread(s) can be pinned to CPU core(s) manually or automatically

	## Manual pinning of thread(s) to CPU core(s)

	## Set logical CPU core where main thread runs
	# main-core 1

	## Set logical CPU core(s) where worker threads are running
	# corelist-workers 2-3,18-19

	## Automatic pinning of thread(s) to CPU core(s)

	## Sets number of CPU core(s) to be skipped (1 ... N-1)
	## Skipped CPU core(s) are not used for pinning main thread and working thread(s).
	## The main thread is automatically pinned to the first available CPU core and worker(s)
	## are pinned to next free CPU core(s) after core assigned to main thread
	# skip-cores 4

	## Specify a number of workers to be created
	## Workers are pinned to N consecutive CPU cores while skipping "skip-cores" CPU core(s)
	## and main thread's CPU core
	# workers 2

	## Set scheduling policy and priority of main and worker threads

	## Scheduling policy options are: other (SCHED_OTHER), batch (SCHED_BATCH)
	## idle (SCHED_IDLE), fifo (SCHED_FIFO), rr (SCHED_RR)
	# scheduler-policy fifo

	## Scheduling priority is used only for "real-time policies (fifo and rr),
	## and has to be in the range of priorities supported for a particular policy
	# scheduler-priority 50
}

# dpdk {
	## Change default settings for all intefaces
	# dev default {
		## Number of receive queues, enables RSS
		## Default is 1
		# num-rx-queues 3

		## Number of transmit queues, Default is equal
		## to number of worker threads or 1 if no workers treads
		# num-tx-queues 3

		## Number of descriptors in transmit and receive rings
		## increasing or reducing number can impact performance
		## Default is 1024 for both rx and tx
		# num-rx-desc 512
		# num-tx-desc 512

		## VLAN strip offload mode for interface
		## Default is off
		# vlan-strip-offload on
	# }

	## Whitelist specific interface by specifying PCI address
	# dev 0000:02:00.0

	## Whitelist specific interface by specifying PCI address and in
	## addition specify custom parameters for this interface
	# dev 0000:02:00.1 {
	#	num-rx-queues 2
	# }

	## Change UIO driver used by VPP, Options are: igb_uio, vfio-pci
	## and uio_pci_generic (default)
	# uio-driver vfio-pci

	## Disable mutli-segment buffers, improves performance but
	## disables Jumbo MTU support
	# no-multi-seg

	## Increase number of buffers allocated, needed only in scenarios with
	## large number of interfaces and worker threads. Value is per CPU socket.
	## Default is 16384
	# num-mbufs 128000

	## Change hugepages allocation per-socket, needed only if there is need for
	## larger number of mbufs. Default is 256M on each detected CPU socket
	# socket-mem 2048,2048
# }

EOF

# Get list of network device PCI bus addresses
    get_nic_pci_list() {
        while read -r line ; do
            if [ "$line" != "${line#*network device}" ]; then
                echo -n "${line%% *} "
            fi
        done < <(lspci)
    }

    NICS=$(get_nic_pci_list)
    NICS=`echo ${NICS} | sed 's/[0]\+\([0-9]\)/\1/g' | sed 's/[.:]/\//g'`

    MUX_GW_NIC=GigabitEthernet`echo ${NICS} | cut -d " " -f 2`  # second interface in list
    GW_PUB_NIC=GigabitEthernet`echo ${NICS} | cut -d " " -f 4`   # fourth interface in list

cat > /etc/vpp/setup.gate << EOF
set int state ${MUX_GW_NIC} up
set int ip address ${MUX_GW_NIC} ${MUX_GW_IP}/${MUX_GW_CIDR#*/}

set int state ${GW_PUB_NIC} up
set dhcp client intfc ${GW_PUB_NIC} hostname vg-1

create vxlan tunnel src ${MUX_GW_IP} dst ${MUX_IP_ADDR} vni ${VG_VGMUX_TUNNEL_VNI}
set interface l2 bridge vxlan_tunnel0 10 1
set bridge-domain arp term 10

loopback create
set int l2 bridge loop0 10 bvi 2
set int ip address loop0 192.168.1.254/24
set int state loop0 up
set int snat in loop0 out ${GW_PUB_NIC} 
snat add int address ${GW_PUB_NIC}

EOF

fi  # endif BUILD_STATE != "build"

if [[ $BUILD_STATE != "done" ]]
then

    # Download and install HC2VPP from source
    cd /opt
    git clone ${HC2VPP_SOURCE_REPO_URL} -b ${HC2VPP_SOURCE_REPO_BRANCH} hc2vpp

    apt --allow-unauthenticated install -y python-ply-lex-3.5 python-ply-yacc-3.5 python-pycparser python-cffi
    apt-get install -y maven
    mkdir -p ~/.m2
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

    cd hc2vpp
    mvn clean install
    l_version=$(cat pom.xml | grep "<version>" | head -1)
    l_version=$(echo "${l_version%<*}")
    l_version=$(echo "${l_version#*>}")
    mv vpp-integration/minimal-distribution/target/vpp-integration-distribution-${l_version}-hc/vpp-integration-distribution-${l_version} /opt/honeycomb
    sed -i 's/127.0.0.1/0.0.0.0/g' /opt/honeycomb/config/honeycomb.json

    # Disable automatic upgrades
    if [[ $CLOUD_ENV != "rackspace" ]]
    then
        echo "APT::Periodic::Unattended-Upgrade \"0\";" >> /etc/apt/apt.conf.d/10periodic
        sed -i 's/\(APT::Periodic::Unattended-Upgrade\) "1"/\1 "0"/' /etc/apt/apt.conf.d/20auto-upgrades
    fi
fi  # endif BUILD_STATE != "done

if [[ $BUILD_STATE != "build" ]]
then
    # Create systemctl service for Honeycomb
    cat > /etc/systemd/system/honeycomb.service << EOF
[Unit]
Description=Honeycomb Agent for the VPP control plane
Documentation=https://wiki.fd.io/view/Honeycomb
Requires=vpp.service
After=vpp.service

[Service]
ExecStart=/opt/honeycomb/honeycomb
Restart=always
RestartSec=10

[Install]
WantedBy=multi-user.target
EOF
    systemctl enable /etc/systemd/system/honeycomb.service

    # Install the DHCP server and config
    apt-get install -y isc-dhcp-server
    cat >> /etc/dhcp/dhcpd.conf << EOF
subnet 192.168.1.0 netmask 255.255.255.0 {
  range 192.168.1.2 192.168.1.253;
  option subnet-mask 255.255.255.0;
  option routers 192.168.1.254;
  option broadcast-address 192.168.1.255;
  default-lease-time 600;
  max-lease-time 7200;
}
EOF

echo '#!/bin/bash
STATUS=$(ip link show lstack)
if [ -z "$STATUS" ]
then
   vppctl tap connect lstack address 192.168.1.1/24
   vppctl set int state tap-0 up
   vppctl set interface l2 bridge tap-0 10 0
fi
IP=$(/sbin/ifconfig lstack | grep "inet addr:" | cut -d: -f2 | awk "{ print $1 }")
if [ ! -z "$STATUS" ] && [ -z "$IP" ]
then
   ip link delete lstack
   vppctl tap delete tap-0
   vppctl tap connect lstack address 192.168.1.1/24
   vppctl set int state tap-0 up
   vppctl set interface l2 bridge tap-0 10 0
fi' > /opt/v_gw_init.sh

    chmod +x v_gw_init.sh

    cat > /etc/systemd/system/vgw.service << EOF
[Unit]
Description=vGW service to run after honeycomb service
Requires=honeycomb.service
After=honeycomb.service

[Service]
ExecStart=/opt/v_gw_init.sh
Restart=always
RestartSec=10

[Install]
WantedBy=multi-user.target
EOF

    systemctl enable /etc/systemd/system/vgw.service

    cp /etc/systemd/system/multi-user.target.wants/isc-dhcp-server.service /etc/systemd/system/
    sed -i '/Documentation/a Wants=vgw.service\nAfter=vgw.service' /etc/systemd/system/isc-dhcp-server.service
    sed -i '/exec dhcpd/a Restart=always\nRestartSec=10' /etc/systemd/system/isc-dhcp-server.service

    # Rename network interface in openstack Ubuntu 16.04 images. Then, reboot the VM to pick up changes
    if [[ $CLOUD_ENV != "rackspace" ]]
    then
	sed -i "s/GRUB_CMDLINE_LINUX=.*/GRUB_CMDLINE_LINUX=\"net.ifnames=0 biosdevname=0\"/g" /etc/default/grub
	grub-mkconfig -o /boot/grub/grub.cfg
	sed -i "s/ens[0-9]*/eth0/g" /etc/network/interfaces.d/*.cfg
	sed -i "s/ens[0-9]*/eth0/g" /etc/udev/rules.d/70-persistent-net.rules
	echo 'network: {config: disabled}' >> /etc/cloud/cloud.cfg.d/99-disable-network-config.cfg
	reboot
fi

fi  # endif BUILD_STATE != "build"
