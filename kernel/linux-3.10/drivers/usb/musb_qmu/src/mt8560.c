/*-----------------------------------------------------------------------------
 * drivers/usb/musb_qmu/mt8560.c
 *
 * MT53xx USB driver
 *
 * Copyright (c) 2008-2012 MediaTek Inc.
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
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include "musb.h"
#include "musb_core.h"

#include "mt8560.h"
#include "musbhsdma.h"
#if defined(CONFIG_ARCH_MT85XX)
#include"musbconf.h"
#endif

#if !defined(CONFIG_ARCH_MT85XX)
#ifdef CONFIG_64BIT
#include <linux/soc/mediatek/mt53xx_linux.h>
#else
#include <mach/mt53xx_linux.h>
#endif
#endif
extern void __iomem *pUSBadmaIoMap;

void musb_hcd_release (struct device *dev) 
{
    DBG(3, "dev = 0x%p.\n", dev);
}  
//-------------------------------------------------------------------------
#ifdef CC_MT8580
static struct musb_hdrc_config musb_config_mt8560[] = {
 [0]={
        .multipoint    =true,
        .dyn_fifo       = true,
        .soft_con       = true,
        .dma            = true,

        .num_eps        = 11,
        .dma_channels   = 4,
        .ram_bits       = 10,
        .id             = 0,
      },
 [1]={
        .multipoint    =true,
        .dyn_fifo       = true,
        .soft_con       = true,
        .dma            = true,

        .num_eps        = 11,
        .dma_channels   = 4,
        .ram_bits       = 10,
        .id             = 1,
      },     
 [2]={
        .multipoint    =true,
        .dyn_fifo       = true,
        .soft_con       = true,
        .dma            = true,

        .num_eps        = 11,
        .dma_channels   = 4,
        .ram_bits       = 10,
        .id             = 2,
      },   
};
#elif defined(CC_MT8563)
static struct musb_hdrc_config musb_config_mt8560[] = {
 [0]={
        .multipoint    =true,
        .dyn_fifo       = true,
        .soft_con       = true,
        .dma            = true,

        .num_eps        = 6,
        .dma_channels   = 4,
        .ram_bits       = 10,
        .id             = 0,
      },
 [1]={
        .multipoint    =true,
        .dyn_fifo       = true,
        .soft_con       = true,
        .dma            = true,

        .num_eps        = 6,
        .dma_channels   = 4,
        .ram_bits       = 10,
        .id             = 1,
      },     
 [2]={
        .multipoint    =true,
        .dyn_fifo       = true,
        .soft_con       = true,
        .dma            = true,

        .num_eps        = 6,
        .dma_channels   = 4,
        .ram_bits       = 10,
        .id             = 2,
      },   
};
#else //MT5XXX
static struct musb_hdrc_config musb_config_mt8560[] = {
 [0]={
        .multipoint     = true,
        .dyn_fifo       = true,
        .soft_con       = true,
        .dma            = true,

        .num_eps        = 8,
        .dma_channels   = 4,
        .ram_bits       = 10,
        .id             = 0,
      },
 [1]={
        .multipoint     = true,
        .dyn_fifo       = true,
        .soft_con       = true,
        .dma            = true,

        .num_eps        = 4,
        .dma_channels   = 2,
        .ram_bits       = 10,
        .id             = 1,
      },     
 [2]={
        .multipoint     = true,
        .dyn_fifo       = true,
        .soft_con       = true,
        .dma            = true,

        .num_eps        = 6,
        .dma_channels   = 2,
        .ram_bits       = 10,
        .id             = 2,
      },   
[3]={
        .multipoint     = true,
        .dyn_fifo       = true,
        .soft_con       = true,
        .dma            = true,

        .num_eps        = 6,
        .dma_channels   = 2,
        .ram_bits       = 10,
        .id             = 3,
      }, 
};
#endif

static struct musb_hdrc_platform_data usb_data_mt8560[]  = {
 [0]={	
        #if defined(CONFIG_USB_GADGET_MUSB_HDRC)
        	.mode           = MUSB_PERIPHERAL,
        #elif defined(CONFIG_USB_MUSB_HDRC_HCD)
        	.mode           = MUSB_HOST,
        #endif
        	.config         = musb_config_mt8560 ,
 	 },	
 [1]={	
        #if defined(CONFIG_USB_GADGET_MUSB_HDRC)
        	.mode           = MUSB_PERIPHERAL,
        #elif defined(CONFIG_USB_MUSB_HDRC_HCD)
        	.mode           = MUSB_HOST,
        #endif
        	.config         = musb_config_mt8560+1 ,
 	 },	
 [2]={	
        #if defined(CONFIG_USB_GADGET_MUSB_HDRC)
        	.mode           = MUSB_PERIPHERAL,
        #elif defined(CONFIG_USB_MUSB_HDRC_HCD)
        	.mode           = MUSB_HOST,
        #endif
        	.config         = musb_config_mt8560+2 ,
 	 },	 
[3]={	
        #if defined(CONFIG_USB_GADGET_MUSB_HDRC)
        	.mode           = MUSB_PERIPHERAL,
        #elif defined(CONFIG_USB_MUSB_HDRC_HCD)
        	.mode           = MUSB_HOST,
        #endif
        	.config         = musb_config_mt8560+3 ,
 	 },	 
};


#if !defined(CONFIG_ARCH_MT85XX)
static struct resource usb0_resources_mt8560 [] = {
       {    /* physical address */
                .flags          = IORESOURCE_MEM,
        },
        {	/*general IRQ*/
                .start          = MUSB_VECTOR_USB0,
                .flags          = IORESOURCE_IRQ,
        },
        {	/*DMA IRQ*/
                .start          = MUSB_VECTOR_USB0 ,
                .flags          = IORESOURCE_IRQ,
        },

	
};
static struct resource usb1_resources_mt8560 [] = {
       {    /* physical address */
           //     .start          = MUSB_BASE1,
           //     .end            =MUSB_BASE1+ MUSB_DEFAULT_ADDRESS_SPACE_SIZE-1,
                .flags          = IORESOURCE_MEM,
        },
        {	/*general IRQ*/
                .start          = MUSB_VECTOR_USB1,
                .flags          = IORESOURCE_IRQ,
        },
        {	/*DMA IRQ*/
                .start          = MUSB_VECTOR_USB1,
                .flags          = IORESOURCE_IRQ,
        },

	
};
static struct resource usb3_resources_mt8560 [] = {
       {    /* physical address */
              //  .start          = MUSB_BASE3,
              //  .end            =MUSB_BASE3+ MUSB_DEFAULT_ADDRESS_SPACE_SIZE-1,
                .flags          = IORESOURCE_MEM,
        },
        {	/*general IRQ*/
                .start          = MUSB_VECTOR_USB3,
                .flags          = IORESOURCE_IRQ,
        },
        {	/*DMA IRQ*/
                .start          = MUSB_VECTOR_USB3,
                .flags          = IORESOURCE_IRQ,
        },

	
};

