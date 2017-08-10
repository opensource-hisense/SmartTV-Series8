/*
 * linux/arch/arm/mach-mt53xx/core.c
 *
 * CPU core init - irq, time, baseio
 *
 * Copyright (c) 2010-2012 MediaTek Inc.
 * $Author: tiangang.ran $
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
 */

/* system header files */
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/bootmem.h>
#include <linux/slab.h>
#include <linux/seq_file.h>
#include <linux/sched.h>
#include <mach/hardware.h>
#include <mach/mt53xx_linux.h>
#include <mach/x_hal_io.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <asm/setup.h>
#include <asm/mach-types.h>
#include <asm/pgtable.h>
#include <asm/page.h>
#include <asm/uaccess.h>

#include <asm/mach/arch.h>
#include <asm/mach/irq.h>
#include <asm/mach/map.h>

#include <asm/mach/time.h>
#include <asm/cacheflush.h>
#include <asm/tlbflush.h>

//#include <asm/pmu.h>
#include <linux/platform_device.h>
#include <linux/memblock.h>

// NDC patch start
#include <linux/random.h>
#include <linux/net.h>
#include <asm/uaccess.h>
// NDC patch stop
#if defined(CC_ANDROID_FASTBOOT_SUPPORT) || defined(CC_ANDROID_RECOVERY_SUPPORT)
#include <linux/mmc/mtk_msdc_part.h>
#endif

#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/of_address.h>

// PCI related header file.
#include <linux/pci.h>
#include <asm/mach/pci.h>
#ifdef CONFIG_PCI
#include <plat/mtk_pci.h>
#endif

#ifdef CONFIG_NATIVE_CB2
#include <linux/semaphore.h>

#include <linux/suspend.h>
#include <mach/cb_data.h>
#include <mach/cb_low.h>
extern void _CB_Init(void);
#endif

#ifdef CONFIG_ARM_GIC
//#include <asm/hardware/gic.h>
#include <linux/irqchip/arm-gic.h>
static void (*gic_irq_mask)(struct irq_data *data) = NULL;
static void (*gic_irq_unmask)(struct irq_data *data) = NULL;
static void (*gic_irq_eoi)(struct irq_data *data) = NULL;
#endif

#include <linux/platform_device.h>

#define CONFIG_BOOT_TIME
#ifdef ANDROID
//#include <linux/usb/android_composite.h>
#include <linux/platform_device.h>
#endif

/* For Memory Size */
#define TCM_SRAM_ADDR       (pBimIoMap + 0x100)
#define TCM_DRAM_SIZE      (*((volatile unsigned long *)(TCM_SRAM_ADDR - 0x4)))

#ifndef CONFIG_SMP
void mt53xx_resume_up_check(void);
EXPORT_SYMBOL(mt53xx_resume_up_check);
#endif
#if defined(CONFIG_HIBERNATION) || defined(CONFIG_OPM)
#ifdef CONFIG_SMP
void arch_disable_nonboot_cpus_early_resume(void);
EXPORT_SYMBOL(arch_disable_nonboot_cpus_early_resume);
#endif
#endif
DEFINE_SPINLOCK(mt53xx_bim_lock);
EXPORT_SYMBOL(mt53xx_bim_lock);

void __iomem *pBimIoMap;
void __iomem *pPdwncIoMap;
void __iomem *pCkgenIoMap;
void __iomem *pDramIoMap;
void __iomem *pMcuIoMap;
#ifdef CONFIG_HAVE_ARM_SCU
void __iomem *pScuIoMap;
#endif

unsigned int mt53xx_cha_mem_size __read_mostly;
static unsigned long mt53xx_reserve_start __read_mostly, mt53xx_reserve_size __read_mostly;
static spinlock_t rtimestamp_lock;

void mt53xx_get_reserved_area(unsigned long *start, unsigned long *size)
{
    *start = mt53xx_reserve_start;
    *size = mt53xx_reserve_size;
}
EXPORT_SYMBOL(mt53xx_get_reserved_area);

/*======================================================================
 * irq
 */

void __pdwnc_reboot_in_kernel(void)
{
        #ifdef CC_S_PLATFORM
        __pdwnc_writel(0x7ff00000, 0x104); /* wait for a while, instead of triggering immediately. */
        #else
        __pdwnc_writel(0x7fff0000, 0x104);
        #endif
        __pdwnc_writel((__pdwnc_readl(0x100) & 0x00ffffff) | 0x01, 0x100);
}

static inline int _bim_is_vector_valid(u32 irq_num)
{
    return (irq_num < NR_IRQS);
}


#ifdef CONFIG_MT53XX_NATIVE_GPIO
#define MUC_NUM_MAX_CONTROLLER      (4)
int MUC_aPwrGpio[MUC_NUM_MAX_CONTROLLER]= {0} ;
int MUC_aPwrPolarity[MUC_NUM_MAX_CONTROLLER] = {0};
int MUC_aOCGpio[MUC_NUM_MAX_CONTROLLER] = {0} ;
int MUC_aOCPolarity[MUC_NUM_MAX_CONTROLLER] = {0};
int MUC_aPortUsing[MUC_NUM_MAX_CONTROLLER] = {0};
//int MUC_aCdcSupport[MUC_NUM_MAX_CONTROLLER] = {0};
EXPORT_SYMBOL(MUC_aPwrGpio);
EXPORT_SYMBOL(MUC_aPwrPolarity);
EXPORT_SYMBOL(MUC_aOCGpio);
EXPORT_SYMBOL(MUC_aOCPolarity);
EXPORT_SYMBOL(MUC_aPortUsing);
//EXPORT_SYMBOL(MUC_aCdcSupport);

static int __init MGC_CommonParamParsing(char *str, int *pi4Dist,int *pi4Dist2)
{
    char tmp1[8]={0};
    char tmp2[8]={0};
    char *p1,*p2,*s;
    int len1,len2,i,j,k;
    if(strlen(str) != 0)
    {
        //DBG(-2, "Parsing String = %s \n",str);
    }
    else
    {
        //DBG(-2, "Parse Error!!!!! string = NULL\n");
        return 0;
    }
	//parse usb gpio arg, format is: x:x,x:x,x:x,x:x, use -1 means no used
    for (i=0; i<MUC_NUM_MAX_CONTROLLER; i++)
    {
        s=str;
        if (i  !=  (MUC_NUM_MAX_CONTROLLER-1) )
        {
            if( (!(p1=strchr (str, ':'))) || (!(p2=strchr (str, ','))) )
            {
                //DBG(-2, "Parse Error!!string format error 1\n");
                break;
            }
            if( (!((len1=p1-s) >= 1)) || (!((len2=p2-p1-1) >= 1)) )
            {
                //DBG(-2, "Parse Error!! string format error 2\n");
                break;
            }
        }
        else
        {
			if(!(p1=strchr (str, ':')))
			{
				//DBG(-2, "Parse Error!!string format error 1\n");
				break;
			}
            if(!((len1=p1-s) >= 1))
            {
                //DBG(-2, "Parse Error!! string format error 2\n");
                break;
            }
			p2=p1+1;
            len2 = strlen(p2);
        }

        for(j=0;j<len1;j++)
		{
            tmp1[j]=*s;
            s++;
			//printk("tmp1[%d]=%c\n", j, tmp1[j]);
        }
        tmp1[j]=0;

        for(k=0;k<len2;k++)
		{
            tmp2[k]=*(p1+1+k);
			//printk("tmp2[%d]=%c\n", k, tmp2[k]);
        }
        tmp2[k]=0;

        pi4Dist[i] = (int)simple_strtol(&tmp1[0], NULL, 10);
		pi4Dist2[i] = (int)simple_strtol(&tmp2[0], NULL, 10);
        printk("Parse Done: usb port[%d] Gpio=%d, Polarity=%d \n", i, pi4Dist[i],pi4Dist2[i]);

        str += (len1+len2+2);
    }

    return 1;
}
static int __init MGC_PortUsingParamParsing(char *str, int *pi4Dist)
{
    char tmp[8]={0};
    char *p,*s;
    int len,i,j;
    if(strlen(str) != 0)
	{
        //DBG(-2, "Parsing String = %s \n",str);
    }
    else
	{
        //DBG(-2, "Parse Error!!!!! string = NULL\n");
        return 0;
    }

    for (i=0; i<MUC_NUM_MAX_CONTROLLER; i++)
	{
        s=str;
        if (i  !=  (MUC_NUM_MAX_CONTROLLER-1) )
		{
            if(!(p=strchr (str, ',')))
			{
                //DBG(-2, "Parse Error!!string format error 1\n");
                break;
            }
            if (!((len=p-s) >= 1))
            {
                //DBG(-2, "Parse Error!! string format error 2\n");
                break;
            }
        }
		else
        {
            len = strlen(s);
        }

        for(j=0;j<len;j++)
        {
            tmp[j]=*s;
            s++;
        }
        tmp[j]=0;

        pi4Dist[i] = (int)simple_strtol(&tmp[0], NULL, 10);
        printk("Parse Done: usb port[%d] Using =%d \n", i, pi4Dist[i]);

        str += (len+1);
    }

    return 1;
}

