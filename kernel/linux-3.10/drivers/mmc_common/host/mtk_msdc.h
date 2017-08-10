/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 */
/* MediaTek Inc. (C) 2010. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
 * AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek Software")
 * have been modified by MediaTek Inc. All revisions are subject to any receiver's
 * applicable license agreements with MediaTek Inc.
 */
#ifndef __MT_SD_H__
#define __MT_SD_H__

#include <linux/bitops.h>
#include <linux/mmc/host.h>
#include <linux/mmc/card.h>
#include <linux/semaphore.h>
#include "card/queue.h"
#ifdef CONFIG_64BIT
#include <linux/soc/mediatek/mt53xx_linux.h>
#else
#include <mach/mt53xx_linux.h>
#endif
/* writel etc. */
#include <linux/io.h>
#include <asm/barrier.h>

/*--------------------------------------------------------------------------*/
/* Common Macro                                                             */
/*--------------------------------------------------------------------------*/
#ifdef CONFIG_OF
#define MSDC_USE_OF
#endif

/* for MT5891 HS200 & HS400 defined in kernel config
 * for others determined by CC_EMMC_HS200
 */
#if defined(CONFIG_EMMC_HS200_SUPPORT) || defined(CC_EMMC_HS200)
#define MSDC_SUPPORT_HS200
#endif

#ifdef CONFIG_ARCH_MT5891
#define MSDC_ONLY_USE_ONE_CLKSRC
#define MSDC_SAVE_AUTOK_INFO
#endif

#ifndef MSDC_USE_OF
#if defined(CONFIG_ARCH_MT5883)
#define MSDC_0_BASE		0xF0012000
#define MSDC_1_BASE		0xF0012000
#else
#define MSDC_0_BASE		0xF0012000
#define MSDC_1_BASE		0xF006D000
#endif
#endif
#define MSDC_CLK_S_REG1	0xF000D380
#define MSDC_CLK_S_REG0	0xF000D32C
#define MSDC_CLK_H_REG1	0xF000D384
#define MSDC_CLK_H_REG0	0xF000D3A0

#define MSDC_SDCARD_FLAG  (MSDC_SYS_SUSPEND | MSDC_REMOVABLE | MSDC_HIGHSPEED | MSDC_UHS1)
//Please enable/disable SD card MSDC_CD_PIN_EN for customer request
#ifdef MSDC_SUPPORT_HS200
#ifdef CONFIG_EMMC_HS400_SUPPORT
#define MSDC_EMMC_FLAG	  (MSDC_SYS_SUSPEND | MSDC_HIGHSPEED | MSDC_UHS1 | MSDC_HS400)
#else
#define MSDC_EMMC_FLAG	  (MSDC_SYS_SUSPEND | MSDC_HIGHSPEED | MSDC_UHS1)
#endif
#else /* MSDC_SUPPORT_HS200 */
#define MSDC_EMMC_FLAG	  (MSDC_SYS_SUSPEND | MSDC_HIGHSPEED)
#endif /* MSDC_SUPPORT_HS200 */

#ifdef CC_MTD_ENCRYPT_SUPPORT
extern mtk_partition mtk_msdc_partition;
#define AES_ADDR_ALIGN   0x40
#define AES_LEN_ALIGN    0x20
#define AES_BUFF_LEN     0x10000
#endif

#define CC_EMMC_KDELAY
#ifdef CC_EMMC_KDELAY
#define MT5890_TT_KDELAY  62
#define MT5861_TT_KDELAY  70
#endif

#define REQ_CMD_EIO  (0x1 << 0)
#define REQ_CMD_TMO  (0x1 << 1)
#define REQ_DAT_ERR  (0x1 << 2)
#define REQ_STOP_EIO (0x1 << 3)
#define REQ_STOP_TMO (0x1 << 4)

#define MAX_GPD_NUM         (1 + 1)  /* one null gpd */
#define MAX_BD_NUM          (1024)
#define MAX_BD_PER_GPD      (MAX_BD_NUM)
#define HOST_MAX_NUM        (2)
#define CLK_SRC_MAX_NUM		(1)

#define MSDC_GPIO_MAX_NUMBERS 6

#define CUST_EINT_POLARITY_LOW              0
#define CUST_EINT_POLARITY_HIGH             1
#define CUST_EINT_DEBOUNCE_DISABLE          0
#define CUST_EINT_DEBOUNCE_ENABLE           1
#define CUST_EINT_EDGE_SENSITIVE            0
#define CUST_EINT_LEVEL_SENSITIVE           1
//////////////////////////////////////////////////////////////////////////////

#define EINT_MSDC2_INS_NUM              14
#define EINT_MSDC2_INS_DEBOUNCE_CN      1
#define EINT_MSDC2_INS_POLARITY         CUST_EINT_POLARITY_LOW
#define EINT_MSDC2_INS_SENSITIVE        CUST_EINT_LEVEL_SENSITIVE
#define EINT_MSDC2_INS_DEBOUNCE_EN      CUST_EINT_DEBOUNCE_ENABLE

#define EINT_MSDC1_INS_NUM              15
#define EINT_MSDC1_INS_DEBOUNCE_CN      1
#define EINT_MSDC1_INS_POLARITY         CUST_EINT_POLARITY_LOW
#define EINT_MSDC1_INS_SENSITIVE        CUST_EINT_LEVEL_SENSITIVE
#define EINT_MSDC1_INS_DEBOUNCE_EN      CUST_EINT_DEBOUNCE_ENABLE

#define MSDC_DESENSE_REG	(0xf0007070)

#define MSDC_AUTOCMD12          (0x0001)
#define MSDC_AUTOCMD23          (0x0002)
#define MSDC_AUTOCMD19          (0x0003)

#define REG_ADDR(x)	((volatile u32*)((unsigned long)base + OFFSET_##x))

#define UNSTUFF_BITS(resp,start,size) \
	({ \
		const int __size = size; \
		const u32 __mask = (__size < 32 ? 1 << __size : 0) - 1; \
		const int __off = 3 - ((start) / 32); \
		const int __shft = (start) & 31; \
		u32 __res; \
		__res = resp[__off] >> __shft; \
		if (__size + __shft > 32) \
			__res |= resp[__off-1] << ((32 - __shft) % 32); \
		__res & __mask; \
	})

#if defined(MSDC_SUPPORT_HS200) || defined(CONFIG_EMMC_HS400_SUPPORT)
#define HOST_MAX_MCLK       (200000000)
#else
#define HOST_MAX_MCLK       (50000000)
#endif

#define HOST_MIN_MCLK       (260000)
#define HOST_MAX_BLKSZ      (2048)
#define MSDC_OCR_AVAIL      (MMC_VDD_28_29 | MMC_VDD_29_30 | MMC_VDD_30_31 | MMC_VDD_31_32 | MMC_VDD_32_33)

#define DEFAULT_DTOC        (0xff) //to check. follow dtv set to max
#define CMD_TIMEOUT         (HZ / 10 * 5)   /* 100ms x5 */
#define DAT_TIMEOUT         (HZ * 5)   /* 1000ms x5 */
#define POLLING_BUSY		(HZ * 3)
#if defined(CONFIG_ARCH_MT5891)
#define MAX_DMA_CNT			(0xffffffU & ~511)
#else
#define MAX_DMA_CNT			(0xffffU & ~511)
#endif

#define MAX_HW_SGMTS        (MAX_BD_NUM)
#define MAX_PHY_SGMTS       (MAX_BD_NUM)
#define MAX_SGMT_SZ         (MAX_DMA_CNT)
#define MAX_REQ_SZ          (512 * 1024)

#define MAX_CKGEN_TUNE_TIME    10
#define CMD_TUNE_UHS_MAX_TIME 	(2*32*8*8)
#define CMD_TUNE_HS_MAX_TIME 	(2*32)

#define READ_TUNE_UHS_CLKMOD1_MAX_TIME 	(2*32*32*8)
#define READ_TUNE_UHS_MAX_TIME 	(2*32*32)
#define READ_TUNE_HS_MAX_TIME 	(2*32)

#define WRITE_TUNE_HS_MAX_TIME 	(2*32)
#define WRITE_TUNE_UHS_MAX_TIME (2*32*8)

//=================================
#define MSDC_LOWER_FREQ
#define MSDC_MAX_TIMEOUT_RETRY	(1)
#define MSDC_MAX_W_TIMEOUT_TUNE (5)
#define MSDC_MAX_R_TIMEOUT_TUNE	(3)

#define REQ_CMD_EIO  (0x1 << 0)
#define REQ_CMD_TMO  (0x1 << 1)
#define REQ_DAT_ERR  (0x1 << 2)
#define REQ_STOP_EIO (0x1 << 3)
#define REQ_STOP_TMO (0x1 << 4)

/*--------------------------------------------------------------------------*/
/* Common Definition                                                        */
/*--------------------------------------------------------------------------*/
#define MTK_MSDC_DMA_MASK 0x1FFFFFFF

#define MSDC_FIFO_SZ            (128)
#define MSDC_FIFO_THD_1K		(1024)
#define MSDC_FIFO_THD           (64)	/* (128) */
#define MSDC_NUM                (4)

#define MSDC_MS                 (0)	/* No memory stick mode, 0 use to gate clock */
#define MSDC_SDMMC              (1)

#define MSDC_MODE_UNKNOWN       (0)
#define MSDC_MODE_PIO           (1)
#define MSDC_MODE_DMA_BASIC     (2)
#define MSDC_MODE_DMA_DESC      (3)
#define MSDC_MODE_DMA_ENHANCED  (4)
#define MSDC_MODE_MMC_STREAM    (5)

#define MSDC_BUS_1BITS          (0)
#define MSDC_BUS_4BITS          (1)
#define MSDC_BUS_8BITS          (2)

#define MSDC_BRUST_8B           (3)
#define MSDC_BRUST_16B          (4)
#define MSDC_BRUST_32B          (5)
#define MSDC_BRUST_64B          (6)

#define MSDC_PIN_PULL_NONE      (0)
#define MSDC_PIN_PULL_DOWN      (1)
#define MSDC_PIN_PULL_UP        (2)
#define MSDC_PIN_KEEP           (3)

#define MSDC_AUTOCMD12          (0x0001)
#define MSDC_AUTOCMD23          (0x0002)
#define MSDC_AUTOCMD19          (0x0003)

#define MSDC_EMMC_BOOTMODE0     (0)     /* Pull low CMD mode */
#define MSDC_EMMC_BOOTMODE1     (1)     /* Reset CMD mode */

