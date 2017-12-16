/*
 * This confidential and proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 * (C) COPYRIGHT 2012 ARM Limited
 * ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 */

#include "mali_kernel_common.h"
#include "mali_osk.h"

static _mali_osk_atomic_t mali_job_autonumber;

_mali_osk_errcode_t mali_scheduler_initialize(void)
{
	if ( _MALI_OSK_ERR_OK != _mali_osk_atomic_init(&mali_job_autonumber, 0))
	{
		MALI_DEBUG_PRINT(1,  ("Initialization of atomic job id counter failed.\n"));
		return _MALI_OSK_ERR_FAULT;
	}

	return _MALI_OSK_ERR_OK;
}

void mali_scheduler_terminate(void)
{
	_mali_osk_atomic_term(&mali_job_autonumber);
}

u32 mali_scheduler_get_new_id(void)
{
	u32 job_id = _mali_osk_atomic_inc_return(&mali_job_autonumber);
	return job_id;
}

