#!/bin/bash

NEXUS_USERNAME=$(cat /opt/config/nexus_username.txt)
NEXUS_PASSWD=$(cat /opt/config/nexus_password.txt)
NEXUS_DOCKER_REPO=$(cat /opt/config/nexus_docker_repo.txt)
DOCKER_IMAGE_VERSION=$(cat /opt/config/msb_docker.txt)
NEXUS_REPO=$(cat /opt/config/nexus_repo.txt)
ARTIFACTS_VERSION=$(cat /opt/config/artifacts_version.txt)

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
export ONAP_HOST_URL=http://$OPENO_IP:80
export CLI_PRODUCT_VERSION=onap-1.1
onap -v

#aai
onap microservice-create --service-name aai-cloudInfrastructure --service-version v11 --service-url /aai/v11/cloud-infrastructure $AAI_IP1 8443

onap microservice-create --service-name aai-cloudInfrastructure-deprecated --service-version v11 --service-url /aai/v11/cloud-infrastructure $AAI_IP1 8443

onap microservice-create --service-name aai-business --service-version v11 --service-url /aai/v11/business $AAI_IP1 8443

onap microservice-create --service-name aai-business-deprecated --service-version v11 --service-url /aai/v11/business $AAI_IP1 8443

onap microservice-create --service-name aai-search --service-version v11 --service-url /aai/v11/search $AAI_IP1 8443

onap microservice-create --service-name aai-search-deprecated --service-version v11 --service-url /aai/v11/search $AAI_IP1 8443

onap microservice-create --service-name aai-actions --service-version v11 --service-url /aai/v11/actions $AAI_IP1 8443

onap microservice-create --service-name aai-actions-deprecated --service-version v11 --service-url /aai/v11/actions $AAI_IP1 8443

onap microservice-create --service-name aai-service-design-and-creation --service-version v11 --service-url /aai/v11/service-design-and-creation $AAI_IP1 8443

onap microservice-create --service-name aai-service-design-and-creation-deprecated --service-version v11 --service-url /aai/v11/service-design-and-creation $AAI_IP1 8443

onap microservice-create --service-name aai-network --service-version v11 --service-url /aai/v11/network $AAI_IP1 8443

onap microservice-create --service-name aai-network-deprecated --service-version v11 --service-url /aai/v11/network $AAI_IP1 8443

#so
onap microservice-create --service-name so --service-version v1 --service-url /ecomp/mso/infra $SO_IP 8080

onap microservice-create --service-name so-deprecated --service-version v1 --service-url /ecomp/mso/infra $SO_IP 8080

#Dmaap message router
#curl -X POST -H "Content-Type: application/json" '{"serviceName": "message-router", "version": "v1", "url": "/","protocol": "REST",  "nodes": [ {"ip": "'$DMAAP_IP'","port": "3904"}]}' "http://$OPENO_IP:10081/api/microservices/v1/services"

#policy
onap microservice-create --service-name policy-pdp --service-version v1 --service-url /ecomp/mso/infra $POLICY_IP 8081

onap microservice-create --service-name policy-pdp-deprecated --service-version v1 --service-url /ecomp/mso/infra $POLICY_IP 8081

#portal
onap microservice-create --service-name portal --service-version v2 --service-url / $PORTAL_IP 8989

#sdc
onap microservice-create --service-name sdc --service-version v1 --service-url /sdc/v1 $SDC_IP 8080

onap microservice-create --service-name sdc-deprecated --service-version v1 --service-url /sdc/v1 $SDC_IP 8080

#sdnc
onap microservice-create --service-name sdnc --service-version v1 --service-url /sdc/v1 $SDNC_IP 8282

onap microservice-create --service-name sdnc-deprecated --service-version v1 --service-url /restconf $SDNC_IP 8282

#multi-vim
onap microservice-create --service-name multicloud-titanium_cloud --service-version v0 --service-url /api/multicloud-titanium_cloud/v0 $OPENO_IP 9005

#VF-C
onap microservice-create --service-name nslcm --service-version v1 --service-url /api/nslcm/v1 $OPENO_IP 8403

onap microservice-create --service-name ztevmanagerdriver --service-version v1 --service-url /api/ztevmanagerdriver/v1 $OPENO_IP 8410

onap microservice-create --service-name ztesdncdriver --service-version v1 --service-url /api/ztesdncdriver/v1 $OPENO_IP 8411

onap microservice-create --service-name resmgr --service-version v1 --service-url /api/resmgr/v1 $OPENO_IP 8480

onap microservice-create --service-name gvnfmdriver --service-version v1 --service-url /api/gvnfmdriver/v1 $OPENO_IP 8484

onap microservice-create --service-name huaweivnfmdriver --service-version v1 --service-url /api/huaweivnfmdriver/v1 $OPENO_IP 8482

onap microservice-create --service-name nokiavnfmdriver --service-version v1 --service-url /api/nokiavnfmdriver/v1 $OPENO_IP 8485

onap microservice-create --service-name jujuvnfmdriver --service-version v1 --service-url /api/jujuvnfmdriver/v1 $OPENO_IP 8483

onap microservice-create --service-name vnflcm --service-version v1 --service-url /api/vnflcm/v1 $OPENO_IP 8801

onap microservice-create --service-name vnfres --service-version v1 --service-url /api/vnfres/v1 $OPENO_IP 8802

onap microservice-create --service-name vnfmgr --service-version v1 --service-url /api/vnfmgr/v1 $OPENO_IP 8803

onap microservice-create --service-name activiti --service-version v1 --service-url /api/activiti/v1 $OPENO_IP 8804

onap microservice-create --service-name activiti --service-version v1 --service-url /api/workflow/v1 $OPENO_IP 8805

onap microservice-create --service-name catalog --service-version v1 --service-url /api/catalog/v1 $OPENO_IP 8806

onap microservice-create --service-name emsdriver --service-version v1 --service-url /api/emsdriver/v1 $OPENO_IP 8206

#UUI
onap microservice-create --service-name usecaseui --service-version v1 --service-url /api/usecaseui/server/v1 $OPENO_IP 8901

onap microservice-create --service-name usecaseui-gui --service-version v1 --service-url /iui/usecaseui $OPENO_IP 8900

#Print the registered services
onap microservice-list --long
