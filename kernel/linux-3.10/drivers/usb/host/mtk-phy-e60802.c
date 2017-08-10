#include "mtk-phy.h"
#ifdef CONFIG_64BIT
#include <linux/soc/mediatek/mt53xx_linux.h>
#include <linux/soc/mediatek/x_typedef.h>
#else
#include <mach/mt53xx_linux.h>
#include <x_typedef.h>
#endif
#ifdef CONFIG_E60802_SUPPORT
#include "mtk-phy-e60802.h"

PHY_INT32 phy_init_e60802(struct u3phy_info *info){	

    /**************phya part******************/
    // DA_SSUSB_XTAL_EXT_EN = 2'b10
	U3PhyWriteField32(((UPTR)&info->u3phya_da_regs_e->reg0)
		, E60802_RG_SSUSB_XTAL_EXT_EN_U3_OFST, E60802_RG_SSUSB_XTAL_EXT_EN_U3
		, 0x2);
    // RG_SSUSB_FRC_XTAL_RX_PWD = 1
	U3PhyWriteField32(((UPTR)&info->spllc_regs_e->u3d_xtalctl3)
		, E60802_RG_SSUSB_FRC_XTAL_RX_PWD_OFST, E60802_RG_SSUSB_FRC_XTAL_RX_PWD
		, 0x1);
    // DA_SSUSB_XTAL_RX_PWD = 1
	U3PhyWriteField32(((UPTR)&info->spllc_regs_e->u3d_xtalctl3)
		, E60802_RG_SSUSB_XTAL_RX_PWD_OFST, E60802_RG_SSUSB_XTAL_RX_PWD
		, 0x1);
    // DA_SSUSB_LFPS_DEGLITCH = 0x3
//	U3PhyWriteField32(((PHY_UINT32)&info->u3phya_da_regs_e->reg32)
//		, E60802_RG_SSUSB_LFPS_DEGLITCH_U3_OFST, E60802_RG_SSUSB_LFPS_DEGLITCH_U3
//		, 0x3);
    // RG_SSUSB_IDEM_BIAS = 0x1
	U3PhyWriteField32(((UPTR)&info->u3phya_regs_e->reg5)
		, E60802_RG_SSUSB_IDEM_BIAS_OFST, E60802_RG_SSUSB_IDEM_BIAS
		, 0x1);
    // RG_SSUSB_TX_EIDLE_CM = 4'b1111
	U3PhyWriteField32(((UPTR)&info->u3phya_regs_e->reg6)
		, E60802_RG_SSUSB_TX_EIDLE_CM_OFST, E60802_RG_SSUSB_TX_EIDLE_CM
		, 0xf);
    // RG_SSUSB_RXDET_STB2_SET_P3 = 0x1A
	U3PhyWriteField32(((UPTR)&info->u3phyd_bank2_regs_e->b2_phyd_rxdet2)
		, E60802_RG_SSUSB_RXDET_STB2_SET_P3_OFST, E60802_RG_SSUSB_RXDET_STB2_SET_P3
		, 0x1A);

    if(IS_IC_MT5890_ES1())
    {
        printk("MT5890 temp USB2.0 Phy setting\n");
        U3PhyWriteField32(((UPTR)&info->u2phy_regs_e->usbphyacr1)
            ,E60802_RG_USB20_TERM_VREF_SEL_OFST,E60802_RG_USB20_TERM_VREF_SEL, 0x0);
        U3PhyWriteField32(((UPTR)&info->u2phy_regs_e->usbphyacr6)
            ,E60802_RG_USB20_SQTH_OFST,E60802_RG_USB20_SQTH, 0xB);
    }
    else
    {
        U3PhyWriteField32(((UPTR)&info->u2phy_regs_e->usbphyacr6)
            ,E60802_RG_USB20_DISCTH_OFST,E60802_RG_USB20_DISCTH, 0xB);
        U3PhyWriteField32(((UPTR)&info->u3phya_regs_e->regb)
            , E60802_RG_SSUSB_RXAFE_RESERVE_OFST, E60802_RG_SSUSB_RXAFE_RESERVE
            , 0xf);
    }
    
    /**********u2phy part******************/
    U3PhyWriteField32(((UPTR)&info->u2phy_regs_e->usbphyacr5)
        ,E60802_RG_USB20_HS_100U_U3_EN_OFST,E60802_RG_USB20_HS_100U_U3_EN, 0x1);
    U3PhyWriteField32(((UPTR)&info->u2phy_regs_e->usbphyacr5)
        ,E60802_RG_USB20_DISCD_OFST,E60802_RG_USB20_DISCD, 0x2);
    U3PhyWriteField32(((UPTR)&info->u2phy_regs_e->usbphyacr5)
        ,E60802_RG_USB20_DISC_FIT_EN_OFST,E60802_RG_USB20_DISC_FIT_EN, 1);

#if 0
	/**********u2phy part******************/
	//enabe VBUS CMP to save power since cause 6593 will use OTG
	U3PhyWriteField32(((PHY_UINT32)&info->u2phy_regs_e->usbphyacr6)
		,E60802_RG_USB20_OTG_VBUSCMP_EN_OFST,E60802_RG_USB20_OTG_VBUSCMP_EN, 0x1);
	
	/*********phyd part********************/
	//disable ssusb_p3_entry to work around resume from P3
	U3PhyWriteField32(((PHY_UINT32)&info->u3phyd_regs_e->phyd_lfps0)
		, E60802_RG_SSUSB_P3_ENTRY_OFST, E60802_RG_SSUSB_P3_ENTRY, 0x0);
	U3PhyWriteField32(((PHY_UINT32)&info->u3phyd_regs_e->phyd_lfps0)
		, E60802_RG_SSUSB_P3_ENTRY_SEL_OFST, E60802_RG_SSUSB_P3_ENTRY_SEL, 0x1);
	/**************phya part******************/
	// Enable internal VRT to bypass bandgap voltage too high issue
	U3PhyWriteField32(((PHY_UINT32)&info->u3phya_regs_e->reg0)
		, E60802_RG_SSUSB_INTR_EN_OFST, E60802_RG_SSUSB_INTR_EN, 0x1);
	//RG_SSUSB_XTAL_TOP_RESERVE<15:11> =10001
	U3PhyWriteField32(((PHY_UINT32)&info->u3phya_regs_e->reg1)
		, E60802_RG_SSUSB_XTAL_TOP_RESERVE_OFST, E60802_RG_SSUSB_XTAL_TOP_RESERVE
		, (0x11<<11));
	/*************phya da part*****************/
	// fine tune SSC delta1 to let SSC min average ~0ppm
	U3PhyWriteField32(((PHY_UINT32)&info->u3phya_da_regs_e->reg19)
		, E60802_RG_SSUSB_PLL_SSC_DELTA1_U3_OFST, E60802_RG_SSUSB_PLL_SSC_DELTA1_U3
		, 0x42);
	// fine tune SSC delta to let SSC min average ~0ppm
	U3PhyWriteField32(((PHY_UINT32)&info->u3phya_da_regs_e->reg21)
		, E60802_RG_SSUSB_PLL_SSC_DELTA_U3_OFST, E60802_RG_SSUSB_PLL_SSC_DELTA_U3
		, 0x3e);
	// Fine tune SYSPLL to improve phase noise
	U3PhyWriteField32(((PHY_UINT32)&info->u3phya_da_regs_e->reg4)
		, E60802_RG_SSUSB_PLL_BC_U3_OFST, E60802_RG_SSUSB_PLL_BC_U3, 0x3);
	U3PhyWriteField32(((PHY_UINT32)&info->u3phya_da_regs_e->reg4)
		, E60802_RG_SSUSB_PLL_DIVEN_U3_OFST, E60802_RG_SSUSB_PLL_DIVEN_U3, 0x2);
	U3PhyWriteField32(((PHY_UINT32)&info->u3phya_da_regs_e->reg5)
		, E60802_RG_SSUSB_PLL_IC_U3_OFST, E60802_RG_SSUSB_PLL_IC_U3, 0x1);
	U3PhyWriteField32(((PHY_UINT32)&info->u3phya_da_regs_e->reg5)
		, E60802_RG_SSUSB_PLL_BR_U3_OFST, E60802_RG_SSUSB_PLL_BR_U3, 0x0);
	U3PhyWriteField32(((PHY_UINT32)&info->u3phya_da_regs_e->reg6)
		, E60802_RG_SSUSB_PLL_IR_U3_OFST, E60802_RG_SSUSB_PLL_IR_U3, 0x1);
	U3PhyWriteField32(((PHY_UINT32)&info->u3phya_da_regs_e->reg7)
		, E60802_RG_SSUSB_PLL_BP_U3_OFST, E60802_RG_SSUSB_PLL_BP_U3, 0xf);
	//disable ssusb_p3_bias_pwd to work around resume from P3
	U3PhyWriteField32(((PHY_UINT32)&info->spllc_regs_e->u3d_xtalctl_2)
		, E60802_RG_SSUSB_P3_BIAS_PWD_OFST, E60802_RG_SSUSB_P3_BIAS_PWD, 0x0);
#endif
	return PHY_TRUE;
}

