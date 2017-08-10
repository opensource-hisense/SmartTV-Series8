/*
 * mt85xx partition operation functions
 * For IC Verify use Linux nand Native drivers.
 */
#ifndef NAND_VFY_DRIVER_C
#define NAND_VFY_DRIVER_C

#include "nand_vfy_driver.h"
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/kernel.h>

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


#define CONFIG_DRV_FPGA_BOARD 0

#define MAX_PAGE_SIZE	8192
#define MAX_SPASE_SIZE  576


#undef  ASSERT
#define ASSERT(e) \
        if(!(e)){ \
          while(1); \
        }
		
static BOOL _fgDmaEn1 = TRUE;

static void erase_buffer(void *buffer, size_t size)
{
	const uint8_t kEraseByte = 0xff;

	if (buffer != NULL && size > 0) 
	{
		memset(buffer, kEraseByte, size);
	}
}


INT32 i4NFIVerifyReadPage(UINT32 u4DevId, UINT32 u8Offset, UINT32 u4MemPtr, UINT32 u4MemLen)
{
    unsigned long start_addr;        // start address
    unsigned long ofs;
    unsigned long long blockstart = 1;
    int  pagelen, badblock = 0;
    UINT32 u4RdByteCnt = 0;
	UINT32 u4RdLen = 0;
	UINT32 i4DataUnit;
    INT32 i4Ret = 0;
	UINT8 *pu1MemBuf = (UINT8*)u4MemPtr;
	UINT8 *pu1TempBuf = NULL;
	
	struct mtd_info *mtd = get_mtd_device(NULL,u4DevId);
	
    printk(KERN_INFO "[NFI Verify read]u4DevId =%d, u8Offset =0x%x, u4MemPtr =0x%x, u4MemLen =0x%x \n", \
		    u4DevId, u8Offset, u4MemPtr, u4MemLen);

	ASSERT(pu1MemBuf != NULL);
	//get_mtd_device(mtd, u4DevId);

    /* Initialize page size */
    pagelen = mtd->writesize;
    start_addr = u8Offset;        // start address
    // Determine data unit for temp buffer
    i4DataUnit = (u4MemLen / mtd->writesize) * mtd->writesize;
    if ((u4MemLen % mtd->writesize) != 0)
    {
        i4DataUnit += mtd->writesize;
    }
    
    // Alloc Temp buf for reading data
    //pu1TempBuf = kzalloc(i4DataUnit,GFP_KERNEL);
    pu1TempBuf = kzalloc(i4DataUnit+mtd->oobsize,GFP_KERNEL);
    ASSERT(pu1TempBuf != NULL);
    /* Dump the flash contents */   
    for (ofs = start_addr; (unsigned long long)ofs < mtd->size && u4RdByteCnt < u4MemLen ; ofs+=pagelen) 
    {
        // new eraseblock , check for bad block
        if (blockstart != (ofs & (~mtd->erasesize + 1))) 
        {
            blockstart = ofs & (~mtd->erasesize + 1);
			if ((badblock = mtd->_block_isbad(mtd, blockstart)) < 0)
          //  if ((badblock = ioctl(fd, MEMGETBADBLOCK, &blockstart)) < 0) 
            {
                printk(KERN_INFO "[NFI Verify read]Error check bad block\n");
                goto closeall;
            }
        }

        if (badblock) 
        {
            // Start address changed
            start_addr = ofs;
  //          printk(KERN_INFO "[NFI Verify read] partition_%d - bad block found at : 0x%x, start_addr = 0x%x\n", u4DevId, ofs, start_addr);
            continue;
        }
        else 
        {        
            /* Read page data and exit on failure */                   
            i4Ret = mtd->_read(mtd, ofs, pagelen, (size_t *)&u4RdLen, (u_char *)(pu1TempBuf + u4RdByteCnt));   
        
            if (i4Ret)
            {
       //         printk(KERN_INFO "[NFI Verify read]Page read error occur at : 0x%x, Ret=%d\n", ofs, i4Ret);
                goto closeall;
            }

            u4RdByteCnt += (UINT32)u4RdLen;
        }

    }

    if (u4RdByteCnt < u4MemLen)
    {
        printk(KERN_INFO "[NFI Verify read]Can't read enough data, u4RdByteCnt = 0x%x, u4MemLen = 0x%x\n", u4RdByteCnt, u4MemLen);        
        goto closeall;
    }
	
	if (_fgDmaEn1) {
    	//BSP_FlushDCacheRange((UINT32)pu1TempBuf, u4MemLen);
		memcpy(pu1MemBuf, pu1TempBuf, u4MemLen);
	}
	else {
    	//BSP_FlushDCacheRange((UINT32)pu1TempBuf, (u4MemLen+mtd->oobsize));
		memcpy(pu1MemBuf, pu1TempBuf, (u4MemLen+mtd->oobsize));
	}
    //printk(KERN_INFO "[nand part read] successful\n");
    /* Exit happy */
	kfree(pu1TempBuf);
    return i4Ret;

closeall:    
    kfree(pu1TempBuf);
    i4Ret = -1;    
    return i4Ret;
}

