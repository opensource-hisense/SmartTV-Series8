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

#include <common.h>

#if defined(CONFIG_CMD_NAND)

#include "x_bim.h"
#include "x_timer.h"
#include "nandhw_reg.h"
#include "drvcust_if.h"
#include "x_hal_arm.h"

#undef NAND_CMD_RESET
#include <linux/mtd/nand.h>
#include <linux/mtd/mtd.h>
#include <config.h>
#include <malloc.h>
#ifdef CC_UBOOT_2011_12
#include <asm/errno.h>
#else
#include <asm-arm/errno.h>
#endif
//-----------------------------------------------------------------------------
// Constant definitions
//-----------------------------------------------------------------------------
#define NAND_SCRAMBLE_EN            (0)
#define NAND_RANDOM_EN              (1)
#define NAND_AUTORD_DONE            (0)
#if CC_NAND_60BIT_NFI
   #if NAND_RANDOM_EN
   static  BOOL fgRandomizer = FALSE;
   static  BOOL fgEmptyPage  = FALSE;
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
   #endif
#endif
/////////////////////////////////////////////////////////////////
unsigned long volatile jiffies;
#define HZ 5000
#define time_after(x, y) 0
#define complete(x)
#define init_completion(x)
#define wait_for_completion(x)
#define virt_to_phys(x) ((u_long)x)
#define init_MUTEX(x)
#define printk  printf
/////////////////////////////////////////////////////////////////

#define NAND_DEBUG_PRINT                (0)

#define NAND_DMA_CACHE_LINE             (32)
#define NAND_DMA_PATTERN                (0xFEDCBA98)
#define NAND_DMA_RETYR_DELAY            (2)
#define NAND_DMA_RETYR_CNT              (50)

#define NAND_DMA_READ_VERIFY            (0)

#define NAND_SUCC                       (0)
#define NAND_FAIL                       (1)
#define NAND_ECC_ERROR                  (4)
#define NAND_CMD_ERROR                  (8)
#define NAND_INVALID_ARG                (16)
#define POLLING_RETRY                   (512*512)
#if (defined(CC_MT5398) && defined(CC_NAND_60BIT)) || defined(CC_MT5890) || defined(CC_MT5891) || defined(CC_MT5891)
    #define NAND_MAX_PAGE_SIZE              (1024*16)
    #define NAND_MAX_OOB_SIZE               (80*16)
#else
    #define NAND_MAX_PAGE_SIZE              (1024*8)
    #define NAND_MAX_OOB_SIZE               (64*8)
#endif

#if CC_NAND_60BIT_NFI
#define NAND_REG_BASE                   ((UINT32)0xF0029400)
#else
#define NAND_REG_BASE                   ((UINT32)0xF0029000)
#endif
#define NAND_READ16(offset)             (*((volatile UINT16*)(NAND_REG_BASE + offset)))
#define NAND_READ32(offset)             (*((volatile UINT32*)(NAND_REG_BASE + offset)))
#define NAND_WRITE32(offset, _val_)     (*((volatile UINT32*)(NAND_REG_BASE + offset)) = (_val_))

#define NAND_IO_READ32(offset)          (*((volatile UINT32*)(offset)))
#define NAND_IO_WRITE32(offset, _val_)  (*((volatile UINT32*)(offset)) = (_val_))

#if defined(YES_BIMSWAP)
#define NAND_BIMSWAP(X)                 ((((X) & 0xf00) > 0x200) ? (X) : (((X) & 0xffffffeb) | (((X) & 0x10) >> 2) | (((X) & 0x4) << 2)))
#else
#define NAND_BIMSWAP(X)                 (X)
#endif

#define NAND_BIM_READ32(offset)         (*((volatile UINT32*)(BIM_BASE + NAND_BIMSWAP(offset))))
#define NAND_BIM_WRITE32(offset, _val_) (*((volatile UINT32*)(BIM_BASE + NAND_BIMSWAP(offset))) = (_val_))

// mtk partition information
extern mtk_part_info_t rMTKPartInfo[];

// Device information
#define NAND_BBMODE_BBMARK	0x0000FF00
#define NAND_BBMODE_ECCBIT	0x000000FF

typedef struct
{
    CHAR   acName[32];
    UINT8  u1AddrCycle;
    UINT32 u4IDVal;
    UINT32 u4IDMask;
    UINT32 u4PageSize;
    UINT32 u4OOBSize;
    UINT32 u4BBMode;
    UINT32 u4BlkSize;
    UINT32 u4BlkPgCount;
    UINT32 u4ChipSize;
    UINT32 u4AccCon;
    UINT32 u4RdAcc;
} NAND_FLASH_DEV_T;

// static variables
static BOOL _fgDmaEn = TRUE;
static NAND_FLASH_DEV_T *_prFlashDev;
static UINT32 _u4BlkPgCount;
static UINT32 _u4FdmNum;
static UINT32 _u4SectorNum;
static UINT32 _u4PgAddrLen;
static UINT32 _u4TotalAddrLen;
static UINT32 _u4PageSize = 2048;
static UINT32 _u4OOBSize = 16;
static UINT32 _u4SectorSize = 1024;
static UINT32 _u4ECCBits = 0;
#ifdef CC_UBOOT_2011_12
static UINT32 _u4CurBitFlip = 0;
#endif
static UINT32 _u4CurPage = 0xFFFFFFFF;

static UINT32 _u4NandDataBuf[(NAND_MAX_PAGE_SIZE + 64)/4];
static UINT32 _u4NandOobBuf[NAND_MAX_OOB_SIZE/4];

static UINT8 *_pu1NandDataBuf = NULL;
static UINT8 *_pu1NandOobBuf  = NULL;

#if NAND_DMA_READ_VERIFY
static UINT32 _u4CopyDataBuf[(NAND_MAX_PAGE_SIZE + 64)/4];
static UINT32 _u4CopyOobBuf[NAND_MAX_OOB_SIZE/4];
static UINT32 _u4VerifyDataBuf[(NAND_MAX_PAGE_SIZE + 64)/4];
static UINT32 _u4VerifyOobBuf[NAND_MAX_OOB_SIZE/4];
#endif

const static NAND_FLASH_DEV_T _arNand_DevInfo[] =
{
    { "SS K9F2808U0B",         3, 0x000073EC, 0x0000FFFF, 512,  16, 0x00000501,  0x04000,  32,  0x01000000, 0x00411888, 0x00080008},
    { "SS K9F5608U0B",         3, 0x000075EC, 0x0000FFFF, 512,  16, 0x00000501,  0x04000,  32,  0x02000000, 0x00411888, 0x00080008},
    { "SS K9F1208U0B/C",       4, 0x000076EC, 0x0000FFFF, 512,  16, 0x00000501,  0x04000,  32,  0x04000000, 0x00411888, 0x00080008},
    { "SS K9F1G08U0B/C/D",     4, 0x9500F1EC, 0xFFFFFFFF, 2048, 16, 0x00000001,  0x20000,  64,  0x08000000, 0x00433322, 0x00050005},
    { "SS K9F2G08U0B/C",       5, 0x9510DAEC, 0xFFFFFFFF, 2048, 16, 0x00000001,  0x20000,  64,  0x10000000, 0x00433322, 0x00050005},
    { "SS K9F4G08U0B/D",       5, 0x9510DCEC, 0xFFFFFFFF, 2048, 16, 0x00000001,  0x20000,  64,  0x20000000, 0x00433322, 0x00050005},
    { "SS K9K8G08U0D",         5, 0x9551D3EC, 0xFFFFFFFF, 2048, 16, 0x00000001,  0x20000,  64,  0x40000000, 0x00433322, 0x00050005},
    { "SS K9F8G08U0M",         5, 0xA610D3EC, 0xFFFFFFFF, 4096, 16, 0x00000001,  0x40000,  64,  0x40000000, 0x00433322, 0x00050005},
    { "SS K9GAG08U0E",         5, 0x7284D5EC, 0xFFFFFFFF, 8192, 26, 0x00000018,  0x100000, 128, 0x80000000, 0x00433322, 0x00050005},
    { "SS K9GAG08U0F",         5, 0x7694D5EC, 0xFFFFFFFF, 8192, 26, 0x00000018,  0x100000, 128, 0x80000000, 0x00433322, 0x00050005},
    //1.8V nand
    { "SS KF91G08Q2V",         5, 0x1900A1EC, 0xFFFFFFFF, 2048, 16, 0x00000001,  0x20000,  64,  0x10000000, 0x00433322, 0x00050005},
    { "SS KF92G08Q2V",         5, 0x1900AAEC, 0xFFFFFFFF, 2048, 16, 0x00000001,  0x20000,  64,  0x10000000, 0x00433322, 0x00050005},

    { "SS 1Gb",                4, 0x0000F1EC, 0x0000FFFF, 2048, 16, 0x00000001,  0x20000,  64,  0x08000000, 0x00411888, 0x00080008},

    { "NAND128W3A2B",          3, 0x00007320, 0x0000FFFF, 512,  16, 0x00000501,  0x04000,  32,  0x01000000, 0x00411888, 0x00080008},
    { "NAND256W3A2B",          3, 0x00007520, 0x0000FFFF, 512,  16, 0x00000501,  0x04000,  32,  0x02000000, 0x00411888, 0x00080008},
    { "NAND512W3A2B/D",        4, 0x00007620, 0x0000FFFF, 512,  16, 0x00000501,  0x04000,  32,  0x04000000, 0x00411888, 0x00080008},
    { "NAND01GW3B2B",          4, 0x1D80F120, 0xFFFFFFFF, 2048, 16, 0x00000001,  0x20000,  64,  0x08000000, 0x00411888, 0x00080008},
    { "NAND01GW3B2C",          4, 0x1D00F120, 0xFFFFFFFF, 2048, 16, 0x00000001,  0x20000,  64,  0x08000000, 0x00433322, 0x00050005},
    { "NAND02GW3B2C",          5, 0x1D80DA20, 0xFFFFFFFF, 2048, 16, 0x00000001,  0x20000,  64,  0x10000000, 0x00411888, 0x00080008},
    { "NAND02GW3B2D",          5, 0x9510DA20, 0xFFFFFFFF, 2048, 16, 0x00000001,  0x20000,  64,  0x10000000, 0x00433322, 0x00050005},
    { "NAND04GW3B2B",          5, 0x9580DC20, 0xFFFFFFFF, 2048, 16, 0x00000001,  0x20000,  64,  0x20000000, 0x00411888, 0x00080008},
    { "NAND04GW3B2D",          5, 0x9510DC20, 0xFFFFFFFF, 2048, 16, 0x00000001,  0x20000,  64,  0x20000000, 0x00411888, 0x00080008},
    { "NAND08GW3B2C",          5, 0x9551D320, 0xFFFFFFFF, 2048, 16, 0x00000001,  0x20000,  64,  0x40000000, 0x00411888, 0x00080008},
    { "NAND08GW3C2B/16GW3C4B", 5, 0xA514D320, 0xFFFFFFFF, 2048, 16, 0x00000008,  0x40000,  128, 0x40000000, 0x00411888, 0x00080008},
    { "NAND08GW3F2A",          5, 0xA610D320, 0xFFFFFFFF, 4096, 16, 0x00000001,  0x40000,  64,  0x40000000, 0x00433322, 0x00050005},
    { "ST 1Gb",                4, 0x0000F120, 0x0000FFFF, 2048, 16, 0x00000001,  0x20000,  64,  0x08000000, 0x00411888, 0x00080008},
    { "ST 2Gb",                5, 0x0000DA20, 0x0000FFFF, 2048, 16, 0x00000001,  0x20000,  64,  0x10000000, 0x00411888, 0x00080008},
    { "ST 4Gb",                5, 0x0000DC20, 0x0000FFFF, 2048, 16, 0x00000001,  0x20000,  64,  0x20000000, 0x00411888, 0x00080008},
    { "ST 8Gb",                5, 0x0000D320, 0x0000FFFF, 4096, 16, 0x00000001,  0x40000,  64,  0x40000000, 0x00411888, 0x00080008},

    { "HY27US08281A",          3, 0x000073AD, 0x0000FFFF, 512,  16, 0x00000501,  0x04000,  32,  0x01000000, 0x00411888, 0x00080008},
    { "HY27US08561A",          3, 0x000075AD, 0x0000FFFF, 512,  16, 0x00000501,  0x04000,  32,  0x02000000, 0x00411888, 0x00080008},
    { "H27U518S2CTR/P",        4, 0x000076AD, 0x0000FFFF, 512,  16, 0x00000001,  0x04000,  32,  0x04000000, 0x00433322, 0x00050005},
    { "HY27UF081G2A",          4, 0x1D80F1AD, 0xFFFFFFFF, 2048, 16, 0x00000001,  0x20000,  64,  0x08000000, 0x00411888, 0x00080008},
    { "HY27U1G8F2BTR",         4, 0x1D00F1AD, 0xFFFFFFFF, 2048, 16, 0x00000001,  0x20000,  64,  0x08000000, 0x00433322, 0x00050005},
    { "HY27UF082G2B",          5, 0x9510DAAD, 0xFFFFFFFF, 2048, 16, 0x00000001,  0x20000,  64,  0x10000000, 0x00433322, 0x00050005},
    { "HY27U2G8F2CTR",         5, 0x9590DAAD, 0xFFFFFFFF, 2048, 16, 0x00000001,  0x20000,  64,  0x10000000, 0x00433322, 0x00050005},
    { "HY27U4G8F2D/ETR",       5, 0x9590DCAD, 0xFFFFFFFF, 2048, 16, 0x00000001,  0x20000,  64,  0x20000000, 0x00433433, 0x00060006},
    { "HY27UF084G2B",          5, 0x9510DCAD, 0xFFFFFFFF, 2048, 16, 0x00000001,  0x20000,  64,  0x20000000, 0x00433322, 0x00050005},
    { "HY27UT088G2A",          5, 0xA514D3AD, 0xFFFFFFFF, 2048, 16, 0x00000008,  0x40000,  128, 0x40000000, 0x00411888, 0x00080008},
    { "H27U8G8T2BTR",          5, 0xB614D3AD, 0xFFFFFFFF, 4096, 16, 0x00000008,  0x80000,  128, 0x40000000, 0x00433322, 0x00050005},
    { "H27UAG8T2MTR",          5, 0xB614D5AD, 0xFFFFFFFF, 4096, 16, 0x00000008,  0x80000,  128, 0x80000000, 0x00433322, 0x00050005},
    { "H27UAG8T2ATR",          5, 0x2594D5AD, 0xFFFFFFFF, 4096, 26, 0x00000018,  0x80000,  128, 0x80000000, 0x00433322, 0x00050005},
    { "H27UAG8T2BTR",          5, 0x9A94D5AD, 0xFFFFFFFF, 8192, 26, 0x00000018,  0x200000, 256, 0x80000000, 0x00433322, 0x00050005},
    { "HYNIX 1Gb",             4, 0x0000F1AD, 0x0000FFFF, 2048, 16, 0x00000001,  0x20000,  64,  0x08000000, 0x00411888, 0x00080008},
    { "HYNIX 2Gb",             5, 0x0000DAAD, 0x0000FFFF, 2048, 16, 0x00000001,  0x20000,  64,  0x10000000, 0x00411888, 0x00080008},
    { "HYNIX 4Gb",             5, 0x0000DCAD, 0x0000FFFF, 2048, 16, 0x00000001,  0x20000,  64,  0x20000000, 0x00411888, 0x00080008},
    { "HYNIX 8Gb",             5, 0x0000D3AD, 0x0000FFFF, 2048, 16, 0x00000001,  0x40000,  128, 0x40000000, 0x00411888, 0x00080008},

    { "MT29F1G08AAC/ABAD/EA",  4, 0x9580F12C, 0xFFFFFFFF, 2048, 16, 0x00000008,  0x20000,  64,  0x08000000, 0x00433322, 0x00050005},
    { "MT29F2G08AAD",          5, 0x9580DA2C, 0xFFFFFFFF, 2048, 16, 0x00000001,  0x20000,  64,  0x10000000, 0x00433322, 0x00050005},
    { "MT29F2G08ABAE/FA",      5, 0x9590DA2C, 0xFFFFFFFF, 2048, 16, 0x00000001,  0x20000,  64,  0x10000000, 0x00433322, 0x00050005},
    { "MT29F4G08ABADA",        5, 0x9590DC2C, 0xFFFFFFFF, 2048, 16, 0x00000008,  0x20000,  64,  0x20000000, 0x00433322, 0x00050005},
    { "MT29F4G08ABAEA",        5, 0xA690DC2C, 0xFFFFFFFF, 4096, 26, 0x00000008,  0x40000,  64,  0x20000000, 0x00433322, 0x00050005},
    { "MT29F8G08ABABA",        5, 0x2600382C, 0xFFFFFFFF, 4096, 26, 0x00000008,  0x80000,  128, 0x40000000, 0x00433322, 0x00050005},
    { "MT29F16G08CBABA",       5, 0x4604482C, 0xFFFFFFFF, 4096, 26, 0x00000018,  0x100000, 256, 0x80000000, 0x00433322, 0x00050005},
    { "MT29F16G08CBACA",       5, 0x4A04482C, 0xFFFFFFFF, 4096, 26, 0x00000018,  0x100000, 256, 0x80000000, 0x00433322, 0x00050005},
    { "Micron 1Gb",            4, 0x0000F12C, 0x0000FFFF, 2048, 16, 0x00000001,  0x20000,  64,  0x08000000, 0x00411888, 0x00080008},
    { "Micron 2Gb",            5, 0x0000DA2C, 0x0000FFFF, 2048, 16, 0x00000001,  0x20000,  64,  0x10000000, 0x00411888, 0x00080008},
    { "Micron 16GB",           5, 0x0000D92C, 0x0000FFFF, 4096, 26, 0x00000004,  0x80000,  128, 0x80000000, 0x00411888, 0x00080008},

    { "TC58DVM92A5TAI0/J0",    4, 0x00007698, 0x0000FFFF, 512,  16, 0x00000501,  0x04000,  32,  0x04000000, 0x00433322, 0x00050005},
    { "TC58NVM9S3ETA00/I0",    4, 0x1500F098, 0xFFFFFFFF, 2048, 16, 0x00000001,  0x20000,  64,  0x04000000, 0x00433322, 0x00050005},
    { "TC58NVG0S3ETA00",       4, 0x1590D198, 0xFFFFFFFF, 2048, 16, 0x00000001,  0x20000,  64,  0x08000000, 0x00433322, 0x00050005},
    { "TC58NVG0S3ETA0B",       4, 0x1590F198, 0xFFFFFFFF, 2048, 16, 0x00000001,  0x20000,  64,  0x08000000, 0x00433322, 0x00050005},
    { "TC58NVG0S3HTA00",       4, 0x1580F198, 0xFFFFFFFF, 2048, 16, 0x00000001,  0x20000,  64,  0x08000000, 0x00433322, 0x00050005},
    { "TC58NVG1S3ETA00/I0",    5, 0x1590DA98, 0xFFFFFFFF, 2048, 16, 0x00000001,  0x20000,  64,  0x10000000, 0x00433322, 0x00050005},
    { "TC58NVG2S3ETA00/I0",    5, 0x1590DC98, 0xFFFFFFFF, 2048, 16, 0x00000001,  0x20000,  64,  0x20000000, 0x00433322, 0x00050005},
    { "TC58NVG2S0FTA00/I0",    5, 0x2690DC98, 0xFFFFFFFF, 4096, 26, 0x00000008,  0x40000,  64,  0x20000000, 0x00433322, 0x00050005},
    { "TC58D/NVG3S0E/FTA00",   5, 0x2690D398, 0xFFFFFFFF, 4096, 16, 0x00000008,  0x40000,  64,  0x40000000, 0x00433322, 0x00050005},
    { "TC58NVG4D2FTA00",       5, 0x3294D598, 0xFFFFFFFF, 8192, 26, 0x00000018,  0x100000, 128, 0x80000000, 0x00433322, 0x00050005},
	  { "EN27LN2G08", 		   5, 0x9590DAC8, 0xFFFFFFFF, 2048, 16, 0x00000004,  0x20000,  64,	0x10000000, 0x00433322, 0x00050005},
    { "EN27LN4G08", 		    5, 0x9590DCC8, 0xFFFFFFFF, 2048, 16, 0x00000004,  0x20000,  64,	0x20000000, 0x00433322, 0x00050005},

    { "MX30LF1G08AM-TIB",      4, 0x1D80F1C2, 0xFFFFFFFF, 2048, 16, 0x00000001,  0x20000,  64,  0x08000000, 0x00433322, 0x00050005},
	{ "MX30LF4G28AB   ",	   5, 0x9590DCC2, 0xFFFFFFFF, 2048, 16, 0x00000001,  0x20000,  64,	0x20000000, 0x00433322, 0x00050005},
	{ "MX30LF2G18AC-TI",	   5, 0x9590DAC2, 0xFFFFFFFF, 2048, 16, 0x00000001,  0x20000,  64,	0x10000000, 0x00433322, 0x00050005},	
	{ "MX30LF1G18AC-TI",	   4, 0x9580F1C2, 0xFFFFFFFF, 2048, 16, 0x00000001,  0x20000,  64,	0x08000000, 0x00433322, 0x00050005},	
	{ "MX30LF1208AA   ",	   5, 0x1D80F0C2, 0xFFFFFFFF, 2048, 16, 0x00000001,  0x20000,  64,	0x04000000, 0x00433322, 0x00050005},

    { "S34ML08G100TFI000",     5, 0x95D1D301, 0xFFFFFFFF, 2048, 16, 0x00000001,  0x20000,  64,  0x40000000, 0x00433322, 0x00050005},
    { "S34ML04G1/200TFI000",   5, 0x9590DC01, 0xFFFFFFFF, 2048, 16, 0x00000001,  0x20000,  64,  0x20000000, 0x00433322, 0x00050005},
    { "S34ML02G100TFI000",     5, 0x9590DA01, 0xFFFFFFFF, 2048, 16, 0x00000001,  0x20000,  64,  0x10000000, 0x00433322, 0x00050005},
    { "S34ML01G100TFI000",     4, 0x1D00F101, 0xFFFFFFFF, 2048, 16, 0x00000001,  0x20000,  64,  0x08000000, 0x00433322, 0x00050005},
    { "S34ML01G200TFI000",     4, 0x1D80F101, 0xFFFFFFFF, 2048, 16, 0x00000004,  0x20000,  64,  0x08000000, 0x00433322, 0x00050005},

    { "Small 16MB",            3, 0x00007300, 0x0000FF00, 512,  16, 0x00000501,  0x04000,  32,  0x01000000, 0x00411888, 0x00080008},
    { "Small 32MB",            3, 0x00007500, 0x0000FF00, 512,  16, 0x00000501,  0x04000,  32,  0x02000000, 0x00411888, 0x00080008},
    { "Small 64MB",            4, 0x00007600, 0x0000FF00, 512,  16, 0x00000501,  0x04000,  32,  0x04000000, 0x00411888, 0x00080008},
    { "Small 128MB",           4, 0x00007900, 0x0000FF00, 512,  16, 0x00000501,  0x04000,  32,  0x08000000, 0x00411888, 0x00080008},
    { "Large 128MB",           4, 0x0000F100, 0x0000FF00, 2048, 16, 0x00000001,  0x20000,  64,  0x08000000, 0x00411888, 0x00080008},
};

