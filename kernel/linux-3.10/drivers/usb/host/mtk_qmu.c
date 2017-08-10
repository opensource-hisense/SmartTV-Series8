/*-----------------------------------------------------------------------------
 * drivers/usb/musb_qmu/mtk_qmu.c
 *
 * MT53xx USB driver
 *
 * Copyright (c) 2008-2012 MediaTek Inc.
 * $Author: cheng.yuan $
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

#include "mtk_qmu_api.h"
#include "mtk_cust.h"
#ifdef MUSB_DEBUG
extern int mgc_debuglevel;
#endif 

extern unsigned long irq_flags;//added by zile.pang



#ifdef CONFIG_USB_QMU_SUPPORT

static uint8_t QMU_pdu_checksum(uint8_t *data, uint8_t len)
{
    uint8_t *uDataPtr, ckSum;

    int i;
    *(data + 1) = 0x0;
    uDataPtr = data;
    ckSum = 0;

    for (i = 0; i < len; i++)
        ckSum += *(uDataPtr + i);

    return 0xFF - ckSum;
}

static void  QMU_flush_q(MGC_LinuxCd *pThis, uint8_t bEnd, uint8_t isRx)
{
    void __iomem *pBase;
    uint16_t wIntEnable;
    uint32_t dwIntr;

    if (!pThis)
    {
        INFO("QMU_flush_q: pThis NULL !\n");
        return;
    }

    pBase = (void __iomem *)pThis->pRegs;

    if (!pBase)
    {
        INFO("QMU_flush_q: pBase NULL !\n");
        return;
    }

    if (isRx)
    {
        /* write 2x to allow double buffering */
        //CC: see if some check is necessary
        MGC_WriteCsr16(pBase, MGC_O_HDRC_RXCSR, bEnd, MGC_M_RXCSR_FLUSHFIFO|MGC_M_RXCSR_CLRDATATOG);
        MGC_WriteCsr16(pBase, MGC_O_HDRC_RXCSR, bEnd, MGC_M_RXCSR_FLUSHFIFO|MGC_M_RXCSR_CLRDATATOG);
        
        MGC_Write16(pBase, MGC_O_HDRC_INTRRX, (1 << bEnd));

        // Enable endpoint interrupt after flush fifo.
        /*
            IntrRxE, IntrUSB, and IntrUSBE are the same 32 bits group.
            Tricky: Set 0 in all write clear field in IntrUSB field. Prevent to clear IntrUSB.
            Because our 32 bits register access mode.            
        */
        dwIntr = MGC_Read32(pBase, MGC_O_HDRC_INTRRXE);
        dwIntr |= ((ulong)(1 << bEnd));
        dwIntr &= 0xFF00FFFE;
        MGC_Write32(pBase, MGC_O_HDRC_INTRRXE, (ulong)dwIntr);
    }
    else
    {
        MGC_WriteCsr16(pBase, MGC_O_HDRC_TXCSR, bEnd, MGC_M_TXCSR_FLUSHFIFO | MGC_M_TXCSR_CLRDATATOG);
        MGC_WriteCsr16(pBase, MGC_O_HDRC_TXCSR, bEnd, MGC_M_TXCSR_FLUSHFIFO | MGC_M_TXCSR_CLRDATATOG);
        
        MGC_Write16(pBase, MGC_O_HDRC_INTRTX, (1 << bEnd));

        // Enable endpoint interrupt after flush fifo.
        /*
            IntrTxE, and IntrRx are the same 32 bits group.
            Tricky: Set 0 in all write clear field in IntrRx field. Prevent to clear IntrRx.
            Because our 32 bits register access mode.
        */
        wIntEnable = MGC_Read16(pBase, MGC_O_HDRC_INTRTXE);
        dwIntr = (ulong)((wIntEnable | ((uint16_t)(1 << bEnd))) << 16);
        dwIntr &= 0xFFFF0000;
        MGC_Write32(pBase, MGC_O_HDRC_INTRRX, dwIntr);        
    }
}

/*
    Select DMA channel for command queue.
*/
void QMU_select_dma_ch(MGC_LinuxCd *pThis, uint8_t channel, uint8_t burstmode)
{
    void __iomem *pBase = (void __iomem *)pThis->pRegs;
    uint32_t ctrl;

    ctrl = MGC_Read16(pBase, MUSB_HSDMA_CFG);

    MGC_Write16(pBase, MUSB_HSDMA_CFG, ctrl | channel << 4);

    //burst mode
    ctrl = MGC_Read16(pBase, MGC_HSDMA_CHANNEL_OFFSET(channel, MGC_O_HSDMA_CONTROL));

    MGC_Write16(pBase, MGC_HSDMA_CHANNEL_OFFSET(channel, MGC_O_HSDMA_CONTROL), ctrl | burstmode << 9);
}

void QMU_disable_q(MGC_LinuxCd *pThis, uint8_t bEnd, uint8_t isRx)
{
    void __iomem *pBase = (void __iomem *)pThis->pRegs;

    QMU_disable(pThis, bEnd, isRx);

    if (isRx)
    {
        MGC_WriteCsr16(pBase, MGC_O_HDRC_RXCSR, bEnd, 0);
    }
    else
    {
        MGC_WriteCsr16(pBase, MGC_O_HDRC_TXCSR, bEnd, 0);
    }
    
    QMU_flush_q(pThis, bEnd, isRx);
}

void QMU_disable_all_q(MGC_LinuxCd *pThis)
{
    uint32_t bEnd = 0;

    for (bEnd = 1; bEnd <= RXQ_NUM; bEnd++)
    {
        if(QMU_is_enabled(pThis,bEnd, 1))
            QMU_disable_q(pThis, bEnd, 1);
    }

    for (bEnd = 1; bEnd <= TXQ_NUM; bEnd++)
    {
        if(QMU_is_enabled(pThis,bEnd, 0))
            QMU_disable_q(pThis, bEnd, 0);
    }
}

