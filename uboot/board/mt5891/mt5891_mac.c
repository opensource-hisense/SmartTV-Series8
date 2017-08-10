/*-----------------------------------------------------------------------------
 * uboot/board/mt5398/mt5398_mac.c
 *
 * MT53xx Ethernet driver
 *
 * Copyright (c) 2008-2012 MediaTek Inc.
 * $Author: dtvbm11 $
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 *
 *---------------------------------------------------------------------------*/

#include "mt5891_mac.h"
#include "../include/configs/mt5891.h"
#include "c_model.h"

//#define MT539X_LAN8700
#ifdef MT539X_LAN8700
#define DEFAULT_PHY_ADDRESS 22
#else
#define DEFAULT_PHY_ADDRESS 1
#endif
#define DEFAULT_MAC_ADDRESS { 0x00, 0x0C, 0xE7, 0x06, 0x00, 0x00 }
#define TIMEOUT  0x10000//(10*HZ)
//static struct eth_device *this_device;
static unsigned char *pbuf = NULL;
static unsigned char *pbufOrg = NULL;
static u8  BCmacAddr[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };


#if 1
void vDmaInit(void)
{

    vWriteMAC(RW_BURST_TYPE, DMA_BUSMODE_INIT);
    vWriteMAC(RW_FC_CFG, MAC_FLOWCTRL_INIT);
    vWriteMAC(RW_EXTEND_CFG, SEND_PAUSE_RLS_1K);

    vWriteMAC(RW_DMA_CFG, DMA_OPMODE_INIT);
    MmacDmaIntDisable(NULL);

}
#define PHY_ID_NUM_LOW_WORD     0x5AA5
#define NET_DRV_HW_ERROR  0
#define NET_DRV_OK 1

// *********************************************************************
// Function    : i4MacPhyRead
// Description : MII PHY register read
// Parameter   : u4PhyAddr - phy device address
//               u4RegAddr - phy register offset
//               pu4Data - output read data
// Return      : 0 if success
// *********************************************************************
INT32 i4MacPhyRead(UINT32 u4PhyAddr, UINT32 u4RegAddr, UINT32 *pu4Data)
{
    UINT32 u4Data, u4TimeOut;

   // ASSERT(pu4Data != NULL);

   u4Data = 0;
   u4Data = (u4PhyAddr << PHY_ADDR_SHIFT) & PHY_ADDR;
   u4Data |= ((u4RegAddr << PHY_REG_SHIFT) & PHY_REG);
   u4Data |= RD_CMD;
   vWriteMAC(RW_PHY_CTRL0, u4Data);

    u4TimeOut = 0xffff;
    while((dReadMAC(RW_PHY_CTRL0) & RW_OK) == 0) // Wait for R/W command complete
    {
        u4TimeOut --;
        if(u4TimeOut == 0x00)
        {
            TR("MII read PHY timeout\n");
            return(NET_DRV_HW_ERROR);
        }
    }

    *pu4Data = (UINT32)(dReadMAC(RW_PHY_CTRL0) >> RW_DATA_SHIFT);        // Get PHY data
    SetBitMAC(RW_PHY_CTRL0, RW_OK);  //clear RW_OK

    return (NET_DRV_OK);
}

// *********************************************************************
// Function    : void i4MacPhyWrite(UINT32 u4PhyAddr, UINT32 u4RegAddr, UINT32 u4RegData)
// Description : MII PHY register write
// Parameter   : u4PhyAddr, u4RegAddr, u4RegData
// Return      : none
// *********************************************************************
INT32 i4MacPhyWrite(UINT32 u4PhyAddr, UINT32 u4RegAddr, UINT32 u4RegData)
{
    UINT32 u4Data, u4TimeOut;

   u4Data = (u4RegData & 0xFFFF) << RW_DATA_SHIFT;
   u4Data |= ((u4PhyAddr << PHY_ADDR_SHIFT) & PHY_ADDR);
   u4Data |= ((u4RegAddr << PHY_REG_SHIFT) & PHY_REG);
   u4Data |= WT_CMD;

   vWriteMAC(RW_PHY_CTRL0, u4Data);

    u4TimeOut = 0xffff;
    while((dReadMAC(RW_PHY_CTRL0) & RW_OK) == 0) // Wait for R/W command complete
    {
        u4TimeOut --;
        if(u4TimeOut == 0x00)
        {
            TR("MII write PHY timeout\n");
            return(NET_DRV_HW_ERROR);
        }
    }
    SetBitMAC(RW_PHY_CTRL0, RW_OK);  //clear RW_OK

    return(NET_DRV_OK);
}


#if 1//(INTERNAL_PHY)

// *********************************************************************
// Function    : void i4MacIntPhyInit(UINT32 u4PhyAddr)
// Description : PHY device SW init
// Parameter   : u4PhyAddr
// Return      : none
// *********************************************************************
INT32 i4MacIntPhyInit(UINT32 u4PhyAddr)
{

    #if 0
    INT32 u1InterPhyAddr=0;
    #endif

    #if 0
    BSP_PinSet(84 , 1);
    BSP_PinSet(85 , 1);

    i4MacPhyWrite(u4PhyAddr, 0x1F, 0x0002);     //misc page
    i4MacPhyWrite(u4PhyAddr, 0x00, 0x80f0);     //EXT_CTL([15]) = 1
    i4MacPhyWrite(u4PhyAddr, 0x03, 0xc000);     //LED0 on clean
    i4MacPhyWrite(u4PhyAddr, 0x04, 0x003f);     //LED0 blink for 10/100/1000 tx/rx
    i4MacPhyWrite(u4PhyAddr, 0x05, 0xc007);     //LED1 on for 10/100/1000M
    i4MacPhyWrite(u4PhyAddr, 0x06, 0x0000);     //LED1 blink clean
    i4MacPhyWrite(u4PhyAddr, 0x1F, 0x0000);     //main page
    #endif

    #ifdef CONFIG_ARCH_MT5891
    UINT32 data;

    i4MacPhyWrite(u4PhyAddr, 0x1f, 0x3500);

    i4MacPhyWrite(u4PhyAddr, 0x08, 0xfc55);

    i4MacPhyRead(u4PhyAddr, 0x02, &data);
    data = data | 1;
    i4MacPhyWrite(u4PhyAddr, 0x2, data);

    udelay(1);

    i4MacPhyRead(u4PhyAddr, 0x02, &data);
    data = data & ~1;
    i4MacPhyWrite(u4PhyAddr, 0x2, data);

    i4MacPhyWrite(u4PhyAddr, 0x07, 0x0c10);

    i4MacPhyRead(u4PhyAddr, 0x1f, &data);

    i4MacPhyRead(u4PhyAddr, 0x08, &data);

    i4MacPhyRead(u4PhyAddr, 0x07, &data);

    i4MacPhyWrite(u4PhyAddr, 0x1f, 0x2a30);
    i4MacPhyWrite(u4PhyAddr, 0x1d, 0x1051);
    i4MacPhyWrite(u4PhyAddr, 0x15, 0x0f5f);
    i4MacPhyWrite(u4PhyAddr, 0x19, 0x778f);
    i4MacPhyWrite(u4PhyAddr, 0x1f, 0x00);
    #endif

    //End Internal PHY Setting
    ClrBit(ETHERNET_BASE+0x04, AUTO_POLL_DIS);//Enable Auto Polling after  set PHY register

    TR("ethernet internal PHY init successful!\n");
    return(NET_DRV_OK);
}

INT32 i4InterPhyClockExtSource(BOOL fgExtClock)
{
  UINT32 u4Data;

  if(fgExtClock == TRUE)
  {
   i4MacPhyWrite(0x00, 0x1f, 0x2a30);
   i4MacPhyRead(0x00, 0x0f, &u4Data);
   u4Data |= (1<<8);
   i4MacPhyWrite(0x00, 0x0f, u4Data);

   i4MacPhyRead(0x00, 0x01, &u4Data);
   u4Data |= (1<<3);
   i4MacPhyWrite(0x00, 0x01, u4Data);
   i4MacPhyWrite(0x00, 0x1f, 0x0);
  }

    return (NET_DRV_OK);

}

#endif

// *********************************************************************
// Function    : i4MacPhyAddrGet
// Description : Get PHY physical address by r/w register result
// Parameter   : prMac
// Return      : 0 on success, others on failure
// *********************************************************************
u8  u8MacPhyAddrGet(void)
{
   #if (!INTERNAL_PHY) //kenny mark it
   UINT32 u4Data, u4PhyAddr;
   UINT32 phyIdentifier2;
   #endif

   // ASSERT(prMac != NULL);
    #if (!INTERNAL_PHY) //kenny mark it
#define PHY_REG_IDENTFIR2	(3) 	 /* Reg3: PHY Identifier 2 */
#define PHYID2_INTVITESSE	(0x0430) /* Internal PHY */
#define PHYID2_RTL8201		(0x8201) /* RTL8201 */
#define PHYID2_RTL8211		(0xC912) /* RTL8211 */
#define PHYID2_IP101A		(0x0C50) /* IC+IP101A */
#define PHYID2_SMSC7100 	(0xC0B1) /*SMSC LAN7100 */
#define PHYID2_SMSC8700 	(0xC0C4) /*SMSC LAN8700 */


    for(u4PhyAddr = 0; u4PhyAddr < 32; u4PhyAddr++)
    {
        // MII_PHY_ID0_REG can be r/w on SMSC8700, but read only for RTL8201
        i4MacPhyRead(u4PhyAddr, MII_PHY_ID1_REG, &phyIdentifier2);


		if (phyIdentifier2 == PHYID2_INTVITESSE)
		{
			TR("Star Ethernet: Internal Vitesse PHY\n\r");
			break;
		} else if (phyIdentifier2 == PHYID2_RTL8201)
		{
			TR("Star Ethernet: RTL8201 PHY\n\r");
			break;
		} else if (phyIdentifier2 == PHYID2_RTL8211)
		{
			TR("Star Ethernet: RTL8211 PHY\n\r");
			break;
		}
        else if ((phyIdentifier2&0xfff0) == PHYID2_IP101A)
        {
            TR("Star Ethernet: IC+IP101A PHY\n\r");
            break;
        }
        else if (phyIdentifier2 == PHYID2_SMSC7100)
        {
            TR("Star Ethernet: SMSC7100 PHY\n\r");
            break;
        }
        else if (phyIdentifier2 == PHYID2_SMSC8700)
        {
            TR("Star Ethernet: SMSC8700 PHY\n\r");
            break;
        }

    }

    if (u4PhyAddr >= 32)
    {
        for (u4PhyAddr = 0; u4PhyAddr < 32; u4PhyAddr++)
        {
            i4MacPhyRead(u4PhyAddr, MII_PHY_ID1_REG, &phyIdentifier2);
            if (phyIdentifier2 != 0xffff)
                break;
        }
    }
	if (u4PhyAddr >= 32) u4PhyAddr = 0;
	return u4PhyAddr;

    #endif

    return  0;//kenny temply


}

#ifdef CONFIG_ARCH_MT5891
int i4MacPhyZCALInit(u8 phy_addr)
{
	int i4Ret = 0;

	i4MacPhyWrite(phy_addr, 0x1f, 0x0002);//misc page
    	i4MacPhyWrite(phy_addr, 0x0d, 0x1016);
    	i4MacPhyWrite(phy_addr, 0x1f, 0x00);

	UNUSED(i4Ret);
	return(i4Ret);
}

int i4MacPhyZCALRemapTable(u8 phy_addr, u8 value)
{
	int i4Ret = 0;
	u16 temp;

    	i4MacPhyWrite(phy_addr, 0x1f, 0x2a30);

    	i4MacPhyRead(phy_addr, 0x14, &temp);
	temp |= (0x1 << 12);
	temp &= ~(0x0f00);
	temp |= value << 8;
	temp &= ~(0xff);
    	i4MacPhyWrite(phy_addr, 0x14, temp);

    	i4MacPhyWrite(phy_addr, 0x1f, 0x00);

	UNUSED(i4Ret);
	return(i4Ret);
}
#endif
void vMacInit(void)
{
	u8  u8PhyAddr=0;

    	#ifdef CONFIG_ARCH_MT5891
	u32 u4Data;
	u8 index, temp = 0x00;
	u8 mapTable[16][2] = {{0x01, 0x08}, {0x02, 0x07}, {0x03, 0x06}, {0x05, 0x05},
		{0x06, 0x04}, {0x07, 0x03}, {0x09, 0x02}, {0x0b, 0x01}, {0x0d, 0x00}, {0x21, 0x09},
		{0x23, 0x0a}, {0x24, 0x0b}, {0x26, 0x0c}, {0x27, 0x0d}, {0x28, 0x0e}, {0x2a, 0x0f}};
	#endif
    //vWriteMACPDWNC(RW_EXT_CFG, PDWNC_MAC_EXT_INIT);  //Little endian
        ////for 5396 IC test ///internal PHY
    ClrBitMAC(ETH_UP_CFG, CPUCK_BY_8032);

	SetBitMAC(ETHSYS_CONFIG, INT_PHY_SEL);
	SetBitMAC(SW_RESET_CONTROL, PHY_RSTN);

    SetBitMAC(RW_ETHPHY, CFG_INT_ETHPHY);
    ClrBitMAC(RW_ETHPHY, MII_PAD_OE);

    /* Init MAC setting */
    vWriteMAC(RW_MAC_CFG, MAC_CFG_INIT);
    //vHalSetMacAddr((MAC_ADDRESS_T *)MAC_ADDR);
    vWriteMAC(RW_ARL_CFG, MAC_FILTER_INIT);

#if (INTERNAL_PHY)
    i4MacIntPhyInit(u8PhyAddr);  //init internal PHY
#endif

    // PHY software reset, it takes time
    //    i4Ret = i4MacPhyReset(prMac->u4PhyAddr);
    //    if(i4Ret != NET_DRV_OK)
    //    {
    //        return;
    //    }

    /* Init MAC setting */
    vWriteMAC(RW_MAC_CFG, MAC_CFG_INIT);
    SetBitMAC(RW_PHY_CTRL1, AUTO_POLL_DIS);
    // vHalSetMacAddr((MAC_ADDRESS_T *)MAC_ADDR);
    vWriteMAC(RW_ARL_CFG, MAC_FILTER_INIT);


    /* For first time to use the PHY */
    //i4MacPhyAddrGet(prMac);
    vWriteMAC(RW_PHY_CTRL1, PHY_CTRL_INIT | ((u8PhyAddr << PHY_ADDR_AUTO_SHIFT) & PHY_ADDR_AUTO));
    ClrBitMAC(RW_PHY_CTRL1, AN_EN);
    mdelay(1);
    SetBitMAC(RW_PHY_CTRL1, AN_EN);
    #ifdef CONFIG_ARCH_MT5891
	i4MacPhyZCALInit(u8PhyAddr);

   	u4Data = dReadMAC(0x8c);
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
		i4MacPhyZCALRemapTable(u8PhyAddr, 0x08);
	}
	else
	{
		i4MacPhyZCALRemapTable(u8PhyAddr, mapTable[index][1]);
	}

    #endif
}