INT32 i4NFIVerifyWritePage(UINT32 u4DevId, UINT32 u8Offset , UINT32 u4MemPtr, UINT32 u4MemLen)
{
    int imglen = u4MemLen, pagelen, sparelen;
	size_t retlen=0;
    UINT8 baderaseblock = FALSE;
    int blockstart = -1;
	loff_t offs;    
    int readlen;
    UINT32 mtdoffset;
    INT32 i4Ret = 0;
    UINT32 u4WtByteCnt = 0;
	UINT8  *pu1MemBuf = (UINT8*)u4MemPtr;
	u_char *pu1TempMemBuf = NULL;
	struct erase_info *erase;
	struct mtd_info *mtd = get_mtd_device(NULL,u4DevId);
    	
    erase = kzalloc(sizeof(struct erase_info),GFP_KERNEL);
	if (!erase)
	{
	  printk(KERN_INFO "[NFI Verify write]allocate erase info memo failed\n");
	  return -ENOMEM;
	}
	
    ASSERT(pu1MemBuf != NULL);
    //get_mtd_device(mtd, u4DevId);
   
    mtdoffset = u8Offset;        // start address
    
    printk(KERN_INFO "[NFI Verify write]u4DevId =%d, u8Offset =0x%x, u4MemPtr =0x%x, u4MemLen =0x%x \n", \
		    u4DevId, u8Offset, u4MemPtr, u4MemLen);
    
    pagelen = mtd->writesize;
	sparelen = mtd->oobsize;

    /*
     * For the standard input case, the input size is merely an
     * invariant placeholder and is set to the write page
     * size. Otherwise, just use the input file size.
     *
     * TODO: Add support for the -l,--length=length option (see
     * previous discussion by Tommi Airikka <tommi.airikka@ericsson.com> at
     * <http://lists.infradead.org/pipermail/linux-mtd/2008-September/
     * 022913.html>
     */

    // Check, if length fits into device
    if ( ((imglen / pagelen) * mtd->writesize) > (mtd->size - mtdoffset)) 
    {
      //  printk(KERN_INFO "[NFI Verify write] Image %d bytes, NAND page %d bytes, OOB area %u bytes, device size %u bytes\n",
      //           imglen, pagelen, mtd->writesize, mtd->size);
        printk(KERN_INFO "[NFI Verify write] Error : Input file can not fit into device\n");        
        i4Ret = -1;
        goto closeall;
    }
    pu1TempMemBuf = kzalloc((MAX_PAGE_SIZE+MAX_SPASE_SIZE),GFP_KERNEL);
    ASSERT(pu1TempMemBuf != NULL);

    /*
     * Get data from input and write to the device while there is
     * still input to read and we are still within the device
     * bounds. Note that in the case of standard input, the input
     * length is simply a quasi-boolean flag whose values are page
     * length or zero.
     */    
    while (imglen && ((unsigned long long)mtdoffset < mtd->size)) 
    {
        // new eraseblock , check for bad block(s)
        // Stay in the loop to be sure if the mtdoffset changes because
        // of a bad block, that the next block that will be written to
        // is also checked. Thus avoiding errors if the block(s) after the
        // skipped block(s) is also bad (number of blocks depending on
        // the blockalign
        while (blockstart != (mtdoffset & (~mtd->erasesize + 1))) 
        {
            blockstart = mtdoffset & (~mtd->erasesize + 1);
            offs = blockstart;
            baderaseblock = FALSE;
            printk(KERN_INFO "[NFI Verify write]Try to writing data to block %d at offset 0x%x\n",
                         blockstart / mtd->erasesize, mtdoffset);

            /* Check all the blocks in an erase block for bad blocks */
            do 
            {
                if ((i4Ret = mtd->_block_isbad(mtd, offs)) < 0) 
                {
                    printk(KERN_INFO "[NFI Verify write]Error check bad block\n");
                    i4Ret = -1;
                    goto closeall;
                }
                
                if (i4Ret == 1) 
                {
                    baderaseblock = TRUE;
                    printk(KERN_INFO "[NFI Verify write]=> Bad block at partition_%d, %u block(s) from offset 0x%x will be skipped\n", u4DevId, (int)offs, blockstart);
                }
                else
                {
                    ;//NFIVerify_LOG ("[NFI Verify write]=> Good block\n");
                }

                if (baderaseblock) 
                {
                    mtdoffset = blockstart + mtd->erasesize;
                }
                offs +=  mtd->erasesize;
            } 
            while ( offs < blockstart + mtd->erasesize );

            if (!baderaseblock)
            {
                break;
            }
        }

        readlen = mtd->writesize;

        // Pad 0xFF in the end of the write buffer, it it is not fit
        if (imglen < readlen)
        {
            readlen = imglen;
            erase_buffer((u_char *)(pu1TempMemBuf + readlen), (size_t)(mtd->writesize - readlen));
        }
        //memcpy(pu1TempMemBuf, (pu1MemBuf+u4WtByteCnt), readlen);
        memcpy(pu1TempMemBuf, (pu1MemBuf+u4WtByteCnt), readlen+sparelen);
        
        //printk("[NFI CLI]pu1TempMemBuf=0x%x, pu1MemBuf=0x%x\n", pu1TempMemBuf, (pu1MemBuf+u4WtByteCnt));   
        if (memcmp(pu1TempMemBuf, (pu1MemBuf+u4WtByteCnt), (readlen+sparelen)))
        {
            return 0;
        }

        /* Write out the Page data */
        if (mtd->_write(mtd, mtdoffset, mtd->writesize, &retlen, (u_char *)(pu1TempMemBuf)))
        {
            int rewind_blocks;
          //  struct erase_info *erase;
            printk(KERN_INFO "[NFI Verify write] mtd->write error at offset 0x%x\n", mtdoffset);

            /* Must rewind to blockstart if we can */
            rewind_blocks = (mtdoffset - blockstart) / mtd->writesize; /* Not including the one we just attempted */

            {
                wait_queue_head_t waitq;
    			DECLARE_WAITQUEUE(wait, current);
    			init_waitqueue_head(&waitq);
                
                
                erase->addr = blockstart;
                erase->len = mtd->erasesize;
    			erase->mtd = mtd;
    			//erase->callback = mtdchar_erase_callback;
    			//erase->priv = (unsigned long)&waitq;
    				
                printk(KERN_INFO "[NFI Verify write] Erasing failed write from 0x%08lX-0x%08lX\n", 
                    (long)erase->addr, (long)(erase->addr+erase->len-1));
                    
    			i4Ret = mtd->_erase(mtd, erase);
    			if (!i4Ret) 
    			{
    				set_current_state(TASK_UNINTERRUPTIBLE);
    				add_wait_queue(&waitq, &wait);
    				if (erase->state != MTD_ERASE_DONE &&
    					erase->state != MTD_ERASE_FAILED)
    					schedule();
    				remove_wait_queue(&waitq, &wait);
    				set_current_state(TASK_RUNNING);
    					
    				i4Ret = (erase->state == MTD_ERASE_FAILED)?-5:0;
    		    }
    			if (i4Ret != 0)
    			{
    				printk(KERN_INFO "[NFI Verify write] block erase failed !!");
    			    goto closeall;
    			}
            }

            // Mark bad block
            {
                loff_t bad_addr = mtdoffset & (~mtd->erasesize + 1);
                
                printk(KERN_INFO "[NFI Verify write]Marking block at 0x%08lX bad\n", (long)bad_addr);
                
                if (mtd->_block_markbad(mtd, bad_addr)) 
                {
                    printk(KERN_INFO "[NFI Verify write]block mark bad block Error !!\n");                    
                    /* But continue anyway */
                }
            }
            
            mtdoffset = blockstart + mtd->erasesize;
            imglen += rewind_blocks * mtd->writesize;
            u4WtByteCnt -= rewind_blocks * mtd->writesize;
  
            continue;
        }
        
        imglen -= readlen;
        u4WtByteCnt += readlen;
        
        mtdoffset +=mtd->writesize;
    }

closeall:
    kfree(erase);
	kfree(pu1TempMemBuf);
    if (imglen > 0) 
    {
        printk(KERN_INFO "[NFI Verify write]Data was only partially written due to error, imglen = 0x%x\n", imglen);
        i4Ret = -1;
    }
    
    /* Return happy */
    return i4Ret;
}

