#!/bin/bash

NEXUS_USERNAME=$(cat /opt/config/nexus_username.txt)
NEXUS_PASSWD=$(cat /opt/config/nexus_password.txt)
export NEXUS_DOCKER_REPO=$(cat /opt/config/nexus_docker_repo.txt)
DOCKER_IMAGE_VERSION=$(cat /opt/config/docker_version.txt)
DGBUILDER_IMAGE_VERSION=$(cat /opt/config/dgbuilder_version.txt)
export MTU=$(/sbin/ifconfig | grep MTU | sed 's/.*MTU://' | sed 's/ .*//' | sort -n | head -1)
export DNS_IP_ADDR=$(cat /opt/config/dns_ip_addr.txt)

cd /opt/sdnc
git pull

cd /opt/sdnc/installation/src/main/yaml
docker login -u $NEXUS_USERNAME -p $NEXUS_PASSWD $NEXUS_DOCKER_REPO

docker pull $NEXUS_DOCKER_REPO/onap/sdnc-image:$DOCKER_IMAGE_VERSION
docker tag $NEXUS_DOCKER_REPO/onap/sdnc-image:$DOCKER_IMAGE_VERSION onap/sdnc-image:latest

docker pull $NEXUS_DOCKER_REPO/onap/sdnc-ansible-server-image:$DOCKER_IMAGE_VERSION
docker tag $NEXUS_DOCKER_REPO/onap/sdnc-ansible-server-image:$DOCKER_IMAGE_VERSION onap/sdnc-ansible-server-image:latest

docker pull $NEXUS_DOCKER_REPO/onap/admportal-sdnc-image:$DOCKER_IMAGE_VERSION
docker tag $NEXUS_DOCKER_REPO/onap/admportal-sdnc-image:$DOCKER_IMAGE_VERSION onap/admportal-sdnc-image:latest

docker pull $NEXUS_DOCKER_REPO/onap/ccsdk-dgbuilder-image:$DGBUILDER_IMAGE_VERSION
docker tag $NEXUS_DOCKER_REPO/onap/ccsdk-dgbuilder-image:$DGBUILDER_IMAGE_VERSION onap/ccsdk-dgbuilder-image:latest

docker pull $NEXUS_DOCKER_REPO/onap/sdnc-ueb-listener-image:$DOCKER_IMAGE_VERSION
docker tag $NEXUS_DOCKER_REPO/onap/sdnc-ueb-listener-image:$DOCKER_IMAGE_VERSION onap/sdnc-ueb-listener-image:latest

docker pull $NEXUS_DOCKER_REPO/onap/sdnc-dmaap-listener-image:$DOCKER_IMAGE_VERSION
docker tag $NEXUS_DOCKER_REPO/onap/sdnc-dmaap-listener-image:$DOCKER_IMAGE_VERSION onap/sdnc-dmaap-listener-image:latest

echo "Waiting for 10 minutes for SDC to start";
RES=$(curl -s -X GET   http://10.0.3.1:8080/sdc2/rest/healthCheck   -H 'Accept: application/json'   -H 'Cache-Control: no-cache'   -H 'Content-Type: application/json'   -H 'Postman-Token: ffeba4a6-82b6-44d8-87e6-8b510f1127fd' | jq '.componentsInfo[] | select(.healthCheckComponent == "BE") | .description')
counter=0;
while [[ $RES != "\"OK\"" ]]; do
	sleep 10;
	let counter=$counter+1
	if [[ $counter -eq 60 ]]; then
		break;
	fi
done

if [[ $RES == "\"OK\"" ]]; then
	echo "Starting SDNC containers"
else
	echo "Timeout expired before SDC BE health check passed. SDNC containers starting, but UEB Listener may not be healthy"
fi

/opt/docker/docker-compose up -d