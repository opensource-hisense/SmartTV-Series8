##########################################################################
#
#  Copyright (c) 2004, CrystalMedia Technology, Inc.
#  All rights reserved
#
#  Unauthorized use, practice, perform, copy, distribution, reproduction,
#  or disclosure of this information in whole or in part is prohibited.
###########################################################################
# $RCSfile: customer.mak,v $
# $Revision: #7 $
# $Date: 2012/03/28 $
# $Author: ryanhh.lee $
# $MD5HEX: 61118bc277d8ac078b83c14a0d509c1a $
#
# Description:
#         Makefile to build a "MT-5351" demo board build. The following
#         targets are supported:
#
#############################################################################

CROSS_COMPILE      ?= armv6z-mediatek-linux-gnueabi-
CROSS_TARGET       ?= armv6z-mediatek-linux-gnueabi
CROSS_HOST         ?= i686-pc-linux-gnu
INSTALL_DIR        ?= $(shell pwd)
BUILD_DIR          ?= $(shell pwd)
TARGET_INSTALL_DIR ?= $(shell pwd)/install

CCACHE_BIN := /mtkoss/ccache/3.1.4-rhel-4.8/i686/bin/ccache
CROSS_COMPILE := $(filter-out $(CCACHE_BIN),$(CROSS_COMPILE))

ifeq "$(INSTALL_DIR)" "$(shell pwd)"
_PREFIX=$(INSTALL_DIR)/$(notdir $(CROSS_COMPILE)install)
else
_PREFIX=$(INSTALL_DIR)
endif

ifeq "$(install-alone)" "true"
PREFIX = $(_PREFIX)/$@
else
PREFIX = $(_PREFIX)
endif

BUILD_TMPDIR = $(BUILD_DIR)/$(notdir $(CROSS_COMPILE)build)


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

ifeq "$(TOOL_CHAIN)" "4.9.1"
VFP_CFLAGS  =-mfloat-abi=hard -mfpu=vfp
else
VFP_CFLAGS  =-mfloat-abi=softfp -mfpu=vfp
endif

ZLIB_VER                  ?= 1.2.3
ZLIB_FILE                 ?= zlib-$(ZLIB_VER).tar.gz
ZLIB_FOLDER               ?= zlib-$(ZLIB_VER)
ZLIB_DECOMPRESS_FLAG      ?= zxf
                          
PNG_VER                   ?= 1.2.30
PNG_FILE                  ?= libpng-$(PNG_VER).tar.bz2
PNG_FOLDER                ?= libpng-$(PNG_VER)
PNG_DECOMPRESS_FLAG       ?= jxf
                          
JPEG_VER                  ?= 6b
JPEG_FILE                 ?= jpegsrc.v6b.tar.gz
JPEG_PATCHES              ?= *.patch
JPEG_FOLDER               ?= jpeg-$(JPEG_VER)
JPEG_DECOMPRESS_FLAG      ?= zxf
                          
FT_VER                    ?= 2.3.7
FT_FILE                   ?= freetype-$(FT_VER).tar.gz
FT_FOLDER                 ?= freetype-$(FT_VER)
FT_DECOMPRESS_FLAG        ?= zxf

BUSYBOX_VER               ?= 1.15.3
BUSYBOX_FILE              ?= busybox-$(BUSYBOX_VER).tar.gz
BUSYBOX_FOLDER            ?= busybox-$(BUSYBOX_VER)
BUSYBOX_DECOMPRESS_FLAG   ?= zxf
BUSYBOX_CFG_FILE          ?= busybox-config-default
SUBARCH                   ?= arm


define check_build_dir
if [ ! -d $(BUILD_TMPDIR) ]; then mkdir -p $(BUILD_TMPDIR); fi
endef

all_phony := zlib png jpeg freetype busybox

.PHONY: $(all_phony) $(addprefix clean_,$(all_phony))

all: $(all_phony)

clean_all: $(addprefix clean_,$(all_phony))
	$(RM) $(RM_FLAG) $(BUILD_TMPDIR)
	$(RM) $(RM_FLAG) $(_PREFIX)
	
clean_freetype:
	$(RM) $(RM_FLAG) $(BUILD_TMPDIR)

