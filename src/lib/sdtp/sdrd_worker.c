/******************************************************************************
 ** Copyright(C) 2014-2024 Qiware technology Co., Ltd
 **
 ** 文件名: sdtp.c
 ** 版本号: 1.0
 ** 描  述: 共享消息传输通道(Sharing Message Transaction Channel)
 **         1. 主要用于异步系统之间数据消息的传输
 ** 作  者: # Qifeng.zou # 2015.01.06 #
 ******************************************************************************/

#include "sdtp_comm.h"
#include "sdtp_mesg.h"
#include "sdrd_recv.h"
#include "thread_pool.h"

/* 静态函数 */
static sdtp_worker_t *sdrd_worker_get_curr(sdrd_cntx_t *ctx);
static int sdrd_worker_event_core_hdl(sdrd_cntx_t *ctx, sdtp_worker_t *worker);
static int sdrd_worker_cmd_proc_req_hdl(sdrd_cntx_t *ctx, sdtp_worker_t *worker, const sdtp_cmd_t *cmd);

/******************************************************************************
 **函数名称: sdrd_worker_routine
 **功    能: 运行工作线程
 **输入参数:
 **     _ctx: 全局对象
 **输出参数: NONE
 **返    回: VOID *
 **实现描述:
 **     1. 获取工作对象
 **     2. 等待事件通知
 **     3. 进行事件处理
 **注意事项:
 **作    者: # Qifeng.zou # 2015.01.06 #
 ******************************************************************************/
void *sdrd_worker_routine(void *_ctx)
{
    int ret, idx;
    sdtp_worker_t *worker;
    sdtp_cmd_proc_req_t *req;
    struct timeval timeout;
    sdrd_cntx_t *ctx = (sdrd_cntx_t *)_ctx;

    /* 1. 获取工作对象 */
    worker = sdrd_worker_get_curr(ctx);
    if (NULL == worker)
    {
        log_fatal(ctx->log, "Get current worker failed!");
        abort();
        return (void *)-1;
    }

    for (;;)
    {
        /* 2. 等待事件通知 */
        FD_ZERO(&worker->rdset);

        FD_SET(worker->cmd_sck_id, &worker->rdset);
        worker->max = worker->cmd_sck_id;

        timeout.tv_sec = 30;
        timeout.tv_usec = 0;
        ret = select(worker->max+1, &worker->rdset, NULL, NULL, &timeout);
        if (ret < 0)
        {
            if (EINTR == errno) { continue; }
            log_fatal(worker->log, "errmsg:[%d] %s", errno, strerror(errno));
            abort();
            return (void *)-1;
        }
        else if (0 == ret)
        {
            /* 超时: 模拟处理命令 */
            sdtp_cmd_t cmd;
            req = (sdtp_cmd_proc_req_t *)&cmd.args;

            for (idx=0; idx<SDTP_WORKER_HDL_QNUM; ++idx)
            {
                memset(&cmd, 0, sizeof(cmd));

                cmd.type = SDTP_CMD_PROC_REQ;
                req->num = -1;
                req->rqidx = SDTP_WORKER_HDL_QNUM * worker->id + idx;

                sdrd_worker_cmd_proc_req_hdl(ctx, worker, &cmd);
            }
            continue;
        }

        /* 3. 进行事件处理 */
        sdrd_worker_event_core_hdl(ctx, worker);
    }

    abort();
    return (void *)-1;
}

/******************************************************************************
 **函数名称: sdrd_worker_get_curr
 **功    能: 获取工作对象
 **输入参数:
 **     ctx: 全局对象
 **输出参数: NONE
 **返    回: 工作对象
 **实现描述:
 **     1. 获取线程编号
 **     2. 返回工作对象
 **注意事项:
 **作    者: # Qifeng.zou # 2015.01.06 #
 ******************************************************************************/
static sdtp_worker_t *sdrd_worker_get_curr(sdrd_cntx_t *ctx)
{
    int id;

    /* 1. 获取线程编号 */
    id = thread_pool_get_tidx(ctx->worktp);
    if (id < 0)
    {
        log_fatal(ctx->log, "Get thread index failed!");
        return NULL;
    }

    /* 2. 返回工作对象 */
    return (sdtp_worker_t *)(ctx->worktp->data + id * sizeof(sdtp_worker_t));
}

