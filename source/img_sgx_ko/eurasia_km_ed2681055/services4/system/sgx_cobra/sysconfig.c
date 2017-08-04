/*!****************************************************************************
@File			sysconfig.c

@Title			System Description

@Author			Imagination Technologies

@date  		09/04/10 

@Copyright     	Copyright 2010 by Imagination Technologies Limited.
                All rights reserved. No part of this software, either
                material or conceptual may be copied or distributed,
                transmitted, transcribed, stored in a retrieval system
                or translated into any human or computer language in any
                form by any means, electronic, mechanical, manual or
                other-wise, or disclosed to third parties without the
                express written permission of Imagination Technologies
                Limited, Unit 8, HomePark Industrial Estate,
                King's Langley, Hertfordshire, WD4 8LZ, U.K.

@Platform		generic

@Description	System Description functions

@DoxygenVer

******************************************************************************/

/******************************************************************************
Modifications :-

$Log: sysconfig.c $
*****************************************************************************/

#include <asm/io.h>
#include <asm/page.h>

#include "sgxdefs.h"
#include "services_headers.h"
#include "kerneldisplay.h"
#include "oemfuncs.h"
#include "sgxinfo.h"
#include "sgxinfokm.h"
#include "syslocal.h"

#include <linux/time.h>
#include <asm/delay.h>
#include "ocpdefs.h"


/* top level system data anchor point*/
SYS_DATA* gpsSysData = (SYS_DATA*)IMG_NULL;
static SYS_DATA  gsSysData;

SYS_SPECIFIC_DATA * gpsSysSpecificData = (SYS_SPECIFIC_DATA *)IMG_NULL;
static SYS_SPECIFIC_DATA gsSysSpecificData;

/* SGX structures */
static IMG_UINT32 gui32SGXDeviceID;
static SGX_DEVICE_MAP	gsSGXDeviceMap;
static PVRSRV_DEVICE_NODE *gpsSGXDevNode;

#if defined(PDUMP)
/* memory space name for device memory */
#if !defined(SYSTEM_PDUMP_NAME_SGXMEM)
#define SYSTEM_PDUMP_NAME "SYSMEM"
#else
#define SYSTEM_PDUMP_NAME "SGXMEM"
#endif
#endif

#if defined(NO_HARDWARE)
/* mimic register block with contiguous memory */
static IMG_CPU_VIRTADDR gsSGXRegsCPUVAddr;
#endif

#if !defined(SUPPORT_DRI_DRM_EXT)
IMG_UINT32 PVRSRV_BridgeDispatchKM( IMG_UINT32  Ioctl,
									IMG_BYTE   *pInBuf,
									IMG_UINT32  InBufLen,
									IMG_BYTE   *pOutBuf,
									IMG_UINT32  OutBufLen,
									IMG_UINT32 *pdwBytesTransferred);
#endif
static  PVRSRV_ERROR EnableSGXClocksWrap(SYS_DATA *psSysData);

/*!
******************************************************************************

 @Function	SysLocateDevices

 @Description specifies devices in the systems memory map

 @Input    psSysData - sys data

 @Return   PVRSRV_ERROR  :

******************************************************************************/
static PVRSRV_ERROR SysLocateDevices(SYS_DATA *psSysData)
{
#if !defined(NO_HARDWARE) || defined(__linux__)
	IMG_UINT32 ui32RegBasePAddr = 0;
	IMG_UINT32 ui32IRQ = 0;
#endif
#if (!defined(__linux__) || defined(NO_HARDWARE))
	PVRSRV_ERROR eError;
#endif
	
	PVR_TRACE(("%s", __FUNCTION__));

#ifdef __linux__
	ui32RegBasePAddr = SGX_REG_BASE_ADDRESS;
	ui32IRQ = SGX_IRQ;

	PVR_TRACE(("Register Base : 0x%08X", ui32RegBasePAddr));
	PVR_TRACE(("IRQ: %d", ui32IRQ));
#endif	/* __linux__ */

	/* SGX Device: */
	gsSGXDeviceMap.ui32Flags = 0x0;
	gsSGXDeviceMap.ui32IRQ = ui32IRQ;

	/* Registers */
	gsSGXDeviceMap.sRegsSysPBase.uiAddr = ui32RegBasePAddr;
	gsSGXDeviceMap.sRegsCpuPBase = SysSysPAddrToCpuPAddr(gsSGXDeviceMap.sRegsSysPBase);
	gsSGXDeviceMap.ui32RegsSize = SGX_REG_SIZE;

	/* 
		Local Device Memory Region: (not present)
		Note: the device doesn't need to know about its memory 
		but keep info here for now
	*/
#if 0
	gsSGXDeviceMap.sLocalMemSysPBase.uiAddr = 0x40000000;
	gsSGXDeviceMap.sLocalMemDevPBase.uiAddr = 0x40000000;
	gsSGXDeviceMap.sLocalMemCpuPBase.uiAddr = 0x40000000;
	gsSGXDeviceMap.ui32LocalMemSize = 0x2000000; // 32MB
#else
	gsSGXDeviceMap.sLocalMemSysPBase.uiAddr = 0;
	gsSGXDeviceMap.sLocalMemDevPBase.uiAddr = 0;
	gsSGXDeviceMap.sLocalMemCpuPBase.uiAddr = 0;
	gsSGXDeviceMap.ui32LocalMemSize = 0;
#endif

#if defined(PDUMP)
	{
		/* initialise memory region name for pdumping */
		static IMG_CHAR pszPDumpDevName[] = SYSTEM_PDUMP_NAME;
		gsSGXDeviceMap.pszPDumpDevName = pszPDumpDevName;
	}
#endif

	/************************************************
		add other devices here:
	*************************************************/

	return PVRSRV_OK;
}

