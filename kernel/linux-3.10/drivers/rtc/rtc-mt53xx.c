/*
 * A RTC driver for the Mediatek MT53xx
 *
 * Maintainer: KY Lin <ky.lin@mediatek.com>
 * 
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/bcd.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/gfp.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/rtc.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/of_address.h>
#include <mach/platform.h>

static void __iomem *pRtctIoMap;

// TODO: remove double definition
#define PDWNC_YEAR (pRtctIoMap+0x700 )
#define PDWNC_MONTH (pRtctIoMap+0x704)
#define PDWNC_DAY (pRtctIoMap+0x708)
#define PDWNC_WEEK (pRtctIoMap+0x70C)
#define PDWNC_HOUR (pRtctIoMap+0x710)
#define PDWNC_MINUTE (pRtctIoMap+0x714)
#define PDWNC_SECOND (pRtctIoMap+0x718)
#define PDWNC_AYEAR (pRtctIoMap+0x71C)
#define PDWNC_AMONTH (pRtctIoMap+0x720)
#define PDWNC_ADAY (pRtctIoMap+0x724)
#define PDWNC_AWEEK (pRtctIoMap+0x728)
#define PDWNC_AHOUR (pRtctIoMap+0x72C)
#define PDWNC_AMINUTE (pRtctIoMap+0x730)
#define PDWNC_ASECOND (pRtctIoMap+0x734)
#define PDWNC_ALMINT (pRtctIoMap+0x738)
#define PDWNC_RTCCTL (pRtctIoMap+0x73C)
#define PDWNC_LEAPYEAR (pRtctIoMap+0x744)
#define PDWNC_PROT (pRtctIoMap+0x748)
#define PDWNC_INTSTA (pRtctIoMap + 0x041)
#define PDWNC_ARM_INTEN (pRtctIoMap + 0x045)
#define PDWNC_INTCLR (pRtctIoMap + 0x049)
/* Bits in the Interrupts register */
    #define RTC_INT 0x10//[12]
#define PDWNC_WKRSC (pRtctIoMap + 0x108)
#define PDWNC_WAKEN (pRtctIoMap + 0x120)
	#define RTC_WAKEN (0x01<<15)


#define RTC_MONTH_MASK	0x1f
#define RTC_DAY_MASK		0x3f
#define RTC_WEEK_MASK		0x07
#define RTC_HOUR_MASK		0x3f
#define RTC_MINUTE_MASK	0x7f
#define RTC_SECONDS_MASK	0x7f

/* Bits in the Flags register */
#define FLD_PASS	0x40
#define FLD_H24	0x02
#define FLD_STOP 0x01
#define RTC_FLAGS_AF		0x40
#define RTC_FLAGS_PF		0x20
#define RTC_WRITE		0x02
#define RTC_READ		0x01

#define RTC_WRITE_PROTECT_CODE1          0x87
#define RTC_WRITE_PROTECT_CODE2          0x78   

#define UTC_INIT_TIME                   946742400       ///< UTC initial time.

#ifndef TRUE
    #define TRUE                (0 == 0)
#endif  // TRUE

#ifndef FALSE
    #define FALSE               (0 != 0)
#endif  // FALSE

static u8 _fgRTCInit = FALSE;
static u8 _fgRTCDisableRTC = FALSE;

struct rtc_plat_data {
	struct rtc_device *rtc;
	int irq;
	unsigned int irqen;
	int alrm_sec;
	int alrm_min;
	int alrm_hour;
	int alrm_mday;
};

#ifndef SYS_RESET_TIMER
#define SYS_RESET_TIMER             0xB71B00 //0xB71B00 <=> 500ms under 24MHz //0x4F1DD <=>13.5ms under 24MHz 
#endif

#define DEFAULT_RESET_COUNTER      ((SYS_RESET_TIMER == 0x1000000) ? (SYS_RESET_TIMER - 1) : SYS_RESET_TIMER)
static bool PDWNC_IsBootByWakeup(void)
{
	return (readl(PDWNC_WKRSC) == DEFAULT_RESET_COUNTER);
}

