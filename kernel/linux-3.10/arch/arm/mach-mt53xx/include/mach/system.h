/*
 * linux/arch/arm/mach-mt53xx/include/mach/system.h
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

#ifndef __ASM_ARCH_SYSTEM_H
#define __ASM_ARCH_SYSTEM_H

#include <mach/platform.h>
#include <asm/io.h>

#if 0
static inline void arch_idle(void)
{
	/*
	 * switch off the system and wait for interrupt
	 */
	cpu_do_idle();
}

static inline void arch_reset(char mode, const char *cmd)
{
	/*
	 * use powerdown watch dog to reset system
	 * __raw_writel( 0xff000000, BIM_BASE + WATCHDOG_TIMER_COUNT_REG );
	 * __raw_writel( 1, BIM_BASE + WATCHDOG_TIMER_CTRL_REG);
	 */
	 __raw_writel( 0xff000000, PDWNC_VIRT + REG_RW_WATCHDOG_TMR );
	 __raw_writel( 1, PDWNC_VIRT + REG_RW_WATCHDOG_EN);
}
#endif

int mt53xx_imprecise_abort_report(void);

#define arch_imprecise_abort_report()    mt53xx_imprecise_abort_report()

#define arch_adjust_zones(size, hole) \
	mt53xx_adjust_zones(size, hole)

#endif

int __init mt53xx_l2_cache_resume(void);
extern void *mt53xx_saved_core1;
