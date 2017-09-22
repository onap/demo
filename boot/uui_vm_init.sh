#!/bin/bash

# Establish environment variables
NEXUS_USERNAME=$(cat /opt/config/nexus_username.txt)
NEXUS_PASSWD=$(cat /opt/config/nexus_password.txt)
NEXUS_DOCKER_REPO=$(cat /opt/config/nexus_docker_repo.txt)
DOCKER_IMAGE_VERSION=$(cat /opt/config/uui_docker.txt)

# Refresh images
docker login -u $NEXUS_USERNAME -p $NEXUS_PASSWD $NEXUS_DOCKER_REPO
docker pull $NEXUS_DOCKER_REPO/onap/uui/ui:$DOCKER_IMAGE_VERSION
docker pull $NEXUS_DOCKER_REPO/onap/uui/server:$DOCKER_IMAGE_VERSION

docker rm -f uui_ui
docker rm -f uui_server

# Insert docker run instructions here
docker run -i -t -d --name uui_ui -e MSB_ADDR=$OPENO_IP:80 $NEXUS_DOCKER_REPO/onap/uui/ui:$DOCKER_IMAGE_VERSION
docker run -i -t -d --name uui_server -e MSB_ADDR=$OPENO_IP:80 $NEXUS_DOCKER_REPO/onap/uui/server:$DOCKER_IMAGE_VERSION