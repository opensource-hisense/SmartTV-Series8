/*
 *  linux/arch/arm/mach-MT85XX/pll.c
 *
 * Copyright (C) 2009 MediaTek Inc
 *
 * 
 *
 * pll settings.
 */

//==================================================
// header files
//==================================================
#include <linux/module.h>
#include <linux/linkage.h>
#include "cache_operation.h"
#include <asm/cacheflush.h>
//#include <mach/mt85xx.h>
#include <linux/dma-mapping.h>
#include <linux/sched.h>
#include <asm/tlbflush.h>
//==================================================
// Global
//==================================================
#if __LINUX_ARM_ARCH__ >= 7
unsigned int u4Inv_Max=0x80000000;
unsigned int u4Clean_Max=0x80000000;
unsigned int u4Flush_Max=0x80000000;
#else
unsigned int u4Inv_Max=INV_L1_RANGE_MAX;
unsigned int u4Clean_Max=CLEAN_L1_RANGE_MAX;
unsigned int u4Flush_Max=FLUSH_L1_RANGE_MAX;
#endif
//==================================================
// Define
//==================================================
extern void l2x0_clean_all(void);
extern void l2x0_inv_all(void);
extern void l2x0_flush_all(void);
extern void l2x0_flush_all_by_index(void);
//==================================================
// Private functions
//==================================================

#if (__LINUX_ARM_ARCH__ >= 7)
inline void _hal_clean_d_cache_range(UINT32 u4Start, UINT32 u4End)
{
	  u4Start=u4Start & ~(0x1F); // cache line align
      __asm__ volatile(
    "1:  \n"
    "    MCR p15, 0, %0, c7, c10, 1 \n"
    "    ADD	%0, %0, #32 \n"
    "    CMP  %0, %1 \n"
    "    BLO 1b \n"
    "    ISB \n"
    "    DSB \n"
    :
    : "r" (u4Start), "r" (u4End)
		: "cc"
  );
}

inline void _hal_inv_d_cache_range(UINT32 u4Start, UINT32 u4End)
{
  u4Start=u4Start & ~(0x1F); // cache line align
      __asm__ volatile(
    "1:  \n"
    "    MCR p15, 0, %0, c7, c6, 1 \n"
    "    ADD	%0, %0, #32 \n"
    "    CMP  %0, %1 \n"
    "    BLO 1b \n"
    "    ISB \n"
    "    DSB \n"
    :
    : "r" (u4Start), "r" (u4End)
		: "cc"
  );
}

inline void _hal_flush_d_cache_range(UINT32 u4Start, UINT32 u4End)
{
  u4Start=u4Start & ~(0x1F); // cache line align
    __asm__ volatile(
    "1:  \n"
    "    MCR p15, 0, %0, c7, c14, 1 \n"
    "    ADD	%0, %0, #32 \n"
    "    CMP  %0, %1 \n"
    "    BLO 1b \n"
    "    ISB \n"
    "    DSB \n"
    :
    : "r" (u4Start), "r" (u4End)
		: "cc"
  );
}

inline void _hal_clean_d_cache(void)
{
  flush_cache_all();
}

inline void _hal_flush_d_cache(void)
{
  flush_cache_all();
}
#elif __LINUX_ARM_ARCH__ == 6
inline void _hal_clean_d_cache_range(UINT32 u4Start, UINT32 u4End)
{
    INT32 r = 0;
    asm volatile(
        "mcrr   p15, 0, %1, %0, c12\n"
        "mcr    p15, 0, %2, c7, c10, 4\n"
        :
        : "r" (u4Start), "r" (u4End), "r"(r)
        : "cc"
    );
}
inline void _hal_inv_d_cache_range(UINT32 u4Start, UINT32 u4End)
{
    INT32 r = 0;
    asm volatile(
        "mcrr   p15, 0, %1, %0, c6\n"
        "mcr    p15, 0, %2, c7, c10, 4\n"
        :
        : "r" (u4Start), "r" (u4End), "r"(r)
        : "cc"
    );
}
inline void _hal_flush_d_cache_range(UINT32 u4Start, UINT32 u4End)
{
    INT32 r = 0;
    asm volatile(
        "mcrr   p15, 0, %1, %0, c14\n"
        "mcr    p15, 0, %2, c7, c10, 4\n"
        :
        : "r" (u4Start), "r" (u4End), "r"(r)
        : "cc"
    );
}

