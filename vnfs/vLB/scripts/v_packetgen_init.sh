#!/bin/bash

# Start VPP
systemctl start vpp
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

IPADDR1_MASK=$(ifconfig eth1 | grep "Mask" | awk '{print $4}' | awk -F ":" '{print $2}')
IPADDR1_CIDR=$(mask2cidr $IPADDR1_MASK)

# Configure VPP for vPacketGenerator
IPADDR1=$(ifconfig eth1 | grep "inet addr" | tr -s ' ' | cut -d' ' -f3 | cut -d':' -f2)
HWADDR1=$(ifconfig eth1 | grep HWaddr | tr -s ' ' | cut -d' ' -f5)
FAKE_HWADDR1=$(echo -n 00; dd bs=1 count=5 if=/dev/urandom 2>/dev/null | hexdump -v -e '/1 ":%02X"')
VLB_IPADDR=$(cat /opt/config/vlb_ipaddr.txt)
VLB_MAC=$(cat /opt/config/vlb_mac.txt)
GW=$(route -n | grep "^0.0.0.0" | awk '{print $2}')

ifconfig eth1 down
ifconfig eth1 hw ether $FAKE_HWADDR1
ip addr flush dev eth1
ifconfig eth1 up
vppctl tap connect tap111 hwaddr $HWADDR1
vppctl set int ip address tap-0 $IPADDR1"/"$IPADDR1_CIDR
vppctl set int state tap-0 up
brctl addbr br0
brctl addif br0 tap111
brctl addif br0 eth1
ifconfig br0 up
vppctl ip route add 0.0.0.0/0 via $GW
sleep 1

# Set br0 with public IP and valid MAC so that Linux will have public network access
ifconfig br0 hw ether $HWADDR1
ifconfig br0 $IPADDR1 netmask $IPADDR1_MASK
route add default gw $GW
sleep 1
vppctl set ip arp tap-0 $VLB_IPADDR $VLB_MAC

# Install packet streams
sed -i -e "0,/UDP/ s/UDP:.*/UDP: "$IPADDR1" -> "$VLB_IPADDR"/" /opt/dns_streams/stream_dns1
sed -i -e "0,/UDP/ s/UDP:.*/UDP: "$IPADDR1" -> "$VLB_IPADDR"/" /opt/dns_streams/stream_dns2
sed -i -e "0,/UDP/ s/UDP:.*/UDP: "$IPADDR1" -> "$VLB_IPADDR"/" /opt/dns_streams/stream_dns3
sed -i -e "0,/UDP/ s/UDP:.*/UDP: "$IPADDR1" -> "$VLB_IPADDR"/" /opt/dns_streams/stream_dns4
sed -i -e "0,/UDP/ s/UDP:.*/UDP: "$IPADDR1" -> "$VLB_IPADDR"/" /opt/dns_streams/stream_dns5
sed -i -e "0,/UDP/ s/UDP:.*/UDP: "$IPADDR1" -> "$VLB_IPADDR"/" /opt/dns_streams/stream_dns6
sed -i -e "0,/UDP/ s/UDP:.*/UDP: "$IPADDR1" -> "$VLB_IPADDR"/" /opt/dns_streams/stream_dns7
sed -i -e "0,/UDP/ s/UDP:.*/UDP: "$IPADDR1" -> "$VLB_IPADDR"/" /opt/dns_streams/stream_dns8
sed -i -e "0,/UDP/ s/UDP:.*/UDP: "$IPADDR1" -> "$VLB_IPADDR"/" /opt/dns_streams/stream_dns9
sed -i -e "0,/UDP/ s/UDP:.*/UDP: "$IPADDR1" -> "$VLB_IPADDR"/" /opt/dns_streams/stream_dns10

sed -i -e "s/.*-> 53.*/    UDP: $RANDOM -> 53/" /opt/dns_streams/stream_dns1
sed -i -e "s/.*-> 53.*/    UDP: $RANDOM -> 53/" /opt/dns_streams/stream_dns2
sed -i -e "s/.*-> 53.*/    UDP: $RANDOM -> 53/" /opt/dns_streams/stream_dns3
sed -i -e "s/.*-> 53.*/    UDP: $RANDOM -> 53/" /opt/dns_streams/stream_dns4
sed -i -e "s/.*-> 53.*/    UDP: $RANDOM -> 53/" /opt/dns_streams/stream_dns5
sed -i -e "s/.*-> 53.*/    UDP: $RANDOM -> 53/" /opt/dns_streams/stream_dns6
sed -i -e "s/.*-> 53.*/    UDP: $RANDOM -> 53/" /opt/dns_streams/stream_dns7
sed -i -e "s/.*-> 53.*/    UDP: $RANDOM -> 53/" /opt/dns_streams/stream_dns8
sed -i -e "s/.*-> 53.*/    UDP: $RANDOM -> 53/" /opt/dns_streams/stream_dns9
sed -i -e "s/.*-> 53.*/    UDP: $RANDOM -> 53/" /opt/dns_streams/stream_dns10

vppctl exec /opt/dns_streams/stream_dns1
vppctl exec /opt/dns_streams/stream_dns2
vppctl exec /opt/dns_streams/stream_dns3
vppctl exec /opt/dns_streams/stream_dns4
vppctl exec /opt/dns_streams/stream_dns5
vppctl exec /opt/dns_streams/stream_dns6
vppctl exec /opt/dns_streams/stream_dns7
vppctl exec /opt/dns_streams/stream_dns8
vppctl exec /opt/dns_streams/stream_dns9
vppctl exec /opt/dns_streams/stream_dns10

vppctl set int ip address pg0 $(cat /opt/config/pg_int.txt)"/"$IPADDR1_CIDR
sleep 1

# Enable traffic flows
cd /opt
chmod +x run_streams_dns.sh
./run_streams_dns.sh &>/dev/null &disown