#if 0
static int __init MGC_CDCParamParsing(char *str, int *pi4Dist)
{
    char tmp[8]={0};
    char *p,*s;
    int len,i,j;
    if(strlen(str) != 0)
	{
        //DBG(-2, "Parsing String = %s \n",str);
    }
    else
	{
        //DBG(-2, "Parse Error!!!!! string = NULL\n");
        return 0;
    }

    for (i=0; i<MUC_NUM_MAX_CONTROLLER; i++)
	{
        s=str;
        if (i  !=  (MUC_NUM_MAX_CONTROLLER-1) )
		{
            if(!(p=strchr (str, ',')))
			{
                //DBG(-2, "Parse Error!!string format error 1\n");
                break;
            }
            if (!((len=p-s) >= 1))
            {
                //DBG(-2, "Parse Error!! string format error 2\n");
                break;
            }
        }
		else
        {
            len = strlen(s);
        }

        for(j=0;j<len;j++)
        {
            tmp[j]=*s;
            s++;
        }
        tmp[j]=0;

        pi4Dist[i] = (int)simple_strtol(&tmp[0], NULL, 10);
        printk("Parse Done: usb port[%d] Cdc support=%d \n", i, pi4Dist[i]);

        str += (len+1);
    }

    return 1;
}
#endif
static int __init MGC_PwrGpioParseSetup(char *str)
{
	//DBG(-2, "USB Power GPIO Port = .\n");
    return MGC_CommonParamParsing(str,&MUC_aPwrGpio[0],&MUC_aPwrPolarity[0]);
}
static int __init MGC_OCGpioParseSetup(char *str)
{
	//DBG(-2, "USB OC GPIO Port = .\n");
    return MGC_CommonParamParsing(str,&MUC_aOCGpio[0],&MUC_aOCPolarity[0]);
}
static int __init MGC_PortUsingParseSetup(char *str)
{
    //DBG(-2, "PortUsing = %s\n",str);
    return MGC_PortUsingParamParsing(str,&MUC_aPortUsing[0]);
}
#if 0
static int __init MGC_CdcSupportParseSetup(char *str)
{
    //DBG(-2, "CdcSupport = %s\n",str);
    return MGC_CDCParamParsing(str,&MUC_aCdcSupport[0]);
}
#endif
__setup("usbportusing=", MGC_PortUsingParseSetup);
__setup("usbpwrgpio=", MGC_PwrGpioParseSetup);
//__setup("usbpwrpolarity=", MGC_PwrPolarityParseSetup);
__setup("usbocgpio=", MGC_OCGpioParseSetup);
//__setup("usbocpolarity=", MGC_OCPolarityParseSetup);
//__setup("usbcdcsupport=", MGC_CdcSupportParseSetup);

#endif
unsigned int bim_default_on = 1;
EXPORT_SYMBOL(bim_default_on);


static int __init PrintBootReason(char *str)
{
    printk("Last boot reason: %s\n", str);
    return 1;
}
__setup("bootreason=", PrintBootReason);

#ifdef CONFIG_ARM_GIC
static void bim_irq_ack(struct irq_data *data)
{
    unsigned int irq = data->irq;

	if (irq == VECTOR_GDMA || irq == VECTOR_PCMCIA)
	{
		return;
	}

    if (!_bim_is_vector_valid(irq))
    {
        while (1)
            ;
        return;
    }

#ifdef CC_VECTOR_HAS_MISC3
    if (irq >= VECTOR_MISC3_BASE)
    {
        u32 misc_irq;
        misc_irq = _MISC3IRQ(irq);
        //spin_lock_irqsave(&mt53xx_bim_lock, flags);
        __bim_writel(misc_irq, REG_RW_M3INTCLR);
        //spin_unlock_irqrestore(&mt53xx_bim_lock, flags);
    }
    else
#endif // CC_VECTOR_HAS_MISC3
    if (irq >= VECTOR_MISC2_BASE)
    {
        u32 misc_irq;
        misc_irq = _MISC2IRQ(irq);
        //spin_lock_irqsave(&mt53xx_bim_lock, flags);
        __bim_writel(misc_irq, REG_RW_M2INTCLR);
        //spin_unlock_irqrestore(&mt53xx_bim_lock, flags);
    }
    else if (irq >= VECTOR_MISC_BASE)
    {
        u32 misc_irq;
        misc_irq = _MISCIRQ(irq);
        //spin_lock_irqsave(&mt53xx_bim_lock, flags);
        if (irq != VECTOR_DRAMC)
        {
            __bim_writel(misc_irq, REG_RW_MINTCLR);
        }
        //spin_unlock_irqrestore(&mt53xx_bim_lock, flags);
    }
    else
    {
        //spin_lock_irqsave(&mt53xx_bim_lock, flags);
        if (irq != VECTOR_DRAMC)
        {
            __bim_writel((1 << (irq-SPI_OFFSET)), REG_RW_IRQCLR);
        }
        //spin_unlock_irqrestore(&mt53xx_bim_lock, flags);
    }
}
#endif

static void mt53xx_irq_mask_ack(struct irq_data *data)
{
#ifndef CONFIG_ARM_GIC
   u32 regval32;
#endif
    unsigned long flags;
    unsigned int irq = data->irq;

    if (!_bim_is_vector_valid(irq))
    {
        while (1)
            ;
        return;
    }

#ifdef CONFIG_ARM_GIC
    gic_irq_mask(data);
    gic_irq_eoi(data);
#endif
#ifdef CC_VECTOR_HAS_MISC3
    if (irq >= VECTOR_MISC3_BASE)
    {
        u32 misc_irq;
        misc_irq = _MISC3IRQ(irq);
        spin_lock_irqsave(&mt53xx_bim_lock, flags);
        #ifndef CONFIG_ARM_GIC
        regval32 = __bim_readl(REG_RW_M3INTEN);
        regval32 &= ~misc_irq;
        __bim_writel(regval32, REG_RW_M3INTEN);
        #endif
        __bim_writel(misc_irq, REG_RW_M3INTCLR);
        spin_unlock_irqrestore(&mt53xx_bim_lock, flags);
    }
    else
#endif // CC_VECTOR_HAS_MISC3
    if (irq >= VECTOR_MISC2_BASE)
    {
        u32 misc_irq;
        misc_irq = _MISC2IRQ(irq);
        spin_lock_irqsave(&mt53xx_bim_lock, flags);
        #ifndef CONFIG_ARM_GIC
        regval32 = __bim_readl(REG_RW_M2INTEN);
        regval32 &= ~misc_irq;
        __bim_writel(regval32, REG_RW_M2INTEN);
        #endif

        __bim_writel(misc_irq, REG_RW_M2INTCLR);
        spin_unlock_irqrestore(&mt53xx_bim_lock, flags);
    }
    else if (irq >= VECTOR_MISC_BASE)
    {
        u32 misc_irq;
        misc_irq = _MISCIRQ(irq);
        spin_lock_irqsave(&mt53xx_bim_lock, flags);
        #ifndef CONFIG_ARM_GIC
        regval32 = __bim_readl(REG_RW_MINTEN);
        regval32 &= ~misc_irq;
        __bim_writel(regval32, REG_RW_MINTEN);
        #endif

        if (irq != VECTOR_DRAMC)
        {
            __bim_writel(misc_irq, REG_RW_MINTCLR);
        }
        spin_unlock_irqrestore(&mt53xx_bim_lock, flags);
    }
    else
    {
        spin_lock_irqsave(&mt53xx_bim_lock, flags);
        #ifndef CONFIG_ARM_GIC
        regval32 = __bim_readl(REG_RW_IRQEN);
        regval32 &= ~(1 << (irq-SPI_OFFSET));
        __bim_writel(regval32, REG_RW_IRQEN);
        #endif
        if (irq != VECTOR_DRAMC)
        {
            __bim_writel((1 << (irq-SPI_OFFSET)), REG_RW_IRQCLR);
        }
        spin_unlock_irqrestore(&mt53xx_bim_lock, flags);
    }
}

