#!/bin/bash

if [ ! "$#" -eq 1 ]
then
  echo "Usage: ./remove_dns.sh [remote DNS server]"
  exit
fi

DNS_IPADDR=$1
IP_TO_PKTGEN_NET=$(cat /opt/config/ip_to_pktgen_net.txt)
IP_TO_DNS_NET=$(cat /opt/config/ip_to_dns_net.txt)

vppctl lb as $IP_TO_PKTGEN_NET"/32" $DNS_IPADDR del
vppctl create gre tunnel src $IP_TO_DNS_NET dst $DNS_IPADDR del

# Update the number of vDNSs currently active
FD="/opt/VES/evel/evel-library/code/VESreporting/active_dns.txt"
CURR_DNS=$(cat $FD)
let CURR_DNS=$CURR_DNS-1
if [[ $CURR_DNS -lt 0 ]]
then
  CURR_DNS=0
fi
echo $CURR_DNS > $FD