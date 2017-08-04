#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "mw_common.h"

#ifndef _NET_MEMLEAK_DEBUG
#define _NET_MEMLEAK_DEBUG
#endif

#include "memleak/net_memleak.h"

extern int mw_memleak_init (void);
int main (int argc, char** argv)
{
    mw_memleak_init ();
    malloc (10024);
    while (1) {
        malloc (6);
        usleep (50000);
    }
    return 0;
}

