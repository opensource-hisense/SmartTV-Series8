###############################################################################
# linux_core/uboot/symlink.mak
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


ROOTDIR	:= $(shell if [ "$$PWD" != "" ]; then echo $$PWD; else pwd; fi)

VM_LINUX_ROOT ?= $(word 1, $(subst /vm_linux/,/vm_linux /, $(ROOTDIR)))
include include $(DTV_LINUX_MAK_ROOT)/nucleus_host.mak


TARGET_IC      ?= mt5365
TARGET_DRIVER  ?= $(subst mt,,$(TARGET_IC))_driver


TARGET_ROOT    ?= $(VM_LINUX_ROOT)/linux_core/driver/target/$(TARGET_IC)
MTKLOADER_ROOT ?= $(VM_LINUX_ROOT)/linux_core/uboot/_build_mtkloader/project_x
UTARGET_ROOT   ?= $(MTKLOADER_ROOT)/$(TARGET_IC)
UTOOLS_ROOT    ?= $(MTKLOADER_ROOT)/tools
TOOLS_ROOT     ?= $(VM_LINUX_ROOT)/project_x/tools

TOOLS_LIST = $(shell cd $(TOOLS_ROOT)/source/public;find pbuild -type f)

DRV_FILE_LIST = $(UTARGET_ROOT)/drv_file_list.txt

DRV_LIST = $(shell if [ -e $(DRV_FILE_LIST).need_to_do_link ]; then cat $(DRV_FILE_LIST); fi)

all: $(DRV_LIST) $(TOOLS_LIST)
#	@$(call if_file_notexist_w_sym_fct, $(VM_LINUX_ROOT)/project_x/target/d_inc  ,$(MTKLOADER_ROOT)/target/d_inc);
	@$(call if_file_notexist_w_sym_fct, $(TARGET_ROOT)/symlink.mak               ,$(UTARGET_ROOT)/symlink.mak);
       
#	@if [ ! -d "$(MTKLOADER_ROOT)/middleware/public/scripts" ]; then \
		mkdir -p $(MTKLOADER_ROOT)/middleware/public/scripts; \
	fi
#	@if [ ! -d "$(MTKLOADER_ROOT)/middleware/public/dlm" ]; then \
		mkdir -p $(MTKLOADER_ROOT)/middleware/public/dlm; \
	fi
#	@if [ ! -d "$(MTKLOADER_ROOT)/target/mt53xx_com" ]; then \
		mkdir -p $(MTKLOADER_ROOT)/target/mt53xx_com; \
	fi
#	@if [ ! -d "$(MTKLOADER_ROOT)/custom/dev" ]; then \
		mkdir -p $(MTKLOADER_ROOT)/custom/dev; \
	fi
	
#	@if [ ! -d "$(MTKLOADER_ROOT)/tools/archieve_lib" ]; then \
		mkdir -p $(MTKLOADER_ROOT)/tools/archieve_lib; \
	fi

#	@if [ ! -d "$(MTKLOADER_ROOT)/tools/mt5391_pack" ]; then \
		mkdir -p $(MTKLOADER_ROOT)/tools/mt5391_pack; \
	fi

#	@for i in `cd $(VM_LINUX_ROOT)/project_x/; ls *.mak`; do \
		$(call if_file_notexist_w_sym_fct, $(VM_LINUX_ROOT)/project_x/$$i                    ,$(MTKLOADER_ROOT)/$$i); \
	done 

#	@for i in `cd $(VM_LINUX_ROOT)/project_x/middleware/public/; ls *.mak`; do \
		$(call if_file_notexist_w_sym_fct, $(VM_LINUX_ROOT)/project_x/middleware/public/$$i         ,$(MTKLOADER_ROOT)/middleware/public/$$i); \
	done
