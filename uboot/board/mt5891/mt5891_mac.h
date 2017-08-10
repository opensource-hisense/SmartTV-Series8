/*-----------------------------------------------------------------------------
 * uboot/board/mt5398/mt5398_mac.h
 *
 * MT53xx Ethernet driver
 *
 * Copyright (c) 2008-2012 MediaTek Inc.
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
 *---------------------------------------------------------------------------*/

#ifndef MMAC_REG_INCLUDED
#define MMAC_REG_INCLUDED

#include <common.h>
#include <malloc.h>
#include <net.h>
#include <asm/io.h>
#include <miiphy.h>
#include "drvcust_if.h"

//#include <asm/arch/mt85xx.h>       /* get chip and board defs */

//#define MT539X_LAN8700


#define CHECKSUM_OFFLOAD
#define MT53XX_STAR_MAC
#define ETH_MAX_FRAME_SIZE  1536
#define INTERNAL_PHY 1
#define ALIGNMENT_16BYTES_TX_BUFF_TEST 0
#if (INTERNAL_PHY)
#define DEBUG_MII_PIN 0
#define USE_EXT_CLOCK_SOURCE 0
#endif

//#define PINMUX0_OFFSET			        (0xC0)
//#define PINMUX0_BASE                      (CKGEN_BASE + PINMUX0_OFFSET)  
#define IO_BASE                             (0xf0000000)
#define ETHERNET_BASE                       (IO_BASE + 0x32000)
#define CKGEN_BASE                          (IO_BASE + 0x0d000)
#define PDWNC_BASE                          (IO_BASE + 0x28000)
#define PINMUX_BASE                         (IO_BASE + 0x0d400)
#define IPLL1_BASE                          (IO_BASE + 0x0d000)
#define IPLL2_BASE                          (IO_BASE + 0x90300)


#define MAC_REG_OFFSET                      (0x0000)  
#define MMC_REG_OFFSET                      (0x0100)  
#define DMA_REG_OFFSET                      (0x0200)  
#define MAC_BASE                            (ETHERNET_BASE + MAC_REG_OFFSET)  //0x20032000
#define MMC_BASE                            (ETHERNET_BASE + MMC_REG_OFFSET) //0x20032100
#define DMA_BASE                            (ETHERNET_BASE + DMA_REG_OFFSET) //0x20032200

#define ETHERNET_LENGTH     				(0x0300)
#define MAC_LENGTH     					    (0x0100)
#define DMA_LENGTH     					    (0x0100)
#define ETH_IRQ                             (8)



#ifdef MT53XX_STAR_MAC
#define NUM_TX_DESC   4
#define NUM_RX_DESC   4
#define RX_PACKET_BUFFER_SIZE 2048    /* preallocated length for rx packets is MTU + ETHERNET_PACKET_EXTRA */
#define DMA_ALIGN_SIZE 32    /* preallocated length for rx packets is MTU + ETHERNET_PACKET_EXTRA */
#else
#define NUM_TX_DESC   2
#define NUM_RX_DESC   4
#define RX_PACKET_BUFFER_SIZE 2000    /* preallocated length for rx packets is MTU + ETHERNET_PACKET_EXTRA */
#define DMA_ALIGN_SIZE 16    /* preallocated length for rx packets is MTU + ETHERNET_PACKET_EXTRA */
#endif


#define ETHERNET_PACKET_EXTRA 18    /* preallocated length for rx packets is MTU + ETHERNET_PACKET_EXTRA */
#define ETHERNET_PACKET_COPY  250   /* max length when received data is copied to new skb */
#define CHECK_LINK_TIMER_OUT  (jiffies+(HZ*2))	/* timer wakeup time : 2 second */

#define mdelay(n)       udelay((n)*1000)



//////////////////////////////////////////////////////////
//typedef unsigned long   UINT32;
//typedef unsigned long   BOOL;
//typedef long            INT32;
//typedef unsigned char   UINT8;

// *********************************************************************
// Ethernet Registers offset
// *********************************************************************

#define ETHERNET_REG_OFFSET      	        (ETHERNET_BASE + 0x000)
#define ETHERNET_CHKSUM_BASE                (IO_BASE + 0x3D000)//shengming add for star
//#define CHECKSUM_REG_OFFSET     	        (ETHERNET_BASE + 0xA000)
#define CHECKSUM_REG_OFFSET     	        (ETHERNET_CHKSUM_BASE + 0x000)

// *********************************************************************
// Ethernet Registers map
// *********************************************************************
#define RW_PHY_CTRL0        0x00                  
  #define RW_DATA				((UINT32)0xFFFF << 16)  
  #define RW_DATA_SHIFT			(16)  
  #define RW_OK            		((UINT32)0x01 << 15)  
  #define RD_CMD           		((UINT32)0x01 << 14)  
  #define WT_CMD         		((UINT32)0x01 << 13)   
  #define PHY_REG        		((UINT32)0x1F << 8)  
  #define PHY_ADDR       		((UINT32)0x1F << 0)   
  #define PHY_REG_SHIFT        	(8)  
  #define PHY_ADDR_SHIFT      	(0)   

#define RW_PHY_CTRL1   	    0x04                  
  #define AUTO_POLL_DIS			((UINT32)0x01 << 31)  
  #define PHY_ADDR_AUTO      	((UINT32)0x1F << 24)  
  #define PHY_ADDR_AUTO_SHIFT 	(24)  
  #define SWH_CK_PWN           	((UINT32)0x01 << 23)  
  #define PHY_PORT_SEL         	((UINT32)0x01 << 22)   
  #define EXTER_MAC_SEL       	((UINT32)0x01 << 21)  
  #define INTER_PYH_PD          ((UINT32)0x01 << 19)   
  #define RGMII_PHY          	((UINT32)0x01 << 17)   
  #define REV_MII		       	((UINT32)0x01 << 16)   
  #define TXC_CHECK_EN          ((UINT32)0x01 << 14)   
  #define FORCE_FC_TX          	((UINT32)0x01 << 13)   
  #define FORCE_FC_RX          	((UINT32)0x01 << 12)   
  #define FORCE_DUPLEX          ((UINT32)0x01 << 11)   
  #define FORCE_DUPLEX_HALF     ((UINT32)0x00 << 11)  
  #define FORCE_DUPLEX_FULL     ((UINT32)0x01 << 11)  
  #define FORCE_SPEED          	((UINT32)0x03 << 9)   
    #define FORCE_SPEED_10M    	((UINT32)0x00 << 9)  
  	#define FORCE_SPEED_100M    ((UINT32)0x01 << 9)  
  	#define FORCE_SPEED_1000M   ((UINT32)0x02 << 9) 
  #define AN_EN					((UINT32)0x01 << 8)   
  #define MI_DIS		      	((UINT32)0x01 << 7)   
  #define FC_TX_ST	          	((UINT32)0x01 << 6)   
  #define RX_RX_ST	          	((UINT32)0x01 << 5)   
  #define DULPLEX_ST          	((UINT32)0x01 << 4)   
  #define SPEED_ST	          	((UINT32)0x03 << 2)   
  	#define SPEED_ST_10M        ((UINT32)0x00 << 2)  
  	#define SPEED_ST_100M       ((UINT32)0x01 << 2)  
  	#define SPEED_ST_1000M      ((UINT32)0x02 << 2) 
  #define TXC_ST		      	((UINT32)0x01 << 1)   
  #define LINK_ST          		((UINT32)0x01 << 0)   

