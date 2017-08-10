/*----------------------------------------------------------------------------*
 * Copyright Statement:                                                       *
 *                                                                            *
 *   This software/firmware and related documentation ("MediaTek Software")   *
 * are protected under international and related jurisdictions'copyright laws *
 * as unpublished works. The information contained herein is confidential and *
 * proprietary to MediaTek Inc. Without the prior written permission of       *
 * MediaTek Inc., any reproduction, modification, use or disclosure of        *
 * MediaTek Software, and information contained herein, in whole or in part,  *
 * shall be strictly prohibited.                                              *
 * MediaTek Inc. Copyright (C) 2010. All rights reserved.                     *
 *                                                                            *
 *   BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND     *
 * AGREES TO THE FOLLOWING:                                                   *
 *                                                                            *
 *   1)Any and all intellectual property rights (including without            *
 * limitation, patent, copyright, and trade secrets) in and to this           *
 * Software/firmware and related documentation ("MediaTek Software") shall    *
 * remain the exclusive property of MediaTek Inc. Any and all intellectual    *
 * property rights (including without limitation, patent, copyright, and      *
 * trade secrets) in and to any modifications and derivatives to MediaTek     *
 * Software, whoever made, shall also remain the exclusive property of        *
 * MediaTek Inc.  Nothing herein shall be construed as any transfer of any    *
 * title to any intellectual property right in MediaTek Software to Receiver. *
 *                                                                            *
 *   2)This MediaTek Software Receiver received from MediaTek Inc. and/or its *
 * representatives is provided to Receiver on an "AS IS" basis only.          *
 * MediaTek Inc. expressly disclaims all warranties, expressed or implied,    *
 * including but not limited to any implied warranties of merchantability,    *
 * non-infringement and fitness for a particular purpose and any warranties   *
 * arising out of course of performance, course of dealing or usage of trade. *
 * MediaTek Inc. does not provide any warranty whatsoever with respect to the *
 * software of any third party which may be used by, incorporated in, or      *
 * supplied with the MediaTek Software, and Receiver agrees to look only to   *
 * such third parties for any warranty claim relating thereto.  Receiver      *
 * expressly acknowledges that it is Receiver's sole responsibility to obtain *
 * from any third party all proper licenses contained in or delivered with    *
 * MediaTek Software.  MediaTek is not responsible for any MediaTek Software  *
 * releases made to Receiver's specifications or to conform to a particular   *
 * standard or open forum.                                                    *
 *                                                                            *
 *   3)Receiver further acknowledge that Receiver may, either presently       *
 * and/or in the future, instruct MediaTek Inc. to assist it in the           *
 * development and the implementation, in accordance with Receiver's designs, *
 * of certain softwares relating to Receiver's product(s) (the "Services").   *
 * Except as may be otherwise agreed to in writing, no warranties of any      *
 * kind, whether express or implied, are given by MediaTek Inc. with respect  *
 * to the Services provided, and the Services are provided on an "AS IS"      *
 * basis. Receiver further acknowledges that the Services may contain errors  *
 * that testing is important and it is solely responsible for fully testing   *
 * the Services and/or derivatives thereof before they are used, sublicensed  *
 * or distributed. Should there be any third party action brought against     *
 * MediaTek Inc. arising out of or relating to the Services, Receiver agree   *
 * to fully indemnify and hold MediaTek Inc. harmless.  If the parties        *
 * mutually agree to enter into or continue a business relationship or other  *
 * arrangement, the terms and conditions set forth herein shall remain        *
 * effective and, unless explicitly stated otherwise, shall prevail in the    *
 * event of a conflict in the terms in any agreements entered into between    *
 * the parties.                                                               *
 *                                                                            *
 *   4)Receiver's sole and exclusive remedy and MediaTek Inc.'s entire and    *
 * cumulative liability with respect to MediaTek Software released hereunder  *
 * will be, at MediaTek Inc.'s sole discretion, to replace or revise the      *
 * MediaTek Software at issue.                                                *
 *                                                                            *
 *   5)The transaction contemplated hereunder shall be construed in           *
 * accordance with the laws of Singapore, excluding its conflict of laws      *
 * principles.  Any disputes, controversies or claims arising thereof and     *
 * related thereto shall be settled via arbitration in Singapore, under the   *
 * then current rules of the International Chamber of Commerce (ICC).  The    *
 * arbitration shall be conducted in English. The awards of the arbitration   *
 * shall be final and binding upon both parties and shall be entered and      *
 * enforceable in any court of competent jurisdiction.                        *
 *---------------------------------------------------------------------------*/

#if defined(CC_EMMC_BOOT)

#include <common.h>
#include "x_bim.h"
#include "x_timer.h"
#include <config.h>
#include <malloc.h>
#include <asm/errno.h>
#include <linux/mtd/mtd.h>
#include "mmc.h"
#include "msdc_host_hw.h"
#include "msdc_slave_hw.h"
#include "msdc_drv.h"
#include "x_ldr_env.h"
//#include "x_hal_5381.h"
#include "x_hal_arm.h"
#include "drvcust_if.h"
#include "dmx_drm_if.h"

#define MSDC_EMMC_INIT_UNIFY_EN             (1)

int ch = 1;
int devIndex = 0;
/* specify default data transfer mode */
int dataMode = BASIC_DMA_DATA_MODE; //PIO_DATA_MODE;
uint speedmode = SPEED_MODE_DS;
#ifdef CC_PARTITION_WP_SUPPORT
uint wp_config = 0;
#endif
/* the global variable related to descriptor dma */
volatile static uint _u4MsdcAccuVect = 0x00;
msdc_gpd_t DMA_MSDC_Header, DMA_MSDC_End;
msdc_bd_t DMA_BD[MAX_BD_PER_GPD];
uint BDNum = 0;

#define MSDC_AUTOK_PARAM_NUM 17
uint szAutoK_CmdLine[MSDC_AUTOK_PARAM_NUM] = {0};
static volatile BOOL fg_autok_apply_done = FALSE;

#ifdef CC_SUPPORT_MULTI_EMMC_SIZE
    UINT64 msdc_capacity = 0x0;
    UINT32 tmpsize1, tmpsize2;
#endif
/* the global variable related to clcok */
UINT32 msdcClk[][MSDC_CLK_IDX_MAX] = {{MSDC_CLK_TARGET},
                                      {MSDC_CLK_SRC_VAL},
                                      {MSDC_CLK_MODE_VAL},
                                      {MSDC_CLK_DIV_VAL},
                                      {MSDC_CLK_DRV_VAL}};

struct mmc *emmc_dev = NULL;
/* the global variable related to sample edge */
const EMMC_FLASH_DEV_T _arEMMC_UbootDevInfo[] =
{
  // Device name                 ID1        ID2       DS26Sample  DS26Delay   HS52Sample  HS52Delay   DDR52Sample DDR52Delay  HS200Sample HS200Delay //HS200Delay ,0x:WRDAT_CRCS_TA_CNTR,CKGEN_MSDC_DLY_SEL,PAD_DAT_RD_RXDLY,PAD_DAT_WR_RXDLY
  { "UNKNOWN",                0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x1861001c, 0x01000100, 0x00000000, 0x00000000, 0x00000102, 0x0000030A},
  {"H26M21001ECR",            0x4A48594E, 0x49582056, 0x00000000, 0x00000000, 0x00000006, 0x00000000, 0x00000102, 0x00000000, 0x00000006, 0x00000000},
  {"H26M31003FPR",            0x4A205849, 0x4E594812, 0x00000000, 0x00000000, 0x00000100, 0x00000A0A, 0x00000102, 0x00000000, 0x00000006, 0x00000000},
  {"H26M31003GMR",            0x4A483447, 0x31640402, 0x00000000, 0x00000000, 0x00000100, 0x00000A0A, 0x00000102, 0x00000000, 0x00000002, 0x0000030D},
  {"H26M31001HPR",            0x4A483447, 0x32611101, 0x00000000, 0x00000000, 0x1861001c, 0x01000100, 0x00000002, 0x00000F0F, 0x00000002, 0x02090D08},
  {"H26M52103FMR",            0x4A484147, 0x32650502, 0x00000000, 0x00000000, 0x00000006, 0x00000000, 0x00000002, 0x00040F0F, 0x00000100, 0x04150917},
  {"H26M41103HPR",            0x4A483847, 0x31650502, 0x00000000, 0x00000000, 0x00000006, 0x00000000, 0x00000002, 0x00000F0F, 0x00000000, 0x030a080f},
  {"SDIN5D2-4G",              0x0053454D, 0x30344790, 0x00000000, 0x00000000, 0x00000006, 0x00000000, 0x00000102, 0x00000000, 0x00000002, 0x0000030D},
  {"KLM2G1HE3F-B001",         0x004D3247, 0x31484602, 0x00000000, 0x00000000, 0x00000006, 0x00000000, 0x00000102, 0x00000000, 0x00000006, 0x00000000},
  {"KLM8G1GEND-B031",         0x0038474E, 0x44335201, 0x00000000, 0x00000000, 0x00000006, 0x00000000, 0x00000002, 0x03000d0d, 0x00000002, 0x0309080c},
  {"KLMAG2GEND-B031",         0x0041474E, 0x44335201, 0x00000000, 0x00000000, 0x00000006, 0x00000000, 0x00000002, 0x03000d0d, 0x00000002, 0x0309080c},
  {"KLMAG2GEAC-B031000",      0x004D4147, 0x3247430B, 0x00000000, 0x00000000, 0x00000006, 0x00000000, 0x00000102, 0x00040F0F, 0x00000002, 0x0309090C},
  {"KLM8G1GEAC-B031xxx",      0x004D3847, 0x3147430B, 0x00000000, 0x00000000, 0x1861001c, 0x01000100, 0x00000002, 0x00000F0F, 0x00000002, 0x0309090C},
  {"THGBM3G4D1FBAIG",         0x00303032, 0x47303000, 0x00000000, 0x00000000, 0x00000006, 0x00000000, 0x00000102, 0x00000000, 0x00000006, 0x00000000},
  {"THGBM4G4D1FBAIG",         0x00303032, 0x47343900, 0x00000000, 0x00000000, 0x00000006, 0x00000000, 0x00000102, 0x00000000, 0x00000006, 0x00000000},
  {"THGBM4G5D1HBAIR",         0x00303034, 0x47343900, 0x00000000, 0x00000000, 0x00000006, 0x00000000, 0x00000102, 0x00000000, 0x00000006, 0x00000000},
  {"THGBM4G6D2HBAIR",         0x00303038, 0x47344200, 0x00000000, 0x00000000, 0x00000006, 0x00000000, 0x00000102, 0x00000000, 0x00000006, 0x00000000},
  {"MTFC4GMVEA-1M WT",        0x4E4D4D43, 0x3034473A, 0x00000000, 0x00000000, 0x00000006, 0x00000000, 0x00000102, 0x0000000A, 0x00000006, 0x00000000},// JW857
  {"MTFC2GMVEA-0M WT",        0x4E4D4D43, 0x3032473A, 0x00000000, 0x00000000, 0x00000006, 0x00000000, 0x00000102, 0x0000000A, 0x00000006, 0x00000000},// JW896
//  {"MTFC8GLCDM-1M WT",        0x4E503158, 0x58585814, 0x00000000, 0x00000000, 0x00000004, 0x00000000, 0x00000102, 0x0000000A, 0x00000102, 0x1800030D},// JW962
  {"MTFC8GACAAAM-1M WT",      0x4E503158, 0x58585812, 0x00000000, 0x00000000, 0x1861001c, 0x01000100, 0x00000002, 0x03000d0d, 0x00000002, 0x03020815},// JWA61
  {"THGBMAG6A2JBAIR",         0x00303038, 0x47393251, 0x00000000, 0x00000000, 0x00000006, 0x00000000, 0x00000102, 0x0000130F, 0x00000000, 0x030a080C},
  {"MTFC8GACAEAM-1M WT",      0x4E52314A, 0x35354110, 0x00000000, 0x00000000, 0x00000006, 0x00000F0F, 0x00000102, 0x0000130F, 0x00000000, 0x030B090C},
  {"THGBM5G6A2JBAIR",         0x00303038, 0x47393200, 0x00000000, 0x00000000, 0x00000006, 0x00000000, 0x00000102, 0x0000000A, 0x00000102, 0x0000030D},
  {"THGBMBG5D1KBAIL",         0x00303034, 0x47453000, 0x00000000, 0x00000000, 0x00000006, 0x00000000, 0x00000102, 0x00000000, 0x00000002, 0x04060315},
  {"THGBMAG5A1JBAIR",         0x00303034, 0x47393051, 0x00000000, 0x00000000, 0x00000006, 0x00000000, 0x00000102, 0x0000000A, 0x00000102, 0x0000030D},
  {"THGBMAG7A2JBAIR",         0x00303136, 0x47393201, 0x00000000, 0x00000000, 0x00000006, 0x00000000, 0x00000102, 0x0000000A, 0x00000102, 0x0000030D},
  {"THGBMAG6A2JBAIR",         0x00303038, 0x47393251, 0x00000000, 0x00000000, 0x00000006, 0x00000000, 0x00000102, 0x0000130F, 0x00000002, 0x030a080C},
  {"THGBMBG6D1KBAIL-XXX",     0x00303038, 0x47453000, 0x00000000, 0x00000000, 0x00000006, 0x00000F0F, 0x00000102, 0x0000130F, 0x00000002, 0x0309090C},


  //for skymedia
  {"SP18A4G751B-0003_4G",     0x11373531, 0x30303010, 0x00000000, 0x00000000, 0x00001C00, 0x001C000f, 0x00000000, 0x00000000, 0x07201807, 0x00190003},
  {"SP18A8G861B-0003_8G_NEW", 0x11383631, 0x30303010, 0x00000000, 0x00000000, 0x00001C00, 0x001C000f, 0x00000000, 0x00000000, 0x07201807, 0x00190003},
  {"SP18A8G861B-0003_8G_OLD", 0x4d303030, 0x303010b1, 0x00000000, 0x00000000, 0x00001C00, 0x001C000f, 0x00000000, 0x00000000, 0x07201807, 0x00190003},

  //Micron,SDR50,need confirm HS200
  { "JW904",                     0x4e503058, 0x58585801, 0x00000000, 0x00000000, 0X0F401716,0X0017000F, 0x00000000, 0x00000000, 0x00800000, 0x0000000a},
  //toshiba,need confirm SDR50
  { "TMGBMBG5D1KBAIL",           0x303034,   0x47453000, 0x00000000, 0x00000000, 0x06901906, 0x001c000c, 0x00000000, 0x00000000, 0x06901906, 0x001c000c},
  //samsung,need confirm SDR50
  {"KLM8G1GEAC-B031_8G",         0x004d3847, 0x31474305, 0x00000000, 0x00000000, 0x07801603, 0x00130003, 0x00000000, 0x00000000, 0x07801603, 0x00130003},

  //Hynix, support SDR50,HS200
  {"H26M41103HPR",         0x4a483847, 0x31650507, 0x00000000, 0x00000000, 0x0f800f0f, 0x000f000f, 0x00000000, 0x00000000, 0x0c801012, 0x0015000a},


  //for wukong toshiba 16nm,sdr50
  { "THGBMFG7C1LBAIL",              0x00303136, 0x47373000, 0x00000000, 0x00000000, 0x00010000, 0x01000100, 0x00000000, 0x00000000, 0x00000102, 0x0000030A},
  //for wukong toshiba 19nm,sdr50
  { "THGBMFG7D2KBAIL",              0x00303136, 0x47453200, 0x00000000, 0x00000000, 0x00010000, 0x01000100, 0x00000000, 0x00000000, 0x00000102, 0x0000030A},
};


#ifdef CC_MTD_ENCRYPT_SUPPORT
#define AES_ADDR_ALIGN   0x20
#define AES_LEN_ALIGN    0x10
#define AES_BUFF_LEN     0x10000
uchar _u1MsdcBuf_AES[AES_BUFF_LEN + AES_ADDR_ALIGN], *_pu1MsdcBuf_Aes_Align;
#endif

//-----------------------------------------------------------------------------
// Partition encypt information.
//-----------------------------------------------------------------------------

void MsdcDumpRegister(void)
{
    unsigned int i = 0;
    unsigned int u4regs [] = {
        0x0,0x4,0x8,0xc,
        0x10,0x14,0x18,0x1c,
        0x30,0x34,0x38,0x3c,
        0x40,0x44,0x48,0x4c,
        0x50,0x54,0x58,0x5c,
        0x60,0x64,
        0x70,0x74,0x78,0x7c,
        0x80,0x84,0x88,0x8c,
        0x90,0x94,0x98,0x9c,
        0xa0,0xa4,0xa8,
        0xb0,0xb4,0xb8,
        0xf0,0xf4,0xf8,0xfc,
        0x100,0x104,0x110,0x114,
        0x118,
        0x180,0x184,0x188,0x18c,
        0x190,0x194,0x198,0x19c,
        0x208,0x20c,0x228};

    for (i=0; i < sizeof(u4regs)/sizeof(unsigned int); i++)
    printf("[0X%x] 0x%x \n",u4regs[i],MSDC_READ32(0xf006d000 + u4regs[i]));


    printf("\n EMMC Clock Src Setting - 0x%08X: 0x%08X\n", MSDC_CLK_S_REG1, MSDC_READ32(MSDC_CLK_S_REG1));
    printf("\n SD Clock Src Setting - 0x%08X: 0x%08X\n", MSDC_CLK_S_REG0, MSDC_READ32(MSDC_CLK_S_REG0));
}

