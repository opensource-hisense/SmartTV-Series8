#ifndef _NET_MEMLEAK_H_
#define _NET_MEMLEAK_H_

#ifdef _NET_MEMLEAK_DEBUG

#ifdef __cplusplus
extern "C" {
#endif

extern int mw_memleak_init(void);

extern void* _net_mem_alloc (const char* ps_file,
                             unsigned int      ui4_line,
                             size_t      z_size);

extern void* _net_mem_calloc (const char* ps_file,
                              unsigned int      ui4_line,
                              unsigned int      ui4_num_unit,
                              size_t      z_unit_size);

extern void* _net_mem_realloc (const char* ps_file,
                               unsigned int      ui4_line,
                               void*       pv_mem,
                               size_t      z_new_size);

extern void  _net_mem_free (const char* ps_file,
                            unsigned int      ui4_line,
                            void* pv_mem);

extern char* _net_strdup (const char* ps_file,
                          unsigned int      ui4_line,
                          const char* ps_str);

extern void *_net_memset (const char* ps_file,
                          unsigned int      ui4_line,
                          void*       s,
                          int       c,
                          size_t      n);

#ifdef x_mem_alloc
#undef x_mem_alloc
#endif

#ifdef x_mem_calloc
#undef x_mem_calloc
#endif

#ifdef x_mem_free
#undef x_mem_free
#endif

#ifdef x_mem_realloc
#undef x_mem_realloc
#endif

#ifdef x_strdup
#undef x_strdup
#endif

#ifdef x_memset
#undef x_memset
#endif

#ifdef x_strcmp
#undef x_strcmp
#endif

#ifdef malloc
#undef malloc
#endif

#ifdef calloc
#undef calloc
#endif

#ifdef free
#undef free
#endif

#ifdef realloc
#undef realloc
#endif

#ifdef strdup
#undef strdup
#endif

#ifdef memset
#undef memset
#endif

#ifdef strcmp
#undef strcmp
#endif

#define x_mem_alloc(x) _net_mem_alloc (__FILE__, __LINE__, (x))
#define x_mem_calloc(x,y) _net_mem_calloc (__FILE__, __LINE__, (x), (y))
#define x_mem_free(x) _net_mem_free(__FILE__, __LINE__, (x))
#define x_mem_realloc(x,y) _net_mem_realloc (__FILE__, __LINE__, (x), (y))
#define x_strdup(x) _net_strdup (__FILE__, __LINE__, (x))
#define x_memset(x,y,z) _net_memset (__FILE__, __LINE__, (x), (y), (z))

#define malloc(x) _net_mem_alloc (__FILE__, __LINE__, (x))
#define calloc(x,y) _net_mem_calloc (__FILE__, __LINE__, (x), (y))
#define free(x) _net_mem_free(__FILE__, __LINE__, (x))
#define realloc(x,y) _net_mem_realloc (__FILE__, __LINE__, (x), (y))
#define strdup(x) _net_strdup (__FILE__, __LINE__, (x))
#define memset(x,y,z) _net_memset (__FILE__, __LINE__, (x), (y), (z))

extern int _net_strcmp_np (const char* ps_file,
                          unsigned int      ui4_line,
                          const char* s1,
                          const char* s2);
#define x_strcmp(x,y) _net_strcmp_np (__FILE__, __LINE__, (x), (y))
#define strcmp(x,y) _net_strcmp_np (__FILE__, __LINE__, (x), (y))

#if 0
extern int _net_socket_np (const char *ps_file,
                             unsigned int      ui4_line,
                             int       domain,
                             int       type,
                             int       protocol);
#define socket(x,y,z) _net_socket_np (__FILE__, __LINE__, (x), (y), (z))

extern int _net_close_np (const char *ps_file, unsigned int ui4_line, int fd);
#define close(x) _net_close_np (__FILE__, __LINE__, (x))

#endif /* 1 */

#ifdef __cplusplus
}
#endif

/* for c++ new/delete */
#ifdef __cplusplus

#include <cstdio>
#include <new>

inline void memleak_start_here(void)
{
#ifdef NET_MEMLEAK_CK_ALL_MEMORY_HOOK
    char* ps = new char();
    delete ps;
    ps = new char[4];
    delete[] ps;
    ps = (char*)malloc(4);
    free(ps);
#endif
    printf(">>> memleak check start ... <<<\n");
    mw_memleak_init();
}

//extern "C" int _show_backtrace(void);

#ifdef NET_MEMLEAK_CK_ALL_MEMORY_HOOK
inline void* operator new(size_t size) throw()
{
    size_t real_size = size;
#ifdef X86
    size = (size_t)(&size - sizeof(size));
#else
    asm("str r14, [sp, #4]");
#endif
    char file[32] = "";
    sprintf(file, "new:%p", (void*)size);
    return _net_mem_alloc(file, 0, real_size);
}

inline void* operator new[](size_t size) throw()
{
    size_t real_size = size;
#ifdef X86
    size = (size_t)(&size - sizeof(size));
#else
    asm("str r14, [sp, #4]");
#endif
    char file[32] = "";
    sprintf(file, "new:%p", (void*)size);
    return _net_mem_alloc(file, 0, real_size);
}
#endif

inline void* operator new(size_t size, const char* file, int line) throw()
{
    return _net_mem_alloc(file, line, size);
}

inline void* operator new[](size_t size, const char* file, int line) throw()
{
    return _net_mem_alloc(file, line, size);
}

inline void operator delete(void* p) throw()
{
    _net_mem_free("**cxx-delete**", 0, p);
}

inline void operator delete[](void* p) throw()
{
    _net_mem_free("**cxx-delete**", 0, p);
}

#ifndef NO_CXX_MEMLEAK
#define new new(__FILE__,__LINE__)
#endif

#endif /* for c++ */

#endif /* _NET_MEMLEAK_DEBUG */

#endif /* _NET_MEMLEAK_H_ */

