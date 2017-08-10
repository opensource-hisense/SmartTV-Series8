/*-----------------------------------------------------------------------------
 * drivers/usb/musb_qmu/mt8560.h
 *
 * MT53xx USB driver
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

/** @file mtk_hcd.h
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
/*-------------------------------------------------------------------------*/
#ifndef MTK_HCD

#define MTK_HCD


#if !defined(CONFIG_ARCH_MT85XX)
#ifdef CONFIG_64BIT
#include <linux/soc/mediatek/hardware.h>
#include <linux/soc/mediatek/x_typedef.h>
#else
#include <mach/hardware.h>
#include <x_typedef.h>
#endif
#else //#if !defined(CONFIG_ARCH_MT85XX)
#include <mach/irqs.h>
#include <mach/mt85xx.h>
//#include <asm/system.h>
#include <config/arch/chip_ver.h>
#if(CONFIG_CHIP_VER_CURR == CONFIG_CHIP_VER_MT8563)
    #define CC_MT8563 1
#elif(CONFIG_CHIP_VER_CURR == CONFIG_CHIP_VER_MT8580)
    #define CC_MT8580 1		
#else
    #define CC_MT8550 1
#endif

#endif //#if !defined(CONFIG_ARCH_MT85XX)

#define MU_MB()  mb() 

#if defined(CONFIG_ARCH_MT85XX)

#ifdef CC_MT8563
#define MUSB_VECTOR_USB0				(82) 
#define MUSB_VECTOR_USB1				(22)
#define MUSB_VECTOR_USB2				(23)
#else
#define MUSB_VECTOR_USB0				VECTOR_USB
#define MUSB_VECTOR_USB1				VECTOR_USB2 
#define MUSB_VECTOR_USB2				VECTOR_USB3 
#endif

#ifndef MUSB_BASE
#ifdef CC_MT8563
#define MUSB_BASE 						(IO_VIRT + 0x37000)	
#define MUSB_BASE2						 (IO_VIRT + 0xE000)
#define MUSB_BASE3                        (IO_VIRT + 0xF000)
#else
#define MUSB_BASE                       (IO_VIRT + 0xE000)
#define MUSB_BASE2                        (IO_VIRT + 0x3C000)
#define MUSB_BASE3                        (IO_VIRT + 0x37000)
#endif
#endif


#define MUSB_COREBASE                                   (0x000)
#define MUSB_DMABASE                                    (0x200)
#ifdef CC_MT8580
#define MUSB_PHYBASE                                    (0x1000)
#define MUSB_PWD_PHYBASE                                (-0x2000)
#endif
#ifdef CC_MT8563
#define MUSB_PORT0_PHYBASE          (0x35800)
#define MUSB_PORT1_PHYBASE          (0x48800)
#define MUSB_PORT2_PHYBASE          (0x49800)
#define MUSB_QMUBASE                                    (0x800)
#else
#define MUSB_PORT0_PHYBASE                              (0x800)
#define MUSB_PORT1_PHYBASE                              (0x900)
#define MUSB_PORT2_PHYBASE                              (0xA00)
#define MUSB_QMUBASE                                    (0x800)
#endif

#else //#if defined(CONFIG_ARCH_MT85XX)

#define MUSB_VECTOR_USB0				VECTOR_USB0 
#define MUSB_VECTOR_USB1				VECTOR_USB1 
#define MUSB_VECTOR_USB2				VECTOR_USB2
#define MUSB_VECTOR_USB3                VECTOR_USB3

extern void __iomem *pUSBamacIoMap;
extern void __iomem *pUSBaphyIoMap;

#define MUSB_BASE					(pUSBamacIoMap + 0x0000)
#define MUSB_BASE1					(pUSBamacIoMap + 0x1000)
#define MUSB_BASE2					(pUSBamacIoMap + 0x2000)
#if defined(CONFIG_ARCH_MT5891)
#define MUSB_BASE3					(pUSBamacIoMap + 0x2000)
#else
#define MUSB_BASE3					(pUSBamacIoMap + 0x3000)
#endif

