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
# $RCSfile: customer.mak,v $
# $Revision: #42 $
# $Date: 2010/03/18 $
# $Author: brianpc.huang $
# $SWAuthor: Yan Wang $
#
# Description:
#         Makefile to build a "MT-5351" demo board build. The following
#         targets are supported:
#
#############################################################################


#
# Get all the path values set
#
VM_LINUX_ROOT ?= $(word 1, $(subst /vm_linux/,/vm_linux /, $(shell pwd)))

OSS_ROOT ?= $(VM_LINUX_ROOT)/oss

include $(OSS_ROOT)/../project_x/host.mak
include $(OSS_ROOT)/source/mak/target.mak

OSS_SRC_ROOT = $(OSS_ROOT)/source
LIB_SUB_ROOT  = library/gnuarm-$(TOOL_CHAIN)$(VFP_SUFFIX)
OSS_LIB_ROOT = $(OSS_ROOT)/$(LIB_SUB_ROOT)


OSS_LIB_ROOT = $(OSS_ROOT)/$(LIB_SUB_ROOT)

export OSS_ROOT OSS_LIB_ROOT OSS_ROOT OSS_LIB_ROOT
   

#
# Build the bits
#
.PHONY : 

all :  
	echo 'None'

init_dir:
	@mkdir -p $(OSS_ROOT)/library/lib
	@mkdir -p $(OSS_ROOT)/library/3rd/bin	
	@mkdir -p $(OSS_ROOT)/library/basic/lib

install: init_dir


	@echo install expat library
	@if [ -d  "$(OSS_LIB_ROOT)/expat/$(EXPAT_VERSION)/lib" ]; then \
		make -C $(OSS_LIB_ROOT)/expat/$(EXPAT_VERSION)/lib/; \
	fi
	@cp -f $(OSS_LIB_ROOT)/expat/$(EXPAT_VERSION)/lib/libexpat.so.1.5.2 $(OSS_ROOT)/library/basic/lib	
	@ln -sf libexpat.so.1.5.2 $(OSS_ROOT)/library/basic/lib/libexpat.so
	@ln -sf libexpat.so.1.5.2 $(OSS_ROOT)/library/basic/lib/libexpat.so.1