#ifdef __linux__
/*!
******************************************************************************

 @Function		SysCreateVersionString

 @Description

 Generate a device version string

 @Input		psSysData :	System data

 @Return	PVRSRV_ERROR

******************************************************************************/
#define VERSION_STR_MAX_LEN_TEMPLATE "SGX revision = 000.000.000"
static PVRSRV_ERROR SysCreateVersionString(SYS_DATA *psSysData)
{
    IMG_UINT32 ui32MaxStrLen;
    PVRSRV_ERROR eError;
    IMG_INT32 i32Count;
    IMG_CHAR *pszVersionString;
    IMG_UINT32 ui32SGXRevision = 0;

	PVR_TRACE(("%s", __FUNCTION__));
#if !defined(NO_HARDWARE)
    {
	IMG_VOID *pvSGXRegs;

	pvSGXRegs = OSMapPhysToLin(gsSGXDeviceMap.sRegsCpuPBase,
											 gsSGXDeviceMap.ui32RegsSize,
											 PVRSRV_HAP_KERNEL_ONLY|PVRSRV_HAP_UNCACHED,
											 IMG_NULL);

	if (pvSGXRegs != IMG_NULL)
	{
         ui32SGXRevision = OSReadHWReg(pvSGXRegs, EUR_CR_CORE_REVISION);
	     OSUnMapPhysToLin(pvSGXRegs,
		   						 	gsSGXDeviceMap.ui32RegsSize,
								 	PVRSRV_HAP_KERNEL_ONLY|PVRSRV_HAP_UNCACHED,
									IMG_NULL);
	}
	else
	{
	     PVR_DPF((PVR_DBG_ERROR,"SysCreateVersionString: Couldn't map SGX registers"));
	}
    }
#endif

    ui32MaxStrLen = OSStringLength(VERSION_STR_MAX_LEN_TEMPLATE);
    eError = OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP,
                          ui32MaxStrLen + 1,
                          (IMG_PVOID *)&pszVersionString,
                          IMG_NULL,
			  "Version String");
    if(eError != PVRSRV_OK)
    {
		return eError;
    }

    i32Count = OSSNPrintf(pszVersionString, ui32MaxStrLen + 1,
                           "SGX revision = %u.%u.%u",
                           (IMG_UINT)((ui32SGXRevision & EUR_CR_CORE_REVISION_MAJOR_MASK)
                            >> EUR_CR_CORE_REVISION_MAJOR_SHIFT),
                           (IMG_UINT)((ui32SGXRevision & EUR_CR_CORE_REVISION_MINOR_MASK)
                            >> EUR_CR_CORE_REVISION_MINOR_SHIFT),
                           (IMG_UINT)((ui32SGXRevision & EUR_CR_CORE_REVISION_MAINTENANCE_MASK)
                            >> EUR_CR_CORE_REVISION_MAINTENANCE_SHIFT)
                           );
    if(i32Count == -1)
    {
        ui32MaxStrLen = OSStringLength(VERSION_STR_MAX_LEN_TEMPLATE);
        OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP,
                    ui32MaxStrLen + 1,
                    pszVersionString,
                    IMG_NULL);
		/*not nulling pointer, out of scope*/
		return PVRSRV_ERROR_INVALID_PARAMS;
    }

    psSysData->pszVersionString = pszVersionString;

    return PVRSRV_OK;
}

/*!
******************************************************************************

 @Function		SysFreeVersionString
 
 @Description

 Free the device version string
 
 @Input		psSysData :	System data

 @Return	none

******************************************************************************/
static IMG_VOID SysFreeVersionString(SYS_DATA *psSysData)
{ 
	PVR_TRACE(("%s", __FUNCTION__));
    if(psSysData->pszVersionString)
    {
        IMG_UINT32 ui32MaxStrLen;
        ui32MaxStrLen = OSStringLength(VERSION_STR_MAX_LEN_TEMPLATE);
        OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP,
                    ui32MaxStrLen+1,
                    psSysData->pszVersionString,
                    IMG_NULL);
		psSysData->pszVersionString = IMG_NULL;
    }
}

/*!
******************************************************************************

 @Function		SysResetCoreClk
 
 @Description

 Initialise IMG core clk
 
 @Input		psSysData :	System data

 @Return	none

******************************************************************************/
IMG_VOID SysResetCoreClk(SYS_DATA *psSysData)
{
	IMG_CPU_PHYADDR sPLLRegsCpuPBase = {0xF000D000};
	//IMG_CPU_PHYADDR sCFGRegsCpuPBase = {0xF004E000};
	IMG_VOID *pvPLLRegs = OSMapPhysToLin(sPLLRegsCpuPBase, PAGE_SIZE,
											PVRSRV_HAP_KERNEL_ONLY|PVRSRV_HAP_UNCACHED,
											IMG_NULL);
	IMG_VOID *pvSGXRegs = OSMapPhysToLin(gsSGXDeviceMap.sRegsCpuPBase,
											gsSGXDeviceMap.ui32RegsSize,
											PVRSRV_HAP_KERNEL_ONLY|PVRSRV_HAP_UNCACHED,
											IMG_NULL);

#if 0 
	IMG_VOID *pvCFGRegs = OSMapPhysToLin(sCFGRegsCpuPBase, PAGE_SIZE,
											PVRSRV_HAP_KERNEL_ONLY|PVRSRV_HAP_UNCACHED,
											IMG_NULL);
	unsigned int cfg = 0;

	OSWriteHWReg(pvCFGRegs, 0x0000, 0x20010011);
	OSWriteHWReg(pvSGXRegs, 0x4004, 0x00000055);
	OSWriteHWReg(pvSGXRegs, 0x4020, 0x00000155);
	udelay(1);
	OSWriteHWReg(pvSGXRegs, 0x0000, 0x00155555);
	OSWriteHWReg(pvSGXRegs, 0x0004, 0x05454555);
	udelay(1);

	cfg = OSReadHWReg(pvCFGRegs, 0x0000);

	// sequence to operate IMG
	OSWriteHWReg(pvCFGRegs, 0x0000, (1<<1) | cfg);
	udelay(1);

	// clock power down
	OSWriteHWReg(pvPLLRegs, 0x03D8, (1<<7) | 0x1);
	OSWriteHWReg(pvPLLRegs, 0x03D4, (1<<7) | 0x9);

	OSWriteHWReg(pvCFGRegs, 0x0000, cfg);
	udelay(1);

	// clock power up
	OSWriteHWReg(pvPLLRegs, 0x03D8, 0x1);
	udelay(1);
	OSWriteHWReg(pvPLLRegs, 0x03D4, 0x9);
	udelay(1);
	
	OSUnMapPhysToLin(pvSGXRegs, gsSGXDeviceMap.ui32RegsSize, 
							PVRSRV_HAP_KERNEL_ONLY|PVRSRV_HAP_UNCACHED, 
							IMG_NULL);
	OSUnMapPhysToLin(pvCFGRegs, PAGE_SIZE,
							PVRSRV_HAP_KERNEL_ONLY|PVRSRV_HAP_UNCACHED,
							IMG_NULL);
	OSUnMapPhysToLin(pvPLLRegs, PAGE_SIZE,
							PVRSRV_HAP_KERNEL_ONLY|PVRSRV_HAP_UNCACHED,
							IMG_NULL);
#else
	// enable to operate IMG gpu
	//OSWriteHWReg(pvSGXRegs, 0x4004, 0x00000002);
	//OSWriteHWReg(pvSGXRegs, 0x4020, 0x000000AA);

	OSUnMapPhysToLin(pvSGXRegs, gsSGXDeviceMap.ui32RegsSize, 
							PVRSRV_HAP_KERNEL_ONLY|PVRSRV_HAP_UNCACHED, 
							IMG_NULL);
		
	// set pll for img core
	OSWriteHWReg(pvPLLRegs, 0x03D4, 0x9); // syspll/2
	//OSWriteHWReg(pvPLLRegs, 0x03D4, 0x1); // g3dpll
	OSWriteHWReg(pvPLLRegs, 0x03D8, 0x1); // g3dpll

	OSUnMapPhysToLin(pvPLLRegs, PAGE_SIZE,
							PVRSRV_HAP_KERNEL_ONLY|PVRSRV_HAP_UNCACHED,
							IMG_NULL);
#endif

}