/*--------------------------------------------------------------------------*/
/* Register Offset                                                          */
/*--------------------------------------------------------------------------*/
#define OFFSET_MSDC_CFG         (0x0)
#define OFFSET_MSDC_IOCON       (0x04)
#define OFFSET_MSDC_PS          (0x08)
#define OFFSET_MSDC_INT         (0x0c)
#define OFFSET_MSDC_INTEN       (0x10)
#define OFFSET_MSDC_FIFOCS      (0x14)
#define OFFSET_MSDC_TXDATA      (0x18)
#define OFFSET_MSDC_RXDATA      (0x1c)
#define OFFSET_SDC_CFG          (0x30)
#define OFFSET_SDC_CMD          (0x34)
#define OFFSET_SDC_ARG          (0x38)
#define OFFSET_SDC_STS          (0x3c)
#define OFFSET_SDC_RESP0        (0x40)
#define OFFSET_SDC_RESP1        (0x44)
#define OFFSET_SDC_RESP2        (0x48)
#define OFFSET_SDC_RESP3        (0x4c)
#define OFFSET_SDC_BLK_NUM      (0x50)
#define OFFSET_SDC_VOL_CHG      (0x54)
#define OFFSET_SDC_CSTS         (0x58)
#define OFFSET_SDC_CSTS_EN      (0x5c)
#define OFFSET_SDC_DCRC_STS     (0x60)
#define OFFSET_SDC_CMD_STS      (0x64)
#define OFFSET_EMMC_CFG0        (0x70)
#define OFFSET_EMMC_CFG1        (0x74)
#define OFFSET_EMMC_STS         (0x78)
#define OFFSET_EMMC_IOCON       (0x7c)
#define OFFSET_SDC_ACMD_RESP    (0x80)
#define OFFSET_SDC_ACMD19_TRG   (0x84)
#define OFFSET_SDC_ACMD19_STS   (0x88)
#define OFFSET_MSDC_DMA_SA_H4B  (0x8C)
#define OFFSET_MSDC_DMA_SA      (0x90)
#define OFFSET_MSDC_DMA_CA      (0x94)
#define OFFSET_MSDC_DMA_CTRL    (0x98)
#define OFFSET_MSDC_DMA_CFG     (0x9c)
#define OFFSET_MSDC_DBG_SEL     (0xa0)
#define OFFSET_MSDC_DBG_OUT     (0xa4)
#define OFFSET_MSDC_DMA_LEN     (0xa8)
#define OFFSET_MSDC_PATCH_BIT0  (0xb0)
#define OFFSET_MSDC_PATCH_BIT1  (0xb4)
#define OFFSET_MSDC_PATCH_BIT2  (0xb8)
/* Following 6 definitions are only for SD card using */
#define OFFSET_DAT0_TUNE_CRC             (0xc0)
#define OFFSET_DAT1_TUNE_CRC    (0xc4)
#define OFFSET_DAT2_TUNE_CRC    (0xc8)
#define OFFSET_DAT3_TUNE_CRC    (0xcc)
#define OFFSET_CMD_TUNE_CRC     (0xd0)
#define OFFSET_SDIO_TUNE_WIND   (0xd4)
#define OFFSET_MSDC_PAD_CTL0    (0xe0)
#define OFFSET_MSDC_PAD_CTL1    (0xe4)
#define OFFSET_MSDC_PAD_CTL2    (0xe8)
#if defined(CONFIG_ARCH_MT5883) || defined(CONFIG_ARCH_MT5891)
#define OFFSET_MSDC_PAD_TUNE0            (0xf0)
#define OFFSET_MSDC_PAD_TUNE1   (0xf4)
#define OFFSET_MSDC_DAT_RDDLY0  (0xf8)
#define OFFSET_MSDC_DAT_RDDLY1  (0xfc)
#define OFFSET_MSDC_DAT_RDDLY2  (0x100)
#define OFFSET_MSDC_DAT_RDDLY3  (0x104)
#define OFFSET_MSDC_HW_DBG      (0x110)
#define OFFSET_MSDC_VERSION              (0x114)
#define OFFSET_MSDC_ECO_VER     (0x118)
#else
#if defined(CONFIG_ARCH_MT5882)
#define OFFSET_MSDC_PAD_TUNE0    ((IS_IC_MT5882())? (0xEC) : (0xf0))
#define OFFSET_MSDC_DAT_RDDLY0	((IS_IC_MT5882())? (0xF0) : (0xf8))
#define OFFSET_MSDC_DAT_RDDLY1  ((IS_IC_MT5882())? (0xF4) : (0xfc))
#define OFFSET_MSDC_HW_DBG      ((IS_IC_MT5882())? (0xF8) : (0x110))
#define OFFSET_MSDC_VERSION     ((IS_IC_MT5882())? (0x100): (0x114))//MAIN_VERSION
#define OFFSET_MSDC_ECO_VER     ((IS_IC_MT5882())? (0x104): (0x118))
#define OFFSET_MSDC_PAD_TUNE1   (0xf4)
#define OFFSET_MSDC_DAT_RDDLY2  (0x100)
#define OFFSET_MSDC_DAT_RDDLY3  (0x104)

#else
#define OFFSET_MSDC_PAD_TUNE0    (0xec)
#define OFFSET_MSDC_DAT_RDDLY0  (0xf0)
#define OFFSET_MSDC_DAT_RDDLY1  (0xf4)
#define OFFSET_MSDC_HW_DBG      (0xf8)
#define OFFSET_MSDC_VERSION     (0x100)
#define OFFSET_MSDC_ECO_VER     (0x104)
#endif
#endif

#define OFFSET_EMMC50_PAD_CTL0           (0x180)
#define OFFSET_EMMC50_PAD_DS_CTL0        (0x184)
#define OFFSET_EMMC50_PAD_DS_TUNE        (0x188)
#define OFFSET_EMMC50_PAD_CMD_TUNE       (0x18c)
#define OFFSET_EMMC50_PAD_DAT01_TUNE     (0x190)
#define OFFSET_EMMC50_PAD_DAT23_TUNE     (0x194)
#define OFFSET_EMMC50_PAD_DAT45_TUNE     (0x198)
#define OFFSET_EMMC50_PAD_DAT67_TUNE     (0x19c)
#define OFFSET_EMMC51_CFG0               (0x204)
#define OFFSET_EMMC50_CFG0               (0x208)
#define OFFSET_EMMC50_CFG1               (0x20c)
#define OFFSET_EMMC50_CFG2               (0x21c)
#define OFFSET_EMMC50_CFG3               (0x220)
#define OFFSET_EMMC50_CFG4               (0x224)

/*--------------------------------------------------------------------------*/
/* Register Address                                                         */
/*--------------------------------------------------------------------------*/

/* common register */
#define MSDC_CFG                REG_ADDR(MSDC_CFG)
#define MSDC_IOCON              REG_ADDR(MSDC_IOCON)
#define MSDC_PS                 REG_ADDR(MSDC_PS)
#define MSDC_INT                REG_ADDR(MSDC_INT)
#define MSDC_INTEN              REG_ADDR(MSDC_INTEN)
#define MSDC_FIFOCS             REG_ADDR(MSDC_FIFOCS)
#define MSDC_TXDATA             REG_ADDR(MSDC_TXDATA)
#define MSDC_RXDATA             REG_ADDR(MSDC_RXDATA)

/* sdmmc register */
#define SDC_CFG                 REG_ADDR(SDC_CFG)
#define SDC_CMD                 REG_ADDR(SDC_CMD)
#define SDC_ARG                 REG_ADDR(SDC_ARG)
#define SDC_STS                 REG_ADDR(SDC_STS)
#define SDC_RESP0               REG_ADDR(SDC_RESP0)
#define SDC_RESP1               REG_ADDR(SDC_RESP1)
#define SDC_RESP2               REG_ADDR(SDC_RESP2)
#define SDC_RESP3               REG_ADDR(SDC_RESP3)
#define SDC_BLK_NUM             REG_ADDR(SDC_BLK_NUM)
#define SDC_VOL_CHG                 REG_ADDR(SDC_VOL_CHG)
#define SDC_CSTS                REG_ADDR(SDC_CSTS)
#define SDC_CSTS_EN             REG_ADDR(SDC_CSTS_EN)
#define SDC_DCRC_STS            REG_ADDR(SDC_DCRC_STS)
#define SDC_CMD_STS				REG_ADDR(SDC_CMD_STS)

/* emmc register*/
#define EMMC_CFG0               REG_ADDR(EMMC_CFG0)
#define EMMC_CFG1               REG_ADDR(EMMC_CFG1)
#define EMMC_STS                REG_ADDR(EMMC_STS)
#define EMMC_IOCON              REG_ADDR(EMMC_IOCON)

/* auto command register */
#define SDC_ACMD_RESP           REG_ADDR(SDC_ACMD_RESP)
#define SDC_ACMD19_TRG          REG_ADDR(SDC_ACMD19_TRG)
#define SDC_ACMD19_STS          REG_ADDR(SDC_ACMD19_STS)

/* dma register */
#define MSDC_DMA_SA_H4B         REG_ADDR(MSDC_DMA_SA_H4B)
#define MSDC_DMA_SA             REG_ADDR(MSDC_DMA_SA)
#define MSDC_DMA_CA             REG_ADDR(MSDC_DMA_CA)
#define MSDC_DMA_CTRL           REG_ADDR(MSDC_DMA_CTRL)
#define MSDC_DMA_CFG            REG_ADDR(MSDC_DMA_CFG)
#define MSDC_DMA_LEN            REG_ADDR(MSDC_DMA_LEN)

/* pad ctrl register */
#define MSDC_PAD_CTL0           REG_ADDR(MSDC_PAD_CTL0)
#define MSDC_PAD_CTL1           REG_ADDR(MSDC_PAD_CTL1)
#define MSDC_PAD_CTL2           REG_ADDR(MSDC_PAD_CTL2)

/* data read delay */
#define MSDC_DAT_RDDLY0         REG_ADDR(MSDC_DAT_RDDLY0)
#define MSDC_DAT_RDDLY1         REG_ADDR(MSDC_DAT_RDDLY1)
#define MSDC_DAT_RDDLY2             REG_ADDR(MSDC_DAT_RDDLY2)
#define MSDC_DAT_RDDLY3             REG_ADDR(MSDC_DAT_RDDLY3)

/* debug register */
#define MSDC_DBG_SEL            REG_ADDR(MSDC_DBG_SEL)
#define MSDC_DBG_OUT            REG_ADDR(MSDC_DBG_OUT)

/* misc register */
#define MSDC_PATCH_BIT0             REG_ADDR(MSDC_PATCH_BIT0)
#define MSDC_PATCH_BIT1         REG_ADDR(MSDC_PATCH_BIT1)
#define MSDC_PATCH_BIT2         REG_ADDR(MSDC_PATCH_BIT2)
#define DAT0_TUNE_CRC               REG_ADDR(DAT0_TUNE_CRC)
#define DAT1_TUNE_CRC               REG_ADDR(DAT1_TUNE_CRC)
#define DAT2_TUNE_CRC               REG_ADDR(DAT2_TUNE_CRC)
#define DAT3_TUNE_CRC               REG_ADDR(DAT3_TUNE_CRC)
#define CMD_TUNE_CRC                REG_ADDR(CMD_TUNE_CRC)
#define SDIO_TUNE_WIND              REG_ADDR(SDIO_TUNE_WIND)
#define MSDC_PAD_TUNE0              REG_ADDR(MSDC_PAD_TUNE0)
#define MSDC_PAD_TUNE1          REG_ADDR(MSDC_PAD_TUNE1)
#define MSDC_HW_DBG             REG_ADDR(MSDC_HW_DBG)
#define MSDC_VERSION            REG_ADDR(MSDC_VERSION)
#define MSDC_ECO_VER                REG_ADDR(MSDC_ECO_VER)

