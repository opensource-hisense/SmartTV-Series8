/*!****************************************************************************
@File			dc_cobra_displayclass.h

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

$Log: dc_cobra_displayclass.h $
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

#ifndef __DC_COBRA_DISPLAYCLASS_H__
#define __DC_COBRA_DISPLAYCLASS_H__


#define DC_MAX_NUM_FB_DEVICES		3	/* FBDev0, FBDev1, FBDev2 */	

typedef enum _DC_ERROR_
{
	DC_OK								=  0,
	DC_ERROR_GENERIC					=  1,
	DC_ERROR_OUT_OF_MEMORY				=  2,
	DC_ERROR_TOO_FEW_BUFFERS			=  3,
	DC_ERROR_INVALID_PARAMS				=  4,
	DC_ERROR_INIT_FAILURE				=  5,
	DC_ERROR_CANT_REGISTER_CALLBACK 	=  6,
	DC_ERROR_INVALID_DEVICE				=  7,
	DC_ERROR_DEVICE_REGISTER_FAILED 	=  8,
	DC_ERROR_DEVICE_UNREGISTER_FAILED	=  9,
	DC_ERROR_SET_UPDATE_MODE_FAILED		= 10,
	DC_ERROR_NO_PRIMARY					= 11
} DC_ERROR;

typedef enum _DC_UPDATE_MODE_
{
	DC_UPDATE_MODE_UNDEFINED			= 0,
	DC_UPDATE_MODE_MANUAL				= 1,
	DC_UPDATE_MODE_AUTO					= 2,
	DC_UPDATE_MODE_DISABLED				= 3
} DC_UPDATE_MODE;

#ifdef __cplusplus
extern "C"
{
#endif

/* prototypes for init/de-init functions */
DC_ERROR DCInit(void);
DC_ERROR DCDeInit(void);

#ifdef __cplusplus
}
#endif

#endif /* __DC_COBRA_DISPLAYCLASS_H__ */

/******************************************************************************
 End of file (dc_cobra_displayclass.h)
******************************************************************************/