/// @brief Prepare Rx GPD, programming the last NULL gpd, and insert a New NULL GPD to the next
/// .
/// @param gpd: the last NULL gpd
/// @param pBuf: the receive buffer pointer
/// @param data_len: the allow length of buffer
/// @param EP_Num: Endpoint number mapping Queue
/// @return the prepare gpd.
static int QMU_set_gpd(MGC_LinuxCd *pThis, uint32_t pDataBuf_addr, uint32_t data_len, uint8_t EP_Num, uint8_t isIOC, uint8_t IsRx)
{
    Q_Port_Info *pQ = &pThis->rCmdQ;
    Q_Buf_Info *pBuf;
    TGPD *pWGpd;
	uint8_t EP_Q_Num = IsRx? RXQ_NUM : TXQ_NUM;

    if (EP_Num < 1)
    {
        ERR("ERROR: EP_Num < 1.\n");        
        return USB_ST_NOMEMORY;
    }

    if ((EP_Num - 1) >= EP_Q_Num) 
    {
        ERR("ERROR: (EP_Num - 1) overflow.\n");        
        return USB_ST_NOMEMORY;
    }
       
    pBuf = (IsRx) ? (&pQ->rRx[EP_Num - 1]) : (&pQ->rTx[EP_Num - 1]);

    if (Buf_IsFull(pBuf))
    {
        ERR("rx_gpd_container full!\n");
        return USB_ST_NOMEMORY;
    }

    pWGpd = (TGPD *)phys_to_virt(Buf_GetWritePhyAddr(pBuf));

    if (TGPD_GET_FLAG(pWGpd))
    {
        INFO("QMU_GPD: HWO is on !\n");
    }

    memset(pWGpd, 0, sizeof(TGPD));
    
    Buf_IncWrite(pBuf);
   
    TGPD_SET_DATA(pWGpd, pDataBuf_addr);

	DBG(5,"\033[0;34m function and line[%s][%d] pWGpd = 0x%p pBuf = 0x%p pWGpd->pBuf_addr = 0x%x Buf_GetWritePhyAddr(pBuf) = 0x%x \033[0m\n",__FUNCTION__,__LINE__,pWGpd, pBuf, pWGpd->pBuf_addr,(Buf_GetWritePhyAddr(pBuf)));
    TGPD_SET_NEXT(pWGpd, Buf_GetWritePhyAddr(pBuf));

    if (!IsRx)
    {
        // Do not support buffer descriptor.
		TGPD_CLR_FORMAT_BDP(pWGpd);
		TGPD_SET_BUF_LEN(pWGpd, data_len);
    }
    else
    {
        TGPD_SET_DataBUF_LEN(pWGpd, data_len);
    }
    
    if (isIOC)
    {
        TGPD_SET_IOC(pWGpd);
    }

    TGPD_SET_CHKSUM_HWO(pWGpd, 16);

    /*
        After the firmware has finished the preparation of the linked list of the Rx GPDs 
        for the reception of Rx packets, the firmware shall set this bit in each Rx GPD 
        to 1 to specify the ownership of the Rx GPD changed to the HIF Controller and 
        then start or resume the corresponding RxQ.
    */
    TGPD_SET_FLAGS_HWO(pWGpd);

    // Map to read dram for h/w decode.
    (void)dma_map_single(pThis->dev, (void *)pWGpd, pBuf->size, DMA_TO_DEVICE);

    return 0;
}

/// @brief resume the selected Queue
/// .
/// @param EP_Num: Endpoint number mapping Queue
/// @param isRx: RxQ or TxQ
/// @return status
static int QMU_resume(void __iomem *base, uint8_t EP_Num, uint8_t isRx)
{
    if (!isRx)
    {
        MGC_WriteQMU32(base, MGC_O_QMU_TQCSR(EP_Num), DQMU_QUE_RESUME);
    }
    else
    {
        MGC_WriteQMU32(base, MGC_O_QMU_RQCSR(EP_Num), DQMU_QUE_RESUME);
    }

    return 0;
}

int QMU_is_enabled(MGC_LinuxCd *pThis, uint8_t EP_Num, uint8_t isRx)
{
    void __iomem *base = (void __iomem *)pThis->pRegs;

    if (isRx)
    {
        if (MGC_ReadQUCS32(base, MGC_O_QUCS_USBGCSR) & (USB_QMU_Rx_EN(EP_Num)))
            return 1;
    }
    else
    {
        if (MGC_ReadQUCS32(base, MGC_O_QUCS_USBGCSR) & (USB_QMU_Tx_EN(EP_Num)))
            return 1;
    }

    return 0;
}

int QMU_clean(MGC_LinuxCd *pThis)
{   
    uint32_t i;
    Q_Port_Info *pQ = &pThis->rCmdQ;
    Q_Buf_Info *pBuf;
    
    if (!pThis->bSupportCmdQ)
    {
        return 0;
    }

    for (i=0; i<RXQ_NUM; i++)
    {
        pBuf = &pQ->rRx[i];

        if (pBuf->pLogicalBuf)
        {
            kfree(pBuf->pLogicalBuf);
            pBuf->pLogicalBuf = NULL;
        }
        
        pBuf->start = 0;       
        pBuf->read = 0;
        pBuf->write = 0;
        pBuf->size = GPD_LEN_ALIGNED;
        pBuf->length = MAX_URB_NUM;
    }

    for (i=0; i<TXQ_NUM; i++)
    {
        pBuf = &pQ->rTx[i];

        if (pBuf->pLogicalBuf)
        {
            kfree(pBuf->pLogicalBuf);
            pBuf->pLogicalBuf = NULL;
        }
        
        pBuf->start = 0;        
        pBuf->read = 0;
        pBuf->write = 0;
        pBuf->size = GPD_LEN_ALIGNED;
        pBuf->length = MAX_URB_NUM;
    }    

    return 0;
}

int QMU_init(MGC_LinuxCd *pThis)
{   
    uint32_t i;
    Q_Port_Info *pQ = &pThis->rCmdQ;
    Q_Buf_Info *pBuf;
    
    if (!pThis->bSupportCmdQ)
    {
        return 0;
    }

    for (i=0; i<RXQ_NUM; i++)
    {
        pBuf = &pQ->rRx[i];
        
        pBuf->pLogicalBuf = 
            (void *)kzalloc((GPD_LEN_ALIGNED *MAX_URB_NUM), GFP_ATOMIC |GFP_DMA);
        
        if (!pBuf->pLogicalBuf)
        {
            ERR("pBuf->pBuffer kzalloc failed\n");
            return USB_ST_NOMEMORY;
        }
    
        pBuf->start = (ulong)dma_map_single(pThis->dev, pBuf->pLogicalBuf, 
            (GPD_LEN_ALIGNED *MAX_URB_NUM), DMA_TO_DEVICE);

        dma_unmap_single(pThis->dev, (dma_addr_t)pBuf->start, 
            (GPD_LEN_ALIGNED *MAX_URB_NUM), DMA_FROM_DEVICE);
        
        pBuf->read = 0;
        pBuf->write = 0;
        pBuf->size = GPD_LEN_ALIGNED;
        pBuf->length = MAX_URB_NUM;
    }

    for (i=0; i<TXQ_NUM; i++)
    {
        pBuf = &pQ->rTx[i];
        
        pBuf->pLogicalBuf = 
            (void *)kzalloc((GPD_LEN_ALIGNED *MAX_URB_NUM), GFP_ATOMIC |GFP_DMA);
        
        if (!pBuf->pLogicalBuf)
        {
            INFO("pBuf->pBuffer kzalloc failed\n");
            return USB_ST_NOMEMORY;
        }

        pBuf->start = (ulong)dma_map_single(pThis->dev, pBuf->pLogicalBuf, 
            (GPD_LEN_ALIGNED *MAX_URB_NUM), DMA_TO_DEVICE);

        dma_unmap_single(pThis->dev, (dma_addr_t)pBuf->start, 
            (GPD_LEN_ALIGNED *MAX_URB_NUM), DMA_FROM_DEVICE);
        
        pBuf->read = 0;
        pBuf->write = 0;
        pBuf->size = GPD_LEN_ALIGNED;
        pBuf->length = MAX_URB_NUM;
    }    

    return 0;
}