/* eMMC 5.0 register */
#define EMMC50_PAD_CTL0             REG_ADDR(EMMC50_PAD_CTL0)
#define EMMC50_PAD_DS_CTL0          REG_ADDR(EMMC50_PAD_DS_CTL0)
#define EMMC50_PAD_DS_TUNE          REG_ADDR(EMMC50_PAD_DS_TUNE)
#define EMMC50_PAD_CMD_TUNE         REG_ADDR(EMMC50_PAD_CMD_TUNE)
#define EMMC50_PAD_DAT01_TUNE       REG_ADDR(EMMC50_PAD_DAT01_TUNE)
#define EMMC50_PAD_DAT23_TUNE       REG_ADDR(EMMC50_PAD_DAT23_TUNE)
#define EMMC50_PAD_DAT45_TUNE       REG_ADDR(EMMC50_PAD_DAT45_TUNE)
#define EMMC50_PAD_DAT67_TUNE       REG_ADDR(EMMC50_PAD_DAT67_TUNE)
#define EMMC51_CFG0                 REG_ADDR(EMMC51_CFG0)
#define EMMC50_CFG0                 REG_ADDR(EMMC50_CFG0)
#define EMMC50_CFG1                 REG_ADDR(EMMC50_CFG1)
#define EMMC50_CFG2                 REG_ADDR(EMMC50_CFG2)
#define EMMC50_CFG3                 REG_ADDR(EMMC50_CFG3)
#define EMMC50_CFG4                 REG_ADDR(EMMC50_CFG4)

/*--------------------------------------------------------------------------*/
/* Register Mask                                                            */
/*--------------------------------------------------------------------------*/

/* MSDC_CFG mask */
#define MSDC_CFG_MODE           (0x1  << 0)     /* RW */
#define MSDC_CFG_CKPDN          (0x1  << 1)     /* RW */
#define MSDC_CFG_RST            (0x1  << 2)     /* A0 */
#define MSDC_CFG_PIO            (0x1  << 3)     /* RW */
#define MSDC_CFG_CKDRVEN        (0x1  << 4)     /* RW */
#define MSDC_CFG_BV18SDT        (0x1  << 5)     /* RW */
#define MSDC_CFG_BV18PSS        (0x1  << 6)     /* R  */
#define MSDC_CFG_CKSTB          (0x1  << 7)     /* R  */
#if defined(CONFIG_ARCH_MT5882)
#define MSDC_CFG_CKDIV          ((IS_IC_MT5882())?(0xff << 8):(0xfff << 8))     /* RW */
#endif
#if defined(CONFIG_ARCH_MT5891)
#define MSDC_CFG_CKDIV          (0xfff << 8)     /* RW */
#else /* 91 */
#define MSDC_CFG_CKDIV          (0xff << 8)     /* RW */
#endif /* 91 */
#if defined(CONFIG_ARCH_MT5882) || defined(CONFIG_ARCH_MT5883) || defined(CONFIG_ARCH_MT5891)
#if defined(CONFIG_ARCH_MT5882)
#define MSDC_CFG_CKMOD          ((IS_IC_MT5882())?(0x3  << 16):(0x3  << 20))    /* W1C */
#else
#define MSDC_CFG_CKMOD				(0x3  << 20)    /* W1C */
#endif
#else /* 83&91 */
#define MSDC_CFG_CKMOD          (0x3  << 16)    /* W1C */
#endif /* 83&91 */
#define MSDC_CFG_CKMOD_HS400    (0x1  << 22)    /* RW */
#define MSDC_CFG_START_BIT      (0x3  << 23)    /* RW */
#define MSDC_CFG_SCLK_STOP_DDR  (0x1  << 25)    /* RW */
#define MSDC_CFG_BW_SEL			(0x1  << 29)		/* RW */

/* MSDC_IOCON mask */
#define MSDC_IOCON_SDR104CKS    (0x1  << 0)     /* RW */
#define MSDC_IOCON_RSPL         (0x1  << 1)     /* RW */
#define MSDC_IOCON_R_D_SMPL     (0x1  << 2)     /* RW */
#define MSDC_IOCON_DDLSEL       (0x1  << 3)     /* RW */
#define MSDC_IOCON_DDR50CKD     (0x1  << 4)     /* RW */
#define MSDC_IOCON_R_D_SMPL_SEL (0x1  << 5)     /* RW */
#define MSDC_IOCON_W_D_SMPL     (0x1  << 8)     /* RW */
#define MSDC_IOCON_W_D_SMPL_SEL (0x1  << 9)     /* RW */
#define MSDC_IOCON_W_D0SPL      (0x1  << 10)    /* RW */
#define MSDC_IOCON_W_D1SPL      (0x1  << 11)    /* RW */
#define MSDC_IOCON_W_D2SPL      (0x1  << 12)    /* RW */
#define MSDC_IOCON_W_D3SPL      (0x1  << 13)    /* RW */
#define MSDC_IOCON_R_D0SPL      (0x1  << 16)    /* RW */
#define MSDC_IOCON_R_D1SPL      (0x1  << 17)    /* RW */
#define MSDC_IOCON_R_D2SPL      (0x1  << 18)    /* RW */
#define MSDC_IOCON_R_D3SPL      (0x1  << 19)    /* RW */
#define MSDC_IOCON_R_D4SPL      (0x1  << 20)    /* RW */
#define MSDC_IOCON_R_D5SPL      (0x1  << 21)    /* RW */
#define MSDC_IOCON_R_D6SPL      (0x1  << 22)    /* RW */
#define MSDC_IOCON_R_D7SPL      (0x1  << 23)    /* RW */
#define MSDC_IOCON_RISCSZ       (0x3  << 24)    /* RW */

/* MSDC_PS mask */
#define MSDC_PS_CDEN            (0x1  << 0)     /* RW */
#define MSDC_PS_CDSTS           (0x1  << 1)     /* R  */
#define MSDC_PS_CDDEBOUNCE      (0xf  << 12)    /* RW */
#define MSDC_PS_DAT             (0xff << 16)    /* R  */
#define MSDC_PS_CMD             (0x1  << 24)    /* R  */
#define MSDC_PS_WP              (0x1UL<< 31)    /* R  */

/* MSDC_INT mask */
#define MSDC_INT_MMCIRQ         (0x1  << 0)     /* W1C */
#define MSDC_INT_CDSC           (0x1  << 1)     /* W1C */
#define MSDC_INT_ACMDRDY        (0x1  << 3)     /* W1C */
#define MSDC_INT_ACMDTMO        (0x1  << 4)     /* W1C */
#define MSDC_INT_ACMDCRCERR     (0x1  << 5)     /* W1C */
#define MSDC_INT_DMAQ_EMPTY     (0x1  << 6)     /* W1C */
#define MSDC_INT_SDIOIRQ        (0x1  << 7)     /* W1C */
#define MSDC_INT_CMDRDY         (0x1  << 8)     /* W1C */
#define MSDC_INT_CMDTMO         (0x1  << 9)     /* W1C */
#define MSDC_INT_RSPCRCERR      (0x1  << 10)    /* W1C */
#define MSDC_INT_CSTA           (0x1  << 11)    /* R */
#define MSDC_INT_XFER_COMPL     (0x1  << 12)    /* W1C */
#define MSDC_INT_DXFER_DONE     (0x1  << 13)    /* W1C */
#define MSDC_INT_DATTMO         (0x1  << 14)    /* W1C */
#define MSDC_INT_DATCRCERR      (0x1  << 15)    /* W1C */
#define MSDC_INT_ACMD19_DONE    (0x1  << 16)    /* W1C */
#define MSDC_INT_BDCSERR        (0x1  << 17)    /* W1C */
#define MSDC_INT_GPDCSERR       (0x1  << 18)    /* W1C */
#define MSDC_INT_DMAPRO         (0x1  << 19)    /* W1C */

/* MSDC_INTEN mask */
#define MSDC_INTEN_MMCIRQ       (0x1  << 0)     /* RW */
#define MSDC_INTEN_CDSC         (0x1  << 1)     /* RW */
#define MSDC_INTEN_ACMDRDY      (0x1  << 3)     /* RW */
#define MSDC_INTEN_ACMDTMO      (0x1  << 4)     /* RW */
#define MSDC_INTEN_ACMDCRCERR   (0x1  << 5)     /* RW */
#define MSDC_INTEN_DMAQ_EMPTY   (0x1  << 6)     /* RW */
#define MSDC_INTEN_SDIOIRQ      (0x1  << 7)     /* RW */
#define MSDC_INTEN_CMDRDY       (0x1  << 8)     /* RW */
#define MSDC_INTEN_CMDTMO       (0x1  << 9)     /* RW */
#define MSDC_INTEN_RSPCRCERR    (0x1  << 10)    /* RW */
#define MSDC_INTEN_CSTA         (0x1  << 11)    /* RW */
#define MSDC_INTEN_XFER_COMPL   (0x1  << 12)    /* RW */
#define MSDC_INTEN_DXFER_DONE   (0x1  << 13)    /* RW */
#define MSDC_INTEN_DATTMO       (0x1  << 14)    /* RW */
#define MSDC_INTEN_DATCRCERR    (0x1  << 15)    /* RW */
#define MSDC_INTEN_ACMD19_DONE  (0x1  << 16)    /* RW */
#define MSDC_INTEN_BDCSERR      (0x1  << 17)    /* RW */
#define MSDC_INTEN_GPDCSERR     (0x1  << 18)    /* RW */
#define MSDC_INTEN_DMAPRO       (0x1  << 19)    /* RW */

/* MSDC_FIFOCS mask */
#define MSDC_FIFOCS_RXCNT       (0xff << 0)     /* R */
#define MSDC_FIFOCS_TXCNT       (0xff << 16)    /* R */
#define MSDC_FIFOCS_CLR         (0x1UL<< 31)    /* RW */

/* SDC_CFG mask */
#define SDC_CFG_SDIOINTWKUP     (0x1  << 0)     /* RW */
#define SDC_CFG_INSWKUP         (0x1  << 1)     /* RW */
#define SDC_CFG_BUSWIDTH        (0x3  << 16)    /* RW */
#define SDC_CFG_SDIO            (0x1  << 19)    /* RW */
#define SDC_CFG_SDIOIDE         (0x1  << 20)    /* RW */
#define SDC_CFG_INTATGAP        (0x1  << 21)    /* RW */
#define SDC_CFG_DTOC            (0xffUL << 24)  /* RW */

/* SDC_CMD mask */
#define SDC_CMD_OPC             (0x3f << 0)     /* RW */
#define SDC_CMD_BRK             (0x1  << 6)     /* RW */
#define SDC_CMD_RSPTYP          (0x7  << 7)     /* RW */
#define SDC_CMD_DTYP            (0x3  << 11)    /* RW */
#define SDC_CMD_RW              (0x1  << 13)    /* RW */
#define SDC_CMD_STOP            (0x1  << 14)    /* RW */
#define SDC_CMD_GOIRQ           (0x1  << 15)    /* RW */
#define SDC_CMD_BLKLEN          (0xfff<< 16)    /* RW */
#define SDC_CMD_AUTOCMD         (0x3  << 28)    /* RW */
#define SDC_CMD_VOLSWTH         (0x1  << 30)    /* RW */
#define SDC_CMD_ACMD53          (0x1UL  << 31)    /* RW */