static struct resource usb2_resources_mt8560 [] = {
       {        /* physical address */
             //   .start          = MUSB_BASE2,
           //     .end            = MUSB_BASE2+ MUSB_DEFAULT_ADDRESS_SPACE_SIZE-1,
                .flags          = IORESOURCE_MEM,
        },
        {	/*general IRQ*/
                .start          = MUSB_VECTOR_USB2,
                .flags          = IORESOURCE_IRQ,
        },
        {	/*DMA IRQ*/
                .start          = MUSB_VECTOR_USB2,
                .flags          = IORESOURCE_IRQ,
        },

	
};
#else //#if !defined(CONFIG_ARCH_MT85XX)
static struct resource usb0_resources_mt8560 [] = {
       {        /* physical address */
                .start          = MUSB_BASE ,
                .end            = MUSB_BASE + MUSB_DEFAULT_ADDRESS_SPACE_SIZE -1,
                .flags          = IORESOURCE_MEM,
        },
        {	/*general IRQ*/
                .start          = MUSB_VECTOR_USB0,
                .flags          = IORESOURCE_IRQ,
        },
        {	/*DMA IRQ*/
                .start          = MUSB_VECTOR_USB0 ,
                .flags          = IORESOURCE_IRQ,
        },

	
};
static struct resource usb1_resources_mt8560 [] = {
       {        /* physical address */
                .start          = MUSB_BASE2,
                .end            =MUSB_BASE2+ MUSB_DEFAULT_ADDRESS_SPACE_SIZE-1,
                .flags          = IORESOURCE_MEM,
        },
        {	/*general IRQ*/
                .start          = MUSB_VECTOR_USB1,
                .flags          = IORESOURCE_IRQ,
        },
        {	/*DMA IRQ*/
                .start          = MUSB_VECTOR_USB1,
                .flags          = IORESOURCE_IRQ,
        },

	
};
static struct resource usb2_resources_mt8560 [] = {
       {        /* physical address */
                .start          = MUSB_BASE3,
                .end            = MUSB_BASE3+ MUSB_DEFAULT_ADDRESS_SPACE_SIZE-1,
                .flags          = IORESOURCE_MEM,
        },
        {	/*general IRQ*/
                .start          = MUSB_VECTOR_USB2,
                .flags          = IORESOURCE_IRQ,
        },
        {	/*DMA IRQ*/
                .start          = MUSB_VECTOR_USB2,
                .flags          = IORESOURCE_IRQ,
        },

	
};
#endif //#if !defined(CONFIG_ARCH_MT85XX)
static u64 usb_dmamask = DMA_BIT_MASK(32);

