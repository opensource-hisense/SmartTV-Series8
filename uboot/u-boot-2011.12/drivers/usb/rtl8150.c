
#include <common.h>
#include <command.h>
#include "rtl8150.h"

#ifdef CONFIG_DRIVER_RTL8150

#if defined(CONFIG_CMD_NET)

static rtl8150_t rtl8150Dev;

static int get_registers(rtl8150_t * dev, u16 indx, u16 size, void *data)
{
	return usb_control_msg(dev->udev, usb_rcvctrlpipe(dev->udev, 0),
			               RTL8150_REQ_GET_REGS, RTL8150_REQT_READ,
			               indx, 0, data, size, HZ / 2);
}

static int set_registers(rtl8150_t * dev, u16 indx, u16 size, void *data)
{
	return usb_control_msg(dev->udev, usb_sndctrlpipe(dev->udev, 0),
			               RTL8150_REQ_SET_REGS, RTL8150_REQT_WRITE,
			               indx, 0, data, size, HZ / 2);
}

static void set_ethernet_addr(rtl8150_t * dev)
{
    u8 u1Buf[6];
    char Ethbuf[32];
    
    get_registers(dev, IDR, 6, u1Buf);
    memcpy(dev->netdev->enetaddr, u1Buf, 6);

    // set Mac to environment address.
    memset(Ethbuf, 0, sizeof(32));
    sprintf(Ethbuf, "%02X:%02X:%02X:%02X:%02X:%02X", 
            u1Buf[0], u1Buf[1], u1Buf[2], u1Buf[3], u1Buf[4], u1Buf[5]);
    setenv("ethaddr", Ethbuf);

    printf("UsbNet Mac addr = %s.\n", Ethbuf);        
}

static int rtl8150_reset(rtl8150_t * dev)
{
	u8 data = 0x10;
	int i = HZ;

	set_registers(dev, CR, 1, &data);
	do {
		get_registers(dev, CR, 1, &data);
	} while ((data & 0x10) && --i);

	return (i > 0) ? 1 : 0;
}

static void disable_net_traffic(rtl8150_t * dev)
{
	u8 cr;

	get_registers(dev, CR, 1, &cr);
	cr &= 0xf3;
	set_registers(dev, CR, 1, &cr);
}

static int enable_net_traffic(rtl8150_t * dev, struct usb_device *usb)
{
	u8 cr, tcr, rcr, msr;

	if (!rtl8150_reset(dev))
	{
        LOG(3, "%s - device reset failed", __FUNCTION__);
	}
	/* RCR bit7=1 attach Rx info at the end;  =0 HW CRC (which is broken) */
	rcr = 0x9e;
	dev->rx_creg = cpu_to_le16(rcr);
	tcr = 0xd8;
	cr = 0x0c;
	
	set_registers(dev, RCR, 1, &rcr);
	set_registers(dev, TCR, 1, &tcr);
	set_registers(dev, CR, 1, &cr);
	get_registers(dev, MSR, 1, &msr);

	return 0;
}

static void rtl8150_start(rtl8150_t * dev)
{
	set_registers(dev, IDR, 6, dev->netdev->enetaddr);
	
	enable_net_traffic(dev, dev->udev);
}

static int rtl8150_probe(struct eth_device *dev, bd_t *bis)
{
	rtl8150_t *rtl_dev = dev->priv;
/*	
    if (!rtl8150_reset(rtl_dev))
    {
	    LOG(3, "couldn't reset the device\n");
	    return 0;
    }
	    
    set_ethernet_addr(rtl_dev);
*/    
    rtl8150_start(rtl_dev);
    return 1;
}

void rtl8150_halt(struct eth_device *dev)
{
	rtl8150_t *rtl_dev = dev->priv;
	if (!(rtl_dev->flags & RTL8150_UNPLUG))
		disable_net_traffic(rtl_dev);
}