#define RW_MAC_CFG   	    0x08                  
  #define NIC_PD     			((UINT32)0x01 << 31)  
  #define WOL	            	((UINT32)0x01 << 30)  
  #define NIC_PD_READY      	((UINT32)0x01 << 29)  
  #define RXDV_WAKEUP_EN		((UINT32)0x01 << 28)   
  #define TXPART_WAKEUP_EN		((UINT32)0x01 << 27)  
  #define TX_CKS_EN        		((UINT32)0x01 << 26)   
  #define RX_CKS_EN        		((UINT32)0x01 << 25)   
  #define ACPT_CKS_ERR   		((UINT32)0x01 << 24)   
  #define IST_EN		   		((UINT32)0x01 << 23)   
  #define VLAN_STRIPPING		((UINT32)0x01 << 22)   
  #define ACPT_CRC_ERR   		((UINT32)0x01 << 21)   
  #define CRC_STRIPPING  		((UINT32)0x01 << 20)   
  #define ACPT_LONG_PKT 		((UINT32)0x01 << 18)   
  #define MAX_LEN	   			((UINT32)0x03 << 16)   
  	#define MAX_LEN_1518		((UINT32)0x00 << 16)  
  	#define MAX_LEN_1522		((UINT32)0x01 << 16)  
  	#define MAX_LEN_1536		((UINT32)0x02 << 16)  
  #define IPG			   		((UINT32)0x1F << 10)   
  #define DO_NOT_SKIP   		((UINT32)0x01 << 9)   
  #define FAST_RETRY   			((UINT32)0x01 << 8)   
  #define TX_VLAN_TAG_AUTO_PARSE   	((UINT32)0x01 << 0)   

#define RW_FC_CFG           0x0C                  
  #define SEND_PAUSE_TH     	((UINT32)0xFFF << 16)  
  #define SEND_PAUSE_TH_2K     	((UINT32)0x800 << 16)  
  #define COLCNT_CLR_MODE		((UINT32)0x01 << 9)  
  #define UC_PAUSE_DIS  		((UINT32)0x01 << 8)   
  #define BP_ENABLE   			((UINT32)0x01 << 7)   
  #define REV   				((UINT32)0x01 << 6)   
  #define MAX_BP_COL_EN   		((UINT32)0x01 << 5)   
  #define MAX_BP_COL_CNT   		((UINT32)0x1F << 0)   

#define RW_ARL_CFG   	    0x10                  
  #define PRI_TAG_FILTER     	((UINT32)0x01 << 6)  
  #define VLAN_UTAG_FILTER		((UINT32)0x01 << 5)  
  #define MISC_MODE  			((UINT32)0x01 << 4)   
  #define MY_MAC_ONLY   		((UINT32)0x01 << 3)   
  #define CPU_LEARN_DIS			((UINT32)0x01 << 2)   
  #define REV_MC_FILTER   		((UINT32)0x01 << 1)   
  #define HASH_ALG		   		((UINT32)0x01 << 0)   

#define RW_MY_MAC_H   	    0x14                  
#define RW_MY_MAC_L  	    0x18                  

#define RW_HASH_CTRL   	    0x1C                  
  #define HASH_TABLE_BIST_START     ((UINT32)0x01 << 31)  
  #define HASH_TABLE_BIST_DONE     	((UINT32)0x01 << 17)  
  #define HASH_TABLE_BIST_OK		((UINT32)0x01 << 16)  
  #define COMMAND_START	  			((UINT32)0x01 << 14)   
  #define HASH_ACCESS_COMMAND		((UINT32)0x01 << 13)   
  #define HASH_BIT_DATA				((UINT32)0x01 << 12)   
  #define HASH_BIT_ADDRESS   		((UINT32)0x1F << 0)   

#define RW_VLAN_CTRL   	    0x20                  
  #define MY_VID0_3_EN			     	((UINT32)0x0F << 0)  

#define RW_VLAN_ID_0_1	    0x24                 
  #define MY_VID1			     		((UINT32)0xFFF << 16)  
  #define MY_VID0			     		((UINT32)0xFFF << 0)  

#define RW_VLAN_ID_2_3	    0x28                 
  #define MY_VID3			     		((UINT32)0xFFF << 16)  
  #define MY_VID2			     		((UINT32)0xFFF << 0)  

#define RW_DMA_CFG   	    0x30                  
  #define RX_OFFSET_2B_DIS  	((UINT32)0x01 << 16)   
  #define TX_POLL_PERIOD		((UINT32)0x03 << 6)  
  #define TX_POLL_EN  			((UINT32)0x01 << 5)   
  #define TX_SUSPEND   			((UINT32)0x01 << 4)   
  #define RX_POLL_PERIOD		((UINT32)0x03 << 2)  
  #define RX_POLL_EN  			((UINT32)0x01 << 1)   
  #define RX_SUSPEND   			((UINT32)0x01 << 0)   

#define RW_TX_DMA_CTRL   	0x34
  #define TX_EN                 ((UINT32)0x01 << 3)
  #define TX_RESUME				((UINT32)0x01 << 2)
  #define TX_STOP  				((UINT32)0x01 << 1)
  #define TX_START  			((UINT32)0x01 << 0)

