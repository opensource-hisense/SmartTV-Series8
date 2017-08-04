#-----------------------------------------------------------------------------
# Copyright (c) 2009, MediaTek Inc.
# All rights reserved.
#
# Unauthorized use, practice, perform, copy, distribution, reproduction,
# or disclosure of this information in whole or in part is prohibited.
#-----------------------------------------------------------------------------
# $RCSfile:  $
# $Revision:
# $Date:
# $Author:  $
# $CCRevision:  $
# $SWAuthor:  $
# $MD5HEX:  $
#
# Description:
#        A set of files describing the framebuffer interface and core types.
#---------------------------------------------------------------------------*/


all:
ifeq "$(WIFI_SUPPLICANT_PATH)" "basic"
	@mkdir -p $(OSS_OUTPUT)/basic/bin/wpa_supplicant/common/
ifeq ($(filter $(WIFI_DONGLE),MT7601U RT557X),$(WIFI_DONGLE))
	cp -a ./wpa_wext.conf $(OSS_OUTPUT)/basic/bin/wpa_supplicant/wpa.conf
	cp -a ./p2p_wext.conf $(OSS_OUTPUT)/basic/bin/wpa_supplicant/p2p.conf
endif
ifeq ($(filter $(WIFI_DONGLE),MT7603U MT7662U),$(WIFI_DONGLE))
	cp -a ./wpa_cfg80211.conf $(OSS_OUTPUT)/basic/bin/wpa_supplicant/wpa.conf
	cp -a ./p2p_cfg80211.conf $(OSS_OUTPUT)/basic/bin/wpa_supplicant/p2p.conf
endif
ifeq ($(filter $(WIFI_DONGLE),BCM43569),$(WIFI_DONGLE))
	cp -a ./wpa_bcm43569.conf $(OSS_OUTPUT)/basic/bin/wpa_supplicant/wpa.conf
	cp -a ./p2p_bcm43569.conf $(OSS_OUTPUT)/basic/bin/wpa_supplicant/p2p.conf
endif
	cp -a common/*		$(OSS_OUTPUT)/basic/bin/wpa_supplicant/common/
	cp -a ./*		$(OSS_OUTPUT)/basic/bin/wpa_supplicant/
else
	@mkdir -p $(OSS_OUTPUT)/3rd/bin/wpa_supplicant/common/
ifeq ($(filter $(WIFI_DONGLE),MT7601U RT557X),$(WIFI_DONGLE))
	cp -a ./wpa_wext.conf $(OSS_OUTPUT)/3rd/bin/wpa_supplicant/wpa.conf
	cp -a ./p2p_wext.conf $(OSS_OUTPUT)/3rd/bin/wpa_supplicant/p2p.conf
endif
	@echo "==$(WIFI_DONGLE)"
ifeq ($(filter $(WIFI_DONGLE),MT7603U MT7662U),$(WIFI_DONGLE))
	cp -a ./wpa_cfg80211.conf $(OSS_OUTPUT)/3rd/bin/wpa_supplicant/wpa.conf
	cp -a ./p2p_cfg80211.conf $(OSS_OUTPUT)/3rd/bin/wpa_supplicant/p2p.conf
endif
ifeq ($(filter $(WIFI_DONGLE),BCM43569),$(WIFI_DONGLE))
	cp -a ./wpa_bcm43569.conf $(OSS_OUTPUT)/3rd/bin/wpa_supplicant/wpa.conf
	cp -a ./p2p_bcm43569.conf $(OSS_OUTPUT)/3rd/bin/wpa_supplicant/p2p.conf
endif
	cp -a common/*		$(OSS_OUTPUT)/3rd/bin/wpa_supplicant/common/
	cp -a common/libnl*		$(OSS_OUTPUT)/3rd/lib/
	cp -a ./*		$(OSS_OUTPUT)/3rd/bin/wpa_supplicant/
endif

clean:
ifeq "$(WIFI_SUPPLICANT_PATH)" "basic"
	rm -rf $(OSS_OUTPUT)/basic/bin/wpa_supplicant/common/*
	rm -rf $(OSS_OUTPUT)/basic/bin/wpa_supplicant/*
else
	rm -rf $(OSS_OUTPUT)/3rd/bin/wpa_supplicant/common/*
	rm -rf $(OSS_OUTPUT)/3rd/bin/wpa_supplicant/*
endif
