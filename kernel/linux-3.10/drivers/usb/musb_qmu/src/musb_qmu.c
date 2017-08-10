/*-----------------------------------------------------------------------------
 * drivers/usb/musb_qmu/musb_qmu.c
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

#ifdef CONFIG_USB_QMU_SUPPORT 
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/list.h>

#include "musb_core.h"
#include "musb_host.h"
#include <linux/timer.h>
#include <linux/spinlock.h>
#include <linux/stat.h>

#include "mtk_qmu_api.h"
#include "musb_qmu.h"
#include "mt8560.h"
#include "musbhsdma.h"

extern void musb_giveback(struct musb *musb, struct urb *urb, int status);
extern void musb_ep_set_qh(struct musb_hw_ep *ep, int is_in, struct musb_qh *qh);
extern void musb_save_toggle(struct musb_qh *qh, int is_in, struct urb *urb);
extern struct musb_qh *musb_ep_get_qh(struct musb_hw_ep *ep, int is_in);

//Mtk_USB_Result pMtk_usb_result[2][8];
unsigned mtk_usb_debug=10;

#ifdef CONFIG_USB_GADGET_MUSB_HDRC
/// @brief Gadget QMU start a Tx usb request
/// .
/// @param musb:   struct musb
/// @param req: the usb request of gadget
static inline void mtk_q_txstate(struct musb *musb, struct musb_request *req){
    u8            epnum = req->epnum;
    struct musb_ep        *musb_ep;
    struct usb_request    *request;
    void __iomem        *epio = musb->endpoints[epnum].regs;
    u8            isRx=false;

    musb_ep = req->ep;
    request = &req->request;
    request->actual=0;
    musb_qmu_insert_task(musb,epnum, isRx, request->buf, request->length, true);

    /* host may already have the data when this message shows... */
    DBG(3, "%s TX/IN %s len %d/%d, TxMaxP %d\n",
            musb_ep->end_point.name,  "DMAQ",
            request->actual, request->length,
            musb_readw(epio, MUSB_TXMAXP));
}

/// @brief Gadget QMU start a Rx usb request
/// .
/// @param musb:   struct musb
/// @param req: the usb request of gadget
static inline void mtk_q_rxstate(struct musb *musb, struct musb_request *req){
    const u8        epnum = req->epnum;
    struct usb_request    *request = &req->request;
    struct musb_ep        *musb_ep = &musb->endpoints[epnum].ep_out;
    void __iomem        *epio = musb->endpoints[epnum].regs;
    u8            isRx=true;

    request->actual=0;
    musb_qmu_insert_task(musb,epnum, isRx, request->buf, request->length, true);

    DBG(3, "%s RX/OUT %s len %d/%d, RxMaxP %d\n",
            musb_ep->end_point.name, "DMAQ",
            request->actual, request->length,
            musb_readw(epio, MUSB_RXMAXP));
}
#endif

#ifdef CONFIG_USB_MUSB_HDRC_HCD
/// @brief musb QMU advance schedule, urb giveback
/// .
/// @param musb:   struct musb
/// @param urb: urb which will giveback
/// @param hw_ep: the endpoint of urb
/// @param is_in:the direction of urb
void mtk_q_advance_schedule(struct musb *musb, struct urb *urb, struct musb_hw_ep *hw_ep, int is_in)
{
    struct musb_qh        *qh = musb_ep_get_qh(hw_ep, is_in);
    struct musb_hw_ep    *ep = qh->hw_ep;
    int            ready = qh->is_ready;
    int            status = 0;
#ifdef CONFIG_MT7118_HOST
    struct dma_controller *dma_controller;
    struct dma_channel *dma_channel;
#endif

    status = (urb->status == -EINPROGRESS) ? 0 : urb->status;

    /* save toggle eagerly, for paranoia */
    switch (qh->type) {
    //CC: do we need to save toggle for Q handled bulk transfer?
    case USB_ENDPOINT_XFER_BULK:
        musb_save_toggle(qh, is_in, urb);
        break;
    case USB_ENDPOINT_XFER_ISOC:
        if (status == 0 && urb->error_count)
            status = -EXDEV;
        break;
    }

#ifdef CONFIG_MT7118_HOST
/* candidate for DMA? */
	dma_controller = musb->dma_controller;
    dma_channel = is_in ? ep->rx_channel : ep->tx_channel;
    if(dma_channel)
    {
		dma_controller->channel_release(dma_channel);
		dma_channel=NULL;
		if(is_in)
		    ep->rx_channel=NULL;
		else
			ep->tx_channel=NULL;
    }
#endif

    qh->is_ready = 0;
    musb_giveback(musb, urb, status);
    qh->is_ready = ready;

    /* reclaim resources (and bandwidth) ASAP; deschedule it, and
     * invalidate qh as soon as list_empty(&hep->urb_list)
     */
#if 1 
    if (list_empty(&qh->hep->urb_list)) {

        if (is_in)
            ep->rx_reinit = 1;
        else
            ep->tx_reinit = 1;
        
        /* Clobber old pointers to this qh */
        musb_ep_set_qh(ep, is_in, NULL);
        qh->hep->hcpriv = NULL;

        switch (qh->type) {
        case USB_ENDPOINT_XFER_BULK:
            /* fifo policy for these lists, except that NAKing
             * should rotate a qh to the end (for fairness).
             */
            kfree(qh);
            /* We don't use this list, just free qh
            if (qh->mux == 1) {
                head = qh->ring.prev;
                list_del(&qh->ring);
                kfree(qh);
                qh = first_qh(head);
                break;
            }
            */
            break;
         case USB_ENDPOINT_XFER_ISOC:
            kfree(qh);
            qh = NULL;
            break;
        }
    }
#endif
}
#endif

