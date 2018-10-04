#!/bin/bash


NEXUS_USERNAME=$(cat /opt/config/nexus_username.txt)
NEXUS_PASSWD=$(cat /opt/config/nexus_password.txt)
NEXUS_DOCKER_REPO=$(cat /opt/config/nexus_docker_repo.txt)
CASS_MUSIC_IMAGE_VERSION=$(cat /opt/config/cass_version.txt)
MUSIC_IMAGE_VERSION=$(cat /opt/config/music_version.txt)
HAS_IMAGE_VERSION=$(cat /opt/config/has_docker_version.txt)
OSDF_IMAGE_VERSION=$(cat /opt/config/osdf_docker_version.txt)

cd /opt/optf-has
git pull

COND_CONF=/opt/optf-has/conductor.conf
LOG_CONF=/opt/optf-has/log.conf

# Certification file for OOF-HAS
AAI_cert=/usr/local/bin/AAF_RootCA.cer
BUNDLE=/opt/optf-has/AAF_RootCA.cer

OSDF_IMG=${NEXUS_DOCKER_REPO}/onap/optf-osdf:${OSDF_IMAGE_VERSION}
HAS_IMG=${NEXUS_DOCKER_REPO}/onap/optf-has:${HAS_IMAGE_VERSION}

# pull OOF images from repo
docker login -u $NEXUS_USERNAME -p $NEXUS_PASSWD $NEXUS_DOCKER_REPO
docker pull ${OSDF_IMG}
docker pull ${HAS_IMG}

# Install MUSIC
# MUSIC parameters
CASS_IMG=${NEXUS_DOCKER_REPO}/onap/music/cassandra_music:$CASS_MUSIC_IMAGE_VERSION
MUSIC_IMG=${NEXUS_DOCKER_REPO}/onap/music/music:$MUSIC_IMAGE_VERSION
TOMCAT_IMG=library/tomcat:8.5
ZK_IMG=library/zookeeper:3.4
WORK_DIR=/opt/optf-has
CASS_USERNAME=cassandra1
CASS_PASSWORD=cassandra1

# pull MUSIC images
docker pull ${ZK_IMG}
docker pull ${TOMCAT_IMG}
docker pull ${CASS_IMG}
docker pull ${MUSIC_IMG}

# create directory for music properties and logs
mkdir -p /opt/optf-has/music/properties
mkdir -p /opt/optf-has/music/logs

# add music.properties file
cat > /opt/optf-has/music/properties/music.properties<<NEWFILE
my.id=0
all.ids=0
my.public.ip=localhost
all.public.ips=localhost

#######################################

# Optional current values are defaults

#######################################
zookeeper.host=music-zk
cassandra.host=music-db
#music.ip=localhost
#debug=true
#music.rest.ip=localhost
#lock.lease.period=6000
cassandra.user=cassandra1
cassandra.password=cassandra1

# AAF Endpoint if using AAF
aaf.endpoint.url=https://aaf.api.simpledemo.onap.org
NEWFILE

# Create Volume for mapping war file and tomcat
docker volume create music-vol

# Create a network for all the containers to run in.
docker network create music-net

# Start Cassandra
docker run -d --rm --name music-db --network music-net -p "7000:7000" -p "7001:7001" -p "7199:7199" -p "9042:9042" -p "9160:9160" -e CASSUSER=${CASS_USERNAME} -e CASSPASS=${CASS_PASSWORD} ${CASS_IMG}

# Start Music war
docker run -d --rm --name music-war -v music-vol:/app ${MUSIC_IMG}

# Start Zookeeper
docker run -d --rm --name music-zk --network music-net -p "2181:2181" -p "2888:2888" -p "3888:3888" ${ZK_IMG}

# Delay for Cassandra
sleep 20;

# Start Up tomcat - Needs to have properties,logs dir and war file volume mapped.
docker run -d --rm --name music-tomcat --network music-net -p "8080:8080" -v music-vol:/usr/local/tomcat/webapps -v ${WORK_DIR}/music/properties:/opt/app/music/etc:ro -v ${WORK_DIR}/music/logs:/opt/app/music/logs ${TOMCAT_IMG}