#define PHY_DRV_SHIFT	3
#define PHY_PHASE_SHIFT	3
#define PHY_PHASE_DRV_SHIFT	1
PHY_INT32 phy_change_pipe_phase_e60802(struct u3phy_info *info, PHY_INT32 phy_drv, PHY_INT32 pipe_phase){
	PHY_INT32 drv_reg_value;
	PHY_INT32 phase_reg_value;
	PHY_INT32 temp;

	drv_reg_value = phy_drv << PHY_DRV_SHIFT;
	phase_reg_value = (pipe_phase << PHY_PHASE_SHIFT) | (phy_drv << PHY_PHASE_DRV_SHIFT);
	temp = U3PhyReadReg8(((UPTR)&info->sifslv_chip_regs_e->gpio_ctla)+2);
	temp &= ~(0x3 << PHY_DRV_SHIFT);
	temp |= drv_reg_value;
	U3PhyWriteReg8(((UPTR)&info->sifslv_chip_regs_e->gpio_ctla)+2, temp);
	temp = U3PhyReadReg8(((UPTR)&info->sifslv_chip_regs_e->gpio_ctla)+3);
	temp &= ~((0x3 << PHY_PHASE_DRV_SHIFT) | (0x1f << PHY_PHASE_SHIFT));
	temp |= phase_reg_value;
	U3PhyWriteReg8(((UPTR)&info->sifslv_chip_regs_e->gpio_ctla)+3, temp);

	return PHY_TRUE;
}

