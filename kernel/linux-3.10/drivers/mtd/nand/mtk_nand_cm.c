 /*
 * drivers/mtd/nand/mtk_nand.c
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

#include <linux/slab.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>
#include <linux/mtd/nand_ecc.h>
#include <linux/dma-mapping.h>
#include <linux/jiffies.h>
#include <linux/semaphore.h>
#include <../mtdcore.h>

#include <x_typedef.h>
#include <linux/io.h>
#include <mach/hardware.h>
#include "mtk_nand_cm.h"
#include "nfi_common_hal_if.h"

#define NAND_RANDOM_EN              (1)
#if defined(CONFIG_ARCH_MT5399) || defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5861)
#define NAND_AUTORD_DONE            (1)
#else
#define NAND_AUTORD_DONE            (0)
#endif
#if CC_NAND_60BIT_NFI
#if NAND_RANDOM_EN
static  BOOL fgRandomizer = FALSE;
static  BOOL fgEmptyPage  = FALSE;
#endif
#endif
#ifdef CC_MTD_ENCRYPT_SUPPORT
#include <crypto/mtk_crypto.h>
#endif


#define NAND_DEBUG_PRINT            (0)

#if !defined(CONFIG_ARCH_MT5398) && !defined(CONFIG_ARCH_MT5399) && !defined(CONFIG_ARCH_MT5880) && !defined(CONFIG_ARCH_MT5860) && !defined(CONFIG_ARCH_MT5890)&& !defined(CONFIG_ARCH_MT5861)
#define NAND_DMA_CACHE_LINE         (32)
#define NAND_DMA_PATTERN            (0xFEDCBA98)
#define NAND_DMA_RETYR_DELAY        (2)
#define NAND_DMA_RETYR_CNT          (50)
#endif

#define NAND_DMA_READ_VERIFY        (0)
//static UINT32 _NAND_busy_count = 0;

// nand isr used
//UINT32 mtd_nand_isr_en = 1;
//static struct completion comp;

//static struct completion comp;
struct semaphore nand_pin_share;

// MTD structure for MT5365 board
struct mtd_info *mt53xx_mtd = NULL;

// For mtdchar.c and mtdsdm.c used
char *mtdchar_buf = NULL;
struct semaphore mtdchar_share;

// mtk partition information.
mtk_part_info_t rMTKPartInfo[MAX_MTD_DEVICES];

// static variables
BOOL _fgDmaEn = TRUE;
static NAND_FLASH_DEV_T *_prFlashDev;
static UINT32 _u4BlkPgCount;
static UINT32 _u4FdmNum;
static UINT32 _u4SectorNum;
static UINT32 _u4PgAddrLen;
static UINT32 _u4TotalAddrLen;
static UINT32 _u4PageSize = 2048;
static UINT32 _u4OOBSize = 16;
static UINT32 _u4SectorSize = 1024;
//static UINT32 _u4ECCBits = 0;
static UINT32 _u4CurBitFlip = 0;
static UINT32 _u4CurPage = 0xFFFFFFFF;

static UINT32 _u4NandDataBuf[(NAND_MAX_PAGE_SIZE + 64)/4];
static UINT32 _u4NandOobBuf[NAND_MAX_OOB_SIZE/4];

static UINT8 *_pu1NandDataBuf = NULL;
static UINT8 *_pu1NandOobBuf  = NULL;
static NFI_HWCFG_INFO_T *rNFI_HWCFG ;


#if NAND_DMA_READ_VERIFY
static UINT32 _u4CopyDataBuf[(NAND_MAX_PAGE_SIZE + 64)/4];
static UINT32 _u4CopyOobBuf[NAND_MAX_OOB_SIZE/4];
static UINT32 _u4VerifyDataBuf[(NAND_MAX_PAGE_SIZE + 64)/4];
static UINT32 _u4VerifyOobBuf[NAND_MAX_OOB_SIZE/4];
#endif


//-----------------------------------------------------------------------------
/** NANDHW_GetDevCount()
 */
//-----------------------------------------------------------------------------
static UINT32 _NANDHW_GetDevCount(void)
{
    return sizeof(_arNand_DevInfo) / sizeof(NAND_FLASH_DEV_T);
}

//-----------------------------------------------------------------------------
/** NANDHW_GetDev()
 */
//-----------------------------------------------------------------------------
static NAND_FLASH_DEV_T* _NANDHW_GetDev(UINT32 u4Idx)
{
    UINT32 u4DevCount;

    u4DevCount = _NANDHW_GetDevCount();

    if(u4Idx >= u4DevCount)
    {
        return NULL;
    }

    return (((NAND_FLASH_DEV_T *)_arNand_DevInfo) + u4Idx);
}

//-----------------------------------------------------------------------------
/** _NANDHW_countbits()
 */
//-----------------------------------------------------------------------------
static UINT32 _NANDHW_countbits(UINT32 byte)
{
    UINT32 res = 0;

    for (; byte; byte >>= 1)
    {
        res += (byte & 0x01);
    }

    return res;
}

//-----------------------------------------------------------------------------
/** _NANDHW_Command()
 */
//-----------------------------------------------------------------------------
#if defined(CC_AC_DROP_GPIO_ISR)
UINT8 _bACDropISR = 0;
EXPORT_SYMBOL(_bACDropISR);
#endif

//-----------------------------------------------------------------------------
/** _NANDHW_DataExchange()
 */
