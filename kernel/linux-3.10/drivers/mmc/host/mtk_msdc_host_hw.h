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
* [File]			msdc_host_hw.h
* [Version]			v1.0
* [Revision Date]	2011-05-04
* [Author]			Shunli Wang, shunli.wang@mediatek.inc, 82896, 2011-05-04
* [Description]
*	MSDC Host Controller Header File
* [Copyright]
*	Copyright (C) 2011 MediaTek Incorporation. All Rights Reserved.
******************************************************************************/

#ifndef _MSDC_HOST_HW_H_
#define _MSDC_HOST_HW_H_
#include "x_typedef.h"

//---------------------------------------------------------------------------
// Configurations
//---------------------------------------------------------------------------
#define MSDC_BASE_ADDR            (UINT32) (0xF0012000)
#define MSDC_VECTOR               (UINT32) (28)

//---------------------------------------------------------------------------
// MSDC Register definitions
//---------------------------------------------------------------------------
#define MSDC_CH_NUM               (UINT32) (2)
#define MSDC_CH_OFFSET            (UINT32) (0x0005B000)

#define MSDC_CFG(ch)              (MSDC_BASE_ADDR + (ch * MSDC_CH_OFFSET) + 0x00)
#define MSDC_IOCON(ch)            (MSDC_BASE_ADDR + (ch * MSDC_CH_OFFSET) + 0x04)
#define MSDC_PS(ch)               (MSDC_BASE_ADDR + (ch * MSDC_CH_OFFSET) + 0x08)
#define MSDC_INT(ch)              (MSDC_BASE_ADDR + (ch * MSDC_CH_OFFSET) + 0x0C)
#define MSDC_INTEN(ch)            (MSDC_BASE_ADDR + (ch * MSDC_CH_OFFSET) + 0x10)
#define MSDC_FIFOCS(ch)           (MSDC_BASE_ADDR + (ch * MSDC_CH_OFFSET) + 0x14)
#define MSDC_TXDATA(ch)           (MSDC_BASE_ADDR + (ch * MSDC_CH_OFFSET) + 0x18)
#define MSDC_RXDATA(ch)           (MSDC_BASE_ADDR + (ch * MSDC_CH_OFFSET) + 0x1C)
#define SDC_CFG(ch)               (MSDC_BASE_ADDR + (ch * MSDC_CH_OFFSET) + 0x30)
#define SDC_CMD(ch)               (MSDC_BASE_ADDR + (ch * MSDC_CH_OFFSET) + 0x34)
#define SDC_ARG(ch)               (MSDC_BASE_ADDR + (ch * MSDC_CH_OFFSET) + 0x38)
#define SDC_STS(ch)               (MSDC_BASE_ADDR + (ch * MSDC_CH_OFFSET) + 0x3C)
#define SDC_RESP0(ch)             (MSDC_BASE_ADDR + (ch * MSDC_CH_OFFSET) + 0x40)
#define SDC_RESP1(ch)             (MSDC_BASE_ADDR + (ch * MSDC_CH_OFFSET) + 0x44)
#define SDC_RESP2(ch)             (MSDC_BASE_ADDR + (ch * MSDC_CH_OFFSET) + 0x48)
#define SDC_RESP3(ch)             (MSDC_BASE_ADDR + (ch * MSDC_CH_OFFSET) + 0x4C)
#define SDC_BLK_NUM(ch)           (MSDC_BASE_ADDR + (ch * MSDC_CH_OFFSET) + 0x50)
#define SDC_CSTS(ch)              (MSDC_BASE_ADDR + (ch * MSDC_CH_OFFSET) + 0x58)
#define SDC_CSTS_EN(ch)           (MSDC_BASE_ADDR + (ch * MSDC_CH_OFFSET) + 0x5C)
#define SDC_DATCRC_STS(ch)        (MSDC_BASE_ADDR + (ch * MSDC_CH_OFFSET) + 0x60)
#define EMMC_CFG0(ch)             (MSDC_BASE_ADDR + (ch * MSDC_CH_OFFSET) + 0x70)
#define EMMC_CFG1(ch)             (MSDC_BASE_ADDR + (ch * MSDC_CH_OFFSET) + 0x74)
#define EMMC_STS(ch)              (MSDC_BASE_ADDR + (ch * MSDC_CH_OFFSET) + 0x78)
#define EMMC_IOCON(ch)            (MSDC_BASE_ADDR + (ch * MSDC_CH_OFFSET) + 0x7C)
#define ACMD_RESP(ch)             (MSDC_BASE_ADDR + (ch * MSDC_CH_OFFSET) + 0x80)
#define ACMD19_TRG(ch)            (MSDC_BASE_ADDR + (ch * MSDC_CH_OFFSET) + 0x84)
#define ACMD19_STS(ch)            (MSDC_BASE_ADDR + (ch * MSDC_CH_OFFSET) + 0x88)
#define DMA_SA(ch)                (MSDC_BASE_ADDR + (ch * MSDC_CH_OFFSET) + 0x90)
#define DMA_CA(ch)                (MSDC_BASE_ADDR + (ch * MSDC_CH_OFFSET) + 0x94)
#define DMA_CTRL(ch)              (MSDC_BASE_ADDR + (ch * MSDC_CH_OFFSET) + 0x98)
#define DMA_CFG(ch)               (MSDC_BASE_ADDR + (ch * MSDC_CH_OFFSET) + 0x9C)
#define DBG_SEL(ch)               (MSDC_BASE_ADDR + (ch * MSDC_CH_OFFSET) + 0xA0)
#define DBG_OUT(ch)               (MSDC_BASE_ADDR + (ch * MSDC_CH_OFFSET) + 0xA4)
#if defined(CC_MT5399) || defined(CONFIG_ARCH_MT5399) || \
    defined(CC_MT5890) || defined(CONFIG_ARCH_MT5890) || \
    defined(CC_MT5891) || defined(CONFIG_ARCH_MT5891) || \
    defined(CC_MT5882) || defined(CONFIG_ARCH_MT5882) || \
    defined(CC_MT5883) || defined(CONFIG_ARCH_MT5883)
