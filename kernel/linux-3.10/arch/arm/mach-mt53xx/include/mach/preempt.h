/*
 * linux/arch/arm/mach-mt53xx/include/mach/preempt.h
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

#ifndef _ASM_ARCH_PREEMT_H
#define _ASM_ARCH_PREEMT_H

static inline unsigned long clock_diff(unsigned long start, unsigned long stop)
{
	return (stop - start);
}

extern unsigned long mt5391_timer_read(int nr);

/*
 * timer 1 runs @ 100Mhz, 100 tick = 1 microsecond
 * and is configured as a count down timer.
 */

#define TICKS_PER_USEC                  100
#define ARCH_PREDEFINES_TICKS_PER_USEC
#define readclock()                     (~mt5391_timer_read(1))
#define clock_to_usecs(x)               ((x) / TICKS_PER_USEC)
#define INTERRUPTS_ENABLED(x)           (!(x & PSR_I_BIT))

#endif

