/*
 * Copyright 2008, Freescale Semiconductor, Inc
 * Andy Fleming
 *
 * Based vaguely on the Linux code
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <config.h>
#include <common.h>
#include <command.h>
#include <mmc.h>
#include <part.h>
#include <malloc.h>
#include <linux/list.h>
#include <div64.h>
#include <jffs2/load_kernel.h>

#if defined(CC_EMMC_BOOT)

typedef struct mmc_async_operation_s
{
	uint total_sz;
  uint cur_off;
  uint cur_pos;
  uint cur_bytes;
#ifdef CC_MTD_ENCRYPT_SUPPORT
#define ATTR_ENCRYPTION    (0x1<<0)
  uint part_attr;
#endif
  uint fgdone;

  struct mmc_cmd cur_cmd;
  struct mmc_data cur_data;
}mmc_async_operation_t;

mmc_async_operation_t mmc_async;
struct mmc emmc_info[CFG_MAX_EMMC_DEVICE];
static char *_pu1BlkBuf = NULL;
static char *_pAddrAlignBuf = NULL;
static int  u4AlignBufLen = 0;

#ifdef CC_PARTITION_WP_SUPPORT
extern uint wp_config;
#endif
static struct list_head mmc_devices;
static int cur_dev_num = -1;
static char EXT_CSD[512];
static int i4mmcInit = 0;

extern int dataMode;
#if defined(CC_MT5399) || defined(CONFIG_ARCH_MT5399) || \
    defined(CC_MT5890) || defined(CONFIG_ARCH_MT5890) || \
    defined(CC_MT5891) || defined(CONFIG_ARCH_MT5891) || \
    defined(CC_MT5882) || defined(CONFIG_ARCH_MT5882) || \
    defined(CC_MT5883) || defined(CONFIG_ARCH_MT5883) || \
    defined(CC_MT5891) || defined(CONFIG_ARCH_MT5891)
extern void MsdcSetSpeedMode(uint speedMode);
#endif
extern void MsdcDescriptorConfig(void *pBuff, uint u4BufLen);
extern int MsdcWaitHostIdle(uint timeout);
extern void MsdcFindDev(uint *pCID);
extern void MsdcSetSampleEdge(uint fgWhere);
extern int MsdcDMAStart(uint cur_pos, uint cur_off, uint bytes, uint total_sz);
extern int MsdcDMAWaitIntr(struct mmc_cmd *cmd, uint cur_off, uint bytes, uint total_sz);
extern int MsdcDMATernmial(struct mmc *mmc, struct mmc_cmd *cmd, struct mmc_data *data);
extern int MsdcReqCmdStart(struct mmc *mmc, struct mmc_cmd *cmd, struct mmc_data *data);
#ifdef CC_PARTITION_WP_SUPPORT
extern uint MsdcPartitionWp(uint blkindx);
extern uint MsdcPartitionWpConfig(uint wp);
#endif
#ifdef CC_MTD_ENCRYPT_SUPPORT
extern uint MsdcPartitionEncrypted(uint blkindx);
extern int MsdcAesEncryption(uint buff, uint len);
extern int MsdcAesDecryption(uint buff, uint len);
#endif

int __board_mmc_getcd(u8 *cd, struct mmc *mmc) {
	return -1;
}

int board_mmc_getcd(u8 *cd, struct mmc *mmc)__attribute__((weak,
	alias("__board_mmc_getcd")));

int mmc_send_cmd(struct mmc *mmc, struct mmc_cmd *cmd, struct mmc_data *data)
{
	return mmc->send_cmd(mmc, cmd, data);
}

int mmc_set_blocklen(struct mmc *mmc, int len)
{
	struct mmc_cmd cmd;

	cmd.cmdidx = MMC_CMD_SET_BLOCKLEN;
	cmd.resp_type = MMC_RSP_R1;
	cmd.cmdarg = len;
	cmd.flags = 0;

	return mmc_send_cmd(mmc, &cmd, NULL);
}

struct mmc *find_mmc_device(int dev_num)
{
	struct mmc *m;
	struct list_head *entry;

	list_for_each(entry, &mmc_devices) {
		m = list_entry(entry, struct mmc, link);

		if (m->block_dev.dev == dev_num)
			return m;
	}

	printf("MMC Device %d not found\n", dev_num);

	return NULL;
}

static ulong
mmc_write_blocks(struct mmc *mmc, ulong start, lbaint_t blkcnt, const void*src)
{
	struct mmc_cmd cmd;
	struct mmc_data data;
	uint ret;
	char *pAlignBuf = NULL;

	if ((start + blkcnt) > mmc->block_dev.lba) {
		printf("MMC: block number 0x%lx exceeds max(0x%lx)\n",
			(ulong)(start + blkcnt), (ulong)(mmc->block_dev.lba));
		return 0;
	}
/*align buffer address with 32 bytes, normally it only request buffer align with 16 bytes */

	if ((int)src & 0x1F) {
		if ((!_pAddrAlignBuf) || (u4AlignBufLen < (blkcnt*mmc->read_bl_len))) {
			if (_pAddrAlignBuf) {
				free(_pAddrAlignBuf);
				_pAddrAlignBuf = NULL;
			}
			
			_pAddrAlignBuf = (char *)malloc(blkcnt*mmc->read_bl_len + 0x20);
			if (!_pAddrAlignBuf) {
				printf("\n ========== Error! alloc buffer failed @ line %d===========\n",__LINE__);
				return 0;
			}
			u4AlignBufLen = (blkcnt*mmc->read_bl_len);
		}
		pAlignBuf = (char *)((((uint)(_pAddrAlignBuf))&(~0x1F)) + 0x20);
		memcpy(pAlignBuf, src, (blkcnt*mmc->read_bl_len));	
	}
	else {
		pAlignBuf = (char *)src;
	}

	if (blkcnt > 1)
		cmd.cmdidx = MMC_CMD_WRITE_MULTIPLE_BLOCK;
	else
		cmd.cmdidx = MMC_CMD_WRITE_SINGLE_BLOCK;

	if (mmc->high_capacity)
		cmd.cmdarg = start;
	else
		cmd.cmdarg = start * mmc->write_bl_len;

	cmd.resp_type = MMC_RSP_R1;
	cmd.flags = 0;
	cmd.flags |= (dataMode & 0x0F);

	data.src = pAlignBuf;
	data.blocks = blkcnt;
	data.blocksize = mmc->write_bl_len;
	data.flags = MMC_DATA_WRITE;
#ifdef CC_PARTITION_WP_SUPPORT
if(MsdcPartitionWp(start)&&(wp_config))
{
   printf(" partition was  write protected!\n");
   return 0;
}
#endif
#ifdef CC_MTD_ENCRYPT_SUPPORT
    if(MsdcPartitionEncrypted(start))
    {
        if(MsdcAesEncryption((uint)pAlignBuf, (uint)(blkcnt*mmc->write_bl_len)))
        {
            printf("aes encryption to buffer(addr:0x%08X len:0x%08X) before write process failed!\n", (uint)pAlignBuf, (uint)(blkcnt*mmc->write_bl_len));
            //return 0;
        }
    }
#endif

    //if(data.blocks > 1)
    //cmd.flags |= (0x01<<4);

    /* dma mode cache flush */
    if((cmd.flags & 0x0F) > 1)
    {
        HalFlushDCacheMultipleLine((uint)(pAlignBuf), (uint)(blkcnt*mmc->write_bl_len));
    }

    /* descriptor dma mode configuration */
    if((cmd.flags & 0x0F) == 3)
    {
        MsdcDescriptorConfig((void *)(pAlignBuf), (uint)(blkcnt*mmc->write_bl_len));
    }
    
    ret = mmc_send_cmd(mmc, &cmd, &data);
    if(ret)
    {
        printf("mmc write failed\n");
		return 0;
    }

	if ((blkcnt > 1)&&(!(cmd.flags>>4))) {
		cmd.cmdidx = MMC_CMD_STOP_TRANSMISSION;
		cmd.cmdarg = 0;
		cmd.resp_type = MMC_RSP_R1b;
		cmd.flags = 0;
		if (mmc_send_cmd(mmc, &cmd, NULL)) {
			printf("mmc fail to send stop cmd\n");
			return 0;
		}
	}

	//HalFlushInvalidateDCacheMultipleLine((uint)(start*mmc->write_bl_len), (uint)(blkcnt*mmc->write_bl_len));

	return blkcnt;
}

static ulong
mmc_bwrite(int dev_num, ulong start, lbaint_t blkcnt, const void *src)
{
	lbaint_t cur, blocks_todo = blkcnt;

	struct mmc *mmc = find_mmc_device(dev_num);
	if (!mmc)
		return 0;

	//if (mmc_set_blocklen(mmc, mmc->write_bl_len))
		//return 0;

	do {
		/*
		 * The 65535 constraint comes from some hardware has
		 * only 16 bit width block number counter
		 */
		cur = (blocks_todo > 65535) ? 65535 : blocks_todo;
		if(mmc_write_blocks(mmc, start, cur, src) != cur)
			return 0;
		blocks_todo -= cur;
		start += cur;
		src += cur * mmc->write_bl_len;
	} while (blocks_todo > 0);

	return blkcnt;
}

