#if !defined(__RTRD_RECV_H__)
#define __RTRD_RECV_H__

#include "log.h"
#include "sck.h"
#include "slab.h"
#include "list.h"
#include "comm.h"
#include "list2.h"
#include "queue.h"
#include "shm_opt.h"
#include "avl_tree.h"
#include "rtdt_cmd.h"
#include "rtdt_comm.h"
#include "shm_queue.h"
#include "thread_pool.h"

/* 宏定义 */
#define RTDT_CTX_POOL_SIZE      (5 * MB)/* 全局内存池空间 */

/* Recv线程的UNIX-UDP路径 */
#define rtrd_rsvr_usck_path(conf, path, tidx) \
    snprintf(path, sizeof(path), "../temp/rtdt/recv/%s/usck/%s_rsvr_%d.usck", conf->name, conf->name, tidx+1)
/* Worker线程的UNIX-UDP路径 */
#define rtrd_worker_usck_path(conf, path, tidx) \
    snprintf(path, sizeof(path), "../temp/rtdt/recv/%s/usck/%s_wsvr_%d.usck", conf->name, conf->name, tidx+1)
/* Listen线程的UNIX-UDP路径 */
#define rtrd_lsn_usck_path(conf, path) \
    snprintf(path, sizeof(path), "../temp/rtdt/recv/%s/usck/%s_listen.usck", conf->name, conf->name)
/* 发送队列的共享内存KEY路径 */
#define rtrd_shm_sendq_path(conf, path) \
    snprintf(path, sizeof(path), "../temp/rtdt/recv/%s/%s_shm_sendq", conf->name, conf->name)

/* 配置信息 */
typedef struct
{
    char name[FILE_NAME_MAX_LEN];       /* 服务名: 不允许重复出现 */

    rtdt_auth_conf_t auth;              /* 鉴权配置 */

    int port;                           /* 侦听端口 */
    int recv_thd_num;                   /* 接收线程数 */
    int work_thd_num;                   /* 工作线程数 */
    int rqnum;                          /* 接收队列数 */

    queue_conf_t recvq;                 /* 接收队列配置 */
    queue_conf_t sendq;                 /* 发送队列配置 */
} rtrd_conf_t;

/* 侦听对象 */
typedef struct
{
    pthread_t tid;                      /* 侦听线程ID */
    log_cycle_t *log;                   /* 日志对象 */
    int cmd_sck_id;                     /* 命令套接字 */
    int lsn_sck_id;                     /* 侦听套接字 */

    uint64_t serial;                     /* 连接请求序列 */
} rtrd_lsn_t;

/* 套接字信息 */
typedef struct _rtrd_sck_t
{
    int fd;                             /* 套接字ID */
    int devid;                          /* 设备ID */
    uint64_t serial;                    /* 套接字序列号 */

    time_t ctm;                         /* 创建时间 */
    time_t rdtm;                        /* 最近读取时间 */
    time_t wrtm;                        /* 最近写入时间 */
    char ipaddr[IP_ADDR_MAX_LEN];       /* IP地址 */

    int auth_succ;                      /* 鉴权成功(1:成功 0:失败)  */

    rtdt_snap_t recv;                   /* 接收快照 */
    rtdt_snap_t send;                   /* 发送快照 */

    list_t *mesg_list;                  /* 发送消息链表 */

    uint64_t recv_total;                /* 接收的数据条数 */
} rtrd_sck_t;

/* DEV->SVR的映射 */
typedef struct
{
    int rsvr_idx;                       /* 接收服务的索引(链表主键) */
    int count;                          /* 引用计数 */
} rtrd_dev_to_svr_item_t;

