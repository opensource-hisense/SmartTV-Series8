#ifndef _BLOCK_CACHE_HASH_H_
#define _BLOCK_CACHE_HASH_H_

typedef int (*hash_fun)(void* data);

extern void* bc_hash_create(unsigned int hsize, void* tag, hash_fun fun);
extern int bc_hash_destroy (void* hash);
extern void* bc_hash_get_tag(void* hash);
extern int bc_hash_add(void* hash, void *data);
extern void* bc_hash_remove_by_data(void* hash, void *data);
extern void* bc_hash_remove(void* hash, void *data, bc_comp_fun comp_fun);
extern void* bc_hash_search_by_data(void* hash, void* data);
extern void* bc_hash_search(void* hash, void *data, bc_comp_fun comp_fun);

extern int bc_hash_dump (void* hash);

#endif /* _BLOCK_CACHE_HASH_H_ */

