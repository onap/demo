#!/bin/bash

if [ ! "$#" -eq 1 ]
then
  echo "Usage: ./add_dns.sh [remote DNS server]"
  exit
fi

DNS_IPADDR=$1
MY_PUBLIC_IP=$(cat /opt/config/local_public_ipaddr.txt)
MY_PRIVATE_IP=$(cat /opt/config/local_private_ipaddr.txt)

vppctl lb as $MY_PUBLIC_IP"/32" $DNS_IPADDR
GRE=$(vppctl create gre tunnel src $MY_PRIVATE_IP dst $DNS_IPADDR)
vppctl set int state $GRE up

# Update the number of vDNSs currently active
FD="/opt/VES/code/evel_training/VESreporting/active_dns.txt"
CURR_DNS=$(cat $FD)
let CURR_DNS=$CURR_DNS+1
echo $CURR_DNS > $FD
