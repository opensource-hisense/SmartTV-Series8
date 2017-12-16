#
# Copyright (c) MediaTek Inc.
#
# This program is distributed under a dual license of GPL v2.0 and
# MediaTek proprietary license. You may at your option receive a license
# to this program under either the terms of the GNU General Public
# License (GPL) or MediaTek proprietary license, explained in the note
# below.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
#
###############################################################################
# Copyright Statement:                                                        #
#                                                                             #
#   This software/firmware and related documentation ("MediaTek Software")    #
# are protected under international and related jurisdictions'copyright laws  #
# as unpublished works. The information contained herein is confidential and  #
# proprietary to MediaTek Inc. Without the prior written permission of        #
# MediaTek Inc., any reproduction, modification, use or disclosure of         #
# MediaTek Software, and information contained herein, in whole or in part,   #
# shall be strictly prohibited.                                               #
# MediaTek Inc. Copyright (C) 2010. All rights reserved.                      #
#                                                                             #
#   BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND      #
# AGREES TO THE FOLLOWING:                                                    #
#                                                                             #
#   1)Any and all intellectual property rights (including without             #
# limitation, patent, copyright, and trade secrets) in and to this            #
# Software/firmware and related documentation ("MediaTek Software") shall     #
# remain the exclusive property of MediaTek Inc. Any and all intellectual     #
# property rights (including without limitation, patent, copyright, and       #
# trade secrets) in and to any modifications and derivatives to MediaTek      #
# Software, whoever made, shall also remain the exclusive property of         #
# MediaTek Inc.  Nothing herein shall be construed as any transfer of any     #
# title to any intellectual property right in MediaTek Software to Receiver.  #
#                                                                             #
#   2)This MediaTek Software Receiver received from MediaTek Inc. and/or its  #
# representatives is provided to Receiver on an "AS IS" basis only.           #
# MediaTek Inc. expressly disclaims all warranties, expressed or implied,     #
# including but not limited to any implied warranties of merchantability,     #
# non-infringement and fitness for a particular purpose and any warranties    #
# arising out of course of performance, course of dealing or usage of trade.  #
# MediaTek Inc. does not provide any warranty whatsoever with respect to the  #
# software of any third party which may be used by, incorporated in, or       #
# supplied with the MediaTek Software, and Receiver agrees to look only to    #
# such third parties for any warranty claim relating thereto.  Receiver       #
# expressly acknowledges that it is Receiver's sole responsibility to obtain  #
# from any third party all proper licenses contained in or delivered with     #
# MediaTek Software.  MediaTek is not responsible for any MediaTek Software   #
# releases made to Receiver's specifications or to conform to a particular    #
# standard or open forum.                                                     #
#                                                                             #
#   3)Receiver further acknowledge that Receiver may, either presently        #
# and/or in the future, instruct MediaTek Inc. to assist it in the            #
# development and the implementation, in accordance with Receiver's designs,  #
# of certain softwares relating to Receiver's product(s) (the "Services").    #
# Except as may be otherwise agreed to in writing, no warranties of any       #
# kind, whether express or implied, are given by MediaTek Inc. with respect   #
# to the Services provided, and the Services are provided on an "AS IS"       #
# basis. Receiver further acknowledges that the Services may contain errors   #
# that testing is important and it is solely responsible for fully testing    #
# the Services and/or derivatives thereof before they are used, sublicensed   #
# or distributed. Should there be any third party action brought against      #
# MediaTek Inc. arising out of or relating to the Services, Receiver agree    #
# to fully indemnify and hold MediaTek Inc. harmless.  If the parties         #
# mutually agree to enter into or continue a business relationship or other   #
# arrangement, the terms and conditions set forth herein shall remain         #
# effective and, unless explicitly stated otherwise, shall prevail in the     #
# event of a conflict in the terms in any agreements entered into between     #
# the parties.                                                                #
#                                                                             #
#   4)Receiver's sole and exclusive remedy and MediaTek Inc.'s entire and     #
# cumulative liability with respect to MediaTek Software released hereunder   #
# will be, at MediaTek Inc.'s sole discretion, to replace or revise the       #
# MediaTek Software at issue.                                                 #
#                                                                             #
#   5)The transaction contemplated hereunder shall be construed in            #
# accordance with the laws of Singapore, excluding its conflict of laws       #
# principles.  Any disputes, controversies or claims arising thereof and      #
# related thereto shall be settled via arbitration in Singapore, under the    #
# then current rules of the International Chamber of Commerce (ICC).  The     #
# arbitration shall be conducted in English. The awards of the arbitration    #
# shall be final and binding upon both parties and shall be entered and       #
# enforceable in any court of competent jurisdiction.                         #
###############################################################################
###########################################################################
# $RCSfile: Makefile,v $
# $Revision: #1 $
# $Date: 2015/12/22 $
# $Author: dtvbm11 $
# $CCRevision: /main/DTV_X_ATSC/6 $
# $SWAuthor: Joshuah Huang $
# $MD5HEX: e2c57029ea8dd3e986bb5c3b4fa60554 $
#
# Description:
#         For each main program to define the executable file name.
#
#         Specify the source files to be compile in SRC.
#############################################################################

ROOTDIR	:= $(shell if [ "$$PWD" != "" ]; then echo $$PWD; else bash -c pwd -L; fi)
export ROOTDIR

ifndef CUSTOMER
CUSTOMER	:= cmtk / mtk
endif

ifndef MODEL_NAME
MODEL_NAME := mt5395_atsc_linux
endif

ifndef THIS_ROOT
THIS_ROOT	:= $(shell bash -c pwd -L)
endif

ifndef LOCKFILE
export LOCKFILE = lockfile -1 -l 30
endif

ifeq "$(TOOL_CHAIN)" "4.9.1"
export USE_PRIVATE_LIBGCC := $(CHILING_ROOT)/rootfs/$(LIB_SUB_ROOT)/without-multi-arch-lib
endif

#
# Override Object Type
#
export APPEND_OBJ_TYPE := uboot


FROMELF := fromelf

JOBS  ?= 12

SHOW_BUILD_COMMAND=
V ?= 0
ifeq "$(V)" "0"
SHOW_BUILD_COMMAND = -s
endif

ifndef TARGET_IC
TARGET_IC = mt5392b
endif

ifndef UBOOT_VERSION
UBOOT_VERSION = 1.3.4
endif

export UBOOT_DIR = u-boot-$(UBOOT_VERSION)

ifdef LOADER_MAX_SIZE
export DEFINES += -DLOADER_MAX_SIZE=$(LOADER_MAX_SIZE)
endif

ifeq "$(BOOT_TYPE)" "ROM2EMMC"
export DEFINES += -DCC_EMMC_BOOT
ifeq "$(EMMC_4BIT)" "true"
export DEFINES += -DCC_EMMC_4BIT
endif
ifeq "$(EMMC_HS200)" "true"
export DEFINES += -DCC_EMMC_HS200
endif
ifeq "$(EMMC_DDR50)" "true"
export DEFINES += -DCC_EMMC_DDR50
endif
endif

