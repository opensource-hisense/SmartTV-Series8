/*
 * linux/arch/arm/mach-mt53xx/sleep.c
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
#include <linux/slab.h>
#include <asm/cacheflush.h>


#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/wait.h>

#include <asm/io.h>
#include <asm/mach/irq.h>
#include <mach/hardware.h>
#ifdef CONFIG_CACHE_L2X0
#include <mach/system.h>
#endif
#include "pm.h"

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
void (*mt53xx_core_suspend)(void) = NULL;
void mt53xx_register_core_suspend(void (*func)(void))
{
    mt53xx_core_suspend = func;
}
EXPORT_SYMBOL(mt53xx_register_core_suspend);

unsigned int mt53xx_pm_suspend_cnt = 0;
EXPORT_SYMBOL(mt53xx_pm_suspend_cnt);
void mt5396_pm_SuspendEnter(void)
{
    mt53xx_pm_suspend_cnt++;
    printk("mt53xx_pm_SuspendEnter: %d\n", mt53xx_pm_suspend_cnt);
#ifdef CONFIG_CACHE_L2X0
    //outer_flush_all(); // move into mt53xx_core_suspend
    //outer_disable(); // TODO: system panic because imprecise abort
#endif
    if (mt53xx_core_suspend != NULL)
        mt53xx_core_suspend();
#ifdef CONFIG_CACHE_L2X0
    mt53xx_l2_cache_resume();
#endif
    //memcpy((void*)0xFB00BF00, mt53xx_saved_core1, 256); // TODO: send arm event
} 

int _Chip_pm_begin(void)
{
    printk("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");
    printk("_Chip_pm_begin @@@@@@@@@@@@@@@@@@@@@@\n");
    printk(" @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");    
    return 0;
}

int _Chip_pm_prepare(void)
{
    printk("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");
    printk("_Chip_pm_prepare @@@@@@@@@@@@@@@@@@@@@@\n");
    printk(" @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");
    return 0;    
}

int _Chip_pm_enter(suspend_state_t state)
{
    printk("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");
    printk("_Chip_pm_enter @@@@@@@@@@@@@@@@@@@@@@\n");
    printk(" @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");

    /* ensure the debug is initialised (if enabled) */
    switch (state) 
    {
        case PM_SUSPEND_ON:
            break;
        case PM_SUSPEND_STANDBY:
            break;
        case PM_SUSPEND_MEM:
            mt5396_pm_SuspendEnter();    		
            break;
        case PM_SUSPEND_MAX:
            break;
        default:
            break;
    }
    return 0;
}
void _Chip_pm_finish(void)
{
    printk("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");
    printk("_Chip_pm_finish @@@@@@@@@@@@@@@@@@@@@@\n");
    printk(" @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");
}
void _Chip_pm_end(void)
{
    printk("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");
    printk("_Chip_pm_end @@@@@@@@@@@@@@@@@@@@@@@@\n");
    printk(" @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");

}
void _Chip_PM_init(void)
{
}    
