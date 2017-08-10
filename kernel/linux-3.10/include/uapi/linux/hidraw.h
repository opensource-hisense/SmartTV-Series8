/*
 *  Copyright (c) 2007 Jiri Kosina
 */
/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 */
#ifndef _UAPI_HIDRAW_H
#define _UAPI_HIDRAW_H



#include <linux/hid.h>
#include <linux/types.h>

struct hidraw_report_descriptor {
	__u32 size;
	__u8 value[HID_MAX_DESCRIPTOR_SIZE];
};

struct hidraw_devinfo {
	__u32 bustype;
	__s16 vendor;
	__s16 product;
};

/* ioctl interface */
#define HIDIOCGRDESCSIZE	_IOR('H', 0x01, int)
#define HIDIOCGRDESC		_IOR('H', 0x02, struct hidraw_report_descriptor)
#define HIDIOCGRAWINFO		_IOR('H', 0x03, struct hidraw_devinfo)
#define HIDIOCGRAWNAME(len)     _IOC(_IOC_READ, 'H', 0x04, len)
#define HIDIOCGRAWPHYS(len)     _IOC(_IOC_READ, 'H', 0x05, len)
#ifdef CONFIG_HID_SONY_CTRL
#define HIDIOCGRAWUNIQ(len)     _IOC(_IOC_READ, 'H', 0x0B, len)
#endif /* CONFIG_HID_SONY_CTRL */

/* The first byte of SFEATURE and GFEATURE is the report number */
#define HIDIOCSFEATURE(len)    _IOC(_IOC_WRITE|_IOC_READ, 'H', 0x06, len)
#define HIDIOCGFEATURE(len)    _IOC(_IOC_WRITE|_IOC_READ, 'H', 0x07, len)

#ifdef CONFIG_HID_SONY_CTRL
#define HIDIOCGFEATURE_WITHDATASIZE(len)   _IOC(_IOC_WRITE|_IOC_READ, 'H', 0x08, len)
#define HIDIOCSFEATURE_SKIPREPORTID(len)   _IOC(_IOC_WRITE|_IOC_READ, 'H', 0x09, len)
#define HIDIOCSOUTPUT_SKIPREPORTID(len)    _IOC(_IOC_WRITE|_IOC_READ, 'H', 0x0A, len)
#endif /* CONFIG_HID_SONY_CTRL */

#define HIDRAW_FIRST_MINOR 0
#define HIDRAW_MAX_DEVICES 64
/* number of reports to buffer */
#define HIDRAW_BUFFER_SIZE 64


/* kernel-only API declarations */

#endif /* _UAPI_HIDRAW_H */
