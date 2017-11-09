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

docker run -d -p 8500:8500  --name msb_consul consul:0.9.3
CONSUL_IP=`sudo docker inspect -f '{{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}}' msb_consul`

docker run -d  -p 10081:10081  -e CONSUL_IP=$CONSUL_IP --name msb_discovery $NEXUS_DOCKER_REPO/onap/msb/msb_discovery:$DOCKER_IMAGE_VERSION
DISCOVERY_IP=`sudo docker inspect -f '{{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}}' msb_discovery`

docker run -d -p 80:80 -e CONSUL_IP=$CONSUL_IP -e SDCLIENT_IP=$DISCOVERY_IP -e "ROUTE_LABELS=visualRange:0|1" --name msb_apigateway $NEXUS_DOCKER_REPO/onap/msb/msb_apigateway:$DOCKER_IMAGE_VERSION

# Allow the MSB container to come up before registering services
sleep 20

# register ONAP services to MSB
#aai
curl -X POST -H "Content-Type: application/json" -d '{"serviceName": "aai-cloudInfrastructure", "version": "v11", "url": "/aai/v11/cloud-infrastructure","protocol": "REST", "enable_ssl":"true", "nodes": [ {"ip": "'$AAI_IP1'","port": "8443"}]}' "http://$OPENO_IP:10081/api/microservices/v1/services"

curl -X POST -H "Content-Type: application/json" -d '{"serviceName": "aai-cloudInfrastructure-deprecated", "version": "v11", "url": "/aai/v11/cloud-infrastructure","path": "/aai/v11/cloud-infrastructure","protocol": "REST", "enable_ssl":"true", "nodes": [ {"ip": "'$AAI_IP1'","port": "8443"}]}' "http://$OPENO_IP:10081/api/microservices/v1/services"

curl -X POST -H "Content-Type: application/json" -d '{"serviceName": "aai-business", "version": "v11", "url": "/aai/v11/business","protocol": "REST", "enable_ssl":"true", "nodes": [ {"ip": "'$AAI_IP1'","port": "8443"}]}' "http://$OPENO_IP:10081/api/microservices/v1/services"

curl -X POST -H "Content-Type: application/json" -d '{"serviceName": "aai-business-deprecated", "version": "v11", "url": "/aai/v11/business","path": "/aai/v11/business","protocol": "REST", "enable_ssl":"true", "nodes": [ {"ip": "'$AAI_IP1'","port": "8443"}]}' "http://$OPENO_IP:10081/api/microservices/v1/services"

curl -X POST -H "Content-Type: application/json" -d '{"serviceName": "aai-search", "version": "v11", "url": "/aai/v11/search","protocol": "REST", "enable_ssl":"true", "nodes": [ {"ip": "'$AAI_IP1'","port": "8443"}]}' "http://$OPENO_IP:10081/api/microservices/v1/services"

curl -X POST -H "Content-Type: application/json" -d '{"serviceName": "aai-search-deprecated", "version": "v11", "url": "/aai/v11/search","path": "/aai/v11/search","protocol": "REST", "enable_ssl":"true", "nodes": [ {"ip": "'$AAI_IP1'","port": "8443"}]}' "http://$OPENO_IP:10081/api/microservices/v1/services"

curl -X POST -H "Content-Type: application/json" -d '{"serviceName": "aai-actions", "version": "v11", "url": "/aai/v11/actions","protocol": "REST", "enable_ssl":"true", "nodes": [ {"ip": "'$AAI_IP1'","port": "8443"}]}' "http://$OPENO_IP:10081/api/microservices/v1/services"

curl -X POST -H "Content-Type: application/json" -d '{"serviceName": "aai-actions-deprecated", "version": "v11", "url": "/aai/v11/actions","path": "/aai/v11/actions","protocol": "REST", "enable_ssl":"true", "nodes": [ {"ip": "'$AAI_IP1'","port": "8443"}]}' "http://$OPENO_IP:10081/api/microservices/v1/services"

curl -X POST -H "Content-Type: application/json" -d '{"serviceName": "aai-service-design-and-creation", "version": "v11", "url": "/aai/v11/service-design-and-creation","protocol": "REST", "enable_ssl":"true", "nodes": [ {"ip": "'$AAI_IP1'","port": "8443"}]}' "http://$OPENO_IP:10081/api/microservices/v1/services"

curl -X POST -H "Content-Type: application/json" -d '{"serviceName": "aai-service-design-and-creation-deprecated", "version": "v11", "url": "/aai/v11/service-design-and-creation","path": "/aai/v11/service-design-and-creation","protocol": "REST", "enable_ssl":"true", "nodes": [ {"ip": "'$AAI_IP1'","port": "8443"}]}' "http://$OPENO_IP:10081/api/microservices/v1/services"

