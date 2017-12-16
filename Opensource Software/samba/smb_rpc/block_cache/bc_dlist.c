#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <fcntl.h>
#include <pthread.h>

#include "atom_strdup.h"
#include "bc_dlist.h"

int _get_ms (void)
{
    struct timespec tp = {0, 0};
    clock_gettime (CLOCK_MONOTONIC, &tp);
    tp.tv_sec &= 0x3FFFFF;
    return tp.tv_sec * 1000 + tp.tv_nsec / (1000*1000);
}

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

struct bc_dlist_node {
    char* data;
    struct bc_dlist_node* prev;
    struct bc_dlist_node* next;
};

struct bc_dlist {
    int count;
    pthread_mutex_t mutex;
    struct bc_dlist_node* head;
    struct bc_dlist_node* tail;
};

int bc_lock_init(pthread_mutex_t* p_mutex)
{
    pthread_mutexattr_t t_attr_mx;

    if (pthread_mutexattr_init(&t_attr_mx) != 0)
    {
        return -1;
    }

#ifdef _GNU_SOURCE
    if (pthread_mutexattr_settype(&t_attr_mx, PTHREAD_MUTEX_RECURSIVE) != 0)
    {
        pthread_mutexattr_destroy(&t_attr_mx);
        return -1;
    }
#endif
    if (pthread_mutex_init(p_mutex, &t_attr_mx) != 0)
    {
        pthread_mutexattr_destroy(&t_attr_mx);
        return -1;
    }
    pthread_mutexattr_destroy(&t_attr_mx);
    return 0;
}

int bc_lock_deinit(pthread_mutex_t* p_mutex)
{
    return pthread_mutex_destroy(p_mutex);
}

int bc_lock(pthread_mutex_t* p_mutex)
{
    return pthread_mutex_lock(p_mutex);
}

int bc_unlock(pthread_mutex_t* p_mutex)
{
    return pthread_mutex_unlock(p_mutex);
}

void* bc_dlist_create(void)
{
    struct bc_dlist* plist = NULL;

    plist = malloc(sizeof(*plist));
    if (plist == NULL) {
        return NULL;
    }
    memset(plist, 0, sizeof(*plist));

    if (bc_lock_init(&plist->mutex) != 0) {
        free(plist);
        return NULL;
    }

    return plist;
}

int bc_dlist_count(void* list)
{
    struct bc_dlist* plist = (struct bc_dlist*)list;
    if (plist == NULL) {
        return -1;
    }
    return plist->count;
}

int bc_dlist_add_head(void* list, void* data)
{
    struct bc_dlist* plist = (struct bc_dlist*)list;
    struct bc_dlist_node* pnode = NULL;

    if (plist == NULL || data == NULL) {
        return -1;
    }

    pnode = malloc(sizeof(*pnode));
    if (pnode == NULL) {
        return -1;
    }
    memset(pnode, 0, sizeof(*pnode));

    pnode->data = data;

    bc_lock(&plist->mutex);

    pnode->prev = NULL;
    pnode->next = plist->head;

    if (plist->head) plist->head->prev = pnode;

    plist->head = pnode;
    if (plist->count == 0) plist->tail = pnode;

    plist->count++;

    bc_unlock(&plist->mutex);

    return 0;
}

int bc_dlist_add_tail(void* list, void* data)
{
    struct bc_dlist* plist = (struct bc_dlist*)list;
    struct bc_dlist_node* pnode = NULL;

    if (plist == NULL || data == NULL) {
        return -1;
    }

    pnode = malloc(sizeof(*pnode));
    if (pnode == NULL) {
        return -1;
    }
    memset(pnode, 0, sizeof(*pnode));

    pnode->data = data;

    bc_lock(&plist->mutex);

    pnode->prev = plist->tail;
    pnode->next = NULL;

    if (plist->tail) plist->tail->next = pnode;

    plist->tail = pnode;
    if (plist->count == 0) plist->head = pnode;

    plist->count++;

    bc_unlock(&plist->mutex);

    return 0;
}

void* bc_dlist_remove_head(void* list)
{
    struct bc_dlist* plist = (struct bc_dlist*)list;
    struct bc_dlist_node* pnode = NULL;
    void* data = NULL;

    if (plist == NULL || plist->count == 0) return NULL;

    bc_lock(&plist->mutex);

    if (plist->count == 0) {
        bc_unlock(&plist->mutex);
        return NULL;
    }

    pnode = plist->head;
    if (pnode->next) pnode->next->prev = NULL;
    plist->head = pnode->next;

    if (plist->count == 1) plist->tail = pnode->next;

    pnode->prev = NULL;
    pnode->next = NULL;

    plist->count--;

    bc_unlock(&plist->mutex);

    data = pnode->data;

    free(pnode);

    return data;
}

void* bc_dlist_remove_tail(void* list)
{
    struct bc_dlist* plist = (struct bc_dlist*)list;
    struct bc_dlist_node* pnode = NULL;
    void* data = NULL;

    if (plist == NULL || plist->count == 0) return NULL;

    bc_lock(&plist->mutex);

    if (plist->count == 0) {
        bc_unlock(&plist->mutex);
        return NULL;
    }

    pnode = plist->tail;
    if (pnode->prev) pnode->prev->next = NULL;
    plist->tail = pnode->prev;

    if (plist->count == 1) plist->head = pnode->prev;

    pnode->prev = NULL;
    pnode->next = NULL;

    plist->count--;

    bc_unlock(&plist->mutex);

    data = pnode->data;

    free(pnode);

    return data;
}

