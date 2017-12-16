#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <semaphore.h>

/*#define NO_AXM*/
#include "memlist.h"
#include "atom_str.h"

#define MY_LIST_LOGIC 0xBC190DF3

extern void (*pf_free) (void* pv_data);
extern int (*pf_cmp) (const void *pv1, const void *pv2);

void** mm_list_toarray (handle_t h_handle, void *end);
int mm_list_map (handle_t h_handle, int apply(void*, void*), void* pv_arg);
int mm_list_create (handle_t *ph_handle);
int mm_list_destroy (handle_t h_handle, void (*pf_free) (void* pv_data));
void* mm_list_delete_head (handle_t h_handle);
void* mm_list_delete (handle_t h_handle, const void *pv_data);
void* mm_list_delete2 (handle_t h_handle, const void *pv_data, int (*pf_cmp) (const void *pv1, const void* pv2));
int mm_list_add (handle_t h_handle, void *pv_data);
void *mm_list_search (handle_t h_handle, const void *pv_data);
void *mm_list_search2 (handle_t h_handle, const void *pv_data, int (*pf_cmp) (const void *pv1, const void* pv2));

struct st_my_list_head {
    unsigned int ui4_logic;
    void *pv_head;
    void *pv_tail;
    unsigned int i4_len;
    sem_t sem;
};

struct st_my_list {
    struct st_my_list *pt_next;
    struct st_my_list *pt_prev;
    void *pv_data;
};

static void _my_list_lock (sem_t *sem)
{
    sem_wait (sem);
}

static void _my_list_unlock (sem_t *sem)
{
    sem_post (sem);
}

#if 0
int _get_ms (void)
{
    struct timespec tp = {0, 0};
    clock_gettime (CLOCK_MONOTONIC, &tp);
    return tp.tv_sec * 1000 + tp.tv_nsec / (1000*1000);
}
#endif

void** mm_list_toarray (handle_t h_handle, void *end)
{
    struct st_my_list_head *pt_head = (struct st_my_list_head*) h_handle;
    struct st_my_list *pt_list = NULL;
    void **ppv_array = NULL;
    int len = 0;
    int i = 0;

    if (pt_head == NULL || pt_head->ui4_logic != MY_LIST_LOGIC)
    {
        return NULL;
    }

    len = pt_head->i4_len;
    if (len == 0)
    {
        return NULL;
    }

    ppv_array = (void**)malloc (sizeof (void*) * (len + 1));
    if (ppv_array == NULL)
    {
        printf ("[LIST] out of memory\n");
        return NULL;
    }
    _am (ppv_array);

    i = 0;
    pt_list = (struct st_my_list*)pt_head->pv_head;
    while (pt_list != NULL)
    {
        if (i >= len)
        {
            /* need lock else may have bug */
            break;
        }
        ppv_array[i++] = pt_list->pv_data;
        pt_list = pt_list->pt_next;
    }

    ppv_array[len] = end;

    return ppv_array;
}

int mm_list_toarray2 (handle_t h_handle, void **ppv_array, int len)
{
    struct st_my_list_head *pt_head = (struct st_my_list_head*) h_handle;
    struct st_my_list *pt_list = NULL;
    int i = 0;

    if (pt_head == NULL || pt_head->ui4_logic != MY_LIST_LOGIC)
    {
        return 0;
    }

    if (len < 1)
    {
        return 0;
    }

    i = 0;
    pt_list = (struct st_my_list*) pt_head->pv_head;
    while (pt_list != NULL)
    {
        if (i >= len)
        {
            /* need lock else may have bug */
            break;
        }
        ppv_array[i++] = pt_list->pv_data;
        pt_list = pt_list->pt_next;
    }

    return i;
}

int mm_list_map (handle_t h_handle, int apply(void*, void*), void* pv_arg)
{
    struct st_my_list_head *pt_head = (struct st_my_list_head*) h_handle;
    struct st_my_list *pt_list = NULL;
    int i4_len = 0;

    if (pt_head == NULL || pt_head->ui4_logic != MY_LIST_LOGIC)
    {
        return -1;
    }

    if (pt_head->i4_len == 0)
    {
        return 0;
    }

    i4_len = pt_head->i4_len;

    pt_list = (struct st_my_list*)pt_head->pv_head;

    while (pt_list != NULL && i4_len-- != 0)
    {
        struct st_my_list *pt_next = pt_list->pt_next;
        apply (pt_list->pv_data, pv_arg);
        pt_list = pt_next;
    }

    return 0;
}

