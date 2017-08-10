#ifndef _STAR_DEV_H_
#define _STAR_DEV_H_

#include <asm/types.h>
#include <linux/netdevice.h>
#include <linux/delay.h>
#include "star_reg.h"
//#include "switch_reg.h"
#include "star_regUtil.h"

#define TRUE 1
#define FALSE 0

#define TX_DESC_NUM         16// 8
#define RX_DESC_NUM         16

#define STARMAC_SUPPORT_ETHTOOL

#ifndef TX_DESC_NUM
#define TX_DESC_NUM         8
#endif
#ifndef RX_DESC_NUM
#define RX_DESC_NUM         16
#endif

#ifndef ETH_ADDR_LEN
#define ETH_ADDR_LEN        6
#endif

#define TX_DESC_TOTAL_SIZE  (sizeof(TxDesc)*TX_DESC_NUM)
#define RX_DESC_TOTAL_SIZE  (sizeof(RxDesc)*RX_DESC_NUM)


#define INTERNAL_PHY_SLEW_RATE_LEVEL 3
#define INTERNAL_PHY_OUTPUT_BIAS_LEVEL 1 //0:+0%, 1,2:+100%, 3:+200%
#ifdef CC_ETHERNET_6896
#define INTERNAL_PHY_INPUT_BIAS_LEVEL 1 //0:66%, 1:200%, 2:+50%, 3:100%
#define INTERNAL_PHY_10M_AMP_LEVEL 0 //0:110%, 1:108.3% 2:105%, 3:103%
#define INTERNAL_PHY_FEED_BACK_CAP 3 //0:min compensation cap, 1:,2: Modeate, 3:Max
#ifdef KC_EU_PCBA
#define INTERNAL_PHY_DACAMP 6        //min:0x0 , max 0xf
#define INTERNAL_PHY_100AMP 5        //min:0x0 , max 0xf
#else
#define INTERNAL_PHY_DACAMP 4        //min:0x0 , max 0xf
#define INTERNAL_PHY_100AMP 5        //min:0x0 , max 0xf
#endif
#else
#define INTERNAL_PHY_INPUT_BIAS_LEVEL 1 //0:200%, 1:100%, 2:66%, 3:50%
#define INTERNAL_PHY_10M_AMP_LEVEL 2 //0:110%, 1:108.3% 2:105%, 3:103%
#define INTERNAL_PHY_FEED_BACK_CAP 1 //0:min compensation cap, 1:,2: Modeate, 3:Max
#define INTERNAL_PHY_DACAMP 4        //min:0x0 , max 0xf
#define INTERNAL_PHY_100AMP 5        //min:0x0 , max 0xf
#endif

#define INTERNAL_PHY_OUT_EXTRA_BAIS_CONTROL 1  //0:+0% 1:+100%
#define INTERNAL_PHY_EXTEND_10M_AMP_LEVEL 0 //0~0xf Increase
#define INTERNAL_PHY_50_PERCENT_BW_ENABLE 0 //0: BW 100% 1:50%

#if defined(CONFIG_ARCH_MT5891)
#define ETH_SUPPORT_WOL
#endif

/**
 * @brief Internal PHY or switch PHY ID
 */
#define INT_PHYID			0
#define INT_SWITCH_PHYID0	0
#define INT_SWITCH_PHYID1	1

/**
 * @brief structure for Tx descriptor Ring
 */
