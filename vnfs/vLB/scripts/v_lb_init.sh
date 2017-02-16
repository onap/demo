#!/bin/bash

# Start VPP
start vpp
sleep 1

# Configure VPP for vPacketGenerator
IPADDR1=$(ifconfig eth0 | grep "inet addr" | tr -s ' ' | cut -d' ' -f3 | cut -d':' -f2)
IPADDR2=$(ifconfig eth1 | grep "inet addr" | tr -s ' ' | cut -d' ' -f3 | cut -d':' -f2)
HWADDR1=$(ifconfig eth0 | grep HWaddr | tr -s ' ' | cut -d' ' -f5)
HWADDR2=$(ifconfig eth1 | grep HWaddr | tr -s ' ' | cut -d' ' -f5)
FAKE_HWADDR1=$(echo -n 00; dd bs=1 count=5 if=/dev/urandom 2>/dev/null |hexdump -v -e '/1 ":%02X"')
FAKE_HWADDR2=$(echo -n 00; dd bs=1 count=5 if=/dev/urandom 2>/dev/null |hexdump -v -e '/1 ":%02X"')
GW=$(route | grep default | awk '{print $2}')

ifconfig eth0 down
ifconfig eth0 hw ether $FAKE_HWADDR1
ip addr flush dev eth0
ifconfig eth0 up
vppctl tap connect tappub hwaddr $HWADDR1
vppctl set int ip address tap-0 $IPADDR1"/24"
vppctl set int state tap-0 up
brctl addbr br0
brctl addif br0 tappub
brctl addif br0 eth0
ifconfig br0 up

ifconfig eth1 down
ifconfig eth1 hw ether $FAKE_HWADDR2
ip addr flush dev eth1
ifconfig eth1 up
vppctl tap connect tap111 hwaddr $HWADDR2
vppctl set int ip address tap-1 $IPADDR2"/24"
vppctl set int state tap-1 up
brctl addbr br1
brctl addif br1 tap111
brctl addif br1 eth1
ifconfig br1 up
sleep 1

vppctl lb conf ip4-src-address $IPADDR2
vppctl lb vip $IPADDR1"/32" encap gre4
vppctl ip route add 0.0.0.0/0 via $GW
sleep 1

cd /opt/FDserver
./dnsmembership.sh &>/dev/null &disown

# Start VES client
cd /opt/VES/code/evel_training/VESreporting/
echo 0 > active_dns.txt
./go-client.sh &>/dev/null &disown
