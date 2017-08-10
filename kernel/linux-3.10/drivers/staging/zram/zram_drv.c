/*
 * Compressed RAM block device
 *
 * Copyright (C) 2008, 2009, 2010  Nitin Gupta
 *
 * This code is released using a dual license strategy: BSD/GPL
 * You can choose the licence that better fits your requirements.
 *
 * Released under the terms of 3-clause BSD License
 * Released under the terms of GNU General Public License Version 2.0
 *
 * Project home: http://compcache.googlecode.com
 */

#define KMSG_COMPONENT "zram"
#define pr_fmt(fmt) KMSG_COMPONENT ": " fmt

#ifdef CONFIG_ZRAM_DEBUG
#define DEBUG
#endif

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/bio.h>
#include <linux/bitops.h>
#include <linux/blkdev.h>
#include <linux/buffer_head.h>
#include <linux/device.h>
#include <linux/genhd.h>
#include <linux/highmem.h>
#include <linux/slab.h>
#include <linux/lzo.h>
#include <linux/string.h>
#include <linux/vmalloc.h>
#if 1
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#endif

#include "zram_drv.h"

/* Globals */
static int zram_major;
struct zram *zram_devices;

/* Module params (documentation at end) */
static unsigned int num_devices;

static void zram_stat_inc(u32 *v)
{
	*v = *v + 1;
}

static void zram_stat_dec(u32 *v)
{
	*v = *v - 1;
}

static void zram_stat64_add(struct zram *zram, u64 *v, u64 inc)
{
	spin_lock(&zram->stat64_lock);
	*v = *v + inc;
	spin_unlock(&zram->stat64_lock);
}

static void zram_stat64_sub(struct zram *zram, u64 *v, u64 dec)
{
	spin_lock(&zram->stat64_lock);
	*v = *v - dec;
	spin_unlock(&zram->stat64_lock);
}

static void zram_stat64_inc(struct zram *zram, u64 *v)
{
	zram_stat64_add(zram, v, 1);
}

static int zram_test_flag(struct zram *zram, u32 index,
			enum zram_pageflags flag)
{
	return zram->table[index].flags & BIT(flag);
}

static void zram_set_flag(struct zram *zram, u32 index,
			enum zram_pageflags flag)
{
	zram->table[index].flags |= BIT(flag);
}

static void zram_clear_flag(struct zram *zram, u32 index,
			enum zram_pageflags flag)
{
	zram->table[index].flags &= ~BIT(flag);
}

static int page_zero_filled(void *ptr)
{
	unsigned int pos;
	unsigned long *page;

	page = (unsigned long *)ptr;

	for (pos = 0; pos != PAGE_SIZE / sizeof(*page); pos++) {
		if (page[pos])
			return 0;
	}

	return 1;
}

static void zram_set_disksize(struct zram *zram, size_t totalram_bytes)
{
	if (!zram->disksize) {
		pr_info(
		"disk size not provided. You can use disksize_kb module "
		"param to specify size.\nUsing default: (%u%% of RAM).\n",
		default_disksize_perc_ram
		);
		zram->disksize = default_disksize_perc_ram *
					(totalram_bytes / 100);
	}

	if (zram->disksize > 2 * (totalram_bytes)) {
		pr_info(
		"There is little point creating a zram of greater than "
		"twice the size of memory since we expect a 2:1 compression "
		"ratio. Note that zram uses about 0.1%% of the size of "
		"the disk when not in use so a huge zram is "
		"wasteful.\n"
		"\tMemory Size: %zu kB\n"
		"\tSize you selected: %llu kB\n"
		"Continuing anyway ...\n",
		totalram_bytes >> 10, zram->disksize
		);
	}

	zram->disksize &= PAGE_MASK;
}

