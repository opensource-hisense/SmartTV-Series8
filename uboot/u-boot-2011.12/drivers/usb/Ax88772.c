
#include <common.h>
#include <command.h>
#include "Ax88772.h"
#include "pegasus.h"

#ifdef CONFIG_DRIVER_AX88772

#if defined(CONFIG_CMD_NET)

static usbnet AxDev;
static struct eth_device AxEthDev;

/* ASIX AX8817x USB 2.0 Ethernet */
#define AX8817X_GPIO 0x00130103

static unsigned long GPIO_DATA = AX8817X_GPIO;

static int asix_write_cmd(usbnet *dev, u8 cmd, u16 value, u16 index,
                             u16 size, void *data)
{
    return usb_control_msg(dev->udev,
                           usb_sndctrlpipe(dev->udev, 0),
                           cmd,
                           USB_DIR_OUT | USB_TYPE_VENDOR | USB_RECIP_DEVICE,
                           value,
                           index,
                           data,
                           size,
                           CONTROL_TIMEOUT_MS);
}

static int asix_read_cmd(usbnet *dev, u8 cmd, u16 value, u16 index,
                            u16 size, void *data)
{
    return usb_control_msg(dev->udev,
                           usb_rcvctrlpipe(dev->udev, 0),
                           cmd,
                           USB_DIR_IN | USB_TYPE_VENDOR | USB_RECIP_DEVICE,
                           value,
                           index,
                           data,
                           size,
                           CONTROL_TIMEOUT_MS);
}

static int GetEndpoints(usbnet *dev, struct usb_config_descriptor *config)
{
    int tmp;
    struct usb_interface_descriptor *alt = NULL;
    struct usb_endpoint_descriptor *pIn = NULL, *pOut = NULL, *pStatus = NULL;
        
    for (tmp = 0; tmp < config->bNumInterfaces; tmp++)
    {
        unsigned    ep;

        pIn = pOut = pStatus = NULL;
        if (tmp >= USB_MAXINTERFACES)
        {
            return -1;
        }
        alt = &config->if_desc[tmp];

        /* take the first altsetting with in-bulk + out-bulk;
         * remember any status endpoint, just in case;
         * ignore other endpoints and altsetttings.
         */
        for (ep = 0; ep < alt->bNumEndpoints; ep++)
        {
            struct usb_endpoint_descriptor *pEnp;
            int intr = 0;

            if (ep >= USB_MAXENDPOINTS)
            {
                return -1;
            }          
            pEnp = &alt->ep_desc[ep];
            switch (pEnp->bmAttributes)
            {
            case USB_ENDPOINT_XFER_INT:
                if (!(pEnp->bEndpointAddress & USB_DIR_IN))         continue;
                intr = 1;
                
            case USB_ENDPOINT_XFER_BULK:    // FALLTHROUGH 
                break;
                
            default:
                continue;
            }
            
            if (pEnp->bEndpointAddress & USB_DIR_IN)
            {
                if (!intr && !pIn)
                    pIn = pEnp;
                else if (intr && !pStatus)
                    pStatus = pEnp;
            }
            else
            {
                if (!pOut)
                    pOut = pEnp;
            }
        }
        
        if (pIn && pOut)        break;
    }
    
    if (!alt || !pIn || !pOut)
        return -1;

    dev->nInPipe = usb_rcvbulkpipe(dev->udev, pIn->bEndpointAddress);
    dev->nOutPipe = usb_sndbulkpipe(dev->udev, pOut->bEndpointAddress);

    tmp = usb_set_interface(dev->udev, alt->bInterfaceNumber, 
                            alt->bAlternateSetting);
    if (tmp < 0)
        return tmp;
    
    return 0;
}