#ifdef CONFIG_USB_GADGET_MUSB_HDRC

struct platform_device usb_dev[ ]= {
    [0]={
            .name           = "MtkGadgetUsbHcd",
            .id             = 0,
            .dev = {
                    .platform_data          = usb_data_mt8560+3 ,
                    .dma_mask               = &usb_dmamask,
                    .coherent_dma_mask      = DMA_BIT_MASK(32),
                    .release=musb_hcd_release,
            },
            .resource       = usb0_resources_mt8560 ,
            .num_resources  = ARRAY_SIZE(usb3_resources_mt8560 ),
	},

    [1]={
            .name           = "MtkGadgetUsbHcd",
            .id             = 0,
            .dev = {
               .platform_data          = usb_data_mt8560+2 ,
               .dma_mask               = &usb_dmamask,
               .coherent_dma_mask      = DMA_BIT_MASK(32),
               .release=musb_hcd_release,
            },
            .resource       = usb1_resources_mt8560 ,
            .num_resources  = ARRAY_SIZE(usb2_resources_mt8560 ),
    },

    [2]={
            .name           = "MtkGadgetUsbHcd",
            .id             = 0,
            .dev = {
                  .platform_data          = usb_data_mt8560+1 ,
                  .dma_mask               = &usb_dmamask,
                  .coherent_dma_mask      = DMA_BIT_MASK(32),
                  .release=musb_hcd_release,
            },
            .resource       = usb2_resources_mt8560 ,
            .num_resources  = ARRAY_SIZE(usb1_resources_mt8560 ),
    },

    [3]={
            .name           = "MtkGadgetUsbHcd",
            .id             = 0,
            .dev = {
                 .platform_data          = usb_data_mt8560 ,
                 .dma_mask               = &usb_dmamask,
                 .coherent_dma_mask      = DMA_BIT_MASK(32),
                 .release=musb_hcd_release,
            },
            .resource       = usb3_resources_mt8560 ,
            .num_resources  = ARRAY_SIZE(usb0_resources_mt8560 ),
    },
};


#else //#ifdef CONFIG_USB_GADGET_MUSB_HDRC
struct platform_device usb_dev[ ]= {
 	[0]={
        .name           = "MtkGadgetUsbHcd",
        .id             = 0,
        .dev = {
                .platform_data          = usb_data_mt8560 ,
                .dma_mask               = &usb_dmamask,
                .coherent_dma_mask      = DMA_BIT_MASK(32),
                .release=musb_hcd_release,
        },
        .resource       = usb0_resources_mt8560 ,
        .num_resources  = ARRAY_SIZE(usb0_resources_mt8560 ),
 	},
  	[1]={
        .name           = "MtkGadgetUsbHcd",
        .id             = 1,
        .dev = {
                .platform_data          = usb_data_mt8560+1 ,
                .dma_mask               = &usb_dmamask,
                .coherent_dma_mask      = DMA_BIT_MASK(32),
                .release=musb_hcd_release,
        },
        .resource       = usb1_resources_mt8560 ,
        .num_resources  = ARRAY_SIZE(usb1_resources_mt8560 ),
 	},
   	[2]={
        .name           = "MtkGadgetUsbHcd",
        .id             = 2,
        .dev = {
                .platform_data          = usb_data_mt8560+2 ,
                .dma_mask               = &usb_dmamask,
                .coherent_dma_mask      = DMA_BIT_MASK(32),
                .release=musb_hcd_release,
        },
        .resource       = usb2_resources_mt8560 ,
        .num_resources  = ARRAY_SIZE(usb2_resources_mt8560 ),
 	},
 	   	[3]={
        .name           = "MtkGadgetUsbHcd",
        .id             = 3,
        .dev = {
                .platform_data          = usb_data_mt8560+3 ,
                .dma_mask               = &usb_dmamask,
                .coherent_dma_mask      = DMA_BIT_MASK(32),
                .release=musb_hcd_release,
        },
        .resource       = usb3_resources_mt8560 ,
        .num_resources  = ARRAY_SIZE(usb3_resources_mt8560 ),
 	},
};
#endif //#ifdef CONFIG_USB_GADGET_MUSB_HDRC

