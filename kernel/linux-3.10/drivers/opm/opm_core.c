/*
 * drivers/opm/opm_core.c 
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
#include <linux/bootmem.h>
#include <linux/err.h>
#include <linux/slab.h>
//#include <linux/sysdev.h>
#include <linux/syscore_ops.h>


LIST_HEAD(opm_resume_nodes);
LIST_HEAD(opm_suspend_nodes);

#define to_opm_driver(drv)	(container_of((drv), struct opm_driver, \
				 driver))

struct device opm_bus = {
	.init_name		= "opm",
};
EXPORT_SYMBOL(opm_bus);


static struct opm_driver *sysclass_drv;

#if 0
static int opm_sysclass_shutdown(struct sys_device *sysdev)
{
	if (sysclass_drv&&sysclass_drv->shutdown)
		sysclass_drv->shutdown(0);
	return 0;
}
static int opm_sysclass_suspend(struct sys_device *sysdev, pm_message_t state)
{
	return (sysclass_drv&&sysclass_drv->suspend)?sysclass_drv->suspend(0,state):0;
}
static int opm_sysclass_resume(struct sys_device *sysdev)
{
	return (sysclass_drv&&sysclass_drv->resume)?sysclass_drv->resume(0):0;
}
#else
static int opm_sysclass_suspend(void)
{
	//return (sysclass_drv&&sysclass_drv->suspend)?sysclass_drv->suspend(0,state):0;
	printk("opm_sysclass_suspend\n");
	return 0;
}
static void opm_sysclass_resume(void)
{
	//return (sysclass_drv&&sysclass_drv->resume)?sysclass_drv->resume(0):0;
	printk("opm_sysclass_resume\n");
	//return 0;
}
#endif

	
#if 0
struct sysdev_class opm_sysclass = {
	.name = "opm",
	.shutdown = opm_sysclass_shutdown,
	.suspend = opm_sysclass_suspend,
	.resume = opm_sysclass_resume
};
static struct sys_device opm_sysdev = {
	.cls		= &opm_sysclass,
};
#else
static struct syscore_ops opm_syscore_ops = {
	.suspend	= opm_sysclass_suspend,
	.resume		= opm_sysclass_resume,
};
#endif


/**
 * opm_add_devices - add a numbers of opm devices
 * @devs: array of opm devices to add
 * @num: number of opm devices in array
 */
int opm_add_devices(struct opm_device **devs, int num)
{
	int i, ret = 0;

	for (i = 0; i < num; i++) {
		ret = opm_device_register(devs[i]);
		if (ret) {
			while (--i >= 0)
				opm_device_unregister(devs[i]);
			break;
		}
	}

	return ret;
}
EXPORT_SYMBOL(opm_add_devices);

struct opm_object {
	struct opm_device pdev;
	char name[1];
};


static void opm_device_release(struct device *dev)
{
	struct opm_object *pa = container_of(dev, struct opm_object,
						  pdev.dev);

	kfree(pa);
}

/**
 * opm_device_alloc
 * @name: base name of the device we're adding
 * @id: instance id
 *
 * Create a opm device object which can have other objects attached
 * to it, and which will have attached objects freed when it is released.
 */
struct opm_device *opm_device_alloc(const char *name, int id,
	int suspend_order,int resume_order)
{
	struct opm_object *pa;

	pa = kzalloc(sizeof(struct opm_object) + strlen(name), GFP_KERNEL);
	if (pa) {
		strcpy(pa->name, name);
		pa->pdev.name = pa->name;
		pa->pdev.id = id;
		device_initialize(&pa->pdev.dev);
		INIT_LIST_HEAD(&pa->pdev.s_node);
		INIT_LIST_HEAD(&pa->pdev.r_node);
		pa->pdev.suspend_order=suspend_order;
		pa->pdev.resume_order=resume_order;
		//pa->pdev.pm_action=ORDACT_NONE;
		pa->pdev.dev.release = opm_device_release;		
	}

	return pa ? &pa->pdev : NULL;
}
EXPORT_SYMBOL(opm_device_alloc);


/**
 * opm_device_add - add a opm device to device hierarchy
 * @pdev: opm device we're adding
 *
 * This is part 2 of opm_device_register(), though may be called
 * separately _iff_ pdev was allocated by opm_device_alloc().
 */
