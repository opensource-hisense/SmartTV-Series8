/**************************************************************************
 * Name         : dc_cobra_displayclass.c
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
 The 3rd party driver is a specification of an API to integrate the IMG POWERVR
 Services driver with 3rd Party display hardware.  It is NOT a specification for
 a display controller driver, rather a specification to extend the API for a
 pre-existing driver for the display hardware.

 The 3rd party driver interface provides IMG POWERVR client drivers (e.g. PVR2D)
 with an API abstraction of the system’s underlying display hardware, allowing
 the client drivers to indirectly control the display hardware and access its
 associated memory.

 Functions of the API include
 - query primary surface attributes (width, height, stride, pixel format, CPU
     physical and virtual address)
 - swap/flip chain creation and subsequent query of surface attributes
 - asynchronous display surface flipping, taking account of asynchronous read
 (flip) and write (render) operations to the display surface

 Note: having queried surface attributes the client drivers are able to map the
 display memory to any IMG POWERVR Services device by calling
 PVRSRVMapDeviceClassMemory with the display surface handle.

 This code is intended to be an example of how a pre-existing display driver may
 be extended to support the 3rd Party Display interface to POWERVR Services
 ?IMG is not providing a display driver implementation.

 Use of IMG types:
 IMG types are used in the DDK 3rd party example implementations for ease of
 implementation and to correctly interface with POWERVR Services functions.
 The use of IMG types in the example code should not be interpreted as the code
 being the responsibility of Imagination Technologies.
 **************************************************************************/

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/console.h>
#include <linux/fb.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/notifier.h>
#include <asm/delay.h>

/* IMG services headers */
#include "img_defs.h"
#include "servicesext.h"
#include "kerneldisplay.h"

#include "dc_cobra_debug.h"
#include "dc_cobra_displayclass.h"
#include "dc_cobra_osk.h"
#include "dc_cobra.h"

#define DISPLAY_DEVICE_NAME "DC_COBRA"
#define DC_COBRA_COMMAND_COUNT		1

#define MAX(a,b)		(((a)>(b))?(a):(b))
#define MIN(a,b)		(((a)<(b))?(a):(b))
#define ALIGNSIZE(size, alignshift)	(((size) + ((1UL << (alignshift))-1)) & ~((1UL << (alignshift))-1))

/* PowerVR services callback table ptr */
static PFN_DC_GET_PVRJTABLE gpfnGetPVRJTable = 0;

PVRCOBRA_DEVINFO *  CobraDevInit(IMG_UINT32 uiFBDevID);
DC_ERROR CobraInitFBDev(PVRCOBRA_DEVINFO *psDevInfo);
DC_ERROR CobraDeInitFBDev(PVRCOBRA_DEVINFO *psDevInfo);

DC_ERROR CobraEnableLFBEventNotification(PVRCOBRA_DEVINFO *psDevInfo);
DC_ERROR CobraDisableLFBEventNotification(PVRCOBRA_DEVINFO *psDevInfo);
void	 CobraCheckModeAndSync(PVRCOBRA_DEVINFO *psDevInfo);

static void __CobraAdvancedFlipIndex(PVRCOBRA_DEVINFO *psDevInfo, IMG_UINT32 *puiIndex, PVRCOBRA_SWAPCHAIN *psSwapChain)
{
	unsigned long uiMaxFlipIndex;
	
	uiMaxFlipIndex = psSwapChain->uiBufferCount - 1;
	if (uiMaxFlipIndex >= psDevInfo->uiNumFrameBuffers)
		uiMaxFlipIndex = psDevInfo->uiNumFrameBuffers - 1;

	(*puiIndex)++;

	if (*puiIndex > uiMaxFlipIndex )
	{
		*puiIndex = 0;
	}
}

static void __CobraResetVSyncFlipItems(PVRCOBRA_DEVINFO *psDevInfo)
{
	unsigned long i;

	psDevInfo->uiInsertIndex = 0;
	psDevInfo->uiRemoveIndex = 0;

	for(i = 0; i < psDevInfo->uiNumFrameBuffers; ++i)
	{
		psDevInfo->asVSyncFlips[i].bValid = IMG_FALSE;
		psDevInfo->asVSyncFlips[i].bFlipped = IMG_FALSE;
		psDevInfo->asVSyncFlips[i].bCmdCompleted = IMG_FALSE;
	}
}

static DC_ERROR __CobraFlip(PVRCOBRA_DEVINFO *psDevInfo, PVRCOBRA_FRAME_BUFFER *psBuffer)
{
	struct fb_var_screeninfo sFBVar;
	struct fb_info *psLinkFBInfo = psDevInfo->psLinkFBInfo;

	if(!psLinkFBInfo)
	{
		IMG_PRINT_ERROR(("%s: psLinkFBInfo must not be NULL.\n", __FUNCTION__));
	}
	
	IMG_DEBUG_PRINT(7, ("%s: DevInfo(0x%p), Buffer(0x%p)\n", __FUNCTION__, psDevInfo, psBuffer));

	/* FBDev_X */
	IMG_DEBUG_PRINT(7, ("\t\tFBDevID : %d\n", psDevInfo->uiFBDevID));

	/* physical address */
	IMG_DEBUG_PRINT(7, ("\t\tPhyAddr : 0x%08X\n", psBuffer->sSysPAddr.uiAddr));

	/* virtual address */
	IMG_DEBUG_PRINT(7, ("\t\tVirAddr : 0x%08X\n", psBuffer->sCPUVAddr));

	sFBVar = psLinkFBInfo->var;
	sFBVar.xoffset = 0;
	sFBVar.yoffset = psBuffer->iYOffset;

#ifdef SUPPORT_STRETCH_TO_DISPLAY
	// There is nothing to do in the case of customer project.
	// All of the flipping control is in customer side.
#else
  #if 1
	if(psLinkFBInfo->fbops->fb_pan_display)
	{	
		int res;
		IMG_DEBUG_PRINT(7, ("\t--> fb_var_screeninfo\n"));
		IMG_DEBUG_PRINT(7, ("\t- widthmm : %d\n", sFBVar.width));
		IMG_DEBUG_PRINT(7, ("\t- heightmm: %d\n", sFBVar.height));
		IMG_DEBUG_PRINT(7, ("\t- xres  : %d\n", sFBVar.xres));
		IMG_DEBUG_PRINT(7, ("\t- yres  : %d\n", sFBVar.yres));
		IMG_DEBUG_PRINT(7, ("\t- xres_virtual : %d\n", sFBVar.xres_virtual));
		IMG_DEBUG_PRINT(7, ("\t- yres_virtual : %d\n", sFBVar.yres_virtual));
		IMG_DEBUG_PRINT(7, ("\t- xoffset : %d\n", sFBVar.xoffset));
		IMG_DEBUG_PRINT(7, ("\t- yoffset : %d\n", sFBVar.yoffset));
		IMG_DEBUG_PRINT(7, ("\t- bpp : %d\n", sFBVar.bits_per_pixel));

		res = psLinkFBInfo->fbops->fb_pan_display(&sFBVar, psLinkFBInfo);
		if(res != 0)
			IMG_PRINT_ERROR(("%s: fb_pan_display failed (yoffset: %d, error: %d)\n", __FUNCTION__, psBuffer->iYOffset, res));
		else
			IMG_DEBUG_PRINT(7, ("\t--> fb_pan_display success (yoffset: %d)\n", psBuffer->iYOffset));
	}
  #else
	if(psLinkFBInfo->fbops->fb_ioctl)
	{
		int res;
		IMG_DEBUG_PRINT(7, ("\t--> fb_var_screeninfo\n"));
		IMG_DEBUG_PRINT(7, ("\t- widthmm : %d\n", sFBVar.width));
		IMG_DEBUG_PRINT(7, ("\t- heightmm: %d\n", sFBVar.height));
		IMG_DEBUG_PRINT(7, ("\t- xres  : %d\n", sFBVar.xres));
		IMG_DEBUG_PRINT(7, ("\t- yres  : %d\n", sFBVar.yres));
		IMG_DEBUG_PRINT(7, ("\t- xres_virtual : %d\n", sFBVar.xres_virtual));
		IMG_DEBUG_PRINT(7, ("\t- yres_virtual : %d\n", sFBVar.yres_virtual));
		IMG_DEBUG_PRINT(7, ("\t- xoffset : %d\n", sFBVar.xoffset));
		IMG_DEBUG_PRINT(7, ("\t- yoffset : %d\n", sFBVar.yoffset));
		IMG_DEBUG_PRINT(7, ("\t- bpp : %d\n", sFBVar.bits_per_pixel));

		sFBVar.reserved[0] = 0;
		sFBVar.reserved[1] = sFBVar.xres;
		sFBVar.reserved[2] = sFBVar.yres;
		sFBVar.reserved[3] = 0;
		sFBVar.reserved[4] = 0;

		res = psLinkFBInfo->fbops->fb_ioctl(psLinkFBInfo, 0x4662, &sFBVar);
		if(res != 0)
			IMG_PRINT_ERROR(("%s: fb_ioctl failed (yoffset: %d, error: %d)\n", __FUNCTION__, psBuffer->iYOffset, res));
		else
			IMG_DEBUG_PRINT(7, ("\t--> fb_ioctl success (yoffset: %d)\n", psBuffer->iYOffset));
	}
  #endif
#endif

	return DC_OK;
}

