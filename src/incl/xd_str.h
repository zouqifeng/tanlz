#if !defined(__XD_STR_H__)
#define __XD_STR_H__

#include "common.h"

/* 字串 */
typedef struct
{
    char *str;      /* 字串值 */
    int len;        /* 字串长 */
} xd_str_t;
 
xd_str_t *str_to_lower(xd_str_t *s);
xd_str_t *str_to_upper(xd_str_t *s);
int str_trim(const char *in, char *out, size_t size);

/* URI字段 */
typedef struct
{
    char uri[URI_MAX_LEN];      /* URI */
    int len;                    /* URI长度 */

    char protocol[URI_PROTOCOL_MAX_LEN];    /* 协议类型 */
    char host[URI_MAX_LEN];     /* 域名 */
    char path[URI_MAX_LEN];     /* 路径信息 */
    int port;                   /* 端口号 */
} uri_field_t;

int uri_reslove(const char *uri, uri_field_t *f);

#endif /*__XD_STR_H__*/