int mmc_read_blocks(struct mmc *mmc, void *dst, ulong start, lbaint_t blkcnt)
{
	struct mmc_cmd cmd;
	struct mmc_data data;
	uint ret;
	char *pAlignBuf = NULL;
	/*align buffer address with 32 bytes, normally it only request buffer align with 16 bytes */
	if ((int)dst & 0x1F) {
		if ((!_pAddrAlignBuf) || (u4AlignBufLen < (blkcnt*mmc->read_bl_len))) {
			if (_pAddrAlignBuf) {
				free(_pAddrAlignBuf);
				_pAddrAlignBuf = NULL;
			}
			
			_pAddrAlignBuf = (char *)malloc(blkcnt*mmc->read_bl_len + 0x20);
			if (!_pAddrAlignBuf) {
				printf("\n ========== Error! alloc buffer failed @ line %d===========\n",__LINE__);
				return 0;
			}
			u4AlignBufLen = (blkcnt*mmc->read_bl_len);
		}
			pAlignBuf = (char *)((((uint)(_pAddrAlignBuf))&(~0x1F)) + 0x20);
	}
	else {
		pAlignBuf = (char *)dst;
	}

	if (blkcnt > 1)
		cmd.cmdidx = MMC_CMD_READ_MULTIPLE_BLOCK;
	else
		cmd.cmdidx = MMC_CMD_READ_SINGLE_BLOCK;

	if (mmc->high_capacity)
		cmd.cmdarg = start;
	else
		cmd.cmdarg = (start * mmc->read_bl_len);

	cmd.resp_type = MMC_RSP_R1;
	cmd.flags = 0;
	cmd.flags |= (dataMode & 0x0F);

	data.dest = pAlignBuf;
	data.blocks = blkcnt;
	data.blocksize = mmc->read_bl_len;
	data.flags = MMC_DATA_READ;

//  if(data.blocks > 1)
//    cmd.flags |= (0x01<<4);

  /* dma mode cache flush */
  if((cmd.flags & 0x0F) > 1)
    HalFlushInvalidateDCacheMultipleLine((uint)(pAlignBuf), (uint)(blkcnt*mmc->read_bl_len));

  /* descriptor dma mode configuration */
  if((cmd.flags & 0x0F) == 3)
    MsdcDescriptorConfig((void *)(pAlignBuf), (uint)(blkcnt*mmc->read_bl_len));

    /* dma mode cache flush */
    if((cmd.flags & 0x0F) > 1)
    {
        HalFlushInvalidateDCacheMultipleLine((uint)(pAlignBuf), (uint)(blkcnt*mmc->read_bl_len));
    }
    /* descriptor dma mode configuration */
    if((cmd.flags & 0x0F) == 3)
    {
        MsdcDescriptorConfig((void *)(pAlignBuf), (uint)(blkcnt*mmc->read_bl_len));
    }
    
    ret = mmc_send_cmd(mmc, &cmd, &data);
    if(ret)
    {
        printf("mmc read failed\n");
        return 0;
    }

    if ((blkcnt > 1)&&(!(cmd.flags>>4)))
    {
        cmd.cmdidx = MMC_CMD_STOP_TRANSMISSION;
        cmd.cmdarg = 0;
        cmd.resp_type = MMC_RSP_R1b;
        cmd.flags = 0;
        if (mmc_send_cmd(mmc, &cmd, NULL))
        {
            printf("mmc fail to send stop cmd\n");
            return 0;
        }
    }

    if((cmd.flags & 0x0F) > 1)
    {
        HalInvalidateDCacheMultipleLine((uint)(pAlignBuf), (uint)(blkcnt*mmc->read_bl_len));
    }

#ifdef CC_MTD_ENCRYPT_SUPPORT
    if(MsdcPartitionEncrypted(start))
    {
        if(MsdcAesDecryption((uint)pAlignBuf, (uint)(blkcnt*mmc->write_bl_len)))
        {
            printf("aes encryption to buffer(addr:0x%08X len:0x%08X) before write process failed!\n", (uint)pAlignBuf, (uint)(blkcnt*mmc->write_bl_len));
            //return 0;
        }
    }
#endif
    if ((int)dst & 0x1F)
    {
        memcpy(dst, pAlignBuf, (blkcnt*mmc->write_bl_len));
    }

    return blkcnt;
}

static ulong mmc_bread(int dev_num, ulong start, lbaint_t blkcnt, void *dst)
{
	lbaint_t cur, blocks_todo = blkcnt;

	if (blkcnt == 0)
		return 0;

	struct mmc *mmc = find_mmc_device(dev_num);
	if (!mmc)
		return 0;

	if ((start + blkcnt) > mmc->block_dev.lba) {
		printf("MMC: block number 0x%lx exceeds max(0x%lx)\n",
			(ulong)(start + blkcnt), (ulong)(mmc->block_dev.lba));
		return 0;
	}

	//if (mmc_set_blocklen(mmc, mmc->read_bl_len))
		//return 0;

	do {
		/*
		 * The 65535 constraint comes from some hardware has
		 * only 16 bit width block number counter
		 */
		cur = (blocks_todo > 65535) ? 65535 : blocks_todo;
		if(mmc_read_blocks(mmc, dst, start, cur) != cur)
			return 0;
		blocks_todo -= cur;
		start += cur;
		dst += cur * mmc->read_bl_len;
	} while (blocks_todo > 0);

	return blkcnt;
}

static ulong mmc_bytewrite(int dev_num, u64 bytestart, ulong bytecnt, const void *dst)
{
	u64 blkoffset;
	ulong restsize = bytecnt, offsetinblk;
	ulong memptr = (ulong)dst;

	struct mmc *mmc = find_mmc_device(dev_num);
	if (!mmc)
	{
		return 0;
	}

	if (!_pu1BlkBuf)
	{
		_pu1BlkBuf = (char *)malloc(mmc->write_bl_len + 0x1F);
		_pu1BlkBuf = (char *)((((uint)(_pu1BlkBuf))&(~0x1F)) + 0x20);
	}


	offsetinblk = bytestart & (mmc->write_bl_len - 1);
	blkoffset = bytestart - offsetinblk;

	// Handle offset non-block-size-aligned case
	if (offsetinblk)
	{
		ulong opsize;

		if (mmc_bread(dev_num, blkoffset/mmc->write_bl_len, 1, _pu1BlkBuf) != 1)
		{
			return 0;
		}

		if (restsize >= (mmc->write_bl_len - offsetinblk))
		{
			opsize = mmc->write_bl_len - offsetinblk;
		}
		else
		{
			opsize = restsize;
		}

		memcpy((void *)((char *)_pu1BlkBuf + offsetinblk), (void *)memptr, opsize);

		if (mmc_bwrite(dev_num, blkoffset/mmc->write_bl_len, 1, _pu1BlkBuf) != 1)
		{
			return 0;
		}

		restsize  -= opsize;
		memptr    += opsize;
		blkoffset += mmc->write_bl_len;
	}

	// Handle offset & size block-size-aligned case
	if (restsize >= mmc->read_bl_len)
	{
		ulong blkcnt = restsize/mmc->write_bl_len;

		if (mmc_bwrite(dev_num, blkoffset/mmc->write_bl_len, blkcnt, (void *)memptr) != blkcnt)
		{
			return 0;
		}

		restsize  -= (blkcnt * mmc->write_bl_len);
		memptr    += (blkcnt * mmc->write_bl_len);
		blkoffset += (blkcnt * mmc->write_bl_len);
	}

	// Handle size none-block-size-aligned case
	if (restsize)
	{
		if (mmc_bread(dev_num, blkoffset/mmc->write_bl_len, 1, _pu1BlkBuf) != 1)
		{
			return 0;
		}

		memcpy((void *)_pu1BlkBuf, (void *)memptr, restsize);

		if (mmc_bwrite(dev_num, blkoffset/mmc->write_bl_len, 1, _pu1BlkBuf) != 1)
		{
			return 0;
		}
	}

	return bytecnt;
}

static ulong mmc_byteread(int dev_num, u64 bytestart, ulong bytecnt, void *dst)
{
	u64 blkoffset;
	ulong restsize = bytecnt, offsetinblk;
	ulong memptr = (ulong)dst;

	struct mmc *mmc = find_mmc_device(dev_num);
	if (!mmc)
	{
		return 0;
	}

	if (!_pu1BlkBuf)
	{
		_pu1BlkBuf = (char *)malloc(mmc->read_bl_len + 0x1F);
		_pu1BlkBuf = (char *)((((uint)(_pu1BlkBuf))&(~0x1F)) + 0x20);
	}

	offsetinblk = bytestart & (mmc->read_bl_len - 1);
	blkoffset = bytestart - offsetinblk;

	// Handle offset non-block-size-aligned case
	if (offsetinblk)
	{
		ulong opsize;

		if (mmc_bread(dev_num, blkoffset/mmc->read_bl_len, 1, _pu1BlkBuf) != 1)
		{
			return 0;
		}

		if (restsize >= (mmc->read_bl_len - offsetinblk))
		{
			opsize = mmc->read_bl_len - offsetinblk;
		}
		else
		{
			opsize = restsize;
		}

		memcpy((void *)memptr, (void *)((char *)_pu1BlkBuf + offsetinblk), opsize);

		restsize  -= opsize;
		memptr    += opsize;
		blkoffset += mmc->read_bl_len;
	}

	// Handle offset & size block-size-aligned case
	if (restsize >= mmc->read_bl_len)
	{
		ulong blkcnt = restsize/mmc->read_bl_len;

		if (mmc_bread(dev_num, blkoffset/mmc->read_bl_len, blkcnt, (void *)memptr) != blkcnt)
		{
			return 0;
		}

		restsize  -= (blkcnt * mmc->read_bl_len);
		memptr    += (blkcnt * mmc->read_bl_len);
		blkoffset += (blkcnt * mmc->read_bl_len);
	}

	// Handle size none-block-size-aligned case
	if (restsize)
	{
		if (mmc_bread(dev_num, blkoffset/mmc->read_bl_len, 1, _pu1BlkBuf) != 1)
		{
			return 0;
		}

		memcpy((void *)memptr, (void *)_pu1BlkBuf, restsize);
	}

	return bytecnt;
}

/* emmc_async_read_setup
   - only support basic dma mode
   - It is strongly recommanded to adopt basic dma async for little data(<64KB every time)
 */
