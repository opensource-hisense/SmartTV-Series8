/*
 * drivers/mtd/nand/mtk_nand.h
 *
 *  This is the MTD driver for MTK NAND controller
 *
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


/** @file nand_reg.h
 *  nand_reg.h The NAND register constant macros
 */

#ifndef NAND_REG_H
#define NAND_REG_H

#if defined(CONFIG_ARCH_MT5399) || defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5861)
#define CC_NAND_60BIT_NFI       (1) 
#else
#define CC_NAND_60BIT_NFI       (0) 
#endif

#if defined(CC_MT5880) || defined(CC_MT5860) || defined(CONFIG_ARCH_MT5880) || defined(CONFIG_ARCH_MT5881) || defined(CONFIG_ARCH_MT5860) || defined(CONFIG_ARCH_MT5399) || defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5861)
#define CC_NAND_DMMERGE_NFI     (1) 
#else
#define CC_NAND_DMMERGE_NFI     (0) 
#endif

#if !(defined(CONFIG_ARCH_MT5396) || defined(CONFIG_ARCH_MT5368) || defined(CONFIG_ARCH_MT5389))
#define CC_NAND_WDLE_CFG       (1) 
#else
#define CC_NAND_WDLE_CFG       (0) 
#endif
//-----------------------------------------------------------------------------
// Constant definitions
//-----------------------------------------------------------------------------

#define NAND_DRAM_BASE                              ((UINT32)0x80000000)

#define REG_ACDROP_GPIO                             0xF000D540  //GPIOIN23
    #define REG_ACDROP_GPIOMASK                     0x800000    //GPIOIN23

// RS232 12-bit/24-bit ECC controller selection
#define NAND_NFI_PERMISC                            0x004028
    #define NAND_NFI_PERMISC_NFI_SEL                    (((UINT32) 1) << 0)
    
#define NAND_NFI_CNFG                               0x00800
    #define NAND_NFI_CNFG_AHB_MODE                      (((UINT32) 1) << 0)
    #define NAND_NFI_CNFG_READ_MODE                     (((UINT32) 1) << 1)
    #define NAND_NFI_CNFG_SEL_SEC_512BYTE               (((UINT32) 1) << 5)
    #define NAND_NFI_CNFG_BYTE_RW                       (((UINT32) 1) << 6)
    #define NAND_NFI_CNFG_HW_ECC_EN                     (((UINT32) 1) << 8)
    #define NAND_NFI_CNFG_AUTO_FMT_EN                   (((UINT32) 1) << 9)
    #define NAND_NFI_CNFG_OP_MODE(x)                    ((((UINT32) x ) & 0x07) << 12)
        #define NAND_NFI_CNFG_OP_IDLE                       (((UINT32) 0) << 12)
        #define NAND_NFI_CNFG_OP_READ                       (((UINT32) 1) << 12)
        #define NAND_NFI_CNFG_OP_SINGLE                     (((UINT32) 2) << 12)
        #define NAND_NFI_CNFG_OP_PROGRAM                    (((UINT32) 3) << 12)
        #define NAND_NFI_CNFG_OP_ERASE                      (((UINT32) 4) << 12)
        #define NAND_NFI_CNFG_OP_RESET                      (((UINT32) 5) << 12)
        #define NAND_NFI_CNFG_OP_CUSTOM                     (((UINT32) 6) << 12)
        #define NAND_NFI_CNFG_OP_RESERVE                    (((UINT32) 7) << 12)

#define NAND_NFI_PAGEFMT                            0x00804
    #define NAND_NFI_PAGEFMT_PAGE(x)                    ((((UINT32) x ) & 0x03) << 0)
        #define NAND_NFI_PAGEFMT_PAGE_SIZE_512_2k           (((UINT32) 0) << 0)
#if defined(CONFIG_ARCH_MT5881)
        #define NAND_NFIECC_13BIT                         	  13	// 1 bit ECC need 13 bit ecc code
        #define NAND_NFI_PAGEFMT_PAGE_SIZE_2k				(((UINT32) 1) << 0)
        #define NAND_NFI_PAGEFMT_PAGE_SIZE_4k			    (((UINT32) 2) << 0)
#else
        #define NAND_NFI_PAGEFMT_PAGE_SIZE_4k               (((UINT32) 1) << 0)
        #define NAND_NFI_PAGEFMT_PAGE_SIZE_8k               (((UINT32) 2) << 0)
#endif
#if defined(CONFIG_ARCH_MT5399) || defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5861)
        #define NAND_NFI_PAGEFMT_PAGE_SIZE_16k              (((UINT32) 3) << 0)
#endif
#if CC_NAND_60BIT_NFI
    #define NAND_NFI_PAGEFMT_SPARE(x)                   ((((UINT32) x ) & 0x0F) << 2)
        #define NAND_NFI_PAGEFMT_SPARE_16                   (((UINT32) 0) << 2)
        #define NAND_NFI_PAGEFMT_SPARE_26                   (((UINT32) 1) << 2)
        #define NAND_NFI_PAGEFMT_SPARE_27                   (((UINT32) 2) << 2)
        #define NAND_NFI_PAGEFMT_SPARE_28                   (((UINT32) 3) << 2)
        #define NAND_NFI_PAGEFMT_SPARE_32                   (((UINT32) 4) << 2)
        #define NAND_NFI_PAGEFMT_SPARE_36                   (((UINT32) 5) << 2)
        #define NAND_NFI_PAGEFMT_SPARE_40                   (((UINT32) 6) << 2)
        #define NAND_NFI_PAGEFMT_SPARE_44                   (((UINT32) 7) << 2)
        #define NAND_NFI_PAGEFMT_SPARE_48                   (((UINT32) 8) << 2)
        #define NAND_NFI_PAGEFMT_SPARE_50                   (((UINT32) 9) << 2)
        #define NAND_NFI_PAGEFMT_SPARE_52                   (((UINT32) 10) << 2)
        #define NAND_NFI_PAGEFMT_SPARE_54                   (((UINT32) 11) << 2)
        #define NAND_NFI_PAGEFMT_SPARE_56                   (((UINT32) 12) << 2)
        #define NAND_NFI_PAGEFMT_SPARE_62                   (((UINT32) 13) << 2)
        #define NAND_NFI_PAGEFMT_SPARE_63                   (((UINT32) 14) << 2)
        #define NAND_NFI_PAGEFMT_SPARE_64                   (((UINT32) 15) << 2)
#else
    #define NAND_NFI_PAGEFMT_SPARE(x)                   ((((UINT32) x ) & 0x03) << 4)
        #define NAND_NFI_PAGEFMT_SPARE_16                   (((UINT32) 0) << 4)
        #define NAND_NFI_PAGEFMT_SPARE_26                   (((UINT32) 1) << 4)
        #define NAND_NFI_PAGEFMT_SPARE_27                   (((UINT32) 2) << 4)
        #define NAND_NFI_PAGEFMT_SPARE_28                   (((UINT32) 3) << 4)
#endif 
    #define NAND_NFI_PAGEFMT_FDM_NUM(x)                 ((((UINT32) x ) & 0x1F) << 6)
    #define NAND_NFI_PAGEFMT_ECC_NUM(x)                 ((((UINT32) x ) & 0x1F) << 11)
