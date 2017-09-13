#!/bin/bash

REPO_URL_BLOB=$(cat /opt/config/repo_url_blob.txt)
REPO_URL_ARTIFACTS=$(cat /opt/config/repo_url_artifacts.txt)
DEMO_ARTIFACTS_VERSION=$(cat /opt/config/demo_artifacts_version.txt)
INSTALL_SCRIPT_VERSION=$(cat /opt/config/install_script_version.txt)
VPP_SOURCE_REPO_URL=$(cat /opt/config/vpp_source_repo_url.txt)
VPP_SOURCE_REPO_BRANCH=$(cat /opt/config/vpp_source_repo_branch.txt)
VPP_PATCH_URL=$(cat /opt/config/vpp_patch_url.txt)
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

	IP=$(cat /opt/config/brgemu_bng_net_ipaddr.txt)
	BITS=$(cat /opt/config/brgemu_bng_net_cidr.txt | cut -d"/" -f2)
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

	IP=$(cat /opt/config/cpe_signal_net_ipaddr.txt)
	BITS=$(cat /opt/config/cpe_signal_net_cidr.txt | cut -d"/" -f2)
	NETMASK=$(cdr2mask $BITS)
	echo "auto eth3" >> /etc/network/interfaces
	echo "iface eth3 inet static" >> /etc/network/interfaces
	echo "    address $IP" >> /etc/network/interfaces
	echo "    netmask $NETMASK" >> /etc/network/interfaces
	echo "    mtu $MTU" >> /etc/network/interfaces

	IP=$(cat /opt/config/bng_gmux_net_ipaddr.txt)
	BITS=$(cat /opt/config/bng_gmux_net_cidr.txt | cut -d"/" -f2)
	NETMASK=$(cdr2mask $BITS)
	echo "auto eth4" >> /etc/network/interfaces
	echo "iface eth4 inet static" >> /etc/network/interfaces
	echo "    address $IP" >> /etc/network/interfaces
	echo "    netmask $NETMASK" >> /etc/network/interfaces
	echo "    mtu $MTU" >> /etc/network/interfaces

	ifup eth1
	ifup eth2
	ifup eth3
	ifup eth4
fi

# Download required dependencies
echo "deb http://ppa.launchpad.net/openjdk-r/ppa/ubuntu $(lsb_release -c -s) main" >>  /etc/apt/sources.list.d/java.list
echo "deb-src http://ppa.launchpad.net/openjdk-r/ppa/ubuntu $(lsb_release -c -s) main" >>  /etc/apt/sources.list.d/java.list
apt-get update
apt-get install --allow-unauthenticated -y wget openjdk-8-jdk apt-transport-https ca-certificates g++ libcurl4-gnutls-dev
sleep 1

# Install the tools required for download codes
apt-get install -y expect git patch

#Download and build the VPP codes
cd /opt
git clone ${VPP_SOURCE_REPO_URL} -b ${VPP_SOURCE_REPO_BRANCH} vpp
wget -O Vpp-Integrate-FreeRADIUS-Client-for-vBNG.patch ${VPP_PATCH_URL}

cd vpp
patch -p1 < Vpp-Integrate-FreeRADIUS-Client-for-vBNG.patch
expect -c "
        set timeout 60;
        spawn make install-dep;
        expect {
                \"Do you want to continue?*\" {send \"Y\r\"; interact}
        }
"

cd build-root
./bootstrap.sh
make V=0 PLATFORM=vpp TAG=vpp install-deb

# Install the VPP package
dpkg -i *.deb
systemctl stop vpp

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

cat > /etc/vpp/setup.gate << EOF
set int state GigabitEthernet0/8/0 up
set interface ip address GigabitEthernet0/8/0 10.4.0.4/24

set int state GigabitEthernet0/9/0 up
set interface ip address GigabitEthernet0/9/0 10.4.0.3/24

set vbng dhcp4 remote 10.4.0.1 local 10.4.0.3
set vbng aaa config /etc/vpp/vbng-aaa.cfg nas-port 5060
EOF

cat > /etc/vpp/vbng-aaa.cfg << EOF
# General settings

