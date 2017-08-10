/*
* ---------------------------------------------------------------------------
* Copyright (C) 2014 by Nanosic.
*
*
* ---------------------------------------------------------------------------
*/

/*
*   ======== vaudio_card.c ========
*   virtual audio card driver .
*
*/

#include "vaudio_card.h"
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/poll.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/delay.h>
#include <linux/spinlock.h>
#include <linux/sched.h>   //wake_up_process()
#include <linux/kthread.h> //kthread_create()¡¢kthread_run()



#define AUDIO_CARD_NAME "NANOSIC VAUDIO"
#define PCM_CAPTURER_NAME "Vaudio Capture"
#define AUDIO_RATE 16000
#define AUTO_FILL_VOID_AUDIO
extern struct vaudio_audio *vaudio_dev ;
extern int dwDebugLevel;
inline int vaduio_input_pcm(struct vaudio_pcm_dev *pcm_dev ,struct vaudio_event *ev);
static struct snd_pcm_hardware snd_vaudio_capture_hw = {
    .info = (SNDRV_PCM_INFO_INTERLEAVED | SNDRV_PCM_INFO_MMAP 
             | SNDRV_PCM_INFO_MMAP_VALID | SNDRV_PCM_INFO_BLOCK_TRANSFER),
    .formats =          SNDRV_PCM_FMTBIT_S16_LE,
    .rates =            SNDRV_PCM_RATE_16000,
    .rate_min =         AUDIO_RATE,
    .rate_max =         AUDIO_RATE,
    .channels_min =     1,
    .channels_max =     1,
    .buffer_bytes_max = 1024 * 1024,
    .period_bytes_min = 1024,
    .period_bytes_max = 2 * 1024,
    .periods_min =      2,
    .periods_max =      64,
};
static int snd_vaudio_capture_open(struct snd_pcm_substream *substream)
{
    struct vaudio_pcm_dev *pcm_dev = substream->private_data;
    struct snd_pcm_runtime *runtime = substream->runtime;
    if(dwDebugLevel)
        printk("[vaudio]: snd_vaudio_capture_open\n");
    if( mutex_lock_interruptible(&(pcm_dev->audio_dev->devlock)) != 0)
    {
        printk("[vaudio]:snd_vaudio_capture_open  mutex_lock_interruptible failed \n");
    }
    runtime->hw = snd_vaudio_capture_hw;
    pcm_dev->pcm_substream = substream;
    //vaudio_dev->running = true;
    mutex_unlock(&pcm_dev->audio_dev->devlock);
    return 0;
}

static int snd_vaudio_capture_close(struct snd_pcm_substream *substream)
{
    if(dwDebugLevel)
        printk("[vaudio]: snd_vaudio_capture_close\n");
    //vaudio_dev->running = false;
    return 0;
}


static int snd_vaudio_hw_params(struct snd_pcm_substream *substream,struct snd_pcm_hw_params *hw_params)
{
    unsigned int channels, rate, format, period_bytes, buffer_bytes;
    int ret = 0;

    format = params_format(hw_params);
    rate = params_rate(hw_params);
    channels = params_channels(hw_params);
    period_bytes = params_period_bytes(hw_params);
    buffer_bytes = params_buffer_bytes(hw_params);

    if(dwDebugLevel)
        printk("[vaudio]: snd_vaudio_hw_params \n");
        printk("format %d, rate %d, channels %d, period_bytes %d, buffer_bytes %d\n",
                format, rate, channels, period_bytes, buffer_bytes);

    ret = snd_pcm_lib_alloc_vmalloc_buffer(substream,params_buffer_bytes(hw_params));
    if (ret < 0)
    {
        printk("[vaudio]: snd_pcm_lib_alloc_vmalloc_buffer failed %d\n", ret);
    }
    return ret;
}
static int snd_vaudio_hw_free(struct snd_pcm_substream *substream)
{
    struct vaudio_pcm_dev *pcm_dev = substream->private_data;
    int ret = 0;
    if(dwDebugLevel)
        printk("[vaudio]: snd_vaudio_hw_free\n");
    if(mutex_lock_interruptible(&(pcm_dev->audio_dev->devlock)) != 0)
    {
        printk("[vaudio]:snd_vaudio_capture_open  mutex_lock_interruptible failed \n");
    }
    pcm_dev->pcm_substream  = NULL;
    ret = snd_pcm_lib_free_vmalloc_buffer(substream);	
    mutex_unlock(&pcm_dev->audio_dev->devlock);
    return ret;
}
static int snd_vaudio_substream_capture_trigger(struct snd_pcm_substream *substream, int cmd)
{
    struct vaudio_pcm_dev *pcm_dev = substream->private_data;
    if(dwDebugLevel)
        printk("[vaudio]: snd_vaudio_substream_capture_trigger ,cmd %d \n", cmd);
    switch (cmd) 
    {
        case SNDRV_PCM_TRIGGER_START:
        {
            printk("[vaudio]: SNDRV_PCM_TRIGGER_START\n");
            pcm_dev->audio_dev->pcm_stream_open_cb(pcm_dev);
            break;
        }
        case SNDRV_PCM_TRIGGER_STOP:
        {
            printk("[vaudio]: SNDRV_PCM_TRIGGER_STOP\n");
        }
        case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
        {
            printk("[vaudio]: SNDRV_PCM_TRIGGER_PAUSE_PUSH\n");
        }
        case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
        {
            printk("[vaudio]: SNDRV_PCM_TRIGGER_PAUSE_RELEASE\n");
            pcm_dev->audio_dev->pcm_stream_close_cb(pcm_dev);
            break;
        }
        default:
        {
            break;
        }
    }
    return 0;
}
static int snd_vaudio_pcm_prepare(struct snd_pcm_substream *substream)
{
    struct vaudio_pcm_dev *pcm_dev = substream->private_data;
    if(dwDebugLevel)
        printk("[vaudio]: snd_vaudio_pcm_prepare \n");
    pcm_dev->dma_buffer_pointer = 0;
    pcm_dev->dma_period_counter= 0;
    if(dwDebugLevel)
        printk("[vaudio]: fill void auido data \n");
    return 0;
}