int UbootGetMsdcStatus(struct mmc *mmc,int *status)
{
	struct mmc_cmd cmd;
    // Send CMD13
    cmd.cmdidx = MMC_CMD_SEND_STATUS;
    cmd.cmdarg = mmc->rca << 16;
    cmd.resp_type = MMC_RSP_R1;
    cmd.flags = 0;
    cmd.response[0] = 0;

	if (MSDC_SUCCESS == MsdcRequest(mmc, &cmd, NULL))
	{
		*status =  cmd.response[0];
		return MSDC_SUCCESS;
	}
	else
	{
		return MSDC_FAILED;
	}

}
VOID MsdcUbootFifoPathSelAndInit(VOID)
{

    //Printf("---Uboot Set FIFO path and Init---\n");
	MSDC_SET_FIELD(MSDC_IOCON, MSDC_IOCON_R_D_SMPL_SEL, 0);/*IOCON[5]*/
	MSDC_SET_FIELD(MSDC_IOCON, MSDC_IOCON_W_D_SMPL_SEL_SEL, 0);/*IOCON[9]*/
	MSDC_SET_FIELD(MSDC_IOCON, MSDC_IOCON_W_D_SMPL_SEL, 0);/*IOCON[8]*/

	MSDC_SET_FIELD(MSDC_IOCON, MSDC_IOCON_D_DLYLINE_SEL, 0);/*IOCON[3],clK tune all data Line share dly */



	MSDC_SET_FIELD(PAD_TUNE, PAD_RXDLY_SEL, 0);/*PAD_TUNE0[15] ,tune data path, tune mode select*/

	MSDC_SET_FIELD(PATCH_BIT2, CFG_CRCSTS, 1);/*PATCH_BIT2[28],latch CRC Status ,select async fifo path*/


	MSDC_SET_FIELD(PATCH_BIT2, CFG_RESP, 0);/*PATCH_BIT2[15],latch CMD Response ,select async fifo path*/


	/*eMMC50 Function Mux*/
	MSDC_SET_FIELD(EMMC50_CFG0, CRC_STS_SEL, 0); /*EMMC50_CFG0[4],write path switch to emmc45*/

	MSDC_SET_FIELD(EMMC50_CFG0, CFG_CMD_RESP_SEL, 0);/*EMMC50_CFG0[9],response path switch to emmc45*/

	MSDC_SET_FIELD(EMMC50_CFG0, CRC_STS_EDGE, 0);/*EMMC50_CFG0[3],select rising eage at emmc50 hs400*/

	/*Common Setting Config*/
	MSDC_SET_FIELD(PATCH_BIT0, CKGEN_MSDC_DLY_SEL, 0);/*patch_bit0[10], ckgen*/


	MSDC_SET_FIELD(PATCH_BIT1, CMD_RSP_TA_CNTR, 1); /*patch_bit1[3], TA*/

	MSDC_SET_FIELD(PATCH_BIT1, WRDAT_CRCS_TA_CNTR, 1);/*patch_bit1[0], TA*/

	MSDC_SET_FIELD(PAD_TUNE, PAD_CLK_TXDLY, 0);/*PAD_TUNE0[27],tx clk dly fix to 0 for HQA res*/
	MSDC_SET_FIELD(PATCH_BIT1, MSDC_PB1_GET_BUSY_MA, 1);
	MSDC_SET_FIELD(PATCH_BIT1, MSDC_PB1_GET_CRC_MA, 1);

	MSDC_SET_FIELD(PATCH_BIT2, MSDC_BIT2_CRCSTS_LATCH_SEL, 0);
	/* LATCH_TA_EN Config for CMD Path non_HS400 */
	MSDC_SET_FIELD(PATCH_BIT2, MSDC_BIT2_RESP_LATCH_ENSEL, 0);
	MSDC_SET_FIELD(PATCH_BIT0, INT_DAT_LATCH_CK_SEL, 0);

	//set delay to default value

	// CMD_PAD_RDLY:
	MSDC_SET_FIELD(PAD_TUNE, PAD_CMD_RXDLY, 0/*31*/);
	MSDC_SET_FIELD(PAD_TUNE1, PAD_TUNE1_CMDRDLY2, 0);
	MSDC_SET_FIELD(PAD_TUNE, PAD_CMD_RD_RX_DLY_SEL, 0);
	MSDC_SET_FIELD(PAD_TUNE1, PAD_TUNE1_CMDRRDLY2SEL, 0);

	// DAT_PAD_RDLY:
	MSDC_SET_FIELD(PAD_TUNE, PAD_DAT_RD_RXDLY, 0/*31*/);
	MSDC_SET_FIELD(PAD_TUNE1, PAD_TUNE1_DATRRDLY2, 0);
	MSDC_SET_FIELD(PAD_TUNE, PAD_DAT_RD_RXDLY_SEL, 0);
	MSDC_SET_FIELD(PAD_TUNE1, PAD_TUNE1_DATRRDLY2SEL, 0);


}


VOID MSDC_ApplyAutoK_Param(msdc_autok_t *p_env, uint u4clk)
{
    UINT32 i = 0;
	szAutoK_CmdLine[i++] = p_env->K_IOCON;
	szAutoK_CmdLine[i++] = p_env->K_PAD_TUNE0;
	szAutoK_CmdLine[i++] = p_env->K_PAD_TUNE1;
	szAutoK_CmdLine[i++] = p_env->K_PATCH_BIT0;
	szAutoK_CmdLine[i++] = p_env->K_PATCH_BIT1;
	szAutoK_CmdLine[i++] = p_env->K_PATCH_BIT2;
	szAutoK_CmdLine[i++] = p_env->K_EMMC50_CFG0;
	szAutoK_CmdLine[i++] = p_env->K_DAT_RD_DLY0;
	szAutoK_CmdLine[i++] = p_env->K_DAT_RD_DLY1;
	szAutoK_CmdLine[i++] = p_env->K_DAT_RD_DLY2;
	szAutoK_CmdLine[i++] = p_env->K_DAT_RD_DLY3;
	szAutoK_CmdLine[i++] = p_env->K_EMMC50_PAD_DS_TUNE;
	szAutoK_CmdLine[i++] = p_env->K_EMMC50_PAD_CMD_TUNE;
	szAutoK_CmdLine[i++] = p_env->K_EMMC50_PAD_DAT01_TUNE;
	szAutoK_CmdLine[i++] = p_env->K_EMMC50_PAD_DAT23_TUNE;
	szAutoK_CmdLine[i++] = p_env->K_EMMC50_PAD_DAT45_TUNE;
	szAutoK_CmdLine[i++] = p_env->K_EMMC50_PAD_DAT67_TUNE;


    if ((u4clk >= 200) &&
    	((speedmode == SPEED_MODE_HS400) || (speedmode == SPEED_MODE_HS200))
    	)
	{

		Printf("Uboot Apply Param to register ! \n");
		MSDC_WRITE32(MSDC_IOCON,p_env->K_IOCON);
		MSDC_WRITE32(PAD_TUNE,p_env->K_PAD_TUNE0);
		MSDC_WRITE32(PAD_TUNE1,p_env->K_PAD_TUNE1);
		MSDC_WRITE32(PATCH_BIT0,p_env->K_PATCH_BIT0);
		MSDC_WRITE32(PATCH_BIT1,p_env->K_PATCH_BIT1);
		MSDC_WRITE32(PATCH_BIT2,p_env->K_PATCH_BIT2);
		MSDC_WRITE32(EMMC50_CFG0,p_env->K_EMMC50_CFG0);

		MSDC_WRITE32(DAT_RD_DLY0,p_env->K_DAT_RD_DLY0);
		MSDC_WRITE32(DAT_RD_DLY1,p_env->K_DAT_RD_DLY1);
		MSDC_WRITE32(DAT_RD_DLY2,p_env->K_DAT_RD_DLY2);
		MSDC_WRITE32(DAT_RD_DLY3,p_env->K_DAT_RD_DLY3);

		MSDC_WRITE32(EMMC50_PAD_DS_TUNE,p_env->K_EMMC50_PAD_DS_TUNE);
		MSDC_WRITE32(EMMC50_PAD_CMD_TUNE,p_env->K_EMMC50_PAD_CMD_TUNE);
		MSDC_WRITE32(EMMC50_PAD_DAT01_TUNE,p_env->K_EMMC50_PAD_DAT01_TUNE);
		MSDC_WRITE32(EMMC50_PAD_DAT23_TUNE,p_env->K_EMMC50_PAD_DAT23_TUNE);
		MSDC_WRITE32(EMMC50_PAD_DAT45_TUNE,p_env->K_EMMC50_PAD_DAT45_TUNE);
		MSDC_WRITE32(EMMC50_PAD_DAT67_TUNE,p_env->K_EMMC50_PAD_DAT67_TUNE);
	}




	//dump
	Printf("Uboot Apply Param: SpeedMode: 0x%x \n",p_env->SpeedMode);
	Printf("K_IOCON: 0x%x \n",p_env->K_IOCON);
	Printf("K_PAD_TUNE0: 0x%x \n",p_env->K_PAD_TUNE0);
	Printf("K_PAD_TUNE1: 0x%x \n",p_env->K_PAD_TUNE1);
	Printf("K_PATCH_BIT0: 0x%x \n",p_env->K_PATCH_BIT0);
	Printf("K_PATCH_BIT1: 0x%x \n",p_env->K_PATCH_BIT1);
	Printf("K_PATCH_BIT2: 0x%x \n",p_env->K_PATCH_BIT2);
	Printf("K_EMMC50_CFG0: 0x%x \n",p_env->K_EMMC50_CFG0);
	Printf("K_DAT_RD_DLY0: 0x%x \n",p_env->K_DAT_RD_DLY0);
	Printf("K_DAT_RD_DLY1: 0x%x \n",p_env->K_DAT_RD_DLY1);
	Printf("K_DAT_RD_DLY2: 0x%x \n",p_env->K_DAT_RD_DLY2);
	Printf("K_DAT_RD_DLY3: 0x%x \n",p_env->K_DAT_RD_DLY3);
	Printf("K_EMMC50_PAD_DS_TUNE: 0x%x \n",p_env->K_EMMC50_PAD_DS_TUNE);
	Printf("K_EMMC50_PAD_CMD_TUNE: 0x%x \n",p_env->K_EMMC50_PAD_CMD_TUNE);
	Printf("K_EMMC50_PAD_DAT01_TUNE: 0x%x \n",p_env->K_EMMC50_PAD_DAT01_TUNE);
	Printf("K_EMMC50_PAD_DAT23_TUNE: 0x%x \n",p_env->K_EMMC50_PAD_DAT23_TUNE);
	Printf("K_EMMC50_PAD_DAT45_TUNE: 0x%x \n",p_env->K_EMMC50_PAD_DAT45_TUNE);
	Printf("K_EMMC50_PAD_DAT67_TUNE: 0x%x \n",p_env->K_EMMC50_PAD_DAT67_TUNE);

	fg_autok_apply_done = TRUE;

}

UINT8 u1DrivingMoreMap[][2] =   {0,0,//level 0
	  	 					1,0,
	  	 					2,0,
	  	 					3,0,
	  	 					4,0,
	  	 					5,0,//current useage
	  	 					6,0,
	  	 					7,0,
	  	 					0,1,
	  	 					1,1,
	  	 					2,1,
	  	 					3,1,
	  	 					4,1, //12
	  	 					5,1,
	  	 					6,1,
	  	 					7,1,
	  	 					0,2,
	  	 					1,2,
	  	 					2,2, //18
	  	 					3,2,
	  	 					4,2,
	  	 					5,2,
	  	 					6,2,
	  	 					7,2,
	  	 					0,3,
	  	 					1,3,
	  	 					2,3,
	  	 					3,3,
	  	 					4,3,
	  	 					5,3,
	  	 					6,3,
	  	 					7,3//31
	 					};

//Add for HS200/HS400
VOID MSDCUbootDrivingMore(UINT32 CLK, UINT32 CMD, UINT32 DAT)
{
	Printf("==>Uboot Set driving MORE . CLK %d ,CMD %d, DAT %d \n",CLK,CMD,DAT);

	MSDC_CLRBIT(0xF000D488, 0xFFF << 18);
	MSDC_CLRBIT(0xF000D48C, 0x3FFFF << 0);
	MSDC_CLRBIT(0xF000D480, 0xFFF << 12);

	//CLOCK
	MSDC_SETBIT(0xF000D488, ((u1DrivingMoreMap[ CLK ][0] & 0x7)<<18));
	MSDC_SETBIT(0xF000D480, (u1DrivingMoreMap[ CLK ][1] & 0x3) << 12);

	//CMD
	MSDC_SETBIT(0xF000D488, ((u1DrivingMoreMap[ CMD ][0] & 0x7)<<21));
	MSDC_SETBIT(0xF000D480, (u1DrivingMoreMap[ CMD ][1] & 0x3) << 14);

	//DAT
	MSDC_SETBIT(0xF000D480, ((u1DrivingMoreMap[ DAT ][1] & 0x3) << 16) | ((u1DrivingMoreMap[ DAT ][1] & 0x3) << 18) |
				((u1DrivingMoreMap[ DAT ][1] & 0x3) << 20) | ((u1DrivingMoreMap[ DAT ][1] & 0x3) << 22) |
				((u1DrivingMoreMap[ DAT ][1] & 0x3) << 24) | ((u1DrivingMoreMap[ DAT ][1] & 0x3) << 26) |
				((u1DrivingMoreMap[ DAT ][1] & 0x3) << 28) | ((u1DrivingMoreMap[ DAT ][1] & 0x3) << 30));

	MSDC_SETBIT(0xF000D488, ((u1DrivingMoreMap[ DAT ][0] & 0x7)<<24) | ((u1DrivingMoreMap[ DAT ][0] & 0x7)<<27));


	MSDC_SETBIT(0xF000D48C, ((u1DrivingMoreMap[ DAT ][0] & 0x7) << 0) | ((u1DrivingMoreMap[ DAT ][0]&0x7) << 3) |
			   ((u1DrivingMoreMap[ DAT ][0] & 0x7) << 6) | ((u1DrivingMoreMap[ DAT ][0] & 0x7) << 9) | ((u1DrivingMoreMap[ DAT ][0]&0x7) << 12) | ((u1DrivingMoreMap[ DAT ][0]&0x7) << 15));
}

void uboot_print_setting()
{
    Printf("uboot pinmux  0x%x, arbitor 0x%x \n",
			(*(volatile unsigned int *)(0xf000d600)),
			(*(volatile unsigned int *)(0xF0012200)));
}

INT32 MsdcUbootTuneSampleEdge(BOOL fgCmd, BOOL fgRead, BOOL fgWrite,struct mmc *mmc, struct mmc_cmd *cmd, struct mmc_data *data)
{
	UINT32 u4OldVal = 0;
	UINT32 u4NewVal = 0;


	if (fgCmd)//0X04[1]
	{
		MSDC_GET_FIELD(MSDC_IOCON, MSDC_IOCON_R_SMPL, u4OldVal);
		u4NewVal = u4OldVal ^ 0x1;
		MSDC_SET_FIELD(MSDC_IOCON, MSDC_IOCON_R_SMPL, u4NewVal);
		Printf("Tune AutoK CMD sampel edge %d --> %d \n",u4OldVal,u4NewVal);

	}
	else if (fgRead)
	{
		if (1) //emmc
		{
			MSDC_GET_FIELD(MSDC_IOCON, MSDC_IOCON_D_SMPL, u4OldVal);
			u4NewVal = u4OldVal ^ 0x1;
			MSDC_SET_FIELD(MSDC_IOCON, MSDC_IOCON_D_SMPL, u4NewVal);
			Printf("EMMC Tune AutoK READ sampel edge %d --> %d \n",u4OldVal,u4NewVal);
		}
		else  //SD
		{
			MSDC_SET_FIELD(MSDC_IOCON, MSDC_IOCON_D_SMPL, 0);

			MSDC_GET_FIELD(PATCH_BIT0, RD_DAT_SEL, u4OldVal);
			u4NewVal = u4OldVal ^ 0x1;
			MSDC_SET_FIELD(PATCH_BIT0, RD_DAT_SEL, u4NewVal);
			Printf("SD Tune AutoK READ sampel edge %d --> %d \n",u4OldVal,u4NewVal);
		}


	}
	else if (fgWrite)
	{
		MSDC_GET_FIELD(PATCH_BIT2, (0x1  << 25)/*MSDC_BIT2_CFG_CRCSTS_EDGE*/, u4OldVal);
		u4NewVal = u4OldVal ^ 0x1;
		MSDC_SET_FIELD(PATCH_BIT2, (0x1  << 25)/*MSDC_BIT2_CFG_CRCSTS_EDGE*/, u4NewVal);
		Printf("Tune AutoK WRITE sampel edge %d --> %d \n",u4OldVal,u4NewVal);

	}

	return MSDC_SUCCESS;
}


INT32 MsdcUbootOnlineAutoKTune(BOOL fgCmd, BOOL fgRead, BOOL fgWrite,struct mmc *mmc, struct mmc_cmd *cmd, struct mmc_data *data)
{

    Printf("Enter Autok online tune.....\n");

	//if (SPEED_MODE_HS == speedmode)
	if (((SPEED_MODE_HS == speedmode)) ||
		(!fg_autok_apply_done)//before we really change to HS200/HS400, we still use HS mode tuning
		)
	{
	   MsdcUbootTuneSampleEdge(fgCmd, fgRead, fgWrite,mmc,cmd,data);
	}
	else if (SPEED_MODE_HS200 == speedmode)
	{
		Printf("ERROR! Uboot error in HS200 ,Currently not support tune \n");
		return MSDC_FAILED;
	}
	else if (SPEED_MODE_HS400 == speedmode)
	{
		Printf("ERROR! Uboot error in HS400 ,Currently not support tune \n");
		return MSDC_FAILED;
	}
	else
	{
		Printf("ERROR! The current speed mode invalid \n");
		return MSDC_FAILED;
	}

	if (fgCmd)
	{
		if(MsdcReqCmdStart(mmc, cmd, data) == MSDC_SUCCESS)
        {
            MSDC_LOG(MSG_LVL_WARN, "Command Tune Success\n");
            return MSDC_SUCCESS;
        }
		else
		{
			MSDC_LOG(MSG_LVL_WARN, "Command Tune Fail\n");
            return MSDC_FAILED;
		}
	}
	else if (fgRead)
	{

		if(MsdcRequest(mmc, cmd, data) == MSDC_SUCCESS)
        {
            MSDC_LOG(MSG_LVL_WARN, "Data READ Tune Success\n");
            return MSDC_SUCCESS;
        }
		else
		{
			MSDC_LOG(MSG_LVL_WARN, "Data READ Tune Fail\n");
            return MSDC_FAILED;
		}

	}
	else if (fgWrite)
	{
		if(MsdcRequest(mmc, cmd, data) == MSDC_SUCCESS)
        {
            MSDC_LOG(MSG_LVL_WARN, "Data Write Tune Success\n");
            return MSDC_SUCCESS;
        }
		else
		{
			MSDC_LOG(MSG_LVL_WARN, "Data Write Tune Fail\n");
            return MSDC_FAILED;
		}
	}
	return MSDC_SUCCESS;
}


extern mtk_part_info_t rMTKPartInfo[];

#if 0
static VOID msdc_dump_dbg_register(VOID)
{
    UINT32 i;

    for (i = 0; i < 26; i++) {
        MSDC_WRITE32(DBG_SEL, i);
        MSDC_LOG(MSG_LVL_ERR, "SW_DBG_SEL: 0x%x\n", i);
        MSDC_LOG(MSG_LVL_ERR, "SW_DBG_OUT: 0x%x\n", MSDC_READ32(DBG_OUT));
    }

    MSDC_WRITE32(DBG_SEL, 0);
}
#endif
/*
void MsdcDumpRegister(void)
{
    uint i = 0;

    for(; i< 0x104; i+=4)
    {
        if(i%16 == 0)
        {
            MSDC_LOG(MSG_LVL_TITLE, "\r\n%08X |", i);
        }
        if (i != 0x14 && i != 0x18 && i != 0x1c)
            MSDC_LOG(MSG_LVL_TITLE, " %08X", MSDC_READ32(MSDC_BASE_ADDR + (ch * MSDC_CH_OFFSET) + i));
    }
}
*/
uchar MsdcCheckSumCal(uchar *buf, uint len)
{
    uint i = 0, sum = 0;

    for(; i<len;i++)
    {
        sum += *(buf + i);
    }

    return (0xFF-(uchar)sum);
}
#ifdef CC_PARTITION_WP_SUPPORT
uint MsdcPartitionWp(uint blkindx)
{
    uint partId;
    uint size = 0, offset = 0, wp_en = 0;

    for (partId = 0; partId < MAX_MTD_DEVICES; partId++)
    {
        offset = DRVCUST_InitGet((QUERY_TYPE_T)(eNANDFlashPartOffset0 + partId));
        size = DRVCUST_InitGet((QUERY_TYPE_T)(eNANDFlashPartSize0 + partId));
        wp_en = DRVCUST_InitGet((QUERY_TYPE_T)(eNANDFlashPartWp0 + partId));

        if((wp_en != 0) &&
        	 ((uint)(offset>>9) <= blkindx) &&
        	 ((uint)((offset+size)>>9) > blkindx))
        {
        	  MSDC_LOG(MSG_LVL_INFO, " This partition is write protect!\n");
            return 1;
        }
    }

    MSDC_LOG(MSG_LVL_INFO, " This partition is not write protect!\n");
    return 0;
}

