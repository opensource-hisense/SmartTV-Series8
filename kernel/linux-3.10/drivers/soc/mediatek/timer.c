/*
 * linux/arch/arm/mach-mt53xx/timer.c
 *
 * system timer init.
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

/* system header files */
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>

#include <asm/irq.h>
#include <asm/io.h>
#include <asm/setup.h>
#include <asm/pgtable.h>
#include <asm/page.h>
#include <asm/uaccess.h>


#include <linux/clocksource.h>
#include <linux/clockchips.h>

#include <linux/soc/mediatek/platform.h>

/* Valid TIMER_ID: 0 */
#define SYS_TIMER_ID        0

#define SYS_TOFST           (SYS_TIMER_ID * 8)

#define SYS_TIMER_IRQ       (IRQ_TIMER0 +  SYS_TIMER_ID)
#define SYS_TIMER_REG_L     (REG_RW_TIMER0_LOW + SYS_TOFST)
#define SYS_TIMER_REG_H     (REG_RW_TIMER0_HIGH + SYS_TOFST)
#define SYS_TIMER_LIMIT_REG_L (REG_RW_TIMER0_LLMT + SYS_TOFST)
#define SYS_TIMER_LIMIT_REG_H (REG_RW_TIMER0_HLMT + SYS_TOFST)
#define SYS_TIMER_EN        (TCTL_T0EN << SYS_TOFST)
#define SYS_TIMER_AL        (TCTL_T0AL << SYS_TOFST)

//MCU register
#define XGPT_IDX                    (0x60)
#define XGPT_CTRL                   (0x64)

// XTAL config
#define CLK_24MHZ               24000000    // 24 MHz
#define CLK_27MHZ               27000000    // 27 MHz
#define CLK_48MHZ               48000000    // 48 MHz
#define CLK_54MHZ               54000000    // 54 MHz
#define CLK_60MHZ               60000000    // 60 MHz

#define IS_XTAL_54MHZ()         0
#define IS_XTAL_60MHZ()         0
#define IS_XTAL_27MHZ()         0
#define IS_XTAL_24MHZ()         1

#define GET_XTAL_CLK()          (IS_XTAL_54MHZ() ? (CLK_54MHZ) :    \
                                (IS_XTAL_60MHZ() ? (CLK_60MHZ) :    \
                                (IS_XTAL_24MHZ() ? (CLK_24MHZ) :    \
                                (CLK_27MHZ) )))


u32 _CKGEN_GetXtalClock(void)
{
#ifdef CONFIG_MT53_FPGA
    return CLK_27MHZ;
#else
    return GET_XTAL_CLK();
#endif /* CONFIG_FPGA */
}

static int __init mt53xx_timer_init(void)
{
    u32 regval32;
    u32 _u4SysClock;

    /* disable system clock */
    regval32 = __bim_readl(REG_RW_TIMER_CTRL);
    regval32 &= ~(SYS_TIMER_EN | SYS_TIMER_AL);
    __bim_writel(regval32, REG_RW_TIMER_CTRL);

    /* setup timer interval */
    _u4SysClock = _CKGEN_GetXtalClock();
    __bim_writel(_u4SysClock / HZ, SYS_TIMER_LIMIT_REG_L);
    __bim_writel(0, SYS_TIMER_LIMIT_REG_H);

    /* reset timer */
    __bim_writel(_u4SysClock / HZ, SYS_TIMER_REG_L);
    __bim_writel(0, SYS_TIMER_REG_H);

    /* enable timer with auto-load */
    regval32 |= (SYS_TIMER_EN | SYS_TIMER_AL);
    __bim_writel(regval32, REG_RW_TIMER_CTRL);

    return 0;
}
arch_initcall(mt53xx_timer_init);

#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5891)
u64 arch_timer_read_pcounter(void);
u64 mt53xx_read_local_timer(void)
{
#if CONFIG_ARM_ARCH_TIMER
    return arch_timer_read_counter();
#else
    return 0;
#endif
}

void mt53xx_write_local_timer(u64 cnt)
{
    u32 val;

    __raw_writel(0xc, p_mcusys_iomap + XGPT_IDX);
    val = (u32)(cnt >> 32U);
    __raw_writel(val, p_mcusys_iomap + XGPT_CTRL);
    __raw_writel(0x8, p_mcusys_iomap + XGPT_IDX);
    val = (u32)(cnt & 0xffffffff);
    __raw_writel(val, p_mcusys_iomap + XGPT_CTRL);
}
EXPORT_SYMBOL(mt53xx_read_local_timer);
EXPORT_SYMBOL(mt53xx_write_local_timer);
#endif //  defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5891)

unsigned long mt53xx_timer_read(int nr)
{
    return __bim_readl((REG_RW_TIMER0_LOW + (nr * 8)));
}
EXPORT_SYMBOL(mt53xx_timer_read);

unsigned long long mt53xx_timer_read_ll(int nr)
{
    unsigned long long utime;
    unsigned long time_lo1;
    unsigned long time_hi1, time_hi2;

    do
    {
        time_hi1 = __bim_readl(REG_RW_TIMER0_HIGH + (nr * 8));
        time_lo1 = __bim_readl(REG_RW_TIMER0_LOW + (nr * 8));
        time_hi2 = __bim_readl(REG_RW_TIMER0_HIGH + (nr * 8));
    } while (time_hi1 != time_hi2);

    utime = ((unsigned long long)time_hi1 << 32) + (unsigned long long)time_lo1;

    return utime;
}
EXPORT_SYMBOL(mt53xx_timer_read_ll);
