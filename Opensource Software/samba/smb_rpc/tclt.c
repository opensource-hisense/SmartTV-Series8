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

int main (int argc, char** argv)
{
    int dfd = 0;
    struct _smbc_dirent *dir = NULL;
    /*char path[256] = "/cygdrive/d/project/smbmw/smb_mw/smb/smb_rpc/";*/
    /*char path[2048] = "/cygdrive/d/project/";*/
    char file[4096] = "";
    char* path = NULL;

    if (argc < 2)
    {
        fprintf (stderr, "Usage: %s path\n", argv[0]);
        return -1;
    }
    path = argv[1];

    _smbc_init_np (NULL);

    /*_smbc_reset_ctx_np (NULL);*/

    dfd = _smbc_opendir (path);
    if (dfd == 0)
    {
        printf ("errno: %d, %s\n", errno, strerror (errno));
        _smbc_deinit_np();
        return 0;
    }

    while ((dir = _smbc_readdir (dfd)) != NULL) {
        /*int fd = -1;*/
        char s[2] = "/";
        if (strcmp (dir->name, ".") == 0 || strcmp (dir->name, "..") == 0) continue;
        if (path[strlen(path)-1] == '/') s[0] = 0;
        sprintf (file, "%s%s%s", path, s, dir->name);
        /*fd = _smbc_open (file, 0, 0);*/
#if 0
        if (fd >= 0)
        {
            char buf[4096] = "";
            int len = _smbc_read (fd, buf, 64);
            printf ("read %d 64 rc %d\n", fd, len);
            _prn_mem (buf, 32);
            _smbc_lseek (fd, 1, 0);
            len = _smbc_read (fd, buf, 64);
            printf ("read %d 64 rc %d\n", fd, len);
            _prn_mem (buf, 32);
        }
#endif
        {
            struct stat buf;
            int rc = _smbc_stat (file, &buf);
            if (rc == 0) printf ("%s %12lld %d %s\n", S_ISDIR (buf.st_mode) ? "d" : "-", buf.st_size, dir->smbc_type, file);
        }
        /*if (fd >= 0) _smbc_close (fd);*/
    }
    _smbc_closedir (dfd);
    _smbc_deinit_np();
    return 0;
}