static void zram_free_page(struct zram *zram, size_t index)
{
	u32 clen;
	void *obj;

	struct xv_page *page = zram->table[index].page;
	u32 offset = zram->table[index].offset;

	if (unlikely(!page)) {
		/*
		 * No memory is allocated for zero filled pages.
		 * Simply clear zero page flag.
		 */
		if (zram_test_flag(zram, index, ZRAM_ZERO)) {
			zram_clear_flag(zram, index, ZRAM_ZERO);
			zram_stat_dec(&zram->stats.pages_zero);
		}
		return;
	}

	if (unlikely(zram_test_flag(zram, index, ZRAM_UNCOMPRESSED))) {
		clen = PAGE_SIZE;
		xv_free_page(zram->mem_pool, page);
		zram_clear_flag(zram, index, ZRAM_UNCOMPRESSED);
		zram_stat_dec(&zram->stats.pages_expand);
		goto out;
	}

	obj = xv_map(page) + offset;
	clen = xv_get_object_size(obj) - sizeof(struct zobj_header);
	xv_unmap(page, obj);

	xv_free(zram->mem_pool, page, offset);
	if (clen <= PAGE_SIZE / 2)
		zram_stat_dec(&zram->stats.good_compress);

out:
	zram_stat64_sub(zram, &zram->stats.compr_size, clen);
	zram_stat_dec(&zram->stats.pages_stored);

	zram->table[index].page = NULL;
	zram->table[index].offset = 0;
}

static void handle_zero_page(struct bio_vec *bvec)
{
	struct page *page = bvec->bv_page;
	void *user_mem;

	user_mem = kmap_atomic(page);
	memset(user_mem + bvec->bv_offset, 0, bvec->bv_len);
	kunmap_atomic(user_mem);

	flush_dcache_page(page);
}

static void handle_uncompressed_page(struct zram *zram, struct bio_vec *bvec,
				     u32 index, int offset)
{
	struct page *page = bvec->bv_page;
	unsigned char *user_mem, *cmem;

	user_mem = kmap_atomic(page);
	cmem = xv_map(zram->table[index].page);

	memcpy(user_mem + bvec->bv_offset, cmem + offset, bvec->bv_len);
	xv_unmap(zram->table[index].page, cmem);
	kunmap_atomic(user_mem);

	flush_dcache_page(page);
}

static inline int is_partial_io(struct bio_vec *bvec)
{
	return bvec->bv_len != PAGE_SIZE;
}

static int zram_bvec_read(struct zram *zram, struct bio_vec *bvec,
			  u32 index, int offset, struct bio *bio)
{
	int ret;
	size_t clen;
	struct page *page;
	struct zobj_header *zheader;
	unsigned char *user_mem, *cmem, *uncmem = NULL;

	page = bvec->bv_page;

	if (zram_test_flag(zram, index, ZRAM_ZERO)) {
		handle_zero_page(bvec);
		return 0;
	}

	/* Requested page is not present in compressed area */
	if (unlikely(!zram->table[index].page)) {
		pr_debug("Read before write: sector=%lu, size=%u",
			 (ulong)(bio->bi_sector), bio->bi_size);
		handle_zero_page(bvec);
		return 0;
	}

	/* Page is stored uncompressed since it's incompressible */
	if (unlikely(zram_test_flag(zram, index, ZRAM_UNCOMPRESSED))) {
		handle_uncompressed_page(zram, bvec, index, offset);
		return 0;
	}

	if (is_partial_io(bvec)) {
		/* Use  a temporary buffer to decompress the page */
		uncmem = kmalloc(PAGE_SIZE, GFP_KERNEL);
		if (!uncmem) {
			pr_info("Error allocating temp memory!\n");
			return -ENOMEM;
		}
	}

	user_mem = kmap_atomic(page);
	if (!is_partial_io(bvec))
		uncmem = user_mem;
	clen = PAGE_SIZE;

	cmem = xv_map(zram->table[index].page) +
		zram->table[index].offset;

	ret = lzo1x_decompress_safe(cmem + sizeof(*zheader),
				    xv_get_object_size(cmem) - sizeof(*zheader),
				    uncmem, &clen);

	if (is_partial_io(bvec)) {
		memcpy(user_mem + bvec->bv_offset, uncmem + offset,
		       bvec->bv_len);
		kfree(uncmem);
	}

	xv_unmap(zram->table[index].page, cmem);
	kunmap_atomic(user_mem);

	/* Should NEVER happen. Return bio error if it does. */
	if (unlikely(ret != LZO_E_OK)) {
		pr_err("Decompression failed! err=%d, page=%u\n", ret, index);
		zram_stat64_inc(zram, &zram->stats.failed_reads);
		return ret;
	}

	flush_dcache_page(page);

	return 0;
}

