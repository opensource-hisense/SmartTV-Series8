/*-----------------------------------------------------------------------------
 * drivers/usb/musb_qmu/musb_dev_id.h
 *
 * MT53xx USB driver
 *
 * Copyright (c) 2008-2012 MediaTek Inc.
 * $Author: dtvbm11 $
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 *
 *---------------------------------------------------------------------------*/



#include <linux/usb.h>

#if defined(CONFIG_USB_MUSB_HDRC_HCD)
static struct usb_device_id wifi_id_table[] =
{
//athores
    { USB_DEVICE(0x0CF3, 0x7010)},
    { USB_DEVICE(0x0CF3, 0x7011)},
    { USB_DEVICE(0x0CF3, 0x9271)},   //for K2
    { USB_DEVICE(0x045E, 0x02a7)},   //MSFT - Omni 
    { USB_DEVICE(0x0471, 0x209E)},   //TCL K2
    { USB_DEVICE(0x04CA, 0x4605)},   //LiteOn K2
    { USB_DEVICE(0x0411, 0x017F)},   //Sony Magpie
    
//realtek
    {USB_DEVICE(0x0bda, 0x8171)},
    {USB_DEVICE(0x0bda, 0x8172)},
    {USB_DEVICE(0x0bda, 0x8173)},
    {USB_DEVICE(0x0bda, 0x8174)},
    {USB_DEVICE(0x0bda, 0x8712)},
    {USB_DEVICE(0x0bda, 0x8713)},
    {USB_DEVICE(0x0bda, 0xC512)},
    // Abocom  */
    {USB_DEVICE(0x07B8, 0x8188)},
    {USB_DEVICE(0x07B8, 0x8189)},
    // Corega */
    {USB_DEVICE(0x07aa, 0x0047)},
    // Dlink */
    {USB_DEVICE(0x07d1, 0x3303)},
    {USB_DEVICE(0x07d1, 0x3302)},
    {USB_DEVICE(0x07d1, 0x3300)},
    // Dlink for Skyworth */
    {USB_DEVICE(0x14b2, 0x3300)},
    {USB_DEVICE(0x14b2, 0x3301)},
    {USB_DEVICE(0x14b2, 0x3302)},
    // EnGenius */
    {USB_DEVICE(0x1740, 0x9603)},
    {USB_DEVICE(0x1740, 0x9605)},
    // Belkin */
    {USB_DEVICE(0x050d, 0x815F)},
    {USB_DEVICE(0x050d, 0x945A)},
    {USB_DEVICE(0x050d, 0x845A)},
    // Guillemot */
    {USB_DEVICE(0x06f8, 0xe031)},
    // Edimax */
    {USB_DEVICE(0x7392, 0x7611)},
    {USB_DEVICE(0x7392, 0x7612)},
    {USB_DEVICE(0x7392, 0x7622)},
    // Sitecom */	
    {USB_DEVICE(0x0DF6, 0x0045)},
    // Hawking */
    {USB_DEVICE(0x0E66, 0x0015)},
    {USB_DEVICE(0x0E66, 0x0016)},
    {USB_DEVICE(0x0b05, 0x1786)},
    {USB_DEVICE(0x0b05, 0x1791)},	 // 11n mode disable
    //*/
    {USB_DEVICE(0x13D3, 0x3306)},
    {USB_DEVICE(0x13D3, 0x3309)},
    {USB_DEVICE(0x13D3, 0x3310)},
    {USB_DEVICE(0x13D3, 0x3311)},	 // 11n mode disable
    {USB_DEVICE(0x13D3, 0x3325)},
    {USB_DEVICE(0x083A, 0xC512)},

//RT377X
    {USB_DEVICE(0x148F,0x3070)}, /* Ralink 3070 */
	{USB_DEVICE(0x148F,0x3071)}, /* Ralink 3071 */
	{USB_DEVICE(0x148F,0x3072)}, /* Ralink 3072 */
	{USB_DEVICE(0x0DB0,0x3820)}, /* Ralink 3070 */
	{USB_DEVICE(0x0DB0,0x871C)}, /* Ralink 3070 */
	{USB_DEVICE(0x0DB0,0x822C)}, /* Ralink 3070 */
	{USB_DEVICE(0x0DB0,0x871B)}, /* Ralink 3070 */
	{USB_DEVICE(0x0DB0,0x822B)}, /* Ralink 3070 */
	{USB_DEVICE(0x0DF6,0x003E)}, /* Sitecom 3070 */
	{USB_DEVICE(0x0DF6,0x0042)}, /* Sitecom 3072 */
	{USB_DEVICE(0x0DF6,0x0048)}, /* Sitecom 3070 */
	{USB_DEVICE(0x0DF6,0x0047)}, /* Sitecom 3071 */
	{USB_DEVICE(0x14B2,0x3C12)}, /* AL 3070 */
	{USB_DEVICE(0x18C5,0x0012)}, /* Corega 3070 */
	{USB_DEVICE(0x083A,0x7511)}, /* Arcadyan 3070 */
	{USB_DEVICE(0x083A,0xA701)}, /* SMC 3070 */
	{USB_DEVICE(0x083A,0xA702)}, /* SMC 3072 */
	{USB_DEVICE(0x1740,0x9703)}, /* EnGenius 3070 */
	{USB_DEVICE(0x1740,0x9705)}, /* EnGenius 3071 */
	{USB_DEVICE(0x1740,0x9706)}, /* EnGenius 3072 */
	{USB_DEVICE(0x1740,0x9707)}, /* EnGenius 3070 */
	{USB_DEVICE(0x1740,0x9708)}, /* EnGenius 3071 */
	{USB_DEVICE(0x1740,0x9709)}, /* EnGenius 3072 */
	{USB_DEVICE(0x13D3,0x3273)}, /* AzureWave 3070*/
	{USB_DEVICE(0x13D3,0x3305)}, /* AzureWave 3070*/
	{USB_DEVICE(0x1044,0x800D)}, /* Gigabyte GN-WB32L 3070 */
	{USB_DEVICE(0x2019,0xAB25)}, /* Planex Communications, Inc. RT3070 */
	{USB_DEVICE(0x07B8,0x3070)}, /* AboCom 3070 */
	{USB_DEVICE(0x07B8,0x3071)}, /* AboCom 3071 */
	{USB_DEVICE(0x07B8,0x3072)}, /* Abocom 3072 */
	{USB_DEVICE(0x7392,0x7711)}, /* Edimax 3070 */
	{USB_DEVICE(0x1A32,0x0304)}, /* Quanta 3070 */
	{USB_DEVICE(0x1EDA,0x2310)}, /* AirTies 3070 */
	{USB_DEVICE(0x07D1,0x3C0A)}, /* D-Link 3072 */
	{USB_DEVICE(0x07D1,0x3C0D)}, /* D-Link 3070 */
	{USB_DEVICE(0x07D1,0x3C0E)}, /* D-Link 3070 */
	{USB_DEVICE(0x07D1,0x3C0F)}, /* D-Link 3070 */
	{USB_DEVICE(0x07D1,0x3C16)}, /* D-Link 3070 */
	{USB_DEVICE(0x1D4D,0x000C)}, /* Pegatron Corporation 3070 */
	{USB_DEVICE(0x1D4D,0x000E)}, /* Pegatron Corporation 3070 */
	{USB_DEVICE(0x5A57,0x5257)}, /* Zinwell 3070 */
	{USB_DEVICE(0x5A57,0x0283)}, /* Zinwell 3072 */
	{USB_DEVICE(0x04BB,0x0945)}, /* I-O DATA 3072 */
	{USB_DEVICE(0x04BB,0x0947)}, /* I-O DATA 3070 */
	{USB_DEVICE(0x04BB,0x0948)}, /* I-O DATA 3072 */
	{USB_DEVICE(0x203D,0x1480)}, /* Encore 3070 */
	{USB_DEVICE(0x20B8,0x8888)}, /* PARA INDUSTRIAL 3070 */
	{USB_DEVICE(0x0B05,0x1784)}, /* Asus 3072 */
	{USB_DEVICE(0x203D,0x14A9)}, /* Encore 3070*/
	{USB_DEVICE(0x0DB0,0x899A)}, /* MSI 3070*/
	{USB_DEVICE(0x0DB0,0x3870)}, /* MSI 3070*/
	{USB_DEVICE(0x0DB0,0x870A)}, /* MSI 3070*/
	{USB_DEVICE(0x0DB0,0x6899)}, /* MSI 3070 */
	{USB_DEVICE(0x0DB0,0x3822)}, /* MSI 3070 */
	{USB_DEVICE(0x0DB0,0x3871)}, /* MSI 3070 */
	{USB_DEVICE(0x0DB0,0x871A)}, /* MSI 3070 */
	{USB_DEVICE(0x0DB0,0x822A)}, /* MSI 3070 */
	{USB_DEVICE(0x0DB0,0x3821)}, /* Ralink 3070 */
	{USB_DEVICE(0x0DB0,0x821A)}, /* Ralink 3070 */
	{USB_DEVICE(0x5A57,0x0282)}, /* zintech 3072 */	
	{USB_DEVICE(0x083A,0xA703)}, /* IO-MAGIC */
	{USB_DEVICE(0x13D3,0x3307)}, /* Azurewave */
	{USB_DEVICE(0x13D3,0x3321)}, /* Azurewave */
	{USB_DEVICE(0x07FA,0x7712)}, /* Edimax */
	{USB_DEVICE(0x0789,0x0166)}, /* Edimax */
	{USB_DEVICE(0x148F,0x2870)}, /* Ralink */
	{USB_DEVICE(0x148F,0x3370)}, /* Ralink 3370 */
	{USB_DEVICE(0x0489,0x0006)}, /* Foxconn */
	{USB_DEVICE(0x0471,0x20DD)}, /* Philips-TCL */
    {USB_DEVICE(0x148F,0x5370)}, 

};
#endif
