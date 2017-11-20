#!/bin/bash

export MTU=$(/sbin/ifconfig | grep MTU | sed 's/.*MTU://' | sed 's/ .*//' | sort -n | head -1)

cd /opt/dcae-startup-vm-message-router
sed -i 's|wget .*|wget -q \"http://archive.apache.org/dist/kafka/${KAFKA_VERSION}/kafka_${SCALA_VERSION}-${KAFKA_VERSION}.tgz\" \\|g' deploy.sh
bash deploy.sh