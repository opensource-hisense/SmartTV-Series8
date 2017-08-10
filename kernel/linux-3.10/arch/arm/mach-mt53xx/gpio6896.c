/*
 * linux/arch/arm/mach-mt53xx/gpio6896.c
 *
 * Native GPIO driver.
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


#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/module.h>

#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/of_address.h>

#include <mach/mtk_gpio_internal.h>
#include <mach/mt53xx_linux.h>
#include <mach/platform.h>

#include <asm/io.h>

#define ASSERT(x)
#define VERIFY(x)
#define LOG(...)

#define u4IO32Read4B    __raw_readl
#define vIO32Write4B(addr,val)    __raw_writel(val,addr)

extern void __iomem *pPdwncIoMap;
extern void __iomem *pCkgenIoMap;

//-----------------------------------------------------------------------------
// Macro definitions
//-----------------------------------------------------------------------------
#if defined(CONFIG_ARCH_MT5398)
#include <mach/mt5398/x_gpio_hw.h>

#define GPIO_INT_INDEX_MAX ((GPIO_SUPPORT_INT_NUM + 31) >> 5)

static const INT32 _GPIO2BitOffsetOfIntReg[NORMAL_GPIO_NUM] =
{
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
    16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,

    32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
    48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,

	64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
	80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95,

    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,

    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,

    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1

};

static const INT32 _BitOffsetOfIntReg2GPIO[GPIO_SUPPORT_INT_NUM] =
{
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
    16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,

    32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
    48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,

    64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
	80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95
};
#elif defined(CONFIG_ARCH_MT5880)
#include <mach/mt5880/x_gpio_hw.h>

#define GPIO_INT_INDEX_MAX ((GPIO_SUPPORT_INT_NUM + 31) >> 5)

static const INT32 _GPIO2BitOffsetOfIntReg[NORMAL_GPIO_NUM] =
{
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
    16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,

    32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
    48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,

	64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
	80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, -1,

    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,

    -1, -1, -1, -1, -1, -1

};

static const INT32 _BitOffsetOfIntReg2GPIO[GPIO_SUPPORT_INT_NUM] =
{
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
    16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,

    32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
    48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,

	64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
	80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94
};

#elif defined(CONFIG_ARCH_MT5399)
#include <mach/mt5399/x_gpio_hw.h>

#define GPIO_INT_INDEX_MAX ((GPIO_SUPPORT_INT_NUM + 31) >> 5)

static const INT32 _GPIO2BitOffsetOfIntReg[NORMAL_GPIO_NUM] =
{
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
    16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,

    32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
    48, 49, -1, -1, -1, -1, -1, -1, 56, 57, 58, 59, 60, 61, 62, 63,

	64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
	80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95,

    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,

    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,

    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,

	-1, -1, -1, -1, -1, -1, -1,-1
};

static const INT32 _BitOffsetOfIntReg2GPIO[GPIO_SUPPORT_INT_NUM] =
{
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
    16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,

    32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
    48, 49, -1, -1, -1, -1, -1, -1, 56, 57, 58, 59, 60, 61, 62, 63,

    64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
	80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95
};

#elif defined(CONFIG_ARCH_MT5890)
#include <mach/mt5890/x_gpio_hw.h>

#define GPIO_INT_INDEX_MAX ((GPIO_SUPPORT_INT_NUM + 31) >> 5)

static const INT32 _GPIO2BitOffsetOfIntReg[NORMAL_GPIO_NUM] =
{
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
    16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,

    32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
    48, 49, -1, -1, -1, -1, -1, -1, 56, 57, 58, 59, 60, 61, 62, 63,

	64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
	80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95,

    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,

    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,

    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,

	-1, -1, -1, -1, -1, -1, -1,-1

};

static const INT32 _BitOffsetOfIntReg2GPIO[GPIO_SUPPORT_INT_NUM] =
{
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
    16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,

    32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
    48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,

    64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
	80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95
};

#elif defined(CONFIG_ARCH_MT5891)
#include <mach/mt5891/x_gpio_hw.h>

#define GPIO_INT_INDEX_MAX ((GPIO_SUPPORT_INT_NUM + 31) >> 5)

static const INT32 _GPIO2BitOffsetOfIntReg[NORMAL_GPIO_NUM] =
{
    0  , 1  , 2  , 3  , 4  , 5  , 6  , 7  , 8  , 9  ,  	//9
	10 , 11 , 12 , 13 , 14 , 15 , 16 , 17 , 18 , 19 ,  	//19
	20 , 21 , 22 , 23 , 24 , 25 , 26 , 27 , 28 , 29 ,  	//29
	30 , 31 , 32 , 33 , 34 , 35 , 36 , 37 , 38 , 39 ,  	//39
	40 , 41 , 42 , 43 , 44 , 45 , 46 , 47 , 48 , 49 ,  	//49
	50 , 51 , 52 , 53 , 54 , 55 , 56 , 57 , 58 , 59 ,  	//59 
	60 , 61 , 62 , 63 , 64 , 65 , 66 , 67 , 68 , 69 ,  	//69
	70 , 71 , 72 , 73 , 74 , 75 , 76 , 77 , 78 , 79 ,  	//79
    80 , 81 , 82 , 83 , 84 , 85 , 86 , 87 , 88 , 89 ,  	//89
    90 , 91 , 92 , 93 , 94 , 95 , 96 , 97 , 98 , 99 ,  	//99
    100, 101, 102, 103, 104, 105, 106, 107, 108, 109,   //109
    110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 	//119
    120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 	//129		
    130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 	//139
    140, 141, 142, 143, 144, 145, 146, -1 ,	-1 , -1 , 	//149
    -1 , -1 , -1 , -1 , -1 , -1 , -1 , -1 ,	-1 , -1 ,   //159
    -1 , -1 , -1 , -1 , -1 , -1 , -1 ,					//166
};

static const INT32 _BitOffsetOfIntReg2GPIO[GPIO_SUPPORT_INT_NUM] =
{
    0  , 1  , 2  , 3  , 4  , 5  , 6  , 7  , 8  , 9  , 	//9
	10 , 11 , 12 , 13 , 14 , 15 , 16 , 17 , 18 , 19 , 	//19
	20 , 21 , 22 , 23 , 24 , 25 , 26 , 27 , 28 , 29 , 	//29
	30 , 31 , 32 , 33 , 34 , 35 , 36 , 37 , 38 , 39 , 	//39
	40 , 41 , 42 , 43 , 44 , 45 , 46 , 47 , 48 , 49 , 	//49
	50 , 51 , 52 , 53 , 54 , 55 , 56 , 57 , 58 , 59 , 	//59
	60 , 61 , 62 , 63 , 64 , 65 , 66 , 67 , 68 , 69 , 	//69
	70 , 71 , 72 , 73 , 74 , 75 , 76 , 77 , 78 , 79 , 	//79
	80 , 81 , 82 , 83 , 84 , 85 , 86 , 87 , 88 , 89 , 	//89
	90 , 91 , 92 , 93 , 94 , 95 , 96 , 97 , 98 , 99 ,  	//99
	100, 101, 102, 103, 104, 105, 106, 107, 108, 109,   //109
    110, 111, 112, 113, 114, 115, 116, 117, 118, 119,	//119
    120, 121, 122, 123, 124, 125, 126, 127, 128, 129,	//129
    130, 131, 132, 133, 134, 135, 136, 137, 138, 139,	//139
    140, 141, 142, 143, 144, 145, 146,					//146
};

#elif defined(CONFIG_ARCH_MT5861)
#include <mach/mt5861/x_gpio_hw.h>

#define GPIO_INT_INDEX_MAX ((GPIO_SUPPORT_INT_NUM + 31) >> 5)

static const INT32 _GPIO2BitOffsetOfIntReg[NORMAL_GPIO_NUM] =
{
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
    16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,

    32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
    48, 49, -1, -1, -1, -1, -1, -1, 56, 57, 58, 59, 60, 61, 62, 63,

	64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
	80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95,

    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,

    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,

    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 174,175,
	176,177,178,179, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,

	-1, -1, -1, -1, -1, -1, -1,-1

};

static const INT32 _BitOffsetOfIntReg2GPIO[GPIO_SUPPORT_INT_NUM] =
{
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
    16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,

    32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
    48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,

    64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
	80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95
};

#elif defined(CONFIG_ARCH_MT5882)
#include <mach/mt5882/x_gpio_hw.h>

#define GPIO_INT_INDEX_MAX ((GPIO_SUPPORT_INT_NUM + 31) >> 5)

static const INT32 _GPIO2BitOffsetOfIntReg[NORMAL_GPIO_NUM] =
{
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
    16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,

    32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
    48, 49, -1, -1, -1, -1, -1, -1, 56, 57, 58, 59, 60, 61, 62, 63,

	64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
	80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95,

    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,

    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,

    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,

	-1, -1, -1, -1, -1, -1, -1,-1

};

static const INT32 _BitOffsetOfIntReg2GPIO[GPIO_SUPPORT_INT_NUM] =
{
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
    16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,

    32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
    48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,

    64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
	80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95
};

#elif defined(CONFIG_ARCH_MT5883)
#include <mach/mt5883/x_gpio_hw.h>

#define GPIO_INT_INDEX_MAX ((GPIO_SUPPORT_INT_NUM + 31) >> 5)

static const INT32 _GPIO2BitOffsetOfIntReg[NORMAL_GPIO_NUM] =
{
    0  , 1  , 2  , 3  , 4  , 5  , 6  , 7  , 8  , 9  ,  	//9
	10 , 11 , 12 , 13 , 14 , 15 , 16 , 17 , 18 , 19 ,  	//19
	20 , 21 , 22 , 23 , 24 , 25 , 26 , 27 , 28 , 29 ,  	//29
	30 , 31 , 32 , 33 , 34 , 35 , 36 , 37 , 38 , 39 ,  	//39
	40 , 41 , 42 , 43 , 44 , 45 , 46 , 47 , 48 , 49 ,  	//49
	50 , 51 , 52 , 53 , 54 , 55 , 56 , 57 , 58 , 59 ,  	//59 
	60 , 61 , 62 , 63 , 64 , 65 , 66 , 67 , 68 , 69 ,  	//69
	70 , 71 , 72 , 73 , 74 , 75 , 76 , 77 , 78 , 79 ,  	//79
    80 , 81 , 82 , 83 , 84 , 85 , 86 , 87 , 88 , 89 ,  	//89
    90 , 91 , 92 , 93 , 94 , 95 , 96 , 97 , 98 , 99 ,  	//99
    100, 101, 102, 103, 104, 105, 106, 107, 108, 109,   //109
    110, 111, 112, -1 , -1 , -1 , -1 , -1 , -1 , -1 , 	//119
    -1 , -1 , -1 , -1 , -1 , -1 , -1 , -1 ,	-1 , -1 , 	//129		
    -1 , -1 , -1 , -1 , -1 , -1 , -1 , -1 ,	-1 , -1 , 	//139
    -1 , -1 , -1 , -1 , -1 , -1 , -1 , -1 ,	-1 , -1 , 	//149
    -1 , -1 ,
};

static const INT32 _BitOffsetOfIntReg2GPIO[GPIO_SUPPORT_INT_NUM] =
{
    0  , 1  , 2  , 3  , 4  , 5  , 6  , 7  , 8  , 9  , 	//9
	10 , 11 , 12 , 13 , 14 , 15 , 16 , 17 , 18 , 19 , 	//19
	20 , 21 , 22 , 23 , 24 , 25 , 26 , 27 , 28 , 29 , 	//29
	30 , 31 , 32 , 33 , 34 , 35 , 36 , 37 , 38 , 39 , 	//39
	40 , 41 , 42 , 43 , 44 , 45 , 46 , 47 , 48 , 49 , 	//49
	50 , 51 , 52 , 53 , 54 , 55 , 56 , 57 , 58 , 59 , 	//59
	60 , 61 , 62 , 63 , 64 , 65 , 66 , 67 , 68 , 69 , 	//69
	70 , 71 , 72 , 73 , 74 , 75 , 76 , 77 , 78 , 79 , 	//79
	80 , 81 , 82 , 83 , 84 , 85 , 86 , 87 , 88 , 89 , 	//89
	90 , 91 , 92 , 93 , 94 , 95 , 96 , 97 , 98 , 99 ,  	//99
	100, 101, 102, 103, 104, 105, 106, 107, 108, 109,   //109
    110, 111, 112, 	 									//119
};


#endif /* CONFIG_ARCH_MT5398 */