#define RW_RX_DMA_CTRL   	0x38                  
  #define RX_EN  				((UINT32)0x01 << 3)
  #define RX_RESUME				((UINT32)0x01 << 2)
  #define RX_STOP  				((UINT32)0x01 << 1)
  #define RX_START  			((UINT32)0x01 << 0)  

#define RW_TX_DPTR  		0x3C                  
  #define TXSD  				((UINT32)0xFFFFFFF << 4)  

#define RW_RX_DPTR  		0x40                  
  #define RXSD  				((UINT32)0xFFFFFFF << 4)  

#define RW_TX_BASE_ADDR		0x44                 
  #define TX_BASE				((UINT32)0xFFFFFFF << 4)  

#define RW_RX_BASE_ADDR		0x48                 
  #define RX_BASE				((UINT32)0xFFFFFFF << 4)  

#define RW_DLY_INT_CFG		0x50 
  #define STA_RX_PCODE			((UINT32)0x01 << 10)
  #define TX_SKIP				((UINT32)0x01 << 9)  
  #define TNTC					((UINT32)0x01 << 8)  
  #define TNQE					((UINT32)0x01 << 7)  
  #define FNRC					((UINT32)0x01 << 6)  
  #define FNQF					((UINT32)0x01 << 5)  
  #define MAGIC_PKT_REC			((UINT32)0x01 << 4)  
  #define MIB_COUNTER_TH		((UINT32)0x01 << 3)  
  #define PORT_STATUS_CFG		((UINT32)0x01 << 2)  
  #define RX_FIFO_FULL			((UINT32)0x01 << 1)    

#define RW_INT_MASK			0x54               
  #define INT_MASK					((UINT32)0x1FF << 0)  
  #define TX_SKIP_INT_MASK		 	((UINT32)0x01 << 9)  
  #define TNTC_INT_MASK				((UINT32)0x01 << 8)  
  #define TNQE_INT_MASK				((UINT32)0x01 << 7)  
  #define FNRC_INT_MASK				((UINT32)0x01 << 6)  
  #define FNQF_INT_MASK				((UINT32)0x01 << 5)  
  #define MAGIC_PAK_REC_INT_MASK	((UINT32)0x01 << 4)  
  #define MIB_COUNTER_TH_MASK		((UINT32)0x01 << 3)  
  #define PORT_STATUS_CFG_MASK		((UINT32)0x01 << 2)  
  #define RX_FIFO_FULL_MASK			((UINT32)0x01 << 1)  
  #define TX_FIFO_UNDER_RUN_MASK	((UINT32)0x01 << 0)  

#define RW_TEST0			0x58               
  #define ALWAYS_TXC_OUT			((UINT32)0x01 << 16)  
  #define TX_SKEW					((UINT32)0x3F << 8)  
  #define RX_SKEW					((UINT32)0x3F << 0)  

#define RW_TEST1			0x5C               
  #define RESTART_HASH_MBIST		((UINT32)0x01 << 31)  
  #define DBG_SEL					((UINT32)0x03 << 29)  
  #define HANDSHAKE_MODE			((UINT32)0x01 << 28)  
  #define NO_PATCH_READ_C_BIT_TX	((UINT32)0x01 << 27)  
  #define NO_PATCH_READ_C_BIT_RX	((UINT32)0x01 << 26)  
  #define INT_LB_MII				((UINT32)0x01 << 18)  
  #define RX_FIFO_FREE_BYTE			((UINT32)0x1FFF << 0)  
  #define TEST1_EXTEND_RETRY        ((UINT32)0x01 << 20)

#define RW_EXTEND_CFG		0x60               
  #define SEND_PAUSE_RLS			((UINT32)0xFFF << 16)  
  #define SEND_PAUSE_RLS_1K			((UINT32)0x400 << 16)  

#define RW_ETHPHY			0x6C
  #define CFG_INT_ETHPHY            ((UINT32)0x01 << 9)
  #define ADC_IN_MUX_EN				((UINT32)0x01 << 8)  
  #define SWC_MII_MODE				((UINT32)0x01 << 7)  
  #define EXT_MDC_MODE				((UINT32)0x01 << 6)  
  #define AFE_TEST_MODE				((UINT32)0x01 << 5)  
  #define AFE_TEST_OE				((UINT32)0x01 << 4)  
  #define MII_PAD_OE				((UINT32)0x01 << 3)  
  #define FRC_SMI_ENB				((UINT32)0x01 << 2)  
  #define PIFCKLBYP					((UINT32)0x01 << 1)  
  #define PLLCLKBYP					((UINT32)0x01 << 0)  

#define RW_DMA_STATUS		0x70               
  #define TN_BIT					((UINT32)0x01 << 16)  
  #define TN_E_BIT					((UINT32)0x01 << 15)  
  #define TN_L_BIT					((UINT32)0x01 << 14)  
  #define TN_I_BIT					((UINT32)0x01 << 13)  
  #define TN_VTG_BIT				((UINT32)0x01 << 12)  
  #define TN_DMA_STS				((UINT32)0x0F << 8)  
  #define FN_C_BIT					((UINT32)0x01 << 6)  
  #define FN_E_BIT					((UINT32)0x01 << 5)  
  #define FN_DMA_STS				((UINT32)0x1F << 0)  

#define RW_BURST_TYPE		0x74               
  #define TX_BURST					((UINT32)0x07 << 4) 
  	  #define TX_BURST_4			((UINT32)0x03 << 4) 
  	  #define TX_BURST_8			((UINT32)0x05 << 4) 
  	  #define TX_BURST_16			((UINT32)0x07 << 4) 
  #define RX_BURST					((UINT32)0x01 << 0)  
  	  #define RX_BURST_4			((UINT32)0x03 << 0) 
  	  #define RX_BURST_8			((UINT32)0x05 << 0) 
  	  #define RX_BURST_16			((UINT32)0x07 << 0) 

#define EEE_CTRL            0x78
  #define LPI_MODE_EB               ((UINT32)0x01 << 0)
          
#define EEE_SLEEP_TIMER     0x7c
          
#define EEE_WAKEUP_TIMER    0x80
          
#define ETHPHY_CONFIG1      0x84
        
#define ETHPHY_CONFIG2      0x88
          
