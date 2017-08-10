/*
 * drivers/tty/serial/mt53xx_uart.c
 *
 * MT53xx UART driver
 *
 * Copyright (c) 2008-2012 MediaTek Inc.
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

#include <linux/module.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/console.h>
#include <linux/sysrq.h>
#include <linux/device.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/serial_core.h>
#include <linux/serial.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/cdev.h>

#include <asm/delay.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/uaccess.h>
#include <linux/soc/mediatek/hardware.h>
#include <linux/version.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/of_address.h>


#include "mt53xx_uart.h"
#include "linux/soc/mediatek/mt53xx_linux.h"

#if 1//def CC_LOG_PERFORMANCE_SUPPORT
#include <linux/kthread.h>
#include <linux/vmalloc.h>
#include <linux/semaphore.h>
#include <linux/fs.h>
#include <linux/timer.h>
#endif


#ifdef CONFIG_SERIAL_MT53XX_FACTORY_SUPPORT
#define CONFIG_TTYMT7_SUPPORT
#ifdef CC_S_PLATFORM
#define INCOMPLETE_CMD_SUPPORT
#define CC_KERNEL_NO_LOG
unsigned int hotel_mode_id;
static unsigned int _u4FactoryWillStandby = 0;
#endif
#endif

//#define UART_DEBUG_RX_DATA

#ifdef ENABLE_UDMA
#include <linux/mm.h>
#include <linux/dma-mapping.h>
#include <linux/timer.h>
//SMP define
#include <linux/sched.h>
#include <linux/cpumask.h>
#include <asm/current.h>

//#define U0_U1_SHARE_IRQ_HANDLER
//#define ENABLE_DMA_CHK
//#define PRETECT_RESOURCE

#define NOCACHE_UART_DMA
#define CYJ_REMOVE_ONE_COPY
#define ENABLE_RX_TIMER
#define UART_DEBUG 0

#define MIN(x,y) (x<y?x:y)

#define UDMA_BUFFER_SIZE (16*1024)


#ifdef ENABLE_RX_TIMER
#define  TIMER_INTERVAL   3         // 3 HZ    ,3ms
static void _mt53xx_uart_rx_timer(unsigned long data);
static DEFINE_TIMER(uart_rx_timer, _mt53xx_uart_rx_timer, 0, 0);
#endif

int DebugMesFlag = 1;
unsigned int rx_loss_data = 0;
unsigned int enable_ctsrts = 0;

#if UART_DEBUG
#define TX_BUFFER  6000000
#define RX_BUFFER  6000000
char txBuffer[TX_BUFFER];
char rxBuffer[RX_BUFFER];
int txCount = 0, rxCount = 0;
int sameByte;
#endif
//int yjdbg_txcnt;
//int yjdbg_rxcnt;
#if 0
#define RX_LOG_BUF_SIZE 1048576 // 32768
unsigned char yjdbg_rxbuf[RX_LOG_BUF_SIZE];
unsigned int rxbuf_idx;
#endif


#endif

#ifdef ENABLE_UDMA
//#define DISABLE_MODEM_CONTROL
#else
#define DISABLE_MODEM_CONTROL
#endif

/* MT5890 :   support uart0(debug port), uart1(DMA), uart 2(PD), uart 3(Factory mode), uart 4(HT), uart 5(BT) */
/* other IC : support uart0(debug port), uart1(DMA), uart 2(PD), uart 3(Factory mode) */

#define UART_MIN_FACTORY_KEY    (0x20)
#define UART_MAX_FACTORY_KEY    (0x80)

#ifdef CONFIG_TTYMT7_SUPPORT
#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5891)
#define UART_NR                 6
#else
#define UART_NR                 4
#endif
// Uart only handle visible character.
// When receive character > UART_MIN_FACTORY_KEY and character = _u4MagicMaxInputForceFactory or _u4MagicMinInputForceFactory
// it will enter factory mode. Uart driver will notify MW by _pfMwFactoryInput() when receive new character.
// MW will receive data by calling  UART_Read().

#else
#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5891)
#define UART_NR                 5
#else
#define UART_NR                 3
#endif
#endif /* CONFIG_TTYMT7_SUPPORT */

#define SERIAL_MT53xx_MAJOR     204
#define SERIAL_MT53xx_MINOR     16


/* Port */
struct mt53xx_uart_port
{
    struct uart_port port;
    int nport;
    unsigned int old_status;
    unsigned int tx_stop;
    unsigned int rx_stop;
    unsigned int ms_enable;
    unsigned int baud, datalen, stopbit, parity;
#ifdef ENABLE_UDMA
    int hw_init;
    unsigned char* pu1DmaBuffer;
    unsigned char* pu1DrvBuffer;
#endif

    unsigned int    (*fn_read_allow)  (void);
    unsigned int    (*fn_write_allow) (void);
    void            (*fn_int_enable)  (int enable);
    void            (*fn_empty_int_enable) (int enable);
    unsigned int    (*fn_read_byte)   (void);
    void            (*fn_write_byte)  (unsigned int byte);
    void            (*fn_flush)       (void);
    void            (*fn_get_top_err) (int* p_parity, int* p_end, int* p_break);
};
static struct mt53xx_uart_port _mt53xx_uart_ports[];

typedef struct __STR_UART_Setting
{
    int i4BaudRate;
    int i4DataLen;
    int i4StopBit;
    int i4Parity;
} UART_Setting;

#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5891)
UART_Setting _gUartSettings[5];
#else
UART_Setting _gUartSettings[3];
#endif

#ifndef ENABLE_UDMA
static int _hw_init = 0;
#endif

#ifdef CONFIG_TTYMT7_SUPPORT
static int _enable_ttyMT7 = 0;
static unsigned int _u4MagicInputForceFactory = 0x60;
static unsigned int _u4NumMagicInputForceFactory = 0;
static unsigned int _au4OtherMagicInputForceFactory[4] = {0x60, 0x60, 0x60, 0x60};
struct semaphore uart_sema;
#endif

#if 1//def CC_LOG_PERFORMANCE_SUPPORT
typedef struct __BUFFER_INFO
{
    unsigned int Read;        /* @field Current Read index. */
    unsigned int Write;       /* @field Current Write index. */
    unsigned int Length;      /* @field Length of buffer */
    unsigned char *CharBuffer;  /* @field Start of buffer */
} BUFFER_INFO;

#define MINS(x,y) (x<y?x:y)
//#define ResetFifo(Buffer)           Buffer.Write = Buffer.Read = 0
#define BWrite(Buffer)          (Buffer->Write)
#define BRead(Buffer)           (Buffer->Read)
#define BLength(Buffer)         (Buffer->Length)
#define BuffWrite(Buffer)       (Buffer->CharBuffer+Buffer->Write)
#define BuffRead(Buffer)        (Buffer->CharBuffer+Buffer->Read)

#define BWrite_addr(Buffer)     (Buffer.Write)
#define BRead_addr(Buffer)      (Buffer.Read)
#define BLength_addr(Buffer)    (Buffer.Length)
#define BuffWrite_addr(Buffer)  (Buffer.CharBuffer+Buffer.Write)
#define BuffRead_addr(Buffer)   (Buffer.CharBuffer+Buffer.Read)
#define Buff_EndAddr(Buffer)    (Buffer.CharBuffer+Buffer.Length-1)
#define Buff_StartAddr(Buffer)  (Buffer.CharBuffer)

#define Buff_isEmpty            1
#define Buff_notEmpty           0
#define Buff_isFull             1
#define Buff_notFull            0
#define Buff_PushOK             0
#define Buff_PushErr            1
#define Buff_PopOK              0
#define Buff_PopErr             1

#define Buf_init(_Buffer,_Buffaddr,_uTotalSize)                   \
{                                                                 \
    BUFFER_INFO *_Buf=_Buffer;                                    \
    _Buf->Read = 0;                                               \
    _Buf->Write = 0;                                              \
    _Buf->Length = _uTotalSize;                                   \
    _Buf->CharBuffer = _Buffaddr;                                 \
}

#define Buf_IsFull(_Buffer,_result)                               \
{                                                                 \
    BUFFER_INFO *_Buf=_Buffer;                                    \
    kal_uint16 _tmp = BRead(_Buf);                                \
    if (_tmp == 0)                                                \
        _tmp = BLength(_Buf);                                     \
    if ((_tmp-BWrite(_Buf)) == 1)                                 \
    {                                                             \
        _result = Buff_isFull;                                    \
    }                                                             \
    else                                                          \
    {                                                             \
        _result = Buff_notFull;                                   \
    }                                                             \
}

#define Buf_GetRoomLeft(_Buffer,_RoomLeft)                        \
{                                                                 \
    BUFFER_INFO *_Buf=_Buffer;                                    \
    if (BRead(_Buf) <= BWrite(_Buf))                              \
    {                                                             \
        _RoomLeft = (BLength(_Buf) - BWrite(_Buf)) + (BRead(_Buf) - 1); \
    }                                                             \
    else                                                          \
    {                                                             \
        _RoomLeft = (BRead(_Buf) - BWrite(_Buf)) - 1;             \
    }                                                             \
}

#define Buf_Push(_Buffer,_pushData)                               \
{                                                                 \
    BUFFER_INFO *_Buf=_Buffer;                                    \
    *BuffWrite(_Buf) = _pushData;                                 \
    if(BWrite(_Buf) >= (BLength(_Buf) - 1))                       \
    {                                                             \
        BWrite(_Buf) = 0;                                         \
    }                                                             \
    else                                                          \
    {                                                             \
        BWrite(_Buf)++;                                           \
    }                                                             \
}

#define Buf_GetBytesAvail(_Buffer,_BytesAvail)                    \
{                                                                 \
    BUFFER_INFO *_Buf = _Buffer;                                  \
    _BytesAvail = 0;                                              \
    if (BWrite(_Buf) >= BRead(_Buf))                              \
    {                                                             \
        _BytesAvail = BWrite(_Buf) - BRead(_Buf);                 \
    }                                                             \
    else                                                          \
    {                                                             \
        _BytesAvail = (BLength(_Buf) - BRead(_Buf)) + BWrite(_Buf); \
    }                                                             \
}

#define Buf_Pop(_Buffer,_popData)                                 \
{                                                                 \
    BUFFER_INFO *_Buf = _Buffer;                                  \
    _popData= *BuffRead(_Buf);                                    \
    BRead(_Buf)++;                                                \
    if (BRead(_Buf) >= BLength(_Buf))                             \
    {                                                             \
        BRead(_Buf) -= BLength(_Buf);                             \
    }                                                             \
}

#define UART0_RING_BUFFER_SIZE 1024*1024*4
static struct task_struct *_hUart0Thread = NULL;
static unsigned char *_au0TxBuf = NULL;
static BUFFER_INFO _arTxFIFO;
struct semaphore uart0_log_sema;

#ifdef CC_USE_TTYMT1
static int console_index;
#endif
#endif //ifdef CC_LOG_PERFORMANCE_SUPPORT


/* Locals */
static void _mt53xx_uart_stop_tx(struct uart_port* port);
#ifdef CC_LOG2USB_SUPPORT
static void _log2usb_write_byte(unsigned int byte);
#endif

static void __iomem *pUartIoMap;
static void __iomem *pPdwncIoMap;
static void __iomem *pCkgenIoMap;
static void __iomem *pBimIoMap;



/* HW functions of mt53xx uart */
#define UART_READ32(REG)            (((REG) < UART2_BASE) ?                    \
                                     __raw_readl(pUartIoMap + (REG)) :         \
                                     __raw_readl(pPdwncIoMap + (REG)))

#define UART_WRITE32(VAL, REG)      (((REG) < UART2_BASE) ?                    \
                                     __raw_writel(VAL, pUartIoMap + (REG)) :   \
                                     __raw_writel(VAL, pPdwncIoMap + (REG)))

/* Macros */
#define CKGEN_READ32(REG)            __raw_readl(pCkgenIoMap + (REG))
#define CKGEN_WRITE32(VAL, REG)      __raw_writel(VAL, pCkgenIoMap + (REG))

#define UART_REG_BITCLR(BITS, REG)      UART_WRITE32(UART_READ32(REG) & ~(BITS), REG)
#define UART_REG_BITSET(BITS, REG)      UART_WRITE32(UART_READ32(REG) | (BITS), REG)


/* uart 0 and 1 */
#define MT53xx_PA_UART      (0xF0000000 + 0x0c000)//(pUartIoMap + 0x0)//     //0x2000c000
#define MT53xx_VA_UART      (0xF0000000 + 0x0c000)//(pUartIoMap + 0x0)//     //0xf000c000
#define MT53xx_PA_UART_IOMAP ((unsigned long)pUartIoMap + 0x0)
#define MT53xx_VA_UART_IOMAP (pUartIoMap + 0x0)
#define MT53xx_UART_SIZE    0x1000
/* uart 2*/
#define MT53xx_PA_UART2     (0xF0000000 + 0x28800)//(pPdwncIoMap + 0x800)//
#define MT53xx_VA_UART2     (0xF0000000 + 0x28800)//(pPdwncIoMap + 0x800)//
#define MT53xx_PA_UART_IOMAP2 ((unsigned long)pPdwncIoMap + 0x800)
#define MT53xx_VA_UART_IOMAP2 (pPdwncIoMap + 0x800)
#define MT53xx_UART2_SIZE   0x100
#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5891)
#define MT53xx_PA_UART4     (0xF0000000 + 0x28844)//(pPdwncIoMap + 0x844)//
#define MT53xx_VA_UART4     (0xF0000000 + 0x28844)//(pPdwncIoMap + 0x844)//
#define MT53xx_PA_UART_IOMAP4 ((unsigned long)pPdwncIoMap + 0x844)
#define MT53xx_VA_UART_IOMAP4 (pPdwncIoMap + 0x844)

#define MT53xx_PA_UART5     (0xF0000000 + 0x28880)//(pPdwncIoMap + 0x880)//
#define MT53xx_VA_UART5     (0xF0000000 + 0x28880)//(pPdwncIoMap + 0x880)//
#define MT53xx_PA_UART_IOMAP5 ((unsigned long)pPdwncIoMap + 0x880)
#define MT53xx_VA_UART_IOMAP5 (pPdwncIoMap + 0x880)
#endif


#ifdef INCOMPLETE_CMD_SUPPORT
static int __init hotel_mode_setup(char *mode)
{
	sscanf(mode, "%u",&hotel_mode_id);
	return 1;
}
#endif


#ifdef CONFIG_TTYMT7_SUPPORT
static int _UART_CheckMagicChar(int port, unsigned int data_byte)
{
    unsigned int i;
    unsigned int u4Num = _u4NumMagicInputForceFactory;
    if (u4Num == 0)
    {
        return 0;
    }
    // Check for magic char candidate 0
    if (((unsigned char)(_u4MagicInputForceFactory & 0xff) > UART_MAX_FACTORY_KEY || (unsigned char)(_u4MagicInputForceFactory & 0xff) < UART_MIN_FACTORY_KEY) &&
        ((unsigned char)data_byte == (unsigned char)(_u4MagicInputForceFactory & 0xff)) )
    {
        // switch to factory mode.
        unsigned long flags;
        local_irq_save(flags);
        _enable_ttyMT7 = 1;
        local_irq_restore(flags);
        up(&uart_sema);
        return 1;
    }
    // Check for magic char candidate 1 ~ 4
    u4Num = (u4Num > 5) ? 4 : (u4Num - 1);
    for (i = 0; i < u4Num; i++)
    {
        if (((unsigned char)(_au4OtherMagicInputForceFactory[i] & 0xff) > UART_MAX_FACTORY_KEY || (unsigned char)(_au4OtherMagicInputForceFactory[i] & 0xff) < UART_MIN_FACTORY_KEY) &&
            ((unsigned char)data_byte == (unsigned char)(_au4OtherMagicInputForceFactory[i] & 0xff)) )
        {
            // switch to factory mode.
            unsigned long flags;
            local_irq_save(flags);
            _enable_ttyMT7 = 1;
            local_irq_restore(flags);
            up(&uart_sema);
            return 1;
        }
    }
    return 0;
}

#endif

/* HW functions */
static unsigned int _mt53xx_u0_trans_mode_on(void)
{
    return UART_READ32(UART0_STATUS_OLD) & U0_TRANSPARENT;
}

static unsigned int _mt53xx_u2_trans_mode_on(void)
{
    return UART_READ32(UART2_STATUS_OLD) & U2_TRANSPARENT;
}

#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5891)
static unsigned int _mt53xx_u4_trans_mode_on(void)
{
    return UART_READ32(UART4_STATUS_OLD) & U4_TRANSPARENT;
}

static unsigned int _mt53xx_u5_trans_mode_on(void)
{
    return UART_READ32(UART5_STATUS_OLD) & U5_TRANSPARENT;
}
#endif

static void _mt53xx_uart_flush(int port)
{
    unsigned int u4Reg;
    unsigned long flags;

    switch (port)
    {
        case UART_PORT_0:
            u4Reg = UART0_BUFCTRL;
            break;
        case UART_PORT_1:
            u4Reg = UART1_BUFCTRL;
            break;
        case UART_PORT_2:
            u4Reg = UART2_BUFCTRL;
            break;
#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5891)
        case UART_PORT_4:
            u4Reg = UART4_BUFCTRL;
            break;
        case UART_PORT_5:
            u4Reg = UART5_BUFCTRL;
            break;
#endif
        default:
            return;
    }

    local_irq_save(flags);

    UART_REG_BITSET((CLEAR_TBUF | CLEAR_RBUF), u4Reg);

    local_irq_restore(flags);
}

static void _mt53xx_u0_set_trans_mode(unsigned int on)
{
    if (on > 0)
    {
        UART_WRITE32(0xE2, UART0_STATUS_OLD);
    }
    else
    {
        UART_WRITE32(0, UART0_STATUS_OLD);
    }
}

static void _mt53xx_u2_set_trans_mode(unsigned int on)
{
    if (on > 0)
    {
        UART_WRITE32(0xE2, UART2_STATUS_OLD);
    }
    else
    {
        UART_WRITE32(0, UART2_STATUS_OLD);
    }
}

#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5891)
static void _mt53xx_u4_set_trans_mode(unsigned int on)
{
    if (on > 0)
    {
        UART_WRITE32(0xE2, UART4_STATUS_OLD);
    }
    else
    {
        UART_WRITE32(0, UART4_STATUS_OLD);
    }
}

static void _mt53xx_u5_set_trans_mode(unsigned int on)
{
    if (on > 0)
    {
        UART_WRITE32(0xE2, UART5_STATUS_OLD);
    }
    else
    {
        UART_WRITE32(0, UART5_STATUS_OLD);
    }
}
#endif

static void _UART_HwWaitTxBufClear(unsigned int u4Port)
{
    unsigned int u4RegAddr;

    switch (u4Port)
    {
        case UART_PORT_0:
            u4RegAddr = UART0_STATUS;
            break;

        case UART_PORT_1:
            u4RegAddr = UART1_STATUS;
            break;

        case UART_PORT_2:
            u4RegAddr = UART2_STATUS;
            break;
#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5891)
        case UART_PORT_4:
            u4RegAddr = UART4_STATUS;
            break;
        case UART_PORT_5:
            u4RegAddr = UART5_STATUS;
            break;
#endif

        default:
            return;
    }

    while ((UART_READ32(u4RegAddr) & UART_WRITE_ALLOW) != UART_WRITE_ALLOW);
}

#ifdef ENABLE_UDMA
int yjdbg_dram_full;
int yjdbg_fifo_full;
int yjdbg_dmaw_trig;
int yjdbg_dmaw_tout;

static unsigned int _mt5395_u1_HwGetRxDataCnt(void)
{
    return UART_READ32(UART1_DMAW_LEVEL);
}

static void _mt5395_u1_HwDmaWriteDram(unsigned int u4SPTR, unsigned int u4Size, unsigned int u4Threshold)
{
    unsigned int u4Reg = 0;
    u4Threshold &= 0xFFFFFF;
    UART_WRITE32(0, UART1_DMAW_CTRL);
    u4Reg = UART_READ32(UART_INT_EN);
    u4Reg |= U1_WBUF_TOUT | U1_WBUF_FULL | U1_WBUF_OVER;
    UART_WRITE32(u4Reg, UART_INT_EN);
    UART_WRITE32(u4SPTR, UART1_DMAW_SPTR);
    UART_WRITE32((u4SPTR + u4Size), UART1_DMAW_EPTR);
    UART_WRITE32(1, UART1_DMAW_RST);
    UART_WRITE32((u4Threshold << 8) | 3, UART1_DMAW_CTRL);
}

#if 0
static void _mt5395_u1_HwDmaReadDisable(void)
{
    UART_WRITE32(0, UART1_DMAR_EN);
}
#endif

#ifdef CYJ_REMOVE_ONE_COPY
static void fake_memcpy(unsigned char* dst, unsigned char* src, unsigned int length)
{
    struct mt53xx_uart_port* mport1 = &_mt53xx_uart_ports[1];
    unsigned int i;
#if UART_DEBUG
    unsigned int u4flag;
#endif
    for (i = 0; i < length; i++)
    {
        uart_insert_char(&mport1->port, 0, 0, *(volatile unsigned char*)(src + i), TTY_NORMAL);
        mport1->port.icount.rx++;
#if UART_DEBUG
        u4flag = *((volatile unsigned int*)LOG_REGISTER);
        if (u4flag == LOG_PRINT_RX)
        {
            printk("tx: 0x%x(%c)\n", *(src + i), *(src + i));
        }
        rxCount %= RX_BUFFER;
        rxBuffer[rxCount++] = *(src + i);
#endif
#if 0
        if (yjdbg_rxcnt > 0)
        {
            yjdbg_rxcnt--;
            printk("yjdbg.rx.data: 0x%x\n", *(src + i));
        }
#endif
#if 0
        if (rxbuf_idx < RX_LOG_BUF_SIZE)
        {
            yjdbg_rxbuf[rxbuf_idx++] = *(src + i);
        }
#endif
    }
}
#endif

#if 0
void log_rx_buf(void)
{
    unsigned int i;
    printk("yjdbg.rxlog.buffer\n");
    for (i = 0; (i + 15) < rxbuf_idx && (i + 15) < RX_LOG_BUF_SIZE; i += 16)
    {
        printk("%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\n",
               yjdbg_rxbuf[i + 0],
               yjdbg_rxbuf[i + 1],
               yjdbg_rxbuf[i + 2],
               yjdbg_rxbuf[i + 3],
               yjdbg_rxbuf[i + 4],
               yjdbg_rxbuf[i + 5],
               yjdbg_rxbuf[i + 6],
               yjdbg_rxbuf[i + 7],
               yjdbg_rxbuf[i + 8],
               yjdbg_rxbuf[i + 9],
               yjdbg_rxbuf[i + 10],
               yjdbg_rxbuf[i + 11],
               yjdbg_rxbuf[i + 12],
               yjdbg_rxbuf[i + 13],
               yjdbg_rxbuf[i + 14],
               yjdbg_rxbuf[i + 15]);
    }
}
#endif