static INLINE PVRSRV_ERROR EnableSystemClocksWrap(SYS_DATA *psSysData)
{
	PVRSRV_ERROR eError = EnableSystemClocks(psSysData);
	return eError;
}

#endif /* __linux__ */

/*!
******************************************************************************

 @Function	SysInitialise

 @Description Initialises kernel services at 'driver load' time

 @Return   PVRSRV_ERROR  :

******************************************************************************/
PVRSRV_ERROR SysInitialise(IMG_VOID)
{
	IMG_BOOL			bUseLocalMem = IMG_FALSE;
	IMG_UINT32			i = 0;
	PVRSRV_ERROR 		eError;
	PVRSRV_DEVICE_NODE	*psDeviceNode;
	SGX_TIMING_INFORMATION*	psTimingInfo;

	PVR_TRACE(("%s", __FUNCTION__));

	gpsSysData = &gsSysData;
	OSMemSet(gpsSysData, 0, sizeof(SYS_DATA));

	gpsSysSpecificData = &gsSysSpecificData;
	OSMemSet(&gsSysSpecificData, 0, sizeof(SYS_SPECIFIC_DATA));
	
	gpsSysData->pvSysSpecificData = gpsSysSpecificData;

	/* Allocate env data area and initialize it */
	eError = OSInitEnvData(&gpsSysData->pvEnvSpecificData);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SysInitialise: Failed to setup env structure"));
		SysDeinitialise(gpsSysData);
		gpsSysData = IMG_NULL;
		return eError;
	}

	/* Set up timing information*/
	#if !defined(SGX_DYNAMIC_TIMING_INFO)
	psTimingInfo = &gsSGXDeviceMap.sTimingInfo;
	#endif
	psTimingInfo->ui32CoreClockSpeed = SYS_SGX_CLOCK_SPEED;
	psTimingInfo->ui32HWRecoveryFreq = SYS_SGX_HWRECOVERY_TIMEOUT_FREQ;
#if defined(SUPPORT_ACTIVE_POWER_MANAGEMENT)
	psTimingInfo->bEnableActivePM = IMG_TRUE;
#else
	psTimingInfo->bEnableActivePM = IMG_FALSE;
#endif /* SUPPORT_ACTIVE_POWER_MANAGEMENT */
	psTimingInfo->ui32ActivePowManLatencyms = SYS_SGX_ACTIVE_POWER_LATENCY_MS;
	psTimingInfo->ui32uKernelFreq = SYS_SGX_PDS_TIMER_FREQ;

	/* Init device ID's */
	gpsSysData->ui32NumDevices = SYS_DEVICE_COUNT;
	for(i=0; i<SYS_DEVICE_COUNT; i++)
	{
		gpsSysData->sDeviceID[i].uiID = i;
		gpsSysData->sDeviceID[i].bInUse = IMG_FALSE;
	}

	gpsSysData->psDeviceNodeList = IMG_NULL;
	gpsSysData->psQueueList = IMG_NULL;

	/* Initialize pvrsrv */
	eError = SysInitialiseCommon(gpsSysData);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SysInitialise: Failed in SysInitialiseCommon"));
		SysDeinitialise(gpsSysData);
		gpsSysData = IMG_NULL;
		return eError;
	}

	/*
		Locate the devices within the system, specifying
		the physical addresses of each devices components
		(regs, mem, ports etc.)
	*/
	eError = SysLocateDevices(gpsSysData);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SysInitialise: Failed to locate devices"));
		SysDeinitialise(gpsSysData);
		gpsSysData = IMG_NULL;
		return eError;
	}

	/*
		Register devices with the system
		This also sets up their memory maps/heaps
	*/
	eError = PVRSRVRegisterDevice(gpsSysData, SGXRegisterDevice,
								  DEVICE_SGX_INTERRUPT, &gui32SGXDeviceID);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SysInitialise: Failed to register device!"));
		SysDeinitialise(gpsSysData);
		gpsSysData = IMG_NULL;
		return eError;
	}

	/*
		Once all devices are registered, specify the backing store
		and, if required, customise the memory heap config
	*/
	psDeviceNode = gpsSysData->psDeviceNodeList;

	/*
		Create RA if there exist local memory defined.
	*/
	if(gsSGXDeviceMap.ui32LocalMemSize)
	{
		gpsSysData->apsLocalDevMemArena[0] = RA_Create("SGX Local Memory", gsSGXDeviceMap.sLocalMemSysPBase.uiAddr, gsSGXDeviceMap.ui32LocalMemSize,
								IMG_NULL, HOST_PAGESIZE(), IMG_NULL, IMG_NULL, IMG_NULL, IMG_NULL);
		if(gpsSysData->apsLocalDevMemArena != IMG_NULL)
			bUseLocalMem = IMG_TRUE;
		else
		{
			PVR_DPF((PVR_DBG_ERROR, "SysInitialise: Failed to create local dev mem allocator. (base: 0x%08X, size: 0x%08X)",
								gsSGXDeviceMap.sLocalMemSysPBase.uiAddr, gsSGXDeviceMap.ui32LocalMemSize));
		}
	}

	while(psDeviceNode)
	{
		/* perform any OEM SOC address space customisations here */
		switch(psDeviceNode->sDevId.eDeviceType)
		{
			case PVRSRV_DEVICE_TYPE_SGX:
			{
				DEVICE_MEMORY_INFO *psDevMemoryInfo;
				DEVICE_MEMORY_HEAP_INFO *psDeviceMemoryHeap;

				/* specify the backing store to use for the devices MMU PT/PDs */
				if(bUseLocalMem)
					psDeviceNode->psLocalDevMemArena = gpsSysData->apsLocalDevMemArena[0];
				else
					psDeviceNode->psLocalDevMemArena = IMG_NULL;

				/* useful pointers */
				psDevMemoryInfo = &psDeviceNode->sDevMemoryInfo;
				psDeviceMemoryHeap = psDevMemoryInfo->psDeviceMemoryHeap;

				/* specify the backing store for all SGX heaps */
				for(i=0; i<psDevMemoryInfo->ui32HeapCount; i++)
				{
					if(bUseLocalMem)
					{
						/* if the reserved memory exits... */
						psDeviceMemoryHeap[i].ui32Attribs |= PVRSRV_BACKINGSTORE_LOCALMEM_CONTIG;
						psDeviceMemoryHeap[i].psLocalDevMemArena = gpsSysData->apsLocalDevMemArena[0];
					}
					else
						psDeviceMemoryHeap[i].ui32Attribs |= PVRSRV_BACKINGSTORE_SYSMEM_NONCONTIG;
				}

				gpsSGXDevNode = psDeviceNode;
				gpsSysSpecificData->psSGXDevNode = psDeviceNode;
				break;
			}
			default:
			{
				PVR_DPF((PVR_DBG_ERROR,"SysInitialise: Failed to find SGX device node!"));
				return PVRSRV_ERROR_INIT_FAILURE;
			}
		}

		/* advance to next device */
		psDeviceNode = psDeviceNode->psNext;
	}

	/* Initialise all devices 'managed' by services: */
	eError = PVRSRVInitialiseDevice (gui32SGXDeviceID);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SysInitialise: Failed to initialise device!"));
		SysDeinitialise(gpsSysData);
		gpsSysData = IMG_NULL;
		return eError;
	}
	SYS_SPECIFIC_DATA_SET(&gsSysSpecificData, SYS_SPECIFIC_DATA_SGX_INITIALISED);

	EnableSystemClocksWrap(gpsSysData);
	SysResetCoreClk(gpsSysData);
	return PVRSRV_OK;
}