int opm_device_add(struct opm_device *pdev)
{
	struct opm_device *pos;
	int ret = 0;

	if (!pdev)
		return -EINVAL;

	if (!pdev->dev.parent)
		pdev->dev.parent = &opm_bus;

	pdev->dev.bus = &opm_bus_type;

	if (pdev->id != -1)
		dev_set_name(&pdev->dev, "%s.%d", pdev->name, pdev->id);
	else
		dev_set_name(&pdev->dev,  "%s", pdev->name);


	pr_debug("Registering opm device '%s'. Parent at %s\n",
		 dev_name(&pdev->dev), dev_name(pdev->dev.parent));

	ret = device_add(&pdev->dev);
	
	if(pdev->suspend_order>ORDER_IGNORE)
	{
		list_for_each_entry(pos,&opm_suspend_nodes,s_node)
		{
			if(pdev->suspend_order>pos->suspend_order)
				break;			
		}
		list_add(&pdev->s_node,&pos->s_node);
	}
	if(pdev->resume_order>ORDER_IGNORE)
	{
		list_for_each_entry(pos,&opm_resume_nodes,r_node)
		{
			if(pdev->resume_order>pos->resume_order)
				break;			
		}
		list_add(&pdev->r_node,&pos->r_node);
	}
	return ret;
}
EXPORT_SYMBOL(opm_device_add);

/**
 * opm_device_del - remove a opm-level device
 * @pdev: opm device we're removing
 *
 * Note that this function will also release all memory- and port-based
 * resources owned by the device (@dev->resource).  This function must
 * _only_ be externally called in error cases.  All other usage is a bug.
 */
void opm_device_del(struct opm_device *pdev)
{
	struct opm_device *pos;
	if (pdev) {
		if(pdev->suspend_order>ORDER_IGNORE)
		{
			list_for_each_entry(pos,&opm_suspend_nodes,s_node)
			{
				if(pos==pdev)
				{
					list_del(&pos->s_node);
					break;
				}		
			}
		}
		if(pdev->resume_order>ORDER_IGNORE)
		{
			list_for_each_entry(pos,&opm_resume_nodes,r_node)
			{
				if(pos==pdev)
				{
					list_del(&pos->r_node);
					break;
				}
			}
		}
		device_del(&pdev->dev);
	}
}
EXPORT_SYMBOL(opm_device_del);

/**
 * opm_device_register - add a opm-level device
 * @pdev: opm device we're adding
 */
int opm_device_register(struct opm_device *pdev)
{
	device_initialize(&pdev->dev);
	return opm_device_add(pdev);
}
EXPORT_SYMBOL(opm_device_register);

/**
 * opm_device_unregister - unregister a opm-level device
 * @pdev: opm device we're unregistering
 *
 * Unregistration is done in 2 steps. First we release all resources
 * and remove it from the subsystem, then we drop reference count by
 * calling opm_device_put().
 */
void opm_device_unregister(struct opm_device *pdev)
{
	opm_device_del(pdev);
	if (pdev)
		put_device(&pdev->dev);
}
EXPORT_SYMBOL(opm_device_unregister);


struct opm_device *opm_device_register_simple(const char *name,
							int id, int resume_order)
{
	struct opm_device *pdev;
	int retval;

	pdev = opm_device_alloc(name, id,
		(resume_order>0)?ORDER4SUSPEND(resume_order):resume_order, resume_order);
	if (!pdev) {
		retval = -ENOMEM;
		goto error;
	}

	retval = opm_device_add(pdev);
	if (retval)
		goto error;

	return pdev;

error:
	if (pdev)
		put_device(&pdev->dev);
	return ERR_PTR(retval);
}
EXPORT_SYMBOL(opm_device_register_simple);


static int opm_drv_probe(struct device *_dev)
{
	struct opm_driver *drv = to_opm_driver(_dev->driver);
	struct opm_device *dev = to_opm_device(_dev);

	return drv->probe(dev);
}


static int opm_drv_remove(struct device *_dev)
{
	struct opm_driver *drv = to_opm_driver(_dev->driver);
	struct opm_device *dev = to_opm_device(_dev);

	return drv->remove(dev);
}

static void opm_drv_shutdown(struct device *_dev)
{
	struct opm_driver *drv = to_opm_driver(_dev->driver);
	struct opm_device *dev = to_opm_device(_dev);

	drv->shutdown(dev);
}

