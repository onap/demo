#!/bin/bash

if [ ! "$#" -eq 1 ]
then
  echo "Usage: ./add_dns.sh [remote DNS server]"
  exit
fi

DNS_IPADDR=$1
IP_TO_PKTGEN_NET=$(cat /opt/config/ip_to_pktgen_net.txt)
IP_TO_DNS_NET=$(cat /opt/config/ip_to_dns_net.txt)
GRE_IPADDR=$(cat /opt/config/gre_ipaddr.txt)

vppctl lb as $IP_TO_PKTGEN_NET"/32" $DNS_IPADDR
GRE=$(vppctl create gre tunnel src $IP_TO_DNS_NET dst $DNS_IPADDR)
vppctl set int ip address $GRE $GRE_IPADDR"/32"
vppctl set int state $GRE up

# Update the number of vDNSs currently active
FD="/opt/VES/evel/evel-library/code/VESreporting/active_dns.txt"
CURR_DNS=$(cat $FD)
let CURR_DNS=$CURR_DNS+1
echo $CURR_DNS > $FD