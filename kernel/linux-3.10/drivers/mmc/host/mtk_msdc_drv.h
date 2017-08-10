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
* [File]			msdc_drv.h
* [Version]			v1.0
* [Revision Date]	2011-05-04
* [Author]			Shunli Wang, shunli.wang@mediatek.inc, 82896, 2011-05-04
* [Description]
*	MSDC Driver Header File
* [Copyright]
*	Copyright (C) 2011 MediaTek Incorporation. All Rights Reserved.
******************************************************************************/

#ifndef _MSDC_DRV_H_
#define _MSDC_DRV_H_
#include "x_typedef.h"
#include "mtk_msdc_host_hw.h"
#include "mtk_msdc_slave_hw.h"
#include "mach/mt53xx_linux.h"

//---------------------------------------------------------------------------
// Configurations
//---------------------------------------------------------------------------
#if 1

#if defined(CC_MT5396) || defined(CONFIG_ARCH_MT5396) || \
    defined(CC_MT5368) || defined(CONFIG_ARCH_MT5368) || \
    defined(CC_MT5389) || defined(CONFIG_ARCH_MT5389)
#define MSDC_CLK_TARGET   54, 48, 43, 40, 36, 30, 27, 24, 18, 13,  0
#define MSDC_CLK_SRC_VAL   2,  3,  4,  8,  5,  7,  0,  9,  6,  1,  0
#define MSDC_CLK_MODE_VAL  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  0
#define MSDC_CLK_DIV_VAL   0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 16
#define MSDC_CLK_DRV_VAL  60, 60, 57, 57, 25, 23, 16, 16,  7,  7,  0
#define MSDC_CLK_SAMP_VAL 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x0, 0x0, 0x0, 0x0
#define MSDC_CLK_IDX_MAX  11
#define MSDC_CLK_S_REG1   0xF000D2F8
#define MSDC_CLK_S_REG0   0xF000D264
#define MSDC_CLK_H_REG1   0xF000D2F8
#define MSDC_CLK_H_REG0   0xF000D264
#define MSDC_CLK_GATE_BIT (0x1<<7)
#define MSDC_CLK_SEL_MASK (0x0F<<0)
#define MSDC_HIGH_CLK_IDX 1
#define MSDC_NORM_CLK_IDX 7
#define MSDC_INIT_CLK_IDX 10
   
#elif defined(CC_MT5880) || defined(CONFIG_ARCH_MT5880) || \
      defined(CC_MT5860) || defined(CONFIG_ARCH_MT5860) || \
      defined(CC_MT5881) || defined(CONFIG_ARCH_MT5881) || \
      defined(CC_MT5398) || defined(CONFIG_ARCH_MT5398)
#define MSDC_CLK_TARGET   54, 48, 43, 40, 36, 30, 27, 24, 18, 13,  0
#define MSDC_CLK_SRC_VAL   1,  2,  8,  6,  9,  5,  3,  7, 10,  4,  0
#define MSDC_CLK_MODE_VAL  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  0
#define MSDC_CLK_DIV_VAL   0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 16
#define MSDC_CLK_DRV_VAL  60, 60, 57, 57, 25, 23, 16, 16,  7,  7,  0
#define MSDC_CLK_SAMP_VAL 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x0, 0x0, 0x0, 0x0
#define MSDC_CLK_IDX_MAX  11
#define MSDC_CLK_S_REG1   0xF000D380
#define MSDC_CLK_S_REG0   0xF000D32C
#define MSDC_CLK_H_REG1   0xF000D384
#define MSDC_CLK_H_REG0   0xF000D3A0
#define MSDC_CLK_GATE_BIT (0x1<<7)
#define MSDC_CLK_SEL_MASK (0x0F<<0)
#define MSDC_HIGH_CLK_IDX 1
#define MSDC_NORM_CLK_IDX 7
#define MSDC_INIT_CLK_IDX 10

