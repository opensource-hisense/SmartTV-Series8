/*-----------------------------------------------------------------------------
 * drivers/usb/musb_qmu/mtk_cust.c
 *
 * MT53xx USB driver
 *
 * Copyright (c) 2008-2012 MediaTek Inc.
 * $Author: dtvbm11 $
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 *
 *---------------------------------------------------------------------------*/


/** @file mtk_cust.c
 *  This C file implements the mtk53xx USB host controller customization driver.
 */

//---------------------------------------------------------------------------
// Include files
//---------------------------------------------------------------------------
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/list.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/usb.h>
#include <linux/kthread.h>
#include <asm/byteorder.h>
#include <asm/io.h>
#include <asm/irq.h>
//#include <asm/system.h>
#include <asm/unaligned.h>
#include <asm/uaccess.h>
#include <linux/dma-mapping.h>
#include <linux/usb/hcd.h>
#ifdef CONFIG_MT53XX_NATIVE_GPIO
#ifdef CONFIG_64BIT
#include <linux/soc/mediatek/mtk_gpio.h>
#else
#include <mach/mtk_gpio.h>
#endif
#endif

#include "mtk_hcd.h"
#include "mtk_qmu_api.h"


extern int SONY_HUB_OCSTATUS;
extern int SONY_HUB_InPlugStatus;



#ifdef MUSB_DEBUG
extern int mgc_debuglevel;
#endif
extern unsigned int ssusb_adb_flag;
extern unsigned int ssusb_adb_port;

#define MTK_USB_TEST_MAJOR 188
#define DEVICE_NAME "usb_cst"
#ifdef CC_SONY_MT7662
//#define MT7662_POWER_ALWAYS_ENABLE
#endif
#define MUT_MAGIC1 'U'
#define MUT_IOCTL_GET _IOR(MUT_MAGIC1, 0, int)
#define MUT_IOCTL_SET _IOW(MUT_MAGIC1, 1, int)
#define MUT_IOCTL_OC  0x58000000
#define MUT_IOCTL_STATUS  0x58000004

#ifndef DEMO_BOARD
#ifdef CONFIG_USB_OC_SUPPORT
uint8_t u1MUSBVbusEanble = FALSE;   // USB Vbus status, true = Vbus On / false = Vbus off
uint8_t u1MUSBOCEnable = FALSE;      // USB OC function enable status, true= enable oc detect /false = not
uint8_t u1MUSBOCStatus = FALSE;      // USB OC status, true = oc o / false=oc not cours
uint8_t u1MUSBOCPort = 0;      // USB OC port, 1,2,3...

#define OC_USB3_PORT_STA_BASE 0xF0074430
#define OC_USB3_PORT_STA_OFST 0x0
#endif
#endif


//---------------------------------------------------------------------------
// Configurations
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// Constant definitions
//---------------------------------------------------------------------------
#ifdef CONFIG_MT53XX_NATIVE_GPIO
extern int MUC_aPwrGpio[MUC_NUM_MAX_CONTROLLER];
extern int MUC_aPwrPolarity[MUC_NUM_MAX_CONTROLLER];
extern int MUC_aOCGpio[MUC_NUM_MAX_CONTROLLER];
extern int MUC_aOCPolarity[MUC_NUM_MAX_CONTROLLER];
extern int MUC_aPortUsing[MUC_NUM_MAX_CONTROLLER];
#endif


//---------------------------------------------------------------------------
// Global function
//---------------------------------------------------------------------------
#if 0
uint32_t MUCUST_GetControllerNum(void)
{
    return (MUC_NUM_PLATFORM_DEV);
}

uint32_t MUCUST_GetDMAChannelNum(uint32_t u4Index)
{
    return MGC_HSDMA_CHANNELS;
}

MUSB_LinuxController* MUCUST_GetControllerDev(uint32_t u4Index)
{
    return &MUC_aLinuxController[u4Index];
}