# specify which authentication comes first respectively which
# authentication is used. possible values are: "radius" and "local".
# if you specify "radius,local" then the RADIUS server is asked
# first then the local one. if only one keyword is specified only
# this server is asked.
auth_order	radius,local

# maximum login tries a user has
login_tries	2

# timeout for all login tries
# if this time is exceeded the user is kicked out
login_timeout	5

# name of the nologin file which when it exists disables logins.
# it may be extended by the ttyname which will result in
# a terminal specific lock (e.g. /etc/nologin.ttyS2 will disable
# logins on /dev/ttyS2)
nologin /etc/nologin

# name of the issue file. it's only display when no username is passed
# on the radlogin command line
issue	/usr/local/etc/radiusclient/issue

# RADIUS settings

# RADIUS server to use for authentication requests. this config
# item can appear more then one time. if multiple servers are
# defined they are tried in a round robin fashion if one
# server is not answering.
# optionally you can specify a the port number on which is remote
# RADIUS listens separated by a colon from the hostname. if
# no port is specified /etc/services is consulted of the radius
# service. if this fails also a compiled in default is used.
#authserver 	10.4.0.2
authserver      localhost

# RADIUS server to use for accouting requests. All that I
# said for authserver applies, too. 
#
#acctserver 	10.4.0.2
acctserver 	localhost

# file holding shared secrets used for the communication
# between the RADIUS client and server
servers		/usr/local/etc/radiusclient/servers

# dictionary of allowed attributes and values
# just like in the normal RADIUS distributions
dictionary 	/usr/local/etc/radiusclient/dictionary

# program to call for a RADIUS authenticated login
login_radius	/usr/local/sbin/login.radius

# file which holds sequence number for communication with the
# RADIUS server
seqfile		/var/run/radius.seq

# file which specifies mapping between ttyname and NAS-Port attribute
mapfile		/usr/local/etc/radiusclient/port-id-map

# default authentication realm to append to all usernames if no
# realm was explicitly specified by the user
# the radiusd directly form Livingston doesnt use any realms, so leave
# it blank then
default_realm

# time to wait for a reply from the RADIUS server
radius_timeout	10

# resend request this many times before trying the next server
radius_retries	3

# The length of time in seconds that we skip a nonresponsive RADIUS
# server for transaction requests.  Server(s) being in the "dead" state
# are tried only after all other non-dead servers have been tried and
# failed or timeouted.  The deadtime interval starts when the server
# does not respond to an authentication/accounting request transmissions. 
# When the interval expires, the "dead" server would be re-tried again,
# and if it's still down then it will be considered "dead" for another
# such interval and so on. This option is no-op if there is only one
# server in the list. Set to 0 in order to disable the feature.
radius_deadtime	0

# local address from which radius packets have to be sent
bindaddr *

# LOCAL settings

# program to execute for local login
# it must support the -f flag for preauthenticated login
login_local	/bin/login
EOF

cat >> /usr/local/etc/radiusclient/dictionary << EOF

#
#	DHCP Proxy/Relay attributes
#
ATTRIBUTE	DHCP-Agent-Circuit-Id	82.1	integer
ATTRIBUTE	DHCP-Agent-Remote-Id	82.2	string
ATTRIBUTE	DHCP-Relay-Circuit-Id	82.1	integer
ATTRIBUTE	DHCP-Relay-Remote-Id	82.2	string

EOF

cat >> /usr/local/etc/radiusclient/servers << EOF
10.4.0.2					testing123
localhost/localhost				testing123

EOF

# Download DHCP config files
cd /opt
wget $REPO_URL_BLOB/org.onap.demo/vnfs/vcpe/$INSTALL_SCRIPT_VERSION/v_bng_init.sh
wget $REPO_URL_BLOB/org.onap.demo/vnfs/vcpe/$INSTALL_SCRIPT_VERSION/v_bng.sh
chmod +x v_bng_init.sh
chmod +x v_bng.sh
mv v_bng.sh /etc/init.d
update-rc.d v_bng.sh defaults

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

./v_bng_init.sh
