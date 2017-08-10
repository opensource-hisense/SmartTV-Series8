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
 *   Porting to suit mediatek
 *   Copyright (C) 2011-2012
 *   Wei.Wen, Mediatek Inc <wei.wen@mediatek.com>
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
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/dma-mapping.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/of_address.h>
#ifdef CONFIG_64BIT
#include <linux/soc/mediatek/platform.h>
#else
#include <mach/platform.h>
#endif


#include "musb_core.h"


#ifdef CONFIG_ARCH_DAVINCI
#include "davinci.h"
#endif

#include "musb_qmu.h"
#include "mtk_qmu_api.h"
#include "musb_dev_id.h"
#include "mt8560.h"
#include "musb_debug.h"

bool port_qmu_enable = true;
bool q_disabled = true;

extern struct platform_device usb_dev[MUC_NUM_PLATFORM_DEV];
void __iomem *pUSBamacIoMap;
void __iomem *pUSBaphyIoMap;
void __iomem *pUSBadmaIoMap;

#define TA_WAIT_BCON(m) max_t(int, (m)->a_wait_bcon, OTG_TIME_A_WAIT_BCON)


unsigned musb_debug = 0 ;

module_param_named(debug, musb_debug, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(debug, "Debug message level. Default = 0");

#define DRIVER_AUTHOR "Mentor Graphics, Texas Instruments, Nokia"
#define DRIVER_DESC "Inventra Dual-Role USB Controller Driver"

#define MUSB_VERSION "6.0"

#define DRIVER_INFO DRIVER_DESC ", v" MUSB_VERSION

#define MUSB_DRIVER_NAME "MtkGadgetUsbHcd"
const char musb_driver_name[] = MUSB_DRIVER_NAME;

MODULE_DESCRIPTION(DRIVER_INFO);
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:" MUSB_DRIVER_NAME);


/*-------------------------------------------------------------------------*/

static inline struct musb *dev_to_musb(struct device *dev)
{
#ifdef CONFIG_USB_MUSB_HDRC_HCD
	/* usbcore insists dev->driver_data is a "struct hcd *" */
	return hcd_to_musb(dev_get_drvdata(dev));
#else
	return dev_get_drvdata(dev);
#endif
}

/*-------------------------------------------------------------------------*/
extern void musb_read_fifo(struct musb_hw_ep *hw_ep, u16 len, u8 *dst);
extern void musb_write_fifo(struct musb_hw_ep *hw_ep, u16 len, const u8 *src);
void mt85xx_mask_ack_irq(unsigned int irq);
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
	void __iomem	*regs = musb->endpoints[0].regs;

	musb_ep_select(musb->mregs, 0);
	musb_write_fifo(musb->control_ep,
			sizeof(musb_test_packet), musb_test_packet);
	MGC_Write16(regs, MUSB_CSR0, MUSB_CSR0_TXPKTRDY);
}

/*-------------------------------------------------------------------------*/
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 0, 0))
const char *otg_state_string(struct musb *musb)
{
	switch (musb->xceiv.state) {
	case OTG_STATE_A_IDLE:		return "a_idle";
	case OTG_STATE_A_WAIT_VRISE:	return "a_wait_vrise";
	case OTG_STATE_A_WAIT_BCON:	return "a_wait_bcon";
	case OTG_STATE_A_HOST:		return "a_host";
	case OTG_STATE_A_SUSPEND:	return "a_suspend";
	case OTG_STATE_A_PERIPHERAL:	return "a_peripheral";
	case OTG_STATE_A_WAIT_VFALL:	return "a_wait_vfall";
	case OTG_STATE_A_VBUS_ERR:	return "a_vbus_err";
	case OTG_STATE_B_IDLE:		return "b_idle";
	case OTG_STATE_B_SRP_INIT:	return "b_srp_init";
	case OTG_STATE_B_PERIPHERAL:	return "b_peripheral";
	case OTG_STATE_B_WAIT_ACON:	return "b_wait_acon";
	case OTG_STATE_B_HOST:		return "b_host";
	default:			return "UNDEFINED";
	}
}
#endif
#ifdef	CONFIG_USB_MUSB_OTG

/*
 * Handles OTG hnp timeouts, such as b_ase0_brst
 */
void musb_otg_timer_func(unsigned long data)
{
	struct musb	*musb = (struct musb *)data;
	unsigned long	flags;

	spin_lock_irqsave(&musb->lock, flags);
	switch (musb->xceiv.state) {
	case OTG_STATE_B_WAIT_ACON:
		DBG(1, "HNP: b_wait_acon timeout; back to b_peripheral\n");
		musb_g_disconnect(musb);
		musb->xceiv.state = OTG_STATE_B_PERIPHERAL;
		musb->is_active = 0;
		break;
	case OTG_STATE_A_SUSPEND:
	case OTG_STATE_A_WAIT_BCON:
		#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 0, 0))
			DBG(1, "HNP: %s timeout\n", otg_state_string(musb));
		#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))
			DBG(1, "HNP: %s timeout\n", usb_otg_state_string(musb->xceiv.state));
		#else
			DBG(1, "HNP: %s timeout\n", otg_state_string(musb->xceiv.state));
		#endif
		musb_set_vbus(musb, 0);
		musb->xceiv.state = OTG_STATE_A_WAIT_VFALL;
		break;
	default:
		#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 0, 0))
			DBG(1, "HNP: Unhandled mode %s\n", otg_state_string(musb));
		#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))
			DBG(1, "HNP: Unhandled mode %s\n", usb_otg_state_string(musb->xceiv.state));
		#else
			DBG(1, "HNP: Unhandled mode %s\n", otg_state_string(musb->xceiv.state));
		#endif
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
	void __iomem	*mbase = musb->mregs;
	u8	reg;

	#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 0, 0))
		DBG(1, "HNP: stop from %s\n", otg_state_string(musb));
	#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))
		DBG(1, "HNP: stop from %s\n", usb_otg_state_string(musb->xceiv.state));
	#else
		DBG(1, "HNP: stop from %s\n", otg_state_string(musb->xceiv.state));
	#endif

	switch (musb->xceiv.state) {
	case OTG_STATE_A_PERIPHERAL:
		musb_g_disconnect(musb);
		#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 0, 0))
			DBG(1, "HNP: back to %s\n", otg_state_string(musb));
		#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))
			DBG(1, "HNP: back to %s\n", usb_otg_state_string(musb->xceiv.state));
		#else
			DBG(1, "HNP: back to %s\n", otg_state_string(musb->xceiv.state));
		#endif
		break;
	case OTG_STATE_B_HOST:
		DBG(1, "HNP: Disabling HR\n");
		hcd->self.is_b_host = 0;
		musb->xceiv.state = OTG_STATE_B_PERIPHERAL;
		MUSB_DEV_MODE(musb);
		reg = musb_readb(mbase, MUSB_POWER);
		reg |= MUSB_POWER_SUSPENDM;
		MGC_Write8(mbase, MUSB_POWER, reg);
		/* REVISIT: Start SESSION_REQUEST here? */
		break;
	default:
		DBG(1, "HNP: Stopping in unknown state %s\n",
		#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 0, 0))
				otg_state_string(musb));
		#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))
				usb_otg_state_string(musb->xceiv.state));
		#else
				otg_state_string(musb->xceiv.state));
		#endif
	}

	/*
	 * When returning to A state after HNP, avoid hub_port_rebounce(),
	 * which cause occasional OPT A "Did not receive reset after connect"
	 * errors.
	 */
	musb->port1_status &=
		~(1 << USB_PORT_FEAT_C_CONNECTION);
}

#endif

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

#define STAGE0_MASK (MUSB_INTR_RESUME | MUSB_INTR_SESSREQ \
		| MUSB_INTR_VBUSERROR | MUSB_INTR_CONNECT \
		| MUSB_INTR_RESET)

static irqreturn_t musb_stage0_irq(struct musb *musb, u8 int_usb,
				u8 devctl, u8 power)
{
	irqreturn_t handled = IRQ_NONE;
	void __iomem *mbase = musb->mregs;

	DBG(3, "<== Power=%02x, DevCtl=%02x, int_usb=0x%x\n", power, devctl,
		int_usb);

	/* in host mode, the peripheral may issue remote wakeup.
	 * in peripheral mode, the host may resume the link.
	 * spurious RESUME irqs happen too, paired with SUSPEND.
	 */
	if (int_usb & MUSB_INTR_RESUME) {
		handled = IRQ_HANDLED;
		#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 0, 0))
			DBG(3, "RESUME (%s)\n", otg_state_string(musb));
		#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))
			DBG(3, "RESUME (%s)\n", usb_otg_state_string(musb->xceiv.state));
		#else
			DBG(3, "RESUME (%s)\n", otg_state_string(musb->xceiv.state));
		#endif

		if (devctl & MUSB_DEVCTL_HM) {
#ifdef CONFIG_USB_MUSB_HDRC_HCD
			switch (musb->xceiv.state) {
			case OTG_STATE_A_SUSPEND:
				/* remote wakeup?  later, GetPortStatus
				 * will stop RESUME signaling
				 */

				if (power & MUSB_POWER_SUSPENDM) {
					/* spurious */
					musb->int_usb &= ~MUSB_INTR_SUSPEND;
					DBG(2, "Spurious SUSPENDM\n");
					break;
				}

				power &= ~MUSB_POWER_SUSPENDM;
				MGC_Write8(mbase, MUSB_POWER,
						power | MUSB_POWER_RESUME);

				musb->port1_status |=
						(USB_PORT_STAT_C_SUSPEND << 16)
						| MUSB_PORT_STAT_RESUME;
				musb->rh_timer = jiffies
						+ msecs_to_jiffies(20);

				musb->xceiv.state = OTG_STATE_A_HOST;
				musb->is_active = 1;
				usb_hcd_resume_root_hub(musb_to_hcd(musb));
				break;
			case OTG_STATE_B_WAIT_ACON:
				musb->xceiv.state = OTG_STATE_B_PERIPHERAL;
				musb->is_active = 1;
				MUSB_DEV_MODE(musb);
				break;
			default:
				WARNING("bogus %s RESUME (%s)\n",
					"host",
				#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 0, 0))
					otg_state_string(musb));
				#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))
					usb_otg_state_string(musb->xceiv.state));
				#else
					otg_state_string(musb->xceiv.state));
				#endif
			}
#endif
		} else {
			switch (musb->xceiv.state) {
#ifdef CONFIG_USB_MUSB_HDRC_HCD
			case OTG_STATE_A_SUSPEND:
				/* possibly DISCONNECT is upcoming */
				musb->xceiv.state = OTG_STATE_A_HOST;
				usb_hcd_resume_root_hub(musb_to_hcd(musb));
				break;
#endif
#ifdef CONFIG_USB_GADGET_MUSB_HDRC
			case OTG_STATE_B_WAIT_ACON:
			case OTG_STATE_B_PERIPHERAL:
				/* disconnect while suspended?  we may
				 * not get a disconnect irq...
				 */
				if ((devctl & MUSB_DEVCTL_VBUS)
						!= (3 << MUSB_DEVCTL_VBUS_SHIFT)
						) {
					musb->int_usb |= MUSB_INTR_DISCONNECT;
					musb->int_usb &= ~MUSB_INTR_SUSPEND;
					break;
				}
				musb_g_resume(musb);
				break;
			case OTG_STATE_B_IDLE:
				musb->int_usb &= ~MUSB_INTR_SUSPEND;
				break;
#endif
			default:
				WARNING("bogus %s RESUME (%s)\n",
					"peripheral",
				#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 0, 0))
					otg_state_string(musb));
				#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))
					usb_otg_state_string(musb->xceiv.state));
				#else
					otg_state_string(musb->xceiv.state));
				#endif
			}
		}
	}

