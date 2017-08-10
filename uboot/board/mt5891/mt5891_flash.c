/*
 * vm_linux/chiling/uboot/board/mt5398/mt5398_flash.c
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


#include "x_typedef.h"
#include <common.h>
#include <linux/byteorder/swab.h>

#include "nor_if.h"


//---------------------------------------------------------------------------------
flash_info_t flash_info[CFG_MAX_FLASH_BANKS];	/* info for FLASH chips    */

/*-----------------------------------------------------------------------
 * Typedef
 */


/*-----------------------------------------------------------------------
 * Functions
 */
unsigned long flash_init (void);
void flash_print_info (flash_info_t * info);
int flash_erase (flash_info_t * info, int s_first, int s_last);
int write_buff (flash_info_t * info, uchar * src, ulong addr, ulong cnt);

/*-----------------------------------------------------------------------
 */
#ifndef CC_NOR_DISABLE
#include "serialflash_hw.h"

unsigned long flash_init (void)
{
	SFLASH_INFO_T rFlash;
	int i, j;
	ulong size = 0;

	if(SFLASHHW_Init() != 0)
	{
    	flash_info[0].flash_id = FLASH_UNKNOWN;
	    printf("missing or unknown FLASH type\n");
	    return 0;
	}

	SFLASHHW_GetFlashInfo(&rFlash);

	for (i = 0; i < CFG_MAX_FLASH_BANKS; i++) 
	{
        flash_info[i].flash_id = (UINT32)((rFlash.arFlashInfo[i].u1MenuID)<<16) | 
                                 (UINT32)((rFlash.arFlashInfo[i].u1DevID1)<<8) | 
                                 (UINT32)rFlash.arFlashInfo[i].u1DevID2;
        
        flash_info[i].sector_count = rFlash.arFlashInfo[i].u4ChipSize / rFlash.arFlashInfo[i].u4SecSize;
        flash_info[i].size = rFlash.arFlashInfo[i].u4ChipSize;

        VERIFY(flash_info[i].sector_count <= CFG_MAX_FLASH_SECT);

        for(j=0; j<flash_info[i].sector_count; j++)
        {
            flash_info[i].start[j] = PHYS_FLASH_1 + (j * rFlash.arFlashInfo[i].u4SecSize);
            flash_info[i].protect[j] = 0;
        }

		size += flash_info[i].size;
	}

	/* Protect monitor and environment sectors
	 */
	flash_protect (FLAG_PROTECT_SET,
			CFG_FLASH_BASE,
			CFG_FLASH_BASE + monitor_flash_len - 1, &flash_info[0]);

	return size;
}


/*-----------------------------------------------------------------------
 */
void flash_print_info (flash_info_t * info)
{
	int i;
	UINT8 u1MenuID, u1DevID1, u1DevID2;
	CHAR *pStr;

	if (info->flash_id == FLASH_UNKNOWN) 
	{
		printf ("missing or unknown FLASH type\n");
		return;
	}

	u1MenuID = (UINT8)((info->flash_id >> 16) & 0xFF);
	u1DevID1 = (UINT8)((info->flash_id >> 8) & 0xFF);
	u1DevID2 = (UINT8)(info->flash_id & 0xFF);

	pStr = SFLASHHW_GetFlashName(u1MenuID, u1DevID1, u1DevID2);
    if(pStr != NULL)
    {
	   printf("%s", pStr);
    }
	
	printf ("  Size: %ld MB in %d Sectors\n",
			info->size >> 20, info->sector_count);

	if(info->sector_count > CFG_MAX_FLASH_SECT)
	    return;

	printf ("  Sector Start Addresses:");
	for (i = 0; i < info->sector_count; ++i) {
		if ((i % 5) == 0)
			printf ("\n   ");
		printf (" %08lX%s",
			info->start[i], info->protect[i] ? " (RO)" : "     ");
	}
	printf ("\n");
	return;
}


