/**************************************************************************
 * Name         : dc_cobra_linux.c
 * Author       : Billy Park
 * Created      : 01/10/2012
 *
 * Copyright    : 2016 by Mediatek Limited. All rights reserved.
 *
 *
 * Platform     : cobra
 *
 * $Date: 2015/12/22 $ $Revision: #1 $
 * $Log: dc_cobra_linux.c $
 **************************************************************************/

/**************************************************************************
 The 3rd party driver is a specification of an API to integrate the
 IMG PowerVR Services driver with 3rd Party display hardware.
 It is NOT a specification for a display controller driver, rather a
 specification to extend the API for a pre-existing driver for the display hardware.

 The 3rd party driver interface provides IMG PowerVR client drivers (e.g. PVR2D)
 with an API abstraction of the system's underlying display hardware, allowing
 the client drivers to indirectly control the display hardware and access its
 associated memory.

 Functions of the API include

  - query primary surface attributes (width, height, stride, pixel format,
      CPU physical and virtual address)
  - swap/flip chain creation and subsequent query of surface attributes
  - asynchronous display surface flipping, taking account of asynchronous
      read (flip) and write (render) operations to the display surface

 Note: having queried surface attributes the client drivers are able to map
 the display memory to any IMG PowerVR Services device by calling
 PVRSRVMapDeviceClassMemory with the display surface handle.

 This code is intended to be an example of how a pre-existing display driver
 may be extended to support the 3rd Party Display interface to
 PowerVR Services – IMG is not providing a display driver implementation
 **************************************************************************/

#include <linux/version.h>

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,38))
  #ifndef AUTOCONF_INCLUDED
#include <linux/config.h>
  #endif
#endif

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/mutex.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#if defined(SUPPORT_DRI_DRM)
#include <drm/drmP.h>
#endif

#include "img_defs.h"
#include "servicesext.h"
#include "kerneldisplay.h"
#include "pvrmodule.h"

#include "dc_cobra_debug.h"
#include "dc_cobra_displayclass.h"
#include "dc_cobra_osk.h"
#include "dc_cobra.h"

#if SUPPORT_PVRCOBRA_OWN_LINUX_FB
#include <linux/fb.h>
#endif

#if defined(SUPPORT_DRI_DRM)
#include "pvr_drm.h"
#endif

#define unref__ __attribute__ ((unused))
#define	MAKESTRING(x) # x

#if !defined(DISPLAY_CONTROLLER)
#define DISPLAY_CONTROLLER dc_cobra
#endif

#define	DRVNAME	"dc_cobra"
#define DEVNAME DRVNAME

DC_ERROR OSRegisterDevice(PVRCOBRA_DEVINFO *psDevInfo);
DC_ERROR OSUnregisterDevice(PVRCOBRA_DEVINFO *psDevInfo);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36))
static DEFINE_MUTEX(bridge_mutex);
static long bc_bridge_unlocked(struct file * file, unsigned int cmd, unsigned long arg);
#else
static int bc_bridge(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg);
#endif

int img_debug_level = 6;
module_param(img_debug_level, int, S_IRUSR | S_IWUSR | S_IWGRP | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(img_debug_level, "Higher number, more dmesg output");

int img_major = 0;
module_param(img_major, int, S_IRUGO);
MODULE_PARM_DESC(img_major, "Device major number");

/* internal variables */
struct img_dev
{
	struct cdev cdev;
};
static struct img_dev device;
static char img_dev_name[] = DEVNAME;

static struct file_operations img_dev_fops = {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36))
	.unlocked_ioctl = bc_bridge_unlocked
#else
	.ioctl = bc_bridge
#endif
};

static int bc_bridge(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
	int err = -EFAULT;
	int command = _IOC_NR(cmd);

	switch(command)
	{
	default:
		return err;
	}
	return 0;	
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36))
static long bc_bridge_unlocked(struct file *file, unsigned int cmd, unsigned long arg)
{
	int res;

	mutex_lock(&bridge_mutex);
	res = bc_bridge(NULL, file, cmd, arg);
	mutex_unlock(&bridge_mutex);
	
	return res;
}
#endif

#if !SUPPORT_PVRCOBRA_OWN_LINUX_FB
DC_ERROR OSRegisterDevice(PVRCOBRA_DEVINFO *psDevInfo)
{
	return (DC_OK);
}

DC_ERROR OSUnregisterDevice(PVRCOBRA_DEVINFO *psDevInfo)
{
	return (DC_OK);
}

#else

