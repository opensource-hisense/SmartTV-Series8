#ifndef PEGASUS_H

    #define PEGASUS_H

    #include <net.h>
    #include <usb.h>

    //#define MTK_DEBUG

    #ifdef MTK_DEBUG

        #define LOG(level, fmt, args...) \
        printf("[%s:%d] " fmt, __PRETTY_FUNCTION__, __LINE__ , ## args)

    #else

        #define LOG(level, fmt, args...) do {} while(0)

    #endif

    #define HZ                      1000

    #define PEGASUS_II      0x80000000
    #define HAS_HOME_PNA       0x40000000

    #define PEGASUS_MTU     1536
    #define RX_SKBS           4

    #define EPROM_WRITE      0x01
    #define EPROM_READ        0x02
    #define EPROM_DONE       0x04
    #define EPROM_WR_ENABLE      0x10
    #define EPROM_LOAD        0x20

    #define PHY_DONE      0x80
    #define PHY_READ       0x40
    #define PHY_WRITE        0x20
    #define DEFAULT_GPIO_RESET    0x24
    #define DEFAULT_GPIO_SET    0x26

    #define PEGASUS_PRESENT     0x00000001
    #define PEGASUS_TX_BUSY     0x00000004
    #define PEGASUS_RX_BUSY     0x00000008
    #define CTRL_URB_RUNNING 0x00000010
    #define CTRL_URB_SLEEP     0x00000020
    #define PEGASUS_UNPLUG      0x00000040
    #define PEGASUS_RX_URB_FAIL   0x00000080
    #define ETH_REGS_CHANGE        0x40000000
    #define ETH_REGS_CHANGED    0x80000000

    #define RX_MULTICAST       2
    #define RX_PROMISCUOUS       4

    #define REG_TIMEOUT        (HZ)
    #define PEGASUS_TX_TIMEOUT  (HZ*10)

    #define TX_UNDERRUN      0x80
    #define EXCESSIVE_COL        0x40
    #define LATE_COL        0x20
    #define NO_CARRIER     0x10
    #define LOSS_CARRIER        0x08
    #define JABBER_TIMEOUT     0x04

    #define LINK_STATUS       0x01

    #define PEGASUS_REQT_READ    0xc0
    #define PEGASUS_REQT_WRITE 0x40
    #define PEGASUS_REQ_GET_REGS 0xf0
    #define PEGASUS_REQ_SET_REGS   0xf1
    #define PEGASUS_REQ_SET_REG PEGASUS_REQ_SET_REGS

enum pegasus_registers
{
    EthCtrl0 = 0,
    EthCtrl1 = 1,
    EthCtrl2 = 2,
    EthID = 0x10,
    Reg1d = 0x1d,
    EpromOffset = 0x20,
    EpromData = 0x21, /* 0x21 low, 0x22 high byte */
    EpromCtrl = 0x23,
    PhyAddr = 0x25,
    PhyData = 0x26, /* 0x26 low, 0x27 high byte */
    PhyCtrl = 0x28,
    UsbStst = 0x2a,
    EthTxStat0 = 0x2b,
    EthTxStat1 = 0x2c,
    EthRxStat = 0x2d,
    WakeupControl = 0x78,
    Reg7b = 0x7b,
    Gpio0 = 0x7e,
    Gpio1 = 0x7f,
    Reg81 = 0x81,
};

/* Define these values to match your device */

    #define VENDOR_ID_ADMTEK        0x07A6
    #define PRODUCT_ID_ADM8511       0x8511
    #define VENDOR_3COM        0x0506
    #define VENDOR_ABOCOM        0x07b8
    #define VENDOR_ACCTON      0x083a
    #define VENDOR_ADMTEK        0x07a6
    #define VENDOR_AEILAB      0x3334
    #define VENDOR_ALLIEDTEL    0x07c9
    #define VENDOR_ATEN        0x0557
    #define VENDOR_BELKIN        0x050d
    #define VENDOR_BILLIONTON  0x08dd
    #define VENDOR_COMPAQ     0x049f
    #define VENDOR_COREGA       0x07aa
    #define VENDOR_DLINK     0x2001
    #define VENDOR_ELCON        0x0db7
    #define VENDOR_ELECOM       0x056e
    #define VENDOR_ELSA     0x05cc
    #define VENDOR_GIGABYTE     0x1044
    #define VENDOR_HAWKING     0x0e66
    #define VENDOR_HP      0x03f0
    #define VENDOR_IODATA        0x04bb
    #define VENDOR_KINGSTON      0x0951
    #define VENDOR_LANEED      0x056e
    #define VENDOR_LINKSYS        0x066b
    #define VENDOR_LINKSYS2     0x077b
    #define VENDOR_MELCO     0x0411
    #define VENDOR_MICROSOFT    0x045e
    #define VENDOR_MOBILITY        0x1342
    #define VENDOR_NETGEAR        0x0846
    #define VENDOR_OCT     0x0b39
    #define VENDOR_SMARTBRIDGES  0x08d1
    #define VENDOR_SMC       0x0707
    #define VENDOR_SOHOWARE        0x15e8
    #define VENDOR_SIEMENS        0x067c

    #define MII_BMCR            0x00 /* Basic mode control register */
    #define MII_BMSR            0x01 /* Basic mode status register  */
    #define MII_PHYSID1         0x02 /* PHYS ID 1                   */
    #define MII_PHYSID2         0x03 /* PHYS ID 2                   */
    #define MII_ADVERTISE       0x04 /* Advertisement control reg   */
    #define MII_LPA             0x05 /* Link partner ability reg    */

    /* Basic mode status register. */
    #define BMSR_ERCAP              0x0001 /* Ext-reg capability          */
    #define BMSR_JCD                0x0002 /* Jabber detected             */
    #define BMSR_LSTATUS            0x0004 /* Link status                 */
    #define BMSR_ANEGCAPABLE        0x0008 /* Able to do auto-negotiation */
    #define BMSR_RFAULT             0x0010 /* Remote fault detected       */
    #define BMSR_ANEGCOMPLETE       0x0020 /* Auto-negotiation complete   */
    #define BMSR_RESV               0x00c0 /* Unused...                   */
    #define BMSR_ESTATEN       0x0100      /* Extended Status in R15 */
    #define BMSR_100FULL2     0x0200       /* Can do 100BASE-T2 HDX */
    #define BMSR_100HALF2       0x0400     /* Can do 100BASE-T2 FDX */
    #define BMSR_10HALF             0x0800 /* Can do 10mbps, half-duplex  */
    #define BMSR_10FULL             0x1000 /* Can do 10mbps, full-duplex  */
    #define BMSR_100HALF            0x2000 /* Can do 100mbps, half-duplex */
    #define BMSR_100FULL            0x4000 /* Can do 100mbps, full-duplex */
    #define BMSR_100BASE4           0x8000 /* Can do 100mbps, 4k packets  */

    /* Basic mode control register. */
    #define BMCR_RESV               0x003f  /* Unused...                   */
    #define BMCR_SPEED1000		0x0040  /* MSB of Speed (1000)         */
    #define BMCR_CTST               0x0080  /* Collision test              */
    #define BMCR_FULLDPLX           0x0100  /* Full duplex                 */
    #define BMCR_ANRESTART          0x0200  /* Auto negotiation restart    */
    #define BMCR_ISOLATE            0x0400  /* Disconnect DP83840 from MII */
    #define BMCR_PDOWN              0x0800  /* Powerdown the DP83840       */
    #define BMCR_ANENABLE           0x1000  /* Enable auto negotiation     */
    #define BMCR_SPEED100           0x2000  /* Select 100Mbps              */
    #define BMCR_LOOPBACK           0x4000  /* TXD loopback bits           */
    #define BMCR_RESET              0x8000  /* Reset the DP83840           */

    /* Advertisement control register. */
    #define ADVERTISE_SLCT          0x001f /* Selector bits               */
    #define ADVERTISE_CSMA          0x0001 /* Only selector supported     */
    #define ADVERTISE_10HALF        0x0020 /* Try for 10mbps half-duplex  */
    #define ADVERTISE_1000XFULL     0x0020 /* Try for 1000BASE-X full-duplex */
    #define ADVERTISE_10FULL        0x0040 /* Try for 10mbps full-duplex  */
    #define ADVERTISE_1000XHALF     0x0040 /* Try for 1000BASE-X half-duplex */
    #define ADVERTISE_100HALF       0x0080 /* Try for 100mbps half-duplex */
    #define ADVERTISE_1000XPAUSE    0x0080 /* Try for 1000BASE-X pause    */
    #define ADVERTISE_100FULL       0x0100 /* Try for 100mbps full-duplex */
    #define ADVERTISE_1000XPSE_ASYM 0x0100 /* Try for 1000BASE-X asym pause */
    #define ADVERTISE_100BASE4      0x0200 /* Try for 100mbps 4k packets  */
    #define ADVERTISE_PAUSE_CAP     0x0400 /* Try for pause               */
    #define ADVERTISE_PAUSE_ASYM    0x0800 /* Try for asymetric pause     */
    #define ADVERTISE_RESV          0x1000 /* Unused...                   */
    #define ADVERTISE_RFAULT        0x2000 /* Say we can detect faults    */
    #define ADVERTISE_LPACK         0x4000 /* Ack link partners response  */
    #define ADVERTISE_NPAGE         0x8000 /* Next page bit               */

    #define ADVERTISE_FULL (ADVERTISE_100FULL | ADVERTISE_10FULL | \
    			ADVERTISE_CSMA)
    #define ADVERTISE_ALL (ADVERTISE_10HALF | ADVERTISE_10FULL | \
                           ADVERTISE_100HALF | ADVERTISE_100FULL)

struct pegasus
{
    unsigned long feature;
    struct usb_device * udev;
    struct eth_device * netdev;
    int chip;
    u8 rx_buf[PEGASUS_MTU];
    u8 tx_buf[512];
    u8 phy;
};

typedef struct pegasus pegasus_t;

#endif /* #ifndef PEGASUS_H */