#if CC_NAND_60BIT_NFI
    #define NAND_NFI_PAGEFMT_SECTOR_SIZE(x)              ((((UINT32) x ) & 0x7FF) << 16)
        #define NAND_NFI_PAGEFMT_SECTOR_SIZE_512_16      ((((UINT32) 0x210 ) & 0x7FF) << 16)
        #define NAND_NFI_PAGEFMT_SECTOR_SIZE_1024_32     ((((UINT32) 0x420 ) & 0x7FF) << 16)
        #define NAND_NFI_PAGEFMT_SECTOR_SIZE_1024_52     ((((UINT32) 0x434 ) & 0x7FF) << 16)
        #define NAND_NFI_PAGEFMT_SECTOR_SIZE_1024_54     ((((UINT32) 0x436 ) & 0x7FF) << 16)
        #define NAND_NFI_PAGEFMT_SECTOR_SIZE_1024_56     ((((UINT32) 0x438 ) & 0x7FF) << 16)
        #define NAND_NFI_PAGEFMT_SECTOR_SIZE_1024_64     ((((UINT32) 0x440 ) & 0x7FF) << 16)
        #define NAND_NFI_PAGEFMT_SECTOR_SIZE_1024_72     ((((UINT32) 0x448 ) & 0x7FF) << 16)
        #define NAND_NFI_PAGEFMT_SECTOR_SIZE_1024_80     ((((UINT32) 0x450 ) & 0x7FF) << 16)
        #define NAND_NFI_PAGEFMT_SECTOR_SIZE_1024_88     ((((UINT32) 0x458 ) & 0x7FF) << 16)
        #define NAND_NFI_PAGEFMT_SECTOR_SIZE_1024_96     ((((UINT32) 0x460 ) & 0x7FF) << 16)
        #define NAND_NFI_PAGEFMT_SECTOR_SIZE_1024_100    ((((UINT32) 0x464 ) & 0x7FF) << 16)
        #define NAND_NFI_PAGEFMT_SECTOR_SIZE_1024_104    ((((UINT32) 0x468 ) & 0x7FF) << 16)
        #define NAND_NFI_PAGEFMT_SECTOR_SIZE_1024_108    ((((UINT32) 0x46C ) & 0x7FF) << 16)
        #define NAND_NFI_PAGEFMT_SECTOR_SIZE_1024_112    ((((UINT32) 0x470 ) & 0x7FF) << 16)
        #define NAND_NFI_PAGEFMT_SECTOR_SIZE_1024_124    ((((UINT32) 0x47C ) & 0x7FF) << 16)
        #define NAND_NFI_PAGEFMT_SECTOR_SIZE_1024_126    ((((UINT32) 0x47E ) & 0x7FF) << 16)
        #define NAND_NFI_PAGEFMT_SECTOR_SIZE_1024_128    ((((UINT32) 0x480 ) & 0x7FF) << 16)
#endif
#define NAND_NFI_CON                                0x00808
    #define NAND_NFI_CON_FIFO_FLUSH                     (((UINT32) 1) << 0)
    #define NAND_NFI_CON_NFI_RST                        (((UINT32) 1) << 1)
    #define NAND_NFI_CON_SRD                            (((UINT32) 1) << 4)
    #define NAND_NFI_CON_NOB(x)                         ((((UINT32) x ) & 0x7) << 5)
    #define NAND_NFI_CON_BRD                            (((UINT32) 1) << 8)
    #define NAND_NFI_CON_BWR                            (((UINT32) 1) << 9)
    #define NAND_NFI_CON_BRD_HW_EN						(((UINT32) 1) << 10)
#if defined(CONFIG_ARCH_MT5399) || defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5861)
    #define NAND_NFI_CON_SEC_NUM(x)                     ((((UINT32) x ) & 0x1F) << 11)
#else
    #define NAND_NFI_CON_SEC_NUM(x) 					((((UINT32) x ) & 0xF) << 12)
#endif

#define NAND_NFI_ACCCON1                            0x0080C
#define NAND_NFI_ACCCON2                            0x009A4

#define NAND_NFI_INTR_EN                            0x00810
    #define NAND_NFI_INTR_EN_RD_DONE                    (((UINT32) 1) << 0)
    #define NAND_NFI_INTR_EN_WR_DONE                    (((UINT32) 1) << 1)
    #define NAND_NFI_INTR_EN_RESET_DONE                 (((UINT32) 1) << 2)
    #define NAND_NFI_INTR_EN_ERASE_DONE                 (((UINT32) 1) << 3)
    #define NAND_NFI_INTR_EN_BUSY_RETURN                (((UINT32) 1) << 4)
    #define NAND_NFI_INTR_EN_ACCESS_LOCK                (((UINT32) 1) << 5)
    #define NAND_NFI_INTR_EN_AHB_DONE                   (((UINT32) 1) << 6)
#if CC_NAND_60BIT_NFI
    #define NAND_NFI_INTR_EN_BUSY_RETURN_EN2            (((UINT32) 1) << 7)
#endif 
    #define NAND_NFI_INTR_EN_RS232RD_DONE               (((UINT32) 1) << 8)
    #define NAND_NFI_INTR_EN_RS232WR_DONE               (((UINT32) 1) << 9)
    #define NAND_NFI_INTR_EN_MDMA_DONE                  (((UINT32) 1) << 10)

#define NAND_NFI_INTR                               0x00814
    #define NAND_NFI_INTR_RD_DONE                       (((UINT32) 1) << 0)
    #define NAND_NFI_INTR_WR_DONE                       (((UINT32) 1) << 1)
    #define NAND_NFI_INTR_RESET_DONE                    (((UINT32) 1) << 2)
    #define NAND_NFI_INTR_ERASE_DONE                    (((UINT32) 1) << 3)
    #define NAND_NFI_INTR_BUSY_RETURN                   (((UINT32) 1) << 4)
    #define NAND_NFI_INTR_ACCESS_LOCK                   (((UINT32) 1) << 5)
    #define NAND_NFI_INTR_AHB_DONE                      (((UINT32) 1) << 6)
    #define NAND_NFI_INTR_RS232RD_DONE                  (((UINT32) 1) << 8)
    #define NAND_NFI_INTR_RS232WR_DONE                  (((UINT32) 1) << 9)
    #define NAND_NFI_INTR_MDMA_DONE                     (((UINT32) 1) << 10)
    #define NAND_NFI_INTR_MASK                          ((UINT32) 0x44B)

#define NAND_NFI_CMD                                0x00820
    
#define NAND_NFI_ADDRNOB                            0x00830
#define NAND_NFI_COLADDR                            0x00834
#define NAND_NFI_ROWADDR                            0x00838
#define NAND_NFI_STRDATA                            0x00840
    #define NAND_NFI_STRDATA_STRDATA                    (((UINT32) 0x1) << 0)
    
#define NAND_NFI_DATAW                              0x00850
#define NAND_NFI_DATAWB                             0x00850
#define NAND_NFI_DATAR                              0x00854
#define NAND_NFI_DATARB                             0x00854

