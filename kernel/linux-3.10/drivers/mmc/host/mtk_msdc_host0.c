/*----------------------------------------------------------------------------*
 * No Warranty                                                                *
 * Except as may be otherwise agreed to in writing, no warranties of any      *
 * kind, whether express or implied, are given by MTK with respect to any MTK *
 * Deliverables or any use thereof, and MTK Deliverables are provided on an   *
 * "AS IS" basis.  MTK hereby expressly disclaims all such warranties,        *
 * including any implied warranties of merchantability, non-infringement and  *
 * fitness for a particular purpose and any warranties arising out of course  *
 * of performance, course of dealing or usage of trade.  Parties further      *
 * acknowledge that Company may, either presently and/or in the future,       *
 * instruct MTK to assist it in the development and the implementation, in    *
 * accordance with Company's designs, of certain softwares relating to        *
 * Company's product(s) (the "Services").  Except as may be otherwise agreed  *
 * to in writing, no warranties of any kind, whether express or implied, are  *
 * given by MTK with respect to the Services provided, and the Services are   *
 * provided on an "AS IS" basis.  Company further acknowledges that the       *
 * Services may contain errors, that testing is important and Company is      *
 * solely responsible for fully testing the Services and/or derivatives       *
 * thereof before they are used, sublicensed or distributed.  Should there be *
 * any third party action brought against MTK, arising out of or relating to  *
 * the Services, Company agree to fully indemnify and hold MTK harmless.      *
 * If the parties mutually agree to enter into or continue a business         *
 * relationship or other arrangement, the terms and conditions set forth      *
 * hereunder shall remain effective and, unless explicitly stated otherwise,  *
 * shall prevail in the event of a conflict in the terms in any agreements    *
 * entered into between the parties.                                          *
 *---------------------------------------------------------------------------*/
/******************************************************************************
* [File]			mtk_msdc_host0.c
* [Version]			v1.0
* [Revision Date]	2011-05-04
* [Author]			Shunli Wang, shunli.wang@mediatek.inc, 82896, 2011-05-04
* [Description]
*	MSDC Driver Source File
* [Copyright]
*	Copyright (C) 2011 MediaTek Incorporation. All Rights Reserved.
******************************************************************************/
#include <linux/delay.h>
#include <asm/delay.h>
#include <linux/highmem.h>
#include <linux/pci.h>
#include <linux/dma-mapping.h>

#include <linux/platform_device.h>
#include <linux/mmc/host.h>
#include <linux/mmc/card.h>
#include <linux/mmc/mmc.h>
#include <linux/mmc/sd.h>
#include <asm/scatterlist.h>
#include <linux/jiffies.h>
#include <linux/proc_fs.h>
#include <linux/module.h>
#include <linux/random.h>
#include <mach/irqs.h>
#include <mach/memory.h>
#include <mach/mtk_gpio.h>
#include "x_typedef.h"
#include "mtk_msdc_host_hw.h"
#include "mtk_msdc_slave_hw.h"
#include "mtk_msdc_drv.h"

#define DRIVER_NAME0 "MTK_MSDC0"
#define DRIVER_VERSION0 "0.10"

//#define BUGMAIL "Mediatek. Inc"

const int ch0 = 0;
static struct msdc_host *msdc_host0 = NULL;
static struct completion comp0;

#ifdef CONFIG_MTKEMMC_BOOT
extern struct semaphore msdc_host_sema;
#endif

#ifdef CONFIG_OPM
extern int force_suspend;
#endif

extern struct msdc_host *msdc_host_boot;
extern int (*msdc_send_command)(struct msdc_host *host, struct mmc_command *cmd);

/* Removable Card related */
#ifdef CC_SDMMC_SUPPORT	
#ifdef CONFIG_MT53XX_NATIVE_GPIO
#define MSDC_GPIO_MAX_NUMBERS 6     
int MSDC_Gpio[MSDC_GPIO_MAX_NUMBERS]= {0} ;
#endif 
struct tasklet_struct card_tasklet;	/* Tasklet structures */
struct tasklet_struct finish_tasklet;
struct timer_list card_detect_timer;	/* card detection timer */	
UINT32 plugCnt = 0;  /* count fast plug out-in times */	
UINT32 detectFlag = 0; /* detect flag */
UINT32 _bMSDCCardExist = 0;
UINT32 _sd_gpio_isr = 0;
#endif

/* the global variable related to clcok */
UINT32 msdcClk0[][MSDC_CLK_IDX_MAX] = {{MSDC_CLK_TARGET},
                                      {MSDC_CLK_SRC_VAL},
                                      {MSDC_CLK_MODE_VAL},
                                      {MSDC_CLK_DIV_VAL},
                                      {MSDC_CLK_DRV_VAL}};

/* the global variable related to sample edge */
#if defined(CC_MT5399) || defined(CONFIG_ARCH_MT5399) || defined(CC_MT5890) || defined(CONFIG_ARCH_MT5890)
const EMMC_FLASH_DEV_T _arEMMC_DevInfo0[] =
{
    // Device name                                      ID1               ID2         DS26Sample    DS26Delay    HS52Sample    HS52Dleay   DDR52Sample DDR52Delay  HS200Sample  HS200Delay
    { "UNKNOWN",                   0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000006, 0x00000000, 0x00000102, 0x00000000, 0x00000006, 0x00000000},
    { "H26M21001ECR",              0x4A48594E, 0x49582056, 0x00000000, 0x00000000, 0x00000006, 0x00000000, 0x00000102, 0x00000000, 0x00000006, 0x00000000},
    { "H26M31003FPR",              0x4A205849, 0x4E594812, 0x00000000, 0x00000000, 0x00000006, 0x00000000, 0x00000102, 0x00000000, 0x00000006, 0x00000000},
    { "SDIN5D2-4G",                0x0053454D, 0x30344790, 0x00000000, 0x00000000, 0x00000006, 0x00000000, 0x00000102, 0x00000000, 0x00000006, 0x00000000},
    { "KLM2G1HE3F-B001",           0x004D3247, 0x31484602, 0x00000000, 0x00000000, 0x00000006, 0x00000000, 0x00000102, 0x00000000, 0x00000006, 0x00000000},
    { "THGBM3G4D1FBAIG",           0x00303032, 0x47303000, 0x00000000, 0x00000000, 0x00000006, 0x00000000, 0x00000102, 0x00000000, 0x00000006, 0x00000000},
    { "THGBM4G4D1FBAIG",           0x00303032, 0x47343900, 0x00000000, 0x00000000, 0x00000006, 0x00000000, 0x00000102, 0x00000000, 0x00000006, 0x00000000},
    { "THGBM4G5D1HBAIR",           0x00303034, 0x47343900, 0x00000000, 0x00000000, 0x00000006, 0x00000000, 0x00000102, 0x00000000, 0x00000006, 0x00000000},
    { "THGBM4G6D2HBAIR",           0x00303038, 0x47344200, 0x00000000, 0x00000000, 0x00000006, 0x00000000, 0x00000102, 0x00000000, 0x00000006, 0x00000000},
    { "MTFC4GMVEA-1M WT(JW857)",   0x4E4D4D43, 0x3034473A, 0x00000000, 0x00000000, 0x00000006, 0x00000000, 0x00000102, 0x0000000A, 0x00000006, 0x00000000},
};
#else
const EMMC_FLASH_DEV_T _arEMMC_DevInfo0[] =
{
    // Device name              ID1         ID2      Sample
    { "UNKNOWN",            0x00000000, 0x00000000, 0x00110000},
    { "H26M21001ECR",       0x4A48594E, 0x49582056, 0x00101100},
    { "H26M31003FPR",       0x4A205849, 0x4E594812, 0x00110000},
    { "SDIN5D2-4G",         0x0053454D, 0x30344790, 0x00101100},
    { "KLM2G1HE3F-B001",    0x004D3247, 0x31484602, 0x00000000},
    { "THGBM3G4D1FBAIG",    0x00303032, 0x47303000, 0x00000100},
    { "THGBM4G4D1FBAIG",    0x00303032, 0x47343900, 0x00110100},
    { "THGBM4G5D1HBAIR",    0x00303034, 0x47343900, 0x00110100},
    { "THGBM4G6D2HBAIR",    0x00303038, 0x47344200, 0x00110100},
    { "2FA18-JW812",        0x4E4D4D43, 0x30344734, 0x00110000},
};
#endif

void MsdcFindDev0(UINT32 *pCID)
{
    UINT32 i, devNum;

	/*
      * why we need to define the id mask of emmc
      * Some vendors' emmc has the same brand & type but different product revision.
      * That means the firmware in eMMC has different revision
      * We should treat these emmcs as same type and timing
      * So id mask can ignore this case
      */
    UINT32 idMask = 0xFFFFFF00;

    devNum = sizeof(_arEMMC_DevInfo0)/sizeof(EMMC_FLASH_DEV_T);
    MSDC_LOG(MSG_LVL_INFO, "[0]%08X:%08X:%08X:%08X\n", pCID[0], pCID[1], pCID[2], pCID[3]);
    MSDC_LOG(MSG_LVL_INFO, "[0]id1:%08X id2:%08X\n", ID1(pCID), ID2(pCID));

    for(i = 0; i<devNum; i++)
    {
        if((_arEMMC_DevInfo0[i].u4ID1 == ID1(pCID)) &&
           ((_arEMMC_DevInfo0[i].u4ID2 & idMask) == (ID2(pCID) & idMask)))
        {
            break;
        }
    }

    msdc_host0->devIndex = (i == devNum)?0:i;

    MSDC_LOG(MSG_LVL_TITLE, "[0]eMMC Name: %s\n", _arEMMC_DevInfo0[msdc_host0->devIndex].acName);	
}

UINT8 MsdcCheckSumCal0(UINT8 *buf, UINT32 len)
{
    UINT32 i = 0, sum = 0;

    for(; i<len;i++)
    {
        sum += *(buf + i);
    }

    return (0xFF-(UINT8)sum);
}

void MsdcDescriptorDump0(struct msdc_host *host)
{
    msdc_gpd_t *gpd;
    UINT32 i = 0;

	
    gpd = host->gpd;
	printk("addr - %08x\n", (UINT32)(host->gpd_addr));
	for(;i<2*sizeof(msdc_gpd_t);i++)
	{
        printk("%02x ", *((u8*)(gpd)+i));
	}
	
}

void MsdcDescriptorConfig0(struct msdc_host *host, struct mmc_data *data)
{
    UINT32 bdlen, i;
    int nents;
    struct scatterlist *sg;
    INT32 dir = DMA_FROM_DEVICE;
    msdc_gpd_t *gpd;
    msdc_bd_t *bd;
    
    bdlen = data->sg_len;
    sg = data->sg;
    gpd = host->gpd;
    bd = host->bd;
    dir = (data->flags & MMC_DATA_READ)?DMA_FROM_DEVICE:DMA_TO_DEVICE;

    nents = dma_map_sg(mmc_dev(host->mmc), data->sg, data->sg_len, dir);
    if(0 == nents)
    {
        printk(KERN_ERR "[0]bad sg map!!\n");
        return;
    }

    /* modify gpd*/
    //gpd->intr = 0; 
    gpd->hwo = 1;  /* hw will clear it */
    gpd->bdp = 1;     
    gpd->chksum = 0;  /* need to clear first. */   
    gpd->chksum = MsdcCheckSumCal0((UINT8 *)gpd, 16);

    /* modify bd*/          
    for (i = 0; i < bdlen; i++) 
    {
        //msdc_init_bd(&bd[j], blkpad, dwpad, sg_dma_address(sg), sg_dma_len(sg)); 
        bd[i].pBuff = (void *)(sg_dma_address(sg));
        bd[i].buffLen = sg_dma_len(sg);          
        if(i == bdlen - 1) 
        {
            bd[i].eol = 1;     	/* the last bd */
        } 
        else 
        {
            bd[i].eol = 0; 	
        }

        bd[i].chksum = 0; /* checksume need to clear first */
        bd[i].chksum = MsdcCheckSumCal0((UINT8 *)(&bd[i]), 16);         
        sg++;
    }

    mb();

    // Config the DMA HW registers
    MSDC_SETBIT(DMA_CFG(ch0), DMA_CFG_CHKSUM);
    MSDC_WRITE32(DMA_SA(ch0), 0x0);
    MSDC_SETBIT(DMA_CTRL(ch0), DMA_CTRL_BST_64);
    MSDC_SETBIT(DMA_CTRL(ch0), DMA_CTRL_DESC_MODE);

    MSDC_WRITE32(DMA_SA(ch0), (UINT32)host->gpd_addr);

}

void MsdcDescriptorFlush0(struct msdc_host *host, struct mmc_data *data)
{
    UINT32 tmpSgLen;
    struct scatterlist *tmpSg;
    INT32 dir = DMA_FROM_DEVICE;

    tmpSgLen = data->sg_len;
    tmpSg = data->sg;
    dir = (data->flags & MMC_DATA_READ)?DMA_FROM_DEVICE:DMA_TO_DEVICE;

    if(tmpSgLen == 0)
        return;	
        
    dma_unmap_sg(mmc_dev(host->mmc), data->sg, data->sg_len, dir);
    
}

void MsdcSetSample0(UINT8 fgFalling)
{
    // Set command response sample selection
    if(fgFalling & 0x0F)
    {
        MSDC_SETBIT(MSDC_IOCON(ch0), ((0x1) << 1)); 
    } 
    else
    {
        MSDC_CLRBIT(MSDC_IOCON(ch0), ((0x1) << 1));
    }
	    
    // Set read data sample selection
    if((fgFalling>>4) & 0x0F)
    {
        MSDC_SETBIT(MSDC_IOCON(ch0), ((0x1) << 2));  
    }
    else
    {
        MSDC_CLRBIT(MSDC_IOCON(ch0), ((0x1) << 2));
    }		
}

void MsdcClrTiming0(void)
{
    // Clear Sample Edge
    MSDC_CLRBIT(MSDC_IOCON(ch0), (((UINT32)0xF) << 0));

    // Clear Pad Tune
    MSDC_WRITE32(PAD_TUNE(ch0), 0x00000000);
    
    // Clear 
    MSDC_WRITE32(DAT_RD_DLY0(ch0), 0x00000000);
    MSDC_WRITE32(DAT_RD_DLY1(ch0), 0x00000000);

#if defined(CC_MT5396) || defined(CC_MT5368) || defined(CC_MT5389) || \
    defined(CONFIG_ARCH_MT5396) || defined(CONFIG_ARCH_MT5368) || defined(CONFIG_ARCH_MT5389)
    MSDC_LOG(MSG_LVL_ERR, "[0]PAD_CLK_SMT Setting\n");
    MSDC_SETBIT(SD20_PAD_CTL0(ch0), (0x1<<18));
#endif

#if defined(CC_MT5399) || defined(CONFIG_ARCH_MT5399) || defined(CC_MT5890) || defined(CONFIG_ARCH_MT5890)
    MSDC_LOG(MSG_LVL_ERR, "[0]driving strength setting!\n");
    MSDC_SETBIT(SD30_PAD_CTL0(ch0), (0x0<<4) | (0x7<<0));
    MSDC_SETBIT(SD30_PAD_CTL1(ch0), (0x0<<4) | (0x7<<0));
    MSDC_SETBIT(SD30_PAD_CTL2(ch0), (0x0<<4) | (0x7<<0));
#endif

}

#if  defined(CC_MT5880) || defined(CONFIG_ARCH_MT5880)
void MsdcSetSampleEdge0(UINT32 fgWhere)
{
    UINT8 u1Sample;

    u1Sample = (_arEMMC_DevInfo0[msdc_host0->devIndex].u4Sample>>(8*fgWhere)) & 0xFF;

    MSDC_LOG(MSG_LVL_ERR, "[0]Dev Num: %d, Timing Position: %d, Sample Edge: 0x%02X\n", msdc_host0->devIndex, fgWhere, u1Sample);

    // Set Sample Edge
    MsdcSetSample0(u1Sample);

}
#endif

INT32 MsdcReset0(void)
{
#if 0
    UINT32 i;

	// Reset MSDC
    MSDC_SETBIT(MSDC_CFG(ch0), MSDC_CFG_RST);

    for(i = 0; i<MSDC_RST_TIMEOUT_LIMIT_COUNT; i++)
    {
        if (0 == (MSDC_READ32(MSDC_CFG(ch0)) & MSDC_CFG_RST))
        {
            break;
        }

        //HAL_Delay_us(1000);
        udelay(1000);
    }
    if(i == MSDC_RST_TIMEOUT_LIMIT_COUNT)
    {
        return MSDC_FAILED;
    }
#else

    unsigned long u4JiffSt = 0;
    UINT32 u4Val;

    // Reset MSDC
    MSDC_SETBIT(MSDC_CFG(ch0), MSDC_CFG_RST);

    u4JiffSt = jiffies;
    while(1)
    {
        u4Val = (MSDC_READ32(MSDC_CFG(ch0)) & MSDC_CFG_RST);
        if(u4Val == 0)
        {
            break;
        }
        else
        {
            if(time_after(jiffies, u4JiffSt + 20/(1000/HZ)))
            {
                MSDC_LOG(MSG_LVL_WARN, "[0]Wait HOST Controller Stable Timeout!\r\n");
                return MSDC_FAILED;
            }
        }
    }
#endif

    return MSDC_SUCCESS;

}

