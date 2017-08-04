/*!****************************************************************************
@File			syslocal.h

@Title			Local system definitions

@Author			Imagination Technologies

@date   		23/4/2007
 
@Copyright     	Copyright 2003-2007 by Imagination Technologies Limited.
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

@Description	This header provides local system declarations and macros

@DoxygenVer		

******************************************************************************/

/******************************************************************************
Modifications :-
$Log: syslocal.h $
****************************************************************************/

#if !defined(__SYSLOCAL_H__)
#define __SYSLOCAL_H__

#if !defined(NO_HARDWARE) && \
     defined(SYS_USING_INTERRUPTS)
#define SGX_OCP_REGS_ENABLED
#endif

#if defined(__linux__)
#if defined(SGX_OCP_REGS_ENABLED)
/* FIXME: Temporary workaround for OMAP4470 and active power off in 4430 */
#if !defined(SGX544) && defined(SUPPORT_ACTIVE_POWER_MANAGEMENT)
#define SGX_OCP_NO_INT_BYPASS
#endif
#endif
#endif
PVRSRV_ERROR EnableSystemClocks(SYS_DATA *psSysData);


/*
 * Various flags to indicate what has been initialised.
 * Only used at driver initialisation/deinitialisation time.
 */
#define	SYS_SPECIFIC_DATA_SGX_INITIALISED		0x00000001UL
#define	SYS_SPECIFIC_DATA_MISR_INSTALLED		0x00000002UL
#define	SYS_SPECIFIC_DATA_LISR_INSTALLED		0x00000004UL
#define	SYS_SPECIFIC_DATA_PDUMP_INIT			0x00000008UL
#define	SYS_SPECIFIC_DATA_IRQ_ENABLED			0x00000010UL
#define	SYS_SPECIFIC_DATA_PM_UNMAP_SGX_REGS		0x00000020UL
#define	SYS_SPECIFIC_DATA_PM_IRQ_DISABLE		0x00000040UL
#define	SYS_SPECIFIC_DATA_PM_UNINSTALL_LISR		0x00000080UL

#define	SYS_SPECIFIC_DATA_SET(psSysSpecData, flag) ((IMG_VOID)((psSysSpecData)->ui32SysSpecificData |= (flag)))

#define	SYS_SPECIFIC_DATA_CLEAR(psSysSpecData, flag) ((IMG_VOID)((psSysSpecData)->ui32SysSpecificData &= ~(flag)))

#define	SYS_SPECIFIC_DATA_TEST(psSysSpecData, flag) (((psSysSpecData)->ui32SysSpecificData & (flag)) != 0)


/*****************************************************************************
 * system specific data structures
 *****************************************************************************/
 
typedef struct _SYS_SPECIFIC_DATA_TAG_
{
	IMG_UINT32 ui32SysSpecificData;
	PVRSRV_DEVICE_NODE *psSGXDevNode;

#if defined(SUPPORT_DRI_DRM)
	struct drm_device *psDRMDev;
#endif
#if defined(__linux__)
	IMG_BOOL	bSysClocksOneTimeInit;
	atomic_t	sSGXClocksEnabled;
#if defined(PVR_LINUX_USING_WORKQUEUES)
	struct mutex	sPowerLock;
#else
	IMG_BOOL	bConstraintNotificationsEnabled;
	spinlock_t	sPowerLock;
	atomic_t	sPowerLockCPU;
	spinlock_t	sNotifyLock;
	atomic_t	sNotifyLockCPU;
	IMG_BOOL	bCallVDD2PostFunc;
#endif
#endif	/* defined(__linux__) */


} SYS_SPECIFIC_DATA;

/*****************************************************************************
 * system specific function prototypes
 *****************************************************************************/
IMG_VOID SysEnableSGXInterrupts(SYS_DATA* psSysData);
IMG_VOID SysDisableSGXInterrupts(SYS_DATA* psSysData);


IMG_VOID SysResetCoreClk(SYS_DATA *psSysData);
#endif	/* __SYSLOCAL_H__ */


