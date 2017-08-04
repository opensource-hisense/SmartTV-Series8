
GLIBC_ROOT=$(shell pwd)

ifndef OSS_ROOT
OSS_ROOT ?= $(word 1, $(subst /oss/,/oss /, $(shell pwd)))
endif

ifndef THIRDPARTY_ROOT
THIRDPARTY_ROOT := $(OSS_ROOT)/../third_party
endif

ifndef DTV_LINUX_ROOT
DTV_LINUX_ROOT := $(OSS_ROOT)/../dtv_linux
endif


ifndef KERNEL_VER
KERNEL_VER := linux-2.6.27
endif

ifeq "$(KERNEL_VER)" "linux-2.6.27"
KERNEL_VER_NUM = 2.6.27
endif

ifeq "$(KERNEL_VER)" "linux-2.6.35"
KERNEL_VER_NUM = 2.6.35
endif

ifndef TOOL_CHAIN
export TOOL_CHAIN = 4.4.2
endif

include $(DTV_LINUX_ROOT)/mak/common_inc.mak

ifndef TOOL_CHAIN_BIN_PATH
TOOL_CHAIN_BIN_PATH = /usr/local/mtk-toolchain/gnuarm-4.4.2/cross-tools/arm11/bin
endif

ifndef CROSS_COMPILE
CROSS_COMPILE = ${TOOL_CHAIN_BIN_PATH}/armv6z-mediatek442_001-linux-gnueabi-
endif

ifndef KERNEL_ROOT
KERNEL_ROOT=$(THIRDPARTY_ROOT)/../chiling/kernel/$(KERNEL_VER)
endif

LD_LIBRARY_PATH =
MAKE = make
MAKEFLAG = -j4


CLFS_HOST=i686-pc-linux-gnu
CLFS_TARGET=armv6z-mediatek-linux-gnueabi
CLFS_ROOT=${GLIBC_ROOT}/sysroot

ifndef GLIBC_VERSION
ifeq "$(TOOL_CHAIN)" "4.5.1"
GLIBC_VERSION = 2.12.2
else
GLIBC_VERSION = 2.9
endif
endif

GLIBC_FOLDER = glibc-${GLIBC_VERSION}
GLIBC_BUILD  = glibc-build
GLIBC_SOURCE = glibc-source
GLIBC_FILE = ${GLIBC_ROOT}/${GLIBC_VERSION}/glibc-${GLIBC_VERSION}.tar.bz2
GLIBC_PORTS_FILE = ${GLIBC_ROOT}/${GLIBC_VERSION}/glibc-ports-${GLIBC_VERSION}.tar.bz2

GLIBC_PORTS_FOLDER = ports
GLIBC_PORTS_ORIG_FOLDER = glibc-ports-${GLIBC_VERSION}

ifeq "$(ENABLE_VFP)" "true"
GCC_FP_OPTION = -mfpu=vfp -mfloat-abi=softfp
endif
#


ifeq "$(ENABLE_CA9)" "true"
GLIBC_CFLAGS = -march=armv7-a -mtune=cortex-a9 -mfloat-abi=softfp -mfpu=vfpv3-d16
CLFS_TARGET=armv7a-mediatek-linux-gnueabi
else
GLIBC_CFLAGS = -march=armv6z -mtune=arm1176jz-s -mthumb-interwork ${GCC_FP_OPTION}
CLFS_TARGET=armv6z-mediatek-linux-gnueabi
endif

ifeq "$(BUILD_CFG)" "debug"
GLIBC_CFLAGS += -g -O0
endif

.PHONY: linux_headers glibc_prepare glibc_cross

all: glibc_cross


linux_headers:
	cd ${KERNEL_ROOT} && \
	$(MAKE) headers_install ARCH=arm INSTALL_HDR_PATH=${CLFS_ROOT}/usr


glibc_prepare: linux_headers
	if [ -d ${GLIBC_SOURCE} -a -d ${GLIBC_BUILD} ]; then \
	rm -fr ${GLIBC_SOURCE}; \
	rm -fr ${GLIBC_BUILD}; \
	fi 
	mkdir -p ${GLIBC_SOURCE}
	tar jxf ${GLIBC_FILE} -C ${GLIBC_SOURCE}
	cd ${GLIBC_SOURCE}/glibc-${GLIBC_VERSION} && \
	tar jxf ${GLIBC_PORTS_FILE} && mv ${GLIBC_PORTS_ORIG_FOLDER} ${GLIBC_PORTS_FOLDER} && \
	for i in $(shell ls ${GLIBC_ROOT}/${GLIBC_VERSION}/*patch); \
	do \
 		 patch -Np1 -i $$i; \
	done 	
	if [ ! -d ${GLIBC_ROOT}/gcc ]; then \
		tar -zxvf ${GLIBC_ROOT}/gcc.tgz -C ${GLIBC_ROOT} ; \
	fi 
	
glibc_cross: glibc_prepare
	if [ ! -d ${GLIBC_BUILD} ]; then \
	    mkdir -v ${GLIBC_BUILD}	; \
	fi 
	cd ${GLIBC_BUILD} && \
	echo "libc_cv_forced_unwind=yes" > config.cache && \
	echo "libc_cv_c_cleanup=yes" >> config.cache && \
	echo "libc_cv_arm_tls=yes" >> config.cache && \
	echo "libc_cv_gnu89_inline=yes" >> config.cache && \
	echo "libc_cv_slibdir=/lib" >> config.cache && \
	echo "install_root=${CLFS_ROOT}" > configparms && \
	echo "libdir=/usr/lib" >> configparms && \
	BUILD_CC=gcc CC="${CROSS_COMPILE}gcc ${GLIBC_CFLAGS}" AR="${CROSS_COMPILE}ar rus" RANLIB="${CROSS_COMPILE}ranlib" \
	../${GLIBC_SOURCE}/glibc-${GLIBC_VERSION}/configure \
		--prefix=/usr 				\
		--build=${CLFS_HOST} 			\
		--host=${CLFS_TARGET} 			\
		--target=${CLFS_TARGET} 		\
		--with-headers=${CLFS_ROOT}/usr/include \
		--with-binutils=${TOOL_CHAIN_BIN_PATH} 	\
		--with-tls 				\
		--with-__thread 			\
		--enable-sim 				\
		--enable-nptl 				\
		--enable-add-ons 			\
		--enable-kernel=${KERNEL_VER_NUM} 			\
		--disable-profile 			\
		--without-gd 				\
		--without-fp 				\
		--without-cvs 				\
		--cache-file=config.cache
	${MAKE} ${MAKEFLAG} -C ${GLIBC_BUILD}
	${MAKE} -C ${GLIBC_BUILD} install	
ifdef GLIBC_INSTALL_PATH
	cp ${CLFS_ROOT}/lib/libc-${GLIBC_VERSION}.so  ${GLIBC_INSTALL_PATH}
endif
	
clean:
	rm -rf ${CLFS_ROOT}
	rm -rf ${GLIBC_BUILD}
	rm -rf ${GLIBC_SOURCE}

notice:
	cp -rf NOTICE.$(GLIBC_VERSION).txt $(OSS_NOTICE_OUTPUT)/glibc_NOTICE.txt
	
info:
	cp -rf OSS_INFO.$(GLIBC_VERSION).txt $(OSS_INFO_OUTPUT)/glibc_INFO.txt

