#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/console.h>
#include <linux/vmalloc.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/kmsg_dump.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

static unsigned long record_size = 4096;
module_param(record_size, ulong, 0400);
MODULE_PARM_DESC(record_size,
		"record size for MTD OOPS pages in bytes (default 4096)");

static char emmcdev[80];
module_param_string(emmcdev, emmcdev, 80, 0400);
MODULE_PARM_DESC(emmcdev,
		"name or index number of the eMMC device to use");

static int dump_oops = 1;
module_param(dump_oops, int, 0600);
MODULE_PARM_DESC(dump_oops,
		"set to 1 to dump oopses, 0 to only dump panics (default 1)");

extern int emmcoops_write(void *ptr, sector_t lba, sector_t blknr);
extern int emmcoops_read(void *ptr, uint64_t offset, uint64_t size);

#define DUMP_BUF_SZ (4096*8)
#define EMMC_BLOCK_SIZE	(512)
#define BYTE_TO_BLOCK(v) (v >> 9)
static void* dump_buf;
/* How much of the console log to snapshot */
static unsigned long kmsg_bytes = DUMP_BUF_SZ;

static void emmcoops_do_dump(struct kmsg_dumper *dumper,
			    enum kmsg_dump_reason reason)
{
	size_t len;
	unsigned long total = 0;
	unsigned long i;
	unsigned char* tmp;
	unsigned int* size;
	static unsigned int once = 0;

#if 0
	if( reason > KMSG_DUMP_EMERG ) // No Dump when reason = KMSG_DUMP_RESTART KMSG_DUMP_HALT KMSG_DUMP_POWEROFF 
		return;
#endif

	if (once)
	    return;
	once = 1;

	pr_debug("(yjdbg) emmcoops_do_dump\n");
	/* Only dump oopses if dump_oops is set */
	if (!dump_oops)
		return;

	pr_debug("(yjdbg) emmcoops_do_dump:0\n");

	tmp = (unsigned char*)dump_buf;
	tmp[0] = 'k';
	tmp[1] = 'm';
	tmp[2] = 's';
	tmp[3] = 'g';
	tmp[4] = 'd';
	tmp[5] = 'u';
	tmp[6] = 'm';
	tmp[7] = 'p';
	total = 16;
	while (total < kmsg_bytes) {
		tmp = (unsigned char*)dump_buf + total;
		if (!kmsg_dump_get_buffer(dumper, true, tmp, DUMP_BUF_SZ, &len))
			break;
		for (i=0; i<len; i++) {
			pr_debug("%c", tmp[i]);
		}
		total += len;
	}
	size = (unsigned int*)((unsigned char*)dump_buf + 8);
	*size = total-16;
	pr_debug("(yjdbg) emmcoops_do_dump:1, cnt: %lu\n", total-16);

	tmp = (unsigned char*)dump_buf+16;
	for (i=0; i<total; i++) {
		pr_debug("%c", tmp[i]);
	}

	// dump to eMMC
	total = (total + EMMC_BLOCK_SIZE - 1) & ~(EMMC_BLOCK_SIZE - 1);
	emmcoops_write(dump_buf, (sector_t)0, (sector_t)BYTE_TO_BLOCK(total));
}

static struct kmsg_dumper emmc_dumper = {
	.dump = emmcoops_do_dump,
};

unsigned long lastkmsg_size = 0;
static int ram_console_show(struct seq_file *m, void *v)
{
	int ret;

	if (lastkmsg_size == 0) {
		unsigned int* size;
#ifdef DEBUG
		int i;
		char* printbuf;
		unsigned char* tmp;
#endif
		ret = emmcoops_read(dump_buf, 0, 16);
		if (ret < 0) {
			printk("emmcoops_read fail - 1\n");
			return -1;
		}
#ifdef DEBUG
		printbuf = (char*)dump_buf;
		for (i=0; i<8; i++)
			printk("%c", printbuf[i]);
#endif
		if (strncmp(dump_buf, "kmsgdump", 8) != 0) {
			printk("signature is wrong\n");
			return -1;
		}
		size = (unsigned int*)((unsigned char*)dump_buf + 8);
		lastkmsg_size = *size;
		pr_debug("emmcoops_read for lastkmsg: %lu\n", lastkmsg_size);

		ret = emmcoops_read(dump_buf, 16, lastkmsg_size);
		if (ret < 0) {
			printk("emmcoops_read fail - 2\n");
			return -1;
		}
#ifdef DEBUG
		tmp = (unsigned char*)dump_buf;
		for (i=0; i<32; i++) {
			printk("%c", tmp[i]);
		}
#endif
	}
#ifdef DEBUG
	ret = seq_write(m, dump_buf, lastkmsg_size);
	if (ret == -1) {
	    printk("ram_console_show seq_write fail\n");
	}
#else
	seq_write(m, dump_buf, lastkmsg_size);
#endif
	return 0;
}

static int ram_console_file_open(struct inode *inode, struct file *file)
{
	return single_open(file, ram_console_show, inode->i_private);
}

static const struct file_operations ram_console_file_ops = {
	.owner = THIS_MODULE,
	.open = ram_console_file_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int __init emmcoops_init(void)
{
	int err;
	struct proc_dir_entry *entry;

	dump_buf = kmalloc(DUMP_BUF_SZ, GFP_KERNEL);
	if (!dump_buf) {
		printk("emmcoops_init alloc buffer fail\n");
		return 1;
	}
	printk("emmcoops_init (0x%p)\n", &emmc_dumper);
	err = kmsg_dump_register(&emmc_dumper);

#if 0
	emmcoops_read(dump_buf, 0, DUMP_BUF_SZ); // NOTE: depend on mknod
#endif
	entry = proc_create("last_kmsg", 0444, NULL, &ram_console_file_ops);
	if (!entry) {
		pr_err("ram_console: failed to create proc entry\n");
		kfree(dump_buf);
		return 1;
	}
	return err;
}

static void __exit emmcoops_exit(void)
{
	if (kmsg_dump_unregister(&emmc_dumper) < 0)
		printk(KERN_WARNING "emmcoops: could not unregister kmsg_dumper\n");
	kfree(dump_buf);
}


module_init(emmcoops_init);
module_exit(emmcoops_exit);

