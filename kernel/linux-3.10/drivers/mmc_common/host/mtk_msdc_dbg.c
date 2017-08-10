#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/mmc/host.h>

#include <asm/io.h>
//for fpga early porting
#include <linux/mmc/mmc.h>
#include <linux/mmc/card.h>
#include <linux/scatterlist.h>

//end for fpga early porting
#include "mtk_msdc_dbg.h"
#include <linux/seq_file.h>


#ifdef MTK_IO_PERFORMANCE_DEBUG
unsigned int g_mtk_mmc_perf_dbg = 0;
unsigned int g_mtk_mmc_dbg_range = 0;
unsigned int g_dbg_range_start = 0;
unsigned int g_dbg_range_end = 0;
unsigned int g_mtk_mmc_dbg_flag = 0;
unsigned int g_dbg_req_count = 0;
unsigned int g_dbg_raw_count = 0;
unsigned int g_dbg_raw_count_old = 0;
unsigned int g_mtk_mmc_clear = 0;
int g_i = 0;
unsigned long long g_req_buf[4000][30] = {{0}};
unsigned long long g_mmcqd_buf[400][300] = {{0}};
char *g_time_mark[] = {"--start dma map this request",
							"--end dma map this request",
							"--start request",
							"--DMA start",
							"--DMA transfer done",
							"--start dma unmap request",
							"--end dma unmap request",
							"--end of request"
};
#endif

/* for get transfer time with each trunk size, default not open */
#ifdef MTK_MMC_PERFORMANCE_TEST
unsigned int g_mtk_mmc_perf_test = 0;
#endif

typedef enum
{
	MODE_NULL = 0,
	SDHC_HIGHSPEED,		/* 0x1 Host supports HS mode */
	UHS_SDR12,			/* 0x2 Host supports UHS SDR12 mode */
	UHS_SDR25,			/* 0x3 Host supports UHS SDR25 mode */
	UHS_SDR50,			/* 0x4 Host supports UHS SDR50 mode */
	UHS_SDR104,			/* 0x5 Host supports UHS SDR104 mode */
	UHS_DDR50,			/* 0x6 Host supports UHS DDR50 mode */

	DRIVER_TYPE_A,		/* 0x7 Host supports Driver Type A */
	DRIVER_TYPE_B,		/* 0x8 Host supports Driver Type B */
	DRIVER_TYPE_C,		/* 0x9 Host supports Driver Type C */
	DRIVER_TYPE_D,		/* 0xA Host supports Driver Type D */

	MAX_CURRENT_200,	/* 0xB Host max current limit is 200mA */
	MAX_CURRENT_400,	/* 0xC Host max current limit is 400mA */
	MAX_CURRENT_600,	/* 0xD Host max current limit is 600mA */
	MAX_CURRENT_800,	/* 0xE Host max current limit is 800mA */

	SDXC_NO_POWER_CONTROL,/*0xF   Host not supports >150mA current at 3.3V /3.0V/1.8V*/
	SDXC_POWER_CONTROL,   /*0x10 Host supports >150mA current at 3.3V /3.0V/1.8V*/
}HOST_CAPS;

static char cmd_buf[256];

/* for debug zone */
unsigned int sd_debug_zone[HOST_MAX_NUM]={
	0,
	0
};

/* mode select */
u32 dma_size[HOST_MAX_NUM]={
	512,
	512
};
msdc_mode drv_mode[HOST_MAX_NUM]={
	MODE_DMA, /* using DMA or not depend on the size */
	MODE_DMA
};
unsigned char msdc_clock_src[HOST_MAX_NUM]={
	0,
	0
};
drv_mod msdc_drv_mode[HOST_MAX_NUM];

u32 msdc_host_mode[HOST_MAX_NUM]={
	0,
	0
};

int sdio_cd_result = 1;


/* for driver profile */
#define TICKS_ONE_MS  (13000)
u32 gpt_enable = 0;
u32 sdio_pro_enable = 0;   /* make sure gpt is enabled */
static unsigned long long sdio_pro_time = 30;	  /* no more than 30s */
static unsigned long long sdio_profiling_start=0;
struct sdio_profile sdio_perfomance = {0};

u32 sdio_enable_tune = 0;
u32 sdio_iocon_dspl = 0;
u32 sdio_iocon_w_dspl = 0;
u32 sdio_iocon_rspl = 0;
u32 sdio_pad_tune_rrdly = 0;
u32 sdio_pad_tune_rdly = 0;
u32 sdio_pad_tune_wrdly = 0;
u32 sdio_dat_rd_dly0_0 = 0;
u32 sdio_dat_rd_dly0_1 = 0;
u32 sdio_dat_rd_dly0_2 = 0;
u32 sdio_dat_rd_dly0_3 = 0;
u32 sdio_dat_rd_dly1_0 = 0;
u32 sdio_dat_rd_dly1_1 = 0;
u32 sdio_dat_rd_dly1_2 = 0;
u32 sdio_dat_rd_dly1_3 = 0;
u32 sdio_clk_drv = 0;
u32 sdio_cmd_drv= 0;
u32 sdio_data_drv =0;
u32 sdio_tune_flag =0;


extern struct msdc_host *mtk_msdc_host[];
#ifndef FPGA_PLATFORM
extern void msdc_set_driving(struct msdc_host* host, struct msdc_hw* hw);
#endif

