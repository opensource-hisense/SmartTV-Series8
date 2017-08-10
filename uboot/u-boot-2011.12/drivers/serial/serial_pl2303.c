/*
 * vm_linux/chiling/uboot/u-boot-2011.12/drivers/serial/serial_pl2303.c
 *
 * Copyright (c) 2010-2012 MediaTek Inc.
 *	This file is based  ARM Realview platform
 *  Copyright (C) 2002 ARM Ltd.
 *  All Rights Reserved
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

#include <common.h>

#ifdef CONFIG_USB_PL2303

#include "serial_mt53xx.h"
#include "serial_pl2303.h"
#include "x_typedef.h"
#include <usb.h>
#include <usb_defs.h>

/* Integrator AP has two UARTs, we use the first one, at 38400-8-N-1 */
#define baudRate CONFIG_BAUDRATE

#define SET_LINE_REQUEST_TYPE		0x21
#define SET_LINE_REQUEST		0x20

#define SET_CONTROL_REQUEST_TYPE	0x21
#define SET_CONTROL_REQUEST		0x22
#define CONTROL_DTR			0x01
#define CONTROL_RTS			0x02

#define BREAK_REQUEST_TYPE		0x21
#define BREAK_REQUEST			0x23
#define BREAK_ON			0xffff
#define BREAK_OFF			0x0000

#define GET_LINE_REQUEST_TYPE		0xa1
#define GET_LINE_REQUEST		0x21

#define VENDOR_WRITE_REQUEST_TYPE	0x40
#define VENDOR_WRITE_REQUEST		0x01

#define VENDOR_READ_REQUEST_TYPE	0xc0
#define VENDOR_READ_REQUEST		0x01

#define UART_STATE			0x08
#define UART_STATE_TRANSIENT_MASK	0x74
#define UART_DCD			0x01
#define UART_DSR			0x02
#define UART_BREAK_ERROR		0x04
#define UART_RING			0x08
#define UART_FRAME_ERROR		0x10
#define UART_PARITY_ERROR		0x20
#define UART_OVERRUN_ERROR		0x40
#define UART_CTS			0x80

#define RX_BUFFER_SIZE		64

enum pl2303_type {
	type_0,		/* don't know the difference between type 0 and */
	type_1,		/* type 1, until someone from prolific tells us... */
	HX,		/* HX version of the pl2303 chip */
};

