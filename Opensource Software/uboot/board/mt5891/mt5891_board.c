/*
 * vm_linux/chiling/uboot/board/mt5398/mt5398_board.c
 *
 * Copyright (c) 2010-2012 MediaTek Inc.
 *	This file is based  ARM Realview platform
 *  Copyright (C) 2002 ARM Ltd.
 *  All Rights Reserved
 * $Author: tao.tian $
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
 */

#include <common.h>
#include <exports.h>

#include "x_bim.h"
#include "x_timer.h"
#include "x_ckgen.h"
#include "x_assert.h"
#include "x_printf.h"
#include "x_dram.h"
#include "x_gpio.h"
#include "x_irq.h"
#include "drvcust_if.h"
#include "c_model.h"

//-----------------------------------------------------------------------------
// Constant definitions
//-----------------------------------------------------------------------------

// MT5391 timer define
#define TIMER_ID                    2

#define TIMER_LOW_REG               (REG_RW_TIMER0_LOW + (TIMER_ID * 8))
#define TIMER_HIGH_REG              (REG_RW_TIMER0_HIGH + (TIMER_ID * 8))
#define TIMER_LIMIT_LOW_REG         (REG_RW_TIMER0_LLMT + (TIMER_ID * 8))
#define TIMER_LIMIT_HIGH_REG        (REG_RW_TIMER0_HLMT + (TIMER_ID * 8))
#define TCTL_TIMER_EN               (TMR_CNTDWN_EN(TIMER_ID))
#define TCTL_TIMER_AL               (TMR_AUTOLD_EN(TIMER_ID))
#define MAX_TIMER_INTERVAL          (0xffffffff)
#define XTAL_STRAP_MODE     (CKGEN_READ32(0) & 0x60)
    #define SET_XTAL_27MHZ  (0U << 5)
    #define SET_XTAL_60MHZ  (1U << 5)
    #define SET_XTAL_48MHZ  (2U << 5)
    #define SET_XTAL_54MHZ  (3U << 5)

//-----------------------------------------------------------------------------
// Type definitions
//-----------------------------------------------------------------------------
typedef struct {
    char command[32];
    char status[32];
    char recovery[1024];
} misc_message;



//-----------------------------------------------------------------------------
// Static variables
//-----------------------------------------------------------------------------
/* U-Boot expects a 32 bit timer running at CFG_HZ*/
static HAL_RAW_TIME_T lastdec;		/* Timer reading at last call	      */
static u_long timestamp;		        /* U-Boot ticks since startup	      */
static u_long systemclk;

//-----------------------------------------------------------------------------
// Extern functions
//-----------------------------------------------------------------------------
extern INT16 SIF_MT8297_DirectWrite8(UINT8, UINT8);
extern INT32 EEPROM_Read(UINT64, UINT32, UINT32);

//-----------------------------------------------------------------------------
// Static function forward declarations
//-----------------------------------------------------------------------------
static void timer_init (void);

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#if defined(CONFIG_SHOW_BOOT_PROGRESS)
void show_boot_progress(int progress)
{
	printf("Boot reached stage %d\n", progress);
}
#endif


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
static inline void delay (unsigned long loops)
{
	__asm__ volatile ("1:\n"
		"subs %0, %1, #1\n"
		"bne 1b":"=r" (loops):"0" (loops));
}

static inline void arm_write_ctrl_reg(UINT32 val)
{
    asm volatile("mcr p15, 0, %0, c1, c0, 0" : : "r" (val));
}

static inline UINT32 arm_read_ctrl_reg(void)
{
    UINT32 val;
    asm volatile("mrc p15, 0, %0, c1, c0, 0" : "=r" (val));
    return val;
}

void turn_on_USB_power_switch(void)
{
    UINT32 u4VbusGpio;    
    UINT32 u4VbusGpioPolarity;   

    if ((0 == DRVCUST_InitQuery(eUSBVbus0Gpio, &u4VbusGpio)) &&
        (0 == DRVCUST_InitQuery(eUSBVbus0EnablePolarity, &u4VbusGpioPolarity)))
    {
        BSP_PinGpioSel((INT32)u4VbusGpio);
        GPIO_SetOut((INT32)u4VbusGpio, (INT32)u4VbusGpioPolarity);
    }

    if ((0 == DRVCUST_InitQuery(eUSBVbus1Gpio, &u4VbusGpio)) &&
        (0 == DRVCUST_InitQuery(eUSBVbus1EnablePolarity, &u4VbusGpioPolarity)))
    {
        BSP_PinGpioSel((INT32)u4VbusGpio);
        GPIO_SetOut((INT32)u4VbusGpio, (INT32)u4VbusGpioPolarity);
    }
    if ((0 == DRVCUST_InitQuery(eUSBVbus2Gpio, &u4VbusGpio)) &&
        (0 == DRVCUST_InitQuery(eUSBVbus2EnablePolarity, &u4VbusGpioPolarity)))
    {
        BSP_PinGpioSel((INT32)u4VbusGpio);
        GPIO_SetOut((INT32)u4VbusGpio, (INT32)u4VbusGpioPolarity);
    }

    if ((0 == DRVCUST_InitQuery(eUSBVbus3Gpio, &u4VbusGpio)) &&
        (0 == DRVCUST_InitQuery(eUSBVbus3EnablePolarity, &u4VbusGpioPolarity)))
    {
        BSP_PinGpioSel((INT32)u4VbusGpio);
        GPIO_SetOut((INT32)u4VbusGpio, (INT32)u4VbusGpioPolarity);
    }
}

