#!/bin/bash
 
CURRENT_DIR=$(pwd)
export MTU=$(/sbin/ifconfig | grep MTU | sed 's/.*MTU://' | sed 's/ .*//' | sort -n | head -1)
 
NEXUS_USERNAME=$(cat /opt/config/nexus_username.txt)
NEXUS_PASSWD=$(cat /opt/config/nexus_password.txt)
NEXUS_DOCKER_REPO=$(cat /opt/config/nexus_docker_repo.txt)
AAF_DOCKER_VERSION=$(cat /opt/config/docker_version.txt)
HOSTNAME=`hostname -f`
FQDN=aaf.api.simpledemo.onap.org
HOST_IP=$(cat /opt/config/local_ip.txt)
 
echo "$NEXUS_PASSWD" | docker login -u $NEXUS_USERNAME --password-stdin $NEXUS_DOCKER_REPO

if [ -e "/opt/authz" ]; then
  cd /opt/authz
  git pull
else
  cd /opt
  git clone https://gerrit.onap.org/r/aaf/authz
  cd authz
fi

cd /opt/authz/auth/auth-cass/docker
if [ "`docker container ls | grep aaf_cass`" = "" ]; then
   # Cassandra Install
   echo Phase 1 Cassandra Install
   /bin/bash ./dinstall.sh
fi
 
CASS_IP=`docker inspect aaf_cass | grep '"IPAddress' | head -1 | cut -d '"' -f 4`
CASS_HOST="cass.aaf.osaaf.org:"$CASS_IP
 
docker pull $NEXUS_DOCKER_REPO/onap/aaf/aaf_config:$AAF_DOCKER_VERSION
docker pull $NEXUS_DOCKER_REPO/onap/aaf/aaf_cm:$AAF_DOCKER_VERSION
docker pull $NEXUS_DOCKER_REPO/onap/aaf/aaf_fs:$AAF_DOCKER_VERSION
docker pull $NEXUS_DOCKER_REPO/onap/aaf/aaf_gui:$AAF_DOCKER_VERSION
docker pull $NEXUS_DOCKER_REPO/onap/aaf/aaf_hello:$AAF_DOCKER_VERSION
docker pull $NEXUS_DOCKER_REPO/onap/aaf/aaf_locate:$AAF_DOCKER_VERSION
docker pull $NEXUS_DOCKER_REPO/onap/aaf/aaf_oauth:$AAF_DOCKER_VERSION
docker pull $NEXUS_DOCKER_REPO/onap/aaf/aaf_service:$AAF_DOCKER_VERSION
 
cd $CURRENT_DIR
/bin/bash ./aaf_vm_init.sh 

