/*
* arch/arm/lib/mmu.c
*
* MMU init
*
* Copyright (c) 2008-2012 ARM Limited.
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
*
*/

/* ============================================================================
    Includes
============================================================================ */

#include <common.h>
/* ============================================================================
    Definitions
============================================================================ */


// Domain access control definitions of CP15 register r3
enum DOMAIN_ACCESS
{
    NO_ACCESS = 0,
    CLIENT = 1,
    RESERVED = 2,
    MANAGER = 3
};

enum PAGE_TABLE_ENTRY_TYPE
{
    INVALID = 0,
    COARSE_PAGE = 1,
    SECTION = 2,
    FINE_PAGE = 3
};

// Bit definitions of page table
#define C_BIT           (1 << 3)    // Cachable
#define B_BIT           (1 << 2)    // Bufferable
#define U_BIT           (1 << 4)    // Must be 1, for backward compatibility


//-----------------------------------------------------------------------------
// Macro definitions
//-----------------------------------------------------------------------------

// Macro definition for single level 1 entry of page table
#define L1Entry(type, addr, dom, ucb, acc) \
    ((type == SECTION) \
        ? (((addr) & 0xfff00000) | ((acc) << 10) | ((dom) << 5) | (ucb) | \
            U_BIT | (type)) \
        : ((type == COARSE_PAGE) \
            ? (((addr) &0xfffffc00) | ((dom) << 5) | U_BIT | (type)) \
            : 0))

// CP15 control register (c1) bit definitions
#define DATA_UNALIGN_ENABLE         (1<< 22)
#define ICACHE_ENABLE               (1 << 12)
#define DCACHE_ENABLE               (1 << 2)
#define MMU_ENABLE                  (1 << 0)

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
    register uint uiBits = (1 << 22) | (1 << 0);

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
    register uint uwT0;

    asm volatile (
        "mov     %0, #0 \n"
        "mcr     p15, 0, %0, c7, c6, 0\n"

        "mrc     p15, 0, %0, c1, c0, 0\n"
        "orr     %0, %0, #4\n"
        "mcr     p15, 0, %0, c1, c0, 0\n"
        :
        : "r"  (uwT0)
    );

}

void CPU_EnableICache(void)
{
    register uint uwT0;

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
    uint u4DramSize = 0x10000000; //128MB
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

    // DRAM B 0-0x400M(1G), cachable/bufferable
    for (i = 0; i < 0x200; i++)
    {
        if ((i << 20) < u4DramSize)
        {
            pu4Table[i] = L1Entry(SECTION, i << 20, 0, C_BIT | B_BIT, MANAGER);
        }
        else
        {
            pu4Table[i] = L1Entry(SECTION, i << 20, 0, 0, NO_ACCESS);
        }
    }

    // Unused region
    for (i = 0x200; i < 0xf00; i++)
    {
        pu4Table[i] = L1Entry(SECTION, i << 20, 0, 0, NO_ACCESS);
    }

    // IO 0x200-0x201M, non-cachable/non-bufferable, 0x200xxxxx => I/O register address.
    for (i = 0xf00; i < 0xf01; i++)
    {
        pu4Table[i] = L1Entry(SECTION, i << 20, 0, 0, MANAGER);
    }

    // Unused region
    for (i = 0xf01; i < 0x1000; i++)
    {
        pu4Table[i] = L1Entry(SECTION, i << 20, 0, 0, NO_ACCESS);
    }

    // Rom code
    for (i = 0xf40; i < 0xf41; i++)
    {
        pu4Table[i] = L1Entry(SECTION, i << 20, 0, 0, MANAGER);
    }

    // NOR flash
    for (i = 0xf80; i < (0xf80+32); i++)
    {
        pu4Table[i] = L1Entry(SECTION, i << 20, 0, 0, MANAGER);
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