//-----------------------------------------------------------------------------
static INT32 _NANDHW_DataExchange(UINT32 *pu4Buf, UINT32 *pu4Spare, UINT32 u4PgOff, UINT32 u4Len)
{
		UINT32 u4Val = 0, u4Addr = 0;

    if (!pu4Buf)
    {
        return NAND_FAIL;
    }
#if defined(CONFIG_ARCH_MT5881)
		u4Val = _u4OOBSize;
		if (_u4OOBSize == 28)
		{
			if (_u4PageSize == 0x1000)
			{
				u4Addr = 0xF3C;
			}
			else if (_u4PageSize == 0x2000)
			{
				u4Addr = 0x1E7A;
			}
			else
			{
				return NAND_SUCC;
			}
		}
		else
#endif
    if (_u4OOBSize == 16)
    {
        if (_u4PageSize == 0x800)
        {
					u4Addr = 0x7E0 - u4Val;
        }
        else if (_u4PageSize == 0x1000)
        {
					u4Addr = 0xFA0 - u4Val;
        }
        else if (_u4PageSize == 0x2000)
        {
					u4Addr = 0x1F20 - u4Val;
				}
				else if (_u4PageSize == 0x4000)
				{
					u4Addr = 0x3E20 - u4Val;
				}
        else
        {
            return NAND_SUCC;
        }
    }
    else if (_u4OOBSize == 26)
    {
        if (_u4PageSize == 0x800)
        {
					u4Addr = 0x7CC - u4Val;
        }
        else if (_u4PageSize == 0x1000)
        {
					u4Addr = 0xF64 - u4Val;
        }
        else if (_u4PageSize == 0x2000)
        {
					u4Addr = 0x1E94 - u4Val;
				}
				else if (_u4PageSize == 0x4000)
				{
					u4Addr = 0x3CF4 - u4Val;
				}
        else
        {
            return NAND_SUCC;
        }
    }
#if CC_NAND_60BIT_NFI
    else if (_u4OOBSize == 32)
    {
        if (_u4PageSize == 0x1000)
        {
            u4Addr = 0xF40;
        }
        else if (_u4PageSize == 0x2000)
        {
            u4Addr = 0x1E40;
				}
				else if (_u4PageSize == 0x4000)
				{
					u4Addr = 0x4000 - 15*64;
				}
        else
        {
            return NAND_SUCC;
        }
    }
    else if (_u4OOBSize == 36)
    {
        if (_u4PageSize == 0x1000)
        {
            u4Addr = 0xF28;
        }
        else if (_u4PageSize == 0x2000)
        {
            u4Addr = 0x1E08;
        }
        else
        {
            return NAND_SUCC;
        }
    }
    else if (_u4OOBSize == 40)
    {
        if (_u4PageSize == 0x1000)
        {
            u4Addr = 0xF10;
        }
        else if (_u4PageSize == 0x2000)
        {
            u4Addr = 0x1DD0;
        }
        else
        {
            return NAND_SUCC;
        }
    }
    else if (_u4OOBSize == 64)
    {
        if (_u4PageSize == 0x1000)
        {
            u4Addr = 0xE80;
        }
        else if (_u4PageSize == 0x2000)
        {
            u4Addr = 0x1C80;
        }
        else
        {
            return NAND_SUCC;
        }
    }
#endif
    else
    {
        return NAND_FAIL;
    }
    
    if ((u4Addr >= u4PgOff) && (u4Addr < (u4PgOff + u4Len)))
    {
        u4Val = pu4Buf[(u4Addr-u4PgOff)>>2];
        pu4Buf[(u4Addr-u4PgOff)>>2] = pu4Spare[0];
        pu4Spare[0] = u4Val;
    }
    
    return NAND_SUCC;
}

/** _NANDHW_ProbeDev()
 */
//-----------------------------------------------------------------------------

static INT32 _NANDHW_ProbeDev(void)
{
    NAND_FLASH_DEV_T *prDev;
    UINT32 u4ID,u4IDEx,i, u4DevCount;
	UCHAR ucID[6];

    NFI_ReadID(ucID,6);
	u4ID = ucID[3]<<24 | ucID[2]<<16 | ucID[1]<<8 | ucID[0];
	u4IDEx =  ucID[5]<<8 |ucID[4];
    printk(KERN_INFO "Detect NAND flash ID: 0x%X\n", u4ID);

    u4DevCount = _NANDHW_GetDevCount();
    for(i = 0; i < u4DevCount; i++)
    {
        prDev = (NAND_FLASH_DEV_T *)_NANDHW_GetDev(i);
        if((u4ID & prDev->u4IDMask) == (prDev->u4IDVal & prDev->u4IDMask))
        {
            _prFlashDev = prDev;

            if(0x1590DA98 == u4ID)
            {
                if(0x00001676 == u4IDEx)
                {
                    _prFlashDev->u4OOBSize = 26;
                    _prFlashDev->u4BBMode  = 0x00000008;
                    printk(KERN_INFO " NAND flash ExID: 0x%X\n", 0x00001676);
                }
            }
			else if(0x9590DA2C == u4ID)
            {
                if(0x00000004 == u4IDEx)
                {
                    _prFlashDev->u4OOBSize = 26;
                    _prFlashDev->u4BBMode  = 0x00000008;
                    printk(KERN_INFO " NAND flash ExID: 0x%X\n", 0x00000004);
                }
            }
            else if(0x9590DCAD == u4ID)
            {
                if(0x00000056 == (0x000000FF & u4IDEx))
                {
                    _prFlashDev->u4OOBSize = 26;
                    _prFlashDev->u4BBMode  = 0x00000008;
                    printk(KERN_INFO " NAND flash ExID: 0x%X\n", 0x00000056);
                }
            }
            else if(0x9590DC01 == u4ID)
            {
                if(0x00000056 == (0x000000FF & u4IDEx))
                {
                    _prFlashDev->u4OOBSize = 26;
                    _prFlashDev->u4BBMode  = 0x00000008;
                    printk(KERN_INFO " NAND flash ExID: 0x%X\n", 0x00000056);
                }
            }
            break;
        }
    }

    if(i == u4DevCount)
    {
        _prFlashDev = NULL;
        return NAND_FAIL;
    }

    return NAND_SUCC;
}

//-----------------------------------------------------------------------------
/** _NANDHW_Init()
 */
//-----------------------------------------------------------------------------

static void _NANDHW_SetSectorSize(void)
{
#if !defined(CONFIG_ARCH_MT5881)
    if(_u4PageSize > 512)
    {
        _u4SectorSize = 1024;
    }
    else
#endif
    {
        _u4SectorSize = 512;
    }
}


