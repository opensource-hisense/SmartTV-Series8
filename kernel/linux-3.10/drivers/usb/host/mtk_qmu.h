/*-----------------------------------------------------------------------------
 * drivers/usb/musb_qmu/mtk_qmu.h
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

#define USB_HW_QMU_OFF (0x0800)
#define USB_HW_QUCS_OFF  (USB_HW_QMU_OFF + (0x0300))
#define USB_HW_QIRQ_OFF  (USB_HW_QMU_OFF + (0x0400))
#define USB_HW_QDBG_OFF  (USB_HW_QMU_OFF + (0x04F0))

/* --------------------------------------------------------------- */
/*
USB + 800h-AFFh: QMU control and status registers
*/
#define MGC_O_QMU_QCR0 (0x0000)
#define MGC_O_QMU_QCR2 (0x0008)
#define MGC_O_QMU_QCR3 (0x000C)

#define MGC_O_QMU_RQCSR0   (0x0010)
#define MGC_O_QMU_RQSAR0   (0x0014)
#define MGC_O_QMU_RQCPR0  (0x0018)
#define MGC_O_QMU_RQCSR(n) (MGC_O_QMU_RQCSR0+0x0010*((n)-1))
#define MGC_O_QMU_RQSAR(n) (MGC_O_QMU_RQSAR0+0x0010*((n)-1))
#define MGC_O_QMU_RQCPR(n) (MGC_O_QMU_RQCPR0+0x0010*((n)-1))

#define MGC_O_QMU_RQTR_BASE   (0x0090)
#define MGC_O_QMU_RQTR(n)       (MGC_O_QMU_RQTR_BASE+0x4*((n)-1))
#define MGC_O_QMU_RQLDPR0     (0x0100)
#define MGC_O_QMU_RQLDPR(n)  (MGC_O_QMU_RQLDPR0+0x4*((n)-1))

#define MGC_O_QMU_TQCSR0    (0x0200)
#define MGC_O_QMU_TQSAR0   (0x0204)
#define MGC_O_QMU_TQCPR0  (0x0208)
#define MGC_O_QMU_TQCSR(n) (MGC_O_QMU_TQCSR0+0x0010*((n)-1))
#define MGC_O_QMU_TQSAR(n) (MGC_O_QMU_TQSAR0+0x0010*((n)-1))
#define MGC_O_QMU_TQCPR(n) (MGC_O_QMU_TQCPR0+0x0010*((n)-1))

/*
USB + C00h-CEFh: Interrupt control and status registers
*/
#define MGC_O_QMU_QAR     0x0300
#define MGC_O_QUCS_USBGCSR  0x0000
#define MGC_O_QIRQ_QISAR      0x0000
#define MGC_O_QIRQ_QIMR        0x0004
#define MGC_O_QIRQ_QIMCR       0x0008
#define MGC_O_QIRQ_QIMSR     0x000C
#define MGC_O_QIRQ_IOCDISR    0x0030
#define MGC_O_QIRQ_HWVER       0x0040
#define MGC_O_QIRQ_TEPEMPR 0x0060
#define MGC_O_QIRQ_TEPEMPMR  0x0064
#define MGC_O_QIRQ_TEPEMPMCR  0x0068
#define MGC_O_QIRQ_TEPEMPMSR 0x006C
#define MGC_O_QIRQ_REPEMPR    0x0070
#define MGC_O_QIRQ_REPEMPMR 0x0074
#define MGC_O_QIRQ_REPEMPMCR 0x0078
#define MGC_O_QIRQ_REPEMPMSR    0x007C

