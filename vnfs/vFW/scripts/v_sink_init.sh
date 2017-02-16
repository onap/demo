#!/bin/bash

# Set the IP address of the protected network interface of the vFirewall as a default gateway to the unprotected network
PROTECTED_NET_GW=$(cat /opt/config/protected_net_gw.txt)
UNPROTECTED_NET=$(cat /opt/config/unprotected_net.txt)

route add -net $UNPROTECTED_NET netmask 255.255.255.0 gw $PROTECTED_NET_GW
