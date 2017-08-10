/*
 * Direct MTD block device access
 *
 * Copyright © 1999-2010 David Woodhouse <dwmw2@infradead.org>
 * Copyright © 2000-2003 Nicolas Pitre <nico@fluxnic.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/vmalloc.h>

#include <linux/mtd/mtd.h>
#include <linux/mtd/blktrans.h>
#include <linux/mtd/mtk_mtdpart.h>
#include <linux/mutex.h>


struct mtdblk_dev {
	struct mtd_blktrans_dev mbd;
	int count;
	struct mutex cache_mutex;
	unsigned char *cache_data;
	unsigned long cache_offset;
	unsigned int cache_size;
	enum { STATE_EMPTY, STATE_CLEAN, STATE_DIRTY } cache_state;
};

static DEFINE_MUTEX(mtdblks_lock);

static u_int32_t *squashfs_mapping_table[MAX_MTD_DEVICES];

static int mtdblock_is_squashfs(struct mtd_info *mtd)
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
    
    if (!mtd->name)
    {
        return 0;
    }
    
    if (i4NFBPartitionFastboot(mtd))
    {
        return 0;
    }

    return 1;
}

static void squashfs_create_mapping_table (struct mtd_info *mtd)
{
    u_int32_t i, index = 0, numBlocks;
    u_int32_t *mapping_table;
    
    mapping_table = squashfs_mapping_table[mtd->index];	
    if (mapping_table)
    {
        return;
    }
	
    numBlocks = mtd->size >> mtd->erasesize_shift;
    mapping_table = kmalloc(numBlocks * sizeof(mapping_table[0]), GFP_KERNEL);
    if (!mapping_table)
    {
        BUG();
        return;
    }
    
    memset(mapping_table, 0xFFFFFFFF, numBlocks * sizeof(mapping_table[0]));

    for (i = 0; i < numBlocks; i++)
    {
        if (mtd_block_isbad(mtd, i * mtd->erasesize))
        {
            printk(KERN_INFO "mtdblock%d: block %d is bad!\n", mtd->index, i);
            continue;
        }
        
        mapping_table[index] = i;
        index++;
    }
    
    squashfs_mapping_table[mtd->index] = mapping_table;
}

static void squashfs_remove_mapping_table (struct mtd_info *mtd)
{
    int i = mtd->index;
    
    if (squashfs_mapping_table[i])
    {
        kfree(squashfs_mapping_table[i]);
        squashfs_mapping_table[i] = NULL;
    }
}

static void squashfs_init_mapping_table (void)
{
    int i;
    
    for (i = 0; i < MAX_MTD_DEVICES; i++)
    {
    	squashfs_mapping_table[i] = NULL;
    }
}

static void squashfs_clean_mapping_table (void)
{
    int i;
  
    for (i = 0; i < MAX_MTD_DEVICES; i++)
    {
        if (squashfs_mapping_table[i])
        {
            kfree(squashfs_mapping_table[i]);
            squashfs_mapping_table[i] = NULL;
        }
    }
}

/*
 * Cache stuff...
 *
 * Since typical flash erasable sectors are much larger than what Linux's
 * buffer cache can handle, we must implement read-modify-write on flash
 * sectors for each block write requests.  To avoid over-erasing flash sectors
 * and to speed things up, we locally cache a whole flash sector while it is
 * being written to until a different sector is required.
 */

static void erase_callback(struct erase_info *done)
{
	wait_queue_head_t *wait_q = (wait_queue_head_t *)done->priv;
	wake_up(wait_q);
}