#define MGC_O_QIRQ_RQEIR     0x0090
#define MGC_O_QIRQ_RQEIMR       0x0094
#define MGC_O_QIRQ_RQEIMCR    0x0098
#define MGC_O_QIRQ_RQEIMSR 0x009C
#define MGC_O_QIRQ_REPEIR      0x00A0
#define MGC_O_QIRQ_REPEIMR   0x00A4
#define MGC_O_QIRQ_REPEIMCR    0x00A8
#define MGC_O_QIRQ_REPEIMSR    0x00AC
#define MGC_O_QIRQ_TQEIR        0x00B0
#define MGC_O_QIRQ_TQEIMR      0x00B4
#define MGC_O_QIRQ_TQEIMCR   0x00B8
#define MGC_O_QIRQ_TQEIMSR    0x00BC
#define MGC_O_QIRQ_TEPEIR     0x00C0
#define MGC_O_QIRQ_TEPEIMR  0x00C4
#define MGC_O_QIRQ_TEPEIMCR   0x00C8
#define MGC_O_QIRQ_TEPEIMSR   0x00CC

/*
USB + CF0h-CFFh: Debug flag register
*/
#define MGC_O_QDBG_DFCR   0x00F0
#define MGC_O_QDBG_DFMR   0x00F4
/* --------------------------------------------------------------- */

#define DQMU_QUE_START  0x00000001
#define DQMU_QUE_RESUME   0x00000002
#define DQMU_QUE_STOP       0x00000004
#define DQMU_QUE_ACTIVE    0x00008000

#define USB_QMU_Tx0_EN             0x00000001
#define USB_QMU_Tx_EN(n)           (USB_QMU_Tx0_EN<<((n)-1))
#define USB_QMU_Rx0_EN           0x00010000
#define USB_QMU_Rx_EN(n)            (USB_QMU_Rx0_EN<<((n)-1))
#define USB_QMU_HIFEVT_EN         0x00000100
#define USB_QMU_HIFCMD_EN            0x01000000
#define DQMU_SW_RESET       0x00010000
#define DQMU_CS16B_EN        0x80000000
#define DQMU_TQ0CS_EN     0x00010000
#define DQMU_TQCS_EN(n)  (DQMU_TQ0CS_EN<<((n)-1))
#define DQMU_RQ0CS_EN        0x00000001
#define DQMU_RQCS_EN(n) (DQMU_RQ0CS_EN<<((n)-1))
#define DQMU_TX0_ZLP       0x01000000
#define DQMU_TX_ZLP(n)        (DQMU_TX0_ZLP<<((n)-1))
#define DQMU_TX0_MULTIPLE   0x00010000
#define DQMU_TX_MULTIPLE(n) (DQMU_TX0_MULTIPLE<<((n)-1))
#define DQMU_RX0_MULTIPLE  0x00010000
#define DQMU_RX_MULTIPLE(n)    (DQMU_RX0_MULTIPLE<<((n)-1))
#define DQMU_RX0_ZLP         0x01000000
#define DQMU_RX_ZLP(n)      (DQMU_RX0_ZLP<<((n)-1))
#define DQMU_RX0_COZ         0x00000100
#define DQMU_RX_COZ(n)      (DQMU_RX0_COZ<<((n)-1))

#define DQMU_M_TXEP_ERR   0x10000000
#define DQMU_M_TXQ_ERR   0x08000000
#define DQMU_M_RXEP_ERR    0x04000000
#define DQMU_M_RXQ_ERR    0x02000000
#define DQMU_M_RQ_EMPTY 0x00020000
#define DQMU_M_TQ_EMPTY 0x00010000
#define DQMU_M_RX0_EMPTY 0x00000001
#define DQMU_M_RX_EMPTY(n)    (DQMU_M_RX0_EMPTY<<((n)-1))
#define DQMU_M_TX0_EMPTY   0x00000001
#define DQMU_M_TX_EMPTY(n)  (DQMU_M_TX0_EMPTY<<((n)-1))
#define DQMU_M_RX0_DONE 0x00000100
#define DQMU_M_RX_DONE(n) (DQMU_M_RX0_DONE<<((n)-1))
#define DQMU_M_TX0_DONE   0x00000001
#define DQMU_M_TX_DONE(n)   (DQMU_M_TX0_DONE<<((n)-1))

