/*!****************************************************************************
@File			dc_cobra.h

@Title			Dummy 3rd party display driver structures and prototypes

@Author			Billy Park

@date   			1/10/2012

@Copyright     	Copyright 2016 by MediaTek Limited. All rights reserved.

@Platform		cobra

@Description	Dummy 3rd party display driver structures and prototypes

@DoxygenVer

******************************************************************************/

/******************************************************************************
Modifications :-

$Log: dc_cobra.h $
******************************************************************************/

/**************************************************************************
 The 3rd party driver is a specification of an API to integrate the
 IMG PowerVR Services driver with 3rd Party display hardware.
 It is NOT a specification for a display controller driver, rather a
 specification to extend the API for a pre-existing driver for the display hardware.

 The 3rd party driver interface provides IMG PowerVR client drivers (e.g. PVR2D)
 with an API abstraction of the system’s underlying display hardware, allowing
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

#ifndef __DC_COBRA_H__
#define __DC_COBRA_H__


#if defined(__cplusplus)
extern "C" {
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,38))
  #define PVRCOBRA_CONSOLE_LOCK()		console_lock()
  #define PVRCOBRA_CONSOLE_UNLOCK()		console_unlock()
#else
  #define PVRCOBRA_CONSOLE_LOCK()		acquire_console_sem()
  #define PVRCOBRA_CONSOLE_UNLOCK()		release_console_sem()
#endif

extern IMG_BOOL IMG_IMPORT PVRGetDisplayClassJTable(PVRSRV_DC_DISP2SRV_KMJTABLE * psJTable);

#define SUPPORT_PVRCOBRA_OWN_LINUX_FB		0
#define SUPPORT_PVRCOBRA_MULTI_SWAPCHAINS	0

/* maximum number of backbuffers */
#define PVRCOBRA_MAX_BACKBUFFERS (4)
#define PVRCOBRA_MAX_SWAPCHAINS  (8)

/* maximum number of Vsync flip items */
#define PVRCOBRA_MAX_VSYNC_FLIPITEMS 	(4)
#define PAGE_ALIGNED_SHIFT   (12)

#define PVRCOBRA_MAX_FORMATS	 (1)
#define PVRCOBRA_MAX_DIMS		 (1)

#define DC_FALSE	false
#define DC_TRUE		true

typedef void * DC_HANDLE;
typedef atomic_t	DC_ATOMIC_BOOL;
typedef atomic_t	DC_ATOMIC_INT;

typedef struct PVRCOBRA_FRAME_BUFFER_TAG
{
	/* handle to swapchain */
	IMG_HANDLE					hSwapChain;

	/* member using IMG structures to minimise API function code */
	/* replace with own structures where necessary */
	/* system physical address of the buffer */
	IMG_SYS_PHYADDR				sSysPAddr;
	IMG_CPU_VIRTADDR			sCPUVAddr;

	/*
		Offset of start of this buffer from Base of Stolen
        Memory (BSM). This is the value to which we will set
        DSPALINOFF when we flip to this buffer.
	*/
	IMG_INT32					iYOffset;
	
	/*
		ptr to synchronisation information structure
		associated with the buffer
	*/
	PVRSRV_SYNC_DATA*			psSyncData;

	/* ptr to next buffer in a swapchain */
	struct PVRCOBRA_FRAME_BUFFER_TAG	*psNext;
	struct PVRCOBRA_DEVINFO_TAG *psDevInfo;

	DC_HANDLE					hCmdComplete;
	IMG_UINT32					uiSwapInterval;

} PVRCOBRA_FRAME_BUFFER;


/* PVRCOBRA buffer structure */
typedef struct PVRCOBRA_SWAPCHAIN_TAG
{
	/* id of swap chain */
	IMG_UINT32			uiSwapChainID;
	/* number of buffers in swapchain */
	IMG_UINT32			uiBufferCount;
	/* list of buffers in the swapchain */
	PVRCOBRA_FRAME_BUFFER 	*psBuffer;

	IMG_BOOL			bNotVSynced;

	IMG_UINT32			uiFBDevID;

} PVRCOBRA_SWAPCHAIN;


/* PVRPDP display mode information */
typedef struct PVRPSB_FBINFO_TAG
{
	/* pixel width of system/primary surface */
	IMG_UINT32			uiWidth;
	/* pixel height of system/primary surface */
	IMG_UINT32			uiHeight;
	/* byte stride of system/primary surface */
	IMG_UINT32			uiByteStride;
    /* Size in bytes of each surface */
    IMG_UINT32			uiSurfaceSize;
	/* pixelformat of system/primary surface */
	PVRSRV_PIXEL_FORMAT	ePixelFormat;

	/* members using IMG structures to minimise API function code */
	/* replace with own structures where necessary */
	/* system physical address of system/primary surface */
	IMG_SYS_PHYADDR		sSysPAddr;
	
    /* offset of system/primary surface from Base of Stolen Memory*/
    IMG_UINT32			uiDisplayOffset;

	/* cpu virtual address of system/primary surface */
	IMG_CPU_VIRTADDR	sCPUVAddr;
	
	/* system physical address of display device's registers */
	IMG_SYS_PHYADDR		sRegSysPAddr;

	IMG_UINT32			uiFBSize; /* total fb size */
	IMG_UINT32			uiBufferSize; /* single frame size */
	IMG_UINT32			uiRoundedBufferSize;

	IMG_UINT32			uiPhysicalWidthmm;
	IMG_UINT32			uiPhysicalHeightmm;

	IMG_BOOL			bNeedScaleUp;
	IMG_BOOL			bNeedScaleDown;
	IMG_UINT32			uiPanelWidth;
	IMG_UINT32			uiPanelHeight;
	
}PVRCOBRA_FBINFO;

