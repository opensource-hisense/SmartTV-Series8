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

#if 0
#include <linux/types.h>
#include <linux/netlink.h>
#endif

#include <pthread.h>
#include <arpa/inet.h>

#include <sys/mman.h>

#include "mylist.h"
#include "mw_common.h"

#include "smb_common.h"
#include "smb_rpc.h"

/***************************************************/

int _smbc_opendir (const char*);
int _smbc_closedir (int);
void* _smbc_readdir (int);

int _smbc_open (const char*, int, mode_t);
int _smbc_close (int);
ssize_t _smbc_read (int, void*, size_t);
off_t _smbc_lseek (int, off_t, int);
int _smbc_stat (const char*, struct stat*);

/***************************************************/

static void* _srv_smbc_perm_np (void*);
/*typedef void (*smbc_get_auth_data_fn) (const char* srv, const char* shr, char* wg, int wglen, char* un, int unlen, char* pw, int pwlen);*/

static int fd_g = -1;

static smbc_get_auth_data_fn _for_perm = NULL;
static void* _srv_smbc_perm_np (void* buf)
{
    char srv[256] = "";
    char shr[256] = "";
    char wg[256] = "";
    char un[256] = "";
    char pw[256] = "";
    int rlen = 255;

    if (_for_perm == NULL) return mws_return (-1, buf);

    mw_get (buf, 0, MW_FIELD_SMB_SRV, (char*) srv, &rlen);
    rlen = 255;
    mw_get (buf, 0, MW_FIELD_SMB_SHR, (char*) shr, &rlen);

    (*_for_perm) (srv, shr, wg, 255, un, 255, pw, 255);

    mw_append (buf, MW_FIELD_SMB_WG, (char*) wg, strlen (wg) + 1);
    mw_append (buf, MW_FIELD_SMB_UN, (char*) un, strlen (un) + 1);
    mw_append (buf, MW_FIELD_SMB_PW, (char*) pw, strlen (pw) + 1);

    return mws_return (0, buf);
}

static int _get_errno (void* buf)
{
    int no = 0;
    if (buf == NULL) return 0;
    if (mw_get (buf, 0, MW_FIELD_SMB_ERRNO, (char*) &no, 0) != 0) return 0;
    return no;
}

static void _set_fd (void* buf, int fd)
{
    if (buf) mw_append (buf, MW_FIELD_SMB_FD, (char*)&fd, 0);
}

static int _get_dfd (void* buf)
{
    int fd = 0;
    if (buf == NULL) return 0;
    if (mw_get (buf, 0, MW_FIELD_SMB_FD, (char*) &fd, 0) != 0) return 0;
    return fd;
}

static int _get_fd (void* buf)
{
    int fd = -1;
    if (buf == NULL) return -1;
    if (mw_get (buf, 0, MW_FIELD_SMB_FD, (char*) &fd, 0) != 0) return -1;
    return fd;
}

/* stuff function only */
/* Why? the first service may reg fail in some cases */
static void* _srv_stuff0 (void* buf)
{
    return NULL;
}

#include <semaphore.h>

static int sem = 0;

extern void _prn_clients (void);
extern void _prn_services (void);
static void* _srv_status (void* buf)
{
    _prn_services ();
    fprintf (stderr, "\n");
    _prn_clients ();
    fprintf (stderr, "\n");
    /* reading status */
    /*fprintf (stderr, ">> Current reading status: %d, %s\n", reading_status, _get_reading_status_string (reading_status));*/

    {
        int sem_value = 0;
        sem_getvalue((sem_t*)sem, &sem_value);
        fprintf (stderr, ">> Current sem status: 0x%08x, value: %d\n\n", sem, sem_value);
    }
    return mws_return (0, buf);
}