int _i4TestMTDPerformance = 0;
static int erase_write (struct mtd_info *mtd, unsigned long pos,
			int len, const char *buf)
{
	struct erase_info erase;
	DECLARE_WAITQUEUE(wait, current);
	wait_queue_head_t wait_q;
	size_t retlen;
	int ret;

    if (_i4TestMTDPerformance)
    {
        int i;
        for (i = 0; i < 30; i++)
        {
            struct timeval t0, t1;
            size_t temp;

            size_t test_len = 8;
            uint8_t *testbuf = vmalloc(test_len * 1024 * 1024);

            if (!testbuf)
            {
                BUG();
                return -ENOMEM;
            }
            
            if ((i % 3) == 0)
            {
            memset(testbuf, 0, test_len * 1024 * 1024);
            }
            else if ((i % 3) == 1)
            {
                memset(testbuf, 0x5A, test_len * 1024 * 1024);
            }
            else
            {
                memset(testbuf, 0xA5, test_len * 1024 * 1024);
            }
                
            init_waitqueue_head(&wait_q);
            erase.mtd = mtd;
            erase.callback = erase_callback;
            erase.addr = pos;
            erase.len = test_len * 1024 * 1024;
            erase.priv = (u_long)&wait_q;

            set_current_state(TASK_INTERRUPTIBLE);
            add_wait_queue(&wait_q, &wait);

            do_gettimeofday(&t0);
            mtd_erase(mtd, &erase);
            do_gettimeofday(&t1);
            temp = (t1.tv_sec - t0.tv_sec)*1000000 + (t1.tv_usec - t0.tv_usec);
            printk(KERN_CRIT "erase time(8M):  %d us, erase speed: %d (0.1M/s)\n",
                (int)temp, test_len*1000000*10/temp);

            schedule();  /* Wait for erase to finish. */
            remove_wait_queue(&wait_q, &wait);

            do_gettimeofday(&t0);
            mtd_write(mtd, pos, test_len * 1024 * 1024, &retlen, testbuf);
            do_gettimeofday(&t1);
            temp = (t1.tv_sec - t0.tv_sec)*1000000 + (t1.tv_usec - t0.tv_usec);
            printk(KERN_CRIT "write time(8M):  %d us, write speed: %d (0.1M/s)\n", 
                (int)temp, test_len*1000000*10/temp);

            do_gettimeofday(&t0);
            mtd_read(mtd, pos, test_len * 1024 * 1024, &retlen, testbuf);
            do_gettimeofday(&t1);
            temp = (t1.tv_sec - t0.tv_sec)*1000000 + (t1.tv_usec - t0.tv_usec);
            printk(KERN_CRIT "read time(8M):  %d us, read speed: %d (0.1M/s)\n", 
                (int)temp, test_len*1000000*10/temp);
            
            printk(KERN_CRIT "\n");

            vfree(testbuf);
        }
    }

    if (i4NFBPartitionFastboot(mtd))
    {
        return i4NFBPartitionWrite(mtd->index, pos, (u_int32_t)buf, len);
    }

	/*
	 * First, let's erase the flash block.
	 */
	init_waitqueue_head(&wait_q);
	erase.mtd = mtd;
	erase.callback = erase_callback;
	erase.addr = pos;
	erase.len = len;
	erase.priv = (u_long)&wait_q;

	set_current_state(TASK_INTERRUPTIBLE);
	add_wait_queue(&wait_q, &wait);

	ret = mtd_erase(mtd, &erase);
	if (ret) {
		set_current_state(TASK_RUNNING);
		remove_wait_queue(&wait_q, &wait);
		printk (KERN_WARNING "mtdblock: erase of region [0x%lx, 0x%x] "
				     "on \"%s\" failed\n",
			pos, len, mtd->name);
		return ret;
	}

	schedule();  /* Wait for erase to finish. */
	remove_wait_queue(&wait_q, &wait);

	/*
	 * Next, write the data to flash.
	 */

	ret = mtd_write(mtd, pos, len, &retlen, buf);
	if (ret)
		return ret;
	if (retlen != len)
		return -EIO;
	return 0;
}


