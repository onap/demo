#!/bin/bash

NEXUS_USERNAME=$(cat /opt/config/nexus_username.txt)
NEXUS_PASSWD=$(cat /opt/config/nexus_password.txt)
NEXUS_DOCKER_REPO=$(cat /opt/config/nexus_docker_repo.txt)
DOCKER_IMAGE_VERSION=$(cat /opt/config/docker_version.txt)

cd /opt/vid
git pull
cd /opt

docker pull mariadb:10
docker login -u $NEXUS_USERNAME -p $NEXUS_PASSWD $NEXUS_DOCKER_REPO
docker pull $NEXUS_DOCKER_REPO/openecomp/vid:$DOCKER_IMAGE_VERSION

docker rm -f vid-mariadb
docker rm -f vid-server

docker run --name vid-mariadb -e MYSQL_DATABASE=vid_openecomp_epsdk -e MYSQL_USER=vidadmin -e MYSQL_PASSWORD=Kp8bJ4SXszM0WXlhak3eHlcse2gAw84vaoGGmJvUy2U -e MYSQL_ROOT_PASSWORD=LF+tp_1WqgSY -v /opt/vid/lf_config/vid-my.cnf:/etc/mysql/my.cnf -v /opt/vid/lf_config/vid-pre-init.sql:/docker-entrypoint-initdb.d/vid-pre-init.sql -v /var/lib/mysql -d mariadb:10

docker run -e VID_MYSQL_DBNAME=vid_openecomp_epsdk -e VID_MYSQL_PASS=Kp8bJ4SXszM0WXlhak3eHlcse2gAw84vaoGGmJvUy2U --name vid-server -p 8080:8080 --link vid-mariadb:vid-mariadb-docker-instance -d $NEXUS_DOCKER_REPO/openecomp/vid:$DOCKER_IMAGE_VERSION
