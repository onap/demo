#############################################################################
#
# Copyright © 2019 AT&T Intellectual Property. All rights reserved.
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
CODE_ROOT=$(CURDIR)/../..
#CODE_ROOT=../code/evel-library
LIBS_DIR=$(CODE_ROOT)/libs/x86_$(ARCH)
#LIBS_DIR=/usr/lib
INCLUDE_DIR= -I $(CODE_ROOT)/code/evel_library -I . 

#******************************************************************************
# Standard compiler flags.                                                    *
#******************************************************************************
CPPFLAGS=
CFLAGS=-Wall -g -fPIC
FILEOBJLIST= jsmn.o vpp_measurement_reporter.o

all:	vpp_measurement_reporter

clean:
	rm -f  *.o vpp_measurement_reporter

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(INCLUDE_DIR) -c $< -o $@

jsmn.o: jsmn.c jsmn.h
vpp_measurement_reporter.o: vpp_measurement_reporter.c

vpp_measurement_reporter: $(FILEOBJLIST)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o vpp_measurement_reporter \
                                    -L $(LIBS_DIR) \
                               $(FILEOBJLIST) \
                              -lpthread \
                              -level \
                              -lcurl


