/*
 * (C) Copyright 2000-2010
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * (C) Copyright 2001 Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Andreas Heppel <aheppel@sysgo.de>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <command.h>
#include <environment.h>
#include <linux/stddef.h>
#include <search.h>
#include <errno.h>
#include <malloc.h>
#include "x_bim.h"
#include <version.h>

#define CC_UBOOT_TRANSFER_USBGPIO_KERNEL
#ifdef CC_S_PLATFORM
#define CC_UBOOT_TRANSFER_UART_MODE
#endif
#define CC_UBOOT_TRANSFER_MSDCGPIO_KERNEL
#include "drvcust_if.h"

DECLARE_GLOBAL_DATA_PTR;

/************************************************************************
 * Default settings to be used when no valid environment is found
 */
#define XMK_STR(x)	#x
#define MK_STR(x)	XMK_STR(x)

const uchar default_environment[] = {
#ifdef	CONFIG_BOOTARGS
	"bootargs="	CONFIG_BOOTARGS			"\0"
#endif
#ifdef	CONFIG_EMMCLOG
	"emmclog="	CONFIG_EMMCLOG			"\0"
#endif
#ifdef	CONFIG_BOOTCOMMAND
	"bootcmd="	CONFIG_BOOTCOMMAND		"\0"
#endif
#ifdef	CONFIG_RAMBOOTCOMMAND
	"ramboot="	CONFIG_RAMBOOTCOMMAND		"\0"
#endif
#ifdef	CONFIG_NFSBOOTCOMMAND
	"nfsboot="	CONFIG_NFSBOOTCOMMAND		"\0"
#endif
#if defined(CONFIG_BOOTDELAY) && (CONFIG_BOOTDELAY >= 0)
	"bootdelay="	MK_STR(CONFIG_BOOTDELAY)	"\0"
#endif
#if defined(CONFIG_BAUDRATE) && (CONFIG_BAUDRATE >= 0)
	"baudrate="	MK_STR(CONFIG_BAUDRATE)		"\0"
#endif
#ifdef	CONFIG_LOADS_ECHO
	"loads_echo="	MK_STR(CONFIG_LOADS_ECHO)	"\0"
#endif
#ifdef	CONFIG_ETHADDR
	"ethaddr="	MK_STR(CONFIG_ETHADDR)		"\0"
#endif
#ifdef	CONFIG_ETH1ADDR
	"eth1addr="	MK_STR(CONFIG_ETH1ADDR)		"\0"
#endif
#ifdef	CONFIG_ETH2ADDR
	"eth2addr="	MK_STR(CONFIG_ETH2ADDR)		"\0"
#endif
#ifdef	CONFIG_ETH3ADDR
	"eth3addr="	MK_STR(CONFIG_ETH3ADDR)		"\0"
#endif
#ifdef	CONFIG_ETH4ADDR
	"eth4addr="	MK_STR(CONFIG_ETH4ADDR)		"\0"
#endif
#ifdef	CONFIG_ETH5ADDR
	"eth5addr="	MK_STR(CONFIG_ETH5ADDR)		"\0"
#endif
#ifdef	CONFIG_IPADDR
	"ipaddr="	MK_STR(CONFIG_IPADDR)		"\0"
#endif
#ifdef	CONFIG_SERVERIP
	"serverip="	MK_STR(CONFIG_SERVERIP)		"\0"
#endif
#ifdef	CONFIG_SYS_AUTOLOAD
	"autoload="	CONFIG_SYS_AUTOLOAD		"\0"
#endif
#ifdef	CONFIG_PREBOOT
	"preboot="	CONFIG_PREBOOT			"\0"
#endif
#ifdef	CONFIG_ROOTPATH
	"rootpath="	CONFIG_ROOTPATH			"\0"
#endif
#ifdef	CONFIG_GATEWAYIP
	"gatewayip="	MK_STR(CONFIG_GATEWAYIP)	"\0"
#endif
#ifdef	CONFIG_NETMASK
	"netmask="	MK_STR(CONFIG_NETMASK)		"\0"
#endif
#ifdef	CONFIG_HOSTNAME
	"hostname="	MK_STR(CONFIG_HOSTNAME)		"\0"
#endif
#ifdef	CONFIG_BOOTFILE
	"bootfile="	CONFIG_BOOTFILE			"\0"
#endif
#ifdef	CONFIG_LOADADDR
	"loadaddr="	MK_STR(CONFIG_LOADADDR)		"\0"
#endif
#ifdef	CONFIG_CLOCKS_IN_MHZ
	"clocks_in_mhz=1\0"
#endif
#if defined(CONFIG_PCI_BOOTDELAY) && (CONFIG_PCI_BOOTDELAY > 0)
	"pcidelay="	MK_STR(CONFIG_PCI_BOOTDELAY)	"\0"
#endif
#ifdef	CONFIG_EXTRA_ENV_SETTINGS
	CONFIG_EXTRA_ENV_SETTINGS
#endif
	"\0"
};
extern char mtdparts_default[];

struct hsearch_data env_htab;
//int default_environment_size = sizeof(default_environment);
int default_environment_size = 0x2000;
static unsigned int kmemsize = 0;
static unsigned int kmem2start = 0;
static unsigned int kmem2size = 0;
static unsigned int	kmem3start = 0;
static unsigned int	kmem3size = 0;

