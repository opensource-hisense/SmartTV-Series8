/*
* linux/sound/drivers/mtk/mtk.c
*
* MTK Sound Card Driver
*
* Copyright (c) 2010-2012 MediaTek Inc.
* $Author: dtvbm11 $
* 
* Porting from linux/sound/drivers/dummy.c
* Original author: Jaroslav Kysela <perex@perex.cz>
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

#include <linux/init.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/jiffies.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/wait.h>
#include <linux/hrtimer.h>
#include <linux/math64.h>
#include <linux/module.h>
#include <sound/core.h>
#include <sound/control.h>
#include <sound/tlv.h>
#include <sound/pcm.h>
#include <sound/rawmidi.h>
#include <sound/info.h>
#include <sound/initval.h>

//TODO: 2012/2/6 1. Capture Path Review. 2. Mixer Path Shrink

MODULE_AUTHOR("MTK");
MODULE_DESCRIPTION("MTK soundcard (/dev/dsp)");
MODULE_LICENSE("GPL");
MODULE_SUPPORTED_DEVICE("{{ALSA,MTK soundcard}}");

#define ENABLE_PCM_LIB_SRC      1

#define MAX_PCM_DEVICES		1
#define MAX_PCM_SUBSTREAMS	2 //MTK

////////////////////////////////////////
//MTK ALSA Audio Interface
////////////////////////////////////////

#define ALSA_DBG_MSG 0
#define ALSA_DBG_MSG_MIXER 0
#define MTK_ALSA_VERSION_CODE "201202101600"

typedef unsigned int UINT32;
typedef unsigned short UINT16;
typedef unsigned char UINT8;
typedef signed int INT32;
typedef bool BOOL;
typedef unsigned long UPTR;


static UINT32 u4PlayBufSA[MAX_PCM_SUBSTREAMS];
static UINT32 u4PlayBufEA[MAX_PCM_SUBSTREAMS];
static UINT32 u4PlayBufWP[MAX_PCM_SUBSTREAMS];

//PLAYBACK
extern void AUD_InitALSAPlayback_MixSnd(UINT8 u1StreamId);
extern void AUD_DeInitALSAPlayback_MixSnd(UINT8 u1StreamId);
extern UPTR AUD_GetMixSndFIFOStart(UINT8 u1StreamId);
extern UPTR AUD_GetMixSndFIFOEnd(UINT8 u1StreamId);
extern UPTR AUD_GetMixSndReadPtr(UINT8 u1StreamId);
extern void AUD_SetMixSndWritePtr(UINT8 u1StreamId, UINT32 u4WritePtr);
extern void AUD_PlayMixSndRingFifo(UINT8 u1StreamId, UINT32 u4SampleRate, UINT8 u1StereoOnOff, UINT8 u1BitDepth, UINT32 u4BufferSize);

//RECORD
extern UINT32 AUD_GetUploadFIFOStart(void);
extern UINT32 AUD_GetUploadFIFOEnd(void);
extern UINT32 AUD_GetUploadWritePnt(void);
extern void AUD_InitALSARecordSpeaker(void);
extern void AUD_DeInitALSARecordSpeaker(void);
extern void AUD_FlushDram(UINT32 u4Addr, UINT32 u4Len);

#define vInitAlsaPlayback(id) AUD_InitALSAPlayback_MixSnd(id)
#define vDeInitAlsaPlayback(id) AUD_DeInitALSAPlayback_MixSnd(id)
#define u4GetPlayBufSA(id) AUD_GetMixSndFIFOStart(id)
#define u4GetPlayBufEA(id) AUD_GetMixSndFIFOEnd(id)
#define u4GetPlayBufCA(id) AUD_GetMixSndReadPtr(id)
#define vSetPlayBufWA(id, wp)   AUD_SetMixSndWritePtr(id, wp) 
#define vSetPlayParm(u1StreamId, u4SampleRate,u1StereoOnOff,u1BitDepth,u4BufferSize) AUD_PlayMixSndRingFifo(u1StreamId, u4SampleRate,u1StereoOnOff,u1BitDepth,u4BufferSize)
#define vFlushDram(addr,size) AUD_FlushDram(addr,size)

#define u4GetRecBufSA() AUD_GetUploadFIFOStart()
#define u4GetRecBufEA() AUD_GetUploadFIFOEnd()
#define u4GetRecBufCA() AUD_GetUploadWritePnt()
#define AUD_InitALSARecord() AUD_InitALSARecordSpeaker()
#define AUD_DeInitALSARecord() AUD_DeInitALSARecordSpeaker()

////////////////////////////////////////
//End
////////////////////////////////////////

/* defaults */
#if ENABLE_PCM_LIB_SRC
#define MAX_BUFFER_SIZE		(16*1024) // (64*1024)
#define MIN_PERIOD_SIZE		(MAX_BUFFER_SIZE/4) //(MAX_BUFFER_SIZE/8)
#define MAX_PERIOD_SIZE		(MAX_BUFFER_SIZE/4) //(MAX_BUFFER_SIZE/8)
#define USE_FORMATS 		SNDRV_PCM_FMTBIT_S16_LE
#define USE_RATE                (SNDRV_PCM_RATE_CONTINUOUS | SNDRV_PCM_RATE_8000_48000) //SNDRV_PCM_RATE_48000
#define USE_RATE_MIN		8000
#define USE_RATE_MAX		48000
#define USE_CHANNELS_MIN 	2
#define USE_CHANNELS_MAX 	2
#define USE_PERIODS_MIN 	4 // 8 // 4
#define USE_PERIODS_MAX 	4 // 8 // 1024
#else
#define MAX_BUFFER_SIZE     (64*1024)
#define MIN_PERIOD_SIZE     64
#define MAX_PERIOD_SIZE     MAX_BUFFER_SIZE
#define USE_FORMATS         SNDRV_PCM_FMTBIT_S16_LE
#define USE_RATE            (SNDRV_PCM_RATE_CONTINUOUS | SNDRV_PCM_RATE_8000_48000) //SNDRV_PCM_RATE_48000
#define USE_RATE_MIN        8000
#define USE_RATE_MAX        48000
#define USE_CHANNELS_MIN    2
#define USE_CHANNELS_MAX    2
#define USE_PERIODS_MIN     1
#define USE_PERIODS_MAX     1024
#endif