static void mt53xx_irq_mask(struct irq_data *data)
{
#ifndef CONFIG_ARM_GIC
    u32 regval32;
    unsigned long flags;
#endif    
    unsigned int irq = data->irq;

    if (!_bim_is_vector_valid(irq))
    {
        while (1)
            ;
        return;
    }

#ifdef CONFIG_ARM_GIC
    gic_irq_mask(data);
#else
#ifdef CC_VECTOR_HAS_MISC3
    if (irq >= VECTOR_MISC3_BASE)
    {
        u32 misc_irq;
        misc_irq = _MISC3IRQ(irq);
        spin_lock_irqsave(&mt53xx_bim_lock, flags);
        regval32 = __bim_readl(REG_RW_M3INTEN);
        regval32 &= ~misc_irq;
		if (!bim_default_on)
		{
        __bim_writel(regval32, REG_RW_M3INTEN);
		}
        spin_unlock_irqrestore(&mt53xx_bim_lock, flags);
    }
    else
#endif // CC_VECTOR_HAS_MISC3
    if (irq >= VECTOR_MISC2_BASE)
    {
        u32 misc_irq;
        misc_irq = _MISC2IRQ(irq);
        spin_lock_irqsave(&mt53xx_bim_lock, flags);
        regval32 = __bim_readl(REG_RW_M2INTEN);
        regval32 &= ~misc_irq;
		if (!bim_default_on)
		{
          __bim_writel(regval32, REG_RW_M2INTEN);
		}
        spin_unlock_irqrestore(&mt53xx_bim_lock, flags);
    }
    else if (irq >= VECTOR_MISC_BASE)
    {
        u32 misc_irq;
        misc_irq = _MISCIRQ(irq);
        spin_lock_irqsave(&mt53xx_bim_lock, flags);
        regval32 = __bim_readl(REG_RW_MINTEN);
        regval32 &= ~misc_irq;
		if (!bim_default_on)
		{
          __bim_writel(regval32, REG_RW_MINTEN);
		}
        spin_unlock_irqrestore(&mt53xx_bim_lock, flags);
    }
    else
    {
        spin_lock_irqsave(&mt53xx_bim_lock, flags);
        regval32 = __bim_readl(REG_RW_IRQEN);
        regval32 &= ~(1 << (irq-SPI_OFFSET));
		if (!bim_default_on)
		{
		  __bim_writel(regval32, REG_RW_IRQEN);
		}
        spin_unlock_irqrestore(&mt53xx_bim_lock, flags);
    }
#endif    
}

static void mt53xx_irq_unmask(struct irq_data *data)
{
#ifndef CONFIG_ARM_GIC
    u32 regval32;
    unsigned long flags;
    unsigned int irq = data->irq;

#ifdef CC_VECTOR_HAS_MISC3
    if (irq >= VECTOR_MISC3_BASE)
    {
        u32 misc_irq;
        misc_irq = _MISC3IRQ(irq);
        spin_lock_irqsave(&mt53xx_bim_lock, flags);
        regval32 = __bim_readl(REG_RW_M3INTEN);
        regval32 |= (misc_irq);
        
	__bim_writel(regval32, REG_RW_M3INTEN);
       
	 spin_unlock_irqrestore(&mt53xx_bim_lock, flags);
    }
    else
#endif
    if (irq >= VECTOR_MISC2_BASE)
    {
        u32 misc_irq;
        misc_irq = _MISC2IRQ(irq);
        spin_lock_irqsave(&mt53xx_bim_lock, flags);
        regval32 = __bim_readl(REG_RW_M2INTEN);
        regval32 |= (misc_irq);
        
	__bim_writel(regval32, REG_RW_M2INTEN);
        
	spin_unlock_irqrestore(&mt53xx_bim_lock, flags);
    }
    else if (irq >= VECTOR_MISC_BASE)
    {
        u32 misc_irq;
        misc_irq = _MISCIRQ(irq);
        spin_lock_irqsave(&mt53xx_bim_lock, flags);
        regval32 = __bim_readl(REG_RW_MINTEN);
        regval32 |= (misc_irq);
        __bim_writel(regval32, REG_RW_MINTEN);
        spin_unlock_irqrestore(&mt53xx_bim_lock, flags);
    }
    else
    {
        spin_lock_irqsave(&mt53xx_bim_lock, flags);
        regval32 = __bim_readl(REG_RW_IRQEN);
        regval32 |= (1 << (irq-SPI_OFFSET));
        if (!bim_default_on)
        {
        __bim_writel(regval32, REG_RW_IRQEN);
	}
        spin_unlock_irqrestore(&mt53xx_bim_lock, flags);
    }
#else
    gic_irq_unmask(data);
#endif
}

static struct irq_chip mt53xx_irqchip =
{
    .irq_mask_ack = mt53xx_irq_mask_ack,
    .irq_mask     = mt53xx_irq_mask,
    .irq_unmask   = mt53xx_irq_unmask,
};


/* Get IC Version */

static inline int BSP_IsFPGA(void)
{
    unsigned int u4Val;

    /* If there is FPGA ID, it must be FPGA, too. */
    u4Val = __bim_readl(REG_RO_FPGA_ID);
    if (u4Val != 0) { return 1; }

    /* otherwise, it's not FPGA. */
    return 0;
}

unsigned int mt53xx_get_ic_version()
{
    unsigned int u4Version;

    if (BSP_IsFPGA())
        return 0;

#if defined(CONFIG_ARCH_MT5396)
    u4Version = __bim_readl(REG_RO_SW_ID);
    u4Version = __bim_readl(REG_RO_CHIP_ID);
    u4Version = ((u4Version & 0x0fffff00U) << 4) |
                ((u4Version & 0x000000ffU) << 0) |
                ((u4Version & 0xf0000000U) >> 20);
#elif defined(CONFIG_ARCH_MT5399) || defined(CONFIG_ARCH_MT5398) || defined(CONFIG_ARCH_MT5880) || defined(CONFIG_ARCH_MT5881) || defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5861) || defined(CONFIG_ARCH_MT5882)|| defined(CONFIG_ARCH_MT5883) || defined(CONFIG_ARCH_MT5891)
    u4Version = __raw_readl(pCkgenIoMap + REG_RO_SW_ID);
#else
#error Dont know how to get IC version.
#endif

    return u4Version;
}
EXPORT_SYMBOL(mt53xx_get_ic_version);



static struct resource mt53xx_pmu_resource[] = {
    [0] = {
        .start  = VECTOR_PMU0,
        .end    = VECTOR_PMU0,
        .flags  = IORESOURCE_IRQ,
    },
#if defined(CONFIG_SMP) && defined(VECTOR_PMU1)
    [1] = {
        .start  = VECTOR_PMU1,
        .end    = VECTOR_PMU1,
        .flags  = IORESOURCE_IRQ,
    },
#endif
};

static struct platform_device mt53xx_pmu_device = {
    .name       = "arm-pmu",
    .id         = -1,
    .resource   = mt53xx_pmu_resource,
    .num_resources  = ARRAY_SIZE(mt53xx_pmu_resource),
};

#define MTK_MTAL_DMA_MASK 0x1FFFFFFF
struct platform_device mt53xx_mtal_device = {
    .name       = "mtal_coherent_alloc",
    .id         = -1,
    .dev = {
        .coherent_dma_mask = MTK_MTAL_DMA_MASK,
        .dma_mask = &(mt53xx_mtal_device.dev.coherent_dma_mask),
    },

};
EXPORT_SYMBOL(mt53xx_mtal_device);

static void __init mt53xx_init_machine_irq(void)
{
    unsigned long flags;
    unsigned int irq;
    u32 u4Reg;

#ifdef CONFIG_ARM_GIC
    struct irq_chip *gic;
    gic_init(0, 29, (void __iomem *)MPCORE_GIC_DIST_PHY, (void __iomem *)MPCORE_GIC_CPU_PHY);

    gic = irq_get_chip(29);
    gic_irq_unmask = gic->irq_unmask;
    gic_irq_mask = gic->irq_mask;
    gic_irq_eoi = gic->irq_eoi;
    mt53xx_irqchip.irq_set_affinity=gic->irq_set_affinity;
#endif

    spin_lock_irqsave(&mt53xx_bim_lock, flags);

    /* turn all irq off */
    __bim_writel(0, REG_RW_IRQEN);
    __bim_writel(0, REG_RW_MINTEN);
    __bim_writel(0, REG_RW_M2INTEN);

    /* clear all pending irq */
    __bim_writel(0xffffffff, REG_RW_IRQCLR);
    __bim_writel(0xffffffff, REG_RW_IRQCLR);
    __bim_writel(0xffffffff, REG_RW_IRQCLR);
    __bim_writel(0xffffffff, REG_RW_IRQCLR);
    __bim_writel(0xffffffff, REG_RW_IRQCLR);
    __bim_writel(0xffffffff, REG_RW_MINTCLR);
    __bim_writel(0xffffffff, REG_RW_MINTCLR);
    __bim_writel(0xffffffff, REG_RW_MINTCLR);
    __bim_writel(0xffffffff, REG_RW_MINTCLR);
    __bim_writel(0xffffffff, REG_RW_MINTCLR);
    __bim_writel(0xffffffff, REG_RW_M2INTCLR);
    __bim_writel(0xffffffff, REG_RW_M2INTCLR);
    __bim_writel(0xffffffff, REG_RW_M2INTCLR);
    __bim_writel(0xffffffff, REG_RW_M2INTCLR);
    __bim_writel(0xffffffff, REG_RW_M2INTCLR);


    for (irq = SPI_OFFSET; irq < NR_IRQS; irq++)
    {
        irq_set_chip(irq, &mt53xx_irqchip);
        irq_set_handler(irq, handle_level_irq);
        set_irq_flags(irq, IRQF_VALID | IRQF_PROBE);
    }
    
    __bim_writel(0xc0000000, REG_RW_IRQEN);

    #ifdef CONFIG_ARM_GIC
            /*turn on all irq as default*/
    __bim_writel(0xFFFFFFFF, REG_RW_IRQEN);
    __bim_writel(0xFFFFFFFF, REG_RW_MINTEN);
    __bim_writel(0xFFFFFFFF, REG_RW_M2INTEN);
    #ifdef CC_VECTOR_HAS_MISC3
    __bim_writel(0xFFFFFFFF, REG_RW_M3INTEN);
    #endif
    #endif

    /* set pmu irq to level triggered */
    u4Reg = __bim_readl(REG_RW_MISC);
    u4Reg |= 0x100;
    __bim_writel(u4Reg, REG_RW_MISC);

    /* for some reason, people believe we have to do above */
    spin_unlock_irqrestore(&mt53xx_bim_lock, flags);
}

