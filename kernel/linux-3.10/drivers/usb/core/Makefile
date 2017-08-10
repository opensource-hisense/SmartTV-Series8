#
# Makefile for USB Core files and filesystem
#

ccflags-$(CONFIG_USB_DEBUG) := -DDEBUG


ifeq ($(filter true,$(USB_ADB) $(USB_CDC) $(CDC_DUMP_SUPPORT) $(LINUX_EMU_USB_CDC)), true)
ifndef USB_ADB_PORT
export USB_ADB_PORT := 0
endif
endif

ifeq ($(USB_ADB_PORT), 0)
EXTRA_CFLAGS		+= -DMUSB_GADGET_PORT_NUM=0
endif
ifeq ($(USB_ADB_PORT), 1)
EXTRA_CFLAGS		+= -DMUSB_GADGET_PORT_NUM=1
endif
ifeq ($(USB_ADB_PORT), 2)
EXTRA_CFLAGS		+= -DMUSB_GADGET_PORT_NUM=2
endif
ifeq ($(USB_ADB_PORT), 3)
EXTRA_CFLAGS		+= -DMUSB_GADGET_PORT_NUM=3
endif


usbcore-y := usb.o hub.o hcd.o urb.o message.o driver.o
usbcore-y += config.o file.o buffer.o sysfs.o endpoint.o
usbcore-y += devio.o notify.o generic.o quirks.o devices.o
usbcore-y += port.o

usbcore-$(CONFIG_PCI)		+= hcd-pci.o
usbcore-$(CONFIG_ACPI)		+= usb-acpi.o

obj-$(CONFIG_USB)		+= usbcore.o