static void StarMibReset(void)
{
//	u32 base = dev->base;
    dReadMAC(RW_C_RXOKPKT);
    dReadMAC(RW_C_RXOKBYTE);
    dReadMAC(RW_C_RXRUNT);
    dReadMAC(RW_C_RXLONG);
    dReadMAC(RW_C_RXDROP);
    dReadMAC(RW_C_RXCRC);
    dReadMAC(RW_C_RXARLDROP);
    dReadMAC(RW_C_RXVLANDROP);
    dReadMAC(RW_C_RXCSERR);
    dReadMAC(RW_C_RXPAUSE);
    dReadMAC(RW_C_TXOKPKT);
    dReadMAC(RW_C_TXOKBYTE);
    dReadMAC(RW_C_TXPAUSE);
}


/**********************************************************
 * device cfg related functions
 **********************************************************/
void hwcfgsetup(void)
{
    vDmaInit();  //shengming
    vMacInit();

    vWriteMAC(RW_ARL_CFG, MAC_FILTER_INIT);
    //vWriteMAC(RW_PHY_CTRL1, PHY_CTRL_INIT | ((0 << PHY_ADDR_AUTO_SHIFT) & PHY_ADDR_AUTO));
    vWriteMAC(RW_DLY_INT_CFG, DMA_INT_ENABLE_ALL); ///clear all int mask

    StarNICPdSet(0) ;


    MmacDmaRxStop(NULL); 	 /* stop receiver, take ownership of all rx descriptors */
    MmacDmaTxStop(NULL); 	 /* stop transmitter, take ownership of all tx descriptors */

    /* init carrier to off */
    //netif_carrier_off(dev);

    //SetBitMAC(RW_ETHPHY,MII_PAD_OE);
    SetBitMAC(RW_ETHPHY,FRC_SMI_ENB);
    //SetBitMAC(RW_TEST1,TEST1_EXTEND_RETRY);

    /* Set Mac Configuration */

#ifdef CHECKSUM_OFFLOAD
    vWriteMAC(RW_MAC_CFG,
    		  //STAR_MAC_CFG_TXCKSEN |
    		  RX_CKS_EN |
    		  CRC_STRIPPING |
    		  MAX_LEN_1522 |
    		  IPG /* 12 byte IPG */
    		);
#else
    vWriteMAC( RW_MAC_CFG,
    		  CRC_STRIPPING |
    		  MAX_LEN_1522 |
    		  IPG /* 12 byte IPG */
    		);
#endif


    StarMibReset();

    /* Enable Hash Table BIST */
    SetBitMAC(RW_HASH_CTRL, HASH_TABLE_BIST_START);
    /* Reset Hash Table (All reset to 0) */
    SetBitMAC(RW_TEST1,RESTART_HASH_MBIST);

    iPhyCtrlInit(1);

}


/**********************************************************
 * Common functions
 **********************************************************/

int MmacInit( Mmac *Dev, u32 ConfigBase, u32 MacBase, u32 DmaBase, u32 PhyAddr )
{
  memset( Dev, 0, sizeof(*Dev) );

  Dev->configBase = ConfigBase;
  Dev->macBase = MacBase;
  Dev->dmaBase = DmaBase;
  Dev->phyAddr = PhyAddr;

  return 0;
}

int MmacMacInit( Mmac *Dev, u8 Addr[6], u8 Broadcast[6] )
{
  /* Set Mac Address */
  vWriteMAC(RW_MY_MAC_H, (Addr[0]<<8) | (Addr[1]<<0));
  vWriteMAC(RW_MY_MAC_L, (Addr[2]<<24) | (Addr[3]<<16) | (Addr[4]<<8) | (Addr[5]<<0));


   if(Dev->full_duplex)
  {
	  SetBitMAC(RW_PHY_CTRL1, FORCE_DUPLEX_FULL);
  }
  else
  {
	  ClrBitMAC(RW_PHY_CTRL1, FORCE_DUPLEX_FULL);
  }

  /* 100/10 Mbps selection */
  if(Dev->speed_100)
  {
	  SetBitMAC(RW_PHY_CTRL1, FORCE_SPEED_100M);
  }
  else
  {
	  ClrBitMAC(RW_PHY_CTRL1, FORCE_SPEED_10M);
  }



  return 0;
}



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

int MmacDmaInit( Mmac *dev, TxDesc *tdesc, RxDesc *rdesc )
{
	int i;
	//u32 base = dev->base;

	dev->txRingSize = NUM_TX_DESC;
	dev->rxRingSize = NUM_RX_DESC;

	//dev->txdesc = (TxDesc *)((u32)tdesc & (~(DMA_ALIGN_SIZE-1)));
	//dev->rxdesc = (RxDesc *)((u32)rdesc & (~(DMA_ALIGN_SIZE-1)));
	dev->txdesc = (TxDesc *)(((u32)tdesc) & (~(DMA_ALIGN_SIZE-1)));
    	TR("txdesc : 0x%08x\n",(u32)dev->txdesc);
	dev->rxdesc = (RxDesc *)(((u32)rdesc) & (~(DMA_ALIGN_SIZE-1)));
    	TR("rxdesc : 0x%08x\n",(u32)dev->rxdesc);

	for (i=0; i<dev->txRingSize; i++) { DescTxInit(dev->txdesc + i, i==dev->txRingSize-1); }
	for (i=0; i<dev->rxRingSize; i++) { DescRxInit(dev->rxdesc + i, i==dev->rxRingSize-1); }

	for (i=0; i<dev->rxRingSize; i++) DescRxTake(dev->rxdesc + i);
	for (i=0; i<dev->txRingSize; i++) DescTxTake(dev->txdesc + i);


	dev->rxReadIdx = 0;
    dev->txWrIdx = 0 ;

	/* TODO: Reset Procedure */
	/* OOXX.... */



 // MmacDmaRxStop(NULL);
 // MmacDmaTxStop(NULL);
 // vWriteMAC(RW_TX_DMA_CTRL, 0);
 // vWriteMAC(RW_RX_DMA_CTRL, 0);


  /* Set Tx/Rx descriptor address */
  vWriteMAC(RW_TX_BASE_ADDR, (u32)dev->txdesc);
  vWriteMAC(RW_TX_DPTR, dReadMAC(RW_TX_BASE_ADDR));

  vWriteMAC(RW_RX_BASE_ADDR, (u32)dev->rxdesc );
  vWriteMAC(RW_RX_DPTR, dReadMAC(RW_RX_BASE_ADDR));

  /* Enable Rx 2 byte offset (FIXME) */
  //StarDmaRx2BtOffsetEnable(dev);
  /* Init DMA_CFG, Note: RX_OFFSET_2B_DIS is set to 0 */

  vWriteMAC(RW_DMA_CFG,
			  RX_OFFSET_2B_DIS );

 // ClrBitMAC(RW_DMA_CFG,TX_SUSPEND|RX_SUSPEND);


  //MmacWriteDmaReg( Dev, DmaInterrupt, DmaIntDisable );
   MmacDmaIntDisable(NULL);
  return 0;
}


int StarDmaTxSet(Mmac *dev, u32 buffer, u32 length, u32 extBuf)
{
	int descIdx = dev->txWrIdx;
	TxDesc *txDesc = dev->txdesc + descIdx;

	/* Error checking */
	if(DescTxDma(txDesc))  goto err;

	txDesc->buffer = buffer;
	txDesc->ctrlLen &= ~(TX_LEN_MASK<<TX_LEN_OFFSET);
	txDesc->ctrlLen |= ((((length < 60)?60:length) & TX_LEN_MASK) << TX_LEN_OFFSET)| TX_FS | TX_LS   /*Tx Interrupt Enable*/;
	txDesc->reserve = extBuf;
	txDesc->vtag = 0;
	//wmb();
	txDesc->ctrlLen &= ~TX_COWN;     /* Set HW own */

	TR(  "." );

    dev->txWrIdx = DescTxLast(txDesc) ? 0 : descIdx + 1;
	return descIdx;
err:
	return -1;
}



void StarDmaRxSet(Mmac *dev, u32 buffer, u32 length, u32 extBuf, u32 dwIdx)
{
	RxDesc *rxDesc = dev->rxdesc + dwIdx;

	rxDesc->buffer = buffer;
	rxDesc->ctrlLen &= (~(RX_LEN_MASK<<RX_LEN_OFFSET)) ;
	rxDesc->ctrlLen |= ((length & RX_LEN_MASK) << RX_LEN_OFFSET);
	rxDesc->reserve = extBuf;
	//wmb();
	rxDesc->ctrlLen &= ~RX_COWN;       /* Set HW own */

}

int StarDmaRxGet(Mmac *dev /*, u32 *buffer, u32 *ctrlLen, u32 *extBuf */)
{
	int descIdx = dev->rxReadIdx;
	RxDesc *rxDesc = dev->rxdesc + descIdx;


	if (DescRxDma(rxDesc))
        return -1;
	if (DescRxEmpty(rxDesc))         /* descriptor is empty - cannot get */
        return -1;
    //if (rxDesc->reserve == 0)
        //return -1;


	if (StarDmaRxValid(rxDesc->ctrlLen)) /* process good packets only */
    {
        u32 len = StarDmaRxLength(rxDesc->ctrlLen); /* ignore Ethernet CRC bytes */
        TR(  ".");
		NetReceive((uchar*)rxDesc->buffer, len);
    }

	rxDesc->vtag = 0;
	rxDesc->reserve = 0;
	rxDesc->ctrlLen &= (RX_COWN | RX_EOR) ;
	rxDesc->ctrlLen |= ((RX_PACKET_BUFFER_SIZE & RX_LEN_MASK) << RX_LEN_OFFSET);
	rxDesc->ctrlLen &= ~RX_COWN;       /* Set HW own */
    HalFlushInvalidateDCache();
	dev->rxReadIdx = DescRxLast(rxDesc) ? 0 : descIdx + 1;


	return descIdx;

}

///////////////////////////////////////////////


void   MmacRxDescInit(struct eth_device *dev, Private *pr, Mmac *mmac )
{
  int i = 0;
   /* prepare recieve descriptors */
  for(i = 0; i<NUM_RX_DESC ;i++)
  {
    if( pbuf == NULL )
    {
      TR("Mmac::(%s) - no memory for sk_buff\n", dev->name );
      break;
    }
    /* do not skb_reserve( skb, 2 ) because rx buffer must be 4-bytes aligned */
    //r = MmacDmaRxSet( mmac, virt_to_phys(skb->tail), skb_tailroom(skb), (u32)skb );
    StarDmaRxSet( mmac, (u32)(pbuf+(i*RX_PACKET_BUFFER_SIZE)), RX_PACKET_BUFFER_SIZE, (u32)0,i );
    TR(  "Mmac::StarDmaRxSet OK  \n") ;
  }

 }


void MmacDmaRxStart( Mmac *Dev )
{
   vWriteMAC(RW_RX_DMA_CTRL, 1 );
}

void MmacDmaRxStop( Mmac *Dev )
{
  vWriteMAC(RW_RX_DMA_CTRL, (UINT32)0x01 << 1);
}

void MmacDmaTxStart( Mmac *Dev )
{
  vWriteMAC(RW_TX_DMA_CTRL, 1);		  // Start Tx
}

void MmacDmaTxStop( Mmac *Dev )
{
  vWriteMAC(RW_TX_DMA_CTRL, (UINT32)0x01 << 1);
}

void MmacDmaRxResume( Mmac *Dev )
{
  vWriteMAC(RW_RX_DMA_CTRL, (UINT32)0x01 << 2);
}

void MmacDmaTxResume( Mmac *Dev )
{
  vWriteMAC(RW_TX_DMA_CTRL, (UINT32)0x01 << 2);
}



/**********************************************************
 * DMA engine interrupt handling functions
 **********************************************************/
void MmacDmaIntEnable( Mmac *Dev )
{
  vWriteMAC(RW_INT_MASK, 0);               	// enable interrupts
}

void MmacDmaIntDisable( Mmac *Dev )
{
 vWriteMAC(RW_INT_MASK, DMA_INT_ENABLE_ALL);				  // disable interrupts
}

