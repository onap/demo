#!/bin/bash

NEXUS_USERNAME=$(cat /opt/config/nexus_username.txt)
NEXUS_PASSWD=$(cat /opt/config/nexus_password.txt)
NEXUS_DOCKER_REPO=$(cat /opt/config/nexus_docker_repo.txt)
DOCKER_IMAGE_VERSION=$(cat /opt/config/docker_version.txt)

cd /opt/portal
git pull
cd /opt

chmod +x portal/deliveries/new_start.sh
chmod +x portal/deliveries/new_stop.sh
chmod +x portal/deliveries/dbstart.sh

unzip -o portal/deliveries/etc.zip -d /PROJECT/OpenSource/UbuntuEP/

docker login -u $NEXUS_USERNAME -p $NEXUS_PASSWD $NEXUS_DOCKER_REPO

docker pull $NEXUS_DOCKER_REPO/openecomp/portaldb:$DOCKER_IMAGE_VERSION
docker pull $NEXUS_DOCKER_REPO/openecomp/portalapps:$DOCKER_IMAGE_VERSION
docker pull $NEXUS_DOCKER_REPO/openecomp/portalwms:$DOCKER_IMAGE_VERSION

docker create --name data_vol_portal -v /var/lib/mysql mariadb

docker tag $NEXUS_DOCKER_REPO/openecomp/portaldb:$DOCKER_IMAGE_VERSION ecompdb:portal
docker tag $NEXUS_DOCKER_REPO/openecomp/portalapps:$DOCKER_IMAGE_VERSION ep:1610-1
docker tag $NEXUS_DOCKER_REPO/openecomp/portalwms:$DOCKER_IMAGE_VERSION widget-ms:latest

docker rm -f ecompdb_portal
docker rm -f 1610-1
docker rm -f widget-ms:latest

cd portal/deliveries
./dbstart.sh
./new_start.sh
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
	mysql -u root -p'Aa123456' -h $IP_ADDRESS < /opt/portal/deliveries/Apps_Users_OnBoarding_Script.sql
	echo "yes" > /opt/config/boot.txt
fi
