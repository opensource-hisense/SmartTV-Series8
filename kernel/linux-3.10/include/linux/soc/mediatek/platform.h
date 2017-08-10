/*
 * linux/arch/arm/mach-mt53xx/include/mach/platform.h
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

#ifndef __ASM_ARCH_PLATFORM_H
#define __ASM_ARCH_PLATFORM_H

#include "irqs.h"


/*
 * I/O mapping
 */
//#if !defined(CONFIG_OF)
#define IO_PHYS             0xF0000000
#define IO_OFFSET           0x00000000

#ifndef IO_VIRT
#ifdef CONFIG_64BIT
#define IO_VIRT             0xFFFFFF80F0000000ULL    //(IO_PHYS + IO_OFFSET)
#else
#define IO_VIRT             0xF0000000    //(IO_PHYS + IO_OFFSET)
#endif
#endif  // IO_VIRT
//#endif

#ifndef IO_PHY
#define IO_PHY             0xF0000000    //(IO_PHY + IO_OFFSET)
#endif  // IO_PHY

#define IO_SIZE             0x00200000    // 2MB, more then necessary to make kernel map using 1MB page.


/*---------------------------------------------*/

/*
 *  Mt5395 HAL
 */

/* Constant definitions */

#define MEMORY_ALIGNMENT            8

/* IO register definitions */

#define PARSER0_VIRT                (IO_VIRT + 0x00000)
#define VLD0_VIRT                   (IO_VIRT + 0x02000)
#define GRAPH_VIRT                  (IO_VIRT + 0x04000)
#define AUDIO_VIRT                  (IO_VIRT + 0x05000)
#define DRAM_VIRT                   (IO_VIRT + 0x07000)
#define BIM_VIRT                    (IO_VIRT + 0x08000)
#define I1394_VIRT                  (IO_VIRT + 0x09000)
#define MC0_VIRT                    (IO_VIRT + 0x0a000)
#define RS232_VIRT                  (IO_VIRT + 0x0c000)
#define CKGEN_VIRT                  (IO_VIRT + 0x0d000)
#define ATA_VIRT                    (IO_VIRT + 0x0f000)
#define TCMGPR0_VIRT                (IO_VIRT + 0x10000)
#define TCMGPR1_VIRT                (IO_VIRT + 0x11000)
#define FLASH_CARD_VIRT             (IO_VIRT + 0x12000)
#define SMART_CARD_VIRT             (IO_VIRT + 0x13000)
#define PCMCIA_VIRT                 (IO_VIRT + 0x16000)
#define AES_VIRT                    (IO_VIRT + 0x16000)
#define SHA1_VIRT                   (IO_VIRT + 0x16400)
#define DFAST_VIRT                  (IO_VIRT + 0x16800)
#define DEMUX0_VIRT                 (IO_VIRT + 0x17000)
#define DEMUX1_VIRT                 (IO_VIRT + 0x18000)
#define DEMUX2_VIRT                 (IO_VIRT + 0x19000)
#define DEMUX3_VIRT                 (IO_VIRT + 0x1a000)
#define DEMUX4_VIRT                 (IO_VIRT + 0x1b000)
#define DEMUX5_VIRT                 (IO_VIRT + 0x1c000)
#define DEMUX6_VIRT                 (IO_VIRT + 0x1d000)
#define PIDSWAP_VIRT                (IO_VIRT + 0x1e000)
#define IDETESTPORT_VIRT            (IO_VIRT + 0x1f000)
#define GFXSCALER_VIRT              (IO_VIRT + 0x20000)
#define OSD_VIRT                    (IO_VIRT + 0x21000)
#define VDOIN0_VIRT                 (IO_VIRT + 0x22000)
#define VDOIN1_VIRT                 (IO_VIRT + 0x23000)
#define PSCAN_VIRT                  (IO_VIRT + 0x24000)
#define SCPOS_VIRT                  (IO_VIRT + 0x25000)
#define BLK2RS_VIRT                 (IO_VIRT + 0x26000)
#define SDIO_VIRT                   (IO_VIRT + 0x27000)
#define PDWNC_VIRT                  (IO_VIRT + 0x28000)
#define HPDS_VIRT                   (IO_VIRT + 0x29000)
#define ETHER_VIRT                   (IO_VIRT + 0x32000)
#define PCIE_VIRT                      (IO_VIRT + 0x64000)
#define IOMMU_PHYS                  (IO_PHY + 0x68000)
#define DRAM_PHY_VIRT               (IO_VIRT + 0x58000)
#define AES_PHYS                  (IO_PHY + 0x16000)
#define MUSB_CAL                    (IO_PHY + 0x59F00)
#define MUSB_DMA_SET                (IO_PHY + 0x61A00)
#define MUSB_MAC_BASE               (IO_PHY + 0x50000)
#define MUSB_PHY_BASE               (IO_PHY + 0x59800)
#define PSSUSB_DMA_SET              (IO_PHY + 0xD5AC)
#define PSSUSB_PHY_PG_SPLLC         (IO_PHY + 0x70000)
#define PSSUSB_PHY_PG_FM            (IO_PHY + 0x70100)
#define PSSUSB_SIFSLV_IPPC          (IO_PHY + 0x70700)
#define PSSUSB_PHY_PG_U2PHY         (IO_PHY + 0x70800)
#define PSSUSB_PHY_PG_PHYD          (IO_PHY + 0x70900)
#define PSSUSB_PHY_PG_PHYD2         (IO_PHY + 0x70A00)
#define PSSUSB_PHY_PG_PHYA          (IO_PHY + 0x70B00)
#define PSSUSB_PHY_PG_PHYA_DA       (IO_PHY + 0x70C00)
#define PSSUSB_XHCI_BASE            (IO_PHY + 0x74000)
#define PSSUSB_MAC_BASE             (IO_PHY + 0x76400)
#define PSSUSB_SYS_BASE             (IO_PHY + 0x76600)
#define PSSUSB_U2_SYS_BASE          (IO_PHY + 0x77400)
#define PXHC_IO_START               (IO_PHY + 0x74000)
#define MCU_PHY                     (IO_PHY + 0x78000)
#define DRAM_PHY                    (IO_PHY + 0x07000)
#define BIM_PHY                     (IO_PHY + 0x08000)
#define NAND_PHY                    (IO_PHY + 0x29000)
#define EFUSE_PHY                   (IO_PHY + 0x08600)
#define CKGEN_PHY                   (IO_PHY + 0x0d000)
#define RS232_PHY                   (IO_PHY + 0x0c000)
#define PDWNC_PHY                   (IO_PHY + 0x28000)