ifeq "$(KERNEL_ON_NOR)" "y"
export DEFINES += -DCC_KERNEL_ON_NOR
endif

ifeq "$(NOLOG)" "y"
export DEFINES += -DCONFIG_LOG_SILENT=1 -DNOLOG
endif

ifeq ($(USB_ADB_PORT), 2)
export DEFINES += -DCC_USB3_ADB_ENABLE
endif

#LN = ln
#LN_FLAG = -sf
#if_file_notexist_w_sym_fct     = if [ -e $(1) -o -h $(1) ] && [ ! -e $(2) -a ! -h $(2) ]; then echo "symlink: `basename $(1)`";$(LN) $(LN_FLAG) $(1) $(2);if [ "$(SYMLINK_LOG)" != "" ]; then echo $(2) >> $(SYMLINK_LOG); fi ;k fi

#
# Find the local cfg.mak, if existed, include it
#
CFG_MAK_NAME := cfg.mak

INC_CFG_MAK := $(shell if [ -e $(UBOOT_CFG_DIR)/$(CFG_MAK_NAME) ];then echo "$(UBOOT_CFG_DIR)/$(CFG_MAK_NAME)"; else echo ""; fi)

ifneq "$(INC_CFG_MAK)" ""
include $(INC_CFG_MAK)
DEFINES := $(sort $(DEFINES))
endif

RLS_CUSTOM_BUILD ?= false
ifndef CUSTOMER_ARCH_BUILD
export PROJECT_ROOT      = $(word 1, $(subst /linux_core/,/linux_core /, $(shell bash -c pwd -L)))
export TARGET_ROOT       = $(PROJECT_ROOT)/driver/target/public/$(TARGET_IC)

include $(DTV_LINUX_MAK_ROOT)/host.mak

#ifeq "$(RLS_CUSTOM_BUILD)" "false"
#export MTKLOADER_SUBPATH = _build/_mtkloader/project_x
#else
#export MTKLOADER_SUBPATH =  linux_core/dtv_mtkloader/project_x
#endif

#export MTKLOADER_SUBPATH = _build/_mtkloader/project_x

#export MTKLOADER_ROOT    = $(VM_LINUX_ROOT)/$(MTKLOADER_SUBPATH)
#export UTARGET_ROOT      = $(MTKLOADER_ROOT)/target/$(TARGET_IC)
endif

