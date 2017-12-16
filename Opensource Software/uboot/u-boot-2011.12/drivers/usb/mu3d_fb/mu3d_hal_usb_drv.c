#include "mu3d_hal_osal.h"

#define _MTK_USB_DRV_EXT_

#include "mu3d_hal_usb_drv.h"

#undef _MTK_USB_DRV_EXT_

#include "mu3d_hal_hw.h"

#include "mu3d_hal_qmu_drv.h"

#include "mu3d_hal_comm.h"

#include "mtk-phy.h"







struct USB_REQ *mu3d_hal_get_req(DEV_INT32 ep_num, USB_DIR dir){

    DEV_INT32 ep_index=0;



    if(dir == USB_TX){

        ep_index = ep_num;

    }

    else if(dir == USB_RX){

        ep_index = ep_num + MAX_EP_NUM;

    }

    else{

        os_ASSERT(0);

    }

	

    return &g_u3d_req[ep_index];

}

/**

 * mu3d_hal_pdn_dis - disable ssusb power down & clock gated

 *

 */

void mu3d_hal_pdn_dis(void){

	os_clrmsk(U3D_SSUSB_IP_PW_CTRL2, SSUSB_IP_DEV_PDN);

	#ifdef SUPPORT_U3

	os_clrmsk(U3D_SSUSB_U3_CTRL_0P, (SSUSB_U3_PORT_DIS|SSUSB_U3_PORT_PDN|SSUSB_U3_PORT_U2_CG_EN));

	#endif

	os_clrmsk(U3D_SSUSB_U2_CTRL_0P, (SSUSB_U2_PORT_DIS|SSUSB_U2_PORT_PDN|SSUSB_U2_PORT_U2_CG_EN));

}



/**

 * mu3d_hal_ssusb_en - disable ssusb power down & enable u2/u3 ports

 *

 */

void mu3d_hal_ssusb_en(void){

	os_clrmsk(U3D_SSUSB_IP_PW_CTRL0, SSUSB_IP_SW_RST);

	os_clrmsk(U3D_SSUSB_IP_PW_CTRL2, SSUSB_IP_DEV_PDN);	

	#ifdef SUPPORT_U3	

	os_clrmsk(U3D_SSUSB_U3_CTRL_0P, (SSUSB_U3_PORT_DIS | SSUSB_U3_PORT_PDN | SSUSB_U3_PORT_HOST_SEL));

	#endif

	os_clrmsk(U3D_SSUSB_U2_CTRL_0P, (SSUSB_U2_PORT_DIS | SSUSB_U2_PORT_PDN | SSUSB_U2_PORT_HOST_SEL));



	os_setmsk(U3D_SSUSB_REF_CK_CTRL, (SSUSB_REF_MAC_CK_GATE_EN | SSUSB_REF_PHY_CK_GATE_EN | SSUSB_REF_CK_GATE_EN | SSUSB_REF_MAC3_CK_GATE_EN));



	/* check U3D sys125,u3 mac,u2 mac clock status. */

	mu3d_hal_check_clk_sts();

    /* initialize ep fifo address*/
    g_TxFIFOadd = USB_TX_FIFO_START_ADDRESS;    
    g_RxFIFOadd = USB_RX_FIFO_START_ADDRESS;    
}



/**

 * mu3d_hal_dft_reg - apply default register settings

 */

void mu3d_hal_dft_reg(void){

//	DEV_UINT32 version;





	/* set sys_ck related registers */	

	//sys_ck = OSC 125MHz/2 = 62.5MHz

	os_setmsk(U3D_SSUSB_SYS_CK_CTRL, SSUSB_SYS_CK_DIV2_EN);

	//U2 MAC sys_ck = ceil(62.5) = 63

	os_writelmsk(U3D_USB20_TIMING_PARAMETER, 63, TIME_VALUE_1US);

	#ifdef SUPPORT_U3

	//U3 MAC sys_ck = ceil(62.5) = 63

	os_writelmsk(U3D_TIMING_PULSE_CTRL, 63, CNT_1US_VALUE);

	#endif	





	/* set ref_ck related registers */	

	//U2 ref_ck = OSC 20MHz/2 = 10MHz

 	os_writelmsk(U3D_SSUSB_U2_PHY_PLL, 10, SSUSB_U2_PORT_1US_TIMER);

	//>600ns

	os_writelmsk(U3D_SSUSB_IP_PW_CTRL0, 

		(7<<SSUSB_IP_U2_ENTER_SLEEP_CNT_OFST), SSUSB_IP_U2_ENTER_SLEEP_CNT);

	#ifdef SUPPORT_U3

	//U3 ref_ck = 20MHz/2 = 10MHz

 	os_writelmsk(U3D_REF_CK_PARAMETER, 10, REF_1000NS);	

	//>=300ns

	os_writelmsk(U3D_UX_EXIT_LFPS_TIMING_PARAMETER, 

		(3<<RX_UX_EXIT_LFPS_REF_OFST), RX_UX_EXIT_LFPS_REF);

	#endif





	/* code to override HW default values, FPGA ONLY */

	

	//enable debug probe

	os_writel(U3D_SSUSB_PRB_CTRL0, 0xffff);





	//USB 2.0 related

	//response STALL to host if LPM BESL value is not in supporting range

	os_setmsk(U3D_POWER_MANAGEMENT, (LPM_BESL_STALL | LPM_BESLD_STALL));



	#if ISO_UPDATE_TEST

	#if ISO_UPDATE_MODE

	os_setmsk(U3D_POWER_MANAGEMENT, ISO_UPDATE);

	#else

	os_clrmsk(U3D_POWER_MANAGEMENT, ISO_UPDATE);

	#endif

	#endif



	#ifdef U2_U3_SWITCH_AUTO

	os_setmsk(U3D_USB2_TEST_MODE, U2U3_AUTO_SWITCH);

	#endif





	//USB 3.0 related

	#ifdef SUPPORT_U3

	//device responses to u3_exit from host automatically

	os_writel(U3D_LTSSM_CTRL, os_readl(U3D_LTSSM_CTRL) &~ SOFT_U3_EXIT_EN);



	//set TX/RX latch select for D60802A

	if (u3phy->phy_version == 0xd60802a)

		os_writel(U3D_PIPE_LATCH_SELECT, 1);



	#ifdef DIS_ZLP_CHECK_CRC32

	//disable CRC check of ZLP, for compatibility concern

	os_writel(U3D_LINK_CAPABILITY_CTRL, ZLP_CRC32_CHK_DIS);

	#endif

	#endif

}