#define RW_ARB_MON2         0x8C
          
          
#define ETHPHY_CLOCK_CONTROL    0x90

#define ETHSYS_CONFIG       0x94
  #define INT_PHY_SEL               ((UINT32)0x01 << 3)
          
#define MAC_MODE_CONFIG     0x98
  #define BIG_ENDIAN				((UINT32)0x01 << 0)
          
#define SW_RESET_CONTROL    0x9c
  #define PHY_RSTN					((UINT32)0x01 << 4)
  #define MRST						((UINT32)0x01 << 3)
  #define NRST						((UINT32)0x01 << 2)
  #define HRST						((UINT32)0x01 << 1)
  #define DMA_RESET				    ((UINT32)0x01 << 0)
          
#define MAC_CLOCK_CONFIG    0xac
  #define TXCLK_OUT_INV				((UINT32)0x01 << 19)
  #define RXCLK_OUT_INV				((UINT32)0x01 << 18)
  #define TXCLK_IN_INV				((UINT32)0x01 << 17)
  #define RXCLK_IN_INV				((UINT32)0x01 << 16)
  #define MDC_INV					((UINT32)0x01 << 12)
  #define MDC_NEG_LAT				((UINT32)0x01 << 8)
  #define MDC_DIV					((UINT32)0xFF << 0)
  #define MDC_CLK_DIV_10			((UINT32)0x0A << 0)


#define RW_C_RXOKPKT		0x100

#define RW_C_RXOKBYTE		0x104

#define RW_C_RXRUNT			0x108

#define RW_C_RXLONG			0x10C

#define RW_C_RXDROP			0x110

#define RW_C_RXCRC			0x114

#define RW_C_RXARLDROP		0x118

#define RW_C_RXVLANDROP		0x11C

#define RW_C_RXCSERR		0x120

#define RW_C_RXPAUSE		0x124

#define RW_C_TXOKPKT		0x128

#define RW_C_TXOKBYTE		0x12C

#define RW_C_TXPAUSE		0x130

#define RW_C_TXRETRY    	0x134      

#define RW_C_TXSKIP	    	0x138       

#define RW_MY_MAC1_H		0x140
	#define ADDR_EN           ((UINT32)0x01 << 31)   /* address filter module is enabled */

#define RW_MY_MAC1_L		0x144

#define ETH_UP_CFG          0x300
    #define CPUCK_BY_8032           ((UINT32)0x01 << 16)   
/////////////////////////////////////////////////////////

//////////////////////////////////////////////////
//Check SUM Register, base 0x3d000
#define RW_CHKSUM_ADDR   			0x00     //	UDP/TCP/IP Checksum Start Address										
  #define CHECKSUM_SADR_MASK 	 	((UINT32)0xFFFFFFFF)	  //Checksum Start Address (Byte Address)

#define RW_CHKSUM_LENGTH  			0x04
  #define CHECKSUM_LENGTH_MASK 	 	((UINT32)0xFFFFFFFF)	  // Checksum Length (Byte)										

#define RW_CHKSUM_CTRL     			0x08
  #define  CHKSUM_INT 				((UINT32)0x01 << 31) 
  #define  OUT_INV					((UINT32)0x01 << 7)  // 1's complement of UDP/TCP/IP Checksum Result Data
  #define  OUT_REV					((UINT32)0x01 << 6)  //Reverse Byte of Checksum Result Data
  #define  BYTE_REV         		((UINT32)0x01 << 5)  //Reverse Byte of Checksum Input Data (Word)
  #define  DW_REV	      			((UINT32)0x01 << 4)  //Reverse Byte of Checksum Input Data (Double Word)	
  #define  CHKSUM_INT_CLR			((UINT32)0x01 << 2)  //Checksum Interrupt clear
  #define  INTR_EN					((UINT32)0x01 << 1) //Checksum Interrupt Enable
  #define  CHECKSUM_EN				((UINT32)0x01 << 0)  //Checksum Enable. When Checksum is finish, this bit will be clear to 0.	

#define RW_CHKSUM_RESULT     		0x0C
  #define  CHECKSUM_RESULT_MASK		((UINT32)0x0FFFF)  //Checksum Result Data Mask

#define RW_CHKSUM_DMA_CTRL     		0x10
  #define  CHECKSUM_DMA_IDLE_TIME_MASK	((UINT32)0xFFFF0000)  //Checksum DMA Idle Time (dram clock cycle)
  #define  CHECKSUM_DMA_ALE_NUM_MASK	((UINT32)0x0000FFFF)  //Checksum Burst Length (64 bytes). If these registers set to 0, burst length is CHECKSUM_LENGTH.

#if 1 //RX Check SUm
#define RW_RX_CHKSUM_ADDR   		0x14     //	RX UDP/TCP/IP Checksum Start Address										
  #define RX_CHECKSUM_SADR_MASK 	 	((UINT32)0xFFFFFFFF)	  //Checksum Start Address (Byte Address)

#define RW_RX_CHKSUM_LENGTH  		0x18
  #define RX_CHECKSUM_LENGTH_MASK 	 	((UINT32)0xFFFFFFFF)	  // Checksum Length (Byte)										

#define RW_RX_CHKSUM_CTRL     		0x1c
  #define  RX_CHKSUM_INT 				((UINT32)0x01 << 31) 
  #define  RX_OUT_INV					((UINT32)0x01 << 7)  // 1's complement of UDP/TCP/IP Checksum Result Data
  #define  RX_OUT_REV					((UINT32)0x01 << 6)  //Reverse Byte of Checksum Result Data
  #define  RX_BYTE_REV         		    ((UINT32)0x01 << 5)  //Reverse Byte of Checksum Input Data (Word)
  #define  RX_DW_REV	      			((UINT32)0x01 << 4)  //Reverse Byte of Checksum Input Data (Double Word)	
  #define  RX_CHKSUM_INT_CLR			((UINT32)0x01 << 2)  //Checksum Interrupt clear
  #define  RX_INTR_EN					((UINT32)0x01 << 1) //Checksum Interrupt Enable
  #define  RX_CHECKSUM_EN				((UINT32)0x01 << 0)  //Checksum Enable. When Checksum is finish, this bit will be clear to 0.	

#define RW_RX_CHKSUM_RESULT     	0x20
  #define  RX_CHECKSUM_RESULT_MASK		((UINT32)0x0FFFF)  //Checksum Result Data Mask