#if 0
static void msdc_dump_reg(unsigned int base)
{
	u32 id = 0;
	switch(base){
		case MSDC_0_BASE:
#ifdef MTK_EMMC_SUPPORT
			id = 0;
#else
			id = 1;
#endif
			break;
#if defined(CC_MT5883) || defined(CONFIG_ARCH_MT5883)
#else
		case MSDC_1_BASE:
	#ifdef MTK_EMMC_SUPPORT
			id = 1;
	#else
			id = 0;
	#endif
			break;
#endif
		default:
			break;
	}
	pr_err("[SD_Debug][Host%d]Reg[00] MSDC_CFG		 = 0x%.8x\n", id,sdr_read32(base + 0x00));
	pr_err("[SD_Debug][Host%d]Reg[04] MSDC_IOCON	 = 0x%.8x\n", id,sdr_read32(base + 0x04));
	pr_err("[SD_Debug][Host%d]Reg[08] MSDC_PS		 = 0x%.8x\n", id,sdr_read32(base + 0x08));
	pr_err("[SD_Debug][Host%d]Reg[0C] MSDC_INT		 = 0x%.8x\n", id,sdr_read32(base + 0x0C));
	pr_err("[SD_Debug][Host%d]Reg[10] MSDC_INTEN	 = 0x%.8x\n", id,sdr_read32(base + 0x10));
	pr_err("[SD_Debug][Host%d]Reg[14] MSDC_FIFOCS	 = 0x%.8x\n", id,sdr_read32(base + 0x14));
	pr_err("[SD_Debug][Host%d]Reg[18] MSDC_TXDATA	 = not read\n",id);
	pr_err("[SD_Debug][Host%d]Reg[1C] MSDC_RXDATA	 = not read\n",id);
	pr_err("[SD_Debug][Host%d]Reg[30] SDC_CFG		 = 0x%.8x\n", id,sdr_read32(base + 0x30));
	pr_err("[SD_Debug][Host%d]Reg[34] SDC_CMD		 = 0x%.8x\n", id,sdr_read32(base + 0x34));
	pr_err("[SD_Debug][Host%d]Reg[38] SDC_ARG		 = 0x%.8x\n", id,sdr_read32(base + 0x38));
	pr_err("[SD_Debug][Host%d]Reg[3C] SDC_STS		 = 0x%.8x\n", id,sdr_read32(base + 0x3C));
	pr_err("[SD_Debug][Host%d]Reg[40] SDC_RESP0		 = 0x%.8x\n", id,sdr_read32(base + 0x40));
	pr_err("[SD_Debug][Host%d]Reg[44] SDC_RESP1		 = 0x%.8x\n", id,sdr_read32(base + 0x44));
	pr_err("[SD_Debug][Host%d]Reg[48] SDC_RESP2		 = 0x%.8x\n", id,sdr_read32(base + 0x48));
	pr_err("[SD_Debug][Host%d]Reg[4C] SDC_RESP3		 = 0x%.8x\n", id,sdr_read32(base + 0x4C));
	pr_err("[SD_Debug][Host%d]Reg[50] SDC_BLK_NUM	 = 0x%.8x\n", id,sdr_read32(base + 0x50));
	pr_err("[SD_Debug][Host%d]Reg[58] SDC_CSTS		 = 0x%.8x\n", id,sdr_read32(base + 0x58));
	pr_err("[SD_Debug][Host%d]Reg[5C] SDC_CSTS_EN	 = 0x%.8x\n", id,sdr_read32(base + 0x5C));
	pr_err("[SD_Debug][Host%d]Reg[60] SDC_DATCRC_STS = 0x%.8x\n", id,sdr_read32(base + 0x60));
	pr_err("[SD_Debug][Host%d]Reg[70] EMMC_CFG0		 = 0x%.8x\n", id,sdr_read32(base + 0x70));
	pr_err("[SD_Debug][Host%d]Reg[74] EMMC_CFG1		 = 0x%.8x\n", id,sdr_read32(base + 0x74));
	pr_err("[SD_Debug][Host%d]Reg[78] EMMC_STS		 = 0x%.8x\n", id,sdr_read32(base + 0x78));
	pr_err("[SD_Debug][Host%d]Reg[7C] EMMC_IOCON	 = 0x%.8x\n", id,sdr_read32(base + 0x7C));
	pr_err("[SD_Debug][Host%d]Reg[80] SD_ACMD_RESP	 = 0x%.8x\n", id,sdr_read32(base + 0x80));
	pr_err("[SD_Debug][Host%d]Reg[84] SD_ACMD19_TRG  = 0x%.8x\n", id,sdr_read32(base + 0x84));
	pr_err("[SD_Debug][Host%d]Reg[88] SD_ACMD19_STS  = 0x%.8x\n", id,sdr_read32(base + 0x88));
	pr_err("[SD_Debug][Host%d]Reg[90] DMA_SA		 = 0x%.8x\n", id,sdr_read32(base + 0x90));
	pr_err("[SD_Debug][Host%d]Reg[94] DMA_CA		 = 0x%.8x\n", id,sdr_read32(base + 0x94));
	pr_err("[SD_Debug][Host%d]Reg[98] DMA_CTRL		 = 0x%.8x\n", id,sdr_read32(base + 0x98));
	pr_err("[SD_Debug][Host%d]Reg[9C] DMA_CFG		 = 0x%.8x\n", id,sdr_read32(base + 0x9C));
	pr_err("[SD_Debug][Host%d]Reg[A0] SW_DBG_SEL	 = 0x%.8x\n", id,sdr_read32(base + 0xA0));
	pr_err("[SD_Debug][Host%d]Reg[A4] SW_DBG_OUT	 = 0x%.8x\n", id,sdr_read32(base + 0xA4));
	pr_err("[SD_Debug][Host%d]Reg[B0] PATCH_BIT0	 = 0x%.8x\n", id,sdr_read32(base + 0xB0));
	pr_err("[SD_Debug][Host%d]Reg[B4] PATCH_BIT1	 = 0x%.8x\n", id,sdr_read32(base + 0xB4));
	pr_err("[SD_Debug][Host%d]Reg[EC] PAD_TUNE		 = 0x%.8x\n", id,sdr_read32(base + 0xEC));
	pr_err("[SD_Debug][Host%d]Reg[F0] DAT_RD_DLY0	 = 0x%.8x\n", id,sdr_read32(base + 0xF0));
	pr_err("[SD_Debug][Host%d]Reg[F4] DAT_RD_DLY1	 = 0x%.8x\n", id,sdr_read32(base + 0xF4));
	pr_err("[SD_Debug][Host%d]Reg[F8] HW_DBG_SEL	 = 0x%.8x\n", id,sdr_read32(base + 0xF8));
	pr_err("[SD_Debug][Host%d]Rg[100] MAIN_VER		 = 0x%.8x\n", id,sdr_read32(base + 0x100));
	pr_err("[SD_Debug][Host%d]Rg[104] ECO_VER		 = 0x%.8x\n", id,sdr_read32(base + 0x104));
}

static void msdc_set_field(unsigned int address,unsigned int start_bit,unsigned int len,unsigned int value)
{
	unsigned long field;
	if(start_bit > 31 || start_bit < 0|| len > 32 || len <= 0)
		pr_err("[****SD_Debug****]reg filed beyoned (0~31) or length beyoned (1~32)\n");
	else{
		field = ((1 << len) -1) << start_bit;
		value &= (1 << len) -1;
		pr_err("[****SD_Debug****]Original:0x%x (0x%x)\n",address,sdr_read32(address));
		sdr_set_field(address,field, value);
		pr_err("[****SD_Debug****]Modified:0x%x (0x%x)\n",address,sdr_read32(address));
	}
}

static void msdc_get_field(unsigned int address,unsigned int start_bit,unsigned int len,unsigned int value)
{
	unsigned long field;
	if(start_bit > 31 || start_bit < 0|| len > 32 || len <= 0)
		pr_err("[****SD_Debug****]reg filed beyoned (0~31) or length beyoned (1~32)\n");
	else{
		field = ((1 << len) -1) << start_bit;
		sdr_get_field(address,field,value);
		pr_err("[****SD_Debug****]Reg:0x%x start_bit(%d)len(%d)(0x%x)\n",address,start_bit,len,value);
		}
}

static void msdc_init_gpt(void)
{
	GPT_CONFIG config;

	config.num	= GPT6;
	config.mode = GPT_FREE_RUN;
	config.clkSrc = GPT_CLK_SRC_SYS;
	config.clkDiv = GPT_CLK_DIV_1;	 /* 13MHz GPT6 */

	if (GPT_Config(config) == FALSE )
		return;

	GPT_Start(GPT6);
}
#endif

u32 msdc_time_calc(u32 old_L32, u32 old_H32, u32 new_L32, u32 new_H32)
{
	u32 ret = 0;

	if (new_H32 == old_H32) {
		ret = new_L32 - old_L32;
	} else if(new_H32 == (old_H32 + 1)) {
		if (new_L32 > old_L32) {
			pr_err("msdc old_L<0x%x> new_L<0x%x>\n", old_L32, new_L32);
		}
		ret = (0xffffffff - old_L32);
		ret += new_L32;
	} else {
		pr_err("msdc old_H<0x%x> new_H<0x%x>\n", old_H32, new_H32);
	}

	return ret;
}

