ifeq ($(filter true,$(USB_ADB) $(USB_CDC) $(CDC_DUMP_SUPPORT) $(LINUX_EMU_USB_CDC)), true)
#EXTRA_CFLAGS		+= -DUSB_DEVICE_MODE_SUPPORT
#ifndef USB_ADB_PORT
#export USB_ADB_PORT := 0
#endif
endif

ifeq ($(CONFIG_USB_MU3D_O0),y)
	EXTRA_CFLAGS += -O0
endif


obj-$(CONFIG_USB_MU3D_UDC)	+= udc-core.o
obj-$(CONFIG_USB_MU3D)		+= mu3d_hal/
obj-$(CONFIG_USB_MU3D_DRV)	+= mu3d_drv/
obj-$(CONFIG_USB_MU3D_TEST_DRV)	+= mu3d_test_drv/
