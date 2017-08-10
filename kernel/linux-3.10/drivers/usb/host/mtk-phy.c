#include <linux/gfp.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#define U3_PHY_LIB
#include "mtk-phy.h"
#ifdef CONFIG_64BIT
#include <linux/soc/mediatek/x_typedef.h>
#else
#include <x_typedef.h>
#endif
#ifdef CONFIG_C60802_SUPPORT
#include "mtk-phy-c60802.h"
#endif
#ifdef CONFIG_D60802_SUPPORT
#include "mtk-phy-d60802.h"
#endif
#ifdef CONFIG_E60802_SUPPORT
#include "mtk-phy-e60802.h"
#endif

#ifdef CONFIG_C60802_SUPPORT
static struct u3phy_operator c60802_operators = {
	.init = phy_init_c60802,
	.change_pipe_phase = phy_change_pipe_phase_c60802,
	.u2_save_current_entry = u2_save_cur_en_c60802,
	.u2_save_current_recovery = u2_save_cur_re_c60802,
	.u2_slew_rate_calibration = u2_slew_rate_calibration_c60802,
};
#endif
#ifdef CONFIG_D60802_SUPPORT
static const struct u3phy_operator d60802_operators = {
	.init = phy_init_d60802,
	.change_pipe_phase = phy_change_pipe_phase_d60802,
	.u2_slew_rate_calibration = u2_slew_rate_calibration_d60802,
};
#endif
#ifdef CONFIG_E60802_SUPPORT
static const struct u3phy_operator e60802_operators = {
	.init = phy_init_e60802,
	.change_pipe_phase = phy_change_pipe_phase_e60802,
//	.eyescan_init = eyescan_init_e60802,
//	.eyescan = phy_eyescan_e60802,
	.u2_connect = u2_connect_e60802,
	.u2_disconnect = u2_disconnect_e60802,	
	//.u2_save_current_entry = u2_save_cur_en_e60802,
	//.u2_save_current_recovery = u2_save_cur_re_e60802,	
	.u2_slew_rate_calibration = u2_slew_rate_calibration_e60802,
};
#endif

