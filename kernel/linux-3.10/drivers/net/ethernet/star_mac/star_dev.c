/**
 * @brief Star Ethernet Device dependent functions
 * @author mtk02196
 *
 */
#include "star.h"

#ifdef CONFIG_ARM64
uintptr_t tx_skb_reserve[TX_DESC_NUM];
uintptr_t rx_skb_reserve[RX_DESC_NUM];
#endif

static u32 patch_rtl8201el = 0;
static int eth_phy_id = 32;
extern u32 fgOpenStBeforeSuspend;
/* =========================================================
   Common functions
   ========================================================= */
int StarDevInit(StarDev *dev, void __iomem *base)
{
	memset(dev, 0, sizeof(StarDev));

	dev->base = base;

	return 0;
}

void StarResetEthernet(StarDev *dev)
{
	void __iomem *base = dev->base;

	/* Power down NIC  to prevent PHY crash */
	{
#define MAX_WAIT_TIME		5000000
		unsigned int waitLoop;
	
		StarSetBit(STAR_MAC_CFG(base), STAR_MAC_CFG_NICPD);
        /* Wait for MDC/MDIO Done bit is asserted */
		for (waitLoop = 0; waitLoop < MAX_WAIT_TIME; waitLoop ++)
        {
            if (StarGetReg(STAR_DUMMY(base)) & STAR_DUMMY_MDCMDIODONE)
                break;
		}
		if (waitLoop >= MAX_WAIT_TIME)
		{
			STAR_MSG(STAR_ERR, "Star Ethernet Reset Procedure - NIC Power Down doesn't ready.\n\r");
		}
        /* Wait for the NIC_PD_READY to complete */
		for (waitLoop = 0; waitLoop < MAX_WAIT_TIME; waitLoop ++)
        {
            if (StarGetReg(STAR_MAC_CFG(base)) & STAR_MAC_CFG_NICPDRDY)
                break;
        }
		if (waitLoop >= MAX_WAIT_TIME)
		{
			STAR_MSG(STAR_ERR, "Star Ethernet Reset Procedure - NIC Power Down doesn't ready.\n\r");
		}
#undef MAX_WAIT_TIME
	}

}


int StarPhyMode(StarDev *dev)
{
#ifdef INTERNAL_PHY
    return INT_PHY_MODE;
#else
    return EXT_MII_MODE;
#endif
}


/* =========================================================
   MDC MDIO functions
   ========================================================= */
u16 StarMdcMdioRead(StarDev *dev, u32 phyAddr, u32 phyReg)
{
	void __iomem *base = dev->base;
	u32 phyCtl;
	u16 data;

	StarSetReg(STAR_PHY_CTRL0(base), STAR_PHY_CTRL0_RWOK); /* Clear previous read/write OK status (write 1 clear) */
	phyCtl = (  ((phyAddr & STAR_PHY_CTRL0_PA_MASK) << STAR_PHY_CTRL0_PA_OFFSET) |
	            ((phyReg & STAR_PHY_CTRL0_PREG_MASK) << STAR_PHY_CTRL0_PREG_OFFSET) |
	            STAR_PHY_CTRL0_RDCMD /* Read Command */
	          );
	StarSetReg(STAR_PHY_CTRL0(base), phyCtl);
	
    STAR_POLLING_TIMEOUT(StarIsSetBit(STAR_PHY_CTRL0(base), STAR_PHY_CTRL0_RWOK));
//	do {} while (!StarIsSetBit(STAR_PHY_CTRL0(base), STAR_PHY_CTRL0_RWOK)); /* Wait for read/write ok bit is asserted */
	
	data = (u16)StarGetBitMask(STAR_PHY_CTRL0(base), STAR_PHY_CTRL0_RWDATA_MASK, STAR_PHY_CTRL0_RWDATA_OFFSET);

	return data;
}

void StarMdcMdioWrite(StarDev *dev, u32 phyAddr, u32 phyReg, u16 value)
{
	void __iomem *base = dev->base;
	u32 phyCtl;

	StarSetReg(STAR_PHY_CTRL0(base), STAR_PHY_CTRL0_RWOK); /* Clear previous read/write OK status (write 1 clear) */
	phyCtl = ((value & STAR_PHY_CTRL0_RWDATA_MASK) << STAR_PHY_CTRL0_RWDATA_OFFSET) |
	         ((phyAddr & STAR_PHY_CTRL0_PA_MASK) << STAR_PHY_CTRL0_PA_OFFSET) |
	         ((phyReg & STAR_PHY_CTRL0_PREG_MASK) << STAR_PHY_CTRL0_PREG_OFFSET) |
	         STAR_PHY_CTRL0_WTCMD; /* Write Command */
	         
	StarSetReg(STAR_PHY_CTRL0(base), phyCtl);
	STAR_POLLING_TIMEOUT(StarIsSetBit(STAR_PHY_CTRL0(base), STAR_PHY_CTRL0_RWOK));
//	do {} while (!StarIsSetBit(STAR_PHY_CTRL0(base), STAR_PHY_CTRL0_RWOK)); /* Wait for read/write ok bit is asserted */
}

#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5891)
int i4MacPhyZCALInit(StarDev *dev)
{
	int i4Ret = 0;
	StarPrivate *starPrv = dev->starPrv;
 
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1f, 0x0002); //misc page
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x0d, 0x1016);
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1f, 0x00);

	UNUSED(i4Ret);
	return(i4Ret);
}

int i4MacPhyZCALRemapTable(StarDev *dev, u8 value)
{
	int i4Ret = 0;
	u16 temp;
	StarPrivate *starPrv = dev->starPrv;
    
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1f, 0x2a30);
    
	temp = StarMdcMdioRead(dev, starPrv->phy_addr, 0x14);
    
	temp |= (0x1 << 12);
	temp &= ~(0x0f00);    
	temp |= value << 8;
	temp &= ~(0xff); 
    
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x14, temp);
    
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1f, 0x00);
    
	UNUSED(i4Ret);
	return(i4Ret);
}
#endif


/* =========================================================
   DMA related functions
   ========================================================= */
static void DescTxInit(TxDesc *txDesc, u32 isEOR)
{
	txDesc->buffer = 0;
	txDesc->ctrlLen = TX_COWN | (isEOR ? TX_EOR : 0);
	txDesc->vtag = 0;
	txDesc->reserve = 0;
}

static void DescRxInit(RxDesc *rxDesc, u32 isEOR)
{
	rxDesc->buffer = 0;
	rxDesc->ctrlLen = RX_COWN | (isEOR ? RX_EOR : 0);
	rxDesc->vtag = 0;
	rxDesc->reserve = 0;
}

static void DescTxTake(TxDesc *txDesc)     /* Take ownership */ 
{
	if (DescTxDma(txDesc))
	{
		txDesc->ctrlLen |= TX_COWN;       /* Set CPU own */
	}
}

static void DescRxTake(RxDesc *rxDesc)     /* Take ownership */ 
{
	if (DescRxDma(rxDesc))
	{
		rxDesc->ctrlLen |= RX_COWN;       /* Set CPU own */
	}
}

int StarDmaInit(StarDev *dev, uintptr_t desc_virAd, dma_addr_t desc_dmaAd)
{
	int i;
	void __iomem *base = dev->base;

	dev->txRingSize = TX_DESC_NUM;
	dev->rxRingSize = RX_DESC_NUM;

	dev->txdesc = (TxDesc *)desc_virAd;
	dev->rxdesc = (RxDesc *)dev->txdesc + dev->txRingSize;

	for (i=0; i<dev->txRingSize; i++) { DescTxInit(dev->txdesc + i, i==dev->txRingSize-1); }
	for (i=0; i<dev->rxRingSize; i++) { DescRxInit(dev->rxdesc + i, i==dev->rxRingSize-1); }

	dev->txHead = 0;
	dev->txTail = 0;
	dev->rxHead = 0;
	dev->rxTail = 0;
	dev->txNum = 0;
	dev->rxNum = 0;

	/* TODO: Reset Procedure */
	/* OOXX.... */

#ifdef CC_ETHERNET_6896
#else
	/* Disable Tx/Rx DMA */
	StarDmaTxDisable(dev);
	StarDmaRxDisable(dev);
#endif  
        
	/* Set Tx/Rx descriptor address */
	StarSetReg(STAR_TX_BASE_ADDR(base), (u32)desc_dmaAd);
	StarSetReg(STAR_TX_DPTR(base), (u32)desc_dmaAd);
	StarSetReg(STAR_RX_BASE_ADDR(base), (u32)desc_dmaAd + sizeof(TxDesc) * dev->txRingSize);
	StarSetReg(STAR_RX_DPTR(base), (u32)desc_dmaAd + sizeof(TxDesc) * dev->txRingSize);
	/* Enable Rx 2 byte offset (FIXME) */
	//StarDmaRx2BtOffsetEnable(dev);
	/* Init DMA_CFG, Note: RX_OFFSET_2B_DIS is set to 0 */
#ifdef CC_ETHERNET_6896
    vDmaTxStartAndResetTXDesc(dev);
    vDmaRxStartAndResetRXDesc(dev);
#else
    StarSetReg(STAR_DMA_CFG(base),
                /* STAR_DMA_CFG_RX2BOFSTDIS | */
                STAR_DMA_CFG_RX_INTR_WDLE | 
                STAR_DMA_CFG_TX_INTR_WDLE );
#endif
                
	StarIntrDisable(dev);

	return 0;
}

