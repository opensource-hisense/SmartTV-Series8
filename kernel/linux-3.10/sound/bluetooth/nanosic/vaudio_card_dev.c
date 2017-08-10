/*
* ---------------------------------------------------------------------------
* Copyright (C) 2014 by Nanosic.
*
*
* ---------------------------------------------------------------------------
*/

/*
*   ======== vaudio_card_dev.c ========
*   virtual audio devices .
*
*/


#include "vaudio_card_dev.h"
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/poll.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/delay.h>
#include <linux/semaphore.h>
#include <linux/spinlock.h>
#include <linux/sched.h>   //wake_up_process()
#include <linux/kthread.h> //kthread_create()¡¢kthread_run()
#include <linux/delay.h>

#include "vaudio_card.h"

#define DEV_NAME "vaudio"

struct vaudio_audio *vaudio_dev ;

int dwDebugLevel = 0;
module_param(dwDebugLevel, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(dwDebugLevel,"vaudio Debug level");


#define CACHE_LENGT 4*1024

char thread_inited = 0;
bool fill_void_pcm_data = false;
char pcm_cache[CACHE_LENGT];
unsigned int cache_write_pos = 0;
unsigned int cache_read_pos = 0;
unsigned int cache_pcm_frames = 0;
unsigned char pcm_frame_bits = 2;
spinlock_t cache_lock;

//16k *16bit  256,000bits/s
// pcm read interval ,ms
unsigned char fill_pcm_data_interval = 10;
//pcm data size  read ervery times
#define PCM_READ_DATA_SIZE 512
struct semaphore fill_pcm_sem;

struct task_struct *pcm_fill_task;

void write_pcm_data(char* pcm_data,unsigned short length)
{
    unsigned int  bytes = 0, oldptr = 0,cache_length = 0;
    unsigned long flags = 0;
    unsigned char *cp = NULL;
    cache_length = CACHE_LENGT;
    oldptr = cache_write_pos;
    cp = pcm_data;
    bytes = length;

    spin_lock_irqsave(&cache_lock, flags);
    cache_write_pos += bytes;
    if (cache_write_pos >= cache_length)
    {
        cache_write_pos -= cache_length;
    }
    cache_pcm_frames += bytes / pcm_frame_bits;
    if (oldptr + bytes > cache_length) 
    {
        unsigned int bytes1 = cache_length - oldptr;
        memcpy(pcm_cache + oldptr, cp, bytes1);
        memcpy(pcm_cache, cp + bytes1, bytes - bytes1);
    } 
    else 
    {
        memcpy(pcm_cache + oldptr, cp, bytes);
    }
    spin_unlock_irqrestore(&cache_lock, flags);

    if(dwDebugLevel)
        printk("[vaudio]: write_pcm_data write_pos=%d cache_pcm_frames=%d pcm_length=%d \n",cache_write_pos,cache_pcm_frames,bytes);
}

void read_pcm_data(char* pcm_data,unsigned short read_length)
{
    unsigned int   bytes = 0, oldptr = 0,cache_length = 0,cache_pcm_frame_bytes = 0;
    unsigned long flags = 0;
    unsigned char *cp = 0;

    cache_length = CACHE_LENGT;
    oldptr = cache_read_pos;
    cp = pcm_cache+cache_read_pos;
    bytes = read_length;

    spin_lock_irqsave(&cache_lock, flags);
    cache_pcm_frame_bytes = cache_pcm_frames*pcm_frame_bits ;
    if(cache_pcm_frame_bytes < read_length){
        memset(pcm_data, 0, read_length);
        bytes = cache_pcm_frame_bytes ;
    }
    cache_read_pos += bytes;
    if (cache_read_pos >= cache_length)
        cache_read_pos -= cache_length;
    cache_pcm_frames -= bytes / pcm_frame_bits;
    spin_unlock_irqrestore(&cache_lock, flags);

    if(bytes == 0)
    {
        if(dwDebugLevel)
            printk("[vaudio]:=>!read_pcm_data void pcm data!!|read_pos=%d cache_pcm_frames=%d \n",cache_read_pos,cache_pcm_frames);
        return;
    }
    if (oldptr + bytes > cache_length)
    {
        unsigned int bytes1 = cache_length - oldptr;
        memcpy(pcm_data + oldptr, cp, bytes1);
        memcpy(pcm_data, cp + bytes1, bytes - bytes1);
    } 
    else 
    {
        memcpy(pcm_data + oldptr, cp, bytes);
    }
    if(dwDebugLevel)
        printk("[vaudio]:=>read_pcm_data  read_pos=%d cache_pcm_frames=%d pcm_length=%d \n",cache_read_pos,cache_pcm_frames,bytes);

}
int thread_fill_pcm_data(void *data)
{
    struct vaudio_audio *audio = data;
    if(audio == NULL)
    {
        printk("[vaudio]: fill_void_data vaudio_audio not alloc \n");
        return -1;
    }
    msleep(2000);
    while(!kthread_should_stop())
    {
        if(!fill_void_pcm_data)
        {
            printk("[vaudio]: fill_void_pcm_data : stop\n");
            down(&fill_pcm_sem);
            printk("[vaudio]: fill_void_pcm_data : start\n");
        }

        if(audio->running&& audio->pcm_dev)
        {
            read_pcm_data(audio->input_event.input.data,PCM_READ_DATA_SIZE);
            audio->input_event.input.size = PCM_READ_DATA_SIZE;
            vaudio_input(audio,&audio->input_event);
        }
        else
        {
            printk("[vaudio]: pcm_dev not created \n");
            fill_void_pcm_data = false;
        }
        msleep(fill_pcm_data_interval);
    }
    printk("[vaudio]: fill_void_data exit \n");
    return 0;
}

void vaudio_card_pcm_stream_open_cb(struct vaudio_pcm_dev *pcm_dev)
{
    unsigned long flags = 0;
    if(dwDebugLevel)
        printk("[vaudio]: vaudio_card_pcm_stream_open_cb\n");
    fill_void_pcm_data = true;
    spin_lock_irqsave(&cache_lock, flags);
    cache_write_pos = 0;
    cache_read_pos = 0;
    cache_pcm_frames = 0;
    spin_unlock_irqrestore(&cache_lock, flags);

    up(&fill_pcm_sem);
}

void vaudio_card_pcm_stream_close_cb(struct vaudio_pcm_dev *pcm_dev)
{
    if(dwDebugLevel)
        printk("[vaudio]: vaudio_card_pcm_stream_close_cb\n");
    fill_void_pcm_data = false;
}

static int vaudio_char_open(struct inode *inode, struct file *file)
{
    if(dwDebugLevel)
        printk("[vaudio]: vaudio_char_open\n");
    file->private_data = vaudio_dev;
    nonseekable_open(inode, file);
    return 0;
}


static ssize_t vaudio_char_read(struct file *file, char __user *buffer,size_t count, loff_t *ppos)
{
    if(dwDebugLevel)
        printk("[vaudio]: vaudio_char_read\n");
    return 0;
}
static ssize_t vaudio_char_write(struct file *file, const char __user *buffer,size_t count, loff_t *ppos)
{
    struct vaudio_audio *vaudio_dev = file->private_data;
    int ret = 0;
    size_t len = 0;
    if(dwDebugLevel)
        printk("[vaudio] vaudio_char_write count=%d ,running=%d \n",count,vaudio_dev->running);

    if (count < sizeof(unsigned int))
    {
        return -EINVAL;
    }
    if(vaudio_dev->running == false)
    {
        return -ENODEV;
    }
//  memset(&vaudio_dev->input_event, 0, sizeof(vaudio_dev->input_event));
    len = min(count, sizeof(vaudio_dev->input_event));
    if (copy_from_user(&vaudio_dev->input_event, buffer, len))
    {
        return -EFAULT;
    }
    if(dwDebugLevel)
        printk("[vaudio] input event:  type=%d, datasize=%d \n",vaudio_dev->input_event.type,vaudio_dev->input_event.input.size);

    switch (vaudio_dev->input_event.type)
    {
        case START:
        {
            fill_void_pcm_data = false;
            break;
        }
        case STOP:
        {
            if(vaudio_dev->pcm_dev && vaudio_dev->pcm_dev->pcm_substream)
            {
                if(!fill_void_pcm_data)
                {
                    up(&fill_pcm_sem);
                }
                fill_void_pcm_data = true;
            }
            break;
        }
        case INPUT:
        {
            if(vaudio_dev->running == true){
                fill_void_pcm_data = false;
                vaudio_input(vaudio_dev,&vaudio_dev->input_event);
            }
            break;
        }
        default:
        {
            ret = -EOPNOTSUPP;
            break;
        }
    }

    /* return "count" not "len" to not confuse the caller */
    return ret ? ret : count;
}


static int  vaudio_char_release(struct inode *inode, struct file *file)
{
    file->private_data = NULL;
    if(dwDebugLevel)
        printk("[vaudio]: vaudio_char_release\n");
    return 0;
}

 void thread_create(void *data)
 {
    pcm_fill_task= kthread_create(thread_fill_pcm_data, data, "vauido");
    if (!IS_ERR(pcm_fill_task) )
    {
        wake_up_process(pcm_fill_task);
        if(dwDebugLevel)
            printk("[vaudio]: thread_create  \n");

    }
    else
    {
        printk("[vaudio]: thread_create failed \n");
    }
}

void thread_destory(void)
{
    if(pcm_fill_task)
    {
        kthread_stop(pcm_fill_task);
        pcm_fill_task = NULL;
    }
    printk("[vaudio]: thread_destory  \n");
}




static const struct file_operations file_ops = {
    .owner      = THIS_MODULE,
    .open       = vaudio_char_open,
    .release    = vaudio_char_release,
    .read       = vaudio_char_read,
    .write      = vaudio_char_write,
    .llseek     = no_llseek,
};

static struct miscdevice misc_dev = {
    .fops       = &file_ops,
    .minor      = 131,
    .name       = DEV_NAME,
};

static int __init vaudio_dev_init(void)
{
    vaudio_dev = kzalloc(sizeof(*vaudio_dev), GFP_KERNEL);
    if (!vaudio_dev){
        printk("[vaudio]: snd_vaudio_dev kzalloc  failed\n");
        return -ENOMEM;
    }
    mutex_init(&vaudio_dev->devlock);
    vaudio_dev->pcm_stream_close_cb = vaudio_card_pcm_stream_close_cb;
    vaudio_dev->pcm_stream_open_cb = vaudio_card_pcm_stream_open_cb;
    spin_lock_init(&cache_lock);
    sema_init(&fill_pcm_sem, 1);

    thread_create(vaudio_dev);
    if(dwDebugLevel)
        printk("[vaudio]: vaudio_dev_init\n");
    vaudio_create(vaudio_dev);
    return misc_register(&misc_dev);
}

static void __exit vaudio_dev_exit(void)
{
    if(vaudio_dev)
    {
        kfree(vaudio_dev);
        vaudio_dev= NULL;
    }

    misc_deregister(&misc_dev);

    thread_destory();

    vaudio_destroy(vaudio_dev);
    if(dwDebugLevel)
        printk("[vaudio]: vaudio_dev_exit\n");
}

module_init(vaudio_dev_init);
module_exit(vaudio_dev_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Nanosic");
MODULE_DESCRIPTION("virtual audio card device");

