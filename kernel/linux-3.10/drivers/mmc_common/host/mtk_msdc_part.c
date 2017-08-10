/*
 * mtk emmc partition operation functions
 */

#include <linux/slab.h>
#include <linux/scatterlist.h>
#include <linux/blkdev.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/mmc/host.h>
#include <linux/mmc/card.h>
#include <linux/mmc/mmc.h>
#include <linux/mmc/sd.h>
#include <linux/mmc/mtk_msdc_part.h>
#include <linux/dma-mapping.h>
#include <mtk_msdc.h>
#include <mtk_msdc_dbg.h>

extern mtk_partition mtk_msdc_partition;
extern int msdcpart_setup_real(void);
extern struct msdc_host *mtk_msdc_host[];
extern operation_type msdc_latest_operation_type[HOST_MAX_NUM];
extern void msdc_set_timeout(struct msdc_host *host, u32 ns, u32 clks);
extern void msdc_dma_start(struct msdc_host *host);
extern void msdc_dma_setup(struct msdc_host *host, struct msdc_dma *dma, struct scatterlist *sg, unsigned int sglen);
extern unsigned int msdc_command_resp_polling(struct msdc_host	 *host,
									  struct mmc_command *cmd,
									  int				  tune,
									  unsigned long		  timeout);						  
extern unsigned int msdc_command_start(struct msdc_host   *host,
									  struct mmc_command *cmd,
									  int				  tune,   /* not used */
									  unsigned long		  timeout);
extern void msdc_set_blknum(struct msdc_host *host, u32 blknum);
extern void msdc_dma_clear(struct msdc_host *host);
extern void msdc_dma_stop(struct msdc_host *host);

// The max I/O is 32KB for finetune r/w performance.
#define MSDC_MAXPAGE_ORD	3
#define MSDC_MAXBUF_CNT		(PAGE_SIZE * (1<<MSDC_MAXPAGE_ORD))
static struct page *_blkpages = NULL;
static u_char *_pu1blkbuf = NULL;

static int msdc_alloc_tempbuf(void)
{
	struct msdc_host *host = mtk_msdc_host[0];
	if (_pu1blkbuf == NULL)
	{
		#ifdef CC_SUPPORT_CHANNEL_C
		_pu1blkbuf = kmalloc(MSDC_MAXBUF_CNT, GFP_DMA);
		#else
		_pu1blkbuf = kmalloc(MSDC_MAXBUF_CNT, GFP_KERNEL);
		#endif
	}
	BUG_ON(_pu1blkbuf == NULL);
	ERR_MSG("_pu1blkbuf = 0x%p\n", _pu1blkbuf);
	return 0;
}

#if 0
static int msdc_alloc_reqpages(void)
{
	if (!_blkpages)
	{
		_blkpages = alloc_pages(GFP_KERNEL, MSDC_MAXPAGE_ORD);
		if (!_blkpages)
		{
			pr_err("alloc raw r/w page buffer fail!\n");
			return -ENOMEM;
		}

		_pu1blkbuf = (u_char *)page_address(_blkpages);
	}

	return 0;
}
#endif

#if 0
static void memset_alloc_pages(void)
{
	if(_pu1blkbuf)
	{
		memset(_pu1blkbuf, 0xe, MSDC_MAXPAGE_ORD * PAGE_SIZE);
	}
}

#endif

static int msdc_check_partition(void)
{
	return msdcpart_setup_real();
}

static char *msdc_getpart_name(int partno)
{
	BUG_ON(partno < 1 || partno > mtk_msdc_partition.nparts);
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
		//pr_err("switch part access to usr-area failed!\n");

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
		pr_err("msdc_sendrequest fail: offset=0x%016llX, size=0x%016llX, read=%d!\n", offset, size, read);
		return -1;
	}

	return 0;
}
#endif