#define DQMU_M_RX0_ZLP_ERR   0x01000000
#define DQMU_M_RX_ZLP_ERR(n)    (DQMU_M_RX0_ZLP_ERR<<((n)-1))
#define DQMU_M_RX0_LEN_ERR   0x00000100
#define DQMU_M_RX_LEN_ERR(n)    (DQMU_M_RX0_LEN_ERR<<((n)-1))
#define DQMU_M_RX0_GPDCS_ERR        0x00000001
#define DQMU_M_RX_GPDCS_ERR(n)  (DQMU_M_RX0_GPDCS_ERR<<((n)-1))

#define DQMU_M_TX0_LEN_ERR    0x00010000
#define DQMU_M_TX_LEN_ERR(n) (DQMU_M_TX0_LEN_ERR<<((n)-1))
#define DQMU_M_TX0_GPDCS_ERR 0x00000100
#define DQMU_M_TX_GPDCS_ERR(n)    (DQMU_M_TX0_GPDCS_ERR<<((n)-1))
#define DQMU_M_TX0_BDCS_ERR        0x00000001
#define DQMU_M_TX_BDCS_ERR(n)   (DQMU_M_TX0_BDCS_ERR<<((n)-1))

#define DQMU_M_TX0_EP_ERR       0x00000001
#define DQMU_M_TX_EP_ERR(n)    (DQMU_M_TX0_EP_ERR<<((n)-1))

#define DQMU_M_RX0_EP_ERR        0x00000001
#define DQMU_M_RX_EP_ERR(n) (DQMU_M_RX0_EP_ERR<<((n)-1))

#define DQMU_M_TQ_DIS_IOC(n)   (0x1<<((n)-1))
#define DQMU_M_RQ_DIS_IOC(n)   (0x100<<((n)-1))

/**
 * @brief Read a 32-bit register from the core
 * @param _pBase core base address in memory
 * @param _offset offset into the core's register space
 * @return 32-bit datum
 */
#define MGC_ReadQMU32(base,_offset) \
    *((volatile uint32_t *)(((ulong)base) + (USB_HW_QMU_OFF)+ (_offset)))

#define MGC_ReadQUCS32(base,_offset) \
    *((volatile uint32_t *)(((ulong)base) + (USB_HW_QUCS_OFF)+ (_offset)))

#define MGC_ReadQIRQ32(base,_offset) \
    *((volatile uint32_t *)(((ulong)base) + (USB_HW_QIRQ_OFF)+ (_offset)))

/**
 * @briefWrite a 32-bit core register
 * @param _pBase core base address in memory
 * @param _offset offset into the core's register space
 * @param _data 32-bit datum
 */
#define MGC_WriteQMU32(base,_offset, _data) \
    (*((volatile uint32_t *)(((ulong)base) + (USB_HW_QMU_OFF)+ (_offset))) = _data)

#define MGC_WriteQUCS32(base,_offset, _data) \
    (*((volatile uint32_t *)(((ulong)base) + (USB_HW_QUCS_OFF)+ (_offset))) = _data)

#define MGC_WriteQIRQ32(base,_offset, _data) \
    (*((volatile uint32_t *)(((ulong)base) + (USB_HW_QIRQ_OFF)+ (_offset))) = _data)

/// @brief Define DMAQ GPD & BD format
/// @Author_Name:tianhao.fei 4/29/2010
/// @{
#define TGPD_FLAGS_HWO              0x01
#define TGPD_IS_FLAGS_HWO(_pd)      (((volatile TGPD *)_pd)->flag & TGPD_FLAGS_HWO)
#define TGPD_SET_FLAGS_HWO(_pd)     (((volatile TGPD *)_pd)->flag |= TGPD_FLAGS_HWO)
#define TGPD_CLR_FLAGS_HWO(_pd)     (((volatile TGPD *)_pd)->flag &= (~TGPD_FLAGS_HWO))
#define TGPD_FORMAT_BDP             0x02
#define TGPD_IS_FORMAT_BDP(_pd)     (((volatile TGPD *)_pd)->flag & TGPD_FORMAT_BDP)
#define TGPD_SET_FORMAT_BDP(_pd)    (((volatile TGPD *)_pd)->flag |= TGPD_FORMAT_BDP)
#define TGPD_CLR_FORMAT_BDP(_pd)    (((volatile TGPD *)_pd)->flag &= (~TGPD_FORMAT_BDP))

