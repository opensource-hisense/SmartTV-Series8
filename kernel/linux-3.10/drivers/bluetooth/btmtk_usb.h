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

#ifndef __BTMTK_USB_H__
#define __BTMTK_USB_H_

#include "btmtk_define.h"
/* Memory map for MTK BT */

/*support boot on flash*/
#define __MTK_BT_BOOT_ON_FLASH__ 1

typedef unsigned char UINT8;

typedef signed int INT32;
typedef unsigned int UINT32;

typedef unsigned long ULONG;
typedef void VOID;


/* clayton: use this */
typedef struct _OSAL_UNSLEEPABLE_LOCK_ {
	spinlock_t lock;
	ULONG flag;
} OSAL_UNSLEEPABLE_LOCK, *P_OSAL_UNSLEEPABLE_LOCK;

typedef struct {
	OSAL_UNSLEEPABLE_LOCK spin_lock;
	UINT8 buffer[META_BUFFER_SIZE];	/* MTKSTP_BUFFER_SIZE:1024 */
	UINT32 read_p;		/* indicate the current read index */
	UINT32 write_p;		/* indicate the current write index */
} ring_buffer_struct;




#if __MTK_BT_BOOT_ON_FLASH__

//define for bootloader download & flash patch download
//change to 256 because firmware only have 512 bytes to save cmd data
//so not enough memory four 4 cmd
//#define BOOT_MAX_UNIT 512
#define BOOT_MAX_UNIT 256
#define BOOT_SINGLE_UNIT 128

#define FLASH_MAX_UNIT 256
//#define FLASH_MAX_UNIT 512
#define FLASH_SINGLE_UNIT 128

#define FLASH_PATCH_PHASE1 0
#define FLASH_PATCH_PHASE2 1
#define FLASH_PATCH_PHASE3 2

#define BOOTLOADER_UPDATE_DATE 20141218

//return status
#define BT_STATUS_SUCCESS 0
#define BT_STATUS_FLASH_CRC_ERROR  -10

/*
phase 0: start, 1: continue, 2: end
dest, 0: bootloader, 1: patch, 2: version, 3: loader flag
sent_len: current sent length
cur_len: current length of image
write_flash: 0 do not write flash, 1 write flash
data_part: 0, 1, 2, 3 means first second third fourth part
*/
struct patch_down_info
{
	u16 sent_len;
	u32 cur_len;
	u16 single_len;
	u8 phase;
	u8 dest;
	u8 write_flash;
	u8 data_part;
};
#endif

struct btmtk_usb_data {
	struct hci_dev *hdev;
	struct usb_device *udev;	/* store the usb device informaiton */
	struct usb_interface *intf;	/* current interface */
	struct usb_interface *isoc;	/* isochronous interface */

	spinlock_t lock;

	unsigned long flags;
	struct work_struct work;
	struct work_struct waker;

	struct task_struct *fw_dump_tsk;
	struct task_struct *fw_dump_end_check_tsk;
	struct semaphore fw_dump_semaphore;

	struct usb_anchor tx_anchor;
	struct usb_anchor intr_anchor;
	struct usb_anchor bulk_anchor;
	struct usb_anchor isoc_anchor;
	struct usb_anchor deferred;
	int tx_in_flight;

	int meta_tx;

	spinlock_t txlock;

	struct usb_endpoint_descriptor *intr_ep;
	struct usb_endpoint_descriptor *bulk_tx_ep;
	struct usb_endpoint_descriptor *bulk_rx_ep;
	struct usb_endpoint_descriptor *isoc_tx_ep;
	struct usb_endpoint_descriptor *isoc_rx_ep;

	__u8 cmdreq_type;

	unsigned int sco_num;
	int isoc_altsetting;
	int suspend_count;

	/* request for different io operation */
	u8 w_request;
	u8 r_request;

	/* io buffer for usb control transfer */
	unsigned char *io_buf;

	struct semaphore fw_upload_sem;

	unsigned char *fw_image;
	unsigned char *fw_header_image;
	unsigned char *fw_bin_file_name;

	unsigned char *rom_patch;
	unsigned char *rom_patch_header_image;
	unsigned char *rom_patch_bin_file_name;
	u32 chip_id;
	u8 need_load_fw;
	u8 need_load_rom_patch;
	u32 rom_patch_offset;
	u32 rom_patch_len;
	u32 fw_len;

	/* wake on bluetooth */
	unsigned int wobt_irq;
	int wobt_irqlevel;
	atomic_t irq_enable_count;
#if __MTK_BT_BOOT_ON_FLASH__
	unsigned char driver_version[4];
	u32 driver_patch_version; //rom patch version in driver
	u32 flash_patch_version;  //rom patch version read from flash
	u8 patch_version_all_ff;
#endif

};

static inline int is_mt7630(struct btmtk_usb_data *data)
{
	return ((data->chip_id & 0xffff0000) == 0x76300000);
}

static inline int is_mt7650(struct btmtk_usb_data *data)
{
	return ((data->chip_id & 0xffff0000) == 0x76500000);
}

static inline int is_mt7632(struct btmtk_usb_data *data)
{
	return ((data->chip_id & 0xffff0000) == 0x76320000);
}

static inline int is_mt7662(struct btmtk_usb_data *data)
{
	return ((data->chip_id & 0xffff0000) == 0x76620000);
}

static inline int is_mt7662T(struct btmtk_usb_data *data)
{
    return ((data->chip_id & 0xffffffff) == 0x76620100);
}

static inline int is_mt7632T(struct btmtk_usb_data *data)
{
    return ((data->chip_id & 0xffffffff) == 0x76320100);
}

#define REV_MT76x2E3        0x0022

#define MT_REV_LT(_data, _chip, _rev)	\
	is_##_chip(_data) && (((_data)->chip_id & 0x0000ffff) < (_rev))

#define MT_REV_GTE(_data, _chip, _rev)	\
	is_##_chip(_data) && (((_data)->chip_id & 0x0000ffff) >= (_rev))

/*
 *  Load code method
 */
enum LOAD_CODE_METHOD {
	BIN_FILE_METHOD,
	HEADER_METHOD,
};

#endif
