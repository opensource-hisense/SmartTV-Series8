/*
 * This confidential and proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 * (C) COPYRIGHT 2010-2012 ARM Limited
 * ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 */

#ifndef __MALI_DEVICE_PAUSE_RESUME_H__
#define __MALI_DEVICE_PAUSE_RESUME_H__

#include "mali_osk.h"

/**
 * Pause the scheduling and power state changes of Mali device driver.
 * mali_dev_resume() must always be called as soon as possible after this function
 * in order to resume normal operation of the Mali driver.
 *
 * @param power_is_on Receives the power current status of Mali GPU. MALI_TRUE if GPU is powered on
 */
void mali_dev_pause(mali_bool *power_is_on);

/**
 * Resume scheduling and allow power changes in Mali device driver.
 * This must always be called after mali_dev_pause().
 */
void mali_dev_resume(void);

#endif /* __MALI_DEVICE_PAUSE_RESUME_H__ */