uint MsdcPartitionWpConfig(uint wp)
{
    if(wp)
    {
      wp_config = 1;
      MSDC_LOG(MSG_LVL_INFO, "partition write protect function was enabled!\n");
    }
    else
    {
      wp_config = 0;
      MSDC_LOG(MSG_LVL_INFO, "partition write protect function was disabled!\n");
    }
}

#endif
#ifdef CC_MTD_ENCRYPT_SUPPORT
void MsdcAesInit(u16 keylen)
{
	  uint tmp = 0;
	  uchar key[16] = {0};

    if(DMX_NAND_AES_INIT(key, keylen))
    {
        MSDC_LOG(MSG_LVL_ERR, " aes init success!\n");
    }
    else
    {
        MSDC_LOG(MSG_LVL_ERR, " aes init failed!\n");
    }

    memset(_u1MsdcBuf_AES, 0x00, AES_BUFF_LEN + AES_ADDR_ALIGN);

    tmp = (uint)_u1MsdcBuf_AES & (AES_ADDR_ALIGN - 1);
    if(tmp != 0x0)
        _pu1MsdcBuf_Aes_Align = (uchar *)(((uint)_u1MsdcBuf_AES & (~(AES_ADDR_ALIGN - 1))) + AES_ADDR_ALIGN);
    else
    	  _pu1MsdcBuf_Aes_Align = (uchar *)_u1MsdcBuf_AES;

    return;
}

uint MsdcPartitionEncrypted(uint blkindx)
{
    uint partId;
    uint size = 0, offset = 0, fg_en = 0;

    for (partId = 0; partId < MAX_MTD_DEVICES; partId++)
    {
        offset = DRVCUST_InitGet((QUERY_TYPE_T)(eNANDFlashPartOffset0 + partId));
        size = DRVCUST_InitGet((QUERY_TYPE_T)(eNANDFlashPartSize0 + partId));
        fg_en = DRVCUST_InitGet((QUERY_TYPE_T)(eNANDFlashPartEncypt0 + partId));

        if((fg_en != 0) &&
        	 ((uint)(offset>>9) <= blkindx) &&
        	 ((uint)((offset+size)>>9) > blkindx))
        {
        	  MSDC_LOG(MSG_LVL_INFO, " This partition is encrypted!\n");
            return 1;
        }
    }

    MSDC_LOG(MSG_LVL_INFO, " This partition is not encrypted!\n");
    return 0;
}

int MsdcAesEncryption(uint buff, uint len)
{
    uint len_used = 0, len_total = 0;

    if(len & (AES_LEN_ALIGN - 1) != 0x0)
	  {
	      MSDC_LOG(MSG_LVL_ERR, " the buffer to be encrypted is not align to %d bytes!\n", AES_LEN_ALIGN);
	      return MSDC_FAILED;
	  }

	  if( buff & (AES_ADDR_ALIGN - 1) != 0x0)
	  {
			  len_total = (int)len;
			  do
			  {
			  	  len_used = (len_total > AES_BUFF_LEN)?AES_BUFF_LEN:len_total;
			      memcpy((void *)_pu1MsdcBuf_Aes_Align, (void *)buff, len_used);
			      if(DMX_NAND_AES_Encryption((uint)_pu1MsdcBuf_Aes_Align, (uint)_pu1MsdcBuf_Aes_Align, len_used))
		        {
		            MSDC_LOG(MSG_LVL_INFO, "Encryption to buffer(addr:0x%08X size:0x%08X) success!\n", (uint)_pu1MsdcBuf_Aes_Align, len_used);
		        }
		        else
		        {
		            MSDC_LOG(MSG_LVL_ERR, "Encryption to buffer(addr:0x%08X size:0x%08X) failed!\n", (uint)_pu1MsdcBuf_Aes_Align, len_used);
		            return MSDC_FAILED;
		        }
		        memcpy((void *)buff, (void *)_pu1MsdcBuf_Aes_Align, len_used);

		        len_total -= len_used;
		        buff += len_used;

			  }while(len_total > 0);
	  }
	  else
	  {
	      if(DMX_NAND_AES_Encryption(buff, buff, len))
		    {
		        MSDC_LOG(MSG_LVL_INFO, "Encryption to buffer(addr:0x%08X size:0x%08X) success!\n", buff, len);
		    }
		    else
		    {
		        MSDC_LOG(MSG_LVL_ERR, "Encryption to buffer(addr:0x%08X size:0x%08X) failed!\n", buff, len);
		        return MSDC_FAILED;
		    }
	  }

	  return MSDC_SUCCESS;
}

int MsdcAesDecryption(uint buff, uint len)
{
    uint len_used = 0, len_total = 0;

    if(len & (AES_LEN_ALIGN - 1) != 0x0)
	  {
	      MSDC_LOG(MSG_LVL_ERR, " the buffer to be encrypted is not align to %d bytes!\n", AES_LEN_ALIGN);
	      return MSDC_FAILED;
	  }

	  if( buff & (AES_ADDR_ALIGN - 1) != 0x0)
	  {
			  len_total = len;
			  do
			  {
			  	  len_used = (len_total > AES_BUFF_LEN)?AES_BUFF_LEN:len_total;
			      memcpy((void *)_pu1MsdcBuf_Aes_Align, (void *)buff, len_used);
			      if(DMX_NAND_AES_Decryption((uint)_pu1MsdcBuf_Aes_Align, (uint)_pu1MsdcBuf_Aes_Align, len_used))
		        {
		            MSDC_LOG(MSG_LVL_INFO, "Decryption to buffer(addr:0x%08X size:0x%08X) success!\n", (uint)_pu1MsdcBuf_Aes_Align, len_used);
		            //return MSDC_SUCCESS;
		        }
		        else
		        {
		            MSDC_LOG(MSG_LVL_ERR, "Decryption to buffer(addr:0x%08X size:0x%08X) failed!\n", (uint)_pu1MsdcBuf_Aes_Align, len_used);
		            return MSDC_FAILED;
		        }
		        memcpy((void *)buff, (void *)_pu1MsdcBuf_Aes_Align, len_used);

		        len_total -= len_used;
		        buff += len_used;

			  }while(len_total > 0);
    }
    else
	  {
	      if(DMX_NAND_AES_Decryption(buff, buff, len))
		    {
		        MSDC_LOG(MSG_LVL_INFO, "Decryption to buffer(addr:0x%08X size:0x%08X) success!\n", buff, len);
		    }
		    else
		    {
		        MSDC_LOG(MSG_LVL_ERR, "Decryption to buffer(addr:0x%08X size:0x%08X) failed!\n", buff, len);
		        return MSDC_FAILED;
		    }
	  }

	  return MSDC_SUCCESS;
}

#endif

void MsdcDescriptorConfig(void *pBuff, uint u4BufLen)
{
    uint i, tmpBDNum, tmpBuflen/*, fgEOL*/;
    uchar *tmppBuff = (uchar *)pBuff;

    if(u4BufLen == 0)
        return;

    BDNum = u4BufLen / BD_MAX_LEN + ((u4BufLen % BD_MAX_LEN == 0)?0:1);
    tmpBDNum = BDNum;
    tmpBuflen = u4BufLen;

    MSDC_LOG(MSG_LVL_INFO, "\n----BD NUM: %d----\n", BDNum);

    if(BDNum > MAX_BD_NUM)
    {
        MSDC_LOG(MSG_LVL_WARN, "BD Number exceeds MAX value(%d)!\n", MAX_BD_NUM);
        return;
    }

    // Initial the structures
    memset(&DMA_MSDC_Header, 0x0, sizeof(msdc_gpd_t));
    memset(&DMA_MSDC_End, 0x0, sizeof(msdc_gpd_t));
    memset(DMA_BD, 0x0, sizeof(msdc_bd_t)*MAX_BD_NUM);

    // Config the BD structure array
    for(i = 0;i<BDNum;i++)
    {
        if(i != BDNum - 1)
        {
            DMA_BD[i].pNext = (void *)(&DMA_BD[i+1]);
        }
        else
            DMA_BD[i].eol = 1;

        DMA_BD[i].pBuff = (void *)(tmppBuff);
        DMA_BD[i].buffLen = (tmpBuflen>BD_MAX_LEN)?(BD_MAX_LEN):(tmpBuflen);
        tmpBuflen -= DMA_BD[i].buffLen;
        tmppBuff += DMA_BD[i].buffLen;

        DMA_BD[i].chksum = 0;
        DMA_BD[i].chksum = MsdcCheckSumCal((uchar *)(DMA_BD+i), 16);
    }

    // Config the GPD structure
    DMA_MSDC_Header.hwo = 1;  /* hw will clear it */
    DMA_MSDC_Header.bdp = 1;
    DMA_MSDC_Header.pNext = (void *)(&DMA_MSDC_End);
    DMA_MSDC_Header.pBuff = (void *)(DMA_BD);
    DMA_MSDC_Header.chksum = 0;  /* need to clear first. */
    DMA_MSDC_Header.chksum = MsdcCheckSumCal((uchar *)(&DMA_MSDC_Header), 16);

    HalFlushInvalidateDCacheMultipleLine((uint)(&DMA_MSDC_Header), sizeof(msdc_gpd_t));
    HalFlushInvalidateDCacheMultipleLine((UINT32)(&DMA_MSDC_End), sizeof(msdc_gpd_t));
    HalFlushInvalidateDCacheMultipleLine((UINT32)(DMA_BD), sizeof(msdc_bd_t)*BDNum);

    // Config the DMA HW registers
    MSDC_SETBIT(DMA_CFG, DMA_CFG_CHKSUM);
    MSDC_WRITE32(DMA_SA, 0x0);

#if defined(CC_MT5891) || defined(CONFIG_ARCH_MT5891)
    MSDC_WRITE32(DMA_SA_HIGH4BIT, 0x0);
#endif
    MSDC_SETBIT(DMA_CTRL, DMA_CTRL_BST_64);
    MSDC_SETBIT(DMA_CTRL, DMA_CTRL_DESC_MODE);

    MSDC_WRITE32(DMA_SA, (uint)(&DMA_MSDC_Header));

}

void MsdcFindDev(uint *pCID)
{
    uint i, devNum;

    /*
      * why we need to define the id mask of emmc
      * Some vendors' emmc has the same brand & type but different product revision.
      * That means the firmware in eMMC has different revision
      * We should treat these emmcs as same type and timing
      * So id mask can ignore this case
    */
    uint idMask = 0xFFFFFF00;

    devNum = sizeof(_arEMMC_UbootDevInfo)/sizeof(EMMC_FLASH_DEV_T);
    MSDC_LOG(MSG_LVL_ERR, "%08X:%08X:%08X:%08X\n", pCID[0], pCID[1], pCID[2], pCID[3]);
    MSDC_LOG(MSG_LVL_ERR, "id1:%08X id2:%08X\n", ID1(pCID), ID2(pCID));

    for(i = 0; i<devNum; i++)
    {
        if((_arEMMC_UbootDevInfo[i].u4ID1 == ID1(pCID)) &&
           ((_arEMMC_UbootDevInfo[i].u4ID2 & idMask) == (ID2(pCID) & idMask)))
        {
            break;
        }
    }

    devIndex = (i == devNum)?0:i;

    MSDC_LOG(MSG_LVL_TITLE, "eMMC Name: %s\n", _arEMMC_UbootDevInfo[devIndex].acName);
}

/* delay value setting for hs data sample
*/
void MsdcSmapleDelay(UINT32 flag)
{

    #if MSDC_NEW_FIFO_PATH
    //Printf("UBOOT Use NEW AutoK idea \n");
    return;
	#endif

#if defined(CC_MT5891) || defined(CONFIG_ARCH_MT5891)
    //MT5891 default use async-fifo path, currently we change to use delay-line path as default
    MSDC_CLRBIT(PATCH_BIT2, CFG_CRCSTS);
	MSDC_SETBIT(PATCH_BIT2, 0x0 << CFG_CRCSTS_SHIFT);

	MSDC_CLRBIT(PATCH_BIT2, CFG_RESP);
	MSDC_SETBIT(PATCH_BIT2, 0x1 << CFG_RESP_SHIFT);

	MSDC_CLRBIT(PATCH_BIT2, CFG_RDAT);
	MSDC_SETBIT(PATCH_BIT2, 0x1 << CFG_RDAT_SHIFT);

	MSDC_CLRBIT(PAD_TUNE, PAD_RXDLY_SEL);
	MSDC_SETBIT(PAD_TUNE, 0x1 << PAD_RXDLY_SEL_SHIFT);
#endif


	uboot_print_setting();

	Printf("UBOOT SetParam, speed mode %d \n",speedmode);

    // Sample Edge init
    MSDC_CLRBIT(PAD_TUNE, 0xFFFFFFFF);

	//set default value,not separate tune data0 for ett
	MSDC_SET_FIELD(MSDC_IOCON, MSDC_IOCON_D_DLYLINE_SEL, 0);//IOCON[3]
	MSDC_SET_FIELD(MSDC_IOCON, MSDC_IOCON_R_D_SMPL_SEL, 0);//IOCON[5]
	MSDC_SET_FIELD(MSDC_IOCON, MSDC_IOCON_W_D_SMPL_SEL_SEL, 0);//IOCON[9]

    // Sample Edge init
    if(speedmode == SPEED_MODE_HS)
    {
        //TO-DO: Using new Tuning idea or old one ?
   	    if (1)//IS_IC_5891())
		{
		    Printf("uboot HS: set param 0x%x , 0x%x \n",_arEMMC_UbootDevInfo[devIndex].HS52Sample,_arEMMC_UbootDevInfo[devIndex].HS52Delay);
			/*++++++++++++++++++++++clock part+++++++++++++++++++*/
	    	MSDC_CLRBIT(PATCH_BIT0, INT_DAT_LATCH_CK_SEL);
	    	MSDC_SETBIT(PATCH_BIT0, 0x0<< INT_DAT_LATCH_CK_SEL_SHIFT);

		    MSDC_CLRBIT(PATCH_BIT0, CKGEN_MSDC_DLY_SEL);//CKGEN_MSDC_DLY_SEL
	        MSDC_SETBIT(PATCH_BIT0, ((_arEMMC_UbootDevInfo[devIndex].HS52Sample&0x00f00000)>>20)<< CKGEN_MSDC_DLY_SEL_SHIFT);

	   	    /*++++++++++++++++++++++cmd part+++++++++++++++++++*/
	        MSDC_CLRBIT(PATCH_BIT1, CMD_RSP_TA_CNTR);//cmd turn around fix 1
	        MSDC_SETBIT(PATCH_BIT1, 0x1<<CMD_RSP_TA_CNTR_SHIFT);

	        MSDC_CLRBIT(PAD_TUNE, PAD_CMD_RESP_RXDLY);//cmd resp rx delay
	   	    MSDC_SETBIT(PAD_TUNE, ((_arEMMC_UbootDevInfo[devIndex].HS52Sample&0x1f000000)>>24)<<PAD_CMD_RESP_RXDLY_SHIFT); //clock delay 0xc

	   	    MSDC_CLRBIT(MSDC_IOCON, MSDC_IOCON_R_SMPL);//cmd resp rx delay
	   	    MSDC_SETBIT(MSDC_IOCON,((_arEMMC_UbootDevInfo[devIndex].HS52Sample&0x00010000)>>16)<<MSDC_IOCON_R_SMPL_SHIFT); //

	   	    MSDC_CLRBIT(PAD_TUNE, PAD_CMD_RXDLY);//cmd rx dly
	   	    MSDC_SETBIT(PAD_TUNE, ((_arEMMC_UbootDevInfo[devIndex].HS52Sample&0x00001f00)>>8)<<PAD_CMD_RXDLY_SHIFT); //clock delay 0xc

	   	    /*++++++++++++++++++++++write part+++++++++++++++++++*/
	        MSDC_CLRBIT(PATCH_BIT1, WRDAT_CRCS_TA_CNTR);//WRDAT_CRCS_TA_CNTR
	        MSDC_SETBIT(PATCH_BIT1, 0x1<<WRDAT_CRCS_TA_CNTR_SHIFT);//write crc status turn aroud fix 1

	    	MSDC_CLRBIT(PAD_TUNE, PAD_DAT_WR_RXDLY);
	   	    MSDC_SETBIT(PAD_TUNE, ((_arEMMC_UbootDevInfo[devIndex].HS52Sample&0x0000001f))<<PAD_DAT_WR_RXDLY_SHIFT); //

	    	MSDC_CLRBIT(MSDC_IOCON, MSDC_IOCON_W_D_SMPL_SEL);
	   	    MSDC_SETBIT(MSDC_IOCON, ((_arEMMC_UbootDevInfo[devIndex].HS52Delay&0x01000000)>>24)<<MSDC_IOCON_W_D_SMPL_SHIFT); //

	    	MSDC_CLRBIT(DAT_RD_DLY0, MSDC_DAT_RDDLY0_D0);
	   	    MSDC_SETBIT(DAT_RD_DLY0, ((_arEMMC_UbootDevInfo[devIndex].HS52Delay&0x001f0000)>>16)<<MSDC_DAT_RDDLY0_D0_SHIFT); //
			/*++++++++++++++++++++++read part+++++++++++++++++++*/

	        MSDC_CLRBIT(MSDC_IOCON, MSDC_IOCON_D_SMPL);//read sample
	   	    MSDC_SETBIT(MSDC_IOCON,((_arEMMC_UbootDevInfo[devIndex].HS52Delay&0x00000100)>>8)<<MSDC_IOCON_D_SMPL_SHIFT); //

	        MSDC_CLRBIT(PAD_TUNE, PAD_DAT_RD_RXDLY);//read rx delay
	   	    MSDC_SETBIT(PAD_TUNE, ((_arEMMC_UbootDevInfo[devIndex].HS52Delay&0x0000001f))<<PAD_DAT_RD_RXDLY_SHIFT); //clock delay 0xc
	   	}
    }
    else if(speedmode == SPEED_MODE_DDR50)
    {
        MSDC_SETBIT(PAD_TUNE, _arEMMC_UbootDevInfo[devIndex].DDR52Delay&0x0000ffff);//PAD_DAT_RD_RXDLY,PAD_DAT_WR_RXDLY
        MSDC_CLRBIT(PATCH_BIT0, CKGEN_MSDC_DLY_SEL);//CKGEN_MSDC_DLY_SEL
        MSDC_SETBIT(PATCH_BIT0, ((_arEMMC_UbootDevInfo[devIndex].DDR52Delay&0x00ff0000)>>16)<< CKGEN_MSDC_DLY_SEL_SHIFT);
    }
    else if(speedmode == SPEED_MODE_HS200)
    {
        if (1)//IS_IC_5891())
        {
    		MSDC_CLRBIT(PATCH_BIT0, INT_DAT_LATCH_CK_SEL);
        	MSDC_SETBIT(PATCH_BIT0, 0x0<< INT_DAT_LATCH_CK_SEL_SHIFT);

            MSDC_CLRBIT(PAD_TUNE, PAD_CLK_TXDLY);
       	    MSDC_SETBIT(PAD_TUNE, 0xc<<PAD_CLK_TXDLY_SHIFT); //clock delay 0xc
       	    /*++++++++++++++++++++++cmd part+++++++++++++++++++*/
            MSDC_CLRBIT(PATCH_BIT1, CMD_RSP_TA_CNTR);//cmd turn around fix 1
            MSDC_SETBIT(PATCH_BIT1, 0x1<<CMD_RSP_TA_CNTR_SHIFT);

		    MSDC_CLRBIT(PATCH_BIT0, CKGEN_MSDC_DLY_SEL);//CKGEN_MSDC_DLY_SEL
	        MSDC_SETBIT(PATCH_BIT0, ((_arEMMC_UbootDevInfo[devIndex].HS52Sample&0x00f00000)>>20)<< CKGEN_MSDC_DLY_SEL_SHIFT);

            MSDC_CLRBIT(PAD_TUNE, PAD_CMD_RESP_RXDLY);//cmd resp rx delay
       	    MSDC_SETBIT(PAD_TUNE, ((_arEMMC_UbootDevInfo[devIndex].HS200Sample&0x1f000000)>>24)<<PAD_CMD_RESP_RXDLY_SHIFT); //clock delay 0xc

       	    MSDC_CLRBIT(MSDC_IOCON, MSDC_IOCON_R_SMPL);//cmd resp rx delay
       	    MSDC_SETBIT(MSDC_IOCON,((_arEMMC_UbootDevInfo[devIndex].HS200Sample&0x00010000)>>16)<<MSDC_IOCON_R_SMPL_SHIFT); //

       	    MSDC_CLRBIT(PAD_TUNE, PAD_CMD_RXDLY);//cmd rx dly
       	    MSDC_SETBIT(PAD_TUNE, ((_arEMMC_UbootDevInfo[devIndex].HS200Sample&0x00001f00)>>8)<<PAD_CMD_RXDLY_SHIFT); //clock delay 0xc

       	    /*++++++++++++++++++++++write part+++++++++++++++++++*/
            MSDC_CLRBIT(PATCH_BIT1, WRDAT_CRCS_TA_CNTR);//WRDAT_CRCS_TA_CNTR
            MSDC_SETBIT(PATCH_BIT1, 0x1<<WRDAT_CRCS_TA_CNTR_SHIFT);//write crc status turn aroud fix 1

        	MSDC_CLRBIT(PAD_TUNE, PAD_DAT_WR_RXDLY);
       	    MSDC_SETBIT(PAD_TUNE, ((_arEMMC_UbootDevInfo[devIndex].HS200Sample&0x0000001f))<<PAD_DAT_WR_RXDLY_SHIFT); //

    		MSDC_CLRBIT(MSDC_IOCON, MSDC_IOCON_W_D_SMPL_SEL);
    		MSDC_SETBIT(MSDC_IOCON, ((_arEMMC_UbootDevInfo[devIndex].HS200Delay&0x01000000)>>24)<<MSDC_IOCON_W_D_SMPL_SHIFT); //

        	MSDC_CLRBIT(DAT_RD_DLY0, MSDC_DAT_RDDLY0_D0);
       	    MSDC_SETBIT(DAT_RD_DLY0, ((_arEMMC_UbootDevInfo[devIndex].HS200Delay&0x001f0000)>>16)<<MSDC_DAT_RDDLY0_D0_SHIFT); //
    		/*++++++++++++++++++++++read part+++++++++++++++++++*/
            //MSDC_CLRBIT(PATCH_BIT0, CKGEN_MSDC_DLY_SEL);//CKGEN_MSDC_DLY_SEL
            //MSDC_SETBIT(PATCH_BIT0, ((_arEMMC_UbootDevInfo[devIndex].HS200Delay&0x00ff0000)>>16)<< CKGEN_MSDC_DLY_SEL_SHIFT);

            MSDC_CLRBIT(MSDC_IOCON, MSDC_IOCON_D_SMPL);//read sample
       	    MSDC_SETBIT(MSDC_IOCON,((_arEMMC_UbootDevInfo[devIndex].HS200Delay&0x000100)>>8)<<MSDC_IOCON_D_SMPL_SHIFT); //

            MSDC_CLRBIT(PAD_TUNE, PAD_DAT_RD_RXDLY);//read rx delay
       	    MSDC_SETBIT(PAD_TUNE, ((_arEMMC_UbootDevInfo[devIndex].HS200Delay&0x0000001f))<<PAD_DAT_RD_RXDLY_SHIFT); //clock delay 0xc
        }
    }
}