/*****************************************************************************
 Function Name:	PVRPSBPanDisplay
 Description  :	Pans the display.

*****************************************************************************/
static int PVRCobraPanDisplay(struct fb_var_screeninfo *psScreenInfo,
                            struct fb_info *psFBInfo)
{
	//PVRCOBRA_DEVINFO *psDevInfo = (PVRCOBRA_DEVINFO *)psFBInfo->par;
	//IMG_INT32 ui32Offset = 0;
		
    if (psScreenInfo->xoffset != 0)
        return -EINVAL;

    if (psScreenInfo->yoffset + psScreenInfo->yres > psScreenInfo->yres_virtual)
        return -EINVAL;

#if 0
    ui32Offset = psScreenInfo->yoffset * psFBInfo->fix.line_length;
	WriteMMIOReg(psDevInfo, PSB_REG(PVRPSB_DSPLINOFF, gbUsingLvds ?
				PSBREG_PIPEB_OFFSET : PSBREG_PIPEA_OFFSET), ui32Offset);
#endif

	IMG_DEBUG_PRINT(7, ("%s", __FUNCTION__));
    return 0;
}

/*****************************************************************************
 Function Name:	PVRPSBSetColReg
 Description  :	For truecolor, sets a pseudo palette, which translates 16 text 
 				console colors into actual pixel values. 
				Pseudocolor etc. are not supportted.
*****************************************************************************/
static int PVRCobraSetColReg(unsigned int regno,
							 unsigned int red,
							 unsigned int green,
							 unsigned int blue,
							 unsigned int transp,
							 struct fb_info *psFBInfo)
{
	if (psFBInfo->fix.visual == FB_VISUAL_TRUECOLOR) {
		unsigned int tmp;

		if (regno > 16)
			return -EINVAL;

		switch (psFBInfo->var.bits_per_pixel)
		{
			case 16:
				tmp = (red & 0xf800) | ((green & 0xfc00) >> 5) | 
						((blue & 0xf800) >> 11);
				break;
			case 24:
				tmp = ((red & 0xff00) << 8) | (green & 0xff00) | 
						((blue & 0xff00) >> 8);
				break;
			case 32:
				tmp = ((transp & 0xff00) << 16) | ((red & 0xff00) << 8) | 
					((green & 0xff00)) | ((blue & 0xff00) >> 8);
				break;
			default:
				return -EINVAL;
		}
		((unsigned int *)psFBInfo->pseudo_palette)[regno] = tmp;
		return 0;
	}
	return 1;
}

/*****************************************************************************
 Function Name:	PVRCobraBlank
 Description  :	Blanks the display.

*****************************************************************************/
static int PVRCobraBlank(int iBlanMode, struct fb_info *psFBInfo)
{
	/* 
	 * TODO: enable DPLL, PIPE, PLANE and disable them in the reverse order 
	 * when requested.
	 */
	return 0;
}

static struct fb_ops sPVRCobraFbOps = {
    .owner          = THIS_MODULE,
    .fb_pan_display = PVRCobraPanDisplay,
    .fb_setcolreg   = PVRCobraSetColReg,
    .fb_blank       = PVRCobraBlank,
    .fb_fillrect    = cfb_fillrect,
    .fb_copyarea    = cfb_copyarea,
    .fb_imageblit   = cfb_imageblit,
};

static void *MapPhysAddr(IMG_SYS_PHYADDR sSysAddr, IMG_UINT32 uiSize)
{
	void *pvAddr = ioremap(sSysAddr.uiAddr, uiSize);
	return pvAddr;
}

static void UnMapPhysAddr(void *pvAddr, IMG_UINT32 uiSize)
{
	iounmap(pvAddr);
}

