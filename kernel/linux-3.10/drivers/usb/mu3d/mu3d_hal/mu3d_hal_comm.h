
#ifndef _DRV_COMM_H

#define _DRV_COMM_H

#include "mu3d_hal_hw.h"
#ifdef CONFIG_64BIT
#include <linux/soc/mediatek/x_typedef.h>
#else
#include <x_typedef.h>
#endif





#undef EXTERN



#ifdef _DRV_COMM_H

#define EXTERN 

#else 

#define EXTERN extern

#endif





/* CONSTANTS */



#ifndef FALSE

  #define FALSE   0

#endif



#ifndef TRUE

  #define TRUE    1

#endif



/* TYPES */



typedef ulong    DEV_UINT32;

typedef int    			DEV_INT32;

typedef unsigned short  DEV_UINT16;

typedef short    		DEV_INT16;

typedef unsigned char   DEV_UINT8;

typedef char   			DEV_INT8;



typedef enum {

    RET_SUCCESS = 0,

    RET_FAIL,

} USB_RESULT;



/* MACROS */



#define os_writel(addr,data) writel(data, addr) //(*((volatile DEV_UINT32 *)(addr)) = data)

#define os_readl(addr)  readl(addr) //*((volatile DEV_UINT32 *)(addr))

#define os_writelmsk(addr, data, msk) os_writel(addr, ((os_readl(addr) & ~(msk)) | ((data) & (msk))))

#define os_setmsk(addr, msk) os_writel(addr, os_readl(addr) | msk)

#define os_clrmsk(addr, msk) os_writel(addr, os_readl(addr) &~ msk)

/*msk the data first, then umsk with the umsk.*/

#define os_writelmskumsk(addr, data, msk, umsk) os_writel(addr, ((os_readl(addr) & ~(msk)) | ((data) & (msk))) & (umsk))



#define USB_END_OFFSET(_bEnd, _bOffset)	((ulong)(0x10*(_bEnd-1)) + _bOffset)

#define USB_ReadCsr32(_bOffset, _bEnd) os_readl((UPTR*)USB_END_OFFSET(_bEnd, _bOffset))

#define USB_WriteCsr32(  _bOffset, _bEnd, _bData) os_writel( USB_END_OFFSET(_bEnd, _bOffset), _bData)

#define USB_WriteCsr32Msk(  _bOffset, _bEnd, _bData, msk) os_writelmsk( USB_END_OFFSET(_bEnd, _bOffset), _bData, msk)

#define USB_WriteCsr32MskUmsk(  _bOffset, _bEnd, _bData, msk, umsk) os_writelmskumsk( USB_END_OFFSET(_bEnd, _bOffset), _bData, msk, umsk)



/* FUNCTIONS */



EXTERN DEV_INT32 wait_for_value(DEV_UINT32 addr, DEV_INT32 msk, DEV_INT32 value, DEV_INT32 ms_intvl, DEV_INT32 count);





#endif   /*_DRV_COMM_H*/