ifeq "$(TOOL_CHAIN)" "4.5.1"	
ifeq "$(ENABLE_CA9)" "true"	
	@echo install libasound library
	@if [ -d  "$(OSS_LIB_ROOT)/alsa/$(ALSA_VERSION)/alsa-lib-$(ALSA_VERSION)/lib" ]; then \
		make -C $(OSS_LIB_ROOT)/alsa/$(ALSA_VERSION)/alsa-lib-$(ALSA_VERSION)/lib; \
	fi
	@mkdir -p $(OSS_ROOT)/library/basic/lib/alsa-lib-$(ALSA_VERSION)
	@mkdir -p $(OSS_ROOT)/library/basic/lib/alsa-lib-$(ALSA_VERSION)/lib
	@cp -f $(OSS_LIB_ROOT)/alsa/$(ALSA_VERSION)/alsa-lib-$(ALSA_VERSION)/lib/libasound.so.2.0.0 $(OSS_ROOT)/library/basic/lib/alsa-lib-$(ALSA_VERSION)/lib
	@ln -sf libasound.so.2.0.0 $(OSS_ROOT)/library/basic/lib/alsa-lib-$(ALSA_VERSION)/lib/libasound.so
	@ln -sf libasound.so.2.0.0 $(OSS_ROOT)/library/basic/lib/alsa-lib-$(ALSA_VERSION)/lib/libasound.so.2
	@cp -rf $(OSS_LIB_ROOT)/alsa/$(ALSA_VERSION)/alsa-lib-$(ALSA_VERSION)/share $(OSS_ROOT)/library/basic/lib/alsa-lib-$(ALSA_VERSION)
	@mkdir -p $(OSS_ROOT)/library/basic/lib/alsa-lib-$(ALSA_VERSION)/lib/alsa-lib
	@mkdir -p $(OSS_ROOT)/library/basic/lib/alsa-lib-$(ALSA_VERSION)/lib/alsa-lib/smixer
	@cp -f $(OSS_LIB_ROOT)/alsa/$(ALSA_VERSION)/alsa-lib-$(ALSA_VERSION)/lib/alsa-lib/smixer/*.so $(OSS_ROOT)/library/basic/lib/alsa-lib-$(ALSA_VERSION)/lib/alsa-lib/smixer

endif
endif

	@echo install libiconv library
	@if [ -d  "$(OSS_LIB_ROOT)/libiconv/libiconv-1.13.1/lib" ]; then \
		make -C $(OSS_LIB_ROOT)/libiconv/libiconv-1.13.1/lib/; \
	fi

	@echo install libxml2 library
	@if [ -d  "$(OSS_LIB_ROOT)/libxml2/libxml2-2.6.27/lib" ]; then \
		make -C $(OSS_LIB_ROOT)/libxml2/libxml2-2.6.27/lib/; \
	fi

	@if [ -d  "$(OSS_LIB_ROOT)/libxml2/libxml2-2.7.3/lib" ]; then \
		make -C $(OSS_LIB_ROOT)/libxml2/libxml2-2.7.3/lib/; \
	fi

	@if [ -d  "$(OSS_LIB_ROOT)/libxml2/libxml2-2.7.7/lib" ]; then \
		make -C $(OSS_LIB_ROOT)/libxml2/libxml2-2.7.7/lib/; \
	fi

	@if [ -d  "$(OSS_LIB_ROOT)/libxml2/libxml2-2.7.8/lib" ]; then \
		make -C $(OSS_LIB_ROOT)/libxml2/libxml2-2.7.8/lib/; \
	fi

	@echo install qt-4.6.2 library
	@if [ -d  "$(OSS_LIB_ROOT)/qt/qt-4.6.2/lib" ]; then \
		make -C $(OSS_LIB_ROOT)/qt/qt-4.6.2/lib/; \
	fi

	@echo install qt-4.6.2_vfp library
	@if [ -d  "$(OSS_LIB_ROOT)_vfp/qt/qt-4.6.2/lib" ]; then \
		make -C $(OSS_LIB_ROOT)_vfp/qt/qt-4.6.2/lib/; \
	fi

	@echo install qt-4.7.0 library
	@if [ -d  "$(OSS_LIB_ROOT)/qt/qt-4.7.0/lib" ]; then \
		make -C $(OSS_LIB_ROOT)/qt/qt-4.7.0/lib/; \
	fi

	@echo install qt-4.7.0_vfp library
	@if [ -d  "$(OSS_LIB_ROOT)_vfp/qt/qt-4.7.0/lib" ]; then \
		make -C $(OSS_LIB_ROOT)_vfp/qt/qt-4.7.0/lib/; \
	fi	

	@echo install samba library
	@if [ -d  "$(OSS_LIB_ROOT)/samba/samba-3.5.6/lib" ]; then \
		make -C $(OSS_LIB_ROOT)/samba/samba-3.5.6/lib/; \
	fi

	@echo install gtkdfb library
	@if [ -d "$(OSS_LIB_ROOT)/gtkdfb" ]; then \
		make -C $(OSS_LIB_ROOT)/gtkdfb/; \
	fi

	@echo install webvideo library
	@if [ -d "$(OSS_LIB_ROOT)/webvideo" ]; then \
		make -C $(OSS_LIB_ROOT)/webvideo/; \
	fi

    @echo install iptables
	@if [ -d "$(OSS_LIB_ROOT)/iptables" ]; then \
		make -C $(OSS_LIB_ROOT)/iptables/; \
	fi
	
ifeq "$(CHECK_DISK)" "true"
	@echo install dosfsck bin
	@mkdir -p $(OSS_ROOT)/library/3rd/bin
	@cp -f $(OSS_LIB_ROOT)/dosfsck/3.0.9/preinstall/bin/fsck.fat $(OSS_ROOT)/library/3rd/bin
	@chmod +x $(OSS_ROOT)/library/3rd/bin/fsck.fat
	
	@echo install ntfsfix bin
	@cp -f $(OSS_LIB_ROOT)/ntfsprogs/2.0.0/pre-install/bin/ntfsfix $(OSS_ROOT)/library/3rd/bin
	@chmod +x $(OSS_ROOT)/library/3rd/bin/ntfsfix
	@cp -f $(OSS_LIB_ROOT)/ntfsprogs/2.0.0/pre-install/lib/libntfs.so.10.0.0 $(OSS_ROOT)/library/lib
	@ln -sf libntfs.so.10.0.0 $(OSS_ROOT)/library/lib/libntfs.so
	@ln -sf libntfs.so.10.0.0 $(OSS_ROOT)/library/lib/libntfs.so.10
ifeq "$(EXFAT_SUPPORT)" "true"
	@cp -f $(OSS_LIB_ROOT)/exfat/0.9.4/pre-install/bin/exfatfsck $(OSS_ROOT)/library/3rd/bin
	@chmod +x $(OSS_ROOT)/library/3rd/bin/exfatfsck
endif
endif # CHECK_DISK 
		
ifeq "$(NTFS_WRITE_SUPPORT)" "true"
	@echo install fuse library
	@mkdir -p $(OSS_ROOT)/library/3rd/bin
		
endif # NTFS_WRITE_SUPPORT 

ifeq "$(MTP_SUPPORT)" "true"
	@if [ -f  "$(OSS_LIB_ROOT)/file_system/mtpfs/Makefile" ]; then \
		$(MAKE) -C $(OSS_LIB_ROOT)/file_system/mtpfs/; \
	fi	
endif # MTP_SUPPORT 

ifeq "$(EXFAT_SUPPORT)" "true"
	@echo install exfat
	@mkdir -p $(OSS_ROOT)/library/3rd/bin
	@cp -f $(OSS_LIB_ROOT)/exfat/0.9.4/pre-install/bin/mount.exfat-fuse $(OSS_ROOT)/library/3rd/bin
	@chmod +x $(OSS_ROOT)/library/3rd/bin/mount.exfat-fuse
	@cp -f $(OSS_LIB_ROOT)/exfat/0.9.4/pre-install/bin/mkexfatfs $(OSS_ROOT)/library/3rd/bin
	@chmod +x $(OSS_ROOT)/library/3rd/bin/mkexfatfs
endif #EXFAT_SUPPORT


ifeq "$(RELEASE_3RD_LIB_BLUETOOTH)" "true"
ifeq "$(BLUETOOTH)" "true"
	@echo Create symbolic link for ALSA library
	@ln -sf $(OSS_LIB_ROOT)/alsa/lib/libasound.so.2 $(OSS_LIB_ROOT)/alsa/lib/libasound.so
	@echo Create symbolic link for DBus library
	@ln -sf $(OSS_LIB_ROOT)/dbus/lib/libdbus-1.so.3 $(OSS_LIB_ROOT)/dbus/lib/libdbus-1.so
	@echo Create symbolic link for GLib library
	@ln -sf $(OSS_LIB_ROOT)/glib/lib/libglib-2.0.so.0 $(OSS_LIB_ROOT)/glib/lib/libglib-2.0.so
	@echo install ALSA library
	@cp -rf $(OSS_LIB_ROOT)/alsa $(OSS_ROOT)/library/basic
	@echo install DBus library
	@cp -rf $(OSS_LIB_ROOT)/dbus $(OSS_ROOT)/library/basic
endif
endif
	@echo install libusb bin
	@cp -f $(OSS_LIB_ROOT)/libusb/$(LIBUSB_VERSION)/libusb-1.0.so.0.0.0 $(OSS_ROOT)/library/lib
	@ln -sf libusb-1.0.so.0.0.0 $(OSS_ROOT)/library/lib/libusb-1.0.so
	@ln -sf libusb-1.0.so.0.0.0 $(OSS_ROOT)/library/lib/libusb-1.0.so.0

### Added for BIVL ###
ifeq "$(BIVL_SUPPORT)" "true"
##### Sqlite3 Library #####
	@echo install SQLITE3 library from pre-build library
	@cp -f $(OSS_LIB_ROOT)/sqlite3/lib/libsqlite_3_7_2.so $(OSS_ROOT)/library/basic/lib
##### BIVL Library #####
	@echo install BIVL Library from pre-build library
	@cp -f $(OSS_LIB_ROOT)/sony/bivl/lib/libbiv.so $(OSS_ROOT)/library/basic/lib
	@cp -f $(OSS_LIB_ROOT)/sony/bivl/src/certs/cacert.pem $(OUTPUT_ROOT)/basic
	@cp -f $(OSS_LIB_ROOT)/sony/bivl/lib/test $(OUTPUT_ROOT)/basic/bivl_test
endif

ifeq "$(ICU_SUPPORT)" "true"
	@echo "install icu lib"
	@if [ -f  "$(OSS_LIB_ROOT)/file_system/mtpfs/Makefile" ]; then \
		$(MAKE) -C $(OSS_LIB_ROOT)/icu; \
	fi
endif

clean clean_install:
	@rm -rf $(OSS_ROOT)/library/lib
	@rm -rf $(OSS_ROOT)/library/3rd
	@rm -rf $(OSS_ROOT)/library/basic



	@if [ -d  "$(OSS_LIB_ROOT)/expat/2.0.1/lib" ]; then \
		make -C $(OSS_LIB_ROOT)/expat/2.0.1/lib/ clean; \
	fi	
	@rm -rf $(OSS_ROOT)/library/basic/lib/libexpat.so
	@rm -rf $(OSS_ROOT)/library/basic/lib/libexpat.so.1

	@if [ -d  "$(OSS_LIB_ROOT)/libiconv/libiconv-1.13.1/lib" ]; then \
		make -C $(OSS_LIB_ROOT)/libiconv/libiconv-1.13.1/lib/ clean; \
	fi

	@if [ -d  "$(OSS_LIB_ROOT)/libxml2/libxml2-2.6.27/lib" ]; then \
		make -C $(OSS_LIB_ROOT)/libxml2/libxml2-2.6.27/lib/ clean; \
	fi

	@if [ -d  "$(OSS_LIB_ROOT)/libxml2/libxml2-2.7.3/lib" ]; then \
		make -C $(OSS_LIB_ROOT)/libxml2/libxml2-2.7.3/lib/ clean; \
	fi

	@if [ -d  "$(OSS_LIB_ROOT)/libxml2/libxml2-2.7.7/lib" ]; then \
		make -C $(OSS_LIB_ROOT)/libxml2/libxml2-2.7.7/lib/ clean; \
	fi

	@if [ -d  "$(OSS_LIB_ROOT)/libxml2/libxml2-2.7.8/lib" ]; then \
		make -C $(OSS_LIB_ROOT)/libxml2/libxml2-2.7.8/lib/ clean; \
	fi

ifeq "$(MTP_SUPPORT)" "true"
	@if [ -f  "$(OSS_LIB_ROOT)/file_system/mtpfs/Makefile" ]; then \
		make -C $(OSS_LIB_ROOT)/file_system/mtpfs/ clean; \
	fi	
endif # MTP_SUPPORT 

	@if [ -d  "$(OSS_LIB_ROOT)/qt/qt-4.6.2/lib" ]; then \
		make -C $(OSS_LIB_ROOT)/qt/qt-4.6.2/lib/ clean; \
	fi

	@if [ -d  "$(OSS_LIB_ROOT)_vfp/qt/qt-4.6.2/lib" ]; then \
		make -C $(OSS_LIB_ROOT)_vfp/qt/qt-4.6.2/lib/ clean; \
	fi

	@if [ -d  "$(OSS_LIB_ROOT)/qt/qt-4.7.0/lib" ]; then \
		make -C $(OSS_LIB_ROOT)/qt/qt-4.7.0/lib/ clean; \
	fi

	@if [ -d  "$(OSS_LIB_ROOT)_vfp/qt/qt-4.7.0/lib" ]; then \
		make -C $(OSS_LIB_ROOT)_vfp/qt/qt-4.7.0/lib/ clean; \
	fi

	@if [ -d  "$(OSS_LIB_ROOT)/samba/samba-3.5.6/lib" ]; then \
		make -C $(OSS_LIB_ROOT)/samba/samba-3.5.6/lib/ clean; \
	fi

	@if [ -d "$(OSS_LIB_ROOT)/gtkdfb" ]; then \
		make -C $(OSS_LIB_ROOT)/gtkdfb/ clean; \
	fi