struct platform_device* MUCUST_GetPlatformDev(uint32_t u4Index)
{
    return &MUC_pdev[u4Index];
}
#endif
#ifndef DEMO_BOARD
#ifdef CONFIG_USB_OC_SUPPORT
static int proc_oc_message(void __user *arg)
{
    uint8_t *buf = NULL;
    uint8_t size;
	uint8_t id = 0;
#if defined(CC_OX_CUST) || defined(CC_WK_CUST)
	uint8_t u1Is4K = FALSE;
#endif
	
    size = sizeof(u1MUSBOCStatus);

    if ((buf = kmalloc(size, GFP_KERNEL)) == NULL)
	return -ENOMEM;

    *buf = 0;
    if (copy_from_user(buf, (uint8_t __user *)arg, sizeof(u1MUSBOCStatus))){
        INFO("[USB]copy_from_user fail\n");
        return 1;
    }

    id = *buf;


#if defined(CC_OX_CUST) || defined(CC_WK_CUST)

#ifdef CC_SAKURA_CVT_SUPPORT
    if(mtk_gpio_get_value(72))
    {
        u1Is4K = TRUE;      // 4K model
    }
    else
    {
        u1Is4K = FALSE;     // 2K model
    }
#endif

    if(u1Is4K)          // 4K model
        {
        if(id >= 1 && id <= 3)
            {

            *buf = ((u1MUSBOCPort&(1<<(id-1))) == 0)?0:1;

        }
    }
    else                // 2K model
        {
        if((id == 1) || (id == 2))
            {
            *buf = ((u1MUSBOCPort&(1<<id)) == 0)?0:1;
        }
        else if(id == 3)
            {
            *buf = ((u1MUSBOCPort&(1<<0)) == 0)?0:1;
        }
    }

    if((id==0xff)&&((u1Is4K&&(((SONY_HUB_OCSTATUS & (1<<1))!=0) || ((SONY_HUB_OCSTATUS & (1<<2))!=0))) || u1MUSBOCStatus))
        {
        *buf = TRUE;
    }
#endif


#ifdef CC_LM_CUST

	if(u1MUSBOCStatus && (u1MUSBOCPort & (1 << id)))
		{
		*buf = TRUE;
	}
	else if((id==0xFF) && u1MUSBOCStatus && (u1MUSBOCPort != 0))
		{
		*buf = TRUE;
	}
	else
		{
		*buf = FALSE;
	}

#if 0
	if(u1MUSBOCStatus && (id==0))
		{
		*buf = TRUE;
	}
	else if(SONY_HUB_OCSTATUS & (1 << (id+1)))
		{
		*buf = TRUE;
	}
	else if((id==0xFF)&&((SONY_HUB_OCSTATUS != 0) || u1MUSBOCStatus))
		{
		*buf = TRUE;
	}
	else
		{
		*buf = FALSE;
	}
#endif
	
#endif

   // printk("proc_oc_message>>u1MUSBOCStatus = %u\n", u1MUSBOCStatus);
    
    if (copy_to_user((uint8_t __user *)arg, buf, sizeof(u1MUSBOCStatus)))
        return -EFAULT;
    kfree(buf);
    return 0;
}
static int proc_insert_message(void __user *arg)
{
	uint8_t *buf = NULL;
	uint8_t size;
	uint8_t id = 0;
    MUSB_LinuxController *pController;

#if defined(CC_OX_CUST) || defined(CC_WK_CUST)
    uint32_t U3_InsertSTA= 0;
    uint8_t u1Is4K = FALSE;
#endif	
	   
	size = sizeof(u1MUSBOCPort);
	if ((buf = kmalloc(size, GFP_KERNEL)) == NULL){
		INFO("[USB] kmalloc fail\n");
		return 1;
	}
	*buf = 0;
	if (copy_from_user(buf, (uint8_t __user *)arg, sizeof(u1MUSBOCPort))){
		INFO("[USB]copy_from_user fail\n");
		return 1;
	}

	id = *buf;

#if defined(CC_OX_CUST) || defined(CC_WK_CUST)
	
#ifdef CC_SAKURA_CVT_SUPPORT
    if(mtk_gpio_get_value(72))
    {
        u1Is4K = TRUE;      // 4K model
    }
    else
    {
        u1Is4K = FALSE;     // 2K model
    }
#endif

    if(u1Is4K)          // 4K model
        {
        if(id == 1 || id == 2)
            {

            pController = MUCUST_GetControllerDev(id-1);  
            if (!pController || !(pController->pThis))          // ADB pController->pThis will NULL
            {
                return USB_ST_REMOVED;
            }
            *buf = pController->pThis->bInsert;
            
        }
        else if(id == 3)
            {
            U3_InsertSTA = MGC_Read32(OC_USB3_PORT_STA_BASE, OC_USB3_PORT_STA_OFST);
            U3_InsertSTA &= 0x1;
            *buf = U3_InsertSTA;
        }
    }
    else                // 2K model
        {
        if(id == 1)
            {
            pController = MUCUST_GetControllerDev(id);  
            if (!pController || !(pController->pThis))          // ADB pController->pThis will NULL
            {
                return USB_ST_REMOVED;
            }
            *buf = pController->pThis->bInsert;
        }
        else if(id == 2)
            {
            // get the USB3.0 port insert status
            U3_InsertSTA = MGC_Read32(OC_USB3_PORT_STA_BASE, OC_USB3_PORT_STA_OFST);
            U3_InsertSTA &= 0x1;
            *buf = U3_InsertSTA;
        }
        else if(id == 3)
            {
            pController = MUCUST_GetControllerDev(0);  
            if (!pController || !(pController->pThis))          // ADB pController->pThis will NULL
            {
                return USB_ST_REMOVED;
            }
            *buf = pController->pThis->bInsert;
        }

    }
#endif


#ifdef CC_LM_CUST

	if(id==0)
		{
		pController = MUCUST_GetControllerDev(id);	
		if (!pController || !(pController->pThis))			// ADB pController->pThis will NULL
		{
			return USB_ST_REMOVED;
		}
		*buf = pController->pThis->bInsert;
	}
	else if(id==1)		  // just use it in 4K model
		{
		*buf = ((SONY_HUB_InPlugStatus & (1<<2))==0)?0:1;
	}
	else
	{
		*buf = 0;
	}
#endif

	if (copy_to_user((uint8_t __user *)arg, buf, sizeof(u1MUSBOCPort))){
		INFO("[USB]copy_to_user error \n");
		return 1;
		}
	kfree(buf);
	return 0;
}

