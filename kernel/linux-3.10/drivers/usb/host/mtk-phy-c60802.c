#include "mtk-phy.h"
#ifdef CONFIG_E60802_SUPPORT
#include "mtk-phy-c60802.h"

PHY_INT32 phy_init_c60802(struct u3phy_info *info){
	PHY_UINT8 temp;

	//****** u2phy part *******//
//	DRV_MDELAY(100);
	//RG_USB20_BGR_EN = 1
	temp = U3PhyReadReg8((PHY_UINT32)(&info->u2phy_regs_c->u2phyac0));
	temp |= 0x1;
	U3PhyWriteReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phyac0),temp);
//	DRV_MDELAY(100);
	//RG_USB20_REF_EN = 1
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phyac0)+1);
	temp |= 0x80;
	U3PhyWriteReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phyac0)+1,temp);
//	DRV_MDELAY(100);
	//RG_USB20_SW_PLLMODE = 2
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydcr1)+2);
	temp &= ~(0x3<<2);
	temp |= (0x2<<2);
	U3PhyWriteReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydcr1)+2,temp);
//	DRV_MDELAY(100);
	//RG_USB20_DISCTH = 0xB for some device happened disconnect issue
	temp = U3PhyReadReg8((PHY_UINT32)(&info->u2phy_regs_c->u2phyacr2)+2);
	temp &= ~(0xF);
	temp |= (0xB);
	U3PhyWriteReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phyacr2)+2,temp);

	//****** u3phyd part *******//
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u3phyd_regs_c->phyd_lfps0));
	temp &= ~0x1F;
	temp |= 0x19;
	U3PhyWriteReg8(((PHY_UINT32)&info->u3phyd_regs_c->phyd_lfps0), temp);
//	DRV_MDELAY(100);
	//turn off phy clock gating, marked in normal case
	//U3PhyWriteReg8(&info->u3phyd_regs_c->phyd_mix1+2,0x0);
	//mdelay(100);
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u3phyd_regs_c->phyd_lfps0)+1);
	temp |= 0x80;
	U3PhyWriteReg8(((PHY_UINT32)&info->u3phyd_regs_c->phyd_lfps0)+1, temp);
//	DRV_MDELAY(100);
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u3phyd_regs_c->phyd_pll_0));
	temp |= 0x10;
	U3PhyWriteReg8(((PHY_UINT32)&info->u3phyd_regs_c->phyd_pll_0), temp);
//	DRV_MDELAY(100);

	//****** u3phyd bank2 part *******//
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u3phyd_bank2_regs_c->b2_phyd_top1)+2);
	temp &= ~0x03;
	temp |= 0x01;
	U3PhyWriteReg8(((PHY_UINT32)&info->u3phyd_bank2_regs_c->b2_phyd_top1)+2,temp);
//	DRV_MDELAY(100);
    //rg_ssusb_frc_ring_bypass_det = 0
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u3phyd_bank2_regs_c->b2_rosc_0));
	temp &= ~0x02;
	U3PhyWriteReg8(((PHY_UINT32)&info->u3phyd_bank2_regs_c->b2_rosc_0),temp);
    //rg_ssusb_ring_osc_frc_sel = 1
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u3phyd_bank2_regs_c->b2_rosc_1));
	temp |= 0x01;
	U3PhyWriteReg8(((PHY_UINT32)&info->u3phyd_bank2_regs_c->b2_rosc_1),temp);
    //  DRV_MDELAY(100);
	
	//****** u3phya part *******//
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u3phya_regs_c->reg7)+1);
	temp &= ~0x40;
	U3PhyWriteReg8(((PHY_UINT32)&info->u3phya_regs_c->reg7)+1, temp);
//	DRV_MDELAY(100);

	//****** u3phya_da part *******//
	// set SSC to pass electrical compliance SSC min
	U3PhyWriteReg8(((PHY_UINT32)&info->u3phya_da_regs_c->reg21)+2, 0x50);
//	DRV_MDELAY(100);

	//****** u3phy chip part *******//
