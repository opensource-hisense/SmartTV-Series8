#ifndef _STAR_REG_H_
#define _STAR_REG_H_

#include <asm/io.h>
#include <asm/types.h>
#if defined (CONFIG_ARCH_MT5391)
#include <mach/mt5391.h>
#elif defined (CONFIG_ARCH_MT5363)
#include <mach/mt5363.h>
#elif defined (CONFIG_ARCH_MT5365)
#include <mach/mt5365.h>
#elif defined (CONFIG_ARCH_MT5395)
#include <mach/mt5395.h>
#elif defined (CONFIG_ARCH_MT5396) || defined(CONFIG_ARCH_MT5368) || defined(CONFIG_ARCH_MT5389) || defined(CONFIG_ARCH_MT5398) || defined(CONFIG_ARCH_MT5880)|| defined(CONFIG_ARCH_MT5399) || defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5882) || defined(CONFIG_ARCH_MT5883) || defined(CONFIG_ARCH_MT5891)
#ifdef CONFIG_ARM64
#include "linux/soc/mediatek/platform.h"
#else
#include <mach/platform.h>
#endif
#else /* other platform */
#error "Error. No ARCH_MTXXXX been defined!"
#endif

#define UINT32 u32

#if defined(CONFIG_ARCH_MT5396) || defined(CONFIG_ARCH_MT5368) || defined(CONFIG_ARCH_MT5389) || defined(CONFIG_ARCH_MT5398) || defined(CONFIG_ARCH_MT5880)|| defined(CONFIG_ARCH_MT5399) || defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5882) || defined(CONFIG_ARCH_MT5883)|| defined(CONFIG_ARCH_MT5891) 
#define CC_ETHERNET_6896	1
#endif

/* We are using ioremap addresses, so the base should be IO_PHYS */
#define ETH_BASE                            (IO_PHYS + 0x32000)
#define PINMUX_BASE                         (IO_PHYS + 0x0d400)
#if defined(CONFIG_ARCH_MT5891)
#define PDWNC_BASE                          (IO_PHYS + 0x28000)
#endif
//#define ETHERNET_PDWNC_BASE                 (IO_PHYS + 0x24C00)
//#define ETH_CHKSUM_BASE                     (IO_PHYS + 0x3d000)
#define IPLL1_BASE                          (IO_PHYS + 0x0d000)//for CK-GEN
//#define IPLL2_BASE                          (IO_PHYS + 0x90300)
//#define BSP_BASE                            (IO_PHYS + 0x0)

/* Note: ETH_IRQ for MT5365 and MT5395 is different */
#define ETH_IRQ_MT5365                      (VECTOR_ENET)//(8)in include\asm-arm\arch-mt539x
#define ETH_IRQ_MT5395                      (VECTOR_ENET)//(8)
#define ETH_IRQ_MT5368                      (VECTOR_ENET)//(8)
#define ETH_IRQ_MT5396                      (VECTOR_ENET)//(8)

#define ETH_IRQ                             ETH_IRQ_MT5396
#define ETH_DEFAULT_PHY_ADDR                (1)
/* Star Giga Ethernet Controller registers */
/* ====================================================================================== */
#define STAR_PHY_CTRL0(base)				(base + 0x0000)		/* PHY control register0 */
#define STAR_PHY_CTRL1(base)				(base + 0x0004)		/* PHY control register1 */
#define STAR_MAC_CFG(base)					(base + 0x0008)		/* MAC Configuration register */
#define STAR_FC_CFG(base)					(base + 0x000c)		/* Flow Control Configuration register */
#define STAR_ARL_CFG(base)					(base + 0x0010)		/* ARL Configuration register */
#define STAR_My_MAC_H(base)					(base + 0x0014)		/* My MAC High Byte */
#define STAR_My_MAC_L(base)					(base + 0x0018)		/* My MAC Low Byte */
#define STAR_HASH_CTRL(base)				(base + 0x001c)		/* Hash Table Control register */
#define STAR_VLAN_CTRL(base)				(base + 0x0020)		/* My VLAN ID Control register */
#define STAR_VLAN_ID_0_1(base)				(base + 0x0024)		/* My VLAN ID 0 - 1 register */
#define STAR_VLAN_ID_2_3(base)				(base + 0x0028)		/* My VLAN ID 2 - 3 register */
#define STAR_DUMMY(base)					(base + 0x002C)		/* Dummy Register */
#define STAR_DMA_CFG(base)					(base + 0x0030)		/* DMA Configuration register */
#define STAR_TX_DMA_CTRL(base)				(base + 0x0034)		/* TX DMA Control register */
#define STAR_RX_DMA_CTRL(base)				(base + 0x0038)		/* RX DMA Control register */
#define STAR_TX_DPTR(base)					(base + 0x003c)		/* TX Descriptor Pointer */
#define STAR_RX_DPTR(base)					(base + 0x0040)		/* RX Descriptor Pointer */
#define STAR_TX_BASE_ADDR(base)				(base + 0x0044)		/* TX Descriptor Base Address */
#define STAR_RX_BASE_ADDR(base)				(base + 0x0048)		/* RX Descriptor Base Address */
#define STAR_INT_STA(base)					(base + 0x0050)		/* Interrupt Status register */
#define STAR_INT_MASK(base)					(base + 0x0054)		/* Interrupt Mask register */
#define STAR_TEST0(base)					(base + 0x0058)		/* Test0(Clock Skew Setting) register */
#define STAR_TEST1(base)					(base + 0x005c)		/* Test1(Queue status) register */
#define STAR_EXTEND_CFG(base)		        (base + 0x0060)		/* Extended Configuration(Send Pause Off frame thre) register */
#define STAR_ETHPHY(base)					(base + 0x006C)		/* EthPhy Register */
#define STAR_AHB_BURSTTYPE(base)			(base + 0x0074)		/* AHB Burst Type Register */

#ifdef CC_ETHERNET_6896

///0x33034
  #define TX_RESUME				((UINT32)0x01 << 2)
  #define TX_STOP  				((UINT32)0x01 << 1)
  #define TX_START  				((UINT32)0x01 << 0)

#define EEE_CTRL(base)          (base + 0x78)
  #define LPI_MODE_EB ((UINT32)0x01 << 0)
	
