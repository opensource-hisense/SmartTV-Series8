/*
 * linux/arch/arm/mach-mt53xx/platsmp.c
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

#include <linux/init.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/jiffies.h>
#include <linux/smp.h>
#include <linux/io.h>

#include <mach/hardware.h>
#include <mach/x_hal_io.h>
#include <asm/cacheflush.h>
#include <asm/smp_scu.h>
#include <asm/unified.h>
//#include <asm/hardware/gic.h>
#include <asm/cputype.h>
#include <asm/smp_plat.h>

#define CPU_MASK		0xff0ffff0
#define CPU_CORTEX_A7		0x410fc070
#define CPU_CORTEX_A9		0x410fc090
#define CPU_CORTEX_A15		0x410fc0f0
#define CPU_CORTEX_A12		0x410fc0d0
#define CPU_CORTEX_A17		0x410fc0e0
#define CPU_CORTEX_A53		0x410fd030

extern void mt53xx_secondary_startup(void);
extern void mt53xx_secondary_sleep(void);
extern void __ref mt53xx_cpu_die(unsigned int cpu);
extern void __ref mt53xx_cpu_boot(unsigned int cpu);
extern void __cpuinitdata platform_cpu_die(unsigned int cpu);
extern int platform_cpu_kill(unsigned int cpu);
extern int disable_printk_in_caller;

#ifdef CONFIG_SMP
#define SCU_CONFIG		0x04

static unsigned int mt53xx_get_core_count(void)
{
	unsigned int cpu_id, ncores;
	u32 l2ctlr;

	cpu_id = read_cpuid(CPUID_ID) & CPU_MASK;
	switch (cpu_id) {
	case CPU_CORTEX_A15:
	case CPU_CORTEX_A7:
    case CPU_CORTEX_A12:
    case CPU_CORTEX_A17:
    case CPU_CORTEX_A53:
		asm("mrc p15, 1, %0, c9, c0, 2\n" : "=r" (l2ctlr));
		ncores = ((l2ctlr >> 24) & 3) + 1;
		break;
    #ifdef CONFIG_HAVE_ARM_SCU        
	case CPU_CORTEX_A9:
		ncores = __raw_readl(scu_base_addr() + SCU_CONFIG);
		ncores = (ncores & 0x03) + 1;
		break;
    #endif
    default:
		printk("mt53xx_get_core_count failed!! unknown cpu id!\n");
		BUG();
		break;
	}
    return ncores;
}

/*
 * control for which core is the next to come out of the secondary
 * boot "holding pen"
 */
//volatile int __cpuinitdata pen_release = -1;

static DEFINE_SPINLOCK(boot_lock);

void __cpuinit mt53xx_secondary_init(unsigned int cpu)
{
#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5882) || defined(CONFIG_ARCH_MT5883) || defined(CONFIG_ARCH_MT5891)
	printk("(yjdbg) mt53xx_secondary_init: %d, ", cpu);
	cpu = cpu_logical_map(cpu);
	printk("%d\n", cpu);
#endif
	trace_hardirqs_off();

	/*
	 * let the primary processor know we're out of the
	 * pen, then head off into the C entry point
	 */
	pen_release = -1;
	smp_wmb();

	/*
	 * Synchronise with the boot thread.
	 */
	spin_lock(&boot_lock);
	spin_unlock(&boot_lock);
}

int __cpuinit mt53xx_boot_secondary(unsigned int cpu, struct task_struct *idle)
{
	unsigned long timeout;
#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5882) || defined(CONFIG_ARCH_MT5883) || defined(CONFIG_ARCH_MT5891)
	printk("(yjdbg) mt53xx_boot_secondary: %d, ", cpu);
	cpu = cpu_logical_map(cpu);
	printk("%d\n", cpu);
#endif

    // power on the "cpu" if necessary
    mt53xx_cpu_boot(cpu);

	/*
	 * Set synchronisation state between this boot processor
	 * and the secondary one
	 */
	spin_lock(&boot_lock);

	/*
	 * This is really belt and braces; we hold unintended secondary
	 * CPUs in the holding pen until we're ready for them.  However,
	 * since we haven't sent them a soft interrupt, they shouldn't
	 * be there.
	 */
	pen_release = cpu;
	__cpuc_flush_dcache_area((void *)&pen_release, sizeof(pen_release));
	outer_clean_range(__pa(&pen_release), __pa(&pen_release + 1));

	/*
	 * Send the secondary CPU a soft interrupt, thereby causing
	 * the boot monitor to read the system wide flags register,
	 * and branch to the address found there.
	 */
	arm_send_ping_ipi(cpu);
	//arch_send_wakeup_ipi_mask(cpumask_of(cpu));

	timeout = jiffies + (1 * HZ);
	while (time_before(jiffies, timeout)) {
		smp_rmb();
		if (pen_release == -1)
			break;

		udelay(10);
	}

	/*
	 * now the secondary core is starting up let it run its
	 * calibrations, then wait for it to finish
	 */
	spin_unlock(&boot_lock);

	return pen_release != -1 ? -ENOSYS : 0;
}