// SOC don't need it
//	temp = U3PhyReadReg8(((PHY_UINT32)&info->sifslv_chip_regs_c->gpio_ctlb)+3);
//	temp |= 0x01;
//	U3PhyWriteReg8(((PHY_UINT32)&info->sifslv_chip_regs_c->gpio_ctlb)+3, temp);
	//phase set
//	U3PhyWriteReg8(((PHY_UINT32)&info->sifslv_chip_regs_c->gpio_ctlc)+2, 0x10);
//	U3PhyWriteReg8(((PHY_UINT32)&info->sifslv_chip_regs_c->gpio_ctlc)+3, 0x3c);
	
	return 0;
}

#define PHY_DRV_SHIFT	3
#define PHY_PHASE_SHIFT	3
#define PHY_PHASE_DRV_SHIFT	1
PHY_INT32 phy_change_pipe_phase_c60802(struct u3phy_info *info, PHY_INT32 phy_drv, PHY_INT32 pipe_phase){
	PHY_INT32 drv_reg_value;
	PHY_INT32 phase_reg_value;
	PHY_INT32 temp;

	drv_reg_value = phy_drv << PHY_DRV_SHIFT;
	phase_reg_value = (pipe_phase << PHY_PHASE_SHIFT) | (phy_drv << PHY_PHASE_DRV_SHIFT);
	temp = U3PhyReadReg8(((PHY_UINT32)&info->sifslv_chip_regs_c->gpio_ctlc)+2);
	temp &= ~(0x3 << PHY_DRV_SHIFT);
	temp |= drv_reg_value;
	U3PhyWriteReg8(((PHY_UINT32)&info->sifslv_chip_regs_c->gpio_ctlc)+2, temp);
	temp = U3PhyReadReg8(((PHY_UINT32)&info->sifslv_chip_regs_c->gpio_ctlc)+3);
	temp &= ~((0x3 << PHY_PHASE_DRV_SHIFT) | (0x1f << PHY_PHASE_SHIFT));
	temp |= phase_reg_value;
	U3PhyWriteReg8(((PHY_UINT32)&info->sifslv_chip_regs_c->gpio_ctlc)+3, temp);
	return PHY_TRUE;
}




PHY_INT32 u2_save_cur_en_c60802(struct u3phy_info *info){
	PHY_INT32 temp;
	//****** u2phy part *******//
	//switch to USB function. (system register, force ip into usb mode)
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0)+3);
	temp &= ~(0x1<<2);
	U3PhyWriteReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0)+3, temp);
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm1)+2);
	temp &= ~(0x1);
	U3PhyWriteReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm1)+2, temp);
	//Release force suspendm.  (force_suspendm=0) (let suspendm=1, enable usb 480MHz pll
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0)+2);
	temp &= ~(0x1<<2);
	U3PhyWriteReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0)+2, temp);
	//RG_DPPULLDOWN
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0));
	temp |= 0x1<<6;
	U3PhyWriteReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0), temp);
	//RG_DMPULLDOWN
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0));
	temp |= 0x1<<7;
	U3PhyWriteReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0), temp);
	//RG_XCVRSEL[1:0]
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0));
	temp &= ~(0x3<<4);
	temp |= 0x1<<4;
	U3PhyWriteReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0), temp);
	//RG_TERMSEL
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0));
	temp |= 0x1<<2;
	U3PhyWriteReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0), temp);
	//RG_DATAIN[3:0]
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0)+1);
	temp &= ~(0xf<<2);
	U3PhyWriteReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0)+1, temp);
	//force_dp_pulldown
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0)+2);
	temp |= 0x1<<4;
	U3PhyWriteReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0)+2, temp);
	//force_dm_pulldown
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0)+2);
	temp |= 0x1<<5;
	U3PhyWriteReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0)+2, temp);
	//force_xcvrsel
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0)+2);
	temp |= 0x1<<3;
	U3PhyWriteReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0)+2, temp);
	//force_termsel
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0)+2);
	temp |= 0x1<<1;
	U3PhyWriteReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0)+2, temp);
	//force_datain
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0)+2);
	temp |= 0x1<<7;
	U3PhyWriteReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0)+2, temp);
	//DP/DM BC1.1 path Disable
	//RG_USB20_PHY_REV[7]
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phyacr3));
	temp &= ~(0x1<<7);
	U3PhyWriteReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phyacr3), temp);
	//OTG Disable
	//wait 800us (1. force_suspendm=1, 2. set wanted utmi value, 3. rg_usb20_pll_stable=1)
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phyacr2)+3);
	temp &= ~(0x1<<3);
	U3PhyWriteReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phyacr2)+3, temp);
	//(let suspendm=0, set utmi into analog power down )
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0)+2);
	temp |= 0x1<<2;
	U3PhyWriteReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0)+2, temp);
	
	return PHY_TRUE;
}

