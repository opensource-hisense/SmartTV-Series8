#include "xhci-mtk.h"
#include "xhci-mtk-power.h"
#include "xhci.h"
#include "mtk-phy.h"
#include "mtk-phy-c60802.h"
#include "xhci-mtk-scheduler.h"
#include <linux/kernel.h>       /* printk() */
#include <linux/slab.h>
#include <linux/delay.h>
#include <asm/uaccess.h>
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
#ifdef CONFIG_64BIT
#include <linux/soc/mediatek/x_typedef.h>
#else
#include <x_typedef.h>
#endif

/***********register address from I/O remap*************/
void __iomem  *pSsusbdDmaIoMap;
void __iomem  *pSsusbU2phyIoMap;
void __iomem  *pSsusbPhydIoMap;
void __iomem  *pSsusbPhyd2IoMap;
void __iomem  *pSsusbPhyaIoMap;
void __iomem  *pSsusbPhyadaIoMap;
void __iomem  *pSsusbPhyspllcdIoMap;
void __iomem  *pSsusbPhyfmIoMap;
void __iomem  *pSsusbXhciBaseIoMap;
void __iomem  *pSsusbMacBaseIoMap;
void __iomem  *pSsusbSysBaseIoMap;
void __iomem  *pSsusbU2SysBaseIoMap;
void __iomem  *pSsusbSifslvIppcIoMap;
void __iomem  *pSsusbXhciIoStartIoMap;

#define SSUSB_CKEGEN pSsusbdDmaIoMap

void setInitialReg(void){
	void __iomem *addr;
	u32 temp;

#if 0 //only for FPGA
	//set MAC reference clock speed
	addr = (int*) (SSUSB_U3_MAC_BASE+U3_UX_EXIT_LFPS_TIMING_PAR);
	temp = readl(addr);
	temp &= ~(0xff << U3_RX_UX_EXIT_LFPS_REF_OFFSET);
	temp |= (U3_RX_UX_EXIT_LFPS_REF << U3_RX_UX_EXIT_LFPS_REF_OFFSET);
	writel(temp, addr);
	addr = (int*) (SSUSB_U3_MAC_BASE+U3_REF_CK_PAR);
	temp = readl(addr);
	temp &= ~(0xff);
	temp |= U3_REF_CK_VAL;
	writel(temp, addr);

	//set SYS_CK
	addr = (int*) (SSUSB_U3_SYS_BASE+U3_TIMING_PULSE_CTRL);
	temp = readl(addr);
	temp &= ~(0xff);
	temp |= CNT_1US_VALUE;
	writel(temp, addr);
	addr = (int*) (SSUSB_U2_SYS_BASE+USB20_TIMING_PARAMETER);
	temp &= ~(0xff);
	temp |= TIME_VALUE_1US;
	writel(temp, addr);

	//set LINK_PM_TIMER=3
	addr = (int*) (SSUSB_U3_SYS_BASE+LINK_PM_TIMER);
	temp = readl(addr);
	temp &= ~(0xf);
	temp |= PM_LC_TIMEOUT_VALUE;
	writel(temp, addr);
#endif

#ifdef CONFIG_ARCH_MT5399
    //set CHGDT_EN=0x1
    addr = (int*) (SSUSB_U2_PHY_COM+U2_PHYBC12C);
    temp = readl(addr);
    temp &= ~(0x1);
    temp |= CHGDT_EN_Value;
    writel(temp, addr);

    //set BC12_EN=0x1
    addr = (int*) (SSUSB_U2_SYS_BASE+4);
    temp = readl(addr);
    temp |= (0x1<<12);
    writel(temp, addr);
#else
    //set SSUSB_U2_PORT_LPM_PLL_STABLE_TIMER=25
    addr = (int*) (SSUSB_U2_PHY_PLL);
    temp = readl(addr);
    temp &= ~(0xff0000);
    temp |= (0x19<<16);
    writel(temp, addr);
#endif

    //set noise_still_sof=0x1 for special webcam
    addr =  (SSUSB_U2_SYS_BASE+0x20);
    temp = readl(addr);
    temp |= (0x1<<21);
    writel(temp, addr);
}