int StarDmaTxSet(StarDev *dev, u32 buffer, u32 length, uintptr_t extBuf)
{
	int descIdx = dev->txHead;
	TxDesc *txDesc = dev->txdesc + descIdx;

	/* Error checking */
	if (dev->txNum == dev->txRingSize) goto err;
	if (!DescTxEmpty(txDesc)) goto err;          /* descriptor is not empty - cannot set */

	txDesc->buffer = buffer;
	txDesc->ctrlLen |= ((((length < 60)?60:length) & TX_LEN_MASK) << TX_LEN_OFFSET) | TX_FS | TX_LS | TX_INT/*Tx Interrupt Enable*/;
#ifdef CONFIG_ARM64
	tx_skb_reserve[descIdx] = extBuf;
#else
	txDesc->reserve = extBuf;
#endif
	wmb();
	txDesc->ctrlLen &= ~TX_COWN;     /* Set HW own */

	dev->txNum++;
	dev->txHead = DescTxLast(txDesc) ? 0 : descIdx + 1;
	
	return descIdx;
err:
	return -1;
}

int StarDmaTxGet(StarDev *dev, u32 *buffer, u32 *ctrlLen, uintptr_t *extBuf)
{
	int descIdx = dev->txTail;
	TxDesc *txDesc = dev->txdesc + descIdx;

	/* Error checking */
	if (dev->txNum == 0) goto err;             /* No buffer can be got */
	if (DescTxDma(txDesc)) goto err;          /* descriptor is owned by DMA - cannot get */
	if (DescTxEmpty(txDesc)) goto err;        /* descriptor is empty - cannot get */

	if (buffer != 0) *buffer = txDesc->buffer;
	if (ctrlLen != 0) *ctrlLen = txDesc->ctrlLen;
#ifdef CONFIG_ARM64
	if (extBuf != 0) *extBuf = tx_skb_reserve[descIdx];
#else
	if (extBuf != 0) *extBuf = txDesc->reserve;
#endif
	rmb();

	DescTxInit(txDesc, DescTxLast(txDesc));
	dev->txNum--;
	dev->txTail = DescTxLast(txDesc) ? 0 : descIdx + 1;

	return descIdx;
err:
	return -1;
}

int StarDmaRxSet(StarDev *dev, u32 buffer, u32 length, uintptr_t extBuf)
{
	int descIdx = dev->rxHead;
	RxDesc *rxDesc = dev->rxdesc + descIdx;

	/* Error checking */
	if (dev->rxNum == dev->rxRingSize) goto err;
	if (!DescRxEmpty(rxDesc)) goto err;		/* descriptor is not empty - cannot set */

	rxDesc->buffer = buffer;
	rxDesc->ctrlLen |= ((length & RX_LEN_MASK) << RX_LEN_OFFSET);
#ifdef CONFIG_ARM64
	rx_skb_reserve[descIdx] = extBuf;
#else
	rxDesc->reserve = extBuf;
#endif
	wmb();
	rxDesc->ctrlLen &= ~RX_COWN;       /* Set HW own */

	dev->rxNum++;
	dev->rxHead = DescRxLast(rxDesc) ? 0 : descIdx + 1;

	return descIdx;
err:
	return -1;
}

int StarDmaRxGet(StarDev *dev, u32 *buffer, u32 *ctrlLen, uintptr_t *extBuf)
{
	int descIdx = dev->rxTail;
	RxDesc *rxDesc = dev->rxdesc + descIdx;

	/* Error checking */
	if (dev->rxNum == 0) goto err;             /* No buffer can be got */
	if (DescRxDma(rxDesc)) goto err;          /* descriptor is owned by DMA - cannot get */
	if (DescRxEmpty(rxDesc)) goto err;        /* descriptor is empty - cannot get */

	if (buffer != 0) *buffer = rxDesc->buffer;
	if (ctrlLen != 0) *ctrlLen = rxDesc->ctrlLen;
#ifdef CONFIG_ARM64
	if (extBuf != 0) *extBuf = rx_skb_reserve[descIdx];
#else
	if (extBuf != 0) *extBuf = rxDesc->reserve;
#endif
	rmb();

	DescRxInit(rxDesc, DescRxLast(rxDesc));
	dev->rxNum--;
	dev->rxTail = DescRxLast(rxDesc) ? 0 : descIdx + 1;

	return descIdx;
err:
	return -1;
}

void StarDmaTxStop(StarDev *dev)
{
	int i;	
	StarDmaTxDisable(dev);
	for (i=0; i<dev->txRingSize; i++) DescTxTake(dev->txdesc + i);
}

void StarDmaRxStop(StarDev *dev)
{
	int i;	
	StarDmaRxDisable(dev);
	for (i=0; i<dev->rxRingSize; i++) DescRxTake(dev->rxdesc + i);
}


/* =========================================================
   MAC related functions
   ========================================================= */
int StarMacInit(StarDev *dev, u8 macAddr[6])
{
	void __iomem *base = dev->base;
	#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5891)
		u32 u4Data;
		u8 index, temp = 0x00;
		u8 mapTable[16][2] = {{0x01, 0x08}, {0x02, 0x07}, {0x03, 0x06}, {0x05, 0x05},
			{0x06, 0x04}, {0x07, 0x03}, {0x09, 0x02}, {0x0b, 0x01}, {0x0d, 0x00}, {0x21, 0x09},
			{0x23, 0x0a}, {0x24, 0x0b}, {0x26, 0x0c}, {0x27, 0x0d}, {0x28, 0x0e}, {0x2a, 0x0f}};
	#endif
   
#if 1
	#ifdef CC_ETHERNET_6896
		
	#else
        /* MII mode */
        //StarSetReg(STAR_EXT_CFG(dev->ethPdwncBase), PDWNC_MAC_EXT_INIT); //0x73FF0000 Little endian, Kenny temply mark for MT5365
        /* Reset DMA
           Note: Resetting DMA is important when the tx/rx descriptors 
                 are not set by first time. If the descriptor address 
                 registers have ever been set before, you will need this
                 reset. 
                 Reset MRST, NRST, HRST by writing '0' then '1' */
        StarSetReg(STAR_EXT_CFG(dev->base), MAC_EXT_INIT & (~(HRST|MRST|NRST)));//for mt5365, bit18 must be 1
        StarSetReg(STAR_EXT_CFG(dev->base), MAC_EXT_INIT); //0x7FFF0000 Little endian
       
        #ifdef USE_MAC_RMII//for RMII mode,  def CONFIG_MT53_FPGA
        StarSetBit(STAR_EXT_CFG(dev->base),(RMII_MODE|TEST_CLK));
        StarClearBit(STAR_EXT_CFG(dev->base), RMII_CLK_SEL);  
        
        #ifndef CONFIG_MT53_FPGA
        StarClearBit(STAR_EXT_CFG(dev->base), RMII_CLK_INV); //2010/3/19 for MT5365 QFP Board 
        #endif
        
        StarClearBit(STAR_EXT_CFG(dev->base),(MII_INTER_COL|COL_SEL_EXTERNAL));//enable RMII internal Col
        
        
        #else //Mii
        
        StarSetBit(STAR_EXT_CFG(dev->base), COL_SEL_EXTERNAL);//enable MII Col mode        
        StarClearBit(STAR_EXT_CFG(dev->base), MII_INTER_COL);//enable RMII external Col        
        #endif
        
	#endif                
#else
        /* RMII mode */
        StarSetReg(STAR_EXT_CFG(dev->ethPdwncBase), PDWNC_MAC_EXT_INIT_RMII);
        StarSetReg(STAR_EXT_CFG(dev->base), MAC_EXT_INIT_RMII);
#endif


#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5891)
	i4MacPhyZCALInit(dev);
    
	u4Data = StarGetReg(RW_ARB_MON2(base));
	temp = (u4Data & 0x80) >> 2;
	temp |= (u4Data & 0x1f);

	for (index = 0; index < 16; index++)
	{
		if (temp == mapTable[index][0])
		{
			break;
		}
	}

	if (index == 16)
	{  
		i4MacPhyZCALRemapTable(dev, 0x08);
	}
	else
	{
		i4MacPhyZCALRemapTable(dev, mapTable[index][1]);
	}
        
#endif


	/* Set Mac Address */
	StarSetReg(STAR_My_MAC_H(base), macAddr[0]<<8 | macAddr[1]<<0);
	StarSetReg(STAR_My_MAC_L(base), macAddr[2]<<24 | macAddr[3]<<16 | macAddr[4]<<8 | macAddr[5]<<0);

	/* Set Mac Configuration */
#ifdef CHECKSUM_OFFLOAD
	StarSetReg( STAR_MAC_CFG(base),
				//STAR_MAC_CFG_TXCKSEN |
				STAR_MAC_CFG_RXCKSEN |
	            STAR_MAC_CFG_CRCSTRIP |
	            STAR_MAC_CFG_MAXLEN_1522 |
	            (0x1f & STAR_MAC_CFG_IPG_MASK) << STAR_MAC_CFG_IPG_OFFSET /* 12 byte IPG */
	          );
#else
	StarSetReg( STAR_MAC_CFG(base),
	            STAR_MAC_CFG_CRCSTRIP |
	            STAR_MAC_CFG_MAXLEN_1522 |
	            (0x1f & STAR_MAC_CFG_IPG_MASK) << STAR_MAC_CFG_IPG_OFFSET /* 12 byte IPG */
	          );
#endif
    /* Init Flow Control register */
    StarSetReg(STAR_FC_CFG(base),
               STAR_FC_CFG_SEND_PAUSE_TH_DEF |
               STAR_FC_CFG_UCPAUSEDIS |
               STAR_FC_CFG_BPEN
              );
    /* Init SEND_PAUSE_RLS */
    StarSetReg(STAR_EXTEND_CFG(base),
               STAR_EXTEND_CFG_SEND_PAUSE_RLS_DEF);
               
    

	/* Init MIB counter (reset to 0) */
	StarMibInit(dev);
	/* Enable Hash Table BIST */
	StarSetBit(STAR_HASH_CTRL(base), STAR_HASH_CTRL_HASHEN);
	/* Reset Hash Table (All reset to 0) */
	StarResetHashTable(dev);
	
	return 0;
}

