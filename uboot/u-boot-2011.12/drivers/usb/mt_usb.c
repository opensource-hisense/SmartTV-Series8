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

#include <usb.h>
#include "x_typedef.h"

#include "mgc_hdrc.h"
#include "mgc_dma.h"
#include "mt_usb.h"
#include "udc.h"

//#define URB_DEBUG

#ifdef URB_DEBUG
/* DEBUG INFO Sections */
#define DBG_USB_GENERAL_INFO 1
#define DBG_USB_DUMP_DESC 1
#define DBG_USB_DUMP_DATA 1
#define DBG_USB_DUMP_SETUP 0
#define DBG_USB_FIFO 1
#define DBG_USB_IRQ  1
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

/* bits used in all the endpoint status registers */
#define EPT_TX(n) (1 << ((n) + 16))
#define EPT_RX(n) (1 << (n))

/* udc.h wrapper for usbdcore */

static unsigned char usb_config_value = 0;
EP0_STATE ep0_state = EP0_IDLE;
int set_address = 0;
u32 fifo_addr = FIFO_ADDR_START;

#define EP0	0

/* USB transfer directions */
#define UDC_DIR_IN	DEVICE_WRITE	/* val: 0x80 */
#define UDC_DIR_OUT	DEVICE_READ	/* val: 0x00 */

#define EP0_MAX_PACKET_SIZE	64

/* Request types */
#define USB_TYPE_STANDARD	(0x00 << 5)
#define USB_TYPE_CLASS		(0x01 << 5)
#define USB_TYPE_VENDOR		(0x02 << 5)
#define USB_TYPE_RESERVED	(0x03 << 5)

/* values used in GET_STATUS requests */
#define USB_STAT_SELFPOWERED	0x01

/* USB recipients */
#define USB_RECIP_DEVICE	0x00
#define USB_RECIP_INTERFACE	0x01
#define USB_RECIP_ENDPOINT	0x02
#define USB_RECIP_OTHER		0x03

/* Endpoints */
#define USB_EP_NUM_MASK	0x0f		/* in bEndpointAddress */
#define USB_EP_DIR_MASK	0x80

#define USB_TYPE_MASK	0x60
#define USB_RECIP_MASK	0x1f


#define URB_BUF_SIZE 512

struct urb {
	struct udc_endpoint *endpoint;
	struct udc_device *device;
	struct setup_packet device_request;

	u8 *buffer;
	unsigned int actual_length;
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
#ifdef CONFIG_USB_FASTBOOT
#define URB_BUFFER_LEN    1024
char mt_urb_buffer[URB_BUFFER_LEN];
char mt_rx_urb_buffer[URB_BUFFER_LEN];
#endif


/* endpoint data - mt_ep */
struct udc_endpoint {
	/* rx side */
	struct urb *rcv_urb;	/* active urb */

	/* tx side */
	struct urb *tx_urb;	/* active urb */

	/* info from hsusb */
	struct udc_request *req;
	unsigned int bit;	/* EPT_TX/EPT_RX */
	unsigned char num;
	unsigned char in;
	unsigned short maxpkt;
	int status;	/* status for error handling */

