/*-----------------------------------------------------------------------------
 *\drivers\usb\host\mtk_quirks.c
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


/** @file mtk_quirks.c
 *  This C file implements the mtk53xx USB host controller driver.
 */
//---------------------------------------------------------------------------
// Include files
//---------------------------------------------------------------------------
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ioport.h>
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

#include "mtk_hcd.h"
#include "mtk_qmu_api.h"

#ifdef MUSB_DEBUG
extern int mgc_debuglevel;
#endif
extern int mgc_epskipchk;
extern int mgc_qmudisable;
extern int mgc_qmuenable;


/* Quirks: various devices quirks are handled by this table & its flags. */

struct quirk_device_struct {
	uint16_t vendorId;
	uint16_t productId;
	unsigned int device_info;
};

static const struct quirk_device_struct device_quirk_list[] = {
	//{ 0x046D, 0x082B, USB_QUIRK_QMU_DISABLE }, /* Camera Logtech C170, rx length error when video and audio transfer at the same time */
	{ 0x043E, 0x3009, USB_QUIRK_QMU_DISABLE }, /* Camera lg V400 */
    { 0x1460, 0x7290, USB_QUIRK_QMU_DISABLE }, /* Camera MAX */
	{ 0x18EC, 0x3299, USB_QUIRK_QMU_DISABLE }, /* Camera BlueLover */
	{ 0x1D27, 0x0601, USB_QUIRK_3D_CAMERA_WORK}, /* PrimeSense 3D Camera */
	{ 0, 0 , 0}
};

/*
 * Detcte any quirks the device has, and do any special setting for it if needed.
 */
static unsigned int usb_detect_quirks(uint16_t vendor, uint16_t product)
{
	int i;
	unsigned int quirks = 0;

	for (i = 0; device_quirk_list[i].device_info; i++) {
		if (vendor == device_quirk_list[i].vendorId &&
		    product == device_quirk_list[i].productId)
			{
				quirks |= device_quirk_list[i].device_info;
			}
	}
	return quirks;
}

unsigned int MGC_DeviceDetectQuirks(struct urb *pUrb)
{
    struct usb_device *dev = pUrb->dev;
    struct usb_device_descriptor *pDescriptor = &dev->descriptor;
	uint16_t vendorid, productid;
	vendorid = pDescriptor->idVendor;
	productid = pDescriptor->idProduct;

	return usb_detect_quirks(vendorid, productid);

}

int MGC_WIFIHotbootSupported(struct urb *pUrb)
{
    struct usb_device *dev = pUrb->dev;
    struct usb_device_descriptor *pDescriptor = &dev->descriptor;
	if((pDescriptor->idProduct==0x7662)||(pDescriptor->idProduct==0x7632) 
		||(pDescriptor->idProduct==0x7612)||(pDescriptor->idProduct==0x76a0))
		return TRUE;
	else
		return FALSE;

}
#if CONFIG_USB_QMU_SUPPORT
int MGC_DeviceQmuSupported(struct urb *pUrb)
{
    struct usb_device *dev = pUrb->dev;
    struct usb_device_descriptor *pDescriptor = &dev->descriptor;
	uint16_t vendorid, productid;
	
	unsigned int nOut = usb_pipeout(pUrb->pipe);
	uint16_t wPacketSize = usb_maxpacket(pUrb->dev, pUrb->pipe, nOut);

	vendorid = pDescriptor->idVendor;
	productid = pDescriptor->idProduct;
	
	/*Test parameter to enable all device can support qmu to debug */
	if(mgc_qmuenable)
		return TRUE;

	if(mgc_qmudisable || 
		(usb_detect_quirks(vendorid, productid) & USB_QUIRK_QMU_DISABLE))
	{
		/* This Device can not support USB QMU */
		return FALSE;
	}

	/*20150615 Leon Logitech C170 mic in will caused USBQ Length error 
	 * because HW DMA request is not sendout quickly, 
	 * C170 will sendout MaxPacketSz*n length data > MaxPacketSZ,
	 * and cause USBQ detect Length error and stop USBQ
       * Solution: Disable USBQ support for Logitech C170 Audio in.
	 */
	if((0x046D == vendorid) && (0x082B == productid))
	{
        if(wPacketSize < 512)
			return FALSE;
	}
	
	return TRUE;
}
#endif