/*
 * Initialise the CPU possible map early - this describes the CPUs
 * which may be present or become present in the system.
 */
void __init mt53xx_smp_init_cpus(void)
{
	unsigned int i, ncores;
	ncores = num_possible_cpus();

	if (ncores != 1)
	{
		printk("mt53xx_smp_init_cpus with DT, ncores: %d\n", ncores);
		/*
		 * CPU Nodes are passed thru DT and set_cpu_possible
		 * is set by "arm_dt_init_cpu_maps".
		 */
		return; // refer to mach-exynos/platsmp.c
	}
	else
	{
		ncores = mt53xx_get_core_count();
		printk("mt53xx_smp_init_cpus without DT, ncores: %d\n", ncores);
	}

	/* sanity check */
	if (ncores == 0) {
		printk(KERN_ERR
		       "mt53xx: strange CM count of 0? Default to 1\n");

		ncores = 1;
	}

	if (ncores > NR_CPUS) {
		printk(KERN_WARNING
		       "mt53xx: no. of cores (%d) greater than configured "
		       "maximum of %d - clipping\n",
		       ncores, NR_CPUS);
		ncores = NR_CPUS;
	}

	for (i = 0; i < ncores; i++)
		set_cpu_possible(i, true);

	//set_smp_cross_call(gic_raise_softirq);
}

void __cpuinit platform_smp_prepare_cpus_wakeup(void)
{
#if defined(CONFIG_HAVE_ARM_SCU)
	scu_enable(scu_base_addr());
#endif

	/*
	 * Write the address of secondary startup into the
	 * system-wide flags register. The boot monitor waits
	 * until it receives a soft interrupt, and then the
	 * secondary CPU branches to this address.
	 */

	/* use timer1 as global reg
		HLMT: physical addr of entry
		LLMT: magic (0xffffb007)
	 */

	__raw_writel(virt_to_phys(mt53xx_secondary_startup),
		 pBimIoMap+REG_RW_TIMER1_HLMT);
	__raw_writel(0xffffb007,
		 pBimIoMap+REG_RW_TIMER1_LLMT);

	/* make sure write buffer is drained */
	mb();

	/* Wakeup sleepers */
	asm volatile ("sev");
}

void __cpuinit platform_smp_prepare_cpus_boot(void)
{
	__raw_writel(virt_to_phys(mt53xx_secondary_startup),
		 pBimIoMap+REG_RW_TIMER1_HLMT);
	__raw_writel(0xffffb007,
		 pBimIoMap+REG_RW_TIMER1_LLMT);
}

unsigned int saved_max_cpus;
void __init mt53xx_smp_prepare_cpus(unsigned int max_cpus)
{
	int i;
	unsigned int ncores = mt53xx_get_core_count();
	/*
	 * Initialise the present map, which describes the set of CPUs
	 * actually populated at the present time.
	 */
	for (i = 0; i < max_cpus; i++)
		set_cpu_present(i, true);

#if defined(CONFIG_ARCH_MT5399)
	if (max_cpus == 1 && ((num_possible_cpus() > 1) || ncores > 1))
	{
#if defined(CONFIG_HIBERNATION) || defined(CONFIG_OPM)
		saved_max_cpus = max_cpus;
#endif
		// All others, go to sleep.
		__raw_writel(virt_to_phys(mt53xx_secondary_sleep),
			 BIM_VIRT+REG_RW_TIMER1_HLMT);
		__raw_writel(0xffffb007,
			 BIM_VIRT+REG_RW_TIMER1_LLMT);

		/* make sure write buffer is drained */
		mb();

		/* Wakeup sleepers */
		asm volatile ("sev");

		return;
	}
#endif

#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5882) || defined(CONFIG_ARCH_MT5883) || defined(CONFIG_ARCH_MT5891)
	saved_max_cpus = max_cpus;
	/* kill 3rd, 4th core when maxcpus=2 or 3 && CONFIG_NR_CPUS=4 */
	if (((num_possible_cpus() > max_cpus) || ncores > max_cpus))
	{

		printk("max_cpus: %d\n", max_cpus);
		printk("num_possible_cpus: %d\n", num_possible_cpus());
		printk("ncores: %d\n", ncores);

		i = num_possible_cpus();
		if (ncores > i)
			i = ncores;
		for (;i>max_cpus;i--)
		{
			printk("i:%d\n", i);
			platform_cpu_kill(i-1);
		}
	}
#endif

	platform_smp_prepare_cpus_wakeup();
}

