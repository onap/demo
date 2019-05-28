# SPDX-license-identifier: Apache-2.0
##############################################################################
# Copyright (c) 2019 Intel Corporation
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Apache License, Version 2.0
# which accompanies this distribution, and is available at
# http://www.apache.org/licenses/LICENSE-2.0
##############################################################################

GOPATH := $(shell realpath "$(PWD)/../../../../../")
COP = ${PWD}/build/_output/bin/collectd-operator
IMAGE_NAME = dcr.cluster.local:31976/collectd-operator:latest

export GOPATH ...
export GO111MODULE=on

.PHONY: clean plugins


all: clean
	GOOS=linux GOARCH=amd64
	operator-sdk generate k8s --verbose
	operator-sdk generate openapi --verbose
	#@go build -o ${COP} -gcflags all=-trimpath=${GOPATH} -asmflags all=-trimpath=${GOPATH} -mod=vendor demo/vnfs/DAaaS/collectd-operator/cmd/manager
	@operator-sdk build ${IMAGE_NAME} --verbose

# The following is done this way as each patch on CI runs build and each merge runs deploy. So for build we don't need to build binary and hence
# no need to create a static binary with additional flags. However, for generating binary, additional build flags are necessary. This if used with
# mock plugin errors out for unit tests. So the seperation avoids the error.

build: clean test cover
deploy: build publish

publish:
	@docker push ${IMAGE_NAME}
.PHONY: test
test: clean
	@go test -v ./...

format:
	@go fmt ./...

clean:
	@echo "Deleting the collectd-operator binary"
	@rm -rf ${PWD}/build/_output/bin/collectd-operator
	@echo "Deleting the collectd-operator docker image"
	@docker rmi ${IMAGE_NAME}

.PHONY: cover
cover:
	@go test ./... -coverprofile=coverage.out
	@go tool cover -html=coverage.out -o coverage.html