//-----------------------------------------------------------------------------
// Prototypes
//-----------------------------------------------------------------------------
static INT32 _CKGEN_GPIO_Range_Check(INT32 i4GpioNum);
static INT32 _CKGEN_GPIO_Enable(INT32 i4GpioNum, INT32 *pfgSet);
static INT32 _CKGEN_GPIO_Output(INT32 i4GpioNum, INT32 *pfgSet);
static INT32 _CKGEN_GPIO_Input(INT32 i4GpioNum);
static INT32 _CKGEN_GPIO_Intrq(INT32 i4GpioNum, INT32 *pfgSet);
static INT32 _CKGEN_GPIO_Reg(INT32 i4Gpio, MTK_GPIO_IRQ_TYPE eType, mtk_gpio_callback_t pfnCallback, void *data);
static INT32 _CKGEN_GPIO_Init(void);


//-----------------------------------------------------------------------------
// Macros
//-----------------------------------------------------------------------------

#define GPIO_POLLFALL_REG(gpio) \
    (_au4GpioPollFall[GPIO_TO_INDEX(gpio)] & (1U << (gpio & GPIO_INDEX_MASK)))

#define GPIO_POLLRISE_REG(gpio) \
    (_au4GpioPollRise[GPIO_TO_INDEX(gpio)] & (1U << (gpio & GPIO_INDEX_MASK)))