//----------------------------------------------------------------------------
/** RTC_IsAllRegsBcd() Check all RTC register in BCD format.
 *  @retval FALSE - not all BCD, TRUE - all BCD.
 */
//----------------------------------------------------------------------------
static bool RTC_IsAllRegsBcd(void)
{
    u8 u1Bcd;

    u1Bcd = readb(PDWNC_YEAR);
    if (((u1Bcd >> 4) > 9) || ((u1Bcd & 0xf) > 9)) { return FALSE; }  
    u1Bcd = readb(PDWNC_MONTH);
    if (((u1Bcd >> 4) > 9) || ((u1Bcd & 0xf) > 9)) { return FALSE; }
    u1Bcd = readb(PDWNC_DAY);
    if (((u1Bcd >> 4) > 9) || ((u1Bcd & 0xf) > 9)) { return FALSE; }
    u1Bcd = readb(PDWNC_HOUR);
    if (((u1Bcd >> 4) > 9) || ((u1Bcd & 0xf) > 9)) { return FALSE; }
    u1Bcd = readb(PDWNC_MINUTE);
    if (((u1Bcd >> 4) > 9) || ((u1Bcd & 0xf) > 9)) { return FALSE; }
    u1Bcd = readb(PDWNC_SECOND);
    if (((u1Bcd >> 4) > 9) || ((u1Bcd & 0xf) > 9)) { return FALSE; }

    return TRUE;
}

static int mt53xx_rtc_set_time(struct device *dev, struct rtc_time *tm)
{
	u8 flags;

	// Pass write protection
	writeb(RTC_WRITE_PROTECT_CODE1, PDWNC_PROT);
	writeb(RTC_WRITE_PROTECT_CODE2, PDWNC_PROT);

	if (!_fgRTCDisableRTC)
	{      
		// Set H24 and Stop mode
		flags = readb(PDWNC_RTCCTL);
		writeb(flags | (FLD_H24 | FLD_STOP), PDWNC_RTCCTL);
		// Set to RTC

		if (tm->tm_sec != 59)
		{
			writeb(bin2bcd(tm->tm_sec) & RTC_SECONDS_MASK, PDWNC_SECOND);
		}
		else
		{
			writeb(0x00, PDWNC_SECOND);
		}
		writeb(bin2bcd(tm->tm_min) & RTC_MINUTE_MASK, PDWNC_MINUTE);
 		writeb(bin2bcd(tm->tm_hour) & RTC_HOUR_MASK, PDWNC_HOUR);
 		writeb(bin2bcd(tm->tm_mday) & RTC_DAY_MASK, PDWNC_DAY);
		writeb(bin2bcd(tm->tm_mon+1) & RTC_MONTH_MASK, PDWNC_MONTH);
		writeb(bin2bcd(tm->tm_year-100), PDWNC_YEAR);
 		writeb(bin2bcd(tm->tm_wday) & RTC_WEEK_MASK, PDWNC_WEEK);
 		writeb(bin2bcd(tm->tm_year%4), PDWNC_LEAPYEAR);


		if (tm->tm_sec == 59)
		{
			writeb(0x59, PDWNC_SECOND);
		}
		// Disable RTC STOP
		flags = readb(PDWNC_RTCCTL);
		writeb(flags & (~FLD_STOP), PDWNC_RTCCTL);
	}

	// Enable write protection
	writeb(0x0, PDWNC_PROT);
	//printk("mt53xx_rtc_set_time: tm_year=%d, tm_mon=%d, tm_wday=%d,tm_mday=%d,tm_hour=%d,tm_min=%d,tm_sec=%d\n" \
	//, tm->tm_year, tm->tm_mon, tm->tm_wday, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);

	return 0;
}

//----------------------------------------------------------------------------
/** _ReadTimeDate().
 */
