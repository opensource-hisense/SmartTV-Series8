/*-----------------------------------------------------------------------------
 * drivers/usb/musb_qmu/mtk_qmu_api.h
 *
 * MT53xx USB driver
 *
 * Copyright (c) 2008-2012 MediaTek Inc.
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
 *---------------------------------------------------------------------------*/


#ifndef MTK_QMU_API_H
#define MTK_QMU_API_H

#ifdef CONFIG_USB_QMU_SUPPORT
#include "mtk_hcd.h"

extern void QMU_select_dma_ch(MGC_LinuxCd *pThis, uint8_t channel, uint8_t burstmode);

extern void QMU_disable_q(MGC_LinuxCd *pThis, uint8_t bEnd, uint8_t isRx);

extern void QMU_disable_all_q(MGC_LinuxCd *pThis);

extern int QMU_is_enabled(MGC_LinuxCd *pThis, uint8_t EP_Num, uint8_t isRx);

extern int QMU_clean(MGC_LinuxCd *pThis);

extern int QMU_init(MGC_LinuxCd *pThis);

extern int QMU_enable(MGC_LinuxCd *pThis, uint8_t EP_Num, uint8_t isRx, uint8_t isZLP, uint8_t isCSCheck, uint8_t isEmptyCheck);

extern void QMU_disable(MGC_LinuxCd *pThis, uint8_t EP_Num, uint8_t isRx);

extern int QMU_insert_task(MGC_LinuxCd *pThis, uint8_t EP_Num, uint8_t isRx, uint32_t buf_addr, uint32_t length, uint8_t isIOC);

extern void QMU_stop(MGC_LinuxCd *pThis, uint8_t EP_Num, uint8_t isRx);

extern int QMU_irq(MGC_LinuxCd *pThis, uint32_t qisar);

#endif
#endif