/// @brief musb Flush Endpoint for QMU
/// .
/// @param musb:   struct musb
/// @param EP_Num: Endpoint number mapping Queue
/// @param isRx: RxQ or TxQ
/// @param type: Transfer Type for the selected Queue
/// @param isHost: If the usb is host mode
static void flush_ep_csr(struct musb *musb, u8 EP_Num, u8 isRx, bool isHost){
    void __iomem        *mbase = musb->mregs;
    struct musb_hw_ep    *hw_ep = musb->endpoints + EP_Num;
    void __iomem        *epio = hw_ep->regs;
    u16 csr, wCsr;


    if (epio == NULL)
        printk(KERN_ALERT "epio == NULL\n");
    if (hw_ep == NULL)
        printk(KERN_ALERT "hw_ep == NULL\n");

    if (isRx)
    {
        csr = musb_readw(epio, MUSB_RXCSR);
        csr |= MUSB_RXCSR_FLUSHFIFO | MUSB_RXCSR_RXPKTRDY;
        if (isHost)
            csr &= ~MUSB_RXCSR_H_REQPKT;

        /* write 2x to allow double buffering */
        //CC: see if some check is necessary
        musb_writew(epio, MUSB_RXCSR, csr);
        musb_writew(epio, MUSB_RXCSR, csr | MUSB_RXCSR_CLRDATATOG);
    }
    else
    {
        csr = musb_readw(epio, MUSB_TXCSR);
        if (csr&MUSB_TXCSR_TXPKTRDY)
        {
            wCsr = csr | MUSB_TXCSR_FLUSHFIFO | MUSB_TXCSR_TXPKTRDY;
            musb_writew(epio, MUSB_TXCSR, wCsr);
        }
        MU_MB();
        csr |= MUSB_TXCSR_FLUSHFIFO&~MUSB_TXCSR_TXPKTRDY;
        musb_writew(epio, MUSB_TXCSR, csr);
        musb_writew(epio, MUSB_TXCSR, csr | MUSB_TXCSR_CLRDATATOG);
        //CC: why is this special?
        MU_MB();
        musb_writew(mbase, MUSB_INTRTX, 1<<EP_Num);
    }
}

void mtk_q_dma_select (struct musb *musb, u8 channel, u8 burstmode)
{
    void __iomem        *mbase = musb->mregs;
    u32 ctrl;

	ctrl = musb_readw(mbase,MUSB_HSDMA_CFG);

	musb_writew(mbase,MUSB_HSDMA_CFG, ctrl | channel<<4);

	//burst mode
	ctrl = musb_readw(mbase,
		MUSB_HSDMA_CHANNEL_OFFSET(channel, MUSB_HSDMA_CONTROL));
		
	musb_writew(mbase,
		MUSB_HSDMA_CHANNEL_OFFSET(channel, MUSB_HSDMA_CONTROL), ctrl | burstmode<<9);

}