#endif

/*  PDWNC register base
    Note: pdwnc_base starts from PDWNC_BASE + 0x100 */
#define PDWNC_BASE_CFG_OFFSET           (0x100)
/* Note: uP configuration register at 0x24188 */
#define PDWNC_REG_UP_CONF_OFFSET        (0x188)
#define UP_CONF_ETEN_EN                 (1U << 28)


// *********************************************************************
// Ethernet Init setting
// *********************************************************************
enum MacInitReg
{
  MAC_CFG_INIT        	= (UINT32)(RX_CKS_EN | ACPT_CKS_ERR | CRC_STRIPPING | MAX_LEN_1522 | IPG),   
  MAC_CLK_INIT          = (UINT32)( MDC_CLK_DIV_10),
  MAC_EXT_INIT          = (UINT32)( MRST | NRST | HRST | DMA_RESET),
  MAC_EXT_INIT_RMII     = (UINT32)(MRST | NRST | HRST ),
  PDWNC_MAC_EXT_INIT    = (UINT32)(MRST | NRST | HRST | DMA_RESET),
  PDWNC_MAC_EXT_INIT_RMII   = (UINT32)(MRST | NRST | HRST),
  PDWNC_MAC_EXT_CFG     = (UINT32)(MRST | NRST | HRST ),
  PDWNC_MAC_EXT_CFG_RMII    = (UINT32)(MRST | NRST | HRST),
  MAC_FILTER_INIT       = (UINT32)(0),                               
  MAC_FLOWCTRL_INIT  	= (UINT32)(SEND_PAUSE_TH_2K | UC_PAUSE_DIS | BP_ENABLE ),                                      	
  PHY_CTRL_INIT			= (UINT32)(FORCE_FC_TX | FORCE_FC_RX | FORCE_DUPLEX | FORCE_SPEED_100M | AN_EN),
  DMA_BUSMODE_INIT      = (UINT32)(TX_BURST_16 | RX_BURST_16),               	// Bus Mode
  DMA_OPMODE_INIT      	= (UINT32)(RX_OFFSET_2B_DIS |  TX_SUSPEND | RX_SUSPEND),       // Operation Mode  
  DMA_RX_INT_MASK       = (UINT32)(FNRC | RX_FIFO_FULL | FNQF),              
  DMA_TX_INT_MASK       = (UINT32)(TNTC ),
  DMA_INT_ENABLE        = (UINT32)(TNTC | FNRC | FNQF | RX_FIFO_FULL ),         
  DMA_INT_MASK          = (UINT32)(TNTC | FNRC | FNQF | RX_FIFO_FULL ),
  DMA_INT_ENABLE_ALL   	= (UINT32)(TX_SKIP | TNTC | TNQE | FNRC | FNQF |MAGIC_PKT_REC |MIB_COUNTER_TH |PORT_STATUS_CFG |RX_FIFO_FULL ),
  DMA_INT_CLEAR_ALL     = (UINT32)(TX_SKIP | TNTC | TNQE | FNRC | FNQF |MAGIC_PKT_REC |MIB_COUNTER_TH |PORT_STATUS_CFG |RX_FIFO_FULL )
};

#define  MAC_EXT_CFG	(BIG_ENDIAN | DRAM_PRIORITY | MRST | NRST | PRST | HRST | PB_NIC_SEL_20H)

#define  MAC_CLK_CFG	(CLK_27M | MAC_CLK_DIV_1 | MDC_CLK_DIV_10) 	

// *********************************************************************
// Ethernet Descriptor Registers map
// *********************************************************************
#define RX_OWN          ((UINT32)0x01 << 31)  


#define IO_WRITE(addr, val) (*(volatile unsigned long *)(addr) = (val))
#define IO_READ(addr) (*(volatile unsigned long *)(addr))

// *********************************************************************
// Ethernet Macros
// *********************************************************************
#define vWriteMAC(dAddr, dVal)  IO_WRITE((ETHERNET_REG_OFFSET+dAddr), (dVal))
#define dReadMAC(dAddr)         IO_READ((ETHERNET_REG_OFFSET+dAddr))
#define SetBitMAC(Reg, Bit)     vWriteMAC(Reg, (dReadMAC(Reg) & (~(Bit))) | (Bit))
#define ClrBitMAC(Reg, Bit)     vWriteMAC(Reg, dReadMAC(Reg) & (~(Bit)))
#define SetBit(Addr, Bit)       IO_WRITE(Addr, (IO_READ(Addr) & (~(Bit))) | (Bit))
#define ClrBit(Addr, Bit)       IO_WRITE(Addr, IO_READ(Addr) & (~(Bit)))

#define vWriteChkSum(dAddr, dVal)  IO_WRITE((CHECKSUM_REG_OFFSET+dAddr), (dVal))
#define dReadChkSum(dAddr)         IO_READ((CHECKSUM_REG_OFFSET+dAddr))
#define SetBitChkSum(Reg, Bit)     vWriteChkSum(Reg, (dReadChkSum(Reg) & (~(Bit))) | (Bit))
#define ClrBitChkSum(Reg, Bit)     vWriteChkSum(Reg, dReadChkSum(Reg) & (~(Bit)))

/****************************************************************************
** MII PHY register definitions
****************************************************************************/
#define PHY_DEV_ADDR            (0x01)    // Address 1

// MII PHY register 
#define	MII_PHY_CTRL_REG        0x00					/* Basic Control */
  #define SW_RESET              ((unsigned)0x01 << 15)  
  #define LOOKBACK_EN           ((unsigned)0x01 << 14)  
  #define SPEED_100             ((unsigned)0x01 << 13)  
  #define AN_ENABLE             ((unsigned)0x01 << 12)  
  #define PWR_DOWN              ((unsigned)0x01 << 11)  
  #define RESTART_AN            ((unsigned)0x01 << 9)  
  #define DUPLEX_FULL           ((unsigned)0x01 << 8)  
  
#define	MII_PHY_STATUS_REG      0x01					/* Basic Status */
  #define AN_COMPLT             ((unsigned)0x01 << 5)  
  #define LINK                  ((unsigned)0x01 << 2)  