/******************************************************************************
 **函数名称: sdrd_worker_init
 **功    能: 初始化工作服务
 **输入参数:
 **     ctx: 全局对象
 **     worker: 工作对象
 **     id: 工作对象编号
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述:
 **     1. 创建命令套接字
 **注意事项:
 **作    者: # Qifeng.zou # 2015.01.06 #
 ******************************************************************************/
int sdrd_worker_init(sdrd_cntx_t *ctx, sdtp_worker_t *worker, int id)
{
    char path[FILE_PATH_MAX_LEN];
    sdrd_conf_t *conf = &ctx->conf;

    worker->id = id;
    worker->log = ctx->log;

    /* 1. 创建命令套接字 */
    sdrd_worker_usck_path(conf, path, worker->id);

    worker->cmd_sck_id = unix_udp_creat(path);
    if (worker->cmd_sck_id < 0)
    {
        log_error(worker->log, "Create unix-udp socket failed!");
        return SDTP_ERR;
    }

    return SDTP_OK;
}

/******************************************************************************
 **函数名称: sdrd_worker_event_core_hdl
 **功    能: 核心事件处理
 **输入参数:
 **     ctx: 全局对象
 **     worker: 工作对象
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述:
 **     1. 创建命令套接字
 **注意事项:
 **作    者: # Qifeng.zou # 2015.01.06 #
 ******************************************************************************/
static int sdrd_worker_event_core_hdl(sdrd_cntx_t *ctx, sdtp_worker_t *worker)
{
    sdtp_cmd_t cmd;

    if (!FD_ISSET(worker->cmd_sck_id, &worker->rdset))
    {
        return SDTP_OK; /* 无数据 */
    }

    if (unix_udp_recv(worker->cmd_sck_id, (void *)&cmd, sizeof(cmd)) < 0)
    {
        log_error(worker->log, "errmsg:[%d] %s", errno, strerror(errno));
        return SDTP_ERR_RECV_CMD;
    }

    switch (cmd.type)
    {
        case SDTP_CMD_PROC_REQ:
        {
            return sdrd_worker_cmd_proc_req_hdl(ctx, worker, &cmd);
        }
        default:
        {
            log_error(worker->log, "Received unknown type! %d", cmd.type);
            return SDTP_ERR_UNKNOWN_CMD;
        }
    }

    return SDTP_ERR_UNKNOWN_CMD;
}

/******************************************************************************
 **函数名称: sdrd_worker_cmd_proc_req_hdl
 **功    能: 处理请求的处理
 **输入参数:
 **     ctx: 全局对象
 **     worker: 工作对象
 **     cmd: 命令信息
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述:
 **注意事项:
 **作    者: # Qifeng.zou # 2015.01.06 #
 ******************************************************************************/
static int sdrd_worker_cmd_proc_req_hdl(sdrd_cntx_t *ctx, sdtp_worker_t *worker, const sdtp_cmd_t *cmd)
{
    int idx;
    void *addr, *ptr;
    queue_t *rq;
    sdtp_group_t *group;
    sdtp_header_t *head;
    sdtp_reg_t *reg;
    const sdtp_cmd_proc_req_t *work_cmd = (const sdtp_cmd_proc_req_t *)&cmd->args;

    /* 1. 获取接收队列 */
    rq = ctx->recvq[work_cmd->rqidx];

    while (1)
    {
        /* 2. 从接收队列获取数据 */
        addr = queue_pop(rq);
        if (NULL == addr)
        {
            return SDTP_OK;
        }

        group = (sdtp_group_t *)addr;
        ptr = addr + sizeof(sdtp_group_t);

        for (idx=0; idx<group->num; ++idx)
        {
            /* 3. 执行回调函数 */
            head = (sdtp_header_t *)ptr;

            reg = &ctx->reg[head->type];
            if (NULL == reg->proc)
            {
                ptr += head->length + sizeof(sdtp_header_t);
                ++worker->drop_total;   /* 丢弃计数 */
                continue;
            }

            if (reg->proc(head->type, head->nid,
                        ptr+sizeof(sdtp_header_t), head->length, reg->args))
            {
                ++worker->err_total;    /* 错误计数 */
            }
            else
            {
                ++worker->proc_total;   /* 处理计数 */
            }

            ptr += head->length + sizeof(sdtp_header_t);
        }

        /* 4. 释放内存空间 */
        queue_dealloc(rq, addr);
    }

    return SDTP_OK;
}