//----------------------------------------------------------------------------
void _ReadTimeDate(struct rtc_time *tm)
{
	tm->tm_sec = readb(PDWNC_SECOND) & RTC_SECONDS_MASK;
	tm->tm_min =  readb(PDWNC_MINUTE) & RTC_MINUTE_MASK;
	tm->tm_hour = readb(PDWNC_HOUR) & RTC_HOUR_MASK;
	tm->tm_mday = readb(PDWNC_DAY) & RTC_DAY_MASK;
	tm->tm_mon = readb(PDWNC_MONTH) & RTC_MONTH_MASK;
	tm->tm_year = readb(PDWNC_YEAR);
	tm->tm_wday = readb(PDWNC_WEEK) & RTC_WEEK_MASK;
}


static int mt53xx_rtc_read_time(struct device *dev, struct rtc_time *tm)
{
	struct rtc_time rtm, rtm2;

	if (_fgRTCDisableRTC)
	{      
		return -EINVAL;
	}

	// Get time in second from RTC   
	// If two consecutive read RTC values have difference in min, hr, day, mon, yr,
	// It indicates carry occurs upon RTC read and the read value is not reliable,
	// thus we perform another two consecutive read
	do
	{
		_ReadTimeDate(&rtm);
		_ReadTimeDate(&rtm2);   
	}
	while((rtm.tm_year != rtm2.tm_year) ||
	(rtm.tm_mon != rtm2.tm_mon) ||
	(rtm.tm_mday != rtm2.tm_mday) ||
	(rtm.tm_hour != rtm2.tm_hour) ||
	(rtm.tm_min != rtm2.tm_min));

	tm->tm_sec = bcd2bin(rtm2.tm_sec);
	tm->tm_min = bcd2bin(rtm2.tm_min);
	tm->tm_hour = bcd2bin(rtm2.tm_hour);
	tm->tm_mday = bcd2bin(rtm2.tm_mday);
	tm->tm_wday = bcd2bin(rtm2.tm_wday);
	tm->tm_mon = bcd2bin(rtm2.tm_mon)-1;
	tm->tm_year = bcd2bin(rtm2.tm_year)+100;

#if 1	
	if (rtc_valid_tm(tm) < 0) {
		dev_err(dev, "retrieved date/time is not valid.\n");
		rtc_time_to_tm(0, tm);
	}
#endif
	//printk("mt53xx_rtc_read_time: tm_year=%d, tm_mon=%d, tm_wday=%d,tm_mday=%d,tm_hour=%d,tm_min=%d,tm_sec=%d\n" \
	//, tm->tm_year, tm->tm_mon, tm->tm_wday, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);

	return 0;
}