static void __CobraFlushInternalVSyncQueue(PVRCOBRA_DEVINFO *psDevInfo, PVRCOBRA_SWAPCHAIN * psSwapChain)
{
	PVRCOBRA_VSYNC_FLIP_ITEM* psFlipItem;

	psFlipItem = &psDevInfo->asVSyncFlips[psDevInfo->uiRemoveIndex];

	while(psFlipItem->bValid)
	{
		if(psFlipItem->bFlipped == IMG_FALSE)
		{
			__CobraFlip(psDevInfo, psFlipItem->psBuffer);
		}

		if(psFlipItem->bCmdCompleted == IMG_FALSE)
		{
			psDevInfo->sPVRJTable.pfnPVRSRVCmdComplete((IMG_HANDLE)psFlipItem->hCmdComplete, IMG_FALSE);
		}

		__CobraAdvancedFlipIndex(psDevInfo, &psDevInfo->uiRemoveIndex, psSwapChain);

		psFlipItem->bFlipped = IMG_FALSE;
		psFlipItem->bCmdCompleted = IMG_FALSE;
		psFlipItem->bValid = IMG_FALSE;
		psFlipItem = &psDevInfo->asVSyncFlips[psDevInfo->uiRemoveIndex];
	}
	psDevInfo->uiInsertIndex = 0;
	psDevInfo->uiRemoveIndex = 0;
}

static DC_ERROR __CobraSwapChainHasChanged(PVRCOBRA_DEVINFO *psDevInfo, PVRCOBRA_SWAPCHAIN *psSwapChain)
{
	return DC_OK;
}

static DC_ERROR __CobraPVRPixelFormat(const struct fb_var_screeninfo *var, PVRSRV_PIXEL_FORMAT *pPVRPixelFormat)
{
	if(var->bits_per_pixel == 16)
	{
		IMG_DEBUG_PRINT(7, ("%s: 16-bit format\n", __FUNCTION__));

		if( var->red.length == 5  && var->green.length == 6 && var->blue.length == 5 &&
			var->red.offset == 11 && var->green.offset == 5 && var->blue.offset == 0 &&
			var->red.msb_right == 0)
		{
			*pPVRPixelFormat = PVRSRV_PIXEL_FORMAT_RGB565;
			IMG_DEBUG_PRINT(7, ("%s: PVRSRV_PIXEL_FORMAT_RGB565\n", __FUNCTION__));
		}
		else if(var->red.length == 4 && var->green.length == 4 && var->blue.length == 4 &&
				var->red.offset == 8 && var->green.offset == 4 && var->blue.offset == 0 &&
				var->red.msb_right == 0)
		{
			*pPVRPixelFormat = PVRSRV_PIXEL_FORMAT_ARGB4444;
			IMG_DEBUG_PRINT(7, ("%s: PVRSRV_PIXEL_FORMAT_ARGB4444\n", __FUNCTION__));
		}
		else if(var->red.length == 5  && var->green.length == 5 && var->blue.length == 5 &&
				var->red.offset == 10 && var->green.offset == 5 && var->blue.offset == 0)
		{
			*pPVRPixelFormat = PVRSRV_PIXEL_FORMAT_ARGB1555;
			IMG_DEBUG_PRINT(7, ("%s: PVRSRV_PIXEL_FORMAT_ARGB1555\n", __FUNCTION__));
		}
		else
		{
			*pPVRPixelFormat = PVRSRV_PIXEL_FORMAT_UNKNOWN;
			return DC_ERROR_INVALID_PARAMS;
		}
	}
	else if(var->bits_per_pixel == 32)
	{
		IMG_DEBUG_PRINT(7, ("%s: 32-bit format\n", __FUNCTION__));
		
		if( var->red.length == 8  && var->green.length == 8 && var->blue.length == 8 &&
			var->red.offset == 16 && var->green.offset == 8 && var->blue.offset == 0 &&
			var->red.msb_right == 0)
		{
			*pPVRPixelFormat = PVRSRV_PIXEL_FORMAT_ARGB8888; //PVRSRV_PIXEL_FORMAT_XRGB8888;
			IMG_DEBUG_PRINT(7, ("%s: PVRSRV_PIXEL_FORMAT_ARGB8888\n", __FUNCTION__));
		}
		else if(var->red.length == 8 && var->green.length == 8 && var->blue.length == 8  &&
				var->red.offset == 0 && var->green.offset == 8 && var->blue.offset == 16 &&
				var->red.msb_right == 0)
		{
			*pPVRPixelFormat = PVRSRV_PIXEL_FORMAT_ABGR8888; //PVRSRV_PIXEL_FORMAT_XBGR8888;
			IMG_DEBUG_PRINT(7, ("%s: PVRSRV_PIXEL_FORMAT_ABGR8888\n", __FUNCTION__));
		}
		else
		{
			*pPVRPixelFormat = PVRSRV_PIXEL_FORMAT_UNKNOWN;
			return DC_ERROR_INVALID_PARAMS;
		}
	}
	else
	{
		*pPVRPixelFormat = PVRSRV_PIXEL_FORMAT_UNKNOWN;
		return DC_ERROR_INVALID_PARAMS;
	}
	return DC_OK;
}

//////////////////////////////////////////////////////

#if 0
PVRSRV_ERROR CobraUnblankDisplay(PVRCOBRA_DEVINFO *psDevInfo)
{
	PVRCOBRA_CONSOLE_LOCK();
	fb_blank(psDevInfo->psLinkFBInfo, 0);
	PVRCOBRA_CONSOLE_UNLOCK();
	return PVRSRV_OK;
}

void CobraBlankDisplay(PVRCOBRA_DEVINFO *psDevInfo)
{
	PVRCOBRA_CONSOLE_LOCK();
	fb_blank(psDevInfo->psLinkFBInfo, 1);
	PVRCOBRA_CONSOLE_UNLOCK();
}
#endif

//////////////////////////////////////////////////////

/* services to display Kernel APIs */
static PVRSRV_ERROR OpenDCDevice(IMG_UINT32 uiPVRDevID,IMG_HANDLE * phDevice,PVRSRV_SYNC_DATA * psPrimaryBufferSyncData)
{
	IMG_UINT32 i;
	PVRCOBRA_DEVINFO *psDevInfo;	

	IMG_DEBUG_PRINT(7, ("* %s (pid : %d): request device id(%d)\n", __FUNCTION__, _img_osk_get_pid(), uiPVRDevID));
	for(i = 0; i < DC_MAX_NUM_FB_DEVICES; ++i)
	{
		psDevInfo = CobraDCGetDevInfoPtr(i);
		if(psDevInfo != NULL && psDevInfo->uiPVRDevID == uiPVRDevID)
			break;
	}
	if(i == DC_MAX_NUM_FB_DEVICES)
	{
		IMG_DEBUG_PRINT(7, ("%s: PVR Device %u not found\n", __FUNCTION__, uiPVRDevID));
		return PVRSRV_ERROR_INVALID_DEVICEID;
	}

	/* unblank display */
//	if(CobraUnblankDisplay(psDevInfo) != PVRSRV_OK)
//		return PVRSRV_ERROR_UNBLANK_DISPLAY_FAILED;

	/* store the system surface sync data */
	
	psDevInfo->sPrimaryBuffer.psSyncData = psPrimaryBufferSyncData;

	/* return handle to the devinfo */
	*phDevice = (DC_HANDLE)psDevInfo;

	IMG_DEBUG_PRINT(7, ("\t\tDevID : %d\n", psDevInfo->uiPVRDevID));
	return PVRSRV_OK;
}

static PVRSRV_ERROR CloseDCDevice(IMG_HANDLE hDevice)
{
#ifdef DEBUG
	PVRCOBRA_DEVINFO * psDevInfo = (PVRCOBRA_DEVINFO *)hDevice;

//	CobraUnblankDisplay(psDevInfo);
	IMG_DEBUG_PRINT(7, ("* %s (pid : %d)\n", __FUNCTION__, _img_osk_get_pid()));
	IMG_DEBUG_PRINT(7, ("\t\tDevID : %d\n", psDevInfo->uiPVRDevID));
#endif
	return PVRSRV_OK;
}