void MmacDmaIntClear( Mmac *Dev )
{
  //MmacWriteDmaReg( Dev, DmaStatus, MmacReadDmaReg( Dev, DmaStatus) );    /* clear all interrupt requests */
}

void GetHWMacAddr(u8  macAddr[6])
{

    macAddr[0] = (dReadMAC(RW_MY_MAC_H) >>8) & 0xff;
    macAddr[1] = dReadMAC(RW_MY_MAC_H) & 0xff;
    macAddr[2] = (dReadMAC(RW_MY_MAC_L) >>24) & 0xff;
    macAddr[3] = (dReadMAC(RW_MY_MAC_L) >>16) & 0xff;
    macAddr[4] = (dReadMAC(RW_MY_MAC_L) >>8) & 0xff;
    macAddr[5] = dReadMAC(RW_MY_MAC_L) & 0xff;
}

//***********************************************************
// 1. initial chip and board configuration.
// 2. initialize eth_device
//        -function pointer.
//        -priv date
// 3. register eth_device
// 4. mac addr...

int mmac_initialize(bd_t * bis)
{
    struct eth_device *dev;
    Private *pr = NULL;
	Private *tempPr = NULL;
    u32 macBase    = MAC_BASE;
    u32 dmaBase    = DMA_BASE;
    u32 phyAddr    = DEFAULT_PHY_ADDRESS;
    u8  macAddr[6] = DEFAULT_MAC_ADDRESS;

    TR(  "init MMAC driver\n");

    hwcfgsetup();
    if((0==dReadMAC(RW_MY_MAC_H)) && (0==dReadMAC(RW_MY_MAC_L)))
    {
        return -1;
    }
    else
        GetHWMacAddr(macAddr);

    dev = (struct eth_device *) malloc (sizeof *dev);
    if (!dev) return -1;//ENOMEM;

    if (!pbufOrg)
    {
        pbufOrg = malloc((NUM_RX_DESC *RX_PACKET_BUFFER_SIZE) + DMA_ALIGN_SIZE);
        if (!pbufOrg)
        {
            TR("Cannot allocate rx buffers\n");
			free(dev);
            return -1;
        }
	    //get align buffer.
        pbuf = (uchar*)(((u32)pbufOrg + (u32)(DMA_ALIGN_SIZE-1)) & ~((u32)(DMA_ALIGN_SIZE-1)));
    }
    pr = malloc( sizeof(Private) + DMA_ALIGN_SIZE*2);
    if( pr == NULL )
	{
		free(dev);
		return -1;//ENOMEM;
    }

    TR("priv : 0x%08x\n",(u32)pr);
	tempPr = pr;
    if((u32)pr & (u32)0x0F)
    {
        pr = (Private*)(((u32)pr + (u32)(DMA_ALIGN_SIZE-1)) & ~((u32)(DMA_ALIGN_SIZE-1)));
		free(tempPr);
    }

//************************
//eth dev info initial
//    dev->name[0] = 'M';
    dev->priv = (void *) pr;//devno;
    //dev->iobase = (int)bus_to_phys(iobase);
    dev->init = mmac_init;
    dev->halt = mmac_stop;
    dev->send = mmac_send;
    dev->recv = mmac_receive;

    eth_register (dev);

    phyAddr = u8MacPhyAddrGet();
    TR(  "Mmac::PhyAddr=%x \n",phyAddr);

    MmacInit( &pr->mmac, 0, macBase, dmaBase, phyAddr ); /* init Mmac internal data */

    memcpy( dev->enetaddr, macAddr, 6 );       /* Set MAC address */
    TR(  "Mmac::initialize \n");
#ifdef CC_UBOOT_2011_12
     phy_mtk_reset(&pr->mmac);
#else
	phy_reset(&pr->mmac);
#endif

    return 0;
}

static int mmac_receive( struct eth_device *dev )       /* handle received packets */
{
  Private *pr = NULL;
  Mmac *tc = NULL;
  int r = 0;
  int len = 0;
  RxDesc *rxDesc;

  if(dev == NULL)
     return -1;

  pr = (Private *)dev->priv;
  if(pr == NULL)
        return -1;

  tc = &pr->mmac;
  if(tc == NULL)
      return -1;

  rxDesc = tc->rxdesc;
  //HalInvalidateDCacheSingleLine(rxDesc);
  //HalInvalidateDCacheSingleLine(rxDesc+32);


  do   /* handle recieve descriptors */
  {
    HalFlushInvalidateDCache();
	r = StarDmaRxGet(tc);
    MmacDmaRxResume(NULL);

  } while( r >= 0 );

  return len;
}

void StarNICPdSet( u32 val)
{
    u32 data;
    int retry = 0;
    #define MAX_NICPDRDY_RETRY  10000

    data = dReadMAC(RW_MAC_CFG);
    if (val)
    {
        data |= NIC_PD;
		data &= ~NIC_PD_READY;
        vWriteMAC(RW_MAC_CFG, data);
        /* wait until NIC_PD_READY and clear it */
        do
        {
            if ((data = dReadMAC(RW_MAC_CFG)) & NIC_PD_READY)
            {
                data |= NIC_PD_READY; /* clear NIC_PD_READY */
                vWriteMAC(RW_MAC_CFG, data);
                break;
            }
        } while(retry++ < MAX_NICPDRDY_RETRY);
        if (retry >= MAX_NICPDRDY_RETRY)
        {
            TR("Error NIC_PD_READY is not set in %d retries!\n", MAX_NICPDRDY_RETRY);
        }
    }
    else
    {
        data &= (~NIC_PD );
        vWriteMAC(RW_MAC_CFG, data);
    }
}



int iPhyCtrlInit(u32 enable)
{
	u32 data;

	data = FORCE_FC_TX |
	       FORCE_FC_RX |
	       FORCE_DUPLEX |
	       FORCE_SPEED_100M |
	       AN_EN;


	if (enable) /* Enable PHY auto-polling */
	{
		vWriteMAC( RW_PHY_CTRL1,
		            data  /*| ( PHY_ADDR_AUTO)*/
		          );
	} else /* Disable PHY auto-polling */
	{
		vWriteMAC( RW_PHY_CTRL1,
		            data | AUTO_POLL_DIS
		          );
	}

	return 0;
}

//modify from open(struct eth_device *dev)

static int mmac_init(struct eth_device *dev, bd_t *bis)
{
  Private *pr = NULL;
  Mmac *tc = NULL;

  if(dev == NULL)
     return -1;

//  MOD_INC_USE_COUNT;
  pr = (Private *)dev->priv;
  if(pr == NULL)
     return -1;

  tc = &pr->mmac;
  if(tc == NULL)
    return -1;
#if 0
  if(bis->bi_enetaddr)
  {
    memcpy( dev->enetaddr, bis->bi_enetaddr, 6 );       /* Set MAC address */
  }
#else
//	eth_getenv_enetaddr("ethaddr", hw_addr);
//	memcpy(dev->enetaddr, hw_addr, 6);
#endif
   //Initial Internal Phy


  TR(  "open ethernet \n" );

  check_media(dev,pr, tc);

  MmacDmaRxStop(tc);
  MmacDmaTxStop(tc);

  StarNICPdSet(0) ;

 // StarMibReset(0);

  /* set and active a timer process */
  //TR(  "Mmac:: - init DMA\n" );
  if( MmacDmaInit( tc, pr->tdesc, pr->rdesc ) != 0 )        /* Init Mmac DMA Engine */
  {
    TR(  "Mmac::open(%s) - cannot init DMA\n", dev->name );
    return -1;//ENODEV;
  }

  MmacRxDescInit(dev,pr, tc);

  if( MmacMacInit( tc, dev->enetaddr, BCmacAddr ) != 0 )       /* Init Mmac MAC module */
  {
    TR(  "Mmac::open(%s) - cannot init MAC\n", dev->name );
    return -1;//ENODEV;
  }

  //MmacDmaIntClear(tc);    /* clear interrupt requests */
  _FlushDCache();
  MmacDmaRxStart(tc);     /* start receiver */
  MmacDmaTxStart(tc);     /* start transmitter, it must go to suspend immediately */

  return 0;
}

//static int stop( struct eth_device *dev )
static void mmac_stop( struct eth_device *dev )
{
  Private *pr = NULL;
  Mmac *tc = NULL;

  if(dev == NULL)
       return;

  TR(  "stop()\n");
//  MOD_DEC_USE_COUNT;
  pr = (Private *)dev->priv;
  if(pr == NULL)
        return;

  tc = &pr->mmac;
  if(tc == NULL)
     return;

  MmacDmaIntDisable(tc);  /* disable all interrupts */
  MmacDmaRxStop(tc);      /* stop receiver, take ownership of all rx descriptors */
  MmacDmaTxStop(tc);      /* stop transmitter, take ownership of all tx descriptors */
  StarNICPdSet(1) ;

  return;
}

//static int hard_start_xmit( struct sk_buff *skb, struct eth_device *dev )
static int mmac_send(struct eth_device *dev, volatile void *packet, int length)
{
  Private *pr = NULL;
  Mmac *tc = NULL;

  int r = 0;
  //int debug_count;
  //int byte_align_length;
  if(dev == NULL)
      return -1;//EBUSY;
  if(packet == NULL)
     return -1;



  pr = (Private *)dev->priv;
  if(pr == NULL)
     return -1;//EBUSY;

  tc = &pr->mmac;
  if(tc == NULL)
     return -1;//EBUSY;



 if(length > ETH_MAX_FRAME_SIZE)
 {
	 TR(  "Mmac::send (%s) - length too large \n", dev->name );
	 return -1;//EBUSY;
 }

  r = StarDmaTxSet( tc, (u32)packet , length, (u32)packet );
  HalFlushInvalidateDCache();
  MmacDmaTxResume(NULL);

  return r ;

}

#ifdef CC_UBOOT_2011_12
static int phy_mtk_reset(Mmac *Dev)
#else
static int phy_reset(Mmac *Dev)
#endif
{

	INT32 i4Ret;

    i4Ret = i4MacPhyWrite(Dev->phyAddr, MII_PHY_CTRL_REG, SW_RESET);
    if(i4Ret != NET_DRV_OK)
    {
        TR("ethernet PHY reset fail \n");
    }

    return(i4Ret);
}



int check_link (struct eth_device *dev, Mmac *mmac)
{
    int re_link = 0;
    int cur_link;
//    int prev_link;// = netif_carrier_ok(dev);

	cur_link = (dReadMAC(RW_PHY_CTRL1)& LINK_ST) ? TRUE : FALSE;

	if (cur_link && (!mmac->link_status))
	{
        TR(  "  - link up\n");
        mmac->link_status = cur_link;
        re_link = 1;
	}
    else if (!cur_link)
	{
        TR(  "  - no link!\n");
        mmac->link_status = cur_link;
    }

 	return cur_link;

}


void check_media(struct eth_device *dev, Private *pr, Mmac *mmac)
{
  if (check_link(dev, mmac) == 0)
  {
    TR("check_media fail \n");
  	return;
  }


 if(dReadMAC(RW_PHY_CTRL1)& DULPLEX_ST)
 {
   mmac->full_duplex = TRUE;
 }
 else
 {
   mmac->full_duplex = FALSE;
 }


 if((dReadMAC(RW_PHY_CTRL1)& SPEED_ST) == SPEED_ST_10M)
 {
	 mmac->speed_100 = FALSE;
	 mmac->speed_10  = TRUE ;
	 TR("10Mbps Link\n");
 }
 else if((dReadMAC(RW_PHY_CTRL1)& SPEED_ST) == SPEED_ST_100M)
 {
	 mmac->speed_100 = TRUE;
	 mmac->speed_10  = FALSE ;
	 TR("100Mbps Link\n");
 }

}


#else

#if 0
#if LINUX_VERSION_CODE == 131589
    #define eth_device device
    #define early_stop_netif_stop_queue(dev) test_and_set_bit(0, &dev->tbusy)
#else
    #define early_stop_netif_stop_queue(dev) 0
#endif
#endif


///from dev++
/**********************************************************
 * Registers - BASE ADDRESS 0 offsets
 **********************************************************/

enum GmacRegisters              /* GMAC registers, base address is BAR+GmacRegistersBase */
{
  GmacConfig         = 0x00,    /* config */
  GmacFrameFilter    = 0x04,    /* frame filter */
  GmacHashHigh       = 0x08,    /* multi-cast hash table high */
  GmacHashLow        = 0x0C,    /* multi-cast hash table low */
  GmacGmiiAddr       = 0x10,    /* GMII address */
  GmacGmiiData       = 0x14,    /* GMII data */
  GmacFlowControl    = 0x18,    /* Flow control */
  GmacVlan           = 0x1C,    /* VLAN tag */
  GmacAddr0High      = 0x40,    /* address0 high */
  GmacAddr0Low       = 0x44,    /* address0 low */
  GmacAddr1High      = 0x48,    /* address1 high */
  GmacAddr1Low       = 0x4C,    /* address1 low */
  GmacAddr2High      = 0x50,    /* address2 high */
  GmacAddr2Low       = 0x54,    /* address2 low */
  GmacAddr3High      = 0x58,    /* address3 high */
  GmacAddr3Low       = 0x5C,    /* address3 low */
  GmacAddr4High      = 0x60,    /* address4 high */
  GmacAddr4Low       = 0x64,    /* address4 low */
  GmacAddr5High      = 0x68,    /* address5 high */
  GmacAddr5Low       = 0x6C,    /* address5 low */
  GmacAddr6High      = 0x70,    /* address6 high */
  GmacAddr6Low       = 0x74,    /* address6 low */
  GmacAddr7High      = 0x78,    /* address7 high */
  GmacAddr7Low       = 0x7C,    /* address7 low */
  GmacAddr8High      = 0x80,    /* address8 high */
  GmacAddr8Low       = 0x84,    /* address8 low */
  GmacAddr9High      = 0x88,    /* address9 high */
  GmacAddr9Low       = 0x8C,    /* address9 low */
  GmacAddr10High     = 0x90,    /* address10 high */
  GmacAddr10Low      = 0x94,    /* address10 low */
  GmacAddr11High     = 0x98,    /* address11 high */
  GmacAddr11Low      = 0x9C,    /* address11 low */
  GmacAddr12High     = 0xA0,    /* address12 high */
  GmacAddr12Low      = 0xA4,    /* address12 low */
  GmacAddr13High     = 0xA8,    /* address13 high */
  GmacAddr13Low      = 0xAC,    /* address13 low */
  GmacAddr14High     = 0xB0,    /* address14 high */
  GmacAddr14Low      = 0xB4,    /* address14 low */
  GmacAddr15High     = 0xB8,    /* address15 high */
  GmacAddr15Low      = 0xBC,    /* address15 low */
};