#elif defined(CC_MT5399) || defined(CONFIG_ARCH_MT5399) || \
	  defined(CC_MT5890) || defined(CONFIG_ARCH_MT5890) || \
	  defined(CC_MT5891) || defined(CONFIG_ARCH_MT5891) || \
	  defined(CC_MT5882) || defined(CONFIG_ARCH_MT5882) || \
	  defined(CC_MT5883) || defined(CONFIG_ARCH_MT5883)
#define MSDC_CLK_TARGET   200, 144, 120, 100, 80, 50, 50, 25, 12,  0
#define MSDC_CLK_SRC_VAL   11,   5,   6,  11,  9,  8, 11, 11,  0,  0
#define MSDC_CLK_MODE_VAL   1,   1,   1,   0,  1,  1,  2,  0,  0,  0
#define MSDC_CLK_DIV_VAL    0,   0,   0,   0,  0,  0,  0,  2,  0, 16
#define MSDC_CLK_DRV_VAL   60,  60,  57,  25, 23, 23, 16,  7,  7,  0
#define MSDC_CLK_SAMP_VAL 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x0, 0x0, 0x0, 0x0
#define MSDC_CLK_IDX_MAX  10
#define MSDC_CLK_S_REG1   0xF000D380
#define MSDC_CLK_S_REG0   0xF000D32C
#define MSDC_CLK_H_REG1   0xF000D384
#define MSDC_CLK_H_REG0   0xF000D3A0
#define MSDC_CLK_GATE_BIT (0x1<<7)
#define MSDC_CLK_SEL_MASK (0x0F<<0)
#define MSDC_HIGH_CLK_IDX 6
#define MSDC_NORM_CLK_IDX 7
#define MSDC_INIT_CLK_IDX 9

#endif


#else

#if defined(CC_MT5396) || defined(CC_MT5368) || defined(CC_MT5389) || \
    defined(CONFIG_ARCH_MT5396) || defined(CONFIG_ARCH_MT5368) || defined(CONFIG_ARCH_MT5389)
#define MSDC_CLOCK_SELECTION_STRING     54, 48, 43, 40, 36, 30, 27, 24, 18, 13, 0
#define MSDC_CLOCK_SELECTION_VALUE      2, 3, 4, 8, 5, 7, 0, 9, 6, 1, 0
#define MSDC_CLOCK_SELECTION_REG        0xF000D2F8
#define MSDC_SCLOCK_SELECTION_REG1      MSDC_CLOCK_SELECTION_REG
#define MSDC_SCLOCK_SELECTION_REG0      0xF000D264
#define MSDC_CLOCK_CKGEN_GATE_BIT       (0x1<<7)
#define MSDC_CLOCK_CKGEN_SLECTION_MASK  (0x0F<<0)
#define SD_DEFAULT_HIGH_CLOCK_INDEX 1
#define SD_DEFAULT_NORM_CLOCK_INDEX 7
#define SD_DEFAULT_INIT_CLOCK_INDEX 10
#elif defined(CC_MT5398) || \
      defined(CONFIG_ARCH_MT5398)
#define MSDC_CLOCK_SELECTION_STRING     54, 48, 43, 40, 36, 30, 27, 24, 18, 13, 0
#define MSDC_CLOCK_SELECTION_VALUE      1, 2, 8, 6, 9, 5, 3, 7, 10, 4, 0
#define MSDC_CLOCK_SELECTION_REG         0xF000D380
#define MSDC_SCLOCK_SELECTION_REG1       MSDC_CLOCK_SELECTION_REG
#define MSDC_SCLOCK_SELECTION_REG0       0xF000D32C
#define MSDC_HCLOCK_SELECTION_REG0       0xF000D3A0
#define MSDC_HCLOCK_SELECTION_REG1       0xF000D384
#define MSDC_CLOCK_CKGEN_GATE_BIT       (0x1<<7)
#define MSDC_CLOCK_CKGEN_SLECTION_MASK  (0x0F<<0)
#define SD_DEFAULT_HIGH_CLOCK_INDEX 1
#define SD_DEFAULT_NORM_CLOCK_INDEX 7
#define SD_DEFAULT_INIT_CLOCK_INDEX 10
#elif defined(CC_MT5880) || defined(CC_MT5860) || \
      defined(CONFIG_ARCH_MT5880) || defined(CONFIG_ARCH_MT5860)
