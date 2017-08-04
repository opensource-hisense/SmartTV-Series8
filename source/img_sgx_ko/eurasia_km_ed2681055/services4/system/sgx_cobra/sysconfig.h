/*!****************************************************************************
@File			sysconfig.h

@Title			System Description Header

@Author			Imagination Technologies

@date   		3 March 2003
 
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

$Log: sysconfig.h $
*****************************************************************************/

#if !defined(__SOCCONFIG_H__)
#define __SOCCONFIG_H__

#if defined(SUPPORT_EXTERNAL_SYSTEM_CACHE)
#include "extsyscache.h"
#endif

#define VS_PRODUCT_NAME			"SGX Cobra"

#if defined(SGX_FEATURE_MP)
  /* One block for each core plus one for the all-cores bank and one for the master bank.*/
  #define SGX_REG_SIZE			(0xC000)//((0x4000 * (SGX_FEATURE_MP_CORE_COUNT_3D + 2)) + 0x1000)
#else
  #define SGX_REG_SIZE			((0x4000) + 0x1000)
#endif /* SGX_FEATURE_MP */

/*****************************************************************************
 * system physical address
 *****************************************************************************/
#define SGX_REG_BASE_ADDRESS	(0xF0040000)
#define SGX_IRQ					(53 + 32)


/*****************************************************************************
 * system specific data structures
 *****************************************************************************/
 
#endif	/* __SOCCONFIG_H__ */
