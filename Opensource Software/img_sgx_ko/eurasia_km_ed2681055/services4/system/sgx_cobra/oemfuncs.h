/*!****************************************************************************
@File			sgxkmif.h

@Title			SGX kernel/client driver interface structures and prototypes

@Author			Imagination Technologies

@date   		1 / 10 / 2004
 
@Copyright     	Copyright 2003-2005 by Imagination Technologies Limited.
                All rights reserved. No part of this software, either
                material or conceptual may be copied or distributed,
                transmitted, transcribed, stored in a retrieval system
                or translated into any human or computer language in any
                form by any means, electronic, mechanical, manual or
                other-wise, or disclosed to third parties without the
                express written permission of Imagination Technologies
                Limited, Unit 8, HomePark Industrial Estate,
                King's Langley, Hertfordshire, WD4 8LZ, U.K.

@Platform		Cobra

@Description	

@DoxygenVer		

******************************************************************************/

/******************************************************************************
Modifications :-

$Log: oemfuncs.h $
******************************************************************************/

#if !defined(__OEMFUNCS_H__)
#define __OEMFUNCS_H__

#if defined (__cplusplus)
extern "C" {
#endif

/* function identifiers: */
#define OEM_EXCHANGE_POWER_STATE	(1<<0)
#define OEM_DEVICE_MEMORY_POWER		(1<<1)
#define OEM_DISPLAY_POWER			(1<<2)
#define OEM_GET_EXT_FUNCS			(1<<3)

typedef struct OEM_ACCESS_INFO_TAG
{
	IMG_UINT32		ui32Size;
	IMG_UINT32  	ui32FBPhysBaseAddress;
	IMG_UINT32		ui32FBMemAvailable;		/* size of usable FB memory */
	IMG_UINT32  	ui32SysPhysBaseAddress;
	IMG_UINT32		ui32SysSize;
	IMG_UINT32		ui32DevIRQ;
} OEM_ACCESS_INFO, *POEM_ACCESS_INFO; 
 
/* function in/out data structures: */
typedef IMG_UINT32   (*PFN_SRV_BRIDGEDISPATCH)( IMG_UINT32  Ioctl,
												IMG_BYTE   *pInBuf,
												IMG_UINT32  InBufLen, 
											    IMG_BYTE   *pOutBuf,
												IMG_UINT32  OutBufLen,
												IMG_UINT32 *pdwBytesTransferred);


typedef PVRSRV_ERROR (*PFN_SRV_READREGSTRING)(PPVRSRV_REGISTRY_INFO psRegInfo);

/*
	Function table for kernel 3rd party driver to kernel services
*/
typedef struct PVRSRV_DC_OEM_JTABLE_TAG
{
	PFN_SRV_BRIDGEDISPATCH			pfnOEMBridgeDispatch;
	PFN_SRV_READREGSTRING			pfnOEMReadRegistryString;
	PFN_SRV_READREGSTRING			pfnOEMWriteRegistryString;

} PVRSRV_DC_OEM_JTABLE;
#if defined(__cplusplus)
}
#endif

#endif	/* __OEMFUNCS_H__ */

/*****************************************************************************
 End of file (oemfuncs.h)
*****************************************************************************/


