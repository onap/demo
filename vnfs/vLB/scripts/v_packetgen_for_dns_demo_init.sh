#!/bin/bash

# Start VPP
start vpp
sleep 1

# Configure VPP for vPacketGenerator
IPADDR1=$(ifconfig eth0 | grep "inet addr" | tr -s ' ' | cut -d' ' -f3 | cut -d':' -f2)
HWADDR1=$(ifconfig eth0 | grep HWaddr | tr -s ' ' | cut -d' ' -f5)
FAKE_HWADDR1=$(echo -n 00; dd bs=1 count=5 if=/dev/urandom 2>/dev/null | hexdump -v -e '/1 ":%02X"')
VLB_IPADDR=$(cat /opt/config/vlb_ipaddr.txt)
GW=$(route | grep default | awk '{print $2}')

ifconfig eth0 down
ifconfig eth0 hw ether $FAKE_HWADDR1
ip addr flush dev eth0
ifconfig eth0 up
vppctl tap connect tap111 hwaddr $HWADDR1
vppctl set int ip address tap-0 $IPADDR1"/24"
vppctl set int state tap-0 up
brctl addbr br0
brctl addif br0 tap111
brctl addif br0 eth0
ifconfig br0 up
vppctl ip route add 0.0.0.0/0 via $GW
sleep 1


#Let's set br0 with public IP and valid MAC so that Linux will have public network access
ifconfig br0 hw ether $HWADDR1
ifconfig br0 $IPADDR1 netmask 255.255.255.0
route add default gw $GW

#Adding static arp entry for VPP so that it will be able to send packets to default GW
ping -c 1 $VLB_IPADDR &>/dev/null &disown
sleep 3

GW_MAC=$(arp -n | grep -w $GW | tr -s ' ' | cut -d' ' -f3)
VLB_MAC=$(arp -n | grep -w $VLB_IPADDR | tr -s ' ' | cut -d' ' -f3)

#if VLB in our network, we will use its mac
if [ ! -z "$VLB_MAC" ]; then
vppctl set ip arp tap-0 $VLB_IPADDR $VLB_MAC
fi

# in any case let's add arp entry for default gw
vppctl set ip arp tap-0 $GW $GW_MAC


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

#sed -i -e "s/.*-> 53.*/    UDP: $RANDOM -> 53/" /opt/dns_streams/stream_dns*

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
sleep 1

# Start HoneyComb
VERSION=$(cat /opt/config/demo_artifacts_version.txt)
echo "" > /var/lib/honeycomb/persist/context/data.json
echo "" > /var/lib/honeycomb/persist/config/data.json
/opt/honeycomb/sample-distribution-$VERSION/honeycomb &>/dev/null &disown
sleep 20

# Enable traffic flows
cd /opt
chmod +x run_streams_dns.sh
./run_streams_dns.sh &>/dev/null &disown