int QMU_enable(MGC_LinuxCd *pThis, uint8_t EP_Num, uint8_t isRx, uint8_t isZLP, uint8_t isCSCheck, uint8_t isEmptyCheck)
{
    void __iomem *base = (void __iomem *)pThis->pRegs;
    uint32_t QCR;
    Q_Port_Info *pQ = &pThis->rCmdQ;
    Q_Buf_Info *pBuf;
	uint8_t EP_Q_Num = isRx? RXQ_NUM : TXQ_NUM;

    if (EP_Num < 1)
    {
        ERR( "ERROR: EP_Num < 1.\n");        
        return USB_ST_NOMEMORY;
    }
   
    if ((EP_Num - 1) >= EP_Q_Num)
    {
        ERR("ERROR: (EP_Num - 1) overflow.\n");        
        return USB_ST_NOMEMORY;
    }

    if (isRx)
    {
        // Init software data structure.
        pBuf = &pQ->rRx[EP_Num - 1];
        pBuf->read = 0;
        pBuf->write = 0;

        MGC_WriteQMU32(base, MGC_O_QMU_RQSAR(EP_Num), pBuf->start);

        MGC_WriteQIRQ32(base, MGC_O_QIRQ_IOCDISR,
            MGC_ReadQIRQ32(base, MGC_O_QIRQ_IOCDISR) & ~(DQMU_M_RQ_DIS_IOC(EP_Num)));

        MGC_WriteQUCS32(base, MGC_O_QUCS_USBGCSR,
            MGC_ReadQUCS32(base, MGC_O_QUCS_USBGCSR) | (USB_QMU_Rx_EN(EP_Num)));

        if (isCSCheck)
        {
            QCR = MGC_ReadQMU32(base, MGC_O_QMU_QCR0);
            MGC_WriteQMU32(base, MGC_O_QMU_QCR0, QCR | DQMU_RQCS_EN(EP_Num));
        }

        if (isZLP)
        {
            QCR = MGC_ReadQMU32(base, MGC_O_QMU_QCR3);
            MGC_WriteQMU32(base, MGC_O_QMU_QCR3, QCR | DQMU_RX_ZLP(EP_Num));
        }

        // Set data receive ZLP and jump to next GPD configuration.
        QCR = MGC_ReadQMU32(base, MGC_O_QMU_QCR3);
        MGC_WriteQMU32(base, MGC_O_QMU_QCR3, QCR | DQMU_RX_COZ(EP_Num));        

        MGC_WriteQIRQ32(base, MGC_O_QIRQ_QIMCR,
            DQMU_M_RX_DONE(EP_Num) | DQMU_M_RQ_EMPTY | DQMU_M_RXQ_ERR | DQMU_M_RXEP_ERR);

        if (isEmptyCheck)
        {
            MGC_WriteQIRQ32(base, MGC_O_QIRQ_REPEMPMCR, DQMU_M_RX_EMPTY(EP_Num));
        }

        QCR = DQMU_M_RX_LEN_ERR(EP_Num);
        QCR |= isCSCheck ? DQMU_M_RX_GPDCS_ERR(EP_Num) : 0;
        QCR |= isZLP ? DQMU_M_RX_ZLP_ERR(EP_Num) : 0;
        MGC_WriteQIRQ32(base, MGC_O_QIRQ_RQEIMCR, QCR);
        MGC_WriteQIRQ32(base, MGC_O_QIRQ_REPEIMCR, DQMU_M_RX_EP_ERR(EP_Num));

        MGC_WriteQMU32(base, MGC_O_QMU_RQCSR(EP_Num), DQMU_QUE_START);
    }
    else
    {
        // Init software data structure.
        pBuf = &pQ->rTx[EP_Num - 1];
        pBuf->read = 0;
        pBuf->write = 0;

        MGC_WriteQMU32(base, MGC_O_QMU_TQSAR(EP_Num), pBuf->start);

        MGC_WriteQIRQ32(base, MGC_O_QIRQ_IOCDISR,
                        MGC_ReadQIRQ32(base,
                                       MGC_O_QIRQ_IOCDISR) & ~(DQMU_M_TQ_DIS_IOC(EP_Num)));

        MGC_WriteQUCS32(base, MGC_O_QUCS_USBGCSR,
                        MGC_ReadQUCS32(base,
                                       MGC_O_QUCS_USBGCSR) | (USB_QMU_Tx_EN(EP_Num)));

        if (isCSCheck)
        {
            QCR = MGC_ReadQMU32(base, MGC_O_QMU_QCR0);
            MGC_WriteQMU32(base, MGC_O_QMU_QCR0,
                           QCR | DQMU_TQCS_EN(EP_Num));
        }

        if (isZLP)
        {
            QCR = MGC_ReadQMU32(base, MGC_O_QMU_QCR2);
            MGC_WriteQMU32(base, MGC_O_QMU_QCR2,
                           QCR | DQMU_TX_ZLP(EP_Num));
        }

        MGC_WriteQIRQ32(base, MGC_O_QIRQ_QIMCR,
                        DQMU_M_TX_DONE(EP_Num) | DQMU_M_TQ_EMPTY | DQMU_M_TXQ_ERR | DQMU_M_TXEP_ERR);

        if (isEmptyCheck)
        {
            MGC_WriteQIRQ32(base, MGC_O_QIRQ_TEPEMPMCR,
                            DQMU_M_TX_EMPTY(EP_Num));
        }

        QCR = DQMU_M_TX_LEN_ERR(EP_Num);
        QCR |= isCSCheck ? DQMU_M_TX_GPDCS_ERR(EP_Num) | DQMU_M_TX_BDCS_ERR(EP_Num) : 0;
        MGC_WriteQIRQ32(base, MGC_O_QIRQ_TQEIMCR, QCR);
        MGC_WriteQIRQ32(base, MGC_O_QIRQ_TEPEIMCR,
                        DQMU_M_TX_EP_ERR(EP_Num));

        MGC_WriteQMU32(base, MGC_O_QMU_TQCSR(EP_Num), DQMU_QUE_START);
    }

    return 0;
}