int emmc_async_read_setup(int dev_num, u64 bytestart, ulong bytecnt, void *dst)
{
	ulong blkstart, blkcnt;
	struct mmc_cmd cmd;
	struct mmc_data data;

	if (bytecnt == 0)
		return 0;

	struct mmc *mmc = find_mmc_device(dev_num);
	if (!mmc)
		return 0;

	blkstart = bytestart / mmc->read_bl_len;
	blkcnt = bytecnt / mmc->read_bl_len;

	if (blkcnt > 1)
		cmd.cmdidx = MMC_CMD_READ_MULTIPLE_BLOCK;
	else
		cmd.cmdidx = MMC_CMD_READ_SINGLE_BLOCK;

	if (mmc->high_capacity)
		cmd.cmdarg = blkstart;
	else
		cmd.cmdarg = blkstart * mmc->read_bl_len;

	cmd.resp_type = MMC_RSP_R1;
	cmd.flags = 0;
	cmd.flags |= (dataMode & 0x0F);

	data.dest = dst;
	data.blocks = blkcnt;
	data.blocksize = mmc->read_bl_len;
	data.flags = MMC_DATA_READ;

	//if(data.blocks > 1)
	//	cmd.flags |= (0x01<<4);

	/* dma mode cache flush */
	if((cmd.flags & 0x0F) > 1)
		HalFlushInvalidateDCacheMultipleLine((uint)(dst), (uint)(blkcnt*mmc->read_bl_len));

    /* descriptor dma mode configuration */
    if((cmd.flags & 0x0F) == 3)
        MsdcDescriptorConfig((void *)(dst), (uint)(blkcnt*mmc->read_bl_len));

	mmc_async.total_sz = bytecnt;
	mmc_async.cur_off = 0;
	mmc_async.cur_pos = (uint)(dst);
	mmc_async.cur_bytes = 0;
#ifdef CC_MTD_ENCRYPT_SUPPORT
    if(MsdcPartitionEncrypted(blkstart))
    {
	mmc_async.part_attr |= ATTR_ENCRYPTION;
	}
#endif
	mmc_async.fgdone = 0;
	memcpy((void *)(&mmc_async.cur_cmd), (void *)(&cmd), sizeof(struct mmc_cmd));
	memcpy((void *)(&mmc_async.cur_data), (void *)(&data), sizeof(struct mmc_data));

	if((cmd.flags & 0x0F) > 1)
		HalFlushInvalidateDCacheMultipleLine((uint)(dst), (uint)(bytecnt));

	if(MsdcReqCmdStart(mmc, &cmd, &data))
	{
		return -1;
	}

	return 0;
}

int emmc_async_dma_start_trigger(uint length)
{
	mmc_async.cur_bytes = length;
	mmc_async.fgdone = 0;

	if(0 != MsdcDMAStart(mmc_async.cur_pos, mmc_async.cur_off,
		                   mmc_async.cur_bytes, mmc_async.total_sz))
	{
		printf("DMA Start failed!\n");
		return -1;
	}

	return 0;
}

int emmc_async_dma_wait_finish(void)
{
	struct mmc_cmd cmd;
#ifdef CC_MTD_ENCRYPT_SUPPORT
  uint tmp = 0;
#endif

	cmd = mmc_async.cur_cmd;

	if(0 != MsdcDMAWaitIntr(&cmd, mmc_async.cur_off, mmc_async.cur_bytes, mmc_async.total_sz))
	{
		printf("DMA Transfer failed!\n");
		return -1;
	}

	if((cmd.flags & 0x0F) > 1)
        HalInvalidateDCacheMultipleLine(mmc_async.cur_pos, mmc_async.cur_bytes);

#ifdef CC_MTD_ENCRYPT_SUPPORT
  if(mmc_async.part_attr & ATTR_ENCRYPTION)
  {
    if(MsdcAesDecryption((uint)(mmc_async.cur_pos), (uint)(mmc_async.cur_bytes)))
    {
      //printf("aes encryption to buffer(addr:0x%08X len:0x%08X) before write process failed!\n", (uint)(mmc_async.cur_pos),
                                                                                                //(uint)(mmc_async.cur_bytes));
    }
  }
#endif

	mmc_async.cur_pos += mmc_async.cur_bytes;
	mmc_async.cur_off += mmc_async.cur_bytes;
	mmc_async.fgdone = 1;

	return 0;
}

int emmc_async_read_finish(int dev_num)
{
	struct mmc_cmd cmd, cmd_stop;
	struct mmc_data data;

	struct mmc *mmc = find_mmc_device(dev_num);
	if (!mmc)
	return 0;

	cmd = mmc_async.cur_cmd;
	data = mmc_async.cur_data;

	if(mmc_async.cur_off < mmc_async.total_sz)
	{
    if(MsdcDMATernmial(mmc, &cmd, &data))
    {
      printf("force stop dma egineer/emmc failed!\n");
      return -1;
    }
	}
	else
	{
	  if ((data.blocks > 1)&&(!(cmd.flags>>4))) {
		  cmd_stop.cmdidx = MMC_CMD_STOP_TRANSMISSION;
		  cmd_stop.cmdarg = 0;
		  cmd_stop.resp_type = MMC_RSP_R1b;
		  cmd_stop.flags = 0;
		  if (mmc_send_cmd(mmc, &cmd_stop, NULL)) {
			  printf("mmc fail to send stop cmd\n");
			  return -1;
		  }
	  }
  }

	return 0;
}

int emmc_async_read_stop(int dev_num, int i4fg)
{
	struct mmc_cmd cmd;
	struct mmc_data data;

	struct mmc *mmc = find_mmc_device(dev_num);
	if (!mmc)
		return 0;

	cmd = mmc_async.cur_cmd;
	data = mmc_async.cur_data;

	if(MsdcDMATernmial(mmc, &cmd, &data))
	{
		return -1;
	}

	return 0;
}

int mmc_go_idle(struct mmc* mmc)
{
	struct mmc_cmd cmd;
	int err;

	udelay(1000);

	cmd.cmdidx = MMC_CMD_GO_IDLE_STATE;
	cmd.cmdarg = 0;
	cmd.resp_type = MMC_RSP_NONE;
	cmd.flags = 0;

	err = mmc_send_cmd(mmc, &cmd, NULL);

	if (err)
		return err;

	udelay(2000);

	return 0;
}

int sd_send_op_cond(struct mmc *mmc)
{
	int timeout = 1000;
	int err;
	struct mmc_cmd cmd;

	do {
		cmd.cmdidx = MMC_CMD_APP_CMD;
		cmd.resp_type = MMC_RSP_R1;
		cmd.cmdarg = 0;
		cmd.flags = 0;

		err = mmc_send_cmd(mmc, &cmd, NULL);

		if (err)
			return err;

		cmd.cmdidx = SD_CMD_APP_SEND_OP_COND;
		cmd.resp_type = MMC_RSP_R3;

		/*
		 * Most cards do not answer if some reserved bits
		 * in the ocr are set. However, Some controller
		 * can set bit 7 (reserved for low voltages), but
		 * how to manage low voltages SD card is not yet
		 * specified.
		 */
		cmd.cmdarg = mmc->voltages & 0xff8000;

		if (mmc->version == SD_VERSION_2)
			cmd.cmdarg |= OCR_HCS;

		err = mmc_send_cmd(mmc, &cmd, NULL);

		if (err)
			return err;

		udelay(1000);
	} while ((!(cmd.response[0] & OCR_BUSY)) && timeout--);

	if (timeout <= 0)
		return UNUSABLE_ERR;

	if (mmc->version != SD_VERSION_2)
		mmc->version = SD_VERSION_1_0;

	mmc->ocr = cmd.response[0];

	mmc->high_capacity = ((mmc->ocr & OCR_HCS) == OCR_HCS);
	mmc->rca = 0;

	return 0;
}

int mmc_send_op_cond(struct mmc *mmc)
{
	int timeout = 1000;
	struct mmc_cmd cmd;
	int err;

	/* Some cards seem to need this */
	mmc_go_idle(mmc);

	do {
		cmd.cmdidx = MMC_CMD_SEND_OP_COND;
		cmd.resp_type = MMC_RSP_R3;
		cmd.cmdarg = OCR_HCS | mmc->voltages;
		cmd.flags = 0;

		err = mmc_send_cmd(mmc, &cmd, NULL);

		if (err)
			return err;

		udelay(1000);
	} while (!(cmd.response[0] & OCR_BUSY) && timeout--);

	if (timeout <= 0)
		return UNUSABLE_ERR;

	mmc->version = MMC_VERSION_UNKNOWN;
	mmc->ocr = cmd.response[0];

	mmc->high_capacity = ((mmc->ocr & OCR_HCS) == OCR_HCS);
	mmc->rca = 0;

	return 0;
}

int mmc_send_ext_csd(struct mmc *mmc, char *ext_csd)
{
	struct mmc_cmd cmd;
	struct mmc_data data;
	int err;

	/* Get the Card Status Register */
	cmd.cmdidx = MMC_CMD_SEND_EXT_CSD;
	cmd.resp_type = MMC_RSP_R1;
	cmd.cmdarg = 0;
	cmd.flags = 0;
	cmd.flags |= (dataMode & 0x0F);

	/* dma mode cache flush */
	if((cmd.flags & 0x0F) > 1)
		HalFlushInvalidateDCacheMultipleLine((uint)(ext_csd), 512);

	/* descriptor dma mode configuration */
	if((cmd.flags & 0x0F) == 3)
		MsdcDescriptorConfig((void *)(ext_csd), 512);

	data.dest = ext_csd;
	data.blocks = 1;
	data.blocksize = 512;
	data.flags = MMC_DATA_READ;

	err = mmc_send_cmd(mmc, &cmd, &data);

	return err;
}

int mmc_switch(struct mmc *mmc, u8 set, u8 index, u8 value)
{
	struct mmc_cmd cmd;

	cmd.cmdidx = MMC_CMD_SWITCH;
	cmd.resp_type = MMC_RSP_R1b;
	cmd.cmdarg = (MMC_SWITCH_MODE_WRITE_BYTE << 24) | (index << 16) | (value << 8);
	cmd.flags = 0;

	return mmc_send_cmd(mmc, &cmd, NULL);
}

