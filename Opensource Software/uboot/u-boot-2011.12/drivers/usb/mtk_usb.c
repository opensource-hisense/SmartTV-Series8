/*-----------------------------------------------------------------------------
 *\drivers\usb\mtk_usb.c
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


/** @file mtk_usb.c
 *  This C file implements the mtk usb host controller driver functions.  
 */

//---------------------------------------------------------------------------
// Include files
//---------------------------------------------------------------------------
#include <common.h>

#include "x_hal_5381.h"

#ifdef CONFIG_USB_MTKHCD

#include <usb.h>
#include "x_typedef.h"

#include "mgc_hdrc.h"
#include "mgc_dma.h"

//---------------------------------------------------------------------------
// Configurations
//---------------------------------------------------------------------------

//#define MTK_DEBUG

#ifdef MTK_DEBUG
	#define LOG(level, fmt, args...) \
		printf("[%s:%d] " fmt, __PRETTY_FUNCTION__, __LINE__ , ## args)
#else
	#define LOG(level, fmt, args...)
#endif

static inline void delay (unsigned long loops)
{
	__asm__ volatile ("1:\n"
		"subs %0, %1, #1\n"
		"bne 1b":"=r" (loops):"0" (loops));
}


#define mdelay(n) ({unsigned long msec=(n); while (msec--) delay(1000000);})

#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5891)
#define XHCI_REG_READ32(r) \
    *((volatile UINT32 *)(((UINT32)0xf0074000) + (r)))

#define XHCI_REG_WRITE32(r, v) \
    *((volatile UINT32 *)(((UINT32)0xf0074000) + (r))) = v
#define U3PHY_REG_WRITE32(r, v) \
    *((volatile UINT32 *)(((UINT32)0xf0070000) + (r))) = v
#endif
//---------------------------------------------------------------------------
// Constant definitions
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// Type definitions
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
// Macro definitions
//---------------------------------------------------------------------------
/*
 *	PIPE attributes
 */
#define MENTOR_PIPE_CONTROL                      (0)
#define MENTOR_PIPE_ISOCHRONOUS             (1)
#define MENTOR_PIPE_BULK                              (2)
#define MENTOR_PIPE_INTERRUPT                   (3)

/* USB directions */
#define USB_OUT           (TRUE)
#define USB_IN              (FALSE)

/* DMA control */
#define DMA_ON           (TRUE)
#define DMA_OFF           (FALSE)



/** Low-speed USB */
#define MUSB_CONNECTION_SPEED_LOW (1)
/** Full-speed USB */
#define MUSB_CONNECTION_SPEED_FULL (2)
/** High-speed USB */
#define MUSB_CONNECTION_SPEED_HIGH (3)

#if defined(CONFIG_ARCH_MT5398) || defined(CONFIG_ARCH_MT5880)
#define USB_MAX_EP_NUM                   (5)
#else //defined(CONFIG_ARCH_MT5399)
#define USB_MAX_EP_NUM                   (8)
#endif
//---------------------------------------------------------------------------
// Imported variables
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// Imported functions
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// Static function forward declarations
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// Static variables
//---------------------------------------------------------------------------

static const UINT8 _aTestPacket [] =
{
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
    0xaa, 0xee, 0xee, 0xee, 0xee, 0xee, 0xee, 0xee, 0xee, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0x7f, 0xbf, 0xdf, 0xef, 0xf7, 0xfb, 0xfd, 0xfc, 0x7e, 0xbf, 0xdf,
    0xef, 0xf7, 0xfb, 0xfd, 0x7e
};

static unsigned short AdjustIndex = ((0x3<<4)|(0x0C));

#define USB_PORT_TEST_J				0x01
#define USB_PORT_TEST_K				0x02
#define USB_PORT_TEST_SE0NAK		0x03
#define USB_PORT_TEST_PACKET		0x04

//---------------------------------------------------------------------------
// Static functions
//---------------------------------------------------------------------------

//-------------------------------------------------------------------------
/** MUC_WaitEpIrq
 *  wait ep interrupt.
 *  @param  prDev: point to struct usb_device. 
 *  @param  u4EpNum          selected ep number
 *  @param  fgTX                  tx = 1, rx = 0.
 *  @param  fgDMA               check dma.
 *  @retval 0	Success
 *  @retval Others	Error
 */
//-------------------------------------------------------------------------
static INT32 MUC_WaitEpIrq(struct usb_device *prDev, UINT32 u4EpNum, BOOL fgTX, BOOL fgDMA)
{
    volatile UINT32 u4Reg = 0; /* read USB registers into this */
    volatile UINT32 u4Intreg[2] = { 0, 0 };
    volatile UINT16 u2Reg;
    UINT32 u4Port = (UINT32)prDev->portnum;

    //while (u4TimeOut < 0x100000)
    while(1)
    {           
        /*
         *  Start by seeing if INTRUSB active with anything
         */
        u4Reg = MOTG_REG_READ8(M_REG_INTRUSB);
        u4Reg &= (M_INTR_VBUSERROR | M_INTR_DISCONNECT | M_INTR_CONNECT | M_INTR_BABBLE);
        if (u4Reg)
        {
            // must reset this device, no more action.
            mtkusbport[u4Port].u4Insert = 0;
            MOTG_REG_WRITE8(M_REG_INTRUSB, u4Reg);
            prDev->status = USB_ST_BIT_ERR;
            printf("INTRUSB=%X.\n", (unsigned int)u4Reg);
            return -1;
        }

        /*
         *  EP0 active if INTRTX1, bit 0=1.  TX or RX on EP0 causes this bit to
         *  be set.
         */
        u4Intreg[0] = MOTG_REG_READ16(M_REG_INTRTX);
        if (u4Intreg[0])
        {
            MOTG_REG_WRITE16(M_REG_INTRTX, u4Intreg[0]);
            // EP0 case.
            if (u4EpNum == 0)
            {
                u2Reg = MOTG_REG_READ16(M_REG_CSR0);
                if (u2Reg & M_CSR0_H_RXSTALL)
                {                
                    // must reset this device, no more action.
                    prDev->status = USB_ST_BIT_ERR;
                    return -1;
                }

                if ((u2Reg & M_CSR0_H_ERROR) ||
                     (u2Reg & M_CSR0_H_NAKTIMEOUT))
                {                
                    // must reset this device, no more action.
                    mtkusbport[u4Port].u4Insert = 0;                            
                    prDev->status = USB_ST_BIT_ERR;
                    return -1;
                }
            
                //_MAX_u4TimeOut = (_MAX_u4TimeOut < u4TimeOut) ? u4TimeOut: _MAX_u4TimeOut;    
                if (u4Intreg[0] & 1)
                {
                    return 0;
                }
                else
                {
                    // must reset this device, no more action.
                    mtkusbport[u4Port].u4Insert = 0;                                            
                    prDev->status = USB_ST_BIT_ERR;            
                    return -1;
                }                                
            }
            else
            {
                u2Reg = MOTG_REG_READ16(M_REG_TXCSR);
                if ((u2Reg & M_TXCSR_H_RXSTALL) ||
                     (u2Reg & M_TXCSR_H_ERROR) ||
                     (u2Reg & M_TXCSR_H_NAKTIMEOUT))
                {                
                    // must reset this device, no more action.
                    mtkusbport[u4Port].u4Insert = 0;                            
                    prDev->status = USB_ST_BIT_ERR;
                    return -1;
                }
                
                if ((fgTX) && (u4Intreg[0] == (1<<u4EpNum)))
                {
                    return 0;
                }
                else
                {
                    // must reset this device, no more action.
                    mtkusbport[u4Port].u4Insert = 0;                                                            
                    prDev->status = USB_ST_BIT_ERR;            
                    return -1;
                }                
            }
        }

        u4Intreg[1] = MOTG_REG_READ16(M_REG_INTRRX);
        if (u4Intreg[1])
        {
            MOTG_REG_WRITE16(M_REG_INTRRX, u4Intreg[1]);

            u2Reg = MOTG_REG_READ16(M_REG_RXCSR);
            // device nak time out.
            if (u2Reg & M_RXCSR_DATAERROR)
            {                
                MOTG_REG_WRITE16(M_REG_RXCSR, 0);
                prDev->status = USB_ST_NAK_REC;
                return -1;
            }

            if ((u2Reg & M_RXCSR_H_RXSTALL) ||
                 (u2Reg & M_RXCSR_H_ERROR))
            {                
                // must reset this device, no more action.
                mtkusbport[u4Port].u4Insert = 0;                            
                prDev->status = USB_ST_BIT_ERR;
                return -1;
            }
            
            //_MAX_u4TimeOut = (_MAX_u4TimeOut < u4TimeOut) ? u4TimeOut: _MAX_u4TimeOut;            
            if ((!fgTX) && (u4Intreg[1] == (1<<u4EpNum)))
            {
                return 0;
            }
            else
            {
                // must reset this device, no more action.
                mtkusbport[u4Port].u4Insert = 0;                                        
                prDev->status = USB_ST_BIT_ERR;            
                return -1;
            }
        }
    }

    //prDev->status = USB_ST_BIT_ERR;
    //return -1;
}
//-------------------------------------------------------------------------
/** MUC_ReadFifo
 *  read data from selected ep fifo.
 *  @param  *pu1Buf           point to data buffer.
 *  @param  u4EpNum         selected ep number.
 *  @param  u4ReadCount           read data length. 
 *  @return  void
 */
//-------------------------------------------------------------------------
static void MUC_ReadFifo(UINT32 u4Port, UINT8 *pu1Buf, UINT32 u4EpNum, UINT32 u4ReadCount)
{
    UINT32 u4FifoAddr;
    UINT32 u4Val;
    UINT32 i;
    UINT32 u4Count;

    if (u4ReadCount == 0)
    {
        return;
    }

    u4FifoAddr = FIFO_ADDRESS(u4EpNum); /* blds absolute fifo addrs */

    LOG(7, "fifo read = %d.\n", u4ReadCount);
    // set FIFO byte count = 4 bytes.
    MGC_FIFO_CNT(M_REG_FIFOBYTECNT, 2);
    if ((u4ReadCount > 0) && ((UINT32)pu1Buf & 3))
    {
        u4Count = 4;
        /* byte access for unaligned */
        while (u4ReadCount > 0)
        {
		    if(3 == u4ReadCount || 2 == u4ReadCount)
		    {
		       MGC_FIFO_CNT(M_REG_FIFOBYTECNT, 1);
		       u4Val = *((volatile UINT32 *)u4FifoAddr);
		       for (i = 0; i < 2; i++)
		       {
		           *pu1Buf++ = ((u4Val >> (i *8)) & 0xFF);
		       }				   
		       u4ReadCount -=2;
		    }
		    if(1 == u4ReadCount) 
		    {
		        MGC_FIFO_CNT(M_REG_FIFOBYTECNT, 0);
		        u4Val = *((volatile UINT32 *)u4FifoAddr);
		        *pu1Buf++ = (u4Val  & 0xFF);
		        u4ReadCount -= 1;
		    }	
					
			    // set FIFO byte count = 4 bytes.
			    MGC_FIFO_CNT(M_REG_FIFOBYTECNT, 2);
			    u4ReadCount = 0;
        }
    }
    else
    {
        /* word access if aligned */
        while ((u4ReadCount > 3) && !((UINT32)pu1Buf & 3))
        {
            *((volatile UINT32 *)pu1Buf) = *((volatile UINT32 *)u4FifoAddr);

            pu1Buf = pu1Buf + 4;
            u4ReadCount = u4ReadCount - 4;
        }
        if (u4ReadCount > 0)
        {
	 	    if(3 == u4ReadCount ||2 == u4ReadCount )
		    {
				MGC_FIFO_CNT(M_REG_FIFOBYTECNT, 1);
		        u4Val = *((volatile UINT32 *)u4FifoAddr);
				for (i = 0; i < 2; i++)
				{
				    *pu1Buf++ = ((u4Val >> (i *8)) & 0xFF);
		 		}
	            u4ReadCount -= 2;
		    }

		    if(1 == u4ReadCount)
		    {
		        MGC_FIFO_CNT(M_REG_FIFOBYTECNT,0);
		        u4Val = *((volatile UINT32 *)u4FifoAddr);
						   
				*pu1Buf++ = (u4Val & 0xFF);
				u4ReadCount -= 1;			   
		     }
						
		     // set FIFO byte count = 4 bytes.
		     MGC_FIFO_CNT(M_REG_FIFOBYTECNT, 2);
	        }
    }

    // set FIFO byte count = 4 bytes.
   MGC_FIFO_CNT(M_REG_FIFOBYTECNT, 2);
}

//-------------------------------------------------------------------------
/** MUC_WriteFifo
 *  write data to selected ep fifo.
 *  @param  *pu1Buf           point to data buffer.
 *  @param  u4EpNum         selected ep number.
 *  @param  u4WriteCount    write data length. 
 *  @return  void
 */
//-------------------------------------------------------------------------
static void MUC_WriteFifo(UINT32 u4Port, UINT8 *pu1Buf, UINT32 u4EpNum, UINT32 u4WriteCount)
{
    UINT32 u4FifoAddr;
    UINT32 u4Buf;
    UINT32 u4BufSize = 4;

    u4FifoAddr = FIFO_ADDRESS(u4EpNum);

    LOG(7, "fifo write = %d.\n", u4WriteCount);
    /* byte access for unaligned */
    if ((u4WriteCount > 0) && ((UINT32)pu1Buf & 3))
    {
        while (u4WriteCount)
        {
           if (3 == u4WriteCount || 2 == u4WriteCount)
	   {
	       u4BufSize = 2;     		
	       // set FIFO byte count.
	       MGC_FIFO_CNT(M_REG_FIFOBYTECNT, 1);
	   }
	   else if(1 == u4WriteCount)
	   {
                u4BufSize = 1;	 		
	        // set FIFO byte count.
	        MGC_FIFO_CNT(M_REG_FIFOBYTECNT, 0);			   
	    }

	    memcpy((void *)&u4Buf, (const void *)pu1Buf, u4BufSize);

	    *((volatile UINT32 *)u4FifoAddr) = u4Buf;
				
	    u4WriteCount -= u4BufSize;
	    pu1Buf += u4BufSize;
        }
    }
    else /* word access if aligned */
    {
        while ((u4WriteCount > 3) && !((UINT32)pu1Buf & 3))
        {
            *((volatile UINT32 *)u4FifoAddr) = *((volatile UINT32 *)pu1Buf);

            pu1Buf = pu1Buf + 4;
            u4WriteCount = u4WriteCount - 4;
        }
			if (3 == u4WriteCount || 2 == u4WriteCount)
			{
				// set FIFO byte count.
				MGC_FIFO_CNT(M_REG_FIFOBYTECNT, 1);
		
				*((volatile UINT32 *)u4FifoAddr) = *((volatile UINT32 *)pu1Buf);
				u4WriteCount -= 2;
				pu1Buf += 2;
			}
			
			if(1 == u4WriteCount)
			{
				MGC_FIFO_CNT(M_REG_FIFOBYTECNT, 0);
				if((uint32_t)pu1Buf & 3)
				{					
					memcpy((void *)&u4Buf, (const void *)pu1Buf, 1);
					*((volatile UINT32 *)u4FifoAddr) = u4Buf;
				}
				else
				{
					*((volatile UINT32 *)u4FifoAddr) = *((volatile UINT32 *)pu1Buf);		
				}
			}
    }
    // set FIFO byte count = 4 bytes.
	MGC_FIFO_CNT(M_REG_FIFOBYTECNT, 2);
}

//-------------------------------------------------------------------------
/** MUC_DevInsert
 *  check device insert or not. If device insert, try to reset device.
 *  @param   void.
 *  @retval 0	device insert
 *  @retval Others	device not insert
 */
//-------------------------------------------------------------------------
static INT32 MUC_DevInsert(UINT32 u4Port)
{
    volatile UINT32 reg = 0; /* read USB registers into this */
    volatile UINT32 linestate;
	
    /*
     *  Start by seeing if INTRUSB active with anything
     */

     mdelay(30);
	reg = MOTG_REG_READ8(M_REG_INTRUSB);   
	
    if (reg & 0x10)
    {
        MOTG_REG_WRITE8(M_REG_INTRUSB, reg);
        reg = M_INTR_CONNECT;
    }  
    else
    {
        reg = M_INTR_DISCONNECT;
    }    
    
    if (reg & M_INTR_CONNECT) // connect event.            
    {
        mdelay(50);

		linestate = MOTG_PHY_REG_READ32(M_REG_PHYC5);
        linestate = linestate >> 6;
        linestate &= M_REG_LINESTATE_MASK;

       if ((linestate != LINESTATE_DISCONNECT) && (linestate != LINESTATE_HWERROR))
        {            
            printf("\nUSB: Detect device !\n");

            // try to reset device.
            reg = MOTG_REG_READ8(M_REG_POWER);
            reg |= M_POWER_RESET;
            MOTG_REG_WRITE8(M_REG_POWER, reg);

            mdelay(30);

            // clear reset.
            reg = MOTG_REG_READ8(M_REG_POWER);
            reg &= ~M_POWER_RESET;
            MOTG_REG_WRITE8(M_REG_POWER, reg);


            // check device speed: LS, FS, HS.
            reg = MOTG_REG_READ8(M_REG_DEVCTL);

            if (reg & M_DEVCTL_LSDEV)
            {
                printf("USB: LS device.\n");
                mtkusbport[u4Port].u4DeviceSpeed = MUSB_CONNECTION_SPEED_LOW;
            }
            else
            {
                reg = MOTG_REG_READ8(M_REG_POWER);
				
                if (reg & M_POWER_HSMODE)
                {
                    printf("USB: HS device.\n");
                    mtkusbport[u4Port].u4DeviceSpeed = MUSB_CONNECTION_SPEED_HIGH;
                }
                else
                {
                    printf("USB: FS device.\n");
                    mtkusbport[u4Port].u4DeviceSpeed = MUSB_CONNECTION_SPEED_FULL;
                }
            }

            mtkusbport[u4Port].u4Insert= TRUE;
            
            return 0;
        }
    }

    return -1;
}

//-------------------------------------------------------------------------
/** MUC_Initial
 *  host controller register reset and initial.
 *  @param  void 
 *  @retval 0	Success
 *  @retval Others	Error
 */
//-------------------------------------------------------------------------
static INT32 MUC_Initial(UINT32 u4Port)
{
    uint32_t u4Reg = 0;
    uint8_t nDevCtl = 0;
    uint32_t *pu4MemTmp;

    //Soft Reset, RG_RESET for Soft RESET
    u4Reg = MOTG_PHY_REG_READ32(0x68);
    u4Reg |=   0x00004000; 
    MOTG_PHY_REG_WRITE32(0x68, u4Reg);
    u4Reg &=  ~0x00004000; 
    MOTG_PHY_REG_WRITE32(0x68, u4Reg);	
	
    #ifdef CONFIG_ARCH_MT5880
    pu4MemTmp = (uint32_t *)0xf0061300;
    *pu4MemTmp = 0x00cfff00;
	
    #elif defined(CONFIG_ARCH_MT5881)
    pu4MemTmp = (uint32_t *)0xf0061200;
    *pu4MemTmp = 0x00cfff00;
	
    #elif defined(CONFIG_ARCH_MT5399) || defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5861) || defined(CONFIG_ARCH_MT5891)
    pu4MemTmp = (uint32_t *)0xf0061A00;
    *pu4MemTmp = 0x00cfff00;
    
    #elif defined(CONFIG_ARCH_MT5882)
    pu4MemTmp = (uint32_t *)0xf0061B00;
    *pu4MemTmp = 0x00cfff00;
	
	#elif defined(CONFIG_ARCH_MT5883)
    pu4MemTmp = (uint32_t *)0xf0061B00;
    *pu4MemTmp = 0x00cfff00;
	
    #else
    pu4MemTmp = (uint32_t *)0xf0060200;
    *pu4MemTmp = 0x00cfff00;
    #endif
	
	#if !defined(CONFIG_ARCH_MT5890) && !defined(CONFIG_ARCH_MT5891)
    // VRT insert R enable 
    u4Reg = MOTG_PHY_REG_READ32(0x00);
    u4Reg |= 0x4000;
    MOTG_PHY_REG_WRITE32(0x00, u4Reg);
	#endif
	
	#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5891)
	//add for Oryx USB20 PLL fail
	u4Reg = MOTG_PHY_REG_READ32(0x00); //RG_USB20_USBPLL_FORCE_ON=1b
	u4Reg |=   0x00008000; 
	MOTG_PHY_REG_WRITE32(0x00, u4Reg);

	u4Reg = MOTG_PHY_REG_READ32(0x14); //RG_USB20_HS_100U_U3_EN=1b
	u4Reg |=   0x00000800; 
	MOTG_PHY_REG_WRITE32(0x14, u4Reg);
    
	if(IS_IC_5890_ES1())
	{
		//driving adjust
		u4Reg = MOTG_PHY_REG_READ32(0x04);
		u4Reg &=  ~0x00007700;
		MOTG_PHY_REG_WRITE32(0x04, u4Reg);
	}
	#endif
	
    //otg bit setting
    u4Reg = MOTG_PHY_REG_READ32(0x6C);
    u4Reg &= ~0x00003f3f;
    u4Reg |=  0x00003f2c;
    MOTG_PHY_REG_WRITE32(0x6C, u4Reg);

    //suspendom control
    u4Reg = MOTG_PHY_REG_READ32(0x68);
    u4Reg &=  ~0x00040000; 
    MOTG_PHY_REG_WRITE32(0x68, u4Reg);

    //disconnect threshold,set 620mV
    u4Reg = MOTG_PHY_REG_READ32(0x18); //RG_USB20_DISCTH
	#if defined(CC_MT5890) || defined(CC_MT5891)	
	if(IS_IC_5890_ES1())
	{
	    u4Reg &= ~0x0000000f; //0x18[3:0]
	    u4Reg |=  0x0000000b;
	}
	else
	{
	    u4Reg &= ~0x000000f0; //0x18[7:4]
	    u4Reg |=  0x000000b0;
	}
	#else
    u4Reg &= ~0x000f0000; //others IC is 0x18[19:16]
    u4Reg |=  0x000b0000;
	#endif
    MOTG_PHY_REG_WRITE32(0x18, u4Reg);
	
    u4Reg = MOTG_REG_READ8(M_REG_BUSPERF3);
    u4Reg |=  0x80;
    u4Reg &= ~0x40;
    MOTG_REG_WRITE8(M_REG_BUSPERF3, (uint8_t)u4Reg);

    #if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5891)
    u4Reg = MOTG_REG_READ8((uint32_t)0x7C);
    u4Reg |= 0x04;
    MOTG_REG_WRITE8(0x7C, (uint8_t)u4Reg);
    #else
    // MT5368/MT5396/MT5389 
    //0xf0050070[10]= 1 for Fs Hub + Ls Device 
    u4Reg = MOTG_REG_READ8(0x71);
    u4Reg |= 0x04; //RGO_USB20_PRBS7_LOCK
    MOTG_REG_WRITE8(0x71, (uint8_t)u4Reg);
	#endif

    // FS/LS Slew Rate change
    u4Reg = MOTG_PHY_REG_READ32(0x10);
    u4Reg &= (uint32_t)(~0x000000ff);
    u4Reg |= (uint32_t)0x00000011;
    MOTG_PHY_REG_WRITE32(0x10, u4Reg);

    // HS Slew Rate, RG_USB20_HSTX_SRCTRL
	#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5891)
    u4Reg = MOTG_PHY_REG_READ32(0x14); //bit[14:12]
	if(IS_IC_5890_ES1())
	{
		u4Reg &= (uint32_t)(~0x00007000);
		u4Reg |= (uint32_t)0x00001000;
	}
	else
	{
		u4Reg &= (uint32_t)(~0x00007000);
		u4Reg |= (uint32_t)0x00004000;
	}
    MOTG_PHY_REG_WRITE32(0x14, u4Reg);
	#else
    u4Reg = MOTG_PHY_REG_READ32(0x10);
    u4Reg &= (uint32_t)(~0x00070000);
    u4Reg |= (uint32_t)0x00040000;
    MOTG_PHY_REG_WRITE32(0x10, u4Reg);
	#endif
#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5891)//usb2.0 port3 driving adjust
    u4Reg = MOTG_PHY_REG_READ32(0x14);
	u4Reg &= ~(uint32_t)0x300000;
	u4Reg |= (uint32_t)0x10200000; //bit[21:20]: RG_USB20_DISCD[1:0]=2'b10. bit[28]: RG_USB20_DISC_FIT_EN=1'b1
	MOTG_PHY_REG_WRITE32(0x14, u4Reg);
#endif

#if 1
    // This is important: TM1_EN
    // speed up OTG setting for checking device connect event after MUC_Initial().
    u4Reg = MOTG_REG_READ32(0x604);
    u4Reg |= 0x01;
    MOTG_REG_WRITE32(0x604, u4Reg);
#endif
    // open session.
    nDevCtl = MOTG_REG_READ8(MGC_O_HDRC_DEVCTL);	 
    nDevCtl |= MGC_M_DEVCTL_SESSION;		
    MOTG_REG_WRITE8(MGC_O_HDRC_DEVCTL, nDevCtl);

    mdelay(10);
#if 1
    // This is important: TM1_EN 
    // speed up OTG setting for checking device connect event after MUC_Initial().
    u4Reg = MOTG_REG_READ32(0x604);
    u4Reg &= ~0x01;
    MOTG_REG_WRITE32(0x604, u4Reg);
#endif

    return 0;
}

//-------------------------------------------------------------------------
/** MUC_PrepareRx
 *  host controller init register for 539x.
 *  @param  void 
 *  @retval void
 */
//-------------------------------------------------------------------------
static void MUC_PrepareRx(struct usb_device *prDev, UINT32 u4Pipe)
{
    UINT8 u1Reg = 0;
    UINT32 u4DevNum = usb_pipedevice(u4Pipe);
    UINT32 u4EpNum = usb_pipeendpoint(u4Pipe);
    UINT32 u4Port = (UINT32)prDev->portnum;

#ifdef MTK_MHDRC	

    UINT8 bHubAddr = 0;
    UINT8 bHubPort = 0;    
    UINT8 bIsMulti = FALSE;

    if (usb_pipecontrol(u4Pipe))
    {
        u1Reg = MENTOR_PIPE_CONTROL << MGC_S_TYPE_PROTO;
    }
    else if (usb_pipebulk(u4Pipe))
    {
        u1Reg = MENTOR_PIPE_BULK << MGC_S_TYPE_PROTO;
    }
    
    u1Reg |= u4EpNum & MGC_M_TYPE_REMOTE_END;

    /* speed field in proto reg */
    switch(mtkusbport[u4Port].u4DeviceSpeed)
    {
    case MUSB_CONNECTION_SPEED_LOW:
        u1Reg |= 0xc0;
        break;
    case MUSB_CONNECTION_SPEED_FULL:
        u1Reg |= 0x80;
        break;
    default:
        u1Reg |= 0x40;
    }

    MOTG_REG_WRITE8(M_REG_RXTYPE, u1Reg);

    /* target addr & hub addr/port */
    MOTG_REG_WRITE8(MGC_BUSCTL_OFFSET(u4EpNum, MGC_O_MHDRC_RXFUNCADDR), 
        u4DevNum);
    MOTG_REG_WRITE8(MGC_BUSCTL_OFFSET(u4EpNum, MGC_O_MHDRC_RXHUBADDR), 
        (bIsMulti ? (0x80 | bHubAddr) : bHubAddr));
    MOTG_REG_WRITE8(MGC_BUSCTL_OFFSET(u4EpNum, MGC_O_MHDRC_RXHUBPORT), 
        bHubPort);

#else

    if (usb_pipecontrol(u4Pipe))
    {
        u1Reg = MENTOR_PIPE_CONTROL << MGC_S_TYPE_PROTO;
    }
    else if (usb_pipebulk(u4Pipe))
    {
        u1Reg = MENTOR_PIPE_BULK << MGC_S_TYPE_PROTO;
    }
    
    u1Reg |= u4EpNum & MGC_M_TYPE_REMOTE_END;

    MOTG_REG_WRITE8(M_REG_RXTYPE, u1Reg);
           
#endif

    return;
}

//-------------------------------------------------------------------------
/** MUC_PrepareTx
 *  host controller init register for 539x.
 *  @param  void 
 *  @retval void
 */
//-------------------------------------------------------------------------
static void MUC_PrepareTx(struct usb_device *prDev, UINT32 u4Pipe)
{
    UINT8 u1Reg = 0;
    UINT32 u4DevNum = usb_pipedevice(u4Pipe);
    UINT32 u4EpNum = usb_pipeendpoint(u4Pipe);
    UINT32 u4Port = (UINT32)prDev->portnum;

#ifdef MTK_MHDRC	

    UINT8 bHubAddr = 0;
    UINT8 bHubPort = 0;    
    UINT8 bIsMulti = FALSE;
	
#if defined(CONFIG_ARCH_MT5881)
		if(u4Port==0)
		{
			if(u4EpNum>1)
				u4EpNum=1;
		}
#endif

    if (usb_pipecontrol(u4Pipe))
    {
        u1Reg = MENTOR_PIPE_CONTROL << MGC_S_TYPE_PROTO;
    }
    else if (usb_pipebulk(u4Pipe))
    {
        u1Reg = MENTOR_PIPE_BULK << MGC_S_TYPE_PROTO;
    }
	
#if defined(CONFIG_ARCH_MT5881)
		if(u4EpNum==1)
			u1Reg |= 2 & MGC_M_TYPE_REMOTE_END;
		else
#endif
	
    u1Reg |= u4EpNum & MGC_M_TYPE_REMOTE_END;

    /* speed field in proto reg */
    switch(mtkusbport[u4Port].u4DeviceSpeed)
    {
    case MUSB_CONNECTION_SPEED_LOW:
        u1Reg |= 0xc0;
        break;
    case MUSB_CONNECTION_SPEED_FULL:
        u1Reg |= 0x80;
        break;
    default:
        u1Reg |= 0x40;
    }

    MOTG_REG_WRITE8(M_REG_TXTYPE, u1Reg);

    /* target addr & hub addr/port */
    MOTG_REG_WRITE8(MGC_BUSCTL_OFFSET(u4EpNum, MGC_O_MHDRC_TXFUNCADDR), 
        u4DevNum);
    MOTG_REG_WRITE8(MGC_BUSCTL_OFFSET(u4EpNum, MGC_O_MHDRC_TXHUBADDR), 
        (bIsMulti ? (0x80 | bHubAddr) : bHubAddr));
    MOTG_REG_WRITE8(MGC_BUSCTL_OFFSET(u4EpNum, MGC_O_MHDRC_TXHUBPORT), 
        bHubPort);
	
    if(!u4EpNum)
    {
        MOTG_REG_WRITE8(MGC_BUSCTL_OFFSET(u4EpNum, MGC_O_MHDRC_RXFUNCADDR), 
            u4DevNum);
        MOTG_REG_WRITE8(MGC_BUSCTL_OFFSET(u4EpNum, MGC_O_MHDRC_RXHUBADDR), 
            (bIsMulti ? (0x80 | bHubAddr) : bHubAddr));
        MOTG_REG_WRITE8(MGC_BUSCTL_OFFSET(u4EpNum, MGC_O_MHDRC_RXHUBPORT), 
            bHubPort);    
    }

#else

    if (usb_pipecontrol(u4Pipe))
    {
        u1Reg = MENTOR_PIPE_CONTROL << MGC_S_TYPE_PROTO;
    }
    else if (usb_pipebulk(u4Pipe))
    {
        u1Reg = MENTOR_PIPE_BULK << MGC_S_TYPE_PROTO;
    }
    
    u1Reg |= u4EpNum & MGC_M_TYPE_REMOTE_END;

    MOTG_REG_WRITE8(M_REG_TXTYPE, u1Reg);
    
#endif

    return;
}
//-------------------------------------------------------------------------
/** MUC_SetupPkt
 *  send setup packet for ep0.
 * SETUP starts a new control request.  Devices are not allowed to
 * STALL or NAK these; they must cancel any pending control requests. 
 *  @param  prDev: point to struct usb_device.
 *  @param  u4Pipe               selected pipe.
 *  @param   *prSetup         point to setup packet.  
 *  @retval 0	Success
 *  @retval Others	Error
 */
//-------------------------------------------------------------------------
static INT32 MUC_SetupPkt(struct usb_device *prDev, UINT32 u4Pipe, struct devrequest *prSetup)
{
    UINT32 u4Len;
    UINT32 u4EpNum = usb_pipeendpoint(u4Pipe);
    UINT32 u4Port = (UINT32)prDev->portnum;

    MOTG_REG_WRITE8(M_REG_INDEX, u4EpNum);

    //  turn on nak limit.
    MOTG_REG_WRITE8(M_REG_NAKLIMIT0, 
    	(((mtkusbport[u4Port].u4DeviceSpeed) == MUSB_CONNECTION_SPEED_HIGH) ? 16 : 13));

    u4Len = sizeof(struct devrequest);

    MUC_PrepareTx(prDev, u4Pipe);

    MUC_WriteFifo((UINT32)prDev->portnum, (UINT8 *)prSetup, u4EpNum, u4Len);

    MOTG_REG_WRITE16(M_REG_CSR0, M_CSR0_H_NO_PING | M_CSR0_TXPKTRDY | M_CSR0_H_SETUPPKT);

    return MUC_WaitEpIrq(prDev, u4EpNum, USB_OUT, DMA_OFF);
}

//-------------------------------------------------------------------------
/** MUC_StatusPkt
 *  send setup packet for ep0.
 * SETUP starts a new control request.  Devices are not allowed to
 * STALL or NAK these; they must cancel any pending control requests. 
 *  @param  u4Pipe               selected pipe.
 *  @retval 0	Success
 *  @retval Others	Error
 */
//-------------------------------------------------------------------------
static INT32 MUC_StatusPkt(struct usb_device *prDev, UINT32 u4Pipe)
{
    UINT32 u4DevNum = usb_pipedevice(u4Pipe);
    UINT32 u4EpNum = usb_pipeendpoint(u4Pipe);
    INT32 i4Ret = 0;
    UINT32 u4Port = (UINT32)prDev->portnum;

    MOTG_REG_WRITE32(M_REG_EPXADDR(u4EpNum), u4DevNum);

    MOTG_REG_WRITE8(M_REG_INDEX, u4EpNum);

    if (usb_pipein(u4Pipe))
    {   
        MUC_PrepareTx(prDev, u4Pipe);
    
        MOTG_REG_WRITE8(M_REG_CSR0, M_CSR0_TXPKTRDY | M_CSR0_H_STATUSPKT);
    }
    else
    {
        MUC_PrepareRx(prDev, u4Pipe);
    
        MOTG_REG_WRITE8(M_REG_CSR0, M_CSR0_H_REQPKT | M_CSR0_H_STATUSPKT);
    }

    i4Ret = MUC_WaitEpIrq(prDev, u4EpNum, usb_pipeout(u4Pipe), DMA_OFF);

    //  Must clear RxPktRdy in CSR0 when packet has read from FIFO.
    MOTG_REG_WRITE8(M_REG_CSR0, 0);

    return i4Ret;
}

//-------------------------------------------------------------------------
/** MUC_log2
 *  Calculate log2 arithmetic.
 *  @param  x   input value.
 *  @return  log2(x).
 */
//-------------------------------------------------------------------------
static inline s32 MUC_log2(s32 x)
{
    s32 i;

    for (i = 0; x > 1; i++)
    {
        x = x / 2;
    }

    return i;
}

//-------------------------------------------------------------------------
/** MUC_InPkt
 *  send in request to device.
 *  @param   *prDev         usb_device data structure.
 *  @param  u4Pipe               selected pipe.
 *  @param   *pvBuf             user's request data buffer.  
 *  @param   u4Len              user's request data length.   
 *  @retval 0	Success
 *  @retval Others	Error
 */
//------------------------------------------------------------------------- 
static INT32 MUC_InPkt(struct usb_device *prDev, UINT32 u4Pipe, void *pvBuf, UINT32 u4Len)
{
    UINT32 u4DevNum = usb_pipedevice(u4Pipe);
    UINT32 u4EpNum = usb_pipeendpoint(u4Pipe);
    UINT32 u4MaxPktSize = usb_maxpacket(prDev, u4Pipe);
    UINT32 u4RxLen = 0;
    UINT32 u4PktLen = 0;
    INT32 i4Ret = 0;
    UINT32 u4Port = (UINT32)prDev->portnum;

    if (u4EpNum > USB_MAX_EP_NUM)
    {
        u4EpNum = USB_MAX_EP_NUM;
    }
    
    // set device address.
    MOTG_REG_WRITE32(M_REG_EPXADDR(u4EpNum), u4DevNum);

    // set physical ep.
    MOTG_REG_WRITE8(M_REG_INDEX, u4EpNum);

    MUC_PrepareRx(prDev, u4Pipe);

    // set fifo attribute.
    if (usb_pipebulk(u4Pipe))
    {   
        //  turn on nak limit.
        MOTG_REG_WRITE8(M_REG_RXINTERVAL, 
        (((mtkusbport[u4Port].u4DeviceSpeed) == MUSB_CONNECTION_SPEED_HIGH) ? 16 : 13));

#ifdef CONFIG_USB_PL2303
        do{
            extern struct usb_device *pPL2303Dev;
            
            if (pPL2303Dev == prDev)
            {
                //  turn on nak limt for PL2303.
                MOTG_REG_WRITE8(M_REG_RXINTERVAL, 2);
            }
        }while(0);
#endif
		            	    	
        if (MOTG_REG_READ16(M_REG_RXMAXP) < u4MaxPktSize)
        {
            // set rx epx fifo size,                 
            MOTG_REG_WRITE8(M_REG_RXFIFOSZ, MUC_log2(u4MaxPktSize /8));

            // set start address of tx epx fifo.
            MOTG_REG_WRITE16(M_REG_RXFIFOADD, 8);

            //  Establish max packet size.
            MOTG_REG_WRITE16(M_REG_RXMAXP, u4MaxPktSize);
        }
    }

    // use FIFO only.
    u4RxLen = 0;
    while (u4RxLen < u4Len)
    {
        // request an IN transaction.
        if (u4EpNum > 0)
        {
            MOTG_REG_WRITE8(M_REG_RXCSR1, M_RXCSR1_H_REQPKT);
        }
        else
        {
            MOTG_REG_WRITE8(M_REG_CSR0, M_CSR0_H_REQPKT);
        }
        
        i4Ret = MUC_WaitEpIrq(prDev, u4EpNum, USB_IN, DMA_OFF);
        if (0 > i4Ret)
        {
            return i4Ret;
        }

        u4PktLen = MOTG_REG_READ16(M_REG_RXCOUNT);

        // read data from fifo.		
        MUC_ReadFifo((UINT32)prDev->portnum, pvBuf, u4EpNum, u4PktLen);

        u4RxLen += u4PktLen;
        prDev->act_len += u4PktLen;;
        pvBuf += u4PktLen;        

        // check if short packet.
        if (u4PktLen < u4MaxPktSize)
        {
            return 0;
        }
    }
    return 0;
}

//-------------------------------------------------------------------------
/** MUC_OutPkt
 *  send out data to device.
 *  @param   *prDev         usb_device data structure.
 *  @param  u4Pipe               selected pipe.
 *  @param   *pvBuf             user's request data buffer.  
 *  @param   u4Len              user's request data length.   
 *  @retval 0	Success
 *  @retval Others	Error
 */
//------------------------------------------------------------------------- 
static INT32 MUC_OutPkt(struct usb_device *prDev, UINT32 u4Pipe, void *pvBuf, UINT32 u4Len)
{
    UINT32 u4DevNum = usb_pipedevice(u4Pipe);
    UINT32 u4EpNum = usb_pipeendpoint(u4Pipe);
    UINT32 u4MaxPktSize = usb_maxpacket(prDev, u4Pipe);
    UINT32 u4TxLen = 0;
    UINT32 u4PktLen = 0;
    INT32 i4Ret = 0;
    UINT32 u4Port = (UINT32)prDev->portnum;

#if defined(CONFIG_ARCH_MT5881)
		if(0==u4Port)
		{
			if(u4EpNum>1)
			{
				u4EpNum=1;
				u4MaxPktSize=512;
			}
		}
		else
#endif
{
    if (u4EpNum > USB_MAX_EP_NUM)
    {
        u4EpNum = USB_MAX_EP_NUM;
    }
}
    // set device address.
    MOTG_REG_WRITE32(M_REG_EPXADDR(u4EpNum), u4DevNum);

    // set physical ep.
    MOTG_REG_WRITE8(M_REG_INDEX, u4EpNum);

    MUC_PrepareTx(prDev, u4Pipe);

    // set fifo attribute.
    if (usb_pipebulk(u4Pipe))
    {
        //  turn on nak limit.
        MOTG_REG_WRITE8(M_REG_TXINTERVAL, 
        	(((mtkusbport[u4Port].u4DeviceSpeed) == MUSB_CONNECTION_SPEED_HIGH) ? 16 : 13));
    
        if (MOTG_REG_READ16(M_REG_TXMAXP) < u4MaxPktSize)
        {
            // set tx epx fifo size,                 
            MOTG_REG_WRITE8(M_REG_TXFIFOSZ, MUC_log2(u4MaxPktSize /8));

            // set start address of tx epx fifo.
            MOTG_REG_WRITE16(M_REG_TXFIFOADD, 8);

            //  Establish max packet size.
            MOTG_REG_WRITE16(M_REG_TXMAXP, u4MaxPktSize);
        }
    }

    // use FIFO only.
    u4TxLen = 0;
    while (u4TxLen < u4Len)
    {
        // max packet size as tx packet size limit.
        u4PktLen = min(u4MaxPktSize, (u4Len - u4TxLen));

        // write data to fifo.		
        MUC_WriteFifo((UINT32)prDev->portnum, pvBuf, u4EpNum, u4PktLen);
        
        if (u4EpNum > 0)
        {
            MOTG_REG_WRITE8(M_REG_TXCSR, M_TXCSR_TXPKTRDY);
        }
        else
        {
            MOTG_REG_WRITE8(M_REG_CSR0, M_CSR0_TXPKTRDY);
        }

        i4Ret = MUC_WaitEpIrq(prDev, u4EpNum, USB_OUT, DMA_OFF);
        if (0 > i4Ret)
        {
            return i4Ret;
        }

        u4TxLen += u4PktLen;
        pvBuf += u4PktLen;            
    }

    return i4Ret;
}

#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5891)
static int xhci_con = 0;
//-------------------------------------------------------------------------
/** XHCI connect
 *  usb module init.
 *  @param  void.
 *  @retval 0	Success
 *  @retval Others	Fail
 */
//-------------------------------------------------------------------------
int SSUSB_check_connect(void)
{
    UINT32 u4tmp;

    U3PHY_REG_WRITE32(0x700, 0x10F00); //init SSUSB
    U3PHY_REG_WRITE32(0x704, 0x0);
    U3PHY_REG_WRITE32(0x750, 0x4C);
    
    u4tmp = XHCI_REG_READ32(0x430);
    u4tmp &= ~(0x1 << 9);
    XHCI_REG_WRITE32(0x430, u4tmp);
    mdelay(10);
    u4tmp = XHCI_REG_READ32(0x430);
    u4tmp |= (0x1 << 9);
    XHCI_REG_WRITE32(0x430, u4tmp);

    mdelay(200);
    u4tmp = XHCI_REG_READ32(0x430);
    
    if(u4tmp & 0x1)
    	xhci_con = 1;
    else
    	xhci_con = 0;

    return xhci_con;
}
#endif
//---------------------------------------------------------------------------
// Public functions
//---------------------------------------------------------------------------
//-------------------------------------------------------------------------
/** usb_sendse0nak
 *  set se0 nak test mode.
 *  @void.
 *  @retval 0	Success
 *  @retval Others	Fail
 */
//-------------------------------------------------------------------------
int usb_sendse0nak(void)
{
    UINT32 u4Port,d;
    struct usb_device *dev;
    for(u4Port=0; u4Port<(sizeof(mtkusbport) / sizeof(MTKUSBPORT)); u4Port++)
    {
        if(u4Port > 3)
        {
          return -1;    
        }

#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5891)
        UINT32 u4tmp;
        if ((u4Port==2) && (xhci_con==1))
        {
            u4tmp = XHCI_REG_READ32(0x430);
            u4tmp &= ~(0x1 << 9);
            XHCI_REG_WRITE32(0x430, u4tmp);
            mdelay(10);
            u4tmp = XHCI_REG_READ32(0x434);
            u4tmp &= ~(0xf << 28);
            u4tmp |= (0x3 << 28);
            XHCI_REG_WRITE32(0x434, u4tmp);
            printf("USB port2: Set SE0 NAK...OK !\n");
            return 0;            
        }
#endif

        if (mtkusbport[u4Port].u4Insert)
        {
            for (d = 0; d < USB_MAX_DEVICE;d++) 
            {
                dev = usb_get_dev_index(d);
                if((dev->config.if_desc[0].bInterfaceClass==USB_CLASS_MASS_STORAGE)&&(dev->portnum == u4Port)){

                    //set se0 nak.
                    MOTG_REG_WRITE8(0xF, 0x01);
                    printf("USB: Port%d  Set SE0 NAK...OK !\n",u4Port);
                     return 0;        
                }
            }
        }
    }
}


//-------------------------------------------------------------------------
/** usb_hub_sendse0nak
 *  set se0 nak test mode.
 *  @void.
 *  @retval 0	Success
 *  @retval Others	Fail
 */
//-------------------------------------------------------------------------
int usb_hub_sendse0nak(char *argv)
{
	int d,DownPort;
	struct usb_device *dev;
	UINT32 u4Port;

	if (strncmp(argv, "1", 1) == 0)
		DownPort=1;
	else if(strncmp(argv, "2", 1) == 0)
		DownPort=2;
	else if(strncmp(argv, "3", 1) == 0)
		DownPort=3;
	else if(strncmp(argv, "4", 1) == 0)
		DownPort=4;
	else{ 
		printf("Parameter error\n");
		return -1;
	}

	for(u4Port=0; u4Port<(sizeof(mtkusbport) / sizeof(MTKUSBPORT)); u4Port++)
	{
		// device on port 
		if (mtkusbport[u4Port].u4Insert)
		{
			//search device 
			for (d = 0; d < USB_MAX_DEVICE;d++) 
			{
				dev = usb_get_dev_index(d);
				//match hub device and port number
				if((dev->config.if_desc[0].bInterfaceClass==USB_CLASS_HUB)&&(dev->portnum == u4Port)){
				{
					usb_control_msg(dev, 
									usb_sndctrlpipe(dev, 0),
									USB_REQ_SET_FEATURE, 
									USB_RT_PORT,
									USB_PORT_FEAT_TEST, 
									(USB_PORT_TEST_SE0NAK<<8)|(DownPort),
									NULL, 
									0, 
									USB_CNTL_TIMEOUT);
				}
				printf("USB port%d: Set Test SE0...OK !\n",u4Port);
				}
			}
		}
	}
}

//-------------------------------------------------------------------------
/** usb_sendtestj
 *  set test j test mode.
 *  @void.
 *  @retval 0	Success
 *  @retval Others	Fail
 */
//-------------------------------------------------------------------------
int usb_sendtestj(void)
{
	UINT32 u4Port,d;
	struct usb_device *dev;
	
	for(u4Port=0; u4Port<(sizeof(mtkusbport) / sizeof(MTKUSBPORT)); u4Port++)
	{
		if(u4Port > 3)
		{
			return -1;	
		}

#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5891)
        UINT32 u4tmp;
        if ((u4Port==2) && (xhci_con==1))
        {
            u4tmp = XHCI_REG_READ32(0x430);
            u4tmp &= ~(0x1 << 9);
            XHCI_REG_WRITE32(0x430, u4tmp);
            mdelay(10);
            u4tmp = XHCI_REG_READ32(0x434);
            u4tmp &= ~(0xf << 28);
            u4tmp |= (0x1 << 28);
            XHCI_REG_WRITE32(0x434, u4tmp);
            printf("USB port2: Set Test J...OK !\n");
            return 0;            
        }
#endif
        
		if (mtkusbport[u4Port].u4Insert)
		{
			for (d = 0; d < USB_MAX_DEVICE;d++) 
			{
				dev = usb_get_dev_index(d);
				if((dev->config.if_desc[0].bInterfaceClass==USB_CLASS_MASS_STORAGE)&&(dev->portnum == u4Port)){

					//  USB-IF: 539x D+/D- will over spec.
					MOTG_PHY_REG_WRITE32(0x04, 0x4A010406);
					MOTG_PHY_REG_WRITE32(0x08, 0x78004302);

					//set test j.
					MOTG_REG_WRITE8(0xF, 0x02);
					printf("USB: Port%d Set Test J...OK !\n",u4Port);
					return 0;                 
				}
			}
		}
	}
}

int usb_hub_sendtestj(char *argv)
{
	int d,DownPort;
	struct usb_device *dev;
	UINT32 u4Port;

	if (strncmp(argv, "1", 1) == 0)
		DownPort=1;
	else if(strncmp(argv, "2", 1) == 0)
		DownPort=2;
	else if(strncmp(argv, "3", 1) == 0)
		DownPort=3;
	else if(strncmp(argv, "4", 1) == 0)
		DownPort=4;
	else{ 
		printf("Parameter error\n");
		return -1;
	}

	for(u4Port=0; u4Port<(sizeof(mtkusbport) / sizeof(MTKUSBPORT)); u4Port++)
	{
		// device on port 
		if (mtkusbport[u4Port].u4Insert)
		{
			//search device 
			for (d = 0; d < USB_MAX_DEVICE;d++) 
			{
				dev = usb_get_dev_index(d);
				//match hub device and port number
				if((dev->config.if_desc[0].bInterfaceClass==USB_CLASS_HUB)&&(dev->portnum == u4Port)){
				{
					usb_control_msg(dev, 
									usb_sndctrlpipe(dev, 0),
									USB_REQ_SET_FEATURE, 
									USB_RT_PORT,
									USB_PORT_FEAT_TEST, 
									(USB_PORT_TEST_J<<8)|(DownPort),
									NULL, 
									0, 
									USB_CNTL_TIMEOUT);
				}	
				printf("USB port%d: Set Test J...OK !\n",u4Port);
				}
			}
		}
	}
}

//-------------------------------------------------------------------------
/** usb_sendtestk
 *  set test k test mode.
 *  @void.
 *  @retval 0	Success
 *  @retval Others	Fail
 */
//-------------------------------------------------------------------------
int usb_sendtestk(void)
{
	UINT32 u4Port,d;
	struct usb_device *dev;
	for(u4Port=0; u4Port<(sizeof(mtkusbport) / sizeof(MTKUSBPORT)); u4Port++)
	{   
		if(u4Port > 3)
		{
			return -1;	
		}

#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5891)
        UINT32 u4tmp;
        if ((u4Port==2) && (xhci_con==1))
        {
            u4tmp = XHCI_REG_READ32(0x430);
            u4tmp &= ~(0x1 << 9);
            XHCI_REG_WRITE32(0x430, u4tmp);
            mdelay(10);
            u4tmp = XHCI_REG_READ32(0x434);
            u4tmp &= ~(0xf << 28);
            u4tmp |= (0x2 << 28);
            XHCI_REG_WRITE32(0x434, u4tmp);
            printf("USB port2: Set Test K...OK !\n");
            return 0;            
        }
#endif

		if (mtkusbport[u4Port].u4Insert)
		{
			for (d = 0; d < USB_MAX_DEVICE;d++) 
			{
				dev = usb_get_dev_index(d);
				if((dev->config.if_desc[0].bInterfaceClass==USB_CLASS_MASS_STORAGE)&&(dev->portnum == u4Port)){

				//  USB-IF: 539x D+/D- will over spec.
				MOTG_PHY_REG_WRITE32(0x04, 0x4A010406);
				MOTG_PHY_REG_WRITE32(0x08, 0x78004302);

				//set test k.
				MOTG_REG_WRITE8(0xF, 0x04);
				printf("USB: Port%d  Set Test K...OK !\n",u4Port);
				return 0;           
				}
			}
		}
	} 
}

int usb_hub_sendtestk(char *argv)
{
	int d,DownPort;
	struct usb_device *dev;
	UINT32 u4Port;

	if (strncmp(argv, "1", 1) == 0)
		DownPort=1;
	else if(strncmp(argv, "2", 1) == 0)
		DownPort=2;
	else if(strncmp(argv, "3", 1) == 0)
		DownPort=3;
	else if(strncmp(argv, "4", 1) == 0)
		DownPort=4;
	else{ 
		printf("Parameter error\n");
		return -1;
	}

	for(u4Port=0; u4Port<(sizeof(mtkusbport) / sizeof(MTKUSBPORT)); u4Port++)
	{
		// device on port 
		if (mtkusbport[u4Port].u4Insert)
		{
			//search device 
			for (d = 0; d < USB_MAX_DEVICE;d++) 
			{
				dev = usb_get_dev_index(d);
				//match hub device and port number
				if((dev->config.if_desc[0].bInterfaceClass==USB_CLASS_HUB)&&(dev->portnum == u4Port)){
				{
					usb_control_msg(dev, 
									usb_sndctrlpipe(dev, 0),
									USB_REQ_SET_FEATURE, 
									USB_RT_PORT,
									USB_PORT_FEAT_TEST, 
									(USB_PORT_TEST_K<<8)|(DownPort),
									NULL, 
									0, 
									USB_CNTL_TIMEOUT);
				}
				printf("USB port%d: Set Test K...OK !\n",u4Port);
				}
			}
		}
	}
}

//-------------------------------------------------------------------------
/** usb_sendtestpacket
 *  send test packet.
 *  @void.
 *  @retval 0	Success
 *  @retval Others	Fail
 */
//-------------------------------------------------------------------------
int usb_sendtestpacket(void)
{
	UINT32 u4Port,d;
	struct usb_device *dev;

	printf("===>Enter usb_sendtestpacket .\n");
	for(u4Port=0; u4Port<(sizeof(mtkusbport) / sizeof(MTKUSBPORT)); u4Port++)
	{

#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5891)
        UINT32 u4tmp;
        if ((u4Port==2) && (xhci_con==1))
        {
            u4tmp = XHCI_REG_READ32(0x430);
            u4tmp &= ~(0x1 << 9);
            XHCI_REG_WRITE32(0x430, u4tmp);
            mdelay(10);
            u4tmp = XHCI_REG_READ32(0x434);
            u4tmp &= ~(0xf << 28);
            u4tmp |= (0x4 << 28);
            XHCI_REG_WRITE32(0x434, u4tmp);
            printf("USB port2: Sending test packet...OK !\n");
            return 0;            
        }
#endif

		if (mtkusbport[u4Port].u4Insert)
		{
			for (d = 0; d < USB_MAX_DEVICE;d++) 
			{
				dev = usb_get_dev_index(d);

				if(((dev->config.if_desc[0].bInterfaceClass==USB_CLASS_MASS_STORAGE) || (dev->config.if_desc[0].bInterfaceClass==USB_CLASS_HID))&&(dev->portnum == u4Port)){

				if ( u4Port >= (sizeof(mtkusbport) / sizeof(MTKUSBPORT)) )
				{
					return -1;	  
				}
				MOTG_REG_WRITE8(M_REG_INDEX, 0);
				// 53 bytes.
				MUC_WriteFifo(u4Port, (UINT8 *)_aTestPacket, 0, 53);
				//set test packet.
				MOTG_REG_WRITE8(0xF, 0x08);
				MOTG_REG_WRITE8(M_REG_CSR0, M_CSR0_TXPKTRDY);
				printf("USB: Port %d Sending test packet...OK !\n",u4Port);
				return 0;	

				}
			}
		}
	}
}

int usb_hub_sendtestpacket(char *argv)
{
	int DownPort,d;
	struct usb_device *dev;
	UINT32 u4Port;

	if (strncmp(argv, "1", 1) == 0)
		DownPort=1;
	else if(strncmp(argv, "2", 1) == 0)
		DownPort=2;
	else if(strncmp(argv, "3", 1) == 0)
		DownPort=3;
	else if(strncmp(argv, "4", 1) == 0)
		DownPort=4;
	else{ 
		printf("Parameter error\n");
		return -1;
	}

	for(u4Port=0; u4Port<(sizeof(mtkusbport) / sizeof(MTKUSBPORT)); u4Port++)
	{
		// device on port 
		if (mtkusbport[u4Port].u4Insert)
		{
			//search device 
			for (d = 0; d < USB_MAX_DEVICE;d++) 
			{
				dev = usb_get_dev_index(d);
				//match hub device and port number
				if((dev->config.if_desc[0].bInterfaceClass==USB_CLASS_HUB)&&(dev->portnum == u4Port)){
				{
					usb_control_msg(dev, 
									usb_sndctrlpipe(dev, 0),
									USB_REQ_SET_FEATURE, 
									USB_RT_PORT,
									USB_PORT_FEAT_TEST, 
									(USB_PORT_TEST_PACKET<<8)|(DownPort),
									NULL, 
									0, 
									USB_CNTL_TIMEOUT);
				}
				printf("USB port%d: Set Test PKT...OK !\n",u4Port);
				}
			}
		}
	}
}




/////////////////////////////////////////////////////////


int usb_hub_sendslewratepacket(char *onoff,char *argv)
{
	int DownPort,d;
	struct usb_device *dev;
	UINT32 u4Port;
    int ret;
    

	if (strncmp(argv, "1", 1) == 0)
		DownPort=1;
	else if(strncmp(argv, "2", 1) == 0)
		DownPort=2;
	else if(strncmp(argv, "3", 1) == 0)
		DownPort=3;
	else if(strncmp(argv, "4", 1) == 0)
		DownPort=4;
	else{ 
		printf("Parameter error\n");
		return -1;
	}

	if (strncmp(onoff, "1", 1) == 0)
        {
        AdjustIndex |= 1<<(DownPort-1+4+8);
        }
    else
        {
        AdjustIndex &= ~(1<<(DownPort-1+4+8));
        }

	for(u4Port=0; u4Port<(sizeof(mtkusbport) / sizeof(MTKUSBPORT)); u4Port++)
	{
		// device on port 
		if (mtkusbport[u4Port].u4Insert)
		{
			//search device 
			for (d = 0; d < USB_MAX_DEVICE;d++) 
			{
				dev = usb_get_dev_index(d);
				//match hub device and port number
				if((dev->config.if_desc[0].bInterfaceClass==USB_CLASS_HUB)&&(dev->portnum == u4Port))
				{
					ret = usb_control_msg(dev, 
									usb_sndctrlpipe(dev, 0),
									0xE3, 
									0x40,
									0x04, 
									AdjustIndex,
									NULL, 
									0, 
									USB_CNTL_TIMEOUT);


                    if(ret != -1)
                        printf("USB port%d: Set slewrate...OK !\n",u4Port);

                    return 0;
				}
			}
		}
	}
}




int usb_hub_sendjklevelpacket(char *onoff,char *argv)
{
    int DownPort,d,enhance;
    struct usb_device *dev;
    UINT32 u4Port;
    int ret;

    
    if (strncmp(argv, "1", 1) == 0)
        DownPort=1;
    else if(strncmp(argv, "2", 1) == 0)
        DownPort=2;
    else if(strncmp(argv, "3", 1) == 0)
        DownPort=3;
    else if(strncmp(argv, "4", 1) == 0)
        DownPort=4;
    else{ 
        printf("Parameter error\n");
        return -1;
    }


	if (strncmp(onoff, "1", 1) == 0)
        {
        AdjustIndex |= 1<<(DownPort-1+4+8);
        }
    else
        {
        AdjustIndex &= ~(1<<(DownPort-1+4+8));
        }
    

    for(u4Port=0; u4Port<(sizeof(mtkusbport) / sizeof(MTKUSBPORT)); u4Port++)
    {
        // device on port 
        if (mtkusbport[u4Port].u4Insert)
        {
            //search device 
            for (d = 0; d < USB_MAX_DEVICE;d++) 
            {
                dev = usb_get_dev_index(d);
                //match hub device and port number
                if((dev->config.if_desc[0].bInterfaceClass==USB_CLASS_HUB)&&(dev->portnum == u4Port))
                {
                    ret = usb_control_msg(dev, 
                                    usb_sndctrlpipe(dev, 0),
                                    0xE3, 
                                    0x40,
                                    0x04, 
                                    AdjustIndex,
                                    NULL, 
                                    0, 
                                    USB_CNTL_TIMEOUT);
    mdelay(1);
                    if(ret != -1)
                        printf("USB port%d: Set JK level...OK !\n",u4Port);
                
                    return 0;
                }
            }
        }
    }
}





//-------------------------------------------------------------------------
/** usb_speed
 *  usb module init speed.
 *  @param  speed = TRUE: high speed enable, FALSE: only Full speed enable.
 *  @retval 0	Success
 *  @retval Others	Fail
 */
//-------------------------------------------------------------------------
int usb_speed(int speed)
{
        UINT32 u4Port;
        for(u4Port=0; u4Port<(sizeof(mtkusbport) / sizeof(MTKUSBPORT)); u4Port++)
        {
            mtkusbport[u4Port].u4HostSpeed = speed;
        }

        printf("Set speed = %s.\n", ((speed == HS_ON) ? "High speed" : "Full speed"));

        return 0;            
}
//-------------------------------------------------------------------------
/** usb_lowlevel_init
 *  usb module init.
 *  @param  void.
 *  @retval 0	Success
 *  @retval Others	Fail
 */
//-------------------------------------------------------------------------
int usb_lowlevel_init(void)
{
    UINT32 u4Port;

    for(u4Port=0; u4Port<(sizeof(mtkusbport) / sizeof(MTKUSBPORT)); u4Port++)
    {
        if (!mtkusbport[u4Port].u4Insert)
        {
            if ((MUC_Initial(u4Port) == 0) && (MUC_DevInsert(u4Port) == 0))
            {
                printf("USB: Device on port %d.\n", (int)u4Port);
                mtkusbport[u4Port].u4Insert = 1;
         //       return 0;            
            }
#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5891)
            else if (u4Port==2)
            {
                if(SSUSB_check_connect())
                {
        		    printf("USB: device on port %d.\n", (int)u4Port);
        		    return 0;
                }
                else
                    printf("USB: No Device on prot %d.\n", (int)u4Port);
            }
#endif
            else
            {
                printf("USB: No Device on prot %d.\n", (int)u4Port);
            }
        }
	else
	{
		printf("USB: device on port %d.\n", (int)u4Port);
		//return 0;
	}
    }
    return 0;
}


#if defined(CONFIG_ARCH_MT5882)
#define MUSB_MT5885_PORT3_PHYBASE		(0x1000)
#endif
int usb_lowlevel_init_with_port_num(int iPortNum)
{
    UINT32 u4Port = (UINT32)iPortNum;

// mt5885 portD PHYBASE 0x5a800 = 0x59800 + 0x01000
#if defined(CONFIG_ARCH_MT5882)
	if(IS_IC_5885() && (u4Port == 3))
		{
		mtkusbport[3].pPhyBase = (void*)(MUSB_BASE + MUSB_PHYBASE+ MUSB_MT5885_PORT3_PHYBASE);
		printf("USB: MT5885 Port3 MAC Base Addr = %p\n", mtkusbport[3].pBase);
		printf("USB: MT5885 Port3 Phy Base Addr = %p\n", mtkusbport[3].pPhyBase);
	}
#endif

    if (!mtkusbport[u4Port].u4Insert)
    {
        if ((MUC_Initial(u4Port) == 0) && (MUC_DevInsert(u4Port) == 0))
        {
            printf("USB: Device on port %d.\n", (int)u4Port);
            mtkusbport[u4Port].u4Insert = 1;
            return 0;            
        }
        else
        {
            printf("USB: No Device on prot %d.\n", (int)u4Port);
        }
    }
    else
    {
	printf("USB: device on port %d.\n", (int)u4Port);
	return -1;
    }
    return -1;
}
//-------------------------------------------------------------------------
/** usb_silent_reset
 *  usb silent reset device.
 *  @param  void.
 *  @retval 0	Success
 *  @retval Others	Fail
 */
//-------------------------------------------------------------------------
int usb_silent_reset(void)
{
#if 0
    volatile UINT32 reg = 0; /* read USB registers into this */

    // try to reset device.
    reg = MOTG_REG_READ8(M_REG_POWER);
    reg |= M_POWER_RESET;
    MOTG_REG_WRITE8(M_REG_POWER, reg);

    // reset device address.
    MOTG_REG_WRITE8(M_REG_FADDR, 0);
    MOTG_REG_WRITE32(M_REG_EP0ADDR, 0);
    MOTG_REG_WRITE32(M_REG_EP1ADDR, 0);
    MOTG_REG_WRITE32(M_REG_EP2ADDR, 0);
    MOTG_REG_WRITE32(M_REG_EP3ADDR, 0);
    MOTG_REG_WRITE32(M_REG_EP4ADDR, 0);

    mdelay(100);

    // clear reset.
    reg = MOTG_REG_READ8(M_REG_POWER);
    reg &= ~M_POWER_RESET;
    MOTG_REG_WRITE8(M_REG_POWER, reg);

    // check device speed: LS, FS, HS.
    reg = MOTG_REG_READ8(M_REG_DEVCTL);
    if (reg & M_DEVCTL_LSDEV)
    {
        printf("USB Silent Reset: LS device.\n");
        _u4DeviceSpeed = MUSB_CONNECTION_SPEED_LOW;
    }
    else
    {
        reg = MOTG_REG_READ8(M_REG_POWER);
        if (reg & M_POWER_HSMODE)
        {
            printf("USB Silent Reset: HS device.\n");
            _u4DeviceSpeed = MUSB_CONNECTION_SPEED_HIGH;
        }
        else
        {
            printf("USB Silent Reset: FS device.\n");
            _u4DeviceSpeed = MUSB_CONNECTION_SPEED_FULL;
        }
    }

    _u4Insert = TRUE;
#endif

    return 0;
}

//-------------------------------------------------------------------------
/** usb_lowlevel_stop
 *  usb module stop.
 *  @param  void.
 *  @retval 0	Success
 *  @retval Others	Fail
 */
//-------------------------------------------------------------------------
int usb_lowlevel_stop(void)
{
    UINT32 u4Port;

    for(u4Port=0; u4Port<(sizeof(mtkusbport) / sizeof(MTKUSBPORT)); u4Port++)
    {
        if (mtkusbport[u4Port].u4Insert)
        {
            printf("USB: Stop device on port %d.\n", (int)u4Port);
            mtkusbport[u4Port].u4Insert = 0;
        }
    }

    return 0;
}

//-------------------------------------------------------------------------
/** submit_bulk_msg
 *  submit bulk transfer.
 *  @param  prDev: point to struct usb_device.
 *  @param  u4Pipe: usb u4Pipe.
 *  @param  puBuf: point to data buffer.
 *  @param  len: data buffer length.
 *  @retval 0	Success
 *  @retval Others	Fail
 */
//-------------------------------------------------------------------------
int submit_bulk_msg(struct usb_device *prDev, unsigned long u4Pipe, void *pvBuf,
		    int i4Len)
{
        INT32 i4Ret = 0;
        UINT32 u4Port = (UINT32)prDev->portnum;

        if (!mtkusbport[u4Port].u4Insert)
        {
            prDev->act_len = 0;
            prDev->status = USB_ST_BIT_ERR;
            return -1;
        }       

	LOG(7, "prDev = %ld u4Pipe = %ld buf = %p size = %d dir_out = %d\n",
	       usb_pipedevice(u4Pipe), usb_pipeendpoint(u4Pipe), pvBuf, i4Len, usb_pipeout(u4Pipe));

	prDev->status = 0;
        prDev->act_len = 0;
        if ((i4Len > 0) && (pvBuf))
        {
            if (usb_pipein(u4Pipe))
            {
                i4Ret = MUC_InPkt(prDev, u4Pipe, pvBuf, i4Len);
            }
            else
            {
                i4Ret = MUC_OutPkt(prDev, u4Pipe, pvBuf, i4Len);
                prDev->act_len = i4Len;                
            }
            
            if (i4Ret < 0)
            {
                prDev->act_len = 0;
                return i4Ret;    
            }
        }

        return i4Ret;
}

//-------------------------------------------------------------------------
/** submit_control_msg
 *  submit control transfer.
 *  @param  prDev: point to struct usb_device.
 *  @param  u4Pipe: usb u4Pipe.
 *  @param  puBuf: point to data buffer.
 *  @param  len: data buffer length.
 *  @param  setup: point to struct devrequest.
 *  @retval 0	Success
 *  @retval Others	Fail
 */
//-------------------------------------------------------------------------
int submit_control_msg(struct usb_device *prDev, unsigned long u4Pipe, void *pvBuf,
		       int i4Len, struct devrequest *prSetup)
{
        INT32 i4Ret = 0;
        UINT32 u4Port = (UINT32)prDev->portnum;

        if (!mtkusbport[u4Port].u4Insert)
        {
            prDev->act_len = 0;
            prDev->status = USB_ST_BIT_ERR;
            return -1;
        }       

	prDev->status = 0;

	LOG(7, "prDev = %d u4Pipe = %ld buf = %p size = %d rt = %#x req = %#x.\n",
	       usb_pipedevice(u4Pipe), usb_pipeendpoint(u4Pipe), 
	       pvBuf, i4Len, (int)prSetup->requesttype, (int)prSetup->request);

        /* setup phase */
	if (MUC_SetupPkt(prDev, u4Pipe, prSetup) == 0)
        {
	    /* data phase */
            if ((i4Len > 0) && (pvBuf))
            {
                // wait device prepare ok.
                mdelay(10);
            
                if (usb_pipein(u4Pipe))
                {
                    i4Ret = MUC_InPkt(prDev, u4Pipe, pvBuf, i4Len);
                }
                else
                {
                    i4Ret = MUC_OutPkt(prDev, u4Pipe, pvBuf, i4Len);
                }
                
                if (i4Ret < 0)
                {
                    printf("data phase failed!\n");
                    return i4Ret;    
                }
            }

            // wait device prepare ok.
            mdelay(10);

            /* status phase */
            i4Ret = MUC_StatusPkt(prDev, u4Pipe);
            if (i4Ret < 0)
            {
                printf("status phase failed!\n");
                return i4Ret;    
            }
	} 
	else 
	{
            printf("setup phase failed!\n");
	}

        // wait device prepare ok.
        mdelay(10);

	prDev->act_len = i4Len;

	return i4Len;
}

//-------------------------------------------------------------------------
/** submit_int_msg
 *  submit interrupt transfer.
 *  @param  prDev: point to struct usb_device.
 *  @param  u4Pipe: usb u4Pipe.
 *  @param  puBuf: point to data buffer.
 *  @param  len: data buffer length.
 *  @param  interval: interrupt interval.
 *  @retval 0	Success
 *  @retval Others	Fail
 */
//-------------------------------------------------------------------------
int submit_int_msg(struct usb_device *prDev, unsigned long u4Pipe, void *puBuf,
		   int len, int interval)
{
	printf("prDev = %p u4Pipe = %#lx buf = %p size = %d int = %d\n", prDev, u4Pipe,
	       puBuf, len, interval);
	return -1;
}

#endif	/* CONFIG_USB_MTKHCD */