#ifdef CONFIG_CACHE_L2X0
#define L2C_BASE                (IO_VIRT + 0x2000000)
#endif

#ifdef CONFIG_CPU_V7
#if defined(CONFIG_ARCH_MT5890)||defined(CONFIG_ARCH_MT5891)
#define IO_CPU_VIRT				(IO_VIRT + 0x2010000)
#define MPCORE_GIC_DIST_VIRT    (IO_CPU_VIRT + 0x1000)
#define MPCORE_GIC_CPU_VIRT 	(IO_CPU_VIRT + 0x2000)


#define MPCORE_SCU_VIRT     (IO_VIRT + 0x2002000 + 0x0000)
#define MPCORE_GIT_VIRT     (IO_VIRT + 0x2002000 + 0x0600)

#elif defined(CONFIG_ARCH_MT5882)||defined(CONFIG_ARCH_MT5883)
#define IO_CPU_VIRT				(IO_VIRT + 0x1000000)
#define MPCORE_GIC_DIST_VIRT    (IO_CPU_VIRT + 0x1000)
#define MPCORE_GIC_CPU_VIRT 	(IO_CPU_VIRT + 0x2000)

#define MPCORE_SCU_VIRT     (IO_VIRT + 0x2002000 + 0x0000)
#define MPCORE_GIT_VIRT     (IO_VIRT + 0x2002000 + 0x0600)

