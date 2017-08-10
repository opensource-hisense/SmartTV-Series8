/*-----------------------------------------------------------------------------
 * uboot/board/mt5398/mt5398_secure_rom.h
 *
 * MT53xx Secure Boot driver
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


#ifndef SECURE_ROM_H
#define SECURE_ROM_H


//-----------------------------------------------------------------------------
// Include files
//-----------------------------------------------------------------------------
#include "x_bim.h"
//#include <stdint.h>
//-----------------------------------------------------------------------------
// Configurations
//-----------------------------------------------------------------------------
#define ROM_CODE_BASE           (0xf4000000)
//-----------------------------------------------------------------------------
// Constant definitions
//-----------------------------------------------------------------------------
#define LOADER_START_ADDR       (0x4000)
#define LOADER_LENGTH           (0x3C000 - 0x200)

#define OS_START_ADDR           (0x80000)
#define OS_LENGTH               (0x300000 - 0x200)

#define SIGNATURE_BASE_ADDR     (OS_START_ADDR + OS_LENGTH)            // 1264KB - 512bytes
#define PUBLIC_KEY_BASE_ADDR    (OS_START_ADDR + OS_LENGTH + 0x100)    // 1264KB - 256bytes

#define SHA_1_ALGORITHM_ID_SIZE (15)
#define SIGNATURE_SIZE          (64)                    // 32bit * 64

#define REG_SDATA1              (0x664)
#define SECURE_BOOT_EFUSE       (BIM_READ32(REG_SDATA1)& 0x100000)
    #define SECURE_BOOT             (0x100000)

enum
{
    VERIFY_OK,
    VERIFY_FAILED
};

#ifndef _SHA_enum_
#define _SHA_enum_
enum
{
    shaSuccess = 0,
    shaNull,            /* Null pointer parameter */
    shaInputTooLong,    /* input data too long */
    shaStateError       /* called Input after Result */
};
#endif

#define MTK_PUBLIC_KEY   (UINT32 *)(ROM_CODE_BASE + 0x3c00 + ((BIM_READ32(REG_SDATA1)>>17)& 0x3) * 256) //15KB + i * 256

#define MAX_BUF_COUNT       0x100
#define SHA1HashSize        20
#define MD5_HASH_SIZE       16

/*
 *  This structure will hold context information for the SHA-1
 *  hashing operation
 */
typedef struct SHA1Context
{
    UINT32 Intermediate_Hash[SHA1HashSize/4];   /* Message Digest  */

    UINT32 Length_Low;                          /* Message length in bits      */
    UINT32 Length_High;                         /* Message length in bits      */

                                                /* Index into message block array   */
    UINT16 Message_Block_Index;
    UINT8 Message_Block[64];                    /* 512-bit message blocks      */

    INT32 Computed;                             /* Is the digest computed?         */
    INT32 Corrupted;                            /* Is the message digest corrupted? */
} SHA1Context;

typedef struct MD5_CONTEXT_T
{
  UINT32 au4State[4];                           /* state  */
  UINT32 au4Count[2];                           /* number of bits, modulo 2^64 (lsb first) */
  UINT8 au1Buffer[64];                          /* input buffer */
} MD5_CONTEXT_T;

//-----------------------------------------------------------------------------
// Public functions
//-----------------------------------------------------------------------------
int SHA1Reset(SHA1Context *);
int SHA1Input(SHA1Context *, const UINT8 *, UINT32);
int SHA1Result(SHA1Context *, UINT8 Message_Digest[SHA1HashSize]);

void MD5_Reset(MD5_CONTEXT_T*);
void MD5_Input(MD5_CONTEXT_T *, const UINT8 *, UINT32);
void MD5_Result(MD5_CONTEXT_T *, UINT8 Message_Digest[MD5_HASH_SIZE]);

#endif //SECURE_ROM_H