int mmc_change_freq(struct mmc *mmc)
{
	char ext_csd[512];
	char cardtype;
	int err;

	mmc->card_caps = 0;

	/* Only version 4 supports high-speed */
	if (mmc->version < MMC_VERSION_4)
		return 0;

	mmc->card_caps |= MMC_MODE_4BIT;

	err = mmc_send_ext_csd(mmc, ext_csd);

	if (err)
		return err;

	/* It's not absolutely right for some emmc brand, for example THGBM3G4D1FBAIG;
	 * I think it is because of normal capacity with higher spec.
	 if (ext_csd[212] || ext_csd[213] || ext_csd[214] || ext_csd[215])
	 	mmc->high_capacity = 1;
	*/

	cardtype = ext_csd[196] & 0xf;

#if defined(CC_MT5399) || defined(CONFIG_ARCH_MT5399)|| \
    defined(CC_MT5890) || defined(CONFIG_ARCH_MT5890)|| \
    defined(CC_MT5891) || defined(CONFIG_ARCH_MT5891)|| \
    defined(CC_MT5882) || defined(CONFIG_ARCH_MT5882)|| \
    defined(CC_MT5883) || defined(CONFIG_ARCH_MT5883)|| \
    defined(CC_MT5891) || defined(CONFIG_ARCH_MT5891)
#else
	/* Position 1 Sample Edge Setting
	   I think it is necessary to handle high speed mode at low frequency(400KHz);
	   It is some special case for timing;
	   And switch command - cmd6 should be applied by new timing becuase of its response.
	 */
	MsdcSetSampleEdge(1);
#endif

	err = mmc_switch(mmc, EXT_CSD_CMD_SET_NORMAL, EXT_CSD_HS_TIMING, 1);

	if (err)
		return err;

	/* Now check to see that it worked */
	err = mmc_send_ext_csd(mmc, ext_csd);

	if (err)
		return err;

	/* No high-speed support */
	if (!ext_csd[185])
		return 0;

	/* High Speed is set, there are two types: 52MHz and 26MHz */
	if (cardtype & MMC_HS_52MHZ)
		mmc->card_caps |= MMC_MODE_HS_52MHz | MMC_MODE_HS;
	else
		mmc->card_caps |= MMC_MODE_HS;

	return 0;
}

int sd_switch(struct mmc *mmc, int mode, int group, u8 value, u8 *resp)
{
	struct mmc_cmd cmd;
	struct mmc_data data;

	/* Switch the frequency */
	cmd.cmdidx = SD_CMD_SWITCH_FUNC;
	cmd.resp_type = MMC_RSP_R1;
	cmd.cmdarg = (mode << 31) | 0xffffff;
	cmd.cmdarg &= ~(0xf << (group * 4));
	cmd.cmdarg |= value << (group * 4);
	cmd.flags = 0;

	data.dest = (char *)resp;
	data.blocksize = 64;
	data.blocks = 1;
	data.flags = MMC_DATA_READ;

	return mmc_send_cmd(mmc, &cmd, &data);
}

int sd_change_freq(struct mmc *mmc)
{
	int err;
	struct mmc_cmd cmd;
	uint scr[2];
	uint switch_status[16];
	struct mmc_data data;
	int timeout;

	mmc->card_caps = 0;

	/* Read the SCR to find out if this card supports higher speeds */
	cmd.cmdidx = MMC_CMD_APP_CMD;
	cmd.resp_type = MMC_RSP_R1;
	cmd.cmdarg = mmc->rca << 16;
	cmd.flags = 0;

	err = mmc_send_cmd(mmc, &cmd, NULL);

	if (err)
		return err;

	cmd.cmdidx = SD_CMD_APP_SEND_SCR;
	cmd.resp_type = MMC_RSP_R1;
	cmd.cmdarg = 0;
	cmd.flags = 0;

	timeout = 3;

retry_scr:
	data.dest = (char *)&scr;
	data.blocksize = 8;
	data.blocks = 1;
	data.flags = MMC_DATA_READ;

	err = mmc_send_cmd(mmc, &cmd, &data);

	if (err) {
		if (timeout--)
			goto retry_scr;

		return err;
	}

	mmc->scr[0] = __be32_to_cpu(scr[0]);
	mmc->scr[1] = __be32_to_cpu(scr[1]);

	switch ((mmc->scr[0] >> 24) & 0xf) {
		case 0:
			mmc->version = SD_VERSION_1_0;
			break;
		case 1:
			mmc->version = SD_VERSION_1_10;
			break;
		case 2:
			mmc->version = SD_VERSION_2;
			break;
		default:
			mmc->version = SD_VERSION_1_0;
			break;
	}

	/* Version 1.0 doesn't support switching */
	if (mmc->version == SD_VERSION_1_0)
		return 0;

	timeout = 4;
	while (timeout--) {
		err = sd_switch(mmc, SD_SWITCH_CHECK, 0, 1,
				(u8 *)&switch_status);

		if (err)
			return err;

		/* The high-speed function is busy.  Try again */
		if (!(__be32_to_cpu(switch_status[7]) & SD_HIGHSPEED_BUSY))
			break;
	}

	if (mmc->scr[0] & SD_DATA_4BIT)
		mmc->card_caps |= MMC_MODE_4BIT;

	/* If high-speed isn't supported, we return */
	if (!(__be32_to_cpu(switch_status[3]) & SD_HIGHSPEED_SUPPORTED))
		return 0;

	err = sd_switch(mmc, SD_SWITCH_SWITCH, 0, 1, (u8 *)&switch_status);

	if (err)
		return err;

	if ((__be32_to_cpu(switch_status[4]) & 0x0f000000) == 0x01000000)
		mmc->card_caps |= MMC_MODE_HS;

	return 0;
}

/* frequency bases */
/* divided by 10 to be nice to platforms without floating point */
int fbase[] = {
	10000,
	100000,
	1000000,
	10000000,
};

/* Multiplier values for TRAN_SPEED.  Multiplied by 10 to be nice
 * to platforms without floating point.
 */
int multipliers[] = {
	0,	/* reserved */
	10,
	12,
	13,
	15,
	20,
	25,
	30,
	35,
	40,
	45,
	50,
	55,
	60,
	70,
	80,
};

void mmc_set_ios(struct mmc *mmc)
{
	mmc->set_ios(mmc);
}

void mmc_set_clock(struct mmc *mmc, uint clock)
{
	if (clock > mmc->f_max)
		clock = mmc->f_max;

	if (clock < mmc->f_min)
		clock = mmc->f_min;

	mmc->clock = clock;

	mmc_set_ios(mmc);
}

void mmc_set_bus_width(struct mmc *mmc, uint width)
{
	mmc->bus_width = width;

	mmc_set_ios(mmc);
}