/**

 * mu3d_hal_rst_dev - reset all device modules

 *

 */

void mu3d_hal_rst_dev(void){

//	DEV_INT32 count;

	DEV_INT32 ret;	

	DEV_UINT32 dwTmp1, dwTmp2, dwTmp3;	



	os_printk(K_ERR, "%s\n", __func__);

	

	//backup debug probe register values

	dwTmp1 = os_readl(U3D_SSUSB_PRB_CTRL1);

	dwTmp2 = os_readl(U3D_SSUSB_PRB_CTRL2);

	dwTmp3 = os_readl(U3D_SSUSB_PRB_CTRL3);



	//toggle reset bit

	os_writel(U3D_SSUSB_DEV_RST_CTRL, SSUSB_DEV_SW_RST); 

	os_writel(U3D_SSUSB_DEV_RST_CTRL, 0); 



	//restore debug probe register values

	os_writel(U3D_SSUSB_PRB_CTRL1, dwTmp1);

	os_writel(U3D_SSUSB_PRB_CTRL2, dwTmp2);

	os_writel(U3D_SSUSB_PRB_CTRL3, dwTmp3);



	//do not check when SSUSB_U2_PORT_DIS = 1, because U2 port stays in reset state

	if (!(os_readl(U3D_SSUSB_U2_CTRL_0P) & SSUSB_U2_PORT_DIS))

	{

		ret = wait_for_value(U3D_SSUSB_IP_PW_STS2, SSUSB_U2_MAC_SYS_RST_B_STS, SSUSB_U2_MAC_SYS_RST_B_STS, 1, 10);

		if (ret == RET_FAIL)

	        os_printk(K_ERR, "SSUSB_U2_MAC_SYS_RST_B_STS NG\n");

	}



	#ifdef SUPPORT_U3

	//do not check when SSUSB_U3_PORT_PDN = 1, because U3 port stays in reset state

	if (!(os_readl(U3D_SSUSB_U3_CTRL_0P) & SSUSB_U3_PORT_PDN))

	{

		ret = wait_for_value(U3D_SSUSB_IP_PW_STS1, SSUSB_U3_MAC_RST_B_STS, SSUSB_U3_MAC_RST_B_STS, 1, 10);

		if (ret == RET_FAIL)

	        os_printk(K_ERR, "SSUSB_U3_MAC_RST_B_STS NG\n");

	}

	#endif



	ret = wait_for_value(U3D_SSUSB_IP_PW_STS1, SSUSB_DEV_QMU_RST_B_STS, SSUSB_DEV_QMU_RST_B_STS, 1, 10);

	if (ret == RET_FAIL)

        os_printk(K_ERR, "SSUSB_DEV_QMU_RST_B_STS NG\n");



	ret = wait_for_value(U3D_SSUSB_IP_PW_STS1, SSUSB_DEV_BMU_RST_B_STS, SSUSB_DEV_BMU_RST_B_STS, 1, 10);

	if (ret == RET_FAIL)

        os_printk(K_ERR, "SSUSB_DEV_BMU_RST_B_STS NG\n");



	ret = wait_for_value(U3D_SSUSB_IP_PW_STS1, SSUSB_DEV_RST_B_STS, SSUSB_DEV_RST_B_STS, 1, 10);

	if (ret == RET_FAIL)

        os_printk(K_ERR, "SSUSB_DEV_RST_B_STS NG\n");

}



/**

 * mu3d_hal_check_clk_sts - check sys125,u3 mac,u2 mac clock status

 *

 */

DEV_INT32 mu3d_hal_check_clk_sts(void){

//	DEV_INT32 count;

	DEV_INT32 ret;	



	os_printk(K_ERR,"mu3d_hal_check_clk_sts\n");



	ret = wait_for_value(U3D_SSUSB_IP_PW_STS1, SSUSB_SYS125_RST_B_STS, SSUSB_SYS125_RST_B_STS, 1, 10);

	if (ret == RET_FAIL)

	{

        os_printk(K_ERR, "SSUSB_SYS125_RST_B_STS NG\n");

        return RET_FAIL;

	}

	

	#ifdef SUPPORT_U3
	ret = wait_for_value(U3D_SSUSB_IP_PW_STS1, SSUSB_U3_MAC_RST_B_STS, SSUSB_U3_MAC_RST_B_STS, 1, 10);

	if (ret == RET_FAIL)

	{

        os_printk(K_ERR, "SSUSB_U3_MAC_RST_B_STS NG\n");

        return RET_FAIL;

	}
    #endif


	ret = wait_for_value(U3D_SSUSB_IP_PW_STS2, SSUSB_U2_MAC_SYS_RST_B_STS, SSUSB_U2_MAC_SYS_RST_B_STS, 1, 10);

	if (ret == RET_FAIL)

	{

        os_printk(K_ERR, "SSUSB_U2_MAC_SYS_RST_B_STS NG\n");

        return RET_FAIL;

	}



	os_printk(K_CRIT, "check clk pass!!\n");

	return RET_SUCCESS;

}



/**

 * mu3d_hal_link_up - u3d link up

 *

 */

