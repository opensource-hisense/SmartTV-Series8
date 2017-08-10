#
# (C) Copyright 2000-2004
# Wolfgang Denk, DENX Software Engineering, wd@denx.de.
#
# (C) Copyright 2004
# ARM Ltd.
# Philippe Robin, <philippe.robin@arm.com>
#
# See file CREDITS for list of people who contributed to this
# project.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of
# the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston,
# MA 02111-1307 USA
#

include $(TOPDIR)/config.mk

CPPFLAGS += $(COMMON_INC) $(OSAI_INC) -DCC_MTK_LOADER -DCC_MT5890 -DCC_MT5891 -Imodule -I$(ROOTDIR)/drv_lib/drv_cust/ $(CC_SECURE_ROM_DEF) -DCC_CPU_DESCRAMBLER
CFLAGS += $(COMMON_INC) $(OSAI_INC) -DCC_MTK_LOADER -DCC_MT5890 -DCC_MT5891 -Imodule -I$(ROOTDIR)/drv_lib/drv_cust/ $(CC_SECURE_ROM_DEF) -DCC_CPU_DESCRAMBLER

ifeq ($(UBOOT_VERSION), 1.3.4)
LIB	= $(obj)lib$(BOARD).a
else
LIB	= $(obj)lib$(BOARD).o
CFLAGS += -DCC_UBOOT_2011_12
CPPFLAGS += -DCC_UBOOT_2011_12
endif

CFLAGS := $(subst -w,-Wall,$(CFLAGS))

OBJS	:= mt5891_board.o mt5891_flash.o mt5891_nand.o mt5891_mmc.o mt5891_sif.o mt5891_mac.o mt5891_secure_rom.o nand_aes.o env.o

OBJS	+= module/hal_io.o module/serialflash_hw.o module/gpio.o module/pinmux.o \
           module/sif_if.o module/sif_isr.o module/sif_hw_m0.o module/sif_hw_m1.o module/sif_hw_m2.o module/sif_hw_pdm0.o module/sif_hw_pdm1.o module/sif_hw_secm0.o \
           module/pdwnc_entry.o module/pdwnc_gpio.o module/pdwnc_pwm.o module/pdwnc_hw.o  module/pdwnc_srvad.o module/pdwnc_t8032.o \
           module/eeprom_hw.o module/eeprom_if.o module/ir_ph.o module/ir_hw.o module/irrx_if.o \
           module/gcpu_hw.o module/gcpu_api.o module/gcpu_if.o module/dmx_drm_if.o module/dmx_dtcp_api.o

OBJS	+= drv_lib/drv_cust/drvcust_if.o drv_lib/drv_cust/drvcust_init.o \
           drv_lib/drv_cust/panel_table.o drv_lib/drv_cust/drvcust_lib.o \

OBJS    += common/mt53xx_com_board.o

SOBJS	:= memsetup.o

SRCS    := $(SOBJS:.o=.S) $(OBJS:.o=.c)
OBJS    := $(addprefix $(obj),$(OBJS) $(SOBJS))
$(shell mkdir -p $(obj)module $(obj)drv_lib/drv_cust $(obj)common)

ifeq ($(UBOOT_VERSION), 1.3.4)
$(LIB):	$(obj).depend $(OBJS)
	$(AR) $(ARFLAGS) $@ $(OBJS)
else
$(LIB): $(obj).depend $(OBJS)
	$(call cmd_link_o_target, $(OBJS))
endif

clean:
	rm -f $(OBJS)

distclean:	clean
	rm -f $(LIB) core *.bak .depend

#########################################################################

# defines $(obj).depend target
include $(SRCTREE)/rules.mk


-include $(obj).depend

#########################################################################
