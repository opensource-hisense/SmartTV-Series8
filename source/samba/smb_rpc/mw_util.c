#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>

#include <arpa/inet.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>

#include "mylist.h"
#include "mw_common.h"

extern int _get_ms (void);
#define PRINTF(...) do { unsigned int ct = _get_ms(); printf ("[mw.util] %u.%03u, %s, %s.%d >>> ", ct/1000, ct%1000, __FILE__, __FUNCTION__, __LINE__); printf(__VA_ARGS__); } while (0)

static int _mw_len (int len)
{
    int rlen = (len + 3) / 4;
    return rlen * 4;
}

void* mw_nodelay (void* ptr)
{
#if 1
    int magicid = 0;
    int nsize = 0;
    char* pdata = (char*) ptr;
    int delay_free = 0;
    int *pdelay = 0;

    if (pdata == NULL) return NULL;

    magicid = *((int*) pdata);
    nsize = *((int*) ((char*)pdata + 4));
    delay_free = *((int*) ((char*)pdata + 8));
    pdelay = (int*) ((char*)pdata + 12);

    magicid = ntohl (magicid);
    nsize = ntohl (nsize);
    delay_free = ntohl (delay_free);

    if (magicid != MW_FIELD_SYS_MAGIC_ID) return NULL;
    if (nsize <= 0 || nsize > 50*1024*1024) return NULL;

    if (nsize >= 16 && delay_free == MW_FIELD_SYS_DELAY_FREE && ntohl (*pdelay) == 1)
    {
        *pdelay = 0;
    }

    return ptr;
#else
    return NULL;
#endif
}

void* mw_memset (void* ptr)
{
    int magicid = 0;
    int nsize = 0;
    char* pdata = (char*) ptr;
    int delay_free = 0;
    int delay = 0;

    if (pdata == NULL) return NULL;

    magicid = *((int*) pdata);
    nsize = *((int*) ((char*)pdata + 4));
    delay_free = *((int*) ((char*)pdata + 8));
    delay = *((int*) ((char*)pdata + 12));

    magicid = ntohl (magicid);
    nsize = ntohl (nsize);
    delay_free = ntohl (delay_free);
    delay = ntohl (delay);

    /*PRINTF ("magicid: 0x%08x, size: %d, dfree: 0x%08x, delay: %d\n", magicid, nsize, delay_free, delay);*/
    if (magicid != MW_FIELD_SYS_MAGIC_ID) return NULL;
    if (nsize <= 0 || nsize > 50*1024*1024) return NULL;

    if (nsize > 16 && delay_free == MW_FIELD_SYS_DELAY_FREE && delay == 1)
    {
        memset ((char*)pdata + 16, 0, nsize - 16);
    }
    else
    {
        memset ((char*)pdata + 8, 0, nsize - 8);
    }
    return ptr;
}

void* mw_alloc (int size)
{
    char* pdata = NULL;
    int nsize = 0;
    int *pi = NULL;

    if (size <= 0) return NULL;

    nsize = _mw_len (size);
    pdata = (char*) malloc (nsize);
    if (pdata == NULL) return NULL;

    memset (pdata, 0, nsize);

    pi = (int*) pdata;
    *pi = htonl (MW_FIELD_SYS_MAGIC_ID);

    pi = (int*) (pdata + 4);
    *pi = htonl (nsize);

    /*PRINTF ("allocate memory %d-byte 0x%08x\n", size, (int) pdata);*/
    return pdata;
}

int mw_append (void* data, int field_id, const char* presult, int len);
void* mw_alloc_delay_free (int size)
{
    char* pdata = mw_alloc (size);
    int delay = 1;

    if (pdata == NULL) return NULL;

    mw_append (pdata, MW_FIELD_SYS_DELAY_FREE, (char*) &delay, 0);

    return pdata;
}

static int _is_valid_data (const void* data)
{
    char *d = (char*) data;
    int *pi = NULL;

    if (d == NULL) return 0;

    pi = (int*) d;

    if (ntohl (*pi) == MW_FIELD_SYS_MAGIC_ID)
    {
        return 1;
    }
    return 0;
}

void mw_free (void* ptr)
{
    if (ptr)
    {
        int* pm = (int*) ptr;
        if (!_is_valid_data (ptr))
        {
            _prn_mem (ptr, 32);
            PRINTF ("invalid data: 0x%08x\n", (int)ptr);
            return;
        }
        *pm = 0;
        free (ptr);
    }
}

int _mws_need_buf_size (const char* buf)
{
    int *pi = NULL;

    if (buf == NULL) return -1;

    pi = (int*) (buf + 4);
    return ntohl (*pi);
}