typedef struct txDesc_s	/* Tx Ring */
{
#ifdef CC_ETHERNET_6896
u32 ctrlLen;	/* Tx control and length */
	#define TX_COWN 				(1 << 31)	/* Tx descriptor Own bit; 1: CPU own */
	#define TX_EOR					(1 << 30)	/* End of Tx descriptor ring */
	#define TX_FS					(1 << 29)	/* First Segment descriptor */
	#define TX_LS					(1 << 28)	/* Last Segment descriptor */
	#define TX_INT					(1 << 27)	/* Tx complete interrupt enable (when set, DMA generate interrupt after tx sending out pkt) */
	#define TX_INSV 				(1 << 26)	/* Insert VLAN Tag in the following word (in tdes2) */
	#define TX_ICO					(1 << 25)	/* Enable IP checksum generation offload */
	#define TX_UCO					(1 << 24)	/* Enable UDP checksum generation offload */
	#define TX_TCO					(1 << 23)	/* Enable TCP checksum generation offload */
	#define TX_LEN_MASK 			(0xffff)	/* Tx Segment Data length */
	#define TX_LEN_OFFSET			(0)
u32 buffer; 	/* Tx segment data pointer */
#else
	u32 buffer;		/* Tx segment data pointer */
	u32 ctrlLen;	/* Tx control and length */
		#define TX_COWN					(1 << 31)	/* Tx descriptor Own bit; 1: CPU own */
		#define TX_EOR					(1 << 30)	/* End of Tx descriptor ring */
		#define TX_FS					(1 << 29)	/* First Segment descriptor */
		#define TX_LS					(1 << 28)	/* Last Segment descriptor */
		#define TX_INT					(1 << 27)	/* Tx complete interrupt enable (when set, DMA generate interrupt after tx sending out pkt) */
		#define TX_INSV					(1 << 26)	/* Insert VLAN Tag in the following word (in tdes2) */
		#define TX_ICO					(1 << 25)	/* Enable IP checksum generation offload */
		#define TX_UCO					(1 << 24)	/* Enable UDP checksum generation offload */
		#define TX_TCO					(1 << 23)	/* Enable TCP checksum generation offload */
		#define TX_LEN_MASK				(0xffff)	/* Tx Segment Data length */
		#define TX_LEN_OFFSET			(0)
#endif
	u32 vtag;
		#define TX_EPID_MASK			(0xffff)	/* VLAN Tag EPID */
		#define TX_EPID_OFFSET			(16)
		#define TX_PRI_MASK				(0x7)		/* VLNA Tag Priority */
		#define TX_PRI_OFFSET			(13)
		#define TX_CFI					(1 << 12)	/* VLAN Tag CFI (Canonical Format Indicator) */
		#define TX_VID_MASK				(0xfff)		/* VLAN Tag VID */
		#define TX_VID_OFFSET			(0)
	u32 reserve;	/* Tx pointer for external management usage */
} TxDesc;

typedef struct rxDesc_s	/* Rx Ring */
{
#ifdef CC_ETHERNET_6896
u32 ctrlLen;	/* Rx control and length */
	#define RX_COWN 				(1 << 31)	/* RX descriptor Own bit; 1: CPU own */
	#define RX_EOR					(1 << 30)	/* End of Rx descriptor ring */
	#define RX_FS					(1 << 29)	/* First Segment descriptor */
	#define RX_LS					(1 << 28)	/* Last Segment descriptor */
	#define RX_OSIZE				(1 << 25)	/* Rx packet is oversize */
	#define RX_CRCERR				(1 << 24)	/* Rx packet is CRC Error */
	#define RX_RMC					(1 << 23)	/* Rx packet DMAC is Reserved Multicast Address */
	#define RX_HHIT 				(1 << 22)	/* Rx packet DMAC is hit in hash table */
	#define RX_MYMAC				(1 << 21)	/* Rx packet DMAC is My_MAC */
	#define RX_VTAG 				(1 << 20)	/* VLAN Tagged int the following word */
	#define RX_PROT_MASK			(0x3)
	#define RX_PROT_OFFSET			(18)
		#define RX_PROT_IP			(0x0)		/* Protocol: IPV4 */
		#define RX_PROT_UDP 		(0x1)		/* Protocol: UDP */
		#define RX_PROT_TCP 		(0x2)		/* Protocol: TCP */
		#define RX_PROT_OTHERS		(0x3)		/* Protocol: Others */
	#define RX_IPF					(1 << 17)	/* IP checksum fail (meaningful when PROT is IPV4) */
	#define RX_L4F					(1 << 16)	/* Layer-4 checksum fail (meaningful when PROT is UDP or TCP) */
	#define RX_LEN_MASK 			(0xffff)	/* Segment Data length(FS=0) / Whole Packet Length(FS=1) */
	#define RX_LEN_OFFSET			(0)
u32 buffer; 	/* RX segment data pointer */

#else
	u32 buffer;		/* RX segment data pointer */
	u32 ctrlLen;	/* Rx control and length */
		#define RX_COWN					(1 << 31)	/* RX descriptor Own bit; 1: CPU own */
		#define RX_EOR					(1 << 30)	/* End of Rx descriptor ring */
		#define RX_FS					(1 << 29)	/* First Segment descriptor */
		#define RX_LS					(1 << 28)	/* Last Segment descriptor */
		#define RX_OSIZE				(1 << 25)	/* Rx packet is oversize */
		#define RX_CRCERR				(1 << 24)	/* Rx packet is CRC Error */
		#define RX_RMC					(1 << 23)	/* Rx packet DMAC is Reserved Multicast Address */
		#define RX_HHIT					(1 << 22)	/* Rx packet DMAC is hit in hash table */
		#define RX_MYMAC				(1 << 21)	/* Rx packet DMAC is My_MAC */
		#define RX_VTAG					(1 << 20)	/* VLAN Tagged int the following word */
		#define RX_PROT_MASK			(0x3)
		#define RX_PROT_OFFSET			(18)
			#define RX_PROT_IP			(0x0)		/* Protocol: IPV4 */
			#define RX_PROT_UDP			(0x1)		/* Protocol: UDP */
			#define RX_PROT_TCP			(0x2)		/* Protocol: TCP */
			#define RX_PROT_OTHERS		(0x3)		/* Protocol: Others */
		#define RX_IPF					(1 << 17)	/* IP checksum fail (meaningful when PROT is IPV4) */
		#define RX_L4F					(1 << 16)	/* Layer-4 checksum fail (meaningful when PROT is UDP or TCP) */
		#define RX_LEN_MASK				(0xffff)	/* Segment Data length(FS=0) / Whole Packet Length(FS=1) */
		#define RX_LEN_OFFSET			(0)
#endif
	u32 vtag;
		#define RX_EPID_MASK			(0xffff)	/* VLAN Tag EPID */
		#define RX_EPID_OFFSET			(16)
		#define RX_PRI_MASK				(0x7)		/* VLAN Tag Priority */
		#define RX_PRI_OFFSET			(13)
		#define RX_CFI					(1 << 12)	/* VLAN Tag CFI (Canonical Format Indicator) */
		#define RX_VID_MASK				(0xfff)		/* VLAN Tag VID */
		#define RX_VID_OFFSET			(0)
	u32 reserve;	/* Rx pointer for external management usage */
} RxDesc;