/* SDC_VOL_CHG mask */
#define SDC_VOL_CHG_CNT         (0xffff  << 0)     /* RW  */

/* SDC_STS mask */
#define SDC_STS_SDCBUSY         (0x1  << 0)     /* RW */
#define SDC_STS_CMDBUSY         (0x1  << 1)     /* RW */
#define SDC_STS_CMD_WR_BUSY     (0x1  << 16)    /* W1C */
#define SDC_STS_SWR_COMPL       (0x1UL  << 31)  /* RO  */

/* SDC_DCRC_STS mask */
#define SDC_DCRC_STS_POS        (0xff << 0)     /* RO */
#define SDC_DCRC_STS_NEG        (0xff << 8)     /* RO */

/*SDC_CMD_STS*/
#if defined(CONFIG_ARCH_MT5882)
#define SDC_CMD_STS_RSP_CRC			(0x7F << 0)		/* RU */
#define SDC_CMD_STS_RSP_INDEX		(0x3F << 7)		/* RU */
#define SDC_CMD_STS_RSP_ENDBIT		(0x1  << 13)		/* RU */
#define SDC_CMD_STS_INDEX_CHECK		(0x1  << 14)		/* RW */
#define SDC_CMD_STS_ENDBIT_CHECK	(0x1  << 15)		/* RW */
#endif

/* EMMC_CFG0 mask */
#define EMMC_CFG0_BOOTSTART     (0x1  << 0)     /* W */
#define EMMC_CFG0_BOOTSTOP      (0x1  << 1)     /* W */
#define EMMC_CFG0_BOOTMODE      (0x1  << 2)     /* RW */
#define EMMC_CFG0_BOOTACKDIS    (0x1  << 3)     /* RW */
#define EMMC_CFG0_BOOTWDLY      (0x7  << 12)    /* RW */
#define EMMC_CFG0_BOOTSUPP      (0x1  << 15)    /* RW */

/* EMMC_CFG1 mask */
#define EMMC_CFG1_BOOTDATTMC    (0xfffff << 0)  /* RW */
#define EMMC_CFG1_BOOTACKTMC    (0xfffUL << 20) /* RW */

/* EMMC_STS mask */
#define EMMC_STS_BOOTCRCERR     (0x1  << 0)     /* W1C */
#define EMMC_STS_BOOTACKERR     (0x1  << 1)     /* W1C */
#define EMMC_STS_BOOTDATTMO     (0x1  << 2)     /* W1C */
#define EMMC_STS_BOOTACKTMO     (0x1  << 3)     /* W1C */
#define EMMC_STS_BOOTUPSTATE    (0x1  << 4)     /* R */
#define EMMC_STS_BOOTACKRCV     (0x1  << 5)     /* W1C */
#define EMMC_STS_BOOTDATRCV     (0x1  << 6)     /* R */

/* EMMC_IOCON mask */
#define EMMC_IOCON_BOOTRST      (0x1  << 0)     /* RW */

/* SDC_ACMD19_TRG mask */
#define SDC_ACMD19_TRG_TUNESEL  (0xf  << 0)     /* RW */

/* MSDC_DMA_SA_HIGH4BIT */
#define MSDC_DMA_SURR_ADDR_H4B  (0xf  << 0)     /* RW */

/* MSDC_DMA_CTRL mask */
#define MSDC_DMA_CTRL_START     (0x1  << 0)     /* W */
#define MSDC_DMA_CTRL_STOP      (0x1  << 1)     /* W */
#define MSDC_DMA_CTRL_RESUME    (0x1  << 2)     /* W */
#define MSDC_DMA_CTRL_REDAYM    (0x1  << 3)     /* RO */
#define MSDC_DMA_CTRL_MODE      (0x1  << 8)     /* RW */
#define MSDC_DMA_CTRL_ALIGN     (0x1  << 9)     /* RW */
#define MSDC_DMA_CTRL_LASTBUF   (0x1  << 10)    /* RW */
#define MSDC_DMA_CTRL_SPLIT1K   (0x1  << 11)    /* RW */
#define MSDC_DMA_CTRL_BRUSTSZ   (0x7  << 12)    /* RW */
#if defined(CONFIG_ARCH_MT5399) || defined(CONFIG_ARCH_MT5890) || \
    defined(CONFIG_ARCH_MT5882) || defined(CONFIG_ARCH_MT5883) || \
	defined(CONFIG_ARCH_MT5891)
#else
#define MSDC_DMA_CTRL_XFERSZ    (0xffffUL << 16)	/* RW */
#endif

/* MSDC_DMA_CFG mask */
#define MSDC_DMA_CFG_STS        (0x1  << 0)     /* R */
#define MSDC_DMA_CFG_DECSEN     (0x1  << 1)     /* RW */
#define MSDC_DMA_CFG_LOCKDISABLE     (0x1  << 2)     /* RW */
#define MSDC_DMA_CFG_AHBEN      (0x3  << 8)     /* RW */
#define MSDC_DMA_CFG_ACTEN      (0x3  << 12)    /* RW */
#define MSDC_DMA_CFG_CS12B      (0x1  << 16)    /* RW */
#define DMA_CFG_OUTBOUND_STOP_DMA       (0x01 << 17)	/* sd only */

/* MSDC_PATCH_BIT0 mask */
#define MSDC_PB0_RESV1          (0x1 << 0)
#define MSDC_PB0_EN_8BITSUP     (0x1 << 1)
#define MSDC_PB0_DIS_RECMDWR    (0x1 << 2)
#define MSDC_PB0_RD_DAT_SEL     (0x1 << 3)
#define MSDC_PB0_RESV2          (0x3 << 4)        /* eMMC(MSDC1) */
#define MSDC_PB0_MSK_ACMD53_INT (0x1 << 4)        /* SD(MSDC0) */
#define MSDC_PB0_ACMD53_FAIL    (0x1 << 5)        /* SD(MSDC0) */
#define MSDC_PB0_DESCUP         (0x1 << 6)
#define MSDC_PB0_INT_DAT_LATCH_CK_SEL  (0x7 << 7)
#define MSDC_PB0_CKGEN_MSDC_DLY_SEL    (0x1F << 10)
#define MSDC_PB0_FIFORD_DIS     (0x1 << 15)
#define MSDC_PB0_BLKNUM_SEL     (0x1 << 16)
#define MSDC_PB0_SDIO_INTCSEL   (0x1 << 17)
#define MSDC_PB0_SDC_BSYDLY     (0xf << 18)
#define MSDC_PB0_SDC_WDOD       (0xf << 22)
#define MSDC_PB0_CMDIDRTSEL     (0x1 << 26)
#define MSDC_PB0_CMDFAILSEL     (0x1 << 27)
#define MSDC_PB0_SDIO_INTDLYSEL (0x1 << 28)
#define MSDC_PB0_SPCPUSH        (0x1 << 29)
#define MSDC_PB0_DETWR_CRCTMO   (0x1 << 30)
#define MSDC_PB0_EN_DRVRSP      (0x1UL << 31)

/* MSDC_PATCH_BIT1 mask */
#define MSDC_PB1_WRDAT_CRCS_TA_CNTR  (0x7 << 0)
#define MSDC_PB1_CMD_RSP_TA_CNTR     (0x7 << 3)
#define MSDC_PB1_GET_BUSY_MA     (0x1 << 6)
#define MSDC_PB1_GET_CRC_MA      (0x1 << 7)
#define MSDC_PB1_BIAS_TUNE_28NM  (0xf << 8)
#define MSDC_PB1_BIAS_EN18IO_28NM (0x1 << 12)
#define MSDC_PB1_BIAS_EXT_28NM   (0x1 << 13)
#define MSDC_PB1_RESV2           (0x1 << 14)
#define MSDC_PB1_RESET_GDMA      (0x1 << 15)
#define MSDC_PB1_SINGLEBURST     (0x1 << 16)
#define MSDC_PB1_FROCE_STOP      (0x1 << 17)
#define MSDC_PB1_DCM_DEV_SEL2    (0x3 << 18)
#define MSDC_PB1_DCM_DEV_SEL1    (0x1 << 20)
#define MSDC_PB1_DCM_EN          (0x1 << 21)
#define MSDC_PB1_AXI_WRAP_CKEN   (0x1 << 22)
#define MSDC_PB1_AHBCKEN         (0x1 << 23)
#define MSDC_PB1_CKSPCEN         (0x1 << 24)
#define MSDC_PB1_CKPSCEN         (0x1 << 25)
#define MSDC_PB1_CKVOLDETEN      (0x1 << 26)
#define MSDC_PB1_CKACMDEN        (0x1 << 27)
#define MSDC_PB1_CKSDEN          (0x1 << 28)
#define MSDC_PB1_CKWCTLEN        (0x1 << 29)
#define MSDC_PB1_CKRCTLEN        (0x1 << 30)
#define MSDC_PB1_CKSHBFFEN       (0x1UL << 31)

/* MSDC_PATCH_BIT2 mask */
#define MSDC_PB2_ENHANCEGPD      (0x1 << 0)
#define MSDC_PB2_SUPPORT64G      (0x1 << 1)
#define MSDC_PB2_RESPWAITCNT     (0x3 << 2)
#define MSDC_PB2_CFGRDATCNT      (0x1f << 4)
#define MSDC_PB2_CFGRDAT         (0x1 << 9)
#define MSDC_PB2_INTCRESPSEL     (0x1 << 11)
#define MSDC_PB2_CFGRESPCNT      (0x7 << 12)
#define MSDC_PB2_CFGRESP         (0x1 << 15)
#define MSDC_PB2_RESPSTENSEL     (0x7 << 16)
#define MSDC_PB2_DDR5O_SEL		 (0x1 << 19)
#define MSDC_PB2_POPENCNT        (0xf << 20)
#define MSDC_PB2_CFG_CRCSTS_SEL  (0x1 << 24)
#define MSDC_PB2_CFGCRCSTSEDGE   (0x1 << 25)
#define MSDC_PB2_CFGCRCSTSCNT    (0x3 << 26)
#define MSDC_PB2_CFGCRCSTS       (0x1 << 28)
#define MSDC_PB2_CRCSTSENSEL     (0x7UL << 29)

/* MSDC_PAD_CTL0 mask */
#define MSDC_PAD_CTL0_CLKDRVN   (0x7  << 0)     /* RW */
#define MSDC_PAD_CTL0_CLKDRVP   (0x7  << 4)     /* RW */
#define MSDC_PAD_CTL0_CLKSR     (0x1  << 8)     /* RW */
#define MSDC_PAD_CTL0_CLKPD     (0x1  << 16)    /* RW */
#define MSDC_PAD_CTL0_CLKPU     (0x1  << 17)    /* RW */
#define MSDC_PAD_CTL0_CLKSMT    (0x1  << 18)    /* RW */
#define MSDC_PAD_CTL0_CLKIES    (0x1  << 19)    /* RW */
#define MSDC_PAD_CTL0_CLKTDSEL  (0xf  << 20)    /* RW */
#define MSDC_PAD_CTL0_CLKRDSEL  (0xffUL<< 24)   /* RW */

