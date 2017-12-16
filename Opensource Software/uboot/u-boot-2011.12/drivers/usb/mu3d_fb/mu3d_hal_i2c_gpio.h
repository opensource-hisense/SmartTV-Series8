#ifndef MU3D_HAL_I2C_GPIO_H

#define MU3D_HAL_I2C_GPIO_H



#define     DRV_WriteReg16(addr,data)       ((*(volatile unsigned short *)(addr)) = (unsigned short)(data))

#define     DRV_Reg16(addr)                 (*(volatile unsigned short *)(addr))

//
#define     DRV_WriteReg32(addr,data)       ((*(volatile unsigned long *)(addr)) = (unsigned long)(data))

//
#define     DRV_Reg32(addr)                 (*(volatile unsigned long *)(addr))

//
#define     DRV_WriteReg8(addr,data)        ((*(volatile unsigned char *)(addr)) = (unsigned char)(data))

#define     DRV_Reg8(addr)                  (*(volatile unsigned char *)(addr))



int I2cWriteReg(unsigned char dev_id, unsigned char Addr, unsigned char Data);

int I2cReadReg(unsigned char dev_id, unsigned char Addr, unsigned char *Data);





//functions

void _U3_Write_Bank(int bankValue);

int _U3Write_Reg(int address, int value);

int _U3Read_Reg(int address);



#endif /* MU3D_HAL_I2C_GPIO_H */