#define MUSB_PORT0_PHYBASE       (pUSBaphyIoMap+0)
#define MUSB_PORT1_PHYBASE       (pUSBaphyIoMap+0x100) //    -- MAC 1    
#define MUSB_PORT2_PHYBASE       (pUSBaphyIoMap+0x200) //    -- MAC 2
#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5861)
#define MUSB_PORT3_PHYBASE		 (pUSBaphyIoMap+0x200)	
#elif defined(CONFIG_ARCH_MT5891)
#define MUSB_PORT3_PHYBASE		 (pUSBamacIoMap+0x3800)	
#elif defined(CONFIG_ARCH_MT5891) 
#define MUSB_PORT3_PHYBASE		 (pUSBamacIoMap + 0x3800)
#elif defined(CONFIG_ARCH_MT5882) 
#define MUSB_PORT3_PHYBASE       (IS_IC_MT5885())?(pUSBamacIoMap + 0xa800):(pUSBaphyIoMap+0x300)		// MT5885 PortD PHY base address 5a8xx
#else
#define MUSB_PORT3_PHYBASE       (pUSBaphyIoMap+0x300)		
#endif

#define MUSB_COREBASE            (0)  
#define MUSB_DMABASE             (0x200)
#define MUSB_PHYBASE             (0x9800)
#define MUSB_QMUBASE             (0x800)

#endif //#if defined(CONFIG_ARCH_MT85XX)

#define MBIM_VIRT                    (IO_VIRT + 0x08000)
#define MGC_O_HDRC_FADDR  0x00   /* 8-bit */
#define MGC_O_HDRC_INTRTX 0x02   /* 16-bit */
#define MGC_O_HDRC_INTRRX       0x04
#define MGC_O_HDRC_INTRTXE      0x06
#define MGC_O_HDRC_INTRRXE      0x08
#define MGC_O_HDRC_INTRUSB      0x0A /* 8 bit */
#define MUSB_DEFAULT_ADDRESS_SPACE_SIZE 0x00001000
#define MGC_O_HDRC_DUMMY1       0xE0   /* 32 bit */

//level 1 interrupt
#define M_REG_INTRLEVEL1 0xA0
#define M_REG_INTRLEVEL1EN 0xA4
#define MGC_M_DUMMY1_SOFFORCE   (1<<7)

#define MGC_M_FIFO_EP0     0x20
#define MGC_HSDMA_MIN_DMA_LEN        (512) //(64)
#ifdef CONFIG_USB_GADGET_MUSB_HDRC
#define MUC_NUM_PLATFORM_DEV (4)
#else
#define MUC_NUM_PLATFORM_DEV (4)
#endif
//#define MUSB_ASSERT(x)   if (!(x)) BUG();
//Add Hub register for MT7118
//Also Data toggle for HW
//Time:2010-4-16

#define MUSB_TXFADDR( _n )  (0x480 + (((_n))*8) )
#define MUSB_RXFADDR( _n )  (0x484 + (((_n))*8) )

//Add Hub support register for MT7118
//Time;2010-7-1
#define MUSB_MTK_TXHUBADDR(_n)  (0x482 + (((_n))*8) )
#define MUSB_MTK_RXHUBADDR(_n)  (0x486 + (((_n))*8) )

#define MUSB_RXTOG            	0x80
#define MUSB_RXTOGENAB   		0x82
#define MUSB_TXTOG             	0x84
#define MUSB_TXTOGENAB   		0x86
//Add RqPktCount support for DMA RX Mode 1
#define MUSB_RQPKTCOUNT(_n)  (0x300+(((_n))*4))
#define MTK_MUSB_SUPPORTED_HW_VERSION 0
/*
Begin of MTK created register definition.
*/
#define	M_REG_FIFOBYTECNT	 0x690

//  MTK Notice: Max Liao, 2006/08/18.
//  support one to one mapping ep and device address. Base addr = USB_BASE = 0x20029800.
#define M_REG_EP0ADDR    0x90
#define M_REG_EP1ADDR    0x94
#define M_REG_EP2ADDR    0x98
#define M_REG_EP3ADDR    0x9C
#define M_REG_EP4ADDR    0xA0
#define M_REG_EPXADDR(X)    (M_REG_EP0ADDR + ((X) << 2))

