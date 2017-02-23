#!/bin/bash

NEXUS_USERNAME=$(cat /opt/config/nexus_username.txt)
NEXUS_PASSWD=$(cat /opt/config/nexus_password.txt)
NEXUS_DOCKER_REPO=$(cat /opt/config/nexus_docker_repo.txt)

export DOCKER_REPOSITORY=${NEXUS_DOCKER_REPO}

cd /opt/policy
git pull

chmod +x config/drools/drools-tweaks.sh
IP_ADDRESS=$(ifconfig eth0 | grep "inet addr" | tr -s ' ' | cut -d' ' -f3 | cut -d':' -f2)
echo $IP_ADDRESS > config/pe/ip_addr.txt

docker login -u $NEXUS_USERNAME -p $NEXUS_PASSWD $NEXUS_DOCKER_REPO
/opt/docker/docker-compose up -d
