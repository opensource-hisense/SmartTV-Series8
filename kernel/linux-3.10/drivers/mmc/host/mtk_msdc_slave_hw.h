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
* [File]			msdc_slave_hw.h
* [Version]			v1.0
* [Revision Date]	2011-05-04
* [Author]			Shunli Wang, shunli.wang@mediatek.inc, 82896, 2011-05-04
* [Description]
*	MSDC Slave Device Header File
* [Copyright]
*	Copyright (C) 2011 MediaTek Incorporation. All Rights Reserved.
******************************************************************************/

#ifndef _MSDC_SLAVE_HW_H_
#define _MSDC_SLAVE_HW_H_
#include "x_typedef.h"

//---------------------------------------------------------------------------
// Configurations
//---------------------------------------------------------------------------
/* Card Type */
#define CARDTYPE_NOT_SD				0
#define CARDTYPE_MEM_STD			1
#define CARDTYPE_MEM_HC				2
#define CARDTYPE_MEM_MMC			3
#define CARDTYPE_IO					4
#define CARDTYPE_COMBO_STD			5
#define CARDTYPE_COMBO_HC			6

/* Voltage Type */
#define VOLTAGE_NORMAL_RANGE		0x1
#define VOLTAGE_LOW_RANGE			0x2

struct __sd_cid_s__ 
{
	UINT64 resv0			:1;
	UINT64 crc				:7;
	UINT64 mdt				:12;
	UINT64 resv1			:4;
	UINT64 psn				:32;
	UINT64 prv				:8;

	UINT64 pnm				:40;

	UINT64 oid				:16;
	UINT64 mid				:8;
};
typedef struct __sd_cid_s__  sd_cid_t;
  
struct __csd_10_s__ 
{
	UINT32 resv0					:1;
	UINT32 crc					:7;
	UINT32 resv1					:2;
	UINT32 fileFmt				:2;
	UINT32 tmpWrProtect			:1;
	UINT32 permWrProtect			:1;
	UINT32 copy					:1;
	UINT32 fileFmtGrp				:1;

	UINT32 resv2					:5;
	UINT32 wrBlPartial			:1;
	UINT32 wrBlLen				:4;
	UINT32 r2wFact				:3;
	UINT32 resv3					:2;
	UINT32 wpGrpEnable			:1;

	UINT64 wpGrpsize		:7;
	UINT64 sectorSize		:7;
	UINT64 eraseBlkEn		:1;
	UINT64 cSizeMult		:3;
	UINT64 vddWCurrMax		:3;
	UINT64 vddWCurrMin		:3;
	UINT64 vddRCurrMax		:3;
	UINT64 vddRCurrMin		:3;
	UINT64 cSize			:12;
	UINT64 resv4			:2;
	UINT64 dsrImp			:1;
	UINT64 rdBlkMisAlign	:1;
	UINT64 wrBlkMisAlign	:1;
	UINT64 rdBlPartial		:1;
	UINT64 rdBlkLen			:4;
	UINT64 ccc				:12;
};
typedef struct __csd_10_s__  csd_10_t;

struct __csd_20_s__ 
{
	UINT32 resv0					:1;
	UINT32 crc					:7;

	UINT32 resv1					:2;
	UINT32 fileFmt				:2;
	UINT32 tmpWrProtect			:1;
	UINT32 permWrProtect			:1;
	UINT32 copy					:1;
	UINT32 fileFmtGrp				:1;
	UINT32 resv2					:5;
	UINT32 wrBlPartial			:1;
	UINT32 wrBlLen				:4;
	UINT32 r2wFactor				:3;
	UINT32 resv3					:2;
	UINT32 wpGrpEnable			:1;

	UINT64 wpGrpSize		:7;
	UINT64 sectorSize		:7;
	UINT64 eraseBlkLen		:1;
	UINT64 resv4			:1;
	UINT64 cSize			:22;
	UINT64 resv5			:6;
	UINT64 dsrImp			:1;
	UINT64 rdBlkMisAlign	:1;
	UINT64 wrBlkMisAlign	:1;
	UINT64 rdBlPartial		:1;
	UINT64 rdBlLen			:4;
	UINT64 ccc				:12;

};
typedef struct __csd_20_s__  csd_20_t;

struct __sd_csd_s__ 
{

	union {
		csd_10_t v10;
		csd_20_t v20;
	} csdSub;

	UINT32 tranSpeed				:8;
	UINT32 nsac					:8;
	UINT32 taac					:8;
	UINT32 resv0					:6;
	UINT32 csdStructure			:2;
}  __attribute ((packed));
typedef struct __sd_csd_s__  sd_csd_t;
  
