/*
 * u-boot-2011.12/include/configs/mt5396.h
 *
 * Copyright (c) 2010-2012 MediaTek Inc.
 *	This file is based  ARM Realview platform
 *  Copyright (C) 2002 ARM Ltd.
 *  All Rights Reserved
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
 */

#ifndef __CONFIG_H
#define __CONFIG_H

// ANDROID uboot define
#ifdef ANDROID
    #define CONFIG_ANDROID 1
    #ifndef GTV
    #define CONFIG_ANDROID_BOOT 1 // boot with ramdisk
    #else
    #define CONFIG_ANDROID_BOOT 0 // boot w/o ramdisk
    #endif
#else
    #define CONFIG_ANDROID 0
    #define CONFIG_ANDROID_BOOT 0 // MTK turnkey boot
#endif

#include "c_model.h"

#define CONFIG_ARCH_MT5396 1
#define CONFIG_CM1176JZ_S 0 /* CPU core is ARM1176JZ-S */
#define CONFIG_CMCA9 1 /* CPU core is CA9 */


#define CONFIG_FAST_BOOT 1
#define CONFIG_ENABLE_MMU 1
#ifndef CONFIG_LOG_SILENT
#define CONFIG_LOG_SILENT 0
#endif
#ifndef CONFIG_NO_CONSOLE
#define CONFIG_NO_CONSOLE 0
#endif
#ifndef CONFIG_LOAD_MAC_ADDRESS
#define CONFIG_LOAD_MAC_ADDRESS 0
#endif

/*
 * High Level Configuration Options
 * (easy to change)
 */
#define CFG_MEMTEST_START   0x100000
#define CFG_MEMTEST_END     0x10000000
#define CFG_HZ              1000        // ticks every 1ms
#define CFG_HZ_CLOCK        100000000   /* Timer 1 is clocked at 100Mhz */
#define CFG_CLOCK_PER_TICKS (CFG_HZ_CLOCK / CFG_HZ)
#define CFG_HZ_PER_USEC     (CFG_HZ_CLOCK / 1000000)

#define CONFIG_CMDLINE_TAG  1   /* enable passing of ATAGs  */
#define CONFIG_INITRD_TAG   1
#define CONFIG_SETUP_MEMORY_TAGS    1
#define CONFIG_MISC_INIT_R  1   /* call misc_init_r during start up */
#ifndef CC_KERNEL_ON_NOR
    #if !CONFIG_ANDROID
        #define CONFIG_DUAL_BANK_ROOTFS     1
        #define CONFIG_DUAL_BANK_KERNEL     1
    #endif
#endif

#undef CONFIG_INIT_CRITICAL
#define CONFIG_CM_INIT      1
#define CONFIG_CM_REMAP     1
#undef CONFIG_CM_SPD_DETECT

/* Allow serial number (serial#) and MAC address (ethaddr) to be overwritten */
#define CONFIG_ENV_OVERWRITE

/*
 * Select serial console configuration
 */
#define CONFIG_MT53XX_SERIAL
#define CONFIG_CONS_INDEX   0
#define CONFIG_BAUDRATE     115200
#define CFG_BAUDRATE_TABLE  { 9600, 19200, 38400, 57600, 115200 }

/*
 * Select ethernet configuration
 */


/*
 * Command line configuration.
 */
#if !(CONFIG_NO_CONSOLE)
#define CONFIG_CMD_IMI
#define CONFIG_CMD_BDI
#define CONFIG_CMD_MEMORY
#define CONFIG_CMD_FLASH
#define CONFIG_CMD_NET
#define CONFIG_CMD_PING
#define CONFIG_CMD_DHCP
#define CONFIG_CMD_FAT
#define CONFIG_CMD_USB
#endif

#define CONFIG_CMD_ENV

#if defined(CC_EMMC_BOOT)
    #define CONFIG_CMD_EMMC
    #define CONFIG_CMD_JFFS2
