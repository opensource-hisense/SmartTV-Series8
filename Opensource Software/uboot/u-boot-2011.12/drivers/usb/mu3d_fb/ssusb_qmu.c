/*
* Copyright (c) MediaTek Inc.
*
* This program is distributed under a dual license of GPL v2.0 and
* MediaTek proprietary license. You may at your option receive a license
* to this program under either the terms of the GNU General Public
* License (GPL) or MediaTek proprietary license, explained in the note
* below.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
*
/*! @file ssusb_qmu.c
* @par Copyright Statement:
*
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of MediaTek Inc. (C) 2012
*
* $Workfile: ssusb_qmu.c$
*
* @par Project: 
*    MTK ssusb device mode driver
*
* @par Description: 
*    Mtk DMAQ module interface implement
*
* @par Author_Name: 
*    cm.tu 
*
* @par Last_Changed: add comment for Doxygen
* $Author: dtvbm11 $
* $Modtime: 2011-12-14$
* $Revision: #1 $
*
* @todo Need to add support for function X
*/

#if 1//def USE_SSUSB_QMU
//#include <linux/kernel.h>
//#include <linux/list.h>
//#include <linux/timer.h>
//#include <linux/module.h>
//#include <linux/smp.h>
//#include <linux/spinlock.h>
//#include <linux/delay.h>
//#include <linux/moduleparam.h>
//#include <linux/stat.h>
//#include <linux/dma-mapping.h>
//#include <linux/slab.h>

//#include "musb_core.h"
//#include "SSUSB_DEV_c_header.h"
#include "mu3d_hal_osal.h"
#include "mu3d_hal_qmu_drv.h"
#include "ssusb_qmu.h"
#include "mu3d_usb.h"
#include "../udc.h"


extern struct udc_endpoint * mt_find_ep(int ep_num, u8 dir);
/* 
	1. Configure the HW register (starting address)  with gpd heads (starting address)  for each Queue
	2. Enable Queue with each EP and interrupts

*/

void usb_initialize_qmu(){

	mu3d_hal_init_qmu();
	is_bdp = 1; //We need to use BD because for mass storage, the largest data > GPD limitation
 	gpd_buf_size = GPD_BUF_SIZE; //max allowable data buffer length. Don't care when linking with BD.
 	bd_buf_size = BD_BUF_SIZE;
	bBD_Extension = 0;
 	bGPD_Extension = 0;
 	g_dma_buffer_size = STRESS_DATA_LENGTH*MAX_GPD_NUM;
}


/*
  put usb_request buffer into GPD/BD data structures, and ask QMU to start TX.

  caller: musb_g_tx

*/

#if 0
void txstate_qmu(struct musb *musb, struct musb_request *req)
{
	u8			epnum = req->epnum;
	struct musb_ep		*musb_ep;
	struct usb_request	*request;
	u32 txcsr0 = 0;

	musb_ep = &musb->endpoints[epnum].ep_in;	
	request = &req->request;

	txcsr0 = os_readl(musb->endpoints[epnum].addr_txcsr0);

	if (txcsr0 & TX_SENDSTALL) {
		os_printk(K_DEBUG, "%s stalling, txcsr %03x\n",
				musb_ep->end_point.name, txcsr0);
		return;
	}
//	request->actual = request->length;
//	mu3d_hal_insert_transfer_gpd(epnum, USB_TX, (DEV_UINT8 *)request->dma, request->length, true,true,false,(musb_ep->type==USB_ENDPOINT_XFER_ISOC?0:1), musb_ep->packet_sz);
//	mu3d_hal_resume_qmu(epnum, USB_TX);
	os_printk(K_DEBUG,"tx start...length : %d\r\n",request->length);

}

/* the caller ensures that req is not NULL. */

void rxstate_qmu(struct musb *musb, struct musb_request *req)
{
	u8			epnum = req->epnum;
	struct musb_ep		*musb_ep = &musb->endpoints[epnum].ep_out;
	struct usb_request	*request;
	u32 rxcsr0 = os_readl(musb->endpoints[epnum].addr_rxcsr0);
	
	request = &req->request;

	if(rxcsr0 & RX_SENDSTALL) {
		os_printk(K_DEBUG, "%s stalling, RXCSR %04x\n",
			musb_ep->end_point.name, rxcsr0);
		return;
	}

	os_printk(K_DEBUG, "rxstate_qmu 0\n");
	mu3d_hal_insert_transfer_gpd(epnum, USB_RX, (DEV_UINT8*)request->dma, request->length, true,true,false,(musb_ep->type==USB_ENDPOINT_XFER_ISOC?0:1), musb_ep->packet_sz); 
	mu3d_hal_resume_qmu(epnum, USB_RX);
	os_printk(K_DEBUG,"rx start\r\n");

}
#endif

