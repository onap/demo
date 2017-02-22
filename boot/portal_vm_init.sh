#!/bin/bash

NEXUS_USERNAME=$(cat /opt/config/nexus_username.txt)
NEXUS_PASSWD=$(cat /opt/config/nexus_password.txt)
NEXUS_DOCKER_REPO=$(cat /opt/config/nexus_docker_repo.txt)

cd /opt/portal
git pull
cd /opt

chmod +x portal/deliveries/new_start.sh
chmod +x portal/deliveries/new_stop.sh
chmod +x portal/deliveries/dbstart.sh
unzip -o portal/deliveries/etc.zip -d /PROJECT/OpenSource/UbuntuEP/

docker login -u $NEXUS_USERNAME -p $NEXUS_PASSWD $NEXUS_DOCKER_REPO
docker pull $NEXUS_DOCKER_REPO/openecomp/portaldb:1.0.0
docker pull $NEXUS_DOCKER_REPO/openecomp/portalapps:1.0.0

docker create --name data_vol_portal -v /var/lib/mysql mariadb

docker tag $NEXUS_DOCKER_REPO/openecomp/portaldb:1.0.0 ecompdb:portal
docker tag $NEXUS_DOCKER_REPO/openecomp/portalapps:1.0.0 ep:1610-1

docker rm -f ecompdb_portal
docker rm -f 1610-1

cd portal/deliveries
./dbstart.sh
./new_start.sh

sleep 60

if [ ! -e /opt/config/boot.txt ]
then
  IP_ADDRESS=$(ifconfig eth0 | grep "inet addr" | tr -s ' ' | cut -d' ' -f3 | cut -d':' -f2)
  mysql -u root -p'Aa123456' -h $IP_ADDRESS < users.sql
  echo "yes" > /opt/config/boot.txt
fi