static int zram_read_before_write(struct zram *zram, char *mem, u32 index)
{
	int ret;
	size_t clen = PAGE_SIZE;
	struct zobj_header *zheader;
	unsigned char *cmem;

	if (zram_test_flag(zram, index, ZRAM_ZERO) ||
	    !zram->table[index].page) {
		memset(mem, 0, PAGE_SIZE);
		return 0;
	}

	cmem = xv_map(zram->table[index].page) +
		zram->table[index].offset;

	/* Page is stored uncompressed since it's incompressible */
	if (unlikely(zram_test_flag(zram, index, ZRAM_UNCOMPRESSED))) {
		memcpy(mem, cmem, PAGE_SIZE);
		kunmap_atomic(cmem);
		return 0;
	}

	ret = lzo1x_decompress_safe(cmem + sizeof(*zheader),
				    xv_get_object_size(cmem) - sizeof(*zheader),
				    mem, &clen);
	xv_unmap(zram->table[index].page, cmem);

	/* Should NEVER happen. Return bio error if it does. */
	if (unlikely(ret != LZO_E_OK)) {
		pr_err("Decompression failed! err=%d, page=%u\n", ret, index);
		zram_stat64_inc(zram, &zram->stats.failed_reads);
		return ret;
	}

	return 0;
}

unsigned int _yjdbg_zram = 2;
static int zram_bvec_write(struct zram *zram, struct bio_vec *bvec, u32 index,
			   int offset)
{
	int ret;
	u32 store_offset;
	size_t clen;
	struct zobj_header *zheader;
	struct page *page;
	struct xv_page *page_store;
	unsigned char *user_mem, *cmem, *src, *uncmem = NULL;
	struct page *store_page;

	page = bvec->bv_page;
	src = zram->compress_buffer;

	if (is_partial_io(bvec)) {
		/*
		 * This is a partial IO. We need to read the full page
		 * before to write the changes.
		 */
		uncmem = kmalloc(PAGE_SIZE, GFP_KERNEL);
		if (!uncmem) {
			pr_info("Error allocating temp memory!\n");
			ret = -ENOMEM;
			goto out;
		}
		ret = zram_read_before_write(zram, uncmem, index);
		if (ret) {
			kfree(uncmem);
			goto out;
		}
	}

	user_mem = kmap_atomic(page);

	if (is_partial_io(bvec))
		memcpy(uncmem + offset, user_mem + bvec->bv_offset,
		       bvec->bv_len);
	else
		uncmem = user_mem;

	if (page_zero_filled(uncmem)) {
		kunmap_atomic(user_mem);
		/* Free memory associated with this sector now. */
		zram_free_page(zram, index);

		zram_stat_inc(&zram->stats.pages_zero);
		zram_set_flag(zram, index, ZRAM_ZERO);
		ret = 0;
		goto out;
	}

	/*
	 * zram_slot_free_notify could miss free so that let's
	 * double check.
	 */
	if (unlikely(zram->table[index].page ||
			zram_test_flag(zram, index, ZRAM_ZERO)))
		zram_free_page(zram, index);

	ret = lzo1x_1_compress(uncmem, PAGE_SIZE, src, &clen,
			       zram->compress_workmem);

	kunmap_atomic(user_mem);
	if (is_partial_io(bvec))
			kfree(uncmem);

	if (unlikely(ret != LZO_E_OK)) {
		pr_err("Compression failed! err=%d\n", ret);
		goto out;
	}

	/*
	 * Page is incompressible. Store it as-is (uncompressed)
	 * since we do not want to return too many disk write
	 * errors which has side effect of hanging the system.
	 */
	if (unlikely(clen > max_zpage_size)) {
		clen = PAGE_SIZE;
		page_store = xv_alloc_page(zram->mem_pool, GFP_NOIO | __GFP_HIGHMEM);
		if (unlikely(!page_store)) {
			pr_info("Error allocating memory for "
				"incompressible page: %u\n", index);
			ret = -ENOMEM;
			goto out;
		}

		store_offset = 0;
		zram_set_flag(zram, index, ZRAM_UNCOMPRESSED);
		zram_stat_inc(&zram->stats.pages_expand);
		zram->table[index].page = page_store;
		src = kmap_atomic(page);
		goto memstore;
	}

	if (xv_malloc(zram->mem_pool, clen + sizeof(*zheader),
		      &store_page, &store_offset,
		      GFP_NOIO | __GFP_HIGHMEM)) {
		pr_info("Error allocating memory for compressed "
			"page: %u, size=%zu\n", index, clen);
		ret = -ENOMEM;
		goto out;
	}
	/*
	 * Free memory associated with this sector
	 * before overwriting unused sectors.
	 */
	zram_free_page(zram, index);

	zram->table[index].page = store_page;
memstore:
	zram->table[index].offset = store_offset;

	cmem = xv_map(zram->table[index].page) +
			zram->table[index].offset;

#if 0
	/* Back-reference needed for memory defragmentation */
	if (!zram_test_flag(zram, index, ZRAM_UNCOMPRESSED)) {
		zheader = (struct zobj_header *)cmem;
		zheader->table_idx = index;
		cmem += sizeof(*zheader);
	}
#endif

	memcpy(cmem, src, clen);

	xv_unmap(zram->table[index].page, cmem);
	if (unlikely(zram_test_flag(zram, index, ZRAM_UNCOMPRESSED)))
 		kunmap_atomic(src);

	/* Update stats */
	zram_stat64_add(zram, &zram->stats.compr_size, clen);
	zram_stat_inc(&zram->stats.pages_stored);
	if (clen <= PAGE_SIZE / 2)
		zram_stat_inc(&zram->stats.good_compress);

	return 0;

out:
	if (ret)
		zram_stat64_inc(zram, &zram->stats.failed_writes);
	return ret;
}

