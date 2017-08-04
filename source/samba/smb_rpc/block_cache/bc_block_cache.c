#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/stat.h>

#include "atom_strdup.h"
#include "bc_dlist.h"

#include "bc_block_cache.h"
#include "bc_hash.h"
#include "bc_free_block.h"
#include "bc_file_index.h"

static int _hash_fun(void* data)
{
    struct bc_block* node = (struct bc_block*)data;
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

static int _align_4byte(int len)
{
    return (len + 3) / 4 * 4;
}

struct bc_tag {
    int blocks_per_task;
    int block_size;
    int threshold_percent;
};

void* bc_create(int blocks_per_task, int block_size, int threshold_percent)
{
    struct bc_tag* tag = NULL;
    void* hash = NULL;
    int i = 0;

    tag = malloc(sizeof(*tag));
    if (tag == NULL) return NULL;

    tag->blocks_per_task = blocks_per_task;
    tag->block_size = block_size;
    tag->threshold_percent = threshold_percent;

    hash = bc_hash_create(blocks_per_task / 3, (void*)tag, _hash_fun);
    if (hash == NULL) {
        return NULL;
    }

    /* allocate blocks' memory */
    bc_free_block_list_init();

    for (i = 0; i < tag->blocks_per_task; i++) {
        struct bc_block* block = calloc(1, sizeof(*block)+_align_4byte(block_size)); /* may be more than 4-byte, but it is O.K. */
        if (block == NULL) {
            LOG("Error: out of memory\n");
            return NULL;
        }
        block->block_size_only = tag->block_size;
        bc_free_block_list_add_head(block);
    }

    return hash;
}

typedef int (*_io_open)(const char* path, int flags, mode_t mode);
typedef int (*_io_close)(int fd);
typedef ssize_t (*_io_read)(int fd, void* buf, size_t count);
typedef off_t (*_io_lseek)(int fd, off_t offset, int whence);
typedef int (*_io_stat)(const char*, struct stat*);

static _io_open io_open;
static _io_read io_read;
static _io_lseek io_lseek;
static _io_close io_close;
static _io_stat io_stat;

int bc_set_io_fun(_io_open nio_open, _io_read nio_read, _io_lseek nio_lseek, _io_close nio_close, _io_stat nio_stat)
{
    io_open = nio_open;
    io_read = nio_read;
    io_lseek = nio_lseek;
    io_close = nio_close;
    io_stat = nio_stat;
    LOG("IO >>> 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x <<<\n", (int)io_open, (int)io_read, (int)io_lseek, (int)io_close, (int)io_stat);
    return 0;
}

int bc_destroy(void* bc)
{
    int count = bc_free_block_list_count();
    if (count != 0) {
        LOG("== bc: 0x%08x, free block count: %d ==\n", (int)bc, count);
    }
    bc_free_block_list_deinit();
    return bc_hash_destroy (bc);
}

static off_t _align_block(off_t offset, int block_size)
{
    return ((offset + block_size) / block_size - 1) * block_size;
}

static int _block_comp_fun(void* data1, void* data2)
{
    struct bc_block* blk1 = (struct bc_block*)data1;
    struct bc_block* blk2 = (struct bc_block*)data2;

    if (blk1 == NULL || blk2 == NULL) return -1;
    if ((blk1->fd < 0 && blk2->fd < 0) && (blk1->path == NULL && blk2->path == NULL)) return -1;

    if (blk1->offset == blk2->offset) {
        if (blk2->fd < 0 && (blk1->path == blk2->path || strcmp(blk1->path, blk2->path) == 0)) return 0;
        if (blk2->path == NULL && blk1->fd == blk2->fd) return 0;
        if (blk2->fd == blk2->fd && (blk1->path == blk2->path || strcmp(blk1->path, blk2->path) == 0)) return 0;
    }

    return -1;
}

static void* bc_get_block(void* bc, const char* path, int fd, off_t offset)
{
    struct bc_block blk = {-1, (char*)path, fd, offset, 0, 0, {0}};
    struct bc_tag* tag = NULL;
    struct bc_block* pblock = NULL;
    int x = 0;

    tag = bc_hash_get_tag(bc);
    if (tag == NULL) return NULL;

    blk.offset = _align_block(offset, tag->block_size);
    /*if (blk.offset != offset) {
        LOG("offset: %lld -> %lld\n", offset, blk.offset);
    }*/

    pblock = bc_hash_search(bc, &blk, _block_comp_fun);
    if (pblock) {
        while (pblock->block_status == 1) { /* lock */
            usleep(1000);
            if ((++x % 1000) == 0) {
                LOG("??? loop wait: block info: %d, %d, %lld, %s\n", pblock->block_status, pblock->fd, pblock->offset, pblock->path);
            }
        }
    }

    return pblock;
}

static void* bc_get_free_block(void* bc)
{
    struct bc_block* block = bc_free_block_list_remove_head();
    if (block) {
        bc_hash_remove_by_data(bc, block);
        /*LOG("== FREE BLOCK: status: %d, path: %s, offset: %lld, block_size: %d, data_len: %d ==\n", block->block_status, block->path, block->offset, block->block_size_only, block->data_len);*/
    }
    return block;
}

static void* bc_g = NULL;

int bc_init(int blocks_per_task, int block_size, int threshold_percent)
{
    if (bc_g == NULL) {
        if ((bc_g = bc_create(blocks_per_task, block_size, threshold_percent)) == NULL) {
            LOG("bc_create error\n");
            return -1;
        }
    }
    return 0;
}

int bc_deinit(void)
{
    if (bc_g) bc_destroy(bc_g);
    bc_g = NULL;
    return 0;
}

/* return handle */
int bc_open(const char* path, int flags, mode_t mode)
{
    int handle = -1;
    int fd = -1;
    off_t size = 0;

    LOG("==== open(%s)\n", path);

    if (bc_file_index_list_init() != 0) {
        LOG("bc_file_index_list_init error\n");
    }

    handle = bc_file_index_new_handle();
    if (handle < 0) {
        LOG("new handle error\n");
        return -1;
    }

    fd = bc_file_index_list_search_fd_by_path(path);
    if (fd < 0) {
        if (io_open) fd = io_open(path, flags, mode);
        if (fd < 0) {
            bc_file_index_remove_handle(handle);
            return -1;
        }
    }

    {
        struct stat buf;
        int rc = io_stat(path, &buf);
        if (rc == 0) size = buf.st_size;
        LOG("size: %lld\n", size);
    }

    bc_file_index_list_add_handle(path, fd, handle, size);

    return handle;
}

int bc_read(int handle, void* buf, size_t count)
{
    int fd = -1;
    off_t handle_pos = 0;
    off_t fd_pos = 0;
    off_t size = 0;
    struct bc_block* blk = NULL;
    char path[4096] = "";
    struct bc_tag* tag = NULL;

    /*LOG("==== read(handle %d, buf 0x%08x, count: %u)\n", handle, (int)buf, count);*/

    if (bc_file_index_list_search_handle_info_by_handle(handle, path, &fd, &fd_pos, &size, &handle_pos) != 0) {
        LOG("error handle not found\n");
        return -2;
    }

    if (size > 0 && handle_pos == size) {
        LOG("EOF\n");
        return 0;
    }

    blk = bc_get_block(bc_g, path, fd, handle_pos);
    /*LOG("fd: %d, handle_pos: %lld, blk: 0x%08x\n", fd, handle_pos, (int)blk);*/

    if (blk == NULL) {
    tag = bc_hash_get_tag(bc_g);
    if (tag == NULL) {
        LOG("no tag\n");
        return -3;
    }

    blk = bc_get_free_block(bc_g);
    if (blk == NULL) {
        LOG("no free block\n");
        return -4;
    }

    if (blk->path && fd >= 0) {
        bc_file_index_list_remove_offset(blk->path, blk->fd, blk->offset);
        bc_hash_remove_by_data(bc_g, blk);
    }

    blk->block_status = 1; /* locked */

    if (blk->path) atom_free(blk->path);
    blk->path = atom_strdup(path);
    blk->fd = fd;
    blk->offset = _align_block(handle_pos, tag->block_size);
    blk->block_size_only = tag->block_size;
    blk->data_len = 0;

    {
        int rlen = 0;
        int rtotal = 0;
        /*off_t pos = bc_file_index_list_get_pos_by_fd(blk->fd);*/
        /* seek */
        if (fd_pos != handle_pos) {
        /*if (_align_block(fd_pos, tag->block_size) != _align_block(handle_pos, tag->block_size)) {*/
            if (io_lseek(fd, blk->offset, SEEK_SET) < 0) {
                blk->block_status = 3; /* freed */
                bc_free_block_list_add_tail(blk);
                LOG("seek error\n");
                return -5;
            }
            /*LOG("seek-> handle %d, fd %d seek -> %lld\n", handle, fd, blk->offset);*/
        }
        while ((rlen = io_read(fd, blk->data + rtotal, tag->block_size - rtotal)) > 0) {
            rtotal += rlen;
            if (rtotal >= blk->block_size_only) break;
        }
        blk->data_len = rtotal;
        if (rlen < 0) {
            LOG("io_Read error\n");
        }
        bc_file_index_list_update_pos_by_fd(blk->fd, blk->offset + rtotal);
        bc_hash_add(bc_g, blk);
        bc_file_index_list_add_offset(blk->path, blk->fd, blk->offset);
        /*LOG("update fd %d pos: %lld\n", fd, blk->offset + rtotal);*/
    }

    blk->block_status = 2;
    }

    if (blk) {
        int blk_x = blk->data_len - (handle_pos - blk->offset);
        int return_count = count;
        /*LOG("blk_x: %d = blk->data_len: %d - (handle_pos: %lld - blk->offset: %lld)\n", blk_x, blk->data_len, handle_pos, blk->offset);*/
        if (blk_x < count) return_count = blk_x;
        memcpy(buf, blk->data + (handle_pos - blk->offset), return_count);
        if (return_count >= blk->data_len) {
            /* add free block list */
            bc_file_index_list_remove_offset(blk->path, blk->fd, blk->offset);
            bc_free_block_list_add_tail(blk);
        }
        bc_file_index_list_update_handle_pos_by_handle(handle, handle_pos + return_count);
        /*LOG("update handle %d pos: %lld\n", handle, handle_pos + return_count);*/
        /*LOG("read rc %d\n", return_count);*/
        return return_count;
    }

    LOG("no blk\n");
    return -1;
}

off_t bc_lseek(int handle, off_t offset, int whence)
{
    off_t handle_pos = 0;
    off_t size = 0;

    /*LOG("==== lseek(handle %d, offset: %lld, whence: %d)\n", handle, offset, whence);*/
    if (bc_file_index_list_search_handle_info_by_handle(handle, NULL, NULL, NULL, &size, &handle_pos) != 0) return -1;

    if ((whence == SEEK_SET && handle_pos == offset) || (whence == SEEK_CUR && offset == 0)) return handle_pos;

    if (whence == SEEK_SET) {
        handle_pos = offset;
    } else if (whence == SEEK_CUR) {
        handle_pos += offset;
    } else if (whence == SEEK_END) {
        if (size <= 0) {
            LOG("Unknown file size\n");
            return -1;
        }
        handle_pos = size + offset;
    }

    size = bc_file_index_list_update_handle_pos_by_handle(handle, handle_pos);
    /*LOG("rc %lld\n", size);*/
    return size;
}

int bc_close(int handle)
{
    /* TODO */
    /*
     * handle -> fd
     * remove file_index handle and all offset
     * all offset <-> block add into free list (add_tail)
     * if all handle removed, del and close fd
     */
    int fd = -1;
    int count = 0;
    char path[4096] = "";

    LOG("==== close(handle: %d)\n", handle);
    if (bc_file_index_list_search_handle_info_by_handle(handle, path, &fd, NULL, NULL, NULL) != 0) return -1;

    LOG("fd: %d, path: %s\n", fd, path);
    bc_file_index_list_remove_handle(path, fd, handle);
    bc_file_index_remove_handle(handle);

    count = bc_file_index_list_get_handle_count(path, fd);
    LOG("count: %d\n", count);
    if (count <= 0) {
        /* free all fd's blocks */
        off_t offset = -1;
        while ((offset = bc_file_index_list_remove_offset_1by1(path, fd)) != -1) {
            struct bc_block blk = {-1, (char*)path, fd, offset, 0, 0, {0}};
            struct bc_tag* tag = bc_hash_get_tag(bc_g);
            struct bc_block* pblock = NULL;

            if (tag == NULL) continue;

            blk.offset = _align_block(offset, tag->block_size);
            pblock = bc_hash_search(bc_g, &blk, _block_comp_fun);
            if (pblock) {
                bc_hash_remove_by_data(bc_g, pblock);
                bc_free_block_list_add_head(pblock);
            }
        }
        if (fd >= 0) io_close(fd);
    }

    return 0;
}

#if 0 /* for tesing only */
#if 0
static int _my_open(const char* path, int flags, mode_t mode)
{
    LOG("==== open(%s)\n", path);
    return open(path, flags);
}

static int _my_close(int fd)
{
    LOG("==== close(%d)\n", fd);
    return close(fd);
}

static int _my_stat(const char* path, struct stat* buf)
{
    LOG("==== stat(%s)\n", path);
    return stat(path, buf);
}

ssize_t _my_read(int fd, void* buf, size_t count)
{
    int rlen = -1;
    int en = 0;
    rlen = read(fd, buf, count);
    en = errno;
    LOG("==== read(%d,0x%08x,%d) rc %d", fd, (int)buf, count, rlen);
    if (rlen < 0) {
        fprintf(stderr,", %d, %s\n", en, strerror(en));
        LOG("---- current pos: %lld\n", lseek(fd, 0, SEEK_CUR));
    } else {
        fprintf(stderr,"\n");
    }
    fflush(stderr);
    return rlen;
}

static off_t _my_lseek(int fd, off_t offset, int whence)
{
    LOG("==== lseek(%d, %lld, %d)\n", fd, offset, whence);
    return lseek(fd, offset, whence);
}
#endif

int main(void)
{
    void* bc = bc_create(100, 65534, 90);
    /*struct bc_block* block = NULL;
    int i = 0;*/

#if 0
    bc_set_io_fun((_io_open)_my_open, _my_read, _my_lseek, _my_close, _my_stat);
#else
    bc_set_io_fun((_io_open)open, read, lseek, close, stat);
#endif

    LOG("bc: 0x%08x, free count: %d\n", (int)bc, bc_free_block_list_count());

#if 0
    for (i = 0; i < 110; i++) {
        block = bc_get_free_block(bc);
        if (block == NULL) {
            LOG("%d no free block\n", i);
            break;
        }
        block->block_status = 2;
        /*block->path = atom_strdup("smb://192.68.1.2/video/1.avi");*/
        block->path = atom_strdup("./Makefile");
        block->fd = 3;
        block->offset = i * block->block_size_only;

        bc_hash_add(bc, block);
    }

    bc_hash_dump (bc);

    for (i = 0; i < 500; i++) {
        off_t offset = i * 65534 / 3;
        /*block = bc_get_block(bc, "smb://192.68.1.2/video/1.avi", -1, offset);*/
        block = bc_get_block(bc, "./Makefile", -1, offset);
        if (block == NULL) {
            LOG(">>> offset %lld not found\n", offset);
            break;
        }
        LOG("offset %lld found: block: 0x%08x, offset: %lld, ox: %5lld ==\n", offset, (int)block, block->offset, offset - block->offset);
    }
#endif

#if 0
    {
        int fd = -1;
        char buf[65535] = "";

        bc_init(20, 65534, 80);

        fd = bc_open("./Makefile", O_RDONLY, 0400);
        LOG("open ./Makefile fd %d\n", fd);
        if (fd >= 0) {
            int rlen = bc_read(fd, buf, sizeof(buf));
            if (rlen > 0) {
                LOG("read %d-byte\n", rlen);
                /*_prn_mem(buf, rlen);*/
            } else {
                if (rlen < 0) LOG("read error %d, %d, %s\n", rlen, errno, strerror(errno));
            }
            bc_close(fd);
            LOG("fd %d closed\n", fd);
        }

        bc_deinit();
    }
#endif

    {
        int fd = -1;
        char* file = "/proj/mtk40123/mnt/qt.451n.tar.gz";
        char buf[65535] = "";

        bc_init(20, 65534, 80);

        fd = bc_open(file, O_RDONLY, 0400);
        LOG("open %s fd %d\n", file, fd);
        if (fd >= 0) {
            off_t rtotal = 0;
            int rlen = 0;
            while ((rlen = bc_read(fd, buf, sizeof(buf))) > 0) {
                rtotal += rlen;
                /*LOG("==== read %lld-byte\n", rtotal);
                _prn_mem(buf, 32);*/
            }
            if (rlen < 0) {
                LOG("read error %d, %d, %s\n", rlen, errno, strerror(errno));
            }
            if (rtotal > 0) {
                LOG("read %lld-byte\n", rtotal);
                /*_prn_mem(buf, rlen);*/
            } else {
                LOG("read error %d, %d, %s\n", rlen, errno, strerror(errno));
            }
            bc_close(fd);
            LOG("fd %d closed\n", fd);
        }

        bc_deinit();
    }

    /*LOG("bc destroy rc %d\n", bc_destroy(bc));*/
    return 0;
}
#endif

