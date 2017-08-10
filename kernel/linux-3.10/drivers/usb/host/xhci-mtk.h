#ifndef _XHCI_MTK_H
#define _XHCI_MTK_H

#include <linux/usb.h>
#include "xhci.h"
#include <linux/export.h>

extern void __iomem  *pSsusbdDmaIoMap;
extern void __iomem  *pSsusbU2phyIoMap;
extern void __iomem  *pSsusbPhydIoMap;
extern void __iomem  *pSsusbPhyd2IoMap;
extern void __iomem  *pSsusbPhyaIoMap;
extern void __iomem  *pSsusbPhyadaIoMap;
extern void __iomem  *pSsusbPhyspllcdIoMap;
extern void __iomem  *pSsusbPhyfmIoMap;
extern void __iomem  *pSsusbXhciBaseIoMap;
extern void __iomem  *pSsusbMacBaseIoMap;
extern void __iomem  *pSsusbSysBaseIoMap;
extern void __iomem  *pSsusbU2SysBaseIoMap;
extern void __iomem  *pSsusbSifslvIppcIoMap;

#ifdef CC_USB3_5396TOP
#define SSUSB_U3_XHCI_BASE	0xf0040000
#define SSUSB_U3_MAC_BASE	0xf0042400
#define SSUSB_U3_SYS_BASE	0xf0042600
#define SSUSB_U2_SYS_BASE	0xf0043400
#define SIFSLV_IPPC 		0xf0044700
#else
#define SSUSB_U3_XHCI_BASE	pSsusbXhciBaseIoMap
#define SSUSB_U3_MAC_BASE	pSsusbMacBaseIoMap
#define SSUSB_U3_SYS_BASE	pSsusbSysBaseIoMap
#define SSUSB_U2_SYS_BASE	pSsusbU2SysBaseIoMap
#define SIFSLV_IPPC 		pSsusbSifslvIppcIoMap
#define U3_PHY_PG_U2PHY_OFFSET pSsusbU2phyIoMap
#define U3_PHY_PG_PHYD_OFFSET      pSsusbPhydIoMap
#define U3_PHY_PG_PHYD2_OFFSET     pSsusbPhyd2IoMap
#define U3_PHY_PG_PHYA_OFFSET      pSsusbPhyaIoMap
#define U3_PHY_PG_PHYA_DA_OFFSET   pSsusbPhyadaIoMap
#define U3_PHY_PG_SPLLC_OFFSET     pSsusbPhyspllcdIoMap
#define U3_PHY_PG_FM_OFFSET        pSsusbPhyfmIoMap

#endif // CC_USB3_5396TOP

#define U3_PIPE_LATCH_SEL_ADD 		SSUSB_U3_MAC_BASE + 0x130	//0xf0042530
#define U3_PIPE_LATCH_TX	0
#define U3_PIPE_LATCH_RX	0

#define U3_UX_EXIT_LFPS_TIMING_PAR	0xa0
#define U3_REF_CK_PAR	0xb0
#define U3_RX_UX_EXIT_LFPS_REF_OFFSET	8
#define U3_RX_UX_EXIT_LFPS_REF	3
#define	U3_REF_CK_VAL	10

#define U3_TIMING_PULSE_CTRL	0xb4
#define CNT_1US_VALUE			63	//62.5MHz:63, 70MHz:70, 80MHz:80, 100MHz:100, 125MHz:125

#define USB20_TIMING_PARAMETER	0x40
#define TIME_VALUE_1US			63	//62.5MHz:63, 80MHz:80, 100MHz:100, 125MHz:125

#define LINK_PM_TIMER	0x8
#ifdef CONFIG_ARCH_MT5399
#define PM_LC_TIMEOUT_VALUE	3
#else
#define PM_LC_TIMEOUT_VALUE	8
#endif

#define U2_PHYBC12C	0x80
#define CHGDT_EN_Value	1

