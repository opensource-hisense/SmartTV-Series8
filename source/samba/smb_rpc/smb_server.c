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

#include <sys/mman.h>

#include "mylist.h"
#include "mw_common.h"

#ifndef NET_SMB_MODULE_NAME
#define NET_SMB_MODULE_NAME "smb_srv"
#endif /* NET_SMB_MODULE_NAME */
#include "smb_common.h"

/***************************************************/

/* NET_SMB_IF_EXT for after samba-3.2 (ex. samba-3.5.6) */
#ifdef NET_SMB_IF_EXT
#undef NET_SMB_IF_EXT
#endif

void* (*_smbc_sc)(void * context) = NULL;
void* (*_smbc_nc) (void) = NULL;
void* (*_smbc_ic) (void*) = NULL;
int (*_smbc_fc) (void*, int) = NULL;
#ifdef NET_SMB_IF_EXT
int (*_smbc_sa) (void*, smbc_get_auth_data_fn) = NULL;
void (*_smbc_st)(void*, int) = NULL;
#else
void (*_smbc_option_set)(void*, char*, ...) = NULL;
#endif

int (*_smbc_init)(smbc_get_auth_data_fn, int) = NULL;

int (*_smbc_open)(const char*, int, mode_t) = NULL;
ssize_t (*_smbc_read)(int, void*, size_t) = NULL;
off_t (*_smbc_lseek)(int, off_t, int) = NULL;
int (*_smbc_close)(int) = NULL;
int (*_smbc_stat)(const char*, struct stat*) = NULL;

int (*_smbc_opendir)(const char*) = NULL;
int (*_smbc_closedir)(int) = NULL;
void* (*_smbc_readdir)(unsigned int) = NULL;
int (*_smbc_lseekdir)(int, off_t) = NULL;

/***************************************************/

static void* _srv_smbc_init_np (void* buf);
static void* _srv_smbc_deinit_np (void* buf);

static void* _srv_smbc_reset_ctx_np (void* buf);

static void* _srv_smbc_opendir (void* buf);
static void* _srv_smbc_readdir (void* buf);
static void* _srv_smbc_closedir (void* buf);
static void* _srv_smbc_lseekdir (void* buf);
static void* _srv_smbc_open (void* buf);
static void* _srv_smbc_close (void* buf);
static void* _srv_smbc_read (void* buf);
static void* _srv_smbc_lseek (void* buf);
static void* _srv_smbc_stat (void* buf);
static void* _srv_status (void* buf);

static int fd_g = -1;
static void _for_smbc_perm (const char* srv, const char* shr, char* wg, int wglen, char* un, int unlen, char* pw, int pwlen)
{
    if (wg) wg[0] = 0;
    pw[0] = 0;
    strcpy (un, "guest");

    if (fd_g >= 0)
    {
        static void* buf = NULL;
        void* rbuf = NULL;
        int rlen = 0;

        mwc_lock();

        if (buf == NULL && (buf = mw_alloc_delay_free (MW_BUF_SIZE_SMALL)) == NULL) return;
        mw_memset (buf);

        mw_append (buf, MW_FIELD_SMB_SRV, srv, strlen (srv) + 1);
        mw_append (buf, MW_FIELD_SMB_SHR, shr, strlen (shr) + 1);

        if (mwc_call (fd_g, "_smbc_perm_np", buf, MW_BUF_SIZE_SMALL, (char**) &rbuf, &rlen) == 0)
        {
            mw_get (rbuf, 0, MW_FIELD_SMB_WG, (char*) wg, &wglen);
            mw_get (rbuf, 0, MW_FIELD_SMB_UN, (char*) un, &unlen);
            mw_get (rbuf, 0, MW_FIELD_SMB_PW, (char*) pw, &pwlen);
        }

        mwc_unlock();
    }
}

static void _for_smbc_perm_ctx(void* context, const char* srv, const char* shr, char* wg, int wglen, char* un, int unlen, char* pw, int pwlen)
{
    return _for_smbc_perm (srv, shr, wg, wglen, un, unlen, pw, pwlen);
}

/* shm list */
struct _SHM_MAP {
    char* shm_name;
    void* shm_mem;
};

static handle_t h_list_shm = null_handle;

static int _init_shm_list (void)
{
    return my_list_create (&h_list_shm);
}

static int _shm_map_cmp_name (const void* pv1, const void* pv2)
{
    struct _SHM_MAP* _map_list = (struct _SHM_MAP*) pv1;
    struct _SHM_MAP* _map_tag = (struct _SHM_MAP*) pv2;

    if (_map_list == NULL || _map_tag == NULL) {
        return -1;
    }

    if (_map_list->shm_name == NULL || _map_tag->shm_name == NULL) {
        return -1;
    }

    if (strcmp(_map_list->shm_name, _map_tag->shm_name) != 0) {
        return -1;
    }

    return 0;
}

