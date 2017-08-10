#include <mtk_msdc_dump.h>
#ifdef CONFIG_64BIT
#include <linux/soc/mediatek/mt53xx_linux.h>
#else
#include <mach/mt53xx_linux.h> /* for  define */
#endif
#include <linux/delay.h>	/* for mdely */
#include <linux/mmc/mtk_msdc_part.h>

struct msdc_dump mtk_msdc_dump;
#define msdc_dump_host	mtk_msdc_dump.host
#define MSDC_DUMP_PART	"reserved"

static int msdc_rsp[] = {
	0,	/* RESP_NONE */
	1,	/* RESP_R1 */
	2,	/* RESP_R2 */
	3,	/* RESP_R3 */
	4,	/* RESP_R4 */
	1,	/* RESP_R5 */
	1,	/* RESP_R6 */
	1,	/* RESP_R7 */
	7,	/* RESP_R1b */
};

static void msdc_dump_dump_register(void)
{
	void __iomem *base = msdc_dump_host->base;
#if 0
	int i = 0;
	unsigned int dbg_val1, dbg_val2, dbg_val3, dbg_val4;
#endif

	MSDC_DUMP_LOG("R[00]=0x%x  R[04]=0x%x  R[08]=0x%x  R[0C]=0x%x  R[10]=0x%x  R[14]=0x%x\n",
		   sdr_read32(base + 0x00), sdr_read32(base + 0x04), sdr_read32(base + 0x08),
		   sdr_read32(base + 0x0C), sdr_read32(base + 0x10), sdr_read32(base + 0x14));
	MSDC_DUMP_LOG("R[30]=0x%x  R[34]=0x%x  R[38]=0x%x  R[3C]=0x%x  R[40]=0x%x  R[44]=0x%x\n",
		   sdr_read32(base + 0x30), sdr_read32(base + 0x34), sdr_read32(base + 0x38),
		   sdr_read32(base + 0x3C), sdr_read32(base + 0x40), sdr_read32(base + 0x44));
	MSDC_DUMP_LOG("R[48]=0x%x  R[4C]=0x%x  R[50]=0x%x  R[58]=0x%x  R[5C]=0x%x  R[60]=0x%x\n",
		   sdr_read32(base + 0x48), sdr_read32(base + 0x4C), sdr_read32(base + 0x50),
		   sdr_read32(base + 0x58), sdr_read32(base + 0x5C), sdr_read32(base + 0x60));
	MSDC_DUMP_LOG("R[70]=0x%x  R[74]=0x%x  R[78]=0x%x  R[7C]=0x%x  R[80]=0x%x  R[84]=0x%x\n",
		   sdr_read32(base + 0x70), sdr_read32(base + 0x74), sdr_read32(base + 0x78),
		   sdr_read32(base + 0x7C), sdr_read32(base + 0x80), sdr_read32(base + 0x84));
	MSDC_DUMP_LOG("R[88]=0x%x  R[90]=0x%x  R[94]=0x%x  R[98]=0x%x  R[9C]=0x%x  R[A0]=0x%x\n",
		   sdr_read32(base + 0x88), sdr_read32(base + 0x90), sdr_read32(base + 0x94),
		   sdr_read32(base + 0x98), sdr_read32(base + 0x9C), sdr_read32(base + 0xA0));
	MSDC_DUMP_LOG("R[A4]=0x%x  R[A8]=0x%x  R[B0]=0x%x  R[B4]=0x%x  R[E0]=0x%x  R[E4]=0x%x\n",
		   sdr_read32(base + 0xA4), sdr_read32(base + 0xA8), sdr_read32(base + 0xB0),
		   sdr_read32(base + 0xB4), sdr_read32(base + 0xE0), sdr_read32(base + 0xE4));
	MSDC_DUMP_LOG("R[E8]=0x%x  R[EC]=0x%x  R[F0]=0x%x  R[F4]=0x%x  R[F8]=0x%x  R[100]=0x%x	R[104]=0x%x\n",
		   sdr_read32(base + 0xE8), sdr_read32(base + 0xEC), sdr_read32(base + 0xF0),
		   sdr_read32(base + 0xF4), sdr_read32(base + 0xF8), sdr_read32(base + 0x100),
		   sdr_read32(base + 0x104));
#if 0
	MSDC_DUMP_LOG("R[%x]=0x%x  R[%x]=0x%x  R[%x]=0x%x  R[%x]=0x%x  R[%x]=0x%x  R[%x]=0x%x\n",
		   OFFSET_EMMC50_PAD_DS_TUNE, sdr_read32(EMMC50_PAD_DS_TUNE),
		   OFFSET_EMMC50_PAD_CMD_TUNE, sdr_read32(EMMC50_PAD_CMD_TUNE),
		   OFFSET_EMMC50_PAD_DAT01_TUNE, sdr_read32(EMMC50_PAD_DAT01_TUNE),
		   OFFSET_EMMC50_PAD_DAT23_TUNE, sdr_read32(EMMC50_PAD_DAT23_TUNE),
		   OFFSET_EMMC50_PAD_DAT45_TUNE, sdr_read32(EMMC50_PAD_DAT45_TUNE),
		   OFFSET_EMMC50_PAD_DAT67_TUNE, sdr_read32(EMMC50_PAD_DAT67_TUNE));
	MSDC_DUMP_LOG("R[%x]=0x%x  R[%x]=0x%x  R[%x]=0x%x  R[%x]=0x%x  R[%x]=0x%x  R[%p]=0x%x\n",
		   OFFSET_EMMC50_CFG0, sdr_read32(EMMC50_CFG0), OFFSET_EMMC50_CFG1,
		   sdr_read32(EMMC50_CFG1), OFFSET_EMMC50_CFG2, sdr_read32(EMMC50_CFG2),
		   OFFSET_EMMC50_CFG3, sdr_read32(EMMC50_CFG3), OFFSET_EMMC50_CFG4,
		   sdr_read32(EMMC50_CFG4), MSDC0_GPIO_CLK_BASE,
		   sdr_read32(MSDC0_GPIO_CLK_BASE));
	MSDC_DUMP_LOG("R[%p]=0x%x  R[%p]=0x%x  R[%p]=0x%x  R[%p]=0x%x  R[%p]=0x%x  R[%p]=0x%x\n",
		   MSDC0_GPIO_CMD_BASE, sdr_read32(MSDC0_GPIO_CMD_BASE), MSDC0_GPIO_DAT_BASE,
		   sdr_read32(MSDC0_GPIO_DAT_BASE), MSDC0_GPIO_DS_BASE,
		   sdr_read32(MSDC0_GPIO_DS_BASE), MSDC0_GPIO_RST_BASE,
		   sdr_read32(MSDC0_GPIO_RST_BASE), MSDC0_GPIO_MODE0_BASE,
		   sdr_read32(MSDC0_GPIO_MODE0_BASE), MSDC0_GPIO_MODE1_BASE,
		   sdr_read32(MSDC0_GPIO_MODE1_BASE));
	MSDC_DUMP_LOG("R[%p]=0x%x\n", MSDC0_GPIO_MODE2_BASE,
		   sdr_read32(MSDC0_GPIO_MODE2_BASE));

	i = 0;
	while (i <= 25) {
		*(u32 *) (base + 0xa0) = i;
		dbg_val1 = *(u32 *) (base + 0xa4);
		*(u32 *) (base + 0xa0) = i + 1;
		dbg_val2 = *(u32 *) (base + 0xa4);
		*(u32 *) (base + 0xa0) = i + 2;
		dbg_val3 = *(u32 *) (base + 0xa4);
		*(u32 *) (base + 0xa0) = i + 3;
		dbg_val4 = *(u32 *) (base + 0xa4);
		MSDC_DUMP_LOG("R[a0]=0x%x  R[a4]=0x%x  R[a0]=0x%x  R[a4]=0x%x  R[a0]=0x%x  R[a4]=0x%x  R[a0]=0x%x  R[a4]=0x%x\n",
			   i, dbg_val1, (i + 1), dbg_val2, (i + 2), dbg_val3, (i + 3), dbg_val4);
		i += 4;
	}
#endif
}

