#!/bin/bash

set -x

########## Define parameters ##########
VERSION="1.0.0-SNAPSHOT"

PATH_TO_BOOT="/w/workspace/demo-master-merge-java/boot"
PATH_TO_HEAT_MASTER="/w/workspace/demo-master-merge-java/heat/OpenECOMP"
PATH_TO_HEAT_VFW="/w/workspace/demo-master-merge-java/heat/vFW"
PATH_TO_HEAT_VLB="/w/workspace/demo-master-merge-java/heat/vLB"
PATH_TO_VFW="/w/workspace/demo-master-merge-java/vnfs/vFW/scripts"
PATH_TO_VLB="/w/workspace/demo-master-merge-java/vnfs/vLB/scripts"

BOOT_GROUP_ID="org.openecomp.demo/boot"
HEAT_MASTER_GROUP_ID="org.openecomp.demo/heat/OpenECOMP"
HEAT_VFW_GROUP_ID="org.openecomp.demo/heat/vFW"
HEAT_VLB_GROUP_ID="org.openecomp.demo/heat/vLB"
VFW_GROUP_ID="org.openecomp.demo/vnfs/vfw"
VLB_GROUP_ID="org.openecomp.demo/vnfs/vlb"

REPO_URL="https://nexus.openecomp.org/content/sites/raw"
USER=$(xpath -q -e "//servers/server[id='ecomp-raw']/username/text()" "$SETTINGS_FILE")
PASS=$(xpath -q -e "//servers/server[id='ecomp-raw']/password/text()" "$SETTINGS_FILE")
NETRC=$(mktemp)
echo "machine nexus.openecomp.org login ${USER} password ${PASS}" > "${NETRC}"
#######################################

##### Upload scripts into Nexus raw repository #####
cd $PATH_TO_BOOT
ls | xargs -I{} curl -vk --netrc-file "${NETRC}" --upload-file {} $REPO_URL/$BOOT_GROUP_ID/$VERSION/{}

cd $PATH_TO_HEAT_MASTER
ls | xargs -I{} curl -vk --netrc-file "${NETRC}" --upload-file {} $REPO_URL/$HEAT_GROUP_ID/$VERSION/{}

cd $PATH_TO_HEAT_VFW
ls | xargs -I{} curl -vk --netrc-file "${NETRC}" --upload-file {} $REPO_URL/$HEAT_GROUP_ID/$VERSION/{}

cd $PATH_TO_HEAT_VLB
ls | xargs -I{} curl -vk --netrc-file "${NETRC}" --upload-file {} $REPO_URL/$HEAT_GROUP_ID/$VERSION/{}

cd $PATH_TO_VFW
ls | xargs -I{} curl -vk --netrc-file "${NETRC}" --upload-file {} $REPO_URL/$VFW_GROUP_ID/$VERSION/{}

cd $PATH_TO_VLB
ls | xargs -I{} curl -vk --netrc-file "${NETRC}" --upload-file {} $REPO_URL/$VLB_GROUP_ID/$VERSION/{}
####################################################

########## Clean up ##########
rm ${NETRC}
##############################
