#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/un.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>

#if 0
#include <linux/types.h>
#include <linux/netlink.h>
#endif

#include <pthread.h>
#include <arpa/inet.h>

#include <assert.h>

#include "mylist.h"

#include "mw_common.h"

#define NET_SMB_MODULE_NAME "smb_mws"
#include "smb_common.h"

#if 0
extern int _get_ms (void);
#else
int _get_ms (void)
{
    struct timespec tp = {0, 0};
    clock_gettime (CLOCK_MONOTONIC, &tp);
    tp.tv_sec &= 0x3FFFFF;
    return tp.tv_sec * 1000 + tp.tv_nsec / (1000*1000);
}
#endif
#define PRINTF1(...) do { unsigned int ct = _get_ms(); fprintf (stderr, "[smb.mw.srv] %u.%03u, %s, %s.%d >>> ", ct/1000, ct%1000, __FILE__, __FUNCTION__, __LINE__); fprintf (stderr, __VA_ARGS__); fflush (stderr); } while (0)

static void _prn_mem_pre (const char* data, int len, int start_idx)
{
    int i = 0;
    if (start_idx >= 0) fprintf(stderr, "0x%08x: ", start_idx);
    for (i = 0; i < 16; i++)
    {
        if (i < len) fprintf(stderr, "%02x ", (unsigned char) data[i]);
        else fprintf(stderr, "   ");
        if (i == 7) fprintf(stderr, " ");
    }
    fprintf(stderr, "   ");
}

static void _prn_mem_suf (const char* data, int len)
{
    int i;
    for (i = 0; i < 16; i++)
    {
        if (i < len)
        {
            if (isprint (data[i]))
            {
                fprintf(stderr, "%c", data[i]);
            }
            else fprintf(stderr, ".");
        }
        else
        {
            fprintf(stderr, " ");
        }
    }
}

void _prn_mem (const char* data, int len)
{
    int i;
    fprintf(stderr, "====== mem: 0x%08x, size: %d\n", (int) data, len);
    if (data == NULL) return;
    for (i = 0; i < len; i+=16)
    {
        char* pd = (char*) data + i;
        int clen = (len / 16) > 0 ? 16 : len % 16;
        _prn_mem_pre (pd, clen, i);
        _prn_mem_suf (pd, clen);
        fprintf(stderr, "\n");
    }
    fprintf(stderr, "==============================================\n");
    fflush (stderr);
}

struct _MW_SERVER {
    char* path;
    int fd;
    handle_t h_mws_list;
    handle_t h_clt_list;
    pthread_t t_thread;
    int cleaned;
};

struct _MW_SERVICE {
    char name[64];
    int fd;
    void* (*fun) (void*);
    void* tag;
    int calling;
    unsigned int cnt;
};

struct _MW_CLIENT_END {
    int fd;
    int sfd;
    char* sname;
    int rlen;
    char* rbuf;
    int wlen;
    char* wbuf;
};

#define MW_SERVER_MAX 64
struct _MW_SERVER mws[MW_SERVER_MAX];
/*static int _mws_inited = 0;*/

static void* _mws_server (void *pvtag);

static int _mw_nonblock (int fd)
{
    int bf;

    bf = fcntl (fd, F_GETFL, 0);
    if (bf == -1)
    {
        return -1;
    }

    bf |= O_NONBLOCK;

    return fcntl (fd, F_SETFL, bf);
}