INT32 i4NFIVerifyErase(UINT32 u4DevId, UINT32 u8Offset)
{
    UINT8 baderaseblock = FALSE;
    int blockstart = -1;
    loff_t offs;    
    UINT32 mtdoffset;
    INT32 i4Ret = 0;
    struct erase_info *erase;
	struct mtd_info *mtd = get_mtd_device(NULL,u4DevId);
        
    erase = kzalloc(sizeof(struct erase_info),GFP_KERNEL);
    if (!erase)
    {
      printk(KERN_INFO "[NFI Verify erase]allocate erase info memo failed\n");
      return -ENOMEM;
    }
    
   // get_mtd_device(mtd, u4DevId);
   
    mtdoffset = u8Offset;        // start address  block
    
    printk(KERN_INFO "[NFI Verify erase]u4DevId = %d, u8Offset = 0x%x\n", u4DevId, u8Offset);
    
    // new eraseblock , check for bad block(s)
    // Stay in the loop to be sure if the mtdoffset changes because
    // of a bad block, that the next block that will be written to
    // is also checked. Thus avoiding errors if the block(s) after the
    // skipped block(s) is also bad (number of blocks depending on
    // the blockalign
    blockstart = mtdoffset & (~mtd->erasesize + 1);
    offs = blockstart;
    baderaseblock = FALSE;
    printk(KERN_INFO "[NFI Verify erase]Try to erase block %d\n", (blockstart/mtd->erasesize));

    /* Check all the blocks in an erase block for bad blocks */
    do 
    {
        if ((i4Ret = mtd->_block_isbad(mtd, offs)) < 0) 
        {
            printk(KERN_INFO "[NFI Verify erase]Error check bad block\n");
            i4Ret = -1;
            goto closeall;
        }
        
        if (i4Ret == 1) 
        {
            baderaseblock = TRUE;
            printk(KERN_INFO "[NFI Verify erase]=> Bad block at partition_%d, %u block(s) from offset 0x%x will be skipped\n", 
                u4DevId, (int)offs, blockstart);
        }
        else
        {
            baderaseblock = FALSE;
            ;//NFIVerify_LOG ("[NFI Verify erase]=> Good block\n");
        }

        if (baderaseblock) 
        {
            mtdoffset = blockstart + mtd->erasesize;
        }
        offs +=  mtd->erasesize;
    } 
    while ( offs < blockstart + mtd->erasesize );

    if (!baderaseblock)
    {
        // Erase the erase-block before write data into the block  
        wait_queue_head_t waitq;
        DECLARE_WAITQUEUE(wait, current);
        init_waitqueue_head(&waitq);
        
        erase->addr = blockstart;
        erase->len = mtd->erasesize;
        erase->mtd = mtd;
        //erase->callback = mtdchar_erase_callback;
        erase->priv = (unsigned long)&waitq;
        
  //      printk(KERN_INFO "[NFI Verify erase]Erasing this block before write from 0x%08lX-0x%08lX\n", 
//            (long)erase->addr, (long)erase->addr+erase->len-1);
        i4Ret = mtd->_erase(mtd, erase);
        if (!i4Ret) 
        {
            set_current_state(TASK_UNINTERRUPTIBLE);
            add_wait_queue(&waitq, &wait);
            if (erase->state != MTD_ERASE_DONE &&
                erase->state != MTD_ERASE_FAILED)
                schedule();
            remove_wait_queue(&waitq, &wait);
            set_current_state(TASK_RUNNING);

            i4Ret = (erase->state == MTD_ERASE_FAILED)?-5:0;
        }
        if (i4Ret != 0)
        {
            printk(KERN_INFO "[NFI Verify erase]Block erase failed !!");
            goto closeall;
        }
    }

closeall:
    kfree(erase);
    
    /* Return happy */
    return i4Ret;
}

