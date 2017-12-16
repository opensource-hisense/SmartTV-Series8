/*
 * vm_linux/chiling/uboot/board/mt5398/mt5398_sif.c
 *
 * Copyright (c) 2010-2012 MediaTek Inc.
 *	This file is based  ARM Realview platform
 *  Copyright (C) 2002 ARM Ltd.
 *  All Rights Reserved
 * $Author: tao.tian $
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
#include <malloc.h>
#include "x_bim.h"
#include "x_timer.h"
#include "x_assert.h"
#include "x_ckgen.h"
#include "x_pinmux.h"
#include "x_typedef.h"
#include "x_ldr_env.h"
#include "x_gpio.h"

#include "sif_if.h"
#include "drvcust_if.h"

#include "eeprom_if.h"

//-----------------------------------------------------------------------------
// Configurations
//-----------------------------------------------------------------------------
#define CC_LOAD_DTVCFG_FROM_DRAM

//---------------------------------------------------------------------------
// Constant definitions
//---------------------------------------------------------------------------
#define CFG_CLK_DIV     ((CFG_SYS_CLK)/(CFG_BUS_CLK))
#if defined(CC_EMMC_BOOT)
#if defined(CC_TWO_WORLD_RFS)
#define STR_MTDBLOCK_ROOTFS "root=/dev/mmcblk0p1"
#else
#define STR_MTDBLOCK_ROOTFS "root=/dev/mmcblk0p"
#endif
#else
#define STR_MTDBLOCK_ROOTFS "root=/dev/mtdblock"
#endif
#define STR_KERNEL          "kernel"

//-----------------------------------------------------------------------------
// Macros
//-----------------------------------------------------------------------------
#ifdef DEBUG_I2C
#define PRINTD(fmt,args...)	do {	\
		printf (fmt ,##args);	\
	} while (0)
#else
#define PRINTD(fmt,args...)
#define NDEBUG
#endif
//---------------------------------------------------------------------------
// Type definitions
//---------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Local variables
//-----------------------------------------------------------------------------

//---------------------------------------------------------------------------
// Static variables
//---------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Local Functions
//-----------------------------------------------------------------------------

//-------------------------------------------------------------------------
//
// U-BOOT API Implementation (declared in uboot-xxx/include)
//
//-------------------------------------------------------------------------
/*-----------------------------------------------------------------------
 * Probe to see if a chip is present.  Also good for checking for the
 * completion of EEPROM writes since the chip stops responding until
 * the write completes (typically 10mSec).
 */
int i2c_probe(uchar addr)
{
	int ret;
	uchar ucData;
	/* perform 1 byte read transaction */
	ret = SIF_ReadMultipleSubAddr(CFG_CLK_DIV, addr , 0  ,0, &ucData, 1);
        	//rc = write_byte ((addr << 1) | 0);

	return (ret ? 1 : 0);
}

/*-----------------------------------------------------------------------
 * Read bytes
 */
int  i2c_read(uchar chip, uint addr, int alen, uchar *buffer, int len)
{
       int drv_ret=0, ret=0;

	PRINTD("i2c_read: chip %02X addr %02X alen %d buffer %p len %d\n",
	        chip, addr, alen, buffer, len);

	drv_ret = SIF_ReadMultipleSubAddr(CFG_CLK_DIV, chip , alen  , addr , buffer, len);

       ASSERT(drv_ret<= len );

       if (drv_ret != len )
            ret = drv_ret - len;        //fail
       else
            ret = 0;            //success

	return(ret);
}

/*-----------------------------------------------------------------------
 * Write bytes
 */
int  i2c_write(uchar chip, uint addr, int alen, uchar *buffer, int len)
{
       int drv_ret=0, ret=0;

	PRINTD("i2c_write: chip %02X addr %02X alen %d buffer %p len %d\n",
		chip, addr, alen, buffer, len);

	drv_ret = SIF_WriteMultipleSubAddr(CFG_CLK_DIV, chip ,  alen  ,addr , buffer, len);

       ASSERT(drv_ret<= len );

       if (drv_ret != len )
            ret = drv_ret - len;        //fail
       else
            ret = 0;            //success

	return(ret);
}

#if defined(REPLACE_EEPROM_WITH_FLASH)||defined(REPLACE_EEPROM_WITH_EMMC)
INT32 UBOOT_EEPROM_Read(UINT64 u8Offset, UINT32 u4MemPtr, UINT32 u4MemLen)
{
    memcpy(u4MemPtr, u8Offset, u4MemLen);

    return 0;
}


INT32 UBOOT_EEPDTV_Read(UINT64 u8Offset, UINT32 u4MemPtr, UINT32 u4MemLen)
{
    INT32 i4Ret;

    // adjust offset to map to physical offset.
    u8Offset +=  DRVCUST_EEPROM_DATA_MEM_OFFSET + EEPROM_DTV_DRIVER_OFFSET;

    i4Ret = UBOOT_EEPROM_Read(u8Offset, u4MemPtr, u4MemLen);

    return i4Ret;
}