#define	MII_PHY_ID0_REG         0x02					/* PHY Identifier 1 */
#define	MII_PHY_ID1_REG         0x03					/* PHY Identifier 2 */
#define	MII_PHY_ANAR_REG        0x04					/* Auto Negotiation Advertisement */
#define	MII_PHY_ANLPAR_REG      0x05					/* Auto Negotiation Link Partner Ability */
  #define PARTNER_100BASE_FULL  ((unsigned)0x01 << 8)  
  #define PARTNER_100BASE       ((unsigned)0x01 << 7)  
  #define PARTNER_10BASE_FULL   ((unsigned)0x01 << 6)  
  #define PARTNER_10BASE        ((unsigned)0x01 << 5)  

#define	MII_PHY_ANER_REG        0x06					/* Auto Negotiation Expansion */

/**
 * @brief structure for Tx descriptor Ring
 */
typedef struct txDesc_s	/* Tx Ring */
{
    u32 ctrlLen;	/* Tx control and length */
    u32 buffer;		/* Tx segment data pointer */
	u32 vtag;
	u32 reserve;	/* Tx pointer for external management usage */
} TxDesc;

////TxDesc->ctrlLen:
#define TX_COWN 				(1 << 31)	/* Tx descriptor Own bit; 1: CPU own */
#define TX_EOR					(1 << 30)	/* End of Tx descriptor ring */
#define TX_FS					(1 << 29)	/* First Segment descriptor */
#define TX_LS					(1 << 28)	/* Last Segment descriptor */
#define TX_INT					(1 << 27)	/* Tx complete interrupt enable (when set, DMA generate interrupt after tx sending out pkt) */
#define TX_INSV 				(1 << 26)	/* Insert VLAN Tag in the following word (in tdes2) */
#define TX_ICO					(1 << 25)	/* Enable IP checksum generation offload */
#define TX_UCO					(1 << 24)	/* Enable UDP checksum generation offload */
#define TX_TCO					(1 << 23)	/* Enable TCP checksum generation offload */
#define TX_LEN_MASK 			(0xffff)	/* Tx Segment Data length */
#define TX_LEN_OFFSET			(0)

//TxDesc->vtag
#define TX_EPID_MASK			(0xffff)	/* VLAN Tag EPID */
#define TX_EPID_OFFSET			(16)
#define TX_PRI_MASK 			(0x7)		/* VLNA Tag Priority */
#define TX_PRI_OFFSET			(13)
#define TX_CFI					(1 << 12)	/* VLAN Tag CFI (Canonical Format Indicator) */
#define TX_VID_MASK 			(0xfff) 	/* VLAN Tag VID */
#define TX_VID_OFFSET			(0)


typedef struct rxDesc_s	/* Rx Ring */
{
    u32 ctrlLen;	/* Rx control and length */
    u32 buffer;		/* RX segment data pointer */
	u32 vtag;
	u32 reserve;	/* Rx pointer for external management usage */
} RxDesc;

//RxDesc->ctrlLen
#define RX_COWN 				(1 << 31)	/* RX descriptor Own bit; 1: CPU own */
#define RX_EOR					(1 << 30)	/* End of Rx descriptor ring */
#define RX_FS					(1 << 29)	/* First Segment descriptor */
#define RX_LS					(1 << 28)	/* Last Segment descriptor */
#define RX_OSIZE				(1 << 25)	/* Rx packet is oversize */
#define RX_CRCERR				(1 << 24)	/* Rx packet is CRC Error */
#define RX_RMC					(1 << 23)	/* Rx packet DMAC is Reserved Multicast Address */
#define RX_HHIT 				(1 << 22)	/* Rx packet DMAC is hit in hash table */
#define RX_MYMAC				(1 << 21)	/* Rx packet DMAC is My_MAC */
#define RX_VTAG 				(1 << 20)	/* VLAN Tagged int the following word */
#define RX_PROT_MASK			(0x3)
#define RX_PROT_OFFSET			(18)
	#define RX_PROT_IP			(0x0)		/* Protocol: IPV4 */
	#define RX_PROT_UDP 		(0x1)		/* Protocol: UDP */
	#define RX_PROT_TCP 		(0x2)		/* Protocol: TCP */
	#define RX_PROT_OTHERS		(0x3)		/* Protocol: Others */
#define RX_IPF					(1 << 17)	/* IP checksum fail (meaningful when PROT is IPV4) */
#define RX_L4F					(1 << 16)	/* Layer-4 checksum fail (meaningful when PROT is UDP or TCP) */
#define RX_LEN_MASK 			(0xffff)	/* Segment Data length(FS=0) / Whole Packet Length(FS=1) */
#define RX_LEN_OFFSET			(0)

///RxDesc->vtag
#define RX_EPID_MASK			(0xffff)	/* VLAN Tag EPID */
#define RX_EPID_OFFSET			(16)
#define RX_PRI_MASK 			(0x7)		/* VLAN Tag Priority */
#define RX_PRI_OFFSET			(13)
#define RX_CFI					(1 << 12)	/* VLAN Tag CFI (Canonical Format Indicator) */
#define RX_VID_MASK 			(0xfff) 	/* VLAN Tag VID */
#define RX_VID_OFFSET			(0)

/////////////////////////////////////////

typedef struct MmacStruct       /* Mmac device data */
{
  u32 configBase;   /* base address of Config registers */
  u32 macBase;      /* base address of MAC registers */
  u32 dmaBase;      /* base address of DMA registers */
  u32 phyAddr;      /* PHY device address on MII interface */
  TxDesc *txdesc;      /* start of TX descriptors ring */
  RxDesc *rxdesc;      /* start of RX descriptors ring */

  u32 txRingSize;
  u32 rxRingSize;
  u32 txWrIdx;			  /* Head of Tx descriptor (least sent) */
  u32 rxReadIdx;			  /* Head of Rx descriptor (least sent) */


  int advertising;
  int full_duplex;          /* interface is in full-duplex mode */
  int speed_100;    /* interface operates on high speed (100Mb/s) */
  int speed_10;   /* interface operates on high speed (10Mb/s) */
  int link_status;
//  struct net_device_stats stats;
} Mmac;

typedef struct PrivateStruct  /* Driver private data */
{
//  Mmac mmac;            /* mmac device internal data */
//  DmaDesc  desc[512];         /* DMA descriptors - 128 rx and 128 tx */
  TxDesc  tdesc[NUM_TX_DESC];         /* DMA descriptors - 2 rx and 2 tx */
  RxDesc  rdesc[NUM_RX_DESC];  
          
  Mmac mmac;            /* mmac device internal data */
  //struct mii_if_info mii;
  //struct timer_list	timer;
} Private;


