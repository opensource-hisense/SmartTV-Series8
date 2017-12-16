/*-----------------------------------------------------------------------------
 * uboot/board/mt5398/mt5398_secure_rom.c
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


#ifdef CC_SECURE_ROM_BOOT

//-----------------------------------------------------------------------------
// Include files
//-----------------------------------------------------------------------------
#include "mt5891_secure_rom.h"
#include "../include/configs/mt5891.h"
#include "bignum.h"
#include "x_ckgen.h"
#include "x_ldr_env.h"

#include <common.h>

#if defined CC_EMMC_BOOT
#include <mmc.h>
#else
#include <nand.h>
#endif

#ifdef CC_A1_SECURE_BOOT
#define CC_USE_SHA1
#define CC_USE_HW_SHA1

#include <partinfo.h>
#include "lg_modeldef.h"
#include "gcpu_if.h"

extern void writeFullVerifyOTP(void);
#endif // CC_A1_SECURE_BOOT

#define CC_USE_SHA256

// the customer public key is copied from CC_LDR_ENV_OFFSET in mt53xx_sif.c
extern LDR_ENV_T *_prLdrEnv;

int IsRunOnUsb(char* uenv, int uenv_size)
{
    static int run_on_usb = 0;
    static int fgInit = 0;

    if (!fgInit)
    {
	    LDR_ENV_T *prLdrEnv = (LDR_ENV_T *)CC_LDR_ENV_OFFSET;

	    if (prLdrEnv->u4CfgFlag == USIG_RUN_ON_USB)
	    {
	        if (uenv)
	        {
	            strncpy(uenv, prLdrEnv->szUenv, uenv_size);
	        }

	        run_on_usb = 1;
	    }

	    fgInit = 1;
    }

    return run_on_usb;
}


#if defined(CC_USE_SHA256)
#define SHA1_HASH_SIZE      (20)
#define SHA224_HASH_SIZE    (28)
#define SHA256_HASH_SIZE    (32)
#define SHA_BLOCK_SIZE      (64)    //512-bit

// This structure will hold context information for the SHA-256
// hashing operation
typedef struct
{
    UINT32 Intermediate_Hash[SHA256_HASH_SIZE/4];   /* Message Digest  */

    UINT32 Length_Low;                              /* Message length in bits      */
    UINT32 Length_High;                             /* Message length in bits      */

                                                    /* Index into message block array   */
    INT32 Message_Block_Index;
    UINT8 Message_Block[64];                        /* 512-bit message blocks      */

    INT32 Computed;                                 /* Is the digest computed?         */
    INT32 Corrupted;                                /* Is the message digest corrupted? */
} SHA256_CONTEXT_T;

#define x_memset memset

