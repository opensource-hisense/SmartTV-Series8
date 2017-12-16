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
/*
 *    target specific information
 */
#ifndef _MGC_DMA_H_
#define _MGC_DMA_H_

//  MTK Notice: Max Liao, 2006/05/29.
//  test DMA random size.
#define MIN_DMA_TRANSFER_BYTES       (1)

/*
 *   DMA control registers
 */

#define M_REG_DMA_INTR               0x0
#define M_REG_DMA_INTR_MASK    0x1   
#define M_REG_DMA_INTR_CLEAR   0x2
#define M_REG_DMA_INTR_SET     0x3
#define M_REG_DMA_CNTL               0x4
#define M_REG_DMA_ADDR               0x8
#define M_REG_DMA_COUNT             0xC


/*
 *   DMA operation mode
 */

#define DMA_TX                     0x2
#define DMA_RX                     0x0

#define DMA_MODE_ZERO              0x0
#define DMA_MODE_ONE               (0x4)

#define DMA_IRQ_ENABLE             0x8
#define DMA_IRQ_DISABLE            0x0

#define DMA_MODE_MASK              (DMA_TX | DMA_MODE_ONE)

#define DMA_TX_ZERO_IRQ       (DMA_TX | DMA_MODE_ZERO | DMA_IRQ_ENABLE)
#define DMA_RX_ZERO_IRQ       (DMA_RX | DMA_MODE_ZERO | DMA_IRQ_ENABLE)

#define DMA_TX_ONE_IRQ       (DMA_TX | DMA_MODE_ONE | DMA_IRQ_ENABLE)
#define DMA_RX_ONE_IRQ       (DMA_RX | DMA_MODE_ONE | DMA_IRQ_ENABLE)

#define RXCSR2_MODE1  (M_RXCSR2_AUTOCLEAR |M_RXCSR2_H_AUTOREQ |M_RXCSR2_DMAENAB |M_RXCSR2_DMAMODE)
#define RXCSR2_MODE0  (M_RXCSR2_AUTOCLEAR |M_RXCSR2_DMAENAB)
#define TXCSR2_MODE1  (M_TXCSR2_AUTOSET |M_TXCSR2_DMAENAB |M_TXCSR2_DMAMODE)

#define DMA_ENABLE_BIT          0x0001

#define DMA_BUSERROR_BIT        0x0100

#define DMA_DIRECTION_SHIFT          1
#define DMA_MODE_SHIFT               2 
#define DMA_IRQENABLE_SHIFT          3  
#define DMA_ENDPOINT_SHIFT           4
#define DMA_MAXPACKETSIZE_SHIFT      8

/*
 *   for endpoint RX / TX interrupt control
 */

#define EP_IRQ_ENABLE                1
#define EP_IRQ_DISABLE               0
#define EP_IRQ_RX                    0
#define EP_IRQ_TX                    2

#define Enable_TX_EP_Interrupt(endpoint) \
      Control_EP_Interrupt(endpoint, (EP_IRQ_ENABLE | EP_IRQ_TX));

#define Enable_RX_EP_Interrupt(endpoint) \
      Control_EP_Interrupt(endpoint, (EP_IRQ_ENABLE | EP_IRQ_RX));

#define Disable_TX_EP_Interrupt(endpoint) \
      Control_EP_Interrupt(endpoint, (EP_IRQ_DISABLE | EP_IRQ_TX));

#define Disable_RX_EP_Interrupt(endpoint) \
      Control_EP_Interrupt(endpoint, (EP_IRQ_DISABLE | EP_IRQ_RX));

#endif
