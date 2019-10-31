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
PROJECTNAME := $(shell basename "$(PWD)")

ROOTPATH := $(shell realpath "$(PWD)/../")
PKA := prom-kafka-writer
ifndef IMAGE_NAME
override IMAGE_NAME := dcr.cluster.local:32644/prom-kafka-writer:latest
endif

export GO111MODULE=on

.PHONY: clean plugins

## build: Build the binary for prom-kafka-writer
build: clean format
	GOOS=linux GOARCH=amd64
	@go mod download
	@go build -o ./bin/prom-kafka-writer -v ./cmd/prom-kafka-writer

## all: Delete the image, binary, complete build, test and run coverage
all: build test cover
## debug: Build local binary for debugging
debug:
	@echo "Use Cloud Code VSCode plugin or Skaffold to debug"

## deploy: Build Dockerfile and publish to repository
deploy: build test publish

## publish: Push docker image to repository
publish:
	@docker push ${IMAGE_NAME}
.PHONY: test
## test: run tests
test: clean
	@go test -v ./...
## format: format source code
format:
	@go fmt ./...

## clean: clean build artifacts, image, binary 
clean: format
	@echo "Deleting the prom-kafka-writer binary"
	@rm -rf ${PKA}
	@echo "Deleting the prom-kafka-writer docker image"
	@docker 2>/dev/null rmi ${IMAGE_NAME} | true

.PHONY: cover
## cover: run tests and generate coverage report
cover:
	@go test ./... -coverprofile=coverage.out
	@go tool cover -html=coverage.out -o coverage.html

.PHONY: help
## help: Print help message
help: Makefile
	@echo
	@echo " Requires librdkafka v1.1.0 or later, go1.12.5+"
	@echo
	@echo " Choose a command run in "$(PROJECTNAME)":"
	@echo
	@sed -n 's/^## //p' $< | column -t -s ':' |  sed -e 's/^/ /'
	@echo
