/*
 * linux/arch/arm/mach-mt53xx/core.c
 *
 * CPU core init - irq, time, baseio
 *
 * Copyright (c) 2010-2012 MediaTek Inc.
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
#include <linux/soc/mediatek/hardware.h>
#include <linux/soc/mediatek/mt53xx_linux.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <asm/setup.h>
//#include <asm/mach-types.h>
#include <asm/pgtable.h>
#include <asm/page.h>
#include <asm/uaccess.h>
#include <linux/irq.h>
#include <asm/system_misc.h>

//#include <asm/mach/arch.h>
//#include <asm/mach/irq.h>
//#include <asm/mach/map.h>

//#include <asm/mach/time.h>
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

#include <linux/platform_device.h>
unsigned int mt53xx_pm_suspend_cnt = 0;
EXPORT_SYMBOL(mt53xx_pm_suspend_cnt);
#define CONFIG_BOOT_TIME
#ifdef ANDROID
//#include <linux/usb/android_composite.h>
#include <linux/platform_device.h>
#endif

#ifdef CONFIG_NATIVE_CB2
#include <linux/semaphore.h>

#include <linux/suspend.h>
//#include <mach/cb_data.h>
//#include <mach/cb_low.h>
#include <linux/soc/mediatek/cb_data.h>
#include <linux/soc/mediatek/cb_low.h>
extern void _CB_Init(void);
#endif

#ifndef CONFIG_SMP
void mt53xx_resume_up_check(void)
{
    //TODO
}
EXPORT_SYMBOL(mt53xx_resume_up_check);
#endif
#if defined(CONFIG_HIBERNATION) || defined(CONFIG_OPM)
#ifdef CONFIG_SMP
//void arch_disable_nonboot_cpus_early_resume(void);
//EXPORT_SYMBOL(arch_disable_nonboot_cpus_early_resume);
#endif
#endif
DEFINE_SPINLOCK(mt53xx_bim_lock);
EXPORT_SYMBOL(mt53xx_bim_lock);

unsigned int mt53xx_cha_mem_size __read_mostly;
static unsigned long mt53xx_reserve_start __read_mostly, mt53xx_reserve_size __read_mostly;

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
       __pdwnc_writel(0x7fff0000, 0x104);
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

static int __init PrintBootReason(char *str)
{
    printk("Last boot reason: %s\n", str);
    return 1;
}
__setup("bootreason=", PrintBootReason);

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

#if defined(CONFIG_ARCH_MT5399) || defined(CONFIG_ARCH_MT5398) || defined(CONFIG_ARCH_MT5880) || defined(CONFIG_ARCH_MT5881) || defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5861) || defined(CONFIG_ARCH_MT5882)|| defined(CONFIG_ARCH_MT5883) || defined(CONFIG_ARCH_MT5891)
    u4Version = __ckgen_readl(REG_RO_SW_ID);
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

#define MTK_MTAL_DMA_MASK 0xFFFFFFFF
struct platform_device mt53xx_mtal_device = {
    .name       = "mtal_coherent_alloc",
    .id         = -1,
    .dev = {
        .coherent_dma_mask = MTK_MTAL_DMA_MASK,
        .dma_mask = &(mt53xx_mtal_device.dev.coherent_dma_mask),
    },

};

EXPORT_SYMBOL(mt53xx_mtal_device);


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

extern char _text, _etext;
int mt53xx_get_rodata_addr(unsigned long long *start, unsigned long long *end)
{
    if (start)
    {
        *start = (&_text);;
    }

    if (end)
    {
        *end = (&_etext);
    }

    return 0;
}
EXPORT_SYMBOL(mt53xx_get_rodata_addr);

extern char _data, _end; // TODO
int mt53xx_get_rwdata_addr(unsigned long long *start, unsigned long long *end)
{
    if (start)
    {
        *start = (&_data);
    }

    if (end)
    {
        *end = (&_end);
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
	//__raw_writeb(0x01,PDWNC_VIRT + REG_RW_RESRV5);
	__pdwnc_writel((__pdwnc_readl(REG_RW_RESRV5) & 0xFFFFFF00) | 0x01,REG_RW_RESRV5);
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

#ifdef CONFIG_BOOT_TIME
extern u32 _CKGEN_GetXtalClock(void);
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

    for (i=0; i<u4Size; i++)
    {
        u4Timer = prTimeStamp[i].u4TimeStamp;
       // u4Val = ((~u4Timer)/(_CKGEN_GetXtalClock()/1000000));  // us time.
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
    if (_u4TimeStampSize < MAX_SW_TIMESTAMP_SIZE)
    {
        buf = kmalloc(TAGSTRING_SIZE, GFP_KERNEL);
	if (!buf) {
	    printk(KERN_ERR
		    "store_timestamp no buffer \n");
	    return 0;
	}
        if (copy_from_user(buf, buffer, len))
	{
	    kfree(buf);
            return count;
	}
        buf[len] = 0;
        _arTimeStamp[_u4TimeStampSize].u4TimeStamp = __bim_readl(REG_RW_TIMER2_LOW); //BIM_READ32(REG_RW_TIMER2_LOW)
        _arTimeStamp[_u4TimeStampSize].szString = buf;
        _u4TimeStampSize++;

        return strnlen(buf, count);
    }

    return 0;
}

void add_timestamp(char* buffer)
{
    char* buf;
    unsigned long count = strlen(buffer);
    unsigned long len = (count > TAGSTRING_SIZE-1) ? TAGSTRING_SIZE-1 : count;
    if (_u4TimeStampSize < MAX_SW_TIMESTAMP_SIZE)
    {
        buf = kmalloc(TAGSTRING_SIZE, GFP_KERNEL);
	if (!buf) {
	    printk(KERN_ERR
		    "add_timestamp no buffer \n");
	    return;
	}
	memcpy(buf, buffer, len);
        buf[len] = 0;
        _arTimeStamp[_u4TimeStampSize].u4TimeStamp = __bim_readl(REG_RW_TIMER2_LOW); //BIM_READ32(REG_RW_TIMER2_LOW)
        _arTimeStamp[_u4TimeStampSize].szString = buf;
        _u4TimeStampSize++;

        return;
    }

    return;
}
EXPORT_SYMBOL(add_timestamp);

void free_builtin_timestamp(void)
{
    unsigned int i, u4Size;
    TIME_STAMP_T *prTimeStamp;

    u4Size = _u4TimeStampSize;
    prTimeStamp = _arTimeStamp;
    for (i=0; i<u4Size; i++)
    {
	kfree(prTimeStamp[i].szString);
    }
    _u4TimeStampSize = 0;
}
EXPORT_SYMBOL(free_builtin_timestamp);
#else
void * x_os_drv_get_timestampKernel(int *pu4Size)
{
    return NULL;
}
EXPORT_SYMBOL(x_os_drv_get_timestampKernel);
void add_timestamp(char* buffer)
{
    return;
}
EXPORT_SYMBOL(add_timestamp);

void free_builtin_timestamp(void)
{
}
EXPORT_SYMBOL(free_builtin_timestamp);
#endif
/*****************************************************************************
 * 53xx init
 ****************************************************************************/
