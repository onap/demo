CC=gcc

ARCH=$(shell getconf LONG_BIT)
CODE_ROOT=$(CURDIR)/../../..
LIBS_DIR=$(CODE_ROOT)/libs/x86_$(ARCH)
INCLUDE_DIR=$(CODE_ROOT)/code/evel_library
EVEL_DEMO=$(CURDIR)/../../evel_demo
TEST_CONTROL=$(EVEL_DEMO)/evel_test_control.c

#******************************************************************************
# Standard compiler flags.                                                    *
#******************************************************************************
CPPFLAGS=
CFLAGS=-Wall -g -fPIC

all:     hello_evel_world

hello_evel_world: hello_evel_world.c $(TEST_CONTROL)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o hello_evel_world \
	                            -L $(LIBS_DIR) \
	                            -I $(INCLUDE_DIR) \
	                            -I $(EVEL_DEMO) \
                               hello_evel_world.c \
                               $(TEST_CONTROL) \
                              -lpthread \
                              -level \
                              -lcurl

#******************************************************************************
# Configure the vel_username and vel_password to
# vel_username = username
# vel_password = password
#******************************************************************************
run: all
	./hello_evel_world 127.0.0.1 30000 username password