#ifdef CONFIG_USB_MUSB_HDRC_HCD
	/* see manual for the order of the tests */
	if (int_usb & MUSB_INTR_SESSREQ) {
		#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 0, 0))
			DBG(1, "SESSION_REQUEST (%s)\n", otg_state_string(musb));
		#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))
			DBG(1, "SESSION_REQUEST (%s)\n", usb_otg_state_string(musb->xceiv.state));
		#else
			DBG(1, "SESSION_REQUEST (%s)\n", otg_state_string(musb->xceiv.state));
		#endif
		
		/* IRQ arrives from ID pin sense or (later, if VBUS power
		 * is removed) SRP.  responses are time critical:
		 *  - turn on VBUS (with silicon-specific mechanism)
		 *  - go through A_WAIT_VRISE
		 *  - ... to A_WAIT_BCON.
		 * a_wait_vrise_tmout triggers VBUS_ERROR transitions
		 */
		MGC_Write8(mbase, MUSB_DEVCTL, MUSB_DEVCTL_SESSION);
		musb->ep0_stage = MUSB_EP0_START;
		musb->xceiv.state = OTG_STATE_A_IDLE;
		MUSB_HST_MODE(musb);
		//musb_set_vbus(musb, 1);
		handled = IRQ_HANDLED;
	}

	if (int_usb & MUSB_INTR_VBUSERROR) {
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
		switch (musb->xceiv.state) {
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
				devctl |= MUSB_DEVCTL_SESSION;
				MGC_Write8(mbase, MUSB_DEVCTL, devctl);
			} else {
				musb->port1_status |=
					  (1 << USB_PORT_FEAT_OVER_CURRENT)
					| (1 << USB_PORT_FEAT_C_OVER_CURRENT);
			}
			break;
		default:
			break;
		}

		DBG(1, "VBUS_ERROR in %s (%02x, %s), retry #%d, port1 %08x\n",
		#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 0, 0))
				otg_state_string(musb),
		#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))
				usb_otg_state_string(musb->xceiv.state),
		#else
				otg_state_string(musb->xceiv.state),
		#endif
				devctl,
				({ char *s;
				switch (devctl & MUSB_DEVCTL_VBUS) {
				case 0 << MUSB_DEVCTL_VBUS_SHIFT:
					s = "<SessEnd"; break;
				case 1 << MUSB_DEVCTL_VBUS_SHIFT:
					s = "<AValid"; break;
				case 2 << MUSB_DEVCTL_VBUS_SHIFT:
					s = "<VBusValid"; break;
				/* case 3 << MUSB_DEVCTL_VBUS_SHIFT: */
				default:
					s = "VALID"; break;
				}; s; }),
				VBUSERR_RETRY_COUNT - musb->vbuserr_retry,
				musb->port1_status);

		/* go through A_WAIT_VFALL then start a new session */
		if (!ignore)
			musb_set_vbus(musb, 0);
		handled = IRQ_HANDLED;
	}

	if (int_usb & MUSB_INTR_CONNECT) {
		struct usb_hcd *hcd = musb_to_hcd(musb);

		handled = IRQ_HANDLED;
		musb->is_active = 1;
		set_bit(HCD_FLAG_SAW_IRQ, &hcd->flags);

		musb->ep0_stage = MUSB_EP0_START;

#ifdef CONFIG_USB_MUSB_OTG
		/* flush endpoints when transitioning from Device Mode */
		if (is_peripheral_active(musb)) {
			/* REVISIT HNP; just force disconnect */
		}
		MGC_Write16(mbase, MUSB_INTRTXE, musb->epmask);
		MGC_Write16(mbase, MUSB_INTRRXE, musb->epmask & 0xfffe);
		MGC_Write8(mbase, MUSB_INTRUSBE, 0xf7);
#endif
		musb->port1_status &= ~(USB_PORT_STAT_LOW_SPEED
					|USB_PORT_STAT_HIGH_SPEED
					|USB_PORT_STAT_ENABLE
					);
		musb->port1_status |= USB_PORT_STAT_CONNECTION
					|(USB_PORT_STAT_C_CONNECTION << 16);

		/* high vs full speed is just a guess until after reset */
		if (devctl & MUSB_DEVCTL_LSDEV)
			musb->port1_status |= USB_PORT_STAT_LOW_SPEED;

		/* indicate new connection to OTG machine */
		switch (musb->xceiv.state) {
		case OTG_STATE_B_PERIPHERAL:
			if (int_usb & MUSB_INTR_SUSPEND) {
				DBG(1, "HNP: SUSPEND+CONNECT, now b_host\n");
				int_usb &= ~MUSB_INTR_SUSPEND;
				goto b_host;
			} else
				DBG(1, "CONNECT as b_peripheral???\n");
			break;
		case OTG_STATE_B_WAIT_ACON:
			DBG(1, "HNP: CONNECT, now b_host\n");
b_host:
			musb->xceiv.state = OTG_STATE_B_HOST;
			hcd->self.is_b_host = 1;
			musb->ignore_disconnect = 0;
			del_timer(&musb->otg_timer);
			break;
		default:
			if ((devctl & MUSB_DEVCTL_VBUS)
					== (3 << MUSB_DEVCTL_VBUS_SHIFT)) {
				musb->xceiv.state = OTG_STATE_A_HOST;
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
		DBG(1, "CONNECT (%s) devctl %02x\n",
		#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 0, 0))
				otg_state_string(musb), devctl);
		#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))
				usb_otg_state_string(musb->xceiv.state), devctl);
		#else
				otg_state_string(musb->xceiv.state), devctl);
		#endif

	}
#endif	/* CONFIG_USB_MUSB_HDRC_HCD */

	/* mentor saves a bit: bus reset and babble share the same irq.
	 * only host sees babble; only peripheral sees bus reset.
	 */
	if (int_usb & MUSB_INTR_RESET) {
		if (is_host_capable() && (devctl & MUSB_DEVCTL_HM) != 0) {
			/*
			 * Looks like non-HS BABBLE can be ignored, but
			 * HS BABBLE is an error condition. For HS the solution
			 * is to avoid babble in the first place and fix what
			 * caused BABBLE. When HS BABBLE happens we can only
			 * stop the session.
			 */
			if (devctl & (MUSB_DEVCTL_FSDEV | MUSB_DEVCTL_LSDEV))
				DBG(1, "BABBLE devctl: %02x\n", devctl);
			else {
				ERR("Stopping host session -- babble\n");
				MGC_Write8(mbase, MUSB_DEVCTL, 0);
			}
		} else if (is_peripheral_capable()) {
			#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 0, 0))
				DBG(1, "BUS RESET as %s\n", otg_state_string(musb));
			#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))
				DBG(1, "BUS RESET as %s\n", usb_otg_state_string(musb->xceiv.state));
			#else
				DBG(1, "BUS RESET as %s\n", otg_state_string(musb->xceiv.state));
			#endif
			switch (musb->xceiv.state) {
#ifdef CONFIG_USB_OTG
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
				DBG(1, "HNP: in %s, %d msec timeout\n",
				#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 0, 0))
						otg_state_string(musb),
				#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))
						usb_otg_state_string(musb->xceiv.state),
				#else
						otg_state_string(musb->xceiv.state),
				#endif
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
				DBG(1, "HNP: RESET (%s), to b_peripheral\n",
				#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 0, 0))
						otg_state_string(musb));
				#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))
						usb_otg_state_string(musb->xceiv.state));
				#else
						otg_state_string(musb->xceiv.state));
				#endif
				musb->xceiv.state = OTG_STATE_B_PERIPHERAL;
				musb_g_reset(musb);
				break;
#endif
			case OTG_STATE_B_IDLE:
				musb->xceiv.state = OTG_STATE_B_PERIPHERAL;
				/* FALLTHROUGH */
			case OTG_STATE_B_PERIPHERAL:
				musb_g_reset(musb);
				break;
			default:
				DBG(1, "Unhandled BUS RESET as %s\n",
				#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 0, 0))
					otg_state_string(musb));
				#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))
					usb_otg_state_string(musb->xceiv.state));
				#else
					otg_state_string(musb->xceiv.state));
				#endif
			}
		}

		handled = IRQ_HANDLED;
	}
	schedule_work(&musb->irq_work);

	return handled;
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
static irqreturn_t musb_stage2_irq(struct musb *musb, u8 int_usb,
				u8 devctl, u8 power)
{
	irqreturn_t handled = IRQ_NONE;

#if 0
/* REVISIT ... this would be for multiplexing periodic endpoints, or
 * supporting transfer phasing to prevent exceeding ISO bandwidth
 * limits of a given frame or microframe.
 *
 * It's not needed for peripheral side, which dedicates endpoints;
 * though it _might_ use SOF irqs for other purposes.
 *
 * And it's not currently needed for host side, which also dedicates
 * endpoints, relies on TX/RX interval registers, and isn't claimed
 * to support ISO transfers yet.
 */
	if (int_usb & MUSB_INTR_SOF) {
		void __iomem *mbase = musb->mregs;
		struct musb_hw_ep	*ep;
		u8 epnum;
		u16 frame;

		DBG(6, "START_OF_FRAME\n");
		handled = IRQ_HANDLED;

		/* start any periodic Tx transfers waiting for current frame */
		frame = musb_readw(mbase, MUSB_FRAME);
		ep = musb->endpoints;
		for (epnum = 1; (epnum < musb->nr_endpoints)
					&& (musb->epmask >= (1 << epnum));
				epnum++, ep++) {
			/*
			 * FIXME handle framecounter wraps (12 bits)
			 * eliminate duplicated StartUrb logic
			 */
			if (ep->dwWaitFrame >= frame) {
				ep->dwWaitFrame = 0;
				pr_debug("SOF --> periodic TX%s on %d\n",
					ep->tx_channel ? " DMA" : "",
					epnum);
				if (!ep->tx_channel)
					musb_h_tx_start(musb, epnum);
				else
					cppi_hostdma_start(musb, epnum);
			}
		}		/* end of for loop */
	}
#endif

	if ((int_usb & MUSB_INTR_DISCONNECT) && !musb->ignore_disconnect) {
		DBG(1, "DISCONNECT (%s) as %s, devctl %02x\n",
		#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 0, 0))
				otg_state_string(musb),
		#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))
				usb_otg_state_string(musb->xceiv.state),
		#else
				otg_state_string(musb->xceiv.state),
		#endif
				MUSB_MODE(musb), devctl);
		handled = IRQ_HANDLED;
//		mtk_qmu_disable_all();
		switch (musb->xceiv.state) {
#ifdef CONFIG_USB_MUSB_HDRC_HCD
		case OTG_STATE_A_HOST:
		case OTG_STATE_A_SUSPEND:
			usb_hcd_resume_root_hub(musb_to_hcd(musb));
			musb_root_disconnect(musb);
			if (musb->a_wait_bcon != 0 && is_otg_enabled(musb))
				musb_platform_try_idle(musb, jiffies
					+ msecs_to_jiffies(musb->a_wait_bcon));
			break;
#endif	/* HOST */
#ifdef CONFIG_USB_MUSB_OTG
		case OTG_STATE_B_HOST:
			/* REVISIT this behaves for "real disconnect"
			 * cases; make sure the other transitions from
			 * from B_HOST act right too.  The B_HOST code
			 * in hnp_stop() is currently not used...
			 */
			musb_root_disconnect(musb);
			musb_to_hcd(musb)->self.is_b_host = 0;
			musb->xceiv.state = OTG_STATE_B_PERIPHERAL;
			MUSB_DEV_MODE(musb);
			musb_g_disconnect(musb);
			break;
		case OTG_STATE_A_PERIPHERAL:
			musb_hnp_stop(musb);
			musb_root_disconnect(musb);
			/* FALLTHROUGH */
		case OTG_STATE_B_WAIT_ACON:
			/* FALLTHROUGH */
#endif	/* OTG */
#ifdef CONFIG_USB_GADGET_MUSB_HDRC
		case OTG_STATE_B_PERIPHERAL:
		case OTG_STATE_B_IDLE:
			musb_g_disconnect(musb);
			break;
#endif	/* GADGET */
		default:
				WARNING("unhandled DISCONNECT transition (%s)\n",
				#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 0, 0))
					otg_state_string(musb));
				#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))
					usb_otg_state_string(musb->xceiv.state));
				#else
					otg_state_string(musb->xceiv.state));
				#endif
			break;
		}

		schedule_work(&musb->irq_work);
	}

	if (int_usb & MUSB_INTR_SUSPEND) {
		DBG(1, "SUSPEND (%s) devctl %02x power %02x\n",
		#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 0, 0))
				otg_state_string(musb), devctl, power);
		#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))
				usb_otg_state_string(musb->xceiv.state), devctl, power);
		#else
				otg_state_string(musb->xceiv.state), devctl, power);
		#endif
		handled = IRQ_HANDLED;

		switch (musb->xceiv.state) {
#ifdef	CONFIG_USB_MUSB_OTG
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
			musb_root_disconnect(musb);
			musb_platform_try_idle(musb, jiffies
					+ msecs_to_jiffies(musb->a_wait_bcon
						? : OTG_TIME_A_WAIT_BCON));
			break;
#endif
		case OTG_STATE_B_PERIPHERAL:
			musb_g_suspend(musb);
			musb->is_active = is_otg_enabled(musb)
					&& musb->xceiv.gadget->b_hnp_enable;
			if (musb->is_active) {
#ifdef	CONFIG_USB_MUSB_OTG
				musb->xceiv.state = OTG_STATE_B_WAIT_ACON;
				DBG(1, "HNP: Setting timer for b_ase0_brst\n");
				mod_timer(&musb->otg_timer, jiffies
					+ msecs_to_jiffies(
							OTG_TIME_B_ASE0_BRST));
#endif
			}
			break;
		case OTG_STATE_A_WAIT_BCON:
			if (musb->a_wait_bcon != 0)
				musb_platform_try_idle(musb, jiffies
					+ msecs_to_jiffies(musb->a_wait_bcon));
			break;
		case OTG_STATE_A_HOST:
			musb->xceiv.state = OTG_STATE_A_SUSPEND;
			musb->is_active = is_otg_enabled(musb)
					&& musb->xceiv.host->b_hnp_enable;
			break;
		case OTG_STATE_B_HOST:
			/* Transition to B_PERIPHERAL, see 6.8.2.6 p 44 */
			DBG(1, "REVISIT: SUSPEND as B_HOST\n");
			break;
		default:
			/* "should not happen" */
			musb->is_active = 0;
			break;
		}
		schedule_work(&musb->irq_work);
	}


	return handled;
}

