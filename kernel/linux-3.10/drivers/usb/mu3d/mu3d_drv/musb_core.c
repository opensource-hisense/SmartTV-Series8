/*
 * MUSB OTG driver core code
 *
 * Copyright 2005 Mentor Graphics Corporation
 * Copyright (C) 2005-2006 by Texas Instruments
 * Copyright (C) 2006-2007 Nokia Corporation
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
 * NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/*
 * Inventra (Multipoint) Dual-Role Controller Driver for Linux.
 *
 * This consists of a Host Controller Driver (HCD) and a peripheral
 * controller driver implementing the "Gadget" API; OTG support is
 * in the works.  These are normal Linux-USB controller drivers which
 * use IRQs and have no dedicated thread.
 *
 * This version of the driver has only been used with products from
 * Texas Instruments.  Those products integrate the Inventra logic
 * with other DMA, IRQ, and bus modules, as well as other logic that
 * needs to be reflected in this driver.
 *
 *
 * NOTE:  the original Mentor code here was pretty much a collection
 * of mechanisms that don't seem to have been fully integrated/working
 * for *any* Linux kernel version.  This version aims at Linux 2.6.now,
 * Key open issues include:
 *
 *  - Lack of host-side transaction scheduling, for all transfer types.
 *    The hardware doesn't do it; instead, software must.
 *
 *    This is not an issue for OTG devices that don't support external
 *    hubs, but for more "normal" USB hosts it's a user issue that the
 *    "multipoint" support doesn't scale in the expected ways.  That
 *    includes DaVinci EVM in a common non-OTG mode.
 *
 *      * Control and bulk use dedicated endpoints, and there's as
 *        yet no mechanism to either (a) reclaim the hardware when
 *        peripherals are NAKing, which gets complicated with bulk
 *        endpoints, or (b) use more than a single bulk endpoint in
 *        each direction.
 *
 *        RESULT:  one device may be perceived as blocking another one.
 *
 *      * Interrupt and isochronous will dynamically allocate endpoint
 *        hardware, but (a) there's no record keeping for bandwidth;
 *        (b) in the common case that few endpoints are available, there
 *        is no mechanism to reuse endpoints to talk to multiple devices.
 *
 *        RESULT:  At one extreme, bandwidth can be overcommitted in
 *        some hardware configurations, no faults will be reported.
 *        At the other extreme, the bandwidth capabilities which do
 *        exist tend to be severely undercommitted.  You can't yet hook
 *        up both a keyboard and a mouse to an external USB hub.
 */

/*
 * This gets many kinds of configuration information:
 *	- Kconfig for everything user-configurable
 *	- platform_device for addressing, irq, and platform_data
 *	- platform_data is mostly for board-specific informarion
 *	  (plus recentrly, SOC or family details)
 *
 * Most of the conditional compilation will (someday) vanish.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/kobject.h>
#include <linux/prefetch.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/of_address.h>

//#include <linux/usb/hcd.h>
#include "musb_core.h"
#include "musb_comm.h"

#include "SSUSB_DEV_c_header.h"
#include "../mu3d_hal/mu3d_hal_osal.h"
#include "../mu3d_hal/mu3d_hal_usb_drv.h"
#include "ssusb_qmu.h"
#ifdef CONFIG_64BIT
#include <linux/soc/mediatek/platform.h>
#include <linux/soc/mediatek/x_typedef.h>
#else
#include <mach/platform.h>
#include <x_typedef.h>
#endif


void __iomem *pSSUSBU3DBaseIoMap;
void __iomem  *pSsusbDDmaIoMap;



#define TA_WAIT_BCON(m) max_t(int, (m)->a_wait_bcon, OTG_TIME_A_WAIT_BCON)


#define DRIVER_AUTHOR "Mentor Graphics, Texas Instruments, Nokia"
#define DRIVER_DESC "Inventra Dual-Role USB Controller Driver"

#define MUSB_VERSION "6.0"

#define DRIVER_INFO DRIVER_DESC ", v" MUSB_VERSION

#define MUSB_DRIVER_NAME "musb-hdrc"
const char musb_driver_name[] = MUSB_DRIVER_NAME;

MODULE_DESCRIPTION(DRIVER_INFO);
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:" MUSB_DRIVER_NAME);


/*-------------------------------------------------------------------------*/

static inline struct musb *dev_to_musb(struct device *dev)
{
	return dev_get_drvdata(dev);
}

/*-------------------------------------------------------------------------*/

#if 0
//#ifndef CONFIG_BLACKFIN
static int musb_ulpi_read(struct otg_transceiver *otg, u32 offset)
{
	void __iomem *addr = otg->io_priv;
	int	i = 0;
	u8	r;
	u8	power;

	/* Make sure the transceiver is not in low power mode */
	power = musb_readb(addr, MUSB_POWER);
	power &= ~MUSB_POWER_SUSPENDM;
	musb_writeb(addr, MUSB_POWER, power);

	/* REVISIT: musbhdrc_ulpi_an.pdf recommends setting the
	 * ULPICarKitControlDisableUTMI after clearing POWER_SUSPENDM.
	 */

	musb_writeb(addr, MUSB_ULPI_REG_ADDR, (u8)offset);
	musb_writeb(addr, MUSB_ULPI_REG_CONTROL,
			MUSB_ULPI_REG_REQ | MUSB_ULPI_RDN_WR);

	while (!(musb_readb(addr, MUSB_ULPI_REG_CONTROL)
				& MUSB_ULPI_REG_CMPLT)) {
		i++;
		if (i == 10000)
			return -ETIMEDOUT;

	}
	r = musb_readb(addr, MUSB_ULPI_REG_CONTROL);
	r &= ~MUSB_ULPI_REG_CMPLT;
	musb_writeb(addr, MUSB_ULPI_REG_CONTROL, r);

	return musb_readb(addr, MUSB_ULPI_REG_DATA);
}

static int musb_ulpi_write(struct otg_transceiver *otg,
		u32 offset, u32 data)
{
	void __iomem *addr = otg->io_priv;
	int	i = 0;
	u8	r = 0;
	u8	power;

	/* Make sure the transceiver is not in low power mode */
	power = musb_readb(addr, MUSB_POWER);
	power &= ~MUSB_POWER_SUSPENDM;
	musb_writeb(addr, MUSB_POWER, power);

	musb_writeb(addr, MUSB_ULPI_REG_ADDR, (u8)offset);
	musb_writeb(addr, MUSB_ULPI_REG_DATA, (u8)data);
	musb_writeb(addr, MUSB_ULPI_REG_CONTROL, MUSB_ULPI_REG_REQ);

	while (!(musb_readb(addr, MUSB_ULPI_REG_CONTROL)
				& MUSB_ULPI_REG_CMPLT)) {
		i++;
		if (i == 10000)
			return -ETIMEDOUT;
	}

	r = musb_readb(addr, MUSB_ULPI_REG_CONTROL);
	r &= ~MUSB_ULPI_REG_CMPLT;
	musb_writeb(addr, MUSB_ULPI_REG_CONTROL, r);

	return 0;
}
#else
#define musb_ulpi_read		NULL
#define musb_ulpi_write		NULL
#endif



/*-------------------------------------------------------------------------*/

/* for high speed test mode; see USB 2.0 spec 7.1.20 */
static const u8 musb_test_packet[53] = {
	/* implicit SYNC then DATA0 to start */

	/* JKJKJKJK x9 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/* JJKKJJKK x8 */
	0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
	/* JJJJKKKK x8 */
	0xee, 0xee, 0xee, 0xee, 0xee, 0xee, 0xee, 0xee,
	/* JJJJJJJKKKKKKK x8 */
	0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	/* JJJJJJJK x8 */
	0x7f, 0xbf, 0xdf, 0xef, 0xf7, 0xfb, 0xfd,
	/* JKKKKKKK x10, JK */
	0xfc, 0x7e, 0xbf, 0xdf, 0xef, 0xf7, 0xfb, 0xfd, 0x7e

	/* implicit CRC16 then EOP to end */
};

void musb_load_testpacket(struct musb *musb)
{
	u32 maxp;
	
	maxp = musb->endpoints->max_packet_sz_tx;
	mu3d_hal_write_fifo( 0,sizeof(musb_test_packet),(DEV_UINT8 *)musb_test_packet, maxp);
}

/*-------------------------------------------------------------------------*/

/*
 * Handles OTG hnp timeouts, such as b_ase0_brst
 */
void musb_otg_timer_func(unsigned long data)
{
	struct musb	*musb = (struct musb *)data;
	unsigned long	flags;

	spin_lock_irqsave(&musb->lock, flags);
	switch (musb->xceiv->state) {
	case OTG_STATE_B_WAIT_ACON:
		dev_dbg(musb->controller, "HNP: b_wait_acon timeout; back to b_peripheral\n");
		musb_g_disconnect(musb);
		musb->xceiv->state = OTG_STATE_B_PERIPHERAL;
		musb->is_active = 0;
		break;
	case OTG_STATE_A_SUSPEND:
	case OTG_STATE_A_WAIT_BCON:
		dev_dbg(musb->controller, "HNP: %s timeout\n",
			usb_otg_state_string(musb->xceiv->state));
		musb_platform_set_vbus(musb, 0);
		musb->xceiv->state = OTG_STATE_A_WAIT_VFALL;
		break;
	default:
		dev_dbg(musb->controller, "HNP: Unhandled mode %s\n",
			usb_otg_state_string(musb->xceiv->state));
	}
	musb->ignore_disconnect = 0;
	spin_unlock_irqrestore(&musb->lock, flags);
}

/*
 * Stops the HNP transition. Caller must take care of locking.
 */
void musb_hnp_stop(struct musb *musb)
{
	struct usb_hcd	*hcd = musb_to_hcd(musb);
	u32	reg;

	dev_dbg(musb->controller, "HNP: stop from %s\n", usb_otg_state_string(musb->xceiv->state));

	switch (musb->xceiv->state) {
	case OTG_STATE_A_PERIPHERAL:
		musb_g_disconnect(musb);
		dev_dbg(musb->controller, "HNP: back to %s\n",
			usb_otg_state_string(musb->xceiv->state));
		break;
	case OTG_STATE_B_HOST:
		dev_dbg(musb->controller, "HNP: Disabling HR\n");
		hcd->self.is_b_host = 0;
		musb->xceiv->state = OTG_STATE_B_PERIPHERAL;
		MUSB_DEV_MODE(musb);
		//reg = musb_readb(mbase, MUSB_POWER);
		reg = os_readl(U3D_POWER_MANAGEMENT);
		reg |= SUSPENDM_ENABLE;
		os_writel(U3D_POWER_MANAGEMENT, reg);
		/* REVISIT: Start SESSION_REQUEST here? */
		break;
	default:
		dev_dbg(musb->controller, "HNP: Stopping in unknown state %s\n",
			usb_otg_state_string(musb->xceiv->state));
	}

	/*
	 * When returning to A state after HNP, avoid hub_port_rebounce(),
	 * which cause occasional OPT A "Did not receive reset after connect"
	 * errors.
	 */
	musb->port1_status &= ~(USB_PORT_STAT_C_CONNECTION << 16);
}

/*
 * Interrupt Service Routine to record USB "global" interrupts.
 * Since these do not happen often and signify things of
 * paramount importance, it seems OK to check them individually;
 * the order of the tests is specified in the manual
 *
 * @param musb instance pointer
 * @param int_usb register contents
 * @param devctl
 * @param power
 */

