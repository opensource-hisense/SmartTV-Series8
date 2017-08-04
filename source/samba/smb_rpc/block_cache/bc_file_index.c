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

static void* file_index_list = NULL;
static int handles[1024] = {-1};

int bc_file_index_list_init(void)
{
    if (file_index_list) return 0;

    file_index_list = bc_dlist_create();
    if (file_index_list == NULL) {
        return -1;
    }

    memset(handles, -1, sizeof(handles));

    return 0;
}

int bc_file_index_list_deinit(void)
{
    return bc_dlist_destroy(file_index_list);
}

int bc_file_index_list_count(void)
{
    if (file_index_list == NULL) return -1;
    return bc_dlist_count(file_index_list);
}

int bc_file_index_new_handle(void)
{
    int i = 0;
    for (i = 0; i < sizeof(handles) / sizeof(handles[0]); i++) {
        if (handles[i] == -1) {
            handles[i] = 10000 + i;
            return handles[i];
        }
    }
    return -1;
}

int bc_file_index_remove_handle(int handle)
{
    int i = 0;
    for (i = 0; i < sizeof(handles) / sizeof(handles[0]); i++) {
        if (handles[i] == handle) {
            handles[i] = -1;
            return 0;
        }
    }
    return -1;
}

struct bc_file_index {
    char* path;
    int fd;
    off_t pos;
    off_t size;
    void* offset_list;
    void* handle_list;
};

static int _comp_path_and_fd(void* data1, void* data2)
{
    struct bc_file_index* findex1 = (struct bc_file_index*)data1;
    struct bc_file_index* findex2 = (struct bc_file_index*)data2;
    if (findex1 == NULL || findex2 == NULL) {
        return -1;
    }
    if (findex1->path == NULL || findex1->fd < 0) {
        LOG("Unknown data error\n");
        return -1;
    }
    if (findex2->path == NULL && findex2->fd < 0) {
        LOG("searching data error\n");
        return -1;
    }
    if (findex2->path && strcmp(findex1->path, findex2->path) != 0) {
        return -1;
    }
    if (findex2->fd >= 0 && findex1->fd != findex2->fd) {
        return -1;
    }
    return 0;
}

int bc_file_index_list_add_offset(const char* path, int fd, off_t offset)
{
    struct bc_file_index search_index = {(char*)path, fd, 0, 0, NULL};
    struct bc_file_index* pindex = bc_dlist_search(file_index_list, &search_index, _comp_path_and_fd);
    long long int *poffset = NULL;

    if (pindex == NULL) {
        pindex = malloc (sizeof(*pindex));
        if (pindex == NULL) return -1;
        memset(pindex, 0, sizeof(*pindex));

        pindex->path = atom_strdup(path);
        pindex->fd = fd;
        pindex->offset_list = bc_dlist_create();
        pindex->handle_list = bc_dlist_create();

        if (pindex->offset_list == NULL || pindex->handle_list == NULL) {
            atom_free(pindex->path);
            free(pindex);
            return -1;
        }

        if (bc_dlist_add_head(file_index_list, pindex) != 0) {
            bc_dlist_destroy(pindex->offset_list);
            bc_dlist_destroy(pindex->handle_list);
            atom_free(pindex->path);
            free(pindex);
            return -1;
        }
    }

    poffset = malloc(sizeof(*poffset));
    if (poffset == NULL) {
        if (bc_dlist_count(pindex->offset_list) == 0 &&
            bc_dlist_count(pindex->handle_list) == 0) {
            bc_dlist_remove_by_data(file_index_list, pindex);
            bc_dlist_destroy(pindex->offset_list);
            bc_dlist_destroy(pindex->handle_list);
            atom_free(pindex->path);
            free(pindex);
            return -1;
        }
    }

    if (offset == -1) {
        /* create only */
        return 0;
    }

    *poffset = offset;

    if (bc_dlist_add_head(pindex->offset_list, (void*)poffset) != 0) {
        if (bc_dlist_count(pindex->offset_list) == 0 &&
            bc_dlist_count(pindex->handle_list) == 0) {
            bc_dlist_remove_by_data(file_index_list, pindex);
            bc_dlist_destroy(pindex->offset_list);
            bc_dlist_destroy(pindex->handle_list);
            atom_free(pindex->path);
            free(pindex);
        }
        free(poffset);
        return -1;
    }

    return 0;
}

static int _comp_offset(void* data1, void* data2)
{
    long long int* offset1 = (long long int*)data1;
    long long int* poffset = (long long int*)data2;
    if (data1 == NULL || data2 == NULL) return -1;
    if (*offset1 != *poffset) return -1;
    return 0;
}

