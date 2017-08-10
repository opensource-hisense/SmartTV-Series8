/*
 *  linux/arch/arm/mach-mt53xx/ckgen6896.c
 *
 * Licensed under the GPL
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html#TOC1
 *
 * $Author: dtvbm11 $
 * $Date: 2015/12/22 $
 * $RCSfile:  $
 * $Revision: #1 $
 *
 * CKGEN for 68/96 gen IC
 */


#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/clk-provider.h>
#include <linux/clkdev.h>

#include <asm/delay.h>

#include <mach/x_hal_io.h>
#include <mach/platform.h>
#include "mach/mt53xx_clk.h"

#ifdef CONFIG_ARCH_MT5880
#include <mach/mt5880/x_ckgen_hw.h>
#elif defined(CONFIG_ARCH_MT5398)
#include <mach/mt5398/x_ckgen_hw.h>
#elif defined(CONFIG_ARCH_MT5399)
#include <mach/mt5399/x_ckgen_hw.h>
#elif defined(CONFIG_ARCH_MT5890)
#include <mach/mt5890/x_ckgen_hw.h>
#elif defined(CONFIG_ARCH_MT5891)
#include <mach/mt5891/x_ckgen_hw.h>
#elif defined(CONFIG_ARCH_MT5861)
#include <mach/mt5861/x_ckgen_hw.h>
#elif defined(CONFIG_ARCH_MT5882)
#include <mach/mt5882/x_ckgen_hw.h>
#elif defined(CONFIG_ARCH_MT5883)
#include <mach/mt5883/x_ckgen_hw.h>
#else
#error ARCH_MT53xx CKGEN HW not defined.
#endif

#define GET_XTAL_CLK  _CKGEN_GetXtalClock


static UINT32 _CalGetPLLCounter(UINT32 u4CalSel)
{
    UINT32 u4Val, u4Mode, u4RefCount;

    // Select PLL source at REG_PLL_CALIB
    vIO32WriteFldAlign(CKGEN_PLLCALIB, CAL_SRC_SAWLESSPLL, FLD_DBGCKSEL);

    // Select mode, 0: 2048
    vIO32WriteFldAlign(CKGEN_PLLCALIB, 0, FLD_CAL_MODE);

    // Set CAL_SEL, 0 is clk/1, 1 is clk/2, 2 is clk/4
    vIO32WriteFldAlign(CKGEN_PLLCALIB, 0, FLD_CAL_SEL);

    // Select PLL source at REG_PLL_CALIB
    vIO32WriteFldAlign(CKGEN_PLLCALIB, u4CalSel, FLD_DBGCKSEL);

    // Set PLL_CAL_TRIGGER = 1
    vIO32WriteFldAlign(CKGEN_PLLCALIB, 1, FLD_CAL_TRI);

    // Wait calibration finishing.
    while (IO32ReadFldAlign(CKGEN_PLLCALIB, FLD_CAL_TRI)) { }

    // Calculate clock.
    udelay(10);
    u4Val = IO32ReadFldAlign(CKGEN_PLLCALIBCNT, FLD_CALI_CNT);

    u4Mode = IO32ReadFldAlign(CKGEN_PLLCALIB, FLD_CAL_MODE);

    switch(u4Mode) {
    case 0: u4RefCount = 11; break;
    case 1: u4RefCount = 16; break;
    case 2: u4RefCount = 20; break;
    case 3: u4RefCount = 24; break;
    default: u4RefCount = 11; break;
    }

    return (((GET_XTAL_CLK()/1000) * u4Val ) >> u4RefCount);
}

static unsigned long mt5880_get_cpupll(void)
{
    unsigned long u4Clock;

    switch (IO32ReadFldAlign(CKGEN_CPU_CKCFG, FLD_CPU_CK_SEL))
    {
        case  0: u4Clock = GET_XTAL_CLK(); break;
        case  1: u4Clock = _CalGetPLLCounter(CAL_SRC_CPUPLL_D2) * 2 * 1000; break;
        default: u4Clock = 0; break;
    }

    return u4Clock;
}

void __init mt53xx_clk_init(void)
{
    // Try get ARM clk, if it not zero, create
    unsigned long cpupll = mt5880_get_cpupll();
    struct clk *clk;
    struct clk_lookup *cl;

    if (cpupll)
    {
        clk = clk_register_fixed_rate(NULL, "smp_twd", NULL, CLK_IS_ROOT, cpupll/2);
        cl = clkdev_alloc(clk, NULL, "smp_twd", NULL);
        clkdev_add(cl);
    }
}