/* when I test TF card about DDR mode,  I find the truth:
* The write operation in DDR mode needs write status falling edge sample + 0xA delay for CMD/DAT line,
* but rising edge sample should be set for the read operation in DDR mode
*/
void MsdcSampleEdge(UINT32 flag)
{
	#if !MSDC_NEW_FIFO_PATH
    // Sample Edge init
    MSDC_CLRBIT(MSDC_IOCON, 0xFFFFFF);
	#endif

    if(speedmode == SPEED_MODE_HS)
	{
		//MSDC_WRITE32(MSDC_CLK_H_REG1, 0x7);//use higher HCLK cause kernel boot cost 1 second more. may because PLL doesnot start on time.
	}
	else if(speedmode == SPEED_MODE_DDR50)
	{
		#if !MSDC_NEW_FIFO_PATH
        MSDC_SETBIT(MSDC_IOCON, _arEMMC_UbootDevInfo[devIndex].DDR52Sample & 0xFFFFFF);
		#endif
		MSDC_WRITE32(MSDC_CLK_H_REG1, 0x2);
		MSDC_SETBIT(PATCH_BIT2,MSDC_BIT2_DDR5O_SEL);//enable DDR50 wrap data problem support
	}
	else if((speedmode == SPEED_MODE_HS400) || (speedmode == SPEED_MODE_HS200))
	{
		MSDC_WRITE32(MSDC_CLK_H_REG1, 0x7);//must for HS400
		printf("uboot Set H Clock to 7 \n");
	}

}

void MsdcSetSpeedMode(uint speedMode)
{
    //printf("uboot set speed mode to %d \n",speedMode);
    speedmode = speedMode;
	MsdcSampleEdge(0);
	MsdcSmapleDelay(0);
}

void MsdcClrTiming(void)
{
    // Clear Sample Edge
    MSDC_CLRBIT(MSDC_IOCON, (((UINT32)0x3) << 1));

    // Clear Pad Tune
    MSDC_WRITE32(PAD_TUNE, 0x00000000);

    // Clear
    MSDC_WRITE32(DAT_RD_DLY0, 0x00000000);
    MSDC_WRITE32(DAT_RD_DLY1, 0x00000000);

}

