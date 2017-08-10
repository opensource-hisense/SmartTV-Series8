/*
 * vm_linux/chiling/uboot/board/mt5398/mt5398_panel.c
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

//-----------------------------------------------------------------------------
// Include files
//-----------------------------------------------------------------------------

#include <common.h>

#include "pmx_drvif.h"
#include "osd_drvif.h"
#include "panel.h"
#include "drv_video.h"
#include "drv_vdoclk.h"
#include "drv_display.h"
#include "drv_lvds.h"
#include "sv_const.h"
#include "hw_scpos.h"
#include "hw_psc.h"
#include "hw_lvds.h"
#include "vdp_if.h"
#include "nptv_if.h"
#include "x_printf.h"
#include "c_model.h"

//-----------------------------------------------------------------------------
// Configurations
//-----------------------------------------------------------------------------

//---------------------------------------------------------------------
// Constant definitions
//---------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Static variables
//-----------------------------------------------------------------------------
static const UINT32 g_au4PlaneArray[PMX_MAX_INPORT_NS] = { PMX_OSD2, PMX_MAX_INPORT_NS, PMX_MAX_INPORT_NS, PMX_MAX_INPORT_NS, PMX_MAIN};

//-----------------------------------------------------------------------------
// Public functions
//-----------------------------------------------------------------------------

#ifdef LOADER_LOGO_FLASHOFFSET
//-----------------------------------------------------------------------------
/** PmxDisplay(): Setup Panel Pmx/LVDS driver.
 *  This function is going to setup panel pmx and lvds driver with background
 *  color.
 *
 *  @param  u4Background    This is background color setting. 0xRRGGBB.
 *  @retval 0               Success.
 *  @retval otherwise       Failed.
 */
//-----------------------------------------------------------------------------
UINT32 PmxDisplay(UINT32 u4Background)
{
    static UINT32 _fgInit = 0;
    UINT32 i;

    if (_fgInit)
    {
        vRegWriteFldAlign(MUTE_04, u4Background & 0xff, B_BACKGROUND_MJC);
        vRegWriteFldAlign(MUTE_04, (u4Background>>8)&0xff, G_BACKGROUND_MJC);
        vRegWriteFldAlign(MUTE_04, (u4Background>>16)&0xff, R_BACKGROUND_MJC);

        PMX_SetBg(u4Background, FALSE);
        return 0;
    }
    _fgInit = 1;

#ifdef CC_MT5363
    vDrvVOPLLAutoKVCOBand();
#endif /* CC_MT5391 */

    PMX_Init();
    LoadPanelIndex();
    // power off panel
    vApiPanelPowerSequence(FALSE);

    vDrvOutputStageInit();
    vDrvDisplayInit();
    vDrvLVDSInit();
    vDrvVOPLLSet(PANEL_GetPixelClk60Hz());
    vDrvSetLCDTiming();

    // set background color
    vRegWriteFldAlign(MUTE_04, u4Background & 0xff, B_BACKGROUND_MJC);
    vRegWriteFldAlign(MUTE_04, (u4Background>>8)&0xff, G_BACKGROUND_MJC);
    vRegWriteFldAlign(MUTE_04, (u4Background>>16)&0xff, R_BACKGROUND_MJC);
    PMX_SetBg(u4Background, FALSE);
    // remove vdp plane
    PMX_SetPlaneOrderArray(g_au4PlaneArray);

    for (i=0; i<=PMX_UPDATE_DELAY; i++)
    {
        // Use OutputVSync to update PlaneOrder.
        PMX_OnOutputVSync();
    }

    // disable main video window
    vIO32WriteFldAlign(PSCCTRL_11, 0, PSC_OUTPUT_HEIGHT);
    vIO32WriteFldAlign(PSCCTRL_0A, 1, PSC_SET_RES_TOGGLE);
    vIO32WriteFldAlign(PSCCTRL_0A, 0, PSC_SET_RES_TOGGLE);

    // power on panel
    vApiPanelPowerSequence(TRUE);
    DumpPanelAttribute(PANEL_DUMP_CURRENT);

    return 0;
}

