#!/bin/bash
set -e
set -x

sudo rm -rf /usr/local/go
sudo apt-get install make mercurial
wget https://dl.google.com/go/go1.12.9.linux-amd64.tar.gz
sudo tar -xvf go1.12.9.linux-amd64.tar.gz
sudo mv go /usr/local
export GOROOT=/usr/local/go
export PATH=$PATH:/usr/local/go/bin
export GO111MODULE=on

RELEASE_VERSION=v0.9.0
curl -OJL https://github.com/operator-framework/operator-sdk/releases/download/${RELEASE_VERSION}/operator-sdk-${RELEASE_VERSION}-x86_64-linux-gnu

chmod +x operator-sdk-${RELEASE_VERSION}-x86_64-linux-gnu && sudo cp operator-sdk-${RELEASE_VERSION}-x86_64-linux-gnu /usr/local/bin/operator-sdk && rm operator-sdk-${RELEASE_VERSION}-x86_64-linux-gnu
IMAGE_NAME=$1
if [ -z "$IMAGE_NAME" ]
then
    echo "Building Collectd-Operator image with default image name"
    make
else
    echo "Building Collectd-Operator image $IMAGE_NAME"
    make IMAGE_NAME=$IMAGE_NAME
fi
rm -rf go1.12.9.linux-amd64.tar.gz
