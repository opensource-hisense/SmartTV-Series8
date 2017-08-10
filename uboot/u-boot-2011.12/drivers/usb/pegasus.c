#include <common.h>
#include <command.h>
#include <exports.h>

#include "pegasus.h"

#ifdef CONFIG_DRIVER_PEGASUSII

#if defined(CONFIG_CMD_NET)

#define BMSR_MEDIA (BMSR_10HALF | BMSR_10FULL | BMSR_100HALF | \
    BMSR_100FULL | BMSR_ANEGCAPABLE)

#define mdelay(n) ({unsigned long msec=(n); while (msec--) udelay(1000);})

extern volatile uchar * NetRxPackets []; /* Receive packets		*/

static pegasus_t pegasusDev;

static int get_registers( pegasus_t * dev, u16 indx, u16 size, void * data )
{ 
    return usb_control_msg(dev->udev, usb_rcvctrlpipe(dev->udev, 0), 
                PEGASUS_REQ_GET_REGS, PEGASUS_REQT_READ, 0, 
                indx, data, size, HZ / 2);
}

static int set_registers(pegasus_t * dev, u16 indx, u16 size, void * data)
{ 
    return usb_control_msg(dev->udev, usb_sndctrlpipe(dev->udev, 0), 
                PEGASUS_REQ_SET_REGS, PEGASUS_REQT_WRITE, 0,
                indx, data, size, HZ / 2);
}

static int set_register(pegasus_t * dev, u16 indx, u8 data) 
{ 
    return usb_control_msg(dev->udev, usb_sndctrlpipe(dev->udev, 0),
                PEGASUS_REQ_SET_REGS, PEGASUS_REQT_WRITE, 
                data, indx, &data, 1, HZ / 2);
}

static int read_mii_word( pegasus_t * dev, u8 phy, u8 indx, u16 * regd )
{
    int i;

    u8 data[4] =
    {
        phy, 0, 0, indx
    };

    u16 regdi;
    int ret = 0;

    LOG(5, "phy = 0x%x, indx = 0x%x\n", phy, indx);
    set_register(dev, PhyCtrl, 0);
    set_registers(dev, PhyAddr, sizeof(data), data);
    set_register(dev, PhyCtrl, (indx | PHY_READ));

    for (i = 0; i<REG_TIMEOUT; i++)
    {
        ret = get_registers(dev, PhyCtrl, 1, data);

        if (ret == -1)
            goto fail;

        if (data[0] & PHY_DONE)
            break;
    }

    if (i<REG_TIMEOUT)
    {
        ret = get_registers(dev, PhyData, 2, & regdi);

        * regd = le16_to_cpu(regdi);
        return ret;
    }

    fail:
    return ret;
}

static int write_mii_word( pegasus_t * dev, u8 phy, u8 indx, u16 regd )
{
    int i;

    u8 data[4] =
    {
        phy, 0, 0, indx
    };

    int ret = 0;

    data[1] = (u8)regd;
    data[2] = (u8)(regd >> 8);
    set_register(dev, PhyCtrl, 0);
    set_registers(dev, PhyAddr, sizeof(data), data);
    set_register(dev, PhyCtrl, (indx | PHY_WRITE));

    for (i = 0; i<REG_TIMEOUT; i++)
    {
        ret = get_registers(dev, PhyCtrl, 1, data);

        if (ret == -1)
            goto fail;

        if (data[0] & PHY_DONE)
            break;
    }

    if (i<REG_TIMEOUT)
        return ret;

    fail:
    return -1;
}

static int read_eprom_word( pegasus_t * dev, u8 index, u16 * retdata )
{
    int i;

    u8 tmp;
    u16 retdatai;
    int ret = 0;

    set_register(dev, EpromCtrl, 0);
    set_register(dev, EpromOffset, index);
    set_register(dev, EpromCtrl, EPROM_READ);

    for (i = 0; i<REG_TIMEOUT; i++)
    {
        ret = get_registers(dev, EpromCtrl, 1, & tmp);

        if (tmp & EPROM_DONE)
            break;

        if (ret == -1)
            goto fail;
    }

    if (i<REG_TIMEOUT)
    {
        ret = get_registers(dev, EpromData, 2, & retdatai);

        * retdata = le16_to_cpu(retdatai);
        return ret;
    }

    fail:
    return -1;
}