int mmc_startup(struct mmc *mmc)
{
	int err;
	uint mult, freq;
	u64 cmult, csize;
	struct mmc_cmd cmd;
	char ext_csd[512];

	/* Put the Card in Identify Mode */
	cmd.cmdidx = MMC_CMD_ALL_SEND_CID;
	cmd.resp_type = MMC_RSP_R2;
	cmd.cmdarg = 0;
	cmd.flags = 0;

	err = mmc_send_cmd(mmc, &cmd, NULL);

	if (err)
		return err;

	memcpy(mmc->cid, cmd.response, 16);

  /* Find device from supported list*/
	MsdcFindDev(mmc->cid);

	/*
	 * For MMC cards, set the Relative Address.
	 * For SD cards, get the Relatvie Address.
	 * This also puts the cards into Standby State
	 */
	cmd.cmdidx = SD_CMD_SEND_RELATIVE_ADDR;
	cmd.cmdarg = mmc->rca << 16;
	cmd.resp_type = MMC_RSP_R6;
	cmd.flags = 0;

	err = mmc_send_cmd(mmc, &cmd, NULL);

	if (err)
		return err;

	if (IS_SD(mmc))
		mmc->rca = (cmd.response[0] >> 16) & 0xffff;

	/* Get the Card-Specific Data */
	cmd.cmdidx = MMC_CMD_SEND_CSD;
	cmd.resp_type = MMC_RSP_R2;
	cmd.cmdarg = mmc->rca << 16;
	cmd.flags = 0;

	err = mmc_send_cmd(mmc, &cmd, NULL);

	if (err)
		return err;

	mmc->csd[0] = cmd.response[0];
	mmc->csd[1] = cmd.response[1];
	mmc->csd[2] = cmd.response[2];
	mmc->csd[3] = cmd.response[3];

	if (mmc->version == MMC_VERSION_UNKNOWN) {
		int version = (cmd.response[0] >> 26) & 0xf;

		switch (version) {
			case 0:
				mmc->version = MMC_VERSION_1_2;
				break;
			case 1:
				mmc->version = MMC_VERSION_1_4;
				break;
			case 2:
				mmc->version = MMC_VERSION_2_2;
				break;
			case 3:
				mmc->version = MMC_VERSION_3;
				break;
			case 4:
				mmc->version = MMC_VERSION_4;
				break;
			default:
				mmc->version = MMC_VERSION_1_2;
				break;
		}
	}

	/* divide frequency by 10, since the mults are 10x bigger */
	freq = fbase[(cmd.response[0] & 0x7)];
	mult = multipliers[((cmd.response[0] >> 3) & 0xf)];

	mmc->tran_speed = freq * mult;

	mmc->read_bl_len = 1 << ((cmd.response[1] >> 16) & 0xf);

	if (IS_SD(mmc))
		mmc->write_bl_len = mmc->read_bl_len;
	else
		mmc->write_bl_len = 1 << ((cmd.response[3] >> 22) & 0xf);

	if (mmc->high_capacity) {
		csize = (mmc->csd[1] & 0x3f) << 16
			| (mmc->csd[2] & 0xffff0000) >> 16;
		cmult = 8;
		//cmult = 0x7;     //modify by shunli wang at 2011.05.12
	} else {
		csize = (mmc->csd[1] & 0x3ff) << 2
			| (mmc->csd[2] & 0xc0000000) >> 30;
		cmult = (mmc->csd[2] & 0x00038000) >> 15;
	}

	mmc->capacity = (csize + 1) << (cmult + 2);
	mmc->capacity *= mmc->read_bl_len;

	if (mmc->read_bl_len > 512)
		mmc->read_bl_len = 512;

	if (mmc->write_bl_len > 512)
		mmc->write_bl_len = 512;

	/* Select the card, and put it into Transfer Mode */
	cmd.cmdidx = MMC_CMD_SELECT_CARD;
	cmd.resp_type = MMC_RSP_R1b;
	cmd.cmdarg = mmc->rca << 16;
	cmd.flags = 0;
	err = mmc_send_cmd(mmc, &cmd, NULL);

	if (err)
		return err;

	if (!IS_SD(mmc) && (mmc->version >= MMC_VERSION_4)) {
		/* check  ext_csd version and capacity */
		err = mmc_send_ext_csd(mmc, ext_csd);
		if (!err && (ext_csd[192] >= 2) && (mmc->high_capacity)) {
			mmc->capacity = ext_csd[212] << 0 | ext_csd[213] << 8 |
					ext_csd[214] << 16 | ext_csd[215] << 24;
			mmc->capacity *= 512;
		}
		memcpy(EXT_CSD, ext_csd, 512);
	}

	if (IS_SD(mmc))
		err = sd_change_freq(mmc);
	else
		err = mmc_change_freq(mmc);

	if (err)
		return err;

	/* Restrict card's capabilities by what the host can do */
	// It is a bug modified by shunli.wang
	mmc->card_caps |= MMC_MODE_8BIT;
	mmc->card_caps &= mmc->host_caps;

	if (IS_SD(mmc)) {
		if (mmc->card_caps & MMC_MODE_4BIT) {
			cmd.cmdidx = MMC_CMD_APP_CMD;
			cmd.resp_type = MMC_RSP_R1;
			cmd.cmdarg = mmc->rca << 16;
			cmd.flags = 0;

			err = mmc_send_cmd(mmc, &cmd, NULL);
			if (err)
				return err;

			cmd.cmdidx = SD_CMD_APP_SET_BUS_WIDTH;
			cmd.resp_type = MMC_RSP_R1;
			cmd.cmdarg = 2;
			cmd.flags = 0;
			err = mmc_send_cmd(mmc, &cmd, NULL);
			if (err)
				return err;

			mmc_set_bus_width(mmc, 4);
		}

		if (mmc->card_caps & MMC_MODE_HS)
			mmc_set_clock(mmc, 50000000);
		else
			mmc_set_clock(mmc, 25000000);
	} else {
		if (mmc->card_caps & MMC_MODE_4BIT) {
			/* Set the card to use 4 bit*/
			err = mmc_switch(mmc, EXT_CSD_CMD_SET_NORMAL,
					EXT_CSD_BUS_WIDTH,
					EXT_CSD_BUS_WIDTH_4);

			if (err)
				return err;

			mmc_set_bus_width(mmc, 4);
		} else if (mmc->card_caps & MMC_MODE_8BIT) {
			/* Set the card to use 8 bit*/
			err = mmc_switch(mmc, EXT_CSD_CMD_SET_NORMAL,
					EXT_CSD_BUS_WIDTH,
					EXT_CSD_BUS_WIDTH_8);

			if (err)
				return err;

			mmc_set_bus_width(mmc, 8);
		}

		if (mmc->card_caps & MMC_MODE_HS) {
			if (mmc->card_caps & MMC_MODE_HS_52MHz)
				mmc_set_clock(mmc, 52000000);
			else
				mmc_set_clock(mmc, 26000000);
		} else
			mmc_set_clock(mmc, 20000000);
	}

#if defined(CC_MT5399) || defined(CONFIG_ARCH_MT5399)|| \
    defined(CC_MT5890) || defined(CONFIG_ARCH_MT5890)|| \
    defined(CC_MT5891) || defined(CONFIG_ARCH_MT5891)|| \
    defined(CC_MT5882) || defined(CONFIG_ARCH_MT5882)|| \
    defined(CC_MT5883) || defined(CONFIG_ARCH_MT5883)|| \
    defined(CC_MT5891) || defined(CONFIG_ARCH_MT5891)
#else
	/* Position 2 Sample Edge Setting*/
	MsdcSetSampleEdge(2);
#endif

	/* fill in device description */
	mmc->block_dev.lun = 0;
	mmc->block_dev.type = MTD_DEV_TYPE_EMMC;
	mmc->block_dev.blksz = mmc->read_bl_len;
	mmc->block_dev.lba = lldiv(mmc->capacity, mmc->read_bl_len);
	sprintf(mmc->block_dev.vendor, "Man %06x Snr %08x", mmc->cid[0] >> 8,
			(mmc->cid[2] << 8) | (mmc->cid[3] >> 24));
	sprintf(mmc->block_dev.product, "%c%c%c%c%c", mmc->cid[0] & 0xff,
			(mmc->cid[1] >> 24), (mmc->cid[1] >> 16) & 0xff,
			(mmc->cid[1] >> 8) & 0xff, mmc->cid[1] & 0xff);
	sprintf(mmc->block_dev.revision, "%d.%d", mmc->cid[2] >> 28,
			(mmc->cid[2] >> 24) & 0xf);
	//init_part(&mmc->block_dev);

	return 0;
}

int mmc_send_if_cond(struct mmc *mmc)
{
	struct mmc_cmd cmd;
	int err;

	cmd.cmdidx = SD_CMD_SEND_IF_COND;
	/* We set the bit if the host supports voltages between 2.7 and 3.6 V */
	cmd.cmdarg = ((mmc->voltages & 0xff8000) != 0) << 8 | 0xaa;
	cmd.resp_type = MMC_RSP_R7;
	cmd.flags = 0;

	err = mmc_send_cmd(mmc, &cmd, NULL);

	if (err)
		return err;

	if ((cmd.response[0] & 0xff) != 0xaa)
		return UNUSABLE_ERR;
	else
		mmc->version = SD_VERSION_2;

	return 0;
}

unsigned long mmc_berase(int dev_num, unsigned long start, lbaint_t blkcnt, unsigned int mode)
{
	struct mmc *mmc = find_mmc_device(dev_num);
	u64 bytestart = 0, length = 0;

	if (!mmc)
		return 0;

	bytestart = start;
	bytestart <<= 9;
	length = blkcnt;
	length <<= 9;

	return mmc_erase(mmc, bytestart, length, mode);
}

/* Support ordinary erase and trim operation
   mode:
     0 - Erase
     1 - Trim
     2 - Secure Erase
     3 - Secure Trim Step 1 (Spec 4.41 or elder)
     4 - Secure Trim Step 2 (Spec 4.41 or elder)
 */
int mmc_erase(struct mmc *mmc, u64 bytestart, u64 length, uint mode)
{
	struct mmc_cmd cmd;
	uint cmdarg = 0;
	uint tmpcsd[4], ret = 0;
	uint writeblklen, erasegroupsz, wpgroupsz, erasegrpdef;

	switch(mode)
	{
	case 0:
		cmdarg = 0x00000000;
		break;
	case 1:
		cmdarg = 0x00000001;
		break;
	case 2:
		cmdarg = 0x80000000;
		break;
	case 3:
		cmdarg = 0x80000001;
		break;
	case 4:
		cmdarg = 0x80008000;
		break;
	default:
		cmdarg = 0x00000000;
		break;
	}

	if((mode == 1) && ((EXT_CSD[231] & (0x1<<4)) == 0x0))
	{
		printf("Not Support Trim!\n");
		return 0;
	}

	if((bytestart >= mmc->capacity) || (bytestart + length >= mmc->capacity))
	{
		printf("exceed size, please give proper parameter\n");
		return -1;
	}

	/* update the ext_csd datas */
	mmc_send_ext_csd(mmc, EXT_CSD);

    /* reverse csd for the normal order*/
	memcpy((void *)tmpcsd, (void *)(mmc->csd), sizeof(mmc->csd)/sizeof(uchar));
    mmc_reverse_uint(tmpcsd, sizeof(tmpcsd)/sizeof(uint));

    erasegrpdef = EXT_CSD[175];
    writeblklen = (0x1<<((tmpcsd[0]&(0x0F<<22))>>22));

    if(erasegrpdef == 0)
    {
        erasegroupsz = writeblklen;
        erasegroupsz *= (((tmpcsd[1]&(0x1F<<10))>>10) + 1)*(((tmpcsd[1]&(0x1F<<5))>>5) + 1);

        wpgroupsz = erasegroupsz;
        wpgroupsz *= ((tmpcsd[1]&(0x1F<<0))>>0) + 1;
    }
    else
    {
        erasegroupsz = (512*1024);
        erasegroupsz *= (EXT_CSD[224]);

        wpgroupsz = erasegroupsz;
        wpgroupsz *= (EXT_CSD[221]);
    }

	printf("mmc_erase start from 0x%llx, size is 0x%llx, erase group size is 0x%x\n",
			bytestart, length, erasegroupsz);
	// Send CMD35
	cmd.cmdidx = MMC_CMD_ERASE_START;
	cmd.resp_type = MMC_RSP_R1;
	cmd.flags = 0;
	if(mmc->high_capacity)
	    cmd.cmdarg = (u32)(bytestart >> 9);
	else
	    cmd.cmdarg = (u32)bytestart;

	printf("mmc_erase start sector 0x%lx\n",cmd.cmdarg);

	if (mmc_send_cmd(mmc, &cmd, NULL)) {
		printf("mmc fail to send erase start cmd\n");
		return -1;
	}

	// Send CMD36
	cmd.cmdidx = MMC_CMD_ERASE_END;
	cmd.resp_type = MMC_RSP_R1;
	cmd.flags = 0;
	if(mmc->high_capacity)
	    cmd.cmdarg = (u32)((bytestart + length - 1) >> 9);
	else
	    cmd.cmdarg = (u32)(bytestart + length - 1);

	printf("mmc_erase end sector 0x%lx\n",cmd.cmdarg);

	if (mmc_send_cmd(mmc, &cmd, NULL)) {
		printf("mmc fail to send erase end cmd\n");
		return -1;
	}

	// Send CMD38
	cmd.cmdidx = MMC_CMD_ERASE;
	cmd.cmdarg = cmdarg;
	cmd.resp_type = MMC_RSP_R1b;
	cmd.flags = 0;
	if (mmc_send_cmd(mmc, &cmd, NULL)) {
		printf("mmc fail to send erase cmd\n");
		return -1;
	}

	// wait host idle
	MsdcWaitHostIdle(10000);

	// Get Card Status
	cmd.cmdidx = MMC_CMD_SEND_STATUS;
	cmd.cmdarg = mmc->rca << 16;
	cmd.resp_type = MMC_RSP_R1;
	cmd.flags = 0;

	if (mmc_send_cmd(mmc, &cmd, NULL)) {
		printf("mmc fail to send card status cmd\n");
		return -1;
	}
	if(cmd.response[0] & (0x1<<28))
	{
		printf("an error in the sequence of erase commands occurred!\n");
		return -1;
	}
	else if(cmd.response[0] & (0x1<<31))
	{
		printf("out of the allowed range for mmc!\n");
		return -1;
	}
	else if(cmd.response[0] & (0x1<<27))
	{
		printf("an invalid selection of erase groups for erase occurred!\n");
		return -1;
	}

	return 0;
}

