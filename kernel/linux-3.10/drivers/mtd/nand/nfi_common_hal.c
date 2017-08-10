/*----------------------------------------------------------------------------*
 * Copyright Statement:                                                       *
 *                                                                            *
 *   This software/firmware and related documentation ("MediaTek Software")   *
 * are protected under international and related jurisdictions'copyright laws *
 * as unpublished works. The information contained herein is confidential and *
 * proprietary to MediaTek Inc. Without the prior written permission of       *
 * MediaTek Inc., any reproduction, modification, use or disclosure of        *
 * MediaTek Software, and information contained herein, in whole or in part,  *
 * shall be strictly prohibited.                                              *
 * MediaTek Inc. Copyright (C) 2010. All rights reserved.                     *
 *                                                                            *
 *   BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND     *
 * AGREES TO THE FOLLOWING:                                                   *
 *                                                                            *
 *   1)Any and all intellectual property rights (including without            *
 * limitation, patent, copyright, and trade secrets) in and to this           *
 * Software/firmware and related documentation ("MediaTek Software") shall    *
 * remain the exclusive property of MediaTek Inc. Any and all intellectual    *
 * property rights (including without limitation, patent, copyright, and      *
 * trade secrets) in and to any modifications and derivatives to MediaTek     *
 * Software, whoever made, shall also remain the exclusive property of        *
 * MediaTek Inc.  Nothing herein shall be construed as any transfer of any    *
 * title to any intellectual property right in MediaTek Software to Receiver. *
 *                                                                            *
 *   2)This MediaTek Software Receiver received from MediaTek Inc. and/or its *
 * representatives is provided to Receiver on an "AS IS" basis only.          *
 * MediaTek Inc. expressly disclaims all warranties, expressed or implied,    *
 * including but not limited to any implied warranties of merchantability,    *
 * non-infringement and fitness for a particular purpose and any warranties   *
 * arising out of course of performance, course of dealing or usage of trade. *
 * MediaTek Inc. does not provide any warranty whatsoever with respect to the *
 * software of any third party which may be used by, incorporated in, or      *
 * supplied with the MediaTek Software, and Receiver agrees to look only to   *
 * such third parties for any warranty claim relating thereto.  Receiver      *
 * expressly acknowledges that it is Receiver's sole responsibility to obtain *
 * from any third party all proper licenses contained in or delivered with    *
 * MediaTek Software.  MediaTek is not responsible for any MediaTek Software  *
 * releases made to Receiver's specifications or to conform to a particular   *
 * standard or open forum.                                                    *
 *                                                                            *
 *   3)Receiver further acknowledge that Receiver may, either presently       *
 * and/or in the future, instruct MediaTek Inc. to assist it in the           *
 * development and the implementation, in accordance with Receiver's designs, *
 * of certain softwares relating to Receiver's product(s) (the "Services").   *
 * Except as may be otherwise agreed to in writing, no warranties of any      *
 * kind, whether express or implied, are given by MediaTek Inc. with respect  *
 * to the Services provided, and the Services are provided on an "AS IS"      *
 * basis. Receiver further acknowledges that the Services may contain errors  *
 * that testing is important and it is solely responsible for fully testing   *
 * the Services and/or derivatives thereof before they are used, sublicensed  *
 * or distributed. Should there be any third party action brought against     *
 * MediaTek Inc. arising out of or relating to the Services, Receiver agree   *
 * to fully indemnify and hold MediaTek Inc. harmless.  If the parties        *
 * mutually agree to enter into or continue a business relationship or other  *
 * arrangement, the terms and conditions set forth herein shall remain        *
 * effective and, unless explicitly stated otherwise, shall prevail in the    *
 * event of a conflict in the terms in any agreements entered into between    *
 * the parties.                                                               *
 *                                                                            *
 *   4)Receiver's sole and exclusive remedy and MediaTek Inc.'s entire and    *
 * cumulative liability with respect to MediaTek Software released hereunder  *
 * will be, at MediaTek Inc.'s sole discretion, to replace or revise the      *
 * MediaTek Software at issue.                                                *
 *                                                                            *
 *   5)The transaction contemplated hereunder shall be construed in           *
 * accordance with the laws of Singapore, excluding its conflict of laws      *
 * principles.  Any disputes, controversies or claims arising thereof and     *
 * related thereto shall be settled via arbitration in Singapore, under the   *
 * then current rules of the International Chamber of Commerce (ICC).  The    *
 * arbitration shall be conducted in English. The awards of the arbitration   *
 * shall be final and binding upon both parties and shall be entered and      *
 * enforceable in any court of competent jurisdiction.                        *
 *---------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------
 *
 * $Author: dtvbm11 $
 * $Date: 2015/12/22 $
 * $RCSfile: nfi_common_hal.c,v $
 * $Revision: #1 $
 *
 *---------------------------------------------------------------------------*/
#include "nfi_common_reg.h"
#include "nfi_common_hal.h"
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
#include <x_typedef.h>
#include <linux/io.h>
#include <mach/hardware.h>

#include <linux/string.h>


//-------------------------------->include type def .h later  xiaolei

//xiaolei : add unified return status enum later

//naming rule
//static function: _NFI_xxxxx()
//external used function: NFI_xxxxx()

#define ASSERT(e) \
        if(!(e)){ \
          while(1); \
        }

#define IO_R32(offset)          (*((volatile UINT32*)(offset)))
#define IO_W32(offset, _val_)  (*((volatile UINT32*)(offset)) = (_val_))

static UINT32 u4ECCBits = 0;
static UINT32 u4CurBitflip = 0;
static BOOL fgEmptyPage = FALSE;
static struct completion comp;
static UINT32 mtd_nand_isr_en = 1;



static UINT16 RANDOM_SEED[256]=
{
    //for page 0~127
    0x576A, 0x05E8, 0x629D, 0x45A3, 0x649C, 0x4BF0, 0x2342, 0x272E,
    0x7358, 0x4FF3, 0x73EC, 0x5F70, 0x7A60, 0x1AD8, 0x3472, 0x3612,
    0x224F, 0x0454, 0x030E, 0x70A5, 0x7809, 0x2521, 0x484F, 0x5A2D,
    0x492A, 0x043D, 0x7F61, 0x3969, 0x517A, 0x3B42, 0x769D, 0x0647,
    0x7E2A, 0x1383, 0x49D9, 0x07B8, 0x2578, 0x4EEC, 0x4423, 0x352F,
    0x5B22, 0x72B9, 0x367B, 0x24B6, 0x7E8E, 0x2318, 0x6BD0, 0x5519,
    0x1783, 0x18A7, 0x7B6E, 0x7602, 0x4B7F, 0x3648, 0x2C53, 0x6B99,
    0x0C23, 0x67CF, 0x7E0E, 0x4D8C, 0x5079, 0x209D, 0x244A, 0x747B,
    0x350B, 0x0E4D, 0x7004, 0x6AC3, 0x7F3E, 0x21F5, 0x7A15, 0x2379,
    0x1517, 0x1ABA, 0x4E77, 0x15A1, 0x04FA, 0x2D61, 0x253A, 0x1302,
    0x1F63, 0x5AB3, 0x049A, 0x5AE8, 0x1CD7, 0x4A00, 0x30C8, 0x3247,
    0x729C, 0x5034, 0x2B0E, 0x57F2, 0x00E4, 0x575B, 0x6192, 0x38F8,
    0x2F6A, 0x0C14, 0x45FC, 0x41DF, 0x38DA, 0x7AE1, 0x7322, 0x62DF,
    0x5E39, 0x0E64, 0x6D85, 0x5951, 0x5937, 0x6281, 0x33A1, 0x6A32,
    0x3A5A, 0x2BAC, 0x743A, 0x5E74, 0x3B2E, 0x7EC7, 0x4FD2, 0x5D28,
    0x751F, 0x3EF8, 0x39B1, 0x4E49, 0x746B, 0x6EF6, 0x44BE, 0x6DB7,
    //for page 128~255
    0x576A, 0x05E8, 0x629D, 0x45A3, 0x649C, 0x4BF0, 0x2342, 0x272E,
    0x7358, 0x4FF3, 0x73EC, 0x5F70, 0x7A60, 0x1AD8, 0x3472, 0x3612,
    0x224F, 0x0454, 0x030E, 0x70A5, 0x7809, 0x2521, 0x484F, 0x5A2D,
    0x492A, 0x043D, 0x7F61, 0x3969, 0x517A, 0x3B42, 0x769D, 0x0647,
    0x7E2A, 0x1383, 0x49D9, 0x07B8, 0x2578, 0x4EEC, 0x4423, 0x352F,
    0x5B22, 0x72B9, 0x367B, 0x24B6, 0x7E8E, 0x2318, 0x6BD0, 0x5519,
    0x1783, 0x18A7, 0x7B6E, 0x7602, 0x4B7F, 0x3648, 0x2C53, 0x6B99,
    0x0C23, 0x67CF, 0x7E0E, 0x4D8C, 0x5079, 0x209D, 0x244A, 0x747B,
    0x350B, 0x0E4D, 0x7004, 0x6AC3, 0x7F3E, 0x21F5, 0x7A15, 0x2379,
    0x1517, 0x1ABA, 0x4E77, 0x15A1, 0x04FA, 0x2D61, 0x253A, 0x1302,
    0x1F63, 0x5AB3, 0x049A, 0x5AE8, 0x1CD7, 0x4A00, 0x30C8, 0x3247,
    0x729C, 0x5034, 0x2B0E, 0x57F2, 0x00E4, 0x575B, 0x6192, 0x38F8,
    0x2F6A, 0x0C14, 0x45FC, 0x41DF, 0x38DA, 0x7AE1, 0x7322, 0x62DF,
    0x5E39, 0x0E64, 0x6D85, 0x5951, 0x5937, 0x6281, 0x33A1, 0x6A32,
    0x3A5A, 0x2BAC, 0x743A, 0x5E74, 0x3B2E, 0x7EC7, 0x4FD2, 0x5D28,
    0x751F, 0x3EF8, 0x39B1, 0x4E49, 0x746B, 0x6EF6, 0x44BE, 0x6DB7
};

//-----------------------------------------------------------------------------
//                                               static function define
//-----------------------------------------------------------------------------

