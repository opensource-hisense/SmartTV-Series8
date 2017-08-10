/*-----------------------------------------------------------------------------
 *\drivers\usb\host\mtk_hcd.c
 *
 * MT53xx USB driver
 *
 * Copyright (c) 2008-2012 MediaTek Inc.
 * $Author: zile.pang $
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


/** @file mtk_hcd.c
 *  This C file implements the mtk53xx USB host controller driver.
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
#include <linux/workqueue.h>
#include <linux/kthread.h>
#include <asm/byteorder.h>
#include <asm/io.h>
#include <asm/irq.h>
//#include <asm/system.h>
#include <asm/unaligned.h>
#include <asm/uaccess.h>
#include <asm/cacheflush.h>
#include <linux/dma-mapping.h>
#include <linux/usb/hcd.h>
#include <linux/suspend.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/of_address.h>

#include <linux/random.h>
#include <linux/ctype.h>
#include <linux/proc_fs.h>




#ifdef CONFIG_MT53XX_NATIVE_GPIO
#ifdef CONFIG_64BIT
#include <linux/soc/mediatek/mtk_gpio.h>
#include <linux/soc/mediatek/platform.h>
#include <linux/soc/mediatek/mt53xx_linux.h>
#else
#include <mach/mtk_gpio.h>
#include <mach/platform.h>
#include <mach/mt53xx_linux.h>
#endif
#endif

#include "mtk_hcd.h"
#include "mtk_qmu_api.h"
#include "mtk_sched.h"
#include <linux/usb/usb_cust.h>

#ifdef CONFIG_USB_MT53xx_COMPLIANCE_TEST
#include "mtk_debugfs.h"
#endif
#ifdef MUSB_DEBUG
int mgc_debuglevel= 0;
module_param(mgc_debuglevel, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(mgc_debuglevel,"Host Debug level");
#endif

int mgc_qmudisable= 0;
module_param(mgc_qmudisable, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(mgc_qmudisable,"Host qmudisable");

int mgc_qmuenable= 0;
module_param(mgc_qmuenable, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(mgc_qmuenable,"Host qmu enable");

int mgc_epskipchk= 0;
module_param(mgc_epskipchk, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(mgc_epskipchk,"Host ep skipcheck");

int mgc_resumtcnt= 0;
module_param(mgc_resumtcnt, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(mgc_resumtcnt,"Host resume printk number");

int mgc_printnum= 10;
module_param(mgc_printnum, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(mgc_printnum,"Host printk max number");

int mgc_bulkset= 1;
module_param(mgc_bulkset, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(mgc_bulkset,"Host printk bulk number");

int mgc_bulkoutset= 1;
module_param(mgc_bulkoutset, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(mgc_bulkoutset,"Host printk bulk out number");

int mgc_berror= 0;
module_param(mgc_berror, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(mgc_berror,"Host printk bulk out error");

int mgc_toggleErr_debug= 0;
module_param(mgc_toggleErr_debug, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(mgc_toggleErr_debug,"Toggle bit error debug");

//#define TEST_INTERVAL
#ifdef TEST_INTERVAL
int mgc_setinterval= 9;  // 2^(9-1)=256ms NAKTIMEOUT
module_param(mgc_setinterval, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(mgc_setinterval,"Host NAKTIMEOUT time");
#endif

unsigned int mgc_ep0kicktimes = 0;
module_param(mgc_ep0kicktimes, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(mgc_ep0kicktimes,"Host ep0 control enqueue times");

unsigned int mgc_ep0callbacktimes = 0;
module_param(mgc_ep0callbacktimes, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(mgc_ep0callbacktimes,"Host control callback times");

//20141228 WIFI found receive length is not 4 align, now printk it if usb host receive the data length is not 4 align. 
unsigned int mgc_bulkinep4 = 4;
module_param(mgc_bulkinep4, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(mgc_bulkinep4,"wifi bulkinep4 length must be 4 align");


int usb_hotboot= 0;
module_param(usb_hotboot, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(usb_hotboot, "default autosuspend delay");

MODULE_DESCRIPTION("MTK 53xx USB Host Controller Driver");
MODULE_LICENSE("GPL");


/*20150120 WIFI dongle disable/enable WIFI can't search AP.
 *Remote-EP8 toggle error from host, 
 *error order: Data 0 - disable wifi - enable wifi - Data 0.
 *host toggle was set after complete
 */
int mgc_togglecnt = 0;                  
int mgc_togglecnt_prev = 0;
int mgc_toggle_cur= 0;
int mgc_toggle_prev = 0;

int mgc_EP8_FIFO_TX_DONE = 0;

uint8_t bPowerStatus[4]={0};
bool is_wifi_onport3=0;

void __iomem *pUSBdmaIoMap;
void __iomem *pUSBmacIoMap;
void __iomem *pUSBphyIoMap;
void __iomem *pUSBfmIoMap1;
void __iomem *pUSBfmIoMap2;
void __iomem *pUSBpdwncIoMap;

#define USB_HOST_SESSION_TIMEOUT 800
/*Leon 20141106 USB ADB on/off set flag, 
  *ADB On: in uboot press "addboot ssusb_adb=1" and reboot 
  *ADB Off: in uboot press "addboot ssusb_adb=0" and reboot
  * we can press ">cat /proc/cmdline" to check "ssusb_adb"value 
  */
extern unsigned int ssusb_adb_flag;
extern unsigned int ssusb_adb_port;

//---------------------------------------------------------------------------
// Configurations
//---------------------------------------------------------------------------
#define DRIVER_VERSION    "01 May 2014"
#define MUSB_WIFI_PORT 3 // sony build-in  wifi is connected  to USB port3
/* Use MUC_DMA_DISABLE to switch between CPU/DMA mode */
//#define MUC_DMA_DISABLE 1
//---------------------------------------------------------------------------
// Constant definitions
//---------------------------------------------------------------------------
#define EPRX (0)
#define EPTX (1)
#define EP0 (0)
#define EPMSD (0)

unsigned long irq_flags;//added by zile.pang

//---------------------------------------------------------------------------
// Type definitions
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// Macro definitions
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// Imported variables
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// Imported functions
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// Static function forward declarations
//---------------------------------------------------------------------------

static void MGC_ServiceDefaultEnd(MGC_LinuxCd *pThis);
static void MGC_ServiceTxAvail(MGC_LinuxCd *pThis, uint8_t bEnd);
static void MGC_ServiceRxReady(MGC_LinuxCd *pThis, uint8_t bEnd);

static irqreturn_t MUC_Irq(struct usb_hcd *hcd);
static int MUC_start(struct usb_hcd *hcd);
static void MUC_stop(struct usb_hcd *hcd);
static int MUC_urb_enqueue(struct usb_hcd *hcd, struct urb *pUrb, gfp_t mem_flags);
static int MUC_urb_dequeue(struct usb_hcd *hcd, struct urb *pUrb, int status);
static void MUC_endpoint_disable(struct usb_hcd *hcd, struct usb_host_endpoint *hep);
static int MUC_get_frame(struct usb_hcd *hcd);
static int MUC_hub_status_data(struct usb_hcd *hcd, char *buf);
static int MUC_hub_control(struct usb_hcd *hcd, uint16_t typeReq, uint16_t wValue, uint16_t wIndex, char *buf, uint16_t wLength);
static int MUC_bus_suspend(struct usb_hcd *hcd);
static int MUC_bus_resume(struct usb_hcd *hcd);
static void MUC_hub_descriptor(MGC_LinuxCd *pThis, struct usb_hub_descriptor *desc);
static void MGC_CheckConnect(unsigned long pParam);
static void MGC_ResetOff(unsigned long param);

static int MUC_hcd_probe(struct platform_device *pdev);
static int MUC_hcd_remove(struct platform_device *pdev);
static int MUC_hcd_suspend(struct platform_device *pdev, pm_message_t state);
static int MUC_hcd_resume(struct platform_device *pdev);
#ifdef USB_NEW_URB_LIST
static void MGC_FreeEndpointListBuf(struct usb_hcd * hcd,uint8_t force_free);
#endif
#ifdef MGC_UNLINK_URB
static void MGC_UnlinkInvalidUrb(unsigned long pParam);
#endif
#ifdef CONFIG_USB_DETECT_IMPROVE
static bool MGC_IsDevAttached(uint8_t *pBase);
#endif
#ifdef CONFIG_DEVICE_IDENTIFY_SUPPORT
extern int MGC_IsUvcDevice(struct usb_device_descriptor *pDescriptor, struct usb_device *pDev);
extern int MGC_IsUacDevice(struct usb_device_descriptor *pDescriptor, struct usb_device *pDev);
extern int MGC_IsMFIDevice(struct usb_device_descriptor *pDescriptor, struct usb_device *pDev);
#endif


//---------------------------------------------------------------------------
// Static variables
//---------------------------------------------------------------------------
static const char MUC_HcdName[] = "MtkUsbHcd";

#ifdef CONFIG_MT53XX_NATIVE_GPIO
extern int MUC_aPortUsing[MUC_NUM_MAX_CONTROLLER];
extern int MUC_aPwrGpio[MUC_NUM_MAX_CONTROLLER];
extern int MUC_aPwrPolarity[MUC_NUM_MAX_CONTROLLER];
#endif


static struct platform_device MUC_pdev[MUC_NUM_PLATFORM_DEV];
extern MUSB_LinuxController MUC_aLinuxController[];

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


#if defined(CONFIG_PM) || defined(CC_SUPPORT_STR) 
static void MGC_VBusCtrl_pm(uint8_t bPortNum, uint8_t bOn)
{
    unsigned long flags=0;

    local_irq_save(flags);

	#ifdef CONFIG_MT53XX_NATIVE_GPIO
    if(MUC_aPwrGpio[bPortNum] != -1)
    {
        mtk_gpio_set_value(MUC_aPwrGpio[bPortNum], 
                ((bOn) ? (MUC_aPwrPolarity[bPortNum]) : (1-MUC_aPwrPolarity[bPortNum])));
    }	
	#endif

	#if 0
	if(bOn)
		bPowerStatus[bPortNum]=1;
	else 
		bPowerStatus[bPortNum]=0;
	#endif
    local_irq_restore(flags); 
	
}

static int usb_pm_notify(struct notifier_block *nb, unsigned long event,
	void *dummy)
{
	uint8_t bPortNum;
	
	if (event == PM_SUSPEND_PREPARE) {
		for(bPortNum=0; bPortNum< MUC_NUM_MAX_CONTROLLER; bPortNum++)
		{
			//#ifdef MUSB_GADGET_PORT_NUM
            #if 1
			if(((ssusb_adb_flag == 1) && (bPortNum == ssusb_adb_port))||((is_wifi_onport3)&&(bPortNum==MUSB_WIFI_PORT)))
				{
					//printk(KERN_ERR "[USB] skip USB port %d disconnection \n"bPortNum);
					continue;
				}
			#endif
			if((MUC_aPwrGpio[bPortNum] != -1)  )
				MGC_VBusCtrl_pm(bPortNum, 0);			
			printk(KERN_ERR "[USB] TurnOff VBUS Port[%d] GPIO[%d] \n",bPortNum,MUC_aPwrGpio[bPortNum]);
		}		

		printk(KERN_ERR "[USB] PM_SUSPEND_PREPARE start\n");
		#ifdef MUSB_SUPPORT_SUSPEND_BOOT_IDENTIFY
		usb_hotboot = 1;
		#endif
		msleep(200); //wait for finish the usb device disconnect related action. //wifi need 450ms to finish disconnect
		printk(KERN_ERR "[USB] PM_SUSPEND_PREPARE end\n");

	} else if (event == PM_POST_SUSPEND) {
		//msleep(100);		
		for(bPortNum=0; bPortNum< MUC_NUM_MAX_CONTROLLER; bPortNum++)
		{
		//#ifdef MUSB_GADGET_PORT_NUM
        #if 1
			if((ssusb_adb_flag == 1) && (bPortNum == ssusb_adb_port))
				continue;
		#endif
			if((MUC_aPwrGpio[bPortNum] != -1) )
				MGC_VBusCtrl_pm(bPortNum, 1);
			printk(KERN_ERR "[USB] TurnOn VBUS Port[%d] GPIO[%d] \n",bPortNum,MUC_aPwrGpio[bPortNum]);
		}		
		printk(KERN_ERR "[USB] PM_POST_SUSPEND\n");		
	}

	return NOTIFY_OK;
}

static struct notifier_block mtk_usb_pm_notifier = {
	.notifier_call = usb_pm_notify,
};

static void mgc_save_context(MGC_LinuxCd *pThis)
{
	void *pBase = pThis->pRegs;

	pThis->context.power = MGC_Read8(pBase, MGC_O_HDRC_POWER);
	pThis->context.intrtxe = MGC_Read16(pBase, MGC_O_HDRC_INTRTXE);
	pThis->context.intrrxe = MGC_Read16(pBase, MGC_O_HDRC_INTRRXE);
	pThis->context.intrusbe = MGC_Read8(pBase, MGC_O_HDRC_INTRUSBE);
	pThis->context.devctl = MGC_Read8(pBase, MGC_O_HDRC_DEVCTL);
	pThis->context.levelen = MGC_Read32(pBase, M_REG_INTRLEVEL1EN);
	pThis->context.dmamask = MGC_Read8(pBase, MGC_O_HSDMA_INTR_MASK);

	return;
}

static void mgc_clean_context(MGC_LinuxCd *pThis)
{
	void *pBase = pThis->pRegs;

	MGC_Write16(pBase, MGC_O_HDRC_INTRTXE, 0x0000);
	MGC_Write16(pBase, MGC_O_HDRC_INTRRXE, 0x0000);
	MGC_Write8(pBase, MGC_O_HDRC_INTRUSBE, 0x00);
	//MGC_Write8(pBase, MGC_O_HDRC_POWER, 0x00);	
	//MGC_Write8(pBase, MGC_O_HDRC_DEVCTL, 0x00);
 	MGC_Write32(pBase, M_REG_INTRLEVEL1EN, 0x00000000);	

	MGC_Write8(pBase,MGC_O_HSDMA_INTR_UNMASK_SET, 0x00);

#ifdef CONFIG_USB_QMU_SUPPORT
    if (pThis->bSupportCmdQ)
    {
        QMU_disable_all_q(pThis);
        QMU_clean(pThis);
    }
#endif

	return;
}

static void mgc_restore_context(MGC_LinuxCd *pThis)
{
	void *pBase = pThis->pRegs;
	int bChannelCount;

	MGC_Write16(pBase, MGC_O_HDRC_INTRTXE, pThis->context.intrtxe);
	MGC_Write16(pBase, MGC_O_HDRC_INTRRXE, pThis->context.intrrxe);
	MGC_Write8(pBase, MGC_O_HDRC_INTRUSBE, pThis->context.intrusbe);
	MGC_Write8(pBase, MGC_O_HDRC_POWER,pThis->context.power);
	
	MGC_Write8(pBase, MGC_O_HDRC_DEVCTL,pThis->context.devctl);
 	MGC_Write32(pBase, M_REG_INTRLEVEL1EN,pThis->context.levelen);
	
	// set MGC_DMA_UNMASK to "1" to enable DMA Channel
	// if not set, it will cause data request can not be output in bus if using this DMA channel.
	MGC_Write8(pBase,MGC_O_HSDMA_INTR_UNMASK_SET,pThis->context.dmamask);

	// for DMA
	#ifdef SUPPORT_SHARE_USBQ_DMA
	bChannelCount = MUCUST_GetDMAChannelNum(0);
	#else
	bChannelCount = (pThis->bSupportCmdQ) ? 
		(MUCUST_GetDMAChannelNum(0)-1) : (MUCUST_GetDMAChannelNum(0));
	#endif

#ifdef CONFIG_USB_QMU_SUPPORT
	if (pThis->bSupportCmdQ)
	{
		QMU_init(pThis);
		//reserve the last dma channel for command queue.
    	#ifdef SUPPORT_SHARE_USBQ_DMA
		QMU_select_dma_ch(pThis, (bChannelCount -1), 3);
		#else
		QMU_select_dma_ch(pThis, (bChannelCount), 3);
		#endif
	}
#endif  

	return;
}

#ifdef MUSB_SUPPORT_SKIP_SUSPEND
extern unsigned int mt53xx_pm_suspend_cnt;
unsigned int prev_value = 0;
#endif
static int mgc_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct usb_hcd  *hcd = dev_to_hcd(&pdev->dev);
	MGC_LinuxCd *pThis = hcd_to_musbstruct(hcd);
	MGC_LinuxLocalEnd *pEnd;
	struct urb *pUrb;
	int i, bEnd;
#if defined(CONFIG_ARCH_MT5891) || defined(CONFIG_ARCH_MT5882)
	uint32_t u4Reg = 0;
	uint8_t bReg = 0;
#endif
	
	spin_lock_irq(&pThis->SelectHcdLock);	
	INFO("[USB%d] begin.\n", pThis->bPortNum);

#if defined(CONFIG_ARCH_MT5891)
	if (pThis->bPortNum == 3) {
	      /* Force USB host suspend  */
	      INFO("Force USB host to Suspend Mode\n");
	      bReg = MGC_Read8(pThis->pRegs, MGC_O_HDRC_POWER);
	      bReg |= MGC_M_POWER_SUSPENDM;
	      MGC_Write8(pThis->pRegs, MGC_O_HDRC_POWER, bReg);

	      /* Switch Port-D MAC clock to xtal clock 24M  */
	      INFO("Switch standby MAC clock to xtal clock 24M\n");
	      u4Reg = MGC_PHY_Read32(pUSBpdwncIoMap, 0xe0);
	      u4Reg &= ~(0xc);
	      u4Reg |= (0x8);
	      MGC_PHY_Write32(pUSBpdwncIoMap, 0xe0, u4Reg);

	      /* Switch Port-D to standby power domain  */
	      INFO("Switch Port-D to standby power domain");
	      u4Reg = MGC_PHY_Read32(pUSBpdwncIoMap, 0x08);
	      u4Reg |= (0x1 << 31);
	      MGC_PHY_Write32(pUSBpdwncIoMap, 0x08, u4Reg);
	} else
#elif defined (CONFIG_ARCH_MT5882)
	if (pThis->bPortNum == 3 && IS_IC_MT5885()) {
	      /* Force USB host suspend  */
	      INFO("Force USB host to Suspend Mode\n");
	      bReg = MGC_Read8(pThis->pRegs, MGC_O_HDRC_POWER);
	      bReg |= MGC_M_POWER_SUSPENDM;
	      MGC_Write8(pThis->pRegs, MGC_O_HDRC_POWER, bReg);

	      /* Switch Port-D MAC clock to xtal clock 24M  */
	      INFO("Switch standby MAC clock to xtal clock 24M\n");
	      u4Reg = MGC_PHY_Read32(pUSBpdwncIoMap, 0xe8);
	      u4Reg &= ~(0x3);
	      u4Reg |= (0x2);
	      MGC_PHY_Write32(pUSBpdwncIoMap, 0xe8, u4Reg);

	      /* Switch Port-D to standby power domain  */
	      INFO("Switch Port-D to standby power domain");
	      u4Reg = MGC_PHY_Read32(pUSBpdwncIoMap, 0x08);
	      u4Reg |= (0x1 << 31);
	      MGC_PHY_Write32(pUSBpdwncIoMap, 0x08, u4Reg);
	} else
#endif
	{
	    clear_bit(HCD_FLAG_HW_ACCESSIBLE, &hcd->flags);
	    mgc_save_context(pThis);

	    //disable USB interrupt and clean some register
	    mgc_clean_context(pThis);
	}
	
	// Confirm all urb list has been 
	for(i = 0; i < 2; i ++)
	{
		for(bEnd=0; bEnd < MGC_GET_EPS_NUM(pThis); bEnd++)
		{
		    pEnd = &(pThis->aLocalEnd[i][bEnd]); /*0: Rx, 1: Tx*/

			 //giveback all urbs
			 while(!MGC_IsEndIdle(pEnd)){	
				INFO("[Host Ep%d] urb list is %s.\n", bEnd, i? "Tx": "Rx");
				pUrb = MGC_GetNextUrb(pEnd);
				MGC_DequeueEndurb(pThis, pEnd, pUrb);
			 } 
		}
	}
	
	spin_unlock_irq(&pThis->SelectHcdLock);	
	mgc_resumtcnt = 0;
	mgc_berror = 0;
	#ifdef MUSB_SUPPORT_SKIP_SUSPEND
	prev_value = mt53xx_pm_suspend_cnt;
	printk("[USB%d]: pre_value=%d end\n", pThis->bPortNum, prev_value);
	#endif
	return 0;
}

static int mgc_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct usb_hcd  *hcd = dev_to_hcd(&pdev->dev);
	MGC_LinuxCd *pThis = hcd_to_musbstruct(hcd);
	MUSB_LinuxController *pController;
	unsigned int diff = 1;
	uint32_t dwLineState;
#if defined(CONFIG_ARCH_MT5891) || defined(CONFIG_ARCH_MT5882)
	uint32_t u4Reg = 0;
	uint8_t bReg = 0;
#endif
	#ifdef MUSB_SUPPORT_SKIP_SUSPEND
	INFO("[USB%d]: %d : %d\n", pThis->bPortNum, prev_value, mt53xx_pm_suspend_cnt);
	if (prev_value != mt53xx_pm_suspend_cnt)
	{
	    diff = 1;
	}
	else
		diff = 0;
	#else
	INFO("[USB%d].\n", pThis->bPortNum);
	#endif
	mgc_resumtcnt++;
	mgc_berror = 0;
	mgc_ep0kicktimes = 0;
	mgc_ep0callbacktimes = 0;

	spin_lock_irq(&pThis->SelectHcdLock);	

	//resume root hub
	usb_hcd_resume_root_hub(hcd);

#if defined(CONFIG_ARCH_MT5891)
	if (pThis->bPortNum == 3) {
	      /* Switch Port-D to core power domain  */
	      INFO("Re-Switch Port-D to core power domain\n");
	      u4Reg = MGC_PHY_Read32(pUSBpdwncIoMap, 0x08);
	      u4Reg &= ~(0x1 << 31);
	      MGC_PHY_Write32(pUSBpdwncIoMap, 0x08, u4Reg);

	      /* Re-Switch Port-D MAC clock to 120M  */
	      INFO("Re-Switch standby MAC clock to 120M clock\n");
	      u4Reg = MGC_PHY_Read32(pUSBpdwncIoMap, 0xe0);
	      u4Reg &= ~(0xc);
	      MGC_PHY_Write32(pUSBpdwncIoMap, 0xe0, u4Reg);

	      /* USB host return to Resume mode */
	      INFO("Return to Resume Mode\n");
	      bReg = MGC_Read8(pThis->pRegs, MGC_O_HDRC_POWER);
	      bReg &= ~(MGC_M_POWER_SUSPENDM);
	      bReg |= (MGC_M_POWER_RESUME);
	      MGC_Write8(pThis->pRegs, MGC_O_HDRC_POWER, bReg);
	      /*
	       * CPU should clear the RESUME bit after 10 ms to end Resume signal.
	       * In Host Mode, this bit is also automaticlly set Resume Signalling
	       * from the target is detected while the USB2.0 controller is suspended.
	       **/
	      mdelay(10);
	      bReg = MGC_Read8(pThis->pRegs, MGC_O_HDRC_POWER);
	      bReg &= ~(MGC_M_POWER_RESUME);
	      MGC_Write8(pThis->pRegs, MGC_O_HDRC_POWER, bReg);
	} else
#elif defined (CONFIG_ARCH_MT5882)
	if (pThis->bPortNum == 3 && IS_IC_MT5885()) {
	      /* Switch Port-D to core power domain  */
	      INFO("Re-Switch Port-D to core power domain\n");
	      u4Reg = MGC_PHY_Read32(pUSBpdwncIoMap, 0x08);
	      u4Reg &= ~(0x1 << 31);
	      MGC_PHY_Write32(pUSBpdwncIoMap, 0x08, u4Reg);

	      /* Re-Switch Port-D MAC clock to 120M  */
	      INFO("Re-Switch standby MAC clock to 120M clock\n");
	      u4Reg = MGC_PHY_Read32(pUSBpdwncIoMap, 0xe8);
	      u4Reg &= ~(0x3);
	      MGC_PHY_Write32(pUSBpdwncIoMap, 0xe8, u4Reg);

	      /* USB host return to Resume mode */
	      INFO("Return to Resume Mode\n");
	      bReg = MGC_Read8(pThis->pRegs, MGC_O_HDRC_POWER);
	      bReg &= ~(MGC_M_POWER_SUSPENDM);
	      bReg |= (MGC_M_POWER_RESUME);
	      MGC_Write8(pThis->pRegs, MGC_O_HDRC_POWER, bReg);
	      /*
	       * CPU should clear the RESUME bit after 10 ms to end Resume signal.
	       * In Host Mode, this bit is also automaticlly set Resume Signalling
	       * from the target is detected while the USB2.0 controller is suspended.
	       **/
	      mdelay(10);
	      bReg = MGC_Read8(pThis->pRegs, MGC_O_HDRC_POWER);
	      bReg &= ~(MGC_M_POWER_RESUME);
	      MGC_Write8(pThis->pRegs, MGC_O_HDRC_POWER, bReg);
	} else
#endif
	{
	    pController = MUCUST_GetControllerDev(pThis->bPortNum);    
	    if (diff == 1)
	    {
            pController->MGC_pfPhyReset(pController) ;
            /* Now update root hub port state according linesate */
            dwLineState = MGC_PHY_Read32(pThis->pPhyBase, M_REG_U2PHYDMON1);
            dwLineState &= M_REG_LINESTATE_MASK;
            if ((pThis->bInsert) && (dwLineState == LINESTATE_DISCONNECT)) {
                printk("%s---> Detected a Disconnect envent\n", __func__);
                pThis->dwVirtualHubPortStatus = (1 << USB_PORT_FEAT_POWER) | (1 << USB_PORT_FEAT_C_CONNECTION);
                pThis->dwVirtualHubPortStatus &= ~(1 << USB_PORT_FEAT_CONNECTION);
                usb_hcd_poll_rh_status(hcd);
                pThis->bInsert = FALSE;
            }
	    }
	    else
	    {
		printk("skip USB MGC_pfPhyReset\n");
	    }
	    //prevent hcd->state is not HC_STATE_RUNNING when USB_IRQ generate.
	    mod_timer(&pThis->events_timer1, jiffies + msecs_to_jiffies(20));
	    pThis->events_1 = USB_EVENTS_BUS_RESUME;
	}

	spin_unlock_irq(&pThis->SelectHcdLock);	
	return 0;
}


static const struct dev_pm_ops mgc_dev_pm_ops = {
	.suspend	= mgc_suspend,
	.resume 	= mgc_resume,
};

#define MGC_DEV_PM_OPS (&mgc_dev_pm_ops)
struct device_type usb_host_type = 
{
	.name		= "host",
	.release	= NULL,
	.groups		= NULL,
	.uevent		= NULL,
	.pm		    = MGC_DEV_PM_OPS,
};
#endif


#ifdef CONFIG_MUSB_PM
int usbsuspend(struct device *dev, pm_message_t state)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct usb_hcd  *hcd = dev_to_hcd(&pdev->dev);
	MGC_LinuxCd *pThis = hcd_to_musbstruct(hcd);
	
    if(!pdev)
    {
      printk("unexpected usb error");
      return (-1);
    }
    if(pdev->id >= MUCUST_GetControllerNum())
    {
      printk("unexpected usb error");
      return (-1);
    }
      
	uint32_t u4Reg = 0;

    u4Reg = MGC_PHY_Read32(pThis->pPhyBase,0x68);
    u4Reg |=  0x00040000; 
    MGC_PHY_Write32(pThis->pPhyBase, 0x68, u4Reg);
    return 0;
}

int usbresume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct usb_hcd  *hcd = dev_to_hcd(&pdev->dev);
	MGC_LinuxCd *pThis = hcd_to_musbstruct(hcd);
	uint32_t u4Reg = 0;

	u4Reg = MGC_PHY_Read32(pThis->pPhyBase,0x68);
	u4Reg &=	~0x00040000; 
	MGC_PHY_Write32(pThis->pPhyBase, 0x68, u4Reg);

    return 0;
}

struct dev_pm_ops usbdev_pm_ops = {
		.prepare = NULL,
		.complete = NULL,
		.suspend = usbsuspend,
		.resume = usbresume,
		.freeze = NULL,     
		.thaw = NULL,
		.poweroff = NULL,
		.restore = NULL,
};

struct device_type usb_host_type = {
	.name		= "host",
	.release	       = NULL,
	.groups		= NULL,
	.uevent		= NULL,
	.pm		= &usbdev_pm_ops,
};

#endif


static struct platform_driver MUC_hcd_driver = 
{
    .probe = MUC_hcd_probe, 
    .remove = MUC_hcd_remove, 
    .suspend = MUC_hcd_suspend, 
    .resume = MUC_hcd_resume, 
    .driver = 
    {
        .name = (char*)MUC_HcdName, 
        .owner = THIS_MODULE, 
    } , 
};

static struct hc_driver MUC_hc_driver = 
{
    .description = MUC_HcdName, 
    .hcd_priv_size = sizeof(MGC_LinuxCd), 

#ifdef CONFIG_USB_MT53xx_COMPLIANCE_TEST
    /* mtk debugfs api */
    .mtk_debugfs_cb = mtk_usb_in_testmode,
#endif

    /*
     * generic hardware linkage
     */
    .irq = MUC_Irq, 
    .flags = HCD_USB2 | HCD_MEMORY, 

