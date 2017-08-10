#ifndef _MTK_MSDC_ETT_DAT_H_
#define _MTK_MSDC_ETT_DAT_H_

#include <mtk_msdc.h>

#ifdef CONFIG_MTK_USE_NEW_TUNE
static inline void msdc_register_ett(struct msdc_host *host, u32 *cid)
{
	host->load_ett_settings = NULL;
}
#elif defined(CONFIG_MTK_USE_LEGACY_TUNE)
extern void msdc_register_ett(struct msdc_host *host, u32 *cid);
#else
static inline void msdc_register_ett(struct msdc_host *host, u32 *cid)
{
	pr_err("%s => CONFIG_MTK_USE_NEW_TUNE & CONFIG_MTK_USE_LEGACY_TUNE all undefined!!!!!!!!!!!!!!!!\n", __func__);
	host->load_ett_settings = NULL;
}
#endif

#endif /* _MTK_MSDC_ETT_DAT_H_ */
