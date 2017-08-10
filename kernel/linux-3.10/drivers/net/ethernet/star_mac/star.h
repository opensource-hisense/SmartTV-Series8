/**
 *	@file star.h
 *	@brief The header file to define private data structures and necessary header files and definitions
 *	@author mtk02196
 *
 */
 
#ifndef _STAR_H_
#define _STAR_H_

//#include <linux/config.h>
#include <linux/crc32.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/etherdevice.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/ip.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/netdevice.h>
#include <linux/platform_device.h>
#include <linux/pci.h>
#include <linux/mii.h>
#include <linux/version.h>
//#include <asm/arch/ccif/ccif.h>

#include "star_dev.h"

#ifdef CONFIG_ARM64
#include "linux/soc/mediatek/mt53xx_linux.h"
#else
#include <mach/mt53xx_linux.h>
#include <mach/mtk_gpio.h>
#endif


//#define STAR_DBG_LVL_DEFAULT 2 
#define USE_EXT_CLK_SRC 0
#define USE_TX_TASKLET
#define USE_RX_NAPI

#define ETH_MAX_FRAME_SIZE          1536
#define ETH_MAX_LEN_PKT_COPY        250   /* max length when received data is copied to new skb */
#define ETH_EXTRA_PKT_LEN           36
#define ETH_HDR_LEN_FOR_FASTPATH	32

#define ETH_SKB_ALIGNMENT           16

#if defined(ETH_SUPPORT_WOL)
#define MAX_WOL_DETECT_BYTES        128
#define MAX_WOL_PATTERN_NUM         20
#define ETH_WOP_LENGTH              38

#define UDP_PROTOCOL                17
#define TCP_PROTOCOL                6
#define ETH_PROTOCOL_NUM            23
#define ETH_PROTOCOL_DEST_PORT1     36
#define ETH_PROTOCOL_DEST_PORT2     37
#endif

/**
 * @brief structure for Star private data
 */
typedef struct star_private_s
{
	StarDev             stardev; /* star device internal data */
	struct net_device   *dev;
	dma_addr_t          desc_dmaAddr;
    uintptr_t           desc_virAddr; /* allocate by dma_alloc_coherent */
	u32                 phy_addr; /* phy address(0~31) (for phy access and phy auto-polling) */
	spinlock_t          lock;
	struct tasklet_struct dsr;
	
#ifdef USE_RX_NAPI
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26)
	struct napi_struct  napi;
#endif	
#endif
	int                 tsk_tx; /* used for tx tasklet */
	int                 tsk_rx; /* used for rx tasklet */
    spinlock_t          tsk_lock; /* used for tx/rx tasklet lock */
	struct mii_if_info  mii;
} StarPrivate;

#endif /* _STAR_H_ */