static char shm_name[128] = "";
static void* data_sm = NULL;
int _smbc_init_np (smbc_get_auth_data_fn _perm)
{
    int i = 0;
    void* buf = NULL;
    int fd = -1;

    if (_for_perm != NULL)
    {
        PRINTF ("already inited\n");
        return 0;
    }

    buf = mw_alloc (MW_BUF_SIZE_SMALL);
    if (buf == NULL)
    {
        PRINTF ("out of memory\n");
        return -1;
    }

    if (_perm) {
        _for_perm = _perm;
    }

    unlink (SMBC_CLIENT_PW);

    fd = mws_open (SMBC_CLIENT_PW);
    /*PRINTF ("tv-end smb server inited, %d\n", fd);*/
    mws_reg_service (fd, "_test_stuff0", _srv_stuff0, NULL);
    mws_reg_service (fd, "_status", _srv_status, NULL);
    if (mws_reg_service (fd, "_smbc_perm_np", _srv_smbc_perm_np, NULL) != 0)
    {
        PRINTF ("+++++++++ reg '_smbc_perm_np` error, fd: %d\n", fd);
    }

    for (i = 0; i < 10; i++)
    {
        if (system ("if [ -e /tmp/mnp_smbc_rpc ]; then exit 0; else exit 1; fi") == 0) break;
        usleep (300000);
    }

    if (fd_g == -1) fd_g = mwc_open (SMBC_CLIENT);

    if (shm_name[0] == 0) {
        sprintf(shm_name, "%s_%d_%d", SMBC_CLIENT_SM, (int)getpid(), fd_g);
    }

    /* init share mem */
    if (data_sm == NULL)
    {
        int fd_sm = -1;
        fd_sm = open (shm_name, O_CREAT|O_RDWR|O_TRUNC, 00777);
        if (fd_sm >= 0)
        {
            data_sm = (void*) mmap(NULL, MW_BUF_SIZE_BIG, PROT_READ|PROT_WRITE, MAP_SHARED, fd_sm, 0);
            if (data_sm == (void*) -1)
            {
                PRINTF ("===== ERROR: read buffer allocate failure, %d, %s =====\n", errno, strerror(errno));
                data_sm = NULL;
            }
            else
            {
                if (ftruncate (fd_sm ,MW_BUF_SIZE_BIG)) {
                    /* dummy for compile warning */
                }
                sleep (1);
            }
            close (fd_sm);
        }
        else
        {
            PRINTF ("===== ERROR: read buffer file open failure, %d, %s =====\n", errno, strerror(errno));
        }

        /* semaphore */
        if (sem == 0) {
            sem = mw_sem_open(SMBC_CLIENT_SM);
        }
    }

    mw_append (buf, MW_FIELD_SMB_SHM_NAME, (char*) shm_name, strlen (shm_name) + 1);
    mwc_call (fd_g, "_smbc_init_np", buf, MW_BUF_SIZE_SMALL, (char**) NULL, NULL);

    mw_free(buf);

    return 0;
}

int _smbc_deinit_np (void)
{
    void* buf = NULL;

    if (fd_g < 0) return -1;

    PRINTF("call deinit\n");
    buf = mw_alloc (MW_BUF_SIZE_SMALL);
    if (buf == NULL)
    {
        PRINTF ("out of memory\n");
        return -1;
    }

    mw_append (buf, MW_FIELD_SMB_SHM_NAME, (char*) shm_name, strlen (shm_name) + 1);
    mwc_call (fd_g, "_smbc_deinit_np", buf, MW_BUF_SIZE_SMALL, (char**) NULL, NULL);

    PRINTF("free call buf\n");
    mw_free(buf);

    PRINTF("close services\n");
    mwc_close(fd_g);
    fd_g = -1;

    mws_close(SMBC_CLIENT_PW);

    if (data_sm) {
        munmap(data_sm, MW_BUF_SIZE_BIG);
        data_sm = NULL;
    }

    mw_sem_close(sem);
    sem = 0;

    PRINTF("end ...\n");
    return 0;
}

int _smbc_reset_ctx_np (smbc_get_auth_data_fn _perm)
{
    if (fd_g < 0) return -1;
    _for_perm = _perm;
    return mwc_call (fd_g, "_smbc_reset_ctx_np", NULL, 0, NULL, NULL);
}

int _smbc_stat (const char* file, struct stat* stat_buf)
{
    void* buf = mw_alloc (MW_BUF_SIZE);
    void* rbuf = NULL;
    int rlen = 0;
    int rc = -1;

    if (buf == NULL) return -1;

    mw_append (buf, MW_FIELD_SMB_PATH, file, strlen (file) + 1);

    rc = mwc_call (fd_g, "_smbc_stat", buf, MW_BUF_SIZE, (char**) &rbuf, &rlen);
    if (rc != 0)
    {
        int no = _get_errno (rbuf);
        if (no != 0) errno = no;
        mw_free (buf);
        PRINTF ("%s, rc %d, errno %d, %s\n", file, rc, no, strerror (no));
        return -1;
    }
    rlen = sizeof (*stat_buf);
    if (mw_get (rbuf, 0, MW_FIELD_SMB_STAT, (char*) stat_buf, &rlen) != 0)
    {
        mw_free (buf);
        PRINTF ("get buf failed\n");
        return -1;
    }
    mw_free (buf);
    return 0;
}