# Connect tomcat to host bridge network so that its port can be seen.
docker network connect bridge music-tomcat;
sleep 6;
echo "Running onboarding curl command"
curl -X POST \
  http://localhost:8080/MUSIC/rest/v2/admin/onboardAppWithMusic \
  -H 'Cache-Control: no-cache' \
  -H 'Content-Type: application/json' \
  -H 'Postman-Token: 7d2839f4-b032-487a-8998-4d1b27a932d7' \
  -d '{
"appname": "conductor",
"userId" : "conductor",
"isAAF"  : false,
"password" : "c0nduct0r"
}
'
echo "Onboarding curl complete"

# Get MUSIC url
MUSIC_URL=$(docker inspect --format '{{ .NetworkSettings.Networks.bridge.IPAddress}}' music-tomcat)

# Run OOF-HAS
# Set A&AI and MUSIC url inside OOF-HAS conductor.conf
sed -i "147 s%.*%aaf_url = http://aaf.api.simpledemo.onap.org:8100/authz/perms/user/%" $COND_CONF
sed -i "167 s%.*%aaf_sms_url = http://aaf-sms.api.simpledemo.onap.org:10443%" $COND_CONF
sed -i "202 s%.*%server_url = https://aai.api.simpledemo.onap.org:8443/aai%" $COND_CONF
sed -i "211 s%.*%server_url_version = v14%" $COND_CONF
sed -i "382 s%.*%server_url = http://msb.api.simpledemo.onap.org/api/multicloud%" $COND_CONF
sed -i "401 s%.*%server_url = http://$MUSIC_URL:8080/MUSIC/rest/v2%" $COND_CONF
sed -i "433 s%.*%replication_factor = 1%" $COND_CONF
sed -i "536 s%.*%server_url = https://sdnc.api.simpledemo.onap.org:8282/restconf/%" $COND_CONF

# Set A&AI and SMS authentication file locations inside OOF-HAS conductor.conf
sed -i "174 s%.*%aaf_ca_certs = $AAI_cert%" $COND_CONF
sed -i "225 s%.*%certificate_authority_bundle_file = $AAI_cert%" $COND_CONF


echo "Values to data component"
echo $BUNDLE

# run optf-has
docker run -d --name controller -v $COND_CONF:/usr/local/bin/conductor.conf -v $LOG_CONF:/usr/local/bin/log.conf ${HAS_IMG} python /usr/local/bin/conductor-controller --config-file=/usr/local/bin/conductor.conf

docker run -d --name api -p "8091:8091" -v $COND_CONF:/usr/local/bin/conductor.conf -v $LOG_CONF:/usr/local/bin/log.conf ${HAS_IMG} python /usr/local/bin/conductor-api --port=8091 -- --config-file=/usr/local/bin/conductor.conf

docker run -d --name solver -v $COND_CONF:/usr/local/bin/conductor.conf -v $LOG_CONF:/usr/local/bin/log.conf ${HAS_IMG} python /usr/local/bin/conductor-solver --config-file=/usr/local/bin/conductor.conf

docker run -d --name reservation -v $COND_CONF:/usr/local/bin/conductor.conf -v $LOG_CONF:/usr/local/bin/log.conf ${HAS_IMG} python /usr/local/bin/conductor-reservation --config-file=/usr/local/bin/conductor.conf

docker run -d --name data -v $COND_CONF:/usr/local/bin/conductor.conf -v $LOG_CONF:/usr/local/bin/log.conf -v $BUNDLE:/usr/local/bin/AAF_RootCA.cer ${HAS_IMG} python /usr/local/bin/conductor-data --config-file=/usr/local/bin/conductor.conf

sleep 10

echo "Inserting healthcheck plan"

curl -X POST \
  http://$MUSIC_URL:8080/MUSIC/rest/v2/keyspaces/conductor/tables/plans/rows/ \
  -H 'Cache-Control: no-cache' \
  -H 'Content-Type: application/json' \
  -H 'Postman-Token: 502781e8-d588-475d-b181-c2e26625ac95' \
  -H 'X-minorVersion: 3' \
  -H 'X-patchVersion: 0' \
  -H 'ns: conductor' \
  -H 'password: c0nduct0r' \
  -H 'userId: conductor' \
  -d '{
    "consistencyInfo": {
        "type": "eventual"
    },
    "values": {
        "id" : "healthcheck",
        "created": 1479482603641,
        "message": "",
        "name": "foo",
        "recommend_max": 1,
        "solution": "{\"healthcheck\": \" healthcheck\"}",
        "status": "done",
        "template": "{\"healthcheck\": \"healthcheck\"}",
        "timeout": 3600,
        "translation": "{\"healthcheck\": \" healthcheck\"}",
        "updated": 1484324150629
    }
}
'