/*!
******************************************************************************

 @Function	SysFinalise
 
 @Description Final part of initialisation
 

 @Return   PVRSRV_ERROR  : 

******************************************************************************/
PVRSRV_ERROR SysFinalise(IMG_VOID)
{
	PVRSRV_ERROR eError = PVRSRV_OK;

	PVR_TRACE(("%s", __FUNCTION__));

	eError = OSInstallMISR(gpsSysData);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SysFinalise: OSInstallMISR failed"));
		return eError;
	}
	SYS_SPECIFIC_DATA_SET(&gsSysSpecificData, SYS_SPECIFIC_DATA_MISR_INSTALLED);

#if defined(SYS_USING_INTERRUPTS)
	eError = OSInstallDeviceLISR(gpsSysData, gsSGXDeviceMap.ui32IRQ, "SGX ISR", gpsSGXDevNode);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SysFinalise: OSInstallSystemLISR failed"));
		return eError;
	}
	SYS_SPECIFIC_DATA_SET(&gsSysSpecificData, SYS_SPECIFIC_DATA_LISR_INSTALLED);
#endif /* defined(SUPPORT_DRI_DRM_EXT) */

#ifdef	__linux__
	/* Create a human readable version string for this system */
#if 0
	eError = SysCreateVersionString(gpsSysData);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SysInitialise: Failed to create a system version string"));
	}
	else
	{
	    PVR_DPF((PVR_DBG_WARNING, "SysFinalise: Version string: %s", gpsSysData->pszVersionString));
	}
#endif
#endif

	return eError;
}

/*!
******************************************************************************

 @Function	SysDeinitialise

 @Description De-initialises kernel services at 'driver unload' time
 

 @Return   PVRSRV_ERROR  : 

******************************************************************************/
PVRSRV_ERROR SysDeinitialise (SYS_DATA *psSysData)
{
	PVRSRV_ERROR eError;

	SYS_SPECIFIC_DATA *psSysSpecData = (SYS_SPECIFIC_DATA *) psSysData->pvSysSpecificData;
	
	PVR_TRACE(("%s", __FUNCTION__));

#if defined(SYS_USING_INTERRUPTS)
	if (SYS_SPECIFIC_DATA_TEST(psSysSpecData, SYS_SPECIFIC_DATA_LISR_INSTALLED))
	{
		eError = OSUninstallSystemLISR(psSysData);
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR,"SysDeinitialise: OSUninstallSystemLISR failed"));
			return eError;
		}
	}
#endif	/* defined(SYS_USING_INTERRUPTS) && !defined(SUPPORT_DRI_DRM_EXT) */

	if (SYS_SPECIFIC_DATA_TEST(psSysSpecData, SYS_SPECIFIC_DATA_MISR_INSTALLED))
	{
		eError = OSUninstallMISR(psSysData);
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR,"SysDeinitialise: OSUninstallMISR failed"));
			return eError;
		}
	}

	if (SYS_SPECIFIC_DATA_TEST(psSysSpecData, SYS_SPECIFIC_DATA_SGX_INITIALISED))
	{
		/* de-initialise all services managed devices */
		eError = PVRSRVDeinitialiseDevice(gui32SGXDeviceID);
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR,"SysDeinitialise: failed to de-init the device"));
			return eError;
		}
	}

#ifdef __linux__
	SysFreeVersionString(psSysData);
#endif

	eError = OSDeInitEnvData(psSysData->pvEnvSpecificData);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SysDeinitialise: failed to de-init env structure"));
		return eError;
	}

	SysDeinitialiseCommon(gpsSysData);

#if defined(NO_HARDWARE)
	if (SYS_SPECIFIC_DATA_TEST(psSysSpecData, SYS_SPECIFIC_DATA_ALLOC_DUMMY_SGX_REGS))
	{
		OSBaseFreeContigMemory(SGX_REG_SIZE, gsSGXRegsCPUVAddr, gsSGXDeviceMap.sRegsCpuPBase);
	}
#endif

	gpsSysData = IMG_NULL;

	return PVRSRV_OK;
}


/*----------------------------------------------------------------------------
<function>
	FUNCTION:   SysGetInterruptSource

	PURPOSE:    Returns System specific information about the device(s) that 
				generated the interrupt in the system

	PARAMETERS: In:  psSysData
				In:  psDeviceNode

	RETURNS:    System specific information indicating which device(s) 
				generated the interrupt

</function>
-----------------------------------------------------------------------------*/
IMG_UINT32 SysGetInterruptSource(SYS_DATA* psSysData,
								 PVRSRV_DEVICE_NODE *psDeviceNode)
{
	PVR_UNREFERENCED_PARAMETER(psSysData);
	PVR_UNREFERENCED_PARAMETER(psDeviceNode);

	if(psDeviceNode == NULL)
	{
		// from system isr wrapper
	}
	else
	{
		// from device isr wrapper
		switch(psDeviceNode->sDevId.eDeviceType)
		{
		case PVRSRV_DEVICE_TYPE_SGX:
			return DEVICE_SGX_INTERRUPT;
		default:
			return 0;
		}
	}
	return 0;
}