/* io map */

static struct map_desc mt53xx_io_desc[] __initdata =
{
    {IO_PHY, __phys_to_pfn(IO_PHY), IO_SIZE, MT_DEVICE},
#ifdef CONFIG_CACHE_L2X0
    {0xF2000000, __phys_to_pfn(0xF2000000), 0x1000, MT_DEVICE},
#endif
#ifdef CONFIG_OPM
    {0xFB004000, __phys_to_pfn(0xFB004000), 0x1000, MT_MEMORY_NONCACHED},
    {0xFB00B000, __phys_to_pfn(0xFB00B000), 0x1000, MT_MEMORY_ITCM},
#endif
    {DMX_SRAM_VIRT, __phys_to_pfn(0xFB005000), 0x1000, MT_DEVICE}, // ldr env temp copy
#ifdef CONFIG_CPU_V7
#ifdef CONFIG_HAVE_ARM_SCU
    {0xF2002000, __phys_to_pfn(0xF2002000), SZ_16K, MT_DEVICE},    // SCU
#endif
#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5891)
    {0xF2010000, __phys_to_pfn(0xF2010000), SZ_16K, MT_DEVICE},    // GIC
    {0xF2020000, __phys_to_pfn(0x0), SZ_16K, MT_DEVICE},             // Physical 0x0
    {0xF2090000, __phys_to_pfn(0xF2090000), 0x6000, MT_DEVICE},   // CCI
#endif
#if defined(CONFIG_ARCH_MT5882)|| defined(CONFIG_ARCH_MT5883)
	{0xF1000000, __phys_to_pfn(0xF1000000), SZ_16K, MT_DEVICE},    // GIC
	{0xF2020000, __phys_to_pfn(0x0), SZ_16K, MT_DEVICE},		   // Physical 0x0
#endif

    {0xF4000000, __phys_to_pfn(0xF4000000), SZ_16K, MT_DEVICE},    // ROM
#endif
#ifdef CONFIG_PCI
    /* use for PCI express module */
    {PCIE_MMIO_BASE, __phys_to_pfn(PCIE_MMIO_PHYSICAL_BASE), PCIE_MMIO_LENGTH, MT_DEVICE},
#endif /* CONFIG_PCI */
};

static void mt53xx_dynamic_map_io(void)
{
    pBimIoMap = ioremap(BIM_PHY,0x1000);
  	pPdwncIoMap = ioremap(PDWNC_PHY,0x1000);
	pCkgenIoMap = ioremap(CKGEN_PHY,0x1000);
    pDramIoMap = ioremap(DRAM_PHY,0x1000);
    pMcuIoMap = ioremap(MCU_PHY,0x1000);    
    #ifdef CONFIG_HAVE_ARM_SCU
    pScuIoMap = ioremap(MPCORE_SCU_PHY,0x1000);
    #endif    
}

#ifdef CC_TRUSTZONE_SUPPORT
/*
 * Pick out the trustzone size.
 */
static int tz_size=0x1000000; /*default 16M for svp support*/
static __init int early_tzsz(char*p)
{
    tz_size = memparse(p, NULL);
    return 0;
}
early_param("tzsz", early_tzsz);
#endif

#ifdef LIMIT_GPU_TO_ChannelX
unsigned int _gpustart = 0;
unsigned int _gpusize = 0;
unsigned int _gpuionsize = 0;

static int __init gpustart(char* str)
{
    get_option(&str, &_gpustart);
    return 0;
}
static int __init gpusize(char* str)
{
    get_option(&str, &_gpusize);
    return 0;
}
static int __init gpuionsize(char* str)
{
    get_option(&str, &_gpuionsize);
    return 0;
}

early_param("gpustart", gpustart);
early_param("gpusize", gpusize);
early_param("gpuionsize", gpuionsize);

int mt53xx_get_reserve_memory(unsigned int *start, unsigned int *size, unsigned int *ionsize)
{
    if (start)
    {
        *start = (unsigned int) (_gpustart);
	   printk("reserve memory start=0x%x, ", _gpustart);
    }

    if (size)
    {
        *size = (unsigned int) (_gpusize);
	 printk("size=0x%x, ", _gpusize);
    }

     if (ionsize)
    {
        *ionsize = (unsigned int) (_gpuionsize);
	 printk("ionsize=0x%x\n", _gpuionsize);
    }

    return 0;
}
EXPORT_SYMBOL(mt53xx_get_reserve_memory);


static void __init mt53xx_reserve(void)
{
    //printk("yjdbg) mt53xx_reserve\n");
    memblock_reserve(_gpustart, _gpusize);
    printk("_gpustart= 0x%x, _gpusize = 0x%x \n",_gpustart,_gpusize);
}
#endif

static void __init mt53xx_map_io(void)
{
    unsigned long hole, b0end;
    struct map_desc fbm_io_desc = {0, 0, 0, MT_MEMORY};  // For FBM mapping.
    unsigned long cha_mem_size;
#ifdef CONFIG_ZONE_DMA
    extern unsigned long arm_dma_zone_size;
#endif

    // MAP iotable first, so we can access BIM register.
    iotable_init(mt53xx_io_desc, ARRAY_SIZE(mt53xx_io_desc));

    mt53xx_dynamic_map_io();
    
    // Get ChA memory size. By looking up TCM, must do this after io map.
    mt53xx_cha_mem_size = cha_mem_size = ((TCM_DRAM_SIZE) & 0xFFFF) * 1024 * 1024;

#ifdef CONFIG_ZONE_DMA
    arm_dma_zone_size = mt53xx_cha_mem_size;
#endif

    // Check how to map FBM.
    // We prefer to map FBM early, because it will use section mapping
    // (page size=1M). This could reduce TLB miss and save some RAM (pte space)
    // Also, it can save vmlloc space if there's a hole (2 membank)
    //
    // Here's the rule:
    //
    // 1. If there's 2 bank, and 2nd bank start <= 512MB, then FBM is in the middle, just map it in place.
    // 2. If total memory(including FBM) <= 512MB, just map all memory after kenel.
    // 3. If 1st bank >= 512MB, don't do anything now.
    // 4. If trustzone enabled, don't mapping tz memory in mmu.

    if (meminfo.nr_banks >= 2)
    {
        b0end = meminfo.bank[0].start + meminfo.bank[0].size;
        hole = meminfo.bank[1].start - b0end;
        if (meminfo.bank[1].start <= (unsigned long)512*1024*1024 && hole)
        {
            mt53xx_reserve_start = b0end;
            mt53xx_reserve_size = hole;
        }
    }
    else
    {
        // Hueristic to guess how many memory we have and possible size of FBM.
        b0end = meminfo.bank[0].start + meminfo.bank[0].size;
        if (cha_mem_size <= 512*1024*1024 && b0end < cha_mem_size)
        {
            mt53xx_reserve_start = b0end;
            mt53xx_reserve_size = cha_mem_size - b0end;
        }
    }

#ifdef CC_TRUSTZONE_SUPPORT
    if(mt53xx_reserve_size)
     {
        mt53xx_reserve_size -= tz_size;

        // fbm end addr can be non-align with PGDIR implementation
        if(((mt53xx_reserve_start + mt53xx_reserve_size) & (~PGDIR_MASK)) != 0)
        {
            mt53xx_reserve_start = 0;
            mt53xx_reserve_size = 0;
        }
    }
#endif

    if (mt53xx_reserve_start)
    {
        struct map_desc *desc = &fbm_io_desc;

        desc->virtual = (unsigned long)phys_to_virt(mt53xx_reserve_start);
        desc->pfn = __phys_to_pfn(mt53xx_reserve_start);
        desc->length = mt53xx_reserve_size;
        iotable_init(desc, 1);
    }

#ifdef CONFIG_USE_OF
    // temp solution: move to IRQ init
    __bim_writel(0xFFFFFFFF, REG_IRQEN);
    __bim_writel(0xFFFFFFFF, REG_RW_MINTEN);
    __bim_writel(0xFFFFFFFF, REG_RW_M2INTEN);
    __bim_writel(0xFFFFFFFF, REG_RW_M3INTEN);
#endif
#ifdef CONFIG_ARM_GIC
        gic_arch_extn.irq_eoi = bim_irq_ack;
#endif
}