static int Ax88772_start(struct eth_device *dev)
{
    struct usb_config_descriptor *pConfig = NULL;
    usbnet *net_dev = dev->priv;
    unsigned char u1Buf[ETH_ALEN];
    char Ethbuf[32];
    void *pBuf;
    int ret = 0;

    usbnet *usbnet_dev = dev->priv;
    struct usb_device *udev = usbnet_dev->udev;

    pConfig = &udev->config;    
    GetEndpoints(usbnet_dev, pConfig);
    pBuf = (void *)u1Buf;
    
    if (asix_write_cmd(net_dev, AX_CMD_WRITE_GPIOS, 0x00B0, 0, 0, pBuf) < 0)
        goto out;

    msleep(5);
    ret = asix_write_cmd(net_dev, AX_CMD_SW_PHY_SELECT, 0x0001, 0, 0, pBuf);
    if (ret < 0) 
    {
        LOG(5, "Select PHY #1 failed: %d\n", ret);
        goto out;
    }

    ret = asix_write_cmd(net_dev, AX_CMD_SW_RESET, AX_SWRESET_IPPD, 0, 0, pBuf);
    if (ret < 0) 
    {
        LOG(5, "Failed to power down internal PHY: %d\n", ret);
        goto out;
    }

    msleep(150);
    ret = asix_write_cmd(net_dev, AX_CMD_SW_RESET, AX_SWRESET_CLEAR, 0, 0, pBuf);
    if (ret < 0)
    {
        LOG(5, "Failed to perform software reset: %d\n", ret);
        goto out;
    }

    msleep(150);
    ret = asix_write_cmd(net_dev, AX_CMD_SW_RESET,
                            AX_SWRESET_IPRL | AX_SWRESET_PRL, 0, 0, pBuf);
    if (ret < 0)
    {
        LOG(5, "Failed to set Internal/External PHY reset control: %d\n", ret);
        goto out;
    }

    msleep(150);
    ret = asix_write_cmd(net_dev, AX_CMD_WRITE_RX_CTL, 0x0000, 0, 0, pBuf);
    if (ret < 0) 
    {
        LOG(5, "Failed to reset RX_CTL: %d\n", ret);
        goto out;
    }

    // Get the MAC address
    memset(u1Buf, 0, ETH_ALEN);
    ret = asix_read_cmd(net_dev, AX_CMD_READ_NODE_ID, 0, 0, ETH_ALEN, u1Buf);
    if (ret < 0) 
    {
        LOG(5, "Failed to read MAC address: %d\n", ret);
        goto out;
    }
    memcpy(net_dev->netdev->enetaddr, u1Buf, ETH_ALEN);

    // set Mac to environment address.
    memset(Ethbuf, 0, sizeof(32));
    sprintf(Ethbuf, "%02X:%02X:%02X:%02X:%02X:%02X", 
            u1Buf[0], u1Buf[1], u1Buf[2], u1Buf[3], u1Buf[4], u1Buf[5]);
    setenv("ethaddr", Ethbuf);

    printf("UsbNet Mac addr = %s.\n", Ethbuf);        
    
    // Get the PHY id
    ret = asix_read_cmd(net_dev, AX_CMD_READ_PHY_ID, 0, 0, 2, pBuf);
    if (ret < 0)
    {
        LOG(5, "Error reading PHY ID: %02x\n", ret);
        goto out;
    }
    else if (ret < 2)
    {
        // this should always return 2 bytes
        LOG(5, "AX_CMD_READ_PHY_ID returned less than 2 bytes: ret=%02x\n", ret);
        goto out;
    }
    net_dev->nPhy = *((u8 *)pBuf + 1);

    ret = asix_write_cmd(net_dev, AX_CMD_SW_RESET, AX_SWRESET_PRL, 0, 0, pBuf);
    if (ret < 0)
    {
        LOG(5, "Set external PHY reset pin level: %d\n", ret);
        goto out;
    }
    msleep(150);
    
    ret = asix_write_cmd(net_dev, AX_CMD_SW_RESET,
                            AX_SWRESET_IPRL | AX_SWRESET_PRL, 0, 0, pBuf);
    if (ret < 0)
    {
        LOG(5, "Set Internal/External PHY reset control: %d\n", ret);
        goto out;
    }
    msleep(150);

    ret = asix_write_cmd(net_dev, AX_CMD_WRITE_MEDIUM_MODE,
                            AX88772_MEDIUM_DEFAULT, 0, 0, pBuf);
    if (ret < 0)
    {
        LOG(5, "Write medium mode register: %d\n", ret);
        goto out;
    }

    ret = asix_write_cmd(net_dev, AX_CMD_WRITE_IPG0,
                            AX88772_IPG0_DEFAULT | AX88772_IPG1_DEFAULT,
                            AX88772_IPG2_DEFAULT, 0, pBuf);
    if (ret < 0)
    {
        LOG(5, "Write IPG,IPG1,IPG2 failed: %d\n", ret);
        goto out;
    }
    
    ret = asix_write_cmd(net_dev, AX_CMD_SET_HW_MII, 0, 0, 0, &pBuf);
    if (ret < 0)
    {
        LOG(5, "Failed to set hardware MII: %02x\n", ret);
        goto out;
    }

    // Set RX_CTL to default values with 2k buffer, and enable cactus
    ret = asix_write_cmd(net_dev, AX_CMD_WRITE_RX_CTL, 0x0088, 0, 0, pBuf);
    if (ret < 0)
    {
        LOG(5, "Reset RX_CTL failed: %d\n", ret);
        goto out;
    }
        
out:
   
    return ret;
}