#else
#define	IO_CPU_VIRT		(IO_VIRT + 0x2002000)
#define	MPCORE_SCU_VIRT		(IO_CPU_VIRT + 0x0000)
#define	MPCORE_GIC_CPU_VIRT	(IO_CPU_VIRT + 0x0100)
#define	MPCORE_GIT_VIRT		(IO_CPU_VIRT + 0x0600)
#define	MPCORE_GIC_DIST_VIRT	(IO_CPU_VIRT + 0x1000)
#endif
#endif

/*
 * MT53xx BIM
 */

/* BIM register and bitmap defintions */

//----------------------------------------------------------------------------
// Timer registers
#define REG_RW_TIMER0_LLMT  0x0060      // RISC Timer 0 Low Limit Register
#define REG_T0LMT           0x0060      // RISC Timer 0 Low Limit Register
#define REG_RW_TIMER0_LOW   0x0064      // RISC Timer 0 Low Register
#define REG_T0              0x0064      // RISC Timer 0 Low Register
#define REG_RW_TIMER0_HLMT  0x0180      // RISC Timer 0 High Limit Register
#define REG_RW_TIMER0_HIGH  0x0184      // RISC Timer 0 High Register

#define REG_RW_TIMER1_LLMT  0x0068      // RISC Timer 1 Low Limit Register
#define REG_T1LMT           0x0068      // RISC Timer 1 Low Limit Register
#define REG_RW_TIMER1_LOW   0x006c      // RISC Timer 1 Low Register
#define REG_T1              0x006c      // RISC Timer 1 Low Register
#define REG_RW_TIMER1_HLMT  0x0188      // RISC Timer 1 High Limit Register
#define REG_RW_TIMER1_HIGH  0x018c      // RISC Timer 1 High Register

#define REG_RW_TIMER2_LLMT  0x0070      // RISC Timer 2 Low Limit Register
#define REG_T2LMT           0x0070      // RISC Timer 2 Low Limit Register
#define REG_RW_TIMER2_LOW   0x0074      // RISC Timer 2 Low Register
#define REG_T2              0x0074      // RISC Timer 2 Low Register
#define REG_RW_TIMER2_HLMT  0x0190      // RISC Timer 2 High Limit Register
#define REG_RW_TIMER2_HIGH  0x0194      // RISC Timer 2 High Register

#define REG_RW_TIMER_CTRL   0x0078      // RISC Timer Control Register
#define REG_TCTL            0x0078      // RISC Timer Control Register
    #define TMR0_CNTDWN_EN  (1U << 0)   // Timer 0 enable to count down
    #define TCTL_T0EN       (1U << 0)   // Timer 0 enable
    #define TMR0_AUTOLD_EN  (1U << 1)   // Timer 0 audo load enable
    #define TCTL_T0AL       (1U << 1)   // Timer 0 auto-load enable
    #define TMR1_CNTDWN_EN  (1U << 8)   // Timer 1 enable to count down
    #define TCTL_T1EN       (1U << 8)   // Timer 1 enable
    #define TMR1_AUTOLD_EN  (1U << 9)   // Timer 1 audo load enable
    #define TCTL_T1AL       (1U << 9)   // Timer 1 auto-load enable
    #define TMR2_CNTDWN_EN  (1U << 16)  // Timer 2 enable to count down
    #define TCTL_T2EN       (1U << 16)  // Timer 2 enable
    #define TMR2_AUTOLD_EN  (1U << 17)  // Timer 2 audo load enable
    #define TCTL_T2AL       (1U << 17)  // Timer 2 auto-load enable
    #define TMR_CNTDWN_EN(x) (1U << (x*8))
    #define TMR_AUTOLD_EN(x) (1U << (1+(x*8)))