#define MSDC_CLOCK_SELECTION_STRING     54, 48, 43, 40, 36, 30, 27, 24, 18, 13, 0
#define MSDC_CLOCK_SELECTION_VALUE      1, 2, 8, 6, 9, 5, 3, 7, 10, 4, 0
#define MSDC_SCLOCK_SELECTION_REG1       0xF000D380
#define MSDC_SCLOCK_SELECTION_REG0       0xF000D32C
#define MSDC_HCLOCK_SELECTION_REG0       0xF000D3A0
#define MSDC_HCLOCK_SELECTION_REG1       0xF000D384
#define MSDC_CLOCK_CKGEN_GATE_BIT       (0x1<<7)
#define MSDC_CLOCK_CKGEN_SLECTION_MASK  (0x0F<<0)
#define SD_DEFAULT_HIGH_CLOCK_INDEX 1
#define SD_DEFAULT_NORM_CLOCK_INDEX 7
#define SD_DEFAULT_INIT_CLOCK_INDEX 10
#elif defined(CC_MT5881) || defined(CONFIG_ARCH_MT5881)
#define MSDC_CLOCK_SELECTION_STRING     54, 48, 43, 40, 36, 30, 27, 24, 18, 13, 0
#define MSDC_CLOCK_SELECTION_VALUE      1, 2, 8, 6, 9, 5, 3, 7, 10, 4, 0
#define MSDC_SCLOCK_SELECTION_REG1       0xF000D380
#define MSDC_SCLOCK_SELECTION_REG0       0xF000D32C
#define MSDC_CLOCK_CKGEN_GATE_BIT       (0x1<<7)
#define MSDC_CLOCK_CKGEN_SLECTION_MASK  (0x0F<<0)
#define SD_DEFAULT_HIGH_CLOCK_INDEX 1
#define SD_DEFAULT_NORM_CLOCK_INDEX 7
#define SD_DEFAULT_INIT_CLOCK_INDEX 10
#elif defined(CC_MT5399) || defined(CONFIG_ARCH_MT5399) || defined(CC_MT5890) || defined(CONFIG_ARCH_MT5890)
#define MSDC_CLOCK_SELECTION_STRING     225, 216, 192, 172, 162, 144, 120, 108, 80, 48, 24, 13, 0
#define MSDC_CLOCK_SELECTION_VALUE      11, 1, 2, 3, 4, 5, 6, 7, 9, 8, 0, 10, 0
#define MSDC_CLOCK_SELECTION_REG         0xF000D380
#define MSDC_SCLOCK_SELECTION_REG1       MSDC_CLOCK_SELECTION_REG
#define MSDC_SCLOCK_SELECTION_REG0       0xF000D32C
#define MSDC_HCLOCK_SELECTION_REG0       0xF000D3A0
#define MSDC_HCLOCK_SELECTION_REG1       0xF000D384
#define MSDC_CLOCK_CKGEN_GATE_BIT       (0x1<<7)
#define MSDC_CLOCK_CKGEN_SLECTION_MASK  (0x0F<<0)
#define SD_DEFAULT_HIGH_CLOCK_INDEX 9
#define SD_DEFAULT_NORM_CLOCK_INDEX 10
#define SD_DEFAULT_INIT_CLOCK_INDEX 12
#endif
#endif

#if defined(CC_MT5399) || defined(CONFIG_ARCH_MT5399) || \
	defined(CC_MT5890) || defined(CONFIG_ARCH_MT5890) || \
	defined(CC_MT5891) || defined(CONFIG_ARCH_MT5891) || \
	defined(CC_MT5882) || defined(CONFIG_ARCH_MT5882) || \
	defined(CC_MT5883) || defined(CONFIG_ARCH_MT5883)