static INT32 _NANDHW_Init(void)
{
#ifndef CONFIG_MT53_FPGA
    UINT32 i;

    // Set nand/nor pinmux.
    NFI_PMUXSetting();

    _u4BlkPgCount = 64;
    _prFlashDev = NULL;
    _u4PgAddrLen  = 3;
    _u4TotalAddrLen = 5;
    _u4PageSize = 2048;
    _u4OOBSize = 16;
	rNFI_HWCFG->u4FDMNum = 0;
	rNFI_HWCFG->u4FDMECCNum = 0; 
	rNFI_HWCFG->u4Eccbits_hal = 0;
	_u4FdmNum = 0;
	
#if defined(CONFIG_ARCH_MT5881)
    _u4SectorSize = 512;
#else
    _u4SectorSize = 1024;
#endif

#if NAND_RANDOM_EN
    if(TRUE == NFI_RandomEnable())//5399 randomizer efuse bit
        fgRandomizer = TRUE;
    else
        fgRandomizer = FALSE;	
#endif

	rNFI_HWCFG->rCtrlInfo.fgRandomizer = fgRandomizer;
	rNFI_HWCFG->rCtrlInfo.fgAutoRdDone = TRUE;
	
    for(i = 0; i < 3; i++)
    {
        // Reset NFI controller
      //  _NANDHW_Reset();
      	NFI_ResetDev(0);

        if(_NANDHW_ProbeDev() == NAND_SUCC)
        {
            printk(KERN_INFO "Detect %s NAND flash: %dMB\n", 
                _prFlashDev->acName, (_prFlashDev->u4ChipSize/0x100000));
			
            	
			rNFI_HWCFG->rDevInfo.u4BBMode = _prFlashDev->u4BBMode;
			rNFI_HWCFG->rDevInfo.u4BlkSz = _prFlashDev->u4BlkSize;
			rNFI_HWCFG->rDevInfo.u4OOBSz = _prFlashDev->u4OOBSize;
			rNFI_HWCFG->rDevInfo.u4TotalSize = _prFlashDev->u4ChipSize;
			rNFI_HWCFG->rDevInfo.u4PgPerBlk = _prFlashDev->u4BlkPgCount;
			rNFI_HWCFG->rDevInfo.u4BBMode = _prFlashDev->u4BBMode;
            _u4PageSize = _prFlashDev->u4PageSize;
            _u4OOBSize =  _prFlashDev->u4OOBSize;
			rNFI_HWCFG->rDevInfo.u4PgSz = _u4PageSize;
			rNFI_HWCFG->rDevInfo.u4OOBSz = _u4OOBSize;
			_NANDHW_SetSectorSize();
			rNFI_HWCFG->u4SectorSize = _u4SectorSize;
			rNFI_HWCFG->u4SectorNum = _u4PageSize/_u4SectorSize;
			rNFI_HWCFG->u4FDMNum = _u4FdmNum;


			rNFI_HWCFG->rCtrlInfo.rECC_Type = NFIECC_NONE; 
			NFI_Get_Eccbit_CFG(rNFI_HWCFG);
		
            if(_prFlashDev->u4PageSize == 512)
            {
                _u4PgAddrLen = (UINT32)(_prFlashDev->u1AddrCycle - 1);
                _u4TotalAddrLen = _prFlashDev->u1AddrCycle;
            }
            else
            {
                _u4PgAddrLen = (UINT32)(_prFlashDev->u1AddrCycle - 2);
                _u4TotalAddrLen = _prFlashDev->u1AddrCycle;
            }

            _u4BlkPgCount = _prFlashDev->u4BlkPgCount;
			rNFI_HWCFG->rDevInfo.u4PgPerBlk = _u4BlkPgCount;
			rNFI_HWCFG->rDevInfo.u4RowNOB = _u4PgAddrLen;
			rNFI_HWCFG->rDevInfo.u4ColNOB = _u4TotalAddrLen -_u4PgAddrLen;
			
            break;
        }

        printk(KERN_ERR "Not detect any flash!\n");
        _u4PgAddrLen = 2;
        _u4TotalAddrLen = 5;
        _u4PageSize = 2048;
        _u4OOBSize = 16;
    }

    NFI_mtd_nand_regisr();
	rNFI_HWCFG->u4FDMNum = 8;
	rNFI_HWCFG->u4FDMECCNum = 8; 
	NFI_SetPageFmt(rNFI_HWCFG);
	rNFI_HWCFG->pu4Main_Buf = (UINT32 *)((((UINT32)_u4NandDataBuf >> 6) << 6) + 64);
	rNFI_HWCFG->pu4Spare_Buf = (UINT32 *)_u4NandOobBuf;
	rNFI_HWCFG->pu4Partial_Buf = rNFI_HWCFG->pu4Main_Buf;

// Aligned the buffer for DMA used.
   _pu1NandDataBuf = (UINT8 *)((((UINT32)_u4NandDataBuf >> 6) << 6) + 64);
   _pu1NandOobBuf  = (UINT8 *)_u4NandOobBuf;

    
#ifdef CC_MTD_ENCRYPT_SUPPORT
    NAND_AES_INIT();
#endif
#endif
    return NAND_SUCC;
}




void _NANDHW_CLKONOFF(BOOL fgOn)
{
    if(fgOn) //turn on clk
    {        
        #if defined(CONFIG_ARCH_MT5389)
        NAND_IO_WRITE32(0xF000D260, 4);   // 120MHZ
        #elif defined(CONFIG_ARCH_MT5368) || defined(CONFIG_ARCH_MT5396)
        NAND_IO_WRITE32(0xF000D260, 6);   // 162MHZ
        #elif defined(CONFIG_ARCH_MT5398) || defined(CONFIG_ARCH_MT5399) || defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5861)
        NAND_IO_WRITE32(0xF000D328, 6);   // 192MHZ
        #elif defined(CONFIG_ARCH_MT5880) || defined(CONFIG_ARCH_MT5860)|| defined(CONFIG_ARCH_MT5881)
        NAND_IO_WRITE32(0xF000D328, 6);   // 192MHZ
        #else
        NAND_IO_WRITE32(0xF000D260, 6);   // 216MHZ
        #endif
    }
    else //turn off clk
    {
        #if defined(CONFIG_ARCH_MT5389)
        NAND_IO_WRITE32(0xF000D260, 0x80); 
        #elif defined(CONFIG_ARCH_MT5368) || defined(CONFIG_ARCH_MT5396)
        NAND_IO_WRITE32(0xF000D260, 0x80); 
        #elif defined(CONFIG_ARCH_MT5398) || defined(CONFIG_ARCH_MT5399) || defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5861)
        NAND_IO_WRITE32(0xF000D328, 0x80); 
        #elif defined(CONFIG_ARCH_MT5880) || defined(CONFIG_ARCH_MT5860)|| defined(CONFIG_ARCH_MT5881)
        NAND_IO_WRITE32(0xF000D328, 0x80); 
        #else
        NAND_IO_WRITE32(0xF000D260, 0x80); 
        #endif
    }
}



//-----------------------------------------------------------------------------
/** mtk_nand_markbadpage()
 */
//-----------------------------------------------------------------------------
static int mtk_nand_markbadpage(struct mtd_info *mtd, int page)
{
    struct nand_chip *chip = mtd->priv;
    int status = 0;
	dma_addr_t u4DmaAddr;
	UINT32 u4PhyAddr;
	UINT32 *u4NandDataBuf;
	  

    memset(rNFI_HWCFG->pu4Main_Buf, 0, mtd->writesize);
    memset(rNFI_HWCFG->pu4Spare_Buf, 0, mtd->oobsize);

    chip->cmdfunc(mtd, NAND_CMD_SEQIN, 0, page);


    rNFI_HWCFG->u4PageIdx = _u4CurPage;
	rNFI_HWCFG->rCtrlInfo.fgAHBMode = _fgDmaEn; //DMA
	rNFI_HWCFG->rCtrlInfo.fgAHBMode = TRUE;
	rNFI_HWCFG->u4Offset = 0;
	rNFI_HWCFG->rCtrlInfo.rECC_Type = chip->ecc.mode;
	rNFI_HWCFG->rCtrlInfo.fgInterrupt = TRUE;


	u4NandDataBuf = (UINT32 *)_pu1NandDataBuf;
	u4PhyAddr = virt_to_phys((void *)u4NandDataBuf);
	u4PhyAddr += NAND_DRAM_BASE; 
	rNFI_HWCFG->pu4Main_Buf = (UINT32 *)u4PhyAddr;
	u4NandDataBuf = (UINT32 *)_pu1NandDataBuf;
	u4DmaAddr = dma_map_single(NULL, (void *)u4NandDataBuf, _u4PageSize, DMA_FROM_DEVICE);
		
	NFI_PageProgram(rNFI_HWCFG);

	dma_unmap_single(NULL, u4DmaAddr, _u4PageSize, DMA_FROM_DEVICE);

	rNFI_HWCFG->pu4Main_Buf = (UINT32 *)_pu1NandDataBuf;
	rNFI_HWCFG->pu4Spare_Buf = (UINT32 *)_pu1NandOobBuf;
	

	
    chip->cmdfunc(mtd, NAND_CMD_PAGEPROG, -1, -1);
    status = chip->waitfunc(mtd, chip);
    return status & NAND_STATUS_FAIL ? -EIO : 0;
}

