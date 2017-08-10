/*-----------------------------------------------------------------------------
 * drivers/usb/musb_qmu/mtk_cust_bd.c
 *
 * MT85xx USB driver
 *
 * Copyright (c) 2008-2014 MediaTek Inc.
 * $Author: dtvbm11 $
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 *
 *---------------------------------------------------------------------------*/


/** @file mtk_cust_bd.c
 *  This C file implements the mtk85xx USB host controller customization driver.
 */

//---------------------------------------------------------------------------
// Include files
//---------------------------------------------------------------------------

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/list.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/usb.h>
#include <linux/kthread.h>
#include <asm/byteorder.h>
#include <asm/io.h>
#include <asm/irq.h>
//#include <asm/system.h>
#include <asm/unaligned.h>
#include <asm/uaccess.h>
#include <linux/dma-mapping.h>
#include <linux/usb/hcd.h>
#include "mtk_hcd.h"
#include "mtk_qmu_api.h"


extern uint8_t bPowerStatus[4];


int MGC_PhyReset(void * pBase, void * pPhyBase)
{
    uint32_t u4Reg = 0;
    uint8_t nDevCtl = 0;

    u4Reg = *(volatile uint32_t*)0xfd024188;
    if(u4Reg & 0x2000000)
    {
        printk("Risc control Register 0x%x\n",u4Reg);
        u4Reg &= ~0x2000000;
        *(volatile uint32_t*)0xfd024188 = u4Reg;
    }

    //2010.11.13, for MT85xx, set USB module clk.
    u4Reg = MGC_VIRT_Read32(0x7C);
    u4Reg |= 0x00010000; 
    MGC_VIRT_Write32(0x7C, u4Reg);
    //~2010.11.13

    MU_MB();

#ifdef CONFIG_ARCH_MT8563
    printk("[USB][mtkflow]MGC_PhyReset Stage005 !!!\n");

	if((UINT32)pBase == MUSB_BASE1 || (UINT32)pBase == MUSB_BASE2){
	printk("[USB]Enable mt8563 port1/2 clock\n");
	//open port1/port2 clock
	//2010.11.13, for MT8560, set USB module clk.
	u4Reg = MGC_VIRT_Read32(0x7C);
	u4Reg |= 0x00010000; 
	MGC_VIRT_Write32(0x7C, u4Reg);
	}
	if((UINT32)pBase == MUSB_BASE1){
    printk("[USB]SET BC1.2 REGs\n");
    MU_MB();
    u4Reg = MGC_PHY_Read32(pPhyBase,(uint32_t)0x24);
    //printk("[usb][BC12]MUC_ResetPhy u4Reg  READ (0x25 0x27)0x24 == 0x%x\n",u4Reg);
    //u4Reg &= ~0xDF;
    
    MU_MB();
    u4Reg |= 0x7F009100; 
    //u4Reg |= 0x91;
    MGC_PHY_Write32(pPhyBase,(uint32_t)0x24, u4Reg);
    MU_MB();
    
   /*
         u4Reg = MGC_PHY_Read32(pBase,(uint32_t)0x27);
         u4Reg |= 0x7F; 
         MGC_PHY_Write32(pBase,(uint32_t)0x27, u4Reg);
         u4Reg = MGC_PHY_Read32(pBase,(uint32_t)0x27);
         printk("[USB]Reg 0x27 is:0x%x\n", u4Reg);
         */
    }
#endif

    //Soft Reset, RG_RESET for Soft RESET, reset MAC
    u4Reg = MGC_PHY_Read32(pPhyBase,(uint32_t)0x68);
    u4Reg |=   0x00004000; 
    MGC_PHY_Write32(pPhyBase, (uint32_t)0x68, u4Reg);
	MU_MB();
	
    u4Reg = MGC_PHY_Read32(pPhyBase,(uint32_t)0x68);
    u4Reg &=  ~0x00004000; 
    MGC_PHY_Write32(pPhyBase, (uint32_t)0x68, u4Reg);
	MU_MB();

    //otg bit setting
    u4Reg = MGC_PHY_Read32(pPhyBase,(uint32_t)0x6C);
    u4Reg &= ~0x3f3f;
    u4Reg |=  0x403f2c;
    MGC_PHY_Write32(pPhyBase, (uint32_t)0x6C, u4Reg);

    //suspendom control
    u4Reg = MGC_PHY_Read32(pPhyBase,(uint32_t)0x68);
    u4Reg &=  ~0x00040000; 
    MGC_PHY_Write32(pPhyBase, (uint32_t)0x68, u4Reg);
    
    //eye diagram improve   
#if defined(CONFIG_ARCH_MT8563)
    //hs eye finetune
    u4Reg = MGC_PHY_Read32(pPhyBase,(uint32_t)0x10);
    u4Reg &=  ~0x00070000;
    u4Reg |= 0x00050000;
    MGC_PHY_Write32(pPhyBase, (uint32_t)0x10, u4Reg);		
		
    //PLL setting to reduce clock jitter
    u4Reg = MGC_PHY_Read32(pPhyBase, 0x0);
    u4Reg &= ~0x70000000;
    u4Reg |= 0x20000000;
    MGC_PHY_Write32(pPhyBase, 0x0, u4Reg);

    u4Reg = MGC_PHY_Read32(pPhyBase, 0x4);
    u4Reg |= 0x3;
    MGC_PHY_Write32(pPhyBase, 0x4, u4Reg);
    //End of PLL setting

    //For FS/LS eye pattern fine-tune
    u4Reg = MGC_PHY_Read32(pPhyBase, 0x10);
    u4Reg &= ~0x00007707;
    u4Reg |=  0x00005503;
    MGC_PHY_Write32(pPhyBase, 0x10, u4Reg);
    //End of FS/LS eye pattern fine-tune

    //For MT8561/MT8552 need enable TX current When HS to reduce clock jitter
    u4Reg = MGC_PHY_Read32(pPhyBase, 0x1c);
    u4Reg &= ~0x03000000;
    u4Reg |=  0x01000000;
    MGC_PHY_Write32(pPhyBase, 0x1c, u4Reg);
	//End of enable TX current
#endif		
    //TX_Current_EN setting 01 , 25:24 = 01 //enable TX current when
    //EN_HS_TX_I=1 or EN_HS_TERM=1
    u4Reg = MGC_PHY_Read32(pPhyBase,(uint32_t)0x1C);
    u4Reg |=  0x01000000; 
    MGC_PHY_Write32(pPhyBase, (uint32_t)0x1C, u4Reg);

    //disconnect threshold,set 615mV
    u4Reg = MGC_PHY_Read32(pPhyBase,(uint32_t)0x18); //RG_USB20_DISCTH, 5890 is 0x18[3:0], others is 0x18[19:16]

	u4Reg &= ~0x000f0000;
    u4Reg |=  0x000c0000;

    MGC_PHY_Write32(pPhyBase, (uint32_t)0x18, u4Reg);
#ifdef CONFIG_ARCH_MT8563
	//save power
    u4Reg = MGC_PHY_Read32(pPhyBase, 0x18);
	u4Reg &= ~0x08000000;
    MGC_PHY_Write32(pPhyBase, 0x18, u4Reg);
	
    u4Reg = MGC_PHY_Read32(pPhyBase, 0x00);
	u4Reg &= ~0x00008001;
    MGC_PHY_Write32(pPhyBase, 0x00, u4Reg);

	msleep(2);

	if(pBase == MUSB_BASE1){
       printk("[USB][BC12]MUC_ResetPhy**********************************************start 00\n");
       MU_MB();	
       u4Reg = MGC_PHY_Read32(pPhyBase, M_REG_PHYC0);
       u4Reg |=0x00008003;
       MU_MB();	
       //MGC_PHY_Write32(0xF0059800,M_REG_PHYC0,u4Reg);
       MGC_PHY_Write32(pPhyBase,M_REG_PHYC0,u4Reg);
       MU_MB();	
       
       //pull up RG_CHGDT_EN
       u4Reg = MGC_PHY_Read32(pPhyBase, M_REG_PHYACR5);
       u4Reg |= 0x00000001;
       MGC_PHY_Write32(pPhyBase,M_REG_PHYACR5,u4Reg);
       MU_MB();	
    }
	//end 
#endif
    u4Reg = MGC_Read8(pBase,M_REG_PERFORMANCE3);
    u4Reg |=  0x80;
    u4Reg &= ~0x40;
    MGC_Write8(pBase, M_REG_PERFORMANCE3, (uint8_t)u4Reg);

    // MT5368/MT5396/MT5389 
    //0xf0050070[10]= 1 for Fs Hub + Ls Device 
    u4Reg = MGC_Read8(pBase, (uint32_t)0x71);
    u4Reg |= 0x04; //RGO_USB20_PRBS7_LOCK
    MGC_Write8(pBase, 0x71, (uint8_t)u4Reg);

    // open session.
    nDevCtl = MGC_Read8(pBase, MGC_O_HDRC_DEVCTL);     
    nDevCtl |= MGC_M_DEVCTL_SESSION;        
    MGC_Write8(pBase, MGC_O_HDRC_DEVCTL, nDevCtl);

    // This is important: TM1_EN
    // speed up OTG setting for checking device connect event after MUC_Initial().
    u4Reg = MGC_Read32(pBase, 0x604);
    u4Reg |= 0x01;
    MGC_Write32(pBase, 0x604, u4Reg);
	
    // open session.
    nDevCtl = MGC_Read8(pBase, MGC_O_HDRC_DEVCTL);   
    nDevCtl |= MGC_M_DEVCTL_SESSION;        
    MGC_Write8(pBase, MGC_O_HDRC_DEVCTL, nDevCtl);

    // FS/LS Slew Rate change
    u4Reg = MGC_PHY_Read32(pPhyBase, 0x10);
    u4Reg &= (uint32_t)(~0x000000ff);
    u4Reg |= (uint32_t)0x00000011;
    MGC_PHY_Write32(pPhyBase, 0x10, u4Reg);
	
    // HS Slew Rate, RG_USB20_HSTX_SRCTRL
    u4Reg = MGC_PHY_Read32(pPhyBase, 0x10);
    u4Reg &= (uint32_t)(~0x00070000);
    u4Reg |= (uint32_t)0x00040000;
    MGC_PHY_Write32(pPhyBase, 0x10, u4Reg);

    // This is important: TM1_EN
    // speed up OTG setting for checking device connect event after MUC_Initial().
    u4Reg = MGC_Read32(pBase, 0x604);
    u4Reg &= ~0x01;
    MGC_Write32(pBase,0x604, u4Reg);
	
    return 0;
}

