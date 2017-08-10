#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/spinlock.h>
#include <linux/timer.h>
#include <linux/ioport.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/blkdev.h>
#include <linux/slab.h>
#include <linux/wakelock.h>
#include <linux/mmc/host.h>
#include <linux/mmc/card.h>
#include <linux/mmc/core.h>
#include <linux/mmc/mmc.h>
#include <linux/mmc/sd.h>
#include <linux/mmc/sdio.h>
#include <linux/dma-mapping.h>
#include <linux/bootmem.h>  /* for max_low_pfn */
#include <linux/irq.h>
#ifdef CONFIG_64BIT
#include <linux/soc/mediatek/mtk_gpio.h>
#include <linux/soc/mediatek/dma.h>
#include <linux/soc/mediatek/irqs.h>
#else
#include <mach/mtk_gpio.h>
#include <mach/dma.h>
#include <mach/irqs.h>
#endif

#include <linux/proc_fs.h>
#include <mtk_msdc_dbg.h>
#include <mtk_msdc.h>
#include <mtk_msdc_ett_dat.h>

#ifdef CONFIG_MTK_USE_NEW_TUNE
#include <mtk_msdc_autok.h>
#endif

#ifdef CC_MTD_ENCRYPT_SUPPORT
#include <crypto/mtk_crypto.h>
#endif

#ifdef MSDC_USE_OF
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#endif

#define DRV_NAME			"mtk-msdc"

static unsigned int msdc_do_command(struct msdc_host   *host,
									  struct mmc_command *cmd,
									  int				  tune,
									  unsigned long		  timeout);
static int msdc_tune_cmdrsp(struct msdc_host *host);
static int msdc_get_card_status(struct mmc_host *mmc, struct msdc_host *host, u32 *status);
static inline void msdc_clksrc_onoff(struct msdc_host *host, u32 on);
static int msdc_card_exist(void *data);
static int msdc_clk_stable(struct msdc_host *host,u32 mode, u32 div, u32 hs400_src);
extern u32 __mmc_sd_num_wr_blocks(struct mmc_card* card);

extern int autok_execute_tuning(struct msdc_host *host, u8 *res);
extern int hs200_execute_tuning(struct msdc_host *host, u8 *res);
extern int hs400_execute_tuning(struct msdc_host *host, u8 *res);
extern void msdc_kernel_fifo_path_sel_init(struct msdc_host *host);
/* the global variable related to clcok */
u32 msdcClk[7][12] = {
	{MSDC_CLK_TARGET},
	{MSDC_CLK_SRC_VAL},
	{MSDC_CLK_MODE_VAL},
	{MSDC_CLK_DIV_VAL},
	{MSDC_CLK_DRV_VAL},
	{MSDC_CMD_DRV_VAL},
	{MSDC_DAT_DRV_VAL}
};

u32 erase_start = 0;
u32 erase_end = 0;

int MSDC_Gpio[MSDC_GPIO_MAX_NUMBERS]= {0} ;
unsigned int MSDC_EMMClog = 0;

#define MSDC_AUTOK_MAX_NUMBERS  17
#define MSDC_CMD_LINE_PARSE_DONE   (1<<0)
#define MSDC_AUTOK_EXEC_DONE       (1<<1)
int MSDC_Autok[MSDC_AUTOK_MAX_NUMBERS]= {0} ;
static int volatile g_autok_cmdline_parsed = 0;

#ifdef MSDC_ONLY_USE_ONE_CLKSRC
#ifdef CONFIG_ARCH_MT5891
unsigned int msdc_sclk_freq[][16] = {
	{
		24000000, 216000000, 192000000, 384000000,
		162000000, 144000000, 300000000, 108000000,
		48000000, 80000000, 13000000, 200000000
	},
	{
		24000000, 216000000, 192000000, 300000000,
		162000000, 144000000, 120000000, 108000000,
		48000000, 80000000, 13000000, 200000000
	}
};
#endif
#endif

struct msdc_hw msdc1_hw = {
	.sclk_src		= SD_SCLK_DEFAULT,
	.hclk_src		= SD_HCLK_DEFAULT,
	.sclk_base		= MSDC_CLK_S_REG0,
	.hclk_base		= MSDC_CLK_H_REG0,
	.cmd_edge		= MSDC_SMPL_FALLING,
	.rdata_edge		= MSDC_SMPL_FALLING,
	.wdata_edge		= MSDC_SMPL_FALLING,
	.clk_drv		= 0,
	.cmd_drv		= 0,
	.dat_drv		= 0,
	.clk_drv_sd_18	= 3,
	.cmd_drv_sd_18	= 3,
	.dat_drv_sd_18	= 3,
	.data_pins		= 4,
	.flags			= MSDC_SDCARD_FLAG,
	.dat0rddly		= 0,
	.dat1rddly		= 0,
	.dat2rddly		= 0,
	.dat3rddly		= 0,
	.dat4rddly		= 0,
	.dat5rddly		= 0,
	.dat6rddly		= 0,
	.dat7rddly		= 0,
	.datwrddly		= 0,
	.cmdrrddly		= 0,
	.cmdrddly		= 0,
	.host_function	= MSDC_SD,
	.boot			= 0,
};

struct msdc_hw msdc0_hw = {
	.sclk_src		= EMMC_SCLK_DEFAULT,
	.hclk_src		= EMMC_HCLK_DEFAULT,
	.sclk_base		= MSDC_CLK_S_REG1,
	.hclk_base		= MSDC_CLK_H_REG1,
	.cmd_edge		= MSDC_SMPL_FALLING,
	.rdata_edge		= MSDC_SMPL_FALLING,
	.wdata_edge		= MSDC_SMPL_FALLING,
	.clk_drv		= 0,
	.cmd_drv		= 0,
	.dat_drv		= 0,
#ifdef CC_EMMC_4BIT
	.data_pins		= 4,
#else
	.data_pins		= 8,
#endif
	.flags			= MSDC_EMMC_FLAG,
	.dat0rddly		= 0,
	.dat1rddly		= 0,
	.dat2rddly		= 0,
	.dat3rddly		= 0,
	.dat4rddly		= 0,
	.dat5rddly		= 0,
	.dat6rddly		= 0,
	.dat7rddly		= 0,
	.datwrddly		= 0,
	.cmdrrddly		= 0,
	.cmdrddly		= 0,
	.host_function	= MSDC_EMMC,
	.boot			= MSDC_BOOT_EN,
};

static u64 msdc_dma_mask;
#ifndef MSDC_USE_OF
#ifdef CONFIG_MTKEMMC_BOOT
static struct resource mt_resource_msdc0[] = {
	{
		.start	= MSDC_1_BASE,
#if defined(CONFIG_ARCH_MT5883)
		.end	= MSDC_1_BASE + 0x118,
#else
		.end	= MSDC_1_BASE + 0x108,
#endif
		.flags	= IORESOURCE_MEM,
	},
	{
#if defined(CONFIG_ARCH_MT5883)
		.start	= VECTOR_MSDC,
#else
		.start	= VECTOR_MSDC2,
#endif
		.flags	= IORESOURCE_IRQ,
	},
};
#endif

#ifdef CONFIG_MTKSDMMC_USE
static struct resource mt_resource_msdc1[] = {
	{
		.start	= MSDC_0_BASE,
#if defined(CONFIG_ARCH_MT5883)
		.end	= MSDC_0_BASE + 0x118,
#else
		.end	= MSDC_0_BASE + 0x108,
#endif
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= VECTOR_MSDC,
		.flags	= IORESOURCE_IRQ,
	},
};
#endif

static struct platform_device mt_device_msdc[] = {
#ifdef CONFIG_MTKEMMC_BOOT
	{
		.name			= DRV_NAME,
		.id				= 0,
		.num_resources	= ARRAY_SIZE(mt_resource_msdc0),
		.resource		= mt_resource_msdc0,
		.dev = {
			.platform_data = &msdc0_hw,
			.coherent_dma_mask = MTK_MSDC_DMA_MASK,
#ifdef CC_SUPPORT_CHANNEL_C
			.dma_mask = &msdc_dma_mask;
#else
			.dma_mask = &(mt_device_msdc[0].dev.coherent_dma_mask),
#endif
		},
	},
#endif
#ifdef CONFIG_MTKSDMMC_USE
	{
		.name			= DRV_NAME,
		.id				= 1,
		.num_resources	= ARRAY_SIZE(mt_resource_msdc1),
		.resource		= mt_resource_msdc1,
		.dev = {
			.platform_data = &msdc1_hw,
			.coherent_dma_mask = MTK_MSDC_DMA_MASK,
#ifdef CC_SUPPORT_CHANNEL_C
			.dma_mask = &msdc_dma_mask;
#else
			.dma_mask = &(mt_device_msdc[1].dev.coherent_dma_mask),
#endif
		},
	},
#endif
};
#endif

struct msdc_host *mtk_msdc_host[] = {NULL, NULL};

transfer_mode msdc_latest_transfer_mode[HOST_MAX_NUM] = {
	TRAN_MOD_NUM,
	TRAN_MOD_NUM
};

operation_type msdc_latest_operation_type[HOST_MAX_NUM] = {
	OPER_TYPE_NUM,
	OPER_TYPE_NUM
};

struct dma_addr msdc_latest_dma_address[MAX_BD_PER_GPD];

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


static int msdc_get_data(u8* dst,struct mmc_data *data)
{
	int left;
	u8* ptr;
	struct scatterlist *sg = data->sg;
	int num = data->sg_len;

	while (num) {
		left = sg_dma_len(sg);
		ptr = (u8*)sg_virt(sg);
		memcpy(dst,ptr,left);
		sg = sg_next(sg);
		dst+=left;
		num--;
	}

	return 0;
}
#ifdef CONFIG_MTKEMMC_BOOT
u8 ext_csd[512];
int offset = 0;
char partition_access = 0;

static u32 msdc_get_other_capacity(void) //to check?
{
	u32 device_other_capacity = 0;

	device_other_capacity = ext_csd[EXT_CSD_BOOT_MULT]* 128 * 1024
							+ ext_csd[EXT_CSD_BOOT_MULT] * 128 * 1024
							+ ext_csd[EXT_CSD_RPMB_MULT] * 128 * 1024
							+ ext_csd[EXT_CSD_GP_SIZE_MULT + 0]
							+ ext_csd[EXT_CSD_GP_SIZE_MULT + 1] * 256
							+ ext_csd[EXT_CSD_GP_SIZE_MULT + 2] * 256 * 256
							+ ext_csd[EXT_CSD_GP_SIZE_MULT + 3]
							+ ext_csd[EXT_CSD_GP_SIZE_MULT + 4] * 256
							+ ext_csd[EXT_CSD_GP_SIZE_MULT + 5] * 256 * 256
							+ ext_csd[EXT_CSD_GP_SIZE_MULT + 6]
							+ ext_csd[EXT_CSD_GP_SIZE_MULT + 7] * 256
							+ ext_csd[EXT_CSD_GP_SIZE_MULT + 8] * 256 * 256
							+ ext_csd[EXT_CSD_GP_SIZE_MULT + 9]
							+ ext_csd[EXT_CSD_GP_SIZE_MULT + 10] * 256
							+ ext_csd[EXT_CSD_GP_SIZE_MULT + 11] * 256 * 256;

	return device_other_capacity;
}
#endif

static void msdc_dump_dbg_register(struct msdc_host *host)
{
	void __iomem *base = host->base;
	u32 i;

	for (i = 0; i <= 0x27; i++) {
		sdr_write32(MSDC_DBG_SEL, i);
		ERR_MSG("SEL:r[%x]=0x%x", OFFSET_MSDC_DBG_SEL, i);
		ERR_MSG("\tOUT:r[%x]=0x%x\n", OFFSET_MSDC_DBG_OUT,
			 sdr_read32(MSDC_DBG_OUT));
	}

	sdr_write32(MSDC_DBG_SEL, 0);
}

static void msdc_dump_register(struct msdc_host *host)
{
	u32 __iomem *base = host->base;

#if defined(CONFIG_ARCH_MT5883)
	u32 i = 0;
	for (; i < 0x118; i += 4)
		INIT_MSG("Reg[0x%02x] %08X\n", i, sdr_read32(base + i));
#else
#define MSDC_REG_DUMP_LOG(reg) \
	ERR_MSG("Reg[%.3x] "#reg"\t= 0x%.8x\n", \
			((unsigned int)((u32 __iomem*)reg - base)) << 2, sdr_read32(reg));
	MSDC_REG_DUMP_LOG(MSDC_CFG);
	MSDC_REG_DUMP_LOG(MSDC_IOCON);
	MSDC_REG_DUMP_LOG(MSDC_PS);
	MSDC_REG_DUMP_LOG(MSDC_INT);
	MSDC_REG_DUMP_LOG(MSDC_INTEN);
	MSDC_REG_DUMP_LOG(SDC_CFG);
	MSDC_REG_DUMP_LOG(SDC_CMD);
	MSDC_REG_DUMP_LOG(SDC_ARG);
	MSDC_REG_DUMP_LOG(SDC_STS);
	MSDC_REG_DUMP_LOG(SDC_RESP0);
	MSDC_REG_DUMP_LOG(SDC_RESP1);
	MSDC_REG_DUMP_LOG(SDC_RESP2);
	MSDC_REG_DUMP_LOG(SDC_RESP3);
	MSDC_REG_DUMP_LOG(SDC_BLK_NUM);
	MSDC_REG_DUMP_LOG(SDC_VOL_CHG);
	MSDC_REG_DUMP_LOG(SDC_CSTS);
	MSDC_REG_DUMP_LOG(SDC_CSTS_EN);
	MSDC_REG_DUMP_LOG(SDC_DCRC_STS);
	MSDC_REG_DUMP_LOG(SDC_CMD_STS);
	MSDC_REG_DUMP_LOG(EMMC_CFG0);
	MSDC_REG_DUMP_LOG(EMMC_CFG1);
	MSDC_REG_DUMP_LOG(EMMC_STS);
	MSDC_REG_DUMP_LOG(EMMC_IOCON);
	MSDC_REG_DUMP_LOG(SDC_ACMD_RESP);
	MSDC_REG_DUMP_LOG(SDC_ACMD19_TRG);
	MSDC_REG_DUMP_LOG(SDC_ACMD19_STS);
	MSDC_REG_DUMP_LOG(MSDC_DMA_SA_H4B);
	MSDC_REG_DUMP_LOG(MSDC_DMA_SA);
	MSDC_REG_DUMP_LOG(MSDC_DMA_CA);
	MSDC_REG_DUMP_LOG(MSDC_DMA_CTRL);
	MSDC_REG_DUMP_LOG(MSDC_DMA_CFG);
	MSDC_REG_DUMP_LOG(MSDC_DBG_SEL);
	MSDC_REG_DUMP_LOG(MSDC_DBG_OUT);
	MSDC_REG_DUMP_LOG(MSDC_DMA_LEN);
	MSDC_REG_DUMP_LOG(MSDC_PATCH_BIT0);
	MSDC_REG_DUMP_LOG(MSDC_PATCH_BIT1);
	MSDC_REG_DUMP_LOG(MSDC_PATCH_BIT2);
	MSDC_REG_DUMP_LOG(DAT0_TUNE_CRC);
	MSDC_REG_DUMP_LOG(DAT1_TUNE_CRC);
	MSDC_REG_DUMP_LOG(DAT2_TUNE_CRC);
	MSDC_REG_DUMP_LOG(DAT3_TUNE_CRC);
	MSDC_REG_DUMP_LOG(CMD_TUNE_CRC);
	MSDC_REG_DUMP_LOG(SDIO_TUNE_WIND);
	MSDC_REG_DUMP_LOG(MSDC_PAD_CTL0);
	MSDC_REG_DUMP_LOG(MSDC_PAD_CTL1);
	MSDC_REG_DUMP_LOG(MSDC_PAD_CTL2);
	MSDC_REG_DUMP_LOG(MSDC_PAD_TUNE0);
#ifdef OFFSET_MSDC_PAD_TUNE1
	MSDC_REG_DUMP_LOG(MSDC_PAD_TUNE1);
#endif
	MSDC_REG_DUMP_LOG(MSDC_DAT_RDDLY0);
	MSDC_REG_DUMP_LOG(MSDC_DAT_RDDLY1);
#ifdef OFFSET_MSDC_DAT_RDDLY2
	MSDC_REG_DUMP_LOG(MSDC_DAT_RDDLY2);
#endif
#ifdef OFFSET_MSDC_DAT_RDDLY3
	MSDC_REG_DUMP_LOG(MSDC_DAT_RDDLY3);
#endif
	MSDC_REG_DUMP_LOG(MSDC_HW_DBG);
	MSDC_REG_DUMP_LOG(MSDC_VERSION);
	MSDC_REG_DUMP_LOG(MSDC_ECO_VER);
#if defined(CONFIG_EMMC_HS400_SUPPORT) || defined(CONFIG_EMMC_HS200_SUPPORT)
	MSDC_REG_DUMP_LOG(EMMC50_PAD_CTL0);
	MSDC_REG_DUMP_LOG(EMMC50_PAD_DS_CTL0);
	MSDC_REG_DUMP_LOG(EMMC50_PAD_DS_TUNE);
	MSDC_REG_DUMP_LOG(EMMC50_PAD_CMD_TUNE);
	MSDC_REG_DUMP_LOG(EMMC50_PAD_DAT01_TUNE);
	MSDC_REG_DUMP_LOG(EMMC50_PAD_DAT23_TUNE);
	MSDC_REG_DUMP_LOG(EMMC50_PAD_DAT45_TUNE);
	MSDC_REG_DUMP_LOG(EMMC50_PAD_DAT67_TUNE);
	MSDC_REG_DUMP_LOG(EMMC51_CFG0);
	MSDC_REG_DUMP_LOG(EMMC50_CFG0);
	MSDC_REG_DUMP_LOG(EMMC50_CFG1);
	MSDC_REG_DUMP_LOG(EMMC50_CFG2);
	MSDC_REG_DUMP_LOG(EMMC50_CFG3);
	MSDC_REG_DUMP_LOG(EMMC50_CFG4);
#endif
#undef MSDC_REG_DUMP_LOG
	msdc_dump_dbg_register(host);
#endif
}

static inline void msdc_dump_clk(struct msdc_host *host)
{
	ERR_MSG("sclk 0x%p=0x%x, hclk 0x%p=0x%x\n", host->sclk_base,
		sdr_read32(host->sclk_base), host->hclk_base, sdr_read32(host->hclk_base));
}

static void msdc_dump_info(struct msdc_host *host)
{
	u32 __iomem *base;
	u32 temp;

	if (host == NULL) {
		pr_err("msdc_dump_info -> Invalid argument! host=0x%p\n", host);
		return;
	}

	base = host->base;

	/* 1: dump msdc hw register */
	msdc_dump_register(host);

	/* 2: check msdc clock gate and clock source */
	msdc_dump_clk(host);

	/* 3: check the register read_write */
	temp = sdr_read32(MSDC_PATCH_BIT0);
	INIT_MSG("patch reg[0x%p] = 0x%.8x\n", MSDC_PATCH_BIT0, temp);

	temp = (~temp);
	sdr_write32(MSDC_PATCH_BIT0, temp);
	temp = sdr_read32(MSDC_PATCH_BIT0);
	INIT_MSG("patch reg[0x%p] = 0x%.8x second time\n", MSDC_PATCH_BIT0, temp);

	temp = (~temp);
	sdr_write32(MSDC_PATCH_BIT0, temp);
	temp = sdr_read32(MSDC_PATCH_BIT0);
	INIT_MSG("patch reg[0x%p] = 0x%.8x Third time\n", MSDC_PATCH_BIT0, temp);
}

#ifdef CC_MTD_ENCRYPT_SUPPORT
static u8 _pu1MsdcBuf_AES[AES_BUFF_LEN + AES_ADDR_ALIGN], *_pu1MsdcBuf_Aes_Align;

void msdc_aes_init(void)
{
	u32 tmp;

	if (NAND_AES_INIT())
		;//pr_err("%s: yifan aes init ok\n", __func__);
	else
		pr_err("%s: yifan aes init failed\n", __func__);

	tmp = (unsigned long)&_pu1MsdcBuf_AES[0] & (AES_ADDR_ALIGN - 1);
	if(tmp != 0x0)
		_pu1MsdcBuf_Aes_Align = (UINT8 *)(((unsigned long)&_pu1MsdcBuf_AES[0] &
				(~((unsigned long)(AES_ADDR_ALIGN - 1)))) + AES_ADDR_ALIGN);
	else
		_pu1MsdcBuf_Aes_Align = &_pu1MsdcBuf_AES[0];

	//pr_err("%s: _pu1MsdcBuf_Aes_Align is 0x%p, _pu1MsdcBuf_AES is 0x%p\n",
	//		__func__, _pu1MsdcBuf_Aes_Align, &_pu1MsdcBuf_AES[0]);

	return;
}

int msdc_partition_encrypted(u32 blkindx, struct msdc_host *host)
{
	int i = 0;

	BUG_ON(!host);

	if (host->hw->host_function != MSDC_EMMC) {
		ERR_MSG("decrypt on emmc only!\n");
		return -1;
	}

	if (partition_access == 3) {
		ERR_MSG("rpmb skip enc\n");
		return 0;
	}

	for (i = 0; i < mtk_msdc_partition.nparts; i++) {
		if ((mtk_msdc_partition.parts[i].attr & ATTR_ENCRYPTION) &&
			(blkindx >= mtk_msdc_partition.parts[i].start_sect) &&
			(blkindx < (mtk_msdc_partition.parts[i].start_sect +
			mtk_msdc_partition.parts[i].nr_sects))) {
			N_MSG(FUC, " the buf(0x%08x) is encrypted!\n", blkindx);
			return 1;
		}
	}

	return 0;
}


int msdc_aes_encryption_sg(struct mmc_data *data,struct msdc_host *host)
{
	u32 len_used = 0, len_total = 0, len = 0, i = 0;
	unsigned long buff = 0;
	struct scatterlist *sg;

	BUG_ON(!host);

	if(host->hw->host_function != MSDC_EMMC) {
		ERR_MSG("decrypt on emmc only!\n");
		return -1;
	}

	if(!data) {
		ERR_MSG("decrypt data is invalid\n");
		return -1;
	}

	BUG_ON(data->sg_len > 1);
	for_each_sg(data->sg, sg, data->sg_len, i) {
		len = sg->length;
		buff = (unsigned long)sg_virt(sg);
		len_total = len;

		do {
			len_used = (len_total > AES_BUFF_LEN) ? AES_BUFF_LEN : len_total;
			if ((len_used & (AES_LEN_ALIGN - 1)) != 0x0) {
				ERR_MSG(" the buffer(0x%08lx) to be encrypted is not align to %d bytes!\n",
						buff, AES_LEN_ALIGN);
				return -1;
			}
			memcpy((void *)_pu1MsdcBuf_Aes_Align, (void *)buff, len_used);
			// TODO: 64bit to 32bit!!!!!!!!!!!!!!
			if (!NAND_AES_Encryption(virt_to_phys((void *)_pu1MsdcBuf_Aes_Align),
				virt_to_phys((void *)_pu1MsdcBuf_Aes_Align), len_used)) {
				ERR_MSG("Encryption to buffer(addr:0x%p size:0x%08X) failed!\n",
						_pu1MsdcBuf_Aes_Align, len_used);
				return -1;
			}
			memcpy((void *)buff, (void *)_pu1MsdcBuf_Aes_Align, len_used);

			len_total -= len_used;
			buff += len_used;
		} while (len_total > 0);
	}

	return 0;
}

int msdc_aes_decryption_sg(struct mmc_data *data,struct msdc_host *host)
{
	u32 len_used = 0, len_total = 0, len = 0, i = 0;
	unsigned long buff = 0;
	struct scatterlist *sg;

	BUG_ON(!host);

	if (host->hw->host_function != MSDC_EMMC) {
		ERR_MSG("decrypt on emmc only!\n");
		return -1;
	}

	if (!data) {
		ERR_MSG("decrypt data is invalid\n");
		return -1;
	}
	BUG_ON(data->sg_len > 1);
	for_each_sg(data->sg, sg, data->sg_len, i) {
		unsigned int offset = 0;

		len = sg->length;
		buff = (unsigned long)sg_virt(sg);
		len_total = len;
		do {
			len_used = (len_total > AES_BUFF_LEN) ? AES_BUFF_LEN : len_total;
			if ((len_used & (AES_LEN_ALIGN - 1)) != 0x0) {
				ERR_MSG("the buffer(0x%08lx) to be decrypted is not align to %d bytes!\n",
					buff, AES_LEN_ALIGN);
				return -1;
			}

			memcpy((void *)_pu1MsdcBuf_Aes_Align, (void *)buff, len_used);
			// TODO: 64bit to 32bit!!!!!!!!!!!!!!
			if (!NAND_AES_Decryption(virt_to_phys((void *)_pu1MsdcBuf_Aes_Align),
				virt_to_phys((void *)_pu1MsdcBuf_Aes_Align), len_used)) {
				ERR_MSG("Decryption to buffer(addr:0x%p size:0x%08X) failed!\n",
					_pu1MsdcBuf_Aes_Align, len_used);
				return -1;
			}
			memcpy((void *)buff, (void *)_pu1MsdcBuf_Aes_Align, len_used);

			len_total -= len_used;
			buff += len_used;
		} while (len_total > 0);
	}

	return 0;
}

#endif

/*
 * for AHB read / write debug
 * return DMA status.
 */
int msdc_get_dma_status(int host_id)
{
	int result = -1;

	if (host_id < 0 || host_id >= HOST_MAX_NUM) {
		pr_err("[%s] failed to get dma status, bad host_id %d\n", __func__, host_id);
		return result;
	}

	if (msdc_latest_transfer_mode[host_id] == TRAN_MOD_DMA) {
		switch (msdc_latest_operation_type[host_id]) {
		case OPER_TYPE_READ:
			result = 1; // DMA read
			break;
		case OPER_TYPE_WRITE:
			result = 2; // DMA write
			break;
		default:
			break;
		}
	} else if(msdc_latest_transfer_mode[host_id] == TRAN_MOD_PIO) {
		result = 0; // PIO mode
	}

	return result;
}
EXPORT_SYMBOL(msdc_get_dma_status);

struct dma_addr* msdc_get_dma_address(int host_id)
{
	bd_t* bd;
	int i;
	int mode = -1;
	struct msdc_host *host;
	u32 __iomem *base;

	if (host_id < 0 || host_id >= HOST_MAX_NUM) {
		ERR_MSG("failed to get dma status, bad host_id %d\n", host_id);
		return NULL;
	}

	if (!mtk_msdc_host[host_id]) {
		ERR_MSG("failed to get dma status, msdc%d is not exist\n", host_id);
		return NULL;
	}

	host = mtk_msdc_host[host_id];
	base = host->base;
	sdr_get_field(MSDC_DMA_CTRL, MSDC_DMA_CTRL_MODE, mode);
	if (mode == 1) {
		ERR_MSG("Desc.DMA\n");
		bd = host->dma.bd;
		i = 0;
		while (i < MAX_BD_PER_GPD) {
			msdc_latest_dma_address[i].start_address = (unsigned long)bd[i].ptr;
			msdc_latest_dma_address[i].size = bd[i].buflen;
			msdc_latest_dma_address[i].end = bd[i].eol;
			if (i > 0)
				msdc_latest_dma_address[i-1].next = &msdc_latest_dma_address[i];

			if (bd[i].eol)
				break;
			i++;
		}
	} else if (mode == 0) {
		ERR_MSG("Basic DMA\n");
		msdc_latest_dma_address[i].start_address = sdr_read_dma_sa();
#if defined(CONFIG_ARCH_MT5399) || defined(CONFIG_ARCH_MT5890) || \
	defined(CONFIG_ARCH_MT5882) || defined(CONFIG_ARCH_MT5883) || \
	defined(CONFIG_ARCH_MT5891)
		/* a change happens for dma xfer size:
		 * a new 32-bit register(0xA8) is used for xfer size configuration instead of 16-bit register(0x98 DMA_CTRL)
		 */
		msdc_latest_dma_address[i].size = sdr_read32(MSDC_DMA_LEN);
#else
		sdr_get_field(MSDC_DMA_CTRL, MSDC_DMA_CTRL_XFERSZ, msdc_latest_dma_address[i].size);
#endif
		msdc_latest_dma_address[i].end = 1;
	}

	return msdc_latest_dma_address;
}
EXPORT_SYMBOL(msdc_get_dma_address);

