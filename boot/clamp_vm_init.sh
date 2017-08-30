#!/bin/bash

NEXUS_USERNAME=$(cat /opt/config/nexus_username.txt)
NEXUS_PASSWD=$(cat /opt/config/nexus_password.txt)
NEXUS_DOCKER_REPO=$(cat /opt/config/nexus_docker_repo.txt)
DMAAP_TOPIC=$(cat /opt/config/dmaap_topic.txt)
OPENSTACK_USERNAME=$(cat /opt/config/openstack_username.txt)
OPENSTACK_API_KEY=$(cat /opt/config/openstack_api_key.txt)

# Fetch the latest code/scripts
cd /opt/clamp
git pull