#define StarDmaRxValid(ctrlLen)			(((ctrlLen & RX_FS) != 0) && ((ctrlLen & RX_LS) != 0) && ((ctrlLen & RX_CRCERR) == 0) && ((ctrlLen & RX_OSIZE) == 0))
#define StarDmaRxCrcErr(ctrlLen)		((ctrlLen & RX_CRCERR) ? 1 : 0)
#define StarDmaRxOverSize(ctrlLen)		((ctrlLen & RX_OSIZE) ? 1 : 0)
#define StarDmaRxProtocolIP(ctrlLen)	(((ctrlLen >> RX_PROT_OFFSET) & RX_PROT_MASK) != RX_PROT_OTHERS)
#define StarDmaRxProtocolTCP(ctrlLen)	(((ctrlLen >> RX_PROT_OFFSET) & RX_PROT_MASK) == RX_PROT_TCP)
#define StarDmaRxProtocolUDP(ctrlLen)	(((ctrlLen >> RX_PROT_OFFSET) & RX_PROT_MASK) == RX_PROT_UDP)
#define StarDmaRxIPCksumErr(ctrlLen)	(ctrlLen & RX_IPF)
#define StarDmaRxL4CksumErr(ctrlLen)	(ctrlLen & RX_L4F)

#define StarDmaRxLength(ctrlLen)		((ctrlLen >> RX_LEN_OFFSET) & RX_LEN_MASK)
#define StarDmaTxLength(ctrlLen)		((ctrlLen >> TX_LEN_OFFSET) & TX_LEN_MASK)

#define DescTxDma(desc)					((((desc)->ctrlLen) & TX_COWN) ? 0 : 1)
#define DescRxDma(desc)					((((desc)->ctrlLen) & RX_COWN) ? 0 : 1)
#define DescTxLast(desc)				((((desc)->ctrlLen) & TX_EOR) ? 1 : 0)
#define DescRxLast(desc)				((((desc)->ctrlLen) & RX_EOR) ? 1 : 0)
#define DescTxEmpty(desc)				(((desc)->buffer == 0) && \
                                         (((desc)->ctrlLen & ~TX_EOR) == TX_COWN) && \
                                         ((desc)->vtag == 0) && ((desc)->reserve == 0))
                                         
#define DescRxEmpty(desc)				(((desc)->buffer == 0) && \
                                         (((desc)->ctrlLen & ~RX_EOR) == RX_COWN) && \
                                         ((desc)->vtag == 0) && ((desc)->reserve == 0))

///////////////////////////////////////////////////////////////////////




#define TR mmac_trace
#define TRACE
#ifdef TRACE
  //static int debug = 0;
  #define mmac_trace(fmt... )  printf(fmt)
#else
  #define mmac_trace(fmt... ) 
  //void mmac_trace() {}
#endif
#ifndef MT53XX_STAR_MAC

static u32 __inline__ MmacReadMacReg( Mmac *Dev, u32 Reg )
{
  u32 data = __raw_readl(Dev->macBase+Reg);
//  TR( "MmacReadMacReg(%02x)=%08x\n", Reg, data );
  return data;
}

static void __inline__ MmacWriteMacReg( Mmac *Dev, u32 Reg, u32 Data )
{
//  TR( "MmacWriteMacReg(%02x,%08x)\n", Reg, Data );
  __raw_writel(Data,Dev->macBase+Reg);
}

static void __inline__ MmacSetMacReg( Mmac *Dev, u32 Reg, u32 Data )
{
  u32 addr = Dev->macBase+Reg;
  u32 data = __raw_readl(addr);
  data |= Data;
//  TR( "MmacSetMacReg(%02x,%08x)=%08x\n", Reg, Data, data );
  __raw_writel(data,addr);
}

static void __inline__ MmacClearMacReg( Mmac *Dev, u32 Reg, u32 Data )
{
  u32 addr = Dev->macBase+Reg;
  u32 data = __raw_readl(addr);
  data &= ~Data;
//  TR(  "MmacClearMacReg(%02x,%08x)=%08x\n", Reg, Data, data );
  __raw_writel(data,addr);
}

static u32 __inline__ MmacReadDmaReg( Mmac *Dev, u32 Reg )
{
  u32 data = __raw_readl(Dev->dmaBase+Reg);
//  TR(  "MmacReadDmaReg(%02x)=%08x\n", Reg, data );
  return data;
}

static void __inline__ MmacWriteDmaReg( Mmac *Dev, u32 Reg, u32 Data )
{
//  TR(  "MmacWriteDmaReg(%02x,%08x)\n", Reg, Data );
  __raw_writel(Data,Dev->dmaBase+Reg);
}

static void __inline__ MmacSetDmaReg( Mmac *Dev, u32 Reg, u32 Data )
{
  u32 addr = Dev->dmaBase+Reg;
  u32 data = __raw_readl(addr);
  data |= Data;
//  TR(  "MmacSetDmaReg(%02x,%08x)=%08x\n", Reg, Data, data );
  __raw_writel(data,addr);
}

static void __inline__ MmacClearDmaReg( Mmac *Dev, u32 Reg, u32 Data )
{
  u32 addr = Dev->dmaBase+Reg;
  u32 data = __raw_readl(addr);
  data &= ~Data;
//  TR(  "MmacClearDmaReg(%02x,%08x)=%08x\n", Reg, Data, data );
  __raw_writel(data,addr);
}

#endif

//from dev.h ++

/**********************************************************
 * board cfg related functions
 **********************************************************/
void hwcfgsetup(void);
void StarNICPdSet( u32 val);
void MmacDmaIntEnable( Mmac *Dev );
void MmacDmaIntDisable( Mmac *Dev );
void MmacDmaRxStop( Mmac *Dev );
void MmacDmaTxStop( Mmac *Dev );
int iPhyCtrlInit(u32 enable);

extern void HalFlushInvalidateDCache(void);
extern void _FlushDCache(void);

#ifndef MT85XX_STAR_MAC
void hw_reset(void); //no
#endif

/**********************************************************
 * Common functions
 **********************************************************/