int mtk_enable_q(struct musb *musb, u8 EP_Num, u8 isRx, u8 type, u16 MaxP, u8 interval, u8 target_ep, u8 isZLP, u8 isCSCheck, u8 isEmptyCheck, u8 hb_mult)
{
    void __iomem        *mbase = musb->mregs;
    struct musb_hw_ep    *hw_ep = musb->endpoints + EP_Num;
    void __iomem        *epio = hw_ep->regs;
    u16    csr = 0;
    bool isHost = musb->is_host;
    u16 intr_e = 0;
	int status = 0;

    DBG(LOG_QMU, "select target_ep: %d\n", target_ep);
    DBG(LOG_QMU, "select hw_ep: %d\n", EP_Num);

    musb_ep_select(mbase, EP_Num);

    flush_ep_csr(musb, EP_Num,  isRx, isHost);

    if (isRx)
    {
    	
	if (type == USB_ENDPOINT_XFER_ISOC)
       {
 		if(hb_mult == 3)
                	musb_writew(epio, MUSB_RXMAXP, MaxP|0x1000);
		else if(hb_mult == 2)
			musb_writew(epio, MUSB_RXMAXP, MaxP|0x800);
		else
			musb_writew(epio, MUSB_RXMAXP, MaxP);
       }
	   else
       	musb_writew(epio, MUSB_RXMAXP, MaxP);

		
        if (isHost)
        {
            musb_writew(epio, MUSB_RXCSR, MUSB_RXCSR_DMAENAB);
            //CC: speed?
            musb_writeb(epio, MUSB_RXTYPE, type<<4 | target_ep);
            musb_writeb(epio, MUSB_RXINTERVAL, interval);
        }
        else
        {
            csr |= MUSB_RXCSR_DMAENAB;
            if (type == USB_ENDPOINT_XFER_ISOC)
            {
                csr |= MUSB_RXCSR_P_ISO;
            }
            else
            {
                csr &= ~MUSB_RXCSR_P_ISO;
                csr &= ~MUSB_RXCSR_DISNYET;
            }

            musb_writew(epio, MUSB_RXCSR, csr);
        }
#ifdef CONFIG_USB_MUSB_HDRC_HCD
        musb_writeb(mbase, MUSB_RXFADDR(EP_Num), hw_ep->in_qh->addr_reg);
#endif
        
        //turn off intrRx
        intr_e = musb_readw(mbase, MUSB_INTRRXE);
        intr_e = intr_e & (~(1<<EP_Num));
        musb_writew(mbase, MUSB_INTRRXE, intr_e);
    }
    else
    {
        musb_writew(epio, MUSB_TXMAXP, MaxP);

        if (isHost)
        {
            musb_writew(epio, MUSB_TXCSR, MUSB_TXCSR_DMAENAB);
            //CC: speed?
            musb_writeb(epio, MUSB_TXTYPE, type<<4 | target_ep);
            musb_writeb(epio, MUSB_TXINTERVAL, interval);
        }
        else
        {
            csr |= MUSB_TXCSR_DMAENAB;
            if (type==USB_ENDPOINT_XFER_ISOC)
                csr |= MUSB_TXCSR_P_ISO;
            else
                csr &= ~MUSB_TXCSR_P_ISO;

            musb_writew(epio, MUSB_TXCSR, csr);
        }
#ifdef CONFIG_USB_MUSB_HDRC_HCD
        musb_writeb(mbase, MUSB_TXFADDR(EP_Num), hw_ep->out_qh->addr_reg);
#endif
        //turn off intrTx
        intr_e = musb_readw(mbase, MUSB_INTRTXE);
        intr_e = intr_e & (~(1<<EP_Num));
        musb_writew(mbase, MUSB_INTRTXE, intr_e);
    }

    status =  musb_qmu_enable(musb,EP_Num, isRx, isZLP, isCSCheck, isEmptyCheck);
	q_disabled = false;
	return status;
}

void mtk_disable_q_all(struct musb *musb){
    //unsigned long flags;
    u32 EP_Num = 0;
	//spin_lock_irqsave(&musb->lock, flags);
    //memset(ep_bind, 0, ENQUEUE_EP_NUM);
    for(EP_Num = 1; EP_Num <= RXQ_NUM; EP_Num++){
        if(musb_is_qmu_enabled(musb,EP_Num, 1))
            mtk_disable_q(musb, EP_Num, 1);
    }
    for(EP_Num = 1; EP_Num <= TXQ_NUM; EP_Num++){
        if(musb_is_qmu_enabled(musb,EP_Num, 0))
            mtk_disable_q(musb, EP_Num, 0);
    }
	q_disabled = true;
	//spin_unlock_irqrestore(&musb->lock, flags);
}