#define EEE_SLEEP_TIMER(base)  (base + 0x7c)
	
#define EEE_WAKEUP_TIMER(base)  (base + 0x80)
	
#define ETHPHY_CONFIG1(base)  (base + 0x84)
#define RW_INTERNAL_PHY_CTRL(base)  (base + 0x84)
  #define INTERNAL_PHY_ADDRESS_MASK    ((UINT32)0x3c)
    #define INTERNAL_PHY_ADDRESS_POS    (6)
	
#define ETHPHY_CONFIG2(base)  (base + 0x88)
	
#define RW_ARB_MON2(base) 		(base + 0x8C)
	
	
#define ETHPHY_CLOCK_CONTROL(base)  (base + 0x90)
	
#define ETHSYS_CONFIG(base)  (base + 0x94)
  #define INT_PHY_SEL ((UINT32)0x01 << 3)
	
#define MAC_MODE_CONFIG(base)  (base + 0x98)
  #define BIG_ENDIAN				((UINT32)0x01 << 0)
	
#define SW_RESET_CONTROL(base)  (base + 0x9c)
  #define PHY_RSTN					((UINT32)0x01 << 4)
  #define MRST						((UINT32)0x01 << 3)
  #define NRST						((UINT32)0x01 << 2)
  #define HRST						((UINT32)0x01 << 1)
  #define DMA_RESET				    ((UINT32)0x01 << 0)
	
#define MAC_CLOCK_CONFIG(base)  (base + 0xac)
  #define TXCLK_OUT_INV				((UINT32)0x01 << 19)
  #define RXCLK_OUT_INV				((UINT32)0x01 << 18)
  #define TXCLK_IN_INV				((UINT32)0x01 << 17)
  #define RXCLK_IN_INV				((UINT32)0x01 << 16)
  #define MDC_INV					((UINT32)0x01 << 12)
  #define MDC_NEG_LAT				((UINT32)0x01 << 8)
  #define MDC_DIV					((UINT32)0xFF << 0)
    #define MDC_CLK_DIV_10			((UINT32)0x0A << 0)

#else

#define STAR_ETHPHY_CTRL0(base)			    (base + 0x0080)		/* Ethernet PHY Control Register */
#define STAR_ETHPHY_CTRL1(base)			    (base + 0x0084)		/* Ethernet PHY Control Register1 */
#define STAR_ETHPHY_CTRL2(base)			    (base + 0x0088)		/* Ethernet PHY Control Register2 */
#define STAR_ETHPHY_MON(base)			    (base + 0x008c)		/* Ethernet PHY Monitor Register */
#define STAR_EXT_CFG(base)                  (base + 0x0090)     /* External Control Register */
#define STAR_CLK_CFG(base)                  (base + 0x009c)     /* MAC Clock Configuration */
#endif

	/* MIB Counter register */
#define STAR_MIB_RXOKPKT(base)				(base + 0x0100)		/* RX OK Packet Counter register */
#define STAR_MIB_RXOKBYTE(base)				(base + 0x0104)		/* RX OK Byte Counter register */
#define STAR_MIB_RXRUNT(base)				(base + 0x0108)		/* RX Runt Packet Counter register */
#define STAR_MIB_RXOVERSIZE(base)			(base + 0x010c)		/* RX Over Size Packet Counter register */
#define STAR_MIB_RXNOBUFDROP(base)			(base + 0x0110)		/* RX No Buffer Drop Packet Counter register */
#define STAR_MIB_RXCRCERR(base)				(base + 0x0114)		/* RX CRC Error Packet Counter register */
#define STAR_MIB_RXARLDROP(base)			(base + 0x0118)		/* RX ARL Drop Packet Counter register */
#define STAR_MIB_RXVLANDROP(base)			(base + 0x011c)		/* RX My VLAN ID Mismatch Drop Packet Counter register */
#define STAR_MIB_RXCKSERR(base)				(base + 0x0120)		/* RX Checksum Error Packet Counter register */
#define STAR_MIB_RXPAUSE(base)				(base + 0x0124)		/* RX Pause Frame Packet Counter register */
#define STAR_MIB_TXOKPKT(base)				(base + 0x0128)		/* TX OK Packet Counter register */
#define STAR_MIB_TXOKBYTE(base)				(base + 0x012c)		/* TX OK Byte Counter register */
#define STAR_MIB_TXPAUSECOL(base)			(base + 0x0130)		/* TX Pause Frame Counter register (Full->pause count, half->collision count) */

