#!/bin/bash

CURRENT_DIR=$(pwd)

if [ ! -e /opt/authz/auth/docker/d.props ]; then
  cp /opt/authz/auth/docker/d.props.init /opt/authz/auth/docker/d.props
fi
. /opt/authz/auth/docker/d.props


NEXUS_USERNAME=$(cat /opt/config/nexus_username.txt)
NEXUS_PASSWD=$(cat /opt/config/nexus_password.txt)
if [ -e /opt/authz/auth/docker/d.props ]; then
  NEXUS_DOCKER_REPO=`grep "DOCKER_REPOSITORY=" /opt/authz/auth/docker/d.props`
else 
  NEXUS_DOCKER_REPO="DOCKER_REPOSITORY="
fi

if [ "$NEXUS_DOCKER_REPO" = "DOCKER_REPOSITORY=" ]; then
  NEXUS_DOCKER_REPO=$(cat /opt/config/nexus_docker_repo.txt)
else
  NEXUS_DOCKER_REPO=${NEXUS_DOCKER_REPO#DOCKER_REPOSITORY=}
fi

echo $NEXUS_DOCKER_REPO
HOSTNAME=`hostname -f`
FQDN=aaf.api.simpledemo.onap.org
HOST_IP=$(cat /opt/config/public_ip.txt)

cd /opt/authz/auth/auth-cass/docker
if [ "`docker container ls | grep aaf_cass`" = "" ]; then
  # Cassandra Install
  echo Phase 1 Cassandra Install
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

sed -i "s/DOCKER_REPOSITORY=.*/DOCKER_REPOSITORY=$NEXUS_DOCKER_REPO/g" /opt/authz/auth/docker/d.props
sed -i "s/VERSION=.*/VERSION=$VERSION/g" /opt/authz/auth/docker/d.props
sed -i "s/HOSTNAME=.*/HOSTNAME=$HOSTNAME/g" /opt/authz/auth/docker/d.props
sed -i "s/HOST_IP=.*/HOST_IP=$HOST_IP/g" /opt/authz/auth/docker/d.props

SIGNER_B64="$CURRENT_DIR/config/sample_ca/aaf.signer.b64"
SIGNER_P12="$CURRENT_DIR/config/sample_ca/aaf.signer.p12"
AAF_P12="$CURRENT_DIR/config/sample_ca/aaf.bootstrap.p12"
P12_PASSWORD="something easy"

if [ ! -e "$SIGNER_P12" ]; then
  mkdir -p "$CURRENT_DIR/config/sample_ca"
  base64 -d $SIGNER_B64 > $SIGNER_P12
fi

if [ ! -e "$AAF_P12" ]; then
  mkdir -p $CURRENT_DIR/sample_ca
  cd /opt/authz/conf/CA 
  /bin/bash bootstrap.sh $SIGNER_P12 "$P12_PASSWORD"
  if [ ! -e "aaf.bootstrap.p12" ]; then
	  echo "Certificates NOT created.  Stopping installation"
	  exit
  else
	  mv aaf.bootstrap.p12 $AAF_P12
  fi
  cd -
fi 

if [ -e "$AAF_P12" ]; then
    sed -i "s/AAF_INITIAL_X509_P12=.*/AAF_INITIAL_X509_P12=${AAF_P12//\//\\/}/g" /opt/authz/auth/docker/d.props
    sed -i "s/AAF_INITIAL_X509_PASSWORD=.*/AAF_INITIAL_X509_PASSWORD=\"$P12_PASSWORD\"/g" /opt/authz/auth/docker/d.props
fi

if [ -e "$SIGNER_P12" ]; then
    if [ "$CADI_X509_ISSUERS" != "" ]; then
	    CADI_X509_ISSUERS="$CADI_X509_ISSUERS:"
    fi
    # Pick the REAL subject off the P12
    SUBJECT=$(echo "$P12_PASSWORD" | openssl pkcs12 -info -clcerts -in $SIGNER_P12 -nokeys -passin stdin | grep subject)
    SUBJECT=${SUBJECT//\// }
    SUBJECT=${SUBJECT/subject= /}
    # Needs to be reversed, separated by ", "
    for S in $SUBJECT ; do
	if [ "$RSUBJECT" = "" ]; then
	   RSUBJECT=$S
	else
	   RSUBJECT="$S, $RSUBJECT"
        fi
    done
    ISSUERS="$CADI_X509_ISSUERS$RSUBJECT"
    sed -i "s/CADI_X509_ISSUERS=.*/CADI_X509_ISSUERS=\"$ISSUERS\"/g" /opt/authz/auth/docker/d.props
    sed -i "s/AAF_SIGNER_P12=.*/AAF_SIGNER_P12=${SIGNER_P12//\//\\/}/g" /opt/authz/auth/docker/d.props
    sed -i "s/AAF_SIGNER_PASSWORD=.*/AAF_SIGNER_PASSWORD=\"$P12_PASSWORD\"/g" /opt/authz/auth/docker/d.props
fi

cd /opt/authz/auth/docker
# Need new Deployment system properties
bash ./aaf.sh

# run it
bash ./drun.sh
