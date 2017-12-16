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
/*
 * Copyright (c) 2012 MediaTek Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *	notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *	notice, this list of conditions and the following disclaimer in
 *	the documentation and/or other materials provided with the
 *	distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#include <common.h>
     
//#include <usb.h>
#include "x_typedef.h"
     
#include "mu3d_usb.h"
#include "../udc.h"
#include "mu3d_hal_qmu_drv.h"
#include "ssusb_qmu.h"

//#define USE_SSUSB_QMU
//#define URB_DEBUG
#ifdef URB_DEBUG
/* DEBUG INFO Sections */
#define DBG_USB_GENERAL_INFO 1
#define DBG_USB_DUMP_DESC 0
#define DBG_USB_DUMP_DATA 0
#define DBG_USB_DUMP_SETUP 0
#define DBG_USB_FIFO 0
#define DBG_USB_IRQ  0
#endif

#ifdef URB_DEBUG
#if DBG_USB_GENERAL_INFO
#define DBG_I(x...) printf(x)
#else
#define DBG_I(x...) do{} while(0)
#endif

#if DBG_USB_IRQ
#define DBG_IRQ(x...) printf(x)
#else
#define DBG_IRQ(x...) do{} while(0)
#endif

#define DBG_D(x...) printf(x)
#define DBG_C(x...) printf(x)

#else
#define DBG_I(x...) do{} while(0)
#define DBG_D(x...) do{} while(0)
#define DBG_IRQ(x...) do{} while(0)
#define DBG_C(x...) do{} while(0)
#endif

static int usb_online = 0;
static unsigned char usb_config_value = 0;
EP0_STATE ep0_state = EP0_IDLE;
int set_address = 0;
static u8 dev_address = 0;

/* bits used in all the endpoint status registers */
#define EPT_TX(n) (1 << ((n) + 16))
#define EPT_RX(n) (1 << (n))

#define EP0	0
#define EP0_MAX_PACKET_SIZE	64

/* values used in GET_STATUS requests */
#define USB_STAT_SELFPOWERED	0x01

/* USB recipients */
#define USB_RECIP_DEVICE	0x00
#define USB_RECIP_OTHER		0x03

#define USB_EP_DIR_MASK	0x80
#define USB_RECIP_MASK	0x1f

static struct udc_device *the_device;
static struct udc_gadget *the_gadget;

struct udc_descriptor {
	struct udc_descriptor *next;
	unsigned short tag;	/* ((TYPE << 8) | NUM) */
	unsigned short len;	/* total length */
	unsigned char data[0];
};

static struct udc_endpoint *ep0in, *ep0out;
static struct udc_request *ep0req;
static unsigned char ep0_buf[4096] __attribute__((aligned(32)));
struct urb mt_ep0_urb;
struct urb mt_tx_urb;
struct urb mt_rx_urb;
struct urb *ep0_urb = &mt_ep0_urb;
struct urb *tx_urb = &mt_tx_urb;
struct urb *rx_urb = &mt_rx_urb;

#define NUM_ENDPOINTS	2

struct udc_endpoint ep_list[NUM_ENDPOINTS + 1];	/* one extra for control endpoint */

#if (DBG_USB_DUMP_DESC || DBG_USB_DUMP_DATA)
static void hexdump8(const void *ptr, size_t len)
{
	unsigned char *address = (u8 *)ptr;
	size_t count;
	int i;
	
	for (count = 0 ; count < len; count += 16) {
		printf("0x%08lx: ,len(%d) ", address, (len > 16)? 16: (len-count));
		for (i=0; (i < 16) && (i <(len - count)); i++) {
			printf("0x%x ", *(address + i));
		}
		printf("\n");
		printf("\n");
		address += 16;
	}	
}
#endif

#if DBG_USB_DUMP_SETUP
static void dump_setup_packet(char *str, struct setup_packet *sp) {
	DBG_D("\n");
	DBG_D(str);
	DBG_D("	   bmRequestType = %x\n", sp->type);
	DBG_D("	   bRequest = %x\n", sp->request);
	DBG_D("	   wValue = %x\n", sp->value);
	DBG_D("	   wIndex = %x\n", sp->index);
	DBG_D("	   wLength = %x\n", sp->length);
}
#else
static void dump_setup_packet(char *str, struct setup_packet *sp) {}
#endif

static void copy_desc(struct urb *urb, void *data, int length) {

#if DBG_USB_FIFO
	DBG_I("%s: urb: %x, data %x, length: %d, actual_length: %d\n",
		__func__, urb->buffer, data, length, urb->actual_length);
#endif

	//memcpy(urb->buffer + urb->actual_length, data, length);
	memcpy(urb->buffer, data, length);
	//urb->actual_length += length;
	urb->actual_length = length;
#if DBG_USB_FIFO
	DBG_I("%s: urb: %x, data %x, length: %d, actual_length: %d\n",
		__func__, urb, data, length, urb->actual_length);
#endif
}

static void handle_ept_complete(struct udc_endpoint *ept)
{
	unsigned int actual;
	int status;
	struct udc_request *req;

	req = ept->req;
	if (req) {
//		DBG_I("%s: req: %x: req->length: %d: ept->actual_length: %d status: %d\n", __func__, req, req->length, ept->rcv_urb->actual_length, ept->status);
		/* release this request for processing next */
		ept->req = NULL;

		if (ept->status == -1) {
			actual = 0;
			status = -1;
			DBG_C("%s: EP%d/%s FAIL status: %x\n",
				__func__, ept->num, ept->in ? "in" : "out", status);
		} else {
			actual = req->length;
			status = 0;
		}
		if (req->complete)
			req->complete(req, actual, status);
	}

}

struct udc_endpoint * mt_find_ep(int ep_num, u8 dir) {
	int i;
	u8 in = 0;

	/* convert dir to in */
	if (dir == UDC_DIR_IN) /* dir == USB_DIR_IN */
		in = 1;

	/* for (i = 0; i < udc_device->max_endpoints; i++) */
	/* for (i = 0; i < the_gadget->ifc_endpoints; i++) */
	for (i = 0; i < MT_EP_NUM; i++) {
		if ((ep_list[i].num == ep_num) && (ep_list[i].in == in)) {
//			DBG_I("%s: find ep!\n", __func__);
			return &ep_list[i];
		}
	}
	return NULL;
}

struct udc_endpoint *_udc_endpoint_alloc(unsigned char num, unsigned char in,
					 unsigned short max_pkt) {
	int i;
    int dir;
	/*
	 * find an unused slot in ep_list from EP1 to MAX_EP
	 * for example, EP1 will use 2 slot one for IN and the other for OUT
	 */
	if (num != EP0) {
		for (i = 1; i < MT_EP_NUM; i++) {
			if (ep_list[i].num == 0) /* usable */
				break;
		}

		if (i == MT_EP_NUM)	/* ep has been exhausted. */
			return NULL;

		if (in) { /* usb EP1 tx */
			ep_list[i].tx_urb = tx_urb;
 		} else {	/* usb EP1 rx */
			ep_list[i].rcv_urb = rx_urb;
 		}
	} else {
		i = EP0;	/* EP0 */
	}

	ep_list[i].maxpkt = max_pkt;
	ep_list[i].num = num;
	ep_list[i].in = in;
	ep_list[i].req = NULL;

	/* store EPT_TX/RX info */
	if (ep_list[i].in) {
		ep_list[i].bit = EPT_TX(num);
        dir = USB_TX;
	} else {
		ep_list[i].bit = EPT_RX(num);
        dir = USB_RX;
	}

	/* write parameters to this ep (write to hardware) */
//	mt_setup_ep(num, &ep_list[i]);
    if(num!=0)
    {
        mu3d_hal_ep_enable(num, dir, 2, ep_list[i].maxpkt, 0,0,0,0); //slot_id=0
#ifdef USE_SSUSB_QMU
        mu3d_hal_start_qmu(num, dir);
#endif
    }

	DBG_I("[USB] ept%d %s @%p/%p max=%d bit=%x\n",
		num, in ? "in" : "out", &ep_list[i], &ep_list, max_pkt, ep_list[i].bit);

	return &ep_list[i];
}