#if defined(CONFIG_ARCH_MT5891)
#define STAR_STBY_CTRL(base)				(base + 0x01bc)		/* Control register for standby mode*/
#define STAR_STBY_CTRL_WOL_EN			    	(1 << 0)		/*1:enable HW arp offload*/
#define STAR_STBY_CTRL_ARP_EN			    	(1 << 1)		/*1: enable WOL packet detection*/
#define STAR_WOL_SRAM_CTRL(base)			(base + 0x01C0)		/*WOL SRAM read/write control register*/
#define SRAM2_WE					(1 << 9)
#define SRAM1_WE					(1 << 8)
#define SRAM0_WE					(1 << 7)
#define SRAM_ADDR					0x7f			/*0~6 IS SRAM_ADDR*/
#define STAR_WOL_SRAM_STATE(base)			(base + 0x01C4)		/*WOL SRAM configurattion register 1*/
#define WOL_SRAM_IDEL 					0x00
#define WOL_SRAM_CSR_W					0x01
#define WOL_SRAM_CSR_R					0x02
#define WOL_SRAM_ACTIVE					0x03
#define STAR_WOL_SRAM0_CFG0(base)			(base + 0x01C8)		/*WOL check pattern: patn3,patn2,patn1,patn0*/
#define STAR_WOL_SRAM0_CFG1(base)			(base + 0x01CC)		/*WOL check pattern: patn7,patn6,patn5,patn4*/
#define STAR_WOL_SRAM0_CFG2(base)			(base + 0x01D0)		/*WOL check pattern: patn11,patn10,patn9,patn8*/
#define STAR_WOL_SRAM0_CFG3(base)			(base + 0x01D4)		/*WOL check pattern: patn15,patn14,patn13,patn12*/
#define STAR_WOL_SRAM1_CFG(base)			(base + 0x01D8)		/*WOL check pattern: patn19,patn18,patn17,patn16*/
#define STAR_WOL_SRAM2_MASK(base)			(base + 0x01DC)		/*WOL  byte mask for 20 patterns: 1, enable byte check*/
#define STAR_WOL_SRAM_STATUS(base)			(base + 0x01F8)     	/*WOL SRAM config ready, RC */
#define STAR_WOL_DETECT_STATUS(base)			(base + 0x0280)     	/*WOL packet detect status,write0x50[11]=1 clear*/
#define STAR_WOL_CHECK_LEN0(base)			(base + 0x0284)		/*WOL check pattern length(0<x<128): patn3,patn2,patn1,patn0*/
#define STAR_WOL_CHECK_LEN1(base)			(base + 0x0288)		/*WOL check pattern length(0<x<128): patn7,patn6,patn5,patn4*/
#define STAR_WOL_CHECK_LEN2(base)			(base + 0x028c)		/*WOL check pattern length(0<x<128): patn11,patn10,patn9,patn8*/
#define STAR_WOL_CHECK_LEN3(base)			(base + 0x0290)		/*WOL check pattern length(0<x<128): patn15,patn14,patn13,patn12*/
#define STAR_WOL_CHECK_LEN4(base)			(base + 0x0294)		/*WOL check pattern length(0<x<128): patn19,patn18,patn17,patn16*/
#define STAR_WOL_PATTERN_DIS(base)			(base + 0x0298)		/*WOL check pattern disable */
#define STAR_WOL_MIRXBUF_CTL(base)			(base + 0x029c)		/*WOL check pattern disable */
#define WOL_MRXBUF_RESET				(1 << 1)
#define WOL_MRXBUF_EN					(1 << 0)

#define STAR_MGP_LENGTH_STA(base)			(base + 0x01FC)     	/*WOL :  valid lengh(<=128) of magic packet append data */
#define STAR_MGP_DATA_START(base)			(base + 0x0200)		/*latch data 0 */
#define STAR_MGP_DATA_END(base)				(base + 0x027c)		/*latch data 31 */

#define STAR_ARP0_MAC_L(base)				(base + 0x01E0)     	/*mac0[31:0] for ARP reply*/
#define STAR_ARP0_MAC_H(base)				(base + 0x01E4)     	/*mac0[47:32] for ARP reply*/
#define STAR_ARP1_MAC_L(base)				(base + 0x01E8)     	/*mac1[31:0] for ARP reply*/
#define STAR_ARP1_MAC_H(base)				(base + 0x01EC)     	/*mac1[47:32] for ARP reply*/
#define STAR_ARP0_IP(base)				(base + 0x01F0)     	/*IP0 for ARP reply*/
#define STAR_ARP1_IP(base)				(base + 0x01F4)     	/*IP1 for ARP reply*/
#endif

#define STAR_ETH_UP_CFG(base)               (base + 0x0300)
#define CPUCK_BY_8032                       (1 << 16)

#define STAR_ETH_PDREG_SOFT_RST(base)		(base + 0x308)

#define STAR_ETH_PDREG_ETCKSEL(base)		(base + 0x314)
#define ETH_AFE_PWD2						(1 << 11)
#define RG_DA_XTAL1_SEL						(1 << 16)

#define STAR_ETH_PDREG_ENETPLS(base)		(base + 0x31c)
#define PLS_MONCKEN_PRE						(1 << 19)
#define PLS_FMEN_PRE						(1 << 20)

/*  PDWNC register base
    Note: pdwnc_base starts from PDWNC_BASE + 0x100 */
#define PDWNC_BASE_CFG_OFFSET               (0x100)
/* Note: uP configuration register at 0x24188 */
#define PDWNC_REG_UP_CONF_OFFSET(base)      (base + 0x88)
#define UP_CONF_ETEN_EN                     (1U << 28)


/* Ethernet PDWNC register */
#define ETHERNET_PDWNC_REG_OFFSET           (ETHERNET_PDWNC_BASE + 0x000)


/* Pinmux register for Ethernet */
#define PINMUX_REG_PAD_PINMUX2(base)        (base + 0x00f8)     /* Pinmux for Ethernet */
#define PINMUX_REG_PAD_PINMUX2_ETH_OFFSET   (5)
#define PINMUX_REG_PAD_PINMUX2_ETH_MASK     (0x3ffff)


/* =============================================================
		Detail definition of Star Giga Ethernet Controller registers
   ============================================================= */
/* STAR_PHY_CTRL0 (0x33000)*/
/* =================================================================== */
#define STAR_PHY_CTRL0_RWDATA_MASK			(0xffff)			/* Mask of Read/Write Data */
#define STAR_PHY_CTRL0_RWDATA_OFFSET		(16)				/* Offset of Read/Write Data */
#define STAR_PHY_CTRL0_RWOK					(1 << 15)			/* R/W command has completed (write 1 clear) */
#define STAR_PHY_CTRL0_RDCMD				(1 << 14)			/* Read command (self clear) */
#define STAR_PHY_CTRL0_WTCMD				(1 << 13)			/* Write command (self clear) */
#define STAR_PHY_CTRL0_PREG_MASK			(0x1f)				/* Mask of PHY Register Address */
#define STAR_PHY_CTRL0_PREG_OFFSET			(8)					/* Offset of PHY Register Address */
#define STAR_PHY_CTRL0_PA_MASK				(0x1f)				/* Mask of PHY Address */
#define STAR_PHY_CTRL0_PA_OFFSET			(0)					/* Offset of PHY Address */

