#!/bin/bash

NEXUS_USERNAME=$(cat /opt/config/nexus_username.txt)
NEXUS_PASSWD=$(cat /opt/config/nexus_password.txt)
NEXUS_DOCKER_REPO=$(cat /opt/config/nexus_docker_repo.txt)
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

IP_ADDRESS=$(cat /opt/config/private_ip.txt)

cat /data/environments/Template.json | sed "s/yyy/"$IP_ADDRESS"/g" > /data/environments/$ENV_NAME.json
sed -i "s/xxx/"$ENV_NAME"/g" /data/environments/$ENV_NAME.json
sed -i "s/\"ueb_url_list\":.*/\"ueb_url_list\": \""$MR_IP_ADDR","$MR_IP_ADDR"\",/g" /data/environments/$ENV_NAME.json
sed -i "s/\"fqdn\":.*/\"fqdn\": [\""$MR_IP_ADDR"\", \""$MR_IP_ADDR"\"]/g" /data/environments/$ENV_NAME.json

docker login -u $NEXUS_USERNAME -p $NEXUS_PASSWD $NEXUS_DOCKER_REPO
bash /data/scripts/docker_run.sh -r $RELEASE
