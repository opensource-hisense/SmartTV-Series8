/*
 * linux/arch/arm/mach-mt53xx/include/mach/memory.h
 *
 * Copyright (c) 2010-2012 MediaTek Inc.
 * $Author: dtvbm11 $
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

#ifndef __ASM_ARCH_MEMORY_H
#define __ASM_ARCH_MEMORY_H

/*
 * Physical DRAM offset.
 */
#define PCIE_MMIO_BASE                         (0xF0200000)                 /* MMIO: memory mapped io base address for PCI-express. */
#define PCIE_MMIO_PHYSICAL_BASE     (0xF0200000)                  /* AHB-PCI window base */
#define PCIE_MMIO_LENGTH                    (0x00200000)                 /* AHB-PCI window size = 2Mbytes for MMIO. */
#define PCIE_BRIDGE_LENGTH                 (0x00200000)                 /* 2Mbytes Bridge size for PCI-express. */

#ifdef CC_MT53XX_SUPPORT_2G_DRAM
#define MAX_PHYSMEM_BITS    31
#else
#define MAX_PHYSMEM_BITS    30
#endif
#define SECTION_SIZE_BITS   27
/*#define NODE_MEM_SIZE_BITS  29*/


#define ARCH_VECTOR_PAGE_PHYS    0xFB004000
#define DMX_SRAM_VIRT            0xF2008000            // Will mapping DMX SRAM (0xFB00500) to here, put after GIC to reduce fragement.
#define LZHS_SRM_VIRT            0xF4000000            // Mapping LZHS SRAM for DRAM self refresh mode

#endif
