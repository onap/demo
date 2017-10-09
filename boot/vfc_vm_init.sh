#!/bin/bash

# Establish environment variables
NEXUS_USERNAME=$(cat /opt/config/nexus_username.txt)
NEXUS_PASSWD=$(cat /opt/config/nexus_password.txt)
NEXUS_DOCKER_REPO=$(cat /opt/config/nexus_docker_repo.txt)
DOCKER_IMAGE_VERSION=$(cat /opt/config/vfc_docker.txt)

source /opt/config/onap_ips.txt

# Refresh images
docker login -u $NEXUS_USERNAME -p $NEXUS_PASSWD $NEXUS_DOCKER_REPO
docker pull $NEXUS_DOCKER_REPO/onap/vfc/wfengine-activiti:$DOCKER_IMAGE_VERSION
docker pull $NEXUS_DOCKER_REPO/onap/vfc/wfengine-mgrservice:$DOCKER_IMAGE_VERSION
docker pull $NEXUS_DOCKER_REPO/onap/vfc/catalog:$DOCKER_IMAGE_VERSION
docker pull $NEXUS_DOCKER_REPO/onap/vfc/emsdriver:$DOCKER_IMAGE_VERSION
docker pull $NEXUS_DOCKER_REPO/onap/vfc/gvnfmdriver:$DOCKER_IMAGE_VERSION
docker pull $NEXUS_DOCKER_REPO/onap/vfc/jujudriver:$DOCKER_IMAGE_VERSION
docker pull $NEXUS_DOCKER_REPO/onap/vfc/nfvo/svnfm/huawei:$DOCKER_IMAGE_VERSION
docker pull $NEXUS_DOCKER_REPO/onap/vfc/nslcm:$DOCKER_IMAGE_VERSION
docker pull $NEXUS_DOCKER_REPO/onap/vfc/resmanagement:$DOCKER_IMAGE_VERSION
docker pull $NEXUS_DOCKER_REPO/onap/vfc/vnflcm:$DOCKER_IMAGE_VERSION
docker pull $NEXUS_DOCKER_REPO/onap/vfc/vnfmgr:$DOCKER_IMAGE_VERSION
docker pull $NEXUS_DOCKER_REPO/onap/vfc/vnfres:$DOCKER_IMAGE_VERSION
docker pull $NEXUS_DOCKER_REPO/onap/vfc/ztesdncdriver:$DOCKER_IMAGE_VERSION
docker pull $NEXUS_DOCKER_REPO/onap/vfc/ztevmanagerdriver:$DOCKER_IMAGE_VERSION
docker pull $NEXUS_DOCKER_REPO/onap/vfc/nfvo/svnfm/nokia:$DOCKER_IMAGE_VERSION

docker rm -f vfc_wfengine_mgrservice
docker rm -f vfc_wfengine_activiti
docker rm -f vfc_catalog
docker rm -f vfc_emsdriver
docker rm -f vfc_gvnfmdriver
docker rm -f vfc_jujudriver
docker rm -f vfc_svnfm_huawei
docker rm -f vfc_nslcm
docker rm -f vfc_resmanagement
docker rm -f vfc_vnflcm
docker rm -f vfc_vnfmgr
docker rm -f vfc_vnfres
docker rm -f vfc_ztesdncdriver
docker rm -f vfc_ztevmanagerdriver
docker rm -f vfc_svnfm_nokia

# Insert docker run instructions here
docker run -i -t -d --name vfc_wfengine_activiti -p 8804:8080 -e SERVICE_IP=$OPENO_IP -e OPENPALETTE_MSB_IP=$OPENO_IP -e OPENPALETTE_MSB_PORT=80 $NEXUS_DOCKER_REPO/onap/vfc/wfengine-activiti:$DOCKER_IMAGE_VERSION
docker run -i -t -d --name vfc_wfengine_mgrservice -p 8805:10550 -e SERVICE_IP=$OPENO_IP -e OPENPALETTE_MSB_IP=$OPENO_IP -e OPENPALETTE_MSB_PORT=80 $NEXUS_DOCKER_REPO/onap/vfc/wfengine-mgrservice:$DOCKER_IMAGE_VERSION
docker run -i -t -d --name vfc_catalog -e MSB_ADDR=$OPENO_IP:80 $NEXUS_DOCKER_REPO/onap/vfc/catalog:$DOCKER_IMAGE_VERSION
docker run -i -t -d --name vfc_emsdriver -e MSB_ADDR=$OPENO_IP:80 $NEXUS_DOCKER_REPO/onap/vfc/emsdriver:$DOCKER_IMAGE_VERSION
docker run -i -t -d --name vfc_gvnfmdriver -e MSB_ADDR=$OPENO_IP:80 $NEXUS_DOCKER_REPO/onap/vfc/gvnfmdriver:$DOCKER_IMAGE_VERSION
docker run -i -t -d --name vfc_jujudriver -e MSB_ADDR=$OPENO_IP:80 $NEXUS_DOCKER_REPO/onap/vfc/jujudriver:$DOCKER_IMAGE_VERSION
docker run -i -t -d --name vfc_svnfm_huawei -e MSB_ADDR=$OPENO_IP:80 $NEXUS_DOCKER_REPO/onap/vfc/nfvo/svnfm/huawei:$DOCKER_IMAGE_VERSION
docker run -i -t -d --name vfc_nslcm -e MSB_ADDR=$OPENO_IP:80 $NEXUS_DOCKER_REPO/onap/vfc/nslcm:$DOCKER_IMAGE_VERSION
docker run -i -t -d --name vfc_resmanagement -e MSB_ADDR=$OPENO_IP:80 $NEXUS_DOCKER_REPO/onap/vfc/resmanagement:$DOCKER_IMAGE_VERSION
docker run -i -t -d --name vfc_vnflcm -e MSB_ADDR=$OPENO_IP:80 $NEXUS_DOCKER_REPO/onap/vfc/vnflcm:$DOCKER_IMAGE_VERSION
docker run -i -t -d --name vfc_vnfmgr -e MSB_ADDR=$OPENO_IP:80 $NEXUS_DOCKER_REPO/onap/vfc/vnfmgr:$DOCKER_IMAGE_VERSION
docker run -i -t -d --name vfc_vnfres -e MSB_ADDR=$OPENO_IP:80 $NEXUS_DOCKER_REPO/onap/vfc/vnfres:$DOCKER_IMAGE_VERSION
docker run -i -t -d --name vfc_ztesdncdriver -e MSB_ADDR=$OPENO_IP:80 $NEXUS_DOCKER_REPO/onap/vfc/ztesdncdriver:$DOCKER_IMAGE_VERSION
docker run -i -t -d --name vfc_ztevmanagerdriver -e MSB_ADDR=$OPENO_IP:80 $NEXUS_DOCKER_REPO/onap/vfc/ztevmanagerdriver:$DOCKER_IMAGE_VERSION
docker run -i -t -d --name vfc_svnfm_nokia -e MSB_ADDR=$OPENO_IP:80 $NEXUS_DOCKER_REPO/onap/vfc/nfvo/svnfm/nokia:$DOCKER_IMAGE_VERSION