static irqreturn_t musb_stage0_irq(struct musb *musb, u32 int_usb,
				u8 devctl, u8 power)
{
//	struct usb_otg *otg = musb->xceiv->otg;
	irqreturn_t handled = IRQ_NONE;

	dev_dbg(musb->controller, "<== Power=%02x, DevCtl=%02x, int_usb=0x%x\n", power, devctl,
		int_usb);

	/* in host mode, the peripheral may issue remote wakeup.
	 * in peripheral mode, the host may resume the link.
	 * spurious RESUME irqs happen too, paired with SUSPEND.
	 */
	if (int_usb & RESUME_INTR) {
		handled = IRQ_HANDLED;
		dev_dbg(musb->controller, "RESUME (%s)\n", usb_otg_state_string(musb->xceiv->state));

		//We implement device mode only.
		switch (musb->xceiv->state) {
			case OTG_STATE_A_SUSPEND:
				/* possibly DISCONNECT is upcoming */
				musb->xceiv->state = OTG_STATE_A_HOST;
				usb_hcd_resume_root_hub(musb_to_hcd(musb));
				break;
			case OTG_STATE_B_WAIT_ACON:
			case OTG_STATE_B_PERIPHERAL:
				/* disconnect while suspended?  we may
				 * not get a disconnect irq...
				 */
				if ((devctl & USB_DEVCTL_VBUSVALID)
						!= (3 << USB_DEVCTL_VBUS_OFFSET)
						) {
					musb->int_usb |= DISCONN_INTR;
					musb->int_usb &= ~SUSPEND_INTR;
					break;
				}
				musb_g_resume(musb);
				break;
			case OTG_STATE_B_IDLE:
				musb->int_usb &= ~SUSPEND_INTR;
				break;
			default:
				WARNING("bogus %s RESUME (%s)\n",
					"peripheral",
					usb_otg_state_string(musb->xceiv->state));
		}
	}

	/* see manual for the order of the tests */
	if (int_usb & SESSION_REQ_INTR) {
		if ((devctl & USB_DEVCTL_VBUSMASK) == USB_DEVCTL_VBUSVALID
				&& (devctl & USB_DEVCTL_BDEVICE)) {
			dev_dbg(musb->controller, "SessReq while on B state\n");
			return IRQ_HANDLED;
		}

		dev_dbg(musb->controller, "SESSION_REQUEST (%s)\n",
			usb_otg_state_string(musb->xceiv->state));

		/* IRQ arrives from ID pin sense or (later, if VBUS power
		 * is removed) SRP.  responses are time critical:
		 *  - turn on VBUS (with silicon-specific mechanism)
		 *  - go through A_WAIT_VRISE
		 *  - ... to A_WAIT_BCON.
		 * a_wait_vrise_tmout triggers VBUS_ERROR transitions
		 */
		//os_writel(mregs + MAC_DEVICE_CONTROL, devctl & USB_DEVCTL_SESSION);
		musb->ep0_stage = MUSB_EP0_START;
		musb->xceiv->state = OTG_STATE_A_IDLE;
		MUSB_HST_MODE(musb);
		musb_platform_set_vbus(musb, 1);

		handled = IRQ_HANDLED;
	}

	if (int_usb & VBUSERR_INTR) {
		int	ignore = 0;

		/* During connection as an A-Device, we may see a short
		 * current spikes causing voltage drop, because of cable
		 * and peripheral capacitance combined with vbus draw.
		 * (So: less common with truly self-powered devices, where
		 * vbus doesn't act like a power supply.)
		 *
		 * Such spikes are short; usually less than ~500 usec, max
		 * of ~2 msec.  That is, they're not sustained overcurrent
		 * errors, though they're reported using VBUSERROR irqs.
		 *
		 * Workarounds:  (a) hardware: use self powered devices.
		 * (b) software:  ignore non-repeated VBUS errors.
		 *
		 * REVISIT:  do delays from lots of DEBUG_KERNEL checks
		 * make trouble here, keeping VBUS < 4.4V ?
		 */
		switch (musb->xceiv->state) {
		case OTG_STATE_A_HOST:
			/* recovery is dicey once we've gotten past the
			 * initial stages of enumeration, but if VBUS
			 * stayed ok at the other end of the link, and
			 * another reset is due (at least for high speed,
			 * to redo the chirp etc), it might work OK...
			 */
		case OTG_STATE_A_WAIT_BCON:
		case OTG_STATE_A_WAIT_VRISE:
			if (musb->vbuserr_retry) {
				musb->vbuserr_retry--;
				ignore = 1;
				devctl |= USB_DEVCTL_SESSION;
				//os_writel(mregs + MAC_DEVICE_CONTROL, devctl & USB_DEVCTL_SESSION);
			} else {
				musb->port1_status |=
					  USB_PORT_STAT_OVERCURRENT
					| (USB_PORT_STAT_C_OVERCURRENT << 16);
			}
			break;
		default:
			break;
		}

		dev_dbg(musb->controller, "VBUS_ERROR in %s (%02x, %s), retry #%d, port1 %08x\n",
				usb_otg_state_string(musb->xceiv->state),
				devctl,
				({ char *s;
				switch (devctl & USB_DEVCTL_VBUSMASK) {
				case 0 << USB_DEVCTL_VBUS_OFFSET:
					s = "<SessEnd"; break;
				case 1 << USB_DEVCTL_VBUS_OFFSET:
					s = "<AValid"; break;
				case 2 << USB_DEVCTL_VBUS_OFFSET:
					s = "<VBusValid"; break;
				/* case 3 << MUSB_DEVCTL_VBUS_SHIFT: */
				default:
					s = "VALID"; break;
				}; s; }),
				VBUSERR_RETRY_COUNT - musb->vbuserr_retry,
				musb->port1_status);

		/* go through A_WAIT_VFALL then start a new session */
		if (!ignore)
			musb_platform_set_vbus(musb, 0);
		handled = IRQ_HANDLED;
	}

	if (int_usb & SUSPEND_INTR) {
		dev_dbg(musb->controller, "SUSPEND (%s) devctl %02x power %02x\n",
			usb_otg_state_string(musb->xceiv->state), devctl, power);
		handled = IRQ_HANDLED;

		switch (musb->xceiv->state) {
		case OTG_STATE_A_PERIPHERAL:
			/* We also come here if the cable is removed, since
			 * this silicon doesn't report ID-no-longer-grounded.
			 *
			 * We depend on T(a_wait_bcon) to shut us down, and
			 * hope users don't do anything dicey during this
			 * undesired detour through A_WAIT_BCON.
			 */
			musb_hnp_stop(musb);
			usb_hcd_resume_root_hub(musb_to_hcd(musb));
			//musb_root_disconnect(musb); //I don't port virthub now.
			musb_platform_try_idle(musb, jiffies
					+ msecs_to_jiffies(musb->a_wait_bcon
						? : OTG_TIME_A_WAIT_BCON));

			break;
		case OTG_STATE_B_IDLE:
			if (!musb->is_active)
				break;
		case OTG_STATE_B_PERIPHERAL:
			musb_g_suspend(musb);
			musb->is_active = is_otg_enabled(musb)
					&& musb->xceiv->gadget->b_hnp_enable;
			if (musb->is_active) {
				musb->xceiv->state = OTG_STATE_B_WAIT_ACON;
				dev_dbg(musb->controller, "HNP: Setting timer for b_ase0_brst\n");
				mod_timer(&musb->otg_timer, jiffies
					+ msecs_to_jiffies(
							OTG_TIME_B_ASE0_BRST));
			}
			break;
		case OTG_STATE_A_WAIT_BCON:
			if (musb->a_wait_bcon != 0)
				musb_platform_try_idle(musb, jiffies
					+ msecs_to_jiffies(musb->a_wait_bcon));
			break;
		case OTG_STATE_A_HOST:
			musb->xceiv->state = OTG_STATE_A_SUSPEND;
			musb->is_active = is_otg_enabled(musb)
					&& musb->xceiv->host->b_hnp_enable;
			break;
		case OTG_STATE_B_HOST:
			/* Transition to B_PERIPHERAL, see 6.8.2.6 p 44 */
			dev_dbg(musb->controller, "REVISIT: SUSPEND as B_HOST\n");
			break;
		default:
			/* "should not happen" */
			musb->is_active = 0;
			break;
		}
	}

	if (int_usb & CONN_INTR) {
		struct usb_hcd *hcd = musb_to_hcd(musb);
		u32 int_en = 0;

		handled = IRQ_HANDLED;
		musb->is_active = 1;
		set_bit(1, &hcd->flags); //HCD_FLAG_SAW_IRQ =1 

		musb->ep0_stage = MUSB_EP0_START;
		os_printk(K_DEBUG, "----- ep0 state: MUSB_EP0_START\n");

		/* flush endpoints when transitioning from Device Mode */
		if (is_peripheral_active(musb)) {
			/* REVISIT HNP; just force disconnect */
		}
		//musb_writew(musb->mregs, MUSB_INTRTXE, musb->epmask);
		//musb_writew(musb->mregs, MUSB_INTRRXE, musb->epmask & 0xfffe);
#ifdef USE_SSUSB_QMU //For QMU, we don't enable EP interrupts.

	//os_writew(regs + U3D_EPIESR_EPRIER, os_readw(regs + U3D_EPIESR_EPRIER)|EP0ISR);//enable EPN-0 interrupt
	os_writel(U3D_EPIESR, EP0ISR);//enable 0 interrupt	

#else
	//os_writew(regs + U3D_EPIESR_EPTIER, os_readw(regs + U3D_EPIESR_EPTIER)|musb->epmask);//enable EPN interrupt
	//os_writew(regs + U3D_EPIESR_EPRIER, os_readw(regs + U3D_EPIESR_EPRIER)|musb->epmask & EPRISR);//enable EPN-0 interrupt
//	os_writel(U3D_EPIESR, musb->epmask | ((musb->epmask << 16) & EPRISR));//enable EPN-0 interrupt	
    os_writel(U3D_EPIESR, 0xFFFF | ((0XFFFE << 16) ));	
#endif //	#ifndef USE_SSUSB_QMU
	
		int_en = SUSPEND_INTR_EN|RESUME_INTR_EN|RESET_INTR_EN|CONN_INTR_EN|DISCONN_INTR_EN;

		os_writel(U3D_COMMON_USB_INTR_ENABLE, int_en);

		//musb_writeb(musb->mregs, MUSB_INTRUSBE, 0xf7);
		
		musb->port1_status &= ~(USB_PORT_STAT_LOW_SPEED
					|USB_PORT_STAT_HIGH_SPEED
					|USB_PORT_STAT_ENABLE
					);
		musb->port1_status |= USB_PORT_STAT_CONNECTION
					|(USB_PORT_STAT_C_CONNECTION << 16);

		/* high vs full speed is just a guess until after reset */
		if (devctl & USB_DEVCTL_LS_DEV)
			musb->port1_status |= USB_PORT_STAT_LOW_SPEED;

		/* indicate new connection to OTG machine */
		switch (musb->xceiv->state) {
		case OTG_STATE_B_PERIPHERAL:
			if (int_usb & SUSPEND_INTR) {
				dev_dbg(musb->controller, "HNP: SUSPEND+CONNECT, now b_host\n");
				int_usb &= ~SUSPEND_INTR;
				goto b_host;
			} else
				dev_dbg(musb->controller, "CONNECT as b_peripheral???\n");
			break;
		case OTG_STATE_B_WAIT_ACON:
			dev_dbg(musb->controller, "HNP: CONNECT, now b_host\n");
b_host:
			musb->xceiv->state = OTG_STATE_B_HOST;
			hcd->self.is_b_host = 1;
			musb->ignore_disconnect = 0;
			del_timer(&musb->otg_timer);
			break;
		default:
			if ((devctl & USB_DEVCTL_VBUSVALID)
					== (3 << USB_DEVCTL_VBUS_OFFSET)) {
				musb->xceiv->state = OTG_STATE_A_HOST;
				hcd->self.is_b_host = 0;
			}
			break;
		}

		/* poke the root hub */
		MUSB_HST_MODE(musb);
		if (hcd->status_urb)
			usb_hcd_poll_rh_status(hcd);
		else
			usb_hcd_resume_root_hub(hcd);

		dev_dbg(musb->controller, "CONNECT (%s) devctl %02x\n",
				usb_otg_state_string(musb->xceiv->state), devctl);
	}

	if ((int_usb & DISCONN_INTR) && !musb->ignore_disconnect) {
		dev_dbg(musb->controller, "DISCONNECT (%s) as %s, devctl %02x\n",
				usb_otg_state_string(musb->xceiv->state),
				MUSB_MODE(musb), devctl);
		handled = IRQ_HANDLED;

#if USBHSET
		//Clear test mode when bus reset to facilitate electrical test. However this doesn't comply to spec.
printk("DISCONNECT\n");		
		if(musb->test_mode != 0)
		{
			musb->test_mode = 0;
			os_writel(U3D_USB2_TEST_MODE, 0);

			os_writel(U3D_POWER_MANAGEMENT, os_readl(U3D_POWER_MANAGEMENT) & ~SOFT_CONN);
			os_writel(U3D_POWER_MANAGEMENT, os_readl(U3D_POWER_MANAGEMENT) | SOFT_CONN);
		}
		
#endif
		switch (musb->xceiv->state) {
		case OTG_STATE_A_HOST:
		case OTG_STATE_A_SUSPEND:
			usb_hcd_resume_root_hub(musb_to_hcd(musb));
//			musb_root_disconnect(musb);
			if (musb->a_wait_bcon != 0 && is_otg_enabled(musb))
				musb_platform_try_idle(musb, jiffies
					+ msecs_to_jiffies(musb->a_wait_bcon));
			break;
		case OTG_STATE_B_HOST:
			/* REVISIT this behaves for "real disconnect"
			 * cases; make sure the other transitions from
			 * from B_HOST act right too.  The B_HOST code
			 * in hnp_stop() is currently not used...
			 */
			//musb_root_disconnect(musb); //I don't port virthub now.
			musb_to_hcd(musb)->self.is_b_host = 0;
			musb->xceiv->state = OTG_STATE_B_PERIPHERAL;
			MUSB_DEV_MODE(musb);
			musb_g_disconnect(musb);
			break;
		case OTG_STATE_A_PERIPHERAL:
			musb_hnp_stop(musb);
			//musb_root_disconnect(musb); //I don't port virthub now.
			/* FALLTHROUGH */
		case OTG_STATE_B_WAIT_ACON:
			/* FALLTHROUGH */
		case OTG_STATE_B_PERIPHERAL:
		case OTG_STATE_B_IDLE:
			musb_g_disconnect(musb);
			break;
		default:
			WARNING("unhandled DISCONNECT transition (%s)\n",
				usb_otg_state_string(musb->xceiv->state));
			break;
		}
	}

	/* mentor saves a bit: bus reset and babble share the same irq.
	 * only host sees babble; only peripheral sees bus reset.
	 */
	if (int_usb & RESET_INTR) {
		handled = IRQ_HANDLED;
#ifdef POWER_SAVING_MODE
		os_writel(U3D_SSUSB_U3_CTRL_0P, os_readl(U3D_SSUSB_U3_CTRL_0P) & ~SSUSB_U3_PORT_PDN);
#endif

		if(1)//device mode
		{
			dev_dbg(musb->controller, "BUS RESET as %s\n",
				usb_otg_state_string(musb->xceiv->state));
printk("BUS RESET\n");					
			switch (musb->xceiv->state) {
			case OTG_STATE_A_SUSPEND:
				/* We need to ignore disconnect on suspend
				 * otherwise tusb 2.0 won't reconnect after a
				 * power cycle, which breaks otg compliance.
				 */
				musb->ignore_disconnect = 1;
				musb_g_reset(musb);
				/* FALLTHROUGH */
			case OTG_STATE_A_WAIT_BCON:	/* OPT TD.4.7-900ms */
				/* never use invalid T(a_wait_bcon) */
				dev_dbg(musb->controller, "HNP: in %s, %d msec timeout\n",
					usb_otg_state_string(musb->xceiv->state),
					TA_WAIT_BCON(musb));
				mod_timer(&musb->otg_timer, jiffies
					+ msecs_to_jiffies(TA_WAIT_BCON(musb)));
				break;
			case OTG_STATE_A_PERIPHERAL:
				musb->ignore_disconnect = 0;
				del_timer(&musb->otg_timer);
				musb_g_reset(musb);
				break;
			case OTG_STATE_B_WAIT_ACON:
				dev_dbg(musb->controller, "HNP: RESET (%s), to b_peripheral\n",
					usb_otg_state_string(musb->xceiv->state));
				musb->xceiv->state = OTG_STATE_B_PERIPHERAL;
				musb_g_reset(musb);
				break;
			case OTG_STATE_B_IDLE:
				musb->xceiv->state = OTG_STATE_B_PERIPHERAL;
				/* FALLTHROUGH */
			case OTG_STATE_B_PERIPHERAL:
				musb_g_reset(musb);
				break;
			default:
				dev_dbg(musb->controller, "Unhandled BUS RESET as %s\n",
					usb_otg_state_string(musb->xceiv->state));
			}
		}
	}


	schedule_work(&musb->irq_work);

	return handled;
}

