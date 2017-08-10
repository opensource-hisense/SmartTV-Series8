/**
 *	@file star.c
 *	@brief Star Ethernet Driver
 *	@author mtk02196
 *
 */

#include "star.h"

//#include <mach/mt53xx_linux.h>

#ifdef FASTPATH
#include <asm/arch/fastpath/fastpath.h>
#endif
#ifdef SUPPORT_MOD_LOG
#include <asm/arch/log/mod_log.h>
#endif

static spinlock_t star_lock;

#if defined (USE_RX_TASKLET) && defined (USE_RX_NAPI)
#warning "USE_RX_NAPI and USE_RX_TASKLET can not be used at the same time"
#undef USE_RX_TASKLET
#endif

#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/of_address.h>

#ifdef CONFIG_64BIT
#include <linux/soc/mediatek/mt53xx_linux.h>
#else
#include <mach/mt53xx_linux.h>
#endif

void star_if_queue_up_down(struct up_down_queue_par *par);

/* ============= Global variable ============= */
#ifdef STARMAC_SUPPORT_ETHTOOL
static struct ethtool_ops starmac_ethtool_ops;
#endif
static struct net_device *this_device = NULL;
struct device *mtk_dev;
static void __iomem *eth_base;
static void __iomem *pinmux_base;
#if defined(CONFIG_ARCH_MT5891)
static void __iomem *pdwnc_base;
#endif
//static u32 eth_pdwnc_base = 0;
//static u32 eth_chksum_base = 0;
static void __iomem *ipll_base1;
//static u32 ipll_base2 = 0;
//static u32 bsp_base = 0;
static StarDev *star_dev = NULL;
static struct sk_buff *skb_dbg; /* skb pointer used for debugging */
static int multicast_filter_limit = 32;
static u32 fgNetHWActive = 1;
static u32 fgStarOpen=FALSE;

int star_dbg_lvl = STAR_DBG_LVL_DEFAULT;
/* ============= Global Definition ============= */
//#define DEFAULT_MAC_ADDRESS         { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55 }
#define DEFAULT_MAC_ADDRESS         {0x00, 0x0C, 0xE7, 0x06, 0x00, 0x00}
//#define DEFAULT_MAC_ADDRESS         {0xff, 0xff, 0xff, 0xff, 0xff, 0xff}

#define IRQ_SWITCH			    6


#define STAR_DRV_NAME "star-eth"

#if defined(ETH_SUPPORT_WOL)
static u8 index = 0;
static u8 sramPattern[20][128];
static int star_setWOPProtocolPort(char protocol_type, int *port_array, int count);
#endif
#ifdef SUPPORT_ETH_POWERDOWN
int Ethernet_suspend(void);
int Ethernet_resume(void);
#endif//#ifdef SUPPORT_ETH_POWERDOWN

#ifdef USE_RX_NAPI
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26)
static int star_poll(struct napi_struct *napi, int budget)
#else
static int star_poll(struct net_device *dev, int *budget)
#endif
#else
static void star_receive(struct net_device *dev)
#endif
{
	StarPrivate *starPrv = NULL;
	StarDev *starDev = NULL;
	int retval;
	unsigned char *tail;
#ifdef USE_RX_NAPI	
	int npackets = 0;
    //u32 intrStatus = 0;
#if LINUX_VERSION_CODE == KERNEL_VERSION(2,6,10)
//#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,10)
	int quota = min(dev->quota, *budget);
#endif
	int done = 0;
#endif

#if (defined USE_RX_NAPI) && (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26))
	struct net_device *dev;
	starPrv = container_of(napi, StarPrivate, napi);
	dev = starPrv->dev;
#else
	starPrv = netdev_priv(dev);
#endif
	starDev = &starPrv->stardev;

#ifdef USE_RX_NAPI
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26)
	while(npackets < budget)
#else
	while (npackets < quota)
#endif
#else
	do
#endif
    {

        uintptr_t extBuf;
		u32 ctrlLen;
		u32 len;
		
		u32 dmaBuf;
		struct sk_buff *currSkb;
		struct sk_buff *newSkb;

		retval = StarDmaRxGet(starDev, (u32*)&dmaBuf, &ctrlLen, &extBuf);
		if (retval >= 0 && extBuf != 0)
		{
			currSkb = (struct sk_buff *)extBuf;
			dma_unmap_single(mtk_dev, dmaBuf, skb_tailroom(currSkb), DMA_FROM_DEVICE);

			if (StarDmaRxValid(ctrlLen))
			{
#ifdef CHECKSUM_OFFLOAD
				int cksumOk = 0;

				if (StarDmaRxProtocolIP(ctrlLen) && !StarDmaRxIPCksumErr(ctrlLen)) /* IP packet & IP checksum correct */
				{
					cksumOk = 1;
					if (StarDmaRxProtocolTCP(ctrlLen) || StarDmaRxProtocolUDP(ctrlLen)) /* TCP/UDP packet */
					{
						if (StarDmaRxL4CksumErr(ctrlLen)) /* L4 checksum error */
						{
							cksumOk = 0;
						}
					}
				}
#endif
				len = StarDmaRxLength(ctrlLen);

				if (len < ETH_MAX_LEN_PKT_COPY) /* Allocate new skb */
				{
					newSkb = currSkb;
#ifdef FASTPATH
					currSkb = dev_alloc_skb(len + 2 + ETH_HDR_LEN_FOR_FASTPATH);
					if (currSkb) skb_reserve(currSkb, 2 + ETH_HDR_LEN_FOR_FASTPATH);
#else
					currSkb = dev_alloc_skb(len + 2);
					if (currSkb) skb_reserve(currSkb, 2);
#endif
	
					if (!currSkb) /* No skb can be allocate */
					{
						currSkb = newSkb;
						newSkb = NULL;
					} else
					{
						memcpy(currSkb->data, newSkb->data, len);
					}
				} else
				{
#ifdef FASTPATH
					newSkb = dev_alloc_skb(ETH_MAX_FRAME_SIZE + ETH_HDR_LEN_FOR_FASTPATH);
#else
					newSkb = dev_alloc_skb(ETH_MAX_FRAME_SIZE);
#endif

					/* Shift to 16 byte alignment */
                    /*
					if ((u32)(newSkb->tail) & 0xf) 
					{
						u32 shift = ((u32)(newSkb->tail) & 0xf);
						skb_reserve(newSkb, shift);
					}
					*/
					if(newSkb)
                    {
                    	tail = skb_tail_pointer(newSkb);
                        if(((uintptr_t)tail) & (ETH_SKB_ALIGNMENT-1))
                        {
                            u32 offset =((uintptr_t)tail) & (ETH_SKB_ALIGNMENT-1);
                            skb_reserve(newSkb, ETH_SKB_ALIGNMENT - offset);
                        }
                    }
                    else
                    {
                        STAR_MSG(STAR_ERR,"star_receive mem alloc fail(1), packet dropped\n");
                    }

					/* for zero copy */
#ifdef FASTPATH
					if (newSkb) skb_reserve(newSkb, 2 + ETH_HDR_LEN_FOR_FASTPATH);
#else
					if (newSkb) skb_reserve(newSkb, 2);
#endif
				}

				if (!newSkb) /* No skb can be allocated -> packet drop */
				{
					if (printk_ratelimit())
					{
						STAR_MSG(STAR_ERR, "star_receive mem alloc fail, packet dropped\n");
					}
					starDev->stats.rx_dropped ++;
					newSkb = currSkb;
				} else
				{
					skb_put(currSkb, len);
#ifdef CHECKSUM_OFFLOAD
					if (cksumOk) {currSkb->ip_summed = CHECKSUM_UNNECESSARY;} else {currSkb->ip_summed = CHECKSUM_NONE;}
#else
					currSkb->ip_summed = CHECKSUM_NONE;
#endif
					currSkb->dev = dev;
					currSkb->protocol = eth_type_trans(currSkb, dev);

#ifdef FASTPATH
					skb_push(currSkb, ETH_HLEN);
					if (!fastpath_in(FASTPATH_ID, currSkb))
					{
						skb_pull(currSkb, ETH_HLEN);
#endif

#ifdef USE_RX_NAPI					
						netif_receive_skb(currSkb);     /* send the packet up protocol stack */
#else
						netif_rx(currSkb);
#endif

#ifdef FASTPATH
					}
#endif
					dev->last_rx = jiffies;   		/* set the time of the last receive */
					starDev->stats.rx_packets ++;
					starDev->stats.rx_bytes += len;
#ifdef USE_RX_NAPI
					npackets ++;
#endif
				}
			} else
			{   /* Error packet */
				newSkb = currSkb;
				starDev->stats.rx_errors ++;
				starDev->stats.rx_crc_errors += StarDmaRxCrcErr(ctrlLen);
			}

			dmaBuf = dma_map_single(mtk_dev, 
                                    skb_tail_pointer(newSkb) - 2/*Because Star Ethernet buffer must 16 byte align*/, 
								    skb_tailroom(newSkb), 
                                    DMA_FROM_DEVICE);
			StarDmaRxSet(starDev, dmaBuf, skb_tailroom(newSkb), (uintptr_t)newSkb);
#ifdef USE_RX_NAPI
		}else
		{
			done = 1;
			break;
#endif
		}
    }
#ifndef USE_RX_NAPI
	while (retval >= 0);
#endif

	StarDmaRxResume(starDev);

#ifdef USE_RX_NAPI
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26)
	if (done)
	{
		local_irq_disable();
		napi_complete(napi);
		StarIntrRxEnable(starDev);   /* Enable rx interrupt */        
		local_irq_enable();
	}
	return(npackets);    
#else /* LINUX_VERSION_CODE < KERNEL_VERSION(2,6,26) */
	*budget -= npackets;
	dev->quota -= npackets;
	if (done) /* All packets have been processed */
	{
		local_irq_disable();
		__netif_rx_complete(dev);
		StarIntrRxEnable(starDev);   /* Enable rx interrupt */        
		local_irq_enable();                
        return 0;
	}
	/* there are another packets need process */
	return 1;
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26) */
#endif /* USE_RX_NAPI */
}

static void star_finish_xmit(struct net_device *dev)
{
	StarPrivate *starPrv = NULL;
	StarDev *starDev = NULL;
	int retval;
	int wake = 0;

	STAR_MSG(STAR_VERB, "star_finish_xmit(%s)\n", dev->name);

	starPrv = netdev_priv(dev);
	starDev = &starPrv->stardev;

	do {
		uintptr_t extBuf;
		u32 ctrlLen;
		u32 len;
		u32 dmaBuf;
		unsigned long flags;
		
		spin_lock_irqsave(&starPrv->lock, flags);
		retval = StarDmaTxGet(starDev, (u32*)&dmaBuf, &ctrlLen, &extBuf);
		spin_unlock_irqrestore(&starPrv->lock, flags);

		if (retval >= 0 && extBuf != 0)
		{
			len = StarDmaTxLength(ctrlLen);
			dma_unmap_single(mtk_dev, dmaBuf, len, DMA_TO_DEVICE);
			STAR_MSG(STAR_VERB, 
                     "star_finish_xmit(%s) - get tx descriptor %d for skb 0x%lx, length = %08x\n", 
                     dev->name, retval, extBuf, len);

			/* ??????????? reuse skb */

			dev_kfree_skb_irq((struct sk_buff *)extBuf);

			/* Tx statistics, use MIB? */
			starDev->stats.tx_bytes += len;
			starDev->stats.tx_packets ++;
			
			wake = 1;
		}
	} while (retval >= 0);

	if (wake)
	{
		netif_wake_queue(dev);
	}
}

