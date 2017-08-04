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
# $RCSfile: target.mak,v $
# $Revision: #5 $
# $Date: 2017/03/13 $
# $Author: yujuan.qi $
# $SWAuthor: Yan Wang $
#
# Description:
#         Compiler tool parameters to be included in the make process.
#
#         Note: this file provide the default compile, loader, utility
#         commands and their options for the default developer environment
#         (Linux/GNUWIN).
#
#         For a specific TRAGET build, the TARGET directory needs to
#         provide a target specific target.mak which specifies the
#         compiling tools for that target.
#
#############################################################################

ifeq "$(TOOL_CHAIN)" "4.9.1"
export ZLIB_VERSION       ?= 1.2.5
else
export ZLIB_VERSION       ?= 1.2.3
endif

#export FREETYPE_VERSION   ?= 2.3.9
ifeq "$(ICU_SUPPORT)" "true"
export FREETYPE_VERSION   ?= 2.3.9
else
export FREETYPE_VERSION   ?= 2.4.3
endif
export JPEG_VERSION       ?= 6b
ifeq "$(TOOL_CHAIN)" "4.9.1"
export PNG_VERSION        ?= 1.2.46
else
export PNG_VERSION        ?= 1.2.43
endif
ifneq (,$(findstring $(TOOL_CHAIN), 4.9.1 4.9.2))
export CARES_VERSION        ?= 1.10.0
else
export CARES_VERSION        ?= 1.12.0
endif
ifeq "$(TOOL_CHAIN)" "4.9.1"
export EXPAT_VERSION      ?= 2.1.0
else
export EXPAT_VERSION      ?= 2.0.1
endif
export CURL_VERSION       ?= 7.30.0

ifeq "$(TOOL_CHAIN)" "4.9.1"
export OPENSSL_VERSION    ?= 1.0.1m
else
export OPENSSL_VERSION    ?= 1.0.1j
endif

export LIBICONV_VERSION   ?= 1.11.1
export SAMBA_VERSION      ?= 3.0.37
export QT_VERSION         ?= 4.7.0
export DBUS_VERSION       ?= 1.0
export ALSA_VERSION       ?= 1.0.24.1
ifeq "$(ENABLE_WIFI_REMOTE)" "true"
export LIBUSB_VERSION     ?= 1.0.9
else
export LIBUSB_VERSION     ?= 1.0.8
endif
export EFENCE_VERSION     ?= 2.1.13
export BLUEZ_VERSION      ?= 4.70
export LIBXML2_VERSION    ?= 2.7.8
export LIBMXML_VERSION    ?= 1.5
export BUSYBOX_VERSION    ?= 1.15.3
export DOSFSTOOLS_VERSION    ?= 2.9
export EXFAT_VERSION      ?= 0.9.4
export FUSE_VERSION       ?= 2.8.4
export LIBMTP_VERSION     ?= 1.0.1
export LIBUSB_COMPAT_VERSION ?= 0.1.3
export MTPFS_VERSION      ?= 0.9.orig
export NTFS3G_VERSION     ?= 2010.5.22
export NTFSPROGS_VERSION   ?= 2.0.0
export MNG_VERSION				?= 1.0.10
export WEBP_VERSION				?= 0.2.1
export OGG_VERSION 				?= 1.3.2
export DIBBLER_VERSION   ?= 0.8.0
export GLIBC_VERSION   ?= 2.12.2
export WGET_VERSION      ?= 1.10.2
export IPTABLES_VERSION     ?= 1.4.15
export E2FSPROGS_VERSION ?= 1.41.14
export SQLITE_VERSION    ?= 3.7.2
export MTK_SQLITE_VERSION    ?= 3.8.4.3
export E2FSPROGS_VERSION ?= 1.41.14
export DIRECTFB_VERSION  ?= 1.5.3
export LVM2_VERSION ?= 2.02.89
export POPT_VERSION ?= 1.16
export GDISK_VERSION ?= 0.8.1
ifeq "$(ICU_SUPPORT)" "true"
export ICU_VERSION := 4.0
else
export ICU_VERSION := 51.1
endif
export RLOG_VERSION ?= 1.4
export BOOST_VERSION ?= 1.51.0
export ENCFS_VERSION ?= 1.3.2
export OPENSSH_VERSION ?= 6.3

#
# OpenBox - start
#
export BASH_VER             ?= bash-3.2.48
export COREUTIL_VER         ?= coreutils-6.9
export FINDUTIL_VER         ?= findutils-4.2.31
export GAWK_VER             ?= gawk-3.1.5
export GREP_VER             ?= grep-2.5.1a
export GZIP_VER             ?= gzip-1.3.12
export INETUTIL_VER         ?= inetutils-1.4.2
export MODULE_INIT_VER      ?= module-init-tools-3.12
export NCURSES_VER          ?= ncurses-5.7
export PROCPS_VER           ?= procps-3.2.8
export PSMISC_VER           ?= psmisc-22.13
export SED_VER              ?= sed-4.1.5
export TAR_VER              ?= tar-1.17
export UTIL_LINUX_NG_VER    ?= util-linux-ng-2.18
export NET_TOOLS_VER        ?= net-tools-1.60
export IPUTILS_VER          ?= iputils-s20101006
export UDHCP_VER            ?= v0.0
export THTTPD_VER           ?= 2.25b
export QRENCODE_VER			?= 3.4.2

#
# OpenBox - end
#
