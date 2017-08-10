/*
 * linux/arch/arm/mach-mt53xx/sleep.c
 *
 * Debugging macro include header
 *
 * Copyright (c) 2010-2012 MediaTek Inc.
 * $Author: dtvbm11 $
 * * This program is free software; you can redistribute it and/or modify
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
#include <mach/x_hal_io.h>
#include "pm.h"


/******************************************************************************
* Debug Message Settings
******************************************************************************/

/* Debug message event */
#define DBG_PM_NONE			0x00000000	/* No event */
#define DBG_PM_PMIC			0x00000001	/* PMIC related event */
#define DBG_PM_GPIO			0x00000002	/* GPIO related event */
#define DBG_PM_MSGS			0x00000004	/* MSG related event */
#define DBG_PM_SUSP			0x00000008	/* Suspend related event */
#define DBG_PM_ENTER			0x00000010	/* Function Entry */
#define DBG_PM_ALL			0xffffffff

#define DBG_PM_MASK	   (DBG_PM_ALL)


#if 1
u32 PM_DBG_FLAG = 0xFFFFFFFF;

#define MSG(evt, fmt, args...) \
do {	\
	if (((DBG_PM_##evt) & DBG_PM_MASK) && PM_DBG_FLAG) { \
		printk(fmt, ##args); \
	} \
} while(0)

#define MSG_FUNC_ENTRY(f)	MSG(ENTER, "<PM FUNC>: %s\n", __FUNCTION__)
#else
#define MSG(evt, fmt, args...) do{}while(0)
#define MSG_FUNC_ENTRY(f)	   do{}while(0)
#endif


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
extern void mt5396_pm_RegDump(void);

/******************************************************************************
 * FUNCTION DEFINATIONS
 *****************************************************************************/
#if 0
static void setup_fake_source(void)
{
	// fake source #0 -> ice
	// fake source #1
	//disable_irq(VECTOR_RS232);
	//enable_irq(VECTOR_RS232);
	//__bim_writel((0x1<<17), REG_RW_IRQEN);
	//mt53xx_unmask_irq(VECTOR_RS232);
	// fake source #2
	//enable_irq(VECTOR_POST_PROC);
}
#endif

unsigned int (*MID_WarmBootRegionProtect)(unsigned int) = NULL;
void mt5396_register_mid_function(unsigned int (*func)(unsigned int))
{
    MID_WarmBootRegionProtect = func;
}
EXPORT_SYMBOL(mt5396_register_mid_function);

//#define CONFIG_DRAM_PM
#ifdef CONFIG_DRAM_PM
/*
 * Pointers used for sizing and copying suspend function data
 */
extern int lpc32xx_sys_suspend(void);
extern int lpc32xx_sys_suspend_sz;

#define TEMP_IRAM_AREA  IO_ADDRESS(LZHS_SRM_VIRT)
#endif
void mt5396_pm_SuspendEnter(void)
{
#ifdef CONFIG_DRAM_PM 
	u32 u4Val2, uCpuBusClk;
	int (*lpc32xx_suspend_ptr) (void);
	void *iram_swap_area;
#endif
	//u32 temp = 0;
	u32 irqmask, u4Val;
	u32 watchdog;

	irqmask = __bim_readl(REG_RW_IRQEN);
	printk("mt5396_pm_SuspendEnter WFI v7: 0x%x\n", irqmask);
	__bim_writel(0x0, REG_RW_IRQEN);
	watchdog = __raw_readl(PDWNC_VIRT + REG_RW_WATCHDOG_EN);
        // disable watchdog
	__raw_writel( watchdog&0xff000000, PDWNC_VIRT + REG_RW_WATCHDOG_EN);
	//enable_irq(VECTOR_PDWNC);

	//setup_fake_source();

	//mt5396_pm_RegDump();

#ifndef CONFIG_DRAM_PM
#if 1
	while(1)
	{
		u4Val = __raw_readl(PDWNC_VIRT + REG_RW_SHREG0);
		//printk("PDWNC_SHREG0=0x%x\n",u4Val);

		if(u4Val == PDWNC_CMD_ARM_WAKEUP_FROM_SUSPEND)  // wakeup
		{
			break; 
		}
		udelay(2*1000);
		udelay(2*1000);
		udelay(2*1000);
		udelay(2*1000);
		udelay(2*1000);
	}
#else
	__bim_writel(0x1, REG_RW_IRQEN);
	/* enter WFI mode */
	cpu_do_idle();
#endif
#else
	//__bim_writel(0x1, REG_RW_IRQEN);
#ifdef CONFIG_DRAM_PM


	/* Allocate some space for temporary IRAM storage */
	iram_swap_area = kmalloc(lpc32xx_sys_suspend_sz, GFP_KERNEL);
	if (!iram_swap_area) {
		printk(KERN_ERR
		       "PM Suspend: cannot allocate memory to save portion "
			"of SRAM\n");
		//return -ENOMEM;
	}

	/* Backup a small area of IRAM used for the suspend code */
	memcpy(iram_swap_area, (void *) TEMP_IRAM_AREA,
		lpc32xx_sys_suspend_sz);

	/*
	 * Copy code to suspend system into IRAM. The suspend code
	 * needs to run from IRAM as DRAM may no longer be available
	 * when the PLL is stopped.
	 */
	memcpy((void *) TEMP_IRAM_AREA, &lpc32xx_sys_suspend,
		lpc32xx_sys_suspend_sz);
	flush_icache_range((unsigned long)TEMP_IRAM_AREA,
		(unsigned long)(TEMP_IRAM_AREA) + lpc32xx_sys_suspend_sz);

	/* Transfer to suspend code in IRAM */
	lpc32xx_suspend_ptr = (void *) TEMP_IRAM_AREA;
	flush_cache_all();


	//force close digtal clock
	u4Val = __raw_readl(CKGEN_VIRT+0x1c8);//CKGEN_BLOCK_CKEN_CFG0
	u4Val2 = __raw_readl(CKGEN_VIRT+0x1cc);

	__raw_writel(0x01000238, CKGEN_VIRT+0x1c8);
	__raw_writel(0x00000020, CKGEN_VIRT+0x1cc);

	//show bus and cpu clk
	uCpuBusClk = __raw_readl(CKGEN_VIRT+0x208);
	__raw_writel(uCpuBusClk&0x870f&0x409, CKGEN_VIRT+0x208);
	
#if 1
	if (MID_WarmBootRegionProtect != NULL)
	{
	    MID_WarmBootRegionProtect(1);
	}
	(void) lpc32xx_suspend_ptr();
	if (MID_WarmBootRegionProtect != NULL)
	{
	    MID_WarmBootRegionProtect(0);
	}
#else
	while(1)
	{
	        u32 u4Val;
		u4Val = __raw_readl(PDWNC_VIRT + REG_RW_SHREG0);
		//printk("PDWNC_SHREG0=0x%x\n",u4Val);

		if(u4Val == PDWNC_CMD_ARM_WAKEUP_FROM_SUSPEND)  // wakeup
		{
			break; 
		}
		udelay(2*1000);
		udelay(2*1000);
		udelay(2*1000);
		udelay(2*1000);
		udelay(2*1000);
	}
#endif

	/* Restore original IRAM contents */
	memcpy((void *) TEMP_IRAM_AREA, iram_swap_area,
		lpc32xx_sys_suspend_sz);

	kfree(iram_swap_area);
	
	//clear	MID protection first
       //memset((void*)(DRAM_VIRT+0x400), 0, 0x70);
	
	//restore bus&cpu clk
	__raw_writel(uCpuBusClk, CKGEN_VIRT+0x208);

	//force open digtal clock
	__raw_writel(u4Val, CKGEN_VIRT+0x1c8);
	__raw_writel(u4Val2, CKGEN_VIRT+0x1cc);
	

#endif
	/* enter WFI mode */
	//cpu_do_idle();
#endif

	__raw_writel( watchdog, PDWNC_VIRT + REG_RW_WATCHDOG_EN);
	__bim_writel(irqmask, REG_RW_IRQEN);
} 

void mt5396_pm_RegDump(void)
{
} 

int _Chip_pm_begin(void)
{
	MSG_FUNC_ENTRY();	
	/* PMU Sleep mode related setting */
	printk("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");
	printk("_Chip_pm_begin @@@@@@@@@@@@@@@@@@@@@@\n");
	printk(" @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");    
	return 0;
}

int _Chip_pm_prepare(void)
{
	MSG_FUNC_ENTRY();
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
			MSG(SUSP,"_Chip_pm_enter PM_SUSPEND_ON\n\r");
			break;
		case PM_SUSPEND_STANDBY:
			MSG(SUSP,"_Chip_pm_enter PM_SUSPEND_STANDBY\n\r");        
			break;
		case PM_SUSPEND_MEM:
			MSG(SUSP,"_Chip_pm_enter PM_SUSPEND_MEM!!!!\n\r");
			mt5396_pm_SuspendEnter();    		
			break;
		case PM_SUSPEND_MAX:
			MSG(SUSP,"_Chip_pm_enter PM_SUSPEND_MAX\n\r");        
			MSG(SUSP,"Not support for PM_SUSPEND_MAX\n\r");
			break;
		default:
			MSG(SUSP,"_Chip_pm_enter Error state\n\r");
			break;
	}
	return 0;
}
void _Chip_pm_finish(void)
{
	printk("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");
	printk("_Chip_pm_finish @@@@@@@@@@@@@@@@@@@@@@\n");
	printk(" @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");
	//mt6573_pm_RegDump();
}
void _Chip_pm_end(void)
{
	MSG_FUNC_ENTRY();
	printk("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");
	printk("_Chip_pm_end @@@@@@@@@@@@@@@@@@@@@@@@\n");
	printk(" @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");

}
void _Chip_PM_init(void)
{

}    