typedef struct star_dev_s
{
	void __iomem *base;               /* Base register of Star Ethernet */
	void __iomem *pinmuxBase;         /* Base register of PinMux */
    void __iomem *pdwncBase;          /* Base register of PDWNC */
    void __iomem *ethPdwncBase;       /* Base register of Eternet PDWNC */
    void __iomem *ethChksumBase;      /* Base register of Ethernet Checksum */
    void __iomem *ethIPLL1Base;       /* Base register of Ethernet IPLL 1 */
    void __iomem *ethIPLL2Base;       /* Base register of Ethernet IPLL 2 */
    void __iomem *bspBase;            /* Base register of BSP */
	TxDesc *txdesc;         /* Base Address of Tx descriptor Ring */
	RxDesc *rxdesc;         /* Base Address of Rx descriptor Ring */
	u32 txRingSize;
	u32 rxRingSize;
	u32 txHead;             /* Head of Tx descriptor (least sent) */
	u32 txTail;             /* Tail of Tx descriptor (least be free) */
	u32 rxHead;             /* Head of Rx descriptor (least sent) */
	u32 rxTail;             /* Tail of Rx descriptor (least be free) */
	u32 txNum;
	u32 rxNum;
    u32 linkUp;             /*link status */
    void *starPrv;
	struct net_device_stats stats;
} StarDev;

#define SIOC_WRITE_MAC_ADDR_CMD   (SIOCDEVPRIVATE+1)
#define SIOC_ETH_MAC_CMD          (SIOCDEVPRIVATE+2) //see ioctl_eth_mac_cmd
#define SIOC_THROUGHPUT_GET_CMD   (SIOCDEVPRIVATE+7)
#define SIOC_MDIO_CMD   (SIOCDEVPRIVATE+8)//see ioctl_mdio_cmd
#define SIOC_PHY_CTRL_CMD         (SIOCDEVPRIVATE+10)//see ioctl_mdio_cmd
#if defined(ETH_SUPPORT_WOL)
#define SIOC_SET_WOP_CMD          (SIOCDEVPRIVATE+11)
#define SIOC_CLR_WOP_CMD          (SIOCDEVPRIVATE+12)
#endif

struct ioctl_mdio_cmd
{
    u32 wr_cmd; /* 1: write, 0: read */
    u32 reg;    /* reg addr */
    u16 val;    /* value to write, or value been read */
    u16 rsvd;
};