	unsigned int sent;		/* data already sent */
	unsigned int last;		/* data sent in last packet XXX do we need this */
	unsigned char mode;	/* double buffer */
};

/* from mt_usbtty.h */
#define NUM_ENDPOINTS	3

// MUSB_GADGET_PORT_NUM's value get from /usb/Makefile 
#ifndef MUSB_GADGET_PORT_NUM
#define MUSB_GADGET_PORT_NUM       0 
#endif
static int u4Port = MUSB_GADGET_PORT_NUM;
/* origin endpoint_array */
struct udc_endpoint ep_list[NUM_ENDPOINTS + 1];	/* one extra for control endpoint */

static int usb_online = 0;

static u8 dev_address = 0;
static struct udc_device *the_device;
static struct udc_gadget *the_gadget;
/* end from hsusb.c */

/* declare ept_complete handle */
static void handle_ept_complete(struct udc_endpoint *ept);

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
unsigned int uffs(unsigned int x)
{
	unsigned int r = 1;

	if (!x)
		return 0;
	if (!(x & 0xffff)) {
		x >>= 16;
		r += 16;
	}
	if (!(x & 0xff)) {
		x >>= 8;
		r += 8;
	}
	if (!(x & 0xf)) {
		x >>= 4;
		r += 4;
	}
	if (!(x & 3)) {
		x >>= 2;
		r += 2;
	}
	if (!(x & 1)) {
		x >>= 1;
		r += 1;
	}
	return r;
}

void board_usb_init(void)
{
	u32 u4Reg;

	// Soft-reset 
	u4Reg = MOTG_PHY_REG_READ32((uint32_t)0x68);
	u4Reg |=   0x00004000; 
	MOTG_PHY_REG_WRITE32((uint32_t)0x68, u4Reg);
	u4Reg = MOTG_PHY_REG_READ32((uint32_t)0x68);
	u4Reg &=  ~0x00004000; 
	MOTG_PHY_REG_WRITE32((uint32_t)0x68, u4Reg);

	//otg bit setting
	 u4Reg = MOTG_PHY_REG_READ32((uint32_t)0x6C);
	 u4Reg &= ~0x3f3f;
	 u4Reg |=  0x3e2e;
	 MOTG_PHY_REG_WRITE32((uint32_t)0x6C, u4Reg);
	
	 //suspendom control
	 u4Reg = MOTG_PHY_REG_READ32((uint32_t)0x68);
	 u4Reg &=  ~0x00040000; 
	 MOTG_PHY_REG_WRITE32((uint32_t)0x68, u4Reg);
	
	 u4Reg = MOTG_PHY_REG_READ8(M_REG_BUSPERF1);
	 u4Reg |=  0x80;
	 u4Reg &= ~0x40;
	 MOTG_PHY_REG_WRITE8(M_REG_BUSPERF1, (uint8_t)u4Reg);

}


struct udc_descriptor {
	struct udc_descriptor *next;
	unsigned short tag;	/* ((TYPE << 8) | NUM) */
	unsigned short len;	/* total length */
	unsigned char data[0];
};

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

static int mt_read_fifo(struct udc_endpoint *endpoint) {

	struct urb *urb = endpoint->rcv_urb;
	int len = 0, count = 0;
	int ep_num = endpoint->num;
	int index;
	unsigned char *cp;
	u16 dma_cntl = 0;
	u32 u4fifoaddr;
	uint8_t bFifoOffset;
	uint32_t dwDatum = 0;
	int i;
	
	if (ep_num == EP0)
		urb = ep0_urb;

	if (urb) {
		index = MOTG_REG_READ8(INDEX);
		MOTG_REG_WRITE8(INDEX, ep_num);

		cp = (u8 *) (urb->buffer + urb->actual_length);

		count = len = MOTG_REG_READ16(IECSR + RXCOUNT);

#if DBG_USB_FIFO
		DBG_I("%s: ep_num: %d, urb: %x, urb->buffer: %x, urb->actual_length = %d, count = %d\n",
			__func__, ep_num, urb, urb->buffer, urb->actual_length, count);
#endif

		bFifoOffset = MGC_FIFO_OFFSET(ep_num);
		//	do not handle zero length data.
		if (len == 0)
		{
			return;
		}
		
		if (((uint32_t) cp) & 3)
		{
			/* byte access for unaligned */
			while (len > 0)
			{
				if (len < 4)
				{
				   if(3 == len || 2 == len)
				   {
					   MGC_FIFO_CNT(M_REG_FIFOBYTECNT, 1);
					   dwDatum = MOTG_REG_READ32(bFifoOffset);
					   
					   for (i = 0; i < 2; i++)
					   {
						   *cp++ = ((dwDatum >> (i *8)) & 0xFF);
					   }
					   
					   len -=2;
				  }
		
				  if(1 == len) 
				  {
					MGC_FIFO_CNT(M_REG_FIFOBYTECNT, 0);
					dwDatum = MOTG_REG_READ32(bFifoOffset);
					*cp++ = (dwDatum  & 0xFF);
					 len -= 1;
				  }				   
				   
				   // set FIFO byte count = 4 bytes.
				   MGC_FIFO_CNT(M_REG_FIFOBYTECNT, 2);
				   len = 0;
				}
				else
				{
					dwDatum = MOTG_REG_READ32(bFifoOffset);
					for (i = 0; i < 4; i++)
					{
						*cp++ = ((dwDatum >> (i * 8)) & 0xFF);
					}
		
					len -= 4;
				}
			}
		}
		else
		{
			/* word access if aligned */
			while (len >= 4)
			{
				*((uint32_t *) ((void *) cp)) =
					MOTG_REG_READ32(bFifoOffset);
				cp = (unsigned char *) cp;
				//DBG_I("[%d]: %x %x %x %x.\n", len, *cp, *(cp + 1), *(cp + 2), *(cp + 3));
				cp += 4;
				len -= 4;
			}
		
			if (len > 0)
			{
				if(3 == len ||2 == len )
				{
				   MGC_FIFO_CNT(M_REG_FIFOBYTECNT, 1);
				   dwDatum = MOTG_REG_READ32(bFifoOffset);
					for (i = 0; i < 2; i++)
					{
						*cp++ = ((dwDatum >> (i *8)) & 0xFF);
					}
					len -= 2;
				}
		
				if(1 == len)
				{
				   MGC_FIFO_CNT(M_REG_FIFOBYTECNT,0);
				   dwDatum = MOTG_REG_READ32(bFifoOffset);
				   
					*cp++ = (dwDatum & 0xFF);
					len -= 1;			   
				}
				
				// set FIFO byte count = 4 bytes.
				MGC_FIFO_CNT(M_REG_FIFOBYTECNT, 2);
			}
		}

#if DBG_USB_DUMP_DATA
		DBG_D("%s: &urb->buffer: %x\n", __func__, urb->buffer);
		DBG_D("[USB] dump data:\n");
		hexdump8(urb->buffer, count);
#endif

		urb->actual_length += count;

		MOTG_REG_WRITE8(INDEX, index);
	}

	return count;
}

static int mt_write_fifo(struct udc_endpoint *endpoint)
{
	struct urb *urb = endpoint->tx_urb;
	int last = 0, count = 0;
	int ep_num = endpoint->num;
	int index;
	unsigned char *cp = NULL;
    u32 dwDatum = 0;
    u8 bFifoOffset = MGC_FIFO_OFFSET(ep_num);
    u32 dwBufSize = 4;
	

	if (ep_num == EP0)
		urb = ep0_urb;

	if (urb) {
		index = MOTG_REG_READ8(INDEX);
		MOTG_REG_WRITE8(INDEX, ep_num);

#if DBG_USB_DUMP_DESC
	DBG_D("%s: dump desc\n", __func__);
	hexdump8(urb->buffer, urb->actual_length);
#endif


#if DBG_USB_FIFO
	DBG_I("%s: ep_num: %d urb: %x, actual_length: %d\n",
		__func__, ep_num, urb, urb->actual_length);
	DBG_I("%s: sent: %d, tx_pkt_size: %d\n", __func__, endpoint->sent, endpoint->maxpkt);
#endif

		count = last = MIN (urb->actual_length - endpoint->sent,  endpoint->maxpkt);

#if DBG_USB_FIFO
		DBG_I("%s: count: %d\n", __func__, count);
		DBG_I("%s: urb->actual_length = %d\n", __func__, urb->actual_length);
		DBG_I("%s: endpoint->sent = %d, cp addr=0x%x\n", __func__, endpoint->sent, cp);
#endif

		if (count < 0) {
			DBG_C("%s: something is wrong, count < 0", __func__);
		}

		if(count){
			cp = urb->buffer + endpoint->sent;
			
	    if ((uint32_t) cp & 3)
	    {
	        while (count)
	        {
	     		if (3 == count || 2 == count)
	     		{
	     			dwBufSize = 2;
	     		
	     			// set FIFO byte count.
	     			MGC_FIFO_CNT(M_REG_FIFOBYTECNT, 1);
	     		}
				else if(1 == count)
				{
	     			dwBufSize = 1;
	     		
	     			// set FIFO byte count.
	     			MGC_FIFO_CNT(M_REG_FIFOBYTECNT, 0);			   
				}
	     		
	     		memcpy((void *)&dwDatum, (const void *)cp, dwBufSize);
	     		
	     		MOTG_REG_WRITE32(bFifoOffset, dwDatum);
	     		
	     		count -= dwBufSize;
	     		cp += dwBufSize;
	        }
	    }
	else                        /* word access if aligned */
	{
		while ((count > 3) && !((uint32_t) cp & 3))
		{
			MOTG_REG_WRITE32(bFifoOffset,
			*((uint32_t *) ((void *) cp)));
			#if DBG_USB_DUMP_DATA
			if(count <= 8)
			{
				DBG_D("%s  count=%d data = 0x%x, 0x%x, 0x%x, 0x%x.\n", __func__,count, *cp, *(cp + 1), *(cp + 2), *(cp + 3));
				DBG_D("data = 0x%x.\n", *((uint32_t *) ((void *) cp)));
			}
			#endif
			cp += 4;
			count -= 4;
		}
		if (3 == count || 2 == count)
		{
			// set FIFO byte count.
			MGC_FIFO_CNT(M_REG_FIFOBYTECNT, 1);
			#if DBG_USB_DUMP_DATA
			DBG_D("%s  count=%d data = 0x%x, 0x%x.\n", __func__, count, *cp, *(cp + 1));
			#endif
			MOTG_REG_WRITE32(bFifoOffset, *((uint32_t *)((void *)cp)));
			count -= 2;
			cp += 2;
		}

		if(1 == count)
		{
			MGC_FIFO_CNT(M_REG_FIFOBYTECNT, 0);
			#if DBG_USB_DUMP_DATA
			DBG_D("%s  count=%d data = 0x%x.\n", __func__, count, *cp);
			#endif
			if((uint32_t)cp & 3)
			{
				memcpy((void *)&dwDatum, (const void *)cp, 1);

				MOTG_REG_WRITE32(bFifoOffset, dwDatum);		    
			}
			else
			{
				MOTG_REG_WRITE32(bFifoOffset, *((uint32_t *)((void *)cp)));		    
			}
		}
	}
	MGC_FIFO_CNT(M_REG_FIFOBYTECNT, 2);

	}

		endpoint->last = last;
		endpoint->sent += last;

		MOTG_REG_WRITE8(INDEX, index);
	}

	return last;
}

static struct udc_endpoint * mt_find_ep(int ep_num, u8 dir) {
	int i;
	u8 in = 0;