static snd_pcm_uframes_t snd_vaudio_pcm_pointer(struct snd_pcm_substream *substream)
{
    struct vaudio_pcm_dev *pcm_dev = substream->private_data;
    unsigned int buff_pointer = 0;
    spin_lock(&pcm_dev->lock);
    buff_pointer = pcm_dev->dma_buffer_pointer;
    spin_unlock(&pcm_dev->lock);
    return buff_pointer / (substream->runtime->frame_bits >> 3);
}
static struct snd_pcm_ops snd_vaudio_capture_ops = {
    .open =     snd_vaudio_capture_open,
    .close =    snd_vaudio_capture_close,
    .ioctl =    snd_pcm_lib_ioctl,
    .hw_params = snd_vaudio_hw_params,
    .hw_free =  snd_vaudio_hw_free,
    .prepare =  snd_vaudio_pcm_prepare,
    .trigger =  snd_vaudio_substream_capture_trigger,
    .pointer =  snd_vaudio_pcm_pointer,
    .page =     snd_pcm_lib_get_vmalloc_page,
};

//audio card free
static int snd_vaudio_audio_dev_free(struct snd_device *device)
{
    struct vaudio_audio *audio_dev = device->device_data;
    if (audio_dev) 
    {
        kfree(audio_dev);
        audio_dev = NULL;
    }
    return 0;
}
static struct snd_device_ops ops = 
{
   .dev_free = snd_vaudio_audio_dev_free,
};
//audio pcm free
static void snd_vaudio_audio_pcm_free(struct snd_pcm *pcm)
{
    struct vaudio_pcm_dev *pcm_dev = pcm->private_data;
    if (pcm_dev) 
    {
        kfree(pcm_dev);
        pcm_dev = NULL;
    }
}
int vaudio_create_streams(struct vaudio_audio *audio_dev)
{
    struct snd_pcm *pcm = NULL;
    struct vaudio_pcm_dev *pcm_dev = NULL;
    int ret = 0;
    /* create a new pcm */
    pcm_dev = kzalloc(sizeof(*pcm_dev), GFP_KERNEL);
    if (!pcm_dev)
    {
        return -ENOMEM;
    }
    ret = snd_pcm_new(audio_dev->card, PCM_CAPTURER_NAME, 0, 0, 1, &pcm);
    if (ret < 0) 
    {
        kfree(pcm_dev);
        return ret;
    }
    pcm_dev->pcm = pcm;
    pcm_dev->audio_dev = audio_dev;
    pcm->private_data = pcm_dev;
    pcm->private_free = snd_vaudio_audio_pcm_free;
    pcm->info_flags = 0;
    strcpy(pcm->name, PCM_CAPTURER_NAME);
    spin_lock_init(&pcm_dev->lock);

    audio_dev->pcm_dev = pcm_dev;
    audio_dev->running = true;

    snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_CAPTURE, &snd_vaudio_capture_ops);

    return ret;
}


