/*
 * (C) Copyright 2003
 * Kyle Harris, kharris@nexus-tech.net
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <command.h>
#include <jffs2/jffs2.h>
#include <mmc.h>
#include <bootimg.h>
#ifdef CC_EEP2EMMC_DUAL_BANK_SUPPORT
#include "drvcust_if.h"
#endif

#if defined(CC_EMMC_BOOT)

#define MSDC_USE_NORMAL_READ_LZO  1

#ifdef CC_EEP2EMMC_DUAL_BANK_SUPPORT
typedef enum
{
    MSDC_EEPROM_NONE,
    MSDC_EEPROM_PARTA,
    MSDC_EEPROM_PARTB,
} MSDC_EEPROM_PART_T;

#define EEP2MSDC_MASK_ACCESS_PROHIBIT       (0x11)
#define EEP2MSDC_BANKA_ACCESS_PROHIBIT      (1<<0)
#define EEP2MSDC_BANKB_ACCESS_PROHIBIT      (1<<4)

typedef struct _UBOOT_MSDC_EEPROM_T
{
    u32 u4SDMPartId;
    u32 u4EEPPartIdA;
    u32 u4EEPPartIdB;
    u32 u4EEPPartOffA;
    u32 u4EEPPartOffB;
    u32 u4EEPBlkNum;
    u32 u4EEPSize;

    u32 u4WriteCnt;

    u32 u4EEPBlkA;
    u32 u4EEPBlkB;

    u32 u4EEPInitHeaderSize;
    u32 u4EEPHeaderSize;
    u32 u4EEPHeaderOffset;
    u32 u4EEPBankAOffset;
    u32 u4EEPBankBOffset;
    u8  u1fgEEPAccessProtect; //0x00:fully 0x01:A prohibit 0x10:B prohibit 0x11:access prohibit

    u8 *pu1EEPBuf;
} UBOOT_MSDC_EEPROM_T;
static _fgEEPInitialized = 0;
static UBOOT_MSDC_EEPROM_T _rMSDCEEPROM;
#endif


#if defined(CONFIG_CMD_JFFS2) && defined(CONFIG_JFFS2_CMDLINE)
/* parition handling routines */
int mtdparts_init(void);
int id_parse(const char *id, const char **ret_id, u8 *dev_type, u8 *dev_num);
int find_dev_and_part(const char *id, struct mtd_device **dev, u8 *part_num, struct part_info **part);
int find_partition(const char *part_name, u8 *part_num, u32 *part_size, u32 *part_offset);
#endif

static void print_mmcinfo(struct mmc *mmc)
{
	printf("Device: %s\n", mmc->name);
	printf("Manufacturer ID: %x\n", mmc->cid[0] >> 24);
	printf("OEM: %x\n", (mmc->cid[0] >> 8) & 0xffff);
	printf("Name: %c%c%c%c%c \n", mmc->cid[0] & 0xff,
			(mmc->cid[1] >> 24), (mmc->cid[1] >> 16) & 0xff,
			(mmc->cid[1] >> 8) & 0xff, mmc->cid[1] & 0xff);

	printf("Tran Speed: %d\n", mmc->tran_speed);
	printf("Rd Block Len: %d\n", mmc->read_bl_len);

	printf("%s version %d.%d\n", IS_SD(mmc) ? "SD" : "MMC",
			(mmc->version >> 4) & 0xf, mmc->version & 0xf);

	printf("High Capacity: %s\n", mmc->high_capacity ? "Yes" : "No");
	printf("Capacity: %lld\n", mmc->capacity);

	printf("Bus Width: %d-bit\n", mmc->bus_width);
}

int do_mmcinfo (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	struct mmc *mmc;
	int dev_num;

	if (argc < 2)
	{
		dev_num = 0;
	}
	else
	{
		dev_num = simple_strtoul(argv[1], NULL, 0);
	}

	mmc = find_mmc_device(dev_num);

	if (mmc)
  {
		mmc_init(mmc);
		print_mmcinfo(mmc);
	}

	return 0;
}

U_BOOT_CMD(
	mmcinfo, 2, 0, do_mmcinfo,
	"display MMC info",
	"<dev num>\n"
	"    - device number of the device to dislay info of\n"
	""
);