//  MTK Notice: Max Liao, 2006/05/22.
//  read PHY line state. Base addr = USB_PHYBASE = 0x20029400.
#define M_REG_PHYC0              0x00
#define M_REG_PHYC1                0x04
#define M_REG_PHYC2              0x08
#define M_REG_PHYC3                0x0c
#define M_REG_PHYC4              0x10
#define M_REG_PHYC5                0x14
#define M_REG_PHYC6              0x18
#define M_REG_PHYC7                    0x1c

#define M_REG_DEBUG_PROBE       0x620
#define M_REG_LINESTATE_MASK	0x000000c0
#define LINESTATE_DISCONNECT         0x00000000
#define LINESTATE_FS_SPEED              0x00000040
#define LINESTATE_LS_SPEED              0x00000080
#define LINESTATE_HWERROR              0x000000c0

//  MTK Notice: Max Liao, 2006/05/29.
//  MTK add: soft reset register. Base addr = USB_MISCBASE = 0x20029600.
#define M_REG_SOFTRESET                          0x0
#define M_REG_SOFT_RESET_ACTIVE             0x0
#define M_REG_SOFT_RESET_DEACTIVE         0x3

//  MTK add: access unit control register. Base addr = USB_MISCBASE = 0x20029600.
/*
0x20029604
bit[1:0] reg_size  : should be always  2'b10
bit[4] Function address enable : enable function address selected by endpoint, default :1(enable)
bit[8] Force DRAM read byte enable : Force Byte enable = 0xFFFF during DRAM read, default: 0(disable)
bit[9]: Enable group2 DRAM agent, Select group2 DRAM agent, default: 0(group3)
*/
#define M_REG_ACCESSUNIT                          0x4
#define M_REG_ACCESSUNIT_8BIT                  0x0
#define M_REG_ACCESSUNIT_16BIT                0x1
#define M_REG_ACCESSUNIT_32BIT                0x2
#define M_REG_DEV_ADDR_MODE                    0x10


#define M_REG_RXDATATOG		            0x80
#define M_REG_TXDATATOG		            0x84
#define M_REG_SET_DATATOG(ep, v)	    (((1 << ep) << 16) | (v << ep))

//  MTK add: request packet number. Base addr = USB_DMABASE = 0x20029A00.
#define M_REG_REQPKT(ep)                      (0x100 + ((ep)<<2))

//  MTK Notice: Max Liao, 2006/09/19.
//  MTK add: IN packet interrupt. Base addr = USB_MISCBASE = 0x20029600.
/*
0x20029608[15:0] : Interrupt mask ( default : 16'hFFFF, will change to 16'h0 later)
0x2002960C [15:0]: interrupt status ( default : 0)
bit[15:0] RX IN endpoint request bit[0] : EP0, bit[1] : EP1, ...
Notes: Endpoint number is logical endpoint number, not physical.
*/
#define M_REG_INPKT_ENABLE                       0x8
#define M_REG_INPKT_STATUS                        0xC

//  MTK Notice: Max Liao, 2006/09/19.
//  MTK add: Powe down register. Base addr = USB_MISCBASE = 0x20029600.
/*
bit 0 : Enable DRAM clock, default : 1<Enable>
bit 1 : Enable Hardware Auto-PowerDown/Up, default : 0<Disable>, Auto-Clear after PowerUp
bit 2 : Read Only, 1: PHY Clock valid, 0: PHY clock is off.
After turn off DRAM clock, only 0x20029680 registers is accessable.
Write other registers makes no effect, and read other registers return constant value, 32'hDEAD_BEAF
*/
#define M_REG_AUTOPOWER                      0x80
#define M_REG_AUTOPOWER_DRAMCLK         0x01
#define M_REG_AUTOPOWER_ON                     0x02
#define M_REG_AUTOPOWER_PHYCLK             0x04
/*
End of MTK created register definition.
*/

/* Added in HDRC 1.9(?) & MHDRC 1.4 */
/* ULPI pass-through */
#define MGC_O_HDRC_ULPI_VBUSCTL  0x70
#define MGC_O_HDRC_ULPI_REGDATA 0x74
#define MGC_O_HDRC_ULPI_REGADDR 0x75
#define MGC_O_HDRC_ULPI_REGCTL   0x76