void msdc_set_driving(struct msdc_host *host, struct msdc_hw *hw)
{
	/* The value is determined by the experience
	 * So, a best proper value should be investigate further.
	 */
#if defined(CONFIG_ARCH_MT5891)
	u32 __iomem *reg_base;

	if (host->hw->host_function == MSDC_EMMC) {
		/*
		 * set driving
		 * Driving setting	  (driving clk 4.06 mA	  cmd and data 4.33mA)
		 * 0xf000d488:	CLK:[20][19][18]:010	 CMD:[23][22][21]:101	 DAT0:[26][25][24]:101
		 *				DAT1:[29][28][27]:101
		 * 0xf000d48c:	DAT2:[2][1][0]:101	 DAT3:[5][4][3]:101   DAT4:[8][7][6]:101
		 *						   DAT5:[11][10][9]:101  DAT6:[14][13][12]:101	 DAT7:[17][16][15]:101
		 */
		reg_base = ioremap(0xF000D488, 4);
		sdr_clr_bits(reg_base, 0xFFF << 18);
		sdr_set_bits(reg_base, (hw->clk_drv << 18) | (hw->cmd_drv << 21) |
								(hw->dat_drv << 24) | (hw->dat_drv << 27));
		iounmap(reg_base);
		reg_base = ioremap(0xF000D48C, 4);
		sdr_clr_bits(reg_base, 0x3FFFF);
		sdr_set_bits(reg_base, hw->dat_drv | (hw->dat_drv << 3) | (hw->dat_drv << 6) |
								(hw->dat_drv << 9) | (hw->dat_drv << 12) | (hw->dat_drv << 15));
		iounmap(reg_base);

		/* SR0 SR1 */
		reg_base = ioremap(0xF000D480, 4);
		sdr_clr_bits(reg_base, 0xFFF000);
		iounmap(reg_base);

		/* R1 R0(50K) */
		reg_base = ioremap(0xF000D4AC, 4);
		sdr_clr_bits(reg_base, 0xFFFFF000);
		sdr_set_bits(reg_base, 0x55556000);
		iounmap(reg_base);

		/*
		 * (clk pull down	cmd and dat0 pull up)
		 * 0xf000d4b4:	clk:[6]: 1	 cmd:[7]:0	DAT0:[8]:0	DAT1:[9]:0	DAT2:[10]:0
		 *				DAT3:[11]:0  DAT4:[12]:0   DAT5:[13]:0	 DAT6:[14]:0   DAT7:[15]:0
		 */
		reg_base = ioremap(0xF000D4B4, 4);
		sdr_clr_bits(reg_base, 0xFFC0);
		sdr_set_bits(reg_base, 0x40);
		iounmap(reg_base);

		/*
		 * RCLK (SMT config)
		 * 0xf000d4c4: pupd:[0]:1	R1R0:[2:1]:01	  SR1 SR0[4:3]:00	[19:17]:101
		 */
		reg_base = ioremap(0xF000D4C4, 4);
		sdr_clr_bits(reg_base, 0xE007F);
		sdr_set_bits(reg_base, 0xA0063);
		iounmap(reg_base);
		reg_base = ioremap(0xF000D4B0, 4);
		sdr_clr_bits(reg_base, 0xFFC0);
		sdr_set_bits(reg_base, 0xFFC0);
		iounmap(reg_base);
	} else {
		/*
		 * Driving setting	  (driving clk 4.06 mA	  cmd and data 4.33mA) (1.8V)
		 * 0xf000d488: CLK:[2:0]:010   CMD:[5:3]:101  DAT0:[8:6]:101  DAT1:[11:9]:101  DAT2:[14:12]:101   DAT3:[17:15]:101
		 */
		reg_base = ioremap(0xF000D488, 4);
		sdr_clr_bits(reg_base, 0x3FFFF);
		sdr_set_bits(reg_base, hw->clk_drv | (hw->cmd_drv << 3) | (hw->dat_drv << 6) |
								(hw->dat_drv << 9) | (hw->cmd_drv << 12) | (hw->dat_drv << 15));
		iounmap(reg_base);

		/* SR0 SR1 */
		reg_base = ioremap(0xF000D480, 4);
		sdr_clr_bits(reg_base, 0xFFF);
		iounmap(reg_base);

		/* R1 R0(50K) */
		reg_base = ioremap(0xF000D4AC, 4);
		sdr_clr_bits(reg_base, 0xFFF);
		sdr_set_bits(reg_base, 0x556);
		iounmap(reg_base);

		/*
		 * (clk pull down	cmd and dat0 pull up)
		 * 0xf000d4b4 : clk:[0]:1	cmd:[1]:0	DAT0-3:[2:5]:0000
		 */
		reg_base = ioremap(0xF000D4B4, 4);
		sdr_clr_bits(reg_base, 0x3F);
		sdr_set_bits(reg_base, 0x1);
		iounmap(reg_base);
		/* IES for HS400*/
		reg_base = ioremap(0xF000D4B0, 4);
		sdr_clr_bits(reg_base, 0x3F);
		sdr_set_bits(reg_base, 0x3F);
		iounmap(reg_base);
	}
#endif

#if defined(CONFIG_ARCH_MT5883)
	u32 __iomem *reg_base;
	/* set driving
	 * MSDC_SETBIT(0xF000D48C, 0x30C30C30);
	 * MSDC_SETBIT(0xF000D490, 0x0C30C30C);
	 * CLK:48C[9-4]  CMD:48C[15-10]  DATA0:48C[21-16] DATA1:48C[27-22] DATA2 low 4bit:48C[31-28]
	 * DATA2 high 2bit:490[0-1] DATA3:490[7-2]	DATA4:490[13-8] DATA5:490[23-20] DATA6:[49025-20] DATA7:490[31-26]
	 */
	reg_base = ioremap(0xF000D48C, 4);
	sdr_clr_bits(reg_base,(0xFFFFFFF0));
	sdr_set_bits(reg_base, ((hw->clk_drv&0x3F)<<4) | ((hw->cmd_drv&0x3F)<<10)|	((hw->dat_drv&0x3F)<<16)| \
								  ((hw->dat_drv&0x3F)<<22) |((hw->dat_drv&0xF)<<28) );
	iounmap(reg_base);
	reg_base = ioremap(0xF000D490, 4);
	sdr_clr_bits(reg_base,(0xFFFFFFFF));
	sdr_set_bits(reg_base, ((hw->dat_drv>>4)&0x3)  | ((hw->dat_drv&0x3F)<<2)  | ((hw->dat_drv&0x3F)<<8)| \
								  ((hw->dat_drv&0x3F)<<14) | ((hw->dat_drv&0x3F)<<20)| ((hw->dat_drv&0x3F)<<26));
	iounmap(reg_base);
	//set PUPD(CLK CMD DAT0-DAT8)
	reg_base = ioremap(0xF000D4B4, 4);
	sdr_clr_bits(reg_base,(0xFFC0));
	iounmap(reg_base);
	reg_base = ioremap(0xF000D4AC, 4);
	sdr_clr_bits(reg_base,(0xFFFFF000));
	sdr_set_bits(reg_base,(0x55555000));
	iounmap(reg_base);
#endif

#if defined(CONFIG_ARCH_MT5399) || defined(CONFIG_ARCH_MT5882)
	u32 __iomem *base = host->base;

	if (IS_IC_MT5885()) {
		sdr_clr_bits(MSDC_PAD_CTL0, (0x7<<15) |(0xF<<8) |(0x7<<4) | (0x7<<0));
		sdr_clr_bits(MSDC_PAD_CTL1, (0x7<<15) |(0xF<<8) |(0x7<<4) | (0x7<<0));
		sdr_clr_bits(MSDC_PAD_CTL2, (0x7<<15) |(0xF<<8) |(0x7<<4) | (0x7<<0));

		sdr_set_bits(MSDC_PAD_CTL0, ((0x3)<<15) |(((hw->clk_drv>>3)&0x7)<<4) | ((hw->clk_drv&0x7)<<0));
		sdr_set_bits(MSDC_PAD_CTL1, ((0x4)<<15) |(((hw->cmd_drv>>3)&0x7)<<4) | ((hw->cmd_drv&0x7)<<0));
		sdr_set_bits(MSDC_PAD_CTL2, ((0x4)<<15) |(((hw->dat_drv>>3)&0x7)<<4) | ((hw->dat_drv&0x7)<<0));
	} else {
		sdr_clr_bits(MSDC_PAD_CTL0, (0x7<<15) |(0x7<<4) |(0x7<<0));
		sdr_clr_bits(MSDC_PAD_CTL1, (0x7<<15) |(0x7<<4) |(0x7<<0));
		sdr_clr_bits(MSDC_PAD_CTL2, (0x7<<15) |(0x7<<4) |(0x7<<0));

		sdr_set_bits(MSDC_PAD_CTL0, ((0x4)<<15) |(((hw->clk_drv>>3)&0x7)<<4) | ((hw->clk_drv&0x7)<<0));
		sdr_set_bits(MSDC_PAD_CTL1, ((0x4)<<15) |(((hw->cmd_drv>>3)&0x7)<<4) | ((hw->cmd_drv&0x7)<<0));
		sdr_set_bits(MSDC_PAD_CTL2, ((0x4)<<15) |(((hw->dat_drv>>3)&0x7)<<4) | ((hw->dat_drv&0x7)<<0));
	}
#endif

#if defined(CONFIG_ARCH_MT5890)
	u32 __iomem *base = host->base;
	sdr_clr_bits(MSDC_PAD_CTL0, (0x7 << 15) | (0x7 << 0));
	sdr_clr_bits(MSDC_PAD_CTL1, (0x7 << 15) | (0x7 << 0));
	sdr_clr_bits(MSDC_PAD_CTL2, (0x7 << 15) | (0x7 << 0));

	if (IS_IC_MT5861_ES1()) {
		sdr_set_bits(MSDC_PAD_CTL0, ((0x4) << 15) | ((2 & 0x7) << 0));
		sdr_set_bits(MSDC_PAD_CTL1, ((0x4) << 15) | ((2 & 0x7) << 0));
		sdr_set_bits(MSDC_PAD_CTL2, ((0x4) << 15) | ((2 & 0x7) << 0));
	} else {
		sdr_set_bits(MSDC_PAD_CTL0, ((0x4) << 15) | ((hw->clk_drv & 0x7) << 0));
		sdr_set_bits(MSDC_PAD_CTL1, ((0x4) << 15) | ((hw->cmd_drv & 0x7) << 0));
		sdr_set_bits(MSDC_PAD_CTL2, ((0x4) << 15) | ((hw->dat_drv & 0x7) << 0));
	}
#endif
}

void msdc_pinmux(struct msdc_host *host)
{
	u32 __iomem *reg_base;

#if defined(CONFIG_ARCH_MT5891)
	if (host->hw->host_function == MSDC_EMMC) {
		//pinmux register d600[5:4], function 2 - CMD/DAT0~DAT3
		//pinmux register d600[7:6], function 2 - DAT4~DAT7
		//pinmux register d600[20], function 1 - CLK
		reg_base = ioremap(0xF000D600, 4);
		sdr_set_field(reg_base, 0xF << 4, 0xA);
		sdr_set_field(reg_base, 0x1 << 20, 1);
		sdr_set_field(reg_base, 0x1 << 21, 0);
		iounmap(reg_base);

		//Local Arbitor open
		reg_base = ioremap(0xF0012200, 4);
		sdr_set_field(reg_base, 0x1 << 5, 0x1);
		iounmap(reg_base);
	} else if (host->hw->host_function == MSDC_SD) {
		// GPIO Setting
		//pinmux register d604[28:26], function 1 - CMD/CLK/DAT0~DAT3
		reg_base = ioremap(0xF000D604, 4);
		sdr_set_field(reg_base, 0x7 << 26, 0x1);
		iounmap(reg_base);

		//Local Arbitor open
		reg_base = ioremap(0xF0012200, 4);
		sdr_set_field(reg_base, 0x1, 0x1);
		iounmap(reg_base);
	} else
		N_MSG(WRN, "unsupport function %d\n",
			host->hw->host_function);
#elif defined(CONFIG_ARCH_MT5399) || defined(CONFIG_ARCH_MT5890)
	if (IS_IC_MT5861_ES1()) {
		if (host->hw->host_function == MSDC_EMMC) {
			//pinmux register d600[5:4], function 2 - CMD/DAT0~DAT3
			//pinmux register d600[7:6], function 2 - DAT4~DAT7
			//pinmux register d610[7], function 1 - CLK
			reg_base = ioremap(0xF000D600, 4);
			sdr_set_field(reg_base, 0xF << 4, 0xA);
			iounmap(reg_base);
			reg_base = ioremap(0xF000D610, 4);
			sdr_set_field(reg_base, 0x1 << 7, 1);
			iounmap(reg_base);

			//Local Arbitor open
			reg_base = ioremap(0xF0012200, 4);
			sdr_set_field(reg_base, 0x1 << 5, 1);
			iounmap(reg_base);
		} else if(host->hw->host_function == MSDC_SD) {
			// GPIO Setting
			//pinmux register d604[27:26], function 1 - CMD/CLK/DAT0~DAT3
			reg_base = ioremap(0xF000D604, 4);
			sdr_set_field(reg_base, 0x3 << 26, 1);
			iounmap(reg_base);

			//Local Arbitor open
			reg_base = ioremap(0xF0012200, 4);
			sdr_set_field(reg_base, 1, 1);
			iounmap(reg_base);
		} else
			ERR_MSG("unsupport function %d\n", host->hw->host_function);
	} else {
		if (host->hw->host_function == MSDC_EMMC) {
			//pinmux register d600[5:4], function 2 - CMD/DAT0~DAT3
			//pinmux register d600[7:6], function 2 - DAT4~DAT7
			//pinmux register d610[7], function 1 - CLK
			reg_base = ioremap(0xF000D600, 4);
			sdr_set_field(reg_base, 0x3 << 4, 2);
			sdr_set_field(reg_base, 0x3 << 6, 2);
			iounmap(reg_base);
			reg_base = ioremap(0xF000D610, 4);
			sdr_set_field(reg_base, 0x1 << 7, 1);
			iounmap(reg_base);

			//Local Arbitor open
			reg_base = ioremap(0xF0012200, 4);
			sdr_set_field(reg_base, 0x03 << 16, 1);
			sdr_set_field(reg_base, 0x1F << 5, 0x1F);
			iounmap(reg_base);
		} else if(host->hw->host_function == MSDC_SD) {
			// GPIO Setting
			// Switch Pinmux to GPIO Function
			// Set gpio input mode
			reg_base = ioremap(0xF00280B4, 4);
			sdr_set_bits(reg_base, 0x1u<<31);
			iounmap(reg_base);
			reg_base = ioremap(0xF0028080, 4);
			sdr_clr_bits(reg_base, 0x1<<4);
			iounmap(reg_base);

			//pinmux register d604[27:26], function 1 - CMD/CLK/DAT0~DAT3
			reg_base = ioremap(0xF000D604, 4);
			sdr_set_field(reg_base, 0x03 << 26, 1);
			iounmap(reg_base);

			//Local Arbitor open
			reg_base = ioremap(0xF0012200, 4);
			sdr_set_field(reg_base, 0x03 << 16, 1);
			sdr_set_field(reg_base, 0x1F, 0x1F);
			iounmap(reg_base);
		} else
			ERR_MSG("unsupport function %d\n", host->hw->host_function);
	}
#elif defined(CONFIG_ARCH_MT5883)
	if (host->hw->host_function == MSDC_EMMC || host->hw->host_function == MSDC_SD) {
		reg_base = ioremap(0xF000D600, 4);
		sdr_set_field(reg_base, 0x1 << 2, 1);
		sdr_set_field(reg_base, 0xF << 4, 0xA);
		iounmap(reg_base);
		if(IS_IC_MT5883_ES1())
		{
			reg_base = ioremap(0xF0078104, 4);
			sdr_set_bits(reg_base, (0x1<<28));
			iounmap(reg_base);
		}
	} else
		ERR_MSG("unsupport function %d\n", host->hw->host_function);
#elif defined(CONFIG_ARCH_MT5882)
	if (IS_IC_MT5885()) {
		if (host->hw->host_function == MSDC_EMMC) {
			reg_base = ioremap(0xF000D600, 4);
			sdr_set_field(reg_base, 0x1 << 15, 1);
			sdr_set_field(reg_base, 0xF << 4, 0xA);
			iounmap(reg_base);

			//Local Arbitor open
			reg_base = ioremap(0xF0012200, 4);
			sdr_set_field(reg_base, 0x1 << 5, 1);
			iounmap(reg_base);
		} else if (host->hw->host_function == MSDC_SD) {
			// GPIO Setting
			//pinmux register d604[31:29], function 1 - CMD/CLK/DAT0~DAT3
			reg_base = ioremap(0xF000D604, 4);
			sdr_set_field(reg_base, 0x7 << 29, 1);
			iounmap(reg_base);

			//Local Arbitor open
			reg_base = ioremap(0xF0012200, 4);
			sdr_set_field(reg_base, 1, 1);
			iounmap(reg_base);
		} else
			N_MSG(WRN, "unsupport function %d\n",
				host->hw->host_function);
	} else {
		if (host->hw->host_function == MSDC_EMMC) {
			//pinmux register d600[5:4], function 2 - CMD/DAT0~DAT3
			//pinmux register d600[7:6], function 2 - DAT4~DAT7
			//pinmux register d610[7], function 1 - CLK
			reg_base = ioremap(0xF000D600, 4);
			sdr_set_field(reg_base, 0xF << 4, 0xA);
			sdr_set_field(reg_base, 0x1 << 2, 1);
			iounmap(reg_base);

			//Local Arbitor open
			reg_base = ioremap(0xF0012200, 4);
			sdr_set_field(reg_base, 0x1 << 5, 1);
			iounmap(reg_base);
		} else if (host->hw->host_function == MSDC_SD) {
			// GPIO Setting
			//pinmux register d604[31:29], function 1 - CMD/CLK/DAT0~DAT3
			reg_base = ioremap(0xF000D604, 4);
			sdr_set_field(reg_base, 0x7 << 29, 1);
			iounmap(reg_base);

			//Local Arbitor open
			reg_base = ioremap(0xF0012200, 4);
			sdr_set_field(reg_base, 1, 1);
			iounmap(reg_base);
		} else
			N_MSG(WRN, "unsupport function %d\n",
				host->hw->host_function);
	}
#endif
}

void msdc_set_timeout(struct msdc_host *host, u32 ns, u32 clks)
{
	u32 __iomem *base = host->base;
	u32 timeout, clk_ns;

	host->timeout_ns   = ns;
	host->timeout_clks = clks;

	//clk_ns  = 1000000000UL / host->sclk;
	clk_ns	= 1000000000UL / host->mclk;
	timeout = ns / clk_ns + clks;
	timeout = timeout >> 20; /* in 1048576 sclk cycle unit (83/85)*/
	timeout = timeout > 1 ? timeout - 1 : 0;
	timeout = timeout > 255 ? 255 : timeout;

	sdr_set_field(SDC_CFG, SDC_CFG_DTOC, timeout);

	N_MSG(OPS, "Set read data timeout: %dns %dclks -> %d x 1048576	cycles\n",
		ns, clks, timeout + 1);
}

static int MsdcSDPowerSwitch(unsigned int i4V)
{
	if (0 != mtk_gpio_direction_output(MSDC_Gpio[MSDC0VoltageSwitchGpio], i4V))
		return -1;
	return 0;
}

/* detect cd interrupt */

static void msdc_tasklet_card(unsigned long arg)
{
	struct msdc_host *host = (struct msdc_host *)arg;
	u32 inserted;

	inserted = msdc_card_exist(host);

	if (!host->suspend)
	{
		if(inserted && !host->card_inserted)
		{
			host->block_bad_card = 0;
			INIT_MSG("card found<%s>\n", inserted ? "inserted" : "removed");
			host->card_inserted = inserted;
			MsdcSDPowerSwitch(0);//set the host ic gpio to 3.3v
			mmc_detect_change(host->mmc, msecs_to_jiffies(1500));
		}
		else
		if(!inserted && host->card_inserted)
		{
			INIT_MSG("card found<%s>\n", inserted ? "inserted" : "removed");
			host->block_bad_card = 0;
			host->card_inserted = inserted;
			MsdcSDPowerSwitch(0);//set the host ic gpio to 3.3v
			mmc_detect_change(host->mmc, msecs_to_jiffies(10));
		}
		else
		if(host->block_bad_card && !host->card_inserted)
		{
			host->block_bad_card = 0;
			INIT_MSG("card found removed\n");
			MsdcSDPowerSwitch(0);//set the host ic gpio to 3.3v
			mmc_detect_change(host->mmc, msecs_to_jiffies(10));
		}
	}

	mod_timer(host->card_detect_timer, jiffies + (HZ >> 2));
}


// wait for extend if power pin is controlled by GPIO
void msdc0_card_power_up(void)
{
	if (MSDC_Gpio[MSDC0PoweronoffDetectGpio] != -1) {
		mtk_gpio_direction_output(MSDC_Gpio[MSDC0PoweronoffDetectGpio], 1);
		//ERR_MSG("msdc0_card_power_up MSDC_Gpio[MSDC0PoweronoffGpio] :%d \n",MSDC_Gpio[MSDC0PoweronoffDetectGpio]);
	}
}
// wait for extend if power pin is controlled by GPIO
void msdc0_card_power_down(void)
{

	if(MSDC_Gpio[MSDC0PoweronoffDetectGpio] != -1) {
		mtk_gpio_direction_output(MSDC_Gpio[MSDC0PoweronoffDetectGpio], 0);
		//ERR_MSG("msdc0_card_power_down MSDC_Gpio[MSDC0PoweronoffGpio] :%d \n",MSDC_Gpio[MSDC0PoweronoffDetectGpio]);
	}

}

static int msdc_card_exist(void *data)
{
	unsigned int val;
	struct msdc_host *host = (struct msdc_host *)data;

	if (!host)
		return 0;

	//ERR_MSG("MSDC_Gpio[MSDC0DetectGpio] :%d \n",MSDC_Gpio[MSDC0DetectGpio]);

	if(host->hw->host_function == MSDC_EMMC)
		return 1;
	else if (host->hw->host_function == MSDC_SD) {
		/* SD CARD DETECT GPIO VALUE */
		if (MSDC_Gpio[MSDC0DetectGpio] == -1)
			return 0;
		else {
			mtk_gpio_direction_input(MSDC_Gpio[MSDC0DetectGpio]);
			val = mtk_gpio_get_value(MSDC_Gpio[MSDC0DetectGpio]);
			//ERR_MSG("tyler return val: %d \n",val);
		}

		if (val) {
			msdc0_card_power_down();
			//ERR_MSG("[0]Card not Insert(%d)!\n", val);
			return 0;
		}

		msdc0_card_power_up();
		return 1;
	}
	//ERR_MSG("[0]Card Insert(%d)!\n", val);
	return 0;
}

#ifdef MSDC_ONLY_USE_ONE_CLKSRC
static inline void msdc_hclksrc_onoff(struct msdc_host *host, u32 on)
{
	if (on)
		sdr_clr_bits(host->hclk_base, MSDC_CLK_GATE_BIT);
	else
		sdr_set_bits(host->hclk_base, MSDC_CLK_GATE_BIT);
}

static void msdc_config_hclk(struct msdc_host *host)
{
	u32 __iomem *reg;

	/* Configure the hclk */
	msdc_hclksrc_onoff(host, 0);
	sdr_set_field(host->hclk_base, MSDC_CLK_SEL_MASK, host->hw->hclk_src);
	msdc_hclksrc_onoff(host, 1);

	/* Enable the dmuxpll(sawlesspll_d2p5_ck) used by emmc sclk, which is default off */
	reg = ioremap(MSDC_CLK_DMUXPLL_REG_ADDR, 0x4);
	sdr_set_field(reg, MSDC_DMULPLL_GATE_BIT, 1);
	iounmap(reg);
}

static void msdc_select_clksrc(struct msdc_host* host, int clksrc)
{
	if ((clksrc != host->sclk_src) || (host->sclk_src == MSDC_SCLK_SRC_INVALID)) {
		msdc_clksrc_onoff(host, 0);
		sdr_set_field(host->sclk_base, MSDC_CLK_SEL_MASK, clksrc);
		msdc_clksrc_onoff(host, 1);

		host->sclk_src = clksrc;
		//ERR_MSG("select clk source %d(%dHz)\n", clksrc, msdc_cur_sclk_freq(host));
	}
}

static void msdc_apply_kernel_autok(struct msdc_host *host)
{
	u32 __iomem *base = host->base;
	int i = 0;
	int old_ds_dly1 = 0;
	int new_ds_dly1 = 0;

	sdr_write32(MSDC_IOCON,MSDC_Autok[i++]);
	sdr_write32(MSDC_PAD_TUNE0,MSDC_Autok[i++]);
	sdr_write32(MSDC_PAD_TUNE1,MSDC_Autok[i++]);
	sdr_write32(MSDC_PATCH_BIT0,MSDC_Autok[i++]);
	sdr_write32(MSDC_PATCH_BIT1,MSDC_Autok[i++]);
	sdr_write32(MSDC_PATCH_BIT2,MSDC_Autok[i++]);
	sdr_write32(EMMC50_CFG0,MSDC_Autok[i++]);

	sdr_write32(MSDC_DAT_RDDLY0,MSDC_Autok[i++]);
	sdr_write32(MSDC_DAT_RDDLY1,MSDC_Autok[i++]);
	sdr_write32(MSDC_DAT_RDDLY2,MSDC_Autok[i++]);
	sdr_write32(MSDC_DAT_RDDLY3,MSDC_Autok[i++]);

	sdr_write32(EMMC50_PAD_DS_TUNE,MSDC_Autok[i++]);
	sdr_write32(EMMC50_PAD_CMD_TUNE,MSDC_Autok[i++]);
	sdr_write32(EMMC50_PAD_DAT01_TUNE,MSDC_Autok[i++]);
	sdr_write32(EMMC50_PAD_DAT23_TUNE,MSDC_Autok[i++]);
	sdr_write32(EMMC50_PAD_DAT45_TUNE,MSDC_Autok[i++]);
	sdr_write32(EMMC50_PAD_DAT67_TUNE,MSDC_Autok[i++]);

	//adjust DS TUNE,because loader DS window is larger than kernel
	sdr_get_field(EMMC50_PAD_DS_TUNE,MSDC_EMMC50_PAD_DS_TUNE_DLY1,old_ds_dly1);
	#if 0
	//if(IS_IC_5891_BI())
	if (0)
	{
		new_ds_dly1 = (old_ds_dly1 >= 2)? (old_ds_dly1 - 2) : 0;
	}
	else //if(IS_IC_5891_SM())
	{
		new_ds_dly1 = (old_ds_dly1 >= 1)? (old_ds_dly1 - 1) : 0;
	}
	#else
	//currently, we not adjust the DS delay from loader to kernel, at worst,kernel do a autok is ok.
	//in case for some emmc, the adjust may cause always CRCERR
	new_ds_dly1 = old_ds_dly1;
	#endif
	sdr_set_field(EMMC50_PAD_DS_TUNE,MSDC_EMMC50_PAD_DS_TUNE_DLY1,new_ds_dly1);


	//dump the registers
	pr_err("\nIOCON= 0x%x \n",sdr_read32(MSDC_IOCON));
	pr_err("MSDC_PAD_TUNE0= 0x%x \n",sdr_read32(MSDC_PAD_TUNE0));
	pr_err("MSDC_PAD_TUNE1= 0x%x \n",sdr_read32(MSDC_PAD_TUNE1));
	pr_err("MSDC_PATCH_BIT0= 0x%x \n",sdr_read32(MSDC_PATCH_BIT0));
	pr_err("MSDC_PATCH_BIT1= 0x%x \n",sdr_read32(MSDC_PATCH_BIT1));
	pr_err("MSDC_PATCH_BIT2= 0x%x \n",sdr_read32(MSDC_PATCH_BIT2));
	pr_err("EMMC50_CFG0= 0x%x \n",sdr_read32(EMMC50_CFG0));
	pr_err("MSDC_DAT_RDDLY0= 0x%x \n",sdr_read32(MSDC_DAT_RDDLY0));
	pr_err("MSDC_DAT_RDDLY1= 0x%x \n",sdr_read32(MSDC_DAT_RDDLY1));
	pr_err("MSDC_DAT_RDDLY2= 0x%x \n",sdr_read32(MSDC_DAT_RDDLY2));
	pr_err("MSDC_DAT_RDDLY3= 0x%x \n",sdr_read32(MSDC_DAT_RDDLY3));
	pr_err("EMMC50_PAD_DS_TUNE= 0x%x ,adjust delay %d -> %d \n",sdr_read32(EMMC50_PAD_DS_TUNE),old_ds_dly1,new_ds_dly1);
	pr_err("EMMC50_PAD_CMD_TUNE= 0x%x \n",sdr_read32(EMMC50_PAD_CMD_TUNE));
	pr_err("EMMC50_PAD_DAT01_TUNE= 0x%x \n",sdr_read32(EMMC50_PAD_DAT01_TUNE));
	pr_err("EMMC50_PAD_DAT23_TUNE= 0x%x \n",sdr_read32(EMMC50_PAD_DAT23_TUNE));
	pr_err("EMMC50_PAD_DAT45_TUNE= 0x%x \n",sdr_read32(EMMC50_PAD_DAT45_TUNE));
	pr_err("EMMC50_PAD_DAT67_TUNE= 0x%x \n",sdr_read32(EMMC50_PAD_DAT67_TUNE));
}

/* 400K call stack
 * 1. msdc_drv_probe -> mmc_add_host -> mmc_start_host ->
 * mmc_power_up -> msdc_ops_set_ios -> msdc_set_mclk
 * 2. mmc_init_card
 */