static int msdc_request(uint64_t offset, uint64_t size, void *buf, int read)
{

	BUG_ON((offset%512) || (size%512));
	BUG_ON(size > MSDC_MAXBUF_CNT);

	struct msdc_host *host = mtk_msdc_host[0];		
	BUG_ON(host == NULL);

	struct mmc_host *mmc = mtk_msdc_host[0]->mmc;
	BUG_ON(mmc == NULL);

	struct mmc_card *card = mmc->card;
	struct mmc_command cmd = {0};
	struct mmc_command stop = {0};
	struct mmc_data data = {0};
	dma_addr_t phyaddr = 0;
	u32 __iomem *base = host->base;

	memset(&cmd,  0, sizeof(struct mmc_command));
	memset(&stop, 0, sizeof(struct mmc_command));
	memset(&data, 0, sizeof(struct mmc_data));
		
	host->tune = 0;
	host->error = 0;
	host->data = &data;
	cmd.data = &data;
	host->cmd = &cmd;
	/* check msdc is work ok. rule is RX/TX fifocnt must be zero after last request
	 * if find abnormal, try to reset msdc first
	 */
	if (msdc_txfifocnt() || msdc_rxfifocnt()) {
		ERR_MSG("register abnormal,please check!\n");
		//msdc_reset_hw();
	}
	//ERR_MSG("[ZJ!!!] [host->id = %d]   HeHe | >>>>>>>>>>>>>2.2\n",host->id);

	msdc_latest_operation_type[host->id] = read ? OPER_TYPE_READ : OPER_TYPE_WRITE;// I know now	
	data.blksz = 512;
	data.error = 0;
	data.blocks = size>>9;	
	
	//ERR_MSG("[ZJ!!!] blocks = %x blksz = %x HeHe | >>>>>>>>>>>>>2.3\n", data.blocks,data.blksz);
	host->xfer_size = data.blocks * data.blksz;
	host->blksz = 512;
	host->dma_xfer = 1;/* decide the transfer mode */
	
	if (read)
	{
		data.flags = MMC_DATA_READ;
		if (data.blocks > 1)
		{
			//Add this for auto command12
			data.stop = &stop;
			cmd.opcode = MMC_READ_MULTIPLE_BLOCK;
		}
		else
		{
			cmd.opcode = MMC_READ_SINGLE_BLOCK;
		}
	}
	else
	{
		data.flags = MMC_DATA_WRITE;
		if (data.blocks > 1)
		{
			//Add this for auto command12
			data.stop = &stop;
			cmd.opcode = MMC_WRITE_MULTIPLE_BLOCK;
		}
		else
		{
			cmd.opcode = MMC_WRITE_BLOCK;
		}
	}
	
	if (mmc_card_blockaddr(card)) 
	{
		cmd.arg = offset>>9;
	}
	else
	{
		cmd.arg = offset;
	}
	cmd.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_ADTC;
	stop.opcode = MMC_STOP_TRANSMISSION;
	stop.arg = 0;
	stop.flags = MMC_RSP_SPI_R1B | MMC_RSP_R1B | MMC_CMD_AC;

	mmc_set_data_timeout(&data, card);
	if (read) 
	{
		//set timeout
		if ((host->timeout_ns != data.timeout_ns) || (host->timeout_clks != data.timeout_clks)) 
		{
			msdc_set_timeout(host, data.timeout_ns, data.timeout_clks);
		}
	}

	msdc_set_blknum(host, data.blocks);
	
	/* enable DMA mode first!! */
	msdc_dma_on();
	
	init_completion(&host->xfer_done);
	/* start the command first*/
	host->autocmd = MSDC_AUTOCMD12; 
	
	if (msdc_command_start(host, &cmd, 0, CMD_TIMEOUT) != 0)
		return -1;

	/* then wait command done */
	if (msdc_command_resp_polling(host, &cmd, 0, CMD_TIMEOUT) != 0)// not tuning.
		return -1;

	/*
	 * for read, the data coming too fast, then CRC error
	 * start DMA no business with CRC. 
	 */
	//init_completion(&host->xfer_done);
    phyaddr = dma_map_single(mmc->parent, buf, size, read?(DMA_FROM_DEVICE):(DMA_TO_DEVICE));
	if (mmc->parent->dma_mask)
		ERR_MSG("dma_mask[0x%p]=0x%llx\n", mmc->parent->dma_mask,
				*mmc->parent->dma_mask);
	else
		ERR_MSG("mmc->parent->dma_mask is NULL\n");
	if (dma_mapping_error(mmc->parent, phyaddr))
		ERR_MSG("dma_mapping_err \n");
	//sdr_write_dma_sa(virt_to_phys(buf));
	sdr_write_dma_sa(phyaddr);
	sdr_set_field(MSDC_DMA_CTRL, MSDC_DMA_CTRL_LASTBUF, 1);
#if defined(CONFIG_ARCH_MT5399) || defined(CONFIG_ARCH_MT5890) || \
	defined(CONFIG_ARCH_MT5882) || defined(CONFIG_ARCH_MT5883) || \
	defined(CONFIG_ARCH_MT5891)
	/* a change happens for dma xfer size:
	  * a new 32-bit register(0xA8) is used for xfer size configuration instead of 16-bit register(0x98 DMA_CTRL)
	  */
	sdr_write32(MSDC_DMA_LEN, size);
#else
	sdr_set_field(MSDC_DMA_CTRL, MSDC_DMA_CTRL_XFERSZ, size);
#endif
	sdr_set_field(MSDC_DMA_CTRL, MSDC_DMA_CTRL_BRUSTSZ, MSDC_BRUST_64B);
	sdr_set_field(MSDC_DMA_CTRL, MSDC_DMA_CTRL_MODE, 0);
	N_MSG(DMA, "DMA_LEN = 0x%x\n", sdr_read32(MSDC_DMA_LEN));
	N_MSG(DMA, "DMA_CTRL = 0x%x\n", sdr_read32(MSDC_DMA_CTRL));
	N_MSG(DMA, "DMA_CFG = 0x%x\n", sdr_read32(MSDC_DMA_CFG));
	N_MSG(DMA, "DMA_SA = 0x%lx\n", sdr_read_dma_sa());
	msdc_dma_start(host);

	spin_unlock(&host->lock);
	if (!wait_for_completion_timeout(&host->xfer_done, DAT_TIMEOUT)) {
		ERR_MSG("XXX CMD<%d> ARG<0x%x> wait xfer_done<%d> timeout!!\n",
			host->cmd->opcode, host->cmd->arg, size);
		data.error = (unsigned int)-ETIMEDOUT;
	}
	spin_lock(&host->lock);
	msdc_dma_stop(host);
	msdc_dma_off();
    dma_unmap_single(mmc->parent, phyaddr, size, read?(DMA_FROM_DEVICE):(DMA_TO_DEVICE));
	
	return 0;
}

