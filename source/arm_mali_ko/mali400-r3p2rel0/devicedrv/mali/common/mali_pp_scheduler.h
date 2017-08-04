/*
 * This confidential and proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 * (C) COPYRIGHT 2012 ARM Limited
 * ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 */

#ifndef __MALI_PP_SCHEDULER_H__
#define __MALI_PP_SCHEDULER_H__

#include "mali_osk.h"
#include "mali_pp_job.h"
#include "mali_group.h"

_mali_osk_errcode_t mali_pp_scheduler_initialize(void);
void mali_pp_scheduler_terminate(void);

void mali_pp_scheduler_job_done(struct mali_group *group, struct mali_pp_job *job, u32 sub_job, mali_bool success);

void mali_pp_scheduler_suspend(void);
void mali_pp_scheduler_resume(void);

/** @brief Abort all PP jobs from session running or queued
 *
 * This functions aborts all PP jobs from the specified session. Queued jobs are removed from the queue and jobs
 * currently running on a core will be aborted.
 *
 * @param session Pointer to session whose jobs should be aborted
 */
void mali_pp_scheduler_abort_session(struct mali_session_data *session);

/**
 * @brief Reset all groups
 *
 * This function resets all groups known by the PP scheuduler. This must be
 * called after the Mali HW has been powered on in order to reset the HW.
 *
 * This function is intended for power on reset of all cores.
 * No locking is done, which can only be safe if the scheduler is paused and
 * all cores idle. That is always the case on init and power on.
 */
void mali_pp_scheduler_reset_all_groups(void);

/**
 * @brief Zap TLB on all groups with \a session active
 *
 * The scheculer will zap the session on all groups it owns.
 */
void mali_pp_scheduler_zap_all_active(struct mali_session_data *session);

int mali_pp_scheduler_get_queue_depth(void);
u32 mali_pp_scheduler_dump_state(char *buf, u32 size);

#endif /* __MALI_PP_SCHEDULER_H__ */
