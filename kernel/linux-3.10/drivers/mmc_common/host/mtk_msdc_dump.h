#ifndef _MTK_MSDC_DUMP_H
#define _MTK_MSDC_DUMP_H

#include <mtk_msdc.h>
#include <linux/mmc/mmc.h>
#include <linux/mmc/card.h>

#define MSDC_DUMP_LOG(fmt, ...) \
	do { \
		pr_emerg("[KE-DUMP][%s|%d] "fmt, __func__, __LINE__, ##__VA_ARGS__); \
	} while(0)

#define MSDC_DUMP_ERR_EXIT(expr, ret) \
	do { \
		ret = expr; \
		if (ret != 0) { \
			MSDC_DUMP_LOG("execute "#expr" failed!!!\n"); \
			return ret; \
		} \
	} while(0)

#define MTK_MSDC_DUMP_DBG	0
#define MAX_POLLING_STATUS (50000)

struct msdc_dump {
	struct msdc_host *host;
};

#define MSDC_PS_DAT0			(0x1  << 16)	/* R  */

/* command argument */
#define CMD0_ARG			 (0)
#define CMD2_ARG			 (0)
#define CMD3_ARG			 (0)
#define ACMD6_ARG_EMMC		  \
		((MMC_SWITCH_MODE_WRITE_BYTE << 24) | (EXT_CSD_BUS_WIDTH << 16)\
		| (EXT_CSD_BUS_WIDTH_4 << 8) | EXT_CSD_CMD_SET_NORMAL)

#define CMD_RAW(cmd, rspt, dtyp, rw, len, stop) \
	  (cmd) | (rspt << 7) | (dtyp << 11) | (rw << 13) | \
	  (len << 16) | (stop << 14)

/* compare the value with mt6573 [Fix me]*/
#define CMD0_RAW	 CMD_RAW(0 , msdc_rsp[RESP_NONE], 0, 0,   0, 0)
#define CMD1_RAW	 CMD_RAW(1 , msdc_rsp[RESP_R3], 0, 0,	0, 0)
#define CMD2_RAW	 CMD_RAW(2 , msdc_rsp[RESP_R2], 0, 0,	0, 0)
#define CMD3_RAW	 CMD_RAW(3 , msdc_rsp[RESP_R1], 0, 0,	0, 0)
#define CMD7_RAW	 CMD_RAW(7 , msdc_rsp[RESP_R1], 0, 0,	0, 0)
#define CMD8_RAW_EMMC	  CMD_RAW(8 , msdc_rsp[RESP_R1]  , 1, 0,   512, 0)	/* 0x88 -> R1 */
#define CMD9_RAW	 CMD_RAW(9 , msdc_rsp[RESP_R2]	, 0, 0,   0, 0)
#define CMD13_RAW	 CMD_RAW(13, msdc_rsp[RESP_R1]	, 0, 0,   0, 0)
#define ACMD6_RAW_EMMC	  CMD_RAW(6 , msdc_rsp[RESP_R1B]  , 0, 0,	0, 0)

/* block size always 512 */
#define CMD24_RAW	 CMD_RAW(24, msdc_rsp[RESP_R1]	, 1, 1, 512, 0)	/* single	+ write +  */

/* FUNCTION DELARATION */
extern mtk_partition mtk_msdc_partition;
extern struct msdc_host *msdc_get_host(int host_function, bool boot);
extern void msdc_pinmux(struct msdc_host *host);
extern void msdc_set_driving(struct msdc_host *host, struct msdc_hw *hw);

#endif /* _MTK_MSDC_DUMP_H */