/*-------------------------------------------------------------------------*/

/*
* Program the HDRC to start (enable interrupts, dma, etc.).
*/
void musb_start(struct musb *musb)
{
//	u32 int_en;
	u8		devctl = (u8)os_readl(U3D_DEVICE_CONTROL);;

	dev_dbg(musb->controller, "<== devctl %02x\n", devctl);

    os_printk(K_DEBUG, "musb_start\n");
	
	#if 0
	//This will override maxp settings
	mu3d_hal_rst_dev();
	#endif

	os_writel(U3D_LV1IESR, 0xFFFFFFFF);



	mu3d_hal_dft_reg();

	#ifndef EXT_VBUS_DET
	os_writel(U3D_MISC_CTRL, 0); //use VBUS comparation on test chip
	#else
	os_writel(U3D_MISC_CTRL, 0x3); //force VBUS on	
	#endif
	os_writel(U3D_MISC_CTRL, 0x3); //force VBUS on	
	
	os_writel(U3D_LTSSM_RXDETECT_CTRL, 0xc801); //set 200us delay before do Tx detect Rx to workaround PHY test chip issue

	mu3d_hal_system_intr_en();

#ifdef USE_SSUSB_QMU //For QMU, we don't enable EP interrupts.

	//os_writew(regs + U3D_EPIESR_EPRIER, os_readw(regs + U3D_EPIESR_EPRIER)|EP0ISR);//enable EPN-0 interrupt
	os_writel(U3D_EPIESR, EP0ISR);//enable 0 interrupt	

#else
	//os_writew(regs + U3D_EPIESR_EPTIER, os_readw(regs + U3D_EPIESR_EPTIER)|musb->epmask);//enable EPN interrupt
	//os_writew(regs + U3D_EPIESR_EPRIER, os_readw(regs + U3D_EPIESR_EPRIER)|musb->epmask & EPRISR);//enable EPN-0 interrupt
//	os_writel(U3D_EPIESR, musb->epmask | ((musb->epmask << 16) & EPRISR));//enable EPN-0 interrupt	
    os_writel(U3D_EPIESR, 0xFFFF | ((0XFFFE << 16) ));	
#endif //	#ifndef USE_SSUSB_QMU


	
#ifdef CONFIG_USB_GADGET_SUPERSPEED
	//Write 1 clear all LTSSM interrupts
	os_writel(U3D_LTSSM_INTR, os_readl(U3D_LTSSM_INTR));
	//HS/FS detected by HW
	os_writel(U3D_POWER_MANAGEMENT, (os_readl(U3D_POWER_MANAGEMENT) | HS_ENABLE));
	//enable U3 port
	os_writel(U3D_USB3_CONFIG, USB3_EN);
	//U2/U3 detected by HW
	os_writel(U3D_DEVICE_CONF, 0);
	os_writel(U3D_LINK_POWER_CONTROL, os_readl(U3D_LINK_POWER_CONTROL) | SW_U1_ACCEPT_ENABLE | SW_U2_ACCEPT_ENABLE);//enable U1/U2				

#else //if CONFIG_USB_GADGET_DUALSPEED

	//HS/FS detected by HW
	os_writel(U3D_POWER_MANAGEMENT, os_readl(U3D_POWER_MANAGEMENT) | SOFT_CONN | HS_ENABLE);
	//disable U3 port
	os_writel(U3D_USB3_CONFIG, 0);
	//U2/U3 detected by HW
	os_writel(U3D_DEVICE_CONF, 0);
	
//#else

	//FS only
//	os_writel(U3D_POWER_MANAGEMENT, (os_readl(U3D_POWER_MANAGEMENT) | SOFT_CONN) & ~HS_ENABLE);
	//disable U3 port
//	os_writel(U3D_USB3_CONFIG, 0);
	//U2/U3 detected by HW
//	os_writel(U3D_DEVICE_CONF, 0);

#endif

	os_writel(U3D_USB2_TEST_MODE, 0); //clear test mode

    musb->is_active = 1;

	musb_platform_enable(musb);
}


static void musb_generic_disable(void)
{
	mu3d_hal_initr_dis();

}

/*
 * Make the HDRC stop (disable interrupts, etc.);
 * reversible by musb_start
 * called on gadget driver unregister
 * with controller locked, irqs blocked
 * acts as a NOP unless some role activated the hardware
 */
void musb_stop(struct musb *musb)
{
	/* stop IRQs, timers, ... */
	musb_platform_disable(musb);
	musb_generic_disable();
	dev_dbg(musb->controller, "HDRC disabled\n");

    os_printk(K_DEBUG, "musb_stop\n");
	os_writel(U3D_POWER_MANAGEMENT, os_readl(U3D_POWER_MANAGEMENT) & ~SOFT_CONN & ~HS_ENABLE);

	/* FIXME
	 *  - mark host and/or peripheral drivers unusable/inactive
	 *  - disable DMA (and enable it in HdrcStart)
	 *  - make sure we can musb_start() after musb_stop(); with
	 *    OTG mode, gadget driver module rmmod/modprobe cycles that
	 *  - ...
	 */
	musb_platform_try_idle(musb, 0);
}

static void musb_shutdown(struct platform_device *pdev)
{
	struct musb	*musb = dev_to_musb(&pdev->dev);
	unsigned long	flags;

	pm_runtime_get_sync(musb->controller);
	spin_lock_irqsave(&musb->lock, flags);
	musb_platform_disable(musb);
	musb_generic_disable();
	spin_unlock_irqrestore(&musb->lock, flags);

	if (!is_otg_enabled(musb) && is_host_enabled(musb))
		usb_remove_hcd(musb_to_hcd(musb));
	os_writel(U3D_DEVICE_CONTROL, 0);
	musb_platform_exit(musb);

	pm_runtime_put(musb->controller);
	/* FIXME power down */
}


/*-------------------------------------------------------------------------*/

/*
 * The silicon either has hard-wired endpoint configurations, or else
 * "dynamic fifo" sizing.  The driver has support for both, though at this
 * writing only the dynamic sizing is very well tested.   Since we switched
 * away from compile-time hardware parameters, we can no longer rely on
 * dead code elimination to leave only the relevant one in the object file.
 *
 * We don't currently use dynamic fifo setup capability to do anything
 * more than selecting one of a bunch of predefined configurations.
 */
#if defined(CONFIG_USB_MUSB_TUSB6010)			\
	|| defined(CONFIG_USB_MUSB_TUSB6010_MODULE)	\
	|| defined(CONFIG_USB_MUSB_OMAP2PLUS)		\
	|| defined(CONFIG_USB_MUSB_OMAP2PLUS_MODULE)	\
	|| defined(CONFIG_USB_MUSB_AM35X)		\
	|| defined(CONFIG_USB_MUSB_AM35X_MODULE)
static ushort __initdata fifo_mode = 4;
#elif defined(CONFIG_USB_MUSB_UX500)			\
	|| defined(CONFIG_USB_MUSB_UX500_MODULE)
static ushort __initdata fifo_mode = 5;
#else
static ushort __initdata fifo_mode = 2;
#endif

/* "modprobe ... fifo_mode=1" etc */
module_param(fifo_mode, ushort, 0);
MODULE_PARM_DESC(fifo_mode, "initial endpoint configuration");

/*
 * tables defining fifo_mode values.  define more if you like.
 * for host side, make sure both halves of ep1 are set up.
 */

/* mode 0 - fits in 2KB */
static struct musb_fifo_cfg __initdata mode_0_cfg[] = {
{ .hw_ep_num = 1, .style = FIFO_TX,   .maxpacket = 512, },
{ .hw_ep_num = 1, .style = FIFO_RX,   .maxpacket = 512, },
{ .hw_ep_num = 2, .style = FIFO_RXTX, .maxpacket = 512, },
{ .hw_ep_num = 3, .style = FIFO_RXTX, .maxpacket = 256, },
{ .hw_ep_num = 4, .style = FIFO_RXTX, .maxpacket = 256, },
};

/* mode 1 - fits in 4KB */
static struct musb_fifo_cfg __initdata mode_1_cfg[] = {
{ .hw_ep_num = 1, .style = FIFO_TX,   .maxpacket = 512, .mode = BUF_DOUBLE, },
{ .hw_ep_num = 1, .style = FIFO_RX,   .maxpacket = 512, .mode = BUF_DOUBLE, },
{ .hw_ep_num = 2, .style = FIFO_RXTX, .maxpacket = 512, .mode = BUF_DOUBLE, },
{ .hw_ep_num = 3, .style = FIFO_RXTX, .maxpacket = 256, },
{ .hw_ep_num = 4, .style = FIFO_RXTX, .maxpacket = 256, },
};

/* mode 2 - fits in 4KB */
static struct musb_fifo_cfg __initdata mode_2_cfg[] = {
{ .hw_ep_num = 1, .style = FIFO_TX,   .maxpacket = 512, },
{ .hw_ep_num = 1, .style = FIFO_RX,   .maxpacket = 512, },
{ .hw_ep_num = 2, .style = FIFO_TX,   .maxpacket = 512, },
{ .hw_ep_num = 2, .style = FIFO_RX,   .maxpacket = 512, },
};

/* mode 3 - fits in 4KB */
static struct musb_fifo_cfg __initdata mode_3_cfg[] = {
{ .hw_ep_num = 1, .style = FIFO_TX,   .maxpacket = 512, .mode = BUF_DOUBLE, },
{ .hw_ep_num = 1, .style = FIFO_RX,   .maxpacket = 512, .mode = BUF_DOUBLE, },
{ .hw_ep_num = 2, .style = FIFO_TX,   .maxpacket = 512, },
{ .hw_ep_num = 2, .style = FIFO_RX,   .maxpacket = 512, },
{ .hw_ep_num = 3, .style = FIFO_RXTX, .maxpacket = 256, },
{ .hw_ep_num = 4, .style = FIFO_RXTX, .maxpacket = 256, },
};

