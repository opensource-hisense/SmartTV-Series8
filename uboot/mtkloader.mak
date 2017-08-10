###############################################################################
# linux_core/uboot/mtkloader.mak                                                 
#                                                                             
# SMP core init                                                               
#                                                                             
# Copyright (c) 2010-2012 MediaTek Inc.                                       
# $Author: dtvbm11 $                                                    
#                                                                             
# This program is free software; you can redistribute it and/or modify        
# it under the terms of the GNU General Public License version 2 as           
# published by the Free Software Foundation.                                  
#                                                                             
# This program is distributed in the hope that it will be useful, but WITHOUT 
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or       
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for   
# more details.                                                               
###############################################################################

unexport MW_MODEL
unexport LINUX_SOLUTION
unexport ROOTFS_NAME
unexport KEY_FROM_DFB
unexport AV_BYPASS 
unexport UBOOT_VERSION
unexport APP_IF
unexport RAMDISK_ROOTFS
unexport MMP_SUPPORT
unexport RELEASE_3RD_LIB_LIST
unexport TARGET
unexport CUSTOM
unexport CUST_MODEL
unexport MTAL_SUPPORT
#unexport TOOL_CHAIN
unexport SUPPORT_PIP
unexport OPTIMIZE_LVL
unexport AV_BYPASS
unexport MMP_SUPPORT
unexport INET_SUPPORT
unexport FW_UPG_SUPPORT
unexport DIVX_DRM
unexport USB_UPG_VERSION
unexport SERIAL_NUMBER VERSION
unexport PROJECT_ROOT UBOOT_LIBRARY MW_LIB_DIR LINUX_DRV_ROOT THUMB
unexport UBOOT_ROOT
#unexport CROSS_COMPILE
#unexport CPPFLAGS
unexport CFLAGS
unexport AFLAGS
unexport ROM_CODE
unexport CC_SECURE_ROM_DEF
unexport CPPFLAGS CFLAGS AFLAGS 
unexport CC_LOADER_DEF
unexport CC_PRELOADER_DEF
unexport COMMON_INC OSAI_INC
unexport AS

export DEFINES=
#
# Executable name
#
ifndef BUILD_LINUX_LOADER
BUILD_LINUX_LOADER := true
endif
export BUILD_LINUX_LOADER

MODEL_NAME ?= $(MODEL)
export EXE_NAME := $(MODEL_NAME)

ifeq "$(BUILD_CFG)" "debug"
export EXE_NAME := $(EXE_NAME)_dbg

else
ifeq "$(BUILD_CFG)" "cli"
export EXE_NAME := $(EXE_NAME)_cli
endif
endif

export MAP_NAME := $(EXE_NAME)$(MAP_SUFFIX)

RLS_CUSTOM_BUILD ?= false


ROOTDIR	:= $(shell if [ "$$PWD" != "" ]; then echo $$PWD; else bash -c pwd -L; fi)

VM_LINUX_ROOT ?= $(word 1, $(subst /vm_linux/,/vm_linux /, $(ROOTDIR)))

MTKLOADER_SUBPATH ?= _build_mtkloader/project_x

ifdef MTKLOADER_ROOT
export PROJECT_ROOT = $(MTKLOADER_ROOT)
else
export PROJECT_ROOT = $(VM_LINUX_ROOT)/linux_core/uboot/$(MTKLOADER_SUBPATH)
endif
export PROJECT_ROOT = $(VM_LINUX_ROOT)/project_x

include $(DTV_LINUX_MAK_ROOT)/nucleus_host.mak

ifndef WARNING_TO_ERROR
export WARNING_TO_ERROR := false
endif
ifeq "$(BUILD_CFG)" "debug"
export MODE ?= debug
endif

ifeq "$(BUILD_CFG)" "cli"
export MODE ?= debug
endif

export MODE ?= release

ifeq "$(SECURE_BOOT)" "y"
export SECURE=ALL
endif

export BRANCH_NAME  = $(word 2,$(BUILD_NAME))
export WARNING_LOG  = $(THIS_ROOT)/build_$(TARGET_IC).log
export ERROR_LOG    = $(THIS_ROOT)/build_fail.log

