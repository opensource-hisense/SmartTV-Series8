/*
 * board/mt53xx_com/mt53xx_com_board.c
 *
 * MT53xx common board function
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

#include <common.h>
#include <exports.h>

#include "x_bim.h"
#include "x_assert.h"
#include "drvcust_if.h"

#if 0
INT32 BIM_CheckT8032Recovery(void)
{
#ifdef CC_ENABLE_REBOOT_REASON
    extern INT32 PDWNC_Read_T8032_XData(UINT32 u4XAddr, UINT32 u4Size, const void *u4RiscAddr);
    extern INT32 PDWNC_Write_T8032_XData(UINT32 u4XAddr, UINT32 u4Size, const void *u4RiscAddr);
    int ret;
    char *buf;
    UINT32 u4RebootOffset, u4RebootSize;

    if (DRVCUST_InitQuery(eT8032RebootOffset, &u4RebootOffset) || DRVCUST_InitQuery(eT8032RebootSize, &u4RebootSize))
        return 0;

    ASSERT(u4RebootSize >= 0x10 && u4RebootSize <= 1024 &&
           u4RebootOffset != 0 && u4RebootOffset < 0x9000);

    buf = malloc(u4RebootSize);
    buf[0] = 0;
    if (!PDWNC_Read_T8032_XData(u4RebootOffset, u4RebootSize, buf))
    {
        free(buf);
        return 0;
    }

    ret = strncmp(buf, "recovery", sizeof("recovery")-1);
    
    // Clear reboot reason area. But do not alter upgrade flag on last byte
    memset(buf, 0, u4RebootSize);
    PDWNC_Write_T8032_XData(u4RebootOffset, u4RebootSize-1, buf);
    free(buf);

    return (ret == 0);
#else
    return 0;
#endif /* CC_ENABLE_REBOOT_REASON */
}

#else

INT32 BIM_CheckEepromRecovery(void)
{
#ifdef CC_ENABLE_REBOOT_REASON
    extern 	INT32 EEPROM_Write(UINT64 u8Offset, UINT32 u4MemPtr, UINT32 u4MemLen);
    extern 	INT32 EEPROM_Read(UINT64 u8Offset, UINT32 u4MemPtr, UINT32 u4MemLen);
    
    int ret;
    INT32 i4Ret;

    char *buf;
    UINT32 u4RebootEepromOffset, u4RebootEepromSize;

    if (DRVCUST_InitQuery(eRebootEepromAddress, &u4RebootEepromOffset) || DRVCUST_InitQuery(eRebootEepromSize, &u4RebootEepromSize))
        return 0;

    buf = malloc(u4RebootEepromSize);
    buf[0] = 0;
    i4Ret=EEPROM_Read(u4RebootEepromOffset,  (UINT32)((void *)buf), u4RebootEepromSize-1);
    if (i4Ret !=0)
    {
        free(buf);
        return 0;
    }


    ret = strncmp(buf, "recovery", sizeof("recovery")-1);
    
    // Clear reboot reason area. But do not alter upgrade flag on last byte
    memset(buf, 0, u4RebootEepromSize);
    i4Ret=EEPROM_Write(u4RebootEepromOffset,  (UINT32)((void *)buf), u4RebootEepromSize-1);
    if (i4Ret !=0)
    {
        free(buf);
        return 0;
    }
    free(buf);

    return (ret == 0);
#else
    return 0;
#endif /* CC_ENABLE_REBOOT_REASON */
}
#endif