static void msdc_dump_clksrc_on(bool on)
{
	if (on)
		sdr_clr_bits(msdc_dump_host->sclk_base, MSDC_CLK_GATE_BIT);
	else
		sdr_set_bits(msdc_dump_host->sclk_base, MSDC_CLK_GATE_BIT);
}

static int msdc_dump_set_clk(int clksrc)
{
	void __iomem *base = msdc_dump_host->base;
	unsigned int retry, cnt, mode, div;
	unsigned int clk[2][2] = {
		{8, 48000000},
		{2, 192000000}
	};

	/* hclk and sclk use the same clock source,
	 * divide the sclk to 25MHz for stable consideration
	 */
	if (clksrc == clk[0][0]) {
		div = 0;
		msdc_dump_host->sclk = clk[0][1] >> 1;
	} else if (clksrc == clk[1][0]) {
		div = 2;
		msdc_dump_host->sclk = (clk[1][1] >> 2) / div;
	} else if (clksrc == 0) {
		/* 400K for init */
		clksrc = clk[1][0];
		div = 125;
		msdc_dump_host->sclk = (clk[1][1] >> 2) / div;
	} else {
		/* default 48MHz */
		clksrc = clk[0][0];
		div = 0;
		msdc_dump_host->sclk = clk[0][1] >> 1;
	}

	mode = 0;

	sdr_set_bits(msdc_dump_host->hclk_base, MSDC_CLK_GATE_BIT);
	sdr_write32(msdc_dump_host->hclk_base, 0);
	mdelay(1);
	sdr_clr_bits(msdc_dump_host->hclk_base, MSDC_CLK_GATE_BIT);

	msdc_dump_clksrc_on(false);
	sdr_clr_bits(msdc_dump_host->sclk_base, MSDC_CLK_SEL_MASK);
	sdr_set_bits(msdc_dump_host->sclk_base, clksrc);
	mdelay(1);
	#if defined(CONFIG_ARCH_MT5885)
	sdr_set_field(MSDC_CFG, MSDC_CFG_CKMOD, mode);
	sdr_set_field(MSDC_CFG, MSDC_CFG_CKDIV, div);
	#else
	sdr_set_field(MSDC_CFG, MSDC_CFG_CKMOD, mode);
	sdr_set_field(MSDC_CFG, MSDC_CFG_CKDIV, div);
	#endif
	msdc_dump_clksrc_on(true);

	retry = 5000;
	cnt = 1000;
	msdc_retry(!(sdr_read32(MSDC_CFG) & MSDC_CFG_CKSTB), retry, cnt);

	if (retry == 0) {
		MSDC_DUMP_LOG("Wait clk(%d) stable failed!!\n", clksrc);
		return -1;
	}

	msdc_set_driving(msdc_dump_host, msdc_dump_host->hw);

	/* Init tuning settings */
	msdc_dump_host->mmc->ios.timing = MMC_TIMING_LEGACY;
	if (msdc_dump_host->load_ett_settings)
		msdc_dump_host->load_ett_settings();

	return 0;
}