#if 1
/* flip item structure used for queuing of flips */
typedef struct PVRCOBRA_VSYNC_FLIP_ITEM_TAG
{
	/*
		command complete cookie to be passed to services
		command complete callback function
	*/
	IMG_HANDLE        hCmdComplete;

	PVRCOBRA_FRAME_BUFFER 	*psBuffer;

    /* Value to set DSPALINOFF to, to flip to this item flip */
    IMG_UINT32		uiDisplayOffset;
	/* swap interval between flips */
	IMG_UINT32		uiSwapInterval;
	/* is this item valid? */
	IMG_BOOL		bValid;
	/* has this item been flipped? */
	IMG_BOOL		bFlipped;
	/* has the flip cmd completed? */
	IMG_BOOL		bCmdCompleted;
} PVRCOBRA_VSYNC_FLIP_ITEM;
#endif

/* kernel device information structure */
typedef struct PVRCOBRA_DEVINFO_TAG
{
	/* device ID assigned by services */
	IMG_UINT32				uiFBDevID;
	IMG_UINT32				uiPVRDevID;

	/* system surface info */
	IMG_UINT32				uiNumFrameBuffers;
	PVRCOBRA_FRAME_BUFFER	sPrimaryBuffer;

	/* back buffer info */
	PVRCOBRA_FRAME_BUFFER	*psBackBuffers;

	/* display format */
	IMG_UINT32				uiNumFormats;
	DISPLAY_FORMAT			sDisplayFormatList[PVRCOBRA_MAX_FORMATS];
	DISPLAY_FORMAT			*psCurrentDisplayFormat;

	/* display dim */
	IMG_UINT32				uiNumDims;
	DISPLAY_DIMS			sDisplayDimList[PVRCOBRA_MAX_DIMS];
	DISPLAY_DIMS			*psCurrentDisplayDim;

	/* set of vsync flip items - enough for 1 outstanding flip per back buffer */
	PVRCOBRA_VSYNC_FLIP_ITEM	asVSyncFlips[PVRCOBRA_MAX_VSYNC_FLIPITEMS];

	/* insert index for the internal queue of flip items */
	IMG_UINT32				uiInsertIndex;

	/* remove index for the internal queue of flip items */
	IMG_UINT32				uiRemoveIndex;

	/* display mode information */
	PVRCOBRA_FBINFO			sFBInfo;

	/* ref count on this structure */
	IMG_UINT32				uiRefCount;

#if SUPPORT_PVRCOBRA_MULTI_SWAPCHAINS
	/* several swapchains supported by this device */
	PVRCOBRA_SWAPCHAIN		*apsSwapChain[PVRCOBRA_MAX_SWAPCHAINS];
	IMG_UINT32				auiSwapChainOwner[PVRCOBRA_MAX_SWAPCHAINS];
#else
	/* only one swapchain supported by this device so hang it here */
	PVRCOBRA_SWAPCHAIN		*psSwapChain;
#endif

	/* jump table into PVR services */
	PVRSRV_DC_DISP2SRV_KMJTABLE		sPVRJTable;

	/* jump table into DC */
	PVRSRV_DC_SRV2DISP_KMJTABLE		sDCJTable;

	/* OS registration handle */
	IMG_HANDLE				hOSHandle;
	/* True if PVR is flushing its command queues */
	IMG_BOOL				bFlushCommands;

	/* members using IMG structures to minimise API function code */
	/* replace with own structures where necessary */

	struct fb_info *		psLinkFBInfo;

	/* misc. display information */
	DISPLAY_INFO			sDisplayInfo;

    /* offset of surface being displayed, from Base of Stolen Memory */
    IMG_UINT32				uiDisplayOffset;

	/* Offset of the first display surface after the start of device mem */
	IMG_UINT32				uiSysSurfaceOffset;

    IMG_UINT32				uiMaxBackBuffers;
#ifdef SUPPORT_HW_CURSOR
	PVRSRV_CURSOR_INFO  sCursorInfo;
#endif  //SUPPORT_HW_CURSOR

}  PVRCOBRA_DEVINFO;


/*!
 *****************************************************************************
 * Error values
 *****************************************************************************/

#ifndef UNREFERENCED_PARAMETER
#define	UNREFERENCED_PARAMETER(param) (param) = (param)
#endif

/* Util APIs */
PVRCOBRA_DEVINFO * CobraDCGetDevInfoPtr(IMG_UINT32 uiFBDevID);
void CobraDCSetDevInfoPtr(IMG_UINT32 uiFBDevID, PVRCOBRA_DEVINFO *psDevInfo);

void * CobraDCAllocKernelMem(IMG_UINT32 ui32Size);
void   CobraDCFreeKernelMem(void * pvMem);

DC_ERROR DCGetLibFuncAddr(char * szFunctionName,PFN_DC_GET_PVRJTABLE * ppfnFuncTable);

#if defined(__cplusplus)
}
#endif

#endif /* __DC_COBRA_H__ */

/******************************************************************************
 End of file (dc_cobra.h)
******************************************************************************/