static void get_node_id( pegasus_t * dev, u8 * id )
{
    int i;

    u16 w16=0;

    for (i = 0; i<3; i++)
    {
        read_eprom_word(dev, i, & w16);

        ((u16 * )id) [i] = w16;
    }
}

static void set_ethernet_addr( pegasus_t * dev )
{
    u8 u1Buf[6];
    char Ethbuf[32];

    if (dev->feature & PEGASUS_II) 
    {		
        get_registers(dev, 0x10, 6, u1Buf);	
    } 
    else 
    {		
        get_node_id(dev, u1Buf);
        set_registers(dev, EthID, 6, u1Buf);
    }	
    memcpy(dev->netdev->enetaddr, u1Buf, 6);

    // set Mac to environment address.
    memset(Ethbuf, 0, sizeof(32));
    sprintf(Ethbuf, "%02X:%02X:%02X:%02X:%02X:%02X", 
            u1Buf[0], u1Buf[1], u1Buf[2], u1Buf[3], u1Buf[4], u1Buf[5]);
    setenv("ethaddr", Ethbuf);

    printf("UsbNet Mac addr = %s.\n", Ethbuf);    
}

static int pegasus_reset( pegasus_t * dev )
{
    u8 data = 0x8;

    int i;

    set_register(dev, EthCtrl1, data);

    for (i = 0; i<REG_TIMEOUT; i++)
    {
        get_registers(dev, EthCtrl1, 1, & data);

        if (~data & 0x08)
        {
            set_register(dev, Gpio1, 0x26);

            set_register(dev, Gpio0, dev->feature);
            set_register(dev, Gpio0, DEFAULT_GPIO_SET);
            break;
        }
    }

    if (i == REG_TIMEOUT)
        return -1;

    if (dev->udev->descriptor.idVendor == VENDOR_LINKSYS || dev->udev->descriptor.idVendor == VENDOR_DLINK)
    {
        set_register(dev, Gpio0, 0x24);

        set_register(dev, Gpio0, 0x26);
    }

    if (dev->udev->descriptor.idVendor == VENDOR_ELCON)
    {
        u16 auxmode;

        read_mii_word(dev, 3, 0x1b, & auxmode);
        write_mii_word(dev, 3, 0x1b, auxmode | 4);
    }

    return 0;
}

static int pegasus_enable_net_traffic( pegasus_t * dev, struct usb_device * usb )
{
    u16 linkpart;

    u8 data[4];
    int ret;

    if (0>pegasus_reset(dev))
    {
        LOG(3, "couldn't reset the device\n");

        return 0;
    }

    read_mii_word(dev, dev->phy, MII_LPA, & linkpart);
    data[0] = 0xc9;
    data[1] = 0;

    if (linkpart&(ADVERTISE_100FULL | ADVERTISE_10FULL))
        data[1] |= 0x20; /* set full duplex */

    if (linkpart&(ADVERTISE_100FULL | ADVERTISE_100HALF))
        data[1] |= 0x10; /* set 100 Mbps */

    data[2] = 0x01;

    ret = set_registers(dev, EthCtrl0, 3, data);

    if (dev->udev->descriptor.idVendor == VENDOR_LINKSYS || dev->udev->descriptor.idVendor == VENDOR_LINKSYS2
            || dev->udev->descriptor.idVendor == VENDOR_DLINK)
    {
        u16 auxmode;

        read_mii_word(dev, 0, 0x1b, & auxmode);
        write_mii_word(dev, 0, 0x1b, auxmode | 4);
    }

    return ret;
}

static u8 mii_phy_probe( pegasus_t * dev )
{
    int i;

    u16 tmp;

    for (i = 0; i<32; i++)
    {
        read_mii_word(dev, i, MII_BMSR, & tmp);

        if (tmp == 0 || tmp == 0xffff || (tmp& BMSR_MEDIA) == 0)
            continue;
        else
            return i;
    }

    return 0xff;
}

