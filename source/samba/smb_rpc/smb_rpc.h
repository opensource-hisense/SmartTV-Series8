#ifndef _SMB_CLIENT_H_
#define _SMB_CLIENT_H_

#include "smb_common.h"

extern int _smbc_init_np (smbc_get_auth_data_fn _perm);
extern int _smbc_deinit_np (void);
extern int _smbc_reset_ctx_np (smbc_get_auth_data_fn);

extern int _smbc_opendir (const char*);
extern int _smbc_closedir (int fd);
extern void* _smbc_readdir (int fd);
extern off_t _smbc_lseekdir (int, off_t);

extern int _smbc_open (const char*, int, mode_t);
extern int _smbc_close (int);
extern ssize_t _smbc_read (int, void*, size_t);
extern off_t _smbc_lseek (int, off_t, int);
extern int _smbc_stat (const char*, struct stat*);

extern void _status (void);
#ifdef _NET_MEMLEAK_DEBUG
extern void _meminfo (void);
#endif

#endif /* _SMB_CLIENT_H_ */

