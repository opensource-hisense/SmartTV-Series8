#ifndef MTK_PHY_H

#define MTK_PHY_H



#include "mu3d_hal_comm.h"



#undef EXTERN





#define ENTER_U0_TH		 		10

#define MAX_PHASE_RANGE 		31

#define MAX_TIMEOUT_COUNT 		100

#define DATA_DRIVING_MASK 		0x06

#define MAX_DRIVING_RANGE 		0x04

#define MAX_LATCH_SELECT 		0x02





#ifdef _MTK_PHY_EXT_

#define EXTERN

#else 

#define EXTERN extern

#endif





#define U3_PHY_I2C_PCLK_DRV_REG	    0x0A

#define U3_PHY_I2C_PCLK_PHASE_REG	0x0B





EXTERN DEV_INT32 mu3d_hal_phy_scan(DEV_INT32 latch_val);





#undef EXTERN



								

#endif 

