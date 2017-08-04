
ifndef VM_LINUX_ROOT
VM_LINUX_ROOT := $(word 1, $(subst /vm_linux/,/vm_linux /, $(shell pwd)))
endif

TOOL_CHAIN ?= 4.9.1
ENABLE_VFP ?= true
ENABLE_CA9_NEON ?= true

ifeq "$(ENABLE_VFP)" "true"
		OSS_LIB_ROOT        := $(VM_LINUX_ROOT)/oss/library/gnuarm-$(TOOL_CHAIN)_vfp_ca9
		COMPILER := /mtkoss/gnuarm/vfp_4.8.2_2.6.35_cortex-a9-ubuntu/x86_64/bin/armv7a-mediatek482_001_vfp-linux-gnueabi-
else ifeq "$(ENABLE_CA9_NEON)" "true"
		OSS_LIB_ROOT        := $(VM_LINUX_ROOT)/oss/library/gnuarm-$(TOOL_CHAIN)_neon_ca9
		COMPILER := /mtkoss/gnuarm/hard_4.9.1_armv7a-cros/x86_64/armv7a/bin/armv7a-cros-linux-gnueabi-
else
		OSS_LIB_ROOT        := $(VM_LINUX_ROOT)/oss/library/gnuarm-$(TOOL_CHAIN)
		COMPILER := /mtkoss/gnuarm/soft_4.5.1_2.6.27_arm11-rhel4/i686/bin/armv6z-mediatek451_001_soft-linux-gnueabi-
endif

CC ?= $(COMPILER)gcc
CXX ?= $(COMPILER)g++
LD ?= $(COMPILER)ld


ifeq "$(BUILD_CFG)" "debug"
ICU_CFLAGS += -g
ICU_VERBOSE = "VERBOSE=1"
else
ICU_CFLAGS += -O2
ICU_VERBOSE =
endif

ICU_OUTPUT=$(OSS_LIB_ROOT)/icu/$(ICU_VERSION)

ICU_HOST=arm-linux

#ICU_VERSION ?= 51.1
ICU_VERSION ?= 4.0

OSS_SRC_ROOT:=$(shell pwd)

# to unexport the variable TARGET. This conflict with ICU.
unexport TARGET

ifeq "$(ICU_VERSION)" "4.0"
echo ======ICU build===version==$(ICU_VERSION)
echo ==$(VM_LINUX_ROOT)/oss/source/$(ICU_VERSION)/buildA==
all:
	mkdir -p $(ICU_OUTPUT)
	mkdir -p $(ICU_VERSION)/buildA
	mkdir -p $(ICU_VERSION)/buildB
	chmod +x $(ICU_VERSION)/icu/source/runConfigureICU
	chmod +x $(ICU_VERSION)/icu/source/configure
	chmod +x $(ICU_VERSION)/icu/source/install-sh	
	cd $(ICU_VERSION)/buildA ; \
	sh ../icu/source/runConfigureICU Linux/gcc RANLIB=ranlib AR=ar  --cross-compile-dir=$(VM_LINUX_ROOT)/oss/source/icu/$(ICU_VERSION)/buildA; \
	make
	
	cd $(ICU_VERSION)/buildB; \
	sh ../icu/source/configure CC="$(CC)" CXX="$(CXX)" LD="$(LD)" prefix=$(ICU_OUTPUT) --host=$(ICU_HOST) CFLAGS=$(ICU_CFLAGS) CXXFLAGS=$(ICU_CFLAGS)  --cross-compile-dir=$(VM_LINUX_ROOT)/oss/source/icu/$(ICU_VERSION)/buildA; \
	make ; \
	make install	
clean:
	make -C $(ICU_VERSION)/buildB clean
	make -C $(ICU_VERSION)/buildA clean
endif

#ifeq "$(ICU_VERSION)" "51.1"
#all:
#	mkdir -p $(ICU_OUTPUT)
#	mkdir -p $(ICU_VERSION)/buildA
#	mkdir -p $(ICU_VERSION)/buildB
#	chmod +x $(ICU_VERSION)/icu/source/runConfigureICU
#	chmod +x $(ICU_VERSION)/icu/source/configure
#	chmod +x $(ICU_VERSION)/icu/source/install-sh	
#	cd $(ICU_VERSION)/buildA ; \
#	sh ../icu/source/runConfigureICU Linux/gcc; \
#	make RANLIB=ranlib AR=ar $(ICU_VERBOSE)
#	
#	cd $(ICU_VERSION)/buildB; \
#	sh ../icu/source/configure CC="$(CC)" CXX="$(CXX)" LD="$(LD)" prefix=$(ICU_OUTPUT) --host=$(ICU_HOST) CFLAGS=$(ICU_CFLAGS) CXXFLAGS=$(ICU_CFLAGS) --with-cross-build=$(OSS_SRC_ROOT)/$(ICU_VERSION)/buildA ; \
#	chmod +x ../buildA/bin/icupkg \
#	make ; \
#	make install
#	
#clean:
#	make -C $(ICU_VERSION)/buildB clean
#	make -C $(ICU_VERSION)/buildA clean
#endif

notice:
	cp -rf NOTICE.txt $(OSS_NOTICE_OUTPUT)/icu_NOTICE.txt
	
info:
	cp -rf OSS_INFO.txt $(OSS_INFO_OUTPUT)/icu_INFO.txt

clean:
	echo $@
