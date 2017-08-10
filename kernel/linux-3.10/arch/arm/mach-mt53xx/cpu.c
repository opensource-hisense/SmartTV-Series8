/*
 * cpu.c  - Suspend support specific for ARM.
 *		based on arch/i386/power/cpu.c
 *
 * Distribute under GPLv2
 *
 * Copyright (c) 2002 Pavel Machek <pavel@suse.cz>
 * Copyright (c) 2001 Patrick Mochel <mochel@osdl.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/spinlock.h>
#include <linux/poll.h>
#include <linux/delay.h>
#include <linux/sysrq.h>
#include <linux/proc_fs.h>
#include <linux/pm.h>
#include <linux/device.h>
#include <linux/suspend.h>
#include <asm/irq.h>
#include <asm/uaccess.h>
#include <asm/tlbflush.h>
#include <asm/suspend.h>

static struct saved_context saved_context;

extern void enable_sep_cpu(void *);

void __save_processor_state(struct saved_context *ctxt)
{
	/* save preempt state and disable it */
	preempt_disable();

	/* save coprocessor 15 registers */
	asm volatile ("mrc p15, 0, %0, c1, c0, 0" : "=r" (ctxt->CR));
	asm volatile ("mrc p15, 0, %0, c1, c0, 1" : "=r" (ctxt->ACR));
	asm volatile ("mrc p15, 0, %0, c3, c0, 0" : "=r" (ctxt->DACR));
	asm volatile ("mrc p15, 0, %0, c10, c0, 0" : "=r" (ctxt->TLBLR));
	asm volatile ("mrc p15, 0, %0, c13, c0, 0" : "=r" (ctxt->FCSE));
	asm volatile ("mrc p15, 0, %0, c13, c0, 1" : "=r" (ctxt->CID));
	asm volatile ("mrc p15, 0, %0, c2, c0, 2" : "=r" (ctxt->TTBCR));
	asm volatile ("mrc p15, 0, %0, c2, c0, 0" : "=r" (ctxt->TTBR0));
	asm volatile ("mrc p15, 0, %0, c2, c0, 1" : "=r" (ctxt->TTBR1));
}

extern void swsusp_save_pg_dir(void);
void save_processor_state(void)
{
	__save_processor_state(&saved_context);
}

void __restore_processor_state(struct saved_context *ctxt)
{
	/* restore coprocessor 15 registers */
	asm volatile ("mcr p15, 0, %0, c2, c0, 1" : : "r" (ctxt->TTBR1));
	asm volatile ("mcr p15, 0, %0, c2, c0, 0" : : "r" (ctxt->TTBR0));
	asm volatile ("mcr p15, 0, %0, c2, c0, 2" : : "r" (ctxt->TTBCR));
	asm volatile ("mcr p15, 0, %0, c13, c0, 1" : : "r" (ctxt->CID));
	asm volatile ("mcr p15, 0, %0, c13, c0, 0" : : "r" (ctxt->FCSE));
	asm volatile ("mcr p15, 0, %0, c10, c0, 0" : : "r" (ctxt->TLBLR));
	asm volatile ("mcr p15, 0, %0, c3, c0, 0" : : "r" (ctxt->DACR));
	asm volatile ("mcr p15, 0, %0, c1, c0, 1" : : "r" (ctxt->ACR));
	asm volatile ("mcr p15, 0, %0, c1, c0, 0" : : "r" (ctxt->CR));

	/* restore preempt state */
	preempt_enable();
}

void restore_processor_state(void)
{
	__restore_processor_state(&saved_context);
}

EXPORT_SYMBOL(save_processor_state);
EXPORT_SYMBOL(restore_processor_state);
extern const void __nosave_begin, __nosave_end;

int pfn_is_nosave(unsigned long pfn)
{
        unsigned long begin_pfn = __pa(&__nosave_begin) >> PAGE_SHIFT;
        unsigned long end_pfn = PAGE_ALIGN(__pa(&__nosave_end)) >> PAGE_SHIFT;

        return (pfn >= begin_pfn) && (pfn < end_pfn);
}