/*****************************************************************************
 Function Name:	OSRegisterDevice
 Description  :	Registers the device
*****************************************************************************/
DC_ERROR OSRegisterDevice(PVRCOBRA_DEVINFO *psDevInfo)
{
	struct fb_info *psFBInfo;

	psFBInfo = framebuffer_alloc(sizeof(PVRCOBRA_DEVINFO *), 0);
	psFBInfo->par = psDevInfo;

	if (!psFBInfo)
	{
		printk(KERN_ERR DRVNAME ": OSRegisterDevice: Unable to alloc frame buffer.\n");
		return (DC_ERROR_OUT_OF_MEMORY);
	}

	/* 
	 * 'type' field tells what the video memory to pixel mapping is. 
	 * 'FB_TYPE_PACKED_PIXELS' means a direct mapping from video memory to a pixel value. 
	 * This is required when you want to map the video memory into user land.
	 *
	 * FB_TYPE_PACKED_PIXELS: Packed Pixels
	 * FB_TYPE_TEXT: 		  Text/attributes		
	 * FB_TYPE_VGA_PLANES: 	  EGA/VGA planes
	 */
	psFBInfo->fix.type = FB_TYPE_PACKED_PIXELS;
	psFBInfo->fix.type_aux = 0;
	/* 
	 * 'visual' field tells how to map a pixel value to the colors. 
	 *
	 * FB_VISUAL_TRUECOLOR:   True color
     * FB_VISUAL_PSEUDOCOLOR: Pseudo color
     * FB_VISUAL_DIRECTCOLOR: Direct color
	 */
	psFBInfo->fix.visual = FB_VISUAL_TRUECOLOR;

	/*
	 * Display panning: the visible screen is just a window of the video 
	 * memory, console strolling is done by changing the start of the window.
	 * Strolling is fast, because there is no need to copy around data.
	 * To get the console strolling work, 
	 */
	psFBInfo->fix.xpanstep = 0; 
	psFBInfo->fix.ypanstep = 4;

	psFBInfo->fix.ywrapstep = 0;
	psFBInfo->fix.accel = FB_ACCEL_NONE;
	psFBInfo->fix.line_length = psDevInfo->sFBInfo.uiByteStride;

	psFBInfo->var.nonstd = 0;  // != 0 Non standard pixel format 
	psFBInfo->var.height = -1; // height of picture in mm 
	psFBInfo->var.width = -1;  // width of picture in mm
	psFBInfo->var.vmode = FB_VMODE_NONINTERLACED;

	psFBInfo->var.transp.offset = 0;
	psFBInfo->var.transp.length = 0;
	psFBInfo->var.red.offset = 16;
	psFBInfo->var.green.offset = 8;
	psFBInfo->var.blue.offset = 0;
	psFBInfo->var.red.length = 8;
	psFBInfo->var.green.length = 8;
	psFBInfo->var.blue.length = 8;

	psFBInfo->var.bits_per_pixel = 32;

	psFBInfo->fbops = &sPVRCobraFbOps;

	// Turn display panning off for now. 
	// The accelarated strolling does not work well with NoWS.
	psFBInfo->flags = FBINFO_DEFAULT;

	psFBInfo->pseudo_palette = kmalloc(16 * sizeof(IMG_UINT32), GFP_KERNEL);
	if (!psFBInfo->pseudo_palette)
	{
		printk(KERN_ERR DRVNAME ": OSRegisterDevice: Unable to alloc pseudo palette.\n");
		return (DC_ERROR_OUT_OF_MEMORY);
	}

	strncpy(&psFBInfo->fix.id[0], DRVNAME, sizeof(psFBInfo->fix.id));

	psFBInfo->fix.mmio_start = (IMG_UINT32) psDevInfo->sFBInfo.sRegSysAddr.uiAddr;
	psFBInfo->fix.mmio_len = 4096;

	psFBInfo->fix.smem_start = (unsigned long) psDevInfo->sFBInfo.sSysAddr.uiAddr;
	psFBInfo->fix.smem_len = psDevInfo->sFBInfo.uiByteStride * 
							 psDevInfo->sFBInfo.uiHeight * 
							 (psDevInfo->uiMaxBackBuffers + 1);

    psFBInfo->screen_base = (char __iomem *) MapPhysAddr(psDevInfo->sFBInfo.sSysAddr, psFBInfo->fix.smem_len);

    if (!psFBInfo->screen_base)
        goto err_map_video;

	psFBInfo->var.xres = psDevInfo->sFBInfo.uiWidth;
	psFBInfo->var.yres = psDevInfo->sFBInfo.uiHeight;
	psFBInfo->var.xres_virtual = psDevInfo->sFBInfo.uiWidth;
	psFBInfo->var.yres_virtual =(u32)(psFBInfo->fix.smem_len / psFBInfo->fix.line_length);

	if (fb_alloc_cmap(&psFBInfo->cmap, 256, 0) < 0)
	{
		printk(KERN_ERR DRVNAME ": OSRegisterDevice: Unable to alloc cmap.\n");
		goto err_alloc_cmap;
	}

	if(register_framebuffer(psFBInfo) < 0)
	{
		printk(KERN_ERR DRVNAME ": OSRegisterDevice: Unable to register frame buffer.\n");
		goto err_reg_fb;
	}

	psDevInfo->hOSHandle = (IMG_HANDLE) psFBInfo;

	return (DC_OK);

err_reg_fb:
	fb_dealloc_cmap(&psFBInfo->cmap);
err_alloc_cmap:
    UnMapPhysAddr(psFBInfo->screen_base, psFBInfo->fix.smem_len);
err_map_video:
	kfree(psFBInfo->pseudo_palette);
	framebuffer_release(psFBInfo);

	return (DC_ERROR_GENERIC);
}