INT32 MsdcClrFifo0(void)
{
#if 0
    UINT32 i;
    // Reset FIFO
    MSDC_SETBIT(MSDC_FIFOCS(ch0), MSDC_FIFOCS_FIFOCLR);

    for(i = 0; i<MSDC_RST_TIMEOUT_LIMIT_COUNT; i++)
    {
        if (0 == (MSDC_READ32(MSDC_FIFOCS(ch0)) & (MSDC_FIFOCS_FIFOCLR | MSDC_FIFOCS_TXFIFOCNT_MASK | MSDC_FIFOCS_RXFIFOCNT_MASK)))
        {
            break;
        }

        //HAL_Delay_us(1000);
        udelay(1000);
    }
    if(i == MSDC_FIFOCLR_TIMEOUT_LIMIT_COUNT)
    {
        return MSDC_FAILED;
    }
#else

    unsigned long u4JiffSt = 0;
    UINT32 u4Val;

    // Reset FIFO
    MSDC_SETBIT(MSDC_FIFOCS(ch0), MSDC_FIFOCS_FIFOCLR);

    u4JiffSt = jiffies;
    while(1)
    {
        u4Val = (MSDC_READ32(MSDC_FIFOCS(ch0)) & (MSDC_FIFOCS_FIFOCLR | MSDC_FIFOCS_TXFIFOCNT_MASK | MSDC_FIFOCS_RXFIFOCNT_MASK));
        if(u4Val == 0)
        {
            break;
        }
        else
        {
            if(time_after(jiffies, u4JiffSt + 20/(1000/HZ)))
            {
                MSDC_LOG(MSG_LVL_WARN, "[0]Wait HOST Controller FIFO Clear Timeout!\r\n");
                return MSDC_FAILED;
            }
        }
    }
#endif

    return MSDC_SUCCESS;
}

void MsdcChkFifo0(void)
{
    // Check if rx/tx fifo is zero
    if ((MSDC_READ32(MSDC_FIFOCS(ch0)) & (MSDC_FIFOCS_TXFIFOCNT_MASK | MSDC_FIFOCS_RXFIFOCNT_MASK)) != 0)
    {
        MSDC_LOG(MSG_LVL_INFO, "[0]FiFo not 0, FIFOCS:0x%08X !!\r\n", MSDC_READ32(MSDC_FIFOCS(ch0)));
        MsdcClrFifo0();
    }
}

void MsdcClrIntr0(void)
{
    // Check MSDC Interrupt vector register
    if  (0x00  != MSDC_READ32(MSDC_INT(ch0)))
    {
        MSDC_LOG(MSG_LVL_INFO, "[0]MSDC INT(0x%08X) not 0:0x%08X !!\r\n", MSDC_INT(ch0), MSDC_READ32(MSDC_INT(ch0)));

        // Clear MSDC Interrupt vector register
        MSDC_WRITE32(MSDC_INT(ch0), MSDC_READ32(MSDC_INT(ch0)));
    }
}

void MsdcStopDMA0(void)
{
    MSDC_LOG(MSG_LVL_INFO, "[0]DMA status: 0x%.8x\n",MSDC_READ32(DMA_CFG(ch0)));
	
    MSDC_SETBIT(DMA_CTRL(ch0), DMA_CTRL_STOP);
    while(MSDC_READ32(DMA_CFG(ch0)) & DMA_CFG_DMA_STATUS);
	
    MSDC_LOG(MSG_LVL_INFO, "[0]DMA Stopped!\n");
}

INT32 MsdcWaitClkStable0(void)
{
#if 0
    UINT32 i;

    for(i = 0; i<MSDC_CLK_TIMEOUT_LIMIT_COUNT; i++)
    {
        if (0 != (MSDC_READ32(MSDC_CFG(ch0)) & MSDC_CFG_CARD_CK_STABLE))
        {
            break;
        }

        //HAL_Delay_us(1000);
        udelay(1000);
    }
	  if(i == MSDC_CLK_TIMEOUT_LIMIT_COUNT)
	  {
        MSDC_LOG(MSG_LVL_ERR, "WaitClkStable Failed !\r\n");
        return MSDC_FAILED;
	  }
#else

    unsigned long u4JiffSt = 0;
    UINT32 u4Val;

    u4JiffSt = jiffies;
    while(1)
    {
        u4Val = (MSDC_READ32(MSDC_CFG(ch0)) & MSDC_CFG_CARD_CK_STABLE);
        if(u4Val != 0)
        {
            break;
        }
        else
        {
            if(time_after(jiffies, u4JiffSt + 20/(1000/HZ)))
            {
                MSDC_LOG(MSG_LVL_WARN, "[0]Wait HOST Controller Clock Stable Timeout!\r\n");
                return MSDC_FAILED;
            }
        }
    }
#endif

    return MSDC_SUCCESS;
}

#ifdef CC_SDMMC_SUPPORT	
static irqreturn_t MsdcHandleGPIOIsr0(int irq, void *dev_id)
{
    MSDC_LOG(MSG_LVL_ERR, "[0][MsdcHandleGPIOIsr0, %d]---------------------->!\n", plugCnt);
    plugCnt++;

	//SD_CARD_DETECT_GPIO_STS_CLR;
    return IRQ_HANDLED;
}

static void MsdcSetGPIOIsr0(UINT32 u4Mask, struct msdc_host *host)
{
    SD_GPIO_INTERRUPT_EN;
}

static INT32 MsdcRegGPIOIsr0(struct msdc_host *host)
{
	MSDC_LOG(MSG_LVL_INFO, "[0]Register GPIO ISR\n");
    if (request_irq(VECTOR_EXT1, MsdcHandleGPIOIsr0, IRQF_SHARED, DRIVER_NAME0, host) != 0)
    {
        MSDC_LOG(MSG_LVL_ERR, "Request MSDC GPIO IRQ fail!\n");
    }  

    return 0;
}
#endif

static irqreturn_t MsdcHandleIsr0(int irq, void *dev_id)
{
    volatile UINT32 u4TmpAccuVect = 0x00;
    UINT32 u4TriggerComplete = 0;

    u4TmpAccuVect = MSDC_READ32(MSDC_INT(ch0));

    msdc_host0->MsdcAccuVect |= u4TmpAccuVect;

    MSDC_LOG(MSG_LVL_TRACE, "[0][MSDC TMP]%08X!\r\n", u4TmpAccuVect);

    MSDC_WRITE32(MSDC_INT(ch0), u4TmpAccuVect);

    if((msdc_host0->waitIntMode == 0) && ((msdc_host0->MsdcAccuVect & msdc_host0->desVect) != 0))
    {
        MSDC_LOG(MSG_LVL_TRACE, "[0][MSDC FINAL]%08X!\r\n", msdc_host0->MsdcAccuVect);
        u4TriggerComplete = 1;
        //complete(&comp1);
    }
    else if((msdc_host0->waitIntMode == 1) && ((msdc_host0->MsdcAccuVect & msdc_host0->desVect) == msdc_host0->desVect))
    {
        MSDC_LOG(MSG_LVL_TRACE, "[0][MSDC FINAL]%08X!\r\n", msdc_host0->MsdcAccuVect);
        u4TriggerComplete = 1;
        //complete(&comp1);
    }
    else
    {
        // do nothing
    }

    if((msdc_host0->MsdcAccuVect & INT_SD_DATA_CRCERR) != 0)
    {
        // return directly
        MSDC_LOG(MSG_LVL_TRACE, "[0][MSDC FINAL]%08X!\r\n", msdc_host0->MsdcAccuVect);
        u4TriggerComplete = 1;
        //complete(&comp1);
    }	

    if((msdc_host0->MsdcAccuVect & INT_SD_AUTOCMD_RESP_CRCERR) != 0)
    {
        // return directly
        MSDC_LOG(MSG_LVL_TRACE, "[0][MSDC FINAL]%08X!\r\n", msdc_host0->MsdcAccuVect);
        u4TriggerComplete = 1;
        //complete(&comp1);
    }
	
    if (u4TriggerComplete == 1)
    {
        complete(&comp0);
    }

    return IRQ_HANDLED;
}

static void MsdcSetIsr0(UINT32 u4Mask, struct msdc_host *host)
{
    MSDC_SETBIT(MSDC0_EN_REG, (0x1<<MSDC0_EN_SHIFT));
    MSDC_WRITE32(MSDC_INTEN(ch0), u4Mask);
    enable_irq(host->irq);
}

static INT32 MsdcRegIsr0(struct msdc_host *host)
{
    MsdcClrIntr0();  // Clear interrupt, read clear

    if (request_irq(host->irq, MsdcHandleIsr0, IRQF_SHARED, DRIVER_NAME0, host) != 0)
    {
        printk(KERN_ERR "[0]Request MSDC0 IRQ fail!\n");
        return -1;
    }

#ifdef CC_SDMMC_SUPPORT	
    MSDC_LOG(MSG_LVL_INFO, "[0]Register GPIO ISR\n");
    if (request_irq(VECTOR_EXT1, MsdcHandleGPIOIsr0, IRQF_SHARED, DRIVER_NAME0, host) != 0)
    {
        MSDC_LOG(MSG_LVL_ERR, "Request MSDC GPIO IRQ fail!\n");
    }  
#endif

    MsdcClrIntr0();  // Clear interrupt, read clear
    disable_irq(host->irq);

    return 0;

}

void MsdcSetTimeout(struct msdc_host *host, UINT32 ns, UINT32 clks)
{
    UINT32 timeout, clk_ns;

	host->timeout_ns = ns;
	host->timeout_clks = clks;

	clk_ns = 1000000000UL/host->clk;
	timeout = ns/clk_ns + clks;
	timeout = timeout >> 16;
	timeout = (timeout > 1)? (timeout-1):0;
	timeout = (timeout > 255)?255:(timeout);

	MSDC_WRITE32(SDC_CFG(ch0), (timeout<<SDC_CFG_DTOC_SHIFT));
}

void MsdcPinMux0(void)
{
    MSDC_LOG(MSG_LVL_INFO, "[0]msdc0 pinmux for mt%d\n", CONFIG_CHIP_VERSION);

#if defined(CC_MT5396) || defined(CONFIG_ARCH_MT5396) || \
	defined(CC_MT5368) || defined(CONFIG_ARCH_MT5368) || \
	defined(CC_MT5389) || defined(CONFIG_ARCH_MT5389)	  

    MSDC_CLRBIT(0xF000D408, 0x03);
    MSDC_SETBIT(0xF000D408, 0x01);
    
	//Local Arbitor open	
    MSDC_CLRBIT(0xF0012200, (0x03<<16) | (0x1F<<0));	 
    MSDC_SETBIT(0xF0012200, (0x01<<16) | (0x1F<<0));

#elif defined(CC_MT5398) ||defined(CONFIG_ARCH_MT5398) || \
	  defined(CC_MT5399) ||defined(CONFIG_ARCH_MT5399) || \
	  defined(CC_MT5890) || defined(CONFIG_ARCH_MT5890)

    //pinmux register d604[27:26], function 1 - CMD/CLK/DAT0~DAT3		 
    MSDC_CLRBIT(0xF000D604, 0x03<<26); 	   
    MSDC_SETBIT(0xF000D604, 0x01<<26);				 

    //Local Arbitor open	
    MSDC_CLRBIT(0xF0012200, (0x03<<16) | (0x1F<<0));	 
    MSDC_SETBIT(0xF0012200, (0x01<<16) | (0x1F<<0));	
      
#elif  defined(CC_MT5880) || defined(CONFIG_ARCH_MT5880)	

    if (IS_IC_MT5860_E2IC()) // sd card
    {
        MSDC_LOG(MSG_LVL_INFO, "[0]msdc0(sd) pinmux for mt5860\n");			
        MSDC_CLRBIT(0xF000D620, 0x1<<7);
        MSDC_SETBIT(0xF000D620, 0x1<<7);
        MSDC_CLRBIT(0xF000D610, 0x3FFFF<<12);
        MSDC_SETBIT(0xF000D610, 0x12492<<12);
        //Local Arbitor open
        MSDC_CLRBIT(0xF0012200, (0x03<<16) | (0x1F<<0));	 
        MSDC_SETBIT(0xF0012200, (0x01<<16) | (0x1F<<0));
    }
    else // emmc
    {
        MSDC_LOG(MSG_LVL_INFO, "[0]msdc0(emmc) pinmux for mt5860\n");			
        MSDC_CLRBIT(0xF000D604, 0x03<<4);
        MSDC_SETBIT(0xF000D604, 0x02<<4);
    }
#else

    MSDC_CLRBIT(0xF000D408, 0x03);
    MSDC_SETBIT(0xF000D408, 0x01);

    //Local Arbitor open	
    MSDC_CLRBIT(0xF0012200, (0x03<<16) | (0x1F<<0));	 
    MSDC_SETBIT(0xF0012200, (0x01<<16) | (0x1F<<0));

#endif	  
}

INT32 MsdcInit0(void)
{
    // Pinmux Switch
    MsdcPinMux0();

	MSDC_LOG(MSG_LVL_INFO, "[0]msdc1 reset for mt%d\n", CONFIG_CHIP_VERSION);

#if defined(CC_MT5396) || defined(CONFIG_ARCH_MT5396) || \
	defined(CC_MT5368) || defined(CONFIG_ARCH_MT5368) || \
	defined(CC_MT5389) || defined(CONFIG_ARCH_MT5389)

    // Reset MSDC
    MsdcReset0();
    // Set SD/MMC Mode
    MSDC_SETBIT(MSDC_CFG(ch0), MSDC_CFG_SD);
	
#elif defined(CC_MT5398) || defined(CONFIG_ARCH_MT5398) || \
      defined(CC_MT5880) || defined(CONFIG_ARCH_MT5880) || \
      defined(CC_MT5881) || defined(CONFIG_ARCH_MT5881) || \
      defined(CC_MT5860) || defined(CONFIG_ARCH_MT5860) || \
      defined(CC_MT5399) || defined(CONFIG_ARCH_MT5399) || \
      defined(CC_MT5890) || defined(CONFIG_ARCH_MT5890)

    // Set SD/MMC Mode
    MSDC_SETBIT(MSDC_CFG(ch0), MSDC_CFG_SD);
    // Reset MSDC
    MsdcReset0();
	
#else

    // Reset MSDC
    MsdcReset0();
    // Set SD/MMC Mode
    MSDC_SETBIT(MSDC_CFG(ch0), MSDC_CFG_SD);
	
#endif
	
    if(msdc_host0->data_mode > 1)
    {
        // Set DMA Mode
        MSDC_CLRBIT(MSDC_CFG(ch0), MSDC_CFG_PIO_MODE);
    }
    else
    {
        // Set PIO Mode
        MSDC_SETBIT(MSDC_CFG(ch0), MSDC_CFG_PIO_MODE);
    }

    // Disable sdio & Set bus to 1 bit mode
    MSDC_CLRBIT(SDC_CFG(ch0), SDC_CFG_SDIO | SDC_CFG_BW_MASK);

    // set clock mode (DIV mode)
    MSDC_CLRBIT(MSDC_CFG(ch0), (((UINT32)0x03) << 16));

    // Wait until clock is stable
    if (MSDC_FAILED == MsdcWaitClkStable0())
    {
        return MSDC_FAILED;
    }

    // Set default RISC_SIZE for DWRD pio mode
    MSDC_WRITE32(MSDC_IOCON(ch0), (MSDC_READ32(MSDC_IOCON(ch0)) & ~MSDC_IOCON_RISC_SIZE_MASK) | MSDC_IOCON_RISC_SIZE_DWRD);

    // Set Data timeout setting => Maximum setting
    MSDC_WRITE32(SDC_CFG(ch0), (MSDC_READ32(SDC_CFG(ch0)) & ~(((UINT32)0xFF) << SDC_CFG_DTOC_SHIFT)) | (((UINT32)0xFF) << SDC_CFG_DTOC_SHIFT));

    MsdcClrTiming0();

    return MSDC_SUCCESS;
}


void MsdcContinueClock0(int i4ContinueClock)
{
    if (i4ContinueClock)
    {
       // Set clock continuous even if no command
       MSDC_SETBIT(MSDC_CFG(ch0), (((UINT32)0x01) << 1));
    }
    else
    {
       // Set clock power down if no command
       MSDC_CLRBIT(MSDC_CFG(ch0), (((UINT32)0x01) << 1));
    }
}

INT32 MsdcWaitHostIdle0(void)
{
#if 0
    UINT32 i;
    for(i=0;i<MSDC_WAIT_SDC_BUS_TIMEOUT_LIMIT_COUNT;i++)
    {
        if ((0 == (MSDC_READ32(SDC_STS(ch0)) & (SDC_STS_SDCBUSY | SDC_STS_CMDBUSY))) && (0x00  == MSDC_READ32(MSDC_INT(ch0))))
        {
            break;
        }
        //HAL_Delay_us(1000);
        udelay(1000);
    }
    if(i == MSDC_WAIT_SDC_BUS_TIMEOUT_LIMIT_COUNT)
    {
        return MSDC_FAILED;
    }
#else
    unsigned long u4JiffSt = 0;

    u4JiffSt = jiffies;
    while(1)
    {
        if ((0 == (MSDC_READ32(SDC_STS(ch0)) & (SDC_STS_SDCBUSY | SDC_STS_CMDBUSY))) && (0x00  == MSDC_READ32(MSDC_INT(ch0))))
        {
            break;
        }
        else
        {
            if(time_after(jiffies, u4JiffSt + 400/(1000/HZ)))
            {
                MSDC_LOG(MSG_LVL_WARN, "[0]Wait HOST Controller Idle Timeout!\r\n");
                return MSDC_FAILED;
            }
        }
    }
#endif

    return MSDC_SUCCESS;
}