struct udc_descriptor *udc_descriptor_alloc(unsigned type, unsigned num,
						unsigned len)
{
	struct udc_descriptor *desc;
	if ((len > 255) || (len < 2) || (num > 255) || (type > 255))
		return 0;

	if (!(desc = malloc(sizeof(struct udc_descriptor) + len)))
		return 0;

	desc->next = 0;
	desc->tag = (type << 8) | num;
	desc->len = len;
	desc->data[0] = len;
	desc->data[1] = type;

	return desc;
}

static struct udc_descriptor *desc_list = 0;
static unsigned next_string_id = 1;

void udc_descriptor_register(struct udc_descriptor *desc) {
	desc->next = desc_list;
	desc_list = desc;
}

unsigned udc_string_desc_alloc(const char *str)
{
	unsigned len;
	struct udc_descriptor *desc;
	unsigned char *data;

	if (next_string_id > 255)
		return 0;

	if (!str)
		return 0;

	len = strlen(str);
	desc = udc_descriptor_alloc(TYPE_STRING, next_string_id, len * 2 + 2);
	if (!desc)
		return 0;
	next_string_id++;

	/* expand ascii string to utf16 */
	data = desc->data + 2;
	while (len-- > 0) {
		*data++ = *str++;
		*data++ = 0;
	}

	udc_descriptor_register(desc);
	return desc->tag & 0xff;
}

static void udc_ept_desc_fill(struct udc_endpoint *ept, unsigned char *data)
{
	data[0] = 7;
	data[1] = TYPE_ENDPOINT;
	data[2] = ept->num | (ept->in ? UDC_DIR_IN : UDC_DIR_OUT);
	data[3] = 0x02;		/* bulk -- the only kind we support */
	data[4] = ept->maxpkt;
	data[5] = ept->maxpkt >> 8;
	data[6] = ept->in ? 0x00 : 0x01;
}

static unsigned udc_ifc_desc_size(struct udc_gadget *g)
{
	return 9 + g->ifc_endpoints * 7;
}

static void udc_ifc_desc_fill(struct udc_gadget *g, unsigned char *data)
{
	unsigned n;

	data[0] = 0x09;
	data[1] = TYPE_INTERFACE;
	data[2] = 0x00;		/* ifc number */
	data[3] = 0x00;		/* alt number */
	data[4] = g->ifc_endpoints;
	data[5] = g->ifc_class;
	data[6] = g->ifc_subclass;
	data[7] = g->ifc_protocol;
	data[8] = udc_string_desc_alloc(g->ifc_string);

	data += 9;
	for (n = 0; n < g->ifc_endpoints; n++) {
		udc_ept_desc_fill(g->ept[n], data);
		data += 7;
	}
}

#ifndef USE_SSUSB_QMU
/*
 * An endpoint is transmitting data. This can be called either from
 * the IRQ routine or from ep.queue() to kickstart a request on an
 * endpoint.
 *
 * Context: controller locked, IRQs blocked, endpoint selected
 */
static void txstate(u8 epnum)
{
    struct udc_endpoint *endpoint;
    struct udc_request *req;
	u16			fifo_count = 0;
	u32 txcsr0 = 0,ep_tmp;

    endpoint = mt_find_ep(epnum, UDC_DIR_IN);
    req = endpoint->req	;

	/* read TXCSR before */
    ep_tmp = epnum - 1;
    txcsr0 = os_readl(U3D_TX1CSR0+(ep_tmp*0x10));
	fifo_count = req->length;

	if(txcsr0 & TX_TXPKTRDY){
		return;
	}

	if (txcsr0 & TX_SENDSTALL) {
        endpoint->status = -1;
        handle_ept_complete(endpoint);
        /* exception handling: implement this!! */
		return;
	}


        endpoint->sent=mu3d_hal_write_fifo(epnum,fifo_count,endpoint->tx_urb->buffer, 512);
//        os_writel((U3D_TX1CSR0+(ep_tmp*0x10)), txcsr0 | TX_TXPKTRDY);

}
/*
 * FIFO state update (e.g. data ready).
 * Called from IRQ,  with controller locked.
 */
void musb_g_tx(u8 epnum)
{
    struct udc_endpoint *endpoint;
    struct udc_request *req;
	unsigned		fifo_count = 0;
	u32			txcsr0,ep_tmp;

    endpoint = mt_find_ep(epnum, UDC_DIR_IN);
//    printk("Enter musb_g_tx\n");

    ep_tmp = epnum - 1;
	txcsr0 = os_readl(U3D_TX1CSR0+(ep_tmp*0x10));
    req = endpoint->req	;

	if (req) {
		/*
		 * First, maybe a terminating short packet. Some DMA
		 * engines might handle this by themselves.
		 */
		if ((req->length == 0)
			&& (endpoint->tx_urb->actual_length == req->length))
		{
			/*
			 * On DMA completion, FIFO may not be
			 * available yet...
			 */
			if(txcsr0 & TX_TXPKTRDY)
				return;

			printf("sending zero pkt\n");
            os_writel((U3D_TX1CSR0+(ep_tmp*0x10)), (txcsr0 & TX_W1C_BITS) | TX_TXPKTRDY);
            printf("request->actual == request->length TXPACKETREADY\n");
		}

		if (endpoint->sent < req->length) 
    		txstate(epnum);
		if (endpoint->sent== req->length) {
			musb_g_giveback(endpoint, req, 0);
			handle_ept_complete(endpoint);
//			return;
		}

	}
}
#endif
/*================UDC APIs=======================================*/
struct udc_request *udc_request_alloc(void)
{
	struct udc_request *req;
	req = malloc(sizeof(*req));
	req->buf = NULL;
	req->length = 0;
	return req;
}

void udc_request_free(struct udc_request *req)
{
	free(req);
}

int udc_request_queue(struct udc_endpoint *ept, struct udc_request *req) {
	u8 intrrxe;

//	DBG_I("[USB] %s: ept%d %s queue req=%p, req->length=%x\n",
//		__func__, ept->num, ept->in ? "in" : "out", req, req->length);
//	DBG_I("[USB] %s: ept%d: %x, ept->in: %s, ept->rcv_urb->buffer: %x, req->buf: %x\n",
//		__func__, ept->num, ept, ept->in ? "IN" : "OUT" , ept->rcv_urb->buffer, req->buf);

	ept->req = req;
	ept->status = 0;	/* ACTIVE */

	ept->sent = 0;
	ept->last = 0;

#ifdef USE_SSUSB_QMU
    if(ept->in)
    {
        if(req->length > 0) //only enqueue for length > 0 packet. Don't send ZLP here for MSC protocol.
        {
            os_printk(K_DEBUG, "EP%d TX1  len %d buff_add 0x%x\n",ept->num,req->length,(DEV_UINT32)(req->buf));
                    
            mu3d_hal_insert_transfer_gpd(ept->num, USB_TX, (DEV_UINT8*)(req->buf), req->length, 1,1,0,1, ept->maxpkt);
            mu3d_hal_resume_qmu(ept->num, USB_TX);
        }//if(!request->request.length)
        else //If usb_request length <= 0, we don't send anything for MSC. ssusb. Giveback the request right away.
        {
//            struct musb_request *zero_request;
//            os_printk(K_DEBUG, "EP%d TX2  %d\n",request->epnum,request->request.length);
                    
//            zero_request = next_request(musb_ep); //Get the request out from the list
//            if (!zero_request){
//                os_printk(K_DEBUG, "This should not happen that zero request of EP%d is NULL.\n", zero_request->epnum); 
//                return -1;
//            }
//            os_printk(K_DEBUG, "\n\n\n**** Give back zero request of EP%d. actual: %d, length:%d\n\n\n\n", zero_request->epnum, zero_request->request.actual, zero_request->request.length);
            //if (request->actual != request->length){
            //  os_printk(K_DEBUG, "This should not happen...?\n");
            //} 
            musb_g_giveback(ept, req, 0);                  
        }
    }
    else    
    { //rx
        
        os_printk(K_DEBUG, "EP%d RX %d\n",ept->num,req->length);          
        mu3d_hal_insert_transfer_gpd(ept->num, USB_RX, (DEV_UINT8*)(req->buf), req->length, 1,1,0,1, ept->maxpkt);
        mu3d_hal_resume_qmu(ept->num, USB_RX);
    //  while (0 == ((os_readl(U3D_QISAR0) & os_readl(U3D_QIER0))>>17));
    //  printf("RX queue finish!\n");
    
                  
    }//request->tx          
#else
        /* read */
        if (!ept->in) {
            ept->rcv_urb->buffer = req->buf;
            ept->rcv_urb->actual_length = 0;
    
        }
    
        /* write */
        if (ept->in) {
            ept->tx_urb->buffer = req->buf;
            ept->tx_urb->actual_length = req->length;
    
            musb_g_tx(ept->num);
        }
#endif

	return 0;
}