int msdc_partread_rwaccess(int partno, uint64_t offset, uint64_t size, void *pvmem)
{
	uint64_t u8partoffs, u8partsize;
	struct mmc_host *mmc = mtk_msdc_host[0]->mmc;
	struct msdc_host *host = mtk_msdc_host[0];
	BUG_ON(mmc == NULL);

	u_char *p_buffer = (u_char *)pvmem;
	int read_length, left_to_read = size;
	
//memset_alloc_pages();

	BUG_ON(msdc_check_partition());

	u8partoffs = msdc_getpart_offs(partno);
	u8partsize = msdc_getpart_size(partno);

	//pr_err("read partoffset = %llx, partsize = %llx | partno = %d, offset = %llx size = %llx\n", u8partoffs, u8partsize, partno, offset, size);

	BUG_ON(offset+size > u8partsize);

	//might_sleep();
	if (msdc_alloc_tempbuf())
	{
		ERR_MSG("request memory failed!\n");
		goto read_fail;
	}

	offset += u8partoffs;
	mmc_claim_host(mmc);
	spin_lock(&host->lock);
	// read non-block alignment offset.
	if (offset % 512)
	{
		int block_offset;

		block_offset = offset % 512;
		offset -= block_offset;
		//pr_err("block_offset =%x offset = %x\n",block_offset, offset);
		if (left_to_read < (512 - block_offset))
		{
			read_length = left_to_read;
		}
		else
		{
			read_length = 512 - block_offset;
		}

		if (msdc_request(offset, 512, _pu1blkbuf, 1))
		{
			goto read_fail;
		}

		memcpy(p_buffer, _pu1blkbuf + block_offset, read_length);

		offset += 512;
		left_to_read -= read_length;
		p_buffer += read_length;
	}

//memset_alloc_pages();
	// read block alignment offset and size. (MSDC_MAXBUF_CNT)
	while (left_to_read >= MSDC_MAXBUF_CNT)
	{
		read_length = MSDC_MAXBUF_CNT;

		if (msdc_request(offset, read_length, _pu1blkbuf, 1))
		{
			goto read_fail;
		}

		memcpy(p_buffer, _pu1blkbuf, read_length);

		offset += read_length;
		left_to_read -= read_length;
		p_buffer += read_length;
	}

//memset_alloc_pages();
	// read block alignment offset and size. (X512B)
	if (left_to_read >= 512)
	{
		read_length = (left_to_read>>9)<<9;

		//pr_err("read_length = %x \n", read_length);

		if (msdc_request(offset, read_length, _pu1blkbuf, 1))
		{
			goto read_fail;
		}
		memcpy(p_buffer, _pu1blkbuf, read_length);

		offset += read_length;
		left_to_read -= read_length;
		p_buffer += read_length;

		//pr_err("offset=%x left_to_read =%x \n", offset, left_to_read);
	}

//memset_alloc_pages();
	// read non-block alignment size.
	if (left_to_read)
	{
		read_length = left_to_read;

		if (msdc_request(offset, 512, _pu1blkbuf, 1))
		{
			goto read_fail;
		}

		memcpy(p_buffer, _pu1blkbuf, read_length);
	}
	ERR_MSG("msdc_partread_rwaccess OK: partno=%d, offset=0x%llX, size=0x%llX, value=0x%08x\n",partno, offset, size, *((unsigned int *)pvmem));
	
	spin_unlock(&host->lock);
	mmc_release_host(mmc);
	return 0;

read_fail:

	spin_unlock(&host->lock);
	mmc_release_host(mmc);
	ERR_MSG("msdc_partread fail: partno=%d, offset=0x%llX, size=0x%llX, pvmem=0x%08X\n",	partno, offset, size, (int)pvmem);
	return -1;
}