enum DmaRegisters             /* DMA engine registers, base address is BAR+DmaRegistersBase */
{
  DmaBusMode        = 0x00,    /* CSR0 - Bus Mode */
  DmaTxPollDemand   = 0x04,    /* CSR1 - Transmit Poll Demand */
  DmaRxPollDemand   = 0x08,    /* CSR2 - Receive Poll Demand */
  DmaRxBaseAddr     = 0x0C,    /* CSR3 - Receive list base address */
  DmaTxBaseAddr     = 0x10,    /* CSR4 - Transmit list base address */
  DmaStatus         = 0x14,    /* CSR5 - Dma status */
  DmaControl        = 0x18,    /* CSR6 - Dma control */
  DmaInterrupt      = 0x1C,    /* CSR7 - Interrupt enable */
  DmaMissedFr       = 0x20,    /* CSR8 - Missed Frame Counter */
  DmaTxCurrAddr     = 0x50,    /* CSR20 - Current host transmit buffer address */
  DmaRxCurrAddr     = 0x54,    /* CSR21 - Current host receive buffer address */
};

/**********************************************************
 * GMAC Network interface registers
 **********************************************************/

enum GmacConfigReg      /* GMAC Config register layout */
{                                            /* Bit description                      R/W   Reset value */
  GmacWatchdogDisable      = 0x00800000,     /* Disable watchdog timer               RW                */
  GmacWatchdogEnable       = 0,              /* Enable watchdog timer                          0       */

  GmacJabberDisable        = 0x00400000,     /* Disable jabber timer                 RW                */
  GmacJabberEnable         = 0,              /* Enable jabber timer                            0       */

  GmacFrameBurstEnable     = 0x00200000,     /* Enable frame bursting                RW                */
  GmacFrameBurstDisable    = 0,              /* Disable frame bursting                         0       */

  GmacJumboFrameEnable     = 0x00100000,     /* Enable jumbo frame                   RW                */
  GmacJumboFrameDisable    = 0,              /* Disable jumbo frame                            0       */

// CHANGE: Added on 07/28 SNPS
  GmacInterFrameGap7       = 0x000E0000,     /* IFG Config7 - 40 bit times           RW        000     */
  GmacInterFrameGap6       = 0x000C0000,     /* IFG Config6 - 48 bit times                             */
  GmacInterFrameGap5       = 0x000A0000,     /* IFG Config5 - 56 bit times                             */
  GmacInterFrameGap4       = 0x00080000,     /* IFG Config4 - 64 bit times                             */
  GmacInterFrameGap3       = 0x00040000,     /* IFG Config3 - 72 bit times                             */
  GmacInterFrameGap2       = 0x00020000,     /* IFG Config2 - 80 bit times                             */
  GmacInterFrameGap1       = 0x00010000,     /* IFG Config1 - 88 bit times                             */
  GmacInterFrameGap0       = 000,            /* IFG Config0 - 96 bit times                             */

  GmacSelectMii            = 0x00008000,     /* Select MII mode                      RW                */
  GmacSelectGmii           = 0,              /* Select GMII mode                               0       */

//  CHANGE: Commented as Endian mode is not register configurable
//  GmacBigEndian            = 0x00004000,     /* Big endian mode                    RW                */
//  GmacLittleEndian         = 0,              /* Little endian                                0       */

  GmacFESEnable            = 0x00004000,     /* 100Mbps                             RW                */
  GmacFESDisable           = 0,                    /* 10Mbps                                0       */

  GmacDisableRxOwn         = 0x00002000,     /* Disable receive own packets          RW                */
  GmacEnableRxOwn          = 0,              /* Enable receive own packets                     0       */

  GmacLoopbackOn           = 0x00001000,     /* Loopback mode                        RW                */
  GmacLoopbackOff          = 0,              /* Normal mode                                    0       */

  GmacFullDuplex           = 0x00000800,     /* Full duplex mode                     RW                */
  GmacHalfDuplex           = 0,              /* Half duplex mode                               0       */

  GmacRetryDisable         = 0x00000200,     /* Disable retransmission               RW                */
  GmacRetryEnable          = 0,              /* Enable retransmission                          0       */

//  CHANGE: Commented as Pad / CRC strip is one single bit
//  GmacPadStripEnable       = 0x00000100,     /* Pad stripping enable               RW                */
//  GmacPadStripDisable      = 0,              /* Pad stripping disable                        0       */

// CHANGE: 07/28 renamed GmacCrcStrip* GmacPadCrcStrip*
  GmacPadCrcStripEnable    = 0x00000080,     /* Pad / Crc stripping enable           RW                */
  GmacPadCrcStripDisable   = 0,              /* Pad / Crc stripping disable                    0       */

  GmacBackoffLimit3        = 0x00000060,     /* Back-off limit                       RW                */
  GmacBackoffLimit2        = 0x00000040,     /*                                                        */
  GmacBackoffLimit1        = 0x00000020,     /*                                                        */
  GmacBackoffLimit0        = 00,             /*                                                00      */

  GmacDeferralCheckEnable  = 0x00000010,     /* Deferral check enable                RW                */
  GmacDeferralCheckDisable = 0,              /* Deferral check disable                         0       */

  GmacTxEnable             = 0x00000008,     /* Transmitter enable                   RW                */
  GmacTxDisable            = 0,              /* Transmitter disable                            0       */

  GmacRxEnable             = 0x00000004,     /* Receiver enable                      RW                */
  GmacRxDisable            = 0,              /* Receiver disable                               0       */

  GmacEnable             = 0x00000001,     /* Gmac enable                      RW                */
  GmacDisable            = 0,              /* Gmac disable                               0       */

};

enum GmacFrameFilterReg /* GMAC frame filter register layout */
{
  GmacFilterOff            = 0x80000000,     /* Receive all incoming packets         RW                */
  GmacFilterOn             = 0,              /* Receive filtered packets only                  0       */

// CHANGE: Added on 07/28 SNPS
  GmacSrcAddrFilterEnable  = 0x00000200,     /* Source Address Filter enable         RW                */
  GmacSrcAddrFilterDisable = 0,              /*                                                0       */

// CHANGE: Added on 07/28 SNPS
  GmacSrcInvAddrFilterEn   = 0x00000100,     /* Inverse Source Address Filter enable RW                */
  GmacSrcInvAddrFilterDis  = 0,              /*                                                0       */

// CHANGE: Changed the control frame config (07/28)
  GmacPassControl3         = 0x000000C0,     /* Forwards control frames that pass AF RW                */
  GmacPassControl2         = 0x00000080,     /* Forwards all control frames                            */
  GmacPassControl1         = 0x00000040,     /* Does not pass control frames                           */
  GmacPassControl0         = 00,             /* Does not pass control frames                   00      */

  GmacBroadcastDisable     = 0x00000020,     /* Disable reception of broadcast frames RW               */
  GmacBroadcastEnable      = 0,              /* Enable broadcast frames                        0       */

  GmacMulticastFilterOff   = 0x00000010,     /* Pass all multicast packets           RW                */
  GmacMulticastFilterOn    = 0,              /* Pass filtered multicast packets                0       */

// CHANGE: Changed to Dest Addr Filter Inverse (07/28)
  GmacDestAddrFilterInv    = 0x00000008,     /* Inverse filtering for DA             RW                */
  GmacDestAddrFilterNor    = 0,              /* Normal filtering for DA                        0       */

// CHANGE: Changed to Multicast Hash filter (07/28)
  GmacMcastHashFilterOn    = 0x00000004,     /* perfom multicast hash filtering      RW                */
  GmacMcastHashFilterOff   = 0,              /* perfect filtering only                         0       */

// CHANGE: Changed to Unicast Hash filter (07/28)
  GmacUcastHashFilterOn    = 0x00000002,     /* Unicast Hash filtering only          RW                */
  GmacUcastHashFilterOff   = 0,              /* perfect filtering only                         0       */

  GmacPromiscuousModeOn    = 0x00000001,     /* Receive all valid packets            RW                */
  GmacPromiscuousModeOff   = 0,              /* Receive filtered packets only                  0       */
};

enum GmacGmiiAddrReg      /* GMII address register layout */
{
  GmiiDevMask    = 0x0000F800,     /* GMII device address */
  GmiiDevShift   = 11,

  GmiiRegMask    = 0x000007C0,     /* GMII register */
  GmiiRegShift   = 6,

// CHANGED: 3-bit config instead of older 2-bit (07/28)
  GmiiAppClk5    = 0x00000014,     /* Application Clock Range 250-300 MHz */
  GmiiAppClk4    = 0x00000010,     /*                         150-250 MHz */
  GmiiAppClk3    = 0x0000000C,     /*                         35-60 MHz */
  GmiiAppClk2    = 0x00000008,     /*                         20-35 MHz */
  GmiiAppClk1    = 0x00000004,     /*                         100-150 MHz */
  GmiiAppClk0    = 00,             /*                         60-100 MHz */

  GmiiWrite      = 0x00000002,     /* Write to register */
  GmiiRead       = 0,              /* Read from register */

  GmiiBusy       = 0x00000001,     /* GMII interface is busy */
};

enum GmacGmiiDataReg      /* GMII address register layout */
{
  GmiiDataMask   = 0x0000FFFF,     /* GMII Data */
};

enum GmacFlowControlReg  /* GMAC flow control register layout */
{                                          /* Bit description                        R/W   Reset value */
  GmacPauseTimeMask        = 0xFFFF0000,   /* PAUSE TIME field in the control frame  RW      0000      */
  GmacPauseTimeShift       = 16,

// CHANGED: Added on (07/28)
  GmacPauseLowThresh3      = 0x00000030,   /* threshold for pause tmr 256 slot time  RW        00      */
  GmacPauseLowThresh2      = 0x00000020,   /*                         144 slot time                    */
  GmacPauseLowThresh1      = 0x00000010,   /*                         28  slot time                    */
  GmacPauseLowThresh0      = 00,           /*                         4   slot time                    */

  GmacUnicastPauseFrameOn  = 0x00000008,   /* Detect pause frame with unicast addr.  RW                */
  GmacUnicastPauseFrameOff = 0,            /* Detect only pause frame with multicast addr.     0       */

  GmacRxFlowControlEnable  = 0x00000004,   /* Enable Rx flow control                 RW                */
  GmacRxFlowControlDisable = 0,            /* Disable Rx flow control                          0       */

  GmacTxFlowControlEnable  = 0x00000002,   /* Enable Tx flow control                 RW                */
  GmacTxFlowControlDisable = 0,            /* Disable flow control                             0       */

  GmacSendPauseFrame       = 0x00000001,   /* send pause frame                       RW        0       */
};

/**********************************************************
 * DMA Engine registers
 **********************************************************/

enum DmaBusModeReg         /* DMA bus mode register */
{                                         /* Bit description                        R/W   Reset value */
// CHANGED: Commented as not applicable (07/28)
//  DmaBigEndianDesc        = 0x00100000,   /* Big endian data buffer descriptors   RW                */
//  DmaLittleEndianDesc     = 0,            /* Little endian data descriptors                 0       */

// CHANGED: Added on 07/28
  DmaFixedBurstEnable     = 0x00010000,   /* Fixed Burst SINGLE, INCR4, INCR8 or INCR16               */
  DmaFixedBurstDisable    = 0,            /*             SINGLE, INCR                         0       */

  DmaBurstLength32        = 0x00002000,   /* Dma burst length = 32                  RW                */
  DmaBurstLength16        = 0x00001000,   /* Dma burst length = 16                                    */
  DmaBurstLength8         = 0x00000800,   /* Dma burst length = 8                                     */
  DmaBurstLength4         = 0x00000400,   /* Dma burst length = 4                                     */
  DmaBurstLength2         = 0x00000200,   /* Dma burst length = 2                                     */
  DmaBurstLength1         = 0x00000100,   /* Dma burst length = 1                                     */
  DmaBurstLength0         = 0x00000000,   /* Dma burst length = 0                             0       */

// CHANGED: Commented as not applicable (07/28)
//  DmaBigEndianData        = 0x00000080,   /* Big endian data buffers              RW                */
//  DmaLittleEndianData     = 0,            /* Little endian data buffers                     0       */

  DmaDescriptorSkip16     = 0x00000040,   /* number of dwords to skip               RW                */
  DmaDescriptorSkip8      = 0x00000020,   /* between two unchained descriptors                        */
  DmaDescriptorSkip4      = 0x00000010,   /*                                                          */
  DmaDescriptorSkip2      = 0x00000008,   /*                                                          */
  DmaDescriptorSkip1      = 0x00000004,   /*                                                          */
  DmaDescriptorSkip0      = 0,            /*                                                  0       */

