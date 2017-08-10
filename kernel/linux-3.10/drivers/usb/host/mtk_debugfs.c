
/**
 * debugfs.c - DesignWare USB3 DRD Controller DebugFS file
 *
 */
#include <linux/ptrace.h>
#include <linux/types.h>
#include <linux/spinlock.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/list.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/usb.h>
#include <linux/kthread.h>
#include <asm/byteorder.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/system.h>
#include <asm/unaligned.h>
#include <asm/uaccess.h>
#include <linux/dma-mapping.h>
#include <linux/usb/hcd.h>

#include "mtk_hcd.h"
#include "mtk_debugfs.h"

static MGC_LinuxCd *mtk_saved_LinuxCd;
static atomic_t mtk_testmode_value = ATOMIC_INIT(0);

int mtk_read_testmode(void)
{
    return atomic_read(&mtk_testmode_value);
}

void mtk_set_testmode(int value)
{
    atomic_set(&mtk_testmode_value, value);
}

static void mtk_usb_do_suspend(void *pBase)
{
    uint8_t bReg;

    /* Enter suspend */
    printk("Enter suspend...\n");
    bReg = MGC_Read8(pBase, MGC_O_HDRC_POWER);
    MGC_Write8(pBase, MGC_O_HDRC_POWER, bReg | MGC_M_POWER_SUSPENDM);
}

static void mtk_usb_do_resume(void *pBase)
{
    uint8_t bReg;

    /* Enter resume */
    printk("Enter resume...\n");
    bReg = MGC_Read8(pBase, MGC_O_HDRC_POWER) & ~MGC_M_POWER_SUSPENDM;
    MGC_Write8(pBase, MGC_O_HDRC_POWER, bReg | MGC_M_POWER_RESUME);

    /* CPU should clear the RESUME bit after 10 ms to end Resume signal. 
     * In Host mode, this bit is also automaticlly set when Resume Signalling
     * from the target is detected while the USB2.0 controller is suspended.
     * */
    mdelay(10);
    bReg = MGC_Read8(pBase, MGC_O_HDRC_POWER) & ~MGC_M_POWER_RESUME;
    MGC_Write8(pBase, MGC_O_HDRC_POWER, bReg);
}

static void mtk_usb_do_suspend_resmue(void)
{
    int i;
    void *pBase = mtk_saved_LinuxCd->pRegs;

    for (i = 0; ; i = !i) {
	printk("Sending SOFs for 15 secs...\n");
	mdelay(15000);
	
	i == 0 ? mtk_usb_do_suspend(pBase) : mtk_usb_do_resume(pBase);
    }
}

int mtk_usb_in_testmode(struct usb_hcd *hcd)
{
    int mode;
    if (hcd_to_musbstruct(hcd) != mtk_saved_LinuxCd) {
	return 0;
    }

    mode = mtk_read_testmode();
    if (MUSB_HSET_PORT_TEST_SUSPEND_RESUME == mode) {
	/* Enter suspend resume mode, not return */
	mtk_usb_do_suspend_resmue();
    }
    return mode;
}

static const uint8_t MGC_aTestPacket [] =
{
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
    0xaa, 0xee, 0xee, 0xee, 0xee, 0xee, 0xee, 0xee, 
    0xee, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0x7f, 0xbf, 0xdf, 
    0xef, 0xf7, 0xfb, 0xfd, 0xfc, 0x7e, 0xbf, 0xdf,
    0xef, 0xf7, 0xfb, 0xfd, 0x7e
};

static int mtk_testmode_show(struct seq_file *s, void *unused)
{
	seq_printf(s, "[USB Host Compliance Test Case 1~8].\n");
	seq_printf(s, "**********************************************\n");
	seq_printf(s, "Case 1:    SE0_NAK.\n");
	seq_printf(s, "Case 2:    TEST_J.\n");
	seq_printf(s, "Case 3:    TEST_K.\n");
	seq_printf(s, "Case 4:    TEST PACKET.\n");        
	seq_printf(s, "Case 5:    FORCE ENABLE.\n");        
	seq_printf(s, "Case 6:    SUSPEND, WAIT 15 SECS, RESUME.\n");        
	seq_printf(s, "Case 7:    WAIT 15 SECS, PERFORM SETUP PHASE OF GET_DESC.\n");        
	seq_printf(s, "Case 8:    PERFORM SETUP PHASE OF GET_DESC, WAIT 15 SECS, PERFORM IN DATA PHASE.\n");
	seq_printf(s, "Case 9:    TEST SUSPEND.\n");
	seq_printf(s, "Case 10:   TEST RESUME.\n");
	seq_printf(s, "**********************************************\n");

	return 0;
}

static int mtk_testmode_open(struct inode *inode, struct file *file)
{
	return single_open(file, mtk_testmode_show, inode->i_private);
}

