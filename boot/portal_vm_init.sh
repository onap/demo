#!/bin/bash
# Starts docker containers for ONAP Portal in Rackspace.
# Version for Amsterdam/R1 uses docker-compose.

# be verbose
set -x

# Establish environment variables
NEXUS_USERNAME=$(cat /opt/config/nexus_username.txt)
NEXUS_PASSWD=$(cat /opt/config/nexus_password.txt)
NEXUS_DOCKER_REPO=$(cat /opt/config/nexus_docker_repo.txt)
DOCKER_IMAGE_VERSION=$(cat /opt/config/docker_version.txt)
# Use "latest" to deploy snapshot images:
# DOCKER_IMAGE_VERSION=latest
CLI_DOCKER_VERSION=$(cat /opt/config/cli_docker_version.txt)

# Refresh configuration and scripts
cd /opt/portal
git pull
cd deliveries

# Get image names used below from docker-compose environment file
source .env

# Refresh images
docker login -u $NEXUS_USERNAME -p $NEXUS_PASSWD $NEXUS_DOCKER_REPO
docker pull $NEXUS_DOCKER_REPO/$DB_IMG_NAME:$DOCKER_IMAGE_VERSION
docker pull $NEXUS_DOCKER_REPO/$EP_IMG_NAME:$DOCKER_IMAGE_VERSION
docker pull $NEXUS_DOCKER_REPO/$WMS_IMG_NAME:$DOCKER_IMAGE_VERSION
docker pull $NEXUS_DOCKER_REPO/$CLI_IMG_NAME:$CLI_DOCKER_VERSION

# Tag them as expected by docker-compose file
docker tag $NEXUS_DOCKER_REPO/$DB_IMG_NAME:$DOCKER_IMAGE_VERSION $DB_IMG_NAME:$PORTAL_TAG
docker tag $NEXUS_DOCKER_REPO/$EP_IMG_NAME:$DOCKER_IMAGE_VERSION $EP_IMG_NAME:$PORTAL_TAG
docker tag $NEXUS_DOCKER_REPO/$WMS_IMG_NAME:$DOCKER_IMAGE_VERSION $WMS_IMG_NAME:$PORTAL_TAG
docker tag $NEXUS_DOCKER_REPO/$CLI_IMG_NAME:$CLI_DOCKER_VERSION $CLI_IMG_NAME:$PORTAL_TAG

# Export variable for local logs directory, and create directory too
# The leading "./" is required for docker-compose
export LOGS_DIR=./logs
mkdir -p $LOGS_DIR

# Export variable for subdirectory with appropriate property files
# The leading "./" is required for docker-compose
export PROPS_DIR=./properties_simpledemo

# docker-compose is not in /usr/bin
/opt/docker/docker-compose down
/opt/docker/docker-compose up -d