/* Send a data block via Ethernet. */
static int rtl8150_tx(struct eth_device *dev, volatile void *packet, int length)
{
	rtl8150_t *rtl_dev = dev->priv;
	int count = 0;
	int result;
	int partial;
	int max_size;
	int maxtry;
	int this_xfer;
	int pipe;
	int stat;
	int buf_len;
	char *buf, *ptr;

	buf_len = (length < 60) ? 60 : length;
	buf_len = (buf_len & 0x3f) ? buf_len : buf_len + 1;
    ptr = (char*)packet;

	/* determine the maximum packet size for these transfers */
    pipe = usb_sndbulkpipe(rtl_dev->udev, 2);
	max_size = usb_maxpacket(rtl_dev->udev, pipe);
    
	/* while we have data left to transfer */
	while (buf_len)
	{
		/* calculate how long this will be -- maximum or a remainder */
		this_xfer = buf_len > max_size ? max_size : buf_len;
        memcpy((void *)rtl_dev->tx_buf, ptr, this_xfer);
        
		ptr += this_xfer;
		buf = (char*)(rtl_dev->tx_buf);

		/* setup the retry counter */
		maxtry = 10;

		/* set up the transfer loop */
		do {
			/* transfer the data */
			LOG(7, "Bulk xfer 0x%x(%d) try #%d\n",
				   (unsigned int)buf, this_xfer, 11 - maxtry);
			result = usb_bulk_msg(rtl_dev->udev, pipe, buf,
					              this_xfer, &partial, USB_CNTL_TIMEOUT*5);
			LOG(7, "bulk_msg returned %d xferred %d/%d\n",
				   result, partial, this_xfer);
				   
		    count += partial;
		    
			if (rtl_dev->udev->status != 0)
			{
				/* if we stall, we need to clear it before we go on */
				if (rtl_dev->udev->status & USB_ST_STALLED)
				{
					LOG(5, "stalled ->clearing endpoint halt for pipe 0x%x\n", pipe);
					stat = rtl_dev->udev->status;
					usb_clear_halt(rtl_dev->udev, pipe);
					rtl_dev->udev->status = stat;
					if (this_xfer == partial)
					{
						LOG(3, "bulk transferred with error %X, but data ok\n",
						       rtl_dev->udev->status);
					}
					
					goto out;
				}
				
				if (rtl_dev->udev->status & USB_ST_NAK_REC)
				{
					LOG(5, "Device NAKed bulk_msg\n");
					goto out;
				}
				
				if (this_xfer == partial)
				{
					LOG(5, "bulk transferred with error %d, but data ok\n",
					       rtl_dev->udev->status);
					goto out;
				}
				
				/* if our try counter reaches 0, bail out */
				LOG(5, "bulk transferred with error %d, data %d\n",
				       rtl_dev->udev->status, partial);
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
static int rtl8150_rx (struct eth_device *dev)
{
    rtl8150_t *rtl_dev = dev->priv;
    int partial;
    int result;
    int pipe = usb_rcvbulkpipe(rtl_dev->udev, 1);

    result = usb_bulk_msg(rtl_dev->udev, pipe, rtl_dev->rx_buf, 
        RTL8150_MTU, &partial, USB_CNTL_TIMEOUT * 5);

    if (result < 0)
    {
        if (rtl_dev->udev->status & USB_ST_STALLED)
        {
            LOG(3, "stalled ->clearing endpoint halt for pipe 0x%x\n", pipe);

            /* clear the STALL on the endpoint */
            usb_clear_halt(rtl_dev->udev, pipe);
        }
        else
        {
            LOG(5, "usb_bulk_msg error status %ld\n", rtl_dev->udev->status);
        }
        return 0;
    }

    /* Pass Packets to upper Layer */
    memcpy((void*)NetRxPackets[0], (const void *)rtl_dev->rx_buf, partial);
    NetReceive(NetRxPackets[0], partial);
    return partial;
}

int rtl8150_init (struct usb_device *usbdev)
{
	struct eth_device *dev;
	extern void *malloc(size_t);
	
	memset(&rtl8150Dev, 0, sizeof(rtl8150_t));
    rtl8150Dev.udev = usbdev;

	/* Find RTL8150 */

	dev = (struct eth_device *)malloc(sizeof *dev);

	sprintf (dev->name, "RTL8150#%x", rtl8150Dev.udev->descriptor.idVendor);

	dev->priv = (void *)&rtl8150Dev;
	dev->init = rtl8150_probe;
	dev->halt = rtl8150_halt;
	dev->send = rtl8150_tx;
    dev->recv = rtl8150_rx;

	eth_register(dev);

	udelay (10 * 1000);
	rtl8150Dev.netdev = dev;

        if (!rtl8150_reset(&rtl8150Dev))
        {
    	    LOG(3, "couldn't reset the device\n");
    	    return 0;
        }
    	    
        set_ethernet_addr(&rtl8150Dev);
	
	return 0;

}

#endif /* COMMANDS & CFG_NET */

#endif /* CONFIG_DRIVER_RTL8150 */