#define DMA_LENGTH(ch)            (MSDC_BASE_ADDR + (ch * MSDC_CH_OFFSET) + 0xA8)
#endif
#if defined(CC_MT5399) || defined(CONFIG_ARCH_MT5399) || \
	defined(CC_MT5890) || defined(CONFIG_ARCH_MT5890) || \
	defined(CC_MT5891) || defined(CONFIG_ARCH_MT5891) || \
	defined(CC_MT5882) || defined(CONFIG_ARCH_MT5882) || \
	defined(CC_MT5883) || defined(CONFIG_ARCH_MT5883)
#define PATCH_BIT0(ch)            (MSDC_BASE_ADDR + (ch * MSDC_CH_OFFSET) + 0xB0)
#define PATCH_BIT1(ch)            (MSDC_BASE_ADDR + (ch * MSDC_CH_OFFSET) + 0xB4)
#else
#define PATCH_BIT(ch)             (MSDC_BASE_ADDR + (ch * MSDC_CH_OFFSET) + 0xB0)
#endif
#if defined(CC_MT5399) || defined(CONFIG_ARCH_MT5399) || \
	defined(CC_MT5890) || defined(CONFIG_ARCH_MT5890) || \
	defined(CC_MT5891) || defined(CONFIG_ARCH_MT5891) || \
	defined(CC_MT5882) || defined(CONFIG_ARCH_MT5882) || \
	defined(CC_MT5883) || defined(CONFIG_ARCH_MT5883)
#define SD30_PAD_CTL0(ch)         (MSDC_BASE_ADDR + (ch * MSDC_CH_OFFSET) + 0xE0)
#define SD30_PAD_CTL1(ch)         (MSDC_BASE_ADDR + (ch * MSDC_CH_OFFSET) + 0xE4)
#define SD30_PAD_CTL2(ch)         (MSDC_BASE_ADDR + (ch * MSDC_CH_OFFSET) + 0xE8)
#else
#define SD20_PAD_CTL0(ch)         (MSDC_BASE_ADDR + (ch * MSDC_CH_OFFSET) + 0xE0)
#define SD20_PAD_CTL1(ch)         (MSDC_BASE_ADDR + (ch * MSDC_CH_OFFSET) + 0xE4)
#define SD20_PAD_CTL2(ch)         (MSDC_BASE_ADDR + (ch * MSDC_CH_OFFSET) + 0xE8)
#endif
#define GPIO_DBG_OUT(ch)          (MSDC_BASE_ADDR + (ch * MSDC_CH_OFFSET) + 0xEB)
#define PAD_TUNE(ch)              (MSDC_BASE_ADDR + (ch * MSDC_CH_OFFSET) + 0xEC)
#define DAT_RD_DLY0(ch)           (MSDC_BASE_ADDR + (ch * MSDC_CH_OFFSET) + 0xF0)
#define DAT_RD_DLY1(ch)           (MSDC_BASE_ADDR + (ch * MSDC_CH_OFFSET) + 0xF4)
#define HW_DBG(ch)                (MSDC_BASE_ADDR + (ch * MSDC_CH_OFFSET) + 0xF8)
#define VERSION(ch)               (MSDC_BASE_ADDR + (ch * MSDC_CH_OFFSET) + 0xFC)
#define ECO_VER(ch)               (MSDC_BASE_ADDR + (ch * MSDC_CH_OFFSET) + 0x104)