  DmaResetOn              = 0x00000001,   /* Reset DMA engine                       RW                */
  DmaResetOff             = 0,            /*                                                  0       */
};

enum DmaStatusReg         /* DMA Status register */
{                                         /* Bit description                        R/W   Reset value */
// CHANGED: Added on 07/28
  DmaLineIntfIntr         = 0x04000000,   /* Line interface interrupt               R         0       */
// CHANGED: Added on 07/28
  DmaErrorBit2            = 0x02000000,   /* err. 0-data buffer, 1-desc. access     R         0       */
  DmaErrorBit1            = 0x01000000,   /* err. 0-write trnsf, 1-read transfr     R         0       */
  DmaErrorBit0            = 0x00800000,   /* err. 0-Rx DMA, 1-Tx DMA                R         0       */

  DmaTxState              = 0x00700000,   /* Transmit process state                 R         000     */
  DmaTxStopped            = 0x00000000,   /* Stopped                                                  */
  DmaTxFetching           = 0x00100000,   /* Running - fetching the descriptor                        */
  DmaTxWaiting            = 0x00200000,   /* Running - waiting for end of transmission                */
  DmaTxReading            = 0x00300000,   /* Running - reading the data from memory                   */
  DmaTxSuspended          = 0x00600000,   /* Suspended                                                */
  DmaTxClosing            = 0x00700000,   /* Running - closing descriptor                             */

  DmaRxState              = 0x000E0000,   /* Receive process state                  R         000     */
  DmaRxStopped            = 0x00000000,   /* Stopped                                                  */
  DmaRxFetching           = 0x00020000,   /* Running - fetching the descriptor                        */
// CHANGED: Commented as not applicable (07/28)
//  DmaRxChecking           = 0x00040000,   /* Running - checking for end of packet                   */
  DmaRxWaiting            = 0x00060000,   /* Running - waiting for packet                             */
  DmaRxSuspended          = 0x00080000,   /* Suspended                                                */
  DmaRxClosing            = 0x000A0000,   /* Running - closing descriptor                             */
// CHANGED: Commented as not applicable (07/28)
//  DmaRxFlushing           = 0x000C0000,   /* Running - flushing the current frame                   */
  DmaRxQueuing            = 0x000E0000,   /* Running - queuing the recieve frame into host memory     */

  DmaIntNormal            = 0x00010000,   /* Normal interrupt summary               RW        0       */
  DmaIntAbnormal          = 0x00008000,   /* Abnormal interrupt summary             RW        0       */

  DmaIntBusError          = 0x00002000,   /* Fatal bus error (Abnormal)             RW        0       */
  DmaIntRxWdogTO          = 0x00000200,   /* Receive Watchdog Timeout (Abnormal)    RW        0       */
  DmaIntRxStopped         = 0x00000100,   /* Receive process stopped (Abnormal)     RW        0       */
  DmaIntRxNoBuffer        = 0x00000080,   /* Receive buffer unavailable (Abnormal)  RW        0       */
  DmaIntRxCompleted       = 0x00000040,   /* Completion of frame reception (Normal) RW        0       */
  DmaIntTxUnderflow       = 0x00000020,   /* Transmit underflow (Abnormal)          RW        0       */
// CHANGED: Added on 07/28
  DmaIntRcvOverflow       = 0x00000010,   /* Receive Buffer overflow interrupt      RW        0       */
  DmaIntTxJabberTO        = 0x00000008,   /* Transmit Jabber Timeout (Abnormal)     RW        0       */
  DmaIntTxNoBuffer        = 0x00000004,   /* Transmit buffer unavailable (Normal)   RW        0       */
  DmaIntTxStopped         = 0x00000002,   /* Transmit process stopped (Abnormal)    RW        0       */
  DmaIntTxCompleted       = 0x00000001,   /* Transmit completed (Normal)            RW        0       */
};

enum DmaControlReg        /* DMA control register */
{                                         /* Bit description                        R/W   Reset value */
  DmaTxEnable             = 0x00200000,   /*                                        RW        0       */
  DmaTxStart              = 0x00002000,   /* Start/Stop transmission                RW        0       */
// CHANGED: Added on 07/28
  DmaFwdErrorFrames       = 0x00000080,   /* Forward error frames                   RW        0       */
  DmaFwdUnderSzFrames     = 0x00000040,   /* Forward undersize frames               RW        0       */
//  DmaTxSecondFrame        = 0x00000004,   /* Operate on second frame                RW        0       */
  DmaRxStart              = 0x00000002,   /* Start/Stop reception                   RW        0       */
};

enum  DmaInterruptReg     /* DMA interrupt enable register */
{                                         /* Bit description                        R/W   Reset value */
  DmaIeNormal            = DmaIntNormal     ,   /* Normal interrupt enable                 RW        0       */
  DmaIeAbnormal          = DmaIntAbnormal   ,   /* Abnormal interrupt enable               RW        0       */
  DmaIeBusError          = DmaIntBusError   ,   /* Fatal bus error enable                  RW        0       */
  DmaIeRxWdogTO          = DmaIntRxWdogTO   ,   /* Receive Watchdog Timeout enable         RW        0       */
  DmaIeRxStopped         = DmaIntRxStopped  ,   /* Receive process stopped enable          RW        0       */
  DmaIeRxNoBuffer        = DmaIntRxNoBuffer ,   /* Receive buffer unavailable enable       RW        0       */
  DmaIeRxCompleted       = DmaIntRxCompleted,   /* Completion of frame reception enable    RW        0       */
  DmaIeTxUnderflow       = DmaIntTxUnderflow,   /* Transmit underflow enable               RW        0       */
// CHANGED: Added on 07/28
  DmaIeRxOverflow        = DmaIntRcvOverflow,   /* Receive Buffer overflow interrupt       RW        0       */
  DmaIeTxJabberTO        = DmaIntTxJabberTO ,   /* Transmit Jabber Timeout enable          RW        0       */
  DmaIeTxNoBuffer        = DmaIntTxNoBuffer ,   /* Transmit buffer unavailable enable      RW        0       */
  DmaIeTxStopped         = DmaIntTxStopped  ,   /* Transmit process stopped enable         RW        0       */
  DmaIeTxCompleted       = DmaIntTxCompleted,   /* Transmit completed enable               RW        0       */
};

/**********************************************************
 * DMA Engine descriptors
 **********************************************************/

enum DmaDescriptorStatus    /* status word of DMA descriptor */
{
  DescOwnByDma          = 0x80000000,   /* Descriptor is owned by DMA engine  */
// CHANGED: Added on 07/29
  DescDAFilterFail      = 0x40000000,   /* Rx - DA Filter Fail for the received frame        E  */
  DescFrameLengthMask   = 0x3FFF0000,   /* Receive descriptor frame length */
  DescFrameLengthShift  = 16,

  DescError             = 0x00008000,   /* Error summary bit  - OR of the following bits:    v  */

  DescRxTruncated       = 0x00004000,   /* Rx - no more descriptors for receive frame        E  */
// CHANGED: Added on 07/29
  DescSAFilterFail      = 0x00002000,   /* Rx - SA Filter Fail for the received frame        E  */
/* added by reyaz */
  DescRxLengthError	= 0x00001000,   /* Rx - frame size not matching with length field    E  */
  DescRxDamaged         = 0x00000800,   /* Rx - frame was damaged due to buffer overflow     E  */
// CHANGED: Added on 07/29
  DescRxVLANTag         = 0x00000400,   /* Rx - received frame is a VLAN frame               I  */
  DescRxFirst           = 0x00000200,   /* Rx - first descriptor of the frame                I  */
  DescRxLast            = 0x00000100,   /* Rx - last descriptor of the frame                 I  */
  DescRxLongFrame       = 0x00000080,   /* Rx - frame is longer than 1518 bytes              E  */
  DescRxCollision       = 0x00000040,   /* Rx - late collision occurred during reception     E  */
  DescRxFrameEther      = 0x00000020,   /* Rx - Frame type - Ethernet, otherwise 802.3          */
  DescRxWatchdog        = 0x00000010,   /* Rx - watchdog timer expired during reception      E  */
  DescRxMiiError        = 0x00000008,   /* Rx - error reported by MII interface              E  */
  DescRxDribbling       = 0x00000004,   /* Rx - frame contains noninteger multiple of 8 bits    */
  DescRxCrc             = 0x00000002,   /* Rx - CRC error                                    E  */

  DescTxTimeout         = 0x00004000,   /* Tx - Transmit jabber timeout                      E  */
// CHANGED: Added on 07/29
  DescTxFrameFlushed    = 0x00002000,   /* Tx - DMA/MTL flushed the frame due to SW flush    I  */
  DescTxLostCarrier     = 0x00000800,   /* Tx - carrier lost during tramsmission             E  */
  DescTxNoCarrier       = 0x00000400,   /* Tx - no carrier signal from the tranceiver        E  */
  DescTxLateCollision   = 0x00000200,   /* Tx - transmission aborted due to collision        E  */
  DescTxExcCollisions   = 0x00000100,   /* Tx - transmission aborted after 16 collisions     E  */
  DescTxVLANFrame       = 0x00000080,   /* Tx - VLAN-type frame                                 */
  DescTxCollMask        = 0x00000078,   /* Tx - Collision count                                 */
  DescTxCollShift       = 3,
  DescTxExcDeferral     = 0x00000004,   /* Tx - excessive deferral                           E  */
  DescTxUnderflow       = 0x00000002,   /* Tx - late data arrival from the memory            E  */
  DescTxDeferred        = 0x00000001,   /* Tx - frame transmision deferred                      */
};

enum DmaDescriptorLength    /* length word of DMA descriptor */
{
  DescTxIntEnable       = 0x80000000,   /* Tx - interrupt on completion                         */
  DescTxLast            = 0x40000000,   /* Tx - Last segment of the frame                       */
  DescTxFirst           = 0x20000000,   /* Tx - First segment of the frame                      */
  DescTxDisableCrc      = 0x04000000,   /* Tx - Add CRC disabled (first segment only)           */

  DescEndOfRing         = 0x02000000,   /* End of descriptors ring                              */
  DescChain             = 0x01000000,   /* Second buffer address is chain address               */
  DescTxDisablePadd	= 0x00800000,   /* disable padding, added by - reyaz */

  DescSize2Mask         = 0x003FF800,   /* Buffer 2 size                                        */
  DescSize2Shift        = 11,
  DescSize1Mask         = 0x000007FF,   /* Buffer 1 size                                        */
  DescSize1Shift        = 0,
};

/**********************************************************
 * Initial register values
 **********************************************************/

enum InitialRegisters
{
//  GmacConfigInitFdx1000       /* Full-duplex mode with perfect filter on */
  GmacConfigInitFdx100       /* Full-duplex mode with perfect filter on */
                          = GmacWatchdogEnable | GmacJabberEnable         | GmacFrameBurstEnable | GmacJumboFrameDisable
// CHANGED: Removed Endian configuration, added single bit config for PAD/CRC strip,
//                          | GmacSelectGmii     | GmacEnableRxOwn          | GmacLoopbackOff
                          | GmacSelectMii
                          | GmacFESEnable
//temp loopback
                          | GmacEnableRxOwn          | GmacLoopbackOff
//loopback off                          | GmacEnableRxOwn          | GmacLoopbackOn
                          | GmacFullDuplex     | GmacRetryEnable          | GmacPadCrcStripDisable
                          | GmacBackoffLimit0  | GmacDeferralCheckDisable | GmacTxEnable          | GmacRxEnable
                          | GmacEnable
                          ,
  GmacConfigInitFdx10       /* Full-duplex mode with perfect filter on */
                          = GmacWatchdogEnable | GmacJabberEnable         | GmacFrameBurstEnable  | GmacJumboFrameDisable
// CHANGED: Removed Endian configuration, added single bit config for PAD/CRC strip,
//                          | GmacSelectMii      | GmacEnableRxOwn          | GmacLoopbackOff
                          | GmacSelectMii
                          | GmacFESDisable
//temp loopback
                          | GmacEnableRxOwn          | GmacLoopbackOff
//loopback off                          | GmacEnableRxOwn          | GmacLoopbackOn
                          | GmacFullDuplex     | GmacRetryEnable          | GmacPadCrcStripDisable
                          | GmacBackoffLimit0  | GmacDeferralCheckDisable | GmacTxEnable          | GmacRxEnable
                          | GmacEnable
                          ,
  GmacFrameFilterInitFdx  /* Full-duplex mode */
// CHANGED: Pass control config, dest addr filter normal, added source address filter, multicast & unicast
// Hash filter.
                          = GmacFilterOn          | GmacPassControl0   | GmacBroadcastEnable |  GmacSrcAddrFilterDisable
/*                        = GmacFilterOff         | GmacPassControlOff | GmacBroadcastEnable */
                          | GmacMulticastFilterOn | GmacDestAddrFilterNor | GmacMcastHashFilterOff
                          | GmacPromiscuousModeOff | GmacUcastHashFilterOff,
//                          | GmacPromiscuousModeOn | GmacUcastHashFilterOff,
  GmacFlowControlInitFdx  /* Full-duplex mode */
                          = GmacUnicastPauseFrameOff | GmacRxFlowControlEnable | GmacTxFlowControlEnable,