#if defined(CC_DYNAMIC_FBMSRM_CONF)
static unsigned int kmem4ksize = 0;
static unsigned int kmemfhdsize = 0;
static unsigned int kmem24ksize = 0;
static unsigned int kmem2fhdsize = 0;
static unsigned int gpu4kstart = 0;
static unsigned int gpufhdstart = 0;
static unsigned int gpu4ksize = 0;
static unsigned int gpufhdsize = 0;
static unsigned int gpu4kionsize = 0;
static unsigned int gpufhdionsize = 0;
#endif
static uchar env_get_char_init(int index)
{
	/* if crc was bad, use the default environment */
	if (gd->env_valid)
		return env_get_char_spec(index);
	else
		return default_environment[index];
}

uchar env_get_char_memory(int index)
{
	return *env_get_addr(index);
}

uchar env_get_char(int index)
{
	/* if relocated to RAM */
	if (gd->flags & GD_FLG_RELOC)
		return env_get_char_memory(index);
	else
		return env_get_char_init(index);
}

const uchar *env_get_addr(int index)
{
	if (gd->env_valid)
		return (uchar *)(gd->env_addr + index);
	else
		return &default_environment[index];
}

#if defined(CC_FAST_INIT) || defined(CC_ENABLE_PRODUCT_ID) || defined(CC_MT53XX_SUPPORT_2G_DRAM) || defined(CC_ENABLE_MALI_CORE_NUM) || defined(CC_UBOOT_TRANSFER_USBGPIO_KERNEL) || defined(LPJ_IC_VER1) || defined(LPJ_IC_VER2) || defined(CC_UBOOT_ADD_USB_LOG_MODE)
static void env_add_bootargs(char* oldstr, char* newstr)
{
    char *p1 = NULL, *p2 = NULL;
    char *bootargs = NULL;

    // generate bootargs string
    p1 = getenv("bootargs");

    if (p1 == NULL)
    {
        return;
    }

    bootargs = (char *)malloc(default_environment_size);
    memset(bootargs, 0, default_environment_size);
    strcpy(bootargs, p1);

    // delete any existing string in bootargs string.
    p1 = oldstr ? strstr(bootargs, oldstr) : NULL;
    if (p1)
    {
        p2 = strstr(p1, " ");
        if(p2)
        {
            while(*++p2)
            {
                *p1++ = *p2;
            }

            *p1++ = ' ';
        }

        *p1 = '\0';
    }
    else
    {
        strcat(bootargs, " ");
    }

    // append new string at the end of bootargs string.
    strcat(bootargs, newstr);

    // update bootargs environment
    setenv("bootargs", bootargs);

    free(bootargs);
}
#endif
#if defined(CC_UBOOT_ADD_USB_LOG_MODE)
#ifdef CC_S_PLATFORM
static void env_add_usb_log(void)
{
    char usbSerialConsole[] = "console=ttyMT0,115200n1 console=ttyUSB0,115200n1";
    char uartSerialConsole[] = "console=ttyMT0,115200n1";
    unsigned int u4IsUsbLog=0;
    if((0==DRVCUST_InitQuery(eFlagIsUsbLogMode, &u4IsUsbLog)) && u4IsUsbLog)
        {
        env_add_bootargs("console=", usbSerialConsole);
        }
    else
    {
        env_add_bootargs("console=", uartSerialConsole);
    }
}

#else
extern int EEPROM_Read(uint64_t u8Offset, int u4MemPtr, int u4MemLen);

static void env_add_usb_log(void)
{
    char u1FactMode = 0;
    char usbserialconsole[] = "console=ttyUSB0,115200n1";

    if (0 == EEPROM_Read(CC_UBOOT_ADD_USB_LOG_MODE_ADDR, (int)(void*)&u1FactMode, sizeof(char)))
    {
        if (u1FactMode == 1)
        {
            env_add_bootargs(NULL, usbserialconsole);
        }
    }
}
#endif
#endif

#ifdef APP_B2B_SUPPORT
static void env_add_pq_mode(void)
{
    char u1PQMode = 0;
    if(0 == EEPROM_Read((UINT64)0x8e1, (int)(void*)&u1PQMode, sizeof(char)))
    {
        if (u1PQMode == 1)
        {
            env_add_bootargs("console=", "console=ttyMT0,115200n1");
        }
        else
        {
            env_add_bootargs("console=", "console=ttyMT1,115200n1");
        }
    }
}
#endif

#ifdef CC_ENABLE_MTK_MODEL_INDEX
extern unsigned int GetCurrentPanelIndex(void);
#endif
static void env_add_panel_index(void)
{
    char newstr[32] = {0};

#ifdef CC_ENABLE_MTK_MODEL_INDEX
    sprintf(newstr, "modelindex_id=%d", GetCurrentPanelIndex());
#else
    sprintf(newstr, "modelindex_id=%d", GetModelIndexFromEEPROM());
#endif // CC_ENABLE_MTK_MODEL_INDEX
    env_add_bootargs("modelindex_id=", newstr);
}


#ifdef CC_ENABLE_MALI_CORE_NUM
extern unsigned int Efuse_MaliMaliCoreNum(void);
static void env_add_mali_core_num(void)
{
    char newstr[32] = {0};

    sprintf(newstr, "mali_core_num=%d", Efuse_MaliMaliCoreNum());
    env_add_bootargs("mali_core_num=", newstr);
}
#endif
#ifdef CC_SUPPORT_CHIPID

