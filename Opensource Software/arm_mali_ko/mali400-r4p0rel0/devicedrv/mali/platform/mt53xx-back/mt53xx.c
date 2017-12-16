/*
 * This confidential and proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 * (C) COPYRIGHT 2009-2010, 2012 ARM Limited
 * ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 */

/**
 * @file mali_platform.c
 * Platform specific Mali driver functions for the TCC8900 platform
 */
#include <linux/platform_device.h>
#include <linux/version.h>
#include <linux/pm.h>
#ifdef CONFIG_PM_RUNTIME
#include <linux/pm_runtime.h>
#endif
#include <linux/mali/mali_utgard.h>
#include "mali_kernel_common.h"

static void mali_platform_device_release(struct device *device);

#define MALI_GP_IRQ       85
#define MALI_PP0_IRQ      86
#define MALI_PP1_IRQ      110

#define MALI_GP_MMU_IRQ   88
#define MALI_PP0_MMU_IRQ  89
#define MALI_PP1_MMU_IRQ  111

static struct resource mali_gpu_resources[] =
{
	MALI_GPU_RESOURCES_MALI400_MP2(0xF0040000, MALI_GP_IRQ, MALI_GP_MMU_IRQ, MALI_PP0_IRQ, MALI_PP0_MMU_IRQ, MALI_PP1_IRQ, MALI_PP1_MMU_IRQ)
};


static struct mali_gpu_device_data mali_gpu_data =
{
#if 0
	/* Dedicated memory setup (not sure if this is actually reserved on the platforms any more) */
	.dedicated_mem_start = 0x48A00000, /* Physical start address */
	.dedicated_mem_size = 0x07800000, /* 120MB */
#endif
	.shared_mem_size = 300 * 1024 * 1024, /* 96MB */
	.fb_start = 0x48200000,
	.fb_size = 0x01000000,
};

static struct platform_device mali_gpu_device =
{
	.name = MALI_GPU_NAME_UTGARD,
	.id = 0,
	.dev.release = mali_platform_device_release,
};


int mali_platform_device_register(void)
{
	int err;
	MALI_DEBUG_PRINT(4, ("mali_platform_device_register() called\n"));

	/* Connect resources to the device */
	err = platform_device_add_resources(&mali_gpu_device, mali_gpu_resources, sizeof(mali_gpu_resources) / sizeof(mali_gpu_resources[0]));
	if (0 == err)
	{
		err = platform_device_add_data(&mali_gpu_device, &mali_gpu_data, sizeof(mali_gpu_data));
		if (0 == err)
		{
			/* Register the platform device */
			err = platform_device_register(&mali_gpu_device);
			if (0 == err)
			{
#ifdef CONFIG_PM_RUNTIME
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
				pm_runtime_set_autosuspend_delay(&(mali_gpu_device.dev), 1000);
				pm_runtime_use_autosuspend(&(mali_gpu_device.dev));
#endif
				pm_runtime_enable(&(mali_gpu_device.dev));
#endif

				return 0;
			}
		}

		platform_device_unregister(&mali_gpu_device);
	}

	return err;
}

void mali_platform_device_unregister(void)
{
	MALI_DEBUG_PRINT(4, ("mali_platform_device_unregister() called\n"));

	platform_device_unregister(&mali_gpu_device);
}

static void mali_platform_device_release(struct device *device)
{
	MALI_DEBUG_PRINT(4, ("mali_platform_device_release() called\n"));
}