static int Ax88772_open(struct eth_device *dev, bd_t *bis)
{
    usbnet *net_dev = dev->priv;
    unsigned char pBuf[ETH_ALEN];
    int ret = 0;
   
    // Get the MAC address
    memset(pBuf, 0, ETH_ALEN);
    ret = asix_read_cmd(net_dev, AX_CMD_READ_NODE_ID, 0, 0, ETH_ALEN, pBuf);
    if (ret < 0) 
    {
        LOG(5, "Failed to read MAC address: %d\n", ret);
        goto out;
    }
    memcpy(net_dev->netdev->enetaddr, pBuf, ETH_ALEN);
    
out:
    
    return ret;
}

void Ax88772_halt(struct eth_device *dev)
{
}

// Send a data block via Ethernet.
static int Ax88772_tx(struct eth_device *dev, volatile void *packet, int length)
{
    usbnet *net_dev = dev->priv;
    int count = 0;
    int pipe = net_dev->nOutPipe;
    int maxtry, partial, padlen;
    int result, stat;
    int this_xfer;
    u32 *packet_len;
    char *ptr;

    padlen = ((length + 4) % 512) ? 0 : 4;
    this_xfer = length + padlen + 4;
    if (this_xfer > BUFFER_SIZE)
    {
        LOG(7, "tx length %d exceeds the buffer limit\n", length);
        return 0;
    }

    // fix-up tx buffer
    packet_len = (u32 *)net_dev->tx_buf;
    *packet_len = ((length ^ 0x0000ffff) << 16) + (length);
    memcpy((void *)net_dev->tx_buf + 4, (void *)packet, this_xfer);
    if (padlen)
    {
        u32 *padbytes_ptr;
        padbytes_ptr = (u32 *)(net_dev->tx_buf + 4 + length);
        *padbytes_ptr = 0xffff0000;
    }

    ptr = (char *)net_dev->tx_buf;
    
    // while we have data left to transfer
    while (this_xfer)
    {
        // setup the retry counter
        maxtry = 10;

        // transfer the data
        LOG(7, "Bulk xfer 0x%x(%d) try #%d\n",
               (unsigned int)buf, this_xfer, 11 - maxtry);
        result = usb_bulk_msg(net_dev->udev, pipe, ptr,
                              this_xfer, &partial, USB_CNTL_TIMEOUT * 5);
        LOG(7, "bulk_msg returned %d xferred %d/%d\n",
               result, partial, this_xfer);
                   
        count += partial;
            
        if (net_dev->udev->status != 0)
        {
            // if we stall, we need to clear it before we go on
            if (net_dev->udev->status & USB_ST_STALLED)
            {
                LOG(5, "stalled ->clearing endpoint halt for pipe 0x%x\n", pipe);
                stat = net_dev->udev->status;
                usb_clear_halt(net_dev->udev, pipe);
                net_dev->udev->status = stat;
                if (this_xfer == partial)
                {
                    LOG(3, "bulk transferred with error %X, but data ok\n",
                           net_dev->udev->status);
                }
                    
                goto out;
            }
                
            if (net_dev->udev->status & USB_ST_NAK_REC)
            {
                LOG(5, "Device NAKed bulk_msg\n");
                goto out;
            }
                
            if (this_xfer == partial)
            {
                LOG(5, "bulk transferred with error %d, but data ok\n",
                       net_dev->udev->status);
                goto out;
            }
                
            // if our try counter reaches 0, bail out
            LOG(5, "bulk transferred with error %d, data %d\n",
                   net_dev->udev->status, partial);
            if (!maxtry--)
                goto out;
        }
            
        // update to show what data was transferred
        this_xfer -= partial;
        ptr += partial;
            
        // continue until this transfer is done
    }

out:
    return count;
}