struct __card_status_s__ 
{
	UINT32 resv0					:3;
	UINT32 akeSeqErr				:1;
	UINT32 resv1					:1;
	UINT32 appCmd					:1;
	UINT32 resv2					:2;
	UINT32 rdyForData				:1;
	UINT32 currStat				:4;
	UINT32 eraseReset				:1;
	UINT32 cardEccDisabled		:1;
	UINT32 wpEraseSkip			:1;
	UINT32 csdOverwrite			:1;
	UINT32 resv3					:2;
	UINT32 error					:1;
	UINT32 ccError				:1;
	UINT32 cardEccFailed			:1;
	UINT32 illegalCmd				:1;
	UINT32 comCrcErr				:1;
	UINT32 lockUnlockFail			:1;
	UINT32 cardIsLocked			:1;
	UINT32 wpViolation			:1;
	UINT32 eraseParam				:1;
	UINT32 eraseSeqErr			:1;
	UINT32 blkLenErr				:1;
	UINT32 addressErr				:1;
	UINT32 outOfRange				:1;
};
typedef struct __card_status_s__  card_status_t;

struct __mem_ocr_s__ 
{
	UINT32 ocr					:24;
	UINT32 resv0					:6;
	UINT32 hcs					:1;
	UINT32 memRdy					:1;
};
typedef struct __mem_ocr_s__  mem_ocr_t;

struct __io_ocr_s__ 
{
	UINT32 ocr					:24;
	UINT32 resv0					:3;
	UINT32 memPresent				:1;
	UINT32 nof					:3;
	UINT32 ioRdy					:1;
};
typedef struct __io_ocr_s__  io_ocr_t;

struct __io_r5_s__ 
{
	UINT32 rwData					:8;
	UINT32 respFlag				:8;
	UINT32 resv					:16;
};
typedef struct __io_r5_s__  io_r5_t;

struct __sd_scr_s__ 
{
	UINT32 manufactureResv;
	UINT32 resv0					:16;
	UINT32 busWidth				:4;
	UINT32 security				:3;
	UINT32 dsAfterErase			:1;
	UINT32 spec					:4;
	UINT32 scrStructure			:4;
};
typedef struct __sd_scr_s__  sd_scr_t;

#define CSD_LENGTH				(4)

typedef struct __sd_card_s__ 
{

	UINT32 cardType;
	UINT32 blkAddrMode;

  // Common Card Definitions
	sd_cid_t cid;
	csd_20_t csd;

  // Flag for Memory initialized status
	UINT32 memInitDone;		

  // Memory Card Definitions
	UINT32 memRca;
	mem_ocr_t memOcr;	
	
  // Flag
  UINT32 flagHost;
} sdcard_t;

struct __sd_status_s__ 
{
	UINT32 resv0[12];				// 384 bits

	UINT64 resv1			:16;
	UINT64 eraseOffset		:2;
	UINT64 eraseTimeout		:6;
	UINT64 eraseSize		:16;
	UINT64 resv2			:4;
	UINT64 auSize			:4;
	UINT64 performanceMove	:8;
	UINT64 speedClass		:8;

	UINT32 sizeOfProtArea;

	UINT32 sdCardType				:16;
	UINT32 resv3					:13;
	UINT32 securedMode			:1;
	UINT32 dataBusWidth			:2;
};
typedef struct __sd_status_s__ sd_status_t;

#define CMD_TYPE_NORMAL				    0
#define CMD_TYPE_ACMD				    1

/* SDC_CMD */
/* Standard SD 2.0 Commands						Type	Arguments			Response	*/
/* Class 0 */
#define CMD0_GO_IDLE_STATE				0	/*	bc										*/
#define CMD1_MMC_SEND_OP_COND			1	/*	bcr		[23:0] OCR			R3			*/
#define CMD2_ALL_SEND_CID				2	/*	bcr							R2			*/
#define CMD3_SEND_RELATIVE_ADDR			3	/*	bcr							R6			*/
#define CMD4_SET_DSR					4	/*	bc		[31:16] DSR						*/
#define CMD6_MMC_SWITCH				    6	/*	ac		[1:0] Bus width		R1B			*/
#define CMD7_SELECT_CARD				7	/*	ac		[31:16] RCA			R1b			*/
#define CMD8_SEND_IF_COND				8	/*	bcr		[11:8] VHS			R7			*/
#define CMD8_MMC_SEND_EXT_CSD		    8	/*	adtc		[31:0] stuff bits			R1		*/
#define CMD9_SEND_CSD					9	/*	ac		[31:16] RCA			R2			*/
#define CMD10_SEND_CID					10	/*	ac		[31:16] RCA			R2			*/
#define CMD12_STOP_TRANSMISSION			12	/*	ac							R1b			*/
#define CMD13_SEND_STATUS				13	/*	ac		[31:16] RCA			R1b			*/
#define CMD15_GO_INACTIVE_STATE			15	/*	ac		[31:16] RCA						*/
	