static unsigned int _mt5395_u1_GetRxDMABuf(unsigned char* pBuffer, unsigned int u4NumToRead, int timeout)
{
    unsigned int u4WPTR;
    unsigned int u4RPTR;
    unsigned int u4EPTR;
    unsigned int u4SPTR;
    unsigned int u4RxLength = 0;
    unsigned int u4Length;
#ifdef NOCACHE_UART_DMA
    struct mt53xx_uart_port* mport1 = &_mt53xx_uart_ports[1];
#endif

    // only uart 1 support DMA buffer.
    //    ASSERT(u4Port== UART_PORT_1);
    //    ASSERT(pBuffer);

    u4Length = UART_READ32(UART1_DMAW_LEVEL);
    if (u4Length == 0)
    {
        return 0;
    }

    u4WPTR = UART_READ32(UART1_DMAW_WPTR);
    u4RPTR = UART_READ32(UART1_DMAW_RPTR);
    u4SPTR = UART_READ32(UART1_DMAW_SPTR);
    u4EPTR = UART_READ32(UART1_DMAW_EPTR);

    if (u4WPTR > u4RPTR)
    {
        u4Length = MIN((u4WPTR - u4RPTR), u4NumToRead);
        if (enable_ctsrts)
        {
            if (u4Length > rx_loss_data)
            {
                u4Length = u4Length - rx_loss_data;
            }
            else if (!timeout)
            {
                u4Length = 0;
                return u4Length;
            }
        }
#ifdef CYJ_REMOVE_ONE_COPY
#ifdef NOCACHE_UART_DMA
        fake_memcpy((void*)pBuffer, u4RPTR - u4SPTR + (mport1->pu1DmaBuffer), u4Length);
#else
        fake_memcpy((void*)pBuffer, phys_to_virt((const void*)u4RPTR), u4Length);
#endif
#else
        memcpy((void*)pBuffer, phys_to_virt((const void*)u4RPTR), u4Length);
#endif
        UART_WRITE32((u4RPTR + u4Length), UART1_DMAW_RPTR);
        return u4Length;
    }
    else
    {
        u4Length = MIN((u4EPTR - u4RPTR), u4NumToRead);
#ifdef CYJ_REMOVE_ONE_COPY
#ifdef NOCACHE_UART_DMA
        fake_memcpy((void*)pBuffer, u4RPTR - u4SPTR + (mport1->pu1DmaBuffer), u4Length);
#else
        fake_memcpy((void*)pBuffer, phys_to_virt((const void*)u4RPTR), u4Length);
#endif
#else
        memcpy((void*)pBuffer, phys_to_virt((const void*)u4RPTR), u4Length);
#endif
        UART_WRITE32((u4RPTR + u4Length), UART1_DMAW_RPTR);
        if ((u4RPTR + u4Length) == u4EPTR)
        {
            UART_WRITE32(u4SPTR, UART1_DMAW_RPTR);
        }
        if (u4Length == u4NumToRead)
        {
            return u4Length;
        }
        u4RxLength += u4Length;
        pBuffer += u4Length;
        u4NumToRead -= u4Length;
        u4Length = MIN((u4WPTR - u4SPTR), u4NumToRead);
        if (enable_ctsrts)
        {
            if (u4Length > rx_loss_data)
            {
                u4Length = u4Length - rx_loss_data;
            }
            else if (!timeout)
            {
                u4Length = 0;
                return u4RxLength;
            }
        }
        u4RxLength += u4Length;
#ifdef CYJ_REMOVE_ONE_COPY
#ifdef NOCACHE_UART_DMA
        fake_memcpy((void*)pBuffer, (mport1->pu1DmaBuffer), u4Length);
#else
        fake_memcpy((void*)pBuffer, phys_to_virt((const void*)u4SPTR), u4Length);
#endif
#else
        memcpy((void*)pBuffer, phys_to_virt((const void*)u4SPTR), u4Length);
#endif
        UART_WRITE32((u4SPTR + u4Length), UART1_DMAW_RPTR);
        return u4RxLength;
    }
}

static unsigned int _mt5395_u1_ReadRxFifo(struct mt53xx_uart_port* mport, unsigned char* pBuffer, unsigned int u4NumToRead, int timeout)
{
    unsigned int u4DataAvail;
#ifndef NOCACHE_UART_DMA
    dma_map_single(NULL, mport->pu1DmaBuffer, UDMA_BUFFER_SIZE, DMA_FROM_DEVICE);
#endif
    u4DataAvail = _mt5395_u1_GetRxDMABuf(pBuffer, u4NumToRead, timeout);
    return u4DataAvail;
}

unsigned int snapshot_u4SPTR;
unsigned int snapshot_u4Size;
unsigned int snapshot_u4Threshold;
static unsigned int dmainit_inited = 0;
static void _mt5395_uart_dmainit(struct mt53xx_uart_port* mport)
{
    unsigned int z_size;
    unsigned int ui4_order;
#ifdef NOCACHE_UART_DMA
    unsigned int hwaddr;
#endif

    if (mport->nport == 1 && dmainit_inited == 0)
    {
        printk("yjdbg.call._mt5395_uart_dmainit.for.uart1\n");
        z_size = UDMA_BUFFER_SIZE;
        ui4_order = get_order(z_size);
        if (mport->pu1DmaBuffer == NULL)
        {
#ifdef NOCACHE_UART_DMA
            mport->pu1DmaBuffer = (unsigned char*)dma_alloc_coherent(NULL, z_size, (dma_addr_t*)&hwaddr, GFP_KERNEL | GFP_DMA | __GFP_REPEAT);
            printk("<0>yjdbg.dmabuffer.cpuaddr: 0x%x\n", (int)mport->pu1DmaBuffer);
            printk("<0>yjdbg.dmabuffer.busaddr: 0x%x\n", hwaddr);
            printk("<0>yjdbg.dmabuffer.vir_to_phy addr: 0x%x\n", virt_to_phys(mport->pu1DmaBuffer));
#if UART_DEBUG
            memset(txBuffer, 0, TX_BUFFER);
            memset(rxBuffer, 0, RX_BUFFER);
            printk("<0>yjdbg.txBuffer=0x%x, rxBuffer=0x%x, dmasize=1M\n", (int)txBuffer, (int)rxBuffer);
#endif

#else
            mport->pu1DmaBuffer = (unsigned char*)__get_free_pages(GFP_KERNEL, ui4_order);
#endif
        }
        if (mport->pu1DrvBuffer == NULL)
        {
            mport->pu1DrvBuffer = (unsigned char*)__get_free_pages(GFP_KERNEL, ui4_order);
        }
        if (mport->pu1DmaBuffer != NULL)
        {
#ifdef NOCACHE_UART_DMA
            snapshot_u4SPTR = hwaddr;
            snapshot_u4Size = UDMA_BUFFER_SIZE;
            snapshot_u4Threshold = 1;
            _mt5395_u1_HwDmaWriteDram(hwaddr, UDMA_BUFFER_SIZE, 1);
#else
            snapshot_u4SPTR = virt_to_phys(mport->pu1DmaBuffer);
            snapshot_u4Size = UDMA_BUFFER_SIZE;
            snapshot_u4Threshold = 1;
            _mt5395_u1_HwDmaWriteDram(virt_to_phys(mport->pu1DmaBuffer), UDMA_BUFFER_SIZE, 1);
#endif
        }
        dmainit_inited = 1;
    }
}

#endif

#ifdef ENABLE_UDMA
static void _mt53xx_uart_hwinit(struct mt53xx_uart_port* mport, int port)
#else
static void _mt53xx_uart_hwinit(int port)
#endif
{
    unsigned int u4Tmp;
    unsigned long flags;
#ifdef CC_NO_KRL_UART_DRV
    if (port == 1)
    {
        return;
    }
#endif

    local_irq_save(flags);
    //////////////////Port 0//////////////////////////////////////////////////////
    if (port == 0)
    {
#ifdef ENABLE_UDMA
        u4Tmp = ~U0_INTMSK;
        UART_WRITE32(u4Tmp, UART_INT_ID);
#endif
        /* disable uart 0 interrupt */
        u4Tmp = UART_READ32(UART_INT_EN);
        u4Tmp &= ~U0_INTMSK;
        UART_WRITE32(u4Tmp, UART_INT_EN);

        /* config uart mode */
        u4Tmp = UART_READ32(UART_RS232_MODE);
        u4Tmp &= ~DBG_MODE_MSK;
        u4Tmp |= DBG_RS232_ACTIVE;
        UART_WRITE32(u4Tmp, UART_RS232_MODE);

        // Config normal mode buffer size
        if (UART_READ32(UART0_BUFFER_SIZE) != 0)
        {
            _mt53xx_u0_set_trans_mode(0);
            UART_WRITE32(0, UART0_BUFFER_SIZE);
            _mt53xx_u0_set_trans_mode(1);
        }

        // Change to transparent mode if necessary
        if (!_mt53xx_u0_trans_mode_on())
        {
            _mt53xx_u0_set_trans_mode(1);
        }

#ifndef CONFIG_MT53_FPGA
        UART_WRITE32(BUFCTRL_INIT, UART0_BUFCTRL);
        _UART_HwWaitTxBufClear(UART_PORT_0);
#endif

        // Disable modem control by default
        UART_WRITE32(0, UART0_MODEM);
    }

    //////////////////Port 1//////////////////////////////////////////////////////
    if (port == 1)
    {
        /* disable uart 1 interrupt */
        u4Tmp = UART_READ32(UART_INT_EN);
        u4Tmp &= ~U1_INTMSK;
        UART_WRITE32(u4Tmp, UART_INT_EN);

        // Set uart 1 initial settings
#ifndef ENABLE_UDMA
        UART_WRITE32(0, UART1_DMAW_CTRL);
        // Disable modem control by default
        UART_WRITE32(0, UART1_MODEM);
#else
        _mt5395_uart_dmainit(mport);
#if defined(ENABLE_DMA_CHK)
        UART_WRITE32(MODEM_HW_RTS | MODEM_HW_CTS | MODEM_RTS_TRIGGER_VALUE, UART1_MODEM);
#endif
#endif

#ifndef CONFIG_MT53_FPGA
        UART_WRITE32(BUFCTRL_INIT, UART1_BUFCTRL);
        _UART_HwWaitTxBufClear(UART_PORT_1);
#endif

        // Clear interrupt status
        u4Tmp = ~U1_INTMSK;
        UART_WRITE32(u4Tmp, UART_INT_ID);
    }

    //////////////////Port 2//////////////////////////////////////////////////////
    if (port == 2)
    {
#ifdef ENABLE_UDMA
        u4Tmp = ~U2_INTMSK;
        UART_WRITE32(u4Tmp, UART2_INT_ID);
#endif
        /* Change PD UART control to ARM */
        u4Tmp = __raw_readl(pPdwncIoMap + 0x178);
        u4Tmp &= ~(0x01000000);
        __raw_writel(u4Tmp, pPdwncIoMap + 0x178);

        /* disable uart2 interrupt */
        u4Tmp = UART_READ32(UART2_INT_EN);
        u4Tmp &= ~U2_INTMSK;
        UART_WRITE32(u4Tmp, UART2_INT_EN);

        u4Tmp = UART_READ32(UART2_RS232_MODE);
        u4Tmp &= ~DBG_MODE_MSK;
        u4Tmp |= DBG_RS232_ACTIVE;
        UART_WRITE32(u4Tmp, UART2_RS232_MODE);

        if (UART_READ32(UART2_BUFFER_SIZE) != 0)
        {
            _mt53xx_u2_set_trans_mode(0);
            UART_WRITE32(0, UART2_BUFFER_SIZE);
            _mt53xx_u2_set_trans_mode(1);
        }

        if (!_mt53xx_u2_trans_mode_on())
        {
            _mt53xx_u2_set_trans_mode(1);
        }

        /*
         * rx data timeout = 15 * 1/27M sec
         * trigger level 26
         * flush rx, tx buffer
         */
#ifndef CONFIG_MT53_FPGA
        UART_WRITE32(BUFCTRL_INIT, UART2_BUFCTRL);
        _UART_HwWaitTxBufClear(UART_PORT_2);
#endif
        // Clear interrupt status
        u4Tmp = ~U2_INTMSK;
        UART_WRITE32(u4Tmp, UART2_INT_ID);
    }

#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5891)
    //////////////////Port 4//////////////////////////////////////////////////////
    if (port == 4)
    {
        /* Change PDWNC control to ARM */
        u4Tmp = __raw_readl(pPdwncIoMap + 0x178);
        u4Tmp &= ~(0x01000000);
        __raw_writel(u4Tmp, pPdwncIoMap + 0x178);

        /* disable uart4 interrupt */
        u4Tmp = UART_READ32(UART4_INT_EN);
        u4Tmp &= ~U4_INTMSK;
        UART_WRITE32(u4Tmp, UART4_INT_EN);

        u4Tmp = UART_READ32(UART4_RS232_MODE);
        u4Tmp &= ~DBG_MODE_MSK;
        u4Tmp |= DBG_RS232_ACTIVE;
        UART_WRITE32(u4Tmp, UART4_RS232_MODE);

        if (UART_READ32(UART4_BUFFER_SIZE) != 0)
        {
            _mt53xx_u4_set_trans_mode(0);
            UART_WRITE32(0, UART4_BUFFER_SIZE);
            _mt53xx_u4_set_trans_mode(1);
        }

        if (!_mt53xx_u4_trans_mode_on())
        {
            _mt53xx_u4_set_trans_mode(1);
        }
#ifndef CONFIG_MT53_FPGA
        UART_WRITE32(BUFCTRL_INIT, UART4_BUFCTRL);
        _UART_HwWaitTxBufClear(UART_PORT_4);
#endif
        // Clear interrupt status
        u4Tmp = ~U4_INTMSK;
        UART_WRITE32(u4Tmp, UART4_INT_ID);
    }
    //////////////////Port 5//////////////////////////////////////////////////////
    if (port == 5)
    {
        /* Change PDWNC control to ARM */
        u4Tmp = __raw_readl(pPdwncIoMap + 0x178);
        u4Tmp &= ~(0x01000000);
        __raw_writel(u4Tmp, pPdwncIoMap + 0x178);

        /* disable uart5 interrupt */
        u4Tmp = UART_READ32(UART5_INT_EN);
        u4Tmp &= ~U5_INTMSK;
        UART_WRITE32(u4Tmp, UART5_INT_EN);

        u4Tmp = UART_READ32(UART5_RS232_MODE);
        u4Tmp &= ~DBG_MODE_MSK;
        u4Tmp |= DBG_RS232_ACTIVE;
        UART_WRITE32(u4Tmp, UART5_RS232_MODE);

        if (UART_READ32(UART5_BUFFER_SIZE) != 0)
        {
            _mt53xx_u5_set_trans_mode(0);
            UART_WRITE32(0, UART5_BUFFER_SIZE);
            _mt53xx_u5_set_trans_mode(1);
        }

        if (!_mt53xx_u5_trans_mode_on())
        {
            _mt53xx_u5_set_trans_mode(1);
        }
#ifndef CONFIG_MT53_FPGA
        UART_WRITE32(BUFCTRL_INIT, UART5_BUFCTRL);
        _UART_HwWaitTxBufClear(UART_PORT_5);
#endif
        // Clear interrupt status
        u4Tmp = ~U5_INTMSK;
        UART_WRITE32(u4Tmp, UART5_INT_ID);
    }
#endif

    local_irq_restore(flags);
}

void _mt53xx_uart_set(unsigned int uCOMMCTRL, int baud, int datalen, int stop, int parity)
{
#ifdef ENABLE_UDMA
#if UART_DEBUG
    printk(KERN_ERR "%s, uCOMMCTRL=%d,baud=%d\n", __FUNCTION__, uCOMMCTRL, baud);
#endif
#endif

    if (uCOMMCTRL == UART2_COMMCTRL
#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5891)
        || uCOMMCTRL == UART4_COMMCTRL || uCOMMCTRL == UART5_COMMCTRL
#endif
       )
    {
        switch (baud)
        {
            case 115200:
                UART_REG_BITCLR(BAUD_MASK, uCOMMCTRL);
                UART_REG_BITSET(SETBAUD(U2_BAUD_115200), uCOMMCTRL);
                break;
            case 57600:
                UART_REG_BITCLR(BAUD_MASK, uCOMMCTRL);
                UART_REG_BITSET(SETBAUD(U2_BAUD_57600), uCOMMCTRL);
                break;
            case 38400:
                UART_REG_BITCLR(BAUD_MASK, uCOMMCTRL);
                UART_REG_BITSET(SETBAUD(U2_BAUD_38400), uCOMMCTRL);
                break;
            case 19200:
                UART_REG_BITCLR(BAUD_MASK, uCOMMCTRL);
                UART_REG_BITSET(SETBAUD(U2_BAUD_19200), uCOMMCTRL);
                break;
            case 9600:
                UART_REG_BITCLR(BAUD_MASK, uCOMMCTRL);
                UART_REG_BITSET(SETBAUD(U2_BAUD_9600), uCOMMCTRL);
                break;
            case 2400:
                UART_REG_BITCLR(BAUD_MASK, uCOMMCTRL);
                UART_REG_BITSET(SETBAUD(U2_BAUD_2400), uCOMMCTRL);
                break;
            default:
                break;
        }
    }
    else
    {
#ifdef ENABLE_UDMA
        if (uCOMMCTRL == UART1_COMMCTRL)
        {
#if UART_DEBUG
            printk("yjdbg.set.uart1.to.baud.%d\n", baud);
#endif
        }
#endif

        switch (baud)
        {
            case 115200:
                UART_REG_BITCLR(BAUD_MASK, uCOMMCTRL);
                UART_REG_BITSET(SETBAUD(BAUD_115200), uCOMMCTRL);
                break;
            case 230400:
                UART_REG_BITCLR(BAUD_MASK, uCOMMCTRL);
                UART_REG_BITSET(SETBAUD(BAUD_230400), uCOMMCTRL);
                break;
            case 460800:
                UART_REG_BITCLR(BAUD_MASK, uCOMMCTRL);
                UART_REG_BITSET(SETBAUD(BAUD_460800), uCOMMCTRL);
                break;
            case 921600:
                UART_REG_BITCLR(BAUD_MASK, uCOMMCTRL);
                UART_REG_BITSET(SETBAUD(BAUD_921600), uCOMMCTRL);
                break;
            case 57600:
                UART_REG_BITCLR(BAUD_MASK, uCOMMCTRL);
                UART_REG_BITSET(SETBAUD(BAUD_57600), uCOMMCTRL);
                break;
            case 38400:
                UART_REG_BITCLR(BAUD_MASK, uCOMMCTRL);
                UART_REG_BITSET(SETBAUD(BAUD_38400), uCOMMCTRL);
                break;
            case 19200:
                UART_REG_BITCLR(BAUD_MASK, uCOMMCTRL);
                UART_REG_BITSET(SETBAUD(BAUD_19200), uCOMMCTRL);
                break;
            case 9600:
                UART_REG_BITCLR(BAUD_MASK, uCOMMCTRL);
                UART_REG_BITSET(SETBAUD(BAUD_9600), uCOMMCTRL);
                break;
            case 4800:
                UART_REG_BITCLR(BAUD_MASK, uCOMMCTRL);
                UART_REG_BITSET(SETBAUD(BAUD_4800), uCOMMCTRL);
                break;
            case 2400:
                UART_REG_BITCLR(BAUD_MASK, uCOMMCTRL);
                UART_REG_BITSET(SETBAUD(BAUD_2400), uCOMMCTRL);
                break;
            case 1200:
                UART_REG_BITCLR(BAUD_MASK, uCOMMCTRL);
                UART_REG_BITSET(SETBAUD(BAUD_1200), uCOMMCTRL);
                break;
            case 300:
                UART_REG_BITCLR(BAUD_MASK, uCOMMCTRL);
                UART_REG_BITSET(SETBAUD(BAUD_300), uCOMMCTRL);
                break;
            case 110:
                UART_REG_BITCLR(BAUD_MASK, uCOMMCTRL);
                UART_REG_BITSET(SETBAUD(BAUD_110), uCOMMCTRL);
                break;
#ifdef ENABLE_UDMA
            case 1500000:
                UART_REG_BITCLR(CUSTBAUD_MASK, uCOMMCTRL);
                UART_REG_BITSET(SETCUSTBAUD(0xF0), uCOMMCTRL);
                UART_REG_BITCLR(BAUD_MASK, uCOMMCTRL);
                UART_REG_BITSET(SETBAUD(BAUD_CUST), uCOMMCTRL);
                break;
            case 2000000:
                UART_REG_BITCLR(CUSTBAUD_MASK, uCOMMCTRL);
                UART_REG_BITSET(SETCUSTBAUD(0xB0), uCOMMCTRL);
                UART_REG_BITCLR(BAUD_MASK, uCOMMCTRL);
                UART_REG_BITSET(SETBAUD(BAUD_CUST), uCOMMCTRL);
                break;
            case 3000000:
                UART_REG_BITCLR(CUSTBAUD_MASK, uCOMMCTRL);
                UART_REG_BITSET(SETCUSTBAUD(0x70), uCOMMCTRL);
                UART_REG_BITCLR(BAUD_MASK, uCOMMCTRL);
                UART_REG_BITSET(SETBAUD(BAUD_CUST), uCOMMCTRL);
                break;
            case 4000000:
                UART_REG_BITCLR(CUSTBAUD_MASK, uCOMMCTRL);
                UART_REG_BITSET(SETCUSTBAUD(0x50), uCOMMCTRL);
                UART_REG_BITCLR(BAUD_MASK, uCOMMCTRL);
                UART_REG_BITSET(SETBAUD(BAUD_CUST), uCOMMCTRL);
                break;
            case 6000000:
                UART_REG_BITCLR(CUSTBAUD_MASK, uCOMMCTRL);
                UART_REG_BITSET(SETCUSTBAUD(0x30), uCOMMCTRL);
                UART_REG_BITCLR(BAUD_MASK, uCOMMCTRL);
                UART_REG_BITSET(SETBAUD(BAUD_CUST), uCOMMCTRL);
                break;
#endif
            default:
                break;
        }
    }

    /* datalen & stop */
    if (stop == 5)          // 1.5 bits
    {
        if (datalen != 5)
        {
            goto lc_set_parity;
        }
        UART_REG_BITCLR(UBIT_MASK, uCOMMCTRL);
        UART_REG_BITSET(UBIT_5_1_5, uCOMMCTRL);
    }
    else
    {
        UART_REG_BITCLR(STOP_BIT_MASK, uCOMMCTRL);
        UART_REG_BITSET((stop == 2) ? STOP_BIT_2 : STOP_BIT_1, uCOMMCTRL);
        switch (datalen)
        {
            case 8:
                UART_REG_BITCLR(DATA_BIT_MASK, uCOMMCTRL);
                UART_REG_BITSET(DATA_BIT_8, uCOMMCTRL);
                break;
            case 7:
                UART_REG_BITCLR(DATA_BIT_MASK, uCOMMCTRL);
                UART_REG_BITSET(DATA_BIT_7, uCOMMCTRL);
                break;
            case 6:
                UART_REG_BITCLR(DATA_BIT_MASK, uCOMMCTRL);
                UART_REG_BITSET(DATA_BIT_6, uCOMMCTRL);
                break;
            case 5:
                UART_REG_BITCLR(DATA_BIT_MASK, uCOMMCTRL);
                UART_REG_BITSET(DATA_BIT_5, uCOMMCTRL);
                break;
            default:
                break;
        }
    }

lc_set_parity:
    switch (parity)
    {
        case 0:
            UART_REG_BITCLR(PARITY_MASK, uCOMMCTRL);
            UART_REG_BITSET(PARITY_NONE, uCOMMCTRL);
            break;
        case 1:
            UART_REG_BITCLR(PARITY_MASK, uCOMMCTRL);
            UART_REG_BITSET(PARITY_ODD, uCOMMCTRL);
            break;
        case 2:
            UART_REG_BITCLR(PARITY_MASK, uCOMMCTRL);
            UART_REG_BITSET(PARITY_EVEN, uCOMMCTRL);
            break;
        default:
            break;
    }
}