MODULE_DIR_LIST = drv_cust inc
NPTV_DIR_LIST =
MIDDLEWARE_LIST = $(PROJECT_ROOT)/driver/linux/lib/mw/x_inc
PRELOADER_FILE_LIST = $(wildcard $(TARGET_ROOT)/$(TARGET_DRIVER)/bsp/*.c $(TARGET_ROOT)/$(TARGET_DRIVER)/bsp/*.h)

BOARDDIR		:= $(ROOTDIR)/$(UBOOT_DIR)/board/$(TARGET_IC)

DRV_LIB_DIR := $(ROOTDIR)/drv_lib

export DRVLIB_DRVCUST_DIR := $(DRV_LIB_DIR)/$(TARGET_IC)/drv_cust
export DRVLIB_NPTV_DIR := $(DRV_LIB_DIR)/$(TARGET_IC)/nptv
export DRVLIB_DRV_INC_DIR := $(DRV_LIB_DIR)/$(TARGET_IC)/drv_inc


PRELOADER_DIR := preloader/$(TARGET_IC)
PACK_DIR := pack
SECURE_DIR := secure

export UBOOT_NEW_ARCH = y
ifeq "$(TARGET_IC)" "mt5360"
TARGET_DRIVER = 5360_driver
endif
ifeq "$(TARGET_IC)" "mt5381"
TARGET_DRIVER = 5381_driver
endif
ifeq "$(TARGET_IC)" "mt5382p"
endif
ifeq "$(TARGET_IC)" "mt5391"
TARGET_DRIVER = 5391_driver
endif
ifeq "$(TARGET_IC)" "mt5392b"
TARGET_DRIVER = 5392b_driver
endif
ifeq "$(TARGET_IC)" "mt5363"
TARGET_DRIVER = 5363_driver
endif
ifeq "$(TARGET_IC)" "mt5365"
TARGET_DRIVER = 5365_driver
endif
ifeq "$(TARGET_IC)" "mt5395"
TARGET_DRIVER = 5395_driver
endif
ifeq "$(TARGET_IC)" "mt5396"
TARGET_DRIVER = 5396_driver
endif
ifeq "$(TARGET_IC)" "mt5368"
TARGET_DRIVER = 5368_driver
endif
ifeq "$(TARGET_IC)" "mt5389"
TARGET_DRIVER = 5389_driver
endif
ifeq "$(TARGET_IC)" "mt5398"
TARGET_DRIVER = 5398_driver
endif
ifeq "$(TARGET_IC)" "mt5880"
TARGET_DRIVER = 5880_driver
endif
ifeq "$(TARGET_IC)" "mt5881"
TARGET_DRIVER = 5881_driver
endif
ifeq "$(TARGET_IC)" "mt5399"
TARGET_DRIVER = 5399_driver
endif
ifeq "$(TARGET_IC)" "mt5891"
TARGET_DRIVER = 5891_driver
endif
ifeq "$(TARGET_IC)" "mt5890"
TARGET_DRIVER = 5890_driver
endif
ifeq "$(TARGET_IC)" "mt5882"
TARGET_DRIVER = 5882_driver
endif
ifeq "$(TARGET_IC)" "mt5883"
TARGET_DRIVER = 5883_driver
endif
export TARGET_DRIVER
export UBOOT_LIBRARY = y

include config.mak
PLATFORM_LIBS = -L $(shell dirname `$(CC) $(CFLAGS) -print-libgcc-file-name`) -lgcc
export UBOOT_NEW_LIBRARY = y

ifeq "$(BUILD_CFG)" "debug"
export MODE ?= debug
export DEFINES += -DCC_DEBUG
endif

ifeq "$(BUILD_CFG)" "cli"
export MODE ?= debug
endif

export MODE ?= release

ifndef EXE_NAME
EXE_NAME = $(MODEL)
endif

ifeq ($(UBOOT_NEW_ARCH),y)
ifeq ($(TARGET_IC), $(filter $(TARGET_IC), mt5365 mt5395))
MODULE_FILE_LIST = bsp/hal_io.c ckgen/ckgen.c ckgen/gpio.c ckgen/pinmux.c ckgen/setpll.c inc/x_gpio.h inc/x_pdwnc.h pdwnc/*.c pdwnc/*.h \
					nand/nandhw_reg.h nor/serialflash_hw.c nor/serialflash_hw.h nor/nor_debug.h dmx/dmx_aes_reg.h \
					nor/eeprom_hw.c nor/eeprom_if.c sif/sif_diag.c sif/sif_if.c sif/sif_if.h sif/sif_isr.c sif/sif_hw_v1.c sif/sif_hw_v2.c sif/sif_debug.h sif/sif_hw.h sif/sif_isr.h \
					ir/ir_ph.c ir/ir_hw.c  ir/irrx_if.c ir/ir_io.h ir/ir_debug.h ir/ir_ph.h ir/ir_reg.h
else
ifeq ($(TARGET_IC), $(filter $(TARGET_IC), mt5396 mt5368 mt5389))
MODULE_FILE_LIST = bsp/hal_io.c ckgen/ckgen.c ckgen/gpio.c ckgen/pinmux.c ckgen/setpll.c inc/x_gpio.h inc/x_pdwnc.h pdwnc6896/*.c pdwnc6896/*.h \
					nand/nandhw_reg.h msdc/msdc_host_hw.h msdc/msdc_slave_hw.h msdc/msdc_drv.h nor/serialflash_hw.c nor/serialflash_hw.h nor/nor_debug.h dmx6896/dmx_aes_reg.h nor/eeprom_hw.c nor/eeprom_if.c \
					sif6896/sif_diag.c sif6896/sif_if.c sif6896/sif_if.h sif6896/sif_isr.c sif6896/sif_hw_v1.c sif6896/sif_hw_v2.c sif6896/sif_debug.h sif6896/sif_hw.h sif6896/sif_isr.h \
					ir6896/ir_ph.c ir6896/ir_hw.c  ir6896/irrx_if.c ir6896/ir_io.h ir6896/ir_debug.h ir6896/ir_ph.h ir6896/ir_reg.h dmx6896/gcpu_hw.c dmx6896/gcpu_api.c \
					dmx6896/gcpu_if.c dmx6896/*.h dmx6896/gcpu/*.h dmx6896/dmx_drm_if.c  dmx6896/dmx_dtcp_api.c
else
ifeq ($(TARGET_IC), $(filter $(TARGET_IC), mt5398 mt5880))
MODULE_FILE_LIST = bsp/hal_io.c ckgen/ckgen.c ckgen/gpio.c ckgen/pinmux.c ckgen/setpll.c inc/x_gpio.h inc/x_pdwnc.h pdwnc6896/*.c pdwnc6896/*.h \
					nand/nandhw_reg.h msdc/msdc_host_hw.h msdc/msdc_slave_hw.h msdc/msdc_drv.h nor/serialflash_hw.c nor/serialflash_hw.h nor/nor_debug.h dmx6896/dmx_aes_reg.h nor/eeprom_hw.c nor/eeprom_if.c \
					sif6896/sif_diag.c sif6896/sif_if.c sif6896/sif_if.h sif6896/sif_isr.c sif6896/sif_hw_v1.c sif6896/sif_hw_v2.c sif6896/sif_debug.h sif6896/sif_hw.h sif6896/sif_isr.h \
					ir6896/ir_ph.c ir6896/ir_hw.c  ir6896/irrx_if.c ir6896/ir_io.h ir6896/ir_debug.h ir6896/ir_ph.h ir6896/ir_reg.h dmx9880/gcpu/gcpu_hw.c dmx9880/gcpu/gcpu_api.c \
					dmx9880/gcpu/gcpu_if.c dmx9880/include/*.h dmx9880/gcpu/*.h dmx9880/gcpu/iram/*.h dmx9880/drv/dmx_drm_if.c  dmx9880/drv/dmx_dtcp_api.c
else
ifeq ($(TARGET_IC), $(filter $(TARGET_IC),mt5881))
MODULE_FILE_LIST = bsp/hal_io.c ckgen/ckgen.c ckgen/gpio.c ckgen/pinmux.c ckgen/setpll.c inc/x_gpio.h inc/x_pdwnc.h pdwnc5881/*.c pdwnc5881/*.h \
					nand/nandhw_reg.h msdc/msdc_host_hw.h msdc/msdc_slave_hw.h msdc/msdc_drv.h nor/serialflash_hw.c nor/serialflash_hw.h nor/nor_debug.h dmx6896/dmx_aes_reg.h nor/eeprom_hw.c nor/eeprom_if.c \
					sif6896/sif_diag.c sif6896/sif_if.c sif6896/sif_if.h sif6896/sif_isr.c sif6896/sif_hw_v1.c sif6896/sif_hw_v2.c sif6896/sif_debug.h sif6896/sif_hw.h sif6896/sif_isr.h \
					ir6896/ir_ph.c ir6896/ir_hw.c  ir6896/irrx_if.c ir6896/ir_io.h ir6896/ir_debug.h ir6896/ir_ph.h ir6896/ir_reg.h dmx9880/gcpu/gcpu_hw.c dmx9880/gcpu/gcpu_api.c \
					dmx9880/gcpu/gcpu_if.c dmx9880/include/*.h dmx9880/gcpu/*.h dmx9880/gcpu/iram/*.h dmx9880/drv/dmx_drm_if.c  dmx9880/drv/dmx_dtcp_api.c
else
ifeq ($(TARGET_IC), $(filter $(TARGET_IC),mt5399))
MODULE_FILE_LIST = bsp/hal_io.c ckgen/ckgen.c ckgen/gpio.c ckgen/pinmux.c ckgen/setpll.c inc/x_gpio.h inc/x_pdwnc.h pdwnc5399/*.c pdwnc5399/*.h \
					nand/nandhw_reg.h msdc/msdc_host_hw.h msdc/msdc_slave_hw.h msdc/msdc_drv.h nor/serialflash_hw.c nor/serialflash_hw.h nor/nor_debug.h dmx5399/dmx_aes_reg.h nor/eeprom_hw.c nor/eeprom_if.c \
					sif5399/sif_diag.c sif5399/sif_if.c sif5399/sif_if.h sif5399/sif_isr.c sif5399/sif_hw_v1.c sif5399/sif_hw_v2.c sif5399/sif_debug.h sif5399/sif_hw.h sif5399/sif_isr.h \
					ir6896/ir_ph.c ir6896/ir_hw.c  ir6896/irrx_if.c ir6896/ir_io.h ir6896/ir_debug.h ir6896/ir_ph.h ir6896/ir_reg.h dmx5399/gcpu/gcpu_hw.c dmx5399/gcpu/gcpu_api.c \
					dmx5399/gcpu/gcpu_if.c dmx5399/include/*.h dmx5399/gcpu/*.h dmx5399/gcpu/iram/*.h dmx5399/drv/dmx_drm_if.c  dmx5399/drv/dmx_dtcp_api.c
else
ifeq ($(TARGET_IC), $(filter $(TARGET_IC),mt5890))
MODULE_FILE_LIST = bsp/hal_io.c ckgen/ckgen.c ckgen/gpio.c ckgen/pinmux.c ckgen/setpll.c inc/x_gpio.h inc/x_pdwnc.h pdwnc8290/*.c pdwnc8290/*.h \
					nand/nandhw_reg.h msdc/msdc_host_hw.h msdc/msdc_slave_hw.h msdc/msdc_drv.h nor/serialflash_hw.c nor/serialflash_hw.h nor/nor_debug.h dmx8290/dmx_aes_reg.h nor/eeprom_hw.c nor/eeprom_if.c \
					sif5399/sif_diag.c sif5399/sif_if.c sif5399/sif_if.h sif5399/sif_isr.c sif5399/sif_hw_v1.c sif5399/sif_hw_v2.c sif5399/sif_debug.h sif5399/sif_hw.h sif5399/sif_isr.h \
					ir8290_8563/ir_ph.c ir8290_8563/ir_hw.c  ir8290_8563/irrx_if.c  ir8290_8563/ir_debug.h ir8290_8563/ir_ph.h dmx8290/gcpu/gcpu_hw.c dmx8290/gcpu/gcpu_api.c \
					dmx8290/gcpu/gcpu_if.c dmx8290/include/*.h dmx8290/gcpu/*.h dmx8290/gcpu/iram/*.h dmx8290/drv/dmx_drm_if.c  dmx8290/drv/dmx_dtcp_api.c
else
ifeq ($(TARGET_IC), $(filter $(TARGET_IC),mt5891))
MODULE_FILE_LIST = bsp/hal_io.c ckgen/ckgen.c ckgen/gpio.c ckgen/pinmux.c ckgen/setpll.c inc/x_gpio.h inc/x_pdwnc.h pdwnc8290/*.c pdwnc8290/*.h \
					nand/nandhw_reg.h msdc/msdc_host_hw.h msdc/msdc_slave_hw.h msdc/msdc_drv.h nor/serialflash_hw.c nor/serialflash_hw.h nor/nor_debug.h dmx8290/dmx_aes_reg.h nor/eeprom_hw.c nor/eeprom_if.c \
					sif8391/sif_diag.c sif8391/sif_if.c sif8391/sif_if.h sif8391/sif_isr.c sif8391/sif_hw_m0.c sif8391/sif_hw_m1.c sif8391/sif_hw_m2.c sif8391/sif_hw_pdm0.c sif8391/sif_hw_pdm1.c sif8391/sif_hw_secm0.c sif8391/sif_debug.h sif8391/sif_hw.h sif8391/sif_isr.h \
					ir8290_8563/ir_ph.c ir8290_8563/ir_hw.c  ir8290_8563/irrx_if.c  ir8290_8563/ir_debug.h ir8290_8563/ir_ph.h dmx8290/gcpu/gcpu_hw.c dmx8290/gcpu/gcpu_api.c \
					dmx8290/gcpu/gcpu_if.c dmx8290/include/*.h dmx8290/gcpu/*.h dmx8290/gcpu/iram/*.h dmx8290/drv/dmx_drm_if.c  dmx8290/drv/dmx_dtcp_api.c
else
ifeq ($(TARGET_IC), $(filter $(TARGET_IC),mt5882))
MODULE_FILE_LIST = bsp/hal_io.c ckgen/ckgen.c ckgen/gpio.c ckgen/pinmux.c ckgen/setpll.c inc/x_gpio.h inc/x_pdwnc.h pdwnc5399/*.c pdwnc5399/*.h \
					nand/nandhw_reg.h msdc/msdc_host_hw.h msdc/msdc_slave_hw.h msdc/msdc_drv.h nor/serialflash_hw.c nor/serialflash_hw.h nor/nor_debug.h dmx5882/dmx_aes_reg.h nor/eeprom_hw.c nor/eeprom_if.c \
					sif5399/sif_diag.c sif5399/sif_if.c sif5399/sif_if.h sif5399/sif_isr.c sif5399/sif_hw_v1.c sif5399/sif_hw_v2.c sif5399/sif_debug.h sif5399/sif_hw.h sif5399/sif_isr.h \
					ir6896/ir_ph.c ir6896/ir_hw.c  ir6896/irrx_if.c ir6896/ir_io.h ir6896/ir_debug.h ir6896/ir_ph.h ir6896/ir_reg.h dmx8290/gcpu/gcpu_hw.c dmx8290/gcpu/gcpu_api.c \
					dmx8290/gcpu/gcpu_if.c dmx8290/include/*.h dmx8290/gcpu/*.h dmx8290/gcpu/iram/*.h dmx8290/drv/dmx_drm_if.c  dmx8290/drv/dmx_dtcp_api.c
else
ifeq ($(TARGET_IC), $(filter $(TARGET_IC),mt5883))
MODULE_FILE_LIST = bsp/hal_io.c ckgen/ckgen.c ckgen/gpio.c ckgen/pinmux.c ckgen/setpll.c inc/x_gpio.h inc/x_pdwnc.h pdwnc5399/*.c pdwnc5399/*.h \
					nand/nandhw_reg.h msdc/msdc_host_hw.h msdc/msdc_slave_hw.h msdc/msdc_drv.h nor/serialflash_hw.c nor/serialflash_hw.h nor/nor_debug.h dmx8290/dmx_aes_reg.h nor/eeprom_hw.c nor/eeprom_if.c \
					sif8391/sif_diag.c sif8391/sif_if.c sif8391/sif_if.h sif8391/sif_isr.c sif8391/sif_hw_m0.c sif8391/sif_hw_m1.c sif8391/sif_hw_pdm0.c sif8391/sif_hw_pdm1.c sif8391/sif_hw_secm0.c sif8391/sif_debug.h sif8391/sif_hw.h sif8391/sif_isr.h \
					ir6896/ir_ph.c ir6896/ir_hw.c  ir6896/irrx_if.c ir6896/ir_io.h ir6896/ir_debug.h ir6896/ir_ph.h ir6896/ir_reg.h dmx8290/gcpu/gcpu_hw.c dmx8290/gcpu/gcpu_api.c \
					dmx8290/gcpu/gcpu_if.c dmx8290/include/*.h dmx8290/gcpu/*.h dmx8290/gcpu/iram/*.h dmx8290/drv/dmx_drm_if.c  dmx8290/drv/dmx_dtcp_api.c
else
MODULE_FILE_LIST = bsp/hal_io.c ckgen/ckgen.c ckgen/gpio.c ckgen/pinmux.c ckgen/setpll.c inc/x_gpio.h inc/x_pdwnc.h pdwnc/*.c pdwnc/*.h \
					nand/nandhw_reg.h nor/serialflash_hw.c nor/serialflash_hw.h nor/nor_debug.h \
					nor/eeprom_hw.c nor/eeprom_if.c sif/sif_diag.c sif/sif_if.c sif/sif_if.h sif/sif_debug.h sif/sif_hw.h \
					ir/ir_ph.c ir/ir_hw.c ir/irrx_if.c ir/ir_io.h ir/ir_debug.h ir/ir_ph.h ir/ir_reg.h
endif #83
endif #82
endif #90
endif #91
endif #81
endif #98
endif #99
endif #96
endif #95
endif

export BUILD_TARGET_OBJ_ROOT := $(OBJ_ROOT)/uboot
export UBOOT_OBJ_ROOT     := $(shell mkdir -p $(BUILD_TARGET_OBJ_ROOT)/$(UBOOT_DIR) && cd $(BUILD_TARGET_OBJ_ROOT)/$(UBOOT_DIR) && /bin/pwd)
export UBOOT_PACK_ROOT     := $(shell mkdir -p $(BUILD_TARGET_OBJ_ROOT)/pack && cd $(BUILD_TARGET_OBJ_ROOT)/pack && /bin/pwd)
export UBOOT_SECURE_PACK_ROOT     := $(shell mkdir -p $(BUILD_TARGET_OBJ_ROOT)/secure && cd $(BUILD_TARGET_OBJ_ROOT)/secure && /bin/pwd)
export MTKLOADER_OBJ_ROOT := $(shell mkdir -p $(BUILD_TARGET_OBJ_ROOT)/mtkloader && cd $(BUILD_TARGET_OBJ_ROOT)/mtkloader && /bin/pwd)
export O                  := $(UBOOT_OBJ_ROOT)

export DRV_INC_ROOT := $(VM_LINUX_ROOT)/linux_core/driver/ko/$(TARGET_IC)/$(TARGET_DRIVER)/drv_inc

MODULE_DIR_LIST_TXT=  $(UBOOT_OBJ_ROOT)/module_dir_list.txt



.PHONY:	drv_lib uboot_cfg pre_link uboot_linux mtkloader preloader_linux packing_linux packing_secure_linux

#ALL = clean

INIT = drv_lib

INIT += pre_link

ALL =

ALL += uboot_linux
ALL += mtkloader

#DRV_FILE_LIST = $(UTARGET_ROOT)/drv_file_list.txt

all:
	$(MAKE) $(if $(filter -j,$(MAKEFLAGS)),,-j $(JOBS)) uboot_init
	$(MAKE) packing_linux
ifeq ($(SECURE_BOOT),y)
	$(MAKE) packing_secure_linux
endif

uboot_init: $(INIT)

pre_link:
	@echo 'do nothing'

pre_linka :
	@echo "BUILD_UBOOT_$@_START_TIME   = `$(DATE) $(DATE_FLAG)`"
	@if [ ! -d $(UTARGET_ROOT) ]; then \
		mkdir -p $(UTARGET_ROOT); \
	fi

	rm -rf $(DRV_FILE_LIST).need_to_do_link
	if [ ! -e $(DRV_FILE_LIST) ]; then \
		cd $(TARGET_ROOT)/; \
		find . -type l -o -type f -follow 2>&1 |uniq |sed -r "s/[\`\']//g"|sed -r 's/find: //g; s/:.*//g'|grep -v .o.cmd |grep -v modules.order > $(DRV_FILE_LIST); \
		touch $(DRV_FILE_LIST).need_to_do_link;\
	else \
		cd $(TARGET_ROOT)/; \
		find . -type l -o -type f -follow 2>&1 |sed -r 's/find: //g; s/:.*//g'|grep -v .o.cmd |grep -v modules.order > $(DRV_FILE_LIST).tmp; \
		if [ "`diff -q $(DRV_FILE_LIST).tmp $(DRV_FILE_LIST) 2>/dev/null`" != '' ]; then \
			cp $(DRV_FILE_LIST).tmp $(DRV_FILE_LIST); \
			touch $(DRV_FILE_LIST).need_to_do_link; \
			rm -rf $(DRV_FILE_LIST).tmp; \
		fi; \
	fi

	$(MAKE) -f ./symlink.mak $(if $(filter -j,$(MAKEFLAGS)),,-j $(JOBS)) --no-print-directory
	@rm -rf $(DRV_FILE_LIST).need_to_do_link
	@echo "BUILD_UBOOT_$@_END_TIME   = `$(DATE) $(DATE_FLAG)`"

uboot_cfg:
	@echo "BUILD_UBOOT_$@_START_TIME   = `$(DATE) $(DATE_FLAG)`"
	$(MAKE) -C $(UBOOT_DIR) $(TARGET_IC)_config O=$(UBOOT_OBJ_ROOT)
	$(call if_file_notexist_w_sym_fct, $(DRV_INC_ROOT), $(DRVLIB_DRV_INC_DIR));
	@echo "BUILD_UBOOT_$@_END_TIME   = `$(DATE) $(DATE_FLAG)`"

packing_linux:$(ALL)

	@echo "Do packing..."
	$(MAKE) -C $(PACK_DIR) 5365_package MODEL=$(MODEL) BOARD=$(TARGET_IC) TARGET_IC=$(TARGET_IC)

packing_secure_linux:
	@echo "Do packing secure boot loader..."
	$(MAKE) -C $(SECURE_DIR) 5365_package MODEL=$(MODEL) BOARD=$(TARGET_IC) TARGET_IC=$(TARGET_IC)


clean_packing_secure_linux:
	$(MAKE) -C $(SECURE_DIR) clean MODEL=$(MODEL) BOARD=$(TARGET_IC) TARGET_IC=$(TARGET_IC)

preloader_linux:
	@echo "Build preloader..."
	$(MAKE) -C $(DRVLIB_DRVCUST_DIR) clean BOARD=$(TARGET_IC) SECURE_BOOT=$(SECURE_BOOT)
ifeq "$(TARGET_IC)" "mt5392b"
	$(MAKE) -C $(DRVLIB_DRVCUST_DIR) PRELOADER_DEF=CC_5391_PRELOADER BOARD=$(TARGET_IC) SECURE_BOOT=$(SECURE_BOOT)
endif
ifeq "$(TARGET_IC)" "mt5363"
	$(MAKE) -C $(DRVLIB_DRVCUST_DIR) PRELOADER_DEF=CC_MTK_PRELOADER BOARD=$(TARGET_IC) SECURE_BOOT=$(SECURE_BOOT)
endif
	$(MAKE) -C $(PRELOADER_DIR) clean MODEL=$(MODEL) BOARD=$(TARGET_IC) SECURE_BOOT=$(SECURE_BOOT)
	$(MAKE) -C $(PRELOADER_DIR) MODEL=$(MODEL) BOARD=$(TARGET_IC) SECURE_BOOT=$(SECURE_BOOT)
	@cd $(PRELOADER_DIR)
ifeq ($(BOOT),nand)
ifeq  ($(ROM_CODE),y)
	$(LD) -Bstatic -T $(PRELOADER_DIR)/rom2nand.lds \
	--start-group $(PRELOADER_DIR)/libpreloader.a $(DRVLIB_DRVCUST_DIR)/libdrv_cust.a  \
	--end-group $(PLATFORM_LIBS) -Map $(MODEL)_preloader_$(BOOT).map -o $(MODEL)_preloader_$(BOOT)
	$(OBJCOPY) ${OBJCFLAGS} -O binary $(MODEL)_preloader_$(BOOT) $(MODEL)_preloader_$(BOOT).bin
else
	$(LD) -Bstatic -T $(PRELOADER_DIR)/nand.lds \
	--start-group $(PRELOADER_DIR)/libpreloader.a $(DRVLIB_DRVCUST_DIR)/libdrv_cust.a  \
	--end-group $(PLATFORM_LIBS) -Map $(MODEL)_preloader_$(BOOT).map -o $(MODEL)_preloader_$(BOOT)
	$(OBJCOPY) ${OBJCFLAGS} -O binary $(MODEL)_preloader_$(BOOT) $(MODEL)_preloader_$(BOOT).bin
endif
else
	$(LD) -Bstatic -T $(PRELOADER_DIR)/nor.lds \
	--start-group $(PRELOADER_DIR)/libpreloader.a $(DRVLIB_DRVCUST_DIR)/libdrv_cust.a  \
	--end-group $(PLATFORM_LIBS) -Map $(MODEL)_preloader_$(BOOT).map -o $(MODEL)_preloader_$(BOOT)
	$(OBJCOPY) ${OBJCFLAGS} -O binary $(MODEL)_preloader_$(BOOT) $(MODEL)_preloader_$(BOOT).bin
endif
	mv $(MODEL)_preloader_$(BOOT)* preloader

uboot_linux: uboot_cfg
	@echo "BUILD_UBOOT_$@_START_TIME   = `$(DATE) $(DATE_FLAG)`"
	$(MAKE) -C $(UBOOT_DIR) all TARGET_IC=$(TARGET_IC) $(if $(filter -j,$(MAKEFLAGS)),,-j $(JOBS)) $(SHOW_BUILD_COMMAND) --no-print-directory O=$(UBOOT_OBJ_ROOT) 2>&1 | tee $(UBOOT_OBJ_ROOT)/make.log
	@$(call chk_err_fct, $(UBOOT_OBJ_ROOT)/make.log);
	@rm -f $(UBOOT_OBJ_ROOT)/make.log $(UBOOT_OBJ_ROOT)/temp.log
	@rm -f $(UBOOT_OBJ_ROOT)/u-boot.srec
	@echo "BUILD_UBOOT_$@_END_TIME   = `$(DATE) $(DATE_FLAG)`"

drv_lib:
	@echo "BUILD_UBOOT_$@_START_TIME   = `$(DATE) $(DATE_FLAG)`"
	@echo "TARGET_ROOT:$(TARGET_ROOT) TARGET_DRIVER:$(TARGET_DRIVER)"
	echo TARGET_IC: $(TARGET_IC)
	mkdir -p $(DRVLIB_DRVCUST_DIR)
	@if [ ! -h $(UBOOT_DIR)/include/configs/$(TARGET_IC)/drv_cust ]; then \
		echo "Symlink: drv_cust"; \
		mkdir -p $(UBOOT_DIR)/include/configs/$(TARGET_IC) ; \
		$(call if_file_notexist_w_sym_fct, $(DRVLIB_DRVCUST_DIR) , $(ROOTDIR)/$(UBOOT_DIR)/include/configs/$(TARGET_IC)/drv_cust) ; \
	fi
	@if [ ! -h $(ROOTDIR)/$(UBOOT_DIR)/board ]; then \
		echo "Symlink: board"; \
		$(call if_file_notexist_w_sym_fct, $(ROOTDIR)/board , $(ROOTDIR)/$(UBOOT_DIR)/board) ; \
	fi
	@if [ ! -e $(ROOTDIR)/$(UBOOT_DIR)/board/$(TARGET_IC)/drv_lib ]; then \
		mkdir -p $(ROOTDIR)/$(UBOOT_DIR)/board/$(TARGET_IC)/drv_lib ; \
	fi
	for i in `cd $(ROOTDIR)/drv_lib/$(TARGET_IC) ; ls ` ; do  \
		if [ ! -h $(ROOTDIR)/$(UBOOT_DIR)/board/$(TARGET_IC)/drv_lib/$$i ]; then \
			echo "Symlink: drv_lib $$i ";\
	  $(call if_file_notexist_w_sym_fct, $(ROOTDIR)/drv_lib/$(TARGET_IC)/$$i , $(ROOTDIR)/$(UBOOT_DIR)/board/$(TARGET_IC)/drv_lib/$$i) ; \
	fi ; \
	done

	@if [ ! -e $(UBOOT_INC_DIR)/linux/module.h ]; then \
		mkdir -p $(UBOOT_INC_DIR)/linux; \
		touch $(UBOOT_INC_DIR)/linux/module.h; \
	fi

	@if [ ! -h $(DRV_LIB_DIR)/include/d_drv_cust.h ]; then \
		echo "Symlink: d_drv_cust.h"; \
		$(call if_file_notexist_w_sym_fct, $(PROJECT_ROOT)/driver/target/public/d_inc/d_drv_cust.h , $(DRV_LIB_DIR)/include/d_drv_cust.h ) ; \
	fi

	@mkdir -p $(DRVLIB_NPTV_DIR)

	@mkdir -p $(BOARDDIR)/module
	@for i in $(MODULE_FILE_LIST); do \
		for j in $(TARGET_ROOT)/$(TARGET_DRIVER)/$$i; do \
			if [ ! -e $$j -a ! -h $$j ]; then \
				echo $$j does not exist, could not do symlink; \
			else \
				FILE_NAME=`basename $$j`; \
				if [ ! -h $(BOARDDIR)/module/$$FILE_NAME ]; then \
					echo "Symlink : $$j $(BOARDDIR)/module/$$FILE_NAME"; \
					$(call if_file_notexist_w_sym_fct, $$j , $(BOARDDIR)/module/$$FILE_NAME ) ; \
				fi; \
			fi; \
		done; \
	done
	@if [ ! -h $(DRV_LIB_DIR)/include/$(MODEL).def ]; then \
		echo "Symlink: $(MODEL).def"; \
		$(call if_file_notexist_w_sym_fct, $(TARGET_ROOT)/drv_opt/$(COMPANY)/$(MODEL).def , $(DRV_LIB_DIR)/include/$(MODEL).def ) ; \
	fi

	$(call if_file_notexist_w_sym_fct, $(BOARDDIR)/../mt53xx_com, $(BOARDDIR)/common);
	$(call if_file_notexist_w_sym_fct, $(DRV_INC_ROOT), $(TARGET_ROOT)/$(TARGET_DRIVER)/drv_inc);

	@cd $(TARGET_ROOT)/$(TARGET_DRIVER) && \
	find $(MODULE_DIR_LIST) -type f -o -type l|grep -v '\.[oa]$$' |grep -v '\.o\.cmd$$' > $(MODULE_DIR_LIST_TXT).tmp

	@if [ ! -e $(MODULE_DIR_LIST_TXT) ]; then \
		touch $(MODULE_DIR_LIST_TXT).need.to.do.link; \
		cp -au $(MODULE_DIR_LIST_TXT).tmp $(MODULE_DIR_LIST_TXT); \
	else \
		if [ "`diff -q $(MODULE_DIR_LIST_TXT).tmp $(MODULE_DIR_LIST_TXT) 2>/dev/null`" != '' ]; then \
			touch $(MODULE_DIR_LIST_TXT).need.to.do.link; \
			cp -au $(MODULE_DIR_LIST_TXT).tmp $(MODULE_DIR_LIST_TXT); \
		fi; \
	fi
	@rm -rf $(MODULE_DIR_LIST_TXT).tmp

	@if [ -e $(MODULE_DIR_LIST_TXT).need.to.do.link ]; then \
		for j in `cd $(TARGET_ROOT)/$(TARGET_DRIVER); find $(MODULE_DIR_LIST) -type f -o -type l|grep -v '\.[oa]$$' |grep -v '\.o\.cmd$$'`; do \
			FILE_NAME=`basename $$j`; \
			DIR_NAME=`dirname $$j`; \
			if [ ! -h $(DRV_LIB_DIR)/$(TARGET_IC)/$$j ]; then \
				echo "Symlink: $$FILE_NAME"; \
				mkdir -p $(DRV_LIB_DIR)/$(TARGET_IC)/$$DIR_NAME; \
				$(call if_file_notexist_w_sym_fct, $(TARGET_ROOT)/$(TARGET_DRIVER)/$$j , $(DRV_LIB_DIR)/$(TARGET_IC)/$$j ) ; \
			fi;\
		done; \
		rm -rf $(MODULE_DIR_LIST_TXT).need.to.do.link; \
	fi

	@mkdir -p $(DRVLIB_NPTV_DIR)
	@for i in $(NPTV_DIR_LIST); do \
		cp -rfL  $(TARGET_ROOT)/$(TARGET_DRIVER)/$(TARGET_IC)/nptv/$$i $(DRVLIB_NPTV_DIR); \
	done

	@mkdir -p ./preloader/$(TARGET_IC)
	@for i in $(PRELOADER_FILE_LIST); do \
		FILE_NAME=`basename $$i`; \
		if [ ! -h ./preloader/$(TARGET_IC)/$$FILE_NAME ]; then \
			echo "Symlink: $$FILE_NAME"; \
			$(call if_file_notexist_w_sym_fct, $$i , $(ROOTDIR)/preloader/$(TARGET_IC)/$$FILE_NAME ) ; \
		fi; \
	done

	@if [ ! -h ./preloader/$(TARGET_IC)/preloader_main.c ]; then \
		echo "Symlink: preloader_main.c"; \
		$(call if_file_notexist_w_sym_fct, $(TARGET_ROOT)/preloader/preloader_main.c , $(ROOTDIR)/preloader/$(TARGET_IC)/preloader_main.c ) ; \
	fi

ifeq ($(TARGET_IC), $(filter $(TARGET_IC), mt5396 mt5368 mt5389))
	@for i in `cd $(TARGET_ROOT)/$(TARGET_DRIVER)/nptv6896/inc; find . -type f -o -type l -follow`; do \
		FILE_NAME=`basename $$i`; \
		DIR_NAME=`dirname $$i`; \
		if [ ! -h $(DRVLIB_NPTV_DIR)/inc/$$i ]; then \
			echo "Symlink $$i"; \
			mkdir -p $(DRVLIB_NPTV_DIR)/inc/$$DIR_NAME; \
			$(call if_file_notexist_w_sym_fct, $(TARGET_ROOT)/$(TARGET_DRIVER)/nptv6896/inc/$$i , $(DRVLIB_NPTV_DIR)/inc/$$i ) ; \
		fi; \
	done
else
ifeq ($(TARGET_IC), $(filter $(TARGET_IC), mt5398 mt5880))
	@for i in `cd $(TARGET_ROOT)/$(TARGET_DRIVER)/nptv8098/inc; find . -type f -o -type l -follow`; do \
		FILE_NAME=`basename $$i`; \
		DIR_NAME=`dirname $$i`; \
		if [ ! -h $(DRVLIB_NPTV_DIR)/inc/$$i ]; then \
			echo "Symlink: $$i"; \
			mkdir -p $(DRVLIB_NPTV_DIR)/inc/$$DIR_NAME; \
			$(call if_file_notexist_w_sym_fct, $(TARGET_ROOT)/$(TARGET_DRIVER)/nptv8098/inc/$$i , $(DRVLIB_NPTV_DIR)/inc/$$i ) ; \
		fi; \
	done
else
ifeq ($(TARGET_IC), $(filter $(TARGET_IC),mt5881))
	@for i in `cd $(TARGET_ROOT)/$(TARGET_DRIVER)/nptv5881/inc; find . -type f -o -type l -follow`; do \
		FILE_NAME=`basename $$i`; \
		DIR_NAME=`dirname $$i`; \
		if [ ! -h $(DRVLIB_NPTV_DIR)/inc/$$i ]; then \
			echo "Symlink: $$i"; \
			mkdir -p $(DRVLIB_NPTV_DIR)/inc/$$DIR_NAME; \
			$(call if_file_notexist_w_sym_fct, $(TARGET_ROOT)/$(TARGET_DRIVER)/nptv5881/inc/$$i , $(DRVLIB_NPTV_DIR)/inc/$$i ) ; \
		fi; \
	done
else
ifeq ($(TARGET_IC), $(filter $(TARGET_IC),mt5399))
	@for i in `cd $(TARGET_ROOT)/$(TARGET_DRIVER)/nptv5399/inc; find . -type f -o -type l -follow`; do \
		FILE_NAME=`basename $$i`; \
		DIR_NAME=`dirname $$i`; \
		if [ ! -h $(DRVLIB_NPTV_DIR)/inc/$$i ]; then \
			echo "Symlink: $$i"; \
			mkdir -p $(DRVLIB_NPTV_DIR)/inc/$$DIR_NAME; \
			$(call if_file_notexist_w_sym_fct, $(TARGET_ROOT)/$(TARGET_DRIVER)/nptv5399/inc/$$i , $(DRVLIB_NPTV_DIR)/inc/$$i ) ; \
		fi; \
	done
else
ifeq ($(TARGET_IC), $(filter $(TARGET_IC), mt5365 mt5395))
	@for i in `cd $(TARGET_ROOT)/$(TARGET_DRIVER)/nptv6595/inc; find . -type f -o -type l -follow`; do \
		FILE_NAME=`basename $$i`; \
		DIR_NAME=`dirname $$i`; \
		if [ ! -h $(DRVLIB_NPTV_DIR)/inc/$$i ]; then \
			echo "Symlink: $$i"; \
			mkdir -p $(DRVLIB_NPTV_DIR)/inc/$$DIR_NAME; \
			$(call if_file_notexist_w_sym_fct, $(TARGET_ROOT)/$(TARGET_DRIVER)/nptv6595/inc/$$i , $(DRVLIB_NPTV_DIR)/inc/$$i ) ; \
		fi; \
	done
else
ifeq ($(TARGET_IC), $(filter $(TARGET_IC),mt5890))
	@for i in `cd $(TARGET_ROOT)/$(TARGET_DRIVER)/nptv8290/inc; find . -type f -o -type l -follow`; do \
		FILE_NAME=`basename $$i`; \
		DIR_NAME=`dirname $$i`; \
		if [ ! -h $(DRVLIB_NPTV_DIR)/inc/$$i ]; then \
			echo "Symlink: $$i"; \
			mkdir -p $(DRVLIB_NPTV_DIR)/inc/$$DIR_NAME; \
			$(call if_file_notexist_w_sym_fct, $(TARGET_ROOT)/$(TARGET_DRIVER)/nptv8290/inc/$$i , $(DRVLIB_NPTV_DIR)/inc/$$i ) ; \
		fi; \
	done
else
ifeq ($(TARGET_IC), $(filter $(TARGET_IC),mt5891))
	@for i in `cd $(TARGET_ROOT)/$(TARGET_DRIVER)/nptv5891/inc; find . -type f -o -type l -follow`; do \
		FILE_NAME=`basename $$i`; \
		DIR_NAME=`dirname $$i`; \
		if [ ! -h $(DRVLIB_NPTV_DIR)/inc/$$i ]; then \
			echo "Symlink: $$i"; \
			mkdir -p $(DRVLIB_NPTV_DIR)/inc/$$DIR_NAME; \
			$(call if_file_notexist_w_sym_fct, $(TARGET_ROOT)/$(TARGET_DRIVER)/nptv5891/inc/$$i , $(DRVLIB_NPTV_DIR)/inc/$$i ) ; \
		fi; \
	done
else
ifeq ($(TARGET_IC), $(filter $(TARGET_IC),mt5882 mt5883))
	@for i in `cd $(TARGET_ROOT)/$(TARGET_DRIVER)/nptv5399/inc; find . -type f -o -type l -follow`; do \
		FILE_NAME=`basename $$i`; \
		DIR_NAME=`dirname $$i`; \
		if [ ! -h $(DRVLIB_NPTV_DIR)/inc/$$i ]; then \
			echo "Symlink: $$i"; \
			mkdir -p $(DRVLIB_NPTV_DIR)/inc/$$DIR_NAME; \
			$(call if_file_notexist_w_sym_fct, $(TARGET_ROOT)/$(TARGET_DRIVER)/nptv5399/inc/$$i , $(DRVLIB_NPTV_DIR)/inc/$$i ) ; \
		fi; \
	done
else
	@for i in `cd $(TARGET_ROOT)/$(TARGET_DRIVER)/nptv/inc; find . -type f -o -type l`; do \
		FILE_NAME=`basename $$i`; \
		DIR_NAME=`dirname $$i`; \
		if [ ! -h $(DRVLIB_NPTV_DIR)/inc/$$i ]; then \
			echo "Symlink: $$i"; \
			mkdir -p $(DRVLIB_NPTV_DIR)/inc/$$DIR_NAME; \
			$(call if_file_notexist_w_sym_fct, $(TARGET_ROOT)/$(TARGET_DRIVER)/nptv/inc/$$i , $(DRVLIB_NPTV_DIR)/inc/$$i ) ; \
		fi; \
	done
endif #82
endif #90
endif #81
endif #91
endif #98
endif #95
endif #96
endif #99

	@echo "BUILD_UBOOT_$@_END_TIME   = `$(DATE) $(DATE_FLAG)`"

mtkloader:
	@echo "BUILD_UBOOT_$@_START_TIME   = `$(DATE) $(DATE_FLAG)`"
	$(MAKE) -f ./mtkloader.mak IC_NAME=$(subst mt,,$(TARGET_IC))
	@echo "BUILD_UBOOT_$@_END_TIME   = `$(DATE) $(DATE_FLAG)`"

clean_mtkloader:
	$(MAKE) -f ./mtkloader.mak IC_NAME=$(subst mt,,$(TARGET_IC)) clean

clean: clean_packing_secure_linux clean_mtkloader
	$(FCLEAN) $(MTKLOADER_OBJ_ROOT) $(UBOOT_OBJ_ROOT)
	rm -rf $(DRV_LIB_DIR)/$(TARGET_IC) $(ROOTDIR)/board/$(TARGET_IC)/drv_lib $(ROOTDIR)/board/$(TARGET_IC)/module $(ROOTDIR)/board/$(TARGET_IC)/common
	rm -rf $(UBOOT_DIR)/board $(UBOOT_DIR)/include/configs/$(TARGET_IC)/drv_cust
	@if [ -e $(SYMLINK_LOG) ] ; then \
		grep $(ROOTDIR) $(SYMLINK_LOG) | xargs rm -rf ; \
	fi
	rm -rf $(ROOTDIR)/*.bin
	rm -rf $(ROOTDIR)/board/$(TARGET_IC)/.depend
	rm -rf $(ROOTDIR)/board/$(TARGET_IC)/lib*.a
	rm -rf $(ROOTDIR)/board/$(TARGET_IC)/*.o
	rm -rf $(ROOTDIR)/board/$(TARGET_IC)/*.cmd
	rm -rf $(ROOTDIR)/board/$(TARGET_IC)/module/*.o
	rm -rf $(ROOTDIR)/board/$(TARGET_IC)/module/*.cmd
	rm -rf $(ROOTDIR)/pack/*.bin
	rm -rf $(ROOTDIR)/pack/.dummy.hdr
	rm -rf $(ROOTDIR)/pack/packimg
	rm -rf $(ROOTDIR)/pack/temp1.sc
	rm -rf $(ROOTDIR)/pack/temp2.sc
	rm -rf $(ROOTDIR)/pack/u-boot_pack.lzhs
	rm -rf $(DRV_LIB_DIR)/include/linux/module.h
	rm -rf $(THIS_ROOT)/bin/* $(THIS_ROOT)/obj/*
	rm -rf $(MODULE_DIR_LIST_TXT)
	rm -rf $(ROOTDIR)/*.bin
	rm -rf $(ROOTDIR)/board/$(TARGET_IC)/.depend
	rm -rf $(ROOTDIR)/board/$(TARGET_IC)/*.d
	rm -rf $(ROOTDIR)/board/$(TARGET_IC)/lib*.a
	rm -rf $(ROOTDIR)/board/$(TARGET_IC)/*.o
	rm -rf $(ROOTDIR)/board/$(TARGET_IC)/*.cmd
	rm -rf $(ROOTDIR)/board/$(TARGET_IC)/module/*.o
	rm -rf $(ROOTDIR)/board/$(TARGET_IC)/module/*.cmd
	rm -rf $(ROOTDIR)/pack/*.bin
	rm -rf $(ROOTDIR)/pack/.dummy.hdr
	rm -rf $(ROOTDIR)/pack/packimg
	rm -rf $(ROOTDIR)/pack/temp1.sc
	rm -rf $(ROOTDIR)/pack/temp2.sc
	rm -rf $(ROOTDIR)/pack/u-boot_pack.lzhs
	rm -rf $(DRV_LIB_DIR)/include/linux
	rm -rf $(THIS_ROOT)/bin/* $(THIS_ROOT)/obj/*
	rm -rf $(MODULE_DIR_LIST_TXT)
	rm -rf preloader
	rm -rf $(ROOTDIR)/board/$(TARGET_IC)/module
	rm -rf $(UBOOT_DIR)/include/configs/$(TARGET_IC)