/* Class 2 */
#define CMD16_SET_BLOCKLEN				16	/*	ac		[31:0] blk len		R1			*/
#define CMD17_READ_SINGLE_BLOCK			17	/*	adtc	[31:0] data addr.	R1			*/
#define CMD18_READ_MULTIPLE_BLOCK		18	/*	adtc	[31:0] data addr.	R1			*/

/* Class 4 */
#define	CMD24_WRITE_BLOCK				24	/*	adtc	[31:0] data addr.	R1			*/
#define CMD25_WRITE_MULTIPLE_BLOCK		25	/*	adtc	[31:0] data addr.	R1			*/
#define CMD27_PROGRAM_CSD				27	/*	adtc						R1			*/

/* Class 6 */
#define CMD28_SET_WRITE_PROT			  28	/*	ac		[31:0] data addr.	R1b			*/
#define CMD29_CLR_WRITE_PROT			  29	/*	ac		[31:0] data addr.	R1b			*/
#define CMD30_SEND_WRITE_PROT			  30	/*	adtc	[31:0] prot addr.	R1			*/
#define CMD31_SEND_WRITE_PROT_TYPE  31	/*	adtc	[31:0] prot addr.	R1			*/

/* Class 5 */
#define CMD32_ERASE_WR_BLK_START		32	/*	ac		[31:0] data addr.	R1			*/
#define CMD33_ERASE_WR_BLK_END			33	/*	ac		[31:0] data addr.	R1			*/
#define CMD35_ERASE_WR_BLK_START		35	/*	ac		[31:0] data addr.	R1			*/
#define CMD36_ERASE_WR_BLK_END			36	/*	ac		[31:0] data addr.	R1			*/
#define CMD38_ERASE						38	/*	ac							R1b			*/

/* Class 7 */
#define CMD42_LOCK_UNLOCK				42	/*	ac		[31:0] Reserved		R1			*/

/* Class 8 */
#define CMD55_APP_CMD					55	/*	ac		[31:16] RCA			R1			*/
#define CMD56_GEN_CMD					56	/*	adtc	[0] RD/WR			R1			*/

/* Application Specific Cmds */
#define ACMD6_SET_BUS_WIDTH				6	/*	ac		[1:0] Bus width		R1			*/
#define ACMD13_SD_STATUS				13	/*	adtc						R1			*/
#define ACMD22_SEND_NUM_WR_BLOCKS		22	/*	adtc						R1			*/
#define ACMD23_SET_WR_BLK_ERASE_COUNT	23	/*	ac		[22:0] Blk num.		R1			*/
#define ACMD41_SD_SEND_OP_COND			41	/*	bcr		[23:0] OCR			R3			*/
#define ACMD42_SET_CLR_CARD_DETECT		42	/*	ac		[0] Set cd			R1			*/
#define ACMD51_SEND_SCR					51	/*	adtc						R1			*/

/* IO Card Commands */
#define CMD5_IO_SEND_OP_COND			5	/*	ac		[24:0] OCR			R4			*/
#define CMD52_IO_RW_DIRECT				52	/*	ac							R5			*/
#define CMD53_IO_RW_EXTENDED			53	/*	ac							R5			*/

/* Checksum Option for fill_chksum */
#define CHKSUM_OPT_NONE			        (0x00)
#define CHKSUM_OPT_GPD			        (0x01)
#define CHKSUM_OPT_BD			        (0x02)
#define CHKSUM_OPT_ERR_IN_GPD	        (0x04)
#define CHKSUM_OPT_ERR_IN_BD	        (0x08)

#define ACMD41_BUS_WIDTH_1				(0x00)
#define ACMD41_BUS_WIDTH_4				(0x02)
#define ACMD41_BUS_WIDTH_8				(0xFF)	//TODO: UNDEFINED!!

// MMC CMD6 Argument :
// (1) Bus Width Selection :
// Access bits = 0x03 (Write byte), Index = 0xB7 = 183, Value = 0(1bit), 1(4bits), 2(8bits)
#define MMC_CMD6_ARG_1BIT_BUS            0x03B70000
#define MMC_CMD6_ARG_4BIT_BUS            0x03B70100
#define MMC_CMD6_ARG_8BIT_BUS            0x03B70200