static unsigned long ept_alloc_table = EPT_TX(0) | EPT_RX(0);

struct udc_endpoint *udc_endpoint_alloc(unsigned type, unsigned maxpkt)
{
	struct udc_endpoint *ept;
	unsigned n;
	unsigned in;

	if (type == UDC_TYPE_BULK_IN) {
		in = 1;
	} else if (type == UDC_TYPE_BULK_OUT) {
		in = 0;
	} else {
		return 0;
	}

	/* udc_endpoint_alloc is used for EPx except EP0 */
	for (n = 1; n < 16; n++) {
		unsigned long bit = in ? EPT_TX(n) : EPT_RX(n);
		if (ept_alloc_table & bit)
			continue;
    printf("ept_alloc_table = 0x%x",ept_alloc_table);
		ept = _udc_endpoint_alloc(n, in, maxpkt);
		if (ept)
			ept_alloc_table |= bit;
		return ept;
	}

	return 0;
}

void udc_endpoint_free(struct udc_endpoint *ept)
{
	/* todo */
}

/**
 * u3d_init - initialize mac & qmu/bmu
 *
 */
void u3d_init(void){

#if 1 //DMA configuration
  DEV_UINT32 regval;
                regval = os_readl(0xF000D5AC);
                regval |= 0x00000007;    
                os_writel(regval, 0xF000D5AC);
            
#endif
    /* initialize PHY related data structure */
    u3phy_init();
    
    /* USB 2.0 slew rate calibration */
    u3phy_ops->u2_slew_rate_calibration(u3phy);

	/* disable ip power down, disable U2/U3 ip power down */
	mu3d_hal_ssusb_en();
	/* reset U3D all dev module. */
	mu3d_hal_rst_dev();
	/* apply default register values */
//	mu3d_hal_dft_reg();
	
	/* register U3D ISR. */
	mu3d_hal_initr_dis();

  	
	/* Initialize QMU GPD/BD memory. */
	mu3d_hal_alloc_qmu_mem();
	/* Initialize usb speed. */
	mu3d_hal_set_speed(U3D_DFT_SPEED);
	/* Detect usb speed. */
	//speed depends on host/cable/device; so speed chk is bypassed
	mu3d_hal_det_speed(U3D_DFT_SPEED, 0);
    /* initialize usb ep0 & system */
//    os_writel(U3D_LV1IESR, 0xFFFFFFFF); //u3d_irq_en
//	u3d_initialize_drv();
#if (BUS_MODE==QMU_MODE)
	/* Initialize QMU module. */
    usb_initialize_qmu();
//	mu3d_hal_init_qmu();
#endif
#ifdef POWER_SAVING_MODE
	mu3d_hal_pdn_cg_en();
#endif

}

int udc_init(struct udc_device *dev)
{
	struct udc_descriptor *desc = NULL;
	u32 usb_l1intm;

	DBG_I("[USB] %s: begin.\n", __func__);

	mdelay(20);
	/* usb phy init */
	u3d_init();

	mu3d_hal_u2dev_connect();


	/* allocate ep0 */
	ep0out = _udc_endpoint_alloc(EP0, 0, EP0_MAX_PACKET_SIZE);
	ep0in = _udc_endpoint_alloc(EP0, 1, EP0_MAX_PACKET_SIZE);
	ep0req = udc_request_alloc();
	ep0req->buf = malloc(4096);
	ep0_urb->buffer = ep0_buf;

	mdelay(20);
	
	{
		/* create and register a language table descriptor */
		/* language 0x0409 is US English */
		desc = udc_descriptor_alloc(TYPE_STRING, EP0, 4);
		desc->data[2] = 0x09;
		desc->data[3] = 0x04;
		udc_descriptor_register(desc);
	}

//	MOTG_REG_WRITE32(USB_L1INTM, 0x2f);
	
	the_device = dev;
	return 0;
}

int udc_register_gadget(struct udc_gadget *gadget)
{
	if (the_gadget) {
		DBG_C("only one gadget supported\n");
		return FALSE;
	}
	the_gadget = gadget;
	return 0;
}

int udc_start(void)
{
	struct udc_descriptor *desc;
	unsigned char *data;
	unsigned size;

	DBG_C("[USB] %s\n", __func__);

	if (!the_device) {
		DBG_C("udc cannot start before init\n");
		return FALSE;
	}
	if (!the_gadget) {
		DBG_C("udc has no gadget registered\n");
		return FALSE;
	}

	/* create our device descriptor */
	desc = udc_descriptor_alloc(TYPE_DEVICE, EP0, 18);
	data = desc->data;
	data[2] = 0x00;		/* usb spec minor rev */
	data[3] = 0x02;		/* usb spec major rev */
	data[4] = 0x00;		/* class */
	data[5] = 0x00;		/* subclass */
	data[6] = 0x00;		/* protocol */
	data[7] = 0x40;		/* max packet size on ept 0 */
	memcpy(data + 8, &the_device->vendor_id, sizeof(short));
	memcpy(data + 10, &the_device->product_id, sizeof(short));
	memcpy(data + 12, &the_device->version_id, sizeof(short));
	data[14] = udc_string_desc_alloc(the_device->manufacturer);
	data[15] = udc_string_desc_alloc(the_device->product);
	data[16] = udc_string_desc_alloc(the_device->serialno);
	data[17] = 1;		/* number of configurations */
	udc_descriptor_register(desc);

	/* create our configuration descriptor */
	size = 9 + udc_ifc_desc_size(the_gadget);
	desc = udc_descriptor_alloc(TYPE_CONFIGURATION, EP0, size);
	data = desc->data;
	data[0] = 0x09;
	data[2] = size;
	data[3] = size >> 8;
	data[4] = 0x01;		/* number of interfaces */
	data[5] = 0x01;		/* configuration value */
	data[6] = 0x00;		/* configuration string */
	data[7] = 0x80;		/* attributes */
	data[8] = 0x80;		/* max power (250ma) -- todo fix this */

	udc_ifc_desc_fill(the_gadget, data + 9);
	udc_descriptor_register(desc);

#if DBG_USB_DUMP_DESC
	DBG_D("%s: dump desc_list\n", __func__);
	for (desc = desc_list; desc; desc = desc->next) {
		DBG_D("tag: %04x\n", desc->tag);
		DBG_D("len: %d\n", desc->len);
		DBG_D("data:");
		hexdump8(desc->data, desc->len);
	}
#endif

	/* unmask usb irq */
    {
        os_writel(U3D_LV1IESR, 0xFFFFFFFF);
        mu3d_hal_dft_reg();
    
	#ifndef EXT_VBUS_DET
        os_writel(U3D_MISC_CTRL, 0); //use VBUS comparation on test chip
	#else
        os_writel(U3D_MISC_CTRL, 0x3); //force VBUS on  
	#endif
        os_writel(U3D_MISC_CTRL, 0x3); //force VBUS on  
        
        os_writel(U3D_LTSSM_RXDETECT_CTRL, 0xc801); //set 200us delay before do Tx detect Rx to workaround PHY test chip issue
    
        mu3d_hal_system_intr_en();
    
#ifdef USE_SSUSB_QMU //For QMU, we don't enable EP interrupts.
        os_writel(U3D_EPIESR, EP0ISR);//enable 0 interrupt  
#else
        os_writel(U3D_EPIESR, 0xFFFF | ((0XFFFE << 16) ));  
#endif //	#ifndef USE_SSUSB_QMU
    
        //HS/FS detected by HW
        os_writel(U3D_POWER_MANAGEMENT, os_readl(U3D_POWER_MANAGEMENT) | SOFT_CONN | HS_ENABLE);
        //disable U3 port
        os_writel(U3D_USB3_CONFIG, 0);
        //U2/U3 detected by HW
        os_writel(U3D_DEVICE_CONF, 0);
        
        os_writel(U3D_USB2_TEST_MODE, 0); //clear test mode
    }

	return 0;
}