static void env_add_chipid(void)
{
    char newstr[64] = {0};
    sprintf(newstr, "androidboot.hardware=mt%x", (*((volatile u32*)(0xf000d064)))>>16);
    env_add_bootargs("androidboot.hardware=", newstr);
}
#endif
#ifdef CC_S_PLATFORM
#ifdef CC_UBOOT_TRANSFER_UART_MODE
static void env_add_uart_mode(void)
{
	int val=0;
	char str[32] = {0};
	EEPROM_Read((UINT64)0x6c8, (UINT32)&val, (UINT32)sizeof(char));//val = 3 VDA 4 Locatel
	sprintf(str, "uart_mode=%d",val);
	env_add_bootargs("uart_mode=", str);
	
}
#endif
#endif

#ifdef CC_KERNEL_NO_LOG
static void env_add_forbid_uart(void)
{
    char newstr[16] = {0};
    sprintf(newstr, "forbid_uart0=%d", 1);
    env_add_bootargs("forbid_uart0=", newstr);
}
#endif

static void env_add_perfor_uart(void)
{
    char newstr[16] = {0};
    sprintf(newstr, "perfor_uart0=%d", 1);
    env_add_bootargs("perfor_uart0=", newstr);
}

static void env_del_perfor_uart(void)
{
    char newstr[16] = {0};
    sprintf(newstr, "perfor_uart0=%d", 0);
    env_add_bootargs("perfor_uart0=", newstr);
}


#ifdef CC_MT5890
static void env_add_have_vp9(void)
{
	char newstr[16] = {0};

    UINT32 IcVersion=BSP_GetIcVersion();
	
	if(IcVersion >= 0x58900003)
	{
		sprintf(newstr, "have_vp9=%d", 1);
		env_add_bootargs("have_vp9=", newstr);
	}
	else
	{
		sprintf(newstr, "have_vp9=%d", 0);
		env_add_bootargs("have_vp9=", newstr);
	}
	
}

#endif

#ifdef CC_VP9_SUPPORT
static void env_add_vp9_support(void)
{
	char newstr[26] = {0};
	sprintf(newstr, "androidboot.vp9_support=%d", 1);
	env_add_bootargs("androidboot.vp9_support=", newstr);
}
#endif

static void env_add_gpu_addr(void)
{
	char newstr[16] = {0};
#if defined(CC_DYNAMIC_FBMSRM_CONF)
	if(FBMDynIs4KMode())
		sprintf(newstr, "gpustart=%s", getenv("gpu4kstart"));
	else	
		sprintf(newstr, "gpustart=%s", getenv("gpufhdstart"));
#else
	sprintf(newstr, "gpustart=%d", PHYS_GPU_START);	
#endif
	env_add_bootargs("gpustart=", newstr);

#if defined(CC_DYNAMIC_FBMSRM_CONF)
	if(FBMDynIs4KMode())
		sprintf(newstr, "gpusize=%s", getenv("gpu4ksize"));
	else	
		sprintf(newstr, "gpusize=%s", getenv("gpufhdsize"));
#else
	sprintf(newstr, "gpusize=%d", PHYS_GPU_SIZE);
#endif
	env_add_bootargs("gpusize=", newstr);

#if defined(CC_DYNAMIC_FBMSRM_CONF)
	if(FBMDynIs4KMode())
		sprintf(newstr, "gpuionsize=%s", getenv("gpu4kionsize"));
	else	
		sprintf(newstr, "gpuionsize=%s", getenv("gpufhdionsize"));
#else
	sprintf(newstr, "gpuionsize=%d", PHYS_GPU_ION_SIZE);
#endif
	env_add_bootargs("gpuionsize=", newstr);
}
#ifdef CC_S_PLATFORM
#ifdef CC_OX_CUST
extern unsigned int UBOOT_EEP_GetSerial(void);
#endif
#endif

static void env_add_usb_serialno(void)
{
    char newstr[50] = {0}; 
	#ifdef CC_S_PLATFORM
	#ifdef CC_OX_CUST
	char *serialstr;
	
	serialstr = UBOOT_EEP_GetSerial();
	//Printf("env_add_usb_serialno(%s)\n",serialstr);
    sprintf(newstr, "androidboot.serialno=%s", serialstr);
    env_add_bootargs("androidboot.serialno=", newstr);
  #else
	#define ETHERNET_BASE1	(0xF0032014)
	#define ETHERNET_BASE2	(0xF0032018)
    sprintf(newstr, "androidboot.serialno=%08x%08x", (*((volatile u32*)(ETHERNET_BASE1))),(*((volatile u32*)(ETHERNET_BASE2))));
    env_add_bootargs("androidboot.serialno=", newstr);
  #endif
	#else
	#define ETHERNET_BASE1	(0xF0032014)
	#define ETHERNET_BASE2	(0xF0032018)
    sprintf(newstr, "androidboot.serialno=%08x%08x", (*((volatile u32*)(ETHERNET_BASE1))),(*((volatile u32*)(ETHERNET_BASE2))));
    env_add_bootargs("androidboot.serialno=", newstr);
	#endif
}
#ifdef CC_3RD_KO_INSERTION
#ifdef CC_EMMC_BOOT
extern int emmc_read_from_partition(unsigned char* buf, char* pname, u64 offset, unsigned int size);
#endif
static void env_add_beic_support(void)
{
    char newstr[32] = {0};
	char ddri_header[4] = {0};

    #ifdef CC_EMMC_BOOT
	emmc_read_from_partition(ddri_header, "ddri", 0, sizeof(ddri_header));
    #endif
	if(strncmp(ddri_header, "DDRI",4)==0)
	{
	  //Printf("env_add_beic_support exist\n");
          sprintf(newstr, "beic_supported=%d", 1);
          env_add_bootargs("beic_supported=", newstr);
	}
}
#endif