static int _add_shm_list (const char* shm_name, void* shm_mem)
{
    struct _SHM_MAP tmap = {(char*)shm_name, shm_mem};
    struct _SHM_MAP* _map = NULL;

    if (shm_name == NULL || shm_mem == NULL) {
        return -1;
    }

    if ((_map = (struct _SHM_MAP*) my_list_search2 (h_list_shm, &tmap, _shm_map_cmp_name)) != NULL) {
        if (_map->shm_mem != shm_mem) {
            _map->shm_mem = shm_mem;
        }
        return 0;
    }

    _map = malloc (sizeof(*_map));
    if (_map == NULL) {
        return -1;
    }

    _map->shm_mem = shm_mem;
    _map->shm_name = strdup (shm_name);
    if (_map->shm_name == NULL) {
        free (_map);
        return -1;
    }

    if (my_list_add_tail (h_list_shm, _map) != 0) {
        free (_map);
        return -1;
    }

    return 0;
}

static struct _SHM_MAP* _get_shm_list (const char* shm_name, void* shm_mem)
{
    struct _SHM_MAP tmap = {(char*)shm_name, shm_mem};

    if (shm_name == NULL) {
        return NULL;
    }

    return (struct _SHM_MAP*) my_list_search2 (h_list_shm, &tmap, _shm_map_cmp_name);
}

static void* _get_shm_from_list (const char* shm_name)
{
    struct _SHM_MAP* _map = _get_shm_list(shm_name, NULL);

    if (_map == NULL) return NULL;

    return _map->shm_mem;
}

static int _shm_map_cmp (const void* pv1, const void* pv2)
{
    struct _SHM_MAP* _map_list = (struct _SHM_MAP*) pv1;
    struct _SHM_MAP* _map_tag = (struct _SHM_MAP*) pv2;

    if (_map_list == NULL || _map_tag == NULL) {
        return -1;
    }

    if (_map_list->shm_name == NULL || _map_tag->shm_name == NULL) {
        return -1;
    }

    if (strcmp(_map_list->shm_name, _map_tag->shm_name) != 0 &&
        _map_list->shm_mem != _map_tag->shm_mem) {
        return -1;
    }

    return 0;
}

static int _del_shm_list (const char* shm_name, void* shm_mem)
{
    struct _SHM_MAP tmap = {(char*)shm_name, shm_mem};
    struct _SHM_MAP* _map = (struct _SHM_MAP*) my_list_delete2(h_list_shm, &tmap, _shm_map_cmp);

    if (_map == NULL) return -1;

    _map->shm_mem = NULL;

    free (_map->shm_name);
    free (_map);

    return 0;
}

/* stuff function only */
/* Why? the first service may reg fail in some cases */
static void* _srv_stuff0 (void* buf)
{
    return NULL;
}

static int _start_smbc_srv (void)
{
    int fd = -1;

    unlink (SMBC_CLIENT);

    fd = mws_open (SMBC_CLIENT);
    PRINTF ("smb server inited, %d\n", fd);

    mws_reg_service (fd, "_test_stuff0", _srv_stuff0, NULL);
    mws_reg_service (fd, "_smbc_opendir", _srv_smbc_opendir, NULL);
    mws_reg_service (fd, "_smbc_readdir", _srv_smbc_readdir, NULL);
    mws_reg_service (fd, "_smbc_closedir", _srv_smbc_closedir, NULL);
    mws_reg_service (fd, "_smbc_lseekdir", _srv_smbc_lseekdir, NULL);
    mws_reg_service (fd, "_smbc_open", _srv_smbc_open, NULL);
    mws_reg_service (fd, "_smbc_close", _srv_smbc_close, NULL);
    mws_reg_service (fd, "_smbc_read", _srv_smbc_read, NULL);
    mws_reg_service (fd, "_smbc_lseek", _srv_smbc_lseek, NULL);
    mws_reg_service (fd, "_smbc_stat", _srv_smbc_stat, NULL);
    mws_reg_service (fd, "_smbc_reset_ctx_np", _srv_smbc_reset_ctx_np, NULL);
    mws_reg_service (fd, "_smbc_init_np", _srv_smbc_init_np, NULL);
    mws_reg_service (fd, "_smbc_deinit_np", _srv_smbc_deinit_np, NULL);

    _init_shm_list();

    return 0;
}