/* MSDC_CFG */
#define MSDC_CFG_SD                     (((UINT32)0x01) << 0)
#define MSDC_CFG_CK_EN                  (((UINT32)0x01) << 1)
#define MSDC_CFG_RST                    (((UINT32)0x01) << 2)
#define MSDC_CFG_PIO_MODE               (((UINT32)0x01) << 3)
#define MSDC_CFG_BUS_VOL_START          (((UINT32)0x01) << 5)
#define MSDC_CFG_BUS_VOL_PASS           (((UINT32)0x01) << 6)
#define MSDC_CFG_CARD_CK_STABLE         (((UINT32)0x01) << 7)
#define MSDC_CFG_CK_DIV_SHIFT           (8)
#define MSDC_CFG_CK_MODE_DIVIDER        (((UINT32)0x00) << 16)
#define MSDC_CFG_CK_MODE_DIRECT         (((UINT32)0x01) << 16)
#define MSDC_CFG_CK_MODE_DDR            (((UINT32)0x02) << 16)

/* MSDC_IOCON */
#define MSDC_IOCON_RISC_SIZE_MASK       (((UINT32)0x03) << 24)
#define MSDC_IOCON_RISC_SIZE_BYTE       (((UINT32)0x00) << 24)
#define MSDC_IOCON_RISC_SIZE_WORD       (((UINT32)0x01) << 24)
#define MSDC_IOCON_RISC_SIZE_DWRD       (((UINT32)0x02) << 24)
#define MSDC_IOCON_R_SMPL_SHIFT         (1)    
#define MSDC_IOCON_R_SMPL               (((UINT32)0x01) << MSDC_IOCON_R_SMPL_SHIFT)
#define MSDC_IOCON_D_SMPL_SHIFT         (2) 
#define MSDC_IOCON_D_SMPL               (((UINT32)0x01) << MSDC_IOCON_D_SMPL_SHIFT)
#define MSDC_IOCON_D_DLYLINE_SEL_SHIFT  (3)
#define MSDC_IOCON_D_DLYLINE_SEL        (((UINT32)0x01) << MSDC_IOCON_D_DLYLINE_SEL_SHIFT)
#if defined(CC_MT5399) || defined(CONFIG_ARCH_MT5399) || \
	defined(CC_MT5890) || defined(CONFIG_ARCH_MT5890) || \
	defined(CC_MT5891) || defined(CONFIG_ARCH_MT5891) || \
	defined(CC_MT5882) || defined(CONFIG_ARCH_MT5882) || \
	defined(CC_MT5883) || defined(CONFIG_ARCH_MT5883)
#define MSDC_IOCON_W_D_SMPL_SHIFT       (8)
#define MSDC_IOCON_W_D_SMPL_SEL         (((UINT32)0x01) << MSDC_IOCON_W_D_SMPL_SHIFT)
#endif