void* bc_dlist_search(void* list, void* org, bc_comp_fun fun)
{
    struct bc_dlist* plist = (struct bc_dlist*)list;
    struct bc_dlist_node* pnode = NULL;

    if (plist == NULL || plist->count == 0 || org == NULL) return NULL;

    bc_lock(&plist->mutex);

    for (pnode = plist->head; pnode != NULL; pnode = pnode->next) {
        if (!fun) {
            if (pnode->data != org) continue;
        } else if (fun) {
            if (fun(pnode->data, org) != 0) continue;
        }
        break;
    }

    bc_unlock(&plist->mutex);

    if (pnode) return pnode->data;
    return NULL;
}

void* bc_dlist_search_and_remove(void* list, void* org, bc_comp_fun fun)
{
    struct bc_dlist* plist = (struct bc_dlist*)list;
    struct bc_dlist_node* pnode = NULL;
    void* data = NULL;

    if (plist == NULL || plist->count == 0 || org == NULL) return NULL;

    bc_lock(&plist->mutex);

    for (pnode = plist->head; pnode != NULL; pnode = pnode->next) {
        if (!fun) {
            if (pnode->data != org) continue;
        } else if (fun) {
            if (fun(pnode->data, org) != 0) continue;
        }

        data = pnode->data;

        if (pnode == plist->head) {
            bc_dlist_remove_head(plist);
        } else if (pnode == plist->tail) {
            bc_dlist_remove_tail(plist);
        } else {
            pnode->prev->next = pnode->next;
            pnode->next->prev = pnode->prev;
            pnode->prev = pnode->next = NULL;
            plist->count--;
        }
        break;
    }

    bc_unlock(&plist->mutex);

    if (data) return data;
    return NULL;
}

void* bc_dlist_remove_by_data(void* list, void* data)
{
    return bc_dlist_search_and_remove(list, data, NULL);
}

int bc_dlist_destroy(void* list)
{
    struct bc_dlist* plist = (struct bc_dlist*)list;
    if (plist == NULL) return -1;
    if (plist->count != 0) {
        LOG("list is not empty\n");
        return -1;
    }
    bc_lock_deinit(&plist->mutex);
    free(plist);
    return 0;
}

int bc_dlist_print(void* list, bc_apply apply)
{
    struct bc_dlist* plist = (struct bc_dlist*)list;
    struct bc_dlist_node* pnode = NULL;
    int i = 0;

    if (plist == NULL) return -1;

    printf("==== list count: %d ==== head/tail: 0x%08x, 0x%08x\n", plist->count, (int)plist->head, (int)plist->tail);
    if (plist->count == 0) {
        return 0;
    }

    bc_lock(&plist->mutex);

    for (pnode = plist->head; pnode != NULL; pnode = pnode->next) {
        printf("  %3d: 0x%08x, data: 0x%08x, prev: 0x%08x, next: 0x%08x.   ", i++, (int)pnode, (int)pnode->data, (int)pnode->prev, (int)pnode->next);
        if (apply) apply(pnode->data);
        printf("\n");
        usleep(1000);
    }

    bc_unlock(&plist->mutex);

    return 0;
}

#if 0 /* for tesing only */
static struct bc_dlist* dlist = NULL;

static int _t_comp(void* data1, void* data2)
{
    if (data1 == data2) return 0;
    return -1;
}

int main(void)
{
    dlist = bc_dlist_create();
    if (dlist == NULL) {
        LOG("cr dlist error\n");
        return -1;
    }

    bc_dlist_add_head(dlist, (void*) 123);
    bc_dlist_add_head(dlist, (void*) 234);
    bc_dlist_add_tail(dlist, (void*) 345);
    bc_dlist_add_tail(dlist, (void*) 456);

    printf("dlist count: %d\n", bc_dlist_count(dlist));

    bc_dlist_remove_tail(dlist);

    bc_dlist_add_tail(dlist, (void*) 567);

    bc_dlist_remove_head(dlist);
    bc_dlist_remove_tail(dlist);
    printf("remove: %d (123)\n", (int)bc_dlist_remove_head(dlist));
    printf("dlist count: %d\n", bc_dlist_count(dlist));
    bc_dlist_remove_tail(dlist);

    printf("dlist count: %d\n", bc_dlist_count(dlist));

    printf("\n");
    /**/
    bc_dlist_add_head(dlist, (void*) 123);
    bc_dlist_add_head(dlist, (void*) 234);
    bc_dlist_add_head(dlist, (void*) 345);
    bc_dlist_add_head(dlist, (void*) 456);
    bc_dlist_add_head(dlist, (void*) 567);

    printf("remove: %d (345)\n", (int)bc_dlist_remove_by_data (dlist, (void*) 345));
    printf("search: %d (567)\n", (int)bc_dlist_search(dlist, (void*) 567, NULL));
    printf("search: %d (234)\n", (int)bc_dlist_search(dlist, (void*) 234, _t_comp));
    printf("dlist count: %d\n", bc_dlist_count(dlist));
    printf("remove: %d (0)\n", (int)bc_dlist_search_and_remove(dlist, (void*) 345, _t_comp));
    printf("dlist count: %d\n", bc_dlist_count(dlist));
    LOG("remove: %d (123)\n", (int)bc_dlist_search_and_remove(dlist, (void*) 123, _t_comp));
    printf("dlist count: %d\n", bc_dlist_count(dlist));

    if (bc_dlist_destroy(dlist) != 0) {
        LOG("destroy dlist err\n");
    }
    while (bc_dlist_remove_head(dlist) != NULL) {
        ;
    }
    if (bc_dlist_destroy(dlist) != 0) {
        LOG("destroy dlist err\n");
    }
    LOG("== test end ==%s\n", "");
    return 0;
}
#endif