INT32 i4NFIVerifyPerformance(UINT32 u4DevId, UINT32 u4StartBlock, UINT32 u4BlockCount)
{
    unsigned long start_addr;        // start address
    unsigned long ofs;
    int  blocklen;
	UINT32 u4RdLen = 0;
	UINT32 i4DataUnit;
    INT32 i4Ret = 0;
	UINT8 *pu1MemBuf = NULL;
	struct mtd_info *mtd = get_mtd_device(NULL,u4DevId);
    UINT32 i;    
	struct timeval tv1;
	struct timeval tv2;
	UINT32 u4DataSize;
   


    blocklen = mtd->erasesize;

    start_addr = u4StartBlock *blocklen;        // start address
    
    // Determine data unit for temp buffer
    i4DataUnit = mtd->erasesize;
    // Alloc Temp buf for reading data
    pu1MemBuf = kzalloc(i4DataUnit, GFP_KERNEL);
    ASSERT(pu1MemBuf != NULL);

    // Read Performance Test
    /* Dump the flash contents */  
    ofs = start_addr;

	do_gettimeofday(&tv1);
    for (i=0; (ofs <mtd->size)&&(i<u4BlockCount); ofs+=blocklen, i++) 
    {
        /* Read page data and exit on failure */                   
        i4Ret = mtd->_read(mtd, ofs, blocklen, (size_t *)&u4RdLen, (u_char *)(pu1MemBuf));        
    
        if (i4Ret)
        {
            printk(KERN_INFO "[NFI Verify Per]Block read error occur at : 0x%lx\n", ofs);
            goto closeall;
        }
    }
    

    do_gettimeofday(&tv2);
 	printk("[NFI Verify Per]GetRefTime(s) %ld.%ld\n", (tv2.tv_sec - tv1.tv_sec), (tv2.tv_usec - tv1.tv_usec));
 
    u4DataSize = (blocklen * u4BlockCount) / 1024;
    printk("[NFI Verify Per]ReadDataSize %dKB\n", u4DataSize);
   
    
    // Write Performance Test    
    ofs = start_addr;
	
	do_gettimeofday(&tv1);


    for (i=0; (ofs <mtd->size)&&(i<u4BlockCount); ofs+=blocklen, i++) 
    {
        /* Read page data and exit on failure */         
        i4Ret = mtd->_write(mtd, ofs, blocklen, (size_t *)&u4RdLen, (u_char *)(pu1MemBuf));    
    
        if (i4Ret)
        {
            printk(KERN_INFO "[NFI Verify Per]Block write error occur at : 0x%lx\n", ofs);
             kfree(pu1MemBuf);
   			 i4Ret = -1;    
    		return i4Ret;
        }
    }
    

    do_gettimeofday(&tv2);
    printk("[NFI Verify Per]GetRefTime(s) %ld.%ld\n", (tv2.tv_sec - tv1.tv_sec), (tv2.tv_usec - tv1.tv_usec));
    u4DataSize = (blocklen * u4BlockCount) / 1024;
    printk("[NFI Verify Per]WriteDataSize %dKB\n", u4DataSize);
   


    /* Exit happy */
	kfree(pu1MemBuf);
    return i4Ret;

closeall:    
    kfree(pu1MemBuf);
    i4Ret = -1;    
    return i4Ret;
}