#if defined (USE_TX_TASKLET) || defined (USE_RX_TASKLET)
static void star_dsr(unsigned long data)
{
	struct net_device *dev = (struct net_device *)data;
	StarPrivate *starPrv = NULL;
	StarDev *starDev = NULL;

	STAR_MSG(STAR_VERB, "star_dsr(%s)\n", dev->name);

	starPrv = netdev_priv(dev);
	starDev = &starPrv->stardev;

#ifdef USE_TX_TASKLET
	if (starPrv->tsk_tx)
	{
		starPrv->tsk_tx = 0;
		star_finish_xmit(dev);
	}
#endif
#ifdef USE_RX_TASKLET
	if (starPrv->tsk_rx)
	{
		starPrv->tsk_rx = 0;
		star_receive(dev);
	}
#endif
}
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26)
static irqreturn_t star_isr(int irq, void *dev_id)
#else
static irqreturn_t star_isr(int irq, void *dev_id, struct pt_regs *regs)
#endif
{
    struct net_device *dev = (struct net_device *)dev_id;
    StarPrivate *starPrv = NULL;
    StarDev *starDev = NULL;
    u32 intrStatus;
    u32 intrClrMsk=0xffffffff;
#if defined(ETH_SUPPORT_WOL)
    int wopPatternDetect = 0;
#endif
#ifdef CC_BGM_SUPPORT_WOL
	intrClrMsk&=~STAR_INT_STA_MAGICPKT;
#endif
#ifdef USE_RX_NAPI

#if defined(ETH_SUPPORT_WOL)
    intrClrMsk &= ~(STAR_INT_STA_RXC | STAR_INT_STA_WOP);
#else
    intrClrMsk &= ~STAR_INT_STA_RXC;
#endif

#endif
    if (!dev)
    {
        STAR_MSG(STAR_ERR, "star_isr - unknown device\n");
        return IRQ_NONE;
    }

	STAR_MSG(STAR_VERB, "star_isr(%s)\n", dev->name);

	starPrv = netdev_priv(dev);
	starDev = &starPrv->stardev;

	intrStatus = StarIntrStatus(starDev);
	StarIntrClear(starDev,intrStatus & intrClrMsk);

	do 
    {
		STAR_MSG(STAR_VERB, "star_isr - interrupt status = 0x%08x\n", intrStatus);

		if (intrStatus & STAR_INT_STA_RXC) /* Rx Complete */
		{
			STAR_MSG(STAR_VERB, "rx complete\n");
#ifdef USE_RX_NAPI
	#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26)
			if (napi_schedule_prep(&starPrv->napi))
			{
				StarIntrRxDisable(starDev); /* Disable rx interrupts */
				StarIntrClear(starDev, STAR_INT_STA_RXC); /* Clear rx interrupt */
				__napi_schedule(&starPrv->napi);
			}
	#else
			if (netif_rx_schedule_prep(dev))
			{
				StarIntrRxDisable(starDev); /* Disable rx interrupts */
				StarIntrClear(starDev, STAR_INT_STA_RXC); /* Clear rx interrupt */
				__netif_rx_schedule(dev);
			}
	#endif
#else
	#ifdef USE_RX_TASKLET
			starPrv->tsk_rx = 1;
	#else
			star_receive(dev);
	#endif
#endif
		}
		
		if (intrStatus & STAR_INT_STA_RXQF) /* Rx Queue Full */
		{
			STAR_MSG(STAR_VERB, "rx queue full\n");
		}

		if (intrStatus & STAR_INT_STA_RXFIFOFULL) /* Rx FIFO Full */
		{
			STAR_MSG(STAR_WARN, "rx fifo full\n");
		}
		
		if (intrStatus & STAR_INT_STA_TXC) /* Tx Complete */
		{
			STAR_MSG(STAR_VERB, " tx complete\n");
#ifdef USE_TX_TASKLET
			starPrv->tsk_tx = 1;
#else
			star_finish_xmit(dev);
#endif
		}

		if (intrStatus & STAR_INT_STA_TXQE) /* Tx Queue Empty */
		{
			STAR_MSG(STAR_VERB, "tx queue empty\n");
		}

     #ifdef CC_ETHERNET_6896
		if (intrStatus & STAR_INT_STA_RX_PCODE) 
		{
		 	STAR_MSG(STAR_DBG, "Rx PCODE\n");
		}
     #else
		if (intrStatus & STAR_INT_STA_TXFIFOUDRUN) /* Tx FIFO underrun */
		{
			STAR_MSG(STAR_ERR, "tx fifo underrun\n");
		}
    #endif

		if (intrStatus & STAR_INT_STA_MAGICPKT) /* Receive magic packet */
		{
			STAR_MSG(STAR_WARN, "magic packet received\n");
			#ifdef CC_BGM_SUPPORT_WOL
			printk("magic packet received\n");
			StarIntrMagicPktDisable(starDev);
			#endif
		}

#if defined(ETH_SUPPORT_WOL)
		if (intrStatus & STAR_INT_STA_WOP) /* Receive wop packet */
		{
			wopPatternDetect = StarGetReg(STAR_WOL_DETECT_STATUS(eth_base));
			STAR_MSG(STAR_WARN, "Wakeup on packet interrupt:pattent=0x%x\n", wopPatternDetect);
			StarSetBit(STAR_INT_STA(eth_base), STAR_INT_STA_WOP); //clear wok interrupt
			StarClearBit(STAR_WOL_MIRXBUF_CTL(eth_base), WOL_MRXBUF_EN);
			StarSetBit(STAR_WOL_MIRXBUF_CTL(eth_base), WOL_MRXBUF_RESET); //clear wok interrupt
			StarClearBit(STAR_STBY_CTRL(eth_base), 1);
			StarClearBit(STAR_WOL_MIRXBUF_CTL(eth_base), WOL_MRXBUF_RESET);
		}
#endif
		if (intrStatus & STAR_INT_STA_MIBCNTHALF) /* MIB counter reach 2G (0x80000000) */
		{
			STAR_MSG(STAR_VERB, " mib counter reach 2G\n");
			StarMibInit(starDev);
		}

		if (intrStatus & STAR_INT_STA_PORTCHANGE) /* Port status change */
		{
			STAR_MSG(STAR_DBG, "port status change\n");
            StarLinkStatusChange(starDev);
		}

		intrStatus = StarIntrStatus(starDev);  /* read interrupt requests came during interrupt handling */
		StarIntrClear(starDev,intrStatus & intrClrMsk);

    }
while ((intrStatus  & intrClrMsk)!= 0);

	STAR_MSG(STAR_VERB, "star_isr return\n");
	
#if defined (USE_TX_TASKLET) && defined (USE_RX_TASKLET)
	if (starPrv->tsk_tx || starPrv->tsk_rx)
#elif defined (USE_TX_TASKLET)
	if (starPrv->tsk_tx)
#elif defined (USE_RX_TASKLET)
	if (starPrv->tsk_rx)
#endif
    {
#if defined (USE_TX_TASKLET) || defined (USE_RX_TASKLET)
		tasklet_schedule(&starPrv->dsr);
#endif	
    }
	return IRQ_HANDLED;
}

#ifdef CONFIG_NET_POLL_CONTROLLER
static void star_netpoll(struct net_device *dev)
{
	StarPrivate *tp   = netdev_priv(dev);
	StarDev     *pdev = tp->mii.dev;

	disable_irq(pdev->irq);
	star_isr(pdev->irq, dev);
	enable_irq(pdev->irq);
}
#endif

static int star_open(struct net_device *dev)
{
	StarPrivate *starPrv = NULL;
	StarDev *starDev = NULL;
	int retval;
	u32 intrStatus;
	unsigned char *tail;
	STAR_MSG(STAR_DBG, "star_open(%s)\n", dev->name);

	starPrv = netdev_priv(dev);
	starDev = &starPrv->stardev;
	
    /* Start RX FIFO receive */
    StarNICPdSet(starDev, 0);
    
	StarIntrDisable(starDev);
	StarDmaTxStop(starDev);
	StarDmaRxStop(starDev);
	intrStatus = StarIntrStatus(starDev);
	StarIntrClear(starDev, intrStatus);

    /* init carrier to off */
    netif_carrier_off(dev);
    
	/* Register NIC interrupt */
	STAR_MSG(STAR_DBG, "request interrupt vector=%d\n", dev->irq);
	if (request_irq(dev->irq, star_isr, 0, dev->name, dev) != 0)	/* request non-shared interrupt */
	{
		STAR_MSG(STAR_ERR, "interrupt %d request fail\n", dev->irq);
		return -ENODEV;
	}

	/* ============== Initialization due to Pin Trap in ASIC mode ==============  */
	/* ===== There are 3 modes: Internal switch/internal phy/RvMII(IC+175C) ===== */
	if (1) //(StarIsASICMode(starDev))
	{
		/* MII Pad output enable */
		StarSetBit(STAR_ETHPHY(starDev->base), STAR_ETHPHY_MIIPAD_OE);
		/* Force SMI Enable */
		StarSetBit(STAR_ETHPHY(starDev->base), STAR_ETHPHY_FRC_SMI_EN);
		/* Disable extended retry - for 10-half mode runs normally */
		StarClearBit(STAR_TEST1(starDev->base), STAR_TEST1_EXTEND_RETRY);
		
		switch (StarPhyMode(starDev))
		{
			case INT_PHY_MODE:
			{
				STAR_MSG(STAR_DBG, "Internal PHY mode\n");
				break;
			}
			case INT_SWITCH_MODE:
			{
				break;
			}
			case EXT_MII_MODE:
			{
				// TODO: External MII device initialization
				break;
			}
			case EXT_RGMII_MODE:
			{
				// TODO: Giga-Ethernet initialization
				break;
			}
			case EXT_RVMII_MODE:
			{
				u32 phyIdentifier;

				phyIdentifier = StarMdcMdioRead(starDev, 0, 3);
				if (phyIdentifier == 0x0d80)
				{	/* IC+ 175C */
					u16 ctrlRegValue = StarMdcMdioRead(starDev, 31, 5);
					ctrlRegValue |= 0x8000; /* Enable P4EXT */
					ctrlRegValue |= 0x0800; /* Enable MII0 mac mode */
					ctrlRegValue &= 0xfbff; /* Disable MII0 RMII mode */
					StarMdcMdioWrite(starDev, 31, 5, ctrlRegValue);
				}
				break;
			}
			default:
			{
				STAR_MSG(STAR_ERR, "star_open unknown Eth mode!\n\r");
				return -1;
			}
		}
	}

	STAR_MSG(STAR_VERB, "MAC Initialization\n");
	if (StarMacInit(starDev, dev->dev_addr) != 0)     /* MAC Initialization */
	{
		STAR_MSG(STAR_ERR, "MAC init fail\n");
		return -ENODEV;		
	}
#if 0
    STAR_MSG(STAR_VERB, "StarDmaInit virAddr=0x%lx, dmaAddr=0x%llx\n", 
                starPrv->desc_virAddr, starPrv->desc_dmaAddr);
#endif
	if (StarDmaInit(starDev, starPrv->desc_virAddr, starPrv->desc_dmaAddr) != 0)    /* DMA Initialization */
	{
		STAR_MSG(STAR_ERR, "DMA init fail\n");
		return -ENODEV;
	}

	STAR_MSG(STAR_VERB, "PHY Control Initialization\n");
	if (StarPhyCtrlInit(starDev, 1/*Enable PHY auto-polling*/, starPrv->phy_addr) != 0)
	{
		STAR_MSG(STAR_ERR, "PHY Control init fail\n");
		return -ENODEV;
	}

	do { /* pre-allocate Rx buffer */
		u32 dmaBuf;
#ifdef FASTPATH
		struct sk_buff *skb = dev_alloc_skb(dev->mtu + ETH_EXTRA_PKT_LEN + ETH_HDR_LEN_FOR_FASTPATH);
#else
		struct sk_buff *skb = dev_alloc_skb(dev->mtu + ETH_EXTRA_PKT_LEN);
#endif

    		if (skb == NULL)
    		{
    			STAR_MSG(STAR_ERR, "Error! No momory for rx sk_buff!\n");
    			/* TODO */
    			return -ENOMEM;
    		}

		/* Shift to 16 byte alignment */	
		tail = skb_tail_pointer(skb);
        if(((uintptr_t)tail) & (ETH_SKB_ALIGNMENT-1))
        {
            u32 offset = ((uintptr_t)tail) & (ETH_SKB_ALIGNMENT-1);
            skb_reserve(skb, ETH_SKB_ALIGNMENT - offset);
        }


		/* Reserve 2 bytes for zero copy */
#ifdef FASTPATH		
		if (skb) {skb_reserve(skb, 2 + ETH_HDR_LEN_FOR_FASTPATH);}
#else
        /* Reserving 2 bytes makes the skb->data points to
           a 16-byte aligned address after eth_type_trans is called.
           Since eth_type_trans will extracts the pointer (ETH_LEN)
           14 bytes. With this 2 bytes reserved, the skb->data
           can be 16-byte aligned before passing to upper layer. */
		if (skb) {skb_reserve(skb, 2);}
#endif
		
        /* Note:
            We pass to dma addr with skb->tail-2 (4N aligned). But
            the RX_OFFSET_2B_DIS has to be set to 0, making DMA to write
            tail (4N+2) addr. 
         */
		dmaBuf = dma_map_single(mtk_dev, 
                                skb_tail_pointer(skb) - 2/*Because Star Ethernet buffer must 16 byte align*/, 
                                skb_tailroom(skb), 
                                DMA_FROM_DEVICE);
		if (dma_mapping_error(mtk_dev, dmaBuf)) {
			STAR_MSG(STAR_ERR, "dma_mapping_error error\n");
			return -ENOMEM;
		}


		retval = StarDmaRxSet(starDev, dmaBuf, skb_tailroom(skb), (uintptr_t)skb);

		STAR_MSG(STAR_VERB, "rx descriptor idx:%d for skb %p\n", retval, skb);
		
		if (retval < 0)
		{
			dma_unmap_single(mtk_dev, dmaBuf, skb_tailroom(skb), DMA_FROM_DEVICE);
			dev_kfree_skb(skb);
		}
	} while (retval >= 0);

#ifdef USE_RX_NAPI
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26)
	napi_enable(&starPrv->napi);
