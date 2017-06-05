#!/bin/bash

# Start VPP
start vpp
sleep 1

# Compute the network CIDR from the Netmask
mask2cidr() {
    nbits=0
    IFS=.
    for dec in $1 ; do
        case $dec in
            255) let nbits+=8;;
            254) let nbits+=7;;
            252) let nbits+=6;;
            248) let nbits+=5;;
            240) let nbits+=4;;
            224) let nbits+=3;;
            192) let nbits+=2;;
            128) let nbits+=1;;
            0);;
            *) echo "Error: $dec is not recognized"; exit 1
        esac
    done
    echo "$nbits"
}

IPADDR1_MASK=$(ifconfig eth0 | grep "Mask" | awk '{print $4}' | awk -F ":" '{print $2}')
IPADDR1_CIDR=$(mask2cidr $IPADDR1_MASK)
IPADDR2_MASK=$(ifconfig eth1 | grep "Mask" | awk '{print $4}' | awk -F ":" '{print $2}')
IPADDR2_CIDR=$(mask2cidr $IPADDR2_MASK)

# Configure VPP for vPacketGenerator
IPADDR1=$(ifconfig eth0 | grep "inet addr" | tr -s ' ' | cut -d' ' -f3 | cut -d':' -f2)
IPADDR2=$(ifconfig eth1 | grep "inet addr" | tr -s ' ' | cut -d' ' -f3 | cut -d':' -f2)
HWADDR1=$(ifconfig eth0 | grep HWaddr | tr -s ' ' | cut -d' ' -f5)
HWADDR2=$(ifconfig eth1 | grep HWaddr | tr -s ' ' | cut -d' ' -f5)
FAKE_HWADDR1=$(echo -n 00; dd bs=1 count=5 if=/dev/urandom 2>/dev/null |hexdump -v -e '/1 ":%02X"')
FAKE_HWADDR2=$(echo -n 00; dd bs=1 count=5 if=/dev/urandom 2>/dev/null |hexdump -v -e '/1 ":%02X"')
GW=$(route -n | grep "^0.0.0.0" | awk '{print $2}')

ifconfig eth0 down
ifconfig eth0 hw ether $FAKE_HWADDR1
ip addr flush dev eth0
ifconfig eth0 up
vppctl tap connect tappub hwaddr $HWADDR1
vppctl set int ip address tap-0 $IPADDR1"/"$IPADDR1_CIDR
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
vppctl set int ip address tap-1 $IPADDR2"/"$IPADDR2_CIDR
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
