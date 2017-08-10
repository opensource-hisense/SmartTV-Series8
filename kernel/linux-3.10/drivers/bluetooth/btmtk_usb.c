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
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <linux/errno.h>
#include <linux/skbuff.h>
#include <linux/completion.h>
#include <linux/usb.h>
#include <linux/version.h>
#include <linux/firmware.h>
#include <linux/vmalloc.h>
#include <net/bluetooth/bluetooth.h>
#include <net/bluetooth/hci_core.h>
#include "btmtk_usb.h"
#include <linux/kallsyms.h>
#include <linux/cdev.h>

// Local Configuration =========================================================
#define VERSION "3.0.10"
#define LOAD_PROFILE 0
#define SUPPORT_BT_ATE 0
#define SUPPORT_BT_RECOVER 1
#define SUPPORT_BT_WAKEUP 0
#define BT_REDUCE_EP2_POLLING_INTERVAL_BY_INTR_TRANSFER 0
#define BT_ROM_PATCH_FROM_BIN 1
#define BT_SEND_HCI_CMD_BEFORE_SUSPEND 0
#define SUPPORT_FW_DUMP 1
#define SUPPORT_FW_DUMP_UI 0
#define SUPPORT_FAILED_RESET 1
#define SUPPORT_UNPREDICTED_HCI_CMD_FILTER 0
#define SUPPORT_SEND_DUMMY_BULK_OUT_AFTER_RESUME 1
#define SUPPORT_FW_ASSERT_TRIGGER_KERNEL_PANIC_AND_REBOOT 0
#define SUPPORT_CHECK_WAKE_UP_REASON_DECIDE_SEND_HCI_RESET 0
#define SUPPORT_A2DP_LATENCY_MEASUREMENT 0
#define SUPPORT_A2DP_LATENCY_DEBUG 0
#define SUPPORT_PRINT_FW_LOG 0
#define SUPPORT_DEFAULT_TX_POWER_LEVEL_5 0
#define ENABLE_BT_LOW_POWER_MODE 0
#define SUPPORT_STPBT_DEVICE 1

// =============================================================================

#if !defined(BT_STACK_BD) && !defined(BT_STACK_BA)
#define BT_STACK_BD
#endif
#if !defined(BT_RC_VENDOR_N0) && !defined(BT_RC_VENDOR_T0) && !defined(BT_RC_VENDOR_T1)
#define BT_RC_VENDOR_T0
#endif

#if defined(BT_STACK_BA)
#undef SUPPORT_KEEP_HCI_DEV_INSTANCE_WHEN_DISCONNECT
#undef SUPPORT_SEND_DUMMY_ERROR_HCI_EVENT_AFTER_RESUME
#define SUPPORT_KEEP_HCI_DEV_INSTANCE_WHEN_DISCONNECT 1
#define SUPPORT_SEND_DUMMY_ERROR_HCI_EVENT_AFTER_RESUME 1
#endif

#if defined(BT_STACK_BD)
#undef SUPPORT_KEEP_HCI_DEV_INSTANCE_WHEN_DISCONNECT
#undef SUPPORT_SEND_DUMMY_ERROR_HCI_EVENT_AFTER_RESUME
#define SUPPORT_KEEP_HCI_DEV_INSTANCE_WHEN_DISCONNECT 0
#define SUPPORT_SEND_DUMMY_ERROR_HCI_EVENT_AFTER_RESUME 0
#endif

// =============================================================================

#if BT_ROM_PATCH_FROM_BIN
int LOAD_CODE_METHOD = BIN_FILE_METHOD;
unsigned char *mt7662_rom_patch;
unsigned char *mt7662t_rom_patch;
#else
int LOAD_CODE_METHOD = HEADER_METHOD;
#include "./mt7662_rom_patch.h"
#include "./mt7662t_rom_patch.h"
#endif

#define CAM_COLOR_END    "\033[m"
#define CAM_RED          "\033[0;32;31m"

#if SUPPORT_STPBT_DEVICE
/* stpbt character device for meta */
static int BT_major;
struct class *pBTClass = NULL;
struct device *pBTDev = NULL;
static struct cdev BT_cdev;
static int BT_devs = 1;        /* device count */
static int isbtready;
#endif

void btmtk_usb_load_profile(struct hci_dev *hdev);

static struct usb_driver btmtk_usb_driver;
static char driver_version[64]      = {0};
static char rom_patch_version[64]   = {0};
static char fw_version[64]          = {0};
static unsigned char probe_counter  = 0;
static int dongle_reset_enable = 0;
static int dongle_reset_done = 0;

static int timestamp_counter = 0;

#define HCI_DUMP_ENTRY_NUM 30
#define HCI_DUMP_BUF_SIZE 32
static unsigned char hci_cmd_dump[HCI_DUMP_ENTRY_NUM][HCI_DUMP_BUF_SIZE];
static unsigned char hci_cmd_dump_len[HCI_DUMP_ENTRY_NUM] = {0};
static unsigned int  hci_cmd_dump_timestamp[HCI_DUMP_ENTRY_NUM];
static unsigned char hci_event_dump[HCI_DUMP_ENTRY_NUM][HCI_DUMP_BUF_SIZE];
static unsigned char hci_event_dump_len[HCI_DUMP_ENTRY_NUM] = {0};
static unsigned int  hci_event_dump_timestamp[HCI_DUMP_ENTRY_NUM];
static int hci_cmd_dump_index = 0;
static int hci_event_dump_index = 0;

static struct usb_device *g_udev = NULL;

static struct hci_dev *g_hdev = NULL;
static int g_hdev_registered = 0;

static int is_in_skip_all_cmd_state = 0;
static int is_probe_done = 0;
static int should_trigger_core_dump_after_open = 0;

static unsigned int a2dp_latency_debug_last_frame_time = 0;

#if SUPPORT_A2DP_LATENCY_MEASUREMENT
static struct timeval send_frame;
static struct timeval send_frame_done;
static int send_frame_counter = 0;
static int send_frame_done_counter = 0;

struct file *bt_timestamp_file;
mm_segment_t timestamp_fs;

struct timeval before_send;
struct timeval after_send;
int usb_pkt_index=1;

void btmtk_usb_get_microseconds(struct timeval *now)
{
    do_gettimeofday(now);
}

#if 0
void btmtk_usb_print_timestamp(void)
{
    /*if (!IS_ERR(bt_timestamp_file))
    {
        char dbg_buff[512] = {0}; 
        snprintf(dbg_buff, 512, "Index:%d, receive time:(%lds:%ldms - %lds:%ldms) %lds:%ldms\n", 
                    usb_pkt_index, after_send.tv_sec, after_send.tv_usec/1000,
                    before_send.tv_sec, before_send.tv_usec/1000,
                    after_send.tv_sec - before_send.tv_sec, 
                    (after_send.tv_usec - before_send.tv_usec)/1000);
		usb_pkt_index++;
        printk("into print.\n");

        timestamp_fs = get_fs();
        set_fs(KERNEL_DS);

        if(dbg_buff)
        {
            printk("dbg_buff not null.\n");
        }
        else
        {
            printk("dbg_buff null.\n");
        }

        if(bt_timestamp_file->f_pos)
        {
            printk("pos not null.\n");
        }
        else
        {
            printk("pos null.\n");
        }
        
        bt_timestamp_file->f_op->write(bt_timestamp_file, dbg_buff, strlen(dbg_buff), &bt_timestamp_file->f_pos);
        //set_fs(timestamp_fs);
        //fwrite(dbg_buff, 1, strlen(dbg_buff), bt_timestamp_file);
    }
    else
    {
        set_fs(timestamp_fs);
        printk("into print else.\n");
    }*/
   //printk("I:%d, r(%lds:%ldms - %lds:%ldms) %ldms\n", 
   // usb_pkt_index, after_send.tv_sec, after_send.tv_usec/1000,
   // before_send.tv_sec, before_send.tv_usec/1000,
   //((after_send.tv_sec*1000+after_send.tv_usec/1000) - (before_send.tv_sec*1000+before_send.tv_usec/1000)));
   printk("I:%d, %ldms\n", 
    usb_pkt_index, (after_send.tv_sec*1000+after_send.tv_usec/1000));
   usb_pkt_index++;
}
#endif

static void _btmtk_print_timestamp(char *s, int idx, int len, struct timeval *now)
{
    //if (idx % 2 == 0)
        printk(KERN_DEBUG "drv %d\t%d <%s> %ldus\n", idx, len, s, now->tv_sec * 1000 * 1000 + now->tv_usec);
}
#endif


#if __MTK_BT_BOOT_ON_FLASH__
static char rom_patch_crc[8] = {0};
#endif
//=============================================================================
static void btmtk_usb_send_dummy_error_hci_event(struct sk_buff *skb, struct hci_dev *hdev);
static int btmtk_usb_send_switch_to_interrupt_in_cmd(struct usb_device *udev);
static int btmtk_usb_send_tx_power_level_5_cmd(struct usb_device *udev);
static int btmtk_usb_send_core_dump_cmd(struct usb_device *udev);
static int btmtk_usb_send_hci_suspend_cmd(struct usb_device *udev);
static int btmtk_usb_send_enable_low_power_cmd(struct usb_device *udev);
static int btmtk_usb_send_hci_set_ce_cmd(struct usb_device *udev);
#if SUPPORT_BT_WAKEUP
static int btmtk_usb_common_control_transfer(struct usb_device *udev, 
                                                __u8 bRequestType, 
                                                __u8 bRequest, 
                                                __le16 wValue, 
                                                __le16 wIndex, 
                                                unsigned char* buf, 
                                                int len);
static void btmtk_usb_set_feature_remote_wake_up(struct btmtk_usb_data *data);
static void btmtk_usb_clear_feature_remote_wake_up(struct btmtk_usb_data *data);
static int enable_bt_wakeup = 0;
#endif
static unsigned int _btmtk_usb_get_microseconds(void)
{
    struct timeval now;
    do_gettimeofday(&now);
	return (now.tv_sec * 1000000 + now.tv_usec);
}

static void btmtk_usb_dewarning(void) {
    (void)_btmtk_usb_get_microseconds;
    (void)timestamp_counter;
    (void)g_hdev;
    (void)g_hdev_registered;
    (void)hci_cmd_dump;
    (void)hci_cmd_dump_len;
    (void)hci_cmd_dump_timestamp;
    (void)hci_event_dump;
    (void)hci_event_dump_len;
    (void)hci_event_dump_timestamp;
    (void)hci_cmd_dump_index;
    (void)hci_event_dump_index;
    (void)fw_version;
    (void)a2dp_latency_debug_last_frame_time;
    (void)btmtk_usb_send_dummy_error_hci_event;
    (void)btmtk_usb_send_switch_to_interrupt_in_cmd;
    (void)btmtk_usb_send_tx_power_level_5_cmd;
    (void)btmtk_usb_send_core_dump_cmd;
    (void)btmtk_usb_send_hci_suspend_cmd;
    (void)is_in_skip_all_cmd_state;
    (void)btmtk_usb_send_enable_low_power_cmd;
    (void)btmtk_usb_send_hci_set_ce_cmd;
}

static void btmtk_usb_hci_dmp_init(void) {
    int i;
    hci_cmd_dump_index = HCI_DUMP_ENTRY_NUM-1;
    hci_event_dump_index = HCI_DUMP_ENTRY_NUM-1;
    for ( i = 0; i < HCI_DUMP_ENTRY_NUM; i++ ) {
        hci_cmd_dump_len[i] = 0;
        hci_event_dump_len[i] = 0;
    }
}

static void btmtk_usb_save_hci_cmd(int len, unsigned char* buf) {
    int copy_len = HCI_DUMP_BUF_SIZE;
    if ( buf ) {
        if ( len < HCI_DUMP_BUF_SIZE )
            copy_len = len;
        hci_cmd_dump_len[hci_cmd_dump_index] = copy_len&0xff;
        memset(hci_cmd_dump[hci_cmd_dump_index], 0, HCI_DUMP_BUF_SIZE);
        memcpy(hci_cmd_dump[hci_cmd_dump_index], buf, copy_len&0xff);
        hci_cmd_dump_timestamp[hci_cmd_dump_index] = _btmtk_usb_get_microseconds();

        hci_cmd_dump_index--;
        if ( hci_cmd_dump_index < 0 )
            hci_cmd_dump_index = HCI_DUMP_ENTRY_NUM-1;
    }
}

static void btmtk_usb_save_hci_event(int len, unsigned char* buf) {
    int copy_len = HCI_DUMP_BUF_SIZE;
    if ( buf ) {
        if ( len < HCI_DUMP_BUF_SIZE )
            copy_len = len;
        hci_event_dump_len[hci_event_dump_index] = copy_len;
        memset(hci_event_dump[hci_event_dump_index], 0, HCI_DUMP_BUF_SIZE);
        memcpy(hci_event_dump[hci_event_dump_index], buf, copy_len);
        hci_event_dump_timestamp[hci_cmd_dump_index] = _btmtk_usb_get_microseconds();

        hci_event_dump_index--;
        if ( hci_event_dump_index < 0 )
            hci_event_dump_index = HCI_DUMP_ENTRY_NUM-1;
    }
}

static void btmtk_usb_hci_dump_print_to_log(void) {
    int counter, index, j;

    printk("btmtk_usb : HCI Command Dump\n");
    printk("    index(len)(timestamp:us) :HCI Command\n");
    index = hci_cmd_dump_index+1;
    if ( index >= HCI_DUMP_ENTRY_NUM )
        index = 0;
    for ( counter = 0; counter < HCI_DUMP_ENTRY_NUM; counter++ ) {
        if ( hci_cmd_dump_len[index] > 0 ) {
            printk("    %d(%02d)(%u) :", counter, hci_cmd_dump_len[index], hci_cmd_dump_timestamp[index]);
            for ( j = 0; j < hci_cmd_dump_len[index]; j++ ) {
                printk("%02X ", hci_cmd_dump[index][j]);
            }
            printk("\n");
        }
        index++;
        if ( index >= HCI_DUMP_ENTRY_NUM )
            index = 0;
    }

    printk("btmtk_usb : HCI Event Dump\n");
    printk("    index(len)(timestamp:us) :HCI Event\n");
    index = hci_event_dump_index+1;
    if ( index >= HCI_DUMP_ENTRY_NUM )
        index = 0;
    for ( counter = 0; counter < HCI_DUMP_ENTRY_NUM; counter++ ) {
        if ( hci_event_dump_len[index] > 0 ) {
            printk("    %d(%02d)(%u) :", counter, hci_event_dump_len[index], hci_event_dump_timestamp[index]);
            for ( j = 0; j < hci_event_dump_len[index]; j++ ) {
                printk("%02X ", hci_event_dump[index][j]);
            }
            printk("\n");
        }
        index++;
        if ( index >= HCI_DUMP_ENTRY_NUM )
            index = 0;
    }
}

void btmtk_usb_hex_dump(char *str, u8 *src_buf, u32 src_buf_len) {
    unsigned char *pt;
    int x;

    pt = src_buf;

    printk("%s: %p, len = %d\n", str, src_buf, src_buf_len);

    for (x = 0; x < src_buf_len; x++) {
        if (x % 16 == 0)
            printk("0x%04x : ", x);
        printk("%02x ", ((unsigned char)pt[x]));
        if (x % 16 == 15)
            printk("\n");
    }

    printk("\n");
}

void btmtk_usb_force_assert(void) {
#if SUPPORT_FW_ASSERT_TRIGGER_KERNEL_PANIC_AND_REBOOT
    printk("%s\n", __FUNCTION__);
    BUG();
#endif
}

void btmtk_usb_toggle_rst_pin(void) {
    dongle_reset_enable = 1;

#if SUPPORT_FAILED_RESET
    printk("%s\n", __FUNCTION__);
/*    #if  SUPPORT_BT_RECOVER
    typedef int (*fpGpio_set)(int);

    fpGpio_set mtkGpioFunc = NULL;

    mtkGpioFunc = (fpGpio_set)kallsyms_lookup_name("MDrv_GPIO_Pull_Low");
    if (mtkGpioFunc) {
        printk("[%s]-%s, Call MDrv_GPIO_Pull_Low :(%p) \n", __this_module, __FUNCTION__, mtkGpioFunc);
        mtkGpioFunc(57);
    }
    else {
        printk("[%s]-%s, Call MDrv_GPIO_Pull_Low :(fail!) \n", __this_module, __FUNCTION__);
    }

    mdelay(100);

    mtkGpioFunc = NULL;
    mtkGpioFunc = (fpGpio_set)kallsyms_lookup_name("MDrv_GPIO_Pull_High");
    if (mtkGpioFunc) {
        printk("[%s]-%s, Call MDrv_GPIO_Pull_Hig :(%p) \n", __this_module, __FUNCTION__, mtkGpioFunc);
        mtkGpioFunc(57);
    }
    else {
        printk("[%s]-%s, Call MDrv_GPIO_Pull_Hig :(fail!) \n", __this_module, __FUNCTION__);
    }

    btmtk_usb_force_assert();
    #else*/
    printk("%s\n", __FUNCTION__);
    {
        extern int mtk_gpio_direction_output(unsigned gpio, int init_value);
        mtk_gpio_direction_output(213, 0);
        mdelay(100);
        mtk_gpio_direction_output(213, 1);
    }

    btmtk_usb_force_assert();
//    #endif
#else
    printk("%s : No action due to GPIO not defined.\n", __FUNCTION__);
#endif
}

static int btmtk_usb_reset(struct usb_device *udev) {
    int ret;

    printk("%s\n", __FUNCTION__);

    ret = usb_control_msg(udev, usb_sndctrlpipe(udev, 0), 0x01, DEVICE_VENDOR_REQUEST_OUT,
                          0x01, 0x00, NULL, 0x00, CONTROL_TIMEOUT_JIFFIES);

    if (ret < 0) {
        printk("%s error(%d)\n", __FUNCTION__, ret);
        return ret;
    }

    if (ret > 0)
        ret = 0;

    return ret;
}

static int btmtk_usb_io_read32(struct btmtk_usb_data *data, u32 reg, u32 *val) {
    u8 request = data->r_request;
    struct usb_device *udev = data->udev;
    int ret;

    ret = usb_control_msg(udev, usb_rcvctrlpipe(udev, 0), request, DEVICE_VENDOR_REQUEST_IN,
                          0x0, reg, data->io_buf, 4,
                          CONTROL_TIMEOUT_JIFFIES);

    if (ret < 0) {
        *val = 0xffffffff;
        printk("%s error(%d), reg = %x, value = %x\n", __FUNCTION__, ret, reg, *val);
        btmtk_usb_hci_dump_print_to_log();
        return ret;
    }

    memmove(val, data->io_buf, 4);

    *val = le32_to_cpu(*val);

    if (ret > 0)
        ret = 0;

    return ret;
}

static int btmtk_usb_io_write32(struct btmtk_usb_data *data, u32 reg, u32 val) {
    u16 value, index;
    u8 request = data->w_request;
    struct usb_device *udev = data->udev;
    int ret;

    index = (u16)reg;
    value = val & 0x0000ffff;

    ret = usb_control_msg(udev, usb_sndctrlpipe(udev, 0), request, DEVICE_VENDOR_REQUEST_OUT,
                          value, index, NULL, 0,
                          CONTROL_TIMEOUT_JIFFIES);

    if (ret < 0) {
        printk("%s error1(%d), reg = %x, value = %x\n", __FUNCTION__, ret, reg, val);
        btmtk_usb_hci_dump_print_to_log();
        return ret;
    }

    index = (u16)(reg + 2);
    value = (val & 0xffff0000) >> 16;

    ret = usb_control_msg(udev, usb_sndctrlpipe(udev, 0), request, DEVICE_VENDOR_REQUEST_OUT,
                          value, index, NULL, 0,
                          CONTROL_TIMEOUT_JIFFIES);

    if (ret < 0) {
        printk("%s error2(%d), reg = %x, value = %x\n", __FUNCTION__, ret, reg, val);
        btmtk_usb_hci_dump_print_to_log();
        return ret;
    }

    if (ret > 0)
        ret = 0;

    return ret;
}

#if SUPPORT_BT_ATE
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 13, 0)
static void usb_ate_hci_cmd_complete(struct urb *urb) {
    if ( urb ) {
        if ( urb->setup_packet )
            kfree(urb->setup_packet);
        if ( urb->transfer_buffer )
            kfree(urb->transfer_buffer);
        urb->setup_packet = NULL;
        urb->transfer_buffer = NULL;
    }
}

static int usb_send_ate_hci_cmd(struct usb_device *udev, unsigned char* buf, int len) {
    struct urb *urb;
    struct usb_ctrlrequest    *class_request;
    unsigned char    *hci_cmd;
    unsigned int pipe;
    int err;
    int i;

    urb = usb_alloc_urb(0, GFP_ATOMIC);
    if (!urb) {
        printk("%s: allocate usb urb failed!\n", __FUNCTION__);
        return -ENOMEM;
    }

    class_request = kmalloc(sizeof(struct usb_ctrlrequest), GFP_ATOMIC);
    if (!class_request) {
        usb_free_urb(urb);
        printk("%s: allocate class request failed!\n", __FUNCTION__);
        return -ENOMEM;
    }

    hci_cmd = kmalloc(len, GFP_ATOMIC);
    if (!hci_cmd) {
        usb_free_urb(urb);
        kfree(class_request);
        printk("%s: allocate hci_cmd failed!\n", __FUNCTION__);
        return -ENOMEM;
    }

    for ( i = 0; i < len; i++ ) {
        hci_cmd[i] = buf[i];
    }

    class_request->bRequestType = USB_TYPE_CLASS;
    class_request->bRequest = 0;
    class_request->wIndex = 0;
    class_request->wValue = 0;
    class_request->wLength = len;

    pipe = usb_sndctrlpipe(udev, 0x0);

    usb_fill_control_urb(urb, udev, pipe, (void *)class_request,
            hci_cmd, len,
            usb_ate_hci_cmd_complete, udev);

    err = usb_submit_urb(urb, GFP_ATOMIC);
    if (err < 0) {
        kfree(urb->setup_packet);
        kfree(urb->transfer_buffer);
    }
    else {
        usb_mark_last_busy(udev);
    }

    usb_free_urb(urb);
    return err;
}
#endif
#endif  // SUPPORT_BT_ATE

#if SUPPORT_FW_DUMP
#include <asm/uaccess.h>
#include <linux/fs.h>
#include <linux/kthread.h>

// #define FW_DUMP_FILE_NAME "/data/@btmtk/bt_fw_dump"
#define FW_DUMP_FILE_NAME "/tmp/bt_fw_dump"
#define FW_DUMP_BUF_SIZE (1024*64)
static struct file *fw_dump_file = NULL;
static char fw_dump_file_name[64] = {0};
int fw_dump_total_read_size = 0;
int fw_dump_total_write_size = 0;
int fw_dump_buffer_used_size = 0;
int fw_dump_buffer_full = 0;
int fw_dump_task_should_stop = 0;
u8* fw_dump_ptr = NULL;
u8* fw_dump_read_ptr = NULL;
u8* fw_dump_write_ptr = NULL;
struct timeval fw_dump_last_write_time;

int fw_dump_end_checking_task_should_stop = 0;
int fw_dump_end_checking_task_stoped = 0;
struct urb *fw_dump_bulk_urb = NULL;

static int btmtk_usb_fw_dump_end_checking_thread(void* param) {
    struct timeval t;
    while (1) {
        mdelay(300);

        if ( kthread_should_stop() || fw_dump_end_checking_task_should_stop ) {
            printk("%s : stop before fw dump finish !\n", __FUNCTION__);
            fw_dump_end_checking_task_should_stop = 0;
            fw_dump_end_checking_task_stoped = 1;
            btmtk_usb_toggle_rst_pin();
            return 0;
        }

        printk("%s : FW dump on going ... %d:%d\n", "btmtk_usb", fw_dump_total_read_size, fw_dump_total_write_size);

        do_gettimeofday(&t);
        if ( (t.tv_sec-fw_dump_last_write_time.tv_sec) > 1 ) {
            printk("%s : fw dump total read size = %d\n", "btmtk_usb", fw_dump_total_read_size);
            printk("%s : fw dump total write size = %d\n", "btmtk_usb", fw_dump_total_write_size);

            if ( fw_dump_file ) {
                vfs_fsync(fw_dump_file, 0);
                printk("%s : close file  %s\n", __FUNCTION__, fw_dump_file_name);
                filp_close(fw_dump_file, NULL);
                fw_dump_file = NULL;
            }

            fw_dump_end_checking_task_should_stop = 0;
            fw_dump_end_checking_task_stoped = 1;
            btmtk_usb_toggle_rst_pin();
            printk("%s : stop\n", __FUNCTION__);
            return 0;
        }
    }
    return 0;
}

