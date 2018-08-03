#!/bin/bash

NEXUS_USERNAME=$(cat /opt/config/nexus_username.txt)
NEXUS_PASSWD=$(cat /opt/config/nexus_password.txt)
NEXUS_DOCKER_REPO=$(cat /opt/config/nexus_docker_repo.txt)
DOCKER_IMAGE_VERSION=$(cat /opt/config/docker_version.txt)

docker pull library/vault:0.10.0
docker pull library/consul:1.0.6

docker login -u $NEXUS_USERNAME -p $NEXUS_PASSWD $NEXUS_DOCKER_REPO
docker pull $NEXUS_DOCKER_REPO/onap/aaf/sms:$DOCKER_IMAGE_VERSION
docker pull $NEXUS_DOCKER_REPO/onap/aaf/smsquorumclient:$DOCKER_IMAGE_VERSION

cd /opt/sms/sms-service/bin/deploy

#Stop existing docker containers (if any)
if [ $(docker ps | wc -l) > 1 ]; then
	./sms.sh stop
	sleep 10
fi

./sms.sh start