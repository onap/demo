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

all:     hello_evel_world

hello_evel_world: hello_evel_world.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -o hello_evel_world \
	                            -L $(LIBS_DIR) \
	                            -I $(INCLUDE_DIR) \
                               hello_evel_world.c \
                              -lpthread \
                              -level \
                              -lcurl

#******************************************************************************
# Configure the vel_username and vel_password to
# vel_username = username
# vel_password = password
#******************************************************************************
run: all
	./hello_evel_world localhost 30000 username password