int do_mmcops(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int rc = 0;

	switch (argc)
  {
	case 3:
    if (strcmp(argv[1], "rescan") == 0)
    {
			int dev = simple_strtoul(argv[2], NULL, 10);
			struct mmc *mmc = find_mmc_device(dev);

			if (!mmc)
			{
				return 1;
			}

			mmc_init(mmc);
			return 0;
		}
    else if (strncmp(argv[1], "part", 4) == 0)
    {
			int dev = simple_strtoul(argv[2], NULL, 10);
			block_dev_desc_t *mmc_dev;
			struct mmc *mmc = find_mmc_device(dev);

			if (!mmc)
      {
				puts("no mmc devices available\n");
				return 1;
			}

			mmc_init(mmc);
			mmc_dev = mmc_get_dev(dev);
			if (mmc_dev != NULL && mmc_dev->type != DEV_TYPE_UNKNOWN)
			{
				print_part(mmc_dev);
				return 0;
			}

			puts("get mmc type error!\n");
			return 1;
		}
		else if (strcmp(argv[1], "eraseall") == 0)
    {
      int dev = simple_strtoul(argv[2], NULL, 10);
			struct mmc *mmc = find_mmc_device(dev);
			u32 n;

			if (!mmc)
			{
				return 1;
			}

			n = mmc_erase_all(mmc);

      return n;
    }

	case 0:
	case 1:
	case 4:
    return cmd_usage(cmdtp);

	case 2:
		if (!strcmp(argv[1], "list"))
    {
			print_mmc_devices('\n');
			return 0;
		}
		return 1;

	default: /* at least 5 args */
		if ((strcmp(argv[1], "read") == 0) || (strcmp(argv[1], "read.b") == 0))
    {
			int dev = simple_strtoul(argv[2], NULL, 10);
			void *addr = (void *)simple_strtoul(argv[3], NULL, 16);
			u32 cnt = simple_strtoul(argv[5], NULL, 16);
			u32 n;
			u64 blk = simple_strtoull(argv[4], NULL, 16);

			struct mmc *mmc = find_mmc_device(dev);
			if (!mmc)
			{
				return 1;
			}

      if (strcmp(argv[1], "read") == 0)
      {
			    printf("\nMMC block read: dev # %d, block # %lld, count %d ... ", dev, blk, cnt);
      }
      else
      {
			    printf("\nMMC byte read: dev # %d, offset # %lld, count %d ... ", dev, blk, cnt);
      }

			mmc_init(mmc);

      if (strcmp(argv[1], "read") == 0)
      {
        n = mmc->block_dev.block_read(dev, blk, cnt, addr);

        /* flush cache after read */
        flush_cache((ulong)addr, cnt * 512); /* FIXME */
      }
      else
      {
        n = mmc->block_dev.byte_read(dev, blk, cnt, addr);

        /* flush cache after read */
        flush_cache((ulong)addr, cnt); /* FIXME */
      }

      if (strcmp(argv[1], "read") == 0)
      {
        printf("%d blocks read: %s\n", n, (n==cnt) ? "OK" : "ERROR");
      }
      else
      {
        printf("%d bytes read: %s\n", n, (n==cnt) ? "OK" : "ERROR");
      }
			return (n == cnt) ? 0 : 1;
		}
    else if ((strcmp(argv[1], "write") == 0) || (strcmp(argv[1], "write.b") == 0))
    {
			int dev = simple_strtoul(argv[2], NULL, 10);
			void *addr = (void *)simple_strtoul(argv[3], NULL, 16);
			u32 cnt = simple_strtoul(argv[5], NULL, 16);
			u32 n;
			u64 blk = simple_strtoull(argv[4], NULL, 16);
			struct mmc *mmc = find_mmc_device(dev);
			if (!mmc)
			{
				return 1;
			}

      if (strcmp(argv[1], "read") == 0)
      {
        printf("\nMMC block write: dev # %d, block # %lld, count %d ... ", dev, blk, cnt);
      }
      else
      {
        printf("\nMMC byte write: dev # %d, offset # %lld, count %d ... ", dev, blk, cnt);
      }

			mmc_init(mmc);

      if (strcmp(argv[1], "write") == 0)
      {
        n = mmc->block_dev.block_write(dev, blk, cnt, addr);
      }
      else
      {
        n = mmc->block_dev.byte_write(dev, blk, cnt, addr);
      }

      if (strcmp(argv[1], "read") == 0)
      {
        printf("%d blocks written: %s\n", n, (n == cnt) ? "OK" : "ERROR");
      }
      else
      {
        printf("%d bytes written: %s\n", n, (n == cnt) ? "OK" : "ERROR");
      }
			return (n == cnt) ? 0 : 1;
		}
    else if ((strcmp(argv[1], "erase") == 0) ||
    	       (strcmp(argv[1], "trim") == 0) ||
    	       (strcmp(argv[1], "secureerase") == 0) ||
    	       (strcmp(argv[1], "securetrim1") == 0) ||
    	       (strcmp(argv[1], "securetrim2") == 0))
    {
      int dev = simple_strtoul(argv[2], NULL, 10);
			u64 bytestart = simple_strtoull(argv[3], NULL, 16);
			u64 length = simple_strtoull(argv[4], NULL, 16);
			int mode;
			u32 n;

			struct mmc *mmc = find_mmc_device(dev);
			if (!mmc)
			{
				return 1;
			}

			if (strcmp(argv[1], "erase") == 0)
      {
          mode = 0;
      }
      else if (strcmp(argv[1], "trim") == 0)
      {
          mode = 1;
      }
      else if (strcmp(argv[1], "secureerase") == 0)
      {
          mode = 2;
      }
      else if (strcmp(argv[1], "securetrim1") == 0)
      {
          mode = 3;
      }
      else if (strcmp(argv[1], "securetrim2") == 0)
      {
          mode = 4;
      }

      n = mmc_erase(mmc, bytestart, length, mode);

      return n;
    }
    else if (strcmp(argv[1], "wp") == 0)
    {
      int dev = simple_strtoul(argv[2], NULL, 10);
			u32 bytestart = simple_strtoul(argv[3], NULL, 16);
			u32 level = simple_strtoul(argv[4], NULL, 10);
			u32 type = simple_strtoul(argv[5], NULL, 10);
			u32 fgen = simple_strtoul(argv[6], NULL, 10);
			u32 n;

			struct mmc *mmc = find_mmc_device(dev);
			if (!mmc)
			{
				return 1;
			}

			n = mmc_wp(mmc, bytestart, level, type, fgen);

			return n;

    }
    else
    {
			rc = cmd_usage(cmdtp);
    }

		return rc;
	}
}

U_BOOT_CMD(
	mmc, 7, 1, do_mmcops,
	"MMC sub system",
	"read <device num> addr blk# cnt\n"
	"mmc write <device num> addr blk# cnt\n"
	"mmc eraseall\n"
	"mmc erase <device num> byte_addr length mode\n"
	"mmc wp <device num> byte_addr level type en\n"
	"mmc rescan <device num>\n"
	"mmc part <device num> - lists available partition on mmc\n"
	"mmc list - lists available devices");

