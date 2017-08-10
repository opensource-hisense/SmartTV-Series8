/*
 * drivers/mtd/mtk_mtdpart.c
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

/*
 * header file
 */
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/nand_ecc.h>
#include <linux/mtd/mtk_mtdpart.h>
#include <linux/interrupt.h>
#include <linux/bitops.h>
#include <linux/leds.h>
#include <linux/io.h>
#include <linux/vmalloc.h>

#include <x_typedef.h>

/* proc */
#include <linux/proc_fs.h>
#include <linux/seq_file.h>


static UINT32 *badblock_mapping_table[MAX_MTD_DEVICES];

#define NFB_FASTBOOT_HEADER_SIZE 0x600000
INT32 i4NFBPartitionFastboot(struct mtd_info *mtd)
{
    if (!mtd)
    {
        return 0;
    }
    
    /* if bad block checking not available, do nothing */
    if (!mtd_can_have_bb(mtd))
    {
        return 0;
    }

    /* if not fast boot partition, no nothing */
    if (!mtd->name || strstr(mtd->name, "susp") == NULL)
    {
        return 0;
    }

    return 1;
}

static void i4NFBUpdateMappingTable (struct mtd_info *mtd)
{
    UINT32 *mapping_table = NULL;
    UINT32 u4PhyBlk, u4BlkNum, u4VirtBlk;
    INT32  i4Fastboot = i4NFBPartitionFastboot(mtd);
    
    u4BlkNum = mtd->size >> mtd->erasesize_shift;
    mapping_table = badblock_mapping_table[mtd->index];
    if(!mapping_table)
    {
        mapping_table = kmalloc(u4BlkNum * sizeof(mapping_table[0]), GFP_KERNEL);
    }

    if (!mapping_table)
    {
        BUG();
        return;
    }
    
    memset(mapping_table, 0xFFFFFFFF, u4BlkNum * sizeof(mapping_table[0]));

    u4VirtBlk = 0;
    for (u4PhyBlk = 0; u4PhyBlk < u4BlkNum; u4PhyBlk++)
    {
        if (i4Fastboot && (u4PhyBlk * mtd->erasesize == NFB_FASTBOOT_HEADER_SIZE))
        {
            u4VirtBlk = u4PhyBlk;
        }
        
        if (mtd_block_isbad(mtd, u4PhyBlk * mtd->erasesize))
        {
            printk(KERN_INFO "[i4NFBUpdateMappingTable] part %d, block %d is bad!\n", mtd->index, u4PhyBlk);
            continue;
        }
        
        mapping_table[u4VirtBlk] = u4PhyBlk;
        u4VirtBlk++;
    }
    
    badblock_mapping_table[mtd->index] = mapping_table;
}

static INT32 i4NFBRead(struct mtd_info *mtd, UINT32 u4Offset, UINT8 *pu1MemPtr, UINT32 u4MemLen)
{
    INT32   i4Ret = 0;
    UINT32  *mapping_table;
    UINT32  u4RetLen = 0, u4PhyOffs, u4PhyBlkIdx;

    mapping_table = badblock_mapping_table[mtd->index];
    u4PhyBlkIdx = mapping_table[u4Offset / mtd->erasesize];
    u4PhyOffs = (u4PhyBlkIdx * mtd->erasesize) | (u4Offset & (mtd->erasesize - 1));

    if (u4PhyBlkIdx == 0xFFFFFFFF)
    {
        printk(KERN_ERR "[i4NFBRead] Too many bad block!\n");
        return -1;
    }
    
    i4Ret = mtd_read(mtd, u4PhyOffs, u4MemLen, &u4RetLen, pu1MemPtr);
    if (i4Ret)
    {
        if (i4Ret == -EUCLEAN)
        {
            printk(KERN_INFO "[i4NFBRead] %d fixable bit-flips has been detected\n", mtd->ecc_stats.corrected);
        }
        else
        {
            return i4Ret;
        }  
    }
   
    if (u4RetLen != u4MemLen)
    {
        return -EIO;
    }
    else
    {
        return 0;
    }
}

