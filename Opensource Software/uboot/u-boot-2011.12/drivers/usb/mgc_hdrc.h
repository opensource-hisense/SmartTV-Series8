/******************************************************************
 *                                                                *
 *      Copyright Mentor Graphics Corporation 2003-2004           *
 *                                                                *
 *                All Rights Reserved.                            *
 *                                                                *
 *    THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION *
 *  WHICH IS THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS   *
 *  LICENSORS AND IS SUBJECT TO LICENSE TERMS.                    *
 *                                                                *
 *   Porting to suit mediatek
 *   Copyright (C) 2011-2012
 *   Wei.Wen, Mediatek Inc <wei.wen@mediatek.com>
 *
 ******************************************************************/


/** @file mgc_hdrc.h
 *  This header file implements the mtk53xx USB host controller driver.
 *
 * Status:  Passed basic stress testing, works with hubs, mice, keyboards,
 * and usb-storage.
 *
 * TODO:
 * - usb suspend/resume triggered by prHcd (with USB_SUSPEND)
 * - various issues noted in the code
 * - performance work; use both register banks; ...
 * - use urb->iso_frame_desc[] with ISO transfers
 */

#ifndef _MGC_HDRC_H_
#define _MGC_HDRC_H_

#define IO_BASE_USB                                    (0xf0000000)

#define MUSB_BASE                   (IO_BASE_USB + 0x50000)
#define MUSB_BASE1                  (IO_BASE_USB + 0x51000)
#if !defined(CONFIG_ARCH_MT5890)
#define MUSB_BASE2                  (IO_BASE_USB + 0x52000)
#else
#define MUSB_BASE2                  (IO_BASE_USB + 0x53000)
#endif
#if defined(CONFIG_ARCH_MT5891)
#define MUSB_BASE3                  (IO_BASE_USB + 0x52000)
#else
#define MUSB_BASE3                  (IO_BASE_USB + 0x53000)
#endif
#define MUSB_COREBASE            (0)  
#define MUSB_DMABASE             (0x200)
#define MUSB_PHYBASE             (0x9800)
#define MUSB_PORT0_PHYBASE       (0)
#define MUSB_PORT1_PHYBASE       (0x100)
#define MUSB_PORT2_PHYBASE       (0x200)
#if defined(CONFIG_ARCH_MT5891)
#define MUSB_PORT3_PHYBASE       (-0x6000)	
#else
#define MUSB_PORT3_PHYBASE       (0x300)	
#endif

typedef struct 
{
    UINT32 *pBase;
    UINT32 *pPhyBase;
    UINT32 u4Insert;
    UINT32 u4HostSpeed;
    UINT32 u4DeviceSpeed;
}MTKUSBPORT;

/* SPEED control */
#define HS_ON           (TRUE)
#define HS_OFF           (FALSE)


static MTKUSBPORT mtkusbport[] = 
{
    {  /* Port 0 information. */
        .pBase = (void*)(MUSB_BASE + MUSB_COREBASE), 
        .pPhyBase = (void*)(MUSB_BASE + MUSB_PHYBASE+ MUSB_PORT0_PHYBASE), 
        .u4HostSpeed = HS_ON, 
    },
    {  /* Port 1 information. */
        .pBase = (void*)(MUSB_BASE1 + MUSB_COREBASE), 
        .pPhyBase = (void*)(MUSB_BASE + MUSB_PHYBASE+ MUSB_PORT1_PHYBASE), 
        .u4HostSpeed = HS_ON, 
    },
    {  /* Port 2 information. */
        .pBase = (void*)(MUSB_BASE2 + MUSB_COREBASE), 
        .pPhyBase = (void*)(MUSB_BASE + MUSB_PHYBASE+ MUSB_PORT2_PHYBASE),             
        .u4HostSpeed = HS_ON, 
    },
    {  /* Port 3 information. */
        .pBase = (void*)(MUSB_BASE3 + MUSB_COREBASE), 
        .pPhyBase = (void*)(MUSB_BASE + MUSB_PHYBASE+ MUSB_PORT3_PHYBASE),             
        .u4HostSpeed = HS_ON, 
    }
};

	
/*
 *     MUSBHDRC Register map 
 */

/* Common USB registers */