static int write_cached_data (struct mtdblk_dev *mtdblk)
{
	struct mtd_info *mtd = mtdblk->mbd.mtd;
	int ret;

	if (mtdblk->cache_state != STATE_DIRTY)
		return 0;

	pr_debug("mtdblock: writing cached data for \"%s\" "
			"at 0x%lx, size 0x%x\n", mtd->name,
			mtdblk->cache_offset, mtdblk->cache_size);

	ret = erase_write (mtd, mtdblk->cache_offset,
			   mtdblk->cache_size, mtdblk->cache_data);
	if (ret)
		return ret;

	/*
	 * Here we could arguably set the cache state to STATE_CLEAN.
	 * However this could lead to inconsistency since we will not
	 * be notified if this content is altered on the flash by other
	 * means.  Let's declare it empty and leave buffering tasks to
	 * the buffer cache instead.
	 */
	mtdblk->cache_state = STATE_EMPTY;
	return 0;
}


static int do_cached_write (struct mtdblk_dev *mtdblk, unsigned long pos,
			    int len, const char *buf)
{
	struct mtd_info *mtd = mtdblk->mbd.mtd;
	unsigned int sect_size = mtdblk->cache_size;
	size_t retlen;
	int ret;

	pr_debug("mtdblock: write on \"%s\" at 0x%lx, size 0x%x\n",
		mtd->name, pos, len);

    if (!sect_size)
    {
        if (i4NFBPartitionFastboot(mtd))
        {
            return i4NFBPartitionWrite(mtd->index, pos, (u_int32_t)buf, len);
        }
        else
        {
            return mtd_write(mtd, pos, len, &retlen, buf);
        }
    }

	while (len > 0) {
		unsigned long sect_start = (pos/sect_size)*sect_size;
		unsigned int offset = pos - sect_start;
		unsigned int size = sect_size - offset;
		if( size > len )
			size = len;

		if (size == sect_size) {
			/*
			 * We are covering a whole sector.  Thus there is no
			 * need to bother with the cache while it may still be
			 * useful for other partial writes.
			 */
			ret = erase_write (mtd, pos, size, buf);
			if (ret)
				return ret;
		} else {
			/* Partial sector: need to use the cache */

			if (mtdblk->cache_state == STATE_DIRTY &&
			    mtdblk->cache_offset != sect_start) {
				ret = write_cached_data(mtdblk);
				if (ret)
					return ret;
			}

			if (mtdblk->cache_state == STATE_EMPTY ||
			    mtdblk->cache_offset != sect_start) {
				/* fill the cache with the current sector */
				mtdblk->cache_state = STATE_EMPTY;

                if (i4NFBPartitionFastboot(mtd))
                {
                    ret = i4NFBPartitionRead(mtd->index, sect_start, (u_int32_t)mtdblk->cache_data, sect_size);
                    if (ret)
                    {
                        return ret;
                    }
                }
                else
                {
                    ret = mtd_read(mtd, sect_start, sect_size, &retlen, mtdblk->cache_data);
                    if (ret)
                    {
                        if (ret == -EUCLEAN)
                        {
                            printk(KERN_INFO "Total %d fixable bit-flips has been detected \n", mtd->ecc_stats.corrected);
                        }
                        else
                        {
                            return ret;
                        }
                    }
                    
                    if (retlen != sect_size)
                    {
                        return -EIO;
                    }
                }

				mtdblk->cache_offset = sect_start;
				mtdblk->cache_size = sect_size;
				mtdblk->cache_state = STATE_CLEAN;
			}

			/* write data to our local cache */
			memcpy (mtdblk->cache_data + offset, buf, size);
			mtdblk->cache_state = STATE_DIRTY;
		}

		buf += size;
		pos += size;
		len -= size;
	}

	return 0;
}