#ifdef MGC_PRE_CHECK_FREE_ENDPOINT
static int MGC_SkipIntfSettingChk(struct usb_device *dev)
{
	return FALSE;
}
static int MGC_SkipEpChk(struct usb_device *dev)
{
    struct usb_device_descriptor *pDescriptor = &dev->descriptor;
	
#ifndef SUPPORT_SHARE_MGC_END_MSD
	struct usb_interface_descriptor *d;
	d = &dev->actconfig->interface[0]->cur_altsetting->desc;
#endif

  	if(mgc_epskipchk)
	{
        DBG(-2,"Skip EpCheck: mgc_epskipchk is true.\n");
        return TRUE;
	}
	
	if(pDescriptor->idVendor == 0x1d6b &&
	   pDescriptor->idProduct == 0x0002 &&
	   pDescriptor->bDeviceClass == 0x09)
	{
        DBG(-2,"Skip EpCheck: MTK Root Hub.\n");
        return TRUE;
	}
#ifndef SUPPORT_SHARE_MGC_END_MSD //move to MGC_CheckFreeEndpoint()
	if( (pDescriptor->bDeviceClass == USB_CLASS_MASS_STORAGE) || 
             (d->bInterfaceClass == USB_CLASS_MASS_STORAGE) )
	{
		DBG(-2,"Skip EpCheck: A Mass Storage Device.\n");
		return TRUE;
	}
#endif
	/*20121009 WIFI Dongle skip endpoint check */
	if(pDescriptor->idVendor == 0x148F ||           /*Ralink Technology, Corp.*/
	   	pDescriptor->idVendor == 0x0E8D||           /*Mediatek, Corp.*/
	    	pDescriptor->idVendor == 0x0CF3 ||			/*Atheros Communications, Inc.*/
		 pDescriptor->idVendor == 0x0bda)			/*Realtek Semiconductor Corp.*/
	{
		DBG(-2, "Skip EpCheck: WIFI Dongle.\n");
		return TRUE;
	}
	/*20121009 Ralink WIFI Dongle has 6 Tx,1 Rx.
	 *The wifi dongle cann't enumerate complete because host not enough tx endpoint.
	 * skip endpoint check step
	 */	
    if(dev->manufacturer==NULL || dev->product==NULL)
    {
        DBG(-2, "[USB]manufacturer OR product is NULL.\n");
        return FALSE;
    }
        
    if(strcmp("Ralink",dev->manufacturer) == 0
    	  && strcmp("11n Adapter",dev->product) == 0)
    {
       DBG(-2, "[USB]Ralink 11n Adapter skip ep checking.\n");
       return TRUE;
    }

    if(strcmp("Ralink",dev->manufacturer) == 0
    	  && strcmp("802.11 n WLAN",dev->product) == 0)
    {
       DBG(-2, "[USB]Ralink 802.11 n WLAN Adapter skip ep checking.\n");
       return TRUE;
    }

   if(0x0421 == pDescriptor->idVendor && 0x051a == pDescriptor->idProduct){
       printk("[usb]NOKIA N9, skip ep checking.\n");
       return TRUE;
   }

    return FALSE;
}