#define M_REG_FADDR        0x00   /* 8 bit */
#define M_REG_POWER        0x01   /* 8 bit */
#define M_REG_INTRTX       0x02  
#define M_REG_INTRRX       0x04
#define M_REG_INTRTXE      0x06  
#define M_REG_INTRRXE      0x08  
#define M_REG_INTRUSB      0x0A   /* 8 bit */
#define M_REG_INTRUSBE     0x0B   /* 8 bit */
#define M_REG_FRAME        0x0C  
#define M_REG_INDEX        0x0E   /* 8 bit */
#define M_REG_TESTMODE     0x0F   /* 8 bit */
#define M_REG_TARGET_FUNCTION_BASE     0x80   /* 8 bit */

/* Indexed registers */

#define M_REG_TXMAXP       0x10
#define M_REG_CSR0         0x12
#define M_REG_TXCSR        0x12
#define M_REG_RXMAXP       0x14
#define M_REG_RXCSR        0x16
#define M_REG_COUNT0       0x18
#define M_REG_RXCOUNT      0x18
#define M_REG_TXTYPE       0x1A    /* 8 bit, only valid in Host mode */
#define M_REG_TYPE0        0x1A    /* 2 bit, only valid in MDRC Host mode */
#define	M_REG_NAKLIMIT0	   0x1B    /* 8 bit, only valid in Host mode */
#define M_REG_TXINTERVAL   0x1B    /* 8 bit, only valid in Host mode */
#define M_REG_RXTYPE       0x1C    /* 8 bit, only valid in Host mode */
#define M_REG_RXINTERVAL   0x1D    /* 8 bit, only valid in Host mode */
#define M_REG_CONFIGDATA   0x1F    /* 8 bit */
#define M_REG_FIFOSIZE     0x1F    /* 8 bit */    


/* FIFOs for Endpoints 0 - 15, 32 bit word boundaries */

#define M_FIFO_EP0         0x20


#define M_REG_DEVCTL       (0x0060)        /* OTG Device Control Register */
#define M_REG_PWRUPCNT     (0x0061)        /* Power Up Counter Register */
#define M_REG_TXFIFOSZ     (0x0062)        /* TX FIFO Size Register */
#define M_REG_RXFIFOSZ     (0x0063)        /* RX FIFO Size Register */
#define M_REG_TXFIFOADD    (0x0064)        /* TX FIFO Address Register */
#define M_REG_RXFIFOADD    (0x0066)        /* RX FIFO Address Register */
#define M_REG_HWCAPS       (0x006c)        /* H/W Capability Register */
#define M_REG_HWSVERS      (0x006e)		   /* H/W Version Register */
#define M_REG_BUSPERF1     (0x0070)        /* H/W Bus Performance Register 1 */
#define M_REG_BUSPERF2     (0x0072)        /* H/W Bus Performance Register 2 */
#define M_REG_BUSPERF3     (0x0074)        /* H/W Bus Performance Register 3 */
#define M_REG_EPINFO	   (0x0078)        /* TX and RX Information Register */
#define M_REG_RAMINFO      (0x0079)        /* RAM and DMA Information Register */
#define M_REG_LINKINFO     (0x007a)        /* Delay Time Information Register */
#define M_REG_VPLEN        (0x007b)        /* VBUS Pulse Charge Time Register */
#define M_REG_HSEOF1       (0x007c)        /* High Speed EOF1 Register */
#define M_REG_FSEOF1       (0x007d)        /* Full Speed EOF1 Register */
#define M_REG_LSEOF1       (0x007e)        /* Low Speed EOF1 Register */
#define M_REG_RSTINFO      (0x007f)        /* Reset Information Register */


#define MTK_MHDRC


#ifdef MTK_MHDRC
#define M_REG_FIFOBYTECNT    0x690
#else
#define	M_REG_FIFOBYTECNT	 0x80
#endif

//  MTK Notice: Max Liao, 2006/08/18.
//  support one to one mapping ep and device address.
#define	M_REG_EP0ADDR	 0x90
#define	M_REG_EP1ADDR	 0x94
#define	M_REG_EP2ADDR	 0x98
#define	M_REG_EP3ADDR	 0x9C
#define	M_REG_EP4ADDR	 0xA0
#define	M_REG_EPXADDR(X)	 (M_REG_EP0ADDR + ((X) << 2))


