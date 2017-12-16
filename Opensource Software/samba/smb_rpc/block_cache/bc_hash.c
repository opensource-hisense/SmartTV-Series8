#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <fcntl.h>
#include <pthread.h>

#include "atom_strdup.h"
#include "bc_dlist.h"

#include "bc_hash.h"

struct bc_hash {
    int hsize;
    int count;
    hash_fun hfun;
    void* tag;
    struct bc_dlist** buckets;
};

static int hsizes[] = {67, 257, 521, 1031, 2053, 4093 };
static int _bc_hash_rec_size (int i4_size)
{
    if (i4_size <= 0) return 1031;

    {
        int i, hlen = sizeof (hsizes) / sizeof (hsizes[0]);
        for (i = 0; i < hlen; i++)
        {
            if (hsizes[i] >= i4_size) return hsizes[i];
        }
        return hsizes[hlen-1];
    }
}

void* bc_hash_get_tag(void* hash)
{
    struct bc_hash* phash = (struct bc_hash*)hash;
    if (phash == NULL) return NULL;
    return phash->tag;
}

void* bc_hash_create(unsigned int hsize, void* tag, hash_fun fun)
{
    struct bc_hash* phash = NULL;
    int i = 0;

    if (fun == NULL) return NULL;
    if (hsize <= 0) hsize = 512;

    phash = malloc(sizeof(*phash));
    if (phash == NULL) return NULL;
    memset(phash, 0, sizeof(*phash));

    phash->hsize = _bc_hash_rec_size (hsize);
    phash->count = 0;
    phash->hfun = fun;
    phash->tag = tag;
    phash->buckets = calloc(1, phash->hsize * sizeof (*phash->buckets));
    if (phash->buckets == NULL) {
        free(phash);
        return NULL;
    }

    for (i = 0; i < phash->hsize; i++) {
        if ((phash->buckets[i] = bc_dlist_create()) == NULL) {
            LOG("%d create dlist error\n", i);
        }
    }
    return phash;
}

int bc_hash_destroy (void* hash)
{
    struct bc_hash* phash = (struct bc_hash*)hash;
    int i = 0;

    if (phash == NULL) return -1;

    for (i = 0; i < phash->hsize; i++) {
        if (bc_dlist_destroy(phash->buckets[i]) != 0) {
            LOG("%d destroy hash dlist error\n", i);
        }
        phash->buckets[i] = NULL;
    }

    free(phash->buckets);
    free(phash);

    return 0;
}

static struct bc_dlist* bc_hash_get_bucket(void* hash, void* data)
{
    struct bc_hash* phash = (struct bc_hash*)hash;
    int hvalue = -1;

    if (phash == NULL) return NULL;

    hvalue = phash->hfun(data) % phash->hsize;
    return phash->buckets[hvalue];
}

void* bc_hash_remove_by_data(void* hash, void *data)
{
    if (data == NULL) return NULL;
    return bc_dlist_remove_by_data(bc_hash_get_bucket(hash, data), data);
}

void* bc_hash_remove(void* hash, void *data, bc_comp_fun comp_fun)
{
    if (data == NULL) return NULL;
    return bc_dlist_search_and_remove(bc_hash_get_bucket(hash, data), data, comp_fun);
}

int bc_hash_add(void* hash, void *data)
{
    if (data == NULL) return -1;
    return bc_dlist_add_head(bc_hash_get_bucket(hash, data), data);
}

static int _comp_data_internal(void* data1, void* data2)
{
    return data1 != data2;
}

void* bc_hash_search_by_data(void* hash, void* data)
{
    return bc_dlist_search(bc_hash_get_bucket(hash, data), data, _comp_data_internal);
    /*return bc_dlist_search(bc_hash_get_bucket(hash, data), data, NULL);*/
}

void* bc_hash_search(void* hash, void *data, bc_comp_fun comp_fun)
{
    return bc_dlist_search(bc_hash_get_bucket(hash, data), data, comp_fun);
}

int bc_hash_dump (void* hash)
{
    struct bc_hash *phash = (struct bc_hash*)hash;
    int i = 0;
    int hitem_total = 0;
    int hitem_count = 0;

    if (phash == NULL) return -1;

    for (i = 0; i < phash->hsize; i++)
    {
        int hitem_cnt = 0;

        hitem_cnt = bc_dlist_count(phash->buckets[i]);

        if (hitem_cnt <= 0) continue;

        hitem_total += hitem_cnt;
        ++hitem_count;
        LOG("- hash.buckets[%4d]: %d\n", i, hitem_cnt);
    }
    LOG("- hash total buckets size: %d (buckets: %d)\n", hitem_total, hitem_count);

    return 0;
}

#if 0 /* for tesing only */
struct test_hash_node {
    volatile int block_status;
    char* path;
    off_t offset;
    int block_size_only;
    int data_len;
    char data[1];
};