typedef struct
{
    UINT8 acName[32];
    UINT32 u4ID1;
    UINT32 u4ID2;
    UINT32 DS26Sample;
	UINT32 DS26Delay;
    UINT32 HS52Sample;
    UINT32 HS52Delay;
    UINT32 DDR52Sample;
    UINT32 DDR52Delay;
    UINT32 HS200Sample;
    UINT32 HS200Delay;
} EMMC_FLASH_DEV_T;
#else
typedef struct
{
    char acName[32];
    UINT32 u4ID1;
    UINT32 u4ID2;
    UINT32  u4Sample;
} EMMC_FLASH_DEV_T;
#endif

#if defined(CC_MT5396) || defined(CONFIG_ARCH_MT5396)
#define CONFIG_CHIP_VERSION         5396
#elif defined(CC_MT5398) || defined(CONFIG_ARCH_MT5398)
#define CONFIG_CHIP_VERSION         5398
#elif defined(CC_MT5880) || defined(CONFIG_ARCH_MT5880)
#define CONFIG_CHIP_VERSION         5880
#elif defined(CC_MT5399) || defined(CONFIG_ARCH_MT5399)
#define CONFIG_CHIP_VERSION         5399
#elif defined(CC_MT5890) || defined(CONFIG_ARCH_MT5890)
#define CONFIG_CHIP_VERSION         5890
#elif defined(CC_MT5891) || defined(CONFIG_ARCH_MT5891)
#define CONFIG_CHIP_VERSION         5891
#elif defined(CC_MT5882) || defined(CONFIG_ARCH_MT5882)
#define CONFIG_CHIP_VERSION         5882
#elif defined(CC_MT5883) || defined(CONFIG_ARCH_MT5883)
#define CONFIG_CHIP_VERSION         5883

#else
#define CONFIG_CHIP_VERSION         5398
#endif

#define CMD0_RESET_LIMIT            (3)
#define MMC_CMD1_RETRY_LIMIT        (1500)
#define SDHC_BLK_SIZE               (512)
#define MMC_DEF_RCA                 0x0001

#if defined(CONFIG_MT53XX_MAP_CHANNELB_DRAM) || defined(CONFIG_MT53XX_USE_CHANNELB_DRAM)
#define MTK_MSDC_DMA_MASK 0x1FFFFFFF
#else
#define MTK_MSDC_DMA_MASK 0xFFFFFFFF
#endif

#define MASK_CID0                     (0xFF00FFFF)
#define MASK_CID1                     (0xFFFFFFFF)
#define MASK_CID2                     (0xFFFF0000)
#define MASK_CID3                     (0x00000000)

//---------------------------------------------------------------------------
// MSDC Interrupt Number
//---------------------------------------------------------------------------

#define MISC2_EN_REG                                        (0xF0008034)
#define MISC2_EN_SHIFT                                      (30)
#define MSDC1_EN_REG                                        (0xF0008084)
#define MSDC1_EN_SHIFT                                      (4)
#define MSDC0_EN_REG                                        (0xF0008034)
#define MSDC0_EN_SHIFT                                      (28)

//---------------------------------------------------------------------------
// Macro definitions
//---------------------------------------------------------------------------

#define MSDC_RST_TIMEOUT_LIMIT_COUNT                        5         
#define MSDC_CLK_TIMEOUT_LIMIT_COUNT                        5 
#define MSDC_FIFOCLR_TIMEOUT_LIMIT_COUNT                    5   

#define MSDC_WAIT_SDC_BUS_TIMEOUT_LIMIT_COUNT               400                 
#define MSDC_WAIT_CMD_TIMEOUT_LIMIT_COUNT                   200                     
#define MSDC_WAIT_DATA_TIMEOUT_LIMIT_COUNT                  5000              
#define MSDC_WAIT_DATA_COMPLETE_TIMEOUT_LIMIT_COUNT         1000              

#define MSDC_WAIT_BTSTS_1_TIMEOUT_LIMIT_COUNT               20                     
#define MSDC_WAIT_BTSTS_0_TIMEOUT_LIMIT_COUNT               1000                   
#define MSDC_WAIT_EMMC_ACK_TIMEOUT_LIMIT_COUNT              60                 
#define MSDC_WAIT_EMMC_DATA_TIMEOUT_LIMIT_COUNT             1100 