static int btmtk_usb_fw_dump_thread(void* param) {
    struct btmtk_usb_data *data = param;
    mm_segment_t old_fs;
    int len;

    while ( down_interruptible(&data->fw_dump_semaphore) == 0 ) {
        if ( kthread_should_stop() || fw_dump_task_should_stop ) {
            fw_dump_task_should_stop = 0;
            return 0;
        }

        len = fw_dump_write_ptr - fw_dump_read_ptr;

        if ( len > 0 && fw_dump_read_ptr != NULL ) {
            old_fs = get_fs();
            set_fs(KERNEL_DS);

            if ( fw_dump_file == NULL ) {
#if SUPPORT_FW_DUMP_UI
                void DRVCUST_SetErrorPattern(unsigned char u1ID);
                DRVCUST_SetErrorPattern(0);
#endif

                memset(fw_dump_file_name, 0, sizeof(fw_dump_file_name));
                snprintf(fw_dump_file_name, sizeof(fw_dump_file_name), FW_DUMP_FILE_NAME"_%d", probe_counter);
                printk("%s : open file %s \n", __FUNCTION__, fw_dump_file_name);
                fw_dump_file = filp_open(fw_dump_file_name, O_RDWR | O_CREAT, 0644);
                if (IS_ERR(fw_dump_file)) {
                    printk("%s : error occured while opening file %s.\n", __FUNCTION__, fw_dump_file_name);
                    set_fs(old_fs);
                    return 0;
                }
                printk("%s : FW dump started ! print HCI command/event : \n", __FUNCTION__);
                btmtk_usb_hci_dump_print_to_log();
            }

            if ( fw_dump_file != NULL ) {
                fw_dump_file->f_op->write(fw_dump_file, fw_dump_read_ptr, len, &fw_dump_file->f_pos);
                fw_dump_read_ptr += len;
                fw_dump_total_write_size += len;
                do_gettimeofday(&fw_dump_last_write_time);
            }
            set_fs(old_fs);
        }

        if ( fw_dump_buffer_full && fw_dump_write_ptr == fw_dump_read_ptr ) {
            int err;
            fw_dump_buffer_full = 0;
            fw_dump_buffer_used_size = 0;
            fw_dump_read_ptr = fw_dump_ptr;
            fw_dump_write_ptr = fw_dump_ptr;

            err = usb_submit_urb(fw_dump_bulk_urb, GFP_ATOMIC);
            if (err < 0) {
                /* -EPERM: urb is being killed;
                 * -ENODEV: device got disconnected */
                if (err != -EPERM && err != -ENODEV)
                    printk("%s: urb %p failed to resubmit bulk_in_urb(%d)", __FUNCTION__, fw_dump_bulk_urb, -err);
                usb_unanchor_urb(fw_dump_bulk_urb);
            }
        }

        if ( data->fw_dump_end_check_tsk == NULL ) {
            fw_dump_end_checking_task_should_stop = 0;
            fw_dump_end_checking_task_stoped = 0;
            data->fw_dump_end_check_tsk = kthread_create(btmtk_usb_fw_dump_end_checking_thread, (void*)data, "btmtk_usb_fw_dump_end_checking_thread");
            if (IS_ERR(data->fw_dump_end_check_tsk)) {
                printk("%s : create fw dump end check thread failed!\n", __FUNCTION__);
                data->fw_dump_end_check_tsk = NULL;
            }
            else {
                wake_up_process(data->fw_dump_end_check_tsk);
            }
        }
    }

    printk("%s end : down != 0\n", __FUNCTION__);
    return 0;
}
#endif

#define DUMMY_BULK_OUT_BUF_SIZE 8
static void btmtk_usb_send_dummy_bulk_out_packet(struct btmtk_usb_data *data) {
    int ret = 0;
    int actual_len;
    unsigned int pipe;
    unsigned char dummy_bulk_out_fuffer[DUMMY_BULK_OUT_BUF_SIZE] = {0};

    pipe = usb_sndbulkpipe(data->udev, data->bulk_tx_ep->bEndpointAddress);
    ret = usb_bulk_msg(data->udev, pipe, dummy_bulk_out_fuffer, DUMMY_BULK_OUT_BUF_SIZE, &actual_len, 100);
    if (ret)
        printk("%s: submit dummy bulk out failed!\n", __FUNCTION__);
    else
        printk("%s : OK\n", __FUNCTION__);

    ret = usb_bulk_msg(data->udev, pipe, dummy_bulk_out_fuffer, DUMMY_BULK_OUT_BUF_SIZE, &actual_len, 100);
    if (ret)
        printk("%s: submit dummy bulk out failed!\n", __FUNCTION__);
    else
        printk("%s : OK\n", __FUNCTION__);
}

static void btmtk_usb_send_dummy_error_hci_event(struct sk_buff *skb, struct hci_dev *hdev) {
    int index;
    u8 error_event_buf[] = {0x0F, 0x04, 0x00, 0x01, 0xFF, 0xFF};

    if ( skb != NULL ) {
        printk("%s : got command (after resume) : 0x", __FUNCTION__);
        for ( index = 0; index < skb->len; index++ )
            printk("%02X ", skb->data[index]);
        printk("\n");
        printk("%s : skip it\n", __FUNCTION__);
    }

    printk("%s : send dummy error event : 0x0F 04 00 01 FF FF\n", __FUNCTION__);

    if (hci_recv_fragment(hdev, HCI_EVENT_PKT,
                    (void*)error_event_buf,
                    sizeof(error_event_buf)) < 0) {
        printk("%s : %s corrupted event packet\n", __FUNCTION__, hdev->name);
        hdev->stat.err_rx++;
    }
}

static int btmtk_usb_send_switch_to_interrupt_in_cmd(struct usb_device *udev) {
    int ret;


    ret = usb_control_msg(udev,
                          usb_sndctrlpipe(udev, 0),
                          0x91,                       // request
                          DEVICE_VENDOR_REQUEST_OUT,  // request type
                          0x04,                       // value
                          0x2403,                     // index
                          NULL,                       // data
                          0x00,                       // size
                          CONTROL_TIMEOUT_JIFFIES);   // timeout

    if (ret < 0) {
        if ( ret == -71 ) {
            printk("%s OK\n", __FUNCTION__);
        }
        else {
            printk("%s error (%d)\n", __FUNCTION__, ret);
        }
        return ret;
    }

    if (ret > 0)
        ret = 0;

    return ret;
}

static int btmtk_usb_send_tx_power_level_5_cmd(struct usb_device *udev) {
    int ret = 0;
    char buf[9] = {0x79, 0xFC, 0x06, 0x05, 0x00, 0x80, 0x00, 0x05, 0x05};

    ret = usb_control_msg(udev, usb_sndctrlpipe(udev, 0), 0x0, DEVICE_CLASS_REQUEST_OUT,
                          0x00, 0x00, buf, 0x09, CONTROL_TIMEOUT_JIFFIES);

    if (ret < 0) {
        printk("%s error1(%d)\n", __FUNCTION__, ret);
        return ret;
    }
    printk("%s : OK\n", __FUNCTION__);

    return 0;
}

static int btmtk_usb_send_core_dump_cmd(struct usb_device *udev) {
    int ret = 0;
    char buf[8] = {0x6f, 0xFC, 0x05, 0x01, 0x02, 0x01, 0x00, 0x08};

    printk("%s : Trigger FW assert by sending HCI Command (0x6f fc 05 01 02 01 00 08)\n", __FUNCTION__);

    ret = usb_control_msg(udev, usb_sndctrlpipe(udev, 0), 0x0, DEVICE_CLASS_REQUEST_OUT,
                          0x00, 0x00, buf, sizeof(buf), CONTROL_TIMEOUT_JIFFIES);

    if (ret < 0) {
        printk("%s error : %d\n", __FUNCTION__, ret);
    }

    return ret;
}

static int btmtk_usb_send_hci_suspend_cmd(struct usb_device *udev) {
    int ret = 0;
#if defined(BT_RC_VENDOR_T0)
    char buf[] = {0xc9, 0xfc, 0x0e, 0x01, 0x0e, 0x00, 0x06, 0x4d, 0x54, 0x4b, 0x2d, 0x52, 0x43, 0x20, 0x04, 0x32, 0x00};
    ret = usb_control_msg(udev, usb_sndctrlpipe(udev, 0), 0x0, DEVICE_CLASS_REQUEST_OUT, 0x00, 0x00, buf, 0x11, CONTROL_TIMEOUT_JIFFIES);
#elif defined(BT_RC_VENDOR_T1)
    {
        // yellow: gpio5; blue:gpio19
        char buf2[] = {0x6f, 0xfc, 0x38,                        // HCI header
            0x01, 0x18, 0x34, 0x00, 0x00, 0x01, 0x00, 0x00,
            0x05, 0x01, 0x00, 0x00, 0x01, 0x80, 0x64, 0x00,
            0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80,
            0x01, 0x80, 0x64, 0x00, 0x01, 0x80, 0x00, 0x00,
            0x13, 0x01, 0x00, 0x00, 0x01, 0x80, 0x01, 0x80,
            0x01, 0x80, 0x01, 0x80, 0x64, 0x00, 0x01, 0x80,
            0x01, 0x80, 0x64, 0x00, 0x01, 0x80, 0x00, 0x00};

        ret = usb_control_msg(udev, usb_sndctrlpipe(udev, 0), 0x1, DEVICE_CLASS_REQUEST_OUT, 0x30, 0x00, buf2, 59, CONTROL_TIMEOUT_JIFFIES);

        if (ret < 0) {
            printk("%s error1(%d)\n", __FUNCTION__, ret);
            return ret;
        }
        printk("%s : sent WMT OK\n", __FUNCTION__);
    }
    {
        char buf[] = {0xc9, 0xfc, 0x02, 0x01, 0x0c};
        ret = usb_control_msg(udev, usb_sndctrlpipe(udev, 0), 0x0, DEVICE_CLASS_REQUEST_OUT, 0x00, 0x00, buf, 0x05, CONTROL_TIMEOUT_JIFFIES);
    }
#elif defined(BT_RC_VENDOR_N0)
    char buf[] = {0xc9, 0xfc, 0x02, 0x01, 0x0a};
    ret = usb_control_msg(udev, usb_sndctrlpipe(udev, 0), 0x0, DEVICE_CLASS_REQUEST_OUT, 0x00, 0x00, buf, 0x05, CONTROL_TIMEOUT_JIFFIES);
#else
    printk("%s : not handled\n", __FUNCTION__);
    return 0
#endif

    if (ret < 0) {
        printk("%s error1(%d)\n", __FUNCTION__, ret);
        return ret;
    }
    printk("%s : OK\n", __FUNCTION__);

    return 0;
}

static int btmtk_usb_send_hci_reset_cmd(struct usb_device *udev) {
    int retry_counter = 0;
    // Send HCI Reset
    {
        int ret = 0;
        char buf[4] = {0};
        buf[0] = 0x03;
        buf[1] = 0x0C;
        ret = usb_control_msg(udev, usb_sndctrlpipe(udev, 0), 0x0, DEVICE_CLASS_REQUEST_OUT,
                              0x00, 0x00, buf, 0x03, CONTROL_TIMEOUT_JIFFIES);

        if (ret < 0) {
            printk("%s error1(%d)\n", __FUNCTION__, ret);
            return ret;
        }
    }

    // Get response of HCI reset
    {
        while (1) {
            int ret = 0;
            char buf[64] = {0};
            int actual_length;
            ret = usb_interrupt_msg(udev, usb_rcvintpipe(udev, 1),
                                    buf, 64, &actual_length, 2000);

            if (ret < 0) {
                printk("%s error2(%d). (-110 means timeout, command sent successfully, but no event back!)\n", __FUNCTION__, ret);
                printk("%s : trigger assert when btmtk_usb_open() been invoked\n", __FUNCTION__);
                should_trigger_core_dump_after_open = 1;
                return -1;
            }

            if ( actual_length == 6 &&
                    buf[0] == 0x0e &&
                    buf[1] == 0x04 &&
                    buf[2] == 0x01 &&
                    buf[3] == 0x03 &&
                    buf[4] == 0x0c &&
                    buf[5] == 0x00 ) {
                break;
            }
            else {
                int i;
                printk("%s,  drop unknown event : \n", __FUNCTION__);
                for ( i = 0; i < actual_length && i < 64; i++ ) {
                    printk("%02X ", buf[i]);
                }
                printk("\n");
                mdelay(10);
                retry_counter++;
            }

            if ( retry_counter > 10 ) {
                printk("%s retry timeout!\n", __FUNCTION__);
                return ret;
            }
        }
    }

    printk("%s : OK\n", __FUNCTION__);
    return 0;
}

static int btmtk_usb_send_enable_low_power_cmd(struct usb_device *udev) {
    // Send 0x41 fc 00
    {
        int ret = 0;
        char buf[3] = {0x41, 0xfc, 0x00};
        ret = usb_control_msg(udev, usb_sndctrlpipe(udev, 0), 0x0, DEVICE_CLASS_REQUEST_OUT,
                              0x00, 0x00, buf, sizeof(buf), CONTROL_TIMEOUT_JIFFIES);

        if (ret < 0) {
            printk("%s error1(%d)\n", __FUNCTION__, ret);
            return ret;
        }
    }


    // Get response
    {
        int retry_counter = 0;
        while (1) {
            int ret = 0;
            unsigned char buf[64] = {0};
            int actual_length;
            ret = usb_interrupt_msg(udev, usb_rcvintpipe(udev, 1),
                                    buf, 64, &actual_length, 2000);

            if (ret < 0) {
                printk("%s error2(%d)\n", __FUNCTION__, ret);
                return ret;
            }

            if ( actual_length == 6 &&
                    buf[0] == 0x0e &&
                    buf[1] == 0x04 &&
                    buf[2] == 0x01 &&
                    buf[3] == 0x41 &&
                    buf[4] == 0xfc &&
                    buf[5] == 0x00 ) {
                break;
            }
            else {
                int i;
                printk("%s : drop unknown event : \n", __FUNCTION__);
                for ( i = 0; i < actual_length && i < 64; i++ ) {
                    printk("%02X ", buf[i]);
                }
                printk("\n");
                mdelay(10);
                retry_counter++;
            }

            if ( retry_counter > 10 ) {
                printk("%s retry timeout!\n", __FUNCTION__);
                return ret;
            }
        }
    }

    printk("%s : OK\n", __FUNCTION__);
    return 0;
}

// disable bt  low  power
#if !ENABLE_BT_LOW_POWER_MODE
static int btmtk_usb_send_disable_low_power_cmd(struct usb_device *udev) {
    // Send 0x40 fc 00
    {
        int ret = 0;
        char buf[3] = {0x40, 0xfc, 0x00};
        ret = usb_control_msg(udev, usb_sndctrlpipe(udev, 0), 0x0, DEVICE_CLASS_REQUEST_OUT,
                              0x00, 0x00, buf, sizeof(buf), CONTROL_TIMEOUT_JIFFIES);

        if (ret < 0) {
            printk("%s error1(%d)\n", __FUNCTION__, ret);
            return ret;
        }
    }


    // Get response
    {
        int retry_counter = 0;
        while (1) {
            int ret = 0;
            unsigned char buf[64] = {0};
            int actual_length;
            ret = usb_interrupt_msg(udev, usb_rcvintpipe(udev, 1),
                                    buf, 64, &actual_length, 2000);

            if (ret < 0) {
                printk("%s error2(%d)\n", __FUNCTION__, ret);
                return ret;
            }

            if ( actual_length == 6 &&
                    buf[0] == 0x0e &&
                    buf[1] == 0x04 &&
                    buf[2] == 0x01 &&
                    buf[3] == 0x40 &&
                    buf[4] == 0xfc &&
                    buf[5] == 0x00 ) {
                break;
            }
            else {
                int i;
                printk("%s : drop unknown event : \n", __FUNCTION__);
                for ( i = 0; i < actual_length && i < 64; i++ ) {
                    printk("%02X ", buf[i]);
                }
                printk("\n");
                mdelay(10);
                retry_counter++;
            }

            if ( retry_counter > 10 ) {
                printk("%s retry timeout!\n", __FUNCTION__);
                return ret;
            }
        }
    }

    printk("%s : OK\n", __FUNCTION__);
    return 0;
}

static int btmtk_usb_send_hci_set_ce_cmd_to_zero(struct usb_device *udev) {
    char result_buf[64] = {0};

    // Read 0x41070c
    {
        int ret = 0;
        char buf[8] = {0xd1, 0xFC, 0x04, 0x0c, 0x07, 0x41, 0x00};
        ret = usb_control_msg(udev, usb_sndctrlpipe(udev, 0), 0x0, DEVICE_CLASS_REQUEST_OUT,
                              0x00, 0x00, buf, 0x07, CONTROL_TIMEOUT_JIFFIES);

        if (ret < 0) {
            printk("%s error1(%d)\n", __FUNCTION__, ret);
            return ret;
        }
    }

    // Get response of read command
    {
        int ret = 0;
        int actual_length;
        ret = usb_interrupt_msg(udev, usb_rcvintpipe(udev, 1),
                                result_buf, 64, &actual_length, 2000);

        if (ret < 0) {
            printk("%s error2(%d)\n", __FUNCTION__, ret);
            return ret;
        }
//        else {
//            if (  result_buf[6] & 0x01 ) {
//                printk("%s, warning, 0x41070c[0] is 1!\n", __FUNCTION__);
//            }
//        }
    }

    // Write 0x41070c[0] to 0
    {
        int ret = 0;
        char buf[12] = {0xd0, 0xfc, 0x08, 0x0c, 0x07, 0x41, 0x00};
        buf[7] = result_buf[6]&0xFE;
        buf[8] = result_buf[7];
        buf[9] = result_buf[8];
        buf[10] = result_buf[9];
        ret = usb_control_msg(udev, usb_sndctrlpipe(udev, 0), 0x0, DEVICE_CLASS_REQUEST_OUT,
                              0x00, 0x00, buf, 0x0b, CONTROL_TIMEOUT_JIFFIES);

        if (ret < 0) {
            printk("%s error1(%d)\n", __FUNCTION__, ret);
            return ret;
        }
    }

    printk("%s : OK\n", __FUNCTION__);
    return 0;
}
#endif  // !ENABLE_BT_LOW_POWER_MODE

#if __MTK_BT_BOOT_ON_FLASH__
static int btmtk_usb_read_dummy_event_2(struct usb_device *udev) {
    // use usb_interrupt_msg to see if have unexpected hci event reset
    {
        int ret = 0;
        char buf[64] = {0};
        int actual_length;
        ret = usb_interrupt_msg(udev, usb_rcvintpipe(udev, 1),
                                buf, 64, &actual_length, 2000);

        if (ret < 0) {
            printk("%s no dummy event ret(%d)\n", __FUNCTION__, ret);
            return ret;
        }
        printk("get unexpect dummy event_2, actual_length:%d, 0x%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X \n",
                    actual_length, buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7], buf[8], buf[9]);
    }
    return 0;
}

static int btmtk_usb_read_dummy_event(struct usb_device *udev) {
    // use usb_interrupt_msg to see if have unexpected hci event reset
    {
        int ret = 0;
        char buf[64] = {0};
        int actual_length;
        ret = usb_interrupt_msg(udev, usb_rcvintpipe(udev, 1),
                                buf, 64, &actual_length, 20);

        if (ret < 0) {
            BT_DBG("%s no dummy event ret(%d)\n", __FUNCTION__, ret);
            return ret;
        }
        printk("get unexpect dummy event, actual_length:%d, 0x%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X \n",
                    actual_length, buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7], buf[8], buf[9]);
        return ret;
    }
}

static int btmtk_usb_get_flash_patch_version(struct btmtk_usb_data *data) {
    char result_buf[64] = {0};
    u8 year1 = 0, year2 = 0, month = 0, day = 0;

    data->patch_version_all_ff = 0;
    btmtk_usb_read_dummy_event(data->udev);    // read dummy hci event
    // Send fcd1 cmd to read flash patch_version flag address: 0x00503ffc
    {
        int ret = 0;
        char buf[8] = {0xd1, 0xFC, 0x04, 0xfc, 0x3f, 0x50, 0x00};
        ret = usb_control_msg(data->udev, usb_sndctrlpipe(data->udev, 0), 0x0, DEVICE_CLASS_REQUEST_OUT,
                              0x00, 0x00, buf, 0x07, 100);

        if (ret < 0) {
            BT_ERR("%s error1(%d)\n", __FUNCTION__, ret);
            return ret;
        }
    }

    // Get response
    {
        int ret = 0;
        int actual_length;
        ret = usb_interrupt_msg(data->udev, usb_rcvintpipe(data->udev, 1),
                                result_buf, 64, &actual_length, 2000);

        if (ret < 0) {
            BT_ERR("%s error2(%d)\n", __FUNCTION__, ret);
            return ret;
        }
        else {
            printk("len = %d\n", actual_length);
            printk("Current flash patch version : %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X \n",
                        result_buf[0],
                        result_buf[1],
                        result_buf[2],
                        result_buf[3],
                        result_buf[4],
                        result_buf[5],
                        result_buf[6],
                        result_buf[7],
                        result_buf[8],
                        result_buf[9]);
        }
        // sample result log
        // Current rom patch version : 0E 08 01 D1 FC 00 11 09 14 20
        year1 = (result_buf[9]/16)*10 + result_buf[9]%16;
        year2 = (result_buf[8]/16)*10 + result_buf[8]%16;
        month = (result_buf[7]/16)*10 + result_buf[7]%16;
        day   = (result_buf[6]/16)*10 + result_buf[6]%16;

        printk("year1:%d, year2:%d, month:%d, day:%d\n", year1, year2, month, day);
        data->flash_patch_version = year1*1000000 + year2*10000 + month*100 + day;

        if ( result_buf[9] == 0xff && result_buf[8] == 0xff && result_buf[7] == 0xff && result_buf[6] == 0xff ) {
            data->patch_version_all_ff = 1;
            printk("flash_patch_version all ff: %d\n", data->patch_version_all_ff);
        }
        else {
            data->patch_version_all_ff = 0;

            if (year1 != 20 || year2 > 25 || year2 < 10) {
                printk("flash patch version'year is not between 2010 to 2025\n");
                return -1;
            }

            if (data->flash_patch_version != data->driver_patch_version)
                data->patch_version_all_ff = 2;
        }

        printk("flash_patch_version not all ff: %d\n", data->patch_version_all_ff);
        printk("btmtk_usb_get_flash_patch_version calculated flash_patch_version is : %d\n", data->flash_patch_version);
    }
    return 0;
}
/*
function name: btmtk_usb_load_data_to_flash
parameters: data, phase, dest, sent_len, write_flash, data_part, cur_len

data:    btmtk_usb_data structure
phase 0: start, 1: continue, 2: end
dest, 0: bootloader, 1: patch, 2: version, 3: loader flag
sent_len: current sent length
cur_len: current length of image
write_flash: 0 do not write flash, 1 write flash
data_part: 0, 1, 2, 3 means first second third fourth part

struct patch_down_info
{
    u8 phase;
    u8 dest;
    u8 cur_len;
    u8 sent_len;
    u8 single_len;
    u8 write_flash;
    u8 data_part;
};
*/
// static int btmtk_usb_load_data_to_flash(struct btmtk_usb_data *data, struct patch_down_info)
static int btmtk_usb_load_data_to_flash(struct btmtk_usb_data *data, struct patch_down_info patch_info) {
    static u16 sent_len = 0;
    u16 back_len = 0;
    static u16 sent_time = 0;

    if (patch_info.phase == 0 && patch_info.write_flash == 0) {
        sent_len = 0;
        sent_time = 0;
        printk("btmtk_usb_load_data_to_flash begin and reset data\n");
    }

    {
        int ret = 0;
        char buf[256] = {0};

        /* prepare HCI header */
        buf[0] = 0xEE;
        buf[1] = 0xFC;
        buf[2] = patch_info.single_len + 5;
        buf[3] = patch_info.dest;

        buf[4] = patch_info.write_flash;
        buf[5] = patch_info.data_part;
        buf[6] = patch_info.phase;

        buf[7] = patch_info.single_len;
        memcpy(&buf[8], data->rom_patch + PATCH_INFO_SIZE + patch_info.cur_len, patch_info.single_len);

        ret = usb_control_msg(data->udev, usb_sndctrlpipe(data->udev, 0), 0x0, DEVICE_CLASS_REQUEST_OUT,
                              0x00, 0x00, buf, patch_info.single_len + 8, 100);
        sent_time++;

        if (ret < 0) {
            BT_ERR("%s error1(%d), sent_time(%d)\n", __FUNCTION__, ret, sent_time);
            sent_len = 0;
            return ret;
        }
        sent_len = sent_len + patch_info.single_len;    // record the sent length
    }
    {
        // TODO: do I need to wait for a moment?
        // printk("btmtk_usb_load_data_to_flash, write_flash == 1\n");
        int ret = 0;
        char buf[64] = {0};
        int actual_length;
        ret = usb_interrupt_msg(data->udev, usb_rcvintpipe(data->udev, 1),
                                buf, 64, &actual_length, 4000);

        if (ret < 0) {
            BT_ERR("%s error2(%d), sent count(%d)\n", __FUNCTION__, ret, sent_time);
            sent_len = 0;
            return ret;
        }

        back_len = buf[6] + buf[7]*256;    // is this like this
        if ( (patch_info.write_flash == 1) && (buf[0] == 0x0e) && (buf[1] == 0x06) && (buf[2] == 0x01) && (buf[3] == 0xee) && (buf[4] == 0xfc) && (buf[5] == 0x00) && (sent_len == back_len) ) {
            // printk("load data to flash success OK\n");
            sent_len = 0;
        }
        else {
            if ( (patch_info.write_flash != 0) || (buf[0] != 0x0e) || (buf[1] != 0x04) || (buf[2] != 0x01) || (buf[3] != 0xee) || (buf[4] != 0xfc) || (buf[5] != 0x00) ) {
                printk("load data to flash: NG response, error, sent_time(%d)\n", sent_time);
                printk("btmtk_usb_load_data_to_flash event actual length %d, 0x%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X \n", \
                actual_length, buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7], buf[8], buf[9]);
                printk("phase:%d, dest:%d, cur_len:%d, sent_len:%d, single_len:%d, write_patch:%d, data_part:%d\n", \
                patch_info.phase, patch_info.dest, patch_info.cur_len, patch_info.sent_len, patch_info.single_len, \
                patch_info.write_flash, patch_info.data_part);
                printk("btmtk_usb_load_data_to_flash sent_len = %d, back_len = %d, write_flash = %d\n", sent_len, back_len, patch_info.write_flash);
                sent_len = 0;
                return -1;
            }
        }
    }
    return 0;
}