/* MSDC_PAD_CTL1 mask */
#define MSDC_PAD_CTL1_CMDDRVN   (0x7  << 0)     /* RW */
#define MSDC_PAD_CTL1_CMDDRVP   (0x7  << 4)     /* RW */
#define MSDC_PAD_CTL1_CMDSR     (0x1  << 8)     /* RW */
#define MSDC_PAD_CTL1_CMDPD     (0x1  << 16)    /* RW */
#define MSDC_PAD_CTL1_CMDPU     (0x1  << 17)    /* RW */
#define MSDC_PAD_CTL1_CMDSMT    (0x1  << 18)    /* RW */
#define MSDC_PAD_CTL1_CMDIES    (0x1  << 19)    /* RW */
#define MSDC_PAD_CTL1_CMDTDSEL  (0xf  << 20)    /* RW */
#define MSDC_PAD_CTL1_CMDRDSEL  (0xffUL<< 24)   /* RW */

/* MSDC_PAD_CTL2 mask */
#define MSDC_PAD_CTL2_DATDRVN   (0x7  << 0)     /* RW */
#define MSDC_PAD_CTL2_DATDRVP   (0x7  << 4)     /* RW */
#define MSDC_PAD_CTL2_DATSR     (0x1  << 8)     /* RW */
#define MSDC_PAD_CTL2_DATPD     (0x1  << 16)    /* RW */
#define MSDC_PAD_CTL2_DATPU     (0x1  << 17)    /* RW */
#define MSDC_PAD_CTL2_DATIES    (0x1  << 19)    /* RW */
#define MSDC_PAD_CTL2_DATSMT    (0x1  << 18)    /* RW */
#define MSDC_PAD_CTL2_DATTDSEL  (0xf  << 20)    /* RW */
#define MSDC_PAD_CTL2_DATRDSEL  (0xffUL<< 24)   /* RW */

/* MSDC_PAD_TUNE0 mask */
#define MSDC_PAD_TUNE0_DATWRDLY      (0x1F <<  0)     /* RW */
#define MSDC_PAD_TUNE0_DELAYEN       (0x1  <<  7)     /* RW */
#define MSDC_PAD_TUNE0_DATRRDLY      (0x1F <<  8)     /* RW */
#define MSDC_PAD_TUNE0_DATRRDLYSEL   (0x1  << 13)     /* RW */
#define MSDC_PAD_TUNE0_RXDLYSEL      (0x1  << 15)     /* RW */
#define MSDC_PAD_TUNE0_CMDRDLY       (0x1F << 16)     /* RW */
#define MSDC_PAD_TUNE0_CMDRRDLYSEL   (0x1  << 21)     /* RW */
#define MSDC_PAD_TUNE0_CMDRRDLY      (0x1FUL << 22)   /* RW */
#define MSDC_PAD_TUNE0_CLKTXDLY      (0x1FUL << 27)   /* RW */

/* MSDC_PAD_TUNE1 mask */
#define MSDC_PAD_TUNE1_DATRRDLY2     (0x1F <<  8)     /* RW */
#define MSDC_PAD_TUNE1_DATRRDLY2SEL  (0x1  << 13)     /* RW */
#define MSDC_PAD_TUNE1_CMDRDLY2      (0x1F << 16)     /* RW */
#define MSDC_PAD_TUNE1_CMDRRDLY2SEL  (0x1  << 21)     /* RW */

/* MSDC_DAT_RDDLY0/1/2/3 mask */
#define MSDC_DAT_RDDLY0_D3      (0x1F << 0)     /* RW */
#define MSDC_DAT_RDDLY0_D2      (0x1F << 8)     /* RW */
#define MSDC_DAT_RDDLY0_D1      (0x1F << 16)    /* RW */
#define MSDC_DAT_RDDLY0_D0      (0x1FUL << 24)  /* RW */

#define MSDC_DAT_RDDLY1_D7      (0x1F << 0)     /* RW */
#define MSDC_DAT_RDDLY1_D6      (0x1F << 8)     /* RW */
#define MSDC_DAT_RDDLY1_D5      (0x1F << 16)    /* RW */
#define MSDC_DAT_RDDLY1_D4      (0x1FUL << 24)    /* RW */

#define MSDC_DAT_RDDLY2_D3      (0x1F << 0)     /* RW */
#define MSDC_DAT_RDDLY2_D2      (0x1F << 8)     /* RW */
#define MSDC_DAT_RDDLY2_D1      (0x1F << 16)    /* RW */
#define MSDC_DAT_RDDLY2_D0      (0x1FUL << 24)  /* RW */

#define MSDC_DAT_RDDLY3_D7      (0x1F << 0)     /* RW */
#define MSDC_DAT_RDDLY3_D6      (0x1F << 8)     /* RW */
#define MSDC_DAT_RDDLY3_D5      (0x1F << 16)    /* RW */
#define MSDC_DAT_RDDLY3_D4      (0x1FUL << 24)  /* RW */

/* MSDC_HW_DBG_SEL mask */
#define MSDC_HW_DBG0_SEL         (0xff << 0)
#define MSDC_HW_DBG1_SEL         (0xff << 8)
#define MSDC_HW_DBG2_SEL         (0xff << 16)
#define MSDC_HW_DBG3_SEL         (0x1f << 24)
#define MSDC_HW_DBG_WRAPTYPE_SEL (0x3UL  << 29)

/* EMMC50_PAD_DS_TUNE mask */
#define MSDC_EMMC50_PAD_DS_TUNE_DLYSEL  (0x1 << 0)
#define MSDC_EMMC50_PAD_DS_TUNE_DLY2SEL (0x1 << 1)
#define MSDC_EMMC50_PAD_DS_TUNE_DLY1    (0x1f << 2)
#define MSDC_EMMC50_PAD_DS_TUNE_DLY2    (0x1f << 7)
#define MSDC_EMMC50_PAD_DS_TUNE_DLY3    (0x1F << 12)

/* EMMC50_PAD_CMD_TUNE mask */
#define MSDC_EMMC50_PAD_CMD_TUNE_DLY3SEL (0x1 << 0)
#define MSDC_EMMC50_PAD_CMD_TUNE_RXDLY3  (0x1f << 1)
#define MSDC_EMMC50_PAD_CMD_TUNE_TXDLY   (0x1f << 6)

/* EMMC50_PAD_DAT01_TUNE mask */
#define MSDC_EMMC50_PAD_DAT0_RXDLY3SEL   (0x1 << 0)
#define MSDC_EMMC50_PAD_DAT0_RXDLY3      (0x1f << 1)
#define MSDC_EMMC50_PAD_DAT0_TXDLY       (0x1f << 6)
#define MSDC_EMMC50_PAD_DAT1_RXDLY3SEL   (0x1 << 16)
#define MSDC_EMMC50_PAD_DAT1_RXDLY3      (0x1f << 17)
#define MSDC_EMMC50_PAD_DAT1_TXDLY       (0x1f << 22)

/* EMMC50_PAD_DAT23_TUNE mask */
#define MSDC_EMMC50_PAD_DAT2_RXDLY3SEL   (0x1 << 0)
#define MSDC_EMMC50_PAD_DAT2_RXDLY3      (0x1f << 1)
#define MSDC_EMMC50_PAD_DAT2_TXDLY       (0x1f << 6)
#define MSDC_EMMC50_PAD_DAT3_RXDLY3SEL   (0x1 << 16)
#define MSDC_EMMC50_PAD_DAT3_RXDLY3      (0x1f << 17)
#define MSDC_EMMC50_PAD_DAT3_TXDLY       (0x1f << 22)

/* EMMC50_PAD_DAT45_TUNE mask */
#define MSDC_EMMC50_PAD_DAT4_RXDLY3SEL   (0x1 << 0)
#define MSDC_EMMC50_PAD_DAT4_RXDLY3      (0x1f << 1)
#define MSDC_EMMC50_PAD_DAT4_TXDLY       (0x1f << 6)
#define MSDC_EMMC50_PAD_DAT5_RXDLY3SEL   (0x1 << 16)
#define MSDC_EMMC50_PAD_DAT5_RXDLY3      (0x1f << 17)
#define MSDC_EMMC50_PAD_DAT5_TXDLY       (0x1f << 22)

/* EMMC50_PAD_DAT67_TUNE mask */
#define MSDC_EMMC50_PAD_DAT6_RXDLY3SEL   (0x1 << 0)
#define MSDC_EMMC50_PAD_DAT6_RXDLY3      (0x1f << 1)
#define MSDC_EMMC50_PAD_DAT6_TXDLY       (0x1f << 6)
#define MSDC_EMMC50_PAD_DAT7_RXDLY3SEL   (0x1 << 16)
#define MSDC_EMMC50_PAD_DAT7_RXDLY3      (0x1f << 17)
#define MSDC_EMMC50_PAD_DAT7_TXDLY       (0x1f << 22)

/* EMMC51_CFG0 mask */
#define MSDC_EMMC51_CFG0_CMDQ_EN         (0x1 << 0)
#define MSDC_EMMC51_CFG0_WDAT_CNT        (0x3ff << 1)
#define MSDC_EMMC51_CFG0_RDAT_CNT        (0x3ff << 11)
#define MSDC_EMMC51_CFG0_CMDQ_CMD_EN     (0x1 << 21)

/* EMMC50_CFG0 mask */
#define MSDC_EMMC50_CFG_PADCMD_LATCHCK      (0x1 << 0)
#define MSDC_EMMC50_CFG_CRC_STS_CNT         (0x3 << 1)
#define MSDC_EMMC50_CFG_CRC_STS_EDGE        (0x1 << 3)
#define MSDC_EMMC50_CFG_CRC_STS_SEL         (0x1 << 4)
#define MSDC_EMMC50_CFG_END_BIT_CHK_CNT     (0xf << 5)
#define MSDC_EMMC50_CFG_CMD_RESP_SEL        (0x1 << 9)
#define MSDC_EMMC50_CFG_CMD_EDGE_SEL        (0x1 << 10)
#define MSDC_EMMC50_CFG_ENDBIT_CNT          (0x3ff << 11)
#define MSDC_EMMC50_CFG_READ_DAT_CNT        (0x7 << 21)
#define MSDC_EMMC50_CFG_EMMC50_MON_SEL      (0x1 << 24)
#define MSDC_EMMC50_CFG_MSDC_WR_VALID       (0x1 << 25)
#define MSDC_EMMC50_CFG_MSDC_RD_VALID       (0x1 << 26)
#define MSDC_EMMC50_CFG_MSDC_WR_VALID_SEL   (0x1 << 27)
#define MSDC_EMMC50_CFG_EMMC50_MON_SE       (0x1 << 28)
#define MSDC_EMMC50_CFG_TXSKEWSEL           (0x1 << 29)

