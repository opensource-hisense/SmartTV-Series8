/*
* Copyright (c) MediaTek Inc.
*
* This program is distributed under a dual license of GPL v2.0 and
* MediaTek proprietary license. You may at your option receive a license
* to this program under either the terms of the GNU General Public
* License (GPL) or MediaTek proprietary license, explained in the note
* below.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
*
/*
 * Copyright (c) 2012 MediaTek Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef _MT6577_USB_H_
#define _MT6577_USB_H_

#include "mgc_hdrc.h"

/* hardware spec */
#define MT_EP_NUM 4
#define MT_CHAN_NUM 4
#define MT_EP0_FIFOSIZE 64

#define FIFO_ADDR_START  512

#define MT_BULK_MAXP 512
#define MT_INT_MAXP  1024

#define USB_BASE    (MUSB_BASE + MUSB_COREBASE)
void arch_enable_ints(void);


/* USB common registers */
#define FADDR    M_REG_FADDR    /* Function Address Register */
#define POWER    M_REG_POWER    /* Power Management Register */
#define INTRTX   M_REG_INTRTX    /* TX Interrupt Status Register */
#define INTRRX   M_REG_INTRRX    /* RX Interrupt Status Register */
#define INTRTXE  M_REG_INTRTXE    /* TX Interrupt Status Enable Register */
#define INTRRXE  M_REG_INTRRXE    /* RX Interrupt Status Enable Register */
#define INTRUSB  M_REG_INTRUSB    /* Common USB Interrupt Register */
#define INTRUSBE M_REG_INTRUSBE    /* Common USB Interrupt Enable Register */
#define FRAME    M_REG_FRAME    /* Frame Number Register */
#define INDEX    M_REG_INDEX    /* Endpoint Selecting Index Register */
#define TESTMODE M_REG_TESTMODE    /* Test Mode Enable Register */

/* POWER fields */
#define PWR_ISO_UPDATE       (1<<7)
#define PWR_SOFT_CONN        (1<<6)
#define PWR_HS_ENAB          (1<<5)
#define PWR_HS_MODE          (1<<4)
#define PWR_RESET            (1<<3)
#define PWR_RESUME           (1<<2)
#define PWR_SUSPEND_MODE     (1<<1)
#define PWR_ENABLE_SUSPENDM  (1<<0)

/* INTRUSB fields */
#define INTRUSB_VBUS_ERROR (1<<7)
#define INTRUSB_SESS_REQ   (1<<6)
#define INTRUSB_DISCON     (1<<5)
#define INTRUSB_CONN       (1<<4)
#define INTRUSB_SOF        (1<<3)
#define INTRUSB_RESET      (1<<2)
#define INTRUSB_RESUME     (1<<1)
#define INTRUSB_SUSPEND    (1<<0)

/* DMA control registers */
#define USB_DMA_INTR (0x0200)
#define USB_DMA_INTR_UNMASK_SET_OFFSET (24)

#define USB_DMA_CNTL(chan)	(0x0204 + 0x10*(chan-1))
#define USB_DMA_ADDR(chan)	(0x0208 + 0x10*(chan-1))
#define USB_DMA_COUNT(chan)	(0x020c + 0x10*(chan-1))

/* Endpoint Control/Status Registers */
#define IECSR (0x0000)

/* for EP0 */
#define CSR0         M_REG_CSR0        /* EP0 Control Status Register */
                          				/* For Host Mode, it would be 0x2 */
#define COUNT0       M_REG_COUNT0        /* EP0 Received Bytes Register */
#define NAKLIMIT0    M_REG_NAKLIMIT0        /* NAK Limit Register */
#define CONFIGDATA   M_REG_CONFIGDATA        /* Core Configuration Register */


/* for other endpoints */
#define TXMAP        M_REG_TXMAXP        /* TXMAP Register: Max Packet Size for TX */
#define TXCSR        M_REG_TXCSR        /* TXCSR Register: TX Control Status Register */
#define RXMAP        M_REG_RXMAXP        /* RXMAP Register: Max Packet Size for RX */
#define RXCSR        M_REG_RXCSR        /* RXCSR Register: RX Control Status Register */
#define RXCOUNT      M_REG_RXCOUNT        /* RXCOUNT Register */
#define TXTYPE       M_REG_TXTYPE        /* TX Type Register */
#define TXINTERVAL   M_REG_TXINTERVAL        /* TX Interval Register */
#define RXTYPE       M_REG_RXTYPE        /* RX Type Register */
#define RXINTERVAL   M_REG_RXINTERVAL        /* RX Interval Register */
#define FIFOSIZE     M_REG_FIFOSIZE        /* configured FIFO size register */

