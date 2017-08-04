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
#include <fcntl.h>
#include <ctype.h>

#include <pthread.h>
#include <arpa/inet.h>

#include "mylist.h"
#include "mw_common.h"

extern int _get_ms (void);
#define PRINTF(...) do { unsigned int ct = _get_ms(); printf ("[mw.cli] %d.%03d, %s, %s.%d >>> ", ct/1000, ct%1000, __FILE__, __FUNCTION__, __LINE__); printf(__VA_ARGS__); } while (0)

struct _MW_CLIENT {
    int fd;
    char* path;
};

static handle_t h_mwc_list = null_handle;

static int _cmp_fd (const void* pv1, const void* pv2)
{
    struct _MW_CLIENT* pclt = (struct _MW_CLIENT*) pv1;
    int fd = (int) pv2;

    if (pclt == NULL) return -1;

    if (fd != pclt->fd) return 1;

    return 0;
}

static handle_t _get_clt (int fd)
{
    return my_list_search2 (h_mwc_list, (void*) fd, _cmp_fd);
}

int mwc_open (const char* path)
{
    int fd = -1;
    struct sockaddr_un servaddr;
    int rc;
    struct _MW_CLIENT* pmwc = NULL;

    mwc_lock_init ();

    if (path == NULL)
    {
        return -1;
    }

    if (h_mwc_list == null_handle) {
        my_list_create (&h_mwc_list);
    }
    if (h_mwc_list == null_handle)
    {
        return -1;
    }

    fd = socket (AF_LOCAL, SOCK_STREAM, 0);
    if (fd < 0)
    {
        return -1;
    }

    servaddr.sun_family = AF_LOCAL;
    strcpy (servaddr.sun_path, path);

    rc = connect (fd, (const struct sockaddr*) &servaddr, sizeof (servaddr));
    if (rc != 0)
    {
        PRINTF ("connect error %d, %d, %s\n", rc, errno, strerror(errno));
        close (fd);
        return -1;
    }

    pmwc = malloc (sizeof(*pmwc));
    if (pmwc == NULL)
    {
        close (fd);
        return -1;
    }
    memset (pmwc, 0, sizeof(*pmwc));

    pmwc->fd = fd;
    pmwc->path = strdup (path);

    my_list_add_tail (h_mwc_list, pmwc);

    return fd;
}

int mwc_close (int fd)
{
    handle_t h_clt = null_handle;
    struct _MW_CLIENT *pdata = NULL;

    h_clt = _get_clt (fd);

    pdata = my_list_delete (h_mwc_list, h_clt);
    if (pdata)
    {
        close (pdata->fd);
        free (pdata->path);
        pdata->path = NULL;
        pdata->fd = -1;
        free (pdata);
        pdata = NULL;
    }

    if (my_list_count(h_mwc_list) == 0) {
        /* because the length/count is 0, so set the free fun to NULL */
        my_list_destroy(h_mwc_list, NULL);
        h_mwc_list = null_handle;
    }

    return 0;
}