void StarGetHwMacAddr(StarDev *dev, u8 macAddr[6])
{
    void __iomem *base = dev->base;
    
    if((0==StarGetReg(dev->base+0x14)) && (0==StarGetReg(dev->base+0x18)))
    {
        STAR_MSG(STAR_ERR, "%s, HW Mac Address Error!!! All Zero!!!\n", __FUNCTION__);
    }
    
    macAddr[0] = (StarGetReg(STAR_My_MAC_H(base)) >>8) & 0xff;
    macAddr[1] = StarGetReg(STAR_My_MAC_H(base)) & 0xff;
    macAddr[2] = (StarGetReg(STAR_My_MAC_L(base)) >>24) & 0xff;
    macAddr[3] = (StarGetReg(STAR_My_MAC_L(base)) >>16) & 0xff;
    macAddr[4] = (StarGetReg(STAR_My_MAC_L(base)) >>8) & 0xff;
    macAddr[5] = StarGetReg(STAR_My_MAC_L(base)) & 0xff;
}
static void StarMibReset(StarDev *dev)
{
	void __iomem *base = dev->base;

	/* MIB counter is read clear */   
	StarGetReg(STAR_MIB_RXOKPKT(base));
	StarGetReg(STAR_MIB_RXOKBYTE(base));
	StarGetReg(STAR_MIB_RXRUNT(base));
	StarGetReg(STAR_MIB_RXOVERSIZE(base));
	StarGetReg(STAR_MIB_RXNOBUFDROP(base));
	StarGetReg(STAR_MIB_RXCRCERR(base));
	StarGetReg(STAR_MIB_RXARLDROP(base));
	StarGetReg(STAR_MIB_RXVLANDROP(base));
	StarGetReg(STAR_MIB_RXCKSERR(base));
	StarGetReg(STAR_MIB_RXPAUSE(base));
	StarGetReg(STAR_MIB_TXOKPKT(base));
	StarGetReg(STAR_MIB_TXOKBYTE(base));
	StarGetReg(STAR_MIB_TXPAUSECOL(base));
	/*	
    StarSetReg(STAR_MIB_RXOKPKT(base), 0);
    StarSetReg(STAR_MIB_RXOKBYTE(base), 0);
    StarSetReg(STAR_MIB_RXRUNT(base), 0);
    StarSetReg(STAR_MIB_RXOVERSIZE(base), 0);
    StarSetReg(STAR_MIB_RXNOBUFDROP(base), 0);
    StarSetReg(STAR_MIB_RXCRCERR(base), 0);
    StarSetReg(STAR_MIB_RXARLDROP(base), 0);
    StarSetReg(STAR_MIB_RXVLANDROP(base), 0);
    StarSetReg(STAR_MIB_RXCKSERR(base), 0);
    StarSetReg(STAR_MIB_RXPAUSE(base), 0);
    StarSetReg(STAR_MIB_TXOKPKT(base), 0);
    StarSetReg(STAR_MIB_TXOKBYTE(base), 0);
    StarSetReg(STAR_MIB_TXPAUSECOL(base), 0);
	*/
}

int StarMibInit(StarDev *dev)
{
	StarMibReset(dev);

	return 0;
}

int StarPhyCtrlInit(StarDev *dev, u32 enable, u32 phyAddr)
{
	void __iomem *base = dev->base;
	u32 data;

	data = STAR_PHY_CTRL1_FORCETXFC | 
	       STAR_PHY_CTRL1_FORCERXFC | 
	       STAR_PHY_CTRL1_FORCEFULL | 
	       STAR_PHY_CTRL1_FORCESPD_100M |
	       STAR_PHY_CTRL1_ANEN;

	if (enable) /* Enable PHY auto-polling */
	{
		StarSetReg( STAR_PHY_CTRL1(base),
		            data | STAR_PHY_CTRL1_APEN | ((phyAddr & STAR_PHY_CTRL1_PHYADDR_MASK) << STAR_PHY_CTRL1_PHYADDR_OFFSET)
		          );
	} else /* Disable PHY auto-polling */
	{
		StarSetReg( STAR_PHY_CTRL1(base),
		            data | STAR_PHY_CTRL1_APDIS
		          );
	}

	return 0;
}

void StarSetHashBit(StarDev *dev, u32 addr, u32 value)
{
	void __iomem *base = dev->base;
	u32 data;

    STAR_POLLING_TIMEOUT(StarIsSetBit(STAR_HASH_CTRL(base), STAR_HASH_CTRL_HTBISTDONE));
    STAR_POLLING_TIMEOUT(StarIsSetBit(STAR_HASH_CTRL(base), STAR_HASH_CTRL_HTBISTOK));
    STAR_POLLING_TIMEOUT(!StarIsSetBit(STAR_HASH_CTRL(base), STAR_HASH_CTRL_START));
#if 0
	do {} while (!StarIsSetBit(STAR_HASH_CTRL(base), STAR_HASH_CTRL_HTBISTDONE)); /* Wait for BIST Done */
	do {} while (!StarIsSetBit(STAR_HASH_CTRL(base), STAR_HASH_CTRL_HTBISTOK));   /* Wait for BIST OK */
	do {} while (StarIsSetBit(STAR_HASH_CTRL(base), STAR_HASH_CTRL_START));       /* Wait for Start Command clear */
#endif
	data = (STAR_HASH_CTRL_HASHEN |
			STAR_HASH_CTRL_ACCESSWT | STAR_HASH_CTRL_START |
			(value ? STAR_HASH_CTRL_HBITDATA : 0) |
			((addr & STAR_HASH_CTRL_HBITADDR_MASK) << STAR_HASH_CTRL_HBITADDR_OFFSET) );
	StarSetReg(STAR_HASH_CTRL(base), data);
	//StarSetBit(STAR_HASH_CTRL(base), STAR_HASH_CTRL_START);
	
    STAR_POLLING_TIMEOUT(!StarIsSetBit(STAR_HASH_CTRL(base), STAR_HASH_CTRL_START));
//	do {} while (StarIsSetBit(STAR_HASH_CTRL(base), STAR_HASH_CTRL_START));       /* Wait for Start Command clear */
}

int StarDetectPhyId(StarDev *dev)
{
	#define PHY_REG_IDENTFIR2	(3)      /* Reg3: PHY Identifier 2 */
	#define PHYID2_INTVITESSE	(0x0430) /* Internal PHY */
	#define PHYID2_RTL8201		(0x8201) /* RTL8201 */
	#define PHYID2_RTL8211		(0xC912) /* RTL8211 */
    #define PHYID2_RTL8306M     (0xC852) /* RTL8306M */
    #define PHYID2_IP101A       (0x0C50) /* IC+IP101A */
    #define PHYID2_SMSC7100     (0xC0B1) /*SMSC LAN7100 */
	#define PHYID2_SMSC8700     (0xC0C4) /*SMSC LAN8700 */
	#define PHYID2_RTL8201EL	(0xC815) /* RTL8201EL */
	
	int phyId;
	u16 phyIdentifier2;

	for (phyId = 0; phyId < 32; phyId ++)
	{
		phyIdentifier2 = StarMdcMdioRead(dev, phyId, PHY_REG_IDENTFIR2);

		if (phyIdentifier2 == PHYID2_INTVITESSE)
		{
			printk("Star Ethernet: Internal Vitesse PHY\n\r");
			break;
		} else if (phyIdentifier2 == PHYID2_RTL8201)
		{
			printk("Star Ethernet: RTL8201 PHY\n\r");
			break;
		} else if (phyIdentifier2 == PHYID2_RTL8211)
		{
			printk("Star Ethernet: RTL8211 PHY\n\r");
			break;
		}
        else if ((phyIdentifier2) == PHYID2_RTL8306M)
        {
            phyIdentifier2 = StarMdcMdioRead(dev, phyId, PHY_REG_IDENTFIR2-1);
            if (phyIdentifier2 == 0x001C)
            {
                u32 phy6reg22;
                printk("Star Ethernet: RTL8306M PHY\n\r");
                phyId = 6;
                phy6reg22 = StarMdcMdioRead(dev, phyId, 22);
                StarMdcMdioWrite(dev, phyId, 22, phy6reg22 | (1<<15));
                break;
            }
        }
        else if ((phyIdentifier2&0xfff0) == PHYID2_IP101A)
        {
            printk("Star Ethernet: IC+IP101A PHY\n\r");
            break;
        }
        else if (phyIdentifier2 == PHYID2_SMSC7100)
        {
            printk("Star Ethernet: SMSC7100 PHY\n\r");
            break;
        }
        else if (phyIdentifier2 == PHYID2_SMSC8700)
        {
            printk("Star Ethernet: SMSC8700 PHY\n\r");
            break;
        }
        else if (phyIdentifier2 == PHYID2_RTL8201EL)
        {
            printk("Star Ethernet: RTL8201EL PHY\n\r");
            StarMdcMdioWrite(dev, phyId, 25, 0x44F9);
            patch_rtl8201el = 1;
            break;
        }
	}

    /* If can't find phy id, try reading from PHY ID 2,
       and check the return value. If success, should return
       a value other than 0xffff.
     */
    if (phyId >= 32)
    {
        for (phyId = 0; phyId < 32; phyId ++)
        {
            phyIdentifier2 = StarMdcMdioRead(dev, phyId, PHY_REG_IDENTFIR2);
            if (phyIdentifier2 != 0xffff)
                break;
        }
    }
	return phyId;

}