#elif !defined(CC_KERNEL_ON_NOR)
    #define CONFIG_CMD_NAND
    #define CONFIG_CMD_JFFS2
    #define CONFIG_JFFS2_NAND
    #define CONFIG_CMD_UBI
#endif

#define CONFIG_BOOTDELAY    0

#if defined(CC_EMMC_BOOT)
    #define CONFIG_BOOTARGS_BASE    "root=/dev/mmcblk0p7 rootwait lpj=4571136 console=ttyMT0,115200n1"
    #define CONFIG_BOOTARGS_BASE2   "root=/dev/mmcblk0p8 rootwait lpj=4571136 console=ttyMT0,115200n1"
    #if CONFIG_ANDROID_BOOT
	    #define CONFIG_BOOTARGS_BASE_ANDROID  "lpj=4571136 console=ttyMT0,115200n1 root=/dev/mmcblk0p5 vmalloc=380M"
    #else
        #define CONFIG_BOOTARGS_BASE_ANDROID  "lpj=4571136 console=ttyMT0,115200n1 ubi.mtd=6 root=ubi0:rootfs" \
                                              " rootfstype=ubifs init=/init vmalloc=380MB"
    #endif //CONFIG_ANDROID_BOOT

#elif !defined(CC_KERNEL_ON_NOR)
    #define CONFIG_BOOTARGS_BASE    "root=/dev/mtdblock6 rootwait lpj=4571136 console=ttyMT0,115200n1"
    #define CONFIG_BOOTARGS_BASE2   "root=/dev/mtdblock7 rootwait lpj=4571136 console=ttyMT0,115200n1"
    #if CONFIG_ANDROID_BOOT
        #define CONFIG_BOOTARGS_BASE_ANDROID  "lpj=4571136 console=ttyMT0,115200n1 root=/dev/ram vmalloc=380M"
    #else
    #define CONFIG_BOOTARGS_BASE_ANDROID  "lpj=4571136 console=ttyMT0,115200n1 ubi.mtd=6 root=ubi0:rootfs" \
                                          " rootfstype=ubifs init=/init vmalloc=380MB"
    #endif //CONFIG_ANDROID_BOOT
#else
    #define CONFIG_BOOTARGS_BASE    "root=/dev/ram lpj=4571136 console=ttyMT0,115200n1"
#endif

#if CONFIG_ANDROID
    #ifndef CC_UBOOT_QUIET_BOOT
        #define CONFIG_BOOTARGS         CONFIG_BOOTARGS_BASE_ANDROID
    #else
        #define CONFIG_BOOTARGS         CONFIG_BOOTARGS_BASE_ANDROID" quiet"
    #endif
#else
    #ifndef CONFIG_BOOTARGS
        #ifndef CC_UBOOT_QUIET_BOOT
            #define CONFIG_BOOTARGS         CONFIG_BOOTARGS_BASE
            #define CONFIG_BOOTARGS2        CONFIG_BOOTARGS_BASE2
        #else
            #define CONFIG_BOOTARGS         CONFIG_BOOTARGS_BASE" quiet"
            #define CONFIG_BOOTARGS2        CONFIG_BOOTARGS_BASE2" quiet"
        #endif
    #endif
#endif

#define CONFIG_BOOTCOMMAND2     "nboot 0x2000000 0 0x700000 0x0"

#define CONFIG_ZERO_BOOTDELAY_CHECK /* check keypress when bootdelay = 0 */
#define CONFIG_SILENT_CONSOLE   /* enable silent startup support    */

/*
 * BOOTP options
 */
#define CONFIG_BOOTP_SUBNETMASK
#define CONFIG_BOOTP_GATEWAY
#define CONFIG_BOOTP_HOSTNAME
#define CONFIG_BOOTP_BOOTPATH

/*
 * Miscellaneous configurable options
 */
#define CFG_LONGHELP    /* undef to save memory     */
#define CFG_PROMPT  "mt5396 # " /* Monitor Command Prompt   */
#define CFG_CBSIZE  2048        /* Console I/O Buffer Size  */