/* mode 4 - fits in 16KB */
static struct musb_fifo_cfg __initdata mode_4_cfg[] = {
{ .hw_ep_num =  1, .style = FIFO_TX,   .maxpacket = 512, },
{ .hw_ep_num =  1, .style = FIFO_RX,   .maxpacket = 512, },
{ .hw_ep_num =  2, .style = FIFO_TX,   .maxpacket = 512, },
{ .hw_ep_num =  2, .style = FIFO_RX,   .maxpacket = 512, },
{ .hw_ep_num =  3, .style = FIFO_TX,   .maxpacket = 512, },
{ .hw_ep_num =  3, .style = FIFO_RX,   .maxpacket = 512, },
{ .hw_ep_num =  4, .style = FIFO_TX,   .maxpacket = 512, },
{ .hw_ep_num =  4, .style = FIFO_RX,   .maxpacket = 512, },
{ .hw_ep_num =  5, .style = FIFO_TX,   .maxpacket = 512, },
{ .hw_ep_num =  5, .style = FIFO_RX,   .maxpacket = 512, },
{ .hw_ep_num =  6, .style = FIFO_TX,   .maxpacket = 512, },
{ .hw_ep_num =  6, .style = FIFO_RX,   .maxpacket = 512, },
{ .hw_ep_num =  7, .style = FIFO_TX,   .maxpacket = 512, },
{ .hw_ep_num =  7, .style = FIFO_RX,   .maxpacket = 512, },
{ .hw_ep_num =  8, .style = FIFO_TX,   .maxpacket = 512, },
{ .hw_ep_num =  8, .style = FIFO_RX,   .maxpacket = 512, },
{ .hw_ep_num =  9, .style = FIFO_TX,   .maxpacket = 512, },
{ .hw_ep_num =  9, .style = FIFO_RX,   .maxpacket = 512, },
{ .hw_ep_num = 10, .style = FIFO_TX,   .maxpacket = 256, },
{ .hw_ep_num = 10, .style = FIFO_RX,   .maxpacket = 64, },
{ .hw_ep_num = 11, .style = FIFO_TX,   .maxpacket = 256, },
{ .hw_ep_num = 11, .style = FIFO_RX,   .maxpacket = 64, },
{ .hw_ep_num = 12, .style = FIFO_TX,   .maxpacket = 256, },
{ .hw_ep_num = 12, .style = FIFO_RX,   .maxpacket = 64, },
{ .hw_ep_num = 13, .style = FIFO_RXTX, .maxpacket = 4096, },
{ .hw_ep_num = 14, .style = FIFO_RXTX, .maxpacket = 1024, },
{ .hw_ep_num = 15, .style = FIFO_RXTX, .maxpacket = 1024, },
};

/* mode 5 - fits in 8KB */
static struct musb_fifo_cfg __initdata mode_5_cfg[] = {
{ .hw_ep_num =  1, .style = FIFO_TX,   .maxpacket = 512, },
{ .hw_ep_num =  1, .style = FIFO_RX,   .maxpacket = 512, },
{ .hw_ep_num =  2, .style = FIFO_TX,   .maxpacket = 512, },
{ .hw_ep_num =  2, .style = FIFO_RX,   .maxpacket = 512, },
{ .hw_ep_num =  3, .style = FIFO_TX,   .maxpacket = 512, },
{ .hw_ep_num =  3, .style = FIFO_RX,   .maxpacket = 512, },
{ .hw_ep_num =  4, .style = FIFO_TX,   .maxpacket = 512, },
{ .hw_ep_num =  4, .style = FIFO_RX,   .maxpacket = 512, },
{ .hw_ep_num =  5, .style = FIFO_TX,   .maxpacket = 512, },
{ .hw_ep_num =  5, .style = FIFO_RX,   .maxpacket = 512, },
{ .hw_ep_num =  6, .style = FIFO_TX,   .maxpacket = 32, },
{ .hw_ep_num =  6, .style = FIFO_RX,   .maxpacket = 32, },
{ .hw_ep_num =  7, .style = FIFO_TX,   .maxpacket = 32, },
{ .hw_ep_num =  7, .style = FIFO_RX,   .maxpacket = 32, },
{ .hw_ep_num =  8, .style = FIFO_TX,   .maxpacket = 32, },
{ .hw_ep_num =  8, .style = FIFO_RX,   .maxpacket = 32, },
{ .hw_ep_num =  9, .style = FIFO_TX,   .maxpacket = 32, },
{ .hw_ep_num =  9, .style = FIFO_RX,   .maxpacket = 32, },
{ .hw_ep_num = 10, .style = FIFO_TX,   .maxpacket = 32, },
{ .hw_ep_num = 10, .style = FIFO_RX,   .maxpacket = 32, },
{ .hw_ep_num = 11, .style = FIFO_TX,   .maxpacket = 32, },
{ .hw_ep_num = 11, .style = FIFO_RX,   .maxpacket = 32, },
{ .hw_ep_num = 12, .style = FIFO_TX,   .maxpacket = 32, },
{ .hw_ep_num = 12, .style = FIFO_RX,   .maxpacket = 32, },
{ .hw_ep_num = 13, .style = FIFO_RXTX, .maxpacket = 512, },
{ .hw_ep_num = 14, .style = FIFO_RXTX, .maxpacket = 1024, },
{ .hw_ep_num = 15, .style = FIFO_RXTX, .maxpacket = 1024, },
};

/* mode 6 - fits in 8KB */
//Use SS EPs for SS/HS/FS BULK transfers
//TODO: this is suscipious
static struct musb_fifo_cfg __initdata mode_6_cfg[] = {
{ .hw_ep_num =  1, .style = FIFO_TX,   .maxpacket = 1024, },
{ .hw_ep_num =  1, .style = FIFO_RX,   .maxpacket = 1024, },
#if 0
{ .hw_ep_num =  2, .style = FIFO_TX,   .maxpacket = 1024, },
{ .hw_ep_num =  2, .style = FIFO_RX,   .maxpacket = 1024, },
#endif
};

void ep0_setup(struct musb *musb, struct musb_hw_ep  *hw_ep0, const struct musb_fifo_cfg *cfg)
{
    os_printk(K_DEBUG, "ep0_setup maxpacket: %d\n", cfg->maxpacket);

	hw_ep0->fifoaddr_rx = 0;
	hw_ep0->fifoaddr_tx = 0;
	hw_ep0->is_shared_fifo = true;
	hw_ep0->fifo = (void __iomem *)MUSB_FIFO_OFFSET(0);

	//for U2
	hw_ep0->max_packet_sz_tx = cfg->maxpacket;
	hw_ep0->max_packet_sz_rx = cfg->maxpacket;

	os_writelmskumsk(U3D_EP0CSR, hw_ep0->max_packet_sz_tx, EP0_MAXPKTSZ0, EP0_W1C_BITS);
//	os_writel(U3D_EPIESR, os_readl(U3D_EPIESR)|EP0ISR);//enable EP0 interrupt
    os_writel(U3D_EPIESR, 0xFFFF | ((0XFFFE << 16) ));	

	return;
}

/*
 * configure a fifo; for non-shared endpoints, this may be called
 * once for a tx fifo and once for an rx fifo.
 *
 * returns negative errno or offset for next fifo.
 */
static int __init
fifo_setup(struct musb *musb, struct musb_hw_ep  *hw_ep,
		const struct musb_fifo_cfg *cfg, u16 offset)
{
	u16	maxpacket = cfg->maxpacket;
	//u16	c_off = offset >> 3;
	u16 ret_offset = 0;
	u32 maxpreg = 0;
	u8	mult = 0;

    //calculate mult. added for ssusb.
	if(maxpacket > 1024){
		maxpreg = 1024;
		mult = (maxpacket/1024) - 1; //?????
	}
	else{
		maxpreg = maxpacket;
		//set EP0 TX/RX slot to 3 by default
		if (hw_ep->epnum == 1)
			mult = 1;
	}
	
	/* EP0 reserved endpoint for control, bidirectional;
	 * EP1 reserved for bulk, two unidirection halves.
	 */
	if (hw_ep->epnum == 1)
		musb->bulk_ep = hw_ep;
	/* REVISIT error check:  be sure ep0 can both rx and tx ... */
	if((cfg->style == FIFO_TX) || (cfg->style == FIFO_RXTX))
	{
		hw_ep->max_packet_sz_tx = maxpreg;
		hw_ep->mult_tx = mult;
		
		hw_ep->fifoaddr_tx = musb->txfifoadd_offset;
		if(maxpacket == 1023){
			musb->txfifoadd_offset += (1024 * (hw_ep->mult_tx + 1));
		}
		else{	
			musb->txfifoadd_offset += (maxpacket * (hw_ep->mult_tx + 1));
		}	
		ret_offset = musb->txfifoadd_offset;		
	}
	
	if((cfg->style == FIFO_RX) || (cfg->style == FIFO_RXTX))
	{
		hw_ep->max_packet_sz_rx = maxpreg;
		hw_ep->mult_rx = mult;
		
		hw_ep->fifoaddr_rx = musb->rxfifoadd_offset;
		if(maxpacket == 1023){
			musb->rxfifoadd_offset += (1024 * (hw_ep->mult_rx + 1));
		}
		else{
			musb->rxfifoadd_offset += (maxpacket * (hw_ep->mult_rx + 1));
		}
		ret_offset = musb->rxfifoadd_offset;
	}

	/* NOTE rx and tx endpoint irqs aren't managed separately,
	 * which happens to be ok
	 */
	musb->epmask |= (1 << hw_ep->epnum);

   return ret_offset;

}


struct musb_fifo_cfg ep0_cfg_u3 = {
	.style = FIFO_RXTX, .maxpacket = 512,
};

struct musb_fifo_cfg ep0_cfg_u2 = {
	.style = FIFO_RXTX, .maxpacket = 64,
};

static int __init ep_config_from_table(struct musb *musb)
{
	const struct musb_fifo_cfg	*cfg;
	unsigned		i, n;
	int			offset;
	struct musb_hw_ep	*hw_ep = musb->endpoints;

#if 0
	if (musb->config->fifo_cfg) {
		cfg = musb->config->fifo_cfg;
		n = musb->config->fifo_cfg_size;
		goto done;
	}
#endif
//	fifo_mode=6; //force U3D setting 

	switch (fifo_mode) {
	default:
		fifo_mode = 0;
		/* FALLTHROUGH */
	case 0:
		cfg = mode_0_cfg;
		n = ARRAY_SIZE(mode_0_cfg);
		break;
	case 1:
		cfg = mode_1_cfg;
		n = ARRAY_SIZE(mode_1_cfg);
		break;
	case 2:
		cfg = mode_2_cfg;
		n = ARRAY_SIZE(mode_2_cfg);
		break;
	case 3:
		cfg = mode_3_cfg;
		n = ARRAY_SIZE(mode_3_cfg);
		break;
	case 4:
		cfg = mode_4_cfg;
		n = ARRAY_SIZE(mode_4_cfg);
		break;
	case 5:
		cfg = mode_5_cfg;
		n = ARRAY_SIZE(mode_5_cfg);
		break;
	case 6:    //U3D
		cfg = mode_6_cfg;
		n = ARRAY_SIZE(mode_6_cfg);
		break;
	}

	os_printk(K_EMERG, "%s: ep_config_from_table setup fifo_mode %d\n",
			musb_driver_name, fifo_mode);


//done:
	//offset = fifo_setup(musb, hw_ep, &ep0_cfg, 0);
	#ifdef CONFIG_USB_GADGET_SUPERSPEED //SS
	//use SS EP0 as default; it may be changed later
	os_printk(K_EMERG, "ep_config_from_table ep0_cfg_u3\n");
	ep0_setup(musb, hw_ep, &ep0_cfg_u3);
	#else //HS, FS
	os_printk(K_EMERG, "ep_config_from_table ep0_cfg_u2\n");	
	ep0_setup(musb, hw_ep, &ep0_cfg_u2);
	#endif
	/* assert(offset > 0) */

	/* NOTE:  for RTL versions >= 1.400 EPINFO and RAMINFO would
	 * be better than static musb->config->num_eps and DYN_FIFO_SIZE...
	 */

	for (i = 0; i < n; i++) {
		u8	epn = cfg->hw_ep_num;

		if (epn >= musb->config->num_eps) {
			printk("%s: invalid ep %d\n",
					musb_driver_name, epn);
			return -EINVAL;
		}
		offset = fifo_setup(musb, hw_ep + epn, cfg++, offset);
		if (offset < 0) {
			printk("%s: mem overrun, ep %d\n",
					musb_driver_name, epn);
			return -EINVAL;
		}
		printk("%d= epn \n", epn);
		epn++;
		musb->nr_endpoints = max(epn, musb->nr_endpoints);
	}

	os_printk(K_EMERG, "%s: %d/%d max ep, %d/%d memory\n",
			musb_driver_name,
			n + 1, musb->config->num_eps * 2 - 1,
			offset, (1 << (musb->config->ram_bits + 2)));

	if (!musb->bulk_ep) {
		printk("%s: missing bulk\n", musb_driver_name);
		return -EINVAL;
	}

	return 0;
}


/*
 * ep_config_from_hw - when MUSB_C_DYNFIFO_DEF is false
 * @param musb the controller
 */

enum { MUSB_CONTROLLER_MHDRC, MUSB_CONTROLLER_HDRC, };

/* Initialize MUSB (M)HDRC part of the USB hardware subsystem;
 * configure endpoints, or take their config from silicon
 */