/* STAR_PHY_CTRL1 (0x33004) */
/* =================================================================== */
#define STAR_PHY_CTRL1_APDIS				(1 << 31)			/* Disable PHY auto polling (1:disable) */
#define STAR_PHY_CTRL1_APEN					(0 << 31)			/* Enable PHY auto polling (0:enable) */
#define STAR_PHY_CTRL1_PHYADDR_MASK			(0x1f)				/* Mask of PHY Address used for auto-polling */
#define STAR_PHY_CTRL1_PHYADDR_OFFSET		(24)				/* Offset of PHY Address used for auto-polling */
#define STAR_PHY_CTRL1_RGMII				(1 << 17)			/* RGMII_PHY used */
#define STAR_PHY_CTRL1_REVMII				(1 << 16)			/* Reversed MII Mode Enable, 0:normal 1:reversed MII(phy side) */
#define STAR_PHY_CTRL1_TXCLK_CKEN			(1 << 14)			/* TX clock period checking Enable */
#define STAR_PHY_CTRL1_FORCETXFC			(1 << 13)			/* Force TX Flow Control when MI disable */
#define STAR_PHY_CTRL1_FORCERXFC			(1 << 12)			/* Force RX Flow Control when MI disable */
#define STAR_PHY_CTRL1_FORCEFULL			(1 << 11)			/* Force Full Duplex when MI disable */
#define STAR_PHY_CTRL1_FORCESPD_MASK		(0x3)				/* Mask of Force Speed when MI disable */
#define STAR_PHY_CTRL1_FORCESPD_OFFSET		(9)					/* Offset of Force Speed when MI disable */
#define STAR_PHY_CTRL1_FORCESPD_10M			(0 << STAR_PHY_CTRL1_FORCESPD_OFFSET)
#define STAR_PHY_CTRL1_FORCESPD_100M		(1 << STAR_PHY_CTRL1_FORCESPD_OFFSET)
#define STAR_PHY_CTRL1_FORCESPD_1G			(2 << STAR_PHY_CTRL1_FORCESPD_OFFSET)
#define STAR_PHY_CTRL1_FORCESPD_RESV		(3 << STAR_PHY_CTRL1_FORCESPD_OFFSET)
#define STAR_PHY_CTRL1_ANEN					(1 << 8)			/* Auto-Negotiation Enable */
#define STAR_PHY_CTRL1_MIDIS				(1 << 7)			/* MI auto-polling disable, 0:active 1:disable */
	/* PHY status */
#define STAR_PHY_CTRL1_STA_TXFC				(1 << 6)			/* TX Flow Control status (only for 1000Mbps) */
#define STAR_PHY_CTRL1_STA_RXFC				(1 << 5)			/* RX Flow Control status */
#define STAR_PHY_CTRL1_STA_FULL				(1 << 4)			/* Duplex status, 1:full 0:half */
#define STAR_PHY_CTRL1_STA_SPD_MASK			(0x3)				/* Mask of Speed status */
#define STAR_PHY_CTRL1_STA_SPD_OFFSET		(2)					/* Offset of Speed status */
#define STAR_PHY_CTRL1_STA_SPD_10M			(0 << STAR_PHY_CTRL1_STA_SPD_OFFSET)
#define STAR_PHY_CTRL1_STA_SPD_100M			(1 << STAR_PHY_CTRL1_STA_SPD_OFFSET)
#define STAR_PHY_CTRL1_STA_SPD_1G			(2 << STAR_PHY_CTRL1_STA_SPD_OFFSET)
#define STAR_PHY_CTRL1_STA_SPD_RESV			(3 << STAR_PHY_CTRL1_STA_SPD_OFFSET)
#define STAR_PHY_CTRL1_STA_TXCLK			(1 << 1)			/* TX clock status, 0:normal 1:no TXC or clk period too long */
#define STAR_PHY_CTRL1_STA_LINK				(1 << 0)			/* PHY Link status */
 

/* STAR_MAC_CFG (0x33008)*/
/* =================================================================== */
#define STAR_MAC_CFG_NICPD					(1 << 31)			/* NIC Power Down */
#define STAR_MAC_CFG_WOLEN					(1 << 30)			/* Wake on LAN Enable */
#define STAR_MAC_CFG_NICPDRDY				(1 << 29)			/* NIC Power Down Ready */
#define STAR_MAC_CFG_TXCKSEN				(1 << 26)			/* TX IP/TCP/UDP Checksum offload Enable */
#define STAR_MAC_CFG_RXCKSEN				(1 << 25)			/* RX IP/TCP/UDP Checksum offload Enable */
#define STAR_MAC_CFG_ACPTCKSERR				(1 << 24)			/* Accept Checksum Error Packets */
#define STAR_MAC_CFG_ISTEN					(1 << 23)			/* Inter Switch Tag Enable */
#define STAR_MAC_CFG_VLANSTRIP				(1 << 22)			/* VLAN Tag Stripping */
#define STAR_MAC_CFG_ACPTCRCERR				(1 << 21)			/* Accept CRC Error Packets */
#define STAR_MAC_CFG_CRCSTRIP				(1 << 20)			/* CRC Stripping */
#define STAR_MAC_CFG_ACPTLONGPKT			(1 << 18)			/* Accept Oversized Packets */
#define STAR_MAC_CFG_MAXLEN_MASK			(0x3)				/* Mask of Maximum Packet Length */
#define STAR_MAC_CFG_MAXLEN_OFFSET			(16)				/* Offset of Maximum Packet Length */
#define STAR_MAC_CFG_MAXLEN_1518			(0 << STAR_MAC_CFG_MAXLEN_OFFSET)
#define STAR_MAC_CFG_MAXLEN_1522			(1 << STAR_MAC_CFG_MAXLEN_OFFSET)
#define STAR_MAC_CFG_MAXLEN_1536			(2 << STAR_MAC_CFG_MAXLEN_OFFSET)
#define STAR_MAC_CFG_MAXLEN_RESV			(3 << STAR_MAC_CFG_MAXLEN_OFFSET)
#define STAR_MAC_CFG_IPG_MASK				(0x1f)				/* Mask of Inter Packet Gap */
#define STAR_MAC_CFG_IPG_OFFSET				(10)				/* Offset of Inter Packet Gap */
#define STAR_MAC_CFG_NSKP16COL				(1 << 9)			/* Dont's skip 16 consecutive collisions packet */
#define STAR_MAC_CFG_FASTBACKOFF			(1 << 8)			/* Collision Fast Back-off */
#define STAR_MAC_CFG_TXVLAN_ATPARSE			(1 << 0)			/* TX VLAN Auto Parse. 1: Hardware decide VLAN packet */

