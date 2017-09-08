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
	local cmd=${2}
	local is_add=${1}

	if [[ ${is_add} == add ]] ;then
		while read -r line
		do
			if [[ ${line} == ${cmd} ]] ;then
				return 0
			fi
		done < ${VPP_SETUP_GATE}

		echo "${cmd}" >> ${VPP_SETUP_GATE}
	else
		while read -r line
		do
			if [[ ${line} == ${cmd} ]] ;then
				sed -i "/${line}/d" ${VPP_SETUP_GATE}
				return 0
			fi
		done < ${VPP_SETUP_GATE}
	fi
}

# Saves the VES agent configuration to the startup script.
#
# Get the current VES agent configuration from the bash command:
# $vppctl show ves agent
#    Server Addr    Server Port Interval Enabled
#    127.0.0.1        8080         10    True
# Set the VES agent configuration with the bash command:
# $vppctl set ves agent server 127.0.0.1 port 8080 intval 10
#
save_ves_config()
{
	local server=""
	local port=""
	local intval=""

	local ves_config=`vppctl show ves agent | head -2 | tail -1`
	if [ "${ves_config}" != "" ] ;then
		server=`echo ${ves_config} | awk '{ print $1 }'`
		port=`echo ${ves_config} | awk '{ print $2 }'`
		intval=`echo ${ves_config} | awk '{ print $3 }'`
		write_startup_scripts add "set ves agent server ${server} port ${port} intval ${intval}"
	fi
}

# Save the VxLAN Tunnel Configuration to the startup script.
#
# Get the current VxLAN tunnel configuration with bash command:
# $vppctl show vxlan tunnel
#  [0] src 10.3.0.2 dst 10.1.0.20 vni 100 sw_if_index 1 encap_fib_index 0 fib_entry_index 7 decap_next l2
#  [1] src 10.5.0.20 dst 10.5.0.21 vni 100 sw_if_index 2 encap_fib_index 0 fib_entry_index 8 decap_next l2
# Set the VxLAN Tunnel with the bash command:
# $vppctl create vxlan tunnel src 10.3.0.2 dst 10.1.0.20 vni 100
# vxlan_tunnel0
save_vxlan_tunnel()
{
	local src=""
	local dst=""
	local vni=""

	vppctl show vxlan tunnel | while read line
	do
		if [ "${line}" != "" ] ;then
			src=`echo ${line} | awk '{ print $3 }'`
			dst=`echo ${line} | awk '{ print $5 }'`
			vni=`echo ${line} | awk '{ print $7 }'`

			write_startup_scripts add "create vxlan tunnel src ${src} dst ${dst} vni ${vni}"
		fi
	done
}

# Save the VxLAN tunnel L2 xconnect configuration to the startup script.
#
# Get the Current L2 Address configuration with bash command:
# $vppctl show int addr
# local0 (dn):
# vxlan_tunnel0 (up):
#   l2 xconnect vxlan_tunnel1
# vxlan_tunnel1 (up):
#   l2 xconnect vxlan_tunnel0
# Save the VxLAN tunnel L2 xconnect configuration with bash command:
# $vppctl set interface l2 xconnect vxlan_tunnel0 vxlan_tunnel1
#
save_vxlan_xconnect()
{
	local ingress=""
	local egress=""

	vppctl show int addr | while read line
	do
		if [[ ${line} == vxlan_tunnel* ]] ;then
			read next
			while [[ ${next} != l2* ]] || [[ ${next} == "" ]]
			do
				line=`echo ${next}`
				read next
			done
			if [[ ${next} == l2* ]] ;then
				ingress=`echo ${line} | awk '{ print $1 }'`
				egress=`echo ${next} | awk '{ print $3 }'`
				write_startup_scripts add "set interface l2 xconnect ${ingress} ${egress}"
			fi
		fi
	done
}

################################# MAIN ###################################

save_ves_config

save_vxlan_tunnel

save_vxlan_xconnect