void QMU_disable(MGC_LinuxCd *pThis, uint8_t EP_Num, uint8_t isRx)
{
    void __iomem *base = (void __iomem *)pThis->pRegs;
    Q_Port_Info *pQ = &pThis->rCmdQ;
    Q_Buf_Info *pBuf;    
    uint32_t QCR;
	uint8_t EP_Q_Num = isRx? RXQ_NUM : TXQ_NUM;

    if (EP_Num < 1)
    {
        ERR("ERROR: EP_Num < 1.\n");        
        return;
    }

    if ((EP_Num - 1) >= EP_Q_Num)
    {
        ERR("ERROR: (EP_Num - 1) overflow.\n");        
        return;
    }

    QMU_stop(pThis, EP_Num, isRx);

    if (isRx)
    {
        MGC_WriteQUCS32(base, MGC_O_QUCS_USBGCSR,
                        MGC_ReadQUCS32(base,
                                       MGC_O_QUCS_USBGCSR) & (~(USB_QMU_Rx_EN(EP_Num))));

        QCR = MGC_ReadQMU32(base, MGC_O_QMU_QCR0);
        MGC_WriteQMU32(base, MGC_O_QMU_QCR0,
                       QCR&(~(DQMU_RQCS_EN(EP_Num))));

        QCR = MGC_ReadQMU32(base, MGC_O_QMU_QCR3);
        MGC_WriteQMU32(base, MGC_O_QMU_QCR3,
                       QCR&(~(DQMU_RX_ZLP(EP_Num))));

        MGC_WriteQIRQ32(base, MGC_O_QIRQ_QIMSR, DQMU_M_RX_DONE(EP_Num));
        MGC_WriteQIRQ32(base, MGC_O_QIRQ_REPEMPMSR,
                        DQMU_M_RX_EMPTY(EP_Num));

        MGC_WriteQIRQ32(base, MGC_O_QIRQ_RQEIMSR,
                        DQMU_M_RX_LEN_ERR(EP_Num) | DQMU_M_RX_GPDCS_ERR(EP_Num) | DQMU_M_RX_ZLP_ERR(EP_Num));
        MGC_WriteQIRQ32(base, MGC_O_QIRQ_REPEIMSR,
                        DQMU_M_RX_EP_ERR(EP_Num));
    }
    else
    {
        MGC_WriteQUCS32(base, MGC_O_QUCS_USBGCSR,
                        MGC_ReadQUCS32(base,
                                       MGC_O_QUCS_USBGCSR) & (~(USB_QMU_Tx_EN(EP_Num))));

        QCR = MGC_ReadQMU32(base, MGC_O_QMU_QCR0);
        MGC_WriteQMU32(base, MGC_O_QMU_QCR0,
                       QCR&(~(DQMU_TQCS_EN(EP_Num))));

        QCR = MGC_ReadQMU32(base, MGC_O_QMU_QCR2);
        MGC_WriteQMU32(base, MGC_O_QMU_QCR2,
                       QCR&(~(DQMU_TX_ZLP(EP_Num))));

        MGC_WriteQIRQ32(base, MGC_O_QIRQ_QIMSR, DQMU_M_TX_DONE(EP_Num));
        MGC_WriteQIRQ32(base, MGC_O_QIRQ_TEPEMPMSR,
                        DQMU_M_TX_EMPTY(EP_Num));

        MGC_WriteQIRQ32(base, MGC_O_QIRQ_TQEIMSR,
                        DQMU_M_TX_LEN_ERR(EP_Num) | DQMU_M_TX_GPDCS_ERR(EP_Num) | DQMU_M_TX_BDCS_ERR(EP_Num));
        MGC_WriteQIRQ32(base, MGC_O_QIRQ_TEPEIMSR,
                        DQMU_M_TX_EP_ERR(EP_Num));
    }

    pBuf = (isRx) ? (&pQ->rRx[EP_Num - 1]) : (&pQ->rTx[EP_Num - 1]); 

    // Clear command queue in dram.
    memset(pBuf->pLogicalBuf, 0, (pBuf->size*pBuf->length));

    // Map to read dram for h/w decode.
    pBuf->start = (ulong)dma_map_single(pThis->dev, 
        (void *)pBuf->pLogicalBuf, (pBuf->size*pBuf->length), DMA_TO_DEVICE);

    dma_unmap_single(pThis->dev, (dma_addr_t)pBuf->start, 
        (GPD_LEN_ALIGNED *MAX_URB_NUM), DMA_FROM_DEVICE);    
}

int QMU_insert_task(MGC_LinuxCd *pThis, uint8_t EP_Num, uint8_t isRx, uint32_t buf_addr, uint32_t length, uint8_t isIOC)
{
    void __iomem *base = (void __iomem *)pThis->pRegs;
    int status = 0;
    
    //INFO("EP_Num: %d, isRx: %d, buf: %x, length: %d\n", EP_Num, isRx, (uint32_t)buf, length);

    status = QMU_set_gpd(pThis, buf_addr, length, EP_Num, isIOC, isRx);

    if (status == 0)
    {
        QMU_resume(base, EP_Num, isRx);
    }
    
    return status;
}

void QMU_stop(MGC_LinuxCd *pThis, uint8_t EP_Num, uint8_t isRx)
{
    void __iomem *base = (void __iomem *)pThis->pRegs;

    /*
        Note: After set QUE_STOP, 
        must wait until QUE_INACTIVE to continue to clean the GPD or other setting.
    */    

    if (!isRx)
    {
        while (MGC_ReadQMU32(base, MGC_O_QMU_TQCSR(EP_Num)) & DQMU_QUE_ACTIVE)
        {           
            MGC_WriteQMU32(base, MGC_O_QMU_TQCSR(EP_Num), DQMU_QUE_STOP);
            //INFO("Stop Tx Queue %d!\n", EP_Num);
        }
    }
    else
    {
        while (MGC_ReadQMU32(base, MGC_O_QMU_RQCSR(EP_Num)) & DQMU_QUE_ACTIVE)
        {
            MGC_WriteQMU32(base, MGC_O_QMU_RQCSR(EP_Num), DQMU_QUE_STOP);
            //INFO("Stop Rx Queue %d!\n", EP_Num);
        }
    }
}