static void mt53xx_rtc_update_alarm(struct rtc_plat_data *pdata)
{
	//unsigned long irqflags;
	//u8 flags;

	// Pass write protection
	writeb(RTC_WRITE_PROTECT_CODE1, PDWNC_PROT);
	writeb(RTC_WRITE_PROTECT_CODE2, PDWNC_PROT);

	writeb(pdata->alrm_mday < 0 || (pdata->irqen & RTC_UF) ?
	       0x80 : bin2bcd(pdata->alrm_mday),
	       PDWNC_ADAY);
	writeb(pdata->alrm_hour < 0 || (pdata->irqen & RTC_UF) ?
	       0x80 : bin2bcd(pdata->alrm_hour),
	       PDWNC_AHOUR);
	writeb(pdata->alrm_min < 0 || (pdata->irqen & RTC_UF) ?
	       0x80 : bin2bcd(pdata->alrm_min),
	       PDWNC_AMINUTE);
	writeb(pdata->alrm_sec < 0 || (pdata->irqen & RTC_UF) ?
	       0x80 : bin2bcd(pdata->alrm_sec),
	       PDWNC_ASECOND);
	//flags = readb(PDWNC_ARM_INTEN);
	//writeb(pdata->irqen ? (flags | RTC_INT) : (flags & ~RTC_INT), PDWNC_ARM_INTEN);
	//writeb(pdata->irqen ? (flags | RTC_INT) : (flags & ~RTC_INT), PDWNC_INTCLR);/* clear interrupts */
	// Enable write protection
	writeb(0x0, PDWNC_PROT);
}
#if defined(CC_SUPPORT_ANDROID_L_RTC_WAKEUP)
static void mt53xx_rtc_update_alarm_2(struct rtc_plat_data *pdata)
{
	//u8 flags;
	writeb(RTC_WRITE_PROTECT_CODE1, PDWNC_PROT);
	writeb(RTC_WRITE_PROTECT_CODE2, PDWNC_PROT);
	writeb(bin2bcd(pdata->alrm_mday),PDWNC_ADAY);
	writeb(bin2bcd(pdata->alrm_hour), PDWNC_AHOUR);
	writeb(bin2bcd(pdata->alrm_min), PDWNC_AMINUTE);
	writeb(bin2bcd(pdata->alrm_sec), PDWNC_ASECOND);
	//flags = readb(PDWNC_ARM_INTEN);
	//writeb(pdata->irqen ? (flags | RTC_INT) : (flags & ~RTC_INT), PDWNC_ARM_INTEN);
	//writeb(pdata->irqen ? (flags | RTC_INT) : (flags & ~RTC_INT), PDWNC_INTCLR);/* clear interrupts */
	writeb(0x2f, PDWNC_ALMINT);
	// Enable write protection
	writeb(0x0, PDWNC_PROT);
	writel(readl(PDWNC_WAKEN) | RTC_WAKEN, PDWNC_WAKEN); //enable RTC wakeup.
}

static int mt53xx_rtc_set_alarm_2(struct device *dev, struct rtc_wkalrm *alrm)
{
	//printk("mt53xx_rtc_set_alarm_2 enter : %d,%d,%d,%d,%d,%d,%d,%d,%d .\n",alrm->time.tm_isdst,alrm->time.tm_yday,alrm->time.tm_wday,alrm->time.tm_year,alrm->time.tm_mon,\
	//	alrm->time.tm_mday,alrm->time.tm_hour,alrm->time.tm_min,alrm->time.tm_sec);
	struct rtc_plat_data pdata;
	
	pdata.alrm_mday = alrm->time.tm_mday;
	pdata.alrm_hour = alrm->time.tm_hour;
	pdata.alrm_min = alrm->time.tm_min;
	pdata.alrm_sec = alrm->time.tm_sec;
	if (alrm->enabled)
		pdata.irqen |= RTC_AF;
	mt53xx_rtc_update_alarm_2(&pdata);
	printk("set rtc wakeup time : %d-%d-%d  %d-%d-%d.\n",(1900 + alrm->time.tm_year),(alrm->time.tm_mon + 1),\
		alrm->time.tm_mday,alrm->time.tm_hour,alrm->time.tm_min,alrm->time.tm_sec);
	return 0;
}
#endif

static int mt53xx_rtc_set_alarm(struct device *dev, struct rtc_wkalrm *alrm)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct rtc_plat_data *pdata = platform_get_drvdata(pdev);

	if (pdata->irq <= 0)
		return -EINVAL;
	pdata->alrm_mday = alrm->time.tm_mday;
	pdata->alrm_hour = alrm->time.tm_hour;
	pdata->alrm_min = alrm->time.tm_min;
	pdata->alrm_sec = alrm->time.tm_sec;
	if (alrm->enabled)
		pdata->irqen |= RTC_AF;
	mt53xx_rtc_update_alarm(pdata);
	return 0;
}


static int mt53xx_rtc_read_alarm(struct device *dev, struct rtc_wkalrm *alrm)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct rtc_plat_data *pdata = platform_get_drvdata(pdev);

	if (pdata->irq <= 0)
		return -EINVAL;
	alrm->time.tm_mday = pdata->alrm_mday < 0 ? 0 : pdata->alrm_mday;
	alrm->time.tm_hour = pdata->alrm_hour < 0 ? 0 : pdata->alrm_hour;
	alrm->time.tm_min = pdata->alrm_min < 0 ? 0 : pdata->alrm_min;
	alrm->time.tm_sec = pdata->alrm_sec < 0 ? 0 : pdata->alrm_sec;
	alrm->enabled = (pdata->irqen & RTC_AF) ? 1 : 0;
	return 0;
}

