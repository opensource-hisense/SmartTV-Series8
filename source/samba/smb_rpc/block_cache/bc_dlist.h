#ifndef _BLOCK_CACHE_DLIST_H_
#define _BLOCK_CACHE_DLIST_H_

extern int _get_ms (void);
extern void _prn_mem (const char* data, int len);
#ifndef LOG
#define LOG(...) do { unsigned int ct = _get_ms(); fprintf (stderr, "[bc] %u.%03u, %s, %s.%d >>> ", ct/1000, ct%1000, __FUNCTION__, __FILE__, __LINE__); fprintf (stderr, __VA_ARGS__); fflush (stderr); } while (0)
#endif

typedef int (*bc_comp_fun)(void* data, void* org);
typedef void (*bc_apply)(void* data);

extern void* bc_dlist_create(void);
extern int bc_dlist_count(void* list);
extern int bc_dlist_add_head(void* list, void* data);
extern int bc_dlist_add_tail(void* list, void* data);
extern void* bc_dlist_remove_head(void* list);
extern void* bc_dlist_remove_tail(void* list);
extern void* bc_dlist_remove_by_data(void* list, void* data);
extern void* bc_dlist_search(void* list, void* org, bc_comp_fun fun);
extern void* bc_dlist_search_and_remove(void* list, void* org, bc_comp_fun fun);
extern int bc_dlist_destroy(void* dlist);
extern int bc_dlist_print(void* list, bc_apply apply); /* for test only */

extern int bc_lock_init(pthread_mutex_t* p_mutex);
extern int bc_lock(pthread_mutex_t* p_mutex);
extern int bc_unlock(pthread_mutex_t* p_mutex);
extern int bc_lock_deinit(pthread_mutex_t* p_mutex);

#endif /* _BLOCK_CACHE_DLIST_H_ */