INT32 MsdcWaitIntr0(UINT32 vector, UINT32 timeoutCnt, UINT32 fgMode)
{
    //UINT32 i; 
    UINT32 u4Ret;

    // Clear Vector variable
    msdc_host0->MsdcAccuVect = 0;
    msdc_host0->desVect = vector;
    msdc_host0->waitIntMode = fgMode;
    
    //if(!(MSDC_READ32(MSDC0_EN_REG)&(0x1<<MSDC0_EN_SHIFT)))
        //MSDC_SETBIT(MSDC0_EN_REG, (0x1<<MSDC0_EN_SHIFT));    

    if(!msdc_host0->msdc_isr_en)
    {
#if 0
        for(i = 0; i<timeoutCnt; i++)
        {
            // Status match any bit
            if (0 != (MSDC_READ32(MSDC_INT(ch0)) & vector))
            {
                msdc_host0->MsdcAccuVect |= MSDC_READ32(MSDC_INT(ch0));
                printk(KERN_ERR "->%08X\r\n", MSDC_READ32(MSDC_INT(ch0)));
                printk(KERN_ERR "->%08X, %08X, %08X, %08X, %08X\r\n", MSDC_READ32(MSDC_INTEN(ch0)), MSDC_READ32(0xF0008034), MSDC_READ32(0xF0008030), MSDC_READ32(0xF0008084), MSDC_READ32(0xF0008080));
                MSDC_WRITE32(MSDC_INT(ch0), msdc_host0->MsdcAccuVect);
                return MSDC_SUCCESS;
            }

            //HAL_Delay_us(1000);
            udelay(1000);
        }
#else
        unsigned long u4JiffSt = 0;

        u4JiffSt = jiffies;
        while(1)
        {
            if((fgMode == 0) && ((MSDC_READ32(MSDC_INT(ch0)) & vector) != 0))
            {
                msdc_host0->MsdcAccuVect |= MSDC_READ32(MSDC_INT(ch0));
                MSDC_WRITE32(MSDC_INT(ch0), msdc_host0->MsdcAccuVect);
                return MSDC_SUCCESS;
            }
            else if((fgMode == 1) && ((MSDC_READ32(MSDC_INT(ch0)) & vector) == vector))
            {
                msdc_host0->MsdcAccuVect |= MSDC_READ32(MSDC_INT(ch0));
                MSDC_WRITE32(MSDC_INT(ch0), msdc_host0->MsdcAccuVect);
                return MSDC_SUCCESS;
            }
            else
            {
                if(time_after(jiffies, u4JiffSt + timeoutCnt/(1000/HZ)))
                {
                    MSDC_LOG(MSG_LVL_WARN, "[0]Wait HOST Controller Interrupt Timeout!\r\n");
                    return MSDC_FAILED;
                }
            }
        }
#endif
    }
    else
    {
        u4Ret = wait_for_completion_timeout(&comp0, timeoutCnt);

        if(u4Ret == 0)
        {
            MSDC_LOG(MSG_LVL_WARN, "[0]Wait HOST Controller Interrupt Timeout!\r\n");
            return MSDC_FAILED;
        }
        else
        {
            if((msdc_host0->MsdcAccuVect & INT_SD_DATA_CRCERR) != 0)
            {
                MSDC_LOG(MSG_LVL_ERR, "[0]CRC ERROR, Interrupt Handle Function return directly!\n");
                return MSDC_FAILED;
            }

			if((msdc_host0->MsdcAccuVect & INT_SD_AUTOCMD_RESP_CRCERR) != 0)
			{
                MSDC_LOG(MSG_LVL_ERR, "[0]Auto CMD12 Response CRC ERROR, We don't care this case!\n");
                return MSDC_SUCCESS;
            }
			
            return MSDC_SUCCESS;
        }
  	}

    // Timeout case
    return MSDC_FAILED;
}
void MsdcSmapleDelay0(UINT32 flag)
{
#if defined(CC_MT5399) || defined(CONFIG_ARCH_MT5399)|| \
    defined(CC_MT5890) || defined(CONFIG_ARCH_MT5890)
    struct mmc_host *mmc;	
    mmc = msdc_host0->mmc;
	
    // Sample Edge init
    MSDC_CLRBIT(PAD_TUNE(ch0), 0xFFFFFFFF);

    // Sample Edge init
    if(mmc->ios.timing == MMC_TIMING_LEGACY)
    {
        MSDC_SETBIT(PAD_TUNE(ch0), _arEMMC_DevInfo0[msdc_host0->devIndex].DS26Delay);
    }
    else if(mmc->ios.timing == MMC_TIMING_MMC_HS)
    {
        MSDC_SETBIT(PAD_TUNE(ch0), _arEMMC_DevInfo0[msdc_host0->devIndex].HS52Delay);
    }
    else if(mmc->ios.timing == MMC_TIMING_UHS_DDR50)
    {
        MSDC_SETBIT(PAD_TUNE(ch0), _arEMMC_DevInfo0[msdc_host0->devIndex].DDR52Delay);
    }
    else if(mmc->ios.timing == MMC_TIMING_MMC_HS200)
    {
        MSDC_SETBIT(PAD_TUNE(ch0), _arEMMC_DevInfo0[msdc_host0->devIndex].HS200Delay);

        MSDC_CLRBIT(PATCH_BIT0(ch0), CKGEN_MSDC_DLY_SEL);
        MSDC_SETBIT(PATCH_BIT0(ch0), 0x3 << CKGEN_MSDC_DLY_SEL_SHIFT);
//        MSDC_CLRBIT(PATCH_BIT1(ch0), CMD_RSP_TA_CNTR);
//        MSDC_SETBIT(PATCH_BIT0(ch0), 0x2 << CMD_RSP_TA_CNTR_SHIFT);
    }
#endif
}
/* when I test TF card about DDR mode,  I find the truth:
* The write operation in DDR mode needs write status falling edge sample + 0xA delay for CMD/DAT line, 
* but rising edge sample should be set for the read operation in DDR mode
*/
void MsdcSampleEdge0(UINT32 flag)
{
    struct mmc_host *mmc;	
    mmc = msdc_host0->mmc;
	
#if defined(CC_MT5399) || defined(CONFIG_ARCH_MT5399)|| \
    defined(CC_MT5890) || defined(CONFIG_ARCH_MT5890)
    // Sample Edge init
    MSDC_CLRBIT(MSDC_IOCON(ch0), 0xFFFFFF);

    // Sample Edge init
    if(mmc->ios.timing == MMC_TIMING_LEGACY)
    {
        MSDC_SETBIT(MSDC_IOCON(ch0), _arEMMC_DevInfo0[msdc_host0->devIndex].DS26Sample & 0xFFFFFF);
    }
    else if(mmc->ios.timing == MMC_TIMING_MMC_HS)
    {
        MSDC_SETBIT(MSDC_IOCON(ch0), _arEMMC_DevInfo0[msdc_host0->devIndex].HS52Sample & 0xFFFFFF);
    }
    else if(mmc->ios.timing == MMC_TIMING_UHS_DDR50)
    {
        MSDC_SETBIT(MSDC_IOCON(ch0), _arEMMC_DevInfo0[msdc_host0->devIndex].DDR52Sample & 0xFFFFFF);
        MSDC_WRITE32(MSDC_CLK_H_REG1, 0x2);
    }
    else if(mmc->ios.timing == MMC_TIMING_MMC_HS200)
    {
        MSDC_SETBIT(MSDC_IOCON(ch0), _arEMMC_DevInfo0[msdc_host0->devIndex].HS200Sample & 0xFFFFFF);
        MSDC_WRITE32(MSDC_CLK_H_REG1, 0x2);
    }
#else
    UINT8 u1Sample = 0;
    UINT32 fgWhere = 0;

	if(msdc_host0->clk >= 26000000)
    {
        if(mmc->caps & MMC_CAP_MMC_HIGHSPEED)
        {
            fgWhere = 2;

            u1Sample = (_arEMMC_DevInfo0[msdc_host0->devIndex].u4Sample>>(8*fgWhere)) & 0xFF;
            
            MSDC_LOG(MSG_LVL_ERR, "[1]Dev Num: %d, Timing Position: %d, Sample Edge: 0x%02X\n", msdc_host0->devIndex, fgWhere, u1Sample);
            
            // Set Sample Edge
            // Set command response sample selection
            if(u1Sample & 0x0F)
            {
                MSDC_SETBIT(MSDC_IOCON(ch0), ((0x1) << MSDC_IOCON_R_SMPL_SHIFT));
            } 
            else
            {
                MSDC_CLRBIT(MSDC_IOCON(ch0), ((0x1) << MSDC_IOCON_R_SMPL_SHIFT));
            }
                
            // Set read data sample selection
            if((u1Sample>>4) & 0x0F)
            {
                MSDC_SETBIT(MSDC_IOCON(ch0), ((0x1) << MSDC_IOCON_D_SMPL_SHIFT));
            }
            else
            {
                MSDC_CLRBIT(MSDC_IOCON(ch0), ((0x1) << MSDC_IOCON_D_SMPL_SHIFT));
            }
        }
    }
#endif
}
/* different clock needs different max driving strength
*/
void MsdcDrivingStrength0(UINT32 driving)
{
    // PAD CTRL
    /* The value is determined by the experience
      * So, a best proper value should be investigate further.
      *   - when I test TF card, which is connected to main board by a SDIO daughter board,
      *      it will happen crc error in 48MHz, so I enhance pad drving strenght again.
      */
#if defined(CC_MT5399) || defined(CONFIG_ARCH_MT5399) || defined(CC_MT5890) || defined(CONFIG_ARCH_MT5890)
    MSDC_CLRBIT(SD30_PAD_CTL0(ch0), (0x7<<4) | (0x7<<0));
    MSDC_CLRBIT(SD30_PAD_CTL1(ch0), (0x7<<4) | (0x7<<0));
    MSDC_CLRBIT(SD30_PAD_CTL2(ch0), (0x7<<4) | (0x7<<0));	

	MSDC_SETBIT(SD30_PAD_CTL0(ch0), (((driving>>3)&0x7)<<4) | ((driving&0x7)<<0));
    MSDC_SETBIT(SD30_PAD_CTL1(ch0), (((driving>>3)&0x7)<<4) | ((driving&0x7)<<0));
    MSDC_SETBIT(SD30_PAD_CTL2(ch0), (((driving>>3)&0x7)<<4) | ((driving&0x7)<<0));	
#endif
}

#if defined(CC_MT5399) || defined(CONFIG_ARCH_MT5399) || defined(CC_MT5890) || defined(CONFIG_ARCH_MT5890)
INT32 MsdcSetClkfreq0(UINT32 clkFreq)
{
    UINT32 idx = 0, ddr = 0;
    struct mmc_host *mmc;	 

    clkFreq /= (1000*1000);		
    mmc = msdc_host0->mmc;
    MSDC_LOG(MSG_LVL_INFO, "[0]ios.timing = %d clock = %d!\n", mmc->ios.timing, clkFreq);
    ddr = (mmc->ios.timing == MMC_TIMING_UHS_DDR50)?1:0;
    do
    {
	    if((clkFreq < msdcClk0[0][idx]) ||
	       (ddr && (msdcClk0[2][idx] != 2)))
		    continue;
	    else
		    break;
	
    }while(++idx < MSDC_CLK_IDX_MAX);
    
    idx = (idx >= MSDC_CLK_IDX_MAX)?MSDC_CLK_IDX_MAX:idx;

    // Enable msdc_src_clk gate
    MSDC_SETBIT(MSDC_CLK_S_REG0, MSDC_CLK_GATE_BIT);

    // Set clock source value
    // Clr msdc_src_clk selection
    MSDC_CLRBIT(MSDC_CLK_S_REG0, MSDC_CLK_SEL_MASK);  
    MSDC_SETBIT(MSDC_CLK_S_REG0, msdcClk0[1][idx]<<0);	 

    // Set clock mode value
    MSDC_CLRBIT(MSDC_CFG(ch0), (((UINT32)0x03) << 16));
    MSDC_SETBIT(MSDC_CFG(ch0), ((msdcClk0[2][idx]) << 16));

    // Set clock divide value
    MSDC_CLRBIT(MSDC_CFG(ch0), (((UINT32)0xFF) << 8));
    MSDC_SETBIT(MSDC_CFG(ch0), ((msdcClk0[3][idx]) << 8));	


    // Disable msdc_src_clk gate
    MSDC_CLRBIT(MSDC_CLK_S_REG0, MSDC_CLK_GATE_BIT);


    // Wait until clock is stable
    if (MSDC_FAILED == MsdcWaitClkStable0())
    {
	    MSDC_LOG(MSG_LVL_ERR, "Set Bus Clock as %d(MHz) Failed!\n", msdcClk0[0][idx]);
	    return MSDC_FAILED;
    }

    MSDC_LOG(MSG_LVL_INFO, "Set Bus Clock as %d(MHz) Success!\n", msdcClk0[0][idx]); 
    msdc_host0->clk = msdcClk0[0][idx];

    MsdcDrivingStrength0(msdcClk0[4][idx]);

    return MSDC_SUCCESS;

}
#else
INT32 MsdcSetClkfreq0(UINT32 clkFreq)
{
    UINT32 sdClkSel = 0, expFreq = 0, index = 0;
    clkFreq /= (1000*1000);
    
    index = sizeof(msdcClk0[0])/sizeof(UINT32) - 1;
    if(clkFreq >= msdcClk0[0][0])
    {
        sdClkSel = msdcClk0[1][0];
        expFreq = msdcClk0[0][0];    	
    }
    else if(clkFreq <= msdcClk0[0][index])
    {
        sdClkSel = msdcClk0[1][index];
        expFreq = msdcClk0[0][index]; 	
    }
    else
    {
        for(index -= 1;index > 0;index--)
        {
            if((clkFreq >= msdcClk0[0][index]) && (clkFreq < msdcClk0[0][index-1]))
            {
                sdClkSel = msdcClk0[1][index];
                expFreq = msdcClk0[0][index]; 
                break;	
            }	
        }
    }
    MSDC_LOG(MSG_LVL_ERR, "[0]Request Clk:%dMHz Bus Clk:%dMHz, SelVal:%d\n", clkFreq, expFreq, sdClkSel);

    // Gate msdc_src_clk
    MSDC_SETBIT(MSDC_CLK_S_REG0, MSDC_CLK_GATE_BIT);

    // set clock mode (DIV mode)
    MSDC_CLRBIT(MSDC_CFG(ch0), (((UINT32)0x03) << 16));
    if (expFreq >= 1)
    {
        MSDC_SETBIT(MSDC_CFG(ch0), (((UINT32)0x01) << 16));
		
    }
    else
    {
        MSDC_CLRBIT(MSDC_CFG(ch0), (((UINT32)0xFF) << 8));
        MSDC_SETBIT(MSDC_CFG(ch0), (((UINT32)0x11) << 8));
    }
    
    // Clr msdc_src_clk selection
    MSDC_CLRBIT(MSDC_CLK_S_REG0, MSDC_CLK_SEL_MASK);  
    MSDC_SETBIT(MSDC_CLK_S_REG0, sdClkSel<<0);  

    // Disable gating msdc_src_clk
    MSDC_CLRBIT(MSDC_CLK_S_REG0, MSDC_CLK_GATE_BIT);    
	
    // Wait until clock is stable
    if (MSDC_FAILED == MsdcWaitClkStable0())
    {
        MSDC_LOG(MSG_LVL_ERR, "[0]Set Bus Clock as %d(MHz) Failed!\n", expFreq);
        return MSDC_FAILED;
    }

    MSDC_LOG(MSG_LVL_ERR, "[0]Set Bus Clock as %d(MHz) Success!\n", expFreq);	

    return MSDC_SUCCESS;
}
#endif

INT32 MsdcSetBusWidth0(INT32 busWidth)
{
    MSDC_LOG(MSG_LVL_INFO, "[0]Set Bus Width:%d\n", (1<<busWidth));

    /* Modify MSDC Register Settings */
    if (MMC_BUS_WIDTH_1 == busWidth)
    {
        MSDC_WRITE32(SDC_CFG(ch0), (MSDC_READ32(SDC_CFG(ch0)) & ~SDC_CFG_BW_MASK) | (0x00 <<  SDC_CFG_BW_SHIFT));
    }
    else if (MMC_BUS_WIDTH_4 == busWidth)
    {
        MSDC_WRITE32(SDC_CFG(ch0), (MSDC_READ32(SDC_CFG(ch0)) & ~SDC_CFG_BW_MASK) | (0x01 <<  SDC_CFG_BW_SHIFT));
    }
    else if (MMC_BUS_WIDTH_8 == busWidth)
    {
        MSDC_WRITE32(SDC_CFG(ch0), (MSDC_READ32(SDC_CFG(ch0)) & ~SDC_CFG_BW_MASK) | (0x02 <<  SDC_CFG_BW_SHIFT));
    }

    return MSDC_SUCCESS;
}

void MsdcSetIos0(struct mmc_host *mmc, struct mmc_ios *ios)
{
    UINT32 clock = ios->clock;
    UINT32 busWidth = ios->bus_width;

#ifdef CONFIG_OPM
    if (force_suspend != 0)
    {
        MSDC_LOG(MSG_LVL_INFO, "[0]MSDC skip set clock(%d) and buswidth(%d) if suspend ~~~~~~~~~~~~~~~~\n", clock, busWidth);
        return;
    }
#endif
    MsdcSetClkfreq0(0);
    MsdcSetClkfreq0(clock);
    MsdcSampleEdge0(0);
    MsdcSmapleDelay0(0);

    MsdcSetBusWidth0(busWidth);

}