static void _mt53xx_uart_get(unsigned int uCOMMCTRL, int* p_baud, int* p_datalen, int* p_stop, int* p_parity)
{
    int baud, datalen, stop, parity;

    if (uCOMMCTRL == UART2_COMMCTRL
#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5891)
        || uCOMMCTRL == UART4_COMMCTRL || uCOMMCTRL == UART5_COMMCTRL
#endif
       )
    {
        switch (GETBAUD(UART_READ32(uCOMMCTRL)))
        {
            case U2_BAUD_X1:
                baud = 115200;
                break;
            case U2_BAUD_57600:
                baud = 57600;
                break;
            case U2_BAUD_38400:
                baud = 38400;
                break;
            case U2_BAUD_19200:
                baud = 19200;
                break;
            case U2_BAUD_9600:
                baud = 9600;
                break;
            case U2_BAUD_2400:
                baud = 2400;
                break;
            default:
                baud = 115200;
                break;
        }
    }
    else
    {
        switch (GETBAUD(UART_READ32(uCOMMCTRL)))
        {
            case BAUD_X1:
                baud = 115200;
                break;
            case BAUD_X2:
                baud = 230400;
                break;
            case BAUD_X4:
                baud = 460800;
                break;
            case BAUD_X8:
                baud = 921600;
                break;
            case BAUD_57600:
                baud = 57600;
                break;
            case BAUD_38400:
                baud = 38400;
                break;
            case BAUD_19200:
                baud = 19200;
                break;
            case BAUD_9600:
                baud = 9600;
                break;
            case BAUD_4800:
                baud = 4800;
                break;
            case BAUD_2400:
                baud = 2400;
                break;
            case BAUD_1200:
                baud = 1200;
                break;
            case BAUD_300:
                baud = 300;
                break;
            case BAUD_110:
                baud = 110;
                break;
#ifdef ENABLE_UDMA
            case BAUD_CUST:
                baud = 3000000;
                break;
#endif
            default:
                baud = 115200;
                break;
        }
    }

    switch (UART_READ32(uCOMMCTRL) & UBIT_MASK)
    {
        case UBIT_8_1:
            datalen = 8;
            stop    = 1;
            break;
        case UBIT_8_2:
            datalen = 8;
            stop    = 2;
            break;
        case UBIT_7_1:
            datalen = 7;
            stop    = 1;
            break;
        case UBIT_7_2:
            datalen = 7;
            stop    = 2;
            break;
        case UBIT_6_1:
            datalen = 6;
            stop    = 1;
            break;
        case UBIT_6_2:
            datalen = 6;
            stop    = 2;
            break;
        case UBIT_5_1:
            datalen = 5;
            stop    = 1;
            break;
        case UBIT_5_1_5:
            datalen = 5;
            stop    = 5;
            break;
        default:
            datalen = 8;
            stop    = 1;
            break;
    }

    switch (UART_READ32(uCOMMCTRL) & PARITY_MASK)
    {
        case PARITY_NONE:
            parity = 0;
            break;
        case PARITY_ODD:
            parity = 1;
            break;
        case PARITY_EVEN:
            parity = 2;
            break;
        default:
            parity = 0;
            break;
    }

    *p_baud = baud;
    *p_datalen = datalen;
    *p_stop = stop;
    *p_parity = parity;
}

/* uart member functions */
static unsigned int _mt53xx_u0_read_allow(void)
{
    if (_mt53xx_u0_trans_mode_on())
    {
        return GET_RX_DATA_SIZE(UART_READ32(UART0_STATUS));
    }
    else
    {
        return 1;
    }
}

static unsigned int _mt53xx_u0_write_allow(void)
{
    if (_mt53xx_u0_trans_mode_on())
    {
        return GET_TX_EMPTY_SIZE(UART_READ32(UART0_STATUS));
    }
    else
    {
        return 1;
    }
}

static void _mt53xx_u0_int_enable(int enable)
{
    unsigned long flags;

    local_irq_save(flags);
    if (enable)
    {
        UART_REG_BITSET(U0_INTALL, UART_INT_EN);
    }
    else
    {
        UART_REG_BITCLR(U0_INTALL, UART_INT_EN);
    }
    local_irq_restore(flags);
}

static void _mt53xx_u0_empty_int_enable(int enable)
{
    unsigned long flags;

    local_irq_save(flags);
    if (enable)
    {
        UART_REG_BITSET(U0_TBUF, UART_INT_EN);
    }
    else
    {
        UART_REG_BITCLR(U0_TBUF, UART_INT_EN);
    }
    local_irq_restore(flags);
}
#ifdef CC_KERNEL_NO_LOG
static struct timer_list uart0_timer;
static void restore_uart0(unsigned long);
unsigned int _forbid_uart0 = 0;
unsigned int _uart0_state = 0;

static void restore_uart0(unsigned long data)
{
    _forbid_uart0 = 0;
    //mod_timer(&uart0_timer, jiffies + 5 * HZ);
}

static void disable_uart0(unsigned long nsec)
{
    _forbid_uart0 = 1;
    uart0_timer.expires = jiffies + nsec * HZ;
    //add_timer(&uart0_timer);
}
static DEFINE_TIMER(uart0_timer, restore_uart0, 0, 0);
static int __init forbid_uart0(char* str)
{
    get_option(&str, &_forbid_uart0);
    return 0;
}
early_param("forbid_uart0", forbid_uart0);
#endif

unsigned int _perfor_uart0 = 0;

static int __init perfor_uart0(char* str)
{
    get_option(&str, &_perfor_uart0);
    return 0;
}
early_param("perfor_uart0", perfor_uart0);


static unsigned int _mt53xx_u0_read_byte(void)
{
    unsigned int i4Ret;
#ifdef CC_KERNEL_NO_LOG
    if (_forbid_uart0 == 1)
    {
        _forbid_uart0 = 0;
    }
#endif
    i4Ret = UART_READ32(UART0_DATA_BYTE);

    return i4Ret;
}

static void _mt53xx_u0_write_byte(unsigned int byte)
{
#ifdef CC_KERNEL_NO_LOG
    if (!_forbid_uart0)
    {
#ifdef ENABLE_UDMA
        if (DebugMesFlag)
#endif
        {
            UART_WRITE32(byte, UART0_DATA_BYTE);
        }
    }
#else
#ifdef ENABLE_UDMA
    if (DebugMesFlag)
#endif
    {
        UART_WRITE32(byte, UART0_DATA_BYTE);
    }
#endif
#ifdef CC_LOG2USB_SUPPORT
    _log2usb_write_byte(byte);
#endif
}

static void _mt53xx_u0_flush(void)
{
    _mt53xx_uart_flush(UART_PORT_0);
    _UART_HwWaitTxBufClear(UART_PORT_0);
}

static void _mt53xx_u0_get_top_err(int* p_parity, int* p_end, int* p_break)
{
    *p_parity = (UART_READ32(UART0_STATUS) & ERR_PARITY) ? 1 : 0;
    *p_end    = (UART_READ32(UART0_STATUS) & ERR_END)    ? 1 : 0;
    *p_break  = (UART_READ32(UART0_STATUS) & ERR_BREAK)  ? 1 : 0;
}

static unsigned int _mt53xx_u1_read_allow(void)
{
#ifndef CC_NO_KRL_UART_DRV
    return GET_RX_DATA_SIZE(UART_READ32(UART1_STATUS));
#else
    return 0;
#endif
}

static unsigned int _mt53xx_u1_write_allow(void)
{
#ifndef CC_NO_KRL_UART_DRV
    return GET_TX_EMPTY_SIZE(UART_READ32(UART1_STATUS));
#else
    return 0;
#endif
}

static void _mt53xx_u1_int_enable(int enable)
{
#ifdef ENABLE_UDMA
#if UART_DEBUG
    printk(KERN_ERR "u1 %d UART_INT_EN=%x,UART_READ32=%x\n", enable, UART_READ32(UART_INT_EN), UART_READ32(UART1_MODEM));
#endif
#endif

#ifndef CC_NO_KRL_UART_DRV
    unsigned long flags;

    local_irq_save(flags);
#ifdef ENABLE_UDMA
    if (enable)
    {
        if (_mt53xx_uart_ports[1].ms_enable)
        {
            UART_REG_BITSET(U1_WBUF_FULL | U1_WBUF_TOUT | U1_WBUF_OVER | U1_MODEM, UART_INT_EN);
        }
        else
        {
            UART_REG_BITSET(U1_WBUF_FULL | U1_WBUF_TOUT | U1_WBUF_OVER, UART_INT_EN);
        }
    }
    else
    {
        if (_mt53xx_uart_ports[1].ms_enable)
        {
            UART_REG_BITCLR(U1_WBUF_FULL | U1_WBUF_TOUT | U1_WBUF_OVER | U1_MODEM, UART_INT_EN);
        }
        else
        {
            UART_REG_BITCLR(U1_WBUF_FULL | U1_WBUF_TOUT | U1_WBUF_OVER, UART_INT_EN);
        }
    }
#else
    if (enable)
    {
        UART_REG_BITSET(U1_INTALL, UART_INT_EN);
    }
    else
    {
        UART_REG_BITCLR(U1_INTALL, UART_INT_EN);
    }
#endif
    local_irq_restore(flags);
#ifdef ENABLE_UDMA
#if UART_DEBUG
    printk(KERN_ERR "u1 UART_INT_EN 0x%x=0x%x\n", UART_INT_EN, UART_READ32(UART_INT_EN));
#endif
#endif

#else
    return;
#endif
}

static void _mt53xx_u1_empty_int_enable(int enable)
{
#ifdef ENABLE_UDMA
#if UART_DEBUG
    printk(KERN_ERR "empty %d UART_INT_EN=%x,UART_READ32=%x\n", enable, UART_READ32(UART_INT_EN), UART_READ32(UART1_MODEM));
#endif
#endif

#ifndef CC_NO_KRL_UART_DRV
    unsigned long flags;

    local_irq_save(flags);
    if (enable)
    {
        UART_REG_BITSET(U1_TBUF, UART_INT_EN);
    }
    else
    {
        UART_REG_BITCLR(U1_TBUF, UART_INT_EN);
    }
    local_irq_restore(flags);
#else
    return;
#endif
}

static unsigned int _mt53xx_u1_read_byte(void)
{
#ifndef CC_NO_KRL_UART_DRV
    unsigned int i4Ret;
    i4Ret = UART_READ32(UART1_DATA_BYTE);

    return i4Ret;
#else
    return 0;
#endif
}

static void _mt53xx_u1_write_byte(unsigned int byte)
{
#ifdef ENABLE_UDMA
#if UART_DEBUG
    unsigned int u4flag;
    u4flag = *((volatile unsigned int*)LOG_REGISTER);
    if (u4flag == LOG_PRINT_TX)
    {
        printk("tx: 0x%x(%c)\n", byte, byte);
    }
    txCount %= TX_BUFFER;
    txBuffer[txCount++] = byte;
#endif

#if 0
    if (yjdbg_txcnt > 0)
    {
        yjdbg_txcnt--;
        printk("yjdbg.tx.data: 0x%x\n", (unsigned char) byte);
    }
#endif
#endif

#ifndef CC_NO_KRL_UART_DRV
    UART_WRITE32(byte, UART1_DATA_BYTE);
#else
    return;
#endif
}

static void _mt53xx_u1_flush(void)
{
#ifndef CC_NO_KRL_UART_DRV
    _mt53xx_uart_flush(UART_PORT_1);
    _UART_HwWaitTxBufClear(UART_PORT_1);
#else
    return;
#endif
}

static void _mt53xx_u1_get_top_err(int* p_parity, int* p_end, int* p_break)
{
#ifndef CC_NO_KRL_UART_DRV
    *p_parity = (UART_READ32(UART1_STATUS) & ERR_PARITY) ? 1 : 0;
    *p_end    = (UART_READ32(UART1_STATUS) & ERR_END)    ? 1 : 0;
    *p_break  = (UART_READ32(UART1_STATUS) & ERR_BREAK)  ? 1 : 0;
#else
    return;
#endif
}

static unsigned int _mt53xx_u2_read_allow(void)
{
    if (_mt53xx_u2_trans_mode_on())
    {
        return GET_RX_DATA_SIZE(UART_READ32(UART2_STATUS));
    }
    else
    {
        return 1;
    }
}

static unsigned int _mt53xx_u2_write_allow(void)
{
    if (_mt53xx_u2_trans_mode_on())
    {
        return GET_TX_EMPTY_SIZE(UART_READ32(UART2_STATUS));
    }
    else
    {
        return 1;
    }
}

static void _mt53xx_u2_int_enable(int enable)
{
    unsigned long flags;

    local_irq_save(flags);
    if (enable)
    {
        unsigned int u4Reg;
        u4Reg = __raw_readl(pPdwncIoMap + 0x44);
        u4Reg |= 0x200;
        __raw_writel(u4Reg, pPdwncIoMap + 0x44);
        UART_REG_BITSET(U2_INTALL, UART2_INT_EN);
    }
    else
    {
        /* FIXME: disable PDWNC interrupt ? */
        UART_REG_BITCLR(U2_INTALL, UART2_INT_EN);
    }
    local_irq_restore(flags);
}

static void _mt53xx_u2_empty_int_enable(int enable)
{
    unsigned long flags;

    local_irq_save(flags);
    if (enable)
    {
        UART_REG_BITSET(U2_TBUF, UART2_INT_EN);
    }
    else
    {
        UART_REG_BITCLR(U2_TBUF, UART2_INT_EN);
    }
    local_irq_restore(flags);
}

static unsigned int _mt53xx_u2_read_byte(void)
{
    unsigned int i4Ret;
    i4Ret = UART_READ32(UART2_DATA_BYTE);

    return i4Ret;
}

static void _mt53xx_u2_write_byte(unsigned int byte)
{
    UART_WRITE32(byte, UART2_DATA_BYTE);
}

static void _mt53xx_u2_flush(void)
{
    _mt53xx_uart_flush(UART_PORT_2);
    _UART_HwWaitTxBufClear(UART_PORT_2);
}

static void _mt53xx_u2_get_top_err(int* p_parity, int* p_end, int* p_break)
{
    *p_parity = (UART_READ32(UART2_STATUS) & ERR_PARITY) ? 1 : 0;
    *p_end    = (UART_READ32(UART2_STATUS) & ERR_END)    ? 1 : 0;
    *p_break  = (UART_READ32(UART2_STATUS) & ERR_BREAK)  ? 1 : 0;
}

#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5891)
// UART_HT
static unsigned int _mt53xx_u4_read_allow(void)
{
    if (_mt53xx_u4_trans_mode_on())
    {
        return GET_RX_DATA_SIZE(UART_READ32(UART4_STATUS));
    }
    else
    {
        return 1;
    }
}

static unsigned int _mt53xx_u4_write_allow(void)
{
    if (_mt53xx_u4_trans_mode_on())
    {
        return GET_TX_EMPTY_SIZE(UART_READ32(UART4_STATUS));
    }
    else
    {
        return 1;
    }
}

static void _mt53xx_u4_int_enable(int enable)
{
    unsigned long flags;

    local_irq_save(flags);
    if (enable)
    {
        unsigned int u4Reg;
        u4Reg = __raw_readl(pPdwncIoMap + 0x44);// bit 5 is HT interrupt enable bit
        u4Reg |= 0x20;
        __raw_writel(u4Reg, pPdwncIoMap + 0x44);
        UART_REG_BITSET(U4_INTALL, UART4_INT_EN);
    }
    else
    {
        /* FIXME: disable PDWNC interrupt ? */
        UART_REG_BITCLR(U4_INTALL, UART4_INT_EN);
    }
    local_irq_restore(flags);
}

static void _mt53xx_u4_empty_int_enable(int enable)
{
    unsigned long flags;

    local_irq_save(flags);
    if (enable)
    {
        UART_REG_BITSET(U4_TBUF, UART4_INT_EN);
    }
    else
    {
        UART_REG_BITCLR(U4_TBUF, UART4_INT_EN);
    }
    local_irq_restore(flags);
}

static unsigned int _mt53xx_u4_read_byte(void)
{
    unsigned int i4Ret;
    i4Ret = UART_READ32(UART4_DATA_BYTE);

    return i4Ret;
}

static void _mt53xx_u4_write_byte(unsigned int byte)
{
    UART_WRITE32(byte, UART4_DATA_BYTE);
}

static void _mt53xx_u4_flush(void)
{
    _mt53xx_uart_flush(UART_PORT_4);
    _UART_HwWaitTxBufClear(UART_PORT_4);
}

static void _mt53xx_u4_get_top_err(int* p_parity, int* p_end, int* p_break)
{
    *p_parity = (UART_READ32(UART4_STATUS) & ERR_PARITY) ? 1 : 0;
    *p_end    = (UART_READ32(UART4_STATUS) & ERR_END)    ? 1 : 0;
    *p_break  = (UART_READ32(UART4_STATUS) & ERR_BREAK)  ? 1 : 0;
}

// UART_BT
static unsigned int _mt53xx_u5_read_allow(void)
{
    if (_mt53xx_u5_trans_mode_on())
    {
        return GET_RX_DATA_SIZE(UART_READ32(UART5_STATUS));
    }
    else
    {
        return 1;
    }
}

static unsigned int _mt53xx_u5_write_allow(void)
{
    if (_mt53xx_u5_trans_mode_on())
    {
        return GET_TX_EMPTY_SIZE(UART_READ32(UART5_STATUS));
    }
    else
    {
        return 1;
    }
}

static void _mt53xx_u5_int_enable(int enable)
{
    unsigned long flags;

    local_irq_save(flags);
    if (enable)
    {
        unsigned int u4Reg;
        u4Reg = __raw_readl(pPdwncIoMap + 0x44);
        u4Reg |= 0x40;
        __raw_writel(u4Reg, pPdwncIoMap + 0x44);
        UART_REG_BITSET(U5_INTALL, UART5_INT_EN);
    }
    else
    {
        /* FIXME: disable PDWNC interrupt ? */
        UART_REG_BITCLR(U5_INTALL, UART5_INT_EN);
    }
    local_irq_restore(flags);
}

static void _mt53xx_u5_empty_int_enable(int enable)
{
    unsigned long flags;

    local_irq_save(flags);
    if (enable)
    {
        UART_REG_BITSET(U5_TBUF, UART5_INT_EN);
    }
    else
    {
        UART_REG_BITCLR(U5_TBUF, UART5_INT_EN);
    }
    local_irq_restore(flags);
}

static unsigned int _mt53xx_u5_read_byte(void)
{
    unsigned int i4Ret;
    i4Ret = UART_READ32(UART5_DATA_BYTE);

    return i4Ret;
}

static void _mt53xx_u5_write_byte(unsigned int byte)
{
    UART_WRITE32(byte, UART5_DATA_BYTE);
}

static void _mt53xx_u5_flush(void)
{
    _mt53xx_uart_flush(UART_PORT_5);
    _UART_HwWaitTxBufClear(UART_PORT_5);
}

static void _mt53xx_u5_get_top_err(int* p_parity, int* p_end, int* p_break)
{
    *p_parity = (UART_READ32(UART5_STATUS) & ERR_PARITY) ? 1 : 0;
    *p_end    = (UART_READ32(UART5_STATUS) & ERR_END)    ? 1 : 0;
    *p_break  = (UART_READ32(UART5_STATUS) & ERR_BREAK)  ? 1 : 0;
}

#endif

/*
 * interrupt handling
 */
static void _mt53xx_uart_rx_chars(struct mt53xx_uart_port* mport)
{
    struct tty_port* tport;
    int max_count;
    unsigned int data_byte;
    unsigned int flag;
    int err_parity, err_end, err_break;

    if (!mport || !(&mport->port) || !(mport->port.state))
    {
        return;
    }

    tport = &mport->port.state->port;

    max_count = RX_BUF_SIZE;

    if (((mport->nport == 0) && !(_mt53xx_u0_trans_mode_on())) ||
        ((mport->nport == 2) && !(_mt53xx_u2_trans_mode_on()))
#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5891)
        || ((mport->nport == 4) && !(_mt53xx_u4_trans_mode_on()))
        || ((mport->nport == 5) && !(_mt53xx_u5_trans_mode_on()))
#endif
       )
    {
        max_count = 1;
    }

    while (max_count > 0)
    {
        max_count--;

        /* check status */
        if (mport->fn_read_allow())
        {
            /* in mt53xx, process error before read byte */
            mport->fn_get_top_err(&err_parity, &err_end, &err_break);

            /* read the byte */
            data_byte = mport->fn_read_byte();
            mport->port.icount.rx++;
            flag = TTY_NORMAL;

            /* error handling routine */
            if (err_break)
            {
                mport->port.icount.brk++;
                if (uart_handle_break(&mport->port))
                {
                    continue;
                }
                flag = TTY_BREAK;
            }
            else if (err_parity)
            {
                mport->port.icount.parity++;
                flag = TTY_PARITY;
            }
            else if (err_end)
            {
                mport->port.icount.frame++;
                flag = TTY_FRAME;
            }
        }
        else
        {
            break;
        }

        if (uart_handle_sysrq_char(&mport->port, data_byte))
        {
            continue;
        }
#if 0
        if (mport->nport == 1) // uart 1, fifo mode
        {
            if (yjdbg_rxcnt > 0)
            {
                yjdbg_rxcnt--;
                printk("yjdbg.rx.data: 0x%x\n", data_byte);
            }
        }
#endif

        uart_insert_char(&mport->port, 0, 0, data_byte, flag);
    }

    tty_flip_buffer_push(tport);
}

