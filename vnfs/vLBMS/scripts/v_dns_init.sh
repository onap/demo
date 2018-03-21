#!/bin/bash

cd /opt
./vlb-vnf-onap-distribution-$(cat /opt/config/demo_artifacts_version.txt)-SNAPSHOT/honeycomb &>/var/log/honeycomb.log &disown
./set_gre_tunnel.sh
