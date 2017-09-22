#!/bin/bash
# Starts docker containers for VNFSDK VNF  repository.
# Version for Amsterdam/R1 uses docker-compose.

# be verbose
set -x

# Establish environment variables
NEXUS_USERNAME=$(cat /opt/config/nexus_username.txt)
NEXUS_PASSWD=$(cat /opt/config/nexus_password.txt)
NEXUS_DOCKER_REPO=$(cat /opt/config/nexus_docker_repo.txt)
export MTU=$(/sbin/ifconfig | grep MTU | sed 's/.*MTU://' | sed 's/ .*//' | sort -n | head -1)
#DOCKER_IMAGE_VERSION=$(cat /opt/config/docker_version.txt) --> not needed at the moment

# Refresh configuration and scripts
cd /opt/refrepo
git pull
cd vnfmarket-be/deployment/install

# Get image names used below from docker-compose environment file
source .env

# Refresh images
docker login -u $NEXUS_USERNAME -p $NEXUS_PASSWD $NEXUS_DOCKER_REPO
docker pull $NEXUS_DOCKER_REPO/onap/refrepo:${REFREPO_TAG}
docker pull $NEXUS_DOCKER_REPO/onap/refrepo:${POSTGRES_TAG}

# docker-compose is not in /usr/bin
/opt/docker/docker-compose down
/opt/docker/docker-compose up -d