/* ======================= */
static int msdc_dump_send_cmd(unsigned int cmd, unsigned int raw, unsigned int arg,
								int rsptyp, unsigned int *resp)
{
	unsigned int retry = 5000, cnt = 1000;	/* CMD_WAIT_RETRY; */
	void __iomem *base = msdc_dump_host->base;
	unsigned int error = 0;
	unsigned int intsts = 0;
	unsigned int cmdsts = MSDC_INT_CMDRDY | MSDC_INT_CMDTMO | MSDC_INT_RSPCRCERR;

	/* wait before send command */
	msdc_retry(sdc_is_cmd_busy(), retry, cnt);
	if (retry == 0) {
		error = (cmd == MMC_SEND_STATUS) ? 1 : 2;
		goto end;
	}

	sdc_send_cmd(raw, arg);
#if MTK_MSDC_DUMP_DBG
	MSDC_DUMP_LOG("cmd=0x%x, arg=0x%x\n", raw, arg);
#endif
	/* polling to check the interrupt */
	retry = 5000; /* CMD_WAIT_RETRY; */
	while ((intsts & cmdsts) == 0) {
		intsts = sdr_read32(MSDC_INT);
		retry--;
#if MTK_MSDC_DUMP_DBG
		if (retry % 1000 == 0) {
			MSDC_DUMP_LOG("int cmd=0x%x, arg=0x%x, retry=0x%x, intsts=0x%x\n", raw, arg, retry,
					intsts);
			msdc_dump_dump_register();
		}
#endif
		if (retry == 0) {
			error = 3;
			goto end;
		}
		mdelay(1);
	}

	intsts &= cmdsts;
	sdr_write32(MSDC_INT, intsts);	/* clear interrupts */

	if (intsts & MSDC_INT_CMDRDY) {
		/* get the response */
		switch (rsptyp) {
		case RESP_NONE:
			break;
		case RESP_R2:
			resp[0] = sdr_read32(SDC_RESP3);
			resp[1] = sdr_read32(SDC_RESP2);
			resp[2] = sdr_read32(SDC_RESP1);
			resp[3] = sdr_read32(SDC_RESP0);
			break;
		default:	/* Response types 1, 3, 4, 5, 6, 7(1b) */
			resp[0] = sdr_read32(SDC_RESP0);
		}
#if MTK_MSDC_DUMP_DBG
		MSDC_DUMP_LOG("msdc cmd<%d> arg<0x%x> resp<0x%x>Ready\n", cmd, arg, resp[0]);
#endif
	} else {
		error = 4;
		goto end;
	}

	if (rsptyp == RESP_R1B) {
		retry = 9999;
		while ((sdr_read32(MSDC_PS) & MSDC_PS_DAT0) != MSDC_PS_DAT0) {
			retry--;
			if (retry % 5000 == 0) {
				MSDC_DUMP_LOG("int cmd=0x%x, arg=0x%x, retry=0x%x, intsts=0x%x\n", raw,
						arg, retry, intsts);
				msdc_dump_dump_register();
			}
			if (retry == 0) {
				error = 5;
				goto end;
			}
			mdelay(1);
		}
#if MTK_MSDC_DUMP_DBG
		MSDC_DUMP_LOG("msdc cmd<%d> done \r\n", cmd);
#endif
	}

 end:
	if (error) {
		MSDC_DUMP_LOG("cmd:%d,arg:0x%x,error=%d,intsts=0x%x\n", cmd, arg, error,
				intsts);
		msdc_dump_dump_register();
	}

	return error;
}

