#!/bin/bash

NEXUS_USERNAME=$(cat /opt/config/nexus_username.txt)
NEXUS_PASSWD=$(cat /opt/config/nexus_password.txt)
NEXUS_DOCKER_REPO=`grep "DOCKER_REPOSITORY=" /opt/authz/auth/docker/d.props`
if [ "$NEXUS_DOCKER_REPO" = "DOCKER_REPOSITORY=" ]; then
  NEXUS_DOCKER_REPO=$(cat /opt/config/nexus_docker_repo.txt)
else
  NEXUS_DOCKER_REPO=${NEXUS_DOCKER_REPO#DOCKER_REPOSITORY=}
fi

HOSTNAME=`hostname`
FQDN=aaf.api.simpledemo.onap.org
HOST_IP=$(cat /opt/config/local_ip.txt)

docker login -u $NEXUS_USERNAME -p $NEXUS_PASSWD $NEXUS_DOCKER_REPO

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
  bash ./dinstall.sh
fi

VERSION=$(grep VERSION /opt/authz/auth/docker/d.props.init)
VERSION=${VERSION#VERSION=}
CASS_IP=`docker inspect aaf_cass | grep '"IPAddress' | head -1 | cut -d '"' -f 4`
CASS_HOST="cass.aaf.osaaf.org:"$CASS_IP
sed -i "s/CASS_HOST=.*/CASS_HOST="$CASS_HOST"/g" /opt/authz/auth/docker/cass.props
# TODO Pull from Config Dir
CADI_LATITUDE=37.781
CADI_LONGITUDE=-122.261

sed -i "s/DOCKER_REPOSITORY=.*/DOCKER_REPOSITORY="$NEXUS_DOCKER_REPO"/g" /opt/authz/auth/docker/d.props
sed -i "s/VERSION=.*/VERSION="$VERSION"/g" /opt/authz/auth/docker/d.props
sed -i "s/HOSTNAME=.*/HOSTNAME="$HOSTNAME"/g" /opt/authz/auth/docker/d.props
sed -i "s/HOST_IP=.*/HOST_IP="$HOST_IP"/g" /opt/authz/auth/docker/d.props
sed -i "s/CADI_LATITUDE=.*/CADI_LATITUDE="$CADI_LATITUDE"/g" /opt/authz/auth/docker/d.props
sed -i "s/CASS_LONGITUDE=.*/CADI_LONGITUDE="$CADI_LONGITUDE"/g" /opt/authz/auth/docker/d.props
# Set Location information
# Need new Deployment system properties

if [ ! -e "/opt/app/osaaf/etc" ]; then
  # Nothing installed, install sample
  mkdir -p /opt/app/osaaf/logs
  cd /opt/app/osaaf/logs
  mkdir -p fs cm gui hello locate oauth service
  cd /opt
  cp -Rf /opt/authz/auth/sample/* /opt/app/osaaf
fi

cd /opt/authz/auth/docker
#
# Build AAF config volume
#./dbuild.sh
#sleep 5

# Create or check Credentials on startup
bash ./aaf.sh

bash ./drun.sh