// Get a data block via Ethernet
static int Ax88772_rx(struct eth_device *dev)
{
    usbnet *net_dev = dev->priv;
    static int partial = 0;
    static char *packet;
    u32 packet_header;
    u16 size;

    if (partial == 0)
    {
        int result;
        
        result = usb_bulk_msg(net_dev->udev, net_dev->nInPipe, net_dev->rx_buf,
                              BUFFER_SIZE, &partial, USB_CNTL_TIMEOUT * 5);
        if (result < 0)
        {
            if (net_dev->udev->status & USB_ST_STALLED)
            {
                LOG(3, "stalled ->clearing endpoint halt for pipe 0x%x\n", 
                       net_dev->nInPipe);
        
                // clear the STALL on the endpoint
                usb_clear_halt(net_dev->udev, net_dev->nInPipe);
            }
            else
            {
                LOG(5, "usb_bulk_msg error status %ld\n",
                       net_dev->udev->status);
            }
            return 0;
        }

        packet = (char *)net_dev->rx_buf;
    }
    
    // fix-up rx buffer
    packet_header = (*packet) + (*(packet + 1) << 8) + 
                    (*(packet + 2) << 16) + (*(packet+ 3) << 24);
    packet += 4;
    partial -= sizeof(packet_header);
    if ((short)(packet_header & 0x0000ffff) != 
        ~((short)((packet_header & 0xffff0000) >> 16)))
    {
        LOG(3, "header length data is error\n");
        return 0;
    }

    // get the packet length
    size = (u16)(packet_header & 0x0000ffff);
    if ((size > ETH_FRAME_LEN) || (size == 0))
    {
        LOG(5, "invalid rx length %d (return packet size: %d)\n",
               size, partial);
        return 0;
    }
    else if (partial < size)
    {
        LOG(5, "Partial size :%d is smaller than the rx length %d \n",
               partial, size);
        partial = 0;
        return 0;
    }
    else
    {
        // Pass Packets to upper Layer
        memcpy((void *)NetRxPackets[0], (void *)packet, size);
        NetReceive(NetRxPackets[0], size);

        size += (size % 2);
        partial -= size;
        packet += size;
        return size;
    }
}

int Ax88772_init(struct usb_device *usbdev)
{
    struct eth_device *dev;
    
    memset(&AxDev, 0, sizeof(usbnet));
    AxDev.udev = usbdev;

    dev = (struct eth_device *)&AxEthDev;

    sprintf (dev->name, "AX88772#%x", AxDev.udev->descriptor.idVendor);

    dev->priv = (void *)&AxDev;
    dev->init = Ax88772_open;
    dev->halt = Ax88772_halt;
    dev->send = Ax88772_tx;
    dev->recv = Ax88772_rx;

    eth_register(dev);
    Ax88772_start(dev);
    udelay (10 * 1000);
    AxDev.netdev = dev;
    return 0;

}

/*
==============================================================
*/

static int asix_set_sw_mii(usbnet *dev)
{
	int ret;
	ret = asix_write_cmd(dev, AX_CMD_SET_SW_MII, 0x0000, 0, 0, NULL);
	if (ret < 0)
		printf("Failed to enable software MII access");
	return ret;
}