inline void _hal_clean_d_cache(void)
{
  UINT32 r = 0;

  __asm__ volatile(
    "mcr    p15, 0, %0, c7, c10, 0\n"
        "mcr    p15, 0, %0, c7, c10, 4\n"
    :
    : "r" (r)
    : "cc"
  );
}

inline void _hal_flush_d_cache(void)
{
  UINT32 r = 0;

  __asm__ volatile(
    "mcr    p15, 0, %0, c7, c14, 0\n"
        "mcr    p15, 0, %0, c7, c10, 4\n"
    :
    : "r" (r)
    : "cc"
  );
}
#else
  #error - miss core version
#endif
//==================================================
// Debug functions
//==================================================
#define MT85XX_CACHE_OPERATION_DEBUG 0
#if MT85XX_CACHE_OPERATION_DEBUG
#define cacheline_size 32
void BSP_CACHEOPERATION_CHECK(UINT32 u4Start, UINT32 u4Len, UINT32 u4Limit)
{
    UINT32 u4End;
    u4End=(u4Start + u4Len - 0x1);
	
	BUG_ON(!virt_addr_valid(u4Start) || !virt_addr_valid(u4End));
	BUG_ON( u4Start & (cacheline_size-1));	
	BUG_ON( u4Len   & (cacheline_size-1));
	BUG_ON( (u4Len == 0) );
}

#endif
//==================================================
// public functions
//==================================================

void bsp_flush_L1(unsigned long u4Start, unsigned long u4Len)
{
    flush_cache_all();
}

void bsp_clean_L1(unsigned long u4Start, unsigned long u4Len)
{
    _hal_clean_d_cache();
}


void bsp_inv_L1(unsigned long u4Start, unsigned long u4Len)
{
    flush_cache_all();             //inv all is forbidden
}

void (*bsp_flush_operation)(unsigned long, unsigned long)=bsp_flush_L1;
void (*bsp_clean_operation)(unsigned long, unsigned long)=bsp_clean_L1;
void (*bsp_inv_operation)(unsigned long, unsigned long)=bsp_inv_L1;

#ifdef CONFIG_OUTER_CACHE
//extern void bsp_flush_L1L2(void);
//extern void bsp_clean_L1L2(void);
//extern void bsp_inv_L1L2(void);
void __init BSP_L2Cache_Operation_Init(void)
{
        //bsp_flush_operation=bsp_flush_L1L2;
        //bsp_clean_operation=bsp_clean_L1L2;
        //bsp_inv_operation=bsp_inv_L1L2;
        printk(KERN_INFO "BSP_L2Cache_Operation_Init\n");
}
#endif

static inline void check_cache_line(UINT32 x)
{
    if(x & (CACHE_LINE_UNIT-1))
    {
        printk(KERN_ERR "check_cache_line=0x%08x not align, pls modify it...\n", x);
        dump_stack();
    } else {
        return;
    }   
}

static inline void _sync_pmd(unsigned long addr)
{
    unsigned int index = pgd_index(addr);
    pgd_t *pgd_k, *pgd;
    pmd_t *pmd_k, *pmd;
    pgd_k = init_mm.pgd + index;
    pgd = cpu_get_pgd() + index;
    pmd_k = pmd_offset((pud_t *)pgd_k, addr);
    pmd = pmd_offset((pud_t *)pgd, addr);
    if (pmd_present(*pmd_k))
        copy_pmd(pmd, pmd_k);
}