/* 接收对象 */
typedef struct
{
    int tidx;                           /* 线程索引 */
    slab_pool_t *pool;                  /* 内存池 */
    log_cycle_t *log;                   /* 日志对象 */

    int cmd_sck_id;                     /* 命令套接字 */

    int max;                            /* 最大套接字 */
    time_t ctm;                         /* 当前时间 */
    fd_set rdset;                       /* 可读集合 */
    fd_set wrset;                       /* 可写集合 */
    list2_t conn_list;                  /* 套接字链表 */

    /* 统计信息 */
    uint32_t connections;               /* TCP连接数 */
    uint64_t recv_total;                /* 获取的数据总条数 */
    uint64_t err_total;                 /* 错误的数据条数 */
    uint64_t drop_total;                /* 丢弃的数据条数 */
} rtrd_rsvr_t;

/* 服务端外部对象 */
typedef struct
{
    shm_queue_t *sendq;                 /* 发送队列 */
} rtrd_cli_t;

/* 全局对象 */
typedef struct
{
    rtrd_conf_t conf;                   /* 配置信息 */
    log_cycle_t *log;                   /* 日志对象 */
    slab_pool_t *pool;                  /* 内存池对象 */

    rtdt_reg_t reg[RTDT_TYPE_MAX];      /* 回调注册对象 */

    rtrd_lsn_t listen;                  /* 侦听对象 */
    thread_pool_t *recvtp;              /* 接收线程池 */
    thread_pool_t *worktp;              /* 工作线程池 */

    queue_t **recvq;                    /* 接收队列(内部队列) */
    queue_t **sendq;                    /* 发送队列(内部队列) */
    shm_queue_t *shm_sendq;             /* 发送队列(外部队列)
                                           注: 外部接口首先将要发送的数据放入
                                           此队列, 再从此队列分发到不同的线程队列 */

    pthread_rwlock_t dev_to_svr_map_lock;  /* 读写锁: DEV->SVR映射表 */
    avl_tree_t *dev_to_svr_map;         /* DEV->SVR的映射表(以devid为主键) */
} rtrd_cntx_t;

/* 外部接口 */
rtrd_cntx_t *rtrd_init(const rtrd_conf_t *conf, log_cycle_t *log);
int rtrd_register(rtrd_cntx_t *ctx, int type, rtdt_reg_cb_t proc, void *args);
int rtrd_startup(rtrd_cntx_t *ctx);

rtrd_cli_t *rtrd_cli_init(const rtrd_conf_t *conf);
int rtrd_cli_send(rtrd_cli_t *cli, int type, int dest, void *data, int len);

/* 内部接口 */
void *rtrd_lsn_routine(void *_ctx);
int rtrd_lsn_destroy(rtrd_lsn_t *lsn);

void *rtrd_dist_routine(void *_ctx);

void *rtrd_rsvr_routine(void *_ctx);
int rtrd_rsvr_init(rtrd_cntx_t *ctx, rtrd_rsvr_t *rsvr, int tidx);

void *rtrd_worker_routine(void *_ctx);
int rtrd_worker_init(rtrd_cntx_t *ctx, rtdt_worker_t *worker, int tidx);

void rtrd_rsvr_del_all_conn_hdl(rtrd_cntx_t *ctx, rtrd_rsvr_t *rsvr);

int rtrd_cmd_to_rsvr(rtrd_cntx_t *ctx, int cmd_sck_id, const rtdt_cmd_t *cmd, int idx);
int rtrd_link_auth_check(rtrd_cntx_t *ctx, rtdt_link_auth_req_t *link_auth_req);

shm_queue_t *rtrd_shm_sendq_creat(const rtrd_conf_t *conf );
shm_queue_t *rtrd_shm_sendq_attach(const rtrd_conf_t *conf);

int rtrd_dev_to_svr_map_init(rtrd_cntx_t *ctx);
int rtrd_dev_to_svr_map_add(rtrd_cntx_t *ctx, int devid, int rsvr_idx);
int rtrd_dev_to_svr_map_rand(rtrd_cntx_t *ctx, int devid);
int rtrd_dev_to_svr_map_del(rtrd_cntx_t *ctx, int devid, int rsvr_idx);

#endif /*__RTRD_RECV_H__*/