static int asix_set_hw_mii(usbnet *dev)
{
	int ret;
	ret = asix_write_cmd(dev, AX_CMD_SET_HW_MII, 0x0000, 0, 0, NULL);
	if (ret < 0)
		printf("Failed to enable hardware MII access");
	return ret;
}

#if 0   // fix compile warning.
static int asix_mdio_read(usbnet *dev, int phy_id, int loc)
{
	u16 res;

	asix_set_sw_mii(dev);
	asix_read_cmd(dev, AX_CMD_READ_MII_REG, phy_id,
				(__u16)loc, 2, (u16 *)&res);
	asix_set_hw_mii(dev);

	return le16_to_cpu(res & 0xffff);
}
#endif

static void asix_mdio_write(usbnet *dev, int phy_id, int loc, int val)
{
	u16 res = cpu_to_le16(val);

	asix_set_sw_mii(dev);
	asix_write_cmd(dev, AX_CMD_WRITE_MII_REG, phy_id,
				(__u16)loc, 2, (u16 *)&res);
	asix_set_hw_mii(dev);
}

static int Ax88172_start(struct eth_device *dev)
{
    struct usb_config_descriptor *pConfig = NULL;
    usbnet *net_dev = dev->priv;
    unsigned char u1Buf[20];
    char Ethbuf[32];
    void *pBuf;
    int ret = 0;
    int i = 0;
    usbnet *usbnet_dev = dev->priv;
    struct usb_device *udev = usbnet_dev->udev;
    unsigned short rx_ctl = AX_DEFAULT_RX_CTL;

    pConfig = &udev->config;
    GetEndpoints(usbnet_dev, pConfig);
    pBuf = (void *)u1Buf;

    /* Toggle the GPIOs in a manufacturer/model specific way */
    for (i = 2; i >= 0; i--) 
    {
        if (asix_write_cmd(
            net_dev, AX_CMD_WRITE_GPIOS, (GPIO_DATA >> (i * 8)) & 0xff, 0, 0, NULL) < 0)
        {
            goto out;
        }        
        udelay (5 * 1000);
    }

    if (asix_write_cmd(net_dev, AX_CMD_WRITE_RX_CTL, 0x80, 0, 0, NULL) < 0)
        goto out;

    // Get the MAC address
    memset(u1Buf, 0, ETH_ALEN);
    ret = asix_read_cmd(net_dev, AX88172_CMD_READ_NODE_ID, 0, 0, ETH_ALEN, u1Buf);
    if (ret < 0) 
    {
        LOG(5, "Failed to read MAC address: %d\n", ret);
        goto out;
    }
    memcpy(net_dev->netdev->enetaddr, u1Buf, ETH_ALEN);

    // set Mac to environment address.
    memset(Ethbuf, 0, sizeof(32));
    sprintf(Ethbuf, "%02X:%02X:%02X:%02X:%02X:%02X", 
            u1Buf[0], u1Buf[1], u1Buf[2], u1Buf[3], u1Buf[4], u1Buf[5]);
    setenv("ethaddr", Ethbuf);

    printf("UsbNet Mac addr = %s.\n", Ethbuf);        

    // Get the PHY id
    ret = asix_read_cmd(net_dev, AX_CMD_READ_PHY_ID, 0, 0, 2, pBuf);
    if (ret < 0)
    {
        LOG(5, "Error reading PHY ID: %02x\n", ret);
        goto out;
    }
    else if (ret < 2)
    {
        // this should always return 2 bytes
        LOG(5, "AX_CMD_READ_PHY_ID returned less than 2 bytes: ret=%02x\n", ret);
        goto out;
    }
    net_dev->nPhy = *((u8 *)pBuf + 1);

    asix_mdio_write(net_dev, net_dev->nPhy, MII_BMCR, BMCR_RESET);

    asix_mdio_write(net_dev, net_dev->nPhy, MII_ADVERTISE,
    	ADVERTISE_ALL | ADVERTISE_CSMA | ADVERTISE_PAUSE_CAP);

    asix_mdio_write(net_dev, net_dev->nPhy, MII_BMCR,
    	BMCR_SPEED100|BMCR_ANENABLE|BMCR_ANRESTART);

    ret = asix_write_cmd(net_dev, AX_CMD_WRITE_MEDIUM_MODE,
                            AX88172_MEDIUM_DEFAULT, 0, 0, NULL);
    if (ret < 0)
    {
        LOG(5, "Write medium mode register: %d\n", ret);
        goto out;
    }

    rx_ctl |= (AX_DEFAULT_RX_CTL|AX_RX_CTL_SEP);
    ret = asix_write_cmd(net_dev, AX_CMD_WRITE_RX_CTL, rx_ctl, 0, 0, NULL);
    if (ret < 0)
    {
        LOG(5, "Reset RX_CTL failed: %d\n", ret);
        goto out;
    }

    // set multi-filter array.
    memset(u1Buf, 0, AX_MCAST_FILTER_SIZE);
    u1Buf[3] = 0x80;
    ret = asix_write_cmd(net_dev, AX_CMD_WRITE_MULTI_FILTER, 0, 0, AX_MCAST_FILTER_SIZE, u1Buf);
    if (ret < 0)
    {
        LOG(5, "Reset RX_CTL failed: %d\n", ret);
        goto out;
    }

    rx_ctl |= (AX_DEFAULT_RX_CTL|AX_RX_CTL_AM|AX_RX_CTL_SEP);
    ret = asix_write_cmd(net_dev, AX_CMD_WRITE_RX_CTL, rx_ctl, 0, 0, NULL);
    if (ret < 0)
    {
        LOG(5, "Reset RX_CTL failed: %d\n", ret);
        goto out;
    }

    // set multi-filter array.
    memset(u1Buf, 0, AX_MCAST_FILTER_SIZE);
    u1Buf[3] = 0x80;
    ret = asix_write_cmd(net_dev, AX_CMD_WRITE_MULTI_FILTER, 0, 0, AX_MCAST_FILTER_SIZE, u1Buf);
    if (ret < 0)
    {
        LOG(5, "Reset RX_CTL failed: %d\n", ret);
        goto out;
    }

    rx_ctl |= (AX_DEFAULT_RX_CTL|AX_RX_CTL_AM|AX_RX_CTL_SEP);
    ret = asix_write_cmd(net_dev, AX_CMD_WRITE_RX_CTL, rx_ctl, 0, 0, NULL);
    if (ret < 0)
    {
        LOG(5, "Reset RX_CTL failed: %d\n", ret);
        goto out;
    }
       
out:
    
    return ret;
}

