BUILD_CFG ?= rel
#SAMBA_VERSION ?= 3.5.6
MEMLEAK ?= false

export OSS_ROOT ?= $(word 1, $(subst /oss/,/oss /, $(shell pwd)))
OSS_SRC_ROOT ?= $(OSS_ROOT)/source
OSS_LIB_ROOT ?= $(OSS_ROOT)/library

ifeq ($(TOOL_CHAIN), x86)
CC = gcc
STRIP = strip
else
include $(OSS_SRC_ROOT)/mak/target.mak
endif

CC_FLAG += -Wall -Werror -Wno-unused-but-set-variable -ansi

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
all: memleak/libmemleak.so libsmb_mw.so smb_server libsmb_rpc.so tclt tccp tmem tstatus
else
all: libsmb_mw.so smb_server libsmb_rpc.so tclt tccp tmem tstatus
endif
ifeq ($(strip $(findstring -DDEBUG, $(DEFINES))),)
ifneq "$(STRIP)" ""
	$(STRIP) $^
endif
endif

smb_server: smb_server.o libsmb_mw.so
	$(CC) $(CC_FLAG) $(DEFINES) -o $@ $^ -L./ libsmb_mw.so $(ALINK) -lpthread -ldl -lrt -Wl,-rpath=$(RPATH)

memleak/libmemleak.so:
	make -C ./memleak

tccp: tccp.o
	$(CC) $(CC_FLAG) $(DEFINES) -o $@ $^ -L./ libsmb_rpc.so libsmb_mw.so $(ALINK) -lpthread -lrt -Wl,-rpath=$(RPATH)

tclt: tclt.o
	$(CC) $(CC_FLAG) $(DEFINES) -o $@ $^ -L./ libsmb_rpc.so libsmb_mw.so $(ALINK) -lpthread -lrt -Wl,-rpath=$(RPATH)

tmem: tmem.o
	$(CC) $(CC_FLAG) $(DEFINES) -o $@ $^ -L./ libsmb_rpc.so libsmb_mw.so $(ALINK) -lpthread -lrt -Wl,-rpath=$(RPATH)

m_test: m_test.o
	$(CC) $(CC_FLAG) $(DEFINES) -o $@ $^ -L./ libsmb_mw.so $(ALINK) -lpthread -lrt

tstatus: tstatus.o
	$(CC) $(CC_FLAG) $(DEFINES) -o $@ $^ -L./ libsmb_rpc.so libsmb_mw.so $(ALINK) -lpthread -lrt -Wl,-rpath=$(RPATH)

libsmb_rpc.so: smb_rpc.o
	$(CC) $(CC_FLAG) -shared $(DEFINES) -o $@ $^ $(ALINK)

libsmb_mw.so: mw_server.o mw_client.o mw_util.o mylist.o
	$(CC) $(CC_FLAG) -shared $(DEFINES) -o $@ $^ $(ALINK)

%.o: %.c
	$(CC) -c $(CC_FLAG) $(DEFINES) $< -o $@

install: all
	cp -f smb_server tstatus tccp tclt ../../../library/gnuarm-$(TOOL_CHAIN)$(VFP_SUFFIX)/samba/$(SAMBA_VERSION)/bin/
	cp -f libsmb_mw.so libsmb_rpc.so ../../../library/gnuarm-$(TOOL_CHAIN)$(VFP_SUFFIX)/samba/$(SAMBA_VERSION)/lib/

clean:
	rm -rf *.o libsmb_mw.so libsmb_rpc.so tclt tccp tmem m_test tstatus smb_server
	make -C ./memleak clean