  GmacGmiiAddrInitFdx     /* Full-duplex mode */
                          = GmiiAppClk2,

//  GmacConfigInitHdx1000       /* Half-duplex mode with perfect filter on */
  GmacConfigInitHdx100       /* Half-duplex mode with perfect filter on */
                          = GmacWatchdogEnable | GmacJabberEnable         | GmacFrameBurstEnable  | GmacJumboFrameDisable
// CHANGED: Removed Endian configuration, added single bit config for PAD/CRC strip,
                        /*| GmacSelectMii      | GmacLittleEndian         | GmacDisableRxOwn      | GmacLoopbackOff*/
                          | GmacSelectMii
                          | GmacFESEnable
//                          | GmacSelectGmii     | GmacDisableRxOwn         | GmacLoopbackOff
                                                      | GmacDisableRxOwn         | GmacLoopbackOff
                          | GmacHalfDuplex     | GmacRetryEnable          | GmacPadCrcStripDisable
                          | GmacBackoffLimit0  | GmacDeferralCheckDisable | GmacTxEnable          | GmacRxEnable
                          | GmacEnable
                          ,

  GmacConfigInitHdx10       /* Half-duplex mode with perfect filter on */
                          = GmacWatchdogEnable | GmacJabberEnable         | GmacFrameBurstEnable  | GmacJumboFrameDisable
// CHANGED: Removed Endian configuration, added single bit config for PAD/CRC strip,
                          | GmacSelectMii
                          | GmacFESDisable
//                          | GmacSelectMii      | GmacDisableRxOwn         | GmacLoopbackOff
                                                      | GmacDisableRxOwn         | GmacLoopbackOff
                          | GmacHalfDuplex     | GmacRetryEnable          | GmacPadCrcStripDisable
                          | GmacBackoffLimit0  | GmacDeferralCheckDisable | GmacTxEnable          | GmacRxEnable
                          | GmacEnable
                          ,

  GmacFrameFilterInitHdx  /* Half-duplex mode */
                          = GmacFilterOn          | GmacPassControl0        | GmacBroadcastEnable | GmacSrcAddrFilterDisable
                          | GmacMulticastFilterOn | GmacDestAddrFilterNor   | GmacMcastHashFilterOff
                          | GmacUcastHashFilterOff| GmacPromiscuousModeOff,

  GmacFlowControlInitHdx  /* Half-duplex mode */
                          = GmacUnicastPauseFrameOff | GmacRxFlowControlDisable | GmacTxFlowControlDisable,

  GmacGmiiAddrInitHdx     /* Half-duplex mode */
                          = GmiiAppClk2,

  DmaBusModeInit          /* Little-endian mode */
// CHANGED: Removed Endian configuration
//                          = DmaFixedBurstEnable |   DmaBurstLength8   | DmaDescriptorSkip2       | DmaResetOff,
                          = DmaFixedBurstEnable |   DmaBurstLength8   | DmaResetOff,

//  DmaControlInit1000      /* 1000 Mb/s mode */
//                          = DmaTxEnable       | DmaTxSecondFrame ,

  DmaControlInit100       /* 100 Mb/s mode */
                          = DmaTxEnable,

  DmaControlInit10        /* 10 Mb/s mode */
                          = DmaTxEnable,

                          /* Interrupt groups */
  DmaIntErrorMask         = DmaIntBusError,           /* Error */
  DmaIntRxAbnMask         = DmaIntRxNoBuffer,         /* receiver abnormal interrupt */
  DmaIntRxNormMask        = DmaIntRxCompleted,        /* receiver normal interrupt   */
  DmaIntRxStoppedMask     = DmaIntRxStopped,          /* receiver stopped */
  DmaIntTxAbnMask         = DmaIntTxUnderflow,        /* transmitter abnormal interrupt */
  DmaIntTxNormMask        = DmaIntTxCompleted,        /* transmitter normal interrupt */
  DmaIntTxStoppedMask     = DmaIntTxStopped,          /* receiver stopped */

#if 0
  DmaIntEnable            = DmaIeNormal     | DmaIeAbnormal    | DmaIntErrorMask
                          | DmaIntRxAbnMask | DmaIntRxNormMask | DmaIntRxStoppedMask
                          | DmaIntTxAbnMask | DmaIntTxNormMask | DmaIntTxStoppedMask,
#else
  DmaIntEnable            = DmaIeNormal     | DmaIeAbnormal    | DmaIntErrorMask
                          | DmaIntRxAbnMask | DmaIntRxNormMask
                          | DmaIntTxAbnMask | DmaIntTxNormMask,
#endif
  DmaIntDisable           = 0,
};


/**********************************************************
 * device cfg related functions
 **********************************************************/
void hwcfgsetup(void)
{
    u32 data;
    //pinmux setup
    //MII & gpio125
    data = __raw_readl(CKGEN_VIRT + 0x404);
    data &= ~0xfff00000;
#ifdef MT539X_LAN8700
    data |= 0x11100000;                                 //use TX_ER
#else
    data |= 0x12100000;                                 //[26:24]->2 GPIO125(ONDA8)
#endif
    __raw_writel(data, CKGEN_VIRT + 0x404);  //0x12100000 [30:28], [26:24], [22:20]

    data = __raw_readl(CKGEN_VIRT + 0x408);
    data &= ~0x0000FFFF;
    data |= 0x1111;
     __raw_writel(data, CKGEN_VIRT + 0x408); //0x00001111 [14:12], [10:8], [6:4], [2:0]

    //ckgen setup, all register own by MII CKCFG
    __raw_writel(0x3200, CKGEN_VIRT + 0x264);


}

void hw_reset(void)
{
    u32 data;

#ifdef MT539X_LAN8700   //use gpio18
    //reset output ++
    data = __raw_readl(CKGEN_VIRT + 0x520);
    data &= ~0x00040000;
    data |= 0x00040000;                                 //[29]->1 GPIO125 out (ONDA8)
    __raw_writel(data, CKGEN_VIRT + 0x520);  //0x12100000 [30:28], [26:24], [22:20]
    //reset output --

    //reset low
    data = __raw_readl(CKGEN_VIRT + 0x500);
    data &= ~0x00040000;                              //[29]->0 GPIO125 low (ONDA8)
    __raw_writel(data, CKGEN_VIRT + 0x500);

    //wait
    /* wait for 10ms */
    mdelay(10);

    //reset high
    //release HW reset pin first
    data = __raw_readl(CKGEN_VIRT + 0x500);
    data |= 0x00040000;                                 //[29]->0 GPIO125 high (ONDA8)
    __raw_writel(data, CKGEN_VIRT + 0x500);

#else
    //reset output ++
    data = __raw_readl(CKGEN_VIRT + 0x52C);
    data &= ~0x20000000;
    data |= 0x20000000;                                 //[29]->1 GPIO125 out (ONDA8)
    __raw_writel(data, CKGEN_VIRT + 0x52C);  //0x12100000 [30:28], [26:24], [22:20]
    //reset output --

    //reset low
    data = __raw_readl(CKGEN_VIRT + 0x50C);
    data &= ~0x20000000;                              //[29]->0 GPIO125 low (ONDA8)
    __raw_writel(data, CKGEN_VIRT + 0x50C);

    //wait
    /* wait for 10ms */
    mdelay(10);

    //reset high
    //release HW reset pin first
    data = __raw_readl(CKGEN_VIRT + 0x50C);
    data |= 0x20000000;                                 //[29]->0 GPIO125 high (ONDA8)
    __raw_writel(data, CKGEN_VIRT + 0x50C);
#endif
}

/**********************************************************
 * Common functions
 **********************************************************/

int MmacInit( Mmac *Dev, u32 ConfigBase, u32 MacBase, u32 DmaBase, u32 PhyAddr )
{
  memset( Dev, 0, sizeof(*Dev) );

  Dev->configBase = ConfigBase;
  Dev->macBase = MacBase;
  Dev->dmaBase = DmaBase;
  Dev->phyAddr = PhyAddr;

  return 0;
}

/**********************************************************
 * MAC module functions
 **********************************************************/

static int Hash( u8 addr[6] )   /* returns hash bit number for given MAC address */
{
  int i;
  u32 crc = 0xFFFFFFFF;
  u32 poly = 0xEDB88320;

  for( i=0; i<6; i++ )
  {
    int bit;
    u8 data = addr[i];
    for( bit=0; bit<8; bit++ )
    {
      int p = (crc^data) & 1;
      crc >>= 1;
      if( p != 0 ) crc ^= poly;
      data >>= 1;
    }
  }

  return (crc>>26) & 0x3F;      /* return upper 6 bits */
}

int MmacMacInit( Mmac *Dev, u8 Addr[6], u8 Broadcast[6] )
{
  u32 data;

  data = (Addr[5]<<8) | Addr[4];                            /* set our MAC address */
  MmacWriteMacReg( Dev, GmacAddr0High, data );
  data = (Addr[3]<<24) | (Addr[2]<<16) | (Addr[1]<<8) | Addr[0];
  MmacWriteMacReg( Dev, GmacAddr0Low, data );

  if(0)
  {
    int n;

    MmacWriteMacReg( Dev, GmacHashHigh, 0 );                /* clear hash table */
    MmacWriteMacReg( Dev, GmacHashLow, 0 );

    n = Hash( Broadcast );                                    /* set the broadcast address */
    if( n>=32 ) MmacSetMacReg( Dev, GmacHashHigh, 1<<(n-32) );
    else MmacSetMacReg( Dev, GmacHashLow, 1<<n );
  }
  MmacSetMacReg( Dev, GmacConfig, GmacEnable ); /* set init values of config registers */
  MmacClearMacReg(Dev, GmacConfig, GmacTxEnable |GmacRxEnable | GmacFullDuplex | GmacFESEnable);

  if( Dev->full_duplex)
  {
    if(Dev->speed_100)
    {
      MmacSetMacReg( Dev, GmacConfig, GmacConfigInitFdx100 ); /* set init values of config registers */
    }
    else
    {
      MmacSetMacReg( Dev, GmacConfig, GmacConfigInitFdx10 ); /* set init values of config registers with MII port */
    }

    MmacWriteMacReg( Dev, GmacFrameFilter, GmacFrameFilterInitFdx );
    MmacWriteMacReg( Dev, GmacFlowControl, GmacFlowControlInitFdx );
    MmacWriteMacReg( Dev, GmacGmiiAddr, GmacGmiiAddrInitFdx );
  }
  else
  {
    if(Dev->speed_100)
    {
      MmacSetMacReg( Dev, GmacConfig, GmacConfigInitHdx100 ); /* set init values of config registers */
    }
    else
    {
      MmacSetMacReg( Dev, GmacConfig, GmacConfigInitHdx10 ); /* set init values of config registers with MII port */
    }

    MmacWriteMacReg( Dev, GmacFrameFilter, GmacFrameFilterInitHdx );
    MmacWriteMacReg( Dev, GmacFlowControl, GmacFlowControlInitHdx );
    MmacWriteMacReg( Dev, GmacGmiiAddr, GmacGmiiAddrInitHdx );
  }

  return 0;
}

u16 MmacMiiRead( Mmac *Dev, u8 Reg )
{
  u32 addr;
  u16 data;

  addr = ((Dev->phyAddr << GmiiDevShift) & GmiiDevMask) | ((Reg << GmiiRegShift) & GmiiRegMask);
  MmacWriteMacReg( Dev, GmacGmiiAddr, (addr | GmiiAppClk4 | GmiiBusy));

  do{} while( (MmacReadMacReg( Dev, GmacGmiiAddr ) & GmiiBusy) == GmiiBusy );

  data = MmacReadMacReg( Dev, GmacGmiiData ) & 0xFFFF;
  return data;
}

void MmacMiiWrite( Mmac *Dev, u8 Reg, u16 Data )
{
  u32 addr;

  MmacWriteMacReg( Dev, GmacGmiiData, Data );

  addr = ((Dev->phyAddr << GmiiDevShift) & GmiiDevMask) | ((Reg << GmiiRegShift) & GmiiRegMask) | GmiiWrite;
  MmacWriteMacReg( Dev, GmacGmiiAddr, (addr | GmiiAppClk4 | GmiiBusy));

  do{} while( (MmacReadMacReg( Dev, GmacGmiiAddr ) & GmiiBusy) == GmiiBusy );
}

/**********************************************************
 * DMA engine functions
 **********************************************************/
void DescInit( DmaDesc *Desc, int EndOfRing )
{
  Desc->status = 0;
  Desc->length = EndOfRing ? DescEndOfRing : 0;
  Desc->length |= DescChain;
  Desc->buffer1 = 0;
//  Desc->buffer2 = 0;
  Desc->data1 = 0;
  Desc->data2 = 0;
}

static int DescLast( DmaDesc *Desc )
{
  return (Desc->length & DescEndOfRing) != 0;
}

static int DescEmpty( DmaDesc *Desc )
{
//  return (Desc->length & ~ (DescEndOfRing|DescChain)) == 0;
  return (Desc->length & DescSize1Mask) == 0;
}

