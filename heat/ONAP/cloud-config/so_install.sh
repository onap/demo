#!/bin/bash

# Read configuration files
#OPENSTACK_API_KEY=$(cat /opt/config/openstack_api_key.txt)
GERRIT_BRANCH=$(cat /opt/config/gerrit_branch.txt)
CODE_REPO=$(cat /opt/config/remote_repo.txt)
HTTP_PROXY=$(cat /opt/config/http_proxy.txt)
HTTPS_PROXY=$(cat /opt/config/https_proxy.txt)

if [ $HTTP_PROXY != "no_proxy" ]
then
    export http_proxy=$HTTP_PROXY
    export https_proxy=$HTTPS_PROXY
fi

# Clone Gerrit repository and run docker containers.
cd /opt
git clone -b $GERRIT_BRANCH --single-branch $CODE_REPO test_lab
#SO_ENCRYPTION_KEY=$(cat /opt/test_lab/encryption.key)
#echo -n "$OPENSTACK_API_KEY" | openssl aes-128-ecb -e -K $SO_ENCRYPTION_KEY -nosalt | xxd -c 256 -p > /opt/config/api_key.txt

./so_vm_init.sh
