/*
 * drivers/opm/opm_sample.c 
 *
 * ordered power management devices
 *
 * Copyright (c) 2010-2012 MediaTek Inc.
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
#include <linux/opm_device.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/err.h>

static struct opm_device *device[4];

static int opmsample_probe(struct opm_device *devptr)
{
	printk("opmsample_probe %s-%d\n",devptr->dev.driver->name, devptr->id);
	return 0;
}

static int opmsample_remove(struct opm_device *devptr)
{
	printk("opmsample_remove %s-%d\n",devptr->dev.driver->name, devptr->id);
//	opm_get_drvdata(devptr);
//	opm_set_drvdata(devptr, NULL);
	return 0;
}

int opmsample_suspend(struct opm_device *devptr, pm_message_t state)
{
	printk("opmsample_suspend %s-%d,order[%d/%d]\n",devptr->dev.driver->name,devptr->id,
		devptr->suspend_order,devptr->resume_order);
	return 0;
}
int opmsample_resume(struct opm_device *devptr)
{
	printk("opmsample_resume %s-%d,order[%d/%d]\n",devptr->dev.driver->name, devptr->id,
		devptr->suspend_order,devptr->resume_order);
	return 0;
}
		
static struct opm_driver opmsample_driver1 = {
	.probe		= opmsample_probe,
	.remove		= opmsample_remove,
	.suspend	= opmsample_suspend,
	.resume		= opmsample_resume,
	.driver		= {
		.name	= "opmsample1"
	},
};

static struct opm_driver opmsample_driver2 = {
	.probe		= opmsample_probe,
	.remove		= opmsample_remove,
	.suspend	= opmsample_suspend,
	.resume		= opmsample_resume,
	.driver		= {
		.name	= "opmsample2"
	},
};

static struct opm_driver opmsample_driver3 = {
	.probe		= opmsample_probe,
	.remove		= opmsample_remove,
	.suspend	= opmsample_suspend,
	.resume		= opmsample_resume,
	.driver		= {
		.name	= "opmsample3"
	},
};
static struct opm_driver opmsample_driver4 = {
	.probe		= opmsample_probe,
	.remove		= opmsample_remove,
	.suspend	= opmsample_suspend,
	.resume		= opmsample_resume,
	.driver		= {
		.name	= "opmsample4"
	},
};

void opmcls_shutdown(struct opm_device *devptr)
{
	printk("opmcls_shutdown\n");
}

static struct opm_driver opmcls_driver = {
	.shutdown	= opmcls_shutdown,
	.driver		= {
		.name	= "opm_sysclass"
	},
};

static int __init opmsample_init(void)
{
	int err;

	if ((err = opm_driver_register(&opmsample_driver1)) < 0)
		return err;
	if ((err = opm_driver_register(&opmsample_driver2)) < 0)
		return err;
	if ((err = opm_driver_register(&opmsample_driver3)) < 0)
		return err;
	if ((err = opm_driver_register(&opmsample_driver4)) < 0)
		return err;

	if ((err = opm_driver_register(&opmcls_driver)) < 0)
		return err;
	device[0] = opm_device_register_simple("opmsample1",0,ORDER_IGNORE);
	device[1] = opm_device_register_simple("opmsample2",0,4);
	device[2] = opm_device_register_simple("opmsample3",0,2);
	device[3] = opm_device_register_simple("opmsample4",0,3);
	
	//if (IS_ERR(device))
	//	return -ENODEV;
							 
	return 0;
}

static void __exit opmsample_exit(void)
{
	opm_device_unregister(device[0]);
	opm_device_unregister(device[1]);
	opm_device_unregister(device[2]);
	opm_device_unregister(device[3]);
	opm_driver_unregister(&opmsample_driver1);
	opm_driver_unregister(&opmsample_driver2);
	opm_driver_unregister(&opmsample_driver3);
	opm_driver_unregister(&opmsample_driver4);
	opm_driver_unregister(&opmcls_driver);
}

module_init(opmsample_init);
module_exit(opmsample_exit)
