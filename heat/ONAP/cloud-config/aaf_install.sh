#!/bin/bash
 
 export MTU=$(/sbin/ifconfig | grep MTU | sed 's/.*MTU://' | sed 's/ .*//' | sort -n | head -1)
 
 NEXUS_USERNAME=$(cat /opt/config/nexus_username.txt)
 NEXUS_PASSWD=$(cat /opt/config/nexus_password.txt)
 NEXUS_DOCKER_REPO=$(cat /opt/config/nexus_docker_repo.txt)
 DOCKER_IMAGE_VERSION=$(cat /opt/config/docker_version.txt)
 HOSTNAME=`hostname`
 FQDN=aaf.api.simpledemo.onap.org
 HOST_IP=$(cat /opt/config/local_ip.txt)
 
 
 
echo  docker login -u $NEXUS_USERNAME -p $NEXUS_PASSWD $NEXUS_DOCKER_REPO
exit
 
 cd /opt/authz
 git pull
 
 cd /opt/authz/auth/auth-cass/docker
 if [ "`docker container ls | grep aaf_cass`" = "" ]; then
   # Cassandra Install
   echo Phase 1 Cassandra Install
   ./dinstall.sh
 fi
 
 CASS_IP=`docker inspect aaf_cass | grep '"IPAddress' | head -1 | cut -d '"' -f 4`
 CASS_HOST="cass.aaf.osaaf.org:"$CASS_IP
 
 docker pull $NEXUS_DOCKER_REPO/onap/aaf/aaf_agent:latest
 docker pull $NEXUS_DOCKER_REPO/onap/aaf/aaf_config:latest
 docker pull $NEXUS_DOCKER_REPO/onap/aaf/aaf_core:latest
 docker pull $NEXUS_DOCKER_REPO/onap/aaf/aaf_cm:latest
 docker pull $NEXUS_DOCKER_REPO/onap/aaf/aaf_fs:latest
 docker pull $NEXUS_DOCKER_REPO/onap/aaf/aaf_gui:latest
 docker pull $NEXUS_DOCKER_REPO/onap/aaf/aaf_hello:latest
 docker pull $NEXUS_DOCKER_REPO/onap/aaf/aaf_locate:latest
 docker pull $NEXUS_DOCKER_REPO/onap/aaf/aaf_oauth:latest
 docker pull $NEXUS_DOCKER_REPO/onap/aaf/aaf_service:latest
 
 
 cd /opt/authz/auth/docker
 bash ./drun.sh