static inline unsigned long vaddr_to_phys(void const *x)
{
   unsigned long flags, i;
    local_irq_save(flags);
    asm volatile (
    "mcr  p15, 0, %1, c7, c8, 0\n"
    "  mrc p15, 0, %0, c7, c4, 0\n"
    : "=r" (i) : "r" (x) : "cc");
    if (i & 1)
    {
       _sync_pmd((unsigned long)x);
        asm volatile (
        "mcr  p15, 0, %1, c7, c8, 0\n"
        "  mrc p15, 0, %0, c7, c4, 0\n"
        : "=r" (i) : "r" (x) : "cc");
    }
   local_irq_restore(flags);
    if (i & 1)
        return 0;
    return (i & PAGE_MASK) | ((unsigned long)(x) & ~PAGE_MASK);
}
#if (Linux_MonCacheOperation == 1)
#undef BSP_FlushDCacheRange
#undef BSP_CleanDCacheRange
#undef BSP_InvDCacheRange
extern void BSP_FlushDCacheRange(UINT32 u4Start, UINT32 u4Len);
extern void BSP_CleanDCacheRange(UINT32 u4Start, UINT32 u4Len);
extern void BSP_InvDCacheRange(UINT32 u4Start, UINT32 u4Len);

void BSP_dma_map_vaddr_intl(UINT32 u4Start, UINT32 u4Len, enum data_direction dir)
{
#if MT85XX_CACHE_OPERATION_DEBUG
    check_cache_line(u4Start);
    check_cache_line(u4Len);
#endif
#if __LINUX_ARM_ARCH__ == 6
    if (dir == FROM_DEVICE && u4Len > u4Inv_Max)
    {
        bsp_inv_operation(u4Start, u4Len);
        return;
    }
    if (dir == TO_DEVICE && u4Len > u4Clean_Max)
    {
        bsp_clean_operation(u4Start, u4Len);
        return;
    }
    if(dir == BIDIRECTIONAL && u4Len > u4Flush_Max)
    {
        bsp_flush_operation(u4Start, u4Len);
        return;
    }
#endif
    if (virt_addr_valid(u4Start))
    {
        dma_map_single(NULL, (void *)u4Start, u4Len, dir);
    }
    else
    {
        UINT32 u4End = u4Start + u4Len;
        UINT32 i = u4Start;
        while (i < u4End)
        {
            UINT32 j = (i + PAGE_SIZE) & PAGE_MASK;
            if (j > u4End) j = u4End;
            dma_map_single(NULL, __va(vaddr_to_phys((void *)i)), (j - i), dir);
            i = j;
        }
    }
}
//EXPORT_SYMBOL(BSP_dma_map_vaddr);

void BSP_dma_unmap_vaddr_intl(UINT32 u4Start, UINT32 u4Len, enum data_direction dir)
{
#if __LINUX_ARM_ARCH__ >= 7
#if MT85XX_CACHE_OPERATION_DEBUG
    check_cache_line(u4Start);
    check_cache_line(u4Len);
#endif
    if (virt_addr_valid(u4Start))
    {
        dma_unmap_single(NULL, __pa(u4Start), u4Len, dir);
    }
    else
    {
        UINT32 u4End = u4Start + u4Len;
        UINT32 i = u4Start;
        while (i < u4End)
        {
            UINT32 j = (i + PAGE_SIZE) & PAGE_MASK;
            if (j > u4End) j = u4End;
            dma_unmap_single(NULL, vaddr_to_phys((void *)i), (j - i), dir);
            i = j;
        }
    }
#endif
}
//EXPORT_SYMBOL(BSP_dma_unmap_vaddr);
#else