static int index[SNDRV_CARDS] = SNDRV_DEFAULT_IDX;	/* Index 0-MAX */
static char *id[SNDRV_CARDS] = SNDRV_DEFAULT_STR;	/* ID for this card */
static int enable[SNDRV_CARDS] = {1, [1 ... (SNDRV_CARDS - 1)] = 0};
static char *model[SNDRV_CARDS] = {[0 ... (SNDRV_CARDS - 1)] = NULL};
static int pcm_devs[SNDRV_CARDS] = {[0 ... (SNDRV_CARDS - 1)] = 1};
static int pcm_substreams[SNDRV_CARDS] = {[0 ... (SNDRV_CARDS - 1)] = 8};

module_param_array(index, int, NULL, 0444);
MODULE_PARM_DESC(index, "Index value for mtk soundcard.");
module_param_array(id, charp, NULL, 0444);
MODULE_PARM_DESC(id, "ID string for mtk soundcard.");
module_param_array(enable, int, NULL, 0444);
MODULE_PARM_DESC(enable, "Enable this mtk soundcard.");
module_param_array(model, charp, NULL, 0444);
MODULE_PARM_DESC(model, "Soundcard model.");
module_param_array(pcm_devs, int, NULL, 0444);
MODULE_PARM_DESC(pcm_devs, "PCM devices for mtk driver.");
module_param_array(pcm_substreams, int, NULL, 0444);
MODULE_PARM_DESC(pcm_substreams, "PCM substreams for mtk driver."); //MTK

static struct platform_device *devices[SNDRV_CARDS];

#define MIXER_ADDR_MASTER	0
#define MIXER_ADDR_LINE		1
#define MIXER_ADDR_MIC		2
#define MIXER_ADDR_SYNTH	3
#define MIXER_ADDR_CD		4
#define MIXER_ADDR_LAST		4

struct mtk_timer_ops 
{
	int (*create)(struct snd_pcm_substream *);
	void (*free)(struct snd_pcm_substream *);
	int (*prepare)(struct snd_pcm_substream *);
	int (*start)(struct snd_pcm_substream *);
	int (*stop)(struct snd_pcm_substream *);
	snd_pcm_uframes_t (*pointer)(struct snd_pcm_substream *);
};

struct snd_mtk 
{
	struct snd_card *card;
	struct snd_pcm *pcm;
	spinlock_t mixer_lock;
	int mixer_volume[MIXER_ADDR_LAST+1][2];
	int capture_source[MIXER_ADDR_LAST+1][2];
	const struct mtk_timer_ops *timer_ops;
};

/*
 * system timer interface
 */
struct mtk_systimer_pcm 
{
	spinlock_t lock;
	struct timer_list timer;
	unsigned long base_time;
	unsigned int frac_pos;	/* fractional sample position (based HZ) */
	unsigned int frac_period_rest;
	unsigned int frac_buffer_size;	/* buffer_size * HZ */
	unsigned int frac_period_size;	/* period_size * HZ */
	unsigned int rate;
	int elapsed;
	struct snd_pcm_substream *substream;
};

#if ENABLE_PCM_LIB_SRC
static snd_pcm_uframes_t
mtk_systimer_pointer(struct snd_pcm_substream *substream)
{
#if 0
	struct mtk_systimer_pcm *dpcm = substream->runtime->private_data;
	snd_pcm_uframes_t pos;

	spin_lock(&dpcm->lock);
	mtk_systimer_update(dpcm);
	pos = dpcm->frac_pos / HZ;
	spin_unlock(&dpcm->lock);
	return pos;	
#else	
	snd_pcm_uframes_t pos;

    UINT32 u4Start = u4PlayBufSA[substream->number];
    UINT32 u4ABufPnt = u4GetPlayBufCA(substream->number);

	return pos = HZ * ((u4ABufPnt - u4Start)>>2);   
#endif
}
#endif

static void mtk_systimer_rearm(struct mtk_systimer_pcm *dpcm)
{
	dpcm->timer.expires = jiffies +
		(dpcm->frac_period_rest + dpcm->rate - 1) / dpcm->rate;
	add_timer(&dpcm->timer);
}

static void mtk_systimer_update(struct mtk_systimer_pcm *dpcm)
{
#if ENABLE_PCM_LIB_SRC
	unsigned long delta;
	snd_pcm_uframes_t pos;
	
	struct snd_pcm_runtime *runtime = dpcm->substream->runtime;
	snd_pcm_sframes_t avail = runtime->status->hw_ptr + runtime->buffer_size - runtime->control->appl_ptr;
	snd_pcm_sframes_t hw_ptr = (runtime->status->hw_ptr)%(runtime->buffer_size);
	
	delta = jiffies - dpcm->base_time;
	if (!delta)
		return;

	pos = mtk_systimer_pointer(dpcm->substream);

	if(((!avail) && ((pos / dpcm->frac_period_size) != (hw_ptr / runtime->period_size))) 
		|| (runtime->status->state == SNDRV_PCM_STATE_DRAINING))
	{
		dpcm->elapsed++;
	}
#else
	unsigned long delta;

	delta = jiffies - dpcm->base_time;
	if (!delta)
		return;
	dpcm->base_time += delta;
	delta *= dpcm->rate;
	dpcm->frac_pos += delta;
	while (dpcm->frac_pos >= dpcm->frac_buffer_size)
		dpcm->frac_pos -= dpcm->frac_buffer_size;
	while (dpcm->frac_period_rest <= delta) {
		dpcm->elapsed++;
		dpcm->frac_period_rest += dpcm->frac_period_size;
	}
	dpcm->frac_period_rest -= delta;
#endif
}

static int mtk_systimer_start(struct snd_pcm_substream *substream)
{
	struct mtk_systimer_pcm *dpcm = substream->runtime->private_data;
	spin_lock(&dpcm->lock);
	dpcm->base_time = jiffies;
	mtk_systimer_rearm(dpcm);
	spin_unlock(&dpcm->lock);
	return 0;
}

static int mtk_systimer_stop(struct snd_pcm_substream *substream)
{
	struct mtk_systimer_pcm *dpcm = substream->runtime->private_data;
	spin_lock(&dpcm->lock);
	del_timer_sync(&dpcm->timer);
	spin_unlock(&dpcm->lock);
	return 0;
}