#ifdef CC_S_PLATFORM
#ifdef CC_DYNAMIC_USB_ADB_CONFIG
extern unsigned int UBOOT_EEPDTV_IsADB(void);
static void env_add_adb_support(void)
{
    char newstr[16] = {0};
    unsigned int u4Support=0;

    u4Support = UBOOT_EEPDTV_IsADB();
	
    if(u4Support)
    {
        sprintf(newstr, "adb_enable=%d", 1);
        env_add_bootargs("adb_enable=", newstr);

        //sprintf(newstr, "adb_port=%d", 0);
        //env_add_bootargs("adb_port=", newstr);
    }
}
#endif
#ifdef CC_USB3_TO_ADB_PORT_CONFIG
extern unsigned int UBOOT_EEPDTV_IsU3ToADB(void);
static void env_add_u32adb_support(void)
{
    char newstr[16] = {0};
	unsigned int u4Support=0;
	char ddri_header[4] = {0};

    u4Support = UBOOT_EEPDTV_IsU3ToADB();
	
	if(u4Support)
	{
          sprintf(newstr, "u32adb=%d", 1);
          env_add_bootargs("u32adb=", newstr);
	}
	else
	{
          sprintf(newstr, "u32adb=%d", 0);
          env_add_bootargs("u32adb=", newstr);	  
	}
}
#endif
#else
#if 0
static void env_add_adb_support(void)
{
    char newstr[16] = {0};
    sprintf(newstr, "adb_enable=%d", 1);
    env_add_bootargs("adb_enable=", newstr);

    sprintf(newstr, "adb_port=%d", 0);
    env_add_bootargs("adb_port=", newstr);
}
#endif
#endif

#ifdef CC_UBOOT_TRANSFER_USBGPIO_KERNEL
#define MUSB_MAX_CONTROLLERS 4

static void env_add_usb_gpio(void)
{
	int i;
	int u4UsbPortConfig;
	char pstr[32];
	int _i4usbportusing[MUSB_MAX_CONTROLLERS] ={eUSB0Config,eUSB1Config,eUSB2Config,eUSB3Config};
	int _i4usbpwrgpio[MUSB_MAX_CONTROLLERS] ={eUSBVbus0Gpio,eUSBVbus1Gpio,eUSBVbus2Gpio,eUSBVbus3Gpio};
	int _i4usbpwrpolarity[MUSB_MAX_CONTROLLERS] ={eUSBVbus0EnablePolarity,eUSBVbus1EnablePolarity,eUSBVbus2EnablePolarity,eUSBVbus3EnablePolarity};
	int _i4usbocgpio[MUSB_MAX_CONTROLLERS] ={eUSB0OCGpio,eUSB1OCGpio,eUSB2OCGpio,eUSB3OCGpio};
	int _i4usbocgpiopolarity[MUSB_MAX_CONTROLLERS] ={eUSB0OCEnablePolarity,eUSB1OCEnablePolarity,eUSB2OCEnablePolarity,eUSB3OCEnablePolarity};
	
	char usbportusing[64] = "usbportusing=";
	char usbpwrgpio[64] = "usbpwrgpio=";
//	char usbpwrpolarity[32] = "usbpwrpolarity=";
	char usbocgpio[64] = "usbocgpio=";
//	char usbocgpiopolarity[32] = "usbocgpiopolarity=";
    char usbhubgpio[30] = "usbhubrstgpio=";
	
    for(i = 0; i < MUSB_MAX_CONTROLLERS; i++)
    	{
    		if(0 == DRVCUST_InitQuery(_i4usbportusing[i], &u4UsbPortConfig))
    			{
				sprintf(pstr,"%d",u4UsbPortConfig);
				strcat(usbportusing,pstr );
    		if( 0 ==DRVCUST_InitQuery(_i4usbpwrgpio[i], &u4UsbPortConfig))
    			{
					sprintf(pstr,"%d",u4UsbPortConfig);
					strcat(usbpwrgpio,pstr );
    			}
			else
				{
					strcat(usbpwrgpio,"-1");
				}
    			}
			else
				{
					strcat(usbpwrgpio,"-1");
					strcat(usbportusing,"-1");
				}

			if( 0==DRVCUST_InitQuery(_i4usbpwrpolarity[i], &u4UsbPortConfig))
				{
					sprintf(pstr,"%d",u4UsbPortConfig);
					strcat(usbpwrgpio,":");
					strcat(usbpwrgpio,pstr);
				}
			else
				{
					strcat(usbpwrgpio,":");
					strcat(usbpwrgpio,"-1");
				}

			if ( 0==DRVCUST_InitQuery(_i4usbocgpio[i], &u4UsbPortConfig))
				{
					sprintf(pstr,"%d",u4UsbPortConfig);
					strcat(usbocgpio,pstr);
				}
			else
				{
					strcat(usbocgpio,"-1");
				}

			if(0 ==DRVCUST_InitQuery(_i4usbocgpiopolarity[i], &u4UsbPortConfig))
				{
					sprintf(pstr,"%d",u4UsbPortConfig);
					strcat(usbocgpio,":");
					strcat(usbocgpio,pstr);
				}
			
			else
				{
					strcat(usbocgpio,":");
					strcat(usbocgpio,"-1");
				}
			if(i !=MUSB_MAX_CONTROLLERS-1)
				{
				
					strcat(usbportusing,",");
					strcat(usbpwrgpio,",");
				//	strcat(usbpwrpolarity,",");
					strcat(usbocgpio,",");
				//	strcat(usbocgpiopolarity,",");
					
				}
			
    	}
     
        if(0 == DRVCUST_InitQuery(eUSBHubResetGpio, &u4UsbPortConfig))
        {
            sprintf(pstr,"%d",u4UsbPortConfig);
            strcat(usbhubgpio,pstr);			
	    }
		else
		{
		    strcat(usbhubgpio,"-1"); 
		}
		
		if(0 == DRVCUST_InitQuery(eUSBHubResetPolarity, &u4UsbPortConfig))
        {
            sprintf(pstr,":%d",u4UsbPortConfig);
            strcat(usbhubgpio,pstr);			
	    }
		else
		{
		    strcat(usbhubgpio,":-1"); 
		}
	//printf("%s,\n",	usbportusing);
	//printf("%s,\n",	usbpwrgpio);
	//printf("%s,\n",	usbpwrpolarity);
	//printf("%s,\n",	usbocgpio);
	//printf("%s,\n",	usbocgpiopolarity);

    env_add_bootargs("usbportusing=", usbportusing);
    env_add_bootargs("usbpwrgpio=", usbpwrgpio);
   // env_add_bootargs("usbpwrpolarity=", usbpwrpolarity);
    env_add_bootargs("usbocgpio=", usbocgpio);
  //  env_add_bootargs("usbocgpiopolarity=", usbocgpiopolarity); 
    /* for cdc support, default support usb port0 */
    //env_add_bootargs("usbcdcsupport=", "usbcdcsupport=1,0,0,0");
    env_add_bootargs("usbhubrstgpio=", usbhubgpio);
}
#endif