static int __init musb_core_init(u16 musb_type, struct musb *musb)
{
	//u8 reg;
	//char *type;
	//char aInfo[90], aRevision[32], aDate[12];
	void __iomem	*mbase = musb->mregs;
	int		status = 0;
	int		i;

	/* log release info */
	//musb->hwvers = musb_read_hwvers(mbase);
	//snprintf(aRevision, 32, "%d.%d%s", MUSB_HWVERS_MAJOR(musb->hwvers),
	//	MUSB_HWVERS_MINOR(musb->hwvers),
	//	(musb->hwvers & MUSB_HWVERS_RC) ? "RC" : "");
	//printk(KERN_DEBUG "%s: %sHDRC RTL version %s %s\n",
	//		musb_driver_name, type, aRevision, aDate);
    musb->hwvers = os_readl(U3D_SSUSB_HW_ID);

	/* discover endpoint configuration */
	musb->nr_endpoints = 1;
	musb->epmask = 1;
    //add for U3D
	musb->txfifoadd_offset = U3D_FIFO_START_ADDRESS;
	musb->rxfifoadd_offset = U3D_FIFO_START_ADDRESS;

	os_printk(K_DEBUG, "musb_core_init 03_2\n");


	//if (musb->dyn_fifo)
		status = ep_config_from_table(musb);
	//else
	//	status = ep_config_from_hw(musb);

	if (status < 0)
		return status;

	/* finish init, and print endpoint config */
	for (i = 0; i < musb->nr_endpoints; i++) {
		struct musb_hw_ep	*hw_ep = musb->endpoints + i;

		hw_ep->fifo = (void __iomem *)MUSB_FIFO_OFFSET(i);
#ifdef CONFIG_USB_MUSB_TUSB6010
		hw_ep->fifo_async = musb->async + 0x400 + MUSB_FIFO_OFFSET(i);
		hw_ep->fifo_sync = musb->sync + 0x400 + MUSB_FIFO_OFFSET(i);
		hw_ep->fifo_sync_va =
			musb->sync_va + 0x400 + MUSB_FIFO_OFFSET(i);

		if (i == 0)
			hw_ep->conf = mbase - 0x400 + TUSB_EP0_CONF;
		else
			hw_ep->conf = mbase + 0x400 + (((i - 1) & 0xf) << 2);
#endif

		//change data structure for ssusb
		hw_ep->addr_txcsr0 = (void __iomem *)SSUSB_EP_TXCR0_OFFSET(i, 0);
		hw_ep->addr_txcsr1 = (void __iomem *)SSUSB_EP_TXCR1_OFFSET(i, 0);
		hw_ep->addr_txcsr2 = (void __iomem *)SSUSB_EP_TXCR2_OFFSET(i, 0);
		hw_ep->addr_rxcsr0 = (void __iomem *)SSUSB_EP_RXCR0_OFFSET(i, 0);
		hw_ep->addr_rxcsr1 = (void __iomem *)SSUSB_EP_RXCR1_OFFSET(i, 0);
		hw_ep->addr_rxcsr2 = (void __iomem *)SSUSB_EP_RXCR2_OFFSET(i, 0);
		hw_ep->addr_rxcsr3 = (void __iomem *)SSUSB_EP_RXCR3_OFFSET(i, 0);		

		hw_ep->target_regs = musb_read_target_reg_base(i, mbase);
		hw_ep->rx_reinit = 1;
		hw_ep->tx_reinit = 1;

		if (hw_ep->max_packet_sz_tx) {
			dev_dbg(musb->controller,
				"%s: hw_ep %d%s, %smax %d\n",
				musb_driver_name, i,
				hw_ep->is_shared_fifo ? "shared" : "tx",
                "",
				hw_ep->max_packet_sz_tx);
		}
		if (hw_ep->max_packet_sz_rx && !hw_ep->is_shared_fifo) {
			dev_dbg(musb->controller,
				"%s: hw_ep %d%s, %smax %d\n",
				musb_driver_name, i,
				"rx",
                "",
				hw_ep->max_packet_sz_rx);
		}
		if (!(hw_ep->max_packet_sz_tx || hw_ep->max_packet_sz_rx))
			dev_dbg(musb->controller, "hw_ep %d not configured\n", i);
	}
#ifdef USE_SSUSB_QMU 
	mu3d_hal_alloc_qmu_mem();
	usb_initialize_qmu();
#endif //#ifdef USE_SSUSB_QMU

	return 0;
}

/*-------------------------------------------------------------------------*/
#if 1 //mem test code
    struct timeval tv1,tv2; 
    long tv1_total;
   
#endif



#if REPRO_TD925_ISSUE
u32 exit_u12 = 0;
#endif

//static u32 soft_conn_num = 0;
#ifdef USE_SSUSB_QMU 
//static QMU_DONE_ISR_DATA param;
//static DECLARE_TASKLET(t_qmu_done, qmu_done_interrupt, (unsigned long)&param);
#endif
irqreturn_t generic_interrupt(int irq, void *__hci)
{
	unsigned long	flags;
	irqreturn_t	retval = IRQ_NONE;
	struct musb	*musb = __hci;

	u32 dwIntrUsbValue;
	u32 dwDmaIntrValue;
	u32 dwIntrEPValue;
	u16 wIntrTxValue;
	u16 wIntrRxValue;
	u32 wIntrQMUValue = 0;
	u32 wIntrQMUDoneValue = 0;
	u32 dwLtssmValue;
    u32 dwLinkIntValue;
	u32 dwTemp;

//	os_printk(K_EMERG, "generic_interrupt start\n");
    do_gettimeofday(&tv1);

	spin_lock_irqsave(&musb->lock, flags);
//    printk("EP1RXCSR0 is 0x%X\n",os_readl(0xfe601210));
	if(os_readl(U3D_LV1ISR) & MAC2_INTR){
		dwIntrUsbValue = os_readl(U3D_COMMON_USB_INTR) & os_readl(U3D_COMMON_USB_INTR_ENABLE);
	}
	else{
		dwIntrUsbValue = 0;
	}
	if(os_readl(U3D_LV1ISR) & MAC3_INTR){
		dwLtssmValue = os_readl(U3D_LTSSM_INTR) & os_readl(U3D_LTSSM_INTR_ENABLE);
	}
	else{
		dwLtssmValue = 0;
	}
	dwDmaIntrValue = os_readl(U3D_DMAISR)& os_readl(U3D_DMAIER);
	dwIntrEPValue = os_readl(U3D_EPISR)& os_readl(U3D_EPIER);
	dwLinkIntValue = os_readl(U3D_DEV_LINK_INTR) & os_readl(U3D_DEV_LINK_INTR_ENABLE);		

    //for QMU, process this later.
	wIntrQMUValue = os_readl(U3D_QISAR1);
	wIntrQMUDoneValue = os_readl(U3D_QISAR0) & os_readl(U3D_QIER0);
	wIntrTxValue = dwIntrEPValue&0xFFFF;
	wIntrRxValue = (dwIntrEPValue>>16);
	
//    printk("wIntrTxValue is 0x%X\n",wIntrTxValue);

	//os_printk(K_DEBUG,"dwIntrEPValue :%x\n",dwIntrEPValue);
	//os_printk(K_DEBUG,"USB_EPIntSts :%x\n",Reg_Read32(USB_EPIntSts));
	os_printk(K_INFO, "Interrupt: IntrUsb [%x] IntrTx[%x] IntrRx [%x] IntrDMA[%x] IntrQMUDone[%x] IntrQMU [%x] IntrLTSSM [%x]\r\n", 
		dwIntrUsbValue,
		wIntrTxValue,
		wIntrRxValue,
		dwDmaIntrValue, wIntrQMUDoneValue, wIntrQMUValue, dwLtssmValue);		
/*
	printk("Interrupt: IntrUsb [%x] IntrTx[%x] IntrRx [%x] IntrDMA[%x] IntrQMUDone[%x] IntrQMU [%x] IntrLTSSM [%x]\r\n", 
		dwIntrUsbValue,
		wIntrTxValue,
		wIntrRxValue,
		dwDmaIntrValue, wIntrQMUDoneValue, wIntrQMUValue, dwLtssmValue);		
*/

//	os_printk(K_EMERG, "Interrupt: IntrQMUDone[%x]\r\n", wIntrQMUDoneValue);


	/* write one clear */
	os_writel(U3D_QISAR0, wIntrQMUDoneValue);

	if(os_readl(U3D_LV1ISR) & MAC2_INTR){
   		os_writel(U3D_COMMON_USB_INTR, dwIntrUsbValue);
	}
	if(os_readl(U3D_LV1ISR) & MAC3_INTR){
		os_writel(U3D_LTSSM_INTR, dwLtssmValue); 
	}
	os_writel(U3D_EPISR, dwIntrEPValue);
	os_writel(U3D_DMAISR, dwDmaIntrValue); 
	os_writel(U3D_DEV_LINK_INTR, dwLinkIntValue); 

#ifdef USE_SSUSB_QMU 
    //For speeding up QMU processing, let it be the first priority to be executed. Other interrupts will be handled at next run.
    //ported from Liang-yen's code
	if(wIntrQMUDoneValue){
//	 	param.musb = musb;
//		param.wQmuVal = wIntrQMUDoneValue;
        if(wIntrQMUDoneValue>>16)
    		os_printk(K_INFO,"USB_QMU_RQCPR %x\r\n", os_readl(USB_QMU_RQCPR(1)));
        if(wIntrQMUDoneValue & 0xffff)
    		os_printk(K_INFO,"USB_QMU_TQCPR %x\r\n", os_readl(USB_QMU_TQCPR(1)));

		qmu_done_interrupt(musb,wIntrQMUDoneValue);
	 	//tasklet_schedule(&t_qmu_done);
	}
	
 	if(wIntrQMUValue){
		qmu_exception_interrupt(musb->mregs, wIntrQMUValue);
 	}
#endif
      
/*      
	if (dwIntrUsbValue & DISCONN_INTR)
		{
		os_printk(K_DEBUG, "[DISCONN_INTR]\n");
		
		os_printk(K_DEBUG, "Set SOFT_CONN to 0\n");		
		os_writelmsk(U3D_POWER_MANAGEMENT, 0, SOFT_CONN);
		}
*/
		if (dwIntrUsbValue)
		{
			if (dwIntrUsbValue & DISCONN_INTR)
			{	
#ifdef POWER_SAVING_MODE
				os_writel(U3D_SSUSB_IP_PW_CTRL2, os_readl(U3D_SSUSB_IP_PW_CTRL2) & ~SSUSB_IP_DEV_PDN);
				os_writel(U3D_SSUSB_U3_CTRL_0P, os_readl(U3D_SSUSB_U3_CTRL_0P) & ~SSUSB_U3_PORT_PDN);
				os_writel(U3D_SSUSB_U2_CTRL_0P, os_readl(U3D_SSUSB_U2_CTRL_0P) & ~SSUSB_U2_PORT_PDN);
#endif
				os_printk(K_DEBUG, "[DISCONN_INTR]\n");
				os_printk(K_DEBUG, "Set SOFT_CONN to 0\n");		
				os_clrmsk(U3D_POWER_MANAGEMENT, SOFT_CONN);
			}
	
			if (dwIntrUsbValue & LPM_INTR)		
			{
				os_printk(K_DEBUG, "LPM interrupt\n");
#ifdef POWER_SAVING_MODE
				if(!((os_readl(U3D_POWER_MANAGEMENT) & LPM_HRWE))){
					os_writel(U3D_SSUSB_U2_CTRL_0P, os_readl(U3D_SSUSB_U2_CTRL_0P) | SSUSB_U2_PORT_PDN);
					os_writel(U3D_SSUSB_IP_PW_CTRL2, os_readl(U3D_SSUSB_IP_PW_CTRL2) | SSUSB_IP_DEV_PDN); 		
				}
#endif
			}

			if(dwIntrUsbValue & LPM_RESUME_INTR){
				if(!(os_readl(U3D_POWER_MANAGEMENT) & LPM_HRWE)){
#ifdef POWER_SAVING_MODE
					os_writel(U3D_SSUSB_IP_PW_CTRL2, os_readl(U3D_SSUSB_IP_PW_CTRL2) & ~SSUSB_IP_DEV_PDN);
					os_writel(U3D_SSUSB_U2_CTRL_0P, os_readl(U3D_SSUSB_U2_CTRL_0P) & ~SSUSB_U2_PORT_PDN);
					while(!(os_readl(U3D_SSUSB_IP_PW_STS2) & SSUSB_U2_MAC_SYS_RST_B_STS));					
#endif
					os_writel(U3D_USB20_MISC_CONTROL, os_readl(U3D_USB20_MISC_CONTROL) | LPM_U3_ACK_EN);
				}
			}

			if(dwIntrUsbValue & SUSPEND_INTR){
				os_printk(K_DEBUG,"Suspend Interrupt\n");
#ifdef POWER_SAVING_MODE
				os_writel(U3D_SSUSB_U2_CTRL_0P, os_readl(U3D_SSUSB_U2_CTRL_0P) | SSUSB_U2_PORT_PDN);
				os_writel(U3D_SSUSB_IP_PW_CTRL2, os_readl(U3D_SSUSB_IP_PW_CTRL2) | SSUSB_IP_DEV_PDN);
#endif
			}
			if (dwIntrUsbValue & RESUME_INTR){
				os_printk(K_NOTICE,"Resume Interrupt\n");
#ifdef POWER_SAVING_MODE
				os_writel(U3D_SSUSB_IP_PW_CTRL2, os_readl(U3D_SSUSB_IP_PW_CTRL2) & ~SSUSB_IP_DEV_PDN);
				os_writel(U3D_SSUSB_U2_CTRL_0P, os_readl(U3D_SSUSB_U2_CTRL_0P) & ~SSUSB_U2_PORT_PDN);
				while(!(os_readl(U3D_SSUSB_IP_PW_STS2) & SSUSB_U2_MAC_SYS_RST_B_STS));				
#endif
			}
	
	
		}

	if (dwLtssmValue)
	{
		if (dwLtssmValue & SS_DISABLE_INTR)
		{
			os_printk(K_DEBUG, "LTSSM: SS_DISABLE_INTR [%d]\n", ++soft_conn_num);
			os_printk(K_DEBUG, "Set SOFT_CONN to 1\n");
			//enable U2 link. after host reset, HS/FS EP0 configuration is applied in musb_g_reset	
#ifdef POWER_SAVING_MODE
			os_writel(U3D_SSUSB_U2_CTRL_0P, os_readl(U3D_SSUSB_U2_CTRL_0P) &~ (SSUSB_U2_PORT_DIS | SSUSB_U2_PORT_PDN));
#endif
			os_setmsk(U3D_POWER_MANAGEMENT, SOFT_CONN);
		}

	    if (dwLtssmValue & ENTER_U0_INTR)
	    {
			os_printk(K_DEBUG, "LTSSM: ENTER_U0_INTR\n");
			//do not apply U3 EP0 setting again, if the speed is already U3
			//LTSSM may go to recovery and back to U0
			if (musb->g.speed != USB_SPEED_SUPER)
				musb_conifg_ep0(musb);
		}

#if REPRO_TD925_ISSUE
	    if (dwLtssmValue & EXIT_U1_INTR)
    	{    		
			os_printk(K_DEBUG, "LTSSM: EXIT_U1_INTR\n");
    		exit_u12 = 1;
    	}

	    if (dwLtssmValue & EXIT_U2_INTR)
    	{
			os_printk(K_DEBUG, "LTSSM: EXIT_U2_INTR\n");
    		exit_u12 = 1;
    	}
#endif

		if (dwLtssmValue & VBUS_FALL_INTR)
		{
#ifdef POWER_SAVING_MODE
			os_writel(U3D_SSUSB_IP_PW_CTRL2, os_readl(U3D_SSUSB_IP_PW_CTRL2) &~ SSUSB_IP_DEV_PDN);
			os_writel(U3D_SSUSB_U2_CTRL_0P, os_readl(U3D_SSUSB_U2_CTRL_0P) &~ (SSUSB_U2_PORT_DIS | SSUSB_U2_PORT_PDN));
			os_writel(U3D_SSUSB_U3_CTRL_0P, os_readl(U3D_SSUSB_U3_CTRL_0P) &~ SSUSB_U3_PORT_PDN);
#endif
			os_printk(K_DEBUG, "LTSSM: VBUS_FALL_INTR\n");		
			os_writel(U3D_USB3_CONFIG, 0);
		}

		if (dwLtssmValue & VBUS_RISE_INTR)
		{
			os_printk(K_DEBUG, "LTSSM: VBUS_RISE_INTR\n");		
			os_writel(U3D_USB3_CONFIG, USB3_EN);
		}

		if (dwLtssmValue & ENTER_U3_INTR)
		{
#ifdef POWER_SAVING_MODE
			os_writel(U3D_SSUSB_U3_CTRL_0P, os_readl(U3D_SSUSB_U3_CTRL_0P) | SSUSB_U3_PORT_PDN);
			os_writel(U3D_SSUSB_IP_PW_CTRL2, os_readl(U3D_SSUSB_IP_PW_CTRL2) | SSUSB_IP_DEV_PDN);
#endif
		}
		
		if (dwLtssmValue & EXIT_U3_INTR)
		{
#ifdef POWER_SAVING_MODE
			os_writel(U3D_SSUSB_U3_CTRL_0P, os_readl(U3D_SSUSB_U3_CTRL_0P) &~ SSUSB_U3_PORT_PDN);
			os_writel(U3D_SSUSB_IP_PW_CTRL2, os_readl(U3D_SSUSB_IP_PW_CTRL2) &~ SSUSB_IP_DEV_PDN);
			while(!(os_readl(U3D_SSUSB_IP_PW_STS1) & SSUSB_U3_MAC_RST_B_STS));			
#endif
		}
		
#ifndef POWER_SAVING_MODE
		if (dwLtssmValue & U3_RESUME_INTR)
		{
			os_writel(U3D_SSUSB_U3_CTRL_0P, os_readl(U3D_SSUSB_U3_CTRL_0P) &~ SSUSB_U3_PORT_PDN);
			os_writel(U3D_SSUSB_IP_PW_CTRL2, os_readl(U3D_SSUSB_IP_PW_CTRL2) &~ SSUSB_IP_DEV_PDN);
			while(!(os_readl(U3D_SSUSB_IP_PW_STS1) & SSUSB_U3_MAC_RST_B_STS));
			os_writel(U3D_LINK_POWER_CONTROL, os_readl(U3D_LINK_POWER_CONTROL) | UX_EXIT);
		}
#endif

	}

	musb->int_usb = dwIntrUsbValue;
	musb->int_tx = wIntrTxValue;
    musb->int_rx = wIntrRxValue;

if (musb->int_usb || musb->int_tx || musb->int_rx)
	retval = musb_interrupt(musb);

else
    retval = IRQ_HANDLED;
	if (dwLinkIntValue & SSUSB_DEV_SPEED_CHG_INTR)
	{
		os_printk(K_ALET,"Speed Change Interrupt\n");
				
		printk("Speed Change Interrupt : ");
		dwTemp = os_readl(U3D_DEVICE_CONF) & SSUSB_DEV_SPEED;
#ifdef POWER_SAVING_MODE
		mu3d_hal_pdn_cg_en();
#endif	
		switch (dwTemp)
		{
			case SSUSB_SPEED_FULL:
				printk("FS\n");
				//BESLCK = 4 < BESLCK_U3 = 10 < BESLDCK = 15
				os_writel(U3D_USB20_LPM_PARAMETER, 0xa4f0);
				os_setmsk(U3D_POWER_MANAGEMENT, (LPM_BESL_STALL|LPM_BESLD_STALL));
			break;
			case SSUSB_SPEED_HIGH:
				printk("HS\n");
				//BESLCK = 4 < BESLCK_U3 = 10 < BESLDCK = 15
				os_writel(U3D_USB20_LPM_PARAMETER, 0xa4f0);
				os_setmsk(U3D_POWER_MANAGEMENT, (LPM_BESL_STALL|LPM_BESLD_STALL));
			break;
			case SSUSB_SPEED_SUPER:
				printk("SS\n");
			break;
			default:
			os_printk(K_ALET,"Invalid\n");
		}
	}
	spin_unlock_irqrestore(&musb->lock, flags);

    do_gettimeofday(&tv2);
//    if( tv1_total < (tv2.tv_sec-tv1.tv_sec)*1000000 + (tv2.tv_usec-tv1.tv_usec))
	tv1_total = (tv2.tv_sec-tv1.tv_sec)*1000000 + (tv2.tv_usec-tv1.tv_usec);
//	os_printk(K_EMERG, "generic_interrupt end\n");

	return retval;

}



