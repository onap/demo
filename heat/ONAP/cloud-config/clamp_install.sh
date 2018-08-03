#!/bin/bash

# Read configuration files
GERRIT_BRANCH=$(cat /opt/config/gerrit_branch.txt)
CODE_REPO=$(cat /opt/config/remote_repo.txt)
HTTP_PROXY=$(cat /opt/config/http_proxy.txt)
HTTPS_PROXY=$(cat /opt/config/https_proxy.txt)

if [ $HTTP_PROXY != "no_proxy" ]
then
    export http_proxy=$HTTP_PROXY
    export https_proxy=$HTTPS_PROXY
fi


# Download scripts from Nexus
update-rc.d clamp_serv.sh defaults


# Clone Gerrit repository and run docker containers
cd /opt
git clone -b $GERRIT_BRANCH --single-branch $CODE_REPO
./clamp_vm_init.sh
