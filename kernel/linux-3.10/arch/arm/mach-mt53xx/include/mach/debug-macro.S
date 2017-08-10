/*
 * linux/arch/arm/mach-mt53xx/include/mach/debug-macro.S
 *
 * Debugging macro include header
 *
 * Copyright (c) 2010-2012 MediaTek Inc.
 * $Author:  $
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

#include <mach/hardware.h>

        .macro  addruart,rx
        mrc p15, 0, \rx, c1, c0
        tst \rx, #1                 @ MMU enabled?
        moveq   \rx, #IO_PHYS       @ physical base address
        movne   \rx, #IO_BASE       @ virtual base
        add     \rx, \rx, #0xc0 << 8
        .endm

        .macro  senduart,rd,rx
        strb    \rd, [\rx, #0x0]
        .endm

        .macro  waituart,rd,rx
1001:   ldr \rd, [\rx, #0x4]    @ UART0 status
        tst \rd, 0x2            @ write allow
        bne 1001b
        .endm

        .macro  busyuart,rd,rx
1002:   ldr \rd, [\rx, #0xcc]   @ UART0 status
        tst \rd, 0x2            @ uart full .eq. busy
        beq 1002b
        .endm

