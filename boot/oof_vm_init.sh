#!/bin/bash

CERT=/opt/optf-has/aai_cert.cer
KEY=/opt/optf-has/aai_key.key
BUNDLE=/opt/optf-has/aai_bundle.pem

NEXUS_USERNAME=$(cat /opt/config/nexus_username.txt)
NEXUS_PASSWD=$(cat /opt/config/nexus_password.txt)
NEXUS_DOCKER_REPO=$(cat /opt/config/nexus_docker_repo.txt)
DOCKER_IMAGE_VERSION=$(cat /opt/config/docker_version.txt)

cd /opt/optf-has
git pull

COND_CONF=/opt/optf-has/conductor.conf
LOG_CONF=/opt/optf-has/log.conf

#!!! THE FOLLOWING LINE IS A PLACEHOLDER !!!
AAI_cert=/usr/local/bin/aai_cert.cer

IMAGE_NAME="$NEXUS_DOCKER_REPO/onap/optf-has"

# MUSIC parameters
CASS_IMG=${NEXUS_DOCKER_REPO}/onap/music/cassandra_music:latest
MUSIC_IMG=${NEXUS_DOCKER_REPO}/onap/music/music:latest
TOMCAT_IMG=library/tomcat:8.5
ZK_IMG=library/zookeeper:3.4
WORK_DIR=/opt/optf-has
CASS_USERNAME=cassandra1
CASS_PASSWORD=cassandra1

# pull images from repo
docker login -u $NEXUS_USERNAME -p $NEXUS_PASSWD $NEXUS_DOCKER_REPO
docker pull $NEXUS_DOCKER_REPO/onap/optf-osdf:$DOCKER_IMAGE_VERSION
docker pull ${ZK_IMG}
docker pull ${TOMCAT_IMG}
docker pull ${CASS_IMG}
docker pull ${MUSIC_IMG}
docker pull $NEXUS_DOCKER_REPO/onap/optf-has:$DOCKER_IMAGE_VERSION


#run optf-osdf

OSDF_IMAGE_NAME="$NEXUS_DOCKER_REPO/onap/optf-osdf"
OSDF_CONFIG=/opt/optf-osdf/config/osdf_config.yaml

mkdir -p /opt/optf-osdf/config

cat > $OSDF_CONFIG<<NEWFILE

# Credentials for SO
soUsername: ""   # SO username for call back.
soPassword: ""   # SO password for call back.

# Credentials for Conductor
conductorUrl: https://localhost:8091/v1/plans/
conductorUsername: admin1
conductorPassword: plan.15
conductorPingWaitTime: 60  # seconds to wait before calling the conductor retry URL
conductorMaxRetries: 30  # if we don't get something in 30 minutes, give up

# Policy Platform -- requires ClientAuth, Authorization, and Environment
policyPlatformUrl: http://onap-pdp:8081/pdp/api/getConfig # Policy Dev platform URL
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

docker run -d --name osdf -v $OSDF_CONFIG:/optf/config/osdf_config.yaml -p 8698:8699 ${OSDF_IMAGE_NAME}:latest

# install MUSIC
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

# Get MUSIC url
#MUSIC_URL=$(docker inspect --format '{{ .NetworkSettings.Networks.bridge.IPAddress}}' music-tomcat)
MUSIC_URL=localhost

# Set A&AI and MUSIC url inside OOF-HAS conductor.conf
sed -i "138 s%.*%server_url = https://aai.api.simpledemo.onap.org:8443/aai%" $COND_CONF
sed -i "141 s%.*%server_url_version = v13%" $COND_CONF
#sed -i "250 s%.*%server_url = $MUSIC_URL:8080/MUSIC/rest/v2%" $COND_CONF

# Set A&AI authentication file locations inside OOF-HAS conductor.conf
sed -i "153 s%.*%certificate_authority_bundle_file = $AAI_cert%" $COND_CONF


echo "Values to data component"
echo $CERT
echo $KEY
echo $BUNDLE

# run optf-has
docker run -d --name controller -v $COND_CONF:/usr/local/bin/conductor.conf -v $LOG_CONF:/usr/local/bin/log.conf ${IMAGE_NAME}:latest python /usr/local/bin/conductor-controller --config-file=/usr/local/bin/conductor.conf

docker run -d --name api -p "8091:8091" -v $COND_CONF:/usr/local/bin/conductor.conf -v $LOG_CONF:/usr/local/bin/log.conf ${IMAGE_NAME}:latest python /usr/local/bin/conductor-api --port=8091 -- --config-file=/usr/local/bin/conductor.conf

docker run -d --name solver -v $COND_CONF:/usr/local/bin/conductor.conf -v $LOG_CONF:/usr/local/bin/log.conf ${IMAGE_NAME}:latest python /usr/local/bin/conductor-solver --config-file=/usr/local/bin/conductor.conf

docker run -d --name reservation -v $COND_CONF:/usr/local/bin/conductor.conf -v $LOG_CONF:/usr/local/bin/log.conf ${IMAGE_NAME}:latest python /usr/local/bin/conductor-reservation --config-file=/usr/local/bin/conductor.conf

docker run -d --name data -v $COND_CONF:/usr/local/bin/conductor.conf -v $LOG_CONF:/usr/local/bin/log.conf -v $CERT:/usr/local/bin/aai_cert.cer -v $KEY:/usr/local/bin/aai_key.key -v $BUNDLE:/usr/local/bin/bundle.pem ${IMAGE_NAME}:latest python /usr/local/bin/conductor-data --config-file=/usr/local/bin/conductor.conf


