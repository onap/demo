#!/bin/bash

#Disable all the running streams
vppctl packet-gen disable

#Initial configuration: run only two streams
vppctl packet-gen enable-stream dns1
vppctl packet-gen enable-stream dns2

sleep 180

#Rehash port numbers and re-run five streams every minute
while true; do
	vppctl packet-gen disable
	vppctl pac del dns1
	vppctl pac del dns2
	vppctl pac del dns3
	vppctl pac del dns4
	vppctl pac del dns5

	#Update destination (vLB) IP
	VLB_IPADDR=$(cat /opt/config/vlb_ipaddr.txt)
	IPADDR1=$(cat /opt/config/local_private_ipaddr.txt)
	sed -i -e "0,/UDP/ s/UDP:.*/UDP: "$IPADDR1" -> "$VLB_IPADDR"/" /opt/dns_streams/stream_dns1
	sed -i -e "0,/UDP/ s/UDP:.*/UDP: "$IPADDR1" -> "$VLB_IPADDR"/" /opt/dns_streams/stream_dns2
	sed -i -e "0,/UDP/ s/UDP:.*/UDP: "$IPADDR1" -> "$VLB_IPADDR"/" /opt/dns_streams/stream_dns3
	sed -i -e "0,/UDP/ s/UDP:.*/UDP: "$IPADDR1" -> "$VLB_IPADDR"/" /opt/dns_streams/stream_dns4
	sed -i -e "0,/UDP/ s/UDP:.*/UDP: "$IPADDR1" -> "$VLB_IPADDR"/" /opt/dns_streams/stream_dns5

	#Update source ports (make them random)
	sed -i -e "s/.*-> 53.*/    UDP: $RANDOM -> 53/" /opt/dns_streams/stream_dns1
	sed -i -e "s/.*-> 53.*/    UDP: $RANDOM -> 53/" /opt/dns_streams/stream_dns2
	sed -i -e "s/.*-> 53.*/    UDP: $RANDOM -> 53/" /opt/dns_streams/stream_dns3
	sed -i -e "s/.*-> 53.*/    UDP: $RANDOM -> 53/" /opt/dns_streams/stream_dns4
	sed -i -e "s/.*-> 53.*/    UDP: $RANDOM -> 53/" /opt/dns_streams/stream_dns5

	vppctl exec /opt/dns_streams/stream_dns1
	vppctl exec /opt/dns_streams/stream_dns2
	vppctl exec /opt/dns_streams/stream_dns3
	vppctl exec /opt/dns_streams/stream_dns4
	vppctl exec /opt/dns_streams/stream_dns5

	#Resume stream execution
	vppctl packet-gen enable-stream dns1
	vppctl packet-gen enable-stream dns2
	vppctl packet-gen enable-stream dns3
	vppctl packet-gen enable-stream dns4
	vppctl packet-gen enable-stream dns5

	sleep 60
done