static int mtk_systimer_prepare(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct mtk_systimer_pcm *dpcm = runtime->private_data;

	dpcm->frac_pos = 0;
	dpcm->rate = runtime->rate;
	dpcm->frac_buffer_size = runtime->buffer_size * HZ;
	dpcm->frac_period_size = runtime->period_size * HZ;
	dpcm->frac_period_rest = dpcm->frac_period_size;
	dpcm->elapsed = 0;

	return 0;
}

static void mtk_systimer_callback(unsigned long data)
{
	struct mtk_systimer_pcm *dpcm = (struct mtk_systimer_pcm *)data;
	unsigned long flags;
	int elapsed = 0;
	
	spin_lock_irqsave(&dpcm->lock, flags);
	mtk_systimer_update(dpcm);
	mtk_systimer_rearm(dpcm);
	elapsed = dpcm->elapsed;
	dpcm->elapsed = 0;
	spin_unlock_irqrestore(&dpcm->lock, flags);
	if (elapsed)
		snd_pcm_period_elapsed(dpcm->substream);
}

#if !ENABLE_PCM_LIB_SRC
static snd_pcm_uframes_t
mtk_systimer_pointer(struct snd_pcm_substream *substream)
{
	struct mtk_systimer_pcm *dpcm = substream->runtime->private_data;
	snd_pcm_uframes_t pos;

	spin_lock(&dpcm->lock);
	mtk_systimer_update(dpcm);
	pos = dpcm->frac_pos / HZ;
	spin_unlock(&dpcm->lock);
	return pos;
}
#endif

static int mtk_systimer_create(struct snd_pcm_substream *substream)
{
	struct mtk_systimer_pcm *dpcm;

	dpcm = kzalloc(sizeof(*dpcm), GFP_KERNEL);
	if (!dpcm)
		return -ENOMEM;
	substream->runtime->private_data = dpcm;
	init_timer(&dpcm->timer);
	dpcm->timer.data = (unsigned long) dpcm;
	dpcm->timer.function = mtk_systimer_callback;
	spin_lock_init(&dpcm->lock);
	dpcm->substream = substream;
	return 0;
}

static void mtk_systimer_free(struct snd_pcm_substream *substream)
{
	kfree(substream->runtime->private_data);
}

static struct mtk_timer_ops mtk_systimer_ops = {
	.create =	mtk_systimer_create,
	.free =		mtk_systimer_free,
	.prepare =	mtk_systimer_prepare,
	.start =	mtk_systimer_start,
	.stop =		mtk_systimer_stop,
	.pointer =	mtk_systimer_pointer,
};

/*
 * PCM interface
 */
static int mtk_pcm_trigger(struct snd_pcm_substream *substream, int cmd)
{
	struct snd_mtk *mtk = snd_pcm_substream_chip(substream);

    switch (cmd) 
    {
    case SNDRV_PCM_TRIGGER_START:
        #if ALSA_DBG_MSG
        printk(KERN_ERR "!@#!@# @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ snd_card_mtk_pcm_trigger - start\n"); //0513
        #endif
        return mtk->timer_ops->start(substream);
    case SNDRV_PCM_TRIGGER_RESUME:
        #if ALSA_DBG_MSG
        printk(KERN_ERR "!@#!@# @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ snd_card_mtk_pcm_trigger - resume\n"); //0513
        #endif
        return mtk->timer_ops->start(substream);
    case SNDRV_PCM_TRIGGER_STOP:
        #if ALSA_DBG_MSG
        printk(KERN_ERR "!@#!@# @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ snd_card_mtk_pcm_trigger - stop\n"); //0513
        #endif
        return mtk->timer_ops->stop(substream);
    case SNDRV_PCM_TRIGGER_SUSPEND:
        #if ALSA_DBG_MSG
        printk(KERN_ERR "!@#!@# @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ snd_card_mtk_pcm_trigger - suspend\n"); //0513                       
        #endif
        return mtk->timer_ops->stop(substream);
    }
	return -EINVAL;    
}

static int mtk_pcm_playback_prepare(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct snd_mtk *mtk = snd_pcm_substream_chip(substream);

    #if ALSA_DBG_MSG
    printk(KERN_ERR "!@#!@# @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ snd_card_mtk_playback_pcm_prepare\n"); //0513
    #endif

    vSetPlayParm(substream->number, runtime->rate, 1, 16, snd_pcm_lib_buffer_bytes(substream));

    if (runtime->dma_area == 0)
    {
        runtime->dma_area = (unsigned char*)u4PlayBufSA[substream->number];
        runtime->dma_addr = 0;
        runtime->dma_bytes = snd_pcm_lib_buffer_bytes(substream);
    }

    printk(KERN_ERR "format: %d rate: %d channels: %d\n", runtime->format, runtime->rate, runtime->channels);
    printk(KERN_ERR "runtime->buffer_size: %x\n", (unsigned int)(runtime->buffer_size));
    printk(KERN_ERR "runtime->period_size: %x\n", (unsigned int)(runtime->period_size));
    #if ALSA_DBG_MSG    
    //printk(KERN_ERR "dpcm->pcm_bps: %x\n", dpcm->pcm_bps);
    //printk(KERN_ERR "dpcm->pcm_hz: %x\n", dpcm->pcm_hz);
    printk(KERN_ERR "snd_pcm_lib_buffer_bytes(substream): %x\n", snd_pcm_lib_buffer_bytes(substream));
    printk(KERN_ERR "snd_pcm_lib_period_bytes(substream): %x\n", snd_pcm_lib_period_bytes(substream));
    printk(KERN_ERR "runtime->dma_area: 0x%08x\n", runtime->dma_area);
    printk(KERN_ERR "runtime->dma_addr: 0x%08x\n", runtime->dma_addr);
    printk(KERN_ERR "runtime->dma_bytes: 0x%08x\n", runtime->dma_bytes);
    #endif

	return mtk->timer_ops->prepare(substream);
}