off_t _smbc_lseek (int fd, off_t offset, int whence)
{
    static void* buf = NULL;
    void* rbuf = NULL;
    int rlen = 0;
    off_t rc = -1;

    mwc_lock ();

    if (buf == NULL && (buf = mw_alloc_delay_free (MW_BUF_SIZE_SMALL)) == NULL) return -1;
    mw_memset (buf);

    _set_fd (buf, fd);
    mw_append (buf, MW_FIELD_SMB_OFFSET, (char*) &offset, 8);
    mw_append (buf, MW_FIELD_SMB_WHENCE, (char*) &whence, 0);

    rc = mwc_call (fd_g, "_smbc_lseek", buf, MW_BUF_SIZE_SMALL, (char**) &rbuf, &rlen);
    if (rc < 0)
    {
        int no = _get_errno (rbuf);
        if (no != 0) errno = no;
        mwc_unlock ();
        PRINTF ("call (%d, %lld, %d) rc %d, errno %d\n", fd, offset, whence, (int) rc, no);
        return -1;
    }
    rlen = 8;
    mw_get (rbuf, 0, MW_FIELD_SMB_LSEEK_RC, (char*) &rc, &rlen);

    mwc_unlock ();

    return rc;
}

static unsigned int read_sid = 0;
int _smbc_read (int fd, void* read_buf, size_t count)
{
    static void* buf = NULL;
    void* rbuf = NULL;
    int rlen = 0;
    int rc = -1;
    unsigned int sid = 0;
    unsigned int rsid = 0;

    if (data_sm == NULL)
    {
        PRINTF ("no memory\n");
        return -1;
    }

    mwc_lock ();

    if (buf == NULL && (buf = mw_alloc_delay_free (MW_BUF_SIZE_SMALL)) == NULL) return -1;

    if (read_buf == NULL || count <= 0)
    {
        mwc_unlock();
        PRINTF ("invalid argument\n");
        return -1;
    }
    mw_memset (buf);

    _set_fd (buf, fd);
    mw_append (buf, MW_FIELD_SMB_CNT, (char*) &count, 0);

    sid = ++read_sid;
    mw_append (buf, MW_FIELD_SMB_SESSION, (char*) &sid, 0);

    mw_append (buf, MW_FIELD_SMB_SHM_NAME, (char*) shm_name, strlen(shm_name)+1);

    rc = mwc_call (fd_g, "_smbc_read", buf, MW_BUF_SIZE_SMALL, (char**) &rbuf, &rlen);
    if (rc < 0)
    {
        int no = _get_errno (rbuf);
        if (no != 0) errno = no;
        mwc_unlock ();
        PRINTF ("call _smbc_read (%d, %d): rc %d, errno %d\n", fd, count, rc, no);
        return -1;
    }
    else if (rc == 0)
    {
        mwc_unlock ();
        return 0;
    }

    if ((rsid = *(int*) data_sm) != sid)
    {
        PRINTF ("session invalid: req %u, res %u\n", sid, rsid);
    }

    memcpy (read_buf, (void*) (data_sm + sizeof (unsigned int)), rc);
    *(int*)data_sm = 0;

    mwc_unlock ();

    if (mw_sem_unlock (sem) != 0) {
        PRINTF ("ERROR: sem_unlock %d, %s\n", errno, strerror(errno));
    }

    return rc;
}

int _smbc_open (const char* file, int flags, mode_t mode)
{
    void* buf = mw_alloc (MW_BUF_SIZE);
    void* rbuf = NULL;
    int rlen = 0;
    int rc = -1;
    int fd = -1;

    if (buf == NULL) return -1;

    mw_append (buf, MW_FIELD_SMB_PATH, file, strlen (file) + 1);

    rc = mwc_call (fd_g, "_smbc_open", buf, MW_BUF_SIZE, (char**) &rbuf, &rlen);
    if (rc != 0)
    {
        int no = _get_errno (rbuf);
        if (no != 0) errno = no;
        PRINTF ("%s, rc %d, errno %d, %s\n", file, rc, no, strerror(no));
        mw_free (buf);
        return -1;
    }
    fd = _get_fd (rbuf);
    mw_free (buf);
    return fd;
}

int _smbc_close (int fd)
{
    void* buf = mw_alloc (MW_BUF_SIZE_SMALL);
    void* rbuf = NULL;
    int rlen = 0;
    int rc = -1;

    if (buf == NULL) return -1;

    _set_fd (buf, fd);

    rc = mwc_call (fd_g, "_smbc_close", buf, MW_BUF_SIZE_SMALL, (char**) &rbuf, &rlen);
    if (rc != 0)
    {
        int no = _get_errno (rbuf);
        if (no != 0) errno = no;
        PRINTF ("fd %d, rc %d, errno %d, %s\n", fd, rc, no, strerror(no));
        mw_free (buf);
        return rc;
    }
    mw_free (buf);
    return 0;
}