//define static read/write function to make sure that only nfi hal can access nfi reg
//-----------------------------------------------------------------------------
// NFI_WRITE32()
//
//-----------------------------------------------------------------------------
static void NFI_WRITE32(unsigned int addr, unsigned int data)
{
	*((volatile unsigned int *)(NFI_BASE_ADD + addr)) = (unsigned int)(data);
}
//-----------------------------------------------------------------------------
// NFI_READ32()
//
//-----------------------------------------------------------------------------
static unsigned int NFI_READ32(unsigned int addr)
{
	return *((volatile unsigned int *)(NFI_BASE_ADD + addr));
}
//-----------------------------------------------------------------------------
// NFI_WRITE32()
//
//-----------------------------------------------------------------------------
#if 0
static void NFI_WRITE16(unsigned int addr, unsigned int data)
{
	*((volatile unsigned short *)(NFI_BASE_ADD + addr)) = (unsigned short)(data);
}
#endif

//-----------------------------------------------------------------------------
// NFI_READ32()
//
//-----------------------------------------------------------------------------
static unsigned short NFI_READ16(unsigned int addr)
{
	return *((volatile unsigned short *)(NFI_BASE_ADD + addr));
}
//-----------------------------------------------------------------------------
// NFIECC_WRITE32()
//
//-----------------------------------------------------------------------------
static void NFIECC_WRITE32(unsigned int addr, unsigned int data)
{
	*((volatile unsigned int *)(NFIECC_BASE_ADD + addr)) = (unsigned int)(data);
}
//-----------------------------------------------------------------------------
// NFIECC_READ32()
//
//-----------------------------------------------------------------------------
static unsigned int NFIECC_READ32(unsigned int addr)
{
	return *((volatile unsigned int *)(NFIECC_BASE_ADD + addr));
}



//-----------------------------------------------------------------------------
// NFI_INT_SET()
//
//-----------------------------------------------------------------------------

UINT32 volatile mtd_nand_request_waiting = 1;

static irqreturn_t _NFI_mtd_nand_isr(int irq, void *dev_id)
{
    u32 intr_status = NFI_READ32(NFI_INTR);
    if(intr_status == 0)
    {
        printk(KERN_CRIT "curious mtd_nand_irq!\n");
    }
		mtd_nand_request_waiting = 1;
		
    complete(&comp);
    return IRQ_HANDLED;
}


int NFI_mtd_nand_regisr(void)
{
    NFI_READ32(NFI_INTR);  // Clear interrupt, read clear

    if (request_irq(VECTOR_NAND, _NFI_mtd_nand_isr, 0, "NAND DMA", NULL) != 0)
    {
        printk(KERN_ERR "Request NAND IRQ fail!\n");
        return -1;
    }

    NFI_READ32(NFI_INTR);  // Clear interrupt, read clear
    disable_irq(VECTOR_NAND);
    return 0;
}


static void _NFI_mtd_nand_setisr(UINT32 u4Mask)
{
    if (mtd_nand_isr_en)
    {
        enable_irq(VECTOR_NAND);
        NFI_WRITE32(NFI_INTR_EN, u4Mask);

        init_completion(&comp);
    }
}

static void _NFI_mtd_nand_waitisr(UINT32 u4Mask)
{
    if (mtd_nand_isr_en)
    {
        wait_for_completion(&comp);
        mtd_nand_request_waiting = 0;

        NFI_WRITE32(NFI_INTR_EN, 0);
        disable_irq(VECTOR_NAND);
    }
}

void mtd_nand_setisr(UINT32 u4Mask)
{
  _NFI_mtd_nand_setisr(u4Mask);
}

void mtd_nand_waitisr(UINT32 u4Mask)
{
    _NFI_mtd_nand_waitisr(u4Mask);
}

EXPORT_SYMBOL(mtd_nand_setisr);
EXPORT_SYMBOL(mtd_nand_waitisr);


//-----------------------------------------------------------------------------
// _NFI_Random_Mizer_Encode()
// UINT32 u4PageIdx : page index; UINT32 u4BlkPgCnt : page count of one block; BOOL fgRandomOn : randomizer on/off
//-----------------------------------------------------------------------------
static void _NFI_Randomizer_Encode(UINT32 u4PageIdx, UINT32 u4BlkPgCnt, BOOL fgRandomOn)
{
    UINT32 u4Val;

    if (fgRandomOn)
    {
        NFI_WRITE32(NFI_RANDOM_CFG, 0);
		if(u4BlkPgCnt <= (sizeof(RANDOM_SEED) / sizeof(UINT16)))
	        u4Val = NFI_ENCODING_RANDON_SEED(RANDOM_SEED[u4PageIdx % u4BlkPgCnt]) | NFI_ENCODING_RANDON_EN;
		else
			u4Val = NFI_ENCODING_RANDON_SEED(RANDOM_SEED[u4PageIdx % (sizeof(RANDOM_SEED) / sizeof(UINT16))]) | NFI_ENCODING_RANDON_EN;
		NFI_WRITE32(NFI_RANDOM_CFG, u4Val);
    }
    else
    {
        NFI_WRITE32(NFI_RANDOM_CFG, 0);
    }
}

void NFI_Randomizer_Encode(UINT32 u4PageIdx, UINT32 u4BlkPgCnt, BOOL fgRandomOn)
{
	_NFI_Randomizer_Encode(u4PageIdx, u4BlkPgCnt, fgRandomOn);
}

//-----------------------------------------------------------------------------
// _NFI_Random_Mizer_Decode()
// UINT32 u4PageIdx : page index; UINT32 u4BlkPgCnt : page count of one block; BOOL fgRandomOn : randomizer on/off
//-----------------------------------------------------------------------------
static void _NFI_Randomizer_Decode(UINT32 u4PageIdx, UINT32 u4BlkPgCnt, BOOL fgRandomOn)
{
    UINT32 u4Val;

    if (fgRandomOn)
    {
        NFI_WRITE32(NFI_RANDOM_CFG, 0);
		if(u4BlkPgCnt <= (sizeof(RANDOM_SEED) / sizeof(UINT16)))
        	u4Val = NFI_DECODING_RANDON_SEED(RANDOM_SEED[u4PageIdx % u4BlkPgCnt]) | NFI_DECODING_RANDON_EN;
		else
			u4Val = NFI_DECODING_RANDON_SEED(RANDOM_SEED[u4PageIdx % (sizeof(RANDOM_SEED) / sizeof(UINT16))]) | NFI_DECODING_RANDON_EN;
        NFI_WRITE32(NFI_RANDOM_CFG, u4Val);
    }
    else
    {
        NFI_WRITE32(NFI_RANDOM_CFG, 0);
    }
}

void NFI_Randomizer_Decode(UINT32 u4PageIdx, UINT32 u4BlkPgCnt, BOOL fgRandomOn)
{
	_NFI_Randomizer_Decode(u4PageIdx, u4BlkPgCnt, fgRandomOn);
}

//-----------------------------------------------------------------------------
// _NFI_WaitBusy()
//
//-----------------------------------------------------------------------------
static void _NFI_WaitBusy(void)
{
	UINT32 u4Val;

	do
	{
		u4Val = NFI_READ32(NFI_STA);
    	u4Val &= NFI_STA_NFI_BUSY;  // bit8=1: ready to busy
	}while(u4Val);
}

void NFI_WaitBusy(void)
{
	_NFI_WaitBusy();
}


//-----------------------------------------------------------------------------
// _NFIECC_WaitBusy()
//
//-----------------------------------------------------------------------------
static void _NFIECC_WaitBusy(void)
{
	UINT32 u4Val;

    do
    {
        u4Val = NFIECC_READ32(NFIECC_ENCIDLE);
    }
    while (u4Val == 0);

    do
    {
        u4Val = NFIECC_READ32(NFIECC_DECIDLE);
    }
    while (u4Val == 0);
}

void NFIECC_WaitBusy(void)
{
	_NFIECC_WaitBusy();
}

//-----------------------------------------------------------------------------
/** _NFI_WaitEccDone() must wait done
 */
//-----------------------------------------------------------------------------
static void _NFI_WaitEccDone(UINT32 u4SectIdx)
{
    UINT32 u4Val;

    do
    {
        u4Val = NFIECC_READ32(NFIECC_DECDONE);
        u4Val = (u4Val >> u4SectIdx) & 0x1;
    }
    while (u4Val == 0);
}

//-----------------------------------------------------------------------------
// _NFI_WaitRead()
// UINT32 u4WaitNum : read number
//-----------------------------------------------------------------------------
static void _NFI_WaitRead(UINT32 u4WaitNum)
{
    UINT32 u4Val;

    do
    {
        u4Val = NFI_READ32(NFI_FIFOSTA);
        u4Val &= NFI_FIFOSTA_RD_REMAIN;
    }
    while (u4Val < u4WaitNum);
}

//-----------------------------------------------------------------------------
// _NANDHW_WaitWrite()
//
//-----------------------------------------------------------------------------
static void _NFI_WaitWrite(void)
{
    UINT32 u4Val;

    do
    {
        u4Val = NFI_READ32(NFI_FIFOSTA);
        u4Val &= NFI_FIFOSTA_WT_EMPTY;
    }
    while (u4Val == 0);
}

//-----------------------------------------------------------------------------
// _NFI_Reset(void)
//
//-----------------------------------------------------------------------------
static void _NFI_Reset(void)
{
    NFI_WRITE32(NFI_CNFG, 0);
    NFI_WRITE32(NFI_CON, NFI_CON_NFI_RST | NFI_CON_FIFO_FLUSH);
    _NFI_WaitBusy();
}


//-----------------------------------------------------------------------------
// _NFI_Reset(void)
//
//-----------------------------------------------------------------------------
static void _NFIECC_Reset(void)
{
    NFIECC_WRITE32(NFIECC_ENCON, 0);
    NFIECC_WRITE32(NFIECC_DECCON, 0);
    _NFIECC_WaitBusy();
}

void NFIECC_Reset(void)
{
	_NFIECC_Reset();
}

//-----------------------------------------------------------------------------
// _NFIECC_Trig(UINT32 u4OpMode)
//
//-----------------------------------------------------------------------------
static void _NFIECC_Trig(UINT32 u4OpMode)
{
    switch (u4OpMode)
    {
    case NFI_CNFG_OP_READ:
        NFIECC_WRITE32(NFIECC_DECCON, 1);
        break;

    case NFI_CNFG_OP_PROGRAM:
        NFIECC_WRITE32(NFIECC_ENCON, 1);
        break;

    default:
        break;
    }
}


