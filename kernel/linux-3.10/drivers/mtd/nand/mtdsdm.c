/*
 * Copyright ? 1999-2010 David Woodhouse <dwmw2@infradead.org>
 * Modified by qingqing.li, MediaTek Inc <qingqing.li@mediatek.com>

 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include <linux/device.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/mutex.h>
#include <linux/backing-dev.h>
#include <linux/compat.h>
#include <linux/mount.h>
#include <linux/blkpg.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/mtd/map.h>

#include <asm/uaccess.h>
#include <x_typedef.h>

static struct class *mtd_class;

#define MTD_SDM_MAJOR           239
#define SDM_DEBUG_PRINT         0

extern char *mtdchar_buf;
extern struct semaphore mtdchar_share;

// register mtd-sdm read/write operation callback function for mtk driver.
typedef INT32 (*PFN_SDMOPS)(UINT64 u8Offset, UINT32 u4MemPtr, UINT32 u4MemLen);

typedef struct
{
    UINT32      u4SDMPart1;
    UINT32      u4SDMPart2;
    PFN_SDMOPS  pfnSDMRead;
    PFN_SDMOPS  pfnSDMWrite;
} SDM_STRUCT_T;

static SDM_STRUCT_T _rSdmStruct;

void RegisterSDMOpsCB(UINT32 u4SDMPart1, UINT32 u4SDMPart2, PFN_SDMOPS pfnRead, PFN_SDMOPS pfnWrite)
{
    _rSdmStruct.u4SDMPart1  = u4SDMPart1;
    _rSdmStruct.u4SDMPart2  = u4SDMPart2;
    _rSdmStruct.pfnSDMRead  = pfnRead;
    _rSdmStruct.pfnSDMWrite = pfnWrite;
}
EXPORT_SYMBOL(RegisterSDMOpsCB);

static DEFINE_MUTEX(mtd_mutex);

/*
 * Data structure to hold the pointer to the mtd device as well
 * as mode information ofr various use cases.
 */
struct mtd_file_info {
    struct mtd_info *mtd;
};

static loff_t sdm_lseek (struct file *file, loff_t offset, int orig)
{
	struct mtd_file_info *mfi = file->private_data;
	struct mtd_info *mtd = mfi->mtd;

#if SDM_DEBUG_PRINT
    printk(KERN_CRIT "sdm_lseek: offset = 0x%x, orig = %d\n", (UINT32)offset, orig);
#endif

	switch (orig) {
	case SEEK_SET:
		break;
	case SEEK_CUR:
		offset += file->f_pos;
		break;
	case SEEK_END:
		offset += mtd->size;
		break;
	default:
		return -EINVAL;
	}

	if (offset >= 0 && offset <= mtd->size)
		return file->f_pos = offset;

	return -EINVAL;
}

static int sdm_open(struct inode *inode, struct file *file)
{
	int minor = iminor(inode);
	int ret = 0;
	struct mtd_info *mtd;
	struct mtd_file_info *mfi;
    
#if SDM_DEBUG_PRINT
    printk(KERN_CRIT "sdm_open: major = %d, minor = %d\n", imajor(inode), minor);
#endif
    
    if (!_rSdmStruct.pfnSDMRead || !_rSdmStruct.pfnSDMWrite)
    {
        printk(KERN_ERR "sdm_open fail - mtk driver has not init!\n");
        return -EFAULT;
    }
    
    if(minor < _rSdmStruct.u4SDMPart1 || minor >= _rSdmStruct.u4SDMPart2)
    {
        printk(KERN_ERR "sdm_open fail - not sdm partition, sdm ID:(%d ~ %d), open ID: %d!\n",
            _rSdmStruct.u4SDMPart1, _rSdmStruct.u4SDMPart2, minor);
        return -ENODEV;
    }

	mutex_lock(&mtd_mutex);
    mtd = get_mtd_device(NULL, minor);

	if (IS_ERR(mtd)) {
		ret = PTR_ERR(mtd);
		goto out;
	}

	if (mtd->type == MTD_ABSENT) {
		put_mtd_device(mtd);
		ret = -ENODEV;
		goto out;
	}


	/* You can't open it RW if it's not a writeable device */
	if ((file->f_mode & FMODE_WRITE) && !(mtd->flags & MTD_WRITEABLE)) {
		put_mtd_device(mtd);
		ret = -EACCES;
		goto out;
	}

	mfi = kzalloc(sizeof(*mfi), GFP_KERNEL);
	if (!mfi) {
		put_mtd_device(mtd);
		ret = -ENOMEM;
		goto out;
	}
	mfi->mtd = mtd;
	file->private_data = mfi;

out:
	mutex_unlock(&mtd_mutex);
	return ret;
} /* sdm_open */

