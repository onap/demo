#!/bin/bash

NEXUS_USERNAME=$(cat /opt/config/nexus_username.txt)
NEXUS_PASSWD=$(cat /opt/config/nexus_password.txt)
NEXUS_DOCKER_REPO=$(cat /opt/config/nexus_docker_repo.txt)
DOCKER_IMAGE_VERSION=$(cat /opt/config/docker_version.txt)

docker login -u $NEXUS_USERNAME -p $NEXUS_PASSWD $NEXUS_DOCKER_REPO

cd /opt/authz
git pull

sed -i "s/DOCKER_REPOSITORY=.*/DOCKER_REPOSITORY="$NEXUS_DOCKER_REPO"/g" /opt/authz/auth/docker/d.props
sed -i "s/VERSION=.*/VERSION="$DOCKER_IMAGE_VERSION"/g" /opt/authz/auth/docker/d.props

cd /opt/authz/auth/auth-cass/docker
./dinstall.sh

sleep 2

cd /opt/authz/auth/docker
./drun.sh