//For SIOC_ETH_MAC_CMD
typedef enum
{
 ETH_MAC_REG_READ =0,//Paramter: reg_par
 ETH_MAC_REG_WRITE,	//Paramter: reg_par
 ETH_MAC_TX_DESCRIPTOR_READ, //Paramter: tx_descriptor_par		
 ETH_MAC_RX_DESCRIPTOR_READ, //Paramter: rx_descriptor_par	
 ETH_MAC_UP_DOWN_QUEUE ,//Paramter: up_down_queue_par
	
}ETH_MAC_CMD_TYPE;	
//For SIOC_ETH_MAC_CMD
struct reg_par
{
    u32 addr;    /* reg addr */
    u32 val;    /* value to write, or value been read */
};
    
struct tx_descriptor_par
{
    u32 index;    /* tx descriptor index*/
    TxDesc *prTxDesc;    
};    

struct rx_descriptor_par
{
    u32 index;    /* tx descriptor index*/
    RxDesc *prRxDesc;    
};  


struct up_down_queue_par
{
    u32 up;    
    
};
    
struct ioctl_eth_mac_cmd
{
	
    u32 eth_cmd; /* see  ETH_MAC_CMD_TYPE*/
  
    union {
	struct reg_par reg;
    struct tx_descriptor_par tx_desc;
    struct rx_descriptor_par rx_desc;
    struct up_down_queue_par up_down_queue;
		
	} ifr_ifru;
};

struct ioctl_phy_ctrl_cmd
{
    u32 wr_cmd; /* 1: write, 0: read */
    u32 Prm;    /* prm */
    u32 val;    /* value to write, or value been read */
    u32 rsvd;
};

struct ioctl_wop_para_cmd
{
    u8 protocol_type;
    u8 port_count;
    u32 *port_array;
};

enum
{
 ETH_PHY_SET =1,//Paramter:
 ETH_PHY_GET=0,	//Paramter:
 
 ETH_PHY_DACAMP_CTRL=0, //:
 ETH_PHY_10MAMP_CTRL=1, //:
 ETH_PHY_IN_BIAS_CTRL=2, //:
 ETH_PHY_OUT_BIAS_CTRL=3, //:
 ETH_PHY_FEDBAK_CAP_CTRL=4, //:
 ETH_PHY_SLEW_RATE_CTRL=5, //:
 ETH_PHY_EYE_OPEN_CTRL=6, //:
 ETH_PHY_BW_50P_CTRL=7
 
};	

int StarDevInit(StarDev *dev, void __iomem *base);
void StarResetEthernet(StarDev *dev);
int StarPhyMode(StarDev *dev);
int StarHwInit(StarDev *dev);

u16 StarMdcMdioRead(StarDev *dev, u32 phyAddr, u32 phyReg);
void StarMdcMdioWrite(StarDev *dev, u32 phyAddr, u32 phyReg, u16 value);


int StarDmaInit(StarDev *dev, uintptr_t desc_virAd, dma_addr_t desc_dmaAd);
int StarDmaTxSet(StarDev *dev, u32 buffer, u32 length, uintptr_t extBuf);
int StarDmaTxGet(StarDev *dev, u32 *buffer, u32 *ctrlLen, uintptr_t *extBuf);
int StarDmaRxSet(StarDev *dev, u32 buffer, u32 length, uintptr_t extBuf);
int StarDmaRxGet(StarDev *dev, u32 *buffer, u32 *ctrlLen, uintptr_t *extBuf);
void StarDmaTxStop(StarDev *dev);
void StarDmaRxStop(StarDev *dev);

int StarMacInit(StarDev *dev, u8 macAddr[6]);
void StarGetHwMacAddr(StarDev *dev, u8 macAddr[6]);
int StarMibInit(StarDev *dev);
int StarPhyCtrlInit(StarDev *dev, u32 enable, u32 phyAddr);
void StarSetHashBit(StarDev *dev, u32 addr, u32 value);
int StarDetectPhyId(StarDev *dev);

void StarLinkStatusChange(StarDev *dev);
void StarNICPdSet(StarDev *dev, u32 val);
void StarPhyReset(StarDev *dev);
int i4WriteEthMacAddr(struct net_device *dev, struct sockaddr *addr);
void vStarSetSlewRate(StarDev *dev , u32 uRate);
void vStarGetSlewRate(StarDev *dev , u32 * uRate);
void vStarSetOutputBias(StarDev *dev , u32 uOBiasLevel);
void vStarGetOutputBias(StarDev *dev , u32 * uOBiasLevel);
void vStarSetInputBias(StarDev *dev , u32 uInBiasLevel);
void vStarGetInputBias(StarDev *dev , u32 * uInBiasLevel);
int StarInitPinMux(StarDev *dev);
void vSetDACAmp(StarDev *dev , u32 uLevel);
void vGetDACAmp(StarDev *dev , u32 * uLevel);
void vSet100SlewRate(StarDev *dev , u32 uLevel);
void vGet100SlewRate(StarDev *dev , u32 * uLevel);
void vStarSet10MAmp(StarDev *dev , u32 uLevel);
void vStarGet10MAmp(StarDev *dev , u32* uLevel);
void vStarSet50PercentBW(StarDev *dev , u32 uEnable);
void vStarGet50PercentBW(StarDev *dev , u32* uEnable);
void vStarSetFeedBackCap(StarDev *dev , u32 uLevel);
void vStarGetFeedBackCap(StarDev *dev , u32* uLevel);