static void msdc_set_mclk(struct msdc_host *host, msdc_state state, u32 hz)
{
	u32 __iomem *base = host->base;
	u32 mode = 1; /* no divisor */
	u32 div  = 0;
	u32 inten_bak;
	u32 hs400_src = 0;
	u32 sclk; /* sclk: the really clock after divition */
	u32 clksrc_freq;

	host->mclk = hz;
	if (!hz) {
		msdc_reset_hw();
		return;
	}

	clksrc_freq = msdc_cur_sclk_freq(host);

	if (state == MSDC_STATE_HS400) {
		mode = 0x3; /* HS400 mode */
		if (clksrc_freq <= 400000000) {
			hs400_src = 1;
			div = 0; /* If mode=3, div MUST BE 0 !!!! */
			sclk = clksrc_freq >> 1;
		} else {
			ERR_MSG("HW limitation, can not use clk source more than 400MHz (%dHz)\n",
					clksrc_freq);
			return;
		}
	} else if (state == MSDC_STATE_DDR) {
		mode = 0x2; /* ddr mode and use divisor */
		if (hz >= (clksrc_freq >> 2)) {
			div  = 0; /* mean div = 1/4 */
			sclk = clksrc_freq >> 2; /* sclk = clk / 4 */
		} else {
			div  = (clksrc_freq + ((hz << 2) - 1)) / (hz << 2);
			sclk = (clksrc_freq >> 2) / div;
			div  = (div >> 1);
		}
	} else if (hz >= clksrc_freq) {
		mode = 0x1; /* no divisor */
		div  = 0;
		sclk = clksrc_freq;
	} else {
		mode = 0x0; /* use divisor */
		if (hz >= (clksrc_freq >> 1)) {
			div  = 0; /* mean div = 1/2 */
			sclk = clksrc_freq >> 1; /* sclk = clk / 2 */
		} else {
			div = (clksrc_freq + ((hz << 2) - 1)) / (hz << 2);
			if (0)//host->sclk_src == EMMC_SCLK_24M)
				div++;
			sclk = (clksrc_freq >> 2) / div;
		}
	}

	if (state == MSDC_STATE_HS400 || state == MSDC_STATE_HS200) {
		host->hw->clk_drv = 5;
		host->hw->cmd_drv = 6;
		host->hw->dat_drv = 5;
	} else {
		host->hw->clk_drv = 2;
		host->hw->cmd_drv = 5;
		host->hw->dat_drv = 5;
	}
	msdc_set_driving(host, host->hw);

	msdc_inten_save(inten_bak);
	msdc_clk_stable(host, mode, div, hs400_src);
	msdc_inten_restore(inten_bak);

	host->sclk = sclk;
	host->state = state;

	msdc_set_timeout(host, host->timeout_ns, host->timeout_clks);

    /*
	ERR_MSG("Set<%dKHz> Source<%dKHz> -> real<%dKHz> state<%d> mode<%d> div<%d> hs400_src<%d>\n",
		   hz / 1000, clksrc_freq / 1000, sclk / 1000, state, mode, div, hs400_src);
    */

	if ((hz > 100000000) &&
		((state == MSDC_STATE_HS400) || (state == MSDC_STATE_HS200)) &&
		(g_autok_cmdline_parsed & MSDC_CMD_LINE_PARSE_DONE)
		)
	{
		if (state == MSDC_STATE_HS400)
			ERR_MSG("========MSDC APPLY HS400 AUTOK PARAM=========\n");
		else if (state == MSDC_STATE_HS200)
			ERR_MSG("========MSDC APPLY HS200 AUTOK PARAM=========\n");

		msdc_apply_kernel_autok(host);
	}

	//msdc_save_host_setting(host);
}

#else /* MSDC_ONLY_USE_ONE_CLKSRC */

#define msdc_config_hclk(h)
static void msdc_select_clksrc(struct msdc_host *host, int clksrc)
{
	if (clksrc != host->hw->sclk_src) {
		ERR_MSG("select clk source %d\n", clksrc);

		msdc_clksrc_onoff(host, 0);
		sdr_clr_bits(host->sclk_base, MSDC_CLK_SEL_MASK);
	#if defined(CC_EMMC_DDR50)
		// TODO: why DDR50 use 8?????????
		if (host->hw->host_function == MSDC_EMMC)
			sdr_set_bits(host->sclk_base, 0x8);
	#endif
		sdr_set_bits(host->sclk_base, clksrc);
		msdc_clksrc_onoff(host, 1);

		host->hw->sclk_src = clksrc;
	}
}

static void msdc_set_mclk(struct msdc_host *host, msdc_state state, u32 hz)
{
	u32 __iomem *base = host->base;
	u32 mode = 1; /* no divisor */
	u32 div  = 0;
	u32 flags;
	u32 sdClkSel = 0, idx = 0;

	if (!hz) { // set mmc system clock to 0
		ERR_MSG("set mclk to 0\n");  // fix me: need to set to 0
		host->mclk = 0;
		msdc_reset_hw();
		return;
	} else
		ERR_MSG("set mclk to %d\n", hz);

	msdc_inten_save(flags);

	do {
		if((hz < msdcClk[IDX_CLK_TARGET][idx]) ||
		   ((state == MSDC_STATE_DDR) && (msdcClk[IDX_CLK_MODE_VAL][idx] != 2)))
			continue;
		else
			break;

	} while (++idx < (sizeof(msdcClk[0]) / sizeof(msdcClk[0][0])));

	idx = (idx >= (sizeof(msdcClk[0]) / sizeof(msdcClk[0][0]))) ?
			((sizeof(msdcClk[0]) / sizeof(msdcClk[0][0]))) : idx;

	sdClkSel = msdcClk[IDX_CLK_SRC_VAL][idx];
	div = msdcClk[IDX_CLK_DIV_VAL][idx];
	mode = msdcClk[IDX_CLK_MODE_VAL][idx];

	msdc_select_clksrc(host, sdClkSel);

	host->hw->clk_drv = msdcClk[IDX_CLK_DRV_VAL][idx];
	host->hw->cmd_drv = msdcClk[IDX_CMD_DRV_VAL][idx];
	host->hw->dat_drv = msdcClk[IDX_DAT_DRV_VAL][idx];
	msdc_set_driving(host, host->hw);

	msdc_clk_stable(host, mode, div, 0);

	host->sclk = msdcClk[IDX_CLK_TARGET][idx];
	host->mclk = hz;
	host->state = state;

	msdc_set_timeout(host, host->timeout_ns, host->timeout_clks); // need because clk changed.

	ERR_MSG("Set<%dKHz> Source<%dKHz> -> real<%dKHz> state<%d> mode<%d> div<%d>\n",
		   hz/1000, host->sclk ? host->sclk/1000 : 24000, host->mclk/1000, state, mode, div);

	msdc_inten_restore(flags);
}
#endif /* MSDC_ONLY_USE_ONE_CLKSRC */

void msdc_send_stop(struct msdc_host *host)
{
	struct mmc_command stop = {0};
	struct mmc_request mrq = {0};
	u32 err = -1;

	stop.opcode = MMC_STOP_TRANSMISSION;
	stop.arg = 0;
	stop.flags = MMC_RSP_R1B | MMC_CMD_AC;

	mrq.cmd = &stop; stop.mrq = &mrq;
	stop.data = NULL;

	err = msdc_do_command(host, &stop, 0, CMD_TIMEOUT);
}

static void msdc_reset_tune_counter(struct msdc_host *host,TUNE_COUNTER index)
{
	if(index >= 0 && index <= all_counter){
	switch (index){
		case cmd_counter:
			if(host->t_counter.time_cmd != 0)
				ERR_MSG("TUNE CMD Times(%d)\n", host->t_counter.time_cmd);
			host->t_counter.time_cmd = 0;
			break;
		case read_counter:
			if (host->t_counter.time_read != 0)
				ERR_MSG("TUNE READ Times(%d)\n", host->t_counter.time_read);
			host->t_counter.time_read = 0;
			break;
		case write_counter:
			if (host->t_counter.time_write != 0)
				ERR_MSG("TUNE WRITE Times(%d)\n", host->t_counter.time_write);
			host->t_counter.time_write = 0;
			break;
		case all_counter:
			if (host->t_counter.time_cmd != 0)
				ERR_MSG("TUNE CMD Times(%d)\n", host->t_counter.time_cmd);
							 if(host->t_counter.time_read != 0)
				ERR_MSG("TUNE READ Times(%d)\n", host->t_counter.time_read);
							 if(host->t_counter.time_write != 0)
				ERR_MSG("TUNE WRITE Times(%d)\n", host->t_counter.time_write);
							 host->t_counter.time_cmd = 0;
							 host->t_counter.time_read = 0;
							 host->t_counter.time_write = 0;
							 host->read_time_tune = 0;
							 host->write_time_tune = 0;
							 host->rwcmd_time_tune = 0;
							 break;
		default:
			break;
		}
	} else
		ERR_MSG("reset counter index(%d) error!\n", index);
}

/* Fix me. when need to abort */
static u32 msdc_abort_data(struct msdc_host *host)
{
	struct mmc_host *mmc = host->mmc;
	u32 __iomem *base = host->base;
	u32 status = 0;
	u32 state = 0;
	u32 err = 0;
	unsigned long tmo = jiffies + POLLING_BUSY;

	while (state != 4) { // until status to "tran"
		msdc_reset_hw();
		while ((err = msdc_get_card_status(mmc, host, &status))) {
			ERR_MSG("CMD13 ERR<%d>\n", err);
			if (err != (unsigned int)-EIO) {
				return 1;
			} else if (msdc_tune_cmdrsp(host)) {
				ERR_MSG("update cmd para failed\n");
				return 1;
			}
		}

		state = R1_CURRENT_STATE(status);
		//ERR_MSG("check card state<%d>\n", state);
		if (state == 5 || state == 6) {
			ERR_MSG("state<%d> need cmd12 to stop\n", state);
			msdc_send_stop(host); // don't tuning
		} else if (state == 7) {  // busy in programing
			ERR_MSG("state<%d> card is busy\n", state);
			spin_unlock(&host->lock);
			msleep(100);
			spin_lock(&host->lock);
		} else if (state != 4) {
			ERR_MSG("state<%d> ??? \n", state);
			return 1;
		}

		if (time_after(jiffies, tmo)) {
			ERR_MSG("abort timeout. Do power cycle\n");
			if (host->hw->host_function == MSDC_SD &&
				(host->sclk >= 100000000 || state == MSDC_STATE_DDR))
				host->sd_30_busy++;
			return 1;
		}
	}

	msdc_reset_hw();
	return 0;
}
static u32 msdc_polling_idle(struct msdc_host *host)
{
	struct mmc_host *mmc = host->mmc;
	u32 status = 0;
	u32 state = 0;
	u32 err = 0;
	unsigned long tmo = jiffies + POLLING_BUSY;

	while (state != 4) { // until status to "tran"
		while ((err = msdc_get_card_status(mmc, host, &status))) {
			ERR_MSG("CMD13 ERR<%d>\n", err);
			if (err != (unsigned int)-EIO) {
				return 1;
			} else if (msdc_tune_cmdrsp(host)) {
				ERR_MSG("update cmd para failed\n");
				return 1;
			}
		}

		state = R1_CURRENT_STATE(status);
		//ERR_MSG("check card state<%d>", state);
		if (state == 5 || state == 6) {
			ERR_MSG("state<%d> need cmd12 to stop\n", state);
			msdc_send_stop(host); // don't tuning
		} else if (state == 7) {  // busy in programing
			ERR_MSG("state<%d> card is busy\n", state);
			spin_unlock(&host->lock);
			msleep(100);
			spin_lock(&host->lock);
		} else if (state != 4) {
			ERR_MSG("state<%d> ??? \n", state);
			return 1;
		}

		if (time_after(jiffies, tmo)) {
			ERR_MSG("abort timeout. Do power cycle\n");
			return 1;
		}
	}

	return 0;
}

static inline void msdc_clksrc_onoff(struct msdc_host *host, u32 on)
{
	if (on)
		sdr_clr_bits(host->sclk_base, MSDC_CLK_GATE_BIT);
	else
		sdr_set_bits(host->sclk_base, MSDC_CLK_GATE_BIT);
}

static int msdc_clk_stable(struct msdc_host *host, u32 mode, u32 div, u32 hs400_src)
{
	u32 __iomem *base = host->base;
	int retry = 0;
	int cnt = 1000;
	int retry_cnt = 1;

	do {
		retry = 3000;
		msdc_clksrc_onoff(host, 0);
		sdr_set_field(MSDC_CFG, MSDC_CFG_CKMOD, mode);
		sdr_set_field(MSDC_CFG, MSDC_CFG_CKDIV, (div + retry_cnt) % 0xff);
	#ifdef CONFIG_EMMC_HS400_SUPPORT
		sdr_set_field(MSDC_CFG, MSDC_CFG_CKMOD_HS400, hs400_src);
	#endif
		msdc_clksrc_onoff(host, 1);

		msdc_retry(!(sdr_read32(MSDC_CFG) & MSDC_CFG_CKSTB), retry, cnt);
		if (retry == 0) {
			ERR_MSG("wait clock stable failed ===> retry twice\n");
			msdc_clksrc_onoff(host,0);
			msdc_clksrc_onoff(host,1);
		}

		retry = 3000;
		msdc_clksrc_onoff(host, 0);
		sdr_set_field(MSDC_CFG, MSDC_CFG_CKDIV, div);//to check while set ckdiv again here
		msdc_clksrc_onoff(host, 1);
		msdc_retry(!(sdr_read32(MSDC_CFG) & MSDC_CFG_CKSTB), retry, cnt);

		if(retry_cnt == 0)
			ERR_MSG("wait clock stable retry failed\n");

		msdc_reset_hw();

		if(retry_cnt == 2)
			break;

		retry_cnt++;
	} while(!retry);

	return 0;
}


/*
   register as callback function of WIFI(combo_sdio_register_pm) .
   can called by msdc_drv_suspend/resume too.
*/
static void msdc_save_emmc_setting(struct msdc_host *host)
{
	u32 __iomem *base = host->base;
	host->saved_para.state = host->state;
	host->saved_para.hz = host->mclk;
	host->saved_para.sdc_cfg = sdr_read32(SDC_CFG);
	sdr_get_field(MSDC_IOCON, MSDC_IOCON_RSPL, host->hw->cmd_edge); // save the para
	sdr_get_field(MSDC_IOCON, MSDC_IOCON_R_D_SMPL, host->hw->rdata_edge);
	sdr_get_field(MSDC_IOCON, MSDC_IOCON_W_D_SMPL, host->hw->wdata_edge);
	host->saved_para.pad_tune = sdr_read32(MSDC_PAD_TUNE0);
	host->saved_para.ddly0 = sdr_read32(MSDC_DAT_RDDLY0);
	host->saved_para.ddly1 = sdr_read32(MSDC_DAT_RDDLY1);
	sdr_get_field(MSDC_PATCH_BIT1, MSDC_PB1_CMD_RSP_TA_CNTR,	host->saved_para.cmd_resp_ta_cntr);
	sdr_get_field(MSDC_PATCH_BIT1, MSDC_PB1_WRDAT_CRCS_TA_CNTR, host->saved_para.wrdat_crc_ta_cntr);
}

static void msdc_restore_emmc_setting(struct msdc_host *host)
{
	u32 __iomem *base = host->base;
	msdc_set_mclk(host, host->saved_para.state, host->mclk);
	sdr_write32(SDC_CFG,host->saved_para.sdc_cfg);
	sdr_set_field(MSDC_IOCON, MSDC_IOCON_RSPL, host->hw->cmd_edge);
	sdr_set_field(MSDC_IOCON, MSDC_IOCON_R_D_SMPL, host->hw->rdata_edge);
	sdr_set_field(MSDC_IOCON, MSDC_IOCON_W_D_SMPL, host->hw->wdata_edge);
	sdr_write32(MSDC_PAD_TUNE0,host->saved_para.pad_tune);
	sdr_write32(MSDC_DAT_RDDLY0,host->saved_para.ddly0);
	sdr_write32(MSDC_DAT_RDDLY1,host->saved_para.ddly1);
	sdr_set_field(MSDC_PATCH_BIT1, MSDC_PB1_WRDAT_CRCS_TA_CNTR, host->saved_para.wrdat_crc_ta_cntr);
	sdr_set_field(MSDC_PATCH_BIT1, MSDC_PB1_CMD_RSP_TA_CNTR,	host->saved_para.cmd_resp_ta_cntr);
}


static u64 msdc_get_user_capacity(struct msdc_host *host)
{
	u64 device_capacity = 0;
	u32 device_legacy_capacity = 0;
	struct mmc_host* mmc = NULL;

	BUG_ON(!host);
	BUG_ON(!host->mmc);
	BUG_ON(!host->mmc->card);

	mmc = host->mmc;
	if (mmc_card_mmc(mmc->card)) {
		if(mmc->card->csd.read_blkbits)
			device_legacy_capacity = mmc->card->csd.capacity * (2 << (mmc->card->csd.read_blkbits - 1));
		else {
			device_legacy_capacity = mmc->card->csd.capacity;
			ERR_MSG("XXX read_blkbits = 0 XXX\n");
		}
		device_capacity = (u64)(mmc->card->ext_csd.sectors)* 512 > device_legacy_capacity ?
			(u64)(mmc->card->ext_csd.sectors)* 512 : device_legacy_capacity;
	} else if(mmc_card_sd(mmc->card))
		device_capacity = (u64)(mmc->card->csd.capacity) << (mmc->card->csd.read_blkbits);

	return device_capacity;
}

u32 msdc_get_capacity(int get_emmc_total)
{
	u64 user_size = 0;
	u32 other_size = 0;
	u64 total_size = 0;
	int index = 0;

	for (index = 0; index < HOST_MAX_NUM; ++index) {
		if ((mtk_msdc_host[index] != NULL) && (mtk_msdc_host[index]->hw->boot)) {
			user_size = msdc_get_user_capacity(mtk_msdc_host[index]);
			#ifdef CONFIG_MTKEMMC_BOOT
			if (get_emmc_total) {
				if (mmc_card_mmc(mtk_msdc_host[index]->mmc->card))
					other_size = msdc_get_other_capacity();
			}
			#endif
			break;
		}
	}
	total_size = user_size + other_size;
	return total_size/512;
}
EXPORT_SYMBOL(msdc_get_capacity);
/*--------------------------------------------------------------------------*/
/* mmc_host_ops members														 */
/*--------------------------------------------------------------------------*/
static u32 wints_cmd = MSDC_INT_CMDRDY	| MSDC_INT_RSPCRCERR  | MSDC_INT_CMDTMO  |
						MSDC_INT_ACMDRDY | MSDC_INT_ACMDCRCERR | MSDC_INT_ACMDTMO;
unsigned int msdc_command_start(struct msdc_host   *host,
									  struct mmc_command *cmd,
									  int				  tune,   /* not used */
									  unsigned long		  timeout)
{
	u32 __iomem *base = host->base;
	u32 opcode = cmd->opcode;
	u32 rawcmd = 0;
	u32 rawarg = 0;
	u32 resp = 0;
	unsigned long tmo = 0;

	/* Protocol layer does not provide response type, but our hardware needs
	 * to know exact type, not just size!
	 */
	if (opcode == MMC_SEND_OP_COND || opcode == SD_APP_OP_COND)
		resp = RESP_R3;
	else if (opcode == MMC_SET_RELATIVE_ADDR || opcode == SD_SEND_RELATIVE_ADDR)
		resp = (mmc_cmd_type(cmd) == MMC_CMD_BCR) ? RESP_R6 : RESP_R1;
	else if (opcode == MMC_FAST_IO)
		resp = RESP_R4;
	else if (opcode == MMC_GO_IRQ_STATE)
		resp = RESP_R5;
	else if (opcode == MMC_SELECT_CARD) {
		resp = (cmd->arg != 0) ? RESP_R1B : RESP_NONE;
		host->app_cmd_arg = cmd->arg;
		//ERR_MSG("msdc%d select card<0x%.8x>\n", host->id, cmd->arg);  // select and de-select
	} else if (opcode == SD_SEND_IF_COND && (mmc_cmd_type(cmd) == MMC_CMD_BCR))
		resp = RESP_R1;
	else {
		switch (mmc_resp_type(cmd)) {
		case MMC_RSP_R1:
			resp = RESP_R1;
			break;
		case MMC_RSP_R1B:
			resp = RESP_R1B;
			break;
		case MMC_RSP_R2:
			resp = RESP_R2;
			break;
		case MMC_RSP_R3:
			resp = RESP_R3;
			break;
		case MMC_RSP_NONE:
		default:
			resp = RESP_NONE;
			break;
		}
	}

	cmd->error = 0;
	/* rawcmd :
	 * vol_swt << 30 | auto_cmd << 28 | blklen << 16 | go_irq << 15 |
	 * stop << 14 | rw << 13 | dtype << 11 | rsptyp << 7 | brk << 6 | opcode
	 */
	rawcmd = opcode | msdc_rsp[resp] << 7 | host->blksz << 16;

	if (opcode == MMC_READ_MULTIPLE_BLOCK) {
		rawcmd |= (2 << 11);
		if (host->autocmd & MSDC_AUTOCMD12)
			rawcmd |= (1 << 28);
	} else if ((opcode == MMC_READ_SINGLE_BLOCK) || (opcode == MMC_SEND_TUNING_BLOCK) || (opcode == MMC_SEND_TUNING_BLOCK_HS200)) {
		rawcmd |= (1 << 11);
	} else if (opcode == MMC_WRITE_MULTIPLE_BLOCK) {
			 if(cmd->data->blocks > 1)
		 rawcmd |= ((2 << 11) | (1 << 13));
		 else
			rawcmd |= ((1 << 11) | (1 << 13));
		if (host->autocmd & MSDC_AUTOCMD12)
			rawcmd |= (1 << 28);
	} else if (opcode == MMC_WRITE_BLOCK) {
		rawcmd |= ((1 << 11) | (1 << 13));
	} else if (opcode == SD_IO_RW_EXTENDED) {
		if (cmd->data->flags & MMC_DATA_WRITE)
			rawcmd |= (1 << 13);
		if (cmd->data->blocks > 1)
			rawcmd |= (2 << 11);
		else
			rawcmd |= (1 << 11);
	} else if (opcode == SD_IO_RW_DIRECT && cmd->flags == (unsigned int)-1) {
		rawcmd |= (1 << 14);
	} else if (opcode == SD_SWITCH_VOLTAGE) {
		rawcmd |= (1 << 30);
	} else if ((opcode == SD_APP_SEND_SCR) ||
		(opcode == SD_APP_SEND_NUM_WR_BLKS) ||
		(opcode == SD_SWITCH && (mmc_cmd_type(cmd) == MMC_CMD_ADTC)) ||
		(opcode == SD_APP_SD_STATUS && (mmc_cmd_type(cmd) == MMC_CMD_ADTC)) ||
		(opcode == MMC_SEND_EXT_CSD && (mmc_cmd_type(cmd) == MMC_CMD_ADTC))) {
		rawcmd |= (1 << 11);
	} else if (opcode == MMC_STOP_TRANSMISSION) {
		rawcmd |= (1 << 14);
		rawcmd &= ~(0x0FFF << 16);
	}

	N_MSG(CMD, "CMD<%d><0x%.8x> Arg<0x%.8x>\n", opcode , rawcmd, cmd->arg);

	tmo = jiffies + timeout;

	if (opcode == MMC_SEND_STATUS) {
		for (;;) {
			if (!sdc_is_cmd_busy())
				break;

			if (time_after(jiffies, tmo)) {
				ERR_MSG("XXX cmd_busy timeout: before CMD<%d>\n", opcode);
				cmd->error = (unsigned int)-ETIMEDOUT;
				msdc_reset_hw();
				return cmd->error;	/* Fix me: error handling */
			}
		}
	} else {
		for (;;) {
			if (!sdc_is_busy())
				break;
			if (time_after(jiffies, tmo)) {
				ERR_MSG("XXX sdc_busy timeout: before CMD<%d>\n", opcode);
				cmd->error = (unsigned int)-ETIMEDOUT;
				msdc_reset_hw();
				return cmd->error;
			}
		}
	}

	host->cmd = cmd;
	host->cmd_rsp = resp;

	if (cmd->opcode == MMC_SEND_STATUS) {
		init_completion(&host->cmd_done);
		sdr_set_bits(MSDC_INTEN, wints_cmd);
	} else {
		/* use polling way */
		sdr_clr_bits(MSDC_INTEN, wints_cmd);
	}
	rawarg = cmd->arg;
#ifdef MTK_EMMC_SUPPORT
		if(host->hw->host_function == MSDC_EMMC &&
			host->hw->boot == MSDC_BOOT_EN &&
			(cmd->opcode == MMC_READ_SINGLE_BLOCK
			|| cmd->opcode == MMC_READ_MULTIPLE_BLOCK
			|| cmd->opcode == MMC_WRITE_BLOCK
			|| cmd->opcode == MMC_WRITE_MULTIPLE_BLOCK
			|| cmd->opcode == MMC_ERASE_GROUP_START
			|| cmd->opcode == MMC_ERASE_GROUP_END)
			&& (partition_access == 0)) {
			rawarg	+= offset;
			if(offset)
				ERR_MSG("retune start off from 0 to 0x%x\n",rawarg);
			if(cmd->opcode == MMC_ERASE_GROUP_START)
				erase_start = rawarg;
			if(cmd->opcode == MMC_ERASE_GROUP_END)
				erase_end = rawarg;
		}
		if(cmd->opcode == MMC_ERASE
			&& (cmd->arg == MMC_SECURE_ERASE_ARG || cmd->arg == MMC_ERASE_ARG)
			&& host->mmc->card
			&& host->hw->host_function == MSDC_EMMC
			&& host->hw->boot == MSDC_BOOT_EN
			&& (!mmc_erase_group_aligned(host->mmc->card,erase_start,erase_end))){
				if(cmd->arg == MMC_SECURE_ERASE_ARG && mmc_can_secure_erase_trim(host->mmc->card))
					rawarg = MMC_SECURE_TRIM1_ARG;
				else if(cmd->arg == MMC_ERASE_ARG
				   ||(cmd->arg == MMC_SECURE_ERASE_ARG && !mmc_can_secure_erase_trim(host->mmc->card)))
					rawarg = MMC_TRIM_ARG;
			}
#endif

	sdc_send_cmd(rawcmd, rawarg);

//end:
	return 0;  // irq too fast, then cmd->error has value, and don't call msdc_command_resp, don't tune.
}

extern int __cmd_tuning_test;
unsigned int msdc_command_resp_polling(struct msdc_host	 *host,
									  struct mmc_command *cmd,
									  int				  tune,
									  unsigned long		  timeout)
{
	u32 __iomem *base = host->base;
	u32 intsts;
	u32 resp;
	unsigned long tmo;
	u32 cmdsts = MSDC_INT_CMDRDY  | MSDC_INT_RSPCRCERR	| MSDC_INT_CMDTMO;

	resp = host->cmd_rsp;

	/*polling*/
	tmo = jiffies + timeout;
	while (1) {
		if (((intsts = sdr_read32(MSDC_INT)) & cmdsts) != 0){
			/* clear all int flag */
			intsts &= cmdsts;
			sdr_write32(MSDC_INT, intsts);
			break;
		}

		if (time_after(jiffies, tmo)) {
			ERR_MSG("XXX CMD<%d> polling_for_completion timeout ARG<0x%.8x>\n",
					cmd->opcode, cmd->arg);
			cmd->error = (unsigned int)-ETIMEDOUT;
			host->sw_timeout++;
			msdc_dump_info(host);
			msdc_reset_hw();
			goto out;
		}
	}

	/* command interrupts */
	if (intsts & cmdsts) {
		if ((intsts & MSDC_INT_CMDRDY) || (intsts & MSDC_INT_ACMDRDY) ||
			(intsts & MSDC_INT_ACMD19_DONE)) {
			u32 *rsp = NULL;
			rsp = &cmd->resp[0];

			if ((-1 == __cmd_tuning_test) &&
				(cmd->opcode == MMC_READ_SINGLE_BLOCK ||
				cmd->opcode == MMC_READ_MULTIPLE_BLOCK ||
				cmd->opcode == MMC_WRITE_MULTIPLE_BLOCK)) {
				cmd->error = (unsigned int)-EIO;
				IRQ_MSG("yf test:fake XXX CMD<%d> MSDC_INT_RSPCRCERR Arg<0x%.8x>\n",
						cmd->opcode, cmd->arg);
				msdc_dump_info(host);
				msdc_reset_hw();
				__cmd_tuning_test = 0;
				goto out;
			}

			switch (host->cmd_rsp) {
			case RESP_NONE:
				break;
			case RESP_R2:
				*rsp++ = sdr_read32(SDC_RESP3);
				*rsp++ = sdr_read32(SDC_RESP2);
				*rsp++ = sdr_read32(SDC_RESP1);
				*rsp++ = sdr_read32(SDC_RESP0);
				break;
			default: /* Response types 1, 3, 4, 5, 6, 7(1b) */
				*rsp = sdr_read32(SDC_RESP0);
				break;
			}
		} else if (intsts & MSDC_INT_RSPCRCERR) {
			cmd->error = (unsigned int)-EIO;
			//IRQ_MSG("XXX CMD<%d> MSDC_INT_RSPCRCERR Arg<0x%.8x>\n", cmd->opcode, cmd->arg);
			/* msdc_dump_info(host); */
			msdc_reset_hw();
		} else if (intsts & MSDC_INT_CMDTMO) {
			cmd->error = (unsigned int)-ETIMEDOUT;
			//IRQ_MSG("XXX CMD<%d> MSDC_INT_CMDTMO Arg<0x%.8x>\n", cmd->opcode, cmd->arg);
			/* msdc_dump_info(host); */
			msdc_reset_hw();
		}
	}

out:
	host->cmd = NULL;

	return cmd->error;
}

static unsigned int msdc_command_resp(struct msdc_host	 *host,
									  struct mmc_command *cmd,
									  int				  tune,
									  unsigned long		  timeout)
{
	u32 __iomem *base = host->base;
	u32 opcode = cmd->opcode;

	spin_unlock(&host->lock);
	if (!wait_for_completion_timeout(&host->cmd_done, 10*timeout)) {
		ERR_MSG("XXX CMD<%d> wait_for_completion timeout ARG<0x%.8x>\n",
				opcode, cmd->arg);
		host->sw_timeout++;
		msdc_dump_info(host);
		cmd->error = (unsigned int)-ETIMEDOUT;
		msdc_reset_hw();
	}
	spin_lock(&host->lock);

	sdr_clr_bits(MSDC_INTEN, wints_cmd);
	host->cmd = NULL;

	return cmd->error;
}