    /* Basic lifecycle operations */
    .start = MUC_start, 
    .stop = MUC_stop, 

    /*
     * managing i/o requests and associated device resources
     */
    .urb_enqueue = MUC_urb_enqueue, 
    .urb_dequeue = MUC_urb_dequeue, 
    .endpoint_disable = MUC_endpoint_disable, 

    /*
     * periodic schedule support
     */
    .get_frame_number = MUC_get_frame, 

    /*
     * root hub support
     */
    .hub_status_data = MUC_hub_status_data, 
    .hub_control = MUC_hub_control, 
    .bus_suspend = MUC_bus_suspend, 
    .bus_resume = MUC_bus_resume, 
};


static char *MGC_DecodeUrbProtocol(struct urb *pUrb)
{
    static char buffer[8];

    if (!pUrb)
    {
        strcpy(&buffer[0], "NULL");

        return buffer;
    } 

    buffer[0] = usb_pipein(pUrb->pipe) ? 'I' : 'O';

    if (usb_pipeint(pUrb->pipe))
    {
        strcpy(&buffer[1], " int");
    }
    else if (usb_pipeisoc(pUrb->pipe))
    {
        strcpy(&buffer[1], " isoc");
    }
    else if (usb_pipebulk(pUrb->pipe))
    {
        strcpy(&buffer[1], " bulk");
    }
    else if (usb_pipecontrol(pUrb->pipe))
    {
        strcpy(&buffer[0], " ctl");
    }

    return buffer;
}

int MGC_CheckTxCsrValidForWrite(MGC_LinuxCd *pThis, uint8_t bEnd, uint8_t bState)
{
    uint32_t dwCtrl;
    uint8_t bChannel;
    void *pBase = pThis->pRegs;
    MGC_HsDmaController *pDmaController;
    if(MGC_GetDebugEnalbe())
    {
        pDmaController = &pThis->rDma;

        // Assert when this condition happens :  Write EP TxCSR when the EP is doing DMA. 
        if (bEnd)
        {
            for (bChannel = 0; bChannel < pDmaController->bChannelCount; bChannel++)
            {
                dwCtrl = MGC_Read32(pBase, MGC_HSDMA_CHANNEL_OFFSET(bChannel, MGC_O_HSDMA_CONTROL));
                if ((((dwCtrl &0xf0) >> 4) == bEnd) && ((dwCtrl&0x3) == 0x3))
                {
                    INFO("[USB]ASSERT check error! bEnd=%x, TxCSR=0x%x bState = 0x%x!\n", bEnd, dwCtrl, bState);
                    MUSB_ASSERT(0);
                    return 0;
                }
            }
        }
    }

    return 1;
}

#ifdef USB_NEW_URB_LIST
static inline MGC_LinuxUrbList* MGC_AllocUrbList(MGC_LinuxUrbList* list_buf)
{
  int i;
  MGC_LinuxUrbList* pUrbList;
  for(i=0; i< USB_MAX_URB_NUM; i++)
  {
    	pUrbList = list_buf+i;
	if(!pUrbList->used){
	   pUrbList->used = 1;
       return pUrbList;
	}	
  }
  return NULL;
}

static inline void MGC_FreeUrbList(MGC_LinuxUrbList* pUrbList)
{
   pUrbList->used = 0;
}
#endif

#ifdef SUPPORT_SHARE_USBQ_DMA
int MGC_CheckIsUseDMAChannel(struct urb *pUrb, uint32_t dwLength)
{	
	if (dwLength <= MGC_HSDMA_MIN_DMA_LEN)
	{
		return FALSE;
	}  

	if(pUrb->dev->speed == USB_SPEED_FULL)
	{
		return FALSE;
	}
	return TRUE;

}
#endif

int MGC_IsEndIdle(MGC_LinuxLocalEnd *pEnd)
{
    if (pEnd->pCurrentUrb)
    {
        return FALSE;
    }

    return list_empty(&pEnd->list);
}

struct urb *MGC_GetNextUrb(MGC_LinuxLocalEnd *pEnd)
{
    MGC_LinuxUrbList *pUrbList;
    struct urb *pUrb = NULL;

    if (pEnd->pCurrentUrb)
    {
        pUrb = pEnd->pCurrentUrb;
    } 
    else
    {
        MUSB_ASSERT(list_empty(&pEnd->list) == FALSE);
        pUrbList = list_entry(pEnd->list.next, MGC_LinuxUrbList, list);
        if (pUrbList)
        {            
            pUrb = pUrbList->pUrb;
            if (pUrb)
            {                
                pEnd->pCurrentUrb = pUrb;                        
            }        
            list_del(&pUrbList->list);
            #ifndef USB_NEW_URB_LIST
            kfree (pUrbList);            
            #else
            MGC_FreeUrbList(pUrbList);
            #endif          
        }
    }

    /*
    if (pUrb)
    {
        printk("[USB]Next pUrb=0x%08X, size=%d.\n", (uint32_t)pUrb, pUrb->transfer_buffer_length);
    }
    else
    {
        printk("[USB]Next pUrb=NULL.\n");
    }
     */

    return pUrb;
}

struct urb *MGC_GetCurrentUrb(MGC_LinuxLocalEnd *pEnd)
{
    return pEnd->pCurrentUrb;
} 

static void MGC_ClearEnd(MGC_LinuxLocalEnd *pEnd)
{
    pEnd->dwOffset = 0;
    pEnd->dwRequestSize = 0;
    pEnd->dwIsoPacket = 0;
    pEnd->bRetries = 0;
    pEnd->bTrafficType = 0;
}

static int MGC_EnqueueEndUrb(MGC_LinuxCd *pThis, MGC_LinuxLocalEnd *pEnd, struct urb *pUrb)
{
    struct usb_hcd  *hcd = musbstruct_to_hcd(pThis);
    MGC_LinuxUrbList *pUrbList;
    int rc;

    rc = usb_hcd_link_urb_to_ep(hcd, pUrb);
    if (rc) 
    {
        return rc;
    }
 
    if ((pEnd->pCurrentUrb == NULL) && (list_empty(&pEnd->list) == TRUE))
    {
        // Only put pUrb to pCurrentUrb when pCurrentUrb and list are both empty.
        pEnd->pCurrentUrb = pUrb;
    } 
    else
    {
        #ifndef USB_NEW_URB_LIST
        pUrbList = kzalloc(sizeof(MGC_LinuxUrbList), GFP_ATOMIC); 
        if (!pUrbList)
        {        
    	    usb_hcd_unlink_urb_from_ep(hcd, pUrb);			
            return USB_ST_NOMEMORY;
        }
        #else
         //avoid to alloc memory in critical session
	    if(!pEnd->list_buf)
	    {
	      pEnd->list_buf = (MGC_LinuxUrbList*)kzalloc(USB_MAX_URB_NUM * sizeof(MGC_LinuxUrbList), pEnd->mem_flags);
		  if(!pEnd->list_buf)
		  {
		    return USB_ST_NOMEMORY; 
		  }
		  memset((void*)pEnd->list_buf, 0, USB_MAX_URB_NUM*sizeof(MGC_LinuxUrbList));
	    }

        pUrbList = MGC_AllocUrbList(pEnd->list_buf);
        if (!pUrbList)
        {
            return USB_ST_NOMEMORY;
        }
        #endif
        pUrbList->pUrb = pUrb;
        list_add_tail(&pUrbList->list, &pEnd->list);
    }
    // Add pUrb to pEnd structure.
    pUrb->hcpriv = pEnd;

    return 0;
}

void MGC_DequeueEndurb(MGC_LinuxCd *pThis, MGC_LinuxLocalEnd *pEnd, struct urb *pUrb)
{
    MGC_LinuxUrbList *pUrbList;
    MGC_LinuxUrbList *pNextUrbList;

    // Remove pUrb to pEnd structure.
    pUrb->hcpriv = NULL;

    // Dequeue urb from it's endpoint list.
    usb_hcd_unlink_urb_from_ep(musbstruct_to_hcd(pThis), pUrb);   

    if (pEnd->pCurrentUrb == pUrb)
    {
        pEnd->pCurrentUrb = NULL;
        MGC_ClearEnd(pEnd);
    } 
    else
    {
        list_for_each_entry_safe(pUrbList, pNextUrbList, &pEnd->list, list)
        {
            if (pUrbList->pUrb == pUrb)
            {
                list_del(&pUrbList->list);
                #ifndef USB_NEW_URB_LIST
                kfree(pUrbList);
                #else
                MGC_FreeUrbList(pUrbList);
                #endif
                break;
            }
        }
    }
}

static void MGC_InitEnd(MGC_LinuxCd *pThis, uint8_t bEndpoint_Tx_num, uint8_t bEndpoint_Rx_num)
{
    uint8_t bEnd;
    uint8_t bTx;
	uint8_t bEndCount = 0;
    MGC_LinuxLocalEnd *pEnd;
    void *pBase = pThis->pRegs;
   
    /* use the defined end points */
    pThis->bEndTxCount = bEndpoint_Tx_num;
    pThis->bEndRxCount = bEndpoint_Rx_num;	
	
    if ((pThis->bEndTxCount > MGC_GET_TX_EPS_NUM(pThis)) || (pThis->bEndRxCount > MGC_GET_RX_EPS_NUM(pThis)))
    {
        INFO("Endpoint[Tx:%d][Rx:%d]array out of boundary.\n", pThis->bEndTxCount, pThis->bEndRxCount);
        return;
    }

    bEndCount = (pThis->bEndTxCount > pThis->bEndRxCount) ? pThis->bEndTxCount : pThis->bEndRxCount;
 	
    for (bEnd=0; bEnd < bEndCount; bEnd++)
    {        
        MGC_SelectEnd(pBase, bEnd);
        MGC_Write8(pBase, MGC_O_HDRC_TXFIFOSZ, 3);  // 64 bytes.
        MGC_Write8(pBase, MGC_O_HDRC_RXFIFOSZ, 3);                
        MGC_Write16(pBase, MGC_O_HDRC_TXFIFOADD, 0);
        MGC_Write16(pBase, MGC_O_HDRC_RXFIFOADD, 0);

        pThis->wEndMask |= (1 << bEnd);
        for (bTx = EPRX; bTx <= EPTX; bTx++)
        {
            pEnd = &(pThis->aLocalEnd[bTx][bEnd]);
            pEnd->bEnd = bEnd;
            pEnd->bIsTx = bTx;
            pEnd->wPacketSize = 0;
            
            if (bEnd==0)
            {
                pEnd->wFifoAddress = 0;
                pEnd->wMaxPacketSize = MGC_END0_FIFOSIZE;
				pEnd->wFIFOSize = MGC_END0_FIFOSIZE;
                pEnd->bDoublePacketBuffer = FALSE;
            }
            else
            {
                pEnd->wFifoAddress = MGC_END0_FIFOSIZE; 
                pEnd->wMaxPacketSize = 0;  /* EPx is not occupied */
				pEnd->wFIFOSize = 0;
                pEnd->bDoublePacketBuffer = FALSE;                
            }
            
            pEnd->pCurrentUrb = NULL;
            INIT_LIST_HEAD(&pEnd->list);
            pEnd->wPacketSize = 0;
            pEnd->bRemoteAddress = 0;
            pEnd->bRemoteEnd = 0;
            pEnd->bTrafficType = 0;
        }        
    }
}

static void usb_events_timer_func (unsigned long _pThis)
{
    uint8_t nDevCtl = 0;
    MGC_LinuxCd *pThis = (MGC_LinuxCd *)_pThis;
    MUSB_LinuxController *pController;
    uint8_t *pBase = (uint8_t*)pThis->pRegs;
    
	if(pThis->events & USB_EVENTS_UNLINK_INVALID_URB)
	{
	  //INFO("[usb]USB_EVENTS_UNLINK_INVALID_URB\n");
      #ifdef MGC_UNLINK_URB
	  MGC_UnlinkInvalidUrb(_pThis);
      #endif
	  pThis->events &= ~USB_EVENTS_UNLINK_INVALID_URB;
	}	

	if(pThis->events & USB_EVENTS_CHECK_CONNECT)
	{
	  INFO("[USB%d]USB_EVENTS_CHECK_CONNECT\n", pThis->bPortNum);
	  MGC_CheckConnect(_pThis);
	  pThis->events &= ~USB_EVENTS_CHECK_CONNECT;
	}
	
	if(pThis->events & USB_EVENTS_RESET_OFF)
	{
	  DBG(-1, "[USB%d]USB_EVENTS_RESET_OFF\n", pThis->bPortNum);
	  MGC_ResetOff(_pThis);
	  pThis->events &= ~USB_EVENTS_RESET_OFF;
	}

	pController = MUCUST_GetControllerDev(pThis->bPortNum);    
	if(pThis->events & USB_EVENTS_VBUS_ON)
	{		
		// Turn on vbus.
		//INFO("[usb]USB_EVENTS_VBUS_ON\n");
		pController->MGC_pfVbusControl(pThis->bPortNum, 1);
		pThis->events &= ~USB_EVENTS_VBUS_ON;
	}

    if(pThis->events & USB_EVENTS_HOST_SESSION_ON)
    {
        // open session.
        nDevCtl = MGC_Read8(pBase, MGC_O_HDRC_DEVCTL);   
        nDevCtl |= MGC_M_DEVCTL_SESSION;        
        MGC_Write8(pBase, MGC_O_HDRC_DEVCTL, nDevCtl);
        pThis->events &= ~USB_EVENTS_HOST_SESSION_ON;

        // turn on vbus.
        if (pController->MGC_pfVbusControl)
        {
            // After open session, Wait 4000 ms to turn on vbus.
            mod_timer(&pThis->events_timer, jiffies + msecs_to_jiffies(2000));      
            pThis->events |= USB_EVENTS_VBUS_ON;
        }
        
    }
    
	return;

}
static void usb_events_timer1_func (unsigned long _pThis)
{
#if defined(CONFIG_PM) || defined(CC_SUPPORT_STR) 
	MGC_LinuxCd *pThis = (MGC_LinuxCd *)_pThis;
	struct platform_device *pdev = to_platform_device(pThis->dev);
	struct usb_hcd	*hcd = dev_to_hcd(&pdev->dev);

	if(pThis->events_1 & USB_EVENTS_BUS_RESUME)
	{
		pThis->events_1 &= ~USB_EVENTS_BUS_RESUME;
		INFO("[USB%d]USB_EVENTS_BUS_RESUME\n", pThis->bPortNum);
		spin_lock_irq(&pThis->SelectHcdLock);	
		mgc_restore_context(pThis);
		set_bit(HCD_FLAG_HW_ACCESSIBLE, &hcd->flags);
		spin_unlock_irq(&pThis->SelectHcdLock);
		
		#ifdef MUSB_SUPPORT_SUSPEND_BOOT_IDENTIFY
		mod_timer(&pThis->events_timer1, 
					  jiffies + msecs_to_jiffies(5000));
		pThis->events_1 |= USB_EVENTS_HOST_SUSPEND_DONE;
		goto done;
		#endif
	}
	#ifdef MUSB_SUPPORT_SUSPEND_BOOT_IDENTIFY
	if(pThis->events_1 & USB_EVENTS_HOST_SUSPEND_DONE)
	{
		INFO("[USB%d]USB_EVENTS_HOST_SUSPEND_DONE. \n", pThis->bPortNum);
		pThis->events_1 |= USB_EVENTS_HOST_SUSPEND_DONE;
		usb_hotboot = 0;
		goto done;
	}
	#endif
	done:
#endif
	return;
}

#ifdef MGC_UNLINK_URB
static void MGC_UnlinkInvalidUrb(unsigned long pParam)
{
    MGC_LinuxCd *pThis = (MGC_LinuxCd*)pParam;
    struct usb_hcd *hcd = musbstruct_to_hcd(pThis);
    int i, j, k = 0;
    struct urb *pUrb = NULL;
	uint8_t bEndCount;

    if (!pThis)
    {
        return ;
    }

    if (!hcd)
    {
        return ;
    }

    for (i = 0; i < 2; i++)
    {
    	bEndCount = i? pThis->bEndRxCount : pThis->bEndTxCount;
        for (j = 0; (j < bEndCount); j++)
        {
            pUrb = MGC_GetCurrentUrb(&(pThis->aLocalEnd[i][j]));
            if (!pUrb)
            {
                continue;
            }
            if (usb_pipecontrol(pUrb->pipe) || usb_pipebulk(pUrb->pipe))
            {
                if (pUrb->setup_packet)
                {
                    //INFO("[USB]to be unlink request is 0x%02X\n", ((struct usb_ctrlrequest*)(pUrb->setup_packet))->bRequest);
                }
                if ((ulong)pUrb->dev == (ulong)hcd->self.root_hub->children[0] || 
					(ulong)pUrb->dev->parent == (ulong)hcd->self.root_hub->children[0] || 
					(ulong)pUrb->dev->parent == (ulong)hcd->self.root_hub)
                {
                    MUC_urb_dequeue(hcd, pUrb,  - EFAULT);
                    continue;
                }
                if (!hcd->self.root_hub->children[0])
                {
                    continue;
                }
                for (k = 0; k < hcd->self.root_hub->children[0]->maxchild; k++)
                {
                    if ((ulong)hcd->self.root_hub->children[0]->children[k] == (ulong)pUrb->dev)
                    {
                        MUC_urb_dequeue(hcd, pUrb,  - EFAULT);
                        continue;
                    }
                }
            }
        }
    }
#ifdef USB_NEW_URB_LIST
	MGC_FreeEndpointListBuf(hcd,FALSE);   
#endif
}
#endif 

#ifdef USB_NEW_URB_LIST
static void MGC_FreeEndpointListBuf(struct usb_hcd * hcd,uint8_t force_free)
{
  int i,j;
  MGC_LinuxCd *pThis = hcd_to_musbstruct(hcd);
  MGC_LinuxLocalEnd *pEnd;
  unsigned long flags;
  uint8_t bEndCount;
  
  if(!pThis)
  {
    printk("[USB]pThis is NULL\n");
    return;
  }

  
  spin_lock_irqsave(&pThis->SelectEndLock, flags);
  for(i = 0; i < 2; i ++)
  {
    	bEndCount = i? pThis->bEndRxCount : pThis->bEndTxCount;
        for (j = 0; (j < bEndCount); j++)
      {
        pEnd = &(pThis->aLocalEnd[i][j]);
	    if(MGC_IsEndIdle(pEnd) || force_free)
		{
			if(pEnd->list_buf)
			{
                kfree(pEnd->list_buf);
				pEnd->list_buf = NULL;
			}
		}
	    else
		{
            printk("[usb]endpoint#%d is busy... !!!\n",pEnd->bEnd);
			continue;
	    }		
      }
  }
  spin_unlock_irqrestore(&pThis->SelectEndLock, flags);
  
}
#endif

static void MGC_StartTx(MGC_LinuxCd *pThis, uint8_t bEnd)
{
    uint8_t *pBase = (uint8_t*)pThis->pRegs;

    MGC_CheckTxCsrValidForWrite(pThis, bEnd, 0x01);

    if (bEnd)
    {
        MGC_WriteCsr16(pBase, MGC_O_HDRC_TXCSR, bEnd, MGC_M_TXCSR_TXPKTRDY);
    }
    else
    {
        if (MGC_END0_START == pThis->bEnd0Stage)
        {
            MGC_WriteCsr16(pBase, MGC_O_HDRC_CSR0, 0, 
                MGC_M_CSR0_H_NO_PING | MGC_M_CSR0_H_SETUPPKT | MGC_M_CSR0_TXPKTRDY);
        }
        else
        {
            MGC_WriteCsr16(pBase, MGC_O_HDRC_CSR0, 0, MGC_M_CSR0_TXPKTRDY);
        }        
    }
}

static uint32_t MGC_Log2(uint32_t x)
{
    uint32_t i;

    for (i = 0; x > 1; i++)
    {
        x = x / 2;
    }

    return i;
}

static uint16_t MGC_SelectFifoSize(uint16_t wPacketSize)
{
    /* Setting    Fifo size
        4'b0000: 8
        4'b0001: 16
        4'b0010: 32
        4'b0011: 64
        4'b0100: 128
        4'b0101: 256
        4'b0110: 512
        4'b0111: 1024
        4'b1000: 2048 (Single-packet buffering only)
        4'b1001: 4096 (Single-packet buffering only)
    */
    if (wPacketSize <= 8)
    {
        return 0;
    }
    else if ((wPacketSize > 8) && (wPacketSize <= 16))
    {
        return 1;
    }
    else if ((wPacketSize > 16) && (wPacketSize <= 32))
    {
        return 2;
    }
    else if ((wPacketSize > 32) && (wPacketSize <= 64))
    {
        return 3;
    }
    else if ((wPacketSize > 64) && (wPacketSize <= 128))
    {
        return 4;
    }
    else if ((wPacketSize > 128) && (wPacketSize <= 256))
    {
        return 5;
    }
    else if ((wPacketSize > 256) && (wPacketSize <= 512))
    {
        return 6;
    }
    else if ((wPacketSize > 512) && (wPacketSize <= 1024))
    {
        return 7;
    }
    else if ((wPacketSize > 1024) && (wPacketSize <= 2048))
    {
        return 8;
    }
    else // if (wPacketSize > 2048)
    {
        return 9;
    }
}

static MGC_LinuxLocalEnd *MGC_FindEnd(MGC_LinuxCd *pThis, struct urb *pUrb)
{
	void *pBase = pThis->pRegs;
	MGC_LinuxLocalEnd *pEnd = NULL;    
	MGC_LinuxLocalEnd *pDbgEnd = NULL;		  
	unsigned int nOut = usb_pipeout(pUrb->pipe);
	uint16_t wPacketSize = usb_maxpacket(pUrb->dev, pUrb->pipe, nOut);
	struct usb_device *dev = pUrb->dev;
	struct usb_host_endpoint *ep = pUrb->ep;
	//struct usb_device_descriptor *pDescriptor = &dev->descriptor;
	struct usb_driver *udriver = to_usb_driver(dev->dev.driver);
	
	uint8_t bAddTransaction;   
	uint16_t wStartAddr = MGC_END0_FIFOSIZE;
	uint16_t wSize = 0;
	uint16_t i, j;
	uint8_t bIndex;
	uint8_t bEnd = 0;
	uint8_t bEndCount = 0;
	uint16_t wFIFOSize=8;
	
	DBG(3, "[USB]<== pUrb=%p\n", pUrb);

	/* control is always EP0, and can always be queued */
	if (usb_pipecontrol(pUrb->pipe))
	{
		DBG(3, "[USB]==> is a control pipe use ep0\n");
		pEnd = &(pThis->aLocalEnd[EP0][0]); /*0: Rx, 1: Tx*/

	#ifdef MGC_PRE_CHECK_FREE_ENDPOINT
		pEnd->bIsOccupy = TRUE;
	#endif		
		return pEnd;
	} 

	if (ep->hcpriv)
	{
		pEnd = (MGC_LinuxLocalEnd*)ep->hcpriv;
	#ifdef MGC_PRE_CHECK_FREE_ENDPOINT
		pEnd->bIsOccupy = TRUE;
	#endif
		return pEnd;
	}

	#ifdef MGC_RX_LENGTH_HANDLE
	if((wPacketSize <= 100) && (usb_pipeisoc(pUrb->pipe)) && (!nOut))
		wPacketSize *= EXTEND_BUFFER_TIMES;
	#endif
	// Handle high speed isochronous transfer, multiple packet part.
	bAddTransaction = (wPacketSize >> 11) & 0x3;
	wPacketSize &= 0x7ff;
	if (bAddTransaction == 1)
	{
		wPacketSize = 2*wPacketSize;
	}
	else if (bAddTransaction == 2)
	{
		wPacketSize = 3*wPacketSize;
	}

	wPacketSize = (wPacketSize < 8) ? 8: wPacketSize;

	
/*	INFO("[USB] Port-%d: New Dev=0x%lX, bRemoteEp=%d, proto=%s, wPacketSize=%d.\n", 
		pThis->bPortNum, (ulong)dev, usb_pipeendpoint(pUrb->pipe), MGC_DecodeUrbProtocol(pUrb), wPacketSize);
	INFO("[USB] Port-%d: idVendor=0x%04lX, idProduct=0x%04lX, bcdDevice=0x%04lX.\n", 
		pThis->bPortNum, (ulong)pDescriptor->idVendor, 
		(ulong)pDescriptor->idProduct, (ulong)pDescriptor->bcdDevice);
*/	
	if(MGC_WIFIHotbootSupported(pUrb))
	{
		if(pThis->bPortNum==MUSB_WIFI_PORT)
			is_wifi_onport3=1;

		//INFO("hotboot mtk_get_usb_suspend = %d ,is_wifi_onport3=1.\n", mtk_get_usb_suspend());
		if(mtk_get_usb_suspend() 
			&&(strncmp(udriver->name,MTK_WiFiDriverName,((sizeof(udriver->name)>WifiDriverNameLen)?WifiDriverNameLen:sizeof(udriver->name)))==0))
		{
			mtk_set_usb_suspend(0);
		}
	}

	if ((pThis->bEndTxCount > MGC_GET_TX_EPS_NUM(pThis)) || (pThis->bEndRxCount > MGC_GET_RX_EPS_NUM(pThis)))
	{
		INFO("Endpoint[Tx:%d][Rx:%d]array out of boundary.\n", pThis->bEndTxCount, pThis->bEndRxCount);
		return NULL;
	}

	/* 20150423 Leon wStartAddr must 8 byte align.
	  * but if for high PacketSize, e.g. Camera PacketSize=3072, wFIFOSize will set to 4096, 
	  * cause USB FIFO not enough error.
	  * direct return wFIFOSize = wPacketSize if >= 1K to save USB FIFO size to support more devices
	  */
	wSize = MGC_SelectFifoSize(wPacketSize);

	if(wSize < 8)   // <  1K
	{
		for(i=0; i < wSize; i++)
			wFIFOSize *=2;
	}
	else
		wFIFOSize = wPacketSize;
	bEnd = mtk_select_endpoint(pThis, pUrb);

	if (bEnd == 0)
	{
		INFO("[USB] No endpoint left.\n");
		return NULL;
	}

	// Occupy this fifo address.
	pEnd = &(pThis->aLocalEnd[nOut][bEnd]); /*0: Rx, 1: Tx*/ 
	if (pEnd->bDoublePacketBuffer) {
	    wFIFOSize *=2;
	}
	wStartAddr = mtk_get_fifo(pThis, wFIFOSize);
	if (!wStartAddr) {
		INFO("[USB] Out of fifo boundary.\n");
		return NULL;
	}
	INFO("Port-%d: %s EP %d  Get Fifo Addr = %x, size = %x\n",
	    pThis->bPortNum, nOut ? "Tx" : "Rx", bEnd, wStartAddr, wFIFOSize);
	pEnd->wFifoAddress = wStartAddr;
	pEnd->wMaxPacketSize = wPacketSize;
	pEnd->wFIFOSize=wFIFOSize;
#ifdef MGC_PRE_CHECK_FREE_ENDPOINT
	pEnd->bIsOccupy = TRUE;
#endif
	mtk_dump_fifo_node(pThis);
	ep->hcpriv = (void*)pEnd;
			
	/* save index */
	bIndex = MGC_Read8(pBase, MGC_O_HDRC_INDEX);
	MGC_SelectEnd(pBase, bEnd);
	if (nOut)
	{
		wSize = (pEnd->bDoublePacketBuffer) ? (wSize | 0x10) : wSize;
		MGC_Write8(pBase, MGC_O_HDRC_TXFIFOSZ, wSize);
		MGC_Write16(pBase, MGC_O_HDRC_TXFIFOADD, wStartAddr >> 3);
	}
	else
	{
		wSize = (pEnd->bDoublePacketBuffer) ? (wSize | 0x10) : wSize;
		MGC_Write8(pBase, MGC_O_HDRC_RXFIFOSZ, wSize);
		MGC_Write16(pBase, MGC_O_HDRC_RXFIFOADD, wStartAddr >> 3);		  
	}

	/* restore index */
	MGC_Write8(pBase, MGC_O_HDRC_INDEX, bIndex);

	if(MGC_GetDebugEnalbe())
	{
	// Debug information for all fifo allocation.	 
	for (i=1; i< bEndCount; i++)
	{
		for (j=0; j<2; j++)
		{
			pDbgEnd = &(pThis->aLocalEnd[j][i]); /*0: Rx, 1: Tx*/ 
			printk("[USB] Port-%d: %s Ep %d use fifo at %d, size=%d, DB=%d for RemoteEp=%d.\n", 
				pThis->bPortNum, ((j) ? "Tx" : "Rx"), i, 
				pDbgEnd->wFifoAddress, pDbgEnd->wMaxPacketSize, 
				pDbgEnd->bDoublePacketBuffer, usb_pipeendpoint(pUrb->pipe));
		}
	}
	}
	pEnd->bCmdQEnable = FALSE;
	return pEnd;
}

void MGC_CallbackUrb(MGC_LinuxCd *pThis, struct urb *pUrb)
{
#ifdef MUC_DMA_DISABLE
    if (usb_pipein(pUrb->pipe) && usb_pipetype(pUrb ->pipe) != PIPE_CONTROL) {
        void *ptr;
        for (ptr = pUrb ->transfer_buffer;
             ptr < pUrb ->transfer_buffer + pUrb ->transfer_buffer_length;
             ptr += PAGE_SIZE)
                flush_dcache_page(virt_to_page(ptr));
    }
#endif

    usb_hcd_giveback_urb(musbstruct_to_hcd(pThis), pUrb, pUrb->status);

    if (pUrb->status)
    {
        DBG(2, "[USB]done completing pUrb=%p, status=%d.\n", pUrb, pUrb->status);
    }
}

