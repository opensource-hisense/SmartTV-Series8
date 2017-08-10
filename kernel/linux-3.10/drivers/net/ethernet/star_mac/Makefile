#
# Makefile for linux/drivers/net/star_mac
#


#EXTRA_CFLAGS = -D__KERNEL__ -DMODULE -O -Wall -I$(INCLUDEDIR)
ifeq "$(TOOL_CHAIN)" "4.8.2"
EXTRA_CFLAGS += -Wno-maybe-uninitialized   -Werror
else
EXTRA_CFLAGS += -Werror
endif
EXTRA_CFLAGS += -D STAR_DBG_LVL_DEFAULT=2 

#EXTRA_CFLAGS += -D TX_DESC_NUM=8 \
#                -D RX_DESC_NUM=16 \
#                -D STAR_DBG_LVL_DEFAULT=2 \
#                -D USE_EXT_CLK_SRC=0 \
#                -D USE_TX_TASKLET \
#                -D USE_RX_NAPI \

                
# STAR_DBG_LVL_DEFAULT
# ERR = 0, WARN = 1, DBG = 2, VERB = 3, ALL = STAR_DBG_MAX

#EXTRA_CFLAGS += -D INTERNAL_PHY

ifeq ($(CONFIG_PHY_SMSC8700),y)
EXTRA_CFLAGS += -DMT85XX_LAN8700
endif

ifeq ($(CONFIG_MAC85XX_DEBUG_LOG),y)
EXTRA_CFLAGS += -DDEBUG_ETH_ON
endif

ifeq ($(CONFIG_MAC5365_RMII),y)
EXTRA_CFLAGS += -DUSE_MAC_RMII
obj-$(CONFIG_MAC5365_RMII) += star_mac.o
endif

ifeq ($(CONFIG_MAC5366_MII),y)
EXTRA_CFLAGS += -DUSE_MAC_MII
obj-$(CONFIG_MAC5366_MII) += star_mac.o
endif

ifeq ($(CONFIG_MAC5395_MII),y)
EXTRA_CFLAGS += -DINTERNAL_PHY
obj-$(CONFIG_MAC5395_MII) += star_mac.o
endif

ifeq ($(CONFIG_MAC5396_MII),y)
EXTRA_CFLAGS += -DINTERNAL_PHY
endif

obj-$(CONFIG_MAC5396_MII) += star_mac.o

ifeq ($(CONFIG_MAC5368_MII),y)
EXTRA_CFLAGS += -DINTERNAL_PHY
endif

obj-$(CONFIG_MAC5368_MII) += star_mac.o

ifeq ($(CONFIG_MAC5389_MII),y)
EXTRA_CFLAGS += -DINTERNAL_PHY
endif

obj-$(CONFIG_MAC5389_MII) += star_mac.o

ifeq ($(CONFIG_MAC5398_MII),y)
EXTRA_CFLAGS += -DINTERNAL_PHY
endif

obj-$(CONFIG_MAC5398_MII) += star_mac.o

ifeq ($(CONFIG_MAC5880_MII),y)
EXTRA_CFLAGS += -DINTERNAL_PHY
endif

ifdef CONFIG_MAC5882_MII
EXTRA_CFLAGS += -DINTERNAL_PHY
endif

ifdef CONFIG_MAC5883_MII
EXTRA_CFLAGS += -DINTERNAL_PHY
endif

#ifeq ($(CONFIG_MAC5399_MII),y)
ifdef CONFIG_MAC5399_MII
EXTRA_CFLAGS += -DINTERNAL_PHY
endif

obj-$(CONFIG_MAC5399_MII) += star_mac.o

obj-$(CONFIG_MAC5880_MII) += star_mac.o

obj-$(CONFIG_MAC5882_MII) += star_mac.o

obj-$(CONFIG_MAC5883_MII) += star_mac.o

ifdef CONFIG_MAC5890_MII
EXTRA_CFLAGS += -DINTERNAL_PHY
endif

obj-$(CONFIG_MAC5890_MII) += star_mac.o

ifdef CONFIG_MAC5891_MII
EXTRA_CFLAGS += -DINTERNAL_PHY
endif

obj-$(CONFIG_MAC5891_MII) += star_mac.o
star_mac-objs := star.o star_dev.o
