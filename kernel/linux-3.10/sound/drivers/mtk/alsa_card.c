/*
* linux/sound/drivers/mtk/alsa_card.c
*
* MTK Sound Card Driver
*
* Copyright (c) 2010-2012 MediaTek Inc.
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
#include <linux/init.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/jiffies.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/wait.h>
#include <linux/module.h>
#include <linux/switch.h>
#include <linux/workqueue.h>

#include <sound/core.h>
#include <sound/control.h>
#include <sound/tlv.h>
#include <sound/pcm.h>
#include <sound/rawmidi.h>
#include <sound/initval.h>

#include "alsa_pcm.h"

#ifndef MTK_AUDIO_SUPPORT_MULTI_STREAMOUT 
#define MAX_PCM_DEVICES     1
#define MAX_PCM_SUBSTREAMS  2
#else
 #ifndef SUPPORT_NEW_TUNAL_MODE
#define MAX_PCM_DEVICES     4
#define MAX_PCM_SUBSTREAMS  2
 #else
   #define MAX_PCM_DEVICES     6
   #define MAX_PCM_SUBSTREAMS  2
 #endif
#endif

static int index[SNDRV_CARDS] = SNDRV_DEFAULT_IDX;  /* Index 0-MAX */
static char *id[SNDRV_CARDS] = SNDRV_DEFAULT_STR;   /* ID for this card */

#ifndef SUPPORT_COMPRESS_OFFLOAD
static int enable[SNDRV_CARDS] = {1, [1 ... (SNDRV_CARDS - 1)] = 0};
#else
static int enable[SNDRV_CARDS] = {1, 1, 0, [3 ... (SNDRV_CARDS - 1)] = 0};
#endif

#ifndef MTK_AUDIO_SUPPORT_MULTI_STREAMOUT
static int pcm_devs[SNDRV_CARDS] = {[0 ... (SNDRV_CARDS - 1)] = 1};
#else
#ifndef SUPPORT_COMPRESS_OFFLOAD
static int pcm_devs[SNDRV_CARDS] = {[0 ... (SNDRV_CARDS - 1)] = 4};
#else
   #ifndef SUPPORT_NEW_TUNAL_MODE
static int pcm_devs[SNDRV_CARDS] = {4,[1 ... (SNDRV_CARDS - 1)] = 1};
   #else
      static int pcm_devs[SNDRV_CARDS] = {6,[1 ... (SNDRV_CARDS - 1)] = 1};
   #endif
#endif
#endif

static struct platform_device *devices[SNDRV_CARDS];
#ifdef CONFIG_SWITCH
#define HP_DETECT_ANDROID
#endif
#ifdef HP_DETECT_ANDROID
static struct timer_list hp_detect_timer;
static struct switch_dev hp_switch;
static int hp_state = 0;
static struct work_struct hp_detect_work;
static struct workqueue_struct *hp_detect_workqueue;
extern char isHpPlugIn(void);
static void hp_detect_work_callback(struct work_struct *work) {
	printk("[HP_DETECT]hp_detect_work_callback receive work new send uevent\n");
	switch_set_state((struct switch_dev *)&hp_switch, hp_state);
}
static void do_detect(unsigned long para) {
	int ret = 0;
	//int tmp_state = isHpPlugIn()?2:0;
	int tmp_state;
	char tmp = isHpPlugIn();
	if(tmp == (char)1)
	{
	    tmp_state = 2;
	}else{
        tmp_state = 0;
	}
	//printk(KERN_DEBUG "[HP_DETECT]do_detect tmp_state:%d\n",tmp_state);
	if (tmp_state != hp_state) {
		printk("[HP_DETECT]headphone state changed from %d to %d\n", hp_state, tmp_state);
		hp_state = tmp_state;
		ret = queue_work(hp_detect_workqueue, &hp_detect_work);
		if (!ret) {
			printk("[HP_DETECT]queue_work return:%d\n",ret);
		}
	}
	del_timer(&hp_detect_timer);
	hp_detect_timer.data = (unsigned long) 1;
	hp_detect_timer.function = do_detect;
	hp_detect_timer.expires = jiffies + HZ/10;
	add_timer(&hp_detect_timer);
}

static void hp_timer_init(void) {
	int ret = 0;
	printk("[HP_DETECT]hp_timer_init\n");

	INIT_WORK(&hp_detect_work, hp_detect_work_callback);
	hp_detect_workqueue = create_singlethread_workqueue("hp_detect");
	hp_switch.name = "h2w";
	hp_switch.index = 0;
	hp_switch.state = 0;

	ret = switch_dev_register(&hp_switch);
	if (ret != 0) {
		printk("[HP_DETECT]register switch dev error ret:%d \n",ret);
	}
	
	init_timer(&hp_detect_timer);
	hp_detect_timer.data = (unsigned long) 1;
	hp_detect_timer.function = do_detect;
	hp_detect_timer.expires = jiffies + HZ/10;
	add_timer(&hp_detect_timer);
}
#endif