//-----------------------------------------------------------------------------
// _NFIECC_Cfg(NFIECC_TYPE_T rECCType)
//
//-----------------------------------------------------------------------------
static void _NFIECC_Cfg(NFIECC_TYPE_T rECCType, UINT32 u4OOBSize, UINT32 u4PageSize)
{
    UINT32 u4EncMsg = 0, u4DecMsg = 0;
    if (u4OOBSize == 16)
    {
        if (u4PageSize == 512)
        {
            u4ECCBits = 4;
            u4EncMsg = NFIECC_ENCCNFG_ENC_TNUM_4 | NFIECC_ENCCNFG_ENC_MS_520;
            u4DecMsg = NFIECC_DECCNFG_DEC_TNUM_4 | NFIECC_DECCNFG_DEC_CS_520_4;
        }
        else
        {
            u4ECCBits = 12;
            u4EncMsg = NFIECC_ENCCNFG_ENC_TNUM_12 | NFIECC_ENCCNFG_ENC_MS_1032;
            u4DecMsg = NFIECC_DECCNFG_DEC_TNUM_12 | NFIECC_DECCNFG_DEC_CS_1032_12;
        }
    }
    else if (u4OOBSize == 26)
    {
        if (u4PageSize == 512)
        {
            u4ECCBits = 10;
            u4EncMsg = NFIECC_ENCCNFG_ENC_TNUM_10 | NFIECC_ENCCNFG_ENC_MS_520;
            u4DecMsg = NFIECC_DECCNFG_DEC_TNUM_10 | NFIECC_DECCNFG_DEC_CS_520_10;
        }
        else
        {
            u4ECCBits = 24;
            u4EncMsg = NFIECC_ENCCNFG_ENC_TNUM_24 | NFIECC_ENCCNFG_ENC_MS_1032;
            u4DecMsg = NFIECC_DECCNFG_DEC_TNUM_24 | NFIECC_DECCNFG_DEC_CS_1032_24;
        }
    }
    else if (u4OOBSize == 32)
    {
        u4ECCBits = 32;
        u4EncMsg = NFIECC_ENCCNFG_ENC_TNUM_32 | NFIECC_ENCCNFG_ENC_MS_1032;
        u4DecMsg = NFIECC_DECCNFG_DEC_TNUM_32 | NFIECC_DECCNFG_DEC_CS_1032_32;
    }
    else if (u4OOBSize == 36)
    {
        u4ECCBits = 36;
        u4EncMsg = NFIECC_ENCCNFG_ENC_TNUM_36 | NFIECC_ENCCNFG_ENC_MS_1032;
        u4DecMsg = NFIECC_DECCNFG_DEC_TNUM_36 | NFIECC_DECCNFG_DEC_CS_1032_36;
    }
    else if (u4OOBSize == 40)
    {
        u4ECCBits = 40;
        u4EncMsg = NFIECC_ENCCNFG_ENC_TNUM_40 | NFIECC_ENCCNFG_ENC_MS_1032;
        u4DecMsg = NFIECC_DECCNFG_DEC_TNUM_40 | NFIECC_DECCNFG_DEC_CS_1032_40;
    }
    else if (u4OOBSize == 64)
    {
        u4ECCBits = 60;
        u4EncMsg = NFIECC_ENCCNFG_ENC_TNUM_60 | NFIECC_ENCCNFG_ENC_MS_1032;
        u4DecMsg = NFIECC_DECCNFG_DEC_TNUM_60 | NFIECC_DECCNFG_DEC_CS_1032_60;
    }

    u4EncMsg |= NFIECC_ENCCNFG_ENC_NFI_MODE;
    u4DecMsg |= NFIECC_DECCNFG_DEC_CS_EMPTY_EN | NFIECC_DECCNFG_DEC_NFI_MODE;

    if (rECCType == NFIECC_HARD)
    {
        u4DecMsg |= NFIECC_DECCNFG_DEC_CON_AUTO;
    }
    else if (rECCType == NFIECC_SOFT)
    {
        u4DecMsg |= NFIECC_DECCNFG_DEC_CON_SOFT;
    }
    else if (rECCType == NFIECC_NONE)
    {
        u4DecMsg |= NFIECC_DECCNFG_DEC_CON_NONE;
    }
    else
    {
        ASSERT(0);
    }

    NFIECC_WRITE32(NFIECC_ENCCNFG, u4EncMsg);
    NFIECC_WRITE32(NFIECC_DECCNFG, u4DecMsg);
}


void NFI_Get_Eccbit_CFG(NFI_HWCFG_INFO_T *prCNFG_Setting)
{
	_NFIECC_Cfg(prCNFG_Setting->rCtrlInfo.rECC_Type, prCNFG_Setting->rDevInfo.u4OOBSz, prCNFG_Setting->rDevInfo.u4PgSz);
	prCNFG_Setting->u4Eccbits_hal = u4ECCBits;
}

//-----------------------------------------------------------------------------
/** _NANDHW_NFI_ReadFifo()
 */
//-----------------------------------------------------------------------------
static void _NFI_ReadFifo(UINT8 *pu1Buf, UINT32 u4Len)
{
    UINT32 i;
    UINT32 *pu4Buf = (UINT32 *)pu1Buf;
    ASSERT(pu4Buf != NULL);
    {
        for (i = 0; i < (u4Len>>2); i++)
        {
            _NFI_WaitRead(4);
            pu4Buf[i] = NFI_READ32(NFI_DATAR);
        }
    }
}

//-----------------------------------------------------------------------------
// _NFI_WriteFifo(UINT8 *pu1Buf, UINT32 u4Len)
//
//-----------------------------------------------------------------------------
static void _NFI_WriteFifo(UINT8 *pu1Buf, UINT32 u4Len)
{
    UINT32 i;
    UINT32 *pu4Buf = (UINT32 *)pu1Buf;
    ASSERT(pu4Buf != NULL);
	for (i = 0; i < (u4Len >> 2); i++)
	{
	    _NFI_WaitWrite();
	    NFI_WRITE32(NFI_DATAW, pu4Buf[i]);
	}
}

//-----------------------------------------------------------------------------
// _NFI_WriteFifo(UINT8 *pu1Buf, UINT32 u4Len)
//
//-----------------------------------------------------------------------------
static void _NFIECC_GetECCParity(UINT32 *pu4EccCode)
{
    UINT32 u4Val, i=0;
    UINT32 u4EccCodeBit, u4EccCodeDWord, u4EccModBit;

    // the redundant bit of parity bit must be 1 for rom code bug. -- 20110328 mtk40109
    u4EccCodeBit = u4ECCBits * NFIECC_1BIT;  //to do later xiaolei
    u4EccModBit = u4EccCodeBit % 32;
    u4EccCodeDWord = u4EccModBit ? ((u4EccCodeBit>>5) + 1) : (u4EccCodeBit>>5);
    ASSERT(u4EccCodeDWord <= NFI_ENCPAR_NUM);

    do
    {
        u4Val = NFIECC_READ32(NFIECC_ENCSTA);
        u4Val &= NFIECC_ENCSTA_FSM_MASK;
    }
    while (u4Val != NFIECC_ENCSTA_FSM_PAROUT);

    for (i = 0; i < u4EccCodeDWord; i++)
    {
        pu4EccCode[i] = NFIECC_READ32(NFIECC_ENCPAR0 + (i<<2));
    }

    for (i = u4EccCodeDWord; i < NFI_ENCPAR_NUM; i++)
    {
        pu4EccCode[i] = 0xFFFFFFFF;
    }

    if (u4EccModBit)
    {
        pu4EccCode[u4EccCodeDWord - 1] |= (~(UINT32)((1<<u4EccModBit) - 1));
    }
}

//-----------------------------------------------------------------------------
// _NFI_WaitSectorCnt(UINT32 u4SectorNum)
//
//-----------------------------------------------------------------------------
static void _NFI_WaitSectorCnt(UINT32 u4SectorNum)
{
    UINT32 u4Val, u4Cnt;

    do
    {
        u4Val = NFI_READ32(NFI_ADDRCNTR);
        u4Val &= 0xF800;
        u4Cnt = NFI_ADDRCNTR_SEC_CNTR(u4SectorNum);
    }
    while (u4Val != u4Cnt);
}

//-----------------------------------------------------------------------------
// _NANDHW_countbits()
//
//-----------------------------------------------------------------------------
static UINT32 _NFI_countbits(UINT32 u4byte)
{
    UINT32 u4res = 0;

    for (; u4byte; u4byte >>= 1)
    {
        u4res += (u4byte & 0x01);
    }

    return u4res;
}

//-----------------------------------------------------------------------------
// _NFIECC_CorrectEmptySector()
///
//-----------------------------------------------------------------------------
static BOOL _NFIECC_CorrectEmptySector(UINT8 *buf, UINT8 *spare, UINT32 u4SectIdx, NFI_HWCFG_INFO_T *prECC_Correct_Info)
{
    UINT32 i, u4Val, u4SectOobSz, u4bitflip = 0;
    UINT8 *pu1Data, *pu1Spare;
    UINT32 *pu4Data, *pu4Spare;

    u4Val = NFIECC_READ32(NFIECC_DECFER);
    if ((u4Val & (1 << u4SectIdx)) == 0)
    {
        return FALSE; //NAND_SUCC;
    }

    u4SectOobSz = (prECC_Correct_Info->rDevInfo.u4OOBSz* prECC_Correct_Info->u4SectorSize)>>9;

    pu1Data = buf + u4SectIdx * prECC_Correct_Info->u4SectorSize;
    pu1Spare = spare + u4SectIdx * u4SectOobSz;
    pu4Data = (UINT32 *)pu1Data;
    pu4Spare = (UINT32 *)pu1Spare;

    for (i = 0; i < prECC_Correct_Info->u4SectorSize/4; i++)
    {
        u4bitflip += (32 - _NFI_countbits(pu4Data[i]));
    }

    for (i = 0; i < prECC_Correct_Info->u4FDMNum/4; i++)
    {
        u4bitflip += (32 - _NFI_countbits(pu4Spare[i]));
    }

    if (u4bitflip <= u4ECCBits)
    {
        memset(pu1Data, 0xFF, prECC_Correct_Info->u4SectorSize);
        memset(pu1Spare, 0xFF, prECC_Correct_Info->u4FDMNum);

        if (u4bitflip > (u4ECCBits/2))
        {
            u4CurBitflip += u4bitflip;
        }

        //LOG(0, "Correctable ECC found in empty page 0x%X, sector %d: %d bit-flip\n",
        //    _u4CurPage, u4SectIdx, u4bitflip);
        return FALSE;//NAND_SUCC;
    }
    else if (prECC_Correct_Info->rCtrlInfo.fgRandomizer && (NFI_READ32(NFI_FIFOSTA)&0x00010000) )
    {
        fgEmptyPage = TRUE;
        memset(pu1Data, 0xFF, prECC_Correct_Info->u4SectorSize);
        memset(pu1Spare, 0xFF, prECC_Correct_Info->u4FDMNum);
        //if (NAND_READ32(NAND_NFI_FIFOSTA)&0x00FC0000)
         //   LOG(0, "empty page check = 0x%08X, bit flip = 0x%08X \n", NAND_READ32(NAND_NFI_FIFOSTA), (NAND_READ32(NAND_NFI_FIFOSTA)&&0x00FC0000)>>18);
        return FALSE;
    }
    else
    {
        //printk(KERN_ERR "Un-Correctable ECC found in page 0x%X, sector %d, bitflip=%d\n", _u4CurPage, u4SectIdx, u4bitflip);
        fgEmptyPage = FALSE;
        UNUSED(fgEmptyPage);
        return TRUE;
    }
}