/*-----------------------------------------------------------------------
 */

int flash_erase (flash_info_t * info, int s_first, int s_last)
{
	int flag, prot, sect;
	ulong start, last;
	int rcode = 0;

	if ((s_first < 0) || (s_first > s_last)) {
		if (info->flash_id == FLASH_UNKNOWN) {
			printf ("- missing\n");
		} else {
			printf ("- no sectors to erase\n");
		}
		return 1;
	}

	prot = 0;
	for (sect = s_first; sect <= s_last; ++sect) {
		if (info->protect[sect]) {
			prot++;
		}
	}

	if (prot) {
		printf ("- Warning: %d protected sectors will not be erased!\n",
				prot);
	} else {
		printf ("\n");
	}

	start = get_timer (0);
	last = start;

	/* Disable interrupts which might cause a timeout here */
	flag = disable_interrupts ();
	
    if(SFLASHHW_WriteProtect(0, FALSE) != 0)
	{
		printf("Unprotect flash fail!\n");
		return 1;
	}    
	
	/* Start erase on unprotected sectors */
	for (sect = s_first; sect <= s_last; sect++) 
	{
#if CC_PROJECT_X
        if ((sect != 0) && (sect != 1))
#else /* CC_PROJECT_X */
		if (info->protect[sect] == 0)
#endif /* CC_PROJECT_X */
		{	/* not protected */

			printf ("Erasing sector %2d ... ", sect);

			/* arm simple, non interrupt dependent timer */
			reset_timer_masked ();

			SFLASHHW_EraseSector(0, info->start[sect] - PHYS_FLASH_1);

			printf (" done\n");
		}
	}
	return rcode;
}

/*-----------------------------------------------------------------------
 * Copy memory to flash, returns:
 * 0 - OK
 * 1 - write timeout
 * 2 - Flash not erased
 * 4 - Flash not identified
 */

int write_buff (flash_info_t * info, uchar * src, ulong addr, ulong cnt)
{
    UINT32 i, u4WriteAddr, u4Len, u4WriteByte, u4SectStart, u4SectEnd;

    if (info->flash_id == FLASH_UNKNOWN) 
    {
		return 4;
	}

    u4WriteAddr = addr;
    u4WriteByte = cnt;
    u4SectStart = u4SectEnd = 0;
    
    if(info->sector_count > CFG_MAX_FLASH_SECT)
        return 1;
    
    if(SFLASHHW_WriteProtect(0, FALSE) != 0)
	{
		printf("Unprotect flash fail!\n");
		return 1;
	}    

    for(i=0; i<info->sector_count;i++)
    {
        u4SectStart = info->start[i];
        if(i == (info->sector_count - 1))
        {
            u4SectEnd = (info->size + PHYS_FLASH_1);
        }
        else if(i < (info->sector_count - 1))
        {
            u4SectEnd = info->start[i+1];
        }
        else
        {
        }

        if((u4SectStart<=u4WriteAddr)&&(u4WriteAddr<u4SectEnd))
        {
            u4Len = u4SectEnd - u4WriteAddr;
            if(u4Len >= u4WriteByte)
                u4Len = u4WriteByte;
        	
            if(SFLASHHW_WriteSector(0, u4WriteAddr - PHYS_FLASH_1, u4Len, src) != 0)
            {
                printf("Write flash error !\n");
                return 1;
            }

            u4WriteAddr += u4Len;                
            u4WriteByte -= u4Len;
            src += u4Len;

            if(u4WriteByte == 0)
            {
                return 0;
            }
        }
    }

    return 1;
}
#else

unsigned long flash_init (void)
{
	return 0;
}

void flash_print_info (flash_info_t * info)
{
	return 0;

}
int flash_erase (flash_info_t * info, int s_first, int s_last)
{
	return 0;
}

int write_buff (flash_info_t * info, uchar * src, ulong addr, ulong cnt)
{
	return 0;
}

#endif