/*----------------------------------------------------------------------------
<function>
	FUNCTION:   SysClearInterrupts

	PURPOSE:    Clears specified system interrupts

	PARAMETERS: psSysData
				ui32ClearBits

	RETURNS:    

</function>
-----------------------------------------------------------------------------*/
IMG_VOID SysClearInterrupts(SYS_DATA* psSysData, IMG_UINT32 ui32ClearBits)
{
	PVR_UNREFERENCED_PARAMETER(psSysData);
	PVR_UNREFERENCED_PARAMETER(ui32ClearBits);

	if(ui32ClearBits & DEVICE_SGX_INTERRUPT)
	{
	}
}

/*!
******************************************************************************

 @Function		SysGetDeviceMemoryMap

 @Description	returns a device address map for the specified device

 @Input			eDeviceType - device type
 @Input			ppvDeviceMap - void ptr to receive device specific info.

 @Return   		PVRSRV_ERROR

******************************************************************************/
PVRSRV_ERROR SysGetDeviceMemoryMap(PVRSRV_DEVICE_TYPE eDeviceType,
									IMG_VOID **ppvDeviceMap)
{
	switch(eDeviceType)
	{
		case PVRSRV_DEVICE_TYPE_SGX:
		{
			/* just return a pointer to the structure */
			*ppvDeviceMap = (IMG_VOID*)&gsSGXDeviceMap;
			break;
		}
		default:
		{
			PVR_DPF((PVR_DBG_ERROR,"SysGetDeviceMemoryMap: unsupported device type"));
		}
	}
	return PVRSRV_OK;
}


/*----------------------------------------------------------------------------
<function>
	FUNCTION:   SysCpuPAddrToDevPAddr

	PURPOSE:    Compute a device physical address from a cpu physical
	            address. Relevant when

	PARAMETERS:	In:  cpu_paddr - cpu physical address.
				In:  eDeviceType - device type required if DevPAddr
									address spaces vary across devices 
									in the same system
	RETURNS:	device physical address.

</function>
------------------------------------------------------------------------------*/
IMG_DEV_PHYADDR SysCpuPAddrToDevPAddr (PVRSRV_DEVICE_TYPE eDeviceType,
										IMG_CPU_PHYADDR CpuPAddr)
{
	IMG_DEV_PHYADDR DevPAddr;

	PVR_UNREFERENCED_PARAMETER(eDeviceType);

	/* Note: for this UMA system we assume DevP == CpuP */
	DevPAddr.uiAddr = CpuPAddr.uiAddr;

	return DevPAddr;
}


/*----------------------------------------------------------------------------
<function>
	FUNCTION:   SysSysPAddrToCpuPAddr

	PURPOSE:    Compute a cpu physical address from a system physical
	            address.

	PARAMETERS:	In:  sys_paddr - system physical address.
	RETURNS:	cpu physical address.

</function>
------------------------------------------------------------------------------*/
IMG_CPU_PHYADDR SysSysPAddrToCpuPAddr (IMG_SYS_PHYADDR sys_paddr)
{
	IMG_CPU_PHYADDR cpu_paddr;

	/* This would only be an inequality if the CPU's MMU did not point to sys address 0, 
	   ie. multi CPU system */
	cpu_paddr.uiAddr = sys_paddr.uiAddr;
	return cpu_paddr;
}

/*----------------------------------------------------------------------------
<function>
	FUNCTION:   SysCpuPAddrToSysPAddr

	PURPOSE:    Compute a system physical address from a cpu physical
	            address.

	PARAMETERS:	In:  cpu_paddr - cpu physical address.
	RETURNS:	device physical address.

</function>
------------------------------------------------------------------------------*/
IMG_SYS_PHYADDR SysCpuPAddrToSysPAddr (IMG_CPU_PHYADDR cpu_paddr)
{
	IMG_SYS_PHYADDR sys_paddr;

	/* This would only be an inequality if the CPU's MMU did not point to sys address 0, 
	   ie. multi CPU system */
	sys_paddr.uiAddr = cpu_paddr.uiAddr;
	return sys_paddr;
}


/*----------------------------------------------------------------------------
<function>
	FUNCTION:   SysSysPAddrToDevPAddr

	PURPOSE:    Compute a device physical address from a system physical
	            address.
         	
	PARAMETERS: In:  SysPAddr - system physical address.
				In:  eDeviceType - device type required if DevPAddr
									address spaces vary across devices 
									in the same system

	RETURNS:    Device physical address.

</function>
-----------------------------------------------------------------------------*/
IMG_DEV_PHYADDR SysSysPAddrToDevPAddr (PVRSRV_DEVICE_TYPE eDeviceType, IMG_SYS_PHYADDR SysPAddr)
{
    IMG_DEV_PHYADDR DevPAddr;

	PVR_UNREFERENCED_PARAMETER(eDeviceType);

	/* Note: for this UMA system we assume DevP == CpuP */
    DevPAddr.uiAddr = SysPAddr.uiAddr;

    return DevPAddr;
}


/*----------------------------------------------------------------------------
<function>
	FUNCTION:   SysDevPAddrToSysPAddr

	PURPOSE:    Compute a device physical address from a system physical
	            address.

	PARAMETERS: In:  DevPAddr - device physical address.
				In:  eDeviceType - device type required if DevPAddr
									address spaces vary across devices 
									in the same system

	RETURNS:    System physical address.

</function>
-----------------------------------------------------------------------------*/
IMG_SYS_PHYADDR SysDevPAddrToSysPAddr (PVRSRV_DEVICE_TYPE eDeviceType, IMG_DEV_PHYADDR DevPAddr)
{
    IMG_SYS_PHYADDR SysPAddr;

	PVR_UNREFERENCED_PARAMETER(eDeviceType);

    /* Note: for this UMA system we assume DevP == CpuP */
    SysPAddr.uiAddr = DevPAddr.uiAddr;

    return SysPAddr;
}


