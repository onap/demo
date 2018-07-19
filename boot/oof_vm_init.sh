#!/bin/bash

NEXUS_USERNAME=$(cat /opt/config/nexus_username.txt)
NEXUS_PASSWD=$(cat /opt/config/nexus_password.txt)
NEXUS_DOCKER_REPO=$(cat /opt/config/nexus_docker_repo.txt)
HAS_IMAGE_VERSION=$(cat /opt/config/has_docker_version.txt)
OSDF_IMAGE_VERSION=$(cat /opt/config/osdf_docker_version.txt)
MUSIC_URL=music.api.simpledemo.onap.org

cd /opt/optf-has
git pull

COND_CONF=/opt/optf-has/conductor.conf
LOG_CONF=/opt/optf-has/log.conf

#Certification file for OOF-HAS
BUNDLE=/opt/optf-has/AAF_RootCA.cer
AAI_cert=/usr/local/bin/AAF_RootCA.cer

OSDF_IMG=${NEXUS_DOCKER_REPO}/onap/optf-osdf:${OSDF_IMAGE_VERSION}
HAS_IMG=${NEXUS_DOCKER_REPO}/onap/optf-has:${HAS_IMAGE_VERSION}

# pull images from repo
docker login -u $NEXUS_USERNAME -p $NEXUS_PASSWD $NEXUS_DOCKER_REPO
docker pull ${OSDF_IMG}
docker pull ${HAS_IMG}

# Run OOF-HAS
# Set A&AI and MUSIC url inside OOF-HAS conductor.conf
sed -i "138 s%.*%server_url = https://aai.api.simpledemo.onap.org:8443/aai%" $COND_CONF
sed -i "141 s%.*%server_url_version = v13%" $COND_CONF
sed -i "257 s%.*%server_url = http://$MUSIC_URL:8080/MUSIC/rest/v2%" $COND_CONF
sed -i "284 s%.*%replication_factor = 1%" $COND_CONF
sed -i "359 s%.*%server_url = http://msb.api.simpledemo.onap.org/api/multicloud%" $COND_CONF

# Set A&AI authentication file locations inside OOF-HAS conductor.conf
sed -i "153 s%.*%certificate_authority_bundle_file = $AAI_cert%" $COND_CONF


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

# Credentials for SO
soUsername: ""   # SO username for call back.
soPassword: ""   # SO password for call back.

# Credentials for Conductor
conductorUrl: http://$HAS_HOST:8091/v1/plans/
conductorUsername: admin1
conductorPassword: plan.15
conductorPingWaitTime: 60  # seconds to wait before calling the conductor retry URL
conductorMaxRetries: 30  # if we don't get something in 30 minutes, give up

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

# Credentials for the OOF CM scheduling service - Generic
osdfCMSchedulerUsername: test1
osdfCMSchedulerPassword: testpwd1

NEWFILE

docker run -d --name osdf -v $OSDF_CONFIG:/opt/app/config/osdf_config.yaml -p 8698:8699 ${OSDF_IMG}
