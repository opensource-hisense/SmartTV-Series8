/*
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Marius Groeger <mgroeger@sysgo.de>
 *
 * Copyright (C) 2001  Erik Mouw (J.A.K.Mouw@its.tudelft.nl)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307	 USA
 *
 */

#include <common.h>
#include <command.h>
#include <image.h>
#include <u-boot/zlib.h>
#include <asm/byteorder.h>
#include <fdt.h>
#include <libfdt.h>
#include <fdt_support.h>

#if defined(CC_TRUSTZONE_SUPPORT) || defined (CC_TRUSTZONE_64_SUPPORT)
#include "drvcust_if.h"
#include "c_model.h"
#endif

#include "x_bim.h"

DECLARE_GLOBAL_DATA_PTR;

#if defined (CONFIG_SETUP_MEMORY_TAGS) || \
    defined (CONFIG_CMDLINE_TAG) || \
    defined (CONFIG_INITRD_TAG) || \
    defined (CONFIG_SERIAL_TAG) || \
    defined (CONFIG_REVISION_TAG)
static void setup_start_tag (bd_t *bd);

# ifdef CONFIG_SETUP_MEMORY_TAGS
static void setup_memory_tags (bd_t *bd);
# endif
static void setup_commandline_tag (bd_t *bd, char *commandline);

# ifdef CONFIG_INITRD_TAG
static void setup_initrd_tag (bd_t *bd, ulong initrd_start,
			      ulong initrd_end);
# endif
static void setup_end_tag (bd_t *bd);

static struct tag *params;
#endif /* CONFIG_SETUP_MEMORY_TAGS || CONFIG_CMDLINE_TAG || CONFIG_INITRD_TAG */

static ulong get_sp(void);
#if defined(CONFIG_OF_LIBFDT)
static int bootm_linux_fdt(int machid, bootm_headers_t *images);
#endif

void arch_lmb_reserve(struct lmb *lmb)
{
	ulong sp;

	/*
	 * Booting a (Linux) kernel image
	 *
	 * Allocate space for command line and board info - the
	 * address should be as high as possible within the reach of
	 * the kernel (see CONFIG_SYS_BOOTMAPSZ settings), but in unused
	 * memory, which means far enough below the current stack
	 * pointer.
	 */
	sp = get_sp();
	debug("## Current stack ends at 0x%08lx ", sp);

	/* adjust sp by 1K to be safe */
	sp -= 1024;
	lmb_reserve(lmb, sp,
		    gd->bd->bi_dram[0].start + gd->bd->bi_dram[0].size - sp);
}

static void announce_and_cleanup(void)
{
	printf("\nStarting kernel ...\n\n");

#ifdef CONFIG_USB_DEVICE
	{
		extern void udc_disconnect(void);
		udc_disconnect();
	}
#endif

    {
        extern void turn_off_USB_power_switch(void);
        turn_off_USB_power_switch();
    }

	cleanup_before_linux();
}