static int msdc_dump_go_idle(void)
{
	int err = 0;
	unsigned int resp = 0;

	MSDC_DUMP_LOG("send cmd0========\n");
	err = msdc_dump_send_cmd(MMC_GO_IDLE_STATE, CMD0_RAW, CMD0_ARG, RESP_NONE, &resp);
	if (err) {
		MSDC_DUMP_LOG("cmd0: error(0x%d)\n", err);
	}

	return err;
}

static int msdc_dump_send_op_cond(unsigned int ocr, unsigned int *rocr)
{
	int err = 0, i;
	unsigned int resp = 0;

	for (i = 500; i; i--) {
		err = msdc_dump_send_cmd(MMC_SEND_OP_COND, CMD1_RAW, ocr, RESP_R3, &resp);
		if (err) {
			MSDC_DUMP_LOG("cmd1: error(0x%d)\n", err);
			break;
		}

		/* if we're just probing, do a single pass */
		if (ocr == 0)
			break;

		/* otherwise wait until reset completes */
		if (resp & MMC_CARD_BUSY) {
			break;
		}

		err = -1;

		mdelay(10);
	}

	if (rocr)
		*rocr = resp;

	if (i <= 400)
		MSDC_DUMP_LOG("cmd1: resp(0x%x), i=%d\n", resp, i);

	return err;
}

static int msdc_dump_all_send_cid(unsigned int *cid)
{
	int err = 0;
	unsigned int resp[4] = {0};

	err = msdc_dump_send_cmd(MMC_ALL_SEND_CID, CMD2_RAW, 0, RESP_R2, resp);
	if (err)
		MSDC_DUMP_LOG("cmd2: error(0x%d)\n", err);

#if MTK_MSDC_DUMP_DBG
	MSDC_DUMP_LOG("resp: 0x%x 0x%x 0x%x 0x%x\n", resp[0], resp[1], resp[2], resp[3]);
#endif
	memcpy(cid, resp, sizeof(u32) * 4);

	return 0;
}

static int msdc_dump_set_relative_addr(void)
{
	int err;
	unsigned int resp;

	err = msdc_dump_send_cmd(MMC_SET_RELATIVE_ADDR, CMD3_RAW, 1 << 16, RESP_R1, &resp);
	if (err) {
		MSDC_DUMP_LOG("cmd3: error(0x%d)\n", err);
	}

	return err;
}

static int msdc_dump_send_csd(unsigned int *csd)
{
	int err;
	unsigned int resp[4] = { 0 };

	err = msdc_dump_send_cmd(MMC_SEND_CSD, CMD9_RAW, 1 << 16, RESP_R2, resp);
	if (err) {
		MSDC_DUMP_LOG("cmd9: error(0x%d)\n", err);
	}

	memcpy(csd, resp, sizeof(u32) * 4);

	return err;
}

static int msdc_dump_select_card(void)
{
	int err;
	unsigned int resp;

	err = msdc_dump_send_cmd(MMC_SELECT_CARD, CMD7_RAW, 1 << 16, RESP_R1, &resp);
	if (err)
		MSDC_DUMP_LOG("cmd7: select card error(0x%d)\n", err);

	return 0;
}

static void msdc_dump_set_blknum(unsigned int blknum)
{
	void __iomem *base = msdc_dump_host->base;
	sdr_write32(SDC_BLK_NUM, blknum);
}

