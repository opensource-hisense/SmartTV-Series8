/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 *
 * MediaTek Inc. (C) 2010. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
 * AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek Software")
 * have been modified by MediaTek Inc. All revisions are subject to any receiver's
 * applicable license agreements with MediaTek Inc.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/timer.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/earlysuspend.h>
#include <linux/slab.h>

#include <linux/device.h>
#include <linux/errno.h>
#include <asm/atomic.h>
#include <asm/uaccess.h>

#define CC_KEY_BUFFER 
#ifdef CC_KEY_BUFFER
#define HID_KEY_BUF_NUM 40
typedef enum
{
    S_KEY_RELEASE =  0,
    S_KEY_PRESS
}HID_KEY_STATE_E;

typedef struct _HID_Key 
{
    bool	valid;
    unsigned int keycode ;
    HID_KEY_STATE_E state;
} HID_KEY_T ;

HID_KEY_T key_buf[HID_KEY_BUF_NUM] ;
unsigned int key_buf_idx = 0 ;
bool key_buf_en  = 1 ;
#endif
static unsigned short hid_keycode[256] = {
	  KEY_RESERVED,  
	  BTN_LEFT,
	  KEY_BACK,
	  KEY_RESERVED,
	  KEY_A,
	  KEY_B, 
	  KEY_C, 
	  KEY_D, 
	  KEY_E, 
	  KEY_F, 
	  KEY_G, 
	  KEY_H, 
	  KEY_I, 
	  KEY_J, 
	  KEY_K, 
	  KEY_L,
	  KEY_M, 
	  KEY_N, 
	  KEY_O, 
	  KEY_P, 
	  KEY_Q, 
	  KEY_R, 
	  KEY_S, 
	  KEY_T, 
	  KEY_U, 
	  KEY_V, 
	  KEY_W, 
	  KEY_X, 
	  KEY_Y, 
	  KEY_Z,  
	  KEY_1,  
	  KEY_2,
	  KEY_3,  
	  KEY_4,  
	  KEY_5,  
	  KEY_6,  
	  KEY_7,  
	  KEY_8, 
	  KEY_9, 
	  KEY_0, 
	  KEY_ENTER,
	  KEY_BACK,
	  KEY_BACKSPACE,
	  KEY_TAB,
	  KEY_SPACE,
	  KEY_MINUS,
	  KEY_EQUAL,
	  KEY_LEFTBRACE,
	  KEY_RIGHTBRACE,
	  KEY_BACKSLASH,
	  KEY_BACKSLASH,
	  KEY_SEMICOLON,
	  KEY_APOSTROPHE,
	  KEY_GRAVE,
	  KEY_COMMA,
	  KEY_DOT,
	  KEY_SLASH,
	  KEY_CAPSLOCK,
	  KEY_F1,
	  KEY_F2,
	  KEY_F3,
	  KEY_F4,
	  KEY_F5,
	  KEY_F6,
	  KEY_F7,
	  KEY_F8,
	  KEY_F9,
	  KEY_F10,
	  KEY_F11,
	  KEY_F12,
	  KEY_SYSRQ,
	  KEY_SCROLLLOCK,
	  KEY_PAUSE,
	  KEY_INSERT,
	  KEY_HOME,
	  KEY_PAGEUP,
	  KEY_DELETE,
	  KEY_END,
	  KEY_PAGEDOWN,
	  KEY_RIGHT,
	  KEY_LEFT,
	  KEY_DOWN,
	  KEY_UP,
	  KEY_NUMLOCK,
	  KEY_KPSLASH,
	  KEY_KPASTERISK,
	  KEY_KPMINUS,
	  KEY_KPPLUS, 
	  KEY_KPENTER,
	  KEY_KP1,
	  KEY_KP2,
	  KEY_KP3,
	  KEY_KP4,
	  KEY_KP5,
	  KEY_KP6,
	  KEY_KP7,
	  KEY_KP8,
	  KEY_KP9,
	  KEY_KP0,
	  KEY_KPDOT,
	  KEY_102ND,
	  KEY_COMPOSE,
	  KEY_POWER,
	  KEY_KPEQUAL,
	  KEY_F13,
	  KEY_F14,
	  KEY_F15,
	  KEY_F16,
	  KEY_F17,
	  KEY_F18,
	  KEY_F19,
	  KEY_F20,
	  KEY_F21,
	  KEY_F22,
	  KEY_F23,
	  KEY_F24,
	  KEY_OPEN,
	  KEY_HELP,
	  KEY_PROPS,
	  KEY_FRONT,
	  KEY_STOP,
	  KEY_AGAIN,
	  KEY_UNDO,
	  KEY_CUT,
	  KEY_COPY,
	  KEY_PASTE,
	  KEY_FIND,
	  KEY_MUTE,
	  KEY_VOLUMEUP,
	  KEY_VOLUMEDOWN,
	  KEY_UNKNOWN,
	  KEY_UNKNOWN,
	  KEY_UNKNOWN,
	  KEY_KPCOMMA,
	  KEY_UNKNOWN, 
	  KEY_RO, 
	  KEY_KATAKANAHIRAGANA,
	  KEY_YEN,
	  KEY_HENKAN,
	  KEY_MUHENKAN,
	  KEY_KPJPCOMMA,
	  KEY_UNKNOWN,
	  KEY_UNKNOWN,
	  KEY_UNKNOWN,
	  KEY_HANGEUL,
	  KEY_HANJA,
	  KEY_KATAKANA,
	  KEY_HIRAGANA,
	  KEY_ZENKAKUHANKAKU,
	  KEY_UNKNOWN,
	  KEY_UNKNOWN,
	  KEY_UNKNOWN,
	  KEY_UNKNOWN,
	  KEY_UNKNOWN,
	  KEY_UNKNOWN,
	  KEY_UNKNOWN,
	  KEY_UNKNOWN,
	  KEY_UNKNOWN,
	  KEY_UNKNOWN,
	  KEY_UNKNOWN,
	  KEY_UNKNOWN,
	  KEY_UNKNOWN,
	  KEY_UNKNOWN,
	  KEY_UNKNOWN,
	  KEY_UNKNOWN,
	  KEY_UNKNOWN,
	  KEY_UNKNOWN,
	  KEY_UNKNOWN,
	  KEY_UNKNOWN,
	  KEY_UNKNOWN,
	  KEY_UNKNOWN,
	  KEY_UNKNOWN,
	  KEY_UNKNOWN,
	  KEY_UNKNOWN,
	  KEY_UNKNOWN,
	  KEY_UNKNOWN,
	  KEY_UNKNOWN,
	  KEY_UNKNOWN,
	  KEY_UNKNOWN,
	  KEY_UNKNOWN,
	  KEY_UNKNOWN,
	  KEY_UNKNOWN,
	  KEY_KPLEFTPAREN,
	  KEY_KPRIGHTPAREN,
	  KEY_UNKNOWN,
	  KEY_UNKNOWN,
	  KEY_UNKNOWN,
	  KEY_UNKNOWN,
	  KEY_UNKNOWN,
	  KEY_UNKNOWN,
	  KEY_UNKNOWN,
	  KEY_UNKNOWN,
	  KEY_UNKNOWN,
	  KEY_UNKNOWN,
	  KEY_UNKNOWN,
	  KEY_UNKNOWN,
	  KEY_UNKNOWN,
	  KEY_UNKNOWN,
	  KEY_UNKNOWN,
	  KEY_UNKNOWN,
	  KEY_UNKNOWN,
	  KEY_UNKNOWN,
	  KEY_UNKNOWN,
	  KEY_UNKNOWN,
	  KEY_UNKNOWN,
	  KEY_UNKNOWN,
	  KEY_UNKNOWN,
	  KEY_UNKNOWN,
	  KEY_UNKNOWN,
	  KEY_UNKNOWN,
	  KEY_UNKNOWN,
	  KEY_UNKNOWN,
	  KEY_UNKNOWN,
	  KEY_UNKNOWN,
	  KEY_UNKNOWN,
	  KEY_UNKNOWN,
	  KEY_ZOOMRESET,
	  KEY_ZOOMIN,
	  KEY_ZOOMOUT,
	  KEY_COFFEE,
	  KEY_MESSENGER,
	  KEY_IMAGES,
	  KEY_AUDIO,
	  KEY_VIDEO,
	  KEY_LEFTCTRL, 
	  KEY_LEFTSHIFT, 
	  KEY_LEFTALT,
	  KEY_LEFTMETA,
	  KEY_RIGHTCTRL, 
	  KEY_RIGHTSHIFT,
	  KEY_RIGHTALT,
	  KEY_RIGHTMETA,
	  KEY_PLAYPAUSE,
	  KEY_STOPCD,
	  KEY_PREVIOUSSONG,
	  KEY_NEXTSONG,
	  KEY_EJECTCD,
	  KEY_VOLUMEUP,
	  KEY_VOLUMEDOWN,
	  KEY_MUTE,
	  KEY_WWW,
	  KEY_BACK,
	  KEY_FORWARD,
	  KEY_STOP,
	  KEY_SEARCH,
	  KEY_SCROLLUP,
	  KEY_SCROLLDOWN,
	  KEY_EDIT,
	  KEY_SLEEP,
	  KEY_COFFEE,
	  KEY_REFRESH,
	  KEY_CALC,
	  KEY_HOMEPAGE,
	  KEY_MAIL,
	  KEY_CONFIG,
	  KEY_UNKNOWN
};

