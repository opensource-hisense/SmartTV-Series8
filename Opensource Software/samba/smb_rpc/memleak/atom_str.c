#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <fcntl.h>
#include <pthread.h>

/*#define NO_AXM*/
#include "atom_str.h"

struct htable {
    struct htable *next;
    int len;
    int refcnt;
    char *str;
};

/*static struct htable *buckets[23] = {NULL};*/
#define HASH_SIZE 257
static struct htable *buckets[HASH_SIZE] = {NULL};
static unsigned int rt[256] = {0};

/*static pthread_mutex_t p_mutex;
static int mtx_inited = 0;*/

extern void L_LOCK (void);
extern void L_UNLOCK (void);
static void MEM_LOCK (void)
{
#if 0
    if (mtx_inited == 0)
    {
        pthread_mutexattr_t t_attr_mx;
        if (pthread_mutexattr_init (&t_attr_mx) != 0)
        {
            return;
        }
        if (pthread_mutexattr_settype (&t_attr_mx, PTHREAD_MUTEX_RECURSIVE) != 0)
        {
            pthread_mutexattr_destroy (&t_attr_mx);
            return;
        }

        if (pthread_mutex_init (&p_mutex, &t_attr_mx) != 0)
        {
            pthread_mutexattr_destroy (&t_attr_mx);
            return;
        }
        pthread_mutexattr_destroy (&t_attr_mx);
        mtx_inited = 1;
    }
    pthread_mutex_lock (&p_mutex);
#endif
    L_LOCK();
}

static void MEM_UNLOCK (void)
{
    /*pthread_mutex_unlock (&p_mutex);*/
    L_UNLOCK();
}

int atom_init (int r)
{
    /*time_t t = time (NULL);*/
    int i;

#if 1
    srand (time(NULL)); /* 85, 91, 93 */
#endif
    for (i=0;i<256;i++)
    {
        rt[i] = rand();
        /*rt[i] = rand (0x00FFFFFF);*/
    }

    if (r) return 0; /* dummy */
    return 0;
}

static int int_mem = 0;

void _adm (int len)
{
    MEM_LOCK ();
    int_mem += len;
    MEM_UNLOCK ();
}

static int _mms (void* data, int a, const char* f, int line)
{
    if (!data) return 0;

    {
    long long int *pl = (long long int*) (((const char*) data) - 8);
    int *l = (int*) (((unsigned char*) pl) + 4);
    char *c = (char*) l;
    char c1 = (*c & 0xF0) >> 4;
    char c2 = (*c & 0x0F);
    int rc = *l - c1 - c2;
    /*if (f && rc > 0) fprintf (stderr, ">> 0x%08x ==> len: %c%-7d %s.%d\n", (int) data, a ? '+' : '-', rc, f, line);*/
    if (a || f || line)
        return rc;
    return rc;
    }
}

void _aam (void* p, const char* f, int line)
{
    MEM_LOCK ();
    int_mem += _mms (p, 1, f, line);
    MEM_UNLOCK ();
}

void _xxm (void* p, const char* f, int line)
{
    MEM_LOCK ();
    int_mem -= _mms (p, 0, f, line);
    MEM_UNLOCK ();
}

int _cm (void)
{
    return int_mem;
}

struct htable* _atom_search (const char *str, int len, int *hv);

extern int _smrec(int v);
#define SMREC(x) int __o = _smrec((x))
#define RMREC() _smrec(__o)
const char *atom_new (const char *str, int len)
{
    int h;
    struct htable *pht;

    MEM_LOCK ();
    pht = _atom_search (str, len, &h);
    if (pht != NULL)
    {
        ++pht->refcnt;
        MEM_UNLOCK ();
        return pht->str;
    }

    /* new */
    SMREC(1);
    pht = (struct htable*) malloc (sizeof (struct htable) + len + 1);
    RMREC();
    if (pht == NULL)
    {
        MEM_UNLOCK ();
        return NULL;
    }
    _am (pht);

    pht->next = buckets[h];
    pht->len = len;
    pht->str = (char*)(pht + 1);
    strncpy (pht->str, str, len);
    pht->str[len] = '\0';
    pht->refcnt = 1;

    buckets[h] = pht;

    MEM_UNLOCK ();

    return pht->str;
}

void atom_free (const char *str)
{
    int h;
    struct htable *pht;
    struct htable *p, *lp;

    MEM_LOCK ();
    if ((pht = _atom_search (str, strlen (str), &h)) == NULL)
    {
        MEM_UNLOCK ();
        return;
    }

    if (--pht->refcnt > 0)
    {
        MEM_UNLOCK ();
        return;
    }

    lp = NULL;
    for (p = buckets[h]; p != NULL; p = p->next)
    {
        if (p == pht)
        {
            if (lp == NULL)
                buckets[h] = p->next;
            else
                lp->next = p->next;
            _xm (p);
            SMREC(1);
            free (p);
            RMREC();

            MEM_UNLOCK ();
            return;
        }
        lp = p;
    }
    MEM_UNLOCK ();

    printf ("Oops!!\n");
}