freetype:
	$(check_build_dir)
	$(TAR) $(FT_DECOMPRESS_FLAG) $(FT_FILE) -C $(BUILD_TMPDIR)
	cd $(BUILD_TMPDIR)/$(FT_FOLDER) && \
	$(if $(filter $(ENABLE_VFP),true),CPPFLAGS="$(VFP_CFLAGS)" CFLAGS="$(VFP_CFLAGS)",) \
	CC="$(CC)" RANLIB="$(RANLIB)" \
	   ./configure --prefix=$(PREFIX) --host=$(CROSS_HOST) --target=$(CROSS_TARGET) \
	   --enable-shared --enable-static && \
	$(MAKE) && \
	$(MAKE) install

	
install_freetype:
	echo 'freetype installion '
	$(RM)    $(RM_FLAG)    $(TARGET_INSTALL_DIR)/$(FT_VER)/pre-install
	$(MKDIR) $(MKDIR_FLAG) $(TARGET_INSTALL_DIR)/$(FT_VER)/pre-install/static_lib
	$(CP) $(CP_ALL_FLAG) $(PREFIX)/* $(TARGET_INSTALL_DIR)/$(FT_VER)/pre-install
	$(MV) $(TARGET_INSTALL_DIR)/$(FT_VER)/pre-install/lib/*.a  $(TARGET_INSTALL_DIR)/$(FT_VER)/pre-install/static_lib
	$(MV) $(TARGET_INSTALL_DIR)/$(FT_VER)/pre-install/lib/*.la $(TARGET_INSTALL_DIR)/$(FT_VER)/pre-install/static_lib
	

clean_png:
	$(RM) $(RM_FLAG) $(BUILD_TMPDIR)/$(PNG_FOLDER)

png:
	$(check_build_dir)
ifeq "$(ZLIB_DIR)" ""
	@echo "ZLIB_DIR = NULL, please type 'make ... ZLIB_DIR=/.../...', exit now!"
	@exit 1
endif
	$(TAR) $(PNG_DECOMPRESS_FLAG) $(PNG_FILE) -C $(BUILD_TMPDIR)
	cd $(BUILD_TMPDIR)/$(PNG_FOLDER) && \
	$(if $(filter $(ENABLE_VFP),true),CPPFLAGS="$(VFP_CFLAGS)" CFLAGS="$(VFP_CFLAGS) -I$(ZLIB_DIR)/include",CFLAGS="$(VFP_CFLAGS) -I$(ZLIB_DIR)/include") \
	LDFLAGS="-Wl,-rpath,$(ZLIB_DIR)/lib -L$(ZLIB_DIR)/lib" CC="$(CC)" ./configure --prefix=$(PREFIX) --host=$(CROSS_HOST) --target=$(CROSS_TARGET) && \
	$(MAKE) && \
	$(MAKE) install

install_png:
	echo 'png installion '
	$(RM)    $(RM_FLAG)    $(TARGET_INSTALL_DIR)/$(PNG_VER)/pre-install
	$(MKDIR) $(MKDIR_FLAG) $(TARGET_INSTALL_DIR)/$(PNG_VER)/pre-install
	$(MKDIR) $(MKDIR_FLAG) $(TARGET_INSTALL_DIR)/$(PNG_VER)/pre-install/static_lib
	$(CP) $(CP_ALL_FLAG) $(PREFIX)/* $(TARGET_INSTALL_DIR)/$(PNG_VER)/pre-install
	$(MV) $(TARGET_INSTALL_DIR)/$(PNG_VER)/pre-install/lib/*.a  $(TARGET_INSTALL_DIR)/$(PNG_VER)/pre-install/static_lib
	$(MV) $(TARGET_INSTALL_DIR)/$(PNG_VER)/pre-install/lib/*.la $(TARGET_INSTALL_DIR)/$(PNG_VER)/pre-install/static_lib
	$(CP) $$(TARGET_INSTALL_DIR)/$(PNG_VER)/pre-install/include/libpng12/* $$(TARGET_INSTALL_DIR)/$(PNG_VER)/pre-install/include/* 
	
	
clean_jpeg:
	$(RM) $(RM_FLAG) $(BUILD_TMPDIR)

jpeg:
	$(MAKE) -C $(JPEG_FOLDER)
	$(MAKE) -C $(JPEG_FOLDER) install

install_jpeg:
	echo 'jpeg installion '
	$(RM)    $(RM_FLAG)    $(TARGET_INSTALL_DIR)/$(JPEG_VER)/pre-install
	$(MKDIR) $(MKDIR_FLAG) $(TARGET_INSTALL_DIR)/$(JPEG_VER)/pre-install
	$(MKDIR) $(MKDIR_FLAG) $(TARGET_INSTALL_DIR)/$(JPEG_VER)/pre-install/static_lib
	$(CP) $(CP_ALL_FLAG) $(JPEG_FOLDER)-install/* $(TARGET_INSTALL_DIR)/$(JPEG_VER)/pre-install
	$(MV) $(TARGET_INSTALL_DIR)/$(JPEG_VER)/pre-install/lib/*.a  $(TARGET_INSTALL_DIR)/$(JPEG_VER)/pre-install/static_lib
	$(MV) $(TARGET_INSTALL_DIR)/$(JPEG_VER)/pre-install/lib/*.la $(TARGET_INSTALL_DIR)/$(JPEG_VER)/pre-install/static_lib
	

clean_zlib:
	$(RM) $(RM_FLAG) $(BUILD_TMPDIR)

zlib:
	$(check_build_dir)
	$(TAR) $(ZLIB_DECOMPRESS_FLAG) $(ZLIB_FILE) -C $(BUILD_TMPDIR)
	cd $(BUILD_TMPDIR)/$(ZLIB_FOLDER)/ ; \
	$(if $(filter $(ENABLE_VFP),true),CPPFLAGS="$(VFP_CFLAGS)" CFLAGS="$(VFP_CFLAGS)",) \
	AR="$(AR) rc" ./configure --prefix=$(PREFIX)
	$(MAKE) -C $(BUILD_TMPDIR)/$(ZLIB_FOLDER)
	$(MAKE) -C $(BUILD_TMPDIR)/$(ZLIB_FOLDER) install
	$(MAKE) -C $(BUILD_TMPDIR)/$(ZLIB_FOLDER) distclean
	cd $(BUILD_TMPDIR)/$(ZLIB_FOLDER)/ ; \
	AR="$(AR) rc" ./configure --prefix=$(PREFIX) --shared
	$(MAKE) -C $(BUILD_TMPDIR)/$(ZLIB_FOLDER)
	$(MAKE) -C $(BUILD_TMPDIR)/$(ZLIB_FOLDER) install
 
install_zlib:
	echo 'zlib installion '
	$(RM)    $(RM_FLAG)    $(TARGET_INSTALL_DIR)/$(ZLIB_VER)/pre-install
	$(MKDIR) $(MKDIR_FLAG) $(TARGET_INSTALL_DIR)/$(ZLIB_VER)/pre-install
	$(MKDIR) $(MKDIR_FLAG) $(TARGET_INSTALL_DIR)/$(ZLIB_VER)/pre-install/static_lib
	$(CP) $(CP_ALL_FLAG) $(PREFIX)/* $(TARGET_INSTALL_DIR)/$(ZLIB_VER)/pre-install
	$(MV) $(TARGET_INSTALL_DIR)/$(ZLIB_VER)/pre-install/lib/*.a $(TARGET_INSTALL_DIR)/$(ZLIB_VER)/pre-install/static_lib
	
	
	
	
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
	$(CP) $(CP_ALL_FLAG) busybox-1.15.3_patch/* $(BUILD_TMPDIR)/$(BUSYBOX_FOLDER)/
	$(if $(filter $(ENABLE_VFP),true),CPPFLAGS="$(VFP_CFLAGS)" CFLAGS="$(VFP_CFLAGS)",) \
	$(MAKE) -j 20 all     -C $(BUILD_TMPDIR)/$(BUSYBOX_FOLDER) SUBARCH=$(SUBARCH) CROSS_COMPILE=$(CROSS_COMPILE)
	$(MAKE) -j 20 install -C $(BUILD_TMPDIR)/$(BUSYBOX_FOLDER)
	$(MKDIR) $(MKDIR_FLAG) $(PREFIX)/$(notdir $(BUSYBOX_CFG_FILE))
	$(CP) $(CP_ALL_FLAG) $(BUILD_TMPDIR)/$(BUSYBOX_FOLDER)/_install/* $(PREFIX)/$(notdir $(BUSYBOX_CFG_FILE))

	
install_busybox :
	$(MKDIR) $(MKDIR_FLAG) $(TARGET_INSTALL_DIR)/$(BUSYBOX_VER)/pre-install/$(BUSYBOX_CFG_FILE)/bin
	$(CP) $(CP_ALL_FLAG) $(PREFIX)/$(notdir $(BUSYBOX_CFG_FILE))/bin/busybox $(TARGET_INSTALL_DIR)/$(BUSYBOX_VER)/pre-install/$(BUSYBOX_CFG_FILE)/bin/busybox
	