static int do_cached_read (struct mtdblk_dev *mtdblk, unsigned long pos,
			   int len, char *buf)
{
	struct mtd_info *mtd = mtdblk->mbd.mtd;
	unsigned int sect_size = mtdblk->cache_size;
	size_t retlen = 0;
	int ret;

	pr_debug("mtdblock: read on \"%s\" at 0x%lx, size 0x%x\n",
			mtd->name, pos, len);

	if (!sect_size)
    {
        ret = mtd_read(mtd, pos, len, &retlen, buf);
        if (ret)
        {
            if (ret == -EUCLEAN)
            {
                printk(KERN_INFO "Total %d fixable bit-flips has been detected \n", mtd->ecc_stats.corrected);
            }
            else
            {
                return ret;
            }
        }
        
        if (retlen != len)
        {
            return -EIO;
        }
        else
        {
            return 0;
        }
    }

	while (len > 0) {
		unsigned long sect_start = (pos/sect_size)*sect_size;
		unsigned int offset = pos - sect_start;
		unsigned int size = sect_size - offset;
		if (size > len)
			size = len;

		/*
		 * Check if the requested data is already cached
		 * Read the requested amount of data from our internal cache if it
		 * contains what we want, otherwise we read the data directly
		 * from flash.
		 */
		if (mtdblk->cache_state != STATE_EMPTY &&
		    mtdblk->cache_offset == sect_start) {
			memcpy (buf, mtdblk->cache_data + offset, size);
		} else {
			ret = mtd_read(mtd, pos, size, &retlen, buf);
			if (ret)
            {
                 if (ret == -EUCLEAN) 
  			/*
  			 * -EUCLEAN is reported if there was a bit-flip which
  			 * was corrected, so this is harmless.
  			 *
  			 * We do not report about it here unless debugging is
  			 * enabled. A corresponding message will be printed
  			 * later, when it is has been scrubbed.
      			 */
                     printk(KERN_INFO "Total %d fixable bit-flips has been detected \n", mtd->ecc_stats.corrected);
                 else
                     return ret;
            }
			if (retlen != size)
				return -EIO;
		}

		buf += size;
		pos += size;
		len -= size;
	}

	return 0;
}

static int squashfs_readsect(struct mtdblk_dev *mtdblk, unsigned long pos, int len, char *buf)
{
    struct mtd_info *mtd = mtdblk->mbd.mtd;
    u_int32_t *mapping_table;
    u_int32_t numBlocks, phyblock;

    // For squashfs read
    mapping_table = squashfs_mapping_table[mtd->index];
    if (!mapping_table)
    {
        BUG();
    }
    
    numBlocks = mtd->size >> mtd->erasesize_shift;
    phyblock = mapping_table[pos >> mtd->erasesize_shift];
    
    if (phyblock >= numBlocks)
    {
        printk(KERN_ERR "squashfs_readsect: too many bad block, phyblock = %d, numBlocks = %d!n", phyblock, numBlocks);
        BUG();
    }

    pos = (phyblock * mtd->erasesize) | (pos & (mtd->erasesize - 1));
    return do_cached_read(mtdblk, pos, len, buf);
}

static int mtdblock_readsect(struct mtd_blktrans_dev *dev,
			      unsigned long block, char *buf)
{
	struct mtdblk_dev *mtdblk = container_of(dev, struct mtdblk_dev, mbd);
	struct mtd_info *mtd = dev->mtd;

    if (i4NFBPartitionFastboot(mtd))
    {
        return i4NFBPartitionRead(mtd->index, block<<11, (u_int32_t)buf, 2048);
    }
    
    if (mtdblock_is_squashfs(mtd))
    {
        return squashfs_readsect(mtdblk, block<<11, 2048, buf);
    }

    return do_cached_read(mtdblk, block<<11, 2048, buf);
}

static int mtdblock_writesect(struct mtd_blktrans_dev *dev,
			      unsigned long block, char *buf)
{
	struct mtdblk_dev *mtdblk = container_of(dev, struct mtdblk_dev, mbd);
	if (unlikely(!mtdblk->cache_data && mtdblk->cache_size)) {
		mtdblk->cache_data = vmalloc(mtdblk->mbd.mtd->erasesize);
		if (!mtdblk->cache_data)
			return -EINTR;
		/* -EINTR is not really correct, but it is the best match
		 * documented in man 2 write for all cases.  We could also
		 * return -EAGAIN sometimes, but why bother?
		 */
	}
	return do_cached_write(mtdblk, block<<11, 2048, buf);
}