int bc_file_index_list_remove_offset(const char* path, int fd, off_t offset)
{
    struct bc_file_index search_index = {(char*)path, fd, 0, 0, NULL};
    struct bc_file_index* pindex = bc_dlist_search(file_index_list, &search_index, _comp_path_and_fd);
    long long int* poffset = NULL;

    if (pindex == NULL) {
        return -1;
    }

    poffset = bc_dlist_search_and_remove(pindex->offset_list, &offset, _comp_offset);
    if (poffset) {
        free(poffset);
        poffset = NULL;
    }

    if (bc_dlist_count(pindex->offset_list) == 0 &&
        bc_dlist_count(pindex->handle_list) == 0) {
        bc_dlist_remove_by_data(file_index_list, pindex);
        bc_dlist_destroy(pindex->offset_list);
        bc_dlist_destroy(pindex->handle_list);
        atom_free(pindex->path);
        free(pindex);
    }

    return 0;
}

off_t bc_file_index_list_remove_offset_1by1(const char* path, int fd)
{
    struct bc_file_index search_index = {(char*)path, fd, 0, 0, NULL};
    struct bc_file_index* pindex = bc_dlist_search(file_index_list, &search_index, _comp_path_and_fd);
    long long int* poffset = NULL;
    off_t offset = -1;

    if (pindex == NULL) {
        return -1;
    }

    poffset = bc_dlist_remove_tail(pindex->offset_list);
    if (poffset == NULL) return -1;

    offset = *poffset;
    free(poffset);
    poffset = NULL;

    if (bc_dlist_count(pindex->offset_list) == 0 &&
        bc_dlist_count(pindex->handle_list) == 0) {
        bc_dlist_remove_by_data(file_index_list, pindex);
        bc_dlist_destroy(pindex->offset_list);
        bc_dlist_destroy(pindex->handle_list);
        atom_free(pindex->path);
        free(pindex);
    }

    return offset;
}

struct handle_pos_map {
    int handle;
    off_t pos;
};

/* handle */
int bc_file_index_list_add_handle(const char* path, int fd, int handle, off_t size)
{
    struct bc_file_index search_index = {(char*)path, fd, 0, 0, NULL};
    struct bc_file_index* pindex = bc_dlist_search(file_index_list, &search_index, _comp_path_and_fd);
    struct handle_pos_map* phandle = NULL;

    if (pindex == NULL) {
        pindex = malloc (sizeof(*pindex));
        if (pindex == NULL) return -1;
        memset(pindex, 0, sizeof(*pindex));

        pindex->path = atom_strdup(path);
        pindex->fd = fd;
        pindex->size = size;
        pindex->offset_list = bc_dlist_create();
        pindex->handle_list = bc_dlist_create();

        if (pindex->offset_list == NULL || pindex->handle_list == NULL) {
            atom_free(pindex->path);
            free(pindex);
            return -1;
        }

        if (bc_dlist_add_head(file_index_list, pindex) != 0) {
            bc_dlist_destroy(pindex->offset_list);
            bc_dlist_destroy(pindex->handle_list);
            atom_free(pindex->path);
            free(pindex);
            return -1;
        }
    }

    if (handle == -1) {
        /* create only */
        return 0;
    }

    phandle = malloc(sizeof(*phandle));
    if (phandle == NULL) return 0;
    phandle->handle = handle;
    phandle->pos = 0;

    if (bc_dlist_add_head(pindex->handle_list, (void*)phandle) != 0) {
        if (bc_dlist_count(pindex->offset_list) == 0 &&
            bc_dlist_count(pindex->handle_list) == 0) {
            bc_dlist_remove_by_data(file_index_list, pindex);
            bc_dlist_destroy(pindex->offset_list);
            bc_dlist_destroy(pindex->handle_list);
            atom_free(pindex->path);
            free(pindex);
        }
        free(phandle);
        return -1;
    }

    return 0;
}

static int _comp_handle(void* data1, void* data2)
{
#if 0
    int* handle1 = (int*)data1;
    struct handle_offset_map* phandle = (struct handle_offset_map*)data2;
    if (data1 == NULL || data2 == NULL) return -1;
    if (*handle1 != phandle->handle) return -1;
#else
    struct handle_pos_map* phandle = (struct handle_pos_map*)data1;
    int handle = (int)data2;
    if (data1 == NULL || handle < 0) return -1;
    if (phandle->handle != handle) return -1;
#endif
    return 0;
}

int bc_file_index_list_get_handle_count(const char* path, int fd)
{
    struct bc_file_index search_index = {(char*)path, fd, 0, 0, NULL};
    struct bc_file_index* pindex = bc_dlist_search(file_index_list, &search_index, _comp_path_and_fd);

    if (pindex == NULL) {
        return -1;
    }

    return bc_dlist_count(pindex->handle_list);
}