void MsdcSetupCmd0(struct mmc_command *cmd, struct mmc_data *data)
{
    UINT32 u4sdcCmd = 0;
	UINT32 u4DataLen = 0;

    /* Figure out the response type */
    switch(cmd->opcode)
    {
    case MMC_GO_IDLE_STATE:
        u4sdcCmd |= SDC_CMD_RSPTYPE_NO;
        break;
    case MMC_SEND_OP_COND:
        u4sdcCmd |= SDC_CMD_RSPTYPE_R3;
        break;
    case MMC_ALL_SEND_CID:
    case MMC_SEND_CSD:
    case MMC_SEND_CID:
        u4sdcCmd |= SDC_CMD_RSPTYPE_R2;
        break;
    case MMC_SET_RELATIVE_ADDR:
        u4sdcCmd |= SDC_CMD_RSPTYPE_R1;
        break;
    case MMC_SWITCH:
        u4sdcCmd |= ((mmc_resp_type(cmd) == MMC_RSP_R1)?(SDC_CMD_RSPTYPE_R1):(SDC_CMD_RSPTYPE_R1B));
        if(data)
        {
            u4sdcCmd |= (DTYPE_SINGLE_BLK | SDC_CMD_READ);
            MSDC_WRITE32(SDC_BLK_NUM(ch0), 1);
        }
        break;
    case MMC_SELECT_CARD:
        u4sdcCmd |= SDC_CMD_RSPTYPE_R1;
        break;
    case SD_SWITCH_VOLTAGE:
        u4sdcCmd |= (SDC_CMD_RSPTYPE_R1 | SDC_CMD_VOL_SWITCH);
        break;
    case MMC_SEND_EXT_CSD:
        u4sdcCmd |= ((mmc_resp_type(cmd) == MMC_RSP_R1)?(SDC_CMD_RSPTYPE_R1):(SDC_CMD_RSPTYPE_R7));
        if(data)
        {
            u4sdcCmd |= (DTYPE_SINGLE_BLK | SDC_CMD_READ);
            MSDC_WRITE32(SDC_BLK_NUM(ch0), 1);
        }
        break;
    case MMC_STOP_TRANSMISSION:
        u4sdcCmd |= (SDC_CMD_STOP | SDC_CMD_RSPTYPE_R1);
        break;
    case MMC_SEND_STATUS:
        u4sdcCmd |= SDC_CMD_RSPTYPE_R1;
        if(data)
        {
            u4sdcCmd |= (DTYPE_SINGLE_BLK | SDC_CMD_READ);
            MSDC_WRITE32(SDC_BLK_NUM(ch0), 1);
        }
        break;
    case MMC_SET_BLOCKLEN:
        u4sdcCmd |= SDC_CMD_RSPTYPE_R1;
        break;
    case MMC_READ_SINGLE_BLOCK:
        u4sdcCmd |= (SDC_CMD_RSPTYPE_R1 | DTYPE_SINGLE_BLK | SDC_CMD_READ);
        MSDC_WRITE32(SDC_BLK_NUM(ch0), 1);
        break;
    case MMC_READ_MULTIPLE_BLOCK:
        u4sdcCmd |= (SDC_CMD_RSPTYPE_R1 | DTYPE_MULTI_BLK | SDC_CMD_READ);
        if(data)
        {
            MSDC_WRITE32(SDC_BLK_NUM(ch0), data->blocks);
        }
        else
        {
            MSDC_WRITE32(SDC_BLK_NUM(ch0), 0x0);
        }
        break;
    case MMC_WRITE_BLOCK:
        u4sdcCmd |= (SDC_CMD_RSPTYPE_R1 | DTYPE_SINGLE_BLK | SDC_CMD_WRITE);
        MSDC_WRITE32(SDC_BLK_NUM(ch0), 1);
        break;
    case MMC_WRITE_MULTIPLE_BLOCK:
        u4sdcCmd |= (SDC_CMD_RSPTYPE_R1 | DTYPE_MULTI_BLK | SDC_CMD_WRITE);
        if(data)
        {
            MSDC_WRITE32(SDC_BLK_NUM(ch0), data->blocks);
        }
        else
        {
            MSDC_WRITE32(SDC_BLK_NUM(ch0), 0x0);
        }
        break;
    case SD_APP_OP_COND:
        u4sdcCmd |= SDC_CMD_RSPTYPE_R3;
        break;
    case SD_APP_SEND_SCR:			
        u4sdcCmd |= (DTYPE_SINGLE_BLK | SDC_CMD_READ | SDC_CMD_RSPTYPE_R1);
        MSDC_WRITE32(SDC_BLK_NUM(ch0), 1);			
        break; 
    case MMC_APP_CMD:
        u4sdcCmd |= SDC_CMD_RSPTYPE_R1;
        break;
    }

    // Set Blk Length
    if(data)
    {
        u4DataLen = (data->blksz)*(data->blocks);
		u4DataLen = (u4DataLen >= SDHC_BLK_SIZE)?SDHC_BLK_SIZE:u4DataLen;
        u4sdcCmd |= ((u4DataLen) << SDC_CMD_LEN_SHIFT);
    }

    // Set SDC_CMD.CMD
    u4sdcCmd |= (cmd->opcode & 0x3F);

    if(data && data->stop)
    {
        u4sdcCmd |= SDC_CMD_AUTO_CMD12;
    }

    MSDC_LOG(MSG_LVL_INFO, "[0]CMD%d CMD:%08X, ARGU:%08X, BLKNUM:%08X\n", cmd->opcode, u4sdcCmd, cmd->arg, 
		                                                                 MSDC_READ32(SDC_BLK_NUM(ch0)));

    // Set SDC Argument
    MSDC_WRITE32(SDC_ARG(ch0), cmd->arg);

    /* Send the commands to the device */
    MSDC_WRITE32(SDC_CMD(ch0), u4sdcCmd);

}

void MsdcHandleResp0(struct mmc_command *cmd)
{
    // Handle the response
    switch (mmc_resp_type(cmd))
    {
    case MMC_RSP_NONE:
        MSDC_LOG(MSG_LVL_INFO, "[0]CMD%d ARG 0x%08X RESPONSE_NO\r\n", cmd->opcode, cmd->arg);
        break;
    case MMC_RSP_R1:
    case MMC_RSP_R1B:
        cmd->resp[0] = MSDC_READ32(SDC_RESP0(ch0));
        MSDC_LOG(MSG_LVL_INFO, "[0]CMD%d ARG 0x%08X RESPONSE_R1/R1B/R5/R6/R6 0x%08X\r\n", cmd->opcode, cmd->arg, cmd->resp[0]);
        break;
    case MMC_RSP_R2:
        cmd->resp[0] = MSDC_READ32(SDC_RESP3(ch0));
        cmd->resp[1] = MSDC_READ32(SDC_RESP2(ch0));
        cmd->resp[2] = MSDC_READ32(SDC_RESP1(ch0));
        cmd->resp[3] = MSDC_READ32(SDC_RESP0(ch0));
        MSDC_LOG(MSG_LVL_INFO, "[0]CMD%d ARG 0x%08X RESPONSE_R2 0x%08X 0x%08X 0x%08X 0x%08X\r\n", cmd->opcode, cmd->arg, cmd->resp[0], cmd->resp[1], cmd->resp[2], cmd->resp[3]);
        break;
    case MMC_RSP_R3:
        cmd->resp[0] = MSDC_READ32(SDC_RESP0(ch0));
        MSDC_LOG(MSG_LVL_INFO, "[0]CMD%d ARG 0x%08X RESPONSE_R3/R4 0x%08X\r\n", cmd->opcode, cmd->arg, cmd->resp[0]);
        break;
    }

	if(cmd->opcode == MMC_ALL_SEND_CID)
	{
        MsdcFindDev0(cmd->resp);
	}
}

int MsdcReqCmd0(struct mmc_command *cmd, struct mmc_data *data)
{
    int i4Ret = MSDC_SUCCESS;
    UINT32 u4CmdDoneVect;
    cmd->error = 0;
	
    // Check if rx/tx fifo is zero
    MsdcChkFifo0();
	
    // Clear interrupt Vector
    MsdcClrIntr0();
	
    MSDC_LOG(MSG_LVL_INFO, "[0]MsdcSendCmd : CMD%d ARG%08X!!\r\n", cmd->opcode, cmd->arg);
	
    if(MSDC_FAILED == MsdcWaitHostIdle0())
    {
        i4Ret = ERR_CMD_FAILED;
        cmd->error = ERR_HOST_BUSY;
        goto ErrorEnd;
    }
	
    MsdcSetupCmd0(cmd, data);
	
    // Wait for command and response if existed
    u4CmdDoneVect = INT_SD_CMDRDY | INT_SD_CMDTO | INT_SD_RESP_CRCERR;

    if (MSDC_SUCCESS != MsdcWaitIntr0(u4CmdDoneVect, CMD_TIMEOUT, 0))
    {
        MSDC_LOG(MSG_LVL_ERR, "[0]Failed to send CMD/RESP, DoneVect = 0x%x.\r\n", u4CmdDoneVect);
        i4Ret = ERR_CMD_FAILED;
        cmd->error = ERR_CMD_NO_INT;
        goto ErrorEnd;
    }
	
    //printf("Interrupt = %08X\n", msdc_host0->MsdcAccuVect);
	
    if (msdc_host0->MsdcAccuVect & INT_SD_CMDTO)
    {
        MSDC_LOG(MSG_LVL_ERR, "[0]CMD%d ARG 0x%08X - CMD Timeout (AccuVect 0x%08X INTR 0x%08X).\r\n", cmd->opcode, cmd->arg, msdc_host0->MsdcAccuVect, MSDC_READ32(MSDC_INT(ch0)));
        i4Ret = ERR_CMD_FAILED;
        cmd->error = ERR_CMD_RESP_TIMEOUT;
        goto ErrorEnd;
    }
    else if (msdc_host0->MsdcAccuVect & INT_SD_RESP_CRCERR)
    {
        MSDC_LOG(MSG_LVL_ERR, "[0]CMD%d ARG 0x%08X - CMD CRC Error (AccuVect 0x%08X INTR 0x%08X).\r\n", cmd->opcode, cmd->arg, msdc_host0->MsdcAccuVect, MSDC_READ32(MSDC_INT(ch0)));
        i4Ret = ERR_CMD_FAILED;
        cmd->error = ERR_CMD_RESP_CRCERR;
        goto ErrorEnd;
    }
    else if ((msdc_host0->MsdcAccuVect & (~(INT_SD_CMDRDY))) || (0 != MSDC_READ32(MSDC_INT(ch0))))
    {
        MSDC_LOG(MSG_LVL_INFO, "[0]CMD%d ARG 0x%08X - UnExpect status (AccuVect 0x%08X INTR 0x%08X).\r\n", cmd->opcode, cmd->arg, msdc_host0->MsdcAccuVect, MSDC_READ32(MSDC_INT(ch0)));
        //i4Ret = ERR_CMD_FAILED;
        //cmd->error = ERR_CMD_UNEXPECT_INT;
        //goto ErrorEnd;
    }
	
    // Handle the response
    MsdcHandleResp0(cmd);

ErrorEnd:
    return i4Ret;

}

int MsdcReqCmdTune0(struct msdc_host *host, struct mmc_command *cmd, struct mmc_data *data)
{
    int result = MSDC_SUCCESS;
    UINT32 rsmpl = 0, cur_rsmpl = 0, orig_rsmpl = 0;
    UINT32 rrdly = 0, cur_rrdly = 0, orig_rrdly = 0;
    UINT32 skip = 1;

    MSDC_LOG(MSG_LVL_ERR, "[0]----->Go into Command Tune!\n");
	
    MsdcReset0();
    MsdcClrFifo0();
    MsdcClrIntr0();

    orig_rsmpl = ((MSDC_READ32(MSDC_IOCON(ch0)) & MSDC_IOCON_R_SMPL) >> MSDC_IOCON_R_SMPL_SHIFT);
    orig_rrdly = ((MSDC_READ32(PAD_TUNE(ch0)) & PAD_CMD_RESP_RXDLY) >> PAD_CMD_RESP_RXDLY_SHIFT);

    rrdly = 0; 
    do 
    {
        for (rsmpl = 0; rsmpl < 2; rsmpl++) 
        {
            /* Lv1: R_SMPL[1] */		
            cur_rsmpl = (orig_rsmpl + rsmpl) % 2;		  
            if (skip == 1) 
            {
                skip = 0;	
                continue;	
            }

            MSDC_CLRBIT(MSDC_IOCON(ch0), MSDC_IOCON_R_SMPL);
            MSDC_SETBIT(MSDC_IOCON(ch0), (cur_rsmpl << MSDC_IOCON_R_SMPL_SHIFT));

            result = MsdcReqCmd0(cmd, data);
            if(result == MSDC_SUCCESS)
            {
                MSDC_LOG(MSG_LVL_ERR, "[0]TUNE_CMD<%s> OLD_RSMPL<%d> CUR_RSMPL<%d> OLD_RRDLY<%d> CUR_RRDLY<%d>\n", "PASS",
					                                    orig_rsmpl, cur_rsmpl, orig_rrdly, cur_rrdly);
				
                return MSDC_SUCCESS;
            }
            else
            {
                MsdcReset0();
                MsdcClrFifo0();
                MsdcClrIntr0();
            }
        }

#if 1
        /* Lv2: PAD_CMD_RESP_RXDLY[26:22] */              	
        cur_rrdly = (orig_rrdly + rrdly + 1) % 32;
        MSDC_CLRBIT(PAD_TUNE(ch0), PAD_CMD_RESP_RXDLY);
        MSDC_SETBIT(PAD_TUNE(ch0), (cur_rrdly << PAD_CMD_RESP_RXDLY_SHIFT));
    }while (++rrdly < 32);
#else
    }while (0);
#endif

    return result;
}

INT32 MsdcReqDataStop0(struct msdc_host *host)
{   
    INT32 i4Ret;
    struct mmc_command stop;
	
    memset(&stop, 0, sizeof(struct mmc_command));
    stop.opcode = MMC_STOP_TRANSMISSION;
    stop.arg = 0;
    stop.flags = MMC_RSP_R1B | MMC_CMD_AC;
	
    i4Ret = MsdcReqCmd0(&stop, NULL);
    MSDC_LOG(MSG_LVL_ERR, "[0]Stop Sending-Data State(%d)!\n", i4Ret);

    return i4Ret;    
}

INT32 MsdcReqCardStatus0(struct msdc_host *host, UINT32 *status)
{
    INT32 i4Ret;
    struct mmc_command query_status;
	
    memset(&query_status, 0, sizeof(struct mmc_command));
    query_status.opcode = MMC_SEND_STATUS;
    query_status.arg = (host->mmc->card->rca << 16);
    query_status.flags = MMC_RSP_R1 | MMC_CMD_AC;
	
    i4Ret = MsdcReqCmd0(&query_status, NULL);

    if(status)
    {
	    *status = query_status.resp[0];
    }

	return i4Ret;

}

INT32 MsdcCheckCardBusy(struct msdc_host *host)
{
    UINT32 err = 0, status = 0;

	do
	{
        err = MsdcReqCardStatus0(host, &status);
		if(err)
		{
		    MSDC_LOG(MSG_LVL_ERR, "[0]get card status failed(%d)\n", err);
            return err;
		}
         
		MSDC_LOG(MSG_LVL_ERR, "[0]get card status(%08X)\n", status);
	}while(R1_CURRENT_STATE(status) != 4);

	return err;
}

INT32 MsdcErrorHandling0(struct msdc_host *host, struct mmc_command *cmd, struct mmc_data *data)
{
    INT32 i4Ret = MSDC_SUCCESS, iTmpRet = 0;
	UINT32 status = 0;
    
    MSDC_LOG(MSG_LVL_ERR, "[0]Start Error Handling(CMD%d ARGU %08X,blksz %08X blknum %08X)...!\n", cmd->opcode, cmd->arg, 	                                                                              data->blksz, data->blocks);

    // Reset MSDC
    MsdcReset0();

    // Stop DMA
    if(host->data_mode > 1)
        MsdcStopDMA0();

    // Clear FIFO and wait it becomes 0
    i4Ret = MsdcClrFifo0();
    if(MSDC_SUCCESS != i4Ret)
    {
        goto ErrorEnd;	 
    }

    // Clear MSDC interrupts and make sure all are cleared
    MsdcClrIntr0();
    if  (0x00  != MSDC_READ32(MSDC_INT(ch0)))
    {
        i4Ret = MSDC_FAILED;
        goto ErrorEnd;	
    }

    // Send Stop Command for Multi-Write/Read
    if(data && ((cmd->opcode == MMC_READ_SINGLE_BLOCK) ||
		       (cmd->opcode == MMC_READ_MULTIPLE_BLOCK) || 
		       (cmd->opcode == MMC_WRITE_BLOCK) ||
		       (cmd->opcode == MMC_WRITE_MULTIPLE_BLOCK)))
    {
        iTmpRet = MsdcReqCardStatus0(host, &status); 
        if(iTmpRet)
        {
            MSDC_LOG(MSG_LVL_ERR, "read card status before stop command failed!\n");
        }
        else
        {
            MSDC_LOG(MSG_LVL_ERR, "read card status before stop command(%08X)!\n", status);
        }
    
        if(data->blocks > 1)
        {
            if (MsdcReqDataStop0(host)) 
            {
                MSDC_LOG(MSG_LVL_WARN, "[0]mmc fail to send stop cmd\n");
            }
        }

		// polling the card status to idle
		if(MsdcCheckCardBusy(host))
		{
            i4Ret = MSDC_FAILED;
			goto ErrorEnd;
		}		
    }

    MSDC_LOG(MSG_LVL_ERR, "[0]End Error Handling...!\n");

ErrorEnd:
    return i4Ret;

}

