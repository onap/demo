#!/bin/bash

NEXUS_USERNAME=$(cat /opt/config/nexus_username.txt)
NEXUS_PASSWD=$(cat /opt/config/nexus_password.txt)
NEXUS_DOCKER_REPO=$(cat /opt/config/nexus_docker_repo.txt)
DOCKER_IMAGE_VERSION=$(cat /opt/config/docker_version.txt)

# Fetch the latest docker-compose.yml
cd /opt/nbi
wget https://git.onap.org/externalapi/nbi/tree/docker-compose.yml?h=master

# Pull the nbi docker image from nexus
# MariaDB and mongoDB will be pulled automatically from docker.io during docker-compose
docker login -u $NEXUS_USERNAME -p $NEXUS_PASSWD $NEXUS_DOCKER_REPO

docker pull $NEXUS_DOCKER_REPO/onap/externalapi/nbi:$DOCKER_IMAGE_VERSION

# Start nbi, MariaDB and MongoDB containers with docker compose and nbi/docker-compose.yml
/opt/docker/docker-compose up -d