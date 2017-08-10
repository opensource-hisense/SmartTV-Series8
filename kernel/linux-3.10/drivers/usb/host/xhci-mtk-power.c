#include "xhci-mtk.h"
#include "xhci-mtk-power.h"
#include "xhci.h"
#include <linux/kernel.h>       /* printk() */
#include <linux/slab.h>
#include <linux/delay.h>
#ifdef CONFIG_64BIT
#include <linux/soc/mediatek/mtk_gpio.h>
#include <linux/soc/mediatek/x_typedef.h>
#else
#include <mach/mtk_gpio.h>
#include <x_typedef.h>
#endif
static int g_num_u3_port;
static int g_num_u2_port;
//static int u4SSUSB_VbusGpio=-1;    
//static int u4SSUSB_VbusGpioPolarity=-1;   

void enableXhciAllPortPower(struct xhci_hcd *xhci){
	int i;
	u32 port_id, temp;
	u32 __iomem *addr;

	g_num_u3_port = SSUSB_U3_PORT_NUM(readl((UPTR*)(ulong)SSUSB_IP_CAP));
	g_num_u2_port = SSUSB_U2_PORT_NUM(readl((UPTR*)(ulong)SSUSB_IP_CAP));
	
	for(i=1; i<=g_num_u3_port; i++){
		port_id=i;
		addr = &xhci->op_regs->port_status_base + NUM_PORT_REGS*((port_id-1) & 0xff);
		temp = xhci_readl(xhci, addr);
		temp = xhci_port_state_to_neutral(temp);
		temp |= PORT_POWER;
		xhci_writel(xhci, temp, addr);
	}
	for(i=1; i<=g_num_u2_port; i++){
		port_id=i+g_num_u3_port;
		addr = &xhci->op_regs->port_status_base + NUM_PORT_REGS*((port_id-1) & 0xff);
		temp = xhci_readl(xhci, addr);
		temp = xhci_port_state_to_neutral(temp);
		temp |= PORT_POWER;
		xhci_writel(xhci, temp, addr);
	}
}

void enableAllClockPower(void){

	int i;
	u32 temp, temp_mask;
	u32 u4TimeoutCount = 100;
	
	g_num_u3_port = SSUSB_U3_PORT_NUM(readl(SSUSB_IP_CAP));
	g_num_u2_port = SSUSB_U2_PORT_NUM(readl(SSUSB_IP_CAP));
	
	//2.	Enable xHC
	writel(readl(SSUSB_IP_PW_CTRL) | (SSUSB_IP_SW_RST), SSUSB_IP_PW_CTRL);
	writel(readl(SSUSB_IP_PW_CTRL) & (~SSUSB_IP_SW_RST), SSUSB_IP_PW_CTRL);	
	writel(readl(SSUSB_IP_PW_CTRL_1) & (~SSUSB_IP_PDN), SSUSB_IP_PW_CTRL_1);
	
	//1.	Enable target ports 
	for(i=0; i<g_num_u3_port; i++){
		temp = readl((UPTR*)(ulong)SSUSB_U3_CTRL(i));
		temp = temp & (~SSUSB_U3_PORT_PDN) & (~SSUSB_U3_PORT_DIS);
		writel(temp, (UPTR*)(ulong)SSUSB_U3_CTRL(i));
	}
	for(i=0; i<g_num_u2_port; i++){
		temp = readl((UPTR*)(ulong)SSUSB_U2_CTRL(i));
		temp = temp & (~SSUSB_U2_PORT_PDN) & (~SSUSB_U2_PORT_DIS);
		writel(temp, (UPTR*)(ulong)SSUSB_U2_CTRL(i));
	}

	temp_mask = (SSUSB_SYSPLL_STABLE | SSUSB_XHCI_RST_B_STS | SSUSB_U3_MAC_RST_B_STS);

	do{
		temp = readl((UPTR*)(ulong)SSUSB_IP_PW_STS1);		
		if ((temp & temp_mask) == temp_mask) 
		{
			break;
		}
        mdelay(1);
	}while(u4TimeoutCount--);
//	msleep(100);
}