static inline void msdc0_init_sg(struct msdc_host *host, struct mmc_data *data);
static inline int msdc0_next_sg(struct msdc_host *host);
static inline char *msdc0_sg_to_buffer(struct msdc_host *host);
INT32 MsdcReqDataPIO0(struct msdc_host *host, struct mmc_data *data)
{
    int i4Ret = MSDC_SUCCESS;
    UINT32 fgAutoCmd12, IntrWaitVect = 0, IntrCheckVect = 0;
    UINT32 i, u4Tmp;
	
    UINT32 u4RxFifoCnt, u4TxFifoCnt;
    UINT32 u4BufLen = (UINT32)((data->blocks)*(data->blksz));
    UINT8 *buffer;
    UINT32 fifo = 4;
    UINT8 fifo_buf[4] = {0x0};
    UINT32 u4RxCnt = 0;
	
    buffer = msdc0_sg_to_buffer(host) + host->offset;
	
    if(buffer == NULL)
    {
        i4Ret = ERR_DAT_FAILED;
        MSDC_LOG(MSG_LVL_INFO, "[0]End Error Handling...! buffer NULL --- 1 ---\n");
        goto ErrorEnd;
    }
    fgAutoCmd12 = (data->stop)?1:0;

    if(fgAutoCmd12)
    {		 
        IntrWaitVect = INT_SD_XFER_COMPLETE | INT_SD_AUTOCMD_RDY; 	   
        IntrCheckVect = INT_SD_XFER_COMPLETE | INT_SD_AUTOCMD_RDY;	 
    }	  
    else	  
    { 	   
        IntrWaitVect = INT_SD_XFER_COMPLETE; 	   
        IntrCheckVect = INT_SD_XFER_COMPLETE;	
	}
	
    // Read
    if (data->flags & MMC_DATA_READ)
    {
        while (u4BufLen)
        {
            // wait until fifo has enough data
            u4RxFifoCnt = (MSDC_READ32(MSDC_FIFOCS(ch0)) & MSDC_FIFOCS_RXFIFOCNT_MASK);
	
            while ((u4BufLen) && (sizeof(int) <= u4RxFifoCnt))
            {
                // Read Data
                *((UINT32 *)fifo_buf) = MSDC_READ32(MSDC_RXDATA(ch0));
	
                u4RxFifoCnt -= sizeof(int);
                u4RxCnt += sizeof(int);
	
                for (i = 0; i < fifo; i++)
                {
                    *buffer = fifo_buf[i];
                    buffer++;
                    host->offset++;
                    host->remain--;
	
                    data->bytes_xfered++;
	
                    /*
                                * End of scatter list entry?
                                */
                    if (host->remain == 0)
                    {
                        /*
                                      * Get next entry. Check if last.
                                      */
                        if (!msdc0_next_sg(host))
                        {
                            break;
                        }
	
                        buffer = msdc0_sg_to_buffer(host);
                    }
                }
	
                if(u4RxCnt == SDHC_BLK_SIZE)
                {
                    // Check CRC error happens by every 512 Byte
                    // Check if done vector occurs
                    u4Tmp = MSDC_READ32(MSDC_INT(ch0));
                    if(INT_SD_DATTO & u4Tmp)
                        data->error = ERR_DAT_TIMEOUT;
                    else if(INT_SD_DATA_CRCERR & u4Tmp)
                        data->error = ERR_DAT_CRCERR;
							 
                    if(data->error)
                    {
                        i4Ret = ERR_DAT_FAILED;
                        MSDC_LOG(MSG_LVL_ERR, "[0]Read Error Break !!\r\n");
                        goto ErrorEnd;
                    }
                    else
                    {
                        u4RxCnt = 0;
                        u4BufLen -= SDHC_BLK_SIZE;
                    }
                }
	
            }
        }
    }
    else
    {
        while (u4BufLen)
        {
            // Check if error done vector occurs
            u4Tmp = MSDC_READ32(MSDC_INT(ch0));
            if(INT_SD_DATTO & u4Tmp)
                data->error = ERR_DAT_TIMEOUT;
            else if(INT_SD_DATA_CRCERR & u4Tmp)
                data->error = ERR_DAT_CRCERR;
							 
            if(data->error)
            {
                i4Ret = ERR_DAT_FAILED;
                MSDC_LOG(MSG_LVL_ERR, "[0]Write Error Break !!\r\n");
                goto ErrorEnd;
            }
	
            // wait until fifo has enough space
            while(1)
            {
                if((MSDC_READ32(MSDC_FIFOCS(ch0)) & MSDC_FIFOCS_TXFIFOCNT_MASK) == 0)
                {
                    break;
                }
            }
	
            u4TxFifoCnt = MSDC_FIFO_LEN;
	
            if (sizeof(int) <= u4TxFifoCnt)
            {
                while ((u4BufLen) && (sizeof(int) <= u4TxFifoCnt))
                {
                    // gather byte data into fifo_buf
                    for (i = 0; i < fifo; i++)
                    {
                        fifo_buf[i] = *buffer;
                        buffer++;
                        host->offset++;
                        host->remain--;
	
                        data->bytes_xfered++;
	
                        /*
                                      * End of scatter list entry?
                                      */
                        if (host->remain == 0)
                        {
                            /*
                                            * Get next entry. Check if last.						  
                                            */
                            if (!msdc0_next_sg(host))
                            {
                                break;
                            }
	
                            buffer = msdc0_sg_to_buffer(host);
                        }
                    }
	
                    // Write Data
                    MSDC_WRITE32(MSDC_TXDATA(ch0), *((UINT32 *)(fifo_buf)));
                    u4TxFifoCnt -= sizeof(int);
                    u4BufLen -= sizeof(int);
                }
            }
        }
    }
	
    // Wait for data complete
    if (MSDC_SUCCESS != MsdcWaitIntr0(IntrWaitVect, DAT_TIMEOUT, 1))
    {
        MSDC_LOG(MSG_LVL_ERR, "[0]Wait Intr timeout (AccuVect 0x%08X INTR 0x%08X).\r\n", msdc_host0->MsdcAccuVect, MSDC_READ32(MSDC_INT(ch0)));
        i4Ret = ERR_DAT_FAILED;
        data->error = ERR_DAT_NO_INT;
        goto ErrorEnd;
    }
	
    if ((msdc_host0->MsdcAccuVect & ~(IntrCheckVect)) || (0 != MSDC_READ32(MSDC_INT(ch0))))
    {
        MSDC_LOG(MSG_LVL_ERR, "[0]UnExpect status (AccuVect 0x%08X INTR 0x%08X).\r\n", msdc_host0->MsdcAccuVect, MSDC_READ32(MSDC_INT(ch0)));
        i4Ret = ERR_DAT_FAILED;
        data->error = ERR_DAT_UNEXPECT_INT;
        goto ErrorEnd;
    }

ErrorEnd:
    return i4Ret;

}

INT32 MsdcReqDataBasicDMA0(struct msdc_host *host, struct mmc_data *data)
{
    INT32 i4Ret = MSDC_SUCCESS;
    UINT32 u4BufLen = (UINT32)((data->blocks)*(data->blksz)), u4ActuralLen, u4PhyAddr;
    UINT8 *buffer;	
    UINT32 fgAutoCmd12, IntrWaitVect = 0, IntrCheckVect = 0;

    fgAutoCmd12 = (data->stop)?1:0;

    if(fgAutoCmd12)
    {		 
        IntrWaitVect = INT_SD_XFER_COMPLETE | INT_DMA_XFER_DONE | INT_SD_AUTOCMD_RDY; 	   
        IntrCheckVect = INT_SD_XFER_COMPLETE | INT_DMA_XFER_DONE | INT_SD_AUTOCMD_RDY;	 
    }	  
    else	  
    { 	   
        IntrWaitVect = INT_SD_XFER_COMPLETE | INT_DMA_XFER_DONE; 	   
        IntrCheckVect = INT_SD_XFER_COMPLETE | INT_DMA_XFER_DONE;	
    }
	
    while(u4BufLen)
    {
        /*
            * End of scatter list entry?
            */
        if (host->remain == 0)
        {
            /*
             * Get next entry. Check if last.
             */
            if (!msdc0_next_sg(host))
            {
                break;
            }
        }
	
        buffer = msdc0_sg_to_buffer(host) + host->offset;
        if(buffer == NULL)
       	{
            i4Ret = ERR_DAT_FAILED;
            MSDC_LOG(MSG_LVL_INFO, "[0]End Error Handling...! buffer NULL --- 4 ---\n");
            goto ErrorEnd;
       	}
	
        u4PhyAddr = dma_map_single(NULL, buffer, host->remain, (data->flags & MMC_DATA_READ)?(DMA_FROM_DEVICE):(DMA_TO_DEVICE));
	
        if(u4PhyAddr >= mt53xx_cha_mem_size)
            printk("[0]!!!!MTK MSDC: DMA to ChB %p\n", buffer);
        MSDC_WRITE32(DMA_SA(ch0), u4PhyAddr);
	
        if((host->num_sg == 1) && ((host->remain) <= BASIC_DMA_MAX_LEN))
        {
            u4ActuralLen = host->remain;
            MSDC_LOG(MSG_LVL_INFO, "[0](Last)AccLen = %08X\r\n", u4ActuralLen);
#if defined(CC_MT5399) || defined(CONFIG_ARCH_MT5399) || defined(CC_MT5890) || defined(CONFIG_ARCH_MT5890)
            MSDC_WRITE32(DMA_LENGTH(ch0), (u4ActuralLen));
            MSDC_WRITE32(DMA_CTRL(ch0), (DMA_BST_64 << DMA_CTRL_BST_SHIFT) | DMA_CTRL_LAST_BUF | DMA_CTRL_START);
#else
            MSDC_WRITE32(DMA_CTRL(ch0), ((u4ActuralLen) << DMA_CTRL_XFER_SIZE_SHIFT) | (DMA_BST_64 << DMA_CTRL_BST_SHIFT) | DMA_CTRL_LAST_BUF | DMA_CTRL_START);
#endif

            // Wait for sd xfer complete
            if (MSDC_SUCCESS != MsdcWaitIntr0(IntrWaitVect, DAT_TIMEOUT, 1)) 
            {
                MSDC_LOG(MSG_LVL_ERR, "[0](L)%s: Failed to send/receive data (AccuVect 0x%08X INTR 0x%08X). %s line %d\r\n", __FUNCTION__, host->MsdcAccuVect, MSDC_READ32(MSDC_INT(ch0)), __FILE__, __LINE__);
                i4Ret = ERR_DAT_FAILED;
                data->error = ERR_DAT_NO_INT;
                goto ErrorEnd;
            }
	
            if ((host->MsdcAccuVect & ~(IntrCheckVect)) || (0 != MSDC_READ32(MSDC_INT(ch0)))) 
            {
                MSDC_LOG(MSG_LVL_ERR, "[0]%s: Unexpected status (AccuVect 0x%08X INTR 0x%08X). %s line %d\r\n", __FUNCTION__, host->MsdcAccuVect, MSDC_READ32(MSDC_INT(ch0)), __FILE__, __LINE__);
                i4Ret = ERR_DAT_FAILED;
                data->error = ERR_DAT_UNEXPECT_INT;
                goto ErrorEnd;
            }
	
            // Check DMA status
            if (0 != (MSDC_READ32(DMA_CFG(ch0)) & (DMA_CFG_DMA_STATUS))) 
            {
                MSDC_LOG(MSG_LVL_ERR, "[0]%s: Incorrect DMA status. DMA_CFG: 0x%08X\r\n", __FUNCTION__, MSDC_READ32(DMA_CFG(ch0)));
                i4Ret = ERR_DAT_FAILED;
                data->error = ERR_DMA_STATUS_FAILED;
                goto ErrorEnd;
            }
        }
        else
        {
            u4ActuralLen = ((host->remain) <= BASIC_DMA_MAX_LEN)?(host->remain):(BASIC_DMA_MAX_LEN);
            MSDC_LOG(MSG_LVL_INFO, "[0](NOT LAST)AccLen = %08X\r\n", u4ActuralLen);
#if defined(CC_MT5399) || defined(CONFIG_ARCH_MT5399) || defined(CC_MT5890) || defined(CONFIG_ARCH_MT5890)
            MSDC_WRITE32(DMA_LENGTH(ch0), (u4ActuralLen));
            MSDC_WRITE32(DMA_CTRL(ch0), (DMA_BST_64 << DMA_CTRL_BST_SHIFT) | DMA_CTRL_START);
#else
            MSDC_WRITE32(DMA_CTRL(ch0), (u4ActuralLen << DMA_CTRL_XFER_SIZE_SHIFT) | (DMA_BST_64 << DMA_CTRL_BST_SHIFT) | DMA_CTRL_START);
#endif	
            if (MSDC_SUCCESS != MsdcWaitIntr0(INT_DMA_XFER_DONE, MSDC_WAIT_DATA_COMPLETE_TIMEOUT_LIMIT_COUNT, 1)) 
            {
                MSDC_LOG(MSG_LVL_ERR, "[0](N)%s: Failed to send/receive data (AccuVect 0x%08X INTR 0x%08X). %s line %d\r\n", __FUNCTION__, host->MsdcAccuVect, MSDC_READ32(MSDC_INT(ch0)), __FILE__, __LINE__);
                i4Ret = ERR_DAT_FAILED;
                data->error = ERR_DAT_NO_INT;
                goto ErrorEnd;
            }
	
            if ((host->MsdcAccuVect & ~(INT_DMA_XFER_DONE)) || (0 != MSDC_READ32(MSDC_INT(ch0)))) 
            {
                MSDC_LOG(MSG_LVL_ERR, "[0]%s: Unexpected status (AccuVect 0x%08X INTR 0x%08X). %s line %d\r\n", __FUNCTION__, host->MsdcAccuVect, MSDC_READ32(MSDC_INT(ch0)), __FILE__, __LINE__);
                i4Ret = ERR_DAT_FAILED;
                data->error = ERR_DAT_UNEXPECT_INT;
                goto ErrorEnd;
            }
        }
	
        dma_unmap_single(NULL, u4PhyAddr, host->remain, (data->flags & MMC_DATA_READ)?(DMA_FROM_DEVICE):(DMA_TO_DEVICE));
	
        u4BufLen -= u4ActuralLen;
        host->offset += u4ActuralLen;
        host->remain -= u4ActuralLen;
        data->bytes_xfered += u4ActuralLen;	
    }

ErrorEnd:
    return i4Ret;

}

INT32 MsdcReqDataDescriptorDMA0(struct msdc_host *host, struct mmc_data *data)
{
    INT32 i4Ret = MSDC_SUCCESS;
    UINT32 u4BufLen = (UINT32)((data->blocks)*(data->blksz));
    UINT32 fgAutoCmd12, IntrWaitVect = 0, IntrCheckVect = 0;

    fgAutoCmd12 = (data->stop)?1:0;

    if(fgAutoCmd12)
    {		 
        IntrWaitVect = INT_SD_XFER_COMPLETE | INT_DMA_XFER_DONE | INT_SD_AUTOCMD_RDY | INT_DMA_Q_EMPTY; 	   
        IntrCheckVect = INT_SD_XFER_COMPLETE | INT_DMA_XFER_DONE | INT_SD_AUTOCMD_RDY | INT_DMA_Q_EMPTY; 
    }	  
    else	  
    { 	   
        IntrWaitVect = INT_SD_XFER_COMPLETE | INT_DMA_XFER_DONE | INT_DMA_Q_EMPTY; 	   
        IntrCheckVect = INT_SD_XFER_COMPLETE | INT_DMA_XFER_DONE | INT_DMA_Q_EMPTY;
    }
    
    MsdcDescriptorConfig0(host, data);
    MSDC_SETBIT(DMA_CTRL(ch0), DMA_CTRL_START);
	
    // Wait for sd xfer complete
    if (MSDC_SUCCESS != MsdcWaitIntr0(IntrWaitVect, DAT_TIMEOUT, 1)) 
    {
        MSDC_LOG(MSG_LVL_ERR, "[0]%s: Failed to send/receive data (AccuVect 0x%08X INTR 0x%08X). %s line %d\r\n", __FUNCTION__, host->MsdcAccuVect, MSDC_READ32(MSDC_INT(ch0)), __FILE__, __LINE__);
        i4Ret = ERR_DAT_FAILED;
        data->error = ERR_DAT_NO_INT;
        goto ErrorEnd;
    }
	
    if ((host->MsdcAccuVect & ~(IntrCheckVect)) || (0 != MSDC_READ32(MSDC_INT(ch0)))) 
    {
        MSDC_LOG(MSG_LVL_INFO, "[0]%s: Unexpected status (AccuVect 0x%08X INTR 0x%08X). %s line %d\r\n", __FUNCTION__, host->MsdcAccuVect, MSDC_READ32(MSDC_INT(ch0)), __FILE__, __LINE__);
        i4Ret = ERR_DAT_FAILED;
        data->error = ERR_DAT_UNEXPECT_INT;
        goto ErrorEnd;
    }
	
    if(MSDC_READ32(DMA_CFG(ch0)) & (DMA_CFG_GPD_CS_ERR))
    {
        MSDC_LOG(MSG_LVL_ERR, "[0]Descriptor DMA GPD checksum error");
        i4Ret = ERR_DAT_FAILED;
        data->error = ERR_DMA_STATUS_FAILED;
        goto ErrorEnd;
    }
	
    if(MSDC_READ32(DMA_CFG(ch0)) & (DMA_CFG_BD_CS_ERR))
    {
        MSDC_LOG(MSG_LVL_ERR, "[0]Descriptor DMA BD checksum error");
        i4Ret = ERR_DAT_FAILED;
        data->error = ERR_DMA_STATUS_FAILED;
        goto ErrorEnd;
    }
	
    // Check DMA status
    if (0 != (MSDC_READ32(DMA_CFG(ch0)) & (DMA_CFG_DMA_STATUS))) 
    {
        MSDC_LOG(MSG_LVL_ERR, "[0]%s: Incorrect DMA status. DMA_CFG: 0x%08X\r\n", __FUNCTION__, MSDC_READ32(DMA_CFG(ch0)));
        i4Ret = ERR_DAT_FAILED;
        data->error = ERR_DMA_STATUS_FAILED;
        goto ErrorEnd;
    } 
            
    MsdcDescriptorFlush0(host, data);       	
    data->bytes_xfered += u4BufLen;   
    
ErrorEnd:
    return i4Ret;
    	
}

INT32 MsdcReqData0(struct msdc_host *host, struct mmc_command *cmd, struct mmc_data *data)
{
    int i4Ret = MSDC_SUCCESS;
    data->error = 0;

    // Initialize sg for data transfer
    msdc0_init_sg(host, data);

    /*
      * Handle excessive data.
      */
    if (host->num_sg == 0)
        return MSDC_SUCCESS;

    if(PIO_DATA_MODE == host->data_mode)
    {
        i4Ret = MsdcReqDataPIO0(host, data);
        if(MSDC_SUCCESS != i4Ret)
        {
            goto ErrorEnd;
        }			
    }
    else if(BASIC_DMA_DATA_MODE == host->data_mode)
    {
        i4Ret = MsdcReqDataBasicDMA0(host, data);
        if(MSDC_SUCCESS != i4Ret)
        {
            goto ErrorEnd;
        }
    }
    else if(DESC_DMA_DATA_MODE == host->data_mode)
    {
        i4Ret = MsdcReqDataDescriptorDMA0(host, data);
        if(MSDC_SUCCESS != i4Ret)
        {
            goto ErrorEnd;
        }    	
    }

ErrorEnd:
	return i4Ret;
	
}

int MsdcSendCmd0(struct msdc_host *host, struct mmc_command *cmd, struct mmc_data *data)
{
    int i4Ret = MSDC_SUCCESS;

    i4Ret = MsdcReqCmd0(cmd, data);
    if(MSDC_SUCCESS != i4Ret)
    {
        if(data)
        {
            if(MSDC_SUCCESS != MsdcErrorHandling0(host, cmd, data))
            {
                goto ErrorEnd;
            }
        }
		
        i4Ret = MsdcReqCmdTune0(host, cmd, data);
        if(MSDC_SUCCESS != i4Ret)
        {
            goto ErrorEnd;
        }
    }
	
    if(data)
    {
        i4Ret = MsdcReqData0(host, cmd, data);
        if(MSDC_SUCCESS != i4Ret)
        {
            if(MSDC_SUCCESS != MsdcErrorHandling0(host, cmd, data))
            {
                goto ErrorEnd;
            }
        }
    }

ErrorEnd:
    return i4Ret;
    
}

