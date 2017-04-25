#!/bin/bash

LB_OAM_INT=$(cat /opt/config/lb_oam_int.txt)
PID=$(cat /opt/config/local_private_ipaddr.txt)
VERSION=$(cat /opt/config/demo_artifacts_version.txt)

java -jar dns-client-$VERSION.jar $PID $LB_OAM_INT 8888 10 0