UINT32 BSP_dma_map_single(UINT32 u4Start, UINT32 u4Len, enum data_direction dir)
{
#if MT85XX_CACHE_OPERATION_DEBUG
    check_cache_line(u4Start);
    check_cache_line(u4Len);
#endif
#if __LINUX_ARM_ARCH__ == 6
    if (dir == FROM_DEVICE && u4Len > u4Inv_Max)
    {
        bsp_inv_operation(u4Start, u4Len);
        return __pa(u4Start);
    }
    if (dir == TO_DEVICE && u4Len > u4Clean_Max)
    {
        bsp_clean_operation(u4Start, u4Len);
        return __pa(u4Start);
    }
    if (dir == BIDIRECTIONAL && u4Len > u4Flush_Max)
    {
        bsp_flush_operation(u4Start, u4Len);
        return __pa(u4Start);
    }
#endif
    return dma_map_single(NULL, (void *)u4Start, u4Len, dir);
}
EXPORT_SYMBOL(BSP_dma_map_single);

void BSP_dma_unmap_single(UINT32 u4Start, UINT32 u4Len, enum data_direction dir)
{
#if __LINUX_ARM_ARCH__ >= 7
#if MT85XX_CACHE_OPERATION_DEBUG
    check_cache_line(u4Start);
    check_cache_line(u4Len);
#endif
    dma_unmap_single(NULL, u4Start, u4Len, dir);
#endif
}
EXPORT_SYMBOL(BSP_dma_unmap_single);

void BSP_dma_map_vaddr(UINT32 u4Start, UINT32 u4Len, enum data_direction dir)
{
#if MT85XX_CACHE_OPERATION_DEBUG
    check_cache_line(u4Start);
    check_cache_line(u4Len);
#endif
#if __LINUX_ARM_ARCH__ == 6
    if (dir == FROM_DEVICE && u4Len > u4Inv_Max)
    {
        bsp_inv_operation(u4Start, u4Len);
        return;
    }
    if (dir == TO_DEVICE && u4Len > u4Clean_Max)
    {
        bsp_clean_operation(u4Start, u4Len);
        return;
    }
    if(dir == BIDIRECTIONAL && u4Len > u4Flush_Max)
    {
        bsp_flush_operation(u4Start, u4Len);
        return;
    }
#endif
    if (virt_addr_valid(u4Start))
    {
        dma_map_single(NULL, (void *)u4Start, u4Len, dir);
    }
    else
    {
        UINT32 u4End = u4Start + u4Len;
        UINT32 i = u4Start;
        while (i < u4End)
        {
            UINT32 j = (i + PAGE_SIZE) & PAGE_MASK;
            if (j > u4End) j = u4End;
            dma_map_single(NULL, __va(vaddr_to_phys((void *)i)), (j - i), dir);
            i = j;
        }
    }
}
EXPORT_SYMBOL(BSP_dma_map_vaddr);

void BSP_dma_unmap_vaddr(UINT32 u4Start, UINT32 u4Len, enum data_direction dir)
{
#if __LINUX_ARM_ARCH__ >= 7
#if MT85XX_CACHE_OPERATION_DEBUG
    check_cache_line(u4Start);
    check_cache_line(u4Len);
#endif
    if (virt_addr_valid(u4Start))
    {
        dma_unmap_single(NULL, __pa(u4Start), u4Len, dir);
    }
    else
    {
        UINT32 u4End = u4Start + u4Len;
        UINT32 i = u4Start;
        while (i < u4End)
        {
            UINT32 j = (i + PAGE_SIZE) & PAGE_MASK;
            if (j > u4End) j = u4End;
            dma_unmap_single(NULL, vaddr_to_phys((void *)i), (j - i), dir);
            i = j;
        }
    }
#endif
}
EXPORT_SYMBOL(BSP_dma_unmap_vaddr);

#endif


#if 0
static void sync_single(UINT32 u4Start, UINT32 u4Len, enum data_direction dir)
{
    if (virt_addr_valid(u4Start))
    {
        dma_map_single(NULL, (void *)u4Start, u4Len, dir);
    }
    else
    {
        UINT32 u4End = u4Start + u4Len;
        UINT32 i = u4Start;
        while (i < u4End)
        {
            UINT32 j = (i + PAGE_SIZE) & PAGE_MASK;
            if (j > u4End) j = u4End;
            dma_sync_single_for_device(NULL, vaddr_to_phys((void *)i), (j - i), dir);
            i = j;
        }
    }
}
#endif

