/*-----------------------------------------------------------------------------
/*
 *lib_arm\hal.c
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
 *-----------------------------------------------------------------------------
 *
 * $Author: dtvbm11 $
 * $Date: 2015/12/22 $
 * $RCSfile: hal.c,v $
 * $Revision: #1 $
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

// Translate VA (Virtual Address) to MVA (Modified Virtual Address)
#define MVA(addr)                   ((addr) & 0xffffffe0)
#define ICACHE_LINE_SIZE            32
#define DCACHE_LINE_SIZE            32
#define ALLOC_BY_KMALLOC            0x1234
#define ALLOC_BY_GETPAGE            0x8765

// Cache parameters (in bytes)
#define DCACHE_LINE_SIZE_MSK        (DCACHE_LINE_SIZE - 1)
#define DCACHE_FLUSH_THRESHOLD      (DCACHE_LINE_SIZE * 16)

// PSR bit definitions
#define PSR_IRQ_DISABLE             (1 << 7)
#define PSR_FIQ_DISABLE             (1 << 6)

#if defined(CC_MT5365) || defined(CC_MT5395)
    #define FLUSH_FIFO IO_WRITE32MSK(BIM_BASE, REG_WFIFO, FLUSH_CMD, FLUSH_CMD)
#else
    #define FLUSH_FIFO
#endif

//-----------------------------------------------------------------------------
// Public functions
//-----------------------------------------------------------------------------

void HalFlushWriteBuffer(void)
{
    register UINT32 r = 0;

    __asm__ ("MCR     p15, 0, %0, c7, c10, 4" : : "r" (r));
    FLUSH_FIFO;
}

void HalFlushInvalidateDCacheSingleLine(UINT32 u4Addr)
{
    u4Addr = MVA(u4Addr);

    __asm__ ("MCR   p15, 0, %0, c7, c14, 1" : : "r" (u4Addr));
    FLUSH_FIFO;
}

void _FlushInvalidateDCache(void)
{
    register UINT32 reg1, reg2, reg3;

    //__asm__ ("MOV     %0, #0" : "=r" (reg1));
    reg1 = 0;
    __asm__ ("MRS     %0, CPSR" : "=r" (reg2));
    __asm__ ("MOV     %0, %1" : "=r" (reg3) : "r" (reg2));
    __asm__ ("ORR     %0, %1, %2" : "=r" (reg2) : "r" (reg2), "r" (PSR_IRQ_DISABLE));
    __asm__ ("ORR     %0, %1, %2" : "=r" (reg2) : "r" (reg2), "r" (PSR_FIQ_DISABLE));
    __asm__ ("MSR     CPSR_c, %0" : : "r" (reg2));
    __asm__ ("MCR     p15, 0, %0, c7, c14, 0" : : "r" (reg1));
    __asm__ ("MCR     p15, 0, %0, c7, c10, 4" : : "r" (reg1));
    __asm__ ("MSR     CPSR_c, %0" : : "r" (reg3));
    FLUSH_FIFO;
}

void HalFlushInvalidateDCache(void)
{
    _FlushInvalidateDCache();
    HalFlushWriteBuffer();
}

void HalInvalidateDCache(void)
{
    register UINT32 r = 0;

    __asm__ ("MCR     p15, 0, %0, c7, c6, 0" : : "r" (r));
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
    FLUSH_FIFO;
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


void HalInvalidateAllCaches(void)
{
    register UINT32 r = 0;

    __asm__ ("MCR p15, 0, %0, c7, c7, 0" : : "r" (r));
}


void _FlushDCache(void)
{
    register UINT32 reg1, reg2, reg3;

    //__asm__ ("MOV     %0, #0" : "=r" (reg1));
    reg1 = 0;
    __asm__ ("MRS     %0, CPSR" : "=r" (reg2));
    __asm__ ("MOV     %0, %1" : "=r" (reg3) : "r" (reg2));
    __asm__ ("ORR     %0, %1, %2" : "=r" (reg2) : "r" (reg2), "r" (PSR_IRQ_DISABLE));
    __asm__ ("ORR     %0, %1, %2" : "=r" (reg2) : "r" (reg2), "r" (PSR_FIQ_DISABLE));
    __asm__ ("MSR     CPSR_c, %0" : : "r" (reg2));
    __asm__ ("MCR     p15, 0, %0, c7, c10, 0" : : "r" (reg1));
    __asm__ ("MCR     p15, 0, %0, c7, c10, 4" : : "r" (reg1));
    __asm__ ("MSR     CPSR_c, %0" : : "r" (reg3));
    FLUSH_FIFO;
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
    __asm__ ("BIC     %0, %1, %2" : "=g" (r) : "r" (r), "r" (ICACHE_ENABLE));
    __asm__ ("BIC     %0, %1, %2" : "=g" (r) : "r" (r), "r" (DCACHE_ENABLE));
    __asm__ ("MCR     p15, 0, %0, c1, c0, 0" : : "r" (r));
}

void HalDisableMMU(void)
{
    register UINT32 r;

    __asm__ ("MRC     p15, 0, %0, c1, c0, 0" : "=g" (r));
    __asm__ ("BIC     %0, %1, %2" : "=g" (r) : "r" (r), "r" (MMU_ENABLE));
    __asm__ ("MCR     p15, 0, %0, c1, c0, 0" : : "r" (r));
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