//  MTK Notice: Lawrance, 2009/09/24.
#define M_REG_PHYC5             0x74
#define M_REG_LINESTATE_MASK	0x00030000

#define MGC_O_HDRC_DEVCTL     0x60 /* 8 bit */
#define MGC_M_DEVCTL_SESSION    0x01


//  MTK Notice: Max Liao, 2006/05/22.
//  read PHY line state.
#define M_REG_PHYC6		                0x14

#define LINESTATE_DISCONNECT         0x00000000
#define LINESTATE_FS_SPEED              0x00010000
#define LINESTATE_LS_SPEED              0x00020000
#define LINESTATE_HWERROR              0x00030000
    
//  MTK Notice: Max Liao, 2006/05/29.
//  MTK add: soft reset register. Base addr = USB_MISCBASE.
#define M_REG_SOFTRESET		                    0x0
#define M_REG_SOFT_RESET_ACTIVE             0x0 
#define M_REG_SOFT_RESET_DEACTIVE         0x3

//  MTK add: access unit control register. Base addr = USB_MISCBASE.
#define M_REG_ACCESSUNIT		            0x4
#define M_REG_ACCESSUNIT_8BIT           0x0
#define M_REG_ACCESSUNIT_16BIT         0x1
#define M_REG_ACCESSUNIT_32BIT         0x2
#define M_REG_DEV_ADDR_MODE                    0x10

//  MTK add: data toggle control register. Base addr = USB_MISCBASE.
#define M_REG_RXDATATOG		            0x10
#define M_REG_TXDATATOG		            0x14
#define M_REG_SET_DATATOG(ep, v)	    (((1 << ep) << 16) | (v << ep))

//  MTK add: request packet number. Base addr = USB_DMABASE.
#define M_REG_REQPKT(ep)	                    (0x100 + ((ep)<<2))	      

//  MTK Notice: Max Liao, 2006/09/19.
//  MTK add: IN packet interrupt. Base addr = USB_MISCBASE.
/*
0x20029608[15:0] : Interrupt mask ( default : 16'hFFFF, will change to 16'h0 later)
0x2002960C [15:0]: interrupt status ( default : 0)
bit[15:0] RX IN endpoint request bit[0] : EP0, bit[1] : EP1, ...
Notes: Endpoint number is logical endpoint number, not physical.
*/
#define M_REG_INPKT_ENABLE	            0x8
#define M_REG_INPKT_STATUS	            0xC

//  MTK Notice: Max Liao, 2006/09/19.
//  MTK add: Powe down register. Base addr = USB_MISCBASE.
/*
bit 0 : Enable DRAM clock, default : 1<Enable>
bit 1 : Enable Hardware Auto-PowerDown/Up, default : 0<Disable>, Auto-Clear after PowerUp
bit 2 : Read Only, 1: PHY Clock valid, 0: PHY clock is off.
After turn off DRAM clock, only 0x20029680 registers is accessable.
Write other registers makes no effect, and read other registers return constant value, 32'hDEAD_BEAF
*/
#define M_REG_AUTOPOWER	                    0x80
#define M_REG_AUTOPOWER_DRAMCLK         0x01
#define M_REG_AUTOPOWER_ON                     0x02
#define M_REG_AUTOPOWER_PHYCLK             0x04

//level 1 interrupt
#define M_REG_INTRLEVEL1 0xA0	/* USB level 1 interrupt status register */
#define M_REG_INTRLEVEL1EN 0xA4	/* USB level 1 interrupt mask register  */
#define M_REG_INTRLEVEL1TP 0xA8	/* USB level 1 interrupt polarity register  */


#define MOTG_PHY_REG_READ32(r) \
    *((volatile UINT32 *)(((UINT32)mtkusbport[u4Port].pPhyBase) + (r)))

#define MOTG_PHY_REG_WRITE32(r, v) \
    *((volatile UINT32 *)(((UINT32)mtkusbport[u4Port].pPhyBase) + (r))) = v

#define MOTG_PHY_REG_READ8(r) \
    *((volatile UINT8 *)(((UINT32)mtkusbport[u4Port].pPhyBase) + (r)))