int MsdcReset(void)
{
    uint i;

    // Reset MSDC
    MSDC_SETBIT(MSDC_CFG, MSDC_CFG_RST);

    for(i = 0; i<MSDC_RST_TIMEOUT_LIMIT_COUNT; i++)
    {
        if (0 == (MSDC_READ32(MSDC_CFG) & MSDC_CFG_RST))
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

    return MSDC_SUCCESS;
}

int MsdcClrFifo(void)
{
    uint i;
    // Reset FIFO
    MSDC_SETBIT(MSDC_FIFOCS, MSDC_FIFOCS_FIFOCLR);

    for(i = 0; i<MSDC_RST_TIMEOUT_LIMIT_COUNT; i++)
    {
        if (0 == (MSDC_READ32(MSDC_FIFOCS) & (MSDC_FIFOCS_FIFOCLR | MSDC_FIFOCS_TXFIFOCNT_MASK | MSDC_FIFOCS_RXFIFOCNT_MASK)))
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

    return MSDC_SUCCESS;
}

void MsdcChkFifo(void)
{
    // Check if rx/tx fifo is zero
    if ((MSDC_READ32(MSDC_FIFOCS) & (MSDC_FIFOCS_TXFIFOCNT_MASK | MSDC_FIFOCS_RXFIFOCNT_MASK)) != 0)
    {
        MSDC_LOG(MSG_LVL_WARN, "FiFo not 0, FIFOCS:0x%08X !!\r\n", MSDC_READ32(MSDC_FIFOCS));
        MsdcClrFifo();
    }
}

void MsdcClrIntr(void)
{
    // Check MSDC Interrupt vector register
    if  (0x00  != MSDC_READ32(MSDC_INT))
    {
        MSDC_LOG(MSG_LVL_WARN, "MSDC INT(0x%08X) not 0:0x%08X !!\r\n", MSDC_INT, MSDC_READ32(MSDC_INT));

        // Clear MSDC Interrupt vector register
        MSDC_WRITE32(MSDC_INT, MSDC_READ32(MSDC_INT));
    }
}

void MsdcStopDMA(void)
{
    MSDC_LOG(MSG_LVL_INFO, "DMA status: 0x%.8x\n",MSDC_READ32(DMA_CFG));

    MSDC_SETBIT(DMA_CTRL, DMA_CTRL_STOP);
    while(MSDC_READ32(DMA_CFG) & DMA_CFG_DMA_STATUS);

    MSDC_LOG(MSG_LVL_INFO, "DMA Stopped!\n");
}

int MsdcWaitClkStable (void)
{
    uint i;

    for(i = 0; i<MSDC_CLK_TIMEOUT_LIMIT_COUNT; i++)
    {
        if (0 != (MSDC_READ32(MSDC_CFG) & MSDC_CFG_CARD_CK_STABLE))
        {
            break;
        }

        HAL_Delay_us(1000);
        //udelay(1000);
    }
    if(i == MSDC_CLK_TIMEOUT_LIMIT_COUNT)
    {
        MSDC_LOG(MSG_LVL_ERR, "WaitClkStable Failed !\r\n");
        return MSDC_FAILED;
    }

    return MSDC_SUCCESS;
}

int MsdcWaitHostIdle(uint timeout)
{
	  uint i;

    for(i = 0;i<timeout;i++)
    {
        if ((0 == (MSDC_READ32(SDC_STS) & (SDC_STS_SDCBUSY | SDC_STS_CMDBUSY))) && (0x00  == MSDC_READ32(MSDC_INT)))
        {
        	  return MSDC_SUCCESS;
        }
        //HAL_Delay_us(1000);
        udelay(1000);
    }

    return MSDC_FAILED;
}

void MsdSetDataMode(uint datamode)
{
	  //Assert((NULL_DATA_MODE < datamode) && (datamode <= ENHANCED_DMA_DATA_MODE));

    dataMode = datamode;

    if(datamode < BASIC_DMA_DATA_MODE)
    {
        MSDC_SETBIT(MSDC_CFG, MSDC_CFG_PIO_MODE);
        MSDC_LOG(MSG_LVL_ERR, "Set PIO Mode!\r\n");
    }
    else
    {
        MSDC_CLRBIT(MSDC_CFG, MSDC_CFG_PIO_MODE);
        MSDC_LOG(MSG_LVL_ERR, "Set DMA Mode!\r\n");
    }
}

int MsdcInit (struct mmc *mmc)
{
    // Reset MSDC
    MsdcReset();

    // SD/MMC Mode
    MSDC_SETBIT(MSDC_CFG, MSDC_CFG_PIO_MODE | MSDC_CFG_SD);
	// Reset MSDC
    MsdcReset();
    MsdSetDataMode(dataMode);

    // Disable sdio & Set bus to 1 bit mode
    MSDC_CLRBIT(SDC_CFG, SDC_CFG_SDIO | SDC_CFG_BW_MASK);

    // set clock mode (DIV mode)
    MSDC_CLRBIT(MSDC_CFG, (((uint)0x03) << 20));

    // Wait until clock is stable
    if (MSDC_FAILED == MsdcWaitClkStable())
    {
        return MSDC_FAILED;
    }

    // Set default RISC_SIZE for DWRD pio mode
    MSDC_WRITE32(MSDC_IOCON, (MSDC_READ32(MSDC_IOCON) & ~MSDC_IOCON_RISC_SIZE_MASK) | MSDC_IOCON_RISC_SIZE_DWRD);

    // Set Data timeout setting => Maximum setting
    MSDC_WRITE32(SDC_CFG, (MSDC_READ32(SDC_CFG) & ~(((uint)0xFF) << SDC_CFG_DTOC_SHIFT)) | (((uint)0xFF) << SDC_CFG_DTOC_SHIFT));

    // Clear Timing to default value
    MsdcClrTiming();


	MsdcUbootFifoPathSelAndInit();

    return MSDC_SUCCESS;
}

void MsdcContinueClock (int i4ContinueClock)
{
    if (i4ContinueClock)
    {
       // Set clock continuous even if no command
       MSDC_SETBIT(MSDC_CFG, (((UINT32)0x01) << 1));
    }
    else
    {
       // Set clock power down if no command
       MSDC_CLRBIT(MSDC_CFG, (((UINT32)0x01) << 1));
    }
}

// Return when any interrupt is matched or timeout
int MsdcWaitIntr (uint vector, uint timeoutCnt, uint fgMode)
{
    uint i, retVector;

    // Clear Vector variable
    _u4MsdcAccuVect = 0;
    retVector = INT_SD_DATA_CRCERR;

    for(i = 0; i<timeoutCnt; i++)
    {
        // Status match any bit
        if((fgMode == 0) && ((MSDC_READ32(MSDC_INT) & vector) != 0))
        {
            _u4MsdcAccuVect |= MSDC_READ32(MSDC_INT);
            MSDC_WRITE32(MSDC_INT, _u4MsdcAccuVect & vector);
            return MSDC_SUCCESS;
        }
        else if((fgMode == 1) && ((MSDC_READ32(MSDC_INT) & vector) == vector))
        {
            _u4MsdcAccuVect |= MSDC_READ32(MSDC_INT);
            MSDC_WRITE32(MSDC_INT, _u4MsdcAccuVect & vector);
            return MSDC_SUCCESS;
        }

        if((MSDC_READ32(MSDC_INT) & retVector) != 0)
        {
            // return directly
            MSDC_LOG(MSG_LVL_ERR, "unexpected interrupt: INT=0x%x\n", MSDC_READ32(MSDC_INT));
            return MSDC_FAILED;
        }

        HAL_Delay_us(1000);
        //Mtk_udelay(1000);
    }

    MSDC_LOG(MSG_LVL_ERR, "timeout expected INT=0x%x, real INT=0x%x\n", vector, MSDC_READ32(MSDC_INT));
    // Timeout case
    return MSDC_FAILED;
}

void MsdcSetupCmd(struct mmc *mmc, struct mmc_cmd *cmd, struct mmc_data *data, uint *pu4respType, uint *pu4sdcCmd)
{
    /* Figure out the response type */
    switch(cmd->cmdidx)
    {
    case MMC_CMD_GO_IDLE_STATE:
        *pu4respType = MMC_RSP_NONE;
        (*pu4sdcCmd) |= SDC_CMD_RSPTYPE_NO;
        break;
    case MMC_CMD_SEND_OP_COND:
        *pu4respType = MMC_RSP_R3;
        (*pu4sdcCmd) |= SDC_CMD_RSPTYPE_R3;
        break;
    case MMC_CMD_ALL_SEND_CID:
		case MMC_CMD_SEND_CSD:
        *pu4respType = MMC_RSP_R2;
        (*pu4sdcCmd) |= SDC_CMD_RSPTYPE_R2;
        break;
    case MMC_CMD_SET_RELATIVE_ADDR:
        *pu4respType = MMC_RSP_R1;
        (*pu4sdcCmd) |= SDC_CMD_RSPTYPE_R1;
        break;
    case MMC_CMD_SWITCH:
        *pu4respType = MMC_RSP_R1b;
        (*pu4sdcCmd) |= SDC_CMD_RSPTYPE_R1B;
        break;
    case MMC_CMD_SELECT_CARD:
        *pu4respType = MMC_RSP_R1;
        (*pu4sdcCmd) |= SDC_CMD_RSPTYPE_R1;
        break;
    case MMC_CMD_SEND_EXT_CSD:
        if(mmc->version & MMC_VERSION_MMC)
        {
            *pu4respType = MMC_RSP_R1;
            (*pu4sdcCmd) |= (SDC_CMD_RSPTYPE_R1 | DTYPE_SINGLE_BLK | SDC_CMD_READ);
            MSDC_WRITE32(SDC_BLK_NUM, 1);
        }
        else
        { // for SD or unkown card
            *pu4respType = MMC_RSP_R7;
            (*pu4sdcCmd) |= SDC_CMD_RSPTYPE_R7;
        }
        break;
    case MMC_CMD_STOP_TRANSMISSION:
        *pu4respType = MMC_RSP_R1b;
        (*pu4sdcCmd) |= (SDC_CMD_STOP | SDC_CMD_RSPTYPE_R1B);
        break;
    case MMC_CMD_SEND_STATUS:
        *pu4respType = MMC_RSP_R1;
        (*pu4sdcCmd) |= SDC_CMD_RSPTYPE_R1;
        break;
    case MMC_CMD_SET_BLOCKLEN:
        *pu4respType = MMC_RSP_R1;
        (*pu4sdcCmd) |= SDC_CMD_RSPTYPE_R1;
        break;
    case MMC_CMD_READ_SINGLE_BLOCK:
        *pu4respType = MMC_RSP_R1;
        (*pu4sdcCmd) |= (SDC_CMD_RSPTYPE_R1 | DTYPE_SINGLE_BLK | SDC_CMD_READ);
        MSDC_WRITE32(SDC_BLK_NUM, 1);
        break;
    case MMC_CMD_READ_MULTIPLE_BLOCK:
        *pu4respType = MMC_RSP_R1;
        (*pu4sdcCmd) |= (SDC_CMD_RSPTYPE_R1 | DTYPE_MULTI_BLK | SDC_CMD_READ);
        if(data)
        {
            MSDC_WRITE32(SDC_BLK_NUM, data->blocks);
        }
        break;
    case MMC_CMD_WRITE_SINGLE_BLOCK:
        *pu4respType = MMC_RSP_R1;
        (*pu4sdcCmd) |= (SDC_CMD_RSPTYPE_R1 | DTYPE_SINGLE_BLK | SDC_CMD_WRITE);
        MSDC_WRITE32(SDC_BLK_NUM, 1);
        break;
    case MMC_CMD_WRITE_MULTIPLE_BLOCK:
        *pu4respType = MMC_RSP_R1;
        (*pu4sdcCmd) |= (SDC_CMD_RSPTYPE_R1 | DTYPE_MULTI_BLK | SDC_CMD_WRITE);
        if(data)
        {
            MSDC_WRITE32(SDC_BLK_NUM, data->blocks);
        }
        break;
    case MMC_CMD_PROGRAM_CSD:
        *pu4respType = MMC_RSP_R1;
        (*pu4sdcCmd) |= (SDC_CMD_RSPTYPE_R1 | DTYPE_SINGLE_BLK | SDC_CMD_WRITE);
        if(data)
        {
            MSDC_WRITE32(SDC_BLK_NUM, data->blocks);
        }
        break;
    case MMC_CMD_SET_WRITE_PROT:
        *pu4respType = MMC_RSP_R1b;
        (*pu4sdcCmd) |= SDC_CMD_RSPTYPE_R1B;
        break;
    case MMC_CMD_CLR_WRITE_PROT:
        *pu4respType = MMC_RSP_R1b;
        (*pu4sdcCmd) |= SDC_CMD_RSPTYPE_R1B;
        break;
    case MMC_CMD_ERASE_START:
    case MMC_CMD_ERASE_END:
        *pu4respType = MMC_RSP_R1;
        (*pu4sdcCmd) |= (SDC_CMD_RSPTYPE_R1);
        break;
    case MMC_CMD_ERASE:
        *pu4respType = MMC_RSP_R1b;
        (*pu4sdcCmd) |= (SDC_CMD_RSPTYPE_R1B);
        break;
    case SD_CMD_APP_SEND_OP_COND:
        *pu4respType = MMC_RSP_R3;
        (*pu4sdcCmd) |= SDC_CMD_RSPTYPE_R3;
        break;
    case MMC_CMD_APP_CMD:
        *pu4respType = MMC_RSP_R1;
        (*pu4sdcCmd) |= SDC_CMD_RSPTYPE_R1;
        break;
    }

    // Set Blk Length
    if(data)
    {
        (*pu4sdcCmd) |= ((data->blocksize) << SDC_CMD_LEN_SHIFT);
        MSDC_LOG(MSG_LVL_INFO, "block size: %08x\n", data->blocksize);
    }

    // Set SDC_CMD.CMD
    (*pu4sdcCmd) |= (cmd->cmdidx & 0x3F);

    // Set SDC_CMD.AUTO
    if((cmd->flags)>>4)
        (*pu4sdcCmd) |= (SDC_CMD_AUTO_CMD12);

    // Set SDC Argument
    MSDC_WRITE32(SDC_ARG, cmd->cmdarg);

    /* Send the commands to the device */
    MSDC_WRITE32(SDC_CMD, (*pu4sdcCmd));

}


void MsdcHandleResp(struct mmc_cmd *cmd, uint *pu4respType, uint *pu4sdcCmd)
{
    // Handle the response
    switch (*pu4respType)
    {
    case MMC_RSP_NONE:
        MSDC_LOG(MSG_LVL_INFO, "%s: CMD%d CMD 0x%08X ARG 0x%08X RESPONSE_NO\r\n", __FUNCTION__, cmd->cmdidx, *pu4sdcCmd, cmd->cmdarg);
        break;
    case MMC_RSP_R1:
    case MMC_RSP_R1b:
        cmd->response[0] = MSDC_READ32(SDC_RESP0);
        MSDC_LOG(MSG_LVL_INFO, "%s: CMD%d CMD 0x%08X ARG 0x%08X RESPONSE_R1/R1B/R5/R6/R6 0x%08X\r\n", __FUNCTION__, cmd->cmdidx, *pu4sdcCmd, cmd->cmdarg, cmd->response[0]);
        break;
    case MMC_RSP_R2:
        cmd->response[0] = MSDC_READ32(SDC_RESP3);
        cmd->response[1] = MSDC_READ32(SDC_RESP2);
        cmd->response[2] = MSDC_READ32(SDC_RESP1);
        cmd->response[3] = MSDC_READ32(SDC_RESP0);
        MSDC_LOG(MSG_LVL_INFO, "%s: CMD%d CMD 0x%08X ARG 0x%08X RESPONSE_R2 0x%08X 0x%08X 0x%08X 0x%08X\r\n", __FUNCTION__, cmd->cmdidx, *pu4sdcCmd, cmd->cmdarg,
                                                                                     cmd->response[0], cmd->response[1], cmd->response[2], cmd->response[3]);
        break;
    case MMC_RSP_R3:
        cmd->response[0] = MSDC_READ32(SDC_RESP0);
        MSDC_LOG(MSG_LVL_INFO, "%s: CMD%d CMD 0x%08X ARG 0x%08X RESPONSE_R3/R4 0x%08X\r\n", __FUNCTION__, cmd->cmdidx, *pu4sdcCmd, cmd->cmdarg, cmd->response[0]);
        break;
    }
}

int MsdcReqCmdStart(struct mmc *mmc, struct mmc_cmd *cmd, struct mmc_data *data)
{
    uint respType = MMC_RSP_NONE;
    uint sdcCmd = 0;
    int i4Ret = MSDC_SUCCESS;
    uint u4CmdDoneVect;

    // Check if rx/tx fifo is zero
    MsdcChkFifo();

    // Clear interrupt Vector
    MsdcClrIntr();

    MSDC_LOG(MSG_LVL_INFO, "MsdcSendCmd : CMD%d, ARGU!%08X!\r\n", cmd->cmdidx, cmd->cmdarg);

    if(MsdcWaitHostIdle(MSDC_WAIT_SDC_BUS_TIMEOUT_LIMIT_COUNT))
    {
        i4Ret = ERR_CMD_FAILED;
        MSDC_LOG(MSG_LVL_ERR, "Wait HOST idle failed: SDC_STS(%08X), MSDC_INT(%08X)!\n", MSDC_READ32(SDC_STS), MSDC_READ32(MSDC_INT));
        goto ErrorEnd;
    }

    MsdcSetupCmd(mmc, cmd, data, &respType, &sdcCmd);

    // Wait for command and response if existed
    u4CmdDoneVect = INT_SD_CMDRDY | INT_SD_CMDTO | INT_SD_RESP_CRCERR;

    if (MSDC_SUCCESS != MsdcWaitIntr(u4CmdDoneVect, MSDC_WAIT_CMD_TIMEOUT_LIMIT_COUNT, 0))
    {
        MSDC_LOG(MSG_LVL_ERR, "Failed to send CMD/RESP, DoneVect = 0x%x.\r\n", u4CmdDoneVect);
        i4Ret = ERR_CMD_FAILED;
        goto ErrorEnd;
    }

    MSDC_LOG(MSG_LVL_INFO, "Interrupt = %08X\n", _u4MsdcAccuVect);

    if (_u4MsdcAccuVect & INT_SD_CMDTO)
    {
        MSDC_LOG(MSG_LVL_ERR, "CMD%d ARG 0x%08X - CMD Timeout (AccuVect 0x%08X INTR 0x%08X).\r\n", cmd->cmdidx, cmd->cmdarg, _u4MsdcAccuVect, MSDC_READ32(MSDC_INT));
        i4Ret = ERR_CMD_FAILED;
        goto ErrorEnd;
    }
    else if (_u4MsdcAccuVect & INT_SD_RESP_CRCERR)
    {
        MSDC_LOG(MSG_LVL_ERR, "CMD%d ARG 0x%08X - CMD CRC Error (AccuVect 0x%08X INTR 0x%08X).\r\n", cmd->cmdidx, cmd->cmdarg, _u4MsdcAccuVect, MSDC_READ32(MSDC_INT));
        i4Ret = ERR_CMD_FAILED;
        goto ErrorEnd;
    }

    /*
    else if ((_u4MsdcAccuVect & (~(INT_SD_CMDRDY))) || (0 != MSDC_READ32(MSDC_INT)))
    {
        MSDC_LOG(MSG_LVL_ERR, "CMD%d ARG 0x%08X - UnExpect status (AccuVect 0x%08X INTR 0x%08X).\r\n", cmd->cmdidx, cmd->cmdarg, _u4MsdcAccuVect, MSDC_READ32(MSDC_INT));
        i4Ret = ERR_CMD_FAILED;
        goto ErrorEnd;
    }
    */

    // Handle the response
    MsdcHandleResp(cmd, &respType, &sdcCmd);

ErrorEnd:
    return i4Ret;

}

int MsdcReqDataStop(struct mmc *mmc)
{
    struct mmc_cmd stop;
    int i4Ret;

    memset(&stop, 0, sizeof(struct mmc_cmd));
    stop.cmdidx = MMC_CMD_STOP_TRANSMISSION;
    stop.cmdarg = 0;
    stop.resp_type = MMC_RSP_R1b;
    stop.flags = 0;

    i4Ret = MsdcReqCmdStart(mmc, &stop, NULL);
    MSDC_LOG(MSG_LVL_INFO, "Stop Sending-Data State(%d)!\n", i4Ret);

    return i4Ret;
}

int MsdcErrorHandling(struct mmc *mmc, struct mmc_cmd *cmd, struct mmc_data *data)
{
    int i4Ret = MSDC_SUCCESS;
	int status = 0;
    MSDC_LOG(MSG_LVL_INFO, "Uboot Start Error Handling...!\n");

    // Reset MSDC
    MsdcReset();

    // Stop DMA
    if((cmd->flags & 0x0F) > 1)
    {
        MsdcStopDMA();
    }

    // Clear FIFO and wait it becomes 0
    i4Ret = MsdcClrFifo();
    if(MSDC_SUCCESS != i4Ret)
    {
		Printf("%s clear fifo fail \n",__FUNCTION__);
        goto ErrorEnd;
    }

    // Clear MSDC interrupts and make sure all are cleared
    MsdcClrIntr();
    if  (0x00  != MSDC_READ32(MSDC_INT))
    {
		Printf("%s clear INT fail \n",__FUNCTION__);
        i4Ret = MSDC_FAILED;
        goto ErrorEnd;
    }

    // Send Stop Command for Multi-Write/Read
    if(data && (data->blocks > 1))
    {
        if (MsdcReqDataStop(mmc))
        {
            MSDC_LOG(MSG_LVL_WARN, "mmc fail to send stop cmd\n");
        }
    }
	else //check emmc status
	{
		if(MSDC_SUCCESS != UbootGetMsdcStatus(mmc,&status))
		{
			MSDC_LOG(MSG_LVL_ERR, "%s Failed to get card status 0x%x \n", __FUNCTION__,status);
			goto ErrorEnd;
		}
		MSDC_LOG(MSG_LVL_ERR, "card status 0x%x in error handle \n",R1_CURRENT_STATE(status));

		if (R1_CURRENT_STATE(status) == R1_STATE_DATA ||
    	    R1_CURRENT_STATE(status) == R1_STATE_RCV)
		{
			MSDC_LOG(MSG_LVL_ERR, "Error handle, send stop again! \n");
			if (MsdcReqDataStop(mmc))
            {
                MSDC_LOG(MSG_LVL_ERR, "MSDC fail to send stop cmd at line %d \n",__LINE__);
				goto ErrorEnd;
            }

			//wait until emmc change back to trans state
			{
			    if(MSDC_SUCCESS != UbootGetMsdcStatus(mmc,&status))
			    {
			        MSDC_LOG(MSG_LVL_ERR, "%s Failed to get card status, line %d \n", __FUNCTION__,__LINE__);
			    }
			    MSDC_LOG(MSG_LVL_ERR, "(R)Card Status: 0x%x\n", R1_CURRENT_STATE(status));
			}
        }
    }

    MSDC_LOG(MSG_LVL_INFO, "End Error Handling...!\n");

ErrorEnd:
    return i4Ret;
}

void MsdcSetBuffBits(uint *pBuff, uint bits)
{
    (*pBuff) |= bits;
}

void MsdcClrBuffBits(uint *pBuff, uint bits)
{
    (*pBuff) &= (~bits);
}

int MsdcCalCheckSum(uchar *pBuf, uint len)
{
    uint i, checksum = 0;
    for(i = 0;i<len;i++)
    {
        checksum += pBuf[i];
    }

    return checksum;
}

int MsdcHandleDataTransfer(struct mmc *mmc, struct mmc_cmd *cmd, struct mmc_data *data)
{
    int i4Ret = MSDC_SUCCESS;
    uint u4CmdDoneVect, fgAutoCmd12, IntrWaitVect = 0, IntrCheckVect = 0;

    fgAutoCmd12 = ((cmd->flags>>4)?1:0);

    if (PIO_DATA_MODE == (cmd->flags & 0x0F))
    {
        uint u4RxFifoCnt, u4TxFifoCnt;
        uint *pBufData = (uint*)((data->flags & MMC_DATA_READ)?(data->dest):(data->src));
        uint u4BufLen = (uint)((data->blocks)*(data->blocksize));
        uint u4BufEnd = (uint)pBufData + u4BufLen;
        uint u4RxCnt = 0;

        u4CmdDoneVect = INT_SD_DATTO | INT_SD_DATA_CRCERR;

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
                u4RxFifoCnt = (MSDC_READ32(MSDC_FIFOCS) & MSDC_FIFOCS_RXFIFOCNT_MASK);

                while ((u4BufLen) && (sizeof(int) <= u4RxFifoCnt))
                {
                    // Read Data
                    *pBufData = MSDC_READ32(MSDC_RXDATA);
                    pBufData++;

                    u4RxFifoCnt -= sizeof(int);
                    u4RxCnt += sizeof(int);

                    if(u4RxCnt == SDHC_BLK_SIZE)
                    {
                        // Check CRC error happens by every 512 Byte
                        // Check if done vector occurs
                        if (u4CmdDoneVect & MSDC_READ32(MSDC_INT))
                        {
                            MSDC_LOG(MSG_LVL_ERR, "Read Error Break !!\r\n");
                            break;
                        }

                        u4RxCnt = 0;
                        u4BufLen -= SDHC_BLK_SIZE;
                    }
                }
            }
        }
        else
		    {
            while (u4BufEnd > (UINT32)pBufData)
            {
                // Check if error done vector occurs
                if (u4CmdDoneVect & (_u4MsdcAccuVect | MSDC_READ32(MSDC_INT)))
                {
                    MSDC_LOG(MSG_LVL_ERR, "DoneVect:0x%08X, accuVect:0x%08X,  INTR:0x%08X\r\n", u4CmdDoneVect, _u4MsdcAccuVect, MSDC_READ32(MSDC_INT));
                    break;
                }

                // wait until fifo has enough space
                while(1)
                {
                    if((MSDC_READ32(MSDC_FIFOCS) & MSDC_FIFOCS_TXFIFOCNT_MASK) == 0)
                    {
                        break;
                    }
                }

                u4TxFifoCnt = MSDC_FIFO_LEN;

                if (sizeof(int) <= u4TxFifoCnt)
                {
                    while ((u4BufEnd > (UINT32)pBufData) && (sizeof(int) <= u4TxFifoCnt))
                    {
                        // Write Data
                        MSDC_WRITE32(MSDC_TXDATA, *pBufData);
                        pBufData++;
                        u4TxFifoCnt -= sizeof(int);
                    }
                }
            }

        }

        // Wait for data complete
        if (MSDC_SUCCESS != MsdcWaitIntr((IntrWaitVect), MSDC_WAIT_DATA_COMPLETE_TIMEOUT_LIMIT_COUNT, 1))
        {
            MSDC_LOG(MSG_LVL_ERR, "Wait Intr timeout (AccuVect 0x%08X INTR 0x%08X).\r\n", _u4MsdcAccuVect, MSDC_READ32(MSDC_INT));
            i4Ret = ERR_DAT_FAILED;
            goto ErrorEnd;
        }

        if (_u4MsdcAccuVect & INT_SD_DATTO)
        {
            MSDC_LOG(MSG_LVL_ERR, "CMD%d ARG 0x%08X - Data Timeout (AccuVect 0x%08X INTR 0x%08X).\r\n", cmd->cmdidx, cmd->cmdarg, _u4MsdcAccuVect, MSDC_READ32(MSDC_INT));
            i4Ret = ERR_DAT_FAILED;
            goto ErrorEnd;
        }
        else if (_u4MsdcAccuVect & INT_SD_DATA_CRCERR)
        {
            MSDC_LOG(MSG_LVL_ERR, "CMD%d ARG 0x%08X - Data CRC Error (AccuVect 0x%08X INTR 0x%08X).\r\n", cmd->cmdidx, cmd->cmdarg, _u4MsdcAccuVect, MSDC_READ32(MSDC_INT));
            i4Ret = ERR_DAT_FAILED;
            goto ErrorEnd;
        }
        else if ((_u4MsdcAccuVect & ~(IntrCheckVect)) || (0 != MSDC_READ32(MSDC_INT)))
        {
            MSDC_LOG(MSG_LVL_ERR, "UnExpect status (AccuVect 0x%08X INTR 0x%08X).\r\n", _u4MsdcAccuVect, MSDC_READ32(MSDC_INT));
            i4Ret = ERR_DAT_FAILED;
            goto ErrorEnd;
        }
    }
    else if(BASIC_DMA_DATA_MODE == (cmd->flags & 0x0F))
    {
        uint *pBufData = (uint*)((data->flags & MMC_DATA_READ)?(data->dest):(data->src));
        uint u4BufLen = (uint)((data->blocks)*(data->blocksize));
        unsigned int u4AccLen = 0;

		if(fgAutoCmd12)
		{
			IntrWaitVect = INT_SD_XFER_COMPLETE | INT_DMA_XFER_DONE | INT_SD_AUTOCMD_RDY;
			IntrCheckVect = INT_SD_XFER_COMPLETE | INT_DMA_XFER_DONE | INT_SD_AUTOCMD_RDY;
			MSDC_LOG(MSG_LVL_INFO, "1IntrWaitVect:%08X, Check vector: %08X\n", IntrWaitVect, IntrCheckVect);
		}
		else
		{
			IntrWaitVect = INT_SD_XFER_COMPLETE | INT_DMA_XFER_DONE;
			IntrCheckVect = INT_SD_XFER_COMPLETE | INT_DMA_XFER_DONE;
			MSDC_LOG(MSG_LVL_INFO, "2IntrWaitVect:%08X, Check vector: %08X\n", IntrWaitVect, IntrCheckVect);
		}

        MSDC_LOG(MSG_LVL_INFO, "DMA Mode: pBufData = 0x%x, Length = %08X\r\n", (uint)pBufData, u4BufLen);

        while (u4AccLen < u4BufLen)
        {
            MSDC_WRITE32(DMA_SA, (unsigned int) pBufData + u4AccLen);
            //TODO: Check why the buffer length of the last one can't be 512??
            if (u4BufLen - u4AccLen <= BASIC_DMA_MAX_LEN)
            {
                MSDC_LOG(MSG_LVL_INFO, "->Last: AccLen = %08X, waitvector: %08X, checkvector: %08X\r\n", u4AccLen, IntrWaitVect, IntrCheckVect);
#if defined(CC_MT5891) || defined(CONFIG_ARCH_MT5891)
                MSDC_WRITE32(DMA_LENGTH, (u4BufLen - u4AccLen));
                MSDC_WRITE32(DMA_CTRL, (DMA_BST_64 << DMA_CTRL_BST_SHIFT) | DMA_CTRL_LAST_BUF | DMA_CTRL_START);
#else
                MSDC_WRITE32(DMA_CTRL, ((u4BufLen - u4AccLen) << DMA_CTRL_XFER_SIZE_SHIFT) | (DMA_BST_64 << DMA_CTRL_BST_SHIFT) | DMA_CTRL_LAST_BUF | DMA_CTRL_START);
#endif

                // Wait for sd xfer complete
                if (MSDC_SUCCESS != MsdcWaitIntr(IntrWaitVect, MSDC_WAIT_DATA_COMPLETE_TIMEOUT_LIMIT_COUNT, 1))
                {
                    MSDC_LOG(MSG_LVL_ERR, "(L)%s: Failed to send/receive data (AccuVect 0x%08X INTR 0x%08X). %s line %d\r\n", __FUNCTION__, _u4MsdcAccuVect, MSDC_READ32(MSDC_INT), __FILE__, __LINE__);
                    return ERR_DAT_FAILED;
                }
                if ((_u4MsdcAccuVect & ~(IntrCheckVect)) || (0 != MSDC_READ32(MSDC_INT)))
                {
                    MSDC_LOG(MSG_LVL_ERR, "%s: Unexpected status (AccuVect 0x%08X INTR 0x%08X). %s line %d\r\n", __FUNCTION__, _u4MsdcAccuVect, MSDC_READ32(MSDC_INT), __FILE__, __LINE__);
                    return ERR_DAT_FAILED;
                }

                // Check DMA status
                if (0 != (MSDC_READ32(DMA_CFG) & (DMA_CFG_DMA_STATUS)))
                {
                    MSDC_LOG(MSG_LVL_ERR, "%s: Incorrect DMA status. DMA_CFG: 0x%08X\r\n", __FUNCTION__, MSDC_READ32(DMA_CFG));
                    return ERR_DAT_FAILED;
                }

                u4AccLen += u4BufLen - u4AccLen;
            }
            else
            {
                MSDC_LOG(MSG_LVL_INFO, "->AccLen = %08X, waitvector: %08X, checkvector: %08X\r\n", u4AccLen, IntrWaitVect, IntrCheckVect);
#if defined(CC_MT5891) || defined(CONFIG_ARCH_MT5891)
                MSDC_WRITE32(DMA_LENGTH, (BASIC_DMA_MAX_LEN));
                MSDC_WRITE32(DMA_CTRL, (DMA_BST_64 << DMA_CTRL_BST_SHIFT) | DMA_CTRL_START);
#else
                MSDC_WRITE32(DMA_CTRL, (BASIC_DMA_MAX_LEN << DMA_CTRL_XFER_SIZE_SHIFT) | (DMA_BST_64 << DMA_CTRL_BST_SHIFT) | DMA_CTRL_START);
#endif

                if (MSDC_SUCCESS != MsdcWaitIntr(INT_DMA_XFER_DONE, MSDC_WAIT_DATA_COMPLETE_TIMEOUT_LIMIT_COUNT, 1))
                {
                    MSDC_LOG(MSG_LVL_ERR, "(N)%s: Failed to send/receive data (AccuVect 0x%08X INTR 0x%08X). %s line %d\r\n", __FUNCTION__, _u4MsdcAccuVect, MSDC_READ32(MSDC_INT), __FILE__, __LINE__);
                    return ERR_DAT_FAILED;
                }

                if ((_u4MsdcAccuVect & ~(INT_DMA_XFER_DONE)) || (0 != MSDC_READ32(MSDC_INT)))
                {
                    MSDC_LOG(MSG_LVL_ERR, "%s: Unexpected status (AccuVect 0x%08X INTR 0x%08X). %s line %d\r\n", __FUNCTION__, _u4MsdcAccuVect, MSDC_READ32(MSDC_INT), __FILE__, __LINE__);
                    return ERR_DAT_FAILED;
                }
                u4AccLen += BASIC_DMA_MAX_LEN;
            }
        }

		/* DMA must be stopped even if the transfer was succeed */
		MsdcStopDMA();
    }
    else if(DESC_DMA_DATA_MODE == (cmd->flags & 0x0F))
    {
	      if(fgAutoCmd12)
	      {
	  	     IntrWaitVect = INT_SD_XFER_COMPLETE | INT_DMA_XFER_DONE | INT_SD_AUTOCMD_RDY;
	  	     IntrCheckVect = INT_SD_XFER_COMPLETE | INT_DMA_XFER_DONE | INT_SD_AUTOCMD_RDY | INT_DMA_Q_EMPTY;
	  	     MSDC_LOG(MSG_LVL_INFO, "1IntrWaitVect:%08X, Check vector: %08X\n", IntrWaitVect, IntrCheckVect);
	      }
	      else
	      {
	  	     IntrWaitVect = INT_SD_XFER_COMPLETE | INT_DMA_XFER_DONE;
	  	     IntrCheckVect = INT_SD_XFER_COMPLETE | INT_DMA_XFER_DONE | INT_DMA_Q_EMPTY;
	  	     MSDC_LOG(MSG_LVL_INFO, "2IntrWaitVect:%08X, Check vector: %08X\n", IntrWaitVect, IntrCheckVect);
	      }

        MSDC_SETBIT(DMA_CTRL, DMA_CTRL_START);

        // Wait for sd xfer complete
        if (MSDC_SUCCESS != MsdcWaitIntr(IntrWaitVect, 10*MSDC_WAIT_DATA_COMPLETE_TIMEOUT_LIMIT_COUNT, 1))
        {
            MSDC_LOG(MSG_LVL_ERR, "%s: Failed to send/receive data (AccuVect 0x%08X INTR 0x%08X). %s line %d\r\n", __FUNCTION__, _u4MsdcAccuVect, MSDC_READ32(MSDC_INT), __FILE__, __LINE__);
            i4Ret = CMD_ERR_DATA_FAILED;
            goto ErrorEnd;
        }

        if ((_u4MsdcAccuVect & ~(IntrCheckVect)) || (0 != MSDC_READ32(MSDC_INT)))
        {
            MSDC_LOG(MSG_LVL_ERR, "%s: Unexpected status (AccuVect 0x%08X INTR 0x%08X). %s line %d\r\n", __FUNCTION__, _u4MsdcAccuVect, MSDC_READ32(MSDC_INT), __FILE__, __LINE__);
            i4Ret = CMD_ERR_DATA_FAILED;
            goto ErrorEnd;
        }

        if(MSDC_READ32(DMA_CFG) & (DMA_CFG_GPD_CS_ERR))
        {
            MSDC_LOG(MSG_LVL_ERR, "Descriptor DMA GPD checksum error");
            i4Ret = CMD_ERR_DATA_FAILED;
            goto ErrorEnd;
        }

        if(MSDC_READ32(DMA_CFG) & (DMA_CFG_BD_CS_ERR))
        {
            MSDC_LOG(MSG_LVL_ERR, "Descriptor DMA BD checksum error");
            i4Ret = CMD_ERR_DATA_FAILED;
            goto ErrorEnd;
        }

        // Check DMA status
        if (0 != (MSDC_READ32(DMA_CFG) & (DMA_CFG_DMA_STATUS)))
        {
            MSDC_LOG(MSG_LVL_ERR, "%s: Incorrect DMA status. DMA_CFG: 0x%08X\r\n", __FUNCTION__, MSDC_READ32(DMA_CFG));
            i4Ret = CMD_ERR_DATA_FAILED;
            goto ErrorEnd;
        }

		/* DMA must be stopped even if the transfer was succeed */
		MsdcStopDMA();
    }


ErrorEnd:
    return i4Ret;

}

