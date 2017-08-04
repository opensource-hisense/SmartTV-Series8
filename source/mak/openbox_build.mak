include openbox_inc.mak


CFLAGS=


ifeq "$(TOOL_CHAIN)" "4.8.2"
    ifeq "$(ENABLE_CA9)" "true"
        export TOOL_CHAIN_ROOT = /mtkoss/gnuarm/vfp_4.8.2_2.6.35_cortex-a9-ubuntu/x86_64
        export TOOL_CHAIN_BIN_PATH				:= $(TOOL_CHAIN_ROOT)/bin
        export CROSS_COMPILE := $(TOOL_CHAIN_BIN_PATH)/armv7a-mediatek482_001_vfp-linux-gnueabi-

        CFLAGS += -march=armv7-a -mtune=cortex-a9 -mfloat-abi=softfp -mfpu=vfpv3-d16 -fPIC
    else ifeq "$(ENABLE_CA9_NEON)" "true"
        export TOOL_CHAIN_ROOT = /mtkoss/gnuarm/neon_4.8.2_2.6.35_cortex-a9-ubuntu/x86_64
        export TOOL_CHAIN_BIN_PATH				:= $(TOOL_CHAIN_ROOT)/bin
        export CROSS_COMPILE := $(TOOL_CHAIN_BIN_PATH)/armv7a-mediatek482_001_neon-linux-gnueabi-

        CFLAGS += -march=armv7-a -mtune=cortex-a9 -mfloat-abi=softfp -mfpu=neon-vfpv4 -fPIC
    else ifeq "$(ENABLE_CA15_NEON)" "true"
        export TOOL_CHAIN_ROOT = /mtkoss/gnuarm/neon_4.8.2_2.6.35_cortex-a15-ubuntu/x86_64
        export TOOL_CHAIN_BIN_PATH				:= $(TOOL_CHAIN_ROOT)/bin
        export CROSS_COMPILE := $(TOOL_CHAIN_BIN_PATH)/armv7a-mediatek482_001_neon-linux-gnueabi-

        CFLAGS += -march=armv7-a -mtune=cortex-a15 -mfloat-abi=hard -mfpu=neon-vfpv4 -fPIC       
    else
        $(error "ENABLE_CA9" or "ENABLE_CA9_NEON" must be set to true)
    endif
#else ifeq "$(TOOL_CHAIN)" "4.5.1"
#    ifeq "$(ENABLE_CA9)" "true"
#        #export TOOL_CHAIN_ROOT = /mtkoss/gnuarm/vfp_4.8.2_2.6.35_cortex-a9-ubuntu/x86_64
#        export TOOL_CHAIN_ROOT = /mtkoss/gnuarm/vfp_4.5.1_2.6.27_cortex-a9-rhel4/i686
#
#        export TOOL_CHAIN_BIN_PATH				:= $(TOOL_CHAIN_ROOT)/bin
#
#
#        #export CROSS_COMPILE := $(TOOL_CHAIN_BIN_PATH)/armv7a-mediatek482_001_vfp-linux-gnueabi-
#        export CROSS_COMPILE := $(TOOL_CHAIN_BIN_PATH)/armv7a-mediatek451_001_vfp-linux-gnueabi-
#
#        CFLAGS += -march=armv7-a -mtune=cortex-a9 -mfloat-abi=softfp -mfpu=vfpv3-d16 -fPIC
#    endif
else ifeq "$(TOOL_CHAIN)" "4.9.1"
    ifeq "$(ENABLE_CA9_NEON)" "true"
        export TOOL_CHAIN_ROOT = /mtkoss/gnuarm/hard_4.9.1_armv7a-cros/x86_64/armv7a
        export TOOL_CHAIN_BIN_PATH				:= $(TOOL_CHAIN_ROOT)/bin
        export CROSS_COMPILE := $(TOOL_CHAIN_BIN_PATH)/armv7a-cros-linux-gnueabi-
        
        CFLAGS += -march=armv7-a -mtune=cortex-a9 -mfloat-abi=hard -mfpu=neon-vfpv4 -fPIC
    endif
else
    $(error TOOL_CHAIN=$(TOOL_CHAIN) is not support support)
endif


export CC=${CROSS_COMPILE}gcc
export AR=${CROSS_COMPILE}ar
export LD=${CROSS_COMPILE}ld
export STRIP=${CROSS_COMPILE}strip
export CXX=${CROSS_COMPILE}g++