//-----------------------------------------------------------------------------
/** mtk_nand_markbadblk()
 */
//-----------------------------------------------------------------------------
static int mtk_nand_markbadblk(struct mtd_info *mtd, loff_t ofs)
{
    struct nand_chip *chip = mtd->priv;
    int block;

    block = (int)(ofs >> chip->phys_erase_shift);

    printk(KERN_CRIT "mtk_nand_markbadblk: %d\n", block);

    // erase block before mark it bad,
    // cause MLC does not support re-write absolutely. mtk40109 2010-12-06
	rNFI_HWCFG->u4PageIdx = block * _u4BlkPgCount;
	NFI_BlockErase(rNFI_HWCFG);

    // Mark the first page of the block.
    mtk_nand_markbadpage(mtd, block * _u4BlkPgCount);

    // Mark the last page of the block if MLC.
    if (_u4OOBSize > 16)
    {
        mtk_nand_markbadpage(mtd, block * _u4BlkPgCount + _u4BlkPgCount - 1);
    }

    // Always return successful ,
    // cause ubifs will become read-only mode while erase fail.  mtk40109 2010-12-06
    return 0;
}

//-----------------------------------------------------------------------------
/** mtk_nand_verify_buf()
 */
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
/** mtk_nand_read_byte()
 */
//-----------------------------------------------------------------------------
static u_char mtk_nand_read_byte(struct mtd_info *mtd)
{
    UINT8 u1Reg = 0;
	
	rNFI_HWCFG->rCtrlInfo.rECC_Type = NFIECC_NONE;
	rNFI_HWCFG->rCtrlInfo.fgAHBMode = FALSE;
	rNFI_HWCFG->rCtrlInfo.fgAutoFmt = FALSE;
	u1Reg = NFI_ReadOneByte(rNFI_HWCFG);

	return u1Reg;
}

//-----------------------------------------------------------------------------
/** mtk_nand_read_buf()
 */
//-----------------------------------------------------------------------------
static void mtk_nand_read_buf(struct mtd_info *mtd, UINT8 *buf, INT32 len)
{
    printk(KERN_ERR "mtk_nand_read_buf addr=0x%08x, len=%d\n", (UINT32)buf, len);
    BUG();
}

//-----------------------------------------------------------------------------
/** mtk_nand_write_buf()
 */
//-----------------------------------------------------------------------------
static void mtk_nand_write_buf(struct mtd_info *mtd, const UINT8 *buf, INT32 len)
{
    printk(KERN_ERR "mtk_nand_write_buf addr=0x%08x, len=%d\n", (UINT32)buf, len);
    BUG();
}

//-----------------------------------------------------------------------------
/** mt5365_nand_need_aes()
 */
//-----------------------------------------------------------------------------
#ifdef CC_MTD_ENCRYPT_SUPPORT
static int mtk_nand_enctyped(struct mtd_info *mtd, int page, UINT32 *pu4Data)
{
    int i, j;
    mtk_part_info_t *p;

    for(i = 0; i < MAX_MTD_DEVICES; i++)
    {
        p = &(rMTKPartInfo[i]);

        if (page >= p->u4PgStart && page <= p->u4PgEnd)
        {
            if (p->u4Encryped == 1)
            {
                for (j = 0; j < mtd->writesize/4; j++)
                {
                    if (pu4Data[j] != 0xFFFFFFFF)
                    {
                        return 1;
                    }
                }
            }

            break;
        }
    }

    return 0;
}
#endif

//-----------------------------------------------------------------------------
/** mtk_nand_read_page()
 */
//-----------------------------------------------------------------------------
static int mtk_nand_read_page(struct mtd_info *mtd, struct nand_chip *chip, UINT8 *buf, int oob_required, int page)
{
    BOOL i4Ret = TRUE;
    UINT8 *pu1Data  = (UINT8 *)buf;
    UINT8 *pu1Spare = (UINT8 *)chip->oob_poi;
	dma_addr_t u4DmaAddr;
	UINT32 u4PhyAddr;
	UINT32 *u4NandDataBuf;

	
    #if NAND_DEBUG_PRINT
    printk(KERN_INFO "mtk_nand_read_page, page = 0x%X\n", _u4CurPage);
    #endif

    if ((UINT32)page != _u4CurPage)
    {
        BUG();
    }
    fgEmptyPage  = FALSE;

	if(_fgDmaEn)
	{
		rNFI_HWCFG->u4PageIdx = _u4CurPage;
		rNFI_HWCFG->rCtrlInfo.fgAHBMode = _fgDmaEn;
		rNFI_HWCFG->u4Offset = 0;
		rNFI_HWCFG->rCtrlInfo.rECC_Type = chip->ecc.mode;
		rNFI_HWCFG->rCtrlInfo.fgInterrupt = TRUE;
		rNFI_HWCFG->rCtrlInfo.fgAutoRdDone = TRUE;

		u4NandDataBuf = (UINT32 *)_pu1NandDataBuf;
		u4PhyAddr = virt_to_phys((void *)u4NandDataBuf);
		u4PhyAddr += NAND_DRAM_BASE; 
		rNFI_HWCFG->pu4Main_Buf = (UINT32 *)u4PhyAddr;
		u4NandDataBuf = (UINT32 *)_pu1NandDataBuf;
		u4DmaAddr = dma_map_single(NULL, (void *)u4NandDataBuf, _u4PageSize, DMA_FROM_DEVICE);
		i4Ret =NFI_PageRead(rNFI_HWCFG);
		dma_unmap_single(NULL, u4DmaAddr, _u4PageSize, DMA_FROM_DEVICE);
		rNFI_HWCFG->pu4Main_Buf = (UINT32 *)_pu1NandDataBuf;
		rNFI_HWCFG->pu4Spare_Buf = (UINT32 *)_pu1NandOobBuf;
	}
	else
	{
		rNFI_HWCFG->u4PageIdx = _u4CurPage;
		rNFI_HWCFG->rCtrlInfo.fgAHBMode = _fgDmaEn;
		rNFI_HWCFG->u4Offset = 0;
		rNFI_HWCFG->rCtrlInfo.rECC_Type = chip->ecc.mode;
		rNFI_HWCFG->rCtrlInfo.fgInterrupt = FALSE;
		rNFI_HWCFG->rCtrlInfo.fgAutoRdDone = FALSE;

		i4Ret =NFI_PageRead(rNFI_HWCFG);
		rNFI_HWCFG->pu4Main_Buf = (UINT32 *)_pu1NandDataBuf;
		rNFI_HWCFG->pu4Spare_Buf = (UINT32 *)_pu1NandOobBuf;
	}
	
	if (i4Ret)
    {
        printk(KERN_ERR " mtk_nand_read_page fail, page = 0x%X\n", _u4CurPage);
        mtd->ecc_stats.failed++;
    }
    else
    {
        mtd->ecc_stats.corrected += _u4CurBitFlip;
        _NANDHW_DataExchange((UINT32 *)rNFI_HWCFG->pu4Main_Buf, (UINT32 *)rNFI_HWCFG->pu4Spare_Buf, 0, mtd->writesize);
#ifdef CC_MTD_ENCRYPT_SUPPORT
        if (mtk_nand_enctyped(mtd, _u4CurPage, (UINT32 *)_pu1NandDataBuf))
        {
            NAND_AES_Decryption(virt_to_phys((void *)rNFI_HWCFG->pu4Main_Buf), virt_to_phys((void *)rNFI_HWCFG->pu4Main_Buf), mtd->writesize);
        }
#endif
    }

     memcpy(pu1Data,  rNFI_HWCFG->pu4Main_Buf, mtd->writesize);
     memcpy(pu1Spare, rNFI_HWCFG->pu4Spare_Buf, mtd->oobsize);

    return  0;
}

