#!/bin/bash

set -x

########## Define parameters ##########
source ${WORKSPACE}"/version.properties"
BRANCH=$(echo ${WORKSPACE} | cut -d'-' -f2)
if [[ $BRANCH == "master" ]]
then
	VERSION=$snapshot_version
else
	VERSION=$release_version
fi

echo "Workspace: " ${WORKSPACE}
echo "Gerrit branch: " $BRANCH
echo "Version number: " $VERSION

PATH_TO_BOOT=${WORKSPACE}"/boot"
PATH_TO_HEAT=${WORKSPACE}"/heat"
PATH_TO_HEAT_MASTER=${WORKSPACE}"/heat/OpenECOMP"
PATH_TO_HEAT_VFW=${WORKSPACE}"/heat/vFW"
PATH_TO_HEAT_VLB=${WORKSPACE}"/heat/vLB"
PATH_TO_VFW=${WORKSPACE}"/vnfs/vFW/scripts"
PATH_TO_VLB=${WORKSPACE}"/vnfs/vLB/scripts"

BOOT_GROUP_ID="org.openecomp.demo/boot"
HEAT_GROUP_ID="org.openecomp.demo/heat"
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

cd $PATH_TO_HEAT
curl -vk --netrc-file "${NETRC}" --upload-file $PATH_TO_HEAT/README.md $REPO_URL/$HEAT_GROUP_ID/
curl -vk --netrc-file "${NETRC}" --upload-file $PATH_TO_HEAT/LICENSE.TXT $REPO_URL/$HEAT_GROUP_ID/

cd $PATH_TO_HEAT_MASTER
ls | xargs -I{} curl -vk --netrc-file "${NETRC}" --upload-file {} $REPO_URL/$HEAT_MASTER_GROUP_ID/$VERSION/{}

cd $PATH_TO_HEAT_VFW
ls | xargs -I{} curl -vk --netrc-file "${NETRC}" --upload-file {} $REPO_URL/$HEAT_VFW_GROUP_ID/$VERSION/{}

cd $PATH_TO_HEAT_VLB
ls | xargs -I{} curl -vk --netrc-file "${NETRC}" --upload-file {} $REPO_URL/$HEAT_VLB_GROUP_ID/$VERSION/{}

cd $PATH_TO_VFW
ls | xargs -I{} curl -vk --netrc-file "${NETRC}" --upload-file {} $REPO_URL/$VFW_GROUP_ID/$VERSION/{}

cd $PATH_TO_VLB
ls | xargs -I{} curl -vk --netrc-file "${NETRC}" --upload-file {} $REPO_URL/$VLB_GROUP_ID/$VERSION/{}
####################################################

########## Clean up ##########
rm ${NETRC}
##############################
