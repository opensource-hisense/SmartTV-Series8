#ifndef _BLOCK_CACHE_BLOCK_CACHE_H_
#define _BLOCK_CACHE_BLOCK_CACHE_H_

/*
 * block_status:
 * 0: invalid, 1: locked, 2: valid, 3: freed, 4: destroyed
 */
struct bc_block {
    volatile int block_status;
    char* path;
    int fd;
    off_t offset;
    int block_size_only;
    int data_len;
    char data[1];
};

#endif /* _BLOCK_CACHE_BLOCK_CACHE_H_ */