//-----------------------------------------------------------------------------
/** mtk_nand_write_page()
 */
//-----------------------------------------------------------------------------
static int mtk_nand_write_page(struct mtd_info *mtd, struct nand_chip *chip, const UINT8 *buf, int oob_required)
{
    UINT8 *pu1Data  = (UINT8 *)buf;
    UINT8 *pu1Spare = (UINT8 *)chip->oob_poi;
	dma_addr_t u4DmaAddr;
	UINT32 u4PhyAddr;
	UINT32 *u4NandDataBuf;

    #if NAND_DEBUG_PRINT
    printk(KERN_INFO "mtk_nand_write_page, page = 0x%X\n", _u4CurPage);
    #endif

   memcpy(_pu1NandDataBuf, pu1Data, mtd->writesize);
    memcpy(_pu1NandOobBuf,  pu1Spare, mtd->oobsize);

#ifdef CC_MTD_ENCRYPT_SUPPORT
    if (mtk_nand_enctyped(mtd, _u4CurPage, (UINT32 *)_pu1NandDataBuf))
    {
        NAND_AES_Encryption(virt_to_phys((void *)_pu1NandDataBuf), virt_to_phys((void *)_pu1NandDataBuf), mtd->writesize);
    }
#endif

	_NANDHW_DataExchange((UINT32 *)_pu1NandDataBuf, (UINT32 *)_pu1NandOobBuf, 0, mtd->writesize);
	rNFI_HWCFG->pu4Main_Buf = (UINT32 *)((((UINT32)_u4NandDataBuf >> 6) << 6) + 64);
	rNFI_HWCFG->pu4Spare_Buf = (UINT32 *)_u4NandOobBuf;


	if (_fgDmaEn)
	{
		rNFI_HWCFG->u4PageIdx = _u4CurPage;
		rNFI_HWCFG->rCtrlInfo.fgAHBMode = _fgDmaEn;
		rNFI_HWCFG->u4Offset = 0;
		rNFI_HWCFG->rCtrlInfo.rECC_Type = chip->ecc.mode;
		rNFI_HWCFG->rCtrlInfo.fgInterrupt = TRUE;


		u4NandDataBuf = (UINT32 *)_pu1NandDataBuf;
		u4PhyAddr = virt_to_phys((void *)u4NandDataBuf);
		u4PhyAddr += NAND_DRAM_BASE; 
		rNFI_HWCFG->pu4Main_Buf = (UINT32 *)u4PhyAddr;
		u4NandDataBuf = (UINT32 *)_pu1NandDataBuf;
		u4DmaAddr = dma_map_single(NULL, (void *)u4NandDataBuf, _u4PageSize, DMA_TO_DEVICE);
		NFI_PageProgram(rNFI_HWCFG);
		dma_unmap_single(NULL, u4DmaAddr, _u4PageSize, DMA_TO_DEVICE);

	}
	else
	{
		rNFI_HWCFG->u4PageIdx = _u4CurPage;
		rNFI_HWCFG->rCtrlInfo.fgAHBMode = _fgDmaEn;
		rNFI_HWCFG->u4Offset = 0;
		rNFI_HWCFG->rCtrlInfo.rECC_Type = chip->ecc.mode;
		rNFI_HWCFG->rCtrlInfo.fgInterrupt = FALSE;
		rNFI_HWCFG->rCtrlInfo.fgAutoRdDone = FALSE;

		NFI_PageProgram(rNFI_HWCFG);
	}

	return 0;

}

static int mtk_nand_read_oob(struct mtd_info *mtd, struct nand_chip *chip, INT32 page)
{
    BOOL i4Ret = TRUE;
    UINT8 *pu1Spare = (UINT8 *)chip->oob_poi;
	dma_addr_t u4DmaAddr;
	UINT32 u4PhyAddr;
	UINT32 *u4NandDataBuf;
	

    #if NAND_DEBUG_PRINT
    printk(KERN_INFO "mtk_nand_read_oob\n");
    #endif

	if(_fgDmaEn)
	{
		rNFI_HWCFG->u4PageIdx = _u4CurPage;
		rNFI_HWCFG->rCtrlInfo.fgAHBMode = _fgDmaEn;
		rNFI_HWCFG->u4Offset = 0;
		rNFI_HWCFG->rCtrlInfo.rECC_Type = chip->ecc.mode;
		rNFI_HWCFG->rCtrlInfo.fgInterrupt = TRUE;
		rNFI_HWCFG->rCtrlInfo.fgAutoRdDone = TRUE;

		u4NandDataBuf = (UINT32 *)_pu1NandDataBuf;
		u4PhyAddr = virt_to_phys((void *)u4NandDataBuf);
		u4PhyAddr += NAND_DRAM_BASE; 
		rNFI_HWCFG->pu4Main_Buf = (UINT32 *)u4PhyAddr;
		u4NandDataBuf = (UINT32 *)_pu1NandDataBuf;
		u4DmaAddr = dma_map_single(NULL, (void *)u4NandDataBuf, _u4PageSize, DMA_FROM_DEVICE);
		i4Ret =NFI_PageRead(rNFI_HWCFG);
		dma_unmap_single(NULL, u4DmaAddr, _u4PageSize, DMA_FROM_DEVICE);
		rNFI_HWCFG->pu4Main_Buf = (UINT32 *)_pu1NandDataBuf;
		rNFI_HWCFG->pu4Spare_Buf = (UINT32 *)_pu1NandOobBuf;
	}
	else
	{
		rNFI_HWCFG->u4PageIdx = _u4CurPage;
		rNFI_HWCFG->rCtrlInfo.fgAHBMode = _fgDmaEn;
		rNFI_HWCFG->u4Offset = 0;
		rNFI_HWCFG->rCtrlInfo.rECC_Type = chip->ecc.mode;
		rNFI_HWCFG->rCtrlInfo.fgInterrupt = FALSE;
		rNFI_HWCFG->rCtrlInfo.fgAutoRdDone = FALSE;

		i4Ret =NFI_PageRead(rNFI_HWCFG);
		rNFI_HWCFG->pu4Main_Buf = (UINT32 *)_pu1NandDataBuf;
		rNFI_HWCFG->pu4Spare_Buf = (UINT32 *)_pu1NandOobBuf;
	}
	

    if (i4Ret)
    {
        printk(KERN_ERR "mtk_nand_read_oob fail, page = 0x%X\n", _u4CurPage);
        mtd->ecc_stats.failed++;
    }
    else
    {
        mtd->ecc_stats.corrected += _u4CurBitFlip;
		_NANDHW_DataExchange((UINT32 *)_pu1NandDataBuf, (UINT32 *)_pu1NandOobBuf, 0, mtd->writesize);

    }

    memcpy(pu1Spare, _pu1NandDataBuf, mtd->oobsize);

    return 0;
}