int mws_open (const char* path)
{
    struct sockaddr_un servaddr;
    int fd = -1;
    int alen = 0;
    int rc = -1;

    int mwidx = 0;

    mws_lock_init ();

    if (path == NULL) {
        PRINTF ("path is NULL\n");
        return -1;
    }
    /*if (_mws_inited == 1)
    {
        PRINTF ("existed mw server, path: %s\n", mws.path);
        return 0;
    }*/

    /*_mws_inited = 1;*/

    mws_lock();

    {
        for (mwidx = 0; mwidx < MW_SERVER_MAX; mwidx++) {
            if (mws[mwidx].path == NULL) break;
        }
        if (mwidx >= MW_SERVER_MAX) {
            mws_unlock();
            PRINTF ("MAX server list error\n");
            return -1;
        }
    }

    memset(&mws[mwidx], 0, sizeof(mws[mwidx]));
    mws[mwidx].path = NULL;
    mws[mwidx].fd = -1;
    mws[mwidx].h_mws_list = null_handle;
    mws[mwidx].h_clt_list = null_handle;
    mws[mwidx].cleaned = 0;

    memset (&servaddr, 0, sizeof (servaddr));
    fd = socket (AF_LOCAL, SOCK_STREAM, 0);
    if (fd < 0) {
        mws_unlock();
        PRINTF ("socket create fail, %d, %s\n", errno, strerror(errno));
        return -1;
    }

    servaddr.sun_family = AF_LOCAL;
    strcpy (servaddr.sun_path, path);

    alen = 1;
    rc = setsockopt (fd, SOL_SOCKET, SO_REUSEADDR, &alen, sizeof (alen));
    if (rc != 0)
    {
        PRINTF ("setsockopt REUSEADDR err %d\n", rc);
    }

    rc = bind (fd, (struct sockaddr *)&servaddr, sizeof (servaddr));
    if (rc != 0)
    {
        mws_unlock();
        PRINTF ("bind err %d, %s\n", errno, strerror(errno));
        close (fd);
        return -1;
    }

    rc = listen (fd, 5);
    if (rc != 0)
    {
        mws_unlock();
        PRINTF ("listen err %d\n", rc);
        close (fd);
        return -1;
    }

    _mw_nonblock(fd);

    mws[mwidx].path = strdup (path);
    mws[mwidx].fd = fd;

    {
#if 0
        pthread_t t_thread;
        pthread_attr_t t_attr;

        pthread_attr_init (&t_attr);
        pthread_attr_setdetachstate (&t_attr, PTHREAD_CREATE_DETACHED);

        if (pthread_create (&t_thread, &t_attr, _mws_server, (void*)fd) != 0)
#endif
        if (pthread_create (&mws[mwidx].t_thread, NULL, _mws_server, (void*)fd) != 0)
        {
            mws_unlock();
            PRINTF ("create thread err %d, %s\n", errno, strerror(errno));
            close (fd);
            return -1;
        }
    }

    mws_unlock();

    return fd;
}

int mws_unreg_services_all(int fd);
static int _mws_clean_clts_all(handle_t h_clt_list);
int mws_close(const char* path)
{
    int fd = -1;
    int mwidx = 0;

    mws_lock();

    for (mwidx = 0; mwidx < MW_SERVER_MAX; mwidx++) {
        if (mws[mwidx].path && strcmp(mws[mwidx].path, path) == 0) break;
    }
    if (mwidx >= MW_SERVER_MAX) {
        mws_unlock();
        PRINTF ("no matched item in srv list\n");
        return -1;
    }

    if (mws[mwidx].fd >= 0) {
        PRINTF("unreg all services ...\n");
        mws_unreg_services_all(mws[mwidx].fd);
    }

    free(mws[mwidx].path);
    close(mws[mwidx].fd);

    if (pthread_cancel(mws[mwidx].t_thread) == 0) {
        pthread_join(mws[mwidx].t_thread, NULL);
    }

    if (mws[mwidx].cleaned == 0) {
        _mws_clean_clts_all(mws[mwidx].h_clt_list);
        my_list_destroy(mws[mwidx].h_mws_list, NULL);
        my_list_destroy(mws[mwidx].h_clt_list, NULL);
        mws[mwidx].cleaned = 1;
    }

    mws[mwidx].path = NULL;
    mws[mwidx].fd = -1;
    mws[mwidx].h_mws_list = null_handle;
    mws[mwidx].h_clt_list = null_handle;

    mws_unlock();

    return fd;
}

int mws_fd(const char* path)
{
    int fd = -1;
    int mwidx = 0;

    mws_lock();

    {
        for (mwidx = 0; mwidx < MW_SERVER_MAX; mwidx++) {
            if (mws[mwidx].path && strcmp(mws[mwidx].path, path) == 0) break;
        }
        if (mwidx >= MW_SERVER_MAX) {
            mws_unlock();
            PRINTF ("not matched item in srv list\n");
            return -1;
        }
    }

    mws_unlock();

    return fd;
}

struct _MW_SERVER_TAG {
    int maxfd;
    int basefd;
    fd_set* prset;
    fd_set* pwset;
};