/*
 * interrupt handling
 */
#ifdef CONFIG_TTYMT7_SUPPORT
static void _mt53xx_uart_rx_chars_2(struct mt53xx_uart_port* mport,
                                    struct mt53xx_uart_port* mport_factory)
{
    struct tty_port* tport, *tport_factory = NULL;
    int max_count;
    unsigned int data_byte;
    unsigned int flag;
    int err_parity, err_end, err_break;
    int ttyMT0_has_char = 0;
    int ttyMT7_has_char = 0;
	#ifdef INCOMPLETE_CMD_SUPPORT
	unsigned int CMDData[RX_BUF_SIZE];
	unsigned int CMDFlag[RX_BUF_SIZE];
	unsigned int u4CMDRecvCount = 0;
	unsigned int u4CMDLength = 0;
	unsigned int i;
	#endif
#ifdef UART_DEBUG_RX_DATA
    unsigned int CMDData[RX_BUF_SIZE];
    unsigned int u4CMDRecvCount = 0;
    unsigned int i;
    memset(CMDData, 0, RX_BUF_SIZE * sizeof(unsigned int));
#endif
#ifdef INCOMPLETE_CMD_SUPPORT
	memset(CMDData, 0, RX_BUF_SIZE*sizeof(unsigned int));
	memset(CMDFlag, 0, RX_BUF_SIZE*sizeof(unsigned int));
#endif

    if (!mport || !(&mport->port) || !(mport->port.state) || !mport_factory)
    {
        return;
    }

    tport = &mport->port.state->port;
    if (mport_factory && (&mport_factory->port) && (mport_factory->port.state))
    {
        tport_factory = &mport_factory->port.state->port;
    }

    max_count = RX_BUF_SIZE;

    if (((mport->nport == 0) && !(_mt53xx_u0_trans_mode_on())) ||
        ((mport->nport == 2) && !(_mt53xx_u2_trans_mode_on()))
#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5891)
        || ((mport->nport == 4) && !(_mt53xx_u4_trans_mode_on()))
        || ((mport->nport == 5) && !(_mt53xx_u5_trans_mode_on()))
#endif
       )
    {
        max_count = 1;
    }

    while (max_count > 0)
    {
        max_count--;

        /* check status */
        if (!_enable_ttyMT7)
        {
            ttyMT0_has_char = 1;
            if (mport->fn_read_allow())
            {
                /* in mt5395, process error before read byte */
                mport->fn_get_top_err(&err_parity, &err_end, &err_break);

                /* read the byte */
                data_byte = mport->fn_read_byte();
#ifdef UART_DEBUG_RX_DATA
                CMDData[u4CMDRecvCount++] = data_byte;
#endif
                mport->port.icount.rx++;
                flag = TTY_NORMAL;

                if (_UART_CheckMagicChar(mport->nport, data_byte))
                {
                    ttyMT7_has_char = 1;
                    mport_factory->port.icount.rx++;
                    uart_insert_char(&mport_factory->port, 0, 0, data_byte, flag);
                }

                /* error handling routine */
                if (err_break)
                {
                    mport->port.icount.brk++;
                    if (uart_handle_break(&mport->port))
                    {
                        continue;
                    }
                    flag = TTY_BREAK;
                }
                else if (err_parity)
                {
                    mport->port.icount.parity++;
                    flag = TTY_PARITY;
                }
                else if (err_end)
                {
                    mport->port.icount.frame++;
                    flag = TTY_FRAME;
                }
            }
            else
            {
                break;
            }

            if (uart_handle_sysrq_char(&mport->port, data_byte))
            {
                continue;
            }

            //2.6.18. no overrun support, set status and mask to 0
            uart_insert_char(&mport->port, 0, 0, data_byte, flag);
        }
        else
        {
            if (mport_factory && mport_factory->fn_read_allow())
            {
            	#if 0	
				if((_u4FactoryWillStandby == 1)&&(hotel_mode_id == 4))
				{
					//if(hotel_mode_id == 4)//locatel mode
					{
						_forbid_uart0 = 0;
						_mt53xx_u0_write_byte(0x01);
						_mt53xx_u0_write_byte(0x05);
						_mt53xx_u0_write_byte(0xFF);
						_mt53xx_u0_write_byte(0x08);
						_mt53xx_u0_write_byte(0x00);
						_mt53xx_u0_write_byte(0x0D);
						_forbid_uart0 = 1;
					}
					#if 0
					if(hotel_mode_id == 3)//VDA mode
					{
						_mt53xx_u0_write_byte(0x70);
						_mt53xx_u0_write_byte(0x4);
						_mt53xx_u0_write_byte(0x74);
					}	
					#endif
				}
				else
				#endif
				{
                ttyMT7_has_char = 1;
                /* in mt5395, process error before read byte */
                mport_factory->fn_get_top_err(&err_parity, &err_end, &err_break);

                /* read the byte */
                data_byte = mport_factory->fn_read_byte();
                mport_factory->port.icount.rx++;
                flag = TTY_NORMAL;

                /* error handling routine */
                if (err_break)
                {
                    mport_factory->port.icount.brk++;
                    if (uart_handle_break(&mport_factory->port))
                    {
                        continue;
                    }
                    flag = TTY_BREAK;
                }
                else if (err_parity)
                {
                    mport_factory->port.icount.parity++;
                    flag = TTY_PARITY;
                }
                else if (err_end)
                {
                    mport_factory->port.icount.frame++;
                    flag = TTY_FRAME;
                }
				#ifdef INCOMPLETE_CMD_SUPPORT
				CMDData[u4CMDRecvCount] = data_byte;
				CMDFlag[u4CMDRecvCount++] = flag;
				#endif				
              }
            }
            else
            {
                break;
            }
#ifndef INCOMPLETE_CMD_SUPPORT
            //2.6.18. no overrun support, set status and mask to 0
            //if(tty_factory)
            if (tport_factory)
            {
                uart_insert_char(&mport_factory->port, 0, 0, data_byte, flag);
            }
#endif
        }
    }

#ifdef INCOMPLETE_CMD_SUPPORT// check whether there is incomplete cmd, if it is exist, drop it, only insert complete cmd.
	if(_u4FactoryWillStandby == 0)
	{
		if(tport_factory)
		{
			if(hotel_mode_id == 4)//locatel mode
			{
				if(u4CMDRecvCount == 9
					&& CMDData[0] == 0x80 && CMDData[1] == 0x8 && CMDData[2] == 0x32
					&& CMDData[3] == 0x4D && CMDData[4] == 0x47 && CMDData[5] == 0x54
					&& CMDData[6] == 0x49 && CMDData[7] == 0x50 && CMDData[8] == 0x3B)// a password
				{
					;
				}
				else if(u4CMDRecvCount == 4
						&& CMDData[0] == 0x81 && CMDData[1] == 0x3 && CMDData[2] == 0x80 && CMDData[3] == 0x4)// ISB data. means heart beat
				{
					;
				}
				else// may be other CMDs
				{
					if(u4CMDRecvCount < 3)
					{
						u4CMDRecvCount = 0;// drop all the data
					}
					else
					{
						u4CMDLength = CMDData[1];
						if(u4CMDLength != (u4CMDRecvCount - 1))
							u4CMDRecvCount = 0;// drop all the data
					}
				}
			}

			if(hotel_mode_id == 3)// VDA mode
			{
				if(u4CMDRecvCount == 6 && CMDData[0] == 0x83 && CMDData[3] == 0xFF && CMDData[4] == 0xFF)
				{
					;
				}
				else
				{
					if(u4CMDRecvCount < 6)// at least Header1 Header2 Type length(=1) data checksum.
					{
						u4CMDRecvCount = 0;// drop all the data
	        }
					else
					{
						u4CMDLength = CMDData[3];
						u4CMDLength += 4;// length is not include Header1 Header2 Type length.So Add 4 is total length.
						if(u4CMDLength != u4CMDRecvCount)
						{
							u4CMDRecvCount = 0;// drop all the data
						}
					}
	    }
			}

			for(i = 0; i < u4CMDRecvCount; i++)// insert data
			{
				uart_insert_char(&mport_factory->port, 0, 0, CMDData[i], CMDFlag[i]);
			}

			if((u4CMDRecvCount == 0) && (hotel_mode_id == 3))// VDA mode send error code [DTV00576204]
			{
				_mt53xx_u0_write_byte(0x70);
				_mt53xx_u0_write_byte(0x4);
				_mt53xx_u0_write_byte(0x74);
			}
		}
}
#endif

#ifdef INCOMPLETE_CMD_SUPPORT// check whether there is incomplete cmd, if it is exist, drop it, only insert complete cmd.
	if(_u4FactoryWillStandby == 0){
#endif

#ifdef UART_DEBUG_RX_DATA
    printk("[uart recv](%d)Start===>", u4CMDRecvCount);
    for (i = 0; i < u4CMDRecvCount; i++)
    {
        printk("[%d][0x%x][%c]", i, CMDData[i], CMDData[i]);
    }
    printk("<===[uart recv]End\n");
#endif

    if (ttyMT0_has_char)
    {
        tty_flip_buffer_push(tport);
    }
    if (ttyMT7_has_char && tport_factory)
    {
    	#ifdef INCOMPLETE_CMD_SUPPORT
		if(u4CMDRecvCount != 0)
		#endif
        tty_flip_buffer_push(tport_factory);
    }
	
#ifdef INCOMPLETE_CMD_SUPPORT// check whether there is incomplete cmd, if it is exist, drop it, only insert complete cmd.
}
#endif


#ifdef INCOMPLETE_CMD_SUPPORT  
else
{      
		if(u4CMDRecvCount != 0)
		{
			#if 1
			if(hotel_mode_id == 4)//locatel mode
			{
				_forbid_uart0 = 0;
				_mt53xx_u0_write_byte(0x01);
				_mt53xx_u0_write_byte(0x05);
				_mt53xx_u0_write_byte(0x08);
				_mt53xx_u0_write_byte(0xFF);
				_mt53xx_u0_write_byte(0x00);
				_mt53xx_u0_write_byte(0x0D);
				_forbid_uart0 = 1;

			}
			#endif

			if(hotel_mode_id == 3)//VDA mode
			{
				if(u4CMDRecvCount == 6 && CMDData[0] == 0x83 && CMDData[1] == 0x0 && CMDData[2] == 0x0
					&& CMDData[3] == 0xFF && CMDData[4] == 0xFF && CMDData[5] == 0x81)
				{
					_forbid_uart0 = 0;
					//70 00 02 00 72
					_mt53xx_u0_write_byte(0x70);
					_mt53xx_u0_write_byte(0x00);
					_mt53xx_u0_write_byte(0x02);
					_mt53xx_u0_write_byte(0x00);
					_mt53xx_u0_write_byte(0x72);
					_forbid_uart0 = 1;
				}
				else{
					_forbid_uart0 = 0;
					_mt53xx_u0_write_byte(0x70);
					_mt53xx_u0_write_byte(0x4);
					_mt53xx_u0_write_byte(0x74);
					_forbid_uart0 = 1;
				}
			}	
		    
		}

}
#endif
}
#endif

static void _mt53xx_uart_tx_chars(struct mt53xx_uart_port* mport)
{
    struct uart_port* port;
    struct circ_buf* xmit;
    int count;
	#if 1//def CC_LOG_PERFORMANCE_SUPPORT
	BUFFER_INFO *prRxFIFO;
	unsigned int u4RomeLeft;
	#endif

    if (!mport || !(&mport->port) || !mport->port.state)
    {
        return;
    }

    port = &mport->port;
    xmit = &port->state->xmit;

    if (!xmit)
    {
        return;
    }

    /* deal with x_char first */
    if (port->x_char)
    {
        /* make sure we have room to write */
        #if 1//def CC_LOG_PERFORMANCE_SUPPORT
		if(!_hUart0Thread)
		#endif
		{
	        while (!mport->fn_write_allow())
	        {
	            barrier();
	        }
	        mport->fn_write_byte(port->x_char);
		}
		#if 1//def CC_LOG_PERFORMANCE_SUPPORT
		else
		{
			prRxFIFO = &_arTxFIFO;
	        Buf_GetRoomLeft(prRxFIFO, u4RomeLeft);
			if(u4RomeLeft)
			{
	        	Buf_Push(prRxFIFO,(port->x_char));
				up(&uart0_log_sema);
			}
		}
		#endif
        port->icount.tx++;
        port->x_char = 0;
        return;
    }

    /* stop tx if circular buffer is empty or this port is stopped */
    if (uart_circ_empty(xmit) || uart_tx_stopped(port))
    {
        _mt53xx_uart_stop_tx(port);
        return;
    }

#ifdef ENABLE_UDMA
    count = TX_BUF_SIZE - 8;
#else
    count = TX_BUF_SIZE;
#endif

    while ((count > 0) &&
           (mport->fn_write_allow()) &&
           !(uart_circ_empty(xmit)))
    {
        barrier();
        count--;
        mport->fn_write_byte(xmit->buf[xmit->tail]);
        xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
        port->icount.tx++;
    }

    if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS)
    {
        uart_write_wakeup(port);
    }

    if (uart_circ_empty(xmit))
    {
        _mt53xx_uart_stop_tx(port);
    }
}

#ifdef ENABLE_UDMA
static void _mt53xx_uart1_tx_chars(struct mt53xx_uart_port* mport)
{
    struct uart_port* port;
    struct circ_buf* xmit;
    unsigned int phyAddr;
    int count;
    if (!mport || !(&mport->port) || !mport->port.state)
    {
        return;
    }
    port = &mport->port;
    xmit = &port->state->xmit;
    if (!xmit)
    {
        return;
    }
    if (port->x_char)
    {
        while (!mport->fn_write_allow())
        {
            barrier();
        }
        mport->fn_write_byte(port->x_char);
        port->icount.tx++;
        port->x_char = 0;
        return;
    }
    if (uart_circ_empty(xmit) || uart_tx_stopped(port))
    {
        _mt53xx_uart_stop_tx(port);
        return;
    }
    count = TX_BUF_SIZE;
    phyAddr = dma_map_single(NULL, xmit->buf, UART_XMIT_SIZE, DMA_BIDIRECTIONAL);
    while ((count > 0) &&
           ((mport->fn_write_allow() > 2)) &&
           !(uart_circ_empty(xmit)))
    {
        barrier();
        count--;
        mport->fn_write_byte(xmit->buf[xmit->tail]);
        xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
        port->icount.tx++;
    }
    dma_unmap_single(NULL, phyAddr, UART_XMIT_SIZE, DMA_BIDIRECTIONAL);
    if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS)
    {
        uart_write_wakeup(port);
    }

    if (uart_circ_empty(xmit))
    {
        _mt53xx_uart_stop_tx(port);
    }
}
#endif

// Fake
#ifdef CONFIG_TTYMT7_SUPPORT
static void _mt53xx_uart_tx_chars_2(struct mt53xx_uart_port* mport)
{
    struct uart_port* port;
    struct circ_buf* xmit;
    int count;

    if (!mport || !(&mport->port) || !mport->port.state)
    {
        return;
    }

    port = &mport->port;
    xmit = &port->state->xmit;

    if (!xmit)
    {
        return;
    }

    /* deal with x_char first */
    if (port->x_char)
    {
	#ifdef CC_LOG2USB_SUPPORT
        _log2usb_write_byte(port->x_char);
		#endif
        port->icount.tx++;
        port->x_char = 0;
        return;
    }

    /* stop tx if circular buffer is empty or this port is stopped */
    if (uart_circ_empty(xmit) || uart_tx_stopped(port))
    {
        _mt53xx_uart_stop_tx(port);
        return;
    }

    count = TX_BUF_SIZE;

    while ((count > 0) &&
           !(uart_circ_empty(xmit)))
    {
	#ifdef CC_LOG2USB_SUPPORT
        _log2usb_write_byte(xmit->buf[xmit->tail]);
        #else
		count--;
		#endif
        xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
        port->icount.tx++;
    }

    if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS)
    {
        uart_write_wakeup(port);
    }

    if (uart_circ_empty(xmit))
    {
        _mt53xx_uart_stop_tx(port);
    }
}
#endif

static void _mt53xx_uart_modem_state(struct mt53xx_uart_port* mport)
{
    struct uart_port* port;
    unsigned int status, delta;
    unsigned int uMS;

    if (!mport || !(&mport->port))
    {
        return;
    }

    port = &mport->port;

    switch (mport->nport)
    {
        case 0:
            uMS = UART0_MODEM;
            break;
        case 1:
            uMS = UART1_MODEM;
            break;
        default:
            return;
    }

    status = UART_READ32(uMS);        /* actually, support uart 1 only */
    status &= MDM_DSR | MDM_CTS | MDM_DCD;

    delta = status ^ mport->old_status;


    if (!delta)
    {
        return;
    }

    if (mport->ms_enable)
    {
        if (delta & MDM_DCD)
        {
            //        	printk("%s set MDM_DCD\n",__FUNCTION__);
            uart_handle_dcd_change(port, status & MDM_DCD);
        }
        if (delta & MDM_DSR)
        {
            port->icount.dsr++;
        }
        if (delta & MDM_CTS)
        {
            //printk("%s set MDM_CTS,status:0x%x,!stauts=%d\n",__FUNCTION__,status,!(status & MDM_CTS));
            //printk("%s 0xf000d60c=0x%x, 0x0c=0x%x,0xd4=0x%x\n",__FUNCTION__,__raw_readl(0XF000D60C),UART_READ32(UART_INT_EN),UART_READ32(UART1_MODEM));
            uart_handle_cts_change(port, (status & MDM_CTS));
        }

        wake_up_interruptible(&port->state->port.delta_msr_wait);
    }

    mport->old_status = status;
}

/*
 * uart ops
 */

/* called from thread context */
static unsigned int _mt53xx_uart_tx_empty(struct uart_port* port)
{
    struct mt53xx_uart_port* mport;
    unsigned int count;
    unsigned long flags;

    if (!port)
    {
        return 0;
    }

    mport = (struct mt53xx_uart_port*)port;
#ifdef CC_NO_KRL_UART_DRV
    if (mport->nport == 1)
    {
        return 0;
    }
#endif
    spin_lock_irqsave(&port->lock, flags);

#ifdef CONFIG_TTYMT7_SUPPORT
    if ((_enable_ttyMT7 && mport->nport == 0) || (!_enable_ttyMT7 && mport->nport == 3))
    {
        count = UART_FIFO_SIZE;
    }
    else
#endif
    {
        count = mport->fn_write_allow();
    }

    spin_unlock_irqrestore(&port->lock, flags);

    return ((count == UART_FIFO_SIZE) ? TIOCSER_TEMT : 0);
}

static void _mt53xx_uart_set_mctrl(struct uart_port* port, unsigned int mctrl)
{
    struct mt53xx_uart_port* mport;
    unsigned int regval;
    unsigned int uMS;
    unsigned long flags;
#ifdef ENABLE_UDMA
#if UART_DEBUG
    unsigned long u4Flag;
#endif
#endif

    if (!port)
    {
        return;
    }

    mport = (struct mt53xx_uart_port*)port;
    if (!mport->ms_enable)
    {
        return;
    }
#ifdef CC_NO_KRL_UART_DRV
    if (mport->nport == 1)
    {
        return;
    }
#endif
    switch (mport->nport)
    {
        case 0:
            uMS = UART0_MODEM;
            break;
        case 1:
            uMS = UART1_MODEM;
            break;
        default:
            return;
    }

#ifdef ENABLE_UDMA
#if !defined(ENABLE_DMA_CHK)
    local_irq_save(flags);

    regval = UART_READ32(uMS);

    if (mctrl & TIOCM_RTS)
    {
        regval |= MDM_RTS;
    }
    else
    {
        regval &= ~MDM_RTS;
    }

    UART_WRITE32(regval, uMS);
    local_irq_restore(flags);
#endif
#else
    local_irq_save(flags);

    regval = UART_READ32(uMS);

    if (mctrl & TIOCM_RTS)
    {
        regval |= MDM_RTS;
    }
    else
    {
        regval &= ~MDM_RTS;
    }

    if (mctrl & TIOCM_DTR)
    {
        regval |= MDM_DTR;
    }
    else
    {
        regval &= ~MDM_DTR;
    }
    UART_WRITE32(regval, uMS);
    local_irq_restore(flags);
#endif

#ifdef ENABLE_UDMA
#if UART_DEBUG
    u4Flag = *((volatile unsigned int*)LOG_REGISTER);
    if (u4Flag == LOG_RTSCTS_CHANGE)
    {
        printk(KERN_ERR "mctl, 0x%x=0x%x\n", uMS, UART_READ32(uMS));

    }
#endif
#endif
}

static unsigned int _mt53xx_uart_get_mctrl(struct uart_port* port)
{
    struct mt53xx_uart_port* mport;
    unsigned int modem_status;
    unsigned int result = 0;
    unsigned int uMS;

    if (!port)
    {
        return 0;
    }

    mport = (struct mt53xx_uart_port*)port;

    if (!mport->ms_enable)
    {
        return TIOCM_CAR | TIOCM_DSR | TIOCM_CTS;
    }

    switch (mport->nport)
    {
        case 0:
            uMS = UART0_MODEM;
            break;
        case 1:
            uMS = UART1_MODEM;
            break;
        default:
            // no modem control lines
            return TIOCM_CAR | TIOCM_DSR | TIOCM_CTS;
    }

    modem_status = UART_READ32(uMS);
    if (modem_status & MDM_DCD)
    {
        result |= TIOCM_CAR;
    }
#ifndef ENABLE_UDMA
    if (modem_status & MDM_DSR)
    {
        result |= TIOCM_DSR;
    }
    if (modem_status & MDM_CTS)
    {
        result |= TIOCM_CTS;
    }
#endif

    return result;
}

static void _mt53xx_uart_stop_tx(struct uart_port* port)
{
    struct mt53xx_uart_port* mport;
    unsigned long flags;

    if (!port)
    {
        return;
    }

    mport = (struct mt53xx_uart_port*)port;

    local_irq_save(flags);
    mport->tx_stop = 1;
    local_irq_restore(flags);
}

static void _mt53xx_uart_start_tx(struct uart_port* port)
{
    struct mt53xx_uart_port* mport;
    unsigned long flags;

    if (!port)
    {
        return;
    }

    mport = (struct mt53xx_uart_port*)port;

    local_irq_save(flags);
#ifdef CONFIG_TTYMT7_SUPPORT
    if ((!_enable_ttyMT7 && mport->nport == 3) || (_enable_ttyMT7 && mport->nport == 0))
    {
        _mt53xx_uart_tx_chars_2(mport);
        mport->tx_stop = 0;
        local_irq_restore(flags);
        return;
    }
#endif

    if (mport->fn_write_allow())
    {
        _mt53xx_uart_tx_chars(mport);
    }
    mport->tx_stop = 0;

    local_irq_restore(flags);
}