static irqreturn_t mt53xx_rtc_interrupt(int irq, void *dev_id)
{
	struct platform_device *pdev = dev_id;
	struct rtc_plat_data *pdata = platform_get_drvdata(pdev);
	unsigned long events = 0;

	/* read and clear interrupt */
	if (readb(PDWNC_INTSTA) & RTC_INT) {
		events = RTC_IRQF;
		if (readb(PDWNC_ASECOND) & 0x80)
			events |= RTC_UF;
		else
			events |= RTC_AF;
		if (likely(pdata->rtc))
			rtc_update_irq(pdata->rtc, 1, events);
	}
	return events ? IRQ_HANDLED : IRQ_NONE;
}

static int mt53xx_rtc_alarm_irq_enable(struct device *dev,
	unsigned int enabled)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct rtc_plat_data *pdata = platform_get_drvdata(pdev);

	if (pdata->irq <= 0)
		return -EINVAL;
	if (enabled)
		pdata->irqen |= RTC_AF;
	else
		pdata->irqen &= ~RTC_AF;
	mt53xx_rtc_update_alarm(pdata);
	return 0;
}

static const struct rtc_class_ops mt53xx_rtc_ops = {
	.read_time	= mt53xx_rtc_read_time,
	.set_time	= mt53xx_rtc_set_time,
	#if defined(CC_SUPPORT_ANDROID_L_RTC_WAKEUP)
	.set_alarm	= mt53xx_rtc_set_alarm_2,
	#endif
};

static const struct rtc_class_ops mt53xx_rtc_alarm_ops = {
	.read_time		= mt53xx_rtc_read_time,
	.set_time		= mt53xx_rtc_set_time,
	.read_alarm		= mt53xx_rtc_read_alarm,
	.set_alarm		= mt53xx_rtc_set_alarm,
	.alarm_irq_enable	= mt53xx_rtc_alarm_irq_enable,
};