static int emmc_read(u64 offset, void* addr, size_t cnt)
{
    int r;
    struct mmc *mmc;

    mmc = find_mmc_device(CONFIG_SYS_MMC_ENV_DEV);
    if (!mmc)
    {
        return 1;
    }

    if (mmc_init(mmc))
    {
        puts("MMC init failed\n");
        return 1;
    }

    r = mmc->block_dev.byte_read(CONFIG_SYS_MMC_ENV_DEV, offset, cnt, addr);
    if (r != cnt)
    {
        puts("** Read error\n");
        return 1;
    }

    return cnt;
}
static int emmc_write(u64 offset, void* addr, size_t cnt)
{
    int r;
    struct mmc *mmc;

    mmc = find_mmc_device(CONFIG_SYS_MMC_ENV_DEV);
    if (!mmc)
    {
        return 1;
    }

    if (mmc_init(mmc))
    {
        puts("MMC init failed\n");
        return 1;
    }

    r = mmc->block_dev.byte_write(CONFIG_SYS_MMC_ENV_DEV, offset, cnt, addr);
    if (r != cnt)
    {
        puts("** Read error\n");
        return 1;
    }

    return cnt;
}

extern int emmc_read_msdc(u64 offset, void* addr, size_t cnt)
{
    int r;
    struct mmc *mmc;

    mmc = find_mmc_device(CONFIG_SYS_MMC_ENV_DEV);
    if (!mmc)
    {
        return -1;
    }

    if (mmc_init(mmc))
    {
        puts("MMC init failed\n");
        return -1;
    }

    r = mmc->block_dev.byte_read(CONFIG_SYS_MMC_ENV_DEV, offset, cnt, addr);
    if (r != cnt)
    {
        puts("Read error\n");
        return -1;
    }
    else
    {
        puts("Read ok\n");
    }

    return 0;
}

extern int emmc_write_msdc(u64 offset, void* addr, size_t cnt)
{
    int r;
    struct mmc *mmc;

    mmc = find_mmc_device(CONFIG_SYS_MMC_ENV_DEV);
    if (!mmc)
    {
        return -1;
    }

    if (mmc_init(mmc))
    {
        puts("MMC init failed\n");
        return -1;
    }

    r = mmc->block_dev.byte_write(CONFIG_SYS_MMC_ENV_DEV, offset, cnt, addr);
    if (r != cnt)
    {
        printf("write error\n");
        return -1;
    }
    else
    {
        printf("write ok\n");
    }

    return 0;
}

#ifdef CC_EEP2EMMC_DUAL_BANK_SUPPORT
static int UBOOT_ReadMsdc(u32 u4PartId, u32 u4Offset, void *pvMemPtr, u32 u4MemLen)
{
    // get physical address by partition id.
    u64 u8Offset = u4Offset;
    u8Offset += DRVCUST_InitGet64((QUERY_TYPE_T)(eNANDFlashPartOffset0 + u4PartId));

    if (emmc_read_msdc(u8Offset, (u32)pvMemPtr, u4MemLen) == 0)
    {
        return (int)u4MemLen;
    }
    else
    {
        return -1;
    }
}

static int UBOOT_WriteMsdc(u32 u4PartId, u32 u4Offset, void *pvMemPtr, u32 u4MemLen)
{
    // get physical address by partition id.
    u64 u8Offset = u4Offset;
    u8Offset += DRVCUST_InitGet64((QUERY_TYPE_T)(eNANDFlashPartOffset0 + u4PartId));

    if (emmc_write_msdc(u8Offset, (u32)pvMemPtr, u4MemLen) == 0)
    {
        return (int)u4MemLen;
    }
    else
    {
        return -1;
    }
}