static unsigned int msdc_do_command(struct msdc_host   *host,
									  struct mmc_command *cmd,
									  int				  tune,
									  unsigned long		  timeout)
{
	u32 __iomem *base = host->base;

	// TODO: ??? why delay 10ms????????
	if ((cmd->opcode == MMC_GO_IDLE_STATE) && (host->hw->host_function == MSDC_SD))
		mdelay(10);

	// TODO: ??? WTF
	if ((cmd->opcode == MMC_ALL_SEND_CID) && (host->mclk == 400000))
		sdr_clr_bits(MSDC_PATCH_BIT0, MSDC_PB0_INT_DAT_LATCH_CK_SEL);

	if (msdc_command_start(host, cmd, tune, timeout))
		goto end;

	if (cmd->opcode == MMC_SEND_STATUS){
		if (msdc_command_resp(host, cmd, tune, timeout))
			goto end;
	} else {
		if (msdc_command_resp_polling(host, cmd, tune, timeout))
			goto end;
	}

end:
	N_MSG(CMD, "\t\treturn<%d> resp<0x%.8x>\n", cmd->error, cmd->resp[0]);
	return cmd->error;
}

/* The abort condition when PIO read/write
   tmo:
*/
static int msdc_pio_abort(struct msdc_host *host, struct mmc_data *data, unsigned long tmo)
{
	int ret = 0;
	u32 __iomem *base = host->base;

	if (atomic_read(&host->abort))
		ret = 1;

	if (time_after(jiffies, tmo)) {
		data->error = (unsigned int)-ETIMEDOUT;
		ERR_MSG("XXX PIO Data Timeout: CMD<%d>\n", host->mrq->cmd->opcode);
		msdc_dump_info(host);
		ret = 1;
	}

	if (ret) {
		msdc_reset_hw();
		ERR_MSG("msdc pio find abort\n");
	}

	return ret;
}

/*
   Need to add a timeout, or WDT timeout, system reboot.
*/
// pio mode data read/write
static int msdc_pio_read(struct msdc_host *host, struct mmc_data *data)
{
	struct scatterlist *sg = data->sg;
	u32 __iomem *base = host->base;
	u32  num = data->sg_len;
	u32 *ptr;
	u8	*u8ptr;
	u8	*test;
	u32  left = 0;
	u32  count, size = 0;
	u32  wints = MSDC_INTEN_DATTMO | MSDC_INTEN_DATCRCERR | MSDC_INTEN_XFER_COMPL;
	u32  ints = 0;
	bool get_xfer_done = 0;
	unsigned long tmo = jiffies + DAT_TIMEOUT;

	while (1) {
		if(!get_xfer_done){
			ints = sdr_read32(MSDC_INT);
			ints &= wints;
			sdr_write32(MSDC_INT,ints);
		}
		if(ints & MSDC_INT_DATTMO){
			data->error = (unsigned int)-ETIMEDOUT;
			msdc_reset_hw();
			break;
		} else if(ints & MSDC_INT_DATCRCERR) {
			data->error = (unsigned int)-EIO;
			msdc_reset_hw();
			break;
		} else if(ints & MSDC_INT_XFER_COMPL)
			get_xfer_done = 1;

		if((num == 0) && (left == 0) && get_xfer_done)
			break;

		if(left == 0 && sg){
			left = sg_dma_len(sg);
			ptr = sg_virt(sg);
			test = sg_virt(sg);
		}
		if ((left >=  MSDC_FIFO_THD) && (msdc_rxfifocnt() >= MSDC_FIFO_THD)) {
			 count = MSDC_FIFO_THD >> 2;
			 do {
				*ptr++ = msdc_fifo_read32();
			 } while (--count);
			 left -= MSDC_FIFO_THD;
		} else if ((left < MSDC_FIFO_THD) && msdc_rxfifocnt() >= left) {
			 while (left > 3) {
				*ptr++ = msdc_fifo_read32();
				 left -= 4;
			 }
			 u8ptr = (u8 *)ptr;
			 while(left) {
				 * u8ptr++ = msdc_fifo_read8();
				 left--;
			 }
		}

		if (msdc_pio_abort(host, data, tmo)) {
			 ERR_MSG("PIO read ABORT left 0x%x ,rx_fifocnt %d , int 0x%x num %d \n",left, msdc_rxfifocnt(),sdr_read32(MSDC_INT),num);
			 goto end;
		}

		if(left == 0 && sg){
			size += sg_dma_len(sg);
			sg = sg_next(sg);
			num--;
		}
	}

end:
	data->bytes_xfered += size;

	if(data->error)
		ERR_MSG("read pio data->error<%d> left<%d> size<%d>\n", data->error, left, size);

	return data->error;
}

/* please make sure won't using PIO when size >= 512
   which means, memory card block read/write won't using pio
   then don't need to handle the CMD12 when data error.
*/
static int msdc_pio_write(struct msdc_host* host, struct mmc_data *data)
{
	u32 __iomem *base = host->base;
	struct scatterlist *sg = data->sg;
	u32  num = data->sg_len;
	u32 *ptr;
	u8	*u8ptr;
	u32  left = 0;
	u32  count, size = 0;
	u32  wints = MSDC_INTEN_DATTMO | MSDC_INTEN_DATCRCERR | MSDC_INTEN_XFER_COMPL;
	bool get_xfer_done = 0;
	unsigned long tmo = jiffies + DAT_TIMEOUT;
	u32 ints = 0;

	while (1) {
		if (!get_xfer_done) {
			ints = sdr_read32(MSDC_INT);
			ints &= wints;
			sdr_write32(MSDC_INT,ints);
		}
		if (ints & MSDC_INT_DATTMO) {
			data->error = (unsigned int)-ETIMEDOUT;
			msdc_reset_hw();
			break;
		} else if(ints & MSDC_INT_DATCRCERR) {
			data->error = (unsigned int)-EIO;
			msdc_reset_hw();
			break;
		} else if(ints & MSDC_INT_XFER_COMPL)
			get_xfer_done = 1;

		if ((num == 0) && (left == 0) && get_xfer_done)
			break;

		if (left == 0 && sg) {
			left = sg_dma_len(sg);
			ptr = sg_virt(sg);
		}

		if (left >= MSDC_FIFO_SZ && msdc_txfifocnt() == 0) {
			count = MSDC_FIFO_SZ >> 2;
			do {
				msdc_fifo_write32(*ptr); ptr++;
			} while(--count);
			left -= MSDC_FIFO_SZ;
		} else if (left < MSDC_FIFO_SZ && msdc_txfifocnt() == 0) {
			while(left > 3) {
				msdc_fifo_write32(*ptr); ptr++;
				left -= 4;
			}
			u8ptr = (u8*)ptr;
			while(left) {
				msdc_fifo_write8(*u8ptr);	u8ptr++;
				left--;
			}
		}
		if (msdc_pio_abort(host, data, tmo)){
			ERR_MSG("PIO write ABORT left 0x%x ,rx_fifocnt %d , int 0x%x num %d \n",left, msdc_rxfifocnt(),sdr_read32(MSDC_INT),num);
			goto end;
		}

		if(left == 0 && sg){
			size += sg_dma_len(sg);
			sg = sg_next(sg);
			num--;
		}
	}
end:
	data->bytes_xfered += size;
	if (data->error)
		ERR_MSG("write pio data->error<%d>\n", data->error);

	return data->error;
}

void msdc_dma_start(struct msdc_host *host)
{
	u32 __iomem *base = host->base;
	u32 wints = MSDC_INTEN_XFER_COMPL | MSDC_INTEN_DATTMO | MSDC_INTEN_DATCRCERR;
	if (host->autocmd == MSDC_AUTOCMD12)
		wints |= MSDC_INT_ACMDCRCERR | MSDC_INT_ACMDTMO | MSDC_INT_ACMDRDY;
	sdr_set_bits(MSDC_INTEN, wints);
	mb();
	sdr_set_field(MSDC_DMA_CTRL, MSDC_DMA_CTRL_START, 1);

	N_MSG(DMA, "DMA start\n");
}

void msdc_dma_stop(struct msdc_host *host)
{
	u32 __iomem *base = host->base;
	int retry = 30000;
	int count = 1000;
	//u32 retries=500;
	u32 wints = MSDC_INTEN_XFER_COMPL | MSDC_INTEN_DATTMO | MSDC_INTEN_DATCRCERR ;
	if(host->autocmd == MSDC_AUTOCMD12)
		wints |= MSDC_INT_ACMDCRCERR | MSDC_INT_ACMDTMO | MSDC_INT_ACMDRDY;
	N_MSG(DMA, "DMA status: 0x%.8x\n",sdr_read32(MSDC_DMA_CFG));
	//while (sdr_read32(MSDC_DMA_CFG) & MSDC_DMA_CFG_STS);

	sdr_set_field(MSDC_DMA_CTRL, MSDC_DMA_CTRL_STOP, 1);
	//while (sdr_read32(MSDC_DMA_CFG) & MSDC_DMA_CFG_STS);
	msdc_retry((sdr_read32(MSDC_DMA_CFG) & MSDC_DMA_CFG_STS),retry,count);
	if (retry == 0) {
		ERR_MSG("!!ASSERT!!\n");
		BUG();
	}
	mb();
	sdr_clr_bits(MSDC_INTEN, wints); /* Not just xfer_comp */

	N_MSG(DMA, "DMA stop\n");
}


/* calc checksum */
static u8 msdc_dma_calcs(u8 *buf, u32 len)
{
	u32 i, sum = 0;
	for (i = 0; i < len; i++) {
		sum += buf[i];
	}
	return 0xFF - (u8)sum;
}

/* gpd bd setup + dma registers */
static int msdc_dma_config(struct msdc_host *host, struct msdc_dma *dma)
{
	u32 __iomem *base = host->base;
	u32 sglen = dma->sglen;
	//u32 i, j, num, bdlen, arg, xfersz;
	u32 j, num, bdlen;
	dma_addr_t dma_address;
	u32 dma_len;
	u8	blkpad, dwpad, chksum;
	struct scatterlist *sg = dma->sg;
	gpd_t *gpd;
	bd_t *bd;

	switch (dma->mode) {
	case MSDC_MODE_DMA_BASIC:
		//BUG_ON(dma->xfersz > 65535); //to check, no such limitation here since 5399
		BUG_ON(dma->sglen > 1);
		dma_address = sg_dma_address(sg);

		dma_len = sg_dma_len(sg);
		BUG_ON(dma_len >  512 * 1024);

		if (dma_len > 64 * 1024) {
			//ERR_MSG("yf sg len 0x%x\n", dma_len);
		}
		sdr_write_dma_sa(dma_address);

		sdr_set_field(MSDC_DMA_CTRL, MSDC_DMA_CTRL_LASTBUF, 1);
#if defined(CONFIG_ARCH_MT5399) || defined(CONFIG_ARCH_MT5890) || \
	defined(CONFIG_ARCH_MT5882) || defined(CONFIG_ARCH_MT5883) || \
	defined(CONFIG_ARCH_MT5891)
		/* a change happens for dma xfer size:
			  * a new 32-bit register(0xA8) is used for xfer size configuration instead of 16-bit register(0x98 DMA_CTRL)
			  */
		sdr_write32(MSDC_DMA_LEN, dma_len);
	#else
		sdr_set_field(MSDC_DMA_CTRL, MSDC_DMA_CTRL_XFERSZ, dma_len);
	#endif
		sdr_set_field(MSDC_DMA_CTRL, MSDC_DMA_CTRL_BRUSTSZ, dma->burstsz);
		sdr_set_field(MSDC_DMA_CTRL, MSDC_DMA_CTRL_MODE, 0);
		break;
	case MSDC_MODE_DMA_DESC:
		blkpad = (dma->flags & DMA_FLAG_PAD_BLOCK) ? 1 : 0;
		dwpad  = (dma->flags & DMA_FLAG_PAD_DWORD) ? 1 : 0;
		chksum = (dma->flags & DMA_FLAG_EN_CHKSUM) ? 1 : 0;

		/* calculate the required number of gpd */
		num = (sglen + MAX_BD_PER_GPD - 1) / MAX_BD_PER_GPD;
		BUG_ON(num > 1 );

		gpd = dma->gpd;
		bd	= dma->bd;
		bdlen = sglen;

		/* modify gpd*/
		//gpd->intr = 0;
		gpd->hwo = 1;  /* hw will clear it */
		gpd->bdp = 1;
		gpd->chksum = 0;  /* need to clear first. */
		gpd->chksum = (chksum ? msdc_dma_calcs((u8 *)gpd, 16) : 0);

		/* modify bd*/
		for (j = 0; j < bdlen; j++) {
			dma_address = sg_dma_address(sg);
			dma_len = sg_dma_len(sg);
			msdc_init_bd(&bd[j], blkpad, dwpad, (u32)dma_address, dma_len);

			if (j == bdlen - 1)
				bd[j].eol = 1;		/* the last bd */
			else
				bd[j].eol = 0;

			bd[j].chksum = 0; /* checksume need to clear first */
			bd[j].chksum = (chksum ? msdc_dma_calcs((u8 *)(&bd[j]), 16) : 0);
			sg++;
		}

		dma->used_gpd += 2;
		dma->used_bd += bdlen;

		sdr_set_field(MSDC_DMA_CFG, MSDC_DMA_CFG_DECSEN, chksum);
		sdr_set_field(MSDC_DMA_CTRL, MSDC_DMA_CTRL_BRUSTSZ, dma->burstsz);
		sdr_set_field(MSDC_DMA_CTRL, MSDC_DMA_CTRL_MODE, 1);

		sdr_write_dma_sa(dma->gpd_addr);
		break;

	default:
		break;
	}

	N_MSG(DMA, "DMA_CTRL = 0x%x\n", sdr_read32(MSDC_DMA_CTRL));
	N_MSG(DMA, "DMA_CFG = 0x%x\n", sdr_read32(MSDC_DMA_CFG));
	N_MSG(DMA, "DMA_SA = 0x%llx\n", sdr_read_dma_sa());

	return 0;
}

void msdc_dma_setup(struct msdc_host *host, struct msdc_dma *dma,
	struct scatterlist *sg, unsigned int sglen)
{
	BUG_ON(sglen > MAX_BD_NUM); /* not support currently */

	dma->sg = sg;
	dma->flags = DMA_FLAG_EN_CHKSUM;
	//dma->flags = DMA_FLAG_NONE; /* CHECKME */
	dma->sglen = sglen;
	dma->xfersz = host->xfer_size;
	dma->burstsz = MSDC_BRUST_64B;

	if (sglen == 1 && sg_dma_len(sg) <= MAX_DMA_CNT)
		dma->mode = MSDC_MODE_DMA_BASIC;
	else
		dma->mode = MSDC_MODE_DMA_DESC;

#ifdef CC_MTD_ENCRYPT_SUPPORT
	BUG_ON(sglen > 1);
	dma->mode = MSDC_MODE_DMA_BASIC;
#endif

	N_MSG(DMA, "DMA mode<%d> sglen<%d> xfersz<%d>\n", dma->mode, dma->sglen, dma->xfersz);

	msdc_dma_config(host, dma);
}

/* set block number before send command */
void msdc_set_blknum(struct msdc_host *host, u32 blknum)
{
	u32 __iomem *base = host->base;

	sdr_write32(SDC_BLK_NUM, blknum);
}

static int msdc_do_request(struct mmc_host *mmc, struct mmc_request *mrq)
{
	struct msdc_host *host = mmc_priv(mmc);
	struct mmc_command *cmd;
	struct mmc_data *data;
	u32 __iomem *base = host->base;
	unsigned int left=0;
	int dma = 0, read = 1, dir = DMA_FROM_DEVICE;
	/* Fix the bug of dma_map_sg and dma_unmap_sg not match issue */
	u32 map_sg = 0;
	unsigned long pio_tmo;

	BUG_ON(mmc == NULL);
	BUG_ON(mrq == NULL);

	host->error = 0;
	atomic_set(&host->abort, 0);

	cmd  = mrq->cmd;
	data = mrq->cmd->data;

	/* check msdc is work ok. rule is RX/TX fifocnt must be zero after last request
	 * if find abnormal, try to reset msdc first
	 */
	if (msdc_txfifocnt() || msdc_rxfifocnt()) {
		ERR_MSG("register abnormal,please check!\n");
		msdc_reset_hw();
	}

	if (!data) {
		if (msdc_do_command(host, cmd, 0, CMD_TIMEOUT) != 0)
			goto done;

		if (host->hw->host_function == MSDC_EMMC &&
			cmd->opcode == MMC_ALL_SEND_CID) {
			/* This function can ONLY be invoked
			 * after the CID is retrived
			 */
			msdc_register_ett(host, &cmd->resp[0]);
            #ifdef MSDC_SAVE_AUTOK_INFO
			memcpy((void *)host->szCID, (void *)&cmd->resp[0], sizeof(host->szCID));
			//pr_err("kernel get CID: 0x%x, 0x%x, 0x%x, 0x%x \n",
		    //    cmd->resp[0],cmd->resp[1],cmd->resp[2],cmd->resp[3]);
            #endif
			}

#ifdef CONFIG_MTKEMMC_BOOT
		if (host->hw->host_function == MSDC_EMMC &&
			host->hw->boot == MSDC_BOOT_EN &&
			cmd->opcode == MMC_SWITCH &&
			(((cmd->arg >> 16) & 0xFF) == EXT_CSD_PART_CONFIG)) {
			partition_access = (char)((cmd->arg >> 8) & 0x07);
			ERR_MSG("partition_access is 0x%x\n", partition_access);
		}
#endif
	} else {
		BUG_ON(data->blksz > HOST_MAX_BLKSZ);

		data->error = 0;
		read = data->flags & MMC_DATA_READ ? 1 : 0;
		msdc_latest_operation_type[host->id] = read ? OPER_TYPE_READ : OPER_TYPE_WRITE;
		host->data = data;
		host->xfer_size = data->blocks * data->blksz;
		host->blksz = data->blksz;

		/* deside the transfer mode */
		if (drv_mode[host->id] == MODE_PIO) {
			host->dma_xfer = dma = 0;
			msdc_latest_transfer_mode[host->id] = TRAN_MOD_PIO;
		} else if (drv_mode[host->id] == MODE_DMA) {
			host->dma_xfer = dma = 1;
			msdc_latest_transfer_mode[host->id] = TRAN_MOD_DMA;
		} else if (drv_mode[host->id] == MODE_SIZE_DEP) {
			host->dma_xfer = dma = ((host->xfer_size >= dma_size[host->id]) ? 1 : 0);
			msdc_latest_transfer_mode[host->id] = dma ? TRAN_MOD_DMA: TRAN_MOD_PIO;
		}

		if (read) {
			if ((host->timeout_ns != data->timeout_ns) ||
				(host->timeout_clks != data->timeout_clks)) {
				msdc_set_timeout(host, data->timeout_ns, data->timeout_clks);
			}
		}

		msdc_set_blknum(host, data->blocks);
		//msdc_clr_fifo();	/* no need */
		if(dma)
			sdr_clr_bits(MSDC_CFG, MSDC_CFG_PIO);
		else
			sdr_set_bits(MSDC_CFG, MSDC_CFG_PIO);

		if (dma) {
			u32 sgcurnum;

			msdc_dma_on();	/* enable DMA mode first!! */
			init_completion(&host->xfer_done);

			/* start the command first*/
			host->autocmd = MSDC_AUTOCMD12;

			if(3 == partition_access)
				host->autocmd = 0;

			if (msdc_command_start(host, cmd, 0, CMD_TIMEOUT) != 0)
				goto done;

		#ifdef CC_MTD_ENCRYPT_SUPPORT
			if (((cmd->opcode == MMC_WRITE_BLOCK) || (cmd->opcode == MMC_WRITE_MULTIPLE_BLOCK)) &&
				(host->hw->host_function == MSDC_EMMC)) {
				u32 addr_blk = mmc_card_blockaddr(host->mmc->card) ? (cmd->arg) : (cmd->arg >> 9);

				if (msdc_partition_encrypted(addr_blk, host)) {
					spin_unlock(&host->lock);
					if (msdc_aes_encryption_sg(host->data, host)) {
						ERR_MSG("[1]encryption before write process failed!\n");
						BUG_ON(1);
					}
					spin_lock(&host->lock);
					//ERR_MSG("[1]encryption before write process success!\n");
				}
			}
		#endif
			dir = read ? DMA_FROM_DEVICE : DMA_TO_DEVICE;
			sgcurnum = dma_map_sg(mmc_dev(mmc), data->sg, data->sg_len, dir);
			if (data->sg_len != sgcurnum) {
				ERR_MSG("orig sg num is %d, cur sg num is %d\n", data->sg_len, sgcurnum);
				data->sg_len = sgcurnum;
			}
			map_sg = 1;

			/* then wait command done */
			if (msdc_command_resp_polling(host, cmd, 0, CMD_TIMEOUT) != 0) { //not tuning
				goto stop;
			}

			/* for read, the data coming too fast, then CRC error
			   start DMA no business with CRC. */
			//init_completion(&host->xfer_done);
			msdc_dma_setup(host, &host->dma, data->sg, data->sg_len);
			msdc_dma_start(host);

			spin_unlock(&host->lock);
			if (!wait_for_completion_timeout(&host->xfer_done, DAT_TIMEOUT)) {
				ERR_MSG("XXX CMD<%d> ARG<0x%x> wait xfer_done<%d> timeout!!\n",
					cmd->opcode, cmd->arg, data->blocks * data->blksz);
				host->sw_timeout++;

				msdc_dump_info(host);
				data->error = (unsigned int)-ETIMEDOUT;

				msdc_reset_hw();
			}
			spin_lock(&host->lock);
			msdc_dma_stop(host);
			if ((mrq->data && mrq->data->error) ||
				(host->autocmd == MSDC_AUTOCMD12 && mrq->stop && mrq->stop->error)) {
				msdc_clr_fifo();
				msdc_clr_int();
			}

		} else {
			/* Firstly: send command */
		   host->autocmd = 0;
		   host->dma_xfer = 0;
			if (msdc_do_command(host, cmd, 0, CMD_TIMEOUT) != 0) {
				goto stop;
			}

			/* Secondly: pio data phase */
			sdr_set_field(MSDC_IOCON, MSDC_IOCON_RISCSZ, 0x2);
			if (read) {
				if (msdc_pio_read(host, data)){
					goto stop;	 // need cmd12.
				}
			} else {
				if (msdc_pio_write(host, data)) {
					goto stop;
				}
			}

			/* For write case: make sure contents in fifo flushed to device */
			if (!read) {
				pio_tmo = jiffies + DAT_TIMEOUT;
				while (1) {
					left=msdc_txfifocnt();
					if (left == 0)
						break;

					if (msdc_pio_abort(host, data, pio_tmo)) {
						break;
						/* Fix me: what about if data error, when stop ? how to? */
					}
				}
			} else {
				/* Fix me: read case: need to check CRC error */
			}

			/* For write case: SDCBUSY and Xfer_Comp will assert when DAT0 not busy.
			 * For read case : SDCBUSY and Xfer_Comp will assert when last byte read out from FIFO.
			*/

			/* try not to wait xfer_comp interrupt.
			 * the next command will check SDC_BUSY.
			 * SDC_BUSY means xfer_comp assert
			*/
		} // PIO mode

stop:
		/* Last: stop transfer */
		if (data->stop){
			if (!((cmd->error == 0) && (data->error == 0) && (host->autocmd == MSDC_AUTOCMD12) &&
				(cmd->opcode == MMC_READ_MULTIPLE_BLOCK || cmd->opcode == MMC_WRITE_MULTIPLE_BLOCK))) {
				if (msdc_do_command(host, data->stop, 0, CMD_TIMEOUT) != 0) {
					goto done;
				}
			}
		}
	}

done:

	if (data != NULL) {
		host->data = NULL;
		host->dma_xfer = 0;

		if (dma != 0) {
			msdc_dma_off();
			host->dma.used_bd  = 0;
			host->dma.used_gpd = 0;
			if (map_sg == 1) {
				/*if(data->error == 0){
				int retry = 3000;
				int count = 1000;
				msdc_retry(host->dma.gpd->hwo,retry,count);
				}*/
				dma_unmap_sg(mmc_dev(mmc), data->sg, data->sg_len, dir);
				#ifdef CC_MTD_ENCRYPT_SUPPORT
				 if (((cmd->opcode == MMC_READ_SINGLE_BLOCK) || (cmd->opcode == MMC_READ_MULTIPLE_BLOCK)) &&
					 (host->hw->host_function == MSDC_EMMC)) {
					  u32 addr_blk = mmc_card_blockaddr(host->mmc->card)?(cmd->arg):(cmd->arg>>9);

					if (msdc_partition_encrypted(addr_blk, host)) {
					  spin_unlock(&host->lock);
						if (msdc_aes_decryption_sg(data,host)) {
							ERR_MSG("[1]decryption after read process failed!\n");
							BUG_ON(1);
						}
						spin_lock(&host->lock);

//						ERR_MSG("[1]decryption after read process success!\n");
					 }
				 }
				 #endif
			}
		}
#ifdef CONFIG_MTKEMMC_BOOT
		if (cmd->opcode == MMC_SEND_EXT_CSD)
		{
			msdc_get_data(ext_csd,data);
            #ifdef MSDC_SAVE_AUTOK_INFO

			if (0 == host->error)
			{
				host->u1PartitionCfg = ext_csd[179];
		
			}
            #endif
		}
#else
#ifdef MSDC_SAVE_AUTOK_INFO
	if ((cmd->opcode == MMC_SEND_EXT_CSD) &&
	    (0 == host->error)
	    )
		{

			u8 * p_temp_ext_csd = vmalloc(512);
			BUG_ON(!p_temp_ext_csd);
			msdc_get_data(p_temp_ext_csd,data);
			host->u1PartitionCfg = ext_csd[179];

			vfree(p_temp_ext_csd);
			p_temp_ext_csd = NULL;
		}
#endif
#endif
		host->blksz = 0;


		N_MSG(OPS, "CMD<%d> data<%s %s> blksz<%d> block<%d> error<%d>\n",
			cmd->opcode, (dma? "dma":"pio"),
				(read ? "read ":"write") ,data->blksz, data->blocks, data->error);

		if ((cmd->opcode != 17) && (cmd->opcode != 18) && (cmd->opcode != 24) && (cmd->opcode != 25))
			N_MSG(NRW, "CMD<%3d> arg<0x%8x> Resp<0x%8x> data<%s> size<%d>\n",
				cmd->opcode, cmd->arg, cmd->resp[0],
				(read ? "read ":"write") ,data->blksz * data->blocks);
		else
			N_MSG(RW,  "CMD<%3d> arg<0x%8x> Resp<0x%8x> block<%d>\n",
				cmd->opcode, cmd->arg, cmd->resp[0], data->blocks);
	} else {
		if (cmd->opcode != 13)// by pass CMD13
			N_MSG(NRW, "CMD<%3d> arg<0x%8x> resp<%8x %8x %8x %8x>\n", cmd->opcode, cmd->arg,
				cmd->resp[0],cmd->resp[1], cmd->resp[2], cmd->resp[3]);
	}

	if (mrq->cmd->error == (unsigned int)-EIO) {
		host->error |= REQ_CMD_EIO;
		sdio_tune_flag |= 0x1;

		if( mrq->cmd->opcode == SD_IO_RW_EXTENDED )
			sdio_tune_flag |= 0x1;
	}

	if (mrq->cmd->error == (unsigned int)-ETIMEDOUT)
		host->error |= REQ_CMD_TMO;

	 if (mrq->data && mrq->data->error) {
		host->error |= REQ_DAT_ERR;

		sdio_tune_flag |= 0x10;

		if (mrq->data->flags & MMC_DATA_READ)
		   sdio_tune_flag |= 0x80;
		 else
		   sdio_tune_flag |= 0x40;
	}

	if (mrq->stop && (mrq->stop->error == (unsigned int)-EIO))
		host->error |= REQ_STOP_EIO;

	if (mrq->stop && (mrq->stop->error == (unsigned int)-ETIMEDOUT))
		host->error |= REQ_STOP_TMO;

	return host->error;
}