static void setup_pegasus_II( pegasus_t * dev )
{
    u8 data = 0xa5;

    set_register(dev, Reg1d, 0);
    set_register(dev, Reg7b, 1);
    mdelay(100);

    set_register(dev, Reg7b, 2);

    set_register(dev, 0x83, data);
    get_registers(dev, 0x83, 1, & data);

    if (data == 0xa5)
    {
        dev->chip = 0x8513;
    }
    else
    {
        dev->chip = 0;
    }

    set_register(dev, 0x80, 0xc0);
    set_register(dev, 0x83, 0xff);
    set_register(dev, 0x84, 0x01);

    set_register(dev, Reg81, 2);
}

static void pegasus_start( pegasus_t * dev )
{
    // support ADMtek 8511, 8513, 8515
    dev->feature = 0xC0000024;

    if (0>pegasus_reset(dev))
    {
        LOG(3, "couldn't reset the device\n");

        return;
    }

    set_ethernet_addr(dev);

    if (dev->feature & PEGASUS_II)
    {
        LOG(3, "Setup Pegasus II specific registers\n");

        setup_pegasus_II(dev);
    }

    dev->phy = mii_phy_probe(dev);

    if (dev->phy == 0xff)
    {
        dev->phy = 1;

        LOG(3, "can't locate MII phy, using default = %d.\n", dev->phy);
    }

    LOG(5, "Locate MII phy = %d.\n", dev->phy);

    pegasus_enable_net_traffic(dev, dev->udev);

    set_register(dev, WakeupControl, 0x04);

    pegasus_enable_net_traffic(dev, dev->udev);
}

static int pegasus_probe( struct eth_device * dev, bd_t * bis ) 
{ 
    return 1;
}

void pegasus_halt( struct eth_device * dev ) 
{ 
    LOG(1, "pegasus_halt\n");
}

/* Send a data block via Ethernet. */
static int pegasus_tx( struct eth_device * dev, volatile void * packet, int length )
{
    pegasus_t * pegasusdev = dev->priv;

    int count = 0;
    int result;
    int partial;
    int max_size;
    int maxtry;
    int this_xfer;
    unsigned long pipe;
    int stat;
    int buf_len;
    void * buf;

    buf_len = ((length + 2) & 0x3f) ? (length + 2) : (length + 3);
    pegasusdev->tx_buf[0] = length & 0xFF;
    pegasusdev->tx_buf[1] = (length >> 8) & 0xFF;
    memcpy((void * )(pegasusdev->tx_buf + 2), (const void * )packet, length);

    /* determine the maximum packet size for these transfers */
    pipe = usb_sndbulkpipe(pegasusdev->udev, 2);
    max_size = usb_maxpacket(pegasusdev->udev, pipe);
    buf = (void * )pegasusdev->tx_buf;

    /* while we have data left to transfer */
    while (buf_len)
    {
        /* calculate how long this will be -- maximum or a remainder */
        this_xfer = buf_len>max_size ? max_size : buf_len;

        /* setup the retry counter */
        maxtry = 10;

        /* set up the transfer loop */
        do
        {
            /* transfer the data */
            LOG(7, "Bulk xfer 0x%x(%d) try #%d\n", (unsigned int)buf, this_xfer, 11 - maxtry);

            result = usb_bulk_msg(pegasusdev->udev, pipe, buf, this_xfer, & partial, USB_CNTL_TIMEOUT * 5);
            LOG(7, "bulk_msg returned %d xferred %d/%d\n", result, partial, this_xfer);

            count += partial;

            if (pegasusdev->udev->status != 0)
            {
                /* if we stall, we need to clear it before we go on */
                if (pegasusdev->udev->status & USB_ST_STALLED)
                {
                    LOG(5, "stalled ->clearing endpoint halt for pipe 0x%x\n", pipe);

                    stat = pegasusdev->udev->status;
                    usb_clear_halt(pegasusdev->udev, pipe);
                    pegasusdev->udev->status = stat;

                    if (this_xfer == partial)
                    {
                        LOG(3, "bulk transferred with error %X, but data ok\n", pegasusdev->udev->status);
                    }

                    goto out;
                }

                if (pegasusdev->udev->status & USB_ST_NAK_REC)
                {
                    LOG(5, "Device NAKed bulk_msg\n");

                    goto out;
                }

                if (this_xfer == partial)
                {
                    LOG(5, "bulk transferred with error %d, but data ok\n", pegasusdev->udev->status);

                    goto out;
                }

                /* if our try counter reaches 0, bail out */
                LOG(5, "bulk transferred with error %d, data %d\n", pegasusdev->udev->status, partial);

                if (!maxtry--)
                    goto out;
            }

            /* update to show what data was transferred */
            this_xfer -= partial;
            buf_len -= partial;
            buf += partial;

        /* continue until this transfer is done */
        } while (this_xfer);
    }

    out:
    return count;
}