/* Star Ethernet Configuration*/
/* ======================================================================================= */
#define StarPhyDisableMdc(dev)			StarSetBit(STAR_PHY_CTRL1((dev)->base), STAR_PHY_CTRL1_MIDIS)
#define StarPhyEnableMdc(dev)			StarClearBit(STAR_PHY_CTRL1((dev)->base), STAR_PHY_CTRL1_MIDIS)
#define StarPhyDisableAtPoll(dev)		StarSetBit(STAR_PHY_CTRL1((dev)->base), STAR_PHY_CTRL1_APDIS)
#define StarPhyEnableAtPoll(dev)		StarClearBit(STAR_PHY_CTRL1((dev)->base), STAR_PHY_CTRL1_APDIS)

#define StarDisablePortCHGInt(dev)		StarSetBit(STAR_INT_MASK((dev)->base), STAR_INT_STA_PORTCHANGE) 
#define StarEnablePortCHGInt(dev)		StarClearBit(STAR_INT_MASK((dev)->base), STAR_INT_STA_PORTCHANGE) 
#define StarIntrDisable(dev)			StarSetReg(STAR_INT_MASK((dev)->base), 0x1ff)
#define StarIntrEnable(dev)				StarSetReg(STAR_INT_MASK((dev)->base), 0)
#define StarIntrClear(dev, intrStatus)	do{StarSetReg(STAR_INT_STA((dev)->base), intrStatus);}while(0)
#define StarIntrStatus(dev)				StarGetReg(STAR_INT_STA((dev)->base))
#define StarIntrRxEnable(dev)			do{StarClearBit(STAR_INT_MASK((dev)->base), STAR_INT_STA_RXC);}while(0)
#define StarIntrRxDisable(dev)			do{StarSetBit(STAR_INT_MASK((dev)->base), STAR_INT_STA_RXC);}while(0)
#define StarIntrMagicPktDisable(dev)			do{StarSetBit(STAR_INT_MASK((dev)->base), STAR_INT_STA_MAGICPKT);}while(0)
#ifdef CC_ETHERNET_6896
  #define RX_RESUME				((UINT32)0x01 << 2)
  #define RX_STOP  				((UINT32)0x01 << 1)
  #define RX_START  				((UINT32)0x01 << 0)

#define vDmaTxStartAndResetTXDesc(dev)   StarSetBit(STAR_TX_DMA_CTRL((dev)->base), TX_START) 
#define vDmaRxStartAndResetRXDesc(dev)   StarSetBit(STAR_RX_DMA_CTRL((dev)->base), RX_START) 
#define StarDmaTxEnable(dev)			StarSetBit(STAR_TX_DMA_CTRL((dev)->base), TX_RESUME)
#define StarDmaTxDisable(dev)			StarSetBit(STAR_TX_DMA_CTRL((dev)->base), TX_STOP)
#define StarDmaRxEnable(dev)			StarSetBit(STAR_RX_DMA_CTRL((dev)->base), RX_RESUME)
#define StarDmaRxDisable(dev)			StarSetBit(STAR_RX_DMA_CTRL((dev)->base), RX_STOP)



#else
#define StarDmaTxEnable(dev)			StarSetReg(STAR_TX_DMA_CTRL((dev)->base), 1)
#define StarDmaTxDisable(dev)			StarSetReg(STAR_TX_DMA_CTRL((dev)->base), 0)
#define StarDmaRxEnable(dev)			StarSetReg(STAR_RX_DMA_CTRL((dev)->base), 1)
#define StarDmaRxDisable(dev)			StarSetReg(STAR_RX_DMA_CTRL((dev)->base), 0)
#endif
#define StarDmaRx2BtOffsetEnable(dev)	StarClearBit(STAR_DMA_CFG((dev)->base), STAR_DMA_CFG_RX2BOFSTDIS)
#define StarDmaRx2BtOffsetDisable(dev)	StarSetBit(STAR_DMA_CFG((dev)->base), STAR_DMA_CFG_RX2BOFSTDIS)