#ifdef CC_UBOOT_TRANSFER_MSDCGPIO_KERNEL
#define MSDC_GPIO_MAX_NUMBERS 6

static void env_add_msdc_gpio(void)
{
	int i;
	int u4MsdcGpioConfig;
	char pstr[32];
	int _i4MSDCgpio[MSDC_GPIO_MAX_NUMBERS]={eMSDC0DetectGpio,eMSDC0WriteProtectGpio,eMSDC0PoweronoffDetectGpio,eMSDC0VoltageSwitchGpio,eMSDCbackup1Gpio,eMSDCbackup2Gpio};
    
	char msdcgpio[32] = "msdcgpio=";

    for(i = 0; i < MSDC_GPIO_MAX_NUMBERS; i++)
    {
		if( 0 ==DRVCUST_InitQuery(_i4MSDCgpio[i], &u4MsdcGpioConfig))
		{
			sprintf(pstr,"%d",u4MsdcGpioConfig);
			strcat(msdcgpio,pstr );
		}
		else
		{
			strcat(msdcgpio,"-1");
		}
	
		if(i !=MSDC_GPIO_MAX_NUMBERS-1)
		{
			strcat(msdcgpio,",");	
		}			
    }
	printf("msdcgpio%s,\n",msdcgpio);
    env_add_bootargs("msdcgpio=",msdcgpio);
}
#endif

#ifdef CC_TRUSTZONE_SUPPORT
static void env_add_tz_size(void)
{
    char *p;
    char buf[32];
    p = getenv("tzsize");
    sprintf(buf,"tzsz=%s", p);
    env_add_bootargs("tzsz=", buf);
}
#endif


#if defined(CC_CMDLINE_SUPPORT_BOOTREASON)
#define PDWNC_REG_RESERVE_BOOTREASON  0xF0028624
static void env_add_bootreason(void)
{
    char newstr[32] = {0};
	char wakeupreason = (*((volatile char*)(PDWNC_REG_RESERVE_BOOTREASON)));
	(*((volatile char*)(PDWNC_REG_RESERVE_BOOTREASON))) = wakeupreason & 0xFFFFFF00;
	if(( wakeupreason & 0xF)==0x1)
	{
		sprintf(newstr, "bootreason=kernel_panic");
	}
	else if(( wakeupreason & 0xF)==0x2)
	{
		sprintf(newstr, "bootreason=reboot");
	}
	else if(( wakeupreason & 0xF)==0x3)
	{
		sprintf(newstr, "bootreason=watchdog");
	}
	else
	{
		return;
	}
	env_add_bootargs("bootreason=", newstr);
}
#endif 

#ifdef ANDROID
#ifdef CC_UBOOT_MINIGUNZIP_SUPPORT
static void env_add_minigzip(void)
{
    char newstr[32] = {0};

    sprintf(newstr, "minigzip");
    env_add_bootargs("minigzip", newstr);
}
#endif
#endif
#ifdef CC_SUPPORT_STR
static void env_add_no_console_suspend(void)
{
    char newstr[32] = {0};

    sprintf(newstr, "no_console_suspend");
    env_add_bootargs("no_console_suspend", newstr);
}
#endif
#ifdef CC_PDWNC_REBOOT_NOTIFY 
static void env_add_recovery_mode(void)
{
    char newstr[32] = {0};

    if (BIM_IsRecoveryMode())
    {
        sprintf(newstr, "AndroidRecovery");
        env_add_bootargs("AndroidRecovery", newstr);
    }
}
#endif

extern int DRVCUST_QueryPart(int i, int *pData, int type);
extern int DRVCUST_QueryPart64(int i, u64 *pData, int type);

