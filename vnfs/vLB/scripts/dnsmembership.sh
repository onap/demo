#!/bin/bash

IP_TO_PKTGEN_NET=$(cat /opt/config/ip_to_pktgen_net.txt)
VERSION=$(cat /opt/config/demo_artifacts_version.txt)

java -jar dns-manager-$VERSION.jar $IP_TO_PKTGEN_NET 8888 10 3 0