extern char _text, _etext;
int mt53xx_get_rodata_addr(unsigned int *start, unsigned int *end)
{
    if (start)
    {
        *start = (unsigned int) (&_text);;
    }

    if (end)
    {
        *end = (unsigned int) (&_etext);
    }

    return 0;
}
EXPORT_SYMBOL(mt53xx_get_rodata_addr);

extern char _data, _end;
int mt53xx_get_rwdata_addr(unsigned int *start, unsigned int *end)
{
    if (start)
    {
        *start = (unsigned int) (&_data);
    }

    if (end)
    {
        *end = (unsigned int) (&_end);
    }

    return 0;
}
EXPORT_SYMBOL(mt53xx_get_rwdata_addr);

int (*query_tvp_enabled)(void)=NULL;
EXPORT_SYMBOL(query_tvp_enabled);

// Print imprecise abort debug message according to BIM debug register
int mt53xx_imprecise_abort_report(void)
{
    u32 iobus_tout = __bim_readl(REG_IOBUS_TOUT_DBG);
    u32 dramf_tout = __bim_readl(REG_DRAMIF_TOUT_DBG);
    int print = 0;

    if (iobus_tout & 0x10000000)
    {
        print = 1;
        printk(KERN_ALERT "IOBUS %s addr 0xf00%05x timeout\n",
              (iobus_tout & 0x8000000) ? "write" : "read", iobus_tout & 0xfffff);
    }

    if (dramf_tout & 0x10)
    {
        print = 1;
        printk(KERN_ALERT "DRAM %s addr 0x%08x timeout\n",
              (dramf_tout & 0x8) ? "write" : "read", __bim_readl(REG_DRAMIF_TOUT_ADDR));
    }

	//printk(KERN_ERR "query_tvp_enabled=0x%x,query_tvp_enabled=0x%x\n",(unsigned int)query_tvp_enabled,query_tvp_enabled());

	if(query_tvp_enabled && query_tvp_enabled())
	{
		//printk(KERN_ALERT "MT53xx unknown imprecise abort\n");
		return 1;
	}
	else
	{
    	if (!print)
        	printk(KERN_ALERT "MT53xx unknown imprecise abort\n");
	}
	return 0;
}

#if defined(CC_CMDLINE_SUPPORT_BOOTREASON)
static int panic_event(struct notifier_block *this, unsigned long event,
	 void *ptr)
{
	printk("mt53xx panic_event\n");
	// fill the reboot code in pdwnc register
	#if defined(CONFIG_MACH_MT5890) || defined(CONFIG_ARCH_MT5891)
	__raw_writeb(0x01,pPdwncIoMap + REG_RW_RESRV5);
	#endif
	return NOTIFY_DONE;
}

static struct notifier_block panic_block = {
	.notifier_call	= panic_event,
};

static int __init mt53xx_panic_setup(void)
{
	printk("mt53xx_panic_setup\n");
	/* Setup panic notifier */
	atomic_notifier_chain_register(&panic_notifier_list, &panic_block);

	return 0;
}
postcore_initcall(mt53xx_panic_setup);
#endif

/*****************************************************************************
 * PCI
 ****************************************************************************/
#ifdef CONFIG_PCI
static struct hw_pci mtk_pci __initdata = {
    .nr_controllers = 1,
    .swizzle    = pci_std_swizzle,
    .setup      = MTK_pci_sys_setup,
    .scan       = MTK_pci_sys_scan,
    .map_irq    = MTK_pci_map_irq
};

static int __init mtk_pci_init(void)
{
    pcibios_min_io = 0;
    pcibios_min_mem = PCIE_MMIO_LENGTH;

    pci_common_init(&mtk_pci);

    return 0;
}

subsys_initcall(mtk_pci_init);
#endif /* CONFIG_PCI */

/*****************************************************************************
 * RTC
 ****************************************************************************/

#ifdef CONFIG_RTC_DRV_MT53XX
static struct resource mt53xx_rtc_resource[] = {
    [0] = {
//        .start  = VECTOR_PDWNC,
//        .end    = VECTOR_PDWNC,
        .start  = -1,//TODO
        .end    = -1,
        .flags  = IORESOURCE_IRQ,
    },
};
static struct platform_device mt53xx_rtc_device = {
	.name		= "mt53xx_rtc",
	.id		= 0,
    .resource   = mt53xx_rtc_resource,
    .num_resources  = ARRAY_SIZE(mt53xx_rtc_resource),
};

static void __init mt53xx_add_device_rtc(void)
{
	platform_device_register(&mt53xx_rtc_device);
}
#else
static void __init mt53xx_add_device_rtc(void) {}
#endif

#ifdef CONFIG_KEYBOARD_HID
static struct platform_device mt53xx_hid_dev = {
	.name = "hid-keyboard",
	.id = -1,
};

static void __init mt53xx_add_device_hid(void)
{
	platform_device_register(&mt53xx_hid_dev);
}
#else
static void __init mt53xx_add_device_hid(void) {}
#endif

#ifdef CONFIG_BOOT_TIME
extern u32 _CKGEN_GetXtalClock(void);
// TODO.enclosing.by.define; enable it in debug version
#define MAX_SW_TIMESTAMP_SIZE   (1024)
typedef struct _TIME_STAMP
{
    unsigned int u4TimeStamp;
    unsigned char* szString;
} TIME_STAMP_T;
static unsigned int _u4TimeStampSize = 0;
static TIME_STAMP_T _arTimeStamp[MAX_SW_TIMESTAMP_SIZE];
TIME_STAMP_T * x_os_drv_get_timestampKernel(int *pu4Size)
{
    *pu4Size = _u4TimeStampSize;
    return _arTimeStamp;
}
EXPORT_SYMBOL(x_os_drv_get_timestampKernel);


#if 0
int _CmdDisplayTimeItem(unsigned int u4Timer, unsigned char* szString)
{
    unsigned int u4Val;

    u4Val = ((~u4Timer)/(_CKGEN_GetXtalClock()/1000000));  // us time.
    printf("0x%08x | %6d.%03d ms - %s\n", (unsigned int)u4Timer, (int)(u4Val / 1000), (int)(u4Val % 1000), szString);
}
#endif
/*
static int show_boottime(char *buf, char **start, off_t offset,
	                            int count, int *eof, void *data)
*/
static int show_boottime(struct seq_file *m, void *v)
{
    unsigned int i, u4Size;
    //int l, len = 0;
    //char *pos=buf;
    TIME_STAMP_T *prTimeStamp;
    unsigned int u4Timer, u4Val;

    u4Size = _u4TimeStampSize;
    prTimeStamp = _arTimeStamp;
#if 0 //todo...how to port??
    if(offset>0)
    {
	*eof=1;
	return 0;
    }
#endif
    for (i=0; i<u4Size; i++)
    {
        u4Timer = prTimeStamp[i].u4TimeStamp;
        u4Val = ((~u4Timer)/(_CKGEN_GetXtalClock()/1000000));  // us time.
        seq_printf(m, "0x%08x | %6d.%03d ms - %s\n", (unsigned int)u4Timer, (int)(u4Val / 1000), (int)(u4Val % 1000), prTimeStamp[i].szString);
    }
    return 0;
}

static int store_timestamp(struct file *file, const char *buffer,
	                            size_t count, loff_t *pos)
{
#define TAGSTRING_SIZE 256
    char* buf;
    unsigned long len = (count > TAGSTRING_SIZE-1) ? TAGSTRING_SIZE-1 : count;
    spin_lock(&rtimestamp_lock);
    if (_u4TimeStampSize < MAX_SW_TIMESTAMP_SIZE)
    {
        buf = kmalloc(TAGSTRING_SIZE, GFP_KERNEL);
	if (!buf) {
	    printk(KERN_ERR
		    "store_timestamp no buffer \n");
		spin_unlock(&rtimestamp_lock);
	    return 0;
	}
        if (copy_from_user(buf, buffer, len))
	{
	    kfree(buf);
		spin_unlock(&rtimestamp_lock);
            return count;
	}
        buf[len] = 0;
        _arTimeStamp[_u4TimeStampSize].u4TimeStamp = __bim_readl(REG_RW_TIMER2_LOW); //BIM_READ32(REG_RW_TIMER2_LOW)
        _arTimeStamp[_u4TimeStampSize].szString = buf;
        _u4TimeStampSize++;
        spin_unlock(&rtimestamp_lock);

        return strnlen(buf, count);
    }
    spin_unlock(&rtimestamp_lock);
    return 0;
}