void msdc_sdio_profile(struct sdio_profile* result)
{
	struct cmd_profile*  cmd;
	u32 i;

	pr_err("sdio === performance dump ===\n");
	pr_err("sdio === total execute tick<%d> time<%dms> Tx<%dB> Rx<%dB>\n",
					result->total_tc, result->total_tc / TICKS_ONE_MS,
					result->total_tx_bytes, result->total_rx_bytes);

	/* CMD52 Dump */
	cmd = &result->cmd52_rx;
	pr_err("sdio === CMD52 Rx <%d>times tick<%d> Max<%d> Min<%d> Aver<%d>\n", cmd->count, cmd->tot_tc,
					cmd->max_tc, cmd->min_tc, cmd->tot_tc/cmd->count);
	cmd = &result->cmd52_tx;
	pr_err("sdio === CMD52 Tx <%d>times tick<%d> Max<%d> Min<%d> Aver<%d>\n", cmd->count, cmd->tot_tc,
					cmd->max_tc, cmd->min_tc, cmd->tot_tc/cmd->count);

	/* CMD53 Rx bytes + block mode */
	for (i=0; i<512; i++) {
		cmd = &result->cmd53_rx_byte[i];
		if (cmd->count) {
			pr_err("sdio<%6d><%3dB>_Rx_<%9d><%9d><%6d><%6d>_<%9dB><%2dM>\n", cmd->count, i, cmd->tot_tc,
							 cmd->max_tc, cmd->min_tc, cmd->tot_tc/cmd->count,
							 cmd->tot_bytes, (cmd->tot_bytes/10)*13 / (cmd->tot_tc/10));
		}
	}
	for (i=0; i<100; i++) {
		cmd = &result->cmd53_rx_blk[i];
		if (cmd->count) {
			pr_err("sdio<%6d><%3d>B_Rx_<%9d><%9d><%6d><%6d>_<%9dB><%2dM>\n", cmd->count, i, cmd->tot_tc,
							 cmd->max_tc, cmd->min_tc, cmd->tot_tc/cmd->count,
							 cmd->tot_bytes, (cmd->tot_bytes/10)*13 / (cmd->tot_tc/10));
		}
	}

	/* CMD53 Tx bytes + block mode */
	for (i=0; i<512; i++) {
		cmd = &result->cmd53_tx_byte[i];
		if (cmd->count) {
			pr_err("sdio<%6d><%3dB>_Tx_<%9d><%9d><%6d><%6d>_<%9dB><%2dM>\n", cmd->count, i, cmd->tot_tc,
							 cmd->max_tc, cmd->min_tc, cmd->tot_tc/cmd->count,
							 cmd->tot_bytes, (cmd->tot_bytes/10)*13 / (cmd->tot_tc/10));
		}
	}
	for (i=0; i<100; i++) {
		cmd = &result->cmd53_tx_blk[i];
		if (cmd->count) {
			pr_err("sdio<%6d><%3d>B_Tx_<%9d><%9d><%6d><%6d>_<%9dB><%2dM>\n", cmd->count, i, cmd->tot_tc,
							 cmd->max_tc, cmd->min_tc, cmd->tot_tc/cmd->count,
							 cmd->tot_bytes, (cmd->tot_bytes/10)*13 / (cmd->tot_tc/10));
		}
	}

	pr_err("sdio === performance dump done ===\n");
}

//========= sdio command table ===========
void msdc_performance(u32 opcode, u32 sizes, u32 bRx, u32 ticks)
{
	struct sdio_profile* result = &sdio_perfomance;
	struct cmd_profile*  cmd;
	u32 block;
	long long endtime;

	if (sdio_pro_enable == 0) {
		return;
	}

	if (opcode == 52) {
		cmd = bRx ?  &result->cmd52_rx : &result->cmd52_tx;
	} else if (opcode == 53) {
		if (sizes < 512) {
			cmd = bRx ?  &result->cmd53_rx_byte[sizes] : &result->cmd53_tx_byte[sizes];
		} else {
			block = sizes / 512;
			if (block >= 99) {
			   pr_err("cmd53 error blocks\n");
			   while(1);
			}
			cmd = bRx ?  &result->cmd53_rx_blk[block] : &result->cmd53_tx_blk[block];
		}
	} else {
		return;
	}

	/* update the members */
	if (ticks > cmd->max_tc){
		cmd->max_tc = ticks;
	}
	if (cmd->min_tc == 0 || ticks < cmd->min_tc) {
		cmd->min_tc = ticks;
	}
	cmd->tot_tc += ticks;
	cmd->tot_bytes += sizes;
	cmd->count ++;

	if (bRx) {
		result->total_rx_bytes += sizes;
	} else {
		result->total_tx_bytes += sizes;
	}
	result->total_tc += ticks;
#if 0
	/* dump when total_tc > 30s */
	if (result->total_tc >= sdio_pro_time * TICKS_ONE_MS * 1000) {
		msdc_sdio_profile(result);
		memset(result, 0 , sizeof(struct sdio_profile));
	}
#endif



	 endtime = sched_clock();
	 if((endtime-sdio_profiling_start)>=  sdio_pro_time * 1000000000) {
	 msdc_sdio_profile(result);
	 memset(result, 0 , sizeof(struct sdio_profile));
	 sdio_profiling_start = endtime;
	 }


}



#define COMPARE_ADDRESS_MMC   0x0
#define COMPARE_ADDRESS_SD	  0x2000
#define COMPARE_ADDRESS_SDIO  0x0
#define COMPARE_ADDRESS_SD_COMBO  0x2000

#define MSDC_MULTI_BUF_LEN	(4*1024)
u32 msdc_multi_wbuf[MSDC_MULTI_BUF_LEN];
u32 msdc_multi_rbuf[MSDC_MULTI_BUF_LEN];

#define is_card_present(h)	   (((struct msdc_host*)(h))->card_inserted)