	/* convert dir to in */
	if (dir == UDC_DIR_IN) /* dir == USB_DIR_IN */
		in = 1;

	/* for (i = 0; i < udc_device->max_endpoints; i++) */
	/* for (i = 0; i < the_gadget->ifc_endpoints; i++) */
	for (i = 0; i < MT_EP_NUM; i++) {
		if ((ep_list[i].num == ep_num) && (ep_list[i].in == in)) {
			DBG_I("%s: find ep!\n", __func__);
			return &ep_list[i];
		}
	}
	return NULL;
}

static void mt_udc_flush_fifo(u8 ep_num, u8 dir)
{
	u16 tmpReg16;
	u8 index;
	struct udc_endpoint *endpoint;

	index = MOTG_REG_READ8(INDEX);
	MOTG_REG_WRITE8(INDEX, ep_num);

	if (ep_num == 0) {
		tmpReg16 = MOTG_REG_READ16(IECSR + CSR0);
		tmpReg16 |= EP0_FLUSH_FIFO;
		MOTG_REG_WRITE16(IECSR + CSR0,tmpReg16);
		MOTG_REG_WRITE16(IECSR + CSR0, tmpReg16);
	} else {
		endpoint = mt_find_ep(ep_num, dir);
		if (endpoint->in == 0) { /* UDC_DIR_OUT */
			tmpReg16 = MOTG_REG_READ16(IECSR + RXCSR);
			tmpReg16 |= EPX_RX_FLUSHFIFO;
			MOTG_REG_WRITE16(IECSR + RXCSR, tmpReg16);
			MOTG_REG_WRITE16(IECSR + RXCSR, tmpReg16);
		} else {
			tmpReg16 = MOTG_REG_READ16(IECSR + TXCSR);
			tmpReg16 |= EPX_TX_FLUSHFIFO;
			MOTG_REG_WRITE16(IECSR + TXCSR, tmpReg16);
			MOTG_REG_WRITE16(IECSR + TXCSR, tmpReg16);
		}
	}

	/* recover index register */
	MOTG_REG_WRITE8(INDEX, index);
}

/* the endpoint does not support the received command, stall it!! */
static void udc_stall_ep(unsigned int ep_num, u8 dir) {
	struct udc_endpoint *endpoint = mt_find_ep(ep_num, dir);
	u8 index;
	u16 csr;

	DBG_C("[USB] %s\n", __func__);

	index = MOTG_REG_READ8(INDEX);
	MOTG_REG_WRITE8(INDEX, ep_num);

	if (ep_num == 0) {
		csr = MOTG_REG_READ16(IECSR + CSR0);
		csr |= EP0_SENDSTALL;
		MOTG_REG_WRITE16(IECSR + CSR0, csr);
		mt_udc_flush_fifo(UDC_DIR_OUT, ep_num);
	} else {
		if (endpoint->in == 0) { /* UDC_DIR_OUT */
			csr = MOTG_REG_READ8(IECSR + RXCSR);
			csr |= EPX_RX_SENDSTALL;
			MOTG_REG_WRITE16(IECSR + RXCSR, csr);
			mt_udc_flush_fifo(UDC_DIR_OUT, ep_num);
		} else {
			csr = MOTG_REG_READ8(IECSR + TXCSR);
			csr |= EPX_TX_SENDSTALL;
			MOTG_REG_WRITE16(IECSR + TXCSR, csr);
			mt_udc_flush_fifo(ep_num, UDC_DIR_IN);
		}
	}

	ep0_state = EP0_IDLE;

	MOTG_REG_WRITE8(INDEX, index);

	return;
}

/*
 * If abnormal DATA transfer happened, like USB unplugged,
 * we cannot fix this after mt_udc_reset().
 * Because sometimes there will come reset twice.
 */
static void mt_udc_suspend(void) {
	/* handle abnormal DATA transfer if we had any */
	struct udc_endpoint *endpoint;
	int i;

	/* deal with flags */
	usb_online = 0;
	usb_config_value = 0;
	the_gadget->notify(the_gadget, UDC_EVENT_OFFLINE);

	/* error out any pending reqs */
	for (i = 1; i < MT_EP_NUM; i++) {
		/* ensure that ept_complete considers
		 * this to be an error state
		 */
		DBG_I("%s: ep: %i, in: %s, req: %x\n",
			__func__, ep_list[i].num, ep_list[i].in ? "IN" : "OUT", ep_list[i].req);
		if ((ep_list[i].req && (ep_list[i].in == 0)) || /* UDC_DIR_OUT */
			(ep_list[i].req && (ep_list[i].in == 1))) { /* UDC_DIR_IN */
			ep_list[i].status = -1;	/* HALT */
			endpoint = &ep_list[i];
			handle_ept_complete(endpoint);
		}
	}
}

static void mt_udc_rxtxmap_recover(void) {
	int i;

	for (i = 1; i < MT_EP_NUM; i++) {
		if (ep_list[i].num != 0) { /* allocated */

			MOTG_REG_WRITE8(INDEX, ep_list[i].num);

			if (ep_list[i].in == 0) /* UDC_DIR_OUT */
				MOTG_REG_WRITE32((RXMAP), ep_list[i].maxpkt);
			else
				MOTG_REG_WRITE32((TXMAP), ep_list[i].maxpkt);
		}
	}
}

static void mt_udc_reset(void) {

	/* MUSBHDRC automatically does the following when reset signal is detected */
	/* 1. Sets FAddr to 0
	 * 2. Sets Index to 0
	 * 3. Flush all endpoint FIFOs
	 * 4. Clears all control/status registers
	 * 5. Enables all endpoint interrupts
	 * 6. Generates a Rest interrupt
	 */
    uint32_t u4Reg = 0;

	dev_address = 0;

	/* flush FIFO */
	mt_udc_flush_fifo(0, UDC_DIR_OUT);
	mt_udc_flush_fifo(1, UDC_DIR_OUT);
	mt_udc_flush_fifo(1, UDC_DIR_IN);
	//mt_udc_flush_fifo (2, UDC_DIR_IN);

	/* detect USB speed */
	if (MOTG_REG_READ8(POWER) & PWR_HS_MODE) {
		DBG_I("[USB] USB High Speed\n");
//		enable_highspeed();
	} else {
		DBG_I("[USB] USB Full Speed\n");
	}

	/* restore RXMAP and TXMAP if the endpoint has been configured */
	mt_udc_rxtxmap_recover();

	/* enable suspend */
	MOTG_REG_WRITE8(INTRUSBE, (INTRUSB_SUSPEND | INTRUSB_RESUME | INTRUSB_RESET |INTRUSB_DISCON));

}

static void mt_udc_ep0_write(void) {

	struct udc_endpoint *endpoint = &ep_list[EP0];
	int count = 0;
	u16 csr0 = 0;
	u8 index = 0;

	index = MOTG_REG_READ8(INDEX);
	MOTG_REG_WRITE8(INDEX, 0);

	csr0 = MOTG_REG_READ16(IECSR + CSR0);
	if (csr0 & EP0_TXPKTRDY) {
		DBG_I("mt_udc_ep0_write: ep0 is not ready to be written\n");
		return;
	}

	count = mt_write_fifo(endpoint);

	DBG_I("%s: count = %d\n", __func__, count);

	if (count < EP0_MAX_PACKET_SIZE) {
		/* last packet */
		csr0 |= (EP0_TXPKTRDY | EP0_DATAEND);
		ep0_urb->actual_length = 0;
		endpoint->sent = 0;
		ep0_state = EP0_IDLE;
	} else {
		/* more packets are waiting to be transferred */
		csr0 |= EP0_TXPKTRDY;
	}

	MOTG_REG_WRITE16(IECSR + CSR0, csr0);
	MOTG_REG_WRITE8(INDEX, index);

	return;
}

static void mt_udc_ep0_read(void) {

	struct udc_endpoint *endpoint = &ep_list[EP0];
	int count = 0;
	u16 csr0 = 0;
	u8 index = 0;

	index = MOTG_REG_READ8(INDEX);
	MOTG_REG_WRITE8(INDEX, EP0);

	csr0 = MOTG_REG_READ16(IECSR + CSR0);

	/* erroneous ep0 interrupt */
	if (!(csr0 & EP0_RXPKTRDY)) {
		return;
	}

	count = mt_read_fifo(endpoint);

	if (count <= EP0_MAX_PACKET_SIZE) {
		/* last packet */
		csr0 |= (EP0_SERVICED_RXPKTRDY | EP0_DATAEND);
		ep0_state = EP0_IDLE;
	} else {
		/* more packets are waiting to be transferred */
		csr0 |= EP0_SERVICED_RXPKTRDY;
	}

	MOTG_REG_WRITE16(IECSR + CSR0, csr0);

	MOTG_REG_WRITE8(INDEX, index);

	return;
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
			DBG_I("GET_DESCRIPTOR\n");
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

					DBG_I("%s: urb: %x, cp: %p\n", __func__, urb, cp);
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
					mt_setup_ep((unsigned int)ep_num, endpoint);
					return 0;
				}

