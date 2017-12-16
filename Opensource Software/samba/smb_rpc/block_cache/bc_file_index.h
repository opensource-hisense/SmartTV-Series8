#ifndef _BLOCK_CACHE_FILE_INDEX_H_
#define _BLOCK_CACHE_FILE_INDEX_H_

extern int bc_file_index_list_init(void);
extern int bc_file_index_list_deinit(void);
extern int bc_file_index_list_count(void);

extern int bc_file_index_new_handle(void);
extern int bc_file_index_remove_handle(int handle);

extern int bc_file_index_list_add_offset(const char* path, int fd, off_t offset);
extern int bc_file_index_list_remove_offset(const char* path, int fd, off_t offset);
extern off_t bc_file_index_list_remove_offset_1by1(const char* path, int fd);

extern int bc_file_index_list_add_handle(const char* path, int fd, int handle, off_t size);
extern int bc_file_index_list_remove_handle(const char* path, int fd, int handle);
extern int bc_file_index_list_get_handle_count(const char* path, int fd);

extern int bc_file_index_list_search_fd_by_handle(int handle);
extern int bc_file_index_list_search_fd_by_path(const char* path);
extern char* bc_file_index_list_search_path_by_handle(int handle);
extern char* bc_file_index_list_search_path_by_fd(int fd);

extern off_t bc_file_index_list_get_pos_by_fd(int fd);
extern int bc_file_index_list_update_pos_by_fd(int fd, off_t pos);

extern off_t bc_file_index_list_get_handle_pos_by_handle(int handle);
extern off_t bc_file_index_list_update_handle_pos_by_handle(int handle, off_t pos);

extern off_t bc_file_index_list_update_fd_pos_by_handle(int handle, off_t pos);

extern int bc_file_index_list_search_handle_info_by_handle(int handle, char* path, int* fd, off_t* fd_pos, off_t* size, off_t* handle_pos);

#endif /* _BLOCK_CACHE_FILE_INDEX_H_ */