extern struct msdc_host *mtk_msdc_host[];
static int sd_multi_rw_compare_slave(int host_num, int read, uint address)
{
	struct scatterlist msdc_sg;
	struct mmc_data  msdc_data;
	struct mmc_command msdc_cmd;
	struct mmc_command msdc_stop;
	struct mmc_request	msdc_mrq;
	struct msdc_host *host_ctl;
	//struct msdc_host *host = mtk_msdc_host[host_num];
	int result = 0, forIndex = 0;
	u8 *wPtr;
	u8 wData[16]= {
			0x67, 0x45, 0x23, 0x01,
			0xef, 0xcd, 0xab, 0x89,
			0xce, 0x8a, 0x46, 0x02,
			0xde, 0x9b, 0x57, 0x13 };

	host_ctl = mtk_msdc_host[host_num];
	if(!host_ctl || !host_ctl->mmc || !host_ctl->mmc->card)
	{
		pr_err(" there is no card initialized in host[%d]\n",host_num);
		return -1;
	}

	if(!is_card_present(host_ctl))
	{
		pr_err("  [%s]: card is removed!\n", __func__);
		return -1;
	}

	mmc_claim_host(host_ctl->mmc);

	memset(&msdc_data, 0, sizeof(struct mmc_data));
	memset(&msdc_mrq, 0, sizeof(struct mmc_request));
	memset(&msdc_cmd, 0, sizeof(struct mmc_command));
	memset(&msdc_stop, 0, sizeof(struct mmc_command));

	msdc_mrq.cmd = &msdc_cmd;
	msdc_mrq.data = &msdc_data;

	if (read){
		//init read command
		msdc_data.flags = MMC_DATA_READ;
		msdc_cmd.opcode = MMC_READ_MULTIPLE_BLOCK;
		msdc_data.blocks = sizeof(msdc_multi_rbuf) / 512;
		wPtr =(u8*)msdc_multi_rbuf;
		//init read buffer
		for(forIndex=0;forIndex<MSDC_MULTI_BUF_LEN*4;forIndex++)
			*(wPtr + forIndex) = 0x0;
		//for(forIndex=0;forIndex<MSDC_MULTI_BUF_LEN;forIndex++)
		//pr_err("R_buffer[0x%x] \n",msdc_multi_rbuf[forIndex]);
	} else {
		//init write command
		msdc_data.flags = MMC_DATA_WRITE;
		msdc_cmd.opcode = MMC_WRITE_MULTIPLE_BLOCK;
		msdc_data.blocks = sizeof(msdc_multi_wbuf) / 512;
		//init write buffer
		wPtr =(u8*)msdc_multi_wbuf;
		for(forIndex=0;forIndex<MSDC_MULTI_BUF_LEN*4;forIndex++)
			*(wPtr + forIndex) = wData[forIndex%16];
		//for(forIndex=0;forIndex<MSDC_MULTI_BUF_LEN;forIndex++)
		//pr_err("W_buffer[0x%x]\n",msdc_multi_wbuf[forIndex]);
	}

	msdc_cmd.arg = address;

	BUG_ON(!host_ctl->mmc->card);
	if (!mmc_card_blockaddr(host_ctl->mmc->card)){
		//pr_err("this device use byte address!!\n");
		msdc_cmd.arg <<= 9;
	}
	msdc_cmd.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_ADTC;

	msdc_stop.opcode = MMC_STOP_TRANSMISSION;
	msdc_stop.arg = 0;
	msdc_stop.flags = MMC_RSP_SPI_R1B | MMC_RSP_R1B | MMC_CMD_AC;

	msdc_data.stop = &msdc_stop;
	msdc_data.blksz = 512;
	msdc_data.sg = &msdc_sg;
	msdc_data.sg_len = 1;

	if(read)
		sg_init_one(&msdc_sg, msdc_multi_rbuf, sizeof(msdc_multi_rbuf));
	else
		sg_init_one(&msdc_sg, msdc_multi_wbuf, sizeof(msdc_multi_wbuf));

	mmc_set_data_timeout(&msdc_data, host_ctl->mmc->card);
	mmc_wait_for_req(host_ctl->mmc, &msdc_mrq);

	if (read){
		for(forIndex=0;forIndex<MSDC_MULTI_BUF_LEN;forIndex++){
			//pr_err("index[%d]\tW_buffer[0x%x]\tR_buffer[0x%x]\t\n", forIndex, msdc_multi_wbuf[forIndex], msdc_multi_rbuf[forIndex]);
			if(msdc_multi_wbuf[forIndex]!=msdc_multi_rbuf[forIndex]){
				pr_err("index[%d]\tW_buffer[0x%x]\tR_buffer[0x%x]\tfailed\n", forIndex, msdc_multi_wbuf[forIndex], msdc_multi_rbuf[forIndex]);
				result =-1;
			}
		}
		/*if(result == 0)
			pr_err("pid[%d][%s]: data compare successed!!\n", current->pid, __func__);
		else
			pr_err("pid[%d][%s]: data compare failed!! \n", current->pid, __func__);*/
	}

	mmc_release_host(host_ctl->mmc);

	if (msdc_cmd.error)
		result = msdc_cmd.error;

	if (msdc_data.error){
		result = msdc_data.error;
	} else {
		result = 0;
	}

	return result;
}

static int sd_multi_rw_compare(int host_num, uint address, int count)
{
	int i=0, j=0;
	int error = 0;

	pr_err("==Start data compare! addr 0x%x, count %d \n",address,count);

	for(i=0; i<count; i++)
	{
		//pr_err("============ cpu[%d] pid[%d]: start the %d time compare ============\n", task_cpu(current), current->pid, i);

		error = sd_multi_rw_compare_slave(host_num, 0, address); //write
		if(error)
		{
			pr_err("[%s]: failed to write data, error=%d\n", __func__, error);
			break;
		}

		for(j=0; j<1; j++){
			error = sd_multi_rw_compare_slave(host_num, 1, address); //read
			if(error)
			{
				pr_err("[%s]: failed to read data, error=%d\n", __func__, error);
				break;
			}
		}
		if(error)
		   pr_err("============ cpu[%d] pid[%d]: FAILED the %d time compare ============\n", task_cpu(current), current->pid, i);
		else
		   pr_err("============ cpu[%d] pid[%d]: FINISH the %d time compare ============\n", task_cpu(current), current->pid, i);
	}

	if (i == count)
		pr_err("pid[%d]: successed to compare data within %d times\n", current->pid, count);

	return error;
}



#define MAX_THREAD_NUM_FOR_SMP 20

static uint smp_address_on_sd[MAX_THREAD_NUM_FOR_SMP] =
{
  0x2000,
  0x20000,
  0x200000,
  0x2000000,
  0x2200000,
  0x2400000,
  0x2800000,
  0x2c00000,
  0x4000000,
  0x4200000,
  0x4400000,
  0x4800000,
  0x4c00000,
  0x8000000,
  0x8200000,
  0x8400000,
  0x8800000,
  0x8c00000,
  0xc000000,
  0xc200000
};

static uint smp_address_on_mmc[MAX_THREAD_NUM_FOR_SMP] =
{
  0x200,
  0x2000,
  0x20000,
  0x200000,
  0x2000000,
  0x2200000,
  0x2400000,
  0x2800000,
  0x2c00000,
  0x4000000,
  0x4200000,
  0x4400000,
  0x4800000,
  0x4c00000,
  0x8000000,
  0x8200000,
  0x8400000,
  0x8800000,
  0x8c00000,
  0xc000000,
};

static uint smp_address_on_sd_combo[MAX_THREAD_NUM_FOR_SMP] =
{
  0x2000,
  0x20000,
  0x200000,
  0x2000000,
  0x2200000,
  0x2400000,
  0x2800000,
  0x2c00000,
  0x4000000,
  0x4200000,
  0x4400000,
  0x4800000,
  0x4c00000,
  0x8000000,
  0x8200000,
  0x8400000,
  0x8800000,
  0x8c00000,
  0xc000000,
  0xc200000
};
struct write_read_data{
	int host_id;		//the target host you want to do SMP test on.
	uint start_address; //where you want to do write/read of the memory card
	int count;			//how many times you want to do read after write bit by bit comparison
};

static struct write_read_data wr_data[HOST_MAX_NUM][MAX_THREAD_NUM_FOR_SMP];
/*
 * 2012-03-25
 * the SMP thread function
 * do read after write the memory card, and bit by bit comparison
 */
static int write_read_thread(void* ptr)
{
	struct write_read_data* data = (struct write_read_data*)ptr;
	sd_multi_rw_compare(data->host_id, data->start_address, data->count);
	return 0;
}