void mtk_disable_q(struct musb *musb, u8 EP_Num, u8 isRx){
    void __iomem        *mbase = musb->mregs;
    struct musb_hw_ep    *hw_ep = musb->endpoints + EP_Num;
    void __iomem        *epio = hw_ep->regs;
    u16    csr;
    //unsigned long flags;
    bool isHost = musb->is_host;

    //spin_lock_irqsave(&musb->lock, flags);
    musb_qmu_disable(musb,EP_Num, isRx);
    musb_ep_select(mbase, EP_Num);
    if(isRx){
        csr = musb_readw(epio, MUSB_RXCSR);
        csr &= ~MUSB_RXCSR_DMAENAB;
        musb_writew(epio, MUSB_RXCSR, csr);
        flush_ep_csr(musb, EP_Num,  isRx, isHost);
    }else{
        csr = musb_readw(epio, MUSB_TXCSR);
        csr &= ~MUSB_TXCSR_DMAENAB;
        musb_writew(epio, MUSB_TXCSR, csr);
        flush_ep_csr(musb, EP_Num,  isRx, isHost);
    }
    //spin_unlock_irqrestore(&musb->lock, flags);
}

#ifdef CONFIG_USB_GADGET_MUSB_HDRC
int mtk_start_request(struct musb *musb, struct musb_request *req){
    DBG(3, "<== %s request %p len %u on hw_ep%d\n",
        req->tx ? "TX/IN" : "RX/OUT",
        &req->request, req->request.length, req->epnum);

    musb_ep_select(musb->mregs, req->epnum);
    if (req->tx){
        mtk_q_txstate(musb, req);
    }else{
        mtk_q_rxstate(musb, req);
    }
    return 0;
}
#endif

#if 0
int mtk_remove_request(struct usb_request request){
    return 0;
}
#endif

//int Mtk_Cleanup_Q(struct musb *musb, u8 EP_Num, u8 isRx);

void mtk_stop_q(struct musb* musb,u8 EP_Num, u8 isRx){
    musb_qmu_stop(musb,EP_Num, isRx);
}

int mtk_restart_q(struct musb *musb, u8 EP_Num, u8 isRx){
    void __iomem *mbase = musb->mregs;
    bool isHost = musb->is_host;
    musb_ep_select(mbase, EP_Num);
    flush_ep_csr(mbase, EP_Num,  isRx, isHost);
    musb_qmu_cleanup(musb,EP_Num, isRx);
    return musb_qmu_restart(musb,EP_Num, isRx);
}


int mtk_q_remove_urb(struct musb *musb,struct urb* urb,u8 EP_Num,bool isRx,u8* irq_cb){

    u8 do_stop = false;
	u8* buf = NULL;

    MUSB_ASSERT(urb);
	buf = (u8*)urb->transfer_dma;
	if(buf == NULL)
		buf = virt_to_phys(urb->transfer_buffer);

	MUSB_ASSERT(buf);
	if(musb_qmu_remove_task(musb,EP_Num,isRx,buf,urb->transfer_buffer_length,irq_cb,&do_stop))
	{
	  return 1;
	}

	if(do_stop)
		mtk_restart_q(musb,EP_Num,isRx);
	
	return 0;
}

irqreturn_t mtk_q_interrupt(struct musb *musb){
    irqreturn_t    retval = IRQ_NONE;
    u8 RxIndex;
    u8 TxIndex;
    bool isHost = musb->is_host;

    //CC: some way to reduce uncessary function calls?
    if (musb_qmu_irq(musb,musb->int_queue, musb->pMtk_usb_result))
    {
        for (RxIndex=1; RxIndex<=RXQ_NUM; RxIndex++)
        {
            //CC: add this to save efforts
            if (musb->pMtk_usb_result[RXQ][RxIndex].status || musb->pMtk_usb_result[RXQ][RxIndex].number_of_sdu)
            {
                if(isHost)
                {
#ifdef CONFIG_USB_MUSB_HDRC_HCD
                    mtk_q_host_rx(musb, RxIndex);
#endif
                }
                else
                {
#ifdef CONFIG_USB_GADGET_MUSB_HDRC
                    mtk_q_gagdet_rx(musb, RxIndex);
#endif
                }
            }
        }

        for (TxIndex=1; TxIndex<=TXQ_NUM; TxIndex++)
        {
            //CC: add this to save efforts
            if (musb->pMtk_usb_result[TXQ][TxIndex].status || musb->pMtk_usb_result[TXQ][TxIndex].number_of_sdu)
            {
                if(isHost)
                {
#ifdef CONFIG_USB_MUSB_HDRC_HCD
                    mtk_q_host_tx(musb, TxIndex);
#endif
                }
                else
                {
#ifdef CONFIG_USB_GADGET_MUSB_HDRC
                    mtk_q_gagdet_tx(musb, TxIndex);
#endif
                }
            }
        }
    }

    return retval;
}