void MGC_CompleteUrb(MGC_LinuxCd *pThis, MGC_LinuxLocalEnd *pEnd, struct urb *pUrb)
{
	if(MGC_WIFIHotbootSupported(pUrb))
	{
		// make sure the wifi bulkin ep4 the receive data length is 4 algin.
		if(mgc_bulkinep4 &&(((uint8_t)usb_pipeendpoint(pUrb->pipe) ==4) && (usb_pipein(pUrb->pipe))))
		{
			if((pUrb->actual_length == 0) || (pUrb->actual_length % mgc_bulkinep4))
				INFO("[USB %d]pUrb=0x%p urb.length=%d,  actual_lenght=%d,proto=%s\n", mgc_bulkinep4, pUrb, pUrb->transfer_buffer_length, pUrb->actual_length, MGC_DecodeUrbProtocol(pUrb));
		}

		if(usb_pipecontrol(pUrb->pipe))
		{
			mgc_ep0callbacktimes ++;
			//if(mgc_ep0kicktimes != mgc_ep0callbacktimes)
				//INFO("[USB]WARNING... WIFI maybe has not callback between working, %d: %d.\n", mgc_ep0kicktimes, mgc_ep0callbacktimes);
		}

		//Leon DEBUG_PRINTK
		if((mgc_resumtcnt < mgc_printnum) && (((uint8_t)usb_pipeendpoint(pUrb->pipe) ==8)))
		{
			// if need printk bulk-out, need set mgc_bulkset = 1, mgc_bulkoutset=1 
			if(mgc_bulkset && usb_pipebulk(pUrb->pipe)) 
			{
				if(mgc_bulkoutset && (usb_pipein(pUrb->pipe)== 0))
				{
					//INFO("[USB %d]pUrb=0x%p on host.end=%d urb.length=%d, actual_lenght=%d, proto=%s\n", mgc_resumtcnt, pUrb, pEnd->bEnd, pUrb->transfer_buffer_length,pUrb->actual_length, MGC_DecodeUrbProtocol(pUrb));
					mgc_resumtcnt++;
				}		
			}
			
			// if print bulk in+out, set mgc_bulkset =1, mgc_bulkoutset=0
			if(mgc_bulkset && usb_pipebulk(pUrb->pipe)) 
			{
				if(mgc_bulkoutset == 0)
				{
					INFO("[USB %d]pUrb=0x%p on host.end=%d urb.length=%d,  actual_lenght=%d,proto=%s\n", mgc_resumtcnt, pUrb, pEnd->bEnd, pUrb->transfer_buffer_length, pUrb->actual_length, MGC_DecodeUrbProtocol(pUrb));
					mgc_resumtcnt++;
				}		
			}

			// printk control, set mgc_bulkset=0, mgc_bulkoutset=any
			if((mgc_bulkset == 0) && (usb_pipecontrol(pUrb->pipe)))
			{
				INFO("[USB %d]pUrb=0x%p on host.end=%d urb.length=%d,  actual_lenght=%d,proto=%s\n", mgc_resumtcnt, pUrb, pEnd->bEnd, pUrb->transfer_buffer_length, pUrb->actual_length, MGC_DecodeUrbProtocol(pUrb));
				mgc_resumtcnt++;
			}
		}
	}

    DBG(2, "[USB]Completing URB=0x%p, on pEnd->bEnd=%d status=%d, proto=%s\n", pUrb, pEnd->bEnd, pUrb->status, MGC_DecodeUrbProtocol(pUrb));

    if (pUrb->status)
    {
        DBG(2, "[USB]completing URB=%p, status=%d, len=%x\n", pUrb, pUrb->status, pUrb->actual_length);
    }

    MGC_CallbackUrb(pThis, pUrb);
} 

static inline uint8_t MGC_GetTransferType(struct urb *pUrb)
{
    uint8_t bStdType = 0;

    const unsigned int nPipe = pUrb->pipe;

    if (usb_pipeisoc(nPipe))
    {
        bStdType = 1;
    } 
    else if (usb_pipeint(nPipe))
    {
        bStdType = 3;
    }
    else if (usb_pipebulk(nPipe))
    {
        bStdType = 2;
    }

    return bStdType;
}

static void MGC_SetProtocol(MGC_LinuxCd *pThis, struct urb *pUrb, uint8_t bEnd, unsigned int bXmt)
{
    uint8_t reg;

    uint8_t bStdType = MGC_GetTransferType(pUrb);
    unsigned int nPipe = pUrb->pipe;
    uint8_t *pBase = (uint8_t*)pThis->pRegs;

    reg = (bStdType << 4) | (((uint8_t)usb_pipeendpoint(nPipe)) &0xf);

    switch (((uint8_t)pUrb->dev->speed))
    {
        case USB_SPEED_LOW:
            reg |= 0xc0;
            break;

        case USB_SPEED_FULL:
            reg |= 0x80;
            break;

        default:
            reg |= 0x40;
    }

    if (bXmt)
    {
        if (bEnd)
        {
            MGC_WriteCsr8(pBase, MGC_O_HDRC_TXTYPE, bEnd, reg);
        }
        else
        {
            MGC_WriteCsr8(pBase, MGC_O_HDRC_TYPE0, 0, reg &0xc0);
        }
    }
    else
    {
        if (bEnd)
        {
            // Note that although we invoke WriteCsr8, but it actually write 32bits.
            // Writing to MGC_O_HDRC_RXTYPE will actually affect MGC_O_HDRC_RXINTERVAL.
            // Therefore, here we have a check to avoid iso transfer interval be affected.
            if (usb_pipeisoc(nPipe))
            {
                uint8_t regvalue;
                regvalue = MGC_ReadCsr8(pBase, MGC_O_HDRC_RXTYPE, bEnd);
                if (regvalue != reg)
                {
                    MGC_WriteCsr8(pBase, MGC_O_HDRC_RXTYPE, bEnd, reg);
                }
            }
            else
            {
                MGC_WriteCsr8(pBase, MGC_O_HDRC_RXTYPE, bEnd, reg);
            }
        }
        else
        {
            MGC_WriteCsr8(pBase, MGC_O_HDRC_TYPE0, 0, reg &0xc0);
        }
    }
}

static void MGC_SetAddress(MGC_LinuxCd *pThis, struct urb *pUrb, uint8_t bEnd, unsigned int bXmt)
{
    uint8_t *pBase = (uint8_t*)pThis->pRegs;
    uint8_t bAddress = (uint8_t)usb_pipedevice(pUrb->pipe);
    uint8_t bHubAddr = 0, bHubPort = 0;
    struct usb_device	*top_dev;

    /* NOTE: there is always a parent due to the virtual root hub */
    if (pUrb->dev->parent)
    {
		for (top_dev = pUrb->dev->parent; top_dev->parent && top_dev->parent->parent && (top_dev->speed != USB_SPEED_HIGH);
				top_dev = top_dev->parent)
			/* Found device below root hub */;
			bHubAddr = (uint8_t)top_dev->devnum;
    } 
	
    /*  HS MTT Hub + Low/Full Speed Device */
    if (pUrb->dev->tt)
    {
        // several deep hub, need attention the hubport
		bHubPort = (uint8_t)pUrb->dev->ttport;
        if(pUrb->dev->tt->multi)
        {
            bHubAddr |= 0x80;
        }
    }    
    /* target addr & hub addr/port */
    if (bXmt)
    {
        MGC_Write8(pBase, MGC_BUSCTL_OFFSET(bEnd, MGC_O_MHDRC_TXFUNCADDR), bAddress);
        MGC_Write8(pBase, MGC_BUSCTL_OFFSET(bEnd, MGC_O_MHDRC_TXHUBADDR), bHubAddr);
        MGC_Write8(pBase, MGC_BUSCTL_OFFSET(bEnd, MGC_O_MHDRC_TXHUBPORT), bHubPort);
    }

    /* also, try Rx (this is a bug ion the core: I always need to to do 
     * both (at least for ep0), needs to be changed when the core is
     * fixed */
    if ((bEnd == 0) || (!bXmt))
    {
        MGC_Write8(pBase, MGC_BUSCTL_OFFSET(bEnd, MGC_O_MHDRC_RXFUNCADDR), bAddress);
        MGC_Write8(pBase, MGC_BUSCTL_OFFSET(bEnd, MGC_O_MHDRC_RXHUBADDR), bHubAddr);
        MGC_Write8(pBase, MGC_BUSCTL_OFFSET(bEnd, MGC_O_MHDRC_RXHUBPORT), bHubPort);
    }

    DBG(3, "[USB]end %d, device %d, parent %d, port %d, speed:%d\n", bEnd, pUrb->dev->devnum, bHubAddr, bHubPort, pUrb->dev->speed);
}

#ifndef CONFIG_USB_MT53xx_COMPLIANCE_TEST
static
#endif
void MGC_LoadFifo(const uint8_t *pBase, uint8_t bEnd, uint16_t wCount, const uint8_t *pSource)
{    
    uint32_t dwDatum = 0;

    uint32_t dwCount = wCount;
    uint8_t bFifoOffset = MGC_FIFO_OFFSET(bEnd);
    uint32_t dwBufSize = 4;

    //  do not handle zero length data.
    if (dwCount == 0)
    {
        return ;
    }

    /* byte access for unaligned */
    if ((dwCount > 0) && ((ulong)pSource &3))
    {
        while (dwCount)
        {
            if (3 == dwCount || 2 == dwCount)
            {
                dwBufSize = 2;             
     		    MU_MB();
                // set FIFO byte count.
                MGC_FIFO_CNT(pBase, M_REG_FIFOBYTECNT, 1);
            }
            else if(1 == dwCount)
            {
                dwBufSize = 1;             
     		    MU_MB();
                // set FIFO byte count.
                MGC_FIFO_CNT(pBase, M_REG_FIFOBYTECNT, 0);               
            }

            memcpy((void *)&dwDatum, (const void *)pSource, dwBufSize);
     		MU_MB();
            MGC_Write32(pBase, bFifoOffset, dwDatum);

            dwCount -= dwBufSize;
            pSource += dwBufSize;
        }
    }
    else /* word access if aligned */
    {
        while ((dwCount > 3) && !((ulong)pSource &3))
        {
            MU_MB();
            MGC_Write32(pBase, bFifoOffset, *((uint32_t*)((void*)pSource)));

            pSource += 4;
            dwCount -= 4;
        }
        if (3 == dwCount || 2 == dwCount)
        {
            MUSB_ASSERT(dwCount < 4);
    
            // set FIFO byte count.
            MU_MB();
            MGC_FIFO_CNT(pBase, M_REG_FIFOBYTECNT, 1);
    
            MU_MB();
            MGC_Write32(pBase, bFifoOffset, *((uint32_t *)((void *)pSource)));
            dwCount -= 2;
            pSource += 2;
        }
        
        if(1 == dwCount)
        {
			MU_MB();
            MGC_FIFO_CNT(pBase, M_REG_FIFOBYTECNT, 0);
            if((ulong)pSource & 3)
            {
            
                memcpy((void *)&dwDatum, (const void *)pSource, 1);
				MU_MB();
                MGC_Write32(pBase, bFifoOffset, dwDatum);            
            }
            else
            {
				MU_MB();
                MGC_Write32(pBase, bFifoOffset, *((uint32_t *)((void *)pSource)));            
            }
        }
    }
	MU_MB();
    MGC_FIFO_CNT(pBase, M_REG_FIFOBYTECNT, 2);
    MU_MB();
    return ;
}

void MGC_UnloadFifo(const uint8_t *pBase, uint8_t bEnd, uint16_t wCount, uint8_t *pDest)
{   
    uint32_t dwDatum = 0;

    uint32_t dwCount = wCount;
    uint8_t bFifoOffset = MGC_FIFO_OFFSET(bEnd);
    uint32_t i;

    //  do not handle zero length data.
    if (dwCount == 0)
    {
        return ;
    }

	MU_MB();
    if (((ulong)pDest) &3)
    {
        /* byte access for unaligned */
        while (dwCount > 0)
        {
            if (dwCount < 4)
            {            
                if(3 == dwCount || 2 == dwCount)
                {
                   MGC_FIFO_CNT(pBase, M_REG_FIFOBYTECNT, 1);
					MU_MB();
                   dwDatum = MGC_Read32(pBase, bFifoOffset);
				   MU_RMB();
                   
                   for (i = 0; i < 2; i++)
                   {
                       *pDest++ = ((dwDatum >> (i *8)) & 0xFF);
                   }                   
                   dwCount -=2;
                }
                if(1 == dwCount) 
                {
                    MGC_FIFO_CNT(pBase, M_REG_FIFOBYTECNT, 0);
				    MU_MB();
                    dwDatum = MGC_Read32(pBase, bFifoOffset);
				    MU_RMB();
                    *pDest++ = (dwDatum  & 0xFF);
                    dwCount -= 1;
                }    
                
                // set FIFO byte count = 4 bytes.
                MGC_FIFO_CNT(pBase, M_REG_FIFOBYTECNT, 2);
                /*use rmb() to save time for iso transfer*/
				MU_RMB();
                dwCount = 0;
            }
            else
            {
                dwDatum = MGC_Read32(pBase, bFifoOffset);
                /*use rmb() to save time for iso transfer*/
				MU_RMB();

                for (i = 0; i < 4; i++)
                {
                    *pDest++ = ((dwDatum >> (i *8)) &0xFF);
                }

                dwCount -= 4;
            }
        }
    }
    else
    {
        /* word access if aligned */
        while (dwCount >= 4)
        {
            *((uint32_t*)((void*)pDest)) = MGC_Read32(pBase, bFifoOffset);

		    /*use rmb() to save time for iso transfer*/
		    MU_RMB();

            pDest += 4;
            dwCount -= 4;
        }

        if (dwCount > 0)
        {    
            if(3 == dwCount ||2 == dwCount )
            {
                MGC_FIFO_CNT(pBase, M_REG_FIFOBYTECNT, 1);
		  MU_MB();
                dwDatum = MGC_Read32(pBase, bFifoOffset);
		  MU_RMB();
                for (i = 0; i < 2; i++)
                 {
                     *pDest++ = ((dwDatum >> (i *8)) & 0xFF);
                 }
                dwCount -= 2;
            }

            if(1 == dwCount)
            {
               MGC_FIFO_CNT(pBase, M_REG_FIFOBYTECNT,0);
		 MU_MB();
               dwDatum = MGC_Read32(pBase, bFifoOffset);
		 MU_RMB();
               
                *pDest++ = (dwDatum & 0xFF);
                dwCount -= 1;               
            }
                
            // set FIFO byte count = 4 bytes.
            MGC_FIFO_CNT(pBase, M_REG_FIFOBYTECNT, 2);
	     MU_MB();
        }
    }

    return ;
}

void MGC_CleanDcache(struct urb* urb,ulong start,ulong len)
{
    uint32_t phy;
    
    if(!(urb->transfer_flags & URB_NO_TRANSFER_DMA_MAP) && virt_addr_valid(start))
    {  
        phy = dma_map_single(urb->dev->bus->controller,(void *)start, len, DMA_TO_DEVICE);
        dma_unmap_single(urb->dev->bus->controller,(dma_addr_t)phy, len, DMA_TO_DEVICE);             
    }
}

static void MGC_SetInterval(MGC_LinuxCd *pThis, struct urb *pUrb, uint8_t bEnd)
{
    uint8_t *pBase = (uint8_t*)pThis->pRegs;

    unsigned int nPipe = pUrb->pipe;
    uint8_t bInterval = 0;

    if (usb_pipeint(nPipe))
    {
		/* 20121029- Leon.Lin
		* Porting from musb_host for device interval's process flow
		*/
		/*
		* Full/low speeds use the  linear encoding,
		* high speed uses the logarithmic encoding.
		*/
		if (pUrb->dev->speed <= USB_SPEED_FULL) {
			bInterval = max_t(u8, pUrb->interval, 1);
		}
		else
		{
			 bInterval = MGC_Log2(pUrb->interval) + 1;
		}
    }
    else if (usb_pipeisoc(nPipe))
    {
        bInterval = MGC_Log2(pUrb->interval) + 1;
    }
    else if (usb_pipebulk(nPipe) && (pUrb->interval > 0))
    {
        if (pThis->bRootSpeed == USB_SPEED_HIGH) /* high speed */
        {
            if (pUrb->interval > 4096)
            {
                bInterval = 16;
            }
            else
            {
                bInterval = MGC_Log2(pUrb->interval) + 1;
            }
        }
        else if (pThis->bRootSpeed == USB_SPEED_FULL) /* full speed */
        {
            if (pUrb->interval > 32768)
            {
                bInterval = 16;
            }
            else
            {
                bInterval = MGC_Log2(pUrb->interval) + 1;
            }
        }
    }
#ifdef TEST_INTERVAL 
		// only for bulk-out device endpoint 4.
		if(mgc_setinterval && ((uint8_t)usb_pipeendpoint(pUrb->pipe) ==4) && usb_pipebulk(pUrb->pipe) && usb_pipeout(nPipe))
		{
			bInterval = mgc_setinterval;
		}
	
		if((mgc_resumtcnt < mgc_printnum) && ((uint8_t)usb_pipeendpoint(pUrb->pipe) ==4)&& usb_pipebulk(pUrb->pipe) && usb_pipeout(nPipe))
		{
			INFO("[%d]URB[bRdEnd=%d] interval old  = %d/0x%x, new = %d.\n", mgc_resumtcnt, usb_pipeendpoint(pUrb->pipe), MGC_ReadCsr8(pBase, (MGC_O_HDRC_TXINTERVAL), bEnd), MGC_ReadCsr16(pBase, (MGC_O_HDRC_TXINTERVAL - 1), bEnd), bInterval);
		}
#endif

    if (usb_pipeout(nPipe))
    {    
        if(MGC_ReadCsr8(pBase, MGC_O_HDRC_TXINTERVAL, bEnd) != bInterval)
        {
            MGC_WriteCsr8(pBase, MGC_O_HDRC_TXINTERVAL, bEnd, bInterval);
        }
    }
    else
    {
        // Everytime when we write to MGC_O_HDRC_RXINTERVAL, there would be 1 frame/micro-frame delay.
        // MGC_O_HDRC_RXINTERVAL should be written only when we want to change interval.
        if (MGC_ReadCsr8(pBase, MGC_O_HDRC_RXINTERVAL, bEnd) != bInterval)
        {
            MGC_WriteCsr8(pBase, MGC_O_HDRC_RXINTERVAL, bEnd, bInterval);
        }
    }
}

/**
 * used ONLY in host mode, I'll be moved to musb_host
 * @param pPrivateData
 * @param bLocalEnd
 * @param bTransmit
 */
static uint8_t MGC_HsDmaChannelStatusChanged(MGC_LinuxCd *pThis, 
    uint8_t bLocalEnd, uint8_t bTransmit)
{
    if (!bLocalEnd)
    {
        MGC_ServiceDefaultEnd(pThis);
    }
    else
    {
        /* endpoints 1..15 */
        if (bTransmit)
        {
            MGC_ServiceTxAvail(pThis, bLocalEnd);
        }
        else
        {
            /* receive */
            MGC_ServiceRxReady(pThis, bLocalEnd);
        }
    }

    return TRUE;
}

static MGC_HsDmaChannel *MGC_HsDmaAllocateChannel(
    struct urb *pUrb, MGC_HsDmaController *pController, uint8_t bLocalEnd, uint8_t bTransmit, 
    uint32_t dwLength, uint8_t bProtocol, uint16_t wMaxPacketSize)
{
    MGC_HsDmaChannel *pImplChannel = NULL;
#ifndef MUC_DMA_DISABLE       
    uint8_t bBit;
    MGC_LinuxCd *pThis = (MGC_LinuxCd *)pController->pThis;

	#ifdef SUPPORT_SHARE_USBQ_DMA
	if(!MGC_CheckIsUseDMAChannel(pUrb, dwLength))
	{
		return NULL;
	}
	#else
   	if ((dwLength <= MGC_HSDMA_MIN_DMA_LEN)||(pUrb->transfer_dma==0))
    {
        return NULL;
    }       
	#endif

    for (bBit = 0; bBit < pController->bChannelCount; bBit++)
    {
        if (!(pController->bmUsedChannels & (1 << bBit)))
        {
            pController->bmUsedChannels |= (1 << bBit);

            pImplChannel = &(pController->aChannel[bBit]);
            pImplChannel->pController = (void *)pController;
            pImplChannel->dwStartAddress = 0;
            pImplChannel->dwCount = 0;
            pImplChannel->dwActualLength = 0;
            pImplChannel->wMaxPacketSize = wMaxPacketSize;
            pImplChannel->bStatus = MGC_DMA_STATUS_FREE;
            pImplChannel->bDesiredMode = 0;
            pImplChannel->bIndex = bBit;
            pImplChannel->bEnd = bLocalEnd;
            pImplChannel->bProtocol = bProtocol;
            pImplChannel->bTransmit = bTransmit;
            break;
        }
    }
	if(MGC_GetDebugEnalbe()){
	    if ((!pImplChannel) && (pThis->bSupportCmdQ))
	    {
	        INFO("[USB]ASSERT No DMA. bEnd=%d, bProtocol=%d, bTransmit=%d, dwlength=0x%x.\n", bLocalEnd, bProtocol, bTransmit, dwLength);
	    }
	}
#endif

    return pImplChannel;
}

static void MGC_HsDmaReleaseChannel(MGC_HsDmaChannel *pChannel)
{
    MGC_HsDmaController *pController = (MGC_HsDmaController *)pChannel->pController;
    MGC_LinuxCd *pThis = (MGC_LinuxCd *)pController->pThis;
    uint8_t *pBase = (uint8_t*)pThis->pRegs;
    uint8_t bEnd = pChannel->bEnd;
    uint8_t bChannel = pChannel->bIndex;
    uint16_t wCsr = 0; 
    uint32_t bIntr;
    
    bIntr = MGC_Read8(pBase, MGC_O_HSDMA_INTR);
    if (bIntr & (1<< bChannel))
    {
        //  write clear interrupt register value.
        MGC_Write8(pBase, MGC_O_HSDMA_INTR, (1 << bChannel));
    }

    MGC_Write32(pBase, MGC_HSDMA_CHANNEL_OFFSET(bChannel, MGC_O_HSDMA_CONTROL), 0);

    // clear TX/RX CSR register.
    if (pChannel->bTransmit)
    {
        wCsr = MGC_ReadCsr16(pBase, MGC_O_HDRC_TXCSR, bEnd);

        if (wCsr &(MGC_M_TXCSR_AUTOSET | MGC_M_TXCSR_DMAENAB | MGC_M_TXCSR_DMAMODE))
        {
            wCsr &= ~(MGC_M_TXCSR_AUTOSET | MGC_M_TXCSR_DMAENAB | MGC_M_TXCSR_DMAMODE);

            MGC_CheckTxCsrValidForWrite(pThis, bEnd, 0x02);
            MGC_WriteCsr16(pBase, MGC_O_HDRC_TXCSR, bEnd, wCsr);
        }
    }
    else
    {
        wCsr = MGC_ReadCsr16(pBase, MGC_O_HDRC_RXCSR, bEnd);

        if (wCsr &(MGC_M_RXCSR_AUTOCLEAR | MGC_M_RXCSR_H_AUTOREQ | 
                MGC_M_RXCSR_DMAENAB | MGC_M_RXCSR_DMAMODE))
        {
            wCsr &= ~(MGC_M_RXCSR_AUTOCLEAR | MGC_M_RXCSR_H_AUTOREQ | 
                MGC_M_RXCSR_DMAENAB | MGC_M_RXCSR_DMAMODE);

            MGC_WriteCsr16(pBase, MGC_O_HDRC_RXCSR, bEnd, wCsr);
        }
    }

    pController->bmUsedChannels &= ~(1 << bChannel);
    pChannel->bStatus = MGC_DMA_STATUS_FREE;
}

static uint8_t MGC_HsDmaProgramChannel(MGC_HsDmaChannel *pChannel)
{
#ifndef MUC_DMA_DISABLE
    MGC_HsDmaController *pController = (MGC_HsDmaController *)pChannel->pController;
    MGC_LinuxCd *pThis = (MGC_LinuxCd *)pController->pThis;
    uint8_t *pBase = (uint8_t*)pThis->pRegs;
    uint8_t bEnd = pChannel->bEnd;
    uint8_t bChannel = pChannel->bIndex;
    uint16_t wMaxPacketSize = pChannel->wMaxPacketSize;
    uint8_t bMode = pChannel->bDesiredMode;
    uint8_t *pBuffer = (uint8_t *)(ulong)pChannel->dwStartAddress;
    uint32_t dwLength = pChannel->dwCount;
    uint32_t dwCsr = (bEnd << MGC_S_HSDMA_ENDPOINT) | (1 << MGC_S_HSDMA_ENABLE);
    uint16_t wCsr = 0;

    #if defined(CONFIG_ARCH_MT5396) && defined(CONFIG_MT53XX_USE_CHANNELB_DRAM)  
    if ((ulong)(pBuffer)+ dwLength >= mt53xx_cha_mem_size)
    {
        INFO("[USB]incorrect address 0x%p %d\n", pBuffer, dwLength);
    }
    #endif
    if (bMode)
    {
        dwCsr |= 1 << MGC_S_HSDMA_MODE1;
    }

    dwCsr |= MGC_M_HSDMA_BURSTMODE;
    
    if (pChannel->bTransmit)
    {   
        //  prevent client task and USB HISR race condition, set csr in MGC_HsDmaProgramChannel().
        wCsr = MGC_ReadCsr16(pBase, MGC_O_HDRC_TXCSR, bEnd);

        if (bMode)
        {            
            wCsr |= (MGC_M_TXCSR_AUTOSET | MGC_M_TXCSR_DMAENAB | MGC_M_TXCSR_DMAMODE);
        }
        else
        {
            wCsr &= ~(MGC_M_TXCSR_AUTOSET | MGC_M_TXCSR_DMAENAB | MGC_M_TXCSR_DMAMODE);
        }

        MGC_CheckTxCsrValidForWrite(pThis, bEnd, 0x03);
        MGC_WriteCsr16(pBase, MGC_O_HDRC_TXCSR, bEnd, wCsr);
        dwCsr |= 1 << MGC_S_HSDMA_TRANSMIT;
    }
    else
    {
        if (bMode)
        {
            wCsr = MGC_ReadCsr16(pBase, MGC_O_HDRC_RXCSR, bEnd);

            wCsr |= (MGC_M_RXCSR_AUTOCLEAR | MGC_M_RXCSR_DMAENAB);
            //  Request the actual number of packet to be received.
            MGC_DMA_Write32(pBase, M_REG_REQPKT(bEnd), (((dwLength + wMaxPacketSize) - 1) / wMaxPacketSize));

            wCsr &= ~MGC_M_RXCSR_RXPKTRDY;
            // host use MGC_M_RXCSR_DMAMODE.
            wCsr |= (MGC_M_RXCSR_H_AUTOREQ | MGC_M_RXCSR_DMAMODE | MGC_M_RXCSR_H_REQPKT);
        }
    }

    dwCsr |= 1 << MGC_S_HSDMA_IRQENABLE;

    /* address/count */
    MGC_Write32(pBase, MGC_HSDMA_CHANNEL_OFFSET(bChannel, MGC_O_HSDMA_ADDRESS), (ulong)pBuffer);
    MGC_Write32(pBase, MGC_HSDMA_CHANNEL_OFFSET(bChannel, MGC_O_HSDMA_COUNT), dwLength);

    MU_MB();
    /* control (this should start things) */
    MGC_Write32(pBase, MGC_HSDMA_CHANNEL_OFFSET(bChannel, MGC_O_HSDMA_CONTROL), dwCsr);

    if (!pChannel->bTransmit)
    {
        /*
            Note: 
            RxCSR should be set after DMA is configured. 
            This can prevent race condition between setuping DMA and data entering fifo.
             */
        if (bMode)
        {
            MU_MB();
            MGC_WriteCsr16(pBase, MGC_O_HDRC_RXCSR, bEnd, wCsr);
        }
    }

    return TRUE;
#else
    return FALSE;
#endif
}

static MGC_DmaChannelStatus MGC_HsDmaGetChannelStatus(
    MGC_HsDmaChannel *pChannel)
{
    MGC_HsDmaController *pController = (MGC_HsDmaController *)pChannel->pController;
    MGC_LinuxCd *pThis = (MGC_LinuxCd *)pController->pThis;
    uint8_t *pBase = (uint8_t*)pThis->pRegs;
    uint8_t bChannel = pChannel->bIndex;
    uint32_t dwAddress;
    uint32_t dwCsr = MGC_Read32(pBase, MGC_HSDMA_CHANNEL_OFFSET(bChannel, MGC_O_HSDMA_CONTROL));
    uint32_t dwCsrMask;

    if (dwCsr &(1 << MGC_S_HSDMA_BUSERROR))
    {
        INFO("[USB]MGC_S_HSDMA_BUSERROR !\n");
        return MGC_DMA_STATUS_BUS_ABORT;
    }

    //  handle last short packet in multiple packet DMA RX mode 1.  
    dwCsrMask = (1 << MGC_S_HSDMA_ENABLE) | (1 << MGC_S_HSDMA_MODE1) | (1 << MGC_S_HSDMA_IRQENABLE);

    if ((dwCsr &0xf) == dwCsrMask)
    {
        /* most DMA controllers would update the count register for simplicity... */
        dwAddress = MGC_Read32(pBase, 
            MGC_HSDMA_CHANNEL_OFFSET(bChannel, MGC_O_HSDMA_ADDRESS));

        pChannel->dwActualLength = dwAddress - pChannel->dwStartAddress;

        MGC_Write32(pBase, 
            MGC_HSDMA_CHANNEL_OFFSET(bChannel, MGC_O_HSDMA_CONTROL), 0);
        
        return MGC_DMA_STATUS_MODE1_SHORTPKT;
    }

    /* most DMA controllers would update the count register for simplicity... */
    dwAddress = MGC_Read32(pBase, 
        MGC_HSDMA_CHANNEL_OFFSET(bChannel, MGC_O_HSDMA_ADDRESS));

    if (dwAddress < (pChannel->dwStartAddress + pChannel->dwCount))
    {
        INFO("[USB]MGC_DMA_STATUS_BUSY, 0x%08X, 0x%08X, 0x%08X.\n", 
            dwAddress, pChannel->dwStartAddress, pChannel->dwCount);
        return MGC_DMA_STATUS_BUSY;
    }

    return MGC_DMA_STATUS_FREE;
}

