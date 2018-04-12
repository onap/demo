#!/bin/bash

NEXUS_USERNAME=$(cat /opt/config/nexus_username.txt)
NEXUS_PASSWD=$(cat /opt/config/nexus_password.txt)
NEXUS_DOCKER_REPO=$(cat /opt/config/nexus_docker_repo.txt)
RELEASE=$(cat /opt/config/sdc_wfd_docker.txt)

source /opt/config/onap_ips.txt

# pull sdc-workflow-designer docker image
docker login -u $NEXUS_USERNAME -p $NEXUS_PASSWD $NEXUS_DOCKER_REPO
docker pull $NEXUS_DOCKER_REPO/onap/sdc/sdc-workflow-designer:$RELEASE

docker rm -f sdc-workflow-designer


# setup sdc-workflow-designer docker image
docker run --detach --name sdc-workflow-designer --ulimit memlock=-1:-1 --memory 1g --memory-swap=1g --ulimit nofile=4096:100000 --publish 9527:8080 $NEXUS_DOCKER_REPO/onap/sdc/sdc-workflow-designer:$RELEASE
