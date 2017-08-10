
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/gpio.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/prefetch.h>

#include <asm/cacheflush.h>

#include "musb_core.h"
#include "SSUSB_DEV_c_header.h"
#include "../mu3d_hal/mu3d_hal_osal.h"
#include "../mu3d_hal/mu3d_hal_phy.h"
#include "musb_comm.h"
#include <linux/usb/nop-usb-xceiv.h>
#include "../mu3d_hal/mu3d_hal_usb_drv.h"
//struct bfin_glue {
struct ssusb_glue {	
	struct device		*dev;
	struct platform_device	*musb;
};
#define glue_to_musb(g)		platform_get_drvdata(g->musb)


////////////////////////
//Temp trying this:

static void ssusb_musb_reg_init(struct musb *musb);
//static int ssusb_set_power(struct otg_transceiver *x, unsigned mA);

static int ssusb_set_power(struct otg_transceiver *x, unsigned mA)
{
	return 0;
}

static struct musb_hdrc_config musb_config_mtkdtu3 = {
        .multipoint     = false,
        .dyn_fifo       = true,
        .soft_con       = true,
        .dma            = true,

        .num_eps        = 5,
        .dma_channels   = 8,
        .ram_bits       = 12,
};

static int ssusb_musb_init(struct musb *musb)
{
	usb_nop_xceiv_register();
//	musb->xceiv = usb_get_transceiver(); //
	if (!musb->xceiv) {
		printk("cannot get transceiver\n");
		return -ENODEV;
	}	

	ssusb_musb_reg_init(musb);

	if (is_peripheral_enabled(musb))
		musb->xceiv->set_power = ssusb_set_power;

	os_printk(K_DEBUG, "ssusb_musb_init\n");
		
	return 0;
}




static int ssusb_musb_exit(struct musb *musb)
{

	return 0;
}



const struct musb_platform_ops ssusb_ops = {
	.init		= ssusb_musb_init,
	.exit		= ssusb_musb_exit,

};


static struct musb_hdrc_platform_data usb_data_mtkdtu3 = {
//#if defined(CONFIG_USB_GADGET_MUSB_HDRC)
	.mode           = MUSB_PERIPHERAL,
//#elif defined(CONFIG_USB_MUSB_HDRC_HCD)
//	.mode           = MUSB_HOST,
//#endif
	.config         = &musb_config_mtkdtu3,
	.platform_ops		= &ssusb_ops,

};
static u64 usb_dmamask = DMA_BIT_MASK(32);

static struct resource usb_resources_mtkdtu3[] = {
        {
                /* physical address */
            //    .start          = U3D_BASE,
            //    .end            = U3D_BASE + U3D_SPACE_SIZE - 1,
                .flags          = IORESOURCE_MEM,
        },
        {	//general IRQ
                .start          = USB_IRQ,
                .flags          = IORESOURCE_IRQ,
        },
        {	//DMA IRQ
                .start          = USB_IRQ,
                .flags          = IORESOURCE_IRQ,
        },
};


struct platform_device usb_dev = {
        .name           = "musb-hdrc",
        .id             = 0,
        .dev = {
                .platform_data          = &usb_data_mtkdtu3,
                .dma_mask               = &usb_dmamask,
                .coherent_dma_mask      = DMA_BIT_MASK(32),
                //.release=musb_hcd_release,
        },
        .resource       = usb_resources_mtkdtu3,
        .num_resources  = ARRAY_SIZE(usb_resources_mtkdtu3),
};

#if 0
#ifdef CONFIG_ARCH_MT6290
unsigned int u3_phy_init(void);

void dev_set_dma_busrt_limiter(DEV_INT8 busrt,DEV_INT8 limiter){
	DEV_UINT32 temp;

	temp = ((busrt<<DMA_BURST_OFST)&DMA_BURST) | ((limiter<<DMALIMITER_OFST)&DMALIMITER);

	os_writel(U3D_TXDMARLCOUNT, temp);
	os_writel(U3D_RXDMARLCOUNT, temp);
}



#else
extern PHY_INT32 u3phy_init(void);

#endif
#endif