#include "x_dram.h"
int do_bootm_linux(int flag, int argc, char *argv[], bootm_headers_t *images)
{
	bd_t	*bd = gd->bd;
	char	*s;
	int	machid = bd->bi_arch_number;
	char *emmc_log;

#if defined(CC_TRUSTZONE_SUPPORT) || defined(CC_TRUSTZONE_64_SUPPORT)
	void	(*kernel_entry)(int zero, int arch, uint params, uint kernel_entry);
#else
	void	(*kernel_entry)(int zero, int arch, uint params);
#endif // CC_TRUSTZONE_SUPPORT

#ifdef CONFIG_CMDLINE_TAG
	char *commandline = getenv ("bootargs");
#endif
    emmc_log   = getenv("emmclog");

	if ((flag != 0) && (flag != BOOTM_STATE_OS_GO))
		return 1;

	s = getenv ("machid");
	if (s) {
		machid = simple_strtoul (s, NULL, 16);
		printf ("Using machid 0x%x from environment\n", machid);
	}

	show_boot_progress (15);

#ifdef CONFIG_OF_LIBFDT
	if (images->ft_len)
		return bootm_linux_fdt(machid, images);
#endif

#if defined (CC_TRUSTZONE_SUPPORT) || (CC_TRUSTZONE_64_SUPPORT)
#if defined(CC_TRUSTZONE_IN_CHB)
    kernel_entry = (void (*)(int, int, uint, uint))(TCMGET_CHANNELA_SIZE()*0x100000+TCMGET_CHANNELB_SIZE()*0x100000-TRUSTZONE_MEM_SIZE+TRUSTZONE_CODE_BASE);
#else
	kernel_entry = (void (*)(int, int, uint, uint))(TOTAL_MEM_SIZE - TRUSTZONE_MEM_SIZE + TRUSTZONE_CODE_BASE);
#endif
#else
	kernel_entry = (void (*)(int, int, uint))images->ep;
#endif // CC_TRUSTZONE_SUPPORT

	debug ("## Transferring control to Linux (at address %08lx) ...\n",
	       (ulong) kernel_entry);

#if defined (CONFIG_SETUP_MEMORY_TAGS) || \
    defined (CONFIG_CMDLINE_TAG) || \
    defined (CONFIG_INITRD_TAG) || \
    defined (CONFIG_SERIAL_TAG) || \
    defined (CONFIG_REVISION_TAG)
	setup_start_tag (bd);
#ifdef CONFIG_SERIAL_TAG
	setup_serial_tag (&params);
#endif
#ifdef CONFIG_REVISION_TAG
	setup_revision_tag (&params);
#endif
#ifdef CONFIG_SETUP_MEMORY_TAGS
	setup_memory_tags (bd);
#endif
#ifdef CONFIG_CMDLINE_TAG
	setup_commandline_tag (bd, commandline);
#endif
#ifdef CONFIG_INITRD_TAG
	if (images->rd_start && images->rd_end)
		setup_initrd_tag (bd, images->rd_start, images->rd_end);
#endif
	setup_end_tag(bd);
#endif
	BIM_SetTimeLog(3);

	announce_and_cleanup();
#ifdef CC_TRUSTZONE_64_SUPPORT
	transform_cpu_to_64bit(0,machid,bd->bi_boot_params,images->ep, kernel_entry);
#elif defined(CC_TRUSTZONE_SUPPORT)
	kernel_entry(0, machid, bd->bi_boot_params, images->ep);
#else
	kernel_entry(0, machid, bd->bi_boot_params);
#endif
	/* does not return */

	return 1;
}

#if defined(CONFIG_OF_LIBFDT)
static int fixup_memory_node(void *blob)
{
	bd_t	*bd = gd->bd;
	
	int bank,bank_nr = CONFIG_NR_DRAM_BANKS;
	u64 start[CONFIG_NR_DRAM_BANKS];
	u64 size[CONFIG_NR_DRAM_BANKS];
		char *s;
#if defined(CC_DYNAMIC_FBMSRM_CONF)
		if(FBMDynIs4KMode())
		{
			s = getenv("kmem4ksize");
		}
		else
		{
			s = getenv("kmemfhdsize");
		}
#else
			s = getenv("kmemsize");
#endif
			if (s)
			{
				bd->bi_dram[0].size = simple_strtoul (s, NULL, 16);
			}
#if CONFIG_NR_DRAM_BANKS>1
			s = getenv("kmem2start");
			if (s)
			{
				bd->bi_dram[1].start = simple_strtoul (s, NULL, 16);
			}
#if defined(CC_DYNAMIC_FBMSRM_CONF)
			if(FBMDynIs4KMode())
			{
				s = getenv("kmem24ksize");
			}
			else
			{
				s = getenv("kmem2fhdsize");
			}
#else
				s = getenv("kmem2size");
#endif
		
			if (s)
			{
				bd->bi_dram[1].size = simple_strtoul (s, NULL, 16);
			}

#endif

#ifdef CC_SUPPORT_CHANNEL_C
				s = getenv("kmem3start");
				if (s)
				{
					bd->bi_dram[2].start = simple_strtoul (s, NULL, 16);
				}
				s = getenv("kmem3size");
				if (s)
				{
					bd->bi_dram[2].size = simple_strtoul (s, NULL, 16);
				}
				
#ifdef CC_MT5891
					if(BSP_GetIcVersion() == IC_VER_5891_AA)
						{
							bank_nr = 2;
						}
#endif
				
#endif
		for (bank = 0; bank < bank_nr; bank++) {

		start[bank] = bd->bi_dram[bank].start;
		size[bank] = bd->bi_dram[bank].size;
		
		printf ("## Linux start= 0x%08llx size= 0x%08llx ...\n",  start[bank],size[bank]);
	}

	return fdt_fixup_memory_banks(blob, start, size, bank_nr);
}