//----------------------------------------------------------------------------
// IRQ/FIQ registers
#define REG_IRQST           0x0030      // RISC IRQ Status Register
#define REG_RO_IRQST        0x0030      // RISC IRQ Status Register
#define REG_IRQEN           0x0034      // RISC IRQ Enable Register
#define REG_RW_IRQEN        0x0034      // RISC IRQ Enable Register
#define REG_IRQCL           0x0038      // RISC IRQ Clear Register
#define REG_RW_IRQCLR       0x0038      // RISC IRQ Clear Register
#define REG_FIQST           0x003c      // RISC IRQ Status Register
#define REG_RO_FIQST        0x003c      // RISC IRQ Status Register
#define REG_FIQEN           0x0040      // RISC IRQ Enable Register
#define REG_RW_FIQEN        0x0040      // RISC IRQ Enable Register
#define REG_FIQCL           0x0044      // RISC IRQ Clear Register
#define REG_RW_FIQCLR       0x0044      // RISC IRQ Clear Register
#define REG_RW_MINTST       0x0048      // MISC IRQ Status Register
#define REG_RW_MINTEN       0x004c      // MISC IRQ Enable Register
#define REG_RW_MINTCLR      0x005c      // MISC IRQ Clear Register
#define REG_RW_M2INTST      0x0080      // MISC2 IRQ Status Register
#define REG_RW_M2INTEN      0x0084      // MISC2 IRQ Enable Register
#define REG_RW_M2INTCLR     0x0088      // MISC2 IRQ Clear Register
#ifdef CC_VECTOR_HAS_MISC3
#define REG_RW_M3INTST      0x008c      // MISC3 IRQ Status Register
#define REG_RW_M3INTEN      0x0090      // MISC3 IRQ Enable Register
#define REG_RW_M3INTCLR     0x007c      // MISC3 IRQ Clear Register
#endif // CC_VECTOR_HAS_MISC3
#define REG_RW_MISC         0x0098      // MISC. Register

// PDWNC register
#define REG_RW_SHREG0       0x0180      // share register

//----------------------------------------------------------------------------
// RISC General Purpose Registers
#define REG_RW_GPRB0        0x00e0      // RISC Byte General Purpose Register 0

/* BIM I/O Debug Register */
#define REG_IOBUS_TOUT_DBG  0x0320      // IOBUS TIME OUT DEBUG REGISTER
#define REG_DRAMIF_TOUT_DBG 0x0330      // DRAMIF TIME OUT DEBUG REGISTER
#define REG_DRAMIF_TOUT_ADDR 0x0334     // DRAMIF TIME OUT ADDRESS REGISTER

#define REG_CA15CLK_CTL		(0x0400)	// CA15L CLK CONTROL
    #define ARMPLL_CK_OFF	(1U << 0)
    #define PLL1_CK_OFF		(1U << 1)
    #define ARM_K1			(0xffU << 2)
    #define MUX1_SEL		(3U << 10)
    #define DCM_ENABLE		(1U << 12)
    #define ARMWFI_DCM_EN	(1U << 13)
    #define ARMWFE_DCM_EN	(1U << 14)
    #define FCA9_CLK_OFF	(1U << 15)
    #define SC_ARMCK_OFF_EN	(1U << 16)
    #define OCC_CTL			(0x3fU << 17)
#if defined(CONFIG_MACH_MT5890) || defined(CONFIG_MACH_MT5891)
#define REG_CA15ISOLATECPU	(0x0404)	// CA15L ISOLATE CONTROL
#define REG_CPU0_PWR_CTL	(0x0408)	// CPU0 POWER CONTROL
#define REG_CPU1_PWR_CTL	(0x040c)	// CPU1 POWER CONTROL
#define REG_CPU2_PWR_CTL	(0x0410)	// CPU2 POWER CONTROL
#define REG_CPU3_PWR_CTL	(0x0414)	// CPU3 POWER CONTROL
    #define PWR_RST_EN  	(1U << 0)
    #define CKISO			(1U << 1)
    #define PWR_ON_2ND		(1U << 2)
    #define PWR_ON			(1U << 3)
    #define PWR_ON_ACK		(1U << 4)
    #define PWR_ON_2ND_ACK	(1U << 5)
    //Oryx bigLittle
    #define ES3_ISOINTB     (1U << 1)
    #define ES3_CKISO		(1U << 2)
    #define ES3_PWR_ON_2ND	(1U << 3)
    #define ES3_PWR_ON		(1U << 4)
    #define ES3_PWR_ON_ACK	(1U << 5)
    #define ES3_PWR_ON_2ND_ACK	(1U << 6)