#define M_REG_PERFORMANCE1 0x70
#define M_REG_PERFORMANCE2 0x72
#define M_REG_PERFORMANCE3 0x74

/*
* DRC-specific definitions
* $Revision: #1 $
*/
#ifndef MUSB_C_NUM_EPS
#define MUSB_C_NUM_EPS   16
#endif

/*
*  CID input signal indicates the ID pin of mini-A/B connector
*  on FDRC, 'CID' is available in DevCtl, 
*  on HDRC, 'B-Device' in DevCtl is valid only while a session is in progress
*/
#define MGC_CID_UNKNOWN    2
#define MGC_CID_A_DEVICE   0
#define MGC_CID_B_DEVICE   1

/*
*  A_DEVICE, B_DEVICE must be qualified by CID_VALID for valid context.
*  x is a pointer to the core object.
*/

#define MGC_A_DEVICE(x)         ((x)->cid == CID_A_DEVICE)
#define MGC_B_DEVICE(x)         ((x)->cid == CID_B_DEVICE)

/* Vbus values */
#define MGC_VBUS_BELOW_SESSION_END  0
#define MGC_VBUS_ABOVE_SESSION_END  1
#define MGC_VBUS_ABOVE_AVALID       2
#define MGC_VBUS_ABOVE_VBUS_VALID   3
#define MGC_VBUS_ERROR             0xff

#define FEATURE_SOFT_CONNECT 1
#define FEATURE_DMA_PRESENT 2
#define FEATURE_HDRC_FS 4
#define FEATURE_HIGH_BW 8
#define FEATURE_DFIFO 16
#define FEATURE_MULTILAYER 32
#define FEATURE_I2C 64

#define MUSB_URB_QUEUE_SIZE   (50)

/* 
*  DRC register access macros
*/

/* Get offset for a given FIFO */
#define MGC_FIFO_OFFSET(_bEnd) (MGC_M_FIFO_EP0 + (_bEnd * 4))

#define MGC_END_OFFSET(_bEnd, _bOffset) (0x100 + (0x10*_bEnd) + _bOffset)

#define MGC_BUSCTL_OFFSET(_bEnd, _bOffset)	(0x480 + (8*_bEnd) + _bOffset)

/* indexed vs. flat register model */
#define MGC_SelectEnd(_pBase, _bEnd)                    \
MGC_Write8(_pBase, MUSB_INDEX, _bEnd)

#define MGC_ReadCsr8(_pBase, _bOffset, _bEnd)           \
MGC_Read8(_pBase, (_bOffset + 0x10))

#define MGC_ReadCsr16(_pBase, _bOffset, _bEnd)          \
MGC_Read16(_pBase, (_bOffset + 0x10))

#define MGC_WriteCsr8(_pBase, _bOffset, _bEnd, _bData)  \
MGC_Write8(_pBase, (_bOffset + 0x10), _bData)

#define MGC_WriteCsr16(_pBase, _bOffset, _bEnd, _bData) \
MGC_Write16(_pBase, (_bOffset + 0x10), _bData)

#define MGC_VBUS_MASK            0x18                   /* DevCtl D4 - D3 */

#define MGC_END0_START  0x0
#define MGC_END0_OUT    0x2
#define MGC_END0_IN     0x4
#define MGC_END0_STATUS 0x8

#define MGC_END0_STAGE_SETUP      0x0
#define MGC_END0_STAGE_TX       0x2
#define MGC_END0_STAGE_RX        0x4
#define MGC_END0_STAGE_STATUSIN     0x8
#define MGC_END0_STAGE_STATUSOUT        0xf
#define MGC_END0_STAGE_STALL_BIT    0x10

/* obsolete */
#define MGC_END0_STAGE_DATAIN      MGC_END0_STAGE_TX
#define MGC_END0_STAGE_DATAOUT     MGC_END0_STAGE_RX

#define MGC_CHECK_INSERT_DEBOUNCE   100

/*-------------------------------------------------------------------------*/
#define MIN_JIFFIES  ((msecs_to_jiffies(2) > 1) ? msecs_to_jiffies(2) : 2)

/* the virtual root hub timer IRQ checks for hub status */