#if 0
static int StarIsExtClkSrcRequired(StarDev *dev)
{
    u32 data;
    
    data = StarGetReg(dev->bspBase);
    if (data == 0x8550)
        return TRUE;
    else
        return FALSE;
}
#endif

#if (USE_EXT_CLK_SRC)
static int StarIntPhyClkExtSrc(StarDev *dev, int enable)
{
    volatile u16 data;
    StarPrivate *starPrv = dev->starPrv;
    
    if (enable)
    {
        StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1f, 0x2a30);
        
        data = StarMdcMdioRead(dev, starPrv->phy_addr, 0x0f);
        data |= (1<<8);
        StarMdcMdioWrite(dev, starPrv->phy_addr, 0x0f, data);
        
        data = StarMdcMdioRead(dev, starPrv->phy_addr, 0x01);
        data |= (1<<3);
        StarMdcMdioWrite(dev, starPrv->phy_addr, 0x01, data);

        StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1f, 0x0);
    }

    return 0;
}

#endif /* USE_EXT_CLK_SRC */

#ifdef INTERNAL_PHY
void vStarSetSlewRate(StarDev *dev , u32 uRate)
{
  StarPrivate *starPrv = dev->starPrv;
  if(uRate>0x06)
  {
	STAR_MSG(STAR_ERR, "setting fail ! value must be 0 ~ 6 \n");
	return;
  }

#ifdef CC_ETHERNET_6896
  StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x2a30);
  switch(uRate)
 {
   case 0:
   StarMdcMdioWrite(dev, starPrv->phy_addr, 0x15, 0x0F01);
   break;
   case 1:
   StarMdcMdioWrite(dev, starPrv->phy_addr, 0x15, 0x0F03);
   break;
   case 2:
   StarMdcMdioWrite(dev, starPrv->phy_addr, 0x15, 0x0F07);
   break;
   case 3:
   StarMdcMdioWrite(dev, starPrv->phy_addr, 0x15, 0x0F0f);
   break;
   case 4:
   StarMdcMdioWrite(dev, starPrv->phy_addr, 0x15, 0x0F1f);
   break;
   case 5:
   StarMdcMdioWrite(dev, starPrv->phy_addr, 0x15, 0x0F3f);
   break;
   case 6:
   StarMdcMdioWrite(dev, starPrv->phy_addr, 0x15, 0x0F7f);
   break;

   default:
   StarMdcMdioWrite(dev, starPrv->phy_addr, 0x15, 0x0F0f);
   break;

  }

#else
  StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x00);
  switch(uRate)
  {
    case 0:
    StarMdcMdioWrite(dev, starPrv->phy_addr, 0x18, 0x0100);
    break;
    case 1:
    StarMdcMdioWrite(dev, starPrv->phy_addr, 0x18, 0x0300);
    break;
    case 2:
    StarMdcMdioWrite(dev, starPrv->phy_addr, 0x18, 0x0700);
    break;
    case 3:
    StarMdcMdioWrite(dev, starPrv->phy_addr, 0x18, 0x0f00);
    break;
    case 4:
    StarMdcMdioWrite(dev, starPrv->phy_addr, 0x18, 0x1f00);
    break;
    case 5:
    StarMdcMdioWrite(dev, starPrv->phy_addr, 0x18, 0x3f00);
    break;
    case 6:
    StarMdcMdioWrite(dev, starPrv->phy_addr, 0x18, 0x7f00);
    break;

    default:
    StarMdcMdioWrite(dev, starPrv->phy_addr, 0x18, 0x0f00);
    break;

   }

  #endif
  
}

void vStarGetSlewRate(StarDev *dev , u32 * uRate)
{
  StarPrivate *starPrv = dev->starPrv;
#ifdef CC_ETHERNET_6896
  StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x2a30);
  switch(StarMdcMdioRead(dev, starPrv->phy_addr, 0x15)&0x7F)
 {
   case 0x01:
    *uRate = 0;
   break;
   case 0x03:
    *uRate = 1;
   break;
   case 0x07:
    *uRate = 2;
   break;
   case 0x0f:
   *uRate = 3;
   break;
   case 0x1f:
   *uRate = 4;
   break;
   case 0x3f:
   *uRate = 5;
   break;
   case 0x7f:
   *uRate = 6;
   break;

   default:
    
   break;

  }

#else
  StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x00);
  switch((StarMdcMdioRead(dev, starPrv->phy_addr, 0x18)>>8)&0x7F)
  {
	  case 0x01:
	   *uRate = 0;
	  break;
	  case 0x03:
	   *uRate = 1;
	  break;
	  case 0x07:
	   *uRate = 2;
	  break;
	  case 0x0f:
	  *uRate = 3;
	  break;
	  case 0x1f:
	  *uRate = 4;
	  break;
	  case 0x3f:
	  *uRate = 5;
	  break;
	  case 0x7f:
	  *uRate = 6;
	  break;


    default:

    break;

   }

  #endif
  
}


void vStarSetOutputBias(StarDev *dev , u32 uOBiasLevel)
{
  u32 val;
  StarPrivate *starPrv = dev->starPrv;
  
  if(uOBiasLevel>0x03)
  {
	STAR_MSG(STAR_ERR, "setting fail ! value must be 0 ~ 3 \n");
	return;
  }

  StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x2a30);

  uOBiasLevel &= 0x03;	

#ifdef CC_ETHERNET_6896
  val = StarMdcMdioRead(dev, starPrv->phy_addr, 0x1d);  ////REG_TXG_OBIAS_100  //default value = 1
  val = ( val & (~(0x03<<4)))|((uOBiasLevel & 0x03)<< 4);//bit4~ bit5
   StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1d, val);

val = StarMdcMdioRead(dev, starPrv->phy_addr, 0x19);  ////REG_TXG_OBIAS_STB_100  //default value = 2
val = ( val & (~(0x03<<4)))|((uOBiasLevel & 0x03)<< 4);//bit4~ bit5
 StarMdcMdioWrite(dev, starPrv->phy_addr, 0x19, val);

#else

  val = StarMdcMdioRead(dev, starPrv->phy_addr, 0x18);

  val = ( val& 0xfff3)|((uOBiasLevel & 0x03)<< 2);//bit 2~ bit3  //default value = 2 
  val =  ( val& 0xffcf)|((uOBiasLevel & 0x03)<< 4);//bit 4~ bit5  //default value = 1
  StarMdcMdioWrite(dev, starPrv->phy_addr, 0x18, val);
#endif

  
  
  StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x0);//Normal Page
  
}

void vStarGetOutputBias(StarDev *dev , u32 * uOBiasLevel)
{
  u32 val;
  StarPrivate *starPrv = dev->starPrv;
  
  StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x2a30);

#ifdef CC_ETHERNET_6896
  val = StarMdcMdioRead(dev, starPrv->phy_addr, 0x1d);  ////REG_TXG_OBIAS_100  //default value = 1
  val = ( val>>4)  & 0x03 ;
#else
  val = StarMdcMdioRead(dev, starPrv->phy_addr, 0x18);
  val = ( val>>2) & 0x03;//bit 2~ bit3  //default value = 2 
#endif

  *uOBiasLevel = val ;
  
  StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x0);//Normal Page
  
}

void vStarSetInputBias(StarDev *dev , u32 uInBiasLevel)
{
  u32 val;
  StarPrivate *starPrv = dev->starPrv;
  
  if(uInBiasLevel>0x03)
  {
	STAR_MSG(STAR_ERR, "setting fail ! value must be 0 ~ 3 \n");
	return;
  }

  StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x2a30);
  	
  uInBiasLevel = 0x03 - (uInBiasLevel& 0x03);	

 
#ifdef CC_ETHERNET_6896
  val = StarMdcMdioRead(dev, starPrv->phy_addr, 0x1D);
  val = ( val& 0xfffc)|((uInBiasLevel & 0x03)<< 0);//bit 0~ bit1	//REG_TXG_BIASCTRL	default = 0
  StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1d, val);

  val = StarMdcMdioRead(dev, starPrv->phy_addr, 0x19);
  val = ( val& 0xfffc)|((uInBiasLevel & 0x03)<< 0);//bit 0~ bit1 //REG_TXG_BIASCTRL  default = 3
  StarMdcMdioWrite(dev, starPrv->phy_addr, 0x19, val);