static void handle_pending_slot_free(struct zram *zram)
{
	struct zram_slot_free *free_rq;

	spin_lock(&zram->slot_free_lock);
	while (zram->slot_free_rq) {
		free_rq = zram->slot_free_rq;
		zram->slot_free_rq = free_rq->next;
		zram_free_page(zram, free_rq->index);
		kfree(free_rq);
	}
	spin_unlock(&zram->slot_free_lock);
}

static int zram_bvec_rw(struct zram *zram, struct bio_vec *bvec, u32 index,
			int offset, struct bio *bio, int rw)
{
	int ret;

	if (rw == READ) {
		down_read(&zram->lock);
		handle_pending_slot_free(zram);
		ret = zram_bvec_read(zram, bvec, index, offset, bio);
		up_read(&zram->lock);
	} else {
		down_write(&zram->lock);
		handle_pending_slot_free(zram);
		ret = zram_bvec_write(zram, bvec, index, offset);
		up_write(&zram->lock);
	}

	return ret;
}

static void update_position(u32 *index, int *offset, struct bio_vec *bvec)
{
	if (*offset + bvec->bv_len >= PAGE_SIZE)
		(*index)++;
	*offset = (*offset + bvec->bv_len) % PAGE_SIZE;
}

static void __zram_make_request(struct zram *zram, struct bio *bio, int rw)
{
	int i, offset;
	u32 index;
	struct bio_vec *bvec;

	switch (rw) {
	case READ:
		zram_stat64_inc(zram, &zram->stats.num_reads);
		break;
	case WRITE:
		zram_stat64_inc(zram, &zram->stats.num_writes);
		break;
	}

	index = bio->bi_sector >> SECTORS_PER_PAGE_SHIFT;
	offset = (bio->bi_sector & (SECTORS_PER_PAGE - 1)) << SECTOR_SHIFT;

	bio_for_each_segment(bvec, bio, i) {
		int max_transfer_size = PAGE_SIZE - offset;

		if (bvec->bv_len > max_transfer_size) {
			/*
			 * zram_bvec_rw() can only make operation on a single
			 * zram page. Split the bio vector.
			 */
			struct bio_vec bv;

			bv.bv_page = bvec->bv_page;
			bv.bv_len = max_transfer_size;
			bv.bv_offset = bvec->bv_offset;

			if (zram_bvec_rw(zram, &bv, index, offset, bio, rw) < 0)
				goto out;

			bv.bv_len = bvec->bv_len - max_transfer_size;
			bv.bv_offset += max_transfer_size;
			if (zram_bvec_rw(zram, &bv, index+1, 0, bio, rw) < 0)
				goto out;
		} else
			if (zram_bvec_rw(zram, bvec, index, offset, bio, rw)
			    < 0)
				goto out;

		update_position(&index, &offset, bvec);
	}

	set_bit(BIO_UPTODATE, &bio->bi_flags);
	bio_endio(bio, 0);
	return;

out:
	bio_io_error(bio);
}

