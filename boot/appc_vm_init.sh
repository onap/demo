#!/bin/bash

NEXUS_USERNAME=$(cat /opt/config/nexus_username.txt)
NEXUS_PASSWD=$(cat /opt/config/nexus_password.txt)
NEXUS_DOCKER_REPO=$(cat /opt/config/nexus_docker_repo.txt)
DMAAP_TOPIC=$(cat /opt/config/dmaap_topic.txt)

cd /opt/appc
git pull
cd /opt/appc/docker-compose

sed -i "s/DMAAP_TOPIC_ENV=.*/DMAAP_TOPIC_ENV="$DMAAP_TOPIC"/g" docker-compose.yml

docker login -u $NEXUS_USERNAME -p $NEXUS_PASSWD $NEXUS_DOCKER_REPO
/opt/docker/docker-compose pull
/opt/docker/docker-compose up -d