#define HID_KBD_NAME  "hid-keyboard"
#define HID_KEY_PRESS 1
#define HID_KEY_RELEASE 0
#define HID_KEY_RESERVE	2
#define HID_POINTER_X	3
#define HID_POINTER_Y	4
#define HID_WHEEL	5
#define HID_KEYBOARD	6
#define HID_MOUSE	7

static struct input_dev *hid_input_dev;

struct hidkeyboard {
    struct input_dev *input;
    unsigned short keymap[ARRAY_SIZE(hid_keycode)];
};

struct hidkeyboard *hidkbd;
int registered = 0;

#ifdef CC_KEY_BUFFER
static void hid_key_buf_flush(void)
{
    int i ;
    key_buf_en = 0 ;

    printk(KERN_INFO "hid_key_buf_flush...\n");
    for(i=0 ; i< HID_KEY_BUF_NUM; i++)
    {
        if(key_buf[i].valid)
        {
            printk(KERN_INFO "en:%d keycode=%d, stat=%d\n", key_buf[i].valid , key_buf[i].keycode, key_buf[i].state);
            input_report_key(hid_input_dev,hid_keycode[key_buf[i].keycode], key_buf[i].state);
            input_sync(hid_input_dev);
            key_buf[i].valid = false ;
        }
    }
}
#endif

