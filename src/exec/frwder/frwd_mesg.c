/******************************************************************************
 ** Coypright(C) 2014-2024 Qiware technology Co., Ltd
 **
 ** 文件名: frwd_mesg.c
 ** 版本号: 1.0
 ** 描  述: 消息处理函数定义
 ** 作  者: # Qifeng.zou # Tue 14 Jul 2015 02:52:16 PM CST #
 ******************************************************************************/
#include "frwd.h"
#include "mesg.h"
#include "agent.h"
#include "command.h"
#include "agent_mesg.h"

/* 静态函数 */
static int frwd_reg_req_cb(frwd_cntx_t *frwd);
static int frwd_reg_rsp_cb(frwd_cntx_t *frwd);

static int frwd_search_word_req_hdl(int type, int orig, char *data, size_t len, void *args);
static int frwd_search_word_rsp_hdl(int type, int orig, char *data, size_t len, void *args);

static int frwd_insert_word_req_hdl(int type, int orig, char *data, size_t len, void *args);
static int frwd_insert_word_rsp_hdl(int type, int orig, char *data, size_t len, void *args);

/******************************************************************************
 **函数名称: frwd_set_reg
 **功    能: 注册处理回调
 **输入参数:
 **     frwd: 全局对象
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述:
 **注意事项:
 **作    者: # Qifeng.zou # 2015.06.10 #
 ******************************************************************************/
int frwd_set_reg(frwd_cntx_t *frwd)
{
    frwd_reg_req_cb(frwd);
    frwd_reg_rsp_cb(frwd);

    return FRWD_OK;
}

/******************************************************************************
 **函数名称: frwd_reg_req_cb
 **功    能: 注册请求回调
 **输入参数:
 **     frwd: 全局对象
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述:
 **注意事项:
 **作    者: # Qifeng.zou # 2016.02.24 14:22:25 #
 ******************************************************************************/
static int frwd_reg_req_cb(frwd_cntx_t *frwd)
{
#define FRWD_REG_REQ_CB(frwd, type, proc, args) \
    if (rtrd_register((frwd)->download, type, (rtmq_reg_cb_t)proc, (void *)args)) { \
        log_error((frwd)->log, "Register type [%d] failed!", type); \
        return FRWD_ERR; \
    }

    FRWD_REG_REQ_CB(frwd, MSG_SEARCH_WORD_REQ, frwd_search_word_req_hdl, frwd);
    FRWD_REG_REQ_CB(frwd, MSG_INSERT_WORD_REQ, frwd_insert_word_req_hdl, frwd);

    return FRWD_OK;
}

/******************************************************************************
 **函数名称: frwd_reg_rsp_cb
 **功    能: 注册应答回调
 **输入参数:
 **     frwd: 全局对象
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述:
 **注意事项:
 **作    者: # Qifeng.zou # 2016.02.24 14:24:06 #
 ******************************************************************************/
static int frwd_reg_rsp_cb(frwd_cntx_t *frwd)
{
#define FRWD_REG_RSP_CB(frwd, type, proc, args) \
    if (rtsd_register((frwd)->upload, type, (rtmq_reg_cb_t)proc, (void *)args)) { \
        log_error((frwd)->log, "Register type [%d] failed!", type); \
        return FRWD_ERR; \
    }

    FRWD_REG_RSP_CB(frwd, MSG_SEARCH_WORD_RSP, frwd_search_word_rsp_hdl, frwd);
    FRWD_REG_RSP_CB(frwd, MSG_INSERT_WORD_RSP, frwd_insert_word_rsp_hdl, frwd);

    return FRWD_OK;
}

/******************************************************************************
 **函数名称: frwd_search_word_req_hdl
 **功    能: 搜索关键字请求处理
 **输入参数:
 **     type: 数据类型
 **     orig: 源结点ID
 **     data: 需要转发的数据
 **     len: 数据长度
 **     args: 附加参数
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述: 将收到的请求转发给倒排服务
 **注意事项:
 **作    者: # Qifeng.zou # 2016.02.23 20:25:53 #
 ******************************************************************************/
