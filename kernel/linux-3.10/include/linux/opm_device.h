/*
 * include/linux/opm_device.h
 *
 * multi-stage power management devices
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

#ifndef _OPM_DEVICE_H_
#define _OPM_DEVICE_H_

#include <linux/device.h>

enum {
	ORDER_IGNORE=0,
};

#define ORDER4SUSPEND(x) (1000-(x))

enum {
	ORDACT_NONE=0,
	ORDACT_SUSPEND,
	ORDACT_RESUME	
};

struct opm_device {
	const char	* name;
	int		id;
	struct device	dev;
	struct list_head  s_node;//suspend list
	struct list_head  r_node;//resume list
	int suspend_order;
	int resume_order;
	int pm_action;
};

extern struct list_head orderpm_resume_nodes;
extern struct list_head orderpm_suspend_nodes;

#define to_opm_device(x) container_of((x), struct opm_device, dev)

extern int opm_device_register(struct opm_device *);
extern void opm_device_unregister(struct opm_device *);

extern struct bus_type opm_bus_type;
extern struct device opm_bus;

extern int opm_add_devices(struct opm_device **, int);

extern struct opm_device *opm_device_alloc(const char *name, int id,int suspend_order,int resume_order);
extern int opm_device_add(struct opm_device *pdev);
extern void opm_device_del(struct opm_device *pdev);

extern struct opm_device *opm_device_register_simple(const char *name,
							int id,int resume_order);
struct opm_driver {
	int (*probe)(struct opm_device *);
	int (*remove)(struct opm_device *);
	void (*shutdown)(struct opm_device *);
	int (*suspend)(struct opm_device *, pm_message_t state);
	int (*suspend_late)(struct opm_device *, pm_message_t state);
	int (*resume_early)(struct opm_device *);
	int (*resume)(struct opm_device *);
	struct dev_pm_ops *pm;
	struct device_driver driver;
};

extern int opm_driver_register(struct opm_driver *);
extern void opm_driver_unregister(struct opm_driver *);

/* non-hotpluggable opm devices may use this so that probe() and
 * its support may live in __init sections, conserving runtime memory.
 */
extern int opm_driver_probe(struct opm_driver *driver,
		int (*probe)(struct opm_device *));

#define opm_get_drvdata(_dev)	dev_get_drvdata(&(_dev)->dev)
#define opm_set_drvdata(_dev,data)	dev_set_drvdata(&(_dev)->dev, (data))

#endif /* _OPM_DEVICE_H_ */
