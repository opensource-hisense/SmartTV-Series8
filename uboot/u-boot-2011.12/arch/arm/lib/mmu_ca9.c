/*
 * \arch\arm\lib\mmu_ca9.c
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
 */


/* ============================================================================
    Includes
============================================================================ */
#include "x_typedef.h"
#include "x_bim.h"
#include "x_dram.h"
#include <common.h>

/* ============================================================================
    Definitions
============================================================================ */


// Domain access control definitions of CP15 register r3
enum DOMAIN_ACCESS
{
    DENIED = 0,
    CLIENT = 1,
    RESERVED = 2,
    MANAGER = 3
};

enum ACCESS_PERMISSION //bit[11:10]
{
    NO_ACCESS,
    USER_NO_ACCESS,
    USER_READ_ONLY,
    USER_ACCESS,
};

enum PAGE_TABLE_ENTRY_TYPE
{
    INVALID = 0,
    COARSE_PAGE = 1,
    SECTION = 2,
    FINE_PAGE = 3,
};

#define TEX_STRONGLY_ORDERED    0 //C,B=0,0
#define TEX_SHARED_DEVICE       0 //C,B=0,1
#define TEX_NON_SHARED_DEVICE   2 //C,B=0,0
#define TEX_WRITE_ALLOCATE      1 //C,B=1,1

// Bit definitions of page table
#define C_BIT           (1 << 3)    // Cachable
#define B_BIT           (1 << 2)    // Bufferable
#define L1_XN_BIT       (1 << 4)    // The execute-never bit. Determines whether the region is executable
#define L2_XN_BIT       (1 << 0)    // for small page


//-----------------------------------------------------------------------------
// Macro definitions
//-----------------------------------------------------------------------------

// Macro definition for single level 1 entry of page table
#define L1Entry(type, addr, tex, ucb, acc) \
    ((type == SECTION) \
        ? (((addr) & 0xfff00000) | ((acc) << 10) | ((tex) << 12) | (ucb) | (type)) \
        : ((type == COARSE_PAGE) ? (((addr) &0xfffffc00) | (type)) : 0))

// for small page (0x2)
#define L2Entry(addr, tex, ucb, acc) \
    (((addr) & 0xfffff000) | ((acc) << 4) | ((tex) << 6) | (ucb) | 0x2)


// CP15 control register (c1) bit definitions
#define ICACHE_ENABLE               (1 << 12)
#define DCACHE_ENABLE               (1 << 2)
#define MMU_ENABLE                  (1 << 0)

extern void HalInvalidateDCache(void);

//-----------------------------------------------------------------------------
/** SetPageTableBase() Set base address of page table
 *  @param u4Address   [in] - The base address of page table
 */
//-----------------------------------------------------------------------------
static void SetPageTableBase(uint u4Address)
{
    asm volatile("mcr p15, 0, %0, c2, c0, 0" : : "r" (u4Address));
}

static int SetDomainAccess(uint u4Domain, uint u4Da)
{
    //lint --e{550} Symbol not accessed

    register uint u4Mask;
    register uint u4Access;
    register uint r;

    // Check if domain out of range
    if (u4Domain > 15)
    {
        return 0;
    }

    u4Mask = 3 << (u4Domain * 2);
    u4Access = u4Da << (u4Domain * 2);
/*
    __asm__ ("mrc     p15, 0, %0, c3, c0, 0" : "=r" (r));
    __asm__ ("bic     %0, %0, %1" : "=r" (r) : "r" (u4Mask));
    __asm__ ("orr     %0, %0, %1" : "=r" (r) : "r" (u4Access));
    __asm__ ("mcr     p15, 0, %0, c3, c0, 0" : "=r" (r));
*/

    asm volatile (
        "mrc     p15, 0, %0, c3, c0, 0\n"
        "bic     %0, %0, %1\n"
        "orr     %0, %0, %2\n"
        "mcr     p15, 0, %0, c3, c0, 0\n"
        : "=&r"  (r)
        : "r" (u4Mask), "r" (u4Access)
    );

    return 1;
}

void HalInvalidateTLB(void)
{
    register uint r = 0;

    asm volatile("mcr p15, 0, %0, c8, c7, 0" : : "r" (r));
}

/* ----------------------------------------------------------------------------
 * Description:
 *     Enable the MMU with ARMv6 extended page-table format.
 *
 * Arguments:
 *     None.
 *
 * Return:
 *     None.
 * ------------------------------------------------------------------------- */