OPEN_SSL_INC_PATH=$(OSS_LIB_ROOT)/openssl/$(OPENSSL_VERSION)/include
OPEN_SSL_LIB_PATH=$(OSS_LIB_ROOT)/openssl/$(OPENSSL_VERSION)/lib

OPENSSL_LIB=$(OPEN_SSL_LIB_PATH)/libresolv.so \
            $(OPEN_SSL_LIB_PATH)/libcrypto.so

CFLAGS += -D_GNU_SOURCE
CFLAGS += -I$(OPEN_SSL_INC_PATH)

OPENSSL_LDFLAGS=-lresolv -L$(OPEN_SSL_LIB_PATH) -lcrypto
export OPENSSL_LDFLAGS
#export CFLAGS

export IPUTILS_CFLAGS:= $(CFLAGS)


NET_TOOLS_PROGS = ifconfig hostname arp netstat route rarp slattach plipconfig nameif
IPUTIL_TARGETS=tracepath ping clockdiff rdisc arping tftpd rarpd tracepath6 traceroute6 ping6
UDHCP_PROGS=udhcpc

.PHONY: all bash coreutils findutils gawk grep gzip inetutils module_init_tools ncurses procps psmisc sed tar util-linux-ng net-tools iputils udhcp

all: bash coreutils findutils gawk grep gzip inetutils module_init_tools ncurses procps sed tar util-linux-ng net-tools iputils udhcp #psmisc

bash:                                     
	cd ${BASH_SRC_BUILD_PATH}; \
	./configure --prefix=${BASH_INSTALL_PATH} --host=arm-linux
	make -C ${BASH_SRC_BUILD_PATH} #CFLAGS=$(CFLAGS)
	make -C ${BASH_SRC_BUILD_PATH} install


coreutils:
	cd ${COREUTIL_SRC_BUILD_PATH}; \
	./configure --prefix=${COREUTIL_INSTALL_PATH} --host=armv7a-mediatek451_001_vfp-linux-gnueabi --enable-install-program=df
	make -C ${COREUTIL_SRC_BUILD_PATH}
	make -C ${COREUTIL_SRC_BUILD_PATH} install
	
findutils:
	cd ${FINDUTIL_SRC_BUILD_PATH}; \
	./configure --prefix=${FINDUTIL_INSTALL_PATH} --host=arm-linux 
	make -C ${FINDUTIL_SRC_BUILD_PATH}
	make -C ${FINDUTIL_SRC_BUILD_PATH} install

gawk:
	cd ${GAWK_SRC_BUILD_PATH}; \
	./configure --prefix=${GAWK_INSTALL_PATH} --host=arm-linux 
	make -C ${GAWK_SRC_BUILD_PATH}
	make -C ${GAWK_SRC_BUILD_PATH} install

grep:
	cd ${GREP_SRC_BUILD_PATH}; \
	./configure --prefix=${GREP_INSTALL_PATH} --host=arm-linux --with-gnu-ld=yes --with-included-regex=no
	make -C ${GREP_SRC_BUILD_PATH}
	make -C ${GREP_SRC_BUILD_PATH} install
	
	
gzip:
	cd ${GZIP_SRC_BUILD_PATH}; \
	./configure --prefix=${GZIP_INSTALL_PATH} --host=arm-linux 
	make -C ${GZIP_SRC_BUILD_PATH}
	make -C ${GZIP_SRC_BUILD_PATH} install	
	
inetutils:
	cd ${INETUTIL_SRC_BUILD_PATH}; \
	./configure --prefix=${INETUTIL_INSTALL_PATH} --host=arm-linux --disable-ftp --disable-ftpd --disable-rexecd \
	--disable-rshd --disable-syslogd --disable-talkd --disable-uucpd --disable-rlogin --disable-rsh --disable-logger --disable-talk \
	--disable-whois \
	--enable-telnetd \
    --enable-ifconfig=yes
	make -C ${INETUTIL_SRC_BUILD_PATH}
	make -C ${INETUTIL_SRC_BUILD_PATH} install
	cp ${INETUTIL_SRC_BUILD_PATH}/telnetd/telnetd ${INETUTIL_INSTALL_PATH}/bin/telnetd
	