static int _start_smbc_cmd (void)
{
    int fd = -1;

    unlink (SMBC_CLIENT_CMD);

    fd = mws_open (SMBC_CLIENT_CMD);
    PRINTF ("smb cmd/test server inited, %d\n", fd);

    mws_reg_service (fd, "_test_stuff0", _srv_stuff0, NULL);
    mws_reg_service (fd, "_status", _srv_status, NULL);

    return 0;
}

/*
 * errno
 * ENOMEM 12
 * EINVAL 22
 */
static void _set_errno (void* buf, int no)
{
    if (buf) mw_append (buf, MW_FIELD_SMB_ERRNO, (char*)&no, 0);
}

static int _get_shm_name (void* buf, char* shm_name)
{
    int rlen = 128;
    if (buf == NULL || shm_name == NULL) return -1;
    if (mw_get(buf, 0, MW_FIELD_SMB_SHM_NAME, (char*) shm_name, &rlen) != 0) return -1;
    return 0;
}

static void* _srv_smbc_init_np (void* buf)
{
    if (fd_g < 0)
    {
        char* client = NULL;
        int rlen = SMB_PATH_SIZE;

        if ((client = malloc (SMB_PATH_SIZE)) == NULL)
        {
            _set_errno (buf, ENOMEM);
            PRINTF ("out of memory\n");
            return mws_return (-1, buf);
        }
        memset (client, 0, SMB_PATH_SIZE);

        if (mw_get (buf, 0, MW_FIELD_SMB_PATH, (char*) client, &rlen) != 0)
        {
            /*free (client);
            client = NULL;
            _set_errno (buf, EINVAL);
            PRINTF ("get client client failed\n");
            return mws_return (-1, buf);*/
            strcpy (client, SMBC_CLIENT_PW);
        }

        fd_g = mwc_open (client);
        free(client);
    }

    {
        char shm_name[128] = "";
        void* shm_mem = NULL;

        if (_get_shm_name(buf, shm_name) != 0)
        {
            PRINTF ("ERROR: client have no set share memory id\n");
            return 0;
        }

        /* init share mem */
        shm_mem = _get_shm_from_list (shm_name);
        if (shm_mem == NULL)
        {
            int fd_sm = -1;
            fd_sm = open (shm_name, O_CREAT|O_RDWR|O_TRUNC, 0777);
            if (fd_sm >= 0)
            {
                shm_mem = (void*) mmap(NULL, MW_BUF_SIZE_BIG, PROT_READ|PROT_WRITE, MAP_SHARED, fd_sm, 0);
                if (shm_mem == (void*) -1)
                {
                    PRINTF ("===== ERROR: read buffer allocate failure, %d, %s =====\n", errno, strerror(errno));
                    shm_mem = NULL;
                }
                else
                {
                    if (ftruncate (fd_sm ,MW_BUF_SIZE_BIG)) {
                        /* dummy for compile warning */
                    }
                }
                close (fd_sm);
            }
            else
            {
                PRINTF ("===== ERROR: read buffer file open failure, %d, %s =====\n", errno, strerror(errno));
            }
            if (shm_mem) {
                _add_shm_list(shm_name, shm_mem);
            }
        }
    }

    return mws_return (0, NULL);
}

static void* _srv_smbc_deinit_np (void* buf)
{
    char shm_name[128] = "";
    void* shm_mem = NULL;

    if (_get_shm_name(buf, shm_name) != 0)
    {
        PRINTF ("ERROR: client have no set share memory id\n");
        return 0;
    }

    /* delete share mem */
    _add_shm_list(shm_name, shm_mem);
    shm_mem = _get_shm_from_list(shm_name);
    if (shm_mem) {
        munmap(shm_mem, MW_BUF_SIZE_BIG);
        /* unlink file */
        unlink(shm_name);
        _del_shm_list(shm_name, shm_mem);
        shm_mem = NULL;
    }
    if (fd_g >= 0) mwc_close(fd_g);
    fd_g = -1;
    return mws_return (0, NULL);
}

#ifndef NET_SMB_IF_EXT
int _smbc_sa(void* context, smbc_get_auth_data_fn_ctx fn_ctx, smbc_get_auth_data_fn fn)
{
    if (context == NULL) return 0;

    if (_smbc_option_set) {
        _smbc_option_set(context, "auth_function", (void*)fn_ctx);
    } else {
        struct _smbc_context *pc = (struct _smbc_context*)context;
        pc->smbc_cb.auth_fn = fn;
    }

    return 0;
}

void _smbc_st(void* context, int ms)
{
    struct _smbc_context *pc = (struct _smbc_context*)context;

    if (context == NULL) return;

    pc->timeout = ms;

    return;
}
#endif

