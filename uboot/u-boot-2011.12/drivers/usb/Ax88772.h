
#include <net.h>
#include <usb.h>

//#define MTK_DEBUG

#ifdef MTK_DEBUG
    #define LOG(level, fmt, args...) \
        if (_u4Debug >= (level)) printf("[%s:%d] " fmt, \
        __PRETTY_FUNCTION__, __LINE__ , ## args)
#else
    #define LOG(level, fmt, args...) do {} while(0)
#endif


#define msleep(ms)                  udelay(ms * 1000)

// for vendor-specific control operations
#define CONTROL_TIMEOUT_MS          500
#define BUFFER_SIZE                 2048

#define ETH_ALEN                    6       // Octets in one ethernet addr
#define ETH_HLEN                    14      // Total octets in header.
#define ETH_ZLEN                    60      // Min. octets in frame sans FCS
#define ETH_DATA_LEN                1500    // Max. octets in payload
#define ETH_FRAME_LEN               1514    // Max. octets in frame sans FCS

#define AX17x_CTRL_PIPE             0
#define AX17x_INTR_PIPE             1
#define AX178_BULKIN_PIPE           2
#define AX178_BULKOUT_PIPE          3

#define AX_CMD_SET_SW_MII           0x06
#define AX_CMD_READ_MII_REG         0x07
#define AX_CMD_WRITE_MII_REG        0x08
#define AX_CMD_SET_HW_MII           0x0a
#define AX_CMD_READ_EEPROM          0x0b
#define AX_CMD_WRITE_EEPROM         0x0c
#define AX_CMD_WRITE_ENABLE         0x0d
#define AX_CMD_WRITE_DISABLE        0x0e
#define AX_CMD_WRITE_RX_CTL         0x10
#define AX_CMD_READ_IPG012          0x11
#define AX_CMD_WRITE_IPG0           0x12
#define AX_CMD_WRITE_IPG1           0x13
#define AX_CMD_WRITE_IPG2           0x14
#define AX_CMD_WRITE_MULTI_FILTER   0x16
#define AX_CMD_READ_NODE_ID         0x13
#define AX_CMD_READ_PHY_ID          0x19
#define AX_CMD_READ_MEDIUM_STATUS   0x1a
#define AX_CMD_WRITE_MEDIUM_MODE    0x1b
#define AX_CMD_READ_MONITOR_MODE    0x1c
#define AX_CMD_WRITE_MONITOR_MODE   0x1d
#define AX_CMD_WRITE_GPIOS          0x1f
#define AX_CMD_SW_RESET             0x20
#define AX_CMD_SW_PHY_STATUS        0x21
#define AX_CMD_SW_PHY_SELECT        0x22
#define AX88172_CMD_READ_NODE_ID	0x17

#define AX_MONITOR_MODE             0x01
#define AX_MONITOR_LINK             0x02
#define AX_MONITOR_MAGIC            0x04
#define AX_MONITOR_HSFS             0x10

/* AX88172 Medium Status Register values */
#define AX_MEDIUM_FULL_DUPLEX       0x02
#define AX_MEDIUM_TX_ABORT_ALLOW    0x04
#define AX_MEDIUM_FLOW_CONTROL_EN   0x10

#define AX_MCAST_FILTER_SIZE        8
#define AX_MAX_MCAST                64

#define AX_EEPROM_LEN               0x40

#define AX_SWRESET_CLEAR            0x00
#define AX_SWRESET_RR               0x01
#define AX_SWRESET_RT               0x02
#define AX_SWRESET_PRTE             0x04
#define AX_SWRESET_PRL              0x08
#define AX_SWRESET_BZ               0x10
#define AX_SWRESET_IPRL             0x20
#define AX_SWRESET_IPPD             0x40

#define AX88772_IPG0_DEFAULT        0x15
#define AX88772_IPG1_DEFAULT        0x0c
#define AX88772_IPG2_DEFAULT        0x12

#define AX88772_MEDIUM_FULL_DUPLEX  0x0002
#define AX88772_MEDIUM_RESERVED     0x0004
#define AX88772_MEDIUM_RX_FC_ENABLE 0x0010
#define AX88772_MEDIUM_TX_FC_ENABLE 0x0020
#define AX88772_MEDIUM_PAUSE_FORMAT 0x0080
#define AX88772_MEDIUM_RX_ENABLE    0x0100
#define AX88772_MEDIUM_100MB        0x0200
#define AX88772_MEDIUM_DEFAULT      (AX88772_MEDIUM_FULL_DUPLEX |   \
                                     AX88772_MEDIUM_RX_FC_ENABLE |  \
                                     AX88772_MEDIUM_TX_FC_ENABLE |  \
                                     AX88772_MEDIUM_100MB |         \
                                     AX88772_MEDIUM_RESERVED |      \
                                     AX88772_MEDIUM_RX_ENABLE)

/* AX88772 & AX88178 RX_CTL values */
#define AX_RX_CTL_SO			0x0080
#define AX_RX_CTL_AP			0x0020
#define AX_RX_CTL_AM			0x0010
#define AX_RX_CTL_AB			0x0008
#define AX_RX_CTL_SEP			0x0004
#define AX_RX_CTL_AMALL			0x0002
#define AX_RX_CTL_PRO			0x0001
#define AX_RX_CTL_MFB_2048		0x0000
#define AX_RX_CTL_MFB_4096		0x0100
#define AX_RX_CTL_MFB_8192		0x0200
#define AX_RX_CTL_MFB_16384		0x0300

#define AX_DEFAULT_RX_CTL	\
	(AX_RX_CTL_SO | AX_RX_CTL_AB )


/* AX88172 Medium Status Register values */
#define AX88172_MEDIUM_FD		0x02
#define AX88172_MEDIUM_TX		0x04
#define AX88172_MEDIUM_FC		0x10
#define AX88172_MEDIUM_DEFAULT \
		( AX88172_MEDIUM_FD | AX88172_MEDIUM_TX | AX88172_MEDIUM_FC )

// Define Vendor & Product ID
#define VENDOR_ID_AX88772                       0x0b95
#define PRODUCT_ID_AX88772                     0x7720
#define PRODUCT_ID_AX88172                     0x1720

struct _usbnet {
    struct usb_device   *udev;
    struct eth_device   *netdev;
    int                 nInPipe;
    int                 nOutPipe;
    int                 nInMaxPacket;
    int                 nOutMaxPacket;
    int                 nPhy;
    u8                  rx_buf[BUFFER_SIZE];
    u8                  tx_buf[BUFFER_SIZE];
};

typedef struct _usbnet usbnet;