static void QMU_rx_error(MGC_LinuxCd *pThis, uint8_t bEnd)
{
    struct urb *pUrb;
    uint16_t wRxCount, wRxCsrVal, wVal = 0;
    MGC_LinuxLocalEnd *pEnd;
    uint8_t *pBase = (uint8_t*)pThis->pRegs;
    uint32_t status = 0;

    if (MGC_GET_RX_EPS_NUM(pThis) <= bEnd)
    {
        ERR("Rx[%d]ERROR: bEnd overflow.\n", bEnd);        
        return;
    }

    MGC_SelectEnd(pBase, bEnd);
    pEnd = &(pThis->aLocalEnd[0][bEnd]); /*0: Rx*/
    
    pUrb = MGC_GetCurrentUrb(pEnd);
    if (!pUrb)
    {
        return ;
    } 

    wRxCsrVal = MGC_ReadCsr16(pBase, MGC_O_HDRC_RXCSR, bEnd);
    wVal = wRxCsrVal;
    wRxCount = MGC_ReadCsr16(pBase, MGC_O_HDRC_RXCOUNT, bEnd);

    /* check for errors, concurrent stall & unlink is not really
     * handled yet! */
    if (wRxCsrVal &MGC_M_RXCSR_H_RXSTALL)
    {    	
        INFO("[USB]RX end %d STALL (%d:%d) !\n", bEnd, pEnd->bRemoteAddress, pEnd->bEnd);
        status = USB_ST_STALL;
    }
    else if (wRxCsrVal &MGC_M_RXCSR_H_ERROR)
    {
        INFO("[USB]end %d Rx error (%d:%d) !\n", bEnd, pEnd->bRemoteAddress, pEnd->bEnd);
        status = USB_ST_NORESPONSE;

        /* do the proper sequence to abort the transfer */
        wVal &= ~MGC_M_RXCSR_H_REQPKT;
        MGC_WriteCsr16(pBase, MGC_O_HDRC_RXCSR, bEnd, wVal);
        MGC_WriteCsr8(pBase, MGC_O_HDRC_RXINTERVAL, bEnd, 0);
    }
    else if (wRxCsrVal &MGC_M_RXCSR_DATAERR)
    {
        if (PIPE_BULK == pEnd->bTrafficType)
        {
            /* cover it up if retries not exhausted, slow devices might  
             * not answer quickly enough: I was expecting a packet but the 
             * packet didn't come. The interrupt is generated after 3 failed
             * attempts, it make MUSB_MAX_RETRIESx3 attempts total..
             */
            if (pUrb->status == ( - EINPROGRESS) && ++pEnd->bRetries < MUSB_MAX_RETRIES)
            {
                /* silently ignore it */
                wRxCsrVal &= ~MGC_M_RXCSR_DATAERR;
                wRxCsrVal &= ~MGC_M_RXCSR_RXPKTRDY;
                MGC_WriteCsr16(pBase, MGC_O_HDRC_RXCSR, bEnd, wRxCsrVal | MGC_M_RXCSR_H_REQPKT);

                DBG(1, "[USB]Rx NAK Timeout (%d:%d), Retry %d !\n", 
                    pEnd->bRemoteAddress, pEnd->bEnd, pEnd->bRetries);
                return ;
            }

            if (pUrb->status == ( - EINPROGRESS))
            {
                DBG(1, "[USB]RX NAK Timeout !\n");
                status = USB_ST_NORESPONSE;
            }

            wVal &= ~MGC_M_RXCSR_H_REQPKT;
            MGC_WriteCsr16(pBase, MGC_O_HDRC_RXCSR, bEnd, wVal);
            MGC_WriteCsr8(pBase, MGC_O_HDRC_RXINTERVAL, bEnd, 0);
            pEnd->bRetries = 0;

            /* do the proper sequence to abort the transfer; 
             * am I dealing with a slow device maybe? */
            INFO("[USB]end=%d device Rx not responding (%d:%d) !\n", 
                bEnd, pEnd->bRemoteAddress, pEnd->bEnd);
        }
        else if (PIPE_ISOCHRONOUS == pEnd->bTrafficType)
        {
            DBG(3, "[USB]bEnd=%d Isochronous error\n", bEnd);
            status = USB_ST_NORESPONSE;
        }
    }

    /* an error won't process the data */
    if (status)
    {
        pUrb->status = status;

        /* data errors are signaled */
        if (USB_ST_STALL != status)
        {
            DBG(1, "[USB]end %d Rx error, status=%d !\n", bEnd, status);
        }
        else
        {
            //INFO("[USB]MGC_ServiceRxReady : Flush FIFO !\n");
            /* twice in case of double packet buffering */
            MGC_WriteCsr16((uint8_t*)pThis->pRegs, MGC_O_HDRC_RXCSR, 
                bEnd, MGC_M_RXCSR_FLUSHFIFO | MGC_M_RXCSR_CLRDATATOG);
            MGC_WriteCsr16((uint8_t*)pThis->pRegs, MGC_O_HDRC_RXCSR, 
                bEnd, MGC_M_RXCSR_FLUSHFIFO | MGC_M_RXCSR_CLRDATATOG);
        }

        DBG(3, "[USB]clearing all error bits, right away\n");
        wVal &= ~(MGC_M_RXCSR_H_ERROR | MGC_M_RXCSR_DATAERR | MGC_M_RXCSR_H_RXSTALL);
        wVal &= ~MGC_M_RXCSR_RXPKTRDY;
        MGC_WriteCsr16(pBase, MGC_O_HDRC_RXCSR, bEnd, wVal);
    }

    /* complete the current request or start next one clearing RxPktRdy 
     * and setting ReqPkt */
    if (pUrb->status != ( - EINPROGRESS))
    {        
        // Do not care data toggle bit, we assign fixed endpoint to CmdQ.
        MGC_DequeueEndurb(pThis, pEnd, pUrb);

        pEnd->bBusyCompleting = 1;
        spin_unlock_irqrestore(&pThis->SelectEndLock,irq_flags); 
        MGC_CompleteUrb(pThis, pEnd, pUrb);
        spin_lock_irqsave(&pThis->SelectEndLock,irq_flags);
        pEnd->bBusyCompleting = 0;
        
        if (!MGC_IsEndIdle(pEnd))
        {
            if ((pEnd->bCmdQEnable) && (!pEnd->bStateGated))
            {
                // Just update pEnd->pCurrentUrb for command queue.
                (void)MGC_GetNextUrb(pEnd);                    
            }
            else
            {
                /* Endpoint is not idle. */
                MGC_KickUrb(pThis, MGC_GetNextUrb(pEnd), pEnd);
            }
        }
    }
}

