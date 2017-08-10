#include <linux/gfp.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <asm/io.h>
#ifdef CONFIG_64BIT
#include <linux/soc/mediatek/x_typedef.h>
#else
#include <x_typedef.h>
#endif
//#include <linux/dmapool.h>
#include "mtk-phy.h"
#include "xhci-mtk.h"
#if 0 //CC_USB3_FPGA
#include "mtk-sif-cmd.h"
/* TEST CHIP PHY define, edit this in different platform */
#define U3_PHY_I2C_DEV			0xc0
#define U3_PHY_PAGE				0xff


int dbg_sif_init(int argc, char** argv)
{
	SIF_V2_Init();
	return 0;
}


void _U3_Write_Bank(PHY_INT32 bankValue){
	char *pu1Buf;
	char *pu2Buf;
	
	pu1Buf = (char *)kmalloc(1, GFP_NOIO);
	pu2Buf = pu1Buf;
	pu1Buf[0] = bankValue;
	SIF_V2_X_Write(0, 0x100, U3_PHY_I2C_DEV, 1, U3_PHY_PAGE, pu1Buf, 1);
	printk(KERN_ERR "Switch I2C bank [0x%x]\n", bankValue);
	kfree(pu2Buf);
}

PHY_INT32 _U3Write_Reg(PHY_INT32 address, PHY_INT32 value){
	char *pu1Buf;
	char *pu2Buf;
	
	pu1Buf = (char *)kmalloc(1, GFP_NOIO);
	pu2Buf = pu1Buf;
	pu1Buf[0] = value;
	SIF_V2_X_Write(0, 0x100, U3_PHY_I2C_DEV, 1, address, pu1Buf, 1);
	printk(KERN_ERR "Write I2C reg[0x%x] value[0x%x]\n", address, value);
	kfree(pu2Buf);
}

PHY_INT32 _U3Read_Reg(PHY_INT32 address){
	PHY_INT8 *pu1Buf;
	PHY_INT32 ret;
	
	pu1Buf = (char *)kmalloc(1, GFP_NOIO);
	ret = SIF_V2_X_Read(0, 0x100, U3_PHY_I2C_DEV, 1, address, pu1Buf, 1);
	if(ret == PHY_FALSE){
		printk(KERN_ERR "Read failed\n");
		return PHY_FALSE;
	}
	ret = (char)pu1Buf[0];
	kfree(pu1Buf);
	return ret;
	
}

#else
#define U3_PHY_PG_U2PHY     0x00
#define U3_PHY_PG_PHYD      0x10
#define U3_PHY_PG_PHYD2     0x20
#define U3_PHY_PG_PHYA      0x30
#define U3_PHY_PG_PHYA_DA   0x40
#define U3_PHY_PG_SPLLC     0x60
#define U3_PHY_PG_FM        0xF0

static ulong u4PhyBankOffset;

int dbg_sif_init(int argc, char** argv)
{
	return 0;
}

void _U3_Write_Bank(int bankValue){

	switch (bankValue) {
		case U3_PHY_PG_U2PHY:
		u4PhyBankOffset =(ulong) U3_PHY_PG_U2PHY_OFFSET;
		break;
        case U3_PHY_PG_PHYD:
            u4PhyBankOffset = (ulong)U3_PHY_PG_PHYD_OFFSET;
            break;
        case U3_PHY_PG_PHYD2:
            u4PhyBankOffset = (ulong)U3_PHY_PG_PHYD2_OFFSET;
            break;
        case U3_PHY_PG_PHYA:
            u4PhyBankOffset = (ulong)U3_PHY_PG_PHYA_OFFSET;
            break;
        case U3_PHY_PG_PHYA_DA:
            u4PhyBankOffset = (ulong)U3_PHY_PG_PHYA_DA_OFFSET;
            break;
        case U3_PHY_PG_SPLLC:
            u4PhyBankOffset = (ulong)U3_PHY_PG_SPLLC_OFFSET;
            break;
        case U3_PHY_PG_FM:
            u4PhyBankOffset = (ulong)U3_PHY_PG_FM_OFFSET;
            break;
        default:
            break;
    }

}