//-----------------------------------------------------------------------------
// _NFIECC_GetErrNum()
//
//-----------------------------------------------------------------------------
static INT32 _NFIECC_GetErrNum(UINT32 u4SectIdx, UINT32 *pu4ErrNum)
{
    UINT32 u4Val;
    ASSERT(pu4ErrNum != NULL);
    if (u4SectIdx < 4)
    {
        u4Val = NFI_READ32(NFIECC_DECENUM0);
        *pu4ErrNum = (u4Val >> (u4SectIdx*8)) & 0x3F;
    }
    else if ((u4SectIdx >= 4) && (u4SectIdx < 8))
    {
        u4Val = NFI_READ32(NFIECC_DECENUM1);
        *pu4ErrNum = (u4Val >> ((u4SectIdx - 4)*8)) & 0x3F;
    }
    else if ((u4SectIdx >= 8) && (u4SectIdx < 12))
    {
        u4Val = NFI_READ32(NFIECC_DECENUM2);
        *pu4ErrNum = (u4Val >> ((u4SectIdx - 8)*8)) & 0x3F;
    }
    else
    {
        u4Val = NFI_READ32(NFIECC_DECENUM3);
        *pu4ErrNum = (u4Val >> ((u4SectIdx - 12)*8)) & 0x3F;
    }
    if (*pu4ErrNum == 0x3F)
    {
        return TRUE;//NAND_ECC_ERROR;
    }
    else
    {
        return FALSE;//NAND_SUCC;
    }

}

//-----------------------------------------------------------------------------
// _NFIECC_CheckEcc()
//
//-----------------------------------------------------------------------------
static BOOL _NFIECC_CheckEcc(UINT8 *buf, UINT8 *spare, NFI_HWCFG_INFO_T *prECC_Correct_Info)
{
    UINT32 u4SectIdx, u4ErrNum;
    BOOL fgRet = FALSE;

    for (u4SectIdx = 0; u4SectIdx < prECC_Correct_Info->u4SectorNum; u4SectIdx++)
    {
        if (_NFIECC_GetErrNum(u4SectIdx, &u4ErrNum))
        {
            fgRet |= _NFIECC_CorrectEmptySector(buf, spare, u4SectIdx, prECC_Correct_Info);
        }
        else if (u4ErrNum != 0)
        {
            if (u4ErrNum > (u4ECCBits/2))
            {
                u4CurBitflip += u4ErrNum;
            }
            //LOG(3, "Correctable ECC found in page 0x%X, sector %d: %d bit-flip\n",  _u4CurPage, u4SectIdx, u4ErrNum);
        }
    }

    return fgRet;
}

//-----------------------------------------------------------------------------
// _NFIECC_CheckEcc()
//
//-----------------------------------------------------------------------------
static BOOL _NFIECC_CorrectEcc(NFI_HWCFG_INFO_T *prECC_Info, UINT32 u4SectIdx)
{
    INT32 i4Ret;
    UINT32 i, u4Val, u4SpareOff, u4SectOobSz;
    UINT32 u4ErrNum, u4ErrFound, u4ErrByte, u4ErrBit;

	UINT8 *pu1Buf = (UINT8 *)prECC_Info->pu4Main_Buf;
	UINT8 *pu1Spare = (UINT8 *)prECC_Info->pu4Spare_Buf;
	
    ASSERT(pu1Buf != NULL);
    ASSERT(pu1Spare != NULL);

    _NFI_WaitEccDone(u4SectIdx); // must wait done

    if (_NFIECC_GetErrNum(u4SectIdx, &u4ErrNum))
    {
        i4Ret = _NFIECC_CorrectEmptySector(pu1Buf, pu1Spare, u4SectIdx, prECC_Info);
        return i4Ret;
    }

    u4SectOobSz = (prECC_Info->rDevInfo.u4OOBSz * prECC_Info->u4SectorSize)>>9;
	
    u4ErrFound = NFI_READ32(NFIECC_DECFER);
	
    if ((u4ErrFound >> u4SectIdx) & 0x01)
    {
        for (i = 0; i < u4ErrNum; i++)
        {
            u4Val = (UINT32)NFI_READ16(NFIECC_DECEL0 + (i*2));
            u4ErrByte = u4Val >> 3;
            u4ErrBit = u4Val & 0x7;

            if (u4ErrByte < prECC_Info->u4SectorSize)
            {
                // Data area bit error.
                u4ErrByte += u4SectIdx * prECC_Info->u4SectorSize;
                pu1Buf[u4ErrByte] = pu1Buf[u4ErrByte] ^ (((UINT32)1)<<u4ErrBit);
            }
            else if (u4ErrByte < (u4SectOobSz + prECC_Info->u4SectorSize))
            {
                // Spare area bit error.
                u4SpareOff = u4ErrByte - prECC_Info->u4SectorSize + u4SectOobSz * u4SectIdx;
                pu1Spare[u4SpareOff] = pu1Spare[u4SpareOff] ^ (((UINT32)1)<<u4ErrBit);
            }
            else
            {
                return TRUE;//NAND_ECC_ERROR;
            }
        }
    }

    if (u4ErrNum > (u4ECCBits/2))
    {
        u4CurBitflip += u4ErrNum;
    }

    return FALSE;//NAND_SUCC;
}

//-----------------------------------------------------------------------------
// _NFI_AddrLatch(UINT32 u4RowAddr, UINT32 u4ColAddr, UINT32 u4ByteCount)
//
//-----------------------------------------------------------------------------
static void _NFI_AddrLatch(UINT32 u4RowAddr, UINT32 u4ColAddr, UINT32 u4ByteCount)
{
	NFI_WRITE32(NFI_COLADDR, u4ColAddr);
	NFI_WRITE32(NFI_ROWADDR, u4RowAddr);
	NFI_WRITE32(NFI_ADDRNOB, u4ByteCount);
	
	// wait for NFI core address mode done
    while(NFI_READ32(NFI_STA)  & NFI_STA_NFI_ADDR);
}

//-----------------------------------------------------------------------------
// _NFI_CmdLatch(UINT32 u4Cmd)
//
//-----------------------------------------------------------------------------
static void _NFI_CmdLatch(UINT32 u4Cmd)
{
    NFI_WRITE32(NFI_CMD, u4Cmd);

	// wait for NFI core command mode done
    while(NFI_READ32(NFI_STA)  & NFI_STA_NFI_ADDR);
}

//-----------------------------------------------------------------------------
// _NFI_SetIrq(UINT32 u4IrqMode)
//
//-----------------------------------------------------------------------------
#if 0
static void _NFI_SetIrq(UINT32 u4IrqMode)
{    
	NFI_READ32(NFI_INTR);	   // Clear interrupt
	NFI_WRITE32(NFI_INTR_EN, u4IrqMode);
}


static void _NFI_WaitIrq(UINT32 u4IrqMode)
{    
	//clear interrupt
	while(!(NFI_READ32(NFI_INTR) & u4IrqMode)); //xiaolei: add timeout later
	//disable interrupt
	NFI_WRITE32(NFI_INTR, 0);
}
#endif 

//-----------------------------------------------------------------------------
// _NFI_CNFGSetting(NFI_CTRL_INFO_T *prCNFG_Setting)
//
//-----------------------------------------------------------------------------
static void _NFI_CNFGSetting(NFI_HWCFG_INFO_T *prCNFG_Setting, UINT32 u4OpMode)
{
	UINT32 u4Val = 0;

	//set auto format if hw ecc or dma mode
	if (prCNFG_Setting->rCtrlInfo.rECC_Type == NFIECC_HARD)
	{
		u4Val |= NFI_CNFG_AUTO_FMT_EN | NFI_CNFG_HW_ECC_EN;
	}
	else if (prCNFG_Setting->rCtrlInfo.rECC_Type == NFIECC_SOFT)
	{
		u4Val |= NFI_CNFG_HW_ECC_EN;
	}

	if (prCNFG_Setting->rCtrlInfo.fgAHBMode)
	{
		u4Val |= (NFI_CNFG_AUTO_FMT_EN | NFI_CNFG_AHB_MODE);
	}

	if(prCNFG_Setting->rCtrlInfo.fgAutoFmt)
	{
		u4Val |= NFI_CNFG_AUTO_FMT_EN;
	}

	switch (u4OpMode)
	{
	case NFI_CNFG_OP_IDLE:
		NFI_WRITE32(NFI_CNFG, 0);
		return;

	case NFI_CNFG_OP_SINGLE:
	case NFI_CNFG_OP_READ:
		u4Val |= (NFI_CNFG_READ_MODE | u4OpMode);
		break;

	case NFI_CNFG_OP_PROGRAM:
	case NFI_CNFG_OP_ERASE:
	case NFI_CNFG_OP_RESET:
	case NFI_CNFG_OP_CUSTOM:
		u4Val |= u4OpMode;
		break;

	default:
		break;
	}

	if(prCNFG_Setting->rCtrlInfo.fgByteRW)
	{
		u4Val |= NFI_CNFG_BYTE_RW;
	}
	
	if(prCNFG_Setting->u4SectorSize == 512)
	{
		u4Val |= NFI_CNFG_SEL_SEC_512BYTE;
	}

	NFI_WRITE32(NFI_CNFG, u4Val);
}


//-----------------------------------------------------------------------------
// _NFI_CNFGSetting(NFI_CTRL_INFO_T *prCNFG_Setting)
//
//-----------------------------------------------------------------------------
static void _NFI_TrigHW(NFI_HWCFG_INFO_T *prTrig_Info, UINT32 u4OpMode, UINT32 u4RdNOB)
{
    UINT32 u4Val = 0;

    switch (u4OpMode)
    {
    case NFI_CNFG_OP_SINGLE:
        u4Val = NFI_CON_NOB(u4RdNOB) | NFI_CON_SRD;
        break;

    case NFI_CNFG_OP_READ:
        u4Val = NFI_CON_SEC_NUM(prTrig_Info->u4SectorNum) | NFI_CON_BRD;
        if (prTrig_Info->rCtrlInfo.fgInterrupt && prTrig_Info->rCtrlInfo.fgAutoRdDone)
        {
            u4Val |= NFI_CON_BRD_HW_EN;    
        }
        break;

    case NFI_CNFG_OP_PROGRAM:
        u4Val = NFI_CON_SEC_NUM(prTrig_Info->u4SectorNum) | NFI_CON_BWR;
        break;

    case NFI_CNFG_OP_CUSTOM:
        u4Val = NFI_CON_SEC_NUM(prTrig_Info->u4SectorNum) | NFI_CON_BWR;
        NFI_WRITE32(NFI_CON, u4Val);
        NFI_WRITE32(NFI_STRDATA, NFI_STRDATA_STRDATA);
        return;

    default:
        u4Val = 0;
        break;
    }

    NFI_WRITE32(NFI_CON, u4Val);
}

