/*. DMA buffer sharing
+---------------------
+
+The dma-buf kernel framework allows DMA buffers to be shared across devices
+and applications. Sharing buffers across display devices and video capture or
+video decoding devices allow zero-copy operation when displaying video content
+produced by a hardware device such as a camera or a hardware codec. This is
+crucial to achieve optimal system performances during video display.
+
+While dma-buf supports both exporting internally allocated memory as a dma-buf
+object (known as the exporter role) and importing a dma-buf object to be used
+as device memory (known as the importer role), the frame buffer API only
+supports the exporter role, as the frame buffer device model doesn't support
+using externally-allocated memory.
+
+The export a frame buffer as a dma-buf file descriptors, applications call the
+FBIOGET_DMABUF ioctl. The ioctl takes a pointer to a fb_dmabuf_export
+structure.
+
+struct fb_dmabuf_export {
+	__u32 fd;
+	__u32 flags;
+};
+
+The flag field specifies the flags to be used when creating the dma-buf file
+descriptor. The only supported flag is O_CLOEXEC. If the call is successful,
+the driver will set the fd field to a file descriptor corresponding to the
+dma-buf object.
+
+Applications can then pass the file descriptors to another application or
+another device driver. The dma-buf object is automatically reference-counted,
+applications can and should close the file descriptor as soon as they don't
+need it anymore. The underlying dma-buf object will not be freed before the
+last device that uses the dma-buf object releases it.
*/


#include <linux/dma-buf.h>
#include <linux/highmem.h>
#include <linux/memblock.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/fb.h>


#ifdef CONFIG_DMA_SHARED_BUFFER
static struct sg_table * mt53_dma_buf_map(struct dma_buf_attachment *attachment, enum dma_data_direction dir) {
        struct sg_table *table;
        struct fb_info *info = attachment->dmabuf->priv;
        struct page *page;
        int err;
		//printk("mt53: mt53_dma_buf_map start\n");

        table = kmalloc(sizeof(struct sg_table), GFP_KERNEL);
        if (!table) return ERR_PTR(-ENOMEM);

        err = sg_alloc_table(table, 1, GFP_KERNEL);
        if (err)
        {
                kfree(table);
                return ERR_PTR(err);
        }

        sg_set_page(table->sgl, NULL, info->fix.smem_len, 0);

        sg_dma_address(table->sgl) = info->fix.smem_start;
        debug_dma_map_sg(NULL, table->sgl, table->nents, table->nents, DMA_BIDIRECTIONAL);
	 //printk("mt53: mt53_dma_buf_map end table=%p\n", table);
        return table;
}

static void mt53_dma_buf_unmap(struct dma_buf_attachment *attachment, struct sg_table *table, enum dma_data_direction dir) {
	//printk("mt53: mt53_dma_buf_unmap start table=%p\n", table);
        debug_dma_unmap_sg(NULL, table->sgl, table->nents, DMA_BIDIRECTIONAL);
        sg_free_table(table);
        kfree(table);
	//printk("mt53: mt53_dma_buf_unmap end \n");
}

static void mt53_dma_buf_release(struct dma_buf *buf) {
        /* Nop */
}

static void *mt53_dma_buf_kmap_atomic(struct dma_buf *buf, unsigned long off) {
        /* Not supported */
        return NULL;
}
static void *mt53_dma_buf_kmap(struct dma_buf *buf, unsigned long off) {
        /* Not supported */
        return NULL;
}


static int mt53_dma_buf_mmap(struct dma_buf *buf, struct vm_area_struct *vma) {
        struct fb_info *info = (struct fb_info*)buf->priv;

        printk(KERN_INFO,"mt53: mmap dma-buf: %p, start: %lu, offset: %lu, size: %lu\n",
               buf, vma->vm_start, vma->vm_pgoff, vma->vm_end - vma->vm_start);

        return io_remap_pfn_range(vma,
                        vma->vm_start + vma->vm_pgoff, (info->fix.smem_start + vma->vm_pgoff) >> PAGE_SHIFT,
                        vma->vm_end - vma->vm_start,
                        pgprot_writecombine(vma->vm_page_prot));
}

static struct dma_buf_ops mt53_dma_buf_ops = {
        .map_dma_buf = mt53_dma_buf_map,
        .unmap_dma_buf = mt53_dma_buf_unmap,
        .release = mt53_dma_buf_release,
        .kmap_atomic = mt53_dma_buf_kmap_atomic,
        .kmap = mt53_dma_buf_kmap,
        .mmap = mt53_dma_buf_mmap,
};

struct dma_buf *mt53_dma_buf_export(struct fb_info *info) {
        struct dma_buf *buf;

        printk(KERN_INFO "mt53: Exporting dma-buf\n");

        buf = dma_buf_export(info, &mt53_dma_buf_ops, info->fix.smem_len, O_RDWR);
		//printk( "mt53: Exporting dma-buf buf=%p\n", buf);

        return buf;
}
#endif

