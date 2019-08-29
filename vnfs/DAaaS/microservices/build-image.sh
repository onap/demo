#!/usr/bin/env bash

set -o nounset
set -o errexit
set -o pipefail

OPERATOR=$1
OPERATOR_IMAGE=$2
echo "Building $OPERATOR image $OPERATOR_IMAGE"
CMD="export GO111MODULE=on && cd $OPERATOR && operator-sdk build $OPERATOR_IMAGE"
echo $CMD
docker run --rm -it \
    -v /var/run/docker.sock:/var/run/docker.sock \
    -v $PWD:/app/demo/vnfs/DAaaS/microservices \
    operator-image-builder:latest bash -c "$CMD"

