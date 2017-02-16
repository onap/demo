#******************************************************************************
# The ECOMP Vendor Event Listener (EVEL) API client library Makefile.
#
# Make the various targets associated with housekeeping functions as part of
# Event Reporting library.
#
# NOTE: because Makefiles assign special meaning to the TAB character you
#       will need to set tabstops to 2 characters for the layout to look OK.
#
# License
# -------
#
# Copyright(c) <2016>, AT&T Intellectual Property.  All other rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
# 3. All advertising materials mentioning features or use of this software
#    must display the following acknowledgement:  This product includes
#    software developed by the AT&T.
# 4. Neither the name of AT&T nor the names of its contributors may be used to
#    endorse or promote products derived from this software without specific
#    prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY AT&T INTELLECTUAL PROPERTY ''AS IS'' AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL AT&T INTELLECTUAL PROPERTY BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#******************************************************************************

ARCH=$(shell getconf LONG_BIT)
CODE_ROOT=$(CURDIR)/..
EVELLIB_ROOT=$(CODE_ROOT)/code/evel_library
EVELDEMO_ROOT=$(CODE_ROOT)/code/evel_demo
EVELUNIT_ROOT=$(CODE_ROOT)/code/evel_unit
EVELTRAINING_ROOT=$(CODE_ROOT)/code/evel_training
LIBS_DIR=$(CODE_ROOT)/libs/x86_$(ARCH)
OUTPUT_DIR=$(CODE_ROOT)/output/x86_$(ARCH)
DOCS_ROOT=$(CODE_ROOT)/docs
CC=gcc
SCP=scp
SSH=ssh
JAVA=java
DOXYGEN=doxygen
PLANTUML=/usr/local/bin/plantuml.jar
PLANTFLAGS=-tsvg

#******************************************************************************
# Standard compiler flags.                                                    *
#******************************************************************************
CPPFLAGS=-I $(EVELLIB_ROOT)
CFLAGS=-Wall -Wextra -m$(ARCH) -g -fPIC
LIBCFLAGS=-Wall -Wextra -m$(ARCH) -g -shared -fPIC

#******************************************************************************
# The testbed is a VM instance where we can install the EVEL example under    *
# CentOS.                                                                     *
#******************************************************************************
VNF_TESTBED_CENTOS=172.18.152.180
VNF_TESTBED_CENTOS_USER=centos
TESTBED_CENTOS_DOWNLOAD_PATH=/home/centos/download/evel_lib
TESTBED_CENTOS_INSTALL_PATH=/home/centos/evel

#******************************************************************************
# The testbed is a VM instance where we can install the EVEL example under    *
# Ubuntu.                                                                     *
#******************************************************************************
VNF_TESTBED_UBUNTU=172.18.152.179
VNF_TESTBED_UBUNTU_USER=ubuntu
TESTBED_UBUNTU_DOWNLOAD_PATH=/home/ubuntu/Downloads/evel_lib
TESTBED_UBUNTU_INSTALL_PATH=/home/ubuntu/evel

#******************************************************************************
# The test-collector is where we can send events to be consumed and checked   *
# during tests.                                                               *
#******************************************************************************
VNF_COLLECTOR_HOST=172.18.152.185
VNF_COLLECTOR_PORT=30000

#******************************************************************************
# A documentation server used by the team where we can install documentation. *
#******************************************************************************
TEAM_DOCS_SERVER=covlx8
DOCS_SERVER_PATH=/var/www/html/evel

#******************************************************************************
# Implicit rule to make dependency files.  Recipe copied from Gnu docs at:    *
# https://www.gnu.org/software/make/manual/html_node/Automatic-Prerequisites.html                                      *
#******************************************************************************
%.d: %.c
	@echo Making dependency file $(notdir $@) for $(notdir $<)
	@set -e; rm -f $@; \
         $(CC) -MM -MT $(<:.c=.o) $(CPPFLAGS) $< > $@.$$$$; \
         sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
         rm -f $@.$$$$

#******************************************************************************
# Implicit rule to make object files.                                         *
#******************************************************************************
%.o: %.c
	@echo Making $(notdir $@) from $(notdir $<)
	@$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

#******************************************************************************
# Implicit rule to make diagram files using PlantUML.                         *
#******************************************************************************
%.svg : %.plantuml
	@echo Making $(notdir $@)
	$(JAVA) -jar $(PLANTUML) $(PLANTFLAGS) $<

all:     api_library \
         evel_library_training

clean:   api_library_clean \
         evel_unit_clean \
         evel_library_training_clean \
         docs_clean

install: evel_install_centos evel_install_ubuntu

test: evel_test_centos evel_test_ubuntu

docs:    docs_clean doxygen_docs