int MsdcReqCmdTune(struct mmc *mmc, struct mmc_cmd *cmd, struct mmc_data *data)
{
	INT32 result = MSDC_SUCCESS;
#if 1
	UINT32 cur_rsmpl = 0, orig_rsmpl = 0;
	UINT32 cur_rrdly_pad = 0, orig_rrdly_pad = 0;
	UINT32 cur_dl_cksel = 0, orig_dl_cksel = 0;
	UINT32 cur_ckgen = 0, orig_ckgen = 0;
	UINT32 cur_respdly_internal = 0, orig_respdly_internal = 0;
	UINT32 u4Status = 0;

#else
	UINT32 rsmpl, cur_rsmpl, orig_rsmpl;
	UINT32 rrdly, cur_rrdly, orig_rrdly;
	UINT32 skip = 1;
#endif

	MSDC_LOG(MSG_LVL_WARN, "----->Go into Command Tune!\n");

	MsdcReset();
	MsdcClrFifo();
	MsdcClrIntr();

#if 1

	RETRY_TUNE:

	MSDC_GET_FIELD(MSDC_IOCON, MSDC_IOCON_R_SMPL, orig_rsmpl);
	MSDC_GET_FIELD(PAD_TUNE, PAD_CMD_RXDLY, orig_rrdly_pad);
	MSDC_GET_FIELD(PATCH_BIT0, INT_DAT_LATCH_CK_SEL, orig_dl_cksel);
	MSDC_GET_FIELD(PATCH_BIT0, CKGEN_MSDC_DLY_SEL, orig_ckgen);
	MSDC_GET_FIELD(PAD_TUNE, PAD_CMD_RESP_RXDLY, orig_respdly_internal);


	cur_rsmpl = (orig_rsmpl + 1);
	MSDC_SET_FIELD(MSDC_IOCON, MSDC_IOCON_R_SMPL, cur_rsmpl & 1);

	if(cur_rsmpl >= 2){
		//rollback
		cur_rsmpl = 0;
		MSDC_SET_FIELD(MSDC_IOCON, MSDC_IOCON_R_SMPL, cur_rsmpl & 1);
		cur_rrdly_pad = (orig_rrdly_pad + 1);
		MSDC_SET_FIELD(PAD_TUNE, PAD_CMD_RXDLY, cur_rrdly_pad & 31);
	}

	//internel delay
	if (cur_rrdly_pad >= 32 ) {
		//rollback
		cur_rrdly_pad = 0;
		MSDC_SET_FIELD(PAD_TUNE, PAD_CMD_RXDLY, cur_rrdly_pad & 31);

		cur_respdly_internal = (orig_respdly_internal + 1) ;
		MSDC_SET_FIELD(PAD_TUNE, PAD_CMD_RESP_RXDLY, cur_respdly_internal & 31);
		}

	//ckgen
	if (cur_respdly_internal >= 32) {

		cur_respdly_internal = 0 ;
		MSDC_SET_FIELD(PAD_TUNE, PAD_CMD_RESP_RXDLY, cur_respdly_internal & 31);

		cur_ckgen = (orig_ckgen + 1) ;
		MSDC_SET_FIELD(PATCH_BIT0, CKGEN_MSDC_DLY_SEL, cur_ckgen & 31);
		}

	//latch_clock,for ckgen, only tune to 10
	if (cur_ckgen >= MAX_CKGEN_TUNE_TIME) {
		//rollback
		cur_ckgen = 0 ;
		MSDC_SET_FIELD(PATCH_BIT0, CKGEN_MSDC_DLY_SEL, cur_ckgen & 31);

		cur_dl_cksel = (orig_dl_cksel +1);
		MSDC_SET_FIELD(PATCH_BIT0, INT_DAT_LATCH_CK_SEL, cur_dl_cksel & 7);
		}

	//overflow, all those proper setttings can not cover the error, we exit
	if (cur_dl_cksel > 7)
	{
		Printf("UBOOT Tune CMD Failed! \n");
		return MSDC_FAILED;
	}


	//DO REQ

	Printf("UBOOT TUNE_CMD: rsmpl<%d> pad_delay<%d> internal_delay<%d> ckgen<%d>,latch_ck<%d> \n",
		cur_rsmpl, cur_rrdly_pad, cur_respdly_internal, cur_ckgen,cur_dl_cksel);

	result = MsdcReqCmdStart(mmc, cmd, data);
	if(result == MSDC_SUCCESS)
	{
		MSDC_LOG(MSG_LVL_WARN, "Command Tune Success\n");
		return MSDC_SUCCESS;
	}
	else
	{
		if(cmd->cmdidx == MMC_CMD_STOP_TRANSMISSION)//CMD12, check status
		{
			if(MSDC_SUCCESS != UbootGetMsdcStatus(mmc,&u4Status))
			{
			    MSDC_LOG(MSG_LVL_ERR, "%s Failed to get card status\n", __FUNCTION__);
			}

			if (((u4Status>>9)&0x0F) == 0x4)
			{
				return MSDC_SUCCESS;
			}
			MSDC_LOG(MSG_LVL_INFO, "(CMD12)Card Status: %08X\n", u4Status);

		}
		else if (_u4MsdcAccuVect & INT_SD_CMDTO)
		{
			return MSDC_FAILED;
		}
		goto RETRY_TUNE;
	}




#else
	orig_rsmpl = ((MSDC_READ32(MSDC_IOCON) & MSDC_IOCON_R_SMPL) >> MSDC_IOCON_R_SMPL_SHIFT);
	orig_rrdly = ((MSDC_READ32(PAD_TUNE) & PAD_CMD_RESP_RXDLY) >> PAD_CMD_RESP_RXDLY_SHIFT);

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
		}

		MSDC_CLRBIT(MSDC_IOCON, MSDC_IOCON_R_SMPL);
		MSDC_SETBIT(MSDC_IOCON, (cur_rsmpl << MSDC_IOCON_R_SMPL_SHIFT));

        result = MsdcReqCmdStart(mmc, cmd, data);
		if(result == MSDC_SUCCESS)
		{
			MSDC_LOG(MSG_LVL_WARN, "Command Tune Success\n");
			return MSDC_SUCCESS;
		}

		/* Lv2: PAD_CMD_RESP_RXDLY[26:22] */
		cur_rrdly = (orig_rrdly + rrdly + 1) % 32;
		MSDC_CLRBIT(PAD_TUNE, PAD_CMD_RESP_RXDLY);
		MSDC_SETBIT(PAD_TUNE, (cur_rrdly << PAD_CMD_RESP_RXDLY_SHIFT));
	}while (++rrdly < 32);
#endif

	return result;

}


int MsdcRequest(struct mmc *mmc, struct mmc_cmd *cmd, struct mmc_data *data)
{
    int i4Ret = MSDC_SUCCESS;

    i4Ret = MsdcReqCmdStart(mmc, cmd, data);
    if(MSDC_SUCCESS != i4Ret)
    {
        if(data)
        {
            if(MSDC_SUCCESS != MsdcErrorHandling(mmc, cmd, data))
            {
                goto ErrorEnd;
            }
        }

        #if MSDC_NEW_FIFO_PATH
            i4Ret = MsdcUbootOnlineAutoKTune(TRUE, FALSE, FALSE,mmc, cmd, data);
            if(MSDC_SUCCESS != i4Ret)
            {
                printf("uboot cmd tune file at MsdcRequest \n");
                MsdcDumpRegister();
                goto ErrorEnd;
            }
        #else
            i4Ret = MsdcReqCmdTune(mmc, cmd, data);
            if(MSDC_SUCCESS != i4Ret)
            {
                goto ErrorEnd;
            }
        #endif
    }

    if(data)
    {
        i4Ret = MsdcHandleDataTransfer(mmc, cmd, data);
        if(MSDC_SUCCESS != i4Ret)
        {
            if(MSDC_SUCCESS != MsdcErrorHandling(mmc, cmd, data))
            {
                goto ErrorEnd;
            }
        }
    }

ErrorEnd:
	return i4Ret;

}

int MsdcReqDataReadTune(struct mmc *mmc, struct mmc_cmd *cmd, struct mmc_data *data)
{

#if 1
	UINT32 cur_dsmpl = 0, orig_dsmpl;
	UINT32 cur_ckgen = 0,orig_ckgen;
	UINT32 cur_dl_cksel = 0,orig_dl_cksel;
	UINT32 cur_rd_pad_dly = 0,orig_rd_pad_dly = 0;
	INT32 result = 0;

#else
	UINT32 ddr=(sdHost[ch].hostAttr & MSDC_DATA_DDR_MODE_MASK)?1:0;
	UINT32 dcrc = 0;
	UINT32 rxdly, cur_rxdly0, cur_rxdly1;
	//uint rxdly, cur_rxdly;
	UINT32 dsmpl, cur_dsmpl,  orig_dsmpl;
	UINT32 cur_dat0,  cur_dat1,  cur_dat2,	cur_dat3;
	UINT32 cur_dat4,  cur_dat5,  cur_dat6,	cur_dat7;
	UINT32 orig_dat0, orig_dat1, orig_dat2, orig_dat3;
	UINT32 orig_dat4, orig_dat5, orig_dat6, orig_dat7;
	//uint cur_dat, orig_dat;
#if defined(CC_MT5890) || defined(CONFIG_ARCH_MT5890)||defined(CC_MT5882) || defined(CONFIG_ARCH_MT5882)|| \
	defined(CC_MT5883) || defined(CONFIG_ARCH_MT5883)||defined(CC_MT5891) || defined(CONFIG_ARCH_MT5891)
	UINT32 dsel, cur_dsel = 0, orig_dsel;
	//uint dl_cksel, cur_dl_cksel = 0, orig_dl_cksel;
#endif
	INT32 result = -1;
	UINT32 skip = 1;
#endif

	MSDC_LOG(MSG_LVL_WARN, "----->Go into Data Read Tune!\n");


#if 1

	RETRY_TUNE:

	MSDC_GET_FIELD(PATCH_BIT0, INT_DAT_LATCH_CK_SEL, orig_dl_cksel);
	MSDC_GET_FIELD(PATCH_BIT0, CKGEN_MSDC_DLY_SEL, orig_ckgen);
	MSDC_GET_FIELD(MSDC_IOCON, MSDC_IOCON_D_SMPL, orig_dsmpl);
	MSDC_GET_FIELD(PAD_TUNE, PAD_DAT_RD_RXDLY, orig_rd_pad_dly);

	cur_dsmpl = (orig_dsmpl + 1) ;
	MSDC_SET_FIELD(MSDC_IOCON, MSDC_IOCON_D_SMPL, cur_dsmpl & 1);

	//pad delay
	if(cur_dsmpl >= 2){
		//rollback
		cur_dsmpl = 0;
		MSDC_SET_FIELD(MSDC_IOCON, MSDC_IOCON_D_SMPL, cur_dsmpl & 1);
		cur_rd_pad_dly = (orig_rd_pad_dly + 1);
		MSDC_SET_FIELD(PAD_TUNE, PAD_DAT_RD_RXDLY, cur_rd_pad_dly & 31);
	}

	//ckgen
	if (cur_rd_pad_dly >= 32) {
		cur_rd_pad_dly = 0;
		MSDC_SET_FIELD(PAD_TUNE, PAD_DAT_RD_RXDLY, cur_rd_pad_dly & 31);

		cur_ckgen = (orig_ckgen + 1) ;
		MSDC_SET_FIELD(PATCH_BIT0, CKGEN_MSDC_DLY_SEL, cur_ckgen & 31);
		}

	//latch_clock,for ckgen, only tune to 10
	if (cur_ckgen >= MAX_CKGEN_TUNE_TIME ) {
		//rollback
		cur_ckgen = 0 ;
		MSDC_SET_FIELD(PATCH_BIT0, CKGEN_MSDC_DLY_SEL, cur_ckgen & 31);

		cur_dl_cksel = (orig_dl_cksel +1);
		MSDC_SET_FIELD(PATCH_BIT0, INT_DAT_LATCH_CK_SEL, cur_dl_cksel & 7);
		}


	//overflow, all those proper setttings can not cover the error, we exit
	if (cur_dl_cksel > 7)
	{
		Printf("UBOOT Tune READ Failed! \n");
		return MSDC_FAILED;
	}


	//DO REQ

	Printf("UBOOT TUNE_READ: rsmpl<%d> pad_delay<%d>  ckgen<%d>,latch_ck<%d>\n",
		cur_dsmpl, cur_rd_pad_dly, cur_ckgen,cur_dl_cksel);

	result = MsdcRequest(mmc, cmd, data);
	if(result == MSDC_SUCCESS)
	{
		MSDC_LOG(MSG_LVL_WARN, "Read Tune Success\n");
		return MSDC_SUCCESS;
	}
	else
	{
		goto RETRY_TUNE;
	}


#else

	orig_dsmpl = ((MSDC_READ32(MSDC_IOCON) & MSDC_IOCON_D_SMPL) >> MSDC_IOCON_D_SMPL_SHIFT);

	/* Tune Method 2. */
	MSDC_CLRBIT(MSDC_IOCON, MSDC_IOCON_D_DLYLINE_SEL);
	MSDC_SETBIT(MSDC_IOCON, (1 << MSDC_IOCON_D_DLYLINE_SEL_SHIFT));

	MSDC_LOG(MSG_LVL_WARN, "CRC(R) Error Register: %08X!\n", MSDC_READ32(SDC_DATCRC_STS));

#if defined(CC_MT5890) || defined(CONFIG_ARCH_MT5890)||defined(CC_MT5882) || defined(CONFIG_ARCH_MT5882)|| \
	defined(CC_MT5883) || defined(CONFIG_ARCH_MT5883)||defined(CC_MT5891) || defined(CONFIG_ARCH_MT5891)
	orig_dsel = ((MSDC_READ32(PATCH_BIT0) & CKGEN_MSDC_DLY_SEL) >> CKGEN_MSDC_DLY_SEL_SHIFT);
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

			MSDC_CLRBIT(MSDC_IOCON, MSDC_IOCON_D_SMPL);
			MSDC_SETBIT(MSDC_IOCON, (cur_dsmpl << MSDC_IOCON_D_SMPL_SHIFT));

                result = MsdcRequest(mmc, cmd, data);

			dcrc = MSDC_READ32(SDC_DATCRC_STS);
			if(!ddr)
				dcrc &= (~SDC_DATCRC_STS_NEG);

#if defined(CC_MT5890) || defined(CONFIG_ARCH_MT5890)||defined(CC_MT5882) || defined(CONFIG_ARCH_MT5882)|| \
	defined(CC_MT5883) || defined(CONFIG_ARCH_MT5883)||defined(CC_MT5891) || defined(CONFIG_ARCH_MT5891)
				MSDC_LOG(MSG_LVL_WARN, "TUNE_READ<%s> dcrc<0x%x> DATRDDLY0/1<0x%x><0x%x> dsmpl<0x%x> CKGEN_MSDC_DLY_SEL<0x%x>",
							(result == MSDC_SUCCESS && dcrc == 0) ? "PASS" : "FAIL", dcrc,
							MSDC_READ32(DAT_RD_DLY0), MSDC_READ32(DAT_RD_DLY1), cur_dsmpl, cur_dsel);
#else
				MSDC_LOG(MSG_LVL_WARN, "TUNE_READ<%s> dcrc<0x%x> DATRDDLY0/1<0x%x><0x%x> dsmpl<0x%x>",
							(result == MSDC_SUCCESS && dcrc == 0) ? "PASS" : "FAIL", dcrc,
							MSDC_READ32(DAT_RD_DLY0), MSDC_READ32(DAT_RD_DLY1), cur_dsmpl);
#endif

			if((result == MSDC_SUCCESS) && dcrc == 0)
			{
				goto done;
			}
			else
			{
				// Tuning Data error but Command error happens, directly return
				if((result != MSDC_SUCCESS) && (result != ERR_DAT_FAILED))
				{
					MSDC_LOG(MSG_LVL_WARN, "TUNE_READ(1): result<0x%x> ", result);
					goto done;
				}
				else if((result != MSDC_SUCCESS) && (result == ERR_DAT_FAILED))
				{
					  // Going On
					MSDC_LOG(MSG_LVL_WARN, "TUNE_READ(2): result<0x%x>", result);
				}
			}
		}

		cur_rxdly0 = MSDC_READ32(DAT_RD_DLY0);
		cur_rxdly1 = MSDC_READ32(DAT_RD_DLY1);

		/* E1 ECO. YD: Reverse */
		if (MSDC_READ32(ECO_VER) >= 4)
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
		if (MSDC_READ32(ECO_VER) >= 4)
		{
			cur_rxdly0 = (cur_dat0 << 24) | (cur_dat1 << 16) | (cur_dat2 << 8) | (cur_dat3 << 0);
			cur_rxdly1 = (cur_dat4 << 24) | (cur_dat5 << 16) | (cur_dat6 << 8) | (cur_dat7 << 0);
		}
		else
		{
			cur_rxdly0 = (cur_dat3 << 24) | (cur_dat2 << 16) | (cur_dat1 << 8) | (cur_dat0 << 0);
			cur_rxdly1 = (cur_dat7 << 24) | (cur_dat6 << 16) | (cur_dat5 << 8) | (cur_dat4 << 0);
		}

		MSDC_WRITE32(DAT_RD_DLY0, cur_rxdly0);
		MSDC_WRITE32(DAT_RD_DLY1, cur_rxdly1);
	}while(++rxdly < 32);