//for async mmc request support
int msdc_partread(int partno, uint64_t offset, uint64_t size, void *pvmem)
{
	struct file *filp;
	char buff[30] = {0};
	loff_t seekpos;
	u32 rd_length,u4Ret=0;
	mm_segment_t fs;

	sprintf(buff, "/dev/block/mmcblk%s%d", partno ? "0p" : "",partno);
	filp = filp_open(buff, O_RDONLY | O_SYNC, 0);
	if (IS_ERR(filp)) {
		//pr_info("[%s]Open %s failed: %p !!\n", __func__, buff, filp);
		sprintf(buff, "/dev/mmcblk%s%d", partno ? "0p" : "",partno);
		filp = filp_open(buff, O_RDONLY | O_SYNC, 0);
	}

	extern MSDC_EMMClog;
	if(1 == MSDC_EMMClog)
		pr_err("msdc_partread(%s) oft 0x%llx size ox%llx",buff,offset,size);
	BUG_ON(msdc_check_partition());

	if (IS_ERR(filp)) {
		u4Ret = msdc_partread_rwaccess(partno, offset, size, pvmem);
		if (-1 == u4Ret)
		{
			pr_err("Can't Open block and read RW Access is also failed!!\n");
			return -1;
		}
		else
		{
			pr_err("Can't Open block but read RW Access is OK!!\n");
			return 0;
		}
		//pr_info("[%s]Open %s failed: %p !!\n", __func__, buff, filp);
		//return -1;
	} else {
		seekpos = filp->f_op->llseek(filp, offset, SEEK_SET);
		if (seekpos != offset) {
			pr_err("[%s]%s seek failed,seekpos:0x%08X \n", __func__, buff, (u32)offset);
			fs = get_fs();
			filp_close(filp, NULL);
			return -1;
		}
		fs = get_fs();
		set_fs(KERNEL_DS);
		rd_length = filp->f_op->read(filp, (void*)pvmem, size, &filp->f_pos);
		if (rd_length != size) {
			pr_err("[%s]%s R/W length not match requested,request length:0x%08X, return length:0x%08X\n", __func__, buff,
					(u32)size, (u32)rd_length);
			set_fs(fs); 
			filp_close(filp, NULL);
			return -1;
		}
		set_fs(fs);
		filp_close(filp, NULL);
	}
	return 0;
}