/* STAR_FC_CFG (0x3300c)*/
/* =================================================================== */
#define STAR_FC_CFG_SENDPAUSETH_MASK		(0xfff)				/* Mask of Send Pause Threshold */
#define STAR_FC_CFG_SENDPAUSETH_OFFSET		(16)				/* Offset of Send Pause Threshold */
#define STAR_FC_CFG_COLCNT_CLR_MODE         (1 << 9)            /* Collisin Count Clear Mode */
#define STAR_FC_CFG_UCPAUSEDIS				(1 << 8)			/* Disable unicast Pause */
#define STAR_FC_CFG_BPEN					(1 << 7)			/* Half Duplex Back Pressure Enable */
#define STAR_FC_CFG_CRS_BP_MODE             (1 << 6)            /* Half Duplex Back Pressure force carrier */
#define STAR_FC_CFG_MAXBPCOLEN				(1 << 5)			/* Pass-one-every-N backpressure collision policy Enable */
#define STAR_FC_CFG_MAXBPCOLCNT_MASK		(0x1f)				/* Mask of Max backpressure collision count */
#define STAR_FC_CFG_MAXBPCOLCNT_OFFSET		(0)					/* Offset of Max backpressure collision count */

    /* default value for SEND_PAUSE_TH */
#define STAR_FC_CFG_SEND_PAUSE_TH_DEF       ((STAR_FC_CFG_SEND_PAUSE_TH_2K & \
                                              STAR_FC_CFG_SENDPAUSETH_MASK) \
                                              << STAR_FC_CFG_SENDPAUSETH_OFFSET)
#define STAR_FC_CFG_SEND_PAUSE_TH_2K        (0x800)


/* STAR_ARL_CFG (0x33010) */
/* =================================================================== */
#define STAR_ARL_CFG_FILTER_PRI_TAG			(1 << 6)			/* Filter Priority-tagged packet */
#define STAR_ARL_CFG_FILTER_VLAN_UNTAG		(1 << 5)			/* Filter VLAN-untagged packet */
#define STAR_ARL_CFG_MISCMODE				(1 << 4)			/* Miscellaneous(promiscuous) mode */
#define STAR_ARL_CFG_MYMACONLY				(1 << 3)			/* 1:Only My_MAC/BC packets are received, 0:My_MAC/BC/Hash_Table_hit packets are received */
#define STAR_ARL_CFG_CPULEARNDIS			(1 << 2)			/* From CPU SA Learning Disable */
#define STAR_ARL_CFG_RESVMCFILTER			(1 << 1)			/* Reserved Multicast Address Filtering, 0:forward to CPU 1:drop */
#define STAR_ARL_CFG_HASHALG_CRCDA			(1 << 0)			/* MAC Address Hashing algorithm, 0:DA as hash 1:CRC of DA as hash */

/* STAR_HASH_CTRL (0x3301c) */
/* =================================================================== */
#define STAR_HASH_CTRL_HASHEN				(1 << 31)			/* Hash Table Enable */
#define STAR_HASH_CTRL_HTBISTDONE			(1 << 17)			/* Hash Table BIST(Build In Self Test) Done */
#define STAR_HASH_CTRL_HTBISTOK				(1 << 16)			/* Hash Table BIST(Build In Self Test) OK */
#define STAR_HASH_CTRL_START				(1 << 14)			/* Hash Access Command Start */
#define STAR_HASH_CTRL_ACCESSWT				(1 << 13)			/* Hash Access Write Command, 0:read 1:write */
#define STAR_HASH_CTRL_ACCESSRD				(0 << 13)			/* Hash Access Read Command, 0:read 1:write */
#define STAR_HASH_CTRL_HBITDATA				(1 << 12)			/* Hash Bit Data (Read or Write) */
#define STAR_HASH_CTRL_HBITADDR_MASK		(0x1ff)				/* Mask of Hash Bit Address */
#define STAR_HASH_CTRL_HBITADDR_OFFSET		(0)					/* Offset of Hash Bit Address */

/* STARETH_DUMMY (0x3302c) */
/* =================================================================== */
#define STAR_DUMMY_FPGA_MODE				(1 << 31)			/* FPGA mode or ASIC mode */
#define STAR_DUMMY_TXRXRDY					(1 << 1)			/* Asserted when tx/rx path is IDLE and rxclk available */
#define STAR_DUMMY_MDCMDIODONE				(1 << 0)			/* MDC/MDIO done */

/* STAR_DMA_CFG (0x0x33030) */
/* =================================================================== */
#define STAR_DMA_CFG_RX2BOFSTDIS			(1 << 16)			/* RX 2 Bytes offset disable */
#define STAR_DMA_CFG_TXPOLLPERIOD_MASK		(0x3)				/* Mask of TX DMA Auto-Poll Period */
#define STAR_DMA_CFG_TXPOLLPERIOD_OFFSET	(6)					/* Offset of TX DMA Auto-Poll Period */
#define STAR_DMA_CFG_TXPOLLPERIOD_1US		(0 << STAR_DMA_CFG_TXPOLLPERIOD_OFFSET)
#define STAR_DMA_CFG_TXPOLLPERIOD_10US		(1 << STAR_DMA_CFG_TXPOLLPERIOD_OFFSET)
#define STAR_DMA_CFG_TXPOLLPERIOD_100US		(2 << STAR_DMA_CFG_TXPOLLPERIOD_OFFSET)
#define STAR_DMA_CFG_TXPOLLPERIOD_1000US	(3 << STAR_DMA_CFG_TXPOLLPERIOD_OFFSET)
#define STAR_DMA_CFG_TXPOLLEN				(1 << 5)			/* TX DMA Auto-Poll C-Bit Enable */
#define STAR_DMA_CFG_TXSUSPEND				(1 << 4)			/* TX DMA Suspend */
#define STAR_DMA_CFG_RXPOLLPERIOD_MASK		(0x3)				/* Mask of RX DMA Auto-Poll Period */
#define STAR_DMA_CFG_RXPOLLPERIOD_OFFSET	(2)					/* Offset of RX DMA Auto-Poll Period */
#define STAR_DMA_CFG_RXPOLLPERIOD_1US		(0 << STAR_DMA_CFG_RXPOLLPERIOD_OFFSET)
#define STAR_DMA_CFG_RXPOLLPERIOD_10US		(1 << STAR_DMA_CFG_RXPOLLPERIOD_OFFSET)
#define STAR_DMA_CFG_RXPOLLPERIOD_100US		(2 << STAR_DMA_CFG_RXPOLLPERIOD_OFFSET)
#define STAR_DMA_CFG_RXPOLLPERIOD_1000US	(3 << STAR_DMA_CFG_RXPOLLPERIOD_OFFSET)
#define STAR_DMA_CFG_RXPOLLEN				(1 << 1)			/* RX DMA Auto-Poll C-Bit Enable */
#define STAR_DMA_CFG_RXSUSPEND				(1 << 0)			/* RX DMA Suspend */
#ifndef CC_ETHERNET_6896
#define STAR_DMA_CFG_RX_INTR_WDLE           (1 << 8)            /* RX interrupt generated after receive wdle */
#define STAR_DMA_CFG_TX_INTR_WDLE           (1 << 9)            /* TX interrupt generated after receive wdle */
#endif