PHY_INT32 u2_save_cur_re_c60802(struct u3phy_info *info){
	PHY_INT32 temp;
	//****** u2phy part *******//
	//switch to USB function. (system register, force ip into usb mode)
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0)+3);
	temp &= ~(0x1<<2);
	U3PhyWriteReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0)+3, temp);
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm1)+2);
	temp &= ~(0x1);
	U3PhyWriteReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm1)+2, temp);
	//Release force suspendm.  (force_suspendm=0) (let suspendm=1, enable usb 480MHz pll
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0)+2);
	temp &= ~(0x1<<2);
	U3PhyWriteReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0)+2, temp);
	//RG_DPPULLDOWN
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0));
	temp &= ~(0x1<<6);
	U3PhyWriteReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0), temp);
	//RG_DMPULLDOWN
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0));
	temp &= ~(0x1<<7);
	U3PhyWriteReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0), temp);
	//RG_XCVRSEL[1:0]
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0));
	temp &= ~(0x3<<4);
	U3PhyWriteReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0), temp);
	//RG_TERMSEL
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0));
	temp &= ~(0x1<<2);
	U3PhyWriteReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0), temp);
	//RG_DATAIN[3:0]
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0)+1);
	temp &= ~(0xf<<2);
	U3PhyWriteReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0)+1, temp);
	//force_dp_pulldown
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0)+2);
	temp &= ~(0x1<<4);
	U3PhyWriteReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0)+2, temp);
	//force_dm_pulldown
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0)+2);
	temp &= ~(0x1<<5);
	U3PhyWriteReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0)+2, temp);
	//force_xcvrsel
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0)+2);
	temp &= ~(0x1<<3);
	U3PhyWriteReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0)+2, temp);
	//force_termsel
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0)+2);
	temp &= ~(0x1<<1);
	U3PhyWriteReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0)+2, temp);
	//force_datain
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0)+2);
	temp &= ~(0x1<<7);
	U3PhyWriteReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phydtm0)+2, temp);
	//DP/DM BC1.1 path Disable
	//RG_USB20_PHY_REV[7]
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phyacr3));
	temp &= ~(0x1<<7);
	U3PhyWriteReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phyacr3), temp);
	//OTG Disable
	//wait 800us (1. force_suspendm=1, 2. set wanted utmi value, 3. rg_usb20_pll_stable=1)
	temp = U3PhyReadReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phyacr2)+3);
	temp |= 0x1<<3;
	U3PhyWriteReg8(((PHY_UINT32)&info->u2phy_regs_c->u2phyacr2)+3, temp);

	return PHY_TRUE;
}

