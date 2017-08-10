#include <linux/gfp.h>

#include <linux/kernel.h>

#include <linux/delay.h>

#include "mu3d_hal_phy.h"

//#include "mu3d_hal_i2c_gpio.h"

#include "ssusb_sifslv_ippc_c_header.h"



/////////////////////////////////////////////////////////////////



#define     OUTPUT   1

#define     INPUT    0 



#define SDA    0        /// GPIO #0: I2C data pin

#define SCL    1        /// GPIO #1: I2C clock pin

 

/////////////////////////////////////////////////////////////////



void gpio_dir_set(int pin){

	int addr, temp;

	addr = U3D_SSUSB_FPGA_I2C_OUT_0P;

	temp = DRV_Reg32(addr);

	if(pin == SDA){

		temp |= SSUSB_FPGA_I2C_SDA_OEN_0P;

		DRV_WriteReg32(addr,temp);

	}

	else{

		temp |= SSUSB_FPGA_I2C_SCL_OEN_0P;

		DRV_WriteReg32(addr,temp);

	}

}



void gpio_dir_clr(int pin){

	int addr, temp;

	addr = U3D_SSUSB_FPGA_I2C_OUT_0P;

	temp = DRV_Reg32(addr);

	if(pin == SDA){

		temp &= ~SSUSB_FPGA_I2C_SDA_OEN_0P;

		DRV_WriteReg32(addr,temp);

	}

	else{

		temp &= ~SSUSB_FPGA_I2C_SCL_OEN_0P;

		DRV_WriteReg32(addr,temp);

	}

}



void gpio_dout_set(int pin){

	int addr, temp;

	addr = U3D_SSUSB_FPGA_I2C_OUT_0P;

	temp = DRV_Reg32(addr);

	if(pin == SDA){

		temp |= SSUSB_FPGA_I2C_SDA_OUT_0P;

		DRV_WriteReg32(addr,temp);

	}

	else{

		temp |= SSUSB_FPGA_I2C_SCL_OUT_0P;

		DRV_WriteReg32(addr,temp);

	}

}



void gpio_dout_clr(int pin){

	int addr, temp;

	addr = U3D_SSUSB_FPGA_I2C_OUT_0P;

	temp = DRV_Reg32(addr);

	if(pin == SDA){

		temp &= ~SSUSB_FPGA_I2C_SDA_OUT_0P;

		DRV_WriteReg32(addr,temp);

	}

	else{

		temp &= ~SSUSB_FPGA_I2C_SCL_OUT_0P;

		DRV_WriteReg32(addr,temp);

	}

}



int gpio_din(int pin){

	int addr, temp;

	addr = U3D_SSUSB_FPGA_I2C_IN_0P;

	temp = DRV_Reg32(addr);

	if(pin == SDA){

		temp = (temp >> SSUSB_FPGA_I2C_SDA_IN_0P_OFST) & 1;

	}

	else{

		temp = (temp >> SSUSB_FPGA_I2C_SCL_IN_0P_OFST) & 1;

	}

	return temp;

}



//#define     GPIO_PULLEN_SET(_no)  (GPIO_PULLEN1_SET+(0x10*(_no)))  

#define     GPIO_DIR_SET(pin)   gpio_dir_set(pin)

#define     GPIO_DOUT_SET(pin)  gpio_dout_set(pin);

//#define     GPIO_PULLEN_CLR(_no) (GPIO_PULLEN1_CLR+(0x10*(_no)))  

#define     GPIO_DIR_CLR(pin)   gpio_dir_clr(pin)

#define     GPIO_DOUT_CLR(pin)  gpio_dout_clr(pin)

#define     GPIO_DIN(pin)       gpio_din(pin)





unsigned int  i2c_dummy_cnt;



#define I2C_DELAY 10

#define I2C_DUMMY_DELAY(_delay) for (i2c_dummy_cnt = ((_delay)) ; i2c_dummy_cnt!=0; i2c_dummy_cnt--)



void GPIO_InitIO(unsigned int dir, unsigned int pin)

{  

    if (dir == OUTPUT)

    {   

        GPIO_DIR_SET(pin);

    }

    else

    {   

        GPIO_DIR_CLR(pin);

    }

    I2C_DUMMY_DELAY(100);

}



void GPIO_WriteIO(unsigned int data, unsigned int pin)

{

    if (data == 1){

		GPIO_DOUT_SET(pin);

    }

    else{

        GPIO_DOUT_CLR(pin);

    }

}



unsigned int GPIO_ReadIO( unsigned int pin)

{

    unsigned short data;

    data=GPIO_DIN(pin);

    return (unsigned int)data;

}