#ifdef ENABLE_UDMA
static void _mt53xx_uart_start1_tx(struct uart_port* port)
{
    struct mt53xx_uart_port* mport;
    unsigned long flags;
    if (!port)
    {
        return;
    }
    mport = (struct mt53xx_uart_port*)port;
    local_irq_save(flags);
    if (mport->fn_write_allow())
    {
        _mt53xx_uart1_tx_chars(mport);
    }
    mport->tx_stop = 0;
    local_irq_restore(flags);
}
#endif

static void _mt53xx_uart_stop_rx(struct uart_port* port)
{
    struct mt53xx_uart_port* mport;
    unsigned long flags;

    if (!port)
    {
        return;
    }

    mport = (struct mt53xx_uart_port*)port;

    local_irq_save(flags);
    mport->rx_stop = 1;
    local_irq_restore(flags);
}

static void _mt53xx_uart_enable_ms(struct uart_port* port)
{
    struct mt53xx_uart_port* mport;

    if (!port)
    {
        return;
    }

    mport = (struct mt53xx_uart_port*)port;

#ifdef DISABLE_MODEM_CONTROL
    mport->ms_enable = 0;
#endif
}

static void _mt53xx_uart_break_ctl(struct uart_port* port, int break_state)
{
    struct mt53xx_uart_port* mport;
    unsigned int regval;
    unsigned long flags;
    unsigned int uCOMMCTRL;

    if (!port)
    {
        return;
    }

    mport = (struct mt53xx_uart_port*)port;
#ifdef CC_NO_KRL_UART_DRV
    if (mport->nport == 1)
    {
        return;
    }
#endif
    switch (mport->nport)
    {
        case 0:
            uCOMMCTRL = UART0_COMMCTRL;
            break;
        case 1:
            uCOMMCTRL = UART1_COMMCTRL;
            break;
        case 2:
            uCOMMCTRL = UART2_COMMCTRL;
            break;
#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5891)
        case 4:
            uCOMMCTRL = UART4_COMMCTRL;
            break;
        case 5:
            uCOMMCTRL = UART5_COMMCTRL;
            break;
#endif
        default:
            return;
    }

    local_irq_save(flags);
    regval = UART_READ32(uCOMMCTRL);
    if (break_state == -1)
    {
        regval |= CONTROL_BREAK;
    }
    else
    {
        regval &= ~CONTROL_BREAK;
    }
    UART_WRITE32(regval, uCOMMCTRL);
    local_irq_restore(flags);
}

static int _u0_irq_allocated = 0;
static int _u1_irq_allocated = 0;
static int _u2_irq_allocated = 0;
static irqreturn_t _mt53xx_uart0_interrupt(int irq, void* dev_id);
static irqreturn_t _mt53xx_uart1_interrupt(int irq, void* dev_id);
static irqreturn_t _mt53xx_uart2_interrupt(int irq, void* dev_id);
#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5891)
static int _u4_irq_allocated = 0;
static int _u5_irq_allocated = 0;
static irqreturn_t _mt53xx_uart4_interrupt(int irq, void* dev_id);
static irqreturn_t _mt53xx_uart5_interrupt(int irq, void* dev_id);
#endif
static struct uart_port* port0_addr = NULL;

static int _mt53xx_uart_startup(struct uart_port* port)
{
    struct mt53xx_uart_port* mport;
    unsigned long flags;
    int retval;
    int u0irq, u1irq, u2irq;
#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5891)
    int u4irq, u5irq;
#endif

    if (!port)
    {
        return -1;
    }

    mport = (struct mt53xx_uart_port*)port;
    if (mport->nport == 0)
    {
        port0_addr = port;
    }
    // ignore if ttyMT3
#ifdef CONFIG_TTYMT7_SUPPORT
    if (mport->nport == 3)
    {
        return 0;
    }
#endif
#ifdef CC_NO_KRL_UART_DRV
    if (mport->nport == 1)
    {
        return 0;
    }
#endif
#ifdef CC_KERNEL_NO_LOG
    if (mport->nport == 0 && _forbid_uart0 == 1)
    {
        disable_uart0(20);
    }
#endif
    /* FIXME: hw init? */
#ifdef ENABLE_UDMA
    _mt53xx_uart_hwinit(mport, mport->nport);
    mport->hw_init = 1;
#else
    _mt53xx_uart_hwinit(mport->nport);
    _hw_init = 1;
#endif
    if (mport->nport == 0)
    {
        _u0_irq_allocated ++;
    }
    else if (mport->nport == 1)
    {
        _u1_irq_allocated ++;
    }
    else if (mport->nport == 2)
    {
        _u2_irq_allocated ++;
    }
#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5891)
    else if (mport->nport == 4)
    {
        _u4_irq_allocated ++;
    }
    else if (mport->nport == 5)
    {
        _u5_irq_allocated ++;
    }
#endif

    u0irq = _u0_irq_allocated;
    u1irq = _u1_irq_allocated;
    u2irq = _u2_irq_allocated;
#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5891)
    u4irq = _u4_irq_allocated;
    u5irq = _u5_irq_allocated;
#endif
    if ((mport->nport == 0) && (u0irq == 1))
    {
        /* allocate irq, u0/u1 share the same interrupt number */
        retval = request_irq(port->irq, _mt53xx_uart0_interrupt, IRQF_SHARED,
                             "MT53xx Serial 0", port);
        if (retval)
        {
            _u0_irq_allocated --;
            return retval;
        }
    }

    if ((mport->nport == 1) && (u1irq == 1))
    {
#ifdef ENABLE_UDMA
#ifndef U0_U1_SHARE_IRQ_HANDLER

        /* allocate irq, u0/u1 share the same interrupt number */
        //printk("_mt53xx_uart1_interrupt port->irq = 0x%x",port->irq);
        retval = request_irq(port->irq, _mt53xx_uart1_interrupt, IRQF_SHARED,
                             "MT53xx Serial 1", port);
#endif

#else
        /* allocate irq, u0/u1 share the same interrupt number */
        //printk("_mt53xx_uart1_interrupt port->irq = 0x%x",port->irq);
        retval = request_irq(port->irq, _mt53xx_uart1_interrupt, IRQF_SHARED,
                             "MT53xx Serial 1", port);
#endif
        if (retval)
        {
            _u1_irq_allocated --;
            return retval;
        }
    }

    if ((mport->nport == 2) && (u2irq == 1))
    {
        /* allocate irq, u0/u1 share the same interrupt number */
        retval = request_irq(port->irq, _mt53xx_uart2_interrupt, IRQF_SHARED,
                             "MT53xx Serial 2", port);
        if (retval)
        {
            _u2_irq_allocated --;
            return retval;
        }
    }

#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5891)
    if ((mport->nport == 4) && (u4irq == 1))
    {
        /* allocate irq, u0/u1 share the same interrupt number */
        retval = request_irq(port->irq, _mt53xx_uart4_interrupt, IRQF_SHARED,
                             "MT53xx Serial 4", port);
        if (retval)
        {
            _u4_irq_allocated --;
            return retval;
        }
    }

    if ((mport->nport == 5) && (u5irq == 1))
    {
        /* allocate irq, u0/u1 share the same interrupt number */
        retval = request_irq(port->irq, _mt53xx_uart5_interrupt, IRQF_SHARED,
                             "MT53xx Serial 5", port);
        if (retval)
        {
            _u5_irq_allocated --;
            return retval;
        }
    }
#endif
    /* enable interrupt */
    mport->fn_empty_int_enable(1);
    mport->fn_int_enable(1);

    local_irq_save(flags);
    if (!(__raw_readl(pBimIoMap + REG_RW_IRQEN) & (1 << port->irq)))
    {
        __raw_writel(__raw_readl(pBimIoMap + REG_RW_IRQEN) | (1 << port->irq),
                     pBimIoMap + REG_RW_IRQEN);
    }
    local_irq_restore(flags);
    return 0;
}

static void _mt53xx_uart_shutdown(struct uart_port* port)
{
    struct mt53xx_uart_port* mport;
    unsigned long flags;
    int u0irq, u1irq, u2irq;
#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5891)
    int u4irq, u5irq;
#endif
    if (!port)
    {
        return;
    }

    mport = (struct mt53xx_uart_port*)port;
#ifdef CC_NO_KRL_UART_DRV
    if (mport->nport == 1)
    {
        return;
    }
#endif
    // ignore if ttyMT3
#ifdef CONFIG_TTYMT7_SUPPORT
    if (mport->nport == 3)
    {
        return;
    }
#endif
#ifdef CC_KERNEL_NO_LOG
    if (mport->nport == 0)
    {
        del_timer(&uart0_timer);
    }
#endif

    /*
     * FIXME: disable BIM IRQ enable bit if all ports are shutdown?
     * PDWNC may need BIM interrupt
     */
    if (mport->nport == 0)
    {
        if (_u0_irq_allocated > 0)
        {
            _u0_irq_allocated --;
        }
    }
    else if (mport->nport == 1)
    {
        if (_u1_irq_allocated > 0)
        {
            _u1_irq_allocated --;
        }
    }
    else if (mport->nport == 2)
    {
        if (_u2_irq_allocated > 0)
        {
            _u2_irq_allocated --;
        }
    }
#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5891)
    else if (mport->nport == 4)
    {
        if (_u4_irq_allocated > 0)
        {
            _u4_irq_allocated --;
        }
    }
    else if (mport->nport == 5)
    {
        if (_u5_irq_allocated > 0)
        {
            _u5_irq_allocated --;
        }
    }
#endif

    u0irq = _u0_irq_allocated;
    u1irq = _u1_irq_allocated;
    u2irq = _u2_irq_allocated;
#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5891)
    u4irq = _u4_irq_allocated;
    u5irq = _u5_irq_allocated;
#endif
    if ((mport->nport == 0) && (u0irq == 0))
    {
        /* disable interrupt and disable port */
        mport->fn_empty_int_enable(0);
        mport->fn_int_enable(0);
        free_irq(port->irq, port);
    }

    if ((mport->nport == 1) && (u1irq == 0))
    {
        /* disable interrupt and disable port */
        mport->fn_empty_int_enable(0);
        mport->fn_int_enable(0);
#ifdef ENABLE_UDMA
#ifndef U0_U1_SHARE_IRQ_HANDLER
        free_irq(port->irq, port);
#endif
#else
        free_irq(port->irq, port);
#endif
    }

    if (((mport->nport == 0) || (mport->nport == 1)) &&
        (u0irq == 0) && (u1irq == 0))
    {
        local_irq_save(flags);
        __raw_writel(__raw_readl(pBimIoMap + REG_RW_IRQEN) & ~(1 << port->irq),
                     pBimIoMap + REG_RW_IRQEN);
        local_irq_restore(flags);
    }

    if ((mport->nport == 2) && (u2irq == 0))
    {
        /* disable interrupt and disable port */
        mport->fn_empty_int_enable(0);
        mport->fn_int_enable(0);
        free_irq(port->irq, port);
    }
#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5891)
    if ((mport->nport == 4) && (u4irq == 0))
    {
        /* disable interrupt and disable port */
        mport->fn_empty_int_enable(0);
        mport->fn_int_enable(0);
        free_irq(port->irq, port);
    }

    if ((mport->nport == 5) && (u5irq == 0))
    {
        /* disable interrupt and disable port */
        mport->fn_empty_int_enable(0);
        mport->fn_int_enable(0);
        free_irq(port->irq, port);
    }
#endif
    /* reset uart port */
#ifdef ENABLE_UDMA
    _mt53xx_uart_hwinit(mport, mport->nport);
    mport->hw_init = 0;
#else
    _mt53xx_uart_hwinit(mport->nport);

    _hw_init = 0;
#endif
}

static unsigned char _mt53xx_uart_cal_rxd_timeout(unsigned int baud)
{
    unsigned int wait_time;

    wait_time = 1000000 / (baud / 8);

    if (wait_time < (64 * 128 / 30))
    {
#ifdef ENABLE_UDMA
        if (wait_time * 30 / 128 == 0)
        {
            return 0x1;
        }
#endif
        return (wait_time * 30 / 128);
    }
    else if (wait_time < (64 * 512 / 30))
    {
        return (0x40 | (wait_time * 30 / 512));
    }
    else if (wait_time < (64 * 2048 / 30))
    {
        return (0x80 | (wait_time * 30 / 2048));
    }
    else if (wait_time < (64 * 8192 / 30))
    {
        return (0xC0 | (wait_time * 30 / 8192));
    }
    else
    {
        return 0xF;
    }
}

static void _mt53xx_uart_set_termios(struct uart_port* port,
                                     struct ktermios* termios, struct ktermios* old)
{
    struct mt53xx_uart_port* mport;
    unsigned long flags;
    int baud;
    int datalen;
    int parity = 0;
    int stopbit = 1;
    unsigned int uCOMMCTRL, uMS;
#ifdef ENABLE_UDMA
#ifndef DISABLE_MODEM_CONTROL
    unsigned int regval;
#endif
#endif

    if (!port)
    {
        return;
    }

    mport = (struct mt53xx_uart_port*)port;
#ifdef CC_NO_KRL_UART_DRV
    if (mport->nport == 1)
    {
        return;
    }
#endif

#ifdef ENABLE_UDMA
    if (mport->nport == 1)
    {
        mport->fn_int_enable(0);
    }
#endif

	if((mport->nport == 3 && _enable_ttyMT7 == 0) || (mport->nport == 0 && _enable_ttyMT7 == 1)) //if port = 3 but not in factory mode or port = 0 but  in factory mode ,do nothing
		return;


    switch (mport->nport)// only DBG & DMA has modem register!!
    {
        case 0:
#ifdef CONFIG_TTYMT7_SUPPORT
        case 3:
#endif
            uCOMMCTRL = UART0_COMMCTRL;
            uMS = UART0_MODEM;
            break;
        case 1:
            uCOMMCTRL = UART1_COMMCTRL;
            uMS = UART1_MODEM;
            break;
        default:
            return;
    }

    /* calculate baud rate */
    baud = (int)uart_get_baud_rate(port, termios, old, 0, port->uartclk);
    /* datalen : default 8 bit */
    switch (termios->c_cflag & CSIZE)
    {
        case CS5:
            datalen = 5;
            break;
        case CS6:
            datalen = 6;
            break;
        case CS7:
            datalen = 7;
            break;
        case CS8:
        default:
            datalen = 8;
            break;
    }

    /* stopbit : default 1 */
    if (termios->c_cflag & CSTOPB)
    {
        stopbit = 2;
    }
    /* parity : default none */
    if (termios->c_cflag & PARENB)
    {
        if (termios->c_cflag & PARODD)
        {
            parity = 1; /* odd */
        }
        else
        {
            parity = 2; /* even */
        }
    }

    //_mt53xx_uart_flush(mport->nport);
    if ((mport->baud != baud) ||
        (mport->datalen != datalen) ||
        (mport->stopbit != stopbit) ||
        (mport->parity != parity))
    {
        unsigned int wait_time;
        if (mport->baud != 0)
        {
            wait_time = 1000000 / (mport->baud / 8);
        }
        else
        {
            wait_time = 1000000 / (baud / 8);
        }

        udelay(wait_time * 2);

        mport->baud = baud;
        mport->datalen = datalen;
        mport->stopbit = stopbit;
        mport->parity = parity;
    }

    /* lock from here */
    spin_lock_irqsave(&port->lock, flags);

    /* update per port timeout */
    uart_update_timeout(port, termios->c_cflag, baud);
    /* read status mask */
    if (termios->c_iflag & INPCK)
    {
        /* frame error, parity error */
        port->read_status_mask |= UST_FRAME_ERROR | UST_PARITY_ERROR;
    }
    if (termios->c_iflag & (BRKINT | PARMRK))
    {
        /* break error */
        port->read_status_mask |= UST_BREAK_ERROR;
    }
    /* status to ignore */
    port->ignore_status_mask = 0;
    if (termios->c_iflag & IGNPAR)
    {
        port->ignore_status_mask |= UST_FRAME_ERROR | UST_PARITY_ERROR;
    }
    if (termios->c_iflag & IGNBRK)
    {
        port->ignore_status_mask |= UST_BREAK_ERROR;
        if (termios->c_iflag & IGNPAR)
        {
            port->ignore_status_mask |= UST_OVRUN_ERROR;
        }
    }
    if ((termios->c_cflag & CREAD) == 0)
    {
        // dummy read
        port->ignore_status_mask |= UST_DUMMY_READ;
    }

	
    _mt53xx_uart_set(uCOMMCTRL, baud, datalen, stopbit, parity);

    /* change rxd timeout period */
    switch (mport->nport)
    {
        case 0:
            UART_REG_BITCLR(RX_TOUT_CYCLE(0xFF) , UART0_BUFCTRL);
            UART_REG_BITSET(RX_TOUT_CYCLE(_mt53xx_uart_cal_rxd_timeout(baud)) , UART0_BUFCTRL);
            break;
        case 1:
            UART_REG_BITCLR(RX_TOUT_CYCLE(0xFF) , UART1_BUFCTRL);
            UART_REG_BITSET(RX_TOUT_CYCLE(_mt53xx_uart_cal_rxd_timeout(baud)) , UART1_BUFCTRL);
            break;
        case 2:
            UART_REG_BITCLR(RX_TOUT_CYCLE(0xFF) , UART2_BUFCTRL);
            UART_REG_BITSET(RX_TOUT_CYCLE(_mt53xx_uart_cal_rxd_timeout(baud)) , UART2_BUFCTRL);
            break;
#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5891)
        case 4:
            UART_REG_BITCLR(RX_TOUT_CYCLE(0xFF) , UART4_BUFCTRL);
            UART_REG_BITSET(RX_TOUT_CYCLE(_mt53xx_uart_cal_rxd_timeout(baud)) , UART4_BUFCTRL);
            break;
        case 5:
            UART_REG_BITCLR(RX_TOUT_CYCLE(0xFF) , UART5_BUFCTRL);
            UART_REG_BITSET(RX_TOUT_CYCLE(_mt53xx_uart_cal_rxd_timeout(baud)) , UART5_BUFCTRL);
            break;
#endif
        default:
            break;
    }

    /* hw rts/cts ? */
#ifndef DISABLE_MODEM_CONTROL
    if (((mport->nport == 0) || (mport->nport == 1)) && (termios->c_cflag & CRTSCTS))
    {
        UART_REG_BITSET(MDM_HW_RTS | MDM_HW_CTS | MODEM_RTS_TRIGGER_VALUE, uMS);
        mport->ms_enable = 1;
#ifdef ENABLE_UDMA
        if (mport->nport == 1)
        {
            regval = 0x4a01;//100byte;0x4201;//10byte//0xc701;
            UART_WRITE32(regval, UART1_BUFCTRL);
            printk("set 0xc0d0=0x%x,read c0d0=0x%x\n", regval, UART_READ32( UART1_BUFCTRL));
            regval = 0x7d003;//0x7d003;//2000;0x3e803;//1000,0x1f403;//500
            UART_WRITE32(regval, UART1_DMAW_CTRL);
            printk("set 0xc140=0x%x,read c140=0x%x\n", regval, UART_READ32( UART1_DMAW_CTRL));
            enable_ctsrts = 1;
#ifdef ENABLE_RX_TIMER
            printk("init uart Rx timer\n");
            mod_timer(&uart_rx_timer, jiffies + TIMER_INTERVAL);
            printk("rx_loss_data=%d\n", rx_loss_data);
#endif
        }
    }

    printk(KERN_ERR "CRTSCTS=0x%x, ms_enable=%d ,uart_enable_ms=%d\n", termios->c_cflag & CRTSCTS, mport->ms_enable, UART_ENABLE_MS(port, termios->c_cflag));
#endif

#endif

    /* unlock here */
#ifdef ENABLE_UDMA
    if (mport->nport == 1)
    {
        mport->fn_int_enable(1);
    }
#endif
    spin_unlock_irqrestore(&port->lock, flags);
}

static const char* _mt53xx_uart_type(struct uart_port* port)
{
    return "MT53xx Serial";
}

static void _mt53xx_uart_release_port(struct uart_port* port)
{
    release_mem_region(port->mapbase, MT53xx_UART_SIZE);
}

static int _mt53xx_uart_request_port(struct uart_port* port)
{
    void* pv_region;

    pv_region = request_mem_region(port->mapbase, MT53xx_UART_SIZE,
                                   "MT53xx Uart IO Mem");
    return pv_region != NULL ? 0 : -EBUSY;
}

static void _mt53xx_uart_config_port(struct uart_port* port, int flags)
{
    if (flags & UART_CONFIG_TYPE)
    {
        port->type = PORT_MT53xx;
        _mt53xx_uart_request_port(port);
    }
}

static int _mt53xx_uart_verify_port(struct uart_port* port,
                                    struct serial_struct* ser)
{
    int ret = 0;
    if (ser->type != PORT_UNKNOWN && ser->type != PORT_MT53xx)
    {
        ret = -EINVAL;
    }
    if (ser->irq != port->irq)
    {
        ret = -EINVAL;
    }
    if (ser->baud_base < 110)
    {
        ret = -EINVAL;
    }
    return ret;
}