static int bootm_linux_fdt(int machid, bootm_headers_t *images)
{
	ulong rd_len;
#if defined(CC_TRUSTZONE_SUPPORT) || defined(CC_TRUSTZONE_64_SUPPORT)
		void	(*kernel_entry)(int zero, int arch, uint params, uint kernel_entry);
#else
		void	(*kernel_entry)(int zero, int arch, uint params);
#endif // CC_TRUSTZONE_SUPPORT
	ulong of_size = images->ft_len;
	char **of_flat_tree = &images->ft_addr;
	ulong *initrd_start = &images->rd_start;
	ulong *initrd_end = &images->rd_end;
	struct lmb *lmb = &images->lmb;
	int ret;

	kernel_entry = (void (*)(int, int, void *))images->ep;
#if defined (CC_TRUSTZONE_SUPPORT) || (CC_TRUSTZONE_64_SUPPORT)
#if defined(CC_TRUSTZONE_IN_CHB)
		kernel_entry = (void (*)(int, int, uint, uint))(TCMGET_CHANNELA_SIZE()*0x100000+TCMGET_CHANNELB_SIZE()*0x100000-TRUSTZONE_MEM_SIZE+TRUSTZONE_CODE_BASE);
#else
		kernel_entry = (void (*)(int, int, uint, uint))(TOTAL_MEM_SIZE - TRUSTZONE_MEM_SIZE + TRUSTZONE_CODE_BASE);
#endif
#else
		kernel_entry = (void (*)(int, int, uint))images->ep;
#endif // CC_TRUSTZONE_SUPPORT

	boot_fdt_add_mem_rsv_regions(lmb, *of_flat_tree);

	rd_len = images->rd_end - images->rd_start;
	#if 0
	ret = boot_ramdisk_high(lmb, images->rd_start, rd_len,
				initrd_start, initrd_end);
	if (ret)
		return ret;
	#endif
	ret = boot_relocate_fdt(lmb, of_flat_tree, &of_size);
	if (ret)
		return ret;

	printf("## Transferring control to Linux (at address %08lx) ...\n",
	       (ulong) kernel_entry);

	fdt_chosen(*of_flat_tree, 1);

	fixup_memory_node(*of_flat_tree);

	//fdt_fixup_ethernet(*of_flat_tree);

	fdt_initrd(*of_flat_tree, *initrd_start, *initrd_end, 1);
	BIM_SetTimeLog(3);

	announce_and_cleanup();
	//run_command("fdt l /memory", 0);
	//run_command("fdt l /chosen", 0);
#ifdef CC_TRUSTZONE_64_SUPPORT
		imagesep = images->ep;
		kernelentry = kernel_entry;

		transform_cpu_to_64bit(*of_flat_tree,machid,0);
#elif defined(CC_TRUSTZONE_SUPPORT)
		kernel_entry(0, machid, *of_flat_tree, images->ep);
#else
		kernel_entry(0, machid, *of_flat_tree);
#endif
	/* does not return */

	return 1;
}
#endif

#if defined (CONFIG_SETUP_MEMORY_TAGS) || \
    defined (CONFIG_CMDLINE_TAG) || \
    defined (CONFIG_INITRD_TAG) || \
    defined (CONFIG_SERIAL_TAG) || \
    defined (CONFIG_REVISION_TAG)
static void setup_start_tag (bd_t *bd)
{
	params = (struct tag *) bd->bi_boot_params;

	params->hdr.tag = ATAG_CORE;
	params->hdr.size = tag_size (tag_core);

	params->u.core.flags = 0;
	params->u.core.pagesize = 0;
	params->u.core.rootdev = 0;

	params = tag_next (params);
}