/*
 * handle all the irqs defined by the HDRC core. for now we expect:  other
 * irq sources (phy, dma, etc) will be handled first, musb->int_* values
 * will be assigned, and the irq will already have been acked.
 *
 * called in irq context with spinlock held, irqs blocked
 */
irqreturn_t musb_interrupt(struct musb *musb)
{
	irqreturn_t	retval = IRQ_NONE;
	u8		devctl, power = 0;
#ifndef USE_SSUSB_QMU	
	u32 	reg = 0, ep_num = 0;
#endif

#ifdef POWER_SAVING_MODE
		if(!(os_readl(U3D_SSUSB_U2_CTRL_0P) & SSUSB_U2_PORT_DIS)){
			devctl = (u8)os_readl(U3D_DEVICE_CONTROL);
			power = (u8)os_readl(U3D_POWER_MANAGEMENT); 

		}else{
			devctl = 0;
			power = 0;
			musb->int_usb = 0;
		}
#else
		devctl = (u8)os_readl(U3D_DEVICE_CONTROL);
		power = (u8)os_readl(U3D_POWER_MANAGEMENT); 
#endif
		
	dev_dbg(musb->controller, "** IRQ %s usb%04x tx%04x rx%04x\n",
		(devctl & USB_DEVCTL_HOSTMODE) ? "host" : "peripheral",
		musb->int_usb, musb->int_tx, musb->int_rx);

	/* the core can interrupt us for multiple reasons; docs have
	 * a generic interrupt flowchart to follow
	 */
	if (musb->int_usb)
		retval |= musb_stage0_irq(musb, musb->int_usb,
				devctl, power);

	/* "stage 1" is handling endpoint irqs */

	/* handle endpoint 0 first */
	if (musb->int_tx & 1) {
		retval |= musb_g_ep0_irq(musb);
	}

#ifndef USE_SSUSB_QMU
	//With QMU, we don't use RX/TX interrupt to trigger EPN flow.

	/* RX on endpoints 1-15 */
	reg = musb->int_rx >> 1;
	printk("musb->int_rx 0x%x \n",reg);
	ep_num = 1;
	while (reg) {
		if (reg & 1) {
			/* musb_ep_select(musb->mregs, ep_num); */
			/* REVISIT just retval = ep->rx_irq(...) */
			printk("EP%d RX interrupt \n",ep_num);
         	printk("----rxcsr0 is 0x%X\n" ,os_readl(U3D_RX1CSR0));
			
			retval = IRQ_HANDLED;
			musb_g_rx(musb, ep_num);
//	        musb_g_tx(musb, ep_num);
			
		}

		reg >>= 1;
		ep_num++;
	}

	/* TX on endpoints 1-15 */
	reg = musb->int_tx >> 1;
	ep_num = 1;
	
	while (reg) {
		if (reg & 1) {
			/* musb_ep_select(musb->mregs, ep_num); */
			/* REVISIT just retval |= ep->tx_irq(...) */
			printk("EP%d TX interrupt \n",ep_num);
			retval = IRQ_HANDLED;
			musb_g_tx(musb, ep_num);
		}
		reg >>= 1;
		ep_num++;
	}
#endif //#ifndef USE_SSUSB_QMU
	return retval;
}
EXPORT_SYMBOL_GPL(musb_interrupt);

#ifndef CONFIG_MUSB_PIO_ONLY
static int __initdata use_dma = 1;

/* "modprobe ... use_dma=0" etc */
module_param(use_dma, int, 0);
MODULE_PARM_DESC(use_dma, "enable/disable use of DMA");
#if 0
void musb_dma_completion(struct musb *musb, u8 epnum, u8 transmit)
{
	u8	devctl = musb_readb(musb->mregs, MUSB_DEVCTL);

	/* called with controller lock already held */

	if (!epnum) {
#ifndef CONFIG_USB_TUSB_OMAP_DMA
		if (!is_cppi_enabled()) {
			/* endpoint 0 */
			if (devctl & MUSB_DEVCTL_HM)
				musb_h_ep0_irq(musb);
			else
				musb_g_ep0_irq(musb);
		}
#endif
	} else {
		/* endpoints 1..15 */
		if (transmit) {
			if (devctl & MUSB_DEVCTL_HM) {
				if (is_host_capable())
					musb_host_tx(musb, epnum);
			} else {
				if (is_peripheral_capable())
					musb_g_tx(musb, epnum);
			}
		} else {
			/* receive */
			if (devctl & MUSB_DEVCTL_HM) {
				if (is_host_capable())
					musb_host_rx(musb, epnum);
			} else {
				if (is_peripheral_capable())
					musb_g_rx(musb, epnum);
			}
		}
	}
}
#endif
#else
#define use_dma			0
#endif

/*-------------------------------------------------------------------------*/

#ifdef CONFIG_SYSFS

static ssize_t
musb_mode_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct musb *musb = dev_to_musb(dev);
	unsigned long flags;
	int ret = -EINVAL;

	spin_lock_irqsave(&musb->lock, flags);
	ret = sprintf(buf, "%s\n", usb_otg_state_string(musb->xceiv->state));
	spin_unlock_irqrestore(&musb->lock, flags);

	return ret;
}

