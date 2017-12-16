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
ifndef UBOOT_ROOT
export UBOOT_ROOT=$(word 1, $(subst /uboot/,/uboot /, $(shell bash -c pwd -L)))
endif

ifndef CROSS_COMPILE
export CROSS_COMPILE = arm11_mtk_le-
endif

ifndef COMPANY
export COMPANY = mtk
endif

ifndef MODEL
export MODEL = common
endif

ifndef DRV_CUSTOM
DRV_CUSTOM = $(MODEL)
endif

#ifndef BOOT_TYPE
#BOOT_TYPE = ROM2NAND
#endif

ifndef ROM_CODE
ROM_CODE = n
endif

ifneq ($(strip $(wildcard $(UBOOT_ROOT)/drv_lib/include/$(MODEL).def)),)
-include $(UBOOT_ROOT)/drv_lib/include/$(MODEL).def
endif

ifeq "$(BOOT_TYPE)" "ROM2NAND"
ROM_CODE = y
BOOT = nand
CPPFLAGS += -DCC_NAND_BOOT
CFLAGS += -DCC_NAND_BOOT
AFLAGS += -DCC_NAND_BOOT
endif

ifeq "$(BOOT_TYPE)" "NAND"
BOOT = nand
CPPFLAGS += -DCC_NAND_BOOT
CFLAGS += -DCC_NAND_BOOT
AFLAGS += -DCC_NAND_BOOT
endif


ifeq "$(BOOT_TYPE)" "NOR"
BOOT = nor
endif

ifeq "$(BOOT_TYPE)" "ROM2NOR"
ROM_CODE = y
BOOT = nor
endif

ifeq "$(BOOT_TYPE)" "ROM2EMMC"
ROM_CODE = y
BOOT = emmc
endif

ifeq "$(SECURE_BOOT)" "y"
export CC_SECURE_ROM_DEF = -DCC_SECURE_ROM_BOOT
endif

export CPPFLAGS CFLAGS AFLAGS 
export ROM_CODE BOOT
export UBOOT_LIBRARY = y

ifeq "$(BOARD)" "mt5392b"
export CC_LOADER_DEF = CC_5391_LOADER -DCC_5392B_LOADER -DCC_53xx_LOADER
export CC_PRELOADER_DEF = CC_5391_PRELOADER -DCC_5392B_PRELOADER
endif

ifeq "$(BOARD)" "mt5363"
export CC_LOADER_DEF = CC_MTK_LOADER
export CC_PRELOADER_DEF = CC_MTK_PRELOADER
endif

COMMON_INC =  -I$(ROOTDIR)/drv_lib/include \
              -I$(ROOTDIR)/drv_lib/$(TARGET_IC)/drv_inc \
              -I$(ROOTDIR)/drv_lib/$(TARGET_IC)/inc \
	             -I$(ROOTDIR)/drv_lib/$(TARGET_IC)/drv_cust

ifdef UBOOT_INC_DIR
COMMON_INC += -I$(UBOOT_INC_DIR)
endif
COMMON_INC += -I$(INC_ROOT)/x_inc

OSAI_INC =     -I. \
               -I$(ROOTDIR)/drv_lib/$(TARGET_IC)/nptv/inc \
               -I$(ROOTDIR)/drv_lib/$(TARGET_IC)/nptv/inc/hw \
               -I$(ROOTDIR)/drv_lib/$(TARGET_IC)/nptv/inc/drv \
               -I$(ROOTDIR)/drv_lib/$(TARGET_IC)/nptv/inc/sys \
               -D_CPU_LITTLE_ENDIAN_ -DCC_UBOOT \
               -DPANEL_TABLE_CUST_FILE=\"$(COMPANY)/panel_table_cust.c\" 

ifeq ($(UBOOT_VERSION), 1.3.4)
 OSAI_INC +=  -DCUSTOM_CFG_FILE=\"$(COMPANY)/config/$(DRV_CUSTOM).h\" 
endif
ifneq ($(strip $(wildcard $(UBOOT_ROOT)/drv_lib/include/$(MODEL).def)),)
OSAI_INC += $(DEFINES)
endif

ifeq "$(BOARD)" "mt5392b"
OSAI_INC += -DCC_MT5391 -DCC_MT5392B
endif

ifeq "$(BOARD)" "mt5363"
OSAI_INC += -DCC_MT5363
endif

export COMMON_INC OSAI_INC

AS	?= $(CROSS_COMPILE)as
LD	?= $(CROSS_COMPILE)ld
CC	?= $(CROSS_COMPILE)gcc
AR	?= $(CROSS_COMPILE)ar
OBJCOPY ?= $(CROSS_COMPILE)objcopy

export AS LD CC AR OBJCOPY