static void mtk_set_port_testmode(void *pBase, MUSB_HsetPortMode mode)
{
	switch(mode) {
	case MUSB_HSET_PORT_TEST_J:
		MGC_Write8(pBase, MGC_O_HDRC_TESTMODE, MGC_M_TEST_J);
		break;
	
	case MUSB_HSET_PORT_TEST_K:
		MGC_Write8(pBase, MGC_O_HDRC_TESTMODE, MGC_M_TEST_K);
		break;
	
	case MUSB_HSET_PORT_TEST_SE0_NAK:
        	MGC_Write8(pBase, MGC_O_HDRC_TESTMODE, MGC_M_TEST_SE0_NAK);
        	break;
	
	case MUSB_HSET_PORT_TEST_PACKET:
        	MGC_SelectEnd(pBase, 0);
        	// 53 bytes.
		MGC_LoadFifo(pBase, 0, sizeof(MGC_aTestPacket), MGC_aTestPacket);
        	MGC_Write8(pBase, MGC_O_HDRC_TESTMODE, MGC_M_TEST_PACKET);
        	MGC_WriteCsr16(pBase, MGC_O_HDRC_CSR0, 0, MGC_M_CSR0_TXPKTRDY);
        	break;

	default:
		break;
	}
}

static ssize_t mtk_testmode_write(struct file *file, 
		const char __user *ubuf, size_t count, loff_t *ppos)
{
	struct seq_file *s = file->private_data;
	MUSB_LinuxController *p = s->private;
	void *pBase = p->pBase;
	int testmode = -1;
	char buf[32];
	s32 value = 0;
	char *endp;

	/* Record the current hcd driver pointer */
	if (p->pThis != mtk_saved_LinuxCd) {
	    mtk_saved_LinuxCd = p->pThis;
	}

	if (copy_from_user(&buf, ubuf, min_t(size_t, sizeof(buf) - 1, count))) {
		return -EFAULT;
	}

	value = simple_strtol(buf, &endp, 16);

	switch (value) {
	case 0x1:
		printk("Case 1: SE0_NAK.\n");

		/* SE0_NAK */
		testmode = MUSB_HSET_PORT_TEST_SE0_NAK;
		break;
	case 0x2:
		printk("Case 2: TEST_J.\n");

		/* J */
		testmode = MUSB_HSET_PORT_TEST_J;
		break;
	case 0x3:
		printk("Case 3: TEST_K.\n");

		/* K */
		testmode = MUSB_HSET_PORT_TEST_K;
		break;
	case 0x4:
		printk("Case 4: TEST_PACKET.\n");

		/* PACKET */
		testmode = MUSB_HSET_PORT_TEST_PACKET;
		break;
	case 0x5:
		printk("Case 5: FORCE_ENABLE, RESERVED.\n");

		/* FPRCE_ENABLE */
		testmode = MUSB_HSET_PORT_TEST_FORCE_ENABLE;
		break;
	case 0x6:
		printk("HS_HOST_PORT_SUSPEND_RESUME.\n");

		/* Suspend, wait 15 secs, resume */
		testmode = MUSB_HSET_PORT_TEST_SUSPEND_RESUME;
		break;
	case 0x7:
		printk("SINGLE_STEP_GET_DEVICE_DESCRIPTOR.\n");

		/* Wait 15 secs, perform setup phase of GET_DESC */
		testmode = MUSB_HSET_PORT_TEST_GET_DESC1;
		break;
	case 0x8:
		printk("SINGLE_STEP_SET_FEATURE.\n");

		/*perform setup phase of GET_DESC, wait 15 secs, perform IN data phase */
		testmode = MUSB_HSET_PORT_TEST_GET_DESC2;
		break;
	case 0x9:
		mtk_usb_do_suspend(pBase);
		break;

	case 0x10:
		mtk_usb_do_resume(pBase);
		break;

	default:
		printk("Unknow value %x for test mode.\n", value);
		break;
	}
	if (-1 != testmode) {
	    mtk_set_testmode(testmode);
	    mtk_set_port_testmode(pBase, testmode);
	}
	
	return count;
}

static const struct file_operations mtk_testmode_fops = {
	.open		= mtk_testmode_open,
	.write		= mtk_testmode_write,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

int mtk_debugfs_init(MUSB_LinuxController *p)
{
	struct dentry *root;
	struct dentry *file;
	struct device *dev;
	int ret = 0;

	dev = p->pThis->dev;
	root = debugfs_create_dir(dev_name(dev), NULL);
	if (!root) {
		ret = -ENOMEM;
		goto err0;
	}
	p->root = root;

	file = debugfs_create_file("testmode", S_IRUGO | S_IWUSR, root, p, &mtk_testmode_fops);
	if (!file) {
		ret = -ENOMEM;
		goto err1;
	}

	return ret;

err1:
	debugfs_remove_recursive(root);
err0:
	return ret;
}

void mtk_debugfs_exit(MUSB_LinuxController *p)
{
	debugfs_remove_recursive(p->root);
	p->root = NULL;
}