static long hid_kbd_dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	void __user *uarg = (void __user *)arg;
	short value;
	unsigned char keycode;
	int err,i;
	//printk(KERN_INFO "hid_kbd_dev_ioctl,cmd=%d\n",cmd);
	switch(cmd)
	{
		case HID_KEYBOARD:
		{		
			hid_input_dev->keycodemax = ARRAY_SIZE(hid_keycode);
			for (i = 0; i < ARRAY_SIZE(hidkbd->keymap); i++)
				__set_bit(hidkbd->keymap[i], hid_input_dev->keybit);
			err = input_register_device(hid_input_dev);
			if (err) 
			{
				//printk(KERN_ERR "register input device failed (%d)\n", err);
				input_free_device(hid_input_dev);
				return err;
			}
			registered = 1;
			break;
		}
		case HID_MOUSE:
		{		
			hid_input_dev->keycodemax = 4;
			for (i = 0; i < hid_input_dev->keycodemax; i++)
				__set_bit(hidkbd->keymap[i], hid_input_dev->keybit);
			err = input_register_device(hid_input_dev);
			if (err) 
			{
				//printk(KERN_ERR "register input device failed (%d)\n", err);
				input_free_device(hid_input_dev);
				return err;
			}
			registered = 1;
			break;
		}
		case HID_KEY_PRESS:
		{
			if (copy_from_user(&keycode, uarg, sizeof(keycode)))
				return -EFAULT;
			//printk(KERN_INFO "hid keycode  %d press\n",keycode);
			#ifdef CC_KEY_BUFFER
			if(key_buf_en)
			{
				if (key_buf_idx < HID_KEY_BUF_NUM)
				{
					 key_buf[key_buf_idx].keycode = keycode ;
					 key_buf[key_buf_idx].state = S_KEY_PRESS ;
					 key_buf[key_buf_idx++].valid = true ;
				 }
			}
			else
			#endif
			{
				input_report_key(hid_input_dev,hid_keycode[keycode], 1);
				input_sync(hid_input_dev);
			}
			break;
		}
		case HID_KEY_RELEASE:
		{
			if (copy_from_user(&keycode, uarg, sizeof(keycode)))
				return -EFAULT;
			//printk(KERN_INFO "hid keycode %d release\n",keycode);
			#ifdef CC_KEY_BUFFER
			if(key_buf_en)
			{
				if (key_buf_idx < HID_KEY_BUF_NUM)
				{
					 key_buf[key_buf_idx].keycode = keycode ;
					 key_buf[key_buf_idx].state = S_KEY_RELEASE;
					 key_buf[key_buf_idx++].valid = true ;
				}
			}
			else
			#endif
			{	
				input_report_key(hid_input_dev,hid_keycode[keycode], 0);
				input_sync(hid_input_dev);
			}
			break;
		}
		case HID_POINTER_X:
		{
			if (copy_from_user(&value, uarg, sizeof(value)))
				return -EFAULT;
			//printk(KERN_INFO "hid pointer X %d \n",value);
			#ifdef CC_KEY_BUFFER
			if(key_buf_en)
			{
				//ignore
			}
			else
			#endif
			{
				input_report_rel(hid_input_dev, REL_X, value);
				input_sync(hid_input_dev);
			}
			break;
		}
		case HID_POINTER_Y:
		{
			if (copy_from_user(&value, uarg, sizeof(value)))
				return -EFAULT;
			//printk(KERN_INFO "hid pointer Y %d \n",value);
			#ifdef CC_KEY_BUFFER
			if(key_buf_en)
			{
				//ignore
			}
			else
			#endif
			{
				input_report_rel(hid_input_dev, REL_Y, value);
				input_sync(hid_input_dev);
			}
			break;
		}
		case HID_WHEEL:
		{
			if (copy_from_user(&value, uarg, sizeof(value)))
				return -EFAULT;
			//printk(KERN_INFO "hid wheel %d \n",value);
			#ifdef CC_KEY_BUFFER
			if(key_buf_en)
			{
				//ignore
			}
			else
			#endif
			{
				input_report_rel(hid_input_dev, REL_WHEEL, value);
				input_sync(hid_input_dev);
			}
			break;
		}
		default:
			return -EINVAL;
	}
	return 0;
}

