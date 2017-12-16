#
# (C) Copyright 2000-2007
# Wolfgang Denk, DENX Software Engineering, wd@denx.de.
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
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston,
# MA 02111-1307 USA
#

include $(TOPDIR)/config.mk

CFLAGS += -I.. -I$(COMMON_INC) -I$(OSAI_INC)

ifeq ($(USB_ADB_PORT), 0)
CFLAGS		+= -DMUSB_GADGET_PORT_NUM=0
endif
ifeq ($(USB_ADB_PORT), 1)
CFLAGS		+= -DMUSB_GADGET_PORT_NUM=1
endif
ifeq ($(USB_ADB_PORT), 2)
CFLAGS		+= -DMUSB_GADGET_PORT_NUM=2
endif
ifeq ($(USB_ADB_PORT), 3)
CFLAGS		+= -DMUSB_GADGET_PORT_NUM=3
endif
LIB	:= $(obj)libusb.o

COBJS-$(CONFIG_USB_R8A66597_HCD) += r8a66597-hcd.o

COBJS-y += isp116x-hcd.o
COBJS-y += sl811_usb.o
COBJS-y += usb_ohci.o
COBJS-y += usbdcore.o
COBJS-y += usbdcore_ep0.o
COBJS-y += usbdcore_mpc8xx.o
COBJS-y += usbdcore_omap1510.o
COBJS-y += Ax88772.o
COBJS-y += mtk_usb.o
COBJS-y += pegasus.o
COBJS-y += rtl8150.o

ifneq ($(USB_ADB_PORT), 2)
COBJS-y += mt_usb.o
endif

COBJS	:= $(COBJS-y)
SRCS	:= $(COBJS:.o=.c)
OBJS	:= $(addprefix $(obj),$(COBJS))

all:	$(LIB)

$(LIB):	$(obj).depend $(OBJS)
	$(call cmd_link_o_target, $(OBJS))

#########################################################################

# defines $(obj).depend target
include $(SRCTREE)/rules.mk

sinclude $(obj).depend

#########################################################################
