#include <linux/init.h>
#include <linux/irq.h>
#include <linux/log2.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/slab.h>

#include <linux/kernel.h>       /* printk() */
#include <linux/fs.h>           /* everything... */
#include <linux/errno.h>        /* error codes */
#include <linux/types.h>        /* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h>        /* O_ACCMODE */
#include <linux/seq_file.h>
#include <linux/cdev.h>
//#include <linux/pci.h>
#include <linux/string.h>
#include <linux/random.h>
#include <linux/scatterlist.h>
#include <asm/unaligned.h>
#include <linux/usb/ch9.h>
#include <asm/uaccess.h>
#include "mu3d_test_test.h"
#include "mu3d_test_unified.h"



struct file_operations xhci_mtk_test_fops;


////////////////////////////////////////////////////////////////////////////

#define CLI_MAGIC 'C'
#define IOCTL_READ _IOR(CLI_MAGIC, 0, int)
#define IOCTL_WRITE _IOW(CLI_MAGIC, 1, int)

#define BUF_SIZE 200
#define MAX_ARG_SIZE 20

////////////////////////////////////////////////////////////////////////////

typedef struct
{
	char name[256];
	int (*cb_func)(int argc, char** argv);
} CMD_TBL_T;

CMD_TBL_T _arPCmdTbl[] =
{
	{"auto.dev", &TS_AUTO_TEST},
	{"auto.u3i", &u3init},
	{"auto.u3w", &u3w},
	{"auto.u3r", &u3r},
	{"auto.u3d", &U3D_Phy_Cfg_Cmd},
	{"auto.link", &u3d_linkup},
//	{"auto.eyeinit", &dbg_phy_eyeinit},	
//	{"auto.eyescan", &dbg_phy_eyescan},
    {"", &u3init},
//	{NULL, NULL}
};

////////////////////////////////////////////////////////////////////////////

char wr_buf[BUF_SIZE];
char rd_buf[BUF_SIZE] = "this is a test";

////////////////////////////////////////////////////////////////////////////

int call_function(char *buf)
{
	int i;
	int argc;
	char *argv[MAX_ARG_SIZE];

	argc = 0;
	do
	{
		argv[argc] = strsep(&buf, " ");
		printk(KERN_DEBUG "[%d] %s\r\n", argc, argv[argc]);
		argc++;
	} while (buf);

	for (i = 0; i < sizeof(_arPCmdTbl)/sizeof(CMD_TBL_T); i++)
	{
		if ((!strcmp(_arPCmdTbl[i].name, argv[0])) && (_arPCmdTbl[i].cb_func != NULL))
			return _arPCmdTbl[i].cb_func(argc, argv);
	}

	return -1;
}


static int xhci_mtk_test_open(struct inode *inode, struct file *file)
{

    printk(KERN_DEBUG "xhci_mtk_test open: successful\n");
    return 0;
}

static int xhci_mtk_test_release(struct inode *inode, struct file *file)
{

    printk(KERN_DEBUG "xhci_mtk_test release: successful\n");
    return 0;
}

static ssize_t xhci_mtk_test_read(struct file *file, char *buf, size_t count, loff_t *ptr)
{

    printk(KERN_DEBUG "xhci_mtk_test read: returning zero bytes\n");
    return 0;
}

static ssize_t xhci_mtk_test_write(struct file *file, const char *buf, size_t count, loff_t * ppos)
{

    printk(KERN_DEBUG "xhci_mtk_test write: accepting zero bytes\n");
    return 0;
}

static long xhci_mtk_test_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{

    long len = BUF_SIZE;

	switch (cmd) {
		case IOCTL_READ:
			if(copy_to_user((char *) arg, rd_buf, len))
                return -EFAULT;
			printk(KERN_DEBUG "IOCTL_READ: %s\r\n", rd_buf);
			break;
		case IOCTL_WRITE:
			if(copy_from_user(wr_buf, (char *) arg, len))
                return -EFAULT;
			printk(KERN_DEBUG "IOCTL_WRITE: %s\r\n", wr_buf);

			//invoke function
			return call_function(wr_buf);
			break;
		default:
			return -ENOTTY;
	}

	return len;
}

struct file_operations xhci_mtk_test_fops = {
    owner:   THIS_MODULE,
    read:    xhci_mtk_test_read,
    write:   xhci_mtk_test_write,
    unlocked_ioctl:   xhci_mtk_test_ioctl,
    open:    xhci_mtk_test_open,
    release: xhci_mtk_test_release,
};


static int __init mtk_test_init(void)
{
	int retval = 0;
	printk(KERN_DEBUG "xchi_mtk_test Init\n");
	retval = register_chrdev(XHCI_MTK_TEST_MAJOR, DEVICE_NAME, &xhci_mtk_test_fops);
	if(retval < 0)
	{
		printk(KERN_DEBUG "xchi_mtk_test Init failed, %d\n", retval);
		goto fail;
	}
	
	return 0;
	fail:
		return retval;
}
module_init(mtk_test_init);

static void __exit mtk_test_cleanup(void)
{
	printk(KERN_DEBUG "xchi_mtk_test End\n");
	unregister_chrdev(XHCI_MTK_TEST_MAJOR, DEVICE_NAME);
	
}
module_exit(mtk_test_cleanup);

MODULE_LICENSE("GPL");