static void mt53xx_restart(char mode, const char *cmd)
{
	/*
	 * use powerdown watch dog to reset system
	 * __raw_writel( 0xff000000, BIM_BASE + WATCHDOG_TIMER_COUNT_REG );
	 * __raw_writel( 1, BIM_BASE + WATCHDOG_TIMER_CTRL_REG);
	 */
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
	#if defined(CC_CMDLINE_SUPPORT_BOOTREASON)
		#if defined(CONFIG_MACH_MT5890) || defined(CONFIG_ARCH_MT5891)
		//__raw_writeb(0x02,PDWNC_VIRT + REG_RW_RESRV5); // TODO: writeb -> writel
		__pdwnc_writel((__pdwnc_readl(REG_RW_RESRV5) & 0xFFFFFF00) | 0x02,REG_RW_RESRV5);
		#endif
	#endif
	 //__raw_writel( 0xffff0000, PDWNC_VIRT + REG_RW_WATCHDOG_TMR );
	 //__raw_writel( 1, PDWNC_VIRT + REG_RW_WATCHDOG_EN);
	 __pdwnc_writel( 0xffff0000, REG_RW_WATCHDOG_TMR);
	 __pdwnc_writel( 1, REG_RW_WATCHDOG_EN);
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

#ifdef CONFIG_BOOT_TIME
static int bt_proc_open(struct inode *inode, struct file *file) {
     return single_open(file, show_boottime, PDE_DATA(inode));
}

static const struct file_operations bt_proc_fops = {
     .open    = bt_proc_open,
     .read    = seq_read,
     .llseek  = seq_lseek,
     .release = seq_release,
     .write = store_timestamp,
};
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

int x_kmem_sync_table(void *ptr, size_t size)
{
    // TODO: prepare armv8 version
    return 1;
}
EXPORT_SYMBOL(x_kmem_sync_table);

////////////////////////////////////////////////////////////////////////////////
#ifdef CONFIG_NATIVE_CB2 // CONFIG_PDWNC_SUSPEND_RESUME_CALLBACK
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

EXPORT_SYMBOL(wakeup_pdwnc_queue);

typedef struct
{
    UINT32 u4PowerStateId;
    UINT32 u4Arg1;
    UINT32 u4Arg2;
    UINT32 u4Arg3;
    UINT32 u4Arg4;
} MTPDWNC_POWER_STATE_T;

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
////////////////////////////////////////////////////////////////////////////////
static int  __init mt53xx_init_machine(void)
{
 #ifdef CONFIG_BOOT_TIME
   struct proc_dir_entry * proc_file __attribute__((unused)) = 0;
 #endif
 arm_pm_restart = mt53xx_restart;

#ifdef CC_SUPPORT_BL_DLYTIME_CUT
	unsigned int u4CurTime;
	u32 gpio, value, time;
	time = __bim_readl((REG_RW_GPRB0 + (4 << 2)));
	printk("[LVDS][backlight] TIME=%d us\n", time);
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

    platform_device_register(&mt53xx_pmu_device);
    platform_device_register(&mt53xx_mtal_device);
 
    mt53xx_add_device_rtc();

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

#ifdef CONFIG_NATIVE_CB2
    init_pdwnc_queue();
    _CB_Init();
    register_pm_notifier(&mtkcore_pm_notifier);
#endif

    return 0;
}
arch_initcall(mt53xx_init_machine);

