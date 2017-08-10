#
# Makefile for USB Host Controller Drivers
#

ccflags-$(CONFIG_USB_DEBUG) := -DDEBUG

ifeq ($(filter true,$(USB_ADB) $(USB_CDC) $(CDC_DUMP_SUPPORT) $(LINUX_EMU_USB_CDC)), true)
#EXTRA_CFLAGS		+= -DUSB_DEVICE_MODE_SUPPORT
#ifndef USB_ADB_PORT
#export USB_ADB_PORT := 0
#endif
endif


isp1760-y := isp1760-hcd.o isp1760-if.o

fhci-y := fhci-hcd.o fhci-hub.o fhci-q.o
fhci-y += fhci-mem.o fhci-tds.o fhci-sched.o

fhci-$(CONFIG_FHCI_DEBUG) += fhci-dbg.o


obj-$(CONFIG_USB_WHCI_HCD)	+= whci/

obj-$(CONFIG_PCI)		+= pci-quirks.o
EXTRA_CFLAGS += -DCONFIG_USB_QMU_SUPPORT
mtk-hcd-objs := mtk_hcd.o mtk_qmu.o mtk_oc.o mtk_cust.o mtk_quirks.o mtk_sched.o
xhci-hcd-objs := xhci.o xhci-mem.o xhci-plat.o xhci-ring.o xhci-hub.o xhci-dbg.o mtk-phy.o xhci-mtk-scheduler.o xhci-mtk-power.o xhci-mtk.o mtk-phy-gpio.o mtk-phy-c60802.o mtk-phy-e60802.o
mtk-hcd-$(CONFIG_USB_MT53xx_COMPLIANCE_TEST) += mtk_debugfs.o
ifeq ($(CONFIG_USB_MT53xx_COMPLIANCE_TEST), y)
EXTRA_CFLAGS += -DMUSB_BC12
endif

obj-$(CONFIG_USB_EHCI_HCD)	+= ehci-hcd.o
obj-$(CONFIG_USB_EHCI_PCI)	+= ehci-pci.o
obj-$(CONFIG_USB_EHCI_HCD_PLATFORM)	+= ehci-platform.o
obj-$(CONFIG_USB_EHCI_MXC)	+= ehci-mxc.o
obj-$(CONFIG_USB_EHCI_HCD_OMAP)	+= ehci-omap.o
obj-$(CONFIG_USB_EHCI_HCD_ORION)	+= ehci-orion.o
obj-$(CONFIG_USB_EHCI_HCD_SPEAR)	+= ehci-spear.o
obj-$(CONFIG_USB_EHCI_S5P)	+= ehci-s5p.o
obj-$(CONFIG_USB_EHCI_HCD_AT91) += ehci-atmel.o
obj-$(CONFIG_USB_EHCI_MSM)	+= ehci-msm.o

obj-$(CONFIG_USB_OXU210HP_HCD)	+= oxu210hp-hcd.o
obj-$(CONFIG_USB_ISP116X_HCD)	+= isp116x-hcd.o
obj-$(CONFIG_USB_ISP1362_HCD)	+= isp1362-hcd.o
obj-$(CONFIG_USB_OHCI_HCD)	+= ohci-hcd.o
obj-$(CONFIG_USB_UHCI_HCD)	+= uhci-hcd.o
obj-$(CONFIG_USB_FHCI_HCD)	+= fhci.o
#ifneq ($(USB_ADB_PORT), 2)
obj-$(CONFIG_USB_MTK_XHCI_HCD)	+= xhci-hcd.o #XHCI_UT/ #xhci-hcd.o
#endif
obj-$(CONFIG_USB_SL811_HCD)	+= sl811-hcd.o
obj-$(CONFIG_USB_SL811_CS)	+= sl811_cs.o
obj-$(CONFIG_USB_U132_HCD)	+= u132-hcd.o
obj-$(CONFIG_USB_R8A66597_HCD)	+= r8a66597-hcd.o
obj-$(CONFIG_USB_ISP1760_HCD)	+= isp1760.o
obj-$(CONFIG_USB_HWA_HCD)	+= hwa-hc.o
obj-$(CONFIG_USB_IMX21_HCD)	+= imx21-hcd.o
obj-$(CONFIG_USB_FSL_MPH_DR_OF)	+= fsl-mph-dr-of.o
obj-$(CONFIG_USB_OCTEON2_COMMON) += octeon2-common.o
obj-$(CONFIG_USB_HCD_BCMA)	+= bcma-hcd.o
obj-$(CONFIG_USB_HCD_SSB)	+= ssb-hcd.o
obj-$(CONFIG_USB_MTK53xx_HCD)	+= mtk-hcd.o