/*****************************************************************************
 FUNCTION	: SysRegisterExternalDevice

 PURPOSE	: Called when a 3rd party device registers with services

 PARAMETERS: In:  psDeviceNode - the new device node.
 
 RETURNS	: IMG_VOID
*****************************************************************************/
IMG_VOID SysRegisterExternalDevice(PVRSRV_DEVICE_NODE *psDeviceNode)
{
#if defined(PVRSRV_NEED_PVR_TRACE)
	static const char const_class_names[][32] = {
		"PVRSRV_DEVICE_CLASS_3D",
		"PVRSRV_DEVICE_CLASS_DISPLAY",
		"PVRSRV_DEVICE_CLASS_BUFFER",
		"PVRSRV_DEVICE_CLASS_VIDEO"
	};
	static const char const_type_names[][32] = {
		"PVRSRV_DEVICE_TYPE_UNKNOWN",		
		"PVRSRV_DEVICE_TYPE_MBX1",
		"PVRSRV_DEVICE_TYPE_MBX1_LITE",
		"PVRSRV_DEVICE_TYPE_M24VA",
		"PVRSRV_DEVICE_TYPE_MVDA2",
		"PVRSRV_DEVICE_TYPE_MVED1",
		"PVRSRV_DEVICE_TYPE_MSVDX",
		"PVRSRV_DEVICE_TYPE_SGX",
		"PVRSRV_DEVICE_TYPE_VGX",
		"PVRSRV_DEVICE_TYPE_EXT"
	};
#endif
	PVR_UNREFERENCED_PARAMETER(psDeviceNode);
	PVR_TRACE(("%s: 0x%p", __FUNCTION__, psDeviceNode));
	PVR_TRACE(("\tDeviceType : %d (%s)", (int)psDeviceNode->sDevId.eDeviceType, const_type_names[(int)psDeviceNode->sDevId.eDeviceType]));
	PVR_TRACE(("\tDeviceClass: %d (%s)", (int)psDeviceNode->sDevId.eDeviceClass, const_class_names[(int)psDeviceNode->sDevId.eDeviceClass]));
	PVR_TRACE(("\tDeviceIndex: %d", (int)psDeviceNode->sDevId.ui32DeviceIndex));
}


/*****************************************************************************
 FUNCTION	: SysRemoveExternalDevice

 PURPOSE	: Called when a 3rd party device unregisters from services

 PARAMETERS: In:  psDeviceNode - the device node being removed.

 RETURNS	: IMG_VOID
*****************************************************************************/
IMG_VOID SysRemoveExternalDevice(PVRSRV_DEVICE_NODE *psDeviceNode)
{
	PVR_UNREFERENCED_PARAMETER(psDeviceNode);
	PVR_TRACE(("%s: 0x%p", __FUNCTION__, psDeviceNode));
	PVR_TRACE(("\tDeviceType : %d", (int)psDeviceNode->sDevId.eDeviceType));
	PVR_TRACE(("\tDeviceClass: %d", (int)psDeviceNode->sDevId.eDeviceClass));
	PVR_TRACE(("\tDeviceIndex: %d", (int)psDeviceNode->sDevId.ui32DeviceIndex));
}

/*****************************************************************************
 FUNCTION	: SysOEMFunction

 PURPOSE	: marshalling function for custom OEM functions

 PARAMETERS	: ui32ID  - function ID
			  pvIn - in data
			  pvOut - out data
 
 RETURNS	: PVRSRV_ERROR
*****************************************************************************/
PVRSRV_ERROR SysOEMFunction (	IMG_UINT32	ui32ID,
								IMG_VOID	*pvIn,
								IMG_UINT32  ulInSize,
								IMG_VOID	*pvOut,
								IMG_UINT32	ulOutSize)
{
	PVR_UNREFERENCED_PARAMETER(ulInSize);
	PVR_UNREFERENCED_PARAMETER(pvIn);
	
	PVR_TRACE(("%s", __FUNCTION__));

	if ((ui32ID == OEM_GET_EXT_FUNCS) &&
		(ulOutSize == sizeof(PVRSRV_DC_OEM_JTABLE)))
	{
		PVRSRV_DC_OEM_JTABLE *psOEMJTable = (PVRSRV_DC_OEM_JTABLE*)pvOut;
		psOEMJTable->pfnOEMBridgeDispatch = &PVRSRV_BridgeDispatchKM;
#ifdef	__linux__
		psOEMJTable->pfnOEMReadRegistryString  = IMG_NULL;
		psOEMJTable->pfnOEMWriteRegistryString = IMG_NULL;
#else
		psOEMJTable->pfnOEMReadRegistryString  = IMG_NULL;//&PVRSRVReadRegistryString;
		psOEMJTable->pfnOEMWriteRegistryString = IMG_NULL;//&PVRSRVWriteRegistryString;
#endif

		return PVRSRV_OK;
	}

	return PVRSRV_ERROR_INVALID_PARAMS;
}


static PVRSRV_ERROR SysMapInRegisters(IMG_VOID)
{
	PVRSRV_DEVICE_NODE *psDeviceNodeList;

	PVR_TRACE(("%s", __FUNCTION__));
	psDeviceNodeList = gpsSysData->psDeviceNodeList;

	while (psDeviceNodeList)
	{
		switch(psDeviceNodeList->sDevId.eDeviceType)
		{
		case PVRSRV_DEVICE_TYPE_SGX:
		{
			PVRSRV_SGXDEV_INFO *psDevInfo = (PVRSRV_SGXDEV_INFO *)psDeviceNodeList->pvDevice;
#if defined(NO_HARDWARE) && defined(__linux__)
			/*
			 * SysLocateDevices will have reallocated the dummy
			 * registers.
			 */
			PVR_ASSERT(gsSGXRegsCPUVAddr);

			psDevInfo->pvRegsBaseKM = gsSGXRegsCPUVAddr;
#else	/* defined(NO_HARDWARE) && defined(__linux__) */
			/* Remap SGX Regs */
			if (SYS_SPECIFIC_DATA_TEST(&gsSysSpecificData, SYS_SPECIFIC_DATA_PM_UNMAP_SGX_REGS))
			{
				psDevInfo->pvRegsBaseKM = OSMapPhysToLin(gsSGXDeviceMap.sRegsCpuPBase,
									 gsSGXDeviceMap.ui32RegsSize,
									 PVRSRV_HAP_KERNEL_ONLY|PVRSRV_HAP_UNCACHED,
									 IMG_NULL);

				if (!psDevInfo->pvRegsBaseKM)
				{
					PVR_DPF((PVR_DBG_ERROR,"SysMapInRegisters : Failed to map in SGX registers\n"));
					return PVRSRV_ERROR_BAD_MAPPING;
				}
				SYS_SPECIFIC_DATA_CLEAR(&gsSysSpecificData, SYS_SPECIFIC_DATA_PM_UNMAP_SGX_REGS);
			}
#endif	/* #if defined(NO_HARDWARE) && defined(__linux__) */

			psDevInfo->ui32RegSize   = gsSGXDeviceMap.ui32RegsSize;
			psDevInfo->sRegsPhysBase = gsSGXDeviceMap.sRegsSysPBase;
			break;
		}
		default:
			break;
		}
		psDeviceNodeList = psDeviceNodeList->psNext;
	}

	return PVRSRV_OK;
}