void add_timestamp(char* buffer)
{
    char* buf;
    unsigned long count = strlen(buffer);
    unsigned long len = (count > TAGSTRING_SIZE-1) ? TAGSTRING_SIZE-1 : count;
	spin_lock(&rtimestamp_lock);
    if (_u4TimeStampSize < MAX_SW_TIMESTAMP_SIZE)
    {
        buf = kmalloc(TAGSTRING_SIZE, GFP_KERNEL);
	if (!buf) {
	    printk(KERN_ERR
		    "add_timestamp no buffer \n");
		spin_unlock(&rtimestamp_lock);
	    return;
	}
	memcpy(buf, buffer, len);
        buf[len] = 0;
        _arTimeStamp[_u4TimeStampSize].u4TimeStamp = __bim_readl(REG_RW_TIMER2_LOW); //BIM_READ32(REG_RW_TIMER2_LOW)
        _arTimeStamp[_u4TimeStampSize].szString = buf;
        _u4TimeStampSize++;
        spin_unlock(&rtimestamp_lock);
        return;
    }

    spin_unlock(&rtimestamp_lock);
    return;
}
EXPORT_SYMBOL(add_timestamp);

void free_builtin_timestamp(void)
{
    unsigned int i, u4Size;
    TIME_STAMP_T *prTimeStamp;

    spin_lock(&rtimestamp_lock);
    u4Size = _u4TimeStampSize;
    prTimeStamp = _arTimeStamp;
    for (i=0; i<u4Size; i++)
    {
	kfree(prTimeStamp[i].szString);
    }
    _u4TimeStampSize = 0;
	spin_unlock(&rtimestamp_lock);
}
EXPORT_SYMBOL(free_builtin_timestamp);
#endif
unsigned long dump_reboot_info = 0; 
static int __init dumprebootinfo(char *str)
{
    printk("dump reboot info: %s\n", str);
    dump_reboot_info = (int)simple_strtol(str, NULL, 10);
    return 1;
}
__setup("dumpreboot=", dumprebootinfo);
/*****************************************************************************
 * 53xx init
 ****************************************************************************/
static void mt53xx_restart(char mode, const char *cmd)
{
	if (dump_reboot_info)
	{
		dump_stack();
		printk("reboot by thread pid: %d\n", current->pid);
		if (current->comm)
		{
			printk("reboot by thread name: %s\n", current->comm);
		}
		if (dump_reboot_info & 0x2)
		{
		    BUG();
		}
	}
	/*
	 * use powerdown watch dog to reset system
	 * __raw_writel( 0xff000000, BIM_BASE + WATCHDOG_TIMER_COUNT_REG );
	 * __raw_writel( 1, BIM_BASE + WATCHDOG_TIMER_CTRL_REG);
	 */
	 
	#ifndef CC_ANDROID_REBOOT
	 #ifdef CC_ANDROID_FASTBOOT_SUPPORT
		if(cmd && !memcmp(cmd, "bootloader", 10))
			{
				int u4Val = msdc_partwrite_byname("misc",512,sizeof("fastboot"),(void *)"fastboot");
				if(u4Val == -1)
				printk("\n writer misc partition failed \n");
			}
	#endif
    #ifdef CC_ANDROID_RECOVERY_SUPPORT
    if(cmd && !memcmp(cmd, "recovery", 8))
    {
       int u4Val = msdc_partwrite_byname("misc",0,sizeof("boot-recovery"),(void *)"boot-recovery");
       if(u4Val == -1)
        printk("\n writer misc partition for recovery failed \n");
    }
    #endif
	#endif
	#if defined(CC_CMDLINE_SUPPORT_BOOTREASON)
		#if defined(CONFIG_MACH_MT5890) || defined(CONFIG_ARCH_MT5891)
		__raw_writel((__raw_readl(pPdwncIoMap + REG_RW_RESRV5) & 0xFFFFFF00) | 0x02,pPdwncIoMap + REG_RW_RESRV5);
		#endif
	#endif
	 __raw_writel( 0xffff0000, pPdwncIoMap + REG_RW_WATCHDOG_TMR );
	 __raw_writel( 1, pPdwncIoMap + REG_RW_WATCHDOG_EN);
}

#ifdef CC_SUPPORT_BL_DLYTIME_CUT
extern u32 _CKGEN_GetXtalClock(void);

int BIM_GetCurTime(void)
{
   unsigned long u4Time;
   int u4Val;
   u4Time = __bim_readl(REG_RW_TIMER2_LOW);
   u4Val = ((~u4Time)/(_CKGEN_GetXtalClock()/1000000));  // us time.
   printk("BIM_GetCurTime %d us \n", u4Val);
   return u4Val;
}
extern int __init mtk_gpio_init(void);
extern void mtk_gpio_set_value(unsigned gpio, int value);
extern int mtk_gpio_direction_output(unsigned gpio, int init_value);
static struct timer_list bl_timer;
static void restore_blTimer(unsigned long data);
static DEFINE_TIMER(bl_timer, restore_blTimer, 0, 0);

static void restore_blTimer(unsigned long data)
{
	u32 gpio, value;
	gpio = __bim_readl(REG_RW_GPRB0 + (5 << 2))&0xffff;
	value = __bim_readl(REG_RW_GPRB0 + (5 << 2))>>16;

	printk("[LVDS][backlight] Time is up!!!\n");
	mtk_gpio_direction_output(gpio, 1);
    mtk_gpio_set_value(gpio, value);
    del_timer(&bl_timer);
}
#endif /* end of CC_SUPPORT_BL_DLYTIME_CUT */

int x_kmem_sync_table(void *ptr, size_t size)
{
    pgd_t *pgd_k = init_mm.pgd;
    pgd_t *pgd = cpu_get_pgd();
    pud_t *pud, *pud_k;
    pmd_t *pmd, *pmd_k;
    unsigned int index;
    unsigned long addr = (unsigned long)(ptr), end = addr + size;

    if(addr >= MODULES_VADDR)//kernel space addr
    {
        while (addr < end)
        {
#if 0
            unsigned int index = pgd_index(addr);
            pmd_t *pmd = pmd_offset(pgd + index, addr);
            if (pmd_none(*pmd))
            {
                pmd_t *pmd_k = pmd_offset(pgd_k + index, addr);
                if (pmd_none(*pmd_k))
                    return -EINVAL;
                copy_pmd(pmd, pmd_k);
            }
#endif
            // refer to do_translation_fault
            index = pgd_index(addr);
            pgd = cpu_get_pgd() + index;
            pgd_k = init_mm.pgd + index;

            if (pgd_none(*pgd_k))
                goto bad_area;
            if (!pgd_present(*pgd))
                set_pgd(pgd, *pgd_k);

	    //__cpuc_flush_dcache_area((void*)pgd, 4);

            pud = pud_offset(pgd, addr);
            pud_k = pud_offset(pgd_k, addr);

            if (pud_none(*pud_k))
                goto bad_area;
            if (!pud_present(*pud))
                set_pud(pud, *pud_k);

            pmd = pmd_offset(pud, addr);
            pmd_k = pmd_offset(pud_k, addr);

            /*
             * On ARM one Linux PGD entry contains two hardware entries (see page
             * tables layout in pgtable.h). We normally guarantee that we always
             * fill both L1 entries. But create_mapping() doesn't follow the rule.
             * It can create inidividual L1 entries, so here we have to call
             * pmd_none() check for the entry really corresponded to address, not
             * for the first of pair.
             */
            index = (addr >> SECTION_SHIFT) & 1;

            if (pmd_none(pmd_k[index]))
                goto bad_area;

            copy_pmd(pmd, pmd_k);
	    __cpuc_flush_dcache_area((void*)pmd, 8);

            if (*pmd & 1)
            {
                unsigned long start = (*pmd) & ~0x7FF;
		__cpuc_flush_dcache_area(__va((void*)start), 4096);
                outer_clean_range(start, start + 4096);
            }
            addr = (addr + PMD_SIZE) & PMD_MASK;
        }
    }

    //outer_clean_range(__pa(pgd) + ((unsigned long)ptr >> 23 << 5), __pa(pgd) + ((end + 0x7FFFFF) >> 23 << 5));
    //outer_clean_range(__pa(pgd_k) + ((unsigned long)ptr >> 23 << 5), __pa(pgd_k) + ((end + 0x7FFFFF) >> 23 << 5));
    return 0;

bad_area:
    printk("x_kmem_sync_table fail\n");
    return 1;
}
EXPORT_SYMBOL(x_kmem_sync_table);

void *mt53xx_saved_core1;
EXPORT_SYMBOL(mt53xx_saved_core1);
//extern void mt53xx_proc_htimer_init(void);

static int bt_proc_open(struct inode *inode, struct file *file) {
     return single_open(file, show_boottime, PDE_DATA(inode));
}

static const struct file_operations bt_proc_fops = {
     .open    = bt_proc_open,
     .read    = seq_read,
     .llseek  = seq_lseek,
     .release = single_release,
     .write = store_timestamp,
};

#ifdef CONFIG_NATIVE_CB2 // CONFIG_PDWNC_SUSPEND_RESUME_CALLBACK
#if 0
static wait_queue_head_t PdwncQueue;

void init_pdwnc_queue(void)
{
    init_waitqueue_head(&PdwncQueue);
}