void setLatchSel(void){
	void __iomem *latch_sel_addr;
	u32 latch_sel_value;
	latch_sel_addr = U3_PIPE_LATCH_SEL_ADD;
	latch_sel_value = ((U3_PIPE_LATCH_TX)<<2) | (U3_PIPE_LATCH_RX);
	writel(latch_sel_value, latch_sel_addr);
}

void reinitIP(void){
//	__u32 __iomem *ip_reset_addr;
//	u32 ip_reset_value;
#ifdef CONFIG_ARCH_MT5399
	char temp;
	unsigned long regval;
#endif


	u3phy_init();
#ifdef CONFIG_ARCH_MT5399
	regval = readl(0xF000D064);// Software ID
	if(((regval & 0xffff0000)==0x53990000) &&
	    (regval > 0x53990001)){
		temp = 0xF0;
		U3PhyWriteReg8((((PHY_UINT32)&u3phy->u3phya_regs_c->reg7)+2),temp);
	}
#endif

	enableAllClockPower();

	setLatchSel();
	mtk_xhci_scheduler_init();
#ifdef PERF_PROBE
	mtk_probe_init(0x38383838);
	mtk_probe_out(0x0);
#endif
#ifdef WEB_CAM_PROBE
	mtk_probe_init(0x70707070);
	mtk_probe_out(0x0);
#endif
}

void dbg_prb_out(void){
	mtk_probe_init(0x0f0f0f0f);
	mtk_probe_out(0xffffffff);
	mtk_probe_out(0x01010101);
	mtk_probe_out(0x02020202);
	mtk_probe_out(0x04040404);
	mtk_probe_out(0x08080808);
	mtk_probe_out(0x10101010);
	mtk_probe_out(0x20202020);
	mtk_probe_out(0x40404040);
	mtk_probe_out(0x80808080);
	mtk_probe_out(0x55555555);
	mtk_probe_out(0xaaaaaaaa);
}



///////////////////////////////////////////////////////////////////////////////

#define RET_SUCCESS 0
#define RET_FAIL 1


static int dbg_u3w(int argc, char**argv)
{
	int u4TimingValue;
	char u1TimingValue;
	int u4TimingAddress;
	
	if (argc<3)
    {
        printk(KERN_ERR "Arg: address value\n");
        return RET_FAIL;
    }
	u3phy_init();

	u4TimingAddress = (int)simple_strtol(argv[1], &argv[1], 16);
	u4TimingValue = (int)simple_strtol(argv[2], &argv[2], 16);
	u1TimingValue = u4TimingValue & 0xff;
	_U3Write_Reg(u4TimingAddress, u1TimingValue);
	printk(KERN_ERR "Write done\n");
	return RET_SUCCESS;

}

static int dbg_u3r(int argc, char**argv)
{
	char u1ReadTimingValue;
	int u4TimingAddress;
	if (argc<2)
    {
        printk(KERN_ERR "Arg: address\n");
        return 0;
    }
	u3phy_init();
	mdelay(500);
	u4TimingAddress = (int)simple_strtol(argv[1], &argv[1], 16);
	u1ReadTimingValue = _U3Read_Reg(u4TimingAddress);
	printk(KERN_ERR "Value = 0x%x\n", u1ReadTimingValue);
    return 0;
}

int dbg_u3init(int argc, char**argv)
{
//	int ret;
//	char temp;

	unsigned long regval;

#ifdef CONFIG_ARCH_MT5399
//DMA configuration
    regval = readl(0xF000D5AC);
    regval |= 0x00000007;    
	writel(regval, 0xF000D5AC);

    regval = readl(0xF000D5A4);
    regval |= (0x24);
	writel(regval, 0xF000D5A4);	
//High Speed EOF Start Offset
	regval = 0x000201F3;
	writel(regval, 0xF0074938);	
 	regval = readl(0xF0074960);
	regval &= ~(0x30000);
	writel(regval, 0xF0074960);	
#else
    //DMA configuration
    regval = readl(SSUSB_CKEGEN);
    regval |= 0x7;    
    writel(regval, SSUSB_CKEGEN);
#endif
	
//	ret = u3phy_init();
	//printk(KERN_ERR "phy registers and operations initial done\n");
	if(u3phy_ops->u2_slew_rate_calibration){
		u3phy_ops->u2_slew_rate_calibration(u3phy);
	}
	else{
		printk(KERN_ERR "WARN: PHY doesn't implement u2 slew rate calibration function\n");
	}
	if(u3phy_ops->init(u3phy) == PHY_TRUE)
		return RET_SUCCESS;
	return RET_FAIL;
}

