#!/bin/bash

NEXUS_USERNAME=$(cat /opt/config/nexus_username.txt)
NEXUS_PASSWD=$(cat /opt/config/nexus_password.txt)
NEXUS_DOCKER_REPO=$(cat /opt/config/nexus_docker_repo.txt)
NEXUS_REPO=$(cat /opt/config/nexus_repo.txt)

#
# Deploy latest robot configuration 
# 
wget --user=$NEXUS_USERNAME --password=$NEXUS_PASSWD --no-check-certificate -O /opt/eteshare/config/robot_properties_ete.py  $NEXUS_REPO/org.openecomp.boot/robot_properties_ete.py
wget --user=$NEXUS_USERNAME --password=$NEXUS_PASSWD --no-check-certificate -O /opt/eteshare/config/robot_preload_parameters.py  $NEXUS_REPO/org.openecomp.boot/robot_preload_parameters.py
wget --user=$NEXUS_USERNAME --password=$NEXUS_PASSWD --no-check-certificate -O /opt/eteshare/config/vm_config2robot.sh  $NEXUS_REPO/org.openecomp.boot/vm_config2robot.sh
/bin/bash /opt/eteshare/config/vm_config2robot.sh

docker login -u $NEXUS_USERNAME -p $NEXUS_PASSWD $NEXUS_DOCKER_REPO
docker pull $NEXUS_DOCKER_REPO/openecompete
docker rm -f openecompete_container
docker run -d --name openecompete_container -v /opt/eteshare:/share -p 88:88 $NEXUS_DOCKER_REPO/openecompete