		default:
			DBG_C("Unsupported command with RX data stage\n");
			break;

		}
	}
	return FALSE;
}

static void mt_udc_ep0_setup(void) {
	struct udc_endpoint *endpoint = &ep_list[0];
	u8 index;
	u8 stall = 0;
	u16 csr0;
	struct setup_packet *request;
	u16 count;

	DBG_I("mt_udc_ep0_setup begin.\n");
	index = MOTG_REG_READ8(INDEX);
	MOTG_REG_WRITE8(INDEX, 0);
	/* Read control status register for endpiont 0 */
	csr0 = MOTG_REG_READ16(IECSR + CSR0);

	/* check whether RxPktRdy is set? */
	if (!(csr0 & EP0_RXPKTRDY))
		return;

	/* unload fifo */
	ep0_urb->actual_length = 0;

	count = mt_read_fifo(endpoint);

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
		udc_stall_ep(0, UDC_DIR_OUT);
		return;
	}

	switch (ep0_state) {
	case EP0_TX:
		/* data stage: from device to host */
		DBG_I("%s: EP0_TX\n", __func__);
		csr0 = MOTG_REG_READ16(IECSR + CSR0);
		csr0 |= (EP0_SERVICED_RXPKTRDY);
		MOTG_REG_WRITE16(IECSR + CSR0, csr0);

		mt_udc_ep0_write();

		break;
	case EP0_RX:
		/* data stage: from host to device */
		DBG_I("%s: EP0_RX\n", __func__);
		csr0 = MOTG_REG_READ16(IECSR + CSR0);
		csr0 |= (EP0_SERVICED_RXPKTRDY);
		MOTG_REG_WRITE16(IECSR + CSR0, csr0);

		break;
	case EP0_IDLE:
		/* no data stage */
		DBG_I("%s: EP0_IDLE\n", __func__);
		csr0 = MOTG_REG_READ16(IECSR + CSR0);
		csr0 |= (EP0_SERVICED_RXPKTRDY | EP0_DATAEND);

		MOTG_REG_WRITE16(IECSR + CSR0, csr0);
		MOTG_REG_WRITE16(IECSR + CSR0, csr0);

		break;
	default:
		break;
	}

	MOTG_REG_WRITE8(INDEX, index);
	return;

}

