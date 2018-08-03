#!/bin/bash

AAI_INSTANCE=$(cat /opt/config/aai_instance.txt)

cd /opt/test-config
git pull

if [[ $AAI_INSTANCE == "aai_instance_1" ]]
then
	./deploy_vm1.sh
elif [[ $AAI_INSTANCE == "aai_instance_2" ]]
then
	./deploy_vm2.sh
else
	echo "Invalid instance. Exiting..."
fi