/*====================================================================*/

static int sdm_close(struct inode *inode, struct file *file)
{
	struct mtd_file_info *mfi = file->private_data;
	struct mtd_info *mtd = mfi->mtd;
    
#if SDM_DEBUG_PRINT
    printk(KERN_CRIT "sdm_close\n");
#endif

	/* Only sync if opened RW */
	if ((file->f_mode & FMODE_WRITE) && mtd->_sync)
		mtd_sync(mtd);

	put_mtd_device(mtd);
	file->private_data = NULL;
	kfree(mfi);

	return 0;
} /* sdm_close */

static ssize_t sdm_read(struct file *file, char __user *buf, size_t count,loff_t *ppos)
{
	struct mtd_file_info *mfi = file->private_data;
	struct mtd_info *mtd = mfi->mtd;
    char    *kbuf;
    int     len;
    size_t  total_retlen = 0;
    UINT64 u8Offset;
	unsigned long now = jiffies;

#if SDM_DEBUG_PRINT
    printk(KERN_CRIT "sdm_read: offset = 0x%x, count = 0x%x\n", (UINT32)(*ppos), (UINT32)count);
#endif

	if (*ppos + count > mtd->size)
		count = mtd->size - *ppos;

	if (!count)
		return 0;

	if (mtdchar_buf == NULL)
	{
		BUG();
	}

	kbuf = mtdchar_buf;
	if (!kbuf)
		return -ENOMEM;
	
	down(&mtdchar_share);

    mtd->io_sectors[0] += (ALIGN(count, mtd->writesize) >> 9);
	while (count) {

        u8Offset = ((UINT64)mtd->index << 32) | ((UINT32)(*ppos));
        
		if (count > mtd->erasesize)
			len = mtd->erasesize;
		else
			len = count;
		
        if (_rSdmStruct.pfnSDMRead(u8Offset, (UINT32)kbuf, (UINT32)len))
        {
           printk(KERN_ERR "sdm_read operation fail!\n");
           up(&mtdchar_share);
           return -EFAULT;
        }
    
        if (copy_to_user(buf, kbuf, len)) 
        {
           up(&mtdchar_share);
           return -EFAULT;
        }
    
        *ppos += len;
        count -= len;
        buf += len;
        total_retlen += len;
    }

    mtd->io_ticks[0] += (jiffies - now);
    
	up(&mtdchar_share);
	return total_retlen;
} /* sdm_read */

static ssize_t sdm_write(struct file *file, const char __user *buf, size_t count,loff_t *ppos)
{
    struct mtd_file_info *mfi = file->private_data;
    struct mtd_info *mtd = mfi->mtd;
    char    *kbuf;
    UINT64 u8Offset;
    size_t total_retlen = 0;
    int len;
	unsigned long now = jiffies;

#if SDM_DEBUG_PRINT
    printk(KERN_CRIT "sdm_write: offset = 0x%x, count = 0x%x\n", (UINT32)(*ppos), (UINT32)count);
#endif

	if (*ppos == mtd->size)
		return -ENOSPC;

	if (*ppos + count > mtd->size)
		count = mtd->size - *ppos;

	if (!count)
		return 0;

	if (mtdchar_buf == NULL)
	{
		BUG();
	}

	kbuf = mtdchar_buf;
	if (!kbuf)
		return -ENOMEM;

	down(&mtdchar_share);

    mtd->io_sectors[1] += (ALIGN(count, mtd->writesize) >> 9);
	while (count) {
        u8Offset = ((UINT64)mtd->index << 32) | ((UINT32)(*ppos));
        
		if (count > mtd->erasesize)
			len = mtd->erasesize;
		else
			len = count;

		if (copy_from_user(kbuf, buf, len)) {
			up(&mtdchar_share);
			return -EFAULT;
		}

        if (_rSdmStruct.pfnSDMWrite(u8Offset, (UINT32)kbuf, (UINT32)len))
        {
            printk(KERN_ERR "sdm_write operation fail!\n");
            up(&mtdchar_share);
            return -EFAULT;
        }

        *ppos += len;
	    count -= len;
	    buf += len;
	    total_retlen += len;
    }

    mtd->io_ticks[1] += (jiffies - now);
    
	up(&mtdchar_share);
	return total_retlen;
} /* sdm_write */

