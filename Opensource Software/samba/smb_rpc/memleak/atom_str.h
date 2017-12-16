#ifndef TEST_ATOM_STR_INCLUDE
#define TEST_ATOM_STR_INCLUDE

extern int atom_init (int r);
extern const char *atom_new (const char *str, int len);
extern const char *atom_str (const char *str);
extern void atom_free (const char *str);

extern void _adm (int len);
extern void _aam (void* p, const char*, int);
extern void _xxm (void* p, const char*, int);
#ifdef DEBUG
#define _am(x) _aam ((x), __FUNCTION__, __LINE__)
#define _xm(x) _xxm ((x), __FUNCTION__, __LINE__)
#else
#define _am(x) _aam ((x), NULL, 0)
#define _xm(x) _xxm ((x), NULL, 0)
#endif
extern int _cm (void);
#ifdef NO_AXM
#ifdef _am
#undef _am
#endif
#ifdef _xm
#undef _xm
#endif
#define _am(...)
#define _xm(...)
#endif

#endif /* TEST_ATOM_STR_INCLUDE */