void turn_off_USB_power_switch(void)
{
    UINT32 u4VbusGpio;    
    UINT32 u4VbusGpioPolarity;   
    
    if ((0 == DRVCUST_InitQuery(eUSBVbus0Gpio, &u4VbusGpio)) &&
        (0 == DRVCUST_InitQuery(eUSBVbus0EnablePolarity, &u4VbusGpioPolarity)))
    {
        BSP_PinGpioSel((INT32)u4VbusGpio);
        GPIO_SetOut((INT32)u4VbusGpio, (INT32)!u4VbusGpioPolarity);
    }

    if ((0 == DRVCUST_InitQuery(eUSBVbus1Gpio, &u4VbusGpio)) &&
        (0 == DRVCUST_InitQuery(eUSBVbus1EnablePolarity, &u4VbusGpioPolarity)))
    {
        BSP_PinGpioSel((INT32)u4VbusGpio);
        GPIO_SetOut((INT32)u4VbusGpio, (INT32)!u4VbusGpioPolarity);
    }
    if ((0 == DRVCUST_InitQuery(eUSBVbus2Gpio, &u4VbusGpio)) &&
        (0 == DRVCUST_InitQuery(eUSBVbus2EnablePolarity, &u4VbusGpioPolarity)))
    {
        BSP_PinGpioSel((INT32)u4VbusGpio);
        GPIO_SetOut((INT32)u4VbusGpio, (INT32)!u4VbusGpioPolarity);
    }

    if ((0 == DRVCUST_InitQuery(eUSBVbus3Gpio, &u4VbusGpio)) &&
        (0 == DRVCUST_InitQuery(eUSBVbus3EnablePolarity, &u4VbusGpioPolarity)))
    {
        BSP_PinGpioSel((INT32)u4VbusGpio);
        GPIO_SetOut((INT32)u4VbusGpio, (INT32)!u4VbusGpioPolarity);
    }    
//#ifdef CONFIG_USB_MTK_XHCI_HCD
	    vIO32WriteFldAlign(0xf0070700, 0x1, Fld(1,0,AC_MSKB0));
	    vIO32WriteFldAlign(0xf0070704, 0x1, Fld(1,0,AC_MSKB0));
//#endif

}

//-----------------------------------------------------------------------------
/*
 * Miscellaneous platform dependent initialisations
 */
//-----------------------------------------------------------------------------
int board_init (void)
{
    UINT32 u4VbusGpio;    
    UINT32 u4VbusGpioPolarity;    
    UINT32 u4Val;

    DECLARE_GLOBAL_DATA_PTR;


    /* Enable unaligned access, disable alignment trap */
    u4Val = arm_read_ctrl_reg();
    u4Val |= (1 << 22);
    u4Val &= ~(1 << 1);
    arm_write_ctrl_reg(u4Val);
    
    /* arch number of MT5363 Board */
    gd->bd->bi_arch_number = machine_arch_type;

    /* adress of boot parameters */
    gd->bd->bi_boot_params = 0x100;

    gd->flags = 0;

#if (CONFIG_LOG_SILENT)
    gd->flags |= GD_FLG_SILENT; /* enable Silent */
#endif

    timer_init();

    // turn on usb vbus control.
    turn_on_USB_power_switch();

    return 0;
}

//-----------------------------------------------------------------------------
static void bootmode_init (void)
{
#if defined(BOOT_MODE_EEPROM_OFFESET) && defined(BOOT_MODE_GPIO_NUM)
    char *cmdline = getenv ("bootargs");        // default setting /dev/mtd1=Kernel,  /dev/mtd3=RFS
    if (BOOT_MODE_EEPROM_OFFESET && BOOT_MODE_GPIO_NUM && cmdline )
    {
        char bootmode[8], buf[256], tmp[16], buf2[256];
        UINT32 u4EEPVal, u4EEPVal2 = 0;
        INT32 i4GPIOVal =0;

        debug ("Bootmode enable, original bootargs:\n  %s\n", cmdline);
   
        DECLARE_GLOBAL_DATA_PTR;
        extern INT32 EEPROM_Read(UINT64, UINT32, UINT32);
        gd->flags |= GD_FLG_SILENT; /* Disable EEPROM read message */
        EEPROM_Read((UINT64)BOOT_MODE_EEPROM_OFFESET, (UINT32)&u4EEPVal, 1);
        EEPROM_Read((UINT64)BOOT_MODE_PARTITION_OFFSET, (UINT32)&u4EEPVal2, 1);
        gd->flags &= ~(GD_FLG_SILENT); /* Restore console */

        debug ("u4EEPVal: %d\n", u4EEPVal);
        debug ("u4EEPVal2: %d\n", u4EEPVal2);

#ifdef CC_NAND_BOOT     /* in case of jig-boot, do nothing here */
        /*
            u4EEPVal2 Spec:
                #define VUM_FLG_BOOT_KERNEL     0x0
                #define VUM_FLG_BOOT_KERNEL2    0x1
                #define VUM_FLG_BOOT_RFS        0x0
                #define VUM_FLG_BOOT_RFS2       0x2
        */

        // in case that un-set flag value
        if ( u4EEPVal2 == 0xFF )
            u4EEPVal2 = 0;  /* boot from /dev/mtd1, /dev/mtd3 */

        if(( u4EEPVal2 & 0x1 ) == 0x1)
        {
            // kernel from /dev/mtd2
            setenv("bootcmd", CONFIG_BOOTCOMMAND2);
        }

        if(( u4EEPVal2 & 0x2 ) == 0x2)
        {
            // RFS from /dev/mtd4
            sprintf(buf2, CONFIG_BOOTARGS2 );
            printf("buf2=%s", buf2);
            setenv("bootargs", buf2);

            cmdline = buf2;
        }
#endif    // #ifdef CC_NAND_BOOT        /* in case of jig-boot, do nothing here */

        /* read 3 bit in operand */
        u4EEPVal &= 0x7;


        if(u4EEPVal <= 3)
        {
            strcpy(bootmode, "uvp");
        }
        else
        {
            GPIO_Enable((INT32)BOOT_MODE_GPIO_NUM, &i4GPIOVal); /* Change to input mode first. */
            i4GPIOVal = GPIO_Input((INT32)BOOT_MODE_GPIO_NUM);

            debug ("i4GPIOVal: %d\n", i4GPIOVal);

            if (!i4GPIOVal)
            {
                strcpy(bootmode, "usb");
            }
            else
            {
                strcpy(bootmode, "normal");
            }
        }

        sprintf (tmp, " bootmode=%s", bootmode);
        strcpy (buf, cmdline);
        strcat (buf, tmp);
        debug ("Add bootmode to bootargs:\n  %s\n", buf);
        setenv ("bootargs", buf);
    }
#endif
}


