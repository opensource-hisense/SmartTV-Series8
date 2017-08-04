export OSS_ROOT ?= $(word 1, $(subst /oss/,/oss /, $(shell pwd)))
OSS_SRC_ROOT ?= $(OSS_ROOT)/source
OSS_LIB_ROOT ?= $(OSS_ROOT)/library

ifeq ($(TOOL_CHAIN), x86)
CC = gcc
STRIP = strip
else
include $(OSS_SRC_ROOT)/mak/target.mak
endif

CC_FLAG += -Wall -Werror -ansi

DEFINES += -D_GNU_SOURCE
DEFINES += -DDEBUG
DEFINES += -D_NET_MEMLEAK_DEBUG
ifeq ($(MEMLEAK_HOOK), true)
DEFINES += -DNET_MEMLEAK_CK_ALL_MEMORY_HOOK
endif

CC_FLAG += -g -Wall -O0 -fPIC

.PHONY: all clean install
all: libmemleak.so

libmemleak.so: atom_str.o net_memleak.o memlist.o
	$(CC) $(CC_FLAG) -shared -fPIC $(DEFINES) -o $@ $^

%.o: %.c
	$(CC) -c $(CC_FLAG) $(DEFINES) $< -o $@

install: all
	cp libmemleak.so ../

clean:
	rm -rf *.o *.so