static const struct usb_device_id pl2303_id_table[] = {
	{ USB_DEVICE(PL2303_VENDOR_ID, PL2303_PRODUCT_ID) },
	{ USB_DEVICE(PL2303_VENDOR_ID, PL2303_PRODUCT_ID_RSAQ2) },
	{ USB_DEVICE(PL2303_VENDOR_ID, PL2303_PRODUCT_ID_DCU11) },
	{ USB_DEVICE(PL2303_VENDOR_ID, PL2303_PRODUCT_ID_RSAQ3) },
	{ USB_DEVICE(PL2303_VENDOR_ID, PL2303_PRODUCT_ID_PHAROS) },
	{ USB_DEVICE(PL2303_VENDOR_ID, PL2303_PRODUCT_ID_ALDIGA) },
	{ USB_DEVICE(PL2303_VENDOR_ID, PL2303_PRODUCT_ID_MMX) },
	{ USB_DEVICE(PL2303_VENDOR_ID, PL2303_PRODUCT_ID_GPRS) },
	{ USB_DEVICE(PL2303_VENDOR_ID, PL2303_PRODUCT_ID_HCR331) },
	{ USB_DEVICE(PL2303_VENDOR_ID, PL2303_PRODUCT_ID_MOTOROLA) },
	{ USB_DEVICE(IODATA_VENDOR_ID, IODATA_PRODUCT_ID) },
	{ USB_DEVICE(IODATA_VENDOR_ID, IODATA_PRODUCT_ID_RSAQ5) },
	{ USB_DEVICE(ATEN_VENDOR_ID, ATEN_PRODUCT_ID) },
	{ USB_DEVICE(ATEN_VENDOR_ID2, ATEN_PRODUCT_ID) },
	{ USB_DEVICE(ELCOM_VENDOR_ID, ELCOM_PRODUCT_ID) },
	{ USB_DEVICE(ELCOM_VENDOR_ID, ELCOM_PRODUCT_ID_UCSGT) },
	{ USB_DEVICE(ITEGNO_VENDOR_ID, ITEGNO_PRODUCT_ID) },
	{ USB_DEVICE(ITEGNO_VENDOR_ID, ITEGNO_PRODUCT_ID_2080) },
	{ USB_DEVICE(MA620_VENDOR_ID, MA620_PRODUCT_ID) },
	{ USB_DEVICE(RATOC_VENDOR_ID, RATOC_PRODUCT_ID) },
	{ USB_DEVICE(TRIPP_VENDOR_ID, TRIPP_PRODUCT_ID) },
	{ USB_DEVICE(RADIOSHACK_VENDOR_ID, RADIOSHACK_PRODUCT_ID) },
	{ USB_DEVICE(DCU10_VENDOR_ID, DCU10_PRODUCT_ID) },
	{ USB_DEVICE(SITECOM_VENDOR_ID, SITECOM_PRODUCT_ID) },
	{ USB_DEVICE(ALCATEL_VENDOR_ID, ALCATEL_PRODUCT_ID) },
	{ USB_DEVICE(SAMSUNG_VENDOR_ID, SAMSUNG_PRODUCT_ID) },
	{ USB_DEVICE(SIEMENS_VENDOR_ID, SIEMENS_PRODUCT_ID_SX1) },
	{ USB_DEVICE(SIEMENS_VENDOR_ID, SIEMENS_PRODUCT_ID_X65) },
	{ USB_DEVICE(SIEMENS_VENDOR_ID, SIEMENS_PRODUCT_ID_X75) },
	{ USB_DEVICE(SIEMENS_VENDOR_ID, SIEMENS_PRODUCT_ID_EF81) },
	{ USB_DEVICE(BENQ_VENDOR_ID, BENQ_PRODUCT_ID_S81) }, /* Benq/Siemens S81 */
	{ USB_DEVICE(SYNTECH_VENDOR_ID, SYNTECH_PRODUCT_ID) },
	{ USB_DEVICE(NOKIA_CA42_VENDOR_ID, NOKIA_CA42_PRODUCT_ID) },
	{ USB_DEVICE(CA_42_CA42_VENDOR_ID, CA_42_CA42_PRODUCT_ID) },
	{ USB_DEVICE(SAGEM_VENDOR_ID, SAGEM_PRODUCT_ID) },
	{ USB_DEVICE(LEADTEK_VENDOR_ID, LEADTEK_9531_PRODUCT_ID) },
	{ USB_DEVICE(SPEEDDRAGON_VENDOR_ID, SPEEDDRAGON_PRODUCT_ID) },
	{ USB_DEVICE(DATAPILOT_U2_VENDOR_ID, DATAPILOT_U2_PRODUCT_ID) },
	{ USB_DEVICE(BELKIN_VENDOR_ID, BELKIN_PRODUCT_ID) },
	{ USB_DEVICE(ALCOR_VENDOR_ID, ALCOR_PRODUCT_ID) },
	{ USB_DEVICE(WS002IN_VENDOR_ID, WS002IN_PRODUCT_ID) },
	{ USB_DEVICE(COREGA_VENDOR_ID, COREGA_PRODUCT_ID) },
	{ USB_DEVICE(YCCABLE_VENDOR_ID, YCCABLE_PRODUCT_ID) },
	{ USB_DEVICE(SUPERIAL_VENDOR_ID, SUPERIAL_PRODUCT_ID) },
	{ USB_DEVICE(HP_VENDOR_ID, HP_LD220_PRODUCT_ID) },
	{ USB_DEVICE(CRESSI_VENDOR_ID, CRESSI_EDY_PRODUCT_ID) },
	{ USB_DEVICE(ZEAGLE_VENDOR_ID, ZEAGLE_N2ITION3_PRODUCT_ID) },
	{ USB_DEVICE(SONY_VENDOR_ID, SONY_QN3USB_PRODUCT_ID) },
	{ USB_DEVICE(SANWA_VENDOR_ID, SANWA_PRODUCT_ID) },
	{ USB_DEVICE(ADLINK_VENDOR_ID, ADLINK_ND6530_PRODUCT_ID) },
	{ }					/* Terminating entry */
};

static enum pl2303_type type = HX;	
struct usb_device *pPL2303Dev = NULL;
//static unsigned char u1RxBuf[RX_BUFFER_SIZE];
static unsigned char *u1RxBuf = NULL;
static unsigned char u1RxLen = 0;
static unsigned char u1RxIndex = 0;

