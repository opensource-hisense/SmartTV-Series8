/*
 * linux/arch/arm/mach-mt53xx/include/mach/mtk_gpio.h
 *
 * Interface for MTK GPIO API
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

#ifndef __MTK_GPIO_H__
#define __MTK_GPIO_H__

typedef enum {
    MTK_GPIO_TYPE_NONE = 0,
    MTK_GPIO_TYPE_INTR_RISE = 1,        
    MTK_GPIO_TYPE_INTR_FALL = 2,
    MTK_GPIO_TYPE_INTR_BOTH = 3,
    MTK_GPIO_TYPE_INTR_LEVEL_HIGH = 4,
    MTK_GPIO_TYPE_INTR_LEVEL_LOW = 8,

    MTK_GPIO_TYPE_LAST = 12
}MTK_GPIO_IRQ_TYPE;

typedef void (*mtk_gpio_callback_t)(int gpio, int status, void *data);


/** Check if the gpio is valid.
 *   
 *  @retval boolean. 0 if not valid, valid otherwise.
 */
int mtk_gpio_is_valid(int number);

/** Set GPIO direction as input
 *  Mapping to GPIO_Input()
 */
int mtk_gpio_direction_input(unsigned gpio);

/** Set GPIO direction as output
 *  Mapping to GPIO_Output()
 *
 * @param init_value  Use this as init_value for output. Add this is to make
 *                    sure the output signal won't have glitch.
 */
int mtk_gpio_direction_output(unsigned gpio, int init_value);

/** Read GPIO input value.
 *  The gpio direction must be set by calling mtk_gpio_direction_input.
 *  Please note some of the GPIO(ex, PDWNC servo) support input voltage 
 *  detection, therefore return range instead of boolean.
 *
 *  @retval GPIO input value.
 */
int mtk_gpio_get_value(unsigned gpio);

/** Set GPIO output value.
 *  The gpio direction must be set by calling mtk_gpio_direction_output.
 */
void mtk_gpio_set_value(unsigned gpio, int value);

/** Request GPIO interrupt.
 *  Mapping to GPIO_Reg()
 *
 * @retval 0 if success, <0 failed.
 */
int mtk_gpio_request_irq(unsigned gpio, MTK_GPIO_IRQ_TYPE eType, mtk_gpio_callback_t callback, void *data);

/** Enable GPIO interrupt.
 *
 * @param enable   0 to disable, non-zero enable.
 */
void mtk_gpio_set_irq(unsigned gpio, int enable);


int mtk_pdwnc_gpio_isr_func(unsigned u2IntIndex);

//-----------------------------------------------------------------------------
/** PDWNC_ReadServoADCChannelValue() Read the ServoADC Channel Value
 *  @param u4Channel the channel number
 *  @retval the channel adc value.
 */
//-----------------------------------------------------------------------------
unsigned int  mtk_servoadc_readchannelvalue(unsigned int u4Channel);

#endif /* __MTK_GPIO_H__ */