#ifdef CC_MTD_ENCRYPT_SUPPORT
extern BOOL NAND_AES_INIT(void);
extern BOOL NAND_AES_Encryption(UINT32 u4InBufStart, UINT32 u4OutBufStart, UINT32 u4BufSize);
extern BOOL NAND_AES_Decryption(UINT32 u4InBufStart, UINT32 u4OutBufStart, UINT32 u4BufSize);
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

    return (((NAND_FLASH_DEV_T*)_arNand_DevInfo) + u4Idx);
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
#if defined(CC_MT5890) || defined(CC_MT5891) || defined(CC_MT5891)
//-----------------------------------------------------------------------------
/** _NANDHW_SetEptNum()
 */
//-----------------------------------------------------------------------------
static void _NANDHW_SetEptNum(UINT32 u4EmptyNum)
{
#ifndef TOOLDRIVER
     NAND_WRITE32(NAND_NFI_FIFOSTA,NAND_NFI_EMPTY_BIT_NUM(u4EmptyNum));
#endif
}
#endif

//-----------------------------------------------------------------------------
/** _NANDHW_WaitRead()
 */
//-----------------------------------------------------------------------------
static void _NANDHW_WaitRead(UINT32 u4WaitNum)
{
    UINT32 u4Val;

    do
    {
        u4Val = NAND_READ32(NAND_NFI_FIFOSTA);
        u4Val &= NAND_NFI_FIFOSTA_RD_REMAIN;
    } while (u4Val < u4WaitNum);
}

//-----------------------------------------------------------------------------
/** _NANDHW_WaitWrite()
 */
//-----------------------------------------------------------------------------
static void _NANDHW_WaitWrite(void)
{
    UINT32 u4Val;

    do
    {
        u4Val = NAND_READ32(NAND_NFI_FIFOSTA);
        u4Val &= NAND_NFI_FIFOSTA_WT_EMPTY;
    } while (u4Val == 0);
}

//-----------------------------------------------------------------------------
/** _NANDHW_WaitEccDone() must wait done
 */
//-----------------------------------------------------------------------------
static void _NANDHW_WaitEccDone(UINT32 u4SectIdx)
{
    UINT32 u4Val;

    do
    {
        u4Val = NAND_READ32(NAND_NFIECC_DECDONE);
        u4Val = (u4Val >> u4SectIdx) & 0x1;
    } while (u4Val == 0);
}
#if CC_NAND_WDLE_CFG
//-----------------------------------------------------------------------------
/** _NANDHW_CheckWDLE() must wait done
 */
//-----------------------------------------------------------------------------
static void _NANDHW_CheckWDLE(void)
{
    UINT32 u4Val;

    do
    {
        u4Val = NAND_IO_READ32(NAND_NFI_MLC_CFG);
        u4Val = (u4Val >> 16) & 0x3FF;
    } while (u4Val != 0);
}
#endif
//-----------------------------------------------------------------------------
/** _NANDHW_WaitBusy()
 */
//-----------------------------------------------------------------------------
static void _NANDHW_WaitBusy(void)
{
    u_long timeo;
    UINT32 u4Val;

    timeo = jiffies + HZ;
    do
    {
        if (time_after(jiffies, timeo))
        {
            u4Val = NAND_READ32(NAND_NFI_STA);
            u4Val &= 0xFF000DFF;
            if (u4Val == 0)
            {
                break;
            }

            printk(KERN_ERR "NAND driver wait timeout! NAND_NFI_STA = 0x%X\n", (unsigned int)u4Val);
        }

        u4Val = NAND_READ32(NAND_NFI_STA);
        u4Val &= 0xFF000DFF;
    } while (u4Val != 0);
}

//-----------------------------------------------------------------------------
/** _NANDHW_NFIECC_WaitBusy()
 */
//-----------------------------------------------------------------------------
static void _NANDHW_NFIECC_WaitBusy(void)
{
    UINT32 u4Val;

    do
    {
        u4Val = NAND_READ32(NAND_NFIECC_ENCIDLE);
    } while (u4Val == 0);

    do
    {
        u4Val = NAND_READ32(NAND_NFIECC_DECIDLE);
    } while (u4Val == 0);
}

//-----------------------------------------------------------------------------
/** _NANDHW_NFI_WaitSectorCnt()
 */
//-----------------------------------------------------------------------------
static void _NANDHW_NFI_WaitSectorCnt(void)
{
    UINT32 u4Val, u4Cnt;

    do
    {
        u4Val = NAND_READ32(NAND_NFI_ADDRCNTR);
#if defined(CC_MT5890) || defined(CC_MT5891) || defined(CC_MT5891)
        u4Val &= 0xF800;
#else
        u4Val &= 0xF000;
#endif
        u4Cnt = NAND_NFI_ADDRCNTR_SEC_CNTR(_u4SectorNum);
    } while (u4Val != u4Cnt);
}

//-----------------------------------------------------------------------------
/** _NANDHW_Command()
 */
//-----------------------------------------------------------------------------
static void _NANDHW_Command(UINT32 u4Cmd)
{
    NAND_WRITE32(NAND_NFI_CMD, u4Cmd);
    _NANDHW_WaitBusy();
}

//-----------------------------------------------------------------------------
/** _NANDHW_Addr()
 */
//-----------------------------------------------------------------------------
static void _NANDHW_Addr(UINT32 u4RowAddr, UINT32 u4ColAddr, UINT32 u4ByteCount)
{
    if (_prFlashDev != NULL)
    {
       if (u4RowAddr >= (_prFlashDev->u4ChipSize/_prFlashDev->u4PageSize))
       {
          printk(KERN_CRIT "page index = 0x%x exceeds nand flash page range, page range from 0 to 0x%x\n",
                (unsigned int)u4RowAddr, (unsigned int)(_prFlashDev->u4ChipSize/_prFlashDev->u4PageSize - 1));
          BUG();
       }
    }
    NAND_WRITE32(NAND_NFI_COLADDR, u4ColAddr);
    NAND_WRITE32(NAND_NFI_ROWADDR, u4RowAddr);
    NAND_WRITE32(NAND_NFI_ADDRNOB, u4ByteCount);
    _NANDHW_WaitBusy();
}

//-----------------------------------------------------------------------------
/** _NANDHW_NFI_Reset()
 */
//-----------------------------------------------------------------------------
static void _NANDHW_NFI_Reset(void)
{
    NAND_WRITE32(NAND_NFI_CNFG, 0);
    NAND_WRITE32(NAND_NFI_CON, NAND_NFI_CON_NFI_RST | NAND_NFI_CON_FIFO_FLUSH);
    _NANDHW_WaitBusy();
}

//-----------------------------------------------------------------------------
/** NANDHW_Reset()
 */
//-----------------------------------------------------------------------------
void NANDHW_Reset(void)
{
    _NANDHW_NFI_Reset();
    _NANDHW_Command(NAND_CMD_RESET);

    _NANDHW_WaitBusy();
}

//-----------------------------------------------------------------------------
/** _NANDHW_NFIECC_Reset()
 */
//-----------------------------------------------------------------------------
static void _NANDHW_NFIECC_Reset(void)
{
    NAND_WRITE32(NAND_NFIECC_ENCON, 0);
    NAND_WRITE32(NAND_NFIECC_DECCON, 0);
    _NANDHW_NFIECC_WaitBusy();
}

//-----------------------------------------------------------------------------
/** _NANDHW_NFI_SetPageFmt()
 */