INT32 i4NFIVerify_Get_Information(UINT32 u4MaxDevice)
{
		struct mtd_info *mtd;
		INT32 i4Ret =0;
		UINT32 u8TotalSize;
		UINT32 u4BlockSize, u4PageSize, u4SpareSize;
	
		
		mtd = get_mtd_device(NULL, u4MaxDevice);
		u8TotalSize = (UINT32)mtd->size;
		u4BlockSize = mtd->erasesize;
		u4PageSize	= mtd->writesize;
		u4SpareSize = mtd->oobsize;
		
		printk(KERN_INFO "[NFI Information] %d MTD partitions\n", u4MaxDevice);
		printk(KERN_INFO "[NFI Information]  MTD partitions name is :%s\n", mtd->name);
		printk(KERN_INFO "[NFI Information] BlockSize=0x%x(%dKBytes), PagesPerBlock=%d\n", 
													u4BlockSize,(u4BlockSize/1024), (u4BlockSize/u4PageSize) );
		printk(KERN_INFO "[NFI Information] PageSize=0x%x(%d), SpareSize=0x%x(%d)\n",
															u4PageSize, u4PageSize, u4SpareSize, u4SpareSize);	  
		printk(KERN_INFO "[NFI Information] BlockCount=%d\n", 
															(u8TotalSize/u4BlockSize));
		
		return i4Ret;
}

EXPORT_SYMBOL(i4NFIVerifyReadPage);
EXPORT_SYMBOL(i4NFIVerifyWritePage);
EXPORT_SYMBOL(i4NFIVerifyErase);
EXPORT_SYMBOL(i4NFIVerifyPerformance);
EXPORT_SYMBOL(i4NFIVerify_Get_Information);

#endif

