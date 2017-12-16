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

/*#define NO_AXM*/
#include "atom_str.h"
#include "x_lnk_list.h"
#include "memlist.h"

static volatile int memory_record_flag_g = 0;

int _smrec(int v)
{
    int o = memory_record_flag_g;
    memory_record_flag_g = v;
    return o;
}
#define SMREC(x) int __o = _smrec((x))
#define RMREC() _smrec(__o)

int _gmrec(void)
{
    return memory_record_flag_g;
}

#ifdef NET_MEMLEAK_CK_ALL_MEMORY_HOOK /* mem hook */
void* _net_mem_alloc (const char*, unsigned int, size_t);
void* _net_mem_realloc (const char*, unsigned int, void*, size_t);
void _net_mem_free (const char*, unsigned int, void*);

/* memory hook */
#include <malloc.h>

static void my_init_hook(void);
static void* my_malloc_hook(size_t, const void*);
static void* my_realloc_hook(void*, size_t, const void*);
static void my_free_hook(void*, const void*);

void (*__malloc_initialize_hook)(void) = my_init_hook;

static void* (*old_malloc_hook)(size_t, const void*);
static void* (*old_realloc_hook)(void*, size_t, const void*);
static void (*old_free_hook)(void*, const void*);

void L_LOCK (void);
void L_UNLOCK (void);
static void my_init_hook(void)
{
    L_LOCK();
    old_malloc_hook = __malloc_hook;
    old_free_hook = __free_hook;
    old_realloc_hook = __realloc_hook;
    __malloc_hook = my_malloc_hook;
    __realloc_hook = my_realloc_hook;
    __free_hook = my_free_hook;
    L_UNLOCK();
}

static void* my_malloc_hook (size_t size, const void *caller)
{
    char file[32] = "";
    void *result = NULL;
    if (size == 0) return NULL;

    L_LOCK();

    __malloc_hook = old_malloc_hook;

    if (_gmrec() == 0) {
        sprintf(file, "malloc:%p", caller);
        result = _net_mem_alloc (file, 0, size);
    } else { /* internal memleak */
        result = malloc(size);
    }

    old_malloc_hook = __malloc_hook;

    __malloc_hook = my_malloc_hook;

    L_UNLOCK();

    /*printf ("malloc (%u) returns %p -- %s\n", (unsigned int) size, result, file);*/
    return result;
}

static void* my_realloc_hook (void* ptr, size_t size, const void *caller)
{
    char file[32] = "";
    void *result = NULL;

    L_LOCK();

    __realloc_hook = old_realloc_hook;

    if (_gmrec() == 0) {
        sprintf(file, "realloc:%p", caller);
        result = _net_mem_realloc (file, 0, ptr, size);
    } else { /* internal memleak */
        result = realloc(ptr, size);
    }

    old_realloc_hook = __realloc_hook;

    __realloc_hook = my_realloc_hook;

    L_UNLOCK();

    /*printf ("realloc (%u) returns %p -- %s\n", (unsigned int) size, result, file);*/
    return result;
}

static void my_free_hook (void *ptr, const void *caller)
{
    char file[32] = "";

    L_LOCK();

    __free_hook = old_free_hook;

    if (_gmrec() == 0) {
        sprintf(file, "free:%p", caller);
        _net_mem_free (file, 0, ptr);
    } else {
        free(ptr);
    }

    old_free_hook = __free_hook;

    __free_hook = my_free_hook;

    L_UNLOCK();
    /*printf ("freed pointer %p -- %s\n", ptr, file);*/
}
#endif
/* memory hook end */

/*#define NET_MEMSET_CHECK*/

static unsigned int ui4_max_size = 0;
static unsigned int ui4_max_cnt = 0;

static unsigned int ui4_all_size = 0;
static unsigned int ui4_all_cnt = 0;

typedef struct _NET_MEM_LISTS
{
    struct _NET_MEM_LISTS *pt_next;
    const char*        ps_file;
    unsigned int             ui4_line;
    const void*        pv_data;
    size_t             z_size;
    unsigned int             tick;
    char*              bt;
    DLIST_ENTRY_T(_NET_MEM_LISTS) t_link;
#ifdef NET_MEMSET_CHECK
    DLIST_ENTRY_T(_NET_MEM_LISTS) t_link2;
#endif
} NET_MEM_LISTS;

/*static struct _NET_MEM_LISTS *mem_buckets[251] = {NULL};*/
/*{67, 257, 521, 1031, 2053, 4093 }*/
#define HASH_SIZE 257
static struct _NET_MEM_LISTS *mem_buckets[HASH_SIZE] = {NULL};

static NET_MEM_LISTS * _net_mem_search (const void *pv_data, int *hv);

/**/
struct st_self {
    char *file;
    unsigned int ui4_line;
    const void *pv_data;
    unsigned int ui4_len;
};

/*#define _NET_MEMLEAK_S_MEMLEAK*/

#ifndef _NET_MEMLEAK_S_MEMLEAK

#define CR_SELF(x, y) 
#define _dl_self(x)

#else /* _NET_MEMLEAK_S_MEMLEAK */

#include "os/myhash.h"

static handle_t h_self = null_handle;
static unsigned int ui4_self = 0;

#define CR_SELF(x, y) _cr_self (__FILE__, __LINE__, (x), (y))

void _init_self (void)
{
#if 0
    mm_list_create (&h_self);
#else
    mm_hash_create (&h_self, 2000, NULL);
#endif
}

static int _cr_self (const char *file, unsigned int ui4_line, const void *pv_data, unsigned int ui4_len)
{
    struct st_self *pt_self = malloc (sizeof (*pt_self));
    if (pt_self != NULL)
    {
        _am (pt_self);
        pt_self->file = strdup (file);
        _am (pt_self->file);
        pt_self->ui4_line = ui4_line;
        pt_self->pv_data = pv_data;
        pt_self->ui4_len = ui4_len;
        ui4_self += ui4_len;

#if 0
        mm_list_add (h_self, pt_self);
#else
        mm_hash_add (h_self, pt_self, pt_self->pv_data);
#endif
        return 0;
    }
    return -1;
}

static int _cmp_self (const void *pv_self, const void *pv_data)
{
    struct st_self* pt_self = (struct st_self*) pv_self;
    if (pt_self->pv_data == pv_data)
        return 0;
    return -1;
}