PHY_INT32 u3phy_init(void){
	PHY_INT32 u3phy_version;
//	PHY_INT32 ret;
	if(u3phy != NULL){
		return PHY_TRUE;
	}
	//parse phy version
	u3phy = kmalloc(sizeof(struct u3phy_info), GFP_NOIO);
#ifdef CONFIG_ARCH_MT5399
	u3phy_version = 0xc60802a;
#else
    u3phy_version = 0xe60802a;
#endif
	//printk(KERN_ERR "phy version: %x\n", u3phy_version);
	u3phy->phy_version = u3phy_version;
	if(u3phy_version == 0xc60802a){
#ifdef CONFIG_C60802_SUPPORT	
    #ifdef CONFIG_U3_PHY_GPIO_SUPPORT
		u3phy->u2phy_regs_c = (struct u2phy_reg_c *)0x0;
		u3phy->u3phyd_regs_c = (struct u3phyd_reg_c *)0x100000;
		u3phy->u3phyd_bank2_regs_c = (struct u3phyd_bank2_reg_c *)0x200000;
		u3phy->u3phya_regs_c = (struct u3phya_reg_c *)0x300000;
		u3phy->u3phya_da_regs_c = (struct u3phya_da_reg_c *)0x400000;
		u3phy->sifslv_chip_regs_c = (struct sifslv_chip_reg_c *)0x500000;
		u3phy->sifslv_fm_regs_c = (struct sifslv_fm_feg_c *)0xf00000;
	#else
		u3phy->u2phy_regs_c = (struct u2phy_reg_c *)U2_PHY_BASE;
		u3phy->u3phyd_regs_c = (struct u3phyd_reg_c *)U3_PHYD_BASE;
		u3phy->u3phyd_bank2_regs_c = (struct u3phyd_bank2_reg_c *)U3_PHYD_B2_BASE;
		u3phy->u3phya_regs_c = (struct u3phya_reg_c *)U3_PHYA_BASE;
		u3phy->u3phya_da_regs_c = (struct u3phya_da_reg_c *)U3_PHYA_DA_BASE;
		u3phy->sifslv_chip_regs_c = (struct sifslv_chip_reg_c *)SIFSLV_CHIP_BASE;		
		u3phy->sifslv_fm_regs_c = (struct sifslv_fm_feg_c *)SIFSLV_FM_FEG_BASE;		
	#endif
		u3phy_ops = (struct u3phy_operator *)&c60802_operators;
#endif
	}
#ifdef CONFIG_D60802_SUPPORT
	else if(u3phy_version == 0xd60802a){
	#ifdef CONFIG_U3_PHY_GPIO_SUPPORT
		u3phy->u2phy_regs_d = (struct u2phy_reg_d *)0x0;
		u3phy->u3phyd_regs_d = (struct u3phyd_reg_d *)0x100000;
		u3phy->u3phyd_bank2_regs_d = (struct u3phyd_bank2_reg_d *)0x200000;
		u3phy->u3phya_regs_d = (struct u3phya_reg_d *)0x300000;
		u3phy->u3phya_da_regs_d = (struct u3phya_da_reg_d *)0x400000;
		u3phy->sifslv_chip_regs_d = (struct sifslv_chip_reg_d *)0x500000;
		u3phy->sifslv_fm_regs_d = (struct sifslv_fm_feg_d *)0xf00000;		
	#else
		u3phy->u2phy_regs_d = (struct u2phy_reg_d *)U2_PHY_BASE;
		u3phy->u3phyd_regs_d = (struct u3phyd_reg_d *)U3_PHYD_BASE;
		u3phy->u3phyd_bank2_regs_d = (struct u3phyd_bank2_reg_d *)U3_PHYD_B2_BASE;
		u3phy->u3phya_regs_d = (struct u3phya_reg_d *)U3_PHYA_BASE;
		u3phy->u3phya_da_regs_d = (struct u3phya_da_reg_d *)U3_PHYA_DA_BASE;
		u3phy->sifslv_chip_regs_d = (struct sifslv_chip_reg_d *)SIFSLV_CHIP_BASE;		
		u3phy->sifslv_fm_regs_d = (struct sifslv_fm_feg_d *)SIFSLV_FM_FEG_BASE;	
	#endif
		u3phy_ops = (struct u3phy_operator *)&d60802_operators;
	}
#endif
	else if(u3phy_version == 0xe60802a){
#ifdef CONFIG_E60802_SUPPORT
	#ifdef CONFIG_U3_PHY_GPIO_SUPPORT
		u3phy->u2phy_regs_e = (struct u2phy_reg_e *)0x0;
		u3phy->u3phyd_regs_e = (struct u3phyd_reg_e *)0x100000;
		u3phy->u3phyd_bank2_regs_e = (struct u3phyd_bank2_reg_e *)0x200000;
		u3phy->u3phya_regs_e = (struct u3phya_reg_e *)0x300000;
		u3phy->u3phya_da_regs_e = (struct u3phya_da_reg_e *)0x400000;
		u3phy->sifslv_chip_regs_e = (struct sifslv_chip_reg_e *)0x500000;
		u3phy->spllc_regs_e = (struct spllc_reg_e *)0x600000;
		u3phy->sifslv_fm_regs_e = (struct sifslv_fm_feg_e *)0xf00000;		
	#else
		u3phy->u2phy_regs_e = (struct u2phy_reg_e *)U2_PHY_BASE;
		u3phy->u3phyd_regs_e = (struct u3phyd_reg_e *)U3_PHYD_BASE;
		u3phy->u3phyd_bank2_regs_e = (struct u3phyd_bank2_reg_e *)U3_PHYD_B2_BASE;
		u3phy->u3phya_regs_e = (struct u3phya_reg_e *)U3_PHYA_BASE;
		u3phy->u3phya_da_regs_e = (struct u3phya_da_reg_e *)U3_PHYA_DA_BASE;
		u3phy->sifslv_chip_regs_e = (struct sifslv_chip_reg_e *)SIFSLV_CHIP_BASE;		
		u3phy->sifslv_fm_regs_e = (struct sifslv_fm_feg_e *)SIFSLV_FM_FEG_BASE;	
	#endif
		u3phy_ops = (struct u3phy_operator *)&e60802_operators;
#endif
	}
	else{
		printk(KERN_ERR "No match phy version\n");
		return PHY_FALSE;
	}
		

	return PHY_TRUE;
}

PHY_INT32 U3PhyWriteField8(PHY_UINT32  addr, PHY_INT32 offset, PHY_INT32 mask, PHY_INT32 value){
	PHY_INT8 cur_value;
	PHY_INT8 new_value;

	cur_value = U3PhyReadReg8(addr);
	new_value = (cur_value & (~mask))| ((value << offset) & mask);
	//udelay(i2cdelayus);
	U3PhyWriteReg8(addr, new_value);
	return PHY_TRUE;
}

PHY_INT32 U3PhyWriteField32(PHY_UINT32 addr, PHY_INT32 offset, PHY_INT32 mask, PHY_INT32 value){
	PHY_INT32 cur_value;
	PHY_INT32 new_value;

	cur_value = U3PhyReadReg32(addr);
	new_value = (cur_value & (~mask)) | ((value << offset) & mask);
	U3PhyWriteReg32(addr, new_value);
	//DRV_MDELAY(100);

	return PHY_TRUE;
}

PHY_INT32 U3PhyReadField8(PHY_UINT32 addr,PHY_INT32 offset,PHY_INT32 mask){
	
	return ((U3PhyReadReg8(addr) & mask) >> offset);
}

PHY_INT32 U3PhyReadField32(PHY_UINT32 addr, PHY_INT32 offset, PHY_INT32 mask){

	return ((U3PhyReadReg32(addr) & mask) >> offset);
}