#if defined(CC_MT5890) || defined(CONFIG_ARCH_MT5890)||defined(CC_MT5882) || defined(CONFIG_ARCH_MT5882)|| \
	defined(CC_MT5883) || defined(CONFIG_ARCH_MT5883)||defined(CC_MT5891) || defined(CONFIG_ARCH_MT5891)
		cur_dsel = (orig_dsel + dsel + 1) % 32;
		MSDC_CLRBIT(PATCH_BIT0, CKGEN_MSDC_DLY_SEL);
		MSDC_SETBIT(PATCH_BIT0, (cur_dsel << CKGEN_MSDC_DLY_SEL_SHIFT));
	} while (++dsel < 32);
#endif

done:
#endif


	return result;
}


int MsdcReqDataWriteTune(struct mmc *mmc, struct mmc_cmd *cmd, struct mmc_data *data)
{
#if 1

	UINT32 cur_w_pad_dly = 0, orig_w_pad_dly = 0;
	UINT32 cur_w_internal_dly = 0, orig_w_internal_dly = 0;
	UINT32 cur_dl_cksel = 0, orig_dl_cksel = 0;
	UINT32 cur_ckgen = 0, orig_ckgen = 0;
	UINT32 cur_dsmpl = 0,	orig_dsmpl;
	INT32 result = -1;


	MSDC_GET_FIELD(PAD_TUNE, PAD_DAT_WR_RXDLY, orig_w_internal_dly);
	MSDC_GET_FIELD(PAD_TUNE, PAD_DAT_RD_RXDLY, orig_w_pad_dly);
	MSDC_GET_FIELD(PATCH_BIT0, INT_DAT_LATCH_CK_SEL, orig_dl_cksel);
	MSDC_GET_FIELD(PATCH_BIT0, CKGEN_MSDC_DLY_SEL, orig_ckgen);
	MSDC_GET_FIELD(MSDC_IOCON, MSDC_IOCON_W_D_SMPL_SEL, orig_dsmpl);

	RETRY_TUNE:

	cur_dsmpl = (orig_dsmpl + 1);
	MSDC_SET_FIELD(MSDC_IOCON, MSDC_IOCON_W_D_SMPL_SEL, cur_dsmpl & 1);

	//pad delay
	if(cur_dsmpl >= 2){
		//rollback
		cur_dsmpl = 0;
		MSDC_SET_FIELD(MSDC_IOCON, MSDC_IOCON_W_D_SMPL_SEL, cur_dsmpl & 1);

		cur_w_pad_dly = (orig_w_pad_dly + 1);
		MSDC_SET_FIELD(PAD_TUNE, PAD_DAT_RD_RXDLY, cur_w_pad_dly & 31);
	}

	//internel delay
	if (cur_w_pad_dly >= 32 ) {
		//rollback
		cur_w_pad_dly = 0;
		MSDC_SET_FIELD(PAD_TUNE, PAD_DAT_RD_RXDLY, cur_w_pad_dly & 31);

		cur_w_internal_dly = (orig_w_internal_dly + 1) ;
		MSDC_SET_FIELD(PAD_TUNE, PAD_DAT_WR_RXDLY, cur_w_internal_dly & 31);
		}

	//ckgen
	if (cur_w_internal_dly >= 32) {

		cur_w_internal_dly = 0 ;
		MSDC_SET_FIELD(PAD_TUNE, PAD_DAT_WR_RXDLY, cur_w_internal_dly & 31);

		cur_ckgen = (orig_ckgen + 1) ;
		MSDC_SET_FIELD(PATCH_BIT0, CKGEN_MSDC_DLY_SEL, cur_ckgen & 31);
		}

	//latch_clock,for ckgen, only tune to 10
	if (cur_ckgen >= MAX_CKGEN_TUNE_TIME) {
		//rollback
		cur_ckgen = 0 ;
		MSDC_SET_FIELD(PATCH_BIT0, CKGEN_MSDC_DLY_SEL, cur_ckgen & 31);

		cur_dl_cksel = (orig_dl_cksel +1);
		MSDC_SET_FIELD(PATCH_BIT0, INT_DAT_LATCH_CK_SEL, cur_dl_cksel & 7);
		}


	//overflow, all those proper setttings can not cover the error, we exit
		if (cur_dl_cksel > 7)
		{
			Printf("UBOOT Tune READ Failed! \n");
			return MSDC_FAILED;
		}


		//DO REQ

		Printf("UBOOT TUNE_WRITE: rsmpl<%d> pad_delay<%d> internal_delay<%d> ckgen<%d>,latch_ck<%d> \n",
			cur_dsmpl, cur_w_pad_dly, cur_w_internal_dly, cur_ckgen,cur_dl_cksel);

		result = MsdcRequest(mmc, cmd, data);
		if(result == MSDC_SUCCESS)
		{
			MSDC_LOG(MSG_LVL_WARN, "Write Tune Success\n");
			return MSDC_SUCCESS;
		}
		else
		{
			goto RETRY_TUNE;
		}


#else
	UINT32 wrrdly, cur_wrrdly = 0, orig_wrrdly;
	UINT32 dsmpl,  cur_dsmpl,  orig_dsmpl;
	UINT32 rxdly,  cur_rxdly0;
	UINT32 orig_dat0, orig_dat1, orig_dat2, orig_dat3;
	UINT32 cur_dat0,  cur_dat1,  cur_dat2,	cur_dat3;
#if defined(CC_MT5890) || defined(CONFIG_ARCH_MT5890)||defined(CC_MT5882) || defined(CONFIG_ARCH_MT5882)|| \
	defined(CC_MT5883) || defined(CONFIG_ARCH_MT5883)||defined(CC_MT5891) || defined(CONFIG_ARCH_MT5891)
	unsigned int d_cntr, cur_d_cntr = 0, orig_d_cntr;
#endif
	INT32 result = -1;
	UINT32 skip = 1;

	MSDC_LOG(MSG_LVL_WARN, "----->Go into Data Write Tune!\n");

	orig_wrrdly = ((MSDC_READ32(PAD_TUNE) & PAD_DAT_WR_RXDLY) >> PAD_DAT_WR_RXDLY_SHIFT);
	orig_dsmpl = ((MSDC_READ32(MSDC_IOCON) & MSDC_IOCON_D_SMPL) >> MSDC_IOCON_D_SMPL_SHIFT);
#if defined(CC_MT5890) || defined(CONFIG_ARCH_MT5890)||defined(CC_MT5882) || defined(CONFIG_ARCH_MT5882)|| \
	defined(CC_MT5883) || defined(CONFIG_ARCH_MT5883)||defined(CC_MT5891) || defined(CONFIG_ARCH_MT5891)
	orig_dsmpl = ((MSDC_READ32(MSDC_IOCON) & MSDC_IOCON_W_D_SMPL_SEL) >> MSDC_IOCON_W_D_SMPL_SHIFT);
#else
	orig_dsmpl = ((MSDC_READ32(MSDC_IOCON) & MSDC_IOCON_D_SMPL) >> MSDC_IOCON_D_SMPL_SHIFT);
#endif
	MSDC_LOG(MSG_LVL_WARN, "CRC(W) Error Register: %08X!\n", MSDC_READ32(SDC_DATCRC_STS));

	/* Tune Method 2. just DAT0 */
	MSDC_CLRBIT(MSDC_IOCON, MSDC_IOCON_D_DLYLINE_SEL);
	MSDC_SETBIT(MSDC_IOCON, (1 << MSDC_IOCON_D_DLYLINE_SEL_SHIFT));
	cur_rxdly0 = MSDC_READ32(DAT_RD_DLY0);

	/* E1 ECO. YD: Reverse */
	if (MSDC_READ32(ECO_VER) >= 4)
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
#if defined(CC_MT5890) || defined(CONFIG_ARCH_MT5890)||defined(CC_MT5882) || defined(CONFIG_ARCH_MT5882)|| \
	defined(CC_MT5883) || defined(CONFIG_ARCH_MT5883)||defined(CC_MT5891) || defined(CONFIG_ARCH_MT5891)
	orig_d_cntr = ((MSDC_READ32(PATCH_BIT1) & WRDAT_CRCS_TA_CNTR) >> WRDAT_CRCS_TA_CNTR_SHIFT);
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

#if defined(CC_MT5890) || defined(CONFIG_ARCH_MT5890)||defined(CC_MT5882) || defined(CONFIG_ARCH_MT5882)|| \
	defined(CC_MT5883) || defined(CONFIG_ARCH_MT5883)||defined(CC_MT5891) || defined(CONFIG_ARCH_MT5891)
					MSDC_CLRBIT(MSDC_IOCON, MSDC_IOCON_W_D_SMPL_SEL);
					MSDC_SETBIT(MSDC_IOCON, (cur_dsmpl << MSDC_IOCON_W_D_SMPL_SHIFT));
#else
					MSDC_CLRBIT(MSDC_IOCON, MSDC_IOCON_D_SMPL);
					MSDC_SETBIT(MSDC_IOCON, (cur_dsmpl << MSDC_IOCON_D_SMPL_SHIFT));
#endif

                    result = MsdcRequest(mmc, cmd, data);

#if defined(CC_MT5890) || defined(CONFIG_ARCH_MT5890)||defined(CC_MT5882) || defined(CONFIG_ARCH_MT5882)|| \
	defined(CC_MT5883) || defined(CONFIG_ARCH_MT5883)||defined(CC_MT5891) || defined(CONFIG_ARCH_MT5891)
					MSDC_LOG(MSG_LVL_WARN,	"TUNE_WRITE<%s> DSPL<%d> DATWRDLY<0x%x> MSDC_DAT_RDDLY0<0x%x> WRDAT_CRCS_TA_CNTR<0x%x>",
									  (result == MSDC_SUCCESS ? "PASS" : "FAIL"), cur_dsmpl, cur_wrrdly, cur_rxdly0, cur_d_cntr);
#else
					MSDC_LOG(MSG_LVL_WARN,	"TUNE_WRITE<%s> DSPL<%d> DATWRDLY<0x%x> MSDC_DAT_RDDLY0<0x%x>",
									  (result == MSDC_SUCCESS ? "PASS" : "FAIL"), cur_dsmpl, cur_wrrdly, cur_rxdly0);
#endif

				if(result == MSDC_SUCCESS)
				{
					goto done;
				}
				else
				{
					  // Tuning Data error but Command error happens, directly return
					if((result != MSDC_SUCCESS) && (result != ERR_DAT_FAILED))
					{
						MSDC_LOG(MSG_LVL_WARN, "TUNE_WRITE(1): result<0x%x>", result);

						goto done;
					}
					else if((result != MSDC_SUCCESS) && (result == ERR_DAT_FAILED))
					{
						  // Going On
						MSDC_LOG(MSG_LVL_WARN, "TUNE_WRITE(2): result<0x%x>", result);
					}
				}
			}

			cur_wrrdly = (orig_wrrdly + wrrdly + 1) % 32;
			MSDC_CLRBIT(PAD_TUNE, PAD_DAT_WR_RXDLY);
			MSDC_SETBIT(PAD_TUNE, (cur_wrrdly << PAD_DAT_WR_RXDLY_SHIFT));
		}while(++wrrdly < 32);

		cur_dat0 = (orig_dat0 + rxdly) % 32; /* only adjust bit-1 for crc */
		cur_dat1 = orig_dat1;
		cur_dat2 = orig_dat2;
		cur_dat3 = orig_dat3;

		/* E1 ECO. YD: Reverse */
		if (MSDC_READ32(ECO_VER) >= 4)
		{
			cur_rxdly0 = (cur_dat0 << 24) | (cur_dat1 << 16) | (cur_dat2 << 8) | (cur_dat3 << 0);
		}
		else
		{
			cur_rxdly0 = (cur_dat3 << 24) | (cur_dat2 << 16) | (cur_dat1 << 8) | (cur_dat0 << 0);
		}
		MSDC_WRITE32(DAT_RD_DLY0, cur_rxdly0);
	}while(++rxdly < 32);
#if defined(CC_MT5890) || defined(CONFIG_ARCH_MT5890)||defined(CC_MT5882) || defined(CONFIG_ARCH_MT5882)|| \
	defined(CC_MT5883) || defined(CONFIG_ARCH_MT5883)||defined(CC_MT5891) || defined(CONFIG_ARCH_MT5891)
		cur_d_cntr = (orig_d_cntr + d_cntr + 1) % 8;
		MSDC_CLRBIT(PATCH_BIT1, WRDAT_CRCS_TA_CNTR);
		MSDC_SETBIT(PATCH_BIT1, (cur_d_cntr << WRDAT_CRCS_TA_CNTR_SHIFT));
	} while (++d_cntr < 8);
#endif

done:

#endif

	return result;
}


int MsdcReqDataTune(struct mmc *mmc, struct mmc_cmd *cmd, struct mmc_data *data)
{
    int i4Ret = MSDC_SUCCESS;

    if (data->flags & MMC_DATA_READ)
    {
        i4Ret = MsdcReqDataReadTune(mmc, cmd, data);
    }
    else
    {
        i4Ret = MsdcReqDataWriteTune(mmc, cmd, data);
    }

    return i4Ret;
}

int MsdcSendCmd (struct mmc *mmc, struct mmc_cmd *cmd, struct mmc_data *data)
{
    int i4Ret = MSDC_SUCCESS;

    i4Ret = MsdcRequest(mmc, cmd, data);
    if((i4Ret != MSDC_SUCCESS) && data && (i4Ret == ERR_DAT_FAILED))
    {
        #if MSDC_NEW_FIFO_PATH
            i4Ret =  MsdcUbootOnlineAutoKTune(FALSE, (data->flags & MMC_DATA_READ), !(data->flags & MMC_DATA_READ),mmc, cmd, data);
            if (MSDC_SUCCESS != i4Ret)
            {
                printf("uboot data tune file at MsdcSendCmd \n");
                MsdcDumpRegister();
                return i4Ret;
            }
        #else
            i4Ret = MsdcReqDataTune(mmc, cmd, data);
            if(MSDC_SUCCESS != i4Ret)
            {
                return i4Ret;
            }
        #endif
    }

    return i4Ret;

}


int MsdcDMAWaitIntr(struct mmc_cmd *cmd, uint cur_off, uint bytes, uint total_sz)
{
    uint fgAutoCmd12, IntrWaitVect = 0, IntrCheckVect = 0;

    fgAutoCmd12 = ((cmd->flags>>4)?1:0);

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


    if(total_sz <= (cur_off + bytes))
    {
        // Wait for sd xfer complete
        if (MSDC_SUCCESS != MsdcWaitIntr(IntrWaitVect, MSDC_WAIT_DATA_COMPLETE_TIMEOUT_LIMIT_COUNT, 1))
        {
            MSDC_LOG(MSG_LVL_ERR, "(L)%s: Failed to send/receive data (AccuVect 0x%08X INTR 0x%08X). %s line %d\r\n", __FUNCTION__, _u4MsdcAccuVect, MSDC_READ32(MSDC_INT), __FILE__, __LINE__);
            return MSDC_FAILED;
        }
        if ((_u4MsdcAccuVect & ~(IntrCheckVect)) || (0 != MSDC_READ32(MSDC_INT)))
        {
            MSDC_LOG(MSG_LVL_ERR, "%s: Unexpected status (AccuVect 0x%08X INTR 0x%08X). %s line %d\r\n", __FUNCTION__, _u4MsdcAccuVect, MSDC_READ32(MSDC_INT), __FILE__, __LINE__);
            return MSDC_FAILED;
        }

        // Check DMA status
        if (0 != (MSDC_READ32(DMA_CFG) & (DMA_CFG_DMA_STATUS)))
        {
            MSDC_LOG(MSG_LVL_ERR, "%s: Incorrect DMA status. DMA_CFG: 0x%08X\r\n", __FUNCTION__, MSDC_READ32(DMA_CFG));
            return MSDC_FAILED;
        }

        return MSDC_SUCCESS;
    }
    else
    {
        if (MSDC_SUCCESS != MsdcWaitIntr(INT_DMA_XFER_DONE, MSDC_WAIT_DATA_COMPLETE_TIMEOUT_LIMIT_COUNT, 1))
        {
            MSDC_LOG(MSG_LVL_ERR, "(N)%s: Failed to send/receive data (AccuVect 0x%08X INTR 0x%08X). %s line %d\r\n", __FUNCTION__, _u4MsdcAccuVect, MSDC_READ32(MSDC_INT), __FILE__, __LINE__);

			uboot_print_setting();
            return MSDC_FAILED;
        }

        if ((_u4MsdcAccuVect & ~(INT_DMA_XFER_DONE)) || (0 != MSDC_READ32(MSDC_INT)))
        {
            MSDC_LOG(MSG_LVL_ERR, "%s: Unexpected status (AccuVect 0x%08X INTR 0x%08X). %s line %d\r\n", __FUNCTION__, _u4MsdcAccuVect, MSDC_READ32(MSDC_INT), __FILE__, __LINE__);
            return MSDC_FAILED;
        }

        return MSDC_SUCCESS;
    }
}