static int opm_drv_suspend(struct device *_dev, pm_message_t state)
{
	struct opm_driver *drv = to_opm_driver(_dev->driver);
	struct opm_device *dev = to_opm_device(_dev);

	if(dev->suspend_order>ORDER_IGNORE)
	{
		struct opm_device *pos;
		struct opm_driver *pdrv;
		list_for_each_entry(pos,&opm_suspend_nodes,s_node)
		{
			if(pos->suspend_order>dev->suspend_order)
				break;
			
			if((pos->pm_action!=ORDACT_SUSPEND)&&(pos!=dev))
			{
				pdrv=to_opm_driver(pos->dev.driver);
				pos->pm_action=ORDACT_SUSPEND;
				pdrv->suspend(pos, state);
			}
		}
	}
	
	if(dev->pm_action!=ORDACT_SUSPEND)
	{
		dev->pm_action=ORDACT_SUSPEND;
		return drv->suspend(dev, state);
	}
	else return 0;
}

static int opm_drv_resume(struct device *_dev)
{
	struct opm_driver *drv = to_opm_driver(_dev->driver);
	struct opm_device *dev = to_opm_device(_dev);
	
	if(dev->resume_order>ORDER_IGNORE)
	{
		struct opm_device *pos;
		struct opm_driver *pdrv;
		list_for_each_entry(pos,&opm_resume_nodes,r_node)
		{
			if(pos->resume_order>dev->resume_order)
				break;
			
			if((pos->pm_action!=ORDACT_RESUME)&&(pos!=dev))
			{
				pdrv=to_opm_driver(pos->dev.driver);
				pos->pm_action=ORDACT_RESUME;
				pdrv->resume(pos);
			}
			
		}
	}
	if(dev->pm_action!=ORDACT_RESUME)
	{
		dev->pm_action=ORDACT_RESUME;
		return drv->resume(dev);
	}
	else return 0;
}

/**
 * opm_driver_register
 * @drv: opm driver structure
 */
int opm_driver_register(struct opm_driver *drv)
{
	drv->driver.bus = &opm_bus_type;
	if (drv->probe)
		drv->driver.probe = opm_drv_probe;
	if (drv->remove)
		drv->driver.remove = opm_drv_remove;
	if (drv->shutdown)
		drv->driver.shutdown = opm_drv_shutdown;
	if (drv->suspend)
		drv->driver.suspend = opm_drv_suspend;
	if (drv->resume)
		drv->driver.resume = opm_drv_resume;

	//special rule of sysclass
	if (strcmp(drv->driver.name,"opm_sysclass") == 0)
		sysclass_drv=drv;
		
	return driver_register(&drv->driver);
}
EXPORT_SYMBOL(opm_driver_register);

/**
 * opm_driver_unregister
 * @drv: opm driver structure
 */
void opm_driver_unregister(struct opm_driver *drv)
{
	driver_unregister(&drv->driver);
}
EXPORT_SYMBOL(opm_driver_unregister);


/* modalias support enables more hands-off userspace setup:
 * (a) environment variable lets new-style hotplug events work once system is
 *     fully running:  "modprobe $MODALIAS"
 * (b) sysfs attribute lets new-style coldplug recover from hotplug events
 *     mishandled before system is fully running:  "modprobe $(cat modalias)"
 */
static ssize_t modalias_show(struct device *dev, struct device_attribute *a,
			     char *buf)
{
	struct opm_device	*pdev = to_opm_device(dev);
	int len = snprintf(buf, PAGE_SIZE, "opm:%s\n", pdev->name);

	return (len >= PAGE_SIZE) ? (PAGE_SIZE - 1) : len;
}

static struct device_attribute opm_dev_attrs[] = {
	__ATTR_RO(modalias),
	__ATTR_NULL,
};

static int opm_uevent(struct device *dev, struct kobj_uevent_env *env)
{
	struct opm_device	*pdev = to_opm_device(dev);

	add_uevent_var(env, "MODALIAS=opm:%s", pdev->name);
	return 0;
}

/**
 * opm_match - bind opm device to opm driver.
 * @dev: device.
 * @drv: driver.
 *
 * opm device IDs are assumed to be encoded like this:
 * "<name><instance>", where <name> is a short description of the type of
 * device, like "pci" or "floppy", and <instance> is the enumerated
 * instance of the device, like '0' or '42'.  Driver IDs are simply
 * "<name>".  So, extract the <name> from the opm_device structure,
 * and compare it against the name of the driver. Return whether they match
 * or not.
 */
static int opm_match(struct device *dev, struct device_driver *drv)
{
	struct opm_device *pdev;

	pdev = container_of(dev, struct opm_device, dev);
	return (strcmp(pdev->name, drv->name) == 0);
}