//-----------------------------------------------------------------------------
static void _NANDHW_NFI_SetPageFmt(void)
{
    UINT32 u4Val = 0;
    _u4FdmNum = 8;

#if CC_NAND_60BIT_NFI
    if (_u4OOBSize == 16)
    {
        u4Val = NAND_NFI_PAGEFMT_SPARE_16;
        if (_u4PageSize == 512)
        {
            u4Val |= NAND_NFI_PAGEFMT_PAGE_SIZE_512_2k;
            u4Val |= NAND_NFI_PAGEFMT_SECTOR_SIZE_512_16;
        }
        else if (_u4PageSize == 2048)
        {
            u4Val |= NAND_NFI_PAGEFMT_PAGE_SIZE_512_2k;
            u4Val |= NAND_NFI_PAGEFMT_SECTOR_SIZE_1024_32;
        }
        else if (_u4PageSize == 4096)
        {
            u4Val |= NAND_NFI_PAGEFMT_PAGE_SIZE_4k;
            u4Val |= NAND_NFI_PAGEFMT_SECTOR_SIZE_1024_32;
        }
        else if (_u4PageSize == 8192)
        {
            u4Val |= NAND_NFI_PAGEFMT_PAGE_SIZE_8k;
            u4Val |= NAND_NFI_PAGEFMT_SECTOR_SIZE_1024_32;
        }
        else if (_u4PageSize == 16384)
        {
            u4Val |= NAND_NFI_PAGEFMT_PAGE_SIZE_16k;
            u4Val |= NAND_NFI_PAGEFMT_SECTOR_SIZE_1024_32;
        }
    }
    else if (_u4OOBSize == 26)
    {
        u4Val = NAND_NFI_PAGEFMT_SPARE_26;
        u4Val |= NAND_NFI_PAGEFMT_SECTOR_SIZE_1024_52;
    }
    else if (_u4OOBSize == 32)
    {
        u4Val = NAND_NFI_PAGEFMT_SPARE_32;
        u4Val |= NAND_NFI_PAGEFMT_SECTOR_SIZE_1024_64;
    }
    else if (_u4OOBSize == 36)
    {
        u4Val = NAND_NFI_PAGEFMT_SPARE_36;
        u4Val |= NAND_NFI_PAGEFMT_SECTOR_SIZE_1024_72;
    }
    else if (_u4OOBSize == 40)
    {
        u4Val = NAND_NFI_PAGEFMT_SPARE_40;
        u4Val |= NAND_NFI_PAGEFMT_SECTOR_SIZE_1024_80;
    }
    else if (_u4OOBSize == 64)
    {
        u4Val = NAND_NFI_PAGEFMT_SPARE_64;
        u4Val |= NAND_NFI_PAGEFMT_SECTOR_SIZE_1024_128;
    }

    if (_u4PageSize == 4096)
    {
        u4Val |= NAND_NFI_PAGEFMT_PAGE_SIZE_4k;
    }
    else if (_u4PageSize == 8192)
    {
        u4Val |= NAND_NFI_PAGEFMT_PAGE_SIZE_8k;
    }
    else if (_u4PageSize == 16384)
    {
        u4Val |= NAND_NFI_PAGEFMT_PAGE_SIZE_16k;
    }
#else
    if (_u4OOBSize == 16)
    {
        u4Val = NAND_NFI_PAGEFMT_SPARE_16;
    }
    else if (_u4OOBSize == 27)
    {
        u4Val = NAND_NFI_PAGEFMT_SPARE_27;
    }
    else if (_u4OOBSize == 28)
    {
        u4Val = NAND_NFI_PAGEFMT_SPARE_28;
    }
    else
    {
        u4Val = NAND_NFI_PAGEFMT_SPARE_26;
    }
#if defined(CC_MT5881)
    if (_u4PageSize == 2048)
    {
        u4Val |= NAND_NFI_PAGEFMT_PAGE_SIZE_2k;
    }
    else if (_u4PageSize == 4096)
    {
        u4Val |= NAND_NFI_PAGEFMT_PAGE_SIZE_4k;
    }
#else
    if (_u4PageSize == 4096)
    {
        u4Val |= NAND_NFI_PAGEFMT_PAGE_SIZE_4k;
    }
    else if (_u4PageSize == 8192)
    {
        u4Val |= NAND_NFI_PAGEFMT_PAGE_SIZE_8k;
    }
#endif
    else
    {
        u4Val |= NAND_NFI_PAGEFMT_PAGE_SIZE_512_2k;
    }
#endif
#if defined(CC_MT5881)
    u4Val |= ((_u4FdmNum & 0xF)<<8) | ((_u4FdmNum & 0xF)<<12);
#else
    u4Val |= NAND_NFI_PAGEFMT_FDM_NUM(8) | NAND_NFI_PAGEFMT_ECC_NUM(8);
#endif
    NAND_WRITE32(NAND_NFI_PAGEFMT, u4Val);
}

//-----------------------------------------------------------------------------
/** _NANDHW_NFI_Cfg()
 */
//-----------------------------------------------------------------------------
static void _NANDHW_NFI_Cfg(int rECCType, BOOL fgDmaEn, UINT32 u4OpMode)
{
    UINT32 u4Val = 0;

    if(rECCType == NAND_ECC_HW)
    {
        u4Val = NAND_NFI_CNFG_AUTO_FMT_EN | NAND_NFI_CNFG_HW_ECC_EN;
    }
    else if(rECCType == NAND_ECC_SOFT)
    {
        u4Val = NAND_NFI_CNFG_HW_ECC_EN;
    }
    else
    {
        u4Val = 0;
    }

    if(fgDmaEn)
    {
        u4Val |= (NAND_NFI_CNFG_AUTO_FMT_EN | NAND_NFI_CNFG_AHB_MODE);
    }

    switch(u4OpMode)
    {
    case NAND_NFI_CNFG_OP_IDLE:
        NAND_WRITE32(NAND_NFI_CNFG, 0);
        return;

    case NAND_NFI_CNFG_OP_SINGLE:
    case NAND_NFI_CNFG_OP_READ:
        u4Val |= (NAND_NFI_CNFG_READ_MODE | u4OpMode);
        break;

    case NAND_NFI_CNFG_OP_PROGRAM:
    case NAND_NFI_CNFG_OP_ERASE:
    case NAND_NFI_CNFG_OP_RESET:
    case NAND_NFI_CNFG_OP_CUSTOM:
        u4Val |= u4OpMode;
        break;

    default:
        break;
    }

    if(_u4PageSize == 512)
    {
        u4Val |= NAND_NFI_CNFG_SEL_SEC_512BYTE;
    }

    NAND_WRITE32(NAND_NFI_CNFG, u4Val);
}

//-----------------------------------------------------------------------------
/** _NANDHW_NFIECC_Cfg()
 */
//-----------------------------------------------------------------------------
static void _NANDHW_NFIECC_Cfg(int rECCType)
{
    UINT32 u4EncMsg = 0, u4DecMsg = 0;

    if (_u4OOBSize == 16)
    {
        if (_u4PageSize == 512)
        {
            _u4ECCBits = 4;
            u4EncMsg = NAND_NFIECC_ENCCNFG_ENC_TNUM_4 | NAND_NFIECC_ENCCNFG_ENC_MS_520;
            u4DecMsg = NAND_NFIECC_DECCNFG_DEC_TNUM_4 | NAND_NFIECC_DECCNFG_DEC_CS_520_4;
        }
        else
        {
            _u4ECCBits = 12;
            u4EncMsg = NAND_NFIECC_ENCCNFG_ENC_TNUM_12 | NAND_NFIECC_ENCCNFG_ENC_MS_1032;
            u4DecMsg = NAND_NFIECC_DECCNFG_DEC_TNUM_12 | NAND_NFIECC_DECCNFG_DEC_CS_1032_12;
        }
    }
    else if(_u4OOBSize == 26)
    {
        if (_u4PageSize == 512)
        {
            _u4ECCBits = 10;
            u4EncMsg = NAND_NFIECC_ENCCNFG_ENC_TNUM_10 | NAND_NFIECC_ENCCNFG_ENC_MS_520;
            u4DecMsg = NAND_NFIECC_DECCNFG_DEC_TNUM_10 | NAND_NFIECC_DECCNFG_DEC_CS_520_10;
        }
        else
        {
            _u4ECCBits = 24;
            u4EncMsg = NAND_NFIECC_ENCCNFG_ENC_TNUM_24 | NAND_NFIECC_ENCCNFG_ENC_MS_1032;
            u4DecMsg = NAND_NFIECC_DECCNFG_DEC_TNUM_24 | NAND_NFIECC_DECCNFG_DEC_CS_1032_24;
        }
    }
#if CC_NAND_60BIT_NFI
    else if(_u4OOBSize == 32)
    {
         _u4ECCBits = 32;
         u4EncMsg = NAND_NFIECC_ENCCNFG_ENC_TNUM_32 | NAND_NFIECC_ENCCNFG_ENC_MS_1032;
         u4DecMsg = NAND_NFIECC_DECCNFG_DEC_TNUM_32 | NAND_NFIECC_DECCNFG_DEC_CS_1032_32;
    }
    else if(_u4OOBSize == 36)
    {
         _u4ECCBits = 36;
         u4EncMsg = NAND_NFIECC_ENCCNFG_ENC_TNUM_36 | NAND_NFIECC_ENCCNFG_ENC_MS_1032;
         u4DecMsg = NAND_NFIECC_DECCNFG_DEC_TNUM_36 | NAND_NFIECC_DECCNFG_DEC_CS_1032_36;
    }
    else if(_u4OOBSize == 40)
    {
         _u4ECCBits = 40;
         u4EncMsg = NAND_NFIECC_ENCCNFG_ENC_TNUM_40 | NAND_NFIECC_ENCCNFG_ENC_MS_1032;
         u4DecMsg = NAND_NFIECC_DECCNFG_DEC_TNUM_40 | NAND_NFIECC_DECCNFG_DEC_CS_1032_40;
    }
    else if(_u4OOBSize == 64)
    {
         _u4ECCBits = 60;
         u4EncMsg = NAND_NFIECC_ENCCNFG_ENC_TNUM_60 | NAND_NFIECC_ENCCNFG_ENC_MS_1032;
         u4DecMsg = NAND_NFIECC_DECCNFG_DEC_TNUM_60 | NAND_NFIECC_DECCNFG_DEC_CS_1032_60;
    }
#endif
    u4EncMsg |= NAND_NFIECC_ENCCNFG_ENC_NFI_MODE;
    u4DecMsg |= NAND_NFIECC_DECCNFG_DEC_CS_EMPTY_EN | NAND_NFIECC_DECCNFG_DEC_NFI_MODE;

    if(rECCType == NAND_ECC_HW)
    {
        u4DecMsg |= NAND_NFIECC_DECCNFG_DEC_CON_AUTO;
    }
    else if(rECCType == NAND_ECC_SOFT)
    {
        u4DecMsg |= NAND_NFIECC_DECCNFG_DEC_CON_SOFT;
    }
    else
    {
        u4DecMsg |= NAND_NFIECC_DECCNFG_DEC_CON_NONE;
    }

    NAND_WRITE32(NAND_NFIECC_ENCCNFG, u4EncMsg);
    NAND_WRITE32(NAND_NFIECC_DECCNFG, u4DecMsg);
}

//-----------------------------------------------------------------------------
/** _NANDHW_NFI_Trig()
 */
//-----------------------------------------------------------------------------
static void _NANDHW_NFI_Trig(UINT32 u4OpMode)
{
    UINT32 u4Val;

    switch(u4OpMode)
    {
    case NAND_NFI_CNFG_OP_SINGLE:
        u4Val = NAND_NFI_CON_NOB(4) | NAND_NFI_CON_SRD;
        break;

    case NAND_NFI_CNFG_OP_READ:
#if NAND_AUTORD_DONE
        u4Val = NAND_NFI_CON_SEC_NUM(_u4SectorNum) | NAND_NFI_CON_BRD | NAND_NFI_CON_BRD_HW_EN;
#else
        u4Val = NAND_NFI_CON_SEC_NUM(_u4SectorNum) | NAND_NFI_CON_BRD;
#endif
        break;

    case NAND_NFI_CNFG_OP_PROGRAM:
        u4Val = NAND_NFI_CON_SEC_NUM(_u4SectorNum) | NAND_NFI_CON_BWR;
        break;

    case NAND_NFI_CNFG_OP_CUSTOM:
        u4Val = NAND_NFI_CON_SEC_NUM(_u4SectorNum) | NAND_NFI_CON_BWR;
        NAND_WRITE32(NAND_NFI_CON, u4Val);
        NAND_WRITE32(NAND_NFI_STRDATA, NAND_NFI_STRDATA_STRDATA);
        return;

    default:
        u4Val = 0;
        break;
    }

    NAND_WRITE32(NAND_NFI_CON, u4Val);
}

//-----------------------------------------------------------------------------
/** _NANDHW_NFIECC_Trig()
 */
//-----------------------------------------------------------------------------
static void _NANDHW_NFIECC_Trig(UINT32 u4OpMode)
{
    switch(u4OpMode)
    {
    case NAND_NFI_CNFG_OP_READ:
        NAND_WRITE32(NAND_NFIECC_DECCON, 1);
        break;

    case NAND_NFI_CNFG_OP_PROGRAM:
        NAND_WRITE32(NAND_NFIECC_ENCON, 1);
        break;

    default:
        break;
    }
}

//-----------------------------------------------------------------------------
/** _NANDHW_NFI_CheckOpMode()
 */
//-----------------------------------------------------------------------------
static INT32 _NANDHW_NFI_CheckOpMode(UINT32 u4OpMode)
{
    UINT32 i, u4Val;

    for(i = 0; i < 3; i++)
    {
        switch(u4OpMode)
        {
        case NAND_NFI_CNFG_OP_READ:
            u4Val = NAND_READ32(NAND_NFI_STA);
            u4Val &= NAND_NFI_STA_NFI_FSM;
            if(u4Val == NAND_NFI_STA_NFI_FSM_READ_DATA)
            {
                return NAND_SUCC;
            }
            break;

        default:
            return NAND_INVALID_ARG;
        }
    }

    return NAND_CMD_ERROR;
}
#if NAND_RANDOM_EN

//-----------------------------------------------------------------------------
/** NANDHW_Random_Mizer_Encode()
 */
//-----------------------------------------------------------------------------
void NANDHW_Random_Mizer_Encode(UINT32 u4PgIdx, BOOL fgRandom)
{
    UINT32 u4Val;

    if (fgRandom)
    {
        NAND_WRITE32(NAND_NFI_RANDOM_CFG, 0);
        u4Val = NAND_NFI_ENCODING_RANDON_SEED(RANDOM_SEED[u4PgIdx%_u4BlkPgCount]) | NAND_NFI_ENCODING_RANDON_EN;
        NAND_WRITE32(NAND_NFI_RANDOM_CFG, u4Val);
    }
    else
    {
        NAND_WRITE32(NAND_NFI_RANDOM_CFG, 0);
    }
}
//-----------------------------------------------------------------------------
/** NANDHW_Random_Mizer_Decode()
 */
//-----------------------------------------------------------------------------
void NANDHW_Random_Mizer_Decode(UINT32 u4PgIdx, BOOL fgRandom)
{
    UINT32 u4Val;

    if (fgRandom)
    {
        NAND_WRITE32(NAND_NFI_RANDOM_CFG, 0);
        u4Val = NAND_NFI_DECODING_RANDON_SEED(RANDOM_SEED[u4PgIdx%_u4BlkPgCount]) | NAND_NFI_DECODING_RANDON_EN;
        NAND_WRITE32(NAND_NFI_RANDOM_CFG, u4Val);
    }
    else
    {
        NAND_WRITE32(NAND_NFI_RANDOM_CFG, 0);
    }
}
#endif
//-----------------------------------------------------------------------------
/** _NANDHW_ReadID()
 */
//-----------------------------------------------------------------------------
static void _NANDHW_ReadID(UINT32 *pu4ID)
{
    _NANDHW_NFI_Reset();

    _NANDHW_Command(NAND_CMD_READID);
    _NANDHW_Addr(0, 0, 1);

    _NANDHW_NFI_Cfg(NAND_ECC_NONE, 0, NAND_NFI_CNFG_OP_SINGLE);
    _NANDHW_NFI_Trig(NAND_NFI_CNFG_OP_SINGLE);

    _NANDHW_WaitBusy();
    _NANDHW_WaitRead(4);
    *pu4ID = NAND_READ32(NAND_NFI_DATAR);
}

//to check if this flash supports ONFI
static BOOL NANDHW_ReadONFISig(void)
{
    UINT32 u4Sig;

    _NANDHW_NFI_Reset();

    _NANDHW_Command(NAND_CMD_READID);
    _NANDHW_Addr(0, 0x20, 1);
    _NANDHW_NFI_Cfg(NAND_ECC_NONE, 0, NAND_NFI_CNFG_OP_SINGLE);
    _NANDHW_NFI_Trig(NAND_NFI_CNFG_OP_SINGLE);
    _NANDHW_WaitBusy();

    _NANDHW_WaitRead(4);
    u4Sig = NAND_READ32(NAND_NFI_DATAR);

    if(0x49464E4F == u4Sig)
    {
        printk(KERN_INFO "Support ONFI! \n");
        return TRUE;
    }
    else
    {
        printk(KERN_INFO "Do Not Support ONFI!(0x%x) \n", u4Sig);
        return FALSE;
    }
}