PHY_INT32 u2_connect_e60802(struct u3phy_info *info){
	//for better LPM BESL value	
	U3PhyWriteField32(((UPTR)&info->u2phy_regs_e->u2phydcr1)
		, E60802_RG_USB20_SW_PLLMODE_OFST, E60802_RG_USB20_SW_PLLMODE, 0x1);
	return PHY_TRUE;
}

PHY_INT32 u2_disconnect_e60802(struct u3phy_info *info){
	//for better LPM BESL value	
	U3PhyWriteField32(((UPTR)&info->u2phy_regs_e->u2phydcr1)
		, E60802_RG_USB20_SW_PLLMODE_OFST, E60802_RG_USB20_SW_PLLMODE, 0x0);
	return PHY_TRUE;
}

PHY_INT32 u2_save_cur_en_e60802(struct u3phy_info *info){
	return PHY_TRUE;
}

PHY_INT32 u2_save_cur_re_e60802(struct u3phy_info *info){
	return PHY_TRUE;
}

PHY_INT32 u2_slew_rate_calibration_e60802(struct u3phy_info *info){
	PHY_INT32 i=0;
	PHY_INT32 fgRet = 0;	
	PHY_INT32 u4FmOut = 0;	
	PHY_INT32 u4Tmp = 0;

	// => RG_USB20_HSTX_SRCAL_EN = 1
	// enable HS TX SR calibration
	U3PhyWriteField32(((UPTR)&info->u2phy_regs_e->usbphyacr5)
		, E60802_RG_USB20_HSTX_SRCAL_EN_OFST, E60802_RG_USB20_HSTX_SRCAL_EN, 1);
	DRV_MDELAY(1);	

	// => RG_FRCK_EN = 1    
	// Enable free run clock
	U3PhyWriteField32(((UPTR)&info->sifslv_fm_regs_e->fmmonr1)
		, E60802_RG_FRCK_EN_OFST, E60802_RG_FRCK_EN, 0x1);

	// => RG_CYCLECNT = 0x400
	// Setting cyclecnt = 0x400
	U3PhyWriteField32(((UPTR)&info->sifslv_fm_regs_e->fmcr0)
		, E60802_RG_CYCLECNT_OFST, E60802_RG_CYCLECNT, 0x400);

	// => RG_FREQDET_EN = 1
	// Enable frequency meter
	U3PhyWriteField32(((UPTR)&info->sifslv_fm_regs_e->fmcr0)
		, E60802_RG_FREQDET_EN_OFST, E60802_RG_FREQDET_EN, 0x1);

	// wait for FM detection done, set 10ms timeout
	for(i=0; i<10; i++){
		// => u4FmOut = USB_FM_OUT
		// read FM_OUT
		u4FmOut = U3PhyReadReg32(((UPTR)&info->sifslv_fm_regs_e->fmmonr0));
		//printk(KERN_ERR "FM_OUT value: u4FmOut = %d(0x%08X)\n", u4FmOut, u4FmOut);

		// check if FM detection done 
		if (u4FmOut != 0)
		{
			fgRet = 0;
			//printk(KERN_ERR "FM detection done! loop = %d\n", i);
			
			break;
		}

		fgRet = 1;
		DRV_MDELAY(1);
	}
	// => RG_FREQDET_EN = 0
	// disable frequency meter
	U3PhyWriteField32(((UPTR)&info->sifslv_fm_regs_e->fmcr0)
		, E60802_RG_FREQDET_EN_OFST, E60802_RG_FREQDET_EN, 0);

	// => RG_FRCK_EN = 0
	// disable free run clock
	U3PhyWriteField32(((UPTR)&info->sifslv_fm_regs_e->fmmonr1)
		, E60802_RG_FRCK_EN_OFST, E60802_RG_FRCK_EN, 0);

	// => RG_USB20_HSTX_SRCAL_EN = 0
	// disable HS TX SR calibration
	U3PhyWriteField32(((UPTR)&info->u2phy_regs_e->usbphyacr5)
		, E60802_RG_USB20_HSTX_SRCAL_EN_OFST, E60802_RG_USB20_HSTX_SRCAL_EN, 0);
	DRV_MDELAY(1);

	if(u4FmOut == 0){
		U3PhyWriteField32(((UPTR)&info->u2phy_regs_e->usbphyacr5)
				, E60802_RG_USB20_HSTX_SRCTRL_OFST, E60802_RG_USB20_HSTX_SRCTRL, 0x4);

		fgRet = 1;
	}
	else{
		// set reg = (1024/FM_OUT) * REF_CK * U2_SR_COEF_E60802 / 1000 (round to the nearest digits)
		u4Tmp = (((1024 * REF_CK * U2_SR_COEF_E60802) / u4FmOut) + 500) / 1000;
		printk("SR calibration value u1SrCalVal = %d\n", (PHY_UINT8)u4Tmp);
		U3PhyWriteField32(((UPTR)&info->u2phy_regs_e->usbphyacr5)
				, E60802_RG_USB20_HSTX_SRCTRL_OFST, E60802_RG_USB20_HSTX_SRCTRL, u4Tmp);
	}

	return fgRet;
}


#endif