static uint8_t MGC_HsDmaControllerIsr(MGC_HsDmaController *pController)
{
    MGC_LinuxCd *pThis = (MGC_LinuxCd *)pController->pThis;
    uint8_t *pBase = (uint8_t*)pThis->pRegs;
    MGC_HsDmaChannel *pChannel;
    uint8_t bChannel;
    uint32_t dwCsr;
    uint32_t dwAddress;
    uint8_t bIntr;
    uint8_t bIntrMask;
    uint8_t bEnd;    
    uint16_t wVal;
    MGC_LinuxLocalEnd *pEnd; 

    bIntr = MGC_Read8(pBase, MGC_O_HSDMA_INTR);
    bIntrMask = MGC_Read8(pBase, MGC_O_HSDMA_INTR_MASK);
    bIntr = (bIntr & bIntrMask);
    if (!bIntr)
    {
        return FALSE;
    }
    
    MGC_Write8(pBase, MGC_O_HSDMA_INTR, bIntr);  

    for (bChannel = 0; bChannel < pController->bChannelCount; bChannel++)
    {    
        if ((bIntr &(1 << bChannel)) == 0)
        {        
            continue;        
        }
        
        pChannel = (MGC_HsDmaChannel*) &(pController->aChannel[bChannel]);
        bEnd = pChannel->bEnd;
        
        dwCsr = MGC_Read32(pBase, MGC_HSDMA_CHANNEL_OFFSET(bChannel, MGC_O_HSDMA_CONTROL));

        if (dwCsr &(1 << MGC_S_HSDMA_BUSERROR))
        {
            INFO("[USB]ISR Bus ERROR !\n");
            pChannel->bStatus = MGC_DMA_STATUS_BUS_ABORT;
        }
        else
        {
            /* most DMA controllers would update the count register for simplicity... */
            dwAddress = MGC_Read32(pBase, MGC_HSDMA_CHANNEL_OFFSET(bChannel, MGC_O_HSDMA_ADDRESS));

            pChannel->bStatus = MGC_DMA_STATUS_FREE;
            pChannel->dwActualLength = dwAddress - pChannel->dwStartAddress;

            if (pChannel->bTransmit)
            {
                /*
                    NOTICE:
                    Because we use shared double packet buffer for mass storage.
                    It can not let tx/rx traffic happend at the same time.
                    So we need to queue all urb (tx/rx) on the same list.
                    It will prevent tx/rx data corruption when tx/rx transfer at the same time.
                    Always use EPMSD direction of pEnd for mass storage.
		  */
                pEnd =  ((0 == bEnd) ? (&(pThis->aLocalEnd[EP0][0])) :  (&(pThis->aLocalEnd[EPTX][bEnd])));
                if (pEnd->bDoublePacketBuffer)
                {    
                    /* Notice: double packet buffer fifo need to check fifo empty */
                    // enable ep tx int after DMA int. If FIFONOTEMPTY is set, then change to req mode 0 to generate ep tx interrupt  
                    wVal = MGC_ReadCsr16(pBase, MGC_O_HDRC_TXCSR, bEnd);     
					if((wVal & (MGC_M_TXCSR_FIFONOTEMPTY|MGC_M_TXCSR_TXPKTRDY)))
                    {                        
                        wVal &= ~MGC_M_TXCSR_DMAMODE;
                        MGC_WriteCsr16(pBase, MGC_O_HDRC_TXCSR, bEnd, wVal); 
                    }
                }
                       
                //  send last short packet in multiple packet transfer and tx single packet.
                if ( (pChannel->dwCount % pChannel->wMaxPacketSize) || 
                     (pChannel->dwCount <= pChannel->wMaxPacketSize) )
                {
                    MGC_StartTx(pThis, bEnd);
                    continue;
                }
            }

            MGC_HsDmaChannelStatusChanged(pThis, bEnd, pChannel->bTransmit);
        }
    }

    return TRUE;
}

static uint8_t MGC_TxPacket(MGC_LinuxCd *pThis, MGC_LinuxLocalEnd *pEnd)
{
    uint16_t wLength = 0;
    uint8_t bDone = FALSE;
    uint8_t *pBase;
    struct urb *pUrb;
    uint8_t *pBuffer;
    int nPipe;
    uint8_t bEnd;
    int i;
    struct usb_iso_packet_descriptor *packet;
    MGC_HsDmaChannel *pDmaChannel;
    uint8_t *pDmaBuffer;
    uint8_t bDmaOk;
	uint8_t bZLP = FALSE;   // zero-length packet
    MGC_DmaChannelStatus DmaChannelStatus = MGC_DMA_STATUS_UNKNOWN;

    if ((!pThis) || (!pEnd))
    {
        return TRUE;
    } 

    pBase = (uint8_t*)pThis->pRegs;
    pUrb = MGC_GetCurrentUrb(pEnd);
    nPipe = pUrb->pipe;
    bEnd = pEnd->bEnd;
    pEnd->dwOffset += pEnd->dwRequestSize;

    DBG(3, "[USB]<== bEnd=%d\n", bEnd);

    /* see if more transactions are needed */
    if (pEnd->pDmaChannel)
    {
        pDmaChannel = pEnd->pDmaChannel;    
        DmaChannelStatus = MGC_HsDmaGetChannelStatus(pDmaChannel);
        MUSB_ASSERT(MGC_DMA_STATUS_FREE == DmaChannelStatus);

        if (usb_pipeisoc(nPipe))
        {
            packet = &pUrb->iso_frame_desc[pEnd->dwIsoPacket];                
            MUSB_ASSERT(pEnd->dwRequestSize >= packet->length);               
            packet->status = 0;
            packet->actual_length = packet->length;
            pEnd->dwIsoPacket++;
            packet ++;

            if (pEnd->dwIsoPacket < pUrb->number_of_packets)
            {
                pDmaBuffer = (uint8_t*)pUrb->transfer_dma;
                pDmaBuffer += packet->offset;
                wLength = packet->length;

                // Iso is always single packet to send.
                pDmaChannel->dwStartAddress = (ulong)pDmaBuffer;
                pDmaChannel->dwCount = (ulong)wLength;
                pDmaChannel->dwActualLength = 0;
                pDmaChannel->bDesiredMode = 0;
                pEnd->dwRequestSize = wLength;
                bDmaOk = MGC_HsDmaProgramChannel(pDmaChannel);
                if (!bDmaOk)
                {
                    MGC_HsDmaReleaseChannel(pDmaChannel);
                    pEnd->pDmaChannel = NULL;
                    pEnd->dwRequestSize = 0;
                }
                else
                {
                    // Wait DMA finish and set TXPKTRDY at DMA ISR.
                    return FALSE;
                }
            }
            else
            {
                // Urb finish.
                pUrb->status = 0;
                return TRUE;
            }
        }
    }

    if (usb_pipeisoc(nPipe))
    {
        /* isoch case */
        if (pEnd->dwIsoPacket >= pUrb->number_of_packets)
        {
            for (i = 0; i < pUrb->number_of_packets; i++)
            {
                packet = &pUrb->iso_frame_desc[i];
                packet->status = 0;
                packet->actual_length = packet->length;
            }

            bDone = TRUE;
        }
        else
        {
            pBuffer = pUrb->transfer_buffer;
            packet = &pUrb->iso_frame_desc[pEnd->dwIsoPacket];
            pBuffer += packet->offset;
            wLength = packet->length;
            pEnd->dwIsoPacket++;
        }
    }
    else
    {
        pBuffer = pUrb->transfer_buffer;    
        pBuffer += pEnd->dwOffset;

        wLength = min((int)pEnd->wPacketSize, (int)(pUrb->transfer_buffer_length - pEnd->dwOffset));

        if (pEnd->dwOffset >= pUrb->transfer_buffer_length)
        {
            /* sent everything; see if we need to send a null */
            if ((pUrb->transfer_flags &USB_ZERO_PACKET) && 
                 (pEnd->dwRequestSize > 0) && 
                 ((pEnd->dwRequestSize % pEnd->wPacketSize) == 0))
            {
                // send null packet.
                pEnd->dwRequestSize = 0;
                bDone = FALSE;
				bZLP = TRUE;
                //INFO("[USB]Tx Send NULL Packet !\n");
            }
            else
            {
                bDone = TRUE;
            }
        }
    }

    if (bDone)
    {
        pUrb->status = 0;
    }
    else if ((wLength) || bZLP)
    {
         /* @assert bDone && !wLength */
        MGC_LoadFifo(pBase, bEnd, wLength, pBuffer);
        pEnd->dwRequestSize = wLength;

        MGC_CheckTxCsrValidForWrite(pThis, bEnd, 0x04);
        MGC_WriteCsr16(pBase, MGC_O_HDRC_TXCSR, bEnd, (MGC_M_TXCSR_TXPKTRDY));        

		if(MGC_WIFIHotbootSupported(pUrb) && (pEnd->bRemoteEnd == 8) && usb_pipebulk(pUrb->pipe)&&(usb_pipein(pUrb->pipe)== 0))
		{
			mgc_EP8_FIFO_TX_DONE = 1;		// means that the fifo data has been trigger to usb bus
			if(mgc_toggleErr_debug==1)
				INFO("ProgramEnd [%d][WIFI EP%d]** Set Toggle , length=%d	ToggleValue = 0x%x TXRDYPT = 0x%x.\n", mgc_togglecnt, pEnd->bRemoteEnd,pUrb->transfer_buffer_length,(MGC_Read32(pBase, M_REG_TXDATATOG)),MGC_ReadCsr16(pBase, MGC_O_HDRC_TXCSR, pEnd->bEnd));

		}
    }
    else
    {
        INFO("[USB]error : 0x%x, 0x%x, 0x%x, 0x%x !\n", pEnd->wPacketSize, pUrb->transfer_buffer_length, pEnd->dwOffset, pEnd->dwRequestSize);
    }

    return bDone;
}

static uint8_t MGC_RxPacket(MGC_LinuxCd *pThis, MGC_LinuxLocalEnd *pEnd, uint16_t wRxCount)
{
    uint16_t wLength;
    uint8_t bDone = FALSE;
    uint8_t *pBase;
    uint8_t bEnd;
    struct urb *pUrb;
    uint8_t *pBuffer;
    int nPipe;
    uint16_t wPacketSize;
    struct usb_iso_packet_descriptor *packet;

    if ((!pThis) || (!pEnd))
    {
        return TRUE;
    } 

    pBase = (uint8_t*)pThis->pRegs;
    pUrb = MGC_GetCurrentUrb(pEnd);
    
    nPipe = pUrb->pipe;
    bEnd = pEnd->bEnd;
    pBuffer = pUrb->transfer_buffer;
    
    DBG(3, "[USB]<== end %d RxCount=%04x\n", bEnd, wRxCount);
    DBG(3, "[USB]bEnd=%d, pUrb->transfer_flags=0x%x pUrb->transfer_buffer=%p\n", bEnd, pUrb->transfer_flags, pUrb->transfer_buffer);
    DBG(3, "[USB]pUrb->transfer_buffer_length=%d, pEnd->dwOffset=%d, wRxCount=%d\n", pUrb->transfer_buffer_length, pEnd->dwOffset, wRxCount);

    /* unload FIFO */
    if (usb_pipeisoc(nPipe))
    {
        /* isoch case */
        packet = &pUrb->iso_frame_desc[pEnd->dwIsoPacket];
        pBuffer += packet->offset;
        wLength = min((unsigned int)wRxCount, packet->length);
        pUrb->actual_length += wLength;

        /* update actual & status */
        packet->actual_length = wLength;
        packet->status = USB_ST_NOERROR;

        /* see if we are done */
        bDone = (++pEnd->dwIsoPacket >= pUrb->number_of_packets);

        DBG(3, "[USB]pEnd->dwIsoPacket=%d, pUrb->number_of_packets=%d, wLength=%d\n", pEnd->dwIsoPacket, pUrb->number_of_packets, wLength);

        if (wLength)
        {
            MGC_UnloadFifo(pBase, bEnd, wLength, pBuffer);
            MGC_CleanDcache(pUrb,(ulong)pBuffer, (ulong)wLength);
        }

        if (bEnd && bDone)
        {
            pUrb->status = 0;
        }
		#ifdef CONFIG_DEVICE_IDENTIFY_SUPPORT
		if(pUrb->dev)
        {
            if(MGC_IsUvcDevice(&pUrb->dev->descriptor,pUrb->dev) || 
               (MGC_IsUacDevice(&pUrb->dev->descriptor,pUrb->dev) && !MGC_IsMFIDevice(&pUrb->dev->descriptor,pUrb->dev)))

            {
                MGC_WriteCsr16(pBase, MGC_O_HDRC_RXCSR, bEnd, MGC_M_RXCSR_H_REQPKT);                
            }
        }
		#endif
    }
    else
    {
        DBG(3, "[USB](bEnd=%d), wRxCount=%d, pUrb->transfer_buffer_length=%d, pEnd->dwOffset=%d, pEnd->wPacketSize=%d\n", bEnd, wRxCount, pUrb->transfer_buffer_length, pEnd->dwOffset, pEnd->wPacketSize);

        /* non-isoch */
        pBuffer += pEnd->dwOffset;

        wLength = min((unsigned int)wRxCount, pUrb->transfer_buffer_length - pEnd->dwOffset);

        wPacketSize = usb_maxpacket(pUrb->dev, pUrb->pipe, FALSE);

        if (wLength > 0)
        {
            pUrb->actual_length += wLength;
            pEnd->dwOffset += wLength;

            MGC_UnloadFifo(pBase, bEnd, wLength, pBuffer);
	    MGC_CleanDcache(pUrb,(ulong)pBuffer, (ulong)wLength);
        }

        /* see if we are done */
        bDone = (pEnd->dwOffset >= pUrb->transfer_buffer_length) || (wRxCount < pEnd->wPacketSize);
    }

    MGC_WriteCsr16(pBase, MGC_O_HDRC_RXCSR, bEnd, 0);

    if (!bDone)
    {
         /* test for short packet */
        MU_MB();
        MGC_WriteCsr16(pBase, MGC_O_HDRC_RXCSR, bEnd, MGC_M_RXCSR_H_REQPKT);
    }

    if (bEnd && bDone)
    {
        pUrb->status = 0;
    }

    DBG(3, "[USB]==> bDone=%d\n", bDone);
    return bDone;
}

#ifdef CONFIG_USB_QMU_SUPPORT
static uint32_t MGC_IsCmdQSupport(MGC_LinuxCd *pThis, MGC_LinuxLocalEnd *pEnd, struct urb *pUrb)
{
    uint8_t bEnd;
    uint32_t nPipe;
    uint8_t bXmt;
    
    if ((!pThis)||(!pEnd) ||(!pUrb))
    {
        return 0;
    }

	if(!MGC_DeviceQmuSupported(pUrb))
	{
		return 0;
	}
    bEnd = pEnd->bEnd;
    nPipe = pUrb->pipe;
    bXmt = usb_pipeout(nPipe);

    if (usb_pipecontrol(nPipe))
    {
        return 0;
    }    

    if ((pThis->bSupportCmdQ) 
        && ((!bXmt) && (bEnd <= RXQ_NUM)) && usb_pipeisoc(nPipe))
    {
        return 1;
    }

    return 0;
}

static uint32_t MGC_IsCmdQEnable(MGC_LinuxCd *pThis, MGC_LinuxLocalEnd *pEnd, struct urb *pUrb)
{
    uint8_t *pBase;
    uint8_t bEnd;
    uint32_t nPipe;
    uint8_t bXmt;
    uint8_t bRemoteAddress;

    if ((!pThis)||(!pEnd) ||(!pUrb))
    {
        return 0;
    }

    bEnd = pEnd->bEnd;
    nPipe = pUrb->pipe;
    bXmt = usb_pipeout(nPipe);

    if (!QMU_is_enabled(pThis, bEnd, !bXmt))
    {
        return 0;
    }

    pBase = (uint8_t*)pThis->pRegs;
    bRemoteAddress = (bXmt) ? 
        MGC_Read8(pBase, MGC_BUSCTL_OFFSET(bEnd, MGC_O_MHDRC_TXFUNCADDR)) :
        MGC_Read8(pBase, MGC_BUSCTL_OFFSET(bEnd, MGC_O_MHDRC_RXFUNCADDR));

    /* Make sure device attribute need to reset or not */
    if (pEnd->bRemoteAddress != bRemoteAddress)
    {
        return 0;
    }

    return 1;
}

static void MGC_KickCmdQ(MGC_LinuxCd *pThis, MGC_LinuxLocalEnd *pEnd, struct urb *pUrb)
{
    uint16_t wIntr;
    uint32_t dwIntr;
    uint8_t *pBase = (uint8_t*)pThis->pRegs;
    unsigned int nPipe = pUrb->pipe;
    uint16_t wPacketSize = usb_maxpacket(pUrb->dev, nPipe, usb_pipeout(nPipe));
    uint8_t bEnd = pEnd->bEnd;
    uint8_t bXmt = pEnd->bIsTx;
    uint8_t bIsZlp;
    int i;
    unsigned int offset;
    uint8_t bIsIoc;
    //uint8_t *pBuffer;
	uint32_t pBuffer_addr;
    uint32_t dwLength;

    /* insert GPD! */
    if (!MGC_IsCmdQEnable(pThis, pEnd, pUrb))
    {
        /* remember software state - find_end() will use this - */
        pEnd->bRemoteAddress = (uint8_t)usb_pipedevice(nPipe);;
        pEnd->bRemoteEnd = (uint8_t)usb_pipeendpoint(nPipe);;
        pEnd->bTrafficType = (uint8_t)usb_pipetype(nPipe);
		#ifdef MGC_RX_LENGTH_HANDLE
		if((wPacketSize <= 100) && (usb_pipeisoc(pUrb->pipe)) && (!bXmt))
			wPacketSize *= EXTEND_BUFFER_TIMES;
		#endif

        /* init urb */
        pEnd->dwOffset = 0;
        pEnd->dwRequestSize = 0;
        pEnd->dwIsoPacket = 0;
        pEnd->bRetries = 0;
        pEnd->wPacketSize = wPacketSize;
        pEnd->bIsTx = bXmt;
    
        INFO("[USB] CmdQ: ep%d, dir=%d is enabled.\n", bEnd, bXmt);
        
        MGC_SetProtocol(pThis, pUrb, bEnd, bXmt);
        MGC_SetAddress(pThis, pUrb, bEnd, bXmt);

        if (bXmt) /* transmit */
        {
            /*
                IntrTxE, and IntrRx are the same 32 bits group.
                Tricky: Set 0 in all write clear field in IntrRx field. Prevent to clear IntrRx.
                Because our 32 bits register access mode.
            */
            wIntr = MGC_Read16(pBase, MGC_O_HDRC_INTRTXE);
            dwIntr = (ulong)((wIntr & (~(uint16_t)(1 << bEnd))) << 16);
            dwIntr &= 0xFFFF0000;
            MGC_Write32(pBase, MGC_O_HDRC_INTRRX, dwIntr);

            /* twice in case of double packet buffering */
            MGC_WriteCsr16(pBase, MGC_O_HDRC_TXCSR, bEnd, 
                MGC_M_TXCSR_FLUSHFIFO |MGC_M_TXCSR_CLRDATATOG);
            MGC_WriteCsr16(pBase, MGC_O_HDRC_TXCSR, bEnd, 
                MGC_M_TXCSR_FLUSHFIFO |MGC_M_TXCSR_CLRDATATOG);

            /* protocol/endpoint/interval/NAKlimit */
            MGC_WriteCsr16(pBase, MGC_O_HDRC_TXMAXP, bEnd, wPacketSize);

            //  Set interrupt polling interval must depend on high, or low/full speed.
            MGC_SetInterval(pThis, pUrb, bEnd);

            // Clean tx interrupt.
            wIntr = MGC_Read16(pBase, MGC_O_HDRC_INTRTX);
            MGC_Write16(pBase, MGC_O_HDRC_INTRTX, wIntr);

            // Cmd Queue: need to turn on DMAENAB.
            MGC_WriteCsr16(pBase, MGC_O_HDRC_TXCSR, bEnd, MGC_M_TXCSR_DMAENAB);
        }
        else
        {
            // Disable endpoint interrupt when using command queue. 
            /*
                IntrRxE, IntrUSB, and IntrUSBE are the same 32 bits group.
                Tricky: Set 0 in all write clear field in IntrUSB field. Prevent to clear IntrUSB.
                Because our 32 bits register access mode.            
            */
            dwIntr = MGC_Read32(pBase, MGC_O_HDRC_INTRRXE);
            dwIntr &= ~((ulong)(1 << bEnd));
            dwIntr &= 0xFF00FFFE;
            MGC_Write32(pBase, MGC_O_HDRC_INTRRXE, (ulong)dwIntr);

            /* twice in case of double packet buffering */
            MGC_WriteCsr16(pBase, MGC_O_HDRC_RXCSR, bEnd, 
                MGC_M_RXCSR_FLUSHFIFO |MGC_M_RXCSR_CLRDATATOG);
            MGC_WriteCsr16(pBase, MGC_O_HDRC_RXCSR, bEnd, 
                MGC_M_RXCSR_FLUSHFIFO |MGC_M_RXCSR_CLRDATATOG);

            /* protocol/endpoint/interval/NAKlimit */
            MGC_WriteCsr16(pBase, MGC_O_HDRC_RXMAXP, bEnd, wPacketSize);

            //  Set interrupt polling interval must depend on high, or low/full speed.
            MGC_SetInterval(pThis, pUrb, bEnd);

            // Clear rx endpoint interrupt.
            MGC_Write16(pBase, MGC_O_HDRC_INTRRX, (1 << bEnd));

            // Cmd Queue: need to turn on DMAENAB.
            MGC_WriteCsr16(pBase, MGC_O_HDRC_RXCSR, bEnd, MGC_M_RXCSR_DMAENAB);            
        }

        bIsZlp = ((usb_pipeisoc(nPipe)) ? FALSE: (pUrb->transfer_flags & URB_ZERO_PACKET));

        if (QMU_enable(pThis, bEnd, !bXmt, bIsZlp, TRUE, FALSE) < 0)
        {
            INFO("[USB] Failed to enable queue, ISOC\n");
            return;
        }

        pEnd->bCmdQEnable = TRUE;
    }
		#if defined(CONFIG_ARCH_MT5396) && defined(CONFIG_MT53XX_USE_CHANNELB_DRAM)  
	    if ((ulong)(pUrb->transfer_dma) >= mt53xx_cha_mem_size)
	    {
	        INFO("[USB]incorrect address 0x%08X.\n", (ulong)(pUrb->transfer_dma));
	    }
		#endif
    if (usb_pipeisoc(nPipe))
    {
        pBuffer_addr= pUrb->transfer_dma;		// In fact the address of transfer_dma is 64bit, but the DMA mask setting is 0xffffffff, so the valid address is low 32 bit of transfer_dma
            
        for(i=0; i<pUrb->number_of_packets; i++)
        {
            offset = pUrb->iso_frame_desc[i].offset;
            dwLength = pUrb->iso_frame_desc[i].length;
            /* If interrupt on complete ? */
            bIsIoc = (i == (pUrb->number_of_packets-1)) ? TRUE : FALSE;  
            
            if (QMU_insert_task(pThis, bEnd, !bXmt, 
                    pBuffer_addr+offset, dwLength, bIsIoc) < 0)
            {
                INFO("[USB] Insert Task Error !\n");
                return;
            }		        
        }
    }
    else
    {
        /* Must be the bulk transfer type */
        pBuffer_addr = pUrb->transfer_dma;
    
        /* 
        Note current GPD only support 16 bits transferred data length.
        Currently no software workaround this problem.
        */        
        MUSB_ASSERT(pUrb->transfer_buffer_length < 65536);
        dwLength = pUrb->transfer_buffer_length;
        bIsIoc = TRUE;
 
        if (QMU_insert_task(pThis, bEnd, !bXmt, pBuffer_addr, dwLength, bIsIoc) < 0)
        {
            INFO("[USB] Insert Task Error !\n");
            return;
        }		        
    }    
}

#endif /* #ifdef CONFIG_USB_QMU_SUPPORT */