static void* _srv_smbc_reset_ctx_np (void* buf)
{
#ifndef USE_NATIVE
    void* nc = NULL;
    void* oc = NULL;

    if ((nc = (*_smbc_nc) ()) == NULL)
    {
        PRINTF ("new context error\n");
        return mws_return (-1, NULL);
    }

#ifndef NET_SMB_IF_EXT
    _smbc_sa (nc, _for_smbc_perm_ctx, _for_smbc_perm);
#endif
    if ((*_smbc_ic) (nc) == NULL)
    {
        PRINTF ("init new context 0x%08x error, error %d, %s\n", (int)nc, errno, strerror(errno));
        (*_smbc_fc) (nc, 0);
        return mws_return (-1, NULL);
    }
    _smbc_st(nc, NET_SMB_SMBC_TIMEO);

    if ((oc = (*_smbc_sc) (nc)) != NULL)
    {
        PRINTF ("set new context, free old context\n");
        (*_smbc_fc) (oc, 1);
    }

#ifdef NET_SMB_IF_EXT
    _smbc_sa (nc, _for_smbc_perm);
#else
    _smbc_sa (nc, _for_smbc_perm_ctx, _for_smbc_perm);
#endif
#endif

    return mws_return (0, NULL);
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

/* for test BGN */
enum READING_STATUS {
    READING_STATUS_PENDING = 0,
    READING_STATUS_CLOSED,
    READING_STATUS_PREPARE,
    READING_STATUS_SEM_WAIT,
    READING_STATUS_SEM_HOLD,
    READING_STATUS_READING,
    READING_STATUS_RETURN
};
static int reading_status = READING_STATUS_PENDING;
static char* _get_reading_status_string (enum READING_STATUS status)
{
    switch (status) {
    case READING_STATUS_PENDING:  return "PEDING";
    case READING_STATUS_CLOSED:   return "CLOSED";
    case READING_STATUS_PREPARE:  return "PREPARE";
    case READING_STATUS_SEM_WAIT: return "SEM_WAIT";
    case READING_STATUS_SEM_HOLD: return "SEM_HOLD";
    case READING_STATUS_READING:  return "READING";
    case READING_STATUS_RETURN:   return "RETURN";
    }
    return "Unknown";
}
#include <semaphore.h>
/* for test END */

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
    fprintf (stderr, ">> Current reading status: %d, %s\n", reading_status, _get_reading_status_string (reading_status));

    {
        int sem_value = 0;
        sem_getvalue((sem_t*)sem, &sem_value);
        fprintf (stderr, ">> Current sem status: 0x%08x, value: %d\n\n", sem, sem_value);
    }
    return mws_return (0, buf);
}

static void* _srv_smbc_stat (void* buf)
{
    char* path = NULL;
    int rlen = SMB_PATH_SIZE;
    int rc = -1;
    volatile char nothing[32] = "";
    struct stat stat_buf;

    nothing[0] = 0;

    if (buf == NULL || _smbc_stat == NULL)
    {
        _set_errno (buf, (int) EINVAL);
        PRINTF ("error buf: 0x%08x, _smbc_stat: 0x%08x\n", (int) buf, (int) _smbc_open);
        return mws_return (-1, buf);
    }

    if ((path = malloc (SMB_PATH_SIZE)) == NULL)
    {
        _set_errno (buf, ENOMEM);
        PRINTF ("out of memory\n");
        return mws_return (-1, buf);
    }
    memset (path, 0, SMB_PATH_SIZE);

    if ((mw_get (buf, 0, MW_FIELD_SMB_PATH, (char*) path, &rlen)) != 0)
    {
        free (path);
        path = NULL;
        _set_errno (buf, EINVAL);
        PRINTF ("get path failed\n");
        return mws_return (-1, buf);
    }

    rc = (*_smbc_stat) (path, &stat_buf);
    if (rc < 0)
    {
        _set_errno (buf, errno);
        PRINTF ("get %s stat failed, %d, %s\n", path, errno, strerror (errno));
        free (path);
        path = NULL;
        return mws_return (-1, buf);
    }
    free (path);
    path = NULL;

    mw_append (buf, MW_FIELD_SMB_STAT, (char*) &stat_buf, sizeof(stat_buf) + 32);

    return mws_return (0, buf);
}