PHY_INT32 u2_slew_rate_calibration_c60802(struct u3phy_info *info){
	PHY_INT32 i=0;
//	PHY_INT32 j=0;
//	PHY_INT8 u1SrCalVal = 0;
//	PHY_INT8 u1Reg_addr_HSTX_SRCAL_EN;
	PHY_INT32 fgRet = 0;	
	PHY_INT32 u4FmOut = 0;	
	PHY_INT32 u4Tmp = 0;
	PHY_INT32 temp;

	// => RG_USB20_HSTX_SRCAL_EN = 1
	// enable HS TX SR calibration
	temp = U3PhyReadReg32(((PHY_UINT32)&info->u2phy_regs_c->u2phyacr0));
	temp |= (0x1<<23);
	U3PhyWriteReg32(((PHY_UINT32)&info->u2phy_regs_c->u2phyacr0), temp);

	DRV_MDELAY(1);

	// => RG_FRCK_EN = 1    
	// Enable free run clock
	temp = U3PhyReadReg32(((PHY_UINT32)&info->sifslv_fm_regs_c->fmmonr1));
	temp |= (0x1<<8);
	U3PhyWriteReg32(((PHY_UINT32)&info->sifslv_fm_regs_c->fmmonr1), temp);

	// => RG_CYCLECNT = 400
	// Setting cyclecnt =400
	temp = U3PhyReadReg32(((PHY_UINT32)&info->sifslv_fm_regs_c->fmcr0));
	temp &= ~(0xffffff);
	temp |= 0x400;
	U3PhyWriteReg32(((PHY_UINT32)&info->sifslv_fm_regs_c->fmcr0), temp);
	
	// => RG_FREQDET_EN = 1
	// Enable frequency meter
	temp = U3PhyReadReg32(((PHY_UINT32)&info->sifslv_fm_regs_c->fmcr0));
	temp |= (0x1<<24);
	U3PhyWriteReg32(((PHY_UINT32)&info->sifslv_fm_regs_c->fmcr0), temp);

	// wait for FM detection done, set 10ms timeout
	for(i=0; i<10; i++){
		// => u4FmOut = USB_FM_OUT
		// read FM_OUT
		u4FmOut = U3PhyReadReg32(((PHY_UINT32)&info->sifslv_fm_regs_c->fmmonr0));
		printk("FM_OUT value: u4FmOut = %d(0x%08X)\n", u4FmOut, u4FmOut);

		// check if FM detection done 
		if (u4FmOut != 0)
		{
			fgRet = 0;
			printk("FM detection done! loop = %d\n", i);
			
			break;
		}

		fgRet = 1;
		DRV_MDELAY(1);
	}
	// => RG_FREQDET_EN = 0
	// disable frequency meter
	temp = U3PhyReadReg32(((PHY_UINT32)&info->sifslv_fm_regs_c->fmcr0));
	temp &= ~(0x1<<24);
	U3PhyWriteReg32(((PHY_UINT32)&info->sifslv_fm_regs_c->fmcr0), temp);
	// => RG_FRCK_EN = 0
	// disable free run clock
	temp = U3PhyReadReg32(((PHY_UINT32)&info->sifslv_fm_regs_c->fmmonr1));
	temp &= ~(0x1<<8);
	U3PhyWriteReg32(((PHY_UINT32)&info->sifslv_fm_regs_c->fmmonr1), temp);
	// => RG_USB20_HSTX_SRCAL_EN = 0
	// disable HS TX SR calibration
	temp = U3PhyReadReg32(((PHY_UINT32)&info->u2phy_regs_c->u2phyacr0));
	temp &= ~(0x1<<23);
	U3PhyWriteReg32(((PHY_UINT32)&info->u2phy_regs_c->u2phyacr0), temp);
	DRV_MDELAY(1);

	if(u4FmOut == 0){
		printk("U3portm USB20 TX slew rate calibration fail! FM_OUT value: u4FmOut = %d\n", u4FmOut);
		temp = U3PhyReadReg32(((PHY_UINT32)&info->u2phy_regs_c->u2phyacr0));
		temp &= ~(0x7<<16);
		temp |= (0x4<<16);
		U3PhyWriteReg32(((PHY_UINT32)&info->u2phy_regs_c->u2phyacr0), temp);

		fgRet = 1;
	}
	else{
		// set reg = (1024/FM_OUT) * 24 * 0.028 (round to the nearest digits)
		u4Tmp = (((1024 * REF_CK * U2_SR_COEF_C60802) / u4FmOut) + 500) / 1000;
		printk("SR calibration value u1SrCalVal = %d\n", (PHY_UINT8)u4Tmp);

        if(u4Tmp > 7){
			u4Tmp = 4;
			printk("Force u1SrCalVal = %d\n",u4Tmp);
		}
		temp = U3PhyReadReg32(((PHY_UINT32)&info->u2phy_regs_c->u2phyacr0));
		temp &= ~(0x7<<16);
		temp |= (u4Tmp<<16);
		U3PhyWriteReg32(((PHY_UINT32)&info->u2phy_regs_c->u2phyacr0), temp);
	}
	return fgRet;
}
#endif