void wait_pdwnc_queue(void)
{
    DEFINE_WAIT(wait);
    prepare_to_wait(&PdwncQueue, &wait, TASK_UNINTERRUPTIBLE);
    //printk("(yjdbg) wait_pdwnc_queue-1\n");
    schedule();
    printk("(yjdbg) wait_pdwnc_queue-2\n");
    finish_wait(&PdwncQueue, &wait);
}

void wakeup_pdwnc_queue(void)
{
    printk("(yjdbg) wakeup_pdwnc_queue\n");
    wake_up_all(&PdwncQueue);
}
#else
struct semaphore pdwnc_mutex = __SEMAPHORE_INITIALIZER(pdwnc_mutex, 0);
void init_pdwnc_queue(void)
{
}

void wait_pdwnc_queue(void)
{
    printk("(yjdbg) wait_pdwnc_queue-1\n");
    down(&pdwnc_mutex);
    printk("(yjdbg) wait_pdwnc_queue-2\n");
}

void wakeup_pdwnc_queue(void)
{
    printk("(yjdbg) wakeup_pdwnc_queue\n");
    up(&pdwnc_mutex);
}
#endif

EXPORT_SYMBOL(wakeup_pdwnc_queue);
#endif

#ifdef CONFIG_NATIVE_CB2
typedef struct
{
    UINT32 u4Reason;
    UINT32 u4Other;
} MTPDWNC_RESUME_REASON_T;

typedef struct
{
    UINT32 u4PowerStateId;
    UINT32 u4Arg1;
    UINT32 u4Arg2;
    UINT32 u4Arg3;
    UINT32 u4Arg4;
} MTPDWNC_POWER_STATE_T;


#if 0
void PDWNC_PostSuspend(void)
{
    MTPDWNC_RESUME_REASON_T t_cb_info;

    printk("(yjdbg) PDWNC_PostSuspend\n");
    t_cb_info.u4Reason = 0x01234567;
    t_cb_info.u4Other = 0xA5A5A5A5;

    _CB_PutEvent(CB2_DRV_PDWNC_POST_SUSPEND_NFY, sizeof(MTPDWNC_RESUME_REASON_T), &t_cb_info);
    wait_pdwnc_queue();
}

void PDWNC_SuspendPrepare(void)
{
    MTPDWNC_RESUME_REASON_T t_cb_info;

    printk("(yjdbg)  PDWNC_SuspendPrepare\n");
    t_cb_info.u4Reason = 0x01234567;
    t_cb_info.u4Other = 0xA5A5A5A5;

    _CB_PutEvent(CB2_DRV_PDWNC_SUSPEND_PREPARE_NFY, sizeof(MTPDWNC_RESUME_REASON_T), &t_cb_info);
    wait_pdwnc_queue();
}
#endif

void PDWNC_PowerStateNotify(UINT32 u4PowerState, UINT32 u4Arg1, UINT32 u4Arg2, UINT32 u4Arg3, UINT32 u4Arg4)
{
    MTPDWNC_POWER_STATE_T t_cb_info;
    printk("(yjdbg)  PDWNC_PowerStateNotify (%d,%d,%d,%d,%d)\n", u4PowerState, u4Arg1, u4Arg2, u4Arg3, u4Arg4);

    t_cb_info.u4PowerStateId = u4PowerState;
    t_cb_info.u4Arg1         = u4Arg1;
    t_cb_info.u4Arg2         = u4Arg2;
    t_cb_info.u4Arg3         = u4Arg3;
    t_cb_info.u4Arg4         = u4Arg4;

    _CB_PutEvent(CB2_DRV_PDWNC_POWER_STATE, sizeof(MTPDWNC_POWER_STATE_T), &t_cb_info);
    wait_pdwnc_queue();
}

static int mtkcore_pm_notify(struct notifier_block *nb, unsigned long event,
	void *dummy)
{
    if (event == PM_SUSPEND_PREPARE) {
        //PDWNC_SuspendPrepare();
        PDWNC_PowerStateNotify(100,1,2,3,4);
    } else if (event == PM_POST_SUSPEND) {
        //PDWNC_PostSuspend();
        //PDWNC_PowerStateNotify(200,5,6,7,8);
    }
    return NOTIFY_OK;
}

static struct notifier_block mtkcore_pm_notifier = {
	.notifier_call = mtkcore_pm_notify,
};
#endif

#if defined(CC_DYNAMIC_IO_ADDRESS)
#ifdef CONFIG_OF
void __iomem *tv_reg_base;
static void  __init mt53xx_dt_init(void)
{
    struct device_node *np;

    np = of_find_compatible_node(NULL, NULL, "Mediatek, TVBIM");
    tv_reg_base = of_iomap(np, 0);
    printk("tv_reg_base: 0x%p\n", tv_reg_base);
}
EXPORT_SYMBOL(tv_reg_base);
#endif
#endif

#ifdef CONFIG_OF
#include <linux/of_platform.h>
#endif

static void  __init mt53xx_init_machine(void)
{
 #ifdef CONFIG_BOOT_TIME
   struct proc_dir_entry * proc_file __attribute__((unused)) = 0;
 #endif

#ifdef CC_SUPPORT_BL_DLYTIME_CUT
	unsigned int u4CurTime;
	u32 gpio, value, time;
	time = __bim_readl((REG_RW_GPRB0 + (4 << 2)));
	printk("[LVDS][backlight] TIME=%d us\n", time);
	mtk_gpio_init();
	if (time!=0)
	{
	  u4CurTime = BIM_GetCurTime();
	  gpio = __bim_readl(REG_RW_GPRB0 + (5 << 2))&0xffff;
	  value = __bim_readl(REG_RW_GPRB0 + (5 << 2))>>16;
	  if (u4CurTime >= time)
	  {
		  printk("[LVDS][backlight] #1\n");
		  mtk_gpio_direction_output(gpio, 1);
		  mtk_gpio_set_value(gpio, value);
	  }
	  else
	  {
		  printk("[LVDS][backlight] #2\n");
		  bl_timer.expires = jiffies + usecs_to_jiffies( abs(__bim_readl((REG_RW_GPRB0 + (4 << 2))) - u4CurTime));
		  add_timer(&bl_timer);
	  }
	}
#endif /* end of CC_SUPPORT_BL_DLYTIME_CUT */

   spin_lock_init((&rtimestamp_lock));
    platform_device_register(&mt53xx_pmu_device);
    platform_device_register(&mt53xx_mtal_device);

    mt53xx_add_device_rtc();

    mt53xx_add_device_hid();
  #ifdef CONFIG_BOOT_TIME
  /*    proc_file = create_proc_entry("boottime", S_IFREG | S_IWUSR, NULL); */
    proc_create("boottime", 0666, NULL, &bt_proc_fops);
#if 0
    proc_file = create_proc_entry("boottime", 0666, NULL);
    if (proc_file)
    {
	proc_file->read_proc = show_boottime;
	proc_file->write_proc = store_timestamp;
	proc_file->data = NULL;
        //proc_file->proc_fops = &_boottime_operations;
    }
#endif
  #endif

#ifdef CONFIG_OPM
    mt53xx_saved_core1 = kmalloc(256, GFP_KERNEL);
    memcpy(mt53xx_saved_core1, (void*)0xFB00BF00, 256);
#endif
#ifdef CONFIG_NATIVE_CB2
    init_pdwnc_queue();
    _CB_Init();
    register_pm_notifier(&mtkcore_pm_notifier);
#endif
#if defined(CC_DYNAMIC_IO_ADDRESS)
#ifdef CONFIG_OF
    //of_platform_populate(NULL, of_default_bus_match_table, NULL, NULL);
    mt53xx_dt_init();
#endif
#endif

	//Register simple bus
#ifdef CONFIG_OF
	of_platform_populate(NULL, of_default_bus_match_table, NULL, NULL);
#endif

}

extern void __init mt53xx_timer_init(void);
extern struct smp_operations mt53xx_smp_ops;


#ifdef CONFIG_CACHE_L2X0
#include <asm/hardware/cache-l2x0.h>