/* EMMC50_CFG1 mask */
#define MSDC_EMMC50_CFG1_WRPTR_MARGIN    (0xff << 0)
#define MSDC_EMMC50_CFG1_CKSWITCH_CNT    (0x7  << 8)
#define MSDC_EMMC50_CFG1_RDDAT_STOP      (0x1  << 11)
#define MSDC_EMMC50_CFG1_WAITCLK_CNT     (0xf  << 12)
#define MSDC_EMMC50_CFG1_DBG_SEL         (0xff << 16)
#define MSDC_EMMC50_CFG1_PSHCNT          (0x7  << 24)
#define MSDC_EMMC50_CFG1_PSHPSSEL        (0x1  << 27)
#define MSDC_EMMC50_CFG1_DSCFG           (0x1  << 28)
#define MSDC_EMMC50_CFG1_RESV0           (0x7UL  << 29)

/* EMMC50_CFG2_mask */
//#define MSDC_EMMC50_CFG2_AXI_GPD_UP            (0x1 << 0)
#define MSDC_EMMC50_CFG2_AXI_IOMMU_WR_EMI      (0x1 << 1)
#define MSDC_EMMC50_CFG2_AXI_SHARE_EN_WR_EMI   (0x1 << 2)
#define MSDC_EMMC50_CFG2_AXI_IOMMU_RD_EMI      (0x1 << 7)
#define MSDC_EMMC50_CFG2_AXI_SHARE_EN_RD_EMI   (0x1 << 8)
#define MSDC_EMMC50_CFG2_AXI_BOUND_128B        (0x1 << 13)
#define MSDC_EMMC50_CFG2_AXI_BOUND_256B        (0x1 << 14)
#define MSDC_EMMC50_CFG2_AXI_BOUND_512B        (0x1 << 15)
#define MSDC_EMMC50_CFG2_AXI_BOUND_1K          (0x1 << 16)
#define MSDC_EMMC50_CFG2_AXI_BOUND_2K          (0x1 << 17)
#define MSDC_EMMC50_CFG2_AXI_BOUND_4K          (0x1 << 18)
#define MSDC_EMMC50_CFG2_AXI_RD_OUTS_NUM       (0x1f<< 19)
#define MSDC_EMMC50_CFG2_AXI_SET_LEN           (0xf << 24)
#define MSDC_EMMC50_CFG2_AXI_RESP_ERR_TYPE     (0x3 << 28)
#define MSDC_EMMC50_CFG2_AXI_BUSY              (0x1 << 30)

/* EMMC50_CFG3_mask */
#define MSDC_EMMC50_CFG3_OUTS_WR               (0x1f << 0)
#define MSDC_EMMC50_CFG3_ULTRA_SET_WR          (0x3f << 5)
#define MSDC_EMMC50_CFG3_PREULTRA_SET_WR       (0x3f << 11)
#define MSDC_EMMC50_CFG3_ULTRA_SET_RD          (0x3f << 17)
#define MSDC_EMMC50_CFG3_PREULTRA_SET_RD       (0x3f << 23)

/* EMMC50_CFG4_mask */
#define MSDC_EMMC50_CFG4_IMPR_ULTRA_SET_WR     (0xff << 0)
#define MSDC_EMMC50_CFG4_IMPR_ULTRA_SET_RD     (0xff << 8)
#define MSDC_EMMC50_CFG4_ULTRA_EN              (0x3  << 16)
#define MSDC_EMMC50_CFG4_RESV0                 (0x3f << 18)

/* SDIO_TUNE_WIND mask*/
#define MSDC_SDIO_TUNE_WIND      (0x1f << 0)

/* MSDC_CFG[START_BIT] value */
#define START_AT_RISING             (0x0)
#define START_AT_FALLING            (0x1)
#define START_AT_RISING_AND_FALLING (0x2)
#define START_AT_RISING_OR_FALLING  (0x3)

#define MSDC_SMPL_RISING        (0)
#define MSDC_SMPL_FALLING       (1)
#define MSDC_SMPL_SEPERATE      (2)


#define TYPE_CMD_RESP_EDGE      (0)
#define TYPE_WRITE_CRC_EDGE     (1)
#define TYPE_READ_DATA_EDGE     (2)
#define TYPE_WRITE_DATA_EDGE    (3)

#define CARD_READY_FOR_DATA             (1<<8)
#define CARD_CURRENT_STATE(x)           ((x&0x00001E00)>>9)

/*DTV related*/
#if defined(CONFIG_ARCH_MT5399) || defined(CONFIG_ARCH_MT5882) || \
    defined(CONFIG_ARCH_MT5883)
#define MSDC_CLK_TARGET	200000000,  192000000,  162000000, 144000000,  120000000, 100000000, 80000000, 50000000, 50000000, 25000000, 12000000,  0
#define MSDC_CLK_SRC_VAL	11,	        12,		    4,	        5,		    6,	    11,	        9,	      8,	    11,	        11,	        0,	0
#define MSDC_CLK_MODE_VAL	1,	        1,		    1,	        1,		    1,	    0,	        1,	      1,	    2,	        0,	        0,	0
#define MSDC_CLK_DIV_VAL	0,	        0,		    0,	        0,		    0,	    0,	        0,	      0,	    0,	        2,	        0,	16
#define MSDC_CLK_DRV_VAL	60,	        60,		    60,         60,		    60,	    25,	        23,	     23,	    16,	        7,	        7,	0
#define MSDC_CMD_DRV_VAL	60,	        60,		    60,	        60,		    60,	    25,	        23,	     23,	    16,	        7,	        7,	0
#define MSDC_DAT_DRV_VAL	60,	        60,		    60,	        60,		    60,	    25,	        23,	     23,	    16,	        7,	        7,	0
#define MSDC_CLK_SAMP_VAL	0x6,	    0x6,		0x6,	    0x6,		0x6,	0x6,	    0x6,	0x6,	    0x6,	   0x0,	      0x0,0x0, 0x0
/*DTV related*/
#elif defined(CONFIG_ARCH_MT5890)
#define MSDC_CLK_TARGET	200000000,  192000000,  162000000, 144000000,  120000000, 100000000, 80000000, 50000000, 50000000, 25000000, 12000000,  0
#define MSDC_CLK_SRC_VAL	11,	        12,		    4,	        5,		    6,	    11,	        9,	      8,	    11,	        11,	        0,	0
#define MSDC_CLK_MODE_VAL	1,	        1,		    1,	        1,		    1,	    0,	        1,	      1,	    2,	        0,	        0,	0
#define MSDC_CLK_DIV_VAL	0,	        0,		    0,	        0,		    0,	    0,	        0,	      0,	    0,	        2,	        0,	16
#define MSDC_CLK_DRV_VAL	1,	        1,		    1,          1,		    1,	    1,	        1,	      1,	    1,	        1,	        1,	0
#define MSDC_CMD_DRV_VAL	1,	        1,		    1,	        1,		    1,	    1,	        1,	      1,	    1,	        1,	        1,	0
#define MSDC_DAT_DRV_VAL	1,	        1,		    1,	        1,		    1,	    1,	        1,	      1,	    1,	        1,	        1,	0
#define MSDC_CLK_SAMP_VAL	0x6,	    0x6,		0x6,	    0x6,		0x6,	0x6,	    0x6,	0x6,	    0x6,	   0x0,	      0x0,0x0, 0x0
#elif defined(CONFIG_ARCH_MT5891)
#define MSDC_CLK_TARGET	200000000,  192000000,  162000000, 144000000,  120000000, 100000000, 80000000, 50000000, 50000000, 25000000, 12000000,  0
#define MSDC_CLK_SRC_VAL	11,	        2,		    4,	        5,		    6,	    11,	        9,	      8,	    11,	        11,	        0,	0
#define MSDC_CLK_MODE_VAL	1,	        1,		    1,	        1,		    1,	    0,	        1,	      1,	    2,	        0,	        0,	0
#define MSDC_CLK_DIV_VAL	0,	        0,		    0,	        0,		    0,	    0,	        0,	      0,	    0,	        2,	        0,	16
#define MSDC_CLK_DRV_VAL	2,	        2,		    2,          2,		    2,	    2,	        2,	      2,	    2,	        2,	        2,	0
#define MSDC_CMD_DRV_VAL	5,	        5,		    5,	        5,		    5,	    5,	        5,	      5,	    5,	        5,	        5,	0
#define MSDC_DAT_DRV_VAL	5,	        5,		    5,	        5,		    5,	    5,	        5,	      5,	    5,	        5,	        5,	0
#define MSDC_CLK_SAMP_VAL	0x6,	    0x6,		0x6,	    0x6,		0x6,	0x6,	    0x6,	0x6,	    0x6,	   0x0,	      0x0,0x0, 0x0
#endif

#define MSDC_CLK_GATE_BIT (0x1 << 7)
#define MSDC_CLK_SEL_MASK (0xF << 0)
#define MSDC_HIGH_CLK_IDX 6
#define MSDC_NORM_CLK_IDX 7
#define MSDC_INIT_CLK_IDX 9

typedef enum {
	MSDC0DetectGpio,
	MSDC0WriteProtectGpio,
	MSDC0PoweronoffDetectGpio,
	MSDC0VoltageSwitchGpio,
	MSDCbackup1Gpio,
	MSDCbackup2Gpio
} MSDCGpioNumber;

typedef struct {
	u8 acName[32];
	u32 u4ID1;
	u32 u4ID2;
	u32 DS26Sample;
	u32 DS26Delay;
	u32 HS52Sample;
	u32 HS52Delay;
	u32 DDR52Sample;
	u32 DDR52Delay;
	u32 HS200Sample;
	u32 HS200Delay;
} EMMC_FLASH_DEV_T;

enum {
    RESP_NONE = 0,
    RESP_R1,
    RESP_R2,
    RESP_R3,
    RESP_R4,
    RESP_R5,
    RESP_R6,
    RESP_R7,
    RESP_R1B
};

#define MSDC_SCLK_SRC_INVALID		0xffffffff
#ifdef MSDC_ONLY_USE_ONE_CLKSRC
	#ifdef CONFIG_ARCH_MT5891
		#define EMMC_SCLK_DEFAULT	EMMC_SCLK_384M
		#define EMMC_HCLK_DEFAULT	EMMC_HCLK_162M
		#define SD_SCLK_DEFAULT		SD_SCLK_192M
		#define SD_HCLK_DEFAULT		SD_HCLK_96M
	#endif
#else
	#define EMMC_SCLK_DEFAULT	(-1)
	#define EMMC_HCLK_DEFAULT	(-1)
	#define SD_SCLK_DEFAULT		(-1)
	#define SD_HCLK_DEFAULT		(-1)
#endif

