/*
* linux/sound/drivers/mtk/alsa_pcm.h
*
* MTK Sound Card Driver
*
* Copyright (c) 2010-2012 MediaTek Inc.
* $Author: dexi.tang $
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

#ifndef _ALSA_PCM_H_
#define _ALSA_PCM_H_

#include <linux/init.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/jiffies.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/wait.h>
#include <linux/module.h>
#include <sound/core.h>
#include <sound/control.h>
#include <sound/tlv.h>
#include <sound/pcm.h>
#include <sound/rawmidi.h>
#include <sound/initval.h>

/*copied from init.h in kernel-3.4*/
/* Used for HOTPLUG */
#define __devinit        __section(.devinit.text) __cold notrace
#define __devinitdata    __section(.devinit.data)
#define __devinitconst   __section(.devinit.rodata)
#define __devexit        __section(.devexit.text) __exitused __cold notrace
#define __devexitdata    __section(.devexit.data)
#define __devexitconst   __section(.devexit.rodata)

//#define  MTK_AUDIO_SUPPORT_MULTI_STREAMOUT
//#define  SUPPORT_COMPRESS_OFFLOAD 
//#define ALSACAPTURE
//#define ENABLE_ALSA_LOG //Take care !, it will cause system very busy !!!
//#define  ENABLE_DETAIL_LOG
//#define  NEW_TIMER_FOR_COMPRESS_OFFLOAD
//#define NEW_LOCK_FOR_COMPRESS_OFFLOAD
//#define SUPPORT_NEW_TUNAL_MODE  //for TUNAL PCM/Tunal AC3 RAW   ==> with AUDIO_OUTPUT_FLAG_HW_AV_SYNC
                                                       //for NON-TUNAL PCM/NON-TUNAL AC3 RAW  ==> without AUDIO_OUTPUT_FLAG_HW_AV_SYNC
//#define SUPPORT_NEW_TUNAL_MODE_TEST //let  PORT_TUNAL_AC3=3, PORT_NON_TUNAL_AC3=3 , and force for set  DSP Decoder= AC3                                                      

#ifdef SUPPORT_COMPRESS_OFFLOAD
#include <uapi/sound/compress_params.h>
#include <sound/compress_offload.h>
#include <sound/compress_driver.h>
#endif

/* Functions marked as __devexit may be discarded at kernel link time, depending
   on config options.  Newer versions of binutils detect references from
   retained sections to discarded sections and flag an error.  Pointers to
   __devexit functions must use __devexit_p(function_name), the wrapper will
   insert either the function_name or NULL, depending on the config options.
 */
#if defined(MODULE) || defined(CONFIG_HOTPLUG)
#define __devexit_p(x) x
#else
#define __devexit_p(x) NULL
#endif

struct snd_mt85xx {
    struct snd_card *card;
    struct snd_pcm *pcm;
#ifdef SUPPORT_COMPRESS_OFFLOAD
    struct snd_compr  *compr; 
#endif
};

#define TIMER_FIRST   0
#define TIMER_NORMAL  1
#define TIMER_LAST    2

struct snd_mt85xx_pcm {
    spinlock_t                  lock;
    struct timer_list           timer;
    unsigned int                timer_started;//2012/12/7 added by daniel
    unsigned int                instance;
    unsigned int                pcm_buffer_size;
    unsigned int                pcm_period_size;
    unsigned int                pcm_rptr; // read byte offset
    unsigned int                pcm_wptr; // write byte offset
    unsigned int                bytes_elapsed;   
    struct snd_pcm_substream   *substream;
#ifdef SUPPORT_COMPRESS_OFFLOAD
    unsigned int                sampling_rate;
    unsigned int                channel_num;
    struct snd_compr_stream  *compr; 
  #ifdef NEW_TIMER_FOR_COMPRESS_OFFLOAD	
    unsigned int                timer_for_compress_offload_started;  
    struct timer_list           timer_for_compress_offload;	
  #endif
  #ifdef NEW_LOCK_FOR_COMPRESS_OFFLOAD
    spinlock_t                  lock_for_compress_offload;
  #endif
#endif
};

extern int __devinit snd_card_mt85xx_pcm(struct snd_mt85xx *mt85xx, int device, int substreams);