#define MTK_PORT_C_MASK               \
((1 << USB_PORT_FEAT_C_CONNECTION)   \
| (1 << USB_PORT_FEAT_C_ENABLE)       \
| (1 << USB_PORT_FEAT_C_SUSPEND)      \
| (1 << USB_PORT_FEAT_C_OVER_CURRENT) \
| (1 << USB_PORT_FEAT_C_RESET))

#define MGC_SPEED_HS 1
#define MGC_SPEED_FS 2
#define MGC_SPEED_LS 3

#ifdef CC_MT8563
#define MGC_PHY_Read32(_pBase, r) (\
	((uint32_t)_pBase == (MUSB_BASE))?\
	*((volatile uint32_t *)((IO_VIRT)+(MUSB_PORT0_PHYBASE)+(r))):\
	(((uint32_t)_pBase == (MUSB_BASE2))?\
	*((volatile uint32_t *)((IO_VIRT)+(MUSB_PORT1_PHYBASE)+(r))):\
	*((volatile uint32_t *)((IO_VIRT)+(MUSB_PORT2_PHYBASE)+(r))))\
	)

#define MGC_PHY_Write32(_pBase, r, v) { \
	if((uint32_t)_pBase == (MUSB_BASE)){ \
    printk("USB PHY Port 0\n");\
    *((volatile uint32_t *)((IO_VIRT)+(MUSB_PORT0_PHYBASE) + (r))) = v;\
    printk("USB PHY@0x%08X = 0x%08X\n", ((uint32_t)((IO_VIRT )+(MUSB_PORT0_PHYBASE) + (r))), (uint32_t)v);\
    }\
    else if((uint32_t)_pBase == (MUSB_BASE2)){\
    printk("USB PHY Port 1\n");\
    *((volatile uint32_t *)((IO_VIRT)+ (MUSB_PORT1_PHYBASE) + (r))) = v;\
    printk("USB PHY@0x%08X = 0x%08X\n", ((uint32_t)((IO_VIRT)+(MUSB_PORT1_PHYBASE) + (r))), (uint32_t)v);\
    }\
    else if((uint32_t)_pBase == (MUSB_BASE3)){\
    printk("USB PHY Port 2\n");\
    *((volatile uint32_t *)((IO_VIRT)+ (MUSB_PORT2_PHYBASE) + (r))) = v;\
    printk("USB PHY@0x%08X = 0x%08X\n", ((uint32_t)((IO_VIRT)+(MUSB_PORT2_PHYBASE) + (r))), (uint32_t)v);\
    }\
}
#elif defined(CC_MT8580)
#define MGC_PHY_Read32(_pBase, r) (\
	((uint32_t)_pBase == (MUSB_BASE))?\
	*((volatile uint32_t *)((MUSB_BASE)+(MUSB_PHYBASE)+ (MUSB_PORT0_PHYBASE) + (r))):\
	(((uint32_t)_pBase == (MUSB_BASE2))?\
	*((volatile uint32_t *)((MUSB_BASE)+(MUSB_PHYBASE)+ (MUSB_PORT1_PHYBASE) + (r))):\
	*((volatile uint32_t *)((MUSB_BASE3)+(MUSB_PWD_PHYBASE)+ (MUSB_PORT0_PHYBASE) + (r))))\
	)