#ifdef CONFIG_ARCH_MT5891
#define MSDC_CLK_DMUXPLL_REG_ADDR	0xF006100C
#define MSDC_DMULPLL_GATE_BIT		(0x1 << 31)
enum {
	EMMC_SCLK_24M,	/* xtal_d1_ck */
	EMMC_SCLK_216M,	/* sawlesspll_d2_ck */
	EMMC_SCLK_192M,	/* usb_phy_pll_d2p5_ck */
	EMMC_SCLK_384M,	/* sawlesspll_d2p5_ck */
	EMMC_SCLK_162M,	/* syspll_d4_ck */
	EMMC_SCLK_144M,	/* sawlesspll_d3_ck */
	EMMC_SCLK_300M,	/* usb_phy_pll_d4 _ck */
	EMMC_SCLK_108M,	/* sawlesspll_d4_ck */
	EMMC_SCLK_48M,	/* sawlesspll_d9_ck */
	EMMC_SCLK_80M,	/* usb_phy_pll_d6 _ck */
	EMMC_SCLK_13M,	/* sawlesspll_d32_ck(13.5M) */
	EMMC_SCLK_200M	/* enetpll_d3_ck */
};

enum {
	EMMC_HCLK_24M,	/* xtal_d1_ck */
	EMMC_HCLK_96M,	/* usb_phy_pll_d5_ck */
	EMMC_HCLK_80M,	/* usb_phy_pll_d6_ck */
	EMMC_HCLK_72M,	/* syspll_d9_ck */
	EMMC_HCLK_60M,	/* usb_phy_pll_d8_ck */
	EMMC_HCLK_54M,	/* sawlesspll_d8_ck */
	EMMC_HCLK_216M,	/* sawlesspll_d9_ck */
	EMMC_HCLK_162M,	/* syspll_d16_ck */
	EMMC_HCLK_192M,	/* sawlesspll_d10_ck */
	EMMC_HCLK_120M	/* sawlesspll_d12_ck */
};

enum {
	SD_SCLK_24M,	/* xtal_d1_ck */
	SD_SCLK_216M,	/* sawlesspll_d2_ck */
	SD_SCLK_192M,	/* usb_phy_pll_d2p5_ck */
	SD_SCLK_300M,	/* sawlesspll_d2p5_ck */
	SD_SCLK_162M,	/* syspll_d4_ck */
	SD_SCLK_144M,	/* sawlesspll_d3_ck */
	SD_SCLK_120M,	/* usb_phy_pll_d4_ck */
	SD_SCLK_108M,	/* sawlesspll_d4_ck */
	SD_SCLK_48M,	/* sawlesspll_d9_ck */
	SD_SCLK_80M,	/* usb_phy_pll_d6_ck */
	SD_SCLK_13M,	/* sawlesspll_d32_ck */
	SD_SCLK_200M	/* enetpll_d3_ck */
};

enum {
	SD_HCLK_24M,	/* xtal_d1_ck */
	SD_HCLK_96M,	/* usb_phy_pll_d5_ck */
	SD_HCLK_80M,	/* usb_phy_pll_d6_ck */
	SD_HCLK_72M,	/* syspll_d9_ck */
	SD_HCLK_60M,	/* usb_phy_pll_d8_ck */
	SD_HCLK_54M,	/* sawlesspll_d8_ck */
	SD_HCLK_48M,	/* sawlesspll_d9_ck */
	SD_HCLK_40M,	/* pix_clk_div2 */
	SD_HCLK_43M,	/* sawlesspll_d10_ck */
	SD_HCLK_36M		/* sawlesspll_d12_ck */
};
#endif

enum {
    IDX_CLK_TARGET = 0,
    IDX_CLK_SRC_VAL,
    IDX_CLK_MODE_VAL,
    IDX_CLK_DIV_VAL,
    IDX_CLK_DRV_VAL,
    IDX_CMD_DRV_VAL,
    IDX_DAT_DRV_VAL,
    IDX_CLK_SAMP_VAL,
    IDX_CLK_IDX_MAX
};

typedef enum {
	cmd_counter = 0,
	read_counter,
	write_counter,
	all_counter,
} TUNE_COUNTER;

/*--------------------------------------------------------------------------*/
/* Descriptor Structure                                                     */
/*--------------------------------------------------------------------------*/
typedef struct {
    u32  hwo:1; /* could be changed by hw */
    u32  bdp:1;
    u32  rsv0:6;
    u32  chksum:8;
    u32  intr:1;
#if defined(CONFIG_ARCH_MT5891)
    u32  rsv1:7;
	u32  h4next:4;
	u32  h4ptr:4;
#else
    u32  rsv1:15;
#endif
    u32  next;
    u32  ptr;
#if defined(CONFIG_ARCH_MT5891)
    u32  buflen:24;
    u32  extlen:8;
#else
    u32  buflen:16;
    u32  extlen:8;
    u32  rsv2:8;
#endif
    u32  arg;
    u32  blknum;
    u32  cmd;
} gpd_t;

typedef struct {
    u32  eol:1;
    u32  rsv0:7;
    u32  chksum:8;
    u32  rsv1:1;
    u32  blkpad:1;
    u32  dwpad:1;
#if defined(CONFIG_ARCH_MT5891)
    u32  rsv2:5;
	u32  h4next:4;
	u32  h4ptr:4;
#else
    u32  rsv2:13;
#endif
    u32  next;
    u32  ptr;
#if defined(CONFIG_ARCH_MT5891)
    u32  buflen:24;
    u32  rsv3:8;
#else
    u32  buflen:16;
    u32  rsv3:16;
#endif
} bd_t;

#define msdc_init_gpd_ex(gpd,extlen,cmd,arg,blknum) \
	do { \
		((gpd_t*)gpd)->extlen = extlen; \
		((gpd_t*)gpd)->cmd    = cmd; \
		((gpd_t*)gpd)->arg    = arg; \
		((gpd_t*)gpd)->blknum = blknum; \
	} while(0)

#define msdc_init_bd(bd, blkpad, dwpad, dptr, dlen) \
	do { \
		BUG_ON(dlen > MAX_SGMT_SZ); \
		((bd_t*)bd)->blkpad = blkpad; \
		((bd_t*)bd)->dwpad  = dwpad; \
		((bd_t*)bd)->ptr    = dptr; \
		((bd_t*)bd)->buflen = dlen; \
	} while(0)

struct scatterlist_ex {
    u32 cmd;
    u32 arg;
    u32 sglen;
    struct scatterlist *sg;
};

#define DMA_FLAG_NONE       (0x00000000)
#define DMA_FLAG_EN_CHKSUM  (0x00000001)
#define DMA_FLAG_PAD_BLOCK  (0x00000002)
#define DMA_FLAG_PAD_DWORD  (0x00000004)

#ifdef MSDC_SAVE_AUTOK_INFO
typedef struct _autok_info_
{
	u32 szMSDCAutoK[18];
	u32 u4RunAutokFlag;
	u32 u4id1;
	u32 u4id2;
	u32 u4SpeedMode;
	u32 u4CpuDelayCell; //identify CPU
}kernel_autok_info;
#endif
struct msdc_dma {
    u32 flags;                   /* flags */
    u32 xfersz;                  /* xfer size in bytes */
    u32 sglen;                   /* size of scatter list */
    u32 blklen;                  /* block size */
    struct scatterlist *sg;      /* I/O scatter list */
    struct scatterlist_ex *esg;  /* extended I/O scatter list */
    u8  mode;                    /* dma mode        */
    u8  burstsz;                 /* burst size      */
    u8  intr;                    /* dma done interrupt */
    u8  padding;                 /* padding */
    u32 cmd;                     /* enhanced mode command */
    u32 arg;                     /* enhanced mode arg */
    u32 rsp;                     /* enhanced mode command response */
    u32 autorsp;                 /* auto command response */

    gpd_t *gpd;                  /* pointer to gpd array */
    bd_t  *bd;                   /* pointer to bd array */
    dma_addr_t gpd_addr;         /* the physical address of gpd array */
    dma_addr_t bd_addr;          /* the physical address of bd array */
    u32 used_gpd;                /* the number of used gpd elements */
    u32 used_bd;                 /* the number of used bd elements */
};

struct mmc_blk_data {
	spinlock_t	lock;
	struct gendisk	*disk;
	struct mmc_queue queue;
	unsigned int	usage;
	unsigned int	read_only;
};

struct tune_counter {
	u32 time_cmd;
	u32 time_read;
	u32 time_write;
	u32 time_hs400;
};

typedef enum {
	MSDC_STATE_DEFAULT = 0,
	MSDC_STATE_DDR,
	MSDC_STATE_HS200,
	MSDC_STATE_HS400
} msdc_state;

struct msdc_saved_para {
	u32							pad_tune;
	u32							ddly0;
	u32							ddly1;
	u32 						msdc_cfg;
	u32 						mode;
	u32 						div;
	u32 						sdc_cfg;
	u32 						iocon;
	msdc_state					state;
	u32 						hz;
	u8							cmd_resp_ta_cntr;
	u8							wrdat_crc_ta_cntr;
};

struct msdc_host {
    struct msdc_hw              *hw;

    struct mmc_host             *mmc;           /* mmc structure */
    struct mmc_command          *cmd;
    struct mmc_data             *data;
    struct mmc_request          *mrq;
	int							id;				/* host id */
    int                         cmd_rsp;

    int                         error;
    spinlock_t                  lock;           /* mutex */
	spinlock_t                  remove_bad_card;	/*to solve removing bad card race condition with hot-plug enable*/

	u32							sclk_src;

    u32                         blksz;          /* host block size */
    u32 __iomem                 *base;           /* host base address */
	u32 __iomem					*sclk_base;
	u32 __iomem					*hclk_base;

    u32                         xfer_size;      /* total transferred size */

    struct msdc_dma             dma;            /* dma channel */
    int                         dma_xfer;       /* dma transfer mode */

    u32                         timeout_ns;     /* data timeout ns */
    u32                         timeout_clks;   /* data timeout clks */

    atomic_t                    abort;          /* abort transfer */

    int                         irq;            /* host interrupt */

    struct completion           cmd_done;
    struct completion           xfer_done;

	u32							mclk;			/* mclk: the request clock of mmc sub-system */
	u32							sclk;			/* sclk: the really clock after divition */
    u8                          power_mode;     /* host power mode */
    u8                          card_inserted;  /* card inserted ? */
    u8                          suspend;        /* host suspended ? */
    u8                          app_cmd;        /* for app command */
    u32                         app_cmd_arg;
    struct tune_counter         t_counter;
	u32							rwcmd_time_tune;
	int							read_time_tune;
    int                         write_time_tune;
	u32							write_timeout_uhs104;
	u32							read_timeout_uhs104;
	u32							sw_timeout;
	u32							sd_30_busy;
	msdc_state					state;
	int							autok_error;
	struct msdc_saved_para		saved_para;
#ifdef CONFIG_MTK_USE_NEW_TUNE
	u32							tune_latch_ck_cnt;
#endif
	struct timer_list			*card_detect_timer;
	u8						    autocmd;
	bool						tune;
	bool						block_bad_card;
	void (*load_ett_settings)(struct mmc_host *);
#ifdef MSDC_SAVE_AUTOK_INFO
	kernel_autok_info			szKAutokInfo;
	u8							u1PartitionCfg;
	u32 						szCID[4];	
#endif
};


