#!/bin/bash

NEXUS_USERNAME=$(cat /opt/config/nexus_username.txt)
NEXUS_PASSWD=$(cat /opt/config/nexus_password.txt)
NEXUS_DOCKER_REPO=$(cat /opt/config/nexus_docker_repo.txt)
DOCKER_IMAGE_VERSION=$(cat /opt/config/msb_docker.txt)

source /opt/config/onap_ips.txt

# start up MSB
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

# Allow the MSB container to come up before registering services
sleep 20

# register ONAP services to MSB
#aai
curl -X POST -H "Content-Type: application/json" -d '{"serviceName": "aai-cloudInfrastructure", "version": "v11", "url": "/aai/v11/cloud-infrastructure","protocol": "REST",  "nodes": [ {"ip": "'$AAI_IP1'","port": "8443"}]}' "http://$OPENO_IP:10081/api/microservices/v1/services"

curl -X POST -H "Content-Type: application/json" -d '{"serviceName": "aai-cloudInfrastructure-deprecated", "version": "v11", "url": "/aai/v11/cloud-infrastructure","path": "/aai/v11/cloud-infrastructure","protocol": "REST",  "nodes": [ {"ip": "'$AAI_IP1'","port": "8443"}]}' "http://$OPENO_IP:10081/api/microservices/v1/services"

curl -X POST -H "Content-Type: application/json" -d '{"serviceName": "aai-business", "version": "v11", "url": "/aai/v11/business","protocol": "REST",  "nodes": [ {"ip": "'$AAI_IP1'","port": "8443"}]}' "http://$OPENO_IP:10081/api/microservices/v1/services"

curl -X POST -H "Content-Type: application/json" -d '{"serviceName": "aai-business-deprecated", "version": "v11", "url": "/aai/v11/business","path": "/aai/v11/business","protocol": "REST",  "nodes": [ {"ip": "'$AAI_IP1'","port": "8443"}]}' "http://$OPENO_IP:10081/api/microservices/v1/services"

curl -X POST -H "Content-Type: application/json" -d '{"serviceName": "aai-search", "version": "v11", "url": "/aai/v11/search","protocol": "REST",  "nodes": [ {"ip": "'$AAI_IP1'","port": "8443"}]}' "http://$OPENO_IP:10081/api/microservices/v1/services"

curl -X POST -H "Content-Type: application/json" -d '{"serviceName": "aai-search-deprecated", "version": "v11", "url": "/aai/v11/search","path": "/aai/v11/search","protocol": "REST",  "nodes": [ {"ip": "'$AAI_IP1'","port": "8443"}]}' "http://$OPENO_IP:10081/api/microservices/v1/services"

curl -X POST -H "Content-Type: application/json" -d '{"serviceName": "aai-actions", "version": "v11", "url": "/aai/v11/actions","protocol": "REST",  "nodes": [ {"ip": "'$AAI_IP1'","port": "8443"}]}' "http://$OPENO_IP:10081/api/microservices/v1/services"

curl -X POST -H "Content-Type: application/json" -d '{"serviceName": "aai-actions-deprecated", "version": "v11", "url": "/aai/v11/actions","path": "/aai/v11/actions","protocol": "REST",  "nodes": [ {"ip": "'$AAI_IP1'","port": "8443"}]}' "http://$OPENO_IP:10081/api/microservices/v1/services"

curl -X POST -H "Content-Type: application/json" -d '{"serviceName": "aai-service-design-and-creation", "version": "v11", "url": "/aai/v11/service-design-and-creation","protocol": "REST",  "nodes": [ {"ip": "'$AAI_IP1'","port": "8443"}]}' "http://$OPENO_IP:10081/api/microservices/v1/services"

curl -X POST -H "Content-Type: application/json" -d '{"serviceName": "aai-service-design-and-creation-deprecated", "version": "v11", "url": "/aai/v11/service-design-and-creation","path": "/aai/v11/service-design-and-creation","protocol": "REST",  "nodes": [ {"ip": "'$AAI_IP1'","port": "8443"}]}' "http://$OPENO_IP:10081/api/microservices/v1/services"

