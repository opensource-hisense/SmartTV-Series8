/*
 *lib_arm\hal_ca9.c
 *
 * MT53xx common board function
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
 *
 *---------------------------------------------------------------------------*/

/** @file hal.c
 *  Hardware abstraction rountines.
 */


//-----------------------------------------------------------------------------
// Include files
//-----------------------------------------------------------------------------

#include "x_bim.h"


//-----------------------------------------------------------------------------
// Constant definitions
//-----------------------------------------------------------------------------

#define ICACHE_ENABLE               (1 << 12)
#define DCACHE_ENABLE               (1 << 2)
#define MMU_ENABLE                  (1 << 0)
#define CTYPE_MASK                  0xf
#define CTYPE_SHIFT                 0x25
#define S_MASK                      1
#define S_SHIFT                     24
#define DSIZE_MASK                  0xf
#define DSIZE_SHIFT                 (6 + 12)
#define ISIZE_MASK                  0xf
#define ISIZE_SHIFT                 6

#define SMP                         (1 << 6)

// Translate VA (Virtual Address) to MVA (Modified Virtual Address)
#if defined(CONFIG_ARCH_MT5882)
#define MVA(addr)                   ((addr) & 0xffffffc0)
#define ICACHE_LINE_SIZE            32
#define DCACHE_LINE_SIZE            64
#define L1_SIZE                     32*1024
#define L2_SIZE                     512*1024
#elif defined(CONFIG_ARCH_MT5883)
#define MVA(addr)                   ((addr) & 0xffffffc0)
#define ICACHE_LINE_SIZE            32
#define DCACHE_LINE_SIZE            64
#define L1_SIZE                     32*1024
#define L2_SIZE                     128*1024
#else
#define MVA(addr)                   ((addr) & 0xffffffc0)
#define ICACHE_LINE_SIZE            64
#define DCACHE_LINE_SIZE            64
#define L1_SIZE                     32*1024
#define L2_SIZE                     2*1024*1024
#endif

// Cache parameters (in bytes)
#define DCACHE_LINE_SIZE_MSK        (DCACHE_LINE_SIZE - 1)
#define DCACHE_FLUSH_THRESHOLD      (DCACHE_LINE_SIZE * 256)

//-----------------------------------------------------------------------------
// Public functions
//-----------------------------------------------------------------------------

void _DMB(void)
{
    __asm__ __volatile__ ("dmb" : : : "memory");
}

void _DSB(void)
{
    __asm__ __volatile__ ("dsb" : : : "memory");
}

void _ISB(void)
{
    __asm__ __volatile__ ("isb" : : : "memory");
}

void HalFlushWriteBuffer(void)
{
    __asm__ __volatile__ ("dsb" : : : "memory");
}

void _HalSelectDCacheLevel(UINT32 u4Level)
{
	UINT32 r;
	//ASSERT(u4Level >= 1);

	r = (u4Level - 1) << 1;
    __asm__ ("MCR   p15, 2, %0, c0, c0, 0" : : "r" (r));
   	_ISB();
	__asm__ ("MRC   p15, 1, %0, c0, c0, 0" : "=r" (r));

	//Printf("Cache size: 0x%x now1 now1 code.\n", r);
}

void HalFlushInvalidateDCacheSingleLine(UINT32 u4Addr)
{
    u4Addr = MVA(u4Addr);

    __asm__ ("MCR   p15, 0, %0, c7, c14, 1" : : "r" (u4Addr));
}

void _FlushInvalidateDCache(void)
{
		asm volatile("push {r0, r1, r2, r3, r4, r5, r6, r7, r9, r10, r11}\n\t");
        asm volatile(
            "dmb\n\t"
            "mrc p15, 1, r0, c0, c0, 1\n\t"
            "ands    r3, r0, #0x7000000\n\t"
            "mov r3, r3, lsr #23\n\t"
            "beq 5f\n\t" // finished
            "mov r10, #0\n\t"
        "1:\n\t" // flush_levels
            "add r2, r10, r10, lsr #1\n\t"
            "mov r1, r0, lsr r2\n\t"
            "and r1, r1, #7\n\t"
            "cmp r1, #2\n\t"
            "blt 4f\n\t" // skip
#ifdef CONFIG_PREEMPT
//            "save_and_disable_irqs_notrace r9\n\t"
#endif
            "mcr p15, 2, r10, c0, c0, 0\n\t"
            "isb\n\t"
            "mrc p15, 1, r1, c0, c0, 0\n\t"
#ifdef CONFIG_PREEMPT
//            "restore_irqs_notrace r9\n\t"
#endif
            "and r2, r1, #7\n\t"
            "add r2, r2, #4\n\t"
            "ldr r4, =0x3ff\n\t"
            "ands    r4, r4, r1, lsr #3\n\t"
            "clz r5, r4\n\t"
            "ldr r7, =0x7fff\n\t"
            "ands    r7, r7, r1, lsr #13\n\t"
        "2:\n\t" // loop1
            "mov r9, r4\n\t"
        "3:\n\t" // loop2
            "orr r11, r10, r9, lsl r5\n\t"
            "orr r11, r11, r7, lsl r2\n\t"
            "mcr p15, 0, r11, c7, c14, 2\n\t"
            "subs    r9, r9, #1\n\t"
            "bge 3b\n\t" // loop2
            "subs    r7, r7, #1\n\t"
            "bge 2b\n\t" //loop1
        "4:\n\t" // skip
            "add r10, r10, #2\n\t"
            "cmp r3, r10\n\t"
            "bgt 1b\n\t" // flush_levels
        "5:\n\t" // finished
            "mov r10, #0\n\t"
            "mcr p15, 2, r10, c0, c0, 0\n\t"
            "dsb\n\t"
            "isb\n\t"
        ::: "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r9", "r10", "r11", "cc", "memory");
		asm volatile("pop {r0, r1, r2, r3, r4, r5, r6, r7, r9, r10, r11}\n\t");
}

