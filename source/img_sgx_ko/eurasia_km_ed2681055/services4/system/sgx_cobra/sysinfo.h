/*!****************************************************************************
@File			sysinfo.h

@Title			System Description Header

@Author			Imagination Technologies

@date   		6 August 2004
 
@Copyright     	Copyright 2003-2004 by Imagination Technologies Limited.
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

@Description	This header provides system-specific declarations and macros

@DoxygenVer		

******************************************************************************/

/******************************************************************************
Modifications :-

$Log: sysinfo.h $
*****************************************************************************/

#if !defined(__SYSINFO_H__)
#define __SYSINFO_H__

/*!< System specific poll/timeout details */
#define MAX_HW_TIME_US				(500000)
#define WAIT_TRY_COUNT				(10000)

#define SYS_FB_DEVICE_COUNT		3
#define SYS_DEVICE_COUNT		(2 + SYS_FB_DEVICE_COUNT) /* SGX, DisplayClass (external), 
													  FBDev0 (external), FBDev1 (external), FBDev2 (external) */

#define DEVICE_SGX_INTERRUPT				(1UL << 0)

#define KHz(x)			((x)*1000UL)
#define MHz(x)			(KHz(x*1000UL))

/* System specific timing defines*/
#define SYS_SGX_CLOCK_SPEED					324000000UL //MHz(324) // (324Mhz)
#define SYS_SGX_HWRECOVERY_TIMEOUT_FREQ		(300UL)//(100UL)  // 10ms (100hz)
#define SYS_SGX_PDS_TIMER_FREQ				(1000UL) // 1ms (1000hz)
#define SYS_SGX_ACTIVE_POWER_LATENCY_MS		(1UL)


#if defined(PVR_LDM_PLATFORM_PRE_REGISTERED_DEV)
#define	SYS_SGX_DEV_NAME	PVR_LDM_PLATFORM_PRE_REGISTERED_DEV
#else

#define	SYS_SGX_DEV_NAME	"omap_gpu"
#endif	/* defined(PVR_LDM_PLATFORM_PRE_REGISTERED_DEV) */


#endif	/* __SYSINFO_H__ */
