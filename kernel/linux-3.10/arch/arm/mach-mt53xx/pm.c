/*
 * linux/arch/arm/mach-mt53xx/pm.c
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

#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/pm.h>
#include <linux/suspend.h>


#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/wait.h>

#include "pm.h"


/******************************************************************************
* Debug Message Settings
******************************************************************************/


/*****************************************************************************
 * REGISTER
 ****************************************************************************/


/*****************************************************************************
 * LOCAL CONST DEFINATION
 ****************************************************************************/


/******************************************************************************
 * Gloabal Variable Defination
 *****************************************************************************/

/******************************************************************************
 * Extern FUNCTION DEFINATIONS
 *****************************************************************************/


/******************************************************************************
 * FUNCTION DEFINATIONS
 *****************************************************************************/


static int mtk_pm_begin(suspend_state_t state)
{
	_Chip_pm_begin();
	return 0;
}



static int mtk_pm_prepare(void)
{
	_Chip_pm_prepare();
	return 0;    
}



static int mtk_pm_enter(suspend_state_t state)
{
	_Chip_pm_enter(state);
	return 0;
}


static void mtk_pm_finish(void)
{
	_Chip_pm_finish();    
}

static void mtk_pm_end(void)
{
	_Chip_pm_end();    
}



static int mtk_pm_state_valid(suspend_state_t pm_state)
{
#ifdef CONFIG_OPM
    return (pm_state == PM_SUSPEND_ON) || (pm_state == PM_SUSPEND_MEM);
#else
    /* remove all suspend support from MT5396 */
    return 0;
#endif
}

static struct platform_suspend_ops mtk_pm_ops = {
	.valid		= mtk_pm_state_valid,
	.begin		= mtk_pm_begin,
	.prepare	= mtk_pm_prepare,
	.enter		= mtk_pm_enter,
	.finish		= mtk_pm_finish,
	.end		= mtk_pm_end,
};

static int __init mtk_pm_init(void)
{
	printk("Power Management for MTK DTV.\n");

	/* Register and set suspend operation */
	suspend_set_ops(&mtk_pm_ops); 

	return 0;
}    

__initcall(mtk_pm_init);
