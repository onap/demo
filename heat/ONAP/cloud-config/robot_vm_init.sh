#!/bin/bash

NEXUS_USERNAME=$(cat /opt/config/nexus_username.txt)
NEXUS_PASSWD=$(cat /opt/config/nexus_password.txt)
NEXUS_DOCKER_REPO=$(cat /opt/config/nexus_docker_repo.txt)
DOCKER_IMAGE_VERSION=$(cat /opt/config/docker_version.txt)
SNIRO_DOCKER_IMAGE_VERSION=$(cat /opt/config/sniro_docker_version.txt)

/bin/bash /opt/eteshare/config/vm_config2robot.sh

docker login -u $NEXUS_USERNAME -p $NEXUS_PASSWD $NEXUS_DOCKER_REPO
docker pull $NEXUS_DOCKER_REPO/onap/testsuite:$DOCKER_IMAGE_VERSION
docker rm -f openecompete_container

docker pull $NEXUS_DOCKER_REPO/onap/sniroemulator:$SNIRO_DOCKER_IMAGE_VERSION
docker rm -f sniroemulator

docker run -d --name openecompete_container -v /opt/eteshare:/share -p 88:88 $NEXUS_DOCKER_REPO/onap/testsuite:$DOCKER_IMAGE_VERSION
docker run -d --name sniroemulator -p 8080:9999 $NEXUS_DOCKER_REPO/onap/sniroemulator:$SNIRO_DOCKER_IMAGE_VERSION