int mm_list_size (handle_t h_handle)
{
    struct st_my_list_head *pt_head = (struct st_my_list_head*) h_handle;

    if (pt_head == NULL || pt_head->ui4_logic != MY_LIST_LOGIC)
    {
        return 0;
    }

    return pt_head->i4_len;
}

int mm_list_create (handle_t *ph_handle)
{
    struct st_my_list_head *pt_head = NULL;

    if (ph_handle == NULL)
    {
        return -1;
    }

    pt_head = (struct st_my_list_head*) malloc (sizeof (struct st_my_list_head));
    if (pt_head == NULL)
    {
        return -1;
    }
    _am (pt_head);

    pt_head->ui4_logic = MY_LIST_LOGIC;
    pt_head->pv_head = NULL;
    pt_head->pv_tail = NULL;
    pt_head->i4_len = 0;

    sem_init (&pt_head->sem, 0, 1);

    *ph_handle = (handle_t) pt_head;

    return 0;
}

int mm_list_destroy (handle_t h_handle, void (*pf_free)(void* pv_data))
{
    struct st_my_list_head *pt_head = (struct st_my_list_head*) h_handle;

    if (pt_head == NULL || pt_head->ui4_logic != MY_LIST_LOGIC)
    {
        return -1;
    }

    /* call mm_list_delete_head may have deal lock so not use lock() */
    /*_my_list_lock ();*/

    while (pt_head->i4_len != 0)
    {
        /* TODO */
        void *pv_data = mm_list_delete_head (h_handle);
        if (pv_data != NULL && pf_free)
        {
            pf_free (pv_data);
        }
    }

    if (pt_head->i4_len == 0)
    {
        /*memset (pt_head, 0, sizeof (struct st_my_list_head));*/
        pt_head->ui4_logic = 0;
        _xm (pt_head);
        free (pt_head);
        pt_head = NULL;
    }

    /*_my_list_unlock ();*/

    return 0;
}

int mm_list_add (handle_t h_handle, void *pv_data)
{
    struct st_my_list_head *pt_head = (struct st_my_list_head*) h_handle;
    struct st_my_list *pt_list = NULL;

    if (pt_head == NULL || pt_head->ui4_logic != MY_LIST_LOGIC)
    {
        return -1;
    }

    if (pv_data == NULL)
    {
        return -1;
    }

    pt_list = (struct st_my_list*) malloc (sizeof (struct st_my_list));
    if (pt_list == NULL)
    {
        return -1;
    }
    _am (pt_list);

    memset (pt_list, 0, sizeof (struct st_my_list));

    pt_list->pv_data = pv_data;

    _my_list_lock (&pt_head->sem);

    /* */
    pt_list->pt_next = (struct st_my_list*)pt_head->pv_head;

    if (pt_head->pv_head)
    {
        ((struct st_my_list*)pt_head->pv_head)->pt_prev = pt_list;
    }

    pt_head->pv_head = pt_list;

    if (pt_head->pv_tail == NULL)
    {
        pt_head->pv_tail = pt_list;
    }

    ++pt_head->i4_len;

    _my_list_unlock (&pt_head->sem);

    return 0;
}

int mm_list_add_head (handle_t h_handle, void *pv_data)
{
    return mm_list_add (h_handle, pv_data);
}

int mm_list_add_tail (handle_t h_handle, void *pv_data)
{
    struct st_my_list_head *pt_head = (struct st_my_list_head*) h_handle;
    struct st_my_list *pt_list = NULL;

    if (pt_head == NULL || pt_head->ui4_logic != MY_LIST_LOGIC)
    {
        return -1;
    }

    if (pv_data == NULL)
    {
        return -1;
    }

    pt_list = (struct st_my_list*) malloc (sizeof (struct st_my_list));
    if (pt_list == NULL)
    {
        return -1;
    }
    _am (pt_list);

    memset (pt_list, 0, sizeof (struct st_my_list));

    pt_list->pv_data = pv_data;

    _my_list_lock (&pt_head->sem);

    /* */
    pt_list->pt_next = NULL;
    pt_list->pt_prev = (struct st_my_list*)pt_head->pv_tail;

    if (pt_head->pv_tail)
    {
        ((struct st_my_list*)pt_head->pv_tail)->pt_next = pt_list;
    }

    pt_head->pv_tail = pt_list;

    if (pt_head->pv_head == NULL)
    {
        pt_head->pv_head = pt_list;
    }

    ++pt_head->i4_len;

    _my_list_unlock (&pt_head->sem);

    return 0;
}

