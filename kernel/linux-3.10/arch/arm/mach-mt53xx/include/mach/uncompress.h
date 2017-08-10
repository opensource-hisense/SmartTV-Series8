/*
 * linux/arch/arm/mach-mt53xx/include/mach/uncompress.h
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

#ifndef __ARCH_ARM_UNCOMPRESS_H
#define __ARCH_ARM_UNCOMPRESS_H

#define MT5391_UART0_DR  (*(volatile unsigned char *)0xF000c000)
#define MT5391_UART0_ST  (*(volatile unsigned int *) 0xF000c04C)

/*
 * This does not append a newline
 */
static inline void putc(int c)
{
    /* wait for empty slot */
    while ((MT5391_UART0_ST & 0x1f00) == 0);

    MT5391_UART0_DR = c;

    if (c == '\n') {
        while ((MT5391_UART0_ST & 0x1f00) == 0);

        MT5391_UART0_DR = '\r';
    }
}

static inline void flush(void)
{
    /* wait for empty slot */
    while ((MT5391_UART0_ST & 0x1f00) == 0);
}

/*
 * nothing to do
 */
#define arch_decomp_setup()

#define arch_decomp_wdog()

#endif /* __ARCH_ARM_UNCOMPRESS_H */