#else
  val = StarMdcMdioRead(dev, starPrv->phy_addr, 0x18);

  val = ( val& 0xfffc)|((uInBiasLevel & 0x03)<< 0);//bit 0~ bit1  //REG_TXG_BIASCTRL_STB default = 3
  val =  ( val& 0xf9ff)|((uInBiasLevel & 0x03)<< 9);//bit 9~ bit10  //REG_TXG_BIASCTRL default=0
  StarMdcMdioWrite(dev, starPrv->phy_addr, 0x18, val);
#endif

  
  
  StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x0);//Normal Page
  
}

void vStarGetInputBias(StarDev *dev , u32 * uInBiasLevel)
{
  u32 val;
  StarPrivate *starPrv = dev->starPrv;
  
  StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x2a30);
  	
#ifdef CC_ETHERNET_6896
  val = StarMdcMdioRead(dev, starPrv->phy_addr, 0x1D);
  val =  val & 0x03;//bit 0~ bit1	//REG_TXG_BIASCTRL	default = 0
#else
  val = StarMdcMdioRead(dev, starPrv->phy_addr, 0x18);
  val =  val & 0x03;//bit 0~ bit1	 //REG_TXG_BIASCTRL  default = 0
#endif

  *uInBiasLevel = 0x03 - val;
  StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x0);//Normal Page
}

#ifdef CC_ETHERNET_6896
void vSetDACAmp(StarDev *dev , u32 uLevel)
{
	u32 val;
	StarPrivate *starPrv = dev->starPrv;
	if(uLevel>0x0F)
	{
	  STAR_MSG(STAR_ERR, "setting fail ! value must be 0~15 \n");
	  return;
	}

	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x2a30);
	  
	val = StarMdcMdioRead(dev, starPrv->phy_addr, 0x1E);
	val = ( val & (~(0x0F<<12)))|((uLevel & 0x0F)<< 12);
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1E, val);  
	  
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x0);//Normal Page

}
void vGetDACAmp(StarDev *dev , u32 * uLevel)
{
	u32 val;
	StarPrivate *starPrv = dev->starPrv;
	
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x2a30);
	  
	val = StarMdcMdioRead(dev, starPrv->phy_addr, 0x1E);
	*uLevel = ( val>>12)& 0x0f ;

	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x0);//Normal Page

}
void vSet100Amp(StarDev *dev , u32 uLevel) /////100M DAC 
{
	u32 val;
	StarPrivate *starPrv = dev->starPrv;
	if(uLevel>0x0F)
	{
	  STAR_MSG(STAR_ERR, "setting fail ! value must be 0~15 \n");
	  return;
	}
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x2a30);
	val = StarMdcMdioRead(dev, starPrv->phy_addr, 0x1A);
	val = ( val & (~0x0F))|(uLevel & 0x0F);
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1A, val);  
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x0);//Normal Page
}
void vGet100Amp(StarDev *dev , u32 * uLevel)
{
	u32 val;
	StarPrivate *starPrv = dev->starPrv;
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x2a30);
	val = StarMdcMdioRead(dev, starPrv->phy_addr, 0x1A);
	*uLevel =  val & 0x0f ;
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x0);//Normal Page
}

void vSet100SlewRate(StarDev *dev , u32 uLevel) //100M SlewRate
{
	u32 val;
	StarPrivate *starPrv = dev->starPrv;
	if(uLevel>0x7F)
	{
	  STAR_MSG(STAR_ERR, "setting fail ! value must be 0x00~0x7f \n");
	  return;
	}
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x2a30);
	val = StarMdcMdioRead(dev, starPrv->phy_addr, 0x15);
	val = ( val & (~0x7F))|(uLevel & 0x7F);
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x15, val);  
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x0);//Normal Page
}
void vGet100SlewRate(StarDev *dev , u32 * uLevel)
{
	u32 val;
	StarPrivate *starPrv = dev->starPrv;
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x2a30);
	val = StarMdcMdioRead(dev, starPrv->phy_addr, 0x15);
	*uLevel =  val & 0x7f ;
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x0);//Normal Page
}

#endif

void vStarSet10MAmp(StarDev *dev , u32 uLevel)
{
  u32 val;
  StarPrivate *starPrv = dev->starPrv;
  
  if(uLevel>3)
  {
    STAR_MSG(STAR_ERR, "setting fail ! value must be 0~3 \n");
	return;
  }

  StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x2a30); 	
  
  val = StarMdcMdioRead(dev, starPrv->phy_addr, 0x18);
#ifdef CC_ETHERNET_6896
  val = ( val& (~(0x03<<0)))|((uLevel & 0x03)<< 0);//bit 0, bit1 default =2
#else
  val = ( val& 0xff3f)|((uLevel & 0x03)<< 6);//bit 6, bit7 ///default = 2 
#endif

  StarMdcMdioWrite(dev, starPrv->phy_addr, 0x18, val);

  StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x0);//Normal Page
}

void vStarGet10MAmp(StarDev *dev , u32* uLevel)
{
  u32 val;
  StarPrivate *starPrv = dev->starPrv;
  

  StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x2a30); 	
  
  val = StarMdcMdioRead(dev, starPrv->phy_addr, 0x18);
#ifdef CC_ETHERNET_6896
  val = ( val);//bit 0, bit1 default =2
#else
  val = ( val>> 6);//bit 6, bit7 ///default = 2 
#endif
  *uLevel=val & 0x03 ;

  StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x0);//Normal Page
}

#if 0
static void vStarSetOutputExtraBias(StarDev *dev , u32 uLevel)
{
  u32 val;
  StarPrivate *starPrv = dev->starPrv;

  StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x2a30);
  	
#ifdef CC_ETHERNET_6896
  val = StarMdcMdioRead(dev, starPrv->phy_addr, 0x1D);
  val = ( val & (~(0x01<<12)))|((uLevel & 0x01)<< 12);//bit 12	 //default = 0 
  StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1D, val);
#else
   val = StarMdcMdioRead(dev, starPrv->phy_addr, 0x18);
   val = ( val& 0xdfff)|((uLevel & 0x01)<< 13);//bit 13   //default = 0 
   StarMdcMdioWrite(dev, starPrv->phy_addr, 0x18, val);
#endif
  
  StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x0);//Normal Page
}
#endif

void vStarSet50PercentBW(StarDev *dev , u32 uEnable)
{
  u32 val;
  StarPrivate *starPrv = dev->starPrv;
  
  if(uEnable>1)
  {
    STAR_MSG(STAR_ERR, "setting fail ! value must be 0~1 \n");
	return;
  }

  StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x2a30);
  	

#ifdef CC_ETHERNET_6896
  val = StarMdcMdioRead(dev, starPrv->phy_addr, 0x1D);
  val = ( val & (~(0x01<<14)))|((uEnable & 0x01)<< 14);//bit 14 default 0
  StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1D, val);
#else
  val = StarMdcMdioRead(dev, starPrv->phy_addr, 0x18);
  val = ( val& 0xf7ff)|((uEnable & 0x01)<< 11);//bit 11  ///default 0
  StarMdcMdioWrite(dev, starPrv->phy_addr, 0x18, val);
#endif
  
  StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x0);//Normal Page
}

void vStarGet50PercentBW(StarDev *dev , u32* uEnable)
{
  u32 val;
  StarPrivate *starPrv = dev->starPrv;
  

  StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x2a30);
  	

#ifdef CC_ETHERNET_6896
  val = StarMdcMdioRead(dev, starPrv->phy_addr, 0x1D);
  val = ( val >> 14)&0x01;//bit 14 default 0
#else
  val = StarMdcMdioRead(dev, starPrv->phy_addr, 0x18);
  val = ( val>> 11)&0x01;//bit 11  ///default 0
#endif
  *uEnable= val ;

  StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x0);//Normal Page
}

void vStarSetFeedBackCap(StarDev *dev , u32 uLevel)
{
  u32 val;
  StarPrivate *starPrv = dev->starPrv;

  if(uLevel>3)
  {
    STAR_MSG(STAR_ERR, "setting fail ! value must be 0~3 \n");
	return;
  }
  StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x2a30);//test page
  	
#ifdef CC_ETHERNET_6896
  val = StarMdcMdioRead(dev, starPrv->phy_addr, 0x1D);
  val = ( val & (~(0x03<<8)))|((uLevel & 0x03)<< 8);//bit 8 bit 9  default 0
  StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1D, val);
#else
  val = StarMdcMdioRead(dev, starPrv->phy_addr, 0x18);
  val = ( val& 0x3fff)|((uLevel & 0x03)<< 14);//bit 14, Bit15  default 0
  StarMdcMdioWrite(dev, starPrv->phy_addr, 0x18, val);
#endif
  
  StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x0);//Normal Page
}

void vStarGetFeedBackCap(StarDev *dev , u32* uLevel)
{
  u32 val;
  StarPrivate *starPrv = dev->starPrv;
  

  StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x2a30);//test page
  	
#ifdef CC_ETHERNET_6896
  val = StarMdcMdioRead(dev, starPrv->phy_addr, 0x1D);
  val = ( val >> 8);//bit 8 bit 9  default 0
#else
  val = StarMdcMdioRead(dev, starPrv->phy_addr, 0x18);
  val = ( val>> 14);//bit 14, Bit15  default 0