static int btmtk_usb_write_patch_version(struct btmtk_usb_data *data) {
    printk("btmtk_usb_write_patch_version++++\n");

    btmtk_usb_read_dummy_event(data->udev);    // wenhui add to read dummy hci event
    {
        int ret = 0;
        char buf[24] = {0};
        buf[0] = 0xEE;
        buf[1] = 0xFC;
        buf[2] = 0x09;
        buf[3] = 0x02;
        buf[4] = 0x01;
        buf[5] = 0x00;
        buf[6] = 0x00;
        buf[7] = 0x04;
        buf[8] = data->driver_version[3];
        buf[9] = data->driver_version[2];
        buf[10] = data->driver_version[1];
        buf[11] = data->driver_version[0];

        printk("buf8~11: %02X, %02X, %02X, %02X\n", buf[8], buf[9], buf[10], buf[11]);
        ret = usb_control_msg(data->udev, usb_sndctrlpipe(data->udev, 0), 0x0, DEVICE_CLASS_REQUEST_OUT,
                              0x00, 0x00, buf, 12, 100);

        if (ret < 0) {
            BT_ERR("%s error1(%d)\n", __FUNCTION__, ret);
            return ret;
        }
    }

    // Get response of flash version request
    {
        int ret = 0;
        char buf[64] = {0};
        int actual_length;
        ret = usb_interrupt_msg(data->udev, usb_rcvintpipe(data->udev, 1),
                                buf, 64, &actual_length, 2000);

        if (ret < 0) {
            BT_ERR("%s error2(%d)\n", __FUNCTION__, ret);
            return ret;
        }
        printk("btmtk_usb_write_patch_version, event: 0x%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X \n",
                    buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7], buf[8], buf[9]);

        if ( (buf[0] == 0x0e) && (buf[1] == 0x06) && (buf[2] == 0x01) && (buf[3] == 0xee) && (buf[4] == 0xfc) && (buf[5] == 0x00) && (buf[6] == 0x04) && (buf[7] == 0x00) ) {
            printk("write patch version success!\n");
        }
        else {
            printk("write patch version get NG response!\n");
        }
    }

    printk("btmtk_usb_write_patch_version----\n");
    return 0;
}

static int btmtk_usb_read_patch_crc(struct usb_device *udev, u32 patch_len) {
    char result_buf[64] = {0};
    char patchLen[5] = {0};

    btmtk_usb_read_dummy_event(udev);    // wenhui add to read dummy hci event
    {
        int ret = 0;
        char buf[12] = {0};
        patchLen[0] = patch_len&0xFF;
        patchLen[1] = (patch_len>>8)&0xFF;
        patchLen[2] = (patch_len>>16)&0xFF;
        patchLen[3] = (patch_len>>24)&0xFF;

        printk("patch_len: 0x%08X\n", patch_len);

        printk("patchLen 0~3: %02X, %02X, %02X, %02X\n", patchLen[0], patchLen[1], patchLen[2], patchLen[3]);

        buf[0] = 0x00;
        buf[1] = 0xfd;
        buf[2] = 0x08;
        buf[3] = 0x00;
        buf[4] = 0x40;
        buf[5] = 0x50;
        buf[6] = 0x00;
        buf[7] = patchLen[0];
        buf[8] = patchLen[1];
        buf[9] = patchLen[2];
        buf[10] = patchLen[3];

        ret = usb_control_msg(udev, usb_sndctrlpipe(udev, 0), 0x0, DEVICE_CLASS_REQUEST_OUT,
                              0x00, 0x00, buf, 11, 100);

        if (ret < 0) {
            BT_ERR("%s error1(%d)\n", __FUNCTION__, ret);
            return ret;
        }
    }

    // Get response of Crc Request
    {
        int ret = 0;
        int actual_length;
        ret = usb_interrupt_msg(udev, usb_rcvintpipe(udev, 1),
                                result_buf, 64, &actual_length, 5000);

        if (ret < 0) {
            BT_ERR("%s error2(%d)\n", __FUNCTION__, ret);
            return ret;
        }
        else {
            printk("btmtk_usb_read_patch_crc: len: %d, ret : %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X \n",
                        actual_length,
                        result_buf[0],
                        result_buf[1],
                        result_buf[2],
                        result_buf[3],
                        result_buf[4],
                        result_buf[5],
                        result_buf[6],
                        result_buf[7],
                        result_buf[8],
                        result_buf[9]);
        }
        if (result_buf[0] == 0x0e && result_buf[1] == 0x06 && result_buf[2] == 0x01 && result_buf[3] == 0 && result_buf[4] == 0xfd && result_buf[5] == 0x00 && result_buf[6] == rom_patch_crc[0] && result_buf[7] == rom_patch_crc[1]) {
            printk("flash crc is the same with driver crc\n");
            return 0;
        }
        else {
            printk("get NG result, return state: %d, crc: %02x, %02x\n", result_buf[5], result_buf[6], result_buf[7]);
            return -1;
        }
    }

    return 0;
}

static int btmtk_usb_load_patch_to_flash(struct btmtk_usb_data *data) {
    u32 patch_len = 0;
    int first_block = 1;

    int ret = 0;
    int crc_ret = 0;
    int i = 0 , count = 0 , tail = 0;
    struct patch_down_info patch_info;

    patch_info.phase = 0;
    patch_info.dest = 1;  // flash set to 1
    patch_info.cur_len = 0;
    patch_info.sent_len = 0;
    patch_info.write_flash = 0;
    patch_info.data_part = 0;
    patch_info.single_len = 0;
    patch_info.cur_len = 0x00;
    patch_len = data->rom_patch_len - PATCH_INFO_SIZE;
    printk("btmtk_usb_load_patch_to_flash+++++++\n");
    while (1) {
        s32 sent_len_max = FLASH_MAX_UNIT;
        patch_info.sent_len = (patch_len - patch_info.cur_len) >= sent_len_max ? sent_len_max : (patch_len - patch_info.cur_len);

        if (patch_info.sent_len > 0) {
            if (first_block == 1) {
                if (patch_info.sent_len < sent_len_max)
                    patch_info.phase = FLASH_PATCH_PHASE3;
                else
                    patch_info.phase = FLASH_PATCH_PHASE1;
                first_block = 0;
            }
            else if (patch_info.sent_len == sent_len_max) {
                patch_info.phase = FLASH_PATCH_PHASE2;
            }
            else {
                patch_info.phase = FLASH_PATCH_PHASE3;
            }

            i = 0;
            // printk("download phase = %d\n", patch_info.phase);

            count = patch_info.sent_len/FLASH_SINGLE_UNIT;
            tail = 0;
            if ((patch_info.phase == FLASH_PATCH_PHASE1) || (patch_info.phase == FLASH_PATCH_PHASE2)) {
                // directly download FLASH_MAX_UNIT bytes to flash
                for (i = 0; i < (FLASH_MAX_UNIT/FLASH_SINGLE_UNIT); i++) {
                    if (i == (FLASH_MAX_UNIT/FLASH_SINGLE_UNIT-1))
                        patch_info.write_flash = 1;
                    else
                        patch_info.write_flash = 0;

                    patch_info.data_part = i;
                    patch_info.single_len = 128;

                    ret = btmtk_usb_load_data_to_flash(data, patch_info);

                    if (ret != 0) {
                        printk("load_data_to_flash phase:%d  ret:%d\n", patch_info.phase, ret);
                        return -1;    // return value
                    }
                    patch_info.cur_len += 128;
                }
            }
            else if (patch_info.phase == FLASH_PATCH_PHASE3) {
                if (patch_len < FLASH_MAX_UNIT)    // if patch_len less than FLASH_MAX_UNIT, then, set to phase1 to trigger erase flash
                    patch_info.phase = FLASH_PATCH_PHASE1;

                count = patch_info.sent_len/FLASH_SINGLE_UNIT;
                tail  = patch_info.sent_len%FLASH_SINGLE_UNIT;
                // printk("phase: %d, count:%d, tail:%d\n", patch_info.phase, count, tail);

                for (i = 0; i < count; i++) {
                    patch_info.single_len = 128;
                    patch_info.data_part = i;

                    if ((i == count-1) && (tail == 0)) {
                        patch_info.write_flash = 1;

                        ret = btmtk_usb_load_data_to_flash(data, patch_info);

                        if (ret != 0) {
                            printk("load_data_to_flash phase:%d  ret:%d\n", patch_info.phase, ret);
                            return -1;    // return value
                        }
                    }
                    else {
                        patch_info.write_flash = 0;

                        ret = btmtk_usb_load_data_to_flash(data, patch_info);

                        if (ret != 0) {
                            printk("load_data_to_flash phase:%d  ret:%d\n", patch_info.phase, ret);
                            return -1;    // return value
                        }
                    }
                    patch_info.cur_len += 128;
                }

                if (i == 0) {    // means sent_len < 128
                    patch_info.data_part = i;
                    patch_info.write_flash = 1;
                    patch_info.single_len = patch_info.sent_len;

                    ret = btmtk_usb_load_data_to_flash(data, patch_info);

                    if (ret != 0) {
                        printk("load_data_to_flash phase:%d  ret:%d\n", patch_info.phase, ret);
                        return -1;    // return value
                    }

                    patch_info.cur_len += patch_info.sent_len;
                }
                if ((tail != 0) && (i != 0)) {    // write the tail part
                    patch_info.data_part = i;
                    patch_info.write_flash = 1;
                    patch_info.single_len = tail;
                    ret = btmtk_usb_load_data_to_flash(data, patch_info);

                    if (ret != 0) {
                        printk("load_data_to_flash phase:%d  ret:%d\n", patch_info.phase, ret);
                        return -1;    // return value
                    }
                    patch_info.cur_len += tail;
                }
            }
        }
        else {
            break;
        }
    }

    btmtk_usb_read_dummy_event_2(data->udev);

    crc_ret = btmtk_usb_read_patch_crc(data->udev, patch_len);
    if (BT_STATUS_SUCCESS != crc_ret)
        return BT_STATUS_FLASH_CRC_ERROR;

    btmtk_usb_write_patch_version(data);    // write patch version to flash
    printk("btmtk_usb_load_patch_to_flash-------\n");
    return 0;
}

static int btmtk_usb_send_set_tx_power_cmd(struct usb_device *udev) {
    printk("btmtk_usb_send_set_tx_power_cmd  7....\n");

    btmtk_usb_read_dummy_event(udev);    // wenhui add to read dummy hci event
    // btmtk_usb_send_set_tx_power_cmd
    {
        int ret = 0;
        char buf[10] = {0x79, 0xFC, 0x06, 0x07, 0x00, 0x80, 0x00, 0x07, 0x07, 0x00};

        ret = usb_control_msg(udev, usb_sndctrlpipe(udev, 0), 0x0, DEVICE_CLASS_REQUEST_OUT,
                              0x00, 0x00, buf, 0x09, 100);

        if (ret < 0) {
            BT_ERR("%s error1(%d)\n", __FUNCTION__, ret);
            return ret;
        }
    }

    // Get response of HCI reset
    {
        int ret = 0;
        char buf[64] = {0};
        int actual_length;
        ret = usb_interrupt_msg(udev, usb_rcvintpipe(udev, 1),
                                buf, 64, &actual_length, 2000);

        if (ret < 0) {
            BT_ERR("%s error2(%d)\n", __FUNCTION__, ret);
            return ret;
        }
        printk("btmtk_usb_send_set_tx_power_cmd 7 get response  0x%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X \n",
                    buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7], buf[8], buf[9]);
    }

    return 0;
}
#endif

static int btmtk_usb_send_hci_set_ce_cmd(struct usb_device *udev) {
    char result_buf[64] = {0};
#if __MTK_BT_BOOT_ON_FLASH__
    BT_DBG("send hci reset cmd");
    printk("btmtk_usb_send_hci_set_ce_cmd: read 0x41070c 0xd1, 0xFC, 0x04, 0x0c, 0x07, 0x41, 0x00\n");
    btmtk_usb_read_dummy_event(udev);    // wenhui add to read dummy hci event
#endif
    // Read 0x41070c
    {
        int ret = 0;
        char buf[8] = {0xd1, 0xFC, 0x04, 0x0c, 0x07, 0x41, 0x00};
        ret = usb_control_msg(udev, usb_sndctrlpipe(udev, 0), 0x0, DEVICE_CLASS_REQUEST_OUT,
                              0x00, 0x00, buf, 0x07, CONTROL_TIMEOUT_JIFFIES);

        if (ret < 0) {
            printk("%s error1(%d)\n", __FUNCTION__, ret);
            return ret;
        }
    }

    // Get response of read command
    {
        int ret = 0;
        int actual_length;
        ret = usb_interrupt_msg(udev, usb_rcvintpipe(udev, 1),
                                result_buf, 64, &actual_length, 2000);

        if (ret < 0) {
            printk("%s error2(%d)\n", __FUNCTION__, ret);
            return ret;
        }
        else {
            if ( result_buf[6] & 0x01 ) {
//                printk("%s, warning, 0x41070c[0] is 1!\n", __FUNCTION__);
            }
        }
    }

    // Write 0x41070c[0] to 1
    {
        int ret = 0;
        char buf[12] = {0xd0, 0xfc, 0x08, 0x0c, 0x07, 0x41, 0x00};
        buf[7] = result_buf[6]|0x01;
        buf[8] = result_buf[7];
        buf[9] = result_buf[8];
        buf[10] = result_buf[9];
        ret = usb_control_msg(udev, usb_sndctrlpipe(udev, 0), 0x0, DEVICE_CLASS_REQUEST_OUT,
                              0x00, 0x00, buf, 0x0b, CONTROL_TIMEOUT_JIFFIES);

        if (ret < 0) {
            printk("%s error1(%d)\n", __FUNCTION__, ret);
            return ret;
        }
    }
#if __MTK_BT_BOOT_ON_FLASH__
        // Get response of write cmd
        {
            int ret = 0;
            int actual_length;
            ret = usb_interrupt_msg(udev, usb_rcvintpipe(udev, 1),
                                    result_buf, 64, &actual_length, 2000);

            if (ret < 0) {
                BT_ERR("%s error2(%d)\n", __FUNCTION__, ret);
                return ret;
            }
            else {
                printk("btmtk_usb_send_hci_set_ce_cmd,    read response: 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x\n", \
                    result_buf[0], result_buf[1], result_buf[2], result_buf[3], result_buf[4], result_buf[5], result_buf[6], result_buf[7], result_buf[8], result_buf[9], result_buf[10]);
                if ( result_buf[6] & 0x01 ) {
                    printk("warning, 0x41020c[0] is 1!\n");
                }
            }
        }
#else
    printk("%s : OK\n", __FUNCTION__);
#endif
    return 0;
}

static int btmtk_usb_send_check_rom_patch_result_cmd(struct usb_device *udev) {
#if __MTK_BT_BOOT_ON_FLASH__
    btmtk_usb_read_dummy_event(udev);    // wenhui add to read dummy hci event
#endif
    {
        int ret = 0;
        unsigned char buf[8] = {0};
        buf[0] = 0xD1;
        buf[1] = 0xFC;
        buf[2] = 0x04;
        buf[3] = 0x00;
        buf[4] = 0xE2;
        buf[5] = 0x40;
        buf[6] = 0x00;


        ret = usb_control_msg(udev, usb_sndctrlpipe(udev, 0), 0x0, DEVICE_CLASS_REQUEST_OUT,
                              0x00, 0x00, buf, 0x07, CONTROL_TIMEOUT_JIFFIES);

        if (ret < 0) {
            printk("%s error1(%d)\n", __FUNCTION__, ret);
            return ret;
        }
    }

    {
        int ret = 0;
        unsigned char buf[64] = {0};
        int actual_length;
        ret = usb_interrupt_msg(udev, usb_rcvintpipe(udev, 1),
                                buf, 64, &actual_length, 2000);

        if (ret < 0) {
            printk("%s error2(%d)\n", __FUNCTION__, ret);
            return ret;
        }
        printk("%s : ", __FUNCTION__);

        if ( buf[6] == 0 && buf[7] == 0 && buf[8] == 0 && buf[9] == 0 ) {
            printk("NG\n");
        }
        else {
            printk("OK\n");
        }
    }

    return 0;
}

static int btmtk_usb_switch_iobase(struct btmtk_usb_data *data, int base) {
    int ret = 0;

    switch (base) {
        case SYSCTL:
            data->w_request = 0x42;
            data->r_request = 0x47;
            break;
        case WLAN:
            data->w_request = 0x02;
            data->r_request = 0x07;
            break;

        default:
            return -EINVAL;
    }

    return ret;
}

static void btmtk_usb_cap_init(struct btmtk_usb_data *data) {
    btmtk_usb_io_read32(data, 0x00, &data->chip_id);

    printk("%s : chip id = %x\n", __FUNCTION__, data->chip_id);

    if (is_mt7630(data) || is_mt7650(data)) {
        data->need_load_fw = 1;
        data->need_load_rom_patch = 0;
        data->fw_header_image = NULL;
        data->fw_bin_file_name = "mtk/mt7650.bin";
        data->fw_len = 0;
    }
    else if (is_mt7662T(data) || is_mt7632T(data)) {
        printk("btmtk:This is 7662T chip\n");
        data->need_load_fw = 0;
        data->need_load_rom_patch = 1;
        if (LOAD_CODE_METHOD == HEADER_METHOD) {
            data->rom_patch_header_image = mt7662t_rom_patch;
            data->rom_patch_len = sizeof(mt7662t_rom_patch);
            data->rom_patch_offset = 0xBC000;
        }
        else {
            data->rom_patch_bin_file_name = kmalloc(32, GFP_KERNEL);
            if (!data->rom_patch_bin_file_name) {
                printk("%s: Can't allocate memory (32)\n", __FUNCTION__);
                return;
            }
            memset(data->rom_patch_bin_file_name, 0, 32);

            if ( (data->chip_id & 0xf) < 0x2 )
                memcpy(data->rom_patch_bin_file_name, "mt7662t_patch_e1_hdr.bin", 24);
            else
                memcpy(data->rom_patch_bin_file_name, "mt7662t_patch_e3_hdr.bin", 24);

            data->rom_patch_offset = 0xBC000;
            data->rom_patch_len = 0;
        }
    }
    else if (is_mt7632(data) || is_mt7662(data)) {
        printk("btmtk:This is 7662U chip\n");
        data->need_load_fw = 0;
        data->need_load_rom_patch = 1;
        if (LOAD_CODE_METHOD == HEADER_METHOD) {
            data->rom_patch_header_image = mt7662_rom_patch;
            data->rom_patch_len = sizeof(mt7662_rom_patch);
            data->rom_patch_offset = 0x90000;
        }
        else {
            data->rom_patch_bin_file_name = kmalloc(32, GFP_KERNEL);
            if ( !data->rom_patch_bin_file_name ) {
                printk("%s: Can't allocate memory (32)\n", __FUNCTION__);
                return;
            }
            memset(data->rom_patch_bin_file_name, 0, 32);

            if ( (data->chip_id & 0xf) < 0x2 )
                memcpy(data->rom_patch_bin_file_name, "mt7662_patch_e1_hdr.bin", 23);
            else
                memcpy(data->rom_patch_bin_file_name, "mt7662_patch_e3_hdr.bin", 23);

            data->rom_patch_offset = 0x90000;
            data->rom_patch_len = 0;
        }
    }
    else {
        printk("%s : unknow chip(%x)\n", __FUNCTION__, data->chip_id);
    }
}

u16 checksume16(u8 *pData, int len) {
    int sum = 0;

    while (len > 1) {
        sum += *((u16*)pData);

        pData = pData + 2;

        if (sum & 0x80000000)
            sum = (sum & 0xFFFF) + (sum >> 16);

        len -= 2;
    }

    if (len)
        sum += *((u8*)pData);

    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    return ~sum;
}

static int btmtk_usb_chk_crc(struct btmtk_usb_data *data, u32 checksum_len) {
    int ret = 0;
    struct usb_device *udev = data->udev;

    BT_DBG("%s", __FUNCTION__);

    memmove(data->io_buf, &data->rom_patch_offset, 4);
    memmove(&data->io_buf[4], &checksum_len, 4);

    ret = usb_control_msg(udev, usb_sndctrlpipe(udev, 0), 0x1, DEVICE_VENDOR_REQUEST_OUT,
                          0x20, 0x00, data->io_buf, 8,
                          CONTROL_TIMEOUT_JIFFIES);

    if (ret < 0) {
        printk("%s error(%d)\n", __FUNCTION__, ret);
    }

    return ret;
}

static u16 btmtk_usb_get_crc(struct btmtk_usb_data *data) {
    int ret = 0;
    struct usb_device *udev = data->udev;
    u16 crc, count = 0;

    BT_DBG("%s", __FUNCTION__);

    while (1) {
        ret = usb_control_msg(udev, usb_rcvctrlpipe(udev, 0), 0x01, DEVICE_VENDOR_REQUEST_IN,
                         0x21, 0x00, data->io_buf, 2,
                          CONTROL_TIMEOUT_JIFFIES);

        if (ret < 0) {
            crc = 0xFFFF;
            printk("%s error(%d)\n", __FUNCTION__, ret);
        }

        memmove(&crc, data->io_buf, 2);

        crc = le16_to_cpu(crc);

        if (crc != 0xFFFF) {
            break;
        }

        mdelay(100);

        if (count++ > 100) {
            printk("%s : Query CRC over %d times\n", __FUNCTION__, count);
            break;
        }
    }

    return crc;
}