#******************************************************************************
# Build the EVEL libraries.                                                   *
#******************************************************************************
API_SOURCES=$(EVELLIB_ROOT)/evel.c \
            $(EVELLIB_ROOT)/metadata.c \
            $(EVELLIB_ROOT)/ring_buffer.c \
            $(EVELLIB_ROOT)/double_list.c \
            $(EVELLIB_ROOT)/evel_event.c \
            $(EVELLIB_ROOT)/evel_fault.c \
            $(EVELLIB_ROOT)/evel_mobile_flow.c \
            $(EVELLIB_ROOT)/evel_option.c \
            $(EVELLIB_ROOT)/evel_other.c \
            $(EVELLIB_ROOT)/evel_json_buffer.c \
            $(EVELLIB_ROOT)/evel_reporting_measurement.c \
            $(EVELLIB_ROOT)/evel_scaling_measurement.c \
            $(EVELLIB_ROOT)/evel_state_change.c \
            $(EVELLIB_ROOT)/evel_strings.c \
            $(EVELLIB_ROOT)/evel_syslog.c \
            $(EVELLIB_ROOT)/evel_throttle.c \
            $(EVELLIB_ROOT)/evel_internal_event.c \
            $(EVELLIB_ROOT)/evel_event_mgr.c \
            $(EVELLIB_ROOT)/evel_logging.c \
            $(EVELLIB_ROOT)/jsmn.c \
            $(EVELLIB_ROOT)/evel_service.c \
            $(EVELLIB_ROOT)/evel_signaling.c

API_OBJECTS=$(API_SOURCES:.c=.o)
-include $(API_SOURCES:.c=.d)

api_library: $(LIBS_DIR)/libevel.so \
             $(LIBS_DIR)/libevel.a

$(LIBS_DIR)/libevel.a: $(API_OBJECTS)
	@echo	Linking API Static Library
	@$(CC) $(LIBCFLAGS) -o $@ $+

$(LIBS_DIR)/libevel.so: $(API_OBJECTS)
	@echo	Linking API Shared Library
	@$(CC) $(LIBCFLAGS) -L $(QLIBCLIBSDIR) -lqlibc -o $@ $+