/*-------------------------------------------------------------------------*/

/*
* Program the HDRC to start (enable interrupts, dma, etc.).
*/
void musb_start(struct musb *musb)
{
	void __iomem	*regs = musb->mregs;
	u8		devctl = musb_readb(regs, MUSB_DEVCTL);
	printk("<== devctl %02x\n", devctl);
	
	/*  Set INT enable registers, enable interrupts */
	MGC_Write16(regs, MUSB_INTRTXE, musb->epmask);
	MGC_Write16(regs, MUSB_INTRRXE, musb->epmask & 0xfffe);
	MGC_Write8(regs, MUSB_INTRUSBE, 0xf7);

	MGC_Write8(regs, MUSB_TESTMODE, 0);
   
  
	/* put into basic highspeed mode and start session */
	MGC_Write8(regs, MUSB_POWER, MUSB_POWER_ISOUPDATE
						| MUSB_POWER_SOFTCONN
						| MUSB_POWER_HSENAB
						/* ENSUSPEND wedges tusb */
						/* | MUSB_POWER_ENSUSPEND */
						);

	musb->is_active = 0;
	devctl = musb_readb(regs, MUSB_DEVCTL);
	devctl &= ~MUSB_DEVCTL_SESSION;

	if (is_otg_enabled(musb)) {
		/* session started after:
		 * (a) ID-grounded irq, host mode;
		 * (b) vbus present/connect IRQ, peripheral mode;
		 * (c) peripheral initiates, using SRP
		 */
		if ((devctl & MUSB_DEVCTL_VBUS) == MUSB_DEVCTL_VBUS)
			musb->is_active = 1;
		else
			devctl |= MUSB_DEVCTL_SESSION;

	} else if (is_host_enabled(musb)) {
		/* assume ID pin is hard-wired to ground */
		devctl |= MUSB_DEVCTL_SESSION;

	} else /* peripheral is enabled */ {
		if ((devctl & MUSB_DEVCTL_VBUS) == MUSB_DEVCTL_VBUS)
			musb->is_active = 1;
	}
 
	musb_platform_enable(musb);

	MGC_Write8(regs, MUSB_DEVCTL, devctl);

}


static void musb_generic_disable(struct musb *musb)
{
	void __iomem	*mbase = musb->mregs;
	u16	temp;

	/* disable interrupts */
	MGC_Write8(mbase, MUSB_INTRUSBE, 0);
	MGC_Write16(mbase, MUSB_INTRTXE, 0);
	MGC_Write16(mbase, MUSB_INTRRXE, 0);

	/* off */
	MGC_Write8(mbase, MUSB_DEVCTL, 0);

	/*  flush pending interrupts */
	temp = musb_readb(mbase, MUSB_INTRUSB);
	temp = musb_readw(mbase, MUSB_INTRTX);
	temp = musb_readw(mbase, MUSB_INTRRX);

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
	musb_generic_disable(musb);
	DBG(3, "HDRC disabled\n");

	/* FIXME
	 *  - mark host and/or peripheral drivers unusable/inactive
	 *  - disable DMA (and enable it in HdrcStart)
	 *  - make sure we can musb_start() after musb_stop(); with
	 *	OTG mode, gadget driver module rmmod/modprobe cycles that
	 *  - ...
	 */
	musb_platform_try_idle(musb, 0);
}