static void _dl_self (const void *pv_data)
{
#if 0
    void *pv_s = mm_list_search2 (h_self, pv_data, _cmp_self);
#else
    void *pv_s = mm_hash_search2 (h_self, pv_data, _cmp_self);
#endif
    if (pv_s)
    {
        struct st_self* pt_self = (struct st_self*) pv_s;
        ui4_self -= pt_self->ui4_len;
#if 0
        mm_list_delete (h_self, pv_s);
#else
        mm_hash_delete (h_self, pt_self, pt_self->pv_data);
#endif
        _xm (pt_self->file);
        free (pt_self->file);
        _xm (pt_self);
        free (pt_self);
    }
}

static int i4_g_idx_self = 0;
static int _mp_self (void *pv_data, void *pv_arg)
{
    struct st_self *pt_self = (struct st_self*) pv_data;
    if (pv_data)
    {
        printf ("%05d: %s:%u  %p len: %d\n", i4_g_idx_self++, pt_self->file, pt_self->ui4_line, pt_self->pv_data, pt_self->ui4_len);
    }
    if (pv_arg) return 0; /* dummy code */
    return 0;
}
#endif /* _NET_MEMLEAK_S_MEMLEAK */

void _pr_mem_self (void)
{
#ifdef _NET_MEMLEAK_S_MEMLEAK
    i4_g_idx_self = 0;
    printf ("-- all self malloc size: %u\n", ui4_self);
#if 0
    mm_list_map (h_self, _mp_self, NULL);
#else
    mm_hash_map (h_self, _mp_self, NULL);
#endif
#endif /* _NET_MEMLEAK_S_MEMLEAK */
}
/**/