int MsdcReqDataReadTune0(struct msdc_host *host, struct mmc_command *cmd, struct mmc_data *data)
{
    UINT32 ddr=0;	
    UINT32 dcrc = 0;
    UINT32 rxdly, cur_rxdly0, cur_rxdly1;
    //UINT32 rxdly, cur_rxdly;
    UINT32 dsmpl, cur_dsmpl,  orig_dsmpl;
    UINT32 cur_dat0,  cur_dat1,  cur_dat2,  cur_dat3;
    UINT32 cur_dat4,  cur_dat5,  cur_dat6,  cur_dat7;
    UINT32 orig_dat0, orig_dat1, orig_dat2, orig_dat3;
    UINT32 orig_dat4, orig_dat5, orig_dat6, orig_dat7;
#if defined(CC_MT5399) || defined(CONFIG_ARCH_MT5399)|| \
    defined(CC_MT5890) || defined(CONFIG_ARCH_MT5890)
    UINT32 dsel, cur_dsel = 0, orig_dsel;
    //UINT32 dl_cksel, cur_dl_cksel = 0, orig_dl_cksel;
#endif
    //UINT32 cur_dat, orig_dat;
    INT32 result = -1;
    UINT32 skip = 1;

    MSDC_LOG(MSG_LVL_ERR, "[0]Go into Data Read Tune(%08X, %08X)!\n", MSDC_READ32(DAT_RD_DLY0(ch0)), MSDC_READ32(DAT_RD_DLY1(ch0)));

    orig_dsmpl = ((MSDC_READ32(MSDC_IOCON(ch0)) & MSDC_IOCON_D_SMPL) >> MSDC_IOCON_D_SMPL_SHIFT);

    /* Tune Method 2. */
    MSDC_CLRBIT(MSDC_IOCON(ch0), MSDC_IOCON_D_DLYLINE_SEL);
    MSDC_SETBIT(MSDC_IOCON(ch0), (1 << MSDC_IOCON_D_DLYLINE_SEL_SHIFT));

    MSDC_LOG(MSG_LVL_ERR, "[0]CRC(R) Error Register: %08X!\n", MSDC_READ32(SDC_DATCRC_STS(ch0)));

#if defined(CC_MT5399) || defined(CONFIG_ARCH_MT5399)|| \
    defined(CC_MT5890) || defined(CONFIG_ARCH_MT5890)
    orig_dsel = ((MSDC_READ32(PATCH_BIT0(ch0)) & CKGEN_MSDC_DLY_SEL) >> CKGEN_MSDC_DLY_SEL_SHIFT);
    cur_dsel = orig_dsel;

    dsel = 0;
    do
    {
#endif
    rxdly = 0; 
    do 
    {
        for (dsmpl = 0; dsmpl < 2; dsmpl++) 
        {
            cur_dsmpl = (orig_dsmpl + dsmpl) % 2;
            if (skip == 1) 
            {
                skip = 0; 	
                continue;	
            } 

            MSDC_CLRBIT(MSDC_IOCON(ch0), MSDC_IOCON_D_SMPL);
            MSDC_SETBIT(MSDC_IOCON(ch0), (cur_dsmpl << MSDC_IOCON_D_SMPL_SHIFT));

            result = MsdcSendCmd0(host, cmd, data);

            dcrc = MSDC_READ32(SDC_DATCRC_STS(ch0));
            if(!ddr)
                dcrc &= (~SDC_DATCRC_STS_NEG);
			
#if defined(CC_MT5399) || defined(CONFIG_ARCH_MT5399)|| \
    defined(CC_MT5890) || defined(CONFIG_ARCH_MT5890)
                MSDC_LOG(MSG_LVL_ERR, "[1]TUNE_READ<%s> dcrc<0x%x> DATRDDLY0/1<0x%x><0x%x> dsmpl<0x%x> CKGEN_MSDC_DLY_SEL<0x%x> cmd_error<%d> data_error<%d>",
                            (result == MSDC_SUCCESS && dcrc == 0) ? "PASS" : "FAIL", dcrc,
                            MSDC_READ32(DAT_RD_DLY0(ch0)), MSDC_READ32(DAT_RD_DLY1(ch0)), cur_dsmpl, cur_dsel,
                            cmd->error, data->error);
#else
            MSDC_LOG(MSG_LVL_ERR, "[0]TUNE_READ<%s> dcrc<0x%x> DATRDDLY0/1<0x%x><0x%x> dsmpl<0x%x> cmd_error<%d> data_error<%d>",
                        (result == MSDC_SUCCESS && dcrc == 0) ? "PASS" : "FAIL", dcrc,
                        MSDC_READ32(DAT_RD_DLY0(ch0)), MSDC_READ32(DAT_RD_DLY1(ch0)), cur_dsmpl,
                        cmd->error, data->error);
#endif
			
            if((result == MSDC_SUCCESS) && dcrc == 0)
            {
                cmd->error = 0;
                data->error = 0;
				if(data->stop)
                    data->stop->error = 0;
				
                goto done;
            }
            else
            {
                // Tuning Data error but Command error happens, directly return;
                if((result != MSDC_SUCCESS) && (result != ERR_DAT_FAILED))
                {
                    MSDC_LOG(MSG_LVL_ERR, "[0]TUNE_READ(1): result<0x%x> cmd_error<%d> data_error<%d>", 
						             result, cmd->error, data->error);	
					
                    goto done;  
                }
                else if((result != MSDC_SUCCESS) && (result == ERR_DAT_FAILED))
                {
                    MSDC_LOG(MSG_LVL_ERR, "[0]TUNE_READ(2): result<0x%x> cmd_error<%d> data_error<%d>", 
						             result, cmd->error, data->error);	
                }
            }
        }

        cur_rxdly0 = MSDC_READ32(DAT_RD_DLY0(ch0));
        cur_rxdly1 = MSDC_READ32(DAT_RD_DLY1(ch0));

        /* E1 ECO. YD: Reverse */
        if (MSDC_READ32(ECO_VER(ch0)) >= 4) 
        {
            orig_dat0 = (cur_rxdly0 >> 24) & 0x1F;
            orig_dat1 = (cur_rxdly0 >> 16) & 0x1F;
            orig_dat2 = (cur_rxdly0 >>	8) & 0x1F;
            orig_dat3 = (cur_rxdly0 >>	0) & 0x1F;
            orig_dat4 = (cur_rxdly1 >> 24) & 0x1F;
            orig_dat5 = (cur_rxdly1 >> 16) & 0x1F;
            orig_dat6 = (cur_rxdly1 >>	8) & 0x1F;
            orig_dat7 = (cur_rxdly1 >>	0) & 0x1F;
        } 
        else 
        {   
            orig_dat0 = (cur_rxdly0 >>	0) & 0x1F;
            orig_dat1 = (cur_rxdly0 >>	8) & 0x1F;
            orig_dat2 = (cur_rxdly0 >> 16) & 0x1F;
            orig_dat3 = (cur_rxdly0 >> 24) & 0x1F;
            orig_dat4 = (cur_rxdly1 >>	0) & 0x1F;
            orig_dat5 = (cur_rxdly1 >>	8) & 0x1F;
            orig_dat6 = (cur_rxdly1 >> 16) & 0x1F;
            orig_dat7 = (cur_rxdly1 >> 24) & 0x1F;
        }

        if(ddr) 
        {
            cur_dat0 = (dcrc & (1 << 0) || dcrc & (1 << 8))  ? ((orig_dat0 + 1) % 32) : orig_dat0;
            cur_dat1 = (dcrc & (1 << 1) || dcrc & (1 << 9))  ? ((orig_dat1 + 1) % 32) : orig_dat1;
            cur_dat2 = (dcrc & (1 << 2) || dcrc & (1 << 10)) ? ((orig_dat2 + 1) % 32) : orig_dat2;
            cur_dat3 = (dcrc & (1 << 3) || dcrc & (1 << 11)) ? ((orig_dat3 + 1) % 32) : orig_dat3;
        } 
        else 
        {
            cur_dat0 = (dcrc & (1 << 0)) ? ((orig_dat0 + 1) % 32) : orig_dat0;
            cur_dat1 = (dcrc & (1 << 1)) ? ((orig_dat1 + 1) % 32) : orig_dat1;
            cur_dat2 = (dcrc & (1 << 2)) ? ((orig_dat2 + 1) % 32) : orig_dat2;
            cur_dat3 = (dcrc & (1 << 3)) ? ((orig_dat3 + 1) % 32) : orig_dat3;
        }
        cur_dat4 = (dcrc & (1 << 4)) ? ((orig_dat4 + 1) % 32) : orig_dat4;
        cur_dat5 = (dcrc & (1 << 5)) ? ((orig_dat5 + 1) % 32) : orig_dat5;
        cur_dat6 = (dcrc & (1 << 6)) ? ((orig_dat6 + 1) % 32) : orig_dat6;
        cur_dat7 = (dcrc & (1 << 7)) ? ((orig_dat7 + 1) % 32) : orig_dat7;

        /* E1 ECO. YD: Reverse */
        if (MSDC_READ32(ECO_VER(ch0)) >= 4) 
        {
            cur_rxdly0 = (cur_dat0 << 24) | (cur_dat1 << 16) | (cur_dat2 << 8) | (cur_dat3 << 0);
            cur_rxdly1 = (cur_dat4 << 24) | (cur_dat5 << 16) | (cur_dat6 << 8) | (cur_dat7 << 0);
        }
        else
        {
            cur_rxdly0 = (cur_dat3 << 24) | (cur_dat2 << 16) | (cur_dat1 << 8) | (cur_dat0 << 0);
            cur_rxdly1 = (cur_dat7 << 24) | (cur_dat6 << 16) | (cur_dat5 << 8) | (cur_dat4 << 0);   
        }

        MSDC_WRITE32(DAT_RD_DLY0(ch0), cur_rxdly0);
        MSDC_WRITE32(DAT_RD_DLY1(ch0), cur_rxdly1);   
    }while(++rxdly < 32);
#if defined(CC_MT5399) || defined(CONFIG_ARCH_MT5399)|| \
    defined(CC_MT5890) || defined(CONFIG_ARCH_MT5890)
        cur_dsel = (orig_dsel + dsel + 1) % 32;
        MSDC_CLRBIT(PATCH_BIT0(ch0), CKGEN_MSDC_DLY_SEL);
        MSDC_SETBIT(PATCH_BIT0(ch0), (cur_dsel << CKGEN_MSDC_DLY_SEL_SHIFT));
    } while (++dsel < 32);
#endif


done:
		
    return result;
}

int MsdcReqDataWriteTune0(struct msdc_host *host, struct mmc_command *cmd, struct mmc_data *data)
{
    UINT32 wrrdly, cur_wrrdly = 0, orig_wrrdly;
    UINT32 dsmpl,  cur_dsmpl,  orig_dsmpl;
    UINT32 rxdly,  cur_rxdly0;
    UINT32 cur_dat0,  cur_dat1,  cur_dat2,  cur_dat3;
    UINT32 orig_dat0, orig_dat1, orig_dat2, orig_dat3;
#if defined(CC_MT5399) || defined(CONFIG_ARCH_MT5399)|| \
    defined(CC_MT5890) || defined(CONFIG_ARCH_MT5890)
    UINT32 d_cntr, cur_d_cntr = 0, orig_d_cntr;
#endif
    int result = -1;
    UINT32 skip = 1;

    MSDC_LOG(MSG_LVL_ERR, "[0]----->Go into Data Write Tune!\n");

    orig_wrrdly = ((MSDC_READ32(PAD_TUNE(ch0)) & PAD_DAT_WR_RXDLY) >> PAD_DAT_WR_RXDLY_SHIFT);
    cur_wrrdly = orig_wrrdly;
#if defined(CC_MT5399) || defined(CONFIG_ARCH_MT5399)|| \
    defined(CC_MT5890) || defined(CONFIG_ARCH_MT5890)
    orig_dsmpl = ((MSDC_READ32(MSDC_IOCON(ch0)) & MSDC_IOCON_W_D_SMPL_SEL) >> MSDC_IOCON_W_D_SMPL_SHIFT);
#else
    orig_dsmpl = ((MSDC_READ32(MSDC_IOCON(ch0)) & MSDC_IOCON_D_SMPL) >> MSDC_IOCON_D_SMPL_SHIFT);
#endif

    MSDC_LOG(MSG_LVL_ERR, "[0]CRC(W) Error Register: %08X!\n", MSDC_READ32(SDC_DATCRC_STS(ch0)));

    /* Tune Method 2. just DAT0 */  
    MSDC_CLRBIT(MSDC_IOCON(ch0), MSDC_IOCON_D_DLYLINE_SEL);
    MSDC_SETBIT(MSDC_IOCON(ch0), (1 << MSDC_IOCON_D_DLYLINE_SEL_SHIFT));
    cur_rxdly0 = MSDC_READ32(DAT_RD_DLY0(ch0));

    /* E1 ECO. YD: Reverse */
    if (MSDC_READ32(ECO_VER(ch0)) >= 4) 
    {
        orig_dat0 = (cur_rxdly0 >> 24) & 0x1F;
        orig_dat1 = (cur_rxdly0 >> 16) & 0x1F;
        orig_dat2 = (cur_rxdly0 >>	8) & 0x1F;
        orig_dat3 = (cur_rxdly0 >>	0) & 0x1F;
    } 
    else 
    {
        orig_dat0 = (cur_rxdly0 >>	0) & 0x1F;
        orig_dat1 = (cur_rxdly0 >>	8) & 0x1F;
        orig_dat2 = (cur_rxdly0 >> 16) & 0x1F;
        orig_dat3 = (cur_rxdly0 >> 24) & 0x1F;
    }
#if defined(CC_MT5399) || defined(CONFIG_ARCH_MT5399)|| \
    defined(CC_MT5890) || defined(CONFIG_ARCH_MT5890)
    orig_d_cntr = ((MSDC_READ32(PATCH_BIT1(ch0)) & WRDAT_CRCS_TA_CNTR) >> WRDAT_CRCS_TA_CNTR_SHIFT);
    cur_d_cntr = orig_d_cntr;

    d_cntr = 0;
    do
    {
#endif
    rxdly = 0;
    do 
    {
        wrrdly = 0;
        do 
        {    
            for (dsmpl = 0; dsmpl < 2; dsmpl++) 
            {
                cur_dsmpl = (orig_dsmpl + dsmpl) % 2;
                if (skip == 1) 
                {
                    skip = 0;
                    continue; 	
                }

#if defined(CC_MT5399) || defined(CONFIG_ARCH_MT5399)|| \
    defined(CC_MT5890) || defined(CONFIG_ARCH_MT5890)
                    MSDC_CLRBIT(MSDC_IOCON(ch0), MSDC_IOCON_W_D_SMPL_SEL);
                    MSDC_SETBIT(MSDC_IOCON(ch0), (cur_dsmpl << MSDC_IOCON_W_D_SMPL_SHIFT));
#else
                MSDC_CLRBIT(MSDC_IOCON(ch0), MSDC_IOCON_D_SMPL);
                MSDC_SETBIT(MSDC_IOCON(ch0), (cur_dsmpl << MSDC_IOCON_D_SMPL_SHIFT));
#endif

                result = MsdcSendCmd0(host, cmd, data);

#if defined(CC_MT5399) || defined(CONFIG_ARCH_MT5399)|| \
    defined(CC_MT5890) || defined(CONFIG_ARCH_MT5890)
                    MSDC_LOG(MSG_LVL_ERR, "[1]TUNE_WRITE<%s> DSPL<%d> DATWRDLY<0x%x> MSDC_DAT_RDDLY0<0x%x> WRDAT_CRCS_TA_CNTR<0x%x> cmd_error<%d> data_error<%d>", 
                                      result == MSDC_SUCCESS ? "PASS" : "FAIL", cur_dsmpl, cur_wrrdly, cur_rxdly0, cur_d_cntr,
                                      cmd->error, data->error);
#else
                MSDC_LOG(MSG_LVL_ERR, "[0]TUNE_WRITE<%s> DSPL<%d> DATWRDLY<%d> MSDC_DAT_RDDLY0<0x%x> cmd_error<%d> data_error<%d>", 
                                  result == MSDC_SUCCESS ? "PASS" : "FAIL", cur_dsmpl, cur_wrrdly, cur_rxdly0,
                                  cmd->error, data->error);
#endif

                if(result == MSDC_SUCCESS)
                {
                    cmd->error = 0;
                    data->error = 0;
                    if(data->stop)
                        data->stop->error = 0;
					
                    goto done;
                }
                else
                {
                    if((result != MSDC_SUCCESS) && (result != ERR_DAT_FAILED))
                    {
                        MSDC_LOG(MSG_LVL_ERR, "[0]TUNE_WRITE(1): result<0x%x> cmd_error<%d> data_error<%d>", 
                                result, cmd->error, data->error);
						
                        goto done; 
                    }
                    else if((result != MSDC_SUCCESS) && (result == ERR_DAT_FAILED))
                    {
                        MSDC_LOG(MSG_LVL_ERR, "[0]TUNE_WRITE(2): result<0x%x> cmd_error<%d> data_error<%d>", 
                                result, cmd->error, data->error);
                    }
                }				
            }

            cur_wrrdly = (orig_wrrdly + wrrdly + 1) % 32;
            MSDC_CLRBIT(PAD_TUNE(ch0), PAD_DAT_WR_RXDLY);
            MSDC_SETBIT(PAD_TUNE(ch0), (cur_wrrdly << PAD_DAT_WR_RXDLY_SHIFT));
        }while(++wrrdly < 32);
		
        cur_dat0 = (orig_dat0 + rxdly) % 32; /* only adjust bit-1 for crc */
        cur_dat1 = orig_dat1;
        cur_dat2 = orig_dat2;
        cur_dat3 = orig_dat3;                    

        /* E1 ECO. YD: Reverse */
        if (MSDC_READ32(ECO_VER(ch0)) >= 4) 
        {
            cur_rxdly0 = (cur_dat0 << 24) | (cur_dat1 << 16) | (cur_dat2 << 8) | (cur_dat3 << 0);  
        }
        else
        {
            cur_rxdly0 = (cur_dat3 << 24) | (cur_dat2 << 16) | (cur_dat1 << 8) | (cur_dat0 << 0); 
        }
        MSDC_WRITE32(DAT_RD_DLY0(ch0), cur_rxdly0); 

    }while(++rxdly < 32);
#if defined(CC_MT5399) || defined(CONFIG_ARCH_MT5399)|| \
    defined(CC_MT5890) || defined(CONFIG_ARCH_MT5890)
        cur_d_cntr = (orig_d_cntr + d_cntr + 1) % 8;
        MSDC_CLRBIT(PATCH_BIT1(ch0), WRDAT_CRCS_TA_CNTR);
        MSDC_SETBIT(PATCH_BIT1(ch0), (cur_d_cntr << WRDAT_CRCS_TA_CNTR_SHIFT));
    } while (++d_cntr < 8);
#endif

done:
    return result;
}