/*
 * Immediately complete a request.
 *
 * @param request the request to complete
 * @param status the status to complete the request with
 * Context: controller locked, IRQs blocked.
 */
void musb_g_giveback(struct udc_endpoint *ep, struct udc_request *req, int status)
{

	struct urb *urb;

	os_printk(K_DEBUG, "*************** musb_g_giveback : %p, #%d\n", req, req->length);		

if(0)	
{
	int i;
	if(ep->in)
	{
        urb = ep->tx_urb;
		printf("buf TX %d bytes: ",urb->actual_length);
        for(i=0; i < urb->actual_length;i++)
            {
                printf("%x ", ((u8 *)(urb->buffer))[i]);                       
            }
        printf("\n");   
	}
	else
    {   
        urb = ep->rcv_urb;
		printf("buf RX %d bytes: ",urb->actual_length);
        for(i=0;i < urb->actual_length;i++)
            {
                printf("%x ", ((u8 *)(urb->buffer))[i]);                       
            }
        printf("\n");   
    }
}

}

/* 
    1. Find the last gpd HW has executed and update Tx_gpd_last[]
    2. Set the flag for txstate to know that TX has been completed

    ported from proc_qmu_tx() from test driver.
    
    caller:qmu_interrupt after getting QMU done interrupt and TX is raised 

*/
//volatile unsigned int u4times = 0;


extern void *gpd_phys_to_virt(void *paddr,USB_DIR dir,DEV_UINT32 num);
extern void *bd_phys_to_virt(void *paddr,USB_DIR dir,DEV_UINT32 num);
void qmu_tx_interrupt(u8 ep_num){

    //struct USB_REQ *req = usbdev_get_req(ep_num, USB_TX);
    TGPD* gpd=Tx_gpd_last[ep_num];
    TGPD* gpd_current = (TGPD*)(os_readl(USB_QMU_TQCPR(ep_num)));
	struct udc_endpoint		*endpoint = mt_find_ep(ep_num, UDC_DIR_IN);;
	struct udc_request *req = NULL;


	//trying to give_back the request to gadget driver.
	req =  endpoint->req;;
	if (!req){
		os_printk(K_DEBUG, "This should not happen. Cannot get next request of %d, but QMU has done.\n", ep_num);	
		return;
	}

	gpd_current = gpd_phys_to_virt(gpd_current,USB_TX,(DEV_UINT32)ep_num);
    os_printk(K_DEBUG,"Tx_gpd_last 0x%x, gpd_current 0x%x, gpd_end 0x%x, \r\n",  (DEV_UINT32)gpd, (DEV_UINT32)gpd_current, (DEV_UINT32)Tx_gpd_end[ep_num]);

	if(gpd==gpd_current){//gpd_current should at least point to the next GPD to the previous last one.
		os_printk(K_DEBUG, "should not happen: %s %d\n", __func__, __LINE__);
		return;
    }    

    while(gpd!=gpd_current && !TGPD_IS_FLAGS_HWO(gpd)){

		 os_printk(K_DEBUG,"Tx gpd %x info { HWO %d, BPD %d, Next_GPD %x , DataBuffer %x, BufferLen %d, Endpoint %d}\r\n", 
		 	(DEV_UINT32)gpd, (DEV_UINT32)TGPD_GET_FLAG(gpd), (DEV_UINT32)TGPD_GET_FORMAT(gpd), (DEV_UINT32)TGPD_GET_NEXT(gpd), 
		 	(DEV_UINT32)TGPD_GET_DATA(gpd), (DEV_UINT32)TGPD_GET_BUF_LEN(gpd), (DEV_UINT32)TGPD_GET_EPaddr(gpd));

		gpd=TGPD_GET_NEXT(gpd);	
		gpd=gpd_phys_to_virt(gpd,USB_TX,(DEV_UINT32)ep_num);

		Tx_gpd_last[ep_num]=gpd;   		
		musb_g_giveback(endpoint, req, 0);	
//		req = next_request(musb_ep);
//		request = &req->request;		
    }

	os_printk(K_DEBUG,"Tx_gpd_last[%d] : 0x%x\r\n", ep_num,(DEV_UINT32)Tx_gpd_last[ep_num]);		
	os_printk(K_DEBUG,"Tx_gpd_end[%d] : 0x%x\r\n", ep_num,(DEV_UINT32)Tx_gpd_end[ep_num]);		
    os_printk(K_DEBUG,"Tx %d complete\r\n", ep_num);   
    os_printk(K_DEBUG,"EP%d Tx complete\r\n", ep_num);   
//    mu3d_hal_resume_qmu(ep_num, USB_TX);
	return;
}