/* STAR_INT_STA (0x33050) */
/* =================================================================== */
#define STAR_INT_STA_TX_SKIP			    (1 << 9)			/* To NIC Tx Skip Interrupt (retry>15) */
#define STAR_INT_STA_WOP			    	(1 << 11)			/* Receive the Wakeup on packet interrupt*/
#define STAR_INT_STA_TXC					(1 << 8)			/* To NIC DMA Transmit Complete Interrupt */
#define STAR_INT_STA_TXQE					(1 << 7)			/* To NIC DMA Queue Empty Interrupt */
#define STAR_INT_STA_RXC					(1 << 6)			/* From NIC DMA Receive Complete Interrupt */
#define STAR_INT_STA_RXQF					(1 << 5)			/* From NIC DMA Receive Queue Full Interrupt */
#define STAR_INT_STA_MAGICPKT				(1 << 4)			/* Magic packet received */
#define STAR_INT_STA_MIBCNTHALF				(1 << 3)			/* Assert when any one MIB counter reach 0x80000000(half) */
#define STAR_INT_STA_PORTCHANGE				(1 << 2)			/* Assert MAC Port Change Link State (link up <-> link down) */
#define STAR_INT_STA_RXFIFOFULL				(1 << 1)			/* Assert when RX Buffer full */
#ifdef CC_ETHERNET_6896
#define STAR_INT_STA_RX_PCODE			    (1 << 10)
#else
#define STAR_INT_STA_TXFIFOUDRUN			(1 << 0)			/* Assert when TX Buffer under run in transmitting a packet */
#endif

/* STAR_EXTEND_CFG (0x33060) */
/* =================================================================== */
#define STAR_EXTEND_CFG_SDPAUSEOFFTH_MASK	    (0xfff)		    /* Mask of Send Pause Off Frame Threshold */
#define STAR_EXTEND_CFG_SDPAUSEOFFTH_OFFSET	    (16)	        /* Offset of Send Pause Off Frame Threshold */

    /* default value for SEND_PAUSE_RLS */
#define STAR_EXTEND_CFG_SEND_PAUSE_RLS_DEF      ((STAR_EXTEND_CFG_SEND_PAUSE_RLS_1K& \
                                                STAR_EXTEND_CFG_SDPAUSEOFFTH_MASK) \
                                                << STAR_EXTEND_CFG_SDPAUSEOFFTH_OFFSET)
#define STAR_EXTEND_CFG_SEND_PAUSE_RLS_1K       (0x400)

/* STAR_TEST1 (0x3305c) */
/* =================================================================== */
#define STAR_TEST1_EXTEND_RETRY				(1 << 20)			/* Extended Retry */
#define STAR_TEST1_RST_HASH_BIST			(1 << 31)			/* Restart Hash Table Bist */

/* STAR_ETHPHY (0x3306c) */
/* =================================================================== */
#define STAR_ETHPHY_CFG_INT_ETHPHY			(1 << 9)			/* Internal PHY Mode */
#define STAR_ETHPHY_EXTMDC_MODE				(1 << 6)			/* External MDC Mode */
#define STAR_ETHPHY_MIIPAD_OE				(1 << 3)			/* MII output enable */
#define STAR_ETHPHY_FRC_SMI_EN				(1 << 2)			/* Force SMI Enable */

#ifndef CC_ETHERNET_6896

/* STAR_EXT_CFG (0x33090) */
/* =================================================================== */
  #define BIG_ENDIAN				((UINT32)0x01 << 31)  
  #define DRAM_PRIORITY				((UINT32)0x01 << 30)  
   	#define DRAM_PRIORITY_HIGH		((UINT32)0x01 << 30)  
  #define PDMAC_SEL					((UINT32)0x01 << 27)
    #define PDMAC_SEL_PDWNC			((UINT32)0x01 << 27)
  #define TEST_CLK					((UINT32)0x01 << 26) 
    #define TEST_CLK_27M			((UINT32)0x01 << 26)
  #define RMII_SRC_SEL_EXT_PAD		((UINT32)0x01 << 26)   
  #define COL_SEL					((UINT32)0x01 << 25) 
  	#define COL_SEL_EXTERNAL		((UINT32)0x01 << 25) // For MII use
	#define COL_SEL_INTERNAL		((UINT32)0x00 << 25)  //For RMII use 
  #define RMII_CLK_SEL				((UINT32)0x01 << 24) 
  #define TCPIP_RST					((UINT32)0x01 << 23) 
  #define MRST						((UINT32)0x01 << 22)  
  #define NRST						((UINT32)0x01 << 21)  
  #define HRST						((UINT32)0x01 << 20) 
  #define RMII_CLK_INV				((UINT32)0x01 << 19) 
  #define DMA_RST   				((UINT32)0x01 << 18)   
  #define PB_NIC_SEL				((UINT32)0x3F << 11)  
  	 #define PB_NIC_SEL_20H			((UINT32)0x20 << 11) 
  #define CFG_NIC_REVMII			((UINT32)0x1F << 10)  
  #define EFUSE_RGMII				((UINT32)0x01 << 9)  
  #define EFUSE_FEPHY				((UINT32)0x01 << 8) 
  #define MII_INTER_COL				((UINT32)0x01 << 7) //1:internal Col 0:external Col
  #define RMII_MODE					((UINT32)0x01 << 6) 
  #define CLK27_SEL					((UINT32)0x01 << 5)  
  #define US_20						((UINT32)0x07 << 2)  
  #define RX_PREREAD_EN				((UINT32)0x01 << 1)  
  #define TX_PREREAD_EN				((UINT32)0x01 << 0)  