static int mtk_pcm_capture_prepare(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct snd_mtk *mtk = snd_pcm_substream_chip(substream);

    #if ALSA_DBG_MSG
    printk(KERN_ERR "!@#!@# @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ snd_card_mtk_record_pcm_prepare\n"); //0513
    #if 0
    if (dpcm->mtk)
    {
        printk(KERN_ERR "  RECSRC MASTER: %d\n", dpcm->mtk->capture_source[MIXER_ADDR_MASTER][0]);
        printk(KERN_ERR "  RECSRC LINE: %d\n", dpcm->mtk->capture_source[MIXER_ADDR_LINE][0]);
        printk(KERN_ERR "  RECSRC MIC: %d\n", dpcm->mtk->capture_source[MIXER_ADDR_MIC][0]);
        printk(KERN_ERR "  RECSRC SYNTH: %d\n", dpcm->mtk->capture_source[MIXER_ADDR_SYNTH][0]);
        printk(KERN_ERR "  RECSRC CD: %d\n", dpcm->mtk->capture_source[MIXER_ADDR_LAST][0]);
    }
    else
    {
        printk(KERN_ERR "  dpcm->mtk is null pointer !!!\n");
    }
    #endif
    #endif

    //snd_pcm_format_set_silence(runtime->format, runtime->dma_area, bytes_to_samples(runtime, runtime->dma_bytes));
    if (runtime->dma_area == 0)
    {
        runtime->dma_area = (unsigned char*)u4GetRecBufSA();
        runtime->dma_addr = 0;
        runtime->dma_bytes = u4GetRecBufEA() - u4GetRecBufSA();

        AUD_InitALSARecord();
    }

	return mtk->timer_ops->prepare(substream);
}

//MTK
#define FIRST_RECORD_DBG 0

#if FIRST_RECORD_DBG
int first_record = 0;
#endif
//MTK

static snd_pcm_uframes_t mtk_pcm_playback_pointer(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;

#if 1
    UINT32 u4Start = u4PlayBufSA[substream->number];
    UINT32 u4ABufPnt = u4GetPlayBufCA(substream->number);
    UINT32 u4WritePnt;

    u4WritePnt = u4Start + (((runtime->control->appl_ptr * 4) % snd_pcm_lib_buffer_bytes(substream))&0xffffff00);
    
    if (u4ABufPnt == u4WritePnt)
    {
        //printk(KERN_ERR "u4ABufPnt: 0x%08x", u4WritePnt);
    }
    else
    {
    #if 0 //Test for DSP Path
        if (u4WritePnt >= u4PlayBufWP[substream->number])
        {
            vFlushDram(u4PlayBufWP[substream->number], u4WritePnt - u4PlayBufWP[substream->number]);
        }
        else
        {
            vFlushDram(u4PlayBufWP[substream->number], u4PlayBufEA[substream->number] - u4PlayBufWP[substream->number]);
            vFlushDram(u4PlayBufSA[substream->number], u4WritePnt - u4PlayBufSA[substream->number]);
        }
        u4PlayBufWP[substream->number] = u4WritePnt;
    #endif
        vSetPlayBufWA(substream->number,u4WritePnt);
    }

    {
      #if ENABLE_PCM_LIB_SRC
        return ((u4ABufPnt - u4Start)>>2);
      #else
        struct snd_mtk *mtk = snd_pcm_substream_chip(substream);
        return mtk->timer_ops->pointer(substream);
      #endif        
    }    
#else
	struct snd_mtk *mtk = snd_pcm_substream_chip(substream);

	return mtk->timer_ops->pointer(substream);
#endif
}

static snd_pcm_uframes_t mtk_pcm_capture_pointer(struct snd_pcm_substream *substream)
{
#if 1
	//struct snd_pcm_runtime *runtime = substream->runtime;

    UINT32 u4Start = u4GetRecBufSA();
    UINT32 u4ABufPnt = u4GetRecBufCA();
    //printk(KERN_ERR "SA: 0x%08x WP: 0x%08x\n", u4Start, u4ABufPnt);
    #if FIRST_RECORD_DBG
    if (!first_record)
    {
        UINT32 ptr;
        
        printk(KERN_ERR "u4Start: 0x%08x u4ABufPnt: 0x%08x\n", u4Start, u4ABufPnt);  
        ptr = u4Start;
        printk(KERN_ERR "%08x %08x %08x %08x\n", *((UINT32*)ptr), *((UINT32*)(ptr+4)), *((UINT32*)(ptr+8)), *((UINT32*)(ptr+12)));
        ptr = u4ABufPnt;
        printk(KERN_ERR "%08x %08x %08x %08x\n", *((UINT32*)ptr), *((UINT32*)(ptr+4)), *((UINT32*)(ptr+8)), *((UINT32*)(ptr+12)));

        first_record = 1;
    }
    #endif
    
    {
      #if 1
        return ((u4ABufPnt - u4Start)>>2);
      #else
    	struct snd_mtk *mtk = snd_pcm_substream_chip(substream);    
    	return mtk->timer_ops->pointer(substream);
      #endif
    }
#else
	struct snd_mtk *mtk = snd_pcm_substream_chip(substream);

	return mtk->timer_ops->pointer(substream);
#endif
}

static struct snd_pcm_hardware mtk_pcm_playback_hardware = {
	.info =			(SNDRV_PCM_INFO_MMAP |
				 SNDRV_PCM_INFO_INTERLEAVED |
				 SNDRV_PCM_INFO_RESUME |
				 SNDRV_PCM_INFO_MMAP_VALID),
	.formats =		USE_FORMATS,
	.rates =		USE_RATE,
	.rate_min =		USE_RATE_MIN,
	.rate_max =		USE_RATE_MAX,
	.channels_min =		USE_CHANNELS_MIN,
	.channels_max =		USE_CHANNELS_MAX,
	.buffer_bytes_max =	MAX_BUFFER_SIZE,
	.period_bytes_min =	MIN_PERIOD_SIZE,
	.period_bytes_max =	MAX_PERIOD_SIZE,
	.periods_min =		USE_PERIODS_MIN,
	.periods_max =		USE_PERIODS_MAX,
	.fifo_size =		0,
};