/* Print Buffer Size */
#define CFG_PBSIZE  (CFG_CBSIZE+sizeof(CFG_PROMPT)+16)
#define CFG_MAXARGS 16      /* max number of command args   */
#define CFG_BARGSIZE    CFG_CBSIZE  /* Boot Argument Buffer Size    */

#undef  CFG_CLKS_IN_HZ      /* everything, incl board info, in Hz */
#define CFG_LOAD_ADDR   (0x1f00000)     /* default load address */
#define CFG_RD_ADDR     (0x4000000)     /* fixed ramdisk load addr */
#define CFG_RD_SRC_ADDR (0x2000000)     /* For unlzo_read */

/*-----------------------------------------------------------------------
 * Stack sizes
 *
 * The stack sizes are set up in start.S using the settings below
 */
#define CONFIG_STACKSIZE    (128*1024)  /* regular stack */
#ifdef CONFIG_USE_IRQ
#define CONFIG_STACKSIZE_IRQ    (4*1024)    /* IRQ stack */
#define CONFIG_STACKSIZE_FIQ    (4*1024)    /* FIQ stack */
#endif

/*-----------------------------------------------------------------------
 * Physical Memory Map
 */

#include "c_model.h"
#include "drv_default.h"

#if defined(CC_UBOOT_TWO_BANK)
#define CONFIG_NR_DRAM_BANKS    2   /* we have 2 bank of DRAM */
#else
#define CONFIG_NR_DRAM_BANKS    1   /* we have 1 bank of DRAM */
#endif
#define PHYS_SDRAM_1        0x00000000  /* SDRAM Bank #1 */
#define PHYS_SDRAM_1_SIZE   LINUX_KERNEL_MEM_SIZE

#if CONFIG_NR_DRAM_BANKS>1
#define PHYS_SDRAM_2        TOTAL_MEM_SIZE  /* SDRAM Bank #2 */
#define PHYS_SDRAM_2_SIZE   (TOTAL_MEM_SIZE-0x7800000)
#endif

#define CFG_FLASH_BASE      0xf8000000

#ifndef GPU_MEM_TOTAL_SIZE
#define GPU_MEM_TOTAL_SIZE (0)
#define GPU_ION_USING_SIZE (0)
#endif
#define PHYS_GPU_START   ((PHYS_SDRAM_1_SIZE > GPU_MEM_TOTAL_SIZE)? (PHYS_SDRAM_1_SIZE - GPU_MEM_TOTAL_SIZE):0)
#define PHYS_GPU_SIZE    ((PHYS_SDRAM_1_SIZE > GPU_MEM_TOTAL_SIZE)?(GPU_MEM_TOTAL_SIZE):0)
#define PHYS_GPU_ION_SIZE (GPU_ION_USING_SIZE)

/*-----------------------------------------------------------------------
 * FLASH and environment organization
 */
#if defined(CC_EMMC_BOOT)
    #define CONFIG_EMMC_BOOT
#elif !defined( CC_KERNEL_ON_NOR)
    #define CONFIG_NAND_BOOT
#endif

#if defined(CONFIG_NAND_BOOT)
    #define CFG_ENV_IS_IN_NAND
#elif defined(CONFIG_EMMC_BOOT)
    #define CFG_ENV_IS_IN_EMMC
    #define CONFIG_SYS_MMC_ENV_DEV  0
#endif

#if defined(CONFIG_NAND_BOOT) || defined(CONFIG_EMMC_BOOT)
    #ifndef CFG_ENV_OFFSET
        #ifdef NAND_PART_OFFSET_1
            #define CFG_ENV_OFFSET      NAND_PART_OFFSET_1
        #else
            #define CFG_ENV_OFFSET      0x200000     // offset: 2MB
        #endif
    #endif

    #ifndef CFG_ENV_SIZE
        #define CFG_ENV_SIZE        0x20000     // 128KB
    #endif