static PVRSRV_ERROR SysUnmapRegisters(IMG_VOID)
{
	PVRSRV_DEVICE_NODE *psDeviceNodeList;

	PVR_TRACE(("%s", __FUNCTION__));
	psDeviceNodeList = gpsSysData->psDeviceNodeList;

	while (psDeviceNodeList)
	{
		switch (psDeviceNodeList->sDevId.eDeviceType)
		{
		case PVRSRV_DEVICE_TYPE_SGX:
		{
			PVRSRV_SGXDEV_INFO *psDevInfo = (PVRSRV_SGXDEV_INFO *)psDeviceNodeList->pvDevice;
#if !(defined(NO_HARDWARE) && defined(__linux__))
			PVR_TRACE(("%s, unmap pvRegsBaseKM........ ", __FUNCTION__));
			/* Unmap Regs */
			if (psDevInfo->pvRegsBaseKM)
			{
				OSUnMapPhysToLin(psDevInfo->pvRegsBaseKM,
				                 gsSGXDeviceMap.ui32RegsSize,
				                 PVRSRV_HAP_KERNEL_ONLY|PVRSRV_HAP_UNCACHED,
				                 IMG_NULL);

				SYS_SPECIFIC_DATA_SET(&gsSysSpecificData, SYS_SPECIFIC_DATA_PM_UNMAP_SGX_REGS);
			}
#endif	/* #if !(defined(NO_HARDWARE) && defined(__linux__)) */

			psDevInfo->pvRegsBaseKM = IMG_NULL;
			psDevInfo->ui32RegSize          = 0;
			psDevInfo->sRegsPhysBase.uiAddr = 0;
			break;
		}
		default:
			break;
		}
		psDeviceNodeList = psDeviceNodeList->psNext;
	}

#if defined(NO_HARDWARE)
	if (SYS_SPECIFIC_DATA_TEST(&gsSysSpecificData, SYS_SPECIFIC_DATA_ALLOC_DUMMY_SGX_REGS))
	{
		PVR_ASSERT(gsSGXRegsCPUVAddr);

		OSBaseFreeContigMemory(SGX_REG_SIZE, gsSGXRegsCPUVAddr, gsSGXDeviceMap.sRegsCpuPBase);
		gsSGXRegsCPUVAddr = IMG_NULL;

		SYS_SPECIFIC_DATA_CLEAR(&gsSysSpecificData, SYS_SPECIFIC_DATA_ALLOC_DUMMY_SGX_REGS);
	}
#endif /* defined(NO_HARDWARE) */

	return PVRSRV_OK;
}

/*!
******************************************************************************

 @Function	SysSystemPrePowerState
 
 @Description	Perform system-level processing required before a system power
 				transition
 
 @Input	   eNewPowerState : 

 @Return   PVRSRV_ERROR : 

******************************************************************************/
PVRSRV_ERROR SysSystemPrePowerState(PVRSRV_SYS_POWER_STATE eNewPowerState)
{
	PVRSRV_ERROR eError= PVRSRV_OK;

	PVR_TRACE(("%s, %d", __FUNCTION__, (IMG_UINT32)eNewPowerState));
	{
		if (eNewPowerState == PVRSRV_SYS_POWER_STATE_D3)
		{
			/*
			 * About to enter D3 state.
			 */
#if defined(SYS_USING_INTERRUPTS)
			if (SYS_SPECIFIC_DATA_TEST(&gsSysSpecificData, SYS_SPECIFIC_DATA_LISR_INSTALLED))
			{
				eError = OSUninstallDeviceLISR(gpsSysData);
				if (eError != PVRSRV_OK)
				{
					PVR_DPF((PVR_DBG_ERROR,"SysSystemPrePowerState: OSUninstallSystemLISR failed (%d)", eError));
				}
				SYS_SPECIFIC_DATA_SET(&gsSysSpecificData, SYS_SPECIFIC_DATA_PM_UNINSTALL_LISR);
				SYS_SPECIFIC_DATA_CLEAR(&gsSysSpecificData, SYS_SPECIFIC_DATA_LISR_INSTALLED);
			}
#endif /* !defined(UNDER_VISTA) && defined (SYS_USING_INTERRUPTS) */

			/*
			 * Unmap the system-level registers.
			 */
			SysUnmapRegisters();
		}
	}
	return eError;
}

/*!
******************************************************************************

 @Function	SysSystemPostPowerState
 
 @Description	Perform system-level processing required after a system power
 				transition
 
 @Input	   eNewPowerState :

 @Return   PVRSRV_ERROR :

******************************************************************************/
PVRSRV_ERROR SysSystemPostPowerState(PVRSRV_SYS_POWER_STATE eNewPowerState)
{
	PVRSRV_ERROR eError = PVRSRV_OK;

	PVR_TRACE(("%s, %d", __FUNCTION__, (IMG_UINT32)eNewPowerState));
	{
		if (eNewPowerState == PVRSRV_SYS_POWER_STATE_D0)
		{
			/*
				Returning from D3 state.
				Find the device again as it may have been remapped.
			*/
			eError = SysLocateDevices(gpsSysData);
			if (eError != PVRSRV_OK)
			{
				PVR_DPF((PVR_DBG_ERROR,"SysSystemPostPowerState: Failed to locate devices"));
				return eError;
			}

			/*
			 * Map the system-level registers.
			 */
			eError = SysMapInRegisters();
			if (eError != PVRSRV_OK)
			{
				PVR_DPF((PVR_DBG_ERROR,"SysSystemPostPowerState: Failed to map in registers"));
				return eError;
			}
		
			/*
			 * Disable all clock gating.
			 */	
			SysResetCoreClk(gpsSysData);

#if defined(SYS_USING_INTERRUPTS)
			if (SYS_SPECIFIC_DATA_TEST(&gsSysSpecificData, SYS_SPECIFIC_DATA_PM_UNINSTALL_LISR))
			{
				//eError = OSInstallSystemLISR(gpsSysData, gsSGXDeviceMap.ui32IRQ);
				eError = OSInstallDeviceLISR(gpsSysData, gsSGXDeviceMap.ui32IRQ, "SGX_ISR", gpsSGXDevNode);
				if (eError != PVRSRV_OK)
				{
					PVR_DPF((PVR_DBG_ERROR,"SysSystemPostPowerState: OSInstallSystemLISR failed to install ISR (%d)", eError));
				}
				SYS_SPECIFIC_DATA_SET(&gsSysSpecificData, SYS_SPECIFIC_DATA_LISR_INSTALLED);
				SYS_SPECIFIC_DATA_CLEAR(&gsSysSpecificData, SYS_SPECIFIC_DATA_PM_UNINSTALL_LISR);
			}
#endif /* defined(SYS_USING_INTERRUPTS) */
		}
	}
	return eError;
}