static int pl2303_vendor_read(__u16 value, __u16 index,
		struct usb_device *dev, unsigned char *buf)
{
    int res = usb_control_msg(dev, usb_rcvctrlpipe(dev, 0),
    		VENDOR_READ_REQUEST, VENDOR_READ_REQUEST_TYPE,
    		value, index, buf, 1, 100);
    /*
    printf("0x%x:0x%x:0x%x:0x%x  %d - %x.\n", VENDOR_READ_REQUEST_TYPE,
    		VENDOR_READ_REQUEST, value, index, res, buf[0]);
    */    		
    return res;
}

static int pl2303_vendor_write(__u16 value, __u16 index,
		struct usb_device *dev)
{
    int res = usb_control_msg(dev, usb_sndctrlpipe(dev, 0),
    		VENDOR_WRITE_REQUEST, VENDOR_WRITE_REQUEST_TYPE,
    		value, index, NULL, 0, 100);
    /*
    printf("0x%x:0x%x:0x%x:0x%x  %d.\n", VENDOR_WRITE_REQUEST_TYPE,
    		VENDOR_WRITE_REQUEST, value, index, res);
    */    		
    return res;
}

static int set_control_lines(struct usb_device *dev, u8 value)
{
	int retval;

	retval = usb_control_msg(dev, usb_sndctrlpipe(dev, 0),
				 SET_CONTROL_REQUEST, SET_CONTROL_REQUEST_TYPE,
				 value, 0, NULL, 0, 100);
	//printf("%s - value = %d, retval = %d.p\n", __func__, value, retval);
	return retval;
}

static void pl2303_set_termios(struct usb_device *dev,
		int baud, int stopbits, int parity, int databits)
{
	unsigned char buf[7];

	/* For reference buf[0]:buf[3] baud rate value */
	/* NOTE: Only the values defined in baud_sup are supported !
	 *       => if unsupported values are set, the PL2303 seems to use
	 *          9600 baud (at least my PL2303X always does)
	 */
	buf[0] = baud & 0xff;
	buf[1] = (baud >> 8) & 0xff;
	buf[2] = (baud >> 16) & 0xff;
	buf[3] = (baud >> 24) & 0xff;

	/* For reference buf[4]=0 is 1 stop bits */
	/* For reference buf[4]=1 is 1.5 stop bits */
	/* For reference buf[4]=2 is 2 stop bits */
	buf[4] = stopbits;

	/* For reference buf[5]=0 is none parity */
	/* For reference buf[5]=1 is odd parity */
	/* For reference buf[5]=2 is even parity */
	/* For reference buf[5]=3 is mark parity */
	/* For reference buf[5]=4 is space parity */
        buf[5] = parity;

	/* For reference buf[6]=8 is data bits = 8 */
	buf[6] = databits;

	usb_control_msg(dev, usb_sndctrlpipe(dev, 0),
			    SET_LINE_REQUEST, SET_LINE_REQUEST_TYPE,
			    0, 0, buf, 7, 100);

        /* Flow control: None */
        set_control_lines(dev, (CONTROL_DTR | CONTROL_RTS));

        /* Do not know what is this from PC log ?? */        
        pl2303_vendor_write(0x0505, 0x0, dev);
}

static int pl2303_startup(struct usb_device *dev)
{
    unsigned char buf[10];

    if (dev->descriptor.bDeviceClass == 0x02)
    	type = type_0;
    else if (dev->descriptor.bMaxPacketSize0 == 0x40)
    	type = HX;
    else if (dev->descriptor.bDeviceClass == 0x00)
    	type = type_1;
    else if (dev->descriptor.bDeviceClass == 0xFF)
    	type = type_1;
    //printf("device type: %d.\n", type);

    pl2303_vendor_read(0x8484, 0, dev, buf);
    pl2303_vendor_write(0x0404, 0, dev);
    pl2303_vendor_read(0x8484, 0, dev, buf);
    pl2303_vendor_read(0x8383, 0, dev, buf);
    pl2303_vendor_read(0x8484, 0, dev, buf);
    pl2303_vendor_write(0x0404, 1, dev);
    pl2303_vendor_read(0x8484, 0, dev, buf);
    pl2303_vendor_read(0x8383, 0, dev, buf);
    pl2303_vendor_write(0, 1, dev);
    pl2303_vendor_write(1, 0, dev);
    if (type == HX)
    	pl2303_vendor_write(2, 0x44, dev);
    else
    	pl2303_vendor_write(2, 0x24, dev);

    return 0;
}

