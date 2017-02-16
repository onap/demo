#!/bin/bash

if [ ! "$#" -eq 1 ]
then
  echo "Usage: ./add_dns.sh [remote DNS server]"
  exit
fi

DNS_IPADDR=$1
MY_PUBLIC_IP=$(cat /opt/config/local_public_ipaddr.txt)
MY_PRIVATE_IP=$(cat /opt/config/local_private_ipaddr.txt)

vppctl lb as $MY_PUBLIC_ID"/32" $DNS_IPADDR del
vppctl create gre tunnel src $MY_PRIVATE_IP dst $DNS_IPADDR del

# Update the number of vDNSs currently active
FD="/opt/VES/code/evel_training/VESreporting/active_dns.txt"
CURR_DNS=$(cat $FD)
let CURR_DNS=$CURR_DNS-1
if [[ $CURR_DNS -lt 0 ]]
then 
  CURR_DNS=0
fi
echo $CURR_DNS > $FD
