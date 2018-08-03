#!/bin/bash

# Read configuration files
GERRIT_BRANCH=$(cat /opt/config/gerrit_branch.txt)
AAI_INSTANCE=$(cat /opt/config/aai_instance.txt)
CODE_REPO=$(cat /opt/config/remote_repo.txt)
HTTP_PROXY=$(cat /opt/config/http_proxy.txt)
HTTPS_PROXY=$(cat /opt/config/https_proxy.txt)

if [ $HTTP_PROXY != "no_proxy" ]
then
    export http_proxy=$HTTP_PROXY
    export https_proxy=$HTTPS_PROXY
fi


# Download scripts from Nexus
update-rc.d aai_serv.sh defaults


# Run docker containers
cd /opt
git clone -b $GERRIT_BRANCH --single-branch $CODE_REPO

if [[ $AAI_INSTANCE == "aai_instance_1" ]]
then
	mkdir -p /opt/aai/logroot/AAI-RESOURCES
	mkdir -p /opt/aai/logroot/AAI-TRAVERSAL
	mkdir -p /opt/aai/logroot/AAI-ML
	mkdir -p /opt/aai/logroot/AAI-SDB
	mkdir -p /opt/aai/logroot/AAI-DRMS
	mkdir -p /opt/aai/logroot/AAI-UI
	chown -R 999:999 /opt/aai/logroot/AAI-RESOURCES /opt/aai/logroot/AAI-TRAVERSAL

	sleep 300
fi

./aai_vm_init.sh