#endif
#endif

	intrStatus = StarIntrStatus(starDev);
	StarIntrClear(starDev, intrStatus);
	StarIntrEnable(starDev);
	#ifdef CC_BGM_SUPPORT_WOL	
	StarIntrMagicPktDisable(starDev);//disable magic packet intr firstly
	#endif
	StarDmaTxEnable(starDev);
	StarDmaRxEnable(starDev);


    /* PHY reset */
    StarPhyReset(starDev);
    /* wait for a while until PHY ready */
    msleep(10);

    /* Because of MT5882 ES1 100M link NG, force MT5882 ES1 Ethernet 10M link. */
    #ifdef CONFIG_ARCH_MT5882
	if (IS_IC_MT5882_ES1())
	{
        StarMdcMdioWrite(starDev, starPrv->phy_addr, 0x4, 0x61);
        STAR_MSG(STAR_DBG, "Force 10M speed!\n");
	}
    #endif

    {
        /* NOTE by sarick---------------------
           This is a workaround for the problem that, when powering on 
           platform and the Ethernet is connected with a 10Mbps Hub, the
           platform could fail to get IP.
           We found if disabling and then enabling the Auto-Negotiation,
           the problem does not appear.
         */
        u32 val = 0;
        
        STAR_MSG(STAR_VERB, "Re-enabling AN\n");    
        val = StarGetReg(STAR_PHY_CTRL1(starDev->base));
        val &= ~(STAR_PHY_CTRL1_ANEN);
        StarSetReg(STAR_PHY_CTRL1(starDev->base), val);
        val |= STAR_PHY_CTRL1_ANEN;
        StarSetReg(STAR_PHY_CTRL1(starDev->base), val);
    }
    
    StarLinkStatusChange(starDev);
	netif_start_queue(dev);
	fgNetHWActive = 1;
	fgStarOpen = TRUE ;
    STAR_MSG(STAR_VERB, "star_open done\n");	
	return 0;
}

#if defined(CONFIG_ARCH_MT5891)
static int __star_resume(struct net_device *dev)
{
	StarPrivate *starPrv = NULL;
	StarDev *starDev = NULL;
	u32 intrStatus;

    STAR_MSG(STAR_DBG, "__star_resume enter\n");	
	starPrv = netdev_priv(dev);
	starDev = &starPrv->stardev;
	
    StarNICPdSet(starDev, 0);
    
	StarIntrDisable(starDev);
	intrStatus = StarIntrStatus(starDev);
	StarIntrClear(starDev, intrStatus);

    /* init carrier to off */
    netif_carrier_off(dev);
    
	/* Register NIC interrupt */
	STAR_MSG(STAR_VERB, "request interrupt vector=%d\n", dev->irq);
	if (request_irq(dev->irq, star_isr, 0, dev->name, dev) != 0)	/* request non-shared interrupt */
	{
		STAR_MSG(STAR_ERR, "interrupt %d request fail\n", dev->irq);
		return -ENODEV;
	}

	STAR_MSG(STAR_VERB, "MAC Initialization\n");
	if (StarMacInit(starDev, dev->dev_addr) != 0)     /* MAC Initialization */
	{
		STAR_MSG(STAR_ERR, "MAC init fail\n");
		return -ENODEV;		
	}

#ifdef USE_RX_NAPI
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26)
	napi_enable(&starPrv->napi);
#endif
#endif

	intrStatus = StarIntrStatus(starDev);
	StarIntrClear(starDev, intrStatus);
	StarIntrEnable(starDev);
	#ifdef CC_BGM_SUPPORT_WOL	
	StarIntrMagicPktDisable(starDev);//disable magic packet intr firstly
	#endif

	netif_start_queue(dev);
	fgNetHWActive = 1;

	return 0;
}
#endif

static int star_stop(struct net_device *dev)
{
	StarPrivate *starPrv = NULL;
	StarDev *starDev = NULL;
	int retval;
	u32 intrStatus;

	STAR_MSG(STAR_DBG, "star_stop(%s)\n", dev->name);

    if(fgStarOpen == FALSE) return -1 ;
	starPrv = netdev_priv(dev);
	starDev = &starPrv->stardev;

	netif_stop_queue(dev);

#ifdef USE_RX_NAPI
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26)
	napi_disable(&starPrv->napi);
#endif
#endif

	StarIntrDisable(starDev);
	StarDmaTxStop(starDev);
	StarDmaRxStop(starDev);
	intrStatus = StarIntrStatus(starDev);
	StarIntrClear(starDev, intrStatus);

	free_irq(dev->irq, dev);      /* free nic irq */

	do { /* Free Tx descriptor */
        uintptr_t extBuf;     
		u32 ctrlLen;
		u32 len;
		u32 dmaBuf;
	
		retval = StarDmaTxGet(starDev, (u32*)&dmaBuf, &ctrlLen, &extBuf);
		if (retval >= 0 && extBuf != 0)
		{
			len = StarDmaTxLength(ctrlLen);
			dma_unmap_single(mtk_dev, dmaBuf, len, DMA_TO_DEVICE);
			STAR_MSG(STAR_DBG, "star_stop - get tx descriptor idx:%d for skb 0x%lx\n", retval, extBuf);

			dev_kfree_skb((struct sk_buff *)extBuf);
		}
	} while (retval >= 0);

	do { /* Free Rx descriptor */
		
		uintptr_t extBuf;  
		u32 dmaBuf;

		retval = StarDmaRxGet(starDev, (u32 *)&dmaBuf, NULL, &extBuf);
		if (retval >= 0 && extBuf != 0)
		{
			dma_unmap_single(mtk_dev, dmaBuf, skb_tailroom((struct sk_buff *)extBuf), DMA_FROM_DEVICE);
						STAR_MSG(STAR_DBG, "star_stop - get tx descriptor idx:%d for skb 0x%lx\n", retval, extBuf);
			dev_kfree_skb((struct sk_buff *)extBuf);
		}
	} while (retval >= 0);
    
    /* Stop RX FIFO receive */
    StarNICPdSet(starDev, 1);

    fgStarOpen = FALSE ;
	return 0;
}

#if defined(CONFIG_ARCH_MT5891)
static int __star_suspend(struct net_device *dev)
{
	StarPrivate *starPrv = NULL;
	StarDev *starDev = NULL;
	u32 intrStatus;

    STAR_MSG(STAR_DBG, "__star_suspend enter\n");	
    if(fgStarOpen == FALSE) return -1;
	starPrv = netdev_priv(dev);
	starDev = &starPrv->stardev;

	netif_stop_queue(dev);

#ifdef USE_RX_NAPI
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26)
	napi_disable(&starPrv->napi);
#endif
#endif

	StarIntrDisable(starDev);
	intrStatus = StarIntrStatus(starDev);
	StarIntrClear(starDev, intrStatus);

	free_irq(dev->irq, dev);      /* free nic irq */
   	return 0;
}
#endif

static int star_start_xmit(struct sk_buff *skb, struct net_device *dev)
{
	StarPrivate *starPrv = NULL;
	StarDev *starDev = NULL;
	u32 dmaBuf;
	unsigned long flags;
	int retval;

	starPrv = netdev_priv(dev);
	starDev = &starPrv->stardev;

	/* If frame size > Max frame size, drop this packet */
	if (skb->len > ETH_MAX_FRAME_SIZE)
	{
		STAR_MSG(STAR_WARN, "start_xmit(%s) - Tx frame length is oversized: %d bytes\n", dev->name, skb->len);
		dev_kfree_skb(skb);
		starDev->stats.tx_dropped ++;
		return 0;
	}

#ifdef FASTPATH
	fastpath_out(FASTPATH_ID, skb);
#endif

	//STAR_MSG(STAR_VERB, "hard_start_xmit\n");

	/* ?????? force to send or return NETDEV_TX_BUSY */

	dmaBuf = dma_map_single(mtk_dev, skb->data, skb->len, DMA_TO_DEVICE);
	
	spin_lock_irqsave(&starPrv->lock, flags);
	

	retval = StarDmaTxSet(starDev, dmaBuf, skb->len, (uintptr_t)skb);


	if (starDev->txNum == starDev->txRingSize) /* Tx descriptor ring full */
	{
	    //STAR_MSG(STAR_WARN, "Tx descriptor full\n");
		netif_stop_queue(dev);
	}

	spin_unlock_irqrestore(&starPrv->lock, flags);

	StarDmaTxResume(starDev);
	dev->trans_start = jiffies;

	return 0;
}

static struct net_device_stats *star_get_stats(struct net_device *dev)
{
	StarPrivate *starPrv = NULL;
	StarDev *starDev = NULL;
  
	STAR_MSG(STAR_VERB, "get_stats\n");

	starPrv = netdev_priv(dev);
	starDev = &starPrv->stardev;

	return &starDev->stats;
}

static void star_set_multicast_list(struct net_device *dev)
{
#define STAR_HTABLE_SIZE		(512)
	unsigned long flags;

	StarPrivate *starPrv = NULL;
	StarDev *starDev = NULL;
  
	STAR_MSG(STAR_VERB, "star_set_multicast_list\n");

	starPrv = netdev_priv(dev);
	starDev = &starPrv->stardev;

	spin_lock_irqsave(&star_lock, flags);

	if (dev->flags & IFF_PROMISC)
	{
		STAR_MSG(STAR_WARN, "%s: Promiscuous mode enabled.\n", dev->name);
		StarArlPromiscEnable(starDev);
	} 
    else if ((netdev_mc_count(dev) > multicast_filter_limit) || (dev->flags & IFF_ALLMULTI))
	{
		u32 hashIdx;
		for (hashIdx = 0; hashIdx < STAR_HTABLE_SIZE; hashIdx ++)
		{
			StarSetHashBit(starDev, hashIdx, 1);
		}
	} 
    else
	{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,34)
		struct netdev_hw_addr *ha;
		netdev_for_each_mc_addr(ha, dev) {
			u32 hashAddr;
			hashAddr = (u32)(((ha->addr[0] & 0x1) << 8) + (u32)(ha->addr[5]));
			StarSetHashBit(starDev, hashAddr, 1);
		}
#else // < linux kernel 2.6.34
		u32 mcIdx;
		struct dev_mc_list *mclist;
		for (mcIdx = 0, mclist = dev->mc_list; mclist && mcIdx < netdev_mc_count(dev); mcIdx++, mclist = mclist->next)
		{
			u32 hashAddr;
			hashAddr = (u32)(((mclist->dmi_addr[0] & 0x1) << 8) + (u32)(mclist->dmi_addr[5]));
			StarSetHashBit(starDev, hashAddr, 1);
		}
#endif
	}

	spin_unlock_irqrestore(&star_lock, flags);
}

