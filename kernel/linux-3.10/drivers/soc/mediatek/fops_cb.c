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
 *
 * $Author: dtvbm11 $
 * $Date: 2015/12/22 $
 * $RCSfile: fops_cb.c,v $
 * $Revision: #1 $
 *
 *---------------------------------------------------------------------------*/

/** @file cb_mod.c
 *  Callback interface of MT53XX.
 */


//-----------------------------------------------------------------------------
// Include files
//-----------------------------------------------------------------------------

//#include "ionodes.h"
#include <linux/soc/mediatek/cb_low.h>
//#include "x_linux.h"
#include <linux/poll.h>
#include <linux/ioport.h>
#include <linux/mm.h>
#include <asm/io.h>
#include <linux/slab.h>
#include <linux/sched.h>
//#include "x_assert.h"
#include <linux/soc/mediatek/mt53xx_cb.h>
#include <linux/wait.h>
#include <linux/kernel.h>
#include <linux/version.h>

//-----------------------------------------------------------------------------
// Constant definitions
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Static Variables
//-----------------------------------------------------------------------------
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 27)
#define DECLARE_MUTEX_LOCKED(name) \
       struct semaphore name = __SEMAPHORE_INITIALIZER(name, 0)
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 24)
#define DECLARE_MUTEX_LOCKED(name) __DECLARE_SEMAPHORE_GENERIC(name,0)
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
#define USE_UNLOCK_IOCTL
#endif


#ifdef CB_TIEMR_DEBUG
typedef struct _private_data
{
    HANDLE_T timer;
}private_data;
#endif


//#define CB_TIEMR_DEBUG
#define CB_TIMER_TIMEROUT 1000

//-----------------------------------------------------------------------------
// Static functions
//-----------------------------------------------------------------------------
#ifdef CB_TIEMR_DEBUG
static VOID _cb_timer_timeout(HANDLE_T  pt_tm_handle, VOID *pv_tag)
{
    CB_CALLBACK_EVENT_T* prCbEv =(CB_CALLBACK_EVENT_T*) pv_tag;

    if(prCbEv != NULL)
    {
        printk("_cb_timer_timeout Id(%d) Timer Out\n",(int)prCbEv->rGetCb.eFctId);
    }
    else
    {
        printk("_cb_timer_timeout Timer Out\n");
    }
}
#endif


// Public functions
//-----------------------------------------------------------------------------


#ifndef USE_UNLOCK_IOCTL
int cb_ioctl(struct inode *inode, struct file *file, unsigned int cmd,
             unsigned long arg)
#else
long cb_ioctl(struct file *file, unsigned int cmd,
              unsigned long arg)
