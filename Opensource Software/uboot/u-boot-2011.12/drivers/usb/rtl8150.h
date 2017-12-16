
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

#define HZ                      1000

#define	IDR			            0x0120
#define	MAR			            0x0126
#define	CR			            0x012e
#define	TCR			            0x012f
#define	RCR			            0x0130
#define	TSR			            0x0132
#define	RSR			            0x0133
#define	CON0			        0x0135
#define	CON1			        0x0136
#define	MSR			            0x0137
#define	PHYADD			        0x0138
#define	PHYDAT			        0x0139
#define	PHYCNT			        0x013b
#define	GPPC			        0x013d
#define	BMCR			        0x0140
#define	BMSR			        0x0142
#define	ANAR			        0x0144
#define	ANLP			        0x0146
#define	AER			            0x0148
#define CSCR			        0x014C  /* This one has the link status */
#define CSCR_LINK_STATUS	    (1 << 3)

#define	IDR_EEPROM		        0x1202

#define	RTL8150_REQT_READ	    0xc0
#define	RTL8150_REQT_WRITE	    0x40
#define	RTL8150_REQ_GET_REGS	0x05
#define	RTL8150_REQ_SET_REGS	0x05

#define	RTL8150_MTU		        1540
#define	RTL8150_TX_TIMEOUT	    (HZ)
#define	RX_SKB_POOL_SIZE	    4

/* rtl8150 flags */
#define	RTL8150_HW_CRC		    0
#define	RX_REG_SET		        1
#define	RTL8150_UNPLUG		    2
#define	RX_URB_FAIL		        3

/* Define these values to match your device */
#define VENDOR_ID_REALTEK		0x0bda
#define	VENDOR_ID_MELCO			0x0411
#define VENDOR_ID_MICRONET		0x3980
#define	VENDOR_ID_LONGSHINE		0x07b8

#define PRODUCT_ID_RTL8150		0x8150
#define	PRODUCT_ID_LUAKTX		0x0012
#define	PRODUCT_ID_LCS8138TX	0x401a
#define PRODUCT_ID_SP128AR		0x0003

struct rtl8150 {
	unsigned long       flags;
	struct usb_device   *udev;
	struct eth_device   *netdev;
	u_int16_t           rx_creg;
	u8                  rx_buf[RTL8150_MTU];
	u8                  tx_buf[512];
	u8                  phy;
};

typedef struct rtl8150 rtl8150_t;


