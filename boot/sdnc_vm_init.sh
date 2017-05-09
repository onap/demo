#!/bin/bash

NEXUS_USERNAME=$(cat /opt/config/nexus_username.txt)
NEXUS_PASSWD=$(cat /opt/config/nexus_password.txt)
export NEXUS_DOCKER_REPO=$(cat /opt/config/nexus_docker_repo.txt)
DOCKER_IMAGE_VERSION=$(cat /opt/config/docker_version.txt)
export MTU=$(/sbin/ifconfig | grep MTU | sed 's/.*MTU://' | sed 's/ .*//' | sort -n | head -1)

cd /opt/sdnc
git pull

cd /opt/sdnc/installation/src/main/yaml
docker login -u $NEXUS_USERNAME -p $NEXUS_PASSWD $NEXUS_DOCKER_REPO

docker pull $NEXUS_DOCKER_REPO/openecomp/sdnc-image:$DOCKER_IMAGE_VERSION
docker tag $NEXUS_DOCKER_REPO/openecomp/sdnc-image:$DOCKER_IMAGE_VERSION openecomp/sdnc-image:latest

docker pull $NEXUS_DOCKER_REPO/openecomp/admportal-sdnc-image:$DOCKER_IMAGE_VERSION
docker tag $NEXUS_DOCKER_REPO/openecomp/admportal-sdnc-image:$DOCKER_IMAGE_VERSION openecomp/admportal-sdnc-image:latest

docker pull $NEXUS_DOCKER_REPO/openecomp/dgbuilder-sdnc-image:$DOCKER_IMAGE_VERSION
docker tag $NEXUS_DOCKER_REPO/openecomp/dgbuilder-sdnc-image:$DOCKER_IMAGE_VERSION openecomp/dgbuilder-sdnc-image:latest

/opt/docker/docker-compose up -d