DEV_INT32 mu3d_hal_link_up(DEV_INT32 latch_val){



	mu3d_hal_ssusb_en();

	mu3d_hal_pdn_dis();

	mu3d_hal_rst_dev();

	os_ms_delay(50);

	os_writel(U3D_USB3_CONFIG, USB3_EN);

	os_writel(U3D_PIPE_LATCH_SELECT, latch_val);//set tx/rx latch sel



	return 0;

}





/**

 * mu3d_hal_initr_dis - disable all interrupts

 *

 */

void mu3d_hal_initr_dis(void){

	/* disable interrupts */

	os_writel(U3D_EPISR, 0xFFFFFFFF);

	os_writel(U3D_DMAISR, 0xFFFFFFFF);

	os_writel(U3D_QISAR0, 0xFFFFFFFF);

	os_writel(U3D_QISAR1, 0xFFFFFFFF);

	os_writel(U3D_QEMIR, 0xFFFFFFFF);

	os_writel(U3D_TQERRIR0, 0xFFFFFFFF);

	os_writel(U3D_RQERRIR0, 0xFFFFFFFF);

	os_writel(U3D_RQERRIR1, 0xFFFFFFFF);

	os_writel(U3D_LV1IECR, 0xFFFFFFFF);

	os_writel(U3D_EPIECR, 0xFFFFFFFF);

	os_writel(U3D_DMAIECR, 0xFFFFFFFF);

	os_writel(U3D_QIECR0, 0xFFFFFFFF);

	os_writel(U3D_QIECR1, 0xFFFFFFFF);

	os_writel(U3D_QEMIECR, 0xFFFFFFFF);

	os_writel(U3D_TQERRIECR0, 0xFFFFFFFF);

	os_writel(U3D_RQERRIECR0, 0xFFFFFFFF);

	os_writel(U3D_RQERRIECR1, 0xFFFFFFFF);

}

/**

 * mu3d_hal_system_intr_en - enable system global interrupt

 *

 */

void mu3d_hal_system_intr_en(void){



	DEV_UINT16 int_en;

//	DEV_UINT32 ltssm_int_en;



	os_printk(K_ERR, "%s\n", __func__);

	

	os_writel(U3D_EPIECR, os_readl(U3D_EPIER));

	os_writel(U3D_DMAIECR, os_readl(U3D_DMAIER));



	//clear and enable common USB interrupts	

	os_writel(U3D_COMMON_USB_INTR_ENABLE, 0x00);

	os_writel(U3D_COMMON_USB_INTR, os_readl(U3D_COMMON_USB_INTR));

	int_en = SUSPEND_INTR_EN|RESUME_INTR_EN|RESET_INTR_EN|CONN_INTR_EN|DISCONN_INTR_EN \

			|VBUSERR_INTR_EN|LPM_INTR_EN|LPM_RESUME_INTR_EN;

	os_writel(U3D_COMMON_USB_INTR_ENABLE, int_en);



	#ifdef SUPPORT_U3

	//clear and enable LTSSM interrupts

	os_writel(U3D_LTSSM_INTR_ENABLE, 0x00);

	os_printk(K_ERR, "U3D_LTSSM_INTR: %x\n", os_readl(U3D_LTSSM_INTR));

	os_writel(U3D_LTSSM_INTR, os_readl(U3D_LTSSM_INTR));

	ltssm_int_en = SS_INACTIVE_INTR_EN|SS_DISABLE_INTR_EN|COMPLIANCE_INTR_EN|LOOPBACK_INTR_EN \

		     |HOT_RST_INTR_EN|WARM_RST_INTR_EN|RECOVERY_INTR_EN|ENTER_U0_INTR_EN|ENTER_U1_INTR_EN \

		     |ENTER_U2_INTR_EN|ENTER_U3_INTR_EN|EXIT_U1_INTR_EN|EXIT_U2_INTR_EN|EXIT_U3_INTR_EN \

		     |RXDET_SUCCESS_INTR_EN|VBUS_RISE_INTR_EN|VBUS_FALL_INTR_EN|U3_LFPS_TMOUT_INTR_EN|U3_RESUME_INTR_EN;

	os_writel(U3D_LTSSM_INTR_ENABLE, ltssm_int_en);

	#endif

	

	os_writel(U3D_DMAIESR, EP0DMAIESR); 

	os_writel(U3D_DEV_LINK_INTR_ENABLE, SSUSB_DEV_SPEED_CHG_INTR_EN); 

	

	return;

}



/**

 * mu3d_hal_resume - power mode resume

 *

 */

void mu3d_hal_resume(void){

#ifdef POWER_SAVING_MODE

	os_writel(U3D_SSUSB_IP_PW_CTRL2, os_readl(U3D_SSUSB_IP_PW_CTRL2) & ~SSUSB_IP_DEV_PDN);

#endif

	

	#ifdef SUPPORT_U3

	if(os_readl(U3D_DEVICE_CONF) & HW_USB2_3_SEL){ //SS

#ifdef POWER_SAVING_MODE

		os_writel(U3D_SSUSB_U3_CTRL_0P, os_readl(U3D_SSUSB_U3_CTRL_0P) & ~SSUSB_U3_PORT_PDN);

		while(!(os_readl(U3D_SSUSB_IP_PW_STS1) & SSUSB_U3_MAC_RST_B_STS));

#endif

		os_writel(U3D_LINK_POWER_CONTROL, os_readl(U3D_LINK_POWER_CONTROL) | UX_EXIT);

	}

	else

	#endif		

	{ //hs fs

#ifdef POWER_SAVING_MODE

		os_writel(U3D_SSUSB_U2_CTRL_0P, os_readl(U3D_SSUSB_U2_CTRL_0P) & ~SSUSB_U2_PORT_PDN);

		while(!(os_readl(U3D_SSUSB_IP_PW_STS2) & SSUSB_U2_MAC_SYS_RST_B_STS));

#endif

		

		os_writel(U3D_POWER_MANAGEMENT, os_readl(U3D_POWER_MANAGEMENT) | RESUME);

    	os_ms_delay(10);  

    	os_writel(U3D_POWER_MANAGEMENT, os_readl(U3D_POWER_MANAGEMENT) & ~RESUME);

	}

}