#define EMMC_READ_CARD_FAIL                                 1
#define EMMC_MSDC_INIT_FAIL                                 2
#define EMMC_IDENT_CARD_FAIL                                3
#define EMMC_SUCCESS                                        0

//---------------------------------------------------------------------------
// MSDC error retry limitation
//---------------------------------------------------------------------------

#define MSDC_LOAD_MMC_IMAGE_RETRY_LIMIT               10
#define MSDC_SYS_INIT_RETRY_LIMIT                      3
#define MSDC_IDENTIFY_CARD_RETRY_LIMIT                10
#define MSDC_READ_CARD_RETRY_LIMIT                     6
#define MSDC_READ_BOOTLDR_DATA_RETRY_LIMIT             8

//---------------------------------------------------------------------------
// Type definitions
//---------------------------------------------------------------------------

#define MSDC_WRITE32(addr, value)      (*(volatile unsigned int *)(addr)) = (value)
#define MSDC_READ32(addr)              (*(volatile unsigned int *)(addr))

#define MSDC_SETBIT(addr, dBit)        MSDC_WRITE32(addr, MSDC_READ32(addr) | (dBit))
#define MSDC_CLRBIT(addr, dBit)        MSDC_WRITE32(addr, MSDC_READ32(addr) & (~(dBit)))

#define MSDC_BITVAL(addr, dBit)        ((MSDC_READ32(addr)>>dBit) & (0x01))             

#define ArraySize(a)              (sizeof(a)/sizeof(a[0]))
#define MAX_NUM(a, b)             ((a)>(b)?(a):(b))
#define MIN_NUM(a, b)             ((a)>(b)?(b):(a))

typedef enum
{
    MSDC0DetectGpio,
    MSDC0WriteProtectGpio,
    MSDC0PoweronoffDetectGpio,
    MSDC0VoltageSwitchGpio,
    MSDCbackup1Gpio,
    MSDCbackup2Gpio
}MSDCGpioNumber;
#if defined(CC_MT5399) || defined(CONFIG_ARCH_MT5399) || \
	defined(CC_MT5890) || defined(CONFIG_ARCH_MT5890) || \
	defined(CC_MT5891) || defined(CONFIG_ARCH_MT5891) || \
	defined(CC_MT5882) || defined(CONFIG_ARCH_MT5882) || \
	defined(CC_MT5883) || defined(CONFIG_ARCH_MT5883)
// Mustang ADIN7_SRV no interrupt function!
// Enable doule dege trigger
// MISC interrupt enable
// dedicater gpio edge interrupt
// PDWNC interrupt to ARM enable
#define SD_GPIO_INTERRUPT_EN   
// Switch Pinmux to GPIO Function
// Set gpio input mode
#define SD_CARD_DETECT_GPIO_SETTING      \
{                                        \
    MSDC_SETBIT(0xF00280B4, 0x1u<<31);    \
    MSDC_CLRBIT(0xF0028080, 0x1<<4);    \
}
// Clr GPIO interrupt status
#define SD_CARD_DETECT_GPIO_STS_CLR     
// Get GPIO interrupt status
#define SD_CARD_DETECT_GPIO_STS_GET      
// Get gpio value
#define SD_CARD_DETECT_GPIO              MSDC_BITVAL(0xF0028088, 4)

#else
// python key_pad0 no interrupt function!
// Enable doule dege trigger
// MISC interrupt enable
// dedicater gpio edge interrupt
// PDWNC interrupt to ARM enable
#define SD_GPIO_INTERRUPT_EN             \
{                                        \
    MSDC_SETBIT(0xF00280A8, 0x1<<31);    \
    MSDC_SETBIT(0xF0008034, 0x1<<31);    \
    MSDC_SETBIT(0xF000804C, 0x1<<1);     \
    MSDC_SETBIT(0xF0028044, 0x1<<31);    \
}
// Switch Pinmux to GPIO Function
// Set gpio input mode
#define SD_CARD_DETECT_GPIO_SETTING      \
{                                        \
    MSDC_SETBIT(0xF00280B4, 0x1<<30);    \
    MSDC_CLRBIT(0xF002807C, 0x1<<23);    \
}
// Clr GPIO interrupt status
#define SD_CARD_DETECT_GPIO_STS_CLR      MSDC_CLRBIT(0xF0028048, 0x1<<31)
// Get GPIO interrupt status
#define SD_CARD_DETECT_GPIO_STS_GET      MSDC_CLRBIT(0xF0028040, 0x1<<31)
// Get gpio value
#define SD_CARD_DETECT_GPIO              MSDC_BITVAL(0xF0028084, 23)
#endif
	