static int __devinit snd_mt85xx_probe(struct platform_device *devptr)
{

#ifndef SUPPORT_COMPRESS_OFFLOAD
    struct snd_card *card;
    struct snd_mt85xx *mt85xx;
    int idx, err;
    int dev = devptr->id;
#ifdef  ENABLE_DETAIL_LOG
    printk("[ALSA] snd_mt85xx_probe()  ###### 01 \n");
#endif

    printk("[ALSA] probe: devptr->id = %d\n", devptr->id);

    err = snd_card_create(index[dev], id[dev], THIS_MODULE,
                  sizeof(struct snd_mt85xx), &card);
#ifdef  ENABLE_DETAIL_LOG
    printk("[ALSA] snd_mt85xx_probe()  ###### 02 \n");
#endif
	
    if (err < 0)
        return err;

#ifdef  ENABLE_DETAIL_LOG
    printk("[ALSA] snd_mt85xx_probe()  ###### 03 \n");
#endif
	

    mt85xx = card->private_data;
    mt85xx->card = card;

#ifdef  ENABLE_DETAIL_LOG
    printk("[ALSA] snd_mt85xx_probe()  ###### 04 \n");
#endif
	

#ifdef MTK_AUDIO_SUPPORT_MULTI_STREAMOUT
    printk("[ALSA] pcm_devs[dev] = %d\n", pcm_devs[dev]);
#endif
    for (idx = 0; idx < MAX_PCM_DEVICES && idx < pcm_devs[dev]; idx++)
    {
        if ((err = snd_card_mt85xx_pcm(mt85xx, idx, MAX_PCM_SUBSTREAMS)) < 0)
            goto __nodev;
    }

    strcpy(card->driver, "mtk");
    strcpy(card->shortname, "mtk");
    sprintf(card->longname, "mtk %i", dev + 1);

    snd_card_set_dev(card, &devptr->dev);

    if ((err = snd_card_register(card)) == 0) {
        platform_set_drvdata(devptr, card);
        return 0;
    }

__nodev:
    snd_card_free(card);
    return err;
#else
    struct snd_card *card;
    struct snd_mt85xx *mt85xx;
    int idx, err;
    int dev = devptr->id;

    printk("[ALSA] probe: devptr->id = %d\n", devptr->id);

  if ( devptr->id != 2)
  {
    err = snd_card_create(index[dev], id[dev], THIS_MODULE,
                  sizeof(struct snd_mt85xx), &card);
    if (err < 0){
		printk(KERN_EMERG "[ALSA]snd_mt85xx_probe 4 err = %d\n", err);
        return err;
    	}

    mt85xx = card->private_data;
    mt85xx->card = card;

#ifdef  ENABLE_DETAIL_LOG
    printk("[ALSA] pcm_devs[dev] = %d\n", pcm_devs[dev]);
#endif


    for (idx = 0; idx < MAX_PCM_DEVICES && idx < pcm_devs[dev]; idx++)
    {
     #ifdef  ENABLE_DETAIL_LOG
       printk("[ALSA] snd_mt85xx_probe()  ###### 05 \n");
     #endif
    
        if ((err = snd_card_mt85xx_pcm(mt85xx, idx, MAX_PCM_SUBSTREAMS)) < 0)
            goto __nodev;
    }

    strcpy(card->driver, "mtk");
    strcpy(card->shortname, "mtk");
    sprintf(card->longname, "mtk %i", dev + 1);

    snd_card_set_dev(card, &devptr->dev);

    if ((err = snd_card_register(card)) == 0) {
        platform_set_drvdata(devptr, card);
		printk(KERN_EMERG "[ALSA]snd_mt85xx_probe 5\n");
        return 0;
    }
  }
  else
  {
#ifdef  ENABLE_DETAIL_LOG
    printk("[ALSA] snd_mt85xx_probe()  ###### 1 \n");
#endif
  
    //Now, devptr->id == 2, it's Compress Card
    err = snd_card_create(index[dev], id[dev], THIS_MODULE,
                  sizeof(struct snd_mt85xx), &card);
#ifdef  ENABLE_DETAIL_LOG
    printk("[ALSA] snd_mt85xx_probe()  ###### 2 \n");
#endif

    if (err < 0){
		printk(KERN_EMERG "[ALSA]snd_mt85xx_probe 6 err = %d\n", err);
        return err;
    	}

#ifdef  ENABLE_DETAIL_LOG
    printk("[ALSA] snd_mt85xx_probe()  ###### 3 \n");
#endif

    mt85xx = card->private_data;
    mt85xx->card = card;

#ifdef  ENABLE_DETAIL_LOG
    printk("[ALSA] pcm_devs[dev] = %d\n", pcm_devs[dev]);
#endif

#ifdef  ENABLE_DETAIL_LOG
    printk("[ALSA] snd_mt85xx_probe()  ###### 4 \n");
#endif

    for (idx = 0; idx < MAX_PCM_DEVICES && idx < pcm_devs[dev]; idx++)
    {
     #ifdef  ENABLE_DETAIL_LOG
       printk("[ALSA] snd_mt85xx_probe()  ###### 5 \n");
     #endif

        if ((err = snd_card_mt85xx_compress(mt85xx, idx, MAX_PCM_SUBSTREAMS)) < 0)
            goto __nodev;
    }

    strcpy(card->driver, "mtk");
    strcpy(card->shortname, "mtk");
    sprintf(card->longname, "mtk %i", dev + 1);

    snd_card_set_dev(card, &devptr->dev);

    if ((err = snd_card_register(card)) == 0) {
        platform_set_drvdata(devptr, card);
		printk(KERN_EMERG "[ALSA]snd_mt85xx_probe 7\n");
        return 0;
    }    
  
  }
__nodev:
    snd_card_free(card);
	printk(KERN_EMERG "[ALSA]snd_mt85xx_probe 8\n");
    return err;
#endif
}