#define REG_L1_PD_CTL		(0x0418)	// L1 POWER DOWN CONTROL
    #define L1_SRAM_PD  	(0xfU << 0)
    #define L1_SRAM_ACK		(0xfU << 4)
    #define ES3_L1_SRAM_ACK	(0xfU << 2)
#define REG_PTP_CTL			(0x041c)	// PTP CONTROL
#define REG_CKDIV			(0x0420)	// CKDIV
#define REG_GIC_CTL			(0x0424)	// GIC CONTROL
#define REG_DCM_CNT			(0x0428)	// DCM CNT
#define REG_CA7_CORE0_PD    (0x0430)
#define REG_CA7_CORE1_PD    (0x0434)
    #define CA7_CPU_PD           (1U << 0)
    #define CA7_CPU_ISOINTB      (1U << 1)
    #define CA7_CPU_CKISO        (1U << 2)
    #define CA7_CPU_PD_ACK       (1U << 3)
#define REG_CA7_CPUSYS_PD   (0x0438)
    #define CA7_CPUSYS_PD           (1U << 0)
    #define CA7_CPUSYS_SLPB         (1U << 1)
    #define CA7_CPUSYS_ISOINTB      (1U << 2)
    #define CA7_CPUSYS_CKISO        (1U << 3)
    #define CA7_CPUSYS_PD_ACK       (1U << 4)
    #define CA7_CPUSYS_SLPB_ACK     (1U << 5)
#define REG_CA7_CORE0_PWR   (0x043c)
#define REG_CA7_CORE1_PWR   (0x0440)
    #define CA7_CPU_CLK_DIS      (1U << 0)
    #define CA7_CPU_RST_EN       (1U << 1)
    #define CA7_CPU_PWR_ON       (1U << 2)
    #define CA7_CPU_PWR_2ND_ON   (1U << 3)
    #define CA7_CPU_PWR_ACK      (1U << 4)
    #define CA7_CPU_PWR_ACK_2ND  (1U << 5)
    #define CA7_CPU_CLAMP        (1U << 6)
#define REG_CA7_DBG_PWR     (0x0444)
    #define CA7_DBG_CLK_DIS      (1U << 0)
    #define CA7_DBG_RST_EN       (1U << 1)
    #define CA7_DBG_PWR_ON       (1U << 2)
    #define CA7_DBG_PWR_2ND_ON   (1U << 3)
    #define CA7_DBG_PWR_ACK      (1U << 4)
    #define CA7_DBG_PWR_ACK_2ND  (1U << 5)
    #define CA7_DBG_CLAMP    (1U << 6)
#define REG_CA7_CPUSYS_TOP_PWR  (0x0448)
    #define CA7_CPUSYS_CLK_DIS      (1U << 0)
    #define CA7_CPUSYS_RST_EN       (1U << 1)
    #define CA7_CPUSYS_PWR_ON       (1U << 2)
    #define CA7_CPUSYS_PWR_2ND_ON   (1U << 3)
    #define CA7_CPUSYS_PWR_ACK      (1U << 4)
    #define CA7_CPUSYS_PWR_ACK_2ND  (1U << 5)
    #define CA7_CPUSYS_TOP_CLAMP    (1U << 6)
#define REG_CA7_CACTIVE     (0x044c)
    #define CA7_PWRDNREQN   (1U << 2)
    #define CA7_PWRDNACKN   (1U << 3)