#endif
  *uLevel= val & 0x03 ;
  
  StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x0);//Normal Page
}
#endif //#if (USE_INTERNAL_PHY)	


#ifdef INTERNAL_PHY	
static int StarIntPhyInit(StarDev *dev)
{

    volatile u32 data;
    StarPrivate *starPrv = dev->starPrv;

    UNUSED(starPrv);

    /* Init IPLL for internal PHY if necessary */
#if (!USE_EXT_CLK_SRC) 
#if !defined(CONFIG_ARCH_MT5395)
#ifndef CC_ETHERNET_6896
    if (!StarIsExtClkSrcRequired(dev))
    {
        /* perform the following settings only when Internal PHY is used */
        data = StarGetReg(dev->pinmuxBase+0x34);
        if ((data & 0x01) == 0)
        {
            StarSetReg(dev->pinmuxBase+0x30, 0x85); /* FERST_CODE */
            StarSetReg(dev->pinmuxBase+0x34, 0x1); /* FE_RST */
            StarSetReg(dev->ethIPLL1Base+0x04, 0x801);
            data = StarGetReg(dev->ethIPLL2Base+0x10);
            data = (data&0xffffff00)|((2<<0)|(2 <<4));
            StarSetReg(dev->ethIPLL2Base+0x10, data);
        }
    }
#endif
#endif
#endif


#ifdef CONFIG_ARCH_MT5395
    {
        /* test page */
        StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1f, 0x2a30);
//        StarMdcMdioWrite(dev, starPrv->phy_addr, 0x03, 0x000c); /* 10M power saving */
        StarMdcMdioWrite(dev, starPrv->phy_addr, 0x08, 0x0210); /* disable gating of RCLK125 according to YT's suggest *///bit0 to clear for Harmonic in 8550
        /* extend page */
        StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x01);   
        data = StarMdcMdioRead(dev, starPrv->phy_addr, 0x1b);
        data &= ~(1<<15);
        StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1b, data); /* enable auto-crossover as AN is disabled */
        // def INT_PHY_LED_POL_INV
        data = StarMdcMdioRead(dev, starPrv->phy_addr, 0x10); //LED_POL default inverse
        data |= (1<<2);
        StarMdcMdioWrite(dev, starPrv->phy_addr, 0x10, data);
        /* main page */
        StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x00);

        // LED pinmux
        StarSetBit((dev->ethIPLL1Base+0x404), (1<<29) | (1<<28));
        StarClearBit((dev->ethIPLL1Base+0x408), (1<<30) | (1<<18));
        StarSetBit((dev->ethIPLL1Base+0x408), (1<<17) | (1<<16));
    }
#endif

#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5891)

	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1f, 0x3500); // Extended test page

	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x08, 0xfc55); 

	data = StarMdcMdioRead(dev, starPrv->phy_addr, 0x02);
	data = data | 1;
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x02, data); 

	udelay(1);
    
	data = StarMdcMdioRead(dev, starPrv->phy_addr, 0x02);
	data = data & ~1;
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x02, data); 
    
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x07, 0x0c10);
 
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1f, 0x2a30);
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x19, 0x778f);
#if defined(CONFIG_ARCH_MT5891)
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1d, 0x1052);
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x15, 0x0f2f);
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1, 0x8000);
#else
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1d, 0x1051);
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x15, 0x0f5f);
#endif
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1f, 0x00);
#endif

#if defined (CONFIG_ARCH_MT5396) || defined(CONFIG_ARCH_MT5368) || defined(CONFIG_ARCH_MT5389) || defined(CONFIG_ARCH_MT5398) || defined(CONFIG_ARCH_MT5880)||defined(CONFIG_ARCH_MT5399) || defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5882) || defined(CONFIG_ARCH_MT5883)|| defined(CONFIG_ARCH_MT5891)
        // configure LED 
#if defined(CONFIG_ARCH_MT5882) || defined(CONFIG_ARCH_MT5883)
	mdelay(100);
        StarSetReg(dev->base+0x6c,StarGetReg(dev->base+0x6c)|(1<<2)); // set FRC_SMI_ENB to write phy successfully
#endif

        StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1f, 0x0002); //misc page
        StarMdcMdioWrite(dev, starPrv->phy_addr, 0x00, 0x80f0);  // EXT_CTL([15]) = 1
        StarMdcMdioWrite(dev, starPrv->phy_addr, 0x03, 0xc000);  // LED0 on clean
        StarMdcMdioWrite(dev, starPrv->phy_addr, 0x04, 0x003f);  // LED0 blink for 10/100/1000 tx/rx
        StarMdcMdioWrite(dev, starPrv->phy_addr, 0x05, 0xc007);  // LED1 on for 10/100/1000M linkup
        StarMdcMdioWrite(dev, starPrv->phy_addr, 0x06, 0x0000);  // LED1 blink clean
        StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x0000);  // main page
#endif
        // LED pinmux
#if defined (CONFIG_ARCH_MT5396)
        StarSetBit((dev->ethIPLL1Base+0x400), (1<<6) | (1<<7));
//#elif defined (CONFIG_ARCH_MT5389)
        //data = StarGetReg(dev->ethIPLL1Base+0x400);
        //data &= ~(0x7 | (0x7<<3) | (0x7<<6));
        //data |= 0x6 | (0x6<<3) | (0x6<<6);
        //StarSetReg((dev->ethIPLL1Base+0x400), data);
#elif defined (CONFIG_ARCH_MT5398)
        data = StarGetReg(dev->ethIPLL1Base+0x608);
        data &= ~((0x3<<8) | (0x3<<10));
        data |= (0x1<<8) | (0x1<<10);
        StarSetReg((dev->ethIPLL1Base+0x608), data);
#endif

#if defined (CONFIG_ARCH_MT5389)
		StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x2a30);  // test page
	    StarMdcMdioWrite(dev, starPrv->phy_addr, 0x05, 0x1010);  // force MDI crossover
		StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x0000);  // main page
#elif defined (CONFIG_ARCH_MT5398)
        StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x2a30);  // test page
        data = StarMdcMdioRead(dev, starPrv->phy_addr, 0x15);
        data = (data & 0xff00) | 0x7f;
	    StarMdcMdioWrite(dev, starPrv->phy_addr, 0x15, data);  // force MDI crossover
		StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x0000);  // main page
#elif defined (CONFIG_ARCH_MT5399) || defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5882) || defined(CONFIG_ARCH_MT5883)|| defined(CONFIG_ARCH_MT5891)  
        StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x2a30);  // test page
        //msleep(50);
        
        data = StarMdcMdioRead(dev, starPrv->phy_addr, 0x1A);
        data = (data & 0xfff0)|(0x03 & 0x0f);//100M amp
        StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1A, data);  // 100M slew rate
        //msleep(50);
        
        data = StarMdcMdioRead(dev, starPrv->phy_addr, 0x15);
        data = (data & 0xff00) |((0x7f & 0x7f)<< 0);//100M slew rate
        StarMdcMdioWrite(dev, starPrv->phy_addr, 0x15, data);  // 100M slew rate
        //msleep(50);
	   
        data = StarMdcMdioRead(dev, starPrv->phy_addr, 0x18);
        data = (data & 0xfffc) |((0 & 0x03)<< 0);//10M amp
        StarMdcMdioWrite(dev, starPrv->phy_addr, 0x18, data);  // 100M slew rate
        //msleep(50);
        
        data = StarMdcMdioRead(dev, starPrv->phy_addr, 0x1E);
        data = (data & 0x0fff) |((0x3)<< 12);//10M amp
        StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1E, data);  // 100M slew rate
        //msleep(50);
        
        StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x0000);  // main page
        
#elif defined(CONFIG_ARCH_MT5880)|| defined (CONFIG_ARCH_MT5881)
        StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x2a30);  // test page
        msleep(50);
        data = StarMdcMdioRead(dev, starPrv->phy_addr, 0x15);
        data = (data & 0xff00) | 0x2f;
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x15, data);  // 100M slew rate
	msleep(50);
        data = StarMdcMdioRead(dev, starPrv->phy_addr, 0x18);
        data = (data & 0xfffC) | 0x0;
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x18, data);  // 10M amplitude
	msleep(50);
	StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x0000);  // main page
        StarMdcMdioWrite(dev, starPrv->phy_addr, 0x1F, 0x0000);  // main page
#endif
    
    //End Internal PHY Setting
    StarClearBit(STAR_PHY_CTRL1(dev->base), STAR_PHY_CTRL1_APDIS);//Enable Auto Polling after  set PHY register

    UNUSED(data);
#ifdef CC_ETHERNET_6896
#if defined(CONFIG_ARCH_MT5890) || defined (CONFIG_ARCH_MT5891)
	vSetDACAmp(dev, 0xa);
#if defined (CONFIG_ARCH_MT5891)
	vSet100Amp(dev, 0xa);
#else
	vSet100Amp(dev, 0x8);
#endif
#else
	vSetDACAmp(dev, INTERNAL_PHY_DACAMP);
	vSet100Amp(dev, INTERNAL_PHY_100AMP);
#endif
	vStarSet10MAmp(dev, INTERNAL_PHY_10M_AMP_LEVEL);

#if defined(CONFIG_ARCH_MT5880) || defined(CONFIG_ARCH_MT5883)
        vSet100Amp(dev, 4);  
#endif
#if defined(CONFIG_ARCH_MT5883)
        vSet100SlewRate(dev, 0x3f);