int vaudio_create(struct vaudio_audio *audio_dev)
{
    struct snd_card *card = NULL;
    int ret = 0;
    if (audio_dev->running) 
    {
        ret = -EALREADY;
        return ret;
    }
    ret = mutex_lock_interruptible(&audio_dev->devlock);
    if (ret)
    {
        return ret;
    }
    ret = snd_card_create(1,AUDIO_CARD_NAME, THIS_MODULE, 0, &card);
    if (ret < 0) 
    {
        printk("[vaudio]: snd_card_create failed %d\n", ret);
        goto unlock;
    }
    audio_dev->card = card;
    if ((ret = snd_device_new(card, SNDRV_DEV_LOWLEVEL, audio_dev, &ops)) < 0) 
    {
        printk("[vaudio]: snd_device_new card failed %d\n", ret);
        goto err_free;
    }

    if((ret = vaudio_create_streams(audio_dev)) < 0)
    {
        printk("[vaudio]: vaudio_create_streams failed %d\n", ret);
        goto err_free;
    }

    if ((ret = snd_card_register(card)) < 0) 
    {
        printk("[vaudio]: snd_card_register failed %d\n", ret);
        goto err_free;
    }
    audio_dev->running = true;
    mutex_unlock(&audio_dev->devlock);
    if(dwDebugLevel)
        printk("[vaudio]: vaudio_create successful\n");
    return 0;
err_free:
    snd_card_free(card);
    audio_dev->running = false;
unlock:
    mutex_unlock(&audio_dev->devlock);
    return ret;

}

int vaudio_destroy(struct vaudio_audio *audio_dev)
{
    int ret = 0;
    if (!audio_dev->running) 
    {
        ret = -EINVAL;
        return ret;
    }
    ret = mutex_lock_interruptible(&audio_dev->devlock);
    if (ret)
    {
        return ret;
    }
    ret = snd_card_free_when_closed(audio_dev->card);
    if (ret < 0) 
    {
        printk("[vaudio]: snd_card_free_when_closed failed %d\n", ret);
        goto unlock;
    }

    audio_dev->running = false;
    if(dwDebugLevel)
        printk("[vaudio]: vaudio__destroy successful\n");
unlock:
    mutex_unlock(&audio_dev->devlock);
    return ret;

}
int vaudio_input(struct vaudio_audio *audio_dev ,struct vaudio_event *ev)
{
    struct snd_pcm_runtime *runtime = NULL;
    struct vaudio_pcm_dev *pcm_dev = audio_dev->pcm_dev;

    unsigned int stride = 0, frames = 0, bytes = 0, oldptr = 0;
    unsigned long flags = 0;
    int period_elapsed = 0;
    unsigned char *cp = NULL;
    if( (audio_dev) && (mutex_lock_interruptible(&audio_dev->devlock) != 0))
    {
        printk("[vaudio]:vaudio_input  mutex_lock_interruptible failed \n");
        return -10;
    }
    if(pcm_dev&&pcm_dev->pcm_substream)
    {
        if(pcm_dev->pcm_substream->runtime)
        {
            runtime = pcm_dev->pcm_substream->runtime;

            if(runtime->dma_area)
            {
                stride = runtime->frame_bits >> 3;
                bytes = ev->input.size;
                cp = ev->input.data;

                spin_lock_irqsave(&pcm_dev->lock, flags);
                oldptr = pcm_dev->dma_buffer_pointer;
                pcm_dev->dma_buffer_pointer += bytes;
                if (pcm_dev->dma_buffer_pointer >= runtime->buffer_size * stride)
                {
                    pcm_dev->dma_buffer_pointer -= runtime->buffer_size * stride;
                }
                frames = (bytes + (oldptr % stride)) / stride;
                pcm_dev->dma_period_counter += frames;
                if (pcm_dev->dma_period_counter >= runtime->period_size) 
                {
                    pcm_dev->dma_period_counter -= runtime->period_size;
                    period_elapsed = 1;
                }
                spin_unlock_irqrestore(&pcm_dev->lock, flags);
                if (oldptr + bytes > runtime->buffer_size * stride) 
                {
                    unsigned int bytes1 = runtime->buffer_size * stride - oldptr;
                    memcpy(runtime->dma_area + oldptr, cp, bytes1);
                    memcpy(runtime->dma_area, cp + bytes1, bytes - bytes1);
                } 
                else 
                {
                    memcpy(runtime->dma_area + oldptr, cp, bytes);
                }

                if (period_elapsed)
                {
                    snd_pcm_period_elapsed(pcm_dev->pcm_substream);
                    if(dwDebugLevel)
                        printk("[vaudio]:snd_pcm_period_elapsed ,call to read dma buff,pointer=%d\n", pcm_dev->dma_buffer_pointer);
                }
                if(dwDebugLevel)
                    printk("[vaudio]:dma_buffer_pointer=%d dma_period_counter %d, period_size %d,buffer_size %d,periods %d ,frame_bits %d\n", pcm_dev->dma_buffer_pointer,(int)pcm_dev->dma_period_counter, (int)runtime->period_size,(int)runtime->buffer_size,(int)runtime->periods,(int)runtime->frame_bits);
            }
            else
            {
                if(dwDebugLevel)
                printk("[vaudio]: runtime->dma_area null\n");
            }
        }
        else
        {
            if(dwDebugLevel)
            printk("[vaudio]: runtime null\n");
        }
    }
    else
    {
        if(dwDebugLevel)
        printk("[vaudio]: pcm_dev->pcm_substream is null\n");
    }
    mutex_unlock(&audio_dev->devlock);
    return 0;
}
