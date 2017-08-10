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
#include <linux/mtk_thermal.h>

struct met_api_tbl {
	int (*met_tag_start) (unsigned int class_id, const char *name);
	int (*met_tag_end) (unsigned int class_id, const char *name);
	int (*met_tag_oneshot) (unsigned int class_id, const char *name, unsigned int value);
	int (*met_tag_dump) (unsigned int class_id, const char *name, void *data, unsigned int length);
	int (*met_tag_disable) (unsigned int class_id);
	int (*met_tag_enable) (unsigned int class_id);
	int (*met_set_dump_buffer) (int size);
	int (*met_save_dump_buffer) (const char *pathname);
	int (*met_save_log) (const char *pathname);
};

struct met_api_tbl met_ext_api;
EXPORT_SYMBOL(met_ext_api);

int met_tag_start(unsigned int class_id, const char *name)
{
	if (met_ext_api.met_tag_start) {
		return met_ext_api.met_tag_start(class_id, name);
	}
	return 0;
}

int met_tag_end(unsigned int class_id, const char *name)
{
	if (met_ext_api.met_tag_end) {
		return met_ext_api.met_tag_end(class_id, name);
	}
	return 0;
}

int met_tag_oneshot(unsigned int class_id, const char *name, unsigned int value)
{
	//trace_printk("8181888\n");
	if (met_ext_api.met_tag_oneshot) {
		return met_ext_api.met_tag_oneshot(class_id, name, value);
	}
	//else {
	//	trace_printk("7171777\n");
	//}
	return 0;
}

int met_tag_dump(unsigned int class_id, const char *name, void *data, unsigned int length)
{
	if (met_ext_api.met_tag_dump) {
		return met_ext_api.met_tag_dump(class_id, name, data, length);
	}
	return 0;
}

int met_tag_disable(unsigned int class_id)
{
	if (met_ext_api.met_tag_disable) {
		return met_ext_api.met_tag_disable(class_id);
	}
	return 0;
}

int met_tag_enable(unsigned int class_id)
{
	if (met_ext_api.met_tag_enable) {
		return met_ext_api.met_tag_enable(class_id);
	}
	return 0;
}

int met_set_dump_buffer(int size)
{
	if (met_ext_api.met_set_dump_buffer) {
		return met_ext_api.met_set_dump_buffer(size);
	}
	return 0;
}

int met_save_dump_buffer(const char *pathname)
{
	if (met_ext_api.met_save_dump_buffer) {
		return met_ext_api.met_save_dump_buffer(pathname);
	}
	return 0;
}

int met_save_log(const char *pathname)
{
	if (met_ext_api.met_save_log) {
		return met_ext_api.met_save_log(pathname);
	}
	return 0;
}

struct met_thermal_api_tbl met_thermal_ext_api;
EXPORT_SYMBOL(met_thermal_ext_api);

int mtk_thermal_get_temp(MTK_THERMAL_SENSOR_ID id)
{
	if (met_thermal_ext_api.mtk_thermal_get_temp) {
		return met_thermal_ext_api.mtk_thermal_get_temp(id);
	}
	return -127000;
}

EXPORT_SYMBOL(met_tag_start);
EXPORT_SYMBOL(met_tag_end);
EXPORT_SYMBOL(met_tag_oneshot);
EXPORT_SYMBOL(met_tag_dump);
EXPORT_SYMBOL(met_tag_disable);
EXPORT_SYMBOL(met_tag_enable);
EXPORT_SYMBOL(met_set_dump_buffer);
EXPORT_SYMBOL(met_save_dump_buffer);
EXPORT_SYMBOL(met_save_log);
EXPORT_SYMBOL(mtk_thermal_get_temp);