#define NAND_NFI_STA                                0x00860
    #define NAND_NFI_STA_NFI_FSM                        (((UINT32) 0xF) << 16)
        #define NAND_NFI_STA_NFI_FSM_READ_DATA              (((UINT32) 0x3) << 16)
        
#define NAND_NFI_FIFOSTA                            0x00864
    #define NAND_NFI_FIFOSTA_RD_REMAIN                  (((UINT32) 0x1F) << 0)
    #define NAND_NFI_FIFOSTA_RD_EMPTY                   (((UINT32) 1) << 6)
    #define NAND_NFI_FIFOSTA_WT_EMPTY                   (((UINT32) 1) << 14)
#if defined(CONFIG_ARCH_MT5399) || defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5861)
    #define NAND_NFI_EMPTY_BIT_NUM(x)                   ((((UINT32) x ) & 0x3F) << 24)
#endif

#define NAND_NFI_LOCKSTA                            0x00868
#define NAND_NFI_ADDRCNTR                           0x00870
#if defined(CONFIG_ARCH_MT5399) || defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5861)
    #define NAND_NFI_ADDRCNTR_SEC_CNTR(x)               ((((UINT32) x ) & 0x1F) << 11)
#else
    #define NAND_NFI_ADDRCNTR_SEC_CNTR(x)               ((((UINT32) x ) & 0xF) << 12)
#endif
#define NAND_NFI_STRADDR                            0x00880
#define NAND_NFI_BYTELEN                            0x00884

// I/O Pin Control
#define NAND_NFI_CSEL                               0x00890
#define NAND_NFI_IOCON                              0x00894

//FDM Data Content
#define NAND_NFI_FDM8L                              0x00700
#define NAND_NFI_FDM0L                              0x008A0
#define NAND_NFI_FDM0M                              0x008A4
#define NAND_NFI_FDM0L2                             0x008A8
#define NAND_NFI_FDM0M2                             0x008AC
#define NAND_NFI_FDM_LEN                            16
#define NAND_NFI_FDM_SECTNUM                        8

//Flash Lock
#define NAND_NFI_LOCKEN                             0x009E0
#define NAND_NFI_LOCKCON                            0x009E4
#define NAND_NFI_LOCKANOB                           0x009E8
#define NAND_NFI_LOCK00ADD                          0x009F0
#define NAND_NFI_LOCK00FMT                          0x009F4
#define NAND_NFI_LOCK01ADD                          0x009F8
#define NAND_NFI_LOCK01FMT                          0x009FC
#define NAND_NFI_LOCK02ADD                          0x00920
#define NAND_NFI_LOCK02FMT                          0x00924
#define NAND_NFI_LOCK03ADD                          0x00928
#define NAND_NFI_LOCK03FMT                          0x0092C
#define NAND_NFI_LOCK04ADD                          0x00930
#define NAND_NFI_LOCK04FMT                          0x00934
#define NAND_NFI_LOCK05ADD                          0x00938
#define NAND_NFI_LOCK05FMT                          0x0093C
#define NAND_NFI_LOCK06ADD                          0x00940
#define NAND_NFI_LOCK06FMT                          0x00944
#define NAND_NFI_LOCK07ADD                          0x00948
#define NAND_NFI_LOCK07FMT                          0x0094C
#define NAND_NFI_LOCK08ADD                          0x00950
#define NAND_NFI_LOCK08FMT                          0x00954
#define NAND_NFI_LOCK09ADD                          0x00958
#define NAND_NFI_LOCK09FMT                          0x0095C
#define NAND_NFI_LOCK10ADD                          0x00960
#define NAND_NFI_LOCK10FMT                          0x00964
#define NAND_NFI_LOCK11ADD                          0x00968
#define NAND_NFI_LOCK11FMT                          0x0096C
#define NAND_NFI_LOCK12ADD                          0x00970
#define NAND_NFI_LOCK12FMT                          0x00974
#define NAND_NFI_LOCK13ADD                          0x00978
#define NAND_NFI_LOCK13FMT                          0x0097C
#define NAND_NFI_LOCK14ADD                          0x00980
#define NAND_NFI_LOCK14FMT                          0x00984
#define NAND_NFI_LOCK15ADD                          0x00988
#define NAND_NFI_LOCK15FMT                          0x0098C

//Debug Register
#define NAND_NFI_FIFODATA0                          0x00990
#define NAND_NFI_FIFODATA1                          0x00994
#define NAND_NFI_FIFODATA2                          0x00998
#define NAND_NFI_FIFODATA3                          0x0099C

#define NAND_NFI_MISC                               0x009A0
    #define NAND_NFI_MISC_FLASH_PMUX                    (((UINT32) 1) << 2)
#define NAND_NFI_LCD2NAND                           0x009A4
// clock division
#define NAND_NFI_CLKDIV                             0x009AC
    #define NAND_NFI_CLKDIV_EN                          (((UINT32) 1) << 0)

// multi-page dma
#define NAND_NFI_MDMACON                            0x009B0
    #define NAND_NFI_MDMA_TRIG                          (((UINT32) 1) << 0)
    #define NAND_NFI_MDMA_EN                            (((UINT32) 1) << 4)
    #define NAND_NFI_MDMA_MODE(x)                       ((((UINT32) x ) & 0x3) << 8)
        #define NAND_NFI_MDMA_READ                          (((UINT32) 0) << 8)
        #define NAND_NFI_MDMA_WRITE                         (((UINT32) 1) << 8)
        #define NAND_NFI_MDMA_ERASE                         (((UINT32) 2) << 8)
    #define NAND_NFI_MDMA_LEN(x)                        ((((UINT32) x ) & 0x7) << 12)

#define NAND_NFI_MDMA_PAGENUM                       8
#define NAND_NFI_MDMAADDR                           0x009B4
#if defined(CONFIG_ARCH_MT5399) || defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5861)
    #define NAND_NFI_MDMAADDR_DRAMADDR                  (NAND_NFI_MDMA_PAGENUM * 0)
    #define NAND_NFI_MDMAADDR_ROWADDR                   (NAND_NFI_MDMA_PAGENUM * 1)
    #define NAND_NFI_MDMAADDR_FDM                       (NAND_NFI_MDMA_PAGENUM * 2)
    #define NAND_NFI_MDMAADDR_DECDONE                   (NAND_NFI_MDMA_PAGENUM * 66)
    #define NAND_NFI_MDMAADDR_DECFER                    (NAND_NFI_MDMA_PAGENUM * 67)
    #define NAND_NFI_MDMAADDR_DECENUM0                  (NAND_NFI_MDMA_PAGENUM * 68)
    #define NAND_NFI_MDMAADDR_DECENUM1                  (NAND_NFI_MDMA_PAGENUM * 69)	
    #define NAND_NFI_MDMAADDR_DECENUM2                  (NAND_NFI_MDMA_PAGENUM * 70)
    #define NAND_NFI_MDMAADDR_DECENUM3                  (NAND_NFI_MDMA_PAGENUM * 71)	
    #define NAND_NFI_MDMAADDR_DECEL0					(NAND_NFI_MDMA_PAGENUM * 72)
