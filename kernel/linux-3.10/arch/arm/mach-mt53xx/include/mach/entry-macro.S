/*
 * linux/arch/arm/mach-mt53xx/include/mach/entry-macro.S
 *
 * Low-level IRQ helper macros for MT5391 platforms
 *
 * Copyright (c) 2010-2012 MediaTek Inc.
 * $Author:  $
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

#include <mach/hardware.h>
#include <mach/irqs.h>

#if defined(CONFIG_ARM_GIC)

#include <asm/hardware/gic.h>

	/* Uses the GIC interrupt controller built into the cpu */
#define ICTRL_BASE (IO_CPU_VIRT + 0x100)

	.macro	disable_fiq
	.endm

	.macro	get_irqnr_preamble, base, tmp
	movw \base, #(ICTRL_BASE & 0x0000ffff)
	movt \base, #((ICTRL_BASE & 0xffff0000) >> 16)
	.endm

	.macro  arch_ret_to_user, tmp1, tmp2
	.endm

	/*
	 * The interrupt numbering scheme is defined in the
	 * interrupt controller spec.  To wit:
	 *
	 * Interrupts 0-15 are IPI
	 * 16-28 are reserved
	 * 29-31 are local.  We allow 30 to be used for the watchdog.
	 * 32-1020 are global
	 * 1021-1022 are reserved
	 * 1023 is "spurious" (no interrupt)
	 *
	 * For now, we ignore all local interrupts so only return an interrupt
	 * if it's between 30 and 1020.  The test_for_ipi routine below will
	 * pick up on IPIs.
	 *
	 * A simple read from the controller will tell us the number of the
	 * highest priority enabled interrupt.  We then just need to check
	 * whether it is in the valid range for an IRQ (30-1020 inclusive).
	 */

	.macro  get_irqnr_and_base, irqnr, irqstat, base, tmp

	/* bits 12-10 = src CPU, 9-0 = int # */
	ldr     \irqstat, [\base, #GIC_CPU_INTACK]

	ldr		\tmp, =1021

	bic     \irqnr, \irqstat, #0x1c00

	cmp     \irqnr, #29
	cmpcc	\irqnr, \irqnr
	cmpne	\irqnr, \tmp
	cmpcs	\irqnr, \irqnr

	.endm

	/* We assume that irqstat (the raw value of the IRQ acknowledge
	 * register) is preserved from the macro above.
	 * If there is an IPI, we immediately signal end of interrupt on the
	 * controller, since this requires the original irqstat value which
	 * we won't easily be able to recreate later.
	 */

	.macro test_for_ipi, irqnr, irqstat, base, tmp
	bic	\irqnr, \irqstat, #0x1c00
	cmp	\irqnr, #16
	strcc	\irqstat, [\base, #GIC_CPU_EOI]
	cmpcs	\irqnr, \irqnr
	.endm

	/* As above, this assumes that irqstat and base are preserved.. */

	.macro test_for_ltirq, irqnr, irqstat, base, tmp
	bic	\irqnr, \irqstat, #0x1c00
	mov 	\tmp, #0
	cmp	\irqnr, #29
	moveq	\tmp, #1
	streq	\irqstat, [\base, #GIC_CPU_EOI]
	cmp	\tmp, #0
	.endm

#else
/* leave disable_fiq as an empty macro */
    .macro disable_fiq
    .endm
    .macro  get_irqnr_preamble, base, tmp
    .endm

    .macro  arch_ret_to_user, tmp1, tmp2
    .endm

/* 
 * returns highest prio irq in irqnr,
 * all other regs are used as tmp regs
 */
    .macro get_irqnr_and_base, irqnr, irqstat, base, tmp

    /* set bim base register */
    ldr \base, =BIM_VIRT
    /* load irq enable bit mask */
    ldr \tmp, [\base, #REG_RW_MINTEN]
    /* load current irq bits */
    ldr \irqstat, [\base, #REG_RW_MINTST]
    /* ignore disabled irq bits */
    and \irqstat, \irqstat, \tmp
    /* check whether DRAMC interrupt happens */
    mov \irqnr, #32
    and \tmp, \irqstat, #1
    teq \tmp, #0
    bne 100f

    /* load irq enable bit mask */
    ldr \tmp, [\base, #REG_IRQEN]
    /* load current irq bits */
    ldr \irqstat, [\base, #REG_IRQST]
    /* ignore disabled irq bits */
    and \irqstat, \irqstat, \tmp
    /* check whether vdoin interrupt happens */
    mov \irqnr, #20
    and \tmp, \irqstat, #0x00100000
    teq \tmp, #0
    bne 100f

    /* ignore misc interrupt bit(31) */
    and \irqstat, \irqstat, #0x7FFFFFFF
    /* get irqnr (highest irq number) */
    clz \irqnr, \irqstat
    rsb \irqnr, \irqnr, #31
    /* test if no irq */
    teq \irqstat, #0
	bne 100f

    /* load irq enable bit mask */
    ldr \tmp, [\base, #REG_RW_MINTEN]
    /* load current irq bits */
    ldr \irqstat, [\base, #REG_RW_MINTST]
    /* ignore disabled irq bits */
    and \irqstat, \irqstat, \tmp

    /* get irqnr (highest irq number) */
    clz \irqnr, \irqstat
    rsb \irqnr, \irqnr, #63

100:	
    /* test if no irq */
    teq \irqstat, #0
    /* if irqstat == 0, means no irq currently */
    /* else irqnr contains the highest irq number */
    .endm
#endif