static int __devexit snd_mt85xx_remove(struct platform_device *devptr)
{
    snd_card_free(platform_get_drvdata(devptr));
    platform_set_drvdata(devptr, NULL);
    return 0;
}

#define SND_MT85XX_DRIVER    "snd_mt85xx"

static struct platform_driver snd_mt85xx_driver = {
    .probe      = snd_mt85xx_probe,
    .remove     = __devexit_p(snd_mt85xx_remove),

    .driver     = {
        .name   = SND_MT85XX_DRIVER
    },
};

static void snd_mt85xx_unregister_all(void)
{
    int i;

    for (i = 0; i < ARRAY_SIZE(devices); ++i)
        platform_device_unregister(devices[i]);
    platform_driver_unregister(&snd_mt85xx_driver);
}

static int __init alsa_card_mt85xx_init(void)
{
    int i, cards, err;

#ifdef SUPPORT_COMPRESS_OFFLOAD
    int    cards_compress = 0;
#endif

    if ((err = platform_driver_register(&snd_mt85xx_driver)) < 0)
        return err;

    cards = 0;
    for (i = 0; i < SNDRV_CARDS; i++) {
        struct platform_device *device;
        if (! enable[i])
            continue;
#ifdef SUPPORT_COMPRESS_OFFLOAD
        if( 2 == i){
			//continue;
            //if there is any boot up issue, may do some fix here
}
#endif
        printk(KERN_EMERG "[ALSA]platform_device_register_simple cards begin: %d\n", i);
        device = platform_device_register_simple(SND_MT85XX_DRIVER,
                             i, NULL, 0);
        if (IS_ERR(device))
            continue;
        if (!platform_get_drvdata(device)) {
            platform_device_unregister(device);
            continue;
        }
        devices[i] = device;
        cards++;
    }

    printk("[ALSA] [Alex_2] alsa_card_mt85xx_init()  total cards = %d \n", cards);
#ifdef SUPPORT_COMPRESS_OFFLOAD
        cards_compress = 2;
  
       for (i = 0; i < 1; i++) {
        struct platform_device *device;
        printk(KERN_EMERG "[ALSA]platform_device_register_simple cards_compress begin: %d\n", cards_compress);
        device = platform_device_register_simple(SND_MT85XX_DRIVER,
                            cards_compress, NULL, 0);
        printk(KERN_EMERG "[ALSA]platform_device_register_simple cards_compress end\n");
        if (IS_ERR(device))
            continue;
        if (!platform_get_drvdata(device)) {
            platform_device_unregister(device);
            continue;
        }
        devices[cards_compress] = device;        
    }
#endif

  
    if (!cards) {
    #ifdef MODULE
        printk(KERN_ERR "mt85xx soundcard not found or device busy\n");
    #endif
        snd_mt85xx_unregister_all();
        return -ENODEV;
    }

    return 0;
}

static void __exit alsa_card_mt85xx_exit(void)
{
    snd_mt85xx_unregister_all();
}

static int __init alsa_mt85xx_init(void)
{
    printk(KERN_EMERG "[ALSA] alsa_mt85xx_init().\n");

    alsa_card_mt85xx_init();
	
#ifdef HP_DETECT_ANDROID
	hp_timer_init();
#endif
    return 0;
}

static void __exit alsa_mt85xx_exit(void)
{
    printk(KERN_EMERG "[ALSA] alsa_mt85xx_exit().\n");

    alsa_card_mt85xx_exit();
#ifdef HP_DETECT_ANDROID
		switch_dev_unregister(&hp_switch);
#endif
}


MODULE_DESCRIPTION("mt85xx soundcard");
MODULE_LICENSE("GPL");

module_init(alsa_mt85xx_init)
module_exit(alsa_mt85xx_exit)


