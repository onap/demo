# SPDX-license-identifier: Apache-2.0
##############################################################################
# Copyright (c) 2019 Intel Corporation
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Apache License, Version 2.0
# which accompanies this distribution, and is available at
# http://www.apache.org/licenses/LICENSE-2.0
##############################################################################

# Common
VERSION := $(shell git describe --tags)
BUILD := $(shell git rev-parse --short HEAD)
PROJECTNAME := $(shell basename "$(pwd)")

ROOTPATH := $(shell realpath "$(PWD)/../../../../../../")
VOP := ${PWD}/build/_output/bin/visualization-operator
VOP_LOCAL := ${VOP}-local
IMAGE_NAME := dcr.cluster.local:32644/visualization-operator:latest

export GO111MODULE=on

.PHONY: clean publish

## build: Generate the k8s and openapi artifacts using operator-sdk
build: clean 
	GOOS=linux GOARCH=amd64
	operator-sdk generate k8s --verbose
	operator-sdk generate openapi --verbose	
	@operator-sdk build ${IMAGE_NAME} --verbose
	@go build -o ${VOP_LOCAL} visualization-operator/cmd/manager
      
## clean: clean build artifacts, image, binary
clean:
	@echo "Deleting the Visualization-operator binary"
	@rm -rf ${VOP}
	@rm -rf ${VOP_LOCAL}
	@if docker images $(IMAGE_NAME) | awk '{ print $$2 }' | grep -q -F latest; then echo "Deleting the visualization-operator docker image";  docker rmi ${IMAGE_NAME}; fi

## publish: Push docker image to registry
publish:
	@docker push ${IMAGE_NAME}

## format: format source code
format:
	@go fmt ./...

.PHONY: help
## help: Print help message
help: Makefile
	@echo
	@echo " Requires Operator SDK v0.8.0+, go1.12.5+"
	@echo
	@echo " Choose a command run in "$(PROJECTNAME)":"
	@echo
	@sed -n 's/^## //p' $< | column -t -s ':' |  sed -e 's/^/ /'
	@echo
