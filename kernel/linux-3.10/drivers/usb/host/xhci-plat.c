/*
 * xHCI host controller driver PCI Bus Glue.
 *
 * Copyright (C) 2008 Intel Corp.
 *
 * Author: Sarah Sharp
 * Some code borrowed from the Linux EHCI driver.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/slab.h>

#include "xhci.h"

static const char hcd_name[] = "MtkUsbHcd.Xhc";

static void xhci_plat_quirks(struct device *dev, struct xhci_hcd *xhci)
{
        /*
         * As of now platform drivers don't provide MSI support so we ensure
         * here that the generic code does not try to make a pci_dev from our
         * dev struct in order to setup MSI
         */
        xhci->quirks |= XHCI_BROKEN_MSI;
		//CC: MTK host controller gives a spurious successful event after a 
		//    short transfer. Ignore it.
        xhci->quirks |= XHCI_SPURIOUS_SUCCESS;
		xhci->quirks |= XHCI_LPM_SUPPORT;
}

/* called during probe() after chip reset completes */
static int xhci_plat_setup(struct usb_hcd *hcd)
{
        return xhci_gen_setup(hcd, xhci_plat_quirks);
}

static const struct hc_driver xhci_plat_xhci_driver = {
        .description =          "xhci-hcd",
        .product_desc =         "xHCI Host Controller",
        .hcd_priv_size =        sizeof(struct xhci_hcd *),

        /*
         * generic hardware linkage
         */
        .irq =                  xhci_irq,
        .flags =                HCD_MEMORY | HCD_USB3 | HCD_SHARED,

        /*
         * basic lifecycle operations
         */
        .reset =                xhci_plat_setup,
        .start =                xhci_run,
        .stop =                 xhci_stop,
        .shutdown =             xhci_shutdown,

        /*
         * managing i/o requests and associated device resources
         */
        .urb_enqueue =          xhci_urb_enqueue,
        .urb_dequeue =          xhci_urb_dequeue,
        .alloc_dev =            xhci_alloc_dev,
        .free_dev =             xhci_free_dev,
        .alloc_streams =        xhci_alloc_streams,
        .free_streams =         xhci_free_streams,
        .add_endpoint =         xhci_add_endpoint,
        .drop_endpoint =        xhci_drop_endpoint,
        .endpoint_reset =       xhci_endpoint_reset,
        .check_bandwidth =      xhci_check_bandwidth,
        .reset_bandwidth =      xhci_reset_bandwidth,
        .address_device =       xhci_address_device,
        .update_hub_device =    xhci_update_hub_device,
        .reset_device =         xhci_discover_or_reset_device,

        /*
         * scheduling support
         */
        .get_frame_number =     xhci_get_frame,

        /* Root hub support */
        .hub_control =          xhci_hub_control,
        .hub_status_data =      xhci_hub_status_data,
        .bus_suspend =          xhci_bus_suspend,
        .bus_resume =           xhci_bus_resume,
#ifdef CONFIG_XHCI_PM
		/*
		 * call back when device connected and addressed
		 */
		.update_device =        xhci_update_device,
		.set_usb2_hw_lpm =	xhci_set_usb2_hardware_lpm,
		.enable_usb3_lpm_timeout =	xhci_enable_usb3_lpm_timeout,
		.disable_usb3_lpm_timeout =	xhci_disable_usb3_lpm_timeout,
#endif
};


/*
struct platform_driver usb_xhci_driver = {
        .probe  = xhci_plat_probe,
        .remove = xhci_plat_remove,
        .driver = {
                .name =		(char *) hcd_name,
				.owner =	THIS_MODULE,
        },
};
*/
MODULE_ALIAS("platform:xhci-hcd");
#if 0
int xhci_register_plat(void)
{
        return platform_driver_register(&usb_xhci_driver);
}

void xhci_unregister_plat(void)
{
        platform_driver_unregister(&usb_xhci_driver);
}
#endif