static int mtk_nand_write_oob(struct mtd_info *mtd, struct nand_chip *chip, INT32 page)
{
    printk(KERN_ERR "mtk_nand_write_oob\n");
    BUG();

    return 0;
}

//-----------------------------------------------------------------------------
/** mtk_nand_dev_ready()
 */
//-----------------------------------------------------------------------------
static int mtk_nand_dev_ready(struct mtd_info *mtd)
{
    return 1;
}

//-----------------------------------------------------------------------------
/** mtk_nand_select_chip()
 */
//-----------------------------------------------------------------------------
static void mtk_nand_select_chip(struct mtd_info *mtd, INT32 chip)
{
}

//-----------------------------------------------------------------------------
/** mtk_nand_command()
 */
//-----------------------------------------------------------------------------
static void mtk_nand_command(struct mtd_info *mtd, unsigned command, INT32 column, INT32 page_addr)
{
    //struct nand_chip *chip = mtd->priv;
	UINT32 BlkPgCount;
    #if NAND_DEBUG_PRINT
    printk(KERN_INFO "command: 0x%x, col: 0x%x, page: 0x%08x\n", command, column, page_addr);
    #endif

    _u4CurPage = (UINT32)page_addr;
    _u4CurBitFlip = 0;
	rNFI_HWCFG->u4PageIdx = _u4CurPage;
	BlkPgCount = rNFI_HWCFG->rDevInfo.u4PgPerBlk;

    switch(command)
    {
    case NAND_CMD_READID:
		NFI_CmdFun(command);
		NFI_AddrFun(0,0,1);
        break;

    case NAND_CMD_READ0:
        if (column >= (mtd->writesize + mtd->oobsize))
        {
            printk(KERN_ERR "oobsize is too large\n");
            BUG();
        }

#if NAND_RANDOM_EN
        if (fgRandomizer == TRUE)
        {
            NFI_Randomizer_Decode(_u4CurPage, BlkPgCount, TRUE);
        }
#endif
        break;

    case NAND_CMD_READ1:
        if (mtd->writesize != 512)
        {
            printk(KERN_ERR "Unhandle command!\n");
            BUG();
        }
        if (column >= (mtd->writesize + mtd->oobsize))
        {
            printk(KERN_ERR "oobsize is too large\n");
            BUG();
        }
		NFI_CmdFun(command);
		NFI_AddrFun(page_addr, column, (_u4PgAddrLen<<4)|(_u4TotalAddrLen - _u4PgAddrLen));
        break;

    case NAND_CMD_READOOB:
        if (mtd->writesize != 512)
        {
            column += mtd->writesize;
            command = NAND_CMD_READ0;
        }
        if (column >= (mtd->writesize + mtd->oobsize))
        {
            printk(KERN_ERR "oobsize is too large\n");
            BUG();
        }
		NFI_CmdFun(command);
		NFI_AddrFun(page_addr, column, (_u4PgAddrLen<<4)|(_u4TotalAddrLen - _u4PgAddrLen));
        if (mtd->writesize != 512)
        {
            NFI_CmdFun(NAND_CMD_READSTART);
        }

        break;

    case NAND_CMD_SEQIN:
        if (column != 0)
        {
            BUG();
        }
		
#if NAND_RANDOM_EN
		if (fgRandomizer == TRUE)
		{
			NFI_Randomizer_Encode(_u4CurPage, BlkPgCount, TRUE);
		}
#endif
		break;

    case NAND_CMD_ERASE1:
		
        NFI_CmdFun(NAND_CMD_ERASE1);
		NFI_AddrFun(page_addr, 0, (_u4PgAddrLen<<4));
        break;

    case NAND_CMD_RESET:
    case NAND_CMD_PAGEPROG:
    case NAND_CMD_STATUS:
    case NAND_CMD_ERASE2:
        NFI_CmdFun(command);
        break;

    default:
        printk(KERN_ERR "Unhandle command!\n");
        BUG();
        break;
    }
	
	NFI_WaitBusy();
}

//-----------------------------------------------------------------------------
/** mtk_nand_isbadpage()
 */
