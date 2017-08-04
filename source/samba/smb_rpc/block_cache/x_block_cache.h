#ifndef _BLOCK_CACHE_X_BLOCK_CACHE_H_
#define _BLOCK_CACHE_X_BLOCK_CACHE_H_

typedef int (*_io_open)(const char* path, int flags, mode_t mode);
typedef int (*_io_close)(int fd);
typedef ssize_t (*_io_read)(int fd, void* buf, size_t count);
typedef off_t (*_io_lseek)(int fd, off_t offset, int whence);
typedef int (*_io_stat)(const char*, struct stat*);

extern int bc_init(int blocks_per_task, int block_size, int threshold_percent);
extern int bc_set_io_fun(_io_open nio_open, _io_read nio_read, _io_lseek nio_lseek, _io_close nio_close, _io_stat nio_stat);

extern int bc_open(const char* path, int flags, mode_t mode);
extern int bc_read(int handle, void* buf, size_t count);
extern int bc_lseek(int handle, off_t offset, int whence);
extern int bc_close(int handle);

#endif /* _BLOCK_CACHE_X_BLOCK_CACHE_H_ */

