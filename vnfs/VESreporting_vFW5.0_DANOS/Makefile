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
MACHINE_ARCH=$(shell uname -m)
CODE_ROOT=$(CURDIR)
LIBS_DIR=$(CODE_ROOT)/evel/evel-library/libs/$(MACHINE_ARCH)
INCLUDE_DIR=$(CODE_ROOT)/evel/evel-library/code/evel_library
EVEL_CODE_PATH=${CODE_ROOT}/evel/evel-library/code/evel_library/
EVEL_BUILD_PATH=${CODE_ROOT}/evel/evel-library/bldjobs

#******************************************************************************
# Standard compiler flags.                                                    *
#******************************************************************************
CPPFLAGS=
CFLAGS=-Wall -g -fPIC

all:	evel \
	vpp-measurement-reporter-danos

clean:
	rm -f vpp-measurement-reporter-danos

vpp-measurement-reporter-danos:	${LIBS_DIR}/evel.so vpp-measurement-reporter-danos.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -o vpp-measurement-reporter-danos \
                                    -L $(LIBS_DIR) \
                                    -I $(INCLUDE_DIR) \
                               vpp-measurement-reporter-danos.c \
                              -lpthread \
                              -level \
                              -ljansson \
			      -lvyatta-config \
                              -lcurl
	ls -l ${CODE_ROOT}/evel/evel-library/libs/x86_64/

${LIBS_DIR}/evel.so:	${EVEL_CODE_PATH}/evel.c
	${MAKE} -C ${EVEL_BUILD_PATH}  all