// Constants defined in SHA-256
const UINT32 K[] = {
	0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
	0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
	0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
	0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
	0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
	0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
	0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
	0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
	0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
	0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
	0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
	0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
	0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
	0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
	0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
	0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

//-----------------------------------------------------------------------------
// Type definitions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Macro definitions
//-----------------------------------------------------------------------------

/*
 *  Define the SHA256
 */
#define	ROTATER(x,n)	(((x) >> (n)) | ((x) << (32 - (n))))
#define	SHIFTR(x,n)		((x) >> (n))

#define	CH(x,y,z)		(((x) & (y)) ^ (~(x) & (z)))
#define	MAJ(x,y,z)		(((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define	SUM0(x)			(ROTATER((x), 2) ^ ROTATER((x), 13) ^ ROTATER((x), 22))
#define	SUM1(x)			(ROTATER((x), 6) ^ ROTATER((x), 11) ^ ROTATER((x), 25))
#define	RHO0(x)			(ROTATER((x), 7) ^ ROTATER((x), 18) ^ SHIFTR((x), 3))
#define	RHO1(x)			(ROTATER((x), 17) ^ ROTATER((x), 19) ^ SHIFTR((x), 10))

//-----------------------------------------------------------------------------
// Static variables
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Static functions
//-----------------------------------------------------------------------------

void SHA256PadMessage(SHA256_CONTEXT_T *);
void SHA256ProcessMessageBlock(SHA256_CONTEXT_T *);

/*
 *  SHA256_Reset
 *
 *  Parameters:
 *      context: [in/out]
 *          The context to reset.
 *
 *  Returns:
 *      sha Error Code.
 *
 */
INT32 SHA256_Reset(SHA256_CONTEXT_T *context)
{
    if (!context)
    {
        return shaNull;
    }

    context->Length_Low             = 0;
    context->Length_High            = 0;
    context->Message_Block_Index    = 0;

    context->Intermediate_Hash[0]   = 0x6a09e667;
    context->Intermediate_Hash[1]   = 0xbb67ae85;
    context->Intermediate_Hash[2]   = 0x3c6ef372;
    context->Intermediate_Hash[3]   = 0xa54ff53a;
    context->Intermediate_Hash[4]   = 0x510e527f;
    context->Intermediate_Hash[5]   = 0x9b05688c;
    context->Intermediate_Hash[6]   = 0x1f83d9ab;
    context->Intermediate_Hash[7]   = 0x5be0cd19;

    context->Computed   = 0;
    context->Corrupted  = 0;

    return shaSuccess;
}

/*
 *  SHA256_Result
 *
 *  Parameters:
 *      context: [in/out]
 *          The context to use to calculate the SHA-256 hash.
 *      Message_Digest: [out]
 *          Where the digest is returned.
 *
 *  Returns:
 *      sha Error Code.
 *
 */
INT32 SHA256_Result( SHA256_CONTEXT_T *context,
                UINT8 Message_Digest[SHA256_HASH_SIZE])
{
    INT32 i;

    if (!context || !Message_Digest)
    {
        return shaNull;
    }

    if (context->Corrupted)
    {
        return context->Corrupted;
    }

    if (!context->Computed)
    {
        SHA256PadMessage(context);
        for(i=0; i<64; ++i)
        {
            /* message may be sensitive, clear it out */
            context->Message_Block[i] = 0;
        }
        context->Length_Low = 0;    /* and clear length */
        context->Length_High = 0;
        context->Computed = 1;

    }

    for(i = 0; i < SHA256_HASH_SIZE; ++i)
    {
        Message_Digest[i] = context->Intermediate_Hash[i>>2]
                            >> 8 * ( 3 - ( i & 0x03 ) );
    }

    return shaSuccess;
}

/*
 *  SHA256_Input
 *
 *  Description:
 *      This function accepts an array of octets as the next portion
 *      of the message.
 *
 *  Parameters:
 *      context: [in/out]
 *          The SHA context to update
 *      message_array: [in]
 *          An array of characters representing the next portion of
 *          the message.
 *      length: [in]
 *          The length of the message in message_array
 *
 *  Returns:
 *      sha Error Code.
 *
 */
INT32 SHA256_Input(SHA256_CONTEXT_T *context, const UINT8 *message_array, UINT32 length)
{
    if (!length)
    {
        return shaSuccess;
    }

    if (!context || !message_array)
    {
        return shaNull;
    }

    if (context->Computed)
    {
        context->Corrupted = shaStateError;

        return shaStateError;
    }

    if (context->Corrupted)
    {
         return context->Corrupted;
    }
    while(length-- && !context->Corrupted)
    {
        context->Message_Block[context->Message_Block_Index++] =
                        (*message_array & 0xFF);

        context->Length_Low += 8;
        if (context->Length_Low == 0)
        {
            context->Length_High++;
            if (context->Length_High == 0)
            {
                /* Message is too long */
                context->Corrupted = 1;
            }
        }

        if (context->Message_Block_Index == 64)
        {
            SHA256ProcessMessageBlock(context);
        }

        message_array++;
    }

    return shaSuccess;
}

/*
 *  SHA256ProcessMessageBlock
 *
 *  Description:
 *      This function will process the next 512 bits of the message
 *      stored in the Message_Block array.
 *
 *  Parameters:
 *      None.
 *
 *  Returns:
 *      Nothing.
 *
 *  Comments:

 *      Many of the variable names in this code, especially the
 *      single character names, were used because those were the
 *      names used in the publication.
 *
 *
 */
void SHA256ProcessMessageBlock(SHA256_CONTEXT_T *context)
{
    INT32       t;                      /* Loop counter                */
    UINT32      temp, temp2;            /* Temporary word value        */
    UINT32      W[64];                  /* Word sequence               */
    UINT32      A, B, C, D, E, F, G, H; /* Word buffers                */

    /*
     *  Initialize the first 16 words in the array W
     */
    for(t = 0; t < 16; t++)
    {
        W[t] = (context->Message_Block[t * 4 + 0] << 24) |
               (context->Message_Block[t * 4 + 1] << 16) |
               (context->Message_Block[t * 4 + 2] <<  8) |
               (context->Message_Block[t * 4 + 3]);
    }

	for(t = 16; t < 64; ++t)
	{
		W[t] = RHO1(W[t - 2]) + W[t - 7] + RHO0(W[t - 15]) + W[t - 16];
	}

    A = context->Intermediate_Hash[0];
    B = context->Intermediate_Hash[1];
    C = context->Intermediate_Hash[2];
    D = context->Intermediate_Hash[3];
    E = context->Intermediate_Hash[4];
    F = context->Intermediate_Hash[5];
    G = context->Intermediate_Hash[6];
    H = context->Intermediate_Hash[7];

	for(t = 0; t < 64; ++t)
	{
		temp = H + SUM1(E) + CH(E, F, G) + K[t] + W[t];
		temp2 = SUM0(A) + MAJ(A, B, C);
		H = G;
		G = F;
		F = E;
		E = D + temp;
		D = C;
		C = B;
		B = A;
		A = temp + temp2;
	}

    context->Intermediate_Hash[0] += A;
    context->Intermediate_Hash[1] += B;
    context->Intermediate_Hash[2] += C;
    context->Intermediate_Hash[3] += D;
    context->Intermediate_Hash[4] += E;
    context->Intermediate_Hash[5] += F;
    context->Intermediate_Hash[6] += G;
    context->Intermediate_Hash[7] += H;

    context->Message_Block_Index = 0;
}

/*
 *  SHA256PadMessage
 *

 *  Description:
 *      According to the standard, the message must be padded to an even
 *      512 bits.  The first padding bit must be a '1'.  The last 64
 *      bits represent the length of the original message.  All bits in
 *      between should be 0.  This function will pad the message
 *      according to those rules by filling the Message_Block array
 *      accordingly.  It will also call the ProcessMessageBlock function
 *      provided appropriately.  When it returns, it can be assumed that
 *      the message digest has been computed.
 *
 *  Parameters:
 *      context: [in/out]
 *          The context to pad
 *      ProcessMessageBlock: [in]
 *          The appropriate SHA*ProcessMessageBlock function
 *  Returns:
 *      Nothing.
 *
 */

void SHA256PadMessage(SHA256_CONTEXT_T *context)
{
    /*
     *  Check to see if the current message block is too small to hold
     *  the initial padding bits and length.  If so, we will pad the
     *  block, process it, and then continue padding into a second
     *  block.
     */
    if (context->Message_Block_Index > 55)
    {
        context->Message_Block[context->Message_Block_Index++] = 0x80;
        while(context->Message_Block_Index < 64)
        {
            context->Message_Block[context->Message_Block_Index++] = 0;
        }

        SHA256ProcessMessageBlock(context);

        while(context->Message_Block_Index < 56)
        {
            context->Message_Block[context->Message_Block_Index++] = 0;
        }
    }
    else
    {
        context->Message_Block[context->Message_Block_Index++] = 0x80;
        while(context->Message_Block_Index < 56)
        {

            context->Message_Block[context->Message_Block_Index++] = 0;
        }
    }

    /*
     *  Store the message length as the last 8 octets
     */
    context->Message_Block[56] = context->Length_High >> 24;
    context->Message_Block[57] = context->Length_High >> 16;
    context->Message_Block[58] = context->Length_High >> 8;
    context->Message_Block[59] = context->Length_High;
    context->Message_Block[60] = context->Length_Low >> 24;
    context->Message_Block[61] = context->Length_Low >> 16;
    context->Message_Block[62] = context->Length_Low >> 8;
    context->Message_Block[63] = context->Length_Low;

    SHA256ProcessMessageBlock(context);
}


INT32 SHA256(const UINT8 *message_array, UINT32 length, UINT8 Message_Digest[SHA256_HASH_SIZE])
{
    SHA256_CONTEXT_T rContext;
    if(SHA256_Reset(&rContext) != shaSuccess)
    {
        //LOG(5, "%s Fail to reset.", __FUNCTION__);
        return shaNull;
    }

    if(SHA256_Input(&rContext, message_array, length) != shaSuccess)
    {
        //LOG(5, "%s Fail to input.", __FUNCTION__);
        return shaStateError;
    }

    if(SHA256_Result(&rContext, Message_Digest) != shaSuccess)
    {
        //LOG(5, "%s Fail to get result.", __FUNCTION__);
        return shaNull;
    }

    return shaSuccess;
}
#endif // CC_USE_SHA256

#if defined(CC_USE_CHKSUM)
int x_chksum(UINT8 *pu1MessageDigest, UINT32 u4StartAddr, UINT32 u4Size)
{
    UINT32 u4CheckSum = 0;
    UINT32 i;

    for (i = 0; i < u4Size; i+=4)
    {
        u4CheckSum += *(UINT32 *)(u4StartAddr + i);
    }

    memcpy(pu1MessageDigest, &u4CheckSum, 4);

    return 0;
}
#endif


#define DWORDSWAP(u4Tmp) (((u4Tmp & 0xff) << 24) | ((u4Tmp & 0xff00) << 8) | ((u4Tmp & 0xff0000) >> 8) | ((u4Tmp & 0xff000000) >> 24))

BOOL DataSwap(UINT32 *pu4Dest, UINT32 *pu4Src, UINT32 u4Size, UINT32 u4Mode)
{
    INT32 i = 0;
    UINT32 u4Tmp;

    if (u4Mode == 0)
    {
        for (i = 0; i < u4Size; i++) //memcpy
        {
            *(pu4Dest + i) = *(pu4Src + i);
        }
    }
    else if (u4Mode == 1) //Endien Change
    {
        for (i = 0; i < u4Size; i++)
        {
            u4Tmp = *(pu4Src + i);
            u4Tmp = DWORDSWAP(u4Tmp);
            *(pu4Dest + i) = u4Tmp;
        }
    }
    else if (u4Mode == 2) //Head Swap
    {
        for (i = 0; i < u4Size; i++)
        {
            *(pu4Dest + u4Size - 1 - i) = *(pu4Src + i);
        }
    }
    else if (u4Mode == 3) //Head Swap + Endien Change
    {
        for (i = 0; i < u4Size; i++)
        {
            u4Tmp = *(pu4Src + i);
            u4Tmp = DWORDSWAP(u4Tmp);
            *(pu4Dest + u4Size - 1 - i) = u4Tmp;
        }
    }

    return TRUE;
}

void ExtractPKCSblock(UINT32 au4CheckSum[], UINT8 au1ExtractMsg[])
{
	UINT8 au1Block[256];
    INT32 i;

    memset(au1Block, 0, 256);
    DataSwap((UINT32 *)au1Block, au4CheckSum, 64, 3);
    if (au1Block[0] == 0x00 || au1Block[1] == 0x02)
    {
    	for (i = 2; i < 256; i++)
    	{	/* Look for zero separating byte */
    		if (au1Block[i] == 0x00)
    			break;
    	}

    	if (((i+1+SHA_1_ALGORITHM_ID_SIZE) >= 256) || ((256-i-1-SHA_1_ALGORITHM_ID_SIZE) > SHA1HashSize))
    	{
    		printf("ERROR: failed to find message in decrypted block %d.\n", i);
    	}
    	else
    	{
    		memcpy(au1ExtractMsg, &au1Block[i+1+SHA_1_ALGORITHM_ID_SIZE], 256 - i - 1 - SHA_1_ALGORITHM_ID_SIZE);
    	}
    }
    else
    {
        printf("ERROR: Bad data entryption.\n");
    }
}

//---------------------------------------------------------------------
// Function    : RSA
// Description : Verify signature by RSA
// Parameter   :
//		pu1Signature	[in]: The signature must be 2048bit
//      pu4CheckSum     [in/out]: Checksum value must be 2048bit
// Return      : verify OK or not
//---------------------------------------------------------------------
INT32 RSADecryption(UINT32 *pu1Signature, UINT32 *pu4PublicKey, UINT32 *pu4CheckSum)
{
    mpi s;
    mpi n;
    mpi t1, t2;
    UINT32 t1_4096[128];
    UINT32 t2_4096[128];

    s.s = 1;
    s.n = 64;
    s.p = pu1Signature;
    n.s = 1;
    n.n = 64;
    n.p = pu4PublicKey;
    t1.s = 1;
    t1.n = 128;
    t1.p = t1_4096;
    t2.s = 1;
    t2.n = 128;
    t2.p = t2_4096;

    // m = s ^ 3 mod n
    //y = s * s mod n
    mpi_mul_mpi(&t1, &s, &s);
    mpi_mod_mpi(&t2, &t1, &n);

    //y = y * s mode n
    mpi_mul_mpi(&t1, &t2, &s);
    mpi_mod_mpi(&t2, &t1, &n);

    memcpy(pu4CheckSum, t2_4096, 256);

    return VERIFY_OK;
}

void sig_authetication(UINT32 u4StartAddr, UINT32 u4Size, UINT32 *pu4Signature, UINT32 *pu4EncryptedKey)
{
    printf("signature verification ");

    if (SECURE_BOOT_EFUSE == SECURE_BOOT)
    {
        //UINT32 au4MiscBlock[64]; // 2048bit key
    	UINT8 au1Block[256];
        BYTE SHA256_Digest[SHA256_HASH_SIZE];
        LDR_ENV_T* prLdrEnv = _prLdrEnv;
        UINT32 au4CheckSum[SIGNATURE_SIZE];
        INT32 i4Ret = 0;

        memcpy((void *)au4CheckSum, (void *)prLdrEnv->au4CustKey, sizeof(prLdrEnv->au4CustKey));

        // RSA verification via au4Signature and au4CheckSum(public key)
        // Then save the decrypted PKCS block into au4CheckSum
        i4Ret = RSADecryption(pu4Signature, au4CheckSum, au4CheckSum);
        if (i4Ret != VERIFY_OK)
        {
            goto _HALT;
        }

        /*
        Len  1  1       221        1               32
            +--+--+---------------+--+-----------------------------+
            |02|00|PADDING MISC V1|00|      SHA256 Of binary       |
            +--+--+---------------+--+-----------------------------+
        */

        memset(au1Block, 0, 256);
        DataSwap((UINT32 *)au1Block, au4CheckSum, 64, 3);
        if (au1Block[0] == 0x02 || au1Block[1] == 0x00)
        {
            GCPU_LoaderInit(0);
            GCPU_SHA256(u4StartAddr, u4Size, SHA256_Digest);
            if (memcmp(SHA256_Digest, (au1Block + 224), SHA256_HASH_SIZE) != 0)
            {
                printf("failed - SHA of Img is wrong\n");
                goto _HALT;
            }
            else
            {
                printf("is OK\n");
                return;
            }
        }
        else
        {
            printf("failed - Img Misc header is wrong\n");
            goto _HALT;
        }
    }
    else
    {
    	printf("is bypassed\n");
        return;
    }

    return;

_HALT:
    printf("System is halted\n");
    while(1);

    return;
}


// A1 secure flow----------------------------------------------------------------------------------------------------------------------
#ifdef CC_A1_SECURE_BOOT

#define BUFFER_ADDRESS  0x4000000
#define SIG_SIZE        256
#define PAD_ALIGN       4095 //4k

//#define SECURE_DEBUG

int gSecureFlag = 0;
extern int do_cp2ram(cmd_tbl_t *, int, int, char *[]);

static void dumpBinary(unsigned char* bin, int size, char* name)
{
#ifdef SECURE_DEBUG
    int i = 0;
    printf("%s=", name);

    for (i = 0; i < size; i++)
    {
        if (i%16 == 0)
        {
            printf("\n");
        }
        printf("%02x ", bin[i]);
    }

    printf("\n");
#endif
}

int getFullVerifyOTP(void)
{
#ifdef FULL_VERIFY_OFF
    printf("getOTP: return 0 forcely by FULL_VERIFY_OFF\n");
    return 0;
#else
    printf("getOTP: value = 0x%x\n", BIM_READ32(0x664));
    return (BIM_READ32(0x664) & 0x1);
#endif
}

int setFullVerifyOTP(void)
{
    UINT32 u4Val;

    // switch bus clock to xtal clock.
    u4Val = CKGEN_READ32(0x208);
    CKGEN_WRITE32(0x208, u4Val & ~0x700);

    // set key, enable in-system e-fuse blowing function.
    BIM_WRITE32(0x688, 0x716);

    // set counter for e-fuse programming timing requirement.
    BIM_WRITE32(0x6B4, 0x80CD9020);

    // turn on burn mode.
    BIM_WRITE32(0x6B0, 0x1);

    // delay 5us for power
    udelay(5);

    // trigger
    BIM_WRITE32(0x6B0, 0x11);

    // polling bit 4, indicate blowing e-fuse is completed.
    while (BIM_READ32(0x6B0) & 0x10);

    // set key, disable in-system e-fuse blowing function.
    BIM_WRITE32(0x688, 0x0);

    // refresh e-fuse.
    BIM_WRITE32(0x6B4, 0x0);
    BIM_WRITE32(0x6B4, 0x80000000);

    // Restore bus clock.
    CKGEN_WRITE32(0x208, u4Val);

    printf("Set partial verification OTP done, 0x%08X!\n", BIM_READ32(0x664));
}

// return 0 -> OK, return -1 -> fail
int copyFlash(const char* szPartName, unsigned char* pu1DestDram, unsigned int u4FlashOffset, unsigned int u4Size)
{
    char* pargs[5];
    char* local_args[4][32];

    sprintf(local_args[0], "%s",   szPartName);
    sprintf(local_args[1], "0x%x", pu1DestDram);
    sprintf(local_args[2], "0x%x", u4FlashOffset);
    sprintf(local_args[3], "0x%x", u4Size);

    pargs[0] = 0;
    pargs[1] = local_args[0];
    pargs[2] = local_args[1];
    pargs[3] = local_args[2];
    pargs[4] = local_args[3];

    return do_cp2ram(NULL, 0, 5, pargs);
}

// return 0 -> OK, return -1 -> fail
static int getFlashPartFileSize(const char* szPartName, unsigned int* pu4Size)
{
    struct partition_info* mpi = get_partition_by_name(szPartName);

    if (mpi == NULL)
    {
        printf("^R^failed to get partition free\n");
        return -1;
    }

    if (mpi->valid == NO)
    {
        printf("^R^Partition is Not valid! => Skip!");
        return -1;
    }

    if (!mpi->filesize)
    {
        printf("^g^File image size is Zero, Using partition size!!");
        printf("sizeof mpi->size : %d", sizeof(mpi->size));
        *pu4Size = mpi->size;
    }
    else
    {
        *pu4Size = mpi->filesize;
    }

    return 0;
}

unsigned int random(void)
{
    return BIM_READ32(REG_RW_TIMER2_LOW);
}

int verifySignature(unsigned int u4StartAddr, unsigned int u4Size, unsigned char *pu1EncryptedSignature)
{
    BYTE au1ExtractMsg[SHA1HashSize];
    BYTE au1MessageDigest[SHA1HashSize];
    LDR_ENV_T* prLdrEnv = (LDR_ENV_T*)0xfb005000;

    printf("SECURE_BOOT_EFUSE : 0x%x\n", SECURE_BOOT_EFUSE);
    printf("SECURE_BOOT : 0x%x\n", SECURE_BOOT);

    if (SECURE_BOOT_EFUSE == SECURE_BOOT)
    {
        UINT32 au4CheckSum[SIGNATURE_SIZE];
        INT32 i4Ret = -1;

#ifdef SECURE_DEBUG
        printf("verifySignature u4StartAddr=%x, u4Size=%x\n", u4StartAddr, u4Size);
#endif
        memcpy((void*)au4CheckSum, (void*)prLdrEnv->au4CustKey, sizeof(prLdrEnv->au4CustKey));

        dumpBinary((unsigned char*)pu1EncryptedSignature, SIG_SIZE, "encrypted signature");
        dumpBinary((unsigned char*)au4CheckSum, SIG_SIZE, "public key");
        dumpBinary((unsigned char*)u4StartAddr, 4096, "signed image");

        // RSA verification via au4Signature and au4CheckSum(public key)
        // Then save the decrypted PKCS block into au4CheckSum
        i4Ret = RSADecryption((UINT32*)pu1EncryptedSignature, au4CheckSum, au4CheckSum);
        if (i4Ret != VERIFY_OK)
        {
            printf("RSADecryption error\n");
            return -2;
        }

        ExtractPKCSblock(au4CheckSum, au1ExtractMsg);
        dumpBinary((unsigned char*)au1ExtractMsg, SHA1HashSize, "au1ExtractMsg");

        memset(au1MessageDigest, 0, SHA1HashSize);

#if defined(CC_USE_SHA1)
        X_CalculateSHA1(au1MessageDigest, u4StartAddr, u4Size);
#else
        x_chksum(au1MessageDigest, u4StartAddr, u4Size);
#endif

        dumpBinary((unsigned char*)au1MessageDigest, SHA1HashSize, "au1MessageDigest");

        if (memcmp(au1ExtractMsg, au1MessageDigest, SHA1HashSize) != 0)
        {
            printf("signature check fail!\n");
            writeFullVerifyOTP();

            while(1);
            return -1;
        }
        else
        {
            printf("SOK\n");
            return 0;
        }
    }
    else
    {
	    printf("Secure boot\n");
        return 0;
    }
}

int verifyPartition(const char *szPartName, ulong addr, unsigned int preloaded)
{
    unsigned int offset = 0, image_size = 0;
    unsigned char *pu1Image;
    int ret = -1;
    unsigned char au1EncryptedSignature[SIG_SIZE];

	printf("full verify ~~ \n");

    // 0. check if partition exist
    if (getFlashPartFileSize(szPartName, &image_size) != 0)
    {
        printf("partition name doesn't exist\n");
        return -1;
    }

    if (preloaded)
    {
        pu1Image = (unsigned char *)addr;
    }
    else
    {
        // 1. copy image
        pu1Image = (unsigned char *)addr;
        if (copyFlash(szPartName, pu1Image, 0, image_size) != 0)
        {
            printf("copy image fail\n");
            return -1;
        }
    }

    // 2. get encrypted signature
     memcpy((void*)au1EncryptedSignature, (void*)(pu1Image+image_size-SIG_SIZE), SIG_SIZE);

    // 3. verify signature
#ifdef SIGN_USE_PARTIAL
    ret = verifySignature((unsigned int)pu1Image, image_size - (SIG_SIZE*21), au1EncryptedSignature);
#else
	ret = verifySignature((unsigned int)pu1Image, image_size - SIG_SIZE, au1EncryptedSignature);
#endif

    return ret;
}

int verifyPartialPartition(const char *szPartName, ulong addr, unsigned int preloaded)
{
    unsigned int i = 0, offset = 0, seed = 0, image_size = 0, group_num;
    int ret = -1;
    unsigned char *pu1AllFrag;
    unsigned char au1EncryptedSignature[SIG_SIZE];
	unsigned int u4FragSize = 4096;
	unsigned int u4FragNum = 20;
	struct partition_info *pi = NULL;

	pi = get_used_partition(szPartName);

	polling_timer();

	printf("partial verify ~~ \n");

    // 0. check if partition exist
    if (getFlashPartFileSize(szPartName, &image_size) != 0)
    {
        printf("partition name doesn't exist\n");
        return -1;
    }

    // if image size is smaller less than group size, do full verification
    group_num = (image_size - ((u4FragNum+1)*256)) / (u4FragNum*u4FragSize);
    if (group_num == 0)
    {
        return verifyPartition(szPartName, addr, preloaded);
    }

    // 1. generate a random number and allocate buffer
    seed = random() % u4FragNum;
    printf("part=%d\n", seed);

	pu1AllFrag = (unsigned char *)addr;

    if (pu1AllFrag == 0)
    {
        printf("malloc fail\n");
        return -1;
    }

	// 2. copy u4FragSize(4K) every u4FragSize*u4FragNum bytes
	printf("group_num = %d\n",group_num);
	printf("preloaded = %d\n", preloaded);
	printf("[%d]:%s read fragments start\n", readMsTicks(), pi->name);
	if (preloaded)
	{
		for (i = 0; i < group_num; i++)
	    {
			offset = addr + (i*u4FragNum + seed) * u4FragSize; //get sigend partial fragment
			memcpy((void *)(pu1AllFrag+(u4FragSize*i)),(void *)offset,u4FragSize); //copy fragment to buffer
		}
	}
	else
	{
	    for (i = 0; i < group_num; i++)
	    {
	        // this is flash offset
	        offset = (i*u4FragNum + seed) * u4FragSize;
			ret = emmc_read((unsigned long)pi->offset + offset, u4FragSize, pu1AllFrag+(u4FragSize*i));
			if (ret)
			{
				printf("block read failed..\n");
				return 1;
			}
	    }
	}
	printf("[%d]:%s read fragments end\n", readMsTicks(), pi->name);
	polling_timer();

	// 3. get encrypted signature
	if (preloaded)
	{
		offset = addr + image_size - SIG_SIZE*(u4FragNum + 1 - seed);
		memcpy((void*)au1EncryptedSignature, (void*)offset, SIG_SIZE);
	}
	else
	{
	    offset = image_size - SIG_SIZE*(u4FragNum + 1 - seed);

		ret = emmc_read((unsigned long)pi->offset + offset, SIG_SIZE, au1EncryptedSignature);
		if (ret)
		{
			printf("block read failed..\n");
			return 1;
		}
	}

	polling_timer();
	dumpBinary((unsigned char*)pu1AllFrag, u4FragSize, "before verfiysiganture start");

    // 4. verify signature
    ret = verifySignature((unsigned int)pu1AllFrag, u4FragSize*group_num, au1EncryptedSignature);
	polling_timer();

    return ret;
}

int do_verification(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]) // should be used integer type function
{
	char pname[20] = {0,};
	ulong addr;
	unsigned char partial_verify = 0;

	if (argc != 4)
	{
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

	partial_verify = simple_strtoul(argv[1], NULL, 16);
	strcpy(pname, argv[2]);
	printf("pname = %s \n", pname);
	addr = simple_strtoul(argv[3], NULL, 16);
	printf("addr = 0x%x \n", addr);
	printf("[%d]:%s verification start \n", readMsTicks(), pname);

	if (!strcmp("kernel", pname))
    {
		gSecureFlag |= SECURE_FLG_KERNEL;
    }
	else
    {
		gSecureFlag |= SECURE_FLG_OTHERS;
    }

#ifdef SIGN_USE_PARTIAL
		if (getFullVerifyOTP())
		{
			verifyPartition(pname, addr, 1);
		}
		else
		{
			if (!strcmp("kernel", pname))
		    {
				verifyPartition(pname, addr, 1);
		    }
			else
		    {
				verifyPartialPartition(pname, addr, 1);
		    }
		}
#else
		if (partial_verify)
	    {
			verifyPartialPartition(pname, addr, 1);
	    }
		else
	    {
			verifyPartition(pname, addr, 1);
	    }
#endif

	printf("[%d]:%s verification end\n", readMsTicks(), pname);
	return 0;
}

U_BOOT_CMD(
	verification,	4,	0,	do_verification,
	"verification\t- cp2ram 1 partition to ram and verification signature\n",
);


extern int DDI_OTP_WR_Enable(uchar ctrl);
int do_controlFullVerifyOtp(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]) // should be used integer type function
{
	int otpval;
	int set;

	if (argc != 2)
	{
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

	set = simple_strtoul(argv[1], NULL, 16);
	if ( set != 0 && set != 1)
	{
		printf (" 0 or 1 is available\n");
	}

	if(set)
	{
		printf("start set full verify OTP\n");
		DDI_OTP_WR_Enable(0);
		udelay(50000);
		setFullVerifyOTP();
		udelay(10000);
		DDI_OTP_WR_Enable(1);
		printf("end set full verify OTP\n");
	}
	else
	{
		otpval = getFullVerifyOTP();
		printf("full verify OTP value = %d\n", otpval);
	}

	return 0;
}

U_BOOT_CMD(
     ctrlotp,   2,  0,  do_controlFullVerifyOtp,
     "ctrlotp\t- [0: get|1: set]set or get full verify otp flag\n",
);

typedef struct
{
	const char* part_name;
	uint32_t	mode;
} verify_list_t;

/* Only For Test in Citrix */
static verify_list_t verify_list[] =
{
	{"lginit",		BOOT_COLD | BOOT_SNAPSHOT},
	{"rootfs",		BOOT_COLD | BOOT_SNAPSHOT},
	{"vendor",		BOOT_COLD},
	{"patch",		BOOT_COLD},
	{"opensrc",		BOOT_COLD},
	{"swue",		BOOT_COLD},
	{"lglib",		BOOT_COLD},
	{"tzfw",		BOOT_COLD | BOOT_SNAPSHOT},
	{"emanual",		BOOT_COLD},
	{"estreamer",	BOOT_COLD},
	{"base",		BOOT_COLD},
	{"extra",		BOOT_COLD},
};

static int is_verify_part(int idx, int boot_mode)
{
	if( boot_mode & verify_list[idx].mode)
	{
		return 1;
	}

	return 0;
}

extern void polling_timer(void);
extern int do_cp2ramz (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]);
int verify_apps(int boot_mode)
{
	int			idx, ret;
	ulong		offset;
	uint32_t	filesize, imgsize;
	uint32_t	flags;
	ulong 		addr = 0x8000000;
	uint32_t	loop_cnt;
	uint32_t	i;
	char* pargs[4] = {"cp2ramz", "kernel", "0x7000000", "0x7FC0"};
	struct partition_info *pi = NULL;

	loop_cnt = sizeof(verify_list)/sizeof(verify_list_t);

	printf("verify_apps: loop_cnt = %d\n", loop_cnt);
	printf("verify_apps: boot_mode = %d\n", boot_mode);

	for (i = 0; i < loop_cnt; i++)
	{
		if (getFullVerifyOTP() || is_verify_part(i, boot_mode))
		{
			pi = get_used_partition(verify_list[i].part_name);

			printf("Verifying image in the '%s' partition\n", pi->name);

			// Load image
			printf("Loading address = 0x%x emmc offset = 0x%01x%08x filesize = 0x%x\n",
			    addr, U64_UPPER(pi->offset), U64_LOWER(pi->offset), pi->filesize);

			if (!DDI_NVM_GetSWUMode() && !strcmp(verify_list[i].part_name, "swue")) //not swum on case
			{
				printf("skip swue verification ...\n");
				continue;
			}

			if (!getFullVerifyOTP() && !DDI_NVM_GetInstopStatus())
			{
                if (!strcmp(verify_list[i].part_name,"patch")     ||
                    !strcmp(verify_list[i].part_name,"emanual")   ||
                    !strcmp(verify_list[i].part_name,"estreamer") ||
                    !strcmp(verify_list[i].part_name,"extra"))
				{
					printf("skip %s verification ...\n",verify_list[i].part_name);
					continue;
				}
			}

			polling_timer();

			// Verify image
			printf("[%d]:%s verification start\n", readMsTicks(), pi->name);
			if (getFullVerifyOTP())
			{
				verifyPartition(pi->name, addr, 0);
			}
			else
			{
				verifyPartialPartition(pi->name, addr, 0);
			}
			printf("[%d]:%s verification end\n", readMsTicks(), pi->name);
		}
	}

	gSecureFlag |= SECURE_FLG_OTHERS;
	printf("Application integrity verified\n");
	return 0;
}

void writeFullVerifyOTP(void)
{
	printf("start set full verify OTP\n");
	if (DDI_NVM_GetInstopStatus() && (DDI_NVM_GetDebugStatus() == RELEASE_LEVEL))
	{
		DDI_OTP_WR_Enable(0);
		udelay(200000);
		setFullVerifyOTP();
		udelay(20000);
		DDI_OTP_WR_Enable(1);
		printf("!!!! full verify OTP is set!!!!\n");
	}
	else
	{
		printf("full verify OTP will be set, after instop and in release level\n");
	}

	printf("end set full verify OTP\n");
}

#endif // CC_A1_SECURE_BOOT

#endif // CC_SECURE_ROM_BOOT