int mmc_erase_all(struct mmc *mmc)
{
  uint64_t capacity = mmc->capacity;
  uint err = 0;

  err = mmc_erase(mmc, 0, capacity, 0);

  return err;
}

int mmc_reverse_uchar(uchar *pBuff, uint len)
{
  uint i, j;
	uchar u1Tmp;

	for(i = 0, j = len - 1; i<j; i++, j--)
	{
    u1Tmp = pBuff[i];
		pBuff[i] = pBuff[j];
		pBuff[j] = u1Tmp;
	}

	return 0;
}

int mmc_reverse_uint(uint *pBuff, uint len)
{
  uint i, j;
  uint u4Tmp;

	for(i = 0, j = len - 1; i<j; i++, j--)
	{
		u4Tmp = pBuff[i];
		pBuff[i] = pBuff[j];
		pBuff[j] = u4Tmp;
	}

	return 0;
}

int mmc_wp_segmant(struct mmc *mmc, uint start, uint fgEn)
{
	struct mmc_cmd cmd;
	int err;

	cmd.cmdidx = fgEn?(MMC_CMD_SET_WRITE_PROT):(MMC_CMD_CLR_WRITE_PROT);
	cmd.resp_type = MMC_RSP_R1b;
	cmd.flags = 0;

	if (mmc->high_capacity)
		cmd.cmdarg = start;
	else
		cmd.cmdarg = start * mmc->write_bl_len;

	err = mmc_send_cmd(mmc, &cmd, NULL);

	return err;
}

/* Support kinds of write protect operations
   level:
     0 - All Card
     1 - USER AREA
     2 - BOOT PARTITION
   type:
     0 - Temporary
     1 - Power on
     2 - Permenent
   fgEn:
     0 - Disable WP
     1 - Enable WP
 */
int mmc_wp(struct mmc *mmc, ulong bytestart, uint level, uint type, uint fgEn)
{
	struct mmc_cmd cmd;
	struct mmc_data data;
	uint tmpcsd[4], ret = 0;
	uint writeblklen, erasegroupsz, wpgroupsz, erasegrpdef;

	/* update the ext_csd datas */
	mmc_send_ext_csd(mmc, EXT_CSD);

    /* reverse csd for the normal order*/
	memcpy((void *)tmpcsd, (void *)(mmc->csd), sizeof(mmc->csd)/sizeof(uchar));
    mmc_reverse_uint(tmpcsd, sizeof(tmpcsd)/sizeof(uint));

    /* calculate necessary variable
        - erase group def
        - write block length
        - erase group size
        - write protect group size
      */
    erasegrpdef = EXT_CSD[175];
    writeblklen = (0x1<<((tmpcsd[0]&(0x0F<<22))>>22));

    if(erasegrpdef == 0)
    {
        erasegroupsz = writeblklen;
        erasegroupsz *= (((tmpcsd[1]&(0x1F<<10))>>10) + 1)*(((tmpcsd[1]&(0x1F<<5))>>5) + 1);

        wpgroupsz = erasegroupsz;
        wpgroupsz *= ((tmpcsd[1]&(0x1F<<0))>>0) + 1;
    }
    else
    {
        erasegroupsz = (512*1024);
        erasegroupsz *= (EXT_CSD[224]);

        wpgroupsz = erasegroupsz;
        wpgroupsz *= (EXT_CSD[221]);
    }

	printf("Write Block Len:%08X, Erapse Group:%08X, WP Group:%08X\n", writeblklen, erasegroupsz, wpgroupsz);

    if(level == 0)
    {
        if(type == 1)
        {
            printf("Not support Power-on write protect for All card!\n");
            ret = -1;
            goto end;
        }

        if(((type == 0) && (((tmpcsd[0]&(0x1<<12))>>12) == fgEn)) ||
            ((type == 2) && (((tmpcsd[0]&(0x1<<13))>>13) == fgEn)))
        {
            printf("write protect has been %s!\n", (fgEn == 0)?"disable":"enable");
            ret = 0;
            goto end;
        }

        if(type == 0)
        {
            if(fgEn == 0x1)
            {
                tmpcsd[0] |= (0x1<<12);
            }
            else if(fgEn == 0x0)
            {
                tmpcsd[0] &= (~(0x1<<12));
            }
        }

#if 0
        if(type == 2)
        {
            if(fgEn == 0x1)
            {
                tmpcsd[0] |= (0x1<<13);
            }
            else if(fgEn == 0x0)
            {
                tmpcsd[0] &= (~(0x1<<13));
            }
        }
#endif

    mmc_reverse_uchar((uchar *)tmpcsd, sizeof(tmpcsd)/sizeof(uchar));

    // Send CMD27
    cmd.cmdidx = MMC_CMD_PROGRAM_CSD;
    cmd.cmdarg = 0;
    cmd.resp_type = MMC_RSP_R1;
    cmd.flags |= (dataMode & 0x0F);

    data.src = (char *)(tmpcsd);
    data.blocks = 1;
    data.blocksize = sizeof(tmpcsd)/sizeof(uchar);
    data.flags = MMC_DATA_WRITE;
    if (mmc_send_cmd(mmc, &cmd, &data)) {
        printf("mmc fail to send program CSD cmd\n");
        ret = -1;
        goto end;
    }

    // Send CMD13
    cmd.cmdidx = MMC_CMD_SEND_STATUS;
    cmd.cmdarg = mmc->rca << 16;
    cmd.resp_type = MMC_RSP_R1;
    cmd.flags = 0;
    if (mmc_send_cmd(mmc, &cmd, NULL)) {
        printf("mmc fail to send card status cmd!\n");
        ret = -1;
        goto end;
    }
    if(cmd.response[0] & (0x1<<16))
    {
        printf("Read only section not match the card content!\n");
        ret = -1;
        goto end;
    }

    mmc_reverse_uchar((uchar *)tmpcsd, sizeof(tmpcsd)/sizeof(uchar));
    mmc_reverse_uint(tmpcsd, sizeof(tmpcsd)/sizeof(uint));
    memcpy((void *)(mmc->csd), (void *)tmpcsd, sizeof(mmc->csd)/sizeof(uchar));

  }
  else if(level == 1)
  {
    if(type == 0)
    {
      if(fgEn == 1)
      {
        EXT_CSD[171]&=(~(0x1<<0));
        EXT_CSD[171]&=(~(0x1<<2));
        if(mmc_switch(mmc, EXT_CSD_CMD_SET_NORMAL, 171, EXT_CSD[171])){
          printf("mmc fail to set ext csd!\n");
				  ret = -1;
          goto end;
        }
      }
      if(mmc_wp_segmant(mmc, bytestart/(mmc->read_bl_len), fgEn)){
          printf("mmc fail to write protect segmant!\n");
				  ret = -1;
          goto end;
      }
    }
    else if (type == 1)
    {
      if(fgEn == 0)
      {
        printf("Not support disable Power-on/Permenant write protect\n");
        ret = -1;
        goto end;
      }

      EXT_CSD[171]|=(0x1<<0);
      EXT_CSD[171]|=(0x1<<0);
      EXT_CSD[171]&=(~(0x1<<2));
      if(mmc_switch(mmc, EXT_CSD_CMD_SET_NORMAL, 171, EXT_CSD[171])){
        printf("mmc fail to set ext csd!\n");
				ret = -1;
        goto end;
      }
      if(mmc_wp_segmant(mmc, bytestart/(mmc->read_bl_len), fgEn)){
        printf("mmc fail to write protect segmant!\n");
				ret = -1;
        goto end;
      }
    }
#if 0
    else if(type == 2)
    {
      if(fgEn == 0)
      {
        printf("Not support disable Power-on/Permenant write protect\n");
        ret = -1;
        goto end;
      }
      EXT_CSD[171]|=(0x1<<2);
      if(mmc_switch(mmc, EXT_CSD_CMD_SET_NORMAL, 171, EXT_CSD[171])){
        printf("mmc fail to set ext csd!\n");
				ret = -1;
        goto end;
      }
      if(mmc_wp_segmant(mmc, bytestart/(mmc->read_bl_len), fgEn)){
        printf("mmc fail to write protect segmant!\n");
				ret = -1;
        goto end;
      }
    }
#endif
  }
  else
  {
    if(type == 0)
    {
      printf("Boot partition not support temporary protect!\n");
      ret = -1;
      goto end;
    }
    else if (type == 1)
    {
    	EXT_CSD[173]&=(~(0x1<<2));
    	EXT_CSD[173]&=(~(0x1<<0));
    	if(mmc_switch(mmc, EXT_CSD_CMD_SET_NORMAL, 173, EXT_CSD[173])){
        printf("mmc fail to set ext csd!\n");
				ret = -1;
        goto end;
      }
    }
#if 0
    else if(type == 2)
    {
    	ext_csd[173]|=(0x1<<2);
    	if(mmc_switch(mmc, EXT_CSD_CMD_SET_NORMAL, 173, EXT_CSD[173])){
        printf("mmc fail to set ext csd!\n");
				ret = -1;
        goto end;
      }
    }
#endif
  }

end:
	return ret;
}