static ssize_t
musb_mode_store(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t n)
{
	struct musb	*musb = dev_to_musb(dev);
	unsigned long	flags;
	int		status;

	spin_lock_irqsave(&musb->lock, flags);
	if (sysfs_streq(buf, "host"))
		status = musb_platform_set_mode(musb, MUSB_HOST);
	else if (sysfs_streq(buf, "peripheral"))
		status = musb_platform_set_mode(musb, MUSB_PERIPHERAL);
	else if (sysfs_streq(buf, "otg"))
		status = musb_platform_set_mode(musb, MUSB_OTG);
	else
		status = -EINVAL;
	spin_unlock_irqrestore(&musb->lock, flags);

	return (status == 0) ? n : status;
}
static DEVICE_ATTR(mode, 0644, musb_mode_show, musb_mode_store);

static ssize_t
musb_vbus_store(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t n)
{
	struct musb	*musb = dev_to_musb(dev);
	unsigned long	flags;
	unsigned long	val;

	if (sscanf(buf, "%lu", &val) < 1) {
		dev_err(dev, "Invalid VBUS timeout ms value\n");
		return -EINVAL;
	}

	spin_lock_irqsave(&musb->lock, flags);
	/* force T(a_wait_bcon) to be zero/unlimited *OR* valid */
	musb->a_wait_bcon = val ? max_t(int, val, OTG_TIME_A_WAIT_BCON) : 0 ;
	if (musb->xceiv->state == OTG_STATE_A_WAIT_BCON)
		musb->is_active = 0;
	musb_platform_try_idle(musb, jiffies + msecs_to_jiffies(val));
	spin_unlock_irqrestore(&musb->lock, flags);

	return n;
}

static ssize_t
musb_vbus_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct musb	*musb = dev_to_musb(dev);
	unsigned long	flags;
	unsigned long	val;
	int		vbus;

	spin_lock_irqsave(&musb->lock, flags);
	val = musb->a_wait_bcon;
	/* FIXME get_vbus_status() is normally #defined as false...
	 * and is effectively TUSB-specific.
	 */
	vbus = musb_platform_get_vbus_status(musb);
	spin_unlock_irqrestore(&musb->lock, flags);

	return sprintf(buf, "Vbus %s, timeout %lu msec\n",
			vbus ? "on" : "off", val);
}
static DEVICE_ATTR(vbus, 0644, musb_vbus_show, musb_vbus_store);

/* Gadget drivers can't know that a host is connected so they might want
 * to start SRP, but users can.  This allows userspace to trigger SRP.
 */
static ssize_t
musb_srp_store(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t n)
{
	struct musb	*musb = dev_to_musb(dev);
	unsigned short	srp;

	if (sscanf(buf, "%hu", &srp) != 1
			|| (srp != 1)) {
		dev_err(dev, "SRP: Value must be 1\n");
		return -EINVAL;
	}

	if (srp == 1)
		musb_g_wakeup(musb);

	return n;
}
static DEVICE_ATTR(srp, 0644, NULL, musb_srp_store);

static struct attribute *musb_attributes[] = {
	&dev_attr_mode.attr,
	&dev_attr_vbus.attr,
	&dev_attr_srp.attr,
	NULL
};

static const struct attribute_group musb_attr_group = {
	.attrs = musb_attributes,
};

#endif	/* sysfs */

/* Only used to provide driver mode change events */
static void musb_irq_work(struct work_struct *data)
{
	struct musb *musb = container_of(data, struct musb, irq_work);
	static int old_state;

	if (musb->xceiv->state != old_state) {
		old_state = musb->xceiv->state;
		sysfs_notify(&musb->controller->kobj, NULL, "mode");
	}
}
const struct hc_driver musb_hc_driver = {
	.description		= "musb-hcd",
	.product_desc		= "MUSB HDRC host driver",
	.hcd_priv_size		= sizeof(struct musb),
	.flags			= HCD_USB2 | HCD_MEMORY,
};

/* --------------------------------------------------------------------------
 * Init support
 */

static struct musb *__init
allocate_instance(struct device *dev,
		struct musb_hdrc_config *config, void __iomem *mbase)
{
	struct musb		*musb;
	struct musb_hw_ep	*ep;
	int			epnum;
#if 0	
	struct usb_hcd	*hcd;

	hcd = usb_create_hcd(&musb_hc_driver, dev, dev_name(dev));
	if (!hcd)
		return NULL;
	/* usbcore sets dev->driver_data to hcd, and sometimes uses that... */

	musb = hcd_to_musb(hcd);
	INIT_LIST_HEAD(&musb->control);
	INIT_LIST_HEAD(&musb->in_bulk);
	INIT_LIST_HEAD(&musb->out_bulk);

	hcd->uses_new_polling = 1;
	hcd->has_tt = 1;

	musb->vbuserr_retry = VBUSERR_RETRY_COUNT;
	musb->a_wait_bcon = OTG_TIME_A_WAIT_BCON;
#endif
	musb = kzalloc(sizeof *musb, GFP_KERNEL);
	if (!musb)
		return NULL;
	dev_set_drvdata(dev, musb);
	
	musb->mregs = mbase;
	musb->ctrl_base = mbase;
	musb->nIrq = -ENODEV;
	musb->config = config;
	BUG_ON(musb->config->num_eps > MUSB_C_NUM_EPS);
	for (epnum = 0, ep = musb->endpoints;
			epnum < musb->config->num_eps;
			epnum++, ep++) {
		ep->musb = musb;
		ep->epnum = epnum;
	}

	musb->controller = dev;

	//added for ssusb:
	musb->xceiv = (struct otg_transceiver*)os_mem_alloc(sizeof(struct otg_transceiver));
	os_memset(musb->xceiv, 0, sizeof(struct usb_phy));
	musb->xceiv->state = OTG_STATE_B_IDLE; //initial its value

	return musb;
}

static void musb_free(struct musb *musb)
{
	/* this has multiple entry modes. it handles fault cleanup after
	 * probe(), where things may be partially set up, as well as rmmod
	 * cleanup after everything's been de-activated.
	 */

#ifdef CONFIG_SYSFS
	sysfs_remove_group(&musb->controller->kobj, &musb_attr_group);
#endif

	musb_gadget_cleanup(musb);

	if (musb->nIrq >= 0) {
		if (musb->irq_wake)
			disable_irq_wake(musb->nIrq);
		free_irq(musb->nIrq, musb);
	}
/*	
	if (is_dma_capable() && musb->dma_controller) {
		struct dma_controller	*c = musb->dma_controller;

		(void) c->stop(c);
		dma_controller_destroy(c);
	}
*/	
	//added for ssusb:
	os_mem_free(musb->xceiv); //free the instance allocated in allocate_instance
	
	kfree(musb);
}

/*
 * Perform generic per-controller initialization.
 *
 * @pDevice: the controller (already clocked, etc)
 * @nIrq: irq
 * @mregs: virtual address of controller registers,
 *	not yet corrected for platform-specific offsets
 */
static int __init
musb_init_controller(struct device *dev, int nIrq, void __iomem *ctrl)
{
	int			status;
	struct musb		*musb;
	struct musb_hdrc_platform_data *plat = dev->platform_data;

	/* The driver might handle more features than the board; OK.
	 * Fail when the board needs a feature that's not enabled.
	 */

	os_printk(K_DEBUG, "\n\n\n\n********************musb_init_controller\n\n\n\n");
	
	if (!plat) {
		dev_dbg(dev, "no platform_data?\n");
		status = -ENODEV;
		goto fail0;
	}

	/* allocate */
	musb = allocate_instance(dev, plat->config, ctrl);
	if (!musb) {
		status = -ENOMEM;
		goto fail0;
	}
	//pm_runtime_use_autosuspend(musb->controller);
	//pm_runtime_set_autosuspend_delay(musb->controller, 200);
	//pm_runtime_enable(musb->controller);

	spin_lock_init(&musb->lock);
	musb->board_mode = plat->mode;
	musb->board_set_power = plat->set_power;
	musb->min_power = plat->min_power;
	musb->ops = plat->platform_ops;

	/* The musb_platform_init() call:
	 *   - adjusts musb->mregs and musb->isr if needed,
	 *   - may initialize an integrated tranceiver
	 *   - initializes musb->xceiv, usually by otg_get_transceiver()
	 *   - stops powering VBUS
	 *
	 * There are various transceiver configurations.  Blackfin,
	 * DaVinci, TUSB60x0, and others integrate them.  OMAP3 uses
	 * external/discrete ones in various flavors (twl4030 family,
	 * isp1504, non-OTG, etc) mostly hooking up through ULPI.
	 */
	musb->isr = generic_interrupt;
	status = musb_platform_init(musb);
	if (status < 0)
		goto fail1;

	if (!musb->isr) {
		status = -ENODEV;
		goto fail3;
	}

	/* ideally this would be abstracted in platform setup */
	if (!is_dma_capable() || !musb->dma_controller)
		dev->dma_mask = NULL;

	/* be sure interrupts are disabled before connecting ISR */
	musb_platform_disable(musb);
	musb_generic_disable();

	/* setup musb parts of the core (especially endpoints) */
	status = musb_core_init(plat->config->multipoint
			? MUSB_CONTROLLER_MHDRC
			: MUSB_CONTROLLER_HDRC, musb);
	if (status < 0)
		goto fail3;

	setup_timer(&musb->otg_timer, musb_otg_timer_func, (unsigned long) musb);

//need to understand INIT_WORK mechanism,maked by dun.li
	/* Init IRQ workqueue before request_irq */
	INIT_WORK(&musb->irq_work, musb_irq_work);

	/* attach to the IRQ */
	if (request_irq(nIrq, musb->isr, IRQF_SHARED, dev_name(dev), musb)) {
		dev_err(dev, "request_irq %d failed!\n", nIrq);
		status = -ENODEV;
		goto fail3;
	}
	musb->nIrq = nIrq;
/* FIXME this handles wakeup irqs wrong */
	if (enable_irq_wake(nIrq) == 0) {
		musb->irq_wake = 1;
		device_init_wakeup(dev, 1);
	} else {
		musb->irq_wake = 0;
	}

	/* For the host-only role, we can activate right away.
	 * (We expect the ID pin to be forcibly grounded!!)
	 * Otherwise, wait till the gadget driver hooks up.
	 */
	MUSB_DEV_MODE(musb);
	musb->xceiv->default_a = 0;
	musb->xceiv->state = OTG_STATE_B_IDLE;

	status = musb_gadget_setup(musb);

	//dev_dbg(musb->controller, "%s mode, status %d, dev%02x\n",
	//	is_otg_enabled(musb) ? "OTG" : "PERIPHERAL",
	//	status,
	//	musb_readb(musb->mregs, MUSB_DEVCTL));

	if (status < 0)
		goto fail3;

	pm_runtime_put(musb->controller);

	status = musb_init_debugfs(musb);
	if (status < 0)
		goto fail4;

#ifdef CONFIG_SYSFS
	status = sysfs_create_group(&musb->controller->kobj, &musb_attr_group);
	if (status)
		goto fail5;
#endif

	dev_info(dev, "USB %s mode controller at %p using %s, IRQ %d\n",
			({char *s;
			 switch (musb->board_mode) {
			 case MUSB_HOST:		s = "Host"; break;
			 case MUSB_PERIPHERAL:	s = "Peripheral"; break;
			 default:		s = "OTG"; break;
			 }; s; }),
			ctrl,
			(is_dma_capable() && musb->dma_controller)
			? "DMA" : "PIO",
			musb->nIrq);

	return 0;

fail5:
	musb_exit_debugfs(musb);

fail4:
	if (!is_otg_enabled(musb) && is_host_enabled(musb))
		usb_remove_hcd(musb_to_hcd(musb));
	else
		musb_gadget_cleanup(musb);

fail3:
	if (musb->irq_wake)
		device_init_wakeup(dev, 0);
	musb_platform_exit(musb);

fail1:
	dev_err(musb->controller,
		"musb_init_controller failed with status %d\n", status);

	musb_free(musb);

fail0:

	return status;

}

/*-------------------------------------------------------------------------*/

/* all implementations (PCI bridge to FPGA, VLYNQ, etc) should just
 * bridge to a platform device; this driver then suffices.
 */

#ifndef CONFIG_MUSB_PIO_ONLY
static u64	*orig_dma_mask;
#endif

static int musb_probe(struct platform_device *pdev)
{
	struct device	*dev = &pdev->dev;
	//int		irq = platform_get_irq_byname(pdev, "mc");//new implementation in 3.2
	int		irq = platform_get_irq(pdev, 0);
	int		status;
	struct resource	*iomem;
	void __iomem	*base;

	printk("\n\nmusb_probe\n\n\n");
	iomem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
os_printk(K_DEBUG, "iomem: %p, irq: %d\n", iomem, irq);	
	if (!iomem || irq <= 0)
		return -ENODEV;

	//base = ioremap(iomem->start, resource_size(iomem)); 
	base = (void __iomem *)iomem->start;
	if (!base) {
		dev_err(dev, "ioremap failed\n");
		return -ENOMEM;
	}


#ifndef CONFIG_MUSB_PIO_ONLY
	/* clobbered by use_dma=n */
	orig_dma_mask = dev->dma_mask;
#endif
	status = musb_init_controller(dev, irq, base);

	if (status < 0)
		iounmap(base);

	return status;
}

