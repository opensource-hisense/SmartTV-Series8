/*
 *  HID driver for some sony "special" devices
 *
 *  Copyright (c) 1999 Andreas Gal
 *  Copyright (c) 2000-2005 Vojtech Pavlik <vojtech@suse.cz>
 *  Copyright (c) 2005 Michael Haboustak <mike-@cinci.rr.com> for Concept2, Inc
 *  Copyright (c) 2008 Jiri Slaby
 *  Copyright (c) 2006-2008 Jiri Kosina
 */

/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#include <linux/device.h>
#include <linux/hid.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/usb.h>

#include "hid-ids.h"

#define VAIO_RDESC_CONSTANT     (1 << 0)
#define SIXAXIS_CONTROLLER_USB  (1 << 1)
#define SIXAXIS_CONTROLLER_BT   (1 << 2)
#define DUALSHOCK4_CONTROLLER_USB  (1 << 5)
#define DUALSHOCK4_CONTROLLER_BT   (1 << 6)

static const u8 sixaxis_rdesc_fixup[] = {
	0x95, 0x13, 0x09, 0x01, 0x81, 0x02, 0x95, 0x0C,
	0x81, 0x01, 0x75, 0x10, 0x95, 0x04, 0x26, 0xFF,
	0x03, 0x46, 0xFF, 0x03, 0x09, 0x01, 0x81, 0x02
};

static const u8 sixaxis_rdesc_fixup2[] = {
	0x05, 0x01, 0x09, 0x04, 0xa1, 0x01, 0xa1, 0x02,
	0x85, 0x01, 0x75, 0x08, 0x95, 0x01, 0x15, 0x00,
	0x26, 0xff, 0x00, 0x81, 0x03, 0x75, 0x01, 0x95,
	0x13, 0x15, 0x00, 0x25, 0x01, 0x35, 0x00, 0x45,
	0x01, 0x05, 0x09, 0x19, 0x01, 0x29, 0x13, 0x81,
	0x02, 0x75, 0x01, 0x95, 0x0d, 0x06, 0x00, 0xff,
	0x81, 0x03, 0x15, 0x00, 0x26, 0xff, 0x00, 0x05,
	0x01, 0x09, 0x01, 0xa1, 0x00, 0x75, 0x08, 0x95,
	0x04, 0x35, 0x00, 0x46, 0xff, 0x00, 0x09, 0x30,
	0x09, 0x31, 0x09, 0x32, 0x09, 0x35, 0x81, 0x02,
	0xc0, 0x05, 0x01, 0x95, 0x13, 0x09, 0x01, 0x81,
	0x02, 0x95, 0x0c, 0x81, 0x01, 0x75, 0x10, 0x95,
	0x04, 0x26, 0xff, 0x03, 0x46, 0xff, 0x03, 0x09,
	0x01, 0x81, 0x02, 0xc0, 0xa1, 0x02, 0x85, 0x02,
	0x75, 0x08, 0x95, 0x30, 0x09, 0x01, 0xb1, 0x02,
	0xc0, 0xa1, 0x02, 0x85, 0xee, 0x75, 0x08, 0x95,
	0x30, 0x09, 0x01, 0xb1, 0x02, 0xc0, 0xa1, 0x02,
	0x85, 0xef, 0x75, 0x08, 0x95, 0x30, 0x09, 0x01,
	0xb1, 0x02, 0xc0, 0xc0,
};

static const u8 dualshock4_digitkeyTbl[8] = {
	0x01,	// up
	0x03, 	// right & up
	0x02, 	// right
	0x06, 	// right & down
	0x04, 	// down
	0x0C, 	// left & down
	0x08, 	// left
	0x09	// left & up
};
struct sony_sc {
	unsigned long quirks;
	struct hid_device *hdev;
	struct work_struct state_worker;
	unsigned long worker_initialized;
	unsigned long is_pressed_ps;
};