#	@for i in `cd $(VM_LINUX_ROOT)/project_x/middleware/public/scripts/; ls`; do \
		$(call if_file_notexist_w_sym_fct, $(VM_LINUX_ROOT)/project_x/middleware/public/scripts/$$i ,$(MTKLOADER_ROOT)/middleware/public/scripts/$$i); \
	done

#	@for i in `cd $(VM_LINUX_ROOT)/project_x/middleware/public/dlm/; ls`; do \
		$(call if_file_notexist_w_sym_fct, $(VM_LINUX_ROOT)/project_x/middleware/public/dlm/$$i      ,$(MTKLOADER_ROOT)/middleware/public/dlm/$$i); \
	done

#	@for i in `cd $(VM_LINUX_ROOT)/project_x/custom/dev/; ls *.mak`; do \
		$(call if_file_notexist_w_sym_fct, $(VM_LINUX_ROOT)/project_x/custom/dev/$$i          ,$(MTKLOADER_ROOT)/custom/dev/$$i); \
	done
	
#	@for i in `cd $(VM_LINUX_ROOT)/project_x/target/mt53xx_com/; ls *.mak`; do \
		$(call if_file_notexist_w_sym_fct, $(VM_LINUX_ROOT)/project_x/target/mt53xx_com/$$i   ,$(MTKLOADER_ROOT)/target/mt53xx_com/$$i); \
	done

#	@for i in `cd $(VM_LINUX_ROOT)/project_x/tools/archieve_lib/; ls`; do \
		$(call if_file_notexist_w_sym_fct, $(VM_LINUX_ROOT)/project_x/tools/archieve_lib/$$i  ,$(MTKLOADER_ROOT)/tools/archieve_lib/$$i); \
	done

#	@for i in `cd $(VM_LINUX_ROOT)/project_x/tools/binary/mt5391_pack/; ls`; do \
		$(call if_file_notexist_w_sym_fct, $(VM_LINUX_ROOT)/project_x/tools/binary/mt5391_pack/$$i  ,$(MTKLOADER_ROOT)/tools/binary/mt5391_pack/$$i); \
	done
	

#	@$(call if_file_notexist_w_sym_fct, $(VM_LINUX_ROOT)/project_x/mtk_obj ,$(MTKLOADER_ROOT)/mtk_obj)

	@if [ -e $(DRV_FILE_LIST).tmp ]; then \
		if [ "`diff -q $(DRV_FILE_LIST).tmp $(DRV_FILE_LIST) 2>/dev/null`" != '' ]; then \
			cp -au $(DRV_FILE_LIST).tmp $(DRV_FILE_LIST); \
		fi; \
		rm -rf $(DRV_FILE_LIST).tmp; \
	fi

	

$(DRV_LIST):
	@if [ ! -d "$(UTARGET_ROOT)/$(dir $@)" ]; then \
		mkdir -p $(UTARGET_ROOT)/$(dir $@); \
	fi
	@if [ ! -e $(UTARGET_ROOT)/$@ -a ! -h $(UTARGET_ROOT)/$@ ]; then \
		if [ "$(suffix $@)" == ".o" ]; then \
			if [ ! -e "$(TARGET_ROOT)/$(@:.o=.c)" ]; then \
				$(call if_file_notexist_w_sym_fct, $(TARGET_ROOT)/$@  ,$(UTARGET_ROOT)/$@); \
			fi; \
		else \
			$(call if_file_notexist_w_sym_fct, $(TARGET_ROOT)/$@  ,$(UTARGET_ROOT)/$@); \
		fi; \
	fi


$(TOOLS_LIST):
	@if [ ! -d "$(UTOOLS_ROOT)/$(dir $@)" ]; then \
		mkdir -p $(UTOOLS_ROOT)/$(dir $@); \
	fi
	@if [ ! -e $(UTOOLS_ROOT)/$@ -a ! -h $(UTOOLS_ROOT)/$@ ]; then \
		$(call if_file_notexist_w_sym_fct, $(TOOLS_ROOT)/source/public/$@  ,$(UTOOLS_ROOT)/$@); \
	fi