#else
    #define NAND_NFI_MDMAADDR_DRAMADDR                  (NAND_NFI_MDMA_PAGENUM * 0)
    #define NAND_NFI_MDMAADDR_ROWADDR                   (NAND_NFI_MDMA_PAGENUM * 1)
    #define NAND_NFI_MDMAADDR_FDM                       (NAND_NFI_MDMA_PAGENUM * 2)
    #define NAND_NFI_MDMAADDR_DECDONE                   (NAND_NFI_MDMA_PAGENUM * 34)
    #define NAND_NFI_MDMAADDR_DECFER                    (NAND_NFI_MDMA_PAGENUM * 35)
    #define NAND_NFI_MDMAADDR_DECENUM0                  (NAND_NFI_MDMA_PAGENUM * 36)
    #define NAND_NFI_MDMAADDR_DECENUM1                  (NAND_NFI_MDMA_PAGENUM * 37)	
    #define NAND_NFI_MDMAADDR_DECEL0                    (NAND_NFI_MDMA_PAGENUM * 38)
#endif
#define NAND_NFI_MDMADATA                           0x009B8

#if CC_NAND_60BIT_NFI
// random mizer
#define NAND_NFI_RANDOM_CFG                         0x009BC
    #define NAND_NFI_ENCODING_RANDON_EN                 (((UINT32) 1) << 0)
    #define NAND_NFI_ENCODING_RANDON_SEED(x)            ((((UINT32) x ) & 0x7FFF) << 1)
    #define NAND_NFI_DECODING_RANDON_EN                 (((UINT32) 1) << 16)
    #define NAND_NFI_DECODING_RANDON_SEED(x)            ((((UINT32) x ) & 0x7FFF) << 17)
#else
// scramble
#define NAND_NFI_SCRAMBLE                           0x009BC
    #define NAND_NFI_INTERLEAVE_EN                      (((UINT32) 1) << 0)
    #define NAND_NFI_SCRAMBLE_EN                        (((UINT32) 1) << 4)
    #define NAND_NFI_SCRAMBLE_PAT(x)                    ((((UINT32) x ) & 0xFF) << 8)
#endif 

// ECC
#define NAND_NFIECC_1BIT                         	  14	// 1 bit ECC need 14 bit ecc code
#define NAND_NFIECC_ENCON                           0x00A00
#define NAND_NFIECC_ENCCNFG                         0x00A04
#if CC_NAND_60BIT_NFI
// 5398 60bit ECC ability
    #define NAND_NFIECC_ENCCNFG_ENC_TNUM_4              (((UINT32) 0) << 0)
    #define NAND_NFIECC_ENCCNFG_ENC_TNUM_8              (((UINT32) 2) << 0)
    #define NAND_NFIECC_ENCCNFG_ENC_TNUM_10             (((UINT32) 3) << 0)
    #define NAND_NFIECC_ENCCNFG_ENC_TNUM_12             (((UINT32) 4) << 0)
    #define NAND_NFIECC_ENCCNFG_ENC_TNUM_20             (((UINT32) 8) << 0)
    #define NAND_NFIECC_ENCCNFG_ENC_TNUM_22             (((UINT32) 9) << 0)
    #define NAND_NFIECC_ENCCNFG_ENC_TNUM_24             (((UINT32) 10) << 0)
    #define NAND_NFIECC_ENCCNFG_ENC_TNUM_28             (((UINT32) 11) << 0)
    #define NAND_NFIECC_ENCCNFG_ENC_TNUM_32             (((UINT32) 12) << 0)
    #define NAND_NFIECC_ENCCNFG_ENC_TNUM_36             (((UINT32) 13) << 0)
    #define NAND_NFIECC_ENCCNFG_ENC_TNUM_40             (((UINT32) 14) << 0)
    #define NAND_NFIECC_ENCCNFG_ENC_TNUM_44             (((UINT32) 15) << 0)
    #define NAND_NFIECC_ENCCNFG_ENC_TNUM_48             (((UINT32) 16) << 0)
    #define NAND_NFIECC_ENCCNFG_ENC_TNUM_52             (((UINT32) 17) << 0)
    #define NAND_NFIECC_ENCCNFG_ENC_TNUM_56             (((UINT32) 18) << 0)
    #define NAND_NFIECC_ENCCNFG_ENC_TNUM_60             (((UINT32) 19) << 0)
    #define NAND_NFIECC_ENCCNFG_ENC_NFI_MODE            (((UINT32) 1) << 5)
    #define NAND_NFIECC_ENCCNFG_ENC_MS_520              (((UINT32) 0x1040) << 16)
    #define NAND_NFIECC_ENCCNFG_ENC_MS_1032             (((UINT32) 0x2040) << 16)
    #define NAND_NFIECC_ENCCNFG_ENC_MS_1040             (((UINT32) 0x2080) << 16)
#else
// 24bit ECC ability
    #define NAND_NFIECC_ENCCNFG_ENC_TNUM_4              (((UINT32) 0) << 0)
    #define NAND_NFIECC_ENCCNFG_ENC_TNUM_8              (((UINT32) 2) << 0)
    #define NAND_NFIECC_ENCCNFG_ENC_TNUM_10             (((UINT32) 3) << 0)
    #define NAND_NFIECC_ENCCNFG_ENC_TNUM_12             (((UINT32) 4) << 0)
    #define NAND_NFIECC_ENCCNFG_ENC_TNUM_20             (((UINT32) 8) << 0)
    #define NAND_NFIECC_ENCCNFG_ENC_TNUM_24             (((UINT32) 10) << 0)
    #define NAND_NFIECC_ENCCNFG_ENC_NFI_MODE            (((UINT32) 1) << 4)
    #define NAND_NFIECC_ENCCNFG_ENC_MS_520              (((UINT32) 0x1040) << 16)
    #define NAND_NFIECC_ENCCNFG_ENC_MS_1032             (((UINT32) 0x2040) << 16)
    #define NAND_NFIECC_ENCCNFG_ENC_MS_1040             (((UINT32) 0x2080) << 16)
#endif 

#define NAND_NFIECC_ENCDIADDR                       0x00A08
#define NAND_NFIECC_ENCIDLE                         0x00A0C
#define NAND_NFIECC_ENCSTA                          0x00A24
    #define NAND_NFIECC_ENCSTA_FSM_MASK                 (((UINT32) 0x3F) << 0)
    #define NAND_NFIECC_ENCSTA_FSM_IDLE                 (((UINT32) 0) << 0)
    #define NAND_NFIECC_ENCSTA_FSM_WAITIN               (((UINT32) 1) << 0)
    #define NAND_NFIECC_ENCSTA_FSM_BUSY                 (((UINT32) 2) << 0)
    #define NAND_NFIECC_ENCSTA_FSM_PAROUT               (((UINT32) 4) << 0)
    #define NAND_NFIECC_ENCSTA_COUNT_PS                 (((UINT32) 0x1FF) << 7)
    #define NAND_NFIECC_ENCSTA_COUNT_MS                 (((UINT32) 0x3FFF) << 16)