static void msdc_dump_set_timeout(u32 ns, u32 clks)
{
	void __iomem *base = msdc_dump_host->base;
	u32 timeout, clk_ns;

	clk_ns = 1000000000UL / msdc_dump_host->sclk;
	timeout = ns / clk_ns + clks;
	timeout = timeout >> 20;	/* in 2^20 sclk cycle unit */
	timeout = timeout > 1 ? timeout - 1 : 1;
	timeout = timeout > 255 ? 255 : timeout;

#if MTK_MSDC_DUMP_DBG
	MSDC_DUMP_LOG("sclk=%d, data timeout is %d\n", msdc_dump_host->sclk, timeout);
#endif
	sdr_set_field(SDC_CFG, SDC_CFG_DTOC, timeout);
}

static int msdc_dump_pio_read(unsigned int *ptr, unsigned int size)
{
	void __iomem *base = msdc_dump_host->base;
	unsigned int left = size;
	unsigned int status = 0;
	unsigned char *u8ptr;
	int l_count = 0;
	int err = 0;

	while (1) {
		status = sdr_read32(MSDC_INT);
		mb();
		sdr_write32(MSDC_INT, status);

		if (status & MSDC_INT_DATCRCERR) {
			MSDC_DUMP_LOG("DAT CRC error (0x%x), Left DAT: %d bytes\n",
						status, left);
			err = -5;
			break;
		} else if (status & MSDC_INT_DATTMO) {
			MSDC_DUMP_LOG("DAT TMO error (0x%x), Left DAT: %d bytes\n",
					status, left);
			err = -110;
			break;
		} else if (status & MSDC_INT_XFER_COMPL)
			break;

		if (left == 0)
			continue;

		while (left) {
		#if MTK_MSDC_DUMP_DBG
			MSDC_DUMP_LOG("left(%d) FIFO(%d) intr=0x%x\n", left, msdc_rxfifocnt(), sdr_read32(MSDC_INT));
		#endif
			if ((left >= MSDC_FIFO_THD) && (msdc_rxfifocnt() >= MSDC_FIFO_THD)) {
				int count = MSDC_FIFO_THD >> 2;
				do {
					*ptr++ = msdc_fifo_read32();
				} while (--count);
				left -= MSDC_FIFO_THD;
			} else if ((left < MSDC_FIFO_THD) && msdc_rxfifocnt() >= left) {
				while (left > 3) {
					*ptr++ = msdc_fifo_read32();
					left -= 4;
				}

				u8ptr = (u8 *) ptr;
				while (left) {
					*u8ptr++ = msdc_fifo_read8();
					left--;
				}
			} else {
				status = sdr_read32(MSDC_INT);

				if ((status & MSDC_INT_DATCRCERR) || (status & MSDC_INT_DATTMO)) {
					if (status & MSDC_INT_DATCRCERR) {
						MSDC_DUMP_LOG("DAT CRC error (0x%x), Left DAT: %d bytes\n",
								status, left);
						err = -5;
					}
					if (status & MSDC_INT_DATTMO) {
						MSDC_DUMP_LOG("DAT TMO error (0x%x), Left DAT: %d bytes\n",
								status, left);
						err = -110;
					}

				#if MTK_MSDC_DUMP_DBG
					msdc_dump_dump_register();
				#endif

					sdr_write32(MSDC_INT, status);
					msdc_reset_hw();

					return err;
				}
			#if MTK_MSDC_DUMP_DBG
				else if (status != 0) {
					MSDC_DUMP_LOG("interrupt = 0x%x\n", status);
				}
			#endif
			}

			/* timeout monitor */
			l_count++;
			if (l_count >= 0x10000) {
				l_count = 0;
				MSDC_DUMP_LOG("loop cnt(%d) size= %d, left= %d.\r\n", l_count, size, left);
				msdc_dump_dump_register();
			}
		}
	}

	return err;
}

static u8 ext_csd[512];
static int msdc_dump_read_ext_csd(void)
{
	int err = 0;
	unsigned int resp;
	void __iomem *base = msdc_dump_host->base;

	memset(&ext_csd[0], 0, 512);
	msdc_clr_fifo();
	msdc_dump_set_blknum(1);
	msdc_dump_set_timeout(100000000, 0);

	err = msdc_dump_send_cmd(MMC_SEND_EXT_CSD, CMD8_RAW_EMMC, 0, RESP_R1, &resp);
	if (err) {
		MSDC_DUMP_LOG("cmd8: send cmd to read ext csd error(0x%d)\n", err);
		goto out;
	}

	err = msdc_dump_pio_read((unsigned int *)(&ext_csd[0]), 512);
	if (err) {
		MSDC_DUMP_LOG("pio read ext csd error(0x%d)\n", err);
		goto out;
	}

out:
	return err;
}