static void* _srv_smbc_lseek (void* buf)
{
    int fd = -1;
    off_t offset = 0;
    int whence = 0;
    long long int rc = 0;
    int offlen = 8;

    if (buf == NULL || _smbc_lseek == NULL)
    {
        _set_errno (buf, EINVAL);
        PRINTF ("error buf: 0x%08x, fun: 0x%08x\n", (int) buf, (int) _smbc_close);
        return mws_return (-1, buf);
    }

    fd = _get_fd (buf);
    if (fd < 0 ||
        mw_get (buf, 0, MW_FIELD_SMB_OFFSET, (char*) &offset, &offlen) != 0 ||
        mw_get (buf, 0, MW_FIELD_SMB_WHENCE, (char*) &whence, 0) != 0)
    {
        _set_errno (buf, (int) EINVAL);
        PRINTF ("get fd or offset or whence failed\n");
        return mws_return (-1, buf);
    }

    if ((rc = _smbc_lseek (fd, offset, whence)) < 0)
    {
        _set_errno (buf, (int) errno);
        PRINTF ("lseek (%d, %lld, %d) errno %d, %s\n", fd, offset, whence, errno, strerror (errno));
        return mws_return (-1, buf);
    }

    mw_append (buf, MW_FIELD_SMB_LSEEK_RC, (char*) &rc, 8);

    return mws_return (0, buf);
}

static void* _srv_smbc_read (void* buf)
{
    int fd = 0;
    int count = 0;
    ssize_t rlen = 0;
    unsigned int sid = -1;

    char shm_name[128] = "";
    void* shm_mem = NULL;

    reading_status = READING_STATUS_PENDING;
    if (buf == NULL || _smbc_read == NULL)
    {
        _set_errno (buf, EINVAL);
        PRINTF ("error buf: 0x%08x, fun: 0x%08x\n", (int) buf, (int) _smbc_close);
        return mws_return (-1, buf);
    }

    reading_status = READING_STATUS_PREPARE;
    if (_get_shm_name(buf, shm_name) != 0)
    {
        _set_errno (buf, EINVAL);
        PRINTF ("ERROR: client have no set share memory id\n");
        reading_status = READING_STATUS_CLOSED;
        return mws_return (-1, buf);
    }

    shm_mem = _get_shm_from_list(shm_name);
    if (shm_mem == NULL) {
        _set_errno (buf, EINVAL);
        PRINTF ("ERROR: cannot found a share memory\n");
        reading_status = READING_STATUS_CLOSED;
        return mws_return (-1, buf);
    }

    fd = _get_fd (buf);
    if (fd < 0)
    {
        _set_errno (buf, (int) EINVAL);
        PRINTF ("get fd failed\n");
        reading_status = READING_STATUS_CLOSED;
        return mws_return (-1, buf);
    }

    if (mw_get (buf, 0, MW_FIELD_SMB_CNT, (char*) &count, 0) != 0 || count <= 0)
    {
        _set_errno (buf, (int) EINVAL);
        PRINTF ("get read count failed\n");
        reading_status = READING_STATUS_CLOSED;
        return mws_return (-1, buf);
    }

    if (count > MW_BUF_SIZE_BIG - MW_BUF_SIZE) count = MW_BUF_SIZE_BIG - MW_BUF_SIZE;

    if (mw_get (buf, 0, MW_FIELD_SMB_SESSION, (char*) &sid, 0) != 0)
    {
        _set_errno (buf, (int) EINVAL);
        PRINTF ("get read session failed\n");
        reading_status = READING_STATUS_CLOSED;
        return mws_return (-1, buf);
    }

    reading_status = READING_STATUS_SEM_WAIT;
    /* lock */
    mw_sem_lock (sem);
    reading_status = READING_STATUS_SEM_HOLD;
    while (*(int*)shm_mem != 0) usleep (1000);
    *(int*)shm_mem = sid;

    reading_status = READING_STATUS_READING;
    if ((rlen = _smbc_read (fd, (void*) (shm_mem + sizeof (int)), count)) < 0)
    {
        PRINTF("===== sem: 0x%08x, shm: %s, 0x%08x, sid: %u\n", sem, shm_name, (int) shm_mem, sid);
        *(int*)shm_mem = 0;
        mw_sem_unlock (sem);
        _set_errno (buf, (int) errno);
        PRINTF ("read %d rc %d, %d, %s\n", fd, rlen, errno, strerror (errno));
        reading_status = READING_STATUS_CLOSED;
        return mws_return (-1, buf);
    }
    reading_status = READING_STATUS_RETURN;

    if (rlen == 0)
    {
        *(int*)shm_mem = 0;
        mw_sem_unlock (sem);
        return mws_return (0, buf);
    }

    return mws_return (rlen, buf);
}