/*****************************************************************************
 Function Name:	OSUnregisterDevice
 Description  :	Remove the device from the OS.
*****************************************************************************/
DC_ERROR OSUnregisterDevice(PVRCOBRA_DEVINFO *psDevInfo)
{
	if (psDevInfo->hOSHandle)
	{
		struct fb_info *psFBInfo = (struct fb_info *) psDevInfo->hOSHandle;

		if (unregister_framebuffer(psFBInfo))
		{
			printk(KERN_ERR DRVNAME ": OSUnregisterDevice: Unable to release frame buffer.\n");
			return (DC_ERROR_GENERIC);
		}

		fb_dealloc_cmap(&psFBInfo->cmap);
		UnMapPhysAddr(psFBInfo->screen_base, psFBInfo->fix.smem_len);
		kfree(psFBInfo->pseudo_palette);
		framebuffer_release(psFBInfo);

		psDevInfo->hOSHandle = 0;
	}

	return (DC_OK);
}

#endif /* SUPPORT_PVRCOBRA_OWN_LINUX_FB) */


/*****************************************************************************
 Function Name:	dc_cobra_init
 Description  :	Insert the driver into the kernel.

				__init places the function in a special memory section that
				the kernel frees once the function has been run.  Refer also
				to module_init() macro call below.

*****************************************************************************/
#if defined(SUPPORT_DRI_DRM)
int PVR_DRV_MAKENAME(DISPLAY_CONTROLLER, _init)(struct drm_device unref__ *dev)
#else
static int __init dc_cobra_init(void)
#endif
{
	int err;
	dev_t dev = 0;
	memset(&device, 0, sizeof(device));

	/* 1. find proper major number */
	{
		if (img_major == 0)
		{
			err = alloc_chrdev_region(&dev, 0, 1, img_dev_name);
			img_major = MAJOR(dev);
		}
		else
		{
			dev = MKDEV(img_major, 0);
			err = register_chrdev_region(dev, 1, img_dev_name);
		}

		if (err)
			goto _init_chrdev_error;
	}

	/* 2. initialize char dev data */
	cdev_init(&device.cdev, &img_dev_fops);
	device.cdev.owner = THIS_MODULE;
	device.cdev.ops = &img_dev_fops;

	/* 3. register char dev */
	err = cdev_add(&device.cdev, dev, 1);
	if (err)
		goto _init_cdev_error;
	
	/* 4. call the Init function */
	if(DCInit() != DC_OK)
	{
		IMG_PRINT_ERROR(("Can't init device\n"));
		return -ENODEV;
	}
	return 0;

_init_cdev_error:
	unregister_chrdev_region(dev, 1);
_init_chrdev_error:
	return err;
}

/*****************************************************************************
 Function Name:	dc_cobra_exit
 Description  :	Remove the driver from the kernel.

				__exit places the function in a special memory section that
				the kernel frees once the function has been run.  Refer also
				to module_exit() macro call below.

*****************************************************************************/
#if defined(SUPPORT_DRI_DRM)
void PVR_DRV_MAKENAME(DISPLAY_CONTROLLER, _exit)(struct drm_device unref__ *dev)
#else
static void __exit dc_cobra_exit(void)
#endif
{
	/* 1. call the Deinit function */
	if(DCDeInit() != DC_OK)
	{
		IMG_PRINT_ERROR(("Can't deinit device\n"));
	}
	else
	{
		dev_t dev = MKDEV(img_major, 0);
		
		/* 1. unregister char device */
		cdev_del(&device.cdev);

		/* 2. free major */
		unregister_chrdev_region(dev, 1);
	}
}

DC_ERROR DCGetLibFuncAddr(char * szFunctionName,PFN_DC_GET_PVRJTABLE * ppfnFuncTable)
{
	if(strcmp("PVRGetDisplayClassJTable", szFunctionName) != 0)
		return DC_ERROR_INVALID_PARAMS;

	/* Nothing to do - should be exported from pvrsrv.ko */
	*ppfnFuncTable = PVRGetDisplayClassJTable;
	return DC_OK;
}

#if !defined(SUPPORT_DRI_DRM)
/*
 These macro calls define the initialisation and removal functions of the
 driver.  Although they are prefixed `module_', they apply when compiling
 statically as well; in both cases they define the function the kernel will
 run to start/stop the driver.
*/
module_init(dc_cobra_init);
module_exit(dc_cobra_exit);

#endif