//-----------------------------------------------------------------------------
BOOL mtk_nand_isbadpage(int page)
{
    INT32 iRet;
    BOOL isEmpty = TRUE;
    UINT32 u4BitCnt, u4Offset, u4OobLastIdx, i;
    UINT32 u4SectOobSz, u4PgOobSz, u4EccSectSz;

    UINT8 *pu1Data  = (UINT8 *)rNFI_HWCFG->pu4Main_Buf;
    UINT8 *pu1Spare =  (UINT8 *)rNFI_HWCFG->pu4Spare_Buf;

#if NAND_DEBUG_PRINT
    printk(KERN_INFO "mtk_nand_isbadpage, page = 0x%X\n", page);
#endif

    // Cause mtk_nand_isbadblk() is not through mtk_nand_command().
    // Init variable _u4CurPage & _u4CurBitFlip
    _u4CurPage = (UINT32)page;
    _u4CurBitFlip = 0;
    
    u4SectOobSz = (_u4OOBSize * _u4SectorSize)>>9;
    u4PgOobSz = (_u4OOBSize * _u4PageSize)>>9;
    u4EccSectSz = _u4SectorSize + u4SectOobSz;

    // read spare data without ecc
    u4Offset = _u4PageSize;
	rNFI_HWCFG->u4Offset = u4Offset;
	
   if (isEmpty == TRUE)
   {
        if ((_prFlashDev->u4BBMode & NAND_BBMODE_ECCBIT) == 1)
        {
        		rNFI_HWCFG->rCtrlInfo.rECC_Type = NFIECC_NONE;
				rNFI_HWCFG->u4Len = _u4OOBSize;
				rNFI_HWCFG->u4PageIdx = _u4CurPage;
				rNFI_HWCFG->rCtrlInfo.fgAHBMode = FALSE;
				rNFI_HWCFG->rCtrlInfo.fgAutoFmt = FALSE;	
				rNFI_HWCFG->rCtrlInfo.fgInterrupt = FALSE;
				rNFI_HWCFG->rCtrlInfo.fgAutoRdDone = FALSE;
				iRet = NFI_PartialPageRead(rNFI_HWCFG);
				 pu1Spare =  (UINT8 *)rNFI_HWCFG->pu4Partial_Buf;
        }
        else
        {
        		rNFI_HWCFG->rCtrlInfo.rECC_Type = NFIECC_NONE;
        		rNFI_HWCFG->u4Len = u4PgOobSz;
				rNFI_HWCFG->u4PageIdx = _u4CurPage;
				rNFI_HWCFG->rCtrlInfo.fgAHBMode = FALSE;
				rNFI_HWCFG->rCtrlInfo.fgAutoFmt = FALSE;	
            	iRet = NFI_PartialPageRead(rNFI_HWCFG);
				pu1Spare =  (UINT8 *)rNFI_HWCFG->pu4Partial_Buf;
        }
    }
    if (!iRet)
    {
        goto HandleBad;
    }

    if ((_prFlashDev->u4BBMode & NAND_BBMODE_BBMARK) == 0x00000500)
    {
        u4BitCnt = _NANDHW_countbits(pu1Spare[5]) + _NANDHW_countbits(pu1Spare[6]);
    }
    else
    {
        u4BitCnt = _NANDHW_countbits(pu1Spare[0]) + _NANDHW_countbits(pu1Spare[1]);
    }
	
    switch (_prFlashDev->u4BBMode & NAND_BBMODE_ECCBIT)
    {
    case 0:
        BUG();
        break;

    case 1:
        if (u4BitCnt < 15)
        {
            goto HandleBad;
        }
        else
        {
            goto HandleGood;
        }

    default:
        u4OobLastIdx = u4PgOobSz - u4SectOobSz;
        for (i = 0; i < 6; i++)
        {
            u4BitCnt += _NANDHW_countbits(pu1Spare[u4OobLastIdx + i]);
        }

        if (u4BitCnt > 62)
        {
            goto HandleGood;
        }

        if (u4BitCnt < 2)
        {
            goto HandleBad;
        }

        break;
    }

    // read last sector data with ecc


	rNFI_HWCFG->rCtrlInfo.rECC_Type = NFIECC_SOFT;
	rNFI_HWCFG->u4Len = u4EccSectSz;
	rNFI_HWCFG->u4PageIdx = _u4CurPage;
	rNFI_HWCFG->rCtrlInfo.fgAHBMode = FALSE;
	rNFI_HWCFG->rCtrlInfo.fgAutoFmt = FALSE;	
	rNFI_HWCFG->rCtrlInfo.fgInterrupt = FALSE;
	rNFI_HWCFG->rCtrlInfo.fgAutoRdDone = FALSE;
    u4Offset = (_u4SectorNum - 1) * u4EccSectSz;
	rNFI_HWCFG->u4Offset = u4Offset;
	iRet = NFI_PartialPageRead(rNFI_HWCFG);
	pu1Data =  (UINT8 *)rNFI_HWCFG->pu4Partial_Buf;
    if (!iRet)
    {
        goto HandleBad;
    }

    //comment from xiaolei.li
	//add this case for DataExchange side effect: when exchange address is not 4 aligned,
	//spare[0] & spare[1] may be wrote not 0xFF in SDM partition. So, the original check method
	//below may take one perfect block as one bad block.
	//It takes more risk to modify DataExchange, and ROM code can not be modified.
	//I think it is enough if it is ok to do reading spare. So, patch here to avoid the problem explained above.
    #if defined(CONFIG_ARCH_MT5881) 
    if (_u4OOBSize%4 > 0)
        goto HandleGood;
    #endif
    
    u4OobLastIdx = u4EccSectSz - (u4SectOobSz*_u4SectorNum);
    if ((pu1Data[u4OobLastIdx] != 0xFF) || (pu1Data[u4OobLastIdx + 1] != 0xFF))
    {
        goto HandleBad;
    }

HandleGood:
    return FALSE;       // Good block

HandleBad:
    return TRUE;       // bad block
}

//-----------------------------------------------------------------------------
/** mtk_nand_isbadblk()
 */
//-----------------------------------------------------------------------------
static int mtk_nand_isbadblk(struct mtd_info *mtd, loff_t ofs, int getchip)
{
    struct nand_chip *chip = mtd->priv;
    int block = (int)(ofs >> chip->phys_erase_shift);

    // check the 1st page of the block.
    if (mtk_nand_isbadpage(block * _u4BlkPgCount))
    {
        return 1;
    }
    // check the 2nd page of the block.
    if (mtk_nand_isbadpage(block * _u4BlkPgCount + 1))
    {
        return 1;
    }

    // check the last page of the block if MLC.
    if (_u4OOBSize > 16)
    {
        if (mtk_nand_isbadpage(block * _u4BlkPgCount + _u4BlkPgCount -1))
        {
            return 1;
        }
    }

    return 0;
}

//-----------------------------------------------------------------------------
/** mtk_nand_enable_hwecc()
 */
//-----------------------------------------------------------------------------
static void mtk_nand_enable_hwecc(struct mtd_info *mtd, INT32 mode)
{
    #if NAND_DEBUG_PRINT
    printk(KERN_INFO "mtk_nand_enable_hwecc\n");
    #endif
}

//-----------------------------------------------------------------------------
/** mtk_nand_calculate_ecc()
 */
//-----------------------------------------------------------------------------
static int mtk_nand_calculate_ecc(struct mtd_info *mtd, const UINT8 *dat, UINT8 *ecc_code)
{
    #if NAND_DEBUG_PRINT
    printk(KERN_INFO "mtk_nand_calculate_ecc\n");
    #endif

    return 0;
}

//-----------------------------------------------------------------------------
/** mtk_nand_correct_data()
 */
//-----------------------------------------------------------------------------
static int mtk_nand_correct_data(struct mtd_info *mtd, UINT8 *dat, UINT8 *read_ecc, UINT8 *calc_ecc)
{
    #if NAND_DEBUG_PRINT
    printk(KERN_INFO "mtk_nand_correct_data\n");
    #endif

    return 0;
}

extern int parse_mtd_partitions(struct mtd_info *master, const char *const *types,
				struct mtd_partition **pparts,
				struct mtd_part_parser_data *data);

//-----------------------------------------------------------------------------
/** add_dynamic_parts()
 */