#ifdef SUPPORT_COMPRESS_OFFLOAD
typedef enum
{
    AUD_FMT_UNKNOWN = 0,
    AUD_FMT_MPEG,
    AUD_FMT_AC3,
    AUD_FMT_PCM,
    AUD_FMT_MP3,
    AUD_FMT_AAC,
    AUD_FMT_DTS,
    AUD_FMT_WMA,
    AUD_FMT_RA,
    AUD_FMT_HDCD,
    AUD_FMT_MLP,     // 10
    AUD_FMT_MTS,
    AUD_FMT_EU_CANAL_PLUS,
    AUD_FMT_PAL,
    AUD_FMT_A2,
    AUD_FMT_FMFM,
    AUD_FMT_NICAM,
    AUD_FMT_TTXAAC,
    AUD_FMT_DETECTOR,
    AUD_FMT_MINER,
    AUD_FMT_LPCM,       // 20
    AUD_FMT_FMRDO,
    AUD_FMT_FMRDO_DET,
    AUD_FMT_SBCDEC,
    AUD_FMT_SBCENC,     // 24,
    AUD_FMT_MP3ENC,     // 25, MP3ENC_SUPPORT
    AUD_FMT_G711DEC,    // 26
    AUD_FMT_G711ENC,    // 27
    AUD_FMT_DRA,        // 28
    AUD_FMT_COOK,        // 29
    AUD_FMT_G729DEC,     // 30
    AUD_FMT_VORBISDEC,    //31, CC_VORBIS_SUPPORT
    AUD_FMT_WMAPRO,    //32  please sync number with middleware\res_mngr\drv\x_aud_dec.h
    AUD_FMT_HE_AAC,     //33 Terry Added for S1 UI notification
    AUD_FMT_HE_AAC_V2,  //34
    AUD_FMT_AMR,        //35 amr-nb run in DSP
    AUD_FMT_AWB,        //36 amr-wb run in DSP
    AUD_FMT_APE,        //37 //ian APE decoder
    AUD_FMT_FLAC = 39,  //39, paul_flac
    AUD_FMT_G726 = 40,  //40, G726 decoder

    AUD_FMT_TV_SYS = 63, //63, sync from x_aud_dec.h
    AUD_FMT_OMX_MSADPCM = 0x80,
    AUD_FMT_OMX_IMAADPCM = 0x81,
    AUD_FMT_OMX_ALAW = 0x82,
    AUD_FMT_OMX_ULAW = 0x83,
}   AUD_FMT_T;

typedef enum
{
    FS_16K = 0x00,
    FS_22K,
    FS_24K,
    FS_32K,
    FS_44K,
    FS_48K,
    FS_64K,
    FS_88K,
    FS_96K,
    FS_176K,
    FS_192K,
    FS_8K, // appended since 09/10/2007, don't change the order
    FS_11K, // appended since 09/10/2007, don't change the order
    FS_12K, // appended since 09/10/2007, don't change the order
    FS_52K, // appended since 24/02/2010, don't change the order
    FS_56K,
    FS_62K,  // appended since 24/02/2010, don't change the order
    FS_64K_SRC,
    FS_6K,
    FS_10K,
    FS_5K
}   SAMPLE_FREQ_T;

extern int __devinit snd_card_mt85xx_compress(struct snd_mt85xx *mt85xx, int device, int substreams);
#endif

#ifdef SUPPORT_NEW_TUNAL_MODE
//Must be the same with Audio.h

/* PCM sub formats */
typedef enum {
    /* All of these are in native byte order */
    AUDIO_FORMAT_PCM_SUB_16_BIT          = 0x1, /* DO NOT CHANGE - PCM signed 16 bits */
    AUDIO_FORMAT_PCM_SUB_8_BIT           = 0x2, /* DO NOT CHANGE - PCM unsigned 8 bits */
    AUDIO_FORMAT_PCM_SUB_32_BIT          = 0x3, /* PCM signed .31 fixed point */
    AUDIO_FORMAT_PCM_SUB_8_24_BIT        = 0x4, /* PCM signed 7.24 fixed point */
    AUDIO_FORMAT_PCM_SUB_FLOAT           = 0x5, /* PCM single-precision floating point */
    AUDIO_FORMAT_PCM_SUB_24_BIT_PACKED   = 0x6, /* PCM signed .23 fixed point packed in 3 bytes */
} audio_format_pcm_sub_fmt_t;


/* AAC sub format field definition: specify profile or bitrate for recording... */
typedef enum {
    AUDIO_FORMAT_AAC_SUB_MAIN            = 0x1,
    AUDIO_FORMAT_AAC_SUB_LC              = 0x2,
    AUDIO_FORMAT_AAC_SUB_SSR             = 0x4,
    AUDIO_FORMAT_AAC_SUB_LTP             = 0x8,
    AUDIO_FORMAT_AAC_SUB_HE_V1           = 0x10,
    AUDIO_FORMAT_AAC_SUB_SCALABLE        = 0x20,
    AUDIO_FORMAT_AAC_SUB_ERLC            = 0x40,
    AUDIO_FORMAT_AAC_SUB_LD              = 0x80,
    AUDIO_FORMAT_AAC_SUB_HE_V2           = 0x100,
    AUDIO_FORMAT_AAC_SUB_ELD             = 0x200,
} audio_format_aac_sub_fmt_t;