void HalFlushInvalidateDCache(void)
{
    _FlushInvalidateDCache();
    HalFlushWriteBuffer();
}

void HalInvalidateDCache(void)
{
		asm volatile("push {r0, r1, r2, r3, r4, r5, r6, r7, r9, r10, r11}\n\t");
        asm volatile(
            "dmb\n\t"
            "mrc p15, 1, r0, c0, c0, 1\n\t"
            "ands    r3, r0, #0x7000000\n\t"
            "mov r3, r3, lsr #23\n\t"
            "beq 5f\n\t" // finished
            "mov r10, #0\n\t"
        "1:\n\t" // flush_levels
            "add r2, r10, r10, lsr #1\n\t"
            "mov r1, r0, lsr r2\n\t"
            "and r1, r1, #7\n\t"
            "cmp r1, #2\n\t"
            "blt 4f\n\t" // skip
#ifdef CONFIG_PREEMPT
//            "save_and_disable_irqs_notrace r9\n\t"
#endif
            "mcr p15, 2, r10, c0, c0, 0\n\t"
            "isb\n\t"
            "mrc p15, 1, r1, c0, c0, 0\n\t"
#ifdef CONFIG_PREEMPT
//            "restore_irqs_notrace r9\n\t"
#endif
            "and r2, r1, #7\n\t"
            "add r2, r2, #4\n\t"
            "ldr r4, =0x3ff\n\t"
            "ands    r4, r4, r1, lsr #3\n\t"
            "clz r5, r4\n\t"
            "ldr r7, =0x7fff\n\t"
            "ands    r7, r7, r1, lsr #13\n\t"
        "2:\n\t" // loop1
            "mov r9, r4\n\t"
        "3:\n\t" // loop2
            "orr r11, r10, r9, lsl r5\n\t"
            "orr r11, r11, r7, lsl r2\n\t"
            "mcr p15, 0, r11, c7, c6, 2\n\t"
            "subs    r9, r9, #1\n\t"
            "bge 3b\n\t" // loop2
            "subs    r7, r7, #1\n\t"
            "bge 2b\n\t" //loop1
        "4:\n\t" // skip
            "add r10, r10, #2\n\t"
            "cmp r3, r10\n\t"
            "bgt 1b\n\t" // flush_levels
        "5:\n\t" // finished
            "mov r10, #0\n\t"
            "mcr p15, 2, r10, c0, c0, 0\n\t"
            "dsb\n\t"
            "isb\n\t"
        ::: "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r9", "r10", "r11", "cc", "memory");
		asm volatile("pop {r0, r1, r2, r3, r4, r5, r6, r7, r9, r10, r11}\n\t");
}

void HalInvalidateDCacheSingleLine(UINT32 u4Addr)
{
    u4Addr = MVA(u4Addr);

    __asm__ ("MCR     p15, 0, %0, c7, c6, 1" : : "r" (u4Addr));
}

void HalFlushDCacheSingleLine(UINT32 u4Addr)
{
    u4Addr = MVA(u4Addr);

    __asm__ ("MCR     p15, 0, %0, c7, c10, 1" : : "r" (u4Addr));
}

BOOL HalIsICacheEnabled(void)
{
    register UINT32 r;

    __asm__ ("MRC   p15, 0, %0, c1, c0, 0" : "=g" (r));

    return ((r & ICACHE_ENABLE) ? TRUE : FALSE);
}

BOOL HalIsDCacheEnabled(void)
{
    register UINT32 r;

    __asm__ ("MRC   p15, 0, %0, c1, c0, 0" : "=g" (r));

    return ((r & DCACHE_ENABLE) ? TRUE : FALSE);
}


