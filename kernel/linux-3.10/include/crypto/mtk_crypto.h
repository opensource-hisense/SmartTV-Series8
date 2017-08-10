/*
 * include/crypto/mtk_crypto.h
 *
 * multi-stage power management devices
 *
 * Copyright (c) 2010-2012 MediaTek Inc.
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


#ifndef DMX_AES_REG_H
#define DMX_AES_REG_H

//-----------------------------------------------------------------------------
// Include files
//-----------------------------------------------------------------------------
#ifdef CONFIG_64BIT
#include <linux/soc/mediatek/x_typedef.h>
#include <linux/soc/mediatek/platform.h>
#else
#include <x_typedef.h>
#include <mach/hardware.h>
#endif


#if defined(CONFIG_ARCH_MT5365) || defined(CONFIG_ARCH_MT5395)
//-----------------------------------------------------------------------------
// Configurations
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Constant definitions
//-----------------------------------------------------------------------------


//
// Demux registers
//
#define AES_REG_DMA_SRC                         (0)     // 0x0
#define AES_REG_DMA_DST							(1)     // 0x4
#define AES_REG_DMA_MODE                        (2)     // 0x8
#define AES_REG_DMA_TRIG                        (3)     // 0xC
#define AES_REG_FILL_DATA                       (4)     // 0x10
#define AES_REG_AES_MODE                        (8)     // 0x20
#define AES_REG_BUS_CTL                         (9)     // 0x24
#define AES_REG_AES_IV0                         (12)    // 0x30
#define AES_REG_AES_IV1                         (13)    // 0x34
#define AES_REG_AES_IV2				            (14)    // 0x38
#define AES_REG_AES_IV3                         (15)    // 0x3c
#define AES_REG_AES_KEY0_0                      (16)    // 0x40
#define AES_REG_AES_KEY0_1                      (17)    // 0x44
#define AES_REG_AES_KEY0_2                      (18)    // 0x48
#define AES_REG_AES_KEY0_3                      (19)    // 0x4C
#define AES_REG_AES_KEY0_4                      (20)    // 0x50
#define AES_REG_AES_KEY0_5                      (21)    // 0x54
#define AES_REG_AES_KEY0_6                      (22)    // 0x58
#define AES_REG_AES_KEY0_7                      (23)    // 0x5C
#define AES_REG_AES_KEY1_0                      (24)    // 0x60
#define AES_REG_AES_KEY1_1                      (25)    // 0x64
#define AES_REG_AES_KEY1_2                      (26)    // 0x68
#define AES_REG_AES_KEY1_3                      (27)    // 0x6C
#define AES_REG_AES_KEY1_4                      (28)    // 0x70
#define AES_REG_AES_KEY1_5                      (29)    // 0x74
#define AES_REG_AES_KEY1_6                      (30)    // 0x78
#define AES_REG_AES_KEY1_7                      (31)    // 0x7C

#define AES_DMA_MODE_B_TO_T                     ((UINT32)1 << 31)
#define AES_DMA_MODE_T_TO_B                     ((UINT32)0 << 31)
#define AES_DMA_MODE_FILL                       ((UINT32)1 << 30)

//-----------------------------------------------------------------------------
// Type definitions
//-----------------------------------------------------------------------------
typedef struct
{
    UINT8 au1Key[32];
    UINT8 au1InitVector[16];
    UINT16 u2KeyLen;
    BOOL fgCbc;
    BOOL fgEncrypt;
    UINT32 u4InBufStart;
    UINT32 u4OutBufStart;
    UINT32 u4BufSize;
    BOOL fgKey1;
} DMX_AES_PARAM_T;

//-----------------------------------------------------------------------------
// Macro definitions
//-----------------------------------------------------------------------------
#define HAL_READ32(_reg_)               (*((volatile UINT32*)(_reg_)))
#define HAL_WRITE32(_reg_, _val_)       (*((volatile UINT32*)(_reg_)) = (_val_))
#define IO_READ32(base, offset)         HAL_READ32((base) + (offset))
#define IO_WRITE32(base, offset, value) HAL_WRITE32((base) + (offset), (value))

#define IO_WRITE32MSK(base, offset, value, msk) \
        IO_WRITE32((base), (offset), \
                   (IO_READ32((base), (offset)) & ~(msk)) | ((value) & (msk)))
///
/// Demux register access commands
///
#define AES_READ32(offset)			(*((volatile UINT32*)(AES_VIRT + ((offset) * 4))))
#define AES_WRITE32(offset, value)	(*((volatile UINT32*)(AES_VIRT + ((offset) * 4))) = (value))


#define REG_WFIFO       (0x0114)        // WFIFO CONTROL REGISTER
#define FLUSH_CMD       (1U << 4)       // CPU-DRAM WFIFO flush command when AUTO_FLUSH disable
#define FLUSH_FIFO IO_WRITE32MSK(BIM_VIRT, REG_WFIFO, FLUSH_CMD, FLUSH_CMD)

//-----------------------------------------------------------------------------
// Prototype  of inter-file functions
//-----------------------------------------------------------------------------
BOOL NAND_AES_INIT(void);
BOOL NAND_AES_Encryption(UINT32 u4InBufStart, UINT32 u4OutBufStart, UINT32 u4BufSize);
BOOL NAND_AES_Decryption(UINT32 u4InBufStart, UINT32 u4OutBufStart, UINT32 u4BufSize);

#else
//-----------------------------------------------------------------------------
// Constant definitions
//-----------------------------------------------------------------------------

//#define GCPU_BASE 0xf0016000
//#define IOMMU_GCPU_BASE 0xf0068040
#define IOMMU_GCPU_BASE     (0x40)
//#define VECTOR_GCPU 44
//#define VECTOR_GCPU_IOMMU 59

//#define CC_MEASURE_TIME

//
// GCPU Registers
//
#define GCPU_REG_CTL                0 //(0x0)
#define GCPU_REG_MSC                1 //(0x4)

#define GCPU_REG_PC_CTL             256 //(0x400)
#define GCPU_REG_MEM_ADDR           257 //(0x404)
#define GCPU_REG_MEM_DATA           258 //(0x408)

#define GCPU_REG_MONCTL             261 //(0x414)

#define GCPU_REG_DRAM_INST_BASE     264 //(0x420)

#define GCPU_REG_TRAP_START         272 // (0x440)
#define GCPU_REG_TRAP_END           286 // (0x478)

#define GCPU_REG_INT_SET            512 //(0x800)
#define GCPU_REG_INT_CLR            513 //(0x804)
#define GCPU_REG_INT_EN             514 //(0x808)

#define GCPU_REG_MEM_CMD            768 //(0xC00)
#define GCPU_REG_MEM_P0             769 //(0xC04)
#define GCPU_REG_MEM_P1             770 //(0xC08)
#define GCPU_REG_MEM_P2             771 //(0xC0C)
#define GCPU_REG_MEM_P3             772 //(0xC10)
#define GCPU_REG_MEM_P4             773 //(0xC14)
#define GCPU_REG_MEM_P5             774 //(0xC18)
#define GCPU_REG_MEM_P6             775 //(0xC1C)
#define GCPU_REG_MEM_P7             776 //(0xC20)
#define GCPU_REG_MEM_P8             777 //(0xC24)
#define GCPU_REG_MEM_P9             778 //(0xC28)
#define GCPU_REG_MEM_P10            779 //(0xC2C)
#define GCPU_REG_MEM_P11            780 //(0xC30)
#define GCPU_REG_MEM_P12            781 //(0xC34)
#define GCPU_REG_MEM_P13            782 //(0xC38)
#define GCPU_REG_MEM_P14            783 //(0xC3C)
#define GCPU_REG_MEM_Slot           784 //(0xC40)

#define REG_IOMMU_CFG0              0x0//0x000        // basic setting
#define REG_IOMMU_CFG1              0x1//0x004        // page table index
#define REG_IOMMU_CFG2              0x2//0x008        // agnet_0~1 setting
#define REG_IOMMU_CFG4              0x4//0x010        // interrupt, monitor and debug
#define REG_IOMMU_CFG5              0x5//0x010
#define REG_IOMMU_CFG6              0x6//0x010
#define REG_IOMMU_CFG7              0x7//0x010
#define REG_IOMMU_CFG8              0x8//0x010
#define REG_IOMMU_CFG9              0x9//0x010
#define REG_IOMMU_CFGA              0xA//0x010
#define REG_IOMMU_CFGB              0xb//0x010
#define REG_IOMMU_CFGC              0xc//0x010
#define REG_IOMMU_CFGD              0xd//0x010

//BIM register
#define TIMER_LOW_REG       		0x074
#define TIMER_HIGH_REG      		0x194

//
// Interrupt masks
//
#define GCPU_INT_MASK               (0x1)





#define GCPU_PARAM_NUM              (48)
#define GCPU_PARAM_RET_PTR          (32)
#define GCPU_PARAM_RET_NUM          (GCPU_PARAM_NUM - GCPU_PARAM_RET_PTR)


#define AES_DPAK                    (0x23)         // AES Packet Decryption
#define AES_EPAK                    (0x24)         // AES Packet Encryption
#define SHA_1                       (0x40)         // SHA-1 Algorithm
#define SHA_256                     (0x41)         // SHA-256 Algorithm
#define GCPU_SHA_1_PACKET_SIZE      (64) //(512bits)
#define GCPU_SHA_256_PACKET_SIZE      (64) 			//(512bits)
#define SHA_256_MAX_SIZE            (4096)        // NFSB max sha 256 buffer size 4096


#define AES_MTD_SECURE_KEY_PTR      (92)

#define GCPU_FIFO_ALIGNMENT         (16)

//-----------------------------------------------------------------------------
// Type definitions
//-----------------------------------------------------------------------------
// GCPU HW CMD Struct
typedef struct
{
  UINT32 u4Param[GCPU_PARAM_NUM];
} GCPU_HW_CMD_T;


// Make size be the multiple of 32 bytes (cache line)
typedef struct
{
    UINT32 u4InitRam;
    UINT32 u4InitDram;
    UINT32 *pu4RamCode;
    UINT32 u4RamCodeSize;
    UINT32 *pu4Trapping;
    UINT32 u4TrapSize;
    UINT32 u4PhyDramCodeAddr;
    UINT32 u4ReturnedValue;
    UINT32 u4MmuSrc1Start;
    UINT32 u4MmuSrc1End;
    UINT32 u4MmuSrc2Start;
    UINT32 u4MmuSrc2End;
    UINT32 u4MmuTblAddr;
    UINT32 u4SlotIdx;
    UINT32 u4SlotData;
    UINT32 u4Padding[1];
} GCPU_TZ_PARAM_T;

typedef enum
{
    HASH_FIRST_PACKET_MODE,
    HASH_SUCCESSIVE_PACKET_MODE,
} HASH_MODE;

typedef struct
{
    UINT32 u4SrcStartAddr;
    UINT32 u4SrcBufStart;
    UINT32 u4SrcBufEnd;
    UINT32 u4DatLen;
    UINT64 u8BitCnt;
    UINT8 au1Hash[32];
    BOOL fgFirstPacket;
    BOOL fgLastPacket;
} MD_PARAM_T;

//=====================================================================
// Type definitions

// Note: For a system with 100Hz timer tick, an UINT32 can only represent
// about 500 days. It may be insufficient for certain cases, but it's more
// convenient and efficient to process a 32-bit integer rather than a 64-
// bit integer.
//
typedef struct
{
	UINT32		u4Ticks;			// Number of timer interrupts from startup
	UINT32		u4Cycles;			// System cycles from last timer interrupt
} HAL_RAW_TIME_T;

typedef struct
{
	UINT32		u4Seconds;			// Number of seconds from startup
	UINT32		u4Micros;			// Remainder in microsecond
} HAL_TIME_T;



//-----------------------------------------------------------------------------
// Macro definitions
//-----------------------------------------------------------------------------
#define KEY_WRAPPER(p)  ((p[3] << 24) | (p[2] << 16) | (p[1] << 8) | (p[0]))
#define SwitchValue(u4Value) \
              ((u4Value << 24) & 0xFF000000) + ((u4Value << 8) & 0x00FF0000) + \
              ((u4Value >> 8) & 0x0000FF00) + ((u4Value >> 24) & 0x000000FF)

#define HAL_READ32(_reg_)               (*((volatile UINT32*)(_reg_)))
#define HAL_WRITE32(_reg_, _val_)       (*((volatile UINT32*)(_reg_)) = (_val_))
#define IO_READ32(base, offset)         HAL_READ32((base) + (offset))
#define IO_WRITE32(base, offset, value) HAL_WRITE32((base) + (offset), (value))

#define IO_WRITE32MSK(base, offset, value, msk) \
        IO_WRITE32((base), (offset), \
                   (IO_READ32((base), (offset)) & ~(msk)) | ((value) & (msk)))
///
/// Gcpu register access commands
///
#define GCPUCMD_READ32(offset)			IO_READ32(pGcpuIoMap, ((offset) * 4))
#define GCPUCMD_WRITE32(offset, value)	IO_WRITE32(pGcpuIoMap, ((offset) * 4), (value))
#define IOMMU_GCPU_READ32(offset)			IO_READ32(pMmuIoMap, ((offset) * 4) + IOMMU_GCPU_BASE)
#define IOMMU_GCPU_WRITE32(offset, value)	IO_WRITE32(pMmuIoMap, ((offset) * 4) + IOMMU_GCPU_BASE, (value))

#define GCPU_LINER_BUFFER_START(u4Addr) (((u4Addr) % GCPU_FIFO_ALIGNMENT) == 0)?(u4Addr): \
                                    (u4Addr) - ((u4Addr) % GCPU_FIFO_ALIGNMENT)

#define GCPU_LINER_BUFFER_END(u4Addr) (((u4Addr) % GCPU_FIFO_ALIGNMENT) == 0)?(u4Addr): \
                                    (u4Addr) + (GCPU_FIFO_ALIGNMENT - ((u4Addr) % GCPU_FIFO_ALIGNMENT))
// BIM register access commands
#define BIM_BASE            0xF0008000
#define BIM_READ32(offset)          IO_READ32(BIM_BASE,(offset))
#define BIM_WRITE32(offset, value)  IO_WRITE32(BIM_BASE,(offset), (value))

// Time measure
#define _u4TimerInterval    27000000

#define ADDR_INCR_IN_RING(addr, incr, ringstart, ringend)      \
    ((((addr) + (incr)) < (ringend)) ? ((addr) + (incr)) : (((addr) + (incr)) - ((ringend) - (ringstart))))

//-----------------------------------------------------------------------------
// Prototype  of inter-file functions
//-----------------------------------------------------------------------------
BOOL NAND_AES_INIT(void);
BOOL NAND_AES_Encryption(UINT32 u4InBufStart, UINT32 u4OutBufStart, UINT32 u4BufSize);
BOOL NAND_AES_Decryption(UINT32 u4InBufStart, UINT32 u4OutBufStart, UINT32 u4BufSize);

struct shash_desc;
int SHA256_HW_INIT(struct shash_desc *desc);
int SHA256_HW_UPDATE(struct shash_desc *desc, const u8 *data, unsigned int len);

#ifdef CC_MEASURE_TIME
void HAL_GetRawTime(HAL_RAW_TIME_T* pRawTime);
void HAL_RawToTime(const HAL_RAW_TIME_T* pRawTime, HAL_TIME_T* pTime);
void HAL_GetTime(HAL_TIME_T* pTime);
void HAL_GetDeltaTime(HAL_TIME_T* pResult, HAL_TIME_T* pT0,HAL_TIME_T* pT1);
void HAL_GetSumTime(HAL_TIME_T* pResult, HAL_TIME_T* pT0);

#endif //CC_MEASURE_TIME

#endif

#endif  // DMX_AES_REG_H