static int _UBOOT_MSDC_EEPROM_Init(void)
{
    u8 u1InitHeader[4]={0};

    if (_fgEEPInitialized)
    {
        return 0;
    }

    _rMSDCEEPROM.u4EEPSize    = DRVCUST_InitGet64(eSystemEepromSize);
    _rMSDCEEPROM.u4SDMPartId  = DRVCUST_InitGet64(eNANDFlashDynamicPartitionID);
    _rMSDCEEPROM.u4EEPPartIdA = DRVCUST_InitGet64(eNANDFlashPartIdEepromA);
    _rMSDCEEPROM.u4EEPPartIdB = DRVCUST_InitGet64(eNANDFlashPartIdEepromB);
    _rMSDCEEPROM.pu1EEPBuf = (u8 *)malloc(_rMSDCEEPROM.u4EEPSize);

    if (_rMSDCEEPROM.pu1EEPBuf == NULL)
    {
        printf("_UBOOT_MSDC_EEPROM_Init Can't allocate memory!\n");
        return -1;
    }

    _rMSDCEEPROM.u4EEPPartOffA = DRVCUST_InitGet64((QUERY_TYPE_T)(eNANDFlashPartOffset0 + _rMSDCEEPROM.u4EEPPartIdA));
    _rMSDCEEPROM.u4EEPPartOffB = DRVCUST_InitGet64((QUERY_TYPE_T)(eNANDFlashPartOffset0 + _rMSDCEEPROM.u4EEPPartIdB));

    _rMSDCEEPROM.u4WriteCnt = 0xFFFFFFFF;
    _rMSDCEEPROM.u4EEPBlkNum = (_rMSDCEEPROM.u4EEPSize < 512) ? 1 : _rMSDCEEPROM.u4EEPSize / 512;
    _rMSDCEEPROM.u4EEPBlkA = 0xFFFFFFFF;
    _rMSDCEEPROM.u4EEPBlkB = 0xFFFFFFFF;

    _rMSDCEEPROM.u4EEPInitHeaderSize = DRVCUST_InitGet64(eSystemEepInitHeaderSize);
    _rMSDCEEPROM.u4EEPHeaderSize = DRVCUST_InitGet64(eSystemEepHeaderSize);
    _rMSDCEEPROM.u4EEPHeaderOffset = _rMSDCEEPROM.u4EEPInitHeaderSize;
    _rMSDCEEPROM.u4EEPBankAOffset = _rMSDCEEPROM.u4EEPHeaderOffset + _rMSDCEEPROM.u4EEPHeaderSize;
    _rMSDCEEPROM.u4EEPBankBOffset = _rMSDCEEPROM.u4EEPBankAOffset + _rMSDCEEPROM.u4EEPHeaderSize;;
    //read init header first
    if (UBOOT_ReadMsdc(_rMSDCEEPROM.u4EEPPartIdA, 0, (void*)(u1InitHeader), 4) != 4)
    {
        printf("_UBOOT_MSDC_EEPROM_Init header(%d) fail!\n",_rMSDCEEPROM.u4EEPPartIdA);
        return -1;
    }
    if((u1InitHeader[0]==0x58) && (u1InitHeader[1]==0x90) &&
		(u1InitHeader[2]==0x58) && (u1InitHeader[3]==0x90))
    {
	    _rMSDCEEPROM.u1fgEEPAccessProtect = 0x00;
    }
    else
    {
        _rMSDCEEPROM.u1fgEEPAccessProtect = 0x11;
    }

    memset((void *)_rMSDCEEPROM.pu1EEPBuf, 0xFF, _rMSDCEEPROM.u4EEPSize);

    _fgEEPInitialized = 1;

   //LOG(3, "_MSDC_EEPROM_Init end!\n");
    return 0;
}
int UBOOT_MSDC_EEPROM_Read_Dual_Bank(u32 u4Offset, u32 u4MemPtr, u32 u4MemLen)
{
    int i4Ret = 0;
    u32 u4HeaderIdx;
    u8 u1HeadderCount=0;

    _UBOOT_MSDC_EEPROM_Init();

    if ((u4Offset >= _rMSDCEEPROM.u4EEPSize) || ((u4Offset + u4MemLen) > _rMSDCEEPROM.u4EEPSize))
    {
        printf("MSDC_EEPROM_Read: out of boundry! u4Offset = 0x%x, u4MemLen = 0x%x\n", u4Offset, u4MemLen);

        return -1;
    }

    if ((_rMSDCEEPROM.u1fgEEPAccessProtect & EEP2MSDC_MASK_ACCESS_PROHIBIT) != 0)
    {
        printf("UBOOT_MSDC_EEPROM_Read access prohibit!\n");
        return -1;
    }

    u4HeaderIdx = u4Offset + _rMSDCEEPROM.u4EEPHeaderOffset;
    if (UBOOT_ReadMsdc(_rMSDCEEPROM.u4EEPPartIdA, u4HeaderIdx,(void*)(&u1HeadderCount), 1) != 1)
    {
        printf("UBOOT_Msdc_Read_EEPROMHeader fail!\n");
        return -1;
    }
    if(u1HeadderCount == 0)
    {
        u4Offset = u4Offset + _rMSDCEEPROM.u4EEPBankAOffset;
    }
    else
    {
        u4Offset = u4Offset + _rMSDCEEPROM.u4EEPBankBOffset;
    }

    if (UBOOT_ReadMsdc(_rMSDCEEPROM.u4EEPPartIdA, u4Offset,(void*)u4MemPtr, u4MemLen) != u4MemLen)
    {
        printf("UBOOT_Msdc_Read_EEPROMData fail!\n");
        return -1;
    }

    return i4Ret;
}

int UBOOT_MSDC_EEPROM_Write_Dual_Bank(u32 u4Offset, u32 u4MemPtr, u32 u4MemLen)
{
    int i4Ret = 0;
    u32 u4HeaderIdx;
    u8 u1HeadderCount=0;
    u32 u4SyncOffset;
    u8 fgDualBankSyncEnalbe=0;

    _UBOOT_MSDC_EEPROM_Init();

    if ((u4Offset >= _rMSDCEEPROM.u4EEPSize) || ((u4Offset + u4MemLen) > _rMSDCEEPROM.u4EEPSize))
    {
        printf("UBOOT_MSDC_EEPROM_Write: out of boundry! u4Offset = 0x%x, u4MemLen = 0x%x\n", u4Offset, u4MemLen);

        return -1;
    }

    //64k area was handled by MTK
    if(u4Offset < 0x10000)
    {
        fgDualBankSyncEnalbe=1;
    }
    if ((_rMSDCEEPROM.u1fgEEPAccessProtect & EEP2MSDC_MASK_ACCESS_PROHIBIT) != 0)
    {
        printf("UBOOT_MSDC_EEPROM_Write access prohibit!\n");
        return -1;
    }
    //read header first
    u4HeaderIdx = u4Offset + _rMSDCEEPROM.u4EEPHeaderOffset;
    if (UBOOT_ReadMsdc(_rMSDCEEPROM.u4EEPPartIdA, u4HeaderIdx,(void*)(&u1HeadderCount), 1) != 1)
    {
        printf("UBOOT_Msdc_Write_Read_EEPROMHeader fail!\n");
        return -1;
    }
    //decide which bank to write to
    if(u1HeadderCount == 0)
    {
        u4SyncOffset = u4Offset + _rMSDCEEPROM.u4EEPBankAOffset;
        u4Offset = u4Offset + _rMSDCEEPROM.u4EEPBankBOffset;
    }
    else
    {
        u4SyncOffset = u4Offset + _rMSDCEEPROM.u4EEPBankBOffset;
        u4Offset = u4Offset + _rMSDCEEPROM.u4EEPBankAOffset;
    }
    //data write
    if (UBOOT_WriteMsdc(_rMSDCEEPROM.u4EEPPartIdA, u4Offset,(void*)u4MemPtr, u4MemLen) != u4MemLen)
    {
        printf("UBOOT_MSDC_EEPROM_Write fail!\n");
        return -1;
    }
    //update header count
    if(u1HeadderCount == 0) {
        u1HeadderCount = 1;
    }
    else {
        u1HeadderCount = 0;
    }
    if (UBOOT_WriteMsdc(_rMSDCEEPROM.u4EEPPartIdA, u4HeaderIdx,(void*)(&u1HeadderCount), 1) != 1)
    {
        printf("UBOOT_Msdc_Write_Read_EEPROMHeader fail!\n");
        return -1;
    }

    //sync data write
    if(fgDualBankSyncEnalbe)
    {
        if (UBOOT_WriteMsdc(_rMSDCEEPROM.u4EEPPartIdA, u4SyncOffset,(void*)u4MemPtr, u4MemLen) != u4MemLen)
        {
            printf("UBOOT_MSDC_EEPROM_Write fail!\n");
            return -1;
        }
    }

    return i4Ret;
}
#endif