BOOL HalIsMMUEnabled(void)
{
    register UINT32 r;

    __asm__ ("MRC   p15, 0, %0, c1, c0, 0" : "=g" (r));

    return ((r & MMU_ENABLE) ? TRUE : FALSE);
}


void _FlushDCache(void)
{
		asm volatile("push {r0, r1, r2, r3, r4, r5, r6, r7, r9, r10, r11}\n\t");
        asm volatile(
            "dmb\n\t"
            "mrc p15, 1, r0, c0, c0, 1\n\t"
            "ands    r3, r0, #0x7000000\n\t"
            "mov r3, r3, lsr #23\n\t"
            "beq 5f\n\t" // finished
            "mov r10, #0\n\t"
        "1:\n\t" // flush_levels
            "add r2, r10, r10, lsr #1\n\t"
            "mov r1, r0, lsr r2\n\t"
            "and r1, r1, #7\n\t"
            "cmp r1, #2\n\t"
            "blt 4f\n\t" // skip
#ifdef CONFIG_PREEMPT
//            "save_and_disable_irqs_notrace r9\n\t"
#endif
            "mcr p15, 2, r10, c0, c0, 0\n\t"
            "isb\n\t"
            "mrc p15, 1, r1, c0, c0, 0\n\t"
#ifdef CONFIG_PREEMPT
//            "restore_irqs_notrace r9\n\t"
#endif
            "and r2, r1, #7\n\t"
            "add r2, r2, #4\n\t"
            "ldr r4, =0x3ff\n\t"
            "ands    r4, r4, r1, lsr #3\n\t"
            "clz r5, r4\n\t"
            "ldr r7, =0x7fff\n\t"
            "ands    r7, r7, r1, lsr #13\n\t"
        "2:\n\t" // loop1
            "mov r9, r4\n\t"
        "3:\n\t" // loop2
            "orr r11, r10, r9, lsl r5\n\t"
            "orr r11, r11, r7, lsl r2\n\t"
            "mcr p15, 0, r11, c7, c10, 2\n\t"
            "subs    r9, r9, #1\n\t"
            "bge 3b\n\t" // loop2
            "subs    r7, r7, #1\n\t"
            "bge 2b\n\t" //loop1
        "4:\n\t" // skip
            "add r10, r10, #2\n\t"
            "cmp r3, r10\n\t"
            "bgt 1b\n\t" // flush_levels
        "5:\n\t" // finished
            "mov r10, #0\n\t"
            "mcr p15, 2, r10, c0, c0, 0\n\t"
            "dsb\n\t"
            "isb\n\t"
        ::: "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r9", "r10", "r11", "cc", "memory");
		asm volatile("pop {r0, r1, r2, r3, r4, r5, r6, r7, r9, r10, r11}\n\t");
}

void HalFlushDCache(void)
{
    _FlushDCache();
    HalFlushWriteBuffer();
}

void HalDisableCaches(void)
{
    register UINT32 r;

    // D-cache must be cleaned of dirty data before it is disabled
    HalFlushDCache();

    __asm__ ("MRC     p15, 0, %0, c1, c0, 0" : "=g" (r));
    //__asm__ ("BIC     %0, %1, %2" : "=g" (r) : "r" (r), "r" (ICACHE_ENABLE));
    __asm__ ("BIC     %0, %1, %2" : "=g" (r) : "r" (r), "r" (DCACHE_ENABLE));
    __asm__ ("MCR     p15, 0, %0, c1, c0, 0" : : "r" (r));
}

//-----------------------------------------------------------------------------
/** HalEnableMMU() Enable MMU
 */
//-----------------------------------------------------------------------------
void HalEnableMMU(void)
{
    register UINT32 r;
    #if defined(CC_MT5891)
    register UINT32 r1;
    #endif

    // TLB content is preserved while enabling MMU, thus a TLB invalidation
    // is necessary.
    HalInvalidateTLB();

    #if defined(CC_MT5891)
	asm volatile(
	"	mrrc p15, 1, %0, %1, c15\n"
	"	orr %0, %0, %4\n"		    // ACTLR.SMP
	"	mcrr p15, 1, %0, %1, c15\n"
	  : "=&r" (r), "=&r" (r1)
	  : "r" (0), "r" (1), "Ir" (0x40)
	  : "cc");
	#else
    __asm__ ("MRC     p15, 0, %0, c1, c0, 1" : "=r" (r));
    __asm__ ("ORR     %0, %1, %2" : "=r" (r) : "r" (r), "r" (SMP));
    __asm__ ("MCR     p15, 0, %0, c1, c0, 1" : : "r" (r));
    #endif
    __asm__ ("MRC     p15, 0, %0, c1, c0, 0" : "=r" (r));
    __asm__ ("ORR     %0, %1, %2" : "=r" (r) : "r" (r), "r" (MMU_ENABLE));
    __asm__ ("MCR     p15, 0, %0, c1, c0, 0" : : "r" (r));

    _ISB();
    _DSB();
}