extern void _l2x0_init(void __iomem *base, u32 aux_val, u32 aux_mask);
#ifdef CONFIG_CPU_V7
static int mt53xx_l2_cache_init(void)
{
/*
[30] Early BRESP enable 0 = Early BRESP disabled. This is the default. 1 = Early BRESP enabled.
[29] Instruction prefetch enable 0 = Instruction prefetching disabled. This is the default. 1 = Instruction prefetching enabled.
[28] Data prefetch enable 0 = Data prefetching disabled. This is the default. 1 = Data prefetching enabled.
[24:23] Force write allocate b00 = Use AWCACHE attributes for WA. This is the default.
b01 = Force no allocate, set WA bit always 0.
b10 = Override AWCACHE attributes, set WA bit always 1, all cacheable write misses become write allocated.
b11 = Internally mapped to 00.
[12] Exclusive cache configuration 0 = Disabled. This is the default. 1 = Enabled
[0] Full Line of Zero Enable 0 = Full line of write zero behavior disabled. This is the default. 1 = Full line of write zero behavior
normal:0xf2000104=0x73041001
benchmark:0xf2000104=0x72040001
*/
    register unsigned int reg;
    unsigned l2val = 0x71000001;
    unsigned l2mask = 0x8e7ffffe;
    void __iomem *l2x0_base = (void __iomem *)L2C_BASE;

	/*
	 * Check if l2x0 controller is already enabled.
	 * If you are booting from non-secure mode
	 * accessing the below registers will fault.
	 */
	if (!(readl_relaxed(l2x0_base + L2X0_CTRL) & 1))
    {
#if defined(CONFIG_ARCH_MT5399) || defined(CONFIG_ARCH_MT5398) || defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5861)|| defined(CONFIG_ARCH_MT5882)|| defined(CONFIG_ARCH_MT5883) || defined(CONFIG_ARCH_MT5891)
        /* set RAM latencies, TAG=1T, DATA=3T */
        writel(0, l2x0_base + L2X0_TAG_LATENCY_CTRL);
        writel(0x21, l2x0_base + L2X0_DATA_LATENCY_CTRL);
#elif defined(CONFIG_ARCH_MT5880)
        /* set RAM latencies, TAG=1T, DATA=2T */
        if (!IS_IC_MT5860())
        {
            writel(0, l2x0_base + L2X0_TAG_LATENCY_CTRL);
            writel(0x11, l2x0_base + L2X0_DATA_LATENCY_CTRL);
        }
        else
        {
            // For Python, Set Tag RAM DELSEL(0x1d8)
            __bim_writel(0xea3, 0x1d8);
        }
#endif

#if defined(CONFIG_ARCH_MT5399) || defined(CONFIG_ARCH_MT5398) || defined(CONFIG_ARCH_MT5880) || defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5861) || defined(CONFIG_ARCH_MT5882)|| defined(CONFIG_ARCH_MT5883) || defined(CONFIG_ARCH_MT5891)
        // Enable L2 double line fill.
        writel(readl(l2x0_base + 0xf60)|(1<<30), l2x0_base + 0xf60);
#endif
    }

#ifdef CONFIG_ENABLE_EXCLUSIVE_CACHE
    l2val |= 0x1000;
#endif

    _l2x0_init((void __iomem *)L2C_BASE, l2val, l2mask);
    __asm__ ("MRC     p15, 0, %0, c1, c0, 1" : "=r" (reg));
    __asm__ ("ORR     %0, %1, #0x8" : "=r" (reg) : "r" (reg)); //FOZ=0x8
    __asm__ ("MCR     p15, 0, %0, c1, c0, 1" : : "r" (reg));
    return 0;
}
#else
static int mt53xx_l2_cache_init(void)
{
    _l2x0_init((void __iomem *)L2C_BASE, 0x31000000, 0xce7fffff);
    return 0;
}
#endif /* CONFIG_CPU_V7 */
early_initcall(mt53xx_l2_cache_init);

int mt53xx_l2_cache_resume(void)
{
    u32 u4Reg;
    /* set pmu irq to level triggered */
    u4Reg = __bim_readl(REG_RW_MISC);
    u4Reg |= 0x100;
    __bim_writel(u4Reg, REG_RW_MISC);

    return mt53xx_l2_cache_init();
}
#endif

/*****************************************************************************
 * Model Index
 ****************************************************************************/

unsigned int modelindex_id;
EXPORT_SYMBOL(modelindex_id);
static int __init modelindex_setup(char *line)
{
	sscanf(line, "%u",&modelindex_id);
	printk("kernel setup modelindex=%u\n",modelindex_id);
	return 1;
}

__setup("modelindex=", modelindex_setup);

#ifdef CONFIG_OF
static const char *mt53xx_boards_compat[] __initdata = {
	"Mediatek,MT53xx",
        NULL,
};

//DT_MACHINE_START(MT53XX, "Mediatek MT53xx (Flattened Device Tree)")
DT_MACHINE_START(MT53XX, "MT5890")
    .smp            = smp_ops(mt53xx_smp_ops),
    .map_io         = mt53xx_map_io,
    //.init_irq       = mt53xx_init_machine_irq,
    //.init_time      = mt53xx_timer_init,
    .init_machine   = mt53xx_init_machine,
    .dt_compat      = mt53xx_boards_compat,
#ifdef CONFIG_ZONE_DMA
    .dma_zone_size  = 0xff,            // Dummy value to make sure we init arm_dma_zone_size
#endif
    // !!!!!FIXME!!!!! The IRQ priority & ARM11 irq support is still missing.
    //.handle_irq     = gic_handle_irq,
#ifdef LIMIT_GPU_TO_ChannelX
    .reserve        = mt53xx_reserve,
#endif
    .restart        = mt53xx_restart,
MACHINE_END
#endif

unsigned int ssusb_adb_flag=0xFF;
EXPORT_SYMBOL(ssusb_adb_flag);
static int __init ssusb_adb_flag_check(char *str)
{
    char *s;
    char tmp[2]={0};
    int j;

    s=str;
    if(strlen(str) != 0)
	{

        for(j=0;j<2;j++)
        {
            tmp[j]=*s;
            s++;
        }
        //printk("Parsing String = %s \n",str);
        if((tmp[0]=='=')&&(tmp[1]=='1'))
        {
            ssusb_adb_flag = 1;
        }
        else
            ssusb_adb_flag = 0;
    }
    else
	{
        ssusb_adb_flag = 0xFF;
    }
    printk("ssusb_adb_flag = %d \n",ssusb_adb_flag);
    return 1;
}

__setup("adb_enable", ssusb_adb_flag_check);


unsigned int ssusb_adb_port = 0xFF;
EXPORT_SYMBOL(ssusb_adb_port);
static int __init ssusb_adb_port_check(char *str)
{
    char *s;
    char tmp[2]={0};
    int j;

    s=str;
    if(strlen(str) != 0)
    {
        for(j=0;j<2;j++)
        {
            tmp[j]=*s;
            s++;
        }
        //        printk("Parsing String = %s \n",str);
        if((tmp[0]=='='))
        {
            ssusb_adb_port = (int)(tmp[1] - '0');
        }
        else
            {
            ssusb_adb_port = 0xFF;
            }
    }
    else
	{
        ssusb_adb_port = 0xFF;
    }

    printk("ssusb_adb_port = %d \n",ssusb_adb_port);
    return 1;
}

__setup("adb_port", ssusb_adb_port_check);

unsigned int isRecoveryMode = 0;
static int __init setRecoveryMode(char *str)
{
    printk(KERN_EMERG "(yjdbg) Android recovery mode\n");
    isRecoveryMode = 1;
    return 1;
}
__setup("AndroidRecovery", setRecoveryMode);

unsigned int isAndroidRecoveryMode(void)
{
    return isRecoveryMode;
}
EXPORT_SYMBOL(isAndroidRecoveryMode);


#ifdef CONFIG_ARCH_MT5368
MACHINE_START(MT5368, "MT5368")
#elif defined(CONFIG_MACH_MT5396)
MACHINE_START(MT5396, "MT5396")
#elif defined(CONFIG_MACH_MT5369)
MACHINE_START(MT5396, "MT5369")
#elif defined(CONFIG_MACH_MT5389)
MACHINE_START(MT5389, "MT5389")
#elif defined(CONFIG_MACH_MT5398)
MACHINE_START(MT5398, "MT5398")
#elif defined(CONFIG_MACH_MT5399)
MACHINE_START(MT5399, "MT5399")
#elif defined(CONFIG_MACH_MT5890)
MACHINE_START(MT5890, "MT5890")
#elif defined(CONFIG_MACH_MT5861)
MACHINE_START(MT5861, "MT5861")
#elif defined(CONFIG_MACH_MT5880)
MACHINE_START(MT5880, "MT5880")
#elif defined(CONFIG_MACH_MT5881)
MACHINE_START(MT5881, "MT5881")
#elif defined(CONFIG_MACH_MT5882)
MACHINE_START(MT5882, "MT5882")
#elif defined(CONFIG_MACH_MT5883)
MACHINE_START(MT5883, "MT5883")
#elif defined(CONFIG_MACH_MT5891)
MACHINE_START(MT5891, "MT5891")
#else
#error Undefined mach/arch
#endif
    .atag_offset    = 0x100,
    .smp            = smp_ops(mt53xx_smp_ops),
    .map_io         = mt53xx_map_io,
    .init_irq       = mt53xx_init_machine_irq,
    .init_time      = mt53xx_timer_init,
    .init_machine   = mt53xx_init_machine,
#ifdef LIMIT_GPU_TO_ChannelX
     .reserve        = mt53xx_reserve,
#endif
#ifdef CONFIG_ZONE_DMA
    .dma_zone_size  = 0xff,            // Dummy value to make sure we init arm_dma_zone_size
#endif
    // !!!!!FIXME!!!!! The IRQ priority & ARM11 irq support is still missing.
    //.handle_irq     = gic_handle_irq,
    .restart        = mt53xx_restart,
MACHINE_END