//#ifndef CONFIG_MTK_USE_NEW_TUNE
#if 1
static int msdc_tune_rw_request(struct mmc_host*mmc, struct mmc_request*mrq)
{
	struct msdc_host *host = mmc_priv(mmc);
	struct mmc_command *cmd;
	struct mmc_data *data;
	u32 __iomem *base = host->base;
	int read = 1, dma = 1;

	BUG_ON(mmc == NULL);
	BUG_ON(mrq == NULL);

	//host->error = 0;
	atomic_set(&host->abort, 0);

	cmd  = mrq->cmd;
	data = mrq->cmd->data;

	/* check msdc is work ok. rule is RX/TX fifocnt must be zero after last request
	 * if find abnormal, try to reset msdc first
	 */
	if (msdc_txfifocnt() || msdc_rxfifocnt()) {
		ERR_MSG("register abnormal,please check!\n");
		msdc_reset_hw();
	}

	BUG_ON(data->blksz > HOST_MAX_BLKSZ);

	data->error = 0;
	read = data->flags & MMC_DATA_READ ? 1 : 0;
	msdc_latest_operation_type[host->id] = read ? OPER_TYPE_READ : OPER_TYPE_WRITE;
	host->data = data;
	host->xfer_size = data->blocks * data->blksz;
	host->blksz = data->blksz;
	host->dma_xfer = 1;

	/* deside the transfer mode */
	/*
	if (drv_mode[host->id] == MODE_PIO) {
		host->dma_xfer = dma = 0;
		msdc_latest_transfer_mode[host->id] = TRAN_MOD_PIO;
	} else if (drv_mode[host->id] == MODE_DMA) {
		host->dma_xfer = dma = 1;
		msdc_latest_transfer_mode[host->id] = TRAN_MOD_DMA;
	} else if (drv_mode[host->id] == MODE_SIZE_DEP) {
		host->dma_xfer = dma = ((host->xfer_size >= dma_size[host->id]) ? 1 : 0);
		msdc_latest_transfer_mode[host->id] = dma ? TRAN_MOD_DMA: TRAN_MOD_PIO;
	}
	*/
	if (read) {
		if ((host->timeout_ns != data->timeout_ns) ||
			(host->timeout_clks != data->timeout_clks)) {
			msdc_set_timeout(host, data->timeout_ns, data->timeout_clks);
		}
	}

	msdc_set_blknum(host, data->blocks);
	//msdc_clr_fifo();	/* no need */
	msdc_dma_on();	/* enable DMA mode first!! */
	init_completion(&host->xfer_done);

	/* start the command first*/
	host->autocmd = MSDC_AUTOCMD12;
	if (msdc_command_start(host, cmd, 0, CMD_TIMEOUT) != 0)
		goto done;

	/* then wait command done */
	if (msdc_command_resp_polling(host, cmd, 0, CMD_TIMEOUT) != 0) { //not tuning
		ERR_MSG("XXX CMD<%d> ARG<0x%x> polling cmd resp <%d> timeout!!\n",
			cmd->opcode, cmd->arg, data->blocks * data->blksz);
		goto stop;
	}

	/* for read, the data coming too fast, then CRC error
		 start DMA no business with CRC. */
	msdc_dma_setup(host, &host->dma, data->sg, data->sg_len);
	msdc_dma_start(host);

	spin_unlock(&host->lock);
	if (!wait_for_completion_timeout(&host->xfer_done, DAT_TIMEOUT)) {
		ERR_MSG("XXX CMD<%d> ARG<0x%x> wait xfer_done<%d> timeout!!\n",
			cmd->opcode, cmd->arg, data->blocks * data->blksz);
		host->sw_timeout++;
		data->error = (unsigned int)-ETIMEDOUT;
		msdc_reset_hw();
	}
	spin_lock(&host->lock);

	msdc_dma_stop(host);
	if ((mrq->data && mrq->data->error) ||
		(host->autocmd == MSDC_AUTOCMD12 && mrq->stop && mrq->stop->error)) {
		msdc_clr_fifo();
		msdc_clr_int();
	}

stop:
	/* Last: stop transfer */
	if (data->stop) {
		if (!((cmd->error == 0) && (data->error == 0) && (host->autocmd == MSDC_AUTOCMD12) &&
			(cmd->opcode == MMC_READ_MULTIPLE_BLOCK || cmd->opcode == MMC_WRITE_MULTIPLE_BLOCK))) {
			if (msdc_do_command(host, data->stop, 0, CMD_TIMEOUT) != 0)
				goto done;
		}
	}

done:
	host->data = NULL;
	host->dma_xfer = 0;
	msdc_dma_off();
	host->dma.used_bd  = 0;
	host->dma.used_gpd = 0;
	host->blksz = 0;

	N_MSG(OPS, "CMD<%d> data<%s %s> blksz<%d> block<%d> error<%d>",
		cmd->opcode, (dma? "dma":"pio"),
			(read ? "read ":"write") ,data->blksz, data->blocks, data->error);

	if ((cmd->opcode != 17) && (cmd->opcode != 18) && (cmd->opcode != 24) && (cmd->opcode != 25))
		N_MSG(NRW, "CMD<%3d> arg<0x%8x> Resp<0x%8x> data<%s> size<%d>",
			cmd->opcode, cmd->arg, cmd->resp[0],
			(read ? "read ":"write") ,data->blksz * data->blocks);
	else
		N_MSG(RW,  "CMD<%3d> arg<0x%8x> Resp<0x%8x> block<%d>",
			cmd->opcode, cmd->arg, cmd->resp[0], data->blocks);
	host->error = 0;
	if (mrq->cmd->error == (unsigned int)-EIO)
		host->error |= REQ_CMD_EIO;
	if (mrq->cmd->error == (unsigned int)-ETIMEDOUT)
		host->error |= REQ_CMD_TMO;
	if (mrq->data && (mrq->data->error))
		host->error |= REQ_DAT_ERR;
	if (mrq->stop && (mrq->stop->error == (unsigned int)-EIO))
		host->error |= REQ_STOP_EIO;
	if (mrq->stop && (mrq->stop->error == (unsigned int)-ETIMEDOUT))
		host->error |= REQ_STOP_TMO;

	return host->error;
}
#endif

static void msdc_pre_req(struct mmc_host *mmc, struct mmc_request *mrq,
				   bool is_first_req)
{
	struct msdc_host *host = mmc_priv(mmc);
	struct mmc_data *data;
	struct mmc_command *cmd = mrq->cmd;
	int read = 1, dir = DMA_FROM_DEVICE;
	u32 __iomem *base = host->base;

	BUG_ON(!cmd);
	data = mrq->data;
	if (data)
		data->host_cookie = 0;
	if (data && (cmd->opcode == MMC_READ_SINGLE_BLOCK ||
				cmd->opcode == MMC_READ_MULTIPLE_BLOCK ||
				cmd->opcode == MMC_WRITE_BLOCK ||
				cmd->opcode == MMC_WRITE_MULTIPLE_BLOCK)) {
		host->xfer_size = data->blocks * data->blksz;
		read = data->flags & MMC_DATA_READ ? 1 : 0;
		if (drv_mode[host->id] == MODE_PIO) {
			data->host_cookie = 0;
			msdc_latest_transfer_mode[host->id] = TRAN_MOD_PIO;
		} else if (drv_mode[host->id] == MODE_DMA) {
			data->host_cookie = 1;
			msdc_latest_transfer_mode[host->id] = TRAN_MOD_DMA;
		} else if (drv_mode[host->id] == MODE_SIZE_DEP) {
			data->host_cookie = ((host->xfer_size >= dma_size[host->id]) ? 1 : 0);
			msdc_latest_transfer_mode[host->id] = data->host_cookie ? TRAN_MOD_DMA: TRAN_MOD_PIO;
		}
		if (data->host_cookie) {
			u32 sgcurnum;
		#ifdef CC_MTD_ENCRYPT_SUPPORT
			if (((cmd->opcode == MMC_WRITE_BLOCK) ||
				(cmd->opcode == MMC_WRITE_MULTIPLE_BLOCK)) &&
				(host->hw->host_function == MSDC_EMMC)) {
				u32 addr_blk = mmc_card_blockaddr(host->mmc->card)?(cmd->arg):(cmd->arg>>9);

				if (msdc_partition_encrypted(addr_blk, host)) {
					if (msdc_aes_encryption_sg(data,host)) {
						ERR_MSG("[1]encryption before write process failed!\n");
						BUG_ON(1);
					}
					//ERR_MSG("[1]encryption before write process success!\n");
				}
			}
		#endif
			dir = read ? DMA_FROM_DEVICE : DMA_TO_DEVICE;
			sgcurnum = dma_map_sg(mmc_dev(mmc), data->sg, data->sg_len, dir);
			if (data->sg_len != sgcurnum) {
				ERR_MSG("orig sg num is %d, cur sg num is %d\n", data->sg_len, sgcurnum);
				data->sg_len = sgcurnum;
			}
		}
		N_MSG(OPS, "CMD<%d> ARG<0x%x>data<%s %s> blksz<%d> block<%d> error<%d>\n",
			mrq->cmd->opcode,mrq->cmd->arg, (data->host_cookie ? "dma":"pio"),
			(read ? "read ":"write") ,data->blksz, data->blocks, data->error);
	}

	return;
}
void msdc_dma_clear(struct msdc_host *host)
{
		u32 __iomem *base = host->base;
		host->data = NULL;
		N_MSG(DMA, "dma clear\n");
		host->mrq = NULL;
		host->dma_xfer = 0;
		msdc_dma_off();
		host->dma.used_bd  = 0;
		host->dma.used_gpd = 0;
		host->blksz = 0;
}
static void msdc_post_req(struct mmc_host *mmc, struct mmc_request *mrq, int err)
{
	struct msdc_host *host = mmc_priv(mmc);
	struct mmc_data *data;
	//struct mmc_command *cmd = mrq->cmd;
	int  read = 1, dir = DMA_FROM_DEVICE;
	data = mrq->data;
	if(data && data->host_cookie){
		host->xfer_size = data->blocks * data->blksz;
		read = data->flags & MMC_DATA_READ ? 1 : 0;
		dir = read ? DMA_FROM_DEVICE : DMA_TO_DEVICE;
		dma_unmap_sg(mmc_dev(mmc), data->sg, data->sg_len, dir);
		#ifdef CC_MTD_ENCRYPT_SUPPORT
		  if(((mrq->cmd->opcode == MMC_READ_SINGLE_BLOCK) ||
			 (mrq->cmd->opcode == MMC_READ_MULTIPLE_BLOCK)) && (host->hw->host_function == MSDC_EMMC))
		  {
			  u32 addr_blk = mmc_card_blockaddr(host->mmc->card)?(mrq->cmd->arg):(mrq->cmd->arg>>9);

			  if(msdc_partition_encrypted(addr_blk, host))
			  {
				if (msdc_aes_decryption_sg(data,host))
				{
					ERR_MSG("[1]decryption after read process failed!\n");
					BUG_ON(1);
				}
			  }
		  }
		 #endif
		data->host_cookie = 0;
		N_MSG(OPS, "CMD<%d> ARG<0x%x> blksz<%d> block<%d> error<%d>\n",mrq->cmd->opcode,mrq->cmd->arg,
				data->blksz, data->blocks, data->error);
	}
	return;

}

static int msdc_do_request_async(struct mmc_host*mmc, struct mmc_request*mrq)
{
	struct msdc_host *host = mmc_priv(mmc);
	struct mmc_command *cmd;
	struct mmc_data *data;
	u32 __iomem *base = host->base;
	//u32 intsts = 0;
	  //unsigned int left=0;
	int read = 1;//, dir = DMA_FROM_DEVICE;
	//u32 map_sg = 0;  /* Fix the bug of dma_map_sg and dma_unmap_sg not match issue */
	//u32 bus_mode = 0;

	BUG_ON(mmc == NULL);
	BUG_ON(mrq == NULL);
	if (!is_card_present(host) || host->power_mode == MMC_POWER_OFF) {
		ERR_MSG("cmd<%d> arg<0x%x> card<%d> power<%d>\n",
			mrq->cmd->opcode, mrq->cmd->arg, is_card_present(host), host->power_mode);
		mrq->cmd->error = (unsigned int)-ENOMEDIUM;
		if(mrq->done)
			mrq->done(mrq);			// call done directly.
		return 0;
	}

	host->tune = 0;

	host->error = 0;
	atomic_set(&host->abort, 0);
	spin_lock(&host->lock);
	cmd  = mrq->cmd;
	data = mrq->cmd->data;
	host->mrq = mrq;
	/* check msdc is work ok. rule is RX/TX fifocnt must be zero after last request
	 * if find abnormal, try to reset msdc first
	 */
	if (msdc_txfifocnt() || msdc_rxfifocnt()) {
		ERR_MSG("register abnormal,please check!\n");
		msdc_reset_hw();
	}

	BUG_ON(data->blksz > HOST_MAX_BLKSZ);

	data->error = 0;
	read = data->flags & MMC_DATA_READ ? 1 : 0;
	msdc_latest_operation_type[host->id] = read ? OPER_TYPE_READ : OPER_TYPE_WRITE;
	host->data = data;
	host->xfer_size = data->blocks * data->blksz;
	host->blksz = data->blksz;
	host->dma_xfer = 1;
	/* deside the transfer mode */

	if (read) {
		if ((host->timeout_ns != data->timeout_ns) ||
			(host->timeout_clks != data->timeout_clks)) {
			msdc_set_timeout(host, data->timeout_ns, data->timeout_clks);
		}
	}

	msdc_set_blknum(host, data->blocks);
	//msdc_clr_fifo();	/* no need */
	msdc_dma_on();	/* enable DMA mode first!! */
	//init_completion(&host->xfer_done);

	/* start the command first*/
	host->autocmd = MSDC_AUTOCMD12;

	if(3 == partition_access)
	{
		host->autocmd = 0;
	}

	if (msdc_command_start(host, cmd, 0, CMD_TIMEOUT) != 0)
		goto done;

	/* then wait command done */
	if (msdc_command_resp_polling(host, cmd, 0, CMD_TIMEOUT) != 0) { // not tuning.
		goto stop;
	}

	/* for read, the data coming too fast, then CRC error
	   start DMA no business with CRC. */
	//init_completion(&host->xfer_done);
	msdc_dma_setup(host, &host->dma, data->sg, data->sg_len);
	msdc_dma_start(host);

	spin_unlock(&host->lock);
return 0;

stop:
	/* Last: stop transfer */
	if (data && data->stop){
		if (!((cmd->error == 0) && (data->error == 0) && (host->autocmd == MSDC_AUTOCMD12) &&
					(cmd->opcode == MMC_READ_MULTIPLE_BLOCK || cmd->opcode == MMC_WRITE_MULTIPLE_BLOCK))) {
			if (msdc_do_command(host, data->stop, 0, CMD_TIMEOUT) != 0)
				goto done;
			}
	}

done:
	msdc_dma_clear(host);

	N_MSG(OPS, "CMD<%d> data<%s %s> blksz<%d> block<%d> error<%d>\n",
		cmd->opcode, "dma", (read ? "read ":"write"),
		data->blksz, data->blocks, data->error);

		if ((cmd->opcode != MMC_READ_SINGLE_BLOCK) && (cmd->opcode != MMC_READ_MULTIPLE_BLOCK) &&
			(cmd->opcode != MMC_WRITE_BLOCK) && (cmd->opcode != MMC_WRITE_MULTIPLE_BLOCK))
			N_MSG(NRW, "CMD<%3d> arg<0x%8x> Resp<0x%8x> data<%s> size<%d>\n",
				cmd->opcode, cmd->arg, cmd->resp[0],
				(read ? "read ":"write") ,data->blksz * data->blocks);
		else
			N_MSG(RW,  "CMD<%3d> arg<0x%8x> Resp<0x%8x> block<%d>\n", cmd->opcode,
				cmd->arg, cmd->resp[0], data->blocks);

	if (mrq->cmd->error == (unsigned int)-EIO)
		host->error |= REQ_CMD_EIO;
	if (mrq->cmd->error == (unsigned int)-ETIMEDOUT)
		host->error |= REQ_CMD_TMO;
	if (mrq->stop && (mrq->stop->error == (unsigned int)-EIO))
		host->error |= REQ_STOP_EIO;
	if (mrq->stop && (mrq->stop->error == (unsigned int)-ETIMEDOUT))
		host->error |= REQ_STOP_TMO;
	if(mrq->done)
		mrq->done(mrq);

	spin_unlock(&host->lock);
	return host->error;
}

static int msdc_app_cmd(struct mmc_host *mmc, struct msdc_host *host)
{
	struct mmc_command cmd = {0};
	struct mmc_request mrq = {0};
	u32 err = -1;

	cmd.opcode = MMC_APP_CMD;
	cmd.arg = host->app_cmd_arg;  /* meet mmc->card is null when ACMD6 */
	cmd.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_AC;

	mrq.cmd = &cmd; cmd.mrq = &mrq;
	cmd.data = NULL;

	err = msdc_do_command(host, &cmd, 0, CMD_TIMEOUT);
	return err;
}

static int msdc_lower_freq(struct msdc_host *host) //to check
{
	u32 div, mode;
	u32 __iomem *base = host->base;
	u32 hs400_mode = 0;

	msdc_reset_tune_counter(host,all_counter);
	sdr_get_field(MSDC_CFG, MSDC_CFG_CKMOD, mode);
	sdr_get_field(MSDC_CFG, MSDC_CFG_CKDIV, div);

	if (mode == 3) {
		sdr_get_field(MSDC_CFG, MSDC_CFG_CKMOD_HS400, hs400_mode);
		if (hs400_mode)
			/* can not divide clock on 400M mode, need to switch to 800M mode */
			hs400_mode = 0;
		else
			div++;

		msdc_clk_stable(host, mode, div, hs400_mode);
	} else if(mode == 1){
		mode = 0;
		msdc_clk_stable(host, mode, div, 0);
#ifdef MSDC_ONLY_USE_ONE_CLKSRC
		host->sclk = (div == 0) ?
			(msdc_cur_sclk_freq(host) >> 1) :
			msdc_cur_sclk_freq(host) / (div << 2);
#else
		host->sclk = (div == 0) ?
			(msdcClk[IDX_CLK_TARGET][host->sclk_src] >> 1) :
			msdcClk[IDX_CLK_TARGET][host->sclk_src] / (div << 2);
#endif
	} else {
		msdc_clk_stable(host, mode, div + 1, 0);
#ifdef MSDC_ONLY_USE_ONE_CLKSRC
		host->sclk = (mode == 2) ?
			msdc_cur_sclk_freq(host) / ((div + 1) << 3) :
			msdc_cur_sclk_freq(host) / ((div + 1) << 2);
#else
		host->sclk = (mode == 2) ?
			msdcClk[IDX_CLK_TARGET][host->sclk_src] / ((div + 1) << 3) :
			msdcClk[IDX_CLK_TARGET][host->sclk_src] / ((div + 1) << 2);
#endif
	}

	ERR_MSG("Try lower frequency, new mode<%d> div<%d> hs400mode<%d> freq<%dKHz>\n",
		mode, div, hs400_mode, host->sclk / 1000);

		return 0;
	}

static int MsdcKernelTuneSampleEdge(struct msdc_host *host,bool fgCmd, bool fgRead, bool fgWrite)
{
	int u4OldVal = 0;
	int u4NewVal = 0;
	u32 __iomem *base = host->base;


	if (fgCmd)//0X04[1]
	{
		sdr_get_field(MSDC_IOCON, MSDC_IOCON_RSPL, u4OldVal);
		u4NewVal = u4OldVal ^ 0x1;
		sdr_set_field(MSDC_IOCON, MSDC_IOCON_RSPL, u4NewVal);
		pr_err("Tune AutoK CMD sampel edge %d --> %d \n",u4OldVal,u4NewVal);

	}
	else if (fgRead)
	{
		if (1) //emmc  [2]
		{
			sdr_get_field(MSDC_IOCON, MSDC_IOCON_R_D_SMPL, u4OldVal);
			u4NewVal = u4OldVal ^ 0x1;
			sdr_set_field(MSDC_IOCON, MSDC_IOCON_R_D_SMPL, u4NewVal);
			//pr_err("EMMC Tune AutoK READ sampel edge %d --> %d \n",u4OldVal,u4NewVal);
		}
		else  //SD
		{
			sdr_set_field(MSDC_IOCON, MSDC_IOCON_R_D_SMPL, 0);

			sdr_get_field(MSDC_PATCH_BIT0, MSDC_PB0_RD_DAT_SEL, u4OldVal);
			u4NewVal = u4OldVal ^ 0x1;
			sdr_set_field(MSDC_PATCH_BIT0, MSDC_PB0_RD_DAT_SEL, u4NewVal);
			pr_err("SD Tune AutoK READ sampel edge %d --> %d \n",u4OldVal,u4NewVal);
		}


	}
	else if (fgWrite)
	{
		sdr_get_field(MSDC_PATCH_BIT2, (0x1  << 25)/*MSDC_BIT2_CFG_CRCSTS_EDGE*/, u4OldVal);
		u4NewVal = u4OldVal ^ 0x1;
		sdr_set_field(MSDC_PATCH_BIT2, (0x1  << 25)/*MSDC_BIT2_CFG_CRCSTS_EDGE*/, u4NewVal);
		pr_err("Tune AutoK WRITE sampel edge %d --> %d \n",u4OldVal,u4NewVal);

	}

}


static int MsdcKernelOnlineAutoKTune(struct msdc_host *host,bool fgCmd, bool fgRead, bool fgWrite)
{
    //pr_err("Enter MsdcKernelOnlineAutoKTune...timing %d..\n",host->mmc->ios.timing);

	if (host->hw->host_function == MSDC_EMMC)
	{
		if (((host->mclk <= 50000000)) ||
			(!(g_autok_cmdline_parsed & MSDC_AUTOK_EXEC_DONE))//before we really change to HS200/HS400, we still use HS mode tuning
			)
		{
		   return MsdcKernelTuneSampleEdge(host,fgCmd, fgRead, fgWrite);
		}
		else if (host->mmc->ios.timing == MMC_TIMING_MMC_HS200)
		{
			pr_err("ERROR! Kernel error in HS200 ,Should not go here \n");
			return hs200_execute_tuning(host, NULL);
		}
		else if (host->mmc->ios.timing == MMC_TIMING_MMC_HS400)
		{
			pr_err("ERROR! Kernel error in HS400 ,Should not go here \n");
			return hs400_execute_tuning(host, NULL);
	
		}
		else
		{
			pr_err("ERROR! The current speed mode invalid ,timing %d \n",host->mmc->ios.timing);
			return -1;
		}
	}
	else if (host->hw->host_function == MSDC_SD)
	{
		pr_err("ERROR! Tuning by run SD Autok again.clk=%d \n",host->mclk);
		if (host->mclk <= 50000000)
		{
			return MsdcKernelTuneSampleEdge(host,fgCmd, fgRead, fgWrite);
		}
		else
		{
		    return autok_execute_tuning(host, NULL);
		}
	}


}

static int msdc_tune_cmdrsp(struct msdc_host *host)
{
	int result = 0;
	u32 __iomem *base = host->base;
	u32 sel = 0;
	u32 cur_rsmpl = 0, orig_rsmpl = 0;
	u32 cur_rrdly_pad = 0, orig_rrdly_pad = 0;
	u32 cur_dl_cksel = 0, orig_dl_cksel = 0;
	u32 cur_ckgen = 0, orig_ckgen = 0;
	u32 cur_respdly_internal = 0, orig_respdly_internal = 0;

	#ifdef CONFIG_MTK_USE_NEW_TUNE
	MsdcKernelOnlineAutoKTune(host,true,false,false);
	host->rwcmd_time_tune++;
	return 0;
	#endif
	
	sdr_get_field(MSDC_IOCON, MSDC_IOCON_RSPL, orig_rsmpl);
	sdr_get_field(MSDC_PAD_TUNE0, MSDC_PAD_TUNE0_CMDRDLY, orig_rrdly_pad);
	sdr_get_field(MSDC_PATCH_BIT0, MSDC_PB0_INT_DAT_LATCH_CK_SEL, orig_dl_cksel);
	sdr_get_field(MSDC_PATCH_BIT0, MSDC_PB0_CKGEN_MSDC_DLY_SEL, orig_ckgen);
	sdr_get_field(MSDC_PAD_TUNE0, MSDC_PAD_TUNE0_CMDRRDLY, orig_respdly_internal);

	cur_rsmpl = orig_rsmpl;
	cur_rrdly_pad = orig_rrdly_pad;
	cur_dl_cksel = orig_dl_cksel;
	cur_ckgen = orig_ckgen;
	cur_respdly_internal = orig_respdly_internal;
	pr_err("========msdc_tune_cmdrsp========\n");

	if (host->mclk >= 100000000){
		sel = 1;
		//sdr_set_field(MSDC_PATCH_BIT0, MSDC_CKGEN_RX_SDCLKO_SEL,0);
	} else {
		sdr_set_field(MSDC_PATCH_BIT1,MSDC_PB1_CMD_RSP_TA_CNTR,1);//to check cmd resp turn around
		//sdr_set_field(MSDC_PATCH_BIT0, MSDC_CKGEN_RX_SDCLKO_SEL,1);
		sdr_set_field(MSDC_PATCH_BIT0, MSDC_PB0_INT_DAT_LATCH_CK_SEL,0);
	}

	cur_rsmpl = (orig_rsmpl + 1);
	sdr_set_field(MSDC_IOCON, MSDC_IOCON_RSPL, cur_rsmpl & 1);
	if (host->mclk <= 400000){//In sd/emmc init flow, fix rising edge for latching cmd response
			sdr_set_field(MSDC_IOCON, MSDC_IOCON_RSPL, 0);
			cur_rsmpl = 2;
		}
	if(cur_rsmpl >= 2){
		//rollback
		cur_rsmpl = 0;
		sdr_set_field(MSDC_IOCON, MSDC_IOCON_RSPL, cur_rsmpl & 1);
		cur_rrdly_pad = (orig_rrdly_pad + 1);
		sdr_set_field(MSDC_PAD_TUNE0, MSDC_PAD_TUNE0_CMDRDLY, cur_rrdly_pad & 31);
	}

	//internel delay
	if (cur_rrdly_pad >= 32 ) {
		//rollback
		cur_rrdly_pad = 0;
		sdr_set_field(MSDC_PAD_TUNE0, MSDC_PAD_TUNE0_CMDRDLY, cur_rrdly_pad & 31);
		
		cur_respdly_internal = (orig_respdly_internal + 1) ;
		sdr_set_field(MSDC_PAD_TUNE0, MSDC_PAD_TUNE0_CMDRRDLY, cur_respdly_internal & 31);
		}

	//ckgen
	if (cur_respdly_internal >= 32) {
		cur_respdly_internal = 0 ;
		sdr_set_field(MSDC_PAD_TUNE0, MSDC_PAD_TUNE0_CMDRRDLY, cur_respdly_internal & 31);
		
		cur_ckgen = (orig_ckgen + 1) ;
		sdr_set_field(MSDC_PATCH_BIT0, MSDC_PB0_CKGEN_MSDC_DLY_SEL, cur_ckgen & 31);
		}

	//latch_clock,for ckgen, only tune to 10
	if (cur_ckgen >= MAX_CKGEN_TUNE_TIME && sel) {
		//rollback
		cur_ckgen = 0 ;
		sdr_set_field(MSDC_PATCH_BIT0, MSDC_PB0_CKGEN_MSDC_DLY_SEL, cur_ckgen & 31);
		
		cur_dl_cksel = (orig_dl_cksel +1);
		sdr_set_field(MSDC_PATCH_BIT0, MSDC_PB0_INT_DAT_LATCH_CK_SEL, cur_dl_cksel & 7);
		}
	

	#if 1
	//overflow, all those proper setttings can not cover the error, we exit
	if (cur_dl_cksel > 7)
	{
		pr_err("Warning! CMD TUNE: alrady tune too many times, go lower freq \n");
		cur_dl_cksel = 0;
		sdr_set_field(MSDC_PATCH_BIT0, MSDC_PB0_INT_DAT_LATCH_CK_SEL, cur_dl_cksel & 7);
		
        #ifdef MSDC_LOWER_FREQ
        	result = msdc_lower_freq(host);
        #else
        	result = 1;
        #endif
        	host->t_counter.time_cmd = 0;
	}
	++(host->t_counter.time_cmd);

	#else
	if ((sel && host->t_counter.time_cmd == CMD_TUNE_UHS_MAX_TIME) ||
		(sel == 0 && host->t_counter.time_cmd == CMD_TUNE_HS_MAX_TIME)) {
#ifdef MSDC_LOWER_FREQ
		result = msdc_lower_freq(host);
#else
		result = 1;
#endif
		host->t_counter.time_cmd = 0;
	}

	sdr_get_field(MSDC_IOCON, MSDC_IOCON_RSPL, orig_rsmpl);
	sdr_get_field(MSDC_PAD_TUNE0, MSDC_PAD_TUNE0_CMDRDLY, orig_rrdly);
	sdr_get_field(MSDC_PATCH_BIT0, MSDC_PB0_INT_DAT_LATCH_CK_SEL, orig_dl_cksel);
	
	#endif
	host->rwcmd_time_tune++;
	pr_err("TUNE_CMD: rsmpl<%d> pad_delay<%d> internal_delay<%d> ckgen<%d>,latch_ck<%d> sfreq.<%d>\n",
		cur_rsmpl, cur_rrdly_pad, cur_respdly_internal, cur_ckgen,cur_dl_cksel, host->sclk);

	return result;
}

