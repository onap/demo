#!/bin/bash

set -x

########## Define parameters ##########
VERSION="1.0.0-SNAPSHOT"

PATH_TO_BOOT="/w/workspace/demo-master-merge-java/boot"
PATH_TO_HEAT="/w/workspace/demo-master-merge-java/heat"
PATH_TO_VFW="/w/workspace/demo-master-merge-java/vnfs/vFW/scripts"
PATH_TO_VLB="/w/workspace/demo-master-merge-java/vnfs/vLB/scripts"

BOOT_GROUP_ID="org.openecomp.demo.boot"
HEAT_GROUP_ID="org.openecomp.demo.heat"
VFW_GROUP_ID="org.openecomp.demo.vnfs.vfw"
VLB_GROUP_ID="org.openecomp.demo.vnfs.vlb"

REPO_URL="https://nexus.openecomp.org/content/sites/raw"
USER=$(xpath -q -e "//servers/server[id='ecomp-raw']/username/text()" "$SETTINGS_FILE")
PASS=$(xpath -q -e "//servers/server[id='ecomp-raw']/password/text()" "$SETTINGS_FILE")
NETRC=$(mktemp)
echo "machine nexus.openecomp.org login ${USER} password ${PASS}" > "${NETRC}"
#######################################

##### Upload scripts into Nexus raw repository #####
cd $PATH_TO_BOOT
#ls | xargs -I{} curl -vk --netrc-file "${NETRC}" --upload-file {} $REPO_URL/$BOOT_GROUP_ID/$VERSION/{}
ls | xargs -I{} curl -k -v -u $USER:$PASS --upload-file {} $REPO_URL/$BOOT_GROUP_ID/$VERSION/{}

cd $PATH_TO_HEAT
#ls | xargs -I{} curl -vk --netrc-file "${NETRC}" --upload-file {} $REPO_URL/$HEAT_GROUP_ID/$VERSION/{}
ls | xargs -I{} curl -k -v -u $USER:$PASS --upload-file {} $REPO_URL/$HEAT_GROUP_ID/$VERSION/{}

cd $PATH_TO_VFW
#ls | xargs -I{} curl -vk --netrc-file "${NETRC}" --upload-file {} $REPO_URL/$VFW_GROUP_ID/$VERSION/{}
ls | xargs -I{} curl -k -v -u $USER:$PASS --upload-file {} $REPO_URL/$VFW_GROUP_ID/$VERSION/{}

cd $PATH_TO_VLB
#ls | xargs -I{} curl -vk --netrc-file "${NETRC}" --upload-file {} $REPO_URL/$VLB_GROUP_ID/$VERSION/{}
ls | xargs -I{} curl -k -v -u $USER:$PASS --upload-file {} $REPO_URL/$VLB_GROUP_ID/$VERSION/{}
####################################################

########## Clean up ##########
rm ${NETRC}
##############################
