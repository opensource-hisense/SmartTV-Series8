/*
 * mtk emmc partition operation functions
 */

#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/semaphore.h>
#include <linux/scatterlist.h>
#include <linux/blkdev.h>
#include <linux/module.h>

#include <linux/mmc/host.h>
#include <linux/mmc/card.h>
#include <linux/mmc/mmc.h>
#include <linux/mmc/sd.h>
#include <linux/mmc/mtk_msdc_part.h>

#include "mtk_msdc_host_hw.h"
#include "mtk_msdc_slave_hw.h"
#include "mtk_msdc_drv.h"

struct msdc_host *msdc_host_boot;
int (*msdc_send_command)(struct msdc_host *host, struct mmc_command *cmd);

// defined in /fs/partitions/mtkemmc.c
extern mtk_partition mtk_msdc_partition;
extern int msdcpart_setup_real(void);

// The max I/O is 32KB for finetune r/w performance.
#define MSDC_MAXPAGE_ORD    3
#define MSDC_MAXBUF_CNT     (PAGE_SIZE * (1<<MSDC_MAXPAGE_ORD))
static struct page *_blkpages = NULL;
static u_char *_pu1blkbuf = NULL;

static int msdc_check_partition(void)
{
    return msdcpart_setup_real();
}

static int msdc_alloc_reqpages(void)
{
    if (!_blkpages)
    {
        _blkpages = alloc_pages(GFP_KERNEL, MSDC_MAXPAGE_ORD);
        if (!_blkpages)
        {
            printk(KERN_ERR "alloc raw r/w page buffer fail!\n");
            return -ENOMEM;
        }

        _pu1blkbuf = (u_char *)page_address(_blkpages);
    }

    return 0;
}

static char *msdc_getpart_name(int partno)
{
    BUG_ON(partno<1 ||partno>mtk_msdc_partition.nparts);
    return mtk_msdc_partition.parts[partno].name;
}

static uint64_t msdc_getpart_offs(int partno)
{
    BUG_ON(partno<1 ||partno>mtk_msdc_partition.nparts);
    return mtk_msdc_partition.parts[partno].start_sect<<9;
}

static uint64_t msdc_getpart_size(int partno)
{
    BUG_ON(partno<1 ||partno>mtk_msdc_partition.nparts);
    return mtk_msdc_partition.parts[partno].nr_sects<<9;
}

#if 0
static int msdc_switch_part(char whichpart)
{
    struct mmc_host *mmc = msdc_host_boot->mmc;
    struct mmc_card *card = mmc->card;
    struct mmc_command cmd;

    BUG_ON(!card);
    BUG_ON(!card->host);	

    down(&msdc_host_boot->msdc_sema);

    memset(&cmd,  0, sizeof(struct mmc_command));
    cmd.data = NULL;

    cmd.opcode = MMC_SWITCH;
    cmd.arg = (MMC_SWITCH_MODE_WRITE_BYTE << 24) |
		  (EXT_CSD_PART_CONFIG << 16) |
		  (0x78 << 8) |
		  EXT_CSD_CMD_SET_NORMAL;
    cmd.flags = MMC_RSP_SPI_R1B | MMC_RSP_R1B | MMC_CMD_AC;
    
    msdc_send_command(msdc_host_boot, &cmd);
    
	up(&msdc_host_boot->msdc_sema);
    
    if (cmd.error)
    {
        printk(KERN_ERR "msdc_switch_part fail!\n");
        return -1;
    }

	return 0;

}
#endif