int mw_get (const void* data, int idx, int field_id, char* presult, int *size)
{
    int nlen = 0;
    int ridx = 0;
    char* cur = (char*) data;

    /*PRINTF ("index: %d, fld_id: 0x%08x (%d) from data 0x%08x\n", idx, field_id, field_id, (int) data);*/
    if (data == NULL || presult == NULL) return -1;
    if (idx < 0) return -1;
    if (field_id < 0x00010000 && field_id > 0x003FFFFF) return -1;

    if (!_is_valid_data (data)) return -1;

    nlen = _mws_need_buf_size (data);

    cur += 8;

    while ((cur - (char*)data) < nlen)
    {
        int* pfid = (int*) cur;
        int fid = htonl (*pfid);

        if (fid < 0x00010000 && fid > 0x003FFFFF) break;

        if ((cur - (char*)data) >= nlen) break;

        /* */
        {
            char* ps = NULL;
            int* pi = NULL;
            void* pdata = NULL;
            int dlen = 0;

            if (fid >= 0x00010000 && fid < 0x00020000) /* sys */
            {
                if (fid == MW_FIELD_SYS_SERVICE_NAME) /* 0x00010001 */
                {
                    ps = (char*) (cur + 4);
                }
                else if (fid == MW_FIELD_SYS_RETURN_CODE) /* 0x00010002 */
                {
                    pi = (int*) (cur + 4);
                }
                else if (fid == MW_FIELD_SYS_DELAY_FREE) /* 0x00010003 */
                {
                    pi = (int*) (cur + 4);
                }
                else
                {
                    break;
                }
            }
            else if (fid >= 0x00100000 && fid < 0x00200000) /* long */
            {
                pi = (int*) (cur + 4);
            }
            else if (fid >= 0x00200000 && fid < 0x00300000) /* string */
            {
                ps = (char*) (cur + 4);
            }
            else if (fid >= 0x00300000 && fid < 0x00400000) /* data */
            {
                int* pdlen = (int*) (cur + 4);
                dlen = ntohl (*pdlen);
                if (dlen < 0 || dlen >= nlen) break;
                pdata = cur + 8;
            }
            else
            {
                break;
            }

            if (pi)
            {
                cur += 8;
                if (((cur - (char*)data) - 2) > nlen) break;
                if (fid == field_id && ridx == idx)
                {
                    *(int*)presult = ntohl (*pi);
                    if (size) *size = 4;
                }
            }
            else if (ps)
            {
                int slen = strlen(ps);
                cur += 4 + _mw_len (slen);
                if (((cur - (char*)data) - 2) > nlen) break;
                if (fid == field_id && ridx == idx)
                {
                    if (size)
                    {
                        if (*size > 0)
                        {
                            memcpy (presult, ps, *size > slen+1 ? slen+1 : *size);
                            if (*size > slen+1) *size = slen+1;
                        }
                    }
                    else
                    {
                        memcpy (presult, ps, slen+1);
                    }
                }
            }
            else if (pdata)
            {
                cur += 8 + _mw_len (dlen);
                if (((cur - (char*)data) - 2) > nlen) break;
                if (fid == field_id && ridx == idx)
                {
                    if (size)
                    {
                        if (*size > 0)
                        {
                            memcpy (presult, pdata, *size > dlen ? dlen : *size);
                            if (*size > dlen) *size = dlen;
                        }
                    }
                    else
                    {
                        memcpy (presult, pdata, dlen);
                    }
                }
            }
            else
            {
                break;
            }

            if (fid == field_id)
            {
                if (ridx == idx)
                {
                    return 0;
                }
                ridx++;
            }
        }
    }

    return -1;
}

