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
CODE_ROOT=$(CURDIR)/../..
#CODE_ROOT=../VES/evel/evel-library
#LIBS_DIR=$(CODE_ROOT)/libs/x86_$(ARCH)
#LIBS_DIR=/usr/lib
INCLUDE_DIR= -I $(CODE_ROOT)/code/evel_library -I .

#******************************************************************************
# Standard compiler flags.                                                    *
#******************************************************************************
CPPFLAGS=
CFLAGS=-Wall -g -fPIC
FILEOBJLIST= afx_ves_reporter.o afx_util.o\
             afx_bgpsyslog.o afx_perfmon.o  afx_linkmon.o  afx_svc.o 

all:	afx_ves_reporter

clean:
	rm -f  *.o afx_ves_reporter

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(INCLUDE_DIR) -c $< -o $@

afx_ves_reporter.o: afx_ves_reporter.c afx_ves_reporter.h
afx_svc.o: afx_svc.c afx_ves_reporter.h
afx_linkmon.o: afx_linkmon.c afx_ves_reporter.h
afx_perfmon.o: afx_perfmon.c afx_ves_reporter.h
afx_bgpsyslog.o: afx_bgpsyslog.c afx_ves_reporter.h
afx_util.o: afx_util.c afx_ves_reporter.h

afx_ves_reporter: $(FILEOBJLIST)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o afx_ves_reporter \
                                $(LIBS_DIR) \
                               $(FILEOBJLIST) \
                              -lpthread \
                              -level \
                              -lcurl