static void set_cust_env(void)
{
#ifndef CC_KERNEL_ON_NOR
    int i = 0, i4PartNum = 0, encrypt = 0;
    u64 size = 0;
    char *p1 = NULL, *p2 = NULL;
    char *name = NULL;
    char buf[64] = {0};
    char *mtdparts = NULL;
    char *bootargs = NULL;
#ifdef CC_PARTITION_WP_SUPPORT
    int wp = 0;
#endif

    mtdparts = (char *)malloc(default_environment_size);
    memset(mtdparts, 0, default_environment_size);

    bootargs = (char *)malloc(default_environment_size);
    memset(bootargs, 0, default_environment_size);

    DRVCUST_QueryPart(0, &i4PartNum, 0);

    // generate mtdparts string
    while (i <= i4PartNum)
    {
        DRVCUST_QueryPart64(i, &size, 1);
        DRVCUST_QueryPart(i, (UPTR *)&name, 2);
        DRVCUST_QueryPart(i, &encrypt, 3);
#ifdef CC_PARTITION_WP_SUPPORT
        DRVCUST_QueryPart(i, &wp, 4);
#endif
        if (!size)
        {
            break;
        }

        if (i == 0)
        {
#if defined(CC_EMMC_BOOT)
            strcat(mtdparts, "mt53xx-emmc:");
#else
            strcat(mtdparts, "mt53xx-nand:");
#endif
        }
        else
        {
            strcat(mtdparts, ",");
        }

		/* CAUTION: 64 bit size limition 2TB !!! */
        if ((size % (1024 * 1024)) == 0)
        {
            sprintf(buf, "%dM(%s)", (u32)(size /1024 /1024), name);
        }
        else if ((size % 1024) == 0)
        {
            sprintf(buf, "%dk(%s)", (u32)(size /1024), name);
        }
        else
        {
            BUG();
        }

        strcat(mtdparts, buf);

        if (encrypt)
        {
            strcat(mtdparts, "enc");
        }
#ifdef CC_PARTITION_WP_SUPPORT
        if (wp)
        {
            strcat(mtdparts, "wp");
        }
#endif

        i++;
    }

    strcpy(mtdparts_default, mtdparts);

    // update mtdparts string
    setenv("mtdparts", mtdparts);

    // generate bootargs string
    p1 = getenv("bootargs");
    if (p1 == NULL)
    {
        free(mtdparts);
        free(bootargs);
        return;
    }

    strcpy(bootargs, p1);

    // delete any existing mtdparts string in bootargs string.
    p1 = strstr(bootargs, "mtdparts=");
    if (p1)
    {
        p2 = strstr(p1, " ");
        if(p2)
        {
            while(*++p2)
            {
                *p1++ = *p2;
            }

            *p1++ = ' ';
        }

        *p1 = '\0';
    }
    else
    {
        strcat(bootargs, " ");
    }

    // append new mtdparts string at the end of bootargs string.
    strcat(bootargs, "mtdparts=");
    strcat(bootargs, mtdparts);

    // update bootargs environment
    setenv("bootargs", bootargs);

    // set kernel memory size, kmemsize always is not equal to zero.
    if (kmemsize)
    {
        sprintf(buf, "0x%x", kmemsize);
        setenv("kmemsize", buf);
		
#if defined(CC_DYNAMIC_FBMSRM_CONF)
		sprintf(buf, "0x%x", kmem4ksize);
		setenv("kmem4ksize", buf);
		sprintf(buf, "0x%x", kmemfhdsize);
		setenv("kmemfhdsize", buf);
#endif
#if CONFIG_NR_DRAM_BANKS>1
        sprintf(buf, "0x%x", kmem2start);
        setenv("kmem2start", buf);
        sprintf(buf, "0x%x", kmem2size);
        setenv("kmem2size", buf);	
#if defined(CC_DYNAMIC_FBMSRM_CONF)
        sprintf(buf, "0x%x", kmem24ksize);
        setenv("kmem24ksize", buf);
        sprintf(buf, "0x%x", kmem2fhdsize);
        setenv("kmem2fhdsize", buf);
        sprintf(buf, "%d", gpu4kstart);
        setenv("gpu4kstart", buf);
        sprintf(buf, "%d", gpufhdstart);
        setenv("gpufhdstart", buf);
        sprintf(buf, "%d", gpu4ksize);
        setenv("gpu4ksize", buf);
        sprintf(buf, "%d", gpufhdsize);
        setenv("gpufhdsize", buf);
        sprintf(buf, "%d", gpu4kionsize);
        setenv("gpu4kionsize", buf);
        sprintf(buf, "%d", gpufhdionsize);
        setenv("gpufhdionsize", buf);
#endif
#endif
#ifdef CC_SUPPORT_CHANNEL_C
#ifdef CC_MT5891
	if(BSP_GetIcVersion() != IC_VER_5891_AA)
		{
			sprintf(buf, "0x%x", kmem3start);
			setenv("kmem3start", buf);
			sprintf(buf, "0x%x", kmem3size);
			setenv("kmem3size", buf);
		}
#else
	sprintf(buf, "0x%x", kmem3start);
	setenv("kmem3start", buf);
	sprintf(buf, "0x%x", kmem3size);
	setenv("kmem3size", buf);	
#endif
#endif

    }

#ifdef CC_TRUSTZONE_SUPPORT
    if(TRUSTZONE_MEM_SIZE%(1024*1024) == 0)
    {
        sprintf(buf,"%dm", TRUSTZONE_MEM_SIZE/(1024*1024));
    }
    else if(TRUSTZONE_MEM_SIZE%1024 == 0)
    {
        sprintf(buf,"%dk", TRUSTZONE_MEM_SIZE/(1024));
    }
    else
    {
        sprintf(buf, "%d", TRUSTZONE_MEM_SIZE);
    }
    setenv("tzsize", buf);
#endif

    free(mtdparts);
    free(bootargs);
#endif
}