#define TGPD_SET_FLAG(_pd, _flag)   ((volatile TGPD *)_pd)->flag = (((volatile TGPD *)_pd)->flag&(~TGPD_FLAGS_HWO))|(_flag)
#define TGPD_GET_FLAG(_pd)             (((volatile TGPD *)_pd)->flag & TGPD_FLAGS_HWO)
#define TGPD_SET_CHKSUM(_pd, _n)    ((volatile TGPD *)_pd)->chksum = QMU_pdu_checksum((uint8_t *)_pd, _n)
#define TGPD_SET_CHKSUM_HWO(_pd, _n)    ((volatile TGPD *)_pd)->chksum = QMU_pdu_checksum((uint8_t *)_pd, _n)-1
#define TGPD_GET_CHKSUM(_pd)        ((volatile TGPD *)_pd)->chksum
#define TGPD_SET_FORMAT(_pd, _fmt)  ((volatile TGPD *)_pd)->flag = (((volatile TGPD *)_pd)->flag&(~TGPD_FORMAT_BDP))|(_fmt)
#define TGPD_GET_FORMAT(_pd)        ((((volatile TGPD *)_pd)->flag & TGPD_FORMAT_BDP)>>1)
#define TGPD_SET_DataBUF_LEN(_pd, _len) ((volatile TGPD *)_pd)->DataBufferLen = _len
#define TGPD_ADD_DataBUF_LEN(_pd, _len) ((volatile TGPD *)_pd)->DataBufferLen += _len
#define TGPD_GET_DataBUF_LEN(_pd)       ((volatile TGPD *)_pd)->DataBufferLen
//#define TGPD_SET_NEXT(_pd, _next)   (( TGPD *)_pd)->pNext = (( TGPD *)(ulong)_next)
#define TGPD_SET_NEXT(_pd, _next)   (( TGPD *)_pd)->pNext_addr = ((uint32_t)_next)

//#define TGPD_GET_NEXT(_pd)          (TGPD *)phys_to_virt((uint32_t)_pd->pNext)
#define TGPD_GET_NEXT(_pd)          (TGPD *)phys_to_virt((uint32_t)_pd->pNext_addr)

#if 0
#define TGPD_SET_TBD(_pd, _tbd)     ((volatile TGPD *)_pd)->pBuf = (uint8_t *)_tbd; \
                                    TGPD_SET_FORMAT_BDP(_pd)
#define TGPD_GET_TBD(_pd)           ((volatile TBD *)(((volatile TGPD *)_pd)->pBuf))
#endif

//#define TGPD_SET_DATA(_pd, _data)   ((volatile TGPD *)_pd)->pBuf = (uint8_t *)_data
#define TGPD_SET_DATA(_pd, _data)   ((volatile TGPD *)_pd)->pBuf_addr = (uint32_t)(_data)

//#define TGPD_GET_DATA(_pd)          (uint8_t *)(((TGPD *)_pd)->pBuf)
#define TGPD_GET_DATA(_pd)          (((TGPD *)_pd)->pBuf_addr)

#define TGPD_SET_BUF_LEN(_pd, _len) ((volatile TGPD *)_pd)->bufLen = _len
#define TGPD_ADD_BUF_LEN(_pd, _len) ((volatile TGPD *)_pd)->bufLen += _len
#define TGPD_GET_BUF_LEN(_pd)       ((volatile TGPD *)_pd)->bufLen
#define TGPD_SET_EXT_LEN(_pd, _len) ((volatile TGPD *)_pd)->ExtLength = _len
#define TGPD_GET_EXT_LEN(_pd)        ((volatile TGPD *)_pd)->ExtLength
#define TGPD_SET_EPaddr(_pd, _EP)  ((volatile TGPD *)_pd)->ZTepFlag =(((volatile TGPD *)_pd)->ZTepFlag&0xF0)|(_EP)
#define TGPD_GET_EPaddr(_pd)        ((volatile TGPD *)_pd)->ZTepFlag & 0x0F

