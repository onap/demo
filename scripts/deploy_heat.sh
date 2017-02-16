#!/bin/bash

source repo_config.txt

SOURCE_DIR=../heat
GROUP_ID=org.openecomp.demo.heat
VERSION=1.0.0

cd $SOURCE_DIR
ls | xargs -I{} curl -v -u $REPO_USERNAME:$REPO_PASSWORD --upload-file {} $REPO_URL/$GROUP_ID/$VERSION/{}

