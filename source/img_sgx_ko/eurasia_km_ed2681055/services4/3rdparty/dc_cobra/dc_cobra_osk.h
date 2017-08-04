/*!****************************************************************************
@File			dc_cobra_osk.h

@Title			Dummy 3rd party display driver structures and prototypes

@Author			Billy Park

@date   			2/6/2012

@Copyright     	Copyright 2016 by MediaTek Limited. All rights reserved.

@Platform		cobra

@Description	Dummy 3rd party display driver structures and prototypes

@DoxygenVer

******************************************************************************/

/******************************************************************************
Modifications :-

$Log: dc_cobra_osk.h $
******************************************************************************/

/**************************************************************************
 The 3rd party driver is a specification of an API to integrate the
 IMG PowerVR Services driver with 3rd Party display hardware.
 It is NOT a specification for a display controller driver, rather a
 specification to extend the API for a pre-existing driver for the display hardware.

 The 3rd party driver interface provides IMG PowerVR client drivers (e.g. PVR2D)
 with an API abstraction of the system’s underlying display hardware, allowing
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

#ifndef __DC_COBRA_OSK_H__
#define __DC_COBRA_OSK_H__

#ifdef __cplusplus
extern "C"
{
#endif

/** @brief Output a device driver debug message.
 *
 * The interpretation of \a fmt is the same as the \c format parameter in printk().
 *
 * @param fmt a format string
 * @param ... a variable-number of parameters suitable for \a fmt
 */
void _img_osk_dbgmsg( const char *fmt, ... );

/** @brief Return an identificator for calling process.
 *
 * @return Identificator for calling process.
 */
u32  _img_osk_get_pid( void );

/** @brief Return an identificator for calling thread.
 *
 * @return Identificator for calling thread.
 */
u32  _img_osk_get_tid( void );

/** @brief Microsecond delay
 *
 * @param usecs a microsecond.
 */
void _img_osk_udelay( u32 usecs );


void _img_osk_lock_init( void );


void _img_osk_lock_signal( void );

void _img_osk_lock_wait( void );

void _img_osk_lock_term( void );

#ifdef __cplusplus
}
#endif

#endif /* __DC_COBRA_OSK_H__ */

/******************************************************************************
 End of file (dc_cobra_osk.h)
******************************************************************************/