curl -X POST -H "Content-Type: application/json" -d '{"serviceName": "aai-network", "version": "v11", "url": "/aai/v11/network","protocol": "REST",  "nodes": [ {"ip": "'$AAI_IP1'","port": "8443"}]}' "http://$OPENO_IP:10081/api/microservices/v1/services"

curl -X POST -H "Content-Type: application/json" -d '{"serviceName": "aai-network-deprecated", "version": "v11", "url": "/aai/v11/network","path": "/aai/v11/network","protocol": "REST",  "nodes": [ {"ip": "'$AAI_IP1'","port": "8443"}]}' "http://$OPENO_IP:10081/api/microservices/v1/services"

#so
curl -X POST -H "Content-Type: application/json" -d '{"serviceName": "so", "version": "v1", "url": "/ecomp/mso/infra","protocol": "REST",  "nodes": [ {"ip": "'$SO_IP'","port": "8080"}]}' "http://$OPENO_IP:10081/api/microservices/v1/services"

curl -X POST -H "Content-Type: application/json" -d '{"serviceName": "so-deprecated", "version": "v1", "url": "/ecomp/mso/infra","path": "/ecomp/mso/infra","protocol": "REST",  "nodes": [ {"ip": "'$SO_IP'","port": "8080"}]}' "http://$OPENO_IP:10081/api/microservices/v1/services"

#Dmaap message router
#curl -X POST -H "Content-Type: application/json" -d '{"serviceName": "message-router", "version": "v1", "url": "/","protocol": "REST",  "nodes": [ {"ip": "'$DMAAP_IP'","port": "3904"}]}' "http://$OPENO_IP:10081/api/microservices/v1/services"

#policy
curl -X POST -H "Content-Type: application/json" -d '{"serviceName": "policy-pdp", "version": "v1", "url": "/pdp","protocol": "REST",  "nodes": [ {"ip": "'$POLICY_IP'","port": "8081"}]}' "http://$OPENO_IP:10081/api/microservices/v1/services"

curl -X POST -H "Content-Type: application/json" -d '{"serviceName": "policy-pdp-deprecated", "version": "v1", "url": "/pdp","path": "/pdp","protocol": "REST",  "nodes": [ {"ip": "'$POLICY_IP'","port": "8081"}]}' "http://$OPENO_IP:10081/api/microservices/v1/services"

#portal
curl -X POST -H "Content-Type: application/json" -d '{"serviceName": "portal", "version": "v2", "url": "/","protocol": "REST",  "nodes": [ {"ip": "'$PORTAL_IP'","port": "8989"}]}' "http://$OPENO_IP:10081/api/microservices/v1/services"

#sdc
curl -X POST -H "Content-Type: application/json" -d '{"serviceName": "sdc", "version": "v1", "url": "/sdc/v1","protocol": "REST",  "nodes": [ {"ip": "'$SDC_IP'","port": "8080"}]}' "http://$OPENO_IP:10081/api/microservices/v1/services"

curl -X POST -H "Content-Type: application/json" -d '{"serviceName": "sdc-deprecated", "version": "v1", "url": "/sdc/v1","path": "/sdc/v1","protocol": "REST",  "nodes": [ {"ip": "'$SDC_IP'","port": "8080"}]}' "http://$OPENO_IP:10081/api/microservices/v1/services"

#sdnc
curl -X POST -H "Content-Type: application/json" -d '{"serviceName": "sdnc", "version": "v1", "url": "/","protocol": "REST",  "nodes": [ {"ip": "'$SDNC_IP'","port": "8282"}]}' "http://$OPENO_IP:10081/api/microservices/v1/services"

curl -X POST -H "Content-Type: application/json" -d '{"serviceName": "sdnc", "version": "v1", "url": "/restconf","path": "/restconf","protocol": "REST",  "nodes": [ {"ip": "'$SDNC_IP'","port": "8282"}]}' "http://$OPENO_IP:10081/api/microservices/v1/services"

#multi-vim
curl -X POST -H "Content-Type: application/json" -d '{"serviceName": "multicloud-titanium_cloud", "version": "v0", "url": "/api/multicloud-titanium_cloud/v0","protocol": "REST",  "nodes": [ {"ip": "'$OPENO_IP'","port": "9005"}]}' "http://$OPENO_IP:10081/api/microservices/v1/services"