/* Enumerate formats function, called from services */
static PVRSRV_ERROR EnumDCFormats(IMG_HANDLE hDevice,IMG_UINT32 * pui32NumFormats,DISPLAY_FORMAT * psFormat)
{
	PVRCOBRA_DEVINFO *psDevInfo;
	IMG_DEBUG_PRINT(7, ("* %s (pid : %d)\n", __FUNCTION__, _img_osk_get_pid()));

	/* check parameters */
	if(!hDevice || !pui32NumFormats)
	{
		IMG_DEBUG_PRINT(7, ("Invalid Parameters.\n"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psDevInfo = (PVRCOBRA_DEVINFO *)hDevice;

	if(pui32NumFormats != IMG_NULL)
		*pui32NumFormats = psDevInfo->uiNumFormats;

	if(psFormat != IMG_NULL)
	{
		IMG_UINT32 i;

		for(i = 0; i < psDevInfo->uiNumFormats; ++i)
		{
			psFormat[i] = psDevInfo->sDisplayFormatList[i];
	
			IMG_DEBUG_PRINT(7, ("\t\tFormats[%d]\n", i));
			IMG_DEBUG_PRINT(7, ("\t\t - PixelFormat : %d\n", psFormat->pixelformat));
			if(psFormat->pixelformat <= PVRSRV_PIXEL_FORMAT_UNKNOWN || psFormat->pixelformat > PVRSRV_PIXEL_FORMAT_RAW1024)
				IMG_PRINT_ERROR(("\t\tInvalid Pixel Format!!!\n"));
		}
	}
	return PVRSRV_OK;
}

/* Enumerate dims function, called from services */
static PVRSRV_ERROR EnumDCDims(IMG_HANDLE hDevice,DISPLAY_FORMAT * psFormat,IMG_UINT32 * pui32NumDims,DISPLAY_DIMS * psDim)
{
	PVRCOBRA_DEVINFO *psDevInfo;
	IMG_DEBUG_PRINT(7, ("* %s (pid : %d)\n", __FUNCTION__, _img_osk_get_pid()));
	
	/* check parameters */
	if(!hDevice || !psFormat ||!pui32NumDims)
	{
		IMG_PRINT_ERROR(("Invalid Parameters.\n"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psDevInfo = (PVRCOBRA_DEVINFO *)hDevice;

	if(pui32NumDims != IMG_NULL)
		*pui32NumDims = psDevInfo->uiNumDims;

	/* given psFormat return the available Dims */
	if(psDim != IMG_NULL)
	{
		IMG_UINT32 i;

		for(i = 0; i < psDevInfo->uiNumDims; ++i)
		{
			psDim[i] = psDevInfo->sDisplayDimList[i];
			IMG_DEBUG_PRINT(7, ("\t\tDCDim[%d]\n", i));
			IMG_DEBUG_PRINT(7, ("\t\t - Width  : %d\n", psDim[i].ui32Width));
			IMG_DEBUG_PRINT(7, ("\t\t - Height : %d\n", psDim[i].ui32Height));
			IMG_DEBUG_PRINT(7, ("\t\t - ByteStride : %d\n", psDim[i].ui32ByteStride));
		}
	}
	return PVRSRV_OK;
}

static PVRSRV_ERROR GetDCSystemBuffer(IMG_HANDLE hDevice,IMG_HANDLE * phBuffer)
{
	PVRCOBRA_DEVINFO *psDevInfo;
	IMG_DEBUG_PRINT(7, ("* %s (pid : %d)\n", __FUNCTION__, _img_osk_get_pid()));

	if(!hDevice || !phBuffer)
	{
		IMG_PRINT_ERROR(("Invalid Parameters.\n"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psDevInfo = (PVRCOBRA_DEVINFO *)hDevice;
	*phBuffer = (IMG_HANDLE)&psDevInfo->sPrimaryBuffer;

	IMG_DEBUG_PRINT(7, ("\t\tDeviceID : %d, FBDevID : %d\n", psDevInfo->uiPVRDevID, psDevInfo->uiFBDevID));
	IMG_DEBUG_PRINT(7, ("\t\tphBuffer : 0x%08X\n", *phBuffer));	
	return PVRSRV_OK;
}

static PVRSRV_ERROR GetDCInfo(IMG_HANDLE hDevice,DISPLAY_INFO * psDCInfo)
{
	PVRCOBRA_DEVINFO * psDevInfo;
	IMG_DEBUG_PRINT(7, ("* %s (pid : %d)\n", __FUNCTION__, _img_osk_get_pid()));

	if(!hDevice || !psDCInfo)
	{
		IMG_PRINT_ERROR(("Invalid Parameters.\n"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psDevInfo = (PVRCOBRA_DEVINFO *)hDevice;
	*psDCInfo = psDevInfo->sDisplayInfo;
	
	IMG_DEBUG_PRINT(7, ("\t\tDeviceID : %d, FBDevID : %d\n", psDevInfo->uiPVRDevID, psDevInfo->uiFBDevID));
	IMG_DEBUG_PRINT(7, ("\t\tui32MaxSwapChains    : %d\n", psDCInfo->ui32MaxSwapChains));	
	IMG_DEBUG_PRINT(7, ("\t\tui32MaxSwapChainBuffers : %d\n", psDCInfo->ui32MaxSwapChainBuffers));	
	IMG_DEBUG_PRINT(7, ("\t\tui32MinSwapInterval  : %d\n", psDCInfo->ui32MinSwapInterval));	
	IMG_DEBUG_PRINT(7, ("\t\tui32MaxSwapInterval  : %d\n", psDCInfo->ui32MaxSwapInterval));	
	IMG_DEBUG_PRINT(7, ("\t\tui32PhysicalWidthmm  : %d\n", psDCInfo->ui32PhysicalWidthmm));	
	IMG_DEBUG_PRINT(7, ("\t\tui32PhysicalHeightmm : %d\n", psDCInfo->ui32PhysicalHeightmm));	
	return PVRSRV_OK;
}

static PVRSRV_ERROR GetDCBufferAddr(IMG_HANDLE hDevice,IMG_HANDLE hBuffer,IMG_SYS_PHYADDR ** ppsSysPAddr,IMG_UINT32 * pui32ByteSize,IMG_VOID ** ppvCpuVAddr,IMG_HANDLE * phOSMapInfo,
									IMG_BOOL * pbIsContiguous,IMG_UINT32 * pui32TilingStride)
{
	PVRCOBRA_DEVINFO *psDevInfo;
	PVRCOBRA_FRAME_BUFFER *psBuffer;
	IMG_DEBUG_PRINT(7, ("* %s (pid : %d)\n", __FUNCTION__, _img_osk_get_pid()));

	UNREFERENCED_PARAMETER(pui32TilingStride);

	if(!hDevice || !hBuffer || !ppsSysPAddr || !pui32ByteSize)
	{
		IMG_PRINT_ERROR(("Invalid Parameters.\n"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psDevInfo = (PVRCOBRA_DEVINFO *)hDevice;
	psBuffer = (PVRCOBRA_FRAME_BUFFER *)hBuffer;

	*ppsSysPAddr = &psBuffer->sSysPAddr;
	*ppvCpuVAddr = psBuffer->sCPUVAddr;
	
	*pui32ByteSize = (IMG_UINT32)(psDevInfo->sFBInfo.uiHeight * psDevInfo->sFBInfo.uiByteStride);

	*phOSMapInfo = IMG_NULL;
	*pbIsContiguous = IMG_TRUE;

	IMG_DEBUG_PRINT(7, ("\t\tDeviceID : %d, FBDevID : %d\n", psDevInfo->uiPVRDevID, psDevInfo->uiFBDevID));
	IMG_DEBUG_PRINT(7, ("\t\thBuffer  : 0x%08X\n", (IMG_UINT32)hBuffer));
	IMG_DEBUG_PRINT(7, ("\t\tSys_PhyAddr   : 0x%08X\n", (*ppsSysPAddr)->uiAddr));
	IMG_DEBUG_PRINT(7, ("\t\tCpu_VirtAddr  : 0x%08X\n", *ppvCpuVAddr));
	IMG_DEBUG_PRINT(7, ("\t\tpui32ByteSize : 0x%08X\n", *pui32ByteSize));
	return PVRSRV_OK;
}

static PVRSRV_ERROR CreateDCSwapChain(IMG_HANDLE hDevice,IMG_UINT32 ui32Flags,DISPLAY_SURF_ATTRIBUTES * psDstSurfAttrib,DISPLAY_SURF_ATTRIBUTES * psSrcSurfAttrib,
									IMG_UINT32 ui32BufferCount,PVRSRV_SYNC_DATA ** ppsSyncData,IMG_UINT32 ui32OEMFlags,IMG_HANDLE *phSwapChain, IMG_UINT32 *pui32SwapChainID)
{
	static IMG_UINT32 s_ui32SwapChainID;
	PVRCOBRA_DEVINFO *psDevInfo;
	PVRCOBRA_SWAPCHAIN *psSwapChain;
	PVRCOBRA_FRAME_BUFFER *psBuffer;
	IMG_UINT32 i;
	PVRSRV_ERROR eError;
	IMG_UINT32 ui32BuffersToSkip;
#if SUPPORT_PVRCOBRA_MULTI_SWAPCHAINS
	int empty_idx = -1;
#endif
	IMG_DEBUG_PRINT(7, ("* %s (pid : %d)\n", __FUNCTION__, _img_osk_get_pid()));

	UNREFERENCED_PARAMETER(ui32OEMFlags);
	UNREFERENCED_PARAMETER(pui32SwapChainID);

	if(!hDevice || !psDstSurfAttrib || !psSrcSurfAttrib || !ppsSyncData || !phSwapChain)
	{
		IMG_PRINT_ERROR(("Invalid Parameters.\n"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}
	
	psDevInfo = (PVRCOBRA_DEVINFO *)hDevice;

	if(psDevInfo->sDisplayInfo.ui32MaxSwapChains == 0)
	{
		IMG_PRINT_ERROR(("Not supported : MaxSwapChains are 0.\n"));
		return PVRSRV_ERROR_NOT_SUPPORTED;
	}

#if SUPPORT_PVRCOBRA_MULTI_SWAPCHAINS
	{
		for(i = 0; i < PVRCOBRA_MAX_SWAPCHAINS; ++i)
		{

			if(psDevInfo->auiSwapChainOwner[i] == _img_osk_get_pid() &&
			   psDevInfo->apsSwapChain[i] != NULL)
			{
				IMG_PRINT_ERROR(("%d(%d), Swap chains already exits.\n", i, _img_osk_get_pid()));
				return PVRSRV_ERROR_FLIP_CHAIN_EXISTS;
			}

			if(empty_idx < 0 &&
			   psDevInfo->auiSwapChainOwner[i] == 0 &&
			   psDevInfo->apsSwapChain[i] == NULL) 
				empty_idx = i;
			
		}
		if(empty_idx < 0)
		{
			IMG_PRINT_ERROR(("Swap chains are full.\n"));
			return PVRSRV_ERROR_FLIP_CHAIN_EXISTS;
		}
	}
#else
	if(psDevInfo->psSwapChain != NULL)
	{
		IMG_PRINT_ERROR(("Swap chains already exits.\n"));
		return PVRSRV_ERROR_FLIP_CHAIN_EXISTS;
	}
#endif

	/* check the buffer count */
	if(ui32BufferCount > psDevInfo->sDisplayInfo.ui32MaxSwapChainBuffers)
	{
		IMG_PRINT_ERROR(("Too many buffers : max(%d), request(%d)\n", 
									psDevInfo->sDisplayInfo.ui32MaxSwapChainBuffers, ui32BufferCount));
		return PVRSRV_ERROR_TOOMANYBUFFERS;
	}

	if((psDevInfo->sFBInfo.uiRoundedBufferSize * (IMG_UINT32)ui32BufferCount) > psDevInfo->sFBInfo.uiFBSize)
	{
		IMG_PRINT_ERROR(("Too many buffers : rounded size(%d bytes), buffers(%d), fb size(%d bytes).\n",
									psDevInfo->sFBInfo.uiRoundedBufferSize, ui32BufferCount, psDevInfo->sFBInfo.uiFBSize));
		return PVRSRV_ERROR_TOOMANYBUFFERS;
	}

	ui32BuffersToSkip = psDevInfo->sDisplayInfo.ui32MaxSwapChainBuffers - ui32BufferCount;

	/* check the DST/SRC attributes */
	if(psDstSurfAttrib->pixelformat != psDevInfo->psCurrentDisplayFormat->pixelformat
	|| psDstSurfAttrib->sDims.ui32ByteStride != psDevInfo->psCurrentDisplayDim->ui32ByteStride
	|| psDstSurfAttrib->sDims.ui32Width != psDevInfo->psCurrentDisplayDim->ui32Width
	|| psDstSurfAttrib->sDims.ui32Height != psDevInfo->psCurrentDisplayDim->ui32Height)
	{
		IMG_PRINT_ERROR(("DST doesn't match the current mode.\n"));
		IMG_DEBUG_PRINT(7, ("--> Dst\n"));
		IMG_DEBUG_PRINT(7, ("\t\tPixelFormat : %d\n", psDstSurfAttrib->pixelformat));
		IMG_DEBUG_PRINT(7, ("\t\tWidth : %d\n", psDstSurfAttrib->sDims.ui32Width));
		IMG_DEBUG_PRINT(7, ("\t\tHeight : %d\n", psDstSurfAttrib->sDims.ui32Height));
		IMG_DEBUG_PRINT(7, ("\t\tByteStride : %d\n", psDstSurfAttrib->sDims.ui32ByteStride));

		IMG_DEBUG_PRINT(7, ("--> Current\n"));
		IMG_DEBUG_PRINT(7, ("\t\tPixelFormat : %d\n", psDevInfo->psCurrentDisplayFormat->pixelformat));
		IMG_DEBUG_PRINT(7, ("\t\tWidth : %d\n", psDevInfo->psCurrentDisplayDim->ui32Width));
		IMG_DEBUG_PRINT(7, ("\t\tHeight : %d\n", psDevInfo->psCurrentDisplayDim->ui32Height));
		IMG_DEBUG_PRINT(7, ("\t\tByteStride : %d\n", psDevInfo->psCurrentDisplayDim->ui32ByteStride));

		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	if(psDstSurfAttrib->pixelformat != psSrcSurfAttrib->pixelformat
	|| psDstSurfAttrib->sDims.ui32ByteStride != psSrcSurfAttrib->sDims.ui32ByteStride
	|| psDstSurfAttrib->sDims.ui32Width != psSrcSurfAttrib->sDims.ui32Width
	|| psDstSurfAttrib->sDims.ui32Height != psSrcSurfAttrib->sDims.ui32Height)
	{
		IMG_PRINT_ERROR(("DST doesn't match the SRC.\n"));
		IMG_DEBUG_PRINT(7, ("--> Dst\n"));
		IMG_DEBUG_PRINT(7, ("\t\tPixelFormat : %d\n", psDstSurfAttrib->pixelformat));
		IMG_DEBUG_PRINT(7, ("\t\tWidth : %d\n", psDstSurfAttrib->sDims.ui32Width));
		IMG_DEBUG_PRINT(7, ("\t\tHeight : %d\n", psDstSurfAttrib->sDims.ui32Height));
		IMG_DEBUG_PRINT(7, ("\t\tByteStride : %d\n", psDstSurfAttrib->sDims.ui32ByteStride));

		IMG_DEBUG_PRINT(7, ("--> Src\n"));
		IMG_DEBUG_PRINT(7, ("\t\tPixelFormat : %d\n", psSrcSurfAttrib->pixelformat));
		IMG_DEBUG_PRINT(7, ("\t\tWidth : %d\n", psSrcSurfAttrib->sDims.ui32Width));
		IMG_DEBUG_PRINT(7, ("\t\tHeight : %d\n", psSrcSurfAttrib->sDims.ui32Height));
		IMG_DEBUG_PRINT(7, ("\t\tByteStride : %d\n", psSrcSurfAttrib->sDims.ui32ByteStride));

		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	UNREFERENCED_PARAMETER(ui32Flags);

	/* create a swapchain structure */
	psSwapChain = (PVRCOBRA_SWAPCHAIN *)CobraDCAllocKernelMem(sizeof(PVRCOBRA_SWAPCHAIN));
	if(!psSwapChain)
	{
		IMG_PRINT_ERROR(("Out of memoryi: the requested size is %d-byte\n", sizeof(PVRCOBRA_SWAPCHAIN)));
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}

	psBuffer = (PVRCOBRA_FRAME_BUFFER *)CobraDCAllocKernelMem(sizeof(PVRCOBRA_FRAME_BUFFER) * ui32BufferCount);
	if(!psBuffer)
	{
		IMG_PRINT_ERROR(("Out of memory: the requested size is %d-byte\n", sizeof(PVRCOBRA_FRAME_BUFFER) * ui32BufferCount));
		eError = PVRSRV_ERROR_OUT_OF_MEMORY;
		goto _error_free_swap_chain;
	}

	psSwapChain->uiBufferCount = ui32BufferCount;
	psSwapChain->psBuffer      = psBuffer;
	psSwapChain->bNotVSynced   = IMG_TRUE;
	psSwapChain->uiFBDevID     = psDevInfo->uiFBDevID;
	psSwapChain->uiSwapChainID = ++s_ui32SwapChainID;

	/* link the buffers */
	for(i = 0; i < ui32BufferCount - 1; ++i)
		psBuffer[i].psNext = &psBuffer[i+1];
	/* and link last to first */
	psBuffer[i].psNext = &psBuffer[0];

	for(i = 0; i < ui32BufferCount; ++i)
	{
		IMG_UINT32 ui32SwapBuffer = i + ui32BuffersToSkip;
		IMG_UINT32 ui32BufferOffset = ui32SwapBuffer * (IMG_UINT32)psDevInfo->sFBInfo.uiRoundedBufferSize;

		psBuffer[i].psSyncData = ppsSyncData[i];

		psBuffer[i].sSysPAddr.uiAddr = psDevInfo->sFBInfo.sSysPAddr.uiAddr + ui32BufferOffset;
		psBuffer[i].sCPUVAddr  = psDevInfo->sFBInfo.sCPUVAddr + ui32BufferOffset;
		psBuffer[i].iYOffset   = (IMG_INT32)(ui32BufferOffset / psDevInfo->sFBInfo.uiByteStride);
		psBuffer[i].hSwapChain = (DC_HANDLE)psSwapChain;
	}

#if SUPPORT_PVRCOBRA_MULTI_SWAPCHAINS
	IMG_DEBUG_PRINT(4, ("(%d)%d: ~~~~~~~~~~~ %s\n", _img_osk_get_pid(), __LINE__, __FILE__));
	psDevInfo->apsSwapChain[empty_idx] = psSwapChain;
	psDevInfo->auiSwapChainOwner[empty_idx] = _img_osk_get_pid();
#else
	psDevInfo->psSwapChain = psSwapChain;
#endif

	*pui32SwapChainID = psSwapChain->uiSwapChainID;
	*phSwapChain = (IMG_HANDLE)psSwapChain;

	__CobraResetVSyncFlipItems(psDevInfo);
	
	IMG_DEBUG_PRINT(4, ("\t\tui32FBDevID     : %d\n", psSwapChain->uiFBDevID));
	IMG_DEBUG_PRINT(4, ("\t\tui32SwapChainID : %d\n", psSwapChain->uiSwapChainID));
	IMG_DEBUG_PRINT(4, ("\t\tui32BufferCount : %d\n", psSwapChain->uiBufferCount));
	return PVRSRV_OK;

_error_free_swap_chain:
	CobraDCFreeKernelMem(psSwapChain);
	return eError;
}

/* destroy swapchain function, called from services */
static PVRSRV_ERROR DestroyDCSwapChain(IMG_HANDLE hDevice,IMG_HANDLE hSwapChain)
{
	PVRCOBRA_DEVINFO *psDevInfo;
	PVRCOBRA_SWAPCHAIN *psSwapChain;
	IMG_DEBUG_PRINT(7, ("* %s (pid : %d)\n", __FUNCTION__, _img_osk_get_pid()));

	if(!hDevice || !hSwapChain)
	{
		IMG_PRINT_ERROR(("Invalid Parameters.\n"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psDevInfo = (PVRCOBRA_DEVINFO *)hDevice;
	psSwapChain = (PVRCOBRA_SWAPCHAIN *)hSwapChain;

	if(__CobraSwapChainHasChanged(psDevInfo, psSwapChain))
	{
		IMG_PRINT_ERROR(("Swap chain mismatched\n"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	/* swap to the primary surface */
	if(__CobraFlip(psDevInfo, &psDevInfo->sPrimaryBuffer) != DC_OK)
	{
		IMG_PRINT_ERROR(("Failed to flip to the primary surface.\n"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

#if SUPPORT_PVRCOBRA_MULTI_SWAPCHAINS
	{
		IMG_UINT32 i;
		for(i = 0; i < PVRCOBRA_MAX_SWAPCHAINS; ++i)
		{
			if(psDevInfo->auiSwapChainOwner[i] == _img_osk_get_pid() &&
			   psDevInfo->apsSwapChain[i] == psSwapChain)
			{
				psDevInfo->auiSwapChainOwner[i] = 0;
				psDevInfo->apsSwapChain[i] = NULL;
				break;
			}
		}
		if(i == PVRCOBRA_MAX_SWAPCHAINS)
		{
			IMG_PRINT_ERROR(("couldn't remove the swapchain from the array.\n"));
		}
	}
#else
	/* mark swapchain as not existing */
	if(psDevInfo->psSwapChain == psSwapChain)
		psDevInfo->psSwapChain = NULL;
#endif

	/* free resources */
	CobraDCFreeKernelMem(psSwapChain->psBuffer);
	CobraDCFreeKernelMem(psSwapChain);

	__CobraResetVSyncFlipItems(psDevInfo);
	return PVRSRV_OK;
}

/* set DST rect function, called from services */
static PVRSRV_ERROR SetDCDstRect(IMG_HANDLE hDevice,IMG_HANDLE hSwapChain,IMG_RECT * psRect)
{
	IMG_DEBUG_PRINT(7, ("* %s (pid : %d)\n", __FUNCTION__, _img_osk_get_pid()));
	UNREFERENCED_PARAMETER(hDevice);
	UNREFERENCED_PARAMETER(hSwapChain);
	UNREFERENCED_PARAMETER(psRect);

	/* only full screen swapchains on this device */

	return PVRSRV_ERROR_NOT_SUPPORTED;
}

/* set SRC rect function, called from services */
static PVRSRV_ERROR SetDCSrcRect(IMG_HANDLE hDevice,IMG_HANDLE hSwapChain,IMG_RECT * psRect)
{
	IMG_DEBUG_PRINT(7, ("* %s (pid : %d)\n", __FUNCTION__, _img_osk_get_pid()));
	UNREFERENCED_PARAMETER(hDevice);
	UNREFERENCED_PARAMETER(hSwapChain);
	UNREFERENCED_PARAMETER(psRect);

	/* only full screen swapchains on this device */

	return PVRSRV_ERROR_NOT_SUPPORTED;
}

/* set DST colour key function, called from services */
static PVRSRV_ERROR SetDCDstColourKey(IMG_HANDLE hDevice,IMG_HANDLE hSwapChain,IMG_UINT32 ui32CKColour)
{
	IMG_DEBUG_PRINT(7, ("* %s (pid : %d)\n", __FUNCTION__, _img_osk_get_pid()));
	UNREFERENCED_PARAMETER(hDevice);
	UNREFERENCED_PARAMETER(hSwapChain);
	UNREFERENCED_PARAMETER(ui32CKColour);

	/* don't support DST colour key on this device */

	return PVRSRV_ERROR_NOT_SUPPORTED;
}

/* set SRC colour key function, called from services */
static PVRSRV_ERROR SetDCSrcColourKey(IMG_HANDLE hDevice,IMG_HANDLE hSwapChain,IMG_UINT32 ui32CKColour)
{
	IMG_DEBUG_PRINT(7, ("* %s (pid : %d)\n", __FUNCTION__, _img_osk_get_pid()));
	UNREFERENCED_PARAMETER(hDevice);
	UNREFERENCED_PARAMETER(hSwapChain);
	UNREFERENCED_PARAMETER(ui32CKColour);

	/* don't support DST colour key on this device */

	return PVRSRV_ERROR_NOT_SUPPORTED;
}

/* get swapchain buffers function, called from services */
static PVRSRV_ERROR GetDCBuffers(IMG_HANDLE hDevice,IMG_HANDLE hSwapChain, IMG_UINT32 * pui32BufferCount,IMG_HANDLE * phBuffer)
{
	int i;
	PVRCOBRA_DEVINFO * psDevInfo;
	PVRCOBRA_SWAPCHAIN * psSwapChain;
	IMG_DEBUG_PRINT(7, ("* %s (pid : %d)\n", __FUNCTION__, _img_osk_get_pid()));

	if(!hDevice || !hSwapChain || !pui32BufferCount || !phBuffer)
	{
		IMG_PRINT_ERROR(("Invalid Parameters.\n"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psDevInfo = (PVRCOBRA_DEVINFO *)hDevice;
	psSwapChain = (PVRCOBRA_SWAPCHAIN *)hSwapChain;

	if(__CobraSwapChainHasChanged(psDevInfo, psSwapChain))
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	/* return the buffer count */
	*pui32BufferCount = (IMG_UINT32)psSwapChain->uiBufferCount;

	/* return the buffers */
	for(i = 0; i < psSwapChain->uiBufferCount; ++i)
		phBuffer[i] = (IMG_HANDLE)&psSwapChain->psBuffer[i];

	IMG_DEBUG_PRINT(7, ("\t\tBufferCount : %d\n", *pui32BufferCount));
	for(i = 0; i < psSwapChain->uiBufferCount; ++i)
	{
		IMG_DEBUG_PRINT(7, ("\t\tphBuffer[%d] : 0x%08X\n", i, phBuffer[i]));
	}
	return PVRSRV_OK;
}

/* swap to buffer function, called from services */
static PVRSRV_ERROR SwapToDCBuffer(IMG_HANDLE hDevice,IMG_HANDLE hBuffer,IMG_UINT32 ui32SwapInterval,IMG_HANDLE hPrivateTag,IMG_UINT32 ui32ClipRectCount,IMG_RECT * psClipRect)
{
	IMG_DEBUG_PRINT(7, ("* %s (pid : %d)\n", __FUNCTION__, _img_osk_get_pid()));
	UNREFERENCED_PARAMETER(ui32SwapInterval);
	UNREFERENCED_PARAMETER(hPrivateTag);
	UNREFERENCED_PARAMETER(psClipRect);

	/* check parameters */
	if(!hDevice || !hBuffer || (ui32ClipRectCount != 0))
	{
		IMG_PRINT_ERROR(("Invalid Parameters.\n"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	return PVRSRV_OK;
}

/* set state function, called from services */
static IMG_VOID SetDCState(IMG_HANDLE hDevice, IMG_UINT32 ui32State)
{
	PVRCOBRA_DEVINFO *psDevInfo = (PVRCOBRA_DEVINFO *)hDevice;
	IMG_DEBUG_PRINT(7, ("* %s (pid : %d)\n", __FUNCTION__, _img_osk_get_pid()));

	switch(ui32State)
	{
	case DC_STATE_FLUSH_COMMANDS:
		IMG_DEBUG_PRINT(7, ("\t\tDC_STATE_FLUSH_COMMANDS\n"));
#if SUPPORT_PVRCOBRA_MULTI_SWAPCHAINS
		{
			IMG_UINT32 i;
			for(i = 0; i < PVRCOBRA_MAX_SWAPCHAINS; ++i)
			{
				if(psDevInfo->auiSwapChainOwner[i] == _img_osk_get_pid() &&
				   psDevInfo->apsSwapChain[i] != 0)
				{
					__CobraFlushInternalVSyncQueue(psDevInfo, psDevInfo->apsSwapChain[i]);
					break;
				}
			}
		}
#else
		if(psDevInfo->psSwapChain != 0)
			__CobraFlushInternalVSyncQueue(psDevInfo, psDevInfo->psSwapChain);
#endif
		psDevInfo->bFlushCommands = IMG_TRUE;
		break;
	case DC_STATE_NO_FLUSH_COMMANDS:
		IMG_DEBUG_PRINT(7, ("\t\tDC_STATE_NO_FLUSH_COMMANDS\n"));
		psDevInfo->bFlushCommands = IMG_FALSE;
		break;
	default:
		IMG_DEBUG_PRINT(7, ("\t\tDefault SetDCState : state(%d)\n", ui32State));
		break;
	}
}

/* 
 * cmd processing flip handler function, called from servies
 * note: in the case of no interrupts just flip and complete
 */
 //#define  COBRA_SUPPORT_FLIPV2

 
#if  1//defined(CONFIG_DSSCOMP)
 
 /* Triggered by PVRSRVSwapToDCBuffer2 */
 static IMG_BOOL ProcessFlipV2(IMG_HANDLE hCmdCookie,
							   PVRCOBRA_DEVINFO *psDevInfo,
							   PDC_MEM_INFO *ppsMemInfos,
							   IMG_UINT32 ui32NumMemInfos)
 {
	 IMG_UINT32 i, k;
	 int res;
 
	 IMG_PRINT(( "ui32NumMemInfos =%d:  \n", ui32NumMemInfos));
	 for (i=0; i < ui32NumMemInfos; i++)
		 {
			IMG_CPU_VIRTADDR virtAddr;
			IMG_CPU_PHYADDR phyAddr;
			IMG_UINT32 ui32NumPages;
			IMG_SIZE_T uByteSize;
			psDevInfo->sPVRJTable.pfnPVRSRVDCMemInfoGetByteSize(ppsMemInfos[i], &uByteSize);
			 ui32NumPages = (uByteSize + PAGE_SIZE - 1) >> PAGE_SHIFT;
 
			 psDevInfo->sPVRJTable.pfnPVRSRVDCMemInfoGetCpuPAddr(ppsMemInfos[i], 0, &phyAddr);
			 IMG_PRINT(( "uByteSize =0x%x: phyAddr	= 0x%x: \n", uByteSize, phyAddr.uiAddr));
		 }
#if 0
	 for(i = k = 0; i < ui32NumMemInfos && k < ARRAY_SIZE(asMemInfo); i++, k++)
	 {
		 struct tiler_pa_info *psTilerInfo;
		 IMG_CPU_VIRTADDR virtAddr;
		 IMG_CPU_PHYADDR phyAddr;
		 IMG_UINT32 ui32NumPages;
		 IMG_SIZE_T uByteSize;
		 int j;
 
		 psDevInfo->sPVRJTable.pfnPVRSRVDCMemInfoGetByteSize(ppsMemInfos[i], &uByteSize);
		 ui32NumPages = (uByteSize + PAGE_SIZE - 1) >> PAGE_SHIFT;
 
		 psDevInfo->sPVRJTable.pfnPVRSRVDCMemInfoGetCpuPAddr(ppsMemInfos[i], 0, &phyAddr);
 
		 /* TILER buffers do not need meminfos */
		 if(is_tiler_addr((u32)phyAddr.uiAddr))
		 {
#ifdef CONFIG_DRM_OMAP_DMM_TILER
			 enum tiler_fmt fmt;
#endif
			 asMemInfo[k].uiAddr = phyAddr.uiAddr;
#ifdef CONFIG_DRM_OMAP_DMM_TILER
			 if(tiler_get_fmt((u32)phyAddr.uiAddr, &fmt) && fmt == TILFMT_8BIT)
#else
			 if(tiler_fmt((u32)phyAddr.uiAddr) == TILFMT_8BIT)
#endif
			 {
				 psDevInfo->sPVRJTable.pfnPVRSRVDCMemInfoGetCpuPAddr(ppsMemInfos[i], (uByteSize * 2) / 3, &phyAddr);
				 asMemInfo[k].uiUVAddr = phyAddr.uiAddr;
			 }
			 continue;
		 }
 
		 /* normal gralloc layer */
		 psTilerInfo = kzalloc(sizeof(*psTilerInfo), GFP_KERNEL);
		 if(!psTilerInfo)
		 {
			 continue;
		 }
 
		 psTilerInfo->mem = kzalloc(sizeof(*psTilerInfo->mem) * ui32NumPages, GFP_KERNEL);
		 if(!psTilerInfo->mem)
		 {
			 kfree(psTilerInfo);
			 continue;
		 }
 
		 psTilerInfo->num_pg = ui32NumPages;
		 psTilerInfo->memtype = TILER_MEM_USING;
		 for(j = 0; j < ui32NumPages; j++)
		 {
			 psDevInfo->sPVRJTable.pfnPVRSRVDCMemInfoGetCpuPAddr(ppsMemInfos[i], j << PAGE_SHIFT, &phyAddr);
			 psTilerInfo->mem[j] = (u32)phyAddr.uiAddr;
		 }
 
		 /* need base address for in-page offset */
		 psDevInfo->sPVRJTable.pfnPVRSRVDCMemInfoGetCpuVAddr(ppsMemInfos[i], &virtAddr);
		 asMemInfo[k].uiAddr = (IMG_UINTPTR_T) virtAddr;
		 asMemInfo[k].psTilerInfo = psTilerInfo;
	 }
 
	 for(i = 0; i < psDssData->num_ovls; i++)
	 {
		 unsigned int ix;
		 apsTilerPAs[i] = NULL;
 
		 /* only supporting Post2, cloned and fbmem layers */
		 if (psDssData->ovls[i].addressing != OMAP_DSS_BUFADDR_LAYER_IX &&
			 psDssData->ovls[i].addressing != OMAP_DSS_BUFADDR_OVL_IX &&
			 psDssData->ovls[i].addressing != OMAP_DSS_BUFADDR_FB)
		 {
			 psDssData->ovls[i].cfg.enabled = false;
		 }
 
		 if (psDssData->ovls[i].addressing != OMAP_DSS_BUFADDR_LAYER_IX)
		 {
			 continue;
		 }
 
		 /* Post2 layers */
		 ix = psDssData->ovls[i].ba;
		 if (ix >= k)
		 {
			 IMG_DEBUG_PRINT(1, "Invalid Post2 layer (%u)", ix);
			 psDssData->ovls[i].cfg.enabled = false;
			 continue;
		 }
 
		 psDssData->ovls[i].addressing = OMAP_DSS_BUFADDR_DIRECT;
		 psDssData->ovls[i].ba = (u32) asMemInfo[ix].uiAddr;
		 psDssData->ovls[i].uv = (u32) asMemInfo[ix].uiUVAddr;
		 apsTilerPAs[i] = asMemInfo[ix].psTilerInfo;
	 }
 
	 res = dsscomp_gralloc_queue(psDssData, apsTilerPAs, false,
						   (void *)psDevInfo->sPVRJTable.pfnPVRSRVCmdComplete,
						   (void *)hCmdCookie);
	 if (res != 0)
	 {
		 IMG_PRINT(( ": %s: Device %u: dsscomp_gralloc_queue failed (%d)\n", __FUNCTION__, psDevInfo->uiFBDevID, res));
	 }
 
	 for(i = 0; i < k; i++)
	 {
		 tiler_pa_free(apsTilerPAs[i]);
	 }
#endif
	 return IMG_TRUE;
 }
 
#endif /* defined(CONFIG_DSSCOMP) */

static IMG_BOOL ProcessFlip(IMG_HANDLE hCmdCookie, IMG_UINT32 ui32DataSize, IMG_VOID *pvData)
{
 #ifdef COBRA_SUPPORT_FLIPV2
     DISPLAYCLASS_FLIP_COMMAND2 *psFlipCmd2;
 #else
	DISPLAYCLASS_FLIP_COMMAND *psFlipCmd;
 #endif
	
	PVRCOBRA_DEVINFO *psDevInfo;
	PVRCOBRA_FRAME_BUFFER *psBuffer;

	IMG_DEBUG_PRINT(7, ("%s : DataSize(%d bytes)\n", __FUNCTION__, ui32DataSize));

	/* check parameters */
	if(!hCmdCookie)
	{
		IMG_PRINT_ERROR(("Invalid Parameters.\n"));
		return IMG_FALSE;
	}

#ifdef COBRA_SUPPORT_FLIPV2
	/* validate data packet */
	psFlipCmd2 = (DISPLAYCLASS_FLIP_COMMAND2 *)pvData;
	if(psFlipCmd2 == IMG_NULL ||  sizeof(DISPLAYCLASS_FLIP_COMMAND2) != ui32DataSize)
#else
	/* validate data packet */
	psFlipCmd = (DISPLAYCLASS_FLIP_COMMAND *)pvData;
	if(psFlipCmd == IMG_NULL ||  sizeof(DISPLAYCLASS_FLIP_COMMAND) != ui32DataSize)
#endif
	{
		IMG_PRINT_ERROR(("Invalid Parameters, %d. %d\n", sizeof(DISPLAYCLASS_FLIP_COMMAND), ui32DataSize));
		return IMG_FALSE;
	}

#ifdef COBRA_SUPPORT_FLIPV2
    /* setup some useful pointers */
	psDevInfo = (PVRCOBRA_DEVINFO *)psFlipCmd2->hExtDevice;
	return ProcessFlipV2(hCmdCookie,
							 psDevInfo,
							 psFlipCmd2->ppsMemInfos,
							 psFlipCmd2->ui32NumMemInfos
							);

      psDevInfo->sPVRJTable.pfnPVRSRVCmdComplete(hCmdCookie, IMG_FALSE);

#else
	/* setup some useful pointers */
	psDevInfo = (PVRCOBRA_DEVINFO *)psFlipCmd->hExtDevice;
	psBuffer = (PVRCOBRA_FRAME_BUFFER *)psFlipCmd->hExtBuffer; /* This is the buffer we are flipping to */
	//psBuffer = (PVRCOBRA_FRAME_BUFFER *)psFlipCmd->ui32SwapInterval; /* This is the buffer we are flipping to */

	if(__CobraFlip(psDevInfo, psBuffer) != DC_OK)
	{
		IMG_PRINT_ERROR(("Failed to flip.\n"));
		return IMG_FALSE;
	}

	/* send complete command to pvrsrv */
	psDevInfo->sPVRJTable.pfnPVRSRVCmdComplete(hCmdCookie, IMG_FALSE);
#endif

	/*
	// save VSync if not complete
	*/
	// ...

	return IMG_TRUE;
}



/************************************************************
 *	init/de-init functions:
 ************************************************************/
DC_ERROR CobraInitFBDev(PVRCOBRA_DEVINFO *psDevInfo)
{
	struct fb_info *psLinkFBInfo;
	struct module  *psLinkFBOwner;
	PVRCOBRA_FBINFO *psPVRFBInfo = &psDevInfo->sFBInfo;
	IMG_UINT32 FBSize;
	IMG_UINT32 uiFBDevID = psDevInfo->uiFBDevID;

	psLinkFBInfo = registered_fb[uiFBDevID];
	if(psLinkFBInfo == NULL)
	{
		IMG_PRINT_ERROR(("%s: Device %u: Couldn't get framebuffer device\n", __FUNCTION__, uiFBDevID));
		return DC_ERROR_INVALID_DEVICE;
	}

	FBSize = (psLinkFBInfo->screen_size) != 0 ? psLinkFBInfo->screen_size
											  : psLinkFBInfo->fix.smem_len;
	if(FBSize == 0 || psLinkFBInfo->fix.line_length == 0)
	{
		IMG_PRINT_ERROR(("%s: Device %u: Invalid device information\n", __FUNCTION__, uiFBDevID));
		return DC_ERROR_INVALID_DEVICE;
	}

	psLinkFBOwner = psLinkFBInfo->fbops->owner;
	if(!try_module_get(psLinkFBOwner))
	{
		IMG_PRINT_ERROR(("%s: Device %u: Couldn't get framebuffer module\n", __FUNCTION__, uiFBDevID));
		return DC_ERROR_INVALID_DEVICE;
	}

	if(psLinkFBInfo->fbops->fb_open != NULL)
	{
		int res = psLinkFBInfo->fbops->fb_open(psLinkFBInfo, 0);
		if(res != 0)
		{
			IMG_PRINT_ERROR(("%s: Device %u: Couldn't open framebuffer(%d)\n", __FUNCTION__, uiFBDevID, res));
			module_put(psLinkFBOwner);
			return DC_ERROR_INVALID_DEVICE;
		}
	}

	psPVRFBInfo->sSysPAddr.uiAddr = psLinkFBInfo->fix.smem_start;
	psPVRFBInfo->sCPUVAddr = psLinkFBInfo->screen_base;

	psPVRFBInfo->uiWidth  = psLinkFBInfo->var.xres;
	psPVRFBInfo->uiHeight = psLinkFBInfo->var.yres;
	psPVRFBInfo->uiByteStride = psLinkFBInfo->fix.line_length;
	psPVRFBInfo->uiFBSize = FBSize;
	psPVRFBInfo->uiBufferSize = psPVRFBInfo->uiHeight * psPVRFBInfo->uiByteStride;

	//psPVRFBInfo->uiRoundedBufferSize = ALIGNSIZE(psPVRFBInfo->uiBufferSize, 2);
	// ALIGNSIZE for 4096-- chenzhu
	psPVRFBInfo->uiRoundedBufferSize = ALIGNSIZE(psPVRFBInfo->uiBufferSize, PAGE_ALIGNED_SHIFT);
	if(DC_OK != __CobraPVRPixelFormat(&psLinkFBInfo->var, &psPVRFBInfo->ePixelFormat))
	{
		IMG_DEBUG_PRINT(7, ("%s: Device %u: Unknown FB format\n", __FUNCTION__, uiFBDevID));
		psPVRFBInfo->ePixelFormat = PVRSRV_PIXEL_FORMAT_UNKNOWN;
	}


	psDevInfo->sFBInfo.uiPhysicalWidthmm = ((int)psLinkFBInfo->var.width > 0) ? psLinkFBInfo->var.width : 90;
	psDevInfo->sFBInfo.uiPhysicalHeightmm = ((int)psLinkFBInfo->var.height > 0) ? psLinkFBInfo->var.height : 54;

	psDevInfo->sFBInfo.sSysPAddr.uiAddr = psPVRFBInfo->sSysPAddr.uiAddr;
	psDevInfo->sFBInfo.sCPUVAddr = psPVRFBInfo->sCPUVAddr;

	IMG_DEBUG_PRINT(7, ("\t ***** FBDev_%d *****\n", uiFBDevID));

	IMG_DEBUG_PRINT(7, ("\t--> fb_fix_screeninfo\n"));
	IMG_DEBUG_PRINT(7, ("\t- smem_start : 0x%08X\n", psLinkFBInfo->fix.smem_start));
	IMG_DEBUG_PRINT(7, ("\t- smem_len   : %d bytes\n", psLinkFBInfo->fix.smem_len));
	IMG_DEBUG_PRINT(7, ("\t- screen_base: 0x%08X\n", psLinkFBInfo->screen_base));
	IMG_DEBUG_PRINT(7, ("\t- line_length: %d bytes\n", psLinkFBInfo->fix.line_length));	
	IMG_DEBUG_PRINT(7, ("\t- rounded buffer size : %d bytes \n", psPVRFBInfo->uiRoundedBufferSize));

	IMG_DEBUG_PRINT(7, ("\t--> fb_var_screeninfo\n"));
	IMG_DEBUG_PRINT(7, ("\t- widthmm : %d\n", psLinkFBInfo->var.width));
	IMG_DEBUG_PRINT(7, ("\t- heightmm: %d\n", psLinkFBInfo->var.height));
	IMG_DEBUG_PRINT(7, ("\t- xres  : %d\n", psLinkFBInfo->var.xres));
	IMG_DEBUG_PRINT(7, ("\t- yres  : %d\n", psLinkFBInfo->var.yres));
	IMG_DEBUG_PRINT(7, ("\t- xres_virtual : %d\n", psLinkFBInfo->var.xres_virtual));
	IMG_DEBUG_PRINT(7, ("\t- yres_virtual : %d\n", psLinkFBInfo->var.yres_virtual));
	IMG_DEBUG_PRINT(7, ("\t- xoffset : %d\n", psLinkFBInfo->var.xoffset));
	IMG_DEBUG_PRINT(7, ("\t- yoffset : %d\n", psLinkFBInfo->var.yoffset));
	IMG_DEBUG_PRINT(7, ("\t- bpp : %d\n", psLinkFBInfo->var.bits_per_pixel));

#if 0 
	if(psLinkFBInfo->fbops->fb_ioctl)
	{
		struct fb_var_screeninfo sFBVar = psLinkFBInfo->var;
		int res;

		sFBVar.reserved[0] = 0;
		sFBVar.reserved[1] = sFBVar.width;
		sFBVar.reserved[2] = sFBVar.height;
		sFBVar.reserved[3] = 0;
		sFBVar.reserved[4] = 0;

		res = psLinkFBInfo->fbops->fb_ioctl(psLinkFBInfo, 0x4663, (unsigned long)&sFBVar);
		if(res != 0)
		{
			IMG_PRINT_ERROR(("%s: fb_ioctl failed (cmd: 0x%04X, error: %d)\n", __FUNCTION__, 0x4660, res));
		}
		psDevInfo->sFBInfo.uiPanelWidth = sFBVar.reserved[1];
		psDevInfo->sFBInfo.uiPanelHeight = sFBVar.reserved[2];

		IMG_DEBUG_PRINT(7, ("\t--> panel info\n"));
		IMG_DEBUG_PRINT(7, ("\t- panel width  : %d\n", psDevInfo->sFBInfo.uiPanelWidth));
		IMG_DEBUG_PRINT(7, ("\t- panel height : %d\n", psDevInfo->sFBInfo.uiPanelHeight));
	}
	else
	{
		IMG_DEBUG_PRINT(7, ("%s: there is no fb_ioctl function.\n", __FUNCTION__));
	}
#endif
	
	psDevInfo->psLinkFBInfo = psLinkFBInfo;
	return DC_OK;
}

DC_ERROR CobraDeInitFBDev(PVRCOBRA_DEVINFO *psDevInfo)
{
	struct fb_info *psLinkFBInfo = psDevInfo->psLinkFBInfo;
	struct module  *psLinkFBOwner;
	IMG_UINT32		i;

	PVRSRV_DC_DISP2SRV_KMJTABLE * psPVRJTable = &psDevInfo->sPVRJTable;
	PVRSRV_ERROR eError;

	eError = psPVRJTable->pfnPVRSRVRemoveCmdProcList(psDevInfo->uiPVRDevID, DC_COBRA_COMMAND_COUNT);
	if (eError != PVRSRV_OK)
	{
		IMG_PRINT_ERROR(("%s: Device %u: Couldn't remove cmd proc list (error %d)\n", __FUNCTION__, psDevInfo->uiFBDevID, eError));
		return DC_ERROR_DEVICE_UNREGISTER_FAILED;
	}

	/* remove display class device from kernel services device register */
	eError = psPVRJTable->pfnPVRSRVRemoveDCDevice(psDevInfo->uiPVRDevID);
	if (eError != PVRSRV_OK)
	{
		IMG_PRINT_ERROR(("%s: Device %u: Couldn't unregister PVR device %u (error %d)\n", __FUNCTION__, psDevInfo->uiFBDevID, psDevInfo->uiPVRDevID, eError));
		return DC_ERROR_DEVICE_UNREGISTER_FAILED;
	}

	/* release backbuffer structures */
	CobraDCFreeKernelMem(psDevInfo->psBackBuffers);
	psDevInfo->psBackBuffers = NULL;

	/* release frame buffers */
	psLinkFBOwner = psLinkFBInfo->fbops->owner;
	for(i = 0; i < psDevInfo->uiNumFrameBuffers; ++i)
	{
		if(psLinkFBInfo->fbops->fb_release != NULL)
			psLinkFBInfo->fbops->fb_release(psLinkFBInfo, 0);
		module_put(psLinkFBOwner);
	}
	psDevInfo->uiNumFrameBuffers = 0;

	CobraDCFreeKernelMem((void *)psDevInfo);
	return DC_OK;
}



PVRCOBRA_DEVINFO *  CobraDevInit(IMG_UINT32 uiFBDevID)
{
	PVRCOBRA_DEVINFO * psDevInfo;
	PFN_CMD_PROC pfnCmdProcList[DC_COBRA_COMMAND_COUNT];
	IMG_UINT32	aui32SyncCountList[DC_COBRA_COMMAND_COUNT][2];
	IMG_INT32  i32NumBackBuffers, i32NumFrameBuffers;
	DC_ERROR eError;

	IMG_DEBUG_PRINT(7, ("%s\n", __FUNCTION__));
	
	/* allocate device info. structure */
	psDevInfo = (PVRCOBRA_DEVINFO *)CobraDCAllocKernelMem(sizeof(PVRCOBRA_DEVINFO));
	if(!psDevInfo)
	{
		eError = DC_ERROR_OUT_OF_MEMORY; /* failure */
		goto _error_exit;
	}

	/* reset */
	memset(psDevInfo, 0, sizeof(PVRCOBRA_DEVINFO));
	psDevInfo->uiFBDevID = uiFBDevID;

	/* got the kernel services function table */
	if((*gpfnGetPVRJTable)(&psDevInfo->sPVRJTable) == IMG_FALSE)
	{
		IMG_PRINT_ERROR(("gpfnGetPVRJTable() is not ok\n"));
		goto _error_exit;
	}

	/* fbdev init */
	if(CobraInitFBDev(psDevInfo) != DC_OK)
	{
		IMG_PRINT_ERROR(("CobraInitFBDev() is not ok\n"));
		goto _error_exit;
	}
	
	i32NumFrameBuffers = (IMG_UINT32)(psDevInfo->sFBInfo.uiFBSize / psDevInfo->sFBInfo.uiRoundedBufferSize);
	if(i32NumFrameBuffers != 0)
	{
		psDevInfo->sDisplayInfo.ui32MaxSwapChains = 1;
		psDevInfo->sDisplayInfo.ui32MaxSwapInterval = 1;
	}
	psDevInfo->sDisplayInfo.ui32MaxSwapChainBuffers = i32NumFrameBuffers;
	psDevInfo->sDisplayInfo.ui32PhysicalWidthmm = psDevInfo->sFBInfo.uiPhysicalWidthmm;
	psDevInfo->sDisplayInfo.ui32PhysicalHeightmm = psDevInfo->sFBInfo.uiPhysicalHeightmm;

	strncpy(psDevInfo->sDisplayInfo.szDisplayName, DISPLAY_DEVICE_NAME, MAX_DISPLAY_NAME_SIZE);

	psDevInfo->uiNumFormats = 1;
	psDevInfo->sDisplayFormatList[0].pixelformat = psDevInfo->sFBInfo.ePixelFormat;
	psDevInfo->psCurrentDisplayFormat = &psDevInfo->sDisplayFormatList[0];

	psDevInfo->uiNumDims = 1;
	psDevInfo->sDisplayDimList[0].ui32Width = (IMG_UINT32)psDevInfo->sFBInfo.uiWidth;
	psDevInfo->sDisplayDimList[0].ui32Height= (IMG_UINT32)psDevInfo->sFBInfo.uiHeight;
	psDevInfo->sDisplayDimList[0].ui32ByteStride = (IMG_UINT32)psDevInfo->sFBInfo.uiByteStride;
	psDevInfo->psCurrentDisplayDim = &psDevInfo->sDisplayDimList[0];

	i32NumBackBuffers = MAX(i32NumFrameBuffers - 1, 0);
	psDevInfo->uiNumFrameBuffers = MIN(i32NumFrameBuffers, PVRCOBRA_MAX_VSYNC_FLIPITEMS);
	/* set up primary buffer */
	psDevInfo->sPrimaryBuffer.sSysPAddr.uiAddr = psDevInfo->sFBInfo.sSysPAddr.uiAddr;
	psDevInfo->sPrimaryBuffer.sCPUVAddr = psDevInfo->sFBInfo.sCPUVAddr;
	psDevInfo->sPrimaryBuffer.psDevInfo = psDevInfo;
	
	IMG_DEBUG_PRINT(7, (" ** FBDev_%d Buffer Information\n", psDevInfo->uiFBDevID));
	IMG_DEBUG_PRINT(7, ("\t--> primary buffer\n"));
	IMG_DEBUG_PRINT(7, ("\t - physical : 0x%08X\n", (IMG_UINT32)psDevInfo->sPrimaryBuffer.sSysPAddr.uiAddr));
	IMG_DEBUG_PRINT(7, ("\t - virtual  : 0x%08X\n", (IMG_UINT32)psDevInfo->sPrimaryBuffer.sCPUVAddr));

	/* set up back buffers */
	if(i32NumBackBuffers > 0)
	{
		IMG_UINT32 i;

		psDevInfo->psBackBuffers = (PVRCOBRA_FRAME_BUFFER *)CobraDCAllocKernelMem(sizeof(PVRCOBRA_FRAME_BUFFER) * i32NumBackBuffers);
		if(!psDevInfo->psBackBuffers)
		{	
			IMG_DEBUG_PRINT(7, ("%s: Out of memory: we need to allocate kernel memory; %d bytes\n", __FUNCTION__, sizeof(PVRCOBRA_FRAME_BUFFER)*i32NumBackBuffers));
		}
		memset(psDevInfo->psBackBuffers, 0, sizeof(PVRCOBRA_FRAME_BUFFER) * i32NumBackBuffers);

		for(i = 0; i < i32NumBackBuffers; ++i)
		{
			PVRCOBRA_FRAME_BUFFER * psBackBuffer = &psDevInfo->psBackBuffers[i];
			psBackBuffer->sSysPAddr.uiAddr = psDevInfo->sFBInfo.sSysPAddr.uiAddr + ((i+1) * psDevInfo->sFBInfo.uiRoundedBufferSize);
			psBackBuffer->sCPUVAddr = (IMG_CPU_VIRTADDR)((IMG_UINT32)psDevInfo->sFBInfo.sCPUVAddr + ((i+1) * psDevInfo->sFBInfo.uiRoundedBufferSize));
			psBackBuffer->psDevInfo = psDevInfo;

			IMG_DEBUG_PRINT(7, ("\t--> back buffer : %d\n", i));
			IMG_DEBUG_PRINT(7, ("\t - physical : 0x%08X\n", (IMG_UINT32)psBackBuffer->sSysPAddr.uiAddr));
			IMG_DEBUG_PRINT(7, ("\t - virtual  : 0x%08X\n", (IMG_UINT32)psBackBuffer->sCPUVAddr));
		}	
	}	
	
	/* setup the DC Jtable so SRVKM can call into this driver */
	{
		PVRSRV_DC_SRV2DISP_KMJTABLE *psDCJTable = &psDevInfo->sDCJTable;
		psDCJTable->ui32TableSize = sizeof(PVRSRV_DC_SRV2DISP_KMJTABLE);
		psDCJTable->pfnOpenDCDevice			= OpenDCDevice;
		psDCJTable->pfnCloseDCDevice		= CloseDCDevice;
		psDCJTable->pfnEnumDCFormats		= EnumDCFormats;
		psDCJTable->pfnEnumDCDims			= EnumDCDims;
		psDCJTable->pfnGetDCSystemBuffer	= GetDCSystemBuffer;
		psDCJTable->pfnGetDCInfo			= GetDCInfo;
		psDCJTable->pfnGetBufferAddr		= GetDCBufferAddr;
		psDCJTable->pfnCreateDCSwapChain	= CreateDCSwapChain;
		psDCJTable->pfnDestroyDCSwapChain	= DestroyDCSwapChain;
		psDCJTable->pfnSetDCDstRect			= SetDCDstRect;
		psDCJTable->pfnSetDCSrcRect			= SetDCSrcRect;
		psDCJTable->pfnSetDCDstColourKey	= SetDCDstColourKey;
		psDCJTable->pfnSetDCSrcColourKey	= SetDCSrcColourKey;
		psDCJTable->pfnGetDCBuffers			= GetDCBuffers;
		psDCJTable->pfnSwapToDCBuffer		= SwapToDCBuffer;
		psDCJTable->pfnSetDCState			= SetDCState;
		//psDCJTable->pfnQuerySwapCommandID	= IMG_NULL;
	}
	
	/* register device with services and retrieve device index */
	if(psDevInfo->sPVRJTable.pfnPVRSRVRegisterDCDevice(
									&psDevInfo->sDCJTable, &psDevInfo->uiPVRDevID) != PVRSRV_OK)
	{
		IMG_PRINT_ERROR(("pfnPVRSRVRegisterDCDevice's been failed\n"));
		goto _error_remove_device_exit;
	}

	/* setup private command processing function table */
	pfnCmdProcList[DC_FLIP_COMMAND] = ProcessFlip;

	/* and associated sync count(s) */
	aui32SyncCountList[DC_FLIP_COMMAND][0] = 0; /* no writes */
	aui32SyncCountList[DC_FLIP_COMMAND][1] = 2; /* 2 reads: To / From */

	/*
	 * register private command processing functions with the Comand Queue Manager and
	 * setup the general command complete function in the devinfo
	 */
	if(psDevInfo->sPVRJTable.pfnPVRSRVRegisterCmdProcList(
									psDevInfo->uiPVRDevID, &pfnCmdProcList[0], aui32SyncCountList,
									DC_COBRA_COMMAND_COUNT) != PVRSRV_OK) 
	{
		IMG_PRINT_ERROR(("pfnPVRSRVRegisterCmdProcList's been failed\n"));
		goto _error_remove_device_exit;
	}
	return psDevInfo;

_error_remove_device_exit:
	(void)psDevInfo->sPVRJTable.pfnPVRSRVRemoveDCDevice(psDevInfo->uiPVRDevID);
_error_exit:
	if(psDevInfo)
		CobraDCFreeKernelMem(psDevInfo);
	return NULL;
}

DC_ERROR DCInit(void)
{
	IMG_UINT32 i, uiDevices = 0;
	IMG_UINT32 num_fb_devs = 1;//num_registered_fb;
	
	/*
		- connect to services
		- register with services
		- allocate and setup private data structure
	*/
	
	/*
		in kernel driver, data structures must be anchored to something for subsequent retrieval
		this may be a single global pointer or TLS or something else - up to you
		call API to retrieve this ptr
	*/
	if(num_fb_devs == 0)
	{
		IMG_PRINT_ERROR(("The framebuffer devices in this system are 0"));
		return DC_ERROR_INIT_FAILURE;
	}

	if(DCGetLibFuncAddr("PVRGetDisplayClassJTable", &gpfnGetPVRJTable) != DC_OK)
	{
		IMG_PRINT_ERROR(("DCGetLibFuncAddr has been failed"));
		return DC_ERROR_INIT_FAILURE;
	}
	for(i = 0; i < num_fb_devs; ++i)
	{
		PVRCOBRA_DEVINFO * psDevInfo = CobraDevInit(i);
		
		if(psDevInfo != NULL)
		{
			CobraDCSetDevInfoPtr(psDevInfo->uiFBDevID, psDevInfo);
			uiDevices++;
		} 
	}

	return (uiDevices != 0) ? DC_OK : DC_ERROR_INIT_FAILURE;
}

DC_ERROR DCDeInit(void)
{
	IMG_UINT32 i;
	DC_ERROR eError = DC_OK;

	for(i = 0; i < DC_MAX_NUM_FB_DEVICES; ++i)
	{
		PVRCOBRA_DEVINFO *psDevInfo = CobraDCGetDevInfoPtr(i);
		if(psDevInfo)
		{
			/* reset DevInfoPtr */
			CobraDCSetDevInfoPtr(psDevInfo->uiFBDevID, NULL);

			/* Deinit DevInfo */
			eError |= CobraDeInitFBDev(psDevInfo);
		}
	}

	/* return success */
	return eError;
}

/******************************************************************************
 End of file (dc_cobra_displayclass.c)
******************************************************************************/