void* mm_list_delete (handle_t h_handle, const void *pv_data)
{
    struct st_my_list_head *pt_head = (struct st_my_list_head*) h_handle;
    struct st_my_list *pt_list = NULL;
    int i4_len = 0;

    if (pt_head == NULL || pt_head->ui4_logic != MY_LIST_LOGIC)
    {
        return NULL;
    }

    if (pt_head->i4_len == 0)
    {
        return NULL;
    }

    _my_list_lock (&pt_head->sem);

    i4_len = pt_head->i4_len;

    pt_list = (struct st_my_list*)pt_head->pv_head;

    while (pt_list != NULL && i4_len-- != 0)
    {
        if (pt_list->pv_data == pv_data)
        {
            if (pt_list->pt_prev != NULL)
            {
                pt_list->pt_prev->pt_next = pt_list->pt_next;
            }
            else
            {
                if (pt_head->pv_head == pt_list)
                {
                    pt_head->pv_head = pt_list->pt_next;
                }
            }

            if (pt_list->pt_next != NULL)
            {
                pt_list->pt_next->pt_prev = pt_list->pt_prev;
            }
            else
            {
                if (pt_head->pv_tail == pt_list)
                {
                    pt_head->pv_tail = pt_list->pt_prev;
                }
            }

            pt_list->pt_prev = NULL;
            pt_list->pt_next = NULL;
            pt_list->pv_data = NULL;

            _xm (pt_list);
            free (pt_list);
            pt_list = NULL;

            --pt_head->i4_len;

            _my_list_unlock (&pt_head->sem);

            return (void*) pv_data;
        }
        pt_list = pt_list->pt_next;
    }

    _my_list_unlock (&pt_head->sem);

    return NULL;
}

void* mm_list_delete_tail (handle_t h_handle)
{
    struct st_my_list_head *pt_head = (struct st_my_list_head*) h_handle;
    struct st_my_list *pt_list = NULL;
    void *pv_data;

    if (pt_head == NULL || pt_head->ui4_logic != MY_LIST_LOGIC)
    {
        return NULL;
    }

    if (pt_head->i4_len == 0)
    {
        return NULL;
    }

    _my_list_lock (&pt_head->sem);

    pt_list = (struct st_my_list*)pt_head->pv_tail;

    if (pt_list == NULL)
    {
        _my_list_unlock (&pt_head->sem);
        return NULL;
    }

    pv_data = pt_list->pv_data;

    if (pt_head->pv_head == pt_list)
    {
        pt_head->pv_head = NULL;
        pt_head->pv_tail = NULL;
        pt_head->i4_len = 0;
    }
    else
    {
        pt_list->pt_prev->pt_next = pt_list->pt_next;
        pt_head->pv_tail = pt_list->pt_prev;
        --pt_head->i4_len;
    }

    _xm (pt_list);
    free (pt_list);
    pt_list = NULL;

    _my_list_unlock (&pt_head->sem);

    return pv_data;
}