#define RESPONSE_NO			          (0)
#define RESPONSE_R1			          (1)
#define RESPONSE_R2			          (2)
#define RESPONSE_R3			          (3)
#define RESPONSE_R4			          (4)
#define RESPONSE_R5			          (5)
#define RESPONSE_R6			          (6)
#define RESPONSE_R7			          (7)
#define RESPONSE_R1B		          (8)

#define MSDC_SUCCESS                  (int)(0)
#define MSDC_FAILED                   (int)(-1)

#define CMD_ERR_SUCCESS               (MSDC_SUCCESS)
#define CMD_ERR_FAILED                (MSDC_FAILED)

#define CMD_ERR_NO_RESP               (0x01 << 0)
#define CMD_ERR_RESP_CRCERR           (0x01 << 1)
#define CMD_ERR_WAIT_CMD_TO           (0x01 << 2)

#define CMD_ERR_DATTO                 (0x01 << 3)
#define CMD_ERR_DATA_CRCERR           (0x01 << 4)
#define CMD_ERR_WAIT_DATATO           (0x01 << 5)
#define CMD_ERR_DATA_FAILED           (0x01 << 6)

#define ERR_CMD_FAILED                (0x01 << 1)
#define ERR_DAT_FAILED                (0x01 << 8)

#define ERR_HOST_BUSY                 (0x01 << 0)

#define ERR_CMD_FAILED                (0x01 << 1)
#define ERR_CMD_NO_INT                (0x01 << 2)
#define ERR_CMD_RESP_TIMEOUT          (0x01 << 3)
#define ERR_CMD_RESP_CRCERR           (0x01 << 4)
#define ERR_AUTOCMD_RESP_CRCERR       (0x01 << 5)
#define ERR_AUTOCMD_RESP_TIMEOUT      (0x01 << 6)
#define ERR_CMD_UNEXPECT_INT          (0x01 << 7)

#define ERR_DAT_FAILED                (0x01 << 8)
#define ERR_DAT_NO_INT                (0x01 << 9)
#define ERR_DAT_CRCERR                (0x01 << 10)   
#define ERR_DAT_TIMEOUT               (0x01 << 11)
#define ERR_DAT_UNEXPECT_INT          (0x01 << 12) 

#define ERR_DMA_STATUS_FAILED         (0x01 << 16)

//---------------------------------------------------------------------------
// Log system declaration
//---------------------------------------------------------------------------

#define MSG_LVL_OFF						(0)
#define MSG_LVL_FATAL					(1)
#define MSG_LVL_ERR						(2)
#define MSG_LVL_WARN					(3)
#define MSG_LVL_TITLE					(4)
#define MSG_LVL_INFO					(5)
#define MSG_LVL_CMD						(6)
#define MSG_LVL_DBG						(7)
#define MSG_LVL_TRACE					(8)

#define MSG_LVL_DISPLAY       (4)
#ifdef CC_MTK_ANDROID_AUTO_TEST
#define MSDC_LOG(level, formatStr...) \
if (level <= MSG_LVL_DISPLAY) {       \
    {printk(KERN_ERR formatStr);}     \
}
#else
#define MSDC_LOG(level, formatStr...) \
if (level <= MSG_LVL_DISPLAY) {       \
	if(printk_ratelimit())            \
    {printk(KERN_ERR formatStr);}     \
}
#endif

//---------------------------------------------------------------------------
// Struct declaration
//---------------------------------------------------------------------------