static INT32 i4NFBWriteBlock(struct mtd_info *mtd, UINT32 u4Offset, UINT8 *pu1MemPtr)
{
    INT32   i4Ret = 0;
    UINT32  u4RetLen = 0, u4PhyOffs, u4PhyBlkIdx;
    struct  erase_info erase;
    UINT32  *mapping_table;
    
    while (1)
    {
        mapping_table = badblock_mapping_table[mtd->index];
        u4PhyBlkIdx = mapping_table[u4Offset / mtd->erasesize];
        u4PhyOffs = u4PhyBlkIdx * mtd->erasesize;

        if (u4PhyBlkIdx == 0xFFFFFFFF)
        {
            i4Ret = -1;
            printk(KERN_ERR "[i4NFBWriteBlock] Too many bad blocks!\n");
            break;
        }
        
        erase.addr = u4PhyOffs;
        erase.len  = mtd->erasesize;
        erase.mtd  = mtd;
        erase.callback = NULL;
        
        // Erase the block before write data
        i4Ret = mtd_erase(mtd, &erase);
        if (i4Ret == 0)
        {
            i4Ret = mtd_write(mtd, u4PhyOffs, mtd->erasesize, &u4RetLen, pu1MemPtr);
            if (i4Ret == 0 && u4RetLen == mtd->erasesize)
            {
                break;
            }
        }

        mtd_block_markbad(mtd, u4PhyOffs);
        i4NFBUpdateMappingTable(mtd);
    }

    return i4Ret;
}

INT32 i4NFBPartitionRead(UINT32 u4DevId, UINT32 u4Offset, UINT32 u4MemPtr, UINT32 u4MemLen)
{
    INT32   i4Ret = 0;
    UINT32  u4RdOffs, u4OffsInPage;
    UINT32  u4RdLen, u4RdByteCnt = 0;

    struct  mtd_info *mtd = get_mtd_device(NULL, u4DevId);

    if ((u4MemLen + u4Offset) > mtd->size)
    {
        printk(KERN_ERR "[i4NFBPartitionRead] Image 0x%x bytes, offset 0x%x bytes, part size 0x%llx bytes, part %d\n",
                 u4MemLen, u4Offset, mtd->size, u4DevId);
        printk(KERN_ERR "[i4NFBPartitionRead] Error: Input file can not fit into device!\n");
        return -1;
    }

    if (!badblock_mapping_table[mtd->index])
    {
        i4NFBUpdateMappingTable(mtd);
    }

    u4RdOffs = u4Offset;
    while (1) 
    {
        if (u4RdOffs >= mtd->size)
        {
            break;
        }
        
        if (u4RdByteCnt >= u4MemLen)
        {
            break;
        }

        // Handle read offset & size non-page-size-aligned case
        u4OffsInPage = u4RdOffs & (mtd->writesize - 1);
        if ((u4MemLen - u4RdByteCnt) < (mtd->writesize - u4OffsInPage))
        {
            u4RdLen = u4MemLen - u4RdByteCnt;
        }
        else
        {
            u4RdLen = mtd->writesize - u4OffsInPage;
        }
        
        // Read data and exit on failure
        i4Ret = i4NFBRead(mtd, u4RdOffs, (UINT8 *)(u4MemPtr + u4RdByteCnt), u4RdLen);
        if (i4Ret)
        {
            printk(KERN_ERR "[i4NFBPartitionRead] read error occur at: 0x%x\n", u4RdOffs);
            return i4Ret;
        }
        
        u4RdOffs += u4RdLen;
        u4RdByteCnt += u4RdLen;
    }
    
    return 0;
}