/*
 * 2012-03-25
 * function:		 do SMP test on the same one MSDC host
 * thread_num:		 the number of thread you want to trigger on this host.
 * host_id:			 the target host you want to do SMP test on.
 * count:			 how many times you want to do read after write bit by bit comparison in each thread.
 * multi_address:	 whether do read/write the same/different address of the memory card in each thread.
 */
static int smp_test_on_one_host(int thread_num, int host_id, int count, int multi_address)
{
   int i=0, ret=0;
   char thread_name[128];
   struct msdc_host *host_ctl;

   pr_err("============================[%s] start ================================\n\n", __func__);
   pr_err(" host %d run %d thread, each thread run %d RW comparison\n", host_id, thread_num, count);
   if(host_id >= HOST_MAX_NUM || host_id < 0)
   {
	  pr_err(" bad host id: %d\n", host_id);
	  ret = -1;
	  goto out;
   }
   if(thread_num > MAX_THREAD_NUM_FOR_SMP)// && (multi_address != 0))
   {
	  pr_err(" too much thread for SMP test, thread_num=%d\n", thread_num);
	  ret = -1;
	  goto out;
   }

   host_ctl = mtk_msdc_host[host_id];
   if(!host_ctl || !host_ctl->mmc || !host_ctl->mmc->card)
   {
	  pr_err(" there is no card initialized in host[%d]\n",host_id);
	  ret = -1;
	  goto out;
   }


   for(i=0; i<thread_num; i++)
   {
	   switch(host_ctl->mmc->card->type)
	   {
		 case MMC_TYPE_MMC:
			if(!multi_address)
			   wr_data[host_id][i].start_address = COMPARE_ADDRESS_MMC;
			else
			   wr_data[host_id][i].start_address = smp_address_on_mmc[i];
			if(i == 0)
			   pr_err(" MSDC[%d], MMC:\n", host_id);
			break;
		 case MMC_TYPE_SD:
			if(!multi_address)
			   wr_data[host_id][i].start_address = COMPARE_ADDRESS_SD;
			else
			   wr_data[host_id][i].start_address = smp_address_on_sd[i];
			if(i == 0)
			   pr_err(" MSDC[%d], SD:\n", host_id);
			break;
		 case MMC_TYPE_SDIO:
			if(i == 0)
			{
			   pr_err(" MSDC[%d], SDIO:\n", host_id);
			   pr_err("   please manually trigger wifi application instead of write/read something on SDIO card\n");
			}
			ret = -1;
			goto out;
		 case MMC_TYPE_SD_COMBO:
			if(!multi_address)
			   wr_data[host_id][i].start_address = COMPARE_ADDRESS_SD_COMBO;
			else
			   wr_data[host_id][i].start_address = smp_address_on_sd_combo[i];
			if(i == 0)
			   pr_err(" MSDC[%d], SD_COMBO:\n", host_id);
			break;
		 default:
			if(i == 0)
			   pr_err(" MSDC[%d], cannot recognize this card\n", host_id);
			ret = -1;
			goto out;
	   }
	   wr_data[host_id][i].host_id = host_id;
	   wr_data[host_id][i].count = count;
	   sprintf(thread_name, "msdc_H%d_T%d", host_id, i);
	   kthread_run(write_read_thread, &wr_data[host_id][i], thread_name);
	   pr_err("   start thread: %s, at address 0x%x\n", thread_name, wr_data[host_id][i].start_address);
   }
out:
   pr_err("============================[%s] end ================================\n\n", __func__);
   return ret;
}

/*
 * 2012-03-25
 * function:		 do SMP test on all MSDC hosts
 * thread_num:		 the number of thread you want to trigger on this host.
 * count:			 how many times you want to do read after write bit by bit comparison in each thread.
 * multi_address:	 whether do read/write the same/different address of the memory card in each thread.
 */
static int smp_test_on_all_host(int thread_num, int count, int multi_address)
{
   int i=0;
   int j=0;
   int ret=0;
   char thread_name[128];
   struct msdc_host *host_ctl;

   pr_err("============================[%s] start ================================\n\n", __func__);
   pr_err(" each host run %d thread, each thread run %d RW comparison\n", thread_num, count);
   if(thread_num > MAX_THREAD_NUM_FOR_SMP) //&& (multi_address != 0))
   {
	  pr_err(" too much thread for SMP test, thread_num=%d\n", thread_num);
	  ret = -1;
	  goto out;
   }

   for(i=0; i<HOST_MAX_NUM; i++)
   {
	  host_ctl = mtk_msdc_host[i];
	  if(!host_ctl || !host_ctl->mmc || !host_ctl->mmc->card)
	  {
		 pr_err(" MSDC[%d], no card is initialized\n", i);
		 continue;
	  }
	  if(host_ctl->mmc->card->type == MMC_TYPE_SDIO)
	  {
		 pr_err(" MSDC[%d], SDIO, please manually trigger wifi application instead of write/read something on SDIO card\n", i);
		 continue;
	  }
	  for(j=0; j<thread_num; j++)
	  {
		 wr_data[i][j].host_id = i;
		 wr_data[i][j].count = count;
		 switch(host_ctl->mmc->card->type)
		 {
			case MMC_TYPE_MMC:
			   if(!multi_address)
				  wr_data[i][j].start_address = COMPARE_ADDRESS_MMC;
			   else
				  wr_data[i][j].start_address = smp_address_on_mmc[i];
			   if(j == 0)
				  pr_err(" MSDC[%d], MMC:\n ", i);
			   break;
			case MMC_TYPE_SD:
			   if(!multi_address)
				  wr_data[i][j].start_address = COMPARE_ADDRESS_SD;
			   else
				  wr_data[i][j].start_address = smp_address_on_sd[i];
			   if(j == 0)
				  pr_err(" MSDC[%d], SD:\n", i);
			   break;
			case MMC_TYPE_SDIO:
			   if(j == 0)
			   {
				  pr_err(" MSDC[%d], SDIO:\n", i);
				  pr_err("	 please manually trigger wifi application instead of write/read something on SDIO card\n");
			   }
			   ret = -1;
			   goto out;
			case MMC_TYPE_SD_COMBO:
			   if(!multi_address)
				  wr_data[i][j].start_address = COMPARE_ADDRESS_SD_COMBO;
			   else
				  wr_data[i][j].start_address = smp_address_on_sd_combo[i];
			   if(j == 0)
				  pr_err(" MSDC[%d], SD_COMBO:\n", i);
			   break;
			default:
			   if(j == 0)
				  pr_err(" MSDC[%d], cannot recognize this card\n", i);
			   ret = -1;
			   goto out;
		 }
		 sprintf(thread_name, "msdc_H%d_T%d", i, j);
		 kthread_run(write_read_thread, &wr_data[i][j], thread_name);
		 pr_err("	start thread: %s, at address: 0x%x\n", thread_name, wr_data[i][j].start_address);
	  }
   }
out:
   pr_err("============================[%s] end ================================\n\n", __func__);
   return ret;
}