int mw_append (void* data, int field_id, const char* presult, int len)
{
    int nlen = 0;
    char* cur = (char*) data;

    if (data == NULL || presult == NULL) return -1;
    if (len < 0) return -1;
    if (field_id < 0x00010000 && field_id > 0x003FFFFF) return -1;

    if (!_is_valid_data (data)) return -1;

    nlen = _mws_need_buf_size (data);

    cur += 8;

    while ((cur - (char*)data) < nlen)
    {
        int* pfid = (int*) cur;
        int fid = htonl (*pfid);

        if (fid != 0 && fid < 0x00010000 && fid > 0x003FFFFF) break;

        if ((cur - (char*)data) >= nlen) break;

        if (fid == 0)
        {
            char* ps = NULL;
            int* pi = NULL;
            void* pdata = NULL;
            /*int last_len = 0;*/

            if (field_id >= 0x00010000 && field_id < 0x00020000) /* sys */
            {
                if (field_id == MW_FIELD_SYS_SERVICE_NAME) /* 0x00010001 */
                {
                    ps = (char*) presult;
                }
                else if (field_id == MW_FIELD_SYS_RETURN_CODE) /* 0x00010002 */
                {
                    pi = (int*) presult;
                }
                else if (field_id == MW_FIELD_SYS_DELAY_FREE) /* 0x00010003 */
                {
                    pi = (int*) presult;
                }
                else
                {
                    break;
                }
            }
            else if (field_id >= 0x00100000 && field_id < 0x00200000) /* long */
            {
                pi = (int*) presult;
            }
            else if (field_id >= 0x00200000 && field_id < 0x00300000) /* string */
            {
                ps = (char*) presult;
            }
            else if (field_id >= 0x00300000 && field_id < 0x00400000) /* data */
            {
                pdata = (void*) presult;
            }
            else
            {
                break;
            }

            if (pi)
            {
                int *pi2 = (int*) (cur + 4);
                if (((cur - (char*)data) + 8) > nlen) break;
                *pfid = htonl (field_id);
                *pi2 = htonl (*pi);
                /*last_len = 8;*/
            }
            else if (ps)
            {
                int slen = strlen(ps);
                if (((cur - (char*)data) + 4 + _mw_len(slen)) > nlen) break;
                *pfid = htonl (field_id);
                memcpy (cur + 4, ps, slen);
                /*last_len = 4 + _mw_len (slen);*/
            }
            else if (pdata)
            {
                int* pdlen = (int*) (cur + 4);
                if (((cur - (char*)data) + 8 + _mw_len(len)) > nlen) break;
                *pfid = htonl (field_id);
                *pdlen = htonl (len);
                if (len > 0) memcpy (cur + 8, pdata, len);
                /*last_len = 8 + _mw_len (len);*/
            }
            else
            {
                break;
            }

#if 0
            /* real size */
            {
                int real_len = (cur - (char*) data) + last_len;
                int* rsize = (int*) ((char*)data + 4);
                PRINTF ("buff size: %d (%d), real length: %d\n", nlen, ntohl (*rsize), real_len);
                /**rsize = htonl (real_len);*/
            }
#endif

            return 0;
        }

        /* */
        {
            char* ps = NULL;
            int* pi = NULL;
            void* pdata = NULL;
            int dlen = 0;

            if (fid >= 0x00010000 && fid < 0x00020000) /* sys */
            {
                if (fid == MW_FIELD_SYS_SERVICE_NAME) /* 0x00010001 */
                {
                    ps = (char*) (cur + 4);
                }
                else if (fid == MW_FIELD_SYS_RETURN_CODE) /* 0x00010002 */
                {
                    pi = (int*) (cur + 4);
                }
                else if (fid == MW_FIELD_SYS_DELAY_FREE) /* 0x00010003 */
                {
                    pi = (int*) (cur + 4);
                }
                else
                {
                    break;
                }
            }
            else if (fid >= 0x00100000 && fid < 0x00200000) /* long */
            {
                pi = (int*) (cur + 4);
            }
            else if (fid >= 0x00200000 && fid < 0x00300000) /* string */
            {
                ps = (char*) (cur + 4);
            }
            else if (fid >= 0x00300000 && fid < 0x00400000) /* data */
            {
                int* pdlen = (int*) (cur + 4);
                dlen = ntohl (*pdlen);
                if (dlen < 0 || dlen >= nlen) break;
                pdata = cur + 8;
            }
            else
            {
                break;
            }

            if (pi)
            {
                cur += 8;
            }
            else if (ps)
            {
                int slen = strlen(ps);
                cur += 4 + _mw_len (slen);
            }
            else if (pdata)
            {
                cur += 8 + _mw_len (dlen);
            }
            else
            {
                break;
            }
            if (((cur - (char*)data) - 2) > nlen) break;
        }
    }

    return -1;
}