void* mm_list_delete2 (handle_t h_handle, const void *pv_data, int (*pf_cmp) (const void*, const void*))
{
    struct st_my_list_head *pt_head = (struct st_my_list_head*) h_handle;
    struct st_my_list *pt_list = NULL;
    int i4_len = 0;

    if (pt_head == NULL || pt_head->ui4_logic != MY_LIST_LOGIC)
    {
        return NULL;
    }

    if (pt_head->i4_len == 0)
    {
        return NULL;
    }

    _my_list_lock (&pt_head->sem);

    i4_len = pt_head->i4_len;

    pt_list = (struct st_my_list*)pt_head->pv_head;

    while (pt_list != NULL && i4_len-- != 0)
    {
        int i4_cmp = -1;

        if (pf_cmp == NULL)
        {
            if (pt_list->pv_data == pv_data)
            {
                i4_cmp = 0;
            }
        }
        else
        {
            i4_cmp = pf_cmp (pt_list->pv_data, pv_data);
        }

        /*if (pt_list->pv_data == pv_data)*/
        if (i4_cmp == 0)
        {
            void *pv_tmp = pt_list->pv_data;

            if (pt_list->pt_prev != NULL)
            {
                pt_list->pt_prev->pt_next = pt_list->pt_next;
            }
            else
            {
                if (pt_head->pv_head == pt_list)
                {
                    pt_head->pv_head = pt_list->pt_next;
                }
            }

            if (pt_list->pt_next != NULL)
            {
                pt_list->pt_next->pt_prev = pt_list->pt_prev;
            }
            else
            {
                if (pt_head->pv_tail == pt_list)
                {
                    pt_head->pv_tail = pt_list->pt_prev;
                }
            }

            pt_list->pt_prev = NULL;
            pt_list->pt_next = NULL;
            pt_list->pv_data = NULL;

            _xm (pt_list);
            free (pt_list);
            pt_list = NULL;

            --pt_head->i4_len;

            _my_list_unlock (&pt_head->sem);

            return pv_tmp;
        }
        pt_list = pt_list->pt_next;
    }

    _my_list_unlock (&pt_head->sem);

    return NULL;
}

void* mm_list_delete_head (handle_t h_handle)
{
    struct st_my_list_head *pt_head = (struct st_my_list_head*) h_handle;

    if (pt_head == NULL || pt_head->ui4_logic != MY_LIST_LOGIC)
    {
        return NULL;
    }

    if (pt_head->i4_len == 0)
    {
        return NULL;
    }

    if (pt_head->pv_head != NULL)
    {
        return mm_list_delete (h_handle, ((struct st_my_list*)(pt_head->pv_head))->pv_data);
    }

    return NULL;
}

void* mm_list_search (handle_t h_handle, const void *pv_data)
{
#if 0
    struct st_my_list_head *pt_head = (struct st_my_list_head*) h_handle;
    struct st_my_list *pt_list = NULL;
    int i4_len = 0;

    if (pt_head == NULL || pt_head->ui4_logic != MY_LIST_LOGIC)
    {
        return NULL;
    }

    if (pt_head->i4_len == 0)
    {
        return NULL;
    }

    i4_len = pt_head->i4_len;

    pt_list = pt_head->pv_head;

    while (pt_list != NULL && i4_len-- != 0)
    {
        if (pt_list->pv_data == pv_data)
        {
            return pv_data;
        }
        pt_list = pt_list->pt_next;
    }

    return NULL;
#endif
    return mm_list_search2 (h_handle, pv_data, NULL);
}

void* mm_list_search2 (handle_t h_handle, const void *pv_data, int (*pf_cmp) (const void*, const void*))
{
    struct st_my_list_head *pt_head = (struct st_my_list_head*) h_handle;
    struct st_my_list *pt_list = NULL;
    int i4_len = 0;

    if (pt_head == NULL || pt_head->ui4_logic != MY_LIST_LOGIC)
    {
        return NULL;
    }

    if (pt_head->i4_len == 0)
    {
        return NULL;
    }

    _my_list_lock (&pt_head->sem);

    i4_len = pt_head->i4_len;

    pt_list = (struct st_my_list*)pt_head->pv_head;

    while (pt_list != NULL && i4_len-- != 0)
    {
        int i4_cmp = -1;

        if (pf_cmp == NULL)
        {
            if (pt_list->pv_data == pv_data)
            {
                i4_cmp = 0;
            }
        }
        else
        {
            i4_cmp = pf_cmp (pt_list->pv_data, pv_data);
        }

        /*if (pt_list->pv_data == pv_data)*/
        if (i4_cmp == 0)
        {
            _my_list_unlock (&pt_head->sem);
            return pt_list->pv_data;
        }
        pt_list = pt_list->pt_next;
    }

    _my_list_unlock (&pt_head->sem);

    return NULL;
}

