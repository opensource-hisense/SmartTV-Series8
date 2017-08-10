#
# Makefile for the kernel USB device drivers.
#

# Object files in subdirectories
# Enable warning to error.
ifeq "$(TOOL_CHAIN)" "4.8.2"
subdir-ccflags-y += -Wno-maybe-uninitialized -Werror
else
subdir-ccflags-y += -Werror
endif

obj-$(CONFIG_USB)		+= core/

obj-$(CONFIG_USB_DWC3)		+= dwc3/

obj-$(CONFIG_USB_MON)		+= mon/

obj-$(CONFIG_PCI)		+= host/
obj-$(CONFIG_USB_EHCI_HCD)	+= host/
obj-$(CONFIG_USB_ISP116X_HCD)	+= host/
obj-$(CONFIG_USB_OHCI_HCD)	+= host/
obj-$(CONFIG_USB_UHCI_HCD)	+= host/
obj-$(CONFIG_USB_FHCI_HCD)	+= host/
obj-$(CONFIG_USB_XHCI_HCD)	+= host/
obj-$(CONFIG_USB_SL811_HCD)	+= host/
obj-$(CONFIG_USB_ISP1362_HCD)	+= host/
obj-$(CONFIG_USB_U132_HCD)	+= host/
obj-$(CONFIG_USB_R8A66597_HCD)	+= host/
obj-$(CONFIG_USB_HWA_HCD)	+= host/
obj-$(CONFIG_USB_ISP1760_HCD)	+= host/
obj-$(CONFIG_USB_IMX21_HCD)	+= host/
obj-$(CONFIG_USB_FSL_MPH_DR_OF)	+= host/
obj-$(CONFIG_USB_MTK53xx_HCD)	+= host/
obj-$(CONFIG_USB_MTK_XHCI_HCD)	+= host/

obj-$(CONFIG_USB_MU3D_TEST_DRV)	+= mu3d/

obj-$(CONFIG_USB_C67X00_HCD)	+= c67x00/

obj-$(CONFIG_USB_WUSB)		+= wusbcore/

obj-$(CONFIG_USB_ACM)		+= class/
obj-$(CONFIG_USB_PRINTER)	+= class/
obj-$(CONFIG_USB_WDM)		+= class/
obj-$(CONFIG_USB_TMC)		+= class/

obj-$(CONFIG_USB_STORAGE)	+= storage/
obj-$(CONFIG_USB)		+= storage/

obj-$(CONFIG_USB_MDC800)	+= image/
obj-$(CONFIG_USB_MICROTEK)	+= image/

obj-$(CONFIG_USB_SERIAL)	+= serial/

obj-$(CONFIG_USB)		+= misc/
obj-$(CONFIG_USB_PHY)		+= phy/
obj-$(CONFIG_EARLY_PRINTK_DBGP)	+= early/

obj-$(CONFIG_USB_ATM)		+= atm/
obj-$(CONFIG_USB_SPEEDTOUCH)	+= atm/

obj-$(CONFIG_USB_MUSB_HDRC)	+= musb/
obj-$(CONFIG_USB_CHIPIDEA)	+= chipidea/
obj-$(CONFIG_USB_RENESAS_USBHS)	+= renesas_usbhs/

ifeq "$(TV_DRV_VFY)" "true"
obj-$(CONFIG_USB_MTK_MUSB)  += musb_qmu/
obj-$(CONFIG_USB_GADGET)	+= gadget/
else
ifeq ($(filter true, $(USB_ADB) $(LINUX_EMU_USB_CDC)), true)

obj-$(CONFIG_USB_MU3D_DRV)	+= mu3d/
obj-$(CONFIG_USB_MU3D_UDC)	+= mu3d/

obj-$(CONFIG_USB_MTK_MUSB)  += musb_qmu/

obj-$(CONFIG_USB_GADGET)	+= gadget/
endif
endif

obj-$(CONFIG_USB_COMMON)	+= usb-common.o