#define NAND_NFIECC_ENCIRQEN                        0x00A28
#define NAND_NFIECC_ENCIRQSTA                       0x00A2C

#define NAND_NFIECC_ENCPAR0                         0x00A30
#if CC_NAND_60BIT_NFI
    #define NAND_NFI_ENCPAR_NUM                         27
#else
    #define NAND_NFI_ENCPAR_NUM                         11
#endif
#define NAND_NFIECC_DECCON                          0x00B00
#define NAND_NFIECC_DECCNFG                         0x00B04
#if CC_NAND_60BIT_NFI
    #define NAND_NFIECC_DECCNFG_DEC_TNUM_4              (((UINT32) 0) << 0)
    #define NAND_NFIECC_DECCNFG_DEC_TNUM_8              (((UINT32) 2) << 0)
    #define NAND_NFIECC_DECCNFG_DEC_TNUM_10             (((UINT32) 3) << 0)
    #define NAND_NFIECC_DECCNFG_DEC_TNUM_12             (((UINT32) 4) << 0)
    #define NAND_NFIECC_DECCNFG_DEC_TNUM_20             (((UINT32) 8) << 0)
    #define NAND_NFIECC_DECCNFG_DEC_TNUM_24             (((UINT32) 10) << 0)
    #define NAND_NFIECC_DECCNFG_DEC_TNUM_28             (((UINT32) 11) << 0)
    #define NAND_NFIECC_DECCNFG_DEC_TNUM_32             (((UINT32) 12) << 0)
    #define NAND_NFIECC_DECCNFG_DEC_TNUM_36             (((UINT32) 13) << 0)
    #define NAND_NFIECC_DECCNFG_DEC_TNUM_40             (((UINT32) 14) << 0)
    #define NAND_NFIECC_DECCNFG_DEC_TNUM_44             (((UINT32) 15) << 0)
    #define NAND_NFIECC_DECCNFG_DEC_TNUM_48             (((UINT32) 16) << 0)
    #define NAND_NFIECC_DECCNFG_DEC_TNUM_52             (((UINT32) 17) << 0)
    #define NAND_NFIECC_DECCNFG_DEC_TNUM_56             (((UINT32) 18) << 0)
    #define NAND_NFIECC_DECCNFG_DEC_TNUM_60             (((UINT32) 19) << 0)
    #define NAND_NFIECC_DECCNFG_DEC_NFI_MODE            (((UINT32) 1) << 5)
    #define NAND_NFIECC_DECCNFG_SEL_CHIEN_SEARCH        (((UINT32) 1) << 7)
#else
    #define NAND_NFIECC_DECCNFG_DEC_TNUM_4              (((UINT32) 0) << 0)
    #define NAND_NFIECC_DECCNFG_DEC_TNUM_8              (((UINT32) 2) << 0)
    #define NAND_NFIECC_DECCNFG_DEC_TNUM_10             (((UINT32) 3) << 0)
    #define NAND_NFIECC_DECCNFG_DEC_TNUM_12             (((UINT32) 4) << 0)
    #define NAND_NFIECC_DECCNFG_DEC_TNUM_20             (((UINT32) 8) << 0)
    #define NAND_NFIECC_DECCNFG_DEC_TNUM_24             (((UINT32) 10) << 0)
    #define NAND_NFIECC_DECCNFG_DEC_NFI_MODE            (((UINT32) 1) << 4)
#endif 
    #define NAND_NFIECC_DECCNFG_DEC_CON_NONE            (((UINT32) 1) << 12)
    #define NAND_NFIECC_DECCNFG_DEC_CON_SOFT            (((UINT32) 2) << 12)
    #define NAND_NFIECC_DECCNFG_DEC_CON_AUTO            (((UINT32) 3) << 12)
    #define NAND_NFIECC_DECCNFG_DEC_CS_520_4            (((UINT32) 0x1078) << 16)
    #define NAND_NFIECC_DECCNFG_DEC_CS_520_4_13			(((UINT32) 0x1074) << 16)
    #define NAND_NFIECC_DECCNFG_DEC_CS_520_10           (((UINT32) 0x10CC) << 16)
    #define NAND_NFIECC_DECCNFG_DEC_CS_520_10_13	    (((UINT32) 0x10C2) << 16)
    #define NAND_NFIECC_DECCNFG_DEC_CS_520_12_13		(((UINT32) 0x10DC) << 16)
    #define NAND_NFIECC_DECCNFG_DEC_CS_1032_8           (((UINT32) 0x20B0) << 16)
    #define NAND_NFIECC_DECCNFG_DEC_CS_1032_12          (((UINT32) 0x20E8) << 16)
    #define NAND_NFIECC_DECCNFG_DEC_CS_1040_8           (((UINT32) 0x20F0) << 16)
    #define NAND_NFIECC_DECCNFG_DEC_CS_1040_20          (((UINT32) 0x2198) << 16)
    #define NAND_NFIECC_DECCNFG_DEC_CS_1032_24          (((UINT32) 0x2190) << 16)
    #define NAND_NFIECC_DECCNFG_DEC_CS_1032_28          (((UINT32) 0x21C8) << 16)
    #define NAND_NFIECC_DECCNFG_DEC_CS_1032_32          (((UINT32) 0x2200) << 16)
    #define NAND_NFIECC_DECCNFG_DEC_CS_1032_36          (((UINT32) 0x2238) << 16)
    #define NAND_NFIECC_DECCNFG_DEC_CS_1032_40          (((UINT32) 0x2270) << 16)
    #define NAND_NFIECC_DECCNFG_DEC_CS_1032_44          (((UINT32) 0x22A8) << 16)
    #define NAND_NFIECC_DECCNFG_DEC_CS_1032_48          (((UINT32) 0x22E0) << 16)
    #define NAND_NFIECC_DECCNFG_DEC_CS_1032_52          (((UINT32) 0x2318) << 16)
    #define NAND_NFIECC_DECCNFG_DEC_CS_1032_56          (((UINT32) 0x2350) << 16)
    #define NAND_NFIECC_DECCNFG_DEC_CS_1032_60          (((UINT32) 0x2388) << 16)
    
    #define NAND_NFIECC_DECCNFG_DEC_CS_EMPTY_EN         (((UINT32) 1) << 31)

#define NAND_NFIECC_DECDIADDR                       0x00B08
#define NAND_NFIECC_DECIDLE                         0x00B0C
#define NAND_NFIECC_DECFER                          0x00B10
#define NAND_NFIECC_DECDONE                         0x00B18
#define NAND_NFIECC_DECIRQEN                        0x00B34
#define NAND_NFIECC_DECIRQSTA                       0x00B38
#define NAND_NFIECC_FDMADDR                         0x00B3C
#define NAND_NFIECC_DECFSM                          0x00B40
#define NAND_NFIECC_SYNSTA                          0x00B44
#define NAND_NFIECC_DECNFIDI                        0x00B48
#define NAND_NFIECC_SYN0                            0x00B4C
#define NAND_NFIECC_DECENUM0                        0x00B50
#define NAND_NFIECC_DECENUM1                        0x00B54
#define NAND_NFIECC_DECENUM2                        0x00B58
#define NAND_NFIECC_DECENUM3                        0x00B5C
#define NAND_NFIECC_DECEL0                          0x00B60
//#if CC_NAND_60BIT_NFI
#define NAND_SELECT_REGISTER                        0x00BF0
//#endif
#define NAND_DMMERGE_CFG                            0x00FF8
#define NAND_NFI_MLC_CFG                            0x00FFC+0xF0029000
    #define NAND_CHK_DRAM_WDLE_EN0                       (((UINT32) 1) << 29)
    #define NAND_CHK_DRAM_WDLE_EN                       (((UINT32) 1) << 30)
    #define NAND_CHK_DRAM_WDLE_EN2                      (((UINT32) 1) << 31)