static void musb_shutdown(struct platform_device *pdev)
{
	struct musb	*musb = dev_to_musb(&pdev->dev);
	unsigned long	flags;
	spin_lock_irqsave(&musb->lock, flags);
	musb_platform_disable(musb);
	musb_generic_disable(musb);
#ifdef CONFIG_USB_QMU_SUPPORT
   if(musb->is_qmu)
	  mtk_disable_q_all(musb);
#endif
	if (musb->clock) {
		clk_put(musb->clock);
		musb->clock = NULL;
	}
	spin_unlock_irqrestore(&musb->lock, flags);

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
#if defined(CONFIG_USB_TUSB6010) || \
	defined(CONFIG_ARCH_OMAP2430) || defined(CONFIG_ARCH_OMAP34XX)
static ushort __initdata fifo_mode = 4;
#elif defined(CONFIG_PM) || defined(CC_SUPPORT_STR) 
static ushort fifo_mode = 5;
#else
static ushort __initdata fifo_mode = 5;
#endif

/* "modprobe ... fifo_mode=1" etc */
module_param(fifo_mode, ushort, 0);
MODULE_PARM_DESC(fifo_mode, "initial endpoint configuration");


enum fifo_style { FIFO_RXTX, FIFO_TX, FIFO_RX } __attribute__ ((packed));
enum buf_mode { BUF_SINGLE, BUF_DOUBLE } __attribute__ ((packed));

struct fifo_cfg {
	u8		hw_ep_num;
	enum fifo_style	style;
	enum buf_mode	mode;
	u16		maxpacket;
	u8 q;
};

/*
 * tables defining fifo_mode values.  define more if you like.
 * for host side, make sure both halves of ep1 are set up.
 */

/* mode 0 - fits in 2KB */
#if defined(CONFIG_PM) || defined(CC_SUPPORT_STR) 
static struct fifo_cfg mode_0_cfg[] = {
#else
static struct fifo_cfg __initdata mode_0_cfg[] = {
#endif
{ .hw_ep_num = 1, .style = FIFO_TX,   .maxpacket = 512, },
{ .hw_ep_num = 1, .style = FIFO_RX,   .maxpacket = 512, },
{ .hw_ep_num = 2, .style = FIFO_RXTX, .maxpacket = 512, },
{ .hw_ep_num = 3, .style = FIFO_RXTX, .maxpacket = 256, },
{ .hw_ep_num = 4, .style = FIFO_RXTX, .maxpacket = 256, },
};

/* mode 1 - fits in 4KB */
#if defined(CONFIG_PM) || defined(CC_SUPPORT_STR) 
static struct fifo_cfg mode_1_cfg[] = {
#else
static struct fifo_cfg __initdata mode_1_cfg[] = {
#endif
{ .hw_ep_num = 1, .style = FIFO_TX,   .maxpacket = 512, .mode = BUF_DOUBLE, },
{ .hw_ep_num = 1, .style = FIFO_RX,   .maxpacket = 512, .mode = BUF_DOUBLE, },
{ .hw_ep_num = 2, .style = FIFO_RXTX, .maxpacket = 512, .mode = BUF_DOUBLE, },
{ .hw_ep_num = 3, .style = FIFO_RXTX, .maxpacket = 256, },
{ .hw_ep_num = 4, .style = FIFO_RXTX, .maxpacket = 256, },
};

/* mode 2 - fits in 4KB */
#if defined(CONFIG_PM) || defined(CC_SUPPORT_STR) 
static struct fifo_cfg mode_2_cfg[] = {
#else
static struct fifo_cfg __initdata mode_2_cfg[] = {
#endif
{ .hw_ep_num = 1, .style = FIFO_TX,   .maxpacket = 512, },
{ .hw_ep_num = 1, .style = FIFO_RX,   .maxpacket = 512, },
{ .hw_ep_num = 2, .style = FIFO_TX,   .maxpacket = 0, },
{ .hw_ep_num = 2, .style = FIFO_RX,   .maxpacket = 4096, },
//{ .hw_ep_num = 3, .style = FIFO_RXTX, .maxpacket = 64, },
//{ .hw_ep_num = 3, .style = FIFO_RX, .maxpacket = 512, },
//{ .hw_ep_num = 3, .style = FIFO_TX, .maxpacket = 0, },
//{ .hw_ep_num = 3, .style = FIFO_RX, .maxpacket = 0, },
{ .hw_ep_num = 4, .style = FIFO_TX, .maxpacket = 64, },
{ .hw_ep_num = 4, .style = FIFO_RX, .maxpacket = 64, },
};

/* mode 3 - fits in 4KB */
#if defined(CONFIG_PM) || defined(CC_SUPPORT_STR) 
static struct fifo_cfg mode_3_cfg[] = {
#else
static struct fifo_cfg __initdata mode_3_cfg[] = {
#endif
{ .hw_ep_num = 1, .style = FIFO_TX,   .maxpacket = 512, .mode = BUF_DOUBLE, },
{ .hw_ep_num = 1, .style = FIFO_RX,   .maxpacket = 512, .mode = BUF_DOUBLE, },
{ .hw_ep_num = 2, .style = FIFO_TX,   .maxpacket = 512, },
{ .hw_ep_num = 2, .style = FIFO_RX,   .maxpacket = 512, },
{ .hw_ep_num = 3, .style = FIFO_RXTX, .maxpacket = 512, },
{ .hw_ep_num = 4, .style = FIFO_RXTX, .maxpacket = 512, },
};

/* mode 4 - fits in 16KB */
#if defined(CONFIG_PM) || defined(CC_SUPPORT_STR) 
static struct fifo_cfg mode_4_cfg[] = {
#else
static struct fifo_cfg __initdata mode_4_cfg[] = {
#endif
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


/* mode 5 - fits in 16KB */
#if defined(CONFIG_PM) || defined(CC_SUPPORT_STR) 
static struct fifo_cfg mode_5_cfg[] = {
#else
static struct fifo_cfg __initdata mode_5_cfg[] = {
#endif
{ .hw_ep_num =  1, .style = FIFO_TX,   .maxpacket = 512, },
{ .hw_ep_num =  1, .style = FIFO_RX,   .maxpacket = 512, },
{ .hw_ep_num =  2, .style = FIFO_TX,   .maxpacket = 512, },
{ .hw_ep_num =	2, .style = FIFO_RX,   .maxpacket = 512, },
{ .hw_ep_num =	3, .style = FIFO_TX,   .maxpacket = 512, },
{ .hw_ep_num =	3, .style = FIFO_RX,   .maxpacket = 512, },
{ .hw_ep_num =  4, .style = FIFO_TX,   .maxpacket = 512, },
{ .hw_ep_num =	4, .style = FIFO_RX,   .maxpacket = 512, },

};

static char ep_q_cfg[] ={
/*1-15 tx*/	
  true,
  true,
  true,
  true,
  false,
  false,
  false,
  false,
  false,
  false,
  false, 
  false,
  false,
  false,
  false,
/*1-15 rx*/ 

  true,
  true,
  true,
  true,
  false,
  false,
  false,
  false,
  false, 
  false,
  false,
  false,
  false,  
  false,
  false,
};	

static u32 ep_total_fifo_sz_cfg[]={
#ifdef CC_MT8580
20,
20,
20,
#elif defined(CC_MT8563)
12,
12,
12,
#else
8,
12,
8,
#endif
};
/*
 * configure a fifo; for non-shared endpoints, this may be called
 * once for a tx fifo and once for an rx fifo.
 *
 * returns negative errno or offset for next fifo.
 */
static int __init
fifo_setup(struct musb *musb, struct musb_hw_ep  *hw_ep,
		const struct fifo_cfg *cfg, u16 offset)
{
	void __iomem	*mbase = musb->mregs;
	u8	size = 0;
	u16	maxpacket = cfg->maxpacket;
	u16	c_off = offset >> 3;
	u8	c_size;

	/* expect hw_ep has already been zero-initialized */

	size = ffs(max(maxpacket, (u16) 8)) - 1;
	maxpacket = 1 << size;

	c_size = size - 3;
	if (cfg->mode == BUF_DOUBLE) {
		if ((offset + (maxpacket << 1)) >
				(1 << (musb->config->ram_bits + 2)))
			return -EMSGSIZE;
		c_size |= MUSB_FIFOSZ_DPB;
	} else {
#if 0
		if ((offset + maxpacket) > (1 << (musb->config->ram_bits + 2)))
			return -EMSGSIZE;
#endif
	}

	/* configure the FIFO */
	MGC_Write8(mbase, MUSB_INDEX, hw_ep->epnum);

#ifdef CONFIG_USB_MUSB_HDRC_HCD
	/* EP0 reserved endpoint for control, bidirectional;
	 * lase EP reserved for bulk, two unidirection halves.
	 */
	if (hw_ep->epnum == musb->config->num_eps-1)
		musb->bulk_ep = hw_ep;
	/* REVISIT error check:  be sure ep0 can both rx and tx ... */
#endif
	switch (cfg->style) {
	case FIFO_TX:
		musb_write_txfifosz(mbase, c_size);
		musb_write_txfifoadd(mbase, c_off);
		hw_ep->tx_double_buffered = !!(c_size & MUSB_FIFOSZ_DPB);
		hw_ep->max_packet_sz_tx = maxpacket;
        //if(musb->config->id == 0)
	  	{
	  	    hw_ep->tx_q_cfged= ep_q_cfg[hw_ep->epnum-1];
	  	}
	    INIT_LIST_HEAD(&hw_ep->tx_hep_listhead);
		break;
	case FIFO_RX:
		musb_write_rxfifosz(mbase, c_size);
		musb_write_rxfifoadd(mbase, c_off);
		hw_ep->rx_double_buffered = !!(c_size & MUSB_FIFOSZ_DPB);
		hw_ep->max_packet_sz_rx = maxpacket;
        //if(musb->config->id == 0)
	  	{
	  	    hw_ep->rx_q_cfged = ep_q_cfg[hw_ep->epnum-1+15];
	  	}
	    INIT_LIST_HEAD(&hw_ep->rx_hep_listhead);
		break;
	case FIFO_RXTX:
		musb_write_txfifosz(mbase, c_size);
		musb_write_txfifoadd(mbase, c_off);
		hw_ep->rx_double_buffered = !!(c_size & MUSB_FIFOSZ_DPB);
		hw_ep->max_packet_sz_rx = maxpacket;

		musb_write_rxfifosz(mbase, c_size);
		musb_write_rxfifoadd(mbase, c_off);
		hw_ep->tx_double_buffered = hw_ep->rx_double_buffered;
		hw_ep->max_packet_sz_tx = maxpacket;

		hw_ep->is_shared_fifo = true;
		INIT_LIST_HEAD(&hw_ep->tx_hep_listhead);
		INIT_LIST_HEAD(&hw_ep->rx_hep_listhead);
		break;
	}

	/* NOTE rx and tx endpoint irqs aren't managed separately,
	 * which happens to be ok
	 */
	musb->epmask |= (1 << hw_ep->epnum);

	return offset + (maxpacket << ((c_size & MUSB_FIFOSZ_DPB) ? 1 : 0));
}
#if defined(CONFIG_PM) || defined(CC_SUPPORT_STR) 
static struct fifo_cfg ep0_cfg = {
#else
static struct fifo_cfg __initdata ep0_cfg = {
#endif
	.style = FIFO_RXTX, .maxpacket = 64,
};

static int __init ep_config_from_table(struct musb *musb)
{
	const struct fifo_cfg	*cfg;
	unsigned		i, n;
	int			offset;
	struct musb_hw_ep	*hw_ep = musb->endpoints;

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
	}


	DBG(0, "%s: setup fifo_mode %d\n",
			musb_driver_name, fifo_mode);

	offset = fifo_setup(musb, hw_ep, &ep0_cfg, 0);
	/* assert(offset > 0) */

	/* NOTE:  for RTL versions >= 1.400 EPINFO and RAMINFO would
	 * be better than static musb->config->num_eps and DYN_FIFO_SIZE...
	 */

	for (i = 0; i < n; i++) {
		u8	epn = cfg->hw_ep_num;

		if (epn >= musb->config->num_eps) {
			break;
		}
		offset = fifo_setup(musb, hw_ep + epn, cfg++, offset);
		if (offset < 0) {

			DBG(0,"%s: mem overrun, ep %d\n",
					musb_driver_name, epn);
			return -EINVAL;
		}
		epn++;
		musb->nr_endpoints = max(epn, musb->nr_endpoints);
	}


	DBG(0, "%s: %d/%d max ep, %d/%d memory\n",
			musb_driver_name,
			n + 1, musb->config->num_eps * 2 - 1,
			offset, (1 << (musb->config->ram_bits + 2)));

#ifdef CONFIG_USB_MUSB_HDRC_HCD
	if (!musb->bulk_ep) {
		DBG(0,"%s: missing bulk\n", musb_driver_name);
		return -EINVAL;
	}
#endif

	return 0;
}


/*
 * ep_config_from_hw - when MUSB_C_DYNFIFO_DEF is false
 * @param musb the controller
 */
static int __init ep_config_from_hw(struct musb *musb)
{
	u8 epnum = 0;
	struct musb_hw_ep *hw_ep;
	void *mbase = musb->mregs;
	int ret = 0;

	DBG(2, "<== static silicon ep config\n");

	/* FIXME pick up ep0 maxpacket size */

	for (epnum = 1; epnum < musb->config->num_eps; epnum++) {
		musb_ep_select(mbase, epnum);
		hw_ep = musb->endpoints + epnum;

		ret = musb_read_fifosize(musb, hw_ep, epnum);
		if (ret < 0)
			break;

		/* FIXME set up hw_ep->{rx,tx}_double_buffered */

#ifdef CONFIG_USB_MUSB_HDRC_HCD
		/* pick an RX/TX endpoint for bulk */
		if (hw_ep->max_packet_sz_tx < 512
				|| hw_ep->max_packet_sz_rx < 512)
			continue;

		/* REVISIT:  this algorithm is lazy, we should at least
		 * try to pick a double buffered endpoint.
		 */
		if (musb->bulk_ep)
			continue;
		musb->bulk_ep = hw_ep;
#endif
	}

#ifdef CONFIG_USB_MUSB_HDRC_HCD
	if (!musb->bulk_ep) {
		pr_debug("%s: missing bulk\n", musb_driver_name);
		return -EINVAL;
	}
#endif

	return 0;
}

enum { MUSB_CONTROLLER_MHDRC, MUSB_CONTROLLER_HDRC, };

/* Initialize MUSB (M)HDRC part of the USB hardware subsystem;
 * configure endpoints, or take their config from silicon
 */
int  musb_core_init(u16 musb_type, struct musb *musb)
{
#ifdef MUSB_AHB_ID
	u32 data;
#endif
	u8 reg;
	char *type;
	u16 hwvers, rev_major, rev_minor;
	char aInfo[256], aRevision[32], aDate[12];
	void __iomem	*mbase = musb->mregs;
	int		status = 0;
	int		i;
	/* log core options (read using indexed model) */
	reg = musb_read_configdata(mbase);
	strcpy(aInfo, (reg & MUSB_CONFIGDATA_UTMIDW) ? "UTMI-16" : "UTMI-8");
	if (reg & MUSB_CONFIGDATA_DYNFIFO)
		strcat(aInfo, ", dyn FIFOs");
	if (reg & MUSB_CONFIGDATA_MPRXE) {
		strcat(aInfo, ", bulk combine");
#ifdef C_MP_RX
		musb->bulk_combine = true;
#else
		strcat(aInfo, " (X)");		/* no driver support */
#endif
	}
	if (reg & MUSB_CONFIGDATA_MPTXE) {
		strcat(aInfo, ", bulk split");
#ifdef C_MP_TX
		musb->bulk_split = true;
#else
		strcat(aInfo, " (X)");		/* no driver support */
#endif
	}
	if (reg & MUSB_CONFIGDATA_HBRXE) {
		strcat(aInfo, ", HB-ISO Rx");
		musb->hb_iso_rx = true;
	}
	if (reg & MUSB_CONFIGDATA_HBTXE) {
		strcat(aInfo, ", HB-ISO Tx");
		musb->hb_iso_tx = true;
	}
	if (reg & MUSB_CONFIGDATA_SOFTCONE)
		strcat(aInfo, ", SoftConn");
	printk(KERN_DEBUG "%s: ConfigData=0x%02x (%s)\n",
			musb_driver_name, reg, aInfo);

#ifdef MUSB_AHB_ID
	data = musb_readl(mbase, 0x404);
	sprintf(aDate, "%04d-%02x-%02x", (data & 0xffff),
		(data >> 16) & 0xff, (data >> 24) & 0xff);
	/* FIXME ID2 and ID3 are unused */
	data = musb_readl(mbase, 0x408);
	printk(KERN_DEBUG "ID2=%lx\n", (long unsigned)data);
	data = musb_readl(mbase, 0x40c);
	printk(KERN_DEBUG "ID3=%lx\n", (long unsigned)data);
	reg = musb_readb(mbase, 0x400);
	musb_type = ('M' == reg) ? MUSB_CONTROLLER_MHDRC : MUSB_CONTROLLER_HDRC;
#else
	aDate[0] = 0;
#endif
	if (MUSB_CONTROLLER_MHDRC == musb_type) {
		musb->is_multipoint = 1;
		type = "M";
	} else {
		musb->is_multipoint = 0;
		type = "";
#ifdef CONFIG_USB_MUSB_HDRC_HCD
#ifndef	CONFIG_USB_OTG_BLACKLIST_HUB
		printk(KERN_ERR
			"%s: kernel must blacklist external hubs\n",
			musb_driver_name);
#endif
#endif
	}

	/* log release info */
	hwvers = musb_read_hwvers(mbase);
	rev_major = (hwvers >> 10) & 0x1f;
	rev_minor = hwvers & 0x3ff;
	snprintf(aRevision, 32, "%d.%d%s", rev_major,
		rev_minor, (hwvers & 0x8000) ? "RC" : "");
	printk(KERN_DEBUG "%s: %sHDRC RTL version %s %s\n",
			musb_driver_name, type, aRevision, aDate);

	 /*Check HW version number*/
	 DBG(0,"hwvers=0x%08X\n",hwvers);
	 hwvers&=0x003F;
	 DBG(0,"USB VERSION CODE=0x%08X\n",hwvers);

	/* configure ep0 */
	musb_configure_ep0(musb);
	/* discover endpoint configuration */
	musb->nr_endpoints = 1;
	musb->epmask = 1;

	musb->ep_fifo = 0;
	musb->ep_fifo_total_sz = ep_total_fifo_sz_cfg[musb->config->id];
	if (reg & MUSB_CONFIGDATA_DYNFIFO) {
		DBG(0,"USB MUSB_CONFIGDATA_DYNFIFO\n");
		if (musb->config->dyn_fifo)
			{

			DBG(0,"USB MUSB_CONFIGDATA_DYNFIFO musb->config->dyn_fifo\n");
			status = ep_config_from_table(musb);
			}
		else {
			ERR("reconfigure software for Dynamic FIFOs\n");
			status = -ENODEV;
		}
	} else {

		DBG(0,"USB !MUSB_CONFIGDATA_DYNFIFO\n");
		if (!musb->config->dyn_fifo)
			status = ep_config_from_hw(musb);
		else {
			ERR("reconfigure software for static FIFOs\n");
			return -ENODEV;
		}
	}
	printk(KERN_ALERT "ep config from table status: %d\n",status);
	if (status < 0)
		return status;

	/* finish init, and print endpoint config */
	for (i = 0; i < musb->nr_endpoints; i++) {
		struct musb_hw_ep	*hw_ep = musb->endpoints + i;
		hw_ep->fifo = MUSB_FIFO_OFFSET(i) + mbase;
#ifdef CONFIG_USB_TUSB6010
		hw_ep->fifo_async = musb->async + 0x400 + MUSB_FIFO_OFFSET(i);
		hw_ep->fifo_sync = musb->sync + 0x400 + MUSB_FIFO_OFFSET(i);
		hw_ep->fifo_sync_va =
			musb->sync_va + 0x400 + MUSB_FIFO_OFFSET(i);
		if (i == 0)
			hw_ep->conf = mbase - 0x400 + TUSB_EP0_CONF;
		else
			hw_ep->conf = mbase + 0x400 + (((i - 1) & 0xf) << 2);
#endif
		hw_ep->regs = MUSB_EP_OFFSET(i, 0) + mbase;
#ifdef CONFIG_USB_MUSB_HDRC_HCD
		hw_ep->target_regs = musb_read_target_reg_base(i, mbase);
		hw_ep->rx_reinit = 1;
		hw_ep->tx_reinit = 1;
#endif
		if (hw_ep->max_packet_sz_tx) {

			DBG(0,
				"%s: hw_ep %d%s, %smax %d\n",
				musb_driver_name, i,
				hw_ep->is_shared_fifo ? "shared" : "tx",
				hw_ep->tx_double_buffered
					? "doublebuffer, " : "",
				hw_ep->max_packet_sz_tx);
		}
		if (hw_ep->max_packet_sz_rx && !hw_ep->is_shared_fifo) {

			DBG(0,
				"%s: hw_ep %d%s, %smax %d\n",
				musb_driver_name, i,
				"rx",
				hw_ep->rx_double_buffered
					? "doublebuffer, " : "",
				hw_ep->max_packet_sz_rx);
		}
		if (!(hw_ep->max_packet_sz_tx || hw_ep->max_packet_sz_rx))
			DBG(1, "hw_ep %d not configured\n", i);
			mdelay(100);
	}
	return 0;
}

/*-------------------------------------------------------------------------*/

irqreturn_t generic_interrupt(int irq, void *__hci)
{
	unsigned long	flags;
	irqreturn_t	retval = IRQ_NONE;
	struct musb	*musb = __hci;

	spin_lock_irqsave(&musb->lock, flags);

	musb->int_usb = musb_readb(musb->mregs, MUSB_INTRUSB) & musb_readb(musb->mregs, MUSB_INTRUSBE);
	musb->int_tx = musb_readw(musb->mregs, MUSB_INTRTX) & musb_readw(musb->mregs, MUSB_INTRTXE);
	musb->int_rx = musb_readw(musb->mregs, MUSB_INTRRX) & musb_readw(musb->mregs, MUSB_INTRRXE);
	musb->int_queue = musb_readl(musb->mregs, MUSB_QISAR);

    if(!musb->int_usb && !musb->int_tx && !musb->int_rx && !musb->int_queue
		&& !musb_readb(musb->mregs, 0x200))
    printk("[musb]NULL interrupt!!!\n");
	
	if(musb->int_usb)
		MGC_Write8(musb->mregs, MUSB_INTRUSB, musb->int_usb);
	
	if(musb->int_tx)
		//MGC_Write8(musb->mregs, MUSB_INTRTX,musb->int_tx);
		MGC_Write16(musb->mregs, MUSB_INTRTX, musb->int_tx);
	
	if(musb->int_rx)
		//MGC_Write8(musb->mregs, MUSB_INTRRX,musb->int_rx);
		MGC_Write16(musb->mregs, MUSB_INTRRX, musb->int_rx);

	if (musb->int_queue){
		MGC_Write32(musb->mregs, MUSB_QISAR, musb->int_queue);
		musb->int_queue &= ~(musb_readl(musb->mregs, MUSB_QIMR));
		
		//if (q_disabled)
		//	musb->int_queue = 0;
	}
#if defined(CONFIG_ARCH_MT85XX)
	mt85xx_mask_ack_irq(musb->nIrq);
#endif

	if (musb->int_usb || musb->int_tx || musb->int_rx || musb->int_queue)
		retval = musb_interrupt(musb);

	spin_unlock_irqrestore(&musb->lock, flags);

	return retval;
}

irqreturn_t dma_controller_irq(int irq, void *private_data);

irqreturn_t mtk_interrupt(int irq, void *data)
{
#ifndef CONFIG_MUSB_PIO_ONLY
	struct musb *musb=data;
#endif
	generic_interrupt(irq, data);
#ifndef CONFIG_MUSB_PIO_ONLY
	dma_controller_irq(irq, musb->dma_controller);
#endif
	return IRQ_HANDLED;
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
	u8		devctl, power;
	int		ep_num;
	u32		reg;

	devctl = musb_readb(musb->mregs, MUSB_DEVCTL);
	power = musb_readb(musb->mregs, MUSB_POWER);
	DBG(6, "** IRQ %s usb%04x tx%04x rx%04x queue%08x\n",
		(devctl & MUSB_DEVCTL_HM) ? "host" : "peripheral",
		musb->int_usb, musb->int_tx, musb->int_rx, musb->int_queue);

	/* the core can interrupt us for multiple reasons; docs have
	 * a generic interrupt flowchart to follow
	 */
	if (musb->int_usb & STAGE0_MASK)
		retval |= musb_stage0_irq(musb, musb->int_usb,
				devctl, power);

	/* "stage 1" is handling endpoint irqs */

	/* handle endpoint 0 first */
	if (musb->int_tx & 1) {
		if (devctl & MUSB_DEVCTL_HM)
			retval |= musb_h_ep0_irq(musb);
		else
			retval |= musb_g_ep0_irq(musb);
	}
	
#ifdef CONFIG_USB_QMU_SUPPORT
	/* process generic queue interrupt*/
	if (musb->int_queue)
		mtk_q_interrupt(musb);
#endif

	/* RX on endpoints 1-15 */
	reg = musb->int_rx >> 1;
	ep_num = 1;
	while (reg) {
		if (reg & 1) {
			/* musb_ep_select(musb->mregs, ep_num); */
			/* REVISIT just retval = ep->rx_irq(...) */
			retval = IRQ_HANDLED;
			if (devctl & MUSB_DEVCTL_HM) {
				if (is_host_capable())
					musb_host_rx(musb, ep_num);
			} else {
				if (is_peripheral_capable())
					musb_g_rx(musb, ep_num);
			}
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
			retval = IRQ_HANDLED;
			if (devctl & MUSB_DEVCTL_HM) {
				if (is_host_capable())
					musb_host_tx(musb, ep_num);
			} else {
				if (is_peripheral_capable())
					musb_g_tx(musb, ep_num);
			}
		}
		reg >>= 1;
		ep_num++;
	}

	/* finish handling "global" interrupts after handling fifos */
	if (musb->int_usb)
		retval |= musb_stage2_irq(musb,
				musb->int_usb, devctl, power);

	return retval;
}


#ifndef CONFIG_MUSB_PIO_ONLY
static bool __initdata use_dma = 1;

/* "modprobe ... use_dma=0" etc */
module_param(use_dma, bool, 0);

MODULE_PARM_DESC(use_dma, "enable/disable use of DMA");

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
	#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 0, 0))
		ret = sprintf(buf, "%s\n", otg_state_string(musb));
	#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))
		ret = sprintf(buf, "%s\n", usb_otg_state_string(musb->xceiv.state));
	#else
		ret = sprintf(buf, "%s\n", otg_state_string(musb->xceiv.state));
	#endif
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
		printk(KERN_ERR "Invalid VBUS timeout ms value\n");
		return -EINVAL;
	}

	spin_lock_irqsave(&musb->lock, flags);
	/* force T(a_wait_bcon) to be zero/unlimited *OR* valid */
	musb->a_wait_bcon = val ? max_t(int, val, OTG_TIME_A_WAIT_BCON) : 0 ;
	if (musb->xceiv.state == OTG_STATE_A_WAIT_BCON)
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

#ifdef CONFIG_USB_GADGET_MUSB_HDRC

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
		printk(KERN_ERR "SRP: Value must be 1\n");
		return -EINVAL;
	}
	printk("musb_srp_store.\n");
	if (srp == 1)
		musb_g_wakeup(musb);

	return n;
}

static ssize_t
musb_srp_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct musb *musb = dev_to_musb(dev);

	if(musb)
		printk("musb_srp has nothing to show !\n");
	
	return sprintf(buf, "0");
}

static DEVICE_ATTR(srp, 0644, musb_srp_show, musb_srp_store);

#endif /* CONFIG_USB_GADGET_MUSB_HDRC */

#endif	/* sysfs */

/* Only used to provide driver mode change events */
static void musb_irq_work(struct work_struct *data)
{
	struct musb *musb = container_of(data, struct musb, irq_work);
	static int old_state;

	if (musb->xceiv.state != old_state) {
		old_state = musb->xceiv.state;
		sysfs_notify(&musb->controller->kobj, NULL, "mode");
	}
}

#ifdef CONFIG_USB_MUSB_HDRC_HCD
bool musb_wifi_dev(struct usb_device *dev)
{
  int i;
  struct usb_device_id * dev_id = wifi_id_table;
  
  MUSB_ASSERT(dev);

  for(i=0; i<ARRAY_SIZE(wifi_id_table); i++)
  {
	if(dev_id->idVendor == dev->descriptor.idVendor 
		&& dev_id->idProduct == dev->descriptor.idProduct)
	{
	  return true;
	}
	
    dev_id++;
  }

  return false;
}

struct musb_hw_ep* musb_ep_binded(struct musb * musb, struct usb_host_endpoint *hep,bool is_in)
{
  struct musb_hep_node* pHepList; 
  struct musb_hep_node*	pNextHepList;
  struct list_head* listhead;
  struct musb_hw_ep* hw_ep;
  int i;
  
  for(i=1; i<musb->nr_endpoints; i++)
  {
     hw_ep = musb->endpoints + i;
     if(is_in)
     {
       listhead = &hw_ep->rx_hep_listhead;
     }
     else
     {
       listhead = &hw_ep->tx_hep_listhead;
     }
     
     list_for_each_entry_safe(pHepList, pNextHepList, listhead, list)
     {
       if(pHepList->hep == hep)
       {
         return pHepList->hw_ep;
       }
     }
  }

  return NULL;
}

bool musb_alloc_ep_fifo(struct musb* musb,struct musb_hw_ep* hw_ep,u32 ep_fifo_sz,bool is_in)
{
      u16 fifo_unit_nr = (ep_fifo_sz+511)/512;
      u16 fifosz;
      u16 free_uint = 0;
      u16 i;
      u8 reversed;
      u8 free;
      u8 found = 0;
      u16 fifoaddr;	  
      u8 index;	  

      reversed = (ep_fifo_sz<=512)?1:0;	  
        
      for(i=0; i<musb->ep_fifo_total_sz; i++)
      {
        if(reversed)
           free = !(musb->ep_fifo & (1<<(musb->ep_fifo_total_sz-1-i)));
	    else
	       free = !(musb->ep_fifo &  (1<<i));
	    
        if(free)
	      free_uint++;
	    else
	      free_uint = 0;
	 
    	 if(free_uint == fifo_unit_nr)
    	 {
    	   found =1;
    	   break;	
    	 } 
	 		
      }

      if(found == 0)
      {
         printk("[musb]fifo not enough!!!!!!!!!!!\n");
	     return false;
      }  

	if(reversed)
	  fifoaddr = musb->ep_fifo_total_sz-1-i;
	else
	  fifoaddr = i-(fifo_unit_nr-1);

      for(i=0; i< fifo_unit_nr; i++)
      {
		  musb->ep_fifo |= (1<<(fifoaddr+i));
      }
	  
      if(ep_fifo_sz <= 512)
      {
        fifosz = 6;
      }
      else if(ep_fifo_sz <= 1024)
      {
        fifosz = 7;
      }
      else if(ep_fifo_sz <= 2048)
      {
        fifosz = 8;
      }
      else if(ep_fifo_sz <= 4096)
      {
        fifosz = 9;
      }  

      index = MGC_Read8(musb->mregs, MUSB_INDEX);
      MGC_Write8(musb->mregs, MUSB_INDEX, hw_ep->epnum);

      printk("[musb]assign hwep%d rx %d fifoaddr 0x%x fifosz 0x%x\n",hw_ep->epnum,
	  	is_in,fifoaddr, fifosz);    
      printk("[musb] -ep fifo status 0x%8x\n",musb->ep_fifo);
	  
      if(is_in)
      {
 	    musb_write_rxfifosz(musb->mregs, fifosz);
		musb_write_rxfifoadd(musb->mregs, 0x08+0x40*fifoaddr);
		hw_ep->max_packet_sz_rx = ep_fifo_sz;        
  
      }
      else
      {
    	    musb_write_txfifosz(musb->mregs, fifosz);
    		musb_write_txfifoadd(musb->mregs, 0x08+0x40*fifoaddr);
    		hw_ep->max_packet_sz_tx = ep_fifo_sz;   	
      }
	  MGC_Write8(musb->mregs, MUSB_INDEX, index);
	
      return true;	  
}

bool musb_release_ep_fifo(struct musb* musb,struct musb_hw_ep* hw_ep,u32 ep_fifo_sz,bool is_in)
{
      u16 fifo_unit_nr = (ep_fifo_sz+511)/512;
      u16 i;
      u16 fifoaddr;	  
      u8 index; 

      index = MGC_Read8(musb->mregs, MUSB_INDEX);
      MGC_Write8(musb->mregs, MUSB_INDEX, hw_ep->epnum);

      if(is_in)
        fifoaddr = musb_read_rxfifoadd(musb->mregs);
	  else
		fifoaddr = musb_read_txfifoadd(musb->mregs); 

      
	  fifoaddr =  (fifoaddr-0x08)/0x40;   	

      for(i=0; i< fifo_unit_nr; i++)
      {
		  musb->ep_fifo &= ~(1<<(fifoaddr+i));
      }

      printk("[musb] release ep%d in%d -ep fifo status 0x%8x\n",hw_ep->epnum,is_in,musb->ep_fifo);

      if(is_in)
      {
 	    musb_write_rxfifosz(musb->mregs, 0);
		musb_write_rxfifoadd(musb->mregs, 0);
		hw_ep->max_packet_sz_rx = 0;        
      	  
      }
	  else
	  {
	    musb_write_txfifosz(musb->mregs, 0);
		musb_write_txfifoadd(musb->mregs, 0);
		hw_ep->max_packet_sz_tx = 0;   
	  }
	  MGC_Write8(musb->mregs, MUSB_INDEX, index);
	
      return true;	    
}

//insert hep into lep
bool musb_lep_insert_hep(struct musb* musb,struct musb_hw_ep* hw_ep, struct usb_host_endpoint *hep,bool q_wanted)
{
    struct musb_hep_node* hep_node = kzalloc(sizeof(*hep_node), GFP_KERNEL);
	struct list_head* head;
	bool is_in = hep->desc.bEndpointAddress & USB_DIR_IN;
	u16 maxpktsz = le16_to_cpu(hep->desc.wMaxPacketSize)&0x7ff ;
	u32 ep_fifo_sz = maxpktsz *(1 + ((le16_to_cpu(hep->desc.wMaxPacketSize) >> 11) & 0x03));
    bool ep_cfged;
	bool q_used = false;
	
	if(hep_node == NULL)
	 return false;

	hep_node->hep = hep;
	hep_node->hw_ep = hw_ep;

	head = is_in ? &hw_ep->rx_hep_listhead : &hw_ep->tx_hep_listhead;
	ep_cfged = is_in ? hw_ep->rx_q_cfged : hw_ep->tx_q_cfged;

    if(ep_cfged && q_wanted)
	  q_used = true;

	if(list_empty(head))
	{
	   musb_alloc_ep_fifo(musb,hw_ep, ep_fifo_sz, is_in);
	   if(is_in)
		 hw_ep->rx_q_used = q_used;
	   else
		 hw_ep->tx_q_used = q_used;	   
	}
						
	list_add_tail(&hep_node->list, head); 
	
	printk("[musb]binding hep%x to lep%d q_used%d\n",hep->desc.bEndpointAddress,hw_ep->epnum,q_used);
	
    return true;
}

//remove hep from local ep
struct musb_hw_ep* musb_lep_remove_ep(struct musb * musb, struct usb_host_endpoint *hep)
{
	  struct musb_hep_node* pHepList; 
	  struct musb_hep_node* pNextHepList;
	  struct list_head* listhead;
	  struct musb_hw_ep* hw_ep;
	  int i;
	//	int bIndex;
	  bool is_in = hep->desc.bEndpointAddress & USB_DIR_IN;
	  u16 maxpktsz = le16_to_cpu(hep->desc.wMaxPacketSize)&0x7ff ;
	  u32 ep_fifo_sz = maxpktsz *(1 + ((le16_to_cpu(hep->desc.wMaxPacketSize) >> 11) & 0x03));
		  
	  
	  for(i=1; i<musb->nr_endpoints; i++)
	  {
		 hw_ep = musb->endpoints + i;
		 if(is_in)
		   listhead = &hw_ep->rx_hep_listhead;
		 else
		   listhead = &hw_ep->tx_hep_listhead;
		 
		 list_for_each_entry_safe(pHepList, pNextHepList, listhead, list)
		 {
		   if(pHepList->hep == hep)
		   {
			 list_del(&pHepList->list);
			 kfree(pHepList);
	
			 if(list_empty(listhead))
			 {
			   if(is_in)
			   	 hw_ep->rx_q_used = false;
			   else
			   	 hw_ep->tx_q_used = false;
			   
			   musb_release_ep_fifo(musb,hw_ep, ep_fifo_sz, is_in);
			 }	
			 
			 printk("[musb]disable hep%x @ lep%d\n",hep->desc.bEndpointAddress,hw_ep->epnum);
		   }
		 }
	  }
	
	  return NULL;

}


int musb_check_free_ep(struct usb_device *dev, int configuration)
{
    struct musb * musb;
    struct usb_hcd *pHcd = NULL;
	struct usb_host_config *cp = NULL;
    //MGC_LinuxLocalEnd *pEnd = NULL;
    //int aIsLocalEndOccupy[2][MUSB_C_NUM_EPS];
//    uint16_t wPacketSize = 0;
    unsigned int nOut = 0;
	int nintf = 0; 
    //int nEnd = -1;
	int i = 0;
	struct musb_hw_ep* hw_ep;
	//musb_hep_node* hep_node;
	struct list_head	*head;
	int k;
	bool q_wanted = false;
	bool q_used;
	
    struct usb_device_descriptor *pDescriptor = &dev->descriptor;

	if(!dev)
	{
        printk("usb_dev is NULL: %s(%d)\n", __FILE__, __LINE__);
        return -1;
	}

    if(!dev->bus)
    {
        printk("usb_bus is NULL: %s(%d)\n", __FILE__, __LINE__);
        return -1;
    }

    pHcd = bus_to_hcd(dev->bus);

    if(!dev->bus)
    {
        printk("usb_hcd is NULL: %s(%d)\n", __FILE__, __LINE__);
        return -1;
    }
    
	musb = hcd_to_musb(bus_to_hcd(dev->bus));

    if(!musb)
    {
        printk("musb is NULL: %s(%d)\n", __FILE__, __LINE__);
        return -1;
    }

	if (dev->authorized == 0 || configuration == -1)
	{
		configuration = 0;
	}
	else 
	{
		for (i = 0; i < dev->descriptor.bNumConfigurations; i++) 
		{
			if (dev->config[i].desc.bConfigurationValue == configuration) 
			{
				cp = &dev->config[i];
				break;
			}
		}
	}
	if ((!cp && configuration != 0))
		return -EINVAL;

	if(pDescriptor->idVendor == 0x1d6b &&
	   pDescriptor->idProduct == 0x0002 &&
	   pDescriptor->bDeviceClass == 0x09)
	{
        printk("[USB CFE] Root Hub found.\n");
        return 0;
	}

	if(dev->parent)
	{
	    printk("[USB:parent] level  = 0x%04X\n", dev->parent->level);
		printk("[USB:parent] idVendor  = 0x%04X\n", dev->parent->descriptor.idVendor);
		printk("[USB:parent] idProduct = 0x%04X\n", dev->parent->descriptor.idProduct);
		printk("[USB:parent] bDeviceClass = 0x%04X\n", dev->parent->descriptor.bDeviceClass);
	}
		
	if (cp && configuration == 0)
		dev_warn(&dev->dev, "config 0 descriptor??\n");

    if(cp)
    {
        nintf = cp->desc.bNumInterfaces;

    	for (i = 0; i < nintf; ++i) 
    	{
    		struct usb_interface *intf = cp->interface[i];
        	struct usb_host_interface *iface_desc;
        	struct usb_endpoint_descriptor *ep_desc;
        	int j = 0;
        	int found = 0;
        	int epnum = 0;
        	int is_in = 0;
            struct usb_host_endpoint *ep = NULL;  


		    printk("[USB] :interfaceb NumEndpoints  = 0x%X\n", intf->cur_altsetting->desc.bNumEndpoints);
		    printk("[USB:interface] bInterfaceClass  = 0x%X\n", intf->cur_altsetting->desc.bInterfaceClass);		


        	iface_desc = intf->cur_altsetting;
			
        	for (j = 0; j < iface_desc->desc.bNumEndpoints; ++j) 
        	{
        		ep_desc = &iface_desc->endpoint[j].desc;
        	    epnum = usb_endpoint_num(ep_desc);
        	    is_in= usb_endpoint_dir_out(ep_desc)? 0 : 1;
			    //iso and wifi bulk use q	
				if(((musb_wifi_dev(dev)) 
					  && ((ep_desc->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK)
            				== USB_ENDPOINT_XFER_BULK))
					|| ((ep_desc->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK)
            				== USB_ENDPOINT_XFER_ISOC))
				{
				  q_wanted = true;
				}
				else 
				{
				  q_wanted = false;
				}

        		printk("[USB] Type: 0x%02X\n", ep_desc->bDescriptorType);
        		printk("[USB] Addr: 0x%02X\n", ep_desc->bEndpointAddress);
        		printk("[USB] Attr: 0x%02X\n", ep_desc->bmAttributes);
        		printk("[USB] Intr: 0x%02X\n", ep_desc->bInterval);
        		printk("[USB] MaxP: 0x%04X\n", ep_desc->wMaxPacketSize);

                nOut = (ep_desc->bEndpointAddress & USB_DIR_IN)? 0 : 1;
                found = 0;
                
            	if (is_in) 
            	{
            		ep = dev->ep_in[epnum];
            	}
            	if (!is_in) 
            	{
            		ep = dev->ep_out[epnum];
            	}

#ifdef USB_DISABLE_MSD_FOR_IPOD
                if(dev->quirks == USB_QUIRK_NOT_SUPPORT_APPLE_MSD)
                {
                    if(iface_desc->desc.bInterfaceClass == USB_CLASS_MASS_STORAGE && 
                       iface_desc->desc.bInterfaceSubClass == 0x06)
                    {   
                        goto NO_FOUND;
                    }
                }
#endif 
            	if(ep && musb_ep_binded(musb,ep,is_in))
            	{
                    found = 1;
                    continue;
            	}

                // BULK reserved EP
                if ((ep_desc->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK)
            				== USB_ENDPOINT_XFER_BULK)
                {
                    if (iface_desc->desc.bInterfaceClass == USB_CLASS_MASS_STORAGE)
                    {
                        hw_ep = musb->bulk_ep;

						q_used = is_in ? hw_ep->rx_q_used : hw_ep->tx_q_used;

						if(q_used)
							goto NO_FOUND;
						
                        musb_lep_insert_hep(musb,hw_ep,ep,q_wanted);
                        found = 1;
                        continue;
                    }
                }

              
                for (k = 1; k < musb->nr_endpoints-1; k++)
                {
                    if(q_wanted)
                     hw_ep = musb->endpoints + k;
					else	
                     hw_ep = musb->endpoints + (musb->nr_endpoints-1-k);	
					 
					 head = is_in ? &hw_ep->rx_hep_listhead : &hw_ep->tx_hep_listhead;
					
                    if(!list_empty(head))
                    {
                      continue;
                    }
					
					found = 1;
					musb_lep_insert_hep(musb,hw_ep,ep,q_wanted);
					break;
			
                }

				if(found == 0)
				{
                //USE reserved EP for BULK 
                    if ((ep_desc->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK)
            				== USB_ENDPOINT_XFER_BULK)
                    {
                        hw_ep = musb->bulk_ep;

						q_used = is_in ? hw_ep->rx_q_used : hw_ep->tx_q_used;

						if(q_used)
							goto NO_FOUND;

					    head = is_in ? &hw_ep->rx_hep_listhead : &hw_ep->tx_hep_listhead;
					
                        if(!list_empty(head))
							q_wanted = false;

                        musb_lep_insert_hep(musb,hw_ep,ep,q_wanted);
						
                        found = 1;
                        continue;                   
                    }
				}
 
                if(found == 0)
                {
//#ifdef USB_DISABLE_MSD_FOR_IPOD
                    NO_FOUND:
//#endif
                    if(iface_desc)
                    {
                        ep_desc->bEndpointAddress = 0xEE;
#ifdef USB_DISABLE_MSD_FOR_IPOD
                        if(dev->quirks == USB_QUIRK_NOT_SUPPORT_APPLE_MSD)
                        {
                            ep_desc->bEndpointAddress = 0xAA;
                        }
#endif
                        iface_desc->desc.bInterfaceClass = 0xEE;
                        iface_desc->desc.bInterfaceSubClass = 0xEE;                    
                    }

                    return -1;
                }
        	}
    	}
    }
	
    return 0;
}
#endif

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
#ifdef CONFIG_USB_MUSB_HDRC_HCD
	struct usb_hcd	*hcd;

	hcd = usb_create_hcd(&musb_hc_driver, dev, (char *)dev_name(dev));
	if (!hcd)
		return NULL;
	/* usbcore sets dev->driver_data to hcd, and sometimes uses that... */

	musb = hcd_to_musb(hcd);
	INIT_LIST_HEAD(&musb->control);
	INIT_LIST_HEAD(&musb->in_bulk);
	INIT_LIST_HEAD(&musb->out_bulk);

	hcd->uses_new_polling = 1;
	hcd->check_free_ep = musb_check_free_ep;
	musb->vbuserr_retry = VBUSERR_RETRY_COUNT;
	musb->a_wait_bcon = OTG_TIME_A_WAIT_BCON;
#else
	musb = kzalloc(sizeof *musb, GFP_KERNEL);
	if (!musb)
		return NULL;
	dev_set_drvdata(dev, musb);

#endif
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
	return musb;
}

static void musb_free(struct musb *musb)
{
	/* this has multiple entry modes. it handles fault cleanup after
	 * probe(), where things may be partially set up, as well as rmmod
	 * cleanup after everything's been de-activated.
	 */

#ifdef CONFIG_SYSFS
	device_remove_file(musb->controller, &dev_attr_mode);
	device_remove_file(musb->controller, &dev_attr_vbus);
#ifdef CONFIG_USB_GADGET_MUSB_HDRC
	device_remove_file(musb->controller, &dev_attr_srp);
#endif
#endif

#ifdef CONFIG_USB_GADGET_MUSB_HDRC
	musb_gadget_cleanup(musb);
#endif

	if (musb->nIrq >= 0) {
		if (musb->irq_wake)
			disable_irq_wake(musb->nIrq);
		free_irq(musb->nIrq, musb);
	}
	if (is_dma_capable() && musb->dma_controller) {
		struct dma_controller	*c = musb->dma_controller;

		(void) c->stop(c);
		dma_controller_destroy(c);
	}

	MGC_Write8(musb->mregs, MUSB_DEVCTL, 0);
	musb_platform_exit(musb);
	MGC_Write8(musb->mregs, MUSB_DEVCTL, 0);

	if (musb->clock) {
		clk_disable(musb->clock);
		clk_put(musb->clock);
	}

#ifdef CONFIG_USB_MUSB_OTG
	put_device(musb->xceiv.dev);
#endif

#ifdef CONFIG_USB_MUSB_HDRC_HCD
	usb_put_hcd(musb_to_hcd(musb));
#else
	kfree(musb);
#endif
}
/*
 * Perform generic per-controller initialization.
 *
 * @pDevice: the controller (already clocked, etc)
 * @nIrq: irq
 * @mregs: virtual address of controller registers,
 *	not yet corrected for platform-specific offsets
 */
int __init
musb_init_controller(struct device *dev, int nIrq, void __iomem *ctrl)
{
	int			status;
	struct musb		*musb;
	struct musb_hdrc_platform_data *plat = dev->platform_data;

	/* The driver might handle more features than the board; OK.
	 * Fail when the board needs a feature that's not enabled.
	 */
	if (!plat) {
		dev_dbg(dev, "no platform_data?\n");
		return -ENODEV;
	}
	//printk(" %s mode = %d, called.\n", __func__, plat_mode);
	switch (plat->mode) {
	case MUSB_HOST:
#ifdef CONFIG_USB_MUSB_HDRC_HCD
		break;
#else
		goto bad_config;
#endif
	case MUSB_PERIPHERAL:
#ifdef CONFIG_USB_GADGET_MUSB_HDRC
		break;
#else
		goto bad_config;
#endif
	case MUSB_OTG:
#ifdef CONFIG_USB_MUSB_OTG
		break;
#else
bad_config:
#endif
	default:
		dev_err(dev, "incompatible Kconfig role setting\n");
		return -EINVAL;
	}

	/* allocate */
	musb = allocate_instance(dev, plat->config, ctrl);
	if (!musb)
		return -ENOMEM;

	spin_lock_init(&musb->lock);
	musb->board_mode = plat->mode;
	musb->board_set_power = plat->set_power;
	musb->set_clock = plat->set_clock;
	musb->min_power = plat->min_power;
#ifdef CONFIG_USB_QMU_SUPPORT
	if(port_qmu_enable)
	 musb->is_qmu=true;
	else
	musb->is_qmu=false;
#endif
	/* Clock usage is chip-specific ... functional clock (DaVinci,
	 * OMAP2430), or PHY ref (some TUSB6010 boards).  All this core
	 * code does is make sure a clock handle is available; platform
	 * code manages it during start/stop and suspend/resume.
	 */
	if (plat->clock) {
		musb->clock = clk_get(dev, plat->clock);
		if (IS_ERR(musb->clock)) {
			status = PTR_ERR(musb->clock);
			musb->clock = NULL;
			goto fail;
		}
	}

	/* The musb_platform_init() call:
	 *   - adjusts musb->mregs and musb->isr if needed,
	 *   - may initialize an integrated tranceiver
	 *   - initializes musb->xceiv, usually by otg_get_transceiver()
	 *   - activates clocks.
	 *   - stops powering VBUS
	 *   - assigns musb->board_set_vbus if host mode is enabled
	 *
	 * There are various transciever configurations.  Blackfin,
	 * DaVinci, TUSB60x0, and others integrate them.  OMAP3 uses
	 * external/discrete ones in various flavors (twl4030 family,
	 * isp1504, non-OTG, etc) mostly hooking up through ULPI.
	 */
	musb->isr = mtk_interrupt;
	status = musb_platform_init(musb);
	if (status < 0)
		goto fail;
	if (!musb->isr) {
		status = -ENODEV;
		goto fail2;
	}

#ifndef CONFIG_MUSB_PIO_ONLY
	if (use_dma && dev->dma_mask) {
		struct dma_controller	*c;

		c = dma_controller_create(musb, musb->mregs);
		musb->dma_controller = c;
		if (c)
			(void) c->start(c);
	}
#endif

	/* ideally this would be abstracted in platform setup */
	if (!is_dma_capable() || !musb->dma_controller)
		dev->dma_mask = NULL;

	/* be sure interrupts are disabled before connecting ISR */
//	musb_platform_disable(musb);
//	musb_generic_disable(musb);

	/* setup musb parts of the core (especially endpoints) */
	status = musb_core_init(plat->config->multipoint
			? MUSB_CONTROLLER_MHDRC
			: MUSB_CONTROLLER_HDRC, musb);
	if (status < 0)
		goto fail2;
	/* Init IRQ workqueue before request_irq */
	INIT_WORK(&musb->irq_work, musb_irq_work);

	musb->nIrq = nIrq;

	pr_info("%s: USB %s mode controller at %p using %s, IRQ %d\n",
			musb_driver_name,
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

	/* host side needs more setup */
	if (is_host_enabled(musb)) {
		struct usb_hcd	*hcd = musb_to_hcd(musb);

		//otg_set_host(musb->xceiv, &hcd->self);

		if (is_otg_enabled(musb))
			hcd->self.otg_port = 1;
		musb->xceiv.host = &hcd->self;
		hcd->power_budget = 2 * (plat->power ? : 250);
	}

	/* For the host-only role, we can activate right away.
	 * (We expect the ID pin to be forcibly grounded!!)
	 * Otherwise, wait till the gadget driver hooks up.
	 */
	if (!is_otg_enabled(musb) && is_host_enabled(musb)) {
		MUSB_HST_MODE(musb);
		musb->xceiv.default_a = 1;
		musb->xceiv.state = OTG_STATE_A_IDLE;

		status = usb_add_hcd(musb_to_hcd(musb), -1, 0);
		if (status)
			goto fail;

		DBG(1, "%s mode, status %d, devctl %02x %c\n",
			"HOST", status,
			musb_readb(musb->mregs, MUSB_DEVCTL),
			(musb_readb(musb->mregs, MUSB_DEVCTL)
					& MUSB_DEVCTL_BDEVICE
				? 'B' : 'A'));

	} else /* peripheral is enabled */ {
		MUSB_DEV_MODE(musb);
		musb->xceiv.default_a = 0;
		musb->xceiv.state = OTG_STATE_B_IDLE;

		status = musb_gadget_setup(musb);
		if (status)
			goto fail;


		DBG(1,"%s mode, status %d, dev%02x\n",
			is_otg_enabled(musb) ? "OTG" : "PERIPHERAL",
			status,
			musb_readb(musb->mregs, MUSB_DEVCTL));
	}
	
	/* attach to the IRQ */
	if (request_irq(nIrq, musb->isr, IRQF_SHARED, dev_name(dev), musb)) {
		dev_err(dev, "request_irq %d failed!\n", nIrq);
		status = -ENODEV;
		goto fail2;
	}

/* FIXME this handles wakeup irqs wrong */
	if (enable_irq_wake(nIrq) == 0) {
		musb->irq_wake = 1;
		device_init_wakeup(dev, 1);
	} else {
		musb->irq_wake = 0;
	}

#ifdef CONFIG_SYSFS
	status = device_create_file(dev, &dev_attr_mode);
	status = device_create_file(dev, &dev_attr_vbus);
#ifdef CONFIG_USB_GADGET_MUSB_HDRC
	status = device_create_file(dev, &dev_attr_srp);
#endif /* CONFIG_USB_GADGET_MUSB_HDRC */
	status = 0;
#endif
	if (status)
		goto fail2;

	return 0;

fail2:
#ifdef CONFIG_SYSFS
	device_remove_file(musb->controller, &dev_attr_mode);
	device_remove_file(musb->controller, &dev_attr_vbus);
#ifdef CONFIG_USB_GADGET_MUSB_HDRC
	device_remove_file(musb->controller, &dev_attr_srp);
#endif
#endif
	musb_platform_exit(musb);
fail:
	dev_err(musb->controller,
		"musb_init_controller failed with status %d\n", status);

	if (musb->clock)
		clk_put(musb->clock);
	device_init_wakeup(dev, 0);
	musb_free(musb);

	return status;

}

/*-------------------------------------------------------------------------*/

/* all implementations (PCI bridge to FPGA, VLYNQ, etc) should just
 * bridge to a platform device; this driver then suffices.
 */

#ifndef CONFIG_MUSB_PIO_ONLY
static u64	*orig_dma_mask;
#endif

void __iomem	*musb_base;

static int __init musb_probe(struct platform_device *pdev)
{
	struct device	*dev = &pdev->dev;
	int		irq = platform_get_irq(pdev, 0);
	struct resource	*iomem;
	void __iomem	*base;

	iomem = platform_get_resource(pdev, IORESOURCE_MEM, 0);

	if (!iomem || irq == 0)
		return -ENODEV;

	base = (void __iomem *)iomem->start;
	if (!base) {
		dev_err(dev, "ioremap failed\n");
		return -ENOMEM;
	}
	musb_base = base;

#ifndef CONFIG_MUSB_PIO_ONLY
	/* clobbered by use_dma=n */
	orig_dma_mask = dev->dma_mask;
#endif
	return musb_init_controller(dev, irq, base);
}

static int musb_remove(struct platform_device *pdev)
{
	struct musb	*musb = dev_to_musb(&pdev->dev);
#if 0
	void __iomem	*ctrl_base = musb->ctrl_base;
#endif

	/* this gets called on rmmod.
	 *  - Host mode: host may still be active
	 *  - Peripheral mode: peripheral is deactivated (or never-activated)
	 *  - OTG mode: both roles are deactivated (or never-activated)
	 */
	musb_shutdown(pdev);
#ifdef CONFIG_USB_MUSB_HDRC_HCD
	if (musb->board_mode == MUSB_HOST)
		usb_remove_hcd(musb_to_hcd(musb));
#endif
	musb_free(musb);
	device_init_wakeup(&pdev->dev, 0);
#ifndef CONFIG_MUSB_PIO_ONLY
	pdev->dev.dma_mask = orig_dma_mask;
#endif
	return 0;
}

#if defined(CONFIG_PM) || defined(CC_SUPPORT_STR) 
static int fifo_set_restore(struct musb *musb, struct musb_hw_ep  *hw_ep,
		const struct fifo_cfg *cfg, u16 offset)
{
	void __iomem	*mbase = musb->mregs;
	u8	size = 0;
	u16	maxpacket = cfg->maxpacket;
	u16	c_off = offset >> 3;
	u8	c_size;

	/* expect hw_ep has already been zero-initialized */

	size = ffs(max(maxpacket, (u16) 8)) - 1;
	maxpacket = 1 << size;

	c_size = size - 3;
	if (cfg->mode == BUF_DOUBLE) {
		if ((offset + (maxpacket << 1)) >
				(1 << (musb->config->ram_bits + 2)))
			return -EMSGSIZE;
		c_size |= MUSB_FIFOSZ_DPB;
	} else {
#if 0
		if ((offset + maxpacket) > (1 << (musb->config->ram_bits + 2)))
			return -EMSGSIZE;
#endif
	}

	/* configure the FIFO */
	MGC_Write8(mbase, MUSB_INDEX, hw_ep->epnum);

#ifdef CONFIG_USB_MUSB_HDRC_HCD
	/* EP0 reserved endpoint for control, bidirectional;
	 * lase EP reserved for bulk, two unidirection halves.
	 */
	if (hw_ep->epnum == musb->config->num_eps-1)
		musb->bulk_ep = hw_ep;
	/* REVISIT error check:  be sure ep0 can both rx and tx ... */
#endif
	switch (cfg->style) {
	case FIFO_TX:
		musb_write_txfifosz(mbase, c_size);
		musb_write_txfifoadd(mbase, c_off);
		hw_ep->tx_double_buffered = !!(c_size & MUSB_FIFOSZ_DPB);
		hw_ep->max_packet_sz_tx = maxpacket;
        if(musb->config->id == 0)
	  	{
	  	    hw_ep->tx_q_cfged= ep_q_cfg[hw_ep->epnum-1];
	  	}
		break;
	case FIFO_RX:
		musb_write_rxfifosz(mbase, c_size);
		musb_write_rxfifoadd(mbase, c_off);
		hw_ep->rx_double_buffered = !!(c_size & MUSB_FIFOSZ_DPB);
		hw_ep->max_packet_sz_rx = maxpacket;
        if(musb->config->id == 0)
	  	{
	  	    hw_ep->rx_q_cfged = ep_q_cfg[hw_ep->epnum-1+15];
	  	}
		break;
	case FIFO_RXTX:
		musb_write_txfifosz(mbase, c_size);
		musb_write_txfifoadd(mbase, c_off);
		hw_ep->rx_double_buffered = !!(c_size & MUSB_FIFOSZ_DPB);
		hw_ep->max_packet_sz_rx = maxpacket;

		musb_write_rxfifosz(mbase, c_size);
		musb_write_rxfifoadd(mbase, c_off);
		hw_ep->tx_double_buffered = hw_ep->rx_double_buffered;
		hw_ep->max_packet_sz_tx = maxpacket;

		hw_ep->is_shared_fifo = true;
		break;
	}

	/* NOTE rx and tx endpoint irqs aren't managed separately,
	 * which happens to be ok
	 */
	musb->epmask |= (1 << hw_ep->epnum);

	return offset + (maxpacket << ((c_size & MUSB_FIFOSZ_DPB) ? 1 : 0));
}

static int ep_restore_from_table(struct musb *musb)
{
	const struct fifo_cfg	*cfg;
	unsigned		i, n;
	int			offset;
	struct musb_hw_ep	*hw_ep = musb->endpoints;

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
	}

	printk(KERN_DEBUG "%s: setup fifo_mode %d\n",
			musb_driver_name, fifo_mode);

	offset = fifo_set_restore(musb, hw_ep, &ep0_cfg, 0);
	/* assert(offset > 0) */

	/* NOTE:  for RTL versions >= 1.400 EPINFO and RAMINFO would
	 * be better than static musb->config->num_eps and DYN_FIFO_SIZE...
	 */

	for (i = 0; i < n; i++) {
		u8	epn = cfg->hw_ep_num;

		if (epn >= musb->config->num_eps) {
			break;
		}
		offset = fifo_set_restore(musb, hw_ep + epn, cfg++, offset);
		if (offset < 0) {
		pr_debug("%s: mem overrun, ep %d\n",
				musb_driver_name, epn);
		return -EINVAL;
		}
		epn++;
		musb->nr_endpoints = max(epn, musb->nr_endpoints);
	}
	return 0;
}

int  musb_endpoint_fifo_restore(struct musb *musb)
{
	void __iomem	*mbase = musb->mregs;	
	u8 reg;
	
	/* log core options (read using indexed model) */
	reg = musb_read_configdata(mbase);	
	if (reg & MUSB_CONFIGDATA_DYNFIFO) {
			if (musb->config->dyn_fifo)
				{
				printk(KERN_INFO "[MUSB]ep_restore_from_table.\n");
				ep_restore_from_table(musb);
				}
		}
	
	return 0;
}
static int musb_suspend(struct platform_device *pdev, pm_message_t message)
{
	unsigned long	flags;
	struct musb	*musb = dev_to_musb(&pdev->dev);
	
	printk(KERN_INFO "[MUSB]musb_suspend.\n");
	if (!musb->clock)
		return 0;

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

	if (musb->set_clock)
		musb->set_clock(musb->clock, 0);
	else
		clk_disable(musb->clock);
	spin_unlock_irqrestore(&musb->lock, flags);
	return 0;
}

static int musb_resume_early(struct platform_device *pdev)
{
	struct musb	*musb = dev_to_musb(&pdev->dev);
	
	printk(KERN_INFO "[MUSB]musb_resume_early.\n");

	musb_platform_init(musb); //musb reset phy
	musb_endpoint_fifo_restore(musb); //musb restore fifo&endpoint
	if (!is_otg_enabled(musb))
	{		
		musb_start(musb); //musb enable interrupts
	}

	if (!musb->clock)
		return 0;

	if (musb->set_clock)
		musb->set_clock(musb->clock, 1);
	else
		clk_enable(musb->clock);

	/* for static cmos like DaVinci, register values were preserved
	 * unless for some reason the whole soc powered down or the USB
	 * module got reset through the PSC (vs just being disabled).
	 */
	return 0;
}

#else
#define	musb_suspend	NULL
#define	musb_resume_early	NULL
#endif

static struct platform_driver musb_driver = {
	.driver = {
		.name		= (char *)musb_driver_name,
		.bus		= &platform_bus_type,
		.owner		= THIS_MODULE,
	},
	.remove		= musb_remove,
	.shutdown	= musb_shutdown,
	.suspend	= musb_suspend,
#if !defined(CONFIG_ARCH_MT85XX)
	.resume	= musb_resume_early,
#endif
};



/*-------------------------------------------------------------------------*/

extern unsigned int ssusb_adb_flag;
extern unsigned int ssusb_adb_port;

static int __init musb_init(void)
{
	  uint32_t retval = 0;
	  //uint32_t nCount;
	  uint32_t adb_port;
	  struct platform_device *pPlatformDev;

#ifdef CONFIG_OF
	struct device_node *np0;
	np0=of_find_compatible_node(NULL,NULL,"Mediatek, TVUSB20");
	pUSBaphyIoMap=of_iomap(np0,0);
	pUSBamacIoMap=of_iomap(np0,1);
	pUSBadmaIoMap=of_iomap(np0,2);
#else
	pUSBaphyIoMap=ioremap(MUSB_PHY_BASE,0x400);
	pUSBamacIoMap=ioremap(MUSB_MAC_BASE,0x4000);
	pUSBadmaIoMap=ioremap(MUSB_DMA_SET,0x10);
#endif

//#ifdef MUSB_GADGET_PORT_NUM
#if 1
  	if(ssusb_adb_flag==0 || ssusb_adb_port==2 )
#endif        
	{
		printk("[MUSB]USB ADB disable Or ADB port[%d] is U3 \n",ssusb_adb_port); 		
		return 0;
	}

    
#ifdef CONFIG_USB_MUSB_HDRC_HCD
	if (usb_disabled())
		return 0;
#endif
	pr_info("%s: version " MUSB_VERSION ", "
#ifdef CONFIG_MUSB_PIO_ONLY
		"pio"
#elif defined(CONFIG_USB_TI_CPPI_DMA)
		"cppi-dma"
#elif defined(CONFIG_USB_INVENTRA_DMA)
		"musb-dma"
#elif defined(CONFIG_USB_TUSB_OMAP_DMA)
		"tusb-omap-dma"
#else
		"?dma?"
#endif
		", "
#ifdef CONFIG_USB_MUSB_OTG
		"otg (peripheral+host)"
#elif defined(CONFIG_USB_GADGET_MUSB_HDRC)
		"peripheral"
#elif defined(CONFIG_USB_MUSB_HDRC_HCD)
		"host"
#endif
		", debug=%d\n",
		musb_driver_name, musb_debug);
//#ifdef MUSB_GADGET_PORT_NUM
#if 1
    adb_port = ssusb_adb_port;
#else
    adb_port = 0;
#endif

	pPlatformDev = &usb_dev[adb_port];
	switch(adb_port){
		case 0:
			pPlatformDev->resource->start=(resource_size_t)MUSB_BASE;
			pPlatformDev->resource->end= (resource_size_t)(MUSB_BASE + MUSB_DEFAULT_ADDRESS_SPACE_SIZE -1);
			break;
		case 1:
			pPlatformDev->resource->start=(resource_size_t)MUSB_BASE1;
			pPlatformDev->resource->end= (resource_size_t)(MUSB_BASE1 + MUSB_DEFAULT_ADDRESS_SPACE_SIZE -1);
			break;
		case 2:
			pPlatformDev->resource->start=(resource_size_t)MUSB_BASE2;
			pPlatformDev->resource->end= (resource_size_t)(MUSB_BASE2 + MUSB_DEFAULT_ADDRESS_SPACE_SIZE -1);
			break;
		case 3:
			pPlatformDev->resource->start=(resource_size_t)MUSB_BASE3;
			pPlatformDev->resource->end= (resource_size_t)(MUSB_BASE3 + MUSB_DEFAULT_ADDRESS_SPACE_SIZE -1);
			break;
		default :
			break;
		}

	retval = platform_device_register(pPlatformDev);
	if (retval < 0)
	{
		platform_device_unregister(pPlatformDev);
		return -ENODEV;
	}

	return platform_driver_probe(&musb_driver, musb_probe);

}
/* make us init after usbcore and i2c (transceivers, regulators, etc)
 * and before usb gadget and host-side drivers start to register
 */
static void __exit musb_cleanup(void)
{
	uint32_t adb_port;
	struct platform_device *pPlatformDev;
#if 1
	if(ssusb_adb_flag==0 || ssusb_adb_port==2 )
#endif
	{
		return;
	}

	adb_port = ssusb_adb_port;

	pPlatformDev = &usb_dev[adb_port];
	platform_device_unregister(pPlatformDev);
	platform_driver_unregister(&musb_driver);
}



#if !defined(CONFIG_ARCH_MT85XX)
module_init(musb_init);
#else
fs_initcall(musb_init);
#endif
module_exit(musb_cleanup);