int MGC_CheckFreeEndpoint(struct usb_device *dev, int configuration)
{
    MGC_LinuxCd *pThis = NULL;
    MGC_LinuxLocalEnd *pEnd = NULL;
	struct usb_host_config *config = NULL;
    int aIsLocalEndOccupy[2][MUSB_C_MAX_NUM_EPS]; /*0: Rx, 1: Tx*/
	int nintf = 0;
	int endfound[2] = {1, 1}; 
    int nEnd = -1;
	int i = 0;
#ifdef SUPPORT_SHARE_MGC_END_MSD
	struct usb_interface_descriptor *d;
    struct usb_device_descriptor *pDescriptor = &dev->descriptor;
#endif
	
	pThis = hcd_to_musbstruct(bus_to_hcd(dev->bus));
	DBG(-2, "configNum = %d, select configuration = 0x%x.\n",dev->descriptor.bNumConfigurations, configuration);
	
	if (dev->authorized == 0 || configuration == -1)
	{
		configuration = 0;
	}
	else 
	{
		for (i = 0; i < dev->descriptor.bNumConfigurations; i++) 
		{
			if (dev->config[i].desc.bConfigurationValue == configuration) 
			{
				config = &dev->config[i];
				break;
			}
		}
	}
	
	if ((!config && configuration != 0))
		return -EINVAL;

	/* Special Device can't do check enpoint function when enumerate */
	if(MGC_SkipEpChk(dev))
	{
        return 0;
	}
	
#ifdef SUPPORT_SHARE_MGC_END_MSD
	d = &dev->actconfig->interface[0]->cur_altsetting->desc;

	/* use a reserved one for bulk if any */
	//if (usb_pipebulk(pUrb->pipe))
	{		 
		if ( (pDescriptor->bDeviceClass == USB_CLASS_MASS_STORAGE) || 
			 (d->bInterfaceClass == USB_CLASS_MASS_STORAGE) )
		{
			if(pThis->bMsdEndTxNum!=0xff) // MSD rx/tx be config at the same time, 0xff means do not config
				return 0;
		}
	}
#endif

	if(dev->parent)
	{
	    DBG(-2,"[USB:parent]level = 0x%04X,idVendor = 0x%04X,idProduct = 0x%04X,bDeviceClass = 0x%04X\n", 
						dev->parent->level,dev->parent->descriptor.idVendor, 
						dev->parent->descriptor.idProduct, dev->parent->descriptor.bDeviceClass);
	}
	
	/* The USB spec says configuration 0 means unconfigured.
	 * But if a device includes a configuration numbered 0,
	 * we will accept it as a correctly configured state.
	 * Use -1 if you really want to unconfigure the device.
	 */
	if (config && configuration == 0)
		dev_warn(&dev->dev, "config 0 descriptor??\n");

    if(config)
    {
		/*0: Rx, 1: Tx*/			
		for (nEnd = 1; nEnd < pThis->bEndTxCount; nEnd++)
		{
            pEnd = &(pThis->aLocalEnd[1][nEnd]); 
            aIsLocalEndOccupy[1][nEnd] = pEnd->bIsOccupy;
		}
		
        for (nEnd = 1; nEnd < pThis->bEndRxCount; nEnd++)
        {               
            pEnd = &(pThis->aLocalEnd[0][nEnd]);
            aIsLocalEndOccupy[0][nEnd] = pEnd->bIsOccupy;
            DBG(-2, "[USB1] [%d: RX=%d TX= %d]\n", 
                nEnd, aIsLocalEndOccupy[0][nEnd], aIsLocalEndOccupy[1][nEnd]);
    	}
		
        nintf = config->desc.bNumInterfaces;
    	for (i = 0; i < nintf; ++i) 
    	{
    		struct usb_interface *intf = config->interface[i];
        	struct usb_host_interface *iface_desc;
        	struct usb_endpoint_descriptor *ep_desc;
            struct usb_host_endpoint *ep = NULL;
			struct usb_interface_cache *intfc = config->intf_cache[i];
			struct usb_host_interface *alt;
			
			int infaceMaxEndponits = 0;
        	int j = 0, cur_setting = 0;
        	int found = 0, ep_end = 0;
        	int epnum = 0, is_out = 0;

        	iface_desc = intf->cur_altsetting;
			infaceMaxEndponits = iface_desc->desc.bNumEndpoints;
			
			DBG(-2, "[USB1:intf][%d]altsettingNum = %d\n", i, intfc->num_altsetting);

			// check the whole interface setting and select the setting that need maximu endpoint.
			if(!MGC_SkipIntfSettingChk(dev))
			{
				for ((j = 0, alt = &intfc->altsetting[0]);
					  j < intfc->num_altsetting;
					 (++j, ++alt)) 
			 	{
					if(alt->desc.bNumEndpoints > infaceMaxEndponits)
					{
						infaceMaxEndponits = alt->desc.bNumEndpoints;
						iface_desc = alt;
						cur_setting = j;
					}
	    		}
			}
				 
			 DBG(-2, "[USB2:intf][%d]intfNmu = %d, cur_altsetting = %d, bNumEndpoints = 0x%X, bInterfaceClass = 0x%X\n", 
					 i, nintf, cur_setting, infaceMaxEndponits, iface_desc->desc.bInterfaceClass);
			 
        	for (j = 0; j < infaceMaxEndponits; ++j) 
        	{
        		ep_desc = &iface_desc->endpoint[j].desc;
        	    epnum = usb_endpoint_num(ep_desc);
        	    is_out = usb_endpoint_dir_out(ep_desc);
                found = 0;
				
        		if(!endfound[is_out])
    			{
  					goto ep_not_enough;
    			}

        		DBG(-2, "[USB %d] Type: 0x%02X, Addr: 0x%02X, Attr: 0x%02X, Intr: 0x%02X, MaxP: 0x%04X\n", 
        						epnum, ep_desc->bDescriptorType, ep_desc->bEndpointAddress, 
    							ep_desc->bmAttributes, ep_desc->bInterval, ep_desc->wMaxPacketSize);

				ep = is_out ? dev->ep_out[epnum] : dev->ep_in[epnum];
            	
            	if(ep && ep->hcpriv)
            	{
                    pEnd = (MGC_LinuxLocalEnd *)ep->hcpriv;            	
                    found = 1;
                    continue;
            	}
				DBG(-2, "[USB]%s Ep Dev_leve = %d, intfClass = 0x%x.\n",
							((is_out) ? "Tx" : "Rx"), dev->level,iface_desc->desc.bInterfaceClass);  

                ep_end = is_out ? pThis->bEndTxCount : pThis->bEndRxCount; 
				
                for (nEnd = 1; nEnd < ep_end; nEnd++)
                {  
                    if(!aIsLocalEndOccupy[is_out][nEnd])
                	{
						// Occupy this endpoint.
                        aIsLocalEndOccupy[is_out][nEnd] = TRUE;
						found = 1;
						break;
                	}                   
                }
				
 				ep_not_enough:
                if(!found)
                {
                	endfound[is_out] = 0;
                    INFO("[USB] !!!!!!!!WARNING: Endpoint not enough! %s Ep\n", ((is_out) ? "Tx" : "Rx"));
                    //Set Device Base Class as a miscellaneous
                    //Set InterfaceClass & InterfaceSubClass to 0xEE as an 
                    //unsupport device.
                    if(iface_desc)
                    {
                        ep_desc->bEndpointAddress = 0xEE;
                        iface_desc->desc.bInterfaceClass = 0xEE;
                        iface_desc->desc.bInterfaceSubClass = 0xEE;                    
                    }
                }
        	}
    	}
    }

    for (nEnd = 1; nEnd < pThis->bEndRxCount; nEnd++)
    {
        DBG(-2, "[USB_EP_End] [%d: RX=%d TX= %d]\n", 
                nEnd, aIsLocalEndOccupy[0][nEnd], aIsLocalEndOccupy[1][nEnd]);
    }
    return 0;
}
#endif

