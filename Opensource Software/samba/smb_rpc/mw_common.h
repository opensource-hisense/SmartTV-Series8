#ifndef _MW_COMMON_H_
#define _MW_COMMON_H_

/*#include <stdio.h>
#include <stdlib.h>*/
#include "mylist.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MW_FIELD_SYS_MAGIC_ID     0x4d594d57
#define MW_FIELD_SYS_SERVICE_NAME 0x00010001
#define MW_FIELD_SYS_RETURN_CODE  0x00010002
#define MW_FIELD_SYS_DELAY_FREE   0x00010003

/* 0x00010000 - 0x0001FFFF  system
 * 0x00100000 - 0x001FFFFF  long
 * 0x00200000 - 0x002FFFFF  string
 * 0x00300000 - 0x003FFFFF  data
 *
 * 0x00010001 - service name
 * 0x00010002 - return code
 */
#define MW_FIELD_LONG_BGN         0x00100000
#define MW_FIELD_STR_BGN          0x00200000
#define MW_FIELD_DATA_BGN         0x00300000

#define MW_FIRST_READ_SIZE 16

extern int mws_open (const char* path);
extern int mws_close (const char* path);
extern int mws_fd (const char* path);
extern void* mws_return (int rc, char* buf);
extern int mws_reg_service (int fd, const char* name, void* (*_fun) (void*), void* tag);
extern int mws_unreg_service (int fd, const char* name);
extern int mws_unreg_services_all (int fd);

extern void* mw_alloc (int size);
extern void* mw_alloc_delay_free (int size);
extern void* mw_memset (void* ptr);
extern void mw_free (void* ptr);

extern int mw_get (const void* data, int idx, int field_id, char* presult, int *size);
extern int mw_append (void* data, int field_id, const char* presult, int len);

extern int mwc_open (const char* path);
extern int mwc_close (int fd);

extern int mwc_call (int fd, const char* name, char* snd_buf, int snd_size, char** pp_rcv_buf, int* p_rcv_size);

extern int mwc_lock_init (void);
extern int mwc_lock (void);
extern int mwc_unlock (void);

extern int mws_lock_init (void);
extern int mws_lock (void);
extern int mws_unlock (void);

extern int mw_sem_open (const char* path);
extern int mw_sem_close (int sem);
extern int mw_sem_lock (int sem);
extern int mw_sem_unlock (int sem);

extern void _prn_mem (const char* data, int len);
extern void _prn_bt(void);

#ifdef __cplusplus
}
#endif

#endif /* _MW_COMMON_H_ */