/*
 * Check if request is within bounds and aligned on zram logical blocks.
 */
static inline int valid_io_request(struct zram *zram, struct bio *bio)
{
	u64 start, end, bound;
	
	/* unaligned request */
	if (unlikely(bio->bi_sector & (ZRAM_SECTOR_PER_LOGICAL_BLOCK - 1)))
		return 0;
	if (unlikely(bio->bi_size & (ZRAM_LOGICAL_BLOCK_SIZE - 1)))
		return 0;
 
	start = bio->bi_sector;
	end = start + (bio->bi_size >> SECTOR_SHIFT);
	bound = zram->disksize >> SECTOR_SHIFT;
	/* out of range range */
	if (unlikely(start >= bound || end > bound || start > end))
		return 0;

	/* I/O request is valid */
	return 1;
}

/*
 * Handler function for all zram I/O requests.
 */
static void zram_make_request(struct request_queue *queue, struct bio *bio)
{
	struct zram *zram = queue->queuedata;

	if (unlikely(!zram->init_done) && zram_init_device(zram))
		goto error;

	down_read(&zram->init_lock);
	if (unlikely(!zram->init_done))
		goto error_unlock;

	if (!valid_io_request(zram, bio)) {
		zram_stat64_inc(zram, &zram->stats.invalid_io);
		goto error_unlock;
	}

	__zram_make_request(zram, bio, bio_data_dir(bio));
	up_read(&zram->init_lock);

	return;

error_unlock:
	up_read(&zram->init_lock);
error:
	bio_io_error(bio);
}

void __zram_reset_device(struct zram *zram)
{
	size_t index;

	zram->init_done = 0;

	/* Free various per-device buffers */
	vfree(zram->compress_workmem);
	free_pages((unsigned long)zram->compress_buffer, 1);

	zram->compress_workmem = NULL;
	zram->compress_buffer = NULL;

	/* Free all pages that are still in this zram device */
	for (index = 0; index < zram->disksize >> PAGE_SHIFT; index++) {
		struct xv_page *page;
		u16 offset;

		page = zram->table[index].page;
		offset = zram->table[index].offset;

		if (!page)
			continue;

		if (unlikely(zram_test_flag(zram, index, ZRAM_UNCOMPRESSED)))
			xv_free_page(zram->mem_pool, page);
		else
			xv_free(zram->mem_pool, page, offset);
	}

	vfree(zram->table);
	zram->table = NULL;

	xv_destroy_pool(zram->mem_pool);
	zram->mem_pool = NULL;

	/* Reset stats */
	memset(&zram->stats, 0, sizeof(zram->stats));

	zram->disksize = 0;
	zram->phyaddr = zram->physize = 0;
}

void zram_reset_device(struct zram *zram)
{
	flush_work(&zram->free_work);

	down_write(&zram->init_lock);
	__zram_reset_device(zram);
	up_write(&zram->init_lock);
}

int zram_init_device(struct zram *zram)
{
	int ret;
	size_t num_pages;

	down_write(&zram->init_lock);

	if (zram->init_done) {
		up_write(&zram->init_lock);
		return 0;
	}

	zram_set_disksize(zram, totalram_pages << PAGE_SHIFT);

	zram->compress_workmem = vzalloc(LZO1X_MEM_COMPRESS);
	if (!zram->compress_workmem) {
		pr_err("Error allocating compressor working memory!\n");
		ret = -ENOMEM;
		goto fail_no_table;
	}

	zram->compress_buffer =
		(void *)__get_free_pages(GFP_KERNEL | __GFP_ZERO, 1);
	if (!zram->compress_buffer) {
		pr_err("Error allocating compressor buffer space\n");
		ret = -ENOMEM;
		goto fail_no_table;
	}

	num_pages = zram->disksize >> PAGE_SHIFT;
	zram->table = vzalloc(num_pages * sizeof(*zram->table));
	if (!zram->table) {
		pr_err("Error allocating zram address table\n");
		ret = -ENOMEM;
		goto fail_no_table;
	}

	set_capacity(zram->disk, zram->disksize >> SECTOR_SHIFT);

	/* zram devices sort of resembles non-rotational disks */
	queue_flag_set_unlocked(QUEUE_FLAG_NONROT, zram->disk->queue);

	if (zram->physize && zram->phyaddr)
		zram->mem_pool = xv_create_phys_pool(zram->phyaddr, zram->physize);
	else
		zram->mem_pool = xv_create_pool();
	if (!zram->mem_pool) {
		pr_err("Error creating memory pool\n");
		ret = -ENOMEM;
		goto fail;
	}

	zram->init_done = 1;
	up_write(&zram->init_lock);

	pr_debug("Initialization done!\n");
	return 0;

fail_no_table:
	/* To prevent accessing table entries during cleanup */
	zram->disksize = 0;
fail:
	__zram_reset_device(zram);
	up_write(&zram->init_lock);
	pr_err("Initialization failed: err=%d\n", ret);
	return ret;
}