/**

 * mu3d_hal_u2dev_connect - u2 device softconnect

 *

 */

void mu3d_hal_u2dev_connect(){

	os_writel(U3D_POWER_MANAGEMENT, os_readl(U3D_POWER_MANAGEMENT) | SOFT_CONN);

	os_writel(U3D_POWER_MANAGEMENT, os_readl(U3D_POWER_MANAGEMENT) | SUSPENDM_ENABLE);

	os_printk(K_ERR, "SOFTCONN = 1\n");

}



	

/**

 * mu3d_hal_u3dev_en - enable U3D ss dev

 *

 */

void mu3d_hal_u3dev_en(void){

	os_writel(U3D_USB3_CONFIG, USB3_EN);

}



void mu3d_hal_u2dev_disconn(){

   os_writel(U3D_POWER_MANAGEMENT, os_readl(U3D_POWER_MANAGEMENT) & ~SOFT_CONN);

   os_writel(U3D_POWER_MANAGEMENT, os_readl(U3D_POWER_MANAGEMENT) & ~SUSPENDM_ENABLE);

}



/**

 * mu3d_hal_set_speed - enable ss or connect to hs/fs

 *@args - arg1: speed

 */

void mu3d_hal_set_speed(USB_SPEED speed){

 	#ifndef EXT_VBUS_DET

 	os_writel(U3D_MISC_CTRL, 0);

	#else

 	os_writel(U3D_MISC_CTRL, 0x3);	

 	#endif

 	

	/* clear ltssm state*/

    if(speed == SSUSB_SPEED_FULL){

        os_writel(U3D_POWER_MANAGEMENT, os_readl(U3D_POWER_MANAGEMENT) & ~HS_ENABLE);

		mu3d_hal_u2dev_connect();

        g_u3d_setting.speed = SSUSB_SPEED_FULL;

    }

    else if(speed == SSUSB_SPEED_HIGH){

        os_writel(U3D_POWER_MANAGEMENT, os_readl(U3D_POWER_MANAGEMENT) | HS_ENABLE);

		mu3d_hal_u2dev_connect();

        g_u3d_setting.speed = SSUSB_SPEED_HIGH;

    }

	#ifdef SUPPORT_U3

	else if(speed == SSUSB_SPEED_SUPER){

        g_u3d_setting.speed = SSUSB_SPEED_SUPER;

		mu3d_hal_u2dev_disconn();

		mu3d_hal_u3dev_en();

    }

	#endif

    else{

        os_printk(K_ALET,"Unsupported speed!!\n");

        os_ASSERT(0);

    }

}



/**

 * mu3d_hal_pdn_cg_en - enable U2/U3 pdn & cg

 *@args - 

 */

void mu3d_hal_pdn_cg_en(void){

	DEV_UINT8 speed = (os_readl(U3D_DEVICE_CONF) & SSUSB_DEV_SPEED);

	

	switch (speed)

	{

		case SSUSB_SPEED_SUPER:

			os_printk(K_EMERG, "Device: SUPER SPEED LOW POWER\n");

	        os_setmsk(U3D_SSUSB_U2_CTRL_0P, SSUSB_U2_PORT_DIS);

			break;			

		case SSUSB_SPEED_HIGH:

			os_printk(K_EMERG, "Device: HIGH SPEED LOW POWER\n");

			#ifdef SUPPORT_U3

 			os_setmsk(U3D_SSUSB_U3_CTRL_0P, SSUSB_U3_PORT_PDN);

			#endif

			break;

		case SSUSB_SPEED_FULL:

			os_printk(K_EMERG, "Device: FULL SPEED LOW POWER\n");

			#ifdef SUPPORT_U3			

			os_setmsk(U3D_SSUSB_U3_CTRL_0P, SSUSB_U3_PORT_PDN);

			#endif

			break;			

	}			

}



/**

 * mu3d_hal_det_speed - detect device speed

 *@args - arg1: speed

 */

void mu3d_hal_det_speed(USB_SPEED speed, DEV_UINT8 det_speed){

	DEV_UINT8 temp;


	#ifdef EXT_VBUS_DET
	DEV_UINT16 cnt_down = 50;

	if (!det_speed)

	{

		while (!(os_readl(U3D_DEV_LINK_INTR) & SSUSB_DEV_SPEED_CHG_INTR))

		{

			os_ms_delay(1);

			cnt_down--;



			if (cnt_down == 0)

				return;

		}

	}

	else

	{

		while(!(os_readl(U3D_DEV_LINK_INTR) & SSUSB_DEV_SPEED_CHG_INTR));	

	}

	#else

	while(!(os_readl(U3D_DEV_LINK_INTR) & SSUSB_DEV_SPEED_CHG_INTR));

	#endif

	os_writel(U3D_DEV_LINK_INTR, SSUSB_DEV_SPEED_CHG_INTR);



	temp = (os_readl(U3D_DEVICE_CONF) & SSUSB_DEV_SPEED);

	switch (temp)

	{

		case SSUSB_SPEED_SUPER:

			os_printk(K_EMERG, "Device: SUPER SPEED\n");			

			break;

		case SSUSB_SPEED_HIGH:

			os_printk(K_EMERG, "Device: HIGH SPEED\n");			

			break;			

		case SSUSB_SPEED_FULL:

			os_printk(K_EMERG, "Device: FULL SPEED\n");			

			break;						

		case SSUSB_SPEED_INACTIVE:

			os_printk(K_EMERG, "Device: INACTIVE\n");

			break;

	}



	if (temp != speed)

	{

		os_printk(K_EMERG, "desired speed: %d, detected speed: %d\n", speed, temp);

		while(1);

	}

}