/* MSDC_INT */
#define INT_MMC_IRQ                     (((UINT32)0x01) << 0)           
#define INT_MSDC_CDSC                   (((UINT32)0x01) << 1) 
#define INT_SD_AUTOCMD_RDY              (((UINT32)0x01) << 3) 
#define INT_SD_AUTOCMD_TO               (((UINT32)0x01) << 4) 
#define INT_SD_AUTOCMD_RESP_CRCERR      (((UINT32)0x01) << 5) 
#define INT_DMA_Q_EMPTY                 (((UINT32)0x01) << 6) 
#define INT_SD_SDIOIRQ                  (((UINT32)0x01) << 7) 
#define INT_SD_CMDRDY                   (((UINT32)0x01) << 8) 
#define INT_SD_CMDTO                    (((UINT32)0x01) << 9) 
#define INT_SD_RESP_CRCERR              (((UINT32)0x01) << 10)
#define INT_SD_CSTA                     (((UINT32)0x01) << 11)
#define INT_SD_XFER_COMPLETE            (((UINT32)0x01) << 12)
#define INT_DMA_XFER_DONE               (((UINT32)0x01) << 13)
#define INT_SD_DATTO                    (((UINT32)0x01) << 14)
#define INT_SD_DATA_CRCERR              (((UINT32)0x01) << 15)
#define INT_MS_RDY                      (((UINT32)0x01) << 24)
#define INT_MS_SIF                      (((UINT32)0x01) << 25)
#define INT_MS_TOER                     (((UINT32)0x01) << 26)
#define INT_MS_CRCERR                   (((UINT32)0x01) << 27)
#define INT_MS_CED                      (((UINT32)0x01) << 28)
#define INT_MS_ERR                      (((UINT32)0x01) << 29)
#define INT_MS_BREQ                     (((UINT32)0x01) << 30)
#define INT_CMDNK                       (((UINT32)0x01) << 31)

#define INT_SD_ALL                      (INT_SD_DATA_CRCERR | INT_SD_DATTO | INT_DMA_XFER_DONE | INT_SD_XFER_COMPLETE | \
                                         INT_SD_RESP_CRCERR | INT_SD_CMDTO | INT_SD_CMDRDY | INT_SD_AUTOCMD_RDY | \
                                         INT_SD_AUTOCMD_TO | INT_SD_AUTOCMD_RESP_CRCERR | INT_DMA_Q_EMPTY)

/* SDC_CFG */
#define SDC_CFG_BW_SHIFT                (16)         
#define SDC_CFG_BW_MASK                 (((UINT32)0x03) << 16) 
#define SDC_CFG_SDIO                    (((UINT32)0x01) << 19) 
#define SDC_CFG_INTAT_BLK_GAP           (((UINT32)0x01) << 21) 
#define SDC_CFG_DTOC_SHIFT              (24)         

/* SDC_STS */                                        
#define SDC_STS_SDCBUSY                 (((UINT32)0x01) << 0x0)  
#define SDC_STS_CMDBUSY                 (((UINT32)0x01) << 0x1)  
                                 
/* SDC_CMD */                                        
#define SDC_CMD_BREAK                   (((UINT32)0x01) << 6)  

/* Response Type */                                                     
#define SDC_CMD_RSPTYPE_NO              (((UINT32)0x00) << 7)  
#define SDC_CMD_RSPTYPE_R1              (((UINT32)0x01) << 7)  
#define SDC_CMD_RSPTYPE_R2              (((UINT32)0x02) << 7)  
#define SDC_CMD_RSPTYPE_R3              (((UINT32)0x03) << 7)  
#define SDC_CMD_RSPTYPE_R4              (((UINT32)0x04) << 7)  
#define SDC_CMD_RSPTYPE_R5              (((UINT32)0x01) << 7)  
#define SDC_CMD_RSPTYPE_R6              (((UINT32)0x01) << 7)  
#define SDC_CMD_RSPTYPE_R7              (((UINT32)0x01) << 7)  
#define SDC_CMD_RSPTYPE_R1B             (((UINT32)0x07) << 7)  

/* Data Type */                                                     
#define DTYPE_NONE                      (((UINT32)0x00) << 11) 
#define DTYPE_SINGLE_BLK                (((UINT32)0x01) << 11) 
#define DTYPE_MULTI_BLK                 (((UINT32)0x02) << 11) 
#define DTYPE_STREAM                    (((UINT32)0x03) << 11) 
                                                     
#define SDC_CMD_READ                    (((UINT32)0x00) << 13) 
#define SDC_CMD_WRITE                   (((UINT32)0x01) << 13) 
                                                     
#define SDC_CMD_STOP                    (((UINT32)0x01) << 14) 
                                                     
#define SDC_CMD_GO_IRQ                  (((UINT32)0x01) << 15) 
                                                     
#define SDC_CMD_LEN_SHIFT               (16)         
                                                     
#define SDC_CMD_AUTO_CMD_NONE           (((UINT32)0x0) << 28)  
#define SDC_CMD_AUTO_CMD12              (((UINT32)0x1) << 28)  
#define SDC_CMD_ATUO_CMD23              (((UINT32)0x2) << 28)  
#define SDC_CMD_AUTO_CMD19              (((UINT32)0x3) << 28)  
                                                     
#define SDC_CMD_VOL_SWITCH              (((UINT32)0x1) << 30)  
                                                     