echo "Healthcheck plan inserted"


#run optf-osdf
OSDF_CONFIG=/opt/optf-osdf/config/osdf_config.yaml
HAS_HOST=$(docker inspect --format '{{ .NetworkSettings.Networks.bridge.IPAddress}}' api)

mkdir -p /opt/optf-osdf/config

cat > $OSDF_CONFIG<<NEWFILE

placementVersioningEnabled: False

# Placement API latest version numbers to be set in HTTP header
placementMajorVersion: "1"
placementMinorVersion: "0"
placementPatchVersion: "0"

# Placement API default version numbers to be set in HTTP header
placementDefaultMajorVersion: "1"
placementDefaultMinorVersion: "0"
placementDefaultPatchVersion: "0"

# Credentials for SO
soUsername: ""   # SO username for call back.
soPassword: ""   # SO password for call back.

# Credentials for Conductor
conductorUrl: http://$HAS_HOST:8091/v1/plans/
conductorUsername: admin1
conductorPassword: plan.15
conductorPingWaitTime: 60  # seconds to wait before calling the conductor retry URL
conductorMaxRetries: 30  # if we don't get something in 30 minutes, give up
# versions to be set in HTTP header
conductorMinorVersion: 0

# Policy Platform -- requires ClientAuth, Authorization, and Environment
policyPlatformUrl: http://policy.api.simpledemo.onap.org:8081/pdp/api/getConfig # Policy Dev platform URL
policyPlatformEnv: TEST  # Environment for policy platform
policyPlatformUsername: testpdp   # Policy platform username.
policyPlatformPassword: alpha123   # Policy platform password.
policyClientUsername: python   # For use with ClientAuth
policyClientPassword: test   # For use with ClientAuth

# Credentials for DMaaP
messageReaderHosts: NA
messageReaderTopic: NA
messageReaderAafUserId: NA
messageReaderAafPassword: NA

# Credentials for SDC
sdcUrl: NA
sdcUsername: NA
sdcPassword: NA
sdcONAPInstanceID: NA

# Credentials for the OOF placement service - Generic
osdfPlacementUsername: test
osdfPlacementPassword: testpwd

# Credentials for the OOF placement service - SO
osdfPlacementSOUsername: so_test
osdfPlacementSOPassword: so_testpwd

# Credentials for the OOF placement service - VFC
osdfPlacementVFCUsername: vfc_test
osdfPlacementVFCPassword: vfc_testpwd

# Credentials for the OOF CM scheduling service - Generic
osdfCMSchedulerUsername: test1
osdfCMSchedulerPassword: testpwd1

is_aaf_enabled: False
aaf_cache_expiry_hrs: 3
aaf_url: https://aaf.api.simpledemo.onap.org:8100
aaf_user_roles:
    - /api/oof/v1/placement:org.onap.osdf.access|*|read ALL

# Secret Management Service from AAF
aaf_sms_url: https://aaf-sms.api.simpledemo.onap.org:10443
aaf_sms_timeout: 30
secret_domain: osdf
aaf_ca_certs: ssl_certs/aaf_root_ca.cer

# config db api
configDbUrl: http://config.db.url:8080
configDbUserName: osdf
configDbPassword: passwd
configDbGetCellListUrl: 'SDNCConfigDBAPI/getCellList'
configDbGetNbrListUrl: 'SDNCConfigDBAPI/getNbrList'

# Credentials for PCIHandler
pciHMSUsername: ""   # pcihandler username for call back.
pciHMSPassword: ""   # pcihandler password for call back.

# Credentials for the OOF PCI Opt service
osdfPCIOptUsername: pci_test
osdfPCIOptPassword: pci_testpwd


NEWFILE

docker run -d --name osdf -v $OSDF_CONFIG:/opt/app/config/osdf_config.yaml -p 8698:8699 ${OSDF_IMG}
