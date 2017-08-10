/*
 * linux/arch/arm/mach-mt53xx/include/mach/hardware.h
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

#ifndef __ASM_ARCH_HARDWARE_H
#define __ASM_ARCH_HARDWARE_H

#include <mach/memory.h>
#include <mach/platform.h>

#ifdef CONFIG_PCI_MSI
/* MSI MTK arch hooks */
#define arch_setup_msi_irqs MTK_setup_msi_irqs
#define arch_teardown_msi_irqs MTK_teardown_msi_irqs
#define arch_msi_check_device MTK_msi_check_device
#endif /* #ifdef CONFIG_PCI_MSI */

#define IO_ADDRESS(x)	(x)

#endif