#ifdef CC_SECURE_ROM_BOOT
// Auth kernel image at addr
static void emmc_auth_image(struct part_info *part, ulong addr, ulong skip)
{
	image_header_t *hdr = (image_header_t *)addr;
    u32 sig_offset = image_get_image_size(hdr);
    #ifdef CC_SECURE_BOOT_V2
    // fix signature offset with pack size - fill 0xff by 16 bytes alignment by sign tool
    sig_offset = ((sig_offset) + 0x10 ) & (~0xf);
    #endif
 	printf("kernel: ");
    emmc_read(part->offset+sig_offset+skip, (void *)CFG_LOAD_ADDR, 0x200);
    sig_authetication(addr, sig_offset, (unsigned long *)CFG_LOAD_ADDR, (unsigned long *)(CFG_LOAD_ADDR+0x100));
}

// Auth ramdisk image at addr
static void emmc_auth_image_ex(struct part_info *part, ulong addr, ulong skip, ulong size_to_check)
{
	#ifndef CC_SECURE_BOOT_WO_RAMDISK
 	printf("ramdisk: ");
    #ifdef CC_SECURE_BOOT_V2
    // this only check size_to_check
    //sig_authetication(addr, size_to_check, (unsigned long *)(addr + skip - 0x100), (unsigned long *)(addr + skip));
    // this check full ramdisk size
    sig_authetication(addr, skip - 0x100, (unsigned long *)(addr + skip - 0x100), (unsigned long *)(addr + skip));
    #else // CC_SECURE_BOOT_V2
    sig_authetication(addr, size_to_check, (unsigned long *)(addr + skip - 0x200), (unsigned long *)(addr + skip - 0x100));
    #endif // CC_SECURE_BOOT_V2
	#endif // CC_SECURE_BOOT_WO_RAMDISK
}
#else
#define emmc_auth_image(part, addr, skip)
#define emmc_auth_image_ex(part, addr, skip, size_to_check)
#endif

static int emmc_load_image(cmd_tbl_t *cmdtp, u64 offset, ulong addr, char *cmd)
{
  u32 n;
  char *ep;
  size_t cnt;
  char *local_args[3];
  char local_args_temp[3][0x30];

  image_header_t *hdr;
  struct mmc *mmc;
#if defined(CONFIG_OF_LIBFDT)
	addr = CFG_RD_SRC_ADDR - 0x40;
#endif

	mmc = find_mmc_device(CONFIG_SYS_MMC_ENV_DEV);
	if (!mmc)
	{
		return 1;
	}

	if (mmc_init(mmc)) {
		puts("MMC init failed\n");
		return 1;
	}

  printf("\nLoading from mmc%d, offset 0x%llx\n", CONFIG_SYS_MMC_ENV_DEV, offset);

  n = mmc->block_dev.byte_read(CONFIG_SYS_MMC_ENV_DEV, offset, mmc->read_bl_len, (void *)addr);
  if (n != mmc->read_bl_len)
  {
    puts("** Read error\n");
    show_boot_progress (-56);
    return 1;
  }
  show_boot_progress (56);

  switch (genimg_get_format ((void *)addr))
  {
  case IMAGE_FORMAT_LEGACY:
    hdr = (image_header_t *)addr;

    show_boot_progress (57);
    image_print_contents (hdr);

    cnt = image_get_image_size (hdr);
    break;

  default:
    show_boot_progress (-57);
    puts ("** Unknown image type\n");
    return 1;
  }

  n = mmc->block_dev.byte_read(CONFIG_SYS_MMC_ENV_DEV, offset, cnt, (void *)addr);
  if (n != cnt)
  {
    puts("** Read error\n");
    show_boot_progress (-58);
    return 1;
  }
  show_boot_progress (58);

#if !(CONFIG_FAST_BOOT) //skip verify for fast boot
  if (((ep = getenv("verify")) != NULL) && (strcmp(ep, "y") == 0))
  {
    puts ("   Verifying Checksum ... ");
    if (!image_check_dcrc (hdr))
    {
      printf ("Bad Data CRC\n");
      show_boot_progress (-58);
      return NULL;
    }
    puts ("OK\n");
  }
#endif

  if(cmd == NULL)
  {
    return 0;
  }

  /* Loading ok, update default load address */

#if defined(CONFIG_OF_LIBFDT)
  load_addr = CFG_RD_SRC_ADDR;
#else
	load_addr = addr;
#endif

  /* Check if we should attempt an auto-start */
  if (((ep = getenv("autostart")) != NULL) &&
       (strcmp(ep, "yes") == 0) &&
       (hdr->ih_type == IH_TYPE_KERNEL))
  {
    local_args[0] = cmd;

    printf("Automatic boot of image at addr 0x%08lx ...\n", addr);

    if ((ep = getenv("initrd")) != NULL)
    {
      local_args[1] = local_args_temp[1];
      local_args[2] = local_args_temp[2];
      addr = CFG_RD_ADDR;
      sprintf(local_args[1], "0x%x", (unsigned int)load_addr);
      sprintf(local_args[2], "0x%x", (unsigned int)addr);
      do_bootm(cmdtp, 0, 3, local_args);
    }
    else
    {
      local_args[1] = NULL;
      local_args[2] = NULL;
      do_bootm(cmdtp, 0, 1, local_args);
    }

    return cnt;
  }

  return 0;
}