static int sony_input_mapping(struct hid_device *hdev, struct hid_input *hi,
		struct hid_field *field, struct hid_usage *usage,
		unsigned long **bit, int *max)
{
	struct sony_sc *sc = hid_get_drvdata(hdev);

	if (sc->quirks & SIXAXIS_CONTROLLER_USB || sc->quirks & SIXAXIS_CONTROLLER_BT) {

		if((usage->hid & HID_USAGE_PAGE) == HID_UP_GENDESK){

			switch (usage->hid) {
			case HID_GD_X: case HID_GD_Y: case HID_GD_Z:
			case HID_GD_RX: case HID_GD_RY: case HID_GD_RZ:
				return 0;
			}

			hid_map_usage(hi, usage, bit, max, EV_ABS, ABS_MISC);
			set_bit(usage->type, hi->input->evbit);

			while (usage->code <= *max && test_and_set_bit(usage->code, *bit))
				usage->code = find_next_zero_bit(*bit, *max + 1, usage->code);

			int a = field->logical_minimum;
			int b = field->logical_maximum;

			if (field->application == HID_GD_GAMEPAD || field->application == HID_GD_JOYSTICK){
				if (usage->code == 0x3b || usage->code == 0x3c || usage->code == 0x3d || usage->code == 0x3e){
					input_set_abs_params(hi->input, usage->code, a, b, 0, 0);
				} else {
					input_set_abs_params(hi->input, usage->code, a, b, (b - a) >> 8, (b - a) >> 4);
				}
			} else {
				input_set_abs_params(hi->input, usage->code, a, b, 0, 0);
			}
			return -1;
		}
	}
	return 0;
}

/* Sony Vaio VGX has wrongly mouse pointer declared as constant */
static __u8 *sony_report_fixup(struct hid_device *hdev, __u8 *rdesc,
		unsigned int *rsize)
{
	struct sony_sc *sc = hid_get_drvdata(hdev);

	/*
	 * Some Sony RF receivers wrongly declare the mouse pointer as a
	 * a constant non-data variable.
	 */
	if ((sc->quirks & VAIO_RDESC_CONSTANT) && *rsize >= 56 &&
	    /* usage page: generic desktop controls */
	    /* rdesc[0] == 0x05 && rdesc[1] == 0x01 && */
	    /* usage: mouse */
	    rdesc[2] == 0x09 && rdesc[3] == 0x02 &&
	    /* input (usage page for x,y axes): constant, variable, relative */
	    rdesc[54] == 0x81 && rdesc[55] == 0x07) {
		hid_info(hdev, "Fixing up Sony RF Receiver report descriptor\n");
		/* input: data, variable, relative */
		rdesc[55] = 0x06;
	}

	/* The HID descriptor exposed over BT has a trailing zero byte */
	if ((((sc->quirks & SIXAXIS_CONTROLLER_USB) && *rsize == 148) ||
			((sc->quirks & SIXAXIS_CONTROLLER_BT) && *rsize == 149)) &&
			rdesc[83] == 0x75) {
		hid_info(hdev, "Fixing up Sony Sixaxis report descriptor\n");
		memcpy((void *)&rdesc[83], (void *)&sixaxis_rdesc_fixup,
			sizeof(sixaxis_rdesc_fixup));
	} else if (sc->quirks & SIXAXIS_CONTROLLER_USB &&
		   *rsize > sizeof(sixaxis_rdesc_fixup2)) {
		hid_info(hdev, "Sony Sixaxis clone detected. Using original report descriptor (size: %d clone; %d new)\n",
			 *rsize, (int)sizeof(sixaxis_rdesc_fixup2));
		*rsize = sizeof(sixaxis_rdesc_fixup2);
		memcpy(rdesc, &sixaxis_rdesc_fixup2, *rsize);
	}
	return rdesc;
}