static void QMU_tx_error(MGC_LinuxCd *pThis, uint8_t bEnd)
{
    struct urb *pUrb;
    uint16_t wTxCsrVal;
    uint16_t wVal = 0;
    MGC_LinuxLocalEnd *pEnd; 
    uint8_t *pBase = (uint8_t*)pThis->pRegs;
    uint32_t status = 0;

    if (MGC_GET_TX_EPS_NUM(pThis) <= bEnd)
    {
        INFO("Tx[%d]ERROR: bEnd overflow.\n", bEnd);        
        return;
    }

    MGC_SelectEnd(pBase, bEnd);
    pEnd = &(pThis->aLocalEnd[1][bEnd]); /*1: Tx*/
    
    pUrb = MGC_GetCurrentUrb(pEnd);
    if (!pUrb)
    {
        return ;
    } 

    wTxCsrVal = MGC_ReadCsr16(pBase, MGC_O_HDRC_TXCSR, bEnd);
    wVal = wTxCsrVal;

    /* check for errors */
    if (wTxCsrVal &MGC_M_TXCSR_H_RXSTALL)
    {
        INFO("[USB]TX end %d STALL, Flush FIFO (%d:%d) !\n", 
            bEnd, pEnd->bRemoteAddress, pEnd->bEnd);
        status = USB_ST_STALL;
        MGC_WriteCsr16(pBase, MGC_O_HDRC_TXCSR, bEnd, 
            MGC_M_TXCSR_FLUSHFIFO |MGC_M_TXCSR_TXPKTRDY);
    }
    else if (wTxCsrVal &MGC_M_TXCSR_H_ERROR)
    {
        INFO("[USB]TX data error on ep=%d, Flush FIFO (%d:%d) !\n", 
            bEnd, pEnd->bRemoteAddress, pEnd->bEnd);
        status = USB_ST_NORESPONSE;

        /* do the proper sequence to abort the transfer */
        MGC_WriteCsr16(pBase, MGC_O_HDRC_TXCSR, bEnd, 
            MGC_M_TXCSR_FLUSHFIFO | MGC_M_TXCSR_TXPKTRDY);
    }
    else if (wTxCsrVal &MGC_M_TXCSR_H_NAKTIMEOUT)
    {
        /* cover it up if retries not exhausted */
        if (pUrb->status == ( - EINPROGRESS) && ++pEnd->bRetries < MUSB_MAX_RETRIES)
        {
            wVal |= MGC_M_TXCSR_TXPKTRDY;
            wVal &= ~MGC_M_TXCSR_H_NAKTIMEOUT;
            MGC_WriteCsr16(pBase, MGC_O_HDRC_TXCSR, bEnd, wVal);

            INFO("[USB]TX NAK Timeout (%d:%d), Retry %d !\n", 
                pEnd->bRemoteAddress, pEnd->bEnd, pEnd->bRetries);
            return ;
        }

        if (pUrb->status == ( - EINPROGRESS))
        {
            INFO("[USB]TX NAK Timeout !\n");
            status = USB_ST_NORESPONSE;
        }

        INFO("[USB]device Tx not responding on ep=%d, Flush FIFO (%d:%d) !\n", 
            bEnd, pEnd->bRemoteAddress, pEnd->bEnd);

        /* do the proper sequence to abort the transfer */
        MGC_WriteCsr16(pBase, MGC_O_HDRC_TXCSR, bEnd, 
            MGC_M_TXCSR_FLUSHFIFO | MGC_M_TXCSR_TXPKTRDY);
        pEnd->bRetries = 0;
    }
    else if (wTxCsrVal &MGC_M_TXCSR_FIFONOTEMPTY)
    {
        INFO("[USB]ASSERT FIFO not empty when Tx done on ep=%d (%d:%d) !\n", 
            bEnd, pEnd->bRemoteAddress, pEnd->bEnd);
        /* whopps, dbould buffering better be enabled. */
        /* skip this interrrupt and wait FIFONOTEMPTY to be clear by h/w */
        MUSB_ASSERT(pEnd->bDoublePacketBuffer);
    }

    if (status)
    {
        pUrb->status = status;

        /* reset error bits */
        wVal &= ~(MGC_M_TXCSR_H_ERROR | MGC_M_TXCSR_H_RXSTALL | MGC_M_TXCSR_H_NAKTIMEOUT);
        wVal |= MGC_M_TXCSR_FRCDATATOG;
        MGC_WriteCsr16(pBase, MGC_O_HDRC_TXCSR, bEnd, wVal);
    }

    /* complete the current request or start next tx transaction */
    if (pUrb->status != ( - EINPROGRESS))
    {
        pUrb->actual_length = pEnd->dwOffset;

        // Do not care data toggle bit, we assign fixed endpoint to CmdQ.                                
        MGC_DequeueEndurb(pThis, pEnd, pUrb);

        pEnd->bBusyCompleting = 1;
        spin_unlock_irqrestore(&pThis->SelectEndLock,irq_flags); 
        MGC_CompleteUrb(pThis, pEnd, pUrb);
        spin_lock_irqsave(&pThis->SelectEndLock,irq_flags);
        pEnd->bBusyCompleting = 0;    
        
        if (!MGC_IsEndIdle(pEnd))
        {
            if ((pEnd->bCmdQEnable) && (!pEnd->bStateGated))
            {
                // Just update pEnd->pCurrentUrb for command queue.
                (void)MGC_GetNextUrb(pEnd);                    
            }
            else
            {
                /* Endpoint is not idle. */
                MGC_KickUrb(pThis, MGC_GetNextUrb(pEnd), pEnd);
            }
        }    
    }
}