#ifdef CONFIG_USB_MUSB_HDRC_HCD
void mtk_q_host_rx(struct musb *musb, u8 epnum){
    struct urb        *urb=NULL;
    struct musb_hw_ep    *hw_ep = musb->endpoints + epnum;
    void __iomem        *epio = hw_ep->regs;
    struct musb_qh        *qh = hw_ep->in_qh;
    size_t            xfer_len;
    void __iomem        *mbase = musb->mregs;
    int            pipe;
    u16            rx_csr;
    u16            ep_type;
    bool            done = false;
    bool            isRx=true;
    int            status=MTK_NO_ERROR;
    Mtk_USB_Result    pResult = musb->pMtk_usb_result[isRx][epnum];
    PMtk_SDU_Result    sdu_result = pResult.link_header;

    musb_ep_select(mbase, epnum);
    rx_csr = musb_readw(epio, MUSB_RXCSR);
    ep_type = musb_readw(epio, MUSB_RXTYPE);
    if((ep_type>>4)==USB_ENDPOINT_XFER_ISOC){
        if (rx_csr & MUSB_RXCSR_DATAERROR) {
            printk(KERN_ALERT  "RX end %d ISO data error\n", epnum);
            /* packet error reported later */
            urb->error_count = true;
            //status=-EPROTO;
            rx_csr |= MUSB_RXCSR_H_WZC_BITS;
            rx_csr &= ~MUSB_RXCSR_DATAERROR;
            musb_writew(epio, MUSB_RXCSR, rx_csr);
        }else if (rx_csr & MUSB_RXCSR_INCOMPRX&& pResult.status!=MTK_BUS_ERROR) {
            printk(KERN_ALERT "end %d high bandwidth incomplete ISO packet RX\n",
                    epnum);
            //status = -EPROTO;
            rx_csr |= MUSB_RXCSR_H_WZC_BITS;
            rx_csr &= ~MUSB_RXCSR_INCOMPRX;
            musb_writew(epio, MUSB_RXCSR, rx_csr);
        }else if (rx_csr & MUSB_RXCSR_PID_ERR) {
            printk(KERN_ALERT "end %d PID Error ISO packet RX\n",
                    epnum);
            status = -EILSEQ;
            rx_csr |= MUSB_RXCSR_H_WZC_BITS;
            rx_csr &= ~MUSB_RXCSR_PID_ERR;
            musb_writew(epio, MUSB_RXCSR, rx_csr);
        }else{
            status= -EINPROGRESS;
        }
    }

    while(pResult.number_of_sdu--){

        done = false;
        urb = next_urb(qh);
        xfer_len = 0;
        if (unlikely(!urb)) {
            DBG(3, "BOGUS RX%d ready\n", epnum);
            mtk_stop_q(musb,epnum, isRx);
            return;
        }
        pipe = urb->pipe;
        if (usb_pipeisoc(pipe)) {
            struct usb_iso_packet_descriptor    *d;
            d = urb->iso_frame_desc + qh->iso_idx;
            d->actual_length = sdu_result->actual_length;
            urb->actual_length += sdu_result->actual_length;
            qh->offset += sdu_result->actual_length;
            if(status){
                urb->error_count++;
                //Chiachun: MTK status could mapping to descriptor status?
                d->status = status;
            }
            else{
                d->status = 0;
            }
            if (++qh->iso_idx >= urb->number_of_packets) {
                done = true;
//                urb->status = 0;
            }
        }else{
            done=true;

            if(sdu_result->actual_length == 65024 &&  urb->transfer_buffer_length == 65536){
                    urb->actual_length = 65536;
                    qh->offset = 65536;
            }
            else if(urb->transfer_buffer_length == 65536 && sdu_result->actual_length == 512){
                done = false;
            }
            else if(urb->transfer_buffer_length == 65536){
                urb->actual_length = sdu_result->actual_length;
                qh->offset = sdu_result->actual_length;
            }
            else{
                urb->actual_length = sdu_result->actual_length;
                qh->offset = sdu_result->actual_length;
            }
        }
        if(done){
            mtk_q_advance_schedule(musb, urb, hw_ep, USB_DIR_IN);
            qh->iso_idx = 0;
        }
        sdu_result = sdu_result->next;
    }

    if(pResult.status){        /// issue: if isochronous Module error or length error, maybe need cleanup gpd.
        mtk_stop_q(musb,epnum, isRx);

        urb = next_urb(qh);
        switch(pResult.status){
            case MTK_MODULE_ERROR:
                urb->status = -EINPROGRESS;
                break;
            case MTK_LENGTH_ERROR:
                urb->status = -EOVERFLOW;
                break;
            case MTK_BUS_ERROR:
                if (rx_csr & MUSB_RXCSR_H_RXSTALL) {
                    printk(KERN_ALERT "RX end %d STALL\n", epnum);
                    /* stall; record URB status */
                    urb->status = -EPIPE;
                    rx_csr |= MUSB_RXCSR_H_WZC_BITS;
                    rx_csr &= ~MUSB_RXCSR_H_RXSTALL;
                } else if (rx_csr & MUSB_RXCSR_H_ERROR) {
           //         printk(KERN_ALERT "end %d RX proto error\n", epnum);
                    urb->status = -EPROTO;
                    rx_csr |= MUSB_RXCSR_H_WZC_BITS;
                    rx_csr &= ~MUSB_RXCSR_H_ERROR;
                } else if (rx_csr & MUSB_RXCSR_DATAERROR) {
                    printk(KERN_ALERT "RX end %d NAK timeout\n", epnum);
                    urb->status = -ETIMEDOUT;
                    rx_csr |= MUSB_RXCSR_H_WZC_BITS;
                    rx_csr &= ~MUSB_RXCSR_DATAERROR;
                }  else if (rx_csr & MUSB_RXCSR_INCOMPRX) {
                    printk(KERN_ALERT "RX end %d No Response\n", epnum);
                    urb->status = -EPROTO;
                    rx_csr |= MUSB_RXCSR_H_WZC_BITS;
                    rx_csr &= ~MUSB_RXCSR_INCOMPRX;
                }
                musb_writew(epio, MUSB_RXCSR, rx_csr);
                break;
            default:
                urb->status = -EINPROGRESS;
                break;
        }
        if(urb->status){
            mtk_q_advance_schedule(musb, urb, hw_ep, USB_DIR_IN);
        }
    }
    pResult.status=MTK_NO_ERROR;
}