void HalDisableMMU(void)
{
    register UINT32 r;
    #if defined(CC_MT5891)
    register UINT32 r1;
    #endif

    __asm__ ("MRC     p15, 0, %0, c1, c0, 0" : "=r" (r));
    __asm__ ("BIC     %0, %1, %2" : "=r" (r) : "r" (r), "r" (MMU_ENABLE));
    __asm__ ("MCR     p15, 0, %0, c1, c0, 0" : : "r" (r));
    #if defined(CC_MT5891)
	asm volatile(
	"	mrrc p15, 1, %0, %1, c15\n"
	"	bic %0, %0, %4\n"		    // ACTLR.SMP
	"	mcrr p15, 1, %0, %1, c15\n"
	  : "=&r" (r), "=&r" (r1)
	  : "r" (0), "r" (1), "Ir" (0x40)
	  : "cc");
	#else
    __asm__ ("MRC     p15, 0, %0, c1, c0, 1" : "=r" (r));
    __asm__ ("BIC     %0, %1, %2" : "=r" (r) : "r" (r), "r" (SMP));
    __asm__ ("MCR     p15, 0, %0, c1, c0, 1" : : "r" (r));
    #endif

    _ISB();
    _DSB();
}


//-----------------------------------------------------------------------------
/** HalInvalidateDCacheMultipleLine() Invalidate multiple line of D-cache
 *  @param u4Addr  [in] - The start address to be invalidated
 *  @param u4Size  [in] - The memory size to be invalidated
 */
//-----------------------------------------------------------------------------
void HalInvalidateDCacheMultipleLine(UINT32 u4Addr, UINT32 u4Size)
{
    UINT32 i;
    if (u4Size == 0)
    {
        return;
    }
    else
    {
        UINT32 start, end;
        start = u4Addr& ~DCACHE_LINE_SIZE_MSK;
        end = (u4Addr + u4Size + DCACHE_LINE_SIZE_MSK) & ~DCACHE_LINE_SIZE_MSK;
        u4Size = end - start;
        for (i=0; i<(u4Size/DCACHE_LINE_SIZE); i++)
        {
            HalInvalidateDCacheSingleLine(u4Addr);
            u4Addr += DCACHE_LINE_SIZE;
        }
    }
}

//-----------------------------------------------------------------------------
/** HalFlushDCacheMultipleLine() Flush (clean) multiple line of D-cache
 *  @param u4Addr  [in] - The start address to be flushed
 *  @param u4Size  [in] - The memory size to be invalidated
 */
//-----------------------------------------------------------------------------
void HalFlushDCacheMultipleLine(UINT32 u4Addr, UINT32 u4Size)
{
    UINT32 i;
    if (u4Size == 0)
    {
        return;
    }
    else if (u4Size > DCACHE_FLUSH_THRESHOLD)
    {
        HalFlushDCache();
    }
    else
    {
        UINT32 start, end;
        start = u4Addr& ~DCACHE_LINE_SIZE_MSK;
        end = (u4Addr + u4Size + DCACHE_LINE_SIZE_MSK) & ~DCACHE_LINE_SIZE_MSK;
        u4Size = end - start;
        for (i=0; i<(u4Size/DCACHE_LINE_SIZE); i++)
        {
            HalFlushDCacheSingleLine(u4Addr);
            u4Addr += DCACHE_LINE_SIZE;
        }
    }
}

//-----------------------------------------------------------------------------
/** HalFlushInvalidateDCacheMultipleLine() Flush (clean) and invalidate multiple \n
 *                  line of D-cache
 *  @param u4Addr  [in] - The start address to be flushed
 *  @param u4Size  [in] - The memory size to be invalidated
 */
//-----------------------------------------------------------------------------
void HalFlushInvalidateDCacheMultipleLine(UINT32 u4Addr, UINT32 u4Size)
{
    UINT32 i;
    if (u4Size == 0)
    {
        return;
    }
    else if (u4Size > DCACHE_FLUSH_THRESHOLD)
    {
    HalFlushInvalidateDCache();
    }
    else
    {
        UINT32 start, end;
        start = u4Addr& ~DCACHE_LINE_SIZE_MSK;
        end = (u4Addr + u4Size + DCACHE_LINE_SIZE_MSK) & ~DCACHE_LINE_SIZE_MSK;
        u4Size = end - start;
        for (i=0; i<(u4Size/DCACHE_LINE_SIZE); i++)
        {
            HalFlushInvalidateDCacheSingleLine(u4Addr);
            u4Addr += DCACHE_LINE_SIZE;
        }
    }
}
