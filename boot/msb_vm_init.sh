#!/bin/bash

NEXUS_USERNAME=$(cat /opt/config/nexus_username.txt)
NEXUS_PASSWD=$(cat /opt/config/nexus_password.txt)
NEXUS_DOCKER_REPO=$(cat /opt/config/nexus_docker_repo.txt)
DOCKER_IMAGE_VERSION=$(cat /opt/config/msb_docker.txt)

source /opt/config/onap_ips.txt

docker login -u $NEXUS_USERNAME -p $NEXUS_PASSWD $NEXUS_DOCKER_REPO
docker pull $NEXUS_DOCKER_REPO/onap/msb/msb_discovery:$DOCKER_IMAGE_VERSION
docker pull $NEXUS_DOCKER_REPO/onap/msb/msb_apigateway:$DOCKER_IMAGE_VERSION

docker rm -f msb_consul
docker rm -f msb_discovery
docker rm -f msb_apigateway

docker run -d -p 8500:8500  --name msb_consul consul
CONSUL_IP=`sudo docker inspect -f '{{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}}' msb_consul`

docker run -d  -p 10081:10081  -e CONSUL_IP=$CONSUL_IP --name msb_discovery $NEXUS_DOCKER_REPO/onap/msb/msb_discovery:$DOCKER_IMAGE_VERSION
DISCOVERY_IP=`sudo docker inspect -f '{{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}}' msb_discovery`

docker run -d -p 80:80 -e CONSUL_IP=$CONSUL_IP -e SDCLIENT_IP=$DISCOVERY_IP -e "ROUTE_LABELS=visualRange:0|1" --name msb_apigateway $NEXUS_DOCKER_REPO/onap/msb/msb_apigateway:$DOCKER_IMAGE_VERSION