void Boot_EnableMMU( void )
{
    register uint uiTmp;
    register uint uiBits = (1 << 0);

    HalInvalidateTLB();

    asm volatile (
        "mrc p15, 0, %0, c1, c0, 0\n"
        "orr %0, %0, %1\n"
        "mcr p15, 0, %0, c1, c0, 0\n"
        "dsb\n"
        "isb\n"
        : "=&r"  (uiTmp)
        : "r" (uiBits)

    );
}

void CPU_EnableDCache(void)
{
    register uint uwT0 = 0;

    HalInvalidateDCache();

    asm volatile (

        "mrc     p15, 0, %0, c1, c0, 0\n"
        "orr     %0, %0, #4\n"
        "mcr     p15, 0, %0, c1, c0, 0\n"
        :
        : "r"  (uwT0)
    );

}

void CPU_EnableICache(void)
{
    register uint uwT0 = 0;

    asm volatile (
        "mov     %0, #0 \n"
        "mcr     p15, 0, %0, c7, c5, 0\n"

        "mrc     p15, 0, %0, c1, c0, 0\n"
        "orr     %0, %0, #1 << 12\n"
        "mcr     p15, 0, %0, c1, c0, 0\n"
        :
        : "r"  (uwT0)
    );
}

int HalInitMMU(uint u4Addr)
{
    uint i;
    uint *pu4Table = (uint*)u4Addr;
    uint u4DramSize = TCMGET_CHANNELA_SIZE();
    if (IS_DRAM_CHANNELB_SUPPORT())
    {
        u4DramSize += TCMGET_CHANNELB_SIZE();
    }
    //UINT32 u4SramPageStart = SRAM_START / (1 * 1024 * 1024);

    // Check 16K Boundary
    if(u4Addr & 0x3fff)
    {
        return 0;
    }

    // Setup page table
    // Note
    // 1. Unused entries must be reserved.
    // 2. All accessible regions are set to domain 0

    // init all
    for (i = 0; i < 0x1000; i++)
    {
        pu4Table[i] = L1Entry(SECTION, i << 20, 0, L1_XN_BIT, NO_ACCESS);
    }

    // DRAM B 0-0x800M(1G), cachable/bufferable
    for (i = 0; i < u4DramSize; i++)
    {
        pu4Table[i] = L1Entry(SECTION, i << 20, TEX_WRITE_ALLOCATE, C_BIT | B_BIT, USER_ACCESS);
    }

    // DRAM A 0x800-0xc00M, non-cachable/non-bufferable
    for (i = 0x800; i < (0x800+u4DramSize); i++)
    {
        pu4Table[i] = L1Entry(SECTION, i << 20, 0, 0, USER_ACCESS);
    }

    // PBI B 32MB
    for (i = 0xe00; i < (0xe00+32); i++)
    {
        pu4Table[i] = L1Entry(SECTION, i << 20, 0, 0, USER_ACCESS);
    }

    // IO 0xf00-0xf01M, from strongly ordered to non-shared device in CA9
    for (i = 0xf00; i < 0xf01; i++)
    {
        pu4Table[i] = L1Entry(SECTION, i << 20, TEX_NON_SHARED_DEVICE, 0, USER_ACCESS);
    }

    // coresight
    for (i = 0xf10; i < 0xf12; i++)
    {
        pu4Table[i] = L1Entry(SECTION, i << 20, TEX_NON_SHARED_DEVICE, 0, USER_ACCESS);
    }

    // L2C register
    for (i = 0xf20; i < 0xf21; i++)
    {
        pu4Table[i] = L1Entry(SECTION, i << 20, 0, 0, USER_ACCESS);
    }

    // Rom code
    for (i = 0xf40; i < 0xf41; i++)
    {
        pu4Table[i] = L1Entry(SECTION, i << 20, 0, 0, USER_ACCESS);
    }

    // PBI A 32MB
    for (i = 0xf80; i < (0xf80+32); i++)
    {
        pu4Table[i] = L1Entry(SECTION, i << 20, 0, 0, USER_ACCESS);
    }

    // SRAM, 0xfb000000 ~ 0xfb006800
    for (i = 0xfb0; i < 0xfb1; i++)
    {
        pu4Table[i] = L1Entry(SECTION, i << 20, 0, 0, USER_ACCESS);
    }

    SetPageTableBase(u4Addr);

    // Enable domain 0 only, set it with access permission enable.
    SetDomainAccess(0, CLIENT);

    // Enable MMU
    Boot_EnableMMU();

    /* Enable caches. */
    CPU_EnableICache();
    CPU_EnableDCache();

    return 1;
}