static void ssusb_musb_reg_init(struct musb *musb)
{
    DEV_UINT32 regval;
	os_printk(K_DEBUG, "ssusb_musb_reg_init\n");
//	mu3d_hal_u3phyinit();
//	mu3d_hal_u2phyinit();
#ifdef CONFIG_ARCH_MT6290
    u3_phy_init();
	mu3d_hal_ssusb_en(); //Need this to enable the RST_CTRL.
	mu3d_hal_rst_dev();  
	dev_set_dma_busrt_limiter(1,0);
#else
#if 1 //DMA configuration
	  regval = readl((UPTR*)(ulong)SSUSB_DMA);
	  regval |= 0x00000007;    
	  writel(regval, SSUSB_DMA);

#endif
	if (!u3phy_ops)
		u3phy_init();
		
	u3phy_ops->init(u3phy);

    
	mu3d_hal_ssusb_en(); //Need this to enable the RST_CTRL.
	mu3d_hal_rst_dev();
#endif
}


#if 0
static int __init ssusb_probe(struct platform_device *pdev)
{
	struct musb_hdrc_platform_data	*pdata = pdev->dev.platform_data;
	struct platform_device		*musb;
	struct ssusb_glue		*glue;

	int				ret = -ENOMEM;

	os_printk(K_CRIT, "ssusb_probe\b");
	
	glue = kzalloc(sizeof(*glue), GFP_KERNEL);
	if (!glue) {
		dev_err(&pdev->dev, "failed to allocate glue context\n");
		goto err0;
	}

	musb = platform_device_alloc("musb-hdrc", -1);
	if (!musb) {
		dev_err(&pdev->dev, "failed to allocate musb device\n");
		goto err1;
	}

	musb->dev.parent		= &pdev->dev;

	glue->dev			= &pdev->dev;
	glue->musb			= musb;

	pdata->platform_ops		= &ssusb_ops;

	platform_set_drvdata(pdev, glue);

	ret = platform_device_add_resources(musb, pdev->resource,
			pdev->num_resources);
	if (ret) {
		dev_err(&pdev->dev, "failed to add resources\n");
		goto err2;
	}

	ret = platform_device_add_data(musb, pdata, sizeof(*pdata));
	if (ret) {
		dev_err(&pdev->dev, "failed to add platform_data\n");
		goto err2;
	}

	ret = platform_device_add(musb);
	if (ret) {
		dev_err(&pdev->dev, "failed to register musb device\n");
		goto err2;
	}

	return 0;

err2:
	platform_device_put(musb);

err1:
	kfree(glue);

err0:
	return ret;
}

static int __exit ssusb_remove(struct platform_device *pdev)
{
	struct ssusb_glue		*glue = platform_get_drvdata(pdev);

	platform_device_del(glue->musb);
	platform_device_put(glue->musb);
	kfree(glue);

	return 0;
}

#ifdef CONFIG_PM
static int ssusb_suspend(struct device *dev)
{
	return 0;
}

static int ssusb_resume(struct device *dev)
{

//	ssusb_musb_reg_init(musb);

	return 0;
}

static struct dev_pm_ops ssusb_pm_ops = {
	.suspend	= ssusb_suspend,
	.resume		= ssusb_resume,
};

#define DEV_PM_OPS	&ssusb_pm_ops
#else
#define DEV_PM_OPS	NULL
#endif

static struct platform_driver ssusb_driver = {
	.remove		= __exit_p(ssusb_remove),
	.driver		= {
		.name	= "musb-ssusb",
		.pm	= DEV_PM_OPS,
	},
};
MODULE_DESCRIPTION("ssusb MUSB Glue Layer");
MODULE_AUTHOR("MediaTek");
MODULE_LICENSE("GPL v2");

//extern struct platform_device dev_usb;

extern unsigned int ssusb_adb_flag;
extern unsigned int ssusb_adb_port;

static int __init ssusb_init(void)
{
	if((ssusb_adb_flag==0) || (ssusb_adb_port != 2))
		return 0;
	os_printk(K_CRIT, "ssusb_init\n");
//	dev_usb.dev->platform_data.platform_ops = &ssusb_ops;

	return platform_driver_probe(&ssusb_driver, ssusb_probe);
}
subsys_initcall(ssusb_init);

static void __exit ssusb_exit(void)
{
	if((ssusb_adb_flag==0) || (ssusb_adb_port != 2))
		return;
	platform_driver_unregister(&ssusb_driver);
}
module_exit(ssusb_exit);
#endif