#define SSUSB_IP_PW_CTRL	(SIFSLV_IPPC+0x0)
#define SSUSB_IP_SW_RST		(1<<0)
#define SSUSB_IP_PW_CTRL_1	(SIFSLV_IPPC+0x4)
#define SSUSB_IP_PDN		(1<<0)
#define SSUSB_U3_CTRL(p)	(SIFSLV_IPPC+0x30+(p*0x08))
#define SSUSB_U3_PORT_DIS	(1<<0)
#define SSUSB_U3_PORT_PDN	(1<<1)
#define SSUSB_U3_PORT_HOST_SEL	(1<<2)
#define SSUSB_U3_PORT_CKBG_EN	(1<<3)
#define SSUSB_U3_PORT_MAC_RST	(1<<4)
#define SSUSB_U3_PORT_PHYD_RST	(1<<5)
#define SSUSB_U2_CTRL(p)	(SIFSLV_IPPC+(0x50)+(p*0x08))
#define SSUSB_U2_PORT_DIS	(1<<0)
#define SSUSB_U2_PORT_PDN	(1<<1)
#define SSUSB_U2_PORT_HOST_SEL	(1<<2)
#define SSUSB_U2_PORT_CKBG_EN	(1<<3)
#define SSUSB_U2_PORT_MAC_RST	(1<<4)
#define SSUSB_U2_PORT_PHYD_RST	(1<<5)
#define SSUSB_IP_CAP			(SIFSLV_IPPC+0x024)
#define SSUSB_IP_PW_STS1        (SIFSLV_IPPC+0x010)
#define SSUSB_SYSPLL_STABLE     (1<<0)
#define SSUSB_XHCI_RST_B_STS    (1<<11)
#define SSUSB_U3_MAC_RST_B_STS  (1<<16)
#define SSUSB_U2_MAC_RST_B_STS  (1<<24)
#define SSUSB_U2_PHY_PLL	(SIFSLV_IPPC+0x7C)

#define LOGCTL_TX_ERR		(1<<0)
#define LOGCTL_STALL		(1<<1)
extern int _mtk_logctl;



#define SSUSB_U3_PORT_NUM(p)	(p & 0xff)
#define SSUSB_U2_PORT_NUM(p)	((p>>8) & 0xff)


#define XHCI_MTK_TEST_MAJOR 234
#define DEVICE_NAME "xhci_mtk_test"

#define CLI_MAGIC 'C'
#define IOCTL_READ _IOR(CLI_MAGIC, 0, int)
#define IOCTL_WRITE _IOW(CLI_MAGIC, 1, int)

void reinitIP(void);
void setInitialReg(void);
void dbg_prb_out(void);
int call_function(char *buf);

long xhci_mtk_test_unlock_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
int xhci_mtk_test_open(struct inode *inode, struct file *file);
int xhci_mtk_test_release(struct inode *inode, struct file *file);
ssize_t xhci_mtk_test_read(struct file *file, char *buf, size_t count, loff_t *ptr);
ssize_t xhci_mtk_test_write(struct file *file, const char *buf, size_t count, loff_t * ppos);


/*
  mediatek probe out
*/
/************************************************************************************/

#define SW_PRB_OUT_ADDR	(SIFSLV_IPPC+0xc0)		//0xf00447c0
#define PRB_MODULE_SEL_ADDR	(SIFSLV_IPPC+0xbc)	//0xf00447bc

static inline void mtk_probe_init(const u32 byte){
	__u32 __iomem *ptr = (__u32 __iomem *) PRB_MODULE_SEL_ADDR;
	writel(byte, ptr);
}

static inline void mtk_probe_out(const u32 value){
	__u32 __iomem *ptr = (__u32 __iomem *) SW_PRB_OUT_ADDR;
	writel(value, ptr);
}

static inline u32 mtk_probe_value(void){
	__u32 __iomem *ptr = (__u32 __iomem *) SW_PRB_OUT_ADDR;

	return readl(ptr);
}


#endif