static int _mt53xx_uart_ioctl(struct uart_port* port, unsigned int cmd,
                              unsigned long arg)
{
    unsigned int regval;
    unsigned long flags;
    int ret = 0;
    struct circ_buf* xmit;
#ifdef ENABLE_UDMA
    unsigned long tmp;
#endif

    if (cmd == 0xffff)
    {
#ifdef CONFIG_TTYMT7_SUPPORT
        if (arg == 0)
        {
            // leave factory mode.            
            spin_lock_irqsave(&port->lock, flags);
            _enable_ttyMT7 = 0;
            spin_unlock_irqrestore(&port->lock, flags);
        }
        else if (arg == 7)
        {
            // switch to factory mode.
            if (port0_addr)
            {
                xmit = &(port0_addr->state->xmit);
                while (!uart_circ_empty(xmit))
                {
                    msleep(5);
                }
                printk(KERN_ALERT "Switch factory mode done\n");
            }

            spin_lock_irqsave(&port->lock, flags);
            _enable_ttyMT7 = 1;
            spin_unlock_irqrestore(&port->lock, flags);
            up(&uart_sema);
        }
        else
        {
            ret = -EINVAL;
        }
#endif
    }
    else if (cmd == 0xfffe)
    {
#ifdef CONFIG_TTYMT7_SUPPORT
        spin_lock_irqsave(&port->lock, flags);
        _u4MagicInputForceFactory =  (unsigned int)arg;
        _u4NumMagicInputForceFactory = 1;
        spin_unlock_irqrestore(&port->lock, flags);
#endif
    }
    else if (cmd == 0xfffc)
    {
#ifdef CONFIG_TTYMT7_SUPPORT
        unsigned int  i;
        unsigned int au4Magic[6]; // num of magic + 5 magic
        if (copy_from_user(au4Magic, (void*)arg, (sizeof(unsigned int) * 6)))
        {
            printk(KERN_CRIT "_mt5365_uart_ioctl - copy_from_user() fail\n");
            ret = -EFAULT;
        }
        // num of magic == 0
        if (au4Magic[0] == 0)
        {
            return 0;
        }

        spin_lock_irqsave(&port->lock, flags);
        _u4MagicInputForceFactory =  (unsigned int)au4Magic[1];
        // Storing magic char 1~4 to array[0] ~ array[3]
        for (i = 1; i < (au4Magic[0] > 5 ? 5 : au4Magic[0]); i++)
        {
            _au4OtherMagicInputForceFactory[i - 1] = au4Magic[i + 1];
        }
        _u4NumMagicInputForceFactory = au4Magic[0] > 5 ? 5 : au4Magic[0];
        spin_unlock_irqrestore(&port->lock, flags);
#endif
    }
    else if (cmd == 0xfff9) // LG DDI_UART_DebugMes
    {
        spin_lock_irqsave(&port->lock, flags);

        _mt53xx_u0_flush();// flush TX and RX, so when customer enable TX, the TX log is up to date.

        regval = UART_READ32(UART0_COMMCTRL);
        if (arg == 0)// disable log
        {
            regval |= CONTROL_BREAK;
        }
        else if (arg == 1)// enable log
        {
            regval &= ~CONTROL_BREAK;
        }
        UART_WRITE32(regval, UART0_COMMCTRL);
        spin_unlock_irqrestore(&port->lock, flags);
    }
#ifdef ENABLE_UDMA
    else if (cmd == 0xfffb) /* enable/disable download*/
    {
        if (arg == 1)       //enable
        {
            UART_REG_BITCLR((1 << 31), 0x1000);	//0xf0028000+0x1000=0xf0029000
        }
        else if (arg == 0)     //disable
        {
            UART_REG_BITSET((1 << 31), 0x1000);
        }
        else
        {
            ret = -EINVAL;
        }
    }
    else if (cmd == 0xfffa) // enable/disable uart for accessing reg
    {
        if (arg == 1)       //enable
        {
            //__raw_writel(__raw_readl(0xf0008098) |(1<<6), 0xf0008098);  //bit6 set
            tmp = *((volatile unsigned int*)0xf0008098);
            tmp |= (1 << 6);
            *((volatile unsigned int*)0xf0008098) = tmp;
        }
        else if (arg == 0)     //disable   0xf0008098[6] = 0
        {
            //__raw_writel(__raw_readl(0xf0008098) & ~(1<<6), 0xf0008098);  //bit6 clean
            tmp = *((volatile unsigned int*)0xf0008098);
            tmp &= ~(1 << 6);
            *((volatile unsigned int*)0xf0008098) = tmp;
        }
        else
        {
            ret = -EINVAL;
        }
    }
    else if (cmd == 0xfff8) // enable/disable uart display debug message
    {
        if (arg == 1)       //enable
        {
            DebugMesFlag = 1;
        }
        else if (arg == 0)     //disable
        {
            DebugMesFlag = 0;
        }
        else
        {
            ret = -EINVAL;
        }
    }
#endif
    else
    {
        ret = -ENOIOCTLCMD;
    }
    return ret;
}

#ifdef ENABLE_UDMA
static struct uart_ops _mt53xx_uart_ops[] =
#else
static struct uart_ops _mt53xx_uart_ops =
#endif
{
#ifdef ENABLE_UDMA
    {
#endif
        .tx_empty       = _mt53xx_uart_tx_empty,
        .set_mctrl      = _mt53xx_uart_set_mctrl,
        .get_mctrl      = _mt53xx_uart_get_mctrl,
        .stop_tx        = _mt53xx_uart_stop_tx,
        .start_tx       = _mt53xx_uart_start_tx,
        /* .send_xchar */
        .stop_rx        = _mt53xx_uart_stop_rx,
        .enable_ms      = _mt53xx_uart_enable_ms,
        .break_ctl      = _mt53xx_uart_break_ctl,
        .startup        = _mt53xx_uart_startup,
        .shutdown       = _mt53xx_uart_shutdown,
        .set_termios    = _mt53xx_uart_set_termios,
        /* .pm */
        /* .set_wake */
        .type           = _mt53xx_uart_type,
        .release_port   = _mt53xx_uart_release_port,
        .request_port   = _mt53xx_uart_request_port,
        .config_port    = _mt53xx_uart_config_port,
        .verify_port    = _mt53xx_uart_verify_port,
        .ioctl			= _mt53xx_uart_ioctl,
#ifdef ENABLE_UDMA
    },
    {
        .tx_empty       = _mt53xx_uart_tx_empty,
        .set_mctrl      = _mt53xx_uart_set_mctrl,
        .get_mctrl      = _mt53xx_uart_get_mctrl,
        .stop_tx        = _mt53xx_uart_stop_tx,
        .start_tx       = _mt53xx_uart_start1_tx,
        .stop_rx        = _mt53xx_uart_stop_rx,
        .enable_ms      = _mt53xx_uart_enable_ms,
        .break_ctl      = _mt53xx_uart_break_ctl,
        .startup        = _mt53xx_uart_startup,
        .shutdown       = _mt53xx_uart_shutdown,
        .set_termios    = _mt53xx_uart_set_termios,
        .type           = _mt53xx_uart_type,
        .release_port   = _mt53xx_uart_release_port,
        .request_port   = _mt53xx_uart_request_port,
        .config_port    = _mt53xx_uart_config_port,
        .verify_port    = _mt53xx_uart_verify_port,
        .ioctl 			= _mt53xx_uart_ioctl,
    },
#endif
};

static struct mt53xx_uart_port _mt53xx_uart_ports[] =
{
    {
        .port =
        {
            .membase        = (void*)MT53xx_VA_UART,
            .mapbase        = MT53xx_PA_UART,
            .iotype         = SERIAL_IO_MEM,
            .irq            = VECTOR_RS232_0,
            .uartclk        = 921600,
            .fifosize       = TX_BUF_SIZE,
#ifdef ENABLE_UDMA
            .ops            = &_mt53xx_uart_ops[0],
#else
            .ops			= &_mt53xx_uart_ops,
#endif
            .flags          = ASYNC_BOOT_AUTOCONF,
            .line           = 0,
            .lock           = __SPIN_LOCK_UNLOCKED(_mt53xx_uart_ports[0].port.lock),
        },
        .nport              = 0,
        .old_status         = 0,
        .tx_stop            = 0,
        .rx_stop            = 0,
        .ms_enable          = 0,
        .fn_read_allow      = _mt53xx_u0_read_allow,
        .fn_write_allow     = _mt53xx_u0_write_allow,
        .fn_int_enable      = _mt53xx_u0_int_enable,
        .fn_empty_int_enable    = _mt53xx_u0_empty_int_enable,
        .fn_read_byte       = _mt53xx_u0_read_byte,
        .fn_write_byte      = _mt53xx_u0_write_byte,
        .fn_flush           = _mt53xx_u0_flush,
        .fn_get_top_err     = _mt53xx_u0_get_top_err,
    },
    {
        .port =
        {
            .membase        = (void*)MT53xx_VA_UART,
            .mapbase        = MT53xx_PA_UART,
            .iotype         = SERIAL_IO_MEM,
#ifndef CC_NO_KRL_UART_DRV
            .irq			= VECTOR_RS232_1,
#else
            .irq			= VECTOR_RS232_INVALID,
#endif
#ifdef ENABLE_UDMA
            .uartclk        = 3000000,
#else
            .uartclk        = 921600,
#endif
            .fifosize       = TX_BUF_SIZE,
#ifdef ENABLE_UDMA
            .ops			= &_mt53xx_uart_ops[1],
#else
            .ops			= &_mt53xx_uart_ops,
#endif
            .flags          = ASYNC_BOOT_AUTOCONF,
            .line           = 1,
            .lock           = __SPIN_LOCK_UNLOCKED(_mt53xx_uart_ports[1].port.lock),
        },
        .nport              = 1,
        .old_status         = 0,
        .tx_stop            = 0,
        .rx_stop            = 0,
        .ms_enable          = 0,
        .fn_read_allow      = _mt53xx_u1_read_allow,
        .fn_write_allow     = _mt53xx_u1_write_allow,
        .fn_int_enable      = _mt53xx_u1_int_enable,
        .fn_empty_int_enable    = _mt53xx_u1_empty_int_enable,
        .fn_read_byte       = _mt53xx_u1_read_byte,
        .fn_write_byte      = _mt53xx_u1_write_byte,
        .fn_flush           = _mt53xx_u1_flush,
        .fn_get_top_err     = _mt53xx_u1_get_top_err,
    },
    {
        .port =
        {
            .membase        = (void*)MT53xx_VA_UART2,
            .mapbase        = MT53xx_PA_UART2,
            .iotype         = SERIAL_IO_MEM,
            .irq            = VECTOR_RS232_2,
            .uartclk        = 921600,
            .fifosize       = TX_BUF_SIZE,
#ifdef ENABLE_UDMA
            .ops			= &_mt53xx_uart_ops[0],
#else
            .ops			= &_mt53xx_uart_ops,
#endif
            .flags          = ASYNC_BOOT_AUTOCONF,
            .line           = 2,
            .lock           = __SPIN_LOCK_UNLOCKED(_mt53xx_uart_ports[2].port.lock),
        },
        .nport              = 2,
        .old_status         = 0,
        .tx_stop            = 0,
        .rx_stop            = 0,
        .ms_enable          = 0,
        .fn_read_allow      = _mt53xx_u2_read_allow,
        .fn_write_allow     = _mt53xx_u2_write_allow,
        .fn_int_enable      = _mt53xx_u2_int_enable,
        .fn_empty_int_enable = _mt53xx_u2_empty_int_enable,
        .fn_read_byte       = _mt53xx_u2_read_byte,
        .fn_write_byte      = _mt53xx_u2_write_byte,
        .fn_flush           = _mt53xx_u2_flush,
        .fn_get_top_err     = _mt53xx_u2_get_top_err,
    },
#ifdef CONFIG_TTYMT7_SUPPORT
    {
        .port =
        {
            .membase        = (void*)MT53xx_VA_UART,
            .mapbase        = MT53xx_PA_UART,
            .iotype         = SERIAL_IO_MEM,
            .irq            = VECTOR_RS232_0,
            .uartclk        = 921600,
            .fifosize       = TX_BUF_SIZE,
#ifdef ENABLE_UDMA
            .ops			= &_mt53xx_uart_ops[0],
#else
            .ops			= &_mt53xx_uart_ops,
#endif
            .flags          = ASYNC_BOOT_AUTOCONF,
            .line           = 3,
            .lock           = __SPIN_LOCK_UNLOCKED(_mt53xx_uart_ports[3].port.lock),
        },
        .nport              = 3,
        .old_status         = 0,
        .tx_stop            = 0,
        .rx_stop            = 0,
        .ms_enable          = 0,
        .fn_read_allow      = _mt53xx_u0_read_allow,
        .fn_write_allow     = _mt53xx_u0_write_allow,
        .fn_int_enable      = _mt53xx_u0_int_enable,
        .fn_empty_int_enable = _mt53xx_u0_empty_int_enable,
        .fn_read_byte       = _mt53xx_u0_read_byte,
        .fn_write_byte      = _mt53xx_u0_write_byte,
        .fn_flush           = _mt53xx_u0_flush,
        .fn_get_top_err     = _mt53xx_u0_get_top_err,
    },
#endif

#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5891)
    {
        .port =
        {
            .membase		= (void*)MT53xx_VA_UART4,
            .mapbase		= MT53xx_PA_UART4,
            .iotype 		= SERIAL_IO_MEM,
            .irq			= VECTOR_RS232_2,
            .uartclk		= 921600,
            .fifosize		= TX_BUF_SIZE,
#ifdef ENABLE_UDMA
            .ops			= &_mt53xx_uart_ops[0],
#else
            .ops			= &_mt53xx_uart_ops,
#endif
            .flags			= ASYNC_BOOT_AUTOCONF,
            .line			= 4,
            .lock			= __SPIN_LOCK_UNLOCKED(_mt53xx_uart_ports[4].port.lock),
        },
        .nport				= 4,
        .old_status 		= 0,
        .tx_stop			= 0,
        .rx_stop			= 0,
        .ms_enable			= 0,
        .fn_read_allow		= _mt53xx_u4_read_allow,
        .fn_write_allow 	= _mt53xx_u4_write_allow,
        .fn_int_enable		= _mt53xx_u4_int_enable,
        .fn_empty_int_enable = _mt53xx_u4_empty_int_enable,
        .fn_read_byte		= _mt53xx_u4_read_byte,
        .fn_write_byte		= _mt53xx_u4_write_byte,
        .fn_flush			= _mt53xx_u4_flush,
        .fn_get_top_err 	= _mt53xx_u4_get_top_err,
    },

    {
        .port =
        {
            .membase		= (void*)MT53xx_VA_UART5,
            .mapbase		= MT53xx_PA_UART5,
            .iotype 		= SERIAL_IO_MEM,
            .irq			= VECTOR_RS232_2,
            .uartclk		= 921600,
            .fifosize		= TX_BUF_SIZE,
#ifdef ENABLE_UDMA
            .ops			= &_mt53xx_uart_ops[0],
#else
            .ops			= &_mt53xx_uart_ops,
#endif
            .flags			= ASYNC_BOOT_AUTOCONF,
            .line			= 5,
            .lock			= __SPIN_LOCK_UNLOCKED(_mt53xx_uart_ports[5].port.lock),
        },
        .nport				= 5,
        .old_status 		= 0,
        .tx_stop			= 0,
        .rx_stop			= 0,
        .ms_enable			= 0,
        .fn_read_allow		= _mt53xx_u5_read_allow,
        .fn_write_allow 	= _mt53xx_u5_write_allow,
        .fn_int_enable		= _mt53xx_u5_int_enable,
        .fn_empty_int_enable = _mt53xx_u5_empty_int_enable,
        .fn_read_byte		= _mt53xx_u5_read_byte,
        .fn_write_byte		= _mt53xx_u5_write_byte,
        .fn_flush			= _mt53xx_u5_flush,
        .fn_get_top_err 	= _mt53xx_u5_get_top_err,
    },
#endif
};

#ifdef ENABLE_UDMA
static irqreturn_t _mt53xx_uart0_interrupt(int irq, void* dev_id)
{
    unsigned int uart_int_ident;
    struct mt53xx_uart_port* mport = (struct mt53xx_uart_port*)dev_id;
#ifdef CONFIG_TTYMT7_SUPPORT
    struct mt53xx_uart_port* mport_factory;
#endif
#ifdef U0_U1_SHARE_IRQ_HANDLER
    // Special
    struct mt53xx_uart_port* mport1 = &_mt53xx_uart_ports[1];
#endif
    irqreturn_t ret = IRQ_NONE;

#ifdef CONFIG_TTYMT7_SUPPORT
    mport = &_mt53xx_uart_ports[0];
    mport_factory = &_mt53xx_uart_ports[3];
#endif
    uart_int_ident = UART_READ32(UART_INT_ID);
    uart_int_ident &= U0_INTMSK;

    /* take care of SA_SHIRQ and return IRQ_NONE if possible [LDD/279] */

    /* ack u0 interrupt */
    UART_WRITE32(~uart_int_ident, UART_INT_ID);
    if (mport != &_mt53xx_uart_ports[0])
    {
        while (1)
            ;
    }
    //_mt53xx_handle_uart_interrupt(mport, regs, uart_int_ident & U0_TBUF);

    /* rx mode */
    if (mport->fn_read_allow() || (uart_int_ident & (U0_RBUF | U0_RERR)))
    {
#ifdef CONFIG_TTYMT7_SUPPORT
        _mt53xx_uart_rx_chars_2(mport, mport_factory);
#else
        _mt53xx_uart_rx_chars(mport);
#endif
        ret = IRQ_HANDLED;
    }

    /* tx mode */
#ifdef CONFIG_TTYMT7_SUPPORT
    if (!_enable_ttyMT7)
    {
        if (mport->fn_write_allow() || (uart_int_ident & U0_TBUF))
        {
            _mt53xx_uart_tx_chars(mport);
            ret = IRQ_HANDLED;
        }
    }
    else
    {
        if (mport_factory->fn_write_allow() || (uart_int_ident & U0_TBUF))
        {
            _mt53xx_uart_tx_chars(mport_factory);
            ret = IRQ_HANDLED;
        }

    }
#else
    if (mport->fn_write_allow() || (uart_int_ident & U0_TBUF))
    {
        _mt53xx_uart_tx_chars(mport);
        ret = IRQ_HANDLED;
    }
#endif

    /* modem mode */
    _mt53xx_uart_modem_state(mport);
#ifdef U0_U1_SHARE_IRQ_HANDLER
    // Special
    uart_int_ident = UART_READ32(UART_INT_ID);
    uart_int_ident &= U1_INTMSK;
    /* ack u1 interrupt */
    UART_WRITE32(~uart_int_ident, UART_INT_ID);

    if (mport1 == NULL || mport1->port.state == NULL || mport1->port.state->port.tty == NULL)
    {
        return ret;
    }

    if (uart_int_ident & (U1_WBUF_TOUT | U1_WBUF_FULL | U1_WBUF_OVER))
    {
        unsigned int u4RxCnt;
        u4RxCnt = _mt5395_u1_HwGetRxDataCnt();
        if (u4RxCnt > 0)
        {
            struct tty_port* tport;
            unsigned int i;
            tport = &mport->port.state->port;

            u4RxCnt = _mt5395_u1_ReadRxFifo(mport1, mport1->pu1DrvBuffer, u4RxCnt, u4RxCnt, uart_int_ident & (U1_WBUF_TOUT));
#ifndef CYJ_REMOVE_ONE_COPY
            /* read the byte */
            for (i = 0; i < u4RxCnt; i++)
            {
                uart_insert_char(&mport1->port, 0, 0, *(mport1->pu1DrvBuffer + i), TTY_NORMAL);
#if 0
                if (yjdbg_rxcnt > 0)
                {
                    yjdbg_rxcnt--;
                    printk("yjdbg.rx.data: 0x%x\n", *(mport1->pu1DrvBuffer + i));
                }
#endif
                mport1->port.icount.rx++;
            }
#endif
            tty_flip_buffer_push(tport);
        }
        ret = IRQ_HANDLED;
    }
    if (uart_int_ident & (U1_WBUF_FULL))
    {
        yjdbg_dram_full++;
        //printk("yjdbg_dram_full: 0x%x\n", yjdbg_dram_full);
    }
    if (uart_int_ident & (U1_OVRUN))
    {
        yjdbg_fifo_full++;
        //printk("yjdbg_fifo_full: 0x%x\n", yjdbg_fifo_full);
    }
    /* tx mode */
    if (mport1->fn_write_allow() || (uart_int_ident & U1_TBUF))
    {
        _mt53xx_uart_tx_chars(mport1);
        ret = IRQ_HANDLED;
    }

    /* modem mode */
    _mt53xx_uart_modem_state(mport1);
#endif
    return ret;
}
#else
static irqreturn_t _mt53xx_uart0_interrupt(int irq, void* dev_id)
{
    unsigned int uart_int_ident;
    struct mt53xx_uart_port* mport = (struct mt53xx_uart_port*)dev_id;
#ifdef CONFIG_TTYMT7_SUPPORT
    struct mt53xx_uart_port* mport_factory;
#endif
    irqreturn_t ret = IRQ_NONE;

#ifdef CONFIG_TTYMT7_SUPPORT
    mport = &_mt53xx_uart_ports[0];
    mport_factory = &_mt53xx_uart_ports[3];
#endif
    uart_int_ident = UART_READ32(UART_INT_ID);
    uart_int_ident &= U0_INTMSK;

    /* take care of SA_SHIRQ and return IRQ_NONE if possible [LDD/279] */

    /* ack u0 interrupt */
    UART_WRITE32(~uart_int_ident, UART_INT_ID);
    if (mport != &_mt53xx_uart_ports[0])
    {
        while (1)
            ;
    }

    /* rx mode */
    if (mport->fn_read_allow() || (uart_int_ident & (U0_RBUF | U0_RERR)))
    {
#ifdef CONFIG_TTYMT7_SUPPORT
        _mt53xx_uart_rx_chars_2(mport, mport_factory);
#else
        _mt53xx_uart_rx_chars(mport);
#endif
        ret = IRQ_HANDLED;
    }

    /* tx mode */
#ifdef CONFIG_TTYMT7_SUPPORT
    if (!_enable_ttyMT7)
    {
        if (mport->fn_write_allow() || (uart_int_ident & U0_TBUF))
        {
            _mt53xx_uart_tx_chars(mport);
            ret = IRQ_HANDLED;
        }
    }
    else
    {
        if (mport_factory->fn_write_allow() || (uart_int_ident & U0_TBUF))
        {
            _mt53xx_uart_tx_chars(mport_factory);
            ret = IRQ_HANDLED;
        }

    }
#else
    if (mport->fn_write_allow() || (uart_int_ident & U0_TBUF))
    {
        _mt53xx_uart_tx_chars(mport);
        ret = IRQ_HANDLED;
    }
#endif

    /* modem mode */
    _mt53xx_uart_modem_state(mport);

    return ret;
}
#endif

#ifdef ENABLE_UDMA
int all_interrupt;
#endif