int udc_stop(void)
{
	mu3d_hal_u2dev_disconn();

	return 0;
}

void ep0_setup(u16 maxpacket)
{


	os_writelmskumsk(U3D_EP0CSR, maxpacket, EP0_MAXPKTSZ0, EP0_W1C_BITS);
//	os_writel(U3D_EPIESR, os_readl(U3D_EPIESR)|EP0ISR);//enable EP0 interrupt
    os_writel(U3D_EPIESR, 0xFFFF | ((0XFFFE << 16) ));	

}

void musb_g_reset(void)
{

    ep0_setup(EP0_MAX_PACKET_SIZE);

}

/*
 * Interrupt Service Routine to record USB "global" interrupts.
 * Since these do not happen often and signify things of
 * paramount importance, it seems OK to check them individually;
 * the order of the tests is specified in the manual
 *
 * @param musb instance pointer
 * @param int_usb register contents
 * @param devctl
 * @param power
 */

void musb_stage0_irq(u32 int_usb, u8 devctl, u8 power)
{

	os_printk(K_DEBUG, "<== Power=%02x, DevCtl=%02x, int_usb=0x%x\n", power, devctl,
		int_usb);

	/* in host mode, the peripheral may issue remote wakeup.
	 * in peripheral mode, the host may resume the link.
	 * spurious RESUME irqs happen too, paired with SUSPEND.
	 */
	if (int_usb & RESUME_INTR) {
		os_printk(K_DEBUG, "RESUME_INTR \n");

		//We implement device mode only.
//				if ((devctl & USB_DEVCTL_VBUSVALID)
//						!= (3 << USB_DEVCTL_VBUS_OFFSET)
//						) {
//					musb->int_usb |= DISCONN_INTR;
//					musb->int_usb &= ~SUSPEND_INTR;
//					break;
//				}
//				musb_g_resume(musb);
		}

	if (int_usb & SUSPEND_INTR) {
		os_printk(K_DEBUG, "SUSPEND_INTR");

//			musb_g_suspend(musb);
	}

	if (int_usb & CONN_INTR) {
		u32 int_en = 0;

		os_printk(K_DEBUG, "----- ep0 state: MUSB_EP0_START\n");

		/* flush endpoints when transitioning from Device Mode */
#ifdef USE_SSUSB_QMU //For QMU, we don't enable EP interrupts.

	os_writel(U3D_EPIESR, EP0ISR);//enable 0 interrupt	

#else
    os_writel(U3D_EPIESR, 0xFFFF | ((0XFFFE << 16) ));	
#endif //	#ifndef USE_SSUSB_QMU
	
		int_en = SUSPEND_INTR_EN|RESUME_INTR_EN|RESET_INTR_EN|CONN_INTR_EN|DISCONN_INTR_EN;

		os_writel(U3D_COMMON_USB_INTR_ENABLE, int_en);


		os_printk(K_DEBUG, "CONN_INTR devctl %02x\n", devctl);
	}

	if (int_usb & DISCONN_INTR) {
		os_printk(K_DEBUG, "DISCONN_INTR , devctl %02x\n",devctl);

#if USBHSET
		//Clear test mode when bus reset to facilitate electrical test. However this doesn't comply to spec.
        printf("DISCONNECT\n");		
		if(os_readl(U3D_USB2_TEST_MODE) != 0)
		{
			os_writel(U3D_USB2_TEST_MODE, 0);
			os_writel(U3D_POWER_MANAGEMENT, os_readl(U3D_POWER_MANAGEMENT) & ~SOFT_CONN);
			os_writel(U3D_POWER_MANAGEMENT, os_readl(U3D_POWER_MANAGEMENT) | SOFT_CONN);
		}
		
#endif
//			musb_g_disconnect(musb);
	}

	/* mentor saves a bit: bus reset and babble share the same irq.
	 * only host sees babble; only peripheral sees bus reset.
	 */
	if (int_usb & RESET_INTR) {
#ifdef POWER_SAVING_MODE
		os_writel(U3D_SSUSB_U3_CTRL_0P, os_readl(U3D_SSUSB_U3_CTRL_0P) & ~SSUSB_U3_PORT_PDN);
#endif

		os_printk(K_DEBUG, "BUS RESET_INTR \n");
		musb_g_reset();
	}


}

static int ep0_standard_setup(struct urb *urb) {
	struct setup_packet *request;
	struct udc_descriptor *desc;
	u8 *cp = urb->buffer;

	/* for CLEAR FEATURE */
	u8 ep_num;  /* ep number */
	u8 dir; /* DIR */
	struct udc_endpoint *endpoint;

	request = &urb->device_request;
	//device = urb->device;

	dump_setup_packet("[USB] Device Request\n", request);

	if ((request->type & USB_TYPE_MASK) != 0) {
		return FALSE;			/* Class-specific requests are handled elsewhere */
	}

	/* handle all requests that return data (direction bit set on bm RequestType) */
	if ((request->type & USB_EP_DIR_MASK)) {
		/* send the descriptor */
		ep0_state = EP0_TX;

		switch (request->request) {
		/* data stage: from device to host */
		case GET_STATUS:
			DBG_I("GET_STATUS\n");
			urb->actual_length = 2;
			cp[0] = cp[1] = 0;
			switch (request->type & USB_RECIP_MASK) {
			case USB_RECIP_DEVICE:
				cp[0] = USB_STAT_SELFPOWERED;
				break;
			case USB_RECIP_OTHER:
				urb->actual_length = 0;
				break;
			default:
				break;
			}

			return 0;

		case GET_DESCRIPTOR:
//			DBG_I("GET_DESCRIPTOR\n");
			for (desc = desc_list; desc; desc = desc->next) {
				#if DBG_USB_DUMP_DESC
				DBG_D("desc->tag: %x: request->value: %x\n", desc->tag, request->value);
				#endif
				if (desc->tag == request->value) {
					#if DBG_USB_DUMP_DESC
					DBG_D("Find packet!\n");
					#endif
					unsigned len = desc->len;
					if (len > request->length)
						len = request->length;

//					DBG_I("%s: urb: %x, cp: %p\n", __func__, urb, cp);
					copy_desc(urb, desc->data, len);
					return 0;
				}
			}
			/* descriptor lookup failed */
			return FALSE;

		case GET_CONFIGURATION:
			DBG_I("GET_CONFIGURATION\n");
			urb->actual_length = 1;
			cp[0] = 1;
			return 0;

		case GET_INTERFACE:
			DBG_I("GET_INTERFACE\n");

		default:
			DBG_C("Unsupported command with TX data stage\n");
			break;
		}
	} else {

		switch (request->request) {

		case SET_ADDRESS:
			DBG_I("SET_ADDRESS\n");

			dev_address = (request->value);
			set_address = 1;
			return 0;

		case SET_CONFIGURATION:
			DBG_I("SET_CONFIGURATION\n");

			if (request->value == 1) {
				usb_config_value = 1;
				the_gadget->notify(the_gadget, UDC_EVENT_ONLINE);
			} else {
				usb_config_value = 0;
				the_gadget->notify(the_gadget, UDC_EVENT_OFFLINE);
			}

			usb_online = request->value ? 1 : 0;
			//usb_status(request->value ? 1 : 0, usb_highspeed);

			return 0;
			
			case CLEAR_FEATURE: 
			
				DBG_I("CLEAR_FEATURE\n");

				ep_num = request->index & 0xf;
				dir = request->index & 0x80;

				if ((request->value == 0) && (request->length == 0)) {

					DBG_I("Clear Feature: ep: %d dir: %d\n", ep_num, dir);

					endpoint = mt_find_ep(ep_num, dir);
                    if(dir==UDC_DIR_OUT)
                        dir=USB_RX;
                    else
                        dir=USB_TX;
//					mt_setup_ep((unsigned int)ep_num, endpoint);
                    mu3d_hal_ep_enable(ep_num, dir, 2, endpoint->maxpkt, 0,0,0,0); //slot_id=0
					return 0;
				}

		default:
			DBG_C("Unsupported command with RX data stage\n");
			break;

		}
	}
	return FALSE;
}