/* Get a data block via Ethernet */
static int pegasus_rx( struct eth_device * dev )
{
    pegasus_t * pegasusdev = dev->priv;

    int result = 0;
    unsigned long pipe = usb_rcvbulkpipe(pegasusdev->udev, 1);
    int rx_status;
    int count;
    u8 * buf = pegasusdev->rx_buf;
    u16 pkt_len = 0;

    result = 0;
    result = usb_bulk_msg(pegasusdev->udev, pipe, buf, (PEGASUS_MTU + 8), & count, USB_CNTL_TIMEOUT * 5);

    if (result<0)
    {
        if (pegasusdev->udev->status & USB_ST_STALLED)
        {
            LOG(3, "stalled ->clearing endpoint halt for pipe 0x%x\n", pipe);

            /* clear the STALL on the endpoint */
            usb_clear_halt(pegasusdev->udev, pipe);
        }
        else
        {
            LOG(5, "usb_bulk_msg error status %d\n", pegasusdev->udev->status);
        }

        return 0;
    }

    if (!count || count<4)
    {
        LOG(5, "ERR: count = %d\n", count);

        return 0;
    }

    rx_status = buf[count - 2];

    if (rx_status & 0x1e)
    {
        LOG(5, "ERR: rx_status = %d\n", rx_status);

        return 0;
    }

    if (pegasusdev->chip == 0x8513)
    {
        pkt_len = (buf[0] | (buf[1] << 8));

        pkt_len &= 0x0fff;
        buf += 2;
        LOG(5, "0x8513: pkt_len = %d\n", pkt_len);
    }
    else
    {
        pkt_len = buf[count - 3] << 8;

        pkt_len += buf[count - 4];
        pkt_len &= 0xfff;
        pkt_len -= 8;
        LOG(5, "pkt_len = %d\n", pkt_len);
    }

    /*
     * If the packet is unreasonably long, quietly drop it rather than
     * kernel panicing by calling skb_put.
     */
    if (pkt_len>PEGASUS_MTU)
        return 0;

    /* Pass Packets to upper Layer */
    memcpy((void * )NetRxPackets[0], buf, pkt_len);
    NetReceive(NetRxPackets[0], pkt_len);

    return pkt_len;
}

int pegasus_init( struct usb_device * usbdev )
{
    struct eth_device * dev;

    memset(& pegasusDev, 0, sizeof(pegasus_t));
    pegasusDev.udev = usbdev;

    /* Find PEGASUS */
    dev = (struct eth_device * )malloc(sizeof * dev);

    sprintf(dev->name, "PEGASUS#%x", pegasusDev.udev->descriptor.idVendor);

    dev->priv = (void * )& pegasusDev;
    dev->init = pegasus_probe;
    dev->halt = pegasus_halt;
    dev->send = pegasus_tx;
    dev->recv = pegasus_rx;

    eth_register(dev);

    pegasus_start(& pegasusDev);

    udelay(10 * 1000);
    pegasusDev.netdev = dev;
    return 0;
}

#endif /* COMMANDS & CFG_NET */
#endif     /* CONFIG_DRIVER_PEGASUSII */
