/*
 * clock scaling for the UniCore-II
 *
 * Code specific to PKUnity SoC and UniCore ISA
 *
 *	Maintained by GUAN Xue-tao <gxt@mprc.pku.edu.cn>
 *	Copyright (C) 2001-2010 Guan Xuetao
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/clk.h>
#include <linux/cpufreq.h>

#include <mach/hardware.h>

static struct cpufreq_driver ucv2_driver;
typedef unsigned int UINT32;
#define IO_VIRT 0xF0000000
#define IO_CKGEN_BASE (IO_VIRT + 0xD000)
#define CKGEN_ANA_PLLGP_CFG0 (IO_CKGEN_BASE + 0x5E0)

#define CPU_WRITE32(offset, _val_)     (*((volatile UINT32*)(IO_CKGEN_BASE + offset)) = (_val_))
#define CPU_READ32(offset)             (*((volatile UINT32*)(IO_CKGEN_BASE + offset)))

static struct cpufreq_driver ucv2_driver;



/* make sure that only the "userspace" governor is running
 * -- anything else wouldn't make sense on this platform, anyway.
 */


int ucv2_verify_speed(struct cpufreq_policy *policy)
{
	return 0;
}

static unsigned int ucv2_getspeed(unsigned int cpu)
{
	
	int val = 0;
	unsigned int hpcpll;
		
	val = CPU_READ32(0x5E0);
	val = (val >> 24) & 0x7f;
	hpcpll = (24000 * (val)) / 2;
	
	return hpcpll;
	
}

static int ucv2_target(struct cpufreq_policy *policy,
			 unsigned int target_freq,
			 unsigned int relation)
{
	return 0;
}

static int __init ucv2_cpu_init(struct cpufreq_policy *policy)
{
	
	policy->cpuinfo.max_freq = 1500000;
	policy->cpuinfo.min_freq = 1100000;
	return 0;
}

static struct cpufreq_driver ucv2_driver = {
	.flags		= CPUFREQ_STICKY,
	.verify		= ucv2_verify_speed,
	.target		= ucv2_target,
	.get		= ucv2_getspeed,
	.init		= ucv2_cpu_init,
	.name		= "UniCore-II",
};

static int __init ucv2_cpufreq_init(void)
{
	return cpufreq_register_driver(&ucv2_driver);
}

arch_initcall(ucv2_cpufreq_init);