#endif



#endif

    return 0;
}
#endif //#ifdef INTERNAL_PHY

void StarPhyReset(StarDev *dev)
{
    u16 val = 0;
        
    StarPrivate *starPrv = dev->starPrv;

    val = StarMdcMdioRead(dev, starPrv->phy_addr, 0);
    val |= 0x8000;
    StarMdcMdioWrite(dev, starPrv->phy_addr, 0, val);
    if (patch_rtl8201el)
        StarMdcMdioWrite(dev, starPrv->phy_addr, 25, 0x44F9);
}


int StarInitPinMux(StarDev *dev)
{
 #ifdef USE_MAC_RMII
  u32 u4Reg = 0;	
 
    u4Reg = StarGetReg((dev->pinmuxBase+0x04));//0xd404
    u4Reg &= ~(0x07 << 28);//bit[30:28] ETMDC
    u4Reg |= (0x02 << 28);//bit[30:28] ETMDC
    u4Reg &= ~(0x07 << 20);//bit[23:20] ETTXD1
    u4Reg |= (0x02 << 20);//bit[23:20] ETTXD1
    StarSetReg((dev->pinmuxBase+0x04), u4Reg);
    
    u4Reg = StarGetReg((dev->pinmuxBase+0x0c));//0xd40c 
    u4Reg &= ~(0x07 << 24);//bit[26:24] ETMDIO
    u4Reg |= (0x02 << 24);//bit[26:24] ETMDIO // need also 0xd41c[16]
    u4Reg &= ~(0x07 << 16);//bit[18:16] ETRXD1
    u4Reg |= (0x02 << 16);//bit[18:16] ETRXD1
    u4Reg &= ~(0x07 << 12);//bit[14:12] ETRXD0
    u4Reg |= (0x02 << 12);//bit[14:12] ETRXD0
    u4Reg &= ~(0x07 << 4);//bit[6:4] ETRXDV
    u4Reg |= (0x02 << 4);//bit[6:4] ETRXDV
    u4Reg &= ~(0x07 << 0);//bit[2:0] ETTXCLK
    u4Reg |= (0x02 << 0);//bit[2:0] ETTXCLK
    StarSetReg((dev->pinmuxBase+0x0c), u4Reg);//0xd40c=0x02022222
    
    u4Reg = StarGetReg((dev->pinmuxBase+0x10));//0xd410 
    u4Reg &= ~(0x07 << 20);//bit[23:20] ETTXEN
    u4Reg |= (0x02 << 20);//bit[23:20] ETTXEN
    u4Reg &= ~(0x07 << 8);//bit[11:8] ETTXd0
    u4Reg |= (0x02 << 8);//bit[11:8] ETTXd0
    u4Reg &= ~(0x07 << 4);//bit[6:4] ETRXCK
    u4Reg |= (0x02 << 4);//bit[6:4]  ETRXCK
    u4Reg &= ~(0x07 << 0);//bit[3:0] ETPHYCLK 25M
    u4Reg |= (0x02 << 0);//bit[3:0] ETPHYCLK 25M
    StarSetReg((dev->pinmuxBase+0x10), u4Reg);//0xd410=0x02022222
    
    u4Reg = StarGetReg((dev->pinmuxBase+0x1c));//kenny add 2010/5/28
    u4Reg &= ~(1<<16); //for ETMDIO
    u4Reg &= ~(1<<19);//for ETMDC
    StarSetReg((dev->pinmuxBase+0x1c), u4Reg);//kenny add 2010/5/28
    
  #endif
  return 0; 	
}	