#define TGPD_FORMAT_TGL             0x10
#define TGPD_IS_FORMAT_TGL(_pd)     (((volatile TGPD *)_pd)->ZTepFlag & TGPD_FORMAT_TGL)
#define TGPD_SET_FORMAT_TGL(_pd)    (((volatile TGPD *)_pd)->ZTepFlag |=TGPD_FORMAT_TGL)
#define TGPD_CLR_FORMAT_TGL(_pd)    (((volatile TGPD *)_pd)->ZTepFlag &= (~TGPD_FORMAT_TGL))
#define TGPD_FORMAT_ZLP             0x20
#define TGPD_IS_FORMAT_ZLP(_pd)     (((volatile TGPD *)_pd)->ZTepFlag & TGPD_FORMAT_ZLP)
#define TGPD_SET_FORMAT_ZLP(_pd)    (((volatile TGPD *)_pd)->ZTepFlag |=TGPD_FORMAT_ZLP)
#define TGPD_CLR_FORMAT_ZLP(_pd)    (((volatile TGPD *)_pd)->ZTepFlag &= (~TGPD_FORMAT_ZLP))

#define TGPD_SET_TGL(_pd, _TGL)  ((volatile TGPD *)_pd)->ZTepFlag |=(( _TGL) ? 0x10: 0x00)
#define TGPD_GET_TGL(_pd)        ((volatile TGPD *)_pd)->ZTepFlag & 0x10 ? 1:0
#define TGPD_SET_ZLP(_pd, _ZLP)  ((volatile TGPD *)_pd)->ZTepFlag |= ((_ZLP) ? 0x20: 0x00)
#define TGPD_GET_ZLP(_pd)        ((volatile TGPD *)_pd)->ZTepFlag & 0x20 ? 1:0

#define TGPD_FLAG_IOC             0x80
#define TGPD_SET_IOC(_pd)    (((volatile TGPD *)_pd)->flag |= TGPD_FLAG_IOC)
#define TGPD_GET_EXT(_pd)           ((uint8_t *)_pd + sizeof(TGPD))
#define TGPD_SET_KEY_IDX(_pd, _data)       \
{                                          \
    uint8_t *_tmp = ((uint8_t *)_pd + sizeof(TGPD)); \
    *_tmp = (unsigned char)_data;          \
}
#define TGPD_GET_KEY_IDX(_pd)        *((uint8_t *)_pd + sizeof(TGPD))

/**
 * TGPD
 */

typedef struct _TGPD
{
    unsigned char flag;
    unsigned char chksum;
    unsigned short DataBufferLen; /*Rx Allow Length*/
    //struct _TGPD *pNext;
    //uint8_t *pBuf;
	unsigned int pNext_addr;
	unsigned int pBuf_addr;
    unsigned short bufLen;
    unsigned char ExtLength;
    unsigned char ZTepFlag;
} __attribute__((packed)) TGPD, *PGPD;

#define TBD_FLAGS_EOL               0x01
#define TBD_IS_FLAGS_EOL(_bd)       (((volatile TBD *)_bd)->flag & TBD_FLAGS_EOL)
#define TBD_SET_FLAGS_EOL(_bd)      (((volatile TBD *)_bd)->flag |= TBD_FLAGS_EOL)
#define TBD_CLR_FLAGS_EOL(_bd)      (((volatile TBD *)_bd)->flag &= (~TBD_FLAGS_EOL))

