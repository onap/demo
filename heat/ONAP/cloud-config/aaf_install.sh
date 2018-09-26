#!/bin/bash
 
CURRENT_DIR=$(pwd)
export MTU=$(/sbin/ifconfig | grep MTU | sed 's/.*MTU://' | sed 's/ .*//' | sort -n | head -1)
 
NEXUS_USERNAME=$(cat /opt/config/nexus_username.txt)
NEXUS_PASSWD=$(cat /opt/config/nexus_password.txt)
NEXUS_DOCKER_REPO=$(cat /opt/config/nexus_docker_repo.txt)
AAF_DOCKER_VERSION=$(cat /opt/config/docker_version.txt)
 
docker login -u $NEXUS_USERNAME -p "$NEXUS_PASSWD" $NEXUS_DOCKER_REPO

if [ -e "/opt/authz" ]; then
  cd /opt/authz
  git pull
else
  cd /opt
  git clone https://gerrit.onap.org/r/aaf/authz
  cd authz
fi
cd $CURRENT_DIR

docker pull $NEXUS_DOCKER_REPO/onap/aaf/aaf_cass:$AAF_DOCKER_VERSION
docker pull $NEXUS_DOCKER_REPO/onap/aaf/aaf_config:$AAF_DOCKER_VERSION
docker pull $NEXUS_DOCKER_REPO/onap/aaf/aaf_cm:$AAF_DOCKER_VERSION
docker pull $NEXUS_DOCKER_REPO/onap/aaf/aaf_fs:$AAF_DOCKER_VERSION
docker pull $NEXUS_DOCKER_REPO/onap/aaf/aaf_gui:$AAF_DOCKER_VERSION
docker pull $NEXUS_DOCKER_REPO/onap/aaf/aaf_hello:$AAF_DOCKER_VERSION
docker pull $NEXUS_DOCKER_REPO/onap/aaf/aaf_locate:$AAF_DOCKER_VERSION
docker pull $NEXUS_DOCKER_REPO/onap/aaf/aaf_oauth:$AAF_DOCKER_VERSION
docker pull $NEXUS_DOCKER_REPO/onap/aaf/aaf_service:$AAF_DOCKER_VERSION
 
/bin/bash ./aaf_vm_init.sh 