void dbg_setU1U2(int argc, char**argv){
//	struct xhci_hcd *xhci;
	int u1_value;
	int u2_value;
	u32 temp;
	u32 __iomem *addr;
	
	if (argc<3)
    {
        printk(KERN_ERR "Arg: u1value u2value\n");
        return;
    }

	u1_value = (int)simple_strtol(argv[1], &argv[1], 10);
	u2_value = (int)simple_strtol(argv[2], &argv[2], 10);
	addr = 	(int*)(SSUSB_U3_XHCI_BASE+0x424);		//0xf0040424
	temp = readl(addr);
	temp = temp & (~(0x0000ffff));
	temp = temp | u1_value | (u2_value<<8);
	writel(temp, addr);
}
///////////////////////////////////////////////////////////////////////////////
extern struct xhci_hcd *_mtk_xhci;
void ssusb_portoff(void){

	int i;
	u32 port_id, temp;
	u32 __iomem *addr;
    int num_u3_port;
    int num_u2_port;

	num_u3_port = SSUSB_U3_PORT_NUM(readl((UPTR*)SSUSB_IP_CAP));
	num_u2_port = SSUSB_U2_PORT_NUM(readl((UPTR*)SSUSB_IP_CAP));
    SSUSB_Vbushandler(VBUS_OFF);
    msleep(300);
	
	for(i=1; i<=num_u3_port; i++){
		port_id=i;
		addr = &_mtk_xhci->op_regs->port_status_base + NUM_PORT_REGS*((port_id-1) & 0xff);
        printk("SSUSB: U3 port status add = 0x%p\n", addr); 
		temp = xhci_readl(_mtk_xhci, addr);
		temp = xhci_port_state_to_neutral(temp);
		temp &= ~PORT_POWER;
		xhci_writel(_mtk_xhci, temp, addr);
	}
	for(i=1; i<=num_u2_port; i++){
		port_id=i+num_u3_port;
		addr = &_mtk_xhci->op_regs->port_status_base + NUM_PORT_REGS*((port_id-1) & 0xff);
        printk("SSUSB: U2 port status add = 0x%p\n", addr); 
		temp = xhci_readl(_mtk_xhci, addr);
		temp = xhci_port_state_to_neutral(temp);
		temp &= ~PORT_POWER;
		xhci_writel(_mtk_xhci, temp, addr);
	}

}

///////////////////////////////////////////////////////////////////////////////
void ssusb_portdisable(void){

	int i;
	u32 port_id, temp;
	u32 __iomem *addr;
    int num_u3_port;
    int num_u2_port;

	num_u3_port = SSUSB_U3_PORT_NUM(readl((UPTR*)SSUSB_IP_CAP));
	num_u2_port = SSUSB_U2_PORT_NUM(readl((UPTR*)SSUSB_IP_CAP));
	
	for(i=1; i<=num_u3_port; i++){ //disable u3 port, enable u2 port power
		port_id=i;
		addr = &_mtk_xhci->op_regs->port_status_base + NUM_PORT_REGS*((port_id-1) & 0xff);
		temp = xhci_readl(_mtk_xhci, addr);
		temp = xhci_port_state_to_neutral(temp);
		temp &= ~PORT_POWER;
		xhci_writel(_mtk_xhci, temp, addr);
	}
	for(i=1; i<=num_u2_port; i++){
		port_id=i+num_u3_port;
		addr = &_mtk_xhci->op_regs->port_status_base + NUM_PORT_REGS*((port_id-1) & 0xff);
		temp = xhci_readl(_mtk_xhci, addr);
		temp = xhci_port_state_to_neutral(temp);
		temp |= PORT_POWER;
		xhci_writel(_mtk_xhci, temp, addr);
	}

    msleep(300);
    SSUSB_Vbushandler(VBUS_ON);
}