#if !defined(CONFIG_ARCH_MT85XX)
int MUC_ResetPhy_Musb(void *pBase)
{
    uint32_t u4Reg = 0;
    uint32_t *pu4MemTmp;
	if(!pBase){
		printk(KERN_ALERT "error: pBase null!!!\n");
		return -1;
	} 
		
	// USB DMA enable	
	// FPGA USB DMA's enable regiater address is different from actual IC.
	#ifdef LINUX_USB_VDEC_FPGA_TEST
	pu4MemTmp = (uint32_t *)0xf0060100;
	#elif defined(CONFIG_ARCH_MT5882) || defined(CONFIG_ARCH_MT5883)
	pu4MemTmp = (uint32_t *)0xf0061B00;
	#else // defined(CONFIG_ARCH_MT5399) || defined(CONFIG_ARCH_MT5890)
	pu4MemTmp = (uint32_t *)pUSBadmaIoMap;
	#endif
	*pu4MemTmp = 0x00cfff00;
	
    MU_MB();
	
#if !(defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5891))//5890 no need set to 1 
	// VRT insert R enable 
	u4Reg = MGC_PHY_Read32(pBase, (uint32_t)0x00);
	u4Reg |= 0x4000; //RG_USB20_INTR_EN, 5890 is 0x00[5], others is 0x00[14]
	MGC_PHY_Write32(pBase, (uint32_t)0x00, u4Reg);
#endif

#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5891)
	//add for Oryx USB20 PLL fail
	u4Reg = MGC_PHY_Read32(pBase,0x00); //RG_USB20_USBPLL_FORCE_ON=1b
	u4Reg |=   0x00008000; 
	//u4Reg =	0x0000940a; //for register overwrite by others issue
	MGC_PHY_Write32(pBase, 0x00, u4Reg);

	u4Reg = MGC_PHY_Read32(pBase,0x14); //RG_USB20_HS_100U_U3_EN=1b
	u4Reg |=   0x00000800; 
	MGC_PHY_Write32(pBase, 0x14, u4Reg);

	if(IS_IC_MT5890_ES1())
	{
		//driving adjust
		u4Reg = MGC_PHY_Read32(pBase,0x04);
		u4Reg &=  ~0x00007700;
		MGC_PHY_Write32(pBase, 0x04, u4Reg);
	}
#endif
	
    MU_MB();
    //Soft Reset, RG_RESET for Soft RESET
    u4Reg = MGC_PHY_Read32(pBase,0x68);
    u4Reg |=   0x00004000; 
    MGC_PHY_Write32(pBase, 0x68, u4Reg);
    MU_MB();
    u4Reg = MGC_PHY_Read32(pBase,0x68);
    u4Reg &=  ~0x00004000; 
    MGC_PHY_Write32(pBase, 0x68, u4Reg);
	MU_MB();
	
    //otg bit setting
    u4Reg = MGC_PHY_Read32(pBase,0x6C);
    u4Reg &= ~0x3f3f;
	#ifdef CONFIG_USB_GADGET_MUSB_HDRC
	printk("support gadget hdrc.\n");
    u4Reg |=  0x3e2e;
	#else
	printk("do not support gadget hdrc.\n");
    u4Reg |=  0x3e2c;
	#endif	
    MGC_PHY_Write32(pBase, 0x6C, u4Reg);

    //suspendom control
    u4Reg = MGC_PHY_Read32(pBase,0x68);
    u4Reg &=  ~0x00040000; 
    MGC_PHY_Write32(pBase, 0x68, u4Reg);

	//eye diagram improve	
	//TX_Current_EN setting 01 , 25:24 = 01 //enable TX current when
	//EN_HS_TX_I=1 or EN_HS_TERM=1
	u4Reg = MGC_PHY_Read32(pBase,0x1C);
	u4Reg |=  0x01000000; 
	MGC_PHY_Write32(pBase, 0x1C, u4Reg);

	//disconnect threshold, 0xb: 620mV
	u4Reg = MGC_PHY_Read32(pBase,(uint32_t)0x18); //RG_USB20_DISCTH
#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5891)
	if(IS_IC_MT5890_ES1())
	{
		u4Reg &= ~0x0000000f;
		u4Reg |=  0x0000000f; //0x18[3:0]
	}
	else
	{
		u4Reg &= ~0x000000f0;
		u4Reg |=  0x000000b0; //0x18[7:4]
	}
#else
	u4Reg &= ~0x000f0000;
	u4Reg |=  0x000b0000;	//others IC is 0x18[19:16]
#endif
	MGC_PHY_Write32(pBase, 0x18, u4Reg);

    u4Reg = MGC_Read8(pBase,M_REG_PERFORMANCE3);
    u4Reg |=  0x80;
    u4Reg &= ~0x40;
    MGC_Write8(pBase, M_REG_PERFORMANCE3, (uint8_t)u4Reg);

#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5891)
	u4Reg = MGC_Read8(pBase, 0x7C);
	u4Reg |= 0x04;
	MGC_Write8(pBase, 0x7C, (uint8_t)u4Reg);
#else
	// MT5368/MT5396/MT5389 
	//0xf0050070[10]= 1 for Fs Hub + Ls Device 
	u4Reg = MGC_Read8(pBase, (ulong)0x71);
	u4Reg |= 0x04; //RGO_USB20_PRBS7_LOCK
	MGC_Write8(pBase, 0x71, (uint8_t)u4Reg);
#endif

	// FS/LS Slew Rate change
	u4Reg = MGC_PHY_Read32(pBase, 0x10);
	u4Reg &= (~0x000000ff);
	u4Reg |= 0x00000011;
	MGC_PHY_Write32(pBase, (ulong)0x10, u4Reg);

	// HS Slew Rate, RG_USB20_HSTX_SRCTRL
#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5891)
	u4Reg = MGC_PHY_Read32(pBase, 0x14); //bit[14:12]
	if(IS_IC_MT5890_ES1())
	{
		u4Reg &= (~0x00007000);
		u4Reg |= 0x00001000;
	}
	else
	{
		u4Reg &= (~0x00007000);
		u4Reg |= 0x00004000;
	}
	MGC_PHY_Write32(pBase, 0x14, u4Reg);
#else
	u4Reg = MGC_PHY_Read32(pBase, 0x10);
	u4Reg &= (~0x00070000);
	u4Reg |= 0x00040000;
	MGC_PHY_Write32(pBase, 0x10, u4Reg);
#endif

#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5891)
	u4Reg = MGC_PHY_Read32(pBase, (ulong)0x14);
	u4Reg &= ~0x300000;
	u4Reg |= 0x10200000; //bit[21:20]: RG_USB20_DISCD[1:0]=2'b10. bit[28]: RG_USB20_DISC_FIT_EN=1'b1
	MGC_PHY_Write32(pBase, 0x14, u4Reg);
#endif

    mdelay(10);
    printk("[usb]pBase 0x%p phy init complete\n",pBase);    	

    return 0;
}
#elif defined(CC_MT8580)
int MUC_ResetPhy_Musb(void *pBase)
{
    uint32_t u4Reg = 0;
    uint32_t tmp = 0;

	if(!pBase){
			printk(KERN_ALERT "error: pBase null!!!\n");
			return -1;
	} 
    //2010.11.13, for MT8560, set USB module clk.
    u4Reg = MGC_VIRT_Read32(0x7C);
	tmp = IO_VIRT;
    u4Reg |= 0x00010000; 
    MGC_VIRT_Write32(0x7C, u4Reg);
    //~2010.11.13

    MU_MB();
    //Soft Reset, RG_RESET for Soft RESET
    u4Reg = MGC_PHY_Read32(pBase,(uint32_t)0x68);
    u4Reg |=   0x00004000; 
    MGC_PHY_Write32(pBase, (uint32_t)0x68, u4Reg);	

    MU_MB();
    u4Reg = MGC_PHY_Read32(pBase,(uint32_t)0x68);
    u4Reg &=  ~0x00004000; 
    MGC_PHY_Write32(pBase, (uint32_t)0x68, u4Reg);

	MU_MB();
    	
    //otg bit setting
    u4Reg = MGC_PHY_Read32(pBase,(uint32_t)0x6C);
    u4Reg &= ~0x3f3f;
	#ifdef CONFIG_USB_GADGET_MUSB_HDRC
        u4Reg |=  0x3e2e;
	#else
        u4Reg |=  0x3e2c;
	#endif	
    MGC_PHY_Write32(pBase, (uint32_t)0x6C, u4Reg);
    		
    //suspendom control
    u4Reg = MGC_PHY_Read32(pBase,(uint32_t)0x68);
    u4Reg &=  ~0x00040000; 
    MGC_PHY_Write32(pBase, (uint32_t)0x68, u4Reg);
	#if 1
    //hs eye finetune
    u4Reg = MGC_PHY_Read32(pBase,(uint32_t)0x10);
    u4Reg &=  ~0x00070000;
    u4Reg |= 0x00050000;
    MGC_PHY_Write32(pBase, (uint32_t)0x10, u4Reg);		
    	
    //PLL setting to reduce clock jitter
    u4Reg = MGC_PHY_Read32(pBase, 0x0);
    u4Reg &= ~0x70000000;
    u4Reg |= 0x20000000;
    MGC_PHY_Write32(pBase, 0x0, u4Reg);

    u4Reg = MGC_PHY_Read32(pBase, 0x4);
    u4Reg |= 0x3;
    MGC_PHY_Write32(pBase, 0x4, u4Reg);
    //End of PLL setting

    //For FS/LS eye pattern fine-tune
    u4Reg = MGC_PHY_Read32(pBase, 0x10);
    u4Reg &= ~0x00007707;
    u4Reg |=  0x00005503;
    MGC_PHY_Write32(pBase, 0x10, u4Reg);
    //End of FS/LS eye pattern fine-tune
	#endif

	//disconnect threshold
	  u4Reg = MGC_PHY_Read32(pBase, 0x18);
	  u4Reg &= ~0x000f0000;
    u4Reg |=  0x00070000;
    MGC_PHY_Write32(pBase, 0x18, u4Reg);
    	
    u4Reg = MGC_Read8(pBase,M_REG_PERFORMANCE3);
    u4Reg |=  0x80;
    u4Reg &= ~0x40;
    MGC_Write8(pBase, M_REG_PERFORMANCE3, (uint8_t)u4Reg);
    mdelay(10);
    printk("[usb]pBase 0x%x phy init complete\n",(uint32_t)pBase);
    	
    return 0;
}

