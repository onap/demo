#!/bin/bash

LB_PUBLIC_IP=$(cat /opt/config/lb_public_ipaddr.txt)
LB_PRIVATE_IP=$(cat /opt/config/lb_private_ipaddr.txt)
MY_PRIVATE_IP=$(cat /opt/config/local_private_ipaddr.txt)

sed -i "s/x.x.x.x/"$LB_PUBLIC_IP"/g" /etc/bind/named.conf.options

ip tunnel add gre123 mode gre remote $LB_PRIVATE_IP local $MY_PRIVATE_IP ttl 255
ip link set gre123 up
ip addr add $LB_PUBLIC_IP"/32" dev gre123
route add default dev gre123
ifconfig eth0 down

service bind9 restart
