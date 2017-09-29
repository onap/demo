#!/bin/bash

NEXUS_USER=$(cat /opt/config/nexus_username.txt)
NEXUS_PASSWORD=$(cat /opt/config/nexus_password.txt)
NEXUS_DOCKER_REPO=$(cat /opt/config/nexus_docker_repo.txt)
DOCKER_VERSION=$(cat /opt/config/docker_version.txt)
ZONE=$(cat /opt/config/dcae_zone.txt)

docker login -u $NEXUS_USER -p $NEXUS_PASSWORD $NEXUS_DOCKER_REPO
docker pull $NEXUS_DOCKER_REPO/onap/org.onap.dcaegen2.deployments.bootstrap:$DOCKER_VERSION
docker run -v /opt/config/priv_key:/opt/app/installer/config/key -v /opt/app/inputs.yaml:/opt/app/installer/config/inputs.yaml -e "LOCATION=$ZONE" $NEXUS_DOCKER_REPO/onap/org.onap.dcaegen2.deployments.bootstrap:$DOCKER_VERSION