static struct snd_pcm_hardware mtk_pcm_capture_hardware = {
	.info =			(SNDRV_PCM_INFO_MMAP |
				 SNDRV_PCM_INFO_INTERLEAVED |
				 SNDRV_PCM_INFO_RESUME |
				 SNDRV_PCM_INFO_MMAP_VALID),
	.formats =		USE_FORMATS,
	.rates =		USE_RATE,
	.rate_min =		USE_RATE_MIN,
	.rate_max =		USE_RATE_MAX,
	.channels_min =		USE_CHANNELS_MIN,
	.channels_max =		USE_CHANNELS_MAX,
	.buffer_bytes_max =	MAX_BUFFER_SIZE,
	.period_bytes_min =	MIN_PERIOD_SIZE,
	.period_bytes_max =	MAX_PERIOD_SIZE,
	.periods_min =		USE_PERIODS_MIN,
	.periods_max =		USE_PERIODS_MAX,
	.fifo_size =		0,
};

static int mtk_pcm_hw_params(struct snd_pcm_substream *substream,
			       struct snd_pcm_hw_params *hw_params)
{
    int ret;
    //struct snd_pcm_runtime *runtime = substream->runtime;    

    #if ALSA_DBG_MSG
    printk(KERN_ERR "!@#!@# @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ snd_card_mtk_hw_params\n"); //0513
    printk(KERN_ERR "params_buffer_bytes(hw_params): 0x%08x\n", params_buffer_bytes(hw_params));
    //printk(KERN_ERR "[Before]\n");
    //printk(KERN_ERR "runtime->dma_area: 0x%08x\n", (UINT32)(runtime->dma_area));
    //printk(KERN_ERR "runtime->dma_addr: 0x%08x\n", (UINT32)(runtime->dma_addr));
    //printk(KERN_ERR "runtime->dma_bytes: 0x%08x\n", (UINT32)(runtime->dma_bytes));
    #endif

    ret = 0;

    #if ALSA_DBG_MSG
    //printk(KERN_ERR "[After] ret: %d\n", ret);
    //printk(KERN_ERR "runtime->dma_area: 0x%08x\n", (UINT32)(runtime->dma_area));
    //printk(KERN_ERR "runtime->dma_addr: 0x%08x\n", (UINT32)(runtime->dma_addr));
    //printk(KERN_ERR "runtime->dma_bytes: 0x%08x\n", (UINT32)(runtime->dma_bytes));   
    {
        int i;
        printk(KERN_ERR "flags: %08x\n", hw_params->flags);
        for (i=SNDRV_PCM_HW_PARAM_FIRST_MASK;i<=SNDRV_PCM_HW_PARAM_LAST_MASK;i++)
            printk(KERN_ERR "masks[%d]: %08x\n", i, hw_params->masks[i].bits[0]);
        for (i=SNDRV_PCM_HW_PARAM_FIRST_INTERVAL;i<=SNDRV_PCM_HW_PARAM_LAST_INTERVAL;i++)
            printk(KERN_ERR "internals[%d]: (%d,%d) omin=%d omax=%d int=%d empty=%d\n",
                            i,
                            hw_params->intervals[i-SNDRV_PCM_HW_PARAM_FIRST_INTERVAL].min,
                            hw_params->intervals[i-SNDRV_PCM_HW_PARAM_FIRST_INTERVAL].max,
                            hw_params->intervals[i-SNDRV_PCM_HW_PARAM_FIRST_INTERVAL].openmin,
                            hw_params->intervals[i-SNDRV_PCM_HW_PARAM_FIRST_INTERVAL].openmax,
                            hw_params->intervals[i-SNDRV_PCM_HW_PARAM_FIRST_INTERVAL].integer,
                            hw_params->intervals[i-SNDRV_PCM_HW_PARAM_FIRST_INTERVAL].empty);
        printk(KERN_ERR "rmask: %08x\n", hw_params->rmask);
        printk(KERN_ERR "cmask: %08x\n", hw_params->cmask);
        printk(KERN_ERR "info: %08x\n", hw_params->info);
        printk(KERN_ERR "msbits: %d\n", hw_params->msbits);
        printk(KERN_ERR "rate: %d/%d\n", hw_params->rate_num,hw_params->rate_den);
        printk(KERN_ERR "fifo: %d\n", hw_params->fifo_size);
    }
    #endif

    return ret;
}

static int mtk_pcm_hw_free(struct snd_pcm_substream *substream)
{
    int ret;
    struct snd_pcm_runtime *runtime = substream->runtime;

    #if ALSA_DBG_MSG
    printk(KERN_ERR "!@#!@# @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ snd_card_mtk_hw_free\n"); //0513
    printk(KERN_ERR "[Before]\n");
    printk(KERN_ERR "runtime->dma_area: 0x%08x\n", (UINT32)(runtime->dma_area));
    printk(KERN_ERR "runtime->dma_addr: 0x%08x\n", (UINT32)(runtime->dma_addr));
    printk(KERN_ERR "runtime->dma_bytes: 0x%08x\n", (UINT32)(runtime->dma_bytes));
    #endif

    ret = 0;
    if (runtime->dma_area)
    {
        runtime->dma_area = 0;
        runtime->dma_addr = 0;
        runtime->dma_bytes = 0;
    }

    #if ALSA_DBG_MSG
    printk(KERN_ERR "[After] ret: %d\n", ret);
    printk(KERN_ERR "runtime->dma_area: 0x%08x\n", (UINT32)(runtime->dma_area));
    printk(KERN_ERR "runtime->dma_addr: 0x%08x\n", (UINT32)(runtime->dma_addr));
    printk(KERN_ERR "runtime->dma_bytes: 0x%08x\n", (UINT32)(runtime->dma_bytes));
    #endif
    
    return ret;
}

