#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ioport.h>
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
#include "mtk_sched.h"

static void mtk_remove_fifo_node(struct fifo_node *node, struct fifo_node *prev)
{
    prev->next = node->next;
    kfree(node);
}

uint16_t mtk_get_fifo(MGC_LinuxCd *pThis, uint16_t size)
{
    //Using First-Fit
    struct fifo_node *node, *prev_node;
    uint16_t addr = 0;

    for (prev_node = &pThis->fifo_list_head, node = prev_node->next; node; 
	prev_node = node, node = node->next) {
	if (node->size > size) {
	    addr = node->pAddr;
	    node->pAddr += size;
	    node->size -= size;
	    return addr;
	} else if (node->size == size) {
	    addr = node->pAddr;
	    mtk_remove_fifo_node(node, prev_node);
	    return addr;
	}
    }

    return 0;
}

int mtk_put_fifo(MGC_LinuxCd *pThis, uint16_t addr, int size)
{
    struct fifo_node *node, *prev_node, *new_node;
    
    /* Find the insert pos */
    for (prev_node = &pThis->fifo_list_head, node = prev_node->next;
	node && node->pAddr < addr; prev_node = node, node = node->next) {
    }
    new_node = kzalloc(sizeof(*new_node), GFP_ATOMIC); 
    if (NULL == new_node) {
	INFO("Allocate Fifo Node error\n");
	return -1;
    }
    new_node->pAddr = addr;
    new_node->size = size;
    prev_node->next = new_node;
    new_node->next = node;

    // Check if the adjacent interval can be combined
    for (prev_node = &pThis->fifo_list_head, node = prev_node->next; node; 
	prev_node = node, node = node->next) {
	if (prev_node != &pThis->fifo_list_head && 
	    prev_node->pAddr + prev_node->size == node->pAddr) {
	    prev_node->size += node->size;
	    prev_node->next = node->next;
	    kfree(node);
	    node = prev_node;
	}
    }
    return 0;
}

void mtk_dump_fifo_node(MGC_LinuxCd *pThis)
{
    struct fifo_node *node;
    printk("\nPort-%d Free Fifo ", pThis->bPortNum);
    for (node = pThis->fifo_list_head.next; node; node = node->next) {
	printk("[0X%08X, 0X%08x] ", node->pAddr, node->pAddr + node->size - 1);
    }
    printk("\n");
}

#ifdef CONFIG_USB_QMU_SUPPORT
static uint8_t MGC_IsCmdQUsed(MGC_LinuxCd *pThis, struct urb *pUrb)
{
	uint32_t nPipe;
	uint8_t bXmt;
	
	if ((!pThis)||(!pUrb))
	{
		return 0;
	}

	if(!MGC_DeviceQmuSupported(pUrb))
	{
		return 0;
	}
	nPipe = pUrb->pipe;
	bXmt = usb_pipeout(nPipe);

	if (usb_pipecontrol(nPipe))
	{
		return 0;
	}	 

	if ((pThis->bSupportCmdQ)&& (!bXmt) && usb_pipeisoc(nPipe))
	{
		return 1;
	}

	return 0;
}
#endif
/*
 * For RX Qmu, we must use DMA channel, but only the front of few endpoints have
 * DMA channel.
 * */
uint8_t mtk_select_endpoint(MGC_LinuxCd *pThis, struct urb *pUrb)
{
    int i;
    uint8_t bQmuUsed = 0;
    uint8_t bEndCount = 0;
    uint8_t bReverse = 0;
    uint8_t bEnd = 0;
    uint8_t bOut = usb_pipeout(pUrb->pipe);
#ifdef CONFIG_USB_QMU_SUPPORT
    bQmuUsed = MGC_IsCmdQUsed(pThis, pUrb);
#endif
    bEndCount = MTK_MAX(pThis->bEndTxCount, pThis->bEndRxCount);
    bReverse = !(bQmuUsed && bOut == 0);

    if (bReverse) {
	for (i = bEndCount - 1; i > 0; i--) {
	    if (pThis->aLocalEnd[bOut][i].wMaxPacketSize == 0) {
		bEnd = i;
		break;
	    }
	}
    } else {
	for (i = 1; i < bEndCount; i++) {
	    if (pThis->aLocalEnd[bOut][i].wMaxPacketSize == 0) {
		bEnd = i;
		break;
	    }
	}
    }

    return bEnd;
}
