#!/bin/bash

NEXUS_USERNAME=$(cat /opt/config/nexus_username.txt)
NEXUS_PASSWD=$(cat /opt/config/nexus_password.txt)
NEXUS_DOCKER_REPO=$(cat /opt/config/nexus_docker_repo.txt)
DOCKER_IMAGE_VERSION=$(cat /opt/config/docker_version.txt)

export DOCKER_REPOSITORY=${NEXUS_DOCKER_REPO}

cd /opt/policy
git pull

chmod +x config/drools/drools-tweaks.sh

if [ -e /opt/config/public_ip.txt ]
then
  IP_ADDRESS=$(cat /opt/config/public_ip.txt)
else
  IP_ADDRESS=$(ifconfig eth0 | grep "inet addr" | tr -s ' ' | cut -d' ' -f3 | cut -d':' -f2)
fi

echo $IP_ADDRESS > config/pe/ip_addr.txt

docker login -u $NEXUS_USERNAME -p $NEXUS_PASSWD $NEXUS_DOCKER_REPO

docker pull $NEXUS_DOCKER_REPO/openecomp/policy/policy-db:$DOCKER_IMAGE_VERSION
docker tag $NEXUS_DOCKER_REPO/openecomp/policy/policy-db:$DOCKER_IMAGE_VERSION latest

docker pull $NEXUS_DOCKER_REPO/openecomp/policy/policy-pe:$DOCKER_IMAGE_VERSION
docker tag $NEXUS_DOCKER_REPO/openecomp/policy/policy-pe:$DOCKER_IMAGE_VERSION latest

docker pull $NEXUS_DOCKER_REPO/openecomp/policy/policy-drools:$DOCKER_IMAGE_VERSION
docker tag $NEXUS_DOCKER_REPO/openecomp/policy/policy-drools:$DOCKER_IMAGE_VERSION latest

docker pull $NEXUS_DOCKER_REPO/openecomp/policy/policy-nexus:$DOCKER_IMAGE_VERSION
docker tag $NEXUS_DOCKER_REPO/openecomp/policy/policy-nexus:$DOCKER_IMAGE_VERSION latest

/opt/docker/docker-compose up -d