int msdc_partwrite_rwaccess(int partno, uint64_t offset, uint64_t size, void *pvmem)
{
	uint64_t u8partoffs, u8partsize;
	struct mmc_host *mmc = mtk_msdc_host[0]->mmc;
	struct msdc_host *host = mtk_msdc_host[0];
	BUG_ON(mmc == NULL);

	u_char *p_buffer = (u_char *)pvmem;
	int write_length, left_to_write = size;

	BUG_ON(msdc_check_partition());

	u8partoffs = msdc_getpart_offs(partno);
	u8partsize = msdc_getpart_size(partno);

	//pr_err("write partoffset = %llx, partsize = %llx | partno = %d, offset = %llx size = %llx\n", u8partoffs, u8partsize, partno, offset, size);

	BUG_ON(offset+size > u8partsize);

	if (msdc_alloc_tempbuf())
	{
		ERR_MSG("request memory failed!\n");
		goto write_fail;
	}

	offset += u8partoffs;
	mmc_claim_host(mmc);
	spin_lock(&host->lock);

	// write non-block alignment offset.
	if (offset % 512)
	{
		int block_offset;

		block_offset = offset % 512;
		offset -= block_offset;

		//pr_err("block_offset =%x offset = %x\n",block_offset, offset);
		if (left_to_write < (512 - block_offset))
		{
			write_length = left_to_write;
		}
		else
		{
			write_length = 512 - block_offset;
		}

		if (msdc_request(offset, 512, _pu1blkbuf, 1))
		{
			goto write_fail;
		}

		memcpy(_pu1blkbuf + block_offset, p_buffer, write_length);
		
		if (msdc_request(offset, 512, _pu1blkbuf, 0))
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

		if (msdc_request(offset, write_length, _pu1blkbuf, 0))
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

		//pr_err("write_length = %x \n", write_length);
		memcpy(_pu1blkbuf, p_buffer, write_length);

		if (msdc_request(offset, write_length, _pu1blkbuf, 0))
		{
			goto write_fail;
		}

		offset += write_length;
		left_to_write -= write_length;
		p_buffer += write_length;
		//pr_err("offset=%x left_to_write =%x \n", offset, left_to_write);
	}

	// write non-block alignment size.
	if (left_to_write)
	{
		write_length = left_to_write;

		if (msdc_request(offset, 512, _pu1blkbuf, 1))
		{
			goto write_fail;
		}

		memcpy(_pu1blkbuf, p_buffer, write_length);
		
		if (msdc_request(offset, 512, _pu1blkbuf, 0))
		{
			goto write_fail;
		}
	}

	spin_unlock(&host->lock);
	mmc_release_host(mmc);
	ERR_MSG("msdc_partwrite_rwaccess OK: partno=%d, offset=0x%llX, size=0x%llX, value=0x%08x\n",partno, offset, size, *((unsigned int *)pvmem));
	return 0;

write_fail:
	spin_unlock(&host->lock);
	mmc_release_host(mmc);
	ERR_MSG("msdc_partwrite fail: partno=%d, offset=0x%llX, size=0x%llX, pvmem=0x%08X\n", partno, offset, size, (int)pvmem);
	return -1;
}