//-----------------------------------------------------------------------------
int misc_init_r (void)
{
#if CONFIG_LOAD_MAC_ADDRESS
    char* s;
#endif

    if(!getenv ("verify"))
        setenv("verify", "y");

    bootmode_init();

#if CONFIG_LOAD_MAC_ADDRESS
    // Uboot load MAC address from uboot environment.
    // If it does not exist, Uboot load MAC address in EEPROM
    if ((s = getenv ("bootargs")) != NULL)
    {
        if(strstr((const char *)s, "mac") == NULL)
        {
            UINT8 au1_mac[6], tmp[512];
            EEPROM_Read((UINT64)1200, au1_mac, 6);
            sprintf(tmp, "%s mac=%02x:%02x:%02x:%02x:%02x:%02x", s,
                au1_mac[0], au1_mac[1], au1_mac[2], au1_mac[3], au1_mac[4], au1_mac[5]);
            setenv("bootargs", tmp);
            saveenv();
        }
    }
    else
    {
        printf("get bootargs failed.");
        ASSERT(0);
    }
#endif
    return (0);
}

//-----------------------------------------------------------------------------
int dram_init (void)
{
	DECLARE_GLOBAL_DATA_PTR;
#ifndef CC_UBOOT_2011_12
	gd->bd->bi_dram[0].start = PHYS_SDRAM_1;
	gd->bd->bi_dram[0].size	 = PHYS_SDRAM_1_SIZE;

        #if defined(PHYS_SDRAM_2) && defined(PHYS_SDRAM_2_SIZE)
	gd->bd->bi_dram[1].start = PHYS_SDRAM_2;
	gd->bd->bi_dram[1].size	 = PHYS_SDRAM_2_SIZE;
        #endif
#else
   /* dram_init must store complete ramsize in gd->ram_size */	
    gd->ram_size = PHYS_SDRAM_1_SIZE;
#endif
	return 0;
}

//-----------------------------------------------------------------------------
int interrupt_init (void)
{
	return (0);
}

//-----------------------------------------------------------------------------
static u_long GetSystemClk(void)
{
    u_long u4Clock;
    switch(XTAL_STRAP_MODE) {
        case SET_XTAL_27MHZ: u4Clock = 27000000; break;
        case SET_XTAL_60MHZ: u4Clock = 60000000; break;
        case SET_XTAL_48MHZ: u4Clock = 48000000; break;
        case SET_XTAL_54MHZ: u4Clock = 54000000; break;
        default: u4Clock = 27000000; break;
    }
    return u4Clock;
}

//-----------------------------------------------------------------------------
static void timer_init (void)
{
    systemclk = GetSystemClk();
    //puts("\nMT5391 timer init\n");

    // Use Timer 2
    // Disable timer
    BIM_REG32(REG_RW_TIMER_CTRL) &= ~(TCTL_TIMER_EN | TCTL_TIMER_AL);	

    // Setup timer interrupt interval
    BIM_WRITE32(TIMER_LIMIT_LOW_REG, MAX_TIMER_INTERVAL);
    BIM_WRITE32(TIMER_LIMIT_HIGH_REG, MAX_TIMER_INTERVAL);

    // Enable timer with auto-load
    BIM_REG32(REG_RW_TIMER_CTRL) |= (TCTL_TIMER_EN | TCTL_TIMER_AL);	
}

//-----------------------------------------------------------------------------
void reset_timer (void)
{
    reset_timer_masked ();
}

//-----------------------------------------------------------------------------
u_long get_timer (u_long base_ticks)
{
    return get_timer_masked () - base_ticks;
}

//-----------------------------------------------------------------------------
void set_timer (u_long ticks)
{
    timestamp = ticks;
    HAL_GetRawTime(&lastdec);
}

