#!/bin/bash

# Establish environment variables
NEXUS_USERNAME=$(cat /opt/config/nexus_username.txt)
NEXUS_PASSWD=$(cat /opt/config/nexus_password.txt)
NEXUS_DOCKER_REPO=$(cat /opt/config/nexus_docker_repo.txt)

source /opt/config/onap_ips.txt
source /opt/config/vfc_docker.txt

# Refresh images
docker login -u $NEXUS_USERNAME -p $NEXUS_PASSWD $NEXUS_DOCKER_REPO
docker pull $NEXUS_DOCKER_REPO/onap/vfc/db:$DB_DOCKER_VER
docker pull $NEXUS_DOCKER_REPO/onap/vfc/wfengine-activiti:$ACTIVITI_DOCKER_VER
docker pull $NEXUS_DOCKER_REPO/onap/vfc/wfengine-mgrservice:$MGRSERVICE_DOCKER_VER
docker pull $NEXUS_DOCKER_REPO/onap/vfc/catalog:$CATALOG_DOCKER_VER
docker pull $NEXUS_DOCKER_REPO/onap/vfc/multivimproxy:$MULTIVIMPROXY_DOCKER_VER
docker pull $NEXUS_DOCKER_REPO/onap/vfc/emsdriver:$EMSDRIVER_DOCKER_VER
docker pull $NEXUS_DOCKER_REPO/onap/vfc/gvnfmdriver:$GVNFMDRIVER_DOCKER_VER
docker pull $NEXUS_DOCKER_REPO/onap/vfc/jujudriver:$JUJUDRIVER_DOCKER_VER
docker pull $NEXUS_DOCKER_REPO/onap/vfc/nfvo/svnfm/huawei:$HUAWEI_DOCKER_VER
docker pull $NEXUS_DOCKER_REPO/onap/vfc/nslcm:$NSLCM_DOCKER_VER
docker pull $NEXUS_DOCKER_REPO/onap/vfc/resmanagement:$RESMANAGEMENT_DOCKER_VER
docker pull $NEXUS_DOCKER_REPO/onap/vfc/vnflcm:$VNFLCM_DOCKER_VER
docker pull $NEXUS_DOCKER_REPO/onap/vfc/vnfmgr:$VNFMGR_DOCKER_VER
docker pull $NEXUS_DOCKER_REPO/onap/vfc/vnfres:$VNFRES_DOCKER_VER
docker pull $NEXUS_DOCKER_REPO/onap/vfc/ztesdncdriver:$ZTESDNCDRIVER_DOCKER_VER
docker pull $NEXUS_DOCKER_REPO/onap/vfc/ztevnfmdriver:$ZTEVNFMDRIVER_DOCKER_VER
docker pull $NEXUS_DOCKER_REPO/onap/vfc/nfvo/svnfm/nokia:$NOKIA_DOCKER_VER
docker pull $NEXUS_DOCKER_REPO/onap/vfc/nfvo/svnfm/nokiav2:$NOKIAV2_DOCKER_VER

docker rm -f vfc_db
docker rm -f vfc_wfengine_mgrservice
docker rm -f vfc_wfengine_activiti
docker rm -f vfc_catalog
docker rm -f vfc_emsdriver
docker rm -f vfc_multivimproxy
docker rm -f vfc_gvnfmdriver
docker rm -f vfc_jujudriver
docker rm -f vfc_svnfm_huawei
docker rm -f vfc_nslcm
docker rm -f vfc_resmanagement
docker rm -f vfc_vnflcm
docker rm -f vfc_vnfmgr
docker rm -f vfc_vnfres
docker rm -f vfc_ztesdncdriver
docker rm -f vfc_ztevnfmdriver
docker rm -f vfc_svnfm_nokia
docker rm -f vfc_svnfm_nokiav2