extern int _mws_need_buf_size (const char* buf);
/*#define MW_FIELD_SYS_SERVICE_NAME 0x00010001
#define MW_FIELD_SYS_RETURN_CODE  0x00010002*/

extern int mw_get (const void* data, int idx, int field_id, char* presult, int *size);
static int _mws_get_service_name (const char* buf, char* name)
{
    if (buf == NULL || name == NULL) return -1;
    return mw_get (buf, 0, MW_FIELD_SYS_SERVICE_NAME, name, NULL);
}

static int _prn_srvs_apply (void* pv_data, void* pv_tag)
{
    struct _MW_SERVICE *ps = (struct _MW_SERVICE*) pv_data;
    int* pi = (int*) pv_tag;

    if (ps == NULL || pi == NULL) return 0;

    fprintf (stderr, "  == Service %d, '%s' be called %u\n", *pi, ps->name, ps->cnt);
    if (ps->calling) {
        fprintf (stderr, "             %*s  '%s' is running ***\n", *pi > 10 ? 2 : 1, "", ps->name);
    }
    (*pi)++;

    return 0;
}

void _prn_clients (void)
{
    int i = 0;
    mws_lock();
    for (i = 0; i < MW_SERVER_MAX; i++) {
        if (mws[i].h_clt_list == null_handle) break;
        fprintf (stderr, ">> server %s have %d client request(s)\n", mws[i].path, my_list_size (mws[i].h_clt_list));
    }
    mws_unlock();
}

/* FIXME: mws.h_mws_list should be sync ?? */
void _prn_services (void)
{
    int i = 0;
    mws_lock();
    for (i = 0; i < MW_SERVER_MAX; i++) {
        int idx = 0;
        if (mws[i].h_mws_list == null_handle) break;
        fprintf (stderr, ">> ===== server %s (%d) have %d service(s) =====\n", mws[i].path, mws[i].fd, my_list_size (mws[i].h_mws_list));
        my_list_map (mws[i].h_mws_list, _prn_srvs_apply, (void*) &idx);
    }
    mws_unlock();
}

static int _rwset_apply (void* pv_data, void* pv_tag)
{
    struct _MW_CLIENT_END *pclt = (struct _MW_CLIENT_END*) pv_data;
    struct _MW_SERVER_TAG *ptag = (struct _MW_SERVER_TAG*) pv_tag;

    if (pclt == NULL || ptag == NULL) return 0;

    /* check if a same server */
    if (pclt->sfd != ptag->basefd) return 0;

    if ((pclt->wlen > 0) ||
        (pclt->rlen > 0 && pclt->rlen == _mws_need_buf_size (pclt->rbuf))) /* prevent blocking */
    {
        FD_SET (pclt->fd, ptag->pwset);
        if (pclt->fd > ptag->maxfd)
        {
            ptag->maxfd = pclt->fd;
        }
    }

    if (pclt->rlen == 0 || pclt->rlen < _mws_need_buf_size (pclt->rbuf))
    {
        FD_SET (pclt->fd, ptag->prset);
        if (pclt->fd > ptag->maxfd)
        {
            ptag->maxfd = pclt->fd;
        }
    }

    return 0;
}

static void _free_wbuf (char** pwbuf)
{
    int delay = 0;
    if (pwbuf == NULL || *pwbuf == NULL) return;
    if (mw_get (*pwbuf, 0, MW_FIELD_SYS_DELAY_FREE, (char*) &delay, 0) != 0 || delay != 1)
    {
        mw_free (*pwbuf);
    }
    *pwbuf = NULL;
}