static int btmtk_usb_reset_wmt(struct btmtk_usb_data *data) {
    int ret = 0;

    /* reset command */
    u8 cmd[9] = {0x6F, 0xFC, 0x05, 0x01, 0x07, 0x01, 0x00, 0x04};

#if __MTK_BT_BOOT_ON_FLASH__
    btmtk_usb_read_dummy_event(data->udev);    // wenhui add to read dummy hci event
#endif
    memmove(data->io_buf, cmd, 8);

    ret = usb_control_msg(data->udev, usb_sndctrlpipe(data->udev, 0), 0x01,
                                DEVICE_CLASS_REQUEST_OUT, 0x30, 0x00, data->io_buf, 8, CONTROL_TIMEOUT_JIFFIES);

    if (ret < 0) {
        printk("%s:Err1(%d)\n", __FUNCTION__, ret);
        return ret;
    }

    mdelay(20);

    ret = usb_control_msg(data->udev, usb_rcvctrlpipe(data->udev, 0), 0x01,
                            DEVICE_VENDOR_REQUEST_IN, 0x30, 0x00, data->io_buf, 7, CONTROL_TIMEOUT_JIFFIES);

    if (ret < 0) {
        printk("%s Err2(%d)\n", __FUNCTION__, ret);
    }

    if ( data->io_buf[0] == 0xe4 &&
         data->io_buf[1] == 0x05 &&
         data->io_buf[2] == 0x02 &&
         data->io_buf[3] == 0x07 &&
         data->io_buf[4] == 0x01 &&
         data->io_buf[5] == 0x00 &&
         data->io_buf[6] == 0x00 ) {
        printk("%s : OK\n", __FUNCTION__);
    }
    else {
        printk("%s : NG\n", __FUNCTION__);
    }


    return ret;
}

static u16 btmtk_usb_get_rom_patch_result(struct btmtk_usb_data *data) {
    int ret = 0;

    ret = usb_control_msg(data->udev, usb_rcvctrlpipe(data->udev, 0), 0x01,
                            DEVICE_VENDOR_REQUEST_IN, 0x30, 0x00, data->io_buf, 7, CONTROL_TIMEOUT_JIFFIES);

    if (ret < 0) {
        printk("%s error(%d)\n", __FUNCTION__, ret);
    }

    if ( data->io_buf[0] == 0xe4 &&
         data->io_buf[1] == 0x05 &&
         data->io_buf[2] == 0x02 &&
         data->io_buf[3] == 0x01 &&
         data->io_buf[4] == 0x01 &&
         data->io_buf[5] == 0x00 &&
         data->io_buf[6] == 0x00 ) {
        printk("%s : OK\n", __FUNCTION__);
    }
    else {
        printk("%s : NG\n", __FUNCTION__);
    }

    return ret;
}

static void load_code_from_bin(unsigned char **image, char *bin_name, struct device *dev, u32 *code_len) {
    const struct firmware *fw_entry;
    int err;
    int retry_counter = 0;

    if ( dev == NULL ) {
        printk("%s : dev is NULL !\n", __FUNCTION__);
        return;
    }

RETRY_REQUEST_FIRMWARE:
    err = request_firmware(&fw_entry, bin_name, dev);
    if ( err == -EAGAIN ) {
        printk("%s : request_firmware return -EAGAIN!\n", __FUNCTION__);
        retry_counter++;
        if ( retry_counter > 100 ) {
            printk("%s : give up!\n", __FUNCTION__);
        }
        else {
            printk("%s : retry it! (%d)\n", __FUNCTION__, retry_counter);
            msleep(100);
            goto RETRY_REQUEST_FIRMWARE;
        }
    }

    if (err) {
        *image = NULL;
        printk("%s : Failed to load %s, %d.\n", __FUNCTION__, bin_name, err);
        return;
    }

    *image = vmalloc(fw_entry->size);
    if (*image == NULL) {
        printk("Fail to alloc patch image!\n");
        return;
    }
    memcpy(*image, fw_entry->data, fw_entry->size);
    *code_len = fw_entry->size;

    release_firmware(fw_entry);
}

static void load_rom_patch_complete(struct urb *urb) {
    struct completion *sent_to_mcu_done = (struct completion *)urb->context;
    complete(sent_to_mcu_done);
}

#if __MTK_BT_BOOT_ON_FLASH__
static int btmtk_usb_load_patch_to_ram(struct btmtk_usb_data *data) {
    s32 sent_len;
    int ret = 0, total_checksum = 0;
    struct urb *urb;
    u32 patch_len = 0;
    u32 cur_len = 0;
    dma_addr_t data_dma;
    struct completion sent_to_mcu_done;
    int first_block = 1;
    unsigned char phase;
    void *buf;
    char *pos;
    unsigned int pipe = usb_sndbulkpipe(data->udev,
                                        data->bulk_tx_ep->bEndpointAddress);

    urb = usb_alloc_urb(0, GFP_ATOMIC);

    if (!urb) {
        ret = -ENOMEM;
        printk("btmtk_usb_load_patch_to_ram, go to error0\n");
        goto error0;
    }

    buf = usb_alloc_coherent(data->udev, UPLOAD_PATCH_UNIT, GFP_ATOMIC, &data_dma);

    if (!buf) {
        ret = -ENOMEM;
        printk("btmtk_usb_load_patch_to_ram, go to error1\n");
        goto error1;
    }

    pos = buf;
    init_completion(&sent_to_mcu_done);

    cur_len = 0x00;
    patch_len = data->rom_patch_len - PATCH_INFO_SIZE;
    /* loading rom patch */
    while (1) {
        s32 sent_len_max = UPLOAD_PATCH_UNIT - PATCH_HEADER_SIZE;
        sent_len = (patch_len - cur_len) >= sent_len_max ? sent_len_max : (patch_len - cur_len);

        printk("patch_len = %d\n", patch_len);
        printk("cur_len = %d\n", cur_len);
        printk("sent_len = %d\n", sent_len);

        if (sent_len > 0) {
            if (first_block == 1) {
                if (sent_len < sent_len_max)
                    phase = PATCH_PHASE3;
                else
                    phase = PATCH_PHASE1;
                first_block = 0;
            }
            else if (sent_len == sent_len_max) {
                phase = PATCH_PHASE2;
            }
            else {
                phase = PATCH_PHASE3;
            }

            /* prepare HCI header */
            pos[0] = 0x6F;
            pos[1] = 0xFC;
            pos[2] = (sent_len + 5) & 0xFF;
            pos[3] = ((sent_len + 5) >> 8) & 0xFF;
            /* prepare WMT header */
            pos[4] = 0x01;
            pos[5] = 0x01;
            pos[6] = (sent_len + 1) & 0xFF;
            pos[7] = ((sent_len + 1) >> 8) & 0xFF;

            pos[8] = phase;

            memcpy(&pos[9], data->rom_patch + PATCH_INFO_SIZE + cur_len, sent_len);
            printk("sent_len + PATCH_HEADER_SIZE = %d, phase = %d\n", sent_len + PATCH_HEADER_SIZE, phase);

            usb_fill_bulk_urb(urb,
                              data->udev,
                              pipe,
                              buf,
                              sent_len + PATCH_HEADER_SIZE,
                              load_rom_patch_complete,
                              &sent_to_mcu_done);
            urb->transfer_dma = data_dma;
            urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;

            ret = usb_submit_urb(urb, GFP_ATOMIC);
            if (ret) {
                printk("btmtk_usb_load_patch_to_ram, submit urb fail, go to error2\n");
                goto error2;
            }
            if (!wait_for_completion_timeout(&sent_to_mcu_done, msecs_to_jiffies(1000))) {
                usb_kill_urb(urb);
                printk("upload rom_patch timeout\n");
                goto error2;
            }

            mdelay(1);
            cur_len += sent_len;
        }
        else {
            break;
        }
    }

    mdelay(20);
    ret = btmtk_usb_get_rom_patch_result(data);

    mdelay(20);
    // Send Checksum request
    total_checksum = checksume16(data->rom_patch + PATCH_INFO_SIZE, patch_len);
    btmtk_usb_chk_crc(data, patch_len);

    mdelay(20);

    if (total_checksum != btmtk_usb_get_crc(data)) {
        printk("checksum fail!, local(0x%x) <> fw(0x%x)\n", total_checksum, btmtk_usb_get_crc(data));
        ret = -1;
        printk("btmtk_usb_load_patch_to_ram, go to error2\n");
        goto error2;
    }

    mdelay(20);
    // send check rom patch result request
    btmtk_usb_send_check_rom_patch_result_cmd(data->udev);
    mdelay(20);
    // CHIP_RESET
    ret = btmtk_usb_reset_wmt(data);
    mdelay(20);
    // BT_RESET
    btmtk_usb_send_hci_reset_cmd(data->udev);

error2:
    usb_free_coherent(data->udev, UPLOAD_PATCH_UNIT, buf, data_dma);
error1:
    usb_free_urb(urb);
error0:
    btmtk_usb_io_write32(data, SEMAPHORE_03, 0x1);
    printk("btmtk_usb_load_patch_to_ram end\n");
    return ret;
}

static int btmtk_usb_load_rom_patch(struct btmtk_usb_data *data) {
    u32 loop = 0;
    u32 value;
    int ret = 0, i = 0;
    u8 boot_on_flag = 0, retry_count = 2, driver_version[8] = {0};
    char *tmp_str;

    //printk("btmtk_usb_load_rom_patch begin+++\n");

load_patch_protect:
    btmtk_usb_switch_iobase(data, WLAN);
    btmtk_usb_io_read32(data, SEMAPHORE_03, &value);
    loop++;

    if ( ( (value & 0x01) == 0x00) && (loop < 600) ) {
        mdelay(1);
        goto load_patch_protect;
    }

    btmtk_usb_switch_iobase(data, SYSCTL);

    btmtk_usb_io_write32(data, 0x1c, 0x30);

    btmtk_usb_switch_iobase(data, WLAN);
    /* check ROM patch if upgrade */
    if ( (MT_REV_GTE(data, mt7662, REV_MT76x2E3)) ||
         (MT_REV_GTE(data, mt7632, REV_MT76x2E3))) {
        btmtk_usb_io_read32(data, CLOCK_CTL, &value);
        if ((value & 0x01) != 0x01) {
            printk("btmtk_usb_load_rom_patch : no patch on flash\n");
           // btmtk_usb_send_hci_reset_cmd(data->udev);
            boot_on_flag = 1;    // flash broken or boot on rom
        }
    }
    else {
        btmtk_usb_io_read32(data, COM_REG0, &value);
        if ((value & 0x02) != 0x02) {
            printk("btmtk_usb_load_rom_patch : no patch on flash \n");
           // btmtk_usb_send_hci_reset_cmd(data->udev);
            boot_on_flag = 1;    // flash broken or boot on rom
        }
    }

    if (LOAD_CODE_METHOD == BIN_FILE_METHOD) {
        load_code_from_bin(&data->rom_patch, data->rom_patch_bin_file_name, &data->udev->dev, &data->rom_patch_len);
        printk("LOAD_CODE_METHOD : BIN_FILE_METHOD\n");
    }
    else {
        data->rom_patch = data->rom_patch_header_image;
        printk("LOAD_CODE_METHOD : HEADER_METHOD\n");
    }

    if (!data->rom_patch) {
        if (LOAD_CODE_METHOD == BIN_FILE_METHOD) {
            BT_ERR("%s:please assign a rom patch(/etc/firmware/%s)\n",
                __FUNCTION__, data->rom_patch_bin_file_name);
        }
        else {
            BT_ERR("%s:please assign a rom patch\n", __FUNCTION__);
        }
        printk("goto load patch end\n");
        goto load_patch_end;
    }

    msleep(200);

    tmp_str = data->rom_patch;
/*    printk("FW Version = %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",
            tmp_str[0], tmp_str[1], tmp_str[2], tmp_str[3],
            tmp_str[4], tmp_str[5], tmp_str[6], tmp_str[7],
            tmp_str[8], tmp_str[9], tmp_str[10], tmp_str[11],
            tmp_str[12], tmp_str[13], tmp_str[14], tmp_str[15]);

    printk("build Time = %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",
            tmp_str[0], tmp_str[1], tmp_str[2], tmp_str[3],
            tmp_str[4], tmp_str[5], tmp_str[6], tmp_str[7],
            tmp_str[8], tmp_str[9], tmp_str[10], tmp_str[11],
            tmp_str[12], tmp_str[13], tmp_str[14], tmp_str[15]);
*/

    memset(rom_patch_version, 0, sizeof(rom_patch_version));
    memcpy(rom_patch_version, tmp_str, 15);

    tmp_str = data->rom_patch + 16;
    //printk("platform = %c%c%c%c\n", tmp_str[0], tmp_str[1], tmp_str[2], tmp_str[3]);

    tmp_str = data->rom_patch + 20;
        //printk("HW/SW version = %c%c%c%c\n", tmp_str[0], tmp_str[1], tmp_str[2], tmp_str[3]);

    tmp_str = data->rom_patch + 24;
    //printk("Patch version = %c%c%c%c\n", tmp_str[0], tmp_str[1], tmp_str[2], tmp_str[3]);

    memset(rom_patch_crc, 0, sizeof(rom_patch_crc));

    tmp_str = data->rom_patch + 0x1c;
    rom_patch_crc[0] = tmp_str[0];
    rom_patch_crc[1] = tmp_str[1];

    //printk("\n driver patch crc: %02x, %02x", rom_patch_crc[0], rom_patch_crc[1]);

    // get patch info
    data->driver_patch_version = 0;
    for (i = 0; i < 8; i++) {
        driver_version[i] = rom_patch_version[i]-0x30;
    }

    printk("\n");
    for (i = 0; i < 4; i++) {
        data->driver_version[i] = (rom_patch_version[2*i]-0x30)*16 + (rom_patch_version[2*i+1]-0x30);
        printk("data->driver_version[%d]: %02X\n", i, data->driver_version[i]);
    }
    data->driver_patch_version =  driver_version[0]*10000000 + driver_version[1]*1000000 + driver_version[2]*100000 + driver_version[3]*10000 + \
         driver_version[4]*1000 + driver_version[5]*100 + driver_version[6]*10 + driver_version[7];

    //printk("probe, driver_patch_version: %d\n", data->driver_patch_version);

    // step1: send hci_reset first
    btmtk_usb_send_hci_reset_cmd(data->udev);

    // step2: prepare patch info & flash patch info & set to btmtk_usb_data struct
    btmtk_usb_get_flash_patch_version(data);

    if (boot_on_flag == 1 || data->patch_version_all_ff == 1) {
        ret = btmtk_usb_load_patch_to_ram(data);
        if (ret < 0) {
            printk("err:btmtk_usb_load_patch_to_ram return %d\n", ret);
        }
        //printk("+++begin receive dummy event+++\n");
        btmtk_usb_read_dummy_event(data->udev);
        //printk("+++end receive dummy event----\n");
    }

    if (data->patch_version_all_ff != 0) {
        retry_count = 2;
        ret = btmtk_usb_load_patch_to_flash(data);
        // need to always load patch or not?
        while ((ret != 0) && (retry_count > 0)) {
            ret = btmtk_usb_load_patch_to_flash(data);
            retry_count = retry_count - 1;
        }
        if (ret!= 0)
            printk("btmtk_usb_load_patch_to_flash fail count:%d  , ret:%d \n", 3-retry_count, ret);
        else
            printk("btmtk_usb_load_patch_to_flash success count:%d  , ret:%d \n", 3-retry_count, ret);
    }
load_patch_end:

    mdelay(20);
    // hci_reset
    btmtk_usb_send_hci_reset_cmd(data->udev);

    // for WoBLE/WoW low power
    btmtk_usb_send_hci_set_ce_cmd(data->udev);

    // btmtk_usb_send_enable_low_power_cmd(data->udev);  // send FC41

    btmtk_usb_send_disable_low_power_cmd(data->udev);    // send FC40

    mdelay(20);
    btmtk_usb_send_set_tx_power_cmd(data->udev);    // default set to 7

    btmtk_usb_io_write32(data, SEMAPHORE_03, 0x1);
    printk("btmtk_usb_load_rom_patch 2@end---\n");
    return ret;
}
#else
static int btmtk_usb_load_rom_patch(struct btmtk_usb_data *data) {
    u32 loop = 0;
    u32 value;
    s32 sent_len;
    int ret = 0, total_checksum = 0;
    struct urb *urb;
    u32 patch_len = 0;
    u32 cur_len = 0;
    dma_addr_t data_dma;
    struct completion sent_to_mcu_done;
    int first_block = 1;
    int rom_patch_loaded = 0;
    unsigned char phase;
    void *buf;
    char *pos;
    char *tmp_str;
    unsigned int pipe = usb_sndbulkpipe(data->udev,
                                        data->bulk_tx_ep->bEndpointAddress);

    printk("%s begin\n", __FUNCTION__);
load_patch_protect:
    btmtk_usb_switch_iobase(data, WLAN);
    btmtk_usb_io_read32(data, SEMAPHORE_03, &value);
    loop++;

    if ( (value & 0x01) == 0x00 ) {
        if (loop < 1000) {
            mdelay(1);
            goto load_patch_protect;
        }
        else {
            printk("%s ERR! Can't get semaphore! Continue \n", __FUNCTION__);
        }
    }

    btmtk_usb_switch_iobase(data, SYSCTL);

    btmtk_usb_io_write32(data, 0x1c, 0x30);

    btmtk_usb_switch_iobase(data, WLAN);

    /* check ROM patch if upgrade */
    if ( (MT_REV_GTE(data, mt7662, REV_MT76x2E3)) ||
         (MT_REV_GTE(data, mt7632, REV_MT76x2E3))) {
        btmtk_usb_io_read32(data, CLOCK_CTL, &value);
        if ((value & 0x01) == 0x01) {
            rom_patch_loaded = 1;
        }
    }
    else {
        btmtk_usb_io_read32(data, COM_REG0, &value);
        if ((value & 0x02) == 0x02) {
            rom_patch_loaded = 1;
        }
    }

    // No need to load rom patch.
    if ( rom_patch_loaded == 1 ) {
        if ( rom_patch_version[0] == 0 ) {
            if (LOAD_CODE_METHOD == BIN_FILE_METHOD)  {
                load_code_from_bin(&data->rom_patch, data->rom_patch_bin_file_name, &data->udev->dev, &data->rom_patch_len);
                printk("%s : BIN_FILE_METHOD\n", __FUNCTION__);
            }
            else {
                data->rom_patch = data->rom_patch_header_image;
                printk("%s : HEADER_METHOD\n", __FUNCTION__);
            }
            memset(rom_patch_version, 0, sizeof(rom_patch_version));
            memcpy(rom_patch_version, data->rom_patch, 15);
            printk("%s : FW version = %s\n", __FUNCTION__, rom_patch_version);
        }
        else {
            printk("%s : FW version = %s\n", __FUNCTION__, rom_patch_version);
        }

        printk("%s : no need to load rom patch\n", __FUNCTION__);

#if SUPPORT_CHECK_WAKE_UP_REASON_DECIDE_SEND_HCI_RESET
        {
            extern unsigned int PDWNC_ReadWakeupReason(void);
            unsigned int WakeupReason = PDWNC_ReadWakeupReason();
            if ( WakeupReason == 2 ||
                 WakeupReason == 7 ||
                 WakeupReason == 30 ||
                 WakeupReason == 31 ||
                 WakeupReason == 33 ||
                 WakeupReason == 34 ||
                 WakeupReason == 35 ||
                 WakeupReason == 36 ) {
                printk("%s : Don't send hci_reset due to wake-up reason is %d!\n", __FUNCTION__, WakeupReason);
            }
            else {
                btmtk_usb_send_hci_reset_cmd(data->udev);
            }
        }
#else
        btmtk_usb_send_hci_reset_cmd(data->udev);

    #if ENABLE_BT_LOW_POWER_MODE
          // Enable BT Low Power
        btmtk_usb_send_enable_low_power_cmd(data->udev);

        // for WoBLE/WoW low power
        btmtk_usb_send_hci_set_ce_cmd(data->udev);
    #else
        // Disable BT Low Power
        btmtk_usb_send_disable_low_power_cmd(data->udev);
        // for WoBLE/WoW low power
        btmtk_usb_send_hci_set_ce_cmd_to_zero(data->udev);
    #endif
#endif

#if SUPPORT_SEND_DUMMY_BULK_OUT_AFTER_RESUME
        btmtk_usb_send_dummy_bulk_out_packet(data);
#endif
        goto error0;
    }

    urb = usb_alloc_urb(0, GFP_ATOMIC);
    if (!urb)  {
        ret = -ENOMEM;
        goto error0;
    }

    buf = usb_alloc_coherent(data->udev, UPLOAD_PATCH_UNIT, GFP_ATOMIC, &data_dma);

    if (!buf) {
        ret = -ENOMEM;
        goto error1;
    }

    pos = buf;

    if (LOAD_CODE_METHOD == BIN_FILE_METHOD)  {
        load_code_from_bin(&data->rom_patch, data->rom_patch_bin_file_name, &data->udev->dev, &data->rom_patch_len);
        printk("%s : BIN_FILE_METHOD\n", __FUNCTION__);
    }
    else  {
        data->rom_patch = data->rom_patch_header_image;
        printk("%s : HEADER_METHOD\n", __FUNCTION__);
    }

    if (!data->rom_patch) {
        if (LOAD_CODE_METHOD == BIN_FILE_METHOD) {
            printk("%s:please assign a rom patch(/etc/firmware/%s)or(/lib/firmware/%s)\n",
                __FUNCTION__, data->rom_patch_bin_file_name, data->rom_patch_bin_file_name);
        }
        else {
            printk("%s:please assign a rom patch\n", __FUNCTION__);
        }

        ret = -1;
        goto error2;
    }

    tmp_str = data->rom_patch;
    printk("%s : FW Version = %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n", __FUNCTION__,
            tmp_str[0], tmp_str[1], tmp_str[2], tmp_str[3],
            tmp_str[4], tmp_str[5], tmp_str[6], tmp_str[7],
            tmp_str[8], tmp_str[9], tmp_str[10], tmp_str[11],
            tmp_str[12], tmp_str[13], tmp_str[14], tmp_str[15]);

    printk("%s : build Time = %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n", __FUNCTION__,
            tmp_str[0], tmp_str[1], tmp_str[2], tmp_str[3],
            tmp_str[4], tmp_str[5], tmp_str[6], tmp_str[7],
            tmp_str[8], tmp_str[9], tmp_str[10], tmp_str[11],
            tmp_str[12], tmp_str[13], tmp_str[14], tmp_str[15]);

    memset(rom_patch_version, 0, sizeof(rom_patch_version));
    memcpy(rom_patch_version, tmp_str, 15);

    tmp_str = data->rom_patch + 16;
    printk("%s : platform = %c%c%c%c\n", __FUNCTION__, tmp_str[0], tmp_str[1], tmp_str[2], tmp_str[3]);


    tmp_str = data->rom_patch + 20;
    printk("%s : HW/SW version = %c%c%c%c \n", __FUNCTION__, tmp_str[0], tmp_str[1], tmp_str[2], tmp_str[3]);

    tmp_str = data->rom_patch + 24;
//    printk("%s : Patch version = %c%c%c%c \n", __FUNCTION__, tmp_str[0], tmp_str[1], tmp_str[2], tmp_str[3]);

    init_completion(&sent_to_mcu_done);

    cur_len = 0x00;
    patch_len = data->rom_patch_len - PATCH_INFO_SIZE;
    printk("%s : patch_len = %d\n", __FUNCTION__, patch_len);
    printk("%s : loading rom patch... \n\n", __FUNCTION__);
    /* loading rom patch */
    while (1) {
        s32 sent_len_max = UPLOAD_PATCH_UNIT - PATCH_HEADER_SIZE;
        sent_len = (patch_len - cur_len) >= sent_len_max ? sent_len_max : (patch_len - cur_len);

        printk("cur_len = %d\n", cur_len);
        printk("sent_len = %d\n", sent_len);

        if (sent_len > 0) {
            if (first_block == 1) {
                if (sent_len < sent_len_max)
                    phase = PATCH_PHASE3;
                else
                    phase = PATCH_PHASE1;
                first_block = 0;
            }
            else if (sent_len == sent_len_max) {
                phase = PATCH_PHASE2;
            }
            else {
                phase = PATCH_PHASE3;
            }

            /* prepare HCI header */
            pos[0] = 0x6F;
            pos[1] = 0xFC;
            pos[2] = (sent_len + 5) & 0xFF;
            pos[3] = ((sent_len + 5) >> 8) & 0xFF;

            /* prepare WMT header */
            pos[4] = 0x01;
            pos[5] = 0x01;
            pos[6] = (sent_len + 1) & 0xFF;
            pos[7] = ((sent_len + 1) >> 8) & 0xFF;

            pos[8] = phase;

            memcpy(&pos[9], data->rom_patch + PATCH_INFO_SIZE + cur_len, sent_len);
            printk("sent_len + PATCH_HEADER_SIZE = %d, phase = %d\n", sent_len + PATCH_HEADER_SIZE, phase);

            usb_fill_bulk_urb(urb,
                              data->udev,
                              pipe,
                              buf,
                              sent_len + PATCH_HEADER_SIZE,
                              load_rom_patch_complete,
                              &sent_to_mcu_done);

            urb->transfer_dma = data_dma;
            urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;

            ret = usb_submit_urb(urb, GFP_ATOMIC);
            if (ret)
                goto error2;

            if (!wait_for_completion_timeout(&sent_to_mcu_done, msecs_to_jiffies(1000))) {
                usb_kill_urb(urb);
                printk("%s : upload rom_patch timeout\n", __FUNCTION__);
                goto error2;
            }

            mdelay(1);
            cur_len += sent_len;
        }
        else {
            break;
        }
    }

    mdelay(20);
    ret = btmtk_usb_get_rom_patch_result(data);
    mdelay(20);

    // Send Checksum request
    total_checksum = checksume16(data->rom_patch + PATCH_INFO_SIZE, patch_len);
    btmtk_usb_chk_crc(data, patch_len);

    mdelay(20);

    if (total_checksum != btmtk_usb_get_crc(data)) {
        printk("%s : checksum fail!, local(0x%x) <> fw(0x%x)\n", __FUNCTION__, total_checksum, btmtk_usb_get_crc(data));
        ret = -1;
        goto error2;
    }

    mdelay(20);
    // send check rom patch result request
    btmtk_usb_send_check_rom_patch_result_cmd(data->udev);
    mdelay(20);
    // CHIP_RESET
    ret = btmtk_usb_reset_wmt(data);
    mdelay(20);
    // BT_RESET
    if ( btmtk_usb_send_hci_reset_cmd(data->udev) < 0 )
        goto error2;

    #if ENABLE_BT_LOW_POWER_MODE
    // Enable BT Low Power
    btmtk_usb_send_enable_low_power_cmd(data->udev);

    // for WoBLE/WoW low power
    btmtk_usb_send_hci_set_ce_cmd(data->udev);
    #else
    // Disable BT Low Power
    btmtk_usb_send_disable_low_power_cmd(data->udev);
    // for WoBLE/WoW low power
    btmtk_usb_send_hci_set_ce_cmd_to_zero(data->udev);
    #endif

error2:
    usb_free_coherent(data->udev, UPLOAD_PATCH_UNIT, buf, data_dma);
error1:
    usb_free_urb(urb);
error0:
    btmtk_usb_io_write32(data, SEMAPHORE_03, 0x1);

    #if BT_REDUCE_EP2_POLLING_INTERVAL_BY_INTR_TRANSFER
    // If rom patch is loaded by BT driver this time, BT driver should trigger switch to interrupt mode.
    if ( rom_patch_loaded == 0 ) {
        btmtk_usb_send_switch_to_interrupt_in_cmd(data->udev);
    }
    #endif

    printk("%s end\n", __FUNCTION__);
    return ret;
}
#endif

