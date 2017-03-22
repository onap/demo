#!/bin/bash

NEXUS_USERNAME=$(cat /opt/config/nexus_username.txt)
NEXUS_PASSWD=$(cat /opt/config/nexus_password.txt)
NEXUS_DOCKER_REPO=$(cat /opt/config/nexus_docker_repo.txt)
NEXUS_REPO=$(cat /opt/config/nexus_repo.txt)
DOCKER_IMAGE_VERSION=$(cat /opt/config/docker_version.txt)

#
# Deploy latest robot configuration 
#
cd /opt/testsuite/properties
git pull
cp integration_* /opt/eteshare/config
cp vm_config2robot.sh /opt/eteshare/config
cp ete.sh /opt
cp demo.sh /opt

chmod +x /opt/ete.sh
chmod +x /opt/demo.sh

/bin/bash /opt/eteshare/config/vm_config2robot.sh

docker login -u $NEXUS_USERNAME -p $NEXUS_PASSWD $NEXUS_DOCKER_REPO
docker pull $NEXUS_DOCKER_REPO/openecomp/testsuite:$DOCKER_IMAGE_VERSION
docker rm -f openecompete_container
docker run -d --name openecompete_container -v /opt/eteshare:/share -p 88:88 $NEXUS_DOCKER_REPO/openecomp/testsuite:$DOCKER_IMAGE_VERSION