#define TBD_SET_FLAG(_bd, _flag)    ((volatile TBD *)_bd)->flag = (unsigned char)_flag
#define TBD_GET_FLAG(_bd)           ((volatile TBD *)_bd)->flag
#define TBD_SET_CHKSUM(_pd, _n)     (((volatile TBD *)_pd)->chksum = QMU_pdu_checksum((uint8_t *)_pd, _n))
#define TBD_GET_CHKSUM(_pd)         ((volatile TBD *)_pd)->chksum
#define TBD_SET_NEXT(_bd, _next)    (((volatile TBD *)_bd)->pNext = (volatile TBD *)_next)
#define TBD_GET_NEXT(_bd)           ((volatile TBD *)(((volatile TBD *)_bd)->pNext))
#define TBD_SET_DATA(_bd, _data)    (((volatile TBD *)_bd)->pBuf = (uint8_t *)_data)
#define TBD_GET_DATA(_bd)           ((volatile uint8_t *)(((volatile TBD *)_bd)->pBuf))
#define TBD_SET_BUF_LEN(_bd, _len)  ((volatile TBD *)_bd)->bufLen = _len
#define TBD_ADD_BUF_LEN(_bd, _len)  ((volatile TBD *)_bd)->bufLen += _len
#define TBD_GET_BUF_LEN(_bd)        ((volatile TBD *)_bd)->bufLen
#define TBD_SET_EXT_LEN(_bd, _len)  ((volatile TBD *)_bd)->extLen = _len
#define TBD_ADD_EXT_LEN(_bd, _len)  ((volatile TBD *)_bd)->extLen += _len
#define TBD_GET_EXT_LEN(_bd)        ((volatile TBD *)_bd)->extLen
#define TBD_GET_EXT(_bd)            ((uint8_t *)_bd + sizeof(TBD))

/**
 * TBD
 */
typedef struct _TBD
{
    unsigned char flag;

    unsigned char chksum;
    unsigned short reserved1;
    struct _TBD *pNext;
    uint8_t *pBuf;
    unsigned short bufLen;
    unsigned char extLen;
    unsigned char reserved2;
} __attribute__((packed)) TBD, *PBD;

/// @brief Define RxQ & TxQ number
/// .
/// @Author_Name:tianhao.fei 4/29/2010
/// @{


#define RXQ_NUM 4
#define TXQ_NUM 1


/// @}

#define MAX_GPD_HW0_RETRY_TIME    8
#define GPD_LEN_ALIGNED (32)  /* Note: ARM should align cache line size */
#define MAX_URB_NUM  (320)

#define Buf_FreeSpace(Buf) \
    (((Buf->read) >= (Buf->write)) ? \
    ((Buf->read) - (Buf->write)) : ((Buf->length) - ((Buf->write) - (Buf->read))))
    
#define Buf_IsFull(Buf) \
    ((((Buf->write) + 1) % Buf->length) == Buf->read)

#define Buf_IsEmpty(Buf) \
    (Buf->write == Buf->read)

#define Buf_IncWrite(Buf) \
    (Buf->write = (((Buf->write) + 1) % Buf->length))

#define Buf_IncRead(Buf) \
    (Buf->read = (((Buf->read) + 1) % Buf->length))

#define Buf_GetWritePhyAddr(Buf) \
    ((Buf->start) + ((Buf->write) * (Buf->size)))

#define Buf_GetReadPhyAddr(Buf) \
    ((Buf->start) + ((Buf->read) * (Buf->size)))

typedef struct Mtk_Queue_Buf_Info
{
    void *pLogicalBuf; /* Point to user's allocate kernel memory. */
    uint32_t start;       /* data buffer start physical address pointer. */
    uint32_t read;       /* hardware consume data index. */
    uint32_t write;      /* software input data index. */
    uint32_t size;        /* single buffer size. */
    uint32_t length;    /* Total circular buffer length (index). */
} Q_Buf_Info;

typedef struct Mtk_Queue_Port_Info
{
    Q_Buf_Info rRx[RXQ_NUM];
    Q_Buf_Info rTx[TXQ_NUM];
} Q_Port_Info;

/// @brief Define DMAQ Error Status
/// .
/// @Author_Name:tianhao.fei 4/29/2010
//CC: maybe we need to redefine this
enum QStatus
{
    MTK_NO_ERROR,     ///No error
    MTK_MODULE_ERROR, ///checksum error
    MTK_LENGTH_ERROR, ///length error (for Rx, receive exceed length data)
    MTK_BUS_ERROR     ///Bus error
};

#endif