void mtk_q_host_tx(struct musb *musb, u8 epnum){
    int            pipe;
    bool        done = false;
    u16            tx_csr;
    u8            isRx=false;
    struct urb    *urb=NULL;
    struct musb_hw_ep    *hw_ep = musb->endpoints + epnum;
    void __iomem        *epio = hw_ep->regs;
    void __iomem        *mbase = musb->mregs;
    struct musb_qh        *qh = hw_ep->out_qh;

    int status = MTK_NO_ERROR;
    Mtk_USB_Result pResult = musb->pMtk_usb_result[isRx][epnum];
    PMtk_SDU_Result sdu_result = pResult.link_header;

    musb_ep_select(mbase, epnum);
    tx_csr = musb_readw(epio, MUSB_TXCSR);

    while (pResult.number_of_sdu--)
    {
        done = false;
        //CC: what is this???
        urb = next_urb(qh);
        status = 0;
        if (!urb) {
            DBG(4, "extra TX%d ready\n", epnum);
            return;
        }
        pipe=urb->pipe;

        //CC: supports for only isochronous and bulk
        //CC: disable isochronous support temporarily

#if 0
        if (usb_pipeisoc(pipe))
        {
            struct usb_iso_packet_descriptor    *d;
            d = urb->iso_frame_desc + qh->iso_idx;

            if (++qh->iso_idx >= urb->number_of_packets) {
                done = true;
                qh->iso_idx = 0;
            }
        }
        else
        #endif
        {
            done = true;
            urb->status = status;
            urb->actual_length = sdu_result->actual_length;

        }

        if (done)
        {
            mtk_q_advance_schedule(musb, urb, hw_ep, USB_DIR_OUT);
        }
        sdu_result = sdu_result->next;
    }

    if(pResult.status){  /// issue: if isochronous Module error or length error, maybe need cleanup gpd.
        mtk_stop_q(musb,epnum, isRx);
//        printk(KERN_ALERT "HOST TX ERROR\n");
        urb = next_urb(qh);
        pipe=urb->pipe;
        switch(pResult.status){
            case MTK_MODULE_ERROR:
            case MTK_LENGTH_ERROR:
                urb->status = -EINPROGRESS;
                break;
            case MTK_BUS_ERROR:
                if (tx_csr & MUSB_TXCSR_H_RXSTALL) {
                    /* dma was disabled, fifo flushed */
                    printk(KERN_ALERT "TX end %d stall\n", epnum);

                    /* stall; record URB status */
                    urb->status = -EPIPE;

                } else if (tx_csr & MUSB_TXCSR_H_ERROR) {
                    /* (NON-ISO) dma was disabled, fifo flushed */
                    printk(KERN_ALERT "TX 3strikes on ep=%d\n", epnum);

                    urb->status = -ETIMEDOUT;

                } else if (tx_csr & MUSB_TXCSR_H_NAKTIMEOUT) {
                    printk(KERN_ALERT "TX end=%d device not responding\n", epnum);

                    urb->status = -ETIMEDOUT;

                    return;
                }
                break;
            default:
                urb->status = -EINPROGRESS;
                break;
        }
        if(urb->status){
            mtk_q_advance_schedule(musb, urb, hw_ep, USB_DIR_OUT);
        }
    }

    musb->pMtk_usb_result[isRx][epnum].status=MTK_NO_ERROR;
}
#endif