static int msdc_dump_switch_bus(void)
{
	unsigned int resp;
	void __iomem *base = msdc_dump_host->base;

	sdr_set_field(SDC_CFG, SDC_CFG_BUSWIDTH, 1);	/* 4 bits mode */
	return msdc_dump_send_cmd(MMC_SWITCH, ACMD6_RAW_EMMC, ACMD6_ARG_EMMC, RESP_R1B, &resp);
}

/* ======================= */

static int msdc_dump_init_card(void)
{
	int err = 0;
	unsigned int ocr;
	unsigned int cid[4];
	unsigned int raw_csd[4];

	/*=================== begin to init emmc card =======================*/
	/* GO idle state */
	msdc_dump_go_idle();

	/* power up host */
	err = msdc_dump_send_op_cond(0, &ocr);

	/*
	 * Sanity check the voltages that the card claims to
	 * support.
	 */
	if (ocr & 0x7F) {
#if MTK_MSDC_DUMP_DBG
		MSDC_DUMP_LOG("msdc0: card claims to support voltages(0x%x) "
				"below the defined range. These will be ignored.\n", ocr);
#endif
		ocr &= ~0x7F;
	}

	/*
	 * Can we support the voltage of the card?
	 */
	if (!ocr) {
		MSDC_DUMP_LOG("msdc0: card voltage not support\n");
		err = -1;
		goto err;
	}

	/*
	 * Since we're changing the OCR value, we seem to
	 * need to tell some cards to go back to the idle
	 * state.  We wait 1ms to give cards time to
	 * respond.
	 * mmc_go_idle is needed for eMMC that are asleep
	 */
	msdc_dump_go_idle();

	/* The extra bit indicates that we support high capacity */
	err = msdc_dump_send_op_cond(0x40FF8080, &ocr);
	if (err)
		goto err;

	err = msdc_dump_all_send_cid(cid);
	if (err)
		goto err;

	/*
	 * For native busses:  set card RCA and quit open drain mode.
	 */
	err = msdc_dump_set_relative_addr();
	if (err)
		goto err;

	/*
	 * Fetch CSD from card.
	 */
	err = msdc_dump_send_csd(raw_csd);
	if (err)
		goto err;

#if 0
	err = simp_mmc_decode_csd(card);
	if (err)
		goto err;
#endif

	err = msdc_dump_select_card();
	if (err)
		goto err;

	err = msdc_dump_read_ext_csd();
	if (err)
		goto err;

#if 0
	simp_mmc_decode_ext_csd(card);
#endif

	err = msdc_dump_switch_bus();
	if (err)
		goto err;

	err = msdc_dump_set_clk(2); /* ENETPLL(11) => 200MHz */
	if (err != 0)
		err = msdc_dump_set_clk(8); /* SAWLESSPLL(8) => 48MHz */

	/*=================== end mmc card init =============================*/
err:
	if (err != 0)
		MSDC_DUMP_LOG("init eMMC failed\n");
	else
		MSDC_DUMP_LOG("init eMMC success\n");

	return err;
}

static void msdc_dump_dma_stop(void)
{
	void __iomem *base = msdc_dump_host->base;
	int retry = 5000;
	int count = 1000;

	sdr_set_field(MSDC_DMA_CTRL, MSDC_DMA_CTRL_STOP, 1);
	msdc_retry((sdr_read32(MSDC_DMA_CFG) & MSDC_DMA_CFG_STS), retry, count);
	if (retry == 0) {
		MSDC_DUMP_LOG("###### Failed to stop DMA! please check it here ######\n");
	}
}

static void msdc_dump_hw_reset(void)
{
	void __iomem *base = msdc_dump_host->base;

	/* check emmc card support HW Rst_n yes or not is the good way.
	 * but if the card not support it , here just failed.
	 * if the card support it, Rst_n function enabled under flashtool,
	 * 1ms pluse to trigger emmc enter pre-idle state */
	sdr_set_bits(EMMC_IOCON, EMMC_IOCON_BOOTRST);
	mdelay(1);
	sdr_clr_bits(EMMC_IOCON, EMMC_IOCON_BOOTRST);

	/* clock is need after Rst_n pull high, and the card need
	 * clock to calculate time for tRSCA, tRSTH */
	sdr_set_bits(MSDC_CFG, MSDC_CFG_CKPDN);
	mdelay(1);

	/* not to close, enable clock free run under mt_dump */
	/* sdr_clr_bits(MSDC_CFG, MSDC_CFG_CKPDNT); */
}

