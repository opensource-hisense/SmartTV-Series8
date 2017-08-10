###############################################################################
# chiling/uboot/secure/Makefile                                               #
#                                                                             #
# SMP core init                                                               #
#                                                                             #
# Copyright (c) 2010-2012 MediaTek Inc.                                       #
# $Author: jacky.ko $                                                         #
#                                                                             #
# This program is free software; you can redistribute it and/or modify        #
# it under the terms of the GNU General Public License version 2 as           #
# published by the Free Software Foundation.                                  #
#                                                                             #
# This program is distributed in the hope that it will be useful, but WITHOUT #
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or       #
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for   #
# more details.                                                               #
###############################################################################

# System config file
include ../config.mak

LZHS_TOOL = $(VM_LINUX_ROOT)/tools/binary/mt5391_pack/lzhsenc
PACKIMG   = $(VM_LINUX_ROOT)/tools/binary/mt5391_pack/packimg
ifeq ($(SECURE_BOOT_V2), true)
ifdef SECURE_BOOT_KEY_INDEX
KEY_INDEX :=  $(SECURE_BOOT_KEY_INDEX)
else
KEY_INDEX := 0
endif
ifeq ($(USE_SBOOT), true)
T_MTK     = $(VM_LINUX_ROOT)/tools/binary/image-tools/sboot
MTK_PUBLIC_KEY = $(VM_LINUX_ROOT)/tools/binary/image-tools/$(TARGET_IC)/MTK_PUBLIC_KEY_$(KEY_INDEX)
MTK_PRIVATE_KEY = $(VM_LINUX_ROOT)/tools/binary/image-tools/$(TARGET_IC)/MTK_PRIVATE_KEY_$(KEY_INDEX)
else
T_MTK     = $(VM_LINUX_ROOT)/tools/binary/image-tools/sign$(KEY_INDEX)
endif
else
T_MTK     = $(VM_LINUX_ROOT)/tools/binary/image-tools/t_mtk
endif
SCRAMBLE = $(VM_LINUX_ROOT)/tools/binary/image-tools/scramble
AES_KEY2 := $(THIS_ROOT)/data/aes_key_set2

ifeq "$(CUST_CLR_IMG)" "true"

CUST_CLR_ROOT      = $(THIS_ROOT)/secure_rls
CUST_CLR_TOOL_PATH = $(CUST_CLR_ROOT)/tools

ifeq "$(BUILD_CFG)" "debug"
CUST_CLR_PATH = $(CUST_CLR_ROOT)/cli/bin
else
ifeq "$(BUILD_CFG)" "cli"
CUST_CLR_PATH = $(CUST_CLR_ROOT)/cli/bin
else
CUST_CLR_PATH = $(CUST_CLR_ROOT)/rel/bin
endif
endif

endif

CFLAGS= -std=c99 -pedantic -Wall -Wpointer-arith -Wstrict-prototypes -O0 -g
CC= gcc

ifeq ($(SECURE_BOOT_V2), true)
SIGGEN_KEY := $(shell if [ -e $(THIS_ROOT)/data/public_key2 -a -e $(THIS_ROOT)/data/private_key2 ]; then echo "$(THIS_ROOT)/data/public_key2 $(THIS_ROOT)/data/private_key2"; else echo "";fi)
else
SIGGEN_KEY := $(shell if [ -e $(THIS_ROOT)/data/public_key -a -e $(THIS_ROOT)/data/private_key ]; then echo "$(THIS_ROOT)/data/public_key $(THIS_ROOT)/data/private_key"; else echo "";fi)
endif