static int musb_remove(struct platform_device *pdev)
{
	struct musb	*musb = dev_to_musb(&pdev->dev);
	void __iomem	*ctrl_base = musb->ctrl_base;

	/* this gets called on rmmod.
	 *  - Host mode: host may still be active
	 *  - Peripheral mode: peripheral is deactivated (or never-activated)
	 *  - OTG mode: both roles are deactivated (or never-activated)
	 */
	pm_runtime_get_sync(musb->controller);
	musb_exit_debugfs(musb);
	musb_shutdown(pdev);

	pm_runtime_put(musb->controller);
	musb_free(musb);
	iounmap(ctrl_base);
	device_init_wakeup(&pdev->dev, 0);
#ifndef CONFIG_MUSB_PIO_ONLY
	pdev->dev.dma_mask = orig_dma_mask;
#endif
	return 0;
}

#ifdef	CONFIG_PM

static void musb_save_context(struct musb *musb)
{
	int i;
	void __iomem *musb_base = musb->mregs;
	void __iomem *epio;

	if (is_host_enabled(musb)) {
		musb->context.frame = musb_readw(musb_base, MUSB_FRAME);
		musb->context.testmode = musb_readb(musb_base, MUSB_TESTMODE);
		musb->context.busctl = musb_read_ulpi_buscontrol(musb->mregs);
	}
	musb->context.power = musb_readb(musb_base, MUSB_POWER);
	musb->context.intrtxe = musb_readw(musb_base, MUSB_INTRTXE);
	musb->context.intrrxe = musb_readw(musb_base, MUSB_INTRRXE);
	musb->context.intrusbe = musb_readb(musb_base, MUSB_INTRUSBE);
	musb->context.index = musb_readb(musb_base, MUSB_INDEX);
	musb->context.devctl = musb_readb(musb_base, MUSB_DEVCTL);

	for (i = 0; i < musb->config->num_eps; ++i) {
		struct musb_hw_ep	*hw_ep;

		hw_ep = &musb->endpoints[i];
		if (!hw_ep)
			continue;

		epio = hw_ep->regs;
		if (!epio)
			continue;

		musb->context.index_regs[i].txmaxp =
			musb_readw(epio, MUSB_TXMAXP);
		musb->context.index_regs[i].txcsr =
			musb_readw(epio, MUSB_TXCSR);
		musb->context.index_regs[i].rxmaxp =
			musb_readw(epio, MUSB_RXMAXP);
		musb->context.index_regs[i].rxcsr =
			musb_readw(epio, MUSB_RXCSR);

		if (musb->dyn_fifo) {
			musb->context.index_regs[i].txfifoadd =
					musb_read_txfifoadd(musb_base);
			musb->context.index_regs[i].rxfifoadd =
					musb_read_rxfifoadd(musb_base);
			musb->context.index_regs[i].txfifosz =
					musb_read_txfifosz(musb_base);
			musb->context.index_regs[i].rxfifosz =
					musb_read_rxfifosz(musb_base);
		}
		/*
		if (is_host_enabled(musb)) {
			musb->context.index_regs[i].txtype =
				musb_readb(epio, MUSB_TXTYPE);
			musb->context.index_regs[i].txinterval =
				musb_readb(epio, MUSB_TXINTERVAL);
			musb->context.index_regs[i].rxtype =
				musb_readb(epio, MUSB_RXTYPE);
			musb->context.index_regs[i].rxinterval =
				musb_readb(epio, MUSB_RXINTERVAL);

			musb->context.index_regs[i].txfunaddr =
				musb_read_txfunaddr(musb_base, i);
			musb->context.index_regs[i].txhubaddr =
				musb_read_txhubaddr(musb_base, i);
			musb->context.index_regs[i].txhubport =
				musb_read_txhubport(musb_base, i);

			musb->context.index_regs[i].rxfunaddr =
				musb_read_rxfunaddr(musb_base, i);
			musb->context.index_regs[i].rxhubaddr =
				musb_read_rxhubaddr(musb_base, i);
			musb->context.index_regs[i].rxhubport =
				musb_read_rxhubport(musb_base, i);
		}
		*/
	}
}

static void musb_restore_context(struct musb *musb)
{
	int i;
	void __iomem *musb_base = musb->mregs;
	void __iomem *ep_target_regs;
	void __iomem *epio;

	if (is_host_enabled(musb)) {
		musb_writew(musb_base, MUSB_FRAME, musb->context.frame);
		musb_writeb(musb_base, MUSB_TESTMODE, musb->context.testmode);
		musb_write_ulpi_buscontrol(musb->mregs, musb->context.busctl);
	}
	musb_writeb(musb_base, MUSB_POWER, musb->context.power);
	musb_writew(musb_base, MUSB_INTRTXE, musb->context.intrtxe);
	musb_writew(musb_base, MUSB_INTRRXE, musb->context.intrrxe);
	musb_writeb(musb_base, MUSB_INTRUSBE, musb->context.intrusbe);
	musb_writeb(musb_base, MUSB_DEVCTL, musb->context.devctl);

	for (i = 0; i < musb->config->num_eps; ++i) {
		struct musb_hw_ep	*hw_ep;

		hw_ep = &musb->endpoints[i];
		if (!hw_ep)
			continue;

		epio = hw_ep->regs;
		if (!epio)
			continue;

		musb_writew(epio, MUSB_TXMAXP,
			musb->context.index_regs[i].txmaxp);
		musb_writew(epio, MUSB_TXCSR,
			musb->context.index_regs[i].txcsr);
		musb_writew(epio, MUSB_RXMAXP,
			musb->context.index_regs[i].rxmaxp);
		musb_writew(epio, MUSB_RXCSR,
			musb->context.index_regs[i].rxcsr);

		if (musb->dyn_fifo) {
			musb_write_txfifosz(musb_base,
				musb->context.index_regs[i].txfifosz);
			musb_write_rxfifosz(musb_base,
				musb->context.index_regs[i].rxfifosz);
			musb_write_txfifoadd(musb_base,
				musb->context.index_regs[i].txfifoadd);
			musb_write_rxfifoadd(musb_base,
				musb->context.index_regs[i].rxfifoadd);
		}

		if (is_host_enabled(musb)) {
			musb_writeb(epio, MUSB_TXTYPE,
				musb->context.index_regs[i].txtype);
			musb_writeb(epio, MUSB_TXINTERVAL,
				musb->context.index_regs[i].txinterval);
			musb_writeb(epio, MUSB_RXTYPE,
				musb->context.index_regs[i].rxtype);
			musb_writeb(epio, MUSB_RXINTERVAL,

			musb->context.index_regs[i].rxinterval);
			musb_write_txfunaddr(musb_base, i,
				musb->context.index_regs[i].txfunaddr);
			musb_write_txhubaddr(musb_base, i,
				musb->context.index_regs[i].txhubaddr);
			musb_write_txhubport(musb_base, i,
				musb->context.index_regs[i].txhubport);

			ep_target_regs =
				musb_read_target_reg_base(i, musb_base);

			musb_write_rxfunaddr(ep_target_regs,
				musb->context.index_regs[i].rxfunaddr);
			musb_write_rxhubaddr(ep_target_regs,
				musb->context.index_regs[i].rxhubaddr);
			musb_write_rxhubport(ep_target_regs,
				musb->context.index_regs[i].rxhubport);
		}
	}
	musb_writeb(musb_base, MUSB_INDEX, musb->context.index);
}

static int musb_suspend(struct device *dev)
{
	struct musb	*musb = dev_to_musb(dev);
	unsigned long	flags;

	spin_lock_irqsave(&musb->lock, flags);

	if (is_peripheral_active(musb)) {
		/* FIXME force disconnect unless we know USB will wake
		 * the system up quickly enough to respond ...
		 */
	} else if (is_host_active(musb)) {
		/* we know all the children are suspended; sometimes
		 * they will even be wakeup-enabled.
		 */
	}

	spin_unlock_irqrestore(&musb->lock, flags);
	return 0;
}

static int musb_resume_noirq(struct device *dev)
{
	/* for static cmos like DaVinci, register values were preserved
	 * unless for some reason the whole soc powered down or the USB
	 * module got reset through the PSC (vs just being disabled).
	 */
	return 0;
}

static int musb_runtime_suspend(struct device *dev)
{
	struct musb	*musb = dev_to_musb(dev);

	musb_save_context(musb);

	return 0;
}

static int musb_runtime_resume(struct device *dev)
{
	struct musb	*musb = dev_to_musb(dev);
	static int	first = 1;

	/*
	 * When pm_runtime_get_sync called for the first time in driver
	 * init,  some of the structure is still not initialized which is
	 * used in restore function. But clock needs to be
	 * enabled before any register access, so
	 * pm_runtime_get_sync has to be called.
	 * Also context restore without save does not make
	 * any sense
	 */
	if (!first)
		musb_restore_context(musb);
	first = 0;

	return 0;
}

static const struct dev_pm_ops musb_dev_pm_ops = {
	.suspend	= musb_suspend,
	.resume_noirq	= musb_resume_noirq,
	.runtime_suspend = musb_runtime_suspend,
	.runtime_resume = musb_runtime_resume,
};

#define MUSB_DEV_PM_OPS (&musb_dev_pm_ops)
#else
#define	MUSB_DEV_PM_OPS	NULL
#endif

static struct platform_driver musb_driver = {
    .probe  = musb_probe,
    .remove = musb_remove,
	.driver = {
		.name		= (char *)musb_driver_name,
//		.bus		= &platform_bus_type,
		.owner		= THIS_MODULE,
//		.pm		= MUSB_DEV_PM_OPS,
	},
//	.shutdown	= musb_shutdown,
};

/*-------------------------------------------------------------------------*/
extern struct platform_device usb_dev;
extern unsigned int ssusb_adb_flag;
extern unsigned int ssusb_adb_port;

static int __init musb_init(void)
{
	int retval;
	struct platform_device *pPlatformDev;
#ifdef CONFIG_OF
	struct device_node *np0;
	np0=of_find_compatible_node(NULL,NULL,"Mediatek, TVSSUSB");
	pSsusbDDmaIoMap=of_iomap(np0,0);
	pSSUSBU3DBaseIoMap=of_iomap(np0,13);
#else
	pSsusbDDmaIoMap=ioremap(PSSUSB_DMA_SET,0x10);

	pSSUSBU3DBaseIoMap=ioremap(PXHC_IO_START,0x10);
#endif
	INFO("[USB ]U3D DMA=%p, base register =0x%p \n",pSsusbDDmaIoMap,pSSUSBU3DBaseIoMap);

//#ifdef MUSB_GADGET_PORT_NUM
#if 1
	if((ssusb_adb_flag==0) || (ssusb_adb_port != 2))
#endif
    {
		return 0;
    }
	if (usb_disabled())
		return 0;
	os_printk(K_CRIT, "musb_init\n");
	
#if 1 //DMA configuration
          retval = readl((UPTR*)(ulong)SSUSB_DMA);
          retval |= 0x00000007;    
          writel(retval, SSUSB_DMA);
#endif
        if (!u3phy_ops)
            u3phy_init();
            
        u3phy_ops->init(u3phy);
    
        
        mu3d_hal_ssusb_en(); //Need this to enable the RST_CTRL.
        mu3d_hal_rst_dev();

	retval = platform_driver_register(&musb_driver);
	if (retval < 0)
	{
		printk("Problem registering musb-hdrc platform driver.\n");
		return retval;
	}
	pPlatformDev = &usb_dev;
	pPlatformDev->resource->start=(resource_size_t)pSSUSBU3DBaseIoMap;
	pPlatformDev->resource->end= (resource_size_t)(pSSUSBU3DBaseIoMap + U3D_SPACE_SIZE - 1);
  //  usb_dev.resource.start=pSSUSBU3DBaseIoMap;
  // usb_dev.resource.end=pSSUSBU3DBaseIoMap + U3D_SPACE_SIZE - 1;
	retval = platform_device_register(pPlatformDev);
	if (retval < 0)
   {
		printk("Problem registering musb-hdrc platform device.\n");
        platform_driver_unregister (&musb_driver);
    }

	pr_info("%s: version " MUSB_VERSION ", "
		"?dma?"
		", "
		"otg (peripheral+host)",
		musb_driver_name);
	return 0;

}

/* make us init after usbcore and i2c (transceivers, regulators, etc)
 * and before usb gadget and host-side drivers start to register
 */
module_init(musb_init);

static void __exit musb_cleanup(void)
{
//#ifdef MUSB_GADGET_PORT_NUM
#if 1
	if((ssusb_adb_flag==0) || (ssusb_adb_port != 2))
#endif
    {
        return;
    }
	platform_driver_unregister(&musb_driver);
}
module_exit(musb_cleanup);