off_t _smbc_lseekdir (int fd, off_t offset)
{
    void* buf = mw_alloc (MW_BUF_SIZE_SMALL);
    void* rbuf = NULL;
    int rlen = 0;
    off_t rc = -1;

    if (buf == NULL) return -1;

    _set_fd (buf, fd);
    mw_append (buf, MW_FIELD_SMB_OFFSET, (char*) &offset, 8);

    rc = mwc_call (fd_g, "_smbc_lseekdir", buf, MW_BUF_SIZE_SMALL, (char**) &rbuf, &rlen);
    if (rc < 0)
    {
        int no = _get_errno (rbuf);
        if (no != 0) errno = no;
        PRINTF ("fd %d, rc %d, errno %d, %s\n", fd, (int) rc, no, strerror(no));
        mw_free (buf);
        return -1;
    }
    mw_free (buf);
    return rc;
}

int _smbc_opendir (const char* path)
{
    void* buf = mw_alloc (MW_BUF_SIZE);
    void* rbuf = NULL;
    int rlen = 0;
    int rc = -1;
    int fd = -1;

    if (buf == NULL) return -1;

    mw_append (buf, MW_FIELD_SMB_PATH, path, strlen (path) + 1);

    rc = mwc_call (fd_g, "_smbc_opendir", buf, MW_BUF_SIZE, (char**) &rbuf, &rlen);
    if (rc != 0)
    {
        int no = _get_errno (rbuf);
        if (no != 0) errno = no;
        mw_free (buf);
        PRINTF ("path %s, rc %d, errno %d, %s\n", path, rc, no, strerror(no));
        return -1;
    }
    fd = _get_dfd (rbuf);
    mw_free (buf);
    return fd;
}

void* _smbc_readdir (int fd)
{
    void* buf = mw_alloc (MW_BUF_SIZE);
    void* rbuf = NULL;
    int rlen = 0;
    int rc = -1;
#ifdef USE_NATIVE
    static struct dirent *smbc_dirent = NULL;
#else
    static struct _smbc_dirent *smbc_dirent = NULL;
#endif
    int dir_size = sizeof (*smbc_dirent) + 4096;

    if (smbc_dirent == NULL) smbc_dirent = malloc (dir_size);
    if (smbc_dirent == NULL || buf == NULL) return NULL;
    memset (smbc_dirent, 0, dir_size);

    _set_fd (buf, fd);

    rc = mwc_call (fd_g, "_smbc_readdir", buf, MW_BUF_SIZE, (char**) &rbuf, &rlen);
    if (rc != 0)
    {
        int no = _get_errno (rbuf);
        if (no != 0) errno = no;
        mw_free (buf);
        PRINTF ("fd %d, rc %d, errno %d, %s\n", fd, rc, no, strerror(no));
        return NULL;
    }
    rc = mw_get (rbuf, 0, MW_FIELD_SMB_DIRENT, (char*) smbc_dirent, &dir_size);
    mw_free (buf);

    if (rc != 0) return NULL;
    return smbc_dirent;
}

int _smbc_closedir (int fd)
{
    void* buf = mw_alloc (MW_BUF_SIZE_SMALL);
    void* rbuf = NULL;
    int rlen = 0;
    int rc = -1;

    if (buf == NULL) return -1;

    _set_fd (buf, fd);

    rc = mwc_call (fd_g, "_smbc_closedir", buf, MW_BUF_SIZE_SMALL, (char**) &rbuf, &rlen);
    if (rc != 0)
    {
        int no = _get_errno (rbuf);
        if (no != 0) errno = no;
        mw_free (buf);
        PRINTF ("fd %d, rc %d, errno %d, %s\n", fd, rc, no, strerror(no));
        return rc;
    }
    mw_free (buf);
    return 0;
}

void _status (void)
{
    int fd = mwc_open (SMBC_CLIENT_CMD);
    if (fd < 0) {
        PRINTF ("cmd client open error\n");
        return;
    }
    mwc_call (fd, "_status", NULL, 0, NULL, NULL);
    mwc_close (fd);

    fd = mwc_open (SMBC_CLIENT_PW);
    if (fd < 0) {
        PRINTF ("pw client open error\n");
        return;
    }
    mwc_call (fd, "_status", NULL, 0, NULL, NULL);
    mwc_close (fd);
}

