/*
 *  linux/arch/arm/mach-MT85xx/cache_operation.h
 *
 * Copyright (C) 2009 MediaTek Inc
 *
 * 
 *
 * cache opertaion.
 */

#ifndef _CACHE_OPERATION_H_
#define _CACHE_OPERATION_H_

#include <include/x_typedef.h>
//#include <config/arch/cache_mon_config.h>

/*
 * BSP_FlushDCacheRange(UINT32 u4Start, UINT32 u4End)
 *
 *    Clean and Invalidate Data Cache Range
 *    - u4Start : virtual start address (inclusive)
 *    - u4Len : length
 */
#define BSP_CACHE_OPERATION_DEBUG 0 
#define CONFIG_LINUX_MONCACHEOPERATION 0
#define FLUSH_L1_RANGE_MAX      0x00008000
#define CLEAN_L1_RANGE_MAX      0x00008000
#define INV_L1_RANGE_MAX        0x00008000

#ifndef CONFIG_OUTER_CACHE
#define CORE_FLUSH_RANGE_MAX    0x00008000
#define CORE_CLEAN_RANGE_MAX    0x00008000
#define CORE_INV_RANGE_MAX      0x00008000
#else
#define FLUSH_L1L2_RANGE_MAX      0x00003800
#define CLEAN_L1L2_RANGE_MAX      0x00003000
#define INV_L1L2_RANGE_MAX        0x00003800
#define CORE_FLUSH_RANGE_MAX    0x00008000
#define CORE_CLEAN_RANGE_MAX    0x00008000
#define CORE_INV_RANGE_MAX      0x00008000
#endif

#define CACHE_LINE_UNIT          32  // 1 cache line = 32 byte
#if CONFIG_LINUX_MONCACHEOPERATION
#define Linux_MonCacheOperation 1
#else
#define Linux_MonCacheOperation 0
#endif
#if (Linux_MonCacheOperation == 0)
#if __LINUX_ARM_ARCH__ == 6
extern void BSP_FlushDCacheRange(UINT32 u4Start, UINT32 u4Len); //DMA_TO_DEVICE + DMA_FROM_DEVICE 
extern void BSP_CleanDCacheRange(UINT32 u4Start, UINT32 u4Len); //DMA_TO_DEVICE
extern void BSP_InvDCacheRange(UINT32 u4Start, UINT32 u4Len);   //DMA_FROM_DEVICE
#endif
#elif !defined(Gen_MonCacheOperation)
//#else
typedef struct mon_data {
    struct mon_data *next;
    unsigned long count;
    unsigned long long amount;
    const char *file;
    unsigned line;
} mon_data;
   
#define MON(func, start, len) \
({ \
    static mon_data \
        __attribute__((section(".data.mon"))) \
        ____m = { \
        .file = __FILE__, \
        .line = __LINE__, \
    }; \
    mon_##func(&____m, start, len); \
})
   
#define BSP_FlushDCacheRange(u4Start, u4Len) MON(BSP_FlushDCacheRange, u4Start, u4Len)
#define BSP_CleanDCacheRange(u4Start, u4Len) MON(BSP_CleanDCacheRange, u4Start, u4Len)
#define BSP_InvDCacheRange(u4Start, u4Len)   MON(BSP_InvDCacheRange, u4Start, u4Len)
   
extern void mon_BSP_FlushDCacheRange(mon_data *m, UINT32 u4Start, UINT32 u4Len);
extern void mon_BSP_CleanDCacheRange(mon_data *m, UINT32 u4Start, UINT32 u4Len);
extern void mon_BSP_InvDCacheRange(mon_data *m, UINT32 u4Start, UINT32 u4Len);   
#endif

extern void Core_CleanDCacheRange(UINT32 u4Start, UINT32 u4Len);//DMA_TO_L2
extern void Core_InvDCacheRange(UINT32 u4Start, UINT32 u4Len);  //DMA_FROM_L2
void Core_CleanOuterCacheRange(UINT32 u4Start, UINT32 u4Len);

enum data_direction {
	BIDIRECTIONAL = 0,
	TO_DEVICE = 1,
	FROM_DEVICE = 2
};
#if (Linux_MonCacheOperation == 0)
extern UINT32 BSP_dma_map_single(UINT32 u4Start, UINT32 u4Len, enum data_direction dir);
extern void BSP_dma_unmap_single(UINT32 u4Start, UINT32 u4Len, enum data_direction dir);
extern void BSP_dma_map_vaddr(UINT32 u4Start, UINT32 u4Len, enum data_direction dir);
extern void BSP_dma_unmap_vaddr(UINT32 u4Start, UINT32 u4Len, enum data_direction dir);
#else

#define MON2(func, start, len, dir) \
({ \
    static mon_data \
        __attribute__((section(".data.mon"))) \
        ____m = { \
        .file = __FILE__, \
        .line = __LINE__, \
    }; \
    mon_##func(&____m, start, len, dir); \
})

#define BSP_dma_map_single(u4Start, u4Len, dir)   MON2(BSP_dma_map_single, u4Start, u4Len, dir)
#define BSP_dma_unmap_single(u4Start, u4Len, dir) MON2(BSP_dma_unmap_single, u4Start, u4Len, dir)

extern UINT32 mon_BSP_dma_map_single(mon_data *m, UINT32 u4Start, UINT32 u4Len, enum data_direction dir);
extern void mon_BSP_dma_unmap_single(mon_data *m, UINT32 u4Start, UINT32 u4Len, enum data_direction dir);


#define BSP_dma_map_vaddr(u4Start, u4Len, dir) MON2(BSP_dma_map_vaddr, u4Start, u4Len, dir)
#define BSP_dma_unmap_vaddr(u4Start, u4Len, dir) MON2(BSP_dma_unmap_vaddr, u4Start, u4Len, dir)

extern void mon_BSP_dma_map_vaddr(mon_data *m, UINT32 u4Start, UINT32 u4Len, enum data_direction dir);
extern void mon_BSP_dma_unmap_vaddr(mon_data *m, UINT32 u4Start, UINT32 u4Len, enum data_direction dir);
#endif

#endif // _CACHE_OPERATION_H_
