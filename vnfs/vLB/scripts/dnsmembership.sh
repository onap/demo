#!/bin/bash

MY_PUBLIC_IP=$(cat /opt/config/local_public_ipaddr.txt)

java -jar dns-manager-1.0.0.jar $MY_PUBLIC_IP 8888 10 3 0
