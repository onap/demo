#!/bin/bash

NEXUS_USERNAME=$(cat /opt/config/nexus_username.txt)
NEXUS_PASSWD=$(cat /opt/config/nexus_password.txt)
NEXUS_DOCKER_REPO=$(cat /opt/config/nexus_docker_repo.txt)
DOCKER_IMAGE_VERSION=$(cat /opt/config/docker_version.txt)

docker pull library/zookeeper:3.4
docker pull library/tomcat:8.5

docker login -u $NEXUS_USERNAME -p $NEXUS_PASSWD $NEXUS_DOCKER_REPO
docker pull $NEXUS_DOCKER_REPO/onap/music/cassandra_music:$DOCKER_IMAGE_VERSION
docker pull $NEXUS_DOCKER_REPO/onap/music/music:$DOCKER_IMAGE_VERSION

cd /opt/music/distribution/dockermusic
git pull

#Stop existing docker containers (if any)
if [ $(docker ps | wc -l) > 1 ]; then
	./music.sh stop
	sleep 2
fi

./music.sh start