#ifdef CONFIG_USB_GADGET_MUSB_HDRC
void mtk_q_gagdet_rx(struct musb *musb, u8 epnum){
    u16            csr;
    struct usb_request    *request;
    void __iomem        *mbase = musb->mregs;
    struct musb_ep        *musb_ep = &musb->endpoints[epnum].ep_out;
    void __iomem        *epio = musb->endpoints[epnum].regs;
    u8            isRx=true;
    Mtk_USB_Result    pResult = musb->pMtk_usb_result[isRx][epnum];
    PMtk_SDU_Result    sdu_result = pResult.link_header;

    musb_ep_select(mbase, epnum);
    csr = musb_readw(epio, MUSB_RXCSR);

    while(pResult.number_of_sdu--){
        request = next_request(musb_ep);
        request->status = 0;
        request->actual = sdu_result->actual_length;


        if (csr & MUSB_RXCSR_P_ISO) {
            if (csr & MUSB_RXCSR_P_OVERRUN) {
                /* csr |= MUSB_RXCSR_P_WZC_BITS; */
                csr &= ~MUSB_RXCSR_P_OVERRUN;
                musb_writew(epio, MUSB_RXCSR, csr);

                DBG(3, "%s iso overrun on %p\n", musb_ep->name, request);
                if (request )
                    request->status = -EOVERFLOW;
            }
            if (csr & MUSB_RXCSR_INCOMPRX) {
                /* REVISIT not necessarily an error */
                DBG(4, "%s, incomprx\n", musb_ep->end_point.name);
            }
        }

        musb_g_giveback(musb_ep, request, request->status);
        sdu_result = sdu_result->next;
    }
    if(pResult.status){

        mtk_stop_q(musb,epnum, isRx);

        request = next_request(musb_ep);
        switch(pResult.status){
            case MTK_MODULE_ERROR:
            case MTK_LENGTH_ERROR:
                request->status = -EINPROGRESS;
                break;
            case MTK_BUS_ERROR:
                if (csr & MUSB_RXCSR_P_ISO) {
                    csr = musb_readw(epio, MUSB_RXCSR);
                    /* REVISIT for high bandwidth, MUSB_TXCSR_P_INCOMPTX
                     * probably rates reporting as a host error
                     */
                    if (csr & MUSB_RXCSR_P_SENTSTALL) {

                        csr |= MUSB_RXCSR_P_WZC_BITS;
                        csr &= ~MUSB_RXCSR_P_SENTSTALL;
                        musb_writew(epio, MUSB_RXCSR, csr);
                        if (request)
                            request->status = -EPIPE;

                        break;
                    }
                }
                break;
            default:
                request->status = -EINPROGRESS;
                break;
        }
        if(request->status){
            musb_g_giveback(musb_ep, request, request->status);
        }
    }
    pResult.status=MTK_NO_ERROR;

    return;
}

