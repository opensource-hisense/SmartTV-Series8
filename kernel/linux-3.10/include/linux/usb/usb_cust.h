/*
 * usb driver head file
 *
 * Copyright (C) 1999 Linus Torvalds
 * Copyright (C) 1999 Johannes Erdfelt
 * Copyright (C) 1999 Gregory P. Smith
 * Copyright (C) 2001 Brad Hards (bhards@bigpond.net.au)
 * Copyright (C) 2012 Intel Corp (tianyu.lan@intel.com)
 *
 *  move struct usb_hub to this file.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 */

/*
For WIFI fast-connection customization
*/

#ifndef USB_FC_SUPPORT_V2
#if defined(CONFIG_ARCH_MT5891) || defined(CONFIG_ARCH_MT5882)	// MT5891/MT5885 PortD support fastconnect Ver2
#define USB_FC_SUPPORT_V2
#endif
#endif


#define MTK_WiFiDriverName "rt2870"
#define WifiDriverNameLen 6         // the length of rt2870


#ifndef USB_FC_SUPPORT_V2
#define MTK_WiFiPath    "5-1:1.2"
#define MTK_WiFiPathLen 7           // the length of WiFiPath
#endif




