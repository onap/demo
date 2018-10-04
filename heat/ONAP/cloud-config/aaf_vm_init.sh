#!/bin/bash

CURRENT_DIR=$(pwd)

if [ ! -e /opt/authz/auth/docker/d.props ]; then
  cp /opt/authz/auth/docker/d.props.init /opt/authz/auth/docker/d.props
fi


NEXUS_USERNAME=$(cat /opt/config/nexus_username.txt)
NEXUS_PASSWD=$(cat /opt/config/nexus_password.txt)
NEXUS_DOCKER_REPO=$(cat /opt/config/nexus_docker_repo.txt)
sed -i "s/DOCKER_REPOSITORY=.*/DOCKER_REPOSITORY=$NEXUS_DOCKER_REPO/" /opt/authz/auth/docker/d.props
. /opt/authz/auth/docker/d.props

HOSTNAME=`hostname -f`
FQDN=aaf.api.simpledemo.onap.org
HOST_IP=$(cat /opt/config/public_ip.txt)

CASS_IP=`docker inspect aaf_cass | grep '"IPAddress' | head -1 | cut -d '"' -f 4`
CASS_HOST="cass.aaf.osaaf.org:"$CASS_IP
 
cd /opt/authz/auth/auth-cass/docker
if [ "`docker container ls | grep aaf_cass`" = "" ]; then
  # Cassandra Install
  echo Cassandra Install
  bash ./dinstall.sh
fi

CASS_IP=`docker inspect aaf_cass | grep '"IPAddress' | head -1 | cut -d '"' -f 4`
CASS_HOST="cass.aaf.osaaf.org:"$CASS_IP
if [ ! -e /opt/authz/auth/docker/cass.props ]; then
  cp /opt/authz/auth/docker/cass.props.init /opt/authz/auth/docker/cass.props
fi

sed -i "s/CASS_HOST=.*/CASS_HOST="$CASS_HOST"/g" /opt/authz/auth/docker/cass.props
# TODO Pull from Config Dir
if [ "$LATITUDE" = "" ]; then
  LATITUDE=37.781
  LONGITUDE=-122.261
  sed -i "s/LATITUDE=.*/LATITUDE=$LATITUDE/g" /opt/authz/auth/docker/d.props
  sed -i "s/LONGITUDE=.*/LONGITUDE=$LONGITUDE/g" /opt/authz/auth/docker/d.props
fi

sed -i "s/VERSION=.*/VERSION=$VERSION/g" /opt/authz/auth/docker/d.props
sed -i "s/HOSTNAME=.*/HOSTNAME=$HOSTNAME/g" /opt/authz/auth/docker/d.props
sed -i "s/HOST_IP=.*/HOST_IP=$HOST_IP/g" /opt/authz/auth/docker/d.props
sed -i "s/AAF_REGISTER_AS=.*/AAF_REGISTER_AS=$FQDN/g" /opt/authz/auth/docker/d.props

cd /opt/authz/auth/docker
# Need new Deployment system properties
bash ./aaf.sh

# run it
bash ./drun.sh