#ifdef CONFIG_DEVICE_IDENTIFY_SUPPORT
int MGC_IsUvcDevice(struct usb_device_descriptor *pDescriptor, struct usb_device *pDev)
{
    struct usb_host_config *config = NULL;
    struct usb_interface_assoc_descriptor *iad = NULL;
    int i = 0;
    
    if(!pDescriptor || !pDev)
    {
        return FALSE;
    }
    
	if(pDescriptor->bDeviceClass == 0xEF && //Multi-interface Function Code Device
	   pDescriptor->bDeviceSubClass == 0x02 &&  //This is the Common Class Sub Class
	   pDescriptor->bDeviceProtocol == 0x01) //This is the Interface Association Descriptor protocol
	{
	    return TRUE;
    }

	if(pDev->config)
	{
	    config = pDev->config;

	    if(config)
	    {
        	for (i = 0; i < USB_MAXIADS; i++) 
        	{
        	    iad = config->intf_assoc[i];
        		if (iad == NULL)
        			break;


        	    if((iad->bFunctionClass == 0x0E || iad->bFunctionClass == 0xFF) &&
                   iad->bFunctionSubClass == 0x03)
        		{
        		    printk("IAD Video Interface Class\n");
                    return TRUE;
        	    }
        	}
	    }

	}

    return FALSE;
}

int MGC_IsUacDevice(struct usb_device_descriptor *pDescriptor, 
                                              struct usb_device *pDev)
{
    struct usb_host_config *c = NULL;
    int i = 0;
    
    if(!pDescriptor || !pDev)
    {
        return FALSE;
    }
    
    if(pDev->actconfig)
    {
        c = pDev->actconfig;

	if(c)
	{
	    for(; i < c->desc.bNumInterfaces; i ++)
	    {
	        if(c->intf_cache[i]->altsetting->desc.bInterfaceClass == USB_CLASS_AUDIO)
	        {
	            return TRUE;
	        }
	    }
	}
    }
    return FALSE;
}

int MGC_IsMFIDevice(struct usb_device_descriptor *pDescriptor, struct usb_device *pDev)
{
    if(!MGC_IsUacDevice(pDescriptor,pDev))
    {
        return FALSE;
    }
    
    if(0x05AC == pDescriptor->idVendor &&  0x12 == (pDescriptor->idProduct >> 8))
    {
        return TRUE;
    }
    
    return FALSE;   
}

#endif
