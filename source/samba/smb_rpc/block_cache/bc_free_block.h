#ifndef _BLOCK_CACHE_FREE_BLOCK_H_
#define _BLOCK_CACHE_FREE_BLOCK_H_

extern int bc_free_block_list_init(void);
extern int bc_free_block_list_deinit(void);
extern int bc_free_block_list_count(void);
extern int bc_free_block_list_add_head(void* block);
extern int bc_free_block_list_add_tail(void* block);
extern void* bc_free_block_list_remove_head(void);
extern void* bc_free_block_list_remove_tail(void);
extern void* bc_free_block_list_remove_by_block(void* block);
extern int bc_free_block_list_print(void);

#endif /* _BLOCK_CACHE_FREE_BLOCK_H_ */

