#!/bin/bash

NEXUS_USERNAME=$(cat /opt/config/nexus_username.txt)
NEXUS_PASSWD=$(cat /opt/config/nexus_password.txt)
NEXUS_DOCKER_REPO=$(cat /opt/config/nexus_docker_repo.txt)
NEXUS_REPO=$(cat /opt/config/nexus_repo.txt)
DOCKER_IMAGE_VERSION=$(cat /opt/config/docker_version.txt)
CLOUD_ENV=$(cat /opt/config/cloud_env.txt)

#
# Deploy latest robot configuration 
#
cd /opt/testsuite/properties
git pull
cp integration_* /opt/eteshare/config
cp vm_config2robot.sh /opt/eteshare/config
cp ete.sh /opt
cp demo.sh /opt

#
# Deploy latest heat templates
#
cd /opt/demo
git pull
cp -rf heat /opt/eteshare

#if [[ $CLOUD_ENV != "rackspace" ]]
#then
#	sed -i "s/10.0.1.1/"$(cat /opt/config/aai1_ip_addr.txt)"/g" /opt/eteshare/config/integration_robot_properties.py
#	sed -i "s/10.0.2.1/"$(cat /opt/config/appc_ip_addr.txt)"/g" /opt/eteshare/config/integration_robot_properties.py
#	sed -i "s/10.0.4.1/"$(cat /opt/config/dcae_ip_addr.txt)"/g" /opt/eteshare/config/integration_robot_properties.py
#	sed -i "s/10.0.5.1/"$(cat /opt/config/so_ip_addr.txt)"/g" /opt/eteshare/config/integration_robot_properties.py
#	sed -i "s/10.0.11.1/"$(cat /opt/config/mr_ip_addr.txt)"/g" /opt/eteshare/config/integration_robot_properties.py
#	sed -i "s/10.0.6.1/"$(cat /opt/config/policy_ip_addr.txt)"/g" /opt/eteshare/config/integration_robot_properties.py
#	sed -i "s/10.0.9.1/"$(cat /opt/config/portal_ip_addr.txt)"/g" /opt/eteshare/config/integration_robot_properties.py
#	sed -i "s/10.0.3.1/"$(cat /opt/config/sdc_ip_addr.txt)"/g" /opt/eteshare/config/integration_robot_properties.py
#	sed -i "s/10.0.7.1/"$(cat /opt/config/sdnc_ip_addr.txt)"/g" /opt/eteshare/config/integration_robot_properties.py
#	sed -i "s/10.0.8.1/"$(cat /opt/config/vid_ip_addr.txt)"/g" /opt/eteshare/config/integration_robot_properties.py
#   sed -i "s/10.0.12.1/"$(cat /opt/config/clamp_ip_addr.txt)"/g" /opt/eteshare/config/integration_robot_properties.py
#	sed -i "s/https:\/\/identity.api.rackspacecloud.com/"$(cat /opt/config/keystone.txt)"/g" /opt/eteshare/config/integration_robot_properties.py
#fi

chmod +x /opt/ete.sh
chmod +x /opt/demo.sh

/bin/bash /opt/eteshare/config/vm_config2robot.sh

docker login -u $NEXUS_USERNAME -p $NEXUS_PASSWD $NEXUS_DOCKER_REPO
docker pull $NEXUS_DOCKER_REPO/openecomp/testsuite:$DOCKER_IMAGE_VERSION
docker rm -f openecompete_container

docker pull $NEXUS_DOCKER_REPO/onap/sniroemulator:latest
docker rm -f sniroemulator

docker run -d --name openecompete_container -v /opt/eteshare:/share -p 88:88 $NEXUS_DOCKER_REPO/openecomp/testsuite:$DOCKER_IMAGE_VERSION
docker run -d --name sniroemulator -p 8080:9999 $NEXUS_DOCKER_REPO/onap/sniroemulator:latest