#define NFB_WRITE_VERIFY    0
INT32 i4NFBPartitionWrite(UINT32 u4DevId, UINT32 u4Offset, UINT32 u4MemPtr, UINT32 u4MemLen)
{
    INT32   i4Ret = 0;
    UINT32  u4RestWtSize = u4MemLen;
    UINT8 *pu1MemPtr = (UINT8 *)u4MemPtr;
    UINT8   *pu1BlockBuf = NULL;
    UINT32  u4OffsInBlk, u4WtBlkOffs;
    
    struct  mtd_info *mtd = get_mtd_device(NULL, u4DevId);
    
    if ((u4MemLen + u4Offset) > mtd->size) 
    {
        printk(KERN_ERR "[i4NFBPartitionWrite] Image 0x%x bytes, offset 0x%x bytes, part size 0x%llx bytes, part %d\n",
                 u4MemLen, u4Offset, mtd->size, u4DevId);
        printk(KERN_ERR "[i4NFBPartitionWrite] Error: Input file can not fit into device!\n");
        
        i4Ret = -1;
        goto closeall;
    }
    
    if (!badblock_mapping_table[mtd->index])
    {
        i4NFBUpdateMappingTable(mtd);
    }

    pu1BlockBuf = vmalloc(mtd->erasesize);
    if (!pu1BlockBuf)
    {
        printk(KERN_ERR "[i4NFBPartitionWrite] allocate block buffer memory failed!\n");
        
        i4Ret = -ENOMEM;
        goto closeall;
    }

    u4OffsInBlk = u4Offset & (mtd->erasesize - 1);
    u4WtBlkOffs = u4Offset - u4OffsInBlk;
    
    // Handle offset non-block-size-aligned case
    if (u4OffsInBlk)
    {
        UINT32 u4WriteLen;
        
        if (i4NFBRead(mtd, u4WtBlkOffs, pu1BlockBuf, mtd->erasesize))
        {
            printk(KERN_ERR "[i4NFBPartitionWrite] read block addr %d failed!\n", u4WtBlkOffs);
            
            i4Ret = -1;
            goto closeall;
        }

        if (u4RestWtSize >= (mtd->erasesize - u4OffsInBlk))
        {
            u4WriteLen = mtd->erasesize - u4OffsInBlk;
        }
        else
        {
            u4WriteLen = u4RestWtSize;
        }

        memcpy(pu1BlockBuf, pu1MemPtr + u4OffsInBlk, u4WriteLen);

        if (i4NFBWriteBlock(mtd, u4WtBlkOffs, pu1BlockBuf))
        {
            printk(KERN_ERR "[i4NFBPartitionWrite] write block addr %d failed!\n", u4WtBlkOffs);
            
            i4Ret = -1;
            goto closeall;
        }
        
        u4RestWtSize -= u4WriteLen;
        pu1MemPtr += u4WriteLen;
        u4WtBlkOffs += mtd->erasesize;
    }
    
    // Handle offset block-size-aligned case
    while (u4RestWtSize >= mtd->erasesize)
    {
        if (i4NFBWriteBlock(mtd, u4WtBlkOffs, pu1MemPtr))
        {
            printk(KERN_ERR "[i4NFBPartitionWrite] write block addr %d failed!\n", u4WtBlkOffs);
            
            i4Ret = -1;
            goto closeall;
        }
        
        u4RestWtSize -= mtd->erasesize;
        pu1MemPtr += mtd->erasesize;
        u4WtBlkOffs += mtd->erasesize;
    }
    
    // Handle write size none-block-size-aligned case
    if (u4RestWtSize)
    {
        if (i4NFBRead(mtd, u4WtBlkOffs, pu1BlockBuf, mtd->erasesize))
        {
            printk(KERN_ERR "[i4NFBPartitionWrite] read block addr %d failed!\n", u4WtBlkOffs);

            i4Ret = -1;
            goto closeall;
        }

        memcpy(pu1BlockBuf, pu1MemPtr, u4RestWtSize);

        if (i4NFBWriteBlock(mtd, u4WtBlkOffs, pu1BlockBuf))
        {
            printk(KERN_ERR "[i4NFBPartitionWrite] write block addr %d failed!\n", u4WtBlkOffs);

            i4Ret = -1;
            goto closeall;
        }
    }

closeall:
    if (pu1BlockBuf)
    {
        vfree(pu1BlockBuf);
    }
    
#if NFB_WRITE_VERIFY
    // Read back for verify
    if (i4Ret == 0)
    {
        UINT8 *pu1VerfBuf = vmalloc(u4MemLen);
        if (!pu1VerfBuf)
        {
            printk(KERN_ERR "[i4NFBPartitionWrite] alloc verify memory failed!\n");
            return -ENOMEM;
        }
        
        i4Ret = i4NFBPartitionRead(u4DevId, u4Offset, (UINT32)pu1VerfBuf, u4MemLen);
        if (i4Ret)
        {
            printk(KERN_ERR "[i4NFBPartitionWrite] Read back failed!\n");
            BUG();
        }
        
        i4Ret = memcmp((void *)u4MemPtr, pu1VerfBuf, u4MemLen);
        if (i4Ret)
        {
            printk(KERN_ERR "[i4NFBPartitionWrite] Fatal Error: Read back compare failed!!\n");
            BUG();
        }
        else
        {
            printk(KERN_INFO "[i4NFBPartitionWrite] Read back compare check OK^^\n");
        }
        
        vfree(pu1VerfBuf);
    }
#endif

    /* Return happy */
    return i4Ret;
}

EXPORT_SYMBOL(i4NFBPartitionRead);
EXPORT_SYMBOL(i4NFBPartitionWrite);