static int sony_raw_event(struct hid_device *hdev, struct hid_report *report,
		__u8 *rd, int size)
{
	struct sony_sc *sc = hid_get_drvdata(hdev);

	/* Sixaxis HID report has acclerometers/gyro with MSByte first, this
	 * has to be BYTE_SWAPPED before passing up to joystick interface
	 */
	if ((sc->quirks & (SIXAXIS_CONTROLLER_USB | SIXAXIS_CONTROLLER_BT)) &&
			rd[0] == 0x01 && size == 49) {
		swap(rd[41], rd[42]);
		swap(rd[43], rd[44]);
		swap(rd[45], rd[46]);
		swap(rd[47], rd[48]);
	}

	if (sc->quirks & DUALSHOCK4_CONTROLLER_USB) {
		if ((rd[7] & 0x01) == 0x01) {
			if (sc->is_pressed_ps == 0) {
				schedule_work(&sc->state_worker);
				sc->is_pressed_ps = 1;
				hid_info( hdev, "Preessed PS\n" );
			}
		} else {
			if (sc->is_pressed_ps == 1) {
				sc->is_pressed_ps = 0;
				hid_info( hdev, "Releaseed PS\n" );
			}
		}
		return -1;
	}
	else if ((sc->quirks & DUALSHOCK4_CONTROLLER_BT) && (rd[0] == 0x11)){
		char tmp = rd[7] & 0x0F;
		rd[7] &= 0xF0;		//clear D-Pad
		if((0 <= tmp) && (tmp < 8)) {
			rd[7] |= dualshock4_digitkeyTbl[tmp];
		}
    }
	return 0;
}

/*
 * The Sony Sixaxis does not handle HID Output Reports on the Interrupt EP
 * like it should according to usbhid/hid-core.c::usbhid_output_raw_report()
 * so we need to override that forcing HID Output Reports on the Control EP.
 *
 * There is also another issue about HID Output Reports via USB, the Sixaxis
 * does not want the report_id as part of the data packet, so we have to
 * discard buf[0] when sending the actual control message, even for numbered
 * reports, humpf!
 */
static int sixaxis_usb_output_raw_report(struct hid_device *hid, __u8 *buf,
		size_t count, unsigned char report_type)
{
	struct usb_interface *intf = to_usb_interface(hid->dev.parent);
	struct usb_device *dev = interface_to_usbdev(intf);
	struct usb_host_interface *interface = intf->cur_altsetting;
#ifdef CONFIG_HID_SONY_CTRL
	int skipped_report_id = 0;
#endif /* CONFIG_HID_SONY_CTRL */
	int report_id = buf[0];
	int ret;

	if (report_type == HID_OUTPUT_REPORT) {
		/* Don't send the Report ID */
		buf++;
		count--;
#ifdef CONFIG_HID_SONY_CTRL
		skipped_report_id = 1;
#endif /* CONFIG_HID_SONY_CTRL */
	}
#ifdef CONFIG_HID_SONY_CTRL
	if (report_type == HID_FEATURE_REPORT_SKIP_REPORTID ||
		report_type == HID_OUTPUT_REPORT_SKIP_REPORTID) {

		buf++;
		count--;
		skipped_report_id = 1;

		switch (report_type) {
		case HID_FEATURE_REPORT_SKIP_REPORTID:
			report_type = HID_FEATURE_REPORT;
			break;
		case HID_OUTPUT_REPORT_SKIP_REPORTID:
			report_type = HID_OUTPUT_REPORT;
			break;
		default:
			break;
		}
	}
#endif /* CONFIG_HID_SONY_CTRL */

	ret = usb_control_msg(dev, usb_sndctrlpipe(dev, 0),
		HID_REQ_SET_REPORT,
		USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE,
		((report_type + 1) << 8) | report_id,
		interface->desc.bInterfaceNumber, buf, count,
		USB_CTRL_SET_TIMEOUT);

	/* Count also the Report ID, in case of an Output report. */
#ifdef CONFIG_HID_SONY_CTRL
	if (ret > 0 && skipped_report_id)
#else
	if (ret > 0 && report_type == HID_OUTPUT_REPORT)
#endif /* CONFIG_HID_SONY_CTRL */
		ret++;

	return ret;
}