static void* _srv_smbc_open (void* buf)
{
    char* path = NULL;
    int rlen = SMB_PATH_SIZE;
    int fd = -1;

    if (buf == NULL || _smbc_open == NULL)
    {
        _set_errno (buf, (int) EINVAL);
        PRINTF ("error buf: 0x%08x, _smbc_open: 0x%08x\n", (int) buf, (int) _smbc_open);
        return mws_return (-1, buf);
    }

    if ((path = malloc (SMB_PATH_SIZE)) == NULL)
    {
        _set_errno (buf, ENOMEM);
        PRINTF ("out of memory\n");
        return mws_return (-1, buf);
    }
    memset (path, 0, SMB_PATH_SIZE);

    if ((mw_get (buf, 0, MW_FIELD_SMB_PATH, (char*) path, &rlen)) != 0)
    {
        free (path);
        path = NULL;
        _set_errno (buf, EINVAL);
        PRINTF ("get path failed\n");
        return mws_return (-1, buf);
    }

    fd = (*_smbc_open) (path, O_RDONLY, 0400);
    if (fd < 0)
    {
        PRINTF ("open %s failed, %d, %s\n", path, errno, strerror (errno));
        free (path);
        path = NULL;
        _set_errno (buf, errno);
        return mws_return (-1, buf);
    }
    free (path);
    path = NULL;

    _set_fd (buf, fd);

    return mws_return (0, buf);
}

static void* _srv_smbc_close (void* buf)
{
    int fd = 0;

    if (buf == NULL || _smbc_close == NULL)
    {
        _set_errno (buf, EINVAL);
        PRINTF ("error buf: 0x%08x, fun: 0x%08x\n", (int) buf, (int) _smbc_close);
        return mws_return (-1, buf);
    }

    fd = _get_fd (buf);
    if (fd < 0)
    {
        _set_errno (buf, (int) EINVAL);
        PRINTF ("get fd failed\n");
        return mws_return (-1, buf);
    }

    if (_smbc_close (fd) < 0)
    {
        _set_errno (buf, (int) errno);
        PRINTF ("close %d failed, %d, %s\n", fd, errno, strerror (errno));
        return mws_return (-1, buf);
    }

    return mws_return (0, NULL);
}

static void* _srv_smbc_lseekdir (void* buf)
{
    int fd = -1;
    off_t offset = 0;
    int rc = 0;
    int offlen = 8;

    if (buf == NULL || _smbc_lseekdir == NULL)
    {
        _set_errno (buf, EINVAL);
        PRINTF ("error buf: 0x%08x, fun: 0x%08x\n", (int) buf, (int) _smbc_lseekdir);
        return mws_return (-1, buf);
    }

    fd = _get_fd (buf);
    if (fd == 0 || mw_get (buf, 0, MW_FIELD_SMB_OFFSET, (char*) &offset, &offlen) != 0)
    {
        _set_errno (buf, (int) EINVAL);
        PRINTF ("get fd or offset failed\n");
        return mws_return (-1, buf);
    }

    if ((rc = _smbc_lseekdir (fd, offset)) < 0)
    {
        _set_errno (buf, (int) errno);
        PRINTF ("lseek %d rc %d, %d, %s\n", fd, rc, errno, strerror(errno));
        return mws_return (-1, buf);
    }

    return mws_return (rc, buf);
}

static void* _srv_smbc_opendir (void* buf)
{
    char* path = NULL;
    int rlen = SMB_PATH_SIZE;
    int fd = 0;

    if (buf == NULL || _smbc_opendir == NULL)
    {
        _set_errno (buf, (int) EINVAL);
        PRINTF ("error buf: 0x%08x, _smbc_opendir: 0x%08x\n", (int) buf, (int) _smbc_opendir);
        return mws_return (-1, buf);
    }

    if ((path = malloc (SMB_PATH_SIZE)) == NULL)
    {
        _set_errno (buf, ENOMEM);
        PRINTF ("out of memory\n");
        return mws_return (-1, buf);
    }
    memset (path, 0, SMB_PATH_SIZE);

    if ((mw_get (buf, 0, MW_FIELD_SMB_PATH, (char*) path, &rlen)) != 0)
    {
        free (path);
        path = NULL;
        _set_errno (buf, EINVAL);
        PRINTF ("get path failed\n");
        return mws_return (-1, buf);
    }

    fd = _smbc_opendir (path);
    if (fd < 0)
    {
        PRINTF ("opendir %s fail, %d, %s\n", path, errno, strerror (errno));
        free (path);
        path = NULL;
        _set_errno (buf, errno);
        return mws_return (-1, buf);
    }
    free (path);
    path = NULL;

    _set_fd (buf, fd);

    return mws_return (0, buf);
}