int emmc_read_from_partition(unsigned char* buf, char* pname, u64 offset, unsigned int size)
{
    struct mtd_device *dev;
    struct part_info *part;
    u8 pnum;

    if ((mtdparts_init() == 0) && (find_dev_and_part(pname, &dev, &pnum, &part) == 0))
    {
        emmc_read(part->offset+offset, buf, size);
        return 0;
    }

    return 1;
}

int emmc_write_to_partition(unsigned char* buf, char* pname, u64 offset, unsigned int size)
{
    struct mtd_device *dev;
    struct part_info *part;
    u8 pnum;

    if ((mtdparts_init() == 0) && (find_dev_and_part(pname, &dev, &pnum, &part) == 0))
    {
        emmc_write(part->offset+offset, buf, size);
        return 0;
    }

    return 1;
}

#define EMMC_ASYNC_SIZE (63*1024)

struct emmc_fill_data
{
    int cmd_data_left;
    int recv_data_left;
};


struct emmc_normal_fill_data
{
    int recv_data_left;
	uint cur_pos;
	u64 cur_off;
	uint cur_bytes;
	
};

#if !MSDC_USE_NORMAL_READ_LZO
int emmc_fill(void *data)
{
    int size, recv_size, ret;
    struct emmc_fill_data *ef = (struct emmc_fill_data *)data;

    if (!ef->recv_data_left)
        return -1;

    // Wait for previous block complete.
    recv_size = size = (ef->recv_data_left < EMMC_ASYNC_SIZE) ? ef->recv_data_left : EMMC_ASYNC_SIZE;
    ret = emmc_async_dma_wait_finish();

    if (ret)
        printf("emmc_async_dma_wait_finish %d ret %d\n", size, ret);

    ef->recv_data_left -= size;

    // Now send command for next.
    if (ef->cmd_data_left)
    {
        size = (ef->cmd_data_left < EMMC_ASYNC_SIZE) ? ef->cmd_data_left : EMMC_ASYNC_SIZE;
        ret = emmc_async_dma_start_trigger(size);
        if (ret)
            printf("emmc_async_dma_start_trigger %d ret %d\n", size, ret);
        ef->cmd_data_left -= size;
    }

    return recv_size;
}

#else
int emmc_normal_fill(void *data)
{
     int ret;
    struct emmc_normal_fill_data *ef = (struct emmc_normal_fill_data *)data;
    int n = 0;

    if (!ef->recv_data_left)
        return -1;

    if (ef->recv_data_left) {
		ef->cur_bytes = (ef->recv_data_left < EMMC_ASYNC_SIZE) ? ef->recv_data_left : EMMC_ASYNC_SIZE;
        n = emmc_read(ef->cur_off,ef->cur_pos,ef->cur_bytes);
		if (n != ef->cur_bytes)
			printf("** emmc_normal_fill error, read cnt 0x%x ,cur off 0x%llx \n",n,ef->cur_off);

		ef->cur_pos += ef->cur_bytes;
		ef->cur_off += ef->cur_bytes;
        ef->recv_data_left -= ef->cur_bytes;
    }

    return ef->cur_bytes;
}
#endif

extern int unlzo_read(u8 *input, int in_len, int (*fill) (void *fill_data), void *fill_data, u8 *output, int *posp);