int mmc_register(struct mmc *mmc)
{
	/* Setup the universal parts of the block interface just once */
	mmc->block_dev.if_type = IF_TYPE_MMC;
	mmc->block_dev.dev = cur_dev_num++;
	mmc->block_dev.removable = 0;
	mmc->block_dev.block_read = mmc_bread;
	mmc->block_dev.block_write = mmc_bwrite;
	mmc->block_dev.block_erase = mmc_berase;

	mmc->block_dev.byte_read = mmc_byteread;
	mmc->block_dev.byte_write = mmc_bytewrite;

	INIT_LIST_HEAD (&mmc->link);

	list_add_tail (&mmc->link, &mmc_devices);

	return 0;
}

block_dev_desc_t *mmc_get_dev(int dev)
{
	struct mmc *mmc = find_mmc_device(dev);

	return mmc ? &mmc->block_dev : NULL;
}

int mmc_init(struct mmc *mmc)
{
	int err;

  if (i4mmcInit)
  {
    return 0;
  }

	memset(EXT_CSD, 0x00, 512);

	err = mmc->init(mmc);

	if (err)
		return err;

	mmc_set_bus_width(mmc, 1);
	mmc_set_clock(mmc, 1);

#if defined(CC_MT5399) || defined(CONFIG_ARCH_MT5399)|| \
    defined(CC_MT5890) || defined(CONFIG_ARCH_MT5890)|| \
    defined(CC_MT5891) || defined(CONFIG_ARCH_MT5891)|| \
    defined(CC_MT5882) || defined(CONFIG_ARCH_MT5882)|| \
    defined(CC_MT5883) || defined(CONFIG_ARCH_MT5883)|| \
    defined(CC_MT5891) || defined(CONFIG_ARCH_MT5891)
#else
	/* Position 0 Sample Edge Setting*/
	MsdcSetSampleEdge(0);
#endif

	/* Reset the Card */
	err = mmc_go_idle(mmc);

	if (err)
		return err;

	/* Test for SD version 2 */
	//err = mmc_send_if_cond(mmc);

	/* Now try to get the SD card's operating condition */
	//err = sd_send_op_cond(mmc);

	/* If the command timed out, we check for an MMC card */
	err = TIMEOUT;
	if (err == TIMEOUT) {
		err = mmc_send_op_cond(mmc);

		if (err) {
			printf("Card did not respond to voltage select!\n");
			return UNUSABLE_ERR;
		}
  }

	err = mmc_startup(mmc);

  i4mmcInit = 1;
  return err;
}

#if defined(CC_MT5399) || defined(CONFIG_ARCH_MT5399) || \
    defined(CC_MT5890) || defined(CONFIG_ARCH_MT5890) || \
    defined(CC_MT5891) || defined(CONFIG_ARCH_MT5891) || \
    defined(CC_MT5882) || defined(CONFIG_ARCH_MT5882) || \
    defined(CC_MT5883) || defined(CONFIG_ARCH_MT5883) || \
    defined(CC_MT5891) || defined(CONFIG_ARCH_MT5891)
/*
 * fast initialization of mmc
 * version: new
 *
 */
int mmc_init_fast(struct mmc *mmc)
{
  uint mult, freq, tmpval, sec_count = 0;
  u64 cmult = 0, csize = 0;
  char cardtype;
  int err;

  if (i4mmcInit)
    return 0;

  /* ext_csd init */
  memset(EXT_CSD, 0x00, 512);

  /* reverse the value of cid */
  tmpval = mmc->cid[0];mmc->cid[0] = mmc->cid[3];mmc->cid[3] = tmpval;
  tmpval = mmc->cid[1];mmc->cid[1] = mmc->cid[2];mmc->cid[2] = tmpval;

  /* Find device from supported list */
  MsdcFindDev(mmc->cid);

  /* reverse the value of csd */
  tmpval = mmc->csd[0];mmc->csd[0] = mmc->csd[3];mmc->csd[3] = tmpval;
  tmpval = mmc->csd[1];mmc->csd[1] = mmc->csd[2];mmc->csd[2] = tmpval;

  /* capacity > 2GB or not */
  mmc->high_capacity = ((mmc->ocr & OCR_HCS) == OCR_HCS);

  /* mmc version assign */
  mmc->version = MMC_VERSION_UNKNOWN;
  if (mmc->version == MMC_VERSION_UNKNOWN) {
    int version = ((mmc->csd[0] >> 26) & 0xf);

    switch (version) {
      case 0:
        mmc->version = MMC_VERSION_1_2;
        break;
      case 1:
        mmc->version = MMC_VERSION_1_4;
        break;
      case 2:
        mmc->version = MMC_VERSION_2_2;
        break;
      case 3:
        mmc->version = MMC_VERSION_3;
        break;
      case 4:
        mmc->version = MMC_VERSION_4;
        break;
      default:
        mmc->version = MMC_VERSION_1_2;
        break;
    }
  }

  /* old version will not be supported */
  if (mmc->version < MMC_VERSION_4) {
  	printf("old version of mmc device: %d\n", mmc->version);
  	return -1;
  }

  /* mmc ext_csd read */
  err = mmc_send_ext_csd(mmc, EXT_CSD);
  if (err) {
  	printf("<mmc_init_fast> get ext_csd failed(%d)\n", err);
    return err;
  }

  /* mmc tran speed assign */
  freq = fbase[(mmc->csd[0] & 0x7)];
  mult = multipliers[((mmc->csd[0] >> 3) & 0xf)];
  mmc->tran_speed = freq * mult;

  /* mmc read/write block length assign */
  mmc->read_bl_len = 1 << ((mmc->csd[1] >> 16) & 0xf);
  mmc->write_bl_len = 1 << ((mmc->csd[3] >> 22) & 0xf);

  /* card type & caps assign */
  cardtype = EXT_CSD[196] & 0xff;
  //printf("\n card type = %d \n",cardtype);
  mmc->card_caps |= cardtype;
  mmc->card_caps |= MMC_MODE_4BIT | MMC_MODE_8BIT;
  mmc->card_caps &= mmc->host_caps;

  /* HS timing setting */
  tmpval = 0;
  if (mmc->card_caps & MMC_MODE_HS400_MASK)
	tmpval = 3;
  else if (mmc->card_caps & MMC_MODE_HS200_MASK)
  	tmpval = 2;
  else if (mmc->card_caps & MMC_MODE_HS)
    tmpval = 1;
  //printf("[emmc]bus timing switch to %s\n", (tmpval>0)?((tmpval>1)?((tmpval>2)?"HS400":"HS200"):"HS"):"DS");
  //printf("\n uboot bus timing %d \n",tmpval);
  err = mmc_switch(mmc, EXT_CSD_CMD_SET_NORMAL, EXT_CSD_HS_TIMING, tmpval);
  if (err) {
  	printf("<mmc_init_fast> switch high speed timing(%d) failed(%d)\n", tmpval, err);
    return err;
  }

  if(tmpval == 1)
    MsdcSetSpeedMode(SPEED_MODE_HS);
  else if(tmpval == 2)
  	MsdcSetSpeedMode(SPEED_MODE_HS200);
  else if(tmpval == 3)
  	MsdcSetSpeedMode(SPEED_MODE_HS400);
  else
  	MsdcSetSpeedMode(SPEED_MODE_DS);

  /* bus width setting */
  tmpval = 0;
  if (!(mmc->card_caps & MMC_MODE_HS200_MASK))
  	if (mmc->card_caps & MMC_MODE_DDR_MASK)
	  tmpval += 4;

  if (mmc->card_caps & MMC_MODE_8BIT)
  	tmpval += 2;
  else if (mmc->card_caps & MMC_MODE_4BIT)
  	tmpval += 1;
  //printf("[emmc]bus width switch to %d(%s)\n", 0x1<<((tmpval>4)?(tmpval-3):((tmpval>0)?(tmpval+1):tmpval)),
  //	                                           (tmpval>4)?"DDR":"SDR");
  //printf("uboot bus width %d \n",tmpval);
  err = mmc_switch(mmc, EXT_CSD_CMD_SET_NORMAL,
    EXT_CSD_BUS_WIDTH,
    tmpval);
  if (err) {
  	printf("<mmc_init_fast> switch bus width(%) failed(%d)\n", tmpval, err);
    return err;
  }


  if ((tmpval > 4) && !(mmc->card_caps & MMC_MODE_HS400_MASK))
    MsdcSetSpeedMode(SPEED_MODE_DDR50);

  mmc_set_bus_width(mmc, tmpval);

  /* clock frequency setting */
  tmpval = 0;
  if (mmc->card_caps & MMC_MODE_HS400_MASK)
  	tmpval = 400000000;
  else if (mmc->card_caps & MMC_MODE_HS200_MASK)
  	tmpval = 200000000;
  else
    tmpval = 50000000;

  printf("\n[emmc]bus clock switch to %d\n", tmpval);
  mmc_set_clock(mmc, tmpval);

  /* mmc capacity calculate */
  if (mmc->high_capacity) {
    sec_count = *((uint *)(EXT_CSD + 212));
	mmc->capacity = (u64)sec_count;
	mmc->capacity *= 0x200;
  } else {
    csize = (mmc->csd[1] & 0x3ff) << 2
			| (mmc->csd[2] & 0xc0000000) >> 30;
    cmult = (mmc->csd[2] & 0x00038000) >> 15;
	mmc->capacity = (csize + 1) << (cmult + 2);
	mmc->capacity *= mmc->read_bl_len;
  }

  /* fill in device description */
  mmc->block_dev.lun = 0;
  mmc->block_dev.type = MTD_DEV_TYPE_EMMC;
  mmc->block_dev.blksz = mmc->read_bl_len;
  mmc->block_dev.lba = lldiv(mmc->capacity, mmc->read_bl_len);
  sprintf(mmc->block_dev.vendor, "Man %06x Snr %08x", mmc->cid[0] >> 8,
			(mmc->cid[2] << 8) | (mmc->cid[3] >> 24));
  sprintf(mmc->block_dev.product, "%c%c%c%c%c", mmc->cid[0] & 0xff,
			(mmc->cid[1] >> 24), (mmc->cid[1] >> 16) & 0xff,
			(mmc->cid[1] >> 8) & 0xff, mmc->cid[1] & 0xff);
  sprintf(mmc->block_dev.revision, "%d.%d", mmc->cid[2] >> 28,
			(mmc->cid[2] >> 24) & 0xf);
  init_part(&mmc->block_dev);

  i4mmcInit = 1;
  return 0;
}

