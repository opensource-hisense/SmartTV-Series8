/*
* ---------------------------------------------------------------------------
* Copyright (C) 2014 by Nanosic.
*
*
* ---------------------------------------------------------------------------
*/

/*
 *  ======== vaudio_card_dev.h ========
 *  virtual audio devices .
 *
 */



#ifndef __VIRTUAL_AUDIO_CARD_DEV__
#define __VIRTUAL_AUDIO_CARD_DEV__

#include <linux/types.h>


enum event_type{
    START = 0,
    INPUT,
    STOP,
};

#define true 1
#define false 0

#endif