/* 
   When receiving RXQ done interrupt, qmu_interrupt calls this function.

   1. Traverse GPD/BD data structures to count actual transferred length.
   2. Set the done flag to notify rxstate_qmu() to report status to upper gadget driver.

    ported from proc_qmu_rx() from test driver.
    
    caller:qmu_interrupt after getting QMU done interrupt and TX is raised 

*/

void qmu_rx_interrupt(u8 ep_num)
{
	DEV_UINT32 bufferlength=0,recivedlength=0;
	TGPD* gpd=(TGPD*)Rx_gpd_last[ep_num];
	TGPD* gpd_current = (TGPD*)(os_readl(USB_QMU_RQCPR(ep_num)));
	TBD*bd; 
	struct udc_endpoint		*endpoint = mt_find_ep(ep_num, UDC_DIR_OUT);;
	struct udc_request *req = NULL;

	//trying to give_back the request to gadget driver.
	req = endpoint->req;
	if (!req){
		os_printk(K_DEBUG, "This should not happen. Cannot get next request of %d, but QMU has done.\n", ep_num);	
		return;
	}
	

	gpd_current = gpd_phys_to_virt(gpd_current,USB_RX,(DEV_UINT32)ep_num);
	os_printk(K_INFO,"ep_num : %d ,Rx_gpd_last : 0x%x, gpd_current : 0x%x, gpd_end : 0x%x \r\n",ep_num,(DEV_UINT32)gpd, (DEV_UINT32)gpd_current, (DEV_UINT32)Rx_gpd_end[ep_num]);

	if(gpd==gpd_current){
		os_printk(K_DEBUG, "should not happen: %s %d\n", __func__, __LINE__);
		return;
	}

	while(gpd != gpd_current && !TGPD_IS_FLAGS_HWO(gpd)){
		
		if(TGPD_IS_FORMAT_BDP(gpd)){

			bd = (TBD *)TGPD_GET_DATA(gpd);
			bd = (TBD *)bd_phys_to_virt(bd,USB_RX,(DEV_UINT32)ep_num);

			while(1){
				
				os_printk(K_INFO,"BD : 0x%x\r\n",(DEV_UINT32)bd);
				os_printk(K_INFO,"Buffer Len : 0x%x\r\n",(DEV_UINT32)TBD_GET_BUF_LEN(bd));
				//req->transferCount += TBD_GET_BUF_LEN(bd);
				//musb_ep->qmu_done_length += TBD_GET_BUF_LEN(bd);
				endpoint->rcv_urb->actual_length += TBD_GET_BUF_LEN(bd);
				
				//os_printk(K_INFO,"Total Len : 0x%x\r\n",musb_ep->qmu_done_length);
				if(TBD_IS_FLAGS_EOL(bd)){
					break;
				}
				bd= TBD_GET_NEXT(bd);
				bd=bd_phys_to_virt(bd,USB_RX,(DEV_UINT32)ep_num);
			}
		}
		else{
			recivedlength = (DEV_UINT32)TGPD_GET_BUF_LEN(gpd);
			bufferlength  = (DEV_UINT32)TGPD_GET_DataBUF_LEN(gpd);
			endpoint->rcv_urb->actual_length += recivedlength;
		}

		os_printk(K_DEBUG,"Rx gpd info { HWO %d, Next_GPD %x ,DataBufferLength %d, DataBuffer %x, Recived Len %d, Endpoint %d, TGL %d, ZLP %d}\r\n", 
				(DEV_UINT32)TGPD_GET_FLAG(gpd), (DEV_UINT32)TGPD_GET_NEXT(gpd), 
				(DEV_UINT32)TGPD_GET_DataBUF_LEN(gpd), (DEV_UINT32)TGPD_GET_DATA(gpd), 
				(DEV_UINT32)TGPD_GET_BUF_LEN(gpd), (DEV_UINT32)TGPD_GET_EPaddr(gpd), 
				(DEV_UINT32)TGPD_GET_TGL(gpd), (DEV_UINT32)TGPD_GET_ZLP(gpd));
		
		gpd=TGPD_GET_NEXT(gpd); 
		gpd=gpd_phys_to_virt(gpd,USB_RX,(DEV_UINT32)ep_num);

		Rx_gpd_last[ep_num]=gpd;		
		musb_g_giveback(endpoint, req, 0);
//		req = next_request(musb_ep);
//		request = &req->request;		
		
	}	 

	os_printk(K_DEBUG,"Rx_gpd_last[%d] : 0x%x\r\n", ep_num,(DEV_UINT32)Rx_gpd_last[ep_num]);		
	os_printk(K_DEBUG,"Rx_gpd_end[%d] : 0x%x\r\n", ep_num,(DEV_UINT32)Rx_gpd_end[ep_num]);		
    os_printk(K_DEBUG,"EP%d Rx complete\r\n", ep_num);   


}