static int star_ioctl(struct net_device *dev, struct ifreq *req, int cmd)
{
	StarPrivate *starPrv = NULL;
	StarDev *starDev = NULL;
	unsigned long flags;
	int rc=0;
    struct ioctl_eth_mac_cmd *mac_cmd= NULL;
	starPrv = netdev_priv(dev);
	starDev = &starPrv->stardev;

	if (!netif_running(dev))
		return -EINVAL;

	if (StarIsASICMode(starDev))
	{
		if (StarPhyMode(starDev) != INT_SWITCH_MODE)
		{
			if (starPrv->mii.phy_id == 32)
				return -EINVAL;
		}
	} 
    else
	{
		if (starPrv->mii.phy_id == 32)
			return -EINVAL;
	}

    switch (cmd)
    {
    	
    	case SIOC_WRITE_MAC_ADDR_CMD:
    	break;
    	
    	case SIOC_THROUGHPUT_GET_CMD:
    	
    	break;
    	
    	case SIOC_ETH_MAC_CMD:
    	 mac_cmd = (struct ioctl_eth_mac_cmd *)&req->ifr_data;
    	
         if(mac_cmd->eth_cmd == ETH_MAC_REG_READ)
         {
         	
         }
         else if(mac_cmd->eth_cmd == ETH_MAC_REG_READ)	
         {
         	
         }	
    	 else if(mac_cmd->eth_cmd == ETH_MAC_REG_WRITE)	
    	 {
    	 }	
    	 else if(mac_cmd->eth_cmd == ETH_MAC_TX_DESCRIPTOR_READ)	
    	 {
    	 	
    	 }	
    	 else if(mac_cmd->eth_cmd == ETH_MAC_RX_DESCRIPTOR_READ)	
    	 {
    	 	
    	 }	
    	 else if(mac_cmd->eth_cmd == ETH_MAC_UP_DOWN_QUEUE)	
    	 {
    	   star_if_queue_up_down(&mac_cmd->ifr_ifru.up_down_queue);	
    	 	
    	 }	
       	
        rc = 0; 
    	break;
    	
    	
        case SIOC_MDIO_CMD: /* for linux ethernet diag command */
        {
            struct ioctl_mdio_cmd *mdio_cmd = (struct ioctl_mdio_cmd *)&req->ifr_data;
            if (mdio_cmd->wr_cmd)
            {
                StarMdcMdioWrite(starDev, starPrv->mii.phy_id, mdio_cmd->reg, mdio_cmd->val);
            }
            else
            {
                mdio_cmd->val = StarMdcMdioRead(starDev, starPrv->mii.phy_id, mdio_cmd->reg);
            }
        }
            rc = 0;
            break;

		case SIOC_PHY_CTRL_CMD: /* for linux ethernet diag command */
        {
            struct ioctl_phy_ctrl_cmd *pc_cmd = (struct ioctl_phy_ctrl_cmd *)&req->ifr_data;
            StarDisablePortCHGInt(starDev);
            StarPhyDisableAtPoll(starDev);
			if (pc_cmd->wr_cmd)
            {
                switch(pc_cmd->Prm)
                {
                 case ETH_PHY_DACAMP_CTRL:  //100M Amp
				 	#ifdef CC_ETHERNET_6896
					STAR_MSG(STAR_ERR, "vSetDACAmp(%d) \n",pc_cmd->val);
				 	vSetDACAmp(starDev,pc_cmd->val);
				 	#endif
				 	break;
					
				 case ETH_PHY_10MAMP_CTRL:
				 	STAR_MSG(STAR_ERR, "vSet10MAmp(%d) \n",pc_cmd->val);
				 	vStarSet10MAmp(starDev,pc_cmd->val);
				 	break;

				 case ETH_PHY_IN_BIAS_CTRL:
				 	STAR_MSG(STAR_ERR, "vSetInputBias(%d) \n",pc_cmd->val);		
				   	vStarSetInputBias(starDev,pc_cmd->val);	 
					break;
				   
				 case ETH_PHY_OUT_BIAS_CTRL:
				 	STAR_MSG(STAR_ERR, "vStarSetOutputBias(%d) \n",pc_cmd->val);
					 vStarSetOutputBias(starDev,pc_cmd->val); 	 
				   break;

				 case ETH_PHY_FEDBAK_CAP_CTRL:
				 	STAR_MSG(STAR_ERR, "vSetFeedbackCap(%d) \n",pc_cmd->val);	 
					 vStarSetFeedBackCap(starDev,pc_cmd->val);
				 	break;

				 case ETH_PHY_SLEW_RATE_CTRL:
				 	STAR_MSG(STAR_ERR, "vSetSlewRate(%d) \n",pc_cmd->val);
					 vStarSetSlewRate(starDev,pc_cmd->val);
				   break;
				   
				 case ETH_PHY_EYE_OPEN_CTRL:
				 	#ifdef CC_ETHERNET_6896
					 STAR_MSG(STAR_ERR, "MT8560 do not have this setting \n");
					#endif 
				   break;

				 case ETH_PHY_BW_50P_CTRL:
				 	STAR_MSG(STAR_ERR, "vSet50percentBW(%d) \n",pc_cmd->val);
					 vStarSet50PercentBW(starDev,pc_cmd->val); 	 
				  break;
				  
                 default:
                  STAR_MSG(STAR_ERR, "set nothing \n");
					break;
					
                }

            }
            else
            {
				switch(pc_cmd->Prm)
				 {
				  case ETH_PHY_DACAMP_CTRL:  //100M Amp
					 #ifdef CC_ETHERNET_6896
					 STAR_MSG(STAR_ERR, "vGetDACAmp() \n");
					 vGetDACAmp(starDev,&(pc_cmd->val));
					 #endif
					 break;
					 
				  case ETH_PHY_10MAMP_CTRL:
					 STAR_MSG(STAR_ERR, "vStarGet10MAmp() \n");
					 vStarGet10MAmp(starDev,&(pc_cmd->val));
					 break;
				
				  case ETH_PHY_IN_BIAS_CTRL:
					 STAR_MSG(STAR_ERR, "vStarGetInputBias() \n");	 
					 vStarGetInputBias(starDev,&(pc_cmd->val));  
					 break;
					
				  case ETH_PHY_OUT_BIAS_CTRL:
					 STAR_MSG(STAR_ERR, "vStarGetOutputBias() \n");
					  vStarGetOutputBias(starDev,&(pc_cmd->val));	  
					break;
				
				  case ETH_PHY_FEDBAK_CAP_CTRL:
					 STAR_MSG(STAR_ERR, "vStarGetFeedBackCap() \n");	  
					  vStarGetFeedBackCap(starDev,&(pc_cmd->val));
					 break;
				
				  case ETH_PHY_SLEW_RATE_CTRL:
					 STAR_MSG(STAR_ERR, "vStarGetSlewRate() \n");
					  vStarGetSlewRate(starDev,&(pc_cmd->val));
					break;
					
				  case ETH_PHY_EYE_OPEN_CTRL:
	            #ifdef CC_ETHERNET_6896
					  STAR_MSG(STAR_ERR, "MT8560 do not have this setting \n");
	            #endif 
					break;
				
				  case ETH_PHY_BW_50P_CTRL:
					 STAR_MSG(STAR_ERR, "vStarGet50PercentBW() \n");
					  vStarGet50PercentBW(starDev,&(pc_cmd->val));   
				   break;
				   
				  default:
				   STAR_MSG(STAR_ERR, "Get nothing \n");
					 break;
					 
				 }

            }
			 StarPhyEnableAtPoll(starDev);
			 StarIntrClear(starDev, STAR_INT_STA_PORTCHANGE);
             StarEnablePortCHGInt(starDev);
            rc = 0;
		}
        break;

#if defined(ETH_SUPPORT_WOL)
        case SIOC_SET_WOP_CMD:
        {
            struct ioctl_wop_para_cmd *wop_cmd = (struct ioctl_wop_para_cmd *)req->ifr_data;
            star_setWOPProtocolPort(wop_cmd->protocol_type, wop_cmd->port_array, wop_cmd->port_count);
            break;
        }

        case SIOC_CLR_WOP_CMD:
        {
            index = 0;
            break;
        }
#endif
        default:
			spin_lock_irqsave(&star_lock, flags);
			rc = generic_mii_ioctl(&starPrv->mii, if_mii(req), cmd, NULL);
			spin_unlock_irqrestore(&star_lock, flags);
            break;
    }

	return rc;
}

static int mdcMdio_read(struct net_device *dev, int phy_id, int location)
{
	StarPrivate *starPrv = NULL;
	StarDev *starDev = NULL;

	starPrv = netdev_priv(dev);  
	starDev = &starPrv->stardev;

    return (StarMdcMdioRead(starDev, phy_id, location));
}

static void mdcMdio_write(struct net_device *dev, int phy_id, int location, int val)
{
	StarPrivate *starPrv = NULL;
	StarDev *starDev = NULL;

	starPrv = netdev_priv(dev);  
	starDev = &starPrv->stardev;


    StarMdcMdioWrite(starDev, phy_id, location, val);
}

const struct net_device_ops star_netdev_ops = {
	.ndo_open		= star_open,
	.ndo_stop		= star_stop,
	.ndo_start_xmit		= star_start_xmit,
	.ndo_get_stats 		= star_get_stats,
	.ndo_set_rx_mode = star_set_multicast_list,
    .ndo_do_ioctl           = star_ioctl,
#ifdef CONFIG_NET_POLL_CONTROLLER
        .ndo_poll_controller	= star_netpoll,
#endif
	.ndo_change_mtu		= eth_change_mtu,
	.ndo_set_mac_address 	= eth_mac_addr,
	.ndo_validate_addr	= eth_validate_addr,
};

#ifdef STARMAC_SUPPORT_ETHTOOL
static int starmac_get_settings(struct net_device *dev, struct ethtool_cmd *cmd)
{
        StarPrivate *starPrv = NULL;
        int rc;
        unsigned long flags;

        if((dev == NULL)||(cmd == NULL))
        {
                return -1;
        }

        starPrv = netdev_priv(dev);
        spin_lock_irqsave(&starPrv->lock, flags);
        rc = mii_ethtool_gset(&starPrv->mii, cmd);
        spin_unlock_irqrestore(&starPrv->lock, flags);

        return rc;
}

static int starmac_set_settings(struct net_device *dev, struct ethtool_cmd *cmd)
{
        StarPrivate *starPrv = NULL;
        int rc;
        unsigned long flags;
        StarDev *stardev = NULL;

        if((dev == NULL)||(cmd == NULL))
        {
                return -1;
        }

        starPrv = netdev_priv(dev);
        stardev = &(starPrv->stardev);
        spin_lock_irqsave(&starPrv->lock, flags);
        rc = mii_ethtool_sset(&starPrv->mii, cmd);
        if (cmd->autoneg == 0)
        {
                /* set speed and duplex to force mode */
                //stardev->full_duplex = cmd->duplex;
                //stardev->speed_100 = (cmd->speed == SPEED_100) ? 1:0;
                // MmacMacReset(mmac);
        }
        spin_unlock_irqrestore(&starPrv->lock, flags);

        return rc;
}