int PL2303_scan(struct usb_device *dev)
{
    int i = 0;
    struct usb_device_id *pdevid = NULL;
    int found = 0;
    
    if (pPL2303Dev)
    {
        printf("PL2303 already init.\n");
        return 1;
    }

    if(!u1RxBuf)
        u1RxBuf = malloc(RX_BUFFER_SIZE);
    
    for(i=0; i<(sizeof(pl2303_id_table) / sizeof(struct usb_device_id)); i++)
    {
        pdevid = (struct usb_device_id *)&pl2303_id_table[i];
    	
    	if ((dev->descriptor.idVendor == pdevid->idVendor) &&
    	    (dev->descriptor.idProduct == pdevid->idProduct))          
        {
            printf("PL2303 support: vid=0x%X, pid=0x%X.\n", 
                pdevid->idVendor, pdevid->idProduct);
            found = 1;
            break;    
        }
    }   
    
    if (!found)
    {
        printf("PL2303 Not support: vid=0x%X, pid=0x%X.\n", 
            pdevid->idVendor, pdevid->idProduct);
        free(u1RxBuf);
        return -1;    
    }
    
    pPL2303Dev = NULL;
    pl2303_startup(dev);    
    pl2303_set_termios(dev, 115200, 0, 0, 8);

    printf("Switch to USB PL2303 cable.\n");
    
    pPL2303Dev = dev;

    printf("Switch to USB PL2303 cable...OK.\n");
    
    return 1;
}

void PL2303_serial_putc (const char c)
{
    int actual_len = 0;
    char data;
    struct usb_device *dev;
    int ret;
    
    if (pPL2303Dev)
    {
        dev = pPL2303Dev;
        
        if (c == '\n')
        {
            data = '\r';
            ret = usb_bulk_msg(dev, usb_sndbulkpipe(dev, 2), (void *)&data,
                1, &actual_len, 100);
            if ((ret < 0) && (dev->status == USB_ST_BIT_ERR))
            {
                dev->devnum = 0;
                pPL2303Dev = NULL;                
            }
        }

        ret = usb_bulk_msg(dev, usb_sndbulkpipe(dev, 2), (void *)&c,
            1, &actual_len, 100);        
        if ((ret < 0) && (dev->status == USB_ST_BIT_ERR))
        {
            dev->devnum = 0;
            pPL2303Dev = NULL;                
        }
    }
}

void PL2303_serial_puts (const char *s)
{
	while (*s) {
		serial_putc (*s++);
	}
}

int PL2303_serial_getc (void)
{   
    int c;
    struct usb_device *dev;
    int ret;
    
    if (pPL2303Dev && u1RxLen == 0)
    {
        dev = pPL2303Dev;
    
        u1RxIndex = 0;
        ret = usb_bulk_msg(dev, usb_rcvbulkpipe(dev, 3), (void *)u1RxBuf,
            64, (int *)&u1RxLen, 500);
        if ((ret < 0) && (dev->status == USB_ST_BIT_ERR))
        {
            dev->devnum = 0;
            pPL2303Dev = NULL;
            u1RxLen = 0;
            u1RxIndex = 0;
            return 0;            
        }        
    }

    if ((u1RxLen > 0) && (RX_BUFFER_SIZE > u1RxIndex))
    {
        c = u1RxBuf[u1RxIndex];
        u1RxIndex ++;
        u1RxLen --;
        return c;
    }

    return 0;
}

int PL2303_serial_tstc (void)
{   
    struct usb_device *dev;
    int ret;

    if (pPL2303Dev && u1RxLen == 0)
    {
        dev = pPL2303Dev;

        u1RxIndex = 0;    
        ret = usb_bulk_msg(dev, usb_rcvbulkpipe(dev, 3), (void *)u1RxBuf,
            64, (int *)&u1RxLen, 500);
        if ((ret < 0) && (dev->status == USB_ST_BIT_ERR))
        {
            dev->devnum = 0;
            pPL2303Dev = NULL;
            u1RxLen = 0;
            u1RxIndex = 0;            
        }                
    }

    return u1RxLen;
}

#endif
