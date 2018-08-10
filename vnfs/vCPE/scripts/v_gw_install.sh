#!/bin/bash

CLOUD_ENV=$(cat /opt/config/cloud_env.txt)
MUX_GW_IP=$(cat /opt/config/mux_gw_private_net_ipaddr.txt)
MUX_GW_CIDR=$(cat /opt/config/mux_gw_private_net_cidr.txt)
MUX_IP_ADDR=$(cat /opt/config/mux_ip_addr.txt)
VG_VGMUX_TUNNEL_VNI=$(cat /opt/config/vg_vgmux_tunnel_vni.txt)



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

# DHCP server config
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
    #sed -i "s/ens[0-9]*/eth0/g" /etc/udev/rules.d/70-persistent-net.rules
    echo 'network: {config: disabled}' >> /etc/cloud/cloud.cfg.d/99-disable-network-config.cfg
    echo 'Execution of vG install script completed' > /opt/script_status.txt
    reboot
fi
echo 'Execution of vG install script completed' > /opt/script_status.txt
