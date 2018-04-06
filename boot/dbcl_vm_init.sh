#!/bin/bash

# Establish environment variables
NEXUS_USERNAME=$(cat /opt/config/nexus_username.txt)
NEXUS_PASSWD=$(cat /opt/config/nexus_password.txt)
NEXUS_DOCKER_REPO=$(cat /opt/config/nexus_docker_repo.txt)
DOCKER_IMAGE_VERSION=$(cat /opt/config/uui_docker.txt)

# Refresh images
docker login -u $NEXUS_USERNAME -p $NEXUS_PASSWD $NEXUS_DOCKER_REPO
docker pull $NEXUS_DOCKER_REPO/onap/dmaap/buscontroller:$DOCKER_IMAGE_VERSION
docker tag $NEXUS_DOCKER_REPO/onap/dmaap/buscontroller:$DOCKER_IMAGE_VERSION onap/dmaap/buscontroller:latest

docker rm -f dmaap-buscontroller

TMP_CFG=/tmp/docker-dmaap-buscontroller.conf
cat >> $TMP_CFG <<!EOF
DMAAPBC_INT_HTTPS_PORT=0
DMAAPBC_PG_ENABLED=false
DMAAPBC_INSTANCE_NAME=ONAP-CSIT
DMAAPBC_AAF_URL=https://aaf.onap.org:9999/proxy/
DMAAPBC_MR_CNAME=dmaap.onap.org
DMAAPBC_DRPROV_FQDN=dr.descoped.org
!EOF

# Insert docker run instructions here
docker run -i -t -d --name dmaap-buscontroller -p 8080:8080 -p 8443:8443 -v $TMP_CFG:/opt/app/config/conf $NEXUS_DOCKER_REPO/onap/dmaap/buscontroller:$DOCKER_IMAGE_VERSION

DBCL_FQDN=localhost
DRPS_FQDN=localhost
MR_FQDN=dmaap.onap.org
# INITIALIZE: dmaap object
JSON=/tmp/$$.dmaap
cat << EOF > $JSON
{
"version": "1",
"topicNsRoot": "org.onap.dmaap",
"drProvUrl": "http://${DRPS_FQDN}:8080",
"dmaapName": "onapStable",
"bridgeAdminTopic": "MM_AGENT_PROV"

}
EOF

curl -v -X POST -d @${JSON} -H "Content-Type: application/json" http://$1:8080/webapi/dmaap 



# INITIALIZE: dcaeLocation object
JSON=/tmp/$$.loc
cat << EOF > $JSON
{
"dcaeLocationName": "csit-sanfrancisco",
"dcaeLayer": "central-cloud",
"clli": "STABLE2345",
"zone": "zoneA"

}
EOF

curl -v -X POST -d @${JSON} -H "Content-Type: application/json" http://$1:8080/webapi/dcaeLocations 


# INITIALIZE: MR object in 1 site
JSON=/tmp/$$.mrc
cat << EOF > $JSON
{
"dcaeLocationName": "csit-sanfrancisco",
"fqdn": "${MR_FQDN}",
"hosts" : [ "10.10.10.1", "10.10.10.2", "10.10.10.3" ],
"protocol" : "https",
"port": "3094"

}
EOF

curl -v -X POST -d @${JSON} -H "Content-Type: application/json" http://$1:8080/webapi/mr_clusters
