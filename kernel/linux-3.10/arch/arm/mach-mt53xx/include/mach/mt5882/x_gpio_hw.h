/*
 * linux/arch/arm/mach-mt53xx/include/mach/x_gpio_hw.h
 *
 * Cobra GPIO defines
 *
 * Copyright (c) 2006-2012 MediaTek Inc.
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

#ifndef __X_GPIO_HW_H__
#define __X_GPIO_HW_H__

#define TOTAL_NORMAL_GPIO_NUM   (NORMAL_GPIO_NUM)
#define NORMAL_GPIO_NUM         200
#define MAX_GPIO_NUM            (409)

#define TOTAL_OPCTRL_NUM        42
#define TOTAL_PDWNC_GPIO_INT_NUM 22
#define TOTAL_PDWNC_GPIO_NUM	TOTAL_OPCTRL_NUM

#define TOTAL_GPIO_NUM          (OPCTRL0 + TOTAL_OPCTRL_NUM)//(NORMAL_GPIO_NUM + 8 + TOTAL_OPCTRL_NUM)
#define TOTAL_GPIO_IDX          ((NORMAL_GPIO_NUM + 31) >> 5)
#define GPIO_INDEX_MASK         ((1 << 5) - 1)
#define GPIO_TO_INDEX(gpio)     (((UINT32)(gpio)) >> 5)
#define GPIO_SUPPORT_INT_NUM	96// GPIO0~GPIO95
#define GPIO_EDGE_INTR_NUM      GPIO_SUPPORT_INT_NUM
#define TOTAL_EDGE_IDX          ((GPIO_EDGE_INTR_NUM + 31) >> 5)
#define EDGE_INDEX_MASK         ((1 << 5) - 1)

#define ADC2GPIO_CH_ID_MAX	7///5
#define ADC2GPIO_CH_ID_MIN	1///2

#define SERVO_GPIO1             (228)
#define SERVO_GPIO0             (SERVO_GPIO1 - 1)//not real gpio, just for judge ather sevoad_gpio function

#define MAX_PDWNC_INT_ID			32	 // Maximum value of PDWNC interrupt id
//#define MAX_PDWNC_INT_ID_2			38	 // Maximum value of PDWNC interrupt id

#define _PINT(v)		(1U << (v))

#define PINMUX0_OFFSET          CKGEN_PMUX0
#define PINMUX1_OFFSET          CKGEN_PMUX1
#define PINMUX2_OFFSET          CKGEN_PMUX2
#define PINMUX3_OFFSET          CKGEN_PMUX3
#define PINMUX4_OFFSET          CKGEN_PMUX4
#define PINMUX_MISC_OFFSET      CKGEN_PMUX_MISC

#define GPIO_OUT0_OFFSET        CKGEN_GPIOOUT0
#define GPIO_EN0_OFFSET         CKGEN_GPIOEN0
#define GPIO_IN0_OFFSET         CKGEN_GPIOIN0
#define GPIO_OUT4_OFFSET        CKGEN_GPIOOUT4
#define GPIO_EN4_OFFSET         CKGEN_GPIOEN4
#define GPIO_IN4_OFFSET         CKGEN_GPIOIN4

#define REG_GPIO_EXTINTEN       CKGEN_EXTINTEN0
#define REG_GPIO_EXTINT         CKGEN_EXTINT0
#define REG_GPIO_ED2INTEN       CKGEN_ED2INTEN0
#define REG_GPIO_LEVINTEN       CKGEN_LEVINTEN0
#define REG_GPIO_ENTPOL         CKGEN_ENTPOL0

#define PINMUX0_WRITE(value)            vIO32Write4B(PINMUX0_OFFSET, (value))
#define PINMUX1_WRITE(value)            vIO32Write4B(PINMUX1_OFFSET, (value))
#define PINMUX2_WRITE(value)            vIO32Write4B(PINMUX2_OFFSET, (value))
#define PINMUX_MISC_WRITE(value)        vIO32Write4B(PINMUX_MISC_OFFSET, (value))
#define PINMUX_WRITE(idx, value)        vIO32Write4B((PINMUX0_OFFSET+(idx << 2)), (value))
#define PINMUX0_REG()                   u4IO32Read4B(PINMUX0_OFFSET)
#define PINMUX1_REG()                   u4IO32Read4B(PINMUX1_OFFSET)
#define PINMUX2_REG()                   u4IO32Read4B(PINMUX2_OFFSET)
#define PINMUX_MISC_REG()               u4IO32Read4B(PINMUX_MISC_OFFSET)
#define PINMUX_REG(idx)                 u4IO32Read4B((PINMUX0_OFFSET+(idx << 2)))


#define GPIO_IN_REG(idx)                u4IO32Read4B(GPIO_IN0_OFFSET+(4*(idx)))
#define GPIO_OUT_REG(idx)               u4IO32Read4B(GPIO_OUT0_OFFSET+(4*(idx)))
#define GPIO_OUT_WRITE(idx,val)         vIO32Write4B((GPIO_OUT0_OFFSET+(4*(idx))), (val))
#define GPIO_EN_REG(idx)                u4IO32Read4B(GPIO_EN0_OFFSET+(4*(idx)))
#define GPIO_EN_WRITE(idx,val)          vIO32Write4B((GPIO_EN0_OFFSET+(4*(idx))), (val))
#define GPIO_ED2INTEN_REG(num)          u4IO32Read4B(REG_GPIO_ED2INTEN + (GPIO_TO_INDEX(num) << 2))
#define GPIO_LEVINTEN_REG(num)          u4IO32Read4B(REG_GPIO_LEVINTEN + (GPIO_TO_INDEX(num) << 2))
#define GPIO_ENTPOL_REG(num)            u4IO32Read4B(REG_GPIO_ENTPOL + (GPIO_TO_INDEX(num) << 2))
#define GPIO_EXTINTEN_REG(num)          u4IO32Read4B(REG_GPIO_EXTINTEN + (GPIO_TO_INDEX(num) << 2))
#define GPIO_RISE_REG(num)			    (~ GPIO_ED2INTEN_REG(num)) & (~ GPIO_LEVINTEN_REG(num))	 & (GPIO_ENTPOL_REG(num)) &  (GPIO_EXTINTEN_REG(num))// 				(~u4IO32Read4B(REG_GPIO_ED2INTEN)) & (~u4IO32Read4B(REG_GPIO_LEVINTEN)) & u4IO32Read4B(REG_GPIO_ENTPOL) & u4IO32Read4B(REG_GPIO_EXTINTEN)
#define GPIO_FALL_REG(num)			    (~ GPIO_ED2INTEN_REG(num)) & (~ GPIO_LEVINTEN_REG(num))	 & (~ GPIO_ENTPOL_REG(num)) &  (GPIO_EXTINTEN_REG(num))						//	(~u4IO32Read4B(REG_GPIO_ED2INTEN)) & (~u4IO32Read4B(REG_GPIO_LEVINTEN)) & (~u4IO32Read4B(REG_GPIO_ENTPOL)) & u4IO32Read4B(REG_GPIO_EXTINTEN)

#define PDWNC_GPIO_IN_REG(idx)                u4IO32Read4B(PDWNC_GPIOIN0+(4*(idx)))


#define IO_CKGEN_BASE (IO_VIRT + 0xD000)

#define CKGEN_GPIOEN0 (IO_CKGEN_BASE + 0x720)
    #define FLD_GPIO_EN0 Fld(32,0,AC_FULLDW)//[31:0]

#define CKGEN_GPIOOUT0 (IO_CKGEN_BASE + 0x700)
    #define FLD_GPIO_OUT0 Fld(32,0,AC_FULLDW)//[31:0]

#define CKGEN_GPIOIN0 (IO_CKGEN_BASE + 0x740)
    #define FLD_GPIO_IN0 Fld(32,0,AC_FULLDW)//[31:0]

#define CKGEN_EXTINTEN0 (IO_CKGEN_BASE + 0x784)
    #define FLD_INTEN0 Fld(32,0,AC_FULLDW)//[31:0]

#define CKGEN_EXTINT0 (IO_CKGEN_BASE + 0x790)
    #define FLD_EXTINT0 Fld(32,0,AC_FULLDW)//[31:0]

#define CKGEN_ED2INTEN0 (IO_CKGEN_BASE + 0x760)
    #define FLD_ED2INTEN0 Fld(32,0,AC_FULLDW)//[31:0]

#define CKGEN_LEVINTEN0 (IO_CKGEN_BASE + 0x76C)
    #define FLD_LEVINTEN0 Fld(32,0,AC_FULLDW)//[31:0]

#define CKGEN_ENTPOL0 (IO_CKGEN_BASE + 0x778)
    #define FLD_INTPOL0 Fld(32,0,AC_FULLDW)//[31:0]


//PDWNC GPIO related
#define IO_PDWNC_BASE 	(IO_VIRT + 0x28000)
#define PDWNC_INTSTA 	(IO_PDWNC_BASE + 0x040)
#define PDWNC_ARM_INTEN (IO_PDWNC_BASE+0x044)
#define PDWNC_INTCLR 	(IO_PDWNC_BASE+0x048)
#define PDWNC_ARM_INTEN_2 (IO_PDWNC_BASE + 0x060)
#define PDWNC_INTCLR_2 	(IO_PDWNC_BASE + 0x064)
#define PDWNC_GPIOOUT0 	(IO_PDWNC_BASE + 0x074)
#define PDWNC_GPIOOUT1 	(IO_PDWNC_BASE + 0x078)
#define PDWNC_GPIOEN0 	(IO_PDWNC_BASE + 0x07C)
#define PDWNC_GPIOEN1 	(IO_PDWNC_BASE + 0x080)
#define PDWNC_GPIOIN0 	(IO_PDWNC_BASE + 0x084)
#define PDWNC_GPIOIN1 	(IO_PDWNC_BASE + 0x088)
#define PDWNC_EXINT2ED  (IO_PDWNC_BASE + 0x0A8)
#define PDWNC_EXINTLEV  (IO_PDWNC_BASE + 0x0AC)
#define PDWNC_EXINTPOL  (IO_PDWNC_BASE + 0x0B0)
#define PDWNC_SRVCFG1 	(IO_PDWNC_BASE + 0x304)
#define PDWNC_SRVCST 	(IO_PDWNC_BASE + 0x390)
#define PDWNC_ADOUT0 	(IO_PDWNC_BASE + 0x394)


#define PDWNC_ADIN0 (400)
#define PDWNC_TOTAL_SERVOADC_NUM    (10)  //Oryx ServoADC channel 0 ~ 9
#define PDWNC_SRVCH_EN_CH(x)      	(1 << (x))

#endif /* __X_GPIO_HW_H__ */
