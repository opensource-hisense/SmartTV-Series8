	EXTRA_CFLAGS += -O0

ifeq ($(CONFIG_USB_MU3D_DRV),y)
EXTRA_CFLAGS += -D_USB_NORMAL_
endif

obj-$(CONFIG_USB_MU3D)	+= mu3d_hal_osal.o mu3d_hal_qmu_drv.o mu3d_hal_usb_drv.o mu3d_hal_phy.o mu3d_hal_comm.o mtk-phy.o mtk-phy-c60802.o mtk-phy-e60802.o mtk-phy-gpio.o mtk-phy-ahb.o