static int msdc_help_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "\n====================[msdc_help]=====================\n");

	seq_printf(m, "\n	LOG control:		   echo %x [host_id] [debug_zone] > msdc_debug\n", SD_TOOL_ZONE);
	seq_printf(m, "			 [debug_zone]		DMA:0x1,  CMD:0x2,	RSP:0x4,   INT:0x8,   CFG:0x10,  FUC:0x20,\n");
	seq_printf(m, "								OPS:0x40, FIO:0x80, WRN:0x100, PWR:0x200, CLK:0x400, RW:0x1000, NRW:0x2000\n");
	seq_printf(m, "\n	DMA mode:\n");
	seq_printf(m, "			 set DMA mode:		echo %x 0 [host_id] [dma_mode] [dma_size] > msdc_debug\n", SD_TOOL_DMA_SIZE);
	seq_printf(m, "			 get DMA mode:		echo %x 1 [host_id] > msdc_debug\n", SD_TOOL_DMA_SIZE);
	seq_printf(m, "			   [dma_mode]		0:PIO, 1:DMA, 2:SIZE_DEP\n");
	seq_printf(m, "			   [dma_size]		valid for SIZE_DEP mode, the min size can trigger the DMA mode\n");
	seq_printf(m, "\n	SDIO profile:		   echo %x [enable] [time] > msdc_debug\n", SD_TOOL_SDIO_PROFILE);
	seq_printf(m, "\n	CLOCK control:	\n");
	seq_printf(m, "			 set clk src:		echo %x 0 [host_id] [clk_src] > msdc_debug\n", SD_TOOL_CLK_SRC_SELECT);
	seq_printf(m, "			 get clk src:		echo %x 1 [host_id] > msdc_debug\n", SD_TOOL_CLK_SRC_SELECT);
	seq_printf(m, "			   [clk_src]		0:SYS PLL(26Mhz), 1:3G PLL(197Mhz), 2:AUD PLL(208Mhz)\n");
	seq_printf(m, "\n	REGISTER control:\n");
	seq_printf(m, "			 write register:	echo %x 0 [host_id] [register_offset] [value] > msdc_debug\n", SD_TOOL_REG_ACCESS);
	seq_printf(m, "			 read register:		echo %x 1 [host_id] [register_offset] > msdc_debug\n", SD_TOOL_REG_ACCESS);
	seq_printf(m, "			 write mask:		echo %x 2 [host_id] [register_offset] [start_bit] [len] [value] > msdc_debug\n", SD_TOOL_REG_ACCESS);
	seq_printf(m, "			 read mask:			echo %x 3 [host_id] [register_offset] [start_bit] [len] > msdc_debug\n", SD_TOOL_REG_ACCESS);
	seq_printf(m, "			 dump all:			echo %x 4 [host_id]> msdc_debug\n", SD_TOOL_REG_ACCESS);
	seq_printf(m, "\n	DRVING control: \n");
	seq_printf(m, "			 set driving:		echo %x 0 [host_id] [clk_drv] [cmd_drv] [dat_drv] > msdc_debug\n", SD_TOOL_SET_DRIVING);
	seq_printf(m, "			 get driving:		echo %x 1 [host_id] > msdc_debug\n", SD_TOOL_SET_DRIVING);
	seq_printf(m, "\n	DESENSE control: \n");
	seq_printf(m, "			 write register:	echo %x 0 [value] > msdc_debug\n", SD_TOOL_DESENSE);
	seq_printf(m, "			 read register:		echo %x 1 > msdc_debug\n", SD_TOOL_DESENSE);
	seq_printf(m, "			 write mask:		echo %x 2 [start_bit] [len] [value] > msdc_debug\n", SD_TOOL_DESENSE);
	seq_printf(m, "			 read mask:			echo %x 3 [start_bit] [len] > msdc_debug\n", SD_TOOL_DESENSE);
	seq_printf(m, "\n	RW_COMPARE test:	   echo %x [host_id] [compare_count] > msdc_debug\n", RW_BIT_BY_BIT_COMPARE);
	seq_printf(m, "			 [compare_count]	how many time you want to \"write=>read=>compare\"\n");
	seq_printf(m, "\n	SMP_ON_ONE_HOST test:  echo %x [host_id] [thread_num] [compare_count] [multi_address] > msdc_debug\n", SMP_TEST_ON_ONE_HOST);
	seq_printf(m, "			 [thread_num]		how many R/W comparision thread you want to run at host_id\n");
	seq_printf(m, "			 [compare_count]	how many time you want to \"write=>read=>compare\" in each thread\n");
	seq_printf(m, "			 [multi_address]	whether read/write different address in each thread, 0:No, 1:Yes\n");
	seq_printf(m, "\n	SMP_ON_ALL_HOST test:  echo %x [thread_num] [compare_count] [multi_address] > msdc_debug\n", SMP_TEST_ON_ALL_HOST);
	seq_printf(m, "			 [thread_num]		how many R/W comparision thread you want to run at each host\n");
	seq_printf(m, "			 [compare_count]	how many time you want to \"write=>read=>compare\" in each thread\n");
	seq_printf(m, "			 [multi_address]	whether read/write different address in each thread, 0:No, 1:Yes\n");
	seq_printf(m, "\n	SPEED_MODE control:\n");
	seq_printf(m, "			 set speed mode:	echo %x 0 [host_id] [speed_mode] [driver_type] [max_current] [power_control] > msdc_debug\n", SD_TOOL_MSDC_HOST_MODE);
	seq_printf(m, "			 get speed mode:	echo %x 1 [host_id]\n", SD_TOOL_MSDC_HOST_MODE);
	seq_printf(m, "			   [speed_mode]		  0:N/A,  1:HS,		 2:SDR12,	3:SDR25,   4:SDR:50,  5:SDR104,  6:DDR\n");
	seq_printf(m, "			   [driver_type]	  0:N/A,  7:type A,  8:type B,	9:type C,  a:type D\n");
	seq_printf(m, "			   [max_current]	  0:N/A,  b:200mA,	 c:400mA,	d:600mA,   e:800mA\n");
	seq_printf(m, "			   [power_control]	  0:N/A,  f:disable, 10:enable\n");
	seq_printf(m, "\n	DMA viloation:		   echo %x [host_id] [ops]> msdc_debug\n", SD_TOOL_DMA_STATUS);
	seq_printf(m, "			 [ops]				0:get latest dma address,  1:start violation test\n");
	seq_printf(m, "\n	NOTE: All input data is Hex number! \n");

	seq_printf(m, "\n======================================================\n\n");

	return 0;
}