static int Ax88172_open(struct eth_device *dev, bd_t *bis)
{
    usbnet *net_dev = dev->priv;
    unsigned char u1Buf[ETH_ALEN];
    int ret = 0;
   
    // Get the MAC address
    memset(u1Buf, 0, ETH_ALEN);
    ret = asix_read_cmd(net_dev, AX88172_CMD_READ_NODE_ID, 0, 0, ETH_ALEN, u1Buf);
    if (ret < 0) 
    {
        LOG(5, "Failed to read MAC address: %d\n", ret);
        goto out;
    }
    memcpy(net_dev->netdev->enetaddr, u1Buf, ETH_ALEN);
    
out:
    
    return ret;
}

void Ax88172_halt(struct eth_device *dev)
{
}

// Send a data block via Ethernet.
static int Ax88172_tx(struct eth_device *dev, volatile void *packet, int length)
{
    usbnet *net_dev = dev->priv;
    int count = 0;
    int pipe = net_dev->nOutPipe;
    int maxtry, partial;
    int result, stat;
    int this_xfer;
    char *ptr;

    this_xfer = length;
    if (this_xfer > BUFFER_SIZE)
    {
        LOG(7, "tx length %d exceeds the buffer limit\n", length);
        return 0;
    }

    ptr = (char *)packet;
    
    // while we have data left to transfer
    while (this_xfer)
    {
        // setup the retry counter
        maxtry = 10;

        // transfer the data
        LOG(7, "Bulk xfer 0x%x(%d) try #%d\n",
               (unsigned int)buf, this_xfer, 11 - maxtry);
        result = usb_bulk_msg(net_dev->udev, pipe, ptr,
                              this_xfer, &partial, USB_CNTL_TIMEOUT * 5);
        LOG(7, "bulk_msg returned %d xferred %d/%d\n",
               result, partial, this_xfer);
                   
        count += partial;
            
        if (net_dev->udev->status != 0)
        {
            // if we stall, we need to clear it before we go on
            if (net_dev->udev->status & USB_ST_STALLED)
            {
                LOG(5, "stalled ->clearing endpoint halt for pipe 0x%x\n", pipe);
                stat = net_dev->udev->status;
                usb_clear_halt(net_dev->udev, pipe);
                net_dev->udev->status = stat;
                if (this_xfer == partial)
                {
                    LOG(3, "bulk transferred with error %X, but data ok\n",
                           net_dev->udev->status);
                }
                    
                goto out;
            }
                
            if (net_dev->udev->status & USB_ST_NAK_REC)
            {
                LOG(5, "Device NAKed bulk_msg\n");
                goto out;
            }
                
            if (this_xfer == partial)
            {
                LOG(5, "bulk transferred with error %d, but data ok\n",
                       net_dev->udev->status);
                goto out;
            }
                
            // if our try counter reaches 0, bail out
            LOG(5, "bulk transferred with error %d, data %d\n",
                   net_dev->udev->status, partial);
            if (!maxtry--)
                goto out;
        }
            
        // update to show what data was transferred
        this_xfer -= partial;
        ptr += partial;
            
        // continue until this transfer is done
    }

out:
    return count;
}