curl -X POST -H "Content-Type: application/json" -d '{"serviceName": "aai-network", "version": "v11", "url": "/aai/v11/network","protocol": "REST", "enable_ssl":"true", "nodes": [ {"ip": "'$AAI_IP1'","port": "8443"}]}' "http://$OPENO_IP:10081/api/microservices/v1/services"

curl -X POST -H "Content-Type: application/json" -d '{"serviceName": "aai-network-deprecated", "version": "v11", "url": "/aai/v11/network","path": "/aai/v11/network","protocol": "REST",  "enable_ssl":"true", "nodes": [ {"ip": "'$AAI_IP1'","port": "8443"}]}' "http://$OPENO_IP:10081/api/microservices/v1/services"

curl -X POST -H "Content-Type: application/json" -d '{"serviceName": "aai-externalSystem", "version": "v11", "url": "/aai/v11/external-system","protocol": "REST", "enable_ssl":"true", "visualRange":"1", "nodes": [ {"ip": "'$AAI_IP1'","port": "8443"}]}' "http://$OPENO_IP:10081/api/microservices/v1/services"

curl -X POST -H "Content-Type: application/json" -d '{"serviceName": "aai-externalSystem-deprecated", "version": "v11", "url": "/aai/v11/external-system","path": "/aai/v11/external-system","protocol": "REST",  "enable_ssl":"true", "nodes": [ {"ip": "'$AAI_IP1'","port": "8443"}]}' "http://$OPENO_IP:10081/api/microservices/v1/services"

curl -X POST -H "Content-Type: application/json" -d '{"serviceName": "aai-esr-server", "version": "v1", "url": "/api/aai-esr-server/v1","protocol": "REST", "visualRange":"1", "nodes": [ {"ip": "'$OPENO_IP'","port": "9518"}]}' "http://$OPENO_IP:10081/api/microservices/v1/services"

curl -X POST -H "Content-Type: application/json" -d '{"serviceName": "aai-esr-gui", "version": "v1", "url": "/esr-gui","path": "/iui/aai-esr-gui","protocol": "UI",  "nodes": [ {"ip": "'$OPENO_IP'","port": "9519"}]}' "http://$OPENO_IP:10081/api/microservices/v1/services"

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

curl -X POST -H "Content-Type: application/json" -d '{"serviceName": "sdnc-compatible", "version": "v1", "url": "/restconf","path": "/restconf","protocol": "REST",  "nodes": [ {"ip": "'$SDNC_IP'","port": "8282"}]}' "http://$OPENO_IP:10081/api/microservices/v1/services"

#multi-vim
curl -X POST -H "Content-Type: application/json" -d '{"serviceName": "multicloud", "version": "v0", "url": "/api/multicloud/v0","protocol": "REST",  "nodes": [ {"ip": "'$OPENO_IP'","port": "9001"}]}' "http://$OPENO_IP:10081/api/microservices/v1/services"

curl -X POST -H "Content-Type: application/json" -d '{"serviceName": "multicloud-vio", "version": "v0", "url": "/api/multicloud-vio/v0","protocol": "REST",  "nodes": [ {"ip": "'$OPENO_IP'","port": "9004"}]}' "http://$OPENO_IP:10081/api/microservices/v1/services"

curl -X POST -H "Content-Type: application/json" -d '{"serviceName": "multicloud-ocata", "version": "v0", "url": "/api/multicloud-ocata/v0","protocol": "REST",  "nodes": [ {"ip": "'$OPENO_IP'","port": "9006"}]}' "http://$OPENO_IP:10081/api/microservices/v1/services"

curl -X POST -H "Content-Type: application/json" -d '{"serviceName": "multicloud-titanium_cloud", "version": "v0", "url": "/api/multicloud-titanium_cloud/v0","protocol": "REST",  "nodes": [ {"ip": "'$OPENO_IP'","port": "9005"}]}' "http://$OPENO_IP:10081/api/microservices/v1/services"

#VF-C
curl -X POST -H "Content-Type: application/json" -d '{"serviceName": "nslcm", "version": "v1", "url": "/api/nslcm/v1","protocol": "REST",  "nodes": [ {"ip": "'$OPENO_IP'","port": "8403"}]}' "http://$OPENO_IP:10081/api/microservices/v1/services"

curl -X POST -H "Content-Type: application/json" -d '{"serviceName": "ztevmanagerdriver", "version": "v1", "url": "/api/ztevmanagerdriver/v1","protocol": "REST",  "nodes": [ {"ip": "'$OPENO_IP'","port": "8410"}]}' "http://$OPENO_IP:10081/api/microservices/v1/services"

curl -X POST -H "Content-Type: application/json" -d '{"serviceName": "ztesdncdriver", "version": "v1", "url": "/api/ztesdncdriver/v1","protocol": "REST",  "nodes": [ {"ip": "'$OPENO_IP'","port": "8411"}]}' "http://$OPENO_IP:10081/api/microservices/v1/services"