//-----------------------------------------------------------------------------
// _NFI_PartialRead(NFI_CTRL_INFO_T *prCNFG_Setting)
//
//-----------------------------------------------------------------------------
static BOOL _NFI_PartialRead(NFI_HWCFG_INFO_T *prPgRead_Info)
{
    UINT32 i, u4Val = 0;
	UINT32 *pu4Buf = prPgRead_Info->pu4Partial_Buf;
    BOOL fgRet = TRUE;
	

    for (i = 0; i < prPgRead_Info->u4Len; i += 4)
    {
        _NFI_WaitRead(4);
        pu4Buf[u4Val] = NFI_READ32(NFI_DATAR);
        u4Val++;
    }

    if (prPgRead_Info->rCtrlInfo.rECC_Type == NFIECC_SOFT)
    {
        if (prPgRead_Info->u4Len == (prPgRead_Info->u4SectorSize + ((prPgRead_Info->rDevInfo.u4OOBSz * prPgRead_Info->u4SectorSize)>>9)))
        {
            /* ECC correction. */
			if(_NFIECC_CorrectEcc(prPgRead_Info, 0))
            {
                fgRet = FALSE;
            }
        }
    }

    _NFI_WaitBusy();
    return fgRet;
}
//                                               interface function define
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// NFI_ReadyBusy()
//
//-----------------------------------------------------------------------------
UINT32 NFI_ReadyBusy(void)
{
    UINT32 u4Val;

    u4Val = NFI_READ32(NFI_STA);
    u4Val &= NFI_STA_NFI_BUSY;  // bit8=1: ready to busy

	return ((u4Val > 0)?0:1);
}

//-----------------------------------------------------------------------------
//NFI_ClkSetting(BOOL fgOn)
//fgOn = 1 : power on; fgOn = 0: power off 
//-----------------------------------------------------------------------------
void NFI_ClkSetting(BOOL fgOn)
{
    //BD
	UINT32 u4Clk;

	if(fgOn)
	{
		
		u4Clk = *NFI_CLK_SEL;
		u4Clk &= 0xFFF0FFFF; //clear old setting
		u4Clk |= NFI_CLK;
		*NFI_CLK_SEL = u4Clk;
	}
	else
	{
		u4Clk = *NFI_CLK_SEL;
    	u4Clk ^= 0x00080000;
    	*NFI_CLK_SEL = u4Clk;
	}
	

	//TV
	#if 0
	if(fgOn) //turn on clk
    {        
        NAND_IO_WRITE32(0xF000D328, 6);   // 192MHZ
    }
    else //turn off clk
    {
        NAND_IO_WRITE32(0xF000D328, 0x80); 
    }
	#endif
}

//-----------------------------------------------------------------------------
// NFI_SetTiming(UINT32 u4AccCon, UINT32 u4RdAcc)
// 
//-----------------------------------------------------------------------------
void NFI_SetTiming(UINT32 u4AccCon, UINT32 u4RdAcc)
{
    NFI_WRITE32(NFI_ACCCON1, u4AccCon);
    NFI_WRITE32(NFI_ACCCON2, u4RdAcc);
}

//-----------------------------------------------------------------------------
// NFI_SetPageFmt(UINT32 u4ChipNum)
// 
//-----------------------------------------------------------------------------
void NFI_SetPageFmt(NFI_HWCFG_INFO_T *prPgFmt_Info)
{
    UINT32 u4Val = 0;

	if(prPgFmt_Info->u4SectorSize == 512)
	{
		//page size
		if (prPgFmt_Info->rDevInfo.u4PgSz== 512)
			u4Val |= NFI_PAGEFMT_PAGE_SIZE_512_2k;		
		else if (prPgFmt_Info->rDevInfo.u4PgSz == 2048)
			u4Val |= NFI_PAGEFMT_PAGE_SIZE_2k_4k;
		else if (prPgFmt_Info->rDevInfo.u4PgSz == 4096)
			u4Val |= NFI_PAGEFMT_PAGE_SIZE_4k_8k;

		//spare size , sector size 
		if (prPgFmt_Info->rDevInfo.u4OOBSz == 16)
		{
			u4Val |= NFI_PAGEFMT_SPARE_16_32;
			u4Val |= NFI_PAGEFMT_SECTOR_SIZE_512_16;
		}
		else if (prPgFmt_Info->rDevInfo.u4OOBSz == 26)
		{
			u4Val |= NFI_PAGEFMT_SPARE_26_52;
			u4Val |= NFI_PAGEFMT_SECTOR_SIZE_512_26;
		}
		else if (prPgFmt_Info->rDevInfo.u4OOBSz == 27)
		{
			u4Val |= NFI_PAGEFMT_SPARE_27_54;
			u4Val |= NFI_PAGEFMT_SECTOR_SIZE_512_27;
		}
		else if (prPgFmt_Info->rDevInfo.u4OOBSz == 28)
		{
			u4Val |= NFI_PAGEFMT_SPARE_28_56;
			u4Val |= NFI_PAGEFMT_SECTOR_SIZE_512_28;
		}
		else if (prPgFmt_Info->rDevInfo.u4OOBSz == 32)
		{
			u4Val |= NFI_PAGEFMT_SPARE_32_64;
			u4Val |= NFI_PAGEFMT_SECTOR_SIZE_512_32;
		}
	    else if (prPgFmt_Info->rDevInfo.u4OOBSz == 36)
		{
			u4Val |= NFI_PAGEFMT_SPARE_36_72;
			u4Val |= NFI_PAGEFMT_SECTOR_SIZE_512_36;
		}
		else if (prPgFmt_Info->rDevInfo.u4OOBSz == 40)
		{
			u4Val |= NFI_PAGEFMT_SPARE_40_80;
			u4Val |= NFI_PAGEFMT_SECTOR_SIZE_512_40;
		}
		else if (prPgFmt_Info->rDevInfo.u4OOBSz == 44)
		{
			u4Val |= NFI_PAGEFMT_SPARE_44_88;
			u4Val |= NFI_PAGEFMT_SECTOR_SIZE_512_44;
		}
		else if (prPgFmt_Info->rDevInfo.u4OOBSz == 48)
		{
			u4Val |= NFI_PAGEFMT_SPARE_48_96;
			u4Val |= NFI_PAGEFMT_SECTOR_SIZE_512_48;
		}
		else if (prPgFmt_Info->rDevInfo.u4OOBSz == 50)
		{
			u4Val |= NFI_PAGEFMT_SPARE_50_100;
			u4Val |= NFI_PAGEFMT_SECTOR_SIZE_512_50;
		}
		else if (prPgFmt_Info->rDevInfo.u4OOBSz == 52)
		{
			u4Val |= NFI_PAGEFMT_SPARE_52_104;
			u4Val |= NFI_PAGEFMT_SECTOR_SIZE_512_52;
		}
		else if (prPgFmt_Info->rDevInfo.u4OOBSz == 54)
		{
			u4Val |= NFI_PAGEFMT_SPARE_54_108;
			u4Val |= NFI_PAGEFMT_SECTOR_SIZE_512_54;
		}
		else if (prPgFmt_Info->rDevInfo.u4OOBSz == 56)
		{
			u4Val |= NFI_PAGEFMT_SPARE_56_112;
			u4Val |= NFI_PAGEFMT_SECTOR_SIZE_512_56;
		}
		else if (prPgFmt_Info->rDevInfo.u4OOBSz == 62)
		{
			u4Val |= NFI_PAGEFMT_SPARE_62_124;
			u4Val |= NFI_PAGEFMT_SECTOR_SIZE_512_62;
		}
		else if (prPgFmt_Info->rDevInfo.u4OOBSz == 63)
		{
			u4Val |= NFI_PAGEFMT_SPARE_63_126;
			u4Val |= NFI_PAGEFMT_SECTOR_SIZE_512_63;
		}
		else if (prPgFmt_Info->rDevInfo.u4OOBSz == 64)
		{
			u4Val |= NFI_PAGEFMT_SPARE_64_128;
			u4Val |= NFI_PAGEFMT_SECTOR_SIZE_512_64;
		}	
	}
	else //1k sector size
	{	
		//page size
		if (prPgFmt_Info->rDevInfo.u4PgSz == 2048)
			u4Val |= NFI_PAGEFMT_PAGE_SIZE_512_2k;
		else if (prPgFmt_Info->rDevInfo.u4PgSz == 4096)
			u4Val |= NFI_PAGEFMT_PAGE_SIZE_2k_4k;
		else if (prPgFmt_Info->rDevInfo.u4PgSz == 8192)
			u4Val |= NFI_PAGEFMT_PAGE_SIZE_4k_8k;
		else if (prPgFmt_Info->rDevInfo.u4PgSz == 16384)
			u4Val |= NFI_PAGEFMT_PAGE_SIZE_16k;

		//spare size , sector size 
		if (prPgFmt_Info->rDevInfo.u4OOBSz == 16)
		{
			u4Val |= NFI_PAGEFMT_SPARE_16_32;
			u4Val |= NFI_PAGEFMT_SECTOR_SIZE_1024_32;
		}
		else if (prPgFmt_Info->rDevInfo.u4OOBSz == 26)
		{
			u4Val |= NFI_PAGEFMT_SPARE_26_52;
			u4Val |= NFI_PAGEFMT_SECTOR_SIZE_1024_52;
		}
		else if (prPgFmt_Info->rDevInfo.u4OOBSz == 27)
		{
			u4Val |= NFI_PAGEFMT_SPARE_27_54;
			u4Val |= NFI_PAGEFMT_SECTOR_SIZE_1024_54;
		}
		else if (prPgFmt_Info->rDevInfo.u4OOBSz == 28)
		{
			u4Val |= NFI_PAGEFMT_SPARE_28_56;
			u4Val |= NFI_PAGEFMT_SECTOR_SIZE_1024_56;
		}
		else if (prPgFmt_Info->rDevInfo.u4OOBSz == 32)
		{
			u4Val |= NFI_PAGEFMT_SPARE_32_64;
			u4Val |= NFI_PAGEFMT_SECTOR_SIZE_1024_64;
		}
	    else if (prPgFmt_Info->rDevInfo.u4OOBSz == 36)
		{
			u4Val |= NFI_PAGEFMT_SPARE_36_72;
			u4Val |= NFI_PAGEFMT_SECTOR_SIZE_1024_72;
		}
		else if (prPgFmt_Info->rDevInfo.u4OOBSz == 40)
		{
			u4Val |= NFI_PAGEFMT_SPARE_40_80;
			u4Val |= NFI_PAGEFMT_SECTOR_SIZE_1024_80;
		}
		else if (prPgFmt_Info->rDevInfo.u4OOBSz == 44)
		{
			u4Val |= NFI_PAGEFMT_SPARE_44_88;
			u4Val |= NFI_PAGEFMT_SECTOR_SIZE_1024_88;
		}
		else if (prPgFmt_Info->rDevInfo.u4OOBSz == 48)
		{
			u4Val |= NFI_PAGEFMT_SPARE_48_96;
			u4Val |= NFI_PAGEFMT_SECTOR_SIZE_1024_96;
		}
		else if (prPgFmt_Info->rDevInfo.u4OOBSz == 50)
		{
			u4Val |= NFI_PAGEFMT_SPARE_50_100;
			u4Val |= NFI_PAGEFMT_SECTOR_SIZE_1024_100;
		}
		else if (prPgFmt_Info->rDevInfo.u4OOBSz == 52)
		{
			u4Val |= NFI_PAGEFMT_SPARE_52_104;
			u4Val |= NFI_PAGEFMT_SECTOR_SIZE_1024_104;
		}
		else if (prPgFmt_Info->rDevInfo.u4OOBSz == 54)
		{
			u4Val |= NFI_PAGEFMT_SPARE_54_108;
			u4Val |= NFI_PAGEFMT_SECTOR_SIZE_1024_108;
		}
		else if (prPgFmt_Info->rDevInfo.u4OOBSz == 56)
		{
			u4Val |= NFI_PAGEFMT_SPARE_56_112;
			u4Val |= NFI_PAGEFMT_SECTOR_SIZE_1024_112;
		}
		else if (prPgFmt_Info->rDevInfo.u4OOBSz == 62)
		{
			u4Val |= NFI_PAGEFMT_SPARE_62_124;
			u4Val |= NFI_PAGEFMT_SECTOR_SIZE_1024_124;
		}
		else if (prPgFmt_Info->rDevInfo.u4OOBSz == 63)
		{
			u4Val |= NFI_PAGEFMT_SPARE_63_126;
			u4Val |= NFI_PAGEFMT_SECTOR_SIZE_1024_126;
		}
		else if (prPgFmt_Info->rDevInfo.u4OOBSz == 64)
		{
			u4Val |= NFI_PAGEFMT_SPARE_64_128;
			u4Val |= NFI_PAGEFMT_SECTOR_SIZE_1024_128;
		}	
		
	}
    u4Val |= NFI_PAGEFMT_FDM_NUM(prPgFmt_Info->u4FDMNum) | NFI_PAGEFMT_ECC_NUM(prPgFmt_Info->u4FDMECCNum);

    NFI_WRITE32(NFI_PAGEFMT, u4Val);
}