static int starmac_nway_reset(struct net_device *dev)
{
        StarPrivate *starPrv = NULL;
        int rc;
        unsigned long flags;


        if(dev == NULL)
        {
                return -1;
        }

        starPrv = netdev_priv(dev);
        spin_lock_irqsave(&starPrv->lock, flags);
        rc = mii_nway_restart(&starPrv->mii);
        spin_unlock_irqrestore(&starPrv->lock, flags);
        return rc;
}

static u32 starmac_get_link(struct net_device *dev)
{
        StarPrivate *starPrv = NULL;
        u32 rc;
        unsigned long flags;

        if(dev == NULL)
        {
                return 1;
        }

        starPrv = netdev_priv(dev);
        spin_lock_irqsave(&starPrv->lock, flags);
        rc = mii_link_ok(&starPrv->mii);
        spin_unlock_irqrestore(&starPrv->lock, flags);
        STAR_MSG(STAR_DBG, "ETHTOOL_TEST is called\n");
        return rc;
}

static struct ethtool_ops starmac_ethtool_ops = {
        .get_settings           = starmac_get_settings,
        .set_settings           = starmac_set_settings,
        .nway_reset             = starmac_nway_reset,
        .get_link               = starmac_get_link,
};
#endif

#if defined(ETH_SUPPORT_WOL)
static int star_readWOPSram(void)
{
	u32 i,j;
	u32 temp;

	struct timeval writeSramStart,writeSramEnd;

	do_gettimeofday(&writeSramStart);

	StarSetReg(STAR_WOL_SRAM_STATE(eth_base),WOL_SRAM_CSR_R);
	for(i = 0; i < MAX_WOL_DETECT_BYTES; i++)
	{
		StarSetReg(STAR_WOL_SRAM_CTRL(eth_base),(i & SRAM_ADDR));
		for(j = 0; j < 5; j++)
		{
			temp=StarGetReg(STAR_WOL_SRAM0_CFG0(eth_base + j * 4));
			sramPattern[3 + j * 4][i] = temp >> 24 & 0xff;
			sramPattern[2 + j * 4][i] = temp >> 16 & 0xff;
			sramPattern[1 + j * 4][i]=temp >> 8 & 0xff;
			sramPattern[0 + j * 4][i]=temp & 0xff;
		}
	}

	StarSetReg(STAR_WOL_SRAM_STATE(eth_base),WOL_SRAM_ACTIVE);

	do_gettimeofday(&writeSramEnd);
	STAR_MSG(STAR_VERB, "read sram pattern time(%ld ms)\n", (writeSramEnd.tv_sec - writeSramStart.tv_sec) * 1000 + (writeSramEnd.tv_usec - writeSramStart.tv_usec) / 1000);

	return 0;
}

static int star_writeWOPSram(void)
{
	unsigned int i,delay = 0;
	unsigned int temp;
	struct timeval writeSramStart,writeSramEnd;

	do_gettimeofday(&writeSramStart);

	StarSetReg(STAR_WOL_SRAM_STATE(eth_base),WOL_SRAM_CSR_W);
	for(i = 0; i < MAX_WOL_DETECT_BYTES; i++)
	{
		delay = 0;
		temp = (sramPattern[3][i] << 24) + (sramPattern[2][i] << 16) + (sramPattern[1][i] << 8) + sramPattern[0][i];
		StarSetReg(STAR_WOL_SRAM0_CFG0(eth_base), temp);
		temp = (sramPattern[7][i] << 24) + (sramPattern[6][i] << 16) + (sramPattern[5][i] << 8) + sramPattern[4][i];
		StarSetReg(STAR_WOL_SRAM0_CFG1(eth_base), temp);
		temp = (sramPattern[11][i] << 24) + (sramPattern[10][i] << 16) + (sramPattern[9][i] << 8) + sramPattern[8][i];
		StarSetReg(STAR_WOL_SRAM0_CFG2(eth_base), temp);
		temp = (sramPattern[15][i] << 24) + (sramPattern[14][i] << 16) + (sramPattern[13][i] << 8) + sramPattern[12][i];
		StarSetReg(STAR_WOL_SRAM0_CFG3(eth_base), temp);
		temp = (sramPattern[19][i] << 24) + (sramPattern[18][i] << 16) + (sramPattern[17][i] << 8) + sramPattern[16][i];
		StarSetReg(STAR_WOL_SRAM1_CFG(eth_base), temp);

        if (i == ETH_PROTOCOL_NUM || i == ETH_PROTOCOL_DEST_PORT1 || i == ETH_PROTOCOL_DEST_PORT2)
		    StarSetReg(STAR_WOL_SRAM2_MASK(eth_base),0xfffff);  //enable all pattern byte check
        else
            StarSetReg(STAR_WOL_SRAM2_MASK(eth_base),0x0);  //disable all pattern byte check

		//start write SRAM0
		StarSetReg(STAR_WOL_SRAM_CTRL(eth_base),SRAM0_WE | (i & SRAM_ADDR));
		do
		{
			mdelay(1);
			temp = StarGetReg(STAR_WOL_SRAM_STATUS(eth_base));

			delay++;
			if(delay == 20)
			{
				STAR_MSG(STAR_ERR, "%s write Sram0 failed 0x1f8=0x%x \n", __FUNCTION__, temp);
				goto failed ;
			}

		}while(temp);

		delay = 0;

		//start write SRAM1
		StarSetReg(STAR_WOL_SRAM_CTRL(eth_base),SRAM1_WE | (i & SRAM_ADDR));
		do
		{
			mdelay(1);
			temp = StarGetReg(STAR_WOL_SRAM_STATUS(eth_base));

			delay++;
			if(delay == 20)
			{
				STAR_MSG(STAR_ERR, "%s write Sram1 failed 0x1f8=0x%x \n", __FUNCTION__, temp);
				goto failed ;
			}

		}while(temp);

		delay = 0;
		//start write SRAM2
		StarSetReg(STAR_WOL_SRAM_CTRL(eth_base),SRAM2_WE | (i & SRAM_ADDR));
		do
		{
			mdelay(1);
			temp = StarGetReg(STAR_WOL_SRAM_STATUS(eth_base));

			delay++;
			if(delay == 20)
			{
				STAR_MSG(STAR_ERR, "%s write Sram2 failed 0x1f8=0x%x \n", __FUNCTION__, temp);
				goto failed ;
			}

		}while(temp);
	}

	StarSetReg(STAR_WOL_SRAM_STATE(eth_base), WOL_SRAM_ACTIVE);

	do_gettimeofday(&writeSramEnd);
	STAR_MSG(STAR_VERB, "write sram  time(%ld ms)\n", (writeSramEnd.tv_sec - writeSramStart.tv_sec) * 1000 + (writeSramEnd.tv_usec - writeSramStart.tv_usec) / 1000);

	return 0;
failed:
	StarSetReg(STAR_WOL_SRAM_STATE(eth_base), WOL_SRAM_ACTIVE);
	return -1;
}

static int star_setWOPDetectLen(int pattern, int len)
{
	u32 temp;
	temp = StarGetReg(STAR_WOL_CHECK_LEN0(eth_base + (pattern / 4) * 4));
	temp &= ~(0xff << ((pattern % 4) * 8));
	temp |= (len & 0xff) << ((pattern % 4) * 8);
	StarSetReg(STAR_WOL_CHECK_LEN0(eth_base + (pattern / 4) * 4),temp);

	return 0;
}

#if 0
static int star_setWOLPacket(char pattern[][128], int count)
{
    int i;

    if (index >= 20)
    {
        STAR_MSG(STAR_ERR, "WOP: has already set 20 packets!\n");
        return -1;
    }

    star_readWOPSram();

    for (i = 0; i < count; i++)
    {
        memcpy(sramPattern[index], pattern[i], 128);
        star_setWOPDetectLen(index, 128);
        index++;
    }

    star_writeWOPSram();
    return index - count;
}

static int readWOPSramMask(int pattern, int start, int end)
{
	u32 temp, i, byteMask;

	struct timeval writeSramStart,writeSramEnd;

	do_gettimeofday(&writeSramStart);

	StarSetReg(STAR_WOL_SRAM_STATE(eth_base),WOL_SRAM_CSR_R);
	for(i = start; i <= end; i++)
	{
		StarSetReg(STAR_WOL_SRAM_CTRL(eth_base), i & SRAM_ADDR);
		temp = StarGetReg(STAR_WOL_SRAM2_MASK(eth_base));
		byteMask = ((temp >> pattern) & 1) ? 1 : 0;
	}

	StarSetReg(STAR_WOL_SRAM_STATE(eth_base), WOL_SRAM_ACTIVE);

	do_gettimeofday(&writeSramEnd);
	STAR_MSG(STAR_VERB,"read pattern mask time(%ld ms)\n", (writeSramEnd.tv_sec - writeSramStart.tv_sec) * 1000 + (writeSramEnd.tv_usec - writeSramStart.tv_usec) / 1000);

	return 0;
}

static int star_setWOPSramMask(u32 pattern, u32 flag, int start, int end)
{
	unsigned int i,delay=0;
	unsigned int temp,maskValue;
	struct timeval writeSramStart,writeSramEnd;

	if((start > end) || (end >= 128))
	{
	    STAR_MSG(STAR_ERR,"%s:input error:pattern:%d,flag:%d,start/end:%d/%d\n", __FUNCTION__, pattern, flag, start, end);
		return -1;
	}

	do_gettimeofday(&writeSramStart);

	for(i = start; i <= end;i++)
	{
		StarSetReg(STAR_WOL_SRAM_STATE(eth_base), WOL_SRAM_CSR_R);
		StarSetReg(STAR_WOL_SRAM_CTRL(eth_base), i & SRAM_ADDR);
		maskValue = StarGetReg(STAR_WOL_SRAM2_MASK(eth_base));

		if(flag)
			maskValue = maskValue | (1 << pattern);  //enable pattern and set the pattern date to SRAM
		else
			maskValue = maskValue & (~(1 << pattern)); //disable  pattern and set the pattern date to SRAM

		//start write SRAM2
		StarSetReg(STAR_WOL_SRAM_STATE(eth_base), WOL_SRAM_CSR_W);
		StarSetReg(STAR_WOL_SRAM2_MASK(eth_base), maskValue);  //enable all pattern and set the pattern date to SRAM
		StarSetReg(STAR_WOL_SRAM_CTRL(eth_base), SRAM2_WE | (i & SRAM_ADDR));

		delay = 0;
		do
		{
			mdelay(1);
			temp = StarGetReg(STAR_WOL_SRAM_STATUS(eth_base));
			delay++;
			if(delay == 20)
			{
				STAR_MSG(STAR_ERR, "%s write Sram2 failed 0x1f8=0x%x \n", __FUNCTION__, temp);
				goto failed ;
			}
		} while(temp);
	}

	StarSetReg(STAR_WOL_SRAM_STATE(eth_base), WOL_SRAM_ACTIVE);
	do_gettimeofday(&writeSramEnd);
	STAR_MSG(STAR_VERB, "write mask  time(%ld ms),mask=0x%x\n",(writeSramEnd.tv_sec - writeSramStart.tv_sec) * 1000 + (writeSramEnd.tv_usec - writeSramStart.tv_usec) / 1000, maskValue);

	return 0;
failed:
	StarSetReg(STAR_WOL_SRAM_STATE(eth_base), WOL_SRAM_ACTIVE);
	STAR_MSG(STAR_ERR, "write sram failed");
	return -1;
}
#endif