#define StarDmaTxResume(dev)			StarDmaTxEnable(dev)
#define StarDmaRxResume(dev)			StarDmaRxEnable(dev)

#define StarResetHashTable(dev)			StarSetBit(STAR_TEST1((dev)->base), STAR_TEST1_RST_HASH_BIST)

#define StarDmaRxValid(ctrlLen)			(((ctrlLen & RX_FS) != 0) && ((ctrlLen & RX_LS) != 0) && ((ctrlLen & RX_CRCERR) == 0) && ((ctrlLen & RX_OSIZE) == 0))
#define StarDmaRxCrcErr(ctrlLen)		((ctrlLen & RX_CRCERR) ? 1 : 0)
#define StarDmaRxOverSize(ctrlLen)		((ctrlLen & RX_OSIZE) ? 1 : 0)
#define StarDmaRxProtocolIP(ctrlLen)	(((ctrlLen >> RX_PROT_OFFSET) & RX_PROT_MASK) != RX_PROT_OTHERS)
#define StarDmaRxProtocolTCP(ctrlLen)	(((ctrlLen >> RX_PROT_OFFSET) & RX_PROT_MASK) == RX_PROT_TCP)
#define StarDmaRxProtocolUDP(ctrlLen)	(((ctrlLen >> RX_PROT_OFFSET) & RX_PROT_MASK) == RX_PROT_UDP)
#define StarDmaRxIPCksumErr(ctrlLen)	(ctrlLen & RX_IPF)
#define StarDmaRxL4CksumErr(ctrlLen)	(ctrlLen & RX_L4F)

#define StarDmaRxLength(ctrlLen)		((ctrlLen >> RX_LEN_OFFSET) & RX_LEN_MASK)
#define StarDmaTxLength(ctrlLen)		((ctrlLen >> TX_LEN_OFFSET) & TX_LEN_MASK)

#define StarArlPromiscEnable(dev)		StarSetBit(STAR_ARL_CFG((dev)->base), STAR_ARL_CFG_MISCMODE)

#define DescTxDma(desc)					((((desc)->ctrlLen) & TX_COWN) ? 0 : 1)
#define DescRxDma(desc)					((((desc)->ctrlLen) & RX_COWN) ? 0 : 1)
#define DescTxLast(desc)				((((desc)->ctrlLen) & TX_EOR) ? 1 : 0)
#define DescRxLast(desc)				((((desc)->ctrlLen) & RX_EOR) ? 1 : 0)
#define DescTxEmpty(desc)				(((desc)->buffer == 0) && \
                                         (((desc)->ctrlLen & ~TX_EOR) == TX_COWN) && \
                                         ((desc)->vtag == 0) && ((desc)->reserve == 0))
                                         
#define DescRxEmpty(desc)				(((desc)->buffer == 0) && \
                                         (((desc)->ctrlLen & ~RX_EOR) == RX_COWN) && \
                                         ((desc)->vtag == 0) && ((desc)->reserve == 0))

/* -- ASIC mode or not -- */
#define StarIsASICMode(dev)				((StarGetReg(STAR_DUMMY((dev)->base)) & STAR_DUMMY_FPGA_MODE) ? 0 : 1)



/**
 *	@name Ethernet Mode Enumeration
 *	@brief There are five modes in Ethernet configuration
 *	      1. Internal PHY
 *	      2. Internal Switch
 *	      3. External MII
 *	      4. External RGMII
 *	      5. External RvMII
 */
enum 
{
	INT_PHY_MODE = 0,
	INT_SWITCH_MODE = 1,
	EXT_MII_MODE = 2,
	EXT_RGMII_MODE = 3,
	EXT_RVMII_MODE = 4,
};

#ifndef UNUSED
#define UNUSED(x)   (void)x
#endif

#ifndef STAR_POLLING_TIMEOUT
#define STAR_TIMEOUT_COUNT      3000
#define STAR_POLLING_TIMEOUT(cond) \
    do {    \
        UINT32 u4Timeout = STAR_TIMEOUT_COUNT;    \
        while (!cond) { \
            if (--u4Timeout == 0)   \
                break;  \
        }   \
        if (u4Timeout == 0) { \
            STAR_MSG(STAR_ERR, "polling timeout in %s\n",  __FUNCTION__); \
        } \
    } while (0)
#endif

#endif /* _STAR_DEV_H_ */
