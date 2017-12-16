/*
 * This confidential and proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 * (C) COPYRIGHT 2012 ARM Limited
 * ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 */

#ifndef __MALI_SCHEDULER_H__
#define __MALI_SCHEDULER_H__

#include "mali_osk.h"
#include "mali_gp_scheduler.h"
#include "mali_pp_scheduler.h"

_mali_osk_errcode_t mali_scheduler_initialize(void);
void mali_scheduler_terminate(void);

u32 mali_scheduler_get_new_id(void);

/**
 * @brief Reset all groups
 *
 * This function resets all groups known by the both the PP and GP scheuduler.
 * This must be called after the Mali HW has been powered on in order to reset
 * the HW.
 */
MALI_STATIC_INLINE void mali_scheduler_reset_all_groups(void)
{
	mali_gp_scheduler_reset_all_groups();
	mali_pp_scheduler_reset_all_groups();
}

/**
 * @brief Zap TLB on all active groups running \a session
 *
 * @param session Pointer to the session to zap
 */
MALI_STATIC_INLINE void mali_scheduler_zap_all_active(struct mali_session_data *session)
{
	mali_gp_scheduler_zap_all_active(session);
	mali_pp_scheduler_zap_all_active(session);
}

#endif /* __MALI_SCHEDULER_H__ */