/* control status register fields */
/* CSR0_DEV */
#define EP0_FLUSH_FIFO           (1<<8)
#define EP0_SERVICE_SETUP_END    (1<<7)
#define EP0_SERVICED_RXPKTRDY    (1<<6)
#define EP0_SENDSTALL            (1<<5)
#define EP0_SETUPEND             (1<<4)
#define EP0_DATAEND              (1<<3)
#define EP0_SENTSTALL            (1<<2)
#define EP0_TXPKTRDY             (1<<1)
#define EP0_RXPKTRDY             (1<<0)

/* TXCSR_DEV */
#define EPX_TX_AUTOSET           (1<<15)
#define EPX_TX_ISO               (1<<14)
#define EPX_TX_MODE              (1<<13)
#define EPX_TX_DMAREQEN          (1<<12)
#define EPX_TX_FRCDATATOG        (1<<11)
#define EPX_TX_DMAREQMODE        (1<<10)
#define EPX_TX_AUTOSETEN_SPKT    (1<<9)
#define EPX_TX_INCOMPTX          (1<<7)
#define EPX_TX_CLRDATATOG        (1<<6)
#define EPX_TX_SENTSTALL         (1<<5)
#define EPX_TX_SENDSTALL         (1<<4)
#define EPX_TX_FLUSHFIFO         (1<<3)
#define EPX_TX_UNDERRUN          (1<<2)
#define EPX_TX_FIFONOTEMPTY      (1<<1)
#define EPX_TX_TXPKTRDY          (1<<0)

/* RXCSR_DEV */
#define EPX_RX_AUTOCLEAR         (1<<15)
#define EPX_RX_ISO               (1<<14)
#define EPX_RX_DMAREQEN          (1<<13)
#define EPX_RX_DISNYET           (1<<12)
#define EPX_RX_PIDERR            (1<<12)
#define EPX_RX_DMAREQMODE        (1<<11)
#define EPX_RX_AUTOCLRENSPKT     (1<<10)
#define EPX_RX_INCOMPRXINTREN    (1<<9)
#define EPX_RX_INCOMPRX          (1<<8)
#define EPX_RX_CLRDATATOG        (1<<7)
#define EPX_RX_SENTSTALL         (1<<6)
#define EPX_RX_SENDSTALL         (1<<5)
#define EPX_RX_FLUSHFIFO         (1<<4)
#define EPX_RX_DATAERR           (1<<3)
#define EPX_RX_OVERRUN           (1<<2)
#define EPX_RX_FIFOFULL          (1<<1)
#define EPX_RX_RXPKTRDY          (1<<0)

/* CONFIGDATA fields */
#define MP_RXE         (1<<7)
#define MP_TXE         (1<<6)
#define BIGENDIAN      (1<<5)
#define HBRXE          (1<<4)
#define HBTXE          (1<<3)
#define DYNFIFOSIZING  (1<<2)
#define SOFTCONE       (1<<1)
#define UTMIDATAWIDTH  (1<<0)

/* FIFO register */
/*
 * for endpint 1 ~ 4, writing to these addresses = writing to the
 * corresponding TX FIFO, reading from these addresses = reading from
 * corresponding RX FIFO
 */

#define FIFO(ep_num)     (USB_BASE + 0x0020 + ep_num*0x0004)

/* ============================ */
/* additional control registers */
/* ============================ */

#define DEVCTL       M_REG_DEVCTL        /* OTG Device Control Register */
#define PWRUPCNT     M_REG_PWRUPCNT        /* Power Up Counter Register */
#define TXFIFOSZ     M_REG_TXFIFOSZ        /* TX FIFO Size Register */
#define RXFIFOSZ     M_REG_RXFIFOSZ        /* RX FIFO Size Register */
#define TXFIFOADD    M_REG_TXFIFOADD        /* TX FIFO Address Register */
#define RXFIFOADD    M_REG_RXFIFOADD        /* RX FIFO Address Register */
#define HWVERS       M_REG_HWSVERS        /* H/W Version Register */
#define EPINFO       M_REG_EPINFO        /* TX and RX Information Register */
#define RAM_DMAINFO  M_REG_RAMINFO        /* RAM and DMA Information Register */
#define LINKINFO     M_REG_LINKINFO        /* Delay Time Information Register */
#define VPLEN        M_REG_VPLEN        /* VBUS Pulse Charge Time Register */
#define HSEOF1       M_REG_HSEOF1        /* High Speed EOF1 Register */
#define FSEOF1       M_REG_FSEOF1        /* Full Speed EOF1 Register */
#define LSEOF1       M_REG_LSEOF1        /* Low Speed EOF1 Register */
#define RSTINFO      M_REG_RSTINFO        /* Reset Information Register */