void set_default_env(const char *s)
{
    unsigned int* penv;
	if (sizeof(default_environment) > ENV_SIZE) {
		puts("\n*** Error - default environment is too large.\n\n");
		return;
	}

	if (s) {
		if (*s == '!') {
			printf("\n*** Warning - %s, using default environment.\n\n", s+1);
		} else {
			puts(s);
		}
	} else {
		puts("\nUsing default environment.\n\n");
	}
	
	penv = (unsigned int*)env_ptr;
	//get ATAG kernel memory from env.o
	if (penv[0]==0x464c457f)
	{
		int i=1;
		for (; i<0x40; i++)
		{
			// find magic key, the next work is global_kernel_dram_size
			if (penv[i] == 0x53965368)
			{
		#if defined(CC_DYNAMIC_FBMSRM_CONF)
				kmemsize = penv[i+1];
				kmem4ksize = penv[i+2];
				kmemfhdsize = penv[i+3];

			#if CONFIG_NR_DRAM_BANKS>1
				kmem2start = penv[i+4];
				kmem2size = penv[i+5];
				kmem24ksize = penv[i+6];
				kmem2fhdsize = penv[i+7];
			#endif
				gpu4kstart = penv[i+8];
				gpufhdstart = penv[i+9];
				gpu4ksize = penv[i+10];
				gpufhdsize = penv[i+11];
				gpu4kionsize = penv[i+12];
				gpufhdionsize = penv[i+13];
		#else
				kmemsize = penv[i+1];
			#if CONFIG_NR_DRAM_BANKS>1
				kmem2start = penv[i+2];
				kmem2size = penv[i+3];

				#ifdef CC_SUPPORT_CHANNEL_C
				kmem3start = penv[i+4];
				kmem3size = penv[i+5];
				#endif

			#endif
		#endif
				printf("set_default_env from env partition !!!\n\n kmemsize\t= 0x%08x\n kmem2start\t= 0x%08x\n kmem2size\t= 0x%08x\n kmem3start\t= 0x%08x\n kmem3size\t= 0x%08x\n\n", kmemsize, kmem2start, kmem2size,kmem3start,kmem3size);

				break;
			}
		}
	}

	//if get ATAG kernel memory from env.o fail, force initial
	if(kmemsize == 0)
	{
		kmemsize = PHYS_SDRAM_1_SIZE;
#if defined(CC_DYNAMIC_FBMSRM_CONF)
		kmem4ksize = PHYS_SDRAM_1_4K_SIZE;
		kmemfhdsize = PHYS_SDRAM_1_FHD_SIZE;

	#if CONFIG_NR_DRAM_BANKS>1
		kmem2start = PHYS_SDRAM_2;
		kmem2size = PHYS_SDRAM_2_SIZE;
		kmem24ksize = PHYS_SDRAM_2_4K_SIZE;
		kmem2fhdsize = PHYS_SDRAM_2_FHD_SIZE;
	#endif
	#if defined(PHYS_GPU_4K_START)&&defined(PHYS_GPU_FHD_START)
		gpu4kstart = PHYS_GPU_4K_START;
		gpufhdstart = PHYS_GPU_FHD_START;
		gpu4ksize = PHYS_GPU_4K_SIZE;
		gpufhdsize = PHYS_GPU_FHD_SIZE;
		gpu4kionsize = PHYS_GPU_4K_ION_SIZE;
		gpufhdionsize = PHYS_GPU_FHD_ION_SIZE;
	#endif
#else
	#if CONFIG_NR_DRAM_BANKS>1
		kmem2start = PHYS_SDRAM_2;
		kmem2size = PHYS_SDRAM_2_SIZE;

		#ifdef CC_SUPPORT_CHANNEL_C
		kmem3start = PHYS_SDRAM_3;
		kmem3size = PHYS_SDRAM_3_SIZE;
		#endif
	#endif
#endif

		printf("set_default_env from macro definition !!!\n\n kmemsize\t= 0x%08x\n kmem2start\t= 0x%08x\n kmem2size\t= 0x%08x\n kmem3start\t= 0x%08x\n kmem3size\t= 0x%08x\n\n", kmemsize, kmem2start, kmem2size,kmem3start,kmem3size);
	}

	if (himport_r(&env_htab, (char *)default_environment,
			sizeof(default_environment), '\0', 0) == 0)
		error("Environment import failed: errno = %d\n", errno);

	gd->flags |= GD_FLG_ENV_READY;
	
	set_cust_env();
    saveenv();
}

/*
 * Check if CRC is valid and (if yes) import the environment.
 * Note that "buf" may or may not be aligned.
 */
int env_import(const char *buf, int check)
{
	env_t *ep = (env_t *)buf;

	if (check) {
		uint32_t crc;

		memcpy(&crc, &ep->crc, sizeof(crc));

		if (crc32(0, ep->data, ENV_SIZE) != crc) {
			set_default_env("!bad CRC");
			return 0;
		}
	}

	if (himport_r(&env_htab, (char *)ep->data, ENV_SIZE, '\0', 0)) {
		gd->flags |= GD_FLG_ENV_READY;
		return 1;
	}

	error("Cannot import environment: errno = %d\n", errno);

	set_default_env("!import failed");

	return 0;
}
#if defined(CC_SECURE_ROM_BOOT)
int IsRunOnUsb(char* uenv, int uenv_size);