#define MSDC_CD_PIN_EN      (1 << 0)  /* card detection pin is wired   */
#define MSDC_WP_PIN_EN      (1 << 1)  /* write protection pin is wired */
#define MSDC_RST_PIN_EN     (1 << 2)  /* emmc reset pin is wired       */
#define MSDC_SDIO_IRQ       (1 << 3)  /* use internal sdio irq (bus)   */
#define MSDC_EXT_SDIO_IRQ   (1 << 4)  /* use external sdio irq         */
#define MSDC_REMOVABLE      (1 << 5)  /* removable slot                */
#define MSDC_SYS_SUSPEND    (1 << 6)  /* suspended by system           */
#define MSDC_HIGHSPEED      (1 << 7)  /* high-speed mode support       */
#define MSDC_UHS1           (1 << 8)  /* uhs-1 mode support            */
#define MSDC_DDR            (1 << 9)  /* ddr mode support              */
#define MSDC_HS400          (1 << 10)  /* uhs-1 mode support            */
#define MSDC_INTERNAL_CLK   (1 << 11)  /* Force Internal clock */

#define MSDC_SMPL_RISING    (0)
#define MSDC_SMPL_FALLING   (1)

#define MSDC_CMD_PIN        (0)
#define MSDC_DAT_PIN        (1)
#define MSDC_CD_PIN         (2)
#define MSDC_WP_PIN         (3)
#define MSDC_RST_PIN        (4)

#define MSDC_BOOT_EN (1)
#define MSDC_CD_HIGH (1)
#define MSDC_CD_LOW  (0)

enum {
    MSDC_EMMC = 0,
    MSDC_SD   = 1,
    MSDC_MAX  = 2
};

typedef void (*pm_callback_t)(pm_message_t state, void *data);
struct msdc_hw {
    unsigned int   flags;            /* hardware capability flags */
    unsigned int   data_pins;        /* data pins */
	unsigned int   host_function;	 /* define host function*/
	unsigned char  sclk_src;		  /* bus clock source */
	unsigned char  hclk_src;		  /* host clock source */
	phys_addr_t    hclk_base;		  /* host clock source base addr */
	phys_addr_t    sclk_base;		  /* bus clock source base addr */
    unsigned char  cmd_edge;         /* command latch edge */
    unsigned char  rdata_edge;        /* read data latch edge */
	unsigned char  wdata_edge;        /* write data latch edge */
    unsigned char  clk_drv;          /* clock pad driving */
    unsigned char  cmd_drv;          /* command pad driving */
    unsigned char  dat_drv;          /* data pad driving */
	unsigned char  clk_drv_sd_18;    /* clock pad driving for SD card at 1.8v*/
    unsigned char  cmd_drv_sd_18;    /* command pad driving for SD card at 1.8v*/
    unsigned char  dat_drv_sd_18;    /* data pad driving for SD card at 1.8v*/
	unsigned char  dat0rddly; //read; range: 0~31
	unsigned char  dat1rddly; //read; range: 0~31
	unsigned char  dat2rddly; //read; range: 0~31
	unsigned char  dat3rddly; //read; range: 0~31
	unsigned char  dat4rddly; //read; range: 0~31
	unsigned char  dat5rddly; //read; range: 0~31
	unsigned char  dat6rddly; //read; range: 0~31
	unsigned char  dat7rddly; //read; range: 0~31
	unsigned char  datwrddly; //write; range: 0~31
	unsigned char  cmdrrddly; //cmd; range: 0~31
	unsigned char  cmdrddly; //cmd; range: 0~31
	bool		   boot;			 /* define boot host*/
};

typedef enum {
   TRAN_MOD_PIO,
   TRAN_MOD_DMA,
   TRAN_MOD_NUM
} transfer_mode;

typedef enum {
   OPER_TYPE_READ,
   OPER_TYPE_WRITE,
   OPER_TYPE_NUM
} operation_type;

struct dma_addr{
   dma_addr_t start_address;
	struct dma_addr *next;
   u32 size;
   u8 end;
};

#ifdef MSDC_ONLY_USE_ONE_CLKSRC
extern unsigned int msdc_sclk_freq[][16];
static inline unsigned int msdc_cur_sclk_freq(struct msdc_host *host)
{
	return msdc_sclk_freq[host->id][host->sclk_src];
}
#else
extern u32 msdcClk[7][12];
static inline unsigned int msdc_cur_sclk_freq(struct msdc_host *host)
{
	return msdcClk[IDX_CLK_TARGET][host->sclk_src];
}
#endif

static inline unsigned int uffs(unsigned int x)
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

#define wait_cond(cond, tmo, left) \
	do { \
		u32 t = tmo; \
		while (1) { \
			if ((cond) || (t == 0)) \
				break; \
			if (t > 0) { \
				ndelay(1); \
				t--; \
			} \
		} \
		left = t; \
	} while (0)
#define msdc_txfifocnt()   ((sdr_read32(MSDC_FIFOCS) & MSDC_FIFOCS_TXCNT) >> 16)
#define msdc_rxfifocnt()   ((sdr_read32(MSDC_FIFOCS) & MSDC_FIFOCS_RXCNT) >> 0)
#define msdc_fifo_write32(v)   sdr_write32(MSDC_TXDATA, (v))
#define msdc_fifo_write8(v)    sdr_write8(MSDC_TXDATA, (v))
#define msdc_fifo_read32()   sdr_read32(MSDC_RXDATA)
#define msdc_fifo_read8()    sdr_read8(MSDC_RXDATA)

#define msdc_dma_on()        sdr_clr_bits(MSDC_CFG, MSDC_CFG_PIO)
#define msdc_dma_off()       sdr_set_bits(MSDC_CFG, MSDC_CFG_PIO)

#define sdc_is_busy()          (sdr_read32(SDC_STS) & SDC_STS_SDCBUSY)
#define sdc_is_cmd_busy()      (sdr_read32(SDC_STS) & SDC_STS_CMDBUSY)

#define sdc_send_cmd(cmd,arg) \
    do { \
        sdr_write32(SDC_ARG, (arg)); \
        mb(); \
        sdr_write32(SDC_CMD, (cmd)); \
    } while(0)

#define is_card_present(h)     (((struct msdc_host*)(h))->card_inserted)

#define msdc_retry(expr, retry, cnt) \
    do { \
        int backup = cnt; \
        while (retry) { \
            if (!(expr)) break; \
            if (cnt-- == 0) { \
                retry--; mdelay(1); cnt = backup; \
            } \
        } \
        WARN_ON(retry == 0); \
    } while(0)

#define msdc_reset() \
    do { \
        int retry = 3000, cnt = 1000; \
        sdr_set_bits(MSDC_CFG, MSDC_CFG_RST); \
        mb(); \
        msdc_retry(sdr_read32(MSDC_CFG) & MSDC_CFG_RST, retry, cnt); \
    } while(0)

#define msdc_clr_int() \
    do { \
        volatile u32 val = sdr_read32(MSDC_INT); \
        sdr_write32(MSDC_INT, val); \
    } while(0)

#define msdc_clr_fifo() \
	do { \
		int retry = 3000, cnt = 1000; \
		sdr_set_bits(MSDC_FIFOCS, MSDC_FIFOCS_CLR); \
		msdc_retry(sdr_read32(MSDC_FIFOCS) & MSDC_FIFOCS_CLR, retry, cnt); \
	} while(0)

#define msdc_reset_hw() \
		msdc_reset(); \
		msdc_clr_fifo(); \
		msdc_clr_int();

#define msdc_inten_save(val) \
    do { \
        val = sdr_read32(MSDC_INTEN); \
        sdr_clr_bits(MSDC_INTEN, val); \
    } while(0)

#define msdc_inten_restore(val) \
    do { \
        sdr_set_bits(MSDC_INTEN, val); \
    } while(0)

#if defined(CONFIG_ARCH_MT5891)
#define sdr_read_dma_sa() \
	(sizeof(dma_addr_t) > 4) ? \
	(((unsigned long long)(sdr_read32(MSDC_DMA_SA_H4B) & 0xf) << 32) | (unsigned long long)sdr_read32(MSDC_DMA_SA)) : \
	sdr_read32(MSDC_DMA_SA)
#define sdr_write_dma_sa(val) \
	do { \
		if (sizeof(val) > 4) { \
			sdr_write32(MSDC_DMA_SA_H4B, ((unsigned long long)val >> 32) & 0xF); \
			sdr_write32(MSDC_DMA_SA, val & 0xFFFFFFFF); \
		} else \
			sdr_write32(MSDC_DMA_SA, val); \
	} while(0)
#else
#define sdr_read_dma_sa()		sdr_read32(MSDC_DMA_SA)
#define sdr_write_dma_sa(val)	sdr_write32(MSDC_DMA_SA, val)
#endif

#define reg_sync_writel(v, a) \
	do {    \
		writel((v), (a));   \
		dsb();  \
	} while (0)

#define reg_sync_writew(v, a) \
	do {    \
		writew((v), (a));   \
		dsb();  \
	} while (0)

#define reg_sync_writeb(v, a) \
	do {    \
		writeb((v), (a));   \
		dsb();  \
	} while (0)

#define sdr_read8(reg)			__raw_readb((const volatile void *)reg)
#define sdr_read16(reg)			__raw_readw((const volatile void *)reg)
#define sdr_read32(reg)			__raw_readl((const volatile void *)reg)
#define sdr_write8(reg,val)		reg_sync_writeb(val,reg)
#define sdr_write16(reg,val)	reg_sync_writew(val,reg)
#define sdr_write32(reg,val)	reg_sync_writel(val,reg)
#define sdr_set_bits(reg,bs) \
	do { \
		volatile unsigned int tv = sdr_read32(reg); \
		tv |= (u32)(bs); \
		sdr_write32(reg,tv); \
	} while(0)
#define sdr_clr_bits(reg,bs) \
	do { \
		volatile unsigned int tv = sdr_read32(reg); \
		tv &= ~((u32)(bs)); \
		sdr_write32(reg,tv); \
	} while(0)

#define sdr_set_field(reg,field,val) \
    do { \
        volatile unsigned int tv = sdr_read32(reg);	\
        tv &= ~(field); \
        tv |= ((val) << (uffs((unsigned int)field) - 1)); \
        sdr_write32(reg,tv); \
    } while(0)
#define sdr_get_field(reg,field,val) \
    do { \
        volatile unsigned int tv = sdr_read32(reg);	\
        val = ((tv & (field)) >> (uffs((unsigned int)field) - 1)); \
    } while(0)
#define sdr_set_field_discrete(reg,field,val) \
			do { \
				volatile unsigned int tv = sdr_read32(reg); \
				tv = (val == 1) ? (tv|(field)):(tv & ~(field));\
				sdr_write32(reg,tv); \
			} while(0)
#define sdr_get_field_discrete(reg,field,val) \
			do { \
				volatile unsigned int tv = sdr_read32(reg); \
				val = tv & (field) ; \
				val = (val == field) ? 1 :0;\
			} while(0)

#define msdc_dev(x)	((x)->mmc->parent)

extern struct msdc_host *msdc_get_host(int host_function, bool boot);
#endif /* end of __MT_SD_H__ */