static void zram_slot_free(struct work_struct *work)
{
	struct zram *zram;

	zram = container_of(work, struct zram, free_work);
	down_write(&zram->lock);
	handle_pending_slot_free(zram);
	up_write(&zram->lock);
}

static void add_slot_free(struct zram *zram, struct zram_slot_free *free_rq)
{
	spin_lock(&zram->slot_free_lock);
	free_rq->next = zram->slot_free_rq;
	zram->slot_free_rq = free_rq;
	spin_unlock(&zram->slot_free_lock);
}

static void zram_slot_free_notify(struct block_device *bdev,
				unsigned long index)
{
	struct zram *zram;
	struct zram_slot_free *free_rq;

	zram = bdev->bd_disk->private_data;
	zram_stat64_inc(zram, &zram->stats.notify_free);

	free_rq = kmalloc(sizeof(struct zram_slot_free), GFP_ATOMIC);
	if (!free_rq)
		return;

	free_rq->index = index;
	add_slot_free(zram, free_rq);
	schedule_work(&zram->free_work);
}

static const struct block_device_operations zram_devops = {
	.swap_slot_free_notify = zram_slot_free_notify,
	.owner = THIS_MODULE
};

static int create_device(struct zram *zram, int device_id)
{
	int ret = 0;

	init_rwsem(&zram->lock);
	init_rwsem(&zram->init_lock);
	spin_lock_init(&zram->stat64_lock);

	INIT_WORK(&zram->free_work, zram_slot_free);
	spin_lock_init(&zram->slot_free_lock);
	zram->slot_free_rq = NULL;

	zram->queue = blk_alloc_queue(GFP_KERNEL);
	if (!zram->queue) {
		pr_err("Error allocating disk queue for device %d\n",
			device_id);
		ret = -ENOMEM;
		goto out;
	}

	blk_queue_make_request(zram->queue, zram_make_request);
	zram->queue->queuedata = zram;

	 /* gendisk structure */
	zram->disk = alloc_disk(1);
	if (!zram->disk) {
		blk_cleanup_queue(zram->queue);
		pr_warning("Error allocating disk structure for device %d\n",
			device_id);
		ret = -ENOMEM;
		goto out;
	}

	zram->disk->major = zram_major;
	zram->disk->first_minor = device_id;
	zram->disk->fops = &zram_devops;
	zram->disk->queue = zram->queue;
	zram->disk->private_data = zram;
	snprintf(zram->disk->disk_name, 16, "zram%d", device_id);

	/* Actual capacity set using syfs (/sys/block/zram<id>/disksize */
	set_capacity(zram->disk, 0);

	/*
	 * To ensure that we always get PAGE_SIZE aligned
	 * and n*PAGE_SIZED sized I/O requests.
	 */
	blk_queue_physical_block_size(zram->disk->queue, PAGE_SIZE);
	blk_queue_logical_block_size(zram->disk->queue,
					ZRAM_LOGICAL_BLOCK_SIZE);
	blk_queue_io_min(zram->disk->queue, PAGE_SIZE);
	blk_queue_io_opt(zram->disk->queue, PAGE_SIZE);

	add_disk(zram->disk);

	ret = sysfs_create_group(&disk_to_dev(zram->disk)->kobj,
				&zram_disk_attr_group);
	if (ret < 0) {
		pr_warning("Error creating sysfs group");
		goto out;
	}

	zram->init_done = 0;

out:
	return ret;
}

static void destroy_device(struct zram *zram)
{
	sysfs_remove_group(&disk_to_dev(zram->disk)->kobj,
			&zram_disk_attr_group);

	if (zram->disk) {
		del_gendisk(zram->disk);
		put_disk(zram->disk);
	}

	if (zram->queue)
		blk_cleanup_queue(zram->queue);
}

