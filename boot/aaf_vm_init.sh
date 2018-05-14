#!/bin/bash

NEXUS_USERNAME=$(cat /opt/config/nexus_username.txt)
NEXUS_PASSWD=$(cat /opt/config/nexus_password.txt)
NEXUS_DOCKER_REPO=$(cat /opt/config/nexus_docker_repo.txt)
DOCKER_IMAGE_VERSION=$(cat /opt/config/docker_version.txt)
HOSTNAME=`hostname`
FQDN=aaf.api.simpledemo.onap.org
HOST_IP=$(cat /opt/config/local_ip.txt)

docker login -u $NEXUS_USERNAME -p $NEXUS_PASSWD $NEXUS_DOCKER_REPO

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

sed -i "s/DOCKER_REPOSITORY=.*/DOCKER_REPOSITORY="$NEXUS_DOCKER_REPO"/g" /opt/authz/auth/docker/d.props
#sed -i "s/VERSION=.*/VERSION="$DOCKER_IMAGE_VERSION"/g" /opt/authz/auth/docker/d.props
sed -i "s/HOSTNAME=.*/HOSTNAME="$HOSTNAME"/g" /opt/authz/auth/docker/d.props
sed -i "s/HOST_IP=.*/HOST_IP="$HOST_IP"/g" /opt/authz/auth/docker/d.props
sed -i "s/CASS_HOST=.*/CASS_HOST="$CASS_HOST"/g" /opt/authz/auth/docker/d.props

if [ ! -e "/opt/app/osaaf/etc" ]; then
  # Nothing installed, install sample
  mkdir -p /opt/app/osaaf/logs
  cd /opt/app/osaaf/logs
  mkdir fs cm gui hello locate oauth service
  cd /opt
  cp -Rf /opt/authz/auth/sample/* /opt/app/osaaf
fi
# Set Location information
# Need new Deployment system properties
CADI_LATITUDE=37.781
CADI_LONGITUDE=-122.261

CADI_TRUST_MASKS="${HOST_IP%\.[0-9]*}\\/24,${CASS_IP%\.[0-9]*}\\/24"
sed -i "s/cadi_latitude=.*/cadi_latitude="$CADI_LATITUDE"/g" /opt/app/osaaf/local/org.osaaf.location.props
sed -i "s/cadi_longitude=.*/cadi_longitude="$CADI_LONGITUDE"/g" /opt/app/osaaf/local/org.osaaf.location.props
sed -i "s/cadi_registration_hostname=.*/cadi_registration_hostname="$FQDN"/g" /opt/app/osaaf/local/org.osaaf.location.props
sed -i "s/cadi_trust_masks=.*/cadi_trust_masks="$CADI_TRUST_MASKS"/g" /opt/app/osaaf/local/org.osaaf.location.props

cd /opt/authz/auth/docker
./dbuild.sh
sleep 5
./drun.sh
