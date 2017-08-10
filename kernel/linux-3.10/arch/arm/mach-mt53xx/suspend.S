/*
 * arch/arm/mach-mt53xx/suspend.S
 * Porting to suit mediatek
 * Copyright (C) 2011-2012
 * yj.chiang, MediaTek Inc <yj.chiang@mediatek.com>
 *
 * Original authors: Dmitry Chigirev, Vitaly Wool <source@mvista.com>
 * Modified by Kevin Wells <kevin.wells@nxp.com>
 *
 * 2005 (c) MontaVista Software, Inc. This file is licensed under
 * the terms of the GNU General Public License version 2. This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */
#include <linux/linkage.h>
#include <asm/assembler.h>
#include <mach/platform.h>
#include <mach/hardware.h>

/* Using named register defines makes the code easier to follow */
#define WORK1_REG			r0
#define WORK2_REG			r1
#define PDWNC_BASE_REG		r2
#define DRAMPHYBASE_REG			r5
#define DRAMABASE_REG			r6
#define DRAMBBASE_REG			r7

	.text

ENTRY(lpc32xx_sys_suspend)
	@ Save a copy of the used registers in IRAM, r0 is corrupted
	adr	r0, tmp_stack_end
	stmfd	r0!, {r3 - r7, sp, lr}

	@ Load a few common register addresses
	adr	WORK1_REG, reg_bases
	ldr	DRAMABASE_REG, [WORK1_REG, #0]
	ldr	DRAMBBASE_REG, [WORK1_REG, #4]
	ldr	PDWNC_BASE_REG, [WORK1_REG, #8]
	ldr	DRAMPHYBASE_REG, [WORK1_REG, #12]

	@ enter self refresh mode of CHA
	ldr	WORK2_REG, [DRAMABASE_REG,\
		#REG_RW_DRAM_00C]
	orr	WORK1_REG, WORK2_REG, #BITS_UNKNOWN_10
	str	WORK1_REG, [DRAMABASE_REG, #REG_RW_DRAM_00C]

	@ enter self refresh mode of CHB
	@ldr	WORK2_REG, [DRAMBBASE_REG,\
	@	#REG_RW_DRAM_00C]
	@orr	WORK1_REG, WORK2_REG, #BITS_UNKNOWN_10
	@str	WORK1_REG, [DRAMBBASE_REG, #REG_RW_DRAM_00C]

	@@ Reset DramA PHY
	ldr	WORK2_REG, [DRAMABASE_REG,\
		#REG_RW_DRAM_224]
	orr	WORK1_REG, WORK2_REG, #BIT_PHY_RESET
	str	WORK1_REG, [DRAMABASE_REG, #REG_RW_DRAM_224]

	@@ Reset DramB PHY
	@ldr	WORK2_REG, [DRAMBBASE_REG,\
	@	#REG_RW_DRAM_224]
	@orr	WORK1_REG, WORK2_REG, #BIT_PHY_RESET
	@str	WORK1_REG, [DRAMBBASE_REG, #REG_RW_DRAM_224]

	@@ Termination ctrl Dram PHY 	0xF0058b10[2:0]=0
	ldr	WORK2_REG, [DRAMPHYBASE_REG,\
		#REG_RW_DRAM_PHY_B10]
	bic	WORK1_REG, WORK2_REG, #BIT_PHY_TERMINATION_CTRL
	str	WORK1_REG, [DRAMPHYBASE_REG, #REG_RW_DRAM_PHY_B10]

	@@  Termination ctrl Dram PHY 	0xF0058b18[2:0]=0
	ldr	WORK2_REG, [DRAMPHYBASE_REG,\
		#REG_RW_DRAM_PHY_B18]
	bic	WORK1_REG, WORK2_REG, #BIT_PHY_TERMINATION_CTRL
	str	WORK1_REG, [DRAMPHYBASE_REG, #REG_RW_DRAM_PHY_B18]

	@@  Termination ctrl Dram PHY 	0xF0058b94[9:8, 5:4]
	ldr	WORK2_REG, [DRAMPHYBASE_REG,\
		#REG_RW_DRAM_PHY_B94]
	bic	WORK1_REG, WORK2_REG, #0x330
	orr	WORK1_REG, WORK1_REG, #0x220
	str	WORK1_REG, [DRAMPHYBASE_REG, #REG_RW_DRAM_PHY_B94]

1:

	ldr	WORK2_REG, [PDWNC_BASE_REG, #REG_RW_SHREG0]
	cmp WORK2_REG, #PDWNC_CMD_ARM_WAKEUP_FROM_SUSPEND
	beq 2f @ receive pwr key break;

	@.rept 100
	@nop
	@.endr

	mov r0, #0x800000
3:
	sub r0, r0, #1
	cmp r0, #0
	bne 3b

	b 1b @@ Branch until

2:
	@ restore from self refresh mode
	@ Load a few common register addresses
	adr	WORK1_REG, reg_bases
	ldr	DRAMABASE_REG, [WORK1_REG, #0]
	ldr	DRAMBBASE_REG, [WORK1_REG, #4]
	ldr	DRAMPHYBASE_REG, [WORK1_REG, #12]

	@@ Termination ctrl Dram PHY 	0xF0058b10[2:0]=010b
	ldr	WORK2_REG, [DRAMPHYBASE_REG,\
		#REG_RW_DRAM_PHY_B10]
	orr	WORK1_REG, WORK2_REG, #0x02
	str	WORK1_REG, [DRAMPHYBASE_REG, #REG_RW_DRAM_PHY_B10]

	@@  Termination ctrl Dram PHY 	0xF0058b18[2:0]=010b
	ldr	WORK2_REG, [DRAMPHYBASE_REG,\
		#REG_RW_DRAM_PHY_B18]
	orr	WORK1_REG, WORK2_REG, #0x02
	str	WORK1_REG, [DRAMPHYBASE_REG, #REG_RW_DRAM_PHY_B18]

	@@  Termination ctrl Dram PHY 	0xF0058b94[9:8, 5:4]
	ldr	WORK2_REG, [DRAMPHYBASE_REG,\
		#REG_RW_DRAM_PHY_B94]
	bic	WORK1_REG, WORK2_REG, #0x330
	orr	WORK1_REG, WORK1_REG, #0x110
	str	WORK1_REG, [DRAMPHYBASE_REG, #REG_RW_DRAM_PHY_B94]


	@@ Reset DramA PHY
	ldr	WORK2_REG, [DRAMABASE_REG,\
		#REG_RW_DRAM_224]
	bic	WORK1_REG, WORK2_REG, #BIT_PHY_RESET
	str	WORK1_REG, [DRAMABASE_REG, #REG_RW_DRAM_224]

	@@ Reset DramB PHY
	@ldr	WORK2_REG, [DRAMBBASE_REG,\
	@	#REG_RW_DRAM_224]
	@bic	WORK1_REG, WORK2_REG, #BIT_PHY_RESET
	@str	WORK1_REG, [DRAMBBASE_REG, #REG_RW_DRAM_224]

	@@ leave self refresh mode of CHB
	@ldr	WORK2_REG, [DRAMBBASE_REG,\
	@	#REG_RW_DRAM_00C]
	@bic	WORK1_REG, WORK2_REG, #BITS_UNKNOWN_10
	@str	WORK1_REG, [DRAMBBASE_REG, #REG_RW_DRAM_00C]

	@ leave self refresh mode of CHA
	ldr	WORK2_REG, [DRAMABASE_REG,\
		#REG_RW_DRAM_00C]
	bic	WORK1_REG, WORK2_REG, #BITS_UNKNOWN_10
	str	WORK1_REG, [DRAMABASE_REG, #REG_RW_DRAM_00C]

	@ restore regs and return
	adr	r0, tmp_stack
	ldmfd	r0!, {r3 - r7, sp, pc}

reg_bases:
	.long	IO_ADDRESS(pDramIoMap)

tmp_stack:
	.long	0, 0, 0, 0, 0, 0, 0
tmp_stack_end:

ENTRY(lpc32xx_sys_suspend_sz)
	.word	. - lpc32xx_sys_suspend