static int star_setWOPProtocolPort(char protocol_type, int *port_array, int count)
{
    int i;

    if (index >= MAX_WOL_PATTERN_NUM)
    {
        STAR_MSG(STAR_ERR, "star_setWOLPort: has already set 20 packets!\n");
        return -1;
    }

    if (protocol_type != UDP_PROTOCOL && protocol_type != TCP_PROTOCOL)
    {
        STAR_MSG(STAR_ERR, "star_setWOLPort: Protocol type is wrong(UDP = 17, TCP = 6).!\n");
        return -1;
    }

    star_readWOPSram();

    for (i = 0; i < count; i++)
    {
        /* byte23 is udp/tcp protocol number. */
        sramPattern[index][ETH_PROTOCOL_NUM] = protocol_type;

        /* byte36 and byte37 is udp/tcp destination port. */
        sramPattern[index][ETH_PROTOCOL_DEST_PORT1] = (char)(port_array[i] >> 8);
        sramPattern[index][ETH_PROTOCOL_DEST_PORT2] = (char)(port_array[i] & 0xff);
        index++;
    }

    star_writeWOPSram();
    return index - count;
}

#endif
#if  defined(CONFIG_OPM)||defined(CONFIG_ARCH_MT5881)||defined(CONFIG_ARCH_MT5399) || defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5882) || defined(CONFIG_ARCH_MT5883)|| defined(CONFIG_ARCH_MT5891) 
u32 fgOpenStBeforeSuspend=FALSE;//to record open status before enter suspend for recovery after resume
static  int star_suspend(struct platform_device *pdev, pm_message_t state)
{
#if defined(CONFIG_ARCH_MT5881) || defined(CONFIG_ARCH_MT5399) || defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5882) || defined(CONFIG_ARCH_MT5883)|| defined(CONFIG_ARCH_MT5891) 
    #if defined(ETH_SUPPORT_WOL)
    u32 u4Reg = 0;
    u8 i;
    #endif

	struct net_device *netdev = platform_get_drvdata(pdev);

    fgOpenStBeforeSuspend = fgStarOpen;

    STAR_MSG(STAR_DBG, "%s entered, fgOpenStBeforeSuspend=%d\n", __FUNCTION__, fgOpenStBeforeSuspend);	
    if(netdev!=NULL)
    {
        #if defined (CONFIG_ARCH_MT5398) || defined (CONFIG_ARCH_MT5399) || defined (CONFIG_ARCH_MT5880) || defined (CONFIG_ARCH_MT5881) || defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5882) || defined(CONFIG_ARCH_MT5883)|| defined(CONFIG_ARCH_MT5891) 

	if (StarGetReg(STAR_ETH_UP_CFG(eth_base)) & CPUCK_BY_8032)
        {
		STAR_MSG(STAR_DBG, "star_suspend 0x300 = 0x%x\n", StarGetReg(STAR_ETH_UP_CFG(eth_base)));
            	StarSetReg(STAR_ETH_UP_CFG(eth_base), (StarGetReg(STAR_ETH_UP_CFG(eth_base)) & (~CPUCK_BY_8032)));
        }

#if defined(CONFIG_ARCH_MT5891)
		if (IS_IC_MT5891_ES1())
			star_stop(netdev);
		else
		    __star_suspend(netdev);
#else
		star_stop(netdev);
#endif

        if (StarGetReg(STAR_MAC_CFG(eth_base)) & STAR_MAC_CFG_WOLEN)
        {
            StarClearBit(STAR_INT_MASK(eth_base), STAR_INT_STA_MAGICPKT);
        }

        #if defined(ETH_SUPPORT_WOL)
        StarClearBit(STAR_INT_MASK(eth_base), STAR_INT_STA_WOP);
        StarSetBit(STAR_STBY_CTRL(eth_base), STAR_STBY_CTRL_WOL_EN);
        StarSetBit(STAR_WOL_MIRXBUF_CTL(eth_base), WOL_MRXBUF_EN);

        for (i = 0; i < index; i++)
        {
            u4Reg |= 1 << i;
            star_setWOPDetectLen(i, ETH_WOP_LENGTH);
        }

        StarSetReg(STAR_WOL_PATTERN_DIS(eth_base), u4Reg);
        STAR_MSG(STAR_DBG, "star_suspend 0x298 = %x\n", StarGetReg(STAR_WOL_PATTERN_DIS(eth_base)));
        #endif

		STAR_MSG(STAR_DBG, "star_suspend 0x3208 value(0x%x)\n", StarGetReg(STAR_ETH_PDREG_SOFT_RST(eth_base)));

#if defined(CONFIG_ARCH_MT5891)
		if (IS_IC_MT5891_ES1()) {
			//todo
		} else {
			StarClearBit(STAR_ETH_PDREG_ETCKSEL(eth_base), ETH_AFE_PWD2);
		}
#endif

		StarSetBit(STAR_ETH_UP_CFG(eth_base), CPUCK_BY_8032);

        #ifdef SUPPORT_ETH_POWERDOWN
        Ethernet_suspend(pdev);
        #endif
        #endif
    }
#endif
	return 0;
}
#if defined(CONFIG_ARCH_MT5891)
static int star_free_buf(struct net_device *dev)
{
	StarPrivate *starPrv = NULL;
	StarDev *starDev = NULL;
	int retval;
	u32 intrStatus;

	STAR_MSG(STAR_DBG, "star_free_buf(%s), fgStarOpen(%d)\n", dev->name, fgStarOpen);

    if(fgStarOpen == FALSE)
		return -1 ;

	starPrv = netdev_priv(dev);
	starDev = &starPrv->stardev;

	StarIntrDisable(starDev);
	StarDmaTxStop(starDev);
	StarDmaRxStop(starDev);
	intrStatus = StarIntrStatus(starDev);
	StarIntrClear(starDev, intrStatus);

	do { /* Free Tx descriptor */
        uintptr_t extBuf;
		u32 ctrlLen;
		u32 len;
		u32 dmaBuf;

		retval = StarDmaTxGet(starDev, (u32*)&dmaBuf, &ctrlLen, &extBuf);
		if (retval >= 0 && extBuf != 0)
		{
			len = StarDmaTxLength(ctrlLen);
			dma_unmap_single(mtk_dev, dmaBuf, len, DMA_TO_DEVICE);
			STAR_MSG(STAR_DBG, "star_free_buf - get tx descriptor idx:%d for skb 0x%lx\n", retval, extBuf);

			dev_kfree_skb((struct sk_buff *)extBuf);
		}
	} while (retval >= 0);

	do { /* Free Rx descriptor */
		uintptr_t extBuf;
		u32 dmaBuf;

		retval = StarDmaRxGet(starDev, (u32 *)&dmaBuf, NULL, &extBuf);
		if (retval >= 0 && extBuf != 0)
		{
			dma_unmap_single(mtk_dev, dmaBuf, skb_tailroom((struct sk_buff *)extBuf), DMA_FROM_DEVICE);
						STAR_MSG(STAR_DBG, "star_free_buf - get tx descriptor idx:%d for skb 0x%lx\n", retval, extBuf);
			dev_kfree_skb((struct sk_buff *)extBuf);
		}
	} while (retval >= 0);

    /* Stop RX FIFO receive */
    StarNICPdSet(starDev, 1);

    fgStarOpen = FALSE ;
	return 0;
}
#endif
static  int star_resume(struct platform_device *pdev)
{
#if defined(CONFIG_ARCH_MT5881) || defined(CONFIG_ARCH_MT5399) || defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5882) || defined(CONFIG_ARCH_MT5883)|| defined(CONFIG_ARCH_MT5891) 
	u32 u4Reg;

	#if defined(CONFIG_ARCH_MT5891)
    void __iomem *reg_addr;
	int power_off_flag = 0; // check mac whether power down in standy mode
    #endif

	struct net_device *netdev = platform_get_drvdata(pdev);

	STAR_MSG(STAR_DBG, "%s entered\n", __FUNCTION__);
#if defined (CONFIG_ARCH_MT5398) || defined (CONFIG_ARCH_MT5399) || defined (CONFIG_ARCH_MT5880) || defined (CONFIG_ARCH_MT5881) || defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5882) || defined(CONFIG_ARCH_MT5883)|| defined(CONFIG_ARCH_MT5891) 
	StarClearBit(STAR_ETH_UP_CFG(eth_base), CPUCK_BY_8032);
#endif

#if defined(CONFIG_ARCH_MT5891)
	if (IS_IC_MT5891_ES1()) {
		// todo
		} else {
			u4Reg = StarGetReg(STAR_ETH_PDREG_SOFT_RST(eth_base));
			u4Reg &= ~(0x7fff);
			/*
			*  It's an ugly way to check whether pdwnc_hw.c power off MAC. There isn't fuction
			*  or MAC register can check this, so here, check 0x308 is default value workaround. shit!
			*/
			if (u4Reg != 0x7f83)
				power_off_flag = 1;
			u4Reg |= 0x7f83;
			StarSetReg(STAR_ETH_PDREG_SOFT_RST(eth_base), u4Reg);

			u4Reg = StarGetReg(STAR_ETH_PDREG_ETCKSEL(eth_base));
			u4Reg &= ~RG_DA_XTAL1_SEL;
			StarSetReg(STAR_ETH_PDREG_ETCKSEL(eth_base), u4Reg);

			u4Reg = StarGetReg(pdwnc_base + 0x14);
			u4Reg &= ~(0x1f<<16);
			StarSetReg(pdwnc_base + 0x14, u4Reg);

			u4Reg = StarGetReg(STAR_ETH_PDREG_ENETPLS(eth_base));
			u4Reg &= ~(PLS_MONCKEN_PRE | PLS_FMEN_PRE);
			StarSetReg(STAR_ETH_PDREG_ENETPLS(eth_base), u4Reg);
		}
#endif

    #if defined(CONFIG_ARCH_MT5882)
	if (IS_IC_MT5885())
	{
	   STAR_MSG(STAR_DBG, "Open Ethernet WOL power!\n");
	   mtk_gpio_set_value(219,0);
	}
    #endif

	if(netdev!=NULL)
	{
		StarPrivate *starPrv = NULL;
	    StarDev *starDev = NULL;
		starPrv = netdev_priv(netdev);
		starDev = &starPrv->stardev;

		#if defined (CONFIG_ARCH_MT5398) || defined (CONFIG_ARCH_MT5399) || defined (CONFIG_ARCH_MT5880) || defined (CONFIG_ARCH_MT5881) || defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5882) || defined(CONFIG_ARCH_MT5883)|| defined(CONFIG_ARCH_MT5891) 
		STAR_MSG(STAR_VERB, "0x300 register is %x\n", StarGetReg(STAR_ETH_UP_CFG(eth_base)));

#if defined(CONFIG_ARCH_MT5891)
		if (IS_IC_MT5891_ES1()) {
	        u4Reg = StarGetReg(ipll_base1+0x164);
	        u4Reg = u4Reg & (~(0x1 <<1));
	        StarSetReg(ipll_base1+0x164, u4Reg);
	        StarWaitReady(ipll_base1+0x164, 0x0,1,0x1,1,1);
		} else {
		/* fix wukong es2 enter suspend use __star_suspend but resume back use star_open case,
		 * to fix memory leak
		*/
			if (power_off_flag == 1)
				star_free_buf(netdev);
		}
#else
        u4Reg = StarGetReg(ipll_base1+0x164);
        u4Reg = u4Reg & (~(0x1 <<1));
        StarSetReg(ipll_base1+0x164, u4Reg);
        StarWaitReady(ipll_base1+0x164, 0x0,1,0x1,1,1);
#endif

        #if defined (CONFIG_ARCH_MT5398) || defined (CONFIG_ARCH_MT5880) || defined (CONFIG_ARCH_MT5881)
		u4Reg = StarGetReg(eth_base+0x308);
        /*only set bit7 for 5398, 5880, 5881*/
        /*As since 5399,  enetpll_d3_ck=200Mhz for SD3.0 and emmc4.5 hs200 mode*/
        /*so bit7 also controls the pll for SD and emmc; bit7 cannot be setted here*/
        u4Reg = u4Reg | 0x80;
        StarSetReg(eth_base+0x308, u4Reg);
         StarWaitReady(eth_base+0x308, 0x1,7,0x1,1,1);
        #endif

#if defined(CONFIG_ARCH_MT5891)
#else		
		u4Reg = StarGetReg(eth_base+0x308);
		u4Reg = u4Reg | 0x3;
		   StarSetReg(eth_base+0x308, u4Reg);
		   mdelay(10);
#endif

 #if defined(CONFIG_ARCH_MT5891)
        STAR_MSG(STAR_VERB, "Ethernet Interrupt register is 0x%x\n", StarGetReg(eth_base + 0x50));
        if (StarIntrStatus(starDev) & STAR_INT_STA_MAGICPKT)
        {
            STAR_MSG(STAR_VERB, "magic packet data length = %d\n", StarGetReg(STAR_MGP_LENGTH_STA(eth_base)));

		    for(reg_addr = STAR_MGP_DATA_START(eth_base); reg_addr <= STAR_MGP_DATA_END(eth_base); reg_addr += 4)
		    {
                if ((uintptr_t)reg_addr % 16 == 0)
                     STAR_MSG(STAR_VERB, "\n");
			    STAR_MSG(STAR_VERB, "0x%08x ", StarGetReg(reg_addr));
		    }
        }
        #if defined(ETH_SUPPORT_WOL)
		StarClearBit(STAR_WOL_MIRXBUF_CTL(eth_base), WOL_MRXBUF_EN);
		StarSetBit(STAR_WOL_MIRXBUF_CTL(eth_base), WOL_MRXBUF_RESET);
		StarClearBit(STAR_STBY_CTRL(eth_base), 1);
		StarClearBit(STAR_WOL_MIRXBUF_CTL(eth_base), WOL_MRXBUF_RESET);

        StarSetReg(STAR_WOL_PATTERN_DIS(eth_base), 0);
        #endif
#endif

        if (StarGetReg(STAR_MAC_CFG(eth_base)) & STAR_MAC_CFG_NICPD)
        {
            StarNICPdSet(starDev, 0);
        }

#if defined(CONFIG_ARCH_MT5891)
		if (IS_IC_MT5891_ES1() || power_off_flag == 1) {
			if (StarHwInit(starDev) != 0)
				STAR_MSG(STAR_ERR, "Ethernet MAC HW init fail!\n");
		}
#else
		 if(StarHwInit(starDev) !=0)
		 {
			 STAR_MSG(STAR_ERR, "%s,StarHwInit fail!!!\n", __FUNCTION__);;
		 }
#endif

         if(fgOpenStBeforeSuspend)
        {
#if defined(CONFIG_ARCH_MT5891)
			if (IS_IC_MT5891_ES1() || power_off_flag == 1) {
				if(star_open(netdev) !=0)
					STAR_MSG(STAR_ERR, "%s,star_open fail!!!\n", __FUNCTION__);;
				/* After resume, Ethernet HW will be reset, and multicast address info will be cleared. */
				star_set_multicast_list(netdev);
				} else {
	    	    if(__star_resume(netdev) !=0)
	                STAR_MSG(STAR_ERR, "%s,__star_resume() fail!!!\n", __FUNCTION__);;
			}
#else
			if(star_open(netdev) !=0)
			{
				STAR_MSG(STAR_ERR, "%s,star_open fail!!!\n", __FUNCTION__);;
			}
			/* After resume, Ethernet HW will be reset, and multicast address info will be cleared. */
			star_set_multicast_list(netdev);
#endif
        }
        #ifdef SUPPORT_ETH_POWERDOWN
        if(Ethernet_resume(pdev) != 0)
        {
            STAR_MSG(STAR_ERR, "%s,Ethernet_resume fail!!!\n", __FUNCTION__);;
        }
        #endif
		#endif
	}	
#endif
	return 0;
}
#endif