// Get a data block via Ethernet
static int Ax88172_rx(struct eth_device *dev)
{
    usbnet *net_dev = dev->priv;
    int result;
    int actual_length = 0;
    
    result = usb_bulk_msg(net_dev->udev, net_dev->nInPipe, (void*)NetRxPackets[0],
                          ETH_FRAME_LEN, &actual_length, USB_CNTL_TIMEOUT * 5);
    if (result < 0)
    {
        if (net_dev->udev->status & USB_ST_STALLED)
        {
            LOG(3, "stalled ->clearing endpoint halt for pipe 0x%x\n", 
                   net_dev->nInPipe);
    
            // clear the STALL on the endpoint
            usb_clear_halt(net_dev->udev, net_dev->nInPipe);
        }
        else
        {
            LOG(5, "usb_bulk_msg error status %ld\n",
                   net_dev->udev->status);
        }
        return 0;
    }

    if ((actual_length > ETH_FRAME_LEN) || (actual_length == 0))
    {
        LOG(5, "Ax88172 Invalid Rx length = %d.\n", actual_length);
        return 0;
    }    

    // Pass Packets to upper Layer
    NetReceive(NetRxPackets[0], actual_length);

    return actual_length;
}

int Ax88172_init(struct usb_device *usbdev)
{
    struct eth_device *dev;
    
    memset(&AxDev, 0, sizeof(usbnet));
    AxDev.udev = usbdev;

    dev = (struct eth_device *)&AxEthDev;

    sprintf (dev->name, "AX88172#%x", AxDev.udev->descriptor.idVendor);

    //Init gpio bits.
    if ((usbdev->descriptor.idVendor == VENDOR_ID_AX88772) &&
        (usbdev->descriptor.idProduct == PRODUCT_ID_AX88172))
    {
        GPIO_DATA = AX8817X_GPIO;
    }   

    dev->priv = (void *)&AxDev;
    dev->init = Ax88172_open;
    dev->halt = Ax88172_halt;
    dev->send = Ax88172_tx;
    dev->recv = Ax88172_rx;

    eth_register(dev);
    Ax88172_start(dev);
    udelay (10 * 1000);
    AxDev.netdev = dev;
    return 0;

}

#endif /* COMMANDS & CFG_NET */
#endif /* CONFIG_DRIVER_AX8872 */