//-----------------------------------------------------------------------------
/** NANDHW_ReadIDEx()
 */
//-----------------------------------------------------------------------------
static UINT32 NANDHW_ReadIDEx(UINT32 *pu4ID)
{
    _NANDHW_NFI_Reset();

    _NANDHW_Command(NAND_CMD_READID);
    NAND_WRITE32(NAND_NFI_COLADDR, 0);
    NAND_WRITE32(NAND_NFI_ROWADDR, 0);
    NAND_WRITE32(NAND_NFI_ADDRNOB, 1);
    _NANDHW_WaitBusy();
    _NANDHW_NFI_Cfg(NAND_ECC_NONE, 0, NAND_NFI_CNFG_OP_SINGLE);
	  NAND_WRITE32(NAND_NFI_CON,  NAND_NFI_CON_NOB(6) | NAND_NFI_CON_SRD);

    _NANDHW_WaitBusy();

    _NANDHW_WaitRead(4);
    *pu4ID  = NAND_READ32(NAND_NFI_DATAR);
	  _NANDHW_WaitRead(2);
    return NAND_READ32(NAND_NFI_DATAR);
}
//-----------------------------------------------------------------------------
/** _NANDHW_NFI_WriteFifo()
 */
//-----------------------------------------------------------------------------
static void _NANDHW_NFI_WriteFifo(UINT8 *pu1Buf, UINT32 u4Len)
{
    UINT32 i;
    UINT32 *pu4Buf = (UINT32 *)pu1Buf;

    for(i = 0; i < (u4Len>>2); i++)
    {
        _NANDHW_WaitWrite();
        if(pu1Buf)
        {
            NAND_WRITE32(NAND_NFI_DATAW, pu4Buf[i]);
        }
        else
        {
            NAND_WRITE32(NAND_NFI_DATAW, (UINT32)0xFFFFFFFF);
        }
    }
}

//-----------------------------------------------------------------------------
/** _NANDHW_DataExchange()
 */