#define MOTG_PHY_REG_WRITE8(r, v) \
	do {\
	volatile UINT32 temp;																 \
	temp = *((volatile UINT32*)(((UINT32)mtkusbport[u4Port].pPhyBase) + ((r) & 0xFFFFFFFC))); \
	temp &= ~(((UINT32)0xFF) << (8*((r) & 0x03)));									 \
	temp |= (UINT32)(((v) & 0xFF) << (8*((r) & 0x03))); 						 \
	*((volatile UINT32*)(((UINT32)mtkusbport[u4Port].pPhyBase) + ((r) & 0xFFFFFFFC))) = temp; \
    }while(0)	


#define MOTG_REG_READ8(r)\
    *((volatile UINT8 *)(((UINT32)mtkusbport[u4Port].pBase)+ (r)))

#define MOTG_REG_WRITE8(r,v)\
	do {\
	volatile UINT32 temp;																 \
	temp = *((volatile UINT32*)(((UINT32)mtkusbport[u4Port].pBase) + ((r) & 0xFFFFFFFC))); \
	temp &= ~(((UINT32)0xFF) << (8*((r) & 0x03)));									 \
	temp |= (UINT32)(((v) & 0xFF) << (8*((r) & 0x03))); 						 \
	*((volatile UINT32*)(((UINT32)mtkusbport[u4Port].pBase) + ((r) & 0xFFFFFFFC))) = temp; \
    }while(0)		

#define MOTG_REG_READ16(r)\
    *((volatile UINT16 *)(((UINT32)mtkusbport[u4Port].pBase) + r))

#define MOTG_REG_WRITE16(r,v)\
	do {\
	volatile UINT32 temp; 																 \
	temp = *((volatile UINT32*)(((UINT32)mtkusbport[u4Port].pBase) + ((r) & 0xFFFC))); \
	temp &= ~(((UINT32)0xFFFF) << (8*((r) & 0x03)));								 \
	temp |= (v) << (8*((r) & 0x03));												 \
	*((volatile UINT32*)(((UINT32)mtkusbport[u4Port].pBase) + ((r) & 0xFFFC))) = temp; \
	}while(0)
        
#define MOTG_REG_READ32(r)\
    *((volatile UINT32 *)(((UINT32)mtkusbport[u4Port].pBase) + r))

#define MOTG_REG_WRITE32(r,v)\
    *((volatile UINT32 *)(((UINT32)mtkusbport[u4Port].pBase) + r)) = v

#define FIFO_ADDRESS(e)\
    (((UINT32)mtkusbport[u4Port].pBase) + (e<<2) + M_FIFO_EP0)

#define MGC_FIFO_OFFSET(e) (M_FIFO_EP0 + (e * 4))


#define MGC_FIFO_CNT    MOTG_REG_WRITE32

#define REG_RO_CHIP_ID      0x01fc      // Chip ID Register

/* "bus control" registers */
#define MGC_O_MHDRC_TXFUNCADDR	0x00
#define MGC_O_MHDRC_TXHUBADDR	0x02
#define MGC_O_MHDRC_TXHUBPORT	0x03

#define MGC_O_MHDRC_RXFUNCADDR	0x04
#define MGC_O_MHDRC_RXHUBADDR	0x06
#define MGC_O_MHDRC_RXHUBPORT	0x07

#define MGC_BUSCTL_OFFSET(_bEnd, _bOffset)	(0x480 + (8*_bEnd) + _bOffset)

/* TxType/RxType */
#define MGC_M_TYPE_PROTO	0x30
#define MGC_S_TYPE_PROTO	4
#define MGC_M_TYPE_REMOTE_END	0xf
    
/*
 *     MUSBHDRC Register bit masks
 */

/* POWER */

#define M_POWER_ISOUPDATE   0x80 
#define	M_POWER_SOFTCONN    0x40
#define	M_POWER_HSENAB	    0x20
#define	M_POWER_HSMODE	    0x10
#define M_POWER_RESET       0x08
#define M_POWER_RESUME      0x04
#define M_POWER_SUSPENDM    0x02
#define M_POWER_ENSUSPEND   0x01

/* INTRUSB, INTRUSBE */

#define M_INTR_VBUSERROR    0x80   /* only valid when A device */
#define M_INTR_SESSREQ      0x40   /* only valid when A device */
#define M_INTR_DISCONNECT   0x20
#define M_INTR_CONNECT      0x10   /* only valid in Host mode */
#define M_INTR_SOF          0x08 
#define M_INTR_RESET        0x04
#define M_INTR_BABBLE       0x04
#define M_INTR_RESUME       0x02
#define M_INTR_SUSPEND      0x01   /* only valid in Peripheral mode */