/* STAR_CLK_CFG (0x3309c) */
/* =================================================================== */
  #define TXCLK_OUT_INV				((UINT32)0x01 << 31)  
  #define RXCLK_OUT_INV				((UINT32)0x01 << 30)  
  #define TXCLK_IN_INV				((UINT32)0x01 << 29)  
  #define RXCLK_IN_INV				((UINT32)0x01 << 28)
  #define MAC_SEL					((UINT32)0x03 << 26)
  	#define MAC_CLK_25M				((UINT32)0x00 << 26)
  	#define MAC_CLK_RXCLK			((UINT32)0x01 << 26)
  	#define MAC_CLK_27M				((UINT32)0x02 << 26)
  	#define MAC_CLK_TXCLK			((UINT32)0x03 << 26)
  #define MDC_SEL					((UINT32)0x03 << 24)
  	#define MDC_CLK_25M				((UINT32)0x00 << 24)
  	#define MDC_CLK_RXCLK			((UINT32)0x01 << 24)
  	#define MDC_CLK_27M				((UINT32)0x02 << 24)
  	#define MDC_CLK_TXCLK			((UINT32)0x03 << 24)
  #define CLK25_SEL					((UINT32)0x03 << 22)
  	#define CLK25_CLK_25M			((UINT32)0x00 << 22)
  	#define CLK25_CLK_RXCLK			((UINT32)0x01 << 22)
  	#define CLK25_CLK_27M			((UINT32)0x02 << 22)
  	#define CLK25_CLK_TXCLK			((UINT32)0x03 << 22)
  #define MAC_DIV					((UINT32)0x01 << 16)
    #define MAC_CLK_DIV_10			((UINT32)0x00 << 16)
  	#define MAC_CLK_DIV_1			((UINT32)0x01 << 16)
  #define MDC_INV					((UINT32)0x01 << 12)  
  #define MDC_STOP					((UINT32)0x01 << 8)  
  #define MDC_DIV					((UINT32)0xFF << 0)  
    #define MDC_CLK_DIV_10			((UINT32)0x0A << 0)
#endif

/* STAR_AHB_BURSTTYPE (0x33074) */
/* =================================================================== */
  #define TX_BURST					((UINT32)0x07 << 4) 
  	  #define TX_BURST_4			((UINT32)0x03 << 4) 
  	  #define TX_BURST_8			((UINT32)0x05 << 4) 
  	  #define TX_BURST_16			((UINT32)0x07 << 4) 
  #define RX_BURST					((UINT32)0x01 << 0)  
  	  #define RX_BURST_4			((UINT32)0x03 << 0) 
  	  #define RX_BURST_8			((UINT32)0x05 << 0) 
  	  #define RX_BURST_16			((UINT32)0x07 << 0) 
#ifdef CC_ETHERNET_6896
#define RW_INTERNAL_PHY_CTRL(base)  (base + 0x84)
  #define INTERNAL_PHY_ADDRESS_MASK    ((UINT32)0x3c)
    #define INTERNAL_PHY_ADDRESS_POS    (6)
		  
#else

/* ETH_CHKSUM_BASE (0x3d000) for Internal PHY control */
/* =================================================================== */
#define  RW_INTERNAL_PHY_CTRL           0x88
  #define INTERNAL_PHY_ADDRESS_MASK     ((UINT32)0xF8)      
  #define INTERNAL_PHY_ADDRESS_POS      (3)      
  #define INTERNAL_PHY_ENABLE_MDC_MDIO  ((UINT32)0x01 << 0)
  #define INTERNAL_PHY_ENABLE_MII_PIN   ((UINT32)0x01 << 1)
        
#define  RW_INTERNAL_PHY_CTRL1          0x80
  #define SOFT_RESET_DISABLE            ((UINT32)0x01 << 1) 

#define STAR_INTERNAL_PHY_ADDR          (0x0)

#endif

/* Internal PHY registers */
#define EXT_PHY_CTRL_5_REG              (0x1b)
    #define BLD_PS_CORR_DIS             (0x1 << 15)


enum MacInitReg
{
  MAC_CFG_INIT        	= (UINT32)(STAR_MAC_CFG_RXCKSEN | 
                                   STAR_MAC_CFG_ACPTCKSERR | 
                                   STAR_MAC_CFG_CRCSTRIP| 
                                   STAR_MAC_CFG_MAXLEN_1522 | 
                                   (STAR_MAC_CFG_IPG_MASK<<STAR_MAC_CFG_IPG_OFFSET)),   

#ifdef CC_ETHERNET_6896
		  MAC_CLK_INIT			= (UINT32)( MDC_CLK_DIV_10),
		  MAC_EXT_INIT			= (UINT32)( MRST | NRST | HRST | DMA_RESET),
		
		  MAC_EXT_INIT_RMII 	= (UINT32)(MRST | NRST | HRST ),
		
		  PDWNC_MAC_EXT_INIT	= (UINT32)(MRST | NRST | HRST | DMA_RESET),
		
		  PDWNC_MAC_EXT_INIT_RMII	= (UINT32)(MRST | NRST | HRST),
		
		  PDWNC_MAC_EXT_CFG 	= (UINT32)(MRST | NRST | HRST ),
		
		  PDWNC_MAC_EXT_CFG_RMII	= (UINT32)(MRST | NRST | HRST),
		
#else

  MAC_CLK_INIT			= (UINT32)(MAC_CLK_27M | 
                                   MDC_CLK_27M | 
                                   CLK25_CLK_27M | 
                                   MAC_CLK_DIV_1 | 
                                   MDC_STOP | 
                                   MDC_CLK_DIV_10),
	
