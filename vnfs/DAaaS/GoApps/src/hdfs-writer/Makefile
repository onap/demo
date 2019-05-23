# SPDX-license-identifier: Apache-2.0
##############################################################################
# Copyright (c) 2019 Intel Corporation
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Apache License, Version 2.0
# which accompanies this distribution, and is available at
# http://www.apache.org/licenses/LICENSE-2.0
##############################################################################


GOPATH := $(shell realpath "$(PWD)/../../")
#export GOPATH=/Users/rajamoha/projects/demo/vnfs/DAaaS/GoApps
export GOPATH ...
export GO111MODULE=on



all:
	echo $$GOPATH
	GOOS=linux GOARCH=amd64
	@go build -tags netgo -o ./bin/hdfs-writer ./cmd/main.go

build:
	echo $$GOPATH
	GOOS=linux GOARCH=amd64
	@go build -tags netgo -o ./bin/hdfs-writer ./cmd/main.go

.PHONY: format
format:
	@go fmt ./...

.PHONY: clean
clean:
	@find . -name "*so" -delete
	@rm -f ./bin/hdfs-writer
