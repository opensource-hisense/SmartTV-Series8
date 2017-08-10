/* USB OTG (On The Go) defines */
/*
 *
 * These APIs may be used between USB controllers.  USB device drivers
 * (for either host or peripheral roles) don't use these calls; they
 * continue to use just usb_device and usb_gadget.
 */

#ifndef __LINUX_USB_OTG_H
#define __LINUX_USB_OTG_H

#include <linux/usb/phy.h>

struct usb_otg {
	u8			default_a;

	struct usb_phy		*phy;
	struct usb_bus		*host;
	struct usb_gadget	*gadget;

	/* bind/unbind the host controller */
	int	(*set_host)(struct usb_otg *otg, struct usb_bus *host);

	/* bind/unbind the peripheral controller */
	int	(*set_peripheral)(struct usb_otg *otg,
					struct usb_gadget *gadget);

	/* effective for A-peripheral, ignored for B devices */
	int	(*set_vbus)(struct usb_otg *otg, bool enabled);

	/* for B devices only:  start session with A-Host */
	int	(*start_srp)(struct usb_otg *otg);

	/* start or continue HNP role switch */
	int	(*start_hnp)(struct usb_otg *otg);

};
//temp solutin for ADB, porting the linux kernel from 3.0
struct otg_transceiver;
struct otg_transceiver {
	struct device		*dev;
	const char		*label;
	unsigned int		 flags;

	u8			default_a;
	enum usb_otg_state	state;

	struct usb_bus		*host;
	struct usb_gadget	*gadget;

	struct otg_io_access_ops	*io_ops;
	void __iomem			*io_priv;

	/* for notification of usb_xceiv_events */
	struct atomic_notifier_head	notifier;

	/* to pass extra port status to the root hub */
	u16			port_status;
	u16			port_change;

	/* initialize/shutdown the OTG controller */
	int	(*init)(struct otg_transceiver *otg);
	void	(*shutdown)(struct otg_transceiver *otg);

	/* bind/unbind the host controller */
	int	(*set_host)(struct otg_transceiver *otg,
				struct usb_bus *host);

	/* bind/unbind the peripheral controller */
	int	(*set_peripheral)(struct otg_transceiver *otg,
				struct usb_gadget *gadget);

	/* effective for B devices, ignored for A-peripheral */
	int	(*set_power)(struct otg_transceiver *otg,
				unsigned mA);

	/* effective for A-peripheral, ignored for B devices */
	int	(*set_vbus)(struct otg_transceiver *otg,
				bool enabled);

	/* for non-OTG B devices: set transceiver into suspend mode */
	int	(*set_suspend)(struct otg_transceiver *otg,
				int suspend);

	/* for B devices only:  start session with A-Host */
	int	(*start_srp)(struct otg_transceiver *otg);

	/* start or continue HNP role switch */
	int	(*start_hnp)(struct otg_transceiver *otg);

};
extern const char *usb_otg_state_string(enum usb_otg_state state);

/* Context: can sleep */
static inline int
otg_start_hnp(struct usb_otg *otg)
{
	if (otg && otg->start_hnp)
		return otg->start_hnp(otg);

	return -ENOTSUPP;
}

/* Context: can sleep */
static inline int
otg_set_vbus(struct usb_otg *otg, bool enabled)
{
	if (otg && otg->set_vbus)
		return otg->set_vbus(otg, enabled);

	return -ENOTSUPP;
}

/* for HCDs */
static inline int
otg_set_host(struct usb_otg *otg, struct usb_bus *host)
{
	if (otg && otg->set_host)
		return otg->set_host(otg, host);

	return -ENOTSUPP;
}

/* for usb peripheral controller drivers */

/* Context: can sleep */
static inline int
otg_set_peripheral(struct usb_otg *otg, struct usb_gadget *periph)
{
	if (otg && otg->set_peripheral)
		return otg->set_peripheral(otg, periph);

	return -ENOTSUPP;
}

static inline int
otg_start_srp(struct usb_otg *otg)
{
	if (otg && otg->start_srp)
		return otg->start_srp(otg);

	return -ENOTSUPP;
}

/* for OTG controller drivers (and maybe other stuff) */
extern int usb_bus_start_enum(struct usb_bus *bus, unsigned port_num);

#endif /* __LINUX_USB_OTG_H */