static void MGC_ProgramEnd(MGC_LinuxCd *pThis, MGC_LinuxLocalEnd *pEnd, struct urb *pUrb)
{
    uint16_t wIntrTxE;
    uint32_t dwIntrTxE;
    uint8_t *pBase = (uint8_t*)pThis->pRegs;
    unsigned int nPipe = pUrb->pipe;
    uint16_t wPacketSize = pEnd->wPacketSize;
    uint8_t bInterval;
    uint8_t bEnd = pEnd->bEnd;
    uint8_t bXmt = pEnd->bIsTx;
    uint8_t *pBuffer;
    uint32_t dwLength;
    uint8_t bDmaOk = FALSE;    
    MGC_HsDmaChannel* pDmaChannel;    
    uint16_t wCount = 0;
    uint16_t wCsr = 0;
    int toggle=0;
    MGC_SelectEnd(pBase, bEnd);
    MGC_SetProtocol(pThis, pUrb, bEnd, bXmt);
    MGC_SetAddress(pThis, pUrb, bEnd, bXmt);

    if (usb_pipecontrol(nPipe))
    {
        pThis->bEnd0Stage = MGC_END0_START;
        dwLength = 8;       
        pBuffer =  (uint8_t*)((ulong)pUrb->setup_packet);
    }
    else if (usb_pipeisoc(nPipe))
    {
        dwLength = pUrb->iso_frame_desc[0].length;
		#ifdef SUPPORT_SHARE_USBQ_DMA
		pBuffer = (uint8_t*)((MGC_CheckIsUseDMAChannel(pUrb, dwLength)) ?
	            ((ulong)pUrb->transfer_dma + pUrb->iso_frame_desc[0].offset) : 
	            ((ulong)pUrb->transfer_buffer + pUrb->iso_frame_desc[0].offset));
		#else
        pBuffer = 
            (uint8_t*)((dwLength > MGC_HSDMA_MIN_DMA_LEN) ? 
            ((ulong)pUrb->transfer_dma + pUrb->iso_frame_desc[0].offset) : 
            ((ulong)pUrb->transfer_buffer + pUrb->iso_frame_desc[0].offset));
		#endif
    }
    else
    {
        dwLength = pUrb->transfer_buffer_length;

		#ifdef SUPPORT_SHARE_USBQ_DMA
        pBuffer = 
            (uint8_t*)((MGC_CheckIsUseDMAChannel(pUrb, dwLength)) ? 
            ((ulong)pUrb->transfer_dma) : 
            ((ulong)pUrb->transfer_buffer));

		#else
        pBuffer = 
            (uint8_t*)((dwLength > MGC_HSDMA_MIN_DMA_LEN) ? 
            ((ulong)pUrb->transfer_dma) : 
            ((ulong)pUrb->transfer_buffer));
		#endif
     }

    if (bXmt) /* transmit */
    {
        /*
            IntrTxE, and IntrRx are the same 32 bits group.
            Tricky: Set 0 in all write clear field in IntrRx field. Prevent to clear IntrRx.
            Because our 32 bits register access mode.
        */
        wIntrTxE = MGC_Read16(pBase, MGC_O_HDRC_INTRTXE);
        dwIntrTxE = (ulong)((wIntrTxE & (~(uint16_t)(1 << bEnd))) << 16);
        dwIntrTxE &= 0xFFFF0000;
        MGC_Write32(pBase, MGC_O_HDRC_INTRRX, dwIntrTxE);

        if (bEnd)
        {
            MGC_CheckTxCsrValidForWrite(pThis, bEnd, 0x05);            
            /* twice in case of double packet buffering */
            MGC_WriteCsr16(pBase, MGC_O_HDRC_TXCSR, bEnd, MGC_M_TXCSR_CLRDATATOG);
            MGC_WriteCsr16(pBase, MGC_O_HDRC_TXCSR, bEnd, MGC_M_TXCSR_CLRDATATOG);
			toggle = usb_gettoggle(pUrb->dev, pEnd->bRemoteEnd, 1);
			
            // get data toggle bit from logical ep.
            if (toggle)
            {
                // set data toggle bit to physical ep.
                MU_MB();
                MGC_Write32(pBase, M_REG_TXDATATOG, M_REG_SET_DATATOG(bEnd, 1));
                MU_MB();
                //unified usb patch, dexiao
                MGC_Write32(pBase, M_REG_TXDATATOG,(1 << bEnd));
            }
		  
			if(MGC_WIFIHotbootSupported(pUrb) && (pEnd->bRemoteEnd == 8) && usb_pipebulk(pUrb->pipe)&&(usb_pipein(pUrb->pipe)== 0))
			{
				if(mgc_toggleErr_debug==1)
					INFO("ProgramEnd [%d][WIFI EP%d][HOST EP%d]** Set Toggle = %d/%d, length=%d  ToggleValue = 0x%x.\n", mgc_togglecnt, pEnd->bRemoteEnd, pEnd->bEnd, mgc_togglecnt_prev, toggle, pUrb->transfer_buffer_length,(MGC_Read32(pBase, M_REG_TXDATATOG)));

				mgc_togglecnt_prev = mgc_togglecnt;
				mgc_toggle_prev = toggle;
				mgc_toggle_cur = toggle;

				mgc_EP8_FIFO_TX_DONE = 0;		// Set the flag to false firstly
			}

            /* protocol/endpoint/interval/NAKlimit */
            MGC_WriteCsr16(pBase, MGC_O_HDRC_TXMAXP, bEnd, wPacketSize);

            //  Set interrupt polling interval must depend on high, or low/full speed.
            MGC_SetInterval(pThis, pUrb, bEnd);
        }
        else
        {
            /* endpoint 0: just flush */
            MGC_WriteCsr16(pBase, MGC_O_HDRC_CSR0, bEnd, MGC_M_CSR0_FLUSHFIFO);

            MGC_WriteCsr16(pBase, MGC_O_HDRC_CSR0, bEnd, MGC_M_CSR0_FLUSHFIFO);

            bInterval = MGC_Log2(pUrb->interval) + 1;
            bInterval = (bInterval > 16) ? 16 : ((bInterval <= 1) ? 0 : bInterval);

            /* protocol/endpoint/interval/NAKlimit */
            MGC_WriteCsr8(pBase, MGC_O_HDRC_NAKLIMIT0, 0, bInterval);
        }

        // clear tx interrupt.
        MGC_Write16(pBase, MGC_O_HDRC_INTRTX, (1 << bEnd));
        
        /* re-enable interrupt and write CSR to transmit */
        dwIntrTxE = (wIntrTxE) << 16;
        dwIntrTxE &= 0xFFFF0000;        
        MGC_Write32(pBase, MGC_O_HDRC_INTRRX, dwIntrTxE);

        if (bEnd)
        {
            MGC_CheckTxCsrValidForWrite(pThis, bEnd, 0x06);
            MGC_WriteCsr16(pBase, MGC_O_HDRC_TXCSR, bEnd, wCsr);
        }

        if (!pEnd->pDmaChannel)
        {
            pEnd->pDmaChannel = 
                MGC_HsDmaAllocateChannel(pUrb, &pThis->rDma, bEnd, TRUE, 
                dwLength, (uint8_t)usb_pipetype(nPipe), wPacketSize);
        }

        if (pEnd->pDmaChannel)
        {
            pDmaChannel = pEnd->pDmaChannel;

            //  set DMAReqMode by sending data size.
            pDmaChannel->dwStartAddress = (ulong)pBuffer;
            pDmaChannel->dwCount = (ulong)dwLength;
            pDmaChannel->dwActualLength = 0;
            pDmaChannel->bDesiredMode = (dwLength > wPacketSize) ? 1 : 0;               
            pEnd->dwRequestSize = dwLength;
            bDmaOk = MGC_HsDmaProgramChannel(pDmaChannel);
			#if 1
			if(MGC_WIFIHotbootSupported(pUrb))
			{
				if(mgc_berror && (mgc_resumtcnt < mgc_printnum) && usb_pipebulk(pUrb->pipe)&&(usb_pipein(pUrb->pipe)== 0) && ((uint8_t)usb_pipeendpoint(pUrb->pipe)==8))
				{
					INFO("[USB %d]Tx pUrb=0x%p, pEnd->bRemoteEnd=%d, bDmaOk=%d, length=%d proto=%s\n", mgc_resumtcnt, pUrb, pEnd->bRemoteEnd, bDmaOk, pUrb->transfer_buffer_length,MGC_DecodeUrbProtocol(pUrb));
				}
			}
			#endif

            if (!bDmaOk)
            {
                MGC_HsDmaReleaseChannel(pDmaChannel);

                pEnd->pDmaChannel = NULL;
                pEnd->dwRequestSize = 0;
            }
        }

        if (!bDmaOk)
        {
            wCount = min((ulong)wPacketSize, dwLength);

            if (bEnd)
            {
            
				#if 1
				if(MGC_WIFIHotbootSupported(pUrb))
				{
					if(mgc_berror && (mgc_resumtcnt < mgc_printnum) && usb_pipebulk(pUrb->pipe)&&(usb_pipein(pUrb->pipe)== 0) && ((uint8_t)usb_pipeendpoint(pUrb->pipe)==8))
					{
						INFO("[USB %d]Tx pUrb=0x%p, pEnd->bRemoteEnd=%d, bDmaOk=%d, length=%d proto=%s\n", mgc_resumtcnt, pUrb, pEnd->bRemoteEnd, bDmaOk, pUrb->transfer_buffer_length,MGC_DecodeUrbProtocol(pUrb));
					}
				}
				#endif
                MGC_TxPacket(pThis, pEnd);
            }
            else
            {
                if (wCount)
                {
                    pEnd->dwRequestSize = wCount;

                    MGC_LoadFifo(pBase, bEnd, wCount, pBuffer);
                }
                MGC_StartTx(pThis, bEnd);                
            }            
        }
    }
    else
    {
        /* protocol/endpoint/interval/NAKlimit */
        if (bEnd)
        {
            /* grab Rx residual if any */
            wCsr = MGC_ReadCsr16(pBase, MGC_O_HDRC_RXCSR, bEnd);

            if (wCsr &MGC_M_RXCSR_RXPKTRDY)
            {
                INFO("[USB]RXPKTRDY when kicking urb !!");
                wCount = MGC_ReadCsr16(pBase, MGC_O_HDRC_RXCOUNT, bEnd);
                MGC_RxPacket(pThis, pEnd, wCount);
                return ;
            }

            MGC_WriteCsr16(pBase, MGC_O_HDRC_RXMAXP, bEnd, wPacketSize);

            //  Set interrupt polling interval must depend on high, or low/full speed.
            MGC_SetInterval(pThis, pUrb, bEnd);

            /* first time or re-program and shared FIFO, flush & clear toggle */
            /* twice in case of double packet buffering */
            MGC_WriteCsr16(pBase, MGC_O_HDRC_RXCSR, bEnd, MGC_M_RXCSR_CLRDATATOG);
            MGC_WriteCsr16(pBase, MGC_O_HDRC_RXCSR, bEnd, MGC_M_RXCSR_CLRDATATOG);
			#if 1
			if(MGC_WIFIHotbootSupported(pUrb))
			{
				if(mgc_berror && (mgc_resumtcnt < mgc_printnum) && usb_pipebulk(pUrb->pipe)&&(usb_pipein(pUrb->pipe)== 0) && ((uint8_t)usb_pipeendpoint(pUrb->pipe)==8))
				{
					INFO("[USB %d]Rx pUrb=0x%p, pEnd->bRemoteEnd=%d, length=%d proto=%s\n", mgc_resumtcnt, pUrb, pEnd->bRemoteEnd, pUrb->transfer_buffer_length,MGC_DecodeUrbProtocol(pUrb));
				}
			}
			#endif

            // HDRC will use this method.
            if (usb_gettoggle(pUrb->dev, pEnd->bRemoteEnd, 0))
            {
				MU_MB();
                MGC_Write32(pBase, M_REG_RXDATATOG, M_REG_SET_DATATOG(bEnd, 1));
            	MU_MB();	
                //unified usb patch, dexiao
                MGC_Write32(pBase, M_REG_RXDATATOG,(1 << bEnd));
            }

            if ((usb_pipebulk(nPipe)) && (dwLength >= wPacketSize))
            {
                pEnd->pDmaChannel = 
                    MGC_HsDmaAllocateChannel(pUrb, &pThis->rDma, bEnd, FALSE, 
                    dwLength, (uint8_t)usb_pipetype(nPipe), wPacketSize);

                if (pEnd->pDmaChannel)
                {
                    pDmaChannel = pEnd->pDmaChannel;
                    /*
                        Note: 
                        Use "multiple packet RX, if size of data block not know" to handle rx data by dma.
                    */
                    pDmaChannel->dwStartAddress = (ulong)pBuffer;
                    pDmaChannel->dwCount = (ulong)dwLength;
                    pDmaChannel->dwActualLength = 0;
                    pDmaChannel->bDesiredMode = 1;
                    pEnd->dwRequestSize = dwLength;
                    bDmaOk = MGC_HsDmaProgramChannel(pDmaChannel);
                    if (!bDmaOk)
                    {
                        MGC_HsDmaReleaseChannel(pDmaChannel);

                        pEnd->pDmaChannel = NULL;
                        pEnd->dwRequestSize = 0;
                    }
                    else
                    {
                        // DMA ok.
                        return ;
                    }
                }
            }

            /* kick things off */
            MU_MB();
            MGC_WriteCsr16(pBase, MGC_O_HDRC_RXCSR, bEnd, MGC_M_RXCSR_H_REQPKT);
         }
    }
}

void MGC_KickUrb(MGC_LinuxCd *pThis, struct urb *pUrb, MGC_LinuxLocalEnd *pEnd)
{
    uint32_t nPipe;
    uint8_t bXmt;
    uint16_t wPacketSize;

    if (!pUrb)
    {
        ERR("[USB]***> bEnd=%d, pUrb = NULL!\n", pEnd->bEnd);
        return ;
    } 

    nPipe = pUrb->pipe;
    bXmt = (usb_pipecontrol(nPipe)) ? TRUE: usb_pipeout(nPipe);
    wPacketSize = usb_maxpacket(pUrb->dev, nPipe, usb_pipeout(nPipe));

    DBG(3, "[USB]<== pUrb=%p, bEnd=%d, wPacketSize=%d, bXmt=%d\n", pUrb, pEnd->bEnd, wPacketSize, bXmt);
#if 1
	if(MGC_WIFIHotbootSupported(pUrb))
	{
		if(mgc_berror && (mgc_resumtcnt < mgc_printnum) && usb_pipebulk(pUrb->pipe)&&(usb_pipein(pUrb->pipe)== 0) && ((uint8_t)usb_pipeendpoint(pUrb->pipe)==8))
		{
			INFO("[USB %d]pUrb=0x%p, pUrb->hcpriv=%p, length=%d proto=%s\n", mgc_resumtcnt, pUrb, pUrb->hcpriv, pUrb->transfer_buffer_length,MGC_DecodeUrbProtocol(pUrb));
		}
	}
#endif

    /* indicate in progress */
    pUrb->actual_length = 0;
    pUrb->error_count = 0;

    DBG(3, "[USB](%p): dir=%s, type=%d, wPacketSize=%d.\n", pUrb, (bXmt) ? "out" : "in", usb_pipetype(nPipe), wPacketSize);

#ifdef CONFIG_USB_QMU_SUPPORT
    if (MGC_IsCmdQSupport(pThis, pEnd, pUrb))
    {
        MGC_KickCmdQ(pThis, pEnd, pUrb);
    }
    else
#endif  /* CONFIG_USB_QMU_SUPPORT */ 
    {
        /* remember software state - find_end() will use this - */
        pEnd->bRemoteAddress = (uint8_t)usb_pipedevice(nPipe);;
        pEnd->bRemoteEnd = (uint8_t)usb_pipeendpoint(nPipe);;
        pEnd->bTrafficType = (uint8_t)usb_pipetype(nPipe);

        /* init urb */
        pEnd->dwOffset = 0;
        pEnd->dwRequestSize = 0;
        pEnd->dwIsoPacket = 0;
        pEnd->bRetries = 0;
        pEnd->wPacketSize = wPacketSize;
        pEnd->bIsTx = bXmt;

        /* Configure endpoint */
        MGC_ProgramEnd(pThis, pEnd, pUrb);
    }
}

static int MGC_ScheduleUrb(MGC_LinuxCd *pThis, MGC_LinuxLocalEnd *pEnd, struct urb *pUrb)
{
    int idle;
    int rc;
 
    DBG(3, "[USB]queued URB %p (current %p) to end %d (bRemoteAddress=%d, bRemoteEnd=%d proto=%d) pEnd->bBusyCompleting=%d\n",
		    pUrb, MGC_GetCurrentUrb(pEnd), pEnd->bEnd, (uint8_t)usb_pipedevice(pUrb->pipe), (uint8_t)usb_pipeendpoint(pUrb->pipe), usb_pipetype(pUrb->pipe), pEnd->bBusyCompleting);
    idle = MGC_IsEndIdle(pEnd);
    rc = MGC_EnqueueEndUrb(pThis, pEnd, pUrb);

    if (rc)
    {
        return rc;
    }

#ifdef CONFIG_USB_QMU_SUPPORT
    if (MGC_IsCmdQSupport(pThis, pEnd, pUrb) 
        && (!pEnd->bStateGated))
    {
        MGC_KickUrb(pThis, pUrb, pEnd);
    }
    else
#endif        
    {       
        //  check pEnd->bBusyCompleting to prevent double kickstart the same urb.           
        if (idle && (pEnd->bBusyCompleting == 0))
        {
            MGC_KickUrb(pThis, MGC_GetCurrentUrb(pEnd), pEnd);
        }
    }

    return 0;
}

static int MGC_ServiceUSBIntr(MGC_LinuxCd *pThis, uint8_t bIntrUSB)
{
    int handled = 0;
    uint8_t bSpeed = 1;
    uint8_t devctl;
    uint8_t *pBase = (uint8_t*)pThis->pRegs;
    uint32_t dwVirtualHubPortStatus = 0;
    uint8_t power;
	uint32_t dummy1;

    devctl = MGC_Read8(pBase, MGC_O_HDRC_DEVCTL);
    power = MGC_Read8(pBase, MGC_O_HDRC_POWER);

    if (bIntrUSB &MGC_M_INTR_RESUME)
    {
        handled++;

        DBG(2, "[USB]RESUME\n");
    }

    if (bIntrUSB &MGC_M_INTR_SESSREQ)
    {
        handled++;

        DBG(2, "[USB]SESSION_REQUEST\n");
    }

    if (bIntrUSB &MGC_M_INTR_VBUSERROR)
    {
        handled++;  

        DBG(2, "[USB]V_BUS ERROR??? this is bad (TM)\n");
    }

    if (bIntrUSB &MGC_M_INTR_CONNECT)
    {
        handled++;
 
        DBG(2, "[USB]CONNECT\n");

        pThis->bEnd0Stage = MGC_END0_START;

        if (devctl &MGC_M_DEVCTL_LSDEV)
        {
            bSpeed = MGC_SPEED_LS;

            dwVirtualHubPortStatus = (1 << USB_PORT_FEAT_C_CONNECTION) | (1 << USB_PORT_FEAT_LOWSPEED) | (1 << USB_PORT_FEAT_POWER) | (1 << USB_PORT_FEAT_CONNECTION);
        }
        else if (devctl &MGC_M_DEVCTL_FSDEV)
        {
            /* NOTE: full-speed is "speculative" until reset */
            //bSpeed = 2;
            bSpeed = (power &MGC_M_POWER_HSMODE) ? MGC_SPEED_HS : MGC_SPEED_FS;

            dwVirtualHubPortStatus = (1 << USB_PORT_FEAT_C_CONNECTION) | (1 << USB_PORT_FEAT_POWER) | (1 << USB_PORT_FEAT_CONNECTION);
        }

        pThis->bRootSpeed = bSpeed;
        pThis->dwVirtualHubPortStatus = dwVirtualHubPortStatus;
        if (musbstruct_to_hcd(pThis)->status_urb)
            usb_hcd_poll_rh_status(musbstruct_to_hcd(pThis));
		else
		{
			usb_hcd_resume_root_hub(musbstruct_to_hcd(pThis));
		}
    }

    /* saved one bit: bus reset and babble share the same bit;
     * If I am host is a babble! i must be the only one allowed
     * to reset the bus; when in otg mode it means that I have 
     * to switch to device
     */
    if (bIntrUSB &MGC_M_INTR_RESET)
    {
        handled++;

        DBG(1, "[USB]BUS babble\n");

        //  Solve the busy usb babble interrupt caused by babble device
        //  if device is attached
        if (devctl &(MGC_M_DEVCTL_FSDEV | MGC_M_DEVCTL_LSDEV))
        {
            dummy1 = MGC_Read8(pBase,M_REG_PERFORMANCE3);

            if (dummy1 & MGC_M_DUMMY1_SOFFORCE)
            {
                DBG(1,"[USB]SOF is still transmit during babble.\n");
                // Ignore this babble.
                bIntrUSB &= ~MGC_M_INTR_RESET;
            }
        }

        handled++;
    }

    if (bIntrUSB &MGC_M_INTR_SOF)
    {
        DBG(2, "[USB]START_OF_FRAME\n");

        handled++;
    }

    /* p35 MUSBHDRC manual for the order of the tests */
    if (bIntrUSB &MGC_M_INTR_DISCONNECT)
    {
        DBG(2, "[USB]DISCONNECT\n");

        handled++;

        pThis->dwVirtualHubPortStatus = (1 << USB_PORT_FEAT_C_CONNECTION) | (1 << USB_PORT_FEAT_POWER);

        // Set UTMI+PHY to low power mode and stop XCLK.
        power = MGC_Read8(pBase, MGC_O_HDRC_POWER);
        power |= (MGC_M_POWER_SUSPENDM | MGC_M_POWER_ENSUSPEND);
        MGC_Write8(pBase, MGC_O_HDRC_POWER, power);
    }

    /* I cannot get suspend while in host mode! go to error mode and ignore 
     * the other signals; need to be last (see manual p35)s  */
    if (bIntrUSB &MGC_M_INTR_SUSPEND)
    {
        DBG(2, "[USB]RECEIVED SUSPEND\n");

        handled++;
    }

    return handled;
}

static void MGC_CheckConnect(unsigned long pParam)
{
    MGC_LinuxCd *pThis = (MGC_LinuxCd*)pParam;
    void *pPhyBase = pThis->pPhyBase;
    uint32_t dwLineState;
#ifdef MUSB_BC12
#if defined(CONFIG_ARCH_MT5881) || defined(CONFIG_ARCH_MT5890) || \
    defined(CONFIG_ARCH_MT5891) || defined(CONFIG_ARCH_MT5882) || \
    defined(CONFIG_ARCH_MT5883)
    uint32_t dwReg = 0;
#endif
#endif
    dwLineState = MGC_PHY_Read32(pPhyBase, M_REG_U2PHYDMON1);
    dwLineState &= M_REG_LINESTATE_MASK;
    if (dwLineState == LINESTATE_DISCONNECT)
    {
        INFO("[USB]Line State  : 0x%x\n", dwLineState);
		goto check_device;
    }

    if (dwLineState == LINESTATE_HWERROR)
    {
        //  Move MGC_dwCheckInsert inside MGC_LinuxCd port structure
        pThis->bCheckInsert++;

        if (pThis->bCheckInsert > 5)
        {
            INFO( "[USB]LINESTATE_HWERROR !!!\n");

            pThis->bCheckInsert = 0;
            return ;
        }

        mod_timer(&pThis->events_timer, 
			      jiffies + msecs_to_jiffies(MGC_CHECK_INSERT_DEBOUNCE));
		pThis->events |= USB_EVENTS_CHECK_CONNECT;
        INFO("[USB]Line State Err : 0x%x\n", dwLineState);
        return ;
    }
#ifdef MUSB_BC12
#if defined(CONFIG_ARCH_MT5881) || defined(CONFIG_ARCH_MT5890) || \
    defined(CONFIG_ARCH_MT5891) || defined(CONFIG_ARCH_MT5882) || \
    defined(CONFIG_ARCH_MT5883)
      if (pThis->bPortNum == 0)
      {
	  INFO("Disable BC12 function.\n");
#if !(defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5891))
	  dwReg = MGC_PHY_Read32(pPhyBase,M_REG_PHYACR5);
#else
	  dwReg = MGC_PHY_Read32(pPhyBase,0x40); //5890 IC issue, read 0x40 instead of M_REG_U2PHYACR5(0x24)
#endif
	  dwReg &= ~0x1;
	  MGC_PHY_Write32(pPhyBase,M_REG_PHYACR5,dwReg);
      }
#endif
#endif //MUSB_BC12

check_device:

    pThis->bInsert = TRUE;

    DBG(3, "[USB]Connect interrupt !!!\n");
#ifdef MGC_UNLINK_URB
    MGC_UnlinkInvalidUrb((unsigned long) pThis);
	INFO("[USB] unlink invalid urb in check connect \n");
#endif
    // handle device connect.
    MGC_ServiceUSBIntr(pThis, MGC_M_INTR_CONNECT);
}

static MGC_DmaControlStatus MGC_ServiceHostDefault(MGC_LinuxCd *pThis, uint16_t wCount, struct urb *pUrb)
{
    MGC_DmaControlStatus bMore = MGC_DMA_FIFO_DATAEND;
    uint8_t *pFifoDest = NULL;
    uint16_t wFifoCount = 0;
    uint8_t *pBase = (uint8_t*)pThis->pRegs;
    MGC_LinuxLocalEnd *pEnd = &(pThis->aLocalEnd[EP0][0]);
    MUSB_DeviceRequest *pRequest = (MUSB_DeviceRequest*)pUrb->setup_packet;
    uint8_t bDmaOk = FALSE;    
    MGC_HsDmaChannel* pDmaChannel;    

    DBG(3, "[USB]<== (wCount=%04x, pUrb=%lx, bStage=%02x)\n", wCount, (unsigned long)pUrb, pThis->bEnd0Stage);

    switch (pThis->bEnd0Stage)
    {
        case MGC_END0_START:
            if (pRequest->bmRequestType & USB_DIR_IN)
            {
                DBG(3, "[USB]just did setup, switching to IN\n");

                /* this means we just did setup; switch to IN */
                pThis->bEnd0Stage = MGC_END0_IN;
                bMore = MGC_DMA_FIFO_DATAMORE;  
#ifdef CONFIG_USB_MT53xx_COMPLIANCE_TEST
		if (mtk_usb_debugfs_cb(pUrb->dev->bus) == 
		    MUSB_HSET_PORT_TEST_GET_DESC2) {
		    printk("Need delay 15 secs before IN Data Status...\n");
		    mdelay(15000);
	      }
#endif				
            }
            else if (pRequest->wLength > 0)
            {
                if (pEnd->pDmaChannel)
                {
                    pDmaChannel = pEnd->pDmaChannel;
                    pFifoDest = (uint8_t *)((ulong)pUrb->transfer_dma + pUrb->actual_length);
                    wFifoCount = min(pEnd->wPacketSize, ((uint16_t)(pUrb->transfer_buffer_length - pUrb->actual_length)));
                    pDmaChannel->bTransmit = TRUE;  /* Change to Out data */
                    pDmaChannel->dwStartAddress = (ulong)pFifoDest;
                    pDmaChannel->dwCount = (ulong)wFifoCount;
                    pDmaChannel->dwActualLength = 0;
                    pDmaChannel->bDesiredMode = 0;
                    pEnd->dwRequestSize = wFifoCount;
                    bDmaOk = MGC_HsDmaProgramChannel(pDmaChannel);
                    if (!bDmaOk)
                    {
                        MGC_HsDmaReleaseChannel(pDmaChannel);
                        pEnd->pDmaChannel = NULL;
                        pEnd->dwRequestSize = 0;
                    }
                    else
                    {            
                        // DMA ok.                
                        pThis->bEnd0Stage = MGC_END0_DMA_OUT;                
                        return MGC_DMA_OUT;
                    }
                }

                pThis->bEnd0Stage = MGC_END0_OUT;
            
                pFifoDest = (uint8_t*)(pUrb->transfer_buffer + pUrb->actual_length);
                wFifoCount = min(pEnd->wPacketSize, ((uint16_t)(pUrb->transfer_buffer_length - pUrb->actual_length)));
                DBG(3, "[USB]Sending %d bytes to %p\n", wFifoCount, pFifoDest);
                MGC_LoadFifo(pBase, 0, wFifoCount, pFifoDest);

                pEnd->dwRequestSize = wFifoCount;
                pUrb->actual_length += wFifoCount;
                if (wFifoCount)
                {
                    bMore = MGC_DMA_FIFO_DATAMORE;
                }
            }
            break;
        case MGC_END0_IN:
            if (pEnd->pDmaChannel)
            {
                pDmaChannel = pEnd->pDmaChannel;           
                pFifoDest = (uint8_t *)((ulong)pUrb->transfer_dma + pUrb->actual_length);
                wFifoCount = min(wCount, ((uint16_t)(pUrb->transfer_buffer_length - pUrb->actual_length)));
                pDmaChannel->bTransmit = FALSE;  /* Change to In data */
                pDmaChannel->dwStartAddress = (ulong)pFifoDest;
                pDmaChannel->dwCount = (ulong)wFifoCount;
                pDmaChannel->dwActualLength = 0;
                pDmaChannel->bDesiredMode = 0;
                pEnd->dwRequestSize = wFifoCount;
                bDmaOk = MGC_HsDmaProgramChannel(pDmaChannel);
                if (!bDmaOk)
                {
                    MGC_HsDmaReleaseChannel(pDmaChannel);
                    pEnd->pDmaChannel = NULL;
                    pEnd->dwRequestSize = 0;
                }
                else
                {            
                    // DMA ok.                
                    pThis->bEnd0Stage = MGC_END0_DMA_IN;
                    return MGC_DMA_IN;
                }
            }
            
            /* we are receiving from peripheral */
            pFifoDest = pUrb->transfer_buffer + pUrb->actual_length;
            wFifoCount = min(wCount, ((uint16_t)(pUrb->transfer_buffer_length - pUrb->actual_length)));

            DBG(3, "[USB]Receiving %d bytes in &%p[%d] (pUrb->actual_length=%u)\n", wFifoCount, pUrb->transfer_buffer, (unsigned int)pUrb->actual_length, pUrb->actual_length);

            MGC_UnloadFifo(pBase, 0, wFifoCount, pFifoDest);
            MGC_CleanDcache(pUrb,(ulong)pFifoDest, (ulong)wFifoCount);
            pUrb->actual_length += wFifoCount;
            if ((pUrb->actual_length < pUrb->transfer_buffer_length) && (wCount == pEnd->wPacketSize))
            {
                bMore = MGC_DMA_FIFO_DATAMORE;
            }             
            break;
        case MGC_END0_DMA_IN:
            MUSB_ASSERT(pEnd->pDmaChannel);
            pDmaChannel = pEnd->pDmaChannel;
            wCount = pDmaChannel->dwActualLength;
            pEnd->dwOffset += wCount;
            pUrb->actual_length += wCount;
            if ((pUrb->actual_length < pUrb->transfer_buffer_length) && 
                 (wCount == pEnd->wPacketSize))
            {
                pThis->bEnd0Stage = MGC_END0_IN;
                bMore = MGC_DMA_FIFO_DATAMORE;
            }
            break;
        case MGC_END0_OUT:
            pFifoDest = (uint8_t*)(pUrb->transfer_buffer + pUrb->actual_length);
            wFifoCount = min(pEnd->wPacketSize, ((uint16_t)(pUrb->transfer_buffer_length - pUrb->actual_length)));
            DBG(3, "[USB]Sending %d bytes to %p\n", wFifoCount, pFifoDest);
            MGC_LoadFifo(pBase, 0, wFifoCount, pFifoDest);

            pEnd->dwRequestSize = wFifoCount;
            pUrb->actual_length += wFifoCount;
            if (wFifoCount)
            {
                bMore = MGC_DMA_FIFO_DATAMORE;
            }            
            break;
        case MGC_END0_DMA_OUT:
            MUSB_ASSERT(pEnd->pDmaChannel);
            pDmaChannel = pEnd->pDmaChannel;
            pEnd->dwOffset += pDmaChannel->dwActualLength;
            pUrb->actual_length += pDmaChannel->dwActualLength;
            if (pUrb->actual_length < pUrb->transfer_buffer_length)
            {
                pFifoDest = (uint8_t *)((ulong)pUrb->transfer_dma + pUrb->actual_length);
                wFifoCount = min(pEnd->wPacketSize, ((uint16_t)(pUrb->transfer_buffer_length - pUrb->actual_length)));
                pDmaChannel->bTransmit = TRUE;  /* Change to Out data */
                pDmaChannel->dwStartAddress = (ulong)pFifoDest;
                pDmaChannel->dwCount = (ulong)wFifoCount;
                pDmaChannel->dwActualLength = 0;
                pDmaChannel->bDesiredMode = 0;
                pEnd->dwRequestSize = wFifoCount;
                bDmaOk = MGC_HsDmaProgramChannel(pDmaChannel);
                if (!bDmaOk)
                {
                    MGC_HsDmaReleaseChannel(pDmaChannel);
                    pEnd->pDmaChannel = NULL;
                    pEnd->dwRequestSize = 0;
                }
                else
                {            
                    return MGC_DMA_OUT;
                }
            }            
            break;
        default:
            break;
    }

    return bMore;
}

