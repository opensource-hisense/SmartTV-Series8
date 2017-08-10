/*
 * linux/arch/arm/mach-mt53xx/pm.h
 *
 * Debugging macro include header
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

#ifndef _MTKPM_H
#define _MTKPM_H


#include <linux/suspend.h>

int _Chip_pm_begin(void);
int _Chip_pm_prepare(void);
int _Chip_pm_enter(suspend_state_t state);
void _Chip_pm_end(void);
void _Chip_pm_finish(void);
void _Chip_PM_init(void);

#endif