#define MGC_PHY_Write32(_pBase, r, v) { \
	if((uint32_t)_pBase == (MUSB_BASE)){ \
    printk("USB PHY Port 0\n");\
    *((volatile uint32_t *)((MUSB_BASE + MUSB_PHYBASE)+(MUSB_PORT0_PHYBASE) + (r))) = v;\
    printk("USB PHY@0x%08X = 0x%08X\n", ((uint32_t)((MUSB_BASE + MUSB_PHYBASE)+(MUSB_PORT0_PHYBASE) + (r))), (uint32_t)v);\
    }\
    else if((uint32_t)_pBase == (MUSB_BASE2)){\
    printk("USB PHY Port 1\n");\
    *((volatile uint32_t *)((MUSB_BASE + MUSB_PHYBASE)+ (MUSB_PORT1_PHYBASE) + (r))) = v;\
    printk("USB PHY@0x%08X = 0x%08X\n", ((uint32_t)((MUSB_BASE + MUSB_PHYBASE)+(MUSB_PORT1_PHYBASE) + (r))), (uint32_t)v);\
    }\
    else if((uint32_t)_pBase == (MUSB_BASE3)){\
    printk("USB PHY Port 2\n");\
    *((volatile uint32_t *)((MUSB_BASE3 + (MUSB_PWD_PHYBASE))+ (MUSB_PORT0_PHYBASE) + (r))) = v;\
    printk("USB PHY@0x%08X = 0x%08X\n", ((uint32_t)((MUSB_BASE3 + (MUSB_PWD_PHYBASE))+(MUSB_PORT0_PHYBASE) + (r))), (uint32_t)v);\
    }\
}
#else //MT5XXX
#define MGC_PHY_Read32(_pBase, r)      \
( \
	((ulong)_pBase ==(ulong) (MUSB_BASE))?\
	*((volatile uint32_t *)( (MUSB_PORT0_PHYBASE) + (r))):\
	(((ulong)_pBase == (ulong)(MUSB_BASE1))?\
	*((volatile uint32_t *)( (MUSB_PORT1_PHYBASE) + (r))):\
	(((ulong)_pBase == (ulong)(MUSB_BASE2))?\
	*((volatile uint32_t *)( (MUSB_PORT2_PHYBASE) + (r))):\
	*((volatile uint32_t *)( (MUSB_PORT3_PHYBASE) + (r))))\
))

#define MGC_PHY_Write32(_pBase, r, v)\
{ \
	if((ulong)_pBase == (ulong)(MUSB_BASE)){ \
    printk("USB PHY Port 0\n");\
    *((volatile uint32_t *)(MUSB_PORT0_PHYBASE + (r))) = v;\
    printk("USB PHY@0x%p = 0x%p\n", ((UPTR*)(ulong)(MUSB_PORT0_PHYBASE + (r))), (UPTR*)(ulong)v);\
    }\
    else if((ulong)_pBase == (ulong)(MUSB_BASE1)){\
    printk("USB PHY Port 1\n");\
    *((volatile uint32_t *)(MUSB_PORT1_PHYBASE + (r))) = v;\
    printk("USB PHY@0x%p = 0x%p\n", ((UPTR*)(ulong)(MUSB_PORT1_PHYBASE + (r))), (UPTR*)(ulong)v);\
    }\
	else if((ulong)_pBase ==(ulong) (MUSB_BASE2)){\
    printk("USB PHY Port 2\n");\
    *((volatile uint32_t *)(MUSB_PORT2_PHYBASE + (r))) = v;\
    printk("USB PHY@0x%p = 0x%p\n", ((UPTR*)(ulong)(MUSB_PORT2_PHYBASE + (r))), (UPTR*)(ulong)v);\
    }\
    else if((ulong)_pBase == (ulong)(MUSB_BASE3)){\
    printk("USB PHY Port 3\n");\
    *((volatile uint32_t *)(MUSB_PORT3_PHYBASE + (r))) = v;\
    printk("USB PHY@0x%p = 0x%p\n", ((UPTR*)(ulong)(MUSB_PORT3_PHYBASE+ (r))), (UPTR*)(ulong)v);\
    }\
	else if((ulong)_pBase ==(ulong) (MUSB_BASE2)){\
	printk("USB PHY Port 2\n");\
	*((volatile uint32_t *)( (MUSB_PORT2_PHYBASE) + (r))) = v;\
	printk("USB PHY@0x%p = 0x%p\n", ((UPTR*)(ulong)((MUSB_PORT2_PHYBASE) + (r))), (UPTR*)(ulong)v);\
	}\
}
#endif
extern int printk(const char *format, ...);
#define MGC_VIRT_Read32(r)\
            *((volatile uint32_t *)((IO_VIRT)+ (r)))
#define MGC_VIRT_Write32(r, v)\
            (*((volatile uint32_t *)((IO_VIRT)+ (r))) = v)

#define MGC_DMA_Read32(_pBase, r)      \
    *((volatile uint32_t *)(((uint32_t)_pBase) + (MUSB_DMABASE)+ (r)))