static int mt53xx_rtc_probe(struct platform_device *pdev)
{
	struct rtc_plat_data *pdata;
	u8 flags;
#ifdef CONFIG_OF
	struct device_node *np;
	np = of_find_compatible_node(NULL, NULL, "Mediatek, PDWNC");
	if (!np) {
		panic("%s: rtc node not found\n", __func__);
	}
	pRtctIoMap = of_iomap(np, 0);
#else
	pRtctIoMap = ioremap(PDWNC_PHY,0x1000);
#endif
	if (!pRtctIoMap)
		panic("Impossible to ioremap RTC\n");
	printk("Rtc_reg_base: 0x%p\n", pRtctIoMap);
#if 0
	int ret = 0;
	int irq;
#endif

	if (_fgRTCInit)
	{
		// Already initialized
		return 0;
	}

	// Pass write protection
	writeb(RTC_WRITE_PROTECT_CODE1, PDWNC_PROT);
	writeb(RTC_WRITE_PROTECT_CODE2, PDWNC_PROT);

	if((readb(PDWNC_RTCCTL) & FLD_PASS) == 0)//?
	{   
		_fgRTCDisableRTC = TRUE;
		// Enable write protection
		writeb(0x0, PDWNC_PROT);
		return 0;
	}

	flags = readb(PDWNC_RTCCTL);
	writeb(flags | (FLD_H24 | FLD_PASS), PDWNC_RTCCTL);

	//printk("mt53xx_rtc_probe: RTC_IsAllRegsBcd()=%d, PDWNC_IsBootByWakeup()=%d\n" \
	//, RTC_IsAllRegsBcd(), PDWNC_IsBootByWakeup());
	if (!RTC_IsAllRegsBcd() || !PDWNC_IsBootByWakeup())                                              
	{      
		//Sat, 01 Jan 2000 16:00:00 on GMT timezone
	 	// Pass write protection
		writeb(RTC_WRITE_PROTECT_CODE1, PDWNC_PROT);
		writeb(RTC_WRITE_PROTECT_CODE2, PDWNC_PROT);

		if (!_fgRTCDisableRTC)
		{      
			// Set H24 and Stop mode
			flags = readb(PDWNC_RTCCTL);
			writeb(flags | (FLD_H24 | FLD_STOP), PDWNC_RTCCTL);

			writeb(0x00, PDWNC_SECOND);
			writeb(bin2bcd(0) & RTC_MINUTE_MASK, PDWNC_MINUTE);
	 		writeb(bin2bcd(0) & RTC_HOUR_MASK, PDWNC_HOUR);
	 		writeb(bin2bcd(1) & RTC_DAY_MASK, PDWNC_DAY);
			writeb(bin2bcd(1) & RTC_MONTH_MASK, PDWNC_MONTH);
			writeb(bin2bcd(15), PDWNC_YEAR);
	 		writeb(bin2bcd(1) & RTC_WEEK_MASK, PDWNC_WEEK);

			// Disable RTC STOP
			flags = readb(PDWNC_RTCCTL);
			writeb(flags & (~FLD_STOP), PDWNC_RTCCTL);
		}
	}
	// Enable write protection
	writeb(0x0, PDWNC_PROT);
	_fgRTCInit = TRUE;

	pdata = devm_kzalloc(&pdev->dev, sizeof(struct rtc_plat_data), GFP_KERNEL);
	if (!pdata) {
		dev_dbg(&pdev->dev, "out of memory\n");
		return -ENOMEM;
	}
	pdata->irq = platform_get_irq(pdev, 0);
	
	if (pdata->irq >= 0) {
		device_init_wakeup(&pdev->dev, 1);
		pdata->rtc = rtc_device_register(pdev->name, &pdev->dev,
						 &mt53xx_rtc_alarm_ops,
						 THIS_MODULE);
	} else{

		pdata->rtc = rtc_device_register(pdev->name, &pdev->dev,
						 &mt53xx_rtc_ops, THIS_MODULE);}
	if (IS_ERR(pdata->rtc))
		return PTR_ERR(pdata->rtc);

	if (pdata->irq >= 0) {
		flags = readb(PDWNC_ARM_INTEN);
		writeb(flags & ~RTC_INT, PDWNC_ARM_INTEN);
		if (devm_request_irq(&pdev->dev, pdata->irq, mt53xx_rtc_interrupt,
				     IRQF_DISABLED | IRQF_SHARED,
				     pdev->name, pdata) < 0) {
			dev_warn(&pdev->dev, "interrupt not available.\n");
			pdata->irq = -1;
		}
	}

	dev_info(&pdev->dev, "MT53xx Real Time Clock driver.\n");
	return 0;
}

static int __exit mt53xx_rtc_remove(struct platform_device *pdev)
{
	struct rtc_plat_data *pdata = platform_get_drvdata(pdev);

	if (pdata->irq >= 0)
		device_init_wakeup(&pdev->dev, 0);

	rtc_device_unregister(pdata->rtc);
	return 0;
}

/* work with hotplug and coldplug */
MODULE_ALIAS("platform:mt53xx_rtc");

static struct platform_driver mt53xx_rtc_driver = {
	.remove		= __exit_p(mt53xx_rtc_remove),
	.driver		= {
		.name	= "mt53xx_rtc",
		.owner	= THIS_MODULE,
	},
};

static __init int mt53xx_init(void)
{
	return platform_driver_probe(&mt53xx_rtc_driver, mt53xx_rtc_probe);
}

static __exit void mt53xx_exit(void)
{
	platform_driver_unregister(&mt53xx_rtc_driver);
}

module_init(mt53xx_init);
module_exit(mt53xx_exit);