#if __LINUX_ARM_ARCH__ == 6
void BSP_FlushDCacheRange(UINT32 u4Start, UINT32 u4Len)
{
    /* flush data cache range */
    UINT32 u4End;
    u4End=(u4Start + u4Len);
    if(u4Len > u4Flush_Max)
    {
        bsp_flush_operation(u4Start, u4Len);
    }
    else if (virt_addr_valid(u4Start))
    {
        //dma_sync_single_for_device(NULL, u4Start,u4Len, DMA_TO_DEVICE);
        //dma_sync_single_for_cpu(NULL, u4Start,u4Len, DMA_FROM_DEVICE);
        _hal_flush_d_cache_range(u4Start, (u4Start + u4Len - 0x1));
    #ifdef CONFIG_OUTER_CACHE
        if(outer_cache.flush_range)
        {
            outer_cache.flush_range(__pa(u4Start), __pa(u4End));
        }
    #endif
    }
    else
    {
        UINT32 i = u4Start;
        unsigned long paddr;
        while (i < u4End)
        {
            UINT32 j = (i + PAGE_SIZE) & PAGE_MASK;
            if (j > u4End) j = u4End;
            dmac_map_area(i, (j - i), DMA_BIDIRECTIONAL);
            paddr = vaddr_to_phys((void *)i);
            outer_flush_range(paddr, paddr + (j - i));
            i = j;
        }
    }
}
EXPORT_SYMBOL(BSP_FlushDCacheRange);

void BSP_CleanDCacheRange(UINT32 u4Start, UINT32 u4Len)
{
    /* clean data cache range */
    if(u4Len > u4Clean_Max)
    {
        bsp_clean_operation(u4Start, u4Len);
    }
    else if (virt_addr_valid(u4Start))
    {
        dma_sync_single_for_device(NULL, virt_to_dma(NULL,u4Start),u4Len, DMA_TO_DEVICE);
    }
    else
    {
        sync_single(u4Start, u4Len, DMA_TO_DEVICE);
    }  
}
EXPORT_SYMBOL(BSP_CleanDCacheRange);

void BSP_InvDCacheRange(UINT32 u4Start, UINT32 u4Len)
{
    /* inv data cache range */
    if(u4Len > u4Inv_Max)
    {
        bsp_inv_operation(u4Start, u4Len);
    }
    else if (virt_addr_valid(u4Start))
    {
        dma_sync_single_for_cpu(NULL, virt_to_dma(NULL,u4Start),u4Len, DMA_FROM_DEVICE);
    }
    else
    {
        sync_single(u4Start, u4Len, DMA_FROM_DEVICE);
    }  
}
EXPORT_SYMBOL(BSP_InvDCacheRange);
#endif

/*For DualCore Share data*/
void Core_FlushDCacheRange(UINT32 u4Start, UINT32 u4Len)
{
    #if MT85XX_CACHE_OPERATION_DEBUG
    BSP_CACHEOPERATION_CHECK(u4Start,u4Len,CORE_FLUSH_RANGE_MAX);
    #endif
    /* flush data cache range */
        _hal_flush_d_cache_range(u4Start, (u4Start + u4Len - 0x1));
    }
EXPORT_SYMBOL(Core_FlushDCacheRange);

void Core_CleanDCacheRange(UINT32 u4Start, UINT32 u4Len)
{
    #if MT85XX_CACHE_OPERATION_DEBUG
    BSP_CACHEOPERATION_CHECK(u4Start,u4Len,CORE_CLEAN_RANGE_MAX);
    #endif
    /* clean data cache range */
        _hal_clean_d_cache_range(u4Start, (u4Start + u4Len - 0x1));
    }
EXPORT_SYMBOL(Core_CleanDCacheRange);