extern int mw_append (void* data, int field_id, const char* presult, int len);
extern int mw_get (const void* data, int idx, int field_id, char* presult, int *size);
#define MW_BIG_BUF_SIZE 262144
int mwc_call (int fd, const char* name, char* snd_buf, int snd_size, char** pp_rcv_buf, int* p_rcv_size)
{
    struct _MW_CLIENT* pmwc = NULL;
    int snd_buf_need_free = 0;
    int slen = 0;
    int rc = -1;

    mwc_lock ();

    do {
        handle_t h_clt = _get_clt (fd);

        pmwc = (struct _MW_CLIENT*) h_clt;
        /*PRINTF ("call service '%s`, hdl: 0x%08x\n", name, (int) h_clt);*/

        if (h_clt == null_handle || name == NULL)
        {
            PRINTF ("clt: 0x%08x, name: %s\n", (int) h_clt, name);
            break;
        }

        /* FIXME: may be comment */
        if (my_list_search (h_mwc_list, h_clt) == NULL)
        {
            break;
        }

        if (snd_buf == NULL)
        {
            snd_buf = mw_alloc (256);
            snd_size = 256;
            snd_buf_need_free = 1;
        }

        if (snd_buf == NULL)
        {
            break;
        }

        if (mw_append (snd_buf, MW_FIELD_SYS_SERVICE_NAME, name, 63) != 0)
        {
            break;
        }

        slen = send (pmwc->fd, snd_buf, snd_size, 0);
        if (slen <= 0)
        {
            PRINTF ("ERROR: %d send msg to '%s` %d, %s\n", pmwc->fd, name, errno, strerror(errno));
            break;
        }

        {
            int rlen = 0;
            char rbuf[16] = "";
            int* pi = NULL;
            static char* prbuf = NULL;
            int nlen = 0;

            rlen = recv (pmwc->fd, rbuf, MW_FIRST_READ_SIZE, 0);
            if (rlen <= 0)
            {
                if (rlen == 0) {
                    rc = 0;
                    close (pmwc->fd);
                    pmwc->fd = -1;
                }
                PRINTF ("ERROR: %d recv msg %d, %s\n", pmwc->fd, errno, strerror(errno));
                break;
            }

            pi = (int*) rbuf;
            if (htonl(*pi) != MW_FIELD_SYS_MAGIC_ID)
            {
                PRINTF ("recv unknown data\n");
                break;
            }
            pi = (int*) (rbuf + 4);
            nlen = ntohl (*pi);
            if (nlen < MW_FIRST_READ_SIZE)
            {
                break;
            }
            else if (nlen == MW_FIRST_READ_SIZE)
            {
                if (p_rcv_size) *p_rcv_size = 0;
                rc = 0;
                break;
            }

            if (prbuf == NULL && (prbuf = malloc (MW_BIG_BUF_SIZE)) == NULL)
            {
                PRINTF ("out of memory\n");
                break;
            }
            /*memset (prbuf, 0, MW_BIG_BUF_SIZE);*/

            memcpy (prbuf, rbuf, MW_FIRST_READ_SIZE);

            {
                int rt1 = 0;
                /*rlen = MW_FIRST_READ_SIZE;*/
                while (rlen < nlen)
                {
                    if ((rt1 = recv (pmwc->fd, prbuf + rlen, nlen - rlen, 0)) <= 0)
                    {
                        PRINTF ("WARN: %d recv rc %d, %d, %s\n", pmwc->fd, rt1, errno, strerror(errno));
                        break;
                    }
                    rlen += rt1;
                }
                /*PRINTF (" >>>> recv %d-byte, need recv %d\n", rlen, nlen);*/
                if (rt1 <= 0) /* FIXME: =? */
                {
                    if (rt1 == 0) {
                        close (pmwc->fd);
                        pmwc->fd = -1;
                    }
                    break;
                }
                rt1 = 0;
                mw_get (prbuf, 0, MW_FIELD_SYS_RETURN_CODE, (char*) &rt1, NULL);
                if (pp_rcv_buf)
                {
                    *pp_rcv_buf = prbuf;
                    if (p_rcv_size) *p_rcv_size = nlen;
                }
                else
                {
                    free (prbuf);
                    prbuf = NULL;
                }

                rc = rt1;
            }
        }
    } while (0);

    if (snd_buf_need_free) {
        free (snd_buf);
        snd_buf = NULL;
    }

    mwc_unlock ();

    return rc;
}

extern int mw_lock_init(pthread_mutex_t* p_mutex);
extern int mw_lock(pthread_mutex_t* p_mutex);
extern int mw_unlock(pthread_mutex_t* p_mutex);

static pthread_mutex_t client_mutex;
static int flag_client_mutext_inited = 0;

int mwc_lock_init(void)
{
    if (flag_client_mutext_inited == 0) {
        if (mw_lock_init(&client_mutex) != 0) {
            return -1;
        }
        flag_client_mutext_inited = 1;
    }
    return 0;
}

int mwc_lock(void)
{
    return mw_lock(&client_mutex);
}

int mwc_unlock(void)
{
    return mw_unlock(&client_mutex);
}

