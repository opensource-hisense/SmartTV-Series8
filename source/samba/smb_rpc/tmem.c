#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/un.h>
#include <dirent.h>
#include <sys/stat.h>
#include <dlfcn.h>
#include <fcntl.h>

#include <pthread.h>
#include <arpa/inet.h>

#include "smb_rpc.h"

#ifdef _NET_MEMLEAK_DEBUG
extern int mw_memleak_ck (int pid, int time, int size);
#else
int mw_memleak_ck (int i, int j, int k)
{
    fprintf (stderr, "not support\n");
    return -1;
}
#endif
int main (int argc, char** argv)
{
    int time = 0, size = 0;
    if (argc < 2) {
        fprintf (stderr, "Usage: %s target_pid [time] [size]\n", argv[0]);
        return -1;
    }
    if (argc > 2) time = atoi (argv[2]);
    if (argc > 3) size = atoi (argv[3]);
    mw_memleak_ck (atoi(argv[1]), time, size);
    return 0;
}