static int load_fw_iv(struct btmtk_usb_data *data) {
    int ret;
    struct usb_device *udev = data->udev;
    char *buf = kmalloc(64, GFP_ATOMIC);

    memmove(buf, data->fw_image + 32, 64);

    ret = usb_control_msg(udev, usb_sndctrlpipe(udev, 0), 0x01,
                          DEVICE_VENDOR_REQUEST_OUT, 0x12, 0x0, buf, 64,
                          CONTROL_TIMEOUT_JIFFIES);
    if (ret < 0) {
        printk("%s error(%d) step4\n", __FUNCTION__, ret);
        kfree(buf);
        return ret;
    }

    if (ret > 0) {
        ret = 0;
    }

    kfree(buf);

    return ret;
}

static void load_fw_complete(struct urb *urb) {
    struct completion *sent_to_mcu_done = (struct completion *)urb->context;
    complete(sent_to_mcu_done);
}

static int btmtk_usb_load_fw(struct btmtk_usb_data *data) {
    struct usb_device *udev = data->udev;
    struct urb *urb;
    void *buf;
    u32 cur_len = 0;
    u32 packet_header = 0;
    u32 value;
    u32 ilm_len = 0, dlm_len = 0;
    u16 fw_ver, build_ver;
    u32 loop = 0;
    dma_addr_t data_dma;
    int ret = 0, sent_len;
    struct completion sent_to_mcu_done;
    unsigned int pipe = usb_sndbulkpipe(data->udev,
                                        data->bulk_tx_ep->bEndpointAddress);

    BT_DBG("bulk_tx_ep = %x\n", data->bulk_tx_ep->bEndpointAddress);

loadfw_protect:
    btmtk_usb_switch_iobase(data, WLAN);
    btmtk_usb_io_read32(data, SEMAPHORE_00, &value);
    loop++;

    if (((value & 0x1) == 0) && (loop < 10000)) {
        goto loadfw_protect;
    }

    /* check MCU if ready */
    btmtk_usb_io_read32(data, COM_REG0, &value);

    if ((value & 0x01) == 0x01) {
        goto error0;
    }

    /* Enable MPDMA TX and EP2 load FW mode */
    btmtk_usb_io_write32(data, 0x238, 0x1c000000);

    btmtk_usb_reset(udev);
    mdelay(100);

    if (LOAD_CODE_METHOD == BIN_FILE_METHOD) {
        load_code_from_bin(&data->fw_image, data->fw_bin_file_name, &data->udev->dev, &data->fw_len);
        printk("%s : BIN_FILE_METHOD\n", __FUNCTION__);
    }
    else {
        data->fw_image = data->fw_header_image;
        printk("%s : HEADER_METHOD\n", __FUNCTION__);
    }

    if (!data->fw_image) {
        if (LOAD_CODE_METHOD == BIN_FILE_METHOD) {
            printk("%s:please assign a fw(/etc/firmware/%s)or(/lib/firmware/%s)\n",
                __FUNCTION__, data->fw_bin_file_name, data->fw_bin_file_name);
        }
        else {
            printk("%s:please assign a fw\n", __FUNCTION__);
        }

        ret = -1;
        goto error0;
    }

    ilm_len = (*(data->fw_image + 3) << 24) | (*(data->fw_image + 2) << 16) |
                (*(data->fw_image +1) << 8) | (*data->fw_image);

    dlm_len = (*(data->fw_image + 7) << 24) | (*(data->fw_image + 6) << 16) |
                (*(data->fw_image + 5) << 8) | (*(data->fw_image + 4));

    fw_ver = (*(data->fw_image + 11) << 8) | (*(data->fw_image + 10));

    build_ver = (*(data->fw_image + 9) << 8) | (*(data->fw_image + 8));

    BT_DBG("fw version:%d.%d.%02d ", (fw_ver & 0xf000) >> 8,
                                    (fw_ver & 0x0f00) >> 8,
                                    (fw_ver & 0x00ff));

    BT_DBG("build:%x\n", build_ver);

    BT_DBG("build Time = ");

    for (loop = 0; loop < 16; loop++)
        BT_DBG("%c", *(data->fw_image + 16 + loop));

    BT_DBG("\n");

    BT_DBG("ILM length = %d(bytes)\n", ilm_len);
    BT_DBG("DLM length = %d(bytes)\n", dlm_len);

    btmtk_usb_switch_iobase(data, SYSCTL);

    /* U2M_PDMA rx_ring_base_ptr */
    btmtk_usb_io_write32(data, 0x790, 0x400230);

    /* U2M_PDMA rx_ring_max_cnt */
    btmtk_usb_io_write32(data, 0x794, 0x1);

    /* U2M_PDMA cpu_idx */
    btmtk_usb_io_write32(data, 0x798, 0x1);

    /* U2M_PDMA enable */
    btmtk_usb_io_write32(data, 0x704, 0x44);

    urb = usb_alloc_urb(0, GFP_ATOMIC);
    if (!urb) {
        ret = -ENOMEM;
        goto error1;
    }

    buf = usb_alloc_coherent(udev, 14592, GFP_ATOMIC, &data_dma);
    if (!buf) {
        ret = -ENOMEM;
        goto error2;
    }

    BT_DBG("loading fw");

    init_completion(&sent_to_mcu_done);

    btmtk_usb_switch_iobase(data, SYSCTL);

    cur_len = 0x40;

    /* Loading ILM */
    while (1) {
        sent_len = (ilm_len - cur_len) >= 14336 ? 14336 : (ilm_len - cur_len);

        if (sent_len > 0) {
            packet_header &= ~(0xffffffff);
            packet_header |= (sent_len << 16);
            packet_header = cpu_to_le32(packet_header);

            memmove(buf, &packet_header, 4);
            memmove(buf + 4, data->fw_image + 32 + cur_len, sent_len);

            /* U2M_PDMA descriptor */
            btmtk_usb_io_write32(data, 0x230, cur_len);

            while ((sent_len % 4) != 0) {
                sent_len++;
            }

            /* U2M_PDMA length */
            btmtk_usb_io_write32(data, 0x234, sent_len << 16);

            usb_fill_bulk_urb(urb,
                              udev,
                              pipe,
                              buf,
                              sent_len + 4,
                              load_fw_complete,
                              &sent_to_mcu_done);

            urb->transfer_dma = data_dma;
            urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;

            ret = usb_submit_urb(urb, GFP_ATOMIC);
            if (ret) {
                goto error3;
            }

            if (!wait_for_completion_timeout(&sent_to_mcu_done, msecs_to_jiffies(1000))) {
                usb_kill_urb(urb);
                printk("%s : upload ilm fw timeout\n", __FUNCTION__);
                goto error3;
            }

            BT_DBG(".");

            mdelay(200);

            cur_len += sent_len;
        }
        else {
            break;
        }
    }

    init_completion(&sent_to_mcu_done);
    cur_len = 0x00;

    /* Loading DLM */
    while (1) {
        sent_len = (dlm_len - cur_len) >= 14336 ? 14336 : (dlm_len - cur_len);

        if (sent_len > 0) {
            packet_header &= ~(0xffffffff);
            packet_header |= (sent_len << 16);
            packet_header = cpu_to_le32(packet_header);

            memmove(buf, &packet_header, 4);
            memmove(buf + 4, data->fw_image + 32 + ilm_len + cur_len, sent_len);

            /* U2M_PDMA descriptor */
            btmtk_usb_io_write32(data, 0x230, 0x80000 + cur_len);

            while ((sent_len % 4) != 0) {
                BT_DBG("sent_len is not divided by 4\n");
                sent_len++;
            }

            /* U2M_PDMA length */
            btmtk_usb_io_write32(data, 0x234, sent_len << 16);

            usb_fill_bulk_urb(urb,
                              udev,
                              pipe,
                              buf,
                              sent_len + 4,
                              load_fw_complete,
                              &sent_to_mcu_done);

            urb->transfer_dma = data_dma;
            urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;

            ret = usb_submit_urb(urb, GFP_ATOMIC);
            if (ret) {
                goto error3;
            }

            if (!wait_for_completion_timeout(&sent_to_mcu_done, msecs_to_jiffies(1000))) {
                usb_kill_urb(urb);
                printk("%s : upload dlm fw timeout\n", __FUNCTION__);
                goto error3;
            }

            BT_DBG(".");

            mdelay(500);

            cur_len += sent_len;
        }
        else {
            break;
        }
    }

    /* upload 64bytes interrupt vector */
    ret = load_fw_iv(data);
    mdelay(100);

    btmtk_usb_switch_iobase(data, WLAN);

    /* check MCU if ready */
    loop = 0;

    do {
        btmtk_usb_io_read32(data, COM_REG0, &value);

        if (value == 0x01) {
            break;
        }

        mdelay(10);
        loop++;
    } while (loop <= 100);

    if (loop > 1000) {
        printk("%s : wait for 100 times\n", __FUNCTION__);
        ret = -ENODEV;
    }

error3:
    usb_free_coherent(udev, 14592, buf, data_dma);
error2:
    usb_free_urb(urb);
error1:
    /* Disbale load fw mode */
    btmtk_usb_io_read32(data, 0x238, &value);
    value = value & ~(0x10000000);
    btmtk_usb_io_write32(data,  0x238, value);
error0:
    btmtk_usb_io_write32(data, SEMAPHORE_00, 0x1);
    return ret;
}

static int inc_tx(struct btmtk_usb_data *data) {
    unsigned long flags;
    int rv;

    spin_lock_irqsave(&data->txlock, flags);
    rv = test_bit(BTUSB_SUSPENDING, &data->flags);
    if (!rv) {
        data->tx_in_flight++;
    }

    spin_unlock_irqrestore(&data->txlock, flags);

    return rv;
}

static void btmtk_usb_intr_complete(struct urb *urb) {
    struct hci_dev *hdev = urb->context;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0)
    struct btmtk_usb_data *data = hci_get_drvdata(hdev);
#else
    struct btmtk_usb_data *data = hdev->driver_data;
#endif
    int err;

    BT_DBG("%s: %s urb %p status %d count %d", __FUNCTION__, hdev->name,
                    urb, urb->status, urb->actual_length);

    if (!test_bit(HCI_RUNNING, &hdev->flags)) {
        return;
    }

    if (urb->status == 0) {
        int skip_this_event = 0;
        hdev->stat.byte_rx += urb->actual_length;

        // btmtk_usb_hex_dump("hci event", urb->transfer_buffer, urb->actual_length);

        if ( urb && urb->transfer_buffer ) {
            btmtk_usb_save_hci_event(urb->actual_length, urb->transfer_buffer);
        }

        

#if SUPPORT_PRINT_FW_LOG
        // check FW log event
        {
            unsigned char* event_buf = urb->transfer_buffer;
            u32 event_buf_len = urb->actual_length;
            if ( event_buf ) {
                int log_len = (event_buf[1] & 0xff);
                if ( event_buf_len && event_buf[0] == 0xff && event_buf[2] == 0x51 ) {
                    int i;
                    skip_this_event = 1;
                    printk("%s FW LOG (%d) : ", "btmtk_usb", event_buf_len);
                    for ( i = 0; i < event_buf_len; i++ ) {
                        printk("%02X ", event_buf[i]);
                    }
                    printk("\n");
                    if ( (event_buf_len-2) != log_len ) {
                        printk("%s FW LOG length error : got %d but len_value is %d\n", __FUNCTION__, event_buf_len, log_len);
                    }
                }
            }
        }
#endif




        {
            unsigned char* event_buf = urb->transfer_buffer;
            u32 event_buf_len = urb->actual_length;
            if ( event_buf ) {
                if ( (event_buf_len == 6 && event_buf[0] == 0x0e &&
                                            event_buf[1] == 0x04 &&
                                            event_buf[2] == 0x01 &&
                                            event_buf[3] == 0x03 &&
                                            event_buf[4] == 0x0c &&
                                            event_buf[5] == 0x00) ||
                     (event_buf_len == 4 && event_buf[0] == 0xe6 &&
                                            event_buf[1] == 0x02 &&
                                            event_buf[2] == 0x08 &&
                                            event_buf[3] == 0x00) ||
                     (event_buf_len == 4 && event_buf[0] == 0xe6 &&
                                            event_buf[1] == 0x02 &&
                                            event_buf[2] == 0x08 &&
                                            event_buf[3] == 0x01) ||
                     (event_buf_len == 14 && event_buf[0] == 0x0e &&
                                            event_buf[1] == 0x0c &&
                                            event_buf[2] == 0x01 &&
                                            event_buf[3] == 0x01 &&
                                            event_buf[4] == 0x10 )
                   ) {
                    int index;
                    printk("%s : got event : 0x", __FUNCTION__);
                    for ( index = 0; index < event_buf_len; index++ )
                        printk("%02X ", event_buf[index]);
                    printk("\n");
                }
            }

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 13, 0)
    // when kernel version >= 3.13, set HCI_RAW will cause hciconfig hci0 up error :
    //     Can't init device hci0: Cannot assign requested address (99)
#if SUPPORT_BLUEZ == 0
            if ( !test_bit(HCI_RAW, &hdev->flags) ) {
                printk("%s : ERROR !  HCI_RAW not set\n", __FUNCTION__);
                printk("%s : set HCI_RAW\n", __FUNCTION__);
                set_bit(HCI_RAW, &hdev->flags);
            }
#endif
#endif
        }

#if SUPPORT_BT_ATE
        // check ATE cmd event
        {
            unsigned char* event_buf = urb->transfer_buffer;
            u32 event_buf_len = urb->actual_length;
            u8 matched = 0;
            int i;
            u32 Count_Tx_ACL = 0;
            u32 Count_ACK = 0;
            u8 PHY_RATE = 0;
            if ( event_buf ) {
                if ( event_buf[3] == 0x4D && event_buf[4] == 0xFC )
                    matched = 1;

                if ( event_buf[3] == 0x3F && event_buf[4] == 0x0C )
                    matched = 2;

                if ( matched == 1 ) {
                    skip_this_event = 1;
                    printk("%s Got ATE event result:(%d) \n    ", __FUNCTION__, event_buf_len);
                    for ( i = 0; i < event_buf_len; i++ ) {
                        printk("%02X ", event_buf[i]);
                    }
                    printk("\n");

                    Count_Tx_ACL = event_buf[6] | ((event_buf[7] << 8) & 0xff00) | ((event_buf[8] << 16) & 0xff0000) | ((event_buf[9] << 24) & 0xff000000);
                    Count_ACK = event_buf[10] | ((event_buf[11] << 8) & 0xff00) | ((event_buf[12] << 16) & 0xff0000) | ((event_buf[13] << 24) & 0xff000000);
                    PHY_RATE = event_buf[14];

                    printk("Count_Tx_ACL = 0x%08X\n", Count_Tx_ACL);
                    printk("Count_ACK = 0x%08X\n", Count_ACK);
                    if ( PHY_RATE == 0 )
                        printk("PHY_RATE = 1M_DM\n");
                    else if ( PHY_RATE == 1 )
                        printk("PHY_RATE = 1M_DH\n");
                    else if ( PHY_RATE == 2 )
                        printk("PHY_RATE = 2M\n");
                    else if ( PHY_RATE == 3 )
                        printk("PHY_RATE = 3M\n");
                }
                else if ( matched == 2 ) {
                    printk("%s Got ATE AFH Host Channel Classification Command Result:(%d) \n    ", __FUNCTION__, event_buf_len);
                    for ( i = 0; i < event_buf_len; i++ ) {
                        printk("%02X ", event_buf[i]);
                    }
                    printk("\n");

                    if ( event_buf[5] == 0 )
                        printk("Result: Success\n");
                    else
                        printk("Result: Failed(0x%x)\n", event_buf[5]);
                }
            }
        }
#endif  // SUPPORT_BT_ATE

        if ( skip_this_event == 0 ) {
            if (hci_recv_fragment(hdev, HCI_EVENT_PKT,
                            urb->transfer_buffer,
                            urb->actual_length) < 0) {
                printk("%s : %s corrupted event packet", __FUNCTION__, hdev->name);
                hdev->stat.err_rx++;
            }
        }
        else {
            printk("%s : skip this event!\n", __FUNCTION__);
        }
    }

    if (!test_bit(BTUSB_INTR_RUNNING, &data->flags))
        return;

    usb_mark_last_busy(data->udev);
    usb_anchor_urb(urb, &data->intr_anchor);

    err = usb_submit_urb(urb, GFP_ATOMIC);
    if (err < 0) {
        /* -EPERM: urb is being killed;
         * -ENODEV: device got disconnected */
        if (err != -EPERM && err != -ENODEV)
            printk("%s : %s urb %p failed to resubmit intr_in_urb(%d)", __FUNCTION__, hdev->name, urb, -err);
        usb_unanchor_urb(urb);
    }
}

static int btmtk_usb_submit_intr_urb(struct hci_dev *hdev, gfp_t mem_flags) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0)
    struct btmtk_usb_data *data = hci_get_drvdata(hdev);
#else
    struct btmtk_usb_data *data = hdev->driver_data;
#endif
    struct urb *urb;
    unsigned char *buf;
    unsigned int pipe;
    int err, size;

    BT_DBG("%s", __FUNCTION__);

    if (!data->intr_ep) {
        printk("%s : error 1 : data->intr_ep == NULL\n", __FUNCTION__);
        return -ENODEV;
    }

    urb = usb_alloc_urb(0, mem_flags);
    if (!urb) {
        printk("%s : error 2 : usb_alloc_urb failed\n", __FUNCTION__);
        return -ENOMEM;
    }

    size = le16_to_cpu(data->intr_ep->wMaxPacketSize);

    buf = kmalloc(size, mem_flags);
    if (!buf) {
        printk("%s : error 3 : kmalloc failed\n", __FUNCTION__);
        usb_free_urb(urb);
        return -ENOMEM;
    }

    pipe = usb_rcvintpipe(data->udev, data->intr_ep->bEndpointAddress);

    usb_fill_int_urb(urb, data->udev, pipe, buf, size,
                        btmtk_usb_intr_complete, hdev,
                        data->intr_ep->bInterval);

    urb->transfer_flags |= URB_FREE_BUFFER;

    usb_anchor_urb(urb, &data->intr_anchor);

    err = usb_submit_urb(urb, mem_flags);
    if (err < 0) {
        if (err != -EPERM && err != -ENODEV)
            printk("%s : %s urb %p submission failed (%d)", __FUNCTION__, hdev->name, urb, -err);
        usb_unanchor_urb(urb);
    }

    usb_free_urb(urb);

    return err;
}

static void btmtk_usb_bulk_in_complete(struct urb *urb) {
    struct hci_dev *hdev = urb->context;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0)
    struct btmtk_usb_data *data = hci_get_drvdata(hdev);
#else
    struct btmtk_usb_data *data = hdev->driver_data;
#endif
    int err;

    BT_DBG("%s:%s urb %p status %d count %d", __FUNCTION__, hdev->name,
                    urb, urb->status, urb->actual_length);

    if (!test_bit(HCI_RUNNING, &hdev->flags)) {
        return;
    }

#if SUPPORT_A2DP_LATENCY_MEASUREMENT
    {
        u8* buf = urb->transfer_buffer;
        u16 len = urb->actual_length;
        if ( len > 8 && buf[8] == 0x80 && buf[9] == 0x01 ) {
            timestamp_counter++;
            if ( timestamp_counter%100 == 1 ) {
                printk("receive frame : %u\n", _btmtk_usb_get_microseconds());
            }
        }
    }
#endif

#if SUPPORT_FW_DUMP
    {
        u8* buf = urb->transfer_buffer;
        u16 len = 0;
        if ( urb->actual_length > 4 ) {
            len = buf[2] + ((buf[3] << 8) & 0xff00);
            if ( buf[0] == 0x6f && buf[1] == 0xfc && len+4 == urb->actual_length ) {
                if ( fw_dump_buffer_full ) {
                    printk("btmtk FW DUMP : data comes when buffer full!! (Should Never Hzappen!!)\n");
                }

                fw_dump_total_read_size += len;
                if ( fw_dump_write_ptr + len < fw_dump_ptr+FW_DUMP_BUF_SIZE ) {
                    fw_dump_buffer_used_size += len;
                    if ( fw_dump_buffer_used_size + 512 > FW_DUMP_BUF_SIZE ) {
                        fw_dump_buffer_full = 1;
                    }

                    memcpy(fw_dump_write_ptr, &buf[4], len);
                    fw_dump_write_ptr += len;
                    up(&data->fw_dump_semaphore);
                }
                else {
                    printk("btmtk FW DUMP : buffer size too small ! (%d:%d) (Should Never Happen!!)\n", FW_DUMP_BUF_SIZE, fw_dump_total_read_size);
                }
            }
        }
    }
#endif

    if (urb->status == 0) {
        hdev->stat.byte_rx += urb->actual_length;

        if (hci_recv_fragment(hdev, HCI_ACLDATA_PKT,
                        urb->transfer_buffer,
                        urb->actual_length) < 0) {
            printk("%s corrupted ACL packet", hdev->name);
            hdev->stat.err_rx++;
        }
    }

    if (!test_bit(BTUSB_BULK_RUNNING, &data->flags))
        return;

    usb_anchor_urb(urb, &data->bulk_anchor);
    usb_mark_last_busy(data->udev);

#if SUPPORT_FW_DUMP
    if ( fw_dump_buffer_full ) {
        fw_dump_bulk_urb = urb;
        err = 0;
    }
    else {
        err = usb_submit_urb(urb, GFP_ATOMIC);
    }
#else
    err = usb_submit_urb(urb, GFP_ATOMIC);