static int DescDma( DmaDesc *Desc )
{
  return (Desc->status & DescOwnByDma) != 0;   /* Owned by DMA */
}
#if 0
static void DescTake( DmaDesc *Desc )   /* Take ownership */
{
  if( DescDma(Desc) )               /* if descriptor is owned by DMA engine */
  {
    Desc->status &= ~DescOwnByDma;    /* clear DMA own bit */
    Desc->status |= DescError;        /* Set error bit to mark this descriptor bad */
  }
}
#endif
int MmacDmaInit( Mmac *Dev, void *Buffer, u32 Size )
{
  //int n = Size/sizeof(DmaDesc);     /* number of descriptors that can fit into the buffer passed */
  int i;

  Dev->txCount = NUM_TX_DESC; //n/2;               /* use 1/2 for Tx descriptors and 1/2 for Rx descriptors */
  Dev->rxCount = NUM_RX_DESC; //n - Dev->txCount;

  Dev->txFreeCount = Dev->txCount;
  Dev->tx = Buffer;
  Dev->rx = Dev->tx + Dev->txCount;

  for( i=0; i<Dev->txCount; i++ ) DescInit( Dev->tx + i, i==Dev->txCount-1 );
  for( i=0; i<Dev->rxCount; i++ ) DescInit( Dev->rx + i, i==Dev->rxCount-1 );

  Dev->txNext = 0;
  Dev->txBusy = 0;
  Dev->rxNext = 0;
  Dev->rxBusy = 0;

  MmacWriteDmaReg( Dev, DmaBusMode, DmaResetOn );       /* Reset DMA engine */
  udelay(2);      /* Delay 2 usec (=50 PCI cycles on 25 Mhz) */
  MmacWriteDmaReg( Dev, DmaBusMode, DmaBusModeInit );   /* Set init register values */
#if 0
  MmacWriteDmaReg( Dev, DmaRxBaseAddr, virt_to_phys(Dev->rx) );
  MmacWriteDmaReg( Dev, DmaTxBaseAddr, virt_to_phys(Dev->tx) );
#else
  MmacWriteDmaReg( Dev, DmaRxBaseAddr, (u32)Dev->rx );
  MmacWriteDmaReg( Dev, DmaTxBaseAddr, (u32)Dev->tx );
#endif
  for( i=0; i<Dev->txCount; i++ )
  {
  	MmacDmaTxSet(Dev,0,0,0);
  }
  if( Dev->speed_100 )
  {
    MmacWriteDmaReg( Dev, DmaControl, DmaControlInit100 );
  }
  else
  {
    MmacWriteDmaReg( Dev, DmaControl, DmaControlInit10 );
  }
  MmacWriteDmaReg( Dev, DmaInterrupt, DmaIntDisable );

  return 0;
}

int MmacDmaRxSet( Mmac *Dev, u32 Buffer1, u32 Length1, u32 Data1 )
{
  int desc = Dev->rxNext;
  DmaDesc *rx = Dev->rx + desc;
  DmaDesc *prxNext;
  if( !DescEmpty(rx) ) return -1;       /* descriptor is not empty - cannot set */
#if 0
  rx->length &= DescEndOfRing;      /* clear everything */
  rx->length |= ((Length1 << DescSize1Shift) & DescSize1Mask);
  rx->buffer1 = Buffer1;
  rx->data1   = Data1;
  rx->status  = DescOwnByDma;

  Dev->rxNext = DescLast(rx) ? 0 : desc + 1;
#else
  rx->length &= DescEndOfRing;      /* clear everything */
  rx->length |= ((Length1 << DescSize1Shift) & DescSize1Mask);
  rx->length |= DescChain;
  rx->buffer1 = Buffer1;
  rx->data1   = Data1;
  prxNext = Dev->rx + (DescLast(rx) ? 0 : desc + 1);
//  rx->buffer2 = virt_to_phys(prxNext);
  rx->buffer2 = (u32)(prxNext);
  rx->status  = DescOwnByDma;

  Dev->rxNext = DescLast(rx) ? 0 : desc + 1;
#endif


  return desc;
}

int MmacDmaRxGet( Mmac *Dev, u32 *Status, u32 *Buffer1, u32 *Length1, u32 *Data1 )
{
  int desc = Dev->rxBusy;
  DmaDesc *rx = Dev->rx + desc;

//  MmacDmaDescSync(Dev, rx, sizeof(DmaDesc), DMA_FROM_DEVICE);  //invalidate cache from physical memory
  if( DescDma(rx) ) return -1;          /* descriptor is owned by DMA - cannot get */
  if( DescEmpty(rx) ) return -1;        /* descriptor is empty - cannot get */

  if( Status != 0 ) *Status = rx->status;
  if( Length1 != 0 ) *Length1 = (rx->length & DescSize1Mask) >> DescSize1Shift;
  if( Buffer1 != 0 ) *Buffer1 = rx->buffer1;
  if( Data1 != 0 ) *Data1 = rx->data1;

  DescInit( rx, DescLast(rx) );
  Dev->rxBusy = DescLast(rx) ? 0 : desc + 1;

  return desc;
}

void   MmacRxDescInit(struct eth_device *dev, Private *pr, Mmac *mmac )
{
  int r = 0;
  int i = 0;
   /* prepare recieve descriptors */
  for(i = 0; i<NUM_RX_DESC ;i++)
  {
//    struct sk_buff *skb = alloc_skb( dev->mtu + ETHERNET_PACKET_EXTRA, GFP_ATOMIC );

    if( pbuf == NULL )
    {
      printf("Mmac::(%s) - no memory for sk_buff\n", dev->name );
      break;
    }
    /* do not skb_reserve( skb, 2 ) because rx buffer must be 4-bytes aligned */
    //r = MmacDmaRxSet( mmac, virt_to_phys(skb->tail), skb_tailroom(skb), (u32)skb );
    r = MmacDmaRxSet( mmac, (u32)(pbuf+(i*RX_PACKET_BUFFER_SIZE)), RX_PACKET_BUFFER_SIZE, (u32)0 );
    TR(  "Mmac::open(%s) - set rx descriptor %d for skb %p\n", dev->name, r, pbuf+(i*RX_PACKET_BUFFER_SIZE) );
  }

//  MmacDmaDescSync(mmac, mmac->rx, mmac->rxCount*sizeof(DmaDesc), DMA_TO_DEVICE);  //flush cache to physical memory
}

int MmacDmaTxSet( Mmac *Dev, u32 Buffer1, u32 Length1, u32 Data1 )
{
  int desc = Dev->txNext;
  DmaDesc *tx = Dev->tx + desc;
  DmaDesc *ptxNext;

  if( !DescEmpty(tx) ) return -1; /* descriptor is not empty - cannot set */

  tx->length &= DescEndOfRing;      /* clear everything */
  tx->length |= ((Length1 << DescSize1Shift) & DescSize1Mask) | DescTxFirst | DescTxLast | DescTxIntEnable;
  tx->length |= DescChain;


if( Buffer1 != 0 ) tx->buffer1 = Buffer1;
if( Data1 != 0 )   tx->data1   = Data1;
  ptxNext = Dev->tx + (DescLast(tx) ? 0 : desc + 1);
//  tx->buffer2 = virt_to_phys(ptxNext);
  tx->buffer2 = (u32)ptxNext;
if( Buffer1 != 0 )  tx->status  = DescOwnByDma;

  Dev->txNext = DescLast(tx) ? 0 : desc + 1;

  return desc;
}

int MmacDmaTxGet( Mmac *Dev, u32 *Status, u32 *Buffer1, u32 *Length1, u32 *Data1 )
{
  int desc = Dev->txBusy;
  DmaDesc *tx = Dev->tx + desc;

//  MmacDmaDescSync(Dev, tx, sizeof(DmaDesc), DMA_FROM_DEVICE);  //invalidate cache from physical memory


  if( DescDma(tx) ) return -1;          /* descriptor is owned by DMA - cannot get */
  if( DescEmpty(tx) ) return -1;        /* descriptor is empty - cannot get */

  if( Status != 0 ) *Status = tx->status;
  if( Buffer1 != 0 ) *Buffer1 = tx->buffer1;
  if( Length1 != 0 ) *Length1 = (tx->length & DescSize1Mask) >> DescSize1Shift;
  if( Data1 != 0 ) *Data1 = tx->data1;

  DescInit( tx, DescLast(tx) );
  Dev->txBusy = DescLast(tx) ? 0 : desc + 1;

  return desc;
}

void MmacDmaRxStart( Mmac *Dev )
{
  MmacSetDmaReg( Dev, DmaControl, DmaRxStart );
}

void MmacDmaRxStop( Mmac *Dev )
{
//  int i;
  MmacClearDmaReg( Dev, DmaControl, DmaRxStart );
//  for( i=0; i<Dev->rxCount; i++ ) DescTake( Dev->rx + i );
}

void MmacDmaTxStart( Mmac *Dev )
{
  MmacSetDmaReg( Dev, DmaControl, DmaTxStart );
}

void MmacDmaTxStop( Mmac *Dev )
{
//  int i;
  MmacClearDmaReg( Dev, DmaControl, DmaTxStart );
//  for( i=0; i<Dev->txCount; i++ ) DescTake( Dev->tx + i );
}

void MmacDmaRxResume( Mmac *Dev )
{
  MmacWriteDmaReg( Dev, DmaRxPollDemand, 0 );
}

void MmacDmaTxResume( Mmac *Dev )
{
  MmacWriteDmaReg( Dev, DmaTxPollDemand, 0 );
}

int MmacDmaRxValid( u32 Status )
{
  return ( (Status & DescError) == 0 )      /* no errors, whole frame is in the buffer */
      && ( (Status & DescRxFirst) != 0 )
      && ( (Status & DescRxLast) != 0 );
}

// Returns 1 when the Dribbling status is set
int MmacDmaRxStatusDriblling( u32 Status )
{
  return ( (Status & DescRxDribbling) != 0 );
}

/* Returns 1 when the Crc Error is set
int MmacDmaRxCrc ( u32 Status )
{
  return ( (Status & DescRxCrc) != 0 );
}
*/

u32 MmacDmaRxLength( u32 Status )
{
  return (Status & DescFrameLengthMask) >> DescFrameLengthShift;
}

int MmacDmaRxCollisions(  u32 Status)
{
  if( Status & (DescRxDamaged | DescRxCollision) ) return 1;
  return 0;
}

int MmacDmaRxCrc(  u32 Status)
{
  if( Status & DescRxCrc ) return 1;
  return 0;
}
/* added by reyaz */
int MmacDmaRxFrame(  u32 Status)
{
  if( Status & DescRxDribbling ) return 1;
  return 0;
}

int MmacDmaRxLongFrame(  u32 Status)
{
  if( Status & DescRxLongFrame ) return 1;
  return 0;
}
int MmacDmaRxLengthError(  u32 Status)
{
  if( Status & DescRxLengthError ) return 1;
  return 0;
}
/* Test the status word if the descriptor is valid */
int MmacDmaTxValid(  u32 Status)
{
  return ( (Status & DescError) == 0 );
}

int MmacDmaTxCollisions(  u32 Status)
{
  return (Status & DescTxCollMask) >> DescTxCollShift;
}

int MmacDmaTxAborted(  u32 Status)
{
  if( Status & (DescTxLateCollision | DescTxExcCollisions )) return 1;
  return 0;
}

int MmacDmaTxCarrier(  u32 Status)
{
  if( Status & (DescTxLostCarrier | DescTxNoCarrier )) return 1;
  return 0;
}


/**********************************************************
 * DMA engine interrupt handling functions
 **********************************************************/

void MmacDmaIntEnable( Mmac *Dev )
{
  MmacWriteDmaReg( Dev, DmaInterrupt, DmaIntEnable );    /* enable interrupts */
}

void MmacDmaIntDisable( Mmac *Dev )
{
  MmacWriteDmaReg( Dev, DmaInterrupt, DmaIntDisable );    /* disable interrupts */
}

void MmacDmaIntClear( Mmac *Dev )
{
  MmacWriteDmaReg( Dev, DmaStatus, MmacReadDmaReg( Dev, DmaStatus) );    /* clear all interrupt requests */
}

u32 MmacDmaIntType( Mmac *Dev )
{
  u32 status = MmacReadDmaReg( Dev, DmaStatus );
  u32 type = 0;

  if( status & DmaIntErrorMask )      type |= MmacDmaError;
  if( status & DmaIntRxNormMask )     type |= MmacDmaRxNormal;
  if( status & DmaIntRxAbnMask )      type |= MmacDmaRxAbnormal;
  if( status & DmaIntRxStoppedMask )  type |= MmacDmaRxStopped;
  if( status & DmaIntTxNormMask )     type |= MmacDmaTxNormal;
  if( status & DmaIntTxAbnMask )      type |= MmacDmaTxAbnormal;
  if( status & DmaIntTxStoppedMask )  type |= MmacDmaTxStopped;

  MmacWriteDmaReg( Dev, DmaStatus, status );     /* clear all interrupt requests */

  return type;
}

//from dev --


//***********************************************************
// 1. initial chip and board configuration.
// 2. initialize eth_device
//        -function pointer.
//        -priv date
// 3. register eth_device
// 4. mac addr...

int mmac_initialize(bd_t * bis)
{
    struct eth_device *dev;
    Private *pr = NULL;
    u32 macBase    = MAC_BASE;
    u32 dmaBase    = DMA_BASE;
    u32 phyAddr    = DEFAULT_PHY_ADDRESS;
    u8  macAddr[6] = DEFAULT_MAC_ADDRESS;

    TR(  "init MMAC driver\n");

    //5391 pinmux, gpio, ckgen setup++
    hwcfgsetup();
    hw_reset();

    dev = (struct eth_device *) malloc (sizeof *dev);
    if (!dev) return -1;//ENOMEM;

    if (!pbufOrg)
    {
      pbufOrg = malloc((NUM_RX_DESC *RX_PACKET_BUFFER_SIZE) + DMA_ALIGN_SIZE);
      if (!pbufOrg)
      {
         printf("Cannot allocate rx buffers\n");
         return -1;
      }
	//get align buffer.
      pbuf = (uchar*)(((u32)pbufOrg + (u32)(DMA_ALIGN_SIZE-1)) & ~((u32)(DMA_ALIGN_SIZE-1)));
    }
    pr = malloc( sizeof(Private) + DMA_ALIGN_SIZE);
    if( pr == NULL ) return -1;//ENOMEM;

    TR("priv : 0x%08x\n",(u32)pr);
    if((u32)pr & (u32)0x0F)
    {
      pr = (Private*)(((u32)pr + (u32)(DMA_ALIGN_SIZE-1)) & ~((u32)(DMA_ALIGN_SIZE-1)));
    }

//************************
//eth dev info initial
//    dev->name[0] = 'M';
    dev->priv = (void *) pr;//devno;
    //dev->iobase = (int)bus_to_phys(iobase);
    dev->init = mmac_init;
    dev->halt = mmac_stop;
    dev->send = mmac_send;
    dev->recv = mmac_receive;

    eth_register (dev);

    MmacInit( &pr->mmac, 0, macBase, dmaBase, phyAddr ); /* init Mmac internal data */

//    memcpy( dev->enetaddr, macAddr, ETH_ALEN );       /* Set MAC address */
    memcpy( dev->enetaddr, macAddr, 6 );       /* Set MAC address */
    TR(  "Mmac::initialize \n");

//  MmacMiiWrite(&pr->mmac, PHY_BMCR, BMCR_RESET);
    phy_mtk_reset(&pr->mmac);

    return 0;
}