#elif defined(CC_MT8563)
int MUC_ResetPhy_Musb(void *pBase)
{
    uint32_t u4Reg = 0;
    uint32_t tmp = 0;

	if(!pBase){
			printk(KERN_ALERT "error: pBase null!!!\n");
			return -1;
	} 

	#ifdef CC_MT8563
	printk("[USB]base addr:0x%x base2:0x%x base3:0x%x\n", MUSB_BASE, MUSB_BASE2, MUSB_BASE3);
	if(pBase == MUSB_BASE2 || pBase == MUSB_BASE3){
		printk("[USB]Enable mt8563 port1/2 clock\n");
		//open port1/port2 clock
		u4Reg = MGC_VIRT_Read32(0x7C);
	    u4Reg |= 0x00010000; 
	    MGC_VIRT_Write32(0x7C, u4Reg);
	}
	#endif

    //Soft Reset, RG_RESET for Soft RESET
    u4Reg = MGC_PHY_Read32(pBase,(uint32_t)0x68);
    u4Reg |=   0x00004000; 
    MGC_PHY_Write32(pBase, (uint32_t)0x68, u4Reg);	

    MU_MB();
    u4Reg = MGC_PHY_Read32(pBase,(uint32_t)0x68);
    u4Reg &=  ~0x00004000; 
    MGC_PHY_Write32(pBase, (uint32_t)0x68, u4Reg);

	MU_MB();
    	
    //otg bit setting
    u4Reg = MGC_PHY_Read32(pBase,(uint32_t)0x6C);
    u4Reg &= ~0x3f3f;
	#ifdef CONFIG_USB_GADGET_MUSB_HDRC
        u4Reg |=  0x403e2e;
	#else
        u4Reg |=  0x403e2c;
	#endif	
    MGC_PHY_Write32(pBase, (uint32_t)0x6C, u4Reg);
    		
    //suspendom control
    u4Reg = MGC_PHY_Read32(pBase,(uint32_t)0x68);
    u4Reg &=  ~0x00040000; 
    MGC_PHY_Write32(pBase, (uint32_t)0x68, u4Reg);
	#if 1
    //hs eye finetune
    u4Reg = MGC_PHY_Read32(pBase,(uint32_t)0x10);
    u4Reg &=  ~0x00070000;
    u4Reg |= 0x00050000;
    MGC_PHY_Write32(pBase, (uint32_t)0x10, u4Reg);		
    	
    //PLL setting to reduce clock jitter
    u4Reg = MGC_PHY_Read32(pBase, 0x0);
    u4Reg &= ~0x70000000;
    u4Reg |= 0x20000000;
    MGC_PHY_Write32(pBase, 0x0, u4Reg);

    u4Reg = MGC_PHY_Read32(pBase, 0x4);
    u4Reg |= 0x3;
    MGC_PHY_Write32(pBase, 0x4, u4Reg);
    //End of PLL setting

    //For FS/LS eye pattern fine-tune
    u4Reg = MGC_PHY_Read32(pBase, 0x10);
    u4Reg &= ~0x00007707;
    u4Reg |=  0x00005503;
    MGC_PHY_Write32(pBase, 0x10, u4Reg);
    //End of FS/LS eye pattern fine-tune
	#endif

	//disconnect threshold
	  u4Reg = MGC_PHY_Read32(pBase, 0x18);
	  u4Reg &= ~0x000f0000;
    u4Reg |=  0x00070000;
    MGC_PHY_Write32(pBase, 0x18, u4Reg);
    	
    u4Reg = MGC_Read8(pBase,M_REG_PERFORMANCE3);
    u4Reg |=  0x80;
    u4Reg &= ~0x40;
    MGC_Write8(pBase, M_REG_PERFORMANCE3, (uint8_t)u4Reg);
    mdelay(10);
    printk("[usb]pBase 0x%x phy init complete\n",(uint32_t)pBase);
    	
    return 0;
}
#endif
int musb_platform_init(struct musb *musb)
{
	u32  retval;
	retval = MUC_ResetPhy_Musb(musb->mregs);
	if (retval < 0)
	{
		DBG(1, "#####Error.%d \n",retval);
		return -ENODEV;
	}
	else
	{
		DBG(1,"PHY Initialize successful!\n");
	}
	return 0;
}