#ifdef ENABLE_UDMA
static irqreturn_t _mt53xx_uart1_interrupt(int irq, void* dev_id)
{
#ifndef U0_U1_SHARE_IRQ_HANDLER
    unsigned int uart_int_ident;
    unsigned long flags, u4Flag;

    struct mt53xx_uart_port* mport = (struct mt53xx_uart_port*)dev_id;
    irqreturn_t ret = IRQ_NONE;
    local_irq_save(flags);

    all_interrupt++;
    uart_int_ident = UART_READ32(UART_INT_ID);
    uart_int_ident &= U1_INTMSK;

    /* take care of SA_SHIRQ and return IRQ_NONE if possible [LDD/279] */
    /* ack u1 interrupt */
    UART_WRITE32(~uart_int_ident, UART_INT_ID);
    if (mport != &_mt53xx_uart_ports[1])
    {
        return  IRQ_HANDLED ;
    }

    if (uart_int_ident & (U1_WBUF_FULL))
    {
        yjdbg_dram_full++;
    }
    if (uart_int_ident & (U1_OVRUN))
    {
        yjdbg_fifo_full++;
        mport->port.icount.overrun++;
        // printk("ff:%d, df=%d,dt:%d,do:%d ", yjdbg_fifo_full,yjdbg_dram_full,yjdbg_dmaw_trig,yjdbg_dmaw_tout);
        //	printk("all:%d, wp:%d,rp:%d,wL:%d\n",all_interrupt,  UART_READ32(UART1_DMAW_WPTR),UART_READ32(UART1_DMAW_RPTR),UART_READ32(UART1_DMAW_LEVEL));
        // printk("ff:%d, all=%d,dt:%d,do:%d \n", yjdbg_fifo_full,all_interrupt,yjdbg_dmaw_trig,yjdbg_dmaw_tout);
    }
    if (uart_int_ident & (U1_WBUF_OVER))
    {
        yjdbg_dmaw_trig++;
    }
    if (uart_int_ident & (U1_WBUF_TOUT))
    {
        yjdbg_dmaw_tout++;
    }
    if (uart_int_ident & U1_MODEM)
    {
        u4Flag = *((volatile unsigned int*)LOG_REGISTER);
        if (u4Flag == LOG_RTSCTS_CHANGE)
        {
            printk("CTS :U1_M=0x%x,int=0x%x\n", UART_READ32(UART1_MODEM), uart_int_ident );
        }
    }
    // dmx rx mode
#ifdef ENABLE_UDMA
    if (uart_int_ident & (U1_WBUF_TOUT | U1_WBUF_FULL | U1_WBUF_OVER))
    {
        unsigned int u4RxCnt;
        u4RxCnt = _mt5395_u1_HwGetRxDataCnt();
        if (u4RxCnt > 0)
        {
            struct tty_port* tport;
            tport = &mport->port.state->port;
            u4RxCnt = _mt5395_u1_ReadRxFifo(mport, mport->pu1DrvBuffer, u4RxCnt, uart_int_ident & (U1_WBUF_TOUT));
#ifndef CYJ_REMOVE_ONE_COPY
            /* read the byte */
            unsigned int i;

            for (i = 0; i < u4RxCnt; i++)
            {
                uart_insert_char(&mport->port, 0, 0, *(mport->pu1DrvBuffer + i), TTY_NORMAL);
                mport->port.icount.rx++;
            }
#endif
            tty_flip_buffer_push(tport);
        }
        ret = IRQ_HANDLED;
    }
#else
    /* rx mode */
    if (mport->fn_read_allow() || (uart_int_ident & (U1_RBUF | U1_RERR)))
    {
        _mt53xx_uart_rx_chars(mport);
        ret = IRQ_HANDLED;
    }
#endif

    /* tx mode */
    spin_lock(&mport->port.lock);
    if (mport->fn_write_allow() || (uart_int_ident & U1_TBUF))
    {
        _mt53xx_uart1_tx_chars(mport);
        ret = IRQ_HANDLED;
    }
    spin_unlock(&mport->port.lock);

    /* modem mode */
    _mt53xx_uart_modem_state(mport);
    local_irq_restore(flags);
    return ret;
#else
    return IRQ_HANDLED;
#endif
}

#else
static irqreturn_t _mt53xx_uart1_interrupt(int irq, void* dev_id)
{
    unsigned int uart_int_ident;
    struct mt53xx_uart_port* mport = (struct mt53xx_uart_port*)dev_id;
    irqreturn_t ret = IRQ_NONE;

    uart_int_ident = UART_READ32(UART_INT_ID);
    uart_int_ident &= U1_INTMSK;

    /* take care of SA_SHIRQ and return IRQ_NONE if possible [LDD/279] */

    /* ack u1 interrupt */
    UART_WRITE32(~uart_int_ident, UART_INT_ID);
    if (mport != &_mt53xx_uart_ports[1])
    {
        while (1)
            ;
    }

    /* rx mode */
    if (mport->fn_read_allow() || (uart_int_ident & (U1_RBUF | U1_RERR)))
    {
        _mt53xx_uart_rx_chars(mport);
        ret = IRQ_HANDLED;
    }

    /* tx mode */
    if (mport->fn_write_allow() || (uart_int_ident & U1_TBUF))
    {
        _mt53xx_uart_tx_chars(mport);
        ret = IRQ_HANDLED;
    }

    /* modem mode */
    _mt53xx_uart_modem_state(mport);

    return ret;
}
#endif

static irqreturn_t _mt53xx_uart2_interrupt(int irq, void* dev_id)
{
    unsigned int uart_int_ident;
    struct mt53xx_uart_port* mport = (struct mt53xx_uart_port*)dev_id;
    irqreturn_t ret = IRQ_NONE;

    uart_int_ident = UART_READ32(UART2_INT_ID);
    uart_int_ident &= U2_INTMSK;

    /* take care of SA_SHIRQ and return IRQ_NONE if possible [LDD/279] */

    /* ack u2 interrupt */
    UART_WRITE32(~uart_int_ident, UART2_INT_ID);
    if (mport != &_mt53xx_uart_ports[2])
    {
        while (1)
            ;
    }

    /* rx mode */
    if (mport->fn_read_allow() || (uart_int_ident & (U2_RBUF | U2_RERR)))
    {
        _mt53xx_uart_rx_chars(mport);
        ret = IRQ_HANDLED;
    }

    /* tx mode */
    if (mport->fn_write_allow() || (uart_int_ident & U2_TBUF))
    {
        _mt53xx_uart_tx_chars(mport);
        ret = IRQ_HANDLED;
    }

    /* modem mode */
    _mt53xx_uart_modem_state(mport);

    return ret;
}

#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5891)
static irqreturn_t _mt53xx_uart4_interrupt(int irq, void* dev_id)
{
    unsigned int uart_int_ident;
    struct mt53xx_uart_port* mport = (struct mt53xx_uart_port*)dev_id;
    irqreturn_t ret = IRQ_NONE;

    uart_int_ident = UART_READ32(UART4_INT_ID);
    uart_int_ident &= U4_INTMSK;

    /* ack u4 interrupt */
    UART_WRITE32(~uart_int_ident, UART4_INT_ID);
    if (mport != &_mt53xx_uart_ports[4])
    {
        while (1)
            ;
    }

    /* rx mode */
    if (mport->fn_read_allow() || (uart_int_ident & (U4_RBUF | U4_RERR)))
    {
        _mt53xx_uart_rx_chars(mport);
        ret = IRQ_HANDLED;
    }

    /* tx mode */
    if (mport->fn_write_allow() || (uart_int_ident & U4_TBUF))
    {
        _mt53xx_uart_tx_chars(mport);
        ret = IRQ_HANDLED;
    }

    /* modem mode */
    _mt53xx_uart_modem_state(mport);

    return ret;
}

static irqreturn_t _mt53xx_uart5_interrupt(int irq, void* dev_id)
{
    unsigned int uart_int_ident;
    struct mt53xx_uart_port* mport = (struct mt53xx_uart_port*)dev_id;
    irqreturn_t ret = IRQ_NONE;

    uart_int_ident = UART_READ32(UART5_INT_ID);
    uart_int_ident &= U5_INTMSK;

    /* ack u5 interrupt */
    UART_WRITE32(~uart_int_ident, UART5_INT_ID);
    if (mport != &_mt53xx_uart_ports[5])
    {
        while (1)
            ;
    }

    /* rx mode */
    if (mport->fn_read_allow() || (uart_int_ident & (U5_RBUF | U5_RERR)))
    {
        _mt53xx_uart_rx_chars(mport);
        ret = IRQ_HANDLED;
    }

    /* tx mode */
    if (mport->fn_write_allow() || (uart_int_ident & U5_TBUF))
    {
        _mt53xx_uart_tx_chars(mport);
        ret = IRQ_HANDLED;
    }

    /* modem mode */
    _mt53xx_uart_modem_state(mport);

    return ret;
}
#endif

/*
 * console
 */

#ifdef CONFIG_SERIAL_MT53XX_CONSOLE

static void _mt53xx_uart_console_write(struct console* co, const char* s,
                                       unsigned int count)
{
    int i;
    struct mt53xx_uart_port* mport;

	#if 1//def CC_LOG_PERFORMANCE_SUPPORT
	BUFFER_INFO *prRxFIFO;
	unsigned int u4RomeLeft;
	char itemps;
	#endif

#ifdef CONFIG_TTYMT7_SUPPORT
    if (_enable_ttyMT7)
    {
	#ifdef CC_LOG2USB_SUPPORT
		for (i = 0; i < count; i++)
		{
		    _log2usb_write_byte(s[i]);  
		}
		#endif
        return;
    }
#endif
    if (((co->index == 0) && (!_mt53xx_u0_trans_mode_on())) ||
        ((co->index == 2) && (!_mt53xx_u2_trans_mode_on()))
#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5891)
        || ((co->index == 4) && !(_mt53xx_u4_trans_mode_on()))
        || ((co->index == 5) && !(_mt53xx_u5_trans_mode_on()))
#endif
       )
    {
        return;
    }

    if (co->index >= UART_NR)
    {
        return;
    }
#ifdef CC_NO_KRL_UART_DRV
    if (co->index == 1)
    {
        return;
    }
#endif

    mport = &_mt53xx_uart_ports[co->index];
#ifdef ENABLE_UDMA
    if (!mport->hw_init)
#else
    if (!_hw_init)
#endif
    {
        unsigned long flags;
#ifdef ENABLE_UDMA
        mport->hw_init = 1;
#else
        _hw_init = 1;
#endif
        local_irq_save(flags);
#ifdef ENABLE_UDMA
        _mt53xx_uart_hwinit(mport, co->index);
#else
        _mt53xx_uart_hwinit(co->index);
#endif
        if (co->index == 0)
        {
            /* enable uart 0 interrupt */
            UART_WRITE32(U0_INTALL | U0_TBUF, UART_INT_EN);
        }
        if (co->index == 1)
        {
            /* enable uart 1 interrupt */
            UART_WRITE32(U1_INTALL | U1_TBUF, UART_INT_EN);
        }
        if (co->index == 2)
        {
            /* enable uart2 interrupt */
            UART_WRITE32(U2_INTALL | U2_TBUF, UART2_INT_EN);
        }
#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5891)
        if (co->index == 4)
        {
            /* enable uart2 interrupt */
            UART_WRITE32(U4_INTALL | U4_TBUF, UART4_INT_EN);
        }
        if (co->index == 5)
        {
            /* enable uart2 interrupt */
            UART_WRITE32(U5_INTALL | U5_TBUF, UART5_INT_EN);
        }
#endif
        local_irq_restore(flags);
    }

	#if 1//def CC_LOG_PERFORMANCE_SUPPORT
		if(!_hUart0Thread)
	#endif
		{
			for (i = 0; i < count; i++)
			{
				while (!mport->fn_write_allow())
				{
					barrier();
				}
				mport->fn_write_byte(s[i]);
	
				if (s[i] == '\n')
				{
					while (!mport->fn_write_allow())
					{
						barrier();
					}
					mport->fn_write_byte('\r');
				}
			}
		}
	#if 1//def CC_LOG_PERFORMANCE_SUPPORT
		else
		{
			prRxFIFO = &_arTxFIFO;
			
			for (i = 0; i < count; i++)
			{
				
				Buf_GetRoomLeft(prRxFIFO, u4RomeLeft);	
				if(u4RomeLeft > 15)
				{
					Buf_Push(prRxFIFO, s[i]);
	
					if (s[i] == '\n')
					{
						itemps = '\r';
						Buf_GetRoomLeft(prRxFIFO, u4RomeLeft);
						if(u4RomeLeft)
						{
							Buf_Push(prRxFIFO, itemps);
						}
					}
				}
				else if(u4RomeLeft >=13 && u4RomeLeft <=15)
				{
					Buf_Push(prRxFIFO, 'L');
					Buf_Push(prRxFIFO, 'O');
					Buf_Push(prRxFIFO, 'G');
					Buf_Push(prRxFIFO, ' ');
					Buf_Push(prRxFIFO, 'd');
					Buf_Push(prRxFIFO, 'r');
					Buf_Push(prRxFIFO, 'o');
					Buf_Push(prRxFIFO, 'p');
					Buf_Push(prRxFIFO, 'p');
					Buf_Push(prRxFIFO, 'e');
					Buf_Push(prRxFIFO, 'd');
					Buf_Push(prRxFIFO, '\n');
					Buf_Push(prRxFIFO, '\r');				
				}
				else
				{
					//buffer overflow,drop log!
				}
			}
			up(&uart0_log_sema);
		}
	#endif
}
    
static void __init _mt53xx_uart_console_get_options(struct uart_port* port,
                                                    int* pbaud, int* pparity, int* pbits)
{
    int baud = 0, parity = 0, stopbit, datalen;
    struct mt53xx_uart_port* mport;
    unsigned int uCOMMCTRL;

    if (!port)
    {
        return;
    }

    mport = (struct mt53xx_uart_port*)port;

    switch (mport->nport)
    {
        case 0:
#ifdef CONFIG_TTYMT7_SUPPORT
            //    case 3: // virtual UART
#endif
            uCOMMCTRL = UART0_COMMCTRL;
            break;
        case 1:
            uCOMMCTRL = UART1_COMMCTRL;
            break;
        case 2:
            uCOMMCTRL = UART2_COMMCTRL;
            break;
#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5891)
        case 4:
            uCOMMCTRL = UART4_COMMCTRL;
            break;
        case 5:
            uCOMMCTRL = UART5_COMMCTRL;
            break;
#endif
        default:
            if (pbaud)
            {
                *pbaud = 115200;
            }
            if (pparity)
            {
                *pparity = 'n';
            }
            if (pbits)
            {
                *pbits = 8;
            }
            return;
    }

    _mt53xx_uart_get(uCOMMCTRL, &baud, &datalen, &stopbit, &parity);

    if (pbaud)
    {
        *pbaud = baud;
    }

    if (pparity)
    {
        switch (parity)
        {
            case 1:
                *pparity = 'o';
                break;
            case 2:
                *pparity = 'e';
                break;
            case 0:
            default:
                *pparity = 'n';
                break;
        }
    }

    if (pbits)
    {
        *pbits = datalen;
    }
}

static int __init _mt53xx_uart_console_setup(struct console* co, char* options)
{
    struct uart_port* port;
    int baud    = 115200;
    int bits    = 8;
    int parity  = 'n';
    int flow    = 'n';
    int stopbit;
    int ret;

    printk(KERN_DEBUG "53xx console setup : co->index %d options:%s\n",
           co->index, options);

    if (co->index >= UART_NR)
    {
        co->index = 0;
    }
    #ifdef CC_USE_TTYMT1
    console_index = co->index;
    #endif
    port = (struct uart_port*)&_mt53xx_uart_ports[co->index];

    if (options)
    {
        uart_parse_options(options, &baud, &parity, &bits, &flow);
    }
    else
    {
        _mt53xx_uart_console_get_options(port, &baud, &parity, &stopbit);
    }

    ret = uart_set_options(port, co, baud, parity, bits, flow);
    printk(KERN_DEBUG "53xx console setup : uart_set_option port(%d) "
           "baud(%d) parity(%c) bits(%d) flow(%c) - ret(%d)\n",
           co->index, baud, parity, bits, flow, ret);
    return ret;
}


static struct uart_driver _mt53xx_uart_reg;
static struct console _mt53xx_uart_console =
{
    .name       = "ttyMT",
    .write      = _mt53xx_uart_console_write,
    .device     = uart_console_device,
    .setup      = _mt53xx_uart_console_setup,
    .flags      = CON_PRINTBUFFER,
    .index      = -1,
    .data       = &_mt53xx_uart_reg,
};

static int __init _mt53xx_uart_console_init(void)
{
#ifdef CONFIG_OF
	struct device_node *np1;
	np1 = of_find_compatible_node(NULL, NULL, "Mediatek, RS232");
	if (!np1) {
		panic("%s: RS232 node not found\n", __func__);
	}
	pUartIoMap = of_iomap(np1, 0); //uart remap
	pPdwncIoMap = of_iomap(np1, 1); //pdwnc remap
	pCkgenIoMap = of_iomap(np1, 2); //ckgen remap
	pBimIoMap = of_iomap(np1, 3); //bim remap
#else
	pUartIoMap = ioremap(RS232_PHY,0x1000);//uart remap
    pPdwncIoMap = ioremap(PDWNC_PHY,0x1000);//pdwnc remap
    pCkgenIoMap = ioremap(CKGEN_PHY,0x1000); //ckgen remap
	pBimIoMap = ioremap(BIM_PHY,0x1000); //bim remap
#endif
	if ((!pUartIoMap) || (!pPdwncIoMap) ||(!pCkgenIoMap) || (!pBimIoMap))
		panic("Impossible to ioremap pdwnc!!\n");
	printk(KERN_DEBUG "Serial:uart_reg_base: 0x%p\n", pUartIoMap);
	printk(KERN_DEBUG "Serial:pdwnc_reg_base: 0x%p\n", pPdwncIoMap);
	printk(KERN_DEBUG "Serial:ckgen_reg_base: 0x%p\n", pCkgenIoMap);
	printk(KERN_DEBUG "Serial:bim_reg_base: 0x%p\n", pBimIoMap);

	
#ifdef CONFIG_OF
		/* reset port membase/mapbase */
		_mt53xx_uart_ports[0].port.membase = MT53xx_VA_UART_IOMAP;
		_mt53xx_uart_ports[0].port.mapbase = MT53xx_PA_UART_IOMAP;
	
		_mt53xx_uart_ports[1].port.membase = MT53xx_VA_UART_IOMAP;
		_mt53xx_uart_ports[1].port.mapbase = MT53xx_PA_UART_IOMAP;
	
		_mt53xx_uart_ports[2].port.membase = MT53xx_VA_UART_IOMAP2;
		_mt53xx_uart_ports[2].port.mapbase = MT53xx_PA_UART_IOMAP2;
	
	#ifdef CONFIG_TTYMT7_SUPPORT
		_mt53xx_uart_ports[3].port.membase = MT53xx_VA_UART_IOMAP;
		_mt53xx_uart_ports[3].port.mapbase = MT53xx_PA_UART_IOMAP;
	#endif
	
	#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5891)
		_mt53xx_uart_ports[4].port.membase = MT53xx_VA_UART_IOMAP4;
		_mt53xx_uart_ports[4].port.mapbase = MT53xx_PA_UART_IOMAP4;
		_mt53xx_uart_ports[5].port.membase = MT53xx_VA_UART_IOMAP5;
		_mt53xx_uart_ports[5].port.mapbase = MT53xx_PA_UART_IOMAP5;
	#endif
	//printk(KERN_DEBUG "###################mapbase: 0x%x\n", _mt53xx_uart_ports[0].port.mapbase);
#endif
	
    register_console(&_mt53xx_uart_console);
    return 0;
}

console_initcall(_mt53xx_uart_console_init);

static int __init _mt53xx_late_console_init(void)
{
    if (!(_mt53xx_uart_console.flags & CON_ENABLED))
    {
        register_console(&_mt53xx_uart_console);
    }
    return 0;
}

late_initcall(_mt53xx_late_console_init);

#define MT53XX_CONSOLE &_mt53xx_uart_console
#else

#define MT53XX_CONSOLE NULL

#endif /* CONFIG_SERIAL_MT53XX_CONSOLE */


static struct uart_driver _mt53xx_uart_reg =
{
    .owner          = THIS_MODULE,
    .driver_name    = "MT53xx serial",
    .dev_name       = "ttyMT",
    .major          = SERIAL_MT53xx_MAJOR,
    .minor          = SERIAL_MT53xx_MINOR,
    .nr             = UART_NR,
    .cons           = MT53XX_CONSOLE,
};

/*
* probe? remove? suspend? resume? ids?
*/
static int _mt53xx_uart_probe(struct platform_device* dev)
{
#ifdef ENABLE_UDMA
    _mt53xx_uart_hwinit(&_mt53xx_uart_ports[dev->id], dev->id);
#else
    _mt53xx_uart_hwinit(dev->id);
#endif
    printk(KERN_DEBUG "Serial : add uart port %d\n", dev->id);
    uart_add_one_port(&_mt53xx_uart_reg, &_mt53xx_uart_ports[dev->id].port);

    platform_set_drvdata(dev, &_mt53xx_uart_ports[dev->id]);
    return 0;
}
static int _mt53xx_uart_remove(struct platform_device* pdev)
{
    struct mt53xx_uart_port* mport = platform_get_drvdata(pdev);

    platform_set_drvdata(pdev, NULL);

    if (mport)
    {
        uart_remove_one_port(&_mt53xx_uart_reg, &mport->port);
    }

    return 0;
}

static int _mt53xx_uart_suspend(struct platform_device* _dev, pm_message_t state)
{
    _mt53xx_uart_get(UART0_COMMCTRL, &_gUartSettings[0].i4BaudRate, &_gUartSettings[0].i4DataLen,
                     &_gUartSettings[0].i4StopBit, &_gUartSettings[0].i4Parity);

    _mt53xx_uart_get(UART1_COMMCTRL, &_gUartSettings[1].i4BaudRate, &_gUartSettings[1].i4DataLen,
                     &_gUartSettings[1].i4StopBit, &_gUartSettings[1].i4Parity);

    _mt53xx_uart_get(UART2_COMMCTRL, &_gUartSettings[2].i4BaudRate, &_gUartSettings[2].i4DataLen,
                     &_gUartSettings[2].i4StopBit, & _gUartSettings[2].i4Parity);
#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5891) 
    _mt53xx_uart_get(UART4_COMMCTRL, &_gUartSettings[3].i4BaudRate, &_gUartSettings[3].i4DataLen,
                     &_gUartSettings[3].i4StopBit, & _gUartSettings[3].i4Parity);
    _mt53xx_uart_get(UART5_COMMCTRL, &_gUartSettings[4].i4BaudRate, &_gUartSettings[4].i4DataLen,
                     &_gUartSettings[4].i4StopBit, & _gUartSettings[4].i4Parity);
#endif
    return 0;
}

