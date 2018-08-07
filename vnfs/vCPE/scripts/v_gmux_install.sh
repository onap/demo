#!/bin/bash

SCRIPT_VERSION=$(cat /opt/config/script_version.txt)
CLOUD_ENV=$(cat /opt/config/cloud_env.txt)
MUX_GW_IP=$(cat /opt/config/mux_gw_net_ipaddr.txt)
MUX_GW_CIDR=$(cat /opt/config/mux_gw_net_cidr.txt)
MUX_TO_BNG_IP=$(cat /opt/config/mux_to_bng_net_ipaddr.txt)
BNG_MUX_CIDR=$(cat /opt/config/bng_mux_net_cidr.txt)
BRG_BNG_CIDR=$(cat /opt/config/brg_bng_net_cidr.txt)
BNG_TO_MUX_IP=$(cat /opt/config/bng_to_mux_net_ipaddr.txt)

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

BNG_MUX_NIC=GigabitEthernet`echo ${NICS} | cut -d " " -f 2`  # second interface in list
MUX_GW_NIC=GigabitEthernet`echo ${NICS} | cut -d " " -f 4`   # fourth interface in list

cat > /etc/vpp/setup.gate << EOF
set int state ${BNG_MUX_NIC} up
set int ip address ${BNG_MUX_NIC} ${MUX_TO_BNG_IP}/${BNG_MUX_CIDR#*/}

set int state ${MUX_GW_NIC} up
set int ip address ${MUX_GW_NIC} ${MUX_GW_IP}/${MUX_GW_CIDR#*/}
ip route add ${BRG_BNG_CIDR} via ${BNG_TO_MUX_IP} ${BNG_MUX_NIC}
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
ExecStop=/opt/reset_ves_mode.sh
Restart=always
RestartSec=10

[Install]
WantedBy=multi-user.target
EOF
systemctl enable /etc/systemd/system/honeycomb.service

# Create script that is used to reset the ves mode configuration
# when the honeycomb service is stopped
cat > /opt/reset_ves_mode.sh << EOF
#!/bin/bash
mode_data=\$(curl -X GET -u admin:admin http://127.0.0.1:8183/restconf/config/vesagent:vesagent/mode 2>/dev/null | sed s/\"base-packet-loss\":[0-9]\*,/\"base-packet-loss\":0,/)
if [[ "\${mode_data}" != "" ]]
then
    curl -X DELETE -u admin:admin http://127.0.0.1:8183/restconf/config/vesagent:vesagent/mode
    curl -H "Content-Type:application/json" --data "\${mode_data}" -X POST -u admin:admin http://127.0.0.1:8183/restconf/config/vesagent:vesagent
fi
EOF
    chmod a+x /opt/reset_ves_mode.sh

    #Create a systemd service for auto-save
    cat > /usr/bin/save_config << EOF
#!/bin/bash

#########################################################################
#
#  Copyright (c) 2017 Intel and/or its affiliates.
# 
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at:
# 
#      http://www.apache.org/licenses/LICENSE-2.0
# 
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#
##########################################################################

############################### Variables ################################
VPP_SETUP_GATE=/etc/vpp/setup.gate

############################### Functions ################################

# Write the commands to the startup scripts.
#
# We could set VPP configuration to the startup.conf.
# Write the configuration to the startup scripts so we could
# restore the system after rebooting.
#
write_startup_scripts()
{
	local cmd=\${2}
	local is_add=\${1}

	if [[ \${is_add} == add ]] ;then
		while read -r line
		do
			if [[ \${line} == \${cmd} ]] ;then
				return 0
			fi
		done < \${VPP_SETUP_GATE}

		echo "\${cmd}" >> \${VPP_SETUP_GATE}
	else
		while read -r line
		do
			if [[ \${line} == \${cmd} ]] ;then
				sed -i "/\${line}/d" \${VPP_SETUP_GATE}
				return 0
			fi
		done < \${VPP_SETUP_GATE}
	fi
}

# Save the VxLAN Tunnel Configuration to the startup script.
#
# Get the current VxLAN tunnel configuration with bash command:
# \$vppctl show vxlan tunnel
#  [0] src 10.3.0.2 dst 10.1.0.20 vni 100 sw_if_index 1 encap_fib_index 0 fib_entry_index 7 decap_next l2
#  [1] src 10.5.0.20 dst 10.5.0.21 vni 100 sw_if_index 2 encap_fib_index 0 fib_entry_index 8 decap_next l2
# Set the VxLAN Tunnel with the bash command:
# \$vppctl create vxlan tunnel src 10.3.0.2 dst 10.1.0.20 vni 100
# vxlan_tunnel0
save_vxlan_tunnel()
{
	local src=""
	local dst=""
	local vni=""

	vppctl show vxlan tunnel | while read line
	do
		if [ "\${line}" != "" ] ;then
			src=\`echo \${line} | awk '{ print \$3 }'\`
			dst=\`echo \${line} | awk '{ print \$5 }'\`
			vni=\`echo \${line} | awk '{ print \$7 }'\`

			write_startup_scripts add "create vxlan tunnel src \${src} dst \${dst} vni \${vni}"
		fi
	done
}

# Save the VxLAN tunnel L2 xconnect configuration to the startup script.
#
# Get the Current L2 Address configuration with bash command:
# \$vppctl show int addr
# local0 (dn):
# vxlan_tunnel0 (up):
#   l2 xconnect vxlan_tunnel1
# vxlan_tunnel1 (up):
#   l2 xconnect vxlan_tunnel0
# Save the VxLAN tunnel L2 xconnect configuration with bash command:
# \$vppctl set interface l2 xconnect vxlan_tunnel0 vxlan_tunnel1
#
save_vxlan_xconnect()
{
	local ingress=""
	local egress=""

	vppctl show int addr | while read line
	do
		if [[ \${line} == vxlan_tunnel* ]] ;then
			read next
			while [[ \${next} != l2* ]] || [[ \${next} == "" ]]
			do
				line=\`echo \${next}\`
				read next
			done
			if [[ \${next} == l2* ]] ;then
				ingress=\`echo \${line} | awk '{ print \$1 }'\`
				egress=\`echo \${next} | awk '{ print \$3 }'\`
				write_startup_scripts add "set interface l2 xconnect \${ingress} \${egress}"
			fi
		fi
	done
}

################################# MAIN ###################################

save_vxlan_tunnel

save_vxlan_xconnect

EOF
    chmod a+x /usr/bin/save_config
    cat > /etc/systemd/system/autosave.service << EOF
[Unit]
Description=Run Scripts at Start and Stop
equires=vpp.service
After=vpp.service

[Service]
Type=oneshot
RemainAfterExit=true
ExecStop=/usr/bin/save_config

[Install]
WantedBy=multi-user.target
EOF
systemctl enable /etc/systemd/system/autosave.service

# Download DHCP config files
cd /opt
unzip -p -j /opt/vcpe-scripts-$SCRIPT_VERSION.zip v_gmux_init.sh > /opt/v_gmux_init.sh
unzip -p -j /opt/vcpe-scripts-$SCRIPT_VERSION.zip v_gmux.sh > /opt/v_gmux.sh
chmod +x v_gmux_init.sh
chmod +x v_gmux.sh
mv v_gmux.sh /etc/init.d
update-rc.d v_gmux.sh defaults

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

./v_gmux_init.sh

