#!/bin/bash

########## Define parameters ##########
VERSION="1.0.0-SNAPSHOT"
pwd
PATH_TO_BOOT="boot"
PATH_TO_HEAT="heat"
PATH_TO_VFW="vnfs/vFW/scripts"
PATH_TO_VLB="vnfs/vLB/scripts"

BOOT_GROUP_ID="org.openecomp.demo.boot"
HEAT_GROUP_ID="org.openecomp.demo.heat"
VFW_GROUP_ID="org.openecomp.demo.vnfs.vfw"
VLB_GROUP_ID="org.openecomp.demo.vnfs.vlb"

#NEXUSPROXY=https://nexus.openecomp.org
REPO_URL="${NEXUSPROXY}/content/sites/raw"
USER=$(xpath -q -e "//servers/server[id='ecomp-raw']/username/text()" "$SETTINGS_FILE")
PASS=$(xpath -q -e "//servers/server[id='ecomp-raw']/password/text()" "$SETTINGS_FILE")
NETRC=$(mktemp)
echo "machine nexus.openecomp.org login ${USER} password ${PASS}" > "${NETRC}"
#######################################

##### Upload scripts into Nexus raw repository #####
cd $PATH_TO_BOOT
ls | xargs -I{} curl -k --netrc-file '${NETRC}' --upload-file {} $REPO_URL/$BOOT_GROUP_ID/$VERSION/{}

cd $PATH_TO_HEAT
ls | xargs -I{} curl -k --netrc-file '${NETRC}' --upload-file {} $REPO_URL/$HEAT_GROUP_ID/$VERSION/{}

cd $PATH_TO_VFW
ls | xargs -I{} curl -k --netrc-file '${NETRC}' --upload-file {} $REPO_URL/$VFW_GROUP_ID/$VERSION/{}

cd $PATH_TO_VLB
ls | xargs -I{} curl -k --netrc-file '${NETRC}' --upload-file {} $REPO_URL/$VLB_GROUP_ID/$VERSION/{}
####################################################

########## Clean up ##########
rm ${NETRC}
##############################