void mtk_q_gagdet_tx(struct musb *musb, u8 epnum){
    u16            csr;
    struct usb_request    *request;
    u8 __iomem        *mbase = musb->mregs;
    struct musb_ep        *musb_ep = &musb->endpoints[epnum].ep_in;
    void __iomem        *epio = musb->endpoints[epnum].regs;
    u8            isRx=false;
    Mtk_USB_Result    pResult = musb->pMtk_usb_result[isRx][epnum];
    PMtk_SDU_Result    sdu_result = pResult.link_header;

    musb_ep_select(mbase, epnum);
    csr = musb_readw(epio, MUSB_TXCSR);

    while(pResult.number_of_sdu--){
        request = next_request(musb_ep);

        request->status = 0;

        if (csr & MUSB_TXCSR_P_ISO) {
            if (csr & MUSB_TXCSR_P_UNDERRUN) {
                /* we no big deal ... little reason to care */
                csr |= MUSB_TXCSR_P_WZC_BITS;
                csr &= ~(MUSB_TXCSR_P_UNDERRUN
                        | MUSB_TXCSR_TXPKTRDY);
                musb_writew(epio, MUSB_TXCSR, csr);
                DBG(20, "underrun on ep%d, req %p\n", epnum, request);
            }
            if ((csr & MUSB_TXCSR_P_INCOMPTX)&&(pResult.status==MTK_BUS_ERROR)) {
                csr |= MUSB_TXCSR_P_WZC_BITS;
                csr &= ~(MUSB_TXCSR_P_INCOMPTX
                        | MUSB_TXCSR_TXPKTRDY);
                musb_writew(epio, MUSB_TXCSR, csr);
                DBG(20, "Tx Incomplete on ep%d, req %p\n", epnum, request);
                pResult.status==MTK_NO_ERROR;
            }
        }

        musb_g_giveback(musb_ep, request, 0);
        sdu_result = sdu_result->next;
    }
    if(pResult.status){

        mtk_stop_q(musb,epnum, isRx);

        request = next_request(musb_ep);
        switch(pResult.status){
            case MTK_MODULE_ERROR:
            case MTK_LENGTH_ERROR:
                request->status = -EINPROGRESS;
                break;
            case MTK_BUS_ERROR:
                if (csr & MUSB_RXCSR_P_ISO) {
                    csr = musb_readw(epio, MUSB_TXCSR);
                    /* REVISIT for high bandwidth, MUSB_TXCSR_P_INCOMPTX
                     * probably rates reporting as a host error
                     */
                    if (csr & MUSB_TXCSR_P_SENTSTALL) {
                        csr |= MUSB_TXCSR_P_WZC_BITS;
                        csr &= ~MUSB_TXCSR_P_SENTSTALL;
                        musb_writew(epio, MUSB_TXCSR, csr);
                        if (request)
                            request->status = -EPIPE;

                        break;
                    }
                }
                break;
            default:
                request->status = -EINPROGRESS;
                break;
        }
        if(request->status){
            musb_g_giveback(musb_ep, request, request->status);
        }
    }
    pResult.status=MTK_NO_ERROR;
}
#endif

bool musb_is_qmu_enabled(struct musb *musb, u8 EP_Num, u8 isRx)
{
  return _uqft->is_qmu_enabled(musb->config->id, EP_Num, isRx);
}

int musb_qmu_enable(struct musb *musb, u8 EP_Num, u8 isRx, u8 isZLP, u8 isCSCheck, u8 isEmptyCheck)
{
  return _uqft->qmu_enable(musb->config->id, EP_Num,isRx,isZLP,isCSCheck,isEmptyCheck);
}

void musb_qmu_disable(struct musb *musb, u8 EP_Num, u8 isRx)
{
  _uqft->qmu_disable(musb->config->id, EP_Num,isRx);
}

int musb_qmu_insert_task(struct musb *musb, u8 EP_Num, u8 isRx, u8* buf, u32 length, u8 isIOC)
{
  return _uqft->qmu_insert_task(musb->config->id, EP_Num,isRx,buf,length,isIOC);
}

int musb_qmu_remove_task(struct musb *musb, u8 EP_Num, u8 isRx, u8* buf, u16 length, u8* irq_cb, u8* do_stop)
{
  return _uqft->qmu_remove_task(musb->config->id, EP_Num,isRx, buf, length, irq_cb, do_stop);
}

int musb_qmu_cleanup(struct musb *musb, u8 EP_Num, u8 isRx)
{
  return _uqft->qmu_cleanup(musb->config->id, EP_Num,isRx);
}

void musb_qmu_stop(struct musb *musb, u8 EP_Num, u8 isRx)
{
  _uqft->qmu_stop(musb->config->id, EP_Num,isRx);
}

int musb_qmu_restart(struct musb *musb, u8 EP_Num, u8 isRx)
{
  return _uqft->qmu_restart(musb->config->id, EP_Num,isRx);
}


int musb_qmu_irq(struct musb *musb, u32 qisar, Mtk_USB_Result pResult[][8])
{
  return _uqft->qmu_irq(musb->config->id, qisar,(void*)pResult);
}

#endif