static void mt_udc_ep0_write(void) {

	struct udc_endpoint *endpoint = &ep_list[EP0];
	int count = 0;
	u32 csr0 = 0;


	csr0 = os_readl(U3D_EP0CSR) & EP0_W1C_BITS; //Don't W1C
	DBG_I("U3D_EP0CSR = 0x%x\n", os_readl(U3D_EP0CSR));
	if (csr0 & EP0_TXPKTRDY) {
		DBG_I("mt_udc_ep0_write: ep0 is not ready to be written\n");
		return;
	}

    while(os_readl(U3D_EP0CSR) & EP0_FIFOFULL)//Wait until FIFOFULL cleared by hrdc
    {
            mdelay(1);
    }           
        
//	count = mt_write_fifo(endpoint);
    if(!(os_readl(U3D_EP0CSR) & EP0_AUTOSET)){
        os_writel(U3D_EP0CSR, os_readl(U3D_EP0CSR)|EP0_AUTOSET);
    }
        
//    os_writel(U3D_EP0CSR, csr0 | EP0_SETUPPKTRDY | EP0_DPHTX);
    count = mu3d_hal_write_fifo(0,ep0_urb->actual_length,ep0_urb->buffer, EP0_MAX_PACKET_SIZE);

    mdelay(1);
	DBG_I("%s: count = %d\n", __func__, count);

	if (count < EP0_MAX_PACKET_SIZE) {
		/* last packet */
		csr0 |= EP0_DATAEND;
		ep0_urb->actual_length = 0;
		endpoint->sent = 0;
		ep0_state = EP0_IDLE;
	} else {
		/* more packets are waiting to be transferred */
		csr0 |= EP0_TXPKTRDY;
	}

    os_writel(U3D_EP0CSR, csr0); 
	return;
}

static void mt_udc_ep0_read(void) {

	struct udc_endpoint *endpoint = &ep_list[EP0];
	int count = 0;
	u32 csr0 = 0;

	csr0 = os_readl(U3D_EP0CSR) & EP0_W1C_BITS; //Don't W1C

	/* erroneous ep0 interrupt */
	if (!(csr0 & EP0_RXPKTRDY)) {
		return;
	}

//	count = mt_read_fifo(endpoint);
    if(!(os_readl(U3D_EP0CSR) & EP0_AUTOCLEAR)){
        os_writel(U3D_EP0CSR, os_readl(U3D_EP0CSR)|EP0_AUTOCLEAR);
    }

//    os_writel(U3D_EP0CSR, (csr0 | EP0_SETUPPKTRDY) & (~EP0_DPHTX)); //Set CSR0.SetupPktRdy(W1C) & CSR0.DPHTX=0
	count = mu3d_hal_read_fifo(0,(u8 *)ep0_urb->buffer);
    ep0_urb->actual_length += count;



	return;
}

static void mt_udc_ep0_setup(void) {
	struct udc_endpoint *endpoint = &ep_list[0];
	u8 index;
	u8 stall = 0;
	u32 csr0;
	struct setup_packet *request;
	u16 count;

//	DBG_I("mt_udc_ep0_setup begin.\n");
	/* Read control status register for endpiont 0 */
	csr0 = os_readl(U3D_EP0CSR);

	/* check whether RxPktRdy is set? */
	if (!(csr0 & EP0_SETUPPKTRDY))//EP0_RXPKTRDY))
		return;

	/* unload fifo */
	ep0_urb->actual_length = 0;

    if(!(os_readl(U3D_EP0CSR) & EP0_AUTOCLEAR)){
        os_writel(U3D_EP0CSR, os_readl(U3D_EP0CSR)|EP0_AUTOCLEAR);
    }
	count = mu3d_hal_read_fifo(0,(u8 *)ep0_urb->buffer);
    ep0_urb->actual_length += count;

	#if DBG_USB_FIFO
	DBG_I("%s: mt_read_fifo count = %d, %d\n", __func__, count, sizeof(struct setup_packet));
	#endif

	/* decode command */
	request = &ep0_urb->device_request;
	memcpy(request, ep0_urb->buffer, sizeof(struct setup_packet));

	if (((request->type) & USB_TYPE_MASK) == USB_TYPE_STANDARD) {
		DBG_I("[USB] Standard Request\n");
		stall = ep0_standard_setup(ep0_urb);
		if (stall) {
			dump_setup_packet("[USB] STANDARD REQUEST NOT SUPPORTED\n", request);
		}
	} else if (((request->type) & USB_TYPE_MASK) == USB_TYPE_CLASS) {
		DBG_I("[USB] Class-Specific Request\n");
		if (stall) {
			dump_setup_packet("[USB] CLASS REQUEST NOT SUPPORTED\n", request);
		}
	} else if (((request->type) & USB_TYPE_MASK) == USB_TYPE_VENDOR) {
		DBG_I("[USB] Vendor-Specific Request\n");
		/* do nothing now */
		DBG_I("[USB] ALL VENDOR-SPECIFIC REQUESTS ARE NOT SUPPORTED!!\n");
	}

	if (stall) {
		/* the received command is not supported */
//		udc_stall_ep(0, UDC_DIR_OUT);
    {// stall ep
		os_writel(U3D_EP0CSR, (csr0 & EP0_W1C_BITS) | EP0_SENTSTALL); //EP0_SENTSTALL is W1C

		if (os_readl(U3D_EP0CSR) & EP0_TXPKTRDY) //try to flushfifo after clear sentstall
		{
			
			csr0 = os_readl(U3D_EP0CSR);
			csr0 &= EP0_W1C_BITS; //don't clear W1C bits
			csr0 |= CSR0_FLUSHFIFO;
			os_writel(U3D_EP0CSR, csr0);

			os_printk(K_DEBUG, "waiting for flushfifo.....");
			while(os_readl(U3D_EP0CSR) & CSR0_FLUSHFIFO) //proceed before it clears
			{
				mdelay(1);
			}
			os_printk(K_DEBUG, "done.\n");
		}
		csr0 = os_readl(U3D_EP0CSR);
		os_printk(K_DEBUG, "----- ep0 state: MUSB_EP0_STAGE_IDLE. now csr is 0x%04x\n", csr0);
	}
    return;
	}

	switch (ep0_state) {
	case EP0_TX:
		/* data stage: from device to host */
		DBG_I("%s: EP0_TX\n", __func__);
		os_writel(U3D_EP0CSR, csr0 | EP0_SETUPPKTRDY | EP0_DPHTX);
		mt_udc_ep0_write();

		break;
	case EP0_RX:
		/* data stage: from host to device */
		DBG_I("%s: EP0_RX\n", __func__);
		os_writel(U3D_EP0CSR, (csr0 | EP0_SETUPPKTRDY) & (~EP0_DPHTX)); //Set CSR0.SetupPktRdy(W1C) & CSR0.DPHTX=0

		break;
	case EP0_IDLE:
		/* no data stage */
//		DBG_I("%s: EP0_IDLE\n", __func__);
		os_writel(U3D_EP0CSR, (csr0 | EP0_SETUPPKTRDY) & (~EP0_DPHTX)); //Set CSR0.SetupPktRdy(W1C) & CSR0.DPHTX=0
		csr0 = os_readl(U3D_EP0CSR);
		csr0 |= EP0_DATAEND;

		os_writel(U3D_EP0CSR, csr0);
		os_writel(U3D_EP0CSR, csr0);

		break;
	default:
		break;
	}

	return;

}

