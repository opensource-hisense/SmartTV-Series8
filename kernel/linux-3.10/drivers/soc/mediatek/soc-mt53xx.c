/*
 * Copyright (C) 2014 Linaro Ltd.
 *
 * Author: Linus Walleij <linus.walleij@linaro.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation.
 *
 */
#include <linux/init.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/sys_soc.h>
#include <linux/platform_device.h>
#include <linux/mfd/syscon.h>
#include <linux/regmap.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/of_address.h>

#include <linux/irq.h>
#include <linux/irqchip/arm-gic.h>
#include <linux/soc/mediatek/irqs.h>
#include <linux/soc/mediatek/platform.h>

void __iomem *tv_reg_base;
void __iomem *p_bim_iomap;
void __iomem *p_pdwnc_iomap;
void __iomem *p_mcusys_iomap;
void __iomem *p_ckgen_iomap;
#if 0
/* HW functions of mt53xx uart */
#define __bim_readl(REG)           __raw_readl(p_bim_iomap + (REG))
#define __bim_writel(VAL, REG)     __raw_writel(VAL, p_bim_iomap + (REG))
#endif


static inline int _bim_is_vector_valid(u32 irq_num)
{
    return (irq_num < NR_IRQS);
}


static void bim_irq_ack(struct irq_data *data)
{
    unsigned int irq = data->irq;

    if (!_bim_is_vector_valid(irq))
    {
        while (1)
            ;
        return;
    }

    if (irq < SPI_OFFSET)
    {
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


#if 0
static void mt53xx_irq_mask_ack(struct irq_data *data)
{
    u32 regval32;
    unsigned long flags;
    unsigned int irq = data->irq;

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
        regval32 = __bim_readl(REG_RW_M3INTEN);
        regval32 &= ~misc_irq;
        __bim_writel(regval32, REG_RW_M3INTEN);
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
        regval32 = __bim_readl(REG_RW_M2INTEN);
        regval32 &= ~misc_irq;
        __bim_writel(regval32, REG_RW_M2INTEN);
        __bim_writel(misc_irq, REG_RW_M2INTCLR);
        //spin_unlock_irqrestore(&mt53xx_bim_lock, flags);
    }
    else if (irq >= VECTOR_MISC_BASE)
    {
        u32 misc_irq;
        misc_irq = _MISCIRQ(irq);
        //spin_lock_irqsave(&mt53xx_bim_lock, flags);
        regval32 = __bim_readl(REG_RW_MINTEN);
        regval32 &= ~misc_irq;
        __bim_writel(regval32, REG_RW_MINTEN);
        if (irq != VECTOR_DRAMC)
        {
            __bim_writel(misc_irq, REG_RW_MINTCLR);
        }
        //spin_unlock_irqrestore(&mt53xx_bim_lock, flags);
    }
    else
    {
        //spin_lock_irqsave(&mt53xx_bim_lock, flags);
        regval32 = __bim_readl(REG_RW_IRQEN);
        regval32 &= ~(1 << (irq-SPI_OFFSET));
        __bim_writel(regval32, REG_RW_IRQEN);
        if (irq != VECTOR_DRAMC)
        {
            __bim_writel((1 << (irq-SPI_OFFSET)), REG_RW_IRQCLR);
        }
        //spin_unlock_irqrestore(&mt53xx_bim_lock, flags);
    }
}
#endif

static void mt53xx_irq_mask(struct irq_data *data)
{
    u32 regval32;
    //unsigned long flags;
    unsigned int irq = data->irq;

    if (!_bim_is_vector_valid(irq))
    {
        while (1)
            ;
        return;
    }

    if (irq < SPI_OFFSET)
    {
       return;
    }

#ifdef CC_VECTOR_HAS_MISC3
    if (irq >= VECTOR_MISC3_BASE)
    {
        u32 misc_irq;
        misc_irq = _MISC3IRQ(irq);
        //spin_lock_irqsave(&mt53xx_bim_lock, flags);
        regval32 = __bim_readl(REG_RW_M3INTEN);
        regval32 &= ~misc_irq;
        __bim_writel(regval32, REG_RW_M3INTEN);
        //spin_unlock_irqrestore(&mt53xx_bim_lock, flags);
    }
    else
#endif // CC_VECTOR_HAS_MISC3
    if (irq >= VECTOR_MISC2_BASE)
    {
        u32 misc_irq;
        misc_irq = _MISC2IRQ(irq);
        //spin_lock_irqsave(&mt53xx_bim_lock, flags);
        regval32 = __bim_readl(REG_RW_M2INTEN);
        regval32 &= ~misc_irq;
        __bim_writel(regval32, REG_RW_M2INTEN);
        //spin_unlock_irqrestore(&mt53xx_bim_lock, flags);
    }
    else if (irq >= VECTOR_MISC_BASE)
    {
        u32 misc_irq;
        misc_irq = _MISCIRQ(irq);
        //spin_lock_irqsave(&mt53xx_bim_lock, flags);
        regval32 = __bim_readl(REG_RW_MINTEN);
        regval32 &= ~misc_irq;
        __bim_writel(regval32, REG_RW_MINTEN);
        //spin_unlock_irqrestore(&mt53xx_bim_lock, flags);
    }
    else
    {
        //spin_lock_irqsave(&mt53xx_bim_lock, flags);
        regval32 = __bim_readl(REG_RW_IRQEN);
        regval32 &= ~(1 << (irq-SPI_OFFSET));
        __bim_writel(regval32, REG_RW_IRQEN);
        //spin_unlock_irqrestore(&mt53xx_bim_lock, flags);
    }
}

static void mt53xx_irq_unmask(struct irq_data *data)
{
    u32 regval32;
    //unsigned long flags;
    unsigned int irq = data->irq;

    if (irq < SPI_OFFSET)
    {
       return;
    }

#ifdef CC_VECTOR_HAS_MISC3
    if (irq >= VECTOR_MISC3_BASE)
    {
        u32 misc_irq;
        misc_irq = _MISC3IRQ(irq);
        //spin_lock_irqsave(&mt53xx_bim_lock, flags);
        regval32 = __bim_readl(REG_RW_M3INTEN);
        regval32 |= (misc_irq);
        __bim_writel(regval32, REG_RW_M3INTEN);
        //spin_unlock_irqrestore(&mt53xx_bim_lock, flags);
    }
    else
#endif
    if (irq >= VECTOR_MISC2_BASE)
    {
        u32 misc_irq;
        misc_irq = _MISC2IRQ(irq);
        //spin_lock_irqsave(&mt53xx_bim_lock, flags);
        regval32 = __bim_readl(REG_RW_M2INTEN);
        regval32 |= (misc_irq);
        __bim_writel(regval32, REG_RW_M2INTEN);
        //spin_unlock_irqrestore(&mt53xx_bim_lock, flags);
    }
    else if (irq >= VECTOR_MISC_BASE)
    {
        u32 misc_irq;
        misc_irq = _MISCIRQ(irq);
        //spin_lock_irqsave(&mt53xx_bim_lock, flags);
        regval32 = __bim_readl(REG_RW_MINTEN);
        regval32 |= (misc_irq);
        __bim_writel(regval32, REG_RW_MINTEN);
        //spin_unlock_irqrestore(&mt53xx_bim_lock, flags);
    }
    else
    {
        //spin_lock_irqsave(&mt53xx_bim_lock, flags);
        regval32 = __bim_readl(REG_RW_IRQEN);
        regval32 |= (1 << (irq-SPI_OFFSET));
        __bim_writel(regval32, REG_RW_IRQEN);
        //spin_unlock_irqrestore(&mt53xx_bim_lock, flags);
    }
}


static void xgpt_init(void)
{
    printk("p_mcusys: 0x%lx\n", p_mcusys_iomap);

    __raw_writel(0, p_mcusys_iomap + 0x674);
    __raw_writel(1, p_mcusys_iomap + 0x670);
/*
    __raw_writel(0xffffffff, p_mcusys + 0x620);
    __raw_writel(0xffffffff, p_mcusys + 0x624);
    __raw_writel(0xffffffff, p_mcusys + 0x628);
    __raw_writel(0xffffffff, p_mcusys + 0x62c);
    __raw_writel(0xffffffff, p_mcusys + 0x630);
    __raw_writel(0xffffffff, p_mcusys + 0x634);
    __raw_writel(0xffffffff, p_mcusys + 0x638);
    __raw_writel(0xffffffff, p_mcusys + 0x63c);
*/
}

static void irq_init(void)
{
    //p_bim_iomap = ioremap(0xf0000000, 0x100000);
    //printk("mtk_iomap: 0x%lx\n", p_bim_iomap);
    //p_bim_iomap = tv_reg_base + 0x8000;

    __raw_writel(0x7a, p_bim_iomap + 0x98);

#ifdef CONFIG_ARM_GIC
    gic_arch_extn.irq_eoi = bim_irq_ack;
    gic_arch_extn.irq_mask = mt53xx_irq_mask;
    gic_arch_extn.irq_unmask = mt53xx_irq_unmask;
#endif

}

static void  __init mt53xx_dt_init(void)
{
    struct device_node *np;

    np = of_find_compatible_node(NULL, NULL, "Mediatek, TVBIM");
    if (np == NULL) {
	printk("please define tv register base address\n");
	BUG();
    }
    tv_reg_base = of_iomap(np, 0);
    printk("tv_reg_base: 0x%p\n", tv_reg_base);
    p_bim_iomap = tv_reg_base + 0x8000;
    p_pdwnc_iomap = tv_reg_base + 0x28000;
    p_mcusys_iomap = tv_reg_base + 0x78000;
	p_ckgen_iomap = tv_reg_base + 0xd000;
}
EXPORT_SYMBOL(tv_reg_base);

static int mt53xx_soc_init(struct platform_device *pdev)
{
    printk("mt53xx_soc_init...\n");

    mt53xx_dt_init();
    xgpt_init();

    irq_init();

	return 0;
}
core_initcall(mt53xx_soc_init);

