#!/bin/bash

# Start Honeycomb
cd /opt
./honeycomb/honeycomb &>/var/log/honeycomb.log &disown
sleep 10

#Set GRE tunnel
./set_gre_tunnel.sh