static int mtk_pcm_playback_open(struct snd_pcm_substream *substream)
{
	struct snd_mtk *mtk = snd_pcm_substream_chip(substream);
	struct snd_pcm_runtime *runtime = substream->runtime;    
	int err;

	mtk->timer_ops = &mtk_systimer_ops;

    printk(KERN_ERR "snd_card_mtk_playback_open(%d,%d)\n", substream->number, substream->stream); //0513

    vInitAlsaPlayback(substream->number);

    u4PlayBufSA[substream->number] = u4GetPlayBufSA(substream->number);
    u4PlayBufEA[substream->number] = u4GetPlayBufEA(substream->number);
    u4PlayBufWP[substream->number] = u4PlayBufSA[substream->number];

    #if ALSA_DBG_MSG
    printk(KERN_ERR "AFIFO start addr: 0x%08x\n", u4PlayBufSA[substream->number]);
    printk(KERN_ERR "AFIFO end addr: 0x%08x\n", u4PlayBufEA[substream->number]);
    #endif

	err = mtk->timer_ops->create(substream);
	if (err < 0)
		return err;

    runtime->hw = mtk_pcm_playback_hardware; //MTK

	if (substream->pcm->device & 1) {
		runtime->hw.info &= ~SNDRV_PCM_INFO_INTERLEAVED;
		runtime->hw.info |= SNDRV_PCM_INFO_NONINTERLEAVED;
	}
	if (substream->pcm->device & 2)
		runtime->hw.info &= ~(SNDRV_PCM_INFO_MMAP | SNDRV_PCM_INFO_MMAP_VALID);

	return 0;
}

static int mtk_pcm_capture_open(struct snd_pcm_substream *substream)
{
	struct snd_mtk *mtk = snd_pcm_substream_chip(substream);
	struct snd_pcm_runtime *runtime = substream->runtime;
	int err;

    #if ALSA_DBG_MSG
    printk(KERN_ERR "!@#!@# @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ snd_card_mtk_capture_open\n"); //0513
    printk(KERN_ERR "  substream->number: %d\n", substream->number);
    printk(KERN_ERR "  substream->stream: %d\n", substream->stream);    
    printk(KERN_ERR "AFIFO start addr: 0x%08x\n", u4GetRecBufSA());
    printk(KERN_ERR "AFIFO end addr: 0x%08x\n", u4GetRecBufEA());
    #if 0
    if (dpcm->mtk)
    {
        printk(KERN_ERR "  RECSRC MASTER: %d\n", dpcm->mtk->capture_source[MIXER_ADDR_MASTER][0]);
        printk(KERN_ERR "  RECSRC LINE: %d\n", dpcm->mtk->capture_source[MIXER_ADDR_LINE][0]);
        printk(KERN_ERR "  RECSRC MIC: %d\n", dpcm->mtk->capture_source[MIXER_ADDR_MIC][0]);
        printk(KERN_ERR "  RECSRC SYNTH: %d\n", dpcm->mtk->capture_source[MIXER_ADDR_SYNTH][0]);
        printk(KERN_ERR "  RECSRC CD: %d\n", dpcm->mtk->capture_source[MIXER_ADDR_CD][0]);
    }
    else
    {
        printk(KERN_ERR "  dpcm->mtk is null pointer !!!\n");
    }
    #endif
    #endif

    //AUD_InitALSARecord();

    #if FIRST_RECORD_DBG
    first_record = 0;
    #endif

	mtk->timer_ops = &mtk_systimer_ops;
	err = mtk->timer_ops->create(substream);
	if (err < 0)
		return err;

	runtime->hw = mtk_pcm_capture_hardware; //MTK

	if (substream->pcm->device & 1) {
		runtime->hw.info &= ~SNDRV_PCM_INFO_INTERLEAVED;
		runtime->hw.info |= SNDRV_PCM_INFO_NONINTERLEAVED;
	}
	if (substream->pcm->device & 2)
		runtime->hw.info &= ~(SNDRV_PCM_INFO_MMAP | SNDRV_PCM_INFO_MMAP_VALID);

	return 0;
}

static int mtk_pcm_playback_close(struct snd_pcm_substream *substream)
{
	struct snd_mtk *mtk = snd_pcm_substream_chip(substream);
	mtk->timer_ops->free(substream);

    printk(KERN_ERR "snd_card_mtk_playback_close(%d,%d)\n", substream->number, substream->stream); //0513

    vDeInitAlsaPlayback(substream->number);
    
  #if 0 //Test for DSP Path
    memset(VIRTUAL(u4PlayBufSA[substream->number]), 0, u4PlayBufEA[substream->number] - u4PlayBufSA[substream->number]);
    vFlushDram(u4PlayBufSA[substream->number], u4PlayBufEA[substream->number] - u4PlayBufSA[substream->number]);
    vSetPlayBufWA(substream->number, u4PlayBufSA[substream->number]);
  #endif
    
	return 0;
}

static int mtk_pcm_capture_close(struct snd_pcm_substream *substream)
{
	struct snd_mtk *mtk = snd_pcm_substream_chip(substream);
	mtk->timer_ops->free(substream);

    #if ALSA_DBG_MSG
    printk(KERN_ERR "!@#!@# @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ snd_card_mtk_capture_close\n"); //0513    
    #endif

    AUD_DeInitALSARecord();

	return 0;
}

static struct snd_pcm_ops mtk_pcm_playback_ops = {
	.open =		mtk_pcm_playback_open,
	.close =	mtk_pcm_playback_close,
	.ioctl =	snd_pcm_lib_ioctl,
	.hw_params =	mtk_pcm_hw_params,
	.hw_free =	mtk_pcm_hw_free,
	.prepare =	mtk_pcm_playback_prepare,
	.trigger =	mtk_pcm_trigger,
	.pointer =	mtk_pcm_playback_pointer,
};

static struct snd_pcm_ops mtk_pcm_capture_ops = {
	.open =		mtk_pcm_capture_open,
	.close =	mtk_pcm_capture_close,
	.ioctl =	snd_pcm_lib_ioctl,
	.hw_params =	mtk_pcm_hw_params,
	.hw_free =	mtk_pcm_hw_free,
	.prepare =	mtk_pcm_capture_prepare,
	.trigger =	mtk_pcm_trigger,
	.pointer =	mtk_pcm_capture_pointer,
};