// (2) High SPeed Mode Selection :
// Access bits = 0x03 (Write byte), Index = 0xB9 = 185, Value = 0(26 Mhz max), 1(52 Mhz max)
#define MMC_CMD6_ARG_NORM_SPEED          0x03B90000
#define MMC_CMD6_ARG_HIGH_SPEED          0x03B90100

// eMMC Internal Registers Parse
// (1) CID
#define ID1(cid)                         (((cid[0]&(0xFFFF<<0))<<16) | ((cid[1]&(0xFFFF<<16))>>16))
#define ID2(cid)                         (((cid[1]&(0xFFFF<<0))<<16) | ((cid[2]&(0xFFFF<<16))>>16))
// (2) CSD
#define WRITE_BL_LEN(csd)                ((csd[0]&(0x0F<<22))>>22)
#define ERASE_GRP_SIZE(csd)              ((csd[1]&(0x1F<<10))>>10)
#define ERASE_GRP_MULT(csd)              ((csd[1]&(0x1F<<5))>>5)
#define WP_GRP_SIZE(csd)                 ((csd[1]&(0x1F<<0))>>0)
#define READ_BL_LEN(csd)                 ((csd[2]&(0x0F<<16))>>16)

#define C_SIZE(csd)                      (((csd[1]&(0x03<<30))>>30) | ((csd[2]&(0x3FF))<<2))
#define C_SIZE_MULT(csd)                 ((csd[1]&(0x07<<15))>>15)

#define TMP_WRITE_PROTECT(csd)           ((csd[0]&(0x1<<12))>>12)
#define PERM_WRITE_PROTECT(csd)          ((csd[0]&(0x1<<13))>>13)
#define SET_TMP_WRITE_PROTECT(csd)       (csd[0] |= (0x1<<12))
#define CLR_TMP_WRITE_PROTECT(csd)       (csd[0] &= (~(0x1<<12)))
#define SET_PERM_WRITE_PROTECT(csd)      (csd[0] |= (0x1<<13))
#define CLR_PERM_WRITE_PROTECT(csd)      (csd[0] &= (~(0x1<<13)))

// (3) EXT_CSD
#define ERASE_GROUP_DEF(ext_csd)         (ext_csd[175])
#define HC_ERASE_GRP_SIZE(ext_csd)       (ext_csd[224])
#define HC_WP_GRP_SIZE(ext_csd)          (ext_csd[221])

#define SEC_COUNT(ext_csd)               ((ext_csd[215]<<24)|(ext_csd[214]<<16)| \
                                         (ext_csd[213]<<8)|(ext_csd[212]<<0))
                                         
#define BOOT_SIZE_MULT(ext_csd)          (ext_csd[226])
 
#define SEC_FEATURE_SUPPORT(ext_csd)     (ext_csd[231])                                        
#define SEC_GB_CL_EN_MASK                (0x1<<4)
#define SEC_BD_BLK_EN_MASK               (0x1<<2)
#define SEC_ER_EN_MASK                   (0x1<<0)

#define BOOT_WP(ext_csd)                 (ext_csd[173])
#define SET_B_PWR_WP_EN(ext_csd)         (ext_csd[173]|=(0x1<<0))
#define CLR_B_PWR_WP_EN(ext_csd)         (ext_csd[173]&=(~(0x1<<0)))
#define SET_B_PERM_WP_EN(ext_csd)        (ext_csd[173]|=(0x1<<2))
#define CLR_B_PERM_WP_EN(ext_csd)        (ext_csd[173]&=(~(0x1<<2)))
#define USER_WP(ext_csd)                 (ext_csd[171])
#define SET_US_PWR_WP_EN(ext_csd)        (ext_csd[171]|=(0x1<<0))
#define CLR_US_PWR_WP_EN(ext_csd)        (ext_csd[171]&=(~(0x1<<0)))
#define SET_US_PERM_WP_EN(ext_csd)       (ext_csd[171]|=(0x1<<2))
#define CLR_US_PERM_WP_EN(ext_csd)       (ext_csd[171]&=(~(0x1<<2)))

#define CMD6_ARGU_EXTCSD(index, value)   ((0x03<<24)|(index<<16)|(value<<8))

// (4) OCR
#define HIGH_CAPACITY(ocr)               (((ocr)&(0x01<<30))!=0x0)

#endif // #ifndef _MSDC_SLAVE_HW_H_