void Core_CleanOuterCacheRange(UINT32 u4Start, UINT32 u4Len)
{
    UINT32 u4End;
    u4End=(u4Start + u4Len - 0x1);
    #ifdef CONFIG_OUTER_CACHE
        if(outer_cache.clean_range)
        {
        	  if(u4Start & 0x40000000)
        	  	{
        	  	 outer_cache.clean_range(u4Start, u4End);	
        	  	}        	  
        	  else
        	  	{        	  			
                outer_cache.clean_range(__pa(u4Start), __pa(u4End));
              } 
        }
    #endif
    }
EXPORT_SYMBOL(Core_CleanOuterCacheRange);

void Core_InvDCacheRange(UINT32 u4Start, UINT32 u4Len)
{
    #if MT85XX_CACHE_OPERATION_DEBUG
    BSP_CACHEOPERATION_CHECK(u4Start,u4Len,CORE_INV_RANGE_MAX);
    #endif
        _hal_inv_d_cache_range(u4Start, (u4Start + u4Len - 0x1));
    }
EXPORT_SYMBOL(Core_InvDCacheRange);

/* Below are the test code*/
// Range operation
void TST_BSP_Clean_L1_DCacheRange(UINT32 u4Start, UINT32 u4Len) //Test only
{
    UINT32 u4End;
    u4End=(u4Start + u4Len - 0x1);
    _hal_clean_d_cache_range(u4Start, (u4Start + u4Len - 0x1));
}
EXPORT_SYMBOL(TST_BSP_Clean_L1_DCacheRange);

void TST_BSP_Clean_L2_DCacheRange(UINT32 u4Start, UINT32 u4Len) //Test only
{
    UINT32 u4End;
    u4End=(u4Start + u4Len - 0x1);
    #ifdef CONFIG_OUTER_CACHE
    if(outer_cache.clean_range)
    {
        outer_cache.clean_range(__pa(u4Start), __pa(u4End));
    }
    #endif
}
EXPORT_SYMBOL(TST_BSP_Clean_L2_DCacheRange);

void TST_BSP_Invalidate_L1_DCacheRange(UINT32 u4Start, UINT32 u4Len) //Test only
{
    UINT32 u4End;
    u4End=(u4Start + u4Len - 0x1);
    _hal_inv_d_cache_range(u4Start, (u4Start + u4Len - 0x1));
}
EXPORT_SYMBOL(TST_BSP_Invalidate_L1_DCacheRange);

void TST_BSP_Invalidate_L2_DCacheRange(UINT32 u4Start, UINT32 u4Len) //Test only
{
    UINT32 u4End;
    u4End=(u4Start + u4Len - 0x1);
    #ifdef CONFIG_OUTER_CACHE
    if(outer_cache.inv_range)
    {
        outer_cache.inv_range(__pa(u4Start), __pa(u4End));
    }
    #endif
}
EXPORT_SYMBOL(TST_BSP_Invalidate_L2_DCacheRange);

void TST_BSP_Flush_L1_DCacheRange(UINT32 u4Start, UINT32 u4Len) //Test only
{
    UINT32 u4End;
    u4End=(u4Start + u4Len - 0x1);
    _hal_flush_d_cache_range(u4Start, (u4Start + u4Len - 0x1));
}
EXPORT_SYMBOL(TST_BSP_Flush_L1_DCacheRange);

void TST_BSP_Flush_L2_DCacheRange(UINT32 u4Start, UINT32 u4Len) //Test only
{
    UINT32 u4End;
    u4End=(u4Start + u4Len - 0x1);
    #ifdef CONFIG_OUTER_CACHE
    if(outer_cache.flush_range)
    {
        outer_cache.flush_range(__pa(u4Start), __pa(u4End));
    }
    #endif
}
EXPORT_SYMBOL(TST_BSP_Flush_L2_DCacheRange);

// Entire operation
void TST_BSP_Clean_L1_DCache(void) //Test only
{
    _hal_clean_d_cache();
}
EXPORT_SYMBOL(TST_BSP_Clean_L1_DCache);