/* TESTMODE */

#define M_TEST_FORCEFS      0x20
#define M_TEST_FORCEHS      0x10
#define M_TEST_PACKET       0x08
#define M_TEST_K            0x04
#define M_TEST_J            0x02
#define M_TEST_SE0_NAK      0x01

/* DEVCTL */

#define M_DEVCTL_BDEVICE    0x80   
#define M_DEVCTL_FSDEV      0x40
#define M_DEVCTL_LSDEV      0x20
#define M_DEVCTL_HM         0x04
#define M_DEVCTL_HR         0x02
#define M_DEVCTL_SESSION    0x01

/* CSR0 in Peripheral and Host mode */

#define	M_CSR0_FLUSHFIFO      0x0100
#define M_CSR0_TXPKTRDY       0x0002
#define M_CSR0_RXPKTRDY       0x0001


/* CSR0 in HSFC */

#define M_CSR0_INPKTRDY       0x02
#define M_CSR0_OUTPKTRDY      0x01

/* CSR0 in Peripheral mode */

#define M_CSR0_P_SVDSETUPEND  0x0080
#define M_CSR0_P_SVDRXPKTRDY  0x0040
#define M_CSR0_P_SENDSTALL    0x0020
#define M_CSR0_P_SETUPEND     0x0010
#define M_CSR0_P_DATAEND      0x0008
#define M_CSR0_P_SENTSTALL    0x0004

/* CSR0 in Host mode */

#define	M_CSR0_H_NAKTIMEOUT   0x0080
#define M_CSR0_H_STATUSPKT    0x0040
#define M_CSR0_H_REQPKT       0x0020
#define M_CSR0_H_ERROR        0x0010
#define M_CSR0_H_SETUPPKT     0x0008
#define M_CSR0_H_RXSTALL      0x0004
/* New in 15-July-2005 (v1.4?) */
#define M_CSR0_H_NO_PING	0x0800

/* CONFIGDATA */

#define M_CONFIGDATA_DMA        0x40
#define M_CONFIGDATA_BIGENDIAN  0x20
#define M_CONFIGDATA_HBRXE      0x10
#define M_CONFIGDATA_HBTXE      0x08
#define M_CONFIGDATA_DYNFIFO    0x04
#define M_CONFIGDATA_SOFTCONE   0x02
#define M_CONFIGDATA_UTMIDW     0x01   /* data width 0 => 8bits, 1 => 16bits */

/* TXCSR in Peripheral and Host mode */

#define M_TXCSR_AUTOSET       0x8000
#define M_TXCSR_ISO           0x4000
#define M_TXCSR_MODE          0x0000
#define M_TXCSR_DMAENAB       0x1000
#define M_TXCSR_FRCDATATOG    0x0800
#define M_TXCSR_DMAMODE       0x0400
#define M_TXCSR_CLRDATATOG    0x0040
#define M_TXCSR_FLUSHFIFO     0x0008
#define M_TXCSR_FIFONOTEMPTY  0x0002
#define M_TXCSR_TXPKTRDY      0x0001

/* TXCSR in Peripheral mode */

#define M_TXCSR_P_INCOMPTX    0x0080
#define M_TXCSR_P_SENTSTALL   0x0020
#define M_TXCSR_P_SENDSTALL   0x0010
#define M_TXCSR_P_UNDERRUN    0x0004

/* TXCSR in Host mode */

#define M_TXCSR_H_NAKTIMEOUT  0x0080
#define M_TXCSR_H_RXSTALL     0x0020
#define M_TXCSR_H_ERROR       0x0004

/* RXCSR in Peripheral and Host mode */

#define M_RXCSR_AUTOCLEAR     0x8000
#define M_RXCSR_DMAENAB       0x2000
#define M_RXCSR_DISNYET       0x1000
#define M_RXCSR_DMAMODE       0x0800
#define M_RXCSR_INCOMPRX      0x0100
#define M_RXCSR_CLRDATATOG    0x0080
#define M_RXCSR_FLUSHFIFO     0x0010
#define M_RXCSR_DATAERROR     0x0008
#define M_RXCSR_FIFOFULL      0x0002
#define M_RXCSR_RXPKTRDY      0x0001