void SerialCommStop(void)

{

	GPIO_InitIO(OUTPUT,SDA);



	GPIO_WriteIO(0,SCL);

	I2C_DUMMY_DELAY(I2C_DELAY);

	GPIO_WriteIO(0,SDA);

	I2C_DUMMY_DELAY(I2C_DELAY);

	GPIO_WriteIO(1,SCL);

	I2C_DUMMY_DELAY(I2C_DELAY);

	GPIO_WriteIO(1,SDA);

	I2C_DUMMY_DELAY(I2C_DELAY);



	//stop to drive SDA/SCL

	GPIO_InitIO(INPUT,SDA);	

	GPIO_InitIO(INPUT,SCL);

}



void SerialCommStart(void) /* Prepare the SDA and SCL for sending/receiving */

{

	GPIO_InitIO(OUTPUT,SCL);

    GPIO_InitIO(OUTPUT,SDA);

    GPIO_WriteIO(1,SDA);

    I2C_DUMMY_DELAY(I2C_DELAY);

    GPIO_WriteIO(1,SCL);

    I2C_DUMMY_DELAY(I2C_DELAY);

    GPIO_WriteIO(0,SDA);

    I2C_DUMMY_DELAY(I2C_DELAY);

    GPIO_WriteIO(0,SCL);

    I2C_DUMMY_DELAY(I2C_DELAY);

}



unsigned int SerialCommTxByte(unsigned char data) /* return 0 --> ack */

{

    int i, ack;

    

    GPIO_InitIO(OUTPUT,SDA);



    for(i=8; --i>0;){

        GPIO_WriteIO((data>>i)&0x01, SDA);

        I2C_DUMMY_DELAY(I2C_DELAY);

        GPIO_WriteIO( 1, SCL); /* high */

        I2C_DUMMY_DELAY(I2C_DELAY);

        GPIO_WriteIO( 0, SCL); /* low */

        I2C_DUMMY_DELAY(I2C_DELAY);

    }

    GPIO_WriteIO((data>>i)&0x01, SDA);

    I2C_DUMMY_DELAY(I2C_DELAY);

    GPIO_WriteIO( 1, SCL); /* high */

    I2C_DUMMY_DELAY(I2C_DELAY);

    GPIO_WriteIO( 0, SCL); /* low */

    I2C_DUMMY_DELAY(I2C_DELAY);

    

    GPIO_WriteIO(0, SDA);

    I2C_DUMMY_DELAY(I2C_DELAY);

    GPIO_InitIO(INPUT,SDA);

    I2C_DUMMY_DELAY(I2C_DELAY);

    GPIO_WriteIO(1, SCL);

    I2C_DUMMY_DELAY(I2C_DELAY);

    ack = GPIO_ReadIO(SDA); /// ack 1: error , 0:ok

    GPIO_WriteIO(0, SCL);

    I2C_DUMMY_DELAY(I2C_DELAY);

    

    if(ack==1)

        return 0;

    else

        return 1;  

}



void SerialCommRxByte(unsigned  char *data, unsigned char ack)

{

   int i;

   unsigned int dataCache;

   dataCache = 0;

   GPIO_InitIO(INPUT,SDA);

   for(i=8; --i>=0;){

      dataCache <<= 1;

      I2C_DUMMY_DELAY(I2C_DELAY);

      GPIO_WriteIO(1, SCL);

      I2C_DUMMY_DELAY(I2C_DELAY);

      dataCache |= GPIO_ReadIO(SDA);

      GPIO_WriteIO(0, SCL);

      I2C_DUMMY_DELAY(I2C_DELAY);

   }

   GPIO_InitIO(OUTPUT,SDA);

   GPIO_WriteIO(ack, SDA);

   I2C_DUMMY_DELAY(I2C_DELAY);

   GPIO_WriteIO(1, SCL);

   I2C_DUMMY_DELAY(I2C_DELAY);

   GPIO_WriteIO(0, SCL);

   I2C_DUMMY_DELAY(I2C_DELAY);

   *data = (unsigned char)dataCache;

}





int I2cWriteReg(unsigned char dev_id, unsigned char Addr, unsigned char Data)

{

    int acknowledge=0;



    SerialCommStart();

    acknowledge=SerialCommTxByte((dev_id<<1) & 0xff);

    if(acknowledge)

        acknowledge=SerialCommTxByte(Addr);

    else

        return 0;

    acknowledge=SerialCommTxByte(Data);

    if(acknowledge)

    {

        SerialCommStop();

        return 1;

    }

    else

    {    

        return 0;

    }        

}