  MAC_EXT_INIT			= (UINT32)(DRAM_PRIORITY_HIGH | 
                                   COL_SEL_EXTERNAL | 
                                   TCPIP_RST | 
                                   MRST | 
                                   NRST | 
                                   HRST | 
                                   DMA_RST | 
                                   PB_NIC_SEL_20H),

  MAC_EXT_INIT_RMII		= (UINT32)(DRAM_PRIORITY_HIGH | 
                                   COL_SEL_INTERNAL | 
                                   TCPIP_RST | 
                                   MRST | 
                                   NRST | 
                                   HRST |
                                   /* RMII_CLK_INV | */
                                   PB_NIC_SEL_20H | 
                                   RMII_MODE),

  PDWNC_MAC_EXT_INIT    = (UINT32)(DRAM_PRIORITY_HIGH | 
                                   COL_SEL_EXTERNAL | 
                                   TCPIP_RST | 
                                   MRST | 
                                   NRST | 
                                   HRST | 
                                   PB_NIC_SEL_20H),

  PDWNC_MAC_EXT_INIT_RMII   = (UINT32)(DRAM_PRIORITY_HIGH | 
                                       COL_SEL_INTERNAL | 
                                       TCPIP_RST | 
                                       MRST | 
                                       NRST | 
                                       HRST | 
                                       RMII_CLK_INV |
                                       PB_NIC_SEL_20H | 
                                       RMII_MODE),

  PDWNC_MAC_EXT_CFG     = (UINT32)(DRAM_PRIORITY_HIGH | 
                                   PDMAC_SEL_PDWNC | 
                                   COL_SEL_EXTERNAL | 
                                   TCPIP_RST | 
                                   MRST | 
                                   NRST | 
                                   HRST | 
                                   PB_NIC_SEL_20H),

  PDWNC_MAC_EXT_CFG_RMII	= (UINT32)(DRAM_PRIORITY_HIGH | 
                                       PDMAC_SEL_PDWNC | 
                                       COL_SEL_INTERNAL | 
                                       TCPIP_RST | 
                                       MRST | 
                                       NRST | 
                                       HRST | 
                                       RMII_CLK_INV | 
                                       PB_NIC_SEL_20H | 
                                       RMII_MODE),

#endif
  MAC_FILTER_INIT       = (UINT32)(0),                               

  MAC_FLOWCTRL_INIT  	= (UINT32)(STAR_FC_CFG_SEND_PAUSE_TH_DEF /* 2k */ | 
                                   STAR_FC_CFG_UCPAUSEDIS | 
                                   STAR_FC_CFG_BPEN ),                                      	

  PHY_CTRL_INIT			= (UINT32)(STAR_PHY_CTRL1_FORCETXFC | 
                                   STAR_PHY_CTRL1_FORCERXFC | 
                                   STAR_PHY_CTRL1_FORCEFULL | 
                                   STAR_PHY_CTRL1_FORCESPD_100M | 
                                   STAR_PHY_CTRL1_ANEN),

  DMA_BUSMODE_INIT      = (UINT32)(TX_BURST_16 | RX_BURST_16),      // Bus Mode

  DMA_OPMODE_INIT      	= (UINT32)(STAR_DMA_CFG_RX2BOFSTDIS | 
#ifndef CC_ETHERNET_6896
                                   STAR_DMA_CFG_RX_INTR_WDLE | 
                                   STAR_DMA_CFG_TX_INTR_WDLE | 
#endif
                                   STAR_DMA_CFG_TXSUSPEND | 
                                   STAR_DMA_CFG_RXSUSPEND),       // Operation Mode  

  DMA_RX_INT_MASK       = (UINT32)(STAR_INT_STA_RXC | 
                                   STAR_INT_STA_RXFIFOFULL | 
                                   STAR_INT_STA_RXQF),
                                     
  DMA_TX_INT_MASK       = (UINT32)(STAR_INT_STA_TXC 
#ifndef CC_ETHERNET_6896
                                   | STAR_INT_STA_TXFIFOUDRUN
#endif
),

  DMA_INT_ENABLE        = (UINT32)(STAR_INT_STA_TXC | 
                                   STAR_INT_STA_RXC | 
                                   STAR_INT_STA_RXQF | 
                                   STAR_INT_STA_RXFIFOFULL 
#ifndef CC_ETHERNET_6896
                                   | STAR_INT_STA_TXFIFOUDRUN
#endif
									),         

  DMA_INT_MASK          = (UINT32)(STAR_INT_STA_TXC | 
                                   STAR_INT_STA_RXC | 
                                   STAR_INT_STA_RXQF | 
                                   STAR_INT_STA_RXFIFOFULL 
#ifndef CC_ETHERNET_6896
                                   | STAR_INT_STA_TXFIFOUDRUN
#endif
								   ),

  DMA_INT_ENABLE_ALL   	= (UINT32)(STAR_INT_STA_TX_SKIP | 
                                   STAR_INT_STA_TXC | 
                                   STAR_INT_STA_TXQE | 
                                   STAR_INT_STA_RXC | 
                                   STAR_INT_STA_RXQF |
                                   STAR_INT_STA_MAGICPKT |
                                   STAR_INT_STA_MIBCNTHALF |
                                   STAR_INT_STA_PORTCHANGE |
                                   STAR_INT_STA_RXFIFOFULL 
#ifndef CC_ETHERNET_6896
                                   | STAR_INT_STA_TXFIFOUDRUN
#else 
                                   | STAR_INT_STA_RX_PCODE
#endif
                                   ),

  DMA_INT_CLEAR_ALL     = (UINT32)(STAR_INT_STA_TX_SKIP | 
                                   STAR_INT_STA_TXC | 
                                   STAR_INT_STA_TXQE | 
                                   STAR_INT_STA_RXC | 
                                   STAR_INT_STA_RXQF |
                                   STAR_INT_STA_MAGICPKT |
                                   STAR_INT_STA_MIBCNTHALF |
                                   STAR_INT_STA_PORTCHANGE |
                                   STAR_INT_STA_RXFIFOFULL 
#ifndef CC_ETHERNET_6896
                                   | STAR_INT_STA_TXFIFOUDRUN
#else 
                                   | STAR_INT_STA_RX_PCODE
#endif

                                   ),

};


#endif /* _STAR_REG_H_ */