#define REG_STANDBY_STS     (0x0450)
#define REG_CA7_CLKCTL      (0x045c)
#else
#define REG_CKDIV			(0x0404)	// CKDIV
#define REG_GIC_CTL			(0x0408)	// GIC CONTROL
#define REG_CPU1_PWR_CTL	(0x0414)	// CPU1 POWER CONTROL
#define REG_CPU2_PWR_CTL	(0x0418)	// CPU2 POWER CONTROL
#define REG_CPU3_PWR_CTL	(0x041c)	// CPU3 POWER CONTROL
    #define MEM_PD  	    (1U << 0)
	#define MEM_ISOINTB  	(1U << 1)
	#define MEM_CKISO  	        (1U << 2)
	#define MEM_PD_ACK  	    (1U << 3)
	#define PWR_CLK_DIS  	(1U << 4)
    #define PWR_RST_EN  	(1U << 5)
    #define PWR_ON			(1U << 6)
    #define PWR_ON_2ND		(1U << 7)
    #define PWR_ON_ACK		(1U << 8)
    #define PWR_ON_2ND_ACK	(1U << 9)
	#define CLAMP       	(1U << 10)
#endif
/* Powerdown module watch dog timer registers */
#define REG_RW_WATCHDOG_EN  0x0100      // Watchdog Timer Control Register
#define REG_RW_WATCHDOG_TMR 0x0104      // Watchdog Timer Register
#define REG_RW_WAKE_RSTCNT  0x0108      // Wakeup Reset Counter Register
#if defined(CONFIG_MACH_MT5890) || defined(CONFIG_MACH_MT5891)
#define REG_RW_RESRV5		0x0624		// Reserve register 5.
#endif

/* For Memory Size */
#define TCM_SRAM_ADDR       (IO_VIRT + 0x8100)
#define TCM_DRAM_SIZE      (*((volatile unsigned long *)(TCM_SRAM_ADDR - 0x4)))

/* CHIP Version related */
#if defined(CONFIG_ARCH_MT5396)
#define REG_RO_SW_ID        0x01f0      // Software ID register
#define REG_RO_FPGA_ID      0x01f8      // FPGA ID Register
#define REG_RO_CHIP_ID      0x01fc      // Chip ID Register
#else  //defined(CONFIG_ARCH_MT5890)
#define REG_RO_SW_ID        0x0064      // Software ID register
#define REG_RO_FPGA_ID      0x01f8      // FPGA ID Register
#define REG_RO_CHIP_ID      0x0060      // Chip ID Register
#endif

#define    PDWNC_CMD_ARM_WAKEUP_FROM_SUSPEND	       0x71   // only for suspend
//#ifdef CONFIG_OPM
/* For DRAM self refresh mode */
#define REG_RW_DRAM_00C	0x00C
#define REG_RW_DRAM_33c 0x33C
#define REG_RW_DRAM_224 0x224
#define REG_RW_DRAM_PHY_B94 0xB94
#define REG_RW_DRAM_PHY_B10 0xB10
#define REG_RW_DRAM_PHY_B18 0xB18

//REG_RW_DRAM_PHY_RESET
#define BIT_PHY_RESET 0x0003

#define BIT_PHY_TERMINATION_CTRL 0x0007
#define BIT_PHY_TERMINATION_CTRL_B94 0x0330

#define _SBF(f, v)		((v) << (f))
#define _BIT(n)			_SBF(n, 1)
#define BITS_UNKNOWN_10 _BIT(10)

extern void __iomem *p_bim_iomap;
extern void __iomem *p_pdwnc_iomap;
extern void __iomem *p_mcusys_iomap;
extern void __iomem *p_ckgen_iomap;
#define __bim_readl(REG)           __raw_readl(p_bim_iomap + (REG))
#define __bim_writel(VAL, REG)     __raw_writel(VAL, p_bim_iomap + (REG))
#define __pdwnc_readl(REG)           __raw_readl(p_pdwnc_iomap + (REG))
#define __pdwnc_writel(VAL, REG)     __raw_writel(VAL, p_pdwnc_iomap + (REG))
#define __ckgen_readl(REG)           __raw_readl(p_ckgen_iomap + (REG))
//TODO: should reverse the order
#define __ckgen_writel(REG, VAL)     __raw_writel(VAL, p_ckgen_iomap + (REG))

#endif /* __ASM_ARCH_PLATFORM_H */