#endif
{
    int retval = 0;
    CB_SET_CALLBACK_T rSetCb;
    CB_CALLBACK_EVENT_T *prCbEv = NULL;
    int sig_return;
    CB_GET_PID_T rGetPidCb;
    CB_GET_CBQUEUE_COUNT_T rGetCbCount;
    #if CB_MULTIPROCESS
    UINT32 u4DoneMask;
    #endif
    
#ifdef CB_TIEMR_DEBUG
    private_data* pdata = (private_data*) file->private_data;
#endif

	switch (cmd) {
	case CB_SET_CALLBACK:
        if (!access_ok(VERIFY_READ, (void __user *)arg,
                       sizeof(CB_SET_CALLBACK_T)))
        {
            return -EFAULT;
        }

        if (copy_from_user(&rSetCb, (void __user *)arg,
                           sizeof(CB_SET_CALLBACK_T)))
        {
            return -EFAULT;
        }
        #if CB_MULTIPROCESS_REPLACE
        retval = _CB_RegMultiCbFct(rSetCb.eFctId, rSetCb.i4SubId, rSetCb.rCbPid);
        #else
        retval = _CB_RegCbFct(rSetCb.eFctId, rSetCb.i4SubId, rSetCb.rCbPid);
        #endif
	    break;
#if CB_MULTIPROCESS
	case CB_SET_MULTI_CALLBACK:
        if (!access_ok(VERIFY_READ, (void __user *)arg,
                       sizeof(CB_SET_CALLBACK_T)))
        {
            return -EFAULT;
        }

        if (copy_from_user(&rSetCb, (void __user *)arg,
                           sizeof(CB_SET_CALLBACK_T)))
        {
            return -EFAULT;
        }

        retval = _CB_RegMultiCbFct(rSetCb.eFctId, rSetCb.i4SubId, rSetCb.rCbPid);

	    break;
#endif
    case CB_GET_CALLBACK:
        if (!access_ok(VERIFY_WRITE, (void __user *)arg,
                       sizeof(CB_SET_CALLBACK_T)))
        {
            //ASSERT(0);
            return -EFAULT;
        }
#ifdef CB_TIEMR_DEBUG
        x_timer_stop(pdata->timer);
#endif

#if CB_MULTIPROCESS
        sig_return = _CB_PutThdIntoWQ((prCbEv = _CB_GetEvent(&u4DoneMask)) != NULL);
#else
        sig_return = _CB_PutThdIntoWQ((prCbEv = _CB_GetEvent()) != NULL);
#endif
        if (sig_return)
        {
            return -EINTR;
        }
		
		if(prCbEv != NULL)
        {
			#ifdef CB_TIEMR_DEBUG
        	x_timer_start(pdata->timer, CB_TIMER_TIMEROUT, X_TIMER_FLAG_ONCE, _cb_timer_timeout, (VOID*)prCbEv);
			#endif        
        	//ASSERT((sizeof(CB_GET_CALLBACK_T) + prCbEv->rGetCb.i4TagSize) < CB_MAX_STRUCT_SIZE);

        	if (copy_to_user((void __user *)arg, &(prCbEv->rGetCb),
                         sizeof(CB_GET_CALLBACK_T) + prCbEv->rGetCb.i4TagSize))
        	{
            	printk("copy_to_user error, user addr=0x%x\n",
                   (unsigned int) (void __user *)arg);
            	retval = -EFAULT;
        	}
                #if CB_MULTIPROCESS
        	_CB_FreeEvent(prCbEv, u4DoneMask);
                #else
        	kfree(prCbEv);
                #endif
        }
        break;

	case CB_UNSET_CALLBACK:
        if (!access_ok(VERIFY_READ, (void __user *)arg,
                       sizeof(CB_SET_CALLBACK_T)))
        {
            return -EFAULT;
        }

        if (copy_from_user(&rSetCb, (void __user *)arg,
                           sizeof(CB_SET_CALLBACK_T)))
        {
            return -EFAULT;
        }
        #if CB_MULTIPROCESS_REPLACE
        retval = _CB_UnRegMultiCbFct(rSetCb.eFctId, rSetCb.i4SubId, rSetCb.rCbPid);
        #else
        retval = _CB_UnRegCbFct(rSetCb.eFctId, rSetCb.i4SubId, rSetCb.rCbPid);
        #endif
	    break;
        #if CB_MULTIPROCESS
	case CB_UNSET_MULTI_CALLBACK:
        if (!access_ok(VERIFY_READ, (void __user *)arg,
                       sizeof(CB_SET_CALLBACK_T)))
        {
            return -EFAULT;
        }

        if (copy_from_user(&rSetCb, (void __user *)arg,
                           sizeof(CB_SET_CALLBACK_T)))
        {
            return -EFAULT;
        }

        retval = _CB_UnRegMultiCbFct(rSetCb.eFctId, rSetCb.i4SubId, rSetCb.rCbPid);

	    break;
        #endif
    case CB_GET_PID:
            
        if (!access_ok(VERIFY_READ, (void __user *)arg,
                       sizeof(CB_GET_PID_T)))
        {
            return -EFAULT;
        }

        if (copy_from_user(&rGetPidCb, (void __user *)arg,
                           sizeof(CB_GET_PID_T)))
        {
            return -EFAULT;
        }

        rGetPidCb.rCbPid  = _CB_GetPid(rGetPidCb.eFctId);
        //printk("GetPID %d\n",rGetPidCb.rCbPid);
        if (copy_to_user((void __user *)arg, &rGetPidCb,
                         sizeof(CB_GET_PID_T)))
        {
            printk("copy_to_user error, user addr=0x%x\n",
                   (unsigned int) (void __user *)arg);
            retval = -EFAULT;
        }
        break;
		
    case CB_GET_CBQUEUE_COUNT:
            
        if (!access_ok(VERIFY_READ, (void __user *)arg,
                       sizeof(CB_GET_CBQUEUE_COUNT_T)))
        {
            return -EFAULT;
        }

        if (copy_from_user(&rGetCbCount, (void __user *)arg,
                           sizeof(CB_GET_CBQUEUE_COUNT_T)))
        {
            return -EFAULT;
        }

        rGetCbCount.iCount = _CB_GetCbCount();
        printk("GetCbCount = %d\n",rGetCbCount.iCount);
        if (copy_to_user((void __user *)arg, &rGetCbCount,
                         sizeof(CB_GET_CBQUEUE_COUNT_T)))
        {
            printk("copy_to_user error, user addr=0x%x\n",
                   (unsigned int) (void __user *)arg);
            retval = -EFAULT;
        }
        break;	
		
#if 0
	case CB_SET_TEMINATE:
	{
		int i4Dummy = 0;
		_CB_PutEvent(CB_MTAL_TEMINATE_TRIGGER, (INT32)sizeof(int), (void*)&i4Dummy);
	}
	break;
#endif

    default:
        //ASSERT(0);
        break;
	}

	return retval;
}


static int cb_open(struct inode *inode, struct file *file)
{
#ifdef CB_TIEMR_DEBUG
    private_data* pdata;
#endif

#ifdef CB_TIEMR_DEBUG
    pdata = kmalloc(sizeof(private_data), GFP_KERNEL);    
    file->private_data = (void*)pdata; 
    if (file->private_data == NULL)
    {
        return -1;
    }    
    x_timer_create(&(pdata->timer));
#endif
    return 0;
}

extern void _CB_ClearPendingEventEntries(void);
extern void _CB_ClearCbIdArray(void);

static int cb_release(struct inode *inode, struct file *file)
{
    _CB_ClearPendingEventEntries();
    _CB_ClearCbIdArray();
    
#ifdef CB_TIEMR_DEBUG
    kfree(file->private_data);
#endif   

    return 0;
}

struct file_operations cb_native_fops = {
#ifndef USE_UNLOCK_IOCTL
    .ioctl		= cb_ioctl,
#else
    .unlocked_ioctl	= cb_ioctl,
#endif
#if IS_ENABLED(CONFIG_COMPAT)
	.compat_ioctl = cb_ioctl,
#endif
    .open       = cb_open,
    .release    = cb_release
};

EXPORT_SYMBOL(cb_native_fops);