#define GPIO_POLLFALL_SET(gpio) \
    (_au4GpioPollFall[GPIO_TO_INDEX(gpio)] |= (1U << (gpio & GPIO_INDEX_MASK)))

#define GPIO_POLLFALL_CLR(gpio) \
    (_au4GpioPollFall[GPIO_TO_INDEX(gpio)] &= ~(1U << (gpio & GPIO_INDEX_MASK)))

#define GPIO_POLLRISE_SET(gpio) \
    (_au4GpioPollRise[GPIO_TO_INDEX(gpio)] |= (1U << (gpio & GPIO_INDEX_MASK)))

#define GPIO_POLLRISE_CLR(gpio) \
    (_au4GpioPollRise[GPIO_TO_INDEX(gpio)] &= ~(1U << (gpio & GPIO_INDEX_MASK)))


#if !(defined(CONFIG_ARCH_MT5883) || defined(CONFIG_ARCH_MT5891))
#define GPIO_INTR_RISE_REG(gpio)    \
    (GPIO_RISE_REG(gpio) & (1U << (gpio & EDGE_INDEX_MASK)))
#define GPIO_INTR_REG(gpio)     \
    (GPIO_EXTINTEN_REG(gpio) & (1U << (gpio & GPIO_INDEX_MASK)))
#define GPIO_INTR_FALL_REG(gpio)    \
    (GPIO_FALL_REG(gpio) & (1U << (gpio & EDGE_INDEX_MASK)))
#define GPIO_INTR_CLR(gpio)     \
    vIO32Write4B(REG_GPIO_EXTINTEN + (GPIO_TO_INDEX(gpio) << 2) , GPIO_EXTINTEN_REG(gpio)  & (~(1U << (gpio & EDGE_INDEX_MASK))))
#define GPIO_INTR_FALL_SET(gpio)    \
    vIO32Write4B(REG_GPIO_ED2INTEN + (GPIO_TO_INDEX(gpio) << 2), GPIO_ED2INTEN_REG(gpio) & (~(1U << (gpio & EDGE_INDEX_MASK)))); \
    vIO32Write4B(REG_GPIO_LEVINTEN + (GPIO_TO_INDEX(gpio) << 2), GPIO_LEVINTEN_REG(gpio) & (~(1U << (gpio & EDGE_INDEX_MASK)))); \
    vIO32Write4B(REG_GPIO_ENTPOL + (GPIO_TO_INDEX(gpio) << 2), GPIO_ENTPOL_REG(gpio) & (~(1U << (gpio & EDGE_INDEX_MASK)))); \
    vIO32Write4B(REG_GPIO_EXTINTEN + (GPIO_TO_INDEX(gpio) << 2), GPIO_EXTINTEN_REG(gpio) | (1U << (gpio & EDGE_INDEX_MASK)));