static int hid_kbd_dev_open(struct inode *inode, struct file *file)
{

	//int err,i;
	printk(KERN_INFO "*** hidkeyboard hid_kbd_dev_open ***\n");

	hidkbd = kzalloc(sizeof(struct hidkeyboard), GFP_KERNEL);
    	hid_input_dev = input_allocate_device();
   	 if (!hidkbd || !hid_input_dev) 
		goto fail;
	
    	memcpy(hidkbd->keymap, hid_keycode,
		sizeof(hid_keycode));
	hidkbd->input = hid_input_dev;

	//__set_bit(EV_KEY, hid_input_dev->evbit);
	hid_input_dev->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_REL);
	hid_input_dev->relbit[0] = BIT_MASK(REL_X) | BIT_MASK(REL_Y) | BIT_MASK(REL_WHEEL);

	hid_input_dev->name = HID_KBD_NAME;
	hid_input_dev->keycode = hidkbd->keymap;
	hid_input_dev->keycodesize = sizeof(unsigned short);
	//hid_input_dev->keycodemax = ARRAY_SIZE(hid_keycode);
	hid_input_dev->id.bustype = BUS_HOST;
/*
	for (i = 0; i < ARRAY_SIZE(hidkbd->keymap); i++)
		__set_bit(hidkbd->keymap[i], hid_input_dev->keybit);

	input_set_capability(hid_input_dev, EV_MSC, MSC_SCAN);
	err = input_register_device(hid_input_dev);
    	if (err) {
		xlog_printk(ANDROID_LOG_ERROR,HID_SAY,"register input device failed (%d)\n", err);
		input_free_device(hid_input_dev);
		return err;
	}
	*/
	return 0;
fail:
    //platform_set_drvdata(pdev, NULL);
    input_free_device(hid_input_dev);
    kfree(hidkbd);
    
    return -EINVAL;
}

static int hid_kbd_dev_release(struct inode *inode, struct file *file)
{
	printk(KERN_INFO "*** hidkeyboard hid_kbd_dev_release ***\n");
	if(registered == 1)
	{
		input_unregister_device(hid_input_dev);
		registered = 0;
	}
	return 0;
}


static struct file_operations hid_kbd_dev_fops = {
	.owner		= THIS_MODULE,
	.unlocked_ioctl	= hid_kbd_dev_ioctl,
	.open		= hid_kbd_dev_open,
	.release		= hid_kbd_dev_release
};

static struct miscdevice hid_kbd_dev = {
	.minor	= MISC_DYNAMIC_MINOR,
	.name	= HID_KBD_NAME,
	.fops	= &hid_kbd_dev_fops,
};


