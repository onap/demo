#!/bin/bash

# Start Honeycomb
VERSION=$(cat /opt/config/nb_api_version.txt)
cd /opt
./honeycomb/vlb-vnf-onap-distribution-$VERSION/honeycomb &>/var/log/honeycomb.log &disown
sleep 10

#Set GRE tunnel
./set_gre_tunnel.sh