int StarHwInit(StarDev *dev)
{    
	u32 u4Reg = 0;
    StarPrivate *starPrv = dev->starPrv;

     /*It's not reasonable that no mac not set hw*/
     /*Remove the mac address checking to StarGetHwMacAddr*/
    /* 
    if((0==StarGetReg(dev->base+0x14)) && (0==StarGetReg(dev->base+0x18)))
        return 1;
    */

    /* Init pdwnc to enable Ethernet engine accessible by ARM only: 
        0: Accessed by ARM
        1: Accessed by T8032
    */
    /* SWITCH_ETHERNET_TO_ARM(); */
    #if 0 //kenny temply mark for MT5365
    StarSetReg(PDWNC_REG_UP_CONF_OFFSET(dev->pdwncBase),
                StarGetReg(PDWNC_REG_UP_CONF_OFFSET(dev->pdwncBase))&(~UP_CONF_ETEN_EN));

    /* Init pinmux for Ethernet */
    StarSetBitMask(PINMUX_REG_PAD_PINMUX2(dev->pinmuxBase),
                   PINMUX_REG_PAD_PINMUX2_ETH_MASK,
                   PINMUX_REG_PAD_PINMUX2_ETH_OFFSET,
                   PINMUX_REG_PAD_PINMUX2_ETH_MASK);
    #endif

#if defined (CONFIG_ARCH_MT5396) || defined(CONFIG_ARCH_MT5368) || defined(CONFIG_ARCH_MT5389)
    StarSetReg((dev->ethIPLL1Base+0x270), 0x9fffffff); // 0xf000d270 [30:29] = ETH reset
    msleep(5);
    StarSetReg((dev->ethIPLL1Base+0x270), 0xffffffff); // 0xf000d270 [30:29] = ETH reset
#elif defined (CONFIG_ARCH_MT5398) || defined (CONFIG_ARCH_MT5880)||defined(CONFIG_ARCH_MT5399) || defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5882) || defined(CONFIG_ARCH_MT5883)|| defined(CONFIG_ARCH_MT5891)
    StarSetReg((dev->base+0x300), 0x0);  //switch mac config by from 8032 to risc
    //msleep(5);
    StarWaitReady((dev->base+0x300), 0x0,0,0xffffffff,1,5);
    if (fgOpenStBeforeSuspend == false)
    {
        StarSetReg((dev->ethIPLL1Base+0x1c0), 0xf9ffffff); // 0xf0032308 [1:0] = ETH reset
        //msleep(5);
        StarWaitReady((dev->ethIPLL1Base+0x1c0), 0x0,25,0x3,1,5);
        StarSetReg((dev->ethIPLL1Base+0x1c0), 0xffffffff); // 0xf0032308 [1:0] = ETH reset
    }
#endif

#ifdef CONFIG_ARCH_MT5365
    #ifdef USE_MAC_RMII //for MT5365 QFP
    StarSetReg((dev->pinmuxBase+0x04), 0x20220000);//IO_WRITE32(0xf000d404, 0, 0x20220000);
    StarSetReg((dev->pinmuxBase+0x0c), 0x02022222); //IO_WRITE32(0xf000d40c, 0, 0x02022222);
    StarSetReg((dev->pinmuxBase+0x10), 0x00200222);//IO_WRITE32(0xf000d410, 0, 0x00200222);
    u4Reg = StarGetReg((dev->pinmuxBase+0x1c));//kenny add 2010/5/28
    u4Reg &= ~(1<<16);
    u4Reg &= ~(1<<19);
    StarSetReg((dev->pinmuxBase+0x1c), u4Reg);//kenny add 2010/5/28
    
    StarSetReg((dev->pinmuxBase+0x08), 0x0); //IO_WRITE32(0xf000d408, 0, 0x0);
    #else //Code for BGA
    StarSetReg((dev->pinmuxBase+0x08), 0x55555555);//    IO_WRITE32(0xf000d408, 0, 0x55555555);  //Pinmux for MT5366 BGA ethernet, only for verify	
    #endif               
      
    //set CK gen
    #ifdef USE_MAC_RMII //for MT5365 QFP
    StarSetReg((dev->ethIPLL1Base+0x28c), 0x00000001);//IO_WRITE32(0xf000d28c, 0, 0x00000001); //set miitx clock source from pad_ettxclk
    StarSetReg((dev->ethIPLL1Base+0x290), 0x00000001);//    IO_WRITE32(0xf000d290, 0, 0x00000001);//set miirx clock source from pad_ettxclk
    StarSetReg((dev->ethIPLL1Base+0x2d4), 0x00000001);//    IO_WRITE32(0xf000d2d4, 0, 0x00000001);//set rmii clock source from pad_ettxclk
    StarSetReg((dev->ethIPLL1Base+0x288), 0x00000000);//    IO_WRITE32(0xf000d288, 0, 0x00010000);//set mac clock source from xtal_d8
    StarSetReg((dev->ethIPLL1Base+0x294), 0x00000002);//IO_WRITE32(0xf000d294, 0, 0x00000002);//set mdc clock source from xtal_d8
    #else
    StarSetReg((dev->ethIPLL1Base+0x28c), 0x00000001);//IO_WRITE32(0xf000d28c, 0, 0x00000001); //set miitx clock source from pad_ettxclk
    StarSetReg((dev->ethIPLL1Base+0x290), 0x00000002);//    IO_WRITE32(0xf000d290, 0, 0x00000001);//set miirx clock source from pad_etrxclk
    StarSetReg((dev->ethIPLL1Base+0x2d4), 0x00000001);//    IO_WRITE32(0xf000d2d4, 0, 0x00000001);//set rmii clock source from pad_ettxclk
    StarSetReg((dev->ethIPLL1Base+0x288), 0x00010000);//    IO_WRITE32(0xf000d288, 0, 0x00010000);//set mac clock source from sawlesspll_d16
    StarSetReg((dev->ethIPLL1Base+0x294), 0x00000002);//IO_WRITE32(0xf000d294, 0, 0x00000002);//set mdc clock source from xtal_d8
    
    #endif

    //set CK gen output 25M clock
    u4Reg = StarGetReg((dev->ethIPLL1Base+0x714));
    u4Reg &= ~0xC0008000;
    u4Reg |= 0xA0000000;    
    StarSetReg((dev->ethIPLL1Base+0x714), u4Reg);
    //StarSetReg((dev->ethIPLL1Base+0x714), 0x30004300);
    StarSetReg((dev->ethIPLL1Base+0x700), 0x125C28F5);
    StarSetReg((dev->ethIPLL1Base+0x704), 0xbfe10001);
    StarSetReg((dev->ethIPLL1Base+0x708), 0xbfe10002);
    StarSetReg((dev->ethIPLL1Base+0x70c), 0xbfe10001);
    StarSetReg((dev->ethIPLL1Base+0x710), 0xbfe10002);
          
    #ifdef USE_MAC_RMII  
    u4Reg = StarGetReg((dev->ethIPLL1Base+0x510));//0xd510 bit4 =1 to GPIO4
    u4Reg |= 0x00000010;
    StarSetReg((dev->ethIPLL1Base+0x510), u4Reg);    
    #else
    u4Reg = StarGetReg((dev->ethIPLL1Base+0x518));//0xd518 bit15 =1 to ETPHCLK
    u4Reg |= 0x00008000;
    StarSetReg((dev->ethIPLL1Base+0x518), u4Reg);    
    #endif
    StarSetReg(STAR_CLK_CFG(dev->base), 0x0a81000a);//kenny for MT5365 QFP

#endif

#ifdef CONFIG_ARCH_MT5395
    /*
    Internal PHY:
        Reg32090 = 7FFF_0000 (little endian)
        Reg3209C = 0A81_000A (select clk27)
        Reg3206C = 0000_0200 (Internal PHY)
        Reg0d28C = 0000_0003 (txclk)
        Reg0d290 = 0000_0103 (rxclk)
        Reg0d288 = 0001_0000 (mac_clk)
        Reg0d294 = 0000_0002 (mdc_clk)
    */
    StarSetReg((dev->ethIPLL1Base+0x28c), 0x00000003);// set miitx clock source
    StarSetReg((dev->ethIPLL1Base+0x290), 0x00000103);// set miirx clock source
    StarSetReg((dev->ethIPLL1Base+0x288), 0x00010000);// set mac clock source
    StarSetReg((dev->ethIPLL1Base+0x294), 0x00000002);// set mdc clock source from

    (void)(u4Reg);
#endif

#if defined(CONFIG_ARCH_MT5396) || defined(CONFIG_ARCH_MT5368) || defined(CONFIG_ARCH_MT5389) || defined(CONFIG_ARCH_MT5398) || defined(CONFIG_ARCH_MT5880)|| defined(CONFIG_ARCH_MT5399) || defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5882) || defined(CONFIG_ARCH_MT5883)|| defined(CONFIG_ARCH_MT5891)
#ifdef CONFIG_ARCH_MT5389
    u4Reg = StarGetReg(dev->ethIPLL1Base+0x6E4);
    u4Reg &= ~(0x1 << 1); 
    StarSetReg(dev->ethIPLL1Base+0x6E4, u4Reg);
    msleep(50);
#endif    
#if defined(CONFIG_ARCH_MT5880)||defined(CONFIG_ARCH_MT5399) || defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5882) || defined(CONFIG_ARCH_MT5883)|| defined(CONFIG_ARCH_MT5891)
    u4Reg = StarGetReg(dev->ethIPLL1Base+0x164);
    u4Reg &= ~(0x1 << 1); 
    StarSetReg(dev->ethIPLL1Base+0x164, u4Reg);
    //msleep(50);
    StarWaitReady((dev->ethIPLL1Base+0x164), 0x0,1,0x1,10,5);
#endif 

#if defined(CONFIG_ARCH_MT5882)

if (IS_IC_MT5885())
{
    u4Reg = StarGetReg(dev->base+0x308);
    u4Reg &= ~(0x1 << 7);
	StarSetReg(dev->base+0x308,u4Reg);
	StarWaitReady(dev->base+0x308, 0x0,7,0x1,10,5);
	STAR_MSG(STAR_DBG, "StarHwInit 0x32308 value(0x%x)!\n", StarGetReg(dev->base+0x308));

	u4Reg = StarGetReg(dev->base+0x308);
	u4Reg |= (0x1 << 7);
	StarSetReg(dev->base+0x308,u4Reg);
	StarWaitReady(dev->base+0x308, 0x1,7,0x1,10,5);
	STAR_MSG(STAR_DBG, "StarHwInit 0x32308 value(0x%x)!\n", StarGetReg(dev->base+0x308));
}

#endif


    ////for 5396 IC test ///internal PHY
    StarSetReg(dev->base+0x94, 0x08);
    StarSetReg(dev->base+0x9C, 0x1F);
    //msleep(100);
    StarWaitReady(dev->base+0x9C, 0x1F,0,0x1F,10,10);
#endif
                    
    /* Init Clock */
    //#ifdef CONFIG_MT53_FPGA //kenny mark for //MT5365 MDC clock is controlled by CKGen
    //StarSetReg(STAR_CLK_CFG(dev->base), MAC_CLK_INIT); //0x0a81010a
    //#endif
    
    //StarSetReg(STAR_CLK_CFG(dev->ethPdwncBase), MAC_CLK_INIT); //0x0a81000a, kenny temply mark for MT5365

    /* Get PHY ID */
    if (eth_phy_id == 32)
    {
        eth_phy_id = StarDetectPhyId(dev);
        if (eth_phy_id == 32)
        {
            eth_phy_id = ETH_DEFAULT_PHY_ADDR;
            STAR_MSG(STAR_ERR, "Cannot detect PHY addr, default to %d\n", eth_phy_id);
        }
        else
            STAR_MSG(STAR_DBG, "PHY addr = 0x%04x\n", eth_phy_id);
    }
    starPrv->phy_addr = (u32)eth_phy_id;
    starPrv->mii.phy_id = eth_phy_id;
    
#ifdef INTERNAL_PHY	
    if (StarPhyMode(dev) == INT_PHY_MODE)
    {
        #if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5891)
        mdelay(50); // Delay for the Phy working in a normal status.
        #endif
        
        StarIntPhyInit(dev);
    }
#endif
    UNUSED(u4Reg);
    return 0;
}

void StarLinkStatusChange(StarDev *dev)
{
    /* This function shall be called only when PHY_AUTO_POLL is enabled */
    u32 val = 0;
    u32 speed = 0;

    val = StarGetReg(STAR_PHY_CTRL1(dev->base));

    if (dev->linkUp != ((val & STAR_PHY_CTRL1_STA_LINK)?1UL:0UL))
    {
        dev->linkUp = (val & STAR_PHY_CTRL1_STA_LINK)?1UL:0UL;
        STAR_MSG(STAR_WARN, "Link status: %s\n", dev->linkUp? "Up":"Down");
        if (dev->linkUp)
        {
            speed = ((val >> STAR_PHY_CTRL1_STA_SPD_OFFSET) & STAR_PHY_CTRL1_STA_SPD_MASK);
            STAR_MSG(STAR_WARN, "%s Duplex - %s Mbps mode\n",
                   (val & STAR_PHY_CTRL1_STA_FULL)?"Full":"Half",
                   !speed? "10":(speed==1?"100":(speed==2?"1000":"unknown")));
            STAR_MSG(STAR_WARN, "TX flow control:%s, RX flow control:%s\n",
                    (val & STAR_PHY_CTRL1_STA_TXFC)?"On":"Off",
                    (val & STAR_PHY_CTRL1_STA_RXFC)?"On":"Off");
        }
        else
        {
            netif_carrier_off(((StarPrivate *)dev->starPrv)->dev);
        }
    }
    if (dev->linkUp)
    {
        netif_carrier_on(((StarPrivate *)dev->starPrv)->dev);
    }
}


void StarNICPdSet(StarDev *dev, u32 val)
{
    u32 data;
    int retry = 0;
#define MAX_NICPDRDY_RETRY  10000
    
    data = StarGetReg(STAR_MAC_CFG(dev->base));
    if (val)
    {
        data |= STAR_MAC_CFG_NICPD;
        StarSetReg(STAR_MAC_CFG(dev->base), data);
        /* wait until NIC_PD_READY and clear it */
        do
        {
            if ((data = StarGetReg(STAR_MAC_CFG(dev->base))) & STAR_MAC_CFG_NICPDRDY)
            {
                data |= STAR_MAC_CFG_NICPDRDY; /* clear NIC_PD_READY */
                StarSetReg(STAR_MAC_CFG(dev->base), data);    
                break;
            }
        } while(retry++ < MAX_NICPDRDY_RETRY);
        if (retry >= MAX_NICPDRDY_RETRY)
        {
            STAR_MSG(STAR_ERR, "Error NIC_PD_READY is not set in %d retries!\n", MAX_NICPDRDY_RETRY);
        }
    }
    else
    {
        data &= (~STAR_MAC_CFG_NICPD);
        StarSetReg(STAR_MAC_CFG(dev->base), data);
    }
}

module_param(eth_phy_id, int, S_IRUGO);