static int MGC_USBOCThreadMain(void * pvArg)
{
	unsigned long	 flags;
	uint8_t fgOCStatus=FALSE;
	uint8_t fgOCNewStatus;
	uint32_t u4OCCount;
	uint8_t i = 0;
	#ifdef CONFIG_MT53XX_NATIVE_GPIO
	uint8_t bPortNum=0;
	#endif
	
	local_irq_save(flags);
	
	//OC pin PIMUX SET TO GPIO as input (set pimux &function)
//      should NOT set pinmux here because pins might depends on customer and pcba. please set pinmux in customization part.
//	*((volatile uint32_t *)0xF000D600) &= ~0x800;  
//	*((volatile uint32_t *)0xF000D724) &= ~0x10000000;

	local_irq_restore(flags);
	
	u1MUSBOCEnable = FALSE;
	u1MUSBOCStatus = FALSE;

   do{
   		if((i == 0) && (u1MUSBVbusEanble))
		{
			//printk("[OC]USB OC enable.\n");
			u1MUSBOCEnable = TRUE;
			i = 1;
		}
		
		if(u1MUSBOCEnable)
		{
			// Check OverCurrent.					
			//u1MUSBOCPort=0;
			fgOCNewStatus = u1MUSBOCStatus;
			//printk("[OC]Start do usb overcurrent checking\n");
			
			// check every USB port over current status.
			for(bPortNum=0;bPortNum<MUC_NUM_MAX_CONTROLLER;bPortNum++)
			{
                u4OCCount = 0;
		        //#ifdef MUSB_GADGET_PORT_NUM
		        #if 1
                if((ssusb_adb_flag == 1) && (bPortNum == ssusb_adb_port))
					continue;
				#endif
				
				if((MUC_aPwrGpio[bPortNum] != -1)&&(MUC_aPortUsing[bPortNum] == 1)&&(MUC_aOCGpio[bPortNum] != -1))
				{
                    do
                    {
    					if(MUC_aOCPolarity[bPortNum] == mtk_gpio_get_value(MUC_aOCGpio[bPortNum]))
    					{
    						fgOCStatus = TRUE;
						}
    					else
                        {
    						fgOCStatus = FALSE;
                        }
                        
                        // debounce for every port
                        if (fgOCNewStatus != fgOCStatus)
                        {
                            fgOCNewStatus = fgOCStatus;
                            u4OCCount = 0;
                        }
                        else
                        {
                            u4OCCount ++;
                        }
                        msleep(5);
                    }
#ifdef CC_WK_CUST
                    while(u4OCCount < 3);
#else
                    while(u4OCCount < 5);
#endif

                    if(fgOCStatus == TRUE)
                        u1MUSBOCPort |= 1<<bPortNum;
                    else
                        u1MUSBOCPort &= ~(1<<bPortNum);
				}
			}

            if(u1MUSBOCPort != 0)
            {
                fgOCStatus = TRUE;
            }
            else
            {
                fgOCStatus = FALSE;
            }

			if (fgOCStatus != u1MUSBOCStatus)
			{	
				// change over current status.
				u1MUSBOCStatus = fgOCStatus; 
				if(u1MUSBOCStatus) printk("###OC OC OC ###. u1MUSBOCPort = 0x%x \n",u1MUSBOCPort);
				// Over current is just disappear. Turn on Vbus, and check if it still happen. 
				if ((!fgOCStatus) && (u1MUSBVbusEanble == FALSE))
				{
					DBG(2, "USB OC: Turn on Vbus GPIO on.\n");
					u1MUSBVbusEanble = TRUE;
					#ifdef CONFIG_MT53XX_NATIVE_GPIO
					for(bPortNum=0;bPortNum<MUC_NUM_MAX_CONTROLLER;bPortNum++)
					{
						//#ifdef MUSB_GADGET_PORT_NUM
						#if 1
						if((ssusb_adb_flag == 1) && (bPortNum == ssusb_adb_port))
							continue;
						#endif
						if((MUC_aPwrGpio[bPortNum] != -1) && (MUC_aPortUsing[bPortNum] == 1))
							MGC_VBusControl(bPortNum,TRUE);
					}
					#else
				        //control GPIO to enable USB power
				        // 1st parameter port num must according to project schmatic
						MGC_VBusControl(1, 1);
					#endif
				}
			
				if ((fgOCStatus) && (u1MUSBVbusEanble == TRUE))
				{
					printk("USB OC: Turn off Vbus GPIO off\n");
					u1MUSBVbusEanble = FALSE;
					
					#ifdef CONFIG_MT53XX_NATIVE_GPIO
					for(bPortNum=0;bPortNum<MUC_NUM_MAX_CONTROLLER;bPortNum++)
					{
						//#ifdef MUSB_GADGET_PORT_NUM
						#if 1
						if((ssusb_adb_flag == 1) && (bPortNum == ssusb_adb_port))
							continue;
						#endif
						if((MUC_aPwrGpio[bPortNum] != -1) && (MUC_aPortUsing[bPortNum] == 1) && (u1MUSBOCPort&(1<<bPortNum)))
							MGC_VBusControl(bPortNum,FALSE);
					}
					#else
				        //control GPIO to enable USB power
				        // 1st parameter port num must according to project schmatic
						MGC_VBusControl(1, 0);
					#endif					   
					msleep(2000);
				}							   
			}
			else
			{
				/*
					When turn on servo gpio to get adc value, it will turn on vbus.
				*/
				if (fgOCStatus)
				{
					if ((u1MUSBVbusEanble == TRUE))
					{
						//printk("USB OC2: Turn off Vbus GPIO off\n");
						u1MUSBVbusEanble = FALSE;	
						#ifdef CONFIG_MT53XX_NATIVE_GPIO
						for(bPortNum=0;bPortNum<MUC_NUM_MAX_CONTROLLER;bPortNum++)
						{
							//#ifdef MUSB_GADGET_PORT_NUM
							#if 1
							if((ssusb_adb_flag == 1) && (bPortNum == ssusb_adb_port))
								continue;
							#endif
							if((MUC_aPwrGpio[bPortNum] != -1) && (MUC_aPortUsing[bPortNum] == 1) && (u1MUSBOCPort&(1<<bPortNum)))
								MGC_VBusControl(bPortNum,FALSE);
						}
						#else
							//control GPIO to enable USB power
							// 1st parameter port num must according to project schmatic
							MGC_VBusControl(1, 0);
						#endif
						msleep(2000);
					}
				}
			}

		 msleep(1000);
	 }
	/*
	    avoid system loop in while(1) hang up system when u1MUSBOCEnable= FASLE
    */
	else
	{
		msleep(1000);
	}
}while(1);

	return 0;
}