//-----------------------------------------------------------------------------
// NFI_ChipSelect(UINT32 u4ChipNum)
// 
//-----------------------------------------------------------------------------
void NFI_ChipSelect(UINT32 u4ChipNum)
{
	NFI_WRITE32(NFI_CSEL, u4ChipNum);
}

//-----------------------------------------------------------------------------
// NFI_ReadStatus(void)
// 
//-----------------------------------------------------------------------------
UINT32 NFI_ReadStatus(void)
{
	UINT32 u4Status, u4Randomizer;
		
	_NFI_Reset();

	//disable randomizer
	u4Randomizer = NFI_READ32(NFI_RANDOM_CFG);
	NFI_WRITE32(NFI_RANDOM_CFG, 0);
	
	NFI_WRITE32(NFI_CNFG, NFI_CNFG_OP_SINGLE | NFI_CNFG_READ_MODE);

	_NFI_CmdLatch(NAND_CMD_STATUS);
	
	// set single read by DWORD
    NFI_WRITE32(NFI_CON, NFI_CON_SRD | NFI_CON_NOB(4));
    // wait for NFI core data read done
    while(NFI_READ32(NFI_STA) & NFI_STA_NFI_DATAR);

	u4Status = NFI_READ32(NFI_DATAR);

	//reset randomizer config
	NFI_WRITE32(NFI_RANDOM_CFG, u4Randomizer);
	return u4Status;
}

//-----------------------------------------------------------------------------
// NFI_CheckWP(void)
// 
//-----------------------------------------------------------------------------
BOOL NFI_CheckWP(void)
{
	UINT32 u4Status;
		
	u4Status = NFI_ReadStatus();

	/* Check the WP bit */
    return (u4Status& STATUS_WRITE_PROTECT) ? FALSE : TRUE;
}

//-----------------------------------------------------------------------------
// NFI_ReadID(void)
// UCHAR *ucID:output, ID buffer add; UINT32 u4IDNum:input, Read ID number
//-----------------------------------------------------------------------------
void NFI_ReadID(UCHAR *ucID, UINT32 u4IDNum)
{
	UINT32 i, u4Randomizer;
	
    _NFI_Reset();

	//disable randomizer
	u4Randomizer = NFI_READ32(NFI_RANDOM_CFG);
	NFI_WRITE32(NFI_RANDOM_CFG, 0);

	_NFI_CmdLatch(NAND_CMD_READ_ID);

	_NFI_AddrLatch(0, 0, 1);

	NFI_WRITE32(NFI_CNFG, NFI_CNFG_OP_SINGLE | NFI_CNFG_BYTE_RW); //enable byte read/write

	NFI_WRITE32(NFI_CON,  NFI_CON_NOB(u4IDNum) | NFI_CON_SRD);

	for(i = 0; i < u4IDNum; i++)
	{
		_NFI_WaitRead(1);
		ucID[i] = NFI_READ32(NFI_DATAR);
	}

	//reset randomizer config
	NFI_WRITE32(NFI_RANDOM_CFG, u4Randomizer);
}

//-----------------------------------------------------------------------------
// NFI_ResetDev(BOOL fgInterrupt)
// BOOL fgInterrupt : whether use Interrupt
//-----------------------------------------------------------------------------
BOOL NFI_ResetDev(BOOL fgInterrupt)
{
    _NFI_Reset();

	if(fgInterrupt)
	{
		NFI_WRITE32(NFI_INTR_EN, NFI_INTR_EN_RESET_DONE);
	}
	else
	{
	
	}

	NFI_WRITE32(NFI_CNFG, NFI_CNFG_OP_RESET);

	_NFI_CmdLatch(NAND_CMD_RESET);

	if(fgInterrupt)
	{
		//clear interrupt
		while(!(NFI_READ32(NFI_INTR) & NFI_INTR_RESET_DONE)); //xiaolei: add timeout later
		//disable interrupt
		NFI_WRITE32(NFI_INTR, 0);
	}
	else
	{
		_NFI_WaitBusy();
	}
	return TRUE;
}

//-----------------------------------------------------------------------------
// NFI_BlockErase(BOOL fgInterrupt)
// UINT32 u4PageIdx : the first page of erase block;  BOOL fgInterrupt : whether use interrupt;  UINT32 u4RowNOB : row add nob
//-----------------------------------------------------------------------------
BOOL NFI_BlockErase(NFI_HWCFG_INFO_T *prBlockErase_Info)
{
	UINT32 u4Status;
	
	while(NFI_CheckWP()); //check WP signal
    

	prBlockErase_Info->u4PageIdx = (prBlockErase_Info->u4PageIdx / prBlockErase_Info->rDevInfo.u4PgPerBlk) * prBlockErase_Info->rDevInfo.u4PgPerBlk;
		
	_NFI_Reset();

	NFI_WRITE32(NFI_CNFG, NFI_CNFG_OP_ERASE);

	_NFI_CmdLatch(NAND_CMD_ERASE1_BLK);
	
	_NFI_AddrLatch(prBlockErase_Info->u4PageIdx, 0, (prBlockErase_Info->rDevInfo.u4RowNOB & 0x7) << 4);
	
	if(prBlockErase_Info->rCtrlInfo.fgInterrupt)
	{
		NFI_WRITE32(NFI_INTR_EN, NFI_INTR_EN_ERASE_DONE);
	}

	_NFI_CmdLatch(NAND_CMD_ERASE2_BLK);
	
	//_NFI_WaitBusy();

	if(prBlockErase_Info->rCtrlInfo.fgInterrupt)
	{
		//clear interrupt
		while(!(NFI_READ32(NFI_INTR) & NFI_INTR_ERASE_DONE)); //xiaolei: add timeout later
		//disable interrupt
		NFI_WRITE32(NFI_INTR, 0);
	}

    while (1)
    {
        u4Status= NFI_ReadStatus();
        if (u4Status& STATUS_READY_BUSY)
        {
            break;
        }
    }

    if (u4Status & STATUS_PASS_FAIL)
    {
        //LOG(0, "NANDHW_EraseBlock failed: u4BlkIdx = %d!\n", u4BlkIdx);
        return FALSE;
    }

	return TRUE;
}