static char tbl[16] = {'0', '1', '2', '3', '4', '5', '6', '7',
	'8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
static void to_hex(char c, char *str)
{
	str[0] = tbl[(unsigned char)c >> 4];
	str[1] = tbl[(unsigned char)c & 0x0f];
	str[2] = 0;
}

/*
 * Sending HID_REQ_GET_REPORT changes the operation mode of the ps3 controller
 * to "operational".  Without this, the ps3 controller will not report any
 * events.
 */
static int sixaxis_set_operational_usb(struct hid_device *hdev)
{
	struct usb_interface *intf = to_usb_interface(hdev->dev.parent);
	struct usb_device *dev = interface_to_usbdev(intf);
	__u16 ifnum = intf->cur_altsetting->desc.bInterfaceNumber;
	int ret;
	char *buf = kmalloc(18, GFP_KERNEL);

	if (!buf)
		return -ENOMEM;

	ret = usb_control_msg(dev, usb_rcvctrlpipe(dev, 0),
				 HID_REQ_GET_REPORT,
				 USB_DIR_IN | USB_TYPE_CLASS |
				 USB_RECIP_INTERFACE,
				 (3 << 8) | 0xf2, ifnum, buf, 17,
				 USB_CTRL_GET_TIMEOUT);
	if (ret < 0)
		hid_err(hdev, "can't set operational mode\n");

	to_hex(buf[4], &hdev->uniq[0]);
	to_hex(buf[5], &hdev->uniq[2]);
	to_hex(buf[6], &hdev->uniq[4]);
	to_hex(buf[7], &hdev->uniq[6]);
	to_hex(buf[8], &hdev->uniq[8]);
	to_hex(buf[9], &hdev->uniq[10]);

	kfree(buf);

	return ret;
}

static int sixaxis_set_operational_bt(struct hid_device *hdev)
{
	unsigned char buf[] = { 0xf4,  0x42, 0x03, 0x00, 0x00 };
	return hdev->hid_output_raw_report(hdev, buf, sizeof(buf), HID_FEATURE_REPORT);
}

#ifdef CONFIG_HID_SONY_CTRL
static int dualshock4_set_operational_usb(struct hid_device *hdev)
{
	struct usb_interface *intf = to_usb_interface(hdev->dev.parent);
	struct usb_device *dev = interface_to_usbdev(intf);
	__u16 ifnum = intf->cur_altsetting->desc.bInterfaceNumber;
	int ret;
	char *buf = kmalloc(18, GFP_KERNEL);

	if (!buf)
		return -ENOMEM;

	ret = usb_control_msg(dev, usb_rcvctrlpipe(dev, 0),
				 HID_REQ_GET_REPORT,
				 USB_DIR_IN | USB_TYPE_CLASS |
				 USB_RECIP_INTERFACE,
				 (3 << 8) | 0x12, ifnum, buf, 16,
				 USB_CTRL_GET_TIMEOUT);
	if (ret < 0)
		hid_err(hdev, "can't set operational mode\n");

	to_hex(buf[6], &hdev->uniq[0]);
	to_hex(buf[5], &hdev->uniq[2]);
	to_hex(buf[4], &hdev->uniq[4]);
	to_hex(buf[3], &hdev->uniq[6]);
	to_hex(buf[2], &hdev->uniq[8]);
	to_hex(buf[1], &hdev->uniq[10]);

	kfree(buf);

	return ret;
}
#endif // CONFIG_HID_SONY_CTRL

static void dualshock4_state_worker(struct work_struct *work)
{
	struct sony_sc *sc = container_of(work, struct sony_sc, state_worker);
	unsigned char report[] = {
		0x14,
		0x01, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00
	};
	sc->hdev->hid_output_raw_report( sc->hdev, report, sizeof(report), HID_FEATURE_REPORT );
	hid_info( sc->hdev, "BTLinkConnect\n" );
}
static inline void sony_init_work(struct sony_sc *sc, void (*worker)(struct work_struct *))
{
    if (!sc->worker_initialized) {
        INIT_WORK(&sc->state_worker, worker);
	}
    sc->worker_initialized = 1;
}
static inline void sony_cancel_work_sync(struct sony_sc *sc)
{
    if (sc->worker_initialized) {
        cancel_work_sync(&sc->state_worker);
    }
}
static int sony_probe(struct hid_device *hdev, const struct hid_device_id *id)
{
	int ret;
	unsigned long quirks = id->driver_data;
	struct sony_sc *sc;

	sc = kzalloc(sizeof(*sc), GFP_KERNEL);
	if (sc == NULL) {
		hid_err(hdev, "can't alloc sony descriptor\n");
		return -ENOMEM;
	}

	sc->quirks = quirks;
	sc->hdev = hdev;
	hid_set_drvdata(hdev, sc);

	ret = hid_parse(hdev);
	if (ret) {
		hid_err(hdev, "parse failed\n");
		goto err_free;
	}
	
#ifdef CONFIG_HID_SONY_CTRL
		if (sc->quirks & SIXAXIS_CONTROLLER_USB) {
			hdev->hid_output_raw_report = sixaxis_usb_output_raw_report;
			ret = sixaxis_set_operational_usb(hdev);
		}
		else if (sc->quirks & SIXAXIS_CONTROLLER_BT)
			ret = sixaxis_set_operational_bt(hdev);
	else if (sc->quirks & DUALSHOCK4_CONTROLLER_USB) {
			ret = dualshock4_set_operational_usb(hdev);
		sony_init_work(sc, dualshock4_state_worker);
	} else
			ret = 0;
	
		if(ret < 0)
			goto err_free;
#endif

	ret = hid_hw_start(hdev, HID_CONNECT_DEFAULT |
			HID_CONNECT_HIDDEV_FORCE);
	if (ret) {
		hid_err(hdev, "hw start failed\n");
		goto err_free;
	}
	
#ifndef CONFIG_HID_SONY_CTRL
	if (sc->quirks & SIXAXIS_CONTROLLER_USB) {
		hdev->hid_output_raw_report = sixaxis_usb_output_raw_report;
		ret = sixaxis_set_operational_usb(hdev);
	}
	else if (sc->quirks & SIXAXIS_CONTROLLER_BT)
		ret = sixaxis_set_operational_bt(hdev);
	else
		ret = 0;

	if (ret < 0)
		goto err_stop;
#endif

	return 0;
err_stop:
	hid_hw_stop(hdev);
err_free:
	sony_cancel_work_sync(sc);
	kfree(sc);
	return ret;
}

static void sony_remove(struct hid_device *hdev)
{
	struct sony_sc *sc = hid_get_drvdata(hdev);
	sony_cancel_work_sync(sc);
	hid_hw_stop(hdev);
	kfree(hid_get_drvdata(hdev));
}

static const struct hid_device_id sony_devices[] = {
	{ HID_USB_DEVICE(USB_VENDOR_ID_SONY, USB_DEVICE_ID_SONY_PS3_CONTROLLER),
		.driver_data = SIXAXIS_CONTROLLER_USB },
#ifdef CONFIG_HID_SONY_CTRL
	{ HID_USB_DEVICE(USB_VENDOR_ID_SONY, USB_DEVICE_ID_SONY_PS4_CONTROLLER),
		.driver_data = DUALSHOCK4_CONTROLLER_USB },
#endif /* CONFIG_HID_SONY_CTRL */
	{ HID_USB_DEVICE(USB_VENDOR_ID_SONY, USB_DEVICE_ID_SONY_NAVIGATION_CONTROLLER),
		.driver_data = SIXAXIS_CONTROLLER_USB },
	{ HID_BLUETOOTH_DEVICE(USB_VENDOR_ID_SONY, USB_DEVICE_ID_SONY_PS3_CONTROLLER),
		.driver_data = SIXAXIS_CONTROLLER_BT },
#ifdef CONFIG_HID_SONY_CTRL
	{ HID_BLUETOOTH_DEVICE(USB_VENDOR_ID_SONY, USB_DEVICE_ID_SONY_PS4_CONTROLLER),
		.driver_data = DUALSHOCK4_CONTROLLER_BT },
#endif /* CONFIG_HID_SONY_CTRL */
	{ HID_USB_DEVICE(USB_VENDOR_ID_SONY, USB_DEVICE_ID_SONY_VAIO_VGX_MOUSE),
		.driver_data = VAIO_RDESC_CONSTANT },
	{ HID_USB_DEVICE(USB_VENDOR_ID_SONY, USB_DEVICE_ID_SONY_VAIO_VGP_MOUSE),
		.driver_data = VAIO_RDESC_CONSTANT },
	{ }
};
MODULE_DEVICE_TABLE(hid, sony_devices);

static struct hid_driver sony_driver = {
	.name = "sony",
	.id_table = sony_devices,
	.input_mapping = sony_input_mapping,
	.probe = sony_probe,
	.remove = sony_remove,
	.report_fixup = sony_report_fixup,
	.raw_event = sony_raw_event
};
module_hid_driver(sony_driver);

MODULE_LICENSE("GPL");