static int _deal_wset_apply (void* pv_data, void* pv_tag)
{
    struct _MW_CLIENT_END *pclt = (struct _MW_CLIENT_END*) pv_data;
    struct _MW_SERVER_TAG *ptag = (struct _MW_SERVER_TAG*) pv_tag;

    if (pclt == NULL || ptag == NULL) return 0;

    /* check if a same server */
    if (pclt->sfd != ptag->basefd) return 0;

    if (FD_ISSET (pclt->fd, ptag->pwset))
    {
        if (pclt->wlen <= 0) return 0;

        if (pclt->wlen == _mws_need_buf_size (pclt->wbuf))
        {
            int slen = 0;
            int len = 0;
            while (slen < pclt->wlen)
            {
                if ((len = send (pclt->fd, ((char*)pclt->wbuf) + slen, pclt->wlen - slen, 0)) <= 0)
                {
                    if (errno == EAGAIN)
                    {
                        /*usleep (1000);*/
                        /*PRINTF (" >>>> send rc %d, %d, %s\n", len, errno, strerror(errno));*/
                        continue;
                    }
                    if (len == 0) {
                        PRINTF ("WARN: %d send %d-byte, need send %d. %d, %s\n", pclt->fd, slen, pclt->wlen, errno, strerror(errno));
                    } else {
                        PRINTF ("ERROR: %d send %d, %s\n", pclt->fd, errno, strerror(errno));
                    }
                    break;
                }
                slen += len;
            }
            /*PRINTF (" >>>> send %d-byte, need send %d\n", slen, pclt->wlen);*/
            _free_wbuf (&(pclt->wbuf));
            pclt->wlen = 0;
        }
    }

    return 0;
}

extern void* mw_alloc (int size);
extern int mw_append (void* data, int field_id, const char* presult, int len);
void* mws_return (int rc, char* buf)
{
    if (buf == NULL)
    {
        buf = mw_alloc (16);
    }

    if (buf == NULL)
    {
        PRINTF ("buf is null\n");
        return NULL;
    }

    mw_append (buf, MW_FIELD_SYS_RETURN_CODE, (char*) &rc, 4);

    return buf;
}

static int _deal_mw_call_apply (void *pv_data, void* pv_tag)
{
    struct _MW_SERVICE *ps = (struct _MW_SERVICE*) pv_data;
    struct _MW_CLIENT_END *pclt = (struct _MW_CLIENT_END*) pv_tag;
    char name[64] = "";

    if (ps == NULL || pclt == NULL || pclt->rbuf == NULL) return 0;
    if (ps->fd != pclt->sfd) return 0;

    _mws_get_service_name (pclt->rbuf, name);

    if (ps->fun && strcmp (name, ps->name) == 0)
    {
        void *rbuf = NULL;
        void *wbuf = NULL;

#if 0
        PRINTF ("==== calling srv.name: %s, fd: %d ==== client: sname: %s, sfd: %d, cfd: %d ====\n",
                ps->name, ps->fd, pclt->sname == NULL ? name : pclt->sname, pclt->sfd, pclt->fd);
        if (strcmp (ps->name, "_smbc_open") == 0) {
            _prn_mem (pclt->rbuf, 128);
        }
#endif

        ps->calling = 1;
        wbuf = ps->fun (pclt->rbuf);
        ps->calling = 0;

#if 0
        PRINTF ("==== calling srv.name: %s (%d, %d) END ====\n", ps->name, pclt->sfd, pclt->fd);
#endif

        ps->cnt++;

        if (wbuf == NULL)
        {
            /* TODO */
            assert (0);
        }

        pclt->wlen = _mws_need_buf_size (wbuf);
        pclt->wbuf = wbuf;
        rbuf = pclt->rbuf;
        /* may have bug */
        pclt->rbuf = NULL;
        pclt->rlen = 0;
        if (rbuf != wbuf)
        {
            mw_free (rbuf);
            rbuf = NULL;
        }

        return 1;
    }

    return 0;
}

static int _print_mw_call_apply (void *pv_data, void* pv_tag)
{
    struct _MW_SERVICE *ps = (struct _MW_SERVICE*) pv_data;
    struct _MW_CLIENT_END *pclt = (struct _MW_CLIENT_END*) pv_tag;
    char name[64] = "";

    if (ps == NULL || pclt == NULL || pclt->rbuf == NULL) return 0;
    if (ps->fd != pclt->sfd) return 0;

    _mws_get_service_name (pclt->rbuf, name);

    PRINTF ("==== name: %s\n", name);
    PRINTF ("==== calling srv.name: %s, fd: %d, fun: %p ==== client: sname: %s, sfd: %d, cfd: %d ====\n", ps->name, ps->fd, ps->fun, pclt->sname == NULL ? name : pclt->sname, pclt->sfd, pclt->fd);
    if (ps->fun && strcmp (name, ps->name) == 0) {
        PRINTF("==== find service, why ? ?????\n");
        return 1;
    }

    return 0;
}

