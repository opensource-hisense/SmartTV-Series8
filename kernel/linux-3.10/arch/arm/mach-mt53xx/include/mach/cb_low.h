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
/*-----------------------------------------------------------------------------
 * $Author: dtvbm11 $
 * $Date: 2015/12/22 $
 * $RCSfile: cb_low.h,v $
 * $Revision: #1 $
 *---------------------------------------------------------------------------*/

/** @file cb_low.h
 *  This header file declares low-layer support of callback interface.
 */

#ifndef CB_LOW_H
#define CB_LOW_H


//-----------------------------------------------------------------------------
// Include files
//-----------------------------------------------------------------------------

#include <mach/lnklist.h>
#include <mach/mt53xx_cb.h>
#include <linux/wait.h>

//-----------------------------------------------------------------------------
// Type definitions
//-----------------------------------------------------------------------------

/// Callback event.
typedef struct _CB_CALLBACK_EVENT_T
{
    DLIST_ENTRY_T(_CB_CALLBACK_EVENT_T) rLink;
    #ifdef CC_MTAL_MULTIPROCESS_CALLBACK
    // This is used for multiple callback reference.
    UINT32              u4BitMask;
    #endif
    // This *MUST* be the last one member of CB_CALLBACK_EVENT_T
    CB_GET_CALLBACK_T   rGetCb;
} CB_CALLBACK_EVENT_T;

typedef struct _CB_EVENT_COUNT_T
{
    UINT32 puCB00IdleFctPid[CB_FCT_NUM];
#ifdef CC_MTAL_MULTIPROCESS_CALLBACK	
	UINT32 puCB01IdleFctPid[CB_FCT_NUM];
	UINT32 puCB02IdleFctPid[CB_FCT_NUM];
#endif
} CB_EVENT_COUNT_T;

#ifdef CC_MTAL_MULTIPROCESS_CALLBACK
#define BITMASK_SINGLE              (1U << 0)
#define BITMASK_MULTI1              (1U << 1)
#define BITMASK_MULTI2              (1U << 2)
#define BITMASK_MULTI3              (1U << 3)
#define BITMASK_MULTI(i)            (1U << (i))
#endif

#ifdef CC_MTAL_MULTIPROCESS_CALLBACK
#define CB_MULTIPROCESS             (1)
#define CB_MULTIPROCESS_REPLACE     (1)
#define CB_MULTIPROCESS_LAST_REPLACE     (1)
#else
#define CB_MULTIPROCESS             (0)
#define CB_MULTIPROCESS_REPLACE     (0)
#endif

//-----------------------------------------------------------------------------
// Inter-file functions
//-----------------------------------------------------------------------------

void _CB_Init(void);
INT32 _CB_RegCbFct(CB_FCT_ID_T eFctId, INT32 i4SubId, pid_t rCbPid);
pid_t _CB_GetPid(CB_FCT_ID_T eFctId);
int _CB_GetCbCount(void);
CB_EVENT_COUNT_T _CB_ParsingEventQueue(void);
INT32 _CB_UnRegCbFct(CB_FCT_ID_T eFctId, INT32 i4SubId, pid_t rCbPid);
INT32 _CB_PutEvent(CB_FCT_ID_T eFctId, INT32 i4TagSize, void *pvTag);
#ifdef CC_MTAL_MULTIPROCESS_CALLBACK
INT32 _CB_RegMultiCbFct(CB_FCT_ID_T eFctId, INT32 i4SubId, pid_t rCbPid);
INT32 _CB_UnRegMultiCbFct(CB_FCT_ID_T eFctId, INT32 i4SubId, pid_t rCbPid);
void _CB_FreeEvent(CB_CALLBACK_EVENT_T *prCbEv, UINT32 u4DoneMask);
CB_CALLBACK_EVENT_T *_CB_GetEvent(UINT32 *pu4DoneMask);
#else
CB_CALLBACK_EVENT_T *_CB_GetEvent(void);
#endif
//void _CB_PutThdIntoWQ(void);

extern wait_queue_head_t _rWq;
#define  _CB_PutThdIntoWQ(condition) wait_event_interruptible(_rWq, condition)



void _CB_ClearPendingEventEntries(void);
void _CB_ClearPendingFctEventEntries(CB_FCT_ID_T eFctId);
void _CB_ClearCbIdArray(void);

#if defined(CC_SUPPORT_STR)
int mt53xx_STR_init(void);
#endif

#endif //CB_LOW_H

