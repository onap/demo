#!/bin/bash

# Start Honeycomb
VERSION=$(cat /opt/config/demo_artifacts_version.txt)
cd /opt
./honeycomb-api/vnfs/vLBMS/apis/vlb-vnf-onap-distribution/target/vlb-vnf-onap-distribution-$VERSION-hc/vlb-vnf-onap-distribution-$VERSION/honeycomb &>/var/log/honeycomb.log &disown
sleep 10

#Set GRE tunnel
./set_gre_tunnel.sh