extern void* mw_nodelay (void* ptr);
static int _deal_rset_apply (void* pv_data, void* pv_tag)
{
    struct _MW_CLIENT_END *pclt = (struct _MW_CLIENT_END*) pv_data;
    struct _MW_SERVER_TAG *ptag = (struct _MW_SERVER_TAG*) pv_tag;
    int nlen = 0;

    if (pclt == NULL || ptag == NULL) return 0;

    /* check if a same server */
    if (pclt->sfd != ptag->basefd) return 0;

    if (pclt->fd < 0) return 0;

    nlen = _mws_need_buf_size (pclt->rbuf);

    if (pclt->rlen > 0 && pclt->rlen == nlen)
    {
        int i = 0;
        if (pclt->wlen != 0)
        {
            return 0;
        }
        for (i = 0; i < MW_SERVER_MAX; i++) {
            if (mws[i].fd == pclt->sfd) {
                my_list_map (mws[i].h_mws_list, _deal_mw_call_apply, pclt);
                break;
            }
        }
        if (pclt->rlen > 0)
        {
            /* TODO */
            /*assert (0);*/
            void* rbuf = pclt->rbuf;
            void *wbuf = mws_return (-1, pclt->rbuf);

            PRINTF (">>> WARN: What's wrong? Service Not Found. i %d, mws[%d].fd %d, pclt->sfd %d, mws.svc.cnt: %p, %d (next: %p, %d)\n",
                    i, i, mws[i].fd, pclt->sfd, mws[i].h_mws_list, my_list_size(mws[i].h_mws_list), mws[i+1].h_mws_list, my_list_size(mws[i+1].h_mws_list));
            _prn_mem (pclt->rbuf, 128);
            my_list_map (mws[i].h_mws_list, _print_mw_call_apply, pclt);

            pclt->wlen = _mws_need_buf_size (wbuf);
            pclt->wbuf = wbuf;
            pclt->rbuf = NULL;
            pclt->rlen = 0;
            if (rbuf != wbuf)
            {
                mw_free (rbuf);
                rbuf = NULL;
            }

            return 0;
        }
        return 0;
    }

    if (FD_ISSET (pclt->fd, ptag->prset))
    {
        int rlen = 0;

        if (pclt->rlen < 0) pclt->rlen = 0;

        if (pclt->rlen == 0)
        {
            char rbuf[MW_FIRST_READ_SIZE] = "";
            int err = 0;

            rlen = recv (pclt->fd, rbuf, MW_FIRST_READ_SIZE, 0);
            nlen = _mws_need_buf_size (rbuf);

            if (rlen < MW_FIRST_READ_SIZE || nlen <= 0 || nlen > 2*1024*1024) err = 1;

            if (err == 0)
            {
                if (pclt->rbuf == NULL) pclt->rbuf = malloc (nlen);
                if (pclt->rbuf == NULL) err = 2;
            }

            if (err)
            {
                if (rlen > 0) {
                    PRINTF ("WARN: %d recv %d, need_buff: %d\n", pclt->fd, rlen, nlen);
                } else if (rlen < 0) {
                    PRINTF ("ERROR: %d recv, %d, %s\n", pclt->fd, errno, strerror(errno));
                }
                close (pclt->fd);
                pclt->fd = -1;

                mw_free (pclt->rbuf);
                pclt->rbuf = NULL;
                _free_wbuf (&(pclt->wbuf));

                pclt->rlen = pclt->wlen = 0;

                return 0;
            }

            memset (pclt->rbuf, 0, nlen);
            memcpy (pclt->rbuf, rbuf, MW_FIRST_READ_SIZE);

            pclt->rlen = MW_FIRST_READ_SIZE;
            /* delete DELAY flag to prevent from memleak */
            mw_nodelay (pclt->rbuf);
        }
        else if (pclt->rlen < nlen)
        {
            rlen = recv (pclt->fd, pclt->rbuf + pclt->rlen, nlen - pclt->rlen, 0);
            if (rlen <= 0)
            {
                if (rlen < 0) {
                    PRINTF ("ERROR: %d recv, %d, %s\n", pclt->fd, errno, strerror(errno));
                }
                close (pclt->fd);
                pclt->fd = -1;

                mw_free (pclt->rbuf);
                pclt->rbuf = NULL;
                _free_wbuf (&(pclt->wbuf));

                pclt->rlen = pclt->wlen = 0;

                return 0;
            }
            pclt->rlen += rlen;
        }
    }

    return 0;
}

