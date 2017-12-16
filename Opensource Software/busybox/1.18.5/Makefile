##########################################################################
#
#  Copyright (c) 2004, CrystalMedia Technology, Inc.
#  All rights reserved
#
#  Unauthorized use, practice, perform, copy, distribution, reproduction,
#  or disclosure of this information in whole or in part is prohibited.
###########################################################################
# $RCSfile: $
# $Revision: #1 $
# $Date: 2012/06/19 $
# $Author:  $
# $MD5HEX:  $
#
# Description:
#
#############################################################################

CROSS_COMPILE =		/mtkoss/gnuarm/vfp_4.5.1_2.6.27_cortex-a9-rhel4/i686/bin/armv7a-mediatek451_001_vfp-linux-gnueabi-
BUILD_DIR          ?= $(shell pwd)

export CC     = $(CROSS_COMPILE)gcc
export AR     = $(CROSS_COMPILE)ar
export AS     = $(CROSS_COMPILE)as
export STRIP  = $(CROSS_COMPILE)strip
export RANLIB = $(CROSS_COMPILE)ranlib

TAR         = /bin/tar
RM          = /bin/rm
RM_FLAG     = -fr
CP          = /bin/cp
CP_FLAG     = -f
CP_ALL_FLAG = -rf
MAKE        = make
MKDIR       = mkdir
MKDIR_FLAG  = -p 
MV          = /bin/mv

VFP_CFLAGS  =-mfloat-abi=softfp -mfpu=vfp

BUSYBOX_VER               ?= 1.18.5
BUSYBOX_FILE              ?= busybox-$(BUSYBOX_VER).tar.bz2
BUSYBOX_FOLDER            ?= busybox-$(BUSYBOX_VER)
BUSYBOX_DECOMPRESS_FLAG   ?= xjf
ifeq "$(ENABLE_FULL)" "true"
BUSYBOX_CFG_FILE          ?= busybox-config-android-full
else
BUSYBOX_CFG_FILE          ?= busybox-config-android-default
endif
BUSYBOX_PATCH_FOLDER      ?= busybox-$(BUSYBOX_VER)_patch
SUBARCH                   ?= arm

BUILD_TMPDIR = $(BUILD_DIR)/$(notdir $(BUSYBOX_CFG_FILE)build)

define check_build_dir
if [ ! -d $(BUILD_TMPDIR) ]; then mkdir -p $(BUILD_TMPDIR); fi
endef

all_phony := busybox

.PHONY: $(all_phony) $(addprefix clean_,$(all_phony))

all: $(all_phony)

clean_all: $(addprefix clean_,$(all_phony))
	$(RM) $(RM_FLAG) $(BUILD_TMPDIR)
	$(RM) $(RM_FLAG) $(_PREFIX)
	
clean_busybox:
	$(RM) $(RM_FLAG) $(BUILD_TMPDIR)

busybox:
ifeq "$(SUBARCH)" ""
	@echo "SUBARCH = NULL(could be i386/arm/sparc64/s390/parisc/powerpc/mips), e.g. 'make busybox SUBARCH=arm', exit now!"
	@exit 1
endif
	@$(check_build_dir)
	$(TAR) $(BUSYBOX_DECOMPRESS_FLAG) $(BUSYBOX_FILE) -C $(BUILD_TMPDIR)
	$(CP) $(CP_FLAG) $(BUSYBOX_CFG_FILE) $(BUILD_TMPDIR)/$(BUSYBOX_FOLDER)/.config
	$(CP) $(CP_ALL_FLAG) $(BUSYBOX_PATCH_FOLDER)/* $(BUILD_TMPDIR)/$(BUSYBOX_FOLDER)/
	$(if $(filter $(ENABLE_VFP),true),CPPFLAGS="$(VFP_CFLAGS)" CFLAGS="$(VFP_CFLAGS)",) \
	$(MAKE) -j 20 all     -C $(BUILD_TMPDIR)/$(BUSYBOX_FOLDER) SUBARCH=$(SUBARCH) CROSS_COMPILE=$(CROSS_COMPILE)