static void MGC_USBOCCreateThread(void)
{
    struct task_struct *tThreadStruct;
    char ThreadName[10]={0};
    // Thread Name : USB_OC
    ThreadName[0] = 'U';
    ThreadName[1] = 'S';
    ThreadName[2] = 'B';
    ThreadName[3] = '_';
    ThreadName[4] = 'O';
    ThreadName[5] = 'C';

	printk("OCCreate thread.\n");
	
    tThreadStruct = kthread_create(MGC_USBOCThreadMain, NULL, ThreadName);

    if ( (unsigned long)tThreadStruct == -ENOMEM )
    {
        printk("[USB]MGC_USBOCCreateThread Failed\n");
        return;
    }

    wake_up_process(tThreadStruct);
    return;
}

#endif
#endif

static ssize_t MUT_read(struct file *file, char __user *buf, size_t count, loff_t *ptr)
{

    //printk("MUT_read\n");
    return 0;
}

static ssize_t MUT_write(struct file *file, const char __user *buf, size_t count, loff_t * ppos)
{

    //printk("MUT_write\n");
    return 0;
}

static long MUT_ioctl( struct file *file, unsigned int cmd, unsigned long arg)
{
    int retval = -ENOTTY;

	switch (cmd) {
#ifndef DEMO_BOARD
#ifdef CONFIG_USB_OC_SUPPORT
		case MUT_IOCTL_OC:
			retval = proc_oc_message((void __user *)arg);
			break;
			
		case MUT_IOCTL_STATUS:
			retval = proc_insert_message((void __user *)arg);
			break;

#endif		
#endif
		case MUT_IOCTL_SET:
			 return retval;
			
		default:
			return -ENOTTY;
	}

	return retval;

}

static int MUT_open(struct inode *inode, struct file *file)
{
    return 0;
}

static int MUT_release(struct inode *inode, struct file *file)
{
    return 0;
}

struct file_operations MUT_fops = {
    .owner =   THIS_MODULE,
    .read =    MUT_read,
    .write =   MUT_write,
    .unlocked_ioctl =   MUT_ioctl,
    .open =    MUT_open,
    .release=  MUT_release,
};

int MUT_init(void)
{
#ifndef DEMO_BOARD
#ifdef CONFIG_USB_OC_SUPPORT

    int retval = 0;
    printk("MUT_init\n");
    retval=__register_chrdev(MTK_USB_TEST_MAJOR,255,1,DEVICE_NAME,&MUT_fops);
    if(retval < 0)
	{
		printk("MUT_init __register_chrdev() failed, %d\n", retval);
		return retval;
	}
    // Init USB OC support.
    MGC_USBOCCreateThread();
#endif
#endif
    return 0;
}

void MUT_exit(void)
{
#ifndef DEMO_BOARD
#ifdef CONFIG_USB_OC_SUPPORT
	__unregister_chrdev(MTK_USB_TEST_MAJOR,255,1,DEVICE_NAME);

#endif
#endif
    printk("MUT_exit\n");
}