static int _clean_clt_cmp (const void *pv1, const void* pv2)
{
    struct _MW_CLIENT_END *pclt = (struct _MW_CLIENT_END*) pv1;

    if (pclt == NULL) return -1;

    if (pclt->fd < 0) return 0;

    return -1;
}

static int _mws_clean_clts (handle_t h_clt_list)
{
    struct _MW_CLIENT_END *pclt = NULL;

    while ((pclt = my_list_delete2 (h_clt_list, NULL, _clean_clt_cmp)) != NULL)
    {
        free (pclt);
    }

    return 0;
}

static int _mws_clean_clts_all(handle_t h_clt_list)
{
    struct _MW_CLIENT_END *pclt = NULL;

    while ((pclt = my_list_delete_head(h_clt_list)) != NULL)
    {
        if (pclt->fd >= 0) {
            close (pclt->fd);
            pclt->fd = -1;
        }
        free(pclt->sname);
        pclt->sname = NULL;
        if (pclt->rbuf == pclt->wbuf) {
            mw_free(pclt->rbuf);
            pclt->rbuf = pclt->wbuf = NULL;
        }
        if (pclt->rbuf) {
            mw_free(pclt->rbuf);
            pclt->rbuf = NULL;
        }
        if (pclt->wbuf) {
            mw_free(pclt->wbuf);
            pclt->wbuf = NULL;
        }
        free (pclt);
    }

    return 0;
}

static void _mws_server_cleanup(void* tag)
{
    int mwidx = (int)tag;

    mws_lock();

    _mws_clean_clts_all(mws[mwidx].h_clt_list);
    my_list_destroy(mws[mwidx].h_mws_list, NULL);
    my_list_destroy(mws[mwidx].h_clt_list, NULL);
    mws[mwidx].cleaned = 1;

    mws_unlock();
}

static void* _mws_server (void *pvtag)
{
    int fd = -1;
    int mwidx = 0;

    fd = (int) pvtag;
    if (fd < 0)
    {
        PRINTF ("server init error, fd is %d?\n", fd);
        return NULL;
    }

    mws_lock();
    for (mwidx = 0; mwidx < MW_SERVER_MAX; mwidx++) {
        if (mws[mwidx].fd == fd) break;
    }
    if (mwidx >= MW_SERVER_MAX) {
        mws_unlock();
        PRINTF ("no such server, fd is %d\n", fd);
        return NULL;
    }

    if (mws[mwidx].h_mws_list == null_handle)
    {
        my_list_create (&mws[mwidx].h_mws_list);
    }

    if (mws[mwidx].h_clt_list == null_handle)
    {
        my_list_create (&mws[mwidx].h_clt_list);
    }
    mws_unlock();

    while (1)
    {
        fd_set rset;
        fd_set wset;
        struct _MW_SERVER_TAG t_st = {-1, -1, NULL, NULL};
        int ready = 0;

        FD_ZERO (&rset);
        FD_ZERO (&wset);

        FD_SET (fd, &rset);

        t_st.maxfd = fd;
        t_st.basefd = fd;
        t_st.prset = &rset;
        t_st.pwset = &wset;

        mws_lock();
        my_list_map (mws[mwidx].h_clt_list, _rwset_apply, (void*) &t_st);
        mws_unlock();

        ready = select (t_st.maxfd + 1, t_st.prset, t_st.pwset, NULL, NULL);

        if (ready == 0) continue;

        mws_lock ();

        pthread_cleanup_push(_mws_server_cleanup, (void*)mwidx);

        do {
            if (FD_ISSET (fd, t_st.prset))
            {
                struct sockaddr_in cliaddr = {0};
                int clen = sizeof(cliaddr);
                int cfd = -1;
                struct _MW_CLIENT_END* pclt = NULL;

                --ready;

                cfd = accept (fd, (struct sockaddr*) &cliaddr, (socklen_t*) &clen);
                if (cfd < 0)
                {
                    break;
                }

                _mw_nonblock (cfd);

                pclt = malloc (sizeof(*pclt));
                if (pclt == NULL)
                {
                    break;
                }

                memset (pclt, 0, sizeof(*pclt));
                pclt->fd = cfd;
                pclt->sfd = fd;

                my_list_add_tail (mws[mwidx].h_clt_list, pclt);
            }
        } while (0);

        /* check write */
        my_list_map (mws[mwidx].h_clt_list, _deal_wset_apply, (void*) &t_st);
        /* check read */
        my_list_map (mws[mwidx].h_clt_list, _deal_rset_apply, (void*) &t_st);

        _mws_clean_clts (mws[mwidx].h_clt_list);

        pthread_cleanup_pop(0);

        mws_unlock ();
    }

    _mws_clean_clts_all(mws[mwidx].h_clt_list);
    my_list_destroy(mws[mwidx].h_mws_list, NULL);
    my_list_destroy(mws[mwidx].h_clt_list, NULL);
    mws[mwidx].cleaned = 1;

    return NULL;
}

