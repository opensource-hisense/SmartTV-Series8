#ifndef _XHCI_MTK_POWER_H
#define _XHCI_MTK_POWER_H

#include <linux/usb.h>
#include "xhci.h"
#include "xhci-mtk.h"
#define VBUS_ON 1
#define VBUS_OFF 0

void enableXhciAllPortPower(struct xhci_hcd *xhci);
void enableAllClockPower(void);
void disablePortClockPower(void);
void enablePortClockPower(int port_index, int port_rev);
//void SSUSB_VbuscheckGPIO(void);
void SSUSB_Vbushandler(int fgMode);

#endif