static int __devinit snd_card_mtk_pcm(struct snd_mtk *mtk, int device,
					int substreams)
{
	struct snd_pcm *pcm;
	struct snd_pcm_ops *playback_ops;
    struct snd_pcm_ops *capture_ops;
	int err;

    #if ALSA_DBG_MSG
    printk(KERN_ERR "!@#!@# @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ snd_card_mtk_pcm\n");
    #endif

	err = snd_pcm_new(mtk->card, "mtk PCM", device,
			       substreams, substreams, &pcm);
	if (err < 0)
		return err;
	mtk->pcm = pcm;

	playback_ops = &mtk_pcm_playback_ops;
	capture_ops = &mtk_pcm_capture_ops;

	snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_PLAYBACK, playback_ops);
	snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_CAPTURE, capture_ops);
	pcm->private_data = mtk;
	pcm->info_flags = 0;

	strcpy(pcm->name, "mtk PCM");

	snd_pcm_lib_preallocate_pages_for_all(pcm,
		SNDRV_DMA_TYPE_CONTINUOUS,
		snd_dma_continuous_data(GFP_KERNEL),
		0, 64*1024);

	return 0;
}

/*
 * mixer interface
 */

#define MTK_VOLUME(xname, xindex, addr) \
{ .iface = SNDRV_CTL_ELEM_IFACE_MIXER, \
  .access = SNDRV_CTL_ELEM_ACCESS_READWRITE | SNDRV_CTL_ELEM_ACCESS_TLV_READ, \
  .name = xname, .index = xindex, \
  .info = snd_mtk_volume_info, \
  .get = snd_mtk_volume_get, .put = snd_mtk_volume_put, \
  .private_value = addr, \
  .tlv = { .p = db_scale_mtk } }

static int snd_mtk_volume_info(struct snd_kcontrol *kcontrol,
				 struct snd_ctl_elem_info *uinfo)
{
#if ALSA_DBG_MSG_MIXER
    printk(KERN_ERR "!@#!@# snd_mtk_volume_info\n");
#endif

	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 2;
	uinfo->value.integer.min = -50;
	uinfo->value.integer.max = 100;
	return 0;
}
 
static int snd_mtk_volume_get(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	struct snd_mtk *mtk = snd_kcontrol_chip(kcontrol);
	int addr = kcontrol->private_value;

#if ALSA_DBG_MSG_MIXER
    printk(KERN_ERR "!@#!@# snd_mtk_volume_get (addr=%d)\n", addr);
    printk(KERN_ERR "!@#!@# mtk->mixer_volume[%d][0]: %d\n", addr, mtk->mixer_volume[addr][0]);
    printk(KERN_ERR "!@#!@# mtk->mixer_volume[%d][1]: %d\n", addr, mtk->mixer_volume[addr][1]);
#endif

	spin_lock_irq(&mtk->mixer_lock);
	ucontrol->value.integer.value[0] = mtk->mixer_volume[addr][0];
	ucontrol->value.integer.value[1] = mtk->mixer_volume[addr][1];
	spin_unlock_irq(&mtk->mixer_lock);
	return 0;
}

static int snd_mtk_volume_put(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	struct snd_mtk *mtk = snd_kcontrol_chip(kcontrol);
	int change, addr = kcontrol->private_value;
	int left, right;

#if ALSA_DBG_MSG_MIXER
    printk(KERN_ERR "!@#!@# snd_mtk_volume_put (addr=%d)\n", addr);
    printk(KERN_ERR "!@#!@# ucontrol->value.integer.value[0]: %d\n", ucontrol->value.integer.value[0]);
    printk(KERN_ERR "!@#!@# ucontrol->value.integer.value[1]: %d\n", ucontrol->value.integer.value[1]);
#endif

	left = ucontrol->value.integer.value[0];
	if (left < -50)
		left = -50;
	if (left > 100)
		left = 100;
	right = ucontrol->value.integer.value[1];
	if (right < -50)
		right = -50;
	if (right > 100)
		right = 100;
	spin_lock_irq(&mtk->mixer_lock);
	change = mtk->mixer_volume[addr][0] != left ||
	         mtk->mixer_volume[addr][1] != right;
	mtk->mixer_volume[addr][0] = left;
	mtk->mixer_volume[addr][1] = right;
	spin_unlock_irq(&mtk->mixer_lock);
	return change;
}

static const DECLARE_TLV_DB_SCALE(db_scale_mtk, -4500, 30, 0);

#define MTK_CAPSRC(xname, xindex, addr) \
{ .iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = xname, .index = xindex, \
  .info = snd_mtk_capsrc_info, \
  .get = snd_mtk_capsrc_get, .put = snd_mtk_capsrc_put, \
  .private_value = addr }

#define snd_mtk_capsrc_info	snd_ctl_boolean_stereo_info
 
static int snd_mtk_capsrc_get(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	struct snd_mtk *mtk = snd_kcontrol_chip(kcontrol);
	int addr = kcontrol->private_value;

#if ALSA_DBG_MSG_MIXER
    printk(KERN_ERR "!@#!@# snd_mtk_capsrc_get (addr=%d)\n", addr);
    printk(KERN_ERR "!@#!@# mtk->capture_source[%d][0]: %d\n", addr, mtk->capture_source[addr][0]);
    printk(KERN_ERR "!@#!@# mtk->capture_source[%d][1]: %d\n", addr, mtk->capture_source[addr][1]);
#endif

	spin_lock_irq(&mtk->mixer_lock);
	ucontrol->value.integer.value[0] = mtk->capture_source[addr][0];
	ucontrol->value.integer.value[1] = mtk->capture_source[addr][1];
	spin_unlock_irq(&mtk->mixer_lock);
	return 0;
}

static int snd_mtk_capsrc_put(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_mtk *mtk = snd_kcontrol_chip(kcontrol);
	int change, addr = kcontrol->private_value;
	int left, right;

#if ALSA_DBG_MSG_MIXER
    printk(KERN_ERR "!@#!@# snd_mtk_capsrc_put (addr=%d)\n",addr);
    printk(KERN_ERR "!@#!@# ucontrol->value.integer.value[0]: %d\n", ucontrol->value.integer.value[0]);
    printk(KERN_ERR "!@#!@# ucontrol->value.integer.value[1]: %d\n", ucontrol->value.integer.value[1]);
#endif

	left = ucontrol->value.integer.value[0] & 1;
	right = ucontrol->value.integer.value[1] & 1;
	spin_lock_irq(&mtk->mixer_lock);
	change = mtk->capture_source[addr][0] != left &&
	         mtk->capture_source[addr][1] != right;
	mtk->capture_source[addr][0] = left;
	mtk->capture_source[addr][1] = right;
	spin_unlock_irq(&mtk->mixer_lock);
	return change;
}