void TST_BSP_Clean_L2_DCache(UINT32 u4Start, UINT32 u4Len) //Test only
{

}
EXPORT_SYMBOL(TST_BSP_Clean_L2_DCache);
/*
void TST_BSP_Invalidate_L1_DCache() //Test only
{
    _hal_inv_d_cache();
}
EXPORT_SYMBOL(TST_BSP_Invalidate_L1_DCache);
*/
void TST_BSP_Invalidate_L2_DCache(UINT32 u4Start, UINT32 u4Len) //Test only
{

}
EXPORT_SYMBOL(TST_BSP_Invalidate_L2_DCache);

void TST_BSP_Flush_L1_DCache(void) //Test only
{
    _hal_flush_d_cache();
}
EXPORT_SYMBOL(TST_BSP_Flush_L1_DCache);

void TST_BSP_Flush_L2_DCache(UINT32 u4Start, UINT32 u4Len) //Test only
{

}
EXPORT_SYMBOL(TST_BSP_Flush_L2_DCache);

void TST_BSP_Flush_L2_DCache_BY_Index(UINT32 u4Start, UINT32 u4Len) //Test only
{

}
EXPORT_SYMBOL(TST_BSP_Flush_L2_DCache_BY_Index);

#if 0
int x_kmem_sync_table(void *ptr, size_t size)
{
    pgd_t *pgd_k = init_mm.pgd;
    pgd_t *pgd = cpu_get_pgd();
    pud_t *pud, *pud_k;
    pmd_t *pmd, *pmd_k;
    unsigned int index;
    unsigned long addr = (unsigned long)(ptr), end = addr + size;
        
    if(addr >= MODULES_VADDR)//kernel space addr
    {       
        while (addr < end)
        {
#if 0
            unsigned int index = pgd_index(addr);
            pmd_t *pmd = pmd_offset(pgd + index, addr);
            if (pmd_none(*pmd))
            {
                pmd_t *pmd_k = pmd_offset(pgd_k + index, addr);
                if (pmd_none(*pmd_k))
                    return -EINVAL;
                copy_pmd(pmd, pmd_k);
            }
#endif
            // refer to do_translation_fault
            index = pgd_index(addr);
            pgd = cpu_get_pgd() + index;
            pgd_k = init_mm.pgd + index;

            if (pgd_none(*pgd_k))
                goto bad_area;
            if (!pgd_present(*pgd))
                set_pgd(pgd, *pgd_k);

	    //__cpuc_flush_dcache_area((void*)pgd, 4);

            pud = pud_offset(pgd, addr);
            pud_k = pud_offset(pgd_k, addr);

            if (pud_none(*pud_k))
                goto bad_area;
            if (!pud_present(*pud))
                set_pud(pud, *pud_k);

            pmd = pmd_offset(pud, addr);
            pmd_k = pmd_offset(pud_k, addr);

            /*
             * On ARM one Linux PGD entry contains two hardware entries (see page
             * tables layout in pgtable.h). We normally guarantee that we always
             * fill both L1 entries. But create_mapping() doesn't follow the rule.
             * It can create inidividual L1 entries, so here we have to call
             * pmd_none() check for the entry really corresponded to address, not
             * for the first of pair.
             */
            index = (addr >> SECTION_SHIFT) & 1;

            if (pmd_none(pmd_k[index]))
                goto bad_area;

            copy_pmd(pmd, pmd_k);
	    __cpuc_flush_dcache_area((void*)pmd, 8);

            if (*pmd & 1)
            {
                unsigned long start = (*pmd) & ~0x7FF;
		__cpuc_flush_dcache_area(__va((void*)start), 4096);
                outer_clean_range(start, start + 4096);
            }
            addr = (addr + PMD_SIZE) & PMD_MASK;
        }
    }

    //outer_clean_range(__pa(pgd) + ((unsigned long)ptr >> 23 << 5), __pa(pgd) + ((end + 0x7FFFFF) >> 23 << 5));
    //outer_clean_range(__pa(pgd_k) + ((unsigned long)ptr >> 23 << 5), __pa(pgd_k) + ((end + 0x7FFFFF) >> 23 << 5));
    return 0;

bad_area:
    printk("x_kmem_sync_table fail\n");
    return 1;
}
EXPORT_SYMBOL(x_kmem_sync_table);
#endif