unsigned int zram_get_num_devices(void)
{
	return num_devices;
}

static int zraminfo_proc_show(struct seq_file *m, void *v)
{
	if (zram_devices->init_done)
    {
#define P2K(x) (((unsigned long)x) << (PAGE_SHIFT - 10))
#define B2K(x) (((unsigned long)x) >> (10))
#define u64P2K(x) (((u64)x) << (PAGE_SHIFT - 10))
#define u64B2K(x) (((u64)x) >> (10))
        seq_printf(m,
            "DiskSize:       %llu kB\n"
            "OrigSize:       %lu kB\n"
            "ComprSize:      %llu kB\n"
            "MemUsed:        %llu kB\n"
            "GoodCompr:      %lu kB\n"
            //"BadCompr:       %lu kB\n"
            "ZeroPage:       %lu kB\n"
            "NotifyFree:     %llu kB\n"
            "NumReads:       %llu kB\n"
            "NumWrites:      %llu kB\n"
            "InvalidIO:      %llu kB\n"
            ,
            u64B2K(zram_devices->disksize),
            P2K(zram_devices->stats.pages_stored),
            u64B2K(zram_devices->stats.compr_size),
            //B2K(zs_get_total_size_bytes(zram_devices->meta->mem_pool)), // porting to DTV
	    u64B2K(xv_get_total_size_bytes(zram_devices->mem_pool) + ((u64)(zram_devices->stats.pages_expand) << PAGE_SHIFT)),
            P2K(zram_devices->stats.good_compress),
            //P2K(zram_devices->stats.bad_compress),
            P2K(zram_devices->stats.pages_zero),
            u64P2K(zram_devices->stats.notify_free),
            u64P2K(zram_devices->stats.num_reads),
            u64P2K(zram_devices->stats.num_writes),
            u64P2K(zram_devices->stats.invalid_io)
        	);
#undef P2K
#undef B2K
#undef u64P2K
#undef u64B2K
    }
    return 0;
}

static int zraminfo_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, zraminfo_proc_show, NULL);
}

static const struct file_operations zraminfo_proc_fops = {
	.open		= zraminfo_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int __init zram_init(void)
{
	int ret, dev_id;

	if (num_devices > max_num_devices) {
		pr_warning("Invalid value for num_devices: %u\n",
				num_devices);
		ret = -EINVAL;
		goto out;
	}

	zram_major = register_blkdev(0, "zram");
	if (zram_major <= 0) {
		pr_warning("Unable to get major number\n");
		ret = -EBUSY;
		goto out;
	}

	if (!num_devices) {
		pr_info("num_devices not specified. Using default: 2\n");
		num_devices = 2;
	}

	/* Allocate the device array and initialize each one */
	zram_devices = kzalloc(num_devices * sizeof(struct zram), GFP_KERNEL);
	if (!zram_devices) {
		ret = -ENOMEM;
		goto unregister;
	}

	for (dev_id = 0; dev_id < num_devices; dev_id++) {
		ret = create_device(&zram_devices[dev_id], dev_id);
		if (ret)
			goto free_devices;
	}

	proc_create("zraminfo", 0, NULL, &zraminfo_proc_fops);
	pr_info("Created %u device(s) ...\n", num_devices);

	return 0;

free_devices:
	while (dev_id)
		destroy_device(&zram_devices[--dev_id]);
	kfree(zram_devices);
	zram_devices = NULL;
unregister:
	unregister_blkdev(zram_major, "zram");
out:
	return ret;
}

static void __exit zram_exit(void)
{
	int i;
	struct zram *zram;

	for (i = 0; i < num_devices; i++) {
		zram = &zram_devices[i];

		destroy_device(zram);
		if (zram->init_done)
			zram_reset_device(zram);
	}

	unregister_blkdev(zram_major, "zram");

	kfree(zram_devices);
	zram_devices = NULL;
	pr_debug("Cleanup done!\n");
}

module_param(num_devices, uint, 0);
MODULE_PARM_DESC(num_devices, "Number of zram devices");

module_init(zram_init);
module_exit(zram_exit);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Nitin Gupta <ngupta@vflare.org>");
MODULE_DESCRIPTION("Compressed RAM Block Device");
