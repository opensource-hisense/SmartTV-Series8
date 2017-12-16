/*
 * vm_linux/chiling/uboot/board/mt5398/env.c
 *
 * Copyright (c) 2010-2012 MediaTek Inc.
 *	This file is based  ARM Realview platform
 *  Copyright (C) 2002 ARM Ltd.
 *  All Rights Reserved
 * $Author: tao.tian $
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
 */

#include <common.h>
#include <exports.h>

#include "x_bim.h"
#include "drvcust_if.h"

#if defined(CC_DYNAMIC_FBMSRM_CONF)
typedef struct {
INT32 magic;
UINT32 kernel_dram_start;
UINT32 kernel_dram_size;
UINT32 kernel_dram_4k_size;
UINT32 kernel_dram_fhd_size;
UINT32 kernel_dram2_start;
UINT32 kernel_dram2_size;
UINT32 kernel_dram2_4k_size;
UINT32 kernel_dram2_fhd_size;
} mem_cfg ;


mem_cfg mt53xx_memcfg =
{
	0x53965368,
	PHYS_SDRAM_1_SIZE,
	PHYS_SDRAM_1_4K_SIZE,
	PHYS_SDRAM_1_FHD_SIZE,
#if CONFIG_NR_DRAM_BANKS>1
	PHYS_SDRAM_2,
	PHYS_SDRAM_2_SIZE,
	PHYS_SDRAM_2_4K_SIZE,
	PHYS_SDRAM_2_FHD_SIZE,
#endif
};

#else
typedef struct {
UINT32 magic;
UINT32 kernel_dram_start;
UINT32 kernel_dram_size;
UINT32 kernel_dram2_start;
UINT32 kernel_dram2_size;
UINT32 kernel_dram3_start;
UINT32 kernel_dram3_size;


} mem_cfg ;


mem_cfg mt53xx_memcfg =
{
	0x53965368,
	PHYS_SDRAM_1_SIZE,
#if CONFIG_NR_DRAM_BANKS>1
	PHYS_SDRAM_2,
	PHYS_SDRAM_2_SIZE,
#ifdef CC_SUPPORT_CHANNEL_C
	PHYS_SDRAM_3,
	PHYS_SDRAM_3_SIZE,
#endif
#endif

  
};
#endif
#if 0
UINT32 global_magic = 0x53965368;
UINT32 global_kernel_dram_size = PHYS_SDRAM_1_SIZE;

#if CONFIG_NR_DRAM_BANKS>1
UINT32 global_kernel_dram2_start = PHYS_SDRAM_2;
UINT32 global_kernel_dram2_size = PHYS_SDRAM_2_SIZE;
#endif
#endif

