#!/bin/bash

# Establish environment variables
NEXUS_USERNAME=$(cat /opt/config/nexus_username.txt)
NEXUS_PASSWD=$(cat /opt/config/nexus_password.txt)
NEXUS_DOCKER_REPO=$(cat /opt/config/nexus_docker_repo.txt)
DOCKER_IMAGE_VERSION=$(cat /opt/config/docker_version.txt)

# Refresh images
docker login -u $NEXUS_USERNAME -p $NEXUS_PASSWD $NEXUS_DOCKER_REPO
docker pull $NEXUS_DOCKER_REPO/onap/dmaap/buscontroller:$DOCKER_IMAGE_VERSION
docker tag $NEXUS_DOCKER_REPO/onap/dmaap/buscontroller:$DOCKER_IMAGE_VERSION onap/dmaap/buscontroller:latest

docker rm -f dmaap-buscontroller

TMP_CFG=/tmp/docker-dmaap-buscontroller.conf
cat >> $TMP_CFG <<!EOF
DMAAPBC_INT_HTTPS_PORT=8443
DMAAPBC_PG_ENABLED=false
DMAAPBC_INSTANCE_NAME=ONAP-CSIT
DMAAPBC_AAF_URL=https://aaf.api.simpledemo.onap.org:8095/proxy/
DMAAPBC_MR_CNAME=mr.api.simpledemo.onap.org
DMAAPBC_DRPROV_FQDN=drprov.simpledemo.onap.org
DMAAPBC_KSTOREPASS="Perish the thought"
DMAAPBC_PVTKEYPASS="Perish the thought"
!EOF

# Insert docker run instructions here
docker run -i -t -d --name dmaap-buscontroller -p 8080:8080 -p 8443:8443 -v $TMP_CFG:/opt/app/config/conf $NEXUS_DOCKER_REPO/onap/dmaap/buscontroller:$DOCKER_IMAGE_VERSION

sleep 15

DBCL_FQDN=dbc.api.simpledemo.onap.org
DRPS_FQDN=drprov.simpledemo.onap.org
MR_FQDN=mr.api.simpledemo.onap.org
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

curl -v -X POST -d @${JSON} -H "Content-Type: application/json" http://${DBCL_FQDN}:8080/webapi/dmaap 



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

curl -v -X POST -d @${JSON} -H "Content-Type: application/json" http://${DBCL_FQDN}:8080/webapi/dcaeLocations 


# INITIALIZE: MR object in 1 site
# Note: the values in the hosts[] are fictitious, and anticipate a 
# future MR cluster deployment.
JSON=/tmp/$$.mrc
cat << EOF > $JSON
{
"dcaeLocationName": "stable-sanfrancisco",
"fqdn": "${MR_FQDN}",
"hosts" : [ 
	"mrhost1.simpledemo.onap.org", 
	"mrhost2.simpledemo.onap.org", 
	"mrhost3.simpledemo.onap.org" 
	],
"protocol" : "https",
"port": "3094"

}
EOF

curl -v -X POST -d @${JSON} -H "Content-Type: application/json" http://${DBCL_FQDN}:8080/webapi/mr_clusters