int MsdcReqDataTune0(struct msdc_host *host, struct mmc_command *cmd, struct mmc_data *data)
{
    int i4Ret = MSDC_SUCCESS;
	
    if (data->flags & MMC_DATA_READ)
    {
        i4Ret = MsdcReqDataReadTune0(host, cmd, data);
    }
    else
    {
        i4Ret = MsdcReqDataWriteTune0(host, cmd, data);
    }

    return i4Ret;
}

/*
 * Scatter/gather functions
 */

static inline void msdc0_init_sg(struct msdc_host *host, struct mmc_data *data)
{
	/*
	 * Get info. about SG list from data structure.
	 */
	host->cur_sg = data->sg;
	host->num_sg = data->sg_len;

	host->offset = 0;
	host->remain = host->cur_sg->length;
}

static inline int msdc0_next_sg(struct msdc_host *host)
{
	/*
	 * Skip to next SG entry.
	 */
	host->cur_sg++;
	host->num_sg--;

	/*
	 * Any entries left?
	 */
	if (host->num_sg > 0)
	{
		host->offset = 0;
		host->remain = host->cur_sg->length;
	}

	return host->num_sg;
}

static inline char *msdc0_sg_to_buffer(struct msdc_host *host)
{
    return sg_virt(host->cur_sg);
}

static int msdc0_send_command(struct msdc_host *host, struct mmc_command *cmd)
{
    int i4Ret;
    struct mmc_data *data = cmd->data;

    MSDC_LOG(MSG_LVL_INFO, "[0]Start - Sending cmd (%d), flags = 0x%08x\n", cmd->opcode, cmd->flags);

    // Save current command
    memcpy(&(host->cur_cmd), cmd, sizeof(struct mmc_command));

    /*
      * Send the command
      */

    i4Ret = MsdcSendCmd0(host, cmd, data);
    if((i4Ret != MSDC_SUCCESS) && data && data->error)
    {
        i4Ret = MsdcReqDataTune0(host, cmd, data);   
        if(i4Ret != MSDC_SUCCESS)
        {
            goto end;
        }
    }

end:
	return i4Ret;
}

/*****************************************************************************\
 *                                                                           *
 * MMC callbacks                                                             *
 *                                                                           *
\*****************************************************************************/

static void msdc0_request_end(struct msdc_host *host, struct mmc_request *mrq)
{

    MSDC_LOG(MSG_LVL_INFO, "[0]%s: req done (CMD%u): %d/%d/%d: %08x %08x %08x %08x\n",
    	 mmc_hostname(host->mmc), mrq->cmd->opcode, mrq->cmd->error,
    	 mrq->data ? mrq->data->error : 0,
    	 mrq->stop ? mrq->stop->error : 0,
    	 mrq->cmd->resp[0], mrq->cmd->resp[1], mrq->cmd->resp[2], mrq->cmd->resp[3]);

    // Save current finished command
    memcpy(&(host->pre_cmd), mrq->cmd, sizeof(struct mmc_command));


    host->mrq = NULL;

    /*
     * MMC layer might call back into the driver so first unlock.
     */
    //spin_unlock(&host->lock);
    up(&host->msdc_sema);
    mmc_request_done(host->mmc, mrq);
    //spin_lock(&host->lock);
    //down_interruptible(&host->msdc_sema);
}

#ifdef CC_SDMMC_SUPPORT
int msdc0_card_exist(void);
#endif
static void msdc0_request(struct mmc_host *mmc, struct mmc_request *mrq)
{
    struct msdc_host *host = mmc_priv(mmc);
    struct mmc_command *cmd;
    int i4Ret;

    //spin_lock_bh(&host->lock);
    down(&host->msdc_sema);

    cmd = mrq->cmd;

    host->mrq = mrq;

    MSDC_LOG(MSG_LVL_INFO, "[0]Request : cmd (%d), arg (0x%x)\n", cmd->opcode, cmd->arg);

#ifdef CC_SDMMC_SUPPORT
    if(!msdc0_card_exist()) 
    {
        MSDC_LOG(MSG_LVL_INFO, "Check if there is acturally a card\n");
        cmd->error = -ENOMEDIUM;
        goto done;
    }
#endif

    i4Ret = msdc0_send_command(host, cmd);

#ifdef CC_SDMMC_SUPPORT
    if((cmd->error == -ENOMEDIUM) || ((cmd->data) && (cmd->data->error == -ENOMEDIUM)))
    {
        goto done;	
    }
#endif

    if (cmd->data && (cmd->error == 0) && (cmd->data->error == 0))
    {
        MSDC_LOG(MSG_LVL_INFO, "[0]Request with Data Success! Ending data transfer (%d bytes)\n", cmd->data->bytes_xfered);
    }
    else if((cmd->data == NULL) && (cmd->error == 0))
    {
        MSDC_LOG(MSG_LVL_INFO, "[0]Request without Data Success!\n");
    }
    else
    {
        MSDC_LOG(MSG_LVL_INFO, "Request Failed: %d!\n", cmd->error);
    }

#ifdef CC_SDMMC_SUPPORT
done:
#endif
    //spin_unlock_bh(&host->lock);
    //up(&host->msdc_sema);
    msdc0_request_end(host, mrq);
}

static void msdc0_reset_variables(struct msdc_host *host)
{
    // Reset variables
    host->hispeedcard = false;
}

/*
 * Initial GPD and BD pointer
 */
static void msdc0_init_gpd_bd(struct msdc_host *host)
{
    msdc_gpd_t *gpd = host->gpd; 
    msdc_bd_t  *bd  = host->bd; 	
    msdc_bd_t  *ptr, *prev;

    /* we just support one gpd */     
    int bdlen = MAX_BD_PER_GPD;   	

    /* init the 2 gpd */
    memset(gpd, 0, sizeof(msdc_gpd_t) * 2);
    //gpd->next = (void *)virt_to_phys(gpd + 1); /* pointer to a null gpd, bug! kmalloc <-> virt_to_phys */  
    //gpd->next = (dma->gpd_addr + 1);    /* bug */
    gpd->pNext = (void *)((u32)host->gpd_addr + sizeof(msdc_gpd_t));    

    //gpd->intr = 0;
    gpd->bdp  = 1;   /* hwo, cs, bd pointer */      
    //gpd->ptr  = (void*)virt_to_phys(bd); 
    gpd->pBuff = (void *)host->bd_addr; /* physical address */

    memset(bd, 0, sizeof(msdc_bd_t) * bdlen);
    ptr = bd + bdlen - 1;
    //ptr->eol  = 1;  /* 0 or 1 [Fix me]*/
    //ptr->next = 0;    

    while (ptr != bd) 
    {
        prev = ptr - 1;
        prev->pNext = (void *)(host->bd_addr + sizeof(msdc_bd_t) *(ptr - bd));
        ptr = prev;
    }
}

/*
 * Allocate/free DMA port and buffer
 */
static void __init msdc0_request_dma(struct msdc_host *host)
{
    /* using dma_alloc_coherent*/  
    host->gpd = (msdc_gpd_t *)dma_alloc_coherent(NULL, MAX_GPD_NUM * sizeof(msdc_gpd_t), 
                                                       &host->gpd_addr, GFP_KERNEL); 
    host->bd =  (msdc_bd_t *)dma_alloc_coherent(NULL, MAX_BD_NUM  * sizeof(msdc_bd_t),  
                                                       &host->bd_addr,  GFP_KERNEL); 
    BUG_ON((!host->gpd) || (!host->bd));    
    msdc0_init_gpd_bd(host);
}

static void __init msdc0_release_dma(struct msdc_host *host)
{
    /* using dma_alloc_coherent*/  
    dma_free_coherent(NULL, MAX_GPD_NUM * sizeof(msdc_gpd_t), 
                      host->gpd, host->gpd_addr); 
    dma_free_coherent(NULL, MAX_BD_NUM  * sizeof(msdc_bd_t),  
                      host->gpd, host->bd_addr);
}

#ifdef CC_SDMMC_SUPPORT
static void msdc0_tasklet_finish(unsigned long param)
{
    struct msdc_host *host = (struct msdc_host *)param;
    
    WARN_ON(host->mrq == NULL);
    
    if ((host->mrq->data) && (host->mrq->data->error))
    {
        host->mrq->data->bytes_xfered = 0;
    }
    else if(host->mrq->data && (host->mrq->data->error == 0))
    {
        host->mrq->data->bytes_xfered = host->mrq->data->blocks * host->mrq->data->blksz;
    }
    else
    {
    	
    }

    up(&host->msdc_sema);
    msdc0_request_end(host, host->mrq);
}

static void msdc0_init_tasklet(struct msdc_host *host)
{
    tasklet_init(&finish_tasklet, msdc0_tasklet_finish, 
		        (unsigned long)host);
}
#endif

/*
 * Allocate/free IRQ.
 */
static int __init msdc0_request_irq(struct msdc_host *host)
{
    if(host->msdc_isr_en == 1)
    {
        MsdcRegIsr0(host);
        MsdcSetIsr0(INT_SD_ALL, host);
    }
    
#ifdef CC_SDMMC_SUPPORT
    if(_sd_gpio_isr == 1)
    {
        MsdcRegGPIOIsr0(host);
        MsdcSetGPIOIsr0(0x0, host);    
    }
#endif

    return 0;
}

static void msdc0_release_irq(struct msdc_host *host)
{
    if (!host->irq)
        return;

    free_irq(host->irq, host);

    host->irq = 0;
}

static int  MsdcSDPowerSwitch(INT32 i4V)
{
    if(MSDC_SUCCESS != mtk_gpio_direction_output(MSDC_Gpio[MSDC0VoltageSwitchGpio], i4V))
    {
		return MSDC_FAILED;
	}
	return MSDC_SUCCESS;
}
#ifdef CC_SDMMC_SUPPORT
// wait for extend if power pin is controlled by GPIO
void msdc0_card_power_up(void)
{	
}
// wait for extend if power pin is controlled by GPIO
void msdc0_card_power_down(void)
{	
}
#ifdef CONFIG_MT53XX_NATIVE_GPIO
int msdc0_card_exist(void)
{
    unsigned int val;	
    MSDC_LOG(MSG_LVL_INFO,"MSDC_Gpio[MSDC0DetectGpio] :%d \n",MSDC_Gpio[MSDC0DetectGpio]);

#if  defined(CC_MT5880) || defined(CONFIG_ARCH_MT5880)
    if (IS_IC_MT5860_E2IC()) // sd card
#endif
    {
        //SD CARD DETECT GPIO VALUE
    if(MSDC_Gpio[MSDC0DetectGpio] == -1)
    {
        MSDC_LOG(MSG_LVL_INFO, "return 0 \n");
        return 0;	
    }
    else
    {
        mtk_gpio_direction_input(MSDC_Gpio[MSDC0DetectGpio]);
        val = mtk_gpio_get_value(MSDC_Gpio[MSDC0DetectGpio]);
        MSDC_LOG(MSG_LVL_INFO, "return val: %d \n",val);
    }
        if(val)	
        {
            msdc0_card_power_down();   
            MSDC_LOG(MSG_LVL_INFO, "[0]Card not Insert(%d)!\n", val);	
            return 0;	
        }	

        msdc0_card_power_up();

        MSDC_LOG(MSG_LVL_INFO, "[0]Card Insert(%d)!\n", val);   
        return 1;
    }
#if  defined(CC_MT5880) || defined(CONFIG_ARCH_MT5880)
    else // emmc
    {
        return 1;
    }
#endif
}

#else
int msdc0_card_exist(void)
{
    unsigned int val;	

#if  defined(CC_MT5880) || defined(CONFIG_ARCH_MT5880)
    if (IS_IC_MT5860_E2IC()) // sd card
#endif
    {
        //SD CARD DETECT GPIO VALUE
        val = SD_CARD_DETECT_GPIO;
	
        if(val)	
        {
            msdc0_card_power_down();   
            MSDC_LOG(MSG_LVL_INFO, "[0]Card not Insert(%d)!\n", val);	
            return 0;	
        }	

        //msdc0_card_power_up();

        MSDC_LOG(MSG_LVL_INFO, "[0]Card Insert(%d)!\n", val);   
        return 1;
    }
#if  defined(CC_MT5880) || defined(CONFIG_ARCH_MT5880)
    else // emmc
    {
        return 1;
    }
#endif
}
#endif
static void msdc0_tasklet_card(unsigned long param)
{
    struct msdc_host *host = (struct msdc_host *)param;
    int delay = -1;

    _bMSDCCardExist = msdc0_card_exist();
    MSDC_LOG(MSG_LVL_INFO, "[0]Card Detect %d(%d)\n", _bMSDCCardExist, plugCnt);

    if(plugCnt >= 2)
    {
        detectFlag = 1;
    }

	if (_bMSDCCardExist)    
	{        	            
	    if (!(host->flags & SDHCI_FCARD_PRESENT))        
		{            		                

		    // Prevent glitch            		                
		    mdelay(10);              
			if(msdc0_card_exist() && (plugCnt < 2))            
			{			                

			    MSDC_LOG(MSG_LVL_ERR, "[0]Card inserted\n");
				host->flags |= SDHCI_FCARD_PRESENT;
				msdc0_card_power_up();

				// Reset variables            			                
                msdc0_reset_variables(host);
				MsdcInit0();
				delay = 0;  
				
                goto ErrEnd;            
            }
            else
            {                
                // do nothing	            	            
            }
		}
		else
        {
        }
    }
    else
    {
        if (host->flags & SDHCI_FCARD_PRESENT)
        {            
            // Prevent glitch
            mdelay(10);
			if((!msdc0_card_exist()) && (plugCnt < 2))
            {
                MSDC_LOG(MSG_LVL_ERR, "[0]Card removed\n");
				host->flags &= ~SDHCI_FCARD_PRESENT;
				if (host->mrq)
                {
                    MSDC_LOG(MSG_LVL_ERR, "[0]%s: Card removed during transfer!\n", mmc_hostname(host->mmc));
					msdc0_card_power_down();

					// Reset variables
					msdc0_reset_variables(host);
					MsdcInit0();
					host->mrq->cmd->error = -ENOMEDIUM;
					tasklet_schedule(&finish_tasklet);
				}

				delay = 0;

				goto ErrEnd;
            }
            else
            {
                // do nothing
            }
        }
        else
        {
        }
    }
	
	if((detectFlag == 1) && (plugCnt < 2))
    {
        // Prevent glitch
        mdelay(10);
        if(msdc0_card_exist())
        {
            MSDC_LOG(MSG_LVL_INFO, "[0]Card inserted(*)\n");
			host->flags |= SDHCI_FCARD_PRESENT;
			msdc0_card_power_up();
			// Reset variables
			msdc0_reset_variables(host);
			MsdcInit0();
			delay = 0;
		}
		else
		{
		    MSDC_LOG(MSG_LVL_INFO, "[0]Card removed(*)\n");
			host->flags &= ~SDHCI_FCARD_PRESENT;
			if (host->mrq)
            {
                MSDC_LOG(MSG_LVL_INFO, "[0]%s: Card removed during transfer!\n", mmc_hostname(host->mmc));
				msdc0_card_power_down();
				// Reset variables
				msdc0_reset_variables(host);
				MsdcInit0();
				host->mrq->cmd->error = -ENOMEDIUM;
				tasklet_schedule(&finish_tasklet);
			}
			
			delay = 0;
		}

		goto ErrEnd;
	}

ErrEnd:
	
    plugCnt = 0;  		      	    
	if (delay != -1)
	{    	  
        detectFlag = 0;  
        mmc_detect_change(host->mmc, msecs_to_jiffies(delay));     
    }   	    

    mod_timer(&card_detect_timer, jiffies + HZ/3);
}
#endif

static void msdc0_free_mmc(struct device *dev)
{
    struct mmc_host *mmc;
    struct msdc_host *host;

    mmc = dev_get_drvdata(dev);
    if (!mmc)
        return;

    host = mmc_priv(mmc);
    BUG_ON(host == NULL);

    mmc_free_host(mmc);

    dev_set_drvdata(dev, NULL);
}

/*
 * Add a set_ios hook which is called when the SDHCI driver
 * is called to change parameters such as clock or card width.
 */
static void msdc0_set_ios(struct mmc_host *mmc, struct mmc_ios *ios)
{
    struct msdc_host *host = mmc_priv(mmc);

    MSDC_LOG(MSG_LVL_INFO, "[0]ios->power_mode = 0x%x, ios->clock =%d, ios->bus_width = 0x%x\n", ios->power_mode, ios->clock, ios->bus_width);

    //spin_lock_bh(&host->lock);
    down(&host->msdc_sema);

    host->clk = ios->clock;
    host->bus_width = ios->bus_width;

    MsdcSetIos0(mmc, ios);
    
#if  defined(CC_MT5880) || defined(CONFIG_ARCH_MT5880)
    if (IS_IC_MT5860_E2IC()) // sd card
    {
    }
    else // emmc
    {
        if(host->clk >= 26000000)
        {
            if(mmc->caps & MMC_CAP_MMC_HIGHSPEED)
            {
                MsdcSetSampleEdge0(2);
            }
         }
    }
#endif

    //spin_unlock_bh(&host->lock);
    up(&host->msdc_sema);
}