//-----------------------------------------------------------------------------
/* delay x useconds */
//-----------------------------------------------------------------------------
void udelay (u_long usec)
{
    HAL_Delay_us(usec);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void reset_timer_masked (void)
{
    /* reset time */
    HAL_GetRawTime(&lastdec);
    timestamp = 0;		/* start "advancing" time stamp from 0 */
}

//-----------------------------------------------------------------------------
/* converts the timer reading to U-Boot ticks	       */
/* the timestamp is the number of ticks since reset    */
/* This routine does not detect wraps unless called regularly
   ASSUMES a call at least every 16 seconds to detect every reload */
//-----------------------------------------------------------------------------   
u_long get_timer_masked (void)
{
    HAL_RAW_TIME_T now, timediff;
    uint64_t cycle;

    HAL_GetRawTime(&now);
    HAL_GetDeltaRawTime(&timediff, &lastdec, &now);

    cycle = ((uint64_t)timediff.u4Ticks << 32) | timediff.u4Cycles;

    timestamp = cycle / CFG_CLOCK_PER_TICKS;
    
#if 0
    // There is no 64-bit divider
    timestamp = 0;
    while(1)
    {
        if(cycle > CFG_CLOCK_PER_TICKS)
            timestamp++;
    }
#endif

    return timestamp;
}

//-----------------------------------------------------------------------------
/* waits specified delay value and resets timestamp */
//-----------------------------------------------------------------------------
void udelay_masked (u_long usec)
{
	udelay(usec);
}

//-----------------------------------------------------------------------------
/*
 * This function is derived from PowerPC code (read timebase as long long).
 * On ARM it just returns the timer value.
 */
//-----------------------------------------------------------------------------
unsigned long long get_ticks(void)
{
    return get_timer(0);
}

unsigned int readMsTicks(void)
{
    HAL_TIME_T now;
    HAL_GetTime(&now);
    return (unsigned int)(now.u4Seconds*1000+(now.u4Micros/1000));
}

//-----------------------------------------------------------------------------
/*
 * Return the timebase clock frequency
 * i.e. how often the timer decrements
 */
//-----------------------------------------------------------------------------
u_long get_tbclk (void)
{
    return CFG_CLOCK_PER_TICKS;
}

#define REG_RW_WATCHDOG_EN  0x0100          // Watchdog Timer Control Register
#define REG_RW_WATCHDOG_TMR 0x0104          // Watchdog Timer Register
#define PDWNC_WRITE32(offset, value)    IO_WRITE32(PDWNC_BASE, (offset), value)

void watchdog_reset()
{

    PDWNC_WRITE32(REG_RW_WATCHDOG_TMR, 0xffff0000);
    PDWNC_WRITE32(REG_RW_WATCHDOG_EN, 0xffff);
    
}

void watchdog_enable()
{

	// Set watchdog timer initial value, 25 seconds.
	PDWNC_WRITE32(REG_RW_WATCHDOG_TMR, 0x8000000);
	// Enable the watchdog, then watchdog can be disable by outside.
	PDWNC_WRITE32(REG_RW_WATCHDOG_EN, 1);
	
}

//-----------------------------------------------------------------------------
/* Uboot commands */
//-----------------------------------------------------------------------------

#define REG_SER_PC_READ_MODE        (*((volatile unsigned long*)(RS232_BASE + 0x18)))
#define REG_SER_CFG                 (*((volatile unsigned long*)(RS232_BASE + 0x04)))
int do_ser_stop(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	printf("System is ready for using MTKTool\n");

    REG_SER_PC_READ_MODE = 0; // Set RS232 active read mode
    REG_SER_CFG = 0;     	// Set to normal mode

    while(1)
    	;
	
	return 0;
}

U_BOOT_CMD(
	0,	1,		1,	do_ser_stop,
	"0.stop  - program flash using MTKTool\n",
	"\n"
	"    - Make system to be ready for MTKTool\n"
);

int do_load (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int i;

	if (argc < 2) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

	for (i=1; i<argc; ++i) {
		char *arg;

		if ((arg = getenv (argv[i])) == NULL) {
			printf ("## Error: \"%s\" not defined\n", argv[i]);
			return 1;
		}
#ifndef CFG_HUSH_PARSER
		if (run_command (arg, flag) == -1)
			return 1;
#else
		if (parse_string_outer(arg,
		    FLAG_PARSE_SEMICOLON | FLAG_EXIT_FROM_LOOP) != 0)
			return 1;
#endif
	}
	return 0;
}    

int DRVCUST_QueryPart(int i, UPTR *pData, int type)
{
    UPTR u4Data = 0;
    
    if (type == 0)
    {
        u4Data = DRVCUST_InitGet(eNANDFlashPartEnd);
    }
    else if(type == 1)
    {
        u4Data = DRVCUST_InitGet((QUERY_TYPE_T)(eNANDFlashPartSize0 + i));
    }
    else if(type == 2)
    {
        u4Data = DRVCUST_InitGetUptr((QUERY_TYPE_T)(eNANDFlashPartName0 + i));
    }
    else if(type == 3)
    {
#ifdef CC_MTD_ENCRYPT_SUPPORT
        u4Data = DRVCUST_InitGet((QUERY_TYPE_T)(eNANDFlashPartEncypt0 + i));
#else
        u4Data = 0;
#endif
    }
#ifdef CC_PARTITION_WP_SUPPORT
    else if(type == 4)
    {
        u4Data = DRVCUST_InitGet((QUERY_TYPE_T)(eNANDFlashPartWp0 + i));
    }
#endif
    else
    {
        ASSERT(0);
        return -1;
    }

    *pData = u4Data;
    return 0;
}

int DRVCUST_QueryPart64(int i, u64 *pData, int type)
{
    u64 u8Data = 0;

    if(type == 1)
    {
#ifdef CC_USE_2TB_EMMC
        u8Data = DRVCUST_InitGet64((QUERY_TYPE_T)(eNANDFlashPartSize0 + i));
#else
        u8Data = DRVCUST_InitGet((QUERY_TYPE_T)(eNANDFlashPartSize0 + i));
#endif
    }
    else
    {
        ASSERT(0);
        return -1;
    }

    *pData = u8Data;
    return 0;
}

U_BOOT_CMD(
	load,	CFG_MAXARGS,	1,	do_load,
	"load     - load configuration from an environment variable\n",
	"var [...]\n"
	"    - load the configuration in the environment variable(s) 'var'\n"
);

#if 0
INT32 EEPROM_Erase()
{

    UINT64 i4Size = 0; 
    UINT64 i = 0;
    UINT32 u4Mem = 0xffffffff;
    
    EEPROM_Init();
    i4Size = EEPROM_GetSize();

    for(i = 0; i < i4Size; i+=4 ) 
    {
        EEPROM_Write(i, &u4Mem, sizeof(UINT32));
    }

    printf("EEPROM erase ");
    return (INT32)i4Size;
}


int do_eeprom ( cmd_tbl_t * cmdtp, int flag, int argc, char *argv[])
{
	const char *const fmt =
		"\nEEPROM @0x%lX %s: addr %08lx  off %04lx  count %ld ... ";

    if (argc == 2) {

        if (strcmp (argv[1], "erase") == 0) {
			int rcode;

            rcode = EEPROM_Erase();

			puts ("done\n");
			return rcode;
		}
    }
	else if (argc == 3) {
		ulong dev_addr = CFG_DEF_EEPROM_ADDR;
		UINT64 off  = simple_strtoul (argv[2], NULL, 16);
        UINT32 data;
		if (strcmp (argv[1], "rb") == 0) {
			int rcode;

            rcode = EEPROM_Read(off, &data, 1);

            printf("0x%x\n", (UINT8)data );
            
			puts ("done\n");
			return rcode;
		} 
	}
	else if (argc == 4) {	
		if (strcmp (argv[1], "wb") == 0) {
			int rcode;
    		UINT64 off  = simple_strtoul (argv[2], NULL, 16);
            UINT32 addr = simple_strtoul (argv[3], NULL, 16);;
			rcode = EEPROM_Write (off, &addr, 1);

			puts ("done\n");
			return rcode;
		} 
				
	}    
	else if (argc == 5) {
		ulong dev_addr = CFG_DEF_EEPROM_ADDR;
		ulong addr = simple_strtoul (argv[2], NULL, 16);
		UINT64 off  = simple_strtoul (argv[3], NULL, 16);
		ulong cnt  = simple_strtoul (argv[4], NULL, 16);

		if (strcmp (argv[1], "read") == 0) {
			int rcode;
            int i;
            
			printf (fmt, dev_addr, argv[1], addr, off, cnt);
 
            rcode = EEPROM_Read(off, addr, cnt);

            for(i = 0; i < cnt; i++)
            {
                if(i % 16 == 0) 
                {
                    printf("\n");
                }
                printf("0x%02x ", *(UINT8 *)(addr+i));

            }
            
			puts ("\ndone\n");
			return rcode;
		} else if (strcmp (argv[1], "write") == 0) {
			int rcode;

			printf (fmt, dev_addr, argv[1], addr, off, cnt);

			rcode = EEPROM_Write (off, addr, cnt);

			puts ("done\n");
			return rcode;
		} 
		
	}

	printf ("Usage:\n%s\n", cmdtp->usage);
	return 1;
}

U_BOOT_CMD(
	eeprom,	5,	1,	do_eeprom,
	"eeprom  - EEPROM sub-system\n",
	"read  mem_addr eeprom_offset cnt\n"
	"eeprom write mem_addr eeprom_off cnt\n"
	"eeprom rb eeprom_off\n"
	"eeprom wb eeprom_off data\n"
	"eeprom erase\n"
	"       - read/write `cnt' bytes at EEPROM offset `eeprom_offset'\n"
);
#endif

#ifndef CC_PROJECT_X
// TODO: The following functions are locally defined, should find a better way to do this

//-----------------------------------------------------------------------------
/** Assert() is the only final function when assertion happens.
 *  @param szExpress the expression of the assertion.
 *  @param szFile the file name of the assertion.
 *  @param i4Line the line number of the assertion.
 */
//-----------------------------------------------------------------------------
void Assert(const CHAR* szExpress, const CHAR* szFile, INT32 i4Line)
{
    // Show assertion messages
    Printf("\nAssertion fails at:\nFile: %s, line %d\n", szFile, (int)i4Line);
    Printf("\n\t%s", szExpress);
    Printf("\n\nProgram terminated.\n");

    // Halt the whole system
    while (1) ; 
}

//-----------------------------------------------------------------------------
/** HAL_GetRawTime() Get system ticks and clock cycles since startup
 *  @param pRawTime [out] - A pointer to RAW_TIME_T to receive raw time
 */
//-----------------------------------------------------------------------------
void HAL_GetRawTime(HAL_RAW_TIME_T* pRawTime)
{
    UINT32 u4NewHigh, u4OldHigh, u4Low;

    ASSERT(pRawTime != NULL);

    u4OldHigh = BIM_READ32(TIMER_HIGH_REG);
    u4Low = BIM_READ32(TIMER_LOW_REG);
    u4NewHigh = BIM_READ32(TIMER_HIGH_REG);

    if (u4OldHigh != u4NewHigh)
    {
        pRawTime->u4Ticks = ~((u4Low < 0x10000000) ? u4OldHigh : u4NewHigh);
    }
    else
    {
        pRawTime->u4Ticks = ~u4OldHigh;
    }
    pRawTime->u4Cycles = ~u4Low;
}

//-----------------------------------------------------------------------------
/** HAL_RawToTime() Convert RAW_TIME_T to TIME_T
 *  @param pRawTime [in]  - Pointer to RAW_TIME_T, source
 *  @param pTime    [out] - Pointer to TIME_T, destination
 */
//-----------------------------------------------------------------------------
void HAL_RawToTime(const HAL_RAW_TIME_T* pRawTime, HAL_TIME_T* pTime)
{
    if ((pRawTime != NULL) && (pTime != NULL))
    {
        UINT64 u8Cycles;

        u8Cycles = pRawTime->u4Ticks;
        u8Cycles = u8Cycles << 32;
        u8Cycles += pRawTime->u4Cycles;
        pTime->u4Seconds = (UINT32)(u8Cycles / (UINT64)systemclk);
        pTime->u4Micros = (UINT32)(((u8Cycles % (UINT64)systemclk) * 1000000L) / systemclk);
    }
}

//-----------------------------------------------------------------------------
/** HAL_GetTime() Get system time from startup
 *  @param pTime    [out] - Pointer to TIME_T to store system time
 */
//-----------------------------------------------------------------------------
void HAL_GetTime(HAL_TIME_T* pTime)
{
    HAL_RAW_TIME_T rt;

    HAL_GetRawTime(&rt);
    HAL_RawToTime(&rt, pTime);
}

//-----------------------------------------------------------------------------
/** HAL_GetDeltaTime() Get delta time of two time stamps
 *  @param pResult  [out] - The result
 *  @param pOlder   [in]  - The older time
 *  @param pNewer   [in]  - The newer time
 */
//-----------------------------------------------------------------------------
void HAL_GetDeltaTime(HAL_TIME_T* pResult, HAL_TIME_T* pT0,
    HAL_TIME_T* pT1)
{
    HAL_TIME_T* pNewer;
    HAL_TIME_T* pOlder;

    ASSERT(pResult != NULL);
    ASSERT(pT0 != NULL);
    ASSERT(pT1 != NULL);
    ASSERT((pT0->u4Micros < 1000000) && (pT1->u4Micros < 1000000));

    // Decide which one is newer
    if ((pT0->u4Seconds > pT1->u4Seconds) ||
        ((pT0->u4Seconds == pT1->u4Seconds) &&
        (pT0->u4Micros > pT1->u4Micros)))
    {
        pNewer = pT0;
        pOlder = pT1;
    }
    else
    {
        pNewer = pT1;
        pOlder = pT0;
    }

    if (pNewer->u4Seconds > pOlder->u4Seconds)
    {
        pResult->u4Seconds = pNewer->u4Seconds - pOlder->u4Seconds;
        if (pNewer->u4Micros >= pOlder->u4Micros)
        {
            pResult->u4Micros = pNewer->u4Micros - pOlder->u4Micros;
        }
        else
        {
            pResult->u4Micros = (1000000 + pNewer->u4Micros)
                - pOlder->u4Micros;
            pResult->u4Seconds--;
        }
    }
    else
    {
        // pNewer->u4Secons == pOlder->u4Seconds
        ASSERT(pNewer->u4Micros >= pOlder->u4Micros);
        pResult->u4Seconds = 0;
        pResult->u4Micros = pNewer->u4Micros - pOlder->u4Micros;
    }
}

//-----------------------------------------------------------------------------
/** HAL_GetDeltaTime() Get delta time of two time stamps
 *  @param pResult  [out] - The result
 *  @param pOlder   [in]  - The older time
 *  @param pNewer   [in]  - The newer time
 */
//-----------------------------------------------------------------------------
void HAL_GetDeltaRawTime(HAL_RAW_TIME_T* pResult, const HAL_RAW_TIME_T* pT0,
    const HAL_RAW_TIME_T* pT1)
{
    /*
    HAL_RAW_TIME_T* pNewer;
    HAL_RAW_TIME_T* pOlder;
    */
    UINT64 u8New;
    UINT64 u8Old;
    UINT64 u8Result;
	
    ASSERT(pResult != NULL);
    ASSERT(pT0 != NULL);
    ASSERT(pT1 != NULL);
    
    u8New     = ((UINT64)pT1->u4Ticks << 32) | pT1->u4Cycles;
    u8Old     = ((UINT64)pT0->u4Ticks << 32) | pT0->u4Cycles;
    u8Result  = u8New - u8Old;
    pResult->u4Ticks  = u8Result >> 32;
    pResult->u4Cycles = u8Result & 0xffffffff;
    
}

//-----------------------------------------------------------------------------
/** HAL_Delay_us() delay X micro seconds
 *  @param u4Micro
 */
//-----------------------------------------------------------------------------
void HAL_Delay_us(UINT32 u4Micros)
{
    HAL_TIME_T rOrgTime, rNewTime, rDiffTime;

    // This function cannot delay more than 10 seconds.
    ASSERT(u4Micros < 10000000);

    if (u4Micros > 10000000)
    {
        u4Micros = 10000000;
    }

    HAL_GetTime(&rOrgTime);
    do {
        HAL_GetTime(&rNewTime);
        HAL_GetDeltaTime(&rDiffTime, &rOrgTime, &rNewTime);
    } while (((1000000*rDiffTime.u4Seconds)+rDiffTime.u4Micros) < u4Micros);
}

UINT32 HalCriticalStart(void)
{
    return 0;
}

void HalCriticalEnd(UINT32 u4Flags)
{
    UNUSED(u4Flags);
}

//-----------------------------------------------------------------------------
/** _BimIsVectorValid() to check the vector value is valid.
 *  @param u4Vector the checking vector value.
 */
//-----------------------------------------------------------------------------
static BOOL _BimIsVectorValid(UINT32 u4Vector)
{
    return (u4Vector <= MAX_IRQ_VECTOR);
}

//-----------------------------------------------------------------------------
/** BIM_EnableIrq() Enable the interrupt of a given vector
 *  @param u4Vector: The vector
 *  @retval TRUE: Succeed
 *  @retval FALSE: Fail
 */
//-----------------------------------------------------------------------------
BOOL BIM_EnableIrq(UINT32 u4Vector)
{
    UINT32 u4Irq;

    if (!_BimIsVectorValid(u4Vector))
    {
        return FALSE;
    }

    if (u4Vector > VECTOR_MISC)
    {
        u4Irq = _MISCIRQ(u4Vector);
        BIM_REG32(REG_MISC_IRQEN) |= u4Irq;
        return TRUE;
    }
    u4Irq = _IRQ(u4Vector);

    // Should be in critical section to avoid race condition
    BIM_REG32(REG_IRQEN) |= u4Irq;
    return TRUE;
}

//-----------------------------------------------------------------------------
/** BIM_DisableIrq() Disable the interrupt of a given vector
 *  @param u4Vector: The vector
 *  @retval TRUE: Succeed
 *  @retval FALSE: Fail
 */
//-----------------------------------------------------------------------------
BOOL BIM_DisableIrq(UINT32 u4Vector)
{
    UINT32 u4Irq;

    if (!_BimIsVectorValid(u4Vector))
    {
        return FALSE;
    }

    if (u4Vector > VECTOR_MISC)
    {
        u4Irq = _MISCIRQ(u4Vector);
        BIM_REG32(REG_MISC_IRQEN) &= ~u4Irq;
        return TRUE;
    }

    u4Irq = _IRQ(u4Vector);

    // Should be in critical section to avoid race condition
    BIM_REG32(REG_IRQEN) &= ~u4Irq;
    return TRUE;
}

//-----------------------------------------------------------------------------
/** BIM_ClearIrq() Clear the pending interrupt of a given vector
 *  @param u4Vector: The vector
 *  @retval TRUE: Succeed
 *  @retval FALSE: Fail
 */
//-----------------------------------------------------------------------------
BOOL BIM_ClearIrq(UINT32 u4Vector)
{
    UINT32 u4Irq;

    if (!_BimIsVectorValid(u4Vector))
    {
        return FALSE;
    }

    if (u4Vector > VECTOR_MISC)
    {
        u4Irq = _MISCIRQ(u4Vector);
        BIM_WRITE32(REG_MISC_IRQEN, u4Irq);
        return TRUE;
    }

    u4Irq = _IRQ(u4Vector);
    BIM_WRITE32(REG_IRQCL, u4Irq);
    return TRUE;
}

//-----------------------------------------------------------------------------
/** BSP_GetIcVersion() Get IC version
 *  @return The IC version, or IC_VER_FPGA if running in a FPGA platform, or
 *      IC_VER_UNKOWN for unknown IC version
 */
//-----------------------------------------------------------------------------
IC_VERSION_T BSP_GetIcVersion(void)
{
#ifndef CONFIG_NAND_BOOT
    IC_VERSION_T eVer = IC_VER_FPGA;

    if (!BSP_IsFPGA())
    {
#endif /* CONFIG_NAND_BOOT */
        UINT32 u4Version;

        // u4Version = *(UINT32*)REG_IC_VERSION;
        u4Version = CKGEN_READ32(REG_RO_SW_ID);

        return (IC_VERSION_T)u4Version;
#ifndef CONFIG_NAND_BOOT
    }

    return eVer;
#endif /* CONFIG_NAND_BOOT */
}

BOOL BSP_IsFPGA(void)
{
    return 0;
}
/*
UINT32 BSP_IsSystemAcOn()
{
    return IS_SYS_ACON();
}
*/
//-----------------------------------------------------------------------------
/** DDR_SetAgentPriority() Set dram group 1 agent priorities
 *  @param pu1Priorities   Priorities of aud, dmx, fci, vbi, osd, pscan,
 *                         b2r, cpu, scpos
 *  @retval none.
 */
//-----------------------------------------------------------------------------
void DDR_SetAgentPriority(const UINT8 *pu1Prio)
{
    UINT64 u8Priority = 0;
    INT32 i;
    UINT32 u4RegVal;
    UINT8 u1Prio;

    ASSERT(pu1Prio != NULL);

    for (i = 0; i < 12; i++)
    {
        u1Prio = pu1Prio[i];
        // '1' ~ '9' ==> Pri 1 ~ 9
        if ((u1Prio <= (UINT8)'9') && (u1Prio >= (UINT8)'0'))
        {
            u1Prio -= (UINT8)'0';
        }
        // 'A' ==> Pri 10
        else if ((u1Prio == (UINT8)'A') || (u1Prio == (UINT8)'a'))
        {
            u1Prio = 10;
        }
        // 'B' ==> Pri 11
        else if ((u1Prio == (UINT8)'B') || (u1Prio == (UINT8)'b'))
        {
            u1Prio = 11;
        }
        // 'C' ==> Pri 12
        else if ((u1Prio == (UINT8)'C') || (u1Prio == (UINT8)'c'))
        {
            u1Prio = 12;
        }
        else if (i == 9)    // backward compatible, for only 9 agents
        {
            u1Prio = 9;
        }
        else
        {
            ASSERT(u1Prio > 11);
            return;
        }

        u8Priority |= (((UINT64)u1Prio) << (4 * i));
    }

    DRAM_WRITE32(0x4C, (UINT32)(u8Priority & 0xffffffff));

    u4RegVal = DRAM_READ32(0x50);
    u4RegVal &= 0xffffff00;
    u4RegVal |= ((UINT32)(u8Priority >> 32) & 0x000000ff);
    DRAM_WRITE32(0x50, u4RegVal);

    u4RegVal = DRAM_READ32(0xf8);
    u4RegVal &= 0xff00ffff;
    u4RegVal |= ((UINT32)(u8Priority >> 24) & 0x00ff0000);
    DRAM_WRITE32(0xf8, u4RegVal);
}

//-----------------------------------------------------------------------------
/** DDR_SetBurstLen() Set dram group 1 agent burst length
 *  @param u4BurstLen1     Burst length of agent 1 ~ 8
 *  @param u4BurstLen2     Burst length of agent 9 ~ 16
 *  @retval none.
 */
//-----------------------------------------------------------------------------
void DDR_SetBurstLen(UINT32 u4BurstLen1, UINT32 u4BurstLen2)
{
    DRAM_WRITE32(0xCC, u4BurstLen1);
    DRAM_WRITE32(0xD0, u4BurstLen2);
}

//-----------------------------------------------------------------------------
/** DDR_SetArbitorTime() Set dram group arbiter time
 *  @param u1Group         Dram agent group (1,2,3)
 *  @param u1Time          Arbitor time (0 ~ 15)
 *  @retval none.
 */
//-----------------------------------------------------------------------------
void DDR_SetArbiterTime(UINT8 u1Group, UINT8 u1Time)
{
    UINT32 u4RegVal;

    if (u1Time > 15)
    {
        u1Time = 15;
    }

    u4RegVal = DRAM_READ32(0x50);
    switch(u1Group)
    {
    case 1:
        u4RegVal &= 0xff0fffff;
        u4RegVal |= (((UINT32)u1Time) << 20);
        break;
    case 2:
        u4RegVal &= 0xf0ffffff;
        u4RegVal |= (((UINT32)u1Time) << 24);
        break;
    case 3:
        u4RegVal &= 0x0fffffff;
        u4RegVal |= (((UINT32)u1Time) << 28);
        break;
    default:
        return;
    }
    DRAM_WRITE32(0x50, u4RegVal);
}

void __inline__ x_thread_delay(UINT32 u4Ms)
{
    HAL_Delay_us(u4Ms*1000);
}

void *x_mem_alloc(SIZE_T z_size)
{
    return malloc(z_size);
}

void x_mem_free(SIZE_T z_size)
{
    return;
}

UINT32 x_crit_start(void)
{
    return 0;
}

void x_crit_end(UINT32 state)
{
    UNUSED(state);
}

/* Replacement (=dummy) for GNU/Linux undefined reference */
void raise(void)
{
    return;
}

void __aeabi_unwind_cpp_pr0(void)
{
    return;
}

PFN_IRQ_HANDLER RegisterIrqHandler(PFN_IRQ_HANDLER pfnNewHandler)
{
    return (PFN_IRQ_HANDLER)0;
}

void HalEnableIRQ(void) 
{
    return;
}

BOOL BIM_IsIrqPending(UINT32 u4Vector) 
{
    return TRUE;
}

void vInternalEDIDInit(void)
{
}

INT32 Loader_ReadNandFlash(UINT32 u4PartId, UINT32 u4Offset, void *pvBuf, UINT32 u4BufSize)
{
    return 0;    
}

#ifdef CONFIG_USB_FASTBOOT

INT32 BIM_CheckFastboot(void)
{
    misc_message *pmisc_msg;
    pmisc_msg = malloc(sizeof(misc_message));

    // check if out-of-memory
    ASSERT(pmisc_msg);
    memset(pmisc_msg, 0, sizeof(misc_message));


#ifdef CC_EMMC_BOOT
    extern int emmc_read_from_partition(unsigned char* buf, char* pname, u64 offset, unsigned int size);
    emmc_read_from_partition((unsigned char*)pmisc_msg, "misc", 512, sizeof(misc_message));
#else
    extern int nand_read_from_partition(unsigned char* buf, char* pname, unsigned int offset, unsigned int size);
    nand_read_from_partition((unsigned char*)pmisc_msg, "misc", 512, sizeof((misc_message)));
#endif
    Printf("\npmisc_msg: %s\n", pmisc_msg->command);

    if(strcmp(pmisc_msg->command, "fastboot")==0)
    {
        free(pmisc_msg);
        return 1;
    }

    free(pmisc_msg);

    return 0;
}
#endif
INT32 BIM_CheckMisc(void)
{
    misc_message *pmisc_msg;
    pmisc_msg = malloc(sizeof(misc_message));

    // check if out-of-memory
    ASSERT(pmisc_msg);
    memset(pmisc_msg, 0, sizeof(misc_message));

#ifdef CC_EMMC_BOOT
    extern int emmc_read_from_partition(unsigned char* buf, char* pname, u64 offset, unsigned int size);
    emmc_read_from_partition((unsigned char*)pmisc_msg, "misc", 0, sizeof(misc_message));
#else
    extern int nand_read_from_partition(unsigned char* buf, char* pname, unsigned int offset, unsigned int size);
    nand_read_from_partition((unsigned char*)pmisc_msg, "misc", 0, sizeof(misc_message));
#endif

    if(strcmp(pmisc_msg->command, "boot-recovery")==0)
    {
        free(pmisc_msg);
        return 1;
    }

    free(pmisc_msg);

    return 0;
}

INT32 BIM_IsRecoveryMode(void)
{
	#if 0	
    extern INT32 BIM_CheckEepromRecovery(void);
    if (BIM_CheckEepromRecovery())
        return 1;

    if (GPIO_GetIn(LOADER_RECOVERY_GPIO_1) && GPIO_GetIn(LOADER_RECOVERY_GPIO_2))
    {
        return 1;
    }
  #endif
  
    return BIM_CheckMisc();
}

#ifdef USING_EMMC_STORAGE_BOOTFLAG

#define EMMC_STORAGE_BOOTFLAG_PARTID    22
#define EMMC_STORAGE_BOOTFLAG_PARTNAME  "hiscfg"
#define EMMC_STORAGE_BOOTFLAG_OFFSET    11

UINT8 DecideBootPartition(void)
{
    UINT8 buffer[2048],ret;
    UINT8 cNum = 0;

    memset(buffer, 0, sizeof(buffer));
    extern int emmc_read_from_partition(unsigned char* buf, char* pname, u64 offset, unsigned int size);
    ret = emmc_read_from_partition((unsigned char*)buffer,
            EMMC_STORAGE_BOOTFLAG_PARTNAME, 0, 512);
    if(ret)
    {
        Printf("\n %s(%d)  read emmc failed  \n",__FUNCTION__, __LINE__);
    }

    Printf("%s(%d)  Uboot Channel: %d, buffer[%d]=%d in uboot \n",
            __FUNCTION__, __LINE__, EMMC_STORAGE_BOOTFLAG_PARTID,
            EMMC_STORAGE_BOOTFLAG_OFFSET, buffer[EMMC_STORAGE_BOOTFLAG_OFFSET]);
    return buffer[EMMC_STORAGE_BOOTFLAG_OFFSET] == 0 ? 0:1;
}
#endif

#endif
