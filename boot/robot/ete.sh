#!/bin/bash

#
# Run the testsuite for the passed tag. Valid tags are ete, health, closedloop, instantiate
# Please clean up logs when you are done...
# Note: Do not run multiple concurrent ete.sh as the --display is not parameterized and tests will collide
#
if [ "$1" == "" ];then
   echo "Usage: ete.sh [ health | ete | closedloop | instantiate | distribute | portal ]"
   exit
fi

mkdir -p /opt/eteshare/logs

export TAGS="-i $1"
export ETEHOME=/var/opt/OpenECOMP_ETE
export GLOBAL_BUILD_NUMBER=$(ls -1q /opt/eteshare/logs/ | wc -l)
export OUTPUT_FOLDER=ETE_$(printf %04d $GLOBAL_BUILD_NUMBER)_$1

VARIABLEFILES="-V /share/config/vm_properties.py -V /share/config/integration_robot_properties.py -V /share/config/integration_preload_parameters.py"
VARIABLES="-v GLOBAL_BUILD_NUMBER:$GLOBAL_BUILD_NUMBER --exclude datarouter"

docker exec openecompete_container ${ETEHOME}/runTags.sh ${VARIABLEFILES} ${VARIABLES} -d /share/logs/${OUTPUT_FOLDER} ${TAGS} --exclude oom --display 88