static int _exist_name_cmp (const void* pv1, const void* pv2)
{
    struct _MW_SERVICE *psi = (struct _MW_SERVICE*) pv1;
    char *name = (char *) pv2;

    if (psi == NULL || name == NULL) return -1;

    if (strcmp (name, psi->name) == 0)
    {
        return 0;
    }

    return -1;
}

int mws_reg_service (int fd, const char* name, void* (*_fun) (void*), void* tag)
{
    struct _MW_SERVICE *psi = NULL;
    int mwidx = 0;
    int rc = -1;

    if (fd < 0 || name == NULL || _fun == NULL) {
        PRINTF(" --- error: %d, %s\n", fd, name);
        return -1;
    }

    mws_lock();
    for (mwidx = 0; mwidx < MW_SERVER_MAX; mwidx++) {
        if (fd == mws[mwidx].fd) break;
    }
    if (mwidx >= MW_SERVER_MAX)
    {
        mws_unlock();
        PRINTF (" -- reg service '%s` error, %d, not found mws[x].fd --\n", name, fd);
        return -1;
    }

    if (mws[mwidx].h_mws_list == null_handle)
    {
        my_list_create (&mws[mwidx].h_mws_list);
    }

    if (my_list_search2 (mws[mwidx].h_mws_list, name, _exist_name_cmp) != NULL)
    {
        mws_unlock();
        PRINTF (" -- reg fun error: '%s` existed !? -- \n", name);
        return -1;
    }

    psi = malloc (sizeof(*psi));
    if (psi == NULL)
    {
        mws_unlock();
        PRINTF (" -- reg fun error: '%s` out of memory\n", name);
        return -1;
    }

    memset (psi, 0, sizeof(*psi));

    strncpy (psi->name, name, sizeof(psi->name)-1);
    psi->fd = fd;
    psi->fun = _fun;
    psi->tag = tag;

    rc = my_list_add_tail (mws[mwidx].h_mws_list, psi);
    if (rc != 0) {
        PRINTF (" -- reg fun error: '%s` add list error\n", name);
    }

    mws_unlock();
    /*PRINTF("p %d, cnt: %d, idx: %d, %p, fd: %d, %s\n", getpid(), my_list_size(mws[mwidx].h_mws_list), mwidx, mws[mwidx].h_mws_list, psi->fd, psi->name);*/
    return rc;
}

int mws_unreg_service(int fd, const char* name)
{
    struct _MW_SERVICE *psi = NULL;
    int mwidx = 0;
    int rc = -1;

    if (fd < 0 || name == NULL) {
        PRINTF(" --- error: %d, %s\n", fd, name);
        return -1;
    }

    mws_lock();
    for (mwidx = 0; mwidx < MW_SERVER_MAX; mwidx++) {
        if (fd == mws[mwidx].fd) break;
    }
    if (mwidx >= MW_SERVER_MAX)
    {
        mws_unlock();
        PRINTF (" -- unreg service '%s` error, %d, not found mws[x].fd --\n", name, fd);
        return -1;
    }

    if (mws[mwidx].h_mws_list == null_handle)
    {
        mws_unlock();
        PRINTF (" -- unreg service '%s`, %d, is null --\n", name, fd);
        return 0;
    }

    psi = my_list_delete2(mws[mwidx].h_mws_list, name, _exist_name_cmp);
    if (psi == NULL)
    {
        mws_unlock();
        PRINTF (" -- unreg fun error: '%s` not existed !? -- \n", name);
        return -1;
    }

    free(psi);

    mws_unlock();

    return rc;
}