#endif
    if (err < 0) {
        /* -EPERM: urb is being killed;
         * -ENODEV: device got disconnected */
        if (err != -EPERM && err != -ENODEV)
            printk("%s : %s urb %p failed to resubmit bulk_in_urb(%d)", __FUNCTION__, hdev->name, urb, -err);
        usb_unanchor_urb(urb);
    }
}

static int btmtk_usb_submit_bulk_in_urb(struct hci_dev *hdev, gfp_t mem_flags) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0)
    struct btmtk_usb_data *data = hci_get_drvdata(hdev);
#else
    struct btmtk_usb_data *data = hdev->driver_data;
#endif
    struct urb *urb;
    unsigned char *buf;
    unsigned int pipe;
    int err, size = HCI_MAX_FRAME_SIZE;

    BT_DBG("%s:%s", __FUNCTION__, hdev->name);

    if (!data->bulk_rx_ep)
        return -ENODEV;

    urb = usb_alloc_urb(0, mem_flags);
    if (!urb)
        return -ENOMEM;

    buf = kmalloc(size, mem_flags);
    if (!buf) {
        usb_free_urb(urb);
        return -ENOMEM;
    }

    // BT_REDUCE_EP2_POLLING_INTERVAL_BY_INTR_TRANSFER
    if ( usb_endpoint_is_int_in(data->bulk_rx_ep) ) {
        pipe = usb_rcvintpipe(data->udev, data->bulk_rx_ep->bEndpointAddress);
        usb_fill_int_urb(urb, data->udev, pipe, buf, size,
                            btmtk_usb_bulk_in_complete, hdev, 4);   // interval : 1ms
    }
    else {
        pipe = usb_rcvbulkpipe(data->udev, data->bulk_rx_ep->bEndpointAddress);
        usb_fill_bulk_urb(urb, data->udev, pipe,
                            buf, size, btmtk_usb_bulk_in_complete, hdev);
    }

    urb->transfer_flags |= URB_FREE_BUFFER;

    usb_mark_last_busy(data->udev);
    usb_anchor_urb(urb, &data->bulk_anchor);

    err = usb_submit_urb(urb, mem_flags);
    if (err < 0) {
        if (err != -EPERM && err != -ENODEV)
            printk("%s : %s urb %p submission failed (%d)", __FUNCTION__, hdev->name, urb, -err);
        usb_unanchor_urb(urb);
    }

    usb_free_urb(urb);

    return err;
}

static void btmtk_usb_isoc_in_complete(struct urb *urb) {
    struct hci_dev *hdev = urb->context;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0)
    struct btmtk_usb_data *data = hci_get_drvdata(hdev);
#else
    struct btmtk_usb_data *data = hdev->driver_data;
#endif
    int i, err;

    BT_DBG("%s: %s urb %p status %d count %d", __FUNCTION__, hdev->name,
                    urb, urb->status, urb->actual_length);

    if (!test_bit(HCI_RUNNING, &hdev->flags))
        return;

    if (urb->status == 0) {
        for (i = 0; i < urb->number_of_packets; i++) {
            unsigned int offset = urb->iso_frame_desc[i].offset;
            unsigned int length = urb->iso_frame_desc[i].actual_length;

            if (urb->iso_frame_desc[i].status)
                continue;

            hdev->stat.byte_rx += length;

            if (hci_recv_fragment(hdev, HCI_SCODATA_PKT,
                        urb->transfer_buffer + offset,
                                length) < 0) {
                printk("%s corrupted SCO packet", hdev->name);
                hdev->stat.err_rx++;
            }
        }
    }

    if (!test_bit(BTUSB_ISOC_RUNNING, &data->flags))
        return;

    usb_anchor_urb(urb, &data->isoc_anchor);

    err = usb_submit_urb(urb, GFP_ATOMIC);
    if (err < 0) {
        /* -EPERM: urb is being killed;
         * -ENODEV: device got disconnected */
        if (err != -EPERM && err != -ENODEV)
            printk("%s urb %p failed to resubmit iso_in_urb(%d)",
                        hdev->name, urb, -err);
        usb_unanchor_urb(urb);
    }
}

static inline void __fill_isoc_descriptor(struct urb *urb, int len, int mtu) {
    int i, offset = 0;

    BT_DBG("len %d mtu %d", len, mtu);

    for (i = 0; i < BTUSB_MAX_ISOC_FRAMES && len >= mtu;
                    i++, offset += mtu, len -= mtu) {
        urb->iso_frame_desc[i].offset = offset;
        urb->iso_frame_desc[i].length = mtu;
    }

    if (len && i < BTUSB_MAX_ISOC_FRAMES) {
        urb->iso_frame_desc[i].offset = offset;
        urb->iso_frame_desc[i].length = len;
        i++;
    }

    urb->number_of_packets = i;
}

static int btmtk_usb_submit_isoc_in_urb(struct hci_dev *hdev, gfp_t mem_flags) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0)
    struct btmtk_usb_data *data = hci_get_drvdata(hdev);
#else
    struct btmtk_usb_data *data = hdev->driver_data;
#endif
    struct urb *urb;
    unsigned char *buf;
    unsigned int pipe;
    int err, size;

    BT_DBG("%s\n", __FUNCTION__);

    if (!data->isoc_rx_ep)
        return -ENODEV;

    urb = usb_alloc_urb(BTUSB_MAX_ISOC_FRAMES, mem_flags);
    if (!urb)
        return -ENOMEM;

    size = le16_to_cpu(data->isoc_rx_ep->wMaxPacketSize) *
                        BTUSB_MAX_ISOC_FRAMES;

    buf = kmalloc(size, mem_flags);
    if (!buf) {
        usb_free_urb(urb);
        return -ENOMEM;
    }

    pipe = usb_rcvisocpipe(data->udev, data->isoc_rx_ep->bEndpointAddress);

    usb_fill_int_urb(urb, data->udev, pipe, buf, size, btmtk_usb_isoc_in_complete,
                hdev, data->isoc_rx_ep->bInterval);

    urb->transfer_flags  = URB_FREE_BUFFER | URB_ISO_ASAP;

    __fill_isoc_descriptor(urb, size,
            le16_to_cpu(data->isoc_rx_ep->wMaxPacketSize));

    usb_anchor_urb(urb, &data->isoc_anchor);

    err = usb_submit_urb(urb, mem_flags);
    if (err < 0) {
        if (err != -EPERM && err != -ENODEV)
            printk("%s urb %p submission failed (%d)",
                        hdev->name, urb, -err);
        usb_unanchor_urb(urb);
    }

    usb_free_urb(urb);

    return err;
}

static int btmtk_usb_open(struct hci_dev *hdev) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0)
    struct btmtk_usb_data *data = hci_get_drvdata(hdev);
#else
    struct btmtk_usb_data *data = hdev->driver_data;
#endif
    int err;

    printk("%s begin\n", __FUNCTION__);

    err = usb_autopm_get_interface(data->intf);
    if (err < 0) {
        printk("%s : error 1\n", __FUNCTION__);
        return err;
    }

    data->intf->needs_remote_wakeup = 1;

    if (test_and_set_bit(HCI_RUNNING, &hdev->flags)) {
        printk("%s : error 2\n", __FUNCTION__);
        goto done;
    }

    if (test_and_set_bit(BTUSB_INTR_RUNNING, &data->flags)) {
        printk("%s : error 3\n", __FUNCTION__);
        goto done;
    }

    err = btmtk_usb_submit_intr_urb(hdev, GFP_KERNEL);
    if (err < 0) {
        printk("%s : error 4\n", __FUNCTION__);
        goto failed;
    }

    err = btmtk_usb_submit_bulk_in_urb(hdev, GFP_KERNEL);
    if (err < 0) {
        printk("%s : error 5\n", __FUNCTION__);
        usb_kill_anchored_urbs(&data->intr_anchor);
        goto failed;
    }

    set_bit(BTUSB_BULK_RUNNING, &data->flags);
    // btmtk_usb_submit_bulk_in_urb(hdev, GFP_KERNEL);

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 13, 0)
    // when kernel version >= 3.13, set HCI_RAW will cause hciconfig hci0 up error :
    //     Can't init device hci0: Cannot assign requested address (99)
#if SUPPORT_BLUEZ == 0
    printk("%s : set HCI_RAW\n", __FUNCTION__);
    set_bit(HCI_RAW, &hdev->flags);
#endif
#endif


#if LOAD_PROFILE
    btmtk_usb_load_profile(hdev);
#endif

#if SUPPORT_BT_WAKEUP
    if (enable_bt_wakeup == 1)
        btmtk_usb_clear_feature_remote_wake_up(data);
#endif

#if SUPPORT_FW_DUMP
    {
        sema_init(&data->fw_dump_semaphore, 0);
        data->fw_dump_tsk = kthread_create(btmtk_usb_fw_dump_thread, (void*)data, "btmtk_usb_fw_dump_thread");
        if (IS_ERR(data->fw_dump_tsk)) {
            printk("%s : create fw dump thread failed!\n", __FUNCTION__);
            err = PTR_ERR(data->fw_dump_tsk);
            data->fw_dump_tsk = NULL;
            goto failed;
        }
        fw_dump_task_should_stop = 0;
        if ( fw_dump_ptr == NULL ) {
            fw_dump_ptr = kmalloc(FW_DUMP_BUF_SIZE, GFP_KERNEL);
        }

        if ( fw_dump_ptr == NULL ) {
            printk("%s : kmalloc(%d) failed!\n", __FUNCTION__, FW_DUMP_BUF_SIZE);
            goto failed;
        }
        memset(fw_dump_ptr, 0, FW_DUMP_BUF_SIZE);

        fw_dump_file = NULL;
        fw_dump_read_ptr = fw_dump_ptr;
        fw_dump_write_ptr = fw_dump_ptr;
        fw_dump_total_read_size = 0;
        fw_dump_total_write_size = 0;
        fw_dump_buffer_used_size = 0;
        fw_dump_buffer_full = 0;
        fw_dump_bulk_urb = NULL;
        data->fw_dump_end_check_tsk = NULL;
        wake_up_process(data->fw_dump_tsk);
    }

    if ( should_trigger_core_dump_after_open ) {
        should_trigger_core_dump_after_open = 0;
        if ( btmtk_usb_send_core_dump_cmd(data->udev) < 0 ) {
            btmtk_usb_toggle_rst_pin();
        }
    }
#endif

#if SUPPORT_A2DP_LATENCY_DEBUG
    a2dp_latency_debug_last_frame_time = 0;
#endif

done:
    usb_autopm_put_interface(data->intf);
    printk("%s end\n", __FUNCTION__);
    return 0;

failed:
    clear_bit(BTUSB_INTR_RUNNING, &data->flags);
    clear_bit(HCI_RUNNING, &hdev->flags);
    usb_autopm_put_interface(data->intf);
    return err;
}

static void btmtk_usb_stop_traffic(struct btmtk_usb_data *data) {
    BT_DBG("%s", __FUNCTION__);

    usb_kill_anchored_urbs(&data->intr_anchor);
    usb_kill_anchored_urbs(&data->bulk_anchor);
    // SCO
    // usb_kill_anchored_urbs(&data->isoc_anchor);
}

static int btmtk_usb_close(struct hci_dev *hdev) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0)
    struct btmtk_usb_data *data = hci_get_drvdata(hdev);
#else
    struct btmtk_usb_data *data = hdev->driver_data;
#endif
    int err;
    int counter = 0;
    (void)counter;

    printk("%s begin\n", __FUNCTION__);

#if SUPPORT_BT_WAKEUP
    if (enable_bt_wakeup == 1)
        btmtk_usb_set_feature_remote_wake_up(data);
#endif
    if (!test_and_clear_bit(HCI_RUNNING, &hdev->flags)) {
        printk("%s end (HCI not running)\n", __FUNCTION__);
        return 0;
    }

#if SUPPORT_FW_DUMP
    if ( data->fw_dump_tsk ) {
        fw_dump_task_should_stop = 1;
        up(&data->fw_dump_semaphore);
    }
    if ( data->fw_dump_end_check_tsk && !fw_dump_end_checking_task_stoped ) {
        printk("%s : wait 10s for fw dump process ....\n", __FUNCTION__);
        mdelay(10000);
        printk("%s : wait 10s for fw dump process done\n", __FUNCTION__);
        fw_dump_end_checking_task_should_stop = 1;
    }
    if ( fw_dump_task_should_stop || (fw_dump_end_checking_task_should_stop && fw_dump_end_checking_task_stoped == 0) ) {
        mdelay(10);
    }

    while ( fw_dump_task_should_stop != 0 || (fw_dump_end_checking_task_should_stop && fw_dump_end_checking_task_stoped == 0) ) {
        counter++;
        printk("%s : wait for bt driver thread termination %d:%d:%d.\n", __FUNCTION__, fw_dump_task_should_stop, fw_dump_end_checking_task_should_stop, fw_dump_end_checking_task_stoped);
        mdelay(100);
        if ( counter > 30 ) {
            printk("%s : 3s wait timeout, stop waiting\n", __FUNCTION__);
            break;
        }
    }
/*
    if ( fw_dump_ptr )
    {
        kfree(fw_dump_ptr);
        fw_dump_ptr = NULL;
    }
*/
#endif

    cancel_work_sync(&data->work);
    cancel_work_sync(&data->waker);
    // SCO
    // clear_bit(BTUSB_ISOC_RUNNING, &data->flags);
    clear_bit(BTUSB_BULK_RUNNING, &data->flags);
    clear_bit(BTUSB_INTR_RUNNING, &data->flags);

    btmtk_usb_stop_traffic(data);

    err = usb_autopm_get_interface(data->intf);
    if (err < 0) {
        printk("%s error!\n", __FUNCTION__);
        goto failed;
    }

    data->intf->needs_remote_wakeup = 0;
    usb_autopm_put_interface(data->intf);

failed:
    usb_scuttle_anchored_urbs(&data->deferred);
    printk("%s end\n", __FUNCTION__);
    return 0;
}

static int btmtk_usb_flush(struct hci_dev *hdev) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0)
    struct btmtk_usb_data *data = hci_get_drvdata(hdev);
#else
    struct btmtk_usb_data *data = hdev->driver_data;
#endif

    BT_DBG("%s", __FUNCTION__);

    usb_kill_anchored_urbs(&data->tx_anchor);

    return 0;
}

static void btmtk_usb_tx_complete(struct urb *urb) {
    struct sk_buff *skb = urb->context;
    struct hci_dev *hdev = (struct hci_dev *)skb->dev;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0)
    struct btmtk_usb_data *data = hci_get_drvdata(hdev);
#else
    struct btmtk_usb_data *data = hdev->driver_data;
#endif

    BT_DBG("%s: %s urb %p status %d count %d\n", __FUNCTION__, hdev->name,
                    urb, urb->status, urb->actual_length);

    if (!test_bit(HCI_RUNNING, &hdev->flags))
        goto done;

    if (!urb->status)
        hdev->stat.byte_tx += urb->transfer_buffer_length;
    else
        hdev->stat.err_tx++;

#if SUPPORT_A2DP_LATENCY_MEASUREMENT
    if ( skb->len > 8 && skb->data[8]==0x80 && skb->data[9]==0x60)
    {
    	//btmtk_usb_get_microseconds(&after_send);
		//btmtk_usb_print_timestamp();
		
        send_frame_done_counter++;
        btmtk_usb_get_microseconds(&send_frame_done);
        _btmtk_print_timestamp("D", send_frame_done_counter, skb->len, &send_frame_done);
    }
   
#endif

done:
    spin_lock(&data->txlock);
    data->tx_in_flight--;
    spin_unlock(&data->txlock);

    kfree(urb->setup_packet);

    kfree_skb(skb);
}

static void btmtk_usb_isoc_tx_complete(struct urb *urb) {
    struct sk_buff *skb = urb->context;
    struct hci_dev *hdev = (struct hci_dev *) skb->dev;

    BT_DBG("%s: %s urb %p status %d count %d", __FUNCTION__, hdev->name,
                    urb, urb->status, urb->actual_length);

    if (!test_bit(HCI_RUNNING, &hdev->flags))
        goto done;

    if (!urb->status)
        hdev->stat.byte_tx += urb->transfer_buffer_length;
    else
        hdev->stat.err_tx++;

done:
    kfree(urb->setup_packet);
    kfree_skb(skb);
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 13, 0)
static int btmtk_usb_send_frame(struct hci_dev *hdev, struct sk_buff *skb) {
#else
static int btmtk_usb_send_frame(struct sk_buff *skb) {
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 13, 0)
    struct hci_dev *hdev = (struct hci_dev *)skb->dev;
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0)
    struct btmtk_usb_data *data = hci_get_drvdata(hdev);
#else
    struct btmtk_usb_data *data = hdev->driver_data;
#endif
    struct usb_ctrlrequest *dr;
    struct urb *urb;
    unsigned int pipe;
    int err;

    BT_DBG("%s\n", __FUNCTION__);

    if (!test_bit(HCI_RUNNING, &hdev->flags)) {
#if SUPPORT_SEND_DUMMY_ERROR_HCI_EVENT_AFTER_RESUME
        if ( is_in_skip_all_cmd_state ) {
            btmtk_usb_send_dummy_error_hci_event(skb, hdev);
            return 0;
        }
#endif
        printk("%s : Error1\n", __FUNCTION__);
        return -EBUSY;
    }

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 13, 0)
    skb->dev = (void*) hdev;
#endif

    switch (bt_cb(skb)->pkt_type) {
        case HCI_COMMAND_PKT:
            btmtk_usb_save_hci_cmd(skb->len, skb->data);

#if SUPPORT_UNPREDICTED_HCI_CMD_FILTER
            if ( skb->len == 6 && skb->data[0] == 0xc9 &&
                                  skb->data[1] == 0xfc &&
                                  skb->data[2] == 0x03 &&
                                  skb->data[3] == 0x01 &&
                                  skb->data[4] == 0x0a &&
                                  skb->data[5] == 0x00 ) {
                printk("%s : unpredicetd HCI command : %02X %02X %02X %02X %02X %02X (reject it)\n", __FUNCTION__,
                                                                                        skb->data[0],
                                                                                        skb->data[1],
                                                                                        skb->data[2],
                                                                                        skb->data[3],
                                                                                        skb->data[4],
                                                                                        skb->data[5]);
                return 0;
            }
#endif
#if SUPPORT_BT_RECOVER
            if ( (skb->len == 8 &&
                  skb->data[0] == 0x6F &&
                  skb->data[1] == 0xFc &&
                  skb->data[2] == 0x05 &&
                  skb->data[3] == 0x01 &&
                  skb->data[4] == 0x02 &&
                  skb->data[5] == 0x01 &&
                  skb->data[6] == 0x00 &&
                  skb->data[7] == 0x08) ) {
                printk("%s : got trigger fw assert command : 0x6F FC 05 01 02 01 00 08 , toggle rst pin \n", __FUNCTION__);
                btmtk_usb_toggle_rst_pin();
            }
#endif
            if ( (skb->len == 3 &&
                  skb->data[0] == 0x03 &&
                  skb->data[1] == 0x0c &&
                  skb->data[2] == 0x00) ) {
                printk("%s : got command : 0x03 0c 00 (HCI_RESET) \n", __FUNCTION__);
#if SUPPORT_SEND_DUMMY_ERROR_HCI_EVENT_AFTER_RESUME
                printk("%s : exit reject_all_cmd state\n", __FUNCTION__);
                is_in_skip_all_cmd_state = 0;
#endif
            }

            if ( (skb->len == 3 &&
                  skb->data[0] == 0x01 &&
                  skb->data[1] == 0x10 &&
                  skb->data[2] == 0x00) ) {
                printk("%s : got command : 0x01 10 00 (READ_LOCAL_VERSION) \n", __FUNCTION__);
            }

            if ( skb->data[0] == 0xc9 &&
                 skb->data[1] == 0xfc ) {
                int index;
                printk("%s : got command : 0x", __FUNCTION__);
                for ( index = 0; index < skb->len; index++ )
                    printk("%02X ", skb->data[index]);
                printk("\n");
#if SUPPORT_SEND_DUMMY_ERROR_HCI_EVENT_AFTER_RESUME
                if ( skb->data[0] == 0xc9 &&
                     skb->data[1] == 0xfc &&
                     skb->data[2] == 0x02 &&
                     skb->data[3] == 0x01 &&
                     skb->data[4] == 0x01 ) {
                    printk("%s : exit reject_all_cmd state\n", __FUNCTION__);
                    is_in_skip_all_cmd_state = 0;
                }
#endif
            }

            if ( (skb->len == 8 &&
                  skb->data[0] == 0x6f &&
                  skb->data[1] == 0xfc &&
                  skb->data[2] == 0x05 &&
                  skb->data[3] == 0x01 &&
                  skb->data[4] == 0x02 &&
                  skb->data[5] == 0x01 &&
                  skb->data[6] == 0x00 &&
                  skb->data[7] == 0x08) ) {
                printk("%s : got FW assert hci cmd (by blueangel)! \n", __FUNCTION__);
                btmtk_usb_hci_dump_print_to_log();
            }

#if SUPPORT_SEND_DUMMY_ERROR_HCI_EVENT_AFTER_RESUME
            if ( is_in_skip_all_cmd_state ) {
                btmtk_usb_send_dummy_error_hci_event(skb, hdev);
                return 0;
            }
#endif

            urb = usb_alloc_urb(0, GFP_ATOMIC);
            if (!urb) {
                printk("%s : usb_alloc_urb failed!\n", __FUNCTION__);
                return -ENOMEM;
            }

            dr = kmalloc(sizeof(*dr), GFP_ATOMIC);
            if (!dr) {
                printk("%s : kmalloc failed!\n", __FUNCTION__);
                usb_free_urb(urb);
                return -ENOMEM;
            }

            dr->bRequestType = data->cmdreq_type;
            dr->bRequest     = 0;
            dr->wIndex       = 0;
            dr->wValue       = 0;
            dr->wLength      = __cpu_to_le16(skb->len);

            pipe = usb_sndctrlpipe(data->udev, 0x00);

            if (test_bit(HCI_RUNNING, &hdev->flags)) {
                u16 op_code;
                memcpy(&op_code, skb->data, 2);
                BT_DBG("ogf = %x\n", (op_code & 0xfc00) >> 10);
                BT_DBG("ocf = %x\n", op_code & 0x03ff);
                // btmtk_usb_hex_dump("hci command", skb->data, skb->len);
            }

            usb_fill_control_urb(urb, data->udev, pipe, (void *) dr,
                    skb->data, skb->len, btmtk_usb_tx_complete, skb);

            hdev->stat.cmd_tx++;
            break;

        case HCI_ACLDATA_PKT:
            if (!data->bulk_tx_ep) {
                printk("%s : data->bulk_tx_ep == NULL !\n", __FUNCTION__);
                return -ENODEV;
            }

#if 0//SUPPORT_A2DP_LATENCY_MEASUREMENT
        {
            /*unsigned int current_time = _btmtk_usb_get_microseconds();
            if ( a2dp_latency_debug_last_frame_time && (current_time - a2dp_latency_debug_last_frame_time) > 30000)
            {
                printk(CAM_RED"%s : Warning, ACL DATA duration more than 30ms! (%dms)\n"CAM_COLOR_END, __FUNCTION__, (unsigned int)(current_time-a2dp_latency_debug_last_frame_time));
            }
            a2dp_latency_debug_last_frame_time =  current_time;*/
            if ( skb->len > 8 && skb->data[8]==0x80 && skb->data[9]==0x60)
            {
            	btmtk_usb_get_microseconds(&before_send);
            }
        }
#endif

            urb = usb_alloc_urb(0, GFP_ATOMIC);
            if (!urb) {
                printk("%s : usb_alloc_urb failed2 !\n", __FUNCTION__);
                return -ENOMEM;
            }

            pipe = usb_sndbulkpipe(data->udev,
                        data->bulk_tx_ep->bEndpointAddress);

            usb_fill_bulk_urb(urb, data->udev, pipe,
                    skb->data, skb->len, btmtk_usb_tx_complete, skb);

#if SUPPORT_A2DP_LATENCY_MEASUREMENT
        {
            if ( skb->len > 8 && skb->data[8]==0x80 && skb->data[9]==0x60)
            {
                /*
                timestamp_counter++;
                if ( timestamp_counter%100 == 1 )
                {
                    printk("send frame : %u\n", _btmtk_usb_get_microseconds());
                }
                */
                
                send_frame_counter++;
                btmtk_usb_get_microseconds(&send_frame);
                //_btmtk_print_timestamp("S", send_frame_counter, skb->len, &send_frame);
            }
        }
#endif

            hdev->stat.acl_tx++;
            BT_DBG("HCI_ACLDATA_PKT:\n");
            break;

        case HCI_SCODATA_PKT:
            if (!data->isoc_tx_ep || hdev->conn_hash.sco_num < 1)
                return -ENODEV;

            urb = usb_alloc_urb(BTUSB_MAX_ISOC_FRAMES, GFP_ATOMIC);
            if (!urb)
                return -ENOMEM;

            pipe = usb_sndisocpipe(data->udev,
                        data->isoc_tx_ep->bEndpointAddress);

            usb_fill_int_urb(urb, data->udev, pipe,
                    skb->data, skb->len, btmtk_usb_isoc_tx_complete,
                    skb, data->isoc_tx_ep->bInterval);

            urb->transfer_flags  = URB_ISO_ASAP;

            __fill_isoc_descriptor(urb, skb->len,
                    le16_to_cpu(data->isoc_tx_ep->wMaxPacketSize));

            hdev->stat.sco_tx++;
            BT_DBG("HCI_SCODATA_PKT:\n");
            goto skip_waking;

        default:
            printk("%s : Error2\n", __FUNCTION__);
            return -EILSEQ;
    }

    err = inc_tx(data);

    if (err) {
        printk("%s : Error3\n", __FUNCTION__);
        usb_anchor_urb(urb, &data->deferred);
        schedule_work(&data->waker);
        err = 0;
        goto done;
    }