static pthread_mutex_t p_mutex = PTHREAD_MUTEX_INITIALIZER;
static void L_MUTEX_INIT(void)
{
    pthread_mutexattr_t t_attr_mx;

    pthread_mutexattr_init (&t_attr_mx);
    pthread_mutexattr_settype (&t_attr_mx, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init (&p_mutex, &t_attr_mx);

    pthread_mutexattr_destroy (&t_attr_mx);
}

static pthread_once_t once_lock_control = PTHREAD_ONCE_INIT;
void L_LOCK (void)
{
    pthread_once(&once_lock_control, L_MUTEX_INIT);
    pthread_mutex_lock (&p_mutex);
}

void L_UNLOCK (void)
{
    pthread_mutex_unlock (&p_mutex);
}

static NET_MEM_LISTS* _net_mem_search1 (const void *pv_data, unsigned int tick);
int _memleak_check_sum (void)
{
    int ht_cnt = HASH_SIZE; /*sizeof (mem_buckets) / sizeof (mem_buckets[0]);*/
    int i, id;

    id = 0;
    for (i = 0; i < ht_cnt; i++)
    {
        NET_MEM_LISTS *pht = NULL;
        int hitem_cnt = 0;
        int id2 = 0;

        L_LOCK ();
        for (pht = mem_buckets[i]; pht != NULL; pht = pht->pt_next)
        {
            hitem_cnt++;
            if (_net_mem_search1 (pht->pv_data, pht->tick) == NULL)
            {
                printf (" --- memchecksum, %d-th: [%d][%d] %p not in tick_lists, %s:%u sz: %zu, tk: %u\n",
                            id, i, id2, pht->pv_data, pht->ps_file, pht->ui4_line, pht->z_size, pht->tick);
                id++;
            }
            id2++;
        }

        /*printf ("- memleak.buckets[%d]: %d\n", i, hitem_cnt);*/
        L_UNLOCK ();
    }

    printf ("-----------------------------------------------\n");

    return 0;
}

extern int _atom_dump (void);
int _memleak_dump (void)
{
    int ht_cnt = HASH_SIZE; /*sizeof (mem_buckets) / sizeof (mem_buckets[0]);*/
    int i, hitem_total = 0, h_min = 0, h_max = 0;

    L_LOCK ();

    _atom_dump ();

    for (i = 0; i < ht_cnt; i++)
    {
        NET_MEM_LISTS *pht = NULL;
        int hitem_cnt = 0;

        for (pht = mem_buckets[i]; pht != NULL; pht = pht->pt_next)
        {
            hitem_cnt++;
        }

        if (i == 0) h_max = h_min = hitem_cnt;
        if (hitem_cnt > h_max) h_max = hitem_cnt;
        if (hitem_cnt < h_min) h_min = hitem_cnt;

        hitem_total += hitem_cnt;

        printf ("- memleak.buckets[%d]: %d\n", i, hitem_cnt);
    }

    L_UNLOCK ();

    printf ("- memleak total buckets size: %d (%d,%d)\n", hitem_total, h_min, h_max);
    printf ("-----------------------------------------------\n");

    return 0;
}

static int _net_mem_op (unsigned char **ppuac_data, unsigned int *pui4_data_len, unsigned int *pui4_total, const char *ps_send, int i4_len)
{
    SMREC(1);
    if (*ppuac_data == NULL)
    {
        *ppuac_data = (unsigned char*)malloc (*pui4_data_len);
        if (*ppuac_data == NULL)
        {
            return -1;
        }
        memset (*ppuac_data, 0, *pui4_data_len);
        _am (*ppuac_data);
    }
    if ((*pui4_total + i4_len) > *pui4_data_len)
    {
        unsigned char *puac_tmp = (unsigned char*)realloc (*ppuac_data, *pui4_data_len + 512000);
        if (puac_tmp == NULL)
        {
            return -1;
        }
        *pui4_data_len += 512000;
        *ppuac_data = puac_tmp;
        _adm (512000);
        {
            int i = 16;
            unsigned char *pc = puac_tmp - 8;
            printf ("==== oopp: ");
            for (i = 0; i < 16; i++) {
                printf ("%02x ", pc[i]);
            }
            printf ("====\n");
        }
    }
    RMREC();

    memcpy (*ppuac_data + *pui4_total, ps_send, i4_len);
    *pui4_total += i4_len;

    return 0;
}

/* time point index */
/* use radix algo */
typedef DLIST_T(_NET_MEM_LISTS) NET_MEM_LISTS_TYPE;
static void ***r_tick[0xFFFF] = {NULL}; /* tick index */
static unsigned int ui4_first_tick = 0xFFFFFFFF;

#ifdef NET_MEMSET_CHECK
static NET_MEM_LISTS_TYPE ****r_mem = NULL; /* malloc mem first address index */
/*static unsigned int ui4_first_mem = 0xFFFFFFFF;*/

static void _insert_sort_lists2 (NET_MEM_LISTS *pt_mem_new)
{
    unsigned int ui4_org = (unsigned int)pt_mem_new->pv_data;
    int i1, i2, i3;

    ui4_org >>= 4; /* 16 align */

    i1 = (ui4_org >> 16) & 0xFFFF;
    i2 = (ui4_org >> 8) & 0xFF;
    i3 = ui4_org & 0xFF;

    if (r_mem == NULL) {
        /* 16 align */
        r_mem = malloc ((0x10000 >> 4) * sizeof (void ***));
        if (r_mem == NULL) {
            printf (" === memcheck, %s, %s:%u no memory\n", __FUNCTION__, __FILE__, __LINE__);
            return;
        }
        _am (r_mem);
        memset (r_mem, 0, (0x10000 >> 4) * sizeof (void ***));
        CR_SELF (r_mem, (0x10000 >> 4) * sizeof (void ***));
    }

    if (r_mem[i1] == NULL) {
        /*r_mem[i1] = malloc (0x100 * sizeof (void *) + 1);*/
        r_mem[i1] = malloc (0x100 * sizeof (void *));
        if (r_mem[i1] == NULL) {
            printf (" === memcheck, %s, %s:%u no memory\n", __FUNCTION__, __FILE__, __LINE__);
            return;
        }
        _am (r_mem[i1]);
        memset (r_mem[i1], 0, 0x100 * sizeof (void *));
        CR_SELF (r_mem[i1], 0x100 * sizeof (void *));
    }

    if (r_mem[i1][i2] == NULL) {
        r_mem[i1][i2] = malloc (0x100 * sizeof (void *));
        if (r_mem[i1][i2] == NULL) {
            printf (" === memcheck, %s, %s:%u no memory\n", __FUNCTION__, __FILE__, __LINE__);
            return;
        }
        _am (r_mem[i1][i2]);
        memset (r_mem[i1][i2], 0, 0x100 * sizeof (void *));
        CR_SELF (r_mem[i1][i2], 0x100 * sizeof (void *));
    }

    if (r_mem[i1][i2][i3] == NULL) {
        r_mem[i1][i2][i3] = malloc (sizeof (*r_mem[i1][i2][i3]));
        if (r_mem[i1][i2][i3] == NULL) {
            printf (" === memcheck, %s, %s:%u Oops!\n", __FUNCTION__, __FILE__, __LINE__);
            return;
        }
        _am (r_mem[i1][i2][i3]);
        memset (r_mem[i1][i2][i3], 0, sizeof (*r_mem[i1][i2][i3]));
        DLIST_INIT (r_mem[i1][i2][i3]);
        CR_SELF (r_mem[i1][i2][i3], sizeof (*r_mem[i1][i2][i3]));
    }

    DLIST_INSERT_TAIL (pt_mem_new, r_mem[i1][i2][i3], t_link2);
}

static void _delete_sort_lists2 (NET_MEM_LISTS *pt_mem_new)
{
    unsigned int ui4_org = (unsigned int)pt_mem_new->pv_data;
    int i1, i2, i3;
    NET_MEM_LISTS_TYPE *pt_head;
    time_t ts, te;

    ui4_org >>= 4; /* 16 align */

    i1 = (ui4_org >> 16) & 0xFFFF;
    i2 = (ui4_org >> 8) & 0xFF;
    i3 = ui4_org & 0xFF;

    if (r_mem[i1] == NULL ||
        r_mem[i1][i2] == NULL ||
        r_mem[i1][i2][i3] == NULL) {
        printf (" === memcheck, ---- no such item2, mem: %p, mem: %p\n", pt_mem_new->pv_data, pt_mem_new);
        return;
    }

    pt_head = (NET_MEM_LISTS_TYPE*) r_mem[i1][i2][i3];
    DLIST_REMOVE (pt_mem_new, pt_head, t_link2);

    /* check r_mem r_mem[i1]... need free ?? */
    if (DLIST_IS_EMPTY(pt_head)) {
        ts = time (NULL);
        int i;
        _xm (r_mem[i1][i2][i3]);
        free (r_mem[i1][i2][i3]);
        _dl_self (r_mem[i1][i2][i3]);
        r_mem[i1][i2][i3] = NULL;
        for (i = 0; i<= 0xFF; i++) {
            if (r_mem[i1][i2][i] != NULL)
            {
                goto DEL_FINAL;
            }
        }
        _xm (r_mem[i1][i2]);
        free (r_mem[i1][i2]);
        _dl_self (r_mem[i1][i2]);
        r_mem[i1][i2] = NULL;
        for (i = 0; i<= 0xFF; i++) {
            if (r_mem[i1][i] != NULL)
            {
                goto DEL_FINAL;
            }
        }
        _xm (r_mem[i1]);
        free (r_mem[i1]);
        _dl_self (r_mem[i1]);
        r_mem[i1] = NULL;
        for (i = 0; i<= 0xFFFF; i++) {
            if (r_mem[i] != NULL)
            {
                goto DEL_FINAL;
            }
        }
        _xm (r_mem);
        free (r_mem);
        _dl_self (r_mem);
        r_mem = NULL;
    }
    else
    {
        return;
    }
DEL_FINAL:
    te = time (NULL);
    {
        time_t t;
        t = te -ts;
        if (t > 2)
        {
            printf ("[R_MEM] %lu\n", t);
        }
    }
    return;
#if 0
    if (DLIST_IS_EMPTY(pt_head)) {
        r_mem[i1][i2][i3] = NULL;
        free (pt_head);
        printf (" ---- delete %d.%d.%d\n", i1, i2, i3);
    }
#endif
}
#endif /* NET_MEMSET_CHECK */

static void _insert_sort_lists (NET_MEM_LISTS *pt_mem_new)
{
    unsigned int ui4_org = pt_mem_new->tick;
    int i1, i2, i3;
    NET_MEM_LISTS_TYPE *pt_head;

    if (pt_mem_new == NULL) {
        printf (" === memcheck, %s, %s:%u new memory is NULL\n", __FUNCTION__, __FILE__, __LINE__);
        return;
    }

    i1 = (ui4_org >> 16) & 0xFF;
    i2 = (ui4_org >> 8) & 0xFF;
    i3 = ui4_org & 0xFF;

    if (r_tick[i1] == NULL) {
        r_tick[i1] = (void***)malloc (0x100 * sizeof (void *));
        if (r_tick[i1] == NULL) {
            printf (" === memcheck, %s, %s:%u no memory\n", __FUNCTION__, __FILE__, __LINE__);
            return;
        }
        _am (r_tick[i1]);
        memset (r_tick[i1], 0, 0x100 * sizeof (void *));
        CR_SELF (r_tick[i1], 0x100 * sizeof (void *));
    }

    if (r_tick[i1][i2] == NULL) {
        r_tick[i1][i2] = (void**)malloc (0x100 * sizeof (void *));
        if (r_tick[i1][i2] == NULL) {
            printf (" === memcheck, %s, %s:%u no memory\n", __FUNCTION__, __FILE__, __LINE__);
            return;
        }
        _am (r_tick[i1][i2]);
        memset (r_tick[i1][i2], 0, 0x100 * sizeof (void *));
        CR_SELF (r_tick[i1][i2], 0x100 * sizeof (void *));
    }

    pt_head = (NET_MEM_LISTS_TYPE*) r_tick[i1][i2][i3];
    if (pt_head == NULL) {
        r_tick[i1][i2][i3] = malloc (sizeof (*pt_head));
        pt_head = (NET_MEM_LISTS_TYPE*) r_tick[i1][i2][i3];
        if (pt_head == NULL) {
            printf (" === memcheck, %s, %s:%u Oops!\n", __FUNCTION__, __FILE__, __LINE__);
            return;
        }
        _am (r_tick[i1][i2][i3]);
        memset (pt_head, 0, sizeof (*pt_head));
        DLIST_INIT (pt_head);
        CR_SELF (r_tick[i1][i2][i3], sizeof (*pt_head));
    }

    DLIST_INSERT_TAIL (pt_mem_new, pt_head, t_link);
}

static void _delete_sort_lists (NET_MEM_LISTS *pt_mem_new)
{
    unsigned int ui4_org = pt_mem_new->tick;
    int i1, i2, i3;
    NET_MEM_LISTS_TYPE *pt_head;
    time_t ts, te;

    i1 = (ui4_org >> 16) & 0xFF;
    i2 = (ui4_org >> 8) & 0xFF;
    i3 = ui4_org & 0xFF;

    if (r_tick[i1] == NULL ||
        r_tick[i1][i2] == NULL ||
        r_tick[i1][i2][i3] == NULL) {
        printf (" === memcheck, ---- no such item, tick: 0x%08x, mem: %p\n", pt_mem_new->tick, pt_mem_new);
        return;
    }

    pt_head = (NET_MEM_LISTS_TYPE*) r_tick[i1][i2][i3];
    DLIST_REMOVE (pt_mem_new, pt_head, t_link);

    /* check r_tick r_tick[i1]... need free ?? */
    if (DLIST_IS_EMPTY(pt_head)) {
        ts = time (NULL);
        int i;
        _xm (r_tick[i1][i2][i3]);
        free (r_tick[i1][i2][i3]);
        _dl_self (r_tick[i1][i2][i3]);
        r_tick[i1][i2][i3] = NULL;
        for (i = 0; i<= 0xFF; i++) {
            if (r_tick[i1][i2][i] != NULL)
            {
                goto DEL_FINAL;
            }
        }
        _xm (r_tick[i1][i2]);
        free (r_tick[i1][i2]);
        _dl_self (r_tick[i1][i2]);
        r_tick[i1][i2] = NULL;
        for (i = 0; i<= 0xFF; i++) {
            if (r_tick[i1][i] != NULL)
            {
                goto DEL_FINAL;
            }
        }
        _xm (r_tick[i1]);
        free (r_tick[i1]);
        _dl_self (r_tick[i1]);
        r_tick[i1] = NULL;
    }
    else
    {
        return;
    }
DEL_FINAL:
    te = time (NULL);
    {
        time_t t;
        t = te -ts;
        if (t > 2)
        {
            printf ("[R_MEM] %lu\n", t);
        }
    }
#if 0
    if (DLIST_IS_EMPTY(pt_head)) {
        r_tick[i1][i2][i3] = NULL;
        free (pt_head);
    }
#endif
}

#if 0
#ifndef MAX
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#endif

#ifndef MIN
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#endif
#endif

static unsigned int _get_ms (void) 
{       
    struct timespec tp = {0, 0};
    clock_gettime (CLOCK_MONOTONIC, &tp);
    tp.tv_sec &= 0x3FFFFF;
    return tp.tv_sec * 1000 + tp.tv_nsec / (1000*1000);
}

void _prn_mem_bt(void* mem)
{
    NET_MEM_LISTS* pt_list = NULL;

    L_LOCK ();

    do {
        pt_list = _net_mem_search(mem, NULL);
        if (pt_list == NULL) {
            /*fprintf(stderr, "the mem addr is not found\n");*/
            break;
        }
        if (pt_list->bt == NULL) {
            /*fprintf(stderr, "no matched bt\n");*/
            break;
        }
        fprintf(stderr, "%s\n", pt_list->bt);
    } while(0);

    L_UNLOCK ();
}

void _prn_mem_lists (unsigned int ui4_memsize, int i4_memtime, const char *ps_ip, unsigned short ui2_port)
{
    int            i4_id   = 0;
    size_t         z_total = 0;
    int            i4_cnt  = 0;
    NET_MEM_LISTS* pt_sortlist = NULL;
    unsigned int   tick;
    int            i4_ret  = 0;
    char           ps_send [512] = "";
    unsigned int   ui4_total = 0;

    unsigned char *puac_data = NULL;
    unsigned int ui4_data_len = 200 * 1024;

    L_LOCK ();
 
    tick = _get_ms() / 1000;

    strncpy (ps_send, "Memory Monitor: \n     ID FILE                                       LINE       MEM-ADDR         SIZE       TIME\n"
             "------- ------------------------------------------ ---------- ---------- ---------- ----------\n", sizeof (ps_send) - 1);

    if (_net_mem_op (&puac_data, &ui4_data_len, &ui4_total, ps_send, strlen (ps_send)) != 0)
    {
        printf ("%s:%u _net_mem_op err\n", __FILE__, __LINE__);
        goto FINAL;
    }

    {
        /* tick index */
        int memtime = i4_memtime;
        unsigned int ui4_org_near = 0;
        unsigned int ui4_org_far = 0;


        int i1, j1, k1;
        int i2, j2, k2;
        int i, j, k;

        size_t z_total2 = 0;
        int i4_cnt2 = 0;
        size_t z_total3 = 0;
        int i4_cnt3 = 0;

        if (memtime >= 0) {
            ui4_org_near = tick - memtime;
            ui4_org_far = ui4_first_tick;
        } else {
            /*memtime = 0 - memtime;
            ui4_org_near = memtime;
            ui4_org_far = 0;*/
            ui4_org_near = tick;
            ui4_org_far = -memtime;
        }

        i1 = (ui4_org_near >> 16) & 0xFFFF;
        /*j1 = (ui4_org_near >> 8) & 0xFF;
        k1 = ui4_org_near & 0xFF;*/
        j1 = k1 = 255;

        i2 = (ui4_org_far >> 16) & 0xFFFF;
        /*j2 = (ui4_org_far >> 8) & 0xFF;
        k2 = ui4_org_far & 0xFF;*/
        j2 = k2 = 0;

        for (i = i2; i <= i1; i++)
        {
            if (r_tick[i] == NULL) continue;
            for (j = j2; j <= j1; j++)
            {
                if (r_tick[i][j] == NULL) continue;
                for (k = k2; k <= k1; k++)
                {
                    NET_MEM_LISTS_TYPE *pt_head;
                    pt_head = (NET_MEM_LISTS_TYPE*) r_tick[i][j][k];
                    if (pt_head == NULL) continue;

                    DLIST_FOR_EACH_BACKWARD (pt_sortlist, pt_head, t_link)
                    {
                        if (pt_sortlist != NULL)
                        {
                            if (pt_sortlist->z_size >= ui4_memsize && 
                                    ((memtime >= 0 && (tick - pt_sortlist->tick) >= (unsigned int)memtime) || (memtime < 0 && (tick - pt_sortlist->tick) < (unsigned int)(-memtime))))
                            {
                                time_t t2 = tick - pt_sortlist->tick;
                                snprintf (ps_send, sizeof (ps_send) - 1, "%7d %-42s %-10u %-10p %10zu %10lu\n",
                                            i4_id++,
                                            pt_sortlist->ps_file,
                                            pt_sortlist->ui4_line,
                                            pt_sortlist->pv_data,
                                            pt_sortlist->z_size,
                                            t2);
                                if (_net_mem_op (&puac_data, &ui4_data_len, &ui4_total, ps_send, strlen (ps_send)) != 0)
                                {
                                    printf ("%s:%u _net_mem_op err\n", __FILE__, __LINE__);
                                    goto FINAL;
                                }

                                z_total2 += pt_sortlist->z_size;
                                ++i4_cnt2;
                            }
                            z_total3 += pt_sortlist->z_size;
                            ++i4_cnt3;
                        }
                    }
                }
            }
        }
        printf ("show size: %zu, count: %d, ", z_total2, i4_cnt2);
        printf ("totalsize: %zu, count: %d\n", z_total3, i4_cnt3);

        z_total = ui4_all_size;
        i4_cnt = (int) ui4_all_cnt;
    }

    {
        snprintf (ps_send, sizeof (ps_send) - 1, "------- ------------------------------------------ ---------- ---------- ---------- ----------\n"
                 "                                                                  TOTAL: %10zu / %-8d\n"
                 "                                                                    MAX: %10u / %-8u\n"
                 "                                                                   SELF: %10d /\n", z_total, i4_cnt, ui4_max_size, ui4_max_cnt, _cm());
        _net_mem_op (&puac_data, &ui4_data_len, &ui4_total, ps_send, strlen (ps_send));
    }

FINAL:
    L_UNLOCK ();

    {
        int i4_sock_flag = 0;
        if (ps_ip != NULL && ui2_port != 0)
        {
            int          sockfd  = -1;
            fd_set         wset;

            FD_ZERO (&wset);

            do
            {
                struct sockaddr_in servaddr;

                sockfd = socket (AF_INET, SOCK_STREAM, 0);
                if (sockfd < 0)
                {
                    break;
                }

                servaddr.sin_family = AF_INET;
                servaddr.sin_addr.s_addr = inet_addr (ps_ip);
                servaddr.sin_port = htons(ui2_port);

                i4_ret = connect (sockfd, (const struct sockaddr*) &servaddr, sizeof (servaddr));
                if (i4_ret != 0)
                {
                    printf (" === memcheck, connect %s:%u error\n", ps_ip, ui2_port);
                    close (sockfd);
                    sockfd = -1;
                    break;
                }

                i4_ret = send (sockfd, puac_data, ui4_total, 0);
                if (i4_ret != (int)ui4_total)
                {
                    printf (" === memcheck, -- send %s:%u err, ret %d, should %u\n", ps_ip, ui2_port, i4_ret, ui4_total);
                }
            } while (0);

            if (sockfd >= 0)
            {
                close (sockfd);
                sockfd = -1;
                i4_sock_flag = 1;
            }
        }

        if (ps_ip == NULL || ui2_port == 0 || i4_sock_flag == 0)
        {
            unsigned char puac_tmp[257];
            int i4_i = 0;
            int i4_t = 0;
            int i4_e = 0;
            int i4_tmp_len = 0;

            i4_tmp_len = sizeof (puac_tmp) - 1;
            i4_t = ui4_total / i4_tmp_len;
            i4_e = ui4_total % i4_tmp_len;

            for (i4_i = 0; i4_i < i4_t; i4_i++)
            {
                memcpy (puac_tmp, puac_data + i4_i * i4_tmp_len, i4_tmp_len);
                puac_tmp [i4_tmp_len] = 0;
                printf ("%s", puac_tmp);
#ifdef __arm
                if (((i4_i + 1) % 50) == 0)
                {
                    sleep (1);
                }
#endif
            }

            if (i4_e > 0)
            {
                memcpy (puac_tmp, puac_data + i4_i * i4_tmp_len, i4_e);
                puac_tmp [i4_e] = 0;
                printf ("%s\n", puac_tmp);
            }
            fflush (stdout);
        }
    }
 
    L_LOCK ();
    if (puac_data != NULL)
    {
        _xm (puac_data);
        free (puac_data);
        _dl_self (puac_data);
        puac_data = NULL;
    }
    L_UNLOCK ();
}

int _change_mem_lists (const void* pv_data,
                         const void* pv_new_data,
                         size_t z_size)
{
    NET_MEM_LISTS* pt_list, *lp, *p;
    int i4_h, i4_h2;

    if (pv_data == NULL)
    {
        return -1;
    }

    /*L_LOCK ();*/

    pt_list = _net_mem_search (pv_data, &i4_h);
    if (pt_list == NULL)
    {
        /*L_UNLOCK ();*/
        return -1;
    }

#ifdef NET_MEMSET_CHECK
    /* memory index */
    _delete_sort_lists2 (pt_list);
#endif

    p = _net_mem_search (pv_new_data, &i4_h2);
    /* not check p, just get i4_h2 */

    if (i4_h != i4_h2)
    {
        lp = NULL;
        for (p = mem_buckets[i4_h]; p != NULL; p = p->pt_next)
        {
            if (p == pt_list && p->pv_data == pt_list->pv_data)
            {
                if (lp == NULL)
                    mem_buckets[i4_h] = p->pt_next;
                else
                    lp->pt_next = p->pt_next;
                /* do not free p */
                break;
            }
            lp = p;
        }
        pt_list->pt_next = mem_buckets[i4_h2];
        mem_buckets[i4_h2] = pt_list;
    }

    pt_list->pv_data = pv_new_data;

    ui4_all_size += z_size;
    ui4_all_size -= pt_list->z_size;

    if (ui4_all_size > ui4_max_size)
    {
        ui4_max_size = ui4_all_size;
    }

    pt_list->z_size = z_size;

#ifdef NET_MEMSET_CHECK
    /* memory index */
    _insert_sort_lists2 (pt_list);
#endif

    /*L_UNLOCK ();*/

    return 0;
}

static unsigned int _net_mem_hvalue (const void *pv_data)
{
    return (intptr_t)pv_data;
}

#define NET_MEMBT

#ifdef NET_MEMBT
#include <execinfo.h>
#define NET_MEMBT_DEEP 10
#endif
int _add_mem_lists (const char* ps_file,
                      unsigned int      ui4_line,
                      const void* pv_data,
                      size_t      z_size)
{
    NET_MEM_LISTS* pt_list = NULL;
    int i4_h = _net_mem_hvalue (pv_data) % HASH_SIZE; /*(sizeof (mem_buckets) / sizeof (mem_buckets[0]));*/

#ifdef _NET_MEMLEAK_S_MEMLEAK
    if (h_self == null_handle)
    {
        _init_self ();
    }
#endif

    pt_list = (NET_MEM_LISTS*)malloc (sizeof (NET_MEM_LISTS));
    if (pt_list == NULL)
    {
        printf (" === memcheck, ---- Oops! malloc err. %s:%u\n", __FILE__, __LINE__);
        return -1;
    }
    _am (pt_list);

    CR_SELF (pt_list, sizeof (*pt_list));

    memset (pt_list, 0, sizeof (*pt_list));

    pt_list->pv_data  = pv_data;
    pt_list->z_size   = z_size;
    pt_list->ps_file  = atom_str (ps_file);
    pt_list->ui4_line = ui4_line;
    pt_list->tick     = _get_ms () / 1000;

#ifdef NET_MEMBT
    /* backtrace */
    do {
        int j, nptrs;
        void *buffer[NET_MEMBT_DEEP];
        char **strings;
        int alen = 0;

        nptrs = backtrace(buffer, NET_MEMBT_DEEP);

        strings = backtrace_symbols(buffer, nptrs);
        if (strings == NULL) {
            break;
        }

        for (j = 2; j < nptrs; j++) {
            alen += strlen(strings[j]) + 1;
        }

        pt_list->bt = (char*)malloc(alen+1);
        if (pt_list->bt != NULL) {
            memset(pt_list->bt, 0, alen+1);
            for (j = 2; j < nptrs; j++) {
                sprintf(pt_list->bt, "%s%s\n", pt_list->bt, strings[j]);
            }
        }

        free(strings);
    } while(0);
#endif

    /*L_LOCK ();*/

    pt_list->pt_next = mem_buckets[i4_h];
    mem_buckets[i4_h] = pt_list;

    ui4_all_size += z_size;
    ui4_all_cnt++;

    if (ui4_all_size > ui4_max_size)
    {
        ui4_max_size = ui4_all_size;
    }
    if (ui4_all_cnt > ui4_max_cnt)
    {
        ui4_max_cnt = ui4_all_cnt;
    }

    _insert_sort_lists (pt_list);

    if (ui4_first_tick == 0xFFFFFFFF)
    {
        ui4_first_tick = pt_list->tick;
    }

#ifdef NET_MEMSET_CHECK
    _insert_sort_lists2 (pt_list);

    /*if (ui4_first_mem > (unsigned int)pt_list->pv_data)
    {
        ui4_first_mem = (unsigned int)pt_list->pv_data;
    }*/
#endif

    /*L_UNLOCK ();*/

    return 0;
}

int _remove_mem_lists (const void* pv_data)
{
    NET_MEM_LISTS *pt_list, *lp, *p;
    int i4_h;

    if (pv_data == NULL)
    {
        return -1;
    }

    /*L_LOCK ();*/

    pt_list = _net_mem_search (pv_data, &i4_h);
    if (pt_list == NULL)
    {
        /*L_UNLOCK ();*/
        return -1;
    }

    lp = NULL;
    for (p = mem_buckets[i4_h]; p != NULL; p = p->pt_next)
    {
        if (p == pt_list && p->pv_data == pt_list->pv_data)
        {
            if (lp == NULL)
                mem_buckets[i4_h] = p->pt_next;
            else
                lp->pt_next = p->pt_next;
            if (p->ps_file != NULL)
            {
                atom_free (p->ps_file);
            }
            free(p->bt);
            p->bt = NULL;

            ui4_all_size -= p->z_size;
            ui4_all_cnt--;

            _delete_sort_lists (p);
#ifdef NET_MEMSET_CHECK
            _delete_sort_lists2 (p);
#endif

            _xm (p);
            free (p);
            _dl_self (p);

            /*L_UNLOCK ();*/
            return 0;
        }
        lp = p;
    }

    /*L_UNLOCK ();*/

    return 0;
}

#ifdef NET_MEMSET_CHECK
static NET_MEM_LISTS* _net_mem_search2 (const void *pv_data)
{
    /* memory index */
    unsigned int ui4_org_near = (unsigned int)pv_data & 0xFFFF0000; /* only check memory-mask range 0xFFFF/64KB */
    unsigned int ui4_org_far = (unsigned int)pv_data | 0x0000FFFF;

    int i1, j1, k1;
    int i2, j2, k2;
    int i, j, k;

    if (r_mem == NULL) return NULL;

    ui4_org_near >>= 4; /* 16 align */
    ui4_org_far >>= 4; /* 16 align */

    i2 = (ui4_org_near >> 16) & 0xFFFF;
    /*j2 = (ui4_org_near >> 8) & 0xFF;
    k2 = ui4_org_near & 0xFF;*/
    j2 = k2 = 0;

    i1 = (ui4_org_far >> 16) & 0xFFFF;
    /*j1 = (ui4_org_far >> 8) & 0xFF;
    k1 = ui4_org_far & 0xFF;*/
    j1 = k1 = 255;

    for (i = i2; i <= i1; i++)
    {
        if (r_mem[i] == NULL) continue;
        for (j = j2; j <= j1; j++)
        {
            if (r_mem[i][j] == NULL) continue;
            for (k = k2; k <= k1; k++)
            {
                NET_MEM_LISTS_TYPE *pt_head;
                NET_MEM_LISTS *pt_sortlist = NULL;
                pt_head = (NET_MEM_LISTS_TYPE*) r_mem[i][j][k];
                if (pt_head == NULL) continue;

                DLIST_FOR_EACH (pt_sortlist, pt_head, t_link2)
                {
                    if (pt_sortlist != NULL &&
                        pt_sortlist->pv_data < pv_data &&
                        (unsigned int)pt_sortlist->pv_data + (unsigned int)pt_sortlist->z_size > (unsigned int)pv_data) /* in range */
                    {
                        return pt_sortlist;
                    }
                }
            }
        }
    }

    return NULL;
}
#endif /* NET_MEMSET_CHECK */

static NET_MEM_LISTS* _net_mem_search1 (const void *pv_data, unsigned int tick)
{
    int i, j, k;

    i = (tick >> 16) & 0xFF;
    j = (tick >> 8) & 0xFF;
    k = tick & 0xFF;

    if (r_tick[i] != NULL && r_tick[i][j] != NULL && r_tick[i][j][k] != NULL)
    {
        NET_MEM_LISTS_TYPE *pt_head;
        NET_MEM_LISTS *pt_sortlist = NULL;

        pt_head = (NET_MEM_LISTS_TYPE*) r_tick[i][j][k];

        DLIST_FOR_EACH (pt_sortlist, pt_head, t_link)
        {
            if (pt_sortlist != NULL &&
                pt_sortlist->pv_data == pv_data)
            {
                return pt_sortlist;
            }
        }
    }

    return NULL;
}

static NET_MEM_LISTS* _net_mem_search (const void *pv_data, int *hv)
{
    NET_MEM_LISTS *pht;
    int h = _net_mem_hvalue (pv_data) % HASH_SIZE; /*(sizeof (mem_buckets) / sizeof (mem_buckets[0]));*/

    if (hv != NULL)
        *hv = h;

    if ((pht = mem_buckets[h]) == NULL)
    {
        return NULL;
    }

    for ( ; pht != NULL; pht = pht->pt_next)
    {
        if (pht->pv_data == pv_data)
        {
            return pht;
        }
    }

    return NULL;
}

void* _net_mem_alloc (const char* ps_file,
                      unsigned int      ui4_line,
                      size_t      z_size)
{
    void* pv_mem;

    L_LOCK ();
    SMREC(1);

    pv_mem = malloc (z_size);
    if (pv_mem != NULL)
    {
        _add_mem_lists (ps_file, ui4_line, pv_mem, z_size);
    }
    else
    {
        if (z_size > 20000000)
        {
            printf (" === memcheck, %s:%u alloc mem size %zu, may be too big\n", ps_file, ui4_line, z_size);
        }
    }

    RMREC();

    L_UNLOCK ();

    return pv_mem;
}

void* _net_mem_calloc (const char* ps_file,
                        unsigned int     ui4_line,
                        unsigned int     ui4_num_unit,
                        size_t     z_unit_size)
{
    void* pv_mem;

    L_LOCK ();
    SMREC(1);

    pv_mem = malloc (ui4_num_unit * z_unit_size);
    if (pv_mem != NULL)
    {
        _add_mem_lists (ps_file, ui4_line, pv_mem, ui4_num_unit * z_unit_size);
        memset(pv_mem, 0, ui4_num_unit * z_unit_size);
    }
    RMREC();

    L_UNLOCK ();

    return pv_mem;
}

void* _net_mem_realloc (const char* ps_file,
                        unsigned int      ui4_line,
                        void*       pv_mem,
                        size_t      z_new_size)
{
    void* pv_old_mem = pv_mem;
    void* pv_new_mem = NULL;

    L_LOCK ();
    SMREC(1);

    pv_new_mem = realloc (pv_old_mem, z_new_size);
    if (pv_new_mem == NULL)
    {
        RMREC();
        L_UNLOCK ();
        return NULL;
    }

#if 0 /* bug */
    if (pv_new_mem == pv_old_mem)
    {
        L_UNLOCK ();
        return pv_old_mem;
    }
#endif

    if (pv_old_mem == NULL)
    {
        _add_mem_lists (ps_file, ui4_line, pv_new_mem, z_new_size);
    }
    else
    {
        if (_change_mem_lists (pv_old_mem, pv_new_mem, z_new_size) != 0)
        {
            /* NOTE */
            printf (" === memcheck, %s:%u realloc () warning, new: %p, old: %p\n",
                        ps_file, ui4_line, pv_new_mem, pv_old_mem);
        }
    }

    RMREC();
    L_UNLOCK ();

    return pv_new_mem;
}

void _net_mem_free (const char* ps_file,
                    unsigned int      ui4_line,
                    void*       pv_mem)
{
    /*unsigned int ms_end = 0;
    unsigned int ms_start = _get_ms();*/
    if (pv_mem == NULL) /* NULL do nothing */
    {
        return;
    }

    if (ps_file || ui4_line) {
        ;
    }

    L_LOCK ();
    SMREC(1);

    if (_remove_mem_lists (pv_mem) != 0)
    {
#if 0 /* no checking */
        /* for c++ override delete */
        if (ps_file[0] != '*') printf (" === memcheck, %s:%u free (%p warning\n", ps_file, ui4_line, (int)pv_mem);
#endif
    }

    free (pv_mem);

    RMREC();
    L_UNLOCK ();
    /*ms_end = _get_ms();
    if (ps_file[0] == '*' && ms_end - ms_start > 1) printf (" === LOG memfree, %s:%u free (%p) total cnt: %u, spd %u-ms\n", ps_file, ui4_line, (int)pv_mem, ui4_all_cnt, ms_end - ms_start);*/
}

char* _net_strdup (const char* ps_file,
                   unsigned int      ui4_line,
                   const char* ps_str)
{
    char* ps_dup;

    L_LOCK ();
    SMREC(1);

    if (ps_str == NULL)
    {
        printf (" === memcheck, %s:%u strdup (NULL) warning\n", ps_file, ui4_line);
    }

    ps_dup = strdup (ps_str);

    if (ps_dup != NULL)
    {
        _add_mem_lists (ps_file, ui4_line, ps_dup, strlen (ps_dup) + 1);
    }

    RMREC();
    L_UNLOCK ();

    return ps_dup;
}

void *_net_memset (const char* ps_file,
                   unsigned int      ui4_line,
                   void*       s,
                   int       c,
                   size_t      n)
{
#ifdef NET_MEMSET_CHECK
    /* check memset overflow */
    NET_MEM_LISTS *pt_list, *p;
    int i4_h;

    L_LOCK ();

    pt_list = _net_mem_search (s, &i4_h);
    if (pt_list != NULL)
    {
        for (p = mem_buckets[i4_h]; p != NULL; p = p->pt_next)
        {
            if (p == pt_list && p->pv_data == pt_list->pv_data)
            {
                if (n > p->z_size)
                {
                    printf (" === memcheck, overflow %s:%u memset (%p, %d, %d); org_mem_alloc: %s:%u, memaddr: %p size: %u\n",
                                ps_file, ui4_line, (int)s, c, n, p->ps_file, p->ui4_line, (int)p->pv_data, p->z_size);
                }

                break;
            }
        }
    } else {
        pt_list = _net_mem_search2 (s);
        if ((pt_list != NULL) && ((unsigned int)s + n) > ((unsigned int)pt_list->pv_data + pt_list->z_size))
        {
            /*printf (" === memcheck, overflow %s:%u memset (%p, %d, %d); org_mem_alloc: %s:%u, memaddr: %p size: %u\n",
                        ps_file, ui4_line, (int)s, c, n, p->ps_file, p->ui4_line, (int)p->pv_data, p->z_size);*/
            printf (" === memcheck, overflow %s:%u memset (%p, %d, %d); org_mem_alloc: %s:%u, memaddr: %p size: %u\n",
                        ps_file, ui4_line, (int)s, c, n, "", 0, 0, 0);
        }
    }

    L_UNLOCK ();
#else
    if (ps_file || ui4_line) {
        ;
    }
#endif

    return memset (s, c, n);
}

/* for fun */
int _net_strcmp_np (const char* ps_file,
                   unsigned int      ui4_line,
                   const char* s1,
                   const char* s2)
{
    if (s1 == NULL || s2 == NULL)
    {
        printf ("warning: strcmp arguments have NULL, %s:%u\n", ps_file, ui4_line);
    }

    return strcmp (s1, s2);
}

/* for socket only */

static const char* _get_domain(int domain)
{
    switch (domain)
    {
    case AF_LOCAL: return "AF_LOCAL";
    case AF_INET: return "AF_INET";
#ifdef AF_INET6
    case AF_INET6: return "AF_INET6";
#endif
    default: return "Unknown";
    }
    return "Unknown";
}

static const char *_get_type (int type)
{
    switch (type)
    {
    case SOCK_STREAM: return "SOCK_STREAM";
    case SOCK_DGRAM: return "SOCK_DGRAM";
    case SOCK_RAW: return "SOCK_RAW";
    case SOCK_RDM: return "SOCK_RDM";
    case SOCK_SEQPACKET: return "SOCK_SEQPACKET";
    default: return "Unknown";
    }
    return "Unknown";
}

struct st_my_fds {
    char *ps_file;
    unsigned int ui4_line;
    int domain;
    int type;
    int fd;
};

static handle_t h_fds_group = null_handle;

static int i4_g_idx = 0;

int _get_fds_info (void *pv_data, void *pv_arg)
{
    struct st_my_fds *pt_fds = (struct st_my_fds*) pv_data;

    if (pt_fds == NULL)
    {
        return 0;
    }

    printf ("%5d fds: %d  domain: %-10s type: %-15s %s:%u\n",
                i4_g_idx++, pt_fds->fd,
                _get_domain (pt_fds->domain),
                _get_type (pt_fds->type),
                pt_fds->ps_file,
                pt_fds->ui4_line);

    if (pv_arg) return 0; /* dummy code */

    return 0;
}

int _fds_infos (void)
{
    if (h_fds_group == null_handle)
    {
        return 0;
    }

    i4_g_idx = 0;
    mm_list_map (h_fds_group, _get_fds_info, NULL);

    return 0;
}

int _net_socket_np (const char *ps_file,
                      unsigned int      ui4_line,
                      int       domain,
                      int       type,
                      int       protocol)
{
    int fd;
    struct st_my_fds *pt_fd;

    fd = socket (domain, type, protocol);

    if (fd < 0)
    {
        return fd;
    }

    if (h_fds_group == null_handle)
    {
        mm_list_create (&h_fds_group);
    }
    if (h_fds_group == null_handle)
    {
        return fd;
    }

    pt_fd = malloc (sizeof (*pt_fd));
    if (pt_fd == NULL)
    {
        return fd;
    }
    _am (pt_fd);
    CR_SELF (pt_fd, sizeof (*pt_fd));

    pt_fd->ps_file = strdup (ps_file);
    _am (pt_fd->ps_file);
    pt_fd->ui4_line = ui4_line;
    pt_fd->domain = domain;
    pt_fd->type = type;
    pt_fd->fd = fd;

    mm_list_add (h_fds_group, pt_fd);

    return fd;
}

static int _net_mylist_fd_cmp (const void *pv_data, const void *pv_tag)
{
    struct st_my_fds *pt_fd = (struct st_my_fds*) pv_data;

    if (pt_fd == NULL)
    {
        return -1;
    }

    if (pt_fd->fd == (intptr_t)pv_tag)
    {
        return 0;
    }

    return -1;
}

int _net_close_np (const char *ps_file, unsigned int ui4_line, int fd)
{
    struct st_my_fds *pt_fd;
    
    pt_fd = mm_list_search2 (h_fds_group, (const void*)(intptr_t)fd, _net_mylist_fd_cmp);
    if (pt_fd != NULL)
    {
        mm_list_delete (h_fds_group, pt_fd);
        _xm (pt_fd->ps_file);
        free (pt_fd->ps_file);
        _xm (pt_fd);
        free (pt_fd);
        _dl_self (pt_fd);
        pt_fd = NULL;
    }

    if (ps_file == NULL && ui4_line == 0)
    {
        printf ("%s:%u\n", ps_file, ui4_line);
    }

    return close(fd);
}