static int _mt53xx_uart_resume(struct platform_device* dev)
{
    int RS232_EN;

    //add hw init, same with _mt53xx_uart_init
#ifdef ENABLE_UDMA
    _mt53xx_uart_hwinit(&_mt53xx_uart_ports[dev->id], dev->id);
#else
    _mt53xx_uart_hwinit(dev->id);
#endif

#ifdef CC_NO_KRL_UART_DRV
    if (dev->id == 1)
    {
        return 0;
    }
#endif
    ((struct mt53xx_uart_port*)dev->dev.platform_data)->fn_empty_int_enable(1);
    ((struct mt53xx_uart_port*)dev->dev.platform_data)->fn_int_enable(1);
    if (dev->id == 0)
    {
        printk(KERN_INFO "Uart0_int_en=%d\n", _u0_irq_allocated);
        _mt53xx_u0_int_enable(_u0_irq_allocated);
    }
    else if (dev->id == 1)
    {
        _mt53xx_u1_int_enable(_u1_irq_allocated);
    }
    else if (dev->id == 2)
    {
        _mt53xx_u2_int_enable(_u2_irq_allocated);
    }
#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5891)
    else if (dev->id == 4)
    {
        _mt53xx_u4_int_enable(_u4_irq_allocated);
    }
    else if (dev->id == 5)
    {
        _mt53xx_u5_int_enable(_u5_irq_allocated);
    }
#endif

    if (_u0_irq_allocated || _u1_irq_allocated || _u2_irq_allocated
#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5891)
        || _u4_irq_allocated || _u5_irq_allocated
#endif
       )
    {
        RS232_EN = VECTOR_RS232_0;
        while (RS232_EN >= 32)
        {
            RS232_EN -= 32;
        }
        __raw_writel(__raw_readl(pBimIoMap + REG_RW_IRQEN) | (1 << RS232_EN),
                     pBimIoMap + REG_RW_IRQEN);
    }

#ifdef ENABLE_UDMA
    _mt5395_u1_HwDmaWriteDram(snapshot_u4SPTR, snapshot_u4Size, snapshot_u4Threshold);
#endif

    _mt53xx_uart_set(UART0_COMMCTRL, _gUartSettings[0].i4BaudRate, _gUartSettings[0].i4DataLen,
                     _gUartSettings[0].i4StopBit, _gUartSettings[0].i4Parity);

    _mt53xx_uart_set(UART1_COMMCTRL, _gUartSettings[1].i4BaudRate, _gUartSettings[1].i4DataLen,
                     _gUartSettings[1].i4StopBit, _gUartSettings[1].i4Parity);

    _mt53xx_uart_set(UART2_COMMCTRL, _gUartSettings[2].i4BaudRate, _gUartSettings[2].i4DataLen,
                     _gUartSettings[2].i4StopBit, _gUartSettings[2].i4Parity);
#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5891)
    _mt53xx_uart_set(UART4_COMMCTRL, _gUartSettings[3].i4BaudRate, _gUartSettings[3].i4DataLen,
                     _gUartSettings[3].i4StopBit, _gUartSettings[3].i4Parity);
    _mt53xx_uart_set(UART5_COMMCTRL, _gUartSettings[4].i4BaudRate, _gUartSettings[4].i4DataLen,
                     _gUartSettings[4].i4StopBit, _gUartSettings[4].i4Parity);
#endif
    return 0;
}

static struct platform_driver _mt53xx_uart_driver =
{
    .probe          = _mt53xx_uart_probe,
    .remove         = _mt53xx_uart_remove,
    .suspend        = _mt53xx_uart_suspend,
    .resume         = _mt53xx_uart_resume,
    .driver         =
    {
        .name   = "MT53xx-uart",
        .owner  = THIS_MODULE,
    },
};

static struct platform_device _mt53xx_uart0_device =
{
    .name                   = "MT53xx-uart",
    .id                     = 0,
    .dev                    =
    {
        .platform_data  = &_mt53xx_uart_ports[0],
    }
};// UART_DBG

#ifndef CC_NO_KRL_UART_DRV
static struct platform_device _mt53xx_uart1_device =
{
    .name                   = "MT53xx-uart",
    .id                     = 1,
    .dev                    =
    {
        .platform_data  = &_mt53xx_uart_ports[1],
    }
};// UART_DMA
#endif

static struct platform_device _mt53xx_uart2_device =
{
    .name                   = "MT53xx-uart",
    .id                     = 2,
    .dev                    =
    {
        .platform_data  = &_mt53xx_uart_ports[2],
    }
};// UART_PD

static struct platform_device _mt53xx_uart3_device =
{
    .name                   = "MT53xx-uart",
    .id                     = 3,
    .dev                    =
    {
        .platform_data  = &_mt53xx_uart_ports[3],
    }
};// UART_DBG with factory mode, I map it to uart 5

#ifdef CC_LOG2USB_SUPPORT

#define LOG2USB_MAJOR               255
#define LOG2USB_MINOR               0
#define LOG2USB_CDEV_NAME           "log2usb"
#define LOG2USB_BUFFER_SIZE         0x40000
#define LOG2USB_BUFFER_MIN_SIZE     0x4000

static int _log2usb_init = 0;
static int _log2usb_cdev_open = 0;

struct LOG2USB_BUFFER_T {
    unsigned int bufferSize;
    char *pBuffer;
    char *pEnd;
    char *pRead;
    char *pWrite;
};

static struct LOG2USB_BUFFER_T _pLogDev;

static void _log2usb_write_byte(unsigned int byte)
{
    if (_log2usb_init)//his_lihui modify
    {
        if(_pLogDev.pWrite)
        {
            *(_pLogDev.pWrite) = byte;

            if (_pLogDev.pWrite+1 >= _pLogDev.pEnd)
                _pLogDev.pWrite = _pLogDev.pBuffer;
            else
                ++_pLogDev.pWrite;
        }
    }
}

static int _log2usb_open(struct inode *inode, struct file *filp)
{
    if (_log2usb_init)
    {
        //printk(KERN_CRIT "_log2usb_open: 0x%x\n", _pLogDev.bufferSize);

        if(_pLogDev.pBuffer)
        {
            memset(_pLogDev.pBuffer, 0, _pLogDev.bufferSize);
            _pLogDev.pRead = _pLogDev.pBuffer;
            _pLogDev.pWrite = _pLogDev.pBuffer;
        }

        _log2usb_cdev_open = 1;
        return 0;          /* success */
    }
    else
    {
        printk(KERN_CRIT "_log2usb_open: not init error\n");
        return -EFAULT;
    }
}

static int _log2usb_release(struct inode *inode, struct file *filp)
{
    //printk(KERN_CRIT "_log2usb_release\n");
    _log2usb_cdev_open = 0;
    return 0;
}

static ssize_t _log2usb_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = 0;
    size_t dataSize = 0;

    if(_log2usb_cdev_open == 0)
    {
        printk(KERN_CRIT "_log2usb_read: not open error\n");
        return -EFAULT;
    }

    if (_pLogDev.pWrite == _pLogDev.pRead)
    {
        retval = 0;
    }
    else if (_pLogDev.pWrite > _pLogDev.pRead)
    {
        dataSize = _pLogDev.pWrite - _pLogDev.pRead;
        if (dataSize > count)
        {
            dataSize = count;
        }
        if (0 == copy_to_user(buf, _pLogDev.pRead, dataSize)) {
            _pLogDev.pRead += dataSize;
            retval = dataSize;
            *f_pos += dataSize;
        }
        else{
            retval = -EFAULT;
        }
    }
    else
    {
        dataSize = _pLogDev.pEnd - _pLogDev.pRead;
        if (dataSize > count)
        {
            dataSize = count;
        }
        if (0 == copy_to_user(buf, _pLogDev.pRead, dataSize)) {
            _pLogDev.pRead = _pLogDev.pBuffer;
            retval = dataSize;
            *f_pos += dataSize;
        }
        else{
            retval = -EFAULT;
        }
    }
    return retval;
}

static ssize_t _log2usb_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
    printk(KERN_CRIT "_log2usb_write\n");
    return 0;
}

static struct file_operations _log2usb_fops = {
    .owner =    THIS_MODULE,
    .open =     _log2usb_open,
    .release =  _log2usb_release,
    .read =     _log2usb_read,
    .write =    _log2usb_write
};

static struct cdev _log2usb_cdev;

static int _log2usb_init_module(void)
{
    int result;
    int memory_size;

    printk(KERN_CRIT "_log2usb_3init_module\n");

    memset(&_pLogDev, 0, sizeof(_pLogDev));

    memory_size = LOG2USB_BUFFER_SIZE;
    while(1)
    {
        _pLogDev.pBuffer = kmalloc(memory_size, GFP_KERNEL);

        if (_pLogDev.pBuffer)
        {
            break;
        }
        else
        {
            memory_size /= 2;
            if (memory_size < LOG2USB_BUFFER_MIN_SIZE)
            {
                printk(KERN_CRIT "_log2usb_init_module: can't allocate buffer %d\n", LOG2USB_BUFFER_MIN_SIZE);
                return -ENOMEM;
            }
        }
    }

    result = register_chrdev_region(MKDEV(LOG2USB_MAJOR, LOG2USB_MINOR), 1, LOG2USB_CDEV_NAME);
    if (result < 0)
    {
        printk(KERN_CRIT "_log2usb_init_module: can't register cdev %d %d\n", LOG2USB_MAJOR, LOG2USB_MINOR);
        return result;
    }

    cdev_init(&_log2usb_cdev, &_log2usb_fops);
    _log2usb_cdev.owner = THIS_MODULE;
    _log2usb_cdev.ops = &_log2usb_fops;
    result = cdev_add (&_log2usb_cdev, MKDEV(LOG2USB_MAJOR, LOG2USB_MINOR), 1);
    if (result < 0)
    {
        printk(KERN_CRIT "_log2usb_init_module: can't add cdev %d %d\n", LOG2USB_MAJOR, LOG2USB_MINOR);
        return result;
    }

    memset(_pLogDev.pBuffer, 0, memory_size);
    _pLogDev.bufferSize = memory_size;
    _pLogDev.pRead = _pLogDev.pBuffer;
    _pLogDev.pWrite = _pLogDev.pBuffer;
    _pLogDev.pEnd = _pLogDev.pBuffer + memory_size;

    _log2usb_init = 1;
    printk(KERN_CRIT "_log2usb_3init_module init ok\n");
    return 0; /* succeed */
}

static void _log2usb_cleanup_module(void)
{
    printk(KERN_INFO "_log2usb_cleanup_module");

    _log2usb_init = 0;

    unregister_chrdev_region(MKDEV(LOG2USB_MAJOR, LOG2USB_MINOR), 1);

    if(_pLogDev.pBuffer)
    {
        kfree(_pLogDev.pBuffer);
    }

    memset(&_pLogDev, 0, sizeof(_pLogDev));
}
#endif

#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5891)
static struct platform_device _mt53xx_uart4_device =
{
    .name                   = "MT53xx-uart",
    .id                     = 4,
    .dev                    =
    {
        .platform_data  = &_mt53xx_uart_ports[4],
    }
};// UART_HT

static struct platform_device _mt53xx_uart5_device =
{
    .name                   = "MT53xx-uart",
    .id                     = 5,
    .dev                    =
    {
        .platform_data  = &_mt53xx_uart_ports[5],
    }
};// UART_BT
#endif

#ifdef ENABLE_UDMA
#ifdef ENABLE_RX_TIMER
unsigned int gWPTR;
unsigned int gRPTR;

static void _mt53xx_uart_rx_timer(unsigned long data)
{
    static unsigned int sameCount = 0;
    int i = 0;
    int length;
    struct mt53xx_uart_port* mport1 = &_mt53xx_uart_ports[1];
    unsigned int u4WPTR;
    unsigned int u4RPTR;
    unsigned int u4EPTR;
    unsigned int u4SPTR;
    struct tty_port* tport;
    tport = &mport1->port.state->port;

    u4WPTR = UART_READ32(UART1_DMAW_WPTR);
    u4RPTR = UART_READ32(UART1_DMAW_RPTR);
    u4SPTR = UART_READ32(UART1_DMAW_SPTR);
    u4EPTR = UART_READ32(UART1_DMAW_EPTR);

    if ((u4WPTR != u4RPTR) && (u4WPTR == gWPTR) && (u4RPTR == gRPTR))
    {
        if (sameCount++ > 1)
        {
            length = u4WPTR - u4RPTR;
            for (i = 0; i < length; i++)
            {
                uart_insert_char(&mport1->port, 0, 0, *(u4RPTR - u4SPTR + mport1->pu1DmaBuffer + i), TTY_NORMAL);
                mport1->port.icount.rx++;
            }
            UART_WRITE32((u4RPTR + length), UART1_DMAW_RPTR);
            tty_flip_buffer_push(tport);
            sameCount = 0;
        }
    }
    else
    {
        gWPTR = u4WPTR;
        gRPTR = u4RPTR;
        sameCount = 0;
    }

    mod_timer(&uart_rx_timer, jiffies + TIMER_INTERVAL);
}
#endif
#endif

#if 1//def CC_LOG_PERFORMANCE_SUPPORT
static int _Uart0LogThread(void* pvArg)
{	
	BUFFER_INFO *prRxFIFO;

    unsigned int u4DataAvail;
    unsigned int i;
    unsigned char itemps;
    #ifdef CC_USE_TTYMT1
    struct mt53xx_uart_port* mport;
    //struct _mt53xx_uart_ports* mport;
    #endif
    while(!kthread_should_stop())
    {   
     #ifdef CC_USE_TTYMT1
        mport = (struct mt53xx_uart_port*)&_mt53xx_uart_ports[console_index];
        if(!mport)
        {
          return -1;
        }
     #endif
        down(&uart0_log_sema);
        
        prRxFIFO = &_arTxFIFO;

        Buf_GetBytesAvail(prRxFIFO, u4DataAvail);
        if(u4DataAvail > 0)
        {
            for (i = 0; i < u4DataAvail; i++)
            {
                Buf_Pop(prRxFIFO, itemps);
                #ifdef CC_USE_TTYMT1
                while (!mport->fn_write_allow())
                {
                    usleep_range(50,100);//schedule();
                }
                mport->fn_write_byte(itemps);
                #else

                while (_mt53xx_u0_write_allow() <= 1)
                {
                    usleep_range(50,100);//schedule();
                }
                _mt53xx_u0_write_byte(itemps);

                #endif
            }
        }

		//msleep(1);
	}	
	return 0;
}


static int uart0_log_thread_create(void)
{	
	_au0TxBuf = (unsigned char *)vmalloc(UART0_RING_BUFFER_SIZE);
	Buf_init(&_arTxFIFO, _au0TxBuf, UART0_RING_BUFFER_SIZE);
	
	_hUart0Thread = kthread_run(_Uart0LogThread, NULL, "Uart0LogThread");
	if (IS_ERR(_hUart0Thread)) 
	{
		printk("Fail to create Uart0LogThread thread!\n");
		_hUart0Thread = NULL;
		if(_au0TxBuf)
		{
			vfree(_au0TxBuf);
			_au0TxBuf = NULL;
		}
		return -1;
	}
	
	return 0;
}
#endif


/*
* init, exit and module
*/

static int __init _mt53xx_uart_init(void)
{
    int ret;
    int i;

#ifdef ENABLE_UDMA
#ifdef PRETECT_RESOURCE
    pid_t pid_uart_prc;
    cpumask_t cpu0;
    pid_uart_prc = current->pid;
    cpu_set(0, cpu0);
    if (0 != sched_setaffinity(pid_uart_prc, &cpu0))
    {
        printk(" sched_setaffinity error \n");
    }
    else
    {
        printk(" sched_setaffinity succ.  \n");
    }
#endif
    //yjdbg_rxcnt=0;
    //yjdbg_txcnt=0;
    yjdbg_dram_full = 0;
    yjdbg_fifo_full = 0;
    yjdbg_dmaw_trig = 0;
    yjdbg_dmaw_tout = 0;
    all_interrupt = 0;
    rx_loss_data = 32;
#if UART_DEBUG
    sameByte = 0;
    txCount = 0;
    rxCount = 0;
#endif
#endif

#ifdef CONFIG_OF
	struct device_node *np1;
	np1 = of_find_compatible_node(NULL, NULL, "Mediatek, RS232");
	if (!np1) {
		panic("%s: RS232 node not found\n", __func__);
	}
	pUartIoMap = of_iomap(np1, 0); //uart remap
	pPdwncIoMap = of_iomap(np1, 1); //pdwnc remap
	pCkgenIoMap = of_iomap(np1, 2); //ckgen remap
	pBimIoMap = of_iomap(np1, 3); //bim remap
#else
	pUartIoMap = ioremap(RS232_PHY,0x1000);//uart remap
    pPdwncIoMap = ioremap(PDWNC_PHY,0x1000);//pdwnc remap
    pCkgenIoMap = ioremap(CKGEN_PHY,0x1000); //ckgen remap
	pBimIoMap = ioremap(BIM_PHY,0x1000); //bim remap
#endif
	if ((!pUartIoMap) || (!pPdwncIoMap) ||(!pCkgenIoMap) || (!pBimIoMap))
		panic("Impossible to ioremap pdwnc!!\n");
	printk(KERN_DEBUG "Serial:uart_reg_base: 0x%p\n", pUartIoMap);
	printk(KERN_DEBUG "Serial:pdwnc_reg_base: 0x%p\n", pPdwncIoMap);
	printk(KERN_DEBUG "Serial:ckgen_reg_base: 0x%p\n", pCkgenIoMap);
	printk(KERN_DEBUG "Serial:bim_reg_base: 0x%p\n", pBimIoMap);

	#ifdef CONFIG_OF
	/* reset port membase/mapbase */
	_mt53xx_uart_ports[0].port.membase = MT53xx_VA_UART_IOMAP;
	_mt53xx_uart_ports[0].port.mapbase = MT53xx_PA_UART_IOMAP;

	_mt53xx_uart_ports[1].port.membase = MT53xx_VA_UART_IOMAP;
	_mt53xx_uart_ports[1].port.mapbase = MT53xx_PA_UART_IOMAP;

	_mt53xx_uart_ports[2].port.membase = MT53xx_VA_UART_IOMAP2;
	_mt53xx_uart_ports[2].port.mapbase = MT53xx_PA_UART_IOMAP2;

	#ifdef CONFIG_TTYMT7_SUPPORT
	_mt53xx_uart_ports[3].port.membase = MT53xx_VA_UART_IOMAP;
	_mt53xx_uart_ports[3].port.mapbase = MT53xx_PA_UART_IOMAP;
	#endif

	#if defined(CONFIG_ARCH_MT5890)
	_mt53xx_uart_ports[4].port.membase = MT53xx_VA_UART_IOMAP4;
	_mt53xx_uart_ports[4].port.mapbase = MT53xx_PA_UART_IOMAP4;
	_mt53xx_uart_ports[5].port.membase = MT53xx_VA_UART_IOMAP5;
	_mt53xx_uart_ports[5].port.mapbase = MT53xx_PA_UART_IOMAP5;
	//printk(KERN_DEBUG "###################mapbase: 0x%x\n", _mt53xx_uart_ports[0].port.mapbase);
	#endif
	#endif

#ifdef CONFIG_OF
	/* reset port membase/mapbase */
	_mt53xx_uart_ports[0].port.membase = MT53xx_VA_UART_IOMAP;
	_mt53xx_uart_ports[0].port.mapbase = MT53xx_PA_UART_IOMAP;

	_mt53xx_uart_ports[1].port.membase = MT53xx_VA_UART_IOMAP;
	_mt53xx_uart_ports[1].port.mapbase = MT53xx_PA_UART_IOMAP;

	_mt53xx_uart_ports[2].port.membase = MT53xx_VA_UART_IOMAP2;
	_mt53xx_uart_ports[2].port.mapbase = MT53xx_PA_UART_IOMAP2;

	#ifdef CONFIG_TTYMT7_SUPPORT
	_mt53xx_uart_ports[3].port.membase = MT53xx_VA_UART_IOMAP;
	_mt53xx_uart_ports[3].port.mapbase = MT53xx_PA_UART_IOMAP;
	#endif

	#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5891)
	_mt53xx_uart_ports[4].port.membase = MT53xx_VA_UART_IOMAP4;
	_mt53xx_uart_ports[4].port.mapbase = MT53xx_PA_UART_IOMAP4;
	_mt53xx_uart_ports[5].port.membase = MT53xx_VA_UART_IOMAP5;
	_mt53xx_uart_ports[5].port.mapbase = MT53xx_PA_UART_IOMAP5;
	//printk(KERN_DEBUG "###################mapbase: 0x%x\n", _mt53xx_uart_ports[0].port.mapbase);
	#endif
#endif


#ifdef CONFIG_TTYMT7_SUPPORT
    sema_init(&uart_sema, 1);
#endif

#ifdef CC_LOG2USB_SUPPORT
    _log2usb_init_module();
#endif

    /* reset hardware */
    for (i = 0; i < UART_NR; i++)
    {
#ifdef ENABLE_UDMA
        _mt53xx_uart_ports[i].pu1DmaBuffer = NULL;
        _mt53xx_uart_ports[i].pu1DrvBuffer = NULL;

        _mt53xx_uart_ports[i].hw_init = 0;
        _mt53xx_uart_hwinit(&_mt53xx_uart_ports[i], i);  /* all port 115200, no int */
#else
        _mt53xx_uart_hwinit(i);  /* all port 115200, no int */
#endif
    }

	if(_perfor_uart0) //on
	{
		
      #if 1//def CC_LOG_PERFORMANCE_SUPPORT
		sema_init(&uart0_log_sema, 1);
      #endif
      #if 1//def CC_LOG_PERFORMANCE_SUPPORT
		uart0_log_thread_create();
      #endif
	}

    printk(KERN_INFO "Serial: MT53xx driver $Revision: #3 $\n");
    ret = uart_register_driver(&_mt53xx_uart_reg);
    printk(KERN_DEBUG "Serial: uart_register_driver %d _perfor_uart0:%d\n", ret,_perfor_uart0);
    if (ret)
    {
        goto out;
    }

    platform_device_register(&_mt53xx_uart0_device);
#ifndef CC_NO_KRL_UART_DRV
    platform_device_register(&_mt53xx_uart1_device);
#endif
    platform_device_register(&_mt53xx_uart2_device);
    platform_device_register(&_mt53xx_uart3_device);
#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5891)
    platform_device_register(&_mt53xx_uart4_device);
    platform_device_register(&_mt53xx_uart5_device);
#endif
    ret = platform_driver_register(&_mt53xx_uart_driver);
    printk(KERN_DEBUG "Serial:  platform_driver_register %d\n", ret);
    if (ret == 0)
    {
        goto out;
    }

out:
    return ret;
}

static void __exit _mt53xx_uart_exit(void)
{
#if 1//def CC_LOG_PERFORMANCE_SUPPORT
		if(_hUart0Thread)
		{
			kthread_stop(_hUart0Thread);
			_hUart0Thread = NULL;
		}
		if(_au0TxBuf)
		{
			vfree(_au0TxBuf);
			_au0TxBuf = NULL;
		}
#endif

#ifdef CC_LOG2USB_SUPPORT
    _log2usb_cleanup_module();
#endif
    uart_unregister_driver(&_mt53xx_uart_reg);
}

module_init(_mt53xx_uart_init);
module_exit(_mt53xx_uart_exit);
#ifdef INCOMPLETE_CMD_SUPPORT
__setup("uart_mode=", hotel_mode_setup);
#endif


MODULE_AUTHOR("MediaTek Inc.");
MODULE_DESCRIPTION("MT53xx serial port driver $Revision: #3 $");
MODULE_LICENSE("MKL");