#define MGC_DMA_Write32(_pBase, r, v)  \
    (*((volatile uint32_t *)(((uint32_t)_pBase) + (MUSB_DMABASE)+ (r))) = v)
	
#define MGC_BIM_READ32(_offset)         *((volatile uint32_t *)(MBIM_VIRT + _offset))
#define MGC_BIM_WRITE32(_offset, value)  (*((volatile uint32_t *)((MBIM_VIRT)+ (_offset))) = value)

/**
* Read an 8-bit register from the core
* @param _pBase core base address in memory
* @param _offset offset into the core's register space
* @return 8-bit datum
*/
#define MGC_Read8(_pBase, _offset) *((volatile uint8_t *)(((ulong)_pBase) + MUSB_COREBASE + _offset))

/**
* Read a 16-bit register from the core
* @param _pBase core base address in memory
* @param _offset offset into the core's register space
* @return 16-bit datum
*/
#define MGC_Read16(_pBase, _offset) *((volatile uint16_t *)(((ulong)_pBase) + MUSB_COREBASE + _offset))

/**
* Read a 32-bit register from the core
* @param _pBase core base address in memory
* @param _offset offset into the core's register space
* @return 32-bit datum
*/
#define MGC_Read32(_pBase, _offset) *((volatile uint32_t *)(((ulong)_pBase) + MUSB_COREBASE + _offset))


/**
* Write an 8-bit core register
* @param _pBase core base address in memory
* @param _offset offset into the core's register space
* @param _data 8-bit datum
*/
#if !defined(CONFIG_ARCH_MT85XX)
#define MGC_Write8(_pBase, _offset, _data)                                                   \
{                                                                                            \
volatile uint32_t u4TmpVar;                                                                  \
u4TmpVar = *((volatile uint32_t*)(((ulong)_pBase) + MUSB_COREBASE + ((_offset) & 0xFFFFFFFC))); \
u4TmpVar &= ~(((ulong)0xFF) << (8*((_offset) & 0x03)));                                   \
u4TmpVar |= (ulong)(((_data) & 0xFF) << (8*((_offset) & 0x03)));  \
if((ulong)_pBase == (ulong)MUSB_BASE || (ulong)_pBase == (ulong)MUSB_BASE1 || (ulong)_pBase == (ulong)MUSB_BASE2 || (ulong)_pBase == (ulong)MUSB_BASE3){\
if((MGC_O_HDRC_INTRTX != _offset)&&( MGC_O_HDRC_FADDR == ((_offset) & 0xFFFFFFFC)))\
u4TmpVar &= 0x0000FFFF;\
else if ((MGC_O_HDRC_INTRRX != _offset)&&(MGC_O_HDRC_INTRRX == ((_offset) & 0xFFFFFFFC)))\
u4TmpVar &= 0xFFFF0000;\
else if ((MGC_O_HDRC_INTRUSB != _offset)&&(MGC_O_HDRC_INTRRXE == ((_offset) & 0xFFFFFFFC)))\
u4TmpVar &= 0xFF00FFFF;\
}\
*((volatile uint32_t*)(((ulong)_pBase) + MUSB_COREBASE + ((_offset) & 0xFFFFFFFC))) = u4TmpVar; \
}
#else
#define MGC_Write8(_pBase, _offset, _data)                                                   \
{                                                                                            \
volatile uint32_t u4TmpVar;                                                                  \
u4TmpVar = *((volatile uint32_t*)(((uint32_t)_pBase) + MUSB_COREBASE + ((_offset) & 0xFFFFFFFC))); \
u4TmpVar &= ~(((uint32_t)0xFF) << (8*((_offset) & 0x03)));                                   \
u4TmpVar |= (uint32_t)(((_data) & 0xFF) << (8*((_offset) & 0x03)));  \
if((uint32_t)_pBase == MUSB_BASE || (uint32_t)_pBase == MUSB_BASE2 || (uint32_t)_pBase == MUSB_BASE3){\
if((MGC_O_HDRC_INTRTX != _offset)&&( MGC_O_HDRC_FADDR == ((_offset) & 0xFFFFFFFC)))\
u4TmpVar &= 0x0000FFFF;\
else if ((MGC_O_HDRC_INTRRX != _offset)&&(MGC_O_HDRC_INTRRX == ((_offset) & 0xFFFFFFFC)))\
u4TmpVar &= 0xFFFF0000;\
else if ((MGC_O_HDRC_INTRUSB != _offset)&&(MGC_O_HDRC_INTRRXE == ((_offset) & 0xFFFFFFFC)))\
u4TmpVar &= 0xFF00FFFF;\
}\
*((volatile uint32_t*)(((uint32_t)_pBase) + MUSB_COREBASE + ((_offset) & 0xFFFFFFFC))) = u4TmpVar; \
}
#endif