/**

 * mu3d_hal_write_fifo - pio write one packet

 *@args - arg1: ep number, arg2: data length,  arg3: data buffer, arg4: max packet size

 */

DEV_INT32 mu3d_hal_write_fifo(DEV_INT32 ep_num,DEV_INT32 length,DEV_UINT8 *buf,DEV_INT32 maxp){

	

    DEV_UINT32 residue, count;

	DEV_UINT32 temp,unit;



	os_printk(K_DEBUG,"mu3d_hal_write_fifo\n");

	os_printk(K_DEBUG,"ep_num : %d, length : %d,  buf : 0x%x, maxp : %d\n",ep_num,length,(unsigned int)buf,maxp);

	count = residue = length;



    while(residue > 0){



		if(residue==1){

			temp=((*buf)&0xFF);

			os_writel(U3D_RISC_SIZE, RISC_SIZE_1B);

			unit=1;

		}

		else if(residue==2){

			temp=((*buf)&0xFF)+(((*(buf+1))<<8)&0xFF00);

			os_writel(U3D_RISC_SIZE, RISC_SIZE_2B);

			unit=2;

		}

		else if(residue==3){

			temp=((*buf)&0xFF)+(((*(buf+1))<<8)&0xFF00);

			os_writel(U3D_RISC_SIZE, RISC_SIZE_2B);

			unit=2;

		}

		else{

			temp=((*buf)&0xFF)+(((*(buf+1))<<8)&0xFF00)+(((*(buf+2))<<16)&0xFF0000)+(((*(buf+3))<<24)&0xFF000000);

			unit=4;

		}

		os_writel(USB_FIFO(ep_num), temp);

        buf=buf+unit;

		residue-=unit;



    }

	if(os_readl(U3D_RISC_SIZE)!=RISC_SIZE_4B){

		os_writel(U3D_RISC_SIZE, RISC_SIZE_4B);

	}



	if(ep_num==0){

		if(count==0){

			

        	os_writel(U3D_EP0CSR, os_readl(U3D_EP0CSR) | EP0_DATAEND);

			os_printk(K_DEBUG,"USB_EP0_DATAEND\n");

			return 0;

		}

		else{



#ifdef AUTOSET

				if(count<maxp){

					os_writel(U3D_EP0CSR, os_readl(U3D_EP0CSR) | EP0_TXPKTRDY);	

					os_printk(K_DEBUG,"EP0_TXPKTRDY\n");

					//printf("EP0_TXPKTRDY\n");

				}

#else				

			os_writel(U3D_EP0CSR, os_readl(U3D_EP0CSR)|EP0_TXPKTRDY);	

#endif

		}

	}else{



		if(count == 0){

			USB_WriteCsr32(U3D_TX1CSR0, ep_num, USB_ReadCsr32(U3D_TX1CSR0, ep_num) | TX_TXPKTRDY);

			return count;

		}

		else{

	

#ifdef AUTOSET

			if(count < maxp){

				USB_WriteCsr32(U3D_TX1CSR0, ep_num, USB_ReadCsr32(U3D_TX1CSR0, ep_num) | TX_TXPKTRDY);

				os_printk(K_DEBUG,"short packet\n");

				return count;

			}

#else

			USB_WriteCsr32(U3D_TX1CSR0, ep_num, USB_ReadCsr32(U3D_TX1CSR0, ep_num) | TX_TXPKTRDY);

#endif

		 }

	}

	return count;

}



/**

 * mu3d_hal_read_fifo - pio read one packet

 *@args - arg1: ep number,  arg2: data buffer

 */

DEV_INT32 mu3d_hal_read_fifo(DEV_INT32 ep_num,DEV_UINT8 *buf){

    DEV_UINT16 count, residue;

	DEV_UINT32 temp;

 	DEV_UINT8 *bp = buf ;

 

	os_printk(K_DEBUG,"mu3d_hal_read_fifo\n");

 	os_printk(K_DEBUG,"req->buf :%x\n", (DEV_UINT32) buf);



	if(ep_num==0){

		residue = count = os_readl(U3D_RXCOUNT0);

	}else{

		residue = count = (USB_ReadCsr32(U3D_RX1CSR3, ep_num)>>16);

	}

    while(residue > 0){

    

		temp= os_readl(USB_FIFO(ep_num));

		

        *bp = temp&0xFF;

		*(bp+1) = (temp>>8)&0xFF;

		*(bp+2) = (temp>>16)&0xFF;

		*(bp+3) = (temp>>24)&0xFF;

        bp=bp+4;

        if(residue>4){

        	residue=residue-4;

       	}

		else{

			residue=0;

		}

	}

	

#ifdef AUTOCLEAR

	if(ep_num==0){

		if(!count){

			os_writel(U3D_EP0CSR, os_readl(U3D_EP0CSR)|EP0_RXPKTRDY);

		}

	}

	else{

		if(!count){

			USB_WriteCsr32(U3D_RX1CSR0, ep_num, USB_ReadCsr32(U3D_RX1CSR0, ep_num) | RX_RXPKTRDY);

			os_printk(K_ALET,"zlp\n");

		}

	}

#else		

	if(ep_num==0){

		os_writel(U3D_EP0CSR, os_readl(U3D_EP0CSR)|EP0_RXPKTRDY);

	}

	else{

		USB_WriteCsr32(U3D_RX1CSR0, ep_num, USB_ReadCsr32(U3D_RX1CSR0, ep_num) | RX_RXPKTRDY);

	}

#endif

    return count;

}



/**

 * mu3d_hal_write_fifo_burst - pio write n packets with polling buffer full (epn only)

 *@args - arg1: ep number, arg2: u3d req

 */