/* RXCSR in Peripheral mode */

#define M_RXCSR_P_ISO         0x4000
#define M_RXCSR_P_SENTSTALL   0x0040
#define M_RXCSR_P_SENDSTALL   0x0020
#define M_RXCSR_P_OVERRUN     0x0004

/* RXCSR in Host mode */

#define M_RXCSR_H_AUTOREQ     0x4000
#define M_RXCSR_H_RXSTALL     0x0040
#define M_RXCSR_H_REQPKT      0x0020
#define M_RXCSR_H_ERROR       0x0004


/*
 *   Another map and bit definition for 8 bit access
 */

#define M_REG_INTRTX1      0x02   /* 8 bit */  
#define M_REG_INTRTX2      0x03   /* 8 bit */  
#define M_REG_INTRRX1      0x04   /* 8 bit */
#define M_REG_INTRRX2      0x05   /* 8 bit */
#define M_REG_INTRTX1E     0x06   /* 8 bit */  
#define M_REG_INTRTX2E     0x07   /* 8 bit */  
#define M_REG_INTRRX1E     0x08   /* 8 bit */    
#define M_REG_INTRRX2E     0x09   /* 8 bit */    
#define M_REG_TXCSR1       0x12
#define M_REG_TXCSR2       0x13
#define M_REG_RXCSR1       0x16
#define M_REG_RXCSR2       0x17

/* TXCSR in Peripheral and Host mode */

#define M_TXCSR2_AUTOSET       0x80
#define M_TXCSR2_ISO           0x40
#define M_TXCSR2_MODE          0x20
#define M_TXCSR2_DMAENAB       0x10
#define M_TXCSR2_FRCDATATOG    0x08
#define M_TXCSR2_DMAMODE       0x04

#define M_TXCSR1_CLRDATATOG    0x40
#define M_TXCSR1_FLUSHFIFO     0x08
#define M_TXCSR1_FIFONOTEMPTY  0x02
#define M_TXCSR1_TXPKTRDY      0x01

/* TXCSR in Peripheral mode */

#define M_TXCSR1_P_INCOMPTX    0x80
#define M_TXCSR1_P_SENTSTALL   0x20
#define M_TXCSR1_P_SENDSTALL   0x10
#define M_TXCSR1_P_UNDERRUN    0x04

/* TXCSR in Host mode */

#define M_TXCSR1_H_NAKTIMEOUT  0x80
#define M_TXCSR1_H_RXSTALL     0x20
#define M_TXCSR1_H_ERROR       0x04

/* RXCSR in Peripheral and Host mode */

#define M_RXCSR2_AUTOCLEAR     0x80
#define M_RXCSR2_DMAENAB       0x20
#define M_RXCSR2_DISNYET       0x10
#define M_RXCSR2_DMAMODE       0x08
#define M_RXCSR2_INCOMPRX      0x01

#define M_RXCSR1_CLRDATATOG    0x80
#define M_RXCSR1_FLUSHFIFO     0x10
#define M_RXCSR1_DATAERROR     0x08
#define M_RXCSR1_FIFOFULL      0x02
#define M_RXCSR1_RXPKTRDY      0x01

/* RXCSR in Peripheral mode */

#define M_RXCSR2_P_ISO         0x40
#define M_RXCSR1_P_SENTSTALL   0x40
#define M_RXCSR1_P_SENDSTALL   0x20
#define M_RXCSR1_P_OVERRUN     0x04

/* RXCSR in Host mode */

#define M_RXCSR2_H_AUTOREQ     0x40
#define M_RXCSR1_H_RXSTALL     0x40
#define M_RXCSR1_H_REQPKT      0x20
#define M_RXCSR1_H_ERROR       0x04


/* 
 *  DRC register access macros
 */

#define MGC_Read_RxCount(core)   MOTG_REG_READ16(core, M_REG_RXCOUNT);

/*
 *  A_DEVICE, B_DEVICE must be qualified by CID_VALID for valid context.
 *  x is a pointer to the core object.
 */
#define A_DEVICE(x)         ((x)->cid == CID_A_DEVICE)
#define B_DEVICE(x)         ((x)->cid == CID_B_DEVICE)

#define VBUS_MASK            0x18    /* DevCtl D4 - D3 */

#endif /* _MGC_HDRC_H_ */