#ifdef CONFIG_PM_SLEEP

static int opm_legacy_suspend(struct device *dev, pm_message_t mesg)
{
	int ret = 0;

	if (dev->driver && dev->driver->suspend)
		ret = dev->driver->suspend(dev, mesg);

	return ret;
}

static int opm_legacy_suspend_late(struct device *dev, pm_message_t mesg)
{
	struct opm_driver *drv = to_opm_driver(dev->driver);
	struct opm_device *pdev;
	int ret = 0;

	pdev = container_of(dev, struct opm_device, dev);
	if (dev->driver && drv->suspend_late)
		ret = drv->suspend_late(pdev, mesg);

	return ret;
}

static int opm_legacy_resume_early(struct device *dev)
{
	struct opm_driver *drv = to_opm_driver(dev->driver);
	struct opm_device *pdev;
	int ret = 0;

	pdev = container_of(dev, struct opm_device, dev);
	if (dev->driver && drv->resume_early)
		ret = drv->resume_early(pdev);

	return ret;
}

static int opm_legacy_resume(struct device *dev)
{
	int ret = 0;

	if (dev->driver && dev->driver->resume)
		ret = dev->driver->resume(dev);

	return ret;
}

static int opm_prepare(struct device *dev)
{
	struct device_driver *drv = dev->driver;
	int ret = 0;

	if (drv && drv->pm && drv->pm->prepare)
		ret = drv->pm->prepare(dev);

	return ret;
}

static void opm_complete(struct device *dev)
{
	struct device_driver *drv = dev->driver;

	if (drv && drv->pm && drv->pm->complete)
		drv->pm->complete(dev);
}

#ifdef CONFIG_SUSPEND

static int opm_suspend(struct device *dev)
{
	struct device_driver *drv = dev->driver;
	int ret = 0;

	if (drv && drv->pm) {
		if (drv->pm->suspend)
			ret = drv->pm->suspend(dev);
	} else {
		ret = opm_legacy_suspend(dev, PMSG_SUSPEND);
	}

	return ret;
}

static int opm_suspend_noirq(struct device *dev)
{
	struct opm_driver *pdrv;
	int ret = 0;

	if (!dev->driver)
		return 0;

	pdrv = to_opm_driver(dev->driver);
	if (pdrv->pm) {
		if (pdrv->pm->suspend_noirq)
			ret = pdrv->pm->suspend_noirq(dev);
	} else {
		ret = opm_legacy_suspend_late(dev, PMSG_SUSPEND);
	}

	return ret;
}

static int opm_resume(struct device *dev)
{
	struct device_driver *drv = dev->driver;
	int ret = 0;

	if (drv && drv->pm) {
		if (drv->pm->resume)
			ret = drv->pm->resume(dev);
	} else {
		ret = opm_legacy_resume(dev);
	}

	return ret;
}

static int opm_resume_noirq(struct device *dev)
{
	struct opm_driver *pdrv;
	int ret = 0;

	if (!dev->driver)
		return 0;

	pdrv = to_opm_driver(dev->driver);
	if (pdrv->pm) {
		if (pdrv->pm->resume_noirq)
			ret = pdrv->pm->resume_noirq(dev);
	} else {
		ret = opm_legacy_resume_early(dev);
	}

	return ret;
}

#else /* !CONFIG_SUSPEND */

#define opm_suspend		NULL
#define opm_resume		NULL
#define opm_suspend_noirq	NULL
#define opm_resume_noirq	NULL

#endif /* !CONFIG_SUSPEND */

#ifdef CONFIG_HIBERNATION

static int opm_freeze(struct device *dev)
{
	struct device_driver *drv = dev->driver;
	int ret = 0;

	if (!drv)
		return 0;

	if (drv->pm) {
		if (drv->pm->freeze)
			ret = drv->pm->freeze(dev);
	} else {
		ret = opm_legacy_suspend(dev, PMSG_FREEZE);
	}

	return ret;
}

static int opm_freeze_noirq(struct device *dev)
{
	struct opm_driver *pdrv;
	int ret = 0;

	if (!dev->driver)
		return 0;

	pdrv = to_opm_driver(dev->driver);
	if (pdrv->pm) {
		if (pdrv->pm->freeze_noirq)
			ret = pdrv->pm->freeze_noirq(dev);
	} else {
		ret = opm_legacy_suspend_late(dev, PMSG_FREEZE);
	}

	return ret;
}