static int msdc_tune_read(struct msdc_host *host)
{
	u32 __iomem *base = host->base;
	u32 sel = 0;
	u32 ddr = 0;
	u32 dcrc;
	u32 clkmode = 0;
	u32 cur_rxdly0, cur_rxdly1;
	u32 cur_dsmpl = 0, orig_dsmpl;
	u32 cur_ckgen = 0,orig_ckgen;
	u32 cur_dl_cksel = 0,orig_dl_cksel;
	u32 cur_dat0 = 0, cur_dat1 = 0, cur_dat2 = 0, cur_dat3 = 0,
	cur_dat4 = 0, cur_dat5 = 0, cur_dat6 = 0, cur_dat7 = 0;
	u32 orig_dat0, orig_dat1, orig_dat2, orig_dat3, orig_dat4, orig_dat5, orig_dat6, orig_dat7;
	u32 cur_rd_pad_dly = 0,orig_rd_pad_dly = 0;
	int result = 0;

#ifdef CONFIG_MTK_USE_NEW_TUNE
	MsdcKernelOnlineAutoKTune(host,false,true,false);
	host->read_time_tune++;
	return 0;
#endif
	pr_err("========msdc_tune_read========\n");

		if(3 == partition_access)
		return 1;

	if (host->mclk >= 100000000)
		sel = 1;
	else
		sdr_set_field(MSDC_PATCH_BIT0, MSDC_PB0_CKGEN_MSDC_DLY_SEL, 0);

	sdr_get_field(MSDC_CFG,MSDC_CFG_CKMOD,clkmode);
	ddr = (clkmode == 2) ? 1 : 0;

	sdr_get_field(MSDC_PATCH_BIT0, MSDC_PB0_CKGEN_MSDC_DLY_SEL, orig_ckgen);
	sdr_get_field(MSDC_PATCH_BIT0, MSDC_PB0_INT_DAT_LATCH_CK_SEL, orig_dl_cksel);
	sdr_get_field(MSDC_IOCON, MSDC_IOCON_R_D_SMPL, orig_dsmpl);
	sdr_get_field(MSDC_PAD_TUNE0, MSDC_PAD_TUNE0_DATRRDLY, orig_rd_pad_dly);
	cur_ckgen = orig_ckgen;
	cur_dl_cksel = orig_dl_cksel;
	cur_dsmpl = orig_dsmpl;
	cur_rd_pad_dly = orig_rd_pad_dly;

	cur_dsmpl = (orig_dsmpl + 1) ;
	sdr_set_field(MSDC_IOCON, MSDC_IOCON_R_D_SMPL, cur_dsmpl & 1);
	#if 1
	//pad delay
	if(cur_dsmpl >= 2){
		//rollback
		cur_dsmpl = 0;
		sdr_set_field(MSDC_IOCON, MSDC_IOCON_R_D_SMPL, cur_dsmpl & 1);
		cur_rd_pad_dly = (orig_rd_pad_dly + 1);
		sdr_set_field(MSDC_PAD_TUNE0, MSDC_PAD_TUNE0_DATRRDLY, cur_rd_pad_dly & 31);
	}

	//ckgen
	if (cur_rd_pad_dly >= 32) {
		cur_rd_pad_dly = 0;
		sdr_set_field(MSDC_PAD_TUNE0, MSDC_PAD_TUNE0_DATRRDLY, cur_rd_pad_dly & 31);
		
		cur_ckgen = (orig_ckgen + 1) ;
		sdr_set_field(MSDC_PATCH_BIT0, MSDC_PB0_CKGEN_MSDC_DLY_SEL, cur_ckgen & 31);
		}

	//latch_clock,for ckgen, only tune to 10
	if (cur_ckgen >= MAX_CKGEN_TUNE_TIME && sel) {
		//rollback
		cur_ckgen = 0 ;
		sdr_set_field(MSDC_PATCH_BIT0, MSDC_PB0_CKGEN_MSDC_DLY_SEL, cur_ckgen & 31);
		
		cur_dl_cksel = (orig_dl_cksel +1);
		sdr_set_field(MSDC_PATCH_BIT0, MSDC_PB0_INT_DAT_LATCH_CK_SEL, cur_dl_cksel & 7);
		}

	
	//overflow, all those proper setttings can not cover the error, we exit
	if (cur_dl_cksel > 7)
	{
		pr_err("Warning! READ TUNE: alrady tune too many times, go lower freq \n");
		
		cur_dl_cksel = 0;
		sdr_set_field(MSDC_PATCH_BIT0, MSDC_PB0_INT_DAT_LATCH_CK_SEL, cur_dl_cksel & 7);
		
        #ifdef MSDC_LOWER_FREQ
        	result = msdc_lower_freq(host);
        #else
        	result = 1;
        #endif
        	host->t_counter.time_read = 0;
	}
	
	++(host->t_counter.time_read);

	#else
	

	sdr_set_field(MSDC_IOCON, MSDC_IOCON_DDLSEL, 1);

	if (cur_dsmpl >= 2) {
		sdr_get_field(SDC_DCRC_STS, SDC_DCRC_STS_POS | SDC_DCRC_STS_NEG, dcrc);
		if (!ddr)
			dcrc &= ~SDC_DCRC_STS_NEG;
		cur_rxdly0 = sdr_read32(MSDC_DAT_RDDLY0);
		cur_rxdly1 = sdr_read32(MSDC_DAT_RDDLY1);

		orig_dat0 = (cur_rxdly0 >> 24) & 0x1F;
		orig_dat1 = (cur_rxdly0 >> 16) & 0x1F;
		orig_dat2 = (cur_rxdly0 >>	8) & 0x1F;
		orig_dat3 = (cur_rxdly0 >>	0) & 0x1F;
		orig_dat4 = (cur_rxdly1 >> 24) & 0x1F;
		orig_dat5 = (cur_rxdly1 >> 16) & 0x1F;
		orig_dat6 = (cur_rxdly1 >>	8) & 0x1F;
		orig_dat7 = (cur_rxdly1 >>	0) & 0x1F;

		if (ddr) {
			cur_dat0 = (dcrc & (1 << 0) || dcrc & (1 <<  8)) ? (orig_dat0 + 1) : orig_dat0;
			cur_dat1 = (dcrc & (1 << 1) || dcrc & (1 <<  9)) ? (orig_dat1 + 1) : orig_dat1;
			cur_dat2 = (dcrc & (1 << 2) || dcrc & (1 << 10)) ? (orig_dat2 + 1) : orig_dat2;
			cur_dat3 = (dcrc & (1 << 3) || dcrc & (1 << 11)) ? (orig_dat3 + 1) : orig_dat3;
			cur_dat4 = (dcrc & (1 << 4) || dcrc & (1 << 12)) ? (orig_dat4 + 1) : orig_dat4;
			cur_dat5 = (dcrc & (1 << 5) || dcrc & (1 << 13)) ? (orig_dat5 + 1) : orig_dat5;
			cur_dat6 = (dcrc & (1 << 6) || dcrc & (1 << 14)) ? (orig_dat6 + 1) : orig_dat6;
			cur_dat7 = (dcrc & (1 << 7) || dcrc & (1 << 15)) ? (orig_dat7 + 1) : orig_dat7;
		} else {
			cur_dat0 = (dcrc & (1 << 0)) ? (orig_dat0 + 1) : orig_dat0;
			cur_dat1 = (dcrc & (1 << 1)) ? (orig_dat1 + 1) : orig_dat1;
			cur_dat2 = (dcrc & (1 << 2)) ? (orig_dat2 + 1) : orig_dat2;
			cur_dat3 = (dcrc & (1 << 3)) ? (orig_dat3 + 1) : orig_dat3;
			cur_dat4 = (dcrc & (1 << 4)) ? (orig_dat4 + 1) : orig_dat4;
			cur_dat5 = (dcrc & (1 << 5)) ? (orig_dat5 + 1) : orig_dat5;
			cur_dat6 = (dcrc & (1 << 6)) ? (orig_dat6 + 1) : orig_dat6;
			cur_dat7 = (dcrc & (1 << 7)) ? (orig_dat7 + 1) : orig_dat7;
		}

			cur_rxdly0 = ((cur_dat0 & 0x1F) << 24) | ((cur_dat1 & 0x1F) << 16) |
						 ((cur_dat2 & 0x1F) << 8)  | ((cur_dat3 & 0x1F) << 0);
			cur_rxdly1 = ((cur_dat4 & 0x1F) << 24) | ((cur_dat5 & 0x1F) << 16) |
						 ((cur_dat6 & 0x1F) << 8)  | ((cur_dat7 & 0x1F) << 0);

		sdr_write32(MSDC_DAT_RDDLY0, cur_rxdly0);
		sdr_write32(MSDC_DAT_RDDLY1, cur_rxdly1);
	}

	if((cur_dat0 >= 32) || (cur_dat1 >= 32) || (cur_dat2 >= 32) || (cur_dat3 >= 32)||
		(cur_dat4 >= 32) || (cur_dat5 >= 32) || (cur_dat6 >= 32) || (cur_dat7 >= 32)){
		if(sel){
			sdr_write32(MSDC_DAT_RDDLY0, 0);
			sdr_write32(MSDC_DAT_RDDLY1, 0);
			cur_ckgen = (orig_ckgen + 1) ;
			sdr_set_field(MSDC_PATCH_BIT0, MSDC_PB0_CKGEN_MSDC_DLY_SEL, cur_ckgen % 32);
		}
	}

	if(cur_ckgen >= 32 ){
		if(clkmode == 1 && sel){
			cur_dl_cksel = (orig_dl_cksel + 1);
			sdr_set_field(MSDC_PATCH_BIT0, MSDC_PB0_INT_DAT_LATCH_CK_SEL, cur_dl_cksel % 8);
		}
	}
	++(host->t_counter.time_read);
	if((sel == 1 && clkmode == 1 && host->t_counter.time_read == READ_TUNE_UHS_CLKMOD1_MAX_TIME)||
		(sel == 1 && (clkmode == 0 ||clkmode == 2) && host->t_counter.time_read == READ_TUNE_UHS_MAX_TIME)||
		(sel == 0 && (clkmode == 0 ||clkmode == 2) && host->t_counter.time_read == READ_TUNE_HS_MAX_TIME)){
#ifdef MSDC_LOWER_FREQ
		result = msdc_lower_freq(host);
#else
		result = 1;
#endif
		host->t_counter.time_read = 0;
	}

	sdr_get_field(MSDC_PATCH_BIT0, MSDC_PB0_CKGEN_MSDC_DLY_SEL, orig_ckgen);
	sdr_get_field(MSDC_PATCH_BIT0, MSDC_PB0_INT_DAT_LATCH_CK_SEL, orig_dl_cksel);
	sdr_get_field(MSDC_IOCON, MSDC_IOCON_R_D_SMPL, orig_dsmpl);
	cur_rxdly0 = sdr_read32(MSDC_DAT_RDDLY0);
	cur_rxdly1 = sdr_read32(MSDC_DAT_RDDLY1);
#endif
	host->read_time_tune++;


	pr_err("TUNE_READ: rsmpl<%d> pad_delay<%d>  ckgen<%d>,latch_ck<%d> sfreq.<%d>\n",
		cur_dsmpl, cur_rd_pad_dly, cur_ckgen,cur_dl_cksel, host->sclk);

	return result;
}

static int msdc_tune_write(struct msdc_host *host)
{
	u32 __iomem *base = host->base;
	//u32 cur_wrrdly = 0, orig_wrrdly;
	u32 cur_dsmpl = 0,	orig_dsmpl;
	u32 cur_rxdly0 = 0;
	u32 orig_dat0, orig_dat1, orig_dat2, orig_dat3;
	u32 cur_dat0 = 0,cur_dat1 = 0,cur_dat2 = 0,cur_dat3 = 0;
	int result = 0;
	int sel = 0;
	int clkmode = 0;
	u32 cur_w_pad_dly = 0, orig_w_pad_dly = 0;
	u32 cur_w_internal_dly = 0, orig_w_internal_dly = 0;
	u32 cur_dl_cksel = 0, orig_dl_cksel = 0;
	u32 cur_ckgen = 0, orig_ckgen = 0;

#ifdef CONFIG_MTK_USE_NEW_TUNE
	MsdcKernelOnlineAutoKTune(host,false,false,true);
	host->write_time_tune++;
	return 0;
#endif
	pr_err("========msdc_tune_write========\n");
	if (3 == partition_access)
		return 1;

	if (host->mclk >= 100000000)
		sel = 1;

	sdr_get_field(MSDC_CFG,MSDC_CFG_CKMOD,clkmode);

	sdr_get_field(MSDC_PAD_TUNE0, MSDC_PAD_TUNE0_DATWRDLY, orig_w_internal_dly);
	sdr_get_field(MSDC_PAD_TUNE0, MSDC_PAD_TUNE0_DATRRDLY, orig_w_pad_dly);
	sdr_get_field(MSDC_PATCH_BIT0, MSDC_PB0_INT_DAT_LATCH_CK_SEL, orig_dl_cksel);
	sdr_get_field(MSDC_PATCH_BIT0, MSDC_PB0_CKGEN_MSDC_DLY_SEL, orig_ckgen);
	sdr_get_field(MSDC_IOCON, MSDC_IOCON_W_D_SMPL, orig_dsmpl);

	cur_w_internal_dly = orig_w_internal_dly;
	cur_w_pad_dly = orig_w_pad_dly;
	cur_dl_cksel = orig_dl_cksel;
	cur_ckgen = orig_ckgen;
	cur_dsmpl = orig_dsmpl;
	
	
	

	cur_dsmpl = (orig_dsmpl + 1);
	sdr_set_field(MSDC_IOCON, MSDC_IOCON_W_D_SMPL, cur_dsmpl & 1);

    #if 1
	//pad delay
	if(cur_dsmpl >= 2){
		//rollback
		cur_dsmpl = 0;
		sdr_set_field(MSDC_IOCON, MSDC_IOCON_W_D_SMPL, cur_dsmpl & 1);
		
		cur_w_pad_dly = (orig_w_pad_dly + 1);
		sdr_set_field(MSDC_PAD_TUNE0, MSDC_PAD_TUNE0_DATRRDLY, cur_w_pad_dly & 31);
	}

	//internel delay
	if (cur_w_pad_dly >= 32 ) {
		//rollback
		cur_w_pad_dly = 0;
		sdr_set_field(MSDC_PAD_TUNE0, MSDC_PAD_TUNE0_DATRRDLY, cur_w_pad_dly & 31);
		
		cur_w_internal_dly = (orig_w_internal_dly + 1) ;
		sdr_set_field(MSDC_PAD_TUNE0, MSDC_PAD_TUNE0_CMDRRDLY, cur_w_internal_dly & 31);
		}

	//ckgen
	if (cur_w_internal_dly >= 32) {
		
		cur_w_internal_dly = 0 ;
		sdr_set_field(MSDC_PAD_TUNE0, MSDC_PAD_TUNE0_CMDRRDLY, cur_w_internal_dly & 31);

		cur_ckgen = (orig_ckgen + 1) ;
		sdr_set_field(MSDC_PATCH_BIT0, MSDC_PB0_CKGEN_MSDC_DLY_SEL, cur_ckgen & 31);
		}

	//latch_clock,for ckgen, only tune to 10
	if (cur_ckgen >= MAX_CKGEN_TUNE_TIME && sel) {
		//rollback
		cur_ckgen = 0 ;
		sdr_set_field(MSDC_PATCH_BIT0, MSDC_PB0_CKGEN_MSDC_DLY_SEL, cur_ckgen & 31);
		
		cur_dl_cksel = (orig_dl_cksel +1);
		sdr_set_field(MSDC_PATCH_BIT0, MSDC_PB0_INT_DAT_LATCH_CK_SEL, cur_dl_cksel & 7);
		}

	//overflow, all those proper setttings can not cover the error, we exit
	if (cur_dl_cksel > 7)
	{
		pr_err("Warning! READ WRITE: alrady tune too many times, go lower freq \n");
		
		cur_dl_cksel = 0;
		sdr_set_field(MSDC_PATCH_BIT0, MSDC_PB0_INT_DAT_LATCH_CK_SEL, cur_dl_cksel & 7);
		
        #ifdef MSDC_LOWER_FREQ
        result = msdc_lower_freq(host);
        #else
        result = 1;
        #endif
        host->t_counter.time_write = 0;
	}

    #else
	sdr_set_field(MSDC_IOCON, MSDC_IOCON_DDLSEL, 1);

	if(cur_dsmpl >= 2){
		cur_rxdly0 = sdr_read32(MSDC_DAT_RDDLY0);

			orig_dat0 = (cur_rxdly0 >> 24) & 0x1F;
			orig_dat1 = (cur_rxdly0 >> 16) & 0x1F;
			orig_dat2 = (cur_rxdly0 >>	8) & 0x1F;
			orig_dat3 = (cur_rxdly0 >>	0) & 0x1F;

		cur_dat0 = (orig_dat0 + 1); /* only adjust bit-1 for crc */
		cur_dat1 = orig_dat1;
		cur_dat2 = orig_dat2;
		cur_dat3 = orig_dat3;

			cur_rxdly0 = ((cur_dat0 & 0x1F) << 24) | ((cur_dat1 & 0x1F) << 16) |
					   ((cur_dat2 & 0x1F) << 8) | ((cur_dat3 & 0x1F) << 0);

		sdr_write32(MSDC_DAT_RDDLY0, cur_rxdly0);
	}

	if (cur_dat0 >= 32 && sel) {
			cur_d_cntr= (orig_d_cntr + 1 );
		sdr_set_field(MSDC_PATCH_BIT1, MSDC_PB1_WRDAT_CRCS_TA_CNTR, cur_d_cntr & 7);
		}

	++(host->t_counter.time_write);
	if ((sel == 0 && host->t_counter.time_write == WRITE_TUNE_HS_MAX_TIME) ||
		(sel && host->t_counter.time_write == WRITE_TUNE_UHS_MAX_TIME)) {
#ifdef MSDC_LOWER_FREQ
		result = msdc_lower_freq(host);
#else
		result = 1;
#endif
		host->t_counter.time_write = 0;
	}

	sdr_get_field(MSDC_IOCON, MSDC_IOCON_W_D_SMPL, orig_dsmpl);
	sdr_get_field(MSDC_PATCH_BIT1, MSDC_PB1_WRDAT_CRCS_TA_CNTR, orig_d_cntr);
	cur_rxdly0 = sdr_read32(MSDC_DAT_RDDLY0);
	#endif
	host->write_time_tune++;

	pr_err("TUNE_WRITE: rsmpl<%d> pad_delay<%d> internal_delay<%d> ckgen<%d>,latch_ck<%d> sfreq.<%d>\n",
		cur_dsmpl, cur_w_pad_dly, cur_w_internal_dly, cur_ckgen,cur_dl_cksel, host->sclk);

	return result;
}

static int msdc_get_card_status(struct mmc_host *mmc, struct msdc_host *host, u32 *status)
{
	struct mmc_command cmd;
	struct mmc_request mrq;
	u32 err;

	memset(&cmd, 0, sizeof(struct mmc_command));
	cmd.opcode = MMC_SEND_STATUS;
	cmd.arg = host->app_cmd_arg;
	cmd.flags = MMC_RSP_SPI_R2 | MMC_RSP_R1 | MMC_CMD_AC;

	memset(&mrq, 0, sizeof(struct mmc_request));
	mrq.cmd = &cmd; cmd.mrq = &mrq;
	cmd.data = NULL;

	err = msdc_do_command(host, &cmd, 0, CMD_TIMEOUT);	// tune until CMD13 pass.

	if (status) {
		*status = cmd.resp[0];
	}

	return err;
}

//#define TUNE_FLOW_TEST
#ifdef TUNE_FLOW_TEST
static void msdc_reset_para(struct msdc_host *host)
{
	u32 __iomem *base = host->base;
	u32 dsmpl, rsmpl;

	// because we have a card, which must work at dsmpl<0> and rsmpl<0>

	sdr_get_field(MSDC_IOCON, MSDC_IOCON_R_D_SMPL, dsmpl);
	sdr_get_field(MSDC_IOCON, MSDC_IOCON_RSPL, rsmpl);

	if (dsmpl == 0) {
		sdr_set_field(MSDC_IOCON, MSDC_IOCON_R_D_SMPL, 1);
		ERR_MSG("set dspl<0>\n");
		sdr_set_field(MSDC_PAD_TUNE0, MSDC_PAD_TUNE0_CMDRDLY, 0);
	}

	if (rsmpl == 0) {
		sdr_set_field(MSDC_IOCON, MSDC_IOCON_RSPL, 1);
		ERR_MSG("set rspl<0>\n");
		sdr_write32(MSDC_DAT_RDDLY0, 0);
		sdr_set_field(MSDC_PAD_TUNE0, MSDC_PAD_TUNE0_DATWRDLY, 0);
	}
}
#endif

static void msdc_dump_trans_error(struct msdc_host	 *host,
								  struct mmc_command *cmd,
								  struct mmc_data	 *data,
								  struct mmc_command *stop)
{
	if ((cmd->opcode == 52) && (cmd->arg == 0xc00))
		return;
	if ((cmd->opcode == 52) && (cmd->arg == 0x80000c08))
		return;
	if ((host->hw->host_function == MSDC_SD) && (cmd->opcode == 5))
		return;
/*
	ERR_MSG("XXX CMD<%d><0x%x> Error<%d> Resp<0x%x>\n",
		cmd->opcode, cmd->arg, cmd->error, cmd->resp[0]);

	if (data)
		ERR_MSG("XXX DAT block<%d> Error<%d>\n", data->blocks, data->error);

	if (stop)
		ERR_MSG("XXX STOP<%d><0x%x> Error<%d> Resp<0x%x>\n",
			stop->opcode, stop->arg, stop->error, stop->resp[0]);
*/
	if ((host->hw->host_function == MSDC_SD) &&
	   (host->sclk > 100000000) &&
	   (data) &&
	   (data->error != (unsigned int)-ETIMEDOUT)) {
	   if ((data->flags & MMC_DATA_WRITE) && (host->write_timeout_uhs104))
			host->write_timeout_uhs104 = 0;
	   if ((data->flags & MMC_DATA_READ) && (host->read_timeout_uhs104))
			host->read_timeout_uhs104 = 0;
	}
}

/* ops.request */
static void msdc_ops_request_legacy(struct mmc_host *mmc, struct mmc_request *mrq)
{
	struct msdc_host *host = mmc_priv(mmc);
	struct mmc_command *cmd;
	struct mmc_data *data;
	struct mmc_command *stop = NULL;
	int data_abort = 0;
	unsigned long flags;
	//=== for sdio profile ===
	u32 old_H32 = 0, old_L32 = 0, new_H32 = 0, new_L32 = 0;
	u32 ticks = 0, opcode = 0, sizes = 0, bRx = 0;

	msdc_reset_tune_counter(host,all_counter);
	if (host->mrq) {
		ERR_MSG("XXX host->mrq<0x%p> cmd<%d>arg<0x%x>\n",
			host->mrq, host->mrq->cmd->opcode, host->mrq->cmd->arg);
		//BUG();
		WARN_ON(1);
	}

	if (!is_card_present(host) || host->power_mode == MMC_POWER_OFF) {
		ERR_MSG("cmd<%d> arg<0x%x> card<%d> power<%d>\n",
			mrq->cmd->opcode,mrq->cmd->arg,is_card_present(host), host->power_mode);
		mrq->cmd->error = (unsigned int)-ENOMEDIUM;

		if(mrq->done)
			mrq->done(mrq);			// call done directly.

		return;
	}

	/* start to process */
	spin_lock(&host->lock);

	cmd = mrq->cmd;
	data = mrq->cmd->data;
	if (data)
		stop = data->stop;

	host->mrq = mrq;

	while (msdc_do_request(mmc,mrq)) { // there is some error
		// becasue ISR executing time will be monitor, try to dump the info here.
		msdc_dump_trans_error(host, cmd, data, stop);
		data_abort = 0;

		if ((cmd->error == (unsigned int)-EIO) || (stop && (stop->error == (unsigned int)-EIO))) {
			if (msdc_tune_cmdrsp(host)){
				ERR_MSG("failed to updata cmd para\n");
				goto out;
			}
		}

		if (data && (data->error == (unsigned int)-EIO)) {
			if (data->flags & MMC_DATA_READ) {	// read
				if (msdc_tune_read(host)) {
					ERR_MSG("failed to updata read para\n");
					goto out;
				}
			} else {
				if (msdc_tune_write(host)) {
					ERR_MSG("failed to updata write para\n");
					goto out;
				}
			}
		}

		// bring the card to "tran" state
		if (data) {
			if (msdc_abort_data(host)) {
				ERR_MSG("abort failed\n");
				data_abort = 1;
				if(host->hw->host_function == MSDC_SD){
					host->card_inserted = 0;
					if(host->mmc->card){
						spin_lock_irqsave(&host->remove_bad_card,flags);
						host->block_bad_card = 1;
						mmc_card_set_removed(host->mmc->card);
						spin_unlock_irqrestore(&host->remove_bad_card,flags);
					}
					goto out;
				}
			}
		}

		// CMD TO -> not tuning
		if (cmd->error == (unsigned int)-ETIMEDOUT) {
			if (cmd->opcode == MMC_READ_SINGLE_BLOCK ||
				cmd->opcode == MMC_READ_MULTIPLE_BLOCK ||
				cmd->opcode == MMC_WRITE_BLOCK ||
				cmd->opcode == MMC_WRITE_MULTIPLE_BLOCK) {
				if (data_abort)
						goto out;
			} else
				goto out;
		}

		// [ALPS114710] Patch for data timeout issue.
		if (data && (data->error == (unsigned int)-ETIMEDOUT)) {
			if (data->flags & MMC_DATA_READ) {
				if( !(host->sw_timeout) &&
					(host->hw->host_function == MSDC_SD) &&
					(host->sclk > 100000000) &&
					(host->read_timeout_uhs104 < MSDC_MAX_R_TIMEOUT_TUNE)){
					if(host->t_counter.time_read)
						host->t_counter.time_read--;
					host->read_timeout_uhs104++;
					msdc_tune_read(host);
				} else if ((host->sw_timeout) ||
							(host->read_timeout_uhs104 >= MSDC_MAX_R_TIMEOUT_TUNE) ||
							(++(host->read_time_tune) > MSDC_MAX_TIMEOUT_RETRY)) {
					ERR_MSG("exceed max read timeout retry times(%d) or"
							"SW timeout(%d) or read timeout tuning times(%d),Power cycle\n",
							host->read_time_tune, host->sw_timeout, host->read_timeout_uhs104);
						goto out;
					}
			} else if (data->flags & MMC_DATA_WRITE) {
				if( (!(host->sw_timeout)) &&
					(host->hw->host_function == MSDC_SD) &&
					(host->sclk > 100000000) &&
					(host->write_timeout_uhs104 < MSDC_MAX_W_TIMEOUT_TUNE)){
					if(host->t_counter.time_write)
						host->t_counter.time_write--;
					host->write_timeout_uhs104++;
					msdc_tune_write(host);
				} else if ((host->sw_timeout) ||
							(host->write_timeout_uhs104 >= MSDC_MAX_W_TIMEOUT_TUNE) ||
							(++(host->write_time_tune) > MSDC_MAX_TIMEOUT_RETRY)) {
					ERR_MSG("exceed max write timeout retry times(%d) or"
							"SW timeout(%d) or write timeout tuning time(%d),Power cycle\n",
							host->write_time_tune, host->sw_timeout, host->write_timeout_uhs104);
						goto out;
					}
				}
		}

		// clear the error condition.
		cmd->error = 0;
		if (data)
			data->error = 0;
		if (stop)
			stop->error = 0;

		// check if an app commmand.
		if (host->app_cmd) {
			while (msdc_app_cmd(host->mmc, host)) {
				if (msdc_tune_cmdrsp(host)){
					ERR_MSG("failed to updata cmd para for app\n");
					goto out;
				}
			}
		}

		if (!is_card_present(host)) {
			goto out;
		}
	}

	if ((host->read_time_tune) &&
		(cmd->opcode == MMC_READ_SINGLE_BLOCK || cmd->opcode == MMC_READ_MULTIPLE_BLOCK)) {
		host->read_time_tune = 0;
		ERR_MSG("sync Read recover\n");
		msdc_dump_trans_error(host, cmd, data, stop);
	}
	if ((host->write_time_tune) &&
		(cmd->opcode == MMC_WRITE_BLOCK || cmd->opcode == MMC_WRITE_MULTIPLE_BLOCK)) {
		host->write_time_tune = 0;
		ERR_MSG("sync Write recover\n");
		msdc_dump_trans_error(host, cmd, data, stop);
	}
	host->sw_timeout = 0;
out:
	msdc_reset_tune_counter(host,all_counter);

#ifdef TUNE_FLOW_TEST
	msdc_reset_para(host);
#endif

	/* ==== when request done, check if app_cmd ==== */
	if (mrq->cmd->opcode == MMC_APP_CMD) {
		host->app_cmd = 1;
		host->app_cmd_arg = mrq->cmd->arg;	/* save the RCA */
	} else {
		host->app_cmd = 0;
		//host->app_cmd_arg = 0;
	}

	host->mrq = NULL;

	//=== for sdio profile ===
	if (sdio_pro_enable) {
		if (mrq->cmd->opcode == 52 || mrq->cmd->opcode == 53) {
			//GPT_GetCounter64(&new_L32, &new_H32);
			ticks = msdc_time_calc(old_L32, old_H32, new_L32, new_H32);

			opcode = mrq->cmd->opcode;
			if (mrq->cmd->data) {
				sizes = mrq->cmd->data->blocks * mrq->cmd->data->blksz;
				bRx = mrq->cmd->data->flags & MMC_DATA_READ ? 1 : 0 ;
			} else {
				bRx = mrq->cmd->arg	& 0x80000000 ? 1 : 0;
			}

			if (!mrq->cmd->error) {
				msdc_performance(opcode, sizes, bRx, ticks);
			}
		}
	}

	spin_unlock(&host->lock);
	mmc_request_done(mmc, mrq);

	return;
}