DEV_INT32 mu3d_hal_write_fifo_burst(DEV_INT32 ep_num,DEV_INT32 length,DEV_UINT8 *buf,DEV_INT32 maxp){

	

    DEV_UINT32 residue, count, actual;

	DEV_UINT32 temp,unit;

    DEV_UINT8 *bp ;



	os_printk(K_DEBUG,"mu3d_hal_write_fifo_burst\n");

	os_printk(K_DEBUG,"ep_num : %d, lenght : %d,  buf : 0x%x, maxp : %d\n",ep_num,length,(unsigned int)buf,maxp);

		

	actual = 0;

	

#ifdef AUTOSET

 	while(!(USB_ReadCsr32(U3D_TX1CSR0, ep_num) & TX_FIFOFULL)){

#endif		



    	if(length - actual > maxp){

        	count = residue = maxp;

    	}

    	else{

        	count = residue = length - actual;

    	}

		

		bp = buf + actual;

		

    	while(residue > 0){



			if(residue==1){

				temp=((*bp)&0xFF);

				os_writel(U3D_RISC_SIZE, RISC_SIZE_1B);

				unit=1;

			}

			else if(residue==2){

				temp=((*bp)&0xFF)+(((*(bp+1))<<8)&0xFF00);

				os_writel(U3D_RISC_SIZE, RISC_SIZE_2B);

				unit=2;

			}

			else if(residue==3){

				temp=((*bp)&0xFF)+(((*(bp+1))<<8)&0xFF00);

				os_writel(U3D_RISC_SIZE, RISC_SIZE_2B);

				unit=2;

			}

			else{

				temp=((*bp)&0xFF)+(((*(bp+1))<<8)&0xFF00)+(((*(bp+2))<<16)&0xFF0000)+(((*(bp+3))<<24)&0xFF000000);

				unit=4;

			}

			os_writel(USB_FIFO(ep_num), temp);

        	bp=bp+unit;

			residue-=unit;

    	}

		if(os_readl(U3D_RISC_SIZE)!=RISC_SIZE_4B){

			os_writel(U3D_RISC_SIZE, RISC_SIZE_4B);

		}

    	actual += count;

		

		if(length == 0){

			USB_WriteCsr32(U3D_TX1CSR0, ep_num, USB_ReadCsr32(U3D_TX1CSR0, ep_num) | TX_TXPKTRDY);

			return actual;

		}

		else{

	

#ifdef AUTOSET

			if((count < maxp) && (count > 0)){

				USB_WriteCsr32(U3D_TX1CSR0, ep_num, USB_ReadCsr32(U3D_TX1CSR0, ep_num) | TX_TXPKTRDY);

				os_printk(K_DEBUG,"short packet\n");

				return actual;

			}

			if(count == 0){

				return actual;

			}

#else

			USB_WriteCsr32(U3D_TX1CSR0, ep_num, USB_ReadCsr32(U3D_TX1CSR0, ep_num) | TX_TXPKTRDY);

#endif

		 }



#ifdef AUTOSET

	}

#endif

	return actual;

}



/**

 * mu3d_hal_read_fifo_burst - pio read n packets with polling buffer empty (epn only)

 *@args - arg1: ep number, arg2: data buffer

 */

DEV_INT32 mu3d_hal_read_fifo_burst(DEV_INT32 ep_num,DEV_UINT8 *buf){

	

    DEV_UINT16 count, residue;

	DEV_UINT32 temp, actual;

 	DEV_UINT8 *bp;

 

	os_printk(K_INFO,"mu3d_hal_read_fifo_burst\n");

 	os_printk(K_ALET,"req->buf :%x\n", (DEV_UINT32)buf);

	actual = 0;

#ifdef AUTOCLEAR

	while(!(USB_ReadCsr32(U3D_RX1CSR0, ep_num) & RX_FIFOEMPTY)){

#endif	

		residue = count = (USB_ReadCsr32(U3D_RX1CSR3, ep_num)>>16);

 		os_printk(K_INFO,"count :%d ; req->actual :%d \n",count,actual);

		bp = buf + actual;

		

    	while(residue > 0){

			temp= os_readl(USB_FIFO(ep_num));

       		*bp = temp&0xFF;

			*(bp+1) = (temp>>8)&0xFF;

			*(bp+2) = (temp>>16)&0xFF;

			*(bp+3) = (temp>>24)&0xFF;

        	bp=bp+4;

        	if(residue>4){

        		residue=residue-4;

       		}

			else{

				residue=0;

			}

		}

    	actual += count;



#ifdef AUTOCLEAR

		if(!count){

			USB_WriteCsr32(U3D_RX1CSR0, ep_num, USB_ReadCsr32(U3D_RX1CSR0, ep_num) | RX_RXPKTRDY);

			os_printk(K_ALET,"zlp\n");

			os_printk(K_ALET,"actual :%d\n",actual);

			return actual;

		}

#else		

		if(ep_num==0){

			os_writel(U3D_EP0CSR, os_readl(U3D_EP0CSR)|EP0_RXPKTRDY);

		}

		else{

			USB_WriteCsr32(U3D_RX1CSR0, ep_num, USB_ReadCsr32(U3D_RX1CSR0, ep_num) | RX_RXPKTRDY);

		}

#endif

#ifdef AUTOCLEAR

	}

#endif



	return actual;

}





/**

* mu3d_hal_unfigured_ep - 

 *@args - 

 */

 