static void MGC_IntrLevel1En(uint8_t bPortNum)
{
    uint8_t *pBase = NULL;
    uint32_t u4Reg = 0;	
    unsigned long flags=0;
	
    local_irq_save(flags);
	
	switch((uint32_t)bPortNum)
		{
			case 0:
					pBase = (uint8_t*)MUSB_BASE;
					break;
			case 1:
					pBase = (uint8_t*)MUSB_BASE1;
					break;
			case 2:
					pBase = (uint8_t*)MUSB_BASE2;
					break;
			default: 
				#ifdef MUSB_BASE3
					pBase = (uint8_t*)MUSB_BASE3;
				#endif
					break;
		}
	
	if(pBase == NULL)
	{
		local_irq_restore(flags);
		return;
	}
	u4Reg = MGC_Read32(pBase, M_REG_INTRLEVEL1EN);
	#ifdef CONFIG_USB_QMU_SUPPORT
	u4Reg |= 0x2f;
	#else
	u4Reg |= 0x0f;
	#endif
	MGC_Write32(pBase, M_REG_INTRLEVEL1EN, u4Reg);
	
    local_irq_restore(flags);

}

void MGC_VBusControl(uint8_t bPortNum, uint8_t bOn)
{
	extern int gpio_configure(unsigned gpio, int dir, int value);

	#ifdef CONFIG_ARCH_MT8563  //M1V1 EVB
	  //USB PORT0 default give power, low electrical level is enable
	  printk("[usb]mt8563 m1v1 power on\n");
	  gpio_configure(1, 1, 1); //USB PORT1
	  gpio_configure(20, 1, 1); //USB PORT1
	#endif

#ifdef CONFIG_SONY_USB_POWER_CTRL
	extern char usb_cust_model;
	char model[12] = {0};
    if(0 == bPortNum) {
		switch(usb_cust_model) {
			case 1:
				model = "bdp7G ax";
                gpio_configure(6,1,bOn);//7G ax/bx wifi port
                gpio_configure(7,1,bOn);//7G ax  normal port//power on in APP Init
				break;
			case 2:
				model = "bdp7G bx";
                gpio_configure(6,1,bOn);//7G ax/bx wifi port
                gpio_configure(7,1,bOn); //7G bx  postpone power of usb
				break;
			case 3:
				model = "bdv5G ex";
                gpio_configure(76,1,bOn);
                gpio_configure(7,1,bOn); //7G bx  postpone power of usb
				break;
			case 6:
				model = "bdv6G nj";
                gpio_configure(7,1,bOn);
                gpio_configure(8,1,bOn);//wifi port
				break;
			case 7:
				model = "bdv5G nx/bdp7G cx";
                gpio_configure(7,1,bOn);
                gpio_configure(13,1,bOn);
				break;
			case 8:
			    model = "bdp8G ax";
                gpio_configure(70,1,bOn);//8G ax port
				break;
			case 9:
				model = "bdp8G bx";
                gpio_configure(70,1,bOn);//8G bx port
				break;
			case 10:
				model = "bdv6G ej";
                gpio_configure(70,1,bOn);//8G ej front port
                gpio_configure(17,1,bOn);//8G ej wifi port
				break;
			default:
				printk("MT85xx Sony Unknow model\n");
				break;
		}
		printk("[usb]MT85xx Sony %s %s VBUS\n", model, bOn?"enable":"disable");
    }	
#else
    printk("MT85xx not support VBus Control\n");
#endif

	if(bOn)
		bPowerStatus[bPortNum]=1;
	else 
		bPowerStatus[bPortNum]=0;

	if(bOn)
		MGC_IntrLevel1En(bPortNum);	
}



