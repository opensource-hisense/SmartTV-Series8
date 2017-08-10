#
# arch/arm64/boot/Makefile
#
# This file is included by the global makefile so that you can add your own
# architecture-specific flags and dependencies.
#
# This file is subject to the terms and conditions of the GNU General Public
# License.  See the file "COPYING" in the main directory of this archive
# for more details.
#
# Copyright (C) 2012, ARM Ltd.
# Author: Will Deacon <will.deacon@arm.com>
#
# Based on the ia64 boot/Makefile.
#

include $(srctree)/arch/arm64/boot/dts/Makefile

targets := Image Image.gz uImage 

ZRELADDR    := 0x80000
ZRELADDR0    := $(shell printf "%d" $(ZRELADDR))
ZRELADDR1    := $(shell expr $(ZRELADDR0) + 64)
ZRELADDRE    := $(shell printf "0x%.x" $(ZRELADDR1))
UIMAGE_LOADADDR=$(ZRELADDR)
$(obj)/uImage: UIMAGE_ENTRYADDR=$(ZRELADDRE)
$(obj)/uImage-dtb: UIMAGE_ENTRYADDR=$(ZRELADDRE)

check_for_multiple_loadaddr = \
if [ $(words $(UIMAGE_LOADADDR)) -ne 1 ]; then \
	echo 'multiple (or no) load addresses: $(UIMAGE_LOADADDR)'; \
	echo 'This is incompatible with uImages'; \
	echo 'Specify LOADADDR on the commandline to build an uImage'; \
	false; \
fi

UIMAGE_COMPRESSION = gzip
UIMAGE_ARCH := arm
DTB_NAMES := $(subst $\",,$(CONFIG_BUILD_ARM64_APPENDED_DTB_IMAGE_NAMES))
ifneq ($(DTB_NAMES),)
DTB_LIST := $(addsuffix .dtb,$(DTB_NAMES))
else
DTB_LIST := $(dtb-y)
endif
DTB_OBJS := $(addprefix $(obj)/dts/,$(DTB_LIST))

$(obj)/Image: vmlinux FORCE
	$(call if_changed,objcopy)

$(obj)/Image.gz: $(obj)/Image FORCE
	$(call if_changed,gzip)

$(obj)/zImage:	$(obj)/Image.gz FORCE
	$(call if_changed,uimage)
	
$(obj)/Image.gz-dtb: $(obj)/Image.gz $(DTB_OBJS) FORCE
	$(call if_changed,cat)		
ifndef CONFIG_ANDROID
$(obj)/uImage:	$(obj)/zImage $(DTB_OBJS) FORCE
	$(call if_changed,cat)
	
$(obj)/uImage-dtb:	$(obj)/uImage FORCE
	$(call if_changed,uimage)
else
$(obj)/uImage-dtb:	$(obj)/zImage $(DTB_OBJS) FORCE
	$(call if_changed,cat)
endif

install: $(obj)/Image
	$(CONFIG_SHELL) $(srctree)/$(src)/install.sh $(KERNELRELEASE) \
	$(obj)/Image System.map "$(INSTALL_PATH)"

zinstall: $(obj)/Image.gz
	$(CONFIG_SHELL) $(srctree)/$(src)/install.sh $(KERNELRELEASE) \
	$(obj)/Image.gz System.map "$(INSTALL_PATH)"
