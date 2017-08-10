/*
 * linux/mtd/mtk_mtdpart.h
 *
 *  This is the MTD driver for MTK NAND controller
 *
 *
 * Copyright (c) 2010-2012 MediaTek Inc.
 *	This file is based  ARM Realview platform
 *  Copyright (C) 2002 ARM Ltd.
 *  All Rights Reserved
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

#ifndef __MT53XX_PART_OPER_H__
#define __MT53XX_PART_OPER_H__

#include <linux/types.h>

int i4NFBPartitionFastboot(struct mtd_info *mtd);
int i4NFBPartitionRead(uint32_t u4DevId, uint32_t u4Offset, uint32_t u4MemPtr, uint32_t u4MemLen);
int i4NFBPartitionWrite(uint32_t u4DevId, uint32_t u4Offset, uint32_t u4MemPtr, uint32_t u4MemLen);

#endif /* !__MT53XX_PART_OPER_H__ */
