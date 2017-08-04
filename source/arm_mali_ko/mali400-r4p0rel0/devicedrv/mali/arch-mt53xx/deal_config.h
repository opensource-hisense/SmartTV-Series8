/*
 * This confidential and proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 * (C) COPYRIGHT 2008-2010 ARM Limited
 * ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 */

#ifndef __ARCH_CONFIG_H__
#define __ARCH_CONFIG_H__

/* Configuration for the EB platform with ZBT memory enabled */

static _mali_osk_resource_t arch_configuration [] =
{
	{
		.type = MALI400GP,
		.description = "Mali-400 GP",
		.base = 0xf0040000,
		.irq = (53+32),
		.mmu_id = 1
	},
	{
		.type = MALI400PP,
		.base = 0xf0048000,
		.irq = (54+32),
		.description = "Mali-400 PP",
		.mmu_id = 2
	},
	{
		.type = MALI400PP,
		.base = 0xf004A000,
		.irq = (78+32),
		.description = "Mali-400 PP 1",
		.mmu_id = 3
	},
#if USING_MMU
	{
		.type = MMU,
		.base = 0xf0043000,
		.irq = (56+32),
		.description = "Mali-400 MMU for GP",
		.mmu_id = 1
	},
	{
		.type = MMU,
		.base = 0xf0044000,
		.irq = (57+32),
		.description = "Mali-400 MMU for PP",
		.mmu_id = 2
	},
	{
		.type = MMU,
		.base = 0xf0045000,
		.irq = (79+32),
		.description = "Mali-400 MMU for PP 1",
		.mmu_id = 3
	},
#endif
#if 0
	{
		.type = MEMORY,
		.description = "Mali SDRAM remapped to baseboard",
		.cpu_usage_adjust = 0,
		.alloc_order = 0, /* Highest preference for this memory */
		.base = 0xD04B000,
		.size = 128 * 1024 * 1024,
		.flags = _MALI_CPU_WRITEABLE | _MALI_CPU_READABLE | _MALI_PP_READABLE | _MALI_PP_WRITEABLE |_MALI_GP_READABLE | _MALI_GP_WRITEABLE
	},
#else
      {
		.type = OS_MEMORY,
		.description = "OS Memory",
		.cpu_usage_adjust = 0,		
		.alloc_order = 10, /* Highest preference for this memory */
		.size = 300 * 1024 * 1024,
		.flags = _MALI_CPU_WRITEABLE | _MALI_CPU_READABLE | _MALI_PP_READABLE | _MALI_PP_WRITEABLE |_MALI_GP_READABLE | _MALI_GP_WRITEABLE
       },
#endif
	{
		.type = MEM_VALIDATION,
		.description = "Framebuffer",
//                .base = 0x0FBFB000,
                .base = 0x0CBFB000,
		.size = 0x01000000, // 0x02000000,
		.flags = _MALI_CPU_WRITEABLE | _MALI_CPU_READABLE | _MALI_PP_WRITEABLE | _MALI_PP_READABLE
	},
	{
		.type = MALI400L2,
		.base = 0xf0041000,
		.description = "Mali-400 L2 cache"
	},
};

#endif /* __ARCH_CONFIG_H__ */
