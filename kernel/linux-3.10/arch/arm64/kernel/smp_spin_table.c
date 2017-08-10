/*
 * Spin Table SMP initialisation
 *
 * Copyright (C) 2013 ARM Ltd.
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/delay.h>
#include <linux/init.h>
#include <linux/of.h>
#include <linux/smp.h>
#include <linux/types.h>

#include <asm/cacheflush.h>
#include <asm/cpu_ops.h>
#include <asm/cputype.h>
#include <asm/smp_plat.h>
#include <asm/io.h>

#define HOTPLUG_SW_SRC  0xfb008000
#define HOTPLUG_SW_DST  0xFFFFFFC000000000
#define HOTPLUG_SW_SIZE 0x1000

extern void secondary_holding_pen(void);
volatile unsigned long secondary_holding_pen_release = INVALID_HWID;

extern void __ca53_cpu_die(void);
static phys_addr_t cpu_release_addr[NR_CPUS];
static DEFINE_RAW_SPINLOCK(boot_lock);

static void __ca53_cpu_power_control(unsigned int cpu, unsigned int en)
{
	__le64 __iomem *map_addr;
    unsigned int val, offset;
    unsigned char *cpu_power_addr;

    if (cpu == 1) {
        offset = 0x8;
    } else if (cpu == 2) {
        offset = 0xc;
    }
    else if (cpu == 3) {
        offset = 0x10;        
    } else {
        printk("cpu %d does not support power control\n");
        return;
    }

	map_addr = ioremap(0xf0008400, 0x20);
	if (!map_addr) {
    	    printk("ioremap failed.\n");
    		return;
    	}

    cpu_power_addr = map_addr;

    val = readl(cpu_power_addr + offset);

    if (en == 1) {
            // rst_en = 0
            val &= ~(1<<5);
        }
    else {
            // rst_en = 1
            val |= (1<<5);
        }

    writel(val, cpu_power_addr + offset);

	iounmap(map_addr);        
}

static int smp_hotplug_sw_prepare(void)
{
	__le64 __iomem *src_addr, *dst_addr;
    static int is_first = 1;
    
    if (is_first) {
        src_addr = ioremap(HOTPLUG_SW_SRC, HOTPLUG_SW_SIZE);
        dst_addr = HOTPLUG_SW_DST;

        if (!src_addr || !dst_addr)
            return -ENOMEM;

        memcpy(dst_addr, src_addr, HOTPLUG_SW_SIZE);
        
        __flush_dcache_area((__force void *)dst_addr,
                    HOTPLUG_SW_SIZE);

        iounmap(src_addr);

        is_first = 0;
    }

    return 0;
}
static void write_pen_release(u64 val)
{
	void *start = (void *)&secondary_holding_pen_release;
	unsigned long size = sizeof(secondary_holding_pen_release);

	secondary_holding_pen_release = val;
	__flush_dcache_area(start, size);
}
static int smp_spin_table_cpu_init(struct device_node *dn, unsigned int cpu)
{
	/*
	 * Determine the address from which the CPU is polling.
	 */
	if (of_property_read_u64(dn, "cpu-release-addr",
				 &cpu_release_addr[cpu])) {
		pr_err("CPU %d: missing or invalid cpu-release-addr property\n",
		       cpu);

		return -1;
	}

	return 0;
}

static int smp_spin_table_cpu_prepare(unsigned int cpu)
{
	__le64 __iomem *release_addr;

    smp_hotplug_sw_prepare();

	if (!cpu_release_addr[cpu])
		return -ENODEV;

	release_addr = ioremap(cpu_release_addr[cpu],
				     sizeof(*release_addr));
	if (!release_addr)
		return -ENOMEM;
	writeq_relaxed(__pa(secondary_holding_pen), release_addr);
	__flush_dcache_area((__force void *)release_addr,
			    sizeof(*release_addr));

	/*
	 * Send an event to wake up the secondary CPU.
	 */
	sev();

	iounmap(release_addr);
	return 0;
}

static int smp_spin_table_cpu_boot(unsigned int cpu)
{
	unsigned long timeout;

    __ca53_cpu_power_control(cpu, 1);
	/*
	 * Set synchronisation state between this boot processor
	 * and the secondary one
	 */
	raw_spin_lock(&boot_lock);

	/*
	 * Update the pen release flag.
	 */
	write_pen_release(cpu_logical_map(cpu));

	/*
	 * Send an event, causing the secondaries to read pen_release.
	 */
	sev();

	timeout = jiffies + (1 * HZ);
	while (time_before(jiffies, timeout)) {
		if (secondary_holding_pen_release == INVALID_HWID)
			break;
		udelay(10);
	}

	/*
	 * Now the secondary core is starting up let it run its
	 * calibrations, then wait for it to finish
	 */
	raw_spin_unlock(&boot_lock);

	return secondary_holding_pen_release != INVALID_HWID ? -ENOSYS : 0;
}

static void smp_spin_table_cpu_postboot(void)
{
	/*
	 * Let the primary processor know we're out of the pen.
	 */
	write_pen_release(INVALID_HWID);

	/*
	 * Synchronise with the boot thread.
	 */
	raw_spin_lock(&boot_lock);
	raw_spin_unlock(&boot_lock);
}

#ifdef CONFIG_HOTPLUG_CPU
static int smp_spin_table_cpu_disable(unsigned int cpu)
{
	/* Fail early if we don't have CPU_OFF support */
	return 0;
}

static void smp_spin_table_cpu_die(unsigned int cpu)
{
    //printk("smp_spin_table_cpu_die is called.\n");
    __ca53_cpu_die();
}

static int smp_spin_table_cpu_kill(unsigned int cpu)
{
	int i;

	/*
	 * cpu_kill could race with cpu_die and we can
	 * potentially end up declaring this cpu undead
	 * while it is dying. So, try again a few times.
	 */

	for (i = 0; i < 10; i++) {
		msleep(30);
	}

    __ca53_cpu_power_control(cpu, 0);

	return 1;
}
#endif

const struct cpu_operations smp_spin_table_ops = {
	.name		= "spin-table",
	.cpu_init	= smp_spin_table_cpu_init,
	.cpu_prepare	= smp_spin_table_cpu_prepare,
	.cpu_boot	= smp_spin_table_cpu_boot,
	.cpu_postboot	= smp_spin_table_cpu_postboot,
#ifdef CONFIG_HOTPLUG_CPU
	.cpu_disable	= smp_spin_table_cpu_disable,
	.cpu_die	= smp_spin_table_cpu_die,
	.cpu_kill	= smp_spin_table_cpu_kill,
#endif
};
