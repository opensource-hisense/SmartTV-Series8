#ifndef _SMB_COMMON_H_
#define _SMB_COMMON_H_

#include "mylist.h"
#include "mw_common.h"

#define SMBC_CLIENT "/tmp/mnp_smbc_rpc"
#define SMBC_CLIENT_CMD "/tmp/mnp_smbc_rpc_cmd"
#define SMBC_CLIENT_PW "/tmp/mnp_smbc_rpc_pw"
#define SMBC_CLIENT_SM "/tmp/mnp_smbc_sm"

#define MW_FIELD_SMB_PATH         MW_FIELD_STR_BGN  + 1000
#define MW_FIELD_SMB_DIR_NAME     MW_FIELD_STR_BGN  + 1001

#define MW_FIELD_SMB_SRV          MW_FIELD_STR_BGN  + 1002
#define MW_FIELD_SMB_SHR          MW_FIELD_STR_BGN  + 1003
#define MW_FIELD_SMB_WG           MW_FIELD_STR_BGN  + 1004
#define MW_FIELD_SMB_UN           MW_FIELD_STR_BGN  + 1005
#define MW_FIELD_SMB_PW           MW_FIELD_STR_BGN  + 1006

#define MW_FIELD_SMB_SHM_NAME     MW_FIELD_STR_BGN  + 1007

#define MW_FIELD_SMB_ERRNO        MW_FIELD_LONG_BGN + 1000
#define MW_FIELD_SMB_FD           MW_FIELD_LONG_BGN + 1001
#define MW_FIELD_SMB_CNT          MW_FIELD_LONG_BGN + 1002
/*#define MW_FIELD_SMB_OFFSET       MW_FIELD_LONG_BGN + 1003*/ /* FIXME: cannot support big file */
#define MW_FIELD_SMB_WHENCE       MW_FIELD_LONG_BGN + 1004

#define MW_FIELD_SMB_PORT         MW_FIELD_LONG_BGN + 1005

#define MW_FIELD_SMB_SESSION      MW_FIELD_LONG_BGN + 1006

#define MW_FIELD_SMB_DIRENT       MW_FIELD_DATA_BGN + 1000
/*#define MW_FIELD_SMB_DATA         MW_FIELD_DATA_BGN + 1001*/
#define MW_FIELD_SMB_STAT         MW_FIELD_DATA_BGN + 1002
#define MW_FIELD_SMB_LSEEK_RC     MW_FIELD_DATA_BGN + 1003
#define MW_FIELD_SMB_OFFSET       MW_FIELD_DATA_BGN + 1004

extern int _get_ms (void);
#ifndef NET_SMB_MODULE_NAME
#define NET_SMB_MODULE_NAME "smb_rpc"
#endif /* NET_SMB_MODULE_NAME */
#ifdef DEBUG
#define PRINTF(...) do { unsigned int ct = _get_ms(); fprintf (stderr, "[%s] %u.%03u, %s(), %s.%d >>> ", NET_SMB_MODULE_NAME, ct/1000, ct%1000, __FUNCTION__, __FILE__, __LINE__); fprintf (stderr, __VA_ARGS__); } while (0)
#else
/*#define PRINTF(...)*/
#define PRINTF(...) do { unsigned int ct = _get_ms(); fprintf (stderr, "[%s] %u.%03u, %s(), %s.%d >>> ", NET_SMB_MODULE_NAME, ct/1000, ct%1000, __FUNCTION__, __FILE__, __LINE__); fprintf (stderr, __VA_ARGS__); } while (0)
#endif

struct _smbc_dirent
{
    unsigned int smbc_type; 
    unsigned int dirlen;
    unsigned int commentlen;
    char *comment;
    unsigned int namelen;
    char name[1];
};

typedef void (*smbc_get_auth_data_fn_ctx) (void*, const char* srv, const char* shr, char* wg, int wglen, char* un, int unlen, char* pw, int pwlen);
typedef void (*smbc_get_auth_data_fn) (const char* srv, const char* shr, char* wg, int wglen, char* un, int unlen, char* pw, int pwlen);

typedef void (*smbc_fun_org) (void);

/* for samba-3.0.x */
struct _smbc_context
{
    int debug;
    char* netbios_name;
    char* workgroup;
    char* user;
    int timeout;
    smbc_fun_org f1;
    smbc_fun_org f2;
    smbc_fun_org f3;
    smbc_fun_org f4;
    smbc_fun_org f5;
    smbc_fun_org f6;
    smbc_fun_org f7;
    smbc_fun_org f9;
    smbc_fun_org f10;
    smbc_fun_org f11;
    smbc_fun_org f12;
    smbc_fun_org f13;
    smbc_fun_org f14;
    smbc_fun_org f15;
    smbc_fun_org f16;
    smbc_fun_org f17;
    smbc_fun_org f18;
    smbc_fun_org f19;
    smbc_fun_org f20;
    smbc_fun_org f21;
    smbc_fun_org f22;
    smbc_fun_org f23;
    smbc_fun_org f24;
    smbc_fun_org f25;
    smbc_fun_org f26;
    smbc_fun_org f27;
    smbc_fun_org f28;
    smbc_fun_org f29;
    struct _smbc_cb {
        smbc_get_auth_data_fn auth_fn;
    } smbc_cb;
};

#define MW_BUF_SIZE_SMALL 256
#define MW_BUF_SIZE 8192
/*#define MW_BUF_SIZE_BIG 81920*/
/*#define MW_BUF_SIZE_BIG 73728*/
#define MW_BUF_SIZE_BIG 131072
#define SMB_PATH_SIZE   8192

#define NET_SMB_SMBC_TIMEO 1000

#endif /* _SMB_COMMON_H_ */