#ifdef SUPPORT_ETH_POWERDOWN	 
int Ethernet_suspend(struct platform_device *pdev)
{	
    STAR_MSG(STAR_ERR,"star:suspend\n");
	
#if defined (CONFIG_ARCH_MT5398) || defined (CONFIG_ARCH_MT5399) || defined (CONFIG_ARCH_MT5880)|| defined (CONFIG_ARCH_MT5881) || defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5882) || defined(CONFIG_ARCH_MT5883)|| defined(CONFIG_ARCH_MT5891) 

    struct net_device *netdev = platform_get_drvdata(pdev);
	if(netdev!=NULL )
	{
	 	StarPrivate *starPrv = NULL;
	    StarDev *starDev = NULL;
		starPrv = netdev_priv(netdev);

		//starPrv = netdev_priv(dev);  
		starDev = &starPrv->stardev;

        StarPhyDisableAtPoll(starDev);
	    StarMdcMdioWrite(starDev, starPrv->phy_addr, 0x1f, 0x2a30);  //test page
	    StarMdcMdioWrite(starDev, starPrv->phy_addr, 0x00, 0x00);  
		StarMdcMdioWrite(starDev, starPrv->phy_addr, 0x01, 0x0080); 
		StarMdcMdioWrite(starDev, starPrv->phy_addr, 0x02, 0x0000); 
		StarMdcMdioWrite(starDev, starPrv->phy_addr, 0x12, StarMdcMdioRead(starDev, starPrv->phy_addr,0x12)&(~(1<<13)));//PL_BIAS_CTRL_MODE 
	    StarMdcMdioWrite(starDev, starPrv->phy_addr, 0x12, StarMdcMdioRead(starDev, starPrv->phy_addr,0x12)|((1<<11)));// 
	    StarMdcMdioWrite(starDev, starPrv->phy_addr, 0x12, StarMdcMdioRead(starDev, starPrv->phy_addr,0x12)&(~(1<<5)));//

		StarMdcMdioWrite(starDev, starPrv->phy_addr, 0x13, StarMdcMdioRead(starDev, starPrv->phy_addr,0x13)|((1<<6)));// PLL power down
		StarMdcMdioWrite(starDev, starPrv->phy_addr, 0x14, StarMdcMdioRead(starDev, starPrv->phy_addr,0x14)|((1<<4)|(1<<5)));// MDI power down
		StarMdcMdioWrite(starDev, starPrv->phy_addr, 0x16, StarMdcMdioRead(starDev, starPrv->phy_addr,0x16)|((1<<14)|(1<<13)));// BG power down
		StarMdcMdioWrite(starDev, starPrv->phy_addr, 0x1F, 0x00);  //main page
		//StarSetBit(STAR_PHY_CTRL1(dev->base),SWH_CK_PWN | INTER_PYH_PD);
		//StarSetBit(STAR_MAC_CFG(dev->base), NIC_PD);
		StarPhyEnableAtPoll(starDev);

		//N_Mask(0x0304,(0<<19),(1<<19) ) ;  ///disable phy clock
		//N_Mask(0x0300,(0<<7),(1<<7) ) ;   ///disable mac clock
		fgNetHWActive = 0;

	}
	
#endif

	return 0;
}	

int Ethernet_resume(struct platform_device *pdev)
{
	STAR_MSG(STAR_ERR,"star:resume\n");
	
#if defined (CONFIG_ARCH_MT5398) || defined (CONFIG_ARCH_MT5399) || defined (CONFIG_ARCH_MT5880)|| defined (CONFIG_ARCH_MT5881) || defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5882) || defined(CONFIG_ARCH_MT5883)|| defined(CONFIG_ARCH_MT5891) 
    struct net_device *netdev = platform_get_drvdata(pdev);
	if( netdev!=NULL && fgNetHWActive == 0)
	{
		StarPrivate *starPrv = NULL;
		StarDev *starDev = NULL;

		//struct net_device *star_device = dev_get_drvdata(dev) ;
		starPrv = netdev_priv(netdev);
		starDev = &starPrv->stardev;

		StarPhyDisableAtPoll(starDev);
		StarMdcMdioWrite(starDev, starPrv->phy_addr, 0x1f, 0x2a30);  //test page
		if(StarMdcMdioRead(starDev, starPrv->phy_addr, 0x01)!= 0x0000)
		{
		   
			StarMdcMdioWrite(starDev, starPrv->phy_addr, 0x00, 0x00);  
			StarMdcMdioWrite(starDev, starPrv->phy_addr, 0x01, 0x0000); 
			StarMdcMdioWrite(starDev, starPrv->phy_addr, 0x02, 0x0000); 
			StarMdcMdioWrite(starDev, starPrv->phy_addr, 0x12, StarMdcMdioRead(starDev, starPrv->phy_addr,0x12)|((1<<13)));//PL_BIAS_CTRL_MODE 
			StarMdcMdioWrite(starDev, starPrv->phy_addr, 0x12, StarMdcMdioRead(starDev, starPrv->phy_addr,0x12)&(~(1<<11)));// 
			StarMdcMdioWrite(starDev, starPrv->phy_addr, 0x12, StarMdcMdioRead(starDev, starPrv->phy_addr,0x12)|((1<<5)));//
		
			StarMdcMdioWrite(starDev, starPrv->phy_addr, 0x13, StarMdcMdioRead(starDev, starPrv->phy_addr,0x13)&(~(1<<6)));// PLL power down
			StarMdcMdioWrite(starDev, starPrv->phy_addr, 0x14, StarMdcMdioRead(starDev, starPrv->phy_addr,0x14)&(~((1<<4)|(1<<5))));// MDI power down
			StarMdcMdioWrite(starDev, starPrv->phy_addr, 0x16, StarMdcMdioRead(starDev, starPrv->phy_addr,0x16)&(~((1<<14)|(1<<13))));// BG power down
		}
		StarMdcMdioWrite(starDev, starPrv->phy_addr, 0x1F, 0x00);  //main page
        StarPhyEnableAtPoll(starDev);
		//StarIntPhyInit(starDev);

        fgNetHWActive = 1 ;
			//StarClearBit(STAR_PHY_CTRL1(dev->base),SWH_CK_PWN | INTER_PYH_PD);
			//StarClearBit(STAR_MAC_CFG(dev->base), NIC_PD);
       
	}
#endif
	
	return 0;
}
#endif//#ifdef SUPPORT_ETH_POWERDOWN


