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

static long long int _get_ms2 (void)
{
    struct timespec tp = {0, 0};
    clock_gettime (CLOCK_MONOTONIC, &tp);
    /*tp.tv_sec &= 0x3FFFFF;*/
    return tp.tv_sec * 1000 + tp.tv_nsec / (1000*1000);
}

int main (int argc, char** argv)
{
    int fd = -1;

    long long int fsize = 0;
    long long int rtt = 0;
    long long int ts = 0, te = 0;

    if (argc < 2)
    {
        fprintf (stderr, "Usage: %s path\n", argv[0]);
        return -1;
    }

    _smbc_init_np (NULL);

    /*_smbc_reset_ctx_np (NULL);*/

    ts = _get_ms2 ();
    fd = _smbc_open (argv[1], O_RDONLY, 0400);
    if (fd < 0) {
        fprintf (stderr, "open %s error, %d, %s\n", argv[1], errno, strerror(errno));
        _smbc_deinit_np();
        return -1;
    }

    /* stat */
    {
        struct stat buf;
        int rc = _smbc_stat (argv[1], &buf);
        if (rc == 0) {
            fprintf (stderr, "%s %12lld %s\n", S_ISDIR (buf.st_mode) ? "d" : "-", buf.st_size, argv[1]);
            fsize = buf.st_size;
        }
    }

    /* read */
    {
        char s[65534];
        int xx = 0;
        int nc = 0;
        long long last_print_time = 0;
        long long cur_time = 0;
        int last_down_percent = 0;
        int down_percent = 0;

        while (1) {
            int nb = 0, i = 0;
            int rlen = _smbc_read (fd, s, sizeof(s));

            ++xx;
            for (i = 0; i < nc; i++) putchar ('\b');

            if (rlen > 0) {
                rtt += rlen;
            } else {
                if (rlen < 0) fprintf (stderr, "read error %d, %s\n", errno, strerror(errno));
                nb = 1;
            }

            down_percent = rtt * 100 / fsize;
            cur_time = _get_ms2();

            /*if (nb || (xx & 0x40) == 0) {*/
            if (nb || (cur_time - last_print_time) >= 1000 || down_percent != last_down_percent) {
                char pr[1024] = "";
                if (fsize > 0) {
                    sprintf (pr, "%3d%% Down: %lld / %lld", down_percent, rtt, fsize);
                } else {
                    sprintf (pr, "Down: %lld", rtt);
                }
                nc = strlen (pr);
                fprintf (stderr, "%s", pr);
                last_print_time = cur_time;
                last_down_percent = down_percent;
            }
            fflush (stdout);
            if (nb) {
                putchar ('\n');
                break;
            }
        }
    }

    _smbc_close (fd);

    te = _get_ms2 ();
    te -= ts;

    fprintf (stderr, "Total Down: %lld, spend: %lld.%03lld sec., speed: %lld bytes\n", rtt, te/1000, te%1000, rtt * 1000 / te);

    _smbc_deinit_np();
    return 0;
}