#define GPIO_INTR_RISE_SET(gpio)    \
    vIO32Write4B(REG_GPIO_ED2INTEN + (GPIO_TO_INDEX(gpio) << 2), GPIO_ED2INTEN_REG(gpio) & (~(1U << (gpio & EDGE_INDEX_MASK)))); \
    vIO32Write4B(REG_GPIO_LEVINTEN + (GPIO_TO_INDEX(gpio) << 2), GPIO_LEVINTEN_REG(gpio) & (~(1U << (gpio & EDGE_INDEX_MASK)))); \
    vIO32Write4B(REG_GPIO_ENTPOL + (GPIO_TO_INDEX(gpio) << 2), GPIO_ENTPOL_REG(gpio) | (1U << (gpio & EDGE_INDEX_MASK)));    \
    vIO32Write4B(REG_GPIO_EXTINTEN + (GPIO_TO_INDEX(gpio) << 2), GPIO_EXTINTEN_REG(gpio) | (1U << (gpio & EDGE_INDEX_MASK)));

#define GPIO_INTR_BOTH_EDGE_SET(gpio)    \
    vIO32Write4B(REG_GPIO_ED2INTEN + (GPIO_TO_INDEX(gpio) << 2), GPIO_ED2INTEN_REG(gpio) | (1U << (gpio & EDGE_INDEX_MASK)));    \
    vIO32Write4B(REG_GPIO_LEVINTEN + (GPIO_TO_INDEX(gpio) << 2), GPIO_LEVINTEN_REG(gpio) | (1U << (gpio & EDGE_INDEX_MASK)));    \
    vIO32Write4B(REG_GPIO_ENTPOL + (GPIO_TO_INDEX(gpio) << 2), GPIO_ENTPOL_REG(gpio) | (1U << (gpio & EDGE_INDEX_MASK)));    \
    vIO32Write4B(REG_GPIO_EXTINTEN + (GPIO_TO_INDEX(gpio) << 2), GPIO_EXTINTEN_REG(gpio) | (1U << (gpio & EDGE_INDEX_MASK)));



#define GPIO_INTR_LEVEL_LOW_SET(gpio)    \
    vIO32Write4B(REG_GPIO_ED2INTEN + (GPIO_TO_INDEX(gpio) << 2), GPIO_ED2INTEN_REG(gpio) & (~(1U << (gpio & EDGE_INDEX_MASK)))); \
    vIO32Write4B(REG_GPIO_LEVINTEN + (GPIO_TO_INDEX(gpio) << 2), GPIO_LEVINTEN_REG(gpio) | (1U << (gpio & EDGE_INDEX_MASK))); \
    vIO32Write4B(REG_GPIO_ENTPOL + (GPIO_TO_INDEX(gpio) << 2), GPIO_ENTPOL_REG(gpio) & (~(1U << (gpio & EDGE_INDEX_MASK)))); \
    vIO32Write4B(REG_GPIO_EXTINTEN + (GPIO_TO_INDEX(gpio) << 2), GPIO_EXTINTEN_REG(gpio) | (1U << (gpio & EDGE_INDEX_MASK)));

#define GPIO_INTR_LEVEL_HIGH_SET(gpio)    \
    vIO32Write4B(REG_GPIO_ED2INTEN + (GPIO_TO_INDEX(gpio) << 2), GPIO_ED2INTEN_REG(gpio) & (~(1U << (gpio & EDGE_INDEX_MASK)))); \
    vIO32Write4B(REG_GPIO_LEVINTEN + (GPIO_TO_INDEX(gpio) << 2), GPIO_LEVINTEN_REG(gpio) | (1U << (gpio & EDGE_INDEX_MASK))); \
    vIO32Write4B(REG_GPIO_ENTPOL + (GPIO_TO_INDEX(gpio) << 2), GPIO_ENTPOL_REG(gpio) | (1U << (gpio & EDGE_INDEX_MASK)));    \
    vIO32Write4B(REG_GPIO_EXTINTEN + (GPIO_TO_INDEX(gpio) << 2), GPIO_EXTINTEN_REG(gpio) | (1U << (gpio & EDGE_INDEX_MASK)));
#endif

//-----------------------------------------------------------------------------
// Static variables
//-----------------------------------------------------------------------------

static UINT32 _u4GPIOInit;

static const GPIO_HANDLER_FUNC_TBL gGPIO_HANDLER_FuncTbl[] =
{
    // pfnIsOwner, pfnInit, pfnEnable, pfnInput, pfnOutput, pfnReg
    {
        _CKGEN_GPIO_Range_Check,
        _CKGEN_GPIO_Init,
        _CKGEN_GPIO_Enable,
        _CKGEN_GPIO_Input,
        _CKGEN_GPIO_Output,
        _CKGEN_GPIO_Intrq,
        _CKGEN_GPIO_Reg,
    },
    {
        PDWNC_Native_GpioRangeCheck,
        PDWNC_Native_InitGpio,
        PDWNC_Native_GpioEnable,
        PDWNC_Native_GpioInput,
        PDWNC_Native_GpioOutput,
		PDWNC_Native_GpioIntrq,
		PDWNC_Native_GpioReg,
    },
    {
        PDWNC_Native_ServoGpioRangeCheck,
        NULL,
        PDWNC_Native_GpioEnable,
        PDWNC_Native_GpioInput,
        PDWNC_Native_GpioOutput,
		NULL, //servoad pin no gpio int function
		NULL, //servoad pin no gpio int function
    }

};

static mtk_gpio_callback_t _afnGpioCallback[NORMAL_GPIO_NUM];//NORMAL_GPIO_NUM for SW gpio polling thread
static void* _au4CkgenGpioCallBackArg[NORMAL_GPIO_NUM];
static UINT32 _au4GpioPrevPoll[TOTAL_GPIO_IDX];
static UINT32 _au4GpioPollFall[TOTAL_GPIO_IDX];
static UINT32 _au4GpioPollRise[TOTAL_GPIO_IDX];