int _U3Write_Reg(ulong address, int value){
    ulong add_buf,u4Value,u1tmp;
    add_buf = address & 0xFC;
    u1tmp = value & 0xFF;
	u4Value = readl((void*)(add_buf+u4PhyBankOffset));

	switch (address % 4) {
		case 0:
			u4Value = (u4Value & 0xFFFFFF00) | u1tmp;
			break;
		case 1:
			u4Value = (u4Value & 0xFFFF00FF) | (u1tmp<<8);
			break;
        case 2:
			u4Value = (u4Value & 0xFF00FFFF) | (u1tmp<<16);
			break;
		default:
			u4Value = (u4Value & 0x00FFFFFF) | (u1tmp<<24);
			break;
	}

    writel(u4Value, (void*)(add_buf+u4PhyBankOffset));

//	printk(KERN_ERR "Write U3 reg[0x%x] value[0x%x]\n", address, value);
	return 0;
}

int _U3Read_Reg(ulong address){
	char ret;
    ulong add_buf,reval;

    add_buf = address & 0xFC;
	reval = readl((UPTR*)(ulong)(add_buf+u4PhyBankOffset));
    
	switch (address % 4) {
		case 0:
			ret = (char)(reval & 0xFF);
			break;
		case 1:
			ret = (char)((reval>>8) & 0xFF);
			break;
        case 2:
			ret = (char)((reval>>16) & 0xFF);
			break;
		default:
			ret = (char)((reval>>24) & 0xFF);
			break;
	}
//	printk(KERN_ERR "Read U3 reg[0x%x] value[0x%x]\n", address, ret);
	return ret;
	
}
#endif

PHY_INT32 U3PhyWriteReg32(PHY_UINT32 addr, PHY_INT32 data){
	PHY_INT32 bank;
	PHY_INT32 addr8;
	PHY_INT32 data_0, data_1, data_2, data_3;

	bank = (addr >> 16) & 0xff;
	addr8 = addr & 0xff;
	data_0 = data & 0xff;
	data_1 = (data>>8) & 0xff;
	data_2 = (data>>16) & 0xff;
	data_3 = (data>>24) & 0xff;
	
	_U3_Write_Bank(bank);
	_U3Write_Reg(addr8, data_0);
	_U3Write_Reg(addr8+1, data_1);
	_U3Write_Reg(addr8+2, data_2);
	_U3Write_Reg(addr8+3, data_3);

	return 0;
}

PHY_INT32 U3PhyReadReg32(PHY_UINT32 addr){
	PHY_INT32 bank;
	PHY_INT32 addr8;
	PHY_INT32 data;

	bank = (addr >> 16) & 0xff;
	addr8 = addr & 0xff;

	_U3_Write_Bank(bank);
	data = _U3Read_Reg(addr8);
	data |= (_U3Read_Reg(addr8+1) << 8);
	data |= (_U3Read_Reg(addr8+2) << 16);
	data |= (_U3Read_Reg(addr8+3) << 24);
	return data;
}

PHY_INT32 U3PhyWriteReg8(PHY_UINT32 addr, PHY_UINT8 data)
{
	PHY_INT32 bank;
	PHY_INT32 addr8;

//	printk(KERN_ERR "[PHY] write regs addr 0x%x value 0x%x\n", addr, data);
	
	bank = (addr >> 16) & 0xff;
	addr8 = addr & 0xff;
	_U3_Write_Bank(bank);
	_U3Write_Reg(addr8, data);
	
	return PHY_TRUE;
}

PHY_INT8 U3PhyReadReg8(PHY_UINT32 addr){
	PHY_INT32 bank;
	PHY_INT32 addr8;
	PHY_INT32 data;

	bank = (addr >> 16) & 0xff;
	addr8 = addr & 0xff;
	_U3_Write_Bank(bank);
	data = _U3Read_Reg(addr8);
//	printk(KERN_ERR "[PHY] read regs addr 0x%x value 0x%x\n", addr, data);
	return data;
}


