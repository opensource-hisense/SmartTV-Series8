/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 */
/* MediaTek Inc. (C) 2012. All rights reserved.
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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/met_drv.h>
#include <linux/mutex.h>

static DEFINE_MUTEX(met_dev_mutex);

#define MET_DEV_MAX		10

//Adding met_ext_dev memory need ensure the room of array is correct
struct metdevice *met_ext_dev2[MET_DEV_MAX+1] =
{
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	(struct metdevice *)0xFFFFFFFF  //Used for lock/unlock status and end item
};

int met_ext_dev_add(struct metdevice *metdev)
{
	int i;

	if (try_module_get(metdev->owner) == 0) {
		//Cannot lock MET device owner
		return -1;
	}

	mutex_lock(&met_dev_mutex);

	if (met_ext_dev2[MET_DEV_MAX] != (struct metdevice *)0xFFFFFFFF) {
		//The met_ext_dev has been locked by met driver 
		mutex_unlock(&met_dev_mutex);
		module_put(metdev->owner);
		return -2;
	}

	for (i=0; i<MET_DEV_MAX; i++) {
		if(met_ext_dev2[i]==NULL) break;
	}

	if (i>=MET_DEV_MAX)	{ //No room for new MET device table pointer
		mutex_unlock(&met_dev_mutex);
		module_put(metdev->owner);
		return -3;
	}

	met_ext_dev2[i] = metdev;

	mutex_unlock(&met_dev_mutex);	
	return 0;
}

int met_ext_dev_del(struct metdevice *metdev)
{
	int i;

	mutex_lock(&met_dev_mutex);

	if (met_ext_dev2[MET_DEV_MAX] != (struct metdevice *)0xFFFFFFFF) {
		//The met_ext_dev has been locked by met driver 
		mutex_unlock(&met_dev_mutex);
		return -1;
	}

	for (i=0; i<MET_DEV_MAX; i++) {
		if(met_ext_dev2[i]==metdev) break;
	}

	if (i>=MET_DEV_MAX)	{ //No such device table pointer found
		mutex_unlock(&met_dev_mutex);
		return -2;
	}
	met_ext_dev2[i] = NULL;

	mutex_unlock(&met_dev_mutex);

	module_put(metdev->owner);
	return 0;
}

int met_ext_dev_lock(int flag)
{
	mutex_lock(&met_dev_mutex);
	if (flag)
		met_ext_dev2[MET_DEV_MAX] = (struct metdevice *)0xFFFFFFF0;
	else
		met_ext_dev2[MET_DEV_MAX] = (struct metdevice *)0xFFFFFFFF;
	mutex_unlock(&met_dev_mutex);
	return MET_DEV_MAX;
}


EXPORT_SYMBOL(met_ext_dev2);
EXPORT_SYMBOL(met_ext_dev_add);
EXPORT_SYMBOL(met_ext_dev_del);
EXPORT_SYMBOL(met_ext_dev_lock);