# Insert docker run instructions here
docker run -i -t -d --name vfc_db -p 3306:3306 -p 6379:6379 $NEXUS_DOCKER_REPO/onap/vfc/db:$DB_DOCKER_VER
VFC_DB_IP=`sudo docker inspect -f '{{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}}' vfc_db`
docker run -i -t -d --name vfc_wfengine_activiti -p 8804:8080 -e SERVICE_IP=$MULTISERV_IP -e SERVICE_PORT=8804 -e OPENPALETTE_MSB_IP=$MULTISERV_IP -e OPENPALETTE_MSB_PORT=80 $NEXUS_DOCKER_REPO/onap/vfc/wfengine-activiti:$ACTIVITI_DOCKER_VER
docker run -i -t -d --name vfc_wfengine_mgrservice -p 8805:10550 -e SERVICE_IP=$MULTISERV_IP -e SERVICE_PORT=8805 -e OPENPALETTE_MSB_IP=$MULTISERV_IP -e OPENPALETTE_MSB_PORT=80 $NEXUS_DOCKER_REPO/onap/vfc/wfengine-mgrservice:$MGRSERVICE_DOCKER_VER
docker run -i -t -d --name vfc_catalog -p 8806:8806 -e MYSQL_ADDR=${VFC_DB_IP}:3306 -e MSB_ADDR=$MULTISERV_IP:80 -e SERVICE_IP=$MULTISERV_IP $NEXUS_DOCKER_REPO/onap/vfc/catalog:$CATALOG_DOCKER_VER
docker run -i -t -d --name vfc_emsdriver -p 8206:8206 -e MSB_ADDR=$MULTISERV_IP:80 -e SERVICE_IP=$MULTISERV_IP -e VES_ADDR=$DCAE_COLL_IP:8080 -e VES_AUTHINFO="":"" $NEXUS_DOCKER_REPO/onap/vfc/emsdriver:$EMSDRIVER_DOCKER_VER
docker run -i -t -d --name vfc_gvnfmdriver -p 8484:8484 -e MSB_ADDR=$MULTISERV_IP:80 -e SERVICE_IP=$MULTISERV_IP $NEXUS_DOCKER_REPO/onap/vfc/gvnfmdriver:$GVNFMDRIVER_DOCKER_VER
docker run -i -t -d --name vfc_jujudriver -p 8483:8483 -e MSB_ADDR=$MULTISERV_IP:80 -e SERVICE_IP=$MULTISERV_IP $NEXUS_DOCKER_REPO/onap/vfc/jujudriver:$JUJUDRIVER_DOCKER_VER
docker run -i -t -d --name vfc_svnfm_huawei -p 8482:8482 -p 8443:8443 -e MSB_ADDR=$MULTISERV_IP:80 -e SERVICE_IP=$MULTISERV_IP $NEXUS_DOCKER_REPO/onap/vfc/nfvo/svnfm/huawei:$HUAWEI_DOCKER_VER
docker run -i -t -d --name vfc_nslcm -p 8403:8403 -e MYSQL_ADDR=${VFC_DB_IP}:3306 -e MSB_ADDR=$MULTISERV_IP:80 -e SERVICE_IP=$MULTISERV_IP $NEXUS_DOCKER_REPO/onap/vfc/nslcm:$NSLCM_DOCKER_VER
docker run -i -t -d --name vfc_resmanagement -p 8480:8480 -e MSB_ADDR=$MULTISERV_IP:80 -e SERVICE_IP=$MULTISERV_IP $NEXUS_DOCKER_REPO/onap/vfc/resmanagement:$RESMANAGEMENT_DOCKER_VER
docker run -i -t -d --name vfc_vnflcm -p 8801:8801 -e MYSQL_ADDR=${VFC_DB_IP}:3306 -e MSB_ADDR=$MULTISERV_IP:80 -e SERVICE_IP=$MULTISERV_IP $NEXUS_DOCKER_REPO/onap/vfc/vnflcm:$VNFLCM_DOCKER_VER
docker run -i -t -d --name vfc_vnfmgr -p 8803:8803 -e MYSQL_ADDR=${VFC_DB_IP}:3306 -e MSB_ADDR=$MULTISERV_IP:80 -e SERVICE_IP=$MULTISERV_IP $NEXUS_DOCKER_REPO/onap/vfc/vnfmgr:$VNFMGR_DOCKER_VER
docker run -i -t -d --name vfc_vnfres -p 8802:8802 -e MYSQL_ADDR=${VFC_DB_IP}:3306 -e MSB_ADDR=$MULTISERV_IP:80 -e SERVICE_IP=$MULTISERV_IP $NEXUS_DOCKER_REPO/onap/vfc/vnfres:$VNFRES_DOCKER_VER
docker run -i -t -d --name vfc_ztesdncdriver -p 8411:8411 -e MSB_ADDR=$MULTISERV_IP:80 -e SERVICE_IP=$MULTISERV_IP $NEXUS_DOCKER_REPO/onap/vfc/ztesdncdriver:$ZTESDNCDRIVER_DOCKER_VER
docker run -i -t -d --name vfc_ztevnfmdriver -p 8410:8410 -e MSB_ADDR=$MULTISERV_IP:80 -e SERVICE_IP=$MULTISERV_IP $NEXUS_DOCKER_REPO/onap/vfc/ztevnfmdriver:$ZTEVNFMDRIVER_DOCKER_VER
docker run -i -t -d --name vfc_svnfm_nokia -p 8486:8486 -e MSB_ADDR=$MULTISERV_IP:80 -e SERVICE_IP=$MULTISERV_IP $NEXUS_DOCKER_REPO/onap/vfc/nfvo/svnfm/nokia:$NOKIA_DOCKER_VER
docker run -i -t -d --name vfc_multivimproxy -p 8481:8481 -e MSB_ADDR=$MULTISERV_IP:80 -e SERVICE_IP=$MULTISERV_IP $NEXUS_DOCKER_REPO/onap/vfc/multivimproxy:$MULTIVIMPROXY_DOCKER_VER

VNFM_ID=vnfmUuid
docker run -i -t -d --name vfc_svnfm_nokiav2 -p 8089:8089 -e EXTERNAL_PORT=8089 -e MSB_IP=$MULTISERV_IP -e CONFIGURE=kuku -e EXTERNAL_IP=$MULTISERV_IP -e "VNFM_ID=$VNFM_ID" -d --stop-timeout 300 $NEXUS_DOCKER_REPO/onap/vfc/nfvo/svnfm/nokiav2:$NOKIAV2_DOCKER_VER
