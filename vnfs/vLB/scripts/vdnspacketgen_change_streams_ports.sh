#!/bin/bash

#Disable all streams via Honeycomb (so that it will in consistent state)
curl -X PUT -H "Authorization: Basic YWRtaW46YWRtaW4=" -H "Content-Type: application/json" -H "Cache-Control: no-cache" -d '{"pg-streams":{"pg-stream": []}}' "http://localhost:8183/restconf/config/sample-plugin:sample-plugin/pg-streams"

vppctl pac del dns1
vppctl pac del dns2
vppctl pac del dns3
vppctl pac del dns4
vppctl pac del dns5
vppctl pac del dns6
vppctl pac del dns7
vppctl pac del dns8
vppctl pac del dns9
vppctl pac del dns10


#Update destination (vLB) IP
VLB_IPADDR=$(cat /opt/config/vlb_ipaddr.txt)
IPADDR1=$(ifconfig br0 | grep "inet addr" | tr -s ' ' | cut -d' ' -f3 | cut -d':' -f2)
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

#Update source ports (make them random)
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