int I2cReadReg(unsigned char dev_id, unsigned char Addr, unsigned char *Data)

{

    int acknowledge=0;    

    SerialCommStart();    

    acknowledge=SerialCommTxByte((dev_id<<1) & 0xff);

    if(acknowledge)

        acknowledge=SerialCommTxByte(Addr);

    else

        return 0;    

    SerialCommStart();

    acknowledge=SerialCommTxByte(((dev_id<<1) & 0xff) | 0x01);

    if(acknowledge)

        SerialCommRxByte(Data, 1);  // ack 0: ok , 1 error

    else

        return 0;    

    SerialCommStop();

    return acknowledge;

} 





void _U3_Write_Bank(int bankValue){

	I2cWriteReg(U3_PHY_I2C_DEV, U3_PHY_PAGE, bankValue);	

}



int _U3Write_Reg(int address, int value){

	I2cWriteReg(U3_PHY_I2C_DEV, address, value);

}



int _U3_WRITE_MSK_REG(int address, int mask, int value){	

	_U3Write_Reg(address, (_U3Read_Reg(address) & ~mask) | (value & mask));

	

	return 0;

}



int _U3Read_Reg(int address){

	char *pu1Buf;

	unsigned int ret;

	

	pu1Buf = (char *)os_mem_alloc(1);

	ret = I2cReadReg(U3_PHY_I2C_DEV, address, pu1Buf);

	if(ret == 0){

		printk(KERN_ERR "Read failed\n");

		return 0;

	}

	ret = (char)pu1Buf[0];

	kfree(pu1Buf);

	return ret;	

}



int UPhyWriteReg(int addr, int offset, int len, int value, int value1){

	char bank;

	char phy_addr;

	unsigned int mask;



	if(len == 32)

		mask = 0xffffffff;

	else

		mask  = ((1 << len) - 1);



	bank = ((addr & 0xff00) >> 8);

	if(bank == 0x00){

		//PHYD

		_U3_Write_Bank(U3_PHY_PG_U2PHY);

	}

	else if(bank == 0x01){

		//PHYA

		_U3_Write_Bank(U3_PHY_PG_PHYD);

	}

	

	phy_addr = (addr & 0xff);

	os_us_delay(i2cdelayus);

	_U3Write_Reg(phy_addr, value1);

	os_us_delay(i2cdelayus);

	

	return 0;

}



int UPhyWriteField(int addr, int offset, int len, int value){

	char bank;

	char phy_addr;

	unsigned int mask;

	char cur_value;

	char new_value;



	if(len == 32)

		mask = 0xffffffff;

	else

		mask  = ((1 << len) - 1);

	mask = mask << offset;

	

	bank = ((addr & 0xff00) >> 8);

	if(bank == 0x00){

		//PHYD

		_U3_Write_Bank(U3_PHY_PG_U2PHY);

	}

	else if(bank == 0x01){

		//PHYA

		_U3_Write_Bank(U3_PHY_PG_PHYD);

	}

	

	phy_addr = (addr & 0xff);

	cur_value = _U3Read_Reg(phy_addr);

	new_value = cur_value & (~mask) | (value << offset);

	os_us_delay(i2cdelayus);

	_U3Write_Reg(phy_addr, new_value);

	os_us_delay(i2cdelayus);



	return 0;

}



int fUPhyReadField(int addr, int offset, int len, int value){

	char bank;

	char phy_addr;

	unsigned int mask;



	if(len == 32)

		mask = 0xffffffff;

	else

		mask  = ((1 << len) - 1);

	

	bank = ((addr & 0xff00) >> 8);

	if(bank == 0x00){

		//PHYD

		_U3_Write_Bank(U3_PHY_PG_U2PHY);

	}

	else if(bank == 0x01){

		//PHYA

		_U3_Write_Bank(U3_PHY_PG_PHYD);

	}

	

	phy_addr = (addr & 0xff);

	

	return ((_U3Read_Reg(phy_addr) >> offset) & mask);

}



int bUPhyReadReg(int addr, int offset, int len, int value){

	char bank;

	char phy_addr;

	unsigned int mask;



	if(len == 32)

		mask = 0xffffffff;

	else

		mask  = ((1 << len) - 1);

	

	bank = ((addr & 0xff00) >> 8);

	if(bank == 0x00){

		//PHYD

		_U3_Write_Bank(U3_PHY_PG_U2PHY);

	}

	else if(bank == 0x01){

		//PHYA

		_U3_Write_Bank(U3_PHY_PG_PHYD);

	}

	

	phy_addr = (addr & 0xff);

	

	return _U3Read_Reg(phy_addr);

}