static void MGC_ServiceDefaultEnd(MGC_LinuxCd *pThis)
{
    struct urb *pUrb;
    uint16_t wCsrVal, wCount;
    int status = USB_ST_NOERROR;
    uint8_t *pBase = (uint8_t*)pThis->pRegs;
    MGC_LinuxLocalEnd *pEnd = &(pThis->aLocalEnd[EP0][0]);
    uint8_t bVal, bOutVal = 0, bComplete = FALSE, bError = FALSE;
    MGC_DmaControlStatus bMore;
    
    pUrb = MGC_GetCurrentUrb(pEnd);
    MGC_SelectEnd(pBase, 0);

    wCsrVal = MGC_ReadCsr16(pBase, MGC_O_HDRC_CSR0, 0);
    wCount = MGC_ReadCsr8(pBase, MGC_O_HDRC_COUNT0, 0);
    bVal = (uint8_t)wCsrVal;

    DBG(3, "[USB]<== CSR0=%04x, wCount=%04x\n", wCsrVal, wCount);

    /* if we just did status stage, we are done */
    if (MGC_END0_STATUS == pThis->bEnd0Stage)
    {
        bComplete = TRUE;
    }

    /* prepare status */
    if ((MGC_END0_START == pThis->bEnd0Stage) && !wCount && (wCsrVal &MGC_M_CSR0_RXPKTRDY))
    {
        INFO("[USB]EP0 missed data, Flush FIFO !\n");
        /* just started and got Rx with no data, so probably missed data */
        status = USB_ST_SHORT_PACKET;
        bError = TRUE;

        //bComplete = TRUE;
        MGC_WriteCsr16(pBase, MGC_O_HDRC_CSR0, 0, MGC_M_CSR0_FLUSHFIFO);
        MGC_WriteCsr16(pBase, MGC_O_HDRC_CSR0, 0, MGC_M_CSR0_FLUSHFIFO);
    }

    if (bVal &MGC_M_CSR0_H_RXSTALL)
    {
       INFO("[USB]STALLING ENDPOINT 0\n");
        status = USB_ST_STALL;
        bError = TRUE;
    }
    else if (bVal &MGC_M_CSR0_H_ERROR)
    {
        INFO("[USB]ep0 no response (error)\n");

        status = USB_ST_NORESPONSE;
        bError = TRUE;
    }
    else if (bVal &MGC_M_CSR0_H_NAKTIMEOUT)
    {
        INFO("[USB]ep0 NAK timeout pEnd->bRetries=%d\n", pEnd->bRetries);

        if (++pEnd->bRetries < MUSB_MAX_RETRIES)
        {
            /* cover it up if retries not exhausted */
            MGC_WriteCsr16(pBase, MGC_O_HDRC_CSR0, 0, 0);
        }
        else
        {
            INFO("[USB]no response (NAK timeout)\n");
            pEnd->bRetries = 0;
            status = USB_ST_NORESPONSE;
            bError = TRUE;
        }
    }
    if (USB_ST_NORESPONSE == status)
    {
        INFO("[USB]ep0 aborting\n");

        /* use the proper sequence to abort the transfer */
        if (bVal &MGC_M_CSR0_H_REQPKT)
        {
            bVal &= ~MGC_M_CSR0_H_REQPKT;
            MGC_WriteCsr16(pBase, MGC_O_HDRC_CSR0, 0, bVal);
            bVal &= ~MGC_M_CSR0_H_NAKTIMEOUT;
            MGC_WriteCsr16(pBase, MGC_O_HDRC_CSR0, 0, bVal);
        }
        else
        {
            INFO("[USB]end 0 no response, Flush FIFO !\n");
            bVal |= MGC_M_CSR0_FLUSHFIFO;
            MGC_WriteCsr16(pBase, MGC_O_HDRC_CSR0, 0, bVal);
            MGC_WriteCsr16(pBase, MGC_O_HDRC_CSR0, 0, bVal);
            bVal &= ~MGC_M_CSR0_H_NAKTIMEOUT;
            MGC_WriteCsr16(pBase, MGC_O_HDRC_CSR0, 0, bVal);
        }

        MGC_WriteCsr8(pBase, MGC_O_HDRC_NAKLIMIT0, 0, 0);
    }

    if (bError)
    {
        INFO("[USB]ep0 handling error\n");

        /* clear it */
        MGC_WriteCsr16(pBase, MGC_O_HDRC_CSR0, 0, 0);
    }

    if (!pUrb)
    {
        /* stop endpoint since we have no place for its data, this 
         * SHOULD NEVER HAPPEN! */
        INFO("[USB]no URB for end 0, Flush FIFO !\n");
        MGC_WriteCsr16(pBase, MGC_O_HDRC_CSR0, 0, MGC_M_CSR0_FLUSHFIFO);
        MGC_WriteCsr16(pBase, MGC_O_HDRC_CSR0, 0, MGC_M_CSR0_FLUSHFIFO);
        MGC_WriteCsr16(pBase, MGC_O_HDRC_CSR0, 0, 0);

        /* start next URB that might be queued for it */
        return ;
    }

    if (!bComplete && !bError)
    {
        /* call common logic and prepare response */
        bMore = MGC_ServiceHostDefault(pThis, wCount, pUrb);
        if (bMore == MGC_DMA_FIFO_DATAMORE)
        {
            /* more packets required */
            bOutVal = (MGC_END0_IN == pThis->bEnd0Stage) ? MGC_M_CSR0_H_REQPKT: MGC_M_CSR0_TXPKTRDY;
            DBG(3, "[USB]Need more bytes bOutVal=%04x\n", bOutVal);
        }
        else if (bMore == MGC_DMA_FIFO_DATAEND)
        {
            /* data transfer complete; perform status phase */
            bOutVal = MGC_M_CSR0_H_STATUSPKT | (usb_pipeout(pUrb->pipe) ? MGC_M_CSR0_H_REQPKT : MGC_M_CSR0_TXPKTRDY);

            /* flag status stage */
            pThis->bEnd0Stage = MGC_END0_STATUS;
            DBG(3, "[USB]Data transfer complete, status phase bOutVal=%04x\n", bOutVal);
        }

        /* write CSR0 if needed */
        if (bOutVal)
        {
            MU_MB();
            MGC_WriteCsr16(pBase, MGC_O_HDRC_CSR0, 0, bOutVal);
        }
    }

    /* call completion handler if done */
    if (bComplete || bError)
    {
        DBG(3, "[USB]completing cntrl URB %p, status=%d, len=%x\n", pUrb, status, pUrb->actual_length);

        // release dma channel.
        if (pEnd->pDmaChannel)
        {
            /* release previously-allocated channel */
            MGC_HsDmaReleaseChannel(pEnd->pDmaChannel);
            pEnd->pDmaChannel = NULL;
        }

        MGC_DequeueEndurb(pThis, pEnd, pUrb);
        pUrb->status = status;

        pEnd->bBusyCompleting = 1;
        spin_unlock_irqrestore(&pThis->SelectEndLock,irq_flags); 
        MGC_CompleteUrb(pThis, pEnd, pUrb);
        spin_lock_irqsave(&pThis->SelectEndLock,irq_flags);
        pEnd->bBusyCompleting = 0;

        if (!MGC_IsEndIdle(pEnd))
        {
            MGC_KickUrb(pThis, MGC_GetNextUrb(pEnd), pEnd);
        }
    }
}

static void MGC_ServiceTxAvail(MGC_LinuxCd *pThis, uint8_t bEnd)
{
    int skip = 0;
    struct urb *pUrb;
    uint16_t wTxCsrVal;
    uint16_t wVal = 0;
    MGC_LinuxLocalEnd *pEnd; 
    uint8_t *pBase = (uint8_t*)pThis->pRegs;
    int toggle;
    uint32_t status = 0;

    if ((bEnd >= pThis->bEndTxCount) || (bEnd >= MGC_GET_TX_EPS_NUM(pThis)))
    {
        INFO("Tx[%d]Error: bEnd >= pThis->bEndCount !.\n", bEnd);
	return;
    }
    
    /*
        NOTICE:
        Because we use shared double packet buffer for mass storage.
        It can not let tx/rx traffic happend at the same time.
        So we need to queue all urb (tx/rx) on the same list.
        It will prevent tx/rx data corruption when tx/rx transfer at the same time.
        Always use EPMSD direction of pEnd for mass storage.
    */
    pEnd = &(pThis->aLocalEnd[EPTX][bEnd]);
    MGC_SelectEnd(pBase, bEnd);

    wTxCsrVal = MGC_ReadCsr16(pBase, MGC_O_HDRC_TXCSR, bEnd);
    wVal = wTxCsrVal;

    if (pEnd->bDoublePacketBuffer)
    {
        if (wTxCsrVal & (MGC_M_TXCSR_FIFONOTEMPTY|MGC_M_TXCSR_TXPKTRDY))
        {
            /* whopps, dbould buffering better be enabled. */
            /* skip this interrrupt and wait FIFONOTEMPTY to be clear by h/w */
            //MUSB_ASSERT(pEnd->bDoublePacketBuffer);    
            return;
        }
        else
        {
            // Clear double packet buffer overhead interrupt.    
            if (MGC_Read16(pBase, MGC_O_HDRC_INTRTX) & (1 << bEnd))
            {            
                MGC_Write16(pBase, MGC_O_HDRC_INTRTX, (1 << bEnd));            
            }
        }
    }
        
    pUrb = MGC_GetCurrentUrb(pEnd);
    if (!pUrb)
    {
        INFO("[USB]Urb has been freed before packet comeback. Tx bEnd =%d.\n", bEnd);    
        MGC_SelectEnd(pBase, bEnd);
        MGC_CheckTxCsrValidForWrite(pThis, bEnd, 0x07);
        MGC_WriteCsr16(pBase, MGC_O_HDRC_TXCSR, bEnd, 0);
        return ;
    } 
#if 1
	if(MGC_WIFIHotbootSupported(pUrb))
	{
		if(mgc_berror && (mgc_resumtcnt < mgc_printnum) && usb_pipebulk(pUrb->pipe)&&(usb_pipein(pUrb->pipe)== 0) && ((uint8_t)usb_pipeendpoint(pUrb->pipe)==8))
		{
			INFO("[USB %d]pUrb=0x%p, pUrb->hcpriv=%p, length=%d proto=%s\n", mgc_resumtcnt, pUrb, pUrb->hcpriv, pUrb->transfer_buffer_length, MGC_DecodeUrbProtocol(pUrb));
		}
	}
#endif

    /* check for errors */
    if (wTxCsrVal &MGC_M_TXCSR_H_RXSTALL)
    {
        INFO("[USB]TX end %d STALL, Flush FIFO (%d:%d) !\n", bEnd, pEnd->bRemoteAddress, pEnd->bEnd);
        status = USB_ST_STALL;
        MGC_WriteCsr16(pBase, MGC_O_HDRC_TXCSR, bEnd, MGC_M_TXCSR_FLUSHFIFO | MGC_M_TXCSR_TXPKTRDY);
    }
    else if (wTxCsrVal &MGC_M_TXCSR_H_ERROR)
    {
        INFO("[USB]TX data error on ep=%d, Flush FIFO (%d:%d) !\n", bEnd, pEnd->bRemoteAddress, pEnd->bEnd);

        status = USB_ST_NORESPONSE;

        /* do the proper sequence to abort the transfer */
        MGC_WriteCsr16(pBase, MGC_O_HDRC_TXCSR, bEnd, MGC_M_TXCSR_FLUSHFIFO | MGC_M_TXCSR_TXPKTRDY);
    }
    else if (wTxCsrVal &MGC_M_TXCSR_H_NAKTIMEOUT)
    {
        /* cover it up if retries not exhausted */
        if (pUrb->status == ( - EINPROGRESS) && ++pEnd->bRetries < MUSB_MAX_RETRIES)
        {

            wVal |= MGC_M_TXCSR_TXPKTRDY;
            wVal &= ~MGC_M_TXCSR_H_NAKTIMEOUT;
            MGC_WriteCsr16(pBase, MGC_O_HDRC_TXCSR, bEnd, wVal);

            INFO("[USB]TX NAK Timeout (%d:%d), Retry %d !\n", pEnd->bRemoteAddress, pEnd->bEnd, pEnd->bRetries);
            return ;
        }

        if (pUrb->status == ( - EINPROGRESS))
        {
            INFO("[USB]TX NAK Timeout !\n");
            status = USB_ST_NORESPONSE;
        }

        INFO("[USB]device Tx not responding on ep=%d, Flush FIFO (%d:%d) !\n", bEnd, pEnd->bRemoteAddress, pEnd->bEnd);

        /* do the proper sequence to abort the transfer */
        MGC_WriteCsr16(pBase, MGC_O_HDRC_TXCSR, bEnd, MGC_M_TXCSR_FLUSHFIFO | MGC_M_TXCSR_TXPKTRDY);
        pEnd->bRetries = 0;
    }   
    else if (wTxCsrVal &MGC_M_TXCSR_FIFONOTEMPTY)
    {
        INFO("[USB]FIFO not empty when Tx done on ep=%d (%d:%d) !\n", bEnd, pEnd->bRemoteAddress, pEnd->bEnd);
        /* whopps, dbould buffering better be enabled */
        skip = TRUE;
    }

    if (status)
    {
        pUrb->status = status;

        /* reset error bits */
        wVal &= ~(MGC_M_TXCSR_H_ERROR | MGC_M_TXCSR_H_RXSTALL | MGC_M_TXCSR_H_NAKTIMEOUT);
        wVal |= MGC_M_TXCSR_FRCDATATOG;
        MGC_WriteCsr16(pBase, MGC_O_HDRC_TXCSR, bEnd, wVal);
    }

    if (!skip && pUrb->status == ( - EINPROGRESS))
    {   
        MGC_TxPacket(pThis, pEnd);
    }

    /* complete the current request or start next tx transaction */
    if (pUrb->status != ( - EINPROGRESS))
    {
        //  set data toggle in device basis for supporting Hub.
        // get data toggle bit from physical ep.
        toggle = ((MGC_Read32(pBase, M_REG_TXDATATOG)) >> bEnd) &0x01;
            
        // release dma channel.
        if (pEnd->pDmaChannel)
        {
            /* release previously-allocated channel */
            MGC_HsDmaReleaseChannel(pEnd->pDmaChannel);
            pEnd->pDmaChannel = NULL;
        }
        pUrb->actual_length = pEnd->dwOffset;

        if (pUrb->status == USB_ST_STALL)
        {
            toggle = 0;
        } 
		
		if(MGC_WIFIHotbootSupported(pUrb) && (pEnd->bRemoteEnd == 8) && usb_pipebulk(pUrb->pipe)&&(usb_pipein(pUrb->pipe)== 0))
		{
			if(mgc_toggleErr_debug==1)
				INFO("TxServiceAvail [%s][%d][%d][WIFI EP%d]*-* Set Toggle = %d/%d, length=%d ToggleValue = 0x%x.\n\n",__FUNCTION__,__LINE__,mgc_togglecnt, pEnd->bRemoteEnd, mgc_togglecnt_prev, toggle, pUrb->transfer_buffer_length,(MGC_Read32(pBase, M_REG_TXDATATOG)));

			mgc_EP8_FIFO_TX_DONE = 0;		// means that the host has got the ACK/NYET, and toggled the bit
			mgc_togglecnt ++;
			//INFO("[WIFI EP%d]Set Toggle = %d, actual_length=%d.\n", pEnd->bRemoteEnd, toggle, pUrb->actual_length);
		}
            /* save data toggle */
        usb_settoggle(pUrb->dev, pEnd->bRemoteEnd, TRUE, toggle);

        MGC_DequeueEndurb(pThis, pEnd, pUrb);

        pEnd->bBusyCompleting = 1;
        spin_unlock_irqrestore(&pThis->SelectEndLock,irq_flags); 
        MGC_CompleteUrb(pThis, pEnd, pUrb);
        spin_lock_irqsave(&pThis->SelectEndLock,irq_flags);
        pEnd->bBusyCompleting = 0;

        if (!MGC_IsEndIdle(pEnd))
        {
            MGC_KickUrb(pThis, MGC_GetNextUrb(pEnd), pEnd);
        }
    }
}

static void MGC_ServiceRxReady(MGC_LinuxCd *pThis, uint8_t bEnd)
{
    struct urb *pUrb;
    uint16_t wRxCount, wRxCsrVal, wVal = 0;
    uint8_t bIsochError = FALSE;
    MGC_LinuxLocalEnd *pEnd;
    uint8_t *pBase = (uint8_t*)pThis->pRegs;
    uint8_t bDone = FALSE;
    uint32_t dwLength;
    MGC_DmaChannelStatus bStatus;
    MGC_HsDmaChannel* pDmaChannel;
    uint8_t bDmaOk = FALSE;
	uint8_t bMaxRetries = MUSB_MAX_RETRIES;
    uint8_t *pDmaBuffer;
    uint32_t dwIntr;    
    int toggle;
    uint32_t status = 0;
    struct usb_iso_packet_descriptor *packet;

    DBG(3, "[USB]<== end%d\n", bEnd);
    
    if((bEnd >= pThis->bEndRxCount) || (bEnd >= MGC_GET_RX_EPS_NUM(pThis)))
    {
        INFO("Rx[%d]Error: bEnd >= pThis->bEndCount !.\n", bEnd);
		return;
    }
    
    pEnd = &(pThis->aLocalEnd[EPRX][bEnd]); /*0: Rx*/
    
    pUrb = MGC_GetCurrentUrb(pEnd);
    if (!pUrb)
    {
        INFO("[USB]Urb has been freed before packet comeback. Rx bEnd =%d.\n", bEnd);
        MGC_WriteCsr16(pBase, MGC_O_HDRC_RXCSR, bEnd, 0);
        return ;
    } 

    MGC_SelectEnd(pBase, bEnd);

    wRxCsrVal = MGC_ReadCsr16(pBase, MGC_O_HDRC_RXCSR, bEnd);
    wVal = wRxCsrVal;
    wRxCount = MGC_ReadCsr16(pBase, MGC_O_HDRC_RXCOUNT, bEnd);
#if 1
	if(MGC_WIFIHotbootSupported(pUrb))
	{
		if(mgc_berror && (mgc_resumtcnt < mgc_printnum) && usb_pipebulk(pUrb->pipe)&&(usb_pipein(pUrb->pipe)== 0) && ((uint8_t)usb_pipeendpoint(pUrb->pipe)==8))
		{
			INFO("[USB %d]pUrb=0x%p, pUrb->hcpriv=%p, length=%d proto=%s\n", mgc_resumtcnt, pUrb, pUrb->hcpriv, pUrb->transfer_buffer_length, MGC_DecodeUrbProtocol(pUrb));
			INFO("[USB]end %d wRxCsrVal=%04x, wRxCount=%d, pUrb->actual_length=%d\n", bEnd, wRxCsrVal, wRxCount, pUrb->actual_length);
		}
	}
#endif

    DBG(3, "[USB]end %d wRxCsrVal=%04x, wRxCount=%d, pUrb->actual_length=%d\n", bEnd, wRxCsrVal, wRxCount, pUrb->actual_length);

	if(pUrb->dev)
    {
        if(pUrb->dev->descriptor.idVendor == 0x152D && pUrb->dev->descriptor.idProduct == 0x2339)
        {
            bMaxRetries = 10;
        }
    }
    /* check for errors, concurrent stall & unlink is not really
     * handled yet! */
    if (wRxCsrVal &MGC_M_RXCSR_H_RXSTALL)
    {
        DBG(1, "[USB]RX end %d STALL (%d:%d) !\n", bEnd, pEnd->bRemoteAddress, pEnd->bEnd);
        status = USB_ST_STALL;
    }
    else if (wRxCsrVal &MGC_M_RXCSR_H_ERROR)
    {
        DBG(1, "[USB]end %d Rx error (%d:%d) !\n", bEnd, pEnd->bRemoteAddress, pEnd->bEnd);
		//20120713 USB Device's compatibility is not good, not data from device, retry for more times to cover it. 
		if ((-EINPROGRESS)  == pUrb->status && PIPE_BULK == pEnd->bTrafficType 
            && ++pEnd->bRetries < MGC_MAX_ERR_RETRIES)
        {
                wRxCsrVal &= ~MGC_M_RXCSR_H_ERROR;
                wRxCsrVal &= ~MGC_M_RXCSR_RXPKTRDY;
                MGC_WriteCsr16(pBase, MGC_O_HDRC_RXCSR, bEnd, wRxCsrVal | MGC_M_RXCSR_H_REQPKT);
                DBG(1, "==> cover rx error\n");
                return;           
        }
        else
        {            
        status = USB_ST_NORESPONSE;

        /* do the proper sequence to abort the transfer */
        wVal &= ~MGC_M_RXCSR_H_REQPKT;
        MGC_WriteCsr16(pBase, MGC_O_HDRC_RXCSR, bEnd, wVal);
        MGC_WriteCsr8(pBase, MGC_O_HDRC_RXINTERVAL, bEnd, 0);
        }
    }
    else if (wRxCsrVal &MGC_M_RXCSR_DATAERR)
    {
        if (PIPE_BULK == pEnd->bTrafficType)
        {
            /* cover it up if retries not exhausted, slow devices might  
             * not answer quickly enough: I was expecting a packet but the 
             * packet didn't come. The interrupt is generated after 3 failed
             * attempts, it make MUSB_MAX_RETRIESx3 attempts total..
             */
            if (pUrb->status == ( - EINPROGRESS) && ++pEnd->bRetries < bMaxRetries)
            {
                /* silently ignore it */
                wRxCsrVal &= ~MGC_M_RXCSR_DATAERR;
                wRxCsrVal &= ~MGC_M_RXCSR_RXPKTRDY;
                MGC_WriteCsr16(pBase, MGC_O_HDRC_RXCSR, bEnd, wRxCsrVal | MGC_M_RXCSR_H_REQPKT);

                DBG(1, "[USB]Rx NAK Timeout (%d:%d), Retry %d !\n", pEnd->bRemoteAddress, pEnd->bEnd, pEnd->bRetries);
                return ;
            }

            if (pUrb->status == ( - EINPROGRESS))
            {
                DBG(1, "[USB]RX NAK Timeout !\n");
                status = USB_ST_NORESPONSE;
            }

            wVal &= ~MGC_M_RXCSR_H_REQPKT;
            MGC_WriteCsr16(pBase, MGC_O_HDRC_RXCSR, bEnd, wVal);
            MGC_WriteCsr8(pBase, MGC_O_HDRC_RXINTERVAL, bEnd, 0);
            pEnd->bRetries = 0;

            /* do the proper sequence to abort the transfer; 
             * am I dealing with a slow device maybe? */
            INFO("[USB]end=%d device Rx not responding (%d:%d) !\n", bEnd, pEnd->bRemoteAddress, pEnd->bEnd);

        }
        else if (PIPE_ISOCHRONOUS == pEnd->bTrafficType)
        {
            DBG(3, "[USB]bEnd=%d Isochronous error\n", bEnd);
            bIsochError = TRUE;
        }
    }

    /* an error won't process the data */
    if (status)
    {
        pUrb->status = status;

        /* data errors are signaled */
        if (USB_ST_STALL != status)
        {
            DBG(1, "[USB]end %d Rx error, status=%d !\n", bEnd, status);
        }
        else
        {
            INFO("[USB]MGC_ServiceRxReady : Flush FIFO !\n");
            /* twice in case of double packet buffering */
            MGC_WriteCsr16((uint8_t*)pThis->pRegs, MGC_O_HDRC_RXCSR, bEnd, MGC_M_RXCSR_FLUSHFIFO | MGC_M_RXCSR_CLRDATATOG);
            MGC_WriteCsr16((uint8_t*)pThis->pRegs, MGC_O_HDRC_RXCSR, bEnd, MGC_M_RXCSR_FLUSHFIFO | MGC_M_RXCSR_CLRDATATOG);
        }

        DBG(3, "[USB]clearing all error bits, right away\n");
        wVal &= ~(MGC_M_RXCSR_H_ERROR | MGC_M_RXCSR_DATAERR | MGC_M_RXCSR_H_RXSTALL);
        wVal &= ~MGC_M_RXCSR_RXPKTRDY;
        MGC_WriteCsr16(pBase, MGC_O_HDRC_RXCSR, bEnd, wVal);
    }

    /* no errors, unload... */
    if (pUrb->status == ( - EINPROGRESS))
    {
        /* be sure a packet is ready for unloading */
        if (!wRxCsrVal &MGC_M_RXCSR_RXPKTRDY)
        {
            pUrb->status = USB_ST_INTERNALERROR;

            /* do the proper sequence to abort the transfer */
            wVal &= ~MGC_M_RXCSR_H_REQPKT;
            MGC_WriteCsr16(pBase, MGC_O_HDRC_RXCSR, bEnd, wVal);
            DBG(3, "[USB]Rx interrupt with no errors or packet!\n");
        }
        else
        {        
            /* we are expecting traffic */
            if (pEnd->pDmaChannel)
            {
                pDmaChannel = pEnd->pDmaChannel;
                
                if (usb_pipeisoc(pUrb->pipe))
                {                    
                    packet = &pUrb->iso_frame_desc[pEnd->dwIsoPacket];                        
                    /* update actual & status */
                    packet->actual_length = pDmaChannel->dwActualLength;                    
                    packet->status = USB_ST_NOERROR;
                    pUrb->actual_length += pDmaChannel->dwActualLength;

                    // Free DMA.
                    MGC_HsDmaReleaseChannel(pDmaChannel);
                    pEnd->pDmaChannel = NULL;
                    pEnd->dwRequestSize = 0;
                    pEnd->dwIsoPacket ++;

                    if (pEnd->dwIsoPacket < pUrb->number_of_packets)
                    {
                        MGC_WriteCsr16(pBase, MGC_O_HDRC_RXCSR, bEnd, MGC_M_RXCSR_H_REQPKT);                        
                        return;
                    }
                    else
                    {
                        //Finish iso transfer.
                        MGC_WriteCsr16(pBase, MGC_O_HDRC_RXCSR, bEnd, 0);
                        pUrb->status = 0;
                        goto rx_urb_end;
                    }
                }

                bStatus = MGC_HsDmaGetChannelStatus(pDmaChannel);
                pEnd->dwOffset += pDmaChannel->dwActualLength;
                pUrb->actual_length += pDmaChannel->dwActualLength;

                if (MGC_DMA_STATUS_FREE == bStatus)
                {                
                    /* see if we are done */
                    bDone = (pEnd->dwOffset >= pUrb->transfer_buffer_length) || (pDmaChannel->dwActualLength < pEnd->wPacketSize);

                    if (bEnd && bDone)
                    {
                        // clear rxpktrdy.
                        MGC_WriteCsr16(pBase, MGC_O_HDRC_RXCSR, bEnd, 0);
                        pUrb->status = 0;
                    }
                    else
                    {
                        dwLength = pUrb->transfer_buffer_length - pEnd->dwOffset;
                        if (dwLength > pEnd->wPacketSize)
                        {
                            INFO("[USB]CASE1 %d %d %d\n", dwLength, pUrb->transfer_buffer_length, pEnd->dwOffset);
                            pDmaBuffer = (uint8_t*)pUrb->transfer_dma;
                            pDmaBuffer += pEnd->dwOffset;                          
                            pDmaChannel->dwStartAddress = (ulong)pDmaBuffer;
                            pDmaChannel->dwCount = (ulong)dwLength;
                            pDmaChannel->dwActualLength = 0;
                            pDmaChannel->bDesiredMode = 1;
                            pEnd->dwRequestSize = dwLength;
                            bDmaOk = MGC_HsDmaProgramChannel(pDmaChannel);
                            if (!bDmaOk)
                            {
                                MGC_HsDmaReleaseChannel(pDmaChannel);
                                pEnd->pDmaChannel = NULL;
                                pEnd->dwRequestSize = 0;
                            }
                        }
                        else
                        {
                            INFO("[USB]CASE2 ! %d %d %d\n", dwLength, pUrb->transfer_buffer_length, pEnd->dwOffset);
                            /* release previously-allocated channel */
                            MGC_HsDmaReleaseChannel(pDmaChannel);
                            pEnd->pDmaChannel = NULL;

                            MGC_WriteCsr16(pBase, MGC_O_HDRC_RXCSR, bEnd, MGC_M_RXCSR_H_REQPKT);
                        }
                    }
                }
                else if (MGC_DMA_STATUS_MODE1_SHORTPKT == bStatus)
                {
                    /*
                    This case must be vary careful.
                    Short packet which is received by MGC_RxPacket() will cause bug.
                    When unmap_urb_for_dma() to invalidate cache, MGC_RxPacket() data will be trashed.
                    We use DMA to receive this short packet.
                    */
                    dwLength = min((ulong)wRxCount, 
                        (ulong)(pUrb->transfer_buffer_length - pUrb->actual_length));

                    /*
                        IntrRxE, IntrUSB, and IntrUSBE are the same 32 bits group.
                        Tricky: Set 0 in all write clear field in IntrUSB field. Prevent to clear IntrUSB.
                        Because our 32 bits register access mode.            
                    */
                    dwIntr = MGC_Read32(pBase, MGC_O_HDRC_INTRRXE);
                    dwIntr &= ~((ulong)(1 << bEnd));
                    dwIntr &= 0xFF00FFFE;
                    MGC_Write32(pBase, MGC_O_HDRC_INTRRXE, (ulong)dwIntr);

                    // Clear RXCSR will trigger one more endpoint interrupt.
                    wRxCsrVal = MGC_ReadCsr16(pBase, MGC_O_HDRC_RXCSR, bEnd);
                    wRxCsrVal &= ~(MGC_M_RXCSR_AUTOCLEAR |MGC_M_RXCSR_DMAENAB|MGC_M_RXCSR_DMAMODE);
                    MGC_WriteCsr16(pBase, MGC_O_HDRC_RXCSR, bEnd, wRxCsrVal);

                    // Clear rx endpoint interrupt.
                    MGC_Write16(pBase, MGC_O_HDRC_INTRRX, (1 << bEnd));

                    // Enable endpoint interrupt after flush fifo.
                    /*
                        IntrRxE, IntrUSB, and IntrUSBE are the same 32 bits group.
                        Tricky: Set 0 in all write clear field in IntrUSB field. Prevent to clear IntrUSB.
                        Because our 32 bits register access mode.            
                    */
                    dwIntr = MGC_Read32(pBase, MGC_O_HDRC_INTRRXE);
                    dwIntr |= ((ulong)(1 << bEnd));
                    dwIntr &= 0xFF00FFFE;
                    MGC_Write32(pBase, MGC_O_HDRC_INTRRXE, (ulong)dwIntr);
                    
                    // Use mode 0 to receive short packet.
                    pDmaBuffer = (uint8_t*)pUrb->transfer_dma;
                    pDmaBuffer += pEnd->dwOffset;
                    pDmaChannel->dwStartAddress = (ulong)pDmaBuffer;
                    pDmaChannel->dwCount = (ulong)dwLength;
                    pDmaChannel->dwActualLength = 0;
                    pDmaChannel->bDesiredMode = 0;
                    pEnd->dwRequestSize = dwLength;
                    bDmaOk = MGC_HsDmaProgramChannel(pDmaChannel);
                    if (!bDmaOk)
                    {
                        MGC_HsDmaReleaseChannel(pDmaChannel);
                        pEnd->pDmaChannel = NULL;
                        pEnd->dwRequestSize = 0;
                    }
                }
            }
            else
            {
                dwLength = min((ulong)wRxCount, 
                    (ulong)(pUrb->transfer_buffer_length - pUrb->actual_length));

                pEnd->pDmaChannel = 
                    MGC_HsDmaAllocateChannel(pUrb, &pThis->rDma, bEnd, FALSE, 
                        dwLength, (uint8_t)usb_pipetype(pUrb->pipe), pEnd->wPacketSize);

                if (pEnd->pDmaChannel)
                {
                    pDmaChannel = pEnd->pDmaChannel;

                    // Use mode 0 to receive short packet.
                    pDmaBuffer = (uint8_t*)pUrb->transfer_dma;
                    
                    if (usb_pipeisoc(pUrb->pipe))
                    {
                        packet = &pUrb->iso_frame_desc[pEnd->dwIsoPacket];
                        pDmaBuffer += packet->offset;
                    }
                    else
                    {
                        pDmaBuffer += pEnd->dwOffset;
                    }                    
                    
                    pDmaChannel->dwStartAddress = (ulong)pDmaBuffer;
                    pDmaChannel->dwCount = dwLength;
                    pDmaChannel->dwActualLength = 0;
                    pDmaChannel->bDesiredMode = 0;               
                    pEnd->dwRequestSize = dwLength;
                    bDmaOk = MGC_HsDmaProgramChannel(pDmaChannel);

                    if (!bDmaOk)
                    {
                        MGC_HsDmaReleaseChannel(pDmaChannel);

                        pEnd->pDmaChannel = NULL;
                        pEnd->dwRequestSize = 0;

                        (void)MGC_RxPacket(pThis, pEnd, (uint16_t)dwLength);
                    }
                }
                else
                {
                    (void)MGC_RxPacket(pThis, pEnd, (uint16_t)dwLength);
                }
            }
        }
    }

rx_urb_end:
    /* complete the current request or start next one clearing RxPktRdy 
     * and setting ReqPkt */
    if (pUrb->status != ( - EINPROGRESS))
    {
        //  set data toggle in device basis for supporting Hub.
        // get data toggle bit from physical ep.
        toggle = ((MGC_Read32(pBase, M_REG_RXDATATOG)) >> bEnd) &0x01;
        // release dma channel.
        if (pEnd->pDmaChannel)
        {
            /* release previously-allocated channel */
            MGC_HsDmaReleaseChannel(pEnd->pDmaChannel);
            pEnd->pDmaChannel = NULL;
        }

        if (pUrb->status == USB_ST_STALL)
        {
            toggle = 0;
        } 

            /* save data toggle */
        usb_settoggle(pUrb->dev, pEnd->bRemoteEnd, FALSE, toggle);

        MGC_DequeueEndurb(pThis, pEnd, pUrb);

        pEnd->bBusyCompleting = 1;
        spin_unlock_irqrestore(&pThis->SelectEndLock,irq_flags); 
        MGC_CompleteUrb(pThis, pEnd, pUrb);
        spin_lock_irqsave(&pThis->SelectEndLock,irq_flags);
        pEnd->bBusyCompleting = 0;
        
        if (!MGC_IsEndIdle(pEnd))
        {
            MGC_KickUrb(pThis, MGC_GetNextUrb(pEnd), pEnd);
        }
    }
}