#if 1
//#ifndef CONFIG_MTK_USE_NEW_TUNE
static void msdc_tune_async_request(struct mmc_host *mmc, struct mmc_request *mrq)
{
	struct msdc_host *host = mmc_priv(mmc);
	struct mmc_command *cmd;
	struct mmc_data *data;
	struct mmc_command *stop = NULL;
	int data_abort = 0;
	unsigned long flags;
	//msdc_reset_tune_counter(host,all_counter);
	if(host->mrq){
		ERR_MSG("XXX host->mrq<0x%p> cmd<%d>arg<0x%x>\n",
			host->mrq, host->mrq->cmd->opcode, host->mrq->cmd->arg);
		BUG();
	}

	if (!is_card_present(host) || host->power_mode == MMC_POWER_OFF) {
		ERR_MSG("cmd<%d> arg<0x%x> card<%d> power<%d>\n",
			mrq->cmd->opcode, mrq->cmd->arg, is_card_present(host), host->power_mode);
		mrq->cmd->error = (unsigned int)-ENOMEDIUM;
		//mrq->done(mrq);		  // call done directly.
		return;
	}

	cmd = mrq->cmd;
	data = mrq->cmd->data;
	if (data) stop = data->stop;
	if((cmd->error == 0) && (data && data->error == 0) && (!stop || stop->error == 0)){
		if(cmd->opcode == MMC_READ_SINGLE_BLOCK || cmd->opcode == MMC_READ_MULTIPLE_BLOCK)
			host->read_time_tune = 0;
		if(cmd->opcode == MMC_WRITE_BLOCK || cmd->opcode == MMC_WRITE_MULTIPLE_BLOCK)
			host->write_time_tune = 0;
		host->rwcmd_time_tune = 0;

		return;
	}
	/* start to process */
	spin_lock(&host->lock);


	host->tune = 1;
	host->mrq = mrq;
	do{
		msdc_dump_trans_error(host, cmd, data, stop);		  // becasue ISR executing time will be monitor, try to dump the info here.

		if ((cmd->error == (unsigned int)-EIO) || (stop && (stop->error == (unsigned int)-EIO))) {
			if (msdc_tune_cmdrsp(host)){
				ERR_MSG("failed to updata cmd para\n");
				goto out;
			}
		}

		if (data && (data->error == (unsigned int)-EIO)) {
			if (data->flags & MMC_DATA_READ) {	// read
				if (msdc_tune_read(host)) {
					ERR_MSG("failed to updata read para\n");
					goto out;
				}
			} else {
				if (msdc_tune_write(host)) {
					ERR_MSG("failed to updata write para\n");
					goto out;
				}
			}
		}

		// bring the card to "tran" state
		if (data) {
			if (msdc_abort_data(host)) {
				ERR_MSG("abort failed\n");
				data_abort = 1;
				if(host->hw->host_function == MSDC_SD){
					host->card_inserted = 0;
					if(host->mmc->card){
						spin_lock_irqsave(&host->remove_bad_card,flags);
						host->block_bad_card = 1;
						mmc_card_set_removed(host->mmc->card);
						spin_unlock_irqrestore(&host->remove_bad_card,flags);
					}
					goto out;
				}
			}
		}

		// CMD TO -> not tuning
		if (cmd->error == (unsigned int)-ETIMEDOUT) {
			if (cmd->opcode == MMC_READ_SINGLE_BLOCK ||
				cmd->opcode == MMC_READ_MULTIPLE_BLOCK ||
				cmd->opcode == MMC_WRITE_BLOCK ||
				cmd->opcode == MMC_WRITE_MULTIPLE_BLOCK) {
				if((host->sw_timeout) || (++(host->rwcmd_time_tune) > MSDC_MAX_TIMEOUT_RETRY)){
					ERR_MSG("exceed max r/w cmd timeout tune times(%d) or SW timeout(%d),Power cycle\n",
							host->rwcmd_time_tune, host->sw_timeout);
					if(!(host->sd_30_busy))
						goto out;
					}
			} else
				goto out;
		}

		// [ALPS114710] Patch for data timeout issue.
		if (data && (data->error == (unsigned int)-ETIMEDOUT)) {
			if (data->flags & MMC_DATA_READ) {
				if( !(host->sw_timeout) &&
					(host->hw->host_function == MSDC_SD) &&
					(host->sclk > 100000000) &&
					(host->read_timeout_uhs104 < MSDC_MAX_R_TIMEOUT_TUNE)){
					if(host->t_counter.time_read)
						host->t_counter.time_read--;
					host->read_timeout_uhs104++;
					msdc_tune_read(host);
				} else if ((host->sw_timeout) ||
							(host->read_timeout_uhs104 >= MSDC_MAX_R_TIMEOUT_TUNE) ||
							(++(host->read_time_tune) > MSDC_MAX_TIMEOUT_RETRY)) {
					ERR_MSG("exceed max read timeout retry times(%d) or"
							"SW timeout(%d) or read timeout tuning times(%d),Power cycle\n",
							host->read_time_tune, host->sw_timeout, host->read_timeout_uhs104);
					if (!(host->sd_30_busy))
						goto out;
					}
			} else if(data->flags & MMC_DATA_WRITE) {
				if( !(host->sw_timeout) &&
					(host->hw->host_function == MSDC_SD) &&
					(host->sclk > 100000000) &&
					(host->write_timeout_uhs104 < MSDC_MAX_W_TIMEOUT_TUNE)){
					if(host->t_counter.time_write)
						host->t_counter.time_write--;
					host->write_timeout_uhs104++;
					msdc_tune_write(host);
				} else if ((host->sw_timeout) ||
							(host->write_timeout_uhs104 >= MSDC_MAX_W_TIMEOUT_TUNE) ||
							(++(host->write_time_tune) > MSDC_MAX_TIMEOUT_RETRY)) {
					ERR_MSG("exceed max write timeout retry times(%d) or"
							"SW timeout(%d) or write timeout tuning time(%d),Power cycle\n",
							host->write_time_tune, host->sw_timeout, host->write_timeout_uhs104);
					if (!(host->sd_30_busy))
						goto out;
					}
				}
		}

		// clear the error condition.
		cmd->error = 0;
		if (data)
			data->error = 0;
		if (stop)
			stop->error = 0;
		host->sw_timeout = 0;
		if (!is_card_present(host))
			goto out;
	} while(msdc_tune_rw_request(mmc,mrq));

	if ((host->rwcmd_time_tune) &&
		(cmd->opcode == MMC_READ_SINGLE_BLOCK ||
		cmd->opcode == MMC_READ_MULTIPLE_BLOCK ||
		cmd->opcode == MMC_WRITE_BLOCK ||
		cmd->opcode == MMC_WRITE_MULTIPLE_BLOCK)) {
		host->rwcmd_time_tune = 0;
		ERR_MSG("RW cmd recover\n");
		msdc_dump_trans_error(host, cmd, data, stop);
	}
	if ((host->read_time_tune) &&
		(cmd->opcode == MMC_READ_SINGLE_BLOCK || cmd->opcode == MMC_READ_MULTIPLE_BLOCK)) {
		host->read_time_tune = 0;
		ERR_MSG("Read recover\n");
		msdc_dump_trans_error(host, cmd, data, stop);
	}
	if ((host->write_time_tune) &&
		(cmd->opcode == MMC_WRITE_BLOCK || cmd->opcode == MMC_WRITE_MULTIPLE_BLOCK)) {
		host->write_time_tune = 0;
		ERR_MSG("Write recover\n");
		msdc_dump_trans_error(host, cmd, data, stop);
	}

	host->sw_timeout = 0;

out:
	if (host->sclk <= 50000000 && host->state != MSDC_STATE_DDR)
		host->sd_30_busy = 0;
	msdc_reset_tune_counter(host,all_counter);
	host->mrq = NULL;

	host->tune = 0;
	spin_unlock(&host->lock);

	//mmc_request_done(mmc, mrq);
	return;
}
#endif

#ifdef CONFIG_MTK_USE_NEW_TUNE
int msdc_execute_tuning(struct mmc_host *mmc, unsigned int opcode)
{
	struct msdc_host *host = mmc_priv(mmc);

	BUG_ON(host == NULL);
	g_autok_cmdline_parsed |= MSDC_AUTOK_EXEC_DONE;
	if (g_autok_cmdline_parsed)
	{
		ERR_MSG("Kernel Do Not RUN Autok, Loader already send the param by command line\n");
		return 0;
	}
	if (host->hw->host_function == MSDC_EMMC) {
		if (mmc->ios.timing == MMC_TIMING_MMC_HS200) {
			ERR_MSG("[AUTOK]eMMC HS200 Tune\n");
			hs200_execute_tuning(host, NULL);
		} else if (mmc->ios.timing == MMC_TIMING_MMC_HS400) {
			ERR_MSG("[AUTOK]eMMC HS400 Tune\n");
			hs400_execute_tuning(host, NULL);
		}
	} else if (host->hw->host_function == MSDC_SD) {
		ERR_MSG("[AUTOK]SD Autok\n");
		autok_execute_tuning(host, NULL);
	}

	return 0;
}
#endif

static void msdc_ops_request(struct mmc_host *mmc, struct mmc_request *mrq)
{
	 struct mmc_data *data;
	 int async = 0;

	 BUG_ON(mmc == NULL);
	 BUG_ON(mrq == NULL);

	 data = mrq->data;
	if (data)
			async = data->host_cookie;

	if (async)
			msdc_do_request_async(mmc,mrq);
	 else
		msdc_ops_request_legacy(mmc,mrq);

	 return;
}

/* called by ops.set_ios */
static void msdc_set_buswidth(struct msdc_host *host, u32 width)
{
	u32 __iomem *base = host->base;
	u32 val = sdr_read32(SDC_CFG);

	val &= ~SDC_CFG_BUSWIDTH;

	switch (width) {
	default:
	case MMC_BUS_WIDTH_1:
		width = 1;
		val |= (MSDC_BUS_1BITS << 16);
		break;
	case MMC_BUS_WIDTH_4:
		val |= (MSDC_BUS_4BITS << 16);
		break;
	case MMC_BUS_WIDTH_8:
		val |= (MSDC_BUS_8BITS << 16);
		break;
	}

	sdr_write32(SDC_CFG, val);

	N_MSG(CFG, "Bus Width = %d\n", width);
}

/* ops.set_ios */
static void msdc_ops_set_ios(struct mmc_host *mmc, struct mmc_ios *ios)
{
	struct msdc_host *host = mmc_priv(mmc);
	struct msdc_hw *hw = host->hw;
	u32 __iomem *base = host->base;
	u32 state;
	u32 cur_rxdly0, cur_rxdly1;

	N_MSG(CFG, "set ios, power_mode=%d, bus_width=%d, clock=%d\n",
		ios->power_mode, ios->bus_width, ios->clock);

	spin_lock(&host->lock);

	if (ios->timing == MMC_TIMING_UHS_DDR50)
		state = MSDC_STATE_DDR;
	else if (ios->timing == MMC_TIMING_MMC_HS200)
		state = MSDC_STATE_HS200;
	else if (ios->timing == MMC_TIMING_MMC_HS400)
		state = MSDC_STATE_HS400;
	else
		state = MSDC_STATE_DEFAULT;

	msdc_set_buswidth(host, ios->bus_width);

	/* Power control ??? */
	switch (ios->power_mode) {
	case MMC_POWER_OFF:
	case MMC_POWER_UP:
		break;
	case MMC_POWER_ON:
		host->power_mode = MMC_POWER_ON;
		break;
	default:
		break;
	}

	if (msdc_host_mode[host->id] != mmc->caps) {
		mmc->caps = msdc_host_mode[host->id];
		sdr_write32(MSDC_PAD_TUNE0,   0x00000000);
		sdr_set_field(MSDC_PAD_TUNE0, MSDC_PAD_TUNE0_DATWRDLY, host->hw->datwrddly);
		sdr_set_field(MSDC_PAD_TUNE0, MSDC_PAD_TUNE0_CMDRRDLY, host->hw->cmdrrddly);
		sdr_set_field(MSDC_PAD_TUNE0, MSDC_PAD_TUNE0_CMDRDLY, host->hw->cmdrddly);
		sdr_write32(MSDC_IOCON,		 0x00000000);
		sdr_set_field(MSDC_IOCON, MSDC_IOCON_DDLSEL, 1);
			cur_rxdly0 = ((host->hw->dat0rddly & 0x1F) << 24) | ((host->hw->dat1rddly & 0x1F) << 16) |
							 ((host->hw->dat2rddly & 0x1F) << 8)  | ((host->hw->dat3rddly & 0x1F) << 0);
				cur_rxdly1 = ((host->hw->dat4rddly & 0x1F) << 24) | ((host->hw->dat5rddly & 0x1F) << 16) |
						 ((host->hw->dat6rddly & 0x1F) << 8)  | ((host->hw->dat7rddly & 0x1F) << 0);
		sdr_write32(MSDC_DAT_RDDLY0, cur_rxdly0);
		sdr_write32(MSDC_DAT_RDDLY1, cur_rxdly1);

		if(host->hw->host_function == MSDC_SD) //to check reserved bit7,6?
			sdr_write32(MSDC_PATCH_BIT1, 0xFFFF00C9);
		else
			sdr_write32(MSDC_PATCH_BIT1, 0xFFFF0009);

		/* internal clock: latch read data, not apply to sdio */
		host->hw->cmd_edge	= 0; // tuning from 0
		host->hw->rdata_edge = 0;
		host->hw->wdata_edge = 0;
	}

	/* msdc_select_clksrc(host, hw->sclk_src); */

	if (host->mclk != ios->clock || host->state != state) { /* not change when clock Freq. not changed ddr need set clock*/
		if(ios->clock >= 25000000) {
			sdr_set_field(MSDC_IOCON, MSDC_IOCON_RSPL, hw->cmd_edge);
			sdr_set_field(MSDC_IOCON, MSDC_IOCON_R_D_SMPL, hw->rdata_edge);
			sdr_set_field(MSDC_IOCON, MSDC_IOCON_W_D_SMPL, hw->wdata_edge);
			sdr_write32(MSDC_PAD_TUNE0,host->saved_para.pad_tune);
			sdr_write32(MSDC_DAT_RDDLY0,host->saved_para.ddly0);
			sdr_write32(MSDC_DAT_RDDLY1,host->saved_para.ddly1);
			sdr_set_field(MSDC_PATCH_BIT1, MSDC_PB1_WRDAT_CRCS_TA_CNTR, host->saved_para.wrdat_crc_ta_cntr);
			sdr_set_field(MSDC_PATCH_BIT1, MSDC_PB1_CMD_RSP_TA_CNTR,	host->saved_para.cmd_resp_ta_cntr);
			if (host->id == 1) {
				sdr_set_field(MSDC_PATCH_BIT1, MSDC_PB1_GET_BUSY_MA, 1);
				sdr_set_field(MSDC_PATCH_BIT1, MSDC_PB1_GET_CRC_MA, 1);
			}
		}

		if (ios->clock == 0) {
			if(ios->power_mode == MMC_POWER_OFF){
				sdr_get_field(MSDC_IOCON, MSDC_IOCON_RSPL, hw->cmd_edge);	// save the para
				sdr_get_field(MSDC_IOCON, MSDC_IOCON_R_D_SMPL, hw->rdata_edge);
				sdr_get_field(MSDC_IOCON, MSDC_IOCON_W_D_SMPL, hw->wdata_edge);
				host->saved_para.pad_tune = sdr_read32(MSDC_PAD_TUNE0);
				host->saved_para.ddly0 = sdr_read32(MSDC_DAT_RDDLY0);
				host->saved_para.ddly1 = sdr_read32(MSDC_DAT_RDDLY1);
				sdr_get_field(MSDC_PATCH_BIT1, MSDC_PB1_CMD_RSP_TA_CNTR,	host->saved_para.cmd_resp_ta_cntr);
				sdr_get_field(MSDC_PATCH_BIT1, MSDC_PB1_WRDAT_CRCS_TA_CNTR, host->saved_para.wrdat_crc_ta_cntr);
			}

			/* reset to default value */
			sdr_write32(MSDC_IOCON,		 0x00000000);
			sdr_write32(MSDC_DAT_RDDLY0, 0x00000000);
			sdr_write32(MSDC_DAT_RDDLY1, 0x00000000);
			sdr_write32(MSDC_PAD_TUNE0,   0x00000000);
			sdr_set_field(MSDC_PATCH_BIT1, MSDC_PB1_CMD_RSP_TA_CNTR,1);
			sdr_set_field(MSDC_PATCH_BIT1, MSDC_PB1_WRDAT_CRCS_TA_CNTR,1);

			if (host->id == 1) {
				sdr_set_field(MSDC_PATCH_BIT1, MSDC_PB1_GET_BUSY_MA, 1);
				sdr_set_field(MSDC_PATCH_BIT1, MSDC_PB1_GET_CRC_MA, 1);
			}
		}
		//set default value
		sdr_set_field(MSDC_IOCON, MSDC_IOCON_DDLSEL, 0);//IOCON[3]
		sdr_set_field(MSDC_IOCON, MSDC_IOCON_W_D_SMPL_SEL, 0);//IOCON[9]

		msdc_set_mclk(host, state, ios->clock);
		if (host->load_ett_settings)
			host->load_ett_settings(mmc);

#ifndef MSDC_ONLY_USE_ONE_CLKSRC
		if (mmc->ios.timing == MMC_TIMING_UHS_DDR50 ||
				mmc->ios.timing == MMC_TIMING_MMC_HS200)
			sdr_write32(host->hclk_base, 0x2);
#endif
#ifdef CONFIG_ARCH_MT5891
	if (mmc->ios.timing == MMC_TIMING_UHS_DDR50)
	{
		sdr_set_field(MSDC_PATCH_BIT2, MSDC_PB2_DDR5O_SEL, 1);//fix DDR50 byte swap issue
	}
#endif

	}

	spin_unlock(&host->lock);
}

/* ops.get_ro */
static int msdc_ops_get_ro(struct mmc_host *mmc)
{
	struct msdc_host *host = mmc_priv(mmc);
	u32 __iomem *base = host->base;
	unsigned long flags;
	int ro = 0;

	spin_lock_irqsave(&host->lock, flags);
	if (host->hw->flags & MSDC_WP_PIN_EN) { /* set for card */
		ro = (sdr_read32(MSDC_PS) >> 31);
	}

	spin_unlock_irqrestore(&host->lock, flags);
	return ro;
}

/* ops.get_cd */
static int msdc_ops_get_cd(struct mmc_host *mmc)
{
	struct msdc_host *host = mmc_priv(mmc);
	u32 __iomem *base;
	unsigned long flags;
	//int present = 1;

	base = host->base;
	spin_lock_irqsave(&host->lock, flags);

	/* for emmc, MSDC_REMOVABLE not set, always return 1 */
	if (!(host->hw->flags & MSDC_REMOVABLE)) {
		host->card_inserted = 1;
		goto end;
	}

	//INIT_MSG("Card insert<%d> Block bad card<%d>\n",
	//	host->card_inserted, host->block_bad_card);
	if(host->hw->host_function == MSDC_SD && host->block_bad_card)
		host->card_inserted = 0;

end:
	spin_unlock_irqrestore(&host->lock, flags);
	return host->card_inserted;
}

static inline void mmc_delay(unsigned int ms)
{
	if (ms < 1000 / HZ) {
		cond_resched();
		mdelay(ms);
	} else
		msleep(ms);
}

static int msdc0_do_start_signal_voltage_switch(struct mmc_host *mmc,
	struct mmc_ios *ios)
{
	struct msdc_host *host = mmc_priv(mmc);
	u32 __iomem *base = host->base;

	// wait SDC_STA.SDCBUSY = 0
	while(sdc_is_busy());

	// Disable pull-up resistors
	sdr_clr_bits(MSDC_PAD_CTL1, (0x1<<17));
	sdr_clr_bits(MSDC_PAD_CTL2, (0x1<<17));

	// make sure CMD/DATA lines are pulled to 0
	while((sdr_read32(MSDC_PS)&(0x1FF<<16)));

	// Program the PMU of host from 3.3V to 1.8V
	if (0 != MsdcSDPowerSwitch(1))
		return -1;

	// Delay at least 5ms
	mmc_delay(6);

	// Enable pull-up resistors
	sdr_clr_bits(MSDC_PAD_CTL1, (0x1<<17));
	sdr_clr_bits(MSDC_PAD_CTL2, (0x1<<17));


	// start 1.8V voltage detecting
	sdr_set_bits(MSDC_CFG, MSDC_CFG_BV18SDT);

	// Delay at most 1ms
	mmc_delay(1);

	// check MSDC.VOL_18V_START_DET is 0
	while(sdr_read32(MSDC_CFG)&MSDC_CFG_BV18SDT);

	// check MSDC_CFG.VOL_18V_PASS
	if (sdr_read32(MSDC_CFG)&MSDC_CFG_BV18PSS) {
		ERR_MSG("switch 1.8v voltage success!\n");
		return 0;
	} else {
		ERR_MSG("switch 1.8v voltage failed!\n");
		return -1;
	}

}

int msdc_ops_switch_volt(struct mmc_host *mmc, struct mmc_ios *ios) //for sdr 104 3.3->1.8
{
	struct msdc_host *host = mmc_priv(mmc);
	int err;
	if(host->hw->host_function == MSDC_EMMC)
		return 0;

	if (mmc->ios.signal_voltage < 1)
		return 0;
	err = msdc0_do_start_signal_voltage_switch(mmc, ios);

	return err;
}

static void msdc_ops_stop(struct mmc_host *mmc,struct mmc_request *mrq)
{
	struct msdc_host *host = mmc_priv(mmc);
	u32 err = -1;

	if(!mrq->stop)
		return;

	if((host->autocmd == MSDC_AUTOCMD12) && (!(host->error & REQ_DAT_ERR)))
		return;

	N_MSG(OPS, "MSDC Stop for non-autocmd12 host->error(%d)host->autocmd(%d)\n",
		host->error, host->autocmd);

	err = msdc_do_command(host, mrq->stop, 0, CMD_TIMEOUT);
	if (err) {
		if (mrq->stop->error == (unsigned int)-EIO)
			host->error |= REQ_STOP_EIO;
		if (mrq->stop->error == (unsigned int)-ETIMEDOUT)
			host->error |= REQ_STOP_TMO;
	}
}

static bool msdc_check_written_data(struct mmc_host *mmc,struct mmc_request *mrq)
{
	u32 result = 0;
	struct msdc_host *host = mmc_priv(mmc);
	struct mmc_card *card;

	if (!is_card_present(host) || host->power_mode == MMC_POWER_OFF) {
		ERR_MSG("cmd<%d> arg<0x%x> card<%d> power<%d>\n",
			mrq->cmd->opcode, mrq->cmd->arg, is_card_present(host), host->power_mode);
		mrq->cmd->error = (unsigned int)-ENOMEDIUM;
		return 0;
	}

	if(mmc->card)
		card = mmc->card;
	else
		return 0;

	if ((host->hw->host_function == MSDC_SD)
		&& (host->sclk > 100000000)
		&& mmc_card_sd(card)
		&& (mrq->data)
		&& (mrq->data->flags & MMC_DATA_WRITE)
		&& (host->error == 0)) {
		spin_lock(&host->lock);
		if (msdc_polling_idle(host)) {
			spin_unlock(&host->lock);
			return 0;
		}
		spin_unlock(&host->lock);
		result = __mmc_sd_num_wr_blocks(card);
		if ((result != mrq->data->blocks) && (is_card_present(host)) && (host->power_mode == MMC_POWER_ON)) {
			mrq->data->error = (unsigned int)-EIO;
			host->error |= REQ_DAT_ERR;
			ERR_MSG("written data<%d> blocks isn't equal to request data blocks<%d>\n",
				result, mrq->data->blocks);
			return 1;
		}
	}
	return 0;
}
static void msdc_dma_error_reset(struct mmc_host *mmc)
{
	struct msdc_host *host = mmc_priv(mmc);
	u32 __iomem *base = host->base;
	struct mmc_data *data = host->data;

	if (data && host->dma_xfer && (data->host_cookie) && (host->tune == 0)) {
		host->sw_timeout++;
		host->error |= REQ_DAT_ERR;
		msdc_reset_hw();
		msdc_dma_stop(host);
		msdc_clr_fifo();
		msdc_clr_int();
		msdc_dma_clear(host);
	}
}

/*--------------------------------------------------------------------------*/
/* interrupt handler													*/
/*--------------------------------------------------------------------------*/
static irqreturn_t msdc_irq(int irq, void *dev_id)
{
	struct msdc_host  *host = (struct msdc_host *)dev_id;
	struct mmc_data   *data = host->data;
	struct mmc_command *cmd = host->cmd;
	struct mmc_command *stop = NULL;
	struct mmc_request *mrq = host->mrq;
	u32 __iomem *base = host->base;
	u32 cmdsts = MSDC_INT_RSPCRCERR | MSDC_INT_CMDTMO | MSDC_INT_CMDRDY |
				MSDC_INT_ACMDCRCERR | MSDC_INT_ACMDTMO | MSDC_INT_ACMDRDY |
				MSDC_INT_ACMD19_DONE;
	u32 datsts = MSDC_INT_DATCRCERR | MSDC_INT_DATTMO;
	u32 intsts, inten;

	intsts = sdr_read32(MSDC_INT);
	inten  = sdr_read32(MSDC_INTEN);
	inten &= intsts;

	sdr_write32(MSDC_INT, intsts);	/* clear interrupts */
	/* MSG will cause fatal error */


	/* sdio interrupt */
	if (intsts & MSDC_INT_SDIOIRQ) {
		IRQ_MSG("XXX MSDC_INT_SDIOIRQ\n");	/* seems not sdio irq */
	}

	/* transfer complete interrupt */
	if (data != NULL) {
		stop = data->stop;
		if (inten & MSDC_INT_XFER_COMPL) {
			data->bytes_xfered = host->dma.xfersz;
			if ((data->host_cookie) && (host->tune == 0)) {
				msdc_dma_stop(host);
				msdc_dma_clear(host);
				mb();
				if (mrq != NULL) {
					if(mrq->done)
						mrq->done(mrq);
				}

				host->error &= ~REQ_DAT_ERR;
			} else
				complete(&host->xfer_done);
		}

		if (intsts & datsts) {
			/* do basic reset, or stop command will sdc_busy */
			msdc_reset_hw();
			atomic_set(&host->abort, 1);  /* For PIO mode exit */

			if (intsts & MSDC_INT_DATTMO) {
				data->error = (unsigned int)-ETIMEDOUT;
				if (mrq != NULL)
					;//IRQ_MSG("XXX CMD<%d> Arg<0x%.8x> MSDC_INT_DATTMO\n",
					//	host->mrq->cmd->opcode, host->mrq->cmd->arg);
				else
					IRQ_MSG("XXX raw access r/w failed! MSDC_INT_DATTMO\n");
			} else if (intsts & MSDC_INT_DATCRCERR) {
				data->error = (unsigned int)-EIO;
				if (mrq != NULL)
					;//IRQ_MSG("XXX CMD<%d> Arg<0x%.8x> MSDC_INT_DATCRCERR, SDC_DCRC_STS<0x%x>\n",
					//	host->mrq->cmd->opcode, host->mrq->cmd->arg, sdr_read32(SDC_DCRC_STS));
				else
					IRQ_MSG("XXX raw access r/w failed! MSDC_INT_DATCRCERR, SDC_DCRC_STS<0x%x>\n",
						sdr_read32(SDC_DCRC_STS));
			}

			//if(sdr_read32(MSDC_INTEN) & MSDC_INT_XFER_COMPL) {
			if (host->dma_xfer) {
				if ((data->host_cookie) && (host->tune == 0)) {
					msdc_dma_stop(host);
					msdc_clr_fifo();
					msdc_clr_int();
					msdc_dma_clear(host);
					mb();
					if (mrq != NULL)  {
						if(mrq->done)
							mrq->done(mrq);
					}

					host->error |= REQ_DAT_ERR;
				} else
					complete(&host->xfer_done); /* Read CRC come fast, XFER_COMPL not enabled */
			} /* PIO mode can't do complete, because not init */
		}

		if ((stop != NULL) && (host->autocmd == MSDC_AUTOCMD12) && (intsts & cmdsts)) {
			if (intsts & MSDC_INT_ACMDRDY) {
				u32 *arsp = &stop->resp[0];
				*arsp = sdr_read32(SDC_ACMD_RESP);
			} else if (intsts & MSDC_INT_ACMDCRCERR) {
					stop->error =(unsigned int)-EIO;
					host->error |= REQ_STOP_EIO;
					msdc_reset_hw();
			} else if (intsts & MSDC_INT_ACMDTMO) {
					stop->error =(unsigned int)-ETIMEDOUT;
					host->error |= REQ_STOP_TMO;
					msdc_reset_hw();
			}
			if ((intsts & MSDC_INT_ACMDCRCERR) || (intsts & MSDC_INT_ACMDTMO)) {
				if (host->dma_xfer) {
					if ((data->host_cookie) && (host->tune == 0)) {
						msdc_dma_stop(host);
						msdc_clr_fifo();
						msdc_clr_int();
						msdc_dma_clear(host);
						mb();
						if (mrq != NULL)  {
							if(mrq->done)
								mrq->done(mrq);
						}
					}
					else
						complete(&host->xfer_done); //Autocmd12 issued but error occur, the data transfer done INT will not issue,so cmplete is need here
				}/* PIO mode can't do complete, because not init */
			}
		}
	}

	/* command interrupts */
	if ((cmd != NULL) && (intsts & cmdsts)) {
		if (intsts & MSDC_INT_CMDRDY) {
			u32 *rsp = NULL;
			rsp = &cmd->resp[0];

			switch (host->cmd_rsp) {
			case RESP_NONE:
				break;
			case RESP_R2:
				rsp[0] = sdr_read32(SDC_RESP3);
				rsp[1] = sdr_read32(SDC_RESP2);
				rsp[2] = sdr_read32(SDC_RESP1);
				rsp[3] = sdr_read32(SDC_RESP0);
				break;
			default: /* Response types 1, 3, 4, 5, 6, 7(1b) */
				rsp[0] = sdr_read32(SDC_RESP0);
				break;
			}
		} else if (intsts & MSDC_INT_RSPCRCERR) {
			cmd->error = (unsigned int)-EIO;
			IRQ_MSG("XXX CMD<%d> MSDC_INT_RSPCRCERR Arg<0x%.8x>\n",
					cmd->opcode, cmd->arg);
			msdc_reset_hw();
		} else if (intsts & MSDC_INT_CMDTMO) {
			cmd->error = (unsigned int)-ETIMEDOUT;
			IRQ_MSG("XXX CMD<%d> MSDC_INT_CMDTMO Arg<0x%.8x>\n",
					cmd->opcode, cmd->arg);
			msdc_reset_hw();
		} else
			IRQ_MSG("XXX CMD<%d> Arg<0x%.8x> UNEXPECTED IRQ 0x%x\n",
					cmd->opcode, cmd->arg, intsts);

		if (intsts & (MSDC_INT_CMDRDY | MSDC_INT_RSPCRCERR | MSDC_INT_CMDTMO))
			complete(&host->cmd_done);
	}

	return IRQ_HANDLED;
}