/**
* Write a 16-bit core register
* @param _pBase core base address in memory
* @param _offset offset into the core's register space
* @param _data 16-bit datum
*/
#define MGC_Write16(_pBase, _offset, _data)                                                  \
{                                                                                            \
volatile uint32_t u4TmpVar;                                                                  \
u4TmpVar = *((volatile uint32_t*)(((ulong)_pBase) + MUSB_COREBASE + ((_offset) & 0xFFFFFFFC))); \
u4TmpVar &= ~(((uint32_t)0xFFFF) << (8*((_offset) & 0x03)));                                 \
u4TmpVar |= (_data) << (8*((_offset) & 0x03));   \
if((ulong)_pBase == (ulong)MUSB_BASE || (ulong)_pBase == (ulong)MUSB_BASE2 || (ulong)_pBase == (ulong)MUSB_BASE3){\
if((MGC_O_HDRC_INTRTX != _offset)&&( MGC_O_HDRC_FADDR == ((_offset) & 0xFFFFFFFC)))\
u4TmpVar &= 0x0000FFFF;\
else if ((MGC_O_HDRC_INTRRX != _offset)&&(MGC_O_HDRC_INTRRX == ((_offset) & 0xFFFFFFFC)))\
u4TmpVar &= 0xFFFF0000;\
else if ((MGC_O_HDRC_INTRUSB != _offset)&&(MGC_O_HDRC_INTRRXE == ((_offset) & 0xFFFFFFFC)))\
u4TmpVar &= 0xFF00FFFF;\
}\
*((volatile uint32_t*)(((ulong)_pBase) + MUSB_COREBASE + ((_offset) & 0xFFFFFFFC))) = u4TmpVar; \
}

/**
* Write a 32-bit core register
* @param _pBase core base address in memory
* @param _offset offset into the core's register space
* @param _data 32-bit datum
*/
#define MGC_Write32(_pBase, _offset, _data) \
(*((volatile uint32_t *)(((ulong)_pBase) + MUSB_COREBASE + _offset)) = _data)

#define MGC_FIFO_CNT    MGC_Write32

#define SPIN_LOCK_IRQSAVE(l, f, irq) \
    u4UsbIrqEnable = MGC_BIM_READ32(REG_IRQEN) & (1 << irq); \
    if (u4UsbIrqEnable) \
    {\
    uint32_t u4TmpVar; \
    spin_lock_irqsave(l, f);\
    u4TmpVar = MGC_BIM_READ32(REG_IRQEN);\
    u4TmpVar &= ~(1 << irq);\
    MGC_BIM_WRITE32(REG_IRQEN, u4TmpVar);\
    spin_unlock_irqrestore(l, f);\
    }
    
#define SPIN_UNLOCK_IRQRESTORE(l,f, irq) \
    if (u4UsbIrqEnable) \
    {\
    uint32_t u4TmpVar; \
    spin_lock_irqsave(l, f);\
    u4TmpVar = MGC_BIM_READ32(REG_IRQEN);\
    u4TmpVar |= (1 << irq);\
    MGC_BIM_WRITE32(REG_IRQEN, u4TmpVar);\
    spin_unlock_irqrestore(l, f);\
    }

#define MGC_HSDMA_CHANNELS 4

#define MGC_O_HSDMA_BASE    0x200
#define MGC_O_HSDMA_INTR    0x200

#define MGC_O_HSDMA_INTR_MASK    0x201
#define MGC_O_HSDMA_INTR_UNMASK_CLEAR 0x202
#define MGC_O_HSDMA_INTR_UNMASK_SET 0x203

#endif                    /* #ifndef MTK_HCD */