static void MGC_ResetOff(unsigned long param)
{
    uint8_t power;
    MGC_LinuxCd *pThis = (MGC_LinuxCd*)param;
    void *pBase = pThis->pRegs;
    unsigned long flags = 0;

    spin_lock_irqsave(&pThis->SelectEndLock, flags);

    power = MGC_Read8(pBase, MGC_O_HDRC_POWER);
    MGC_Write8(pBase, MGC_O_HDRC_POWER, power &~MGC_M_POWER_RESET);
    DBG(-1, "[USB]Reset off\n");
    /* check for high-speed and set in root device if so */
    power = MGC_Read8(pBase, MGC_O_HDRC_POWER);
    if (power &MGC_M_POWER_HSMODE)
    {
        DBG(-1, "[USB]high-speed device connected\n");
        pThis->bRootSpeed = MGC_SPEED_HS;
    }

    // disable reset status.
    pThis->dwVirtualHubPortStatus &= ~(1 << USB_PORT_FEAT_RESET);

    // reset complete. device still connect, set port enable.
    pThis->dwVirtualHubPortStatus |= (1 << USB_PORT_FEAT_C_RESET) | (1 << USB_PORT_FEAT_ENABLE);

    if (pThis->bRootSpeed == MGC_SPEED_HS)
    {
        pThis->dwVirtualHubPortStatus &= ~(1 << USB_PORT_FEAT_LOWSPEED);
        pThis->dwVirtualHubPortStatus |= USB_PORT_STAT_HIGH_SPEED;
    }
    else if (pThis->bRootSpeed == MGC_SPEED_FS)
    {
        pThis->dwVirtualHubPortStatus &= ~(1 << USB_PORT_FEAT_LOWSPEED);
    }
    else
    {
        pThis->dwVirtualHubPortStatus |= (1 << USB_PORT_FEAT_LOWSPEED);
    }

    spin_unlock_irqrestore(&pThis->SelectEndLock, flags);    
}

static void MGC_UnlinkUrb(MGC_LinuxCd *pThis, struct urb *pUrb)
{
    MGC_LinuxLocalEnd *pEnd;
    const void *pBase = pThis->pRegs;
    uint16_t wIntr;
	int toggle;

    pEnd = (MGC_LinuxLocalEnd*)pUrb->hcpriv;

	DBG(3,"cur urbUrb=0x%p, dequeue urb=0x%p.\n", pEnd->pCurrentUrb, pUrb);
	if(MGC_GetCurrentUrb(pEnd) != NULL)
		DBG(3,"cur curUrb info: pUrb=0x%p, bReEnd-%d, length=%d proto=%s.\n ", pEnd->pCurrentUrb, usb_pipeendpoint(pEnd->pCurrentUrb->pipe), pEnd->pCurrentUrb->transfer_buffer_length, 
	MGC_DecodeUrbProtocol(pEnd->pCurrentUrb));
	if (MGC_GetCurrentUrb(pEnd)== pUrb)
    {
        // release dma channel.
        if (pEnd->pDmaChannel)
        {
            /* release previously-allocated channel */
            MGC_HsDmaReleaseChannel(pEnd->pDmaChannel);
            pEnd->pDmaChannel = NULL;
        }
        
        if (pEnd->bEnd)
        {                
            if (!pEnd->bIsTx)                    
            {
                /* twice in case of double packet buffering */
				if(!pEnd->bDoublePacketBuffer)
				{
	                MGC_WriteCsr16((uint8_t*)pThis->pRegs, MGC_O_HDRC_RXCSR, pEnd->bEnd, MGC_M_RXCSR_FLUSHFIFO | MGC_M_RXCSR_CLRDATATOG);
	                MGC_WriteCsr16((uint8_t*)pThis->pRegs, MGC_O_HDRC_RXCSR, pEnd->bEnd, MGC_M_RXCSR_FLUSHFIFO | MGC_M_RXCSR_CLRDATATOG);
				}
                // Clear interrupt.
                wIntr = MGC_Read16(pBase, MGC_O_HDRC_INTRRX);
                if (wIntr &(1 << pEnd->bEnd))
                {
                    MGC_Write16(pBase, MGC_O_HDRC_INTRRX, (1 << pEnd->bEnd));
                }
            }
            else
            {                
                MGC_CheckTxCsrValidForWrite(pThis, pEnd->bEnd, 0x08);
				/* save data toggle */
				toggle = ((MGC_Read32(pBase, M_REG_TXDATATOG)) >> (pEnd->bEnd)) & 0x01;
				usb_settoggle(pUrb->dev, pEnd->bRemoteEnd, TRUE, toggle);
				
				if(MGC_WIFIHotbootSupported(pUrb) && (pEnd->bRemoteEnd == 8) && usb_pipebulk(pUrb->pipe)&&(usb_pipein(pUrb->pipe)== 0))
				{
					INFO("[%s][%d][WIFI EP%d]--- Set Toggle = %d/%d, length=%d, mgc_togglecnt = %d  mgc_EP8_FIFO_TX_DONE = %d TXRDYPT = 0x%x.\n",__FUNCTION__,__LINE__,pEnd->bRemoteEnd, mgc_toggle_prev, toggle, pUrb->transfer_buffer_length,mgc_togglecnt,mgc_EP8_FIFO_TX_DONE,MGC_ReadCsr16(pBase, MGC_O_HDRC_TXCSR, pEnd->bEnd));

					if((mgc_toggle_prev == toggle)&&(mgc_EP8_FIFO_TX_DONE==1))
					{
						//msleep(10);
						toggle = ((MGC_Read32(pBase, M_REG_TXDATATOG)) >> (pEnd->bEnd)) & 0x01;						
						INFO("[WIFI]Set again Toggle = %d/%d.\n", mgc_toggle_prev, toggle);
						if(mgc_toggle_prev == 0)
							toggle = 1;
						else
							toggle = 0;
						usb_settoggle(pUrb->dev, pEnd->bRemoteEnd, TRUE, toggle);
						INFO("[WIFI]After set Toggle = %d/%d.\n", mgc_toggle_prev, toggle);
					}
				}
				

                /* twice in case of double packet buffering */
				if(!pEnd->bDoublePacketBuffer)
				{
	                MGC_WriteCsr16(pBase, MGC_O_HDRC_TXCSR, pEnd->bEnd, MGC_M_TXCSR_FLUSHFIFO | MGC_M_TXCSR_CLRDATATOG);
	                MGC_WriteCsr16(pBase, MGC_O_HDRC_TXCSR, pEnd->bEnd, MGC_M_TXCSR_FLUSHFIFO | MGC_M_TXCSR_CLRDATATOG);
				}
                // Clear interrupt.
                wIntr = MGC_Read16(pBase, MGC_O_HDRC_INTRTX);
                if (wIntr &(1 << pEnd->bEnd))
                {
                    MGC_Write16(pBase, MGC_O_HDRC_INTRTX, (1 << pEnd->bEnd));
                }                    
            }
        }
        else
        {
            /* endpoint 0: just flush */
            MGC_WriteCsr16(pBase, MGC_O_HDRC_CSR0, pEnd->bEnd, MGC_M_CSR0_FLUSHFIFO);
            MGC_WriteCsr16(pBase, MGC_O_HDRC_CSR0, pEnd->bEnd, MGC_M_CSR0_FLUSHFIFO);

            wIntr = MGC_Read16(pBase, MGC_O_HDRC_INTRTX);
            if (wIntr & 1)
            {
                MGC_Write16(pBase, MGC_O_HDRC_INTRTX, 1);
            }
        }
    }

    MGC_DequeueEndurb(pThis, pEnd, pUrb);
}

//-------------------------------------------------------------------------
/** MUC_Irq
 *  system usb irq entry point.
 *  @param   irq                interrupt number.
 *  @param   *_prHcd       point to mtkhcd data structure.
 *  @param   *regs           cpu register before interrupt.
 *  @return  irqreturn_t    irq status: IRQ_HANDLED means successfully irq handled.
 */
//-------------------------------------------------------------------------
static irqreturn_t MUC_Irq(struct usb_hcd *hcd)
{
    MGC_LinuxCd *pThis = hcd_to_musbstruct(hcd);

    const void *pBase = pThis->pRegs;
    uint32_t nSource = 0;
    uint8_t bIntrUsbValue;
    uint16_t wIntrTxValue;
    uint16_t wIntrRxValue;
    uint32_t dwLineState;
    uint8_t bIndex;
    uint8_t bShift = 0;
    uint8_t bPort=0;
    uint32_t reg;
#ifdef CONFIG_USB_QMU_SUPPORT
    uint32_t dwIntrQisar;
#endif 


	bPort = MGC_GetPortNum(pBase);

	spin_lock_irqsave(&pThis->SelectEndLock,irq_flags);

    /* save index */
    bIndex = MGC_Read8(pBase, MGC_O_HDRC_INDEX);

    if ((pThis == NULL) || (hcd->state != HC_STATE_RUNNING))
    {
		 INFO("[USB]IRQ happens after hcd removed !,(pThis == NULL):%d,(hcd->state != HC_STATE_RUNNING):%d\n",(pThis == NULL),(hcd->state != HC_STATE_RUNNING));
		 INFO("[USB]IRQ happens after hcd removed 0x%x , 0x%x, 0x%x, 0x%x!\n", hcd->state, 
			 MGC_Read8(pBase, MGC_O_HDRC_INTRUSB), MGC_Read16(pBase, MGC_O_HDRC_INTRTX), MGC_Read16(pBase, MGC_O_HDRC_INTRRX));
		// goto exit_muc_irq;
    }

    /* ### DMA intr handler added */
    if (MGC_HsDmaControllerIsr(&pThis->rDma))
    {
        nSource |= 1;
    }

    bIntrUsbValue = MGC_Read8(pBase, MGC_O_HDRC_INTRUSB);
    wIntrTxValue = MGC_Read16(pBase, MGC_O_HDRC_INTRTX);
    wIntrRxValue = MGC_Read16(pBase, MGC_O_HDRC_INTRRX);
#ifdef CONFIG_USB_QMU_SUPPORT
    dwIntrQisar = MGC_Read32(pBase, M_REG_QISAR);
#endif

    if (bIntrUsbValue)
    {
        MGC_Write8(pBase, MGC_O_HDRC_INTRUSB, bIntrUsbValue);

        bIntrUsbValue &= MGC_Read8(pBase, MGC_O_HDRC_INTRUSBE);
    }

    if (wIntrTxValue)
    {
        MGC_Write16(pBase, MGC_O_HDRC_INTRTX, wIntrTxValue);

        wIntrTxValue &= MGC_Read16(pBase, MGC_O_HDRC_INTRTXE);
    }

    if (wIntrRxValue)
    {
        MGC_Write16(pBase, MGC_O_HDRC_INTRRX, wIntrRxValue);

        wIntrRxValue &= MGC_Read16(pBase, MGC_O_HDRC_INTRRXE);
    }

#ifdef CONFIG_USB_QMU_SUPPORT
    if (dwIntrQisar)
    {
        MGC_Write32(pBase, M_REG_QISAR, dwIntrQisar);
        dwIntrQisar &= ~(MGC_Read32(pBase, M_REG_QIMR));   
    }
#endif

    if (bIntrUsbValue == (MGC_M_INTR_CONNECT | MGC_M_INTR_DISCONNECT))
    {
        INFO("[USB%d]Connect/Disconnet interrupt = 0x%X.\n", bPort, bIntrUsbValue);

        // treat it as disconnect interrupt only.
        bIntrUsbValue &= ~MGC_M_INTR_CONNECT;
    }

    if (bIntrUsbValue &MGC_M_INTR_CONNECT)
    {
        dwLineState = LINESTATE_FS_SPEED;
        INFO("[USB%d]Connect interrupt  = 0x%X.\n", bPort, bIntrUsbValue);
	

       if ((!pThis->bInsert) && (dwLineState != LINESTATE_DISCONNECT)&&(bPowerStatus[bPort]==VBUS_ON))
        {
            pThis->bCheckInsert = 0;
			
	        mod_timer(&pThis->events_timer, 
				      jiffies + msecs_to_jiffies(MGC_CHECK_INSERT_DEBOUNCE));
			pThis->events |= USB_EVENTS_CHECK_CONNECT;
        }

        // When connect and disconnet happen at the same time,
        // we only handle connect event. This will happen at Vbus not stable when cable is  inserted.
        bIntrUsbValue &= ~(MGC_M_INTR_CONNECT);
    }
    else if (bIntrUsbValue &MGC_M_INTR_DISCONNECT)
    // disconnect event.   
    {
        INFO("[USB%d]Disconnect interrupt  = 0x%X.\n", bPort, bIntrUsbValue);
#ifdef CONFIG_USB_DETECT_IMPROVE
		if(pThis->bInsert || !MGC_IsDevAttached((uint8_t *)pBase))
#endif
		{
        /* <lawrance.liu@mediatek.com> 20100516 */
        /* Patched from BDP, To avoid card reader plug/unplug problem */
        pThis->dwVirtualHubPortStatus = (1 << USB_PORT_FEAT_POWER) | (1 << USB_PORT_FEAT_C_CONNECTION);
        pThis->dwVirtualHubPortStatus &= ~(1 << USB_PORT_FEAT_CONNECTION);
        usb_hcd_poll_rh_status(hcd);


        if (!pThis->bInsert)
        {
            // no need to handle this disconnect event.
            bIntrUsbValue &= ~(MGC_M_INTR_DISCONNECT);
        }
        else
        {
            pThis->bInsert = FALSE;
        }
        #ifdef MGC_UNLINK_URB
		if(bPowerStatus[bPort]==VBUS_ON)
		{
	        mod_timer(&pThis->events_timer, jiffies
						+ msecs_to_jiffies(20));
	        pThis->events = USB_EVENTS_UNLINK_INVALID_URB;
		}
		#endif
    }
#ifdef CONFIG_USB_DETECT_IMPROVE
	else
	{
		bIntrUsbValue &= ~(MGC_M_INTR_DISCONNECT);
		printk("[USB%d]check linestate later!\n", bPort);
	}
#endif
#ifdef MUSB_BC12
#if defined(CONFIG_ARCH_MT5881) || defined(CONFIG_ARCH_MT5890) || \
	defined(CONFIG_ARCH_MT5891) || defined(CONFIG_ARCH_MT5882) ||\
	defined(CONFIG_ARCH_MT5883)
      if (pThis->bPortNum == 0)
	{
	    uint32_t u4Reg;
	    uint8_t power = MGC_Read8(pBase,MGC_O_HDRC_POWER);
	    uint8_t nPower= power ;

	    INFO("[USB]Re-Pull up RG_USB20_BGR_EN RG_USB20_CHP_EN on port0.\n");
	    u4Reg = MGC_PHY_Read32(pThis->pPhyBase, M_REG_PHYC0);
	    u4Reg |= 0x3;
	    MGC_PHY_Write32(pThis->pPhyBase, M_REG_PHYC0, u4Reg);

	    INFO("[USB]Re-Enable BC12 function on port%d when init.\n", pBase != (uint8_t *)MUSB_BASE);
	    INFO("[USB]Reset port\n");
	    nPower |= MGC_M_POWER_RESET ;
	    if(nPower != power)
	    {
		MGC_Write8(pBase,MGC_O_HDRC_POWER,nPower);
	    }
	    //pull up RG_CHGDT_EN
#if !(defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5891))
	    u4Reg = MGC_PHY_Read32(pThis->pPhyBase,M_REG_PHYACR5);
#else
	    u4Reg = MGC_PHY_Read32(pThis->pPhyBase,0x40); //5890 IC issue, read 0x40 instead of M_REG_U2PHYACR5(0x24)
#endif
	    u4Reg |= 0x1;
	    MGC_PHY_Write32(pThis->pPhyBase,M_REG_PHYACR5,u4Reg);
	}
#endif
#endif //MUSB_BC1
    }

#ifdef CONFIG_USB_QMU_SUPPORT
    nSource |= bIntrUsbValue | wIntrTxValue | wIntrRxValue |dwIntrQisar;
#else
    nSource |= bIntrUsbValue | wIntrTxValue | wIntrRxValue;
#endif

    if (!nSource)
    {
        goto exit_muc_irq;
    }

    if (bIntrUsbValue)
    {
        MGC_ServiceUSBIntr(pThis, bIntrUsbValue);
    }

#ifdef CONFIG_USB_QMU_SUPPORT
    /* process generic queue interrupt*/
    if (dwIntrQisar)
    {
        (void)QMU_irq(pThis, dwIntrQisar);
    }	
#endif

    /* handle tx/rx on endpoints; each bit of wIntrTxValue is an endpoint, 
     * endpoint 0 first (p35 of the manual) bc is "SPECIAL" treatment; 
     * WARNING: when operating as device you might start receving traffic 
     * to ep0 before anything else happens so be ready for it */

    reg = wIntrTxValue;
    if (reg &1)
    {
        /* EP0 */
        MGC_ServiceDefaultEnd(pThis);
    }

    /* TX on endpoints 1-15 */
    bShift = 1;
    reg >>= 1;

    while (reg)
    {
        if (reg &1)
        {
            MGC_ServiceTxAvail(pThis, bShift);
        }

        reg >>= 1;
        bShift++;
    }

    /* RX on endpoints 1-15 */
    reg = wIntrRxValue;
    bShift = 1;
    reg >>= 1;

    while (reg)
    {
        if (reg &1)
        {
            MGC_ServiceRxReady(pThis, bShift);
        }

        reg >>= 1;
        bShift++;
    }

exit_muc_irq:
    
    /* restore index */
    MGC_Write8(pBase, MGC_O_HDRC_INDEX, bIndex);
    spin_unlock_irqrestore(&pThis->SelectEndLock,irq_flags);
    
    return IRQ_HANDLED;
}

//-------------------------------------------------------------------------
/** MUC_urb_enqueue
 *  queue and process user's urb sequentially.
 *  @param   *hcd           system usb structure.
 *  @param   *urb             user's request block. 
 *  @param   mem_flags    malloc flag. 
 *  @retval   0        Success
 *  @retval  others Fail
 */
//-------------------------------------------------------------------------
static int MUC_urb_enqueue(struct usb_hcd *hcd, struct urb *pUrb, gfp_t mem_flags)
{
    MGC_LinuxCd *pThis = hcd_to_musbstruct(hcd);
    MGC_LinuxLocalEnd *pEnd = NULL;
    unsigned int pipe = pUrb->pipe;
    unsigned long flags = 0;
    int rc;

    /* host role must be active */
    if(!pThis->bInsert)
    {
	DBG(1, "usb device disconnect.\n");
	DBG(3, "[USB] MUC_urb_enqueue: pUrb=0x%p, pUrb->hcpriv=%p proto=%s\n", pUrb, pUrb->hcpriv, MGC_DecodeUrbProtocol(pUrb));
		//return -ENODEV;
    }
    spin_lock_irqsave(&pThis->SelectEndLock, flags);
	if(MGC_WIFIHotbootSupported(pUrb))
	{
		//if(mgc_ep0kicktimes != mgc_ep0callbacktimes)
			//INFO("[USB]WARNINIG... WIFI maybe do not has callback, %d: %d.\n", mgc_ep0kicktimes, mgc_ep0callbacktimes);
		mgc_ep0kicktimes ++;
	
		//Leon DEBUG_PRINTK
		if((mgc_resumtcnt < mgc_printnum) && (((uint8_t)usb_pipeendpoint(pUrb->pipe) ==8)))
		{
			// if need printk bulk-out, need set mgc_bulkset = 1, mgc_bulkoutset=1 
			if(mgc_bulkset && usb_pipebulk(pUrb->pipe)) 
			{
				if(mgc_bulkoutset && (usb_pipein(pUrb->pipe)== 0) )
				{
					//INFO("[USB %d]pUrb=0x%p, bReEnd-%d, interval=%d, length=%d proto=%s\n", mgc_resumtcnt, pUrb, usb_pipeendpoint(pUrb->pipe), pUrb->interval, pUrb->transfer_buffer_length,MGC_DecodeUrbProtocol(pUrb));
				}				
			}
			
			// if print bulk in+out, set mgc_bulkset =1, mgc_bulkoutset=0
			if(mgc_bulkset && usb_pipebulk(pUrb->pipe)) 
			{
				if(mgc_bulkoutset == 0)
				{
					INFO("[USB %d]pUrb=0x%p, bReEnd-%d, interval=%d, length=%d proto=%s\n", mgc_resumtcnt, pUrb, usb_pipeendpoint(pUrb->pipe), pUrb->interval, pUrb->transfer_buffer_length,MGC_DecodeUrbProtocol(pUrb));
				}		
			}

			// printk control, set mgc_bulkset=0, mgc_bulkoutset=any
			if((mgc_bulkset == 0) && (usb_pipecontrol(pUrb->pipe)))
			{
				INFO("[USB %d]pUrb=0x%p, bReEnd-%d, interval=%d, length=%d proto=%s\n", mgc_resumtcnt, pUrb, usb_pipeendpoint(pUrb->pipe), pUrb->interval, pUrb->transfer_buffer_length,MGC_DecodeUrbProtocol(pUrb));
			}
		}
	}

    DBG(2, "[USB]pUrb=0x%p, pUrb->hcpriv=%p proto=%s\n", pUrb, pUrb->hcpriv, MGC_DecodeUrbProtocol(pUrb));

    /* find appropriate local endpoint to do it */
    pEnd = MGC_FindEnd(pThis, pUrb);
    if (pEnd == NULL)
    {
        INFO("[USB]==> no resource for proto=%d, addr=%d, end=%d\n", 
            usb_pipetype(pipe), usb_pipedevice(pipe), usb_pipeendpoint(pipe));
        spin_unlock_irqrestore(&pThis->SelectEndLock, flags);
        return USB_ST_URB_REQUEST_ERROR;
    }

	pEnd->mem_flags = mem_flags;
	
    rc = MGC_ScheduleUrb(pThis, pEnd, pUrb);
    spin_unlock_irqrestore(&pThis->SelectEndLock, flags);

    return rc;
}

//-------------------------------------------------------------------------
/** MUC_urb_dequeue
 *  kill user's urb.
 *  @param   *hcd           system usb structure.
 *  @param   *urb             user's request block. 
 *  @retval   0        Success
 *  @retval  others Fail
 */
//-------------------------------------------------------------------------
static int MUC_urb_dequeue(struct usb_hcd *hcd, struct urb *pUrb, int status)
{
    MGC_LinuxCd *pThis = hcd_to_musbstruct(hcd);
    MGC_LinuxLocalEnd *pEnd;
    int result;
    unsigned long flags;
    
    if (!pUrb)
    {
        DBG(1, "[USB]==> invalid urb%p, pUrb->hcpriv=%p\n", pUrb, (pUrb) ? pUrb->hcpriv: NULL);
        return USB_ST_URB_INVALID_ERROR;
    } 

    if (!pThis)
    {
        ERR("[USB]==> pThis is null: stopping before unlink\n");
        return USB_ST_REMOVED;
    }

	if(MGC_WIFIHotbootSupported(pUrb))
	{
		if(usb_pipeendpoint(pUrb->pipe) == 8)
		{
			mgc_berror = 1;
		}

		if(usb_pipecontrol(pUrb->pipe))
		{
			mgc_ep0kicktimes = 0;
			mgc_ep0callbacktimes = 0;
		}
	}
    spin_lock_irqsave(&pThis->SelectEndLock, flags);
    result = usb_hcd_check_unlink_urb(hcd, pUrb, status);
    if (result || (!pUrb->hcpriv))
    {
        spin_unlock_irqrestore(&pThis->SelectEndLock, flags);
        return result;
    }

    pUrb->status = status;   

    pEnd = (MGC_LinuxLocalEnd*)pUrb->hcpriv;

#ifdef CONFIG_USB_QMU_SUPPORT
	if ((pEnd->bCmdQEnable) || MGC_IsCmdQSupport(pThis, pEnd, pUrb))
	{		 
		//giveback all urbs
		while(!MGC_IsEndIdle(pEnd)){	

			pUrb = MGC_GetNextUrb(pEnd);
			MGC_DequeueEndurb(pThis, pEnd, pUrb);

			pUrb->status = status;   
			spin_unlock_irqrestore(&pThis->SelectEndLock, flags);

			MGC_CallbackUrb(pThis, pUrb);
			spin_lock_irqsave(&pThis->SelectEndLock, flags);
	     } 
		 
		spin_unlock_irqrestore(&pThis->SelectEndLock, flags);
		QMU_disable_q(pThis, pEnd->bEnd, !pEnd->bIsTx);

	}		 
	else
#endif		
	{
	    MGC_UnlinkUrb(pThis, pUrb);
	    spin_unlock_irqrestore(&pThis->SelectEndLock, flags);	    
	    MGC_CallbackUrb(pThis, pUrb);
	}
    return result;
}

//-------------------------------------------------------------------------
/** MUC_endpoint_disable
 *  disable user's ep.
 *  @param   *hcd             system usb hcd structure.
 *  @param   *hdev           system hcd device structure.
 *  @param   epnum          ep number.
 *  @return   void
 */
