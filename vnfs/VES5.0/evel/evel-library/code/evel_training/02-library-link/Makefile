CC=gcc

#******************************************************************************
# Standard compiler flags.                                                    *
#******************************************************************************
CPPFLAGS=
CFLAGS=-Wall -g -fPIC

ARCH=$(shell getconf LONG_BIT)
CODE_ROOT=$(CURDIR)/../../..
LIBS_DIR=$(CODE_ROOT)/libs/x86_$(ARCH)

all:     hello_evel_world

hello_evel_world: hello_evel_world.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -o hello_evel_world \
	                            -L $(LIBS_DIR) \
                               hello_evel_world.c \
                              -lpthread \
                              -level \
                              -lcurl

