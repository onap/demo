#!/bin/bash

export MTU=$(/sbin/ifconfig | grep MTU | sed 's/.*MTU://' | sed 's/ .*//' | sort -n | head -1)

cd /opt/dcae-startup-vm-message-router
git pull
bash deploy.sh