int bc_file_index_list_remove_handle(const char* path, int fd, int handle)
{
    struct bc_file_index search_index = {(char*)path, fd, 0, 0, NULL};
    struct bc_file_index* pindex = bc_dlist_search(file_index_list, &search_index, _comp_path_and_fd);
    struct handle_pos_map* phandle = NULL;

    if (pindex == NULL) {
        LOG("error not found file index\n");
        return -1;
    }

    phandle = bc_dlist_search_and_remove(pindex->handle_list, (void*)handle, _comp_handle);
    if (phandle && phandle->handle != handle) {
        LOG("handle %d not removed (%d)\n", handle, phandle->handle);
    }

    /* ?? */
    if (bc_dlist_count(pindex->offset_list) == 0 &&
        bc_dlist_count(pindex->handle_list) == 0) {
        LOG("not found any all handle & offset...\n");
        bc_dlist_remove_by_data(file_index_list, pindex);
        bc_dlist_destroy(pindex->offset_list);
        bc_dlist_destroy(pindex->handle_list);
        atom_free(pindex->path);
        free(pindex);
    }

    return 0;
}

static void _print_offset(void* data)
{
    long long int* poffset = (long long int*)data;
    if (poffset) printf("--> offset: %lld ", *poffset);
}

static void _print_handle(void* data)
{
    struct handle_pos_map* phandle = (struct handle_pos_map*)data;
    if (phandle) printf("--> handle: %d, pos: %lld ", phandle->handle, phandle->pos);
}

static void _print_index(void* data)
{
    struct bc_file_index* pindex = (struct bc_file_index*)data;
    if (pindex) {
        printf("-> fd: %3d, offset_count: %d, handle_count: %d, path: 0x%08x, %s\n", pindex->fd, bc_dlist_count(pindex->offset_list), bc_dlist_count(pindex->handle_list), (int)pindex->path, pindex->path);
        bc_dlist_print(pindex->offset_list, _print_offset);
        bc_dlist_print(pindex->handle_list, _print_handle);
    }
}

int bc_file_index_list_print(void)
{
    printf ("==== file_index_list count: %d\n", bc_dlist_count(file_index_list));
    bc_dlist_print(file_index_list, _print_index);
    return 0;
}

static int _file_index_comp_handle(void* data1, void* data2)
{
    struct bc_file_index* pindex = (struct bc_file_index*)data1;
#if 0
    struct handle_pos_map* phandle = (struct handle_pos_map*)data2;

    if (phandle == NULL || phandle->handle < 0) return -1;

    if (bc_dlist_search(pindex->handle_list, (void*)phandle->handle, _comp_handle) == NULL) return -1;
#else
    int handle = (int)data2;

    if (bc_dlist_search(pindex->handle_list, (void*)handle, _comp_handle) == NULL) return -1;
#endif

    return 0;
}

int bc_file_index_list_search_fd_by_handle(int handle)
{
    struct bc_file_index* findex1 = bc_dlist_search(file_index_list, (void*)handle, _file_index_comp_handle);
    if (findex1 == NULL) return -1;
    return findex1->fd;
}

static int _comp_path(void* data1, void* data2)
{
    struct bc_file_index* findex1 = (struct bc_file_index*)data1;
    char* path = (char*)data2;

    if (path == NULL) return -1;

    if (findex1->path == NULL) return -1;

    if (findex1->path != path && strcmp(findex1->path, path) != 0) return -1;
    return 0;
}

int bc_file_index_list_search_fd_by_path(const char* path)
{
    struct bc_file_index* findex1 = bc_dlist_search(file_index_list, (void*)path, _comp_path);
    if (findex1 == NULL) return -1;
    return findex1->fd;
}

char* bc_file_index_list_search_path_by_handle(int handle)
{
    struct bc_file_index* findex1 = bc_dlist_search(file_index_list, (void*)handle, _file_index_comp_handle);
    if (findex1 == NULL) return NULL;
    return findex1->path;
}

static int _comp_fd(void* data1, void* data2)
{
    struct bc_file_index* findex1 = (struct bc_file_index*)data1;
    int fd = (int)data2;
    if (fd < 0) return -1;
    if (findex1->fd != fd) return -1;
    return 0;
}

off_t bc_file_index_list_get_pos_by_fd(int fd)
{
    struct bc_file_index* findex1 = bc_dlist_search(file_index_list, (void*)fd, _comp_fd);
    if (findex1 == NULL) return -1;
    return findex1->pos;
}

int bc_file_index_list_update_pos_by_fd(int fd, off_t pos)
{
    struct bc_file_index* findex1 = bc_dlist_search(file_index_list, (void*)fd, _comp_fd);
    if (findex1 == NULL) return -1;
    findex1->pos = pos;
    return 0;
}

char* bc_file_index_list_search_path_by_fd(int fd)
{
    struct bc_file_index* findex1 = bc_dlist_search(file_index_list, (void*)fd, _comp_fd);
    if (findex1 == NULL) return NULL;
    return findex1->path;
}