export CHK_ERR_WARN_SCRIPT = $(PROJECT_ROOT)/tools/source/public/pbuild/chk_warn_err.sh 
export CHK_ERR_WARN_PARM   = $(PROJECT_ROOT)/tools/source/public/pbuild $(BRANCH_NAME) $(WARNING_LOG) $(ERROR_LOG)
	
#
# Find the local cfg.mak, if existed, include it
#
CFG_MAK_NAME := cfg.mak

INC_CFG_MAK := $(shell if [ -e $(UBOOT_CFG_DIR)/$(CFG_MAK_NAME) ];then echo "$(UBOOT_CFG_DIR)/$(CFG_MAK_NAME)"; else echo ""; fi)

ifneq "$(INC_CFG_MAK)" ""
include $(INC_CFG_MAK)
endif

export TARGET_DRIVER := $(subst mt,,$(TARGET_IC))_driver
export FAST_SUPPORT  := true
include $(TARGET_DIR_ROOT)/$(TARGET_IC)/drv_opt/$(COMPANY)/$(MODEL).def


ifeq "$(BOOT_TYPE)" "ROM2NAND"
DEFINES += -DCC_NAND_LOADER -DCC_NAND_ENABLE -DCC_NAND_BOOT
BOOT = nand
endif

ifeq "$(BOOT_TYPE)" "ROM2EMMC"
DEFINES += -DCC_EMMC_BOOT
BOOT = emmc
endif

ifeq "$(BOOT_TYPE)" "ROM2NOR"
DEFINES += -DCC_NAND_LOADER -DCC_NAND_ENABLE
BOOT = nor
endif

DEFINES += -DCC_LOAD_UBOOT
export NO_SYM_DEBUG ?= TRUE

ifeq "$(MODE)" "release"
DEFINES += -DNDEBUG
else
DEFINES += -DCC_DEBUG
endif

ifeq "$(NOLOG)" "y"
DEFINES += -DNOLOG
endif

SHOW_BUILD_COMMAND=
V ?= 0
ifeq "$(V)" "0"
SHOW_BUILD_COMMAND = -s
endif
JOBS  ?= 12
RLS_CUSTOM_BUILD ?= false

export CFG_FILE_DIR := $(DRIVER_CFG_DIR)
export CC_INC += -I$(DRV_INC_DIR)

export PRELOADER_BIN_DIR := $(shell mkdir -p $(MTKLOADER_OBJ_ROOT)/target/$(TARGET_IC)/preloader && cd $(MTKLOADER_OBJ_ROOT)/target/$(TARGET_IC)/preloader && /bin/pwd)
export LOADER_BIN_DIR    := $(shell mkdir -p $(MTKLOADER_OBJ_ROOT)/target/$(TARGET_IC)/loader && cd $(MTKLOADER_OBJ_ROOT)/target/$(TARGET_IC)/loader && /bin/pwd)
export MTKLOADER_BIN_DIR := $(shell mkdir -p $(MTKLOADER_OBJ_ROOT)/target/$(TARGET_IC)/mtkloader && cd $(MTKLOADER_OBJ_ROOT)/target/$(TARGET_IC)/mtkloader && /bin/pwd)


all: do_drv_inc
	$(MAKE) $(if $(filter -j,$(MAKEFLAGS)),,-j $(JOBS)) -C $(TARGET_DIR_ROOT)/$(TARGET_IC)/mtkloader TARGET=$(TARGET_IC) LINUX_SOLUTION=false UBOOT_LIBRARY= $(SHOW_BUILD_COMMAND) --no-print-directory 2>&1| tee  $(THIS_ROOT)/build.mtkloader.log
	@$(call chk_err_fct, $(THIS_ROOT)/build.mtkloader.log);
	@$(CP) $(MTKLOADER_BIN_DIR)/$(EXE_NAME)_mtkloader.bin $(UBOOT_PACK_ROOT)/$(MODEL)_$(MODE)_mtkloader_$(BOOT).bin

do_drv_inc :
	@$(call if_file_notexist_w_sym_fct, $(DRV_INC_ROOT), $(TARGET_DIR_ROOT)/$(TARGET_IC)/$(TARGET_DRIVER)/drv_inc);

	
clean:
