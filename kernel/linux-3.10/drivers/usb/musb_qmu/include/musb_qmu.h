/*-----------------------------------------------------------------------------
 * drivers/usb/musb_qmu/musb_qmu.h
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
#include "musb_core.h"
#include <linux/dmapool.h>

#include "../../../usb_bltin/usb_bltin.h"


/* QMU USER CONFIG */
#define DEVICE_EP_NUM 4 
#define ENQUEUE_EP_NUM  2
extern u8 ep_bind[ENQUEUE_EP_NUM];
extern struct dma_pool *qmu_dma_pool;
/*     end         */

extern bool q_disabled;
/// @brief DMA channel select for DMAQ
/// .
/// @param musb:   struct musb
/// @param channel: DMA channel selected
/// @param burstmode: DMA burst mode selected
extern void mtk_q_dma_select (
	struct musb *musb, 			
	u8 channel, 					
	u8 burstmode				
);

extern void mtk_q_advance_schedule(
        struct musb *musb, 
        struct urb *urb, 
        struct musb_hw_ep *hw_ep, 
        int is_in
);

/// @brief musb Enable Queue
/// .
/// @param musb:   struct musb
/// @param EP_Num: Endpoint number mapping Queue 
/// @param isRx: RxQ or TxQ
/// @param type: Transfer Type for the selected Queue
/// @param MaxP: Transfer Max Packet size for the selected Queue
/// @param interval: Transfer interval for periodical transfer, or NAK limit for buld transfer
/// @param target_ep: If Host mode, target Endpoint of Pipe
/// @param isZLP: if open ZLP feature
/// @param isCSCheck: if open checksum check
/// @param isEmptyCheck: if open empty notification
/// @return status.
extern int mtk_enable_q(
    struct musb *musb,
	u8 EP_Num, 					
	u8 isRx, 						
	u8 type, 						
	u16 MaxP, 					
	u8 interval, 					
	u8 target_ep, 				
	u8 isZLP, 					
	u8 isCSCheck, 				
	u8 isEmptyCheck,
	u8 hb_mult
);

/// @brief musb disable Queue
/// .
/// @param musb:   struct musb
/// @param EP_Num: Endpoint number mapping Queue  
/// @param isRx: RxQ or TxQ
/// @return status.
extern void mtk_disable_q(
	struct musb *musb, 			
	u8 EP_Num, 					
	u8 isRx						
);
/// @brief musb disable Queue
/// .
/// @param musb:   struct musb
/// @param EP_Num: Endpoint number mapping Queue  
/// @param isRx: RxQ or TxQ
/// @return status.
extern void mtk_disable_q_all(struct musb *musb);

/// @brief Insert a urb task into Queue
/// .
/// @param urb: urb task   
/// @param qh: usb_host_endpoint.hcpriv for scheduled endpoints
/// @return status.
extern int mtk_start_urb(
	struct urb *urb,
	struct musb_qh *qh
);

/// @brief Insert a usb_request task into Queue
/// .
/// @param musb:   struct musb
/// @param request: request task  
/// @return status.
extern int mtk_start_request(
	struct musb *musb, 			
	struct musb_request *req		 
);

/// @brief remove a urb task into Queue
/// .
/// @param urb: urb task   
/// @return status.
extern int mtk_q_remove_urb(struct musb *musb,struct urb* urb,u8 EP_Num,bool isRx,u8* irq_cb);

/// @brief Remove a usb_request task into Queue
/// .
/// @param request: request task   
/// @return status.
extern int mtk_remove_request(
	struct usb_request request		
);

/// @brief musb clean up the selected Queue
/// .
/// @param musb:   struct musb
/// @param EP_Num: Endpoint number mapping Queue 
/// @param isRx: RxQ or TxQ
/// @return status.
extern int mtk_cleanup_q(
	struct musb *musb, 			
	u8 EP_Num, 					
	u8 isRx						
);

/// @brief musb stop the selected Queue
/// .
/// @param musb:   struct musb
/// @param EP_Num: Endpoint number mapping Queue
/// @param isRx: RxQ or TxQ
/// @return status.
extern void mtk_stop_q(
    struct musb* musb,
	u8 EP_Num, 					
	u8 isRx						
);

/// @brief musb restart the selected Queue
/// .
/// @param musb:   struct musb
/// @param EP_Num: Endpoint number mapping Queue
/// @param isRx: RxQ or TxQ
/// @return status.
extern int mtk_restart_q(
	struct musb *musb, 			
	u8 EP_Num, 					  
	u8 isRx						
);

/// @brief musb DMAQ interrupt handling
/// .
/// @param musb:   struct musb
/// @return irq status.   
extern irqreturn_t mtk_q_interrupt(
	struct musb *musb			
);

/// @brief musb host mode DMAQ Rx task done handling
/// .
/// @param musb:   struct musb
/// @epnum: endpoint number
void mtk_q_host_rx(
	struct musb *musb, 			
	u8 epnum					
);

/// @brief musb host mode DMAQ Tx task done handling
/// .
/// @param musb:   struct musb
/// @param epnum: endpoint number
void mtk_q_host_tx(
	struct musb *musb, 			
	u8 epnum					

);

/// @brief musb device mode DMAQ Rx task done handling
/// .
/// @param musb:   struct musb
/// @param epnum: endpoint number
void mtk_q_gagdet_rx(
	struct musb *musb, 			
	u8 epnum					
);

/// @brief musb device mode DMAQ Tx task done handling
/// .
/// @param musb:   struct musb
/// @param epnum: endpoint number
void mtk_q_gagdet_tx(
	struct musb *musb, 			
	u8 epnum					
);

#endif