typedef struct __sd_cmd_t__ 
{
    UINT8 idx;
    UINT32 arg;
    UINT32 resp[4];		// Maximum Length Reserved
    UINT32 autoStopResp;	
    UINT32 dataMode;
    UINT32 buffLen;
    VOID *pBuff;				// Point to data buffer in PIO and basic DMA mode
} sd_cmd_t;

typedef struct __msdc_env_s__ 
{
    UINT8 fgHost;
    UINT8 cid[16];
    UINT8 rca[2];
    UINT8 csd[16];
    UINT8 ocr[4];
    //UINT8 ext_csd_185;
    //UINT8 ext_csd_192;
    //UINT8 ext_csd_196;
    //UINT8 ext_csd_212;
    //UINT8 ext_csd_213;
    //UINT8 ext_csd_214;
    //UINT8 ext_csd_215;
    UINT8 ext_csd_rsv[26];
} msdc_env_t;

typedef struct __sdhost_s__ 
{
    UINT32 busWidth;
    UINT32 blkLen;
    UINT32 speedMode;

    // Select polling mode or interrupt mode 
    UINT32 polling;   

    // Host Data Mode 
    UINT32 dataMode;  

    // Host Bus Clock
    UINT32 maxClock;
    UINT32 curClock;

    // DMA Burst Size 
    UINT32 dmaBstSize;
    // Configurable Maximum Basic DMA Len     
    UINT32 maxBasicDmaLen;    

    // Accumulated interrupt vector
    volatile UINT32 accuVect;

    // EXT_CSD
    UINT8 EXT_CSD[512];
    UINT32 fgUpdateExtCsd; 
} sdhost_t;

//---------------------------------------------------------------------------
// Host definition
//---------------------------------------------------------------------------

struct sdhci_msdc_pdata {
    UINT32 no_wprotect : 1;
    UINT32 no_detect : 1;
    UINT32 wprotect_invert : 1;
    UINT32 detect_invert : 1;   /* set => detect active high. */
    UINT32 use_dma : 1;

    UINT32 gpio_detect;
    UINT32 gpio_wprotect;
    UINT32 ocr_avail;
    void   (*set_power)(UINT8 power_mode,
                       UINT16 vdd);
 };

struct msdc_host
{
    struct mmc_host *mmc;                   /* MMC structure */
    struct platform_device	*pdev;
    void   *pdata;	/* Platform specific data, device core doesn't touch it */
    struct mmc_command cur_cmd;
    struct mmc_command pre_cmd;
    struct semaphore msdc_sema;       /* semaphore */
	
    spinlock_t lock;		/* Mutex */
    
    INT32 devIndex;
    
    UINT32 MsdcAccuVect;
    UINT32 msdc_isr_en;
    UINT32 desVect;
    UINT32 waitIntMode;
	UINT32 timeout_ns;
	UINT32 timeout_clks;

    INT32 flags;		/* Driver states */
#define SDHCI_FCARD_PRESENT	(1<<0)		/* Card is present */

    struct mmc_request*	mrq;		/* Current request */

    struct scatterlist*	cur_sg;		/* Current SG entry */
    UINT32		num_sg;		/* Number of entries left */

    UINT32		offset;		/* Offset into current entry */
    UINT32		remain;		/* Data left in curren entry */

    msdc_gpd_t *gpd;				  /* pointer to gpd array */		
    msdc_bd_t  *bd; 				  /* pointer to bd array */ 	   
    dma_addr_t gpd_addr; 		/* the physical address of gpd array */ 	   
    dma_addr_t bd_addr;			/* the physical address of bd array */
	
    UINT32      clk;		/* Current clock speed */
    UINT8		bus_width;	/* Current bus width */
    INT32 hispeedcard;          /* HiSpeed card or not */
   
    INT32 chip_id;	/* ID of controller */
    INT32 base;		/* I/O port base */
    INT32 irq;		    /* Interrupt irq number*/
    INT32 data_mode;    /* data transfer mode */

};

#endif // #ifndef _MSDC_DRV_H_
