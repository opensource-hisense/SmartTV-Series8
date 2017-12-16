#ifndef _MY_LIST_H_
#define _MY_LIST_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _NET_MEMLEAK_DEBUG
#include "memleak/net_memleak.h"
#endif

typedef void* handle_t;
#define null_handle (handle_t)NULL

extern int my_list_create (handle_t *ph_handle);
extern int my_list_destroy (handle_t h_handle, void (*pf_free) (void* pv_data));
extern void* my_list_delete_head (handle_t h_handle);
extern void* my_list_delete (handle_t h_handle, const void *pv_data);
extern void* my_list_delete2 (handle_t h_handle, const void *pv_data, int (*pf_cmp) (const void *pv1, const void* pv2));
extern int my_list_add (handle_t h_handle, void *pv_data);
extern int my_list_add_head (handle_t h_handle, void *pv_data);
extern int my_list_add_tail (handle_t h_handle, void *pv_data);
extern void *my_list_search (handle_t h_handle, const void *pv_data);
extern void *my_list_search2 (handle_t h_handle, const void *pv_data, int (*pf_cmp) (const void *pv1, const void* pv2));
extern int my_list_map (handle_t h_handle, int apply(void*, void*), void *);
extern int my_list_size (handle_t h_handle);
extern void** my_list_toarray (handle_t h_handle, void *end);
extern int my_list_toarray2 (handle_t h_handle, void **ppv_array, int len);
extern int my_list_count(handle_t h_handle);

#ifdef __cplusplus
}
#endif

#endif /* _MY_LIST_H_ */