int bc_file_index_list_search_handle_info_by_handle(int handle, char* path, int* fd, off_t *fd_pos, off_t* size, off_t *handle_pos)
{
    struct bc_file_index* findex1 = bc_dlist_search(file_index_list, (void*)handle, _file_index_comp_handle);
    struct handle_pos_map* phandle = NULL;

    if (findex1 == NULL) return -1;

    if (fd) *fd = findex1->fd;
    if (path) strcpy(path, findex1->path);
    if (fd_pos) *fd_pos = findex1->pos;
    if (size) *size = findex1->size;

    if (handle_pos) {
        phandle = bc_dlist_search(findex1->handle_list, (void*)handle, _comp_handle);
        if (phandle == NULL) return -1;
        *handle_pos = phandle->pos;
    }

    return 0;
}

off_t bc_file_index_list_get_handle_pos_by_handle(int handle)
{
    struct bc_file_index* findex1 = bc_dlist_search(file_index_list, (void*)handle, _file_index_comp_handle);
    struct handle_pos_map* phandle = NULL;

    if (findex1 == NULL) return -1;

    phandle = bc_dlist_search(findex1->handle_list, (void*)handle, _comp_handle);
    if (phandle == NULL) return -1;

    return phandle->pos;
}

off_t bc_file_index_list_update_handle_pos_by_handle(int handle, off_t pos)
{
    struct bc_file_index* findex1 = bc_dlist_search(file_index_list, (void*)handle, _file_index_comp_handle);
    struct handle_pos_map* phandle = NULL;

    if (findex1 == NULL) return -1;

    phandle = bc_dlist_search(findex1->handle_list, (void*)handle, _comp_handle);
    if (phandle == NULL) return -1;
    phandle->pos = pos;

    return pos;
}

off_t bc_file_index_list_update_fd_pos_by_handle(int handle, off_t pos)
{
    struct bc_file_index* findex1 = bc_dlist_search(file_index_list, (void*)handle, _file_index_comp_handle);

    if (findex1 == NULL) return -1;

    findex1->pos = pos;

    return pos;
}

#if 0 /* for tesing only */
int main(void)
{
    int i = 0;
    off_t offset = -1;

    bc_file_index_list_init();

    bc_file_index_list_add_fd_offset("smb://192.168.1.25/video/1.avi", 10001, -1);
    bc_file_index_list_add_handle("smb://192.168.1.25/video/1.avi", 10001, -1);

    bc_file_index_list_add_fd_offset("smb://192.168.1.25/video/1.avi", 10001, 0);
    bc_file_index_list_add_fd_offset("smb://192.168.1.23/media/just for test/pre-test/video/mp4/from_customer/16678.mp4", 10006, 65534);
    bc_file_index_list_add_fd_offset("smb://192.168.1.25/video/1.avi", 10001, (long long int)9876543210LL);

    /* handle */
    bc_file_index_list_add_handle("smb://192.168.1.25/video/1.avi", 10001, 10000);
    bc_file_index_list_add_handle("smb://192.168.1.25/video/1.avi", 10001, 10001);
    bc_file_index_list_add_handle("smb://192.168.1.25/video/1.avi", 10001, 10002);
    bc_file_index_list_add_handle("smb://192.168.1.25/video/1.avi", 10001, 10003);

    for (i = 0; i < 10; i++) {
        bc_file_index_list_add_fd_offset("smb://192.168.1.21/usrs/dj/test/media/video/authentication/case101.mpg", 10003, 65534*i);
    }

    bc_file_index_list_print();

    bc_file_index_list_remove_offset("smb://192.168.1.25/video/1.avi", 10001, 0);
    bc_file_index_list_remove_offset("smb://192.168.1.23/media/just for test/pre-test/video/mp4/from_customer/16678.mp4", 10006, 65534);

    while ((offset = bc_file_index_list_remove_offset_1by1("smb://192.168.1.21/usrs/dj/test/media/video/authentication/case101.mpg", 10003)) != -1) {
        LOG ("remove offset %lld\n", offset);
    }

    bc_file_index_list_remove_offset("smb://192.168.1.25/video/1.avi", 10001, (long long int)9876543210LL);

    /* handle */
    bc_file_index_list_remove_handle("smb://192.168.1.25/video/1.avi", 10001, 10000);
    bc_file_index_list_remove_handle("smb://192.168.1.25/video/1.avi", 10001, 10001);
    bc_file_index_list_remove_handle("smb://192.168.1.25/video/1.avi", 10001, 10002);
    bc_file_index_list_remove_handle("smb://192.168.1.25/video/1.avi", 10001, 10003);

    LOG("deinit rc %d\n", bc_file_index_list_deinit());

    return 0;
}

#endif