int QMU_irq(MGC_LinuxCd *pThis, uint32_t qisar)
{
    void __iomem *base = (void __iomem *)pThis->pRegs;
    uint8_t i;
    uint8_t result = 0;
    uint32_t wQmuVal;
    uint32_t wRetVal;
    Q_Port_Info *pQ = &pThis->rCmdQ;
    Q_Buf_Info *pBuf;
    TGPD *pRGpd;    
    uint8_t EP_Num;
    uint32_t u4Val;
    MGC_LinuxLocalEnd *pEnd;
    struct usb_iso_packet_descriptor *d;
    struct urb *pUrb;
    uint8_t wait_HWO_times = 0;
    wQmuVal = qisar;

    //RXQ ERROR
    if (wQmuVal & DQMU_M_RXQ_ERR)
    {
        wRetVal = MGC_ReadQIRQ32(base, MGC_O_QIRQ_RQEIR) & (~(MGC_ReadQIRQ32(base, MGC_O_QIRQ_RQEIMR)));

        INFO("Rx Queue error in QMU mode![0x%x]\n", wRetVal);

        result = 0;
        for (i = 0; i < RXQ_NUM; i++)
        {
            EP_Num = i + 1;            
            if (wRetVal & DQMU_M_RX_GPDCS_ERR(EP_Num))
            {
                INFO("Rx %d GPD checksum error!\n", EP_Num);
                result = 1;
                break;
            }

            if (wRetVal & DQMU_M_RX_LEN_ERR(EP_Num))
            {
                INFO("Rx %d recieve length error!\n", EP_Num);

                MGC_SelectEnd(base, EP_Num);
                INFO("RxCsr=0x%04X, RxCount=0x%04X, RxMaxp=0x%04X.\n", 
                    MGC_ReadCsr16(base, MGC_O_HDRC_RXCSR, EP_Num), 
                    MGC_ReadCsr16(base, MGC_O_HDRC_RXCOUNT, EP_Num), 
                    MGC_ReadCsr16(base, MGC_O_HDRC_RXMAXP, EP_Num));
                result = 1;
                break;
            }

            if (wRetVal & DQMU_M_RX_ZLP_ERR(EP_Num))
            {
                INFO("Rx %d recieve an zlp packet!\n", EP_Num);
                result = 1;
                break;
            }            
        }

        if (result == 1)
        {
            QMU_disable_q(pThis, EP_Num, TRUE);
        }            
        
        MGC_WriteQIRQ32(base, MGC_O_QIRQ_RQEIR, wRetVal);
    }

    //TXQ ERROR
    if (wQmuVal & DQMU_M_TXQ_ERR)
    {
        wRetVal = MGC_ReadQIRQ32(base, MGC_O_QIRQ_TQEIR) & (~(MGC_ReadQIRQ32(base, MGC_O_QIRQ_TQEIMR)));

        INFO("Tx Queue error in QMU mode![0x%x]\n", wRetVal);

        result = 0;
        for (i= 0; i < TXQ_NUM; i++)
        {
            EP_Num = i + 1;                    
            if (wRetVal & DQMU_M_TX_BDCS_ERR(EP_Num))
            {
                INFO("Tx %d BD checksum error!\n", EP_Num);
                result = 1;
                break;
            }

            if (wRetVal & DQMU_M_TX_GPDCS_ERR(EP_Num))
            {
                INFO("Tx %d GPD checksum error!\n", EP_Num);
                result = 1;
                break;
            }
            if (wRetVal & DQMU_M_TX_LEN_ERR(EP_Num))
            {
                INFO("Tx %d buffer length error!\n", EP_Num);
                result = 1;
                break;
            }
        }

        if (result == 1)
        {
            QMU_disable_q(pThis, EP_Num, FALSE);
        }            
        
        MGC_WriteQIRQ32(base, MGC_O_QIRQ_TQEIR, wRetVal);
    }

    //RX EP ERROR
    if (wQmuVal & DQMU_M_RXEP_ERR)
    {
        wRetVal = MGC_ReadQIRQ32(base, MGC_O_QIRQ_REPEIR) & (~(MGC_ReadQIRQ32(base, MGC_O_QIRQ_REPEIMR)));

        for (i = 0; i < RXQ_NUM; i++)
        {
            EP_Num = i + 1;                    
            if (wRetVal & DQMU_M_RX_EP_ERR(EP_Num))
            {
                INFO("Rx %d Ep Error!\n", EP_Num);

                // Just skip the error GPD.
                pBuf = &pQ->rRx[EP_Num - 1];
                Buf_IncRead(pBuf);

                // Disable queue.
                QMU_disable_q(pThis, EP_Num, TRUE);

                // Handle error case.
                QMU_rx_error(pThis, EP_Num);
            }
        }
                
        MGC_WriteQIRQ32(base, MGC_O_QIRQ_REPEIR, wRetVal);
    }

    //TX EP ERROR
    if (wQmuVal & DQMU_M_TXEP_ERR)
    {
        wRetVal = MGC_ReadQIRQ32(base, MGC_O_QIRQ_TEPEIR) & (~(MGC_ReadQIRQ32(base, MGC_O_QIRQ_TEPEIMR)));
        for (i = 0; i < TXQ_NUM; i++)
        {
            EP_Num = i + 1;            
            if (wRetVal & DQMU_M_TX_EP_ERR(EP_Num))
            {
                INFO("Tx %d Ep Error!\n", EP_Num);

                // Just skip the error GPD.
                pBuf = &pQ->rTx[EP_Num - 1];
                Buf_IncRead(pBuf);

                // Disable queue.
                QMU_disable_q(pThis, EP_Num, FALSE);

                // Handle error case.
                QMU_tx_error(pThis, EP_Num);
            }
        }
        
        MGC_WriteQIRQ32(base, MGC_O_QIRQ_TEPEIR, wRetVal);
    }

    //RXQ EMPTY
    if (wQmuVal & DQMU_M_RQ_EMPTY)
    {
        wRetVal = MGC_ReadQIRQ32(base, MGC_O_QIRQ_REPEMPR) & (~(MGC_ReadQIRQ32(base, MGC_O_QIRQ_REPEMPMR)));

        for (i = 1; i <= RXQ_NUM; i++)
        {
            if (wRetVal & DQMU_M_RX_EMPTY(i))
            {
                INFO("Rx %d Empty!\n", i);
            }
        }
        MGC_WriteQIRQ32(base, MGC_O_QIRQ_REPEMPR, wRetVal);
    }

    //TXQ EMPTY
    if (wQmuVal & DQMU_M_TQ_EMPTY)
    {
        wRetVal = MGC_ReadQIRQ32(base, MGC_O_QIRQ_TEPEMPR) & (~(MGC_ReadQIRQ32(base, MGC_O_QIRQ_TEPEMPMR)));

        for (i = 1; i <= TXQ_NUM; i++)
        {
            if (wRetVal & DQMU_M_TX_EMPTY(i))
            {
                INFO("Tx %d Empty!\n", i);
            }
        }
        MGC_WriteQIRQ32(base, MGC_O_QIRQ_TEPEMPR, wRetVal);
    }


    //RXQ GPD DONE
    for (i = 0; i < RXQ_NUM; i++)
    {
        EP_Num = i + 1;
        
        if (wQmuVal & DQMU_M_RX_DONE(EP_Num))
        {
            pEnd = &(pThis->aLocalEnd[0][EP_Num]);  /* Rx : 0 */
            
            u4Val = MGC_ReadQMU32(base, MGC_O_QMU_RQCPR(EP_Num));

            pBuf = &pQ->rRx[EP_Num - 1];
            
            while (Buf_GetReadPhyAddr(pBuf) != u4Val)
            {
                if (Buf_IsEmpty(pBuf))
                {
                    INFO("ERROR: rx_gpd_container empty!\n");

                    return USB_ST_NOMEMORY;
                }
                else
                {
	                wait_HWO_times = 0;
wait_rx_ioc:
                    dma_unmap_single(pThis->dev, 
                        (dma_addr_t)Buf_GetReadPhyAddr(pBuf), pBuf->size, DMA_FROM_DEVICE);

                    dma_unmap_single(pThis->dev, 
                        (dma_addr_t)Buf_GetReadPhyAddr(pBuf), pBuf->size, DMA_TO_DEVICE);
                    
                    pRGpd = (TGPD *)phys_to_virt(Buf_GetReadPhyAddr(pBuf));

                    if ((TGPD_GET_FLAG(pRGpd)) && (wait_HWO_times < MAX_GPD_HW0_RETRY_TIME))
                    {
                        //INFO("QMU_IRQ: Rx HWO is on !\n");
                        //INFO("PhyAddr=0x%08X, EndAddr=0x%08X.\n", Buf_GetReadPhyAddr(pBuf), u4Val);
                        wait_HWO_times ++;
                        goto wait_rx_ioc;
                    }

                    pUrb = MGC_GetCurrentUrb(pEnd);    
                    if (!pUrb)
                    {
                        QMU_stop(pThis, EP_Num, TRUE);
                        return USB_ST_NOMEMORY;
                    }

                    Buf_IncRead(pBuf);

                    if (usb_pipeisoc(pUrb->pipe))
                    {
                        d = &pUrb->iso_frame_desc[pEnd->dwIsoPacket];
                        d->actual_length = TGPD_GET_BUF_LEN(pRGpd);
						#ifdef MGC_RX_LENGTH_HANDLE
						//DBG(-9, "IRQ ISO dwLength = 0x%x, mxPakt = 0x%x.\n", d->actual_length, d->length);
						if(d->actual_length > pEnd->wMaxPacketSize)
						{
							d->actual_length = d->length;
						}
						#endif
                        /*
                        if (d->actual_length == 0)
                        {
                            // 5396 h/w will not handle zero length GPD.
                            INFO("[USB] QMU_IRQ: Rx expect %d, actual = 0.\n", d->length);
                        }
                        */
                        d->status = 0; /* Fixed me !! */
                        pUrb->actual_length += d->actual_length;

                        if (((ulong)pUrb->transfer_dma + (ulong)d->offset) != 
                             (ulong)TGPD_GET_DATA(pRGpd))
                        {
                            INFO("[USB] QMU_IRQ: address compare fail, 0x%lX, 0x%lX.\n", 
                                (ulong)(pUrb->transfer_dma + d->offset), 
                                (ulong)TGPD_GET_DATA(pRGpd));                           
                            //BUG();
                        }

                        pEnd->dwIsoPacket ++;
                        
                        if (pEnd->dwIsoPacket >= pUrb->number_of_packets)
                        {
                            pUrb->status = 0;
                            pEnd->dwIsoPacket = 0;
                            
                            // Do not care data toggle bit, we assign fixed endpoint to CmdQ.
                            MGC_DequeueEndurb(pThis, pEnd, pUrb);

                            pEnd->bBusyCompleting = 1;
                            spin_unlock_irqrestore(&pThis->SelectEndLock,irq_flags); 
                            MGC_CompleteUrb(pThis, pEnd, pUrb);
                            spin_lock_irqsave(&pThis->SelectEndLock,irq_flags);
                            pEnd->bBusyCompleting = 0;                            

                            if (!MGC_IsEndIdle(pEnd))
                            {
                                if ((pEnd->bCmdQEnable) && (!pEnd->bStateGated))
                                {
                                    // Just update pEnd->pCurrentUrb for command queue.
                                    (void)MGC_GetNextUrb(pEnd);                    
                                }
                                else
                                {
                                    /* Endpoint is not idle. */
                                    MGC_KickUrb(pThis, MGC_GetNextUrb(pEnd), pEnd);
                                }
                            }
                        }                            
                    }
                    else
                    {
                        /* Must be bulk transfer type */
                        pUrb->actual_length = TGPD_GET_BUF_LEN(pRGpd);
                        pUrb->status = 0;
                        
                        //INFO("QMU_IRQ: RX, actual_length = %d.\n", pUrb->actual_length);                                                

                        // Do not care data toggle bit, we assign fixed endpoint to CmdQ.
                        MGC_DequeueEndurb(pThis, pEnd, pUrb);

                        pEnd->bBusyCompleting = 1;
                        spin_unlock_irqrestore(&pThis->SelectEndLock,irq_flags);
                        MGC_CompleteUrb(pThis, pEnd, pUrb);
                        spin_lock_irqsave(&pThis->SelectEndLock,irq_flags);
                        pEnd->bBusyCompleting = 0;

                        if (!MGC_IsEndIdle(pEnd))
                        {
                            if ((pEnd->bCmdQEnable) && (!pEnd->bStateGated))
                            {
                                // Just update pEnd->pCurrentUrb for command queue.
                                (void)MGC_GetNextUrb(pEnd);                    
                            }
                            else
                            {
                                /* Endpoint is not idle. */
                                MGC_KickUrb(pThis, MGC_GetNextUrb(pEnd), pEnd);
                            }
                        }
                    }
                }
            }

            result = 1;
        }
    }

    //TXQ GPD DONE
    for (i = 0; i < TXQ_NUM; i++)
    {
        EP_Num = i + 1;
    
        if (wQmuVal & DQMU_M_TX_DONE(EP_Num))
        {
            pEnd = &(pThis->aLocalEnd[1][EP_Num]);  /* Tx : 1 */

            u4Val = MGC_ReadQMU32(base, MGC_O_QMU_TQCPR(EP_Num));

            pBuf = &pQ->rTx[EP_Num - 1];

            while (Buf_GetReadPhyAddr(pBuf) != u4Val)
            {
                if (Buf_IsEmpty(pBuf))
                {
                    INFO("ERROR: tx_gpd_container empty!\n");
                    return USB_ST_NOMEMORY;
                }
                else
                {
wait_tx_ioc:   
                    dma_unmap_single(pThis->dev, 
                        (dma_addr_t)Buf_GetReadPhyAddr(pBuf), pBuf->size, DMA_FROM_DEVICE);

                    dma_unmap_single(pThis->dev, 
                        (dma_addr_t)Buf_GetReadPhyAddr(pBuf), pBuf->size, DMA_TO_DEVICE);
                    
                    pRGpd = (TGPD *)phys_to_virt(Buf_GetReadPhyAddr(pBuf));

                    if (TGPD_GET_FLAG(pRGpd))
                    {
                        //INFO("QMU_IRQ: Tx HWO is on !\n");
                        //INFO("PhyAddr=0x%08X, EndAddr=0x%08X.\n", Buf_GetReadPhyAddr(pBuf), u4Val);                        
                        goto wait_tx_ioc;                        
                    }

                    pUrb = MGC_GetCurrentUrb(pEnd);    
                    if (!pUrb)
                    {
                        QMU_stop(pThis, EP_Num, TRUE);
                        return USB_ST_NOMEMORY;
                    }

                    Buf_IncRead(pBuf);

                    if (usb_pipeisoc(pUrb->pipe))
                    {
                        d = &pUrb->iso_frame_desc[pEnd->dwIsoPacket];
                        d->actual_length = TGPD_GET_BUF_LEN(pRGpd);
                        d->status = 0; /* Fixed me !! */                        
                        pUrb->actual_length += d->actual_length;

                        if (((ulong)pUrb->transfer_dma + (ulong)d->offset) != 
                             (ulong)TGPD_GET_DATA(pRGpd))
                        {
                            INFO("[USB] QMU_IRQ: address compare fail, 0x%lX, 0x%lX.\n", 
                                (ulong)(pUrb->transfer_dma + d->offset), 
                                (ulong)TGPD_GET_DATA(pRGpd));
                            BUG();
                        }
                        
                        pEnd->dwIsoPacket ++;
                        
                        if (pEnd->dwIsoPacket >= pUrb->number_of_packets)
                        {
                            pUrb->status = 0;
                            pEnd->dwIsoPacket = 0;

                            // Do not care data toggle bit, we assign fixed endpoint to CmdQ.
                            MGC_DequeueEndurb(pThis, pEnd, pUrb);

                            pEnd->bBusyCompleting = 1;
                            spin_unlock_irqrestore(&pThis->SelectEndLock,irq_flags); 
                            MGC_CompleteUrb(pThis, pEnd, pUrb);
                            spin_lock_irqsave(&pThis->SelectEndLock,irq_flags);
                            pEnd->bBusyCompleting = 0;                            

                            if (!MGC_IsEndIdle(pEnd))
                            {
                                if ((pEnd->bCmdQEnable) && (!pEnd->bStateGated))
                                {
                                    // Just update pEnd->pCurrentUrb for command queue.
                                    (void)MGC_GetNextUrb(pEnd);                    
                                }
                                else
                                {
                                    /* Endpoint is not idle. */
                                    MGC_KickUrb(pThis, MGC_GetNextUrb(pEnd), pEnd);
                                }
                            }
                        }                            
                    }
                    else
                    {
                        /* Must be bulk transfer type */
                        pUrb->actual_length = TGPD_GET_BUF_LEN(pRGpd);
                        pUrb->status = 0;

                        //INFO("QMU_IRQ: TX, actual_length = %d.\n", pUrb->actual_length);                        

                        // Do not care data toggle bit, we assign fixed endpoint to CmdQ.
                        MGC_DequeueEndurb(pThis, pEnd, pUrb);

                        pEnd->bBusyCompleting = 1;
                        spin_unlock_irqrestore(&pThis->SelectEndLock,irq_flags); 
                        MGC_CompleteUrb(pThis, pEnd, pUrb);
                        spin_lock_irqsave(&pThis->SelectEndLock,irq_flags);
                        pEnd->bBusyCompleting = 0;
                       
                        if (!MGC_IsEndIdle(pEnd))
                        {
                            if ((pEnd->bCmdQEnable) && (!pEnd->bStateGated))
                            {
                                // Just update pEnd->pCurrentUrb for command queue.
                                (void)MGC_GetNextUrb(pEnd);                    
                            }
                            else
                            {
                                /* Endpoint is not idle. */
                                MGC_KickUrb(pThis, MGC_GetNextUrb(pEnd), pEnd);
                            }
                        }
                    }                    
                }
            }

            result = 1;
        }
    }
    
    return result;
}

#endif /*  */