//-----------------------------------------------------------------------------
static INT32 _NANDHW_DataExchange(UINT32 *pu4Buf, UINT32 *pu4Spare, UINT32 u4PgOff, UINT32 u4Len)
{
    UINT32 u4Val, u4Addr;
    if(!pu4Buf)
    {
        return NAND_FAIL;
    }

    if(_u4OOBSize == 16)
    {
        if(_u4PageSize == 0x800)
        {
            u4Addr = 0x7E0;
        }
        else if(_u4PageSize == 0x1000)
        {
            u4Addr = 0xFA0;
        }
        else if(_u4PageSize == 0x2000)
        {
            u4Addr = 0x1F20;
        }
        else if (_u4PageSize == 0x4000)
        {
            u4Addr = 0x3E20;
        }
        else
        {
            return NAND_SUCC;
        }
    }
    else if(_u4OOBSize == 26)
    {
        if(_u4PageSize == 0x800)
        {
            u4Addr = 0x7CC;
        }
        else if(_u4PageSize == 0x1000)
        {
            u4Addr = 0xF64;
        }
        else if(_u4PageSize == 0x2000)
        {
            u4Addr = 0x1E94;
        }
        else if (_u4PageSize == 0x4000)
        {
            u4Addr = 0x3CF4;
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

    if((u4Addr >= u4PgOff) && (u4Addr < (u4PgOff + u4Len)))
    {
        u4Val = pu4Buf[(u4Addr-u4PgOff)>>2];
        pu4Buf[(u4Addr-u4PgOff)>>2] = pu4Spare[0];
        pu4Spare[0] = u4Val;
    }

    return NAND_SUCC;
}

//-----------------------------------------------------------------------------
/** _NANDHW_ProbeDev()
 */
//-----------------------------------------------------------------------------
static INT32 _NANDHW_ProbeDev(void)
{
    NAND_FLASH_DEV_T *prDev;
    UINT32 u4ID, i, u4DevCount;

    _NANDHW_ReadID(&u4ID);
    printk(KERN_INFO "Detect NAND flash ID: 0x%X\n", (unsigned int)u4ID);

    u4DevCount = _NANDHW_GetDevCount();
    for(i = 0; i < u4DevCount; i++)
    {
        prDev = (NAND_FLASH_DEV_T*)_NANDHW_GetDev(i);
        if((u4ID & prDev->u4IDMask) == (prDev->u4IDVal& prDev->u4IDMask))
        {
            _prFlashDev = prDev;
            if(0x1590DA98 == u4ID)
            {
                if(0x00001676 == NANDHW_ReadIDEx(&u4ID))
                {
                    _prFlashDev->u4OOBSize = 26;
                    _prFlashDev->u4BBMode  = 0x00000008;
                    printk(KERN_INFO "Detect NAND flash ExID: 0x%X\n", 0x00001676);
                }
            }
            else if(0x9590DA2C == u4ID)
            {
                if(0x00000004 == NANDHW_ReadIDEx(&u4ID))
                {
                    _prFlashDev->u4OOBSize = 26;
                    _prFlashDev->u4BBMode  = 0x00000008;
                    printk(KERN_INFO "Detect NAND flash ExID: 0x%X\n", 0x00000004);
                }
            }
            else if(0x9590DCAD == u4ID)
            {
                if(0x00000056 == (0x000000FF & NANDHW_ReadIDEx(&u4ID)))
                {
                    _prFlashDev->u4OOBSize = 26;
                    _prFlashDev->u4BBMode  = 0x00000008;
                    printk(KERN_INFO "Detect NAND flash ExID: 0x%X\n", 0x00000056);
                }
            }
            else if(0x9590DC01 == u4ID)
            {
                if(0x00000056 == (0x000000FF & NANDHW_ReadIDEx(&u4ID)))
                {
                    _prFlashDev->u4OOBSize = 26;
                    _prFlashDev->u4BBMode  = 0x00000008;
                    printk(KERN_INFO "Detect NAND flash ExID: 0x%X\n", 0x00000056);
                }
            }
            else if(0x1D80F1AD == u4ID)
            {
                if(NANDHW_ReadONFISig())
                {
                    _prFlashDev->acName[0] = 'H';
                    _prFlashDev->acName[1] = '2';
                    _prFlashDev->acName[2] = '7';
                    _prFlashDev->acName[3] = 'U';
                    _prFlashDev->acName[4] = '1';
                    _prFlashDev->acName[5] = 'G';
                    _prFlashDev->acName[6] = '8';
                    _prFlashDev->acName[7] = 'F';
                    _prFlashDev->acName[8] = '2';
                    _prFlashDev->acName[9] = 'C';
                    _prFlashDev->acName[10] = 'T';
                    _prFlashDev->acName[11] = 'R';

                    _prFlashDev->u4BBMode  = 0x00000004;
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
/** _NANDHW_SetSectorSize()
 */
//-----------------------------------------------------------------------------
static void _NANDHW_SetSectorSize(void)
{
    if(_u4PageSize > 512)
    {
        _u4SectorSize = 1024;
    }
    else
    {
        _u4SectorSize = 512;
    }
}

static void part_enc_init(void)
{
    UINT32 i, u4Size = 0, u4Offset = 0, u4Encrypt = 0;
    mtk_part_info_t *p;

    for (i = 0; i < MAX_MTD_DEVICES; i++)
    {
        p = &(rMTKPartInfo[i]);

        u4Offset = DRVCUST_InitGet((QUERY_TYPE_T)(eNANDFlashPartOffset0 + i));
        u4Size = DRVCUST_InitGet((QUERY_TYPE_T)(eNANDFlashPartSize0 + i));

#if defined(CC_MTD_ENCRYPT_SUPPORT)
        u4Encrypt = DRVCUST_InitGet((QUERY_TYPE_T)(eNANDFlashPartEncypt0 + i));
#endif

        p->u4PgStart = u4Offset/_u4PageSize;
        p->u4PgEnd = (u4Offset + u4Size)/_u4PageSize - 1;
        p->u4Encryped = u4Encrypt;
    }

#if defined(CC_MTD_ENCRYPT_SUPPORT)
    NAND_AES_INIT();
#endif
}

//-----------------------------------------------------------------------------
/** _NANDHW_Init()
 */
//-----------------------------------------------------------------------------
static INT32 _NANDHW_Init(void)
{
    UINT32 i, u4Val;

    // Set nand/nor pinmux.
    u4Val = NAND_READ32(NAND_NFI_MISC);
    u4Val |= NAND_NFI_MISC_FLASH_PMUX;
    NAND_WRITE32(NAND_NFI_MISC, u4Val);

    _u4BlkPgCount = 64;
    _prFlashDev = NULL;
    _u4PgAddrLen  = 3;
    _u4TotalAddrLen = 5;
    _u4PageSize = 2048;
    _u4OOBSize = 16;
#if defined(CC_MT5890) || defined(CC_MT5891) || defined(CC_MT5891)
#if NAND_RANDOM_EN
		if(0x00400000 == (NAND_IO_READ32(0xf0008664)&0x00400000))//5399 randomizer efuse bit
			fgRandomizer = TRUE;
		else
			fgRandomizer = FALSE;
#endif
#endif

    for(i = 0; i < 3; i++)
    {
        // Reset NFI controller
        NANDHW_Reset();

        if(_NANDHW_ProbeDev() == NAND_SUCC)
        {
            printk(KERN_INFO "Detect %s NAND flash: %dMB, randomizer: %d \n",
                _prFlashDev->acName, (int)(_prFlashDev->u4ChipSize/0x100000), fgRandomizer);

            _u4PageSize = _prFlashDev->u4PageSize;
            _u4OOBSize =  _prFlashDev->u4OOBSize;

            _NANDHW_SetSectorSize();
            _NANDHW_NFI_Cfg(NAND_ECC_NONE, 0, NAND_NFI_CNFG_OP_SINGLE);
            _NANDHW_NFIECC_Cfg(NAND_ECC_NONE);

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
            break;
        }

        printk(KERN_ERR "Not detect any flash!\n");
        _u4PgAddrLen = 2;
        _u4TotalAddrLen = 5;
        _u4PageSize = 2048;
        _u4OOBSize = 16;
    }

    _NANDHW_NFI_SetPageFmt();
    _u4SectorNum = _u4PageSize/_u4SectorSize;

    // Aligned the buffer for DMA used.
    _pu1NandDataBuf = (UINT8 *)((((UINT32)_u4NandDataBuf >> 6) << 6) + 64);
    _pu1NandOobBuf  = (UINT8 *)_u4NandOobBuf;

    part_enc_init();

    return NAND_SUCC;
}

//-----------------------------------------------------------------------------
/** _NANDHW_NFIECC_GetEcc()
 */
//-----------------------------------------------------------------------------
static UINT32 _NANDHW_NFIECC_GetEcc(UINT32 *pu4EccCode)
{
    UINT32 u4Val, i;
    UINT32 u4EccCodeBit, u4EccCodeDWord, u4EccModBit;

    // the redundant bit of parity bit must be 1 for rom code bug. -- 20110328 mtk40109
    u4EccCodeBit = _u4ECCBits * NAND_NFIECC_1BIT;
    u4EccModBit = u4EccCodeBit % 32;
    u4EccCodeDWord = u4EccModBit ? ((u4EccCodeBit>>5) + 1) : (u4EccCodeBit>>5);

    do
    {
        u4Val = NAND_READ32(NAND_NFIECC_ENCSTA);
        u4Val &= NAND_NFIECC_ENCSTA_FSM_MASK;
    } while (u4Val != NAND_NFIECC_ENCSTA_FSM_PAROUT);

    for (i = 0; i < u4EccCodeDWord; i++)
    {
        pu4EccCode[i] = NAND_READ32(NAND_NFIECC_ENCPAR0 + (i<<2));
    }

    for (i = u4EccCodeDWord; i < NAND_NFI_ENCPAR_NUM; i++)
    {
        pu4EccCode[i] = 0xFFFFFFFF;
    }

    if (u4EccModBit)
    {
        pu4EccCode[u4EccCodeDWord - 1] |= (~(UINT32)((1<<u4EccModBit) - 1));
    }

    return NAND_SUCC;
}

//-----------------------------------------------------------------------------
/** _NANDHW_NFIECC_GetErrNum()
 */
//-----------------------------------------------------------------------------
static INT32 _NANDHW_NFIECC_GetErrNum(UINT32 u4SectIdx, UINT32 *pu4ErrNum)
{
    UINT32 u4Val;

    if(u4SectIdx < 4)
    {
        u4Val = NAND_READ32(NAND_NFIECC_DECENUM0);
#if CC_NAND_60BIT_NFI
        *pu4ErrNum = (u4Val >> (u4SectIdx*8)) & 0x3F;
#else
        *pu4ErrNum = (u4Val >> (u4SectIdx*8)) & 0x1F;
#endif
    }
   	else if ((u4SectIdx >= 4) && (u4SectIdx < 8))
    {
        u4Val = NAND_READ32(NAND_NFIECC_DECENUM1);
#if CC_NAND_60BIT_NFI
        *pu4ErrNum = (u4Val >> ((u4SectIdx - 4)*8)) & 0x3F;
#else
        *pu4ErrNum = (u4Val >> ((u4SectIdx - 4)*8)) & 0x1F;
#endif
    }
    else if ((u4SectIdx >= 8) && (u4SectIdx < 12))
    {
        u4Val = NAND_READ32(NAND_NFIECC_DECENUM2);
        *pu4ErrNum = (u4Val >> ((u4SectIdx - 8)*8)) & 0x3F;
    }
    else
    {
        u4Val = NAND_READ32(NAND_NFIECC_DECENUM3);
        *pu4ErrNum = (u4Val >> ((u4SectIdx - 12)*8)) & 0x3F;
    }
#if CC_NAND_60BIT_NFI
    if (*pu4ErrNum == 0x3F)
    {
        return NAND_ECC_ERROR;
    }
#else
    if(*pu4ErrNum == 0x1F)
    {
        return NAND_ECC_ERROR;
    }
#endif
    else
    {
        return NAND_SUCC;
    }
}

static INT32 _NANDHW_NFIECC_CorrectEmptySector(UINT8 *buf, UINT8 *spare, UINT32 u4SectIdx)
{
    UINT32 i, u4Val, u4SectOobSz, u4bitflip = 0;
    UINT8 *pu1Data, *pu1Spare;
    UINT32 *pu4Data, *pu4Spare;

    u4Val = NAND_READ32(NAND_NFIECC_DECFER);
    if ((u4Val & (1 << u4SectIdx)) == 0)
    {
        return NAND_SUCC;
    }

    u4SectOobSz = (_u4OOBSize * _u4SectorSize)>>9;

    pu1Data = buf + u4SectIdx * _u4SectorSize;
    pu1Spare = spare + u4SectIdx * u4SectOobSz;
    pu4Data = (UINT32 *)pu1Data;
    pu4Spare = (UINT32 *)pu1Spare;

    for (i = 0; i < _u4SectorSize/4; i++)
    {
        u4bitflip += (32 - _NANDHW_countbits(pu4Data[i]));
    }

    for (i = 0; i < _u4FdmNum/4; i++)
    {
        u4bitflip += (32 - _NANDHW_countbits(pu4Spare[i]));
    }

    if (u4bitflip <= _u4ECCBits)
    {
        memset(pu1Data, 0xFF, _u4SectorSize);
        memset(pu1Spare, 0xFF, _u4FdmNum);

        //printk(KERN_INFO "Correctable ECC(Empty page bit flip): page = 0x%X, Sector = 0x%X\n", _u4CurPage, u4SectIdx);
        return NAND_SUCC;
    }
    #if NAND_RANDOM_EN
    else if( fgRandomizer && (NAND_READ32(NAND_NFI_FIFOSTA)&0x00010000) )
    {
  //     if(NAND_READ32(NAND_NFI_FIFOSTA)&0x00FC0000)
       //printk(KERN_ERR  "empty page check = 0x%08X, bit flip = 0x%08X \n", NAND_READ32(NAND_NFI_FIFOSTA), (NAND_READ32(NAND_NFI_FIFOSTA)&&0x00FC0000)>>18);
       memset(pu1Data, 0xFF, _u4SectorSize);
       memset(pu1Spare, 0xFF, _u4FdmNum);
       fgEmptyPage = TRUE;
       return NAND_SUCC;
    }
#endif
    else
    {
        fgEmptyPage = FALSE;
        printk(KERN_ERR "Un-Correctable ECC found: page = 0x%X, Sector = 0x%X\n",
                (unsigned int)_u4CurPage, (unsigned int)u4SectIdx);
        return NAND_ECC_ERROR;
    }
}

//-----------------------------------------------------------------------------
/** _NANDHW_NFIECC_CorrectEcc()
 */
//-----------------------------------------------------------------------------
static INT32 _NANDHW_NFIECC_CorrectEcc(UINT8 *buf, UINT8 *spare, UINT32 u4SectIdx)
{
    INT32 i4Ret;
    UINT32 i, u4Val, u4SpareOff, u4SectOobSz;
    UINT32 u4ErrNum, u4ErrFound, u4ErrByte, u4ErrBit;

    _NANDHW_WaitEccDone(u4SectIdx); // must wait done

    if (_NANDHW_NFIECC_GetErrNum(u4SectIdx, &u4ErrNum))
    {
        i4Ret = _NANDHW_NFIECC_CorrectEmptySector(buf, spare, u4SectIdx);
        return i4Ret;
    }

    u4SectOobSz = (_u4OOBSize * _u4SectorSize)>>9;
    u4ErrFound = NAND_READ32(NAND_NFIECC_DECFER);
    if((u4ErrFound >> u4SectIdx) & 0x01)
    {
        for(i = 0; i < u4ErrNum; i++)
        {
            u4Val = NAND_READ16(NAND_NFIECC_DECEL0 + (i*2));
            u4ErrByte = u4Val >> 3;
            u4ErrBit = u4Val & 0x7;

            if(u4ErrByte < _u4SectorSize)
            {
                // Data area bit error.
                u4ErrByte = (u4Val>>3) + (u4SectIdx*_u4SectorSize);
                buf[u4ErrByte] = buf[u4ErrByte] ^ (((UINT32)1)<<u4ErrBit);
            }
            else if(u4ErrByte < (u4SectOobSz+ _u4SectorSize))
            {
                // Spare area bit error.
                if(spare)
                {
                    u4SpareOff = u4ErrByte - _u4SectorSize + u4SectOobSz*u4SectIdx;
                    spare[u4SpareOff] = spare[u4SpareOff] ^ (((UINT32)1)<<u4ErrBit);
                }
            }
            else
            {
                printk(KERN_ERR  "Un-Correctable ECC found in page = 0x%X, Sector = 0x%X, Loc = 0x%X\n",
                    (unsigned int)_u4CurPage, (unsigned int)u4SectIdx, (unsigned int)u4Val);
                return NAND_ECC_ERROR;
            }
        }
    }

    return NAND_SUCC;
}

//-----------------------------------------------------------------------------
/** _NANDHW_NFIECC_CheckEccDone()
 */
//-----------------------------------------------------------------------------
static void _NANDHW_NFIECC_CheckEccDone(void)
{
    UINT32 u4SectIdx;

    for (u4SectIdx = 0; u4SectIdx < _u4SectorNum; u4SectIdx++)
    {
        _NANDHW_WaitEccDone(u4SectIdx);
    }
}

//-----------------------------------------------------------------------------
/** _NANDHW_NFIECC_CheckEcc()
 */
//-----------------------------------------------------------------------------
static INT32 _NANDHW_NFIECC_CheckEcc(UINT8 *buf, UINT8 *spare)
{
    UINT32 u4SectIdx, u4ErrNum;
    INT32 i4Ret = NAND_SUCC;

    for (u4SectIdx = 0; u4SectIdx < _u4SectorNum; u4SectIdx++)
    {
        if (_NANDHW_NFIECC_GetErrNum(u4SectIdx, &u4ErrNum))
        {
            i4Ret |= _NANDHW_NFIECC_CorrectEmptySector(buf, spare, u4SectIdx);
        }
        else if(u4ErrNum != 0)
        {
            //printk(KERN_INFO "Correctable ECC: %d bit-flip found in page 0x%X, Sector %d\n", u4ErrNum, _u4CurPage, u4SectIdx);
        }
    }
    return i4Ret;
}

//-----------------------------------------------------------------------------
/** _NANDHW_NFI_SetRdDma()
 */
//-----------------------------------------------------------------------------
static void _NANDHW_NFI_SetRdDma(UINT32 *pu4DataAddr)
{
    UINT32 u4PhyAddr;

    u4PhyAddr = virt_to_phys((UINT32)pu4DataAddr);
    u4PhyAddr += NAND_DRAM_BASE;
    NAND_WRITE32(NAND_NFI_STRADDR, u4PhyAddr);
    NAND_WRITE32(NAND_NFIECC_FDMADDR, NAND_REG_BASE + NAND_NFI_FDM0L);
}

//-----------------------------------------------------------------------------
/** _NANDHW_NFI_SetWtDma()
 */
//-----------------------------------------------------------------------------
static void _NANDHW_NFI_SetWtDma(UINT32 *pu4DataAddr, UINT32 *pu4SpareAddr)
{
    UINT32 i, j, u4SectOobSz, u4PhyAddr;
    UINT32 *pu4SectSpare;

    u4PhyAddr = virt_to_phys((UINT32)pu4DataAddr);
    u4PhyAddr += NAND_DRAM_BASE;
    NAND_WRITE32(NAND_NFI_STRADDR, u4PhyAddr);

    u4SectOobSz = (_u4OOBSize * _u4SectorSize)>>9;
    for (i = 0; i < _u4SectorNum; i++)
    {
        pu4SectSpare = (UINT32 *)((UINT32)pu4SpareAddr + u4SectOobSz * i);

        for (j = 0; j < _u4FdmNum/4; j++)
        {
            if(i < 8)
               NAND_WRITE32((NAND_NFI_FDM0L + 4*j+ NAND_NFI_FDM_LEN * i), pu4SectSpare[j]);
            else
               NAND_WRITE32((NAND_NFI_FDM8L + 4*j+ NAND_NFI_FDM_LEN * (i-8)), pu4SectSpare[j]);

        }
    }
}

//-----------------------------------------------------------------------------
/** _NANDHW_NFI_GetRdDma()
 */
//-----------------------------------------------------------------------------
static void _NANDHW_NFI_GetRdDma(UINT32 *pu4SpareAddr)
{
    UINT32 i, j, u4SectOobSz;
    UINT32 *pu4SectSpare;

    u4SectOobSz = (_u4OOBSize * _u4SectorSize)>>9;
    for (i = 0; i < _u4SectorNum; i++)
    {
        pu4SectSpare = (UINT32 *)((UINT32)pu4SpareAddr + u4SectOobSz * i);

        for (j = 0; j < _u4FdmNum/4; j++)
        {
            if(i < 8)
               pu4SectSpare[j] = NAND_READ32(NAND_NFI_FDM0L + 4*j + NAND_NFI_FDM_LEN * i);
            else
               pu4SectSpare[j] = NAND_READ32(NAND_NFI_FDM8L + 4*j + NAND_NFI_FDM_LEN * (i-8));

        }
    }
}

//-----------------------------------------------------------------------------
/** _NANDHW_NFI_ReadSpare()
 */
//-----------------------------------------------------------------------------
static INT32 _NANDHW_NFI_ReadSpare(UINT8 *pu1Buf, UINT32 u4Len, int rECCType)
{
    UINT32 i, u4Val = 0;
    UINT32 *pu4Buf = (UINT32 *)pu1Buf;
    INT32  i4Ret = NAND_SUCC;

    for (i = 0; i < u4Len; i += 4)
    {
        _NANDHW_WaitRead(4);
        pu4Buf[u4Val] = NAND_READ32(NAND_NFI_DATAR);
        u4Val++;
    }

    if (rECCType == NAND_ECC_SOFT)
    {
        if (u4Len ==  (_u4SectorSize + ((_u4OOBSize * _u4SectorSize)>>9)))
        {
            /* ECC correction. */
            if (_NANDHW_NFIECC_CorrectEcc(pu1Buf, &pu1Buf[_u4SectorSize], 0))
            {
                i4Ret |= NAND_ECC_ERROR;
            }
        }
    }

    _NANDHW_WaitBusy();
    return i4Ret;
}

//-----------------------------------------------------------------------------
/** _NANDHW_ReadSpare()
 */
//-----------------------------------------------------------------------------
static INT32 _NANDHW_ReadSpare(UINT8 *pu1Buf, UINT32 u4PgIdx, UINT32 u4PgOff, UINT32 u4Len, int rECCType)
{
    INT32 i4Ret = NAND_SUCC;

    _NANDHW_NFI_Reset();
    _NANDHW_NFIECC_Reset();

    _NANDHW_NFI_Cfg(rECCType, FALSE, NAND_NFI_CNFG_OP_READ);
    _NANDHW_NFIECC_Cfg(rECCType);
    _NANDHW_NFIECC_Trig(NAND_NFI_CNFG_OP_READ);

    /* Write command. */
    if ((u4PgOff == 0x200) && (_u4PageSize == 0x200))
    {
        _NANDHW_Command(NAND_CMD_READOOB);
        _NANDHW_Addr(u4PgIdx, 0, (_u4PgAddrLen<<4)|(_u4TotalAddrLen - _u4PgAddrLen));
    }
    else
    {
        _NANDHW_Command(NAND_CMD_READ0);
        _NANDHW_Addr(u4PgIdx, u4PgOff, (_u4PgAddrLen<<4)|(_u4TotalAddrLen - _u4PgAddrLen));
    }

    if (_u4PageSize != 0x200)
    {
        _NANDHW_Command(NAND_CMD_READSTART);
    }
    _NANDHW_WaitBusy();

    /*Trigger Read . */
    i4Ret = _NANDHW_NFI_CheckOpMode(NAND_NFI_CNFG_OP_READ);
    if (i4Ret)
    {
        goto HandleError;
    }

    _NANDHW_NFI_Trig(NAND_NFI_CNFG_OP_READ);

    _NANDHW_WaitBusy();
    i4Ret = _NANDHW_NFI_ReadSpare(pu1Buf, u4Len, rECCType);
    if (i4Ret)
    {
        goto HandleError;
    }

    _NANDHW_WaitBusy();
    NAND_WRITE32(NAND_NFI_CNFG, 0);
    return NAND_SUCC;

HandleError:
    _NANDHW_WaitBusy();
    NAND_WRITE32(NAND_NFI_CNFG, 0);
    return i4Ret;
}

extern void HalDmaBufferStart(void *pBuffer, UINT32 size, enum HalDmaDirection direction);
extern void HalDmaBufferDone(void *pBuffer, UINT32 size, enum HalDmaDirection direction);

//-----------------------------------------------------------------------------
/** mt5365_dma_readpage()
 */
//-----------------------------------------------------------------------------
static int mt5365_dma_readpage(struct mtd_info *mtd, u_char *buf, u_char *spare)
{
    INT32 i4Ret = NAND_SUCC;
    struct nand_chip *chip = mtd->priv;

#if !defined(CONFIG_ARCH_MT5398) && !defined(CONFIG_ARCH_MT5880) && !defined(CC_MT5399) && !defined(CONFIG_ARCH_MT5860)
    UINT32 u4DmaRetryCnt = 0;
    UINT32 *pu4DmaBuf = (UINT32 *)buf;

    pu4DmaBuf[_u4PageSize/4-1] = NAND_DMA_PATTERN;
    HalDmaBufferStart((void *)(&buf[_u4PageSize - NAND_DMA_CACHE_LINE]), NAND_DMA_CACHE_LINE, HAL_DMA_TO_DEVICE);
    HalDmaBufferDone((void *)(&buf[_u4PageSize - NAND_DMA_CACHE_LINE]), NAND_DMA_CACHE_LINE, HAL_DMA_TO_DEVICE);
#endif
    _NANDHW_NFI_Reset();
    _NANDHW_NFIECC_Reset();

    #if NAND_DEBUG_PRINT
    printk(KERN_INFO "mt5365_dma_readpage: 0x%08x\n", _u4CurPage);
    #endif
#if NAND_RANDOM_EN
    if (fgRandomizer == TRUE)
    {
        NANDHW_Random_Mizer_Decode(_u4CurPage, TRUE);
    }
#endif
    _NANDHW_NFI_SetRdDma((UINT32 *)buf);

    HalDmaBufferStart((void *)buf, _u4PageSize, HAL_DMA_FROM_DEVICE);
#ifdef CC_UBOOT_2011_12
    _NANDHW_NFI_Cfg(chip->ecc.mode, _fgDmaEn, NAND_NFI_CNFG_OP_READ);
    _NANDHW_NFIECC_Cfg(chip->ecc.mode);
#else
    _NANDHW_NFI_Cfg(chip->eccmode, _fgDmaEn, NAND_NFI_CNFG_OP_READ);
    _NANDHW_NFIECC_Cfg(chip->eccmode);
#endif
    _NANDHW_NFIECC_Trig(NAND_NFI_CNFG_OP_READ);
#if defined(CC_MT5890) || defined(CC_MT5891) || defined(CC_MT5891)
    _NANDHW_SetEptNum(40);
#endif

    _NANDHW_Command(NAND_CMD_READ0);
    _NANDHW_Addr(_u4CurPage, 0, (_u4PgAddrLen<<4)|(_u4TotalAddrLen - _u4PgAddrLen));
    if (_u4PageSize != 512)
    {
        _NANDHW_Command(NAND_CMD_READSTART);
    }

    _NANDHW_WaitBusy();
    _NANDHW_NFI_CheckOpMode(NAND_NFI_CNFG_OP_READ);

#if !(NAND_AUTORD_DONE)
    /*Trigger Read . */
    _NANDHW_NFI_Trig(NAND_NFI_CNFG_OP_READ);
#endif

    _NANDHW_WaitBusy();
    _NANDHW_NFIECC_WaitBusy();

    _NANDHW_NFI_GetRdDma((UINT32 *)spare);
    _NANDHW_NFI_WaitSectorCnt();

    // Check ECC done
    _NANDHW_NFIECC_CheckEccDone();
#if CC_NAND_WDLE_CFG
    _NANDHW_CheckWDLE();
#endif
#if !defined(CONFIG_ARCH_MT5398) && !defined(CC_MT5399) && !defined(CONFIG_ARCH_MT5880) && !defined(CONFIG_ARCH_MT5860)
    // Cause nand dma read may send finish interrupt before write all data into dram.
    // So we add a pattern at last location to check if dma read is finished.
    while (1)
    {
        HalDmaBufferDone((void *)buf, _u4PageSize, HAL_DMA_FROM_DEVICE);

        if (pu4DmaBuf[_u4PageSize/4-1] != NAND_DMA_PATTERN)
        {
            if (u4DmaRetryCnt > 0)
            {
                printk(KERN_CRIT "uboot nand driver: u4DmaTryCnt = %d, pageidx = 0x%08X\n", u4DmaRetryCnt, _u4CurPage);
            }
            break;
        }

        u4DmaRetryCnt++;
        if (u4DmaRetryCnt >= NAND_DMA_RETYR_CNT)
        {
            printk(KERN_CRIT "uboot nand driver: dma pattern check time-out, pageidx = 0x%08X\n", _u4CurPage);
            break;
        }

        HAL_Delay_us(NAND_DMA_RETYR_DELAY);
    }
#endif

    // Cause pattern check and CPU prefetch will update cache.
    // Invalidate whole cache to ensure bit-flips writen into dram.
    HalDmaBufferDone((void *)buf, _u4PageSize, HAL_DMA_FROM_DEVICE);

    i4Ret = _NANDHW_NFIECC_CheckEcc(buf, spare);

    _NANDHW_WaitBusy();
    NAND_WRITE32(NAND_NFI_CNFG, 0);
#if NAND_RANDOM_EN
    if (fgRandomizer == TRUE)
    {
        NANDHW_Random_Mizer_Decode(_u4CurPage, FALSE);
    }
#endif
    return i4Ret;
}

//-----------------------------------------------------------------------------
/** mt5365_dma_writepage()
 */
//-----------------------------------------------------------------------------
static void mt5365_dma_writepage(struct mtd_info *mtd, u_char *buf, u_char *spare)
{
    struct nand_chip *chip = mtd->priv;
    #if NAND_DEBUG_PRINT
    printk(KERN_INFO "mt5365_dma_writepage\n");
    #endif

    _NANDHW_NFI_SetWtDma((UINT32 *)buf, (UINT32 *)spare);
    HalDmaBufferStart((void *)buf, _u4PageSize, HAL_DMA_TO_DEVICE);
#if NAND_RANDOM_EN
    if (fgRandomizer == TRUE)
    {
        NANDHW_Random_Mizer_Encode(_u4CurPage, TRUE);
    }
#endif
	#ifdef CC_UBOOT_2011_12
    _NANDHW_NFI_Cfg(chip->ecc.mode, _fgDmaEn, NAND_NFI_CNFG_OP_PROGRAM);
    _NANDHW_NFIECC_Cfg(chip->ecc.mode);
	#else
    _NANDHW_NFI_Cfg(chip->eccmode, _fgDmaEn, NAND_NFI_CNFG_OP_PROGRAM);
    _NANDHW_NFIECC_Cfg(chip->eccmode);
	#endif
    _NANDHW_NFIECC_Trig(NAND_NFI_CNFG_OP_PROGRAM);

    _NANDHW_Command(NAND_CMD_SEQIN);
    _NANDHW_Addr(_u4CurPage, 0, (_u4PgAddrLen<<4) | (_u4TotalAddrLen - _u4PgAddrLen));

    _NANDHW_WaitBusy();

     /*Trigger Write . */
    _NANDHW_NFI_Trig(NAND_NFI_CNFG_OP_PROGRAM);

    _NANDHW_WaitBusy();
    _NANDHW_NFIECC_WaitBusy();

    _NANDHW_NFI_WaitSectorCnt();
    _NANDHW_WaitBusy();

    NAND_WRITE32(NAND_NFI_CNFG, 0);
#if NAND_RANDOM_EN
    if (fgRandomizer == TRUE)
    {
        NANDHW_Random_Mizer_Encode(_u4CurPage, FALSE);
    }
#endif
    HalDmaBufferDone((void *)buf, _u4PageSize, HAL_DMA_TO_DEVICE);
}

//-----------------------------------------------------------------------------
/** mt5365_polling_readpage()
 */
//-----------------------------------------------------------------------------
static int mt5365_polling_readpage(struct mtd_info *mtd, u_char *buf, u_char *spare)
{
    struct nand_chip *chip = mtd->priv;
    UINT32 *pu4Buf = (UINT32 *)buf;
    UINT32 u4SectIdx, j, u4Val, u4LoopCnt;
    INT32 i4Ret = NAND_SUCC;

    UINT32 *pu4Spare = (UINT32 *)spare;
#if NAND_RANDOM_EN
    if (fgRandomizer == TRUE)
    {
        NANDHW_Random_Mizer_Decode(_u4CurPage, TRUE);
    }
#endif
#if defined(CC_MT5890) || defined(CC_MT5891) || defined(CC_MT5891)
    _NANDHW_SetEptNum(40);
#endif

    /*Trigger Read . */
    _NANDHW_NFI_CheckOpMode(NAND_NFI_CNFG_OP_READ);
    _NANDHW_NFI_Trig(NAND_NFI_CNFG_OP_READ);
    _NANDHW_WaitBusy();

    for(u4SectIdx = 0; u4SectIdx < _u4SectorNum; u4SectIdx++)
    {
        // main data
        u4LoopCnt = _u4SectorSize>>2;
        u4Val = u4SectIdx*u4LoopCnt;
        for(j = 0; j < u4LoopCnt; j++)
        {
            _NANDHW_WaitRead(4);
            pu4Buf[u4Val] = NAND_READ32(NAND_NFI_DATAR);
            u4Val++;
        }

        // FDM+ECC data
        u4LoopCnt = (_u4OOBSize * _u4SectorSize) >> 11;
        u4Val = u4SectIdx*u4LoopCnt;
        for(j = 0; j < u4LoopCnt; j++)
        {
            _NANDHW_WaitRead(4);
            pu4Spare[u4Val] = NAND_READ32(NAND_NFI_DATAR);
            u4Val++;
        }
#ifdef CC_UBOOT_2011_12
        if(chip->ecc.mode == NAND_ECC_SOFT )
#else
		if(chip->eccmode == NAND_ECC_SOFT )
#endif
        {
            /* ECC correction. */
            if(_NANDHW_NFIECC_CorrectEcc(buf, spare, u4SectIdx))
            {
                i4Ret |= NAND_ECC_ERROR;
            }
        }

        _NANDHW_WaitBusy();
    }

    _NANDHW_NFIECC_WaitBusy();
    _NANDHW_NFI_WaitSectorCnt();

    _NANDHW_WaitBusy();
    NAND_WRITE32(NAND_NFI_CNFG, 0);
#if NAND_RANDOM_EN
    if (fgRandomizer == TRUE)
    {
        NANDHW_Random_Mizer_Decode(_u4CurPage, FALSE);
    }
#endif
    return i4Ret;
}

//-----------------------------------------------------------------------------
/** mt5365_polling_writepage()
 */
//-----------------------------------------------------------------------------
static int mt5365_polling_writepage(struct mtd_info *mtd, u_char *buf, u_char *spare)
{
    INT32 i4Ret = 0;
    struct nand_chip *chip = mtd->priv;
    UINT32 i, u4Val, u4SectOobSz;
    u_char u1EccCode[NAND_NFI_ENCPAR_NUM*4];
#if NAND_RANDOM_EN
    if (fgRandomizer == TRUE)
    {
         NANDHW_Random_Mizer_Encode(_u4CurPage, TRUE);
    }
#endif
    u4SectOobSz =  (_u4OOBSize * _u4SectorSize)>>9;
    _NANDHW_NFI_Trig(NAND_NFI_CNFG_OP_PROGRAM);

    for(i = 0; i < _u4SectorNum; i++)
    {
        // write main data
        _NANDHW_NFI_WriteFifo(&buf[i*_u4SectorSize], _u4SectorSize);

#ifdef CC_UBOOT_2011_12
				if(chip->ecc.mode == NAND_ECC_SOFT )
#else
				if(chip->eccmode == NAND_ECC_SOFT )
#endif
        {
            // write fdm data
            _NANDHW_NFI_WriteFifo(&spare[i*u4SectOobSz], _u4FdmNum);

            /* ECC correction. */
            _NANDHW_WaitWrite();
            if(_NANDHW_NFIECC_GetEcc((UINT32 *)u1EccCode))
            {
                i4Ret = NAND_ECC_ERROR;
                //goto HandleError;
            }

            // write ecc data
            u4Val = u4SectOobSz - _u4FdmNum;
            _NANDHW_NFI_WriteFifo(u1EccCode, u4Val);
        }
        else
        {
            // write spare data
            _NANDHW_NFI_WriteFifo(&spare[i*u4SectOobSz], u4SectOobSz);
        }
    }

    _NANDHW_NFI_WaitSectorCnt();

    _NANDHW_WaitBusy();
    NAND_WRITE32(NAND_NFI_CNFG, 0);
#if NAND_RANDOM_EN
    if (fgRandomizer == TRUE)
    {
        NANDHW_Random_Mizer_Encode(_u4CurPage, FALSE);
    }
#endif
    return i4Ret;
}

//-----------------------------------------------------------------------------
/** mt5365_nand_verify_buf()
 */
//-----------------------------------------------------------------------------
static int mt5365_nand_verify_buf(struct mtd_info *mtd, const u_char *buf, int len)
{
#ifdef CC_UBOOT_2011_12

#if defined(CC_NAND_WRITE_VERIFY)
		int i4Ret = 0;

    #if NAND_DEBUG_PRINT
		printk(KERN_INFO "mt5365_nand_verify_buf, page = 0x%X\n", _u4CurPage);
    #endif

		if (_fgDmaEn)
		{
			i4Ret = mt5365_dma_readpage(mtd, _pu1NandDataBuf, _pu1NandOobBuf);
		}
		else
		{
			i4Ret = mt5365_polling_readpage(mtd, _pu1NandDataBuf, _pu1NandOobBuf);
		}

		if (i4Ret)
		{
			printk(KERN_ERR "mtk_nand_verify_buf read error! page = 0x%X\n", _u4CurPage);
			return -EIO;
		}

		if (_u4CurBitFlip)
		{
			printk(KERN_ERR "mtk_nand_verify_buf bitflip occur! page = 0x%X\n", _u4CurPage);
			return -EIO;
		}
#endif
#else
#if NAND_DEBUG_PRINT
    printk(KERN_INFO "mt5365_nand_verify_buf\n");
#endif
    #endif

    return 0;
}

//-----------------------------------------------------------------------------
/** mt5365_nand_read_byte()
 */
//-----------------------------------------------------------------------------
static u_char mt5365_nand_read_byte(struct mtd_info *mtd)
{
    u_char u1Reg;

    _NANDHW_WaitBusy();
    _NANDHW_NFI_Cfg(NAND_ECC_NONE, 0, NAND_NFI_CNFG_OP_SINGLE);
    NAND_WRITE32(NAND_NFI_CON, NAND_NFI_CON_NOB(1) | NAND_NFI_CON_SRD);

    _NANDHW_WaitBusy();
    _NANDHW_WaitRead(1);
    u1Reg =  (u_char)(NAND_READ32(NAND_NFI_DATAR) & 0xFF);

    // Add for reset fifo in byte read mode. (jun-2010/03/23)
    _NANDHW_NFI_Reset();
    return u1Reg;
}

//-----------------------------------------------------------------------------
/** mt5365_nand_read_buf()
 */
//-----------------------------------------------------------------------------
static void mt5365_nand_read_buf(struct mtd_info *mtd, uint8_t *buf, int len)
{
    BUG();
}

//-----------------------------------------------------------------------------
/** mt5365_nand_write_buf()
 */
//-----------------------------------------------------------------------------
static void mt5365_nand_write_buf(struct mtd_info *mtd, const uint8_t *buf, int len)
{
    BUG();
}

#ifdef CC_MTD_ENCRYPT_SUPPORT
static int mt5365_nand_enctyped(struct mtd_info *mtd, int page, UINT32 *pu4Data)
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
/** mt5365_nand_read_page()
 */
//-----------------------------------------------------------------------------
static int mt5365_nand_read_page(struct mtd_info *mtd, u_char *data_buf, u_char *oob_buf)
{
    INT32 i4Ret;
    u_char *pu1Data  = (u_char *)data_buf;
    u_char *pu1Spare = (u_char *)oob_buf;
    fgEmptyPage = FALSE;

    if (_fgDmaEn)
    {
       i4Ret = mt5365_dma_readpage(mtd, _pu1NandDataBuf, _pu1NandOobBuf);
    }
    else
    {
       i4Ret = mt5365_polling_readpage(mtd, _pu1NandDataBuf, _pu1NandOobBuf);
    }

    if (i4Ret)
    {
        return -1;
    }

// use poi read to verify dma read
#if NAND_DMA_READ_VERIFY
    {
        UINT32 i;
        struct nand_chip *chip = mtd->priv;
        UINT32 *pu4Buf1, *pu4Buf2;

        // backup the dma read buffercause verify read may let the dma has enough time to write bitflip
        memcpy(_u4CopyDataBuf, _pu1NandDataBuf, mtd->writesize);
        memcpy(_u4CopyOobBuf, _pu1NandOobBuf, mtd->oobsize);

        pu4Buf1 = (UINT32 *)_u4CopyDataBuf;
        pu4Buf2 = (UINT32 *)_u4VerifyDataBuf;

        _fgDmaEn = FALSE;
		#ifdef CC_UBOOT_2011_12
        chip->ecc.mode = NAND_ECC_SOFT;
		#else
		chip->eccmode = NAND_ECC_SOFT;
		#endif

        _NANDHW_NFI_Reset();
        _NANDHW_NFIECC_Reset();

		#ifdef CC_UBOOT_2011_12
        _NANDHW_NFI_Cfg(chip->ecc.mode, _fgDmaEn, NAND_NFI_CNFG_OP_READ);
        _NANDHW_NFIECC_Cfg(chip->ecc.mode);
		#else
        _NANDHW_NFI_Cfg(chip->eccmode, _fgDmaEn, NAND_NFI_CNFG_OP_READ);
        _NANDHW_NFIECC_Cfg(chip->eccmode);
		#endif
        _NANDHW_NFIECC_Trig(NAND_NFI_CNFG_OP_READ);

        _NANDHW_Command(NAND_CMD_READ0);
        _NANDHW_Addr(_u4CurPage, 0, (_u4PgAddrLen<<4)|(_u4TotalAddrLen - _u4PgAddrLen));
        if (mtd->writesize != 512)
        {
            _NANDHW_Command(NAND_CMD_READSTART);
        }

        mt5365_polling_readpage(mtd, (UINT8 *)_u4VerifyDataBuf, (UINT8 *)_u4VerifyOobBuf);

        for (i = 0; i < mtd->writesize/4; i++)
        {
            if (pu4Buf1[i] != pu4Buf2[i])
            {
                printk(KERN_ERR "read verify fail(uboot): page=0x%X, offset=0x%X, dma=0x%08X, pio=0x%08X\n",
                	_u4CurPage, i*4, pu4Buf1[i], pu4Buf2[i]);
            }
        }

        _fgDmaEn = TRUE;
		#ifdef CC_UBOOT_2011_12
        chip->ecc.mode = NAND_ECC_HW;
		#else
        chip->eccmode = NAND_ECC_HW;
		#endif
    }
#endif

    _NANDHW_DataExchange((UINT32 *)_pu1NandDataBuf, (UINT32 *)_pu1NandOobBuf, 0, mtd->writesize);

#ifdef CC_MTD_ENCRYPT_SUPPORT
    if (mt5365_nand_enctyped(mtd, _u4CurPage, (UINT32 *)_pu1NandDataBuf))
    {
        NAND_AES_Decryption(virt_to_phys((UINT32)_pu1NandDataBuf), virt_to_phys((UINT32)_pu1NandDataBuf), mtd->writesize);
    }
#endif

    memcpy(pu1Data,  _pu1NandDataBuf, mtd->writesize);
    memcpy(pu1Spare, _pu1NandOobBuf, mtd->oobsize);

    return 0;
}

//-----------------------------------------------------------------------------
/** mt5365_nand_write_page()
 */
//-----------------------------------------------------------------------------
static int mt5365_nand_write_page(struct mtd_info *mtd, const u_char *data_buf, const u_char *oob_buf)
{
    u_char *pu1Data  = (u_char *)data_buf;
    u_char *pu1Spare = (u_char *)oob_buf;

    memcpy(_pu1NandDataBuf, pu1Data, mtd->writesize);
    memcpy(_pu1NandOobBuf,  pu1Spare, mtd->oobsize);

#ifdef CC_MTD_ENCRYPT_SUPPORT
    if (mt5365_nand_enctyped(mtd, _u4CurPage, (UINT32 *)_pu1NandDataBuf))
    {
        NAND_AES_Encryption(virt_to_phys((UINT32)_pu1NandDataBuf), virt_to_phys((UINT32)_pu1NandDataBuf), mtd->writesize);
    }
#endif

    _NANDHW_DataExchange((UINT32 *)_pu1NandDataBuf, (UINT32 *)_pu1NandOobBuf, 0, mtd->writesize);

    if (_fgDmaEn)
    {
        mt5365_dma_writepage(mtd, _pu1NandDataBuf, _pu1NandOobBuf);
    }
    else
    {
        mt5365_polling_writepage(mtd, _pu1NandDataBuf, _pu1NandOobBuf);
    }

    return 0;
}

#ifdef CC_UBOOT_2011_12

static int mt5398_nand_read_page(struct mtd_info *mtd, struct nand_chip *chip, UINT8 *buf, int page)
{
    int i4Ret = NAND_SUCC;
    UINT8 *pu1Data  = (UINT8 *)buf;
    UINT8 *pu1Spare = (UINT8 *)chip->oob_poi;

    #if NAND_DEBUG_PRINT
    printk(KERN_INFO "mt5398_nand_read_page, _u4CurPage = 0x%x, page = 0x%X\n", _u4CurPage, page);
    #endif
    fgEmptyPage = FALSE;

    if ((UINT32)page != _u4CurPage)
    {
        BUG();
    }

    if (_fgDmaEn)
    {
        i4Ret = mt5365_dma_readpage(mtd, _pu1NandDataBuf, _pu1NandOobBuf);
    }
    else
    {
        i4Ret = mt5365_polling_readpage(mtd, _pu1NandDataBuf, _pu1NandOobBuf);
    }

    if (i4Ret)
    {
        mtd->ecc_stats.failed++;
    }
    else
    {
// use poi read to verify dma read
#if NAND_DMA_READ_VERIFY
        {
            UINT32 i;
            UINT32 *pu4Buf1, *pu4Buf2;

            // backup the dma read buffercause verify read may let the dma has enough time to write bitflip
            memcpy(_u4CopyDataBuf, _pu1NandDataBuf, mtd->writesize);
            memcpy(_u4CopyOobBuf, _pu1NandOobBuf, mtd->oobsize);

            pu4Buf1 = (UINT32 *)_u4CopyDataBuf;
            pu4Buf2 = (UINT32 *)_u4VerifyDataBuf;

            _fgDmaEn = FALSE;
            chip->ecc.mode = NAND_ECC_SOFT;

            _NANDHW_NFI_Reset();
            _NANDHW_NFIECC_Reset();

            _NANDHW_NFI_Cfg(chip->ecc.mode, _fgDmaEn, NAND_NFI_CNFG_OP_READ);
            _NANDHW_NFIECC_Cfg(chip->ecc.mode);
            _NANDHW_NFIECC_Trig(NAND_NFI_CNFG_OP_READ);

            _NANDHW_Command(NAND_CMD_READ0);
            _NANDHW_Addr(_u4CurPage, 0, (_u4PgAddrLen<<4)|(_u4TotalAddrLen - _u4PgAddrLen));
            if (mtd->writesize != 512)
            {
                _NANDHW_Command(NAND_CMD_READSTART);
            }

            mt5365_polling_readpage(mtd, (UINT8 *)_u4VerifyDataBuf, (UINT8 *)_u4VerifyOobBuf);

            for (i = 0; i < mtd->writesize/4; i++)
            {
                if (pu4Buf1[i] != pu4Buf2[i])
                {
                    printk(KERN_ERR "read verify fail(native): page=0x%X, offset=0x%X, dma=0x%08X, pio=0x%08X\n",
                    	_u4CurPage, i*4, pu4Buf1[i], pu4Buf2[i]);
                }
            }

            _fgDmaEn = TRUE;
            chip->ecc.mode = NAND_ECC_HW;
        }
#endif

        mtd->ecc_stats.corrected += _u4CurBitFlip;
        _NANDHW_DataExchange((UINT32 *)_pu1NandDataBuf, (UINT32 *)_pu1NandOobBuf, 0, mtd->writesize);

#ifdef CC_MTD_ENCRYPT_SUPPORT
        if (mt5365_nand_enctyped(mtd, _u4CurPage, (UINT32 *)_pu1NandDataBuf))
        {
            NAND_AES_Decryption(virt_to_phys((void *)_pu1NandDataBuf), virt_to_phys((void *)_pu1NandDataBuf), mtd->writesize);
        }
#endif
    }

     memcpy(pu1Data,  _pu1NandDataBuf, mtd->writesize);
     memcpy(pu1Spare, _pu1NandOobBuf, mtd->oobsize);

    return  0;
}

//-----------------------------------------------------------------------------
/** mt5398_nand_write_page()
 */
//-----------------------------------------------------------------------------
static void mt5398_nand_write_page(struct mtd_info *mtd, struct nand_chip *chip, const UINT8 *buf)
{
    UINT8 *pu1Data  = (UINT8 *)buf;
    UINT8 *pu1Spare = (UINT8 *)chip->oob_poi;

    #if NAND_DEBUG_PRINT
    printk(KERN_INFO "mt5398_nand_write_page, page = 0x%X\n", _u4CurPage);
    #endif

    memcpy(_pu1NandDataBuf, pu1Data, mtd->writesize);
    memcpy(_pu1NandOobBuf,  pu1Spare, mtd->oobsize);

#ifdef CC_MTD_ENCRYPT_SUPPORT
    if (mt5365_nand_enctyped(mtd, _u4CurPage, (UINT32 *)_pu1NandDataBuf))
    {
        NAND_AES_Encryption(virt_to_phys((void *)_pu1NandDataBuf), virt_to_phys((void *)_pu1NandDataBuf), mtd->writesize);
    }
#endif

    _NANDHW_DataExchange((UINT32 *)_pu1NandDataBuf, (UINT32 *)_pu1NandOobBuf, 0, mtd->writesize);

    if (_fgDmaEn)
    {
        mt5365_dma_writepage(mtd, _pu1NandDataBuf, _pu1NandOobBuf);
    }
    else
    {
        mt5365_polling_writepage(mtd, _pu1NandDataBuf, _pu1NandOobBuf);
    }
}

//-----------------------------------------------------------------------------
/** mt5398_nand_read_oob()
 */
//-----------------------------------------------------------------------------
static int mt5398_nand_read_oob(struct mtd_info *mtd, struct nand_chip *chip, INT32 page, INT32 sndcmd)
{
    int i4Ret = NAND_SUCC;
    UINT8 *pu1Spare = (UINT8 *)chip->oob_poi;

    #if NAND_DEBUG_PRINT
    printk(KERN_INFO "mt5398_nand_read_oob\n");
    #endif

    if (likely(sndcmd))
    {
        chip->cmdfunc(mtd, NAND_CMD_READ0, 0, page);
        sndcmd = 0;
    }

    if (_fgDmaEn)
    {
        i4Ret = mt5365_dma_readpage(mtd, _pu1NandDataBuf, _pu1NandOobBuf);
    }
    else
    {
        i4Ret = mt5365_polling_readpage(mtd, _pu1NandDataBuf, _pu1NandOobBuf);
    }

    if (i4Ret)
    {
        mtd->ecc_stats.failed++;
    }
    else
    {
        mtd->ecc_stats.corrected += _u4CurBitFlip;
        _NANDHW_DataExchange((UINT32 *)_pu1NandDataBuf, (UINT32 *)_pu1NandOobBuf, 0, mtd->writesize);
    }

    memcpy(pu1Spare, _pu1NandOobBuf, mtd->oobsize);
    return sndcmd;
}

//-----------------------------------------------------------------------------
/** mt5398_nand_write_oob()
 */
//-----------------------------------------------------------------------------
static int mt5398_nand_write_oob(struct mtd_info *mtd, struct nand_chip *chip, INT32 page)
{
    printk(KERN_ERR "mt5398_nand_write_oob\n");
    BUG();

    return 0;
}
#endif
//-----------------------------------------------------------------------------
/** _NANDHW_Page_Empty()
 */
//-----------------------------------------------------------------------------
static int _NANDHW_Page_Empty(UINT32 *pu4Buf, UINT32 *pu4Spare)
{
    // If all data is 0xff, do not write the page. mtk40109 - 2010/4/23
    UINT32 i, u4PgOobSz;

    for (i = 0; i < _u4PageSize/4; i++)
    {
        if (pu4Buf[i] != 0xFFFFFFFF)
        {
            return 0;
        }
    }
    //for scan bad block in Random mizer, the function is closed to a new nand flash.
#if CC_NAND_60BIT_NFI
#if NAND_RANDOM_EN
    if (fgRandomizer && (!pu4Spare))
    {
        return 1;
    }
#endif
#endif

    u4PgOobSz = (_u4OOBSize * _u4PageSize)>>9;
    for (i = 0; i < u4PgOobSz/4; i++)
    {
        if (pu4Spare[i] != 0xFFFFFFFF)
        {
            return 0;
        }
    }

    return 1;
}

//-----------------------------------------------------------------------------
/** mt5365_nand_isbadpage()
 */
//-----------------------------------------------------------------------------
BOOL mt5365_nand_isbadpage(struct mtd_info *mtd, int page)
{
    INT32 iRet;
    BOOL isEmpty = TRUE;
    UINT32 u4BitCnt, u4Offset, u4OobLastIdx, i;
    UINT32 u4SectOobSz, u4PgOobSz, u4EccSectSz;

    // Cause mt5365_nand_isbadblk() is not through mt5365_nand_command().
    // Init variable _u4CurPage
    _u4CurPage = (UINT32)page;
	#ifdef CC_UBOOT_2011_12
	_u4CurBitFlip = 0;
	#endif

    UINT8 *pu1Data  = (UINT8 *)_pu1NandDataBuf;
    UINT8 *pu1Spare = (UINT8 *)_pu1NandOobBuf;

    u4SectOobSz = (_u4OOBSize * _u4SectorSize)>>9;
    u4PgOobSz = (_u4OOBSize * _u4PageSize)>>9;
    u4EccSectSz = _u4SectorSize + u4SectOobSz;

    // read spare data without ecc
    u4Offset = _u4PageSize;
#if NAND_RANDOM_EN
    if (fgRandomizer == TRUE)
    {
        iRet = mt5365_nand_read_page(mtd, (UINT32 *)pu1Data, (UINT32 *)pu1Spare);
        if (iRet)
        {
            goto HandleBad;
        }
        isEmpty = fgEmptyPage;
		//printk(KERN_INFO "mt5365_nand_isbadpage:0x%08x, 0x%08x\n",page, fgEmptyPage);
    }
#endif
    if (isEmpty == TRUE)
    {
        if ((_prFlashDev->u4BBMode & NAND_BBMODE_ECCBIT) == 1)
        {
             iRet = _NANDHW_ReadSpare(pu1Spare, page, u4Offset, _u4OOBSize, NAND_ECC_NONE);
        }
        else
        {
             iRet = _NANDHW_ReadSpare(pu1Spare, page, u4Offset, u4PgOobSz, NAND_ECC_NONE);
        }
    }

    if (iRet)
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
    u4Offset = (_u4SectorNum - 1) * u4EccSectSz;
    iRet = _NANDHW_ReadSpare(pu1Data, page, u4Offset, u4EccSectSz, NAND_ECC_SOFT);
    if (iRet)
    {
        goto HandleBad;
    }

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
/** mt5365_nand_isbadblk()
 */
//-----------------------------------------------------------------------------
static int mt5365_nand_isbadblk(struct mtd_info *mtd, loff_t ofs, int getchip)
{
    struct nand_chip *chip = mtd->priv;
    int block = (int)(ofs >> chip->phys_erase_shift);

    // check the 1st page of the block.
    if (mt5365_nand_isbadpage(mtd, block * _u4BlkPgCount))
    {
        return 1;
    }
    // check the 2nd page of the block.
    if (mt5365_nand_isbadpage(mtd, block * _u4BlkPgCount + 1))
    {
        return 1;
    }
    // check the last page of the block if MLC.
    if (_u4OOBSize > 16)
    {
        if (mt5365_nand_isbadpage(mtd, block * _u4BlkPgCount + _u4BlkPgCount -1))
        {
            return 1;
        }
    }

    return 0;
}

//-----------------------------------------------------------------------------
/** mt5365_markbad()
 */
//-----------------------------------------------------------------------------
static int mt5365_nand_markbadpage(struct mtd_info *mtd, int page)
{
    struct nand_chip *chip = mtd->priv;
    int status = 0;

    memset(_pu1NandDataBuf, 0, mtd->writesize);
    memset(_pu1NandOobBuf, 0, mtd->oobsize);

    chip->cmdfunc(mtd, NAND_CMD_SEQIN, 0, page);

    if (_fgDmaEn)
    {
        mt5365_dma_writepage(mtd, _pu1NandDataBuf, _pu1NandOobBuf);
    }
    else
    {
        mt5365_polling_writepage(mtd, _pu1NandDataBuf, _pu1NandOobBuf);
    }

    chip->cmdfunc(mtd, NAND_CMD_PAGEPROG, -1, -1);
    #ifdef CC_UBOOT_2011_12
    status = chip->waitfunc(mtd, chip);
	#else
	status = chip->waitfunc(mtd, chip, FL_WRITING);
	#endif
    return status & NAND_STATUS_FAIL ? -EIO : 0;
}

//-----------------------------------------------------------------------------
/** mt5365_nand_eraseblk()
 */
//-----------------------------------------------------------------------------
static INT32 mt5365_nand_eraseblk(struct mtd_info *mtd, UINT32 u4BlkIdx)
{
    struct nand_chip *chip = mtd->priv;
    int status = 0;
    UINT32 u4PgIdx = u4BlkIdx * _u4BlkPgCount;

    _NANDHW_NFI_Reset();
    _NANDHW_NFI_Cfg(NAND_ECC_NONE, FALSE, NAND_NFI_CNFG_OP_ERASE);

    _NANDHW_Command(NAND_CMD_ERASE1);
    _NANDHW_Addr(u4PgIdx, 0, _u4PgAddrLen<<4);
    _NANDHW_WaitBusy();

    /*Trigger Erase. */
    NAND_WRITE32(NAND_NFI_CMD, NAND_CMD_ERASE2);
    _NANDHW_WaitBusy();

    NAND_WRITE32(NAND_NFI_CNFG, 0);


#ifdef CC_UBOOT_2011_12
    status = chip->waitfunc(mtd, chip);
#else
    status = chip->waitfunc(mtd, chip, FL_ERASING);
#endif
    return status & NAND_STATUS_FAIL ? -EIO : 0;
}

//-----------------------------------------------------------------------------
/** mt5365_nand_markbadblk()
 */
//-----------------------------------------------------------------------------
static int mt5365_nand_markbadblk(struct mtd_info *mtd, loff_t ofs)
{
    struct nand_chip *chip = mtd->priv;
    int block, res = 0;

    block = (int)(ofs >> chip->phys_erase_shift);

    // erase block before mark it bad
    // cause MLC does not support re-write absolutely. mtk40109 2010-12-06
    if (mt5365_nand_eraseblk(mtd, block))
    {
        res = -EIO;
    }

    if (mt5365_nand_markbadpage(mtd, block * _u4BlkPgCount))
    {
        res = -EIO;
    }

    // Mark the last page of the block if MLC.
    if (_u4OOBSize > 16)
    {
        if (mt5365_nand_markbadpage(mtd, block * _u4BlkPgCount + _u4BlkPgCount - 1))
        {
            res = -EIO;
        }
    }

    return res;
}

//-----------------------------------------------------------------------------
/** mt5365_nand_dev_ready()
 */
//-----------------------------------------------------------------------------
static int mt5365_nand_dev_ready(struct mtd_info *mtd)
{
    return 1;
}

//-----------------------------------------------------------------------------
/** mt5365_nand_command()
 */
//-----------------------------------------------------------------------------
static void mt5365_nand_command(struct mtd_info *mtd, unsigned command, int column, int page_addr)
{
    struct nand_chip *chip = mtd->priv;

    #if NAND_DEBUG_PRINT
    printk(KERN_INFO "command: 0x%x, col: 0x%x, page: 0x%x\n", command, column, page_addr);
    #endif

    _u4CurPage = (UINT32)page_addr;
	#ifdef CC_UBOOT_2011_12
	_u4CurBitFlip = 0;
	#endif
    _NANDHW_NFI_Reset();
    _NANDHW_NFIECC_Reset();

    switch(command)
    {
    case NAND_CMD_READID:
        _NANDHW_Command(command);
        _NANDHW_Addr(0, 0, 1);
        break;

    case NAND_CMD_READ0:
        if (column >= (_u4PageSize + mtd->oobsize))
        {
            printk(KERN_ERR "column is too large\n");
            BUG();
        }

        if (!_fgDmaEn)
        {
            // in dma mode, move register set to readpage function. -2010/07/13
            // resolve: bit flip can lead to crc error while uncompressing kernel.
            #ifdef CC_UBOOT_2011_12
            _NANDHW_NFI_Cfg(chip->ecc.mode, _fgDmaEn, NAND_NFI_CNFG_OP_READ);
            _NANDHW_NFIECC_Cfg(chip->ecc.mode);
			#else

            _NANDHW_NFI_Cfg(chip->eccmode, _fgDmaEn, NAND_NFI_CNFG_OP_READ);
            _NANDHW_NFIECC_Cfg(chip->eccmode);
			#endif
            _NANDHW_NFIECC_Trig(NAND_NFI_CNFG_OP_READ);

            _NANDHW_Command(command);
            _NANDHW_Addr(page_addr, column, (_u4PgAddrLen<<4)|(_u4TotalAddrLen - _u4PgAddrLen));
            if (_u4PageSize != 512)
            {
                _NANDHW_Command(NAND_CMD_READSTART);
            }
        }
        break;

    case NAND_CMD_READ1:
        if (_u4PageSize != 512)
        {
            printk(KERN_ERR "Unhandle command!\n");
            BUG();
        }
        if (column >= (_u4PageSize + mtd->oobsize))
        {
            printk(KERN_ERR "column is too large\n");
            BUG();
        }
        _NANDHW_Command(command);
        _NANDHW_Addr(page_addr, column, (_u4PgAddrLen<<4)|(_u4TotalAddrLen - _u4PgAddrLen));
        break;

    case NAND_CMD_READOOB:
        if (_u4PageSize != 512)
        {
            column += _u4PageSize;
            command = NAND_CMD_READ0;
        }
        if (column >= (_u4PageSize + mtd->oobsize))
        {
            printk(KERN_ERR "column is too large\n");
            BUG();
        }
        _NANDHW_Command(command);
        _NANDHW_Addr(page_addr, column, (_u4PgAddrLen<<4)|(_u4TotalAddrLen - _u4PgAddrLen));
        if (_u4PageSize != 512)
        {
            _NANDHW_Command(NAND_CMD_READSTART);
        }
        break;

    case NAND_CMD_SEQIN:
        if (column != 0)
        {
            BUG();
        }

        if (!_fgDmaEn)
        {
            // in dma mode, move register set to writepage function. -2010/07/13
            // resolve: bit flip can lead to crc error while uncompressing kernel.
            #ifdef CC_UBOOT_2011_12
            _NANDHW_NFI_Cfg(chip->ecc.mode, _fgDmaEn, NAND_NFI_CNFG_OP_PROGRAM);
            _NANDHW_NFIECC_Cfg(chip->ecc.mode);
			#else

            _NANDHW_NFI_Cfg(chip->eccmode, _fgDmaEn, NAND_NFI_CNFG_OP_PROGRAM);
            _NANDHW_NFIECC_Cfg(chip->eccmode);
			#endif
            _NANDHW_NFIECC_Trig(NAND_NFI_CNFG_OP_PROGRAM);

            _NANDHW_Command(NAND_CMD_SEQIN);
            _NANDHW_Addr(page_addr, 0, (_u4PgAddrLen<<4)|(_u4TotalAddrLen - _u4PgAddrLen));
        }
        break;

    case NAND_CMD_ERASE1:
        _NANDHW_Command(NAND_CMD_ERASE1);
        _NANDHW_Addr(page_addr, 0, _u4PgAddrLen<<4);
        break;

    case NAND_CMD_RESET:
    case NAND_CMD_PAGEPROG:
    case NAND_CMD_STATUS:
    case NAND_CMD_ERASE2:
        _NANDHW_Command(command);
        break;

    default:
        printk(KERN_ERR "Unhandle command!\n");
        BUG();
        break;
    }

    _NANDHW_WaitBusy();
}

//-----------------------------------------------------------------------------
/** mt5365_nand_enable_hwecc()
 */
//-----------------------------------------------------------------------------
static void mt5365_nand_enable_hwecc(struct mtd_info *mtd, int mode)
{
	#ifndef CC_UBOOT_2011_12
    BUG();
	#endif
}

//-----------------------------------------------------------------------------
/** mt5365_nand_calculate_ecc()
 */
//-----------------------------------------------------------------------------
static int mt5365_nand_calculate_ecc(struct mtd_info *mtd, const u_char *dat, u_char *ecc_code)
{
    #ifndef CC_UBOOT_2011_12
    BUG();
    #endif
    return 0;
}

//-----------------------------------------------------------------------------
/** mt5365_nand_correct_data()
 */
//-----------------------------------------------------------------------------
static int mt5365_nand_correct_data(struct mtd_info *mtd, u_char *dat, u_char *read_ecc, u_char *calc_ecc)
{
#ifndef CC_UBOOT_2011_12
    BUG();
#endif
    return 0;
}

//-----------------------------------------------------------------------------
/** mt5365_nand_scan_bbt()
 */
//-----------------------------------------------------------------------------
static int mt5365_nand_scan_bbt(struct mtd_info *mtd)
{
    return 0;
}

//-----------------------------------------------------------------------------
/** mt5365_nand_select_chip()
 */
//-----------------------------------------------------------------------------
static void mt5365_nand_select_chip(struct mtd_info *mtd, int chip)
{
}

#if NAND_ASYNC_READ_ENABLE
//-----------------------------------------------------------------------------
/** mtk_async_readpages_start()
 */
//-----------------------------------------------------------------------------
static int mtk_async_readpages_start(struct mtd_info *mtd, u_char *buf, u_char *spare)
{
    struct nand_chip *chip = mtd->priv;
    UINT32 *pu4DmaBuf = (UINT32 *)buf;

    #if NAND_DEBUG_PRINT
    printk(KERN_INFO "mtk_async_readpages_start\n");
    #endif

    pu4DmaBuf[_u4PageSize/4-1] = NAND_DMA_PATTERN;
    HalDmaBufferStart((void *)(&buf[_u4PageSize - NAND_DMA_CACHE_LINE]), NAND_DMA_CACHE_LINE, HAL_DMA_TO_DEVICE);
    HalDmaBufferDone((void *)(&buf[_u4PageSize - NAND_DMA_CACHE_LINE]), NAND_DMA_CACHE_LINE, HAL_DMA_TO_DEVICE);
#if NAND_RANDOM_EN
    if (fgRandomizer == TRUE)
    {
        NANDHW_Random_Mizer_Decode(_u4CurPage, TRUE);
    }
#endif

    _NANDHW_NFI_SetRdDma((UINT32 *)buf);

    HalDmaBufferStart((void *)buf, _u4PageSize, HAL_DMA_FROM_DEVICE);

#ifdef CC_UBOOT_2011_12
    _NANDHW_NFI_Cfg(chip->ecc.mode, _fgDmaEn, NAND_NFI_CNFG_OP_READ);
    _NANDHW_NFIECC_Cfg(chip->ecc.mode);
#else
    _NANDHW_NFI_Cfg(chip->eccmode, _fgDmaEn, NAND_NFI_CNFG_OP_READ);
    _NANDHW_NFIECC_Cfg(chip->eccmode);
#endif
    _NANDHW_NFIECC_Trig(NAND_NFI_CNFG_OP_READ);

    _NANDHW_Command(NAND_CMD_READ0);
    _NANDHW_Addr(_u4CurPage, 0, (_u4PgAddrLen<<4)|(_u4TotalAddrLen - _u4PgAddrLen));
    if (_u4PageSize != 512)
    {
        _NANDHW_Command(NAND_CMD_READSTART);
    }

    _NANDHW_WaitBusy();
    _NANDHW_NFI_CheckOpMode(NAND_NFI_CNFG_OP_READ);

    /*Trigger Read . */
    _NANDHW_NFI_Trig(NAND_NFI_CNFG_OP_READ);

    return NAND_SUCC;
}

//-----------------------------------------------------------------------------
/** mtk_async_readpages_finish()
 */
//-----------------------------------------------------------------------------
static int mtk_async_readpages_finish(struct mtd_info *mtd, u_char *buf, u_char *spare)
{
    INT32 i4Ret = NAND_SUCC;
    UINT32 u4DmaRetryCnt = 0;
    UINT32 *pu4DmaBuf = (UINT32 *)buf;

    #if NAND_DEBUG_PRINT
    printk(KERN_INFO "mtk_async_readpages_finish\n");
    #endif

    _NANDHW_WaitBusy();
    _NANDHW_NFIECC_WaitBusy();

    _NANDHW_NFI_GetRdDma((UINT32 *)spare);
    _NANDHW_NFI_WaitSectorCnt();

    // Check ECC done
    _NANDHW_NFIECC_CheckEccDone();

    // Cause nand dma read may send finish interrupt before write all data into dram.
    // So we add a pattern at last location to check if dma read is finished.
    while (1)
    {
        HalDmaBufferDone((void *)buf, _u4PageSize, HAL_DMA_FROM_DEVICE);

        if (pu4DmaBuf[_u4PageSize/4-1] != NAND_DMA_PATTERN)
        {
            if (u4DmaRetryCnt > 0)
            {
                printk(KERN_CRIT "uboot nand driver: u4DmaTryCnt = %d, pageidx = 0x%08X\n", u4DmaRetryCnt, _u4CurPage);
            }
            break;
        }

        u4DmaRetryCnt++;
        if (u4DmaRetryCnt >= NAND_DMA_RETYR_CNT)
        {
            printk(KERN_CRIT "uboot nand driver: dma pattern check time-out, pageidx = 0x%08X\n", _u4CurPage);
            break;
        }

        HAL_Delay_us(NAND_DMA_RETYR_DELAY);
    }

    // Cause pattern check and CPU prefetch will update cache.
    // Invalidate whole cache to ensure bit-flips writen into dram.
    HalDmaBufferDone((void *)buf, _u4PageSize, HAL_DMA_FROM_DEVICE);

    i4Ret = _NANDHW_NFIECC_CheckEcc(buf, spare);

    _NANDHW_WaitBusy();
#if NAND_RANDOM_EN
    if (fgRandomizer == TRUE)
    {
        NANDHW_Random_Mizer_Decode(_u4CurPage, FALSE);
    }
#endif
    NAND_WRITE32(NAND_NFI_CNFG, 0);
    return i4Ret;
}

//-----------------------------------------------------------------------------
/** mtk_async_read_start()
 */
//-----------------------------------------------------------------------------
static int mtk_async_read_start(struct mtd_info *mtd)
{
    return mtk_async_readpages_start(mtd, _pu1NandDataBuf, _pu1NandOobBuf);
}

//-----------------------------------------------------------------------------
/** mtk_async_read_finish()
 */
//-----------------------------------------------------------------------------
static int mtk_async_read_finish(struct mtd_info *mtd, int len, u_char *data_buf)
{
    INT32 i4Ret;

    i4Ret = mtk_async_readpages_finish(mtd, _pu1NandDataBuf, _pu1NandOobBuf);
    if (i4Ret)
    {
        return -1;
    }

    _NANDHW_DataExchange((UINT32 *)_pu1NandDataBuf, (UINT32 *)_pu1NandOobBuf, 0, mtd->writesize);

#ifdef CC_MTD_ENCRYPT_SUPPORT
    if (mt5365_nand_enctyped(mtd, _u4CurPage, (UINT32 *)_pu1NandDataBuf))
    {
        NAND_AES_Decryption(virt_to_phys((UINT32)_pu1NandDataBuf), virt_to_phys((UINT32)_pu1NandDataBuf), mtd->writesize);
    }
#endif

    memcpy(data_buf, _pu1NandDataBuf, len);
}
#endif

//-----------------------------------------------------------------------------
/** mt5365_nand_init() - call before nand_scan()
 */
//-----------------------------------------------------------------------------
static int mt5365_nand_init(struct nand_chip *nand)
{
    struct nand_chip *this = nand;

    #if NAND_DEBUG_PRINT
    printk(KERN_INFO "mt5365_nand_init\n");
    #endif

    init_MUTEX(&nand_pin_share);

    _NANDHW_Init();

    /* Initialize structures */
    memset((char *)this, 0, sizeof(struct nand_chip));

    if (_fgDmaEn)
    {
#ifdef CC_UBOOT_2011_12
        this->ecc.mode = NAND_ECC_HW;
#else
this->eccmode = NAND_ECC_HW;

#endif
    }
    else
    {
#ifdef CC_UBOOT_2011_12
			this->ecc.mode = NAND_ECC_SOFT;
#else
	this->eccmode = NAND_ECC_SOFT;

#endif
    }
#ifdef CC_UBOOT_2011_12
	this->ecc.size = _u4PageSize;

    /* Used from chip select and nand_command() */
    this->chip_delay = 0;
    this->badblockpos = 0;
    this->options |= NAND_SKIP_BBTSCAN;
    //this->options |= NAND_USE_FLASH_BBT;
#endif
    this->read_byte = mt5365_nand_read_byte;
    this->select_chip = mt5365_nand_select_chip;
    this->dev_ready = mt5365_nand_dev_ready;
#ifndef CC_UBOOT_2011_12
    this->chip_delay = 0;
#endif
    this->cmdfunc = mt5365_nand_command;
    this->write_buf = mt5365_nand_write_buf;
    this->read_buf = mt5365_nand_read_buf;
    this->verify_buf = mt5365_nand_verify_buf;
#ifndef CC_UBOOT_2011_12
    this->calculate_ecc = mt5365_nand_calculate_ecc;
    this->correct_data = mt5365_nand_correct_data;
    this->enable_hwecc = mt5365_nand_enable_hwecc;
    this->scan_bbt = mt5365_nand_scan_bbt;

    this->read_page = mt5365_nand_read_page;
    this->write_page = mt5365_nand_write_page;
#endif

#if NAND_ASYNC_READ_ENABLE
    this->async_read_start = mtk_async_read_start;
    this->async_read_finish = mtk_async_read_finish;
#endif
    this->block_bad = mt5365_nand_isbadblk;
    this->block_markbad = mt5365_nand_markbadblk;
#ifndef CC_UBOOT_2011_12

    this->badblockpos = 0x0;
#else
    this->ecc.hwctl = mt5365_nand_enable_hwecc;
    this->ecc.calculate = mt5365_nand_calculate_ecc;
    this->ecc.correct = mt5365_nand_correct_data;
    this->ecc.read_page = mt5398_nand_read_page;
    this->ecc.write_page = mt5398_nand_write_page;
    this->ecc.read_oob = mt5398_nand_read_oob;
    this->ecc.write_oob = mt5398_nand_write_oob;
#endif
    /* init completed */
    return 0;
}

int board_nand_init(struct nand_chip *nand)
{
    return mt5365_nand_init(nand);
}

void MTDNANDPinShareEntry(void)
{
    UINT32 u4Val;

    //NANDPinShareLock();
    // Set nand/nor pinmux.

    u4Val = NAND_READ32(NAND_NFI_MISC);
    u4Val |= NAND_NFI_MISC_FLASH_PMUX;
    NAND_WRITE32(NAND_NFI_MISC, u4Val);

    NANDHW_Reset();
}

void MTDNANDPinShareExit(void)
{
    //NANDPinShareUnlock();
}

#endif //defined(CONFIG_CMD_NAND)
