#!/bin/bash

# Convert Network CIDR to Netmask
cdr2mask () {
	# Number of args to shift, 255..255, first non-255 byte, zeroes
	set -- $(( 5 - ($1 / 8) )) 255 255 255 255 $(( (255 << (8 - ($1 % 8))) & 255 )) 0 0 0
	[ $1 -gt 1 ] && shift $1 || shift
	echo ${1-0}.${2-0}.${3-0}.${4-0}
}

# Set the IP address of the protected network interface of the vIPsec as a default gateway to the unprotected network
PROTECTED_NET_GW=$(cat /opt/config/protected_net_gw.txt)
PROTECTED_NET_A=$(cat /opt/config/protected_net_A.txt | cut -d'/' -f1)
BITS=$(cat /opt/config/protected_net_A.txt | cut -d"/" -f2)
NETMASK=$(cdr2mask $BITS) 

route add -net $PROTECTED_NET_A netmask $NETMASK gw $PROTECTED_NET_GW
