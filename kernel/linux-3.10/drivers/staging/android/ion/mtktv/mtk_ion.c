/*
 * drivers/gpu/tegra/tegra_ion.c
 *
 * Copyright (C) 2011 Google, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/err.h>
//#include <linux/ion.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/export.h>
#include "../ion.h"
#include "../ion_priv.h"

struct ion_device *idev;
struct ion_mapper *tegra_user_mapper;
int num_heaps;
struct ion_heap **mtkionheaps;
#ifdef LIMIT_GPU_TO_ChannelX
extern int mt53xx_get_reserve_memory(unsigned int *start, unsigned int *size, unsigned int *ionsize);
#endif

//static struct ion_heap **gapsIonHeaps;

#ifndef ION_CARVEOUT_MEM_BASE
#define ION_CARVEOUT_MEM_BASE 0
#endif

#ifndef ION_CARVEOUT_MEM_SIZE
#define ION_CARVEOUT_MEM_SIZE 0
#endif

int tegra_ion_probe(struct platform_device *pdev)
{
	struct ion_platform_data *pdata = pdev->dev.platform_data;
	int err;
	int i;
	unsigned int addr =0;
	unsigned int size =0;
	unsigned int ionsize =0;

	num_heaps = pdata->nr;

	mtkionheaps = kzalloc(sizeof(struct ion_heap *) * pdata->nr, GFP_KERNEL);

	printk(KERN_ALERT "ion_device_create  num_heaps = %d \n", num_heaps);
#ifdef LIMIT_GPU_TO_ChannelX
	if(0 == mt53xx_get_reserve_memory(&addr, &size, &ionsize))
	{
	   printk("ion using memory addr=0x%x, ionsize=0x%x\n", addr, ionsize);
	}
#endif

	idev = ion_device_create(NULL);
	if (IS_ERR_OR_NULL(idev)) {
		kfree(mtkionheaps);
		return PTR_ERR(idev);
	}

	/* create the heaps as specified in the board file */
	for (i = 0; i < num_heaps; i++) {
		struct ion_platform_heap *heap_data = &pdata->heaps[i];
		if(heap_data->type == ION_HEAP_TYPE_CARVEOUT)
		{
		  heap_data->base = addr;
		  heap_data->size = ionsize;
		     if(ionsize == 0) 
		 	{
		 	  heap_data->base =0;
			  continue;
		     	}
		 
		}
		
		mtkionheaps[i] = ion_heap_create(heap_data);
		if (IS_ERR_OR_NULL(mtkionheaps[i])) {
			err = PTR_ERR(mtkionheaps[i]);
			goto err;
		}
		ion_device_add_heap(idev, mtkionheaps[i]);
	}
	platform_set_drvdata(pdev, idev);
	return 0;
err:
	for (i = 0; i < num_heaps; i++) {
		if (mtkionheaps[i])
			ion_heap_destroy(mtkionheaps[i]);
	}
	kfree(mtkionheaps);
	return err;
}

int tegra_ion_remove(struct platform_device *pdev)
{
	struct ion_device *idev = platform_get_drvdata(pdev);
	int i;

	ion_device_destroy(idev);
	for (i = 0; i < num_heaps; i++)
		ion_heap_destroy(mtkionheaps[i]);
	kfree(mtkionheaps);
	return 0;
}


struct ion_device *get_ion_device()
{
    return idev;
}

EXPORT_SYMBOL(get_ion_device);


static struct platform_driver ion_driver = {
	.probe = tegra_ion_probe,
	.remove = tegra_ion_remove,
	.driver = { .name = "ion-tegra" }
};

static struct ion_platform_heap gsHeap[] =
{
	{
		.type = ION_HEAP_TYPE_SYSTEM_CONTIG,
		.name = "system_contig",
		.id   = 2,
	},
	{
		.type = ION_HEAP_TYPE_SYSTEM,
		.name = "system",
		.id   = ION_HEAP_TYPE_SYSTEM,
	},
	{
		.type = ION_HEAP_TYPE_CARVEOUT,
		.name = "carveout",
		.id   = 1,
		.base = ION_CARVEOUT_MEM_BASE,
		.size = ION_CARVEOUT_MEM_SIZE,
	},
};


static struct ion_platform_data gsGenericConfig =
{
	.nr = 3,
	.heaps = gsHeap,
};

static struct platform_device gsIonDevice = 
{
    .name = "ion-tegra",
	.id = 0,
	.num_resources = 0,
	.dev = {
	    .platform_data = &gsGenericConfig,
	}
};

static int __init ion_init(void)
{
	int ret;

	ret = platform_driver_register(&ion_driver);
	
		if (!ret) {
			ret = platform_device_register(&gsIonDevice);
			if (ret)
				platform_driver_unregister(&ion_driver);
		}
		return ret;
}

static void __exit ion_exit(void)
{
	platform_driver_unregister(&ion_driver);
	platform_device_unregister(&gsIonDevice);
}

module_init(ion_init);
module_exit(ion_exit);