#endif  // REPLACE_EEPROM_WITH_FLASH || REPLACE_EEPROM_WITH_EMMC

LDR_ENV_T *_prLdrEnv = NULL;

#ifdef CC_S_PLATFORM
char pcStr[16]={0};

char *UBOOT_EEP_GetSerial(void)
{
    if (_prLdrEnv == NULL)
    {
        _prLdrEnv = (LDR_ENV_T *)malloc(sizeof(LDR_ENV_T));
        ASSERT(_prLdrEnv);
        memcpy((void *)_prLdrEnv, (void *)CC_LDR_ENV_OFFSET, sizeof(LDR_ENV_T));
    }

    // load serial no from dram cache 1 by 1
    if(pcStr != NULL)
    {
      x_memcpy((void *)pcStr, (void *)_prLdrEnv->AndroidSerailNo, 15);
    }
	//Printf("UBOOT_EEP_GetSerial(%s)\n", pcStr);
	//Printf("UBOOT_EEP_GetSerial(%s)\n", _prLdrEnv->AndroidSerailNo);

    return pcStr;
}
#endif

INT32 UBOOT_EEPDTV_GetCfg(DTVCFG_T *prDtvCfg)
{
#ifndef CC_LOAD_DTVCFG_FROM_DRAM
#if defined(REPLACE_EEPROM_WITH_FLASH)||defined(REPLACE_EEPROM_WITH_EMMC)
    INT32 i4Ret;

    ASSERT(prDtvCfg != NULL);

    // read the first DtvCfg location.
    i4Ret = UBOOT_EEPDTV_Read((UINT64) 0, (UINT32)(void *)prDtvCfg, sizeof(DTVCFG_T));
    return i4Ret;
#else
    EEPDTV_GetCfg(prDtvCfg);
    return 0;
#endif //REPLACE_EEPROM_WITH_FLASH ||REPLACE_EEPROM_WITH_EMMC
#else
    if (_prLdrEnv == NULL)
    {
        _prLdrEnv = (LDR_ENV_T *)malloc(sizeof(LDR_ENV_T));
        ASSERT(_prLdrEnv);
        memcpy((void *)_prLdrEnv, (void *)CC_LDR_ENV_OFFSET, sizeof(LDR_ENV_T));
    }

    // load dtvcfg from dram cache
    x_memcpy((void *)prDtvCfg, (void *)_prLdrEnv->au1DtvCfg, sizeof(DTVCFG_T));
#endif //CC_LOAD_DTVCFG_FROM_DRAM

    return 0;
}

INT32 UBOOT_EEPDTV_IsFastBoot(void)
{
    DTVCFG_T rDtvCfg;
    UBOOT_EEPDTV_GetCfg(&rDtvCfg);
    return ((rDtvCfg.u1Flags3 & DTVCFG_FLAG3_FAST_BOOT) == DTVCFG_FLAG3_FAST_BOOT);
}

INT32 UBOOT_EEPDTV_IsSupportCLI(void)
{
    DTVCFG_T rDtvCfg;
    UBOOT_EEPDTV_GetCfg(&rDtvCfg);
    return ((rDtvCfg.u1Flags3 & DTVCFG_FLAG3_SUPPORT_CLI) == DTVCFG_FLAG3_SUPPORT_CLI);
}
#ifdef CC_S_PLATFORM
INT32 UBOOT_EEPDTV_IsU3ToADB(void)
{
    DTVCFG_T rDtvCfg;
    UBOOT_EEPDTV_GetCfg(&rDtvCfg);
    return ((rDtvCfg.u1Flags5 & DTVCFG_FLAG5_CUSTOM_FEATURE07_SUPPORT) != 0);
}
INT32 UBOOT_EEPDTV_IsADB(void)
{
    DTVCFG_T rDtvCfg;
    UBOOT_EEPDTV_GetCfg(&rDtvCfg);
    return ((rDtvCfg.u1Flags5 & DTVCFG_FLAG5_CUSTOM_FEATURE07_SUPPORT) != 0);
}
#endif

//-----------------------------------------------------------------------------
#if (CONFIG_DUAL_BANK_ROOTFS) || (CONFIG_DUAL_BANK_KERNEL)
#ifdef USING_EMMC_STORAGE_BOOTFLAG
extern UINT8 DecideBootPartition(void);
#endif

