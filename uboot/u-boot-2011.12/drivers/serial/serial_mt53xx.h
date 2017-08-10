/*
 * vm_linux/chiling/uboot/u-boot-2011.12/drivers/serial/serial_mt53xx.h
 *
 * Copyright (c) 2010-2012 MediaTek Inc.
 *	This file is based  ARM Realview platform
 *  Copyright (C) 2002 ARM Ltd.
 *  All Rights Reserved
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


 #ifndef _SERIAL_MT53XX_H
 #define _SERIAL_MT53XX_H

// UART Register Define
#if defined(CC_MT5391)
#define IO_BASE             	    0x20000000
#else
#define IO_BASE                     0xf0000000
#endif
#define SERIAL_BASE         	    (IO_BASE + 0xc000)

// Following are MT53XX UART registers
#define REG_SER_PORT            	(*((volatile UINT8*)(SERIAL_BASE + /*0x40*/0)))
#define REG_SER_CTR                 (*((volatile UINT16*)(SERIAL_BASE + 0x48)))
    #define SER_BAUDRATE_115200     0x0000
    #define SER_BAUDRATE_921600     0x0300
#define REG_SER_CFG                 (*((volatile UINT32*)(SERIAL_BASE + 0x04)))
    #define REG_SER_TRANSMODE       0x40
#define REG_SER_BUFFER_SIZE         (*((volatile UINT32*)(SERIAL_BASE + 0x1c)))
#define REG_SER_STATUS          	(*((volatile UINT16*)(SERIAL_BASE + 0x4c)))
    #define SER_WRITE_ALLOW    	 	0x1F00
    #define SER_READ_ALLOW      	0x001F
#define REG_SER_PC_READ_MODE    	(*((volatile UINT16*)(SERIAL_BASE + 0x18)))
    #define SER_UR_ACTIVE           (0)
    #define SER_ADSP_ACTIVE         (1)
    #define SER_RISC_ACTIVE         (2)

#define REG_SER01_PORT				(*((volatile UINT8*)(SERIAL_BASE + 0xC0)))
#define REG_SER01_CTR   			(*((volatile UINT32*)(SERIAL_BASE + 0xC8)))
#define REG_SER01_STATUS			(*((volatile UINT32*)(SERIAL_BASE + 0xCC)))

#define REG_SER_INT_EN				(*((volatile UINT32*)(SERIAL_BASE + 0xC)))
#define REG_SER_INT_STATUS			(*((volatile UINT32*)(SERIAL_BASE + 0x10)))
    #define INT_SER00_TX_EMPTY	    (UINT32)(0x1<<19)
    #define INT_SER00_RX_TOUT	    (UINT32)(0x1<<18)
    #define INT_SER00_RX_FULL	    (UINT32)(0x1<<17)
    #define INT_SER00_RX_ERR	    (UINT32)(0x1<<16)
    #define INT_SER00_RX_OVERFLOW	(UINT32)(0x1<<21)
    #define INT_SER01_TX_EMPTY	    (UINT32)(0x1<<3)
    #define INT_SER01_RX_TOUT	    (UINT32)(0x1<<2)
    #define INT_SER01_RX_FULL	    (UINT32)(0x1<<1)
    #define INT_SER01_RX_ERR	    (UINT32)(0x1<<0)
    #define INT_SER01_RX_OVERFLOW	(UINT32)(0x1<<5)
    #define INT_SER01_MODEM	        (UINT32)(0x1<<4)

#define REG_SER_MERGE_MODE          (*((volatile UINT32*)(SERIAL_BASE + 0x14)))

#define REG_SER0_BUF_CTL     (*((volatile UINT32*)(SERIAL_BASE + 0x50)))
#define REG_SER1_BUF_CTL     (*((volatile UINT32*)(SERIAL_BASE + 0xD0)))

// Following are MT53XX UART registers
#define REG_MT53XX_SER_INT_EN               (*((volatile UINT16*)(SERIAL_BASE + 0x0c)))
#define REG_MT53XX_SER_INT_STATUS           (*((volatile UINT16*)(SERIAL_BASE + 0x10)))
    #define INT_MT53XX_SER00_INT            (UINT16)(0x1<<15)
    #define INT_MT53XX_SER01_TX_EMPTY       (UINT16)(0x1<<3)
    #define INT_MT53XX_SER01_RX_TOUT	    (UINT16)(0x1<<2)
    #define INT_MT53XX_SER01_RX_FULL	    (UINT16)(0x1<<1)
    #define INT_MT53XX_SER01_RX_ERR	        (UINT16)(0x1<<0)
    #define INT_MT53XX_SER01_RX_OVERFLOW	(UINT16)(0x1<<5)
    #define INT_MT53XX_SER02_TX_EMPTY	    (UINT16)(0x1<<11)
    #define INT_MT53XX_SER02_RX_TOUT	    (UINT16)(0x1<<10)
    #define INT_MT53XX_SER02_RX_FULL	    (UINT16)(0x1<<9)
    #define INT_MT53XX_SER02_RX_ERR	        (UINT16)(0x1<<8)
    #define INT_MT53XX_SER02_RX_OVERFLOW	(UINT16)(0x1<<13)
    #define INT_MT53XX_SER02_MODEM	        (UINT16)(0x1<<12)

#define REG_MT53XX_SER_STATUS               (*((volatile UINT8*)(SERIAL_BASE + 4)))

#define REG_MT53XX_SER_PORT            	    (*((volatile UINT8*)(SERIAL_BASE + 0)))
#define REG_MT53XX_SER_MERGE_MODE        	(*((volatile UINT32*)(SERIAL_BASE + 0x14)))
#define REG_MT53XX_SER_CTR                  (*((volatile UINT32*)(SERIAL_BASE + 0x48)))

#define REG_MT53XX_SER01_PORT				(*((volatile UINT8*)(SERIAL_BASE + 0xC0)))
#define REG_MT53XX_SER01_STATUS			    (*((volatile UINT32*)(SERIAL_BASE + 0xCC)))

#define REG_MT53XX_SER01_CTR_REG			(*((volatile UINT32*)(SERIAL_BASE + 0xC8)))

#define REG_MT53XX_SER02_PORT				(*((volatile UINT8*)(SERIAL_BASE + 0x100)))
#define REG_MT53XX_SER02_STATUS			    (*((volatile UINT32*)(SERIAL_BASE + 0x10C)))
#define REG_MT53XX_SER02_CTR_REG			(*((volatile UINT32*)(SERIAL_BASE + 0x108)))

#define REG_MT53XX_SER_OUT_SEL				(*((volatile UINT8*)(SERIAL_BASE + 0x3C)))
#define REG_MT53XX_SER1_BUF_CTL             (*((volatile UINT32*)(SERIAL_BASE + 0xD0)))

#define REG_UART_UART1_LSR                  (*(volatile UINT32 *)(SERIAL_BASE+0x00CC))
#define REG_UART_UART2_LSR                  (*(volatile UINT32 *)(SERIAL_BASE+0x010C))

// For UART Interrupt mode
#define UART_INT_BUF_SIZE		1024
#define UART_VECTOR				17
#define SER_PORT_0  (0)
#define SER_PORT_1  (1)

// Use old UART2 buffer control register as testing register
#define TEST_REGEST 0x2000c110

// the default value of the last data in image.
#define MAGIC_NUM (0xc0cac01a)



#endif