static int frwd_search_word_req_hdl(int type, int orig, char *data, size_t len, void *args)
{
    frwd_cntx_t *ctx = (frwd_cntx_t *)args;
    mesg_header_t *head = (mesg_header_t *)data;

    log_trace(ctx->log, "Call %s()", __func__);

    /* > 字节序转换 */
    mesg_head_ntoh(head, head);

    log_trace(ctx->log, "serial:%lu type:%d len:%d flag:%d mark:[0x%X/0x%X]",
            head->serial, head->type, head->length, head->flag,
            head->mark, AGENT_MSG_MARK_KEY);

    mesg_head_hton(head, head);
    /* > 发送数据 */
    if (rtsd_cli_send(ctx->upload, type, data, len)) {
        log_error(ctx->log, "Push data into send queue failed! type:%u", type);
        return -1;
    }

    return 0;
}

/******************************************************************************
 **函数名称: frwd_search_word_rsp_hdl
 **功    能: 搜索关键字应答处理
 **输入参数:
 **     type: 数据类型
 **     orig: 源结点ID
 **     data: 需要转发的数据
 **     len: 数据长度
 **     args: 附加参数
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述: 将收到的搜索应答转发至帧听层
 **注意事项:
 **作    者: # Qifeng.zou # 2015.06.10 #
 ******************************************************************************/
static int frwd_search_word_rsp_hdl(int type, int orig, char *data, size_t len, void *args)
{
    serial_t serial;
    frwd_cntx_t *ctx = (frwd_cntx_t *)args;
    mesg_data_t *info = (mesg_data_t *)data;

    log_trace(ctx->log, "Call %s()", __func__);

    serial.serial = ntoh64(info->serial);

    /* > 发送数据 */
    if (rtrd_send(ctx->download, type, serial.nid, data, len)) {
        log_error(ctx->log, "Push data into send queue failed! type:%u", type);
        return -1;
    }

    return 0;
}

/******************************************************************************
 **函数名称: frwd_insert_word_req_hdl
 **功    能: 插入关键字的请求
 **输入参数:
 **     type: 数据类型
 **     orig: 源结点ID
 **     data: 需要转发的数据
 **     len: 数据长度
 **     args: 附加参数
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述:
 **注意事项:
 **作    者: # Qifeng.zou # 2016.02.23 20:26:55 #
 ******************************************************************************/
static int frwd_insert_word_req_hdl(int type, int orig, char *data, size_t len, void *args)
{
    frwd_cntx_t *ctx = (frwd_cntx_t *)args;

    log_trace(ctx->log, "Call %s()", __func__);

    /* > 发送数据 */
    if (rtsd_cli_send(ctx->upload, type, data, len)) {
        log_error(ctx->log, "Push data into send queue failed! type:%u", type);
        return -1;
    }

    return 0;
}

/******************************************************************************
 **函数名称: frwd_insert_word_rsp_hdl
 **功    能: 插入关键字的应答
 **输入参数:
 **     type: 数据类型
 **     orig: 源结点ID
 **     data: 需要转发的数据
 **     len: 数据长度
 **     args: 附加参数
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述:
 **注意事项:
 **作    者: # Qifeng.zou # 2015.06.10 #
 ******************************************************************************/
static int frwd_insert_word_rsp_hdl(int type, int orig, char *data, size_t len, void *args)
{
    serial_t serial;
    frwd_cntx_t *ctx = (frwd_cntx_t *)args;
    mesg_insert_word_rsp_t *rsp = (mesg_insert_word_rsp_t *)data;

    log_trace(ctx->log, "Call %s()", __func__);

    serial.serial = ntoh64(rsp->serial);

    /* > 发送数据 */
    if (rtrd_send(ctx->download, type, serial.nid, data, len)) {
        log_error(ctx->log, "Push data into send queue failed! type:%u", type);
        return -1;
    }

    return 0;
}
