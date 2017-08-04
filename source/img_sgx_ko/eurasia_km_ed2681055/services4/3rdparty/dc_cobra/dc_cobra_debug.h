/*!****************************************************************************
@File			dc_cobra_debug.h

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

$Log: dc_cobra_debug.h $
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

#ifndef __DC_COBRA_DEBUG_H__
#define __DC_COBRA_DEBUG_H__


#define IMG_PRINTF(args)	_img_osk_dbgmsg args;
#define IMG_PRINT_ERROR(args) do{\
	IMG_PRINTF(("IMG: ERR: %s\n", __FILE__));\
	IMG_PRINTF(("          %s()%4d\n           ", __FUNCTION__, __LINE__));\
	IMG_PRINTF(args);\
	IMG_PRINTF(("\n"));\
	} while(0)
#define IMG_PRINT(args) do{\
	IMG_PRINTF(("IMG: "));\
	IMG_PRINTF(args);\
	} while(0)

#ifdef DEBUG
extern int img_debug_level;

#define IMG_DEBUG_PRINT(level, args) do { \
	if((level) <= img_debug_level)\
		{IMG_PRINTF(("IMG<" #level ">: ")); IMG_PRINTF(args); }\
	} while(0)
#define IMG_DEBUG_PRINT_ERROR(args)	IMG_PRINT_ERROR(args)
#define IMG_DEBUG_PRINT_IF(level,cond,args) \
	if((cond) && ((level) <= img_debug_level))\
		{IMG_PRINTF(("IMG<" #level ">: ")); IMG_PRINTF(args); }
#define IMG_DEBUG_PRINT_ELSE(level, args)\
	else if((level) <= img_debug_level)\
		{IMG_PRINTF(("IMG<" #level ">: ")); IMG_PRINTF(args); }

#else /*DeBUG*/

#define IMG_DEBUG_PRINT(level, args)	do{}while(0)
#define IMG_DEBUG_PRINT_ERROR(args)		do{}while(0)
#define IMG_DEBUG_PRINT_IF(level, cond, args) do{}while(0)
#define IMG_DEBUG_PRINT_ELSE(level, args)	  do{}while(0)

#endif /*DEBUG*/

#endif /* __DC_COBRA_DEBUG_H__ */

/******************************************************************************
 End of file (dc_cobra_debug.h)
******************************************************************************/