/* FIFO size register fields and available packet size values */
#define DOUBLE_BUF	1
#define FIFOSZ_DPB	(1 << 4)
#define PKTSZ		0x0f

#define PKTSZ_8		(1<<3)
#define PKTSZ_16	(1<<4)
#define PKTSZ_32	(1<<5)
#define PKTSZ_64	(1<<6)
#define PKTSZ_128	(1<<7)
#define PKTSZ_256	(1<<8)
#define PKTSZ_512	(1<<9)
#define PKTSZ_1024	(1<<10)

#define FIFOSZ_8	(0x0)
#define FIFOSZ_16	(0x1)
#define FIFOSZ_32	(0x2)
#define FIFOSZ_64	(0x3)
#define FIFOSZ_128	(0x4)
#define FIFOSZ_256	(0x5)
#define FIFOSZ_512	(0x6)
#define FIFOSZ_1024	(0x7)
#define FIFOSZ_2048	(0x8)
#define FIFOSZ_4096	(0x9)
#define FIFOSZ_3072	(0xF)

/* SWRST fields */
#define SWRST_PHY_RST         (1<<7)
#define SWRST_PHYSIG_GATE_HS  (1<<6)
#define SWRST_PHYSIG_GATE_EN  (1<<5)
#define SWRST_REDUCE_DLY      (1<<4)
#define SWRST_UNDO_SRPFIX     (1<<3)
#define SWRST_FRC_VBUSVALID   (1<<2)
#define SWRST_SWRST           (1<<1)
#define SWRST_DISUSBRESET     (1<<0)

/* DMA_CNTL */
#define USB_DMA_CNTL_ENDMAMODE2            (1 << 13)
#define USB_DMA_CNTL_PP_RST                (1 << 12)
#define USB_DMA_CNTL_PP_EN                 (1 << 11)
#define USB_DMA_BURST_MODE_MASK            (3 << 9)
#define USB_DMA_BURST_MODE_0               (0 << 9)
#define USB_DMA_BURST_MODE_1               (0x1 << 9)
#define USB_DMA_BURST_MODE_2               (0x2 << 9)
#define USB_DMA_BURST_MODE_3               (0x3 << 9)
#define USB_DMA_BUS_ERROR                  (0x1 << 8)
#define USB_DMA_ENDPNT_MASK                (0xf << 4)
#define USB_DMA_ENDPNT_OFFSET              (4)
#define USB_DMA_INTEN                      (1 << 3)
#define USB_DMA_DMAMODE                    (1 << 2)
#define USB_DMA_DIR                        (1 << 1)
#define USB_DMA_EN                         (1 << 0)

/* USB level 1 interrupt registers */

#define USB_L1INTS M_REG_INTRLEVEL1  /* USB level 1 interrupt status register */
#define USB_L1INTM M_REG_INTRLEVEL1EN  /* USB level 1 interrupt mask register  */
#define USB_L1INTP M_REG_INTRLEVEL1TP  /* USB level 1 interrupt polarity register  */

#define TX_INT_STATUS		(1 << 0)
#define RX_INT_STATUS		(1 << 1)
#define USBCOM_INT_STATUS	(1 << 2)
#define DMA_INT_STATUS		(1 << 3)
#define PSR_INT_STATUS		(1 << 4)
#define QINT_STATUS		(1 << 5)
#define QHIF_INT_STATUS		(1 << 6)
#define DPDM_INT_STATUS		(1 << 7)
#define VBUSVALID_INT_STATUS	(1 << 8)
#define IDDIG_INT_STATUS	(1 << 9)
#define DRVVBUS_INT_STATUS	(1 << 10)
#define POWERDWN_INT_STATUS	(1 << 11)

#define VBUSVALID_INT_POL	(1 << 8)
#define IDDIG_INT_POL		(1 << 9)
#define DRVVBUS_INT_POL		(1 << 10)

/* mt_usb defines */
typedef enum
{
	EP0_IDLE = 0,
	EP0_RX,
	EP0_TX,
} EP0_STATE;


/* some macros */
#define EPMASK(X)	(1 << X)
#define CHANMASK(X)	(1 << X)

#endif