//-----------------------------------------------------------------------------
// CKGEN gpio funcs
//-----------------------------------------------------------------------------

static INT32 _CKGEN_GPIO_Range_Check(INT32 i4GpioNum)
{
    if ((i4GpioNum < 0) || (i4GpioNum >= NORMAL_GPIO_NUM))
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

#if defined(CONFIG_ARCH_MT5399)
// for Mustang only
static INT32 _CKGEN_GPIO_Remap_Mustang(INT32 i4GpioNum)
{
	//gpio 162~167 is replaced gpio 200~205, because gpio 200~205 is larger than 200, and we often make gpio 200 as pdwnc goio.
	if((i4GpioNum >= 162) && (i4GpioNum <= 167))
	{
		i4GpioNum += 38;
		return i4GpioNum;
	}
	else
	{
		return i4GpioNum;
	}
}
#endif

static INT32 _CKGEN_GPIO_Enable(INT32 i4GpioNum, INT32 *pfgSet)
{
    UINT32 u4Val;
    INT32 i4Idx;
    unsigned long rCrit;

#if defined(CONFIG_ARCH_MT5399)
	i4GpioNum = _CKGEN_GPIO_Remap_Mustang(i4GpioNum);
#endif
    i4Idx = ((UINT32)i4GpioNum >> 5);

    spin_lock_irqsave(&mt53xx_bim_lock, rCrit);
    u4Val = GPIO_EN_REG(i4Idx);
    if (pfgSet == NULL)
    {
        spin_unlock_irqrestore(&mt53xx_bim_lock, rCrit);
        return ((u4Val & (1U << (i4GpioNum & GPIO_INDEX_MASK))) ? 1 : 0);
    }

    u4Val = (*pfgSet) ?
            (u4Val | (1U << (i4GpioNum & GPIO_INDEX_MASK))) :
            (u4Val & ~(1U << (i4GpioNum & GPIO_INDEX_MASK)));

    GPIO_EN_WRITE(i4Idx, u4Val);
    spin_unlock_irqrestore(&mt53xx_bim_lock, rCrit);
    return (*pfgSet);
}

static INT32 _CKGEN_GPIO_Output(INT32 i4GpioNum, INT32 *pfgSet)
{
    UINT32 u4Val,u4Val1;
    INT32 i4Idx,i4Val;
    unsigned long rCrit;

#if defined(CONFIG_ARCH_MT5399)
	i4GpioNum = _CKGEN_GPIO_Remap_Mustang(i4GpioNum);
#endif
    i4Idx = ((UINT32)i4GpioNum >> 5);

    spin_lock_irqsave(&mt53xx_bim_lock, rCrit);

    if (pfgSet == NULL)	//NULL: for query gpio status, must be GPIO output , but not change pin level
    {
        spin_unlock_irqrestore(&mt53xx_bim_lock, rCrit);
		if(_CKGEN_GPIO_Enable(i4GpioNum, NULL) == 0)//old is input state, change to be output
		{
			//get pin level
			u4Val1 = GPIO_IN_REG(i4Idx);
			//get current out register setting
			u4Val = GPIO_OUT_REG(i4Idx);
			u4Val = (u4Val1 & (1U << (i4GpioNum & GPIO_INDEX_MASK))) ?
	            (u4Val | (1U << (i4GpioNum & GPIO_INDEX_MASK))) :
	            (u4Val & ~(1U << (i4GpioNum & GPIO_INDEX_MASK)));
			GPIO_OUT_WRITE(i4Idx, u4Val);
			// Set the output mode.
			i4Val = 1;
			_CKGEN_GPIO_Enable(i4GpioNum, &i4Val);
		}
		u4Val = GPIO_OUT_REG(i4Idx);
        return ((u4Val & (1U << (i4GpioNum & GPIO_INDEX_MASK))) ? 1 : 0);
    }

    u4Val = GPIO_OUT_REG(i4Idx);
    u4Val = (*pfgSet) ?
            (u4Val | (1U << (i4GpioNum & GPIO_INDEX_MASK))) :
            (u4Val & ~(1U << (i4GpioNum & GPIO_INDEX_MASK)));

    GPIO_OUT_WRITE(i4Idx, u4Val);
    spin_unlock_irqrestore(&mt53xx_bim_lock, rCrit);

    // Set the output mode.
    i4Val = 1;
    UNUSED(_CKGEN_GPIO_Enable(i4GpioNum, &i4Val));

    return (*pfgSet);
}


static INT32 _CKGEN_GPIO_Input(INT32 i4GpioNum)
{
    UINT32 u4Val;
    INT32 i4Idx;

#if defined(CONFIG_ARCH_MT5399)
		i4GpioNum = _CKGEN_GPIO_Remap_Mustang(i4GpioNum);
#endif

    // Read input value.
    i4Idx = ((UINT32)i4GpioNum >> 5);
    u4Val = GPIO_IN_REG(i4Idx);

    return ((u4Val & (1U << (i4GpioNum & GPIO_INDEX_MASK))) ? 1U : 0);

}

static INT32 _CKGEN_GPIO_Intrq(INT32 i4GpioNum, INT32 *pfgSet)
{
    UINT32 u4Val;
    i4GpioNum = _GPIO2BitOffsetOfIntReg[i4GpioNum];
    if(i4GpioNum == -1)
        return -1;
    u4Val = GPIO_EXTINTEN_REG(i4GpioNum);
    if (pfgSet == NULL)
    {
        return ((u4Val & (1U << (i4GpioNum & GPIO_INDEX_MASK))) ? 1 : 0);
    }

    u4Val = (*pfgSet) ?
            (u4Val | (1U << (i4GpioNum & GPIO_INDEX_MASK))) :
            (u4Val & ~(1U << (i4GpioNum & GPIO_INDEX_MASK)));
#if defined(CONFIG_ARCH_MT5883)	|| defined(CONFIG_ARCH_MT5891)	
	vIO32Write4B(_GPIO_EXTINTEN_REG(GPIO_TO_INDEX(i4GpioNum)), u4Val);
#else
    vIO32Write4B(REG_GPIO_EXTINTEN + (GPIO_TO_INDEX(i4GpioNum)<< 2), u4Val);
#endif

    return (*pfgSet);

}

static UINT32 _GPIO_POLL_IN_REG(INT32 i4Idx)
{//only for normal gpio
    return GPIO_IN_REG(i4Idx);
}

static irqreturn_t _GPIO_DedicatedIsr(int u2VctId, void *dev_id)
{
    INT32 i4Idx;
    UINT32 u4Status[GPIO_INT_INDEX_MAX] = {0};
    UINT32 u4Val,i;
    INT32 i4GpioNum;

#ifdef CC_FIQ_SUPPORT
	ASSERT(u2VctId == VECTOR_EXT_INT0);
#else
    ASSERT(u2VctId == VECTOR_EXT1);
#endif
    // get edge triggered interrupt status.
    for(i=0; i<GPIO_INT_INDEX_MAX; i++)
    {	
    	#if defined(CONFIG_ARCH_MT5883) || defined(CONFIG_ARCH_MT5891)	
		u4Status[i] = u4IO32Read4B(_GPIO_EXTINT_REG(i));
		#else
    	u4Status[i] = u4IO32Read4B(REG_GPIO_EXTINT + 4*i);
		#endif
    }

    // handle edge triggered interrupt.
    for (i4Idx = 0; i4Idx < GPIO_SUPPORT_INT_NUM; i4Idx++)
    {
		i = GPIO_TO_INDEX(i4Idx);
		VERIFY(i < GPIO_INT_INDEX_MAX);
        u4Val = u4Status[i] & (1U << (i4Idx & GPIO_INDEX_MASK));
        if(u4Val)
        {
            i4GpioNum = _BitOffsetOfIntReg2GPIO[i4Idx];
            if ((i4GpioNum < NORMAL_GPIO_NUM) && (_afnGpioCallback[i4GpioNum] != NULL))
            {
                _afnGpioCallback[i4GpioNum](i4GpioNum, _GPIO_POLL_IN_REG(GPIO_TO_INDEX(i4GpioNum)) & (1U << (i4GpioNum & GPIO_INDEX_MASK))? 1:0, _au4CkgenGpioCallBackArg[i4GpioNum]);
            }
        }
    }

    // clear edge triggered interrupt status.
    for(i=0; i<GPIO_INT_INDEX_MAX; i++)
    {
    	#if defined(CONFIG_ARCH_MT5883) || defined(CONFIG_ARCH_MT5891)	
		vIO32Write4B(_GPIO_EXTINT_REG(i), 0);
		#else
        vIO32Write4B((REG_GPIO_EXTINT + 4*i), 0);
		#endif
    }

    return IRQ_HANDLED;
}



static INT32 _CKGEN_GPIO_Init()
{
    INT32 i4Ret;
    INT32 i4Idx;

    for (i4Idx = 0; i4Idx < NORMAL_GPIO_NUM; i4Idx++)
    {
        _afnGpioCallback[i4Idx] = NULL;
        _au4CkgenGpioCallBackArg[i4Idx] = 0;
    }

    // disable all gpio interrupt
    for (i4Idx = 0; i4Idx < GPIO_INT_INDEX_MAX; i4Idx++)
    {
    	#if defined(CONFIG_ARCH_MT5883) || defined(CONFIG_ARCH_MT5891)	
		vIO32Write4B(_GPIO_EXTINTEN_REG(i4Idx), 0);
		#else
		vIO32Write4B(REG_GPIO_EXTINTEN + (i4Idx << 2), 0);
		#endif
    }

    for (i4Idx = 0; i4Idx < TOTAL_GPIO_IDX; i4Idx++)
    {
        _au4GpioPrevPoll[i4Idx] = 0;
        _au4GpioPollFall[i4Idx] = 0;
        _au4GpioPollRise[i4Idx] = 0;
    }

    // clear all edge triggered interrupt status 0.
    for (i4Idx = 0; i4Idx < GPIO_INT_INDEX_MAX; i4Idx++)
    {
    	#if defined(CONFIG_ARCH_MT5883) || defined(CONFIG_ARCH_MT5891)	
        vIO32Write4B(_GPIO_EXTINT_REG(i4Idx), 0);
		#else
		vIO32Write4B(REG_GPIO_EXTINT + (i4Idx << 2), 0);
		#endif
    }

#ifdef CC_FIQ_SUPPORT
	i4Ret = request_irq(VECTOR_EXT_INT0, _GPIO_DedicatedIsr, IRQF_SHARED, "GPIO", _GPIO_DedicatedIsr);
#else
	i4Ret = request_irq(VECTOR_EXT1, _GPIO_DedicatedIsr, IRQF_SHARED, "GPIO", _GPIO_DedicatedIsr);
#endif
    ASSERT(i4Ret == 0);
    UNUSED(i4Ret);

    return i4Ret;
}



static INT32 _CKGEN_GPIO_Reg(INT32 i4Gpio, MTK_GPIO_IRQ_TYPE eType, mtk_gpio_callback_t callback, void *data)
{
//remove warning for klocwork checking
     if ((i4Gpio < 0) || (i4Gpio >= NORMAL_GPIO_NUM))
    {
        return -1;
    }

    if((eType != MTK_GPIO_TYPE_NONE) && (callback == NULL))
    {
        return -1;
    }

    _au4GpioPrevPoll[GPIO_TO_INDEX(i4Gpio)] = _GPIO_POLL_IN_REG(GPIO_TO_INDEX(i4Gpio));

    /* Set the register and callback function. */
    LOG(7,"_CKGEN_GPIO_Reg gpio: %d\n", i4Gpio);
    switch(eType)
    {
    case MTK_GPIO_TYPE_INTR_FALL:
        if(_GPIO2BitOffsetOfIntReg[i4Gpio] == -1)
        {
            GPIO_POLLFALL_SET((UINT32)i4Gpio);
        }
        else
        {
            LOG(7,"_CKGEN_GPIO_Reg type: GPIO_TYPE_INTR_FALL\n");
            GPIO_INTR_FALL_SET((UINT32)_GPIO2BitOffsetOfIntReg[i4Gpio]);
        }
        break;
    case MTK_GPIO_TYPE_INTR_RISE:
        if(_GPIO2BitOffsetOfIntReg[i4Gpio] == -1)
        {
            GPIO_POLLRISE_SET((UINT32)i4Gpio);
        }
        else
        {
            LOG(7,"_CKGEN_GPIO_Reg type: GPIO_TYPE_INTR_RISE\n");
            GPIO_INTR_RISE_SET((UINT32)_GPIO2BitOffsetOfIntReg[i4Gpio]);
        }
        break;
    case MTK_GPIO_TYPE_INTR_BOTH:
        if(_GPIO2BitOffsetOfIntReg[i4Gpio] == -1)
        {
            GPIO_POLLRISE_SET((UINT32)i4Gpio);
            GPIO_POLLFALL_SET((UINT32)i4Gpio);
        }
        else
        {
            LOG(7,"_CKGEN_GPIO_Reg type: GPIO_TYPE_INTR_BOTH\n");
            GPIO_INTR_BOTH_EDGE_SET((UINT32)_GPIO2BitOffsetOfIntReg[i4Gpio]);
        }
        break;
    case MTK_GPIO_TYPE_INTR_LEVEL_LOW:
        if(_GPIO2BitOffsetOfIntReg[i4Gpio] != -1)
        {
            LOG(7,"_CKGEN_GPIO_Reg type: GPIO_TYPE_INTR_LEVEL_LOW\n");
            GPIO_INTR_LEVEL_LOW_SET((UINT32)_GPIO2BitOffsetOfIntReg[i4Gpio]);
        }
        break;
    case MTK_GPIO_TYPE_INTR_LEVEL_HIGH:
        if(_GPIO2BitOffsetOfIntReg[i4Gpio] != -1)
        {
            LOG(7,"_CKGEN_GPIO_Reg type: GPIO_TYPE_INTR_LEVEL_HIGH\n");
            GPIO_INTR_LEVEL_HIGH_SET((UINT32)_GPIO2BitOffsetOfIntReg[i4Gpio]);
        }
        break;
    case MTK_GPIO_TYPE_NONE:
        LOG(7,"_CKGEN_GPIO_Reg type: GPIO_TYPE_NONE\n");
        if(_GPIO2BitOffsetOfIntReg[i4Gpio] != -1)
        {
            GPIO_INTR_CLR((UINT32)_GPIO2BitOffsetOfIntReg[i4Gpio]);
        }
        _afnGpioCallback[i4Gpio] = NULL;
        return 0;
    default:
        return -1;
    }
    if (callback)
    {
        _afnGpioCallback[i4Gpio] = callback;
        _au4CkgenGpioCallBackArg[i4Gpio] = data;
    }
    return 0;
}



//-----------------------------------------------------------------------------
// gpio funcs
//-----------------------------------------------------------------------------
static INT32 _GPIO_OwnerNumOffset(UINT32 u4Val,INT32 i4GpioNum)
{
        switch(u4Val)
        {
            case GPIO_HANDLER_CKGEN:
                return i4GpioNum;
            case GPIO_HANDLER_PDWNC_GPIO:
                return i4GpioNum - OPCTRL(0);
            case GPIO_HANDLER_PDWNC_SRVAD:
                 return (i4GpioNum - SERVO_0_ALIAS) + (SERVO_GPIO0 - OPCTRL(0))  ;
        }
    return i4GpioNum;
}

int mtk_gpio_is_valid(int number)
{
    UINT32 u4Val;

    for(u4Val = 0; u4Val < (sizeof(gGPIO_HANDLER_FuncTbl)/sizeof(GPIO_HANDLER_FUNC_TBL)); u4Val++)
    {
        if(gGPIO_HANDLER_FuncTbl[u4Val].pfnIsOwner(number))
            return 1;
    }

    return 0;
}
EXPORT_SYMBOL(mtk_gpio_is_valid);

int mtk_gpio_direction_input(unsigned gpio)
{
    UINT32 u4Val, set = 0;

    for(u4Val = 0; u4Val < (sizeof(gGPIO_HANDLER_FuncTbl)/sizeof(GPIO_HANDLER_FUNC_TBL)); u4Val++)
    {
        if(gGPIO_HANDLER_FuncTbl[u4Val].pfnIsOwner(gpio))
        {
            BUG_ON(gGPIO_HANDLER_FuncTbl[u4Val].pfnEnable == NULL);
			return	gGPIO_HANDLER_FuncTbl[u4Val].pfnEnable(_GPIO_OwnerNumOffset(u4Val, gpio),  &set);
        }
    }

    return -EINVAL;
}
EXPORT_SYMBOL(mtk_gpio_direction_input);

int mtk_gpio_direction_output(unsigned gpio, int init_value)
{
    UINT32 u4Val, ret, set = 1;

    for(u4Val = 0; u4Val < (sizeof(gGPIO_HANDLER_FuncTbl)/sizeof(GPIO_HANDLER_FUNC_TBL)); u4Val++)
    {
        if(gGPIO_HANDLER_FuncTbl[u4Val].pfnIsOwner(gpio))
        {
            BUG_ON(gGPIO_HANDLER_FuncTbl[u4Val].pfnEnable == NULL);
			ret = gGPIO_HANDLER_FuncTbl[u4Val].pfnEnable(_GPIO_OwnerNumOffset(u4Val, gpio),  &set);
            if (!ret)
                return ret;

			gGPIO_HANDLER_FuncTbl[u4Val].pfnOutput(_GPIO_OwnerNumOffset(u4Val, gpio), &init_value);
            return 0;
        }
    }

    return -EINVAL;
}
EXPORT_SYMBOL(mtk_gpio_direction_output);

int mtk_gpio_get_value(unsigned gpio)
{
    UINT32 u4Val;

    for(u4Val = 0; u4Val < (sizeof(gGPIO_HANDLER_FuncTbl)/sizeof(GPIO_HANDLER_FUNC_TBL)); u4Val++)
    {
        if(gGPIO_HANDLER_FuncTbl[u4Val].pfnIsOwner(gpio))
        {
            BUG_ON(gGPIO_HANDLER_FuncTbl[u4Val].pfnInput == NULL);
			return	gGPIO_HANDLER_FuncTbl[u4Val].pfnInput(_GPIO_OwnerNumOffset(u4Val, gpio));
        }
    }

    return 0;
}
EXPORT_SYMBOL(mtk_gpio_get_value);

void mtk_gpio_set_value(unsigned gpio, int value)
{
    UINT32 u4Val;

    for(u4Val = 0; u4Val < (sizeof(gGPIO_HANDLER_FuncTbl)/sizeof(GPIO_HANDLER_FUNC_TBL)); u4Val++)
    {
        if(gGPIO_HANDLER_FuncTbl[u4Val].pfnIsOwner(gpio))
        {
            BUG_ON(gGPIO_HANDLER_FuncTbl[u4Val].pfnOutput == NULL);
			gGPIO_HANDLER_FuncTbl[u4Val].pfnOutput(_GPIO_OwnerNumOffset(u4Val, gpio), &value);
            return;
        }
    }

    return;
}
EXPORT_SYMBOL(mtk_gpio_set_value);

int mtk_gpio_request_irq(unsigned gpio, MTK_GPIO_IRQ_TYPE eType, mtk_gpio_callback_t callback, void *data)
{
    UINT32 u4Val;

    for(u4Val = 0; u4Val < (sizeof(gGPIO_HANDLER_FuncTbl)/sizeof(GPIO_HANDLER_FUNC_TBL)); u4Val++)
    {
        if(gGPIO_HANDLER_FuncTbl[u4Val].pfnIsOwner(gpio))
        {
            BUG_ON(gGPIO_HANDLER_FuncTbl[u4Val].pfnReg == NULL);
			return	gGPIO_HANDLER_FuncTbl[u4Val].pfnReg(_GPIO_OwnerNumOffset(u4Val, gpio),eType, callback, data);
        }
    }

    return -EINVAL;
}
EXPORT_SYMBOL(mtk_gpio_request_irq);

void mtk_gpio_set_irq(unsigned gpio, int enable)
{
    UINT32 u4Val;

    for(u4Val = 0; u4Val < (sizeof(gGPIO_HANDLER_FuncTbl)/sizeof(GPIO_HANDLER_FUNC_TBL)); u4Val++)
    {
        if(gGPIO_HANDLER_FuncTbl[u4Val].pfnIsOwner(gpio))
        {
            BUG_ON(gGPIO_HANDLER_FuncTbl[u4Val].pfnIntrq == NULL);
			gGPIO_HANDLER_FuncTbl[u4Val].pfnIntrq(_GPIO_OwnerNumOffset(u4Val, gpio), &enable);
            return;
        }
    }
}
EXPORT_SYMBOL(mtk_gpio_set_irq);

int __init mtk_gpio_init(void)
{
    UINT32 u4Val;
#ifdef CONFIG_OF
	struct device_node *np_pdwnc;
	struct device_node *np_ckgen;
#endif
    if (_u4GPIOInit)
    {
        return 0;
    }
    _u4GPIOInit = 1;
/****************************************************************/
#ifdef CONFIG_OF
	np_pdwnc = of_find_compatible_node(NULL, NULL, "Mediatek, PDWNC");
	np_ckgen = of_find_compatible_node(NULL, NULL, "Mediatek, CKGEN");
	if (!np_pdwnc || !np_ckgen) {
		panic("%s: rtc node not found\n", __func__);
	}
	pPdwncIoMap = of_iomap(np_pdwnc, 0);
	pCkgenIoMap = of_iomap(np_ckgen, 0);
#else
	pPdwncIoMap = ioremap(PDWNC_PHY,0x1000);
	pCkgenIoMap = ioremap(CKGEN_PHY,0x1000);
#endif
	if (!pPdwncIoMap || !pCkgenIoMap)
		panic("Impossible to ioremap GPIO\n");
	printk("GPIO_reg_base: 0x%p , 0x%p .\n", pPdwncIoMap,pCkgenIoMap);
/*****************************************************************/
    for(u4Val = 0; u4Val < (sizeof(gGPIO_HANDLER_FuncTbl)/sizeof(GPIO_HANDLER_FUNC_TBL)); u4Val++)
    {
        if(gGPIO_HANDLER_FuncTbl[u4Val].pfnInit != NULL)
            gGPIO_HANDLER_FuncTbl[u4Val].pfnInit();
    }

    return 0;
}
arch_initcall(mtk_gpio_init);