#ifndef MT53XX_STAR_MAC

int MmacInit        /* Initialise device data */
(
  Mmac *Dev,          /* Device structure, must be allocated by caller */
  u32 ConfigBase,         /* Base address of Configuration registers */
  u32 MacBase,            /* Base address of MAC registers */
  u32 DmaBase,            /* Base address of DMA registers */
  u32 PhyAddr             /* PHY device address */
);

/**********************************************************
 * MAC module functions
 **********************************************************/

int MmacMacInit   /* Initialize MAC module  - set MAC and broadcast adresses, set filtering mode */
(
  Mmac *Dev,        /* Device */
  u8 Addr[6],          /* MAC address */
  u8 Broadcast[6]      /* Broadcast address */
);



u16 MmacMiiRead   /* Read MII register */
(
  Mmac *Dev,        /* Device */
  u8 Reg                /* MII register */
);

void MmacMiiWrite /* Write MII register */
(
  Mmac *Dev,        /* Device */
  u8 Reg,               /* MII register */
  u16 Data              /* Data to write */
);
#endif

/**********************************************************
 * MII functions
 **********************************************************/
int check_link (struct eth_device *dev, Mmac *mmac);
void check_media(struct eth_device *dev, Private *pr, Mmac *mmac);

#ifndef MT53XX_STAR_MAC


/**********************************************************
 * DMA engine functions
 **********************************************************/
void   MmacRxDescInit(struct eth_device *dev, Private *pr, Mmac *mmac );
void DescInit( DmaDesc *Desc, int EndOfRing );

int MmacDmaInit   /* Initialize DMA engine  - setup descriptor rings and control registers */
(
  Mmac *Dev,        /* Device */
  void *Buffer,         /* Buffer for DMA descriptors */
  u32 Size              /* Buffer length */
);

int MmacDmaRxSet  /* Set rx descriptor Desc and pass ownership to DMA engine */
(                     /* returns descriptor number if ok, -1 if next descriptor is busy (owned by DMA) */
  Mmac *Dev,        /* Device */
  u32   Buffer1,        /* First buffer address to set to descriptor */
  u32   Length1,        /* First buffer length */
  u32   Data1           /* Driver's data to store into descriptor */
);

int MmacDmaTxSet  /* Set tx descriptor Desc and pass ownership to DMA engine */
(                     /* returns descriptor number if ok, -1 if next descriptor is busy (owned by DMA) */
  Mmac *Dev,        /* Device */
  u32   Buffer1,        /* First buffer address to set to descriptor */
  u32   Length1,        /* First buffer length */
  u32   Data1           /* Driver's data to store into descriptor */
);

int MmacDmaRxGet /* take ownership of the rx descriptor Desc and clear it, returns old data */
(                    /* returns descriptor number if ok, -1 if next descriptor is busy (owned by DMA) */
  Mmac *Dev,        /* Device */
  u32  *Status,         /* Descriptor status */
  u32  *Buffer1,        /* First buffer address to set to descriptor */
  u32  *Length1,        /* First buffer length */
  u32  *Data1           /* Driver's data to store into descriptor */
);

int MmacDmaTxGet  /* take ownership of the rx descriptor Desc and clear it, returns old data */
(                     /* returns descriptor number if ok, -1 if next descriptor is busy (owned by DMA) */
  Mmac *Dev,        /* Device */
  u32  *Status,         /* Descriptor status */
  u32  *Buffer1,        /* First buffer address to set to descriptor */
  u32  *Length1,        /* First buffer length */
  u32  *Data1           /* Driver's data to store into descriptor */
);

u32 MmacDmaRxLength   /* extracts length from the Dma descriptor status word */
(
  u32 Status
);

int MmacDmaRxValid   /* Test the status word if the descriptor is valid */
(
  u32 Status
);

int MmacDmaRxCollisions
(
  u32 Status
);

int MmacDmaRxCrc
(
  u32 Status
);

int MmacDmaTxValid   /* Test the status word if the descriptor is valid */
(
  u32 Status
);

int MmacDmaTxCollisions
(
  u32 Status
);

int MmacDmaTxAborted
(
  u32 Status
);

int MmacDmaTxCarrier
(
  u32 Status
);

void MmacDmaRxStart      /* Start DMA receiver */
(
  Mmac *Dev         /* Device */
);

void MmacDmaRxStop      /* Stop DMA receiver */
(
  Mmac *Dev         /* Device */
);

void MmacDmaRxResume    /* Resume DMA receiver after suspend */
(
  Mmac *Dev         /* Device */
);

void MmacDmaTxStart      /* Start DMA transmitter */
(
  Mmac *Dev         /* Device */
);

void MmacDmaTxStop      /* Stop DMA transmitter */
(
  Mmac *Dev         /* Device */
);

void MmacDmaTxResume    /* Resume DMA transmitter after suspend */
(
  Mmac *Dev         /* Device */
);


int MmacDmaRxFrame    
(
  u32 Status
);

int MmacDmaRxLengthError    
(
  u32 Status
);

/**********************************************************
 * DMA engine interrupt handling functions
 **********************************************************/


void MmacDmaIntEnable /* Enable DMA engine intrerrupts */
(
  Mmac *Dev         /* Device */
);

void MmacDmaIntDisable /* Disable DMA engine intrerrupts */
(
  Mmac *Dev         /* Device */
);

void MmacDmaIntClear /* Clear DMA engine interrupt requests */
(
  Mmac *Dev         /* Device */
);

u32 MmacDmaIntType
(
  Mmac *Dev         /* Device */
);

//void MmacDmaDescSync( Mmac *Dev, void *vaddr, size_t size, int direction );
#endif




//from dev.h --
/**********************************************************
 * function prototype in mmac.c
 **********************************************************/
//static int probe( struct eth_device *dev, u32 Addr );
static int mmac_init(struct eth_device *dev, bd_t *bis)  ;
static void mmac_stop( struct eth_device *dev );
static int mmac_receive( struct eth_device *dev );       /* handle received packets */
static int mmac_send(struct eth_device *dev, volatile void *packet, int length);
#ifdef CC_UBOOT_2011_12
static int phy_mtk_reset(Mmac *Dev);
#else
static int phy_reset(Mmac *Dev);
#endif

#endif /* MMAC_REG_INCLUDED */

 