// Load & decompress LZO image at the same time.
//
// @param cnt      Size of compressed image on EMMC.
// @param src_buf  Output buffer of source compressed image.
// @param out_buf  Output buffer of decompressed data
// @param decomp_size  Decompressed data size.
// @param skip     Number of bytes to skip at start.
static int emmc_read_unlzo(u64 offset, u8 *src_buf, u8 *out_buf, size_t cnt, int *decomp_size, int skip)
{
    int ret, len, dma_size0, dma_size1, rval=0;
    u32 n = 0;

#if MSDC_USE_NORMAL_READ_LZO

struct  emmc_normal_fill_data fdata;

    /* align to 512 bytes */

    len = (cnt + 511) & (~511);
    dma_size0 = (len < EMMC_ASYNC_SIZE) ? len : EMMC_ASYNC_SIZE;

	fdata.recv_data_left = len - dma_size0;
	fdata.cur_bytes = dma_size0;
	fdata.cur_pos = src_buf;
	fdata.cur_off = offset;

	printf("\n -- load lzo from mmc%d, offset 0x%llx\n", CONFIG_SYS_MMC_ENV_DEV, offset);

	/* Read first block */
	n = emmc_read(fdata.cur_off,fdata.cur_pos,fdata.cur_bytes);
	if (n != fdata.cur_bytes) {
		printf("** load lzo Read error, read cnt 0x%x \n",n);
		rval=1;
		goto out;
	}

	fdata.cur_pos += fdata.cur_bytes;
	fdata.cur_off += fdata.cur_bytes;

	/* Check for LZO header, return fail if not found. */
	if (*(int*)(src_buf + skip) != 0x4f5a4c89) {
		printf("** load lzo data check failed, cur data 0x%x  \n",*(int*)(src_buf+skip));
	    rval=1;
	    goto out;
	}

#else
    struct emmc_fill_data fdata;
	
    // align to 512 bytes
    len = (cnt + 511) & (~511);
    dma_size0 = (len < EMMC_ASYNC_SIZE) ? len : EMMC_ASYNC_SIZE;

    fdata.recv_data_left = fdata.cmd_data_left = len - dma_size0;

    ret = emmc_async_read_setup(0, offset, len, src_buf);
    if (ret)
        printf("emmc_async_read_setup done %d\n", ret);

    // Read first block, must wait for first block
    emmc_async_dma_start_trigger(dma_size0);
    emmc_async_dma_wait_finish();

    // Check for LZO header, return fail if not found.
    if (*(int*)(src_buf+skip) != 0x4f5a4c89)
    {
        rval=1;
        emmc_async_read_stop(0, 0);
        goto out;
    }

    // Now, read next, don't wait.
    if (fdata.cmd_data_left)
    {
        dma_size1 = (fdata.cmd_data_left < EMMC_ASYNC_SIZE) ? fdata.cmd_data_left : EMMC_ASYNC_SIZE;
        emmc_async_dma_start_trigger(dma_size1);
        fdata.cmd_data_left -= dma_size1;
    }

#endif

    printf("---start unlzo--ret %d , decomp size 0x%x -\n",rval,dma_size0-skip);

	#if MSDC_USE_NORMAL_READ_LZO
    ret = unlzo_read(src_buf+skip, dma_size0-skip, emmc_normal_fill, &fdata, out_buf, decomp_size);
	#else
	ret = unlzo_read(src_buf+skip, dma_size0-skip, emmc_fill, &fdata, out_buf, decomp_size);
	#endif
	
    if (ret) {
        printf("block unlzo failed.. %d\n", ret);
        rval=1;
        goto out;
    }

	printf("\n---end unlzo---\n");

out:
#if !(MSDC_USE_NORMAL_READ_LZO)

    // Send finish command.
    ret = emmc_async_read_finish(0);
    if (ret)
        printf("emmc_async_read_finish done %d\n", ret);
#endif
    return rval;
}

static int emmc_load_image_lzo(cmd_tbl_t *cmdtp, struct part_info *part, char *cmd)
{
    int r;
    ulong addr;
	u64 offset;
    size_t cnt;
    int decomp_size;
    char *local_args[3];

    // Booting UIMAGE + LZO kernel.
    // !!!!!FIXME!!!!! We don't know the size of image, try part size.
    addr = 0x7fc0;
    cnt = part->size;
    offset = part->offset;
    r = emmc_read_unlzo(offset, (void*)CFG_RD_SRC_ADDR, (void*)addr, cnt, &decomp_size, 0x40);

	printf("unzlo done. decomp size 0x%x, limit 0x%x \n",decomp_size,CONFIG_SYS_TEXT_BASE);
	if(decomp_size + addr >= CONFIG_SYS_TEXT_BASE)
	{
		printf("\n The decomp kernel size is too large and will overlap with uboot !\n");
		while(1);
	}

    if (!r)
    {
        // verify the kernel.
        emmc_auth_image(part, CFG_RD_SRC_ADDR, 0);

        // Good. Decompress work, boot it
        load_addr = addr;
        local_args[0] = cmd;
        local_args[1] = NULL;
        local_args[2] = NULL;
        do_bootm(cmdtp, 0, 1, local_args);
    }

    // read fail, go back and try normal flow
    return 0;
}


#define WITH_PADDING_SIZE(size, page)    (((size)+(page)-1) & (~((page)-1)))