static void* _srv_smbc_readdir (void* buf)
{
    int fd = 0;
#ifdef USE_NATIVE
    struct dirent *smbc_dirent = NULL;
#else
    struct _smbc_dirent *smbc_dirent = NULL;
#endif

    if (buf == NULL || _smbc_readdir == NULL)
    {
        _set_errno (buf, EINVAL);
        PRINTF ("error buf: 0x%08x, fun: 0x%08x\n", (int) buf, (int) _smbc_readdir);
        return mws_return (-1, buf);
    }

    fd = _get_dfd (buf);
    if (fd <= 0)
    {
        _set_errno (buf, EINVAL);
        PRINTF ("get fd failed\n");
        return mws_return (-1, buf);
    }

    smbc_dirent = _smbc_readdir (fd);
    if (smbc_dirent)
    {
#ifdef USE_NATIVE
        if (buf) mw_append (buf, MW_FIELD_SMB_DIRENT, (char*)smbc_dirent, sizeof (*smbc_dirent) + 4096);
#else
        if (buf) mw_append (buf, MW_FIELD_SMB_DIRENT, (char*)smbc_dirent, sizeof (*smbc_dirent) + smbc_dirent->namelen + 4);
#endif
    }

    return mws_return (0, buf);
}

static void* _srv_smbc_closedir (void* buf)
{
    int fd = 0;

    if (buf == NULL || _smbc_readdir == NULL)
    {
        _set_errno (buf, EINVAL);
        PRINTF ("error buf: 0x%08x, fun: 0x%08x\n", (int) buf, (int) _smbc_closedir);
        return mws_return (-1, buf);
    }

    fd = _get_dfd (buf);
    if (fd < 0)
    {
        _set_errno (buf, (int) EINVAL);
        PRINTF ("get fd failed\n");
        return mws_return (-1, buf);
    }

    if (_smbc_closedir (fd) < 0)
    {
        _set_errno (buf, (int) errno);
        PRINTF ("closedir %d failed %d, %s\n", fd, errno, strerror (errno));
        return mws_return (-1, buf);
    }

    return mws_return (0, NULL);
}