//-----------------------------------------------------------------------------
// Common
//-----------------------------------------------------------------------------
// linux kernel has command define. (include\linux\mtd\nand.h)
#if 0
#define NAND_CMD_READ1_00                           0x00
#define NAND_CMD_READ1_01                           0x01
#define NAND_CMD_PROG_PAGE                          0x10 /* WRITE 2 */
#define NAND_CMD_READ_30                            0x30
#define NAND_CMD_READ2                              0x50
#define NAND_CMD_ERASE1_BLK                         0x60
#define NAND_CMD_STATUS                             0x70
#define NAND_CMD_INPUT_PAGE                         0x80    /* WRITE 1 */
#define NAND_CMD_READ_ID                            0x90
#define NAND_CMD_ERASE2_BLK                         0xD0
#define NAND_CMD_RESET                              0xFF
#endif

#define STATUS_WRITE_PROTECT                        0x80
#define STATUS_READY_BUSY                           0x40
#define STATUS_ERASE_SUSPEND                        0x20
#define STATUS_PASS_FAIL                            0x01

//-----------------------------------------------------------------------------
// Constant definitions
//-----------------------------------------------------------------------------
#define NAND_SUCC               (0)
#define NAND_FAIL               (1)
#define NAND_ECC_ERROR          (4)
#define NAND_CMD_ERROR          (8)
#define NAND_INVALID_ARG        (16)
#define POLLING_RETRY           (512*512)
#define NAND_MAX_PAGE_SIZE      (1024*8)
#define NAND_MAX_OOB_SIZE       (64*8)

//-----------------------------------------------------------------------------
// Macros of register read/write
//-----------------------------------------------------------------------------
#if CC_NAND_60BIT_NFI
#define NAND_REG_BASE                   ((UINT32)0xF0029400)
#else
#define NAND_REG_BASE                   ((UINT32)0xF0029000)
#endif
#define NAND_READ16(offset)             (*((volatile UINT16*)(NAND_REG_BASE + offset)))
#define NAND_READ32(offset)             (*((volatile UINT32*)(NAND_REG_BASE + offset)))
#define NAND_WRITE32(offset, _val_)     (*((volatile UINT32*)(NAND_REG_BASE + offset)) = (_val_))

#define NAND_IO_READ32(offset)          (*((volatile UINT32*)(offset)))
#define NAND_IO_WRITE32(offset, _val_)  (*((volatile UINT32*)(offset)) = (_val_))

#if defined(YES_BIMSWAP)
#define NAND_BIMSWAP(X)                 ((((X) & 0xf00) > 0x200) ? (X) : (((X) & 0xffffffeb) | (((X) & 0x10) >> 2) | (((X) & 0x4) << 2)))
#else
#define NAND_BIMSWAP(X)                 (X)
#endif

#define NAND_BIM_READ32(offset)         (*((volatile UINT32*)(BIM_VIRT + NAND_BIMSWAP(offset))))
#define NAND_BIM_WRITE32(offset, _val_) (*((volatile UINT32*)(BIM_VIRT + NAND_BIMSWAP(offset))) = (_val_))

//-----------------------------------------------------------------------------
// Device information 
//-----------------------------------------------------------------------------

#define NAND_BBMODE_BBMARK	0x0000FF00
#define NAND_BBMODE_ECCBIT	0x000000FF

typedef struct
{
    CHAR   acName[32];
    UINT8  u1AddrCycle;
    UINT32 u4IDVal;
    UINT32 u4IDMask;
    UINT32 u4PageSize;
    UINT32 u4OOBSize;
    UINT32 u4BBMode;
    UINT32 u4BlkSize;    
    UINT32 u4BlkPgCount;
    UINT32 u4ChipSize;
    UINT32 u4AccCon;
    UINT32 u4RdAcc;
} NAND_FLASH_DEV_T;