static int star_probe(struct platform_device *pdev)
{
	int result = 0;
	u8 macAddr[ETH_ADDR_LEN] = DEFAULT_MAC_ADDRESS;
    StarPrivate *starPrv = NULL;
	StarDev *starDev;
    struct net_device *netdev = NULL;

	/* Base Register of  Ethernet */
		/* DO NOT use ioremap to mapping ethernet hardware address */
#ifdef CONFIG_OF
		struct device_node *np1;
		np1 = of_find_compatible_node(NULL, NULL, "Mediatek, MAC");
		if (!np1) {
			panic("%s: STARMAC node not found\n", __func__);
		}
		eth_base = of_iomap(np1, 0);
		pinmux_base = of_iomap(np1, 1);
		ipll_base1 = of_iomap(np1, 2);
	#if defined(CONFIG_ARCH_MT5891)
		pdwnc_base = of_iomap(np1, 3);
	#endif
#else
		eth_base = ioremap(ETH_BASE, 320);
		pinmux_base = ioremap(PINMUX_BASE, 320);
		ipll_base1 = ioremap(IPLL1_BASE, 320);
	#if defined(CONFIG_ARCH_MT5891)
		pdwnc_base = ioremap(PDWNC_BASE, 320);
	#endif
#endif
	#if defined(CONFIG_ARCH_MT5891)
		if (!pdwnc_base)
			panic("Impossible to ioremap pdwnc_base starmac!!\n");
		printk(KERN_DEBUG "star:pdwnc_base: 0x%p\n", pdwnc_base);
	#endif
		if ((!eth_base) || (!pinmux_base) ||(!ipll_base1))
			panic("Impossible to ioremap starmac!!\n");
		printk(KERN_DEBUG "star:eth_base: 0x%p\n", eth_base);
		printk(KERN_DEBUG "star:pinmux_base: 0x%p\n", pinmux_base);
		printk(KERN_DEBUG "star:ipll_base1: 0x%p\n", ipll_base1);

#if  defined(CONFIG_OPM)||defined(CONFIG_ARCH_MT5881)||defined(CONFIG_ARCH_MT5399) || defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5882)  || defined(CONFIG_ARCH_MT5883)|| defined(CONFIG_ARCH_MT5891) 
    //struct device *dev; 
#endif
	mtk_dev = &pdev->dev;
	pdev->dev.coherent_dma_mask = DMA_BIT_MASK(32);
	pdev->dev.dma_mask = &pdev->dev.coherent_dma_mask;
    STAR_MSG(STAR_VERB, "%s entered\n", __FUNCTION__);
	netdev = alloc_etherdev(sizeof(StarPrivate));
	if (!netdev)
	{
		result = -ENOMEM;
		goto out;
	}

    this_device = netdev;
    
    SET_NETDEV_DEV(netdev, &pdev->dev);
	
#if 0
	/* PDWNC base Register */
	pdwnc_base = (u32)(ioremap_nocache(PDWNC_BASE+PDWNC_BASE_CFG_OFFSET, 320)); 
	if (!pdwnc_base)
	{
		result = -ENOMEM;
		goto out;
	}
	STAR_MSG(STAR_DBG, "pdwnc_base=0x%08x\n", pdwnc_base);

	/* Ethernet PDWNC Register */
	eth_pdwnc_base = (u32)(ioremap_nocache(ETHERNET_PDWNC_BASE, 320)); 
	if (!eth_pdwnc_base)
	{
		result = -ENOMEM;
		goto out;
	}
	STAR_MSG(STAR_DBG, "eth_pdwnc_base=0x%08x\n", eth_pdwnc_base);

	/* Ethernet checksum Register */
	eth_chksum_base = (u32)(ioremap_nocache(ETH_CHKSUM_BASE, 320)); 
	if (!eth_chksum_base)
	{
		result = -ENOMEM;
		goto out;
	}
	STAR_MSG(STAR_DBG, "eth_chksum_base=0x%08x\n", eth_chksum_base);

	

	/* IPLL2 Register */
	ipll_base2 = (u32)(ioremap_nocache(IPLL2_BASE, 320)); 
	if (!ipll_base2)
	{
		result = -ENOMEM;
		goto out;
	}
	STAR_MSG(STAR_DBG, "ipll_base2=0x%08x\n", ipll_base2);

    bsp_base = (u32)(ioremap_nocache(BSP_BASE, 16));
    if (!bsp_base)
    {
        result = -ENOMEM;
        goto out;
    }
	STAR_MSG(STAR_DBG, "bsp_base=0x%08x\n", bsp_base);
#endif

  	starPrv = netdev_priv(netdev);
	memset(starPrv, 0, sizeof(StarPrivate));
	starPrv->dev = netdev;

#if defined (USE_TX_TASKLET) || defined (USE_RX_TASKLET)
	tasklet_init(&starPrv->dsr, star_dsr, (unsigned long)netdev);
#endif

    /* Init system locks */
	spin_lock_init(&starPrv->lock);
    spin_lock_init(&star_lock);
    spin_lock_init(&starPrv->tsk_lock);

	starPrv->desc_virAddr = (uintptr_t)dma_alloc_coherent(
                                    mtk_dev,
                                    TX_DESC_TOTAL_SIZE + RX_DESC_TOTAL_SIZE, 
                                    &(starPrv->desc_dmaAddr), 
                                    GFP_KERNEL|GFP_DMA);
	if (!starPrv->desc_virAddr)
	{
		result = -ENOMEM;
		goto out;
	}
	//STAR_MSG(STAR_DBG, "desc addr: 0x%lx(virtual)/0x%llx(physical)\n", 
    //         starPrv->desc_virAddr, 
    //         starPrv->desc_dmaAddr);
    
  	starDev = &starPrv->stardev;
	StarDevInit(starDev, eth_base);
	starDev->pinmuxBase = pinmux_base;
	starDev->ethIPLL1Base = ipll_base1;

    StarGetHwMacAddr(starDev, macAddr);
	#if 0
    starDev->pdwncBase = pdwnc_base;
    starDev->pinmuxBase = pinmux_base;
    starDev->ethPdwncBase = eth_pdwnc_base;
    starDev->ethChksumBase = eth_chksum_base;
    starDev->ethIPLL1Base = ipll_base1;
    starDev->ethIPLL2Base = ipll_base2;
    starDev->bspBase = bsp_base;
    #endif
    
    starDev->starPrv = starPrv;

    if((0==StarGetReg(starDev->base+0x14)) && (0==StarGetReg(starDev->base+0x18)))
    {
        STAR_MSG(STAR_ERR, "%s, Not support network, IS_SupportNETWORK=FALSE, MAC is not set, so do not init ethernet hw\n", __FUNCTION__);
        result = -ENODEV;
        goto out;
    }
    else
    {
        /* Init hw related settings (eg. pinmux, clock ...etc) */
        if (StarHwInit(starDev) != 0)
        {
            STAR_MSG(STAR_ERR, "Ethernet MAC HW init fail!\n");
            result = -ENODEV;
            goto out;
        }
    }

    StarNICPdSet(starDev , 1);

	starPrv->mii.dev = netdev;
	starPrv->mii.mdio_read = mdcMdio_read;
	starPrv->mii.mdio_write = mdcMdio_write;
	starPrv->mii.phy_id_mask = 0x1f;
	starPrv->mii.reg_num_mask = 0x1f;
	

    netdev->addr_len = ETH_ADDR_LEN;
	memcpy(netdev->dev_addr, macAddr, netdev->addr_len);       /* Set MAC address */
	netdev->irq = ETH_IRQ;
	netdev->base_addr = (unsigned long)eth_base;
	netdev->netdev_ops = &star_netdev_ops;
#ifdef STARMAC_SUPPORT_ETHTOOL
    STAR_MSG(STAR_DBG, "EthTool installed\n");
    netdev->ethtool_ops = &starmac_ethtool_ops;
#endif
#ifdef USE_RX_NAPI
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26)
	netif_napi_add(netdev, &starPrv->napi, star_poll, 64);
#else
	netdev->poll = star_poll;
	netdev->weight = 64;
#endif
#endif

	/* Check Ethernet Hardware is star or not by AHB Burst Type Register */
	if (StarGetReg(STAR_AHB_BURSTTYPE(eth_base)) != 0x77)
	{
	    STAR_MSG(STAR_ERR, "Ethernet MAC HW is unknown!\n");
		result = -EFAULT;
		goto out;
	}

	if ((result = register_netdev(netdev)) != 0)
	{
		unregister_netdev(netdev);
		goto out;
	}

	platform_set_drvdata(pdev, netdev);

#if  defined(CONFIG_OPM)||defined(CONFIG_ARCH_MT5881)||defined(CONFIG_ARCH_MT5399) || defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5882) || defined(CONFIG_ARCH_MT5883)|| defined(CONFIG_ARCH_MT5891) 
	//dev= &(this_device->dev);
	//dev->class->suspend = star_suspend;
	//dev->class->resume = star_resume;
#endif
    
    STAR_MSG(STAR_WARN, "Star MAC init done\n");
    skb_dbg = NULL;
    star_dev = starDev;

//    star_open(starPrv->dev);
//    star_stop(starPrv->dev);

	return 0;
	
out:

    if (starPrv->desc_virAddr)
    {
	    dma_free_coherent(mtk_dev, 
                        TX_DESC_TOTAL_SIZE + RX_DESC_TOTAL_SIZE, 
                        (void *)starPrv->desc_virAddr, 
                        starPrv->desc_dmaAddr);
    }
#if 0    
    if (bsp_base)
    {
        iounmap((u32 *)bsp_base);
        bsp_base = 0;
    }
    
    if (ipll_base2)
    {
        iounmap((u32 *)ipll_base2);
        ipll_base2 = 0;
    }
    
    if (eth_chksum_base)
    {
        iounmap((u32 *)eth_chksum_base);
        eth_chksum_base = 0;
    }
    if (eth_pdwnc_base)
    {
        iounmap((u32 *)eth_pdwnc_base);
        eth_pdwnc_base = 0;
    }
    if (pdwnc_base)
    {
        iounmap((u32 *)pdwnc_base);
        pdwnc_base = 0;
    }
    
#endif    
    if (netdev)
    {
		unregister_netdev(netdev);
        free_netdev(netdev);
        netdev = NULL;
    }
    return result;
	
}

static int star_cleanup(struct platform_device *pdev)
{
    struct net_device *netdev = platform_get_drvdata(pdev);
    
	if (netdev)
	{
		StarPrivate *starPrv;

		starPrv = netdev_priv(netdev);

		unregister_netdev(netdev);
		dma_free_coherent(mtk_dev, 
                          TX_DESC_TOTAL_SIZE + RX_DESC_TOTAL_SIZE, 
                          (void *)starPrv->desc_virAddr, 
                          starPrv->desc_dmaAddr);
#if 0                          
        if (bsp_base)
        {
            iounmap((u32 *)bsp_base);
            bsp_base = 0;
        }
        if (ipll_base2)
        {
            iounmap((u32 *)ipll_base2);
            ipll_base2 = 0;
        }
        
        if (eth_chksum_base)
        {
            iounmap((u32 *)eth_chksum_base);
            eth_chksum_base = 0;
        }
        if (eth_pdwnc_base)
        {
            iounmap((u32 *)eth_pdwnc_base);
            eth_pdwnc_base = 0;
        }
        if (pdwnc_base)
        {
            iounmap((u32 *)pdwnc_base);
            pdwnc_base = 0;
        }
        
#endif       
        if (ipll_base1)
        {
            iounmap((u32 *)ipll_base1);
            ipll_base1 = 0;
        }
        
        if (pinmux_base)
        {
            iounmap((u32 *)pinmux_base);
            pinmux_base = 0;
        }
         
        if (eth_base)
        {
            iounmap((u32 *)eth_base);
            eth_base = 0;
        }
		free_netdev(netdev);
	} 
    star_dev = NULL;
    return 0;
}

void star_if_up_down(int up)
{
    struct net_device * dev = this_device;

    if (!dev)
        return;
    if (up)
        star_open(dev);
    else
        star_stop(dev);
}


void star_if_queue_up_down(struct up_down_queue_par *par)
{
  struct net_device * dev = this_device;

  if(par->up)
  {
    STAR_MSG(STAR_DBG, "star_wake_queue(%s)\n", dev->name);
    netif_wake_queue(dev);
    	
  }	
  else
  {
    STAR_MSG(STAR_DBG, "star_stop_queue(%s)\n", dev->name);
    netif_stop_queue(dev);
    
  } 	
	
}

EXPORT_SYMBOL(star_if_up_down);
EXPORT_SYMBOL(star_cleanup);

static struct platform_device *star_pdev;

struct platform_driver star_pdrv = {
    .driver = {
        .name = STAR_DRV_NAME,
        .owner = THIS_MODULE,
    },
    .probe = star_probe,
    .suspend = star_suspend,
    .resume = star_resume,
    .remove = star_cleanup,
};

static int __init star_init(void)
{
    int     err;

    pr_info("WDT driver for Acquire single board computer initialising\n");

    err = platform_driver_register(&star_pdrv);
    if (err) {
        return err;
    }

    star_pdev = platform_device_register_simple(STAR_DRV_NAME, -1, NULL, 0);
    if (IS_ERR(star_pdev)) {
        err = PTR_ERR(star_pdev);
        goto unreg_platform_driver;
    }
    STAR_MSG(STAR_WARN, "%s success...\n", __FUNCTION__);
    return 0;

  unreg_platform_driver:
    platform_driver_unregister(&star_pdrv);
    return err;
}

static void __exit star_exit(void)
{
    platform_device_unregister(star_pdev);
    platform_driver_unregister(&star_pdrv);
    STAR_MSG(STAR_WARN, "%s ...\n", __FUNCTION__);
}

#ifndef CONFIG_MT53_FPGA
module_init(star_init)
module_exit(star_exit)
#endif

#ifdef MODULE
MODULE_LICENSE("GPL");
#endif
