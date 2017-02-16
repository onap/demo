
#############################################################################
#
# Copyright © 2017 AT&T Intellectual Property. All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#        http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
#############################################################################

CC=gcc

ARCH=$(shell getconf LONG_BIT)
CODE_ROOT=$(CURDIR)/../../..
LIBS_DIR=$(CODE_ROOT)/libs/x86_$(ARCH)
INCLUDE_DIR=$(CODE_ROOT)/code/evel_library

#******************************************************************************
# Standard compiler flags.                                                    *
#******************************************************************************
CPPFLAGS=
CFLAGS=-Wall -g -fPIC

all:	vpp_measurement_reporter

clean:
	rm -f vpp_measurement_reporter

vpp_measurement_reporter: vpp_measurement_reporter.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -o vpp_measurement_reporter \
                                    -L $(LIBS_DIR) \
                                    -I $(INCLUDE_DIR) \
                               vpp_measurement_reporter.c -lm -lpthread -level -lcurl