static struct snd_kcontrol_new snd_mtk_controls[] = 
{
MTK_VOLUME("Master Volume", 0, MIXER_ADDR_MASTER),
MTK_CAPSRC("Master Capture Switch", 0, MIXER_ADDR_MASTER),
MTK_VOLUME("Synth Volume", 0, MIXER_ADDR_SYNTH),
MTK_CAPSRC("Synth Capture Switch", 0, MIXER_ADDR_SYNTH),
MTK_VOLUME("Line Volume", 0, MIXER_ADDR_LINE),
MTK_CAPSRC("Line Capture Switch", 0, MIXER_ADDR_LINE),
MTK_VOLUME("Mic Volume", 0, MIXER_ADDR_MIC),
MTK_CAPSRC("Mic Capture Switch", 0, MIXER_ADDR_MIC),
MTK_VOLUME("CD Volume", 0, MIXER_ADDR_CD),
MTK_CAPSRC("CD Capture Switch", 0, MIXER_ADDR_CD)
};

static int __devinit snd_card_mtk_new_mixer(struct snd_mtk *mtk)
{
	struct snd_card *card = mtk->card;
	unsigned int idx;
	int err;

    #if ALSA_DBG_MSG || ALSA_DBG_MSG_MIXER
    printk(KERN_ERR "!@#!@# @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ snd_card_mtk_new_mixer\n");
    #endif

	spin_lock_init(&mtk->mixer_lock);
	strcpy(card->mixername, "mtk Mixer");

	for (idx = 0; idx < ARRAY_SIZE(snd_mtk_controls); idx++) 
    {
		err = snd_ctl_add(card, snd_ctl_new1(&snd_mtk_controls[idx], mtk));
		if (err < 0)
			return err;
	}
	return 0;
}

static int __devinit snd_mtk_probe(struct platform_device *devptr)
{
	struct snd_card *card;
	struct snd_mtk *mtk;
	int idx, err;
	int dev = devptr->id;

    #if ALSA_DBG_MSG
    printk(KERN_ERR "!@#!@# @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ snd_mtk_probe\n");
    #endif

	err = snd_card_create(index[dev], id[dev], THIS_MODULE,
			      sizeof(struct snd_mtk), &card);
	if (err < 0)
		return err;
	mtk = card->private_data;
	mtk->card = card;

	for (idx = 0; idx < MAX_PCM_DEVICES && idx < pcm_devs[dev]; idx++) {
		if (pcm_substreams[dev] < 1)
			pcm_substreams[dev] = 1;
		if (pcm_substreams[dev] > MAX_PCM_SUBSTREAMS)
			pcm_substreams[dev] = MAX_PCM_SUBSTREAMS;
		err = snd_card_mtk_pcm(mtk, idx, pcm_substreams[dev]);
		if (err < 0)
			goto __nodev;
	}

	err = snd_card_mtk_new_mixer(mtk);
	if (err < 0)
		goto __nodev;
	strcpy(card->driver, "mtk");
	strcpy(card->shortname, "mtk");
	sprintf(card->longname, "mtk %i", dev + 1);

	snd_card_set_dev(card, &devptr->dev);

	err = snd_card_register(card);
	if (err == 0) 
    {
		platform_set_drvdata(devptr, card);
		return 0;
	}
    
__nodev:
	snd_card_free(card);
	return err;
}

static int __devexit snd_mtk_remove(struct platform_device *devptr)
{
	snd_card_free(platform_get_drvdata(devptr));
	platform_set_drvdata(devptr, NULL);
	return 0;
}

#define SND_MTK_DRIVER	"snd_mtk"

static struct platform_driver snd_mtk_driver = 
{
	.probe		= snd_mtk_probe,
	.remove		= __devexit_p(snd_mtk_remove),
	.driver		= 
    {
		.name	= SND_MTK_DRIVER
	},
};

static void snd_mtk_unregister_all(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(devices); ++i)
		platform_device_unregister(devices[i]);
	platform_driver_unregister(&snd_mtk_driver);
}

static int __init alsa_card_mtk_init(void)
{
	int i, cards, err;

    printk(KERN_ERR "!@#!@# @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ alsa_card_mtk_init %s\n",MTK_ALSA_VERSION_CODE);

	err = platform_driver_register(&snd_mtk_driver);
	if (err < 0)
		return err;

	cards = 0;
	for (i = 0; i < SNDRV_CARDS; i++) 
    {
		struct platform_device *device;

        #if ALSA_DBG_MSG
        printk(KERN_ERR "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ try - %d\n", i);	    
        #endif
		if (! enable[i])
        {
            #if ALSA_DBG_MSG
            printk(KERN_ERR "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ !enable[i] continue - %d\n", i);	    
            #endif
			continue;
        }
		device = platform_device_register_simple(SND_MTK_DRIVER,i, NULL, 0);
        #if ALSA_DBG_MSG
        //printk(KERN_ERR "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ device 0x%08x 0x%08x 0x%08x %s - %d\n", device, device->dev, device->dev.driver_data, device->name, i);
        #endif

		if (IS_ERR(device))
		{
            #if ALSA_DBG_MSG
            printk(KERN_ERR "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ IS_ERR(device) continue - %d\n", i);	    
            #endif
			continue;
		}
		if (!platform_get_drvdata(device)) 
        {
            #if ALSA_DBG_MSG
            printk(KERN_ERR "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ opm_get_drvdata fail - %d\n", i);
            #endif
			platform_device_unregister(device);
			continue;
		}
		devices[i] = device;
		cards++;
	}
	if (!cards) 
    {
        #if ALSA_DBG_MSG
        printk(KERN_ERR "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ mtk soundcard not found or device busy\n");	    
        #endif
#ifdef MODULE
		printk(KERN_ERR "mtk soundcard not found or device busy\n");
#endif
		snd_mtk_unregister_all();
		return -ENODEV;
	}

    #if ALSA_DBG_MSG
    printk(KERN_ERR "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ alsa_card_mtk_init end\n");
    #endif

	return 0;
}

static void __exit alsa_card_mtk_exit(void)
{
	snd_mtk_unregister_all();
}

module_init(alsa_card_mtk_init)
module_exit(alsa_card_mtk_exit)
