/* $Id: //DTV/MP_BR/DTV_X_IDTV1501_1538_4_001_46_007/apollo/linux_core/kernel/linux-3.10/arch/cris/include/asm/dma.h#1 $ */

#ifndef _ASM_DMA_H
#define _ASM_DMA_H

#include <arch/dma.h>

/* it's useless on the Etrax, but unfortunately needed by the new
   bootmem allocator (but this should do it for this) */

#define MAX_DMA_ADDRESS PAGE_OFFSET

/* From PCI */

#ifdef CONFIG_PCI
extern int isa_dma_bridge_buggy;
#else
#define isa_dma_bridge_buggy 	(0)
#endif

#endif /* _ASM_DMA_H */