static int sdm_ioctl(struct file *file, u_int cmd, u_long arg)
{
	struct mtd_file_info *mfi = file->private_data;
	struct mtd_info *mtd = mfi->mtd;
	void __user *argp = (void __user *)arg;
	int ret = 0;
	u_long size;
	struct mtd_info_user info;

#if SDM_DEBUG_PRINT
    printk(KERN_CRIT "sdm_ioctl : cmd = 0x%x\n", cmd);
#endif

	size = (cmd & IOCSIZE_MASK) >> IOCSIZE_SHIFT;
	if (cmd & IOC_IN) {
		if (!access_ok(VERIFY_READ, argp, size))
			return -EFAULT;
	}
	if (cmd & IOC_OUT) {
		if (!access_ok(VERIFY_WRITE, argp, size))
			return -EFAULT;
	}

	switch (cmd) {
	case MEMGETREGIONCOUNT:
		if (copy_to_user(argp, &(mtd->numeraseregions), sizeof(mtd->numeraseregions)))
			return -EFAULT;
		break;

	case MEMGETINFO:
		memset(&info, 0, sizeof(info));
		info.type	= mtd->type;
		info.flags	= mtd->flags;
		info.size	= mtd->size;
		info.erasesize	= mtd->erasesize;
		info.writesize	= mtd->writesize;
		info.oobsize	= mtd->oobsize;
		if (copy_to_user(argp, &info, sizeof(struct mtd_info_user)))
			return -EFAULT;
		break;

    default:
		printk(KERN_CRIT "WARNING ! undefined sdm_ioctl : cmd = 0x%x\n", cmd);
        BUG();
        ret = -ENOTTY;
    }

    return ret;
} /* sdm_ioctl */

static long sdm_unlocked_ioctl(struct file *file, u_int cmd, u_long arg)
{
	int ret;

	mutex_lock(&mtd_mutex);
	ret = sdm_ioctl(file, cmd, arg);
	mutex_unlock(&mtd_mutex);

	return ret;
}

static const struct file_operations sdm_fops = {
	.owner		= THIS_MODULE,
	.llseek		= sdm_lseek,
	.read		= sdm_read,
	.write		= sdm_write,
	.unlocked_ioctl	= sdm_unlocked_ioctl,
	.open		= sdm_open,
	.release	= sdm_close,
};

static void sdm_notify_add(struct mtd_info* mtd)
{
    if (!mtd)
    {
        return;
    }

    device_create(mtd_class, NULL, MKDEV(MTD_SDM_MAJOR, mtd->index), NULL, "mtdsdm%d", mtd->index);
}

static void sdm_notify_remove(struct mtd_info* mtd)
{
    if (!mtd)
    {
        return;
    }

    device_destroy(mtd_class, MKDEV(MTD_SDM_MAJOR, mtd->index));
}

static struct mtd_notifier sdm_notifier = 
{
    .add    = sdm_notify_add,
    .remove = sdm_notify_remove,
};

static int __init init_mtdsdm(void)
{
    if (register_chrdev(MTD_SDM_MAJOR, "mtdsdm", &sdm_fops))
    {
        printk(KERN_NOTICE "Can't allocate major number %d for mtd sdm devices.\n", MTD_SDM_MAJOR);
        return -EAGAIN;
    }

    mtd_class = class_create(THIS_MODULE, "mtdsdm");

    if (IS_ERR(mtd_class))
    {
        printk(KERN_ERR "Error creating mtd class.\n");
        unregister_chrdev(MTD_SDM_MAJOR, "mtdsdm");
        return PTR_ERR(mtd_class);
    }

	register_mtd_user(&sdm_notifier);
    return 0;
}

static void __exit cleanup_mtdsdm(void)
{
	unregister_mtd_user(&sdm_notifier);
    class_destroy(mtd_class);
    unregister_chrdev(MTD_SDM_MAJOR, "mtdsdm");
}

module_init(init_mtdsdm);
module_exit(cleanup_mtdsdm);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("David Woodhouse <dwmw2@infradead.org>");
MODULE_DESCRIPTION("Direct character-device access to MTD devices");