void mu3d_hal_unfigured_ep(void){ 

    DEV_UINT32 i, tx_ep_num, rx_ep_num;

	struct USB_EP_SETTING *ep_setting;



	g_TxFIFOadd = USB_TX_FIFO_START_ADDRESS;	 

	g_RxFIFOadd = USB_RX_FIFO_START_ADDRESS;	

	tx_ep_num = os_readl(U3D_CAP_EPINFO) & CAP_TX_EP_NUM;

	rx_ep_num = (os_readl(U3D_CAP_EPINFO) & CAP_RX_EP_NUM) >> 8;



	for(i=1; i<=tx_ep_num; i++){



		USB_WriteCsr32(U3D_TX1CSR0, i, USB_ReadCsr32(U3D_TX1CSR0, i) & (~0x7FF));

		USB_WriteCsr32(U3D_TX1CSR1, i, 0);		

  		USB_WriteCsr32(U3D_TX1CSR2, i, 0);

		ep_setting = &g_u3d_setting.ep_setting[i];

		ep_setting->fifoaddr = 0;

		ep_setting->enabled = 0;

	}

	

	for(i=1; i<=rx_ep_num; i++){

		USB_WriteCsr32(U3D_RX1CSR0, i, USB_ReadCsr32(U3D_RX1CSR0, i) & (~0x7FF));

		USB_WriteCsr32(U3D_RX1CSR1, i, 0);		

		USB_WriteCsr32(U3D_RX1CSR2, i, 0);

		ep_setting = &g_u3d_setting.ep_setting[i+MAX_EP_NUM];

		ep_setting->fifoaddr = 0;

		ep_setting->enabled = 0;

	}

}

/**

* mu3d_hal_ep_enable - configure ep 

*@args - arg1: ep number, arg2: dir, arg3: transfer type, arg4: max packet size, arg5: interval, arg6: slot, arg7: burst, arg8: mult

*/

