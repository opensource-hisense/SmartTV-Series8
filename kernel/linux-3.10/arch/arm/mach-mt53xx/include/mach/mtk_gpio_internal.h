/*
 * linux/arch/arm/mach-mt53xx/include/mach/mtk_gpio_internal.h
 *
 * Internal interface for MTK GPIO API 
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

#ifndef __MTK_GPIO_INTERNAL_H__
#define __MTK_GPIO_INTERNAL_H__

#include <mach/mtk_gpio.h>
#include <x_typedef.h>

#define OPCTRL(x)               (OPCTRL0 + (x))
#define OPCTRL0                 (200)       ///< define gpio macro for OPCTRL0
#define SERVO_0_ALIAS 400
#define OPCTRL_INTEN(x) (0x1L << (x))

typedef enum {
    GPIO_HANDLER_CKGEN = 0,
    GPIO_HANDLER_PDWNC_GPIO,
    GPIO_HANDLER_PDWNC_SRVAD
} GPIO_HANDLER;

typedef struct {
    INT32 (*pfnIsOwner)(INT32);                 /// Check if specified number is owned by this GPIO handler
    INT32 (*pfnInit)(void);			/// Called when GPIO sys init
    INT32 (*pfnEnable)(INT32, INT32*);          /// Called to set GPIO direction.
    INT32 (*pfnInput)(INT32);                   /// Called to quyewhen input
    INT32 (*pfnOutput)(INT32, INT32*);
    INT32 (*pfnIntrq)(INT32, INT32*);           /// GPIO irq enable/disable
    INT32 (*pfnReg)(INT32, MTK_GPIO_IRQ_TYPE, mtk_gpio_callback_t, void *); /// To register IRQ.
} GPIO_HANDLER_FUNC_TBL;

typedef void (* PFN_GPIO_CALLBACK)(INT32 i4Gpio, BOOL fgStatus);

typedef enum {
    GPIO_TYPE_NONE = 0,
    GPIO_TYPE_INTR_RISE = 1,        
    GPIO_TYPE_INTR_FALL = 2,
    GPIO_TYPE_INTR_BOTH = 3,
    GPIO_TYPE_INTR_LEVEL_HIGH = 4,    
    GPIO_TYPE_INTR_LEVEL_LOW = 8,
    GPIO_TYPE_LAST = 12
} GPIO_TYPE;

//pdwnc function
EXTERN INT32 PDWNC_Native_GpioRangeCheck(INT32 i4Gpio);
EXTERN INT32 PDWNC_Native_InitGpio(void);
EXTERN INT32 PDWNC_Native_GpioEnable(INT32 i4Gpio, INT32 *pfgSet);
EXTERN INT32 PDWNC_Native_GpioInput(INT32 i4Gpio);
EXTERN INT32 PDWNC_Native_GpioOutput(INT32 i4Gpio, INT32 *pfgSet);
EXTERN INT32 PDWNC_Native_GpioIntrq(INT32 i4Gpio, INT32 *pfgSet);
EXTERN INT32 PDWNC_Native_GpioReg(INT32 i4Gpio, MTK_GPIO_IRQ_TYPE eType, mtk_gpio_callback_t callback, void *data);
EXTERN INT32 PDWNC_Native_ServoGpioRangeCheck(INT32 i4Gpio);


#endif /* __MTK_GPIO_INTERNAL_H__ */