#else // CONFIG_NAND_BOOT/CONFIG_EMMC_BOOT
    #define CFG_ENV_IS_IN_FLASH
    #define CONFIG_INFERNO
    #define CFG_ENV_ADDR        0xf8080000
    #define CFG_ENV_SIZE        0x10000     // 64KB
#endif

//#define PHYS_FLASH_SIZE       0x01000000  /* 16MB */
#define CFG_MAX_FLASH_BANKS 1       /* max number of memory banks */
#define PHYS_FLASH_1        (CFG_FLASH_BASE)

/* timeout values are in ticks */
#define CFG_FLASH_ERASE_TOUT    (2*CFG_HZ)  /* Timeout for Flash Erase */
#define CFG_FLASH_WRITE_TOUT    (2*CFG_HZ)  /* Timeout for Flash Write */
#define CFG_MAX_FLASH_SECT      256

#ifndef CC_KERNEL_ON_NOR
    #define CONFIG_JFFS2_CMDLINE
    
    #if defined(CONFIG_NAND_BOOT)
        #define MTDIDS_DEFAULT      "nand0=mt53xx-nand"
    #else
        #define MTDIDS_DEFAULT      "emmc0=mt53xx-emmc"
    #endif

    #if (CONFIG_DUAL_BANK_ROOTFS)
        #if defined(CONFIG_NAND_BOOT) // nand partition index start from 0
            #ifndef MTDPARTS_NUM_OF_ROOTFS_A
                #define MTDPARTS_NUM_OF_ROOTFS_A    '6'
            #endif
            #ifndef MTDPARTS_NUM_OF_ROOTFS_B
                #define MTDPARTS_NUM_OF_ROOTFS_B    '7'
            #endif
        #else // mmc partition index start from 1
            #ifndef MTDPARTS_NUM_OF_ROOTFS_A
                #define MTDPARTS_NUM_OF_ROOTFS_A    '7'
            #endif
            #ifndef MTDPARTS_NUM_OF_ROOTFS_B
                #define MTDPARTS_NUM_OF_ROOTFS_B    '8'
            #endif
        #endif
    #endif

    #if defined(CC_FAST_INIT) && !defined(MTDPARTS_NUM_OF_SWAP)
        #define MTDPARTS_NUM_OF_SWAP 11
    #endif

#endif

/*-----------------------------------------------------------------------
 * Boot env configurations
 */
#define RAMDISK_ENV \
    "ramdisk_env=setenv bootargs root=/dev/ram console=ttyMT0,115200n1 mtdparts=${mtdparts};" \
    "setenv bootcmd nboot.jffs2 kernel rootfs; setenv autostart yes; setenv ramdisk yes; saveenv\0"

#define USBFILE_ENV \
    "usbfile_env=setenv bootargs root=/dev/ram console=ttyMT0,115200n1 mtdparts=${mtdparts}\0"
    
#define RAMDISK_ENV_DUAL \
    "ramdisk_env=setenv bootargs root=/dev/ram console=ttyMT0,115200n1 mtdparts=${mtdparts};" \
    "setenv bootcmd nboot.jffs2 kernelA rootfsA; setenv autostart yes; setenv ramdisk yes; saveenv\0"

#define NANDFS_ENV \
    "nandfs_env=setenv bootargs root=/dev/mtdblock3 console=ttyMT0,115200n1 mtdparts=${mtdparts};" \
    "setenv bootcmd nboot.jffs2 kernel; setenv autostart yes; saveenv\0"

#define NANDFS_ENV_DUAL \
    "nandfs_env=setenv bootargs root=/dev/mtdblock5 console=ttyMT0,115200n1 mtdparts=${mtdparts};" \
    "setenv bootcmd nboot.jffs2 kernelA; setenv autostart yes; saveenv\0"

#define NFS_ENV \
    "serverip= \0" \
    "rootpath= \0" \
    "nfs_env=setenv bootargs root=/dev/nfs console=ttyMT0,115200n1 nfsroot=${serverip}:${rootpath};" \
    "setenv bootcmd nboot.jffs2 kernel; setenv autostart yes; saveenv\0"