int msdc_partwrite(int partno, uint64_t offset, uint64_t size, void *pvmem)
{
	struct file *filp;
	char buff[30] = {0};
	loff_t seekpos;
	u32 rd_length,u4Ret = 0;
	mm_segment_t fs;

	sprintf(buff, "/dev/block/mmcblk%s%d", partno ? "0p" : "",partno);
	filp = filp_open(buff, O_RDWR | O_NONBLOCK | O_SYNC, 0);
	if (IS_ERR(filp))
	{
		//pr_err("[%s]Open %s failed: %p !!\n", __func__, buff, filp);
		sprintf(buff, "/dev/mmcblk%s%d", partno ? "0p" : "",partno);
		filp = filp_open(buff, O_RDWR | O_NONBLOCK | O_SYNC, 0);
	}

   // pr_err("msdc_partwrite(%s) oft 0x%llx size ox%llx",buff,offset,size);

	BUG_ON(msdc_check_partition());

	if (IS_ERR(filp))
	{
		u4Ret = msdc_partwrite_rwaccess(partno, offset, size, pvmem);
		if (-1 == u4Ret)
		{
			pr_err("Can't Open the block and Write RW Access also is failed!!\n");
			return -1;
		}
		else
		{
			pr_err("Can't Open the block but Write RW Access is OK\n");
			return 0;
		}
		//pr_info("[%s]Open %s failed: %p !!\n", __func__, buff, filp);
		//return -1;
	}
	else
	{
		seekpos = filp->f_op->llseek(filp, offset, SEEK_SET);
		if (seekpos != offset)
		{
			pr_err("[%s]%s seek failed,seekpos:0x%08X \n", __func__, buff, (u32)offset);
			filp_close(filp, NULL);
			return -1;
		}
		fs = get_fs();
		set_fs(KERNEL_DS);
		rd_length = filp->f_op->write(filp, (void*)pvmem, size, &filp->f_pos);
		if (rd_length != size)
		{
			pr_err("[%s]%s R/W length not match requested,request length:0x%08X, return length:0x%08X\n", __func__, buff,
				(u32)size, (u32)rd_length);
			set_fs(fs);
			filp_close(filp, NULL);
			return -1;
		}
		set_fs(fs);
		filp_close(filp, NULL);
	}
	return 0;
}

int msdc_partread_byname(const char *name, uint64_t offset, uint64_t size, void *pvmem)
{
	int partno;

	BUG_ON(!name);

	for (partno = 1; partno <= mtk_msdc_partition.nparts; partno++) {
		if (strstr(msdc_getpart_name(partno), name))
			return msdc_partread(partno, offset, size, pvmem);
	}

	return -1;
}

int msdc_partwrite_byname(const char *name, uint64_t offset, uint64_t size, void *pvmem)
{
	int partno;

	BUG_ON(!name);

	for (partno = 1; partno <= mtk_msdc_partition.nparts; partno++) {
		if (strstr(msdc_getpart_name(partno), name))
			return msdc_partwrite(partno, offset, size, pvmem);
	}

	return -1;
}

#if 0
EXPORT_SYMBOL(msdc_host_boot);
EXPORT_SYMBOL(msdc_send_command);
#endif
EXPORT_SYMBOL(msdc_partread);
EXPORT_SYMBOL(msdc_partwrite);
EXPORT_SYMBOL(msdc_partread_rwaccess);
EXPORT_SYMBOL(msdc_partwrite_rwaccess);
EXPORT_SYMBOL(msdc_partread_byname);
EXPORT_SYMBOL(msdc_partwrite_byname);