skip_waking:
    usb_anchor_urb(urb, &data->tx_anchor);

    err = usb_submit_urb(urb, GFP_ATOMIC);
    if (err < 0) {
        printk("%s : %s urb %p submission failed (%d)", __FUNCTION__, hdev->name, urb, -err);
        kfree(urb->setup_packet);
        usb_unanchor_urb(urb);
    }
    else {
        usb_mark_last_busy(data->udev);
    }

done:
    usb_free_urb(urb);
    return err;
}

#if LOAD_PROFILE
static void btmtk_usb_ctrl_complete(struct urb *urb) {
    BT_DBG("btmtk_usb_ctrl_complete\n");
    kfree(urb->setup_packet);
    kfree(urb->transfer_buffer);
}

static int btmtk_usb_submit_ctrl_urb(struct hci_dev *hdev, char* buf, int length) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0)
    struct btmtk_usb_data *data = hci_get_drvdata(hdev);
#else
    struct btmtk_usb_data *data = hdev->driver_data;
#endif
    struct usb_ctrlrequest *setup_packet;
    struct urb *urb;
    unsigned int pipe;
    char* send_buf;
    int err;

    BT_DBG("btmtk_usb_submit_ctrl_urb, length = %d\n", length);

    urb = usb_alloc_urb(0, GFP_ATOMIC);
    if (!urb) {
        printk("%s : usb_alloc_usb failed\n", __FUNCTION__);
        return -ENOMEM;
    }

    send_buf = kmalloc(length, GFP_ATOMIC);
    if (!send_buf) {
        printk("%s : kmalloc failed\n", __FUNCTION__);
        usb_free_urb(urb);
        return -ENOMEM;
    }
    memcpy(send_buf, buf, length);

    setup_packet = kmalloc(sizeof(*setup_packet), GFP_ATOMIC);
    if (!setup_packet) {
        printk("%s kmalloc failed2\n", __FUNCTION__);
        usb_free_urb(urb);
        kfree(send_buf);
        return -ENOMEM;
    }

    setup_packet->bRequestType = data->cmdreq_type;
    setup_packet->bRequest     = 0;
    setup_packet->wIndex       = 0;
    setup_packet->wValue       = 0;
    setup_packet->wLength      = __cpu_to_le16(length);

    pipe = usb_sndctrlpipe(data->udev, 0x00);

    usb_fill_control_urb(urb, data->udev, pipe, (void *) setup_packet,
            send_buf, length, btmtk_usb_ctrl_complete, hdev);

    usb_anchor_urb(urb, &data->tx_anchor);

    err = usb_submit_urb(urb, GFP_ATOMIC);
    if (err < 0) {
        if (err != -EPERM && err != -ENODEV)
            printk("%s : %s urb %p submission failed (%d)", __FUNCTION__, hdev->name, urb, -err);
        kfree(urb->setup_packet);
        usb_unanchor_urb(urb);
    }
    else {
        usb_mark_last_busy(data->udev);
    }

    usb_free_urb(urb);
    return err;
}

int _ascii_to_int(char buf) {
    switch ( buf ) {
        case 'a':
        case 'A':
            return 10;
        case 'b':
        case 'B':
            return 11;
        case 'c':
        case 'C':
            return 12;
        case 'd':
        case 'D':
            return 13;
        case 'e':
        case 'E':
            return 14;
        case 'f':
        case 'F':
            return 15;
        default:
            return buf-'0';
    }
}
void btmtk_usb_load_profile(struct hci_dev *hdev) {
    mm_segment_t old_fs;
    struct file *file = NULL;
    unsigned char *buf;
    unsigned char target_buf[256+4] = {0};
    int i = 0;
    int j = 4;

    old_fs = get_fs();
    set_fs(KERNEL_DS);

    file = filp_open("/etc/Wireless/RT2870STA/BT_CONFIG.dat", O_RDONLY, 0);
    if (IS_ERR(file)) {
        set_fs(old_fs);
        return;
    }

    buf = kmalloc(1280, GFP_KERNEL);
    if (!buf) {
        printk("%s : malloc error when parsing /etc/Wireless/RT2870STA/BT_CONFIG.dat, exiting...\n", __FUNCTION__);
        filp_close(file, NULL);
        set_fs(old_fs);
        return;
    }

    printk("%s : /etc/Wireless/RT2870STA/BT_CONFIG.dat exits, parse it.\n", __FUNCTION__);
    memset(buf, 0, 1280);
    file->f_op->read(file, buf, 1280, &file->f_pos);

    for ( i = 0; i < 1280; i++ ) {
        if ( buf[i] == '\r' )
            continue;
        if ( buf[i] == '\n' )
            continue;
        if ( buf[i] == 0 )
            break;
        if ( buf[i] == '0' && buf[i+1] == 'x' ) {
            i+= 1;
            continue;
        }

        {
            if ( buf[i+1] == '\r' || buf[i+1] == '\n' || buf[i+1] == 0 ) {
                target_buf[j] = _ascii_to_int(buf[i]);
                j++;
            }
            else {
                target_buf[j] = _ascii_to_int(buf[i]) << 4 | _ascii_to_int(buf[i+1]);
                j++;
                i++;
            }
        }
    }
    kfree(buf);
    filp_close(file, NULL);
    set_fs(old_fs);

    // Send to dongle
    {
        target_buf[0] = 0xc3;
        target_buf[1] = 0xfc;
        target_buf[2] = j-4+1;
        target_buf[3] = 0x01;

        printk("%s : Profile Configuration : (%d)\n", __FUNCTION__, j);
        for ( i = 0; i < j; i++ ) {
            printk("    0x%02X\n", target_buf[i]);
        }

        if ( hdev != NULL )
            btmtk_usb_submit_ctrl_urb(hdev, target_buf, j);
    }
}
#endif

#if LINUX_VERSION_CODE <= KERNEL_VERSION(3, 3, 0)
static void btmtk_usb_destruct(struct hci_dev *hdev) {
//    struct btmtk_usb_data *data = hdev->driver_data;
}
#endif

static void btmtk_usb_notify(struct hci_dev *hdev, unsigned int evt) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0)
    struct btmtk_usb_data *data = hci_get_drvdata(hdev);
#else
    struct btmtk_usb_data *data = hdev->driver_data;
#endif

    BT_DBG("%s evt %d", hdev->name, evt);

    if (hdev->conn_hash.sco_num != data->sco_num) {
        data->sco_num = hdev->conn_hash.sco_num;
        schedule_work(&data->work);
    }
}

#if SUPPORT_BT_ATE
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 13, 0)
static int btmtk_usb_ioctl(struct hci_dev *hdev, unsigned int cmd, unsigned long arg) {
#define ATE_TRIGGER     _IOW('H', 300, int)
#define ATE_PARAM_LEN    _IOW('H', 301, int)
#define ATE_PARAM_0        _IOW('H', 302, unsigned char)
#define ATE_PARAM_1        _IOW('H', 303, unsigned char)
#define ATE_PARAM_2     _IOW('H', 304, unsigned char)
#define ATE_PARAM_3     _IOW('H', 305, unsigned char)
#define ATE_PARAM_4     _IOW('H', 306, unsigned char)
#define ATE_PARAM_5     _IOW('H', 307, unsigned char)
#define ATE_PARAM_6     _IOW('H', 308, unsigned char)
#define ATE_PARAM_7     _IOW('H', 309, unsigned char)
#define ATE_PARAM_8     _IOW('H', 310, unsigned char)
#define ATE_PARAM_9     _IOW('H', 311, unsigned char)
#define ATE_PARAM_10     _IOW('H', 312, unsigned char)
#define ATE_PARAM_11    _IOW('H', 313, unsigned char)
#define ATE_PARAM_12    _IOW('H', 314, unsigned char)
#define ATE_PARAM_13    _IOW('H', 315, unsigned char)
#define ATE_PARAM_14    _IOW('H', 316, unsigned char)
#define ATE_PARAM_15    _IOW('H', 317, unsigned char)
#define ATE_PARAM_16    _IOW('H', 318, unsigned char)
#define ATE_PARAM_17    _IOW('H', 319, unsigned char)
#define ATE_READ_DRIVER_VERSION     _IOW('H', 320, int)
#define ATE_READ_ROM_PATCH_VERSION  _IOW('H', 321, int)
#define ATE_READ_FW_VERSION         _IOW('H', 322, int)
#define ATE_READ_PROB_COUNTER       _IOW('H', 323, int)
#define ATE_READ_RESET_STATUS       _IOW('H', 324, int)
#define ATE_SET_FORCE_RESET         _IOW('H', 325, int)
#define ATE_GET_DRIVER_SKIP_ALL_HCI_CMD_STATE   _IOW('H', 326, int)
// SCO
#define FORCE_SCO_ISOC_ENABLE    _IOW('H', 1, int)
#define FORCE_SCO_ISOC_DISABLE   _IOW('H', 0, int)



    #if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0)
    struct btmtk_usb_data *data = hci_get_drvdata(hdev);
    #else
    struct btmtk_usb_data *data = hdev->driver_data;
    #endif

    static char cmd_str[32] = {0};
    static int cmd_len = 0;
    unsigned long ret = 0;

    switch ( cmd ) {
        case ATE_TRIGGER:
        {
            int i;
            int should_reject_cmd = 0;

            printk("btmtk_usb : Got ATE cmd string (%d) : ", cmd_len);

            for ( i = 0; i < cmd_len; i++ ) {
                printk("%02X ", (unsigned char)cmd_str[i]);
            }
            printk("\n");

    #if SUPPORT_UNPREDICTED_HCI_CMD_FILTER
            if ( cmd_str[0] == 0xc9 &&
                 cmd_str[1] == 0xfc &&
                 cmd_str[2] == 0x03 &&
                 cmd_str[3] == 0x01 &&
                 cmd_str[4] == 0x0a &&
                 cmd_str[5] == 0x00 ) {
                should_reject_cmd = 1;
            }
    #endif

            if ( should_reject_cmd ) {
                printk("Reject it.\n");
            }
            else {
                usb_send_ate_hci_cmd(data->udev, cmd_str, cmd_len);
            }
            break;
        }
        case ATE_PARAM_LEN:
            cmd_len = arg&0xff;
            break;
        case ATE_PARAM_0:
            cmd_str[0] = (unsigned char)(arg&0xff);
            break;
        case ATE_PARAM_1:
            cmd_str[1] = (unsigned char)(arg&0xff);
            break;
        case ATE_PARAM_2:
            cmd_str[2] = (unsigned char)(arg&0xff);
            break;
        case ATE_PARAM_3:
            cmd_str[3] = (unsigned char)(arg&0xff);
            break;
        case ATE_PARAM_4:
            cmd_str[4] = (unsigned char)(arg&0xff);
            break;
        case ATE_PARAM_5:
            cmd_str[5] = (unsigned char)(arg&0xff);
            break;
        case ATE_PARAM_6:
            cmd_str[6] = (unsigned char)(arg&0xff);
            break;
        case ATE_PARAM_7:
            cmd_str[7] = (unsigned char)(arg&0xff);
            break;
        case ATE_PARAM_8:
            cmd_str[8] = (unsigned char)(arg&0xff);
            break;
        case ATE_PARAM_9:
            cmd_str[9] = (unsigned char)(arg&0xff);
            break;
        case ATE_PARAM_10:
            cmd_str[10] = (unsigned char)(arg&0xff);
            break;
        case ATE_PARAM_11:
            cmd_str[11] = (unsigned char)(arg&0xff);
            break;
        case ATE_PARAM_12:
            cmd_str[12] = (unsigned char)(arg&0xff);
            break;
        case ATE_PARAM_13:
            cmd_str[13] = (unsigned char)(arg&0xff);
            break;
        case ATE_PARAM_14:
            cmd_str[14] = (unsigned char)(arg&0xff);
            break;
        case ATE_PARAM_15:
            cmd_str[15] = (unsigned char)(arg&0xff);
            break;
        case ATE_PARAM_16:
            cmd_str[16] = (unsigned char)(arg&0xff);
            break;
        case ATE_PARAM_17:
            cmd_str[17] = (unsigned char)(arg&0xff);
            break;

        case ATE_READ_DRIVER_VERSION:
            ret = copy_to_user((char*)arg, driver_version, sizeof(driver_version));
            if ( ret ) {
                printk("%s : copy_to_user failed! (ATE_READ_DRIVER_VERSION)(%d)\n", __FUNCTION__, (int)ret);
            }
            break;
        case ATE_READ_ROM_PATCH_VERSION:
            ret = copy_to_user((char*)arg, rom_patch_version, sizeof(rom_patch_version));
            if ( ret ) {
                printk("%s : copy_to_user failed! (ATE_READ_ROM_PATCH_VERSION)(%d)\n", __FUNCTION__, (int)ret);
            }
            break;
        case ATE_READ_FW_VERSION:
            ret = copy_to_user((char*)arg, fw_version, sizeof(fw_version));
            if ( ret ) {
                printk("%s : copy_to_user failed! (ATE_READ_FW_VERSION)(%d)\n", __FUNCTION__, (int)ret);
            }
            break;
        case ATE_READ_PROB_COUNTER:
            {
                unsigned char counter_value = 0;
                if ( is_probe_done == 1 )
                    counter_value = probe_counter;
                else
                    counter_value = 0;

                printk("%s : Get driver probe counter. Result:%d\n", __FUNCTION__, counter_value);
                ret = copy_to_user((char*)arg, &counter_value, 1);
                if ( ret ) {
                    printk("%s : copy_to_user failed! (ATE_READ_PROB_COUNTER)(%d)\n", __FUNCTION__, (int)ret);
                }
            }
            break;

        case FORCE_SCO_ISOC_ENABLE:
                printk(KERN_INFO "FORCE  SCO btusb_probe sco_num = %d ", hdev->conn_hash.sco_num);
                if (!hdev->conn_hash.sco_num)
                        hdev->conn_hash.sco_num = 1;
                hdev->voice_setting = 0x0020;
                btmtk_usb_notify(hdev, HCI_NOTIFY_VOICE_SETTING);

            break;
        case FORCE_SCO_ISOC_DISABLE:
                printk(KERN_INFO "FORCE  SCO btusb_probe sco_num = %d ", hdev->conn_hash.sco_num);
                if (hdev->conn_hash.sco_num)
                        hdev->conn_hash.sco_num = 0;
                btmtk_usb_notify(hdev, HCI_NOTIFY_VOICE_SETTING);

           break;

        case ATE_READ_RESET_STATUS:
            if ( dongle_reset_done == 1 && dongle_reset_enable == 1 ) {
                dongle_reset_enable = 0;
                dongle_reset_done = 0;
                cmd_str[0] = 0;
            }
            else {
                cmd_str[0] = 1;
            }

//            printk("%s : Get driver reset status. Result:%d\n", __FUNCTION__, (int)cmd_str[0]);
            ret = copy_to_user((char*)arg, cmd_str, 1);
            if ( ret ) {
                printk("%s : copy_to_user failed! (ATE_READ_RESET_STATUS)(%d)\n", __FUNCTION__, (int)ret);
            }
            break;

        case ATE_SET_FORCE_RESET:
            printk("%s : Force Reset Dongle!\n", __FUNCTION__);
            btmtk_usb_toggle_rst_pin();
            break;

        case ATE_GET_DRIVER_SKIP_ALL_HCI_CMD_STATE:
            printk("%s : Get driver skip all hci cmd state. Result:%d\n", __FUNCTION__, is_in_skip_all_cmd_state);
            cmd_str[0] = is_in_skip_all_cmd_state;
            ret = copy_to_user((char*)arg, cmd_str, 1);
            if ( ret ) {
                printk("%s : copy_to_user failed! (ATE_GET_DRIVER_SKIP_ALL_HCI_CMD_STATE)(%d)\n", __FUNCTION__, (int)ret);
            }
            break;

        default:
            break;
    }

    return 0;
}
#endif
#elif SUPPORT_BT_RECOVER
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 13, 0)
static int btmtk_usb_ioctl(struct hci_dev *hdev, unsigned int cmd, unsigned long arg) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0)
        struct btmtk_usb_data *data = hci_get_drvdata(hdev);
#else
        struct btmtk_usb_data *data = hdev->driver_data;
#endif
    // Debug cmd
    #define TRIGGER_HW_RESET    _IOW('H', 200, int)
#if SUPPORT_BT_WAKEUP
    #define ENABLE_BT_WAKEUP    _IOW('H', 3, int)
    #define DISABLE_BT_WAKEUP   _IOW('H', 2, int)
#endif
    #define TRIGGER_CORE_DUMP   _IOW('H', 7, int)
    printk("%s : enter bt kernel driver!\n", __FUNCTION__);
    switch ( cmd ) {
        case TRIGGER_HW_RESET:
            printk("%s : Get TRIGGER_HW_RESET cmd to trigger gpio reset !\n", __FUNCTION__);
            btmtk_usb_toggle_rst_pin();
            break;
        case TRIGGER_CORE_DUMP:
            printk("%s : Get TRIGGER_CORE_DUMP cmd to trigger core dump !\n", __FUNCTION__);
            //btmtk_usb_send_core_dump_cmd(data->udev);
#if SUPPORT_BT_WAKEUP
        case ENABLE_BT_WAKEUP:
            printk(KERN_INFO "ENABLE_BT_WAKEUP\n");
            enable_bt_wakeup = 1;
            break;

        case DISABLE_BT_WAKEUP:
            printk(KERN_INFO "DISABLE_BT_WAKEUP\n");
            enable_bt_wakeup = 0;
            break;
#endif
        default :
            printk("%s : Get no define cmd !\n", __FUNCTION__);
            break;
    }

    return 0;
}
#endif
#endif  // SUPPORT_BT_ATE

static inline int __set_isoc_interface(struct hci_dev *hdev, int altsetting) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0)
    struct btmtk_usb_data *data = hci_get_drvdata(hdev);
#else
    struct btmtk_usb_data *data = hdev->driver_data;
#endif
    struct usb_interface *intf = data->isoc;
    struct usb_endpoint_descriptor *ep_desc;
    int i, err;

    if (!data->isoc)
        return -ENODEV;

    err = usb_set_interface(data->udev, 1, altsetting);
    if (err < 0) {
        printk("%s setting interface failed (%d)", hdev->name, -err);
        return err;
    }

    data->isoc_altsetting = altsetting;

    data->isoc_tx_ep = NULL;
    data->isoc_rx_ep = NULL;

    for (i = 0; i < intf->cur_altsetting->desc.bNumEndpoints; i++) {
        ep_desc = &intf->cur_altsetting->endpoint[i].desc;

        if (!data->isoc_tx_ep && usb_endpoint_is_isoc_out(ep_desc)) {
            data->isoc_tx_ep = ep_desc;
            continue;
        }

        if (!data->isoc_rx_ep && usb_endpoint_is_isoc_in(ep_desc)) {
            data->isoc_rx_ep = ep_desc;
            continue;
        }
    }

    if (!data->isoc_tx_ep || !data->isoc_rx_ep) {
        printk("%s invalid SCO descriptors", hdev->name);
        return -ENODEV;
    }

    return 0;
}

static void btmtk_usb_work(struct work_struct *work) {
    struct btmtk_usb_data *data = container_of(work, struct btmtk_usb_data, work);
    struct hci_dev *hdev = data->hdev;
    int new_alts;
    int err;

    BT_DBG("%s\n", __FUNCTION__);

    if (hdev->conn_hash.sco_num > 0) {
        if (!test_bit(BTUSB_DID_ISO_RESUME, &data->flags)) {
            err = usb_autopm_get_interface(data->isoc ? data->isoc : data->intf);
            if (err < 0) {
                clear_bit(BTUSB_ISOC_RUNNING, &data->flags);
                usb_kill_anchored_urbs(&data->isoc_anchor);
                return;
            }

            set_bit(BTUSB_DID_ISO_RESUME, &data->flags);
        }

        if (hdev->voice_setting & 0x0020) {
            static const int alts[3] = { 2, 4, 5 };
            new_alts = alts[hdev->conn_hash.sco_num - 1];
        }
        else if (hdev->voice_setting & 0x2000) {
            new_alts = 4;
        }
        else if (hdev->voice_setting & 0x0200) {
            new_alts = 6;    // Alt setting for WBS SCO support
        }
        else {
            new_alts = hdev->conn_hash.sco_num;
        }

        if (data->isoc_altsetting != new_alts) {
            clear_bit(BTUSB_ISOC_RUNNING, &data->flags);
            usb_kill_anchored_urbs(&data->isoc_anchor);

            if (__set_isoc_interface(hdev, new_alts) < 0)
                return;
        }

        if (!test_and_set_bit(BTUSB_ISOC_RUNNING, &data->flags)) {
            if (btmtk_usb_submit_isoc_in_urb(hdev, GFP_KERNEL) < 0)
                clear_bit(BTUSB_ISOC_RUNNING, &data->flags);
            else
                btmtk_usb_submit_isoc_in_urb(hdev, GFP_KERNEL);
        }
    }
    else {
        clear_bit(BTUSB_ISOC_RUNNING, &data->flags);
        usb_kill_anchored_urbs(&data->isoc_anchor);

        __set_isoc_interface(hdev, 0);

        if (test_and_clear_bit(BTUSB_DID_ISO_RESUME, &data->flags))
             usb_autopm_put_interface(data->isoc ? data->isoc : data->intf);
    }
}

static void btmtk_usb_waker(struct work_struct *work) {
    struct btmtk_usb_data *data = container_of(work, struct btmtk_usb_data, waker);
    int err;

    err = usb_autopm_get_interface(data->intf);

    if (err < 0)
        return;

    usb_autopm_put_interface(data->intf);
}

static struct btmtk_usb_data *btmtk_usb_data_alloc(struct usb_interface *intf, int *err_code) {
    struct btmtk_usb_data *data = NULL;
    struct usb_endpoint_descriptor *ep_desc;
    int i;
    data = kzalloc(sizeof(*data), GFP_KERNEL);

    if (!data) {
        printk("[ERR] kzalloc failed !\n");
        printk("btmtk_usb_probe end Error 2\n");
        btmtk_usb_toggle_rst_pin();

        *err_code = -ENOMEM;
        return NULL;
    }

    // memset(data, 0, sizeof(struct btmtk_usb_data));    // init data  add by steve wang

    for (i = 0; i < intf->cur_altsetting->desc.bNumEndpoints; i++) {
        ep_desc = &intf->cur_altsetting->endpoint[i].desc;

        if (!data->intr_ep && usb_endpoint_is_int_in(ep_desc)) {
            data->intr_ep = ep_desc;
            continue;
        }

        if (!data->bulk_tx_ep && usb_endpoint_is_bulk_out(ep_desc)) {
            data->bulk_tx_ep = ep_desc;
            continue;
        }

        // BT_REDUCE_EP2_POLLING_INTERVAL_BY_INTR_TRANSFER
        if ( !data->bulk_rx_ep && (usb_endpoint_is_bulk_in(ep_desc) || usb_endpoint_is_int_in(ep_desc)) ) {
            data->bulk_rx_ep = ep_desc;
            if ( usb_endpoint_is_int_in(ep_desc) )
                printk("%s : EP2 is INTR_IN\n", __FUNCTION__);
            continue;
        }
    }