const static NAND_FLASH_DEV_T _arNand_DevInfo[] =
{
    { "SS K9F2808U0B",         3, 0x000073EC, 0x0000FFFF, 512,  16, 0x00000501,  0x04000,  32,  0x01000000, 0x00411888, 0x00080008},
    { "SS K9F5608U0B",         3, 0x000075EC, 0x0000FFFF, 512,  16, 0x00000501,  0x04000,  32,  0x02000000, 0x00411888, 0x00080008},
    { "SS K9F1208U0B/C",       4, 0x000076EC, 0x0000FFFF, 512,  16, 0x00000501,  0x04000,  32,  0x04000000, 0x00411888, 0x00080008},
    { "SS K9F1G08U0B/C/D",     4, 0x9500F1EC, 0xFFFFFFFF, 2048, 16, 0x00000001,  0x20000,  64,  0x08000000, 0x00433322, 0x00050005},
    { "SS K9F2G08U0B/C",       5, 0x9510DAEC, 0xFFFFFFFF, 2048, 16, 0x00000001,  0x20000,  64,  0x10000000, 0x00433322, 0x00050005},
    { "SS K9F4G08U0B/D",       5, 0x9510DCEC, 0xFFFFFFFF, 2048, 16, 0x00000001,  0x20000,  64,  0x20000000, 0x00433322, 0x00050005},
    { "SS K9K8G08U0D",         5, 0x9551D3EC, 0xFFFFFFFF, 2048, 16, 0x00000001,  0x20000,  64,  0x40000000, 0x00433322, 0x00050005},
    { "SS K9F8G08U0M",         5, 0xA610D3EC, 0xFFFFFFFF, 4096, 16, 0x00000001,  0x40000,  64,  0x40000000, 0x00433322, 0x00050005},
    { "SS K9GAG08U0E",         5, 0x7284D5EC, 0xFFFFFFFF, 8192, 26, 0x00000018,  0x100000, 128, 0x80000000, 0x00433322, 0x00050005},
    { "SS K9GAG08U0F",         5, 0x7694D5EC, 0xFFFFFFFF, 8192, 26, 0x00000018,  0x100000, 128, 0x80000000, 0x00433322, 0x00050005},
    { "SS 1Gb",                4, 0x0000F1EC, 0x0000FFFF, 2048, 16, 0x00000001,  0x20000,  64,  0x08000000, 0x00411888, 0x00080008},
    
    { "NAND128W3A2B",          3, 0x00007320, 0x0000FFFF, 512,  16, 0x00000501,  0x04000,  32,  0x01000000, 0x00411888, 0x00080008},
    { "NAND256W3A2B",          3, 0x00007520, 0x0000FFFF, 512,  16, 0x00000501,  0x04000,  32,  0x02000000, 0x00411888, 0x00080008},
    { "NAND512W3A2B/D",        4, 0x00007620, 0x0000FFFF, 512,  16, 0x00000501,  0x04000,  32,  0x04000000, 0x00411888, 0x00080008},
    { "NAND01GW3B2B",          4, 0x1D80F120, 0xFFFFFFFF, 2048, 16, 0x00000001,  0x20000,  64,  0x08000000, 0x00411888, 0x00080008},
    { "NAND01GW3B2C",          4, 0x1D00F120, 0xFFFFFFFF, 2048, 16, 0x00000001,  0x20000,  64,  0x08000000, 0x00433322, 0x00050005},
    { "NAND02GW3B2C",          5, 0x1D80DA20, 0xFFFFFFFF, 2048, 16, 0x00000001,  0x20000,  64,  0x10000000, 0x00411888, 0x00080008},
    { "NAND02GW3B2D",          5, 0x9510DA20, 0xFFFFFFFF, 2048, 16, 0x00000001,  0x20000,  64,  0x10000000, 0x00433322, 0x00050005},
    { "NAND04GW3B2B",          5, 0x9580DC20, 0xFFFFFFFF, 2048, 16, 0x00000001,  0x20000,  64,  0x20000000, 0x00411888, 0x00080008},
    { "NAND04GW3B2D",          5, 0x9510DC20, 0xFFFFFFFF, 2048, 16, 0x00000001,  0x20000,  64,  0x20000000, 0x00411888, 0x00080008},
    { "NAND08GW3B2C",          5, 0x9551D320, 0xFFFFFFFF, 2048, 16, 0x00000001,  0x20000,  64,  0x40000000, 0x00411888, 0x00080008},
    { "NAND08GW3C2B/16GW3C4B", 5, 0xA514D320, 0xFFFFFFFF, 2048, 16, 0x00000008,  0x40000,  128, 0x40000000, 0x00411888, 0x00080008},
    { "NAND08GW3F2A",          5, 0xA610D320, 0xFFFFFFFF, 4096, 16, 0x00000001,  0x40000,  64,  0x40000000, 0x00433322, 0x00050005},
    { "ST 1Gb",                4, 0x0000F120, 0x0000FFFF, 2048, 16, 0x00000001,  0x20000,  64,  0x08000000, 0x00411888, 0x00080008},
    { "ST 2Gb",                5, 0x0000DA20, 0x0000FFFF, 2048, 16, 0x00000001,  0x20000,  64,  0x10000000, 0x00411888, 0x00080008},
    { "ST 4Gb",                5, 0x0000DC20, 0x0000FFFF, 2048, 16, 0x00000001,  0x20000,  64,  0x20000000, 0x00411888, 0x00080008},
    { "ST 8Gb",                5, 0x0000D320, 0x0000FFFF, 4096, 16, 0x00000001,  0x40000,  64,  0x40000000, 0x00411888, 0x00080008},

    { "HY27US08281A",          3, 0x000073AD, 0x0000FFFF, 512,  16, 0x00000501,  0x04000,  32,  0x01000000, 0x00411888, 0x00080008},
    { "HY27US08561A",          3, 0x000075AD, 0x0000FFFF, 512,  16, 0x00000501,  0x04000,  32,  0x02000000, 0x00411888, 0x00080008},
    { "H27U518S2CTR/P",        4, 0x000076AD, 0x0000FFFF, 512,  16, 0x00000001,  0x04000,  32,  0x04000000, 0x00433322, 0x00050005},
    { "HY27UF081G2A",          4, 0x1D80F1AD, 0xFFFFFFFF, 2048, 16, 0x00000001,  0x20000,  64,  0x08000000, 0x00411888, 0x00080008},
    { "HY27U1G8F2BTR",         4, 0x1D00F1AD, 0xFFFFFFFF, 2048, 16, 0x00000001,  0x20000,  64,  0x08000000, 0x00433322, 0x00050005},
    { "HY27UF082G2B",          5, 0x9510DAAD, 0xFFFFFFFF, 2048, 16, 0x00000001,  0x20000,  64,  0x10000000, 0x00433322, 0x00050005},
    { "HY27U2G8F2CTR",         5, 0x9590DAAD, 0xFFFFFFFF, 2048, 16, 0x00000001,  0x20000,  64,  0x10000000, 0x00433322, 0x00050005},
    { "HY27U4G8F2D/ETR",       5, 0x9590DCAD, 0xFFFFFFFF, 2048, 16, 0x00000001,  0x20000,  64,  0x20000000, 0x00433433, 0x00060006},
    { "HY27UF084G2B",          5, 0x9510DCAD, 0xFFFFFFFF, 2048, 16, 0x00000001,  0x20000,  64,  0x20000000, 0x00433322, 0x00050005},
    { "HY27UT088G2A",          5, 0xA514D3AD, 0xFFFFFFFF, 2048, 16, 0x00000008,  0x40000,  128, 0x40000000, 0x00411888, 0x00080008},
    { "H27U8G8T2BTR",          5, 0xB614D3AD, 0xFFFFFFFF, 4096, 16, 0x00000008,  0x80000,  128, 0x40000000, 0x00433322, 0x00050005},
    { "H27UAG8T2MTR",          5, 0xB614D5AD, 0xFFFFFFFF, 4096, 16, 0x00000008,  0x80000,  128, 0x80000000, 0x00433322, 0x00050005},
    { "H27UAG8T2ATR",          5, 0x2594D5AD, 0xFFFFFFFF, 4096, 26, 0x00000018,  0x80000,  128, 0x80000000, 0x00433322, 0x00050005},
    { "H27UAG8T2BTR",          5, 0x9A94D5AD, 0xFFFFFFFF, 8192, 26, 0x00000018,  0x200000, 256, 0x80000000, 0x00433322, 0x00050005},
    { "HYNIX 1Gb",             4, 0x0000F1AD, 0x0000FFFF, 2048, 16, 0x00000001,  0x20000,  64,  0x08000000, 0x00411888, 0x00080008},
    { "HYNIX 2Gb",             5, 0x0000DAAD, 0x0000FFFF, 2048, 16, 0x00000001,  0x20000,  64,  0x10000000, 0x00411888, 0x00080008},
    { "HYNIX 4Gb",             5, 0x0000DCAD, 0x0000FFFF, 2048, 16, 0x00000001,  0x20000,  64,  0x20000000, 0x00411888, 0x00080008},
    { "HYNIX 8Gb",             5, 0x0000D3AD, 0x0000FFFF, 2048, 16, 0x00000001,  0x40000,  128, 0x40000000, 0x00411888, 0x00080008},
    
    { "MT29F1G08AAC/ABAD/EA",  4, 0x9580F12C, 0xFFFFFFFF, 2048, 16, 0x00000008,  0x20000,  64,  0x08000000, 0x00433322, 0x00050005},
    { "MT29F2G08AAD",          5, 0x9580DA2C, 0xFFFFFFFF, 2048, 16, 0x00000001,  0x20000,  64,  0x10000000, 0x00433322, 0x00050005},
    { "MT29F2G08ABAE/FA",      5, 0x9590DA2C, 0xFFFFFFFF, 2048, 16, 0x00000001,  0x20000,  64,  0x10000000, 0x00433322, 0x00050005},
    { "MT29F4G08ABADA",        5, 0x9590DC2C, 0xFFFFFFFF, 2048, 16, 0x00000008,  0x20000,  64,  0x20000000, 0x00433322, 0x00050005},
    { "MT29F4G08ABAEA",        5, 0xA690DC2C, 0xFFFFFFFF, 4096, 26, 0x00000008,  0x40000,  64,  0x20000000, 0x00433322, 0x00050005},
    { "MT29F8G08ABABA",        5, 0x2600382C, 0xFFFFFFFF, 4096, 26, 0x00000008,  0x80000,  128, 0x40000000, 0x00433322, 0x00050005},
    { "MT29F8G08ABACA",        5, 0xA690D32C, 0xFFFFFFFF, 4096, 26, 0x00000008,  0x40000,  64,  0x40000000, 0x00433322, 0x00050005},
    { "MT29F16G08CBABA",       5, 0x4604482C, 0xFFFFFFFF, 4096, 26, 0x00000018,  0x100000, 256, 0x80000000, 0x00433322, 0x00050005},
    { "MT29F16G08CBACA",       5, 0x4A04482C, 0xFFFFFFFF, 4096, 26, 0x00000018,  0x100000, 256, 0x80000000, 0x00433322, 0x00050005},
    { "Micron 1Gb",            4, 0x0000F12C, 0x0000FFFF, 2048, 16, 0x00000001,  0x20000,  64,  0x08000000, 0x00411888, 0x00080008},
    { "Micron 2Gb",            5, 0x0000DA2C, 0x0000FFFF, 2048, 16, 0x00000001,  0x20000,  64,  0x10000000, 0x00411888, 0x00080008},
    { "Micron 16GB",           5, 0x0000D92C, 0x0000FFFF, 4096, 26, 0x00000004,  0x80000,  128, 0x80000000, 0x00411888, 0x00080008},

    { "TC58DVM92A5TAI0/J0",    4, 0x00007698, 0x0000FFFF, 512,  16, 0x00000501,  0x04000,  32,  0x04000000, 0x00433322, 0x00050005},
    { "TC58NVM9S3ETA00/I0",    4, 0x1500F098, 0xFFFFFFFF, 2048, 16, 0x00000001,  0x20000,  64,  0x04000000, 0x00433322, 0x00050005},
    { "TC58NVG0S3ETA00",       4, 0x1590D198, 0xFFFFFFFF, 2048, 16, 0x00000001,  0x20000,  64,  0x08000000, 0x00433322, 0x00050005},
    { "TC58NVG0S3ETA0B",       4, 0x1590F198, 0xFFFFFFFF, 2048, 16, 0x00000001,  0x20000,  64,  0x08000000, 0x00433322, 0x00050005},
    { "TC58NVG0S3HTA00",       4, 0x1580F198, 0xFFFFFFFF, 2048, 26, 0x00000008,  0x20000,  64,  0x08000000, 0x00433322, 0x00050005},
    { "TC58NVG1S3E/HTA00/I0",  5, 0x1590DA98, 0xFFFFFFFF, 2048, 16, 0x00000001,  0x20000,  64,  0x10000000, 0x00433322, 0x00050005},
    { "TC58NVG2S3ETA00/I0",    5, 0x1590DC98, 0xFFFFFFFF, 2048, 16, 0x00000001,  0x20000,  64,  0x20000000, 0x00433322, 0x00050005},
    { "TC58NVG2S0FTA00/I0",    5, 0x2690DC98, 0xFFFFFFFF, 4096, 26, 0x00000008,  0x40000,  64,  0x20000000, 0x00433322, 0x00050005},
    { "TC58D/NVG3S0E/FTA00",   5, 0x2690D398, 0xFFFFFFFF, 4096, 16, 0x00000008,  0x40000,  64,  0x40000000, 0x00433322, 0x00050005},
    { "TC58NVG4D2FTA00",       5, 0x3294D598, 0xFFFFFFFF, 8192, 26, 0x00000018,  0x100000, 128, 0x80000000, 0x00433322, 0x00050005},
    { "TH58NVG7DDJTA20",       5, 0x9394DE98, 0xFFFFFFFF, 16384,32, 0x00000018,  0x400000, 256, 0xFFFFFFFF, 0x00433322, 0x00050005},
			
    { "EN27LN2G08", 		   5, 0x9590DAC8, 0xFFFFFFFF, 2048, 16, 0x00000004,  0x20000,  64,	0x10000000, 0x00433322, 0x00050005},
    { "EN27LN4G08", 		   5, 0x9590DCC8, 0xFFFFFFFF, 2048, 16, 0x00000004,  0x20000,  64,	0x20000000, 0x00433322, 0x00050005},		
    { "MX30LF1G08AM-TIB",      4, 0x1D80F1C2, 0xFFFFFFFF, 2048, 16, 0x00000001,  0x20000,  64,  0x08000000, 0x00433322, 0x00050005},

    { "S34ML08G100TFI000",     5, 0x95D1D301, 0xFFFFFFFF, 2048, 16, 0x00000001,  0x20000,  64,  0x40000000, 0x00433322, 0x00050005},
    { "S34ML04G1/200TFI000",   5, 0x9590DC01, 0xFFFFFFFF, 2048, 16, 0x00000001,  0x20000,  64,  0x20000000, 0x00433322, 0x00050005},
    { "S34ML02G100TFI000",     5, 0x9590DA01, 0xFFFFFFFF, 2048, 16, 0x00000001,  0x20000,  64,  0x10000000, 0x00433322, 0x00050005},
    { "S34ML01G100TFI000",     4, 0x1D00F101, 0xFFFFFFFF, 2048, 16, 0x00000001,  0x20000,  64,  0x08000000, 0x00433322, 0x00050005},
    { "S34ML01G200TFI000",     4, 0x1D80F101, 0xFFFFFFFF, 2048, 16, 0x00000004,  0x20000,  64,  0x08000000, 0x00433322, 0x00050005},

	{ "Small 16MB",            3, 0x00007300, 0x0000FF00, 512,  16, 0x00000501,  0x04000,  32,  0x01000000, 0x00411888, 0x00080008},
    { "Small 32MB",            3, 0x00007500, 0x0000FF00, 512,  16, 0x00000501,  0x04000,  32,  0x02000000, 0x00411888, 0x00080008},
    { "Small 64MB",            4, 0x00007600, 0x0000FF00, 512,  16, 0x00000501,  0x04000,  32,  0x04000000, 0x00411888, 0x00080008},
    { "Small 128MB",           4, 0x00007900, 0x0000FF00, 512,  16, 0x00000501,  0x04000,  32,  0x08000000, 0x00411888, 0x00080008},
    { "Large 128MB",           4, 0x0000F100, 0x0000FF00, 2048, 16, 0x00000001,  0x20000,  64,  0x08000000, 0x00411888, 0x00080008},
};
#endif /* NAND_REG_H */

