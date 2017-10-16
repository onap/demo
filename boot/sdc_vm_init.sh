#!/bin/bash

NEXUS_USERNAME=$(cat /opt/config/nexus_username.txt)
NEXUS_PASSWD=$(cat /opt/config/nexus_password.txt)
NEXUS_DOCKER_REPO=$(cat /opt/config/nexus_docker_repo.txt)
NEXUS_DOCKER_PORT=$(echo $NEXUS_DOCKER_REPO | cut -d':' -f2)
ENV_NAME=$(cat /opt/config/env_name.txt)
MR_IP_ADDR=$(cat /opt/config/mr_ip_addr.txt)
RELEASE=$(cat /opt/config/docker_version.txt)

cd /opt/sdc
git pull

cd /opt
cp sdc/sdc-os-chef/environments/Template.json /data/environments
cp sdc/sdc-os-chef/scripts/docker_run.sh /data/scripts
cp sdc/sdc-os-chef/scripts/docker_health.sh /data/scripts
chmod +x /data/scripts/docker_run.sh
chmod +x /data/scripts/docker_health.sh

if [ -e /opt/config/public_ip.txt ]
then
  IP_ADDRESS=$(cat /opt/config/public_ip.txt)
else
  IP_ADDRESS=$(ifconfig eth0 | grep "inet addr" | tr -s ' ' | cut -d' ' -f3 | cut -d':' -f2)
fi

cat /data/environments/Template.json | sed "s/yyy/"$IP_ADDRESS"/g" > /data/environments/$ENV_NAME.json
sed -i "s/xxx/"$ENV_NAME"/g" /data/environments/$ENV_NAME.json
sed -i "s/\"ueb_url_list\":.*/\"ueb_url_list\": \""$MR_IP_ADDR","$MR_IP_ADDR"\",/g" /data/environments/$ENV_NAME.json
sed -i "s/\"fqdn\":.*/\"fqdn\": [\""$MR_IP_ADDR"\", \""$MR_IP_ADDR"\"]/g" /data/environments/$ENV_NAME.json

docker login -u $NEXUS_USERNAME -p $NEXUS_PASSWD $NEXUS_DOCKER_REPO
bash /data/scripts/docker_run.sh -e $ENV_NAME -r $RELEASE -p $NEXUS_DOCKER_PORT

docker run -d -p 8088:8080 nexus3.onap.org:10001/onap/sdc/sdc-workflow-designer