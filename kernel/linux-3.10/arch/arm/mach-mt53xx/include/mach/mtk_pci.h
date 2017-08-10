/*
 * linux/arch/arm/mach-mt53xx/include/mach/mtk_pci.h
 *
 * Debugging macro include header
 *
 * Copyright (c) 2010-2012 MediaTek Inc.
 * $Author: dtvbm11 $
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 *
 */

#ifndef __ARCH_MTK_PCI_H
#define __ARCH_MTK_PCI_H

#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/slab.h>
#include <linux/mbus.h>
#include <asm/irq.h>
#include <asm/mach/pci.h>
#include <mach/platform.h>

/*
 * PCIe PHY register.
 */
#define PCIE_PHY_BASE                    (PCIE_VIRT + 0x1500)
    #define PCIE_PHYD08                        (0x08)    /* PHY register. Tx amplitude */
    #define PCIE_LTSSM_OFFSET            (0x68)    /* PHY register. LTSSM register */
    #define PCIE_RESET_CTRL                 (0x64)    /* PHY register. RESET Control register */
    #define PCIE_TX_RX_PHASE_SEL     (0x5C)    /* PHY register. TX/RX phase select Control register */
    #define PCIE_PHYA90                        (0x90)    /* PHY register. CLK SSC on/off */
    #define PCIE_PHYA9C                        (0x9C)    /* PHY register. Tx pre-emphasis */
    #define PCIE_PHYAB4                        (0xB4)    /* PHY register. PLL calibration register */
    
/*
 * PCIe unit register offsets.
 */
#define PLDA_WDMA_PCI_ADDR0       (0x0000)
#define PLDA_WDMA_PCI_ADDR1       (0x0004)
#define PLDA_RDMA_PCI_ADDR0       (0x0020)
#define PLDA_RDMA_PCI_ADDR1       (0x0024)
    #define CONF_REG(r)                        (r & 0xFFC)
    #define CONF_BUS(b)                        ((b & 0xFF) << 24)
    #define CONF_DEV(d)                        ((d & 0x1F) << 19)
    #define CONF_FUNC(f)                       ((f & 0x7) << 16)

#define PLDA_WDMA_AHB_ADDR       (0x0008)
#define PLDA_RDMA_AHB_ADDR       (0x0028)

#define PLDA_WDMA_CONTROL          (0x000C)
#define PLDA_RDMA_CONTROL          (0x002C)
    #define CONF_SIZE                       ((4) << 12)
    #define CONF_BYTE1                    ((1) << 8)
    #define CONF_BYTE2                    ((3) << 8)    
    #define CONF_BYTE3                    ((7) << 8)    
    #define CONF_BYTE4                    ((0xF) << 8)    
    #define CONF_WCMD                     ((5) << 4)
    #define CONF_RCMD                      ((4) << 4)
    #define CONF_STOP                       ((1) << 1)
    #define CONF_START                     ((1) << 0)
    

#define PLDA_IMASK                         (0x0040)
#define PLDA_ISTATUS                     (0x0044)
#define PLDA_ICMD                           (0x0048)
#define PLDA_INFO                           (0x004C)
#define PLDA_IMSISTATUS               (0x0050)
    /* Write DMA */
    #define INT_WDMAEND                   (1 << 0)
    #define INT_WPCIERR                     (1 << 1)
    #define INT_WAHBERR                    (1 << 2)
    /* Read DMA */
    #define INT_RDMAEND                     (1 << 4)
    #define INT_RPCIERR                       (1 << 5)
    #define INT_RAHBERR                      (1 << 6)
    /* PCI-AHB window */
    #define INT_PAPOSTERR                       (1 << 8)
    #define INT_PAFETCHERR                     (1 << 9)
    #define INT_PAWIN0DB                         (1 << 10)
    #define INT_PAWIN1DB                         (1 << 11)   
    /* AHB-PCI window */
    #define INT_APDISCARD                       (1 << 13)
    #define INT_APFETCHERR                     (1 << 14)
    /* Legacy PCI Interrupt */    
    #define INT_INTA                            (1 << 16)
    #define INT_INTB                            (1 << 17)
    #define INT_INTC                            (1 << 18)    
    #define INT_INTD                            (1 << 19)
    /* MISC */        
    #define INT_SYSERR                       (1 << 20)
    #define INT_PMEVENT                     (1 << 21)
    #define INT_AER                              (1 << 22)
    #define INT_MSI                              (1 << 23)
    #define INT_PMSTATE                      (1 << 29)
    /* All error flag */
    #define INT_ALL_ERR                     (INT_WPCIERR |INT_WAHBERR |INT_RPCIERR|INT_RPCIERR| \
                                                              INT_PAPOSTERR|INT_PAFETCHERR|INT_APDISCARD|INT_APFETCHERR |\
                                                              INT_SYSERR)
    
#define PLDA_IMSIADDR                   (0x0054)

#define PLDA_SLOTCAP                      (0x005C)
#define PLDA_DV                                 (0x0060)
#define PLDA_SUB                               (0x0064)
#define PLDA_CREV                             (0x0068)
#define PLDA_SLOTCSR                       (0x006C)
#define PLDA_PRMCSR                        (0x0070)
#define PLDA_DEVCSR                        (0x0074)
#define PLDA_LINKCSR                       (0x0078)
#define PLDA_ROOTCSR                      (0x007C)

#define PLDA_PCIAHB0_BASE            (0x0080)
#define PLDA_PCIAHB1_BASE            (0x0084)

#define PLDA_AHBPCI0_BASE0            (0x0090)
#define PLDA_AHBPCI0_BASE1            (0x0094)
#define PLDA_AHBPCI1_BASE0            (0x0098)
#define PLDA_AHBPCI1_BASE1            (0x009C)

#define PLDA_AHBPCI_TIMER            (0x00A0)

#define PLDA_GEN2_CONF            (0x00A8)
#define PLDA_ASPM_CONF            (0x00AC)
#define PLDA_PM_STATUS            (0x00B0)
#define PLDA_PM_CONF0              (0x00B4)
#define PLDA_PM_CONF1             (0x00B8)
#define PLDA_PM_CONF2             (0x00BC)

// MSI communication address: 32 bits. Just use the msi register address as msi communication address.
// Must be 64 bits alignment, bit 0:2 = 000.
#define PLDA_MSI_ADDR_SETTING  (PCIE_VIRT + 0xC0)

//#define LOG(fmt...)   printk(fmt)
#define LOG(fmt...)   

#define PCI_ASSERT(x) if (!(x)) BUG()

/*
 * PCIe functions.
 */
struct pci_bus;
struct pci_sys_data;

extern int MTK_pci_sys_setup(int nr, struct pci_sys_data *sys);
extern struct pci_bus *MTK_pci_sys_scan(int nr, struct pci_sys_data *sys);
extern int MTK_pci_map_irq(struct pci_dev *dev, u8 slot, u8 pin);
extern void MTK_CreateTestProcFs(void);

#endif