//-----------------------------------------------------------------------------
// NFI_PageProgram(BOOL fgInterrupt)
// BOOL fgInterrupt : whether use interrupt  
//-----------------------------------------------------------------------------
BOOL NFI_PageProgram(NFI_HWCFG_INFO_T *prPgProgram_Info)
{
	UINT32 i, j, u4SectorOOBSize, u4SectorIdx, u4Val;
	BOOL fgRet = TRUE;
	UINT32 *pu4SectSpare;
	UINT8 u1EccCode[NFI_ENCPAR_NUM << 2];
	
	while(NFI_CheckWP()); //check WP signal

	_NFI_Reset();
	_NFIECC_Reset();

	if(prPgProgram_Info->rCtrlInfo.fgRandomizer)
	{
		_NFI_Randomizer_Encode(prPgProgram_Info->u4PageIdx, prPgProgram_Info->rDevInfo.u4PgPerBlk, TRUE);
	}
	else
	{
		_NFI_Randomizer_Encode(prPgProgram_Info->u4PageIdx, prPgProgram_Info->rDevInfo.u4PgPerBlk, FALSE);
	}

	u4SectorOOBSize= (prPgProgram_Info->rDevInfo.u4OOBSz * prPgProgram_Info->u4SectorSize) >> 9;

	//set dram addr && fdm data
	if(prPgProgram_Info->rCtrlInfo.fgAHBMode)
	{
		NFI_WRITE32(NFI_STRADDR, (UINT32)prPgProgram_Info->pu4Main_Buf);
		
	    for (i = 0; i < prPgProgram_Info->u4SectorNum; i++)
	    {
	        pu4SectSpare = (UINT32 *)((UINT32)prPgProgram_Info->pu4Spare_Buf + u4SectorOOBSize * i);

	        for (j = 0; j < (prPgProgram_Info->u4FDMNum + 3) / 4; j++)
	        {
	            if (i < 8)
	                NFI_WRITE32((NFI_FDM0L + 4*j+ NFI_FDM_LEN * i), pu4SectSpare[j]);
	            else
	                NFI_WRITE32((NFI_FDM8L + 4*j+ NFI_FDM_LEN * (i-8)), pu4SectSpare[j]);
	        }
	    }
	}
	else if(prPgProgram_Info->rCtrlInfo.rECC_Type == NFIECC_HARD)
	{
		for (i = 0; i < prPgProgram_Info->u4SectorNum; i++)
	    {
	        pu4SectSpare = (UINT32 *)((UINT32)prPgProgram_Info->pu4Spare_Buf + u4SectorOOBSize * i);

	        for (j = 0; j < (prPgProgram_Info->u4FDMNum + 3) / 4; j++)
	        {
	            if (i < 8)
	                NFI_WRITE32((NFI_FDM0L + 4*j+ NFI_FDM_LEN * i), pu4SectSpare[j]);
	            else
	                NFI_WRITE32((NFI_FDM8L + 4*j+ NFI_FDM_LEN * (i-8)), pu4SectSpare[j]);
	        }
	    }
	}

	//set byte r/w
	if(!prPgProgram_Info->rCtrlInfo.fgAHBMode)
	{
		if((u4SectorOOBSize + prPgProgram_Info->u4SectorSize) % 0x3)
			prPgProgram_Info->rCtrlInfo.fgByteRW = TRUE;
		else
			prPgProgram_Info->rCtrlInfo.fgByteRW = FALSE;
	}
		
	_NFI_CNFGSetting(prPgProgram_Info, NFI_CNFG_OP_PROGRAM);

	_NFIECC_Cfg(prPgProgram_Info->rCtrlInfo.rECC_Type, prPgProgram_Info->rDevInfo.u4OOBSz, prPgProgram_Info->rDevInfo.u4PgSz);

	_NFIECC_Trig(NFI_CNFG_OP_PROGRAM);

	_NFI_CmdLatch(NAND_CMD_INPUT_PAGE);
	
	_NFI_AddrLatch(prPgProgram_Info->u4PageIdx, prPgProgram_Info->u4Offset, (prPgProgram_Info->rDevInfo.u4RowNOB << 4) | prPgProgram_Info->rDevInfo.u4ColNOB);

	if(prPgProgram_Info->rCtrlInfo.fgInterrupt)
	{
		_NFI_mtd_nand_setisr(NFI_INTR_EN_AHB_DONE);
	}

	_NFI_TrigHW(prPgProgram_Info, NFI_CNFG_OP_PROGRAM, 0);

	if(prPgProgram_Info->rCtrlInfo.fgInterrupt)
	{
		_NFI_mtd_nand_waitisr(NFI_INTR_AHB_DONE);
	}

	_NFI_WaitBusy();

	if(prPgProgram_Info->rCtrlInfo.fgAHBMode)
	{
		_NFIECC_WaitBusy();
	}
	else
    {
        for (u4SectorIdx = 0; u4SectorIdx < prPgProgram_Info->u4SectorNum; u4SectorIdx++)
        {
            // write main data
            _NFI_WriteFifo((UINT8*)&prPgProgram_Info->pu4Main_Buf[u4SectorIdx * u4SectorOOBSize], prPgProgram_Info->u4SectorSize);

            if (prPgProgram_Info->rCtrlInfo.rECC_Type == NFIECC_SOFT)
            {
                // write fdm data
                _NFI_WriteFifo((UINT8*)&prPgProgram_Info->pu4Spare_Buf[u4SectorIdx * u4SectorOOBSize], prPgProgram_Info->u4FDMNum);

                /* ECC correction. */
                _NFI_WaitWrite();
				
                _NFIECC_GetECCParity((UINT32 *)u1EccCode);
                
                // write ecc data
                u4Val = u4SectorOOBSize - prPgProgram_Info->u4FDMNum;
                _NFI_WriteFifo(u1EccCode, u4Val);
            }
            else if (prPgProgram_Info->rCtrlInfo.rECC_Type == NFIECC_NONE)
            {
                // write spare data
                _NFI_WriteFifo((UINT8*)&prPgProgram_Info->pu4Spare_Buf[u4SectorIdx * u4SectorOOBSize], u4SectorOOBSize);
            }
            else if (prPgProgram_Info->rCtrlInfo.rECC_Type == NFIECC_HARD)
            {
                // HW ECC do nothing
            }
            else
            {
                ASSERT(0);
            }
        }
    }

	_NFI_WaitSectorCnt(prPgProgram_Info->u4SectorNum);
    _NFI_WaitBusy();

    /* Trigger NAND to program*/
	_NFI_CmdLatch(NAND_CMD_PROG_PAGE);

    // Read status back
    while (1)
    {
        u4Val = NFI_ReadStatus();
        if (u4Val & STATUS_READY_BUSY)
        {
            break;
        }
    }

    if (u4Val & STATUS_PASS_FAIL)
    {
        ///LOG(0, "Write page failed, pgidx:%d\n", u4PgIdx);
        fgRet = FALSE;
        goto HandleExit;
    }

    if(prPgProgram_Info->rCtrlInfo.fgRandomizer)
    {
        _NFI_Randomizer_Encode(prPgProgram_Info->u4PageIdx, prPgProgram_Info->rDevInfo.u4PgPerBlk, FALSE);
    }
	
HandleExit:
    _NFI_WaitBusy();
    NFI_WRITE32(NFI_CNFG, 0);

    if(prPgProgram_Info->rCtrlInfo.fgRandomizer)
    {
        _NFI_Randomizer_Encode(prPgProgram_Info->u4PageIdx, prPgProgram_Info->rDevInfo.u4PgPerBlk, FALSE);
    }
    return fgRet;
}

//-----------------------------------------------------------------------------
// NFI_PageRead(BOOL fgInterrupt)
// BOOL fgInterrupt : whether use interrupt  
//-----------------------------------------------------------------------------


BOOL NFI_PageRead(NFI_HWCFG_INFO_T *prPgRead_Info)
{
	UINT32 i, j, u4SectorOOBSize, u4SectorIdx;
	BOOL fgRet = TRUE;
	UINT32 *pu4SectSpare = NULL;
	//UINT8 u1EccCode[NFI_ENCPAR_NUM << 2];

	fgEmptyPage = FALSE;
	
	_NFI_Reset();
	_NFIECC_Reset();

	if(prPgRead_Info->rCtrlInfo.fgRandomizer)
	{
		_NFI_Randomizer_Decode(prPgRead_Info->u4PageIdx, prPgRead_Info->rDevInfo.u4PgPerBlk, TRUE);
	}
	else
	{
		_NFI_Randomizer_Decode(prPgRead_Info->u4PageIdx, prPgRead_Info->rDevInfo.u4PgPerBlk, FALSE);
	}

	u4SectorOOBSize= (prPgRead_Info->rDevInfo.u4OOBSz * prPgRead_Info->u4SectorSize) >> 9;
	
	if(prPgRead_Info->rCtrlInfo.fgAHBMode)
	{
		NFI_WRITE32(NFI_STRADDR, (UINT32)prPgRead_Info->pu4Main_Buf);
		NFIECC_WRITE32(NFIECC_FDMADDR, NFI_BASE_ADD + NFI_FDM0L);
	}

	_NFI_CNFGSetting(prPgRead_Info, NFI_CNFG_OP_READ);

	_NFIECC_Cfg(prPgRead_Info->rCtrlInfo.rECC_Type, prPgRead_Info->rDevInfo.u4OOBSz, prPgRead_Info->rDevInfo.u4PgSz);

	_NFIECC_Trig(NFI_CNFG_OP_READ);

	NFI_WRITE32(NFI_FIFOSTA, NFI_EMPTY_BIT_NUM(40)); //set empty bit number

	_NFI_CmdLatch(NAND_CMD_READ1_00);
	
	_NFI_AddrLatch(prPgRead_Info->u4PageIdx, prPgRead_Info->u4Offset, (prPgRead_Info->rDevInfo.u4RowNOB << 4) | prPgRead_Info->rDevInfo.u4ColNOB);

	if(prPgRead_Info->rDevInfo.u4PgSz >512)
	{
		if(prPgRead_Info->rCtrlInfo.fgInterrupt)
		{
			if(prPgRead_Info->rCtrlInfo.fgAutoRdDone)
				_NFI_mtd_nand_setisr(NFI_INTR_EN_RD_DONE | NFI_INTR_EN_AHB_DONE);
			
			else
				_NFI_mtd_nand_setisr(NFI_INTR_EN_RD_DONE | NFI_INTR_EN_AHB_DONE);
		}
		
		_NFI_CmdLatch(NAND_CMD_READ_30);
		
		if(prPgRead_Info->rCtrlInfo.fgInterrupt)
		{
			if(!prPgRead_Info->rCtrlInfo.fgAutoRdDone)
				_NFI_mtd_nand_waitisr(NFI_INTR_RD_DONE);
		}
	}

	if(!prPgRead_Info->rCtrlInfo.fgAutoRdDone)
	{
		_NFI_WaitBusy();

		while((NFI_READ32(NFI_STA) && NFI_STA_NFI_FSM) != NFI_STA_NFI_FSM_READ_DATA); // check opmode
		
		if(prPgRead_Info->rCtrlInfo.fgInterrupt)
			_NFI_mtd_nand_setisr(NFI_INTR_EN_AHB_DONE);
	}

	_NFI_TrigHW(prPgRead_Info, NFI_CNFG_OP_READ, 0);

	if(prPgRead_Info->rCtrlInfo.fgInterrupt)
	{
		
		_NFI_mtd_nand_waitisr(NFI_INTR_AHB_DONE);
	}

	_NFI_WaitBusy();

	if(prPgRead_Info->rCtrlInfo.fgAHBMode)
	{
		_NFIECC_WaitBusy();
		
		for (i = 0; i < prPgRead_Info->u4SectorNum; i++)
		{
			pu4SectSpare = (UINT32 *)((UINT32)prPgRead_Info->pu4Spare_Buf + u4SectorOOBSize * i);
		
			for (j = 0; j < (prPgRead_Info->u4FDMNum + 3) / 4; j++)
			{
				if (i < 8)
					pu4SectSpare[j] = NFI_READ32(NFI_FDM0L + 4*j + NFI_FDM_LEN * i);
				else
					pu4SectSpare[j] = NFI_READ32(NFI_FDM8L + 4*j + NFI_FDM_LEN * (i-8));
			}
		}
	}
	else
    {
        for (u4SectorIdx = 0; u4SectorIdx < prPgRead_Info->u4SectorNum; u4SectorIdx++)
        {
            // read main data
            _NFI_ReadFifo((UINT8*)&prPgRead_Info->pu4Main_Buf[u4SectorIdx * prPgRead_Info->u4SectorSize], prPgRead_Info->u4SectorSize);

            if (prPgRead_Info->rCtrlInfo.rECC_Type == NFIECC_SOFT)
            {
                // read fdm data
                _NFI_ReadFifo((UINT8*)&prPgRead_Info->pu4Spare_Buf[u4SectorIdx * u4SectorOOBSize], u4SectorOOBSize);

				if (_NFIECC_CorrectEcc(prPgRead_Info, u4SectorIdx))
                {
                    fgRet = FALSE;//|= NAND_ECC_ERROR;
                }
            }
            else if (prPgRead_Info->rCtrlInfo.rECC_Type == NFIECC_NONE)
            {
                // write spare data
                _NFI_ReadFifo((UINT8*)&prPgRead_Info->pu4Spare_Buf[u4SectorIdx * u4SectorOOBSize], u4SectorOOBSize);
            }
            else if (prPgRead_Info->rCtrlInfo.rECC_Type == NFIECC_HARD)
            {
                _NFI_WaitEccDone(u4SectorIdx);
            }
            else
            {
                ASSERT(0);
            }

			_NFI_WaitBusy();
        }
		
		_NFIECC_WaitBusy();
		
        if (prPgRead_Info->rCtrlInfo.rECC_Type == NFIECC_HARD)
        {
        	for (i = 0; i < prPgRead_Info->u4SectorNum; i++)
			{
				pu4SectSpare = (UINT32 *)((UINT32)prPgRead_Info->pu4Spare_Buf + u4SectorOOBSize * i);
			
				for (j = 0; j < (prPgRead_Info->u4FDMNum + 3) / 4; j++)
				{
					if (i < 8)
						pu4SectSpare[j] = NFI_READ32(NFI_FDM0L + 4*j + NFI_FDM_LEN * i);
					else
						pu4SectSpare[j] = NFI_READ32(NFI_FDM8L + 4*j + NFI_FDM_LEN * (i-8));
				}
			}
        }

        if (!fgRet)
        {
            goto HandleExit;
        }
    }

	_NFI_WaitSectorCnt(prPgRead_Info->u4SectorNum);
	
    if (prPgRead_Info->rCtrlInfo.rECC_Type == NFIECC_HARD)
	{
		for (u4SectorIdx = 0; u4SectorIdx < prPgRead_Info->u4SectorNum; u4SectorIdx++)
		{
			_NFI_WaitEccDone(u4SectorIdx);
		}
					
		for (i = 0; i < prPgRead_Info->u4SectorNum; i++)
		{
			pu4SectSpare = (UINT32 *)((UINT32)prPgRead_Info->pu4Spare_Buf + u4SectorOOBSize * i);
		
			for (j = 0; j < (prPgRead_Info->u4FDMNum + 3) / 4; j++)
			{
				if (i < 8)
					pu4SectSpare[j] = NFI_READ32(NFI_FDM0L + 4*j + NFI_FDM_LEN * i);
				else
					pu4SectSpare[j] = NFI_READ32(NFI_FDM8L + 4*j + NFI_FDM_LEN * (i-8));
			}
		}

	    fgRet = _NFIECC_CheckEcc((UINT8*)prPgRead_Info->pu4Main_Buf, (UINT8*)pu4SectSpare, prPgRead_Info);
	    if (fgRet)
	    {
	    	fgRet = FALSE;
	        goto HandleExit;
	    }
	}

HandleExit:
    _NFI_WaitBusy();
    NFI_WRITE32(NFI_CNFG, 0);

    if(prPgRead_Info->rCtrlInfo.fgRandomizer)
    {
        _NFI_Randomizer_Decode(prPgRead_Info->u4PageIdx, prPgRead_Info->rDevInfo.u4PgPerBlk, FALSE);
    }
    return fgRet;
}