#define TFTP_ENV \
    "tftp_env=load nfs_env; setenv bootcmd tftpboot ${loadaddr} ${serverip}:uImage; saveenv\0"

#define AUTO_START_ENV \
    "autostart=yes\0"

#if defined(CC_EMMC_BOOT)
    #define RECOVERY_ENV "recovery_cmd=eboot recovery\0"
#else
#define RECOVERY_ENV "recovery_cmd=nboot recovery\0"
#endif

#if defined(CC_EMMC_BOOT)
    #define MTDIDS_ENV \
        "mtdids=emmc0=mt53xx-emmc\0"
#else
    #define MTDIDS_ENV \
        "mtdids=nand0=mt53xx-nand\0"
#endif

#if defined(CC_EMMC_BOOT)
    #if CONFIG_ANDROID
        #if CONFIG_ANDROID_BOOT
            #define BOOTCMD_ENV "bootcmd=eboot boot\0"
        #else
            #define BOOTCMD_ENV "bootcmd=eboot.jffs2 kernel rootfs\0"
        #endif //CONFIG_ANDROID_BOOT
    #else
        #define BOOTCMD_ENV "bootcmd=eboot kernelA rootfsA\0"
    #endif
#elif !defined(CC_KERNEL_ON_NOR)
    #if CONFIG_ANDROID
    #if CONFIG_ANDROID_BOOT
        #define BOOTCMD_ENV "bootcmd=nboot boot\0"
    #else
        #define BOOTCMD_ENV "bootcmd=nboot.jffs2 kernel rootfs\0"
    #endif //CONFIG_ANDROID_BOOT
    #else
        #define BOOTCMD_ENV "bootcmd=nboot.jffs2 kernelA rootfsA\0"
    #endif
#else
    #ifndef CC_KERNEL_ON_NOR
    #define BOOTCMD_ENV \
        "bootcmd=nboot.jffs2 kernelA rootfsA\0"
    #else
    #define BOOTCMD_ENV \
        "bootcmd=bootm 0xf8090000 0x200000\0"
    #endif
#endif

#if CONFIG_ANDROID
#define MACRO_ENV \
    "boot=update boot boot.img\0" \
    "recovery=update recovery recovery.img\0" \
    "system=update system system.img\0" \
    "userdata=update userdata userdata.img\0" \
    "cache=update cache cache.img\0" \
    "all=load boot recovery userdata cache system\0"

#define ANDROID_ENV \
    "android_env=reloadenv\0"
#endif

#if defined(CC_EMMC_BOOT)
    #define PARTITION_ENV \
        "partition=emmc0,0\0"
#else
    #define PARTITION_ENV \
        "partition=nand0,0\0"
#endif

#if CONFIG_ANDROID
    #define CONFIG_EXTRA_ENV_SETTINGS RAMDISK_ENV USBFILE_ENV NANDFS_ENV AUTO_START_ENV MTDIDS_ENV BOOTCMD_ENV PARTITION_ENV ANDROID_ENV MACRO_ENV RECOVERY_ENV
#else
    #define CONFIG_EXTRA_ENV_SETTINGS RAMDISK_ENV USBFILE_ENV NANDFS_ENV AUTO_START_ENV MTDIDS_ENV BOOTCMD_ENV PARTITION_ENV
#endif

/*
 * Size of malloc() pool
 */
#if !(CONFIG_NO_CONSOLE)
#define CFG_MALLOC_LEN     (1*1024*1024)
#else
#define CFG_MALLOC_LEN     (3 * CFG_ENV_SIZE) //(1*1024*1024)//(CFG_ENV_SIZE + 128*1024)
#endif
#define CFG_GBL_DATA_SIZE      (256)   /* size in bytes reserved for initial data */

/*-----------------------------------------------------------------------
 * Net driver interface
 */
