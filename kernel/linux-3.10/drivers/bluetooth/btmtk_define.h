/*
*  Copyright (c) 2014 MediaTek Inc.
*
*  This program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License version 2 as
*  published by the Free Software Foundation.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*  GNU General Public License for more details.
*/

/*THe following setting in this file are different from each project.*/
#ifndef __BTMTK_DEFINE_H__
#define __BTMTK_DEFINE_H__

/*support boot on flash*/
#define __MTK_BT_BOOT_ON_FLASH__ 0


#define BT_DONGLE_RESET_GPIO_PIN 209
/* SYS Control */
#define SYSCTL	0x400000

/* WLAN */
#define WLAN		0x410000

/* MCUCTL */
#define CLOCK_CTL		0x0708
#define INT_LEVEL		0x0718
#define COM_REG0		0x0730
#define SEMAPHORE_00	0x07B0
#define SEMAPHORE_01	0x07B4
#define SEMAPHORE_02	0x07B8
#define SEMAPHORE_03	0x07BC

/* Chip definition */

#define CONTROL_TIMEOUT_JIFFIES     100
#define DEVICE_VENDOR_REQUEST_OUT	0x40
#define DEVICE_VENDOR_REQUEST_IN	0xc0
#define DEVICE_CLASS_REQUEST_OUT	0x20
#define DEVICE_CLASS_REQUEST_IN	    0xa0

#define BTUSB_MAX_ISOC_FRAMES	10
#define BTUSB_INTR_RUNNING	0
#define BTUSB_BULK_RUNNING	1
#define BTUSB_ISOC_RUNNING	2
#define BTUSB_SUSPENDING	3
#define BTUSB_DID_ISO_RESUME	4

/* ROM Patch */
#define PATCH_HCI_HEADER_SIZE 4
#define PATCH_WMT_HEADER_SIZE 5
#define PATCH_HEADER_SIZE (PATCH_HCI_HEADER_SIZE + PATCH_WMT_HEADER_SIZE)
#define UPLOAD_PATCH_UNIT 2048
#define PATCH_INFO_SIZE 30
#define PATCH_PHASE1 1
#define PATCH_PHASE2 2
#define PATCH_PHASE3 3

#define META_BUFFER_SIZE (1024*500)


#define LOAD_PROFILE 1
#define SUPPORT_BT_ATE 1
#define BT_REDUCE_EP2_POLLING_INTERVAL_BY_INTR_TRANSFER 0
#define BT_ROM_PATCH_FROM_BIN 1
#define BT_SEND_HCI_CMD_BEFORE_SUSPEND 1

#define BT_SEND_HCI_CMD_BEFORE_STANDBY 1
#define FW_PATCH "mt7662_patch_e3_hdr.bin"
#define BT_STACK_RESET_CODE -99
#define BT_MAX_EVENT_SIZE 260

/* stpbt device node */
#define BT_NODE "stpbt"
#define BT_DRIVER_NAME "BT_chrdev"
#define BUFFER_SIZE  (1024*4)	/* Size of RX Queue */
#define SUPPORT_FW_DUMP 1
#define SAVE_FW_DUMP_IN_KERNEL 1/*only control save file in kernel or not*/
#define SET_TX_POWER_AFTER_ROM_PATCH	1/*after rom patch,  set power*/
#define IOC_MAGIC        0xb0
#define IOCTL_FW_ASSERT  _IOWR(IOC_MAGIC, 0, void*)
#define BT_ALLOC_BUF 1
#define ENABLE_BT_FILE_SYSTEM 1
#define SUPPORT_FAILED_RESET 1
//#define ENABLE_BT_RECOVERY 1
#define SUPPORT_FW_ASSERT_TRIGGER_KERNEL_PANIC_AND_REBOOT 0
#define FW_DUMP_FILE_NAME "/sdcard/bt_fw_dump"

#define SUPPORT_MT6632 1
#define SUPPORT_MT7637 1

#define SUPPORT_STABLE_WOBLE_WHEN_HW_RESET 1
#define SUPPORT_HCI_DUMP 1
#define SUPPORT_BT_RECOVER 0
#define SUPPORT_REGISTER_EARLY_SUSPEND_LATE_RESUME_NOTIFICATION 1

static struct usb_device_id btmtk_usb_table[] = {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 6, 0)
	{ USB_DEVICE_AND_INTERFACE_INFO(0x0e8d, 0x7662, 0xe0, 0x01, 0x01), .bInterfaceNumber = 0 },
	{ USB_DEVICE_AND_INTERFACE_INFO(0x0e8d, 0x7632, 0xe0, 0x01, 0x01), .bInterfaceNumber = 0 },
	{ USB_DEVICE_AND_INTERFACE_INFO(0x0e8d, 0x76a0, 0xe0, 0x01, 0x01), .bInterfaceNumber = 0 },
	{ USB_DEVICE_AND_INTERFACE_INFO(0x0e8d, 0x76a1, 0xe0, 0x01, 0x01), .bInterfaceNumber = 0 },
	{ USB_DEVICE_AND_INTERFACE_INFO(0x0e8d, 0x7657, 0xe0, 0x01, 0x01), .bInterfaceNumber = 0 },//7637
	{ USB_DEVICE_AND_INTERFACE_INFO(0x0e8d, 0x7666, 0xe0, 0x01, 0x01), .bInterfaceNumber = 0 },//6632 & 7666
#else
	{ USB_DEVICE_AND_INTERFACE_INFO(0x0e8d, 0x7662, 0xe0, 0x01, 0x01)},
	{ USB_DEVICE_AND_INTERFACE_INFO(0x0e8d, 0x7632, 0xe0, 0x01, 0x01)},
	{ USB_DEVICE_AND_INTERFACE_INFO(0x0e8d, 0x76a0, 0xe0, 0x01, 0x01)},
	{ USB_DEVICE_AND_INTERFACE_INFO(0x0e8d, 0x76a1, 0xe0, 0x01, 0x01)},
#endif
	{ }
};




#endif