/* MSDC_FIFOCS */                                    
#define MSDC_FIFOCS_FIFOCLR             (((UINT32)0x1) << 31)  
#define MSDC_FIFO_LEN                   (128)        
#define MSDC_FIFOCS_TXFIFOCNT_SHIFT     (16)         
#define MSDC_FIFOCS_TXFIFOCNT_MASK      (0x00FF0000) 
#define MSDC_FIFOCS_RXFIFOCNT_SHIFT     (0)          
#define MSDC_FIFOCS_RXFIFOCNT_MASK      (0x000000FF)

/* SDC_DATCRC_STS */
#define SDC_DATCRC_STS_POS              (((unsigned int)0xFF) << 0)    
#define SDC_DATCRC_STS_NEG              (((unsigned int)0x0F) << 8) 

/* DMA_CTRL */
#define DMA_CTRL_BST_SHIFT              (12)
#define DMA_CTRL_BST_8                  (3 << DMA_CTRL_BST_SHIFT)
#define DMA_CTRL_BST_16                 (4 << DMA_CTRL_BST_SHIFT)
#define DMA_CTRL_BST_32                 (5 << DMA_CTRL_BST_SHIFT)
#define DMA_CTRL_BST_64                 (6 << DMA_CTRL_BST_SHIFT)
#define DMA_CTRL_START                  (((UINT32)0x01) << 0)
#define DMA_CTRL_STOP                   (((UINT32)0x01) << 1)
#define DMA_CTRL_RESUME                 (((UINT32)0x01) << 2)
#define DMA_CTRL_LAST_BUF               (((UINT32)0x01) << 10)
#define DMA_CTRL_XFER_SIZE_SHIFT        (16)
#define DMA_CTRL_DESC_MODE              (((UINT32)0x01) << 8)

/* DMA_CFG */
#define DMA_CFG_DMA_STATUS              (((UINT32)0x01) << 0)
#define DMA_CFG_CHKSUM                  (((UINT32)0x01) << 1)
#define DMA_CFG_BD_CS_ERR               (((UINT32)0x01) << 4)
#define DMA_CFG_GPD_CS_ERR              (((UINT32)0x01) << 5)

/* PAD_TUNE */
#define PAD_CMD_RESP_RXDLY_SHIFT        (22)
#define PAD_CMD_RESP_RXDLY              (((unsigned int)0x1F) << PAD_CMD_RESP_RXDLY_SHIFT)
#define PAD_DAT_RD_RXDLY_SHIFT          (8)
#define PAD_DAT_RD_RXDLY                (((unsigned int)0x1F) << PAD_DAT_RD_RXDLY_SHIFT)
#define PAD_DAT_WR_RXDLY_SHIFT          (0)
#define PAD_DAT_WR_RXDLY                (((unsigned int)0x1F) << PAD_DAT_WR_RXDLY_SHIFT)

/* EMMC_CFG0 */
#define BTSUP                           (((UINT32)0x01) << 15)
#define BTWDLY_0x32                     (((UINT32)0x00) << 12)
#define BTWDLY_1x32                     (((UINT32)0x01) << 12)
#define BTWDLY_2x32                     (((UINT32)0x02) << 12)
#define BTWDLY_7x32                     (((UINT32)0x07) << 12)
#define BTACHKDIS                       (((UINT32)0x01) << 3)
#define BTMOD_0                         (((UINT32)0x00) << 2)
#define BTMOD_1                         (((UINT32)0x01) << 2)
#define BTSTOP                          (((UINT32)0x01) << 1)
#define BTSTART                         (((UINT32)0x01) << 0)

/* EMMC_CFG1 */
#define BTATOC_SHIFT                    (20)
#define BTDTOC_SHIFT                    (0)

/* EMMC_STS */
#define BTDRCV                          (((UINT32)0x01) << 6)
#define BTARCV                          (((UINT32)0x01) << 5)
#define BTSTS                           (((UINT32)0x01) << 4)
#define BTATO                           (((UINT32)0x01) << 3)
#define BTDTO                           (((UINT32)0x01) << 2)
#define BTAERR                          (((UINT32)0x01) << 1)
#define BTDERR                          (((UINT32)0x01) << 0)