typedef enum {
    AUDIO_FORMAT_INVALID             = 0xFFFFFFFFUL,
    AUDIO_FORMAT_DEFAULT             = 0,
    AUDIO_FORMAT_PCM                 = 0x00000000UL, /* DO NOT CHANGE */
    AUDIO_FORMAT_MP3                 = 0x01000000UL,
    AUDIO_FORMAT_AMR_NB              = 0x02000000UL,
    AUDIO_FORMAT_AMR_WB              = 0x03000000UL,
    AUDIO_FORMAT_AAC                 = 0x04000000UL,
    AUDIO_FORMAT_HE_AAC_V1           = 0x05000000UL, /* Deprecated, Use AUDIO_FORMAT_AAC_HE_V1*/
    AUDIO_FORMAT_HE_AAC_V2           = 0x06000000UL, /* Deprecated, Use AUDIO_FORMAT_AAC_HE_V2*/
    AUDIO_FORMAT_VORBIS              = 0x07000000UL,
    AUDIO_FORMAT_OPUS                = 0x08000000UL,
    AUDIO_FORMAT_AC3                 = 0x09000000UL,
    AUDIO_FORMAT_E_AC3               = 0x0A000000UL,
    AUDIO_FORMAT_MAIN_MASK           = 0xFF000000UL,
    AUDIO_FORMAT_SUB_MASK            = 0x00FFFFFFUL,

    /* Aliases */
    /* note != AudioFormat.ENCODING_PCM_16BIT */
    AUDIO_FORMAT_PCM_16_BIT          = (AUDIO_FORMAT_PCM |
                                        AUDIO_FORMAT_PCM_SUB_16_BIT),
    /* note != AudioFormat.ENCODING_PCM_8BIT */
    AUDIO_FORMAT_PCM_8_BIT           = (AUDIO_FORMAT_PCM |
                                        AUDIO_FORMAT_PCM_SUB_8_BIT),
    AUDIO_FORMAT_PCM_32_BIT          = (AUDIO_FORMAT_PCM |
                                        AUDIO_FORMAT_PCM_SUB_32_BIT),
    AUDIO_FORMAT_PCM_8_24_BIT        = (AUDIO_FORMAT_PCM |
                                        AUDIO_FORMAT_PCM_SUB_8_24_BIT),
    AUDIO_FORMAT_PCM_FLOAT           = (AUDIO_FORMAT_PCM |
                                        AUDIO_FORMAT_PCM_SUB_FLOAT),
    AUDIO_FORMAT_PCM_24_BIT_PACKED   = (AUDIO_FORMAT_PCM |
                                        AUDIO_FORMAT_PCM_SUB_24_BIT_PACKED),
    AUDIO_FORMAT_AAC_MAIN            = (AUDIO_FORMAT_AAC |
                                        AUDIO_FORMAT_AAC_SUB_MAIN),
    AUDIO_FORMAT_AAC_LC              = (AUDIO_FORMAT_AAC |
                                        AUDIO_FORMAT_AAC_SUB_LC),
    AUDIO_FORMAT_AAC_SSR             = (AUDIO_FORMAT_AAC |
                                        AUDIO_FORMAT_AAC_SUB_SSR),
    AUDIO_FORMAT_AAC_LTP             = (AUDIO_FORMAT_AAC |
                                        AUDIO_FORMAT_AAC_SUB_LTP),
    AUDIO_FORMAT_AAC_HE_V1           = (AUDIO_FORMAT_AAC |
                                        AUDIO_FORMAT_AAC_SUB_HE_V1),
    AUDIO_FORMAT_AAC_SCALABLE        = (AUDIO_FORMAT_AAC |
                                        AUDIO_FORMAT_AAC_SUB_SCALABLE),
    AUDIO_FORMAT_AAC_ERLC            = (AUDIO_FORMAT_AAC |
                                        AUDIO_FORMAT_AAC_SUB_ERLC),
    AUDIO_FORMAT_AAC_LD              = (AUDIO_FORMAT_AAC |
                                        AUDIO_FORMAT_AAC_SUB_LD),
    AUDIO_FORMAT_AAC_HE_V2           = (AUDIO_FORMAT_AAC |
                                        AUDIO_FORMAT_AAC_SUB_HE_V2),
    AUDIO_FORMAT_AAC_ELD             = (AUDIO_FORMAT_AAC |
                                        AUDIO_FORMAT_AAC_SUB_ELD),
} audio_format_t;


#endif

#endif  // _ALSA_PCM_H_