module_init_tools:
	cd ${MODULE_INIT_SRC_BUILD_PATH}; \
	./configure --prefix=${MODULE_INIT_INSTALL_PATH} --host=arm-linux 
	make -C ${MODULE_INIT_SRC_BUILD_PATH}
	make -C ${MODULE_INIT_SRC_BUILD_PATH} install		
	
ncurses:
	cd ${NCURSES_SRC_BUILD_PATH}; \
	./configure --prefix=${NCURSES_INSTALL_PATH} --host=arm-linux 
	make -C ${NCURSES_SRC_BUILD_PATH}
	make -C ${NCURSES_SRC_BUILD_PATH} install	
	
procps:
	cd ${PROCPS_SRC_BUILD_PATH}; \
	mkdir -p ${PROCPS_INSTALL_PATH}/lib
	mkdir -p ${PROCPS_INSTALL_PATH}/bin
	make -C ${PROCPS_SRC_BUILD_PATH} SHARED=0 DESTDIR=${PROCPS_INSTALL_PATH} NCURSES_PATH=${NCURSES_INSTALL_PATH}
	make -C ${PROCPS_SRC_BUILD_PATH} SHARED=0 DESTDIR=${PROCPS_INSTALL_PATH} NCURSES_PATH=${NCURSES_INSTALL_PATH} install		
	
psmisc:
	cd ${PSMISC_SRC_BUILD_PATH}; \
	./configure --prefix=${PSMISC_INSTALL_PATH} --host=arm-linux 
	make -C ${PSMISC_SRC_BUILD_PATH}
	make -C ${PSMISC_SRC_BUILD_PATH} install

sed:                                     
	cd ${SED_SRC_BUILD_PATH}; \
	./configure --prefix=${SED_INSTALL_PATH} --host=arm-linux 
	make -C ${SED_SRC_BUILD_PATH}
	make -C ${SED_SRC_BUILD_PATH} install
	
tar:                                     
	cd ${TAR_SRC_BUILD_PATH}; \
	./configure --prefix=${TAR_INSTALL_PATH} --host=arm-linux
	#LDFLAGS=-static ./configure --prefix=${TAR_INSTALL_PATH} --host=arm-linux --without-included-regex
	make -C ${TAR_SRC_BUILD_PATH}
	make -C ${TAR_SRC_BUILD_PATH} install

util-linux-ng:
	cd ${UTIL_LINUX_NG_SRC_BUILD_PATH}; \
	./configure --prefix=${UTIL_LINUX_NG_INSTALL_PATH} --host=arm-linux  --enable-static --enable-login-utils --without-ncurses --disable-use-tty-group \
	--enable-static-programs=mount,umount,blkid
	make -C ${UTIL_LINUX_NG_SRC_BUILD_PATH}
	make -C ${UTIL_LINUX_NG_SRC_BUILD_PATH} install
	
	
net-tools:                                     
	make -C ${NET_TOOLS_SRC_BUILD_PATH} CC=$(CC) AR=$(AR) #CFLAGS=$(CFLAGS)
	mkdir -p $(NET_TOOLS_INSTALL_PATH)/bin
	for i in $(NET_TOOLS_PROGS); do \
	cp ${NET_TOOLS_SRC_BUILD_PATH}/$$i $(NET_TOOLS_INSTALL_PATH)/bin/$$i; \
	done
	
	OPEN_SSL_LIB_PATH=$(OSS_LIB_ROOT)/openssl/$(OPENSSL_VERSION)/lib

iputils:                                     
	mkdir -p ${IPUTILS_INSTALL_PATH}/bin
	make -C ${IPUTILS_SRC_BUILD_PATH}
	#make -C ${IPUTILS_SRC_BUILD_PATH} # -lcrypto#CFLAGS=$(CFLAGS)
	for i in $(IPUTIL_TARGETS); do \
	cp ${IPUTILS_SRC_BUILD_PATH}/$$i $(IPUTILS_INSTALL_PATH)/bin/$$i; \
	done

udhcp:
	mkdir -p $(UDHCP_INSTALL_PATH)/bin
	make -C ${UDHCP_SRC_BUILD_PATH} #CC=$(CC) LD=$(LD) #CFLAGS=$(CFLAGS)
	for i in $(UDHCP_PROGS); do \
	cp ${UDHCP_SRC_BUILD_PATH}/$$i $(UDHCP_INSTALL_PATH)/bin/$$i; \
	done




	
	
	






