static int msdc_sendrequest(uint64_t offset, uint64_t size, struct page *page, int read)
{
    u32 readcmd, writecmd;
    struct mmc_host *mmc = msdc_host_boot->mmc;
    struct mmc_card *card = mmc->card;
    struct mmc_command cmd, stop;
    struct mmc_data data;
    struct scatterlist sg;
    
    BUG_ON((offset%512) || (size%512));
    BUG_ON(size > MSDC_MAXBUF_CNT);
    
    // switch to user-area access
    //if(msdc_switch_part(0x00))
        //printk(KERN_ERR "switch part access to usr-area failed!\n");
    
    memset(&cmd,  0, sizeof(struct mmc_command));
    memset(&stop, 0, sizeof(struct mmc_command));
    memset(&data, 0, sizeof(struct mmc_data));
    cmd.data = &data;
    
    if (mmc_card_blockaddr(card))
    {
        cmd.arg = offset>>9;
    }
    else
    {
        cmd.arg = offset;
    }
    
    cmd.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_ADTC;
    data.blksz = 512;
    stop.opcode = MMC_STOP_TRANSMISSION;
    stop.arg = 0;
    stop.flags = MMC_RSP_SPI_R1B | MMC_RSP_R1B | MMC_CMD_AC;
    data.blocks = size>>9;

    if (data.blocks > 1)
    {
        // Add this for auto command12
        data.stop = &stop;
    	readcmd = MMC_READ_MULTIPLE_BLOCK;
    	writecmd = MMC_WRITE_MULTIPLE_BLOCK;
    }
    else
    {
    	readcmd = MMC_READ_SINGLE_BLOCK;
    	writecmd = MMC_WRITE_BLOCK;
    }
        
    if (read)
    {
        cmd.opcode = readcmd;
        data.flags |= MMC_DATA_READ;
    }
    else
    {
        cmd.opcode = writecmd;
        data.flags |= MMC_DATA_WRITE;
    }
    
    mmc_set_data_timeout(&data, card);
    
    sg_init_table(&sg, 1);
    sg_set_page(&sg, page, size, 0);
    sg_mark_end(&sg);
    
	data.sg = &sg;
	data.sg_len = 1;
    
    down(&msdc_host_boot->msdc_sema);
    
    msdc_send_command(msdc_host_boot, &cmd);
    
	up(&msdc_host_boot->msdc_sema);
    
    if (cmd.error)
    {
        printk(KERN_ERR "msdc_sendrequest fail: offset=0x%016llX, size=0x%016llX, read=%d!\n", offset, size, read);
        return -1;
    }
    
    return 0;
}


int msdc_partread(int partno, uint64_t offset, uint64_t size, void *pvmem)
{
    uint64_t u8partoffs, u8partsize;
    
    u_char *p_buffer = (u_char *)pvmem;
    int read_length, left_to_read = size;
        
    BUG_ON(msdc_check_partition());

    u8partoffs = msdc_getpart_offs(partno);
    u8partsize = msdc_getpart_size(partno);

    BUG_ON(offset+size > u8partsize);

    if (msdc_alloc_reqpages())
    {
        goto read_fail;
    }

    offset += u8partoffs;

    // read non-block alignment offset.
    if (offset % 512)
    {
        int block_offset;
        
        block_offset = offset % 512;
        offset -= block_offset;
        
		if (left_to_read < (512 - block_offset))
		{
			read_length = left_to_read;
		}
		else
		{
			read_length = 512 - block_offset;
		}

        if (msdc_sendrequest(offset, 512, _blkpages, 1))
        {
            goto read_fail;
        }
        
        memcpy(p_buffer, _pu1blkbuf + block_offset, read_length);

        offset += 512;
        left_to_read -= read_length;
        p_buffer += read_length;
    }

    // read block alignment offset and size. (MSDC_MAXBUF_CNT)
    while (left_to_read >= MSDC_MAXBUF_CNT)
    {
        read_length = MSDC_MAXBUF_CNT;
        
        if (msdc_sendrequest(offset, read_length, _blkpages, 1))
        {
            goto read_fail;
        }
        
        memcpy(p_buffer, _pu1blkbuf, read_length);
        
        offset += read_length;
        left_to_read -= read_length;
        p_buffer += read_length;
    }
    
    // read block alignment offset and size. (X512B)
    if (left_to_read >= 512)
    {
        read_length = (left_to_read>>9)<<9;
        
        if (msdc_sendrequest(offset, read_length, _blkpages, 1))
        {
            goto read_fail;
        }
        
        memcpy(p_buffer, _pu1blkbuf, read_length);
        
        offset += read_length;
        left_to_read -= read_length;
        p_buffer += read_length;
    }

    // read non-block alignment size.
    if (left_to_read)
    {
        read_length = left_to_read;
        
        if (msdc_sendrequest(offset, 512, _blkpages, 1))
        {
            goto read_fail;
        }
        
        memcpy(p_buffer, _pu1blkbuf, read_length);
    }
    
    return 0;

read_fail:
    printk(KERN_ERR "msdc_partread fail: partno=%d, offset=0x%llX, size=0x%llX, pvmem=0x%08X\n", 
        partno, offset, size, (int)pvmem);
    return -1;
}

