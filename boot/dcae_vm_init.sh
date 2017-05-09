#!/bin/bash

export MTU=$(/sbin/ifconfig | grep MTU | sed 's/.*MTU://' | sed 's/ .*//' | sort -n | head -1)

cd /opt/dcae-startup-vm-controller
git pull
bash init.sh
make up