void atom_reset (void)
{
    int ht_cnt = HASH_SIZE; /*sizeof (buckets) / sizeof (buckets[0]);*/
    int i;

    MEM_LOCK ();
    for (i = 0; i < ht_cnt; i++)
    {
        struct htable *pt, *p;
        for (pt = buckets[i]; pt != NULL; )
        {
            p = pt;
            pt = pt->next;
            _xm (p);
            SMREC(1);
            free (p);
            RMREC();
        }
        buckets[i] = NULL;
    }
    MEM_UNLOCK ();
}

const char *atom_str (const char *str)
{
    return atom_new (str, strlen(str));
}

int _atom_hvalue (const char *str, int len)
{
    int i = 0, h = 0;

    for (h = 0, i = 0; i < len; i++)
    {
        /*h = (h<<1) + rt[(unsigned char)str[i]];*/
        h += str[i];
    }

    return h;
}

struct htable* _atom_search (const char *str, int len, int *hv)
{
    int h = _atom_hvalue (str, len) % HASH_SIZE; /*(sizeof (buckets) / sizeof (buckets[0]));*/
    struct htable *pht;

    if (hv != NULL)
        *hv = h;

    if ((pht = buckets[h]) == NULL)
    {
        return NULL;
    }

    for ( ; pht != NULL; pht = pht->next)
    {
        if (pht->len == len && strcmp (pht->str, str) == 0)
        {
            return pht;
        }
    }

    return NULL;
}

int _atom_dump (void)
{
    int ht_cnt = HASH_SIZE; /*sizeof (buckets) / sizeof (buckets[0]);*/
    int i, hitem_total = 0, h_min = 0, h_max = 0;

    for (i = 0; i < ht_cnt; i++)
    {
        struct htable *pht = NULL;
        int hitem_cnt = 0;

        for (pht = buckets[i]; pht != NULL; pht = pht->next)
        {
            hitem_cnt++;
        }

        if (i == 0) h_max = h_min = hitem_cnt;
        if (hitem_cnt > h_max) h_max = hitem_cnt;
        if (hitem_cnt < h_min) h_min = hitem_cnt;

        hitem_total += hitem_cnt;

        printf ("- atom_str.buckets[%d]: %d\n", i, hitem_cnt);
    }
    printf ("- atom_str total buckets size: %d (%d,%d)\n", hitem_total, h_min, h_max);

    return 0;
}

#if 0
int _atom_ltoa (unsigned int i, char *is, int len)
{
    char s[100] = "";
    char *ps = s + sizeof s;
    unsigned int j = i;

#ifdef LONG_MIN
    if ((int)i == LONG_MIN)
        j = LONG_MAX + 1UL;
#else
    if (i == 0xFFFFFFFF)
        j = 0xFFFFFFFF + 1UL;
#endif
    /*else if (i < 0)
        j = -i;*/

    do {
        if (ps > s) {
            *--ps = (j%10) + '0';
        }
    } while ((j = j/10) != 0);

    /*if (i < 0)
        *--ps = '-';*/

    len = sizeof (s) - (ps - s) > (unsigned int)len ? len : (int)(sizeof (s) - (ps - s));

    strncpy (is, ps, len);
    is[len] = '\0';

    return 0;
}

const char *atom_int (unsigned int n)
{
    char s[100] = "";
    _atom_ltoa (n, s, sizeof (s));
    return atom_new (s, strlen(s));
}

unsigned int _atom_time (void)
{
    struct timeval tv = {0, 0};

    gettimeofday (&tv, NULL);

    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

void _atom_test (int seed, int cnt)
{
    int i;
    unsigned int t1, t2;

    t1 = _atom_time ();

    atom_init (seed);

    for (i = 0; i < cnt; i++) atom_int (i);

    t2 = _atom_time ();
    printf ("create time: %d msec.\n", t2 - t1);

    _atom_dump ();

    t1 = _atom_time ();
    for (i = 0; i < cnt; i++) atom_int (i);
    t2 = _atom_time ();
    printf ("search time: %d msec.\n", t2 - t1);

    t1 = _atom_time ();
    for (i = 0; i < cnt; i++)
    {
        char s[100] = "";
        _atom_ltoa (i, s, sizeof (s));
        atom_free (s);
    }
    t2 = _atom_time ();
    printf ("free time: %d msec.\n\n", t2 - t1);

    atom_reset ();
}

int main (int argc, char **argv)
{
    int i, cnt;

#if 1
    if (argc < 2)
    {
        fprintf (stderr, "Usage: %s hcount\n", argv[0]);
        exit (-1);
    }

    cnt = atoi (argv[1]);
#else
    cnt = 100000;
    for (i = 0; i < 100; i++)
#endif
    {
        printf ("seed: %d, cnt: %d\n", i, cnt);
        _atom_test (i, cnt);
    }

    return 0;
}
#endif