static int hid_keyboard_probe(struct platform_device *pdev)
{

    int i,err;
    
    printk(KERN_INFO "*** hidkeyboard probe ***\n");

    hidkbd = kzalloc(sizeof(struct hidkeyboard), GFP_KERNEL);
    hid_input_dev = input_allocate_device();
    if (!hidkbd || !hid_input_dev) 
		goto fail;
	
    	memcpy(hidkbd->keymap, hid_keycode,
		sizeof(hid_keycode));
	hidkbd->input = hid_input_dev;
	__set_bit(EV_KEY, hid_input_dev->evbit);
	platform_set_drvdata(pdev, hidkbd);

	hid_input_dev->name = HID_KBD_NAME;
	hid_input_dev->keycode = hidkbd->keymap;
	hid_input_dev->keycodesize = sizeof(unsigned short);
	hid_input_dev->keycodemax = ARRAY_SIZE(hid_keycode);
	hid_input_dev->id.bustype = BUS_HOST;
	hid_input_dev->dev.parent = &pdev->dev;


	for (i = 0; i < ARRAY_SIZE(hidkbd->keymap); i++)
		__set_bit(hidkbd->keymap[i], hid_input_dev->keybit);

	input_set_capability(hid_input_dev, EV_MSC, MSC_SCAN);
/*
    err = input_register_device(hid_input_dev);
    	if (err) {
		xlog_printk(ANDROID_LOG_ERROR,HID_SAY,"register input device failed (%d)\n", err);
		input_free_device(hid_input_dev);
		return err;
	}*/
	hid_kbd_dev.parent = &pdev->dev;
	err = misc_register(&hid_kbd_dev);
	if (err) {
		printk(KERN_INFO "register device failed (%d)\n", err);
		//input_unregister_device(hid_input_dev);
		return err;
	}

    return 0;

fail:
    platform_set_drvdata(pdev, NULL);
    input_free_device(hid_input_dev);
    kfree(hidkbd);
    
    return -EINVAL;
}

static struct platform_driver hid_keyboard_driver = {
    .probe = hid_keyboard_probe,
    .driver = {
        .name = HID_KBD_NAME,
    },
};
#ifdef CC_KEY_BUFFER
#include <linux/proc_fs.h>
static struct proc_dir_entry *hid_key_file ;
static int proc_read_hid_key(char *buf, char **start,
                            off_t off, int count, 
                            int *eof, void *data)                
{
    int outlen=0, i;
        
    outlen += sprintf((buf + outlen), "key_buf_idx=%d key_buf_en=%d\n", key_buf_idx, key_buf_en);

    for(i=0 ; i< HID_KEY_BUF_NUM; i++)
    {
        outlen += sprintf((buf + outlen), "en:%d keycode=%d, stat=%d\n", key_buf[i].valid , key_buf[i].keycode, key_buf[i].state);
    }
    return outlen;
}

ssize_t proc_write_hid_key( struct file *filp, const char __user *buf,
                        unsigned long bytes, void *pos )                              
{      
    unsigned char tmp[16];
    int tmpval = 0, retval = 0, i = 0;
    
    memset(tmp, 0, 16);
    
    if (bytes > 0) 
    {    
        retval = copy_from_user(tmp , buf, bytes);
        if (retval != 0)
        {
            printk(KERN_INFO "proc_write_hid_key: copy_from_user() failed!\n");
            return -EFAULT;
        }
    
        tmp[bytes] = '\0';
        if((tmp[i] - '0'))
        {
            hid_key_buf_flush();	
        }
    }
    return bytes;
}
#endif //CC_KEY_BUFFER
static int __devinit hid_keyboard_init(void)
{
    printk(KERN_INFO "hid_keyboard_init OK\n");

#ifdef CC_KEY_BUFFER
    memset(key_buf,0,  sizeof(key_buf));
    printk(KERN_INFO "[adam] keybuf %d\n",  sizeof(key_buf));
    //create debug proc file
    hid_key_file = create_proc_entry("hid_key_buf", 0664, NULL);
    if(hid_key_file)
    {
        hid_key_file->read_proc = proc_read_hid_key;
        hid_key_file->write_proc = proc_write_hid_key;
    }
#endif
    return platform_driver_register(&hid_keyboard_driver);
}


static void __exit hid_keyboard_exit(void)
{
}

module_init(hid_keyboard_init);
module_exit(hid_keyboard_exit);

MODULE_DESCRIPTION("hid keyboard Device");
MODULE_LICENSE("GPL");