//========== driver proc interface ===========
static int msdc_debug_proc_show(struct seq_file *m, void *v)
{

	seq_printf(m, "\n=========================================\n");

	seq_printf(m, "Index<0> + Id + Zone\n");
	seq_printf(m, "-> PWR<9> WRN<8> | FIO<7> OPS<6> FUN<5> CFG<4> | INT<3> RSP<2> CMD<1> DMA<0>\n");
	seq_printf(m, "-> echo 0 3 0x3ff >msdc_bebug -> host[3] debug zone set to 0x3ff\n");
	seq_printf(m, "-> MSDC[0] Zone: 0x%.8x\n", sd_debug_zone[0]);
	seq_printf(m, "-> MSDC[1] Zone: 0x%.8x\n", sd_debug_zone[1]);

	seq_printf(m, "Index<1> + ID:4|Mode:4 + DMA_SIZE\n");
	seq_printf(m, "-> 0)PIO 1)DMA 2)SIZE\n");
	seq_printf(m, "-> echo 1 22 0x200 >msdc_bebug -> host[2] size mode, dma when >= 512\n");
	seq_printf(m, "-> MSDC[0] mode<%d> size<%d>\n", drv_mode[0], dma_size[0]);
	seq_printf(m, "-> MSDC[1] mode<%d> size<%d>\n", drv_mode[1], dma_size[1]);

	seq_printf(m, "Index<3> + SDIO_PROFILE + TIME\n");
	seq_printf(m, "-> echo 3 1 0x1E >msdc_bebug -> enable sdio_profile, 30s\n");
	seq_printf(m, "-> SDIO_PROFILE<%d> TIME<%llu s>\n", sdio_pro_enable, sdio_pro_time);
	seq_printf(m, "-> Clokc SRC selection Host[0]<%d>\n", msdc_clock_src[0]);
	seq_printf(m, "-> Clokc SRC selection Host[1]<%d>\n", msdc_clock_src[1]);

	seq_printf(m, "=========================================\n\n");
	return 0;
}
int __cmd_tuning_test = 0;
static int msdc_debug_proc_write(struct file *file, const char *buf, unsigned long count, void *data)
{
	int ret;

	int cmd, p1, p2, p3, p4, p5, p6;
	int id, zone;
	int mode, size;
	int thread_num, compare_count, multi_address;
	unsigned int base;
	unsigned int offset = 0;
	unsigned int reg_value;
	HOST_CAPS spd_mode = MODE_NULL;
	HOST_CAPS drv_type = MODE_NULL;
	HOST_CAPS current_limit = MODE_NULL;
	HOST_CAPS pw_cr = MODE_NULL;
	struct msdc_host *host = NULL;
	struct dma_addr *dma_address, *p_dma_address;
	int dma_status;

	if (count == 0)
		return -1;
	if (count > 255)
		count = 255;

	ret = copy_from_user(cmd_buf, buf, count);
	if (ret < 0)
		return -1;

	cmd_buf[count] = '\0';
	pr_err("[****SD_Debug****]msdc Write %s\n", cmd_buf);

	sscanf(cmd_buf, "%x %x %x %x %x %x %x", &cmd, &p1, &p2, &p3, &p4, &p5, &p6);

	if (cmd == 0xff)//force HS400 to met error, then do autok
	{
		
		unsigned int __iomem *base;

		host = mtk_msdc_host[p1];
		if (host == NULL)
			return count;
		base = host->base;


		
		sdr_set_field(EMMC50_PAD_DS_TUNE,MSDC_EMMC50_PAD_DS_TUNE_DLY1,0);

		pr_err("force set DS delay to 0 \n");




	}
	if (cmd == RW_BIT_BY_BIT_COMPARE) {
		sd_multi_rw_compare(0, 0, p1);
		return 0;
	} else if(cmd == SD_TOOL_ZONE) {
		if (0xeeeeeeee == p2) {
			__cmd_tuning_test = -1;
			return count;
		}
		id = p1; zone = p2; //zone &= 0x3ff;
		pr_err("[****SD_Debug****]msdc host_id<%d> zone<0x%.8x>\n", id, zone);
		if (id >= 0 && id <= HOST_MAX_NUM - 1)
			sd_debug_zone[id] = zone;
		else if(id == HOST_MAX_NUM) {
			sd_debug_zone[0] = sd_debug_zone[1] = zone;
			sd_debug_zone[2] = sd_debug_zone[3] = sd_debug_zone[4] = zone;
		} else
			pr_err("[****SD_Debug****]msdc host_id error when set debug zone\n");
	} else if (cmd == SMP_TEST_ON_ONE_HOST) {
		id = p1;
		thread_num = p2;
		compare_count = p3;
		multi_address = p4;
		smp_test_on_one_host(thread_num, id, compare_count, multi_address);
	} else if (cmd == SMP_TEST_ON_ALL_HOST) {
		thread_num = p1;
		compare_count = p2;
		multi_address = p3;
		smp_test_on_all_host(thread_num, compare_count, multi_address);
	} else if (cmd == PRINT_CUR_STATUS) {
		unsigned int reg;
		unsigned int __iomem *base;

		host = mtk_msdc_host[p1];
		if (host == NULL)
			return count;
		base = host->base;
		pr_err("===========================================\n");
		pr_err("MSDC%d Configurations:\n", p1);
		pr_err("\tCLK:\n");
		pr_err("\t\tsclk source: [0x%x] = 0x%x\n", host->sclk_base,
				sdr_read32(host->sclk_base));
		pr_err("\t\thclk source: [0x%x] = 0x%x\n", host->hclk_base,
				sdr_read32(host->hclk_base));
		pr_err("\t\tsclk source freq: %dHz\n", msdc_cur_sclk_freq(host));
		pr_err("\t\tThe real clock freq: %dHz\n", host->sclk);
		pr_err("\t\tMSDC_CFG:\n");
		sdr_get_field(MSDC_CFG, MSDC_CFG_START_BIT, reg);
		if (reg == 0)
			pr_err("\t\t\tstart bit: Rising edge.\n");
		else if (reg == 1)
			pr_err("\t\t\tstart bit: Falling edge.\n");
		else if (reg == 2)
			pr_err("\t\t\tstart bit: Rising and Falling edge.\n");
		else
			pr_err("\t\t\tstart bit: Rising or Falling edge.\n");

		sdr_get_field(MSDC_CFG, MSDC_CFG_CKMOD, reg);
		if (reg == 0) {
			pr_err("\t\t\tmode: Use clock divider.\n");
		} else if (reg == 1) {
			pr_err("\t\t\tmode: Use clock source directly, no divide.\n");
		} else if (reg == 2) {
			pr_err("\t\t\tmode: DDR mode, use clock divider.\n");
		} else {
			pr_err("\t\t\tmode: HS400 mode, use clock divider.\n");
		}

		if (reg == 1) {
			pr_err("\t\t\tdivide: not used.\n");
		} else {
			sdr_get_field(MSDC_CFG, MSDC_CFG_CKDIV, reg);
			pr_err("\t\t\tdivide: 1\/%d\n", reg == 0 ? 2 : (reg << 2));
		}

		pr_err("===========================================\n");
	} else if (cmd == MMC_ACCESS_READ) {
		int count = 0;
		unsigned char temp_buf[1024] = {0};
		while (count < p4) {
			memset(temp_buf, 0, 1024 * sizeof(unsigned char));
			pr_err("call rwaccess_read\n");
			pr_err("partno = %x, offset = %x, size = %x\n", p1 , p2, p3);
			msdc_partread_rwaccess(p1, p2, p3, temp_buf);
			count ++;
		}
	} else if (cmd == MMC_ACCESS_WRITE) {
		int count = 0;
		unsigned char temp_buf[1024] = {0};
		while (count < p4) {
			memset(temp_buf, 0xAB, 1024 * sizeof(unsigned char));
			pr_err("\n call rwaccess_write \n");
			pr_err("partno = %x, offset = %x, size = %x\n", p1, p2, p3);
			msdc_partwrite_rwaccess(p1, p2, p3, temp_buf);
			count ++;
		}
	}

	return count;
}

static int msdc_tune_flag_proc_read_show(struct seq_file *m, void *data)
{
	seq_printf(m, "0x%X\n", sdio_tune_flag);
	return 0;
}

