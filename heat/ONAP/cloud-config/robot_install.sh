#!/bin/bash -x

# Read configuration files
HTTP_PROXY=$(cat /opt/config/http_proxy.txt)
HTTPS_PROXY=$(cat /opt/config/https_proxy.txt)

if [ $HTTP_PROXY != "no_proxy" ]
then
    export http_proxy=$HTTP_PROXY
    export https_proxy=$HTTPS_PROXY
fi

# Short-term fix to get around MSO to SO name change
cp /opt/config/so_ip_addr.txt /opt/config/mso_ip_addr.txt


# Download scripts from Nexus
mkdir -p /opt/eteshare/config
cp /opt/boot/robot/integration_preload_parameters.py /opt/eteshare/config/integration_preload_parameters.py
cp /opt/boot/robot/integration_robot_properties.py /opt/eteshare/config/integration_robot_properties.py
cp /opt/boot/robot/vm_config2robot.sh /opt/eteshare/config/vm_config2robot.sh
chmod +x /opt/eteshare/config/vm_config2robot.sh
cp /opt/boot/robot/ete.sh /opt/ete.sh
chmod +x /opt/ete.sh
cp /opt/boot/robot/demo.sh /opt/demo.sh
chmod +x /opt/demo.sh

mkdir -p /opt/eteshare/logs

# Run docker containers.
./robot_vm_init.sh