int _mw_get_real_size (const void* data)
{
    int nlen = 0;
    char* cur = (char*) data;

    if (data == NULL) return -1;

    if (!_is_valid_data (data)) return -1;

    nlen = _mws_need_buf_size (data);

    cur += 8;

    while ((cur - (char*)data) < nlen)
    {
        int* pfid = (int*) cur;
        int fid = htonl (*pfid);

        if (fid == 0x0)
        {
            return cur - (char*)data;
        }

        if (fid < 0x00010000 && fid > 0x003FFFFF) break;

        if ((cur - (char*)data) >= nlen) break;

        /* */
        {
            char* ps = NULL;
            int* pi = NULL;
            void* pdata = NULL;
            int dlen = 0;

            if (fid >= 0x00010000 && fid < 0x00020000) /* sys */
            {
                if (fid == MW_FIELD_SYS_SERVICE_NAME) /* 0x00010001 */
                {
                    ps = (char*) (cur + 4);
                }
                else if (fid == MW_FIELD_SYS_RETURN_CODE) /* 0x00010002 */
                {
                    pi = (int*) (cur + 4);
                }
                else if (fid == MW_FIELD_SYS_DELAY_FREE) /* 0x00010003 */
                {
                    pi = (int*) (cur + 4);
                }
                else
                {
                    break;
                }
            }
            else if (fid >= 0x00100000 && fid < 0x00200000) /* long */
            {
                pi = (int*) (cur + 4);
            }
            else if (fid >= 0x00200000 && fid < 0x00300000) /* string */
            {
                ps = (char*) (cur + 4);
            }
            else if (fid >= 0x00300000 && fid < 0x00400000) /* data */
            {
                int* pdlen = (int*) (cur + 4);
                dlen = ntohl (*pdlen);
                if (dlen < 0 || dlen >= nlen) break;
                pdata = cur + 8;
            }
            else
            {
                break;
            }

            if (pi)
            {
                cur += 8;
                if (((cur - (char*)data) - 2) > nlen) break;
            }
            else if (ps)
            {
                int slen = strlen(ps);
                cur += 4 + _mw_len (slen);
                if (((cur - (char*)data) - 2) > nlen) break;
            }
            else if (pdata)
            {
                cur += 8 + _mw_len (dlen);
                if (((cur - (char*)data) - 2) > nlen) break;
            }
            else
            {
                break;
            }
        }
    }

    return -1;
}

int mw_lock_init(pthread_mutex_t* p_mutex)
{
    pthread_mutexattr_t t_attr_mx;

    if (pthread_mutexattr_init (&t_attr_mx) != 0)
    {
        return -1;
    }

#ifdef _GNU_SOURCE
    if (pthread_mutexattr_settype (&t_attr_mx, PTHREAD_MUTEX_RECURSIVE) != 0)
    {
        pthread_mutexattr_destroy (&t_attr_mx);
        return -1;
    }
#endif
    if (pthread_mutex_init (p_mutex, &t_attr_mx) != 0)
    {
        pthread_mutexattr_destroy (&t_attr_mx);
        return -1;
    }
    pthread_mutexattr_destroy (&t_attr_mx);
    return 0;
}

int mw_lock(pthread_mutex_t* p_mutex)
{
    return pthread_mutex_lock (p_mutex);
}

int mw_unlock(pthread_mutex_t* p_mutex)
{
    return pthread_mutex_unlock (p_mutex);
}

#include <sys/ipc.h>
#include <sys/sem.h>

int mw_sem_open(const char* path)
{
    sem_t* sem = sem_open (path+4, O_CREAT);
    if (sem == SEM_FAILED) {
        PRINTF ("ERROR: semaphore %s create error %d, %s\n", path, errno, strerror(errno));
    }
    return (int) sem;
}

int mw_sem_close(int fd)
{
    sem_t* sem = (sem_t*)fd;

    if (sem == SEM_FAILED || fd == 0) return -1;

    return sem_close(sem);
}

int mw_sem_lock(int sem)
{
    struct timespec timeo = {5, 0};
    int rc = 0;
    int num_retry = 0;

    if (sem == 0) return -1;

    while ((rc = sem_timedwait((sem_t*)sem, &timeo)) != 0) {
        if (errno != EAGAIN) {
            return rc;
        }
        if (++num_retry > 3) {
            sem_post((sem_t*)sem);
        }
        PRINTF ("WARN: sem_wait 0x%08x timeout\n", sem);
    }

    return 0;
}

int mw_sem_unlock(int sem)
{
    if (sem == 0) return -1;
    return sem_post((sem_t*)sem);
}

#include <execinfo.h>
void _prn_bt(void)
{
    int j, nptrs;
    void *buffer[100];
    char **strings;

    nptrs = backtrace(buffer, 10);
    fprintf(stderr, "\n ===== backtrace() returned %d addrs ===== \n", nptrs);

    /* The call backtrace_symbols_fd(buffer, nptrs, STDOUT_FILENO)
     * would produce similar output to the following: */

    strings = backtrace_symbols(buffer, nptrs);
    if (strings == NULL) {
        /*perror("backtrace_symbols");*/
        return;
    }

    for (j = 0; j < nptrs; j++)
        fprintf(stderr, "%s\n", strings[j]);

    free(strings);
}

