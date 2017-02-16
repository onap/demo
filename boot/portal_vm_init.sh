#!/bin/bash

NEXUS_USERNAME=$(cat /opt/config/nexus_username.txt)
NEXUS_PASSWD=$(cat /opt/config/nexus_password.txt)
NEXUS_DOCKER_REPO=$(cat /opt/config/nexus_docker_repo.txt)

docker login -u $NEXUS_USERNAME -p $NEXUS_PASSWD $NEXUS_DOCKER_REPO
docker pull $NEXUS_DOCKER_REPO/ecomp/portaldb:1610.3
docker pull $NEXUS_DOCKER_REPO/ecomp/portalapps:1610.3

docker create --name data_vol_portal -v /var/lib/mysql mariadb

docker tag 6ce6ea8c6e52 ecompdb:portal
docker tag 925a8a953d4c ep:1610-1

docker rm -f ecompdb_portal
docker rm -f 1610-1

./dbstart.sh
./new_start.sh

sleep 60

if [ ! -e /opt/config/boot.txt ]
then
  IP_ADDRESS=$(ifconfig eth0 | grep "inet addr" | tr -s ' ' | cut -d' ' -f3 | cut -d':' -f2)
  mysql -u root -p'Aa123456' -h $IP_ADDRESS < users.sql
  echo "yes" > /opt/config/boot.txt
fi