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
# a) scripts for message router (mr)
update-rc.d mr_serv.sh defaults

# b) scripts for bus controller (dbcl)


# Clone Gerrit repository and run docker containers
cd /opt
git clone -b $GERRIT_BRANCH --single-branch $CODE_REPO /opt/startup-vm-message-router
./mr_vm_init.sh