static void msdc_dump_init_host(void)
{
	unsigned int __iomem *base;

	memset(&mtk_msdc_dump, 0, sizeof(struct msdc_dump));
	msdc_dump_host = msdc_get_host(MSDC_EMMC, MSDC_BOOT_EN);

	BUG_ON(msdc_dump_host == NULL);

	base = msdc_dump_host->base;

	msdc_pinmux(msdc_dump_host);

	/* Reset eMMC to PRE-IDLE	state */
	msdc_dump_hw_reset();

	/* Register/IO settings */
	/* set to SD/MMC mode, the first step while operation msdc */
	sdr_set_field(MSDC_CFG, MSDC_CFG_MODE, 1);	/* MSDC_SDMMC */

	/* reset controller */
	msdc_reset();

	/* stop DMA */
	msdc_dump_dma_stop();

	/* clear FIFO */
	msdc_clr_fifo();

	/* Disable & Clear all interrupts */
	msdc_clr_int();
	sdr_write32(MSDC_INTEN, 0);

	/* reset tuning parameter */
	sdr_write32(MSDC_PAD_TUNE0, 0x00000000);
	sdr_write32(MSDC_DAT_RDDLY0, 0x00000000);
	sdr_write32(MSDC_DAT_RDDLY1, 0x00000000);
	sdr_write32(MSDC_IOCON, 0x00000000);
	sdr_write32(MSDC_PATCH_BIT0, 0x003C000F);

	/* PIO mode */
	sdr_set_bits(MSDC_CFG, MSDC_CFG_PIO);

	/* 1bit bus width */
	sdr_set_field(SDC_CFG, SDC_CFG_BUSWIDTH, 0);

	/* sdio + inswkup */
	sdr_clr_bits(SDC_CFG, SDC_CFG_SDIO);
	sdr_clr_bits(SDC_CFG, SDC_CFG_INSWKUP);

	/* set PIO FIFO size to 4bytes(2/3) */
	sdr_set_field(MSDC_IOCON, MSDC_IOCON_RISCSZ, 0x2);

	/* Configure SCLK to 400K */
#ifdef MSDC_USE_OF
	/* The CLK base SHOULD be OK */
#else
	msdc_dump_host->sclk_base = ioremap((phys_addr_t)MSDC_CLK_S_REG1, 0x4);
	msdc_dump_host->hclk_base = ioremap((phys_addr_t)MSDC_CLK_H_REG1, 0x4);
#endif

	msdc_dump_set_clk(0);
}

static int msdc_dump_init_emmc(void)
{
	int ret;

	msdc_dump_init_host();
	MSDC_DUMP_ERR_EXIT(msdc_dump_init_card(), ret);

	return ret;
}

static int msdc_dump_pio_write(unsigned int *ptr, unsigned int size)
{
	void __iomem *base = msdc_dump_host->base;
	unsigned int left = size;
	unsigned int status = 0;
	unsigned char *u8ptr;
	int l_count = 0;
	int err = 0;

	while (1) {
		status = sdr_read32(MSDC_INT);
		sdr_write32(MSDC_INT, status);
		if (status & MSDC_INT_DATCRCERR) {
			MSDC_DUMP_LOG("DAT CRC error (0x%x), Left DAT: %d bytes\n",
					status, left);
			err = -5;
			break;
		} else if (status & MSDC_INT_DATTMO) {
			MSDC_DUMP_LOG("DAT TMO error (0x%x), Left DAT: %d bytes\n",
					status, left);
			err = -110;
			break;
		} else if (status & MSDC_INT_XFER_COMPL)
			break;

		if (left == 0)
			continue;
		if ((left >= MSDC_FIFO_SZ) && (msdc_txfifocnt() == 0)) {
			int count = MSDC_FIFO_SZ >> 2;
			do {
				msdc_fifo_write32(*ptr);
				ptr++;
			} while (--count);
			left -= MSDC_FIFO_SZ;
		} else if (left < MSDC_FIFO_SZ && msdc_txfifocnt() == 0) {
			while (left > 3) {
				msdc_fifo_write32(*ptr);
				ptr++;
				left -= 4;
			}

			u8ptr = (u8 *) ptr;
			while (left) {
				msdc_fifo_write8(*u8ptr);
				u8ptr++;
				left--;
			}
		} else {
			status = sdr_read32(MSDC_INT);

			if ((status & MSDC_INT_DATCRCERR) || (status & MSDC_INT_DATTMO)) {

				if (status & MSDC_INT_DATCRCERR) {
					MSDC_DUMP_LOG("DAT CRC error (0x%x), Left DAT: %d bytes\n",
							status, left);
					err = -5;
				}
				if (status & MSDC_INT_DATTMO) {
					MSDC_DUMP_LOG("DAT TMO error (0x%x), Left DAT: %d bytes\n",
							status, left);
					err = -110;
				}

				msdc_dump_dump_register();

				sdr_write32(MSDC_INT, status);
				msdc_reset_hw();
				return err;
			}
		}

		l_count++;
		if (l_count > 500) {
			l_count = 0;
			MSDC_DUMP_LOG("size= %d, left= %d.\r\n", size, left);
			msdc_dump_dump_register();
		}
	}

	return err;
}