#ifdef USE_NATIVE
static int _init_smbc (int lev)
{
    _smbc_opendir = opendir;
    _smbc_readdir = readdir;
    _smbc_closedir = closedir;
    _smbc_lseekdir = seekdir;
    _smbc_open = open;
    _smbc_close = close;
    _smbc_read = read;
    _smbc_lseek = lseek;
    _smbc_stat = stat;

    return 0;
}
#else
#define LIBICONV_PATH "/3rd/"
#define LIBSMB_PATH "/3rd/lib/samba/"
static void* h_smbc_lib = NULL;
static int _init_smbc (int lev)
{
    /* semaphore */
    if (sem == 0) {
        sem = mw_sem_open(SMBC_CLIENT_SM);
    }

    if (h_smbc_lib == NULL)
    {
        h_smbc_lib = dlopen ("libsmbclient.so", RTLD_LAZY|RTLD_GLOBAL);
        if (h_smbc_lib == NULL)
        {
            void* pv_lib = NULL;
            PRINTF ("Notes: cannot open libsmbclient, %s\n", dlerror());

            /* following code is very bad, may be a test just for /3rd delay mount */
            {
                int i = 0;
                for (i = 0; i < 5; i++)
                {
                    if (access(LIBSMB_PATH, F_OK) == 0)
                    {
                        break;
                    }
                    sleep (1);
                }
                if (i > 0)
                {
                    PRINTF ("sleep %d sec.\n", i);
                }

                if ((pv_lib = dlopen (LIBICONV_PATH"lib/libiconv.so.2", RTLD_NOW|RTLD_GLOBAL)) != NULL)
                {
                    if ((pv_lib = dlopen (LIBSMB_PATH"lib/libtalloc.so.2", RTLD_NOW|RTLD_GLOBAL)) ==  NULL)
                    {
                        PRINTF ("dlopen %slib/libtalloc.so.2 err, %s\n", LIBSMB_PATH, dlerror());
                        pv_lib = dlopen (LIBSMB_PATH"lib/libtalloc.so.1", RTLD_NOW|RTLD_GLOBAL);
                    }
                    if (pv_lib == NULL)
                    {
                        PRINTF ("dlopen %slib/libtalloc.so.1 err, %s\n", LIBSMB_PATH, dlerror());
                    }
                    if ((pv_lib = dlopen (LIBSMB_PATH"lib/libtdb.so.1", RTLD_NOW|RTLD_GLOBAL)) == NULL)
                    {
                        PRINTF ("dlopen %slib/libtdb.so.1 err, %s\n", LIBSMB_PATH,dlerror());
                    }

                    {
                        if (dlopen (LIBSMB_PATH"lib/libwbclient.so.0", RTLD_NOW|RTLD_GLOBAL) != NULL)
                        {
                            h_smbc_lib = dlopen (LIBSMB_PATH"lib/libsmbclient.so.0", RTLD_NOW|RTLD_GLOBAL);
                            if (h_smbc_lib == NULL)
                            {
                                PRINTF ("dlopen %slib/libsmbclient.so.0 err, %s\n", LIBSMB_PATH, dlerror());
                            }
                        }
                        else
                        {
                            PRINTF ("dlopen %slib/libwbclient.so.0 err, %s\n", LIBSMB_PATH, dlerror());
                        }
                    }
                }
                else
                {
                    PRINTF ("dlopen %slib/libiconv.so.2 err %d, %s\n", LIBICONV_PATH, errno, strerror(errno));
                }
            }
        }

        if (h_smbc_lib != NULL)
        {
            PRINTF ("load libsmbclient OK\n");
            _smbc_init = dlsym((void*)h_smbc_lib, "smbc_init");
            _smbc_open = dlsym((void*)h_smbc_lib, "smbc_open");
            _smbc_read = dlsym((void*)h_smbc_lib, "smbc_read");
            _smbc_lseek = dlsym((void*)h_smbc_lib, "smbc_lseek");
            _smbc_stat = dlsym((void*)h_smbc_lib, "smbc_stat");
            _smbc_close = dlsym((void*)h_smbc_lib, "smbc_close");

            _smbc_opendir = dlsym((void*)h_smbc_lib, "smbc_opendir");
            _smbc_closedir = dlsym((void*)h_smbc_lib, "smbc_closedir");
            _smbc_readdir = dlsym((void*)h_smbc_lib, "smbc_readdir");
            _smbc_lseekdir = dlsym((void*)h_smbc_lib, "smbc_lseekdir");

            _smbc_sc = dlsym((void*)h_smbc_lib, "smbc_set_context");
            _smbc_nc = dlsym((void*)h_smbc_lib, "smbc_new_context");
            _smbc_ic = dlsym((void*)h_smbc_lib, "smbc_init_context");
            _smbc_fc = dlsym((void*)h_smbc_lib, "smbc_free_context");
#ifdef NET_SMB_IF_EXT
            _smbc_sa = dlsym((void*)h_smbc_lib, "smbc_setFunctionAuthData");
            _smbc_st = dlsym((void*)h_smbc_lib, "smbc_setTimeout");
#else
            _smbc_option_set = dlsym((void*)h_smbc_lib, "smbc_option_set");
#endif

            if (_smbc_init && _smbc_open && _smbc_read && _smbc_lseek && _smbc_close && _smbc_stat && _smbc_sc
                && _smbc_nc && _smbc_ic && _smbc_fc
#ifdef NET_SMB_IF_EXT
                && _smbc_sa && _smbc_st
#else
                && _smbc_option_set
#endif
                )
            {
                void *statcont = NULL;

                (*_smbc_init) (_for_smbc_perm, lev);

                if ((statcont = _smbc_sc (NULL)) != NULL)
                {
                    _smbc_st(statcont, NET_SMB_SMBC_TIMEO);
                    PRINTF ("set timout %d-ms\n", NET_SMB_SMBC_TIMEO);
                }
            }
            else
            {
                _smbc_init = NULL;
                _smbc_open = NULL;
                _smbc_read = NULL;
                _smbc_lseek = NULL;
                _smbc_close = NULL;
                _smbc_stat = NULL;
                _smbc_opendir = NULL;
                _smbc_closedir = NULL;
                _smbc_readdir = NULL;
                _smbc_lseekdir = NULL;
                dlclose ((void*)h_smbc_lib);
                h_smbc_lib = NULL;
                PRINTF ("unload libsmbclient\n");
            }
        }
    }
    return 0;
}
#endif

extern int mw_memleak_init (void);
int main (int argc, char** argv)
{
    int lev = 0;

    if (argc > 1) lev = atoi (argv[1]);
    if (lev < 0) lev = 0;
    if (lev > 10) lev = 10;

    /* daemon */
#if 1
    {
        int i = 0;
        if (fork() > 0) exit (0);
        setsid ();
        for (i = 3; i < 1024; i++) {
            close (i);
        }
    }
#endif

#ifdef _NET_MEMLEAK_DEBUG
    /* memleak check */
    mw_memleak_init ();
#endif

    _init_smbc(lev);

    _start_smbc_cmd(); /* for testing only */
    _start_smbc_srv();

    while (1) sleep (10);
    return 0;
}

