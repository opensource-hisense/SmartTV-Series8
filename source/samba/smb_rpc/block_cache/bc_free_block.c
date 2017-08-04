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

#include "bc_block_cache.h"

static void* free_block_list = NULL;

int bc_free_block_list_init(void)
{
    if (free_block_list) return 0;

    free_block_list = bc_dlist_create();
    if (free_block_list == NULL) {
        return -1;
    }

    return 0;
}

int bc_free_block_list_deinit(void)
{
    return bc_dlist_destroy(free_block_list);
}

int bc_free_block_list_count(void)
{
    if (free_block_list == NULL) return -1;
    return bc_dlist_count(free_block_list);
}

int bc_free_block_list_add_head(void* block)
{
    return bc_dlist_add_head(free_block_list, block);
}

int bc_free_block_list_add_tail(void* block)
{
    return bc_dlist_add_tail(free_block_list, block);
}

void* bc_free_block_list_remove_head(void)
{
    return bc_dlist_remove_head(free_block_list);
}

void* bc_free_block_list_remove_tail(void)
{
    return bc_dlist_remove_tail(free_block_list);
}

static int _comp_block(void* data1, void* data2)
{
    struct bc_block* b1 = (struct bc_block*)data1;
    struct bc_block* b2 = (struct bc_block*)data2;
    if (b1 == NULL || b2 == NULL) return -1;
    if (b1->path == NULL || b2->path == NULL) return -1;

    if (b1->offset != b2->offset ||
        (b1->path != b2->path && strcmp(b1->path, b2->path) != 0)) {
        return -1;
    }
    return 0;
}

void* bc_free_block_list_remove_by_block(void* block)
{
    void* data = bc_dlist_remove_by_data(free_block_list, block);
    if (data) return data;
    return bc_dlist_search_and_remove(free_block_list, block, _comp_block);
}

static void _print_block(void* data)
{
    struct bc_block* pblock = (struct bc_block*)data;
    if (pblock) {
        printf("-> status: %d, size: %d, len: %d, offset: %lld, path: %s", pblock->block_status, pblock->block_size_only, pblock->data_len, pblock->offset, pblock->path);
    }
}

int bc_free_block_list_print(void)
{
    printf ("==== free_block_list count: %d\n", bc_dlist_count(free_block_list));
    bc_dlist_print(free_block_list, _print_block);
    return 0;
}

#if 0 /* for tesing only */
int main(void)
{
    int i = 0;
    struct bc_block* pblock = NULL;

    struct bc_block bc1 = {0, "smb://192.168.1.25/video/1.avi", (off_t)9876543210LL, 65534, 655};
    struct bc_block bc2 = {0, "smb://192.168.1.25/video/1.avi", (off_t)0, 65534, 65534};
    struct bc_block bc3 = {0, "smb://192.168.1.25/video/1.avi", (off_t)65534000, 65534, 65534};
    struct bc_block bc4 = {0, "smb://192.168.1.21/usrs/dj/test/media/video/authentication/case101.mpg", (off_t)6553400, 65534, 65534};

    struct bc_block bc_i[10] = {{0},};

    bc_free_block_list_init();

    bc_free_block_list_add_head(&bc1);
    bc_free_block_list_add_tail(&bc2);

    bc_free_block_list_add_head(&bc3);

    for (i = 0; i < 10; i++) {
        bc_i[i] = bc4;
        bc_i[i].offset += i * 65534;
        bc_free_block_list_add_head(&bc_i[i]);
    }

    bc_free_block_list_print();

    i = 0;
    pblock = bc_free_block_list_remove_by_block(&bc4);
    LOG("%d, remove by block 0x%08x, %lld, %s\n", i++, (int)pblock, pblock ? pblock->offset : (off_t)-1, pblock ? pblock->path : "");

    pblock = bc_free_block_list_remove_tail();
    LOG("%d, remove tail 0x%08x, %lld, %s\n", i++, (int)pblock, pblock ? pblock->offset : (off_t)-1, pblock ? pblock->path : "");
    pblock = bc_free_block_list_remove_head();
    LOG("%d, remove head 0x%08x, %lld, %s\n", i++, (int)pblock, pblock ? pblock->offset : (off_t)-1, pblock ? pblock->path : "");
    pblock = bc_free_block_list_remove_head();
    LOG("%d, remove head 0x%08x, %lld, %s\n", i++, (int)pblock, pblock ? pblock->offset : (off_t)-1, pblock ? pblock->path : "");
    pblock = bc_free_block_list_remove_tail();
    LOG("%d, remove tail 0x%08x, %lld, %s\n", i++, (int)pblock, pblock ? pblock->offset : (off_t)-1, pblock ? pblock->path : "");
    pblock = bc_free_block_list_remove_head();
    LOG("%d, remove head 0x%08x, %lld, %s\n", i++, (int)pblock, pblock ? pblock->offset : (off_t)-1, pblock ? pblock->path : "");

    while ((pblock = bc_free_block_list_remove_tail()) != NULL) {
        LOG("%d, remove block 0x%08x, %lld, %s\n", i++, (int)pblock, pblock ? pblock->offset : (off_t)-1, pblock ? pblock->path : "");
    }

    //bc_free_block_list_remove_head();

    LOG("deinit rc %d\n", bc_free_block_list_deinit());

    return 0;
}

#endif