#if defined(CC_MT5399) || defined(CONFIG_ARCH_MT5399) || \
	defined(CC_MT5890) || defined(CONFIG_ARCH_MT5890) || \
	defined(CC_MT5891) || defined(CONFIG_ARCH_MT5891) || \
	defined(CC_MT5882) || defined(CONFIG_ARCH_MT5882) || \
	defined(CC_MT5883) || defined(CONFIG_ARCH_MT5883)
/* PATCH_BIT0 */
#define CKGEN_MSDC_DLY_SEL_SHIFT        (10)
#define CKGEN_MSDC_DLY_SEL              (((UINT32)0x1F) << CKGEN_MSDC_DLY_SEL_SHIFT)
#define INT_DAT_LATCH_CK_SEL_SHIFT      (7)
#define INT_DAT_LATCH_CK_SEL            (((UINT32)0x7)  << INT_DAT_LATCH_CK_SEL_SHIFT)
/* PATCH_BIT1 */
#define WRDAT_CRCS_TA_CNTR_SHIFT        (0)
#define WRDAT_CRCS_TA_CNTR              (((UINT32)0x7) << WRDAT_CRCS_TA_CNTR_SHIFT)
#define CMD_RSP_TA_CNTR_SHIFT           (3)
#define CMD_RSP_TA_CNTR                 (((UINT32)0x7) << CMD_RSP_TA_CNTR_SHIFT)
#else
/* PATCH_BIT */
#define R1B_DELAY_CYCLE                 (((UINT32)0x0F) << 18)
#endif

/* 8bit Mode */
#define SUPPORT_8_BIT					(1)

/* Host OCR */
#define HOST_OCR                (0x00FF8000)
#define HOST_MMC_OCR            (0x403C0000)
#define HOST_HCS                (1)
#define HOST_VHS                (0x1)

/* Transaction Type */
#define NULL_DATA_MODE          (0)
#define PIO_DATA_MODE           (1)
#define BASIC_DMA_DATA_MODE     (2)
#define DESC_DMA_DATA_MODE      (3)
#define ENHANCED_DMA_DATA_MODE  (4)

/* DMA Burst */
#define DMA_BST_8               (3)
#define DMA_BST_16              (4)
#define DMA_BST_32              (5)
#define DMA_BST_64              (6)

/* timeout related */
#define DEFAULT_DTOC        (40)      /* data timeout counter. 65536x40 sclk. */
#define CMD_TIMEOUT         (HZ/10)     /* 100ms */
#define DAT_TIMEOUT         (HZ/2 * 5)  /* 500ms x5 */

/* DMA Data Structure Constraint*/
#define BASIC_DMA_MAX_LEN               (0xFFFFFFFF)
#define GPD_MAX_LEN                     (0xFFFF)
#define BD_MAX_LEN                      (0xFFFF)
#define MAX_BD_NUM                      (1024)
#define MAX_BD_PER_GPD                  (MAX_BD_NUM)
#define MAX_GPD_PER_QUEUE               (MAX_BD_NUM)
#define MAX_DMA_CNT                     (64 * 1024 - 512)
#define MAX_HW_SGMTS                    (MAX_BD_NUM)
#define MAX_PHY_SGMTS                   (MAX_BD_NUM)
#define MAX_SGMT_SZ                     (MAX_DMA_CNT)
#define MAX_REQ_SZ                      (MAX_SGMT_SZ * 8)
#define MAX_GPD_NUM                     (1 + 1)

/* DMA Data Structure */
struct __msdc_gpd_s__
{
	UINT32 hwo	:1;
	UINT32 bdp	:1;
	UINT32 resv0	:6;
	UINT32 chksum	:8;
	UINT32 intr	:1;
	UINT32 resv1	:15;

	void *pNext;
	void *pBuff;

	UINT32 buffLen:16;
	UINT32 extLen	:8;
	UINT32 resv3	:8;

	// Enhanced DMA fields
	UINT32 arg;
	UINT32 blkNum;
	UINT32 cmd;
};
typedef struct __msdc_gpd_s__ msdc_gpd_t;

struct __msdc_bd_s__
{	
	UINT32 eol	:1;
	UINT32 resv0	:7;
	UINT32 chksum	:8;
	UINT32 resv1	:1;
	UINT32 blkPad	:1;
	UINT32 dwPad	:1;			// DWORD is 4 bytes
	UINT32 resv2	:13;

	void *pNext;
	void *pBuff;

	UINT32 buffLen:16;
	UINT32 resv3	:16;
};
typedef struct __msdc_bd_s__ msdc_bd_t;

#endif // #ifndef _MSDC_HOST_HW_H_