int dual_bank_init (void)
{
    DTVCFG_T rDtvCfg;
    int boot_flag;
    char u1blocknum = '4';
    char *s, buf[4096];
#ifndef USING_EMMC_STORAGE_BOOTFLAG

    UBOOT_EEPDTV_GetCfg(&rDtvCfg);

    boot_flag = (rDtvCfg.u1Flags3 & DTVCFG_FLAG3_KERNEL_BANK) == DTVCFG_FLAG3_KERNEL_BANK;
#else
boot_flag = DecideBootPartition();
#endif
    u1blocknum = (boot_flag ? 'B' : 'A');
    printf("Boot from kernel%c ", u1blocknum);
    if ((s = getenv ("bootcmd")) != NULL)
    {
        if (strstr((const char *)s, STR_KERNEL) != NULL)
        {
            char *subs;
            subs = strstr((const char *)s, STR_KERNEL);
            if (subs != NULL)
            {
                // update block number only if it doesn't match
                if (*(char *)(subs + strlen(STR_KERNEL)) != u1blocknum)
                {
                    *(char *)(subs + strlen(STR_KERNEL)) = u1blocknum;
                    if (strlen(s)<sizeof(buf))
                    {
                        strncpy(buf, s,strlen(s));
                        buf[strlen(s)] ='\0';
                        setenv("bootcmd", buf);
                        saveenv();
                    }
                    else
                    {
                        printf("error, env space is not enough\n");
                    }
                }
            }
        }
    }
    else
    {
        printf("get bootcmd failed.");
        ASSERT(0);
    }

    u1blocknum = (boot_flag ? MTDPARTS_NUM_OF_ROOTFS_B : MTDPARTS_NUM_OF_ROOTFS_A);
    #if defined(CC_TWO_WORLD_RFS)
	printf("and rootfs%c(partition 1%c)\n", (u1blocknum == MTDPARTS_NUM_OF_ROOTFS_A) ? 'A' : 'B', u1blocknum);
	#else
    printf("and rootfs%c(partition %c)\n", (u1blocknum == MTDPARTS_NUM_OF_ROOTFS_A) ? 'A' : 'B', u1blocknum);
    #endif
    // check if we need to replace block number of bootargs if there is a string STR_MTDBLOCK_ROOTFS
    if ((s = getenv ("bootargs")) != NULL)
    {
        if (strstr((const char *)s, STR_MTDBLOCK_ROOTFS) != NULL)
        {
            char *subs;
            subs = strstr((const char *)s, STR_MTDBLOCK_ROOTFS);
            if (subs != NULL)
            {
                // update block number only if it doesn't match
                if (*(char *)(subs + strlen(STR_MTDBLOCK_ROOTFS)) != u1blocknum)
                {
                    *(char *)(subs + strlen(STR_MTDBLOCK_ROOTFS)) = u1blocknum;
                    if (strlen(s)<sizeof(buf))
                    {
                        strncpy(buf, s,strlen(s));
                        buf[strlen(s)] ='\0';
                        setenv("bootargs", buf);
                        saveenv();
                    }
                    else
                    {
                        printf("error, env space is not enough\n");
                    }
                }
            }
        }
    }
    else
    {
        printf("get bootargs failed.");
        ASSERT(0);
    }

    return (0);
}

int get_boot_bank_index(void)
{
    DTVCFG_T rDtvCfg;
    int ret;

    UBOOT_EEPDTV_GetCfg(&rDtvCfg);

    ret = ((rDtvCfg.u1Flags3 & DTVCFG_FLAG3_KERNEL_BANK) ==
        DTVCFG_FLAG3_KERNEL_BANK)?1:0;

    return ret;
}
#endif

void dtvcfg_init(void)
{
    DTVCFG_T rDtvCfg;
    UBOOT_EEPDTV_GetCfg(&rDtvCfg);
}

 #if defined(CFG_EEPROM_WREN)
/*-----------------------------------------------------------------------
 * Enable/Disalbe WP Pin of EEPROM
 * Input: <dev_addr>  I2C address of EEPROM device to enable.
 *	   <state>     -1: deliver current state
 *		       0: disable write
 *		       1: enable write
 *  Returns:	       -1: wrong device address
 *                  -2: wrong parameter
 *			0: dis-/en- able done
 *		     0/1: current state if <state> was -1.
 */
int eeprom_write_enable (unsigned dev_addr, int state)
{
#ifdef CFG_I2C_EEPROM_ADDR
    if (CFG_I2C_EEPROM_ADDR != dev_addr)
    {
        return -1;
    }
    else
    {
        UINT32 u4SysWP, u4SysWPEnable;
        if (0 == DRVCUST_InitQuery(eSysEepromWPGpio, &u4SysWP) &&
           (0 == DRVCUST_InitQuery(eSysEepromWPEnablePolarity, &u4SysWPEnable)))
        {
            switch (state) {
            case 1:
                /* Enable write access */
                // disable write protect by GPIO.
                GPIO_SetOut( (INT32)u4SysWP , u4SysWPEnable?0:1 );
                state = 0;
                break;
            case 0:
                /* Disable write access,  */
                // Enable write protect by GPIO.
                GPIO_SetOut( (INT32)u4SysWP , u4SysWPEnable?1:0 );
                state = 0;
                break;
            default:
                state = -2;
                break;
            }
        }
    }
#endif // CFG_I2C_EEPROM_ADDR
    return state;
}
#endif     //CFG_EEPROM_WREN