#ifdef CONFIG_SETUP_MEMORY_TAGS
static void setup_memory_tags (bd_t *bd)
{
	int i,bank_nr = CONFIG_NR_DRAM_BANKS;
	char *s;
#if defined(CC_DYNAMIC_FBMSRM_CONF)
if(FBMDynIs4KMode())
{
	s = getenv("kmem4ksize");
}
else
{
    s = getenv("kmemfhdsize");
}
#else
    s = getenv("kmemsize");
#endif
	if (s)
	{
		bd->bi_dram[0].size = simple_strtoul (s, NULL, 16);
	}
#if CONFIG_NR_DRAM_BANKS>1
	s = getenv("kmem2start");
	if (s)
	{
		bd->bi_dram[1].start = simple_strtoul (s, NULL, 16);
	}
#if defined(CC_DYNAMIC_FBMSRM_CONF)
	if(FBMDynIs4KMode())
	{
		s = getenv("kmem24ksize");
	}
	else
	{
		s = getenv("kmem2fhdsize");
	}
#else
		s = getenv("kmem2size");
#endif

	if (s)
	{
		bd->bi_dram[1].size = simple_strtoul (s, NULL, 16);
	}
#endif	
#ifdef CC_SUPPORT_CHANNEL_C
	s = getenv("kmem3start");
	if (s)
	{
		bd->bi_dram[2].start = simple_strtoul (s, NULL, 16);
	}
	s = getenv("kmem3size");
	if (s)
	{
		bd->bi_dram[2].size = simple_strtoul (s, NULL, 16);
	}
	
#ifdef CC_MT5891
	if(BSP_GetIcVersion() == IC_VER_5891_AA)
		{
			bank_nr = 2;
		}
#endif

#endif
	for (i = 0; i < bank_nr; i++) {
		params->hdr.tag = ATAG_MEM;
		params->hdr.size = tag_size (tag_mem32);

		params->u.mem.start = bd->bi_dram[i].start;
		params->u.mem.size = bd->bi_dram[i].size;

		params = tag_next (params);
	}
}
#endif /* CONFIG_SETUP_MEMORY_TAGS */


static void setup_commandline_tag (bd_t *bd, char *commandline)
{
	char *p;

	if (!commandline)
		return;

	/* eat leading white space */
	for (p = commandline; *p == ' '; p++);

	/* skip non-existent command lines so the kernel will still
	 * use its default command line.
	 */
	if (*p == '\0')
		return;

	params->hdr.tag = ATAG_CMDLINE;
	params->hdr.size =
		(sizeof (struct tag_header) + strlen (p) + 1 + 4) >> 2;

	strcpy (params->u.cmdline.cmdline, p);

	params = tag_next (params);
}


#ifdef CONFIG_INITRD_TAG
static void setup_initrd_tag (bd_t *bd, ulong initrd_start, ulong initrd_end)
{
	/* an ATAG_INITRD node tells the kernel where the compressed
	 * ramdisk can be found. ATAG_RDIMG is a better name, actually.
	 */
	params->hdr.tag = ATAG_INITRD2;
	params->hdr.size = tag_size (tag_initrd);

	params->u.initrd.start = initrd_start;
	params->u.initrd.size = initrd_end - initrd_start;

	params = tag_next (params);
}
#endif /* CONFIG_INITRD_TAG */

#ifdef CONFIG_SERIAL_TAG
void setup_serial_tag (struct tag **tmp)
{
	struct tag *params = *tmp;
	struct tag_serialnr serialnr;
	void get_board_serial(struct tag_serialnr *serialnr);

	get_board_serial(&serialnr);
	params->hdr.tag = ATAG_SERIAL;
	params->hdr.size = tag_size (tag_serialnr);
	params->u.serialnr.low = serialnr.low;
	params->u.serialnr.high= serialnr.high;
	params = tag_next (params);
	*tmp = params;
}
#endif

#ifdef CONFIG_REVISION_TAG
void setup_revision_tag(struct tag **in_params)
{
	u32 rev = 0;
	u32 get_board_rev(void);

	rev = get_board_rev();
	params->hdr.tag = ATAG_REVISION;
	params->hdr.size = tag_size (tag_revision);
	params->u.revision.rev = rev;
	params = tag_next (params);
}
#endif  /* CONFIG_REVISION_TAG */

static void setup_end_tag (bd_t *bd)
{
	params->hdr.tag = ATAG_NONE;
	params->hdr.size = 0;
}
#endif /* CONFIG_SETUP_MEMORY_TAGS || CONFIG_CMDLINE_TAG || CONFIG_INITRD_TAG */

static ulong get_sp(void)
{
	ulong ret;

	asm("mov %0, sp" : "=r"(ret) : );
	return ret;
}