static void mt_udc_ep0_handler(void) {

	u16 csr0;
	u8 index = 0;

	index = MOTG_REG_READ8(INDEX);
	MOTG_REG_WRITE8(INDEX, 0);

	csr0 = MOTG_REG_READ16(IECSR + CSR0);
	DBG_I("mt_udc_ep0_handler begin csr0=0x%x.\n", csr0);
	if (csr0 & EP0_SENTSTALL) {
		DBG_I("USB: [EP0] SENTSTALL\n");
		/* needs implementation for exception handling here */
		ep0_state = EP0_IDLE;
	}

	if (csr0 & EP0_SETUPEND) {
		DBG_I("USB: [EP0] SETUPEND\n");
		csr0 |= EP0_SERVICE_SETUP_END;
		MOTG_REG_WRITE16(IECSR + CSR0, csr0);

		ep0_state = EP0_IDLE;
	}

	switch (ep0_state) {
	case EP0_IDLE:
		DBG_I("%s: EP0_IDLE\n", __func__);
		if (set_address) {
			MOTG_REG_WRITE8(FADDR, dev_address);
			set_address = 0;
		}
		mt_udc_ep0_setup();
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

	MOTG_REG_WRITE8(INDEX,index);

	return;
}


/*
 * udc_setup_ep - setup endpoint
 *
 * Associate a physical endpoint with endpoint_instance and initialize FIFO
 */
void mt_setup_ep(unsigned int ep, struct udc_endpoint *endpoint) {
	u8 index;
	u16 csr;
	u16 csr0;
	u16 max_packet_size;
	u8 fifosz = 0;

	/* EP table records in bits hence bit 1 is ep0 */
	index = MOTG_REG_READ8(INDEX);
	MOTG_REG_WRITE8(INDEX, ep);

	if (ep == EP0) {
		/* Read control status register for endpiont 0 */
		csr0 = MOTG_REG_READ16(IECSR + CSR0);

		/* check whether RxPktRdy is set? */
		if (!(csr0 & EP0_RXPKTRDY))
			return;
	}

	if(ep == EP0)
		fifosz=3; //fifo size 64
	else
		fifosz=6; //fifo size 512

	/*
	 *to check fifo boundary :
	 *total HW fifo is 5k Bytes,so the FIFO start address should be <= 5K -512=4608
	 */
	if(fifo_addr > 4608) {
		printf("DPB=%d ,EP=%d , fifo_addr=%d \n",endpoint->mode,ep,fifo_addr);
		fifo_addr=FIFO_ADDR_START;
	}

	/* Configure endpoint fifo */
	/* Set fifo address, fifo size, and fifo max packet size */
	DBG_I("%s: endpoint->in: %d, maxpkt: %d\n",
		__func__, endpoint->in, endpoint->maxpkt);
	if (endpoint->in == 0) { /* UDC_DIR_OUT */
		/* Clear data toggle to 0 */
		csr = MOTG_REG_READ16(IECSR + RXCSR);
		/* pangyen 20090911 */
		csr |= EPX_RX_CLRDATATOG | EPX_RX_FLUSHFIFO;
		MOTG_REG_WRITE16(IECSR + RXCSR, csr);
		/* Set fifo address */
		MOTG_REG_WRITE16(RXFIFOADD, fifo_addr >> 3);
		/* Set fifo max packet size */
		max_packet_size = endpoint->maxpkt;
		MOTG_REG_WRITE16(IECSR + RXMAP, max_packet_size);
		/* Set fifo size (double buffering is currently not enabled) */
		switch (max_packet_size) {
			case 8:
			case 16:
			case 32:
			case 64:
			case 128:
			case 256:
			case 512:
			case 1024:
			case 2048:
				if (endpoint->mode == DOUBLE_BUF)
					fifosz |= FIFOSZ_DPB;
				MOTG_REG_WRITE8(RXFIFOSZ, fifosz);
			case 4096:
				fifosz |= uffs(max_packet_size >> 4);
				MOTG_REG_WRITE8(RXFIFOSZ, fifosz);
				break;
			case 3072:
				fifosz = uffs(4096 >> 4);
				MOTG_REG_WRITE8(RXFIFOSZ, fifosz);
				break;

			default:
				DBG_C("The max_packet_size for ep %d is not supported\n", ep);
		}
	} else {
		/* Clear data toggle to 0 */
		csr = MOTG_REG_READ16(IECSR + TXCSR);
		/* pangyen 20090911 */
		csr |= EPX_TX_CLRDATATOG | EPX_TX_FLUSHFIFO;
		MOTG_REG_WRITE16(IECSR + TXCSR, csr);
		/* Set fifo address */
		MOTG_REG_WRITE16(TXFIFOADD, fifo_addr >> 3);
		/* Set fifo max packet size */
		max_packet_size = endpoint->maxpkt;
		MOTG_REG_WRITE16(IECSR + TXMAP, max_packet_size);
		/* Set fifo size(double buffering is currently not enabled) */
		switch (max_packet_size) {
			case 8:
			case 16:
			case 32:
			case 64:
			case 128:
			case 256:
			case 512:
			case 1024:
			case 2048:
				if (endpoint->mode == DOUBLE_BUF)
					fifosz |= FIFOSZ_DPB;
				MOTG_REG_WRITE8(TXFIFOSZ, fifosz);
			case 4096:
				fifosz |= uffs(max_packet_size >> 4);
				MOTG_REG_WRITE8(TXFIFOSZ, fifosz);
				break;
			case 3072:
				fifosz = uffs(4096 >> 4);
				MOTG_REG_WRITE8(TXFIFOSZ, fifosz);
				break;

			default:
				DBG_C("The max_packet_size for ep %d is not supported\n", ep);
		}
	}

	if (endpoint->mode == DOUBLE_BUF)
		fifo_addr += (max_packet_size << 1);
	else
		fifo_addr += max_packet_size;

	/* recover INDEX register */
	MOTG_REG_WRITE8(INDEX, index);
}

struct udc_endpoint *_udc_endpoint_alloc(unsigned char num, unsigned char in,
					 unsigned short max_pkt) {
	int i;

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
	} else {
		ep_list[i].bit = EPT_RX(num);
	}

	/* write parameters to this ep (write to hardware) */
	mt_setup_ep(num, &ep_list[i]);

	DBG_I("[USB] ept%d %s @%p/%p max=%d bit=%x\n",
		num, in ? "in" : "out", &ep_list[i], &ep_list, max_pkt, ep_list[i].bit);

	return &ep_list[i];
}

