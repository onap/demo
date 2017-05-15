#!/bin/bash

if [ ! "$#" -eq 1 ]
then
  echo "Usage: ./set_gre_tunnel.sh [LB public IP address]"
  exit
fi

LB_PUBLIC_IP=$1
LB_PRIVATE_IP=$(cat /opt/config/lb_private_ipaddr.txt)
MY_PRIVATE_IP=$(cat /opt/config/local_private_ipaddr.txt)
OLD_GW=$(route -n | grep "^0.0.0.0" | awk '{print $2}')

sed -i "s/x.x.x.x/"$LB_PUBLIC_IP"/g" /etc/bind/named.conf.options

ip tunnel add gre123 mode gre remote $LB_PRIVATE_IP local $MY_PRIVATE_IP ttl 255
ip link set gre123 up
ip addr add $LB_PUBLIC_IP"/32" dev gre123
route del default gw $OLD_GW
route add default dev gre123

service bind9 restart
