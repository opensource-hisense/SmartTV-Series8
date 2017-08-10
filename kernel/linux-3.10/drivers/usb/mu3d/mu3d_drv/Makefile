
EXTRA_CFLAGS += -O0

EXTRA_CFLAGS += -DUSE_SSUSB_QMU
ifeq ($(CONFIG_USB_MU3D_DRV),y)
EXTRA_CFLAGS += -D_USB_NORMAL_
endif

obj-$(CONFIG_USB_MU3D_DRV) += musb_hdrc_u3.o
musb_hdrc_u3-y := musb_core.o
musb_hdrc_u3-y					+= musb_gadget_ep0.o musb_gadget.o 
musb_hdrc_u3-y					+= ssusb_qmu.o musb_init.o 
musb_hdrc_u3-$(CONFIG_DEBUG_FS)			+= musb_debugfs.o
musb_hdrc_u3-y	+= ../mu3d_hal/mu3d_hal_osal.o 
musb_hdrc_u3-y	+= ../mu3d_hal/mu3d_hal_qmu_drv.o 
musb_hdrc_u3-y	+= ../mu3d_hal/mu3d_hal_usb_drv.o 
musb_hdrc_u3-y	+= ../mu3d_hal/mu3d_hal_phy.o 
musb_hdrc_u3-y	+= ../mu3d_hal/mu3d_hal_comm.o 
musb_hdrc_u3-y	+= ../mu3d_hal/mtk-phy.o 
musb_hdrc_u3-y	+= ../mu3d_hal/mtk-phy-c60802.o 
musb_hdrc_u3-y	+= ../mu3d_hal/mtk-phy-e60802.o 
musb_hdrc_u3-y	+= ../mu3d_hal/mtk-phy-gpio.o 
musb_hdrc_u3-y	+= ../mu3d_hal/mtk-phy-ahb.o