static void mt_udc_ep0_handler(void) {

	u32		csr0;

	csr0 = os_readl(U3D_EP0CSR);

//	DBG_I("mt_udc_ep0_handler begin csr0=0x%x.\n", csr0);
	if (csr0 & EP0_SENTSTALL) {
		DBG_I("USB: [EP0] SENTSTALL\n");
		/* needs implementation for exception handling here */
		ep0_state = EP0_IDLE;
	}


	switch (ep0_state) {
	case EP0_IDLE:
//		DBG_I("%s: EP0_IDLE\n", __func__);
		mt_udc_ep0_setup();
		if (set_address) {
            os_writel(U3D_DEVICE_CONF, os_readl(U3D_DEVICE_CONF) | (dev_address << DEV_ADDR_OFST));
			set_address = 0;
            os_writel(U3D_EP0CSR,os_readl(U3D_EP0CSR) | EP0_SETUPPKTRDY | EP0_DATAEND);
            os_printk(K_DEBUG,"Device Address :%x\n",(os_readl(U3D_DEVICE_CONF)>>DEV_ADDR_OFST));
		}
		break;
	case EP0_TX:
		DBG_I("%s: EP0_TX\n", __func__);

		mt_udc_ep0_write();
		break;
	case EP0_RX:
		DBG_I("%s: EP0_RX\n", __func__);
		mt_udc_ep0_read();
		break;
	default:
		break;
	}


	return;
}

#ifndef USE_SSUSB_QMU	
/*
 * Context: controller locked, IRQs blocked, endpoint selected
 */
static void rxstate(u8 epnum)
{
    struct udc_endpoint *endpoint;
    struct udc_request *req;
	unsigned		fifo_count = 0;
	u32			rxcsr0,ep_tmp;

    endpoint = mt_find_ep(epnum, UDC_DIR_OUT);

    ep_tmp = epnum - 1;
	rxcsr0 = os_readl(U3D_RX1CSR0+(ep_tmp*0x10));
    req = endpoint->req	;

	if(rxcsr0 & RX_SENDSTALL) {
		printf("RX_SENDSTALL \n");
	
		return;
	}

	if (rxcsr0 & RX_RXPKTRDY)
   {
//		printk("Enter RX_RXPKTRDY before read fifo\n");    

			fifo_count = mu3d_hal_read_fifo(epnum,(req->buf + endpoint->rcv_urb->actual_length));
			endpoint->rcv_urb->actual_length += fifo_count;
	}

    /* Deal with FASTBOOT command */
    if ((req->length >= endpoint->rcv_urb->actual_length) && req->length == 64) {
        req->length = fifo_count;
    
        /* mask EPx INTRRXE */
        /* The buffer is passed from the AP caller.
         * It happens that AP is dealing with the buffer filled data by driver,
         * but the driver is still receiving the next data packets onto the buffer.
         * Data corrupted happens if the every request use the same buffer.
         * Mask the EPx to ensure that AP and driver are not accessing the buffer parallely.
         */
    }
    
    /* Deal with DATA transfer */
    if ((req->length == endpoint->rcv_urb->actual_length) ||
    ((req->length >= endpoint->rcv_urb->actual_length) && req->length == 64)) {
        handle_ept_complete(endpoint);
    
        /* mask EPx INTRRXE */
        /* The buffer is passed from the AP caller.
         * It happens that AP is dealing with the buffer filled data by driver,
         * but the driver is still receiving the next data packets onto the buffer.
         * Data corrupted happens if the every request use the same buffer.
         * Mask the EPx to ensure that AP and driver are not accessing the buffer parallely.
         */
//		printk("musb_g_giveback \n");    
		musb_g_giveback(endpoint, req, 0);
    }
//		printk("rxstate end\n");    
    
}

/*
 * Data ready for a request; called from IRQ
 */
void musb_g_rx(u8 epnum)
{
	u32			rxcsr0,ep_tmp;

    ep_tmp = epnum - 1;
	rxcsr0 = os_readl(U3D_RX1CSR0+(ep_tmp*0x10));

	if (rxcsr0 & RX_SENTSTALL) {
		printf("RX_SENDSTALL \n");

		//EPN needs to continuous sending STALL until host set clear_feature to clear the status.				
		
		//musb_writew(epio, MUSB_RXCSR, csr);
		os_writel(U3D_RX1CSR0+(ep_tmp*0x10), (rxcsr0 & RX_W1C_BITS) | RX_SENTSTALL); //SENTSTALL is W1C. So write again to clear it.
		
		return;
	}


//    printk("call  rxstate\n");

	/* Analyze request */
	rxstate(epnum);
}
#endif
/*
 * handle all the irqs defined by the HDRC core. for now we expect:  other
 * irq sources (phy, dma, etc) will be handled first, musb->int_* values
 * will be assigned, and the irq will already have been acked.
 *
 * called in irq context with spinlock held, irqs blocked
 */
void musb_interrupt(u32 int_usb, u16 int_tx, u16 int_rx)
{
	u8		devctl, power = 0;
#ifndef USE_SSUSB_QMU	
	u32 	reg = 0, ep_num = 0;
    struct udc_endpoint *endpoint;
#endif

#ifdef POWER_SAVING_MODE
		if(!(os_readl(U3D_SSUSB_U2_CTRL_0P) & SSUSB_U2_PORT_DIS)){
			devctl = (u8)os_readl(U3D_DEVICE_CONTROL);
			power = (u8)os_readl(U3D_POWER_MANAGEMENT); 

		}else{
			devctl = 0;
			power = 0;
//			musb->int_usb = 0;
		}
#else
		devctl = (u8)os_readl(U3D_DEVICE_CONTROL);
		power = (u8)os_readl(U3D_POWER_MANAGEMENT); 
#endif
		
	os_printk(K_DEBUG, "** IRQ %s usb%04x tx%04x rx%04x\n",
		(devctl & USB_DEVCTL_HOSTMODE) ? "host" : "peripheral",
		int_usb, int_tx, int_rx);

	/* the core can interrupt us for multiple reasons; docs have
	 * a generic interrupt flowchart to follow
	 */
	if (int_usb)
		musb_stage0_irq(int_usb,devctl, power);

	/* "stage 1" is handling endpoint irqs */

	/* handle endpoint 0 first */
//	if ((int_tx & 1) || (int_rx & 1)) {
		mt_udc_ep0_handler();
//	}

#ifndef USE_SSUSB_QMU
	//With QMU, we don't use RX/TX interrupt to trigger EPN flow.

	/* TX on endpoints 1-15 */
	reg = int_tx >> 1;
	ep_num = 1;
	
	while (reg) {
		if (reg & 1) {
			/* musb_ep_select(musb->mregs, ep_num); */
			/* REVISIT just retval |= ep->tx_irq(...) */
			printf("EP%d TX interrupt \n",ep_num);
			musb_g_tx(ep_num);
        }
		reg >>= 1;
		ep_num++;
	}

	/* RX on endpoints 1-15 */
	reg = int_rx >> 1;
	ep_num = 1;
	while (reg) {
        endpoint = mt_find_ep(ep_num, UDC_DIR_OUT);
        if(endpoint->req){
		if (reg & 1) {
			/* musb_ep_select(musb->mregs, ep_num); */
			/* REVISIT just retval = ep->rx_irq(...) */
			os_printk(K_DEBUG,"EP%d RX interrupt \n",ep_num);
         	os_printk(K_DEBUG,"----rxcsr0 is 0x%X\n" ,os_readl(U3D_RX1CSR0));
			
            os_writel(U3D_EPISR, int_rx<<16);
			musb_g_rx(ep_num);
			
		}
        }

		reg >>= 1;
		ep_num++;
	}

#endif //#ifndef USE_SSUSB_QMU
}