//-------------------------------------------------------------------------
static void MUC_endpoint_disable(struct usb_hcd *hcd, struct usb_host_endpoint *hep)
{
    MGC_LinuxCd *pThis = hcd_to_musbstruct(hcd);
    unsigned long flags = 0;
    MGC_LinuxLocalEnd *pEnd;
    struct urb* pUrb;
    DBG(1, "[USB]MUC_endpoint_disable call.\n");

    spin_lock_irqsave(&pThis->SelectEndLock, flags);
    
    if (hep && hep->hcpriv)
    {
        pEnd = (MGC_LinuxLocalEnd*)hep->hcpriv;
        if (pEnd)
        {
             DBG(1, "[USB]MUC_endpoint_disable: h/w %s ep%d.\n", (pEnd->bIsTx ? "Out": "In"), pEnd->bEnd);                  

           	//unlink all urbs on this ep
       		while(!MGC_IsEndIdle(pEnd)){
                   INFO("[USB]MUC_endpoint_disable: list not empty.\n");
       		   pUrb = MGC_GetNextUrb(pEnd);
       		   MGC_DequeueEndurb(pThis, pEnd, pUrb);
       		   
       		   pUrb->status = -ESHUTDOWN;
       		   
       		   spin_unlock_irqrestore(&pThis->SelectEndLock, flags);
       		   
       		   MGC_CallbackUrb(pThis, pUrb);
       		   
       		   spin_lock_irqsave(&pThis->SelectEndLock, flags);
       		} 

#ifdef CONFIG_USB_QMU_SUPPORT            
            if (pEnd->bCmdQEnable)
            {
            	  QMU_disable_q(pThis, pEnd->bEnd, !pEnd->bIsTx);
	    }
#endif
			
            if (pEnd->bEnd != 0) {
		mtk_put_fifo(pThis, pEnd->wFifoAddress, pEnd->wFIFOSize);
		mtk_dump_fifo_node(pThis);
                pEnd->wFifoAddress = MGC_END0_FIFOSIZE;
                pEnd->wMaxPacketSize = 0;  /* EPx is not occupied */
		pEnd->wFIFOSize = 0;
            }
			
            hep->hcpriv = NULL;           
#ifdef MGC_PRE_CHECK_FREE_ENDPOINT
	    pEnd->bIsOccupy = FALSE;
#endif
        }
    }

    spin_unlock_irqrestore(&pThis->SelectEndLock, flags);
}
//-------------------------------------------------------------------------
/** MUC_get_frame
 *  Get sof frame variable.
 *  @param   *hcd             system usb hcd structure.
 *  @return   frame number
 */
//-------------------------------------------------------------------------
static int MUC_get_frame(struct usb_hcd *hcd)
{
    MGC_LinuxCd *pThis = hcd_to_musbstruct(hcd);

    /* wrong except while periodic transfers are scheduled;
     * never matches the on-the-wire frame;
     * subject to overruns.
     */
    //return pThis->frame;
    (void)pThis;
    return 0;
} 

//-------------------------------------------------------------------------
/** MUC_hub_status_data
 *  Virtual root hub status api.
 *  @param   *hcd             system usb hcd structure.
 *  @param   *buf             point to status buffer.
 *  @retval   0        No change hub status.
 *  @retval   1      Change hub status.
 */
//-------------------------------------------------------------------------
static int MUC_hub_status_data(struct usb_hcd *hcd, char *buf)
{
    MGC_LinuxCd *pThis = hcd_to_musbstruct(hcd);
    int32_t retval;
  
    if (!(pThis->dwVirtualHubPortStatus &MTK_PORT_C_MASK))
    {
        retval = 0;
    } 
    else
    {
        // Hub port status change. Port 1 change detected.
        *buf = (1 << 1);

        DBG(3, "[USB]port status 0x%08X has changes\n", pThis->dwVirtualHubPortStatus);
        retval = 1;
    }

    return retval;
}

//-------------------------------------------------------------------------
/** MUC_hub_control
 *  Virtual root hub control api.
 *  @param   *hcd             system usb hcd structure.
 *  @param   typeReq        request code.
 *  @param   wValue         request value.
 *  @param   wIndex         request index.
 *  @param   *buf             point to status buffer.
 *  @param   wLength       data length.
 *  @retval   0        Success.
 *  @retval   others   Fail.
 */
//-------------------------------------------------------------------------
static int MUC_hub_control(struct usb_hcd *hcd, uint16_t typeReq, uint16_t wValue, uint16_t wIndex, char *buf, uint16_t wLength)
{
	MGC_LinuxCd *pThis = hcd_to_musbstruct(hcd);
	void *pBase = pThis->pRegs;
	int32_t retval = 0;
	unsigned long flags;
	uint8_t power;
	uint8_t bPort=0;

    DBG(-1, "[USB%d]= 0x%X, 0x%X.\n", pThis->bPortNum,typeReq, wValue);
    spin_lock_irqsave(&pThis->SelectEndLock, flags);
  
  if(pBase ==MUSB_BASE0 ) bPort=0;
  else if(pBase ==MUSB_BASE1 ) bPort=1;
  else if(pBase ==MUSB_BASE2 ) bPort=2;
  else bPort=3;

    switch (typeReq)
    {
        case ClearHubFeature:
        case SetHubFeature:
            switch (wValue)
            {
            case C_HUB_OVER_CURRENT:
            case C_HUB_LOCAL_POWER:
                break;

            default:
                goto error;
            }

            break;

        case ClearPortFeature:
            if (wIndex != 1 || wLength != 0)
            {
                goto error;
            }

            switch (wValue)
            {
            case USB_PORT_FEAT_ENABLE:
                pThis->dwVirtualHubPortStatus &= (1 << USB_PORT_FEAT_POWER);
                break;

            case USB_PORT_FEAT_SUSPEND:
                if (!(pThis->dwVirtualHubPortStatus &(1 << USB_PORT_FEAT_SUSPEND)))
                {
                    break;
                }

                /* 20 msec of resume/K signaling, other irqs blocked */
                DBG(3, "[USB]start resume...\n");
                break;

            case USB_PORT_FEAT_POWER:
                break;

            case USB_PORT_FEAT_C_ENABLE:
            case USB_PORT_FEAT_C_SUSPEND:
            case USB_PORT_FEAT_C_CONNECTION:
            case USB_PORT_FEAT_C_OVER_CURRENT:
            case USB_PORT_FEAT_C_RESET:
                break;

            default:
                goto error;
            }

            pThis->dwVirtualHubPortStatus &= ~(1 << wValue);
            break;

        case GetHubDescriptor:
            MUC_hub_descriptor(pThis, (struct usb_hub_descriptor*)buf);
            break;

        case GetHubStatus:
            *(__le32*)buf = cpu_to_le32(0);
            break;

        case GetPortStatus:
            if (wIndex != 1)
            {
                goto error;
            }

            *(__le32*)buf = cpu_to_le32(pThis->dwVirtualHubPortStatus);

            #ifndef VERBOSE

                if (*(uint16_t*)(buf + 2))
                 /* only if wPortChange is interesting */
            #endif 

            {
                DBG(-1,"[USB]GetPortStatus = 0x%08X.\n", pThis->dwVirtualHubPortStatus);
            }

            break;

        case SetPortFeature:
            if (wIndex != 1 || wLength != 0)
            {
                goto error;
            }

            switch (wValue)
            {
            case USB_PORT_FEAT_ENABLE:
                break;

            case USB_PORT_FEAT_SUSPEND:
                if (pThis->dwVirtualHubPortStatus &(1 << USB_PORT_FEAT_RESET))
                {
                    goto error;
                }

                if (!(pThis->dwVirtualHubPortStatus &(1 << USB_PORT_FEAT_ENABLE)))
                {
                    goto error;
                }

                DBG(3, "[USB]suspend...\n");
                break;

            case USB_PORT_FEAT_POWER:
                break;
				
            case USB_PORT_FEAT_RESET:
                if (pThis->dwVirtualHubPortStatus &(1 << USB_PORT_FEAT_SUSPEND))
                {
                    goto error;
                }

                if (!(pThis->dwVirtualHubPortStatus &(1 << USB_PORT_FEAT_POWER)))
                {
                    break;
                }

                // reset port.
                DBG(-1, "[USB]Reset Port %d \n", bPort);
                power = MGC_Read8(pBase, MGC_O_HDRC_POWER);
                power &= ~(MGC_M_POWER_SUSPENDM | MGC_M_POWER_ENSUSPEND);
                MGC_Write8(pBase, MGC_O_HDRC_POWER, power | MGC_M_POWER_RESET);
				
                mod_timer(&pThis->events_timer, jiffies + msecs_to_jiffies(80));
            	pThis->events |= USB_EVENTS_RESET_OFF;

                break;

            default:
                goto error;
            }

            pThis->dwVirtualHubPortStatus |= 1 << wValue;
            break;

        default:
            error: 
            /* "protocol stall" on error */
            retval =  - EPIPE;
    }

    spin_unlock_irqrestore(&pThis->SelectEndLock, flags);
    return retval;
}

//-------------------------------------------------------------------------
/** MUC_bus_suspend
 *  Virtual root hub suspend.
 *  @param   *hcd             system usb hcd structure.
 *  @retval   0       Success.
 *  @retval   1      Fail.
 */
//-------------------------------------------------------------------------
static int MUC_bus_suspend(struct usb_hcd *hcd)
{
    // SOFs off
    return 0;
} 

//-------------------------------------------------------------------------
/** MUC_bus_resume
 *  Virtual root hub resume.
 *  @param   *hcd             system usb hcd structure.
 *  @retval   0       Success.
 *  @retval   1      Fail.
 */
//-------------------------------------------------------------------------
static int MUC_bus_resume(struct usb_hcd *hcd)
{
    // SOFs on
    return 0;
} 

//-------------------------------------------------------------------------
/** MUC_hub_descriptor
 *  get root hub descriptor.
 *  @param  *pHcd           mtkhcd pointer.
 *  @param   *desc             point to hub descriptor buffer.
 *  @return   void
 */
//-------------------------------------------------------------------------
static void MUC_hub_descriptor(MGC_LinuxCd *pThis, struct usb_hub_descriptor *desc)
{
    uint16_t temp = 0;

    desc->bDescriptorType = 0x29;
    desc->bHubContrCurrent = 0;

    desc->bNbrPorts = 1;
    desc->bDescLength = 9;

    /* per-port power switching (gang of one!), or none */
    desc->bPwrOn2PwrGood = 10;

    /* no overcurrent errors detection/handling */
    temp = 0x0011;

    desc->wHubCharacteristics = (__force __u16)cpu_to_le16(temp);

    /* two bitmaps:  ports removable, and legacy PortPwrCtrlMask */
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 0, 0)
    desc->bitmap[0] = 0 << 1;
    desc->bitmap[1] = ~0;
#else
    desc->u.hs.DeviceRemovable[0] = 0 << 1;	
    desc->u.hs.DeviceRemovable[1] = ~0;    
#endif

} 
#ifdef CONFIG_USB_DETECT_IMPROVE
/* Check device is attached */
static bool MGC_IsDevAttached(uint8_t *pBase)
{
    uint8_t devctl;

    MUSB_ASSERT(pBase);
    /* bConnectorId, bIsSession, bIsHost */
    devctl = MGC_Read8(pBase, MGC_O_HDRC_DEVCTL);

    if (devctl & (MGC_M_DEVCTL_FSDEV | MGC_M_DEVCTL_LSDEV))
    {
        return TRUE;
    }

    return FALSE;
}
#endif

/*-------------------------------------------------------------------------*/
static int MUC_start(struct usb_hcd *hcd)
{
    DBG(3, "[USB]hcd = 0x%lX.\n", (ulong)hcd);

    hcd->state = HC_STATE_RUNNING;

    return 0;
} 

/*-------------------------------------------------------------------------*/
static void MUC_stop(struct usb_hcd *hcd)
{
    DBG(3, "[USB]hcd = 0x%lX.\n", (ulong)hcd);
} 

/*-------------------------------------------------------------------------*/
static int MUC_hcd_probe(struct platform_device *pdev)
{
    uint32_t id = 0;
    int32_t retval;
    MUSB_LinuxController *pController;
    struct usb_hcd *hcd;
    MGC_LinuxCd *pThis;
    uint8_t *pBase;
    uint32_t u4Reg = 0;
    uint8_t intrmask = 0;
    uint8_t i;
    MGC_HsDmaController *pDmaController;
    struct fifo_node *node;

    DBG(3, "[USB]pdev = 0x%lX.\n", (ulong)pdev);
    if (!pdev)
    {
        return USB_ST_REMOVED;
    } 

    id = pdev->id;
    if (id >= MUCUST_GetControllerNum())
    {
        return USB_ST_REMOVED;
    }
    
    pController = MUCUST_GetControllerDev(id);    
    if (!pController)
    {
        return USB_ST_REMOVED;
    }
	switch(id)
	{
		case 0:
			pController->pBase=MUSB_BASE0;
			pController->pPhyBase=MUSB_PORT0_PHYBASE;
			pController->pFMBase = MUSB_PORT0_FMBASE;
			break;
		case 1:
			pController->pBase=MUSB_BASE1;
			pController->pPhyBase=MUSB_PORT1_PHYBASE;
			pController->pFMBase = MUSB_PORT1_FMBASE;
			break;
		case 2:
			pController->pBase=MUSB_BASE2;
			pController->pPhyBase=MUSB_PORT2_PHYBASE;
			pController->pFMBase = MUSB_PORT2_FMBASE;
			break;
		case 3:
			pController->pBase=MUSB_BASE3;
			pController->pPhyBase=MUSB_PORT3_PHYBASE;
			pController->pFMBase = MUSB_PORT3_FMBASE;
			break;
		default :
			INFO("register error \n");
			break;
	}

	INFO("[USB- %d]MAC register =%p PHY register=%p FM regsiter = %p\n",pdev->id,pController->pBase,pController->pPhyBase, pController->pFMBase);
    /* allocate and initialize hcd */
    hcd = usb_create_hcd(&MUC_hc_driver, &pdev->dev, dev_name(&pdev->dev));
    if (!hcd)
    {
        return USB_ST_NOMEMORY;
    }
	#ifdef MGC_PRE_CHECK_FREE_ENDPOINT
    //add for endpoint checking when device enumeration.
    hcd->check_free_ep = MGC_CheckFreeEndpoint;
	#endif
	#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))
	hcd->uses_new_polling = 1;
	#endif
	#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 36)
    hcd->self.uses_pio_for_control = 1; 
	#endif

    hcd->rsrc_start = (ulong)pController->pBase;
    pThis = hcd_to_musbstruct(hcd);
    pThis->bPortNum = id;
    pThis->pRegs = pController->pBase;
    pThis->pPhyBase = pController->pPhyBase;
    pThis->nIrq = pController->dwIrq;
    pThis->bSupportCmdQ = pController->bSupportCmdQ;    
    pThis->dev = &pdev->dev;

    init_timer(&pThis->events_timer);
	pThis->events_timer.function = usb_events_timer_func;
	pThis->events_timer.data = (unsigned long) pThis;	
	pThis->events = 0;

	init_timer(&pThis->events_timer1);
	pThis->events_timer1.function = usb_events_timer1_func;
	pThis->events_timer1.data = (unsigned long) pThis;	
	pThis->events_1 = 0;	
	
	pController->pThis = pThis;

    // turn off vbus.
    if (pController->MGC_pfVbusControl)
    {
        pController->MGC_pfVbusControl(pThis->bPortNum, 0);

        // wait about 4000ms after open session in function usb_events_timer_func, the total time is 800ms + 4000ms
        // Wait 5000 ms to turn on vbus.
		//mod_timer(&pThis->events_timer, jiffies	+ msecs_to_jiffies(5000));		
		//pThis->events = USB_EVENTS_VBUS_ON;
    }
    
    spin_lock_init(&pThis->SelectEndLock);
    spin_lock_init(&pThis->SelectHcdLock);	

    // check Phy reset is ok ?
    if ((!pController->MGC_pfPhyReset) ||
         (pController->MGC_pfPhyReset(pController) < 0))
    {
        return USB_ST_REMOVED;            
    }    

    pBase = (uint8_t*)pThis->pRegs;

    DBG(1, "Probe Tx Num[%d] Rx Num[%d]\n", 
         pController->bEndpoint_Tx_num, pController->bEndpoint_Rx_num);


    if(pThis->pPhyBase)
        {
        mod_timer(&pThis->events_timer, 
                  jiffies + msecs_to_jiffies(USB_HOST_SESSION_TIMEOUT));
        pThis->events |= USB_EVENTS_HOST_SESSION_ON;
    }

    // init endpoint, fifo.
    MGC_InitEnd(pThis, pController->bEndpoint_Tx_num, pController->bEndpoint_Rx_num);
    // init fifo free list
    node = kzalloc(sizeof(*node), GFP_ATOMIC); 
    if (node == NULL) {
      return USB_ST_NOMEMORY;
    }
    node->pAddr = MGC_END0_FIFOSIZE;
	//if (wEndAddr >= MGC_GET_TOTAL_FIFOSIZE(pThis))
    node->size = MGC_GET_TOTAL_FIFOSIZE(pThis);
    printk("Init Fifo Node addr = %x, size = %u\n", node->pAddr, node->size);
    node->next = NULL;
    pThis->fifo_list_head.next = node;

    MGC_Write16(pBase, MGC_O_HDRC_INTRTXE, pThis->wEndMask);

    /*
        IntrRxE, IntrUSB, and IntrUSBE are the same 32 bits group.
        Tricky: Set 0 in all write clear field in IntrUSB field. Prevent to clear IntrUSB.
    */
    u4Reg = MGC_M_INTR_SUSPEND | MGC_M_INTR_RESUME | MGC_M_INTR_BABBLE | 
    /* MGC_M_INTR_SOF | */
    MGC_M_INTR_CONNECT | MGC_M_INTR_DISCONNECT | MGC_M_INTR_SESSREQ | MGC_M_INTR_VBUSERROR;
    u4Reg = (u4Reg << 24) | (pThis->wEndMask &0xfffe);
    MGC_Write32(pBase, MGC_O_HDRC_INTRRXE, u4Reg);

	/*
	For some USB self-powered devices
	Move usb interrupt enable to MGC_VBusControl(x, 1) in mtk_cust.c
	It will be enabled after USB_EVENTS_VBUS_ON, to make sure FW is ready when connect interrupt occur

    	u4Reg = MGC_Read32(pBase, M_REG_INTRLEVEL1EN);
    	#ifdef CONFIG_USB_QMU_SUPPORT
    	u4Reg |= 0x2f; 
    	#else
    	u4Reg |= 0x0f;
    	#endif
    	MGC_Write32(pBase, M_REG_INTRLEVEL1EN, u4Reg);
	*/

    /* enable high-speed/low-power and start session */
    MGC_Write8(pBase, MGC_O_HDRC_POWER, MGC_M_POWER_SOFTCONN | MGC_M_POWER_HSENAB | MGC_M_POWER_SUSPENDM | MGC_M_POWER_ENSUSPEND);

    /* enable high-speed/low-power and start session & suspend IM host */
    MGC_Write8(pBase, MGC_O_HDRC_DEVCTL, MGC_M_DEVCTL_SESSION);

    // DMA controller init.
    pDmaController = &pThis->rDma;
    memset(pDmaController, 0, sizeof(MGC_HsDmaController));
    pDmaController->pThis = (void *)pThis;
	
    //reserve the last dma channel for command queue.
  	#ifdef SUPPORT_SHARE_USBQ_DMA
    pDmaController->bChannelCount = MUCUST_GetDMAChannelNum(id);
	#else
    pDmaController->bChannelCount = (pThis->bSupportCmdQ) ? 
        (MUCUST_GetDMAChannelNum(id)-1) : (MUCUST_GetDMAChannelNum(id));
	#endif
	
    for(i=0; i<pDmaController->bChannelCount; i++)
    {
        intrmask |= (1<<i); 
    }	
    MGC_Write8(pBase, MGC_O_HSDMA_INTR_UNMASK_SET, intrmask);
        
#ifdef CONFIG_USB_QMU_SUPPORT
    if (pThis->bSupportCmdQ)
    {
        QMU_init(pThis);
        //reserve the last dma channel for command queue.
        #ifdef SUPPORT_SHARE_USBQ_DMA
        QMU_select_dma_ch(pThis, (pDmaController->bChannelCount -1), 3);
		#else
		QMU_select_dma_ch(pThis, (pDmaController->bChannelCount), 3);
		#endif
    }
#endif  

    retval = usb_add_hcd(hcd, pController->dwIrq, IRQF_SHARED | IRQF_DISABLED);
    if (retval != 0)
    {
        return USB_ST_NOMEMORY;
    }

	#ifdef CONFIG_USB_DETECT_IMPROVE
    if(MGC_IsDevAttached(pBase))
    {
        uint8_t bIntrUsbValue = MGC_Read8(pBase, MGC_O_HDRC_INTRUSB);

        printk("[USB] device attached on init.\n");
        
        if ( !(bIntrUsbValue & (MGC_M_INTR_CONNECT)) )
        {
            //There is a device attached, but NO interrupt event occurred.
            //It should be handle device connection event.
            printk("[USB] handle device connect on init.\n");
            MGC_ServiceUSBIntr(pThis, MGC_M_INTR_CONNECT);            
        }
    }
	#endif
#ifdef CONFIG_USB_MT53xx_COMPLIANCE_TEST
    retval = mtk_debugfs_init(pController);
    if (retval != 0)
    {
        return USB_ST_NOMEMORY;
    }
#endif
    return 0;
}

/*-------------------------------------------------------------------------*/
static int MUC_hcd_remove(struct platform_device *pdev)
{
#ifdef CONFIG_USB_MT53xx_COMPLIANCE_TEST
    MUSB_LinuxController *pController;
#endif
    struct usb_hcd *hcd;
#ifdef CONFIG_USB_QMU_SUPPORT
    MGC_LinuxCd *pThis;
#endif

    DBG(3, "[USB]pdev = 0x%lX.\n", (ulong)pdev);

    hcd = platform_get_drvdata(pdev);
#ifdef CONFIG_USB_QMU_SUPPORT
    pThis = hcd_to_musbstruct(hcd);
    QMU_disable_all_q(pThis);
    QMU_clean(pThis);
#endif		  

#ifdef CONFIG_USB_MT53xx_COMPLIANCE_TEST
    pController = MUCUST_GetControllerDev(pThis->bPortNum);    
    mtk_debugfs_exit(pController);
#endif
    usb_remove_hcd(hcd);
#ifdef USB_NEW_URB_LIST
    //remove urblist buffer
    MGC_FreeEndpointListBuf(hcd,TRUE);
#endif
    usb_put_hcd(hcd);
   
    return 0;
} 

/*-------------------------------------------------------------------------*/
static int MUC_hcd_suspend(struct platform_device *pdev, pm_message_t state)
{
    DBG(3, "[USB]pdev = 0x%lX, state.event=%d.\n", (ulong)pdev, state.event);
    return 0;
} 

/*-------------------------------------------------------------------------*/
static int MUC_hcd_resume(struct platform_device *pdev)
{
    DBG(3, "[USB]pdev = 0x%lX.\n", (ulong)pdev);
    return 0;
} 

/*-------------------------------------------------------------------------*/
static void MUC_hcd_release(struct device *dev)
{
    DBG(3, "[USB]dev = 0x%lX.\n", (ulong)dev);
} 

#if defined(CONFIG_ARCH_MT5396) && (defined(CONFIG_MT53XX_MAP_CHANNELB_DRAM) || defined(CONFIG_MT53XX_USE_CHANNELB_DRAM))
#define MTK_HCD_DMA_MASK (mt53xx_cha_mem_size-1)
#else
#define MTK_HCD_DMA_MASK 0xffffffff
#endif

/*-------------------------------------------------------------------------*/
static int __init MUC_init(void)
{
    int32_t retval = 0;
    uint32_t nCount;
    uint32_t i;

    struct platform_device *pPlatformDev;
#ifdef CONFIG_OF
    struct device_node *np0;
    np0=of_find_compatible_node(NULL,NULL,"Mediatek, TVUSB20");
    pUSBphyIoMap=of_iomap(np0,0);
    pUSBmacIoMap=of_iomap(np0,1);
    pUSBdmaIoMap=of_iomap(np0,2);
    pUSBpdwncIoMap = of_iomap(np0, 3);
    pUSBfmIoMap1 = of_iomap(np0, 4);
    pUSBfmIoMap2 = of_iomap(np0, 5);
#else
    pUSBphyIoMap=ioremap(MUSB_PHY_BASE,0x800);
    pUSBmacIoMap=ioremap(MUSB_MAC_BASE,0x4000);
    pUSBfmIoMap1=ioremap(MUSB_FM_BASE1,0x20);
    pUSBfmIoMap2=ioremap(MUSB_FM_BASE2,0x20);
    pUSBdmaIoMap=ioremap(MUSB_DMA_SET,0x10);
    pUSBpdwncIoMap = ioremap(PDWNC_PHY, 0x100);
#endif
    INFO("[USB ]MAC register =0x%p PHY register=0x%p DMA setting  register=0x%p\n",pUSBmacIoMap,pUSBphyIoMap, pUSBdmaIoMap);

    if (usb_disabled())
    {
        return USB_ST_REMOVED;
    } 

#if defined(CONFIG_ARCH_MT5882)
	if(IS_IC_MT5885())
	{
		INFO("mgc MT5885 controller info reinitial \n");
		MUC_aLinuxController[1].bEndpoint_Rx_num = 16;
		MUC_aLinuxController[1].bEndpoint_Tx_num = 16;
	}
#endif

	INFO("USB Port[%d] ADB enable = %d\n",ssusb_adb_port,ssusb_adb_flag);

    if(ssusb_adb_port > MUCUST_GetControllerNum())
        ssusb_adb_port = 0;

    printk("======== MUC_init USB port Common USB Driver*************=====\n");
    retval = platform_driver_register(&MUC_hcd_driver);
    if (retval < 0)
    {
        return retval;
    }

#ifdef MUC_DMA_DISABLE
    INFO("[USB]Controller in CPU mode\n");
#else
    INFO("[USB]Controller in DMA mode\n");
#endif
    nCount = MUCUST_GetControllerNum();
    for (i = 0; i < nCount; i++)
    {
#ifdef CONFIG_MT53XX_NATIVE_GPIO
#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5891)
			  /* For Oryx 5890 and Wukong 5891, no USB2.0 Port2, but some company 
			   * still config Port2 as USB2.0 Port, so mark Port2 for USB2.0 here
			   * */
			  if(2 != i && (MUC_aPortUsing[i] == 1) 
#else
			  if((MUC_aPortUsing[i] == 1) 
#endif
			//#ifdef MUSB_GADGET_PORT_NUM
			#if 1
			&& (!((ssusb_adb_flag == 1) && (i == ssusb_adb_port)))		// If ADB port, no need to initial host mode
#endif
			)
		#endif
	    {
	        pPlatformDev = MUCUST_GetPlatformDev(i);
	        pPlatformDev->name = MUC_HcdName;
	        pPlatformDev->id = i;
	        DBG(3, "[USB]pdev = 0x%lX, id = %d.\n", (ulong)pPlatformDev, i);
	        //  This controller has DMA capability.        
			#ifdef MUC_DMA_DISABLE
	        pPlatformDev->dev.dma_mask = 0;
	        pPlatformDev->dev.coherent_dma_mask = 0;
			#else
	        pPlatformDev->dev.coherent_dma_mask = MTK_HCD_DMA_MASK;
	        pPlatformDev->dev.dma_mask = &pPlatformDev->dev.coherent_dma_mask;
			#endif
	        pPlatformDev->dev.release = MUC_hcd_release;
			#if defined(CONFIG_PM) || defined(CC_SUPPORT_STR) || defined(CONFIG_MUSB_PM)
			pPlatformDev->dev.type = &usb_host_type;
			#endif
	        retval = platform_device_register(pPlatformDev);
	        if (retval < 0)
	        {
	            platform_device_unregister(pPlatformDev);
	            break;
	        } 
	    }
    }

    if (retval < 0)
    {
        platform_driver_unregister(&MUC_hcd_driver);
    }

#ifdef USB_READ_WRITE_TEST
    // init /proc/usb
    usb_init_procfs();
#endif

	MUT_init();

#if defined(CONFIG_PM) || defined(CC_SUPPORT_STR) 	
	register_pm_notifier(&mtk_usb_pm_notifier);
#endif

    return retval;
}

/*-------------------------------------------------------------------------*/
static void __exit MUC_cleanup(void)
{
    uint32_t nCount;
    uint32_t i;
    struct platform_device *pPlatformDev;

    nCount = MUCUST_GetControllerNum();
    for (i = 0; i < nCount; i++)
    {

		#ifdef CONFIG_MT53XX_NATIVE_GPIO
			if((MUC_aPortUsing[i] == 1) 
			//#ifdef MUSB_GADGET_PORT_NUM
			#if 1
			&& (!((ssusb_adb_flag == 1) && (i == ssusb_adb_port)))
			#endif
			)
		#endif
	    {
	        pPlatformDev = MUCUST_GetPlatformDev(i);
	        DBG(3, "[USB]pdev = 0x%lX, id = %d.\n", (ulong)pPlatformDev, i);
	        platform_device_unregister(pPlatformDev);
		}
    } 

    platform_driver_unregister(&MUC_hcd_driver);

	MUT_exit();

#if defined(CONFIG_PM) || defined(CC_SUPPORT_STR) 	
	unregister_pm_notifier(&mtk_usb_pm_notifier);
#endif

    return ;
}

/*-------------------------------------------------------------------------*/
module_init(MUC_init);
module_exit(MUC_cleanup);