//-----------------------------------------------------------------------------
/** OsdDisplay(): Setup OSD driver.
 *  This function is going to setup osd driver with bmp info.
 *
 *  @param  u4BmpAddr       The bmp data pointer.
 *  @param  u4Width         The width info of the bmp data.
 *  @param  u4Height        The height info of the bmp data.
 *  @retval 0               Success.
 *  @retval otherwise       Failed.
 */
//-----------------------------------------------------------------------------
UINT32 OsdDisplay(UINT32 u4ColorMode, UINT32 u4BmpAddr, UINT32 u4Width, UINT32 u4Height)
{
    UINT32 u4RegionList, u4Region, u4BmpPitch;
    UINT32 u4PanelWidth, u4PanelHeight;
    INT32 ret;

    Printf("Color:%d BmpAddr:0x%08x Width:%d Height:%d\n", (int)u4ColorMode, u4BmpAddr, (int)u4Width, (int)u4Height);

    OSD_Init();

    u4PanelWidth = PANEL_GetPanelWidth();
    u4PanelHeight = PANEL_GetPanelHeight();

    Printf("Panel %d x %d \n", (int)u4PanelWidth, (int)u4PanelHeight);

    ret = OSD_RGN_LIST_Create(&u4RegionList);
    if (ret != OSD_RET_OK) return 1;
    u4BmpPitch = 0;
    OSD_GET_PITCH_SIZE(u4ColorMode, u4Width, u4BmpPitch); // to set u4BmpPitch by u4ColorMode and u4Width.
    ret = OSD_RGN_Create(&u4Region, u4Width, u4Height, (void *)u4BmpAddr,
                            u4ColorMode, u4BmpPitch, 0, 0, u4Width, u4Height);
    if (ret != OSD_RET_OK) return 2;
    ret = OSD_RGN_Insert(u4Region, u4RegionList);
    if (ret != OSD_RET_OK) return 3;
    ret = OSD_PLA_FlipTo(OSD_PLANE_2, u4RegionList);
    if (ret != OSD_RET_OK) return 4;

#if 1   // original bmp size.
    ret = OSD_RGN_Set(u4Region, OSD_RGN_POS_X, (u4PanelWidth - u4Width) >> 1);
    if (ret != OSD_RET_OK) return 5;
    ret = OSD_RGN_Set(u4Region, OSD_RGN_POS_Y, (u4PanelHeight - u4Height) >> 1);
    if (ret != OSD_RET_OK) return 6;
#else   // scale size.
    ret = OSD_SC_Scale(OSD_SCALER_2, 1, u4Width, u4Height, u4PanelWidth, u4PanelHeight);
    if (ret != OSD_RET_OK) return 7;
#endif

    ret = OSD_RGN_SetBigEndian(u4Region, TRUE);
    if (ret != OSD_RET_OK) return 8;
    
    ret = OSD_PLA_Enable(OSD_PLANE_2, TRUE);
    if (ret != OSD_RET_OK) return 9;

    return 0;
}

//-----------------------------------------------------------------------------
/** PanelLogo(): display logo on panel.
 *  This function is going to find out the logo location and call lvds/pmx/osd 
 *  low level driver to display logo on panel.
 *
 *  @param  u4BmpAddr       The memory offset to store logo file.
 *  @retval 0               Success.
 *  @retval otherwise       Failed.
 */
//-----------------------------------------------------------------------------
UINT32 PanelLogo(UINT32 u4BmpAddr)
{
    UINT32 u4Ret;

    Printf("Display background:0x%08x\n", (unsigned int)DRVCUST_InitGet(eLoaderLogoBackground));
    PmxDisplay(DRVCUST_InitGet(eLoaderLogoBackground));
    Printf("OsdDisplay(%d, 0x%08x, %d, %d)\n",
                    LOADER_LOGO_COLOR_MODE,
                    (unsigned int)u4BmpAddr,
                    LOADER_LOGO_WIDTH,
                    LOADER_LOGO_HEIGHT);
    u4Ret = OsdDisplay(LOADER_LOGO_COLOR_MODE,
                    u4BmpAddr,
                    LOADER_LOGO_WIDTH,
                    LOADER_LOGO_HEIGHT);

    return u4Ret;
}

#endif // LOADER_LOGO_FLASHOFFSET