void qmu_done_interrupt(DEV_UINT32 wQmuVal){

//	DEV_UINT32 wQmuVal;
    DEV_INT32 i;
	
//	os_printk(K_CRIT,"*********************qmu_interrupt\n");

	

    for(i=1; i<=MAX_QMU_EP; i++){
    	if(wQmuVal & QMU_RX_DONE(i)){
        //   	os_printk(K_EMERG,"Q_RX_int\n");   	
    	
    		qmu_rx_interrupt(i);
    	}
    	if(wQmuVal & QMU_TX_DONE(i)){
        //  	os_printk(K_EMERG,"Q_TX_int\n");   	
    		qmu_tx_interrupt(i);
    	}

   	}

}

void qmu_exception_interrupt(DEV_UINT32 wQmuVal){
    u32 wErrVal;
	int i = (int)wQmuVal;

//	os_printk(K_CRIT,"*********************qmu_exception_interrupt\n");


	if(wQmuVal & RXQ_CSERR_INT){
		os_printk(K_CRIT, "Rx %d checksum error!\r\n", i); 
     	os_printk(K_CRIT, "\r\n"); 
//		while(1);
	}
	if(wQmuVal & RXQ_LENERR_INT){
		os_printk(K_CRIT, "Rx %d length error!\r\n", i); 
		os_printk(K_CRIT, "\r\n"); 
//		while(1);
	}
	if(wQmuVal & TXQ_CSERR_INT){
		os_printk(K_CRIT, "Tx %d checksum error!\r\n", i); 
		os_printk(K_CRIT, "\r\n"); 
//		while(1);
	}
	if(wQmuVal & TXQ_LENERR_INT){
		os_printk(K_CRIT, "Tx %d length error!\r\n", i); 
		os_printk(K_CRIT, "\r\n"); 
//		while(1);
	}
	
    if((wQmuVal & RXQ_CSERR_INT)||(wQmuVal & RXQ_LENERR_INT)){ 	
     	wErrVal=os_readl(U3D_RQERRIR0);
     	os_printk(K_CRIT, "Rx Queue error in QMU mode![0x%x]\r\n", (unsigned int)wErrVal);
     	for(i=1; i<=MAX_QMU_EP; i++){ 			  
     		if(wErrVal &QMU_RX_CS_ERR(i)){
     			os_printk(K_CRIT, "Rx %d length error!\r\n", i); 
     			os_printk(K_CRIT, "\r\n"); 
	//			while(1);
     		}	
     		if(wErrVal &QMU_RX_LEN_ERR(i)){
     			os_printk(K_CRIT, "Rx %d recieve length error!\r\n", i); 
				os_printk(K_CRIT, "\r\n"); 
	//			while(1);
     		}
     	}
     	os_writel(U3D_RQERRIR0, wErrVal);
    }

    if(wQmuVal & RXQ_ZLPERR_INT){ 	
		wErrVal=os_readl(U3D_RQERRIR1);
		os_printk(K_DEBUG, "Rx Queue error in QMU mode![0x%x]\r\n", (unsigned int)wErrVal);
		
     	for(i=1; i<=MAX_QMU_EP; i++){ 			  
     		if(wErrVal &QMU_RX_ZLP_ERR(i)){
     			os_printk(K_DEBUG, "Rx %d recieve an zlp packet!\r\n", i); 
     		}
     	}
     	os_writel(U3D_RQERRIR1, wErrVal);
    }
	
    if((wQmuVal & TXQ_CSERR_INT)||(wQmuVal & TXQ_LENERR_INT)){ 	
		
 		wErrVal=os_readl(U3D_TQERRIR0);
 		os_printk(K_CRIT, "Tx Queue error in QMU mode![0x%x]\r\n", (unsigned int)wErrVal);

		for(i=1; i<=MAX_QMU_EP; i++){
 			if(wErrVal &QMU_TX_CS_ERR(i)){
 				os_printk(K_CRIT, "Tx %d checksum error!\r\n", i); 
				os_printk(K_CRIT, "\r\n"); 
	//			while(1);
 			}		  
 			
 			if(wErrVal &QMU_TX_LEN_ERR(i)){
 				os_printk(K_CRIT, "Tx %d buffer length error!\r\n", i); 
				os_printk(K_CRIT, "\r\n"); 
	//			while(1);
 			}	
 		}
 		os_writel(U3D_TQERRIR0, wErrVal);

    }

	if((wQmuVal & RXQ_EMPTY_INT)||(wQmuVal & TXQ_EMPTY_INT)){ 	
 		DEV_UINT32 wEmptyVal = os_readl(U3D_QEMIR);
 		os_printk(K_DEBUG, "Rx Empty in QMU mode![0x%x]\r\n", wEmptyVal);
 		os_writel(U3D_QEMIR, wEmptyVal);
	}

}



#endif //USE_SSUSB_QMU

