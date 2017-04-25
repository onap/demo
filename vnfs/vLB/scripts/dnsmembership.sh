#!/bin/bash

MY_PUBLIC_IP=$(cat /opt/config/local_public_ipaddr.txt)
VERSION=$(cat /opt/config/demo_artifacts_version.txt)

java -jar dns-manager-$VERSION.jar $MY_PUBLIC_IP 8888 10 3 0