int musb_platform_exit(struct musb *musb)
{
	return 0;
}

int musb_platform_set_mode(struct musb *musb, u8 musb_mode)
{
	return 0;
}

void musb_platform_enable(struct musb *musb)
{   
	uint32_t u4Reg = 0;
	void __iomem	*regs = musb->mregs;
	u4Reg = MGC_Read32(musb->mregs, M_REG_INTRLEVEL1EN);
	u4Reg |= 0x2f;
	MGC_Write32(musb->mregs, M_REG_INTRLEVEL1EN, u4Reg);
	printk ("[usb]Setting level1En to 0x%08x\n",u4Reg);
	musb_writel(regs,MUSB_TXTOG,0x0);
	musb_writel(regs,MUSB_RXTOG,0x0);
	musb_writel(regs, MUSB_HSDMA_INTR, 0xF000000); 
}
void musb_platform_disable(struct musb *musb)
{
}
static void MGC_LoadFifo_Musb(const uint8_t * pBase, uint8_t bEnd,
                         uint16_t wCount, const uint8_t * pSource)
{
    uint32_t dwDatum = 0;

    uint32_t dwCount = wCount;
    uint8_t bFifoOffset = MGC_FIFO_OFFSET(bEnd);
    uint32_t dwBufSize = 4;

    DBG(2, "pBase=%p, bEnd=%d, wCount=0x%04x, pSrc=%p\n", pBase, bEnd,
        wCount, pSource);

#ifdef MUSB_PARANOID
    if (IS_INVALID_ADDRESS(pSource))
    {
        ERR("loading fifo from a null buffer; why did u do that????\n");

        return;
    }
#endif

    //  do not handle zero length data.
    if (dwCount == 0)
    {
        return;
    }

    /* byte access for unaligned */
    if ((dwCount > 0) && ((ulong) pSource & 3))
    {
        while (dwCount)
        {
     		if (3 == dwCount || 2 == dwCount)
     		{
     			dwBufSize = 2;
     		    MU_MB();
     			// set FIFO byte count.
     			MGC_FIFO_CNT(pBase, M_REG_FIFOBYTECNT, 1);
     		}
			else if(1 == dwCount)
			{
     			dwBufSize = 1;
     		    MU_MB();
     			// set FIFO byte count.
     			MGC_FIFO_CNT(pBase, M_REG_FIFOBYTECNT, 0);			   
			}
     		
     		memcpy((void *)&dwDatum, (const void *)pSource, dwBufSize);
     		MU_MB();
     		MGC_Write32(pBase, bFifoOffset, dwDatum);
     		
     		dwCount -= dwBufSize;
     		pSource += dwBufSize;
        }
    }
    else                        /* word access if aligned */
    {
        while ((dwCount > 3) && !((ulong) pSource & 3))
        {
            MGC_Write32(pBase, bFifoOffset,
                        *((uint32_t *) ((void *) pSource)));

            pSource += 4;
            dwCount -= 4;
        }
        if (3 == dwCount || 2 == dwCount)
        {
            MUSB_ASSERT(dwCount < 4);

            // set FIFO byte count.
            MU_MB();
            MGC_FIFO_CNT(pBase, M_REG_FIFOBYTECNT, 1);

            MU_MB();
            MGC_Write32(pBase, bFifoOffset, *((uint32_t *)((void *)pSource)));
			dwCount -= 2;
			pSource += 2;
        }
		
		if(1 == dwCount)
		{
		  MU_MB();
		  MGC_FIFO_CNT(pBase, M_REG_FIFOBYTECNT, 0);
		  if((ulong)pSource & 3)
		  {
     		
     		memcpy((void *)&dwDatum, (const void *)pSource, 1);
     		MU_MB();
     		MGC_Write32(pBase, bFifoOffset, dwDatum);		    
		  }
		  else
		  {
		    MU_MB();
            MGC_Write32(pBase, bFifoOffset, *((uint32_t *)((void *)pSource)));		    
		  }
		}
    }

	    MU_MB();


		MGC_FIFO_CNT(pBase, M_REG_FIFOBYTECNT, 2);
        MU_MB();

    return;
}

