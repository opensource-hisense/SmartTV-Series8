/*
 * linux/arch/arm/mach-mt53xx/headsmp.S
 *
 * SMP CPU core
 *
 *  Copyright (c) 2010-2012 Mediatek
 *	This file is based  ARM Realview platform
 *  Copyright (c) 2003 ARM Limited
 *  All Rights Reserved
 * $Author: yj.chiang $
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

#include <linux/linkage.h>
#include <linux/init.h>

	__CPUINIT

#ifdef CONFIG_SMP
/*
 * mt53xx specific entry point for secondary CPUs.  This
 * provides a "holding pen" into which all secondary cores are held
 * until we're ready for them to initialise.
 */
ENTRY(mt53xx_secondary_startup)
/* Because turstzone init is before this configuration, we move the configuration to gcc_startup.S
#ifdef CONFIG_ARM_ERRATA_828420
    mrc     p15, 0, r1, c0, c0, 0
    ubfx    r1, r1, #4, #12
    ldr     r2, =0x00000c0e
    teq     r1, r2
    bne     1f
    mrc     p15, 0, r1, c15, c0, 1
    orr     r1, r1, #1 << 30
    mcr     p15, 0, r1, c15, c0, 1
1:
#endif
*/
    bl  v7_invalidate_l1

	mrc	p15, 0, r0, c0, c0, 5
	ldr r1, =0xfff
	and	r0, r0, r1	/* r0 now cpuid*/
	adr	r4, 1f
	ldmia	r4, {r5, r6}	/* r4:1f r5 1f r6:pen_release addr */
	sub	r4, r4, r5	/* r4: relocate offset */
	add	r6, r6, r4	/* r6: relocated pen_release */
	dsb
pen:	ldr	r7, [r6]
	cmp	r7, r0
	bne	pen

	/*
	 * we've been released from the holding pen: secondary_stack
	 * should now contain the SVC stack for this core
	 */
	b	secondary_startup

1:	.long	.
	.long	pen_release
ENDPROC(mt53xx_secondary_startup)
#endif

	.section .text

ENTRY(mt53xx_secondary_sleep)
	mov     r10, #0
	mcr	p15, 0, r10, c7, c5, 0		@ I+BTB cache invalidate
	dsb
	mrc	p15, 0, r10, c1, c0, 0		@ ctrl register
	orr	r10, r10, #0x1000		@ Enable I cache.
	mcr	p15, 0, r10, c1, c0, 0		@ ctrl register
1:	dsb
	wfi
	b	1b
ENDPROC(mt53xx_secondary_sleep)

