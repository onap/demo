#!/bin/bash
# Starts docker containers for ONAP Portal

# be verbose
set -x

# Refresh source area with start scripts
cd /opt/portal
git pull
cd /opt

# Establish environment variables
NEXUS_USERNAME=$(cat /opt/config/nexus_username.txt)
NEXUS_PASSWD=$(cat /opt/config/nexus_password.txt)
NEXUS_DOCKER_REPO=$(cat /opt/config/nexus_docker_repo.txt)
DOCKER_IMAGE_VERSION=$(cat /opt/config/docker_version.txt)

# Get container, image and tag names used below
source portal/deliveries/os_settings.sh

# Unpack property files
unzip -o portal/deliveries/etc.zip -d /PROJECT/OpenSource/UbuntuEP/
# Copy (ecomp-portal-BE-os) logback.xml for volume mapping
mv portal/deliveries/os_logback.xml /PROJECT/OpenSource/UbuntuEP/etc/ECOMPPORTALAPP/

# Refresh images
docker login -u $NEXUS_USERNAME -p $NEXUS_PASSWD $NEXUS_DOCKER_REPO
docker pull $NEXUS_DOCKER_REPO/openecomp/${DB_TAG_NAME}:$DOCKER_IMAGE_VERSION
docker pull $NEXUS_DOCKER_REPO/openecomp/${EP_TAG_NAME}:$DOCKER_IMAGE_VERSION
docker pull $NEXUS_DOCKER_REPO/openecomp/${WMS_TAG_NAME}:$DOCKER_IMAGE_VERSION

# Remove lingering containers; order matters.
docker rm -f $DB_CONT_NAME
docker rm -f $DB_VOL_NAME
docker rm -f $EP_CONT_NAME
docker rm -f $WMS_CONT_NAME

docker create --name $DB_VOL_NAME -v /var/lib/mysql mariadb
docker tag $NEXUS_DOCKER_REPO/openecomp/${DB_TAG_NAME}:$DOCKER_IMAGE_VERSION $DB_IMG_NAME
docker tag $NEXUS_DOCKER_REPO/openecomp/${EP_TAG_NAME}:$DOCKER_IMAGE_VERSION $EP_IMG_NAME
# WMS image has no version in the registry
docker tag $NEXUS_DOCKER_REPO/openecomp/${WMS_TAG_NAME}:$DOCKER_IMAGE_VERSION ${WMS_IMG_NAME}:latest

# Recreate the named containers
cd portal/deliveries
echo "Starting database"
./dbstart.sh
echo "Delaying for database"
sleep 10
echo "Starting apps"
./new_start.sh
echo "Starting widget-ms"
./widget_ms_start.sh

sleep 180

if [ ! -e /opt/config/boot.txt ]
then
	if [ -e /opt/config/public_ip.txt ]
	then
		IP_ADDRESS=$(cat /opt/config/public_ip.txt)
	else
		IP_ADDRESS=$(ifconfig eth0 | grep "inet addr" | tr -s ' ' | cut -d' ' -f3 | cut -d':' -f2)
	fi
	# Wait until MySQL is running...
	while [[ $(netstat -vulntp |grep -i mysql | awk '{print $4}') != ":::3306" ]]
	do
		sleep 1
	done
	# no longer necessary; done at docker build time
	# mysql -u root -p'Aa123456' -h $IP_ADDRESS < /opt/portal/deliveries/Apps_Users_OnBoarding_Script.sql
	echo "yes" > /opt/config/boot.txt
fi