#else
/*
 * fast initialization of mmc
 * version: old
 *
 */
int mmc_init_fast(struct mmc *mmc)
{
  uint mult, freq, tmp;
  u64 cmult, csize;
  char cardtype;
  int err;

  if (i4mmcInit)
  {
    return 0;
  }

  /* Position 0 Sample Edge Setting*/
  MsdcSetSampleEdge(0);

  memset(EXT_CSD, 0x00, 512);

  tmp = mmc->cid[0];
  mmc->cid[0] = mmc->cid[3];
  mmc->cid[3] = tmp;
  tmp = mmc->cid[1];
  mmc->cid[1] = mmc->cid[2];
  mmc->cid[2] = tmp;

  /* Find device from supported list*/
  MsdcFindDev(mmc->cid);

  tmp = mmc->csd[0];
  mmc->csd[0] = mmc->csd[3];
  mmc->csd[3] = tmp;
  tmp = mmc->csd[1];
  mmc->csd[1] = mmc->csd[2];
  mmc->csd[2] = tmp;

  mmc->version = MMC_VERSION_UNKNOWN;
  mmc->high_capacity = ((mmc->ocr & OCR_HCS) == OCR_HCS);
  if (mmc->version == MMC_VERSION_UNKNOWN) {
    int version = ((mmc->csd[0] >> 26) & 0xf);

    switch (version) {
      case 0:
        mmc->version = MMC_VERSION_1_2;
        break;
      case 1:
        mmc->version = MMC_VERSION_1_4;
        break;
      case 2:
        mmc->version = MMC_VERSION_2_2;
        break;
      case 3:
        mmc->version = MMC_VERSION_3;
        break;
      case 4:
        mmc->version = MMC_VERSION_4;
        break;
      default:
        mmc->version = MMC_VERSION_1_2;
        break;
    }
  }

  /* divide frequency by 10, since the mults are 10x bigger */
  freq = fbase[(mmc->csd[0] & 0x7)];
  mult = multipliers[((mmc->csd[0] >> 3) & 0xf)];

  mmc->tran_speed = freq * mult;

  mmc->read_bl_len = 1 << ((mmc->csd[1] >> 16) & 0xf);
  mmc->write_bl_len = 1 << ((mmc->csd[3] >> 22) & 0xf);

  if (mmc->high_capacity) {
    csize = (mmc->csd[1] & 0x3f) << 16
			| (mmc->csd[2] & 0xffff0000) >> 16;
    cmult = 8;
    //cmult = 0x7;     //modify by shunli wang at 2011.05.12
  } else {
    csize = (mmc->csd[1] & 0x3ff) << 2
			| (mmc->csd[2] & 0xc0000000) >> 30;
    cmult = (mmc->csd[2] & 0x00038000) >> 15;
  }

  mmc->capacity = (csize + 1) << (cmult + 2);
  mmc->capacity *= mmc->read_bl_len;

  if (mmc->read_bl_len > 512)
      mmc->read_bl_len = 512;

  if (mmc->write_bl_len > 512)
      mmc->write_bl_len = 512;

  mmc->card_caps = 0;
  /* Only version 4 supports high-speed */
  if (mmc->version >= MMC_VERSION_4)
  {
    mmc->card_caps |= MMC_MODE_4BIT;

    /* Position 1 Sample Edge Setting
	   I think it is necessary to handle high speed mode at low frequency(400KHz);
	   It is some special case for timing;
	   And switch command - cmd6 should be applied by new timing becuase of its response.
      */
    MsdcSetSampleEdge(1);

    err = mmc_switch(mmc, EXT_CSD_CMD_SET_NORMAL, EXT_CSD_HS_TIMING, 1);
    if (err)
      return err;

    /* Now check to see that it worked */
    err = mmc_send_ext_csd(mmc, EXT_CSD);

    if (err)
      return err;

    cardtype = EXT_CSD[196] & 0xf;
    /* No high-speed support */
    if (!EXT_CSD[185])
      return 0;

    /* High Speed is set, there are two types: 52MHz and 26MHz */
    if (cardtype & MMC_HS_52MHZ)
      mmc->card_caps |= MMC_MODE_HS_52MHz | MMC_MODE_HS;
    else
      mmc->card_caps |= MMC_MODE_HS;
  }

  /* Restrict card's capabilities by what the host can do */
  // It is a bug modified by shunli.wang
  mmc->card_caps |= MMC_MODE_8BIT;
  mmc->card_caps &= mmc->host_caps;

  if (mmc->card_caps & MMC_MODE_4BIT) {
    /* Set the card to use 4 bit*/
    err = mmc_switch(mmc, EXT_CSD_CMD_SET_NORMAL,
    EXT_CSD_BUS_WIDTH,
    EXT_CSD_BUS_WIDTH_4);

    if (err)
      return err;

    mmc_set_bus_width(mmc, 4);
  } else if (mmc->card_caps & MMC_MODE_8BIT) {
    /* Set the card to use 8 bit*/
    err = mmc_switch(mmc, EXT_CSD_CMD_SET_NORMAL,
					EXT_CSD_BUS_WIDTH,
					EXT_CSD_BUS_WIDTH_8);

    if (err)
      return err;

    mmc_set_bus_width(mmc, 8);
  }

  if (mmc->card_caps & MMC_MODE_HS) {
    if (mmc->card_caps & MMC_MODE_HS_52MHz)
    {
      mmc_set_clock(mmc, 52000000);}
    else
				mmc_set_clock(mmc, 26000000);
  } else
    mmc_set_clock(mmc, 20000000);

  /* Position 2 Sample Edge Setting*/
  MsdcSetSampleEdge(2);

  if (mmc->version >= MMC_VERSION_4) {
    /* check  ext_csd version and capacity */
    if ((EXT_CSD[192] >= 2) && (mmc->high_capacity)) {
      mmc->capacity = EXT_CSD[212] << 0 | EXT_CSD[213] << 8 |
					EXT_CSD[214] << 16 | EXT_CSD[215] << 24;
      mmc->capacity *= 512;
    }
  }

  /* fill in device description */
  mmc->block_dev.lun = 0;
  mmc->block_dev.type = MTD_DEV_TYPE_EMMC;
  mmc->block_dev.blksz = mmc->read_bl_len;
  mmc->block_dev.lba = lldiv(mmc->capacity, mmc->read_bl_len);
  sprintf(mmc->block_dev.vendor, "Man %06x Snr %08x", mmc->cid[0] >> 8,
			(mmc->cid[2] << 8) | (mmc->cid[3] >> 24));
  sprintf(mmc->block_dev.product, "%c%c%c%c%c", mmc->cid[0] & 0xff,
			(mmc->cid[1] >> 24), (mmc->cid[1] >> 16) & 0xff,
			(mmc->cid[1] >> 8) & 0xff, mmc->cid[1] & 0xff);
  sprintf(mmc->block_dev.revision, "%d.%d", mmc->cid[2] >> 28,
			(mmc->cid[2] >> 24) & 0xf);
  init_part(&mmc->block_dev);

  i4mmcInit = 1;
  return 0;
}
#endif

/*
 * CPU and board-specific MMC initializations.  Aliased function
 * signals caller to move on
 */

/* not used, we marked it.
static int __def_mmc_init(bd_t *bis)
{
	return -1;
}*/

//int cpu_mmc_init(bd_t *bis) __attribute__((weak, alias("__def_mmc_init")));
//int board_mmc_init(bd_t *bis) __attribute__((weak, alias("__def_mmc_init")));

void print_mmc_devices(char separator)
{
  int i = 0;
	struct mmc *m;
	struct list_head *entry;

	list_for_each(entry, &mmc_devices)
  {
    m = list_entry(entry, struct mmc, link);
    memcpy(&emmc_info[i], m, sizeof(struct mmc));
    i++;

		printf("%s: %d", m->name, m->block_dev.dev);

		if (entry->next != &mmc_devices)
			printf("%c ", separator);
	}

	printf("\n");
}

int mmc_initialize(bd_t *bis)
{
	INIT_LIST_HEAD (&mmc_devices);
	cur_dev_num = 0;

extern int board_mmc_init(bd_t *bis);
extern int cpu_mmc_init(bd_t *bis);

	if (board_mmc_init(bis) < 0)
		cpu_mmc_init(bis);

	print_mmc_devices(',');

	return 0;
}

#endif // CC_EMMC_BOOT

