/**************************************************************************
 * Name         : dc_cobra_util.c
 * Author       : Billy Park
 * Created      : 2/6/2012
 *
 * Copyright    : 2016 by Mediatek Limited. All rights reserved.
 *
 *
 * Platform     : cobra
 *
 * $Date: 2015/12/22 $ $Revision: #1 $
 * $Log: dc_cobra_util.c $
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
 PowerVR Services ?IMG is not providing a display driver implementation
 **************************************************************************/

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/jiffies.h>
#include <linux/time.h>
#include <asm/delay.h>

/* IMG services headers */
#include "img_defs.h"
#include "servicesext.h"
#include "kerneldisplay.h"

#include "dc_cobra_debug.h"
#include "dc_cobra_displayclass.h"
#include "dc_cobra.h"
#include "dc_cobra_osk.h"

/* top level 'hook ptr' */
static PVRCOBRA_DEVINFO * gpsDevInfo[DC_MAX_NUM_FB_DEVICES];

/*****************************************************************************
 Function Name:	CobraDCAllocKernelMem
 Description  :	
*****************************************************************************/
void * CobraDCAllocKernelMem(IMG_UINT32 ui32Size)
{
	return kmalloc(ui32Size, GFP_KERNEL);
}

/*****************************************************************************
 Function Name:	CobraDCFreeKernelMem
 Description  :	
*****************************************************************************/
void CobraDCFreeKernelMem(void *pvMem)
{
	kfree(pvMem);
}

/*****************************************************************************
 Function Name:	CobraDCGetDevInfoPtr
 Description  :	
*****************************************************************************/
PVRCOBRA_DEVINFO * CobraDCGetDevInfoPtr(IMG_UINT32 uiFBDevID)
{
	if(uiFBDevID < DC_MAX_NUM_FB_DEVICES)
		return gpsDevInfo[uiFBDevID];
	return NULL;
}

/*****************************************************************************
 Function Name:	CobraDCSetDevInfoPtr
 Description  :	
*****************************************************************************/
void CobraDCSetDevInfoPtr(IMG_UINT32 uiFBDevID, PVRCOBRA_DEVINFO *psDevInfo)
{
	if(uiFBDevID < DC_MAX_NUM_FB_DEVICES)
		gpsDevInfo[uiFBDevID] = psDevInfo;
}
