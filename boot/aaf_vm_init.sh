#!/bin/bash

NEXUS_USERNAME=$(cat /opt/config/nexus_username.txt)
NEXUS_PASSWD=$(cat /opt/config/nexus_password.txt)
NEXUS_DOCKER_REPO=$(cat /opt/config/nexus_docker_repo.txt)
DOCKER_IMAGE_VERSION=$(cat /opt/config/docker_version.txt)
LOCAL_IP=$(cat /opt/config/local_ip.txt)
CASSANDRA_CONTAINER_NAME="aaf_cass"

docker login -u $NEXUS_USERNAME -p $NEXUS_PASSWD $NEXUS_DOCKER_REPO

cd /opt/authz
git pull

cd /opt/authz/auth/auth-cass/docker
./dinstall.sh
sleep 2

#Update configuration file
CASSANDRA_CONTAINER_IP=$(docker inspect -f '{{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}}' $CASSANDRA_CONTAINER_NAME)
sed -i "s/DOCKER_REPOSITORY=.*/DOCKER_REPOSITORY="$NEXUS_DOCKER_REPO"/g" /opt/authz/auth/docker/d.props
sed -i "s/VERSION=.*/VERSION="$DOCKER_IMAGE_VERSION"/g" /opt/authz/auth/docker/d.props
sed -i "s/HOSTNAME=.*/HOSTNAME="$(hostname)"/g" /opt/authz/auth/docker/d.props
sed -i "s/HOST_IP=.*/HOST_IP="$LOCAL_IP"/g" /opt/authz/auth/docker/d.props
sed -i "s/CASS_HOST=cass.aaf.osaaf.org:.*/CASS_HOST=cass.aaf.osaaf.org:"$CASSANDRA_CONTAINER_IP"/g" /opt/authz/auth/docker/d.props

cd /opt/authz/auth/docker
./drun.sh