static int mtdblock_open(struct mtd_blktrans_dev *mbd)
{
	struct mtdblk_dev *mtdblk = container_of(mbd, struct mtdblk_dev, mbd);

	pr_debug("mtdblock_open\n");

	mutex_lock(&mtdblks_lock);
	
    if (mtdblock_is_squashfs(mbd->mtd))
    {
        squashfs_create_mapping_table(mbd->mtd);
    }
	
	if (mtdblk->count) {
		mtdblk->count++;
		mutex_unlock(&mtdblks_lock);
		return 0;
	}

	/* OK, it's not open. Create cache info for it */
	mtdblk->count = 1;
	mutex_init(&mtdblk->cache_mutex);
	mtdblk->cache_state = STATE_EMPTY;
	if (!(mbd->mtd->flags & MTD_NO_ERASE) && mbd->mtd->erasesize) {
		mtdblk->cache_size = mbd->mtd->erasesize;
		mtdblk->cache_data = NULL;
	}

	mutex_unlock(&mtdblks_lock);

	pr_debug("ok\n");

	return 0;
}

static void mtdblock_release(struct mtd_blktrans_dev *mbd)
{
	struct mtdblk_dev *mtdblk = container_of(mbd, struct mtdblk_dev, mbd);

	pr_debug("mtdblock_release\n");

	mutex_lock(&mtdblks_lock);

	mutex_lock(&mtdblk->cache_mutex);
	write_cached_data(mtdblk);
	mutex_unlock(&mtdblk->cache_mutex);

	if (!--mtdblk->count) {
		/*
		 * It was the last usage. Free the cache, but only sync if
		 * opened for writing.
		 */
		if (mbd->file_mode & FMODE_WRITE)
			mtd_sync(mbd->mtd);
		vfree(mtdblk->cache_data);
	}

	mutex_unlock(&mtdblks_lock);

	pr_debug("ok\n");
}

static int mtdblock_flush(struct mtd_blktrans_dev *dev)
{
	struct mtdblk_dev *mtdblk = container_of(dev, struct mtdblk_dev, mbd);

	mutex_lock(&mtdblk->cache_mutex);
	write_cached_data(mtdblk);
	mutex_unlock(&mtdblk->cache_mutex);
	mtd_sync(dev->mtd);
	return 0;
}

static void mtdblock_add_mtd(struct mtd_blktrans_ops *tr, struct mtd_info *mtd)
{
	struct mtdblk_dev *dev = kzalloc(sizeof(*dev), GFP_KERNEL);

	if (!dev)
		return;

	dev->mbd.mtd = mtd;
	dev->mbd.devnum = mtd->index;

	dev->mbd.size = mtd->size >> 11;
	dev->mbd.tr = tr;

	if (!(mtd->flags & MTD_WRITEABLE))
		dev->mbd.readonly = 1;

	if (add_mtd_blktrans_dev(&dev->mbd))
		kfree(dev);
}

static void mtdblock_remove_dev(struct mtd_blktrans_dev *dev)
{
    squashfs_remove_mapping_table(dev->mtd);
    
	del_mtd_blktrans_dev(dev);
}

static struct mtd_blktrans_ops mtdblock_tr = {
	.name		= "mtdblock",
	.major		= 31,
	.part_bits	= 0,
	.blksize 	= 2048,
	.open		= mtdblock_open,
	.flush		= mtdblock_flush,
	.release	= mtdblock_release,
	.readsect	= mtdblock_readsect,
	.writesect	= mtdblock_writesect,
	.add_mtd	= mtdblock_add_mtd,
	.remove_dev	= mtdblock_remove_dev,
	.owner		= THIS_MODULE,
};

static int __init init_mtdblock(void)
{
	squashfs_init_mapping_table();
	return register_mtd_blktrans(&mtdblock_tr);
}

static void __exit cleanup_mtdblock(void)
{
    squashfs_clean_mapping_table();
	deregister_mtd_blktrans(&mtdblock_tr);
}

module_init(init_mtdblock);
module_exit(cleanup_mtdblock);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Nicolas Pitre <nico@fluxnic.net> et al.");
MODULE_DESCRIPTION("Caching read/erase/writeback block device emulation access to MTD devices");