//-----------------------------------------------------------------------------
// NFI_PartialPageRead
//  
// spare read or other less than one sector partial read , no need to wait sector counter
//-----------------------------------------------------------------------------
#if 0
static void _NFI_PrintRegParameter(void)
{
	UINT32 addr  =0x800;

	while(1)
	{
		if(addr > 0xBD4)
			break;

		printk("register 0x%x  value is :0x%x\n", (NFI_BASE_ADD + addr), *(volatile unsigned int *)(NFI_BASE_ADD + addr));

		addr = addr + 0x4;
	}

	addr = 0xBF0;
	printk("register 0x%x  value is :0x%x\n", (NFI_BASE_ADD + addr), *(volatile unsigned int *)(NFI_BASE_ADD + addr));


	addr = 0xBFC;
	printk("register 0x%x  value is :0x%x\n", (NFI_BASE_ADD + addr), *(volatile unsigned int *)(NFI_BASE_ADD + addr));


}
#endif 

BOOL NFI_PartialPageRead(NFI_HWCFG_INFO_T *prPgRead_Info)
{
    BOOL fgRet = TRUE;

    _NFI_Reset();
    _NFIECC_Reset();

	_NFI_CNFGSetting(prPgRead_Info, NFI_CNFG_OP_READ);
    _NFIECC_Cfg(prPgRead_Info->rCtrlInfo.rECC_Type,prPgRead_Info->rDevInfo.u4OOBSz,prPgRead_Info->rDevInfo.u4PgSz);
    _NFIECC_Trig(NFI_CNFG_OP_READ);

    /* Write command. */
    if ((prPgRead_Info->u4Offset == 0x200) && (prPgRead_Info->rDevInfo.u4PgSz == 0x200))
    {
        _NFI_CmdLatch(NAND_CMD_READOOB);
        _NFI_AddrLatch(prPgRead_Info->u4PageIdx, 0, (prPgRead_Info->rDevInfo.u4RowNOB << 4) | prPgRead_Info->rDevInfo.u4ColNOB);
    }
    else
    {
        _NFI_CmdLatch(NAND_CMD_READ0);
		_NFI_AddrLatch(prPgRead_Info->u4PageIdx, prPgRead_Info->u4Offset, (prPgRead_Info->rDevInfo.u4RowNOB << 4) | prPgRead_Info->rDevInfo.u4ColNOB);
    }

    if (prPgRead_Info->rDevInfo.u4PgSz != 0x200)
    {
        _NFI_CmdLatch(NAND_CMD_READSTART);
    }
    _NFI_WaitBusy();

    while((NFI_READ32(NFI_STA) & NFI_STA_NFI_FSM) != NFI_STA_NFI_FSM_READ_DATA); // check opmode

	_NFI_TrigHW(prPgRead_Info, NFI_CNFG_OP_READ, 0);
	
    _NFI_WaitBusy();
		
    fgRet = _NFI_PartialRead(prPgRead_Info);
    if (!fgRet)
    {
        goto HandleError;
    }

    _NFI_WaitBusy();
    NFI_WRITE32(NFI_CNFG, 0);
    return TRUE;

HandleError:
    _NFI_WaitBusy();
    NFI_WRITE32(NFI_CNFG, 0);
    return fgRet;
}

//-----------------------------------------------------------------------------
// NFI_CmdFun
//  
// cmd setting
//-----------------------------------------------------------------------------
void NFI_CmdFun(UINT32 u4Cmd)
{
	_NFI_CmdLatch(u4Cmd);
}

//-----------------------------------------------------------------------------
// NFI_CmdFun
//  
// cmd setting
//-----------------------------------------------------------------------------
void NFI_AddrFun(UINT32 u4RowAddr, UINT32 u4ColAddr, UINT32 u4ByteCount)
{
	_NFI_AddrLatch(u4RowAddr, u4ColAddr, u4ByteCount);
}

//-----------------------------------------------------------------------------
// NFI_ReadOneByte
//  
// just read one byte
//-----------------------------------------------------------------------------
UCHAR NFI_ReadOneByte(NFI_HWCFG_INFO_T *prPgRead_Info)
{
	UCHAR u1Reg;

	_NFI_WaitBusy();
	_NFI_CNFGSetting(prPgRead_Info, NFI_CNFG_OP_SINGLE);
	NFI_WRITE32(NFI_CON, NFI_CON_NOB(1) | NFI_CON_SRD);

	_NFI_WaitBusy();
	_NFI_WaitRead(1);
	u1Reg = (UINT8)(NFI_READ32(NFI_DATAR) & 0xFF);

	_NFI_Reset();
	return u1Reg;
}

//-----------------------------------------------------------------------------
// NFI_PMUXSetting
//  
// set nand pinmux
//-----------------------------------------------------------------------------
void NFI_PMUXSetting(void)
{
	UINT32 u4Val;
	
	u4Val = NFI_READ32(NFI_MISC);
    u4Val |= NFI_MISC_FLASH_PMUX;
    NFI_WRITE32(NFI_MISC, u4Val);

	IO_W32(0xf000d48c, IO_R32(0xf000d48c) & 0x8E38E3FF);//WE driving clear
    IO_W32(0xf000d490, IO_R32(0xf000d490) & 0xE38E38E3);//pin driving clear
    IO_W32(0xf000d48c, IO_R32(0xf000d48c) | 0x10410400);//WE driving setting
    IO_W32(0xf000d490, IO_R32(0xf000d490) | 0x04104104);//pin driving setting
    IO_W32(0xf000d540, IO_R32(0xf000d540) | 0x1);//POOE driving 
    IO_W32(0xf000d550, IO_R32(0xf000d550) & 0xFFFFFFFE);//POOE driving 
}
    
//-----------------------------------------------------------------------------
// NFI_RandomEnable
//	
// nfi randomizer 
//-----------------------------------------------------------------------------
BOOL NFI_RandomEnable(void)
{
	if(0x00400000 == (IO_R32(0xf0008664)&0x00400000))//5399 randomizer efuse bit
		return TRUE; //support randomizer
	else
		return FALSE;
}

//-----------------------------------------------------------------------------
// NFI_CLKONOFF
//	
// nfi clk on/off
//-----------------------------------------------------------------------------
void NFI_CLKONOFF(BOOL fgOn)
{
    if(fgOn) //turn on clk
    {        
        IO_W32(0xF000D328, 6);   // 192MHZ
    }
    else //turn off clk
    {
        IO_W32(0xF000D328, 0x80); 
    }
}