api_library_clean:
	@echo	Cleaning API Library
	@$(RM) $(LIBS_DIR)/libevel.so
	@$(RM) $(API_OBJECTS)
	@$(RM) $(EVELLIB_ROOT)/*.d

#******************************************************************************
# Build the EVEL library unit test.                                           *
#******************************************************************************
UNIT_SOURCES=$(EVELUNIT_ROOT)/evel_unit.c
UNIT_OBJECTS=$(UNIT_SOURCES:.c=.o)
-include $(UNIT_SOURCES:.c=.d)

evel_unit: api_library \
           $(OUTPUT_DIR)/evel_unit

$(OUTPUT_DIR)/evel_unit: $(UNIT_OBJECTS)
	@echo	Linking EVEL unit test
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ \
                          -L $(LIBS_DIR) \
                          $(UNIT_OBJECTS) \
                          -level \
                          -lpthread \
                          -lcurl

evel_unit_clean:
	@echo	Cleaning EVEL unit test
	@$(RM) $(OUTPUT_DIR)/evel_unit
	@$(RM) $(API_OBJECTS)
	@$(RM) $(UNIT_OBJECTS)
	@$(RM) $(EVELLIB_ROOT)/*.d
	@$(RM) $(EVELUNIT_ROOT)/*.d

#******************************************************************************
# Build the EVEL library training files.                                      *
#******************************************************************************
evel_library_training:
	@echo	Making EVEL training
	@$(MAKE) -s -C $(EVELTRAINING_ROOT)/VESreporting

evel_library_training_clean:
	@echo	Cleaning EVEL training
	@$(RM) $(EVELTRAINING_ROOT)/VESreporting/vpp_measurement_reporter

#******************************************************************************
# Copy the EVEL demo onto the CentOS testbed as a package and build it.       *
#******************************************************************************
evel_install_centos: delivery
	@echo Installing EVEL library on CentOS testbed...
	@$(SSH) $(VNF_TESTBED_CENTOS_USER)@$(VNF_TESTBED_CENTOS) \
          rm -rf $(TESTBED_CENTOS_DOWNLOAD_PATH) \; \
          mkdir -p $(TESTBED_CENTOS_DOWNLOAD_PATH) \; \
          mkdir -p $(TESTBED_CENTOS_INSTALL_PATH)
	@$(SCP) -r $(CODE_ROOT)/output/evel-library-package.tgz \
           $(VNF_TESTBED_CENTOS_USER)@$(VNF_TESTBED_CENTOS):$(TESTBED_CENTOS_DOWNLOAD_PATH)
	@$(SSH) $(VNF_TESTBED_CENTOS_USER)@$(VNF_TESTBED_CENTOS) \
          tar zx --directory $(TESTBED_CENTOS_INSTALL_PATH) \
                 --file $(TESTBED_CENTOS_DOWNLOAD_PATH)/evel-library-package.tgz
	@echo Making EVEL library on testbed...
	@$(SSH) $(VNF_TESTBED_CENTOS_USER)@$(VNF_TESTBED_CENTOS) \
           cd $(TESTBED_CENTOS_INSTALL_PATH)/bldjobs \; \
           make clean all

#******************************************************************************
# Copy the EVEL demo onto the Ubuntu testbed as a package and build it.       *
#******************************************************************************
evel_install_ubuntu: delivery
	@echo Installing EVEL library on Ubuntu testbed...
	@$(SSH) $(VNF_TESTBED_UBUNTU_USER)@$(VNF_TESTBED_UBUNTU) \
          rm -rf $(TESTBED_UBUNTU_DOWNLOAD_PATH) \; \
          mkdir -p $(TESTBED_UBUNTU_DOWNLOAD_PATH) \; \
          mkdir -p $(TESTBED_UBUNTU_INSTALL_PATH)
	@$(SCP) -r $(CODE_ROOT)/output/evel-library-package.tgz \
           $(VNF_TESTBED_UBUNTU_USER)@$(VNF_TESTBED_UBUNTU):$(TESTBED_UBUNTU_DOWNLOAD_PATH)
	@$(SSH) $(VNF_TESTBED_UBUNTU_USER)@$(VNF_TESTBED_UBUNTU) \
          tar zx --directory $(TESTBED_UBUNTU_INSTALL_PATH) \
                 --file $(TESTBED_UBUNTU_DOWNLOAD_PATH)/evel-library-package.tgz
	@echo Making EVEL library on testbed...
	@$(SSH) $(VNF_TESTBED_UBUNTU_USER)@$(VNF_TESTBED_UBUNTU) \
           cd $(TESTBED_UBUNTU_INSTALL_PATH)/bldjobs \; \
           make clean all

#******************************************************************************
# Make sure that the Centos platform is up to date and then run the software  *
# against a test collector.  Validating correct operation is not presently    *
# automated.                                                                  *
#******************************************************************************
evel_test_centos: evel_install_centos
	@echo Testing EVEL Demo application on CentOS...
	@$(SSH) $(VNF_TESTBED_CENTOS_USER)@$(VNF_TESTBED_CENTOS) \
           source .bash_profile \; \
           $(TESTBED_CENTOS_INSTALL_PATH)/output/x86_$(ARCH)/evel_demo \
                                                 --fqdn $(VNF_COLLECTOR_HOST) \
                                                 --port $(VNF_COLLECTOR_PORT) \
                                                 --verbose

#******************************************************************************
# Make sure that the Ubuntu platform is up to date and then run the software  *
# against a test collector.  Validating correct operation is not presently    *
# automated.                                                                  *
#******************************************************************************
evel_test_ubuntu: evel_install_ubuntu
	@echo Testing EVEL Demo application on Ubuntu...
	@$(SSH) $(VNF_TESTBED_UBUNTU_USER)@$(VNF_TESTBED_UBUNTU) \
           source .profile \; \
           $(TESTBED_UBUNTU_INSTALL_PATH)/output/x86_$(ARCH)/evel_demo \
                                                 --fqdn $(VNF_COLLECTOR_HOST) \
                                                 --port $(VNF_COLLECTOR_PORT) \
                                                 --verbose

#******************************************************************************
# Making a clean delivery has some very specific dependencies which are order *
# dependent, so we recursively make a series of targets to do a clean  build  *
# of all of the required deliverables and then finally zipping up.            *
#******************************************************************************
delivery:
	@$(MAKE) -s delivery_baseline
	@$(MAKE) -s package

delivery_baseline:	docs

#******************************************************************************
# Package the software for delivery.                                          *
#******************************************************************************
package: api_library_clean \
         evel_unit_clean \
         evel_library_demo_clean \
         evel_library_training_clean \
         docs
	@echo Packaging the software for delivery
	@cd $(CODE_ROOT) && tar cfz output/evel-library-package.tgz  bldjobs \
                                                      code \
                                                      docs \
                                                      libs/x86_64/README \
                                                      output/x86_64/README \
                                                      readme.md

package_clean:
	@echo Clean delivery packages
	@$(RM) $(OUTPUTDIR)/*.tgz

#******************************************************************************
# Create project documentation.                                               *
#******************************************************************************
doxygen_docs:
	@echo Making Doxygen documentation
	@$(DOXYGEN) Doxyfile

pdf_docs: doxygen_docs  # This target is slightly broken.  Run manually.
	@echo	Making PDF...
	@$(MAKE) -C $(DOCS_ROOT)/source/evel/latex

docs_clean:
	@echo Cleaning docs...
	@$(RM) $(DOCS_ROOT)/*.svg
	@$(RM) -r $(DOCS_ROOT)/source/evel/html \
	          $(DOCS_ROOT)/source/evel/latex

docs_install: docs
	@echo Copying docs to team web-server...
	@$(SCP) -r $(DOCS_ROOT)/source/evel/html/* \
	          root@$(TEAM_DOCS_SERVER):$(DOCS_SERVER_PATH)
