/*
 * linux/arch/arm/mach-mt53xx/mtk_pcimsi.c
 *
 * Debugging macro include header
 * PCI and PCIe functions for MTK System On Chip
 *
 * Copyright (c) 2010-2012 MediaTek Inc.
 * Maintainer: Max Liao <max.liao@mediatek.com>
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

#ifdef CONFIG_PCI_MSI

#include <linux/pci.h>
#include <linux/msi.h>
#include <asm/mach/irq.h>
#include <asm/irq.h>
#include <plat/mtk_pci.h>

#define MTK_NUM_MSI_IRQS 32
static DECLARE_BITMAP(msi_irq_in_use, MTK_NUM_MSI_IRQS);

static void _msi_nop(unsigned int irq)
{
    return;
}

static struct irq_chip MTK_msi_chip = {
    .name = "PCI-MSI",
    .ack = _msi_nop,
    .enable = unmask_msi_irq,
    .disable = mask_msi_irq,
    .mask = mask_msi_irq,
    .unmask = unmask_msi_irq,
};

/*
 * Dynamic irq allocate and deallocation
 */
int create_irq(void)
{
    int irq, pos;

again:
    pos = find_first_zero_bit(msi_irq_in_use, MTK_NUM_MSI_IRQS);
    irq = IRQ_PCIE_MSI + pos;
    if (irq > NR_IRQS)
	return -ENOSPC;
    /* test_and_set_bit operates on 32-bits at a time */
    if (test_and_set_bit(pos, msi_irq_in_use))
	goto again;

    dynamic_irq_init(irq);

    return irq;
}

void destroy_irq(unsigned int irq)
{
    int pos = irq - IRQ_PCIE_MSI;

    dynamic_irq_cleanup(irq);

    clear_bit(pos, msi_irq_in_use);
}

void MTK_teardown_msi_irq(unsigned int irq)
{
    destroy_irq(irq);
}

int MTK_setup_msi_irq(struct pci_dev *pdev, struct msi_desc *desc)
{
    int irq = create_irq();
    struct msi_msg msg;

    if (irq < IRQ_PCIE_MSI)
	return irq;

    set_irq_msi(irq, desc);

    msg.address_hi = 0x0;
    msg.address_lo = PLDA_MSI_ADDR_SETTING;
    msg.data = irq - IRQ_PCIE_MSI;

    write_msi_msg(irq, &msg);
    set_irq_chip_and_handler(irq, &MTK_msi_chip, handle_simple_irq);

    return 0;
}

/* MSI MTK arch hooks */
int MTK_msi_check_device(struct pci_dev *dev, int nvec, int type)
{
    if (type == PCI_CAP_ID_MSIX)
    {
        LOG("MTK do not support MSIX !\n");
    }

    return (type == PCI_CAP_ID_MSI) ? 0 : (-EINVAL);
}

int MTK_setup_msi_irqs(struct pci_dev *dev, int nvec, int type)
{
    struct msi_desc *entry;
    int ret;

    if (type == PCI_CAP_ID_MSIX)
    {
        LOG("MTK do not support MSIX !\n");
        return (-EINVAL);
    }

    // Only support one MSI.
    if (type == PCI_CAP_ID_MSI && nvec > 1)
        return 1;

    list_for_each_entry(entry, &dev->msi_list, list) 
    {
        ret = MTK_setup_msi_irq(dev, entry);
        if (ret < 0)
        	return ret;
        if (ret > 0)
        	return -ENOSPC;
    }

    return 0;
}

void MTK_teardown_msi_irqs(struct pci_dev *dev)
{
	struct msi_desc *entry;

	list_for_each_entry(entry, &dev->msi_list, list) {
		int i, nvec;
		if (entry->irq == 0)
			continue;
		nvec = 1 << entry->msi_attrib.multiple;
		for (i = 0; i < nvec; i++)
			MTK_teardown_msi_irq(entry->irq + i);
	}
}

#endif /* #ifdef CONFIG_PCI_MSI */

