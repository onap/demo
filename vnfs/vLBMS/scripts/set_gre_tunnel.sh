#!/bin/bash

LB_TO_PKTGEN_IF=$(cat /opt/config/lb_to_pktgen_if.txt)
LB_PRIVATE_IP=$(cat /opt/config/lb_private_ipaddr.txt)
MY_PRIVATE_IP=$(cat /opt/config/local_private_ipaddr.txt)

sed -i "s/x.x.x.x/"$LB_TO_PKTGEN_IF"/g" /etc/bind/named.conf.options

ip tunnel add gre123 mode gre remote $LB_PRIVATE_IP local $MY_PRIVATE_IP ttl 255
ip link set gre123 up
ip addr add $LB_TO_PKTGEN_IF"/32" dev gre123
route add default dev gre123
ifconfig eth0 down

service bind9 restart