//No use, so MT5891 not modify here..
int MsdcDMAStart(uint cur_pos, uint cur_off, uint bytes, uint total_sz)
{
    MSDC_WRITE32(DMA_SA, cur_pos);

    if (total_sz <= cur_off + bytes)
    {
        MSDC_LOG(MSG_LVL_INFO, "->Last: AccLen = %08X\r\n", (total_sz - cur_off));
#if defined(CC_MT5891) || defined(CONFIG_ARCH_MT5891)
        MSDC_WRITE32(DMA_LENGTH, (total_sz - cur_off));
        MSDC_WRITE32(DMA_CTRL, (DMA_BST_64 << DMA_CTRL_BST_SHIFT) | DMA_CTRL_LAST_BUF | DMA_CTRL_START);
#else
        MSDC_WRITE32(DMA_CTRL, ((total_sz - cur_off) << DMA_CTRL_XFER_SIZE_SHIFT) | (DMA_BST_64 << DMA_CTRL_BST_SHIFT) | DMA_CTRL_LAST_BUF | DMA_CTRL_START);
#endif
    }
    else
    {
	      MSDC_LOG(MSG_LVL_INFO, "->AccLen = %08X\r\n", bytes);
#if defined(CC_MT5891) || defined(CONFIG_ARCH_MT5891)
        MSDC_WRITE32(DMA_LENGTH, (bytes));
        MSDC_WRITE32(DMA_CTRL, (DMA_BST_64 << DMA_CTRL_BST_SHIFT) | DMA_CTRL_START);
#else
        MSDC_WRITE32(DMA_CTRL, (bytes << DMA_CTRL_XFER_SIZE_SHIFT) | (DMA_BST_64 << DMA_CTRL_BST_SHIFT) | DMA_CTRL_START);
#endif
    }

    return MSDC_SUCCESS;
}

int MsdcDMATernmial(struct mmc *mmc, struct mmc_cmd *cmd, struct mmc_data *data)
{
    if(MsdcErrorHandling(mmc, cmd, data))
    {
        return MSDC_FAILED;
    }

    return MSDC_SUCCESS;
}

/* different clock needs different max driving strength
*/
void MsdcDrivingStrength(uint driving)
{
    //Printf("MT5891 uboot Set driving[ch %d] \n",ch);
	if (ch == 1) {
		//emmc setting
		//CLK,CMD,DAT0,DAT1
		MSDC_CLRBIT(0xF000D488, 0xFFF << 18);
		//MSDC_SETBIT(0xF000D488, ((0x5 & 0x7)<<18) | ((0x5&0x7) << 21) | ((0x5 & 0x7)<<24) | ((0x5 & 0x7)<<27));

		//for HQA SDR50 3.3V
		MSDC_SETBIT(0xF000D488, ((0x6 & 0x7)<<18) | ((0x7&0x7) << 21) | ((0x5 & 0x7)<<24) | ((0x5 & 0x7)<<27));

		//DAT2,DAT3,DAT4,DAT5,DAT6,DAT7
		MSDC_CLRBIT(0xF000D48C, 0x3FFFF << 0);
		MSDC_SETBIT(0xF000D48C, ((0x5 & 0x7) << 0) | ((0x5&0x7) << 3) |
			       ((0x5 & 0x7) << 6) | ((0x5 & 0x7) << 9) | ((0x5&0x7) << 12) | ((0x5&0x7) << 15));
		//SR0,SR1
		MSDC_CLRBIT(0xF000D480, 0xFFFFF << 12);
		MSDC_SETBIT(0xF000D480, (0x0 & 0xFFFFF) << 12);
		//R1,R0(50K)
		MSDC_CLRBIT(0xF000D4AC, 0xFFFFF << 12);
		MSDC_SETBIT(0xF000D4AC, (0x55556 & 0xFFFFF) << 12);
		//CLK pull down,CMD/DAT0 pull up
		MSDC_CLRBIT(0xF000D4B4, 0x3FF << 6);
		MSDC_SETBIT(0xF000D4B4, (0x1 & 0x3FF) << 6);
		//RCLK--DS PIN
		MSDC_CLRBIT(0xF000D4c4, 0x3F << 0);
		//MSDC_SETBIT(0xF000D4c4, (0x3 & 0x1F) << 0);//10k
		MSDC_SETBIT(0xF000D4c4, (0x5 & 0x1F) << 0);//50k

		MSDC_CLRBIT(0xF000D4c4, 0x7 << 17);
		MSDC_SETBIT(0xF000D4c4, (0x5 & 0x7) << 17);

		//bit5, DS PIN,AES arbitor,MUST SET
		MSDC_SETBIT(0xF000D4c4, (0x2 & 0x3) << 4);

	}
	else {
		//sd setting, 3.3V default
		Printf("SD 3.3V driving setting \n");
		MSDC_CLRBIT(0xF000D488, 0x3FFFF << 0);
		MSDC_SETBIT(0xF000D488, ((0x4 & 0x7) << 0) | ((0x4 & 0x7) << 3) | ((0x4 & 0x7) << 6) |
			       ((0x4 & 0x7) << 9) | ((0x4 & 0x7) << 12) | ((0x4 & 0x7) << 15));

		//1.8V, driving setting for sd
		/*
		MSDC_CLRBIT(0xF000D488, 0x3FFFF << 0);
		MSDC_SETBIT(0xF000D488, ((0x5 & 0x7) << 0) | ((0x5 & 0x7) << 3) | ((0x5 & 0x7) << 6) |
			       ((0x5 & 0x7) << 9) | ((0x5 & 0x7) << 12) | ((0x5 & 0x7) << 15));
		*/

		//SR0,SR1
		MSDC_CLRBIT(0xF000D480, 0xFFF << 0);
		MSDC_SETBIT(0xF000D480, (0x0 & 0xFFF) << 0);
		//R1,R0(50K)
		MSDC_CLRBIT(0xF000D4AC, 0xFFF << 0);
		MSDC_SETBIT(0xF000D4AC, (0x556 & 0xFFF) << 0);
		//CLK pull down,CMD/DAT0 pull up
		MSDC_CLRBIT(0xF000D4B4, 0x3F << 0);
		MSDC_SETBIT(0xF000D4B4, (0x1 & 0x3F) << 0);
	}


}

int MsdcSetClkfreq(uint clkFreq)
{
    uint idx = 0, ddr = 0;

	clkFreq /= (1000000);
    ddr = (speedmode == SPEED_MODE_DDR50)?1:0;

	MSDC_LOG(MSG_LVL_ERR, "uboot set clock: %d, ch %d \n", clkFreq,ch);

	//Set src clock to 400MHZ,00[22]
	if (400 == clkFreq)
	{
		MSDC_CLRBIT(MSDC_CFG, MSDC_CFG_HS400_CK_MODE);
		MSDC_SETBIT(MSDC_CFG, (0x1 << MSDC_CFG_HS400_CK_MODE_SHIFT));
	}

	do
    {
        if((clkFreq < msdcClk[0][idx]) ||
		   (ddr && (msdcClk[2][idx] != 2)))
		    continue;
		else
			break;

	}while(++idx < MSDC_CLK_IDX_MAX);

	idx = (idx >= MSDC_CLK_IDX_MAX)?MSDC_CLK_IDX_MAX:idx;

	// Enable msdc_src_clk gate
    if (ch == 1)
        MSDC_SETBIT(MSDC_CLK_S_REG1, MSDC_CLK_GATE_BIT);
    else
        MSDC_SETBIT(MSDC_CLK_S_REG0, MSDC_CLK_GATE_BIT);

    // Set clock source value
	if (ch == 1)
	{
        // Clr msdc_src_clk selection
        MSDC_CLRBIT(MSDC_CLK_S_REG1, MSDC_CLK_SEL_MASK);
        #if  defined(CC_EMMC_DDR50)
        MSDC_SETBIT(MSDC_CLK_S_REG1, 0x8<<0);
        #endif
        MSDC_SETBIT(MSDC_CLK_S_REG1, msdcClk[1][idx]<<0);
    }
    else
    {
        // Clr msdc_src_clk selection
        MSDC_CLRBIT(MSDC_CLK_S_REG0, MSDC_CLK_SEL_MASK);
        MSDC_SETBIT(MSDC_CLK_S_REG0, msdcClk[1][idx]<<0);
    }

	// Set clock mode value
	MSDC_CLRBIT(MSDC_CFG, (((UINT32)0x03) << 20));
    MSDC_SETBIT(MSDC_CFG, ((msdcClk[2][idx]) << 20));

    // Set clock divide value
    MSDC_CLRBIT(MSDC_CFG, (((UINT32)0xFFF) << 8));
    MSDC_SETBIT(MSDC_CFG, ((msdcClk[3][idx]) << 8));


    // Disable msdc_src_clk gate
    if (ch == 1)
        MSDC_CLRBIT(MSDC_CLK_S_REG1, MSDC_CLK_GATE_BIT);
    else
        MSDC_CLRBIT(MSDC_CLK_S_REG0, MSDC_CLK_GATE_BIT);


    // Wait until clock is stable
    if (MSDC_FAILED == MsdcWaitClkStable())
    {
        MSDC_LOG(MSG_LVL_ERR, "(%s)Set Bus Clock as %d(MHz) Failed!\n", ddr?"DDR":"SDR", msdcClk[0][idx]);
        return MSDC_FAILED;
    }

	MSDC_LOG(MSG_LVL_ERR, "uboot id %d: src %d: mode %d  div %d  driving %d\n", idx, msdcClk[1][idx], msdcClk[2][idx],
		                                                   msdcClk[3][idx], msdcClk[4][idx]);
    MSDC_LOG(MSG_LVL_ERR, "(%s)Set Bus Clock as %d(MHz) Success!\n", ddr?"DDR":"SDR", msdcClk[0][idx]);


    MsdcDrivingStrength(msdcClk[4][idx]);


	//Special for HS200/HS400,CLK,CMD,DAT
	if ((400 == clkFreq)|| (200 == clkFreq))
	{
		MSDCUbootDrivingMore(5,6,5);//To cover coner IC,DATA[6] for HS400 corner IC, HS200 can use DATA[5]
	}

	if (clkFreq >= 50)
	{
	    LDR_ENV_T *prLdrEnv = (LDR_ENV_T *)CC_LDR_ENV_OFFSET;
	    msdc_autok_t msdcenv;

		MsdcUbootFifoPathSelAndInit();


#if defined(CC_KERNEL_HS200_SUPPORT) || defined(CC_KERNEL_HS400_SUPPORT)
		{
			if(sizeof(prLdrEnv->szMSDCAutoK) != sizeof(msdc_autok_t))
			{
				printf("Loade the autok param fail! %d != %d\n", sizeof(prLdrEnv->szMSDCAutoK), sizeof(msdc_autok_t));
				BUG();
			}
			memcpy((void *)(&msdcenv), (void *)(prLdrEnv->szMSDCAutoK), sizeof(msdc_autok_t));
			MSDC_ApplyAutoK_Param(&msdcenv,clkFreq);
		}
#endif

	}

    return MSDC_SUCCESS;
}

int MsdcSetBusWidth (int busWidth)
{
    /* Modify MSDC Register Settings */
    if (0 == busWidth)
    {
        MSDC_WRITE32(SDC_CFG, (MSDC_READ32(SDC_CFG) & ~SDC_CFG_BW_MASK) | (0x00 <<  SDC_CFG_BW_SHIFT));
    }
    else if ((1 == busWidth) ||
		     (5 == busWidth))
    {
        MSDC_WRITE32(SDC_CFG, (MSDC_READ32(SDC_CFG) & ~SDC_CFG_BW_MASK) | (0x01 <<  SDC_CFG_BW_SHIFT));
    }
    else if ((2 == busWidth) ||
		     (6 == busWidth))
    {
        MSDC_WRITE32(SDC_CFG, (MSDC_READ32(SDC_CFG) & ~SDC_CFG_BW_MASK) | (0x02 <<  SDC_CFG_BW_SHIFT));
    }
	else
	{
        MSDC_LOG(MSG_LVL_INFO, "Invalid buswidth value: %d!\n", busWidth);
	}

    return MSDC_SUCCESS;
}


void MsdcSetIos(struct mmc *mmc)
{
    uint clock = mmc->clock;
    uint busWidth = mmc->bus_width;

    MsdcSetBusWidth(busWidth);
    MsdcSetClkfreq(0);
    MsdcSetClkfreq(clock);
}

void MSDC_PinMux(uint u4Ch)
{
    ch = u4Ch;

//MSDC Controller 0, Basic Address: 0xF0012000
//MSDC Controller 1, Basic Address: 0xF006D000

}

void mtk_mmc_init(void)
{
    emmc_dev = (struct mmc*)malloc(sizeof(struct mmc));
	if (NULL == emmc_dev)
	{
		Printf("uboot mmc alloc fail\n");
		return;
	}
	fg_autok_apply_done = FALSE;


    //Initial some structure element
    //sprintf(emmc_dev->name, "%s", "emmc0");
    emmc_dev->voltages = (MMC_VDD_30_31|MMC_VDD_31_32|MMC_VDD_32_33|MMC_VDD_33_34);
    emmc_dev->f_min = 397000;

#if defined(CC_EMMC_HS400)
    emmc_dev->f_max = 400000000;
    emmc_dev->host_caps = (MMC_MODE_HS|MMC_MODE_HS_52MHz|MMC_MODE_8BIT|MMC_MODE_DDR_18V|MMC_MODE_HS400_MASK|MMC_MODE_DDR_MASK);
#elif defined(CC_EMMC_HS200)
    emmc_dev->f_max = 200000000;
    emmc_dev->host_caps = (MMC_MODE_HS|MMC_MODE_HS_52MHz|MMC_MODE_8BIT|MMC_MODE_DDR_18V|MMC_MODE_HS200_18V);
#elif  defined(CC_EMMC_DDR50)
    emmc_dev->f_max = 200000000;
    emmc_dev->host_caps = (MMC_MODE_HS|MMC_MODE_HS_52MHz|MMC_MODE_8BIT|MMC_MODE_DDR_18V);
	printf("Set max clk to 200MHZ in DDR50 mode \n");
#else
    emmc_dev->f_max = 52000000;
    emmc_dev->host_caps = (MMC_MODE_HS|MMC_MODE_HS_52MHz|MMC_MODE_8BIT);
	printf("Set max clk to 50MHZ in default \n");
#endif

    emmc_dev->send_cmd = MsdcSendCmd;
    emmc_dev->set_ios = MsdcSetIos;
    emmc_dev->init = MsdcInit;
    mmc_register(emmc_dev);

	Printf("Uboot max clock %d \n",emmc_dev->f_max);

#ifdef CC_MTD_ENCRYPT_SUPPORT
    // aes init
    MsdcAesInit(128);
#endif
#ifdef CC_PARTITION_WP_SUPPORT
    // aes init
    MsdcPartitionWpConfig(1);
#endif
#if (MSDC_EMMC_INIT_UNIFY_EN == 1)
    LDR_ENV_T *prLdrEnv = (LDR_ENV_T *)CC_LDR_ENV_OFFSET;
    msdc_env_t msdcenv;

    if(sizeof(prLdrEnv->szMSDCenv1) != sizeof(msdc_env_t))
    {
        printf("%d != %d\n", sizeof(prLdrEnv->szMSDCenv1), sizeof(msdc_env_t));
        BUG();
    }

	x_memset(szAutoK_CmdLine, 0, sizeof(szAutoK_CmdLine));
    memcpy((void *)(&msdcenv), (void *)(prLdrEnv->szMSDCenv1), sizeof(msdc_env_t));
    //memset(EXT_CSD, 0x00, 512);
    memcpy((void *)(&emmc_dev->ocr), (void *)(msdcenv.ocr), sizeof(uint));
    memcpy((void *)(&emmc_dev->rca), (void *)(msdcenv.rca), sizeof(ushort));
    memcpy((void *)(emmc_dev->cid), (void *)(msdcenv.cid), 4*sizeof(uint));
    memcpy((void *)(emmc_dev->csd), (void *)(msdcenv.csd), 4*sizeof(uint));
    //EXT_CSD[185] = msdcenv.ext_csd_185;
    //EXT_CSD[192] = msdcenv.ext_csd_192;
    //EXT_CSD[196] = msdcenv.ext_csd_196;
    //EXT_CSD[212] = msdcenv.ext_csd_212;
    //EXT_CSD[213] = msdcenv.ext_csd_213;
    //EXT_CSD[214] = msdcenv.ext_csd_214;
    //EXT_CSD[215] = msdcenv.ext_csd_215;
    emmc_dev->clock = msdcClk[0][MSDC_NORM_CLK_IDX]*1000000;
    //MsdcClrTiming();

    if(msdcenv.fgHost != 0)
    {
        printf("HOST 1\n");
        emmc_dev->host_caps |= MMC_MODE_8BIT;
        emmc_dev->bus_width = 8;
    }
    else
    {
        printf("HOST 0\n");
        emmc_dev->host_caps |= MMC_MODE_4BIT;
        emmc_dev->bus_width = 4;
    }

    //MSDC_PinMux(msdcenv.fgHost);
    MsdSetDataMode(dataMode);
    if (mmc_init_fast(emmc_dev))
    {
        MSDC_LOG(MSG_LVL_ERR, "MMC init failed\n");
        //BUG();
    }
#else
    emmc_dev->host_caps |= MMC_MODE_8BIT;
    //emmc_dev->host_caps |= MMC_MODE_4BIT;

    MSDC_PinMux(1);
    //MSDC_PinMux(0);
    if (mmc_init(emmc_dev))
    {
        MSDC_LOG(MSG_LVL_ERR, "MMC init failed\n");
       // BUG();
    }
#endif
#ifdef CC_SUPPORT_MULTI_EMMC_SIZE
        msdc_capacity = emmc_dev->capacity;
        tmpsize1 = (msdc_capacity & 0xffff0000)>>32;
        tmpsize2 = msdc_capacity & 0x0000ffff;
        printf("[MSDC_MT5399_MMC] emmc capacity high: 0x%X, low: 0x%X~\n",tmpsize1, tmpsize2);
#endif


}

int board_mmc_init(bd_t *bis)
{
    mtk_mmc_init();
    return 0;
}

int cpu_mmc_init(bd_t *bis)
{
    return 0;
}

#endif // CC_EMMC_BOOT