static int msdc0_get_ro(struct mmc_host *mmc)
{
    return 0;
}

static int msdc0_get_cd(struct mmc_host *mmc)
{
#ifdef CC_SDMMC_SUPPORT  
	return msdc0_card_exist();
#else
    return 1;
#endif
}

#if defined(CC_MT5399) || defined(CONFIG_ARCH_MT5399)|| \
    defined(CC_MT5890) || defined(CONFIG_ARCH_MT5890)
static inline void mmc_delay(unsigned int ms)
{
	if (ms < 1000 / HZ) {
		cond_resched();
		mdelay(ms);
	} else {
		msleep(ms);
	}
}

static int msdc0_do_start_signal_voltage_switch(struct mmc_host *mmc,
	struct mmc_ios *ios)
{   
    // wait SDC_STA.SDCBUSY = 0
    while(MSDC_READ32(SDC_STS(ch0))&SDC_STS_SDCBUSY);

	// Disable pull-up resistors
	MSDC_CLRBIT(SD30_PAD_CTL1(ch0), (0x1<<17));
	MSDC_CLRBIT(SD30_PAD_CTL2(ch0), (0x1<<17));

    // make sure CMD/DATA lines are pulled to 0
	while((MSDC_READ32(MSDC_PS(ch0))&(0x1FF<<16)));

	// Program the PMU of host from 3.3V to 1.8V
	if(MSDC_SUCCESS != MsdcSDPowerSwitch(1))
    {
		return MSDC_FAILED;
	}

	// Delay at least 5ms
	mmc_delay(6);
    

	// Enable pull-up resistors
	MSDC_SETBIT(SD30_PAD_CTL1(ch0), (0x1<<17));
	MSDC_SETBIT(SD30_PAD_CTL2(ch0), (0x1<<17));
    

	// start 1.8V voltage detecting
	MSDC_SETBIT(MSDC_CFG(ch0), MSDC_CFG_BUS_VOL_START);

	// Delay at most 1ms
    mmc_delay(1);
    
	// check MSDC.VOL_18V_START_DET is 0
	while(MSDC_READ32(MSDC_CFG(ch0))&MSDC_CFG_BUS_VOL_START);
  

	// check MSDC_CFG.VOL_18V_PASS
	if(MSDC_READ32(MSDC_CFG(ch0))&MSDC_CFG_BUS_VOL_PASS)
    {
        MSDC_LOG(MSG_LVL_ERR, "switch 1.8v voltage success!\n");  
      	return MSDC_SUCCESS;
	}
	else
	{
        MSDC_LOG(MSG_LVL_ERR, "switch 1.8v voltage faled!\n"); 
		return MSDC_FAILED;
	}
    
}

static int msdc0_start_signal_voltage_switch(struct mmc_host *mmc,
	struct mmc_ios *ios)
{
	struct msdc_host *host = mmc_priv(mmc);
	int err;

	if (mmc->ios.signal_voltage < 1)
		return 0; 
	err = msdc0_do_start_signal_voltage_switch(host, ios);

	return err;
}
#endif
static const struct mmc_host_ops msdc0_ops = {
   .request  = msdc0_request,
   .set_ios  = msdc0_set_ios,
   .get_ro   = msdc0_get_ro,
   .get_cd   = msdc0_get_cd,
#if defined(CC_MT5399) || defined(CONFIG_ARCH_MT5399)|| \
    defined(CC_MT5890) || defined(CONFIG_ARCH_MT5890)
   .start_signal_voltage_switch	= msdc0_start_signal_voltage_switch,
#endif
};

/*****************************************************************************\
 *                                                                           *
 * Suspend/resume                                                            *
 *                                                                           *
\*****************************************************************************/
#ifdef CONFIG_PM
static int msdc0_platform_suspend(struct platform_device *dev, pm_message_t state)
{
    int ret = 0;
    struct mmc_host *mmc = msdc_host0->mmc;

    // mmc suspend call
    if (mmc)
        ret = mmc_suspend_host(mmc);

    down(&msdc_host0->msdc_sema);

    MsdcSetClkfreq0(0);
    MsdcSetClkfreq0(msdc_host0->clk);
    MsdcSetBusWidth0(msdc_host0->bus_width);
    
    disable_irq(msdc_host0->irq);

    //gate hclk & sclk
    MSDC_SETBIT(MSDC_CLK_H_REG0, MSDC_CLK_GATE_BIT);	
    MSDC_SETBIT(MSDC_CLK_S_REG0, MSDC_CLK_GATE_BIT);	

    up(&msdc_host0->msdc_sema);

    return ret;
}

static int msdc0_platform_resume(struct platform_device *dev)
{
    int ret = 0;
    struct mmc_host *mmc = msdc_host0->mmc;

    down(&msdc_host0->msdc_sema);
    
    //gate hclk & sclk
    MSDC_CLRBIT(MSDC_CLK_H_REG0, MSDC_CLK_GATE_BIT);	
    MSDC_CLRBIT(MSDC_CLK_S_REG0, MSDC_CLK_GATE_BIT);	

    // Init Host Controller
    MsdcInit0();
    
    // Enable IRQ
    MSDC_SETBIT(MSDC0_EN_REG, (0x1<<MSDC0_EN_SHIFT));
    MSDC_WRITE32(MSDC_INTEN(ch0), INT_SD_ALL);
    enable_irq(msdc_host0->irq);

    init_completion(&comp0);
    
    // mmc resume call
    if (mmc)
        ret = mmc_resume_host(mmc);

#if  defined(CC_MT5880) || defined(CONFIG_ARCH_MT5880)
    if (IS_IC_MT5860_E2IC()) // sd card
    {
        // don't need set sd card clock now
    }
    else // emmc
    {
        MsdcSetClkfreq0(mmc->f_max);
        MsdcSetBusWidth0(MMC_BUS_WIDTH_4);
    }
#endif

    up(&msdc_host0->msdc_sema);

    return ret;
}
#else
#define msdc0_platform_suspend NULL
#define msdc0_platform_resume NULL
#endif


/*****************************************************************************\
 *                                                                           *
 * Device probing/removal                                                    *
 *                                                                           *
\*****************************************************************************/

/*
 * Allocate/free MMC structure.
 */

static struct sdhci_msdc_pdata sdhci_msdc0_pdata = {
  /* This is currently here to avoid a number of if (host->pdata)
   * checks. Any zero fields to ensure reaonable defaults are picked. */
};

static int __init msdc0_probe(struct platform_device *pdev)
{
    struct msdc_host *host = NULL;
    struct mmc_host *mmc = NULL;
    int ret = 0;
   
#if  defined(CC_MT5880) || defined(CONFIG_ARCH_MT5880)
    if (IS_IC_MT5860_E2IC()) // sd card
#endif
    {
#ifdef CONFIG_MTKEMMC_BOOT
        down(&msdc_host_sema);
#endif
    }
    MSDC_LOG(MSG_LVL_INFO, "\n");

    /*
      * Allocate MMC structure.
      */
    mmc = mmc_alloc_host(sizeof(struct msdc_host), &pdev->dev);

    if (!mmc)
    {
      ret = -ENOMEM;
      goto probe_out;
    }

    host = mmc_priv(mmc);
    host->mmc = mmc;
    host->pdev  = pdev;
    host->pdata = pdev->dev.platform_data;
    host->chip_id = CONFIG_CHIP_VERSION;
    host->base = (MSDC_BASE_ADDR + ch0*MSDC_CH_OFFSET);
    host->data_mode = DESC_DMA_DATA_MODE;
    host->irq = ch0?(VECTOR_MSDC2):(VECTOR_MSDC);
    host->devIndex = 0;
    host->MsdcAccuVect = 0x00;
    host->msdc_isr_en = 1;
    host->desVect = 0;
    host->waitIntMode = 0;
    host->timeout_ns = 0;    
    host->timeout_clks = DEFAULT_DTOC * 65536;

    // Reset variables
    msdc0_reset_variables(host);

#ifdef CC_SDMMC_SUPPORT 
    MsdcSDPowerSwitch(0);//set the host ic gpio to 3.3v
    // Default card exists
    _bMSDCCardExist = msdc0_card_exist();
    if(_bMSDCCardExist)
    {	
        host->flags |= SDHCI_FCARD_PRESENT;
        msdc0_card_power_up();
    }
	else
    {
        host->flags &= (~SDHCI_FCARD_PRESENT);
        msdc0_card_power_down();
    }
#else
    // Default card exists
    host->flags |= SDHCI_FCARD_PRESENT;
#endif

    if (!host->pdata)
    {
      pdev->dev.platform_data = &sdhci_msdc0_pdata;
      host->pdata = &sdhci_msdc0_pdata;
    }

    // Save host pointer for futher useage
    msdc_host0 = host;
#if  defined(CC_MT5880) || defined(CONFIG_ARCH_MT5880)
    if (IS_IC_MT5860_E2IC()) // sd card
    {
    }
    else // emmc
    {
	    msdc_host_boot = msdc_host0;
        msdc_send_command = msdc0_send_command;
    }
#endif

    /*
     * Set host parameters.
     */
    mmc->ops = &msdc0_ops;
    mmc->f_min = 397000;
    mmc->f_max = 48000000;

    mmc->ocr_avail = MMC_VDD_30_31 | MMC_VDD_31_32 | MMC_VDD_32_33 | MMC_VDD_33_34 | MMC_VDD_34_35 | MMC_VDD_35_36;
    mmc->caps = MMC_CAP_4_BIT_DATA | MMC_CAP_SD_HIGHSPEED | MMC_CAP_MMC_HIGHSPEED;
#if defined(CC_MT5399) || defined(CONFIG_ARCH_MT5399)|| \
    defined(CC_MT5890) || defined(CONFIG_ARCH_MT5890)
    mmc->f_max = 200000000; 
    mmc->caps |= (MMC_CAP_1_8V_DDR | MMC_CAP_UHS_DDR50);
    #if 0
    mmc->caps |= MMC_CAP_UHS_SDR12 | MMC_CAP_UHS_SDR25 |
                 MMC_CAP_UHS_SDR50 | MMC_CAP_UHS_SDR104;
    mmc->caps2 |= MMC_CAP2_HS200_1_8V_SDR;
    #endif
	#endif  
#if  defined(CC_MT5880) || defined(CONFIG_ARCH_MT5880)
    if (IS_IC_MT5860_E2IC())  // sd card
#endif
    {
        mmc->special_arg = MSDC_TRY_SD;   // special for SD host
    }
#if  defined(CC_MT5880) || defined(CONFIG_ARCH_MT5880)
    else // emmc
    {
        mmc->special_arg = MSDC_TRY_MMC;   // special for eMMC host
    }
#endif

    spin_lock_init(&host->lock);
    sema_init(&host->msdc_sema, 1);
    init_completion(&comp0);

    /*
     * Maximum number of segments. Worst case is one sector per segment
     * so this will be 64kB/512.
     */
    mmc->max_segs = 128;

    /*	 * Maximum request size. Also limited by 64KiB buffer.	 */
    mmc->max_req_size = 128*(64*1024-512);

    /*
     * Maximum segment size. Could be one segment with the maximum number
     * of bytes.
     */

    mmc->max_seg_size =(64*1024-512);

    /*
     * Maximum block size. We have 12 bits (= 4095) but have to subtract
     * space for CRC. So the maximum is 4095 - 4*2 = 4087.
     */

    mmc->max_blk_size = 512;

    /*
     * Maximum block count. There is no real limit so the maximum
     * request size will be the only restriction.
     */

    mmc->max_blk_count = mmc->max_req_size;

    dev_set_drvdata(&(pdev->dev), mmc);


    // Initialize MSDC hardware
    MsdcInit0();

    /*
     * Allocate DMA.
     */
    msdc0_request_dma(host);

    /* Setup irq function */
    if (msdc0_request_irq(host))
    {
        dev_err(&pdev->dev, "[0]failed to request sdhci interrupt.\n");
        ret = -ENOENT;
        goto probe_out;
    }

#ifdef CC_SDMMC_SUPPORT 
    /* Initial tasklet */
    msdc0_init_tasklet(host);
#endif

    /* We get spurious interrupts even when we have set the IMSK
     * register to ignore everything, so use disable_irq() to make
     * ensure we don't lock the system with un-serviceable requests. */
    // disable_irq(host->irq);

    mmc_add_host(mmc);

    mmc = dev_get_drvdata(&(pdev->dev));

    printk(KERN_ERR "%s:", mmc_hostname(mmc));
    if (host->chip_id != 0)
      printk(" %d", (int)host->chip_id);
    printk(KERN_ERR " at 0x%x irq %d data mode %d\n", (int)host->base, (int)host->irq, (int)(host->data_mode));

    /* 
     * Set up timers     
     */
#ifdef CC_SDMMC_SUPPORT    
    init_timer(&card_detect_timer);    
    card_detect_timer.data = (unsigned long)host;    
    card_detect_timer.function = msdc0_tasklet_card;    
    card_detect_timer.expires = jiffies + 2 * HZ;    
    add_timer(&card_detect_timer);
#endif

probe_out:
#if  defined(CC_MT5880) || defined(CONFIG_ARCH_MT5880)
    if (IS_IC_MT5860_E2IC()) // sd card
#endif
    {
#ifdef CONFIG_MTKEMMC_BOOT
        up(&msdc_host_sema);
#endif
    }
    return ret;
    
}
#ifdef CONFIG_MT53XX_NATIVE_GPIO


static int __init MSDCGPIO_CommonParamParsing(char *str, int *pi4Dist)
{
    char tmp[8]={0};
    char *p,*s;
    int len,i,j;
    if(strlen(str) != 0)
	{      
        printk(KERN_ERR "Parsing String = %s \n",str);
    }
    else
	{
        printk(KERN_ERR "Parse Error!!!!! string = NULL\n");
        return 0;
    }

    for (i=0; i<MSDC_GPIO_MAX_NUMBERS; i++)
	{
        s=str;
        if (i != (MSDC_GPIO_MAX_NUMBERS-1) )
		{
            if(!(p=strchr(str, ',')))
			{
                printk(KERN_ERR "Parse Error!!string format error 1\n");
                break;
            }
            if (!((len=p-s) >= 1))
            {
                printk(KERN_ERR "Parse Error!! string format error 2\n");
                break;
            }
        }
		else 
        {
            len = strlen(s);
        }
        
        for(j=0;j<len;j++)
        {
            tmp[j]=*s;
            s++;
        }
        tmp[j]=0;

        pi4Dist[i] = (int)simple_strtol(&tmp[0], NULL, 10);
        printk(KERN_ERR"Parse Done: msdc [%d] = %d \n", i, pi4Dist[i]);
        
        str += (len+1);
    }
    
    return 1;

}


static int __init MSDC_GpioParseSetup(char *str)
{
    return MSDCGPIO_CommonParamParsing(str,&MSDC_Gpio[0]);
}

__setup("msdcgpio=", MSDC_GpioParseSetup);

#endif
/*****************************************************************************\
 *                                                                           *
 * Driver init/exit                                                          *
 *                                                                           *
\*****************************************************************************/

/*
 * Release all resources for the host.
 */
static void __exit msdc0_shutdown(struct device *dev)
{
    struct mmc_host *mmc = dev_get_drvdata(dev);
    struct msdc_host *host;

    if (!mmc)
        return;

    host = mmc_priv(mmc);

    mmc_remove_host(mmc);

    msdc0_release_dma(host);

    msdc0_release_irq(host);

    msdc0_free_mmc(dev);
}

static int __exit msdc0_remove(struct platform_device *dev)
{
    MSDC_LOG(MSG_LVL_INFO, "\n");

    msdc0_shutdown(&dev->dev);

    return 0;
}

static struct platform_device *msdc0_device;
static struct platform_driver msdc0_driver = {
    .probe    = msdc0_probe,
    .remove   = __exit_p(msdc0_remove),

    .suspend  = msdc0_platform_suspend,
    .resume   = msdc0_platform_resume,
    .driver   = {
        .name = DRIVER_NAME0,
        .owner= THIS_MODULE,
    },
};

static int __init msdc0_drv_init(void)
{
    int result;

    MSDC_LOG(MSG_LVL_INFO, "[0]initialize..##.\n");

    result = platform_driver_register(&msdc0_driver);

    if (result < 0)
    {
        MSDC_LOG(MSG_LVL_ERR, "[0]platform_driver_register failed !!\n");
        return result;
    }

    msdc0_device = platform_device_alloc(DRIVER_NAME0, -1);

    if (!msdc0_device)
    {
        MSDC_LOG(MSG_LVL_ERR, "[0]platform_device_alloc failed !!\n");
        platform_driver_unregister(&msdc0_driver);
        return -ENOMEM;
    }

    msdc0_device->dev.coherent_dma_mask = MTK_MSDC_DMA_MASK;
    msdc0_device->dev.dma_mask = &msdc0_device->dev.coherent_dma_mask;

    result = platform_device_add(msdc0_device);

    if (result)
    {
         MSDC_LOG(MSG_LVL_ERR, "[0]platform_device_add failed !!\n");
         platform_device_put(msdc0_device);
         platform_driver_unregister(&msdc0_driver);
         return result;
    }

    MSDC_LOG(MSG_LVL_INFO, "[0]initialize finish..\n");

    return 0;
}

static void __exit msdc0_drv_exit(void)
{
    MSDC_LOG(MSG_LVL_INFO, "\n");

    platform_device_unregister(msdc0_device);

    platform_driver_unregister(&msdc0_driver);
}


module_init(msdc0_drv_init);
module_exit(msdc0_drv_exit);

MODULE_AUTHOR("Shunli Wang");
MODULE_DESCRIPTION("MTK Secure Digital Host Controller Interface driver");
MODULE_VERSION(DRIVER_VERSION0);
MODULE_LICENSE("GPL");