clean distclean:
	rm -f  $(UBOOT_SECURE_PACK_ROOT)/uboot.* $(UBOOT_SECURE_PACK_ROOT)/*.map $(UBOOT_SECURE_PACK_ROOT)/*preloader*

5365_package:
	chmod +x $(PACKIMG)
	chmod +x $(LZHS_TOOL)
	chmod +x $(T_MTK)
	rm -f $(UBOOT_SECURE_PACK_ROOT)/u-boot_pack.bin
	rm -f $(UBOOT_SECURE_PACK_ROOT)/u-boot_pack.lzhs
	cp $(UBOOT_OBJ_ROOT)/u-boot.bin $(UBOOT_SECURE_PACK_ROOT)/
	cp $(UBOOT_PACK_ROOT)/mtkloader.bin $(UBOOT_SECURE_PACK_ROOT)/
ifeq ($(BOOT_NO_LZHS), true)
	cd $(UBOOT_SECURE_PACK_ROOT) ; $(LZHS_TOOL) u-boot.bin u-boot_pack.lzhs 1
else
	cd $(UBOOT_SECURE_PACK_ROOT) ; $(LZHS_TOOL) u-boot.bin u-boot_pack.lzhs
endif
ifeq "$(CUST_CLR_IMG)" "true"
	cp -f $(UBOOT_SECURE_PACK_ROOT)/u-boot_pack.lzhs $(CUST_CLR_PATH)
endif
ifeq "$(SECURE_BOOT_V2)" "true"
ifeq "$(USE_SBOOT)" "true"
	cd $(UBOOT_SECURE_PACK_ROOT)/ ; $(T_MTK) u-boot_pack.lzhs $(MTK_PUBLIC_KEY) $(MTK_PRIVATE_KEY) 2 0 $(AES_KEY2) $(SIGGEN_KEY)
else
	cd $(UBOOT_SECURE_PACK_ROOT)/ ; $(T_MTK) u-boot_pack.lzhs $(KEY_INDEX) 2 0 $(AES_KEY2) $(SIGGEN_KEY)
endif
else
	cd $(UBOOT_SECURE_PACK_ROOT)/ ; $(T_MTK) u-boot_pack.lzhs 0 2 0 $(SIGGEN_KEY)
endif
ifeq "$(SECURE_BOOT_SCRAMBLE)" "true"
	echo "scramble uboot...$(SCRAMBLE) u-boot_pack.lzhs u-boot_pack.lzhs.aes $(AES_KEY2)";
	cd $(UBOOT_SECURE_PACK_ROOT) ; \
	$(SCRAMBLE) u-boot_pack.lzhs u-boot_pack.lzhs.aes $(AES_KEY2) ; \
	mv u-boot_pack.lzhs u-boot_pack.lzhs.org ; \
	mv u-boot_pack.lzhs.aes u-boot_pack.lzhs
endif
	@rm -f $(UBOOT_SECURE_PACK_ROOT)/MTK_PRIVATE_KEY_0
	@rm -f $(UBOOT_SECURE_PACK_ROOT)/MTK_PUBLIC_KEY_0
ifeq ($(BOOT_NO_LZHS), true)
	/bin/sed 's/MODEL/$(MODEL)/g' uboot_secure_no_lzhs.sc > $(UBOOT_SECURE_PACK_ROOT)/temp1.sc;
else
	/bin/sed 's/MODEL/$(MODEL)/g' uboot_secure.sc > $(UBOOT_SECURE_PACK_ROOT)/temp1.sc;
endif
	/bin/sed 's/BOOT/$(BOOT)/g' $(UBOOT_SECURE_PACK_ROOT)/temp1.sc > $(UBOOT_SECURE_PACK_ROOT)/temp2.sc;
	cd $(UBOOT_SECURE_PACK_ROOT)/ ; $(PACKIMG) -f temp2.sc;
ifeq "$(CUST_CLR_IMG)" "true"
	cp -f $(UBOOT_SECURE_PACK_ROOT)/temp2.sc $(CUST_CLR_TOOL_PATH)
	cp -f $(PACKIMG) $(CUST_CLR_TOOL_PATH)
endif
	mv -f $(UBOOT_SECURE_PACK_ROOT)/$(MODEL)_secure_$(BOOT)boot.bin  $(UBOOT_SECURE_PACK_ROOT)/..

	/bin/sed 's/MODEL/$(MODEL)/g' dual_uboot_secure.sc > $(UBOOT_SECURE_PACK_ROOT)/temp3.sc;
	/bin/sed 's/BOOT/$(BOOT)/g' $(UBOOT_SECURE_PACK_ROOT)/temp3.sc > $(UBOOT_SECURE_PACK_ROOT)/temp4.sc;
	cd $(UBOOT_SECURE_PACK_ROOT)/ ; $(PACKIMG) -f temp4.sc;
	mv -f $(UBOOT_SECURE_PACK_ROOT)/$(MODEL)_secure_dual_$(BOOT)boot.bin  $(UBOOT_SECURE_PACK_ROOT)/..
-include $(wildcard $(UBOOT_SECURE_PACK_ROOT)/*.d)