///////////////////////////////////////////////////////////////////////////////
void ssusb_portup(void){

	int i;
	u32 port_id, temp;
	u32 __iomem *addr;
    int num_u3_port;
    int num_u2_port;

	num_u3_port = SSUSB_U3_PORT_NUM(readl((UPTR*)SSUSB_IP_CAP));
	num_u2_port = SSUSB_U2_PORT_NUM(readl((UPTR*)SSUSB_IP_CAP));
//    SSUSB_Vbushandler(VBUS_OFF);
//    msleep(300);
	
	for(i=1; i<=num_u3_port; i++){ //enable u3 port power, enable u2 port power
		port_id=i;
		addr = &_mtk_xhci->op_regs->port_status_base + NUM_PORT_REGS*((port_id-1) & 0xff);
		temp = xhci_readl(_mtk_xhci, addr);
		temp = xhci_port_state_to_neutral(temp);
		temp |= PORT_POWER;
		xhci_writel(_mtk_xhci, temp, addr);
	}
	for(i=1; i<=num_u2_port; i++){
		port_id=i+num_u3_port;
		addr = &_mtk_xhci->op_regs->port_status_base + NUM_PORT_REGS*((port_id-1) & 0xff);
		temp = xhci_readl(_mtk_xhci, addr);
		temp = xhci_port_state_to_neutral(temp);
		temp |= PORT_POWER;
		xhci_writel(_mtk_xhci, temp, addr);
	}

    msleep(300);
    SSUSB_Vbushandler(VBUS_ON);
}
///////////////////////////////////////////////////////////////////////////////

int call_function(char *buf)
{
//	int i;
	int argc;
	char *argv[80];

	argc = 0;
	do
	{
		argv[argc] = strsep(&buf, " ");
		printk(KERN_DEBUG "[%d] %s\r\n", argc, argv[argc]);
		argc++;
	} while (buf);
	if (!strcmp("dbg.r", argv[0]))
		dbg_prb_out();
	else if (!strcmp("dbg.u3w", argv[0]))
		dbg_u3w(argc, argv);
	else if (!strcmp("dbg.u3r", argv[0]))
		dbg_u3r(argc, argv);
	else if (!strcmp("dbg.u3i", argv[0]))
		dbg_u3init(argc, argv);
	else if (!strcmp("pw.u1u2", argv[0]))
		dbg_setU1U2(argc, argv);
	else if (!strcmp("hal.vbuson", argv[0]))
		SSUSB_Vbushandler(VBUS_ON);
	else if (!strcmp("hal.vbusoff", argv[0]))
		SSUSB_Vbushandler(VBUS_OFF);
	else if (!strcmp("hal.portoff", argv[0]))
		ssusb_portoff();
	else if (!strcmp("hal.portdisable", argv[0]))
		ssusb_portdisable();
	else if (!strcmp("hal.portup", argv[0]))
		ssusb_portup();
	return 0;
}

char w_buf[200];
char r_buf[200] = "this is a test";

long xhci_mtk_test_unlock_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{

    long len = 200;

	switch (cmd) {
		case IOCTL_READ:
			if(copy_to_user((char *) arg, r_buf, len))
                return -EFAULT;
			printk(KERN_DEBUG "IOCTL_READ: %s\r\n", r_buf);
			break;
		case IOCTL_WRITE:
			if (copy_from_user(w_buf, (char *) arg, len))
                return -EFAULT;
			printk(KERN_DEBUG "IOCTL_WRITE: %s\r\n", w_buf);

			//invoke function
			return call_function(w_buf);
			break;
		default:
			return -ENOTTY;
	}

	return len;
}

int xhci_mtk_test_open(struct inode *inode, struct file *file)
{

    printk(KERN_DEBUG "xhci_mtk_test open: successful\n");
    return 0;
}

int xhci_mtk_test_release(struct inode *inode, struct file *file)
{

    printk(KERN_DEBUG "xhci_mtk_test release: successful\n");
    return 0;
}

ssize_t xhci_mtk_test_read(struct file *file, char *buf, size_t count, loff_t *ptr)
{

    printk(KERN_DEBUG "xhci_mtk_test read: returning zero bytes\n");
    return 0;
}

ssize_t xhci_mtk_test_write(struct file *file, const char *buf, size_t count, loff_t * ppos)
{

    printk(KERN_DEBUG "xhci_mtk_test write: accepting zero bytes\n");
    return 0;
}