static void env_add_usig(void)
{
    char *p1 = NULL;
    char uenv[125];

    if (IsRunOnUsb((char *)&uenv, sizeof(uenv)))
    {
        // generate bootargs string
        p1 = getenv("bootargs");
        if (p1 == NULL)
        {
            return;
        }
        printf("run_on_usb, uenv=%s\n", uenv);
        // append new string at the end of bootargs string.
        strcat(p1, " ");
        strcat(p1, uenv);
        // update bootargs environment
        setenv("bootargs", p1);
        setenv("usigenv", uenv);
    }

}
#endif // CC_SECURE_ROM_BOOT

void env_relocate(void)
{
#if defined(CONFIG_NEEDS_MANUAL_RELOC)
	env_reloc();
#endif
	if (gd->env_valid == 0) {
#if defined(CONFIG_ENV_IS_NOWHERE)	/* Environment not changable */
		set_default_env(NULL);
#else
		show_boot_progress(-60);
		set_default_env("!bad CRC");
#endif
	} else {
		env_relocate_spec();
#ifndef CC_KERNEL_ON_NOR
	{
		char *p1 = NULL;

		p1 = getenv("mtdparts");
		if (p1 == NULL)
		{
			return;
		}

		strcpy(mtdparts_default, p1);
	}
#endif
	}
#ifdef CC_ENABLE_MTK_MODEL_INDEX 
    env_add_panel_index();
#endif

#ifdef CC_ENABLE_MALI_CORE_NUM
    env_add_mali_core_num(); 
#endif

#ifdef CC_UBOOT_TRANSFER_USBGPIO_KERNEL
    env_add_usb_gpio(); 
#endif

#ifdef CC_S_PLATFORM
#ifdef CC_UBOOT_TRANSFER_UART_MODE
	env_add_uart_mode();
#endif
#endif

#ifdef CC_UBOOT_TRANSFER_MSDCGPIO_KERNEL
    env_add_msdc_gpio(); 
#endif
#ifdef CC_TRUSTZONE_SUPPORT
    env_add_tz_size();
#endif

#ifdef CC_FAST_INIT
    env_add_fastboot();
#endif
#ifdef CC_SUPPORT_STR
    env_add_no_console_suspend();
#endif
#ifdef ANDROID
#ifdef CC_UBOOT_MINIGUNZIP_SUPPORT
    env_add_minigzip();
#endif
#endif
#ifdef CC_PDWNC_REBOOT_NOTIFY
    env_add_recovery_mode();
#endif
#ifdef CC_MT53XX_SUPPORT_2G_DRAM
    //env_add_vmalloc_cust();
#endif
#ifdef CC_SECURE_ROM_BOOT
    env_add_usig();
#endif
#ifdef CONFIG_AMIGAONEG3SE
    disable_nvram();
#endif
#ifdef CC_KERNEL_NO_LOG
char*	print = getenv("print");
if(print == NULL)
{
	setenv("print", "off");
	print = getenv("print");
}

if(!strcmp(print, "off"))
{
  env_add_forbid_uart();
}
#endif

char*	perfor = getenv("log_performance");
if(perfor == NULL)
{
	setenv("log_performance", "on");
	perfor = getenv("log_performance");
}
if(!strcmp(perfor, "on"))
{
  env_add_perfor_uart();
}
else
{
  env_del_perfor_uart();
}

#ifdef APP_B2B_SUPPORT
    env_add_pq_mode(); //add by heyang3
#endif

#ifdef CC_MT5890
  env_add_have_vp9();
#endif

#ifdef CC_VP9_SUPPORT
  env_add_vp9_support();
#endif

  env_add_gpu_addr();
 env_add_usb_serialno();
#ifdef CC_3RD_KO_INSERTION
  env_add_beic_support();
#endif

#ifdef CC_S_PLATFORM
#ifdef CC_DYNAMIC_USB_ADB_CONFIG
  env_add_adb_support();
#endif
#ifdef CC_USB3_TO_ADB_PORT_CONFIG
  env_add_u32adb_support();
#endif
#else
    //env_add_adb_support();
#endif
#ifdef CC_SUPPORT_CHIPID
#ifdef ANDROID

	env_add_chipid();
#endif
#endif
#if defined(CC_CMDLINE_SUPPORT_BOOTREASON)
	env_add_bootreason();
#endif

	#ifdef LPJ_IC_VER1
	if(BSP_GetIcVersion() == LPJ_IC_VER1)
		{
			env_add_bootargs("lpj=", LPJ_IC_VALUE1);
		}
	#endif

	#ifdef LPJ_IC_VER1
		if(BSP_GetIcVersion() == LPJ_IC_VER2)
		{
			env_add_bootargs("lpj=", LPJ_IC_VALUE2);
		}
	#endif
#if defined(CC_UBOOT_ADD_USB_LOG_MODE)
    env_add_usb_log();
#endif

}

#ifdef CONFIG_AUTO_COMPLETE
int env_complete(char *var, int maxv, char *cmdv[], int bufsz, char *buf)
{
	ENTRY *match;
	int found, idx;

	idx = 0;
	found = 0;
	cmdv[0] = NULL;

	while ((idx = hmatch_r(var, idx, &match, &env_htab))) {
		int vallen = strlen(match->key) + 1;

		if (found >= maxv - 2 || bufsz < vallen)
			break;

		cmdv[found++] = buf;
		memcpy(buf, match->key, vallen);
		buf += vallen;
		bufsz -= vallen;
	}

	qsort(cmdv, found, sizeof(cmdv[0]), strcmp_compar);

	if (idx)
		cmdv[found++] = "...";

	cmdv[found] = NULL;
	return found;
}
#endif