void service_interrupts(void)
    {
    
        u32 dwIntrUsbValue;
        u32 dwDmaIntrValue;
        u32 dwIntrEPValue;
        u16 wIntrTxValue;
        u16 wIntrRxValue;
        u32 wIntrQMUValue = 0;
        u32 wIntrQMUDoneValue = 0;
        u32 dwLtssmValue;
        u32 dwLinkIntValue;
        u32 dwTemp;
    
    //  os_printk(K_EMERG, "generic_interrupt start\n");
    if(!(os_readl(0xf0008048)&0x01000000))
        return;
        
    //    printf("EP1RXCSR0 is 0x%X\n",os_readl(0xfe601210));
        if(os_readl(U3D_LV1ISR) & MAC2_INTR){
            dwIntrUsbValue = os_readl(U3D_COMMON_USB_INTR) & os_readl(U3D_COMMON_USB_INTR_ENABLE);
        }
        else{
            dwIntrUsbValue = 0;
        }
        if(os_readl(U3D_LV1ISR) & MAC3_INTR){
            dwLtssmValue = os_readl(U3D_LTSSM_INTR) & os_readl(U3D_LTSSM_INTR_ENABLE);
        }
        else{
            dwLtssmValue = 0;
        }
        dwDmaIntrValue = os_readl(U3D_DMAISR)& os_readl(U3D_DMAIER);
        dwIntrEPValue = os_readl(U3D_EPISR)& os_readl(U3D_EPIER);
        dwLinkIntValue = os_readl(U3D_DEV_LINK_INTR) & os_readl(U3D_DEV_LINK_INTR_ENABLE);      
    
        //for QMU, process this later.
        wIntrQMUValue = os_readl(U3D_QISAR1);
        wIntrQMUDoneValue = os_readl(U3D_QISAR0) & os_readl(U3D_QIER0);
        wIntrTxValue = dwIntrEPValue&0xFFFF;
        wIntrRxValue = (dwIntrEPValue>>16);
        
    //    printf("wIntrTxValue is 0x%X\n",wIntrTxValue);
    
        //os_printk(K_DEBUG,"dwIntrEPValue :%x\n",dwIntrEPValue);
        //os_printk(K_DEBUG,"USB_EPIntSts :%x\n",Reg_Read32(USB_EPIntSts));
        os_printk(K_INFO, "Interrupt: IntrUsb [%x] IntrTx[%x] IntrRx [%x] IntrDMA[%x] IntrQMUDone[%x] IntrQMU [%x] IntrLTSSM [%x]\r\n", 
            dwIntrUsbValue,
            wIntrTxValue,
            wIntrRxValue,
            dwDmaIntrValue, wIntrQMUDoneValue, wIntrQMUValue, dwLtssmValue);        
    
    //  os_printk(K_EMERG, "Interrupt: IntrQMUDone[%x]\r\n", wIntrQMUDoneValue);
    
    
        /* write one clear */
        os_writel(U3D_QISAR0, wIntrQMUDoneValue);
    
        if(os_readl(U3D_LV1ISR) & MAC2_INTR){
            os_writel(U3D_COMMON_USB_INTR, dwIntrUsbValue);
        }
        if(os_readl(U3D_LV1ISR) & MAC3_INTR){
            os_writel(U3D_LTSSM_INTR, dwLtssmValue); 
        }
#ifdef USE_SSUSB_QMU 
        os_writel(U3D_EPISR, dwIntrEPValue);
#else
        os_writel(U3D_EPISR, wIntrTxValue);
#endif
        os_writel(U3D_DMAISR, dwDmaIntrValue); 
        os_writel(U3D_DEV_LINK_INTR, dwLinkIntValue); 
    
#ifdef USE_SSUSB_QMU 
        //For speeding up QMU processing, let it be the first priority to be executed. Other interrupts will be handled at next run.
        //ported from Liang-yen's code
        if(wIntrQMUDoneValue){
            if(wIntrQMUDoneValue>>16)
                os_printk(K_INFO,"USB_QMU_RQCPR %x\r\n", os_readl(USB_QMU_RQCPR(1)));
            if(wIntrQMUDoneValue & 0xffff)
                os_printk(K_INFO,"USB_QMU_TQCPR %x\r\n", os_readl(USB_QMU_TQCPR(1)));
    
            qmu_done_interrupt(wIntrQMUDoneValue);

            DEV_INT32 i;
//            int ep_num;
            struct udc_endpoint *endpoint;
                for(i=1; i<=MAX_QMU_EP; i++){
                    if(wIntrQMUDoneValue & QMU_RX_DONE(i)){
                        endpoint = mt_find_ep(i, UDC_DIR_OUT);
                        handle_ept_complete(endpoint);
                    }
                    if(wIntrQMUDoneValue & QMU_TX_DONE(i)){
                        endpoint = mt_find_ep(i, UDC_DIR_IN);
                        handle_ept_complete(endpoint);
                    }
            
                }
        }
        
        if(wIntrQMUValue){
            qmu_exception_interrupt(wIntrQMUValue);
//            if(wIntrQMUValue==0x10000)
//                mu3d_hal_resume_qmu(1, USB_RX);
        }
#endif
          
            if (dwIntrUsbValue)
            {
                if (dwIntrUsbValue & DISCONN_INTR)
                {   
#ifdef POWER_SAVING_MODE
                    os_writel(U3D_SSUSB_IP_PW_CTRL2, os_readl(U3D_SSUSB_IP_PW_CTRL2) & ~SSUSB_IP_DEV_PDN);
                    os_writel(U3D_SSUSB_U3_CTRL_0P, os_readl(U3D_SSUSB_U3_CTRL_0P) & ~SSUSB_U3_PORT_PDN);
                    os_writel(U3D_SSUSB_U2_CTRL_0P, os_readl(U3D_SSUSB_U2_CTRL_0P) & ~SSUSB_U2_PORT_PDN);
#endif
                    os_printk(K_DEBUG, "[DISCONN_INTR]\n");
                    os_printk(K_DEBUG, "Set SOFT_CONN to 0\n");     
                    os_clrmsk(U3D_POWER_MANAGEMENT, SOFT_CONN);
                }
        
                if (dwIntrUsbValue & LPM_INTR)      
                {
                    os_printk(K_DEBUG, "LPM interrupt\n");
#ifdef POWER_SAVING_MODE
                    if(!((os_readl(U3D_POWER_MANAGEMENT) & LPM_HRWE))){
                        os_writel(U3D_SSUSB_U2_CTRL_0P, os_readl(U3D_SSUSB_U2_CTRL_0P) | SSUSB_U2_PORT_PDN);
                        os_writel(U3D_SSUSB_IP_PW_CTRL2, os_readl(U3D_SSUSB_IP_PW_CTRL2) | SSUSB_IP_DEV_PDN);       
                    }
#endif
                }
    
                if(dwIntrUsbValue & LPM_RESUME_INTR){
                    if(!(os_readl(U3D_POWER_MANAGEMENT) & LPM_HRWE)){
#ifdef POWER_SAVING_MODE
                        os_writel(U3D_SSUSB_IP_PW_CTRL2, os_readl(U3D_SSUSB_IP_PW_CTRL2) & ~SSUSB_IP_DEV_PDN);
                        os_writel(U3D_SSUSB_U2_CTRL_0P, os_readl(U3D_SSUSB_U2_CTRL_0P) & ~SSUSB_U2_PORT_PDN);
                        while(!(os_readl(U3D_SSUSB_IP_PW_STS2) & SSUSB_U2_MAC_SYS_RST_B_STS));                  
#endif
                        os_writel(U3D_USB20_MISC_CONTROL, os_readl(U3D_USB20_MISC_CONTROL) | LPM_U3_ACK_EN);
                    }
                }
    
                if(dwIntrUsbValue & SUSPEND_INTR){
                    os_printk(K_DEBUG,"Suspend Interrupt\n");
#ifdef POWER_SAVING_MODE
                    os_writel(U3D_SSUSB_U2_CTRL_0P, os_readl(U3D_SSUSB_U2_CTRL_0P) | SSUSB_U2_PORT_PDN);
                    os_writel(U3D_SSUSB_IP_PW_CTRL2, os_readl(U3D_SSUSB_IP_PW_CTRL2) | SSUSB_IP_DEV_PDN);
#endif
                }
                if (dwIntrUsbValue & RESUME_INTR){
                    os_printk(K_NOTICE,"Resume Interrupt\n");
#ifdef POWER_SAVING_MODE
                    os_writel(U3D_SSUSB_IP_PW_CTRL2, os_readl(U3D_SSUSB_IP_PW_CTRL2) & ~SSUSB_IP_DEV_PDN);
                    os_writel(U3D_SSUSB_U2_CTRL_0P, os_readl(U3D_SSUSB_U2_CTRL_0P) & ~SSUSB_U2_PORT_PDN);
                    while(!(os_readl(U3D_SSUSB_IP_PW_STS2) & SSUSB_U2_MAC_SYS_RST_B_STS));              
#endif
                }
        
        
            }
    
        if (dwLtssmValue)
        {
            if (dwLtssmValue & SS_DISABLE_INTR)
            {
//                os_printk(K_DEBUG, "LTSSM: SS_DISABLE_INTR [%d]\n", ++soft_conn_num);
                os_printk(K_DEBUG, "Set SOFT_CONN to 1\n");
                //enable U2 link. after host reset, HS/FS EP0 configuration is applied in musb_g_reset  
#ifdef POWER_SAVING_MODE
                os_writel(U3D_SSUSB_U2_CTRL_0P, os_readl(U3D_SSUSB_U2_CTRL_0P) &~ (SSUSB_U2_PORT_DIS | SSUSB_U2_PORT_PDN));
#endif
                os_setmsk(U3D_POWER_MANAGEMENT, SOFT_CONN);
            }
    
            if (dwLtssmValue & ENTER_U0_INTR)
            {
                os_printk(K_DEBUG, "LTSSM: ENTER_U0_INTR\n");
                //do not apply U3 EP0 setting again, if the speed is already U3
                //LTSSM may go to recovery and back to U0
//                if (musb->g.speed != USB_SPEED_SUPER)
//                    musb_conifg_ep0(musb);
            }
    
    
            if (dwLtssmValue & VBUS_FALL_INTR)
            {
#ifdef POWER_SAVING_MODE
                os_writel(U3D_SSUSB_IP_PW_CTRL2, os_readl(U3D_SSUSB_IP_PW_CTRL2) &~ SSUSB_IP_DEV_PDN);
                os_writel(U3D_SSUSB_U2_CTRL_0P, os_readl(U3D_SSUSB_U2_CTRL_0P) &~ (SSUSB_U2_PORT_DIS | SSUSB_U2_PORT_PDN));
                os_writel(U3D_SSUSB_U3_CTRL_0P, os_readl(U3D_SSUSB_U3_CTRL_0P) &~ SSUSB_U3_PORT_PDN);
#endif
                os_printk(K_DEBUG, "LTSSM: VBUS_FALL_INTR\n");      
                os_writel(U3D_USB3_CONFIG, 0);
            }
    
            if (dwLtssmValue & VBUS_RISE_INTR)
            {
                os_printk(K_DEBUG, "LTSSM: VBUS_RISE_INTR\n");      
                os_writel(U3D_USB3_CONFIG, USB3_EN);
            }
    
            if (dwLtssmValue & ENTER_U3_INTR)
            {
#ifdef POWER_SAVING_MODE
                os_writel(U3D_SSUSB_U3_CTRL_0P, os_readl(U3D_SSUSB_U3_CTRL_0P) | SSUSB_U3_PORT_PDN);
                os_writel(U3D_SSUSB_IP_PW_CTRL2, os_readl(U3D_SSUSB_IP_PW_CTRL2) | SSUSB_IP_DEV_PDN);
#endif
            }
            
            if (dwLtssmValue & EXIT_U3_INTR)
            {
#ifdef POWER_SAVING_MODE
                os_writel(U3D_SSUSB_U3_CTRL_0P, os_readl(U3D_SSUSB_U3_CTRL_0P) &~ SSUSB_U3_PORT_PDN);
                os_writel(U3D_SSUSB_IP_PW_CTRL2, os_readl(U3D_SSUSB_IP_PW_CTRL2) &~ SSUSB_IP_DEV_PDN);
                while(!(os_readl(U3D_SSUSB_IP_PW_STS1) & SSUSB_U3_MAC_RST_B_STS));          
#endif
            }
            
#ifndef POWER_SAVING_MODE
            if (dwLtssmValue & U3_RESUME_INTR)
            {
                os_writel(U3D_SSUSB_U3_CTRL_0P, os_readl(U3D_SSUSB_U3_CTRL_0P) &~ SSUSB_U3_PORT_PDN);
                os_writel(U3D_SSUSB_IP_PW_CTRL2, os_readl(U3D_SSUSB_IP_PW_CTRL2) &~ SSUSB_IP_DEV_PDN);
                while(!(os_readl(U3D_SSUSB_IP_PW_STS1) & SSUSB_U3_MAC_RST_B_STS));
                os_writel(U3D_LINK_POWER_CONTROL, os_readl(U3D_LINK_POWER_CONTROL) | UX_EXIT);
            }
#endif
    
        }
    
    
    if (dwIntrUsbValue || wIntrTxValue || wIntrRxValue)
        musb_interrupt(dwIntrUsbValue ,wIntrTxValue ,wIntrRxValue);
    
    else
        if (dwLinkIntValue & SSUSB_DEV_SPEED_CHG_INTR)
        {
            os_printk(K_ALET,"Speed Change Interrupt\n");
                    
            printf("Speed Change Interrupt : ");
            dwTemp = os_readl(U3D_DEVICE_CONF) & SSUSB_DEV_SPEED;
#ifdef POWER_SAVING_MODE
            mu3d_hal_pdn_cg_en();
#endif	
            switch (dwTemp)
            {
                case SSUSB_SPEED_FULL:
                    printf("FS\n");
                    //BESLCK = 4 < BESLCK_U3 = 10 < BESLDCK = 15
                    os_writel(U3D_USB20_LPM_PARAMETER, 0xa4f0);
                    os_setmsk(U3D_POWER_MANAGEMENT, (LPM_BESL_STALL|LPM_BESLD_STALL));
                break;
                case SSUSB_SPEED_HIGH:
                    printf("HS\n");
                    //BESLCK = 4 < BESLCK_U3 = 10 < BESLDCK = 15
                    os_writel(U3D_USB20_LPM_PARAMETER, 0xa4f0);
                    os_setmsk(U3D_POWER_MANAGEMENT, (LPM_BESL_STALL|LPM_BESLD_STALL));
                break;
                case SSUSB_SPEED_SUPER:
                    printf("SS\n");
                break;
                default:
                os_printk(K_ALET,"Invalid\n");
            }
        }
    
    
    
    }