#if !(CONFIG_NO_CONSOLE)
#define CONFIG_NET_MULTI
#define CONFIG_DRIVER_RTL8150
#define CONFIG_DRIVER_AX88772
#define CONFIG_DRIVER_PEGASUSII
#define CONFIG_DRIVER_MMAC539X
#endif

/*-----------------------------------------------------------------------
 * Network setting
 */
#define CONFIG_SUPPORT_ETHERNET
//#define CONFIG_IPADDR     172.22.61.71
//#define CONFIG_GATEWAYIP    172.22.61.254
//#define CONFIG_NETMASK      255.255.255.0

//#define CONFIG_SERVERIP     172.22.61.70

//#define CONFIG_ETHADDR      00:00:4E:11:16:C6

/*-----------------------------------------------------------------------
 * TFTP load address
 */
#define CONFIG_LOADADDR 0x2000000   // TFTP store address

#define CONFIG_DOS_PARTITION
#define CONFIG_SUPPORT_VFAT

/* USB */
#define CONFIG_USB_STORAGE
#define CONFIG_USB_PL2303
#define CONFIG_USB_MTKHCD
#define LITTLEENDIAN            1       /* used by usb.h  */


//#define CONFIG_WIFI_PORT_DISABLE
#ifdef CONFIG_WIFI_PORT_DISABLE
#define CONFIG_WIFI_PORT_NUMBER    2     /*disable WIFI Port initial in boot loader */
#endif

/*
 *  Board NAND Info.
 */
#define CFG_NAND_ADDR 0x04000000  /* physical address to access nand at CS0*/
#define CFG_NAND_BASE 0x04000000

#define CFG_MAX_NAND_DEVICE 1   /* Max number of NAND devices */
#define CFG_MAX_EMMC_DEVICE 1   /* Max number of EMMC devices */

#define LARGE_NAND_PAGESIZE
#ifdef LARGE_NAND_PAGESIZE
    #define SECTORSIZE          2048
    #define ADDR_ID             1
    #define ADDR_COLUMN         2
    #define ADDR_PAGE           2
    #define ADDR_COLUMN_PAGE    4
    #define SCANBLOCK_POS       62
    #define BADBLOCK_POS        0
    #define BLOCK_SCAN_FLAG     0x5A
#else
    #define SECTORSIZE          512
    #define ADDR_ID             1
    #define ADDR_COLUMN         1
    #define ADDR_PAGE           3
    #define ADDR_COLUMN_PAGE    4
    #define BLOCK_SCAN_POS      14
    #define BADBLOCK_POS        5/*15*/
    #define BLOCK_SCAN_FLAG     0x5A
#endif

#define NAND_ChipID_UNKNOWN 0x00
#define NAND_MAX_FLOORS     1
#define NAND_MAX_CHIPS      1

#define NAND_OP_NONE                0
#define NAND_OP_READID              1
#define NAND_OP_READSTATUS          2
#define NAND_OP_READPAGE            3
#define NAND_OP_WRITEPAGE           4
#define NAND_OP_READSPARE           5
#define NAND_OP_WRITESPARE          6

#define LARGE_NAND_BOOT

/*
 * EEPROM
 */
#define CFG_I2C_EEPROM_ADDR_LEN     2
#define CFG_I2C_EEPROM_ADDR     SYSTEM_EEPROM_ADDRESS
#define CFG_EEPROM_SIZE         SYSTEM_EEPROM_SIZE      /* size in bytes            */
#define CFG_EEPROM_PAGE_WRITE_BITS  2       // (2^CFG_EEPROM_PAGE_WRITE_BITS) bytes
#define CFG_EEPROM_PAGE_WRITE_DELAY_MS 20   //delay  (CFG_EEPROM_PAGE_WRITE_DELAY_MS) ms
#define CFG_SYS_CLK (27*1000*1000)
#define CFG_BUS_CLK  (100*1000)
#define CFG_EEPROM_WREN
//#define CFG_ENV_IS_IN_EEPROM
//#define DEBUG_I2C

#endif  /* __CONFIG_H */
