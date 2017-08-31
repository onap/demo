#!/bin/bash

NEXUS_USERNAME=$(cat /opt/config/nexus_username.txt)
NEXUS_PASSWD=$(cat /opt/config/nexus_password.txt)
NEXUS_DOCKER_REPO=$(cat /opt/config/nexus_docker_repo.txt)
DOCKER_IMAGE_VERSION=$(cat /opt/config/docker_version.txt)

# Fetch the latest code/scripts
cd /opt/clamp
git pull

# Remove unused folders as only extra/ folder is used for docker compose
rm -rf pom.xml
rm -rf src/

# No configuration change here as directly done in the CLAMP repo

# Pull the clamp docker image from nexus and tag it
# Maria db will be pulled automatically from docker.io during docker-compose
docker login -u $NEXUS_USERNAME -p $NEXUS_PASSWD $NEXUS_DOCKER_REPO

docker pull $NEXUS_DOCKER_REPO/onap/clamp:$DOCKER_IMAGE_VERSION
docker tag $NEXUS_DOCKER_REPO/onap/clamp:$DOCKER_IMAGE_VERSION onap/clamp:latest

cd extra/docker/clamp/

# Start Clamp and MariaDB containers with docker compose and clamp/extra/docker/clamp/docker-compose.yml
/opt/docker/docker-compose up -d