curl -X POST -H "Content-Type: application/json" -d '{"serviceName": "resmgr", "version": "v1", "url": "/api/resmgr/v1","protocol": "REST",  "nodes": [ {"ip": "'$OPENO_IP'","port": "8480"}]}' "http://$OPENO_IP:10081/api/microservices/v1/services"

curl -X POST -H "Content-Type: application/json" -d '{"serviceName": "gvnfmdriver", "version": "v1", "url": "/api/gvnfmdriver/v1","protocol": "REST",  "nodes": [ {"ip": "'$OPENO_IP'","port": "8484"}]}' "http://$OPENO_IP:10081/api/microservices/v1/services"

curl -X POST -H "Content-Type: application/json" -d '{"serviceName": "huaweivnfmdriver", "version": "v1", "url": "/api/huaweivnfmdriver/v1","protocol": "REST",  "nodes": [ {"ip": "'$OPENO_IP'","port": "8482"}]}' "http://$OPENO_IP:10081/api/microservices/v1/services"

curl -X POST -H "Content-Type: application/json" -d '{"serviceName": "nokiavnfmdriver", "version": "v1", "url": "/api/nokiavnfmdriver/v1","protocol": "REST",  "nodes": [ {"ip": "'$OPENO_IP'","port": "8485"}]}' "http://$OPENO_IP:10081/api/microservices/v1/services"

curl -X POST -H "Content-Type: application/json" -d '{"serviceName": "jujuvnfmdriver", "version": "v1", "url": "/api/jujuvnfmdriver/v1","protocol": "REST",  "nodes": [ {"ip": "'$OPENO_IP'","port": "8483"}]}' "http://$OPENO_IP:10081/api/microservices/v1/services"

curl -X POST -H "Content-Type: application/json" -d '{"serviceName": "vnflcm", "version": "v1", "url": "/api/vnflcm/v1","protocol": "REST",  "nodes": [ {"ip": "'$OPENO_IP'","port": "8801"}]}' "http://$OPENO_IP:10081/api/microservices/v1/services"

curl -X POST -H "Content-Type: application/json" -d '{"serviceName": "vnfres", "version": "v1", "url": "/api/vnfres/v1","protocol": "REST",  "nodes": [ {"ip": "'$OPENO_IP'","port": "8802"}]}' "http://$OPENO_IP:10081/api/microservices/v1/services"

curl -X POST -H "Content-Type: application/json" -d '{"serviceName": "vnfmgr", "version": "v1", "url": "/api/vnfmgr/v1","protocol": "REST",  "nodes": [ {"ip": "'$OPENO_IP'","port": "8803"}]}' "http://$OPENO_IP:10081/api/microservices/v1/services"

curl -X POST -H "Content-Type: application/json" -d '{"serviceName": "activiti", "version": "v1", "url": "/activiti-rest", "path": "/activiti-rest" "protocol": "REST",  "nodes": [ {"ip": "'$OPENO_IP'","port": "8804"}]}' "http://$OPENO_IP:10081/api/microservices/v1/services"

curl -X POST -H "Content-Type: application/json" -d '{"serviceName": "workflow", "version": "v1", "url": "/api/workflow/v1","protocol": "REST",  "nodes": [ {"ip": "'$OPENO_IP'","port": "8805"}]}' "http://$OPENO_IP:10081/api/microservices/v1/services"

curl -X POST -H "Content-Type: application/json" -d '{"serviceName": "catalog", "version": "v1", "url": "/api/catalog/v1","protocol": "REST",  "nodes": [ {"ip": "'$OPENO_IP'","port": "8806"}]}' "http://$OPENO_IP:10081/api/microservices/v1/services"

curl -X POST -H "Content-Type: application/json" -d '{"serviceName": "emsdriver", "version": "v1", "url": "/api/emsdriver/v1","protocol": "REST",  "nodes": [ {"ip": "'$OPENO_IP'","port": "8206"}]}' "http://$OPENO_IP:10081/api/microservices/v1/services"

#UUI
curl -X POST -H "Content-Type: application/json" -d '{"serviceName": "usecaseui-server", "version": "v1", "url": "/api/usecaseui/server/v1","protocol": "REST",  "nodes": [ {"ip": "'$OPENO_IP'","port": "8082"}]}' "http://$OPENO_IP:10081/api/microservices/v1/services"

curl -X POST -H "Content-Type: application/json" -d '{"serviceName": "usecaseui-ui", "version": "v1", "url": "/usecase-ui","path": "/iui/usecaseui","protocol": "UI",  "nodes": [ {"ip": "'$OPENO_IP'","port": "8080"}]}' "http://$OPENO_IP:10081/api/microservices/v1/services"

