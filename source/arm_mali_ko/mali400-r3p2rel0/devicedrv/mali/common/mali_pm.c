/*
 * This confidential and proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 * (C) COPYRIGHT 2011-2012 ARM Limited
 * ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 */

#include "mali_pm.h"
#include "mali_kernel_common.h"
#include "mali_osk.h"
#include "mali_gp_scheduler.h"
#include "mali_pp_scheduler.h"
#include "mali_scheduler.h"
#include "mali_kernel_utilization.h"
#include "mali_group.h"

static mali_bool mali_power_on = MALI_FALSE;

_mali_osk_errcode_t mali_pm_initialize(void)
{
	_mali_osk_pm_dev_enable();
	return _MALI_OSK_ERR_OK;
}

void mali_pm_terminate(void)
{
	_mali_osk_pm_dev_disable();
}

void mali_pm_core_event(enum mali_core_event core_event)
{
	MALI_DEBUG_ASSERT(MALI_CORE_EVENT_GP_START == core_event ||
	                  MALI_CORE_EVENT_PP_START == core_event ||
	                  MALI_CORE_EVENT_GP_STOP  == core_event ||
	                  MALI_CORE_EVENT_PP_STOP  == core_event);

	if (MALI_CORE_EVENT_GP_START == core_event || MALI_CORE_EVENT_PP_START == core_event)
	{
		_mali_osk_pm_dev_ref_add();
		if (mali_utilization_enabled())
		{
			mali_utilization_core_start(_mali_osk_time_get_ns());
		}
	}
	else
	{
		_mali_osk_pm_dev_ref_dec();
		if (mali_utilization_enabled())
		{
			mali_utilization_core_end(_mali_osk_time_get_ns());
		}
	}
}

/* Reset GPU after power up */
static void mali_pm_reset_gpu(void)
{
	/* Reset all L2 caches */
	mali_l2_cache_reset_all();

	/* Reset all groups */
	mali_scheduler_reset_all_groups();
}

void mali_pm_os_suspend(void)
{
	MALI_DEBUG_PRINT(3, ("Mali PM: OS suspend\n"));
	mali_gp_scheduler_suspend();
	mali_pp_scheduler_suspend();
	mali_group_power_off();
	mali_power_on = MALI_FALSE;
}

void mali_pm_os_resume(void)
{
	MALI_DEBUG_PRINT(3, ("Mali PM: OS resume\n"));
	if (MALI_TRUE != mali_power_on)
	{
		mali_pm_reset_gpu();
		mali_group_power_on();
	}
	mali_gp_scheduler_resume();
	mali_pp_scheduler_resume();
	mali_power_on = MALI_TRUE;
}

void mali_pm_runtime_suspend(void)
{
	MALI_DEBUG_PRINT(3, ("Mali PM: Runtime suspend\n"));
	mali_group_power_off();
	mali_power_on = MALI_FALSE;
}

void mali_pm_runtime_resume(void)
{
	MALI_DEBUG_PRINT(3, ("Mali PM: Runtime resume\n"));
	if (MALI_TRUE != mali_power_on)
	{
		mali_pm_reset_gpu();
		mali_group_power_on();
	}
	mali_power_on = MALI_TRUE;
}