#if defined(CONFIG_HIBERNATION) || defined(CONFIG_OPM)
/* similiar to mt53xx_smp_prepare_cpus */
void arch_disable_nonboot_cpus_early_resume(void)
{
	unsigned int ncores = mt53xx_get_core_count();

#if defined(CONFIG_ARCH_MT5399)
	if (saved_max_cpus == 1 && ((num_possible_cpus() > 1) || ncores > 1))
	{
		// All others, go to sleep.
		__raw_writel(virt_to_phys(mt53xx_secondary_sleep),
			 BIM_VIRT+REG_RW_TIMER1_HLMT);
		__raw_writel(0xffffb007,
			 BIM_VIRT+REG_RW_TIMER1_LLMT);

		/* make sure write buffer is drained */
		mb();

		/* Wakeup sleepers */
		asm volatile ("sev");

		return;
	}
#endif
#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5882) || defined(CONFIG_ARCH_MT5883) || defined(CONFIG_ARCH_MT5891)
	/* kill 3rd, 4th core when maxcpus=2 or 3 && CONFIG_NR_CPUS=4 */
	if (((num_possible_cpus() > saved_max_cpus) || ncores > saved_max_cpus))
	{
		int i;
		i = num_possible_cpus();
		if (ncores > i)
			i = ncores;
		disable_printk_in_caller = 1;
		for (;i>saved_max_cpus;i--)
		{
			platform_cpu_kill(i-1);
		}
		disable_printk_in_caller = 0;
	}
#endif

	platform_smp_prepare_cpus_wakeup();
	return;
}
#endif /* CONFIG_HIBERNATION */

struct smp_operations mt53xx_smp_ops __initdata = {
	.smp_init_cpus		= mt53xx_smp_init_cpus,
	.smp_prepare_cpus	= mt53xx_smp_prepare_cpus,
	.smp_secondary_init	= mt53xx_secondary_init,
	.smp_boot_secondary	= mt53xx_boot_secondary,
#ifdef CONFIG_HOTPLUG_CPU
	.cpu_die		= platform_cpu_die,
    .cpu_kill		= platform_cpu_kill,
#endif
};

#else
#define SCU_CONFIG		0x04

static int __init mt53xx_up_check(void)
{
	unsigned int ncores = 0;

	ncores = mt53xx_get_core_count();

	if (ncores > 1)
	{
		// If there's more then 1 cores, ask all others to go to sleep.
		__raw_writel(virt_to_phys(mt53xx_secondary_sleep),
			 BIM_VIRT+REG_RW_TIMER1_HLMT);
		__raw_writel(0xffffb007,
			 BIM_VIRT+REG_RW_TIMER1_LLMT);

		/* make sure write buffer is drained */
		mb();

		/* Wakeup sleepers */
		asm volatile ("sev");
	}

	return 0;
}
early_initcall(mt53xx_up_check);
void mt53xx_resume_up_check(void)
{
	unsigned int ncores = mt53xx_get_core_count();

	if (ncores > 1)
	{
		// If there's more then 1 cores, ask all others to go to sleep.
		__raw_writel(virt_to_phys(mt53xx_secondary_sleep),
			 BIM_VIRT+REG_RW_TIMER1_HLMT);
		__raw_writel(0xffffb007,
			 BIM_VIRT+REG_RW_TIMER1_LLMT);

		/* make sure write buffer is drained */
		mb();

		/* Wakeup sleepers */
		asm volatile ("sev");
	}
}
#endif /* CONFIG_SMP */
