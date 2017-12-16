/*
 * vm_linux/chiling/uboot/u-boot-2011.12/drivers/serial/serial_mt53xx.c
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


#include <common.h>

#ifdef CONFIG_MT53XX_SERIAL

#include "serial_mt53xx.h"
#include "x_typedef.h"
#include <usb.h>

#define IO_WRITE(addr, val) (*(volatile unsigned int *)(addr) = (val))
#define IO_READ(addr) (*(volatile unsigned int *)(addr))

/* Integrator AP has two UARTs, we use the first one, at 38400-8-N-1 */
#define CONSOLE_PORT CONFIG_CONS_INDEX
#define baudRate CONFIG_BAUDRATE
//static volatile unsigned char *const port[] = CONFIG_PL01x_PORTS;
#define NUM_PORTS (sizeof(port)/sizeof(port[0]))


static void mt53xx_putc (int portnum, char c);
static int mt53xx_getc (int portnum);
static int mt53xx_tstc (int portnum);

static UINT8 _BspSerGetTxEmptyCnt(void);
static UINT8 _BspSerGetRxDataCnt(void);


int serial_init (void)
{
#if CC_PROJECT_X
    REG_SER_PC_READ_MODE = 2;
#else /* CC_PROJECT_X */
	//REG_SER_PC_READ_MODE = 0x2;
	REG_SER_CFG = 0xe2;	// set to transparent mode
#endif /* CC_PROJECT_X */

#if 0
	unsigned int divisor;

	/*
	 ** First, disable everything.
	 */
	IO_WRITE (port[CONSOLE_PORT] + UART_PL010_CR, 0x0);

	/*
	 ** Set baud rate
	 **
	 */
	switch (baudRate) {
	case 9600:
		divisor = UART_PL010_BAUD_9600;
		break;

	case 19200:
		divisor = UART_PL010_BAUD_9600;
		break;

	case 38400:
		divisor = UART_PL010_BAUD_38400;
		break;

	case 57600:
		divisor = UART_PL010_BAUD_57600;
		break;

	case 115200:
		divisor = UART_PL010_BAUD_115200;
		break;

	default:
		divisor = UART_PL010_BAUD_38400;
	}

	IO_WRITE (port[CONSOLE_PORT] + UART_PL010_LCRM,
		  ((divisor & 0xf00) >> 8));
	IO_WRITE (port[CONSOLE_PORT] + UART_PL010_LCRL, (divisor & 0xff));

	/*
	 ** Set the UART to be 8 bits, 1 stop bit, no parity, fifo enabled.
	 */
	IO_WRITE (port[CONSOLE_PORT] + UART_PL010_LCRH,
		  (UART_PL010_LCRH_WLEN_8 | UART_PL010_LCRH_FEN));

	/*
	 ** Finally, enable the UART
	 */
	IO_WRITE (port[CONSOLE_PORT] + UART_PL010_CR, (UART_PL010_CR_UARTEN));
#endif

	return (0);
}

void serial_putc (const char c)
{
#ifdef CONFIG_USB_PL2303
    if (pPL2303Dev)
    {
        PL2303_serial_putc(c);
        return;
    }
#endif

	if (c == '\n')
		mt53xx_putc (CONSOLE_PORT, '\r');

	mt53xx_putc (CONSOLE_PORT, c);
}

void serial_puts (const char *s)
{
#ifdef CONFIG_USB_PL2303
    if (pPL2303Dev)
    {
        PL2303_serial_puts(s);
        return;
    }
#endif

	while (*s) {
		serial_putc (*s++);
	}
}

int serial_getc (void)
{
#ifdef CONFIG_USB_PL2303
    if (pPL2303Dev)
    {
        return PL2303_serial_getc();
    }
#endif

	return mt53xx_getc (CONSOLE_PORT);
}

int serial_tstc (void)
{
#ifdef CONFIG_USB_PL2303
    if (pPL2303Dev)
    {
        return PL2303_serial_tstc();
    }
#endif

	return mt53xx_tstc (CONSOLE_PORT);
}

void serial_setbrg (void)
{
}

static void mt53xx_putc (int portnum, char c)
{
    while(_BspSerGetTxEmptyCnt() == 0) {}

#if CC_PROJECT_X
    // If normal mode, set RISC active mode
    if ((REG_SER_CFG & REG_SER_TRANSMODE) == 0)
    {
        REG_SER_PC_READ_MODE = SER_RISC_ACTIVE;
    }
#endif /* CC_PROJECT_X */

    REG_SER_PORT = c;

#if CC_PROJECT_X
    // If normal mode, wait tx buffer clear & set UR active mode.
    if ((REG_SER_CFG & REG_SER_TRANSMODE) == 0)
    {
        SerWaitTxBufClear();
        REG_SER_PC_READ_MODE = SER_UR_ACTIVE;
    }
#endif /* CC_PROJECT_X */
/*
    if (_BspDebugOutputToPort0())
    {
        REG_SER_PORT = c;
    }
    else // Use UART1
    {
        REG_SER01_PORT = cc;
    }
*/
}

static int mt53xx_getc (int portnum)
{
    while (_BspSerGetRxDataCnt() == 0) {}

    return REG_SER_PORT;

/*
    if (_BspDebugOutputToPort0())
    {
        return REG_SER_PORT;
    }
    else // Use UART1
    {
        return REG_SER01_PORT;
    }
*/
}

static int mt53xx_tstc (int portnum)
{
#if 0
	return !(IO_READ (port[portnum] + UART_PL01x_FR) &
		 UART_PL01x_FR_RXFE);
#endif

	if(_BspSerGetRxDataCnt() > 0)
		return 1;
	else
		return 0;
}

//-----------------------------------------------------------------------------
/** void _BspSerGetTxEmptyCnt()
 *  @return TX buffer empty count.
 */
//-----------------------------------------------------------------------------
static UINT8 _BspSerGetTxEmptyCnt()
{

    return (UINT8)((UINT16)(REG_SER_STATUS & SER_WRITE_ALLOW) >> 8);

}


//-----------------------------------------------------------------------------
/** _BspSerGetRxDataCnt()
 *  @return the number of characters in the RX buffer.
 */
//-----------------------------------------------------------------------------
static UINT8 _BspSerGetRxDataCnt(void)
{
    UINT8 u1RxCnt;

    u1RxCnt = (UINT8)(REG_SER_STATUS & SER_READ_ALLOW);

    return u1RxCnt;
}


#endif