//(X)disable clock/power of a port 
//(X)if all ports are disabled, disable IP ctrl power
//disable all ports and IP clock/power, this is just mention HW that the power/clock of port 
//and IP could be disable if suspended.
//If doesn't not disable all ports at first, the IP clock/power will never be disabled
//(some U2 and U3 ports are binded to the same connection, that is, they will never enter suspend at the same time
//port_index: port number
//port_rev: 0x2 - USB2.0, 0x3 - USB3.0 (SuperSpeed)
void disablePortClockPower(void){
	int i;
	u32 temp;

	g_num_u3_port = SSUSB_U3_PORT_NUM(readl((UPTR*)(ulong)SSUSB_IP_CAP));
	g_num_u2_port = SSUSB_U2_PORT_NUM(readl((UPTR*)(ulong)SSUSB_IP_CAP));
	
	for(i=0; i<g_num_u3_port; i++){
		temp = readl((UPTR*)(ulong)(SSUSB_U3_CTRL(i)));
		temp = temp | (SSUSB_U3_PORT_PDN);
		writel(temp, (UPTR*)(ulong)SSUSB_U3_CTRL(i));
	}
	for(i=0; i<g_num_u2_port; i++){
		temp = readl((UPTR*)(ulong)SSUSB_U2_CTRL(i));
		temp = temp | (SSUSB_U2_PORT_PDN);
		writel(temp, (UPTR*)(ulong)SSUSB_U2_CTRL(i));
	}
	writel(readl((UPTR*)SSUSB_IP_PW_CTRL_1) | (SSUSB_IP_PDN), (UPTR*)(ulong)SSUSB_IP_PW_CTRL_1);
}

//if IP ctrl power is disabled, enable it
//enable clock/power of a port
//port_index: port number
//port_rev: 0x2 - USB2.0, 0x3 - USB3.0 (SuperSpeed)
void enablePortClockPower(int port_index, int port_rev){
//	int i;
	u32 temp;
	
	writel(readl((UPTR*)SSUSB_IP_PW_CTRL_1) & (~SSUSB_IP_PDN), (UPTR*)(ulong)SSUSB_IP_PW_CTRL_1);

	if(port_rev == 0x3){
		temp = readl((UPTR*)(ulong)SSUSB_U3_CTRL(port_index));
		temp = temp & (~SSUSB_U3_PORT_PDN);
		writel(temp, (UPTR*)(ulong)SSUSB_U3_CTRL(port_index));
	}
	else if(port_rev == 0x2){
		temp = readl((UPTR*)(ulong)SSUSB_U2_CTRL(port_index));
		temp = temp & (~SSUSB_U2_PORT_PDN);
		writel(temp, (UPTR*)(ulong)SSUSB_U2_CTRL(port_index));
	}
}

/*void SSUSB_VbuscheckGPIO(void)
{
	int temp;

    temp = readl(SIFSLV_IPPC+0x0B4);
    if (temp!=0xffffffff)
    {
        u4SSUSB_VbusGpio = temp&0xffff;    
        u4SSUSB_VbusGpioPolarity = (temp & 0xffff0000)>>16;   
    }

}
*/
#define SSUSB_PORTMUN 2
#define MUC_NUM_MAX_CONTROLLER      (4)
extern int MUC_aPwrGpio[MUC_NUM_MAX_CONTROLLER];
extern int MUC_aPwrPolarity[MUC_NUM_MAX_CONTROLLER];
extern int MUC_aOCGpio[MUC_NUM_MAX_CONTROLLER];
extern int MUC_aOCPolarity[MUC_NUM_MAX_CONTROLLER];
void SSUSB_Vbushandler(int fgMode)
{
//    int u4temp;

    if(MUC_aPwrGpio[SSUSB_PORTMUN] != -1)
    {
        mtk_gpio_set_value(MUC_aPwrGpio[SSUSB_PORTMUN], 
                ((fgMode) ? (MUC_aPwrPolarity[SSUSB_PORTMUN]) : (1-MUC_aPwrPolarity[SSUSB_PORTMUN])));
        printk("SSUSB: Set GPIO%d = %d.\n", MUC_aPwrGpio[SSUSB_PORTMUN], 
            ((fgMode) ? (MUC_aPwrPolarity[SSUSB_PORTMUN]) : (1-MUC_aPwrPolarity[SSUSB_PORTMUN])));
    }

/*    if (u4SSUSB_VbusGpio != -1)
    {
        if(fgMode==VBUS_OFF)
            u4temp = !u4SSUSB_VbusGpioPolarity;
        else
            u4temp = u4SSUSB_VbusGpioPolarity;
        printk("SSUSB: Set GPIO%d = %d.\n", u4SSUSB_VbusGpio, u4temp);
        mtk_gpio_set_value(u4SSUSB_VbusGpio, u4temp);
    }
*/
}