/*!
******************************************************************************

 @Function	SysDevicePrePowerState
 
 @Description	Perform system-level processing required before a device power
 				transition
 
 @Input	   ui32DeviceIndex : 
 @Input	   eNewPowerState : 
 @Input	   eCurrentPowerState : 

 @Return   PVRSRV_ERROR : 

******************************************************************************/
PVRSRV_ERROR SysDevicePrePowerState(IMG_UINT32			ui32DeviceIndex,
									PVRSRV_DEV_POWER_STATE	eNewPowerState,
									PVRSRV_DEV_POWER_STATE	eCurrentPowerState)
{
	if ((eNewPowerState != eCurrentPowerState) &&
	    (eNewPowerState == PVRSRV_DEV_POWER_STATE_OFF))
	{
		if (ui32DeviceIndex == gui32SGXDeviceID)
		{
			PVR_DPF((PVR_DBG_MESSAGE,"SysDevicePrePowerState: Remove SGX power"));
		}
	}

	return PVRSRV_OK;
}


/*!
******************************************************************************

 @Function	SysDevicePostPowerState
 
 @Description	Perform system-level processing required after a device power
 				transition
 
 @Input	   ui32DeviceIndex :
 @Input	   eNewPowerState :
 @Input	   eCurrentPowerState :

 @Return   PVRSRV_ERROR :

******************************************************************************/
PVRSRV_ERROR SysDevicePostPowerState(IMG_UINT32			ui32DeviceIndex,
									 PVRSRV_DEV_POWER_STATE	eNewPowerState,
									 PVRSRV_DEV_POWER_STATE	eCurrentPowerState)
{
	if ((eNewPowerState != eCurrentPowerState) &&
	    (eCurrentPowerState == PVRSRV_DEV_POWER_STATE_OFF))
	{
		if (ui32DeviceIndex == gui32SGXDeviceID)
		{
			PVR_DPF((PVR_DBG_MESSAGE,"SysDevicePrePowerState: Restore SGX power"));
		}
	}

	return PVRSRV_OK;
}
#if 1//defined(SGX_OCP_NO_INT_BYPASS)

/*!
******************************************************************************

 @Function  EnableSGXClocks

 @Description Enable SGX clocks

 @Return   PVRSRV_ERROR

******************************************************************************/

PVRSRV_ERROR EnableSGXClocks(SYS_DATA *psSysData)
{
#if !defined(NO_HARDWARE)
	SYS_SPECIFIC_DATA *psSysSpecData = (SYS_SPECIFIC_DATA *) psSysData->pvSysSpecificData;

	if (atomic_read(&psSysSpecData->sSGXClocksEnabled) != 0)
	{
		return PVRSRV_OK;
	}

	PVR_DPF((PVR_DBG_MESSAGE, "EnableSGXClocks: Enabling SGX Clocks"));
       SysResetCoreClk(psSysData);  //set the sgx ckgen for  sgx_core & sgx_HYD
	  
	SysEnableSGXInterrupts(psSysData);

	/* Indicate that the SGX clocks are enabled */
	atomic_set(&psSysSpecData->sSGXClocksEnabled, 1);

#else	/* !defined(NO_HARDWARE) */
	PVR_UNREFERENCED_PARAMETER(psSysData);
#endif	/* !defined(NO_HARDWARE) */
	return PVRSRV_OK;
}

IMG_VOID DisableSGXClocks(SYS_DATA *psSysData)
{
#if !defined(NO_HARDWARE)
	SYS_SPECIFIC_DATA *psSysSpecData = (SYS_SPECIFIC_DATA *) psSysData->pvSysSpecificData;

	/* SGX clocks already disabled? */
	if (atomic_read(&psSysSpecData->sSGXClocksEnabled) == 0)
	{
		return;
	}

	PVR_DPF((PVR_DBG_MESSAGE, "DisableSGXClocks: Disabling SGX Clocks"));

	SysDisableSGXInterrupts(psSysData);

	/* Indicate that the SGX clocks are disabled */
	atomic_set(&psSysSpecData->sSGXClocksEnabled, 0);

#else	/* !defined(NO_HARDWARE) */
	PVR_UNREFERENCED_PARAMETER(psSysData);
#endif	/* !defined(NO_HARDWARE) */
}
/*!
******************************************************************************
 @Function        SysEnableSGXInterrupts

 @Description     Enables SGX interrupts

 @Input           psSysData

 @Return        IMG_VOID

******************************************************************************/
IMG_VOID SysEnableSGXInterrupts(SYS_DATA *psSysData)
{
      IMG_CPU_PHYADDR sPLLRegsCpuPBase = {0xF000D000};
//	IMG_CPU_PHYADDR sCFGRegsCpuPBase = {0xF004E000};
	IMG_VOID *pvPLLRegs = OSMapPhysToLin(sPLLRegsCpuPBase, PAGE_SIZE,
											PVRSRV_HAP_KERNEL_ONLY|PVRSRV_HAP_UNCACHED,
											IMG_NULL);


	OSWriteHWReg(pvPLLRegs, EUR_CR_OCP_IRQSTATUS_2, 0x1);
	OSWriteHWReg(pvPLLRegs, EUR_CR_OCP_IRQENABLE_SET_2, 0x1);

	OSUnMapPhysToLin(pvPLLRegs, PAGE_SIZE,
							PVRSRV_HAP_KERNEL_ONLY|PVRSRV_HAP_UNCACHED,
							IMG_NULL);
	
}

/*!
******************************************************************************
 @Function        SysDisableSGXInterrupts

 @Description     Disables SGX interrupts

 @Input           psSysData

 @Return        IMG_VOID

******************************************************************************/
IMG_VOID SysDisableSGXInterrupts(SYS_DATA *psSysData)
{
		 IMG_CPU_PHYADDR sPLLRegsCpuPBase = {0xF000D000};
		//IMG_CPU_PHYADDR sCFGRegsCpuPBase = {0xF004E000};
		IMG_VOID *pvPLLRegs = OSMapPhysToLin(sPLLRegsCpuPBase, PAGE_SIZE,
												PVRSRV_HAP_KERNEL_ONLY|PVRSRV_HAP_UNCACHED,
												IMG_NULL);
	
	
		OSWriteHWReg(pvPLLRegs, EUR_CR_OCP_IRQENABLE_CLR_2, 0x1);
	
		OSUnMapPhysToLin(pvPLLRegs, PAGE_SIZE,
								PVRSRV_HAP_KERNEL_ONLY|PVRSRV_HAP_UNCACHED,
								IMG_NULL);
}

static  PVRSRV_ERROR EnableSGXClocksWrap(SYS_DATA *psSysData)
{
	return EnableSGXClocks(psSysData);
}

#endif	/* defined(SGX_OCP_NO_INT_BYPASS) */
