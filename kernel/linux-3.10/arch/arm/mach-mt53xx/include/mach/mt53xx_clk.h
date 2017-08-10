/*
 *  linux/arch/arm/mach-mt53xx/include/mach/mt53xx_clk.h
 *
 * Licensed under the GPL
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html#TOC1
 *
 * $Author: dtvbm11 $
 * $Date: 2015/12/22 $
 * $RCSfile:  $
 * $Revision: #1 $
 *
 * Internal header file for mach-mt53xx
 */

#ifndef __MT53XX_CLK_H__
#define __MT53XX_CLK_H__

u32 _CKGEN_GetXtalClock(void);


void mt53xx_clk_init(void);

#endif /* __MT53XX_CLK_H__ */