/*--------------------------------------------------------------------------*/
/* platform_driver members														*/
/*--------------------------------------------------------------------------*/
/* called by msdc_drv_probe */
static void msdc_init_hw(struct msdc_host *host)
{
	u32 __iomem *base = host->base;
	struct msdc_hw *hw = host->hw;
	u32 cur_rxdly0, cur_rxdly1;

	msdc_pinmux(host);

	/* open dmuxpll src for HS200 */
	u32 __iomem *reg_base;
	reg_base = ioremap(0xF0062400, 4);
	sdr_set_field(reg_base, 0x1 << 31, 0);
	sdr_set_field(reg_base, 0x1 << 4, 0);
	iounmap(reg_base);
	/* configure hclk */
	msdc_config_hclk(host);

	/* Bug Fix: If clock is disabed, Version Register Can't be read. */
#ifdef MSDC_ONLY_USE_ONE_CLKSRC
	msdc_select_clksrc(host, 0);//insure clock can work, it must need set to 0 before change clock source
	msdc_select_clksrc(host, host->hw->sclk_src);
#else
	msdc_select_clksrc(host, 0);
#endif

	/* Configure to MMC/SD mode */
	sdr_set_field(MSDC_CFG, MSDC_CFG_MODE, MSDC_SDMMC);

	/* Reset */
	msdc_reset_hw();

	/* Disable card detection */
	sdr_clr_bits(MSDC_PS, MSDC_PS_CDEN);

	/* Disable and clear all interrupts */
	sdr_clr_bits(MSDC_INTEN, sdr_read32(MSDC_INTEN));
	sdr_write32(MSDC_INT, sdr_read32(MSDC_INT));

	/* reset tuning parameter */
	sdr_write32(MSDC_PAD_TUNE0,   0x00000000);
	sdr_write32(MSDC_IOCON,		 0x00000000);
	sdr_set_field(MSDC_IOCON, MSDC_IOCON_RISCSZ, 0x2);
	sdr_set_field(MSDC_IOCON, MSDC_IOCON_DDLSEL, 1);
	cur_rxdly0 = ((hw->dat0rddly & 0x1F) << 24) | ((hw->dat1rddly & 0x1F) << 16) |
					 ((hw->dat2rddly & 0x1F) << 8)	| ((hw->dat3rddly & 0x1F) << 0);
	cur_rxdly1 = ((hw->dat4rddly & 0x1F) << 24) | ((hw->dat5rddly & 0x1F) << 16) |
					 ((hw->dat6rddly & 0x1F) << 8)	| ((hw->dat7rddly & 0x1F) << 0);

	sdr_write32(MSDC_DAT_RDDLY0, 0x00000000);
	sdr_write32(MSDC_DAT_RDDLY1, 0x00000000);

	if(host->hw->host_function == MSDC_SD)
		sdr_write32(MSDC_PATCH_BIT1, 0xFFFF00C9); //to check C9??
	else
		sdr_write32(MSDC_PATCH_BIT1, 0xFFFE0009);

	//data delay settings should be set after enter high speed mode(now is at ios function >25MHz), detail information, please refer to P4 description
	host->saved_para.pad_tune = (((hw->cmdrrddly & 0x1F) << 22) |
			((hw->cmdrddly & 0x1F) << 16) | ((hw->datwrddly & 0x1F) << 0));//sdr_read32(MSDC_PAD_TUNE0);
	host->saved_para.ddly0 = cur_rxdly0; //sdr_read32(MSDC_DAT_RDDLY0);
	host->saved_para.ddly1 = cur_rxdly1; //sdr_read32(MSDC_DAT_RDDLY1);
	sdr_get_field(MSDC_PATCH_BIT1, MSDC_PB1_CMD_RSP_TA_CNTR,	host->saved_para.cmd_resp_ta_cntr);
	sdr_get_field(MSDC_PATCH_BIT1, MSDC_PB1_WRDAT_CRCS_TA_CNTR, host->saved_para.wrdat_crc_ta_cntr);

	/* internal clock: latch read data, not apply to sdio */
	host->hw->cmd_edge	= 0; // tuning from 0
	host->hw->rdata_edge = 0;
	host->hw->wdata_edge = 0;

	/* for safety, should clear SDC_CFG.SDIO_INT_DET_EN & set SDC_CFG.SDIO in
	   pre-loader,uboot,kernel drivers. and SDC_CFG.SDIO_INT_DET_EN will be only
	   set when kernel driver wants to use SDIO bus interrupt */
	/* Configure to enable SDIO mode. it's must otherwise sdio cmd5 failed */
	sdr_clr_bits(SDC_CFG, SDC_CFG_SDIO);

	/* disable detect SDIO device interupt function */
	sdr_clr_bits(SDC_CFG, SDC_CFG_SDIOIDE);

	/* set clk, cmd, dat pad driving */
	hw->clk_drv = msdcClk[IDX_CLK_DRV_VAL][sizeof(msdcClk) / sizeof(msdcClk[0]) - 1];
	hw->cmd_drv = msdcClk[IDX_CMD_DRV_VAL][sizeof(msdcClk) / sizeof(msdcClk[0]) - 1];
	hw->dat_drv = msdcClk[IDX_DAT_DRV_VAL][sizeof(msdcClk) / sizeof(msdcClk[0]) - 1];

	/* write crc timeout detection */
	sdr_set_field(MSDC_PATCH_BIT0, MSDC_PB0_DETWR_CRCTMO, 1);

#ifdef CONFIG_ARCH_MT5891
	/* support 64G dram */
	sdr_set_field(MSDC_PATCH_BIT2, MSDC_PB2_SUPPORT64G, 1);
#endif

	/* Configure to default data timeout */
	sdr_set_field(SDC_CFG, SDC_CFG_DTOC, DEFAULT_DTOC);

	msdc_set_buswidth(host, MMC_BUS_WIDTH_1);

	N_MSG(FUC, "init hardware done!\n");
}

/* called by msdc_drv_remove */
static void msdc_deinit_hw(struct msdc_host *host)
{
	u32 __iomem *base = host->base;

	/* Disable and clear all interrupts */
	sdr_clr_bits(MSDC_INTEN, sdr_read32(MSDC_INTEN));
	sdr_write32(MSDC_INT, sdr_read32(MSDC_INT));
}

/* init gpd and bd list in msdc_drv_probe */
static void msdc_init_gpd_bd(struct msdc_host *host, struct msdc_dma *dma)
{
	gpd_t *gpd = dma->gpd;
	bd_t  *bd  = dma->bd;
	bd_t  *ptr, *prev;

	/* we just support one gpd */
	int bdlen = MAX_BD_PER_GPD;

	/* init the 2 gpd */
	memset(gpd, 0, sizeof(gpd_t) * 2);
	gpd->next = (u32)(dma->gpd_addr + sizeof(gpd_t));

	gpd->bdp  = 1;	 /* hwo, cs, bd pointer */
	gpd->ptr = (u32)dma->bd_addr; /* physical address */

	memset(bd, 0, sizeof(bd_t) * bdlen);
	ptr = bd + bdlen - 1;

	while (ptr != bd) {
		prev = ptr - 1;
		prev->next = (u32)(dma->bd_addr + sizeof(bd_t) *(ptr - bd));
		ptr = prev;
	}
}

struct msdc_host *msdc_get_host(int host_function, bool boot)
{
	int host_index = 0;
	struct msdc_host *host = NULL;

	for (; host_index < HOST_MAX_NUM; ++host_index) {
		if(!mtk_msdc_host[host_index])
			continue;

		if ((host_function == mtk_msdc_host[host_index]->hw->host_function)
			&& (boot == mtk_msdc_host[host_index]->hw->boot)) {
			host = mtk_msdc_host[host_index];
			break;
		}
	}

	if(host == NULL){
		ERR_MSG("[MSDC] This host(<host_function:%d> <boot:%d>)"
								" isn't in MSDC host config list\n", host_function, boot);
		//BUG();
	}

	return host;
}
EXPORT_SYMBOL(msdc_get_host);

static struct mmc_host_ops mt_msdc_ops = {
	.post_req		 = msdc_post_req,
	.pre_req		 = msdc_pre_req,
	.request		 = msdc_ops_request,
//#ifndef CONFIG_MTK_USE_NEW_TUNE
#if 1
	.tuning			 = msdc_tune_async_request,
#endif
	.set_ios		 = msdc_ops_set_ios,
	.get_ro			 = msdc_ops_get_ro,
	.get_cd			 = msdc_ops_get_cd,
	.start_signal_voltage_switch = msdc_ops_switch_volt,
//#ifndef CONFIG_MTK_USE_NEW_TUNE
#if 1
	.send_stop		 = msdc_ops_stop,
#endif
	.dma_error_reset = msdc_dma_error_reset,
	.check_written_data = msdc_check_written_data,
#ifdef CONFIG_MTK_USE_NEW_TUNE
	.execute_tuning  = msdc_execute_tuning,
#endif
};

#ifdef MSDC_USE_OF
static int msdc_hw_resource_init(struct msdc_host *host, struct platform_device *pdev)
{
	phys_addr_t sclk_base = 0, hclk_base = 0;

#define CHK_IOMEM_RET(b) \
	if (!b) { \
		dev_err(&pdev->dev, "ioremap "#b" failed!\n"); \
		return -EINVAL; \
	} else \
		dev_err(&pdev->dev, "ioremap "#b" = 0x%p\n", b);

	host->irq = irq_of_parse_and_map(pdev->dev.of_node, 0);
	BUG_ON(host->irq < 0);
	/* dev_err(&pdev->dev, "VECTOR_MSDC=%d, VECTOR_MSDC2=%d, irq # %d\n",
			VECTOR_MSDC, VECTOR_MSDC2, host->irq); */

	host->base = of_iomap(pdev->dev.of_node, 0);
	//CHK_IOMEM_RET(host->base);

	of_property_read_u32(pdev->dev.of_node, "sclk-reg", (u32*)&sclk_base);
	host->sclk_base = ioremap(sclk_base, 0x4);
	//CHK_IOMEM_RET(host->sclk_base);

	of_property_read_u32(pdev->dev.of_node, "hclk-reg", (u32*)&hclk_base);
	host->hclk_base = ioremap(hclk_base, 0x4);
	//CHK_IOMEM_RET(host->hclk_base);

	of_property_read_u32(pdev->dev.of_node, "id", &host->id);
	/* dev_err(&pdev->dev, "pdev->id=%d, of id=%d\n", pdev->id, host->id); */
	if (host->id < 0) {
		dev_err(&pdev->dev, "Invalid id number! (%d)\n", host->id);
		return -EINVAL;
	}
	pdev->id = host->id;

	if (host->id == 0)
		host->hw = &msdc0_hw;
	else if (host->id == 1)
		host->hw = &msdc1_hw;
	dev_err(&pdev->dev, "hw_func=%s\n",
			(host->hw->host_function == MSDC_EMMC) ? "eMMC" : "SD");

	return 0;
#undef CHK_IOMEM_RET
}
#else
static int msdc_hw_resource_init(struct msdc_host *host, struct platform_device *pdev)
{
	struct resource *mem = NULL;

#define CHK_IOMEM_RET(b) \
	if (!b) { \
		dev_err(&pdev->dev, "ioremap "#b" failed!\n"); \
		return -EINVAL; \
	} else \
		dev_err(&pdev->dev, "ioremap "#b" = 0x%p\n", b);

	host->hw = (struct msdc_hw*)pdev->dev.platform_data;
	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	host->irq = platform_get_irq(pdev, 0);
	host->base = ioremap((phys_addr_t)mem->start, 0x1000);
	//CHK_IOMEM_RET(host->base);

	BUG_ON((!host->hw) || (!mem) || (host->irq < 0));

	mem = request_mem_region(mem->start, mem->end - mem->start + 1, DRV_NAME);
	if (mem == NULL)
		return -EBUSY;

	host->sclk_base = ioremap(host->hw->sclk_base, 0x4);
	//CHK_IOMEM_RET(host->sclk_base);
	host->hclk_base = ioremap(host->hw->hclk_base, 0x4);
	//CHK_IOMEM_RET(host->hclk_base);

	host->id = pdev->id;

	return 0;
#undef CHK_IOMEM_RET
}
#endif

static void msdc_config_mmc(struct mmc_host *mmc)
{
	struct msdc_host *host = mmc_priv(mmc);

	/* Set host parameters to mmc */
	mmc->ops		= &mt_msdc_ops;
	mmc->f_min		= HOST_MIN_MCLK;
#if defined(CONFIG_ARCH_MT5882)||defined(CONFIG_ARCH_MT5883)
	if (IS_IC_MT5885())
		mmc->f_max		= 192000000;
	else if (IS_IC_MT5882())
		mmc->f_max		= 162000000;
	else
		mmc->f_max		= 162000000;
#else
	mmc->f_max		= HOST_MAX_MCLK;
#endif
	if (host->hw->host_function == MSDC_EMMC &&
		!(host->hw->flags & MSDC_UHS1))
		mmc->f_max = 50000000;

	mmc->ocr_avail	= MSDC_OCR_AVAIL;
	if (host->hw->flags & MSDC_HIGHSPEED)
		mmc->caps	= MMC_CAP_MMC_HIGHSPEED | MMC_CAP_SD_HIGHSPEED;

	if (host->hw->data_pins == 4)
		mmc->caps  |= MMC_CAP_4_BIT_DATA;
	else if (host->hw->data_pins == 8)
		mmc->caps  |= MMC_CAP_8_BIT_DATA | MMC_CAP_4_BIT_DATA;

	if (host->hw->flags & MSDC_UHS1) {
		mmc->caps |= MMC_CAP_UHS_SDR12 | MMC_CAP_UHS_SDR25 |
				MMC_CAP_UHS_SDR50 | MMC_CAP_UHS_SDR104;
		mmc->caps2 |= MMC_CAP2_HS200_1_8V_SDR;
	}

#ifdef CONFIG_EMMC_HS400_SUPPORT
	if (host->hw->flags & MSDC_HS400)
		mmc->caps2 |= MMC_CAP2_HS400_1_8V_DDR;
#endif

	if (host->hw->flags & MSDC_DDR)
		mmc->caps |= MMC_CAP_UHS_DDR50 | MMC_CAP_1_8V_DDR;

	if (!(host->hw->flags & MSDC_REMOVABLE))
		mmc->caps |= MMC_CAP_NONREMOVABLE;

	mmc->caps |= MMC_CAP_ERASE;

	if (host->hw->host_function == MSDC_EMMC)
		mmc->caps |= MMC_CAP_CMD23;

#if defined(CC_MTD_ENCRYPT_SUPPORT) || defined(CC_SUPPORT_CHANNEL_C)
	/* HW limitation(MSDC HW cannot access Channel C) which been fixed in Phoenix
	 * set max_segs to 1 to avoid sending Channel C address to HW
	 * using bounce buffer(allocated in card/queue.c)
	 */
#ifndef CONFIG_MMC_BLOCK_BOUNCE
#error "CONFIG_MMC_BLOCK_BOUNCE _MUST_ be defined!!!"
#endif
	mmc->max_segs	= 1;
	mmc->max_seg_size  = 512 * 1024;
	mmc->max_blk_size  = HOST_MAX_BLKSZ;
	mmc->max_req_size  = 512 * 1024;
	mmc->max_blk_count = mmc->max_req_size;
#else
	/* MMC core transfer sizes tunable parameters */
	mmc->max_segs	= MAX_HW_SGMTS;
	mmc->max_seg_size  = MAX_SGMT_SZ;
	mmc->max_blk_size  = HOST_MAX_BLKSZ;
	mmc->max_req_size  = MAX_REQ_SZ;
	mmc->max_blk_count = mmc->max_req_size;
#endif

	if (host->hw->host_function == MSDC_EMMC)
		mmc->special_arg = MSDC_TRY_MMC;

	if (host->hw->host_function == MSDC_SD)
		mmc->special_arg = MSDC_TRY_SD;
#ifdef CONFIG_MTK_USE_NEW_TUNE
	//for new tune, SDR50, use FIFO path
	msdc_kernel_fifo_path_sel_init(host);
#endif
}

static void msdc_config_host(struct msdc_host *host)
{
	host->error		= 0;
	host->mclk		= 0;				   /* mclk: the request clock of mmc sub-system */
	host->sclk		= 0;				   /* sclk: the really clock after divition */
	host->sclk_src	= MSDC_SCLK_SRC_INVALID;
	host->suspend	= 0;
	host->power_mode = MMC_POWER_OFF;
	host->timeout_ns = 0;
	host->timeout_clks = DEFAULT_DTOC * 1048576;
	host->autocmd = MSDC_AUTOCMD12;
	host->mrq = NULL;
	host->dma.used_gpd = 0;
	host->dma.used_bd = 0;
	host->read_time_tune = 0;
	host->write_time_tune = 0;
	host->rwcmd_time_tune = 0;
	host->write_timeout_uhs104 = 0;
	host->read_timeout_uhs104 = 0;
	host->sw_timeout = 0;
	host->tune = 0;
	host->state = MSDC_STATE_DEFAULT;
	host->block_bad_card = 0;
	host->sd_30_busy = 0;
	host->load_ett_settings = NULL;
	spin_lock_init(&host->lock);
	spin_lock_init(&host->remove_bad_card);
}

static void msdc_config_dma(struct msdc_host *host)
{
	struct device *dev;

#ifdef MSDC_USE_OF
	dev = msdc_dev(host);
#else
	dev = NULL;
#endif
	host->dma.gpd =
		dma_alloc_coherent(dev, MAX_GPD_NUM * sizeof(gpd_t),
				&host->dma.gpd_addr, GFP_KERNEL);
	host->dma.bd =
		dma_alloc_coherent(dev, MAX_BD_NUM * sizeof(bd_t),
				&host->dma.bd_addr, GFP_KERNEL);
	BUG_ON((!host->dma.gpd) || (!host->dma.bd));
	msdc_init_gpd_bd(host, &host->dma);
}

static int msdc_config_others(struct msdc_host *host)
{
	int ret = 0;
	unsigned long irqflags;

	/* SD card power switch(GPIO configuration) */
#ifdef CONFIG_MTKSDMMC_USE
	if (host->hw->host_function == MSDC_SD)
		ret = MsdcSDPowerSwitch(0);
#endif

	/* Initialize the AES encryption module */
#ifdef CC_MTD_ENCRYPT_SUPPORT
	if (host->hw->host_function == MSDC_EMMC)
		msdc_aes_init();
#endif

	/* register the interrupt handler to kernel */
#ifdef MSDC_USE_OF
	irqflags = IRQF_TRIGGER_NONE;
#else
	irqflags = IRQF_TRIGGER_LOW;
#endif
	ret = request_irq((unsigned int)host->irq, msdc_irq, irqflags, DRV_NAME, host);
	if (ret)
		ERR_MSG("request irq %d failed %d\n", host->irq, ret);

	return ret;
}

static int msdc_sd_init_timer(struct mmc_host *mmc)
{
	struct msdc_host *host = mmc_priv(mmc);

#ifdef CONFIG_MTKSDMMC_USE
	/* create a timer for sd card */
	if (host->hw->host_function == MSDC_SD) {
		host->card_detect_timer = kzalloc(sizeof(struct timer_list), GFP_KERNEL);
		if (host->card_detect_timer == NULL)
			return -ENOMEM;

		init_timer(host->card_detect_timer);
		host->card_detect_timer->data = (unsigned long)host;
		host->card_detect_timer->function = msdc_tasklet_card;
		host->card_detect_timer->expires = jiffies + (HZ << 1);
		add_timer(host->card_detect_timer);
	} else
#else
	host->card_detect_timer = NULL;
#endif

	return 0;
}

static int msdc_drv_probe(struct platform_device *pdev)
{
	struct mmc_host *mmc;
	struct msdc_host *host;
	int ret;

	//dev_err(&pdev->dev, "driver probe!\n");

	/* Allocate MMC host for this device */
	mmc = mmc_alloc_host(sizeof(struct msdc_host), &pdev->dev);
	if (!mmc)
		return -ENOMEM;
	host = mmc_priv(mmc);
	host->mmc = mmc;

	ret = msdc_hw_resource_init(host, pdev);
	if (ret)
		goto release;

	msdc_config_mmc(mmc);
	msdc_config_host(host);
	msdc_config_dma(host);
	ret = msdc_config_others(host);
	if (ret)
		goto release;
	msdc_init_hw(host);

	msdc_clock_src[host->id] = host->hw->sclk_src;
	msdc_host_mode[host->id] = mmc->caps;
	mtk_msdc_host[host->id] = host;

	platform_set_drvdata(pdev, mmc);

	/* Scan card here */
	ret = mmc_add_host(mmc);
	if (ret)
		goto irq_free;

	/*
	 * 64bit dma_mask is null in default.
	 */
#ifdef CC_SUPPORT_CHANNEL_C
	if (!IS_IC_MT5891_ES1())
		/* ES1 PCB does not support Channel-C */
		msdc_dma_mask = memblock.memory.regions[memblock.memory.cnt - 1].base - 1;
	else
#endif
	msdc_dma_mask = (max_low_pfn - 2) << PAGE_SHIFT;
	mmc_dev(mmc)->dma_mask = &msdc_dma_mask;

	ret = msdc_sd_init_timer(mmc);
	if (ret)
		goto irq_free;

	return 0;

irq_free:
	free_irq(host->irq, host);
	msdc_deinit_hw(host);
release:
	platform_set_drvdata(pdev, NULL);
	mmc_free_host(mmc);

	return ret;
}

/* 4 device share one driver, using "drvdata" to show difference */
static int msdc_drv_remove(struct platform_device *pdev)
{
	struct mmc_host *mmc;
	struct msdc_host *host;
#ifndef MSDC_USE_OF
	struct resource *mem;
#endif

	mmc  = platform_get_drvdata(pdev);
	BUG_ON(!mmc);

	host = mmc_priv(mmc);
	BUG_ON(!host);

	ERR_MSG("removed !!!\n");

	if (host->card_detect_timer) {
		del_timer(host->card_detect_timer);
		kfree(host->card_detect_timer);
	}

	platform_set_drvdata(pdev, NULL);
	mmc_remove_host(host->mmc);
	msdc_deinit_hw(host);

	free_irq(host->irq, host);

	dma_free_coherent(NULL, MAX_GPD_NUM * sizeof(gpd_t), host->dma.gpd, host->dma.gpd_addr);
	dma_free_coherent(NULL, MAX_BD_NUM	* sizeof(bd_t),  host->dma.bd,	host->dma.bd_addr);

	iounmap(host->base);
	iounmap(host->sclk_base);
	iounmap(host->hclk_base);

#ifndef MSDC_USE_OF
	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (mem)
		release_mem_region(mem->start, mem->end - mem->start + 1);
#endif

	mmc_free_host(host->mmc);

	return 0;
}

static int __init MSDCEMMClog_CommonParamParsing(char *str, int *pi4Dist)
{
	if(strlen(str) != 0)
		pr_err("Parsing String = %s\n", str);
	else {
		pr_err("Parse Error!!!!! string = NULL\n");
		return 0;
	}

	pi4Dist[0] = (int)simple_strtol(str, NULL, 10);
	//pr_err("Parse Done: emmclog = %d\n", pi4Dist[0]);

	return 1;
}

static int __init MSDC_EMMClogParseSetup(char *str)
{
	return MSDCEMMClog_CommonParamParsing(str, &MSDC_EMMClog);
}

__setup("emmclog=", MSDC_EMMClogParseSetup);

static int __init MSDCGPIO_CommonParamParsing(char *str, int *pi4Dist)
{
	char tmp[8]={0};
	char *p,*s;
	int len,i,j;

	if(strlen(str) != 0)
		pr_err("Parsing String = %s \n",str);
	else {
		pr_err("Parse Error!!!!! string = NULL\n");
		return 0;
	}

	for (i=0; i<MSDC_GPIO_MAX_NUMBERS; i++)
	{
		s=str;
		if (i != (MSDC_GPIO_MAX_NUMBERS-1) )
		{
			if(!(p=strchr(str, ',')))
			{
				pr_err("Parse Error!!string format error 1\n");
				break;
			}
			if (!((len=p-s) >= 1))
			{
				pr_err("Parse Error!! string format error 2\n");
				break;
			}
		}
		else
		{
			len = strlen(s);
		}

		for(j=0;j<len;j++)
		{
			tmp[j]=*s;
			s++;
		}
		tmp[j]=0;

		pi4Dist[i] = (int)simple_strtol(&tmp[0], NULL, 10);
		//pr_err("Parse Done: msdc [%d] = %d \n", i, pi4Dist[i]);

		str += (len+1);
	}

	return 1;

}


static int __init MSDC_GpioParseSetup(char *str)
{
	return MSDCGPIO_CommonParamParsing(str, &MSDC_Gpio[0]);
}

__setup("msdcgpio=", MSDC_GpioParseSetup);

static int __init MSDCAutok_CommonParamParsing(char *str, int *pi4Dist)
{
	char tmp[8] = {0};
	char *p, *s;
	int len, i, j;

	if (strlen(str) != 0)
		pr_err("Parsing String = %s \n", str);
	else {
		pr_err("Parse Error!!!!! string = NULL\n");
		return 0;
	}

	for (i = 0; i < MSDC_AUTOK_MAX_NUMBERS; i++) {
		s = str;
		if (i != (MSDC_AUTOK_MAX_NUMBERS-1)) {
			if (!(p = strchr(str, ','))) {
				pr_err("Parse Error!!string format error 1\n");
				break;
			}
			if (!((len = p - s) >= 1)) {
				pr_err("Parse Error!! string format error 2\n");
				break;
			}
		} else {
			len = strlen(s);
		}

		for (j = 0; j < len; j++) {
			tmp[j] = *s;
			s++;
		}
		tmp[j] = 0;

		pi4Dist[i] = (int)simple_strtol(&tmp[0], NULL, 16);
		pr_err("Parse Done: msdc [%d] = 0x%x \n", i, pi4Dist[i]);

		str += (len + 1);
	}

	g_autok_cmdline_parsed = MSDC_CMD_LINE_PARSE_DONE;
	return 1;
}

static int __init MSDC_AutokParseSetup(char *str)
{
	return MSDCAutok_CommonParamParsing(str, &MSDC_Autok[0]);
}

__setup("msdcautok=", MSDC_AutokParseSetup);

/* Fix me: Power Flow */
#ifdef CONFIG_PM
static int msdc_drv_suspend(struct platform_device *pdev, pm_message_t state)
{
	int ret = 0;
	struct mmc_host *mmc = platform_get_drvdata(pdev);
	struct msdc_host *host = mmc_priv(mmc);

	if (mmc && state.event == PM_EVENT_SUSPEND &&
		(host->hw->flags & MSDC_SYS_SUSPEND)) { /* will set for card */
		/* register will be reset to default value after entering suspend */
		host->sclk_src = MSDC_SCLK_SRC_INVALID;
		ret = mmc_suspend_host(mmc);
		msdc_clksrc_onoff(host,0);
	}

	return ret;
}

static int msdc_drv_resume(struct platform_device *pdev)
{
	int ret = 0;
	int retry = 3000;
	int cnt = 1000;
	struct mmc_host *mmc = platform_get_drvdata(pdev);
	struct msdc_host *host = mmc_priv(mmc);
	u32 __iomem *base = host->base;

	msdc_init_hw(host);
	msdc_clksrc_onoff(host,1);

	if (mmc && (host->hw->flags & MSDC_SYS_SUSPEND)) {/* will set for card */
		msdc_retry(!(sdr_read32(MSDC_CFG) & MSDC_CFG_CKSTB), retry, cnt);
		if (retry == 0) {
			ERR_MSG("turn on clock failed ===> retry twice\n");
			msdc_clksrc_onoff(host,0);
			msdc_clksrc_onoff(host,1);
		}
		ret = mmc_resume_host(mmc);
	}

	return ret;
}
#endif

#ifdef MSDC_USE_OF
static const struct of_device_id msdc_of_match[] = {
	{.compatible = "Mediatek,MSDC0", .data = &msdc0_hw},
	{.compatible = "Mediatek,MSDC1", .data = &msdc1_hw},
	{}
};
MODULE_DEVICE_TABLE(of, msdc_of_match);
#endif

static struct platform_driver mt_msdc_driver = {
	.probe	 = msdc_drv_probe,
	.remove  = msdc_drv_remove,
#ifdef CONFIG_PM
	.suspend = msdc_drv_suspend,
	.resume  = msdc_drv_resume,
#endif
	.driver  = {
		.name  = DRV_NAME,
		.owner = THIS_MODULE,
#ifdef MSDC_USE_OF
		.of_match_table = of_match_ptr(msdc_of_match),
#endif
	},
};

/*--------------------------------------------------------------------------*/
/* module init/exit														 */
/*--------------------------------------------------------------------------*/
static int __init mt_msdc_init(void)
{
	int ret;
#ifndef MSDC_USE_OF
	int i;
#endif

#ifdef CC_SUPPORT_CHANNEL_C
	pr_err("####channel c = %#llx, reserved size = %#llx \n",
	(unsigned long long)memblock.memory.regions[memblock.memory.cnt - 1].base,
	(unsigned long long)memblock.memory.regions[memblock.memory.cnt - 1].size);
#endif
	ret = platform_driver_register(&mt_msdc_driver);
	if (ret) {
		pr_err(DRV_NAME ": Can't register driver");
		return ret;
	}
	//pr_err(DRV_NAME ": MediaTek MSDC Driver\n");

#ifndef MSDC_USE_OF
	for (i = 0; i < ARRAY_SIZE(mt_device_msdc); i++) {
		ret = platform_device_register(&mt_device_msdc[i]);
		if (ret != 0)
			return ret;
	}
#endif
	msdc_debug_proc_init();

	return 0;
}

static void __exit mt_msdc_exit(void)
{
#ifndef MSDC_USE_OF
	int i;

	for (i = 0; i < ARRAY_SIZE(mt_device_msdc); i++)
		platform_device_unregister(&mt_device_msdc[i]);
#endif

	platform_driver_unregister(&mt_msdc_driver);
}

module_init(mt_msdc_init);
module_exit(mt_msdc_exit);
#ifdef CONFIG_MTKEMMC_BOOT
EXPORT_SYMBOL(ext_csd);
//EXPORT_SYMBOL(g_emmc_mode_switch);
#endif
EXPORT_SYMBOL(mtk_msdc_host);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("MediaTek SD/MMC Card Driver");

