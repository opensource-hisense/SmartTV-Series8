/**************************************************************************
 * Name         : dc_cobra_osk.c
 * Author       : Billy Park
 * Created      : 2/6/2012
 *
 * Copyright    : 2016 by Mediatek Limited. All rights reserved.
 *
 *
 * Platform     : cobra
 *
 * $Date: 2015/12/22 $ $Revision: #1 $
 * $Log: dc_cobra_osk.c $
 **************************************************************************/

/**************************************************************************
 The 3rd party driver is a specification of an API to integrate the
 IMG PowerVR Services driver with 3rd Party display hardware.
 It is NOT a specification for a display controller driver, rather a
 specification to extend the API for a pre-existing driver for the display hardware.

 The 3rd party driver interface provides IMG PowerVR client drivers (e.g. PVR2D)
 with an API abstraction of the system's underlying display hardware, allowing
 the client drivers to indirectly control the display hardware and access its
 associated memory.

 Functions of the API include

  - query primary surface attributes (width, height, stride, pixel format,
      CPU physical and virtual address)
  - swap/flip chain creation and subsequent query of surface attributes
  - asynchronous display surface flipping, taking account of asynchronous
      read (flip) and write (render) operations to the display surface

 Note: having queried surface attributes the client drivers are able to map
 the display memory to any IMG PowerVR Services device by calling
 PVRSRVMapDeviceClassMemory with the display surface handle.

 This code is intended to be an example of how a pre-existing display driver
 may be extended to support the 3rd Party Display interface to
 PowerVR Services – IMG is not providing a display driver implementation
 **************************************************************************/

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/module.h>
#include <linux/jiffies.h>
#include <linux/time.h>
#include <asm/delay.h>

#include "dc_cobra_debug.h"
#include "dc_cobra_osk.h"

/*****************************************************************************
 Function Name:	_img_osk_dbgmsg
 Description  :	
*****************************************************************************/
void _img_osk_dbgmsg( const char *fmt, ... )
{
	va_list args;
	va_start(args, fmt);
	vprintk(fmt, args);
	va_end(args);
}

/*****************************************************************************
 Function Name:	_img_osk_get_pid
 Description  :	
*****************************************************************************/
u32 _img_osk_get_pid( void )
{
	return (u32)current->tgid;
}

/*****************************************************************************
 Function Name:	_img_osk_get_tid
 Description  :	
*****************************************************************************/
u32  _img_osk_get_tid( void )
{
	return (u32)current->pid;
}

/*****************************************************************************
 Function Name:	_img_osk_udelay
 Description  :	
*****************************************************************************/
void  _img_osk_udelay( u32 usecs )
{
	udelay(usecs);
}
