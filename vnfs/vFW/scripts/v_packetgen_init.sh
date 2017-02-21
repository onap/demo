#!/bin/bash

# Start VPP
start vpp
sleep 1

# Configure VPP for vPacketGenerator
IPADDR1=$(ifconfig eth1 | grep "inet addr" | tr -s ' ' | cut -d' ' -f3 | cut -d':' -f2)
HWADDR1=$(ifconfig eth1 | grep HWaddr | tr -s ' ' | cut -d' ' -f5)
FAKE_HWADDR1=$(echo -n 00; dd bs=1 count=5 if=/dev/urandom 2>/dev/null | hexdump -v -e '/1 ":%02X"')
PROTECTED_NET_CIDR=$(cat /opt/config/protected_net_cidr.txt)
FW_IPADDR=$(cat /opt/config/fw_ipaddr.txt)
SINK_IPADDR=$(cat /opt/config/sink_ipaddr.txt)

ifconfig eth1 down
ifconfig eth1 hw ether $FAKE_HWADDR1
ip addr flush dev eth1
ifconfig eth1 up
vppctl tap connect tap111 hwaddr $HWADDR1
vppctl set int ip address tap-0 $IPADDR1"/24"
vppctl set int state tap-0 up
brctl addbr br0
brctl addif br0 tap111
brctl addif br0 eth1
ifconfig br0 up
vppctl ip route add $PROTECTED_NET_CIDR via $FW_IPADDR
sleep 1

# Install packet streams
sed -i -e "0,/UDP/ s/UDP:.*/UDP: "$IPADDR1" -> "$SINK_IPADDR"/" /opt/pg_streams/stream_fw_udp1
sed -i -e "0,/UDP/ s/UDP:.*/UDP: "$IPADDR1" -> "$SINK_IPADDR"/" /opt/pg_streams/stream_fw_udp2
sed -i -e "0,/UDP/ s/UDP:.*/UDP: "$IPADDR1" -> "$SINK_IPADDR"/" /opt/pg_streams/stream_fw_udp3
sed -i -e "0,/UDP/ s/UDP:.*/UDP: "$IPADDR1" -> "$SINK_IPADDR"/" /opt/pg_streams/stream_fw_udp4
sed -i -e "0,/UDP/ s/UDP:.*/UDP: "$IPADDR1" -> "$SINK_IPADDR"/" /opt/pg_streams/stream_fw_udp5
sed -i -e "0,/UDP/ s/UDP:.*/UDP: "$IPADDR1" -> "$SINK_IPADDR"/" /opt/pg_streams/stream_fw_udp6
sed -i -e "0,/UDP/ s/UDP:.*/UDP: "$IPADDR1" -> "$SINK_IPADDR"/" /opt/pg_streams/stream_fw_udp7
sed -i -e "0,/UDP/ s/UDP:.*/UDP: "$IPADDR1" -> "$SINK_IPADDR"/" /opt/pg_streams/stream_fw_udp8
sed -i -e "0,/UDP/ s/UDP:.*/UDP: "$IPADDR1" -> "$SINK_IPADDR"/" /opt/pg_streams/stream_fw_udp9
sed -i -e "0,/UDP/ s/UDP:.*/UDP: "$IPADDR1" -> "$SINK_IPADDR"/" /opt/pg_streams/stream_fw_udp10
vppctl exec /opt/pg_streams/stream_fw_udp1
vppctl exec /opt/pg_streams/stream_fw_udp2
vppctl exec /opt/pg_streams/stream_fw_udp3
vppctl exec /opt/pg_streams/stream_fw_udp4
vppctl exec /opt/pg_streams/stream_fw_udp5
vppctl exec /opt/pg_streams/stream_fw_udp6
vppctl exec /opt/pg_streams/stream_fw_udp7
vppctl exec /opt/pg_streams/stream_fw_udp8
vppctl exec /opt/pg_streams/stream_fw_udp9
vppctl exec /opt/pg_streams/stream_fw_udp10
sleep 1

# Start HoneyComb
VERSION=$(cat /opt/config/artifacts_version.txt)
echo "" > /var/lib/honeycomb/persist/context/data.json
echo "" > /var/lib/honeycomb/persist/config/data.json
/opt/honeycomb/sample-distribution-$VERSION/honeycomb &>/dev/null &disown
sleep 20

# Enable traffic flows
cd /opt
chmod +x run_traffic_fw_demo.sh
./run_traffic_fw_demo.sh &>/dev/null &disown
