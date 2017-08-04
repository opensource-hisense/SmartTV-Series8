BUILD_CFG ?= rel
#SAMBA_VERSION ?= 3.5.6
MEMLEAK ?= false

export THIRDPARTY_ROOT ?= $(word 1, $(subst /third_party/,/third_party /, $(shell pwd)))
THIRDPARTY_SRC_ROOT ?= $(THIRDPARTY_ROOT)/source
THIRDPARTY_LIB_ROOT ?= $(THIRDPARTY_ROOT)/library

ifeq ($(TOOL_CHAIN), x86)
CC = gcc
STRIP = strip
else
include $(THIRDPARTY_SRC_ROOT)/mak/target.mak
endif

THIRD_PARTY = $(THIRDPARTY_ROOT)
THIRD_PARTY_LIB = $(THIRD_PARTY)/library/gnuarm-$(TOOL_CHAIN)$(VFP_SUFFIX)

CC_FLAG += -Wall -Werror -ansi

ifeq ($(BUILD_CFG), debug)
DEFINES += -DDEBUG
CC_FLAG += -g -O0
else
CC_FLAG += -O2
endif

DEFINES += -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64
DEFINES += -D_GNU_SOURCE

#DEFINES += -D_NET_MEMLEAK_DEBUG
#DEFINES += -DDEBUG

ALINK += 
RPATH += /3rd/lib/samba/lib

ifeq ($(strip $(findstring -DDEBUG, $(DEFINES))),)
ifeq ($(BUILD_CFG), debug)
DEFINES += -DDEBUG
endif
endif

ifeq ($(MEMLEAK), true)
DEFINES += -D_NET_MEMLEAK_DEBUG
endif

ifneq ($(strip $(findstring -D_NET_MEMLEAK_DEBUG, $(DEFINES))),)
DEFINES += -I./memleak -L./memleak
ALINK += -lmemleak
endif

.PHONY: all clean install
ifeq ($(MEMLEAK), true)
all: ../memleak/libmemleak.so libbc.so
else
all: libbc.so
endif
ifeq ($(strip $(findstring -DDEBUG, $(DEFINES))),)
ifneq "$(STRIP)" ""
	$(STRIP) $^
endif
endif

libbc.so: atom_strdup.o bc_block_cache.o bc_dlist.o bc_file_index.o bc_free_block.o bc_hash.o
	$(CC) $(CC_FLAG) -shared $(DEFINES) -o $@ $^

%.o: %.c
	$(CC) -c $(CC_FLAG) $(DEFINES) $< -o $@

install: all
	cp libbc.so ../

clean:
	rm -rf *.o libbc.so