    if (!data->intr_ep || !data->bulk_tx_ep || !data->bulk_rx_ep) {
        kfree(data);
        printk("btmtk_usb_probe end Error 3 \n");
        btmtk_usb_toggle_rst_pin();

        *err_code = -ENODEV;
        return NULL;
    }

    data->cmdreq_type = USB_TYPE_CLASS;
    data->udev = interface_to_usbdev(intf);
    g_udev = data->udev;
    data->intf = intf;

    spin_lock_init(&data->lock);
    INIT_WORK(&data->work, btmtk_usb_work);
    INIT_WORK(&data->waker, btmtk_usb_waker);
    spin_lock_init(&data->txlock);

    init_usb_anchor(&data->tx_anchor);
    init_usb_anchor(&data->intr_anchor);
    init_usb_anchor(&data->bulk_anchor);
    init_usb_anchor(&data->isoc_anchor);
    init_usb_anchor(&data->deferred);

    data->io_buf = kmalloc(256, GFP_KERNEL);    // GFP_ATOMIC
    if (data->io_buf == NULL) {
        kfree(data);

        printk("btmtk_usb_probe end Error 3_02\n");

        *err_code = -ENOMEM;
        return NULL;
    }

    btmtk_usb_switch_iobase(data, WLAN);

    btmtk_usb_cap_init(data);

    return data;
}

static void btmtk_usb_data_free(struct btmtk_usb_data *data) {
    if (data != NULL) {
#if BT_ROM_PATCH_FROM_BIN
        if (LOAD_CODE_METHOD == BIN_FILE_METHOD) {
            if (data->rom_patch != NULL) {
                vfree(data->rom_patch);
                data->rom_patch = NULL;
            }

            if (data->rom_patch_bin_file_name !=  NULL) {
                kfree(data->rom_patch_bin_file_name);
                data->rom_patch_bin_file_name = NULL;
            }

            if (data->fw_image != NULL) {
                kfree(data->fw_image);
                data->fw_image = NULL;
            }
        }
#endif
        if (data->io_buf != NULL) {
            kfree(data->io_buf);
            data->io_buf = NULL;
        }

        kfree(data);
        data = NULL;
    }
}


#if SUPPORT_STPBT_DEVICE
static int btmtk_usb_fops_open(struct inode *inode, struct file *file) {
    int ret = 0;
    printk("%s:Mediatek Bluetooth USB driver ver %s\n", __func__, VERSION);


    printk("%s: major %d minor %d (pid %d), probe counter:%d\n", __func__,
           imajor(inode), iminor(inode), current->pid, probe_counter);
    if (current->pid == 1) {
        return 0;
    }

    if (isbtready == 0) {
        printk("%s:bluetooth driver is not ready\n", __func__);
        return -ENODEV;
    }

    return 0;
}

struct file_operations BT_fops = {
    .open = btmtk_usb_fops_open,
    .release = 0,
    .read = 0,
    .write = 0,
    .poll = 0,
    .unlocked_ioctl = 0
};

static void create_stpbt_device(void) {
    dev_t devID = MKDEV(BT_major, 0);
    int ret = 0;
    int cdevErr = 0;
    int major = 0;
    printk("%s\n", __FUNCTION__);

    ret = alloc_chrdev_region(&devID, 0, 1, BT_DRIVER_NAME);
    if (ret) {
        printk("fail to allocate chrdev\n");
        return;
    }

    BT_major = major = MAJOR(devID);
    printk("major number:%d", BT_major);

    cdev_init(&BT_cdev, &BT_fops);
    BT_cdev.owner = THIS_MODULE;

    cdevErr = cdev_add(&BT_cdev, devID, BT_devs);

    pBTClass = class_create(THIS_MODULE, BT_DRIVER_NAME);
    if (IS_ERR(pBTClass)) {
        printk("class create fail, error code(%ld)\n", PTR_ERR(pBTClass));
        return;
    }

    pBTDev = device_create(pBTClass, NULL, devID, NULL, BT_NODE);
    if (IS_ERR(pBTDev)) {
        printk("device create fail, error code(%ld)\n", PTR_ERR(pBTDev));
        return;
    }
}

static void destory_stpbt_device(void) {
    dev_t dev = MKDEV(BT_major, 0);
    printk("%s\n", __FUNCTION__);

    if (pBTDev) {
        device_destroy(pBTClass, dev);
        pBTDev = NULL;
    }

    if (pBTClass) {
        class_destroy(pBTClass);
        pBTClass = NULL;
    }

    cdev_del(&BT_cdev);
    unregister_chrdev_region(dev, 1);
}
#endif

static int btmtk_usb_probe(struct usb_interface *intf,
                            const struct usb_device_id *id) {
    struct btmtk_usb_data *data;
    int err;
    struct hci_dev *hdev;

    btmtk_usb_dewarning();
    //printk(" =========================================== \n");
    //printk("btmtk_usb driver ver %s \n", VERSION);
#ifdef BT_STACK_BD
    printk("BT_STACK_BD\n");
#endif
#ifdef BT_STACK_BA
    printk("BT_STACK_BA\n");
#endif
#ifdef BT_RC_VENDOR_N0
    printk("BT_RC_VENDOR_N0\n");
#endif
#ifdef BT_RC_VENDOR_T0
    printk("BT_RC_VENDOR_T0\n");
#endif
#ifdef BT_RC_VENDOR_T1
    printk("BT_RC_VENDOR_T1\n");
#endif
    //printk(" =========================================== \n");
    memset(driver_version, 0, sizeof(driver_version));
    memcpy(driver_version, VERSION, sizeof(VERSION));

    //printk("%s : begin\n", __FUNCTION__);

    if ( dongle_reset_enable == 1 && dongle_reset_done == 0 ) {
        printk("%s : triggered by dongle reset!\n", __FUNCTION__);
        dongle_reset_done = 1;
    }

    /* interface numbers are hardcoded in the spec */
    if (intf->cur_altsetting->desc.bInterfaceNumber != 0) {
        printk("%s : interface number != 0(%d), Skip it\n", __FUNCTION__, intf->cur_altsetting->desc.bInterfaceNumber);
        printk("%s end\n\n\n", __FUNCTION__);
        return -ENODEV;
    }

    btmtk_usb_hci_dmp_init();
    data = btmtk_usb_data_alloc(intf, &err);

    if (data == NULL)
        return err;

    if (data->need_load_rom_patch) {
        err = btmtk_usb_load_rom_patch(data);

        if (err < 0) {
            btmtk_usb_data_free(data);
            data = NULL;
            printk("%s : end Error 4\n", __FUNCTION__);
            btmtk_usb_toggle_rst_pin();
            return err;
        }
    }

    if (data->need_load_fw) {
        err = btmtk_usb_load_fw(data);

        if (err < 0) {
            btmtk_usb_data_free(data);
            data = NULL;
            printk("%s : end Error 5\n", __FUNCTION__);
            btmtk_usb_toggle_rst_pin();
            return err;
        }
    }

#if SUPPORT_KEEP_HCI_DEV_INSTANCE_WHEN_DISCONNECT
    if ( g_hdev == NULL ) {
        hdev = hci_alloc_dev();
        g_hdev = hdev;
        printk("%s : create new hci instance\n", __FUNCTION__);
    }
    else {
        printk("%s : use old hci instance\n", __FUNCTION__);
        hdev = g_hdev;

#if SUPPORT_SEND_DUMMY_ERROR_HCI_EVENT_AFTER_RESUME
        if ( is_in_skip_all_cmd_state ) {
            btmtk_usb_send_dummy_error_hci_event(NULL, hdev);
        }
#endif
    }
#else
    hdev = hci_alloc_dev();
#endif

    if (!hdev) {
        btmtk_usb_data_free(data);
        data = NULL;
        printk("%s : end Error 6\n", __FUNCTION__);
        btmtk_usb_toggle_rst_pin();
        return -ENOMEM;
    }

    hdev->bus = HCI_USB;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0)
    hci_set_drvdata(hdev, data);
#else
    hdev->driver_data = data;
#endif

    data->hdev = hdev;

    SET_HCIDEV_DEV(hdev, &intf->dev);

    hdev->open     = btmtk_usb_open;
    hdev->close    = btmtk_usb_close;
    hdev->flush    = btmtk_usb_flush;
    hdev->send     = btmtk_usb_send_frame;
#if LINUX_VERSION_CODE <= KERNEL_VERSION(3, 3, 0)
    hdev->destruct = btmtk_usb_destruct;
#endif
    hdev->notify   = btmtk_usb_notify;
#if (SUPPORT_BT_ATE || SUPPORT_BT_RECOVER)
    #if LINUX_VERSION_CODE < KERNEL_VERSION(3, 13, 0)
    hdev->ioctl    = btmtk_usb_ioctl;
    #endif
#endif

    /* Interface numbers are hardcoded in the specification */
    data->isoc = usb_ifnum_to_if(data->udev, 1);

    if ( data->rom_patch_bin_file_name ) {
        kfree(data->rom_patch_bin_file_name);
        data->rom_patch_bin_file_name = NULL;
    }

    if (data->isoc) {
        err = usb_driver_claim_interface(&btmtk_usb_driver,
                            data->isoc, data);
        if (err < 0) {
            hci_free_dev(hdev);
            btmtk_usb_data_free(data);
            data = NULL;
            printk("%s : end Error 7\n", __FUNCTION__);
            btmtk_usb_toggle_rst_pin();
            return err;
        }
    }

#if SUPPORT_KEEP_HCI_DEV_INSTANCE_WHEN_DISCONNECT
    if ( g_hdev_registered ) {
        printk("%s : trigger btmtk_usb_open for keep hci dev instance\n", __FUNCTION__);
        btmtk_usb_open(hdev);
    }
    else {
#endif
        err = hci_register_dev(hdev);
        if (err < 0) {
            hci_free_dev(hdev);
            btmtk_usb_data_free(data);
            data = NULL;
            printk("%s : end Error 8\n", __FUNCTION__);
            btmtk_usb_toggle_rst_pin();
            return err;
        }
#if SUPPORT_KEEP_HCI_DEV_INSTANCE_WHEN_DISCONNECT
        g_hdev_registered = 1;
    }
#endif

#if SUPPORT_DEFAULT_TX_POWER_LEVEL_5
    btmtk_usb_send_tx_power_level_5_cmd(data->udev);
#endif

    usb_set_intfdata(intf, data);

    probe_counter++;
    if ( probe_counter == 0 )
        probe_counter = 1;

    is_probe_done = 1;

    printk("%s : probe_counter = %d\n", __FUNCTION__, probe_counter);
    printk("%s end\n", __FUNCTION__);

#if SUPPORT_STPBT_DEVICE
    create_stpbt_device();
    isbtready = 1;
#endif

    return 0;
}

static void btmtk_usb_disconnect(struct usb_interface *intf) {
	struct btmtk_usb_data *data = usb_get_intfdata(intf);
	struct hci_dev *hdev;
	
	printk("%s begin\n", __FUNCTION__);

#if SUPPORT_A2DP_LATENCY_MEASUREMENT
	/*if (!IS_ERR(bt_timestamp_file))
    {
    	filp_close(bt_timestamp_file, NULL);
        set_fs(timestamp_fs);
		bt_timestamp_file = NULL;
        printk("close bt driver timestamp file (/tmp/bt_driver_A2DP_source_timestamp.log).\n");
    }
	else
	{
        printk("no bt driver timestamp file (/tmp/bt_driver_A2DP_source_timestamp.log).\n");
	}*/
#endif

    if (!data) {
        printk("%s end 1\n", __FUNCTION__);
        return;
    }

#if SUPPORT_STPBT_DEVICE
    destory_stpbt_device();
    isbtready = 0;
#endif

    hdev = data->hdev;

    usb_set_intfdata(data->intf, NULL);

    if (data->isoc)
        usb_set_intfdata(data->isoc, NULL);
#if SUPPORT_KEEP_HCI_DEV_INSTANCE_WHEN_DISCONNECT
    btmtk_usb_close(hdev);
#else
    hci_unregister_dev(hdev);
#endif

    if (intf == data->isoc)
        usb_driver_release_interface(&btmtk_usb_driver, data->intf);
    else if (data->isoc)
        usb_driver_release_interface(&btmtk_usb_driver, data->isoc);

#if SUPPORT_KEEP_HCI_DEV_INSTANCE_WHEN_DISCONNECT
#else
    hci_free_dev(hdev);
#endif

    g_udev = NULL;

    btmtk_usb_data_free(data);
    data = NULL;
    printk("%s end\n", __FUNCTION__);
}

#ifdef CONFIG_PM
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 2, 0)
//  #define PMSG_IS_AUTO(msg)       (((msg).event & PM_EVENT_AUTO) != 0)
#endif
#if SUPPORT_BT_WAKEUP
static void btmtk_usb_common_control_transfer_complete(struct urb *urb) {
    if ( urb ) {
        if ( urb->setup_packet )
            kfree(urb->setup_packet);
        if ( urb->transfer_buffer )
            kfree(urb->transfer_buffer);
        urb->setup_packet = NULL;
        urb->transfer_buffer = NULL;
    }
}

static int btmtk_usb_common_control_transfer(struct usb_device *udev,
                                                __u8 bRequestType,
                                                __u8 bRequest,
                                                __le16 wValue,
                                                __le16 wIndex,
                                                unsigned char* buf,
                                                int len) {
    struct urb *urb;
    struct usb_ctrlrequest    *setup_packet;
    unsigned int pipe;
    int err;
    unsigned char    *hci_cmd = NULL;
    int i;

    urb = usb_alloc_urb(0, GFP_ATOMIC);
    if (!urb) {
        printk("%s: allocate usb urb failed!\n",
                __FUNCTION__);
        return -ENOMEM;
    }

    setup_packet = kmalloc(sizeof(struct usb_ctrlrequest), GFP_ATOMIC);
    if (!setup_packet) {
        usb_free_urb(urb);
        printk("%s: allocate class request failed!\n",
                __FUNCTION__);
        return -ENOMEM;
    }

    if ( len != 0 && buf != NULL ) {
        hci_cmd = kmalloc(len, GFP_ATOMIC);
        if (!hci_cmd)  {
            usb_free_urb(urb);
            kfree(setup_packet);
            printk("%s: allocate hci_cmd failed!\n", __FUNCTION__);
            return -ENOMEM;
        }

        memset(hci_cmd, 0, len);
        for ( i = 0; i < len; i++ ) {
            hci_cmd[i] = buf[i];
        }
    }
    else {
        len = 0;
        buf = NULL;
    }


    setup_packet->bRequestType = bRequestType;
    setup_packet->bRequest = bRequest;
    setup_packet->wValue = wValue;
    setup_packet->wIndex = wIndex;
    setup_packet->wLength = len;

    pipe = usb_sndctrlpipe(udev, 0x0);

    usb_fill_control_urb(urb, udev, pipe, (void *)setup_packet,
            hci_cmd, len,
            btmtk_usb_common_control_transfer_complete, udev);

    err = usb_submit_urb(urb, GFP_ATOMIC);
    if (err < 0) {
        kfree(urb->setup_packet);
        kfree(urb->transfer_buffer);
    }
    else {
        usb_mark_last_busy(udev);
    }

    usb_free_urb(urb);
    return err;
}

static void btmtk_usb_set_feature_remote_wake_up(struct btmtk_usb_data *data) {
    unsigned char hci_cmd[] = {0x1a, 0x0c, 0x01, 0x02};
    btmtk_usb_common_control_transfer(data->udev,
        USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_DEVICE,
        0,
        0,
        0,
        hci_cmd,
        4);
    msleep(30);
    btmtk_usb_common_control_transfer(data->udev,
        USB_DIR_OUT | USB_TYPE_STANDARD | USB_RECIP_DEVICE,
        USB_REQ_SET_FEATURE,
        USB_DEVICE_REMOTE_WAKEUP,
        0,
        NULL,
        0);
    msleep(30);
}

static void btmtk_usb_clear_feature_remote_wake_up(struct btmtk_usb_data *data) {
    btmtk_usb_common_control_transfer(data->udev,
        USB_DIR_OUT | USB_TYPE_STANDARD | USB_RECIP_DEVICE,
        USB_REQ_CLEAR_FEATURE,
        USB_DEVICE_REMOTE_WAKEUP,
        0,
        NULL,
        0);
}
#endif

static int btmtk_usb_suspend(struct usb_interface *intf, pm_message_t message) {
    struct btmtk_usb_data *data = usb_get_intfdata(intf);

    printk("%s begin\n", __FUNCTION__);

    if (data->suspend_count++) {
        printk("%s end1\n", __FUNCTION__);
        return 0;
    }

    is_probe_done = 0;

#if SUPPORT_SEND_DUMMY_ERROR_HCI_EVENT_AFTER_RESUME
    is_in_skip_all_cmd_state = 1;
#endif

/*    mask for konka project suggestion from liuyuan
#if !ENABLE_BT_LOW_POWER_MODE
    // Enable BT Low Power
    btmtk_usb_send_enable_low_power_cmd(data->udev);
    // for WoBLE/WoW low power
    btmtk_usb_send_hci_set_ce_cmd(data->udev);
#endif
*/

#if BT_SEND_HCI_CMD_BEFORE_SUSPEND
    btmtk_usb_send_hci_suspend_cmd(data->udev);
#endif

#if SUPPORT_BT_WAKEUP
    if (enable_bt_wakeup == 1)
        btmtk_usb_set_feature_remote_wake_up(data);
#endif

    spin_lock_irq(&data->txlock);

//    if (!(PMSG_IS_AUTO(message) && data->tx_in_flight)) {
    if (!(((message.event & PM_EVENT_AUTO) != 0) && data->tx_in_flight)) {
        set_bit(BTUSB_SUSPENDING, &data->flags);
        spin_unlock_irq(&data->txlock);
    }
    else {
        spin_unlock_irq(&data->txlock);
        data->suspend_count--;
        printk("%s error\n", __FUNCTION__);
        return -EBUSY;
    }

    cancel_work_sync(&data->work);

    btmtk_usb_stop_traffic(data);
    usb_kill_anchored_urbs(&data->tx_anchor);
    printk("%s end\n", __FUNCTION__);
    return 0;
}

static void play_deferred(struct btmtk_usb_data *data) {
    struct urb *urb;
    int err;

    while ( (urb = usb_get_from_anchor(&data->deferred)) ) {
        err = usb_submit_urb(urb, GFP_ATOMIC);
        if (err < 0)
            break;

        data->tx_in_flight++;
    }

    usb_scuttle_anchored_urbs(&data->deferred);
}

static int btmtk_usb_resume(struct usb_interface *intf) {
    struct btmtk_usb_data *data = usb_get_intfdata(intf);
    struct hci_dev *hdev = data->hdev;
    int err = 0;

    printk("%s begin\n", __FUNCTION__);

    if (--data->suspend_count)
        return 0;
#if SUPPORT_BT_WAKEUP
    if (enable_bt_wakeup == 1)
        btmtk_usb_clear_feature_remote_wake_up(data);
#endif

    if (!test_bit(HCI_RUNNING, &hdev->flags))
        goto done;

    if (test_bit(BTUSB_INTR_RUNNING, &data->flags)) {
        err = btmtk_usb_submit_intr_urb(hdev, GFP_NOIO);
        if (err < 0) {
            clear_bit(BTUSB_INTR_RUNNING, &data->flags);
            goto failed;
        }
    }

    if (test_bit(BTUSB_BULK_RUNNING, &data->flags)) {
        err = btmtk_usb_submit_bulk_in_urb(hdev, GFP_NOIO);
        if (err < 0) {
            clear_bit(BTUSB_BULK_RUNNING, &data->flags);
            goto failed;
        }

        btmtk_usb_submit_bulk_in_urb(hdev, GFP_NOIO);
    }

    if (test_bit(BTUSB_ISOC_RUNNING, &data->flags)) {
        if (btmtk_usb_submit_isoc_in_urb(hdev, GFP_NOIO) < 0)
            clear_bit(BTUSB_ISOC_RUNNING, &data->flags);
        else
            btmtk_usb_submit_isoc_in_urb(hdev, GFP_NOIO);
    }

    spin_lock_irq(&data->txlock);
    play_deferred(data);
    clear_bit(BTUSB_SUSPENDING, &data->flags);
    spin_unlock_irq(&data->txlock);
    schedule_work(&data->work);
    printk("%s end\n", __FUNCTION__);
    return 0;

failed:
    usb_scuttle_anchored_urbs(&data->deferred);
done:
    spin_lock_irq(&data->txlock);
    clear_bit(BTUSB_SUSPENDING, &data->flags);
    spin_unlock_irq(&data->txlock);

    return err;
}
#endif

static struct usb_device_id btmtk_usb_table_linux[] = {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 6, 0)
	{ USB_DEVICE_AND_INTERFACE_INFO(0x0e8d, 0x7662, 0xe0, 0x01, 0x01), .bInterfaceNumber = 0 },
	{ USB_DEVICE_AND_INTERFACE_INFO(0x0e8d, 0x7632, 0xe0, 0x01, 0x01), .bInterfaceNumber = 0 },
	{ USB_DEVICE_AND_INTERFACE_INFO(0x0e8d, 0x76a0, 0xe0, 0x01, 0x01), .bInterfaceNumber = 0 },
    { USB_DEVICE_AND_INTERFACE_INFO(0x0e8d, 0x76a1, 0xe0, 0x01, 0x01), .bInterfaceNumber = 0 },
#else
	{ USB_DEVICE_AND_INTERFACE_INFO(0x0e8d, 0x7662, 0xe0, 0x01, 0x01)},
	{ USB_DEVICE_AND_INTERFACE_INFO(0x0e8d, 0x7632, 0xe0, 0x01, 0x01)},
	{ USB_DEVICE_AND_INTERFACE_INFO(0x0e8d, 0x76a0, 0xe0, 0x01, 0x01)},
    { USB_DEVICE_AND_INTERFACE_INFO(0x0e8d, 0x76a1, 0xe0, 0x01, 0x01)},
#endif
	{ }
};

static struct usb_driver btmtk_usb_driver = {
	.name		= "btmtk_usb",
	.probe		= btmtk_usb_probe,
	.disconnect	= btmtk_usb_disconnect,
#ifdef CONFIG_PM
	.suspend	= btmtk_usb_suspend,
	.resume		= btmtk_usb_resume,
#endif
    .id_table    = btmtk_usb_table_linux,
	.supports_autosuspend = 1,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 5, 0)
	.disable_hub_initiated_lpm = 1,
#endif
};

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 3, 0)
module_usb_driver(btmtk_usb_driver);
#else
static int __init btmtk_usb_init(void) {
    BT_INFO("btmtk usb driver ver %s", VERSION);

    return usb_register(&btmtk_usb_driver);
}

static void __exit btmtk_usb_exit(void) {
    usb_deregister(&btmtk_usb_driver);
}

module_init(btmtk_usb_init);
module_exit(btmtk_usb_exit);
#endif


// ==============================================================================
// API exported to other module
// ==============================================================================
void btmtk_usb_trigger_core_dump(void) {
#if 0
    btmtk_usb_send_core_dump_cmd(g_udev);
#else
    printk("%s : Invoked by some other module (WiFi).\n", __FUNCTION__);
    btmtk_usb_toggle_rst_pin();
#endif
}

void btmtk_usb_set_rom_patch_version_string(unsigned char* buf, unsigned int buf_len) {
    int copy_len = buf_len;
    if ( copy_len > sizeof(rom_patch_version) ) {
        copy_len = sizeof(rom_patch_version);
    }

    printk("%s : Invoked by some other module.\n", __FUNCTION__);
    if ( copy_len != 0 ) {
        memset(rom_patch_version, 0, sizeof(rom_patch_version));
        memcpy(rom_patch_version, buf, copy_len);
        printk("%s : rom patch version string = %s\n", __FUNCTION__, rom_patch_version);
    }
}

void btmtk_usb_enter_woble_state(void) {
    printk("%s : Invoked by some other module. \n", __FUNCTION__);
    if ( g_udev )
        btmtk_usb_send_hci_suspend_cmd(g_udev);
}

MODULE_DESCRIPTION("Mediatek Bluetooth USB driver ver " VERSION);
MODULE_VERSION(VERSION);
MODULE_LICENSE("GPL");
MODULE_FIRMWARE("mt7662_patch_e1_hdr.bin");
MODULE_FIRMWARE("mt7662_patch_e3_hdr.bin");

