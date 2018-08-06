#!/bin/bash

# Read configuration files
VNFSDK_BRANCH=$(cat /opt/config/vnfsdk_branch.txt)
VNFSDK_REPO=$(cat /opt/config/vnfsdk_repo.txt)
HTTP_PROXY=$(cat /opt/config/http_proxy.txt)
HTTPS_PROXY=$(cat /opt/config/https_proxy.txt)

if [ $HTTP_PROXY != "no_proxy" ]
then
    export http_proxy=$HTTP_PROXY
    export https_proxy=$HTTPS_PROXY
fi

# Download dependencies
apt-get update
apt-get install -y mysql-client-core-5.6

# Download scripts from Nexus
cp /opt/boot/vnfsdk_vm_init.sh /opt/vnfsdk_vm_init.sh
cp /opt/boot/msb_vm_init.sh /opt/msb_vm_init.sh
cp /opt/boot/mvim_vm_init.sh /opt/mvim_vm_init.sh
cp /opt/boot/vfc_vm_init.sh /opt/vfc_vm_init.sh
cp /opt/boot/uui_vm_init.sh /opt/uui_vm_init.sh
cp /opt/boot/multiserv_all_serv.sh /opt/multiserv_all_serv.sh
cp /opt/boot/esr_vm_init.sh /opt/esr_vm_init.sh
chmod +x /opt/vnfsdk_vm_init.sh
chmod +x /opt/msb_vm_init.sh
chmod +x /opt/mvim_vm_init.sh
chmod +x /opt/vfc_vm_init.sh
chmod +x /opt/uui_vm_init.sh
chmod +x /opt/multiserv_all_serv.sh
chmod +x /opt/esr_vm_init.sh

# Clone Gerrit repository and run docker containers
cd /opt
git clone -b $VNFSDK_BRANCH --single-branch $VNFSDK_REPO
source ./cli_install.sh
./multiserv_all_serv.sh