static int msdc_dump_single_write(unsigned int lba, void *ptr)
{
	unsigned int resp = 0;
	unsigned int err = 0;
	void __iomem *base = msdc_dump_host->base;

	msdc_dump_set_blknum(1);

	/* send command */
	err = msdc_dump_send_cmd(MMC_WRITE_BLOCK, CMD24_RAW, lba, RESP_R1, &resp);
	if (err) {
		MSDC_DUMP_LOG("cmd24: error(%d)\n", err);
		goto error;
	}

	/* write the data to FIFO */
	err = msdc_dump_pio_write((unsigned int *)ptr, 512);
	if (err) {
		MSDC_DUMP_LOG("write data: error(%d)\n", err);
		goto error;
	}

	/* make sure contents in fifo flushed to device */
	BUG_ON(msdc_txfifocnt());

error:
	return err;
}

static unsigned int msdc_dump_get_status(unsigned int *status)
{
	unsigned int resp = 0;
	unsigned int err = 0;

	err = msdc_dump_send_cmd(MMC_SEND_STATUS, CMD13_RAW, 1 << 16, RESP_R1, &resp);
	if (err) {
		MSDC_DUMP_LOG("cmd13: error(0x%d) rsp(0x%x)\n", err, resp);
		return err;
	}

	*status = resp;

	return err;
}

static int msdc_dump_write(void *ptr, sector_t lba, sector_t blknr)
{
	int ret;
	unsigned int i;
	sector_t cur_lba;
	sector_t total_blks;
	unsigned char *cur_ptr;
	unsigned int status = 0;
	int polling;
	int partno;

	for (partno = 1; partno <= mtk_msdc_partition.nparts; partno++) {
		if (strstr(mtk_msdc_partition.parts[partno].name, MSDC_DUMP_PART)) {
			cur_lba = mtk_msdc_partition.parts[partno].start_sect + lba;
			total_blks = mtk_msdc_partition.parts[partno].nr_sects;
			break;
		}

		if (partno == mtk_msdc_partition.nparts) {
			MSDC_DUMP_LOG("ERROR: cannot find partition: %s\n", MSDC_DUMP_PART);
			return -1;
		}
	}

	BUG_ON((lba > total_blks - 2) || (lba + blknr > total_blks - 1));

	cur_ptr = ptr;
	/* Write data */
	for (i = 0; i < blknr; i++) {
		polling = MAX_POLLING_STATUS;

#if MTK_MSDC_DUMP_DBG
		MSDC_DUMP_LOG("cur_lba = 0x%lx\n", cur_lba);
#endif
		ret = msdc_dump_single_write(cur_lba, cur_ptr);
		cur_lba++;
		cur_ptr += 512;
		do {
			ret = msdc_dump_get_status(&status);
		} while (R1_CURRENT_STATE(status) == 7 && polling-- && (ret == 0));

		if (polling == 0 || ret != 0) {
			MSDC_DUMP_LOG("ERROR: current eMMC status=0x%x\n", status);
			ret = -1;
			break;
		}
	}

	if (ret != 0)
		MSDC_DUMP_LOG("write eMMC failed, wrote %ld Bytes data\n",
					(unsigned long)cur_ptr - (unsigned long)ptr);
	else
		MSDC_DUMP_LOG("write eMMC success\n");

	return ret;
}

int emmcoops_write(void *ptr, sector_t lba, sector_t blknr)
{
	int ret;

	/* Initialize host & card then entering TRANS mode */
	MSDC_DUMP_ERR_EXIT(msdc_dump_init_emmc(), ret);
	MSDC_DUMP_ERR_EXIT(msdc_dump_write(ptr, lba, blknr), ret);

	return ret;
}

int emmcoops_read(void *ptr, uint64_t offset, uint64_t size)
{
	int ret;

	//MSDC_DUMP_ERR_EXIT(msdc_partread_byname(MSDC_DUMP_PART, offset, size, ptr), ret);
	MSDC_DUMP_ERR_EXIT(msdc_partread(9, offset, size, ptr), ret);

	return ret;
}

static int __init msdc_dump_init(void)
{
	/* Do nothing, but module should be built-in */
	return 0;
}

static void __exit msdc_dump_exit(void)
{
}

module_init(msdc_dump_init);
module_exit(msdc_dump_exit);