static int test_hash_fun(void* data)
{
    struct test_hash_node* node = (struct test_hash_node*)data;
    char* p = NULL;
    int hvalue = 0;

    if (node == NULL) return -1;

    p = node->path;
    if (p) {
        int v = 0;
        --p;
        while ((v = *++p) != 0) {
            hvalue += v;
        }
    }

    hvalue += (unsigned int)node->offset;
    if (hvalue < 0) {
        hvalue = 0 - hvalue;
    }

    return hvalue;
}

static int test_node_comp(void* data1, void* data2)
{
    struct test_hash_node* pnode1 = (struct test_hash_node*)data1;
    struct test_hash_node* pnode2 = (struct test_hash_node*)data2;
    if (pnode1 == NULL || pnode2 == NULL) return -1;
    if (pnode1->offset != pnode2->offset || (pnode1->path != pnode2->path && strcmp(pnode1->path, pnode2->path) != 0)) {
        /*LOG("offset: %lld <-> %lld, path: %s <-> %s\n", pnode1->offset, pnode2->offset, pnode1->path, pnode2->path);*/
        return -1;
    }
    return 0;
}

#define TEST_NODE_NUM 1000000
int main()
{
    /*struct test_hash_node node[TEST_NODE_NUM] = {{0},};*/
    struct test_hash_node* node = NULL;
    struct test_hash_node node_s = {0};
    struct test_hash_node* pnode = NULL;
    int i = 0;

    void* hash = NULL;

    node = calloc(1, sizeof(*node)*TEST_NODE_NUM);
    if (node == NULL) return -1;

    hash = bc_hash_create(5000, NULL, test_hash_fun);

    for (i = 0; i < TEST_NODE_NUM; i++) {
        char path[256] = "";
        if ((i % 3) == 1) {
            sprintf(path, "smb://192.168.1.2/video/%d.avi", i+1);
        } else if ((i % 3) == 2) {
            sprintf(path, "smb://192.168.1.2/video/new folder/%d.mpg", i+1);
        } else {
            sprintf(path, "smb://192.168.1.2/video/new folder2/mp4/pre-test/for qa test only/%d.mp4", i+1);
        }
        node[i].path = strdup(path);
        node[i].offset = 65534 * i;
        node[i].block_size_only = 65534;
        node[i].data_len = 65534;
        bc_hash_add(hash, &node[i]);
    }

    /* print hash */
    bc_hash_dump (hash);

    pnode = bc_hash_search_by_data(hash, &node[3]);
    if (pnode) {
        LOG("search node3: 0x%08x, path: %s, offset: %lld\n", (int)pnode, pnode->path, pnode->offset);
    } else {
        LOG("==== search node3 error ====\n");
    }

    /*LOG("node[0]: 0x%08x\n", (int)&node[0]);*/
    pnode = bc_hash_search_by_data(hash, &node[0]);
    if (pnode) {
        LOG("search data node0: 0x%08x, path: %s, offset: %lld\n", (int)pnode, pnode->path, pnode->offset);
    } else {
        LOG("==== search data node0 error ====\n");
    }
    pnode = bc_hash_remove_by_data(hash, &node[0]);
    if (pnode) {
        LOG("remove data node0: 0x%08x, path: %s, offset: %lld\n", (int)pnode, pnode->path, pnode->offset);
        free(pnode->path);
        pnode->path = NULL;
    } else {
        LOG("==== remove data node0 error ====\n");
    }

    node_s = node[TEST_NODE_NUM-1];
    pnode = bc_hash_search(hash, &node_s, test_node_comp);
    if (pnode) {
        LOG("search node%d: 0x%08x, path: %s, offset: %lld\n", TEST_NODE_NUM-1, (int)pnode, pnode->path, pnode->offset);
    } else {
        LOG("==== search node%d error ====\n", TEST_NODE_NUM-1);
    }
    pnode = bc_hash_remove(hash, &node_s, test_node_comp);
    if (pnode) {
        LOG("remove node%d: 0x%08x, path: %s, offset: %lld\n", TEST_NODE_NUM-1, (int)pnode, pnode->path, pnode->offset);
        free(pnode->path);
        pnode->path = NULL;
    } else {
        LOG("==== remove node%d error ====\n", TEST_NODE_NUM-1);
    }

    for (i = 0; i < TEST_NODE_NUM; i++)
    {
        if ((i % 2) == 0) {
            node_s = node[i];
            pnode = bc_hash_remove(hash, &node_s, test_node_comp);
        } else {
            pnode = bc_hash_remove_by_data(hash, &node[i]);
        }
        if (pnode) {
            free(pnode->path);
            pnode->path = NULL;
        }
    }

    LOG("hash destroy rc %d\n", bc_hash_destroy(hash));
    return 0;
}
#endif

