#!/bin/bash

LB_OAM_INT=$(cat /opt/config/lb_oam_int.txt)
PID=$(cat /opt/config/local_private_ipaddr.txt)

java -jar dns-client-1.0.0.jar $PID $LB_OAM_INT 8888 10 0
