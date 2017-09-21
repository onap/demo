#!/bin/bash

# Establish environment variables
NEXUS_USERNAME=$(cat /opt/config/nexus_username.txt)
NEXUS_PASSWD=$(cat /opt/config/nexus_password.txt)
NEXUS_DOCKER_REPO=$(cat /opt/config/nexus_docker_repo.txt)
DOCKER_IMAGE_VERSION=$(cat /opt/config/vfc_docker.txt)

# Refresh images
docker login -u $NEXUS_USERNAME -p $NEXUS_PASSWD $NEXUS_DOCKER_REPO
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

# Insert docker run instructions here
docker run -i -t -d --name vfc_catalog -e MSB_ADDR=127.0.0.1:80 $NEXUS_DOCKER_REPO/onap/vfc/catalog:$DOCKER_IMAGE_VERSION
docker run -i -t -d --name vfc_emsdriver -e MSB_ADDR=127.0.0.1:80 $NEXUS_DOCKER_REPO/onap/vfc/emsdriver:$DOCKER_IMAGE_VERSION
docker run -i -t -d --name vfc_gvnfmdriver -e MSB_ADDR=127.0.0.1:80 $NEXUS_DOCKER_REPO/onap/vfc/gvnfmdriver:$DOCKER_IMAGE_VERSION
docker run -i -t -d --name vfc_jujudriver -e MSB_ADDR=127.0.0.1:80 $NEXUS_DOCKER_REPO/onap/vfc/jujudriver:$DOCKER_IMAGE_VERSION
docker run -i -t -d --name vfc_svnfm_huawei -e MSB_ADDR=127.0.0.1:80 $NEXUS_DOCKER_REPO/onap/vfc/nfvo/svnfm/huawei:$DOCKER_IMAGE_VERSION
docker run -i -t -d --name vfc_nslcm -e MSB_ADDR=127.0.0.1:80 $NEXUS_DOCKER_REPO/onap/vfc/nslcm:$DOCKER_IMAGE_VERSION
docker run -i -t -d --name vfc_resmanagement -e MSB_ADDR=127.0.0.1:80 $NEXUS_DOCKER_REPO/onap/vfc/resmanagement:$DOCKER_IMAGE_VERSION
docker run -i -t -d --name vfc_vnflcm -e MSB_ADDR=127.0.0.1:80 $NEXUS_DOCKER_REPO/onap/vfc/vnflcm:$DOCKER_IMAGE_VERSION
docker run -i -t -d --name vfc_vnfmgr -e MSB_ADDR=127.0.0.1:80 $NEXUS_DOCKER_REPO/onap/vfc/vnfmgr:$DOCKER_IMAGE_VERSION
docker run -i -t -d --name vfc_vnfres -e MSB_ADDR=127.0.0.1:80 $NEXUS_DOCKER_REPO/onap/vfc/vnfres:$DOCKER_IMAGE_VERSION
docker run -i -t -d --name vfc_ztesdncdriver -e MSB_ADDR=127.0.0.1:80 $NEXUS_DOCKER_REPO/onap/vfc/ztesdncdriver:$DOCKER_IMAGE_VERSION
docker run -i -t -d --name vfc_ztevmanagerdriver -e MSB_ADDR=127.0.0.1:80 $NEXUS_DOCKER_REPO/onap/vfc/ztevmanagerdriver:$DOCKER_IMAGE_VERSION