static int mmac_receive( struct eth_device *dev )       /* handle received packets */
{
  Private *pr = NULL;
  Mmac *tc = NULL;
  int r = 0;
  int len;


  if(dev == NULL)
     return -1;

  pr = (Private *)dev->priv;
  if(pr == NULL)
        return -1;

  tc = &pr->mmac;
  if(tc == NULL)
      return -1;

  do                          /* handle recieve descriptors */
  {
    u32 status;
    uchar *buffer;

    r = MmacDmaRxGet( tc, &status, (u32*)&buffer, NULL, NULL );   /* get rx descriptor content */
    if( r >= 0 && buffer != 0 )
    {
      TR(  "Mmac::receive(%s) - get rx descriptor %d for buffer %08x, status = %08x\n", dev->name, r, buffer, status );

      /* printf("mmac: Status: %08x",status); */

      if( MmacDmaRxValid(status) ) /* process good packets only */
      {
        /** Sync skb->data in cache with memory **/
        len = MmacDmaRxLength( status ) - 4; /* ignore Ethernet CRC bytes */
        NetReceive(buffer, len);
        /* printf("mmac: Len : %0d, Status: %08x",len,status); */
      }

      if( buffer != NULL )
      {
        r = MmacDmaRxSet( tc, (u32)buffer, RX_PACKET_BUFFER_SIZE, 0 );  /* put the skb to the descriptor */
        if( r < 0 )
        {
          TR(  "Mmac::receive(%s) - cannot set rx descriptor for buffer %p\n", dev->name, buffer );
        }
      }
    }

  } while( r >= 0 );
  return len;
}

//modify from open(struct eth_device *dev)
static int mmac_init(struct eth_device *dev, bd_t *bis)
{
  Private *pr = NULL;
  Mmac *tc = NULL;

  if(dev == NULL)
     return -1;

  TR(  "open(%s)\n", dev->name );

//  MOD_INC_USE_COUNT;
  pr = (Private *)dev->priv;
  if(pr == NULL)
     return -1;

  tc = &pr->mmac;
  if(tc == NULL)
    return -1;

  if(bis->bi_enetaddr)
  {
    memcpy( dev->enetaddr, bis->bi_enetaddr, 6 );       /* Set MAC address */
  }
  if(check_media(dev,pr, tc) == 0)
    return -1;

  /* set and active a timer process */
  TR(  "Mmac::open(%s) - init DMA\n", dev->name );
  if( MmacDmaInit( tc, pr->desc, sizeof( pr->desc ) ) != 0 )        /* Init Mmac DMA Engine */
  {
    printf(  "Mmac::open(%s) - cannot init DMA\n", dev->name );
    return -1;//ENODEV;
  }
  MmacRxDescInit(dev,pr, tc);

  TR(  "Mmac::open(%s) - init MAC\n", dev->name );
  if( MmacMacInit( tc, dev->enetaddr, BCmacAddr ) != 0 )       /* Init Mmac MAC module */
  {
    printf(  "Mmac::open(%s) - cannot init MAC\n", dev->name );
    return -1;//ENODEV;
  }

  MmacDmaIntClear(tc);    /* clear interrupt requests */
  //MmacDmaIntEnable(tc);   /* enable all interrupts */

  MmacDmaRxStart(tc);     /* start receiver */
  MmacDmaTxStart(tc);     /* start transmitter, it must go to suspend immediately */

  //dev->tbusy = 0;             /* transmitter free */
//   netif_start_queue(dev);
  /*dev->start = 1;              device started */

  return 0;
}

//static int stop( struct eth_device *dev )
static void mmac_stop( struct eth_device *dev )
{
  Private *pr = NULL;
  Mmac *tc = NULL;

  if(dev == NULL)
       return;

  TR(  "stop(%s)\n", dev->name );
//  MOD_DEC_USE_COUNT;
  pr = (Private *)dev->priv;
  if(pr == NULL)
        return;

  tc = &pr->mmac;
  if(tc == NULL)
     return;

  MmacDmaIntDisable(tc);  /* disable all interrupts */
  MmacDmaRxStop(tc);      /* stop receiver, take ownership of all rx descriptors */
  MmacDmaTxStop(tc);      /* stop transmitter, take ownership of all tx descriptors */

  return;
}

//static int hard_start_xmit( struct sk_buff *skb, struct eth_device *dev )
static int mmac_send(struct eth_device *dev, volatile void *packet, int length)
{
  Private *pr = NULL;
  Mmac *tc = NULL;
  u32 data1;
  u32 status;
  u32 len,i=0;

  int r = 0;
  //int debug_count;
  //int byte_align_length;
  if(dev == NULL)
      return -1;//EBUSY;
  if(packet == NULL)
     return -1;

  TR(  "Mmac::send (%s)\n", dev->name );

  pr = (Private *)dev->priv;
  if(pr == NULL)
     return -1;//EBUSY;

  tc = &pr->mmac;
  if(tc == NULL)
     return -1;//EBUSY;

#if 0
/*if( test_and_set_bit( 0, &dev->tbusy ) != 0)*/
if (early_stop_netif_stop_queue(dev) != 0)
  {
    if( jiffies - dev->trans_start < TIMEOUT ) return -EBUSY;

    printf(  "Mmac::send (%s) - tx timeout expired, restart\n", dev->name );
    stop(dev);
    open(dev);
    dev->trans_start = jiffies;
  }
#endif
/* Fix Up to DWord Align the Buffer1 Pointer */
/*
  byte_align_length = ( (skb->data) - (skb->head) );

  if (byte_align_length != 0)
  {
  for (debug_count = 0; debug_count < (skb->len); debug_count++)
  {
    *((skb->head)+debug_count) = *((skb->data)+debug_count);
  }
  }
*/

//  r = MmacDmaTxSet( tc, ( virt_to_phys( skb->data ) ), length, (u32)skb );
  r = MmacDmaTxSet( tc, (u32)packet , length, (u32)packet );
  if( r < 0 )
  {
    printf(  "Mmac::send (%s) - no more free tx descriptors\n", dev->name );
    return -1;//EBUSY;
  }

  tc->txFreeCount--;
  if(tc->txFreeCount<0)
  {
    printf(  "Mmac::send - txFreeCount < 0");
  }
//  consistent_sync(skb->data,skb->len,PCI_DMA_TODEVICE);
  //flush cache to dram
  MmacDmaTxResume(tc);      /* resume transmitter */
  //dev->trans_start = jiffies;
  //dev->tbusy = 0;
  if(tc->txFreeCount == 0)
  {
//    netif_stop_queue(dev);
    printf(  "Mmac::send - txFreeCount == 0");
  }

  //wait untill finish TX, handle it here.

  do
  {
    r = MmacDmaTxGet( tc, &status, NULL, &len, &data1 );   /* get tx descriptor content */
    if(r>=0)
    {
       tc->txFreeCount++;
    }
    if (i++ >= TIMEOUT)
    {
      printf("Mmac::send - timeout  status = %08x\n", status );
      return 0;
    }
    udelay(10);	/* give the nic a chance to write to the register */
  }while(r< 0);

  return len;

#if 0
    r = MmacDmaTxGet( tc, &status, NULL, &len, &data1 );   /* get tx descriptor content */
    if(  r >= 0 && data1 != 0 )
    {
      TR(  "Mmac::send (%s) - get tx descriptor %d for skb %08x, status = %08x\n",
                          dev->name, r, data1, status );
      dev_kfree_skb_irq((struct sk_buff *)data1);
      /*printk ("finish_xmit(%s) - get tx descriptor %d for skb %08x, status = %08x\n",dev->name, r, data1, status); */

      if( MmacDmaTxValid(status) )
      {
//        tc->stats.tx_bytes += len;
//        tc->stats.tx_packets++;
      }
      else
      {
        printk("** tx sts err = 0x%x\n", status);
//        tc->stats.tx_errors++;
//        tc->stats.tx_aborted_errors += MmacDmaTxAborted(status);
//        tc->stats.tx_carrier_errors += MmacDmaTxCarrier(status);
      }
//      tc->stats.collisions += MmacDmaTxCollisions(status);
      tc->txFreeCount++;
    }

  //dev->tbusy = 0;
//  netif_start_queue(dev);

  if(tc->txFreeCount > 0)
  {
//    netif_wake_queue(dev);
  }
  return 0;
#endif

}

static int phy_mtk_reset(Mmac *Dev)
{
	u16 miicontrol = PHY_BMCR_RESET;
	unsigned int tries = 0;

      //release HW reset pin first
      //reset low
      //hw_reset();

      MmacMiiWrite(Dev, PHY_BMCR, PHY_BMCR_RESET);

	/* wait for 500ms */
	mdelay(10);
//	udelay(500);
	/* must wait till reset is deasserted */
	while (miicontrol & PHY_BMCR_RESET)
	{
//		udelay(10);
		miicontrol = MmacMiiRead(Dev, PHY_BMCR);
		/* FIXME: 100 tries seem excessive */
		if (tries++ > 100)
		{
               printf("PHY reset fail\n");
               return -1;
		}
	}
      TR("PHY reset ok, %d\n", tries);
	return 0;
}

int check_link (struct eth_device *dev, Mmac *mmac)
{
    int re_link = 0;
    int cur_link;
//    int prev_link;// = netif_carrier_ok(dev);

    MmacMiiRead(mmac, PHY_BMSR);
    cur_link = (MmacMiiRead(mmac, PHY_BMSR) & PHY_BMSR_LS) ? 1:0;
#if 0
	if (cur_link && !prev_link)
	{
//	  netif_carrier_on(mii->dev);
        printf(  "eth check_link  - link up\n");
        re_link = 1;
	}
	else if (prev_link && !cur_link)
	{
//	  netif_carrier_off(dev);
        printf(  "eth check_link  - no link\n");
      }
	return re_link;
#else
	if (cur_link && (!mmac->link_status))
	{
        printf(  "eth check_link  - link up\n");
        mmac->link_status = cur_link;
        re_link = 1;
	}
	//else
       else if (!cur_link)
	{
        printf(  "eth check_link  - no link, check cable\n");
        mmac->link_status = cur_link;
      }
//	return cur_link;
	return re_link;
#endif

}

//void check_media(struct eth_device *dev, Private *pr, Mmac *mmac)
int check_media(struct eth_device *dev, Private *pr, Mmac *mmac)
{
  int PHY_lpa;
  int negotiated;
  int duplex;
  int i;

  if (check_link(dev, mmac) == 0)
  {
  	return mmac->link_status;
  }

//  if (pr->mii.force_media)
//	return;
//restart AN
//  MmacMiiWrite(mmac, PHY_BMCR, (MmacMiiRead(mmac,PHY_BMCR) | PHY_BMCR_RST_NEG));
//wait AN complete
  i=100000;
  while( i-- > 0 )
  {
    if( (MmacMiiRead(mmac,PHY_BMSR) & PHY_BMSR_AUTN_COMP) != 0 )
    {
      TR(  "check_media(%s) - auto-negotiation complete, i=%d\n", dev->name ,i);
      break;   /* wait for autonegotiation complete */
    }
  }
  if(i ==0)
  {
    return 0;
    printf(  "check_media(%s) - auto-negotiation not finish\n", dev->name );
  }
//check AN result
  PHY_lpa = MmacMiiRead(mmac, PHY_ANLPAR);
  mmac->advertising = MmacMiiRead(mmac, PHY_ANAR);
  negotiated = PHY_lpa & (mmac->advertising);
  duplex = (negotiated &  PHY_ANLPAR_TXFD) || ((negotiated &  (PHY_ANLPAR_TX|PHY_ANLPAR_10FD)) == PHY_ANLPAR_10FD);

  if (PHY_lpa == 0xffff)		/* Bogus read */
	return 0;
  if (mmac->full_duplex != duplex)
  {
	mmac->full_duplex = duplex;
  }

  TR(  "check_media(%s) - %s-duplex mode, adv:%4.4x\n", dev->name, mmac->full_duplex? "Full" : "Half" , PHY_lpa);

 if(negotiated & (PHY_ANLPAR_TXFD| PHY_ANLPAR_TX))
 {
  mmac->speed_100 = 1;
  TR(  "check_media(%s) - 100M\n", dev->name);
 }
 else
 {
  mmac->speed_10 = 1;
  TR(  "check_media(%s) - 10M\n", dev->name);
 }

  MmacDmaRxStop(mmac);
  MmacDmaTxStop(mmac);

  MmacMacInit( mmac, dev->enetaddr, BCmacAddr );

  MmacDmaRxStart(mmac);
  MmacDmaTxStart(mmac);
  return 1;
}
#endif

   