int do_emmcboot(cmd_tbl_t * cmdtp, int flag, int argc, char *argv[])
{
    u8 pnum;
    char *tmp, env[64];
    ulong addr;
	u64 rootfsoffset = 0;

    struct mtd_device *dev;
    struct part_info *part;
#if defined(CONFIG_OF_LIBFDT)

			if (getenv("bootm_size")==NULL)
				{

				 char buf[20];
					sprintf(buf, "0x%x", 100*1024*1024);
					setenv("bootm_size", buf);
				}
#endif

			printf("-----------call do_emmcboot---------\n");
    if (argc >= 2)
    {
		printf("-----------argc = %d ---------\n",argc);
        if ((mtdparts_init() == 0) &&
            (find_dev_and_part(argv[1], &dev, &pnum, &part) == 0))
        {
            // Fix kernel address at 0x8000 to avoid from reallocation time
            addr = 0x8000;

            if (dev->id->type != MTD_DEV_TYPE_EMMC)
            {
                puts("Not a EMMC device\n");
                return 1;
            }

            if (argc > 3)
            {
                goto usage;
            }

            if (argc == 3)
            {
                if ((strcmp(argv[2], "rootfs")  == 0) ||
                    (strcmp(argv[2], "rootfsA") == 0) ||
                    (strcmp(argv[2], "rootfsB") == 0) ||
                    (strcmp(argv[2], "linux_rootfsA") == 0) ||
                    (strcmp(argv[2], "linux_rootfsB") == 0))
                {
                    u8 part_num;
                    u64 part_size, part_offset;

                    if (find_partition(argv[2], &part_num, &part_size, &part_offset) != 0)
                    {
                        printf("\n** Ramdisk partition %s not found!\n", argv[2]);
                        show_boot_progress(-55);
                        return 1;
                    }

                    rootfsoffset = part_offset;
                }
                else
                {
                    goto usage;
                }
            }
            else if (argc == 2)
            {
                // android boot
                char *local_args[3];
                char local_args_temp[3][16];
                char env[32];
                struct boot_img_hdr hdr;
                int magic_found, ret;
                int cnt, decomp_size;
                u64 offset;

				printf("---android boot---\n");
                // 1. load header
                sprintf(env, "0x%x", sizeof(boot_img_hdr));
                setenv("squashfs_size", env);
                offset = part->offset;
                cnt = 2048;
                emmc_read(offset, (void*)CFG_RD_ADDR, cnt);
                memcpy((void*)&hdr, (void*)CFG_RD_ADDR, sizeof(boot_img_hdr));
                magic_found = !strncmp((char*)hdr.magic, BOOT_MAGIC, BOOT_MAGIC_SIZE);

                if (magic_found && strstr(argv[0], ".lzo"))
                {
					printf("---lzo boot---\n");
                    // 2. load kernel
                    offset += hdr.page_size;
                    addr = 0x7fc0;
                    cnt = WITH_PADDING_SIZE(hdr.kernel_size, hdr.page_size);
                    ret = emmc_read_unlzo(offset, (void*)CFG_RD_SRC_ADDR, (void*)addr, cnt, &decomp_size, 0x40);

					if(decomp_size + addr >= 0x1000000)
						{
							printf("\n The decomp kernel size is too large and will overlap with uboot !\n");
							while(1);
						}
                    if (ret)
                    {
                        // LZO read fail, read as normal uImage.
                        addr = 0x8000;
                        emmc_read(offset, (void*)addr, WITH_PADDING_SIZE(hdr.kernel_size, hdr.page_size));
                    }

                    // 3. verify the kernel.
                    emmc_auth_image(part, CFG_RD_SRC_ADDR, hdr.page_size);

                    // 4. load ramdisk
                    offset += WITH_PADDING_SIZE(hdr.kernel_size, hdr.page_size);
                    cnt = hdr.ramdisk_size;
                    emmc_read_unlzo(offset, (void*)CFG_RD_SRC_ADDR, (void*)CFG_RD_ADDR, cnt, &decomp_size, 0);

                    // 5. verify the ramdisk.
					emmc_auth_image_ex(part, CFG_RD_SRC_ADDR,  hdr.ramdisk_size, 256);

                    sprintf(env, "0x%x", CFG_RD_ADDR);
                    setenv("initrd", env);
                    sprintf(env, "0x%x", CFG_RD_ADDR+decomp_size);
                    setenv("initrd_end", env);

                    // 6. launch kernel
                    local_args[1] = local_args_temp[1];
                    local_args[2] = local_args_temp[2];
                    sprintf(local_args[1], "0x%x", addr);
                    sprintf(local_args[2], "0x%x", CFG_RD_ADDR);
                    return do_bootm(cmdtp, 0, 3, local_args);
                }
                else if (magic_found)
                {
                    printf("MINIGZIP:\n");

						#if defined(CONFIG_OF_LIBFDT)
						addr = CFG_RD_SRC_ADDR;
						#endif
                    // 2. load kernel
                    offset += hdr.page_size;
                    emmc_read(offset, (void*)addr, WITH_PADDING_SIZE(hdr.kernel_size, hdr.page_size) + 0x100);

                    // 3. verify the kernel.
                    emmc_auth_image(part, addr, hdr.page_size);
                    // 4. load ramdisk
                    offset += WITH_PADDING_SIZE(hdr.kernel_size, hdr.page_size);
                    emmc_read(offset, (void*)CFG_RD_ADDR, hdr.ramdisk_size + 0x100);
                    // 5. verify the ramdisk.
					emmc_auth_image_ex(part, CFG_RD_ADDR,  hdr.ramdisk_size, 256);

                    sprintf(env, "0x%x", CFG_RD_ADDR);
                    setenv("initrd", env);
                    sprintf(env, "0x%x", CFG_RD_ADDR+hdr.ramdisk_size);
                    setenv("initrd_end", env);

                    // 4. launch kernel
                    local_args[1] = local_args_temp[1];
                    local_args[2] = local_args_temp[2];
                    sprintf(local_args[1], "0x%x", addr);
                    sprintf(local_args[2], "0x%x", CFG_RD_ADDR);
                    return do_bootm(cmdtp, 0, 3, local_args);
                }
                else
                {
                    printf("\n no valid boot image!\n");
                    show_boot_progress(-55);
                    return 1;
                }
            }

			//printf(" load image line %d  \n",__LINE__);
            // Load rootfs
            if (((tmp = getenv("ramdisk")) != NULL) && (strcmp(tmp, "yes") == 0))
            {
				printf(" emmc_load_image\n");
                emmc_load_image(cmdtp, rootfsoffset, CFG_RD_ADDR, argv[0]);

                sprintf(env, "0x%x", CFG_RD_ADDR);
                setenv("initrd", env);
            }
            else
            {
                setenv("initrd", NULL);
				printf(" load image line %d  \n",__LINE__);
            }

			//printf(" load image line %d  \n",__LINE__);
            // Loading .lzo kernel if specified.
            tmp = strchr(argv[0], '.');
            if (tmp && !strcmp(tmp, ".lzo"))
			{

				printf(" load image from lzo \n");
                emmc_load_image_lzo(cmdtp, part, argv[0]);
			}

			//printf(" load image line %d  \n",__LINE__);
            // Load kernel
            return emmc_load_image(cmdtp, part->offset, addr, argv[0]);
        }
    }

    return 0;

usage:
    printf("Usage:\n%s\n", cmdtp->usage);
    show_boot_progress(-53);
    return 1;
}

U_BOOT_CMD(eboot, 3, 1, do_emmcboot,
    "eboot   - boot from EMMC device\n",
    "[.lzo] [partition] | [[[loadAddr] dev] offset]\n");

#endif // CC_EMMC_BOOT