#define SETUP(type,request) (((type) << 8) | (request))

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
		ept = _udc_endpoint_alloc(n, in, maxpkt);
		if (ept)
			ept_alloc_table |= bit;
		return ept;
	}

	return 0;
}

static void handle_ept_complete(struct udc_endpoint *ept)
{
	unsigned int actual;
	int status;
	struct udc_request *req;

	req = ept->req;
	if (req) {
		DBG_I("%s: req: %x: req->length: %d: ept->actual_length: %d status: %d\n", __func__, req, req->length, ept->rcv_urb->actual_length, ept->status);
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

static void mt_udc_epx_handler(u8 ep_num, u8 dir)
{
	u8 index;
	u16 csr;
	u32 count;
	struct udc_endpoint *endpoint;
	struct urb *urb;
	struct udc_request *req;	/* for event signaling */
	u8 intrrxe;

	endpoint = mt_find_ep(ep_num, dir);

	index = MOTG_REG_READ8(INDEX);
	MOTG_REG_WRITE8(INDEX, ep_num);

	DBG_I("EP%d Interrupt\n", ep_num);
	DBG_I("dir: %x\n", dir);

	switch (dir) {
	case UDC_DIR_OUT:
		/* transfer direction is from host to device */
		/* from the view of usb device, it's RX */
		csr = MOTG_REG_READ16(IECSR + RXCSR);

		if (csr & EPX_RX_SENTSTALL) {
			DBG_C("EP %d(RX): STALL\n", ep_num);
			/* exception handling: implement this!! */
			return;
		}

		if (!(csr & EPX_RX_RXPKTRDY)) {
			DBG_I("EP %d: ERRONEOUS INTERRUPT\n", ep_num); // normal
			return;
		}

		count = mt_read_fifo(endpoint);

		DBG_I("EP%d(RX), count = %d\n", ep_num, count);

		csr &= ~EPX_RX_RXPKTRDY;
		MOTG_REG_WRITE16(IECSR + RXCSR, csr);
		if (MOTG_REG_READ16(IECSR + RXCSR) & EPX_RX_RXPKTRDY) {
			DBG_I("%s: rxpktrdy clear failed\n", __func__);
		}

		/* do signaling */
		req = endpoint->req;
		/* workaround: if req->lenth == 64 bytes (not huge data transmission)
		 * do normal return */
		DBG_I("%s: req->length: %x, endpoint->rcv_urb->actual_length: %x\n",
			__func__, req->length, endpoint->rcv_urb->actual_length);

		/* Deal with FASTBOOT command */
		if ((req->length >= endpoint->rcv_urb->actual_length) && req->length == 64) {
			req->length = count;

			/* mask EPx INTRRXE */
			/* The buffer is passed from the AP caller.
			 * It happens that AP is dealing with the buffer filled data by driver,
			 * but the driver is still receiving the next data packets onto the buffer.
			 * Data corrupted happens if the every request use the same buffer.
			 * Mask the EPx to ensure that AP and driver are not accessing the buffer parallely.
			 */
			intrrxe = MOTG_REG_READ8(INTRRXE);
			MOTG_REG_WRITE8(INTRRXE, (intrrxe &= ~(1 << ep_num)));
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
			intrrxe = MOTG_REG_READ8(INTRRXE);
			MOTG_REG_WRITE8(INTRRXE, (intrrxe &= ~(1 << ep_num)));
		}
		break;
	case UDC_DIR_IN:
		/* transfer direction is from device to host */
		/* from the view of usb device, it's tx */
		csr = MOTG_REG_READ16(IECSR + TXCSR);

		if (csr & EPX_TX_SENTSTALL) {
			DBG_C("EP %d(TX): STALL\n", ep_num);
			endpoint->status = -1;
			handle_ept_complete(endpoint);
			/* exception handling: implement this!! */
			return;
		}

		if (csr & EPX_TX_TXPKTRDY) {
			DBG_C
				("mt_udc_epx_handler: ep%d is not ready to be written\n",
				ep_num);
			return;
		}

		urb = endpoint->tx_urb;
		if (endpoint->sent == urb->actual_length) {
			/* do signaling */
			handle_ept_complete(endpoint);
			break;
		}

		/* send next packet of the same urb */
		count = mt_write_fifo(endpoint);
		DBG_I("EP%d(TX), count = %d\n", ep_num, endpoint->sent);

		if (count != 0) {
			/* not the interrupt generated by the last tx packet of the transfer */
			csr |= EPX_TX_TXPKTRDY;
			MOTG_REG_WRITE16(IECSR + TXCSR, csr);
		}

		break;
	default:
		break;
	}

	MOTG_REG_WRITE8(INDEX,index);

	return;
}


void mt_udc_irq(u8 intrtx, u8 intrrx, u8 intrusb) {

	int i;

	DBG_IRQ("[USB] INTERRUPT, tx=x%x, rx=x%x, usb=x%x.\n", intrtx, intrrx, intrusb);

	if (intrusb) {
		if (intrusb & INTRUSB_RESUME) {
			DBG_IRQ("[USB] INTRUSB: RESUME\n");
		}

		if (intrusb & INTRUSB_SESS_REQ) {
			DBG_IRQ("[USB] INTRUSB: SESSION REQUEST\n");
		}

		if (intrusb & INTRUSB_VBUS_ERROR) {
			DBG_IRQ("[USB] INTRUSB: VBUS ERROR\n");
		}

		if (intrusb & INTRUSB_SUSPEND) {
			DBG_IRQ("[USB] INTRUSB: SUSPEND\n");
			mt_udc_suspend();
		}

		if (intrusb & INTRUSB_CONN) {
			DBG_IRQ("[USB] INTRUSB: CONNECT\n");
		}

		if (intrusb & INTRUSB_DISCON) {
			DBG_IRQ("[USB] INTRUSB: DISCONNECT\n");
		}

		if (intrusb & INTRUSB_RESET) {
			DBG_IRQ("[USB] INTRUSB: RESET\n");
			mt_udc_reset();
		}

		if (intrusb & INTRUSB_SOF) {
			DBG_IRQ("[USB] INTRUSB: SOF\n");
		}
	}

	/* endpoint 0 interrupt? */
	if (intrtx & EPMASK (0)) {
		mt_udc_ep0_handler();
		intrtx &= ~0x1;
	}

	if (intrtx) {
		for (i = 1; i < MT_EP_NUM; i++) {
			if (intrtx & EPMASK (i)) {
				mt_udc_epx_handler(i, UDC_DIR_IN);
			}
		}
	}

	if (intrrx) {
		for (i = 1; i < MT_EP_NUM; i++) {
			if (intrrx & EPMASK (i)) {
				mt_udc_epx_handler(i, UDC_DIR_OUT);
			}
		}
	}

}

void service_interrupts(void)
{
	volatile u8 intrtx, intrrx, intrusb;
	volatile u8 intrtxen, intrrxen, intrusben;

	intrtxen = MOTG_REG_READ8(INTRTXE);
	intrrxen = MOTG_REG_READ8(INTRRXE);
	intrusben = MOTG_REG_READ8(INTRUSBE);
	/* polling interrupt status for incoming interrupts and service it */
	intrtx = MOTG_REG_READ8(M_REG_INTRTX);
	intrrx = MOTG_REG_READ8(INTRRX);
	intrusb = MOTG_REG_READ8(INTRUSB);

	//printf("intrtx = 0x%x, intrrx = 0x%x, intrusb = 0x%x.\n", intrtx, intrrx, intrusb);
	//printf("intrtxen = 0x%x, intrrxen = 0x%x, intrusben = 0x%x.\n", intrtxen, intrrxen, intrusben);
	//printf("intrtx = 0x%x, intrrx = 0x%x, intrusb = 0x%x.\n", intrtx, intrrx, intrusb);
	intrtx &= intrtxen;
	intrrx &= intrrxen;
	intrusb &= intrusben;
	
	MOTG_REG_WRITE8(M_REG_INTRTX, intrtx);
	MOTG_REG_WRITE8(INTRRX, intrrx);
	MOTG_REG_WRITE8(INTRUSB, intrusb);

	intrusb &= ~INTRUSB_SOF;

	if (intrtx | intrrx | intrusb) {
		mt_udc_irq(intrtx, intrrx, intrusb);
	}

}

int mt_usb_irq_init(void) {
	/* disable all endpoint interrupts */
	MOTG_REG_WRITE8(INTRTXE, 0);
	MOTG_REG_WRITE8(INTRRXE, 0);
	MOTG_REG_WRITE8(INTRUSBE, 0);

	return 0;
}

/* Turn on the USB connection by enabling the pullup resistor */
void mt_usb_connect_internal(void)
{
	u8 tmpReg8;
	u8 nDevCtl;

	/* connect */
	tmpReg8 = MOTG_REG_READ8(POWER);
	tmpReg8 |= PWR_SOFT_CONN;
	tmpReg8 |= PWR_ENABLE_SUSPENDM;

#ifdef USB_FORCE_FULL_SPEED
	tmpReg8 &= ~PWR_HS_ENAB;
#else
	tmpReg8 |= PWR_HS_ENAB;
#endif
	MOTG_REG_WRITE8(POWER, tmpReg8);
	
	// open session.
	nDevCtl = MOTG_REG_READ8(MGC_O_HDRC_DEVCTL);	 
	nDevCtl |= MGC_M_DEVCTL_SESSION;		
	MOTG_REG_WRITE8(MGC_O_HDRC_DEVCTL, nDevCtl);

}

/* Turn off the USB connection by disabling the pullup resistor */
void mt_usb_disconnect_internal(void)
{
	u8 tmpReg8;
	u8 nDevCtl;
	
	/* connect */
	tmpReg8 = MOTG_REG_READ8(POWER);
	tmpReg8 &= ~PWR_SOFT_CONN;
	MOTG_REG_WRITE8(POWER, tmpReg8);

	// open session.
	nDevCtl = MOTG_REG_READ8(MGC_O_HDRC_DEVCTL);	 
	nDevCtl &= ~MGC_M_DEVCTL_SESSION;		
	MOTG_REG_WRITE8(MGC_O_HDRC_DEVCTL, nDevCtl);
}

int udc_init(struct udc_device *dev)
{
	struct udc_descriptor *desc = NULL;
	u32 usb_l1intm;

	DBG_I("[USB] %s: begin.\n", __func__);

	mdelay(20);
	/* usb phy init */
	board_usb_init();

	mt_usb_connect_internal();


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

	MOTG_REG_WRITE32(USB_L1INTM, 0x2f);
	
	the_device = dev;
	return 0;
}

void udc_endpoint_free(struct udc_endpoint *ept)
{
	/* todo */
}

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

/* Called to start packet transmission. */
/* It must be applied in udc_request_queue when polling mode is used.
 * If interrupt mode is used, you can use
 * mt_udc_epx_handler(ept->num, UDC_DIR_IN); to replace mt_ep_write make ISR
 * do it for you.
 */
static int mt_ep_write(struct udc_endpoint *endpoint)
{
	int ep_num = endpoint->num;
	int count;
	u8 index;
	u16 csr;

	index = MOTG_REG_READ8(INDEX);
	MOTG_REG_WRITE8(INDEX, ep_num);

	/* udc_endpoint_write: cannot write ep0 */
	if (ep_num == 0)
		return FALSE;

	 /* udc_endpoint_write: cannot write UDC_DIR_OUT */
	if (endpoint->in == 0)
		return FALSE;

	csr = MOTG_REG_READ16(IECSR + TXCSR);
	if (csr & EPX_TX_TXPKTRDY) {
		DBG_I("[USB]: udc_endpoint_write: ep%d is not ready to be written\n",
			 ep_num);

		return FALSE;
	}
	count = mt_write_fifo(endpoint);

	csr |= EPX_TX_TXPKTRDY;
	MOTG_REG_WRITE16(IECSR + TXCSR, csr);

	MOTG_REG_WRITE8(INDEX, index);

	return count;
}

int udc_request_queue(struct udc_endpoint *ept, struct udc_request *req) {
	u8 intrrxe;

	DBG_I("[USB] %s: ept%d %s queue req=%p, req->length=%x\n",
		__func__, ept->num, ept->in ? "in" : "out", req, req->length);
	DBG_I("[USB] %s: ept%d: %x, ept->in: %s, ept->rcv_urb->buffer: %x, req->buf: %x\n",
		__func__, ept->num, ept, ept->in ? "IN" : "OUT" , ept->rcv_urb->buffer, req->buf);

	ept->req = req;
	ept->status = 0;	/* ACTIVE */

	ept->sent = 0;
	ept->last = 0;

	/* read */
	if (!ept->in) {
		ept->rcv_urb->buffer = req->buf;
		ept->rcv_urb->actual_length = 0;

		/* unmask EPx INTRRXE */
		/*
		 * To avoid the parallely access the buffer,
		 * it is umasked here and umask at complete.
		 */
		intrrxe = MOTG_REG_READ8(INTRRXE);
		intrrxe |= (1 << ept->num);
		MOTG_REG_WRITE8(INTRRXE, intrrxe);
	}

	/* write */
	if (ept->in) {
		ept->tx_urb->buffer = req->buf;
		ept->tx_urb->actual_length = req->length;

		mt_ep_write(ept);
	}
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
	MOTG_REG_WRITE8(INTRTXE, 0xff);
	MOTG_REG_WRITE8(INTRRXE, 0xff);
	MOTG_REG_WRITE8(INTRUSBE, 0xf7);

	return 0;
}

int udc_stop(void)
{
	mt_usb_disconnect_internal();

	return 0;
}