//==================================================
// Monitor functions
//==================================================
#if (Linux_MonCacheOperation == 1)

_Bool mon;
static mon_data head = {
   .next = &head,
};
static DEFINE_SPINLOCK(mon_lock);
#if __LINUX_ARM_ARCH__ == 6
#define MON_IMPL(func) \
void mon_##func(mon_data *m, UINT32 u4Start, UINT32 u4Len) \
{ \
    if (mon) { \
        unsigned long flags; \
        spin_lock_irqsave(&mon_lock, flags); \
        if (!m->next) { \
            m->next = head.next; \
            head.next = m; \
        } \
        m->count++; \
        m->amount += u4Len; \
        spin_unlock_irqrestore(&mon_lock, flags); \
    } \
    func(u4Start, u4Len); \
} \
EXPORT_SYMBOL(mon_##func); \

MON_IMPL(BSP_FlushDCacheRange)
MON_IMPL(BSP_CleanDCacheRange)
MON_IMPL(BSP_InvDCacheRange)
#endif

void mon_dump(void)
{
    mon_data *p;
    for (p = head.next; p != &head; p = p->next)
    {
        printk("%s:%d\t%d\t%lld\n", p->file, p->line, p->count, p->amount);
    }
}
EXPORT_SYMBOL(mon);
EXPORT_SYMBOL(mon_dump);

UINT32 mon_BSP_dma_map_single(mon_data *m, UINT32 u4Start, UINT32 u4Len, enum data_direction dir)
{
    if (mon) {
        unsigned long flags;
        spin_lock_irqsave(&mon_lock, flags);
        if (!m->next) {
            m->next = head.next;
            head.next = m;
        }
        m->count++;
        m->amount += u4Len;
        spin_unlock_irqrestore(&mon_lock, flags);
    }
    return dma_map_single(NULL, (void *)u4Start, u4Len, dir);
}
EXPORT_SYMBOL(mon_BSP_dma_map_single);

void mon_BSP_dma_unmap_single(mon_data *m, UINT32 u4Start, UINT32 u4Len, enum data_direction dir)
{
    if (mon) {
        unsigned long flags;
        spin_lock_irqsave(&mon_lock, flags);
        if (!m->next) {
            m->next = head.next;
            head.next = m;
        }
        m->count++;
        m->amount += u4Len;
        spin_unlock_irqrestore(&mon_lock, flags);
    }
    dma_unmap_single(NULL, u4Start, u4Len, dir);
}
EXPORT_SYMBOL(mon_BSP_dma_unmap_single);


void mon_BSP_dma_map_vaddr(mon_data *m, UINT32 u4Start, UINT32 u4Len, enum data_direction dir)
{
    if (mon) {
        unsigned long flags;
        spin_lock_irqsave(&mon_lock, flags);
        if (!m->next) {
            m->next = head.next;
            head.next = m;
        }
        m->count++;
        m->amount += u4Len;
        spin_unlock_irqrestore(&mon_lock, flags);
    }
    BSP_dma_map_vaddr_intl(u4Start, u4Len, dir);
}
EXPORT_SYMBOL(mon_BSP_dma_map_vaddr);


void mon_BSP_dma_unmap_vaddr(mon_data *m, UINT32 u4Start, UINT32 u4Len, enum data_direction dir)
{
    if (mon) {
        unsigned long flags;
        spin_lock_irqsave(&mon_lock, flags);
        if (!m->next) {
            m->next = head.next;
            head.next = m;
        }
        m->count++;
        m->amount += u4Len;
        spin_unlock_irqrestore(&mon_lock, flags);
    }
    BSP_dma_unmap_vaddr_intl(u4Start, u4Len, dir);
}
EXPORT_SYMBOL(mon_BSP_dma_unmap_vaddr);

#endif