void MGC_UnloadFifo_Musb(const uint8_t * pBase, uint8_t bEnd, uint16_t wCount,
                    uint8_t * pDest)
{
    uint32_t dwDatum = 0;

    uint32_t dwCount = wCount;
    uint8_t bFifoOffset = MGC_FIFO_OFFSET(bEnd);
    uint32_t i;

    DBG(2, "pBase=%p, bEnd=%d, wCount=0x%04x, pDest=%p\n", pBase, bEnd,
        wCount, pDest);

    //  do not handle zero length data.
    if (dwCount == 0)
    {
        return;
    }

    if (((ulong) pDest) & 3)
    {
        /* byte access for unaligned */
        while (dwCount > 0)
        {
            if (dwCount < 4)
            {
               if(3 == dwCount || 2 == dwCount)
               {
                   MU_MB();
				   MGC_FIFO_CNT(pBase, M_REG_FIFOBYTECNT, 1);
				   MU_MB();
				   dwDatum = MGC_Read32(pBase, bFifoOffset);
				   
				   for (i = 0; i < 2; i++)
				   {
					   *pDest++ = ((dwDatum >> (i *8)) & 0xFF);
				   }
				   
                   dwCount -=2;
              }

			  if(1 == dwCount) 
			  {
			    MU_MB();
			    MGC_FIFO_CNT(pBase, M_REG_FIFOBYTECNT, 0);
				MU_MB();
                dwDatum = MGC_Read32(pBase, bFifoOffset);
				*pDest++ = (dwDatum  & 0xFF);
				 dwCount -= 1;
			  }
			   
			   MU_MB();
			   // set FIFO byte count = 4 bytes.
			   MGC_FIFO_CNT(pBase, M_REG_FIFOBYTECNT, 2);
			   MU_MB();
			   
			   dwCount = 0;
            }
            else
            {
                dwDatum = MGC_Read32(pBase, bFifoOffset);

                for (i = 0; i < 4; i++)
                {
                    *pDest++ = ((dwDatum >> (i * 8)) & 0xFF);
                }

                dwCount -= 4;
            }
        }
    }
    else
    {
        /* word access if aligned */
        while (dwCount >= 4)
        {
            *((uint32_t *) ((void *) pDest)) =
                MGC_Read32(pBase, bFifoOffset);

            pDest += 4;
            dwCount -= 4;
        }

        if (dwCount > 0)
        {
		    if(3 == dwCount ||2 == dwCount )
		    {
		       MU_MB();
               MGC_FIFO_CNT(pBase, M_REG_FIFOBYTECNT, 1);
			   MU_MB();
			   dwDatum = MGC_Read32(pBase, bFifoOffset);
			    for (i = 0; i < 2; i++)
         		{
         			*pDest++ = ((dwDatum >> (i *8)) & 0xFF);
         		}
				dwCount -= 2;
			}

			if(1 == dwCount)
			{
			   MU_MB();
               MGC_FIFO_CNT(pBase, M_REG_FIFOBYTECNT,0);
			   MU_MB();
			   dwDatum = MGC_Read32(pBase, bFifoOffset);
			   
			    *pDest++ = (dwDatum & 0xFF);
				dwCount -= 1;			   
			}
			
    		// set FIFO byte count = 4 bytes.
    		MU_MB();
    		MGC_FIFO_CNT(pBase, M_REG_FIFOBYTECNT, 2);
			MU_MB();
        }
    }

    return;
}
void musb_read_fifo(struct musb_hw_ep *hw_ep, u16 len, u8 *dst)
{
    uint8_t bEnd=hw_ep->epnum;
    uint8_t bFifoOffset = MGC_FIFO_OFFSET(bEnd);
    uint8_t *pBase=hw_ep->fifo-bFifoOffset;
    uint32_t temp;
    temp=len;

    MGC_UnloadFifo_Musb(pBase, bEnd,len,dst);
}
void musb_write_fifo(struct musb_hw_ep *hw_ep, u16 len, const u8 *src)
{
    uint8_t  bEnd=hw_ep->epnum;
    volatile uint8_t bFifoOffset = MGC_FIFO_OFFSET(bEnd);
     uint8_t *pBase=hw_ep->fifo-bFifoOffset;
	
    MGC_LoadFifo_Musb(pBase, bEnd,len, src);
}