static int msdc_tune_proc_read_show(struct seq_file *m, void *data)
{
	seq_printf(m, "\n=========================================\n");
	seq_printf(m, "sdio_enable_tune: 0x%.8x\n", sdio_enable_tune);
	seq_printf(m, "sdio_iocon_dspl: 0x%.8x\n", sdio_iocon_dspl);
	seq_printf(m, "sdio_iocon_w_dspl: 0x%.8x\n", sdio_iocon_w_dspl);
	seq_printf(m, "sdio_iocon_rspl: 0x%.8x\n", sdio_iocon_rspl);
	seq_printf(m, "sdio_pad_tune_rrdly: 0x%.8x\n", sdio_pad_tune_rrdly);
	seq_printf(m, "sdio_pad_tune_rdly: 0x%.8x\n", sdio_pad_tune_rdly);
	seq_printf(m, "sdio_pad_tune_wrdly: 0x%.8x\n", sdio_pad_tune_wrdly);
	seq_printf(m, "sdio_dat_rd_dly0_0: 0x%.8x\n", sdio_dat_rd_dly0_0);
	seq_printf(m, "sdio_dat_rd_dly0_1: 0x%.8x\n", sdio_dat_rd_dly0_1);
	seq_printf(m, "sdio_dat_rd_dly0_2: 0x%.8x\n", sdio_dat_rd_dly0_2);
	seq_printf(m, "sdio_dat_rd_dly0_3: 0x%.8x\n", sdio_dat_rd_dly0_3);
	seq_printf(m, "sdio_dat_rd_dly1_0: 0x%.8x\n", sdio_dat_rd_dly1_0);
	seq_printf(m, "sdio_dat_rd_dly1_1: 0x%.8x\n", sdio_dat_rd_dly1_1);
	seq_printf(m, "sdio_dat_rd_dly1_2: 0x%.8x\n", sdio_dat_rd_dly1_2);
	seq_printf(m, "sdio_dat_rd_dly1_3: 0x%.8x\n", sdio_dat_rd_dly1_3);
	seq_printf(m, "sdio_clk_drv: 0x%.8x\n", sdio_clk_drv);
	seq_printf(m, "sdio_cmd_drv: 0x%.8x\n", sdio_cmd_drv);
	seq_printf(m, "sdio_data_drv: 0x%.8x\n", sdio_data_drv);
	seq_printf(m, "sdio_tune_flag: 0x%.8x\n", sdio_tune_flag);
	seq_printf(m, "=========================================\n\n");

	return 0;
}

static int msdc_tune_proc_write(struct file *file, const char *buf, unsigned long count, void *data)
{
	int ret;
	int cmd, p1, p2;

	if (count == 0)return -1;
	if(count > 255)count = 255;

	ret = copy_from_user(cmd_buf, buf, count);
	if (ret < 0)return -1;

	cmd_buf[count] = '\0';
	pr_err("msdc Write %s\n", cmd_buf);

	if(3  == sscanf(cmd_buf, "%x %x %x", &cmd, &p1, &p2))
	{
		switch(cmd)
		{
		case 0:
			if(p1 && p2)
				sdio_enable_tune = 1;
			else
				sdio_enable_tune = 0;
		break;
		case 1://Cmd and Data latch edge
				sdio_iocon_rspl = p1&0x1;
				sdio_iocon_dspl = p2&0x1;
		break;
		case 2://Cmd Pad/Async
				sdio_pad_tune_rrdly= (p1&0x1F);
				sdio_pad_tune_rdly= (p2&0x1F);
		break;
		case 3:
				sdio_dat_rd_dly0_0= (p1&0x1F);
				sdio_dat_rd_dly0_1= (p2&0x1F);
		break;
		case 4:
				sdio_dat_rd_dly0_2= (p1&0x1F);
				sdio_dat_rd_dly0_3= (p2&0x1F);
		break;
		case 5://Write data edge/delay
				sdio_iocon_w_dspl= p1&0x1;
				sdio_pad_tune_wrdly= (p2&0x1F);
		break;
		case 6:
				sdio_dat_rd_dly1_2= (p1&0x1F);
				sdio_dat_rd_dly1_3= (p2&0x1F);
		break;
		case 7:
				sdio_clk_drv= (p1&0x7);
		break;
		case 8:
				sdio_cmd_drv= (p1&0x7);
				sdio_data_drv= (p2&0x7);
		break;


		}
	}

	return count;
}

static int msdc_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, msdc_debug_proc_show, inode->i_private);
}

static const struct file_operations msdc_proc_debug_fops = {
	.open = msdc_proc_open,
	.write = msdc_debug_proc_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int msdc_help_proc_open(struct inode *inode, struct file *file, int test)
{
	return single_open(file, msdc_help_proc_show, inode->i_private);
}

static const struct file_operations msdc_proc_help_fops = {
	.open = msdc_help_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int msdc_tune_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, msdc_tune_proc_read_show, inode->i_private);
}

static const struct file_operations msdc_proc_tune_fops = {
	.open = msdc_tune_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
	.write = msdc_tune_proc_write,
};

static int msdc_tune_flag_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, msdc_tune_flag_proc_read_show, inode->i_private);
}
static const struct file_operations msdc_proc_tune_flag_fops = {
	.open = msdc_tune_flag_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

int msdc_debug_proc_init(void)
{
	struct proc_dir_entry *prEntry;
	/* struct proc_dir_entry *tune; */
	/* struct proc_dir_entry *tune_flag; */

	prEntry = proc_create("msdc_debug", 0660, NULL, &msdc_proc_debug_fops);
	if (prEntry) {
	   ;//prEntry->read_proc  = msdc_debug_proc_read;
	   //prEntry->write_proc = msdc_debug_proc_write;
	   //pr_err("[%s]: successfully create /proc/msdc_debug\n", __func__);
	} else {
	   pr_err("[%s]: failed to create /proc/msdc_debug\n", __func__);
	}

	prEntry = proc_create("msdc_help", 0660, NULL, &msdc_proc_help_fops);
	if (prEntry) {
	   ;//prEntry->read_proc  = msdc_help_proc_read;
	   //pr_err("[%s]: successfully create /proc/msdc_help\n", __func__);
	} else {
	   pr_err("[%s]: failed to create /proc/msdc_help\n", __func__);
	}

	memset(msdc_drv_mode,0,sizeof(msdc_drv_mode));
	prEntry = proc_create("msdc_tune", 0660, NULL, &msdc_proc_tune_fops);
	if (prEntry) {
	   ;//prEntry->read_proc  = msdc_tune_proc_read;
	   //prEntry->write_proc = msdc_tune_proc_read;
	   //pr_err("[%s]: successfully create /proc/msdc_tune\n", __func__);
	} else {
	   pr_err("[%s]: failed to create /proc/msdc_tune\n", __func__);
	}

	prEntry = proc_create("msdc_tune_flag", 0660, NULL, &msdc_proc_tune_flag_fops);
	if (prEntry) {
	   ;//prEntry->read_proc  = msdc_tune_flag_proc_read;
	   //pr_err("[%s]: successfully create /proc/msdc_tune_flag\n", __func__);
	} else {
	   pr_err("[%s]: failed to create /proc/msdc_tune_flag\n", __func__);
	}

	return 0 ;
}
EXPORT_SYMBOL_GPL(msdc_debug_proc_init);