//-----------------------------------------------------------------------------
static inline int add_dynamic_parts(void)
{
    static const char *part_parsers[] = {"cmdlinepart", NULL};
    struct mtd_partition *parts;
    INT32 i, c, err;
    mtk_part_info_t *p;

    #if NAND_DEBUG_PRINT
    printk(KERN_INFO "add_dynamic_parts\n");
    #endif

    c = parse_mtd_partitions(mt53xx_mtd, part_parsers, &parts, 0);
    if (c <= 0)
    {
        return -EIO;
    }
    
    err = mtd_device_register(mt53xx_mtd, parts, c);
    if (err != 0)
    {
        return -EIO;
    }

    for (i = 0; i < c; i++)
    {
        p = &(rMTKPartInfo[i]);
        p->u4PgStart = parts[i].offset >> mt53xx_mtd->writesize_shift;
        p->u4PgEnd = p->u4PgStart + (parts[i].size >> mt53xx_mtd->writesize_shift) - 1;
        p->u4Encryped = (parts[i].mask_flags & MTD_ENCYPTED) ? 1 : 0;
        
        printk(KERN_INFO "partid=%02d, startpg=0x%08x, endpg=0x%08x, enc=%d\n",
            i, p->u4PgStart, p->u4PgEnd, p->u4Encryped);
    }

    return add_mtd_device(mt53xx_mtd);//mtd_device_register(mt53xx_mtd, NULL, 0);
}

//-----------------------------------------------------------------------------
/** mtk_nand_init() - Main initialization routine
 */
//-----------------------------------------------------------------------------
extern UINT32 u4ECCBits;

static INT32 __init mtk_nand_init(void)
{
    struct nand_chip *this;
	UINT32 u4EccCodeBit, u4ECCBits_hal;

    #if NAND_DEBUG_PRINT
    printk(KERN_INFO "mtk_nand_init\n");
    #endif
#ifndef CONFIG_MT53_FPGA
    sema_init(&nand_pin_share, 1);
    
	rNFI_HWCFG = kzalloc(sizeof(NFI_HWCFG_INFO_T), GFP_KERNEL);
    _NANDHW_Init();

    /* Allocate memory for MTD device structure and private data */
    mt53xx_mtd = kzalloc(sizeof(struct mtd_info) + sizeof(struct nand_chip), GFP_KERNEL);
    BUG_ON(!mt53xx_mtd);
    
    /* Get pointer to private data */
    this = (struct nand_chip *)(&mt53xx_mtd[1]);

    /* Link the private data with the MTD structure */
    mt53xx_mtd->priv = this;
    mt53xx_mtd->name = "mt53xx-nand";
    mt53xx_mtd->writesize = _u4PageSize;
    mt53xx_mtd->oobsize = (_u4OOBSize*_u4PageSize)>>9;   // Not use value in nand_base.c

    this->chip_delay = 0;
    this->badblockpos = 0;
    this->options |= NAND_SKIP_BBTSCAN;
    //this->options |= NAND_USE_FLASH_BBT;
    
    this->read_byte = mtk_nand_read_byte;
    this->select_chip = mtk_nand_select_chip;
    this->dev_ready = mtk_nand_dev_ready;
    this->cmdfunc = mtk_nand_command;
    this->write_buf = mtk_nand_write_buf;
    this->read_buf = mtk_nand_read_buf;
    //this->verify_buf = mtk_nand_verify_buf;
    this->block_bad = mtk_nand_isbadblk;
    this->block_markbad = mtk_nand_markbadblk;
    
    
    this->ecc.hwctl = mtk_nand_enable_hwecc;
    this->ecc.calculate = mtk_nand_calculate_ecc;
    this->ecc.correct = mtk_nand_correct_data;
    this->ecc.read_page = mtk_nand_read_page;
    this->ecc.write_page = mtk_nand_write_page;
    this->ecc.read_oob = mtk_nand_read_oob;
    this->ecc.write_oob = mtk_nand_write_oob;
    
    if (_fgDmaEn)
    {
        this->ecc.mode = NAND_ECC_HW;
    }
    else
    {
        this->ecc.mode = NAND_ECC_SOFT;
    }
    this->ecc.size = _u4PageSize;


		rNFI_HWCFG->rCtrlInfo.rECC_Type = NFIECC_NONE; 
		NFI_Get_Eccbit_CFG(rNFI_HWCFG);
		u4ECCBits_hal = rNFI_HWCFG->u4Eccbits_hal;
		printk("********************mtk_nand_init eccbits is %d**********************\n", u4ECCBits_hal);
#if defined(CONFIG_ARCH_MT5881)
			u4EccCodeBit = u4ECCBits_hal * NAND_NFIECC_13BIT;
#else
			u4EccCodeBit = u4ECCBits_hal * NAND_NFIECC_1BIT;
#endif
	this->ecc.strength = u4EccCodeBit;
    
    /* Scan to find existance of the device */
    if (nand_scan(mt53xx_mtd, 1))
    {
        kfree(mt53xx_mtd);
        mt53xx_mtd = NULL;
        
        printk(KERN_ERR "mtk_nand_init: nand scan failed!\n");
        return (-ENXIO);
    }
    
    if (add_dynamic_parts() < 0)
    {
        nand_release(mt53xx_mtd);
        kfree(mt53xx_mtd);
        mt53xx_mtd = NULL;
        
        printk(KERN_ERR "mtk_nand_init: no partitions defined!\n");
        return (-ENODEV);
    }
    
    mtdchar_buf = kmalloc(_prFlashDev->u4BlkSize, GFP_KERNEL);
    BUG_ON(!mtdchar_buf);
    sema_init(&mtdchar_share, 1);
#endif
    /* init completed */
    return 0;
}

//-----------------------------------------------------------------------------
/** mtk_nand_cleanup() - Clean up routine
 */
//-----------------------------------------------------------------------------
static void __exit mtk_nand_cleanup(void)
{
    nand_release(mt53xx_mtd);
    kfree(mt53xx_mtd);
    mt53xx_mtd = NULL;
    kfree(mtdchar_buf);
    mtdchar_buf = NULL;
}

void MTDNANDGetDmaBuf(UINT8 **ppu1DataBuf, UINT8 **ppu1OobBuf)
{
    *ppu1DataBuf = _pu1NandDataBuf;
    *ppu1OobBuf  = _pu1NandOobBuf;
}

void NANDPinShareLock(void)
{
    down(&nand_pin_share);
}

void NANDPinShareUnlock(void)
{
    up(&nand_pin_share);
}

void MTDNANDPinShareEntry(void)
{
    NANDPinShareLock();
	
    // Set nand/nor pinmux.
	 NFI_PMUXSetting();
}

void MTDNANDPinShareExit(void)
{
    NANDPinShareUnlock();
}

EXPORT_SYMBOL(MTDNANDGetDmaBuf);
EXPORT_SYMBOL(NANDPinShareLock);
EXPORT_SYMBOL(NANDPinShareUnlock);
EXPORT_SYMBOL(MTDNANDPinShareEntry);
EXPORT_SYMBOL(MTDNANDPinShareExit);

module_init(mtk_nand_init);
module_exit(mtk_nand_cleanup);