void mu3d_hal_ep_enable(DEV_UINT8 ep_num, USB_DIR dir, TRANSFER_TYPE type, DEV_INT32 maxp, DEV_INT8 interval, DEV_INT8 slot,DEV_INT8 burst, DEV_INT8 mult){ 

	

	DEV_INT32 ep_index=0;

	DEV_INT32 used_before;

	DEV_UINT8 fifosz=0, max_pkt, binterval;

	DEV_INT32 csr0, csr1, csr2;

	struct USB_EP_SETTING *ep_setting;

	

	if(type==USB_CTRL){

	

		ep_setting = &g_u3d_setting.ep_setting[0];

		ep_setting->fifosz = maxp;

		ep_setting->maxp = maxp;

		csr0 = os_readl(U3D_EP0CSR) & EP0_W1C_BITS;

		csr0 |= maxp;

		os_writel(U3D_EP0CSR, csr0);

		

		os_setmsk(U3D_USB2_RX_EP_DATAERR_INTR, BIT16); //EP0 data error interrupt

		return;

	}

	

	if(dir == USB_TX){

		ep_index = ep_num;

	}

	else if(dir == USB_RX){

		ep_index = ep_num + MAX_EP_NUM;

	}

	else{

		os_ASSERT(0);

	}

	ep_setting = &g_u3d_setting.ep_setting[ep_index];

	used_before = ep_setting->fifoaddr;

	if(ep_setting->enabled)

		return;



	binterval = interval;

	if(dir == USB_TX){

		if((g_TxFIFOadd + maxp * (slot + 1) > os_readl(U3D_CAP_EPNTXFFSZ)) && (!used_before)){

			os_printk(K_ALET,"mu3d_hal_ep_enable, FAILED: sram exhausted\n");

			os_printk(K_ALET,"g_FIFOadd :%x\n",g_TxFIFOadd );

			os_printk(K_ALET,"maxp :%d\n",maxp );

			os_printk(K_ALET,"mult :%d\n",slot );

			os_ASSERT(0);

		}

	}

	else{

		if((g_RxFIFOadd + maxp * (slot + 1) > os_readl(U3D_CAP_EPNRXFFSZ)) && (!used_before)){

			os_printk(K_ALET,"mu3d_hal_ep_enable, FAILED: sram exhausted\n");

			os_printk(K_ALET,"g_FIFOadd :%x\n",g_RxFIFOadd );

			os_printk(K_ALET,"maxp :%d\n",maxp );

			os_printk(K_ALET,"mult :%d\n",slot );

			os_ASSERT(0);

		}

	}

		

	ep_setting->transfer_type = type;

	

	if(dir == USB_TX){

		if(!ep_setting->fifoaddr){

			ep_setting->fifoaddr = g_TxFIFOadd;    

		}

	}

	else{

		if(!ep_setting->fifoaddr){

			ep_setting->fifoaddr = g_RxFIFOadd;    

		}

	}

	

	ep_setting->fifosz = maxp;

	ep_setting->maxp = maxp;		

	ep_setting->dir = dir;

	ep_setting->enabled = 1;

	

/*
	switch(maxp){

		case 8:

			fifosz = USB_FIFOSZ_SIZE_16;

			break;

		case 16:

			fifosz = USB_FIFOSZ_SIZE_16;

			break;

		case 32:

			fifosz = USB_FIFOSZ_SIZE_32;

			break;

		case 64:

			fifosz = USB_FIFOSZ_SIZE_64;

			break;

		case 128:

			fifosz = USB_FIFOSZ_SIZE_128;

			break;

		case 256:

			fifosz = USB_FIFOSZ_SIZE_256;

			break;

		case 512:

			fifosz = USB_FIFOSZ_SIZE_512;

			break;

		case 1023:

		case 1024:

			fifosz = USB_FIFOSZ_SIZE_1024;

			break;

		case 2048:

			fifosz = USB_FIFOSZ_SIZE_1024;

			break;

		case 3072:

		case 4096:

			fifosz = USB_FIFOSZ_SIZE_1024;

			break;

		case 8192:

			fifosz = USB_FIFOSZ_SIZE_1024;

			break;

		case 16384:

			fifosz = USB_FIFOSZ_SIZE_1024;

			break;

		case 32768:

			fifosz = USB_FIFOSZ_SIZE_1024;

			break;

		

		}
*/
    if(maxp <= 16){
       fifosz = USB_FIFOSZ_SIZE_16;
        }
    else if(maxp <= 32){
        fifosz = USB_FIFOSZ_SIZE_32;
        }
    else if(maxp <= 64){
        fifosz = USB_FIFOSZ_SIZE_64;
        }
    else if(maxp <= 128){
        fifosz = USB_FIFOSZ_SIZE_128;
        }
    else if(maxp <= 256){
        fifosz = USB_FIFOSZ_SIZE_256;
        }
    else if(maxp <= 512){
        fifosz = USB_FIFOSZ_SIZE_512;
        }
    else{
        fifosz = USB_FIFOSZ_SIZE_1024;
        }
    

	if(dir == USB_TX){

		//CSR0

		csr0 = USB_ReadCsr32(U3D_TX1CSR0, ep_num) &~ TX_TXMAXPKTSZ;

		csr0 |= (maxp & TX_TXMAXPKTSZ);		

#if (BUS_MODE==PIO_MODE)	

#ifdef AUTOSET

		csr0 |= TX_AUTOSET;

#endif	

		csr0 &= ~TX_DMAREQEN;

#endif



		//CSR1

		max_pkt = (burst+1)*(mult+1)-1;

		csr1 = (burst & SS_TX_BURST);

		csr1 |= (slot<<TX_SLOT_OFST) & TX_SLOT;

		csr1 |= (max_pkt<<TX_MAX_PKT_OFST) & TX_MAX_PKT;

		csr1 |= (mult<<TX_MULT_OFST) & TX_MULT;

	

		//CSR2

		csr2 = (g_TxFIFOadd>>4) & TXFIFOADDR;

		csr2 |= (fifosz<<TXFIFOSEGSIZE_OFST) & TXFIFOSEGSIZE;

	

		if(type==USB_BULK){

			csr1 |= TYPE_BULK;

		}

		else if(type==USB_INTR){

			csr1 |= TYPE_INT;

			csr2 |= (binterval<<TXBINTERVAL_OFST)&TXBINTERVAL;

		}

		else if(type==USB_ISO){

			csr1 |= TYPE_ISO;

			csr2 |= (binterval<<TXBINTERVAL_OFST)&TXBINTERVAL;

		}

		os_writel(U3D_EPIECR, os_readl(U3D_EPIECR)|(BIT0<<ep_num));

		USB_WriteCsr32(U3D_TX1CSR0, ep_num, csr0);

		USB_WriteCsr32(U3D_TX1CSR1, ep_num, csr1);		

		USB_WriteCsr32(U3D_TX1CSR2, ep_num, csr2);

	

		os_printk(K_INFO,"[CSR]U3D_TX1CSR0 :%x\n",USB_ReadCsr32(U3D_TX1CSR0, ep_num));

		os_printk(K_INFO,"[CSR]U3D_TX1CSR1 :%x\n",USB_ReadCsr32(U3D_TX1CSR1, ep_num));

		os_printk(K_INFO,"[CSR]U3D_TX1CSR2 :%x\n",USB_ReadCsr32(U3D_TX1CSR2, ep_num));



	}

	else if(dir == USB_RX){

		//CSR0

		csr0 = USB_ReadCsr32(U3D_RX1CSR0, ep_num) &~ RX_RXMAXPKTSZ;

		csr0 |= (maxp & RX_RXMAXPKTSZ);

#if (BUS_MODE==PIO_MODE)	

#ifdef AUTOCLEAR

		csr0 |= RX_AUTOCLEAR;

#endif	

		csr0 &= ~RX_DMAREQEN;

#endif



		//CSR1

		max_pkt = (burst+1)*(mult+1)-1;

		csr1 = (burst & SS_RX_BURST);

		csr1 |= (slot<<RX_SLOT_OFST) & RX_SLOT;

		csr1 |= (max_pkt<<RX_MAX_PKT_OFST) & RX_MAX_PKT;

		csr1 |= (mult<<RX_MULT_OFST) & RX_MULT;

	

		//CSR2

		csr2 = (g_RxFIFOadd>>4) & RXFIFOADDR;

		csr2 |= (fifosz<<RXFIFOSEGSIZE_OFST) & RXFIFOSEGSIZE;

			

		if(type==USB_BULK){

			csr1 |= TYPE_BULK;

		}

		else if(type==USB_INTR){

			csr1 |= TYPE_INT;

			csr2 |= (binterval<<RXBINTERVAL_OFST)&RXBINTERVAL;

		}

		else if(type==USB_ISO){

			csr1 |= TYPE_ISO;

			csr2 |= (binterval<<RXBINTERVAL_OFST)&RXBINTERVAL;

		}

	

		os_writel(U3D_EPIECR, os_readl(U3D_EPIECR)|(BIT16<<ep_num));

		USB_WriteCsr32(U3D_RX1CSR0, ep_num, csr0);

		USB_WriteCsr32(U3D_RX1CSR1, ep_num, csr1);		

		USB_WriteCsr32(U3D_RX1CSR2, ep_num, csr2);

	

		os_printk(K_INFO,"[CSR]U3D_RX1CSR0 :%x\n",USB_ReadCsr32(U3D_RX1CSR0, ep_num));

		os_printk(K_INFO,"[CSR]U3D_RX1CSR1 :%x\n",USB_ReadCsr32(U3D_RX1CSR1, ep_num));

		os_printk(K_INFO,"[CSR]U3D_RX1CSR2 :%x\n",USB_ReadCsr32(U3D_RX1CSR2, ep_num));



		os_setmsk(U3D_USB2_RX_EP_DATAERR_INTR, BIT16<<ep_num); //EPn data error interrupt		

	}

	else{

		printf("none\n");


		os_ASSERT(0);
	}

		

	if(dir == USB_TX){

		if(maxp == 1023){

			g_TxFIFOadd += (1024 * (slot + 1));

		}

		else{

			g_TxFIFOadd += (maxp * (slot + 1));

		}

	}

	else{

		if(maxp == 1023){

			g_RxFIFOadd += (1024 * (slot + 1));

		}

		else{

			g_RxFIFOadd += (maxp * (slot + 1));

		}

	}

	

	return;

}