int mws_unreg_services_all(int fd)
{
    struct _MW_SERVICE *psi = NULL;
    int mwidx = 0;
    int rc = -1;

    if (fd < 0) {
        PRINTF(" --- error: %d\n", fd);
        return -1;
    }

    mws_lock();
    for (mwidx = 0; mwidx < MW_SERVER_MAX; mwidx++) {
        if (fd == mws[mwidx].fd) break;
    }
    if (mwidx >= MW_SERVER_MAX)
    {
        mws_unlock();
        PRINTF (" -- unreg all service error, %d, not found mws[x].fd --\n", fd);
        return -1;
    }

    if (mws[mwidx].h_mws_list == null_handle)
    {
        mws_unlock();
        PRINTF (" -- unreg all service, %d, is null --\n", fd);
        return 0;
    }

    while ((psi = my_list_delete_head(mws[mwidx].h_mws_list)) != NULL) {
        free(psi);
    }

    mws_unlock();

    return rc;
}

/* to checking memleakge */
#define MFD_MEMLEAK_TIME         MW_FIELD_LONG_BGN + 10
#define MFD_MEMLEAK_SIZE         MW_FIELD_LONG_BGN + 11
#ifdef _NET_MEMLEAK_DEBUG
extern void _prn_mem_lists (unsigned int ui4_memsize, unsigned int ui4_memtime, const char *ps_ip, unsigned short ui2_port);
#endif
static void* _srv_memleak (void* buf)
{
    int time = 0, size = 0;
    mw_get (buf, 0, MFD_MEMLEAK_TIME, (char*) &time, 0);
    mw_get (buf, 0, MFD_MEMLEAK_SIZE, (char*) &size, 0);
    /*if (time < 0) time = 0;*/
    if (size < 0) size = 0;
#ifdef _NET_MEMLEAK_DEBUG
    _prn_mem_lists (size, time, NULL, 0);
#else
    PRINTF ("not support memleakage checking\n");
#endif
    return mws_return (-1, buf);
}

int mw_memleak_init (void)
{
    char srv[128] = "";
    int fd = -1;

    sprintf (srv, "/tmp/mw_memleak_%u", getpid());
    unlink (srv);

    fd = mws_open (srv);
    if (fd < 0) {
        fprintf (stderr, "open mws %s error, %d, %s\n", srv, errno, strerror (errno)); 
        return -1;
    }

    mws_reg_service (fd, "_memleak", _srv_memleak, NULL);

    return 0;
}

int mw_memleak_ck (int pid, int time, int size)
{
    char srv[128] = "";
    int fd = -1;

    if (pid <= 0)
    {
        PRINTF ("pid %d is invalid, rc error -1\n", pid);
        return -1;
    }

    sprintf (srv, "/tmp/mw_memleak_%u", pid);

    if ((fd = mwc_open (srv)) < 0)
    {
        PRINTF ("pid %d has no checking memleak\n", pid);
        return -1;
    }

    {
        void* buf = mw_alloc (1024);
        if (buf == NULL) return -1;
        mw_append (buf, MFD_MEMLEAK_TIME, (char*) &time, 0);
        mw_append (buf, MFD_MEMLEAK_SIZE, (char*) &size, 0);
        mwc_call (fd, "_memleak", buf, 1024, NULL, NULL);
        mw_free (buf);
    }

    mwc_close (fd);

    return 0;
}

extern int mw_lock_init(pthread_mutex_t* p_mutex);
extern int mw_lock(pthread_mutex_t* p_mutex);
extern int mw_unlock(pthread_mutex_t* p_mutex);

static pthread_mutex_t server_mutex;
static int flag_server_mutext_inited = 0;

int mws_lock_init(void)
{
    if (flag_server_mutext_inited == 0) {
        if (mw_lock_init(&server_mutex) != 0) {
            return -1;
        }
        flag_server_mutext_inited = 1;
    }
    return 0;
}

int mws_lock(void)
{
    return mw_lock(&server_mutex);
}

int mws_unlock(void)
{
    return mw_unlock(&server_mutex);
}