int msdc_partwrite(int partno, uint64_t offset, uint64_t size, void *pvmem)
{
    uint64_t u8partoffs, u8partsize;
    
    u_char *p_buffer = (u_char *)pvmem;
    int write_length, left_to_write = size;
    
    BUG_ON(msdc_check_partition());

    u8partoffs = msdc_getpart_offs(partno);
    u8partsize = msdc_getpart_size(partno);

    BUG_ON(offset+size > u8partsize);
    
    if (msdc_alloc_reqpages())
    {
        goto write_fail;
    }
    
    offset += u8partoffs;

    // write non-block alignment offset.
    if (offset % 512)
    {
        int block_offset;
        
        block_offset = offset % 512;
        offset -= block_offset;
        
        if (left_to_write < (512 - block_offset))
        {
            write_length = left_to_write;
        }
        else
        {
            write_length = 512 - block_offset;
        }

        if (msdc_sendrequest(offset, 512, _blkpages, 1))
        {
            goto write_fail;
        }
        
        memcpy(_pu1blkbuf + block_offset, p_buffer, write_length);
        
        if (msdc_sendrequest(offset, 512, _blkpages, 0))
        {
            goto write_fail;
        }

        offset += 512;
        left_to_write -= write_length;
        p_buffer += write_length;
    }

    // write block alignment offset and size.  (MSDC_MAXBUF_CNT)
    while (left_to_write >= MSDC_MAXBUF_CNT)
    {
        write_length = MSDC_MAXBUF_CNT;
        
        memcpy(_pu1blkbuf, p_buffer, write_length);
        
        if (msdc_sendrequest(offset, write_length, _blkpages, 0))
        {
            goto write_fail;
        }
        
        offset += write_length;
        left_to_write -= write_length;
        p_buffer += write_length;
    }
    
    // write block alignment offset and size. (X512B)
    if (left_to_write >= 512)
    {
        write_length = (left_to_write>>9)<<9;
        
        memcpy(_pu1blkbuf, p_buffer, write_length);
        
        if (msdc_sendrequest(offset, write_length, _blkpages, 0))
        {
            goto write_fail;
        }
        
        offset += write_length;
        left_to_write -= write_length;
        p_buffer += write_length;
    }

    // write non-block alignment size.
    if (left_to_write)
    {
        write_length = left_to_write;
        
        if (msdc_sendrequest(offset, 512, _blkpages, 1))
        {
            goto write_fail;
        }
        
        memcpy(_pu1blkbuf, p_buffer, write_length);
        
        if (msdc_sendrequest(offset, 512, _blkpages, 0))
        {
            goto write_fail;
        }
    }
    
    return 0;
    
write_fail:
    printk(KERN_ERR "msdc_partwrite fail: partno=%d, offset=0x%llX, size=0x%llX, pvmem=0x%08X\n", 
        partno, offset, size, (int)pvmem);
    return -1;
}

int msdc_partread_byname(const char *name, uint64_t offset, uint64_t size, void *pvmem)
{
    int partno;

    BUG_ON(!name);

    for (partno = 1; partno <= mtk_msdc_partition.nparts; partno++)
    {
        if (strstr(msdc_getpart_name(partno), name))
        {
            return msdc_partread(partno, offset, size, pvmem);
        }
    }

    return -1;
}

int msdc_partwrite_byname(const char *name, uint64_t offset, uint64_t size, void *pvmem)
{
    int partno;

    BUG_ON(!name);

    for (partno = 1; partno <= mtk_msdc_partition.nparts; partno++)
    {
        if (strstr(msdc_getpart_name(partno), name))
        {
            return msdc_partwrite(partno, offset, size, pvmem);
        }
    }

    return -1;
}

EXPORT_SYMBOL(msdc_host_boot);
EXPORT_SYMBOL(msdc_send_command);
EXPORT_SYMBOL(msdc_partread);
EXPORT_SYMBOL(msdc_partwrite);
EXPORT_SYMBOL(msdc_partread_byname);
EXPORT_SYMBOL(msdc_partwrite_byname);