#if defined(CONFIG_ARCH_MT8580)
MUSB_LinuxController MUC_aLinuxController[] = 
{
    {
         /* Port 0 information. */
        .pBase = (void*)MUSB_BASE, 
        .pPhyBase = (void*)((MUSB_BASE + MUSB_PHYBASE)+ (MUSB_PORT0_PHYBASE)), 
        .dwIrq = VECTOR_USB, 
        .bSupportCmdQ = TRUE, 
        .bEndpoint_Tx_num = 11,	  /* Endpoint Tx Number : 10+1 */
        .bEndpoint_Rx_num = 11,   /* Endpoint Rx Number : 10+1  */
        .bHub_support = TRUE,
        .MGC_pfVbusControl = MGC_VBusControl, 
        .MGC_pfPhyReset = MGC_PhyReset,          
    },   
    {
         /* Port 1 information. */
        .pBase = (void*)MUSB_BASE1, 
        .pPhyBase = (void*)((MUSB_BASE + MUSB_PHYBASE)+ (MUSB_PORT1_PHYBASE)), 
        .dwIrq = VECTOR_USB2, 
        .bSupportCmdQ = TRUE, 
        .bEndpoint_Tx_num = 11,    /* Endpoint Tx Number : 10+1  */
        .bEndpoint_Rx_num = 11,    /* Endpoint Rx Number : 10+1  */
        .bHub_support = TRUE,
        .MGC_pfVbusControl = MGC_VBusControl,
        .MGC_pfPhyReset = MGC_PhyReset,        
    },
#ifndef USB_DEVICE_MODE_SUPPORT       
    {
         /* Port 2 information. */
        .pBase = (void*)MUSB_BASE2, 
        .pPhyBase = (void*)((MUSB_BASE2 + MUSB_PWD_PHYBASE)+ (MUSB_PORT0_PHYBASE)), 
        .dwIrq = VECTOR_USB3, 
        .bSupportCmdQ = TRUE, 
        .bEndpoint_Tx_num = 11,    /* Endpoint Tx Number : 10+1  */
        .bEndpoint_Rx_num = 11,    /* Endpoint Rx Number : 10+1  */
        .bHub_support = TRUE,
        .MGC_pfVbusControl = MGC_VBusControl,
        .MGC_pfPhyReset = MGC_PhyReset,        
    }
#endif 
};
#elif defined(CONFIG_ARCH_MT8563)
MUSB_LinuxController MUC_aLinuxController[] = 
{
    {
         /* Port 0 information. */
        .pBase = (void*)MUSB_BASE, 
        .pPhyBase = (void*)(IO_VIRT+ (MUSB_PORT0_PHYBASE)), 
        .dwIrq = VECTOR_USB, 
        .bSupportCmdQ = TRUE, 
        .bEndpoint_Tx_num = 6,	  /* Endpoint Tx Number : 5+1 */
        .bEndpoint_Rx_num = 6,    /* Endpoint Rx Number : 5+1 */
        .bHub_support = FALSE,
        .MGC_pfVbusControl = MGC_VBusControl, 
        .MGC_pfPhyReset = MGC_PhyReset,          
    },    
    {
         /* Port 1 information. */
        .pBase = (void*)MUSB_BASE1, 
        .pPhyBase = (void*)(IO_VIRT + (MUSB_PORT1_PHYBASE)), 
        .dwIrq = VECTOR_USB2, 
        .bSupportCmdQ = FALSE, 
        .bEndpoint_Tx_num = 6,    /* Endpoint Tx Number : 5+1 */
        .bEndpoint_Rx_num = 6,    /* Endpoint Rx Number : 5+1 */
        .bHub_support = FALSE,
        .MGC_pfVbusControl = MGC_VBusControl,
        .MGC_pfPhyReset = MGC_PhyReset,        
    },
#ifndef USB_DEVICE_MODE_SUPPORT     
    {
         /* Port 2 information. */
        .pBase = (void*)MUSB_BASE2, 
        .pPhyBase = (void*)(IO_VIRT+ (MUSB_PORT2_PHYBASE)), 
        .dwIrq = VECTOR_USB3, 
        .bSupportCmdQ = FALSE, 
        .bEndpoint_Tx_num = 6,    /* Endpoint Tx Number : 5+1 */
        .bEndpoint_Rx_num = 6,    /* Endpoint Rx Number :5+1 */
        .bHub_support = TRUE,
        .MGC_pfVbusControl = MGC_VBusControl,
        .MGC_pfPhyReset = MGC_PhyReset,        
    }
#endif
};

#else
    #error CONFIG_ARCH_MT85XX not defined !
#endif



