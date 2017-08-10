/*
* ---------------------------------------------------------------------------
* Copyright (C) 2014 by Nanosic.
*
*
* ---------------------------------------------------------------------------
*/

/*
*======== vaudio_card.h ========
*   virtual audio card driver .
*
*/


#ifndef __VAUDIO_CARD__
#define __VAUDIO_CARD__

#define PCM_DATA_MAX 512
#define VAUDIO_DEBUG

#include <linux/init.h>
#include <linux/spinlock.h>
#include <linux/mutex.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/core.h>

struct vaudio_input_req{
    unsigned int size;
    unsigned char data[PCM_DATA_MAX];
};

struct vaudio_event{
    unsigned int  type;
    struct vaudio_input_req input;
};



//struct snd_vaudio_dev {
//struct snd_card *card;
//  struct snd_upcm_stream *as;

//  bool running;
//  struct mutex devlock;
//  struct vaudio_event input_event;

//};

//struct snd_upcm_stream {
//  struct snd_vaudio_dev *audio_dev;
//  struct snd_pcm *pcm;
/// struct snd_pcm_substream *pcm_substream;

//  unsigned int dma_buffer_pointer;    /* processed byte position in the buffer */
//  unsigned int trans_done_frames;     /* processed frames since last period update */
//  spinlock_t lock;
//};





struct vaudio_pcm_dev{
    struct snd_pcm *pcm;
    struct snd_pcm_substream *pcm_substream;
    struct vaudio_audio* audio_dev;

    unsigned int dma_buffer_pointer;    /* processed byte position in the buffer */
    unsigned int dma_period_counter;        /* trans_done_frames processed frames since last period update */
    spinlock_t lock;
};


struct vaudio_audio{
    struct snd_card *card;
    struct vaudio_pcm_dev *pcm_dev;
    struct vaudio_event input_event;

    void (*pcm_stream_open_cb)(struct vaudio_pcm_dev *pcm_dev);
    void (*pcm_stream_close_cb)(struct vaudio_pcm_dev *pcm_dev);
    bool running;
    struct mutex devlock;
};


int vaudio_create(struct vaudio_audio *audio_dev);
int vaudio_destroy(struct vaudio_audio *audio_dev);
int vaudio_input(struct vaudio_audio *audio_dev, struct vaudio_event *ev);



#endif
