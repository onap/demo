#!/bin/bash

# Set environment variables
export DOCKER_IMAGE_VERSION=$(cat /opt/config/docker_version.txt)
NEXUS_USERNAME=$(cat /opt/config/nexus_username.txt)
NEXUS_PASSWD=$(cat /opt/config/nexus_password.txt)
export NEXUS_DOCKER_REPO=$(cat /opt/config/nexus_docker_repo.txt)
export SDC_IP=$(cat /opt/config/sdc_ip_addr.txt)
export AAI_IP=$(cat /opt/config/aai_ip_addr.txt)
export SO_IP=$(cat /opt/config/so_ip_addr.txt)

# Create nbi directory
mkdir -p /opt/nbi
cd /opt/nbi

# Create .env file to access env variables for docker-compose
echo "SDC_IP=${SDC_IP}" > .env
echo "SO_IP=${SO_IP}" >> .env
echo "AAI_IP=${AAI_IP}" >> .env
echo "NEXUS_DOCKER_REPO=${NEXUS_DOCKER_REPO}" >> .env

# Fetch the latest docker-compose.yml
curl -o docker-compose.yml https://git.onap.org/externalapi/nbi/plain/docker-compose.yml?h=master

# Pull the nbi docker image from nexus
# MariaDB and mongoDB will be pulled automatically from docker.io during docker-compose
docker login -u $NEXUS_USERNAME -p $NEXUS_PASSWD $NEXUS_DOCKER_REPO

docker pull $NEXUS_DOCKER_REPO/onap/externalapi/nbi:$DOCKER_IMAGE_VERSION

# Start nbi, MariaDB and MongoDB containers with docker compose and nbi/docker-compose.yml
/opt/docker/docker-compose up -d