static int opm_thaw(struct device *dev)
{
	struct device_driver *drv = dev->driver;
	int ret = 0;

	if (drv && drv->pm) {
		if (drv->pm->thaw)
			ret = drv->pm->thaw(dev);
	} else {
		ret = opm_legacy_resume(dev);
	}

	return ret;
}

static int opm_thaw_noirq(struct device *dev)
{
	struct opm_driver *pdrv;
	int ret = 0;

	if (!dev->driver)
		return 0;

	pdrv = to_opm_driver(dev->driver);
	if (pdrv->pm) {
		if (pdrv->pm->thaw_noirq)
			ret = pdrv->pm->thaw_noirq(dev);
	} else {
		ret = opm_legacy_resume_early(dev);
	}

	return ret;
}

static int opm_poweroff(struct device *dev)
{
	struct device_driver *drv = dev->driver;
	int ret = 0;

	if (drv && drv->pm) {
		if (drv->pm->poweroff)
			ret = drv->pm->poweroff(dev);
	} else {
		ret = opm_legacy_suspend(dev, PMSG_HIBERNATE);
	}

	return ret;
}

static int opm_poweroff_noirq(struct device *dev)
{
	struct opm_driver *pdrv;
	int ret = 0;

	if (!dev->driver)
		return 0;

	pdrv = to_opm_driver(dev->driver);
	if (pdrv->pm) {
		if (pdrv->pm->poweroff_noirq)
			ret = pdrv->pm->poweroff_noirq(dev);
	} else {
		ret = opm_legacy_suspend_late(dev, PMSG_HIBERNATE);
	}

	return ret;
}

static int opm_restore(struct device *dev)
{
	struct device_driver *drv = dev->driver;
	int ret = 0;

	if (drv && drv->pm) {
		if (drv->pm->restore)
			ret = drv->pm->restore(dev);
	} else {
		ret = opm_legacy_resume(dev);
	}

	return ret;
}

static int opm_restore_noirq(struct device *dev)
{
	struct opm_driver *pdrv;
	int ret = 0;

	if (!dev->driver)
		return 0;

	pdrv = to_opm_driver(dev->driver);
	if (pdrv->pm) {
		if (pdrv->pm->restore_noirq)
			ret = pdrv->pm->restore_noirq(dev);
	} else {
		ret = opm_legacy_resume_early(dev);
	}

	return ret;
}

#else /* !CONFIG_HIBERNATION */

#define opm_freeze		NULL
#define opm_thaw		NULL
#define opm_poweroff		NULL
#define opm_restore		NULL
#define opm_freeze_noirq	NULL
#define opm_thaw_noirq		NULL
#define opm_poweroff_noirq	NULL
#define opm_restore_noirq	NULL

#endif /* !CONFIG_HIBERNATION */

struct dev_pm_ops opm_ops = {
	.prepare = opm_prepare,
	.complete = opm_complete,
	.suspend = opm_suspend,
	.resume = opm_resume,
	.freeze = opm_freeze,
	.thaw = opm_thaw,
	.poweroff = opm_poweroff,
	.restore = opm_restore,
	.suspend_noirq = opm_suspend_noirq,
	.resume_noirq = opm_resume_noirq,
	.freeze_noirq = opm_freeze_noirq,
	.thaw_noirq = opm_thaw_noirq,
	.poweroff_noirq = opm_poweroff_noirq,
	.restore_noirq = opm_restore_noirq,
};

#define opm_PM_OPS_PTR	&opm_ops

#else /* !CONFIG_PM_SLEEP */

#define opm_PM_OPS_PTR	NULL

#endif /* !CONFIG_PM_SLEEP */

struct bus_type opm_bus_type = {
	.name		= "opm",
	.dev_attrs	= opm_dev_attrs,
	.match		= opm_match,
	.uevent		= opm_uevent,
	.pm		= opm_PM_OPS_PTR,
};
EXPORT_SYMBOL(opm_bus_type);



int __init opm_bus_init(void)
{
	int error;

#if 0
    	sysdev_class_register(&opm_sysclass);
	sysdev_register(&opm_sysdev);
#else
	register_syscore_ops(&opm_syscore_ops);
#endif

	error = device_register(&opm_bus);
	if (error)
		return error;
	error =  bus_register(&opm_bus_type);
	if (error)
		device_unregister(&opm_bus);
	return error;
}

subsys_initcall(opm_bus_init);
