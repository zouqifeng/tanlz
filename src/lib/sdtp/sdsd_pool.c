#include "comm.h"
#include "shm_opt.h"
#include "sdtp_comm.h"
#include "sdsd_pool.h"

/******************************************************************************
 **函数名称: sdsd_pool_creat
 **描    述: 创建发送池
 **输入参数:
 **     fpath: 共享内存路径
 **     max: 数据块最大个数
 **     size: 数据块SZ
 **输出参数: NONE
 **返    回: 发送池对象
 **实现描述:
 **注意事项:
 **作    者: # Qifeng.zou # 2015.04.11 #
 ******************************************************************************/
sdsd_pool_t *sdsd_pool_creat(const char *fpath, int max, int size)
{
    int idx;
    void *addr;
    size_t total;
    sdsd_pool_t *pool;
    sdsd_pool_head_t *head;

    total = SDSD_POOL_PAGE_NUM * (max * size) + sizeof(sdsd_pool_head_t);

    /* > 创建共享内存 */
    addr = shm_creat(fpath, total);
    if (NULL == addr) {
        return NULL;
    }

    /* > 创建全局对象 */
    pool = (sdsd_pool_t *)calloc(1, sizeof(sdsd_pool_t));
    if (NULL == pool) {
        return NULL;
    }

    /* > 初始化处理 */
    head = (sdsd_pool_head_t *)addr;

    memset(head, 0, sizeof(sdsd_pool_head_t));

    head->size = size;
    pool->head = head;

    for (idx=0; idx<SDSD_POOL_PAGE_NUM; ++idx) {
        ticket_lock_init(&head->page[idx].lock);
        head->page[idx].idx = idx;
        head->page[idx].size = size * max;
        head->page[idx].begin = sizeof(sdsd_pool_head_t) + idx*size*max; /* 偏移量 */
        head->page[idx].end =  sizeof(sdsd_pool_head_t) + (idx+1)*size*max;
        head->page[idx].off = 0;
        head->page[idx].mode = SDSD_MOD_WR;
        head->page[idx].send_tm = time(NULL);

        pool->addr[idx] = addr + head->page[idx].begin;
    }

    return pool;
}

/******************************************************************************
 **函数名称: sdsd_pool_attach
 **描    述: 附着发送池
 **输入参数:
 **     fpath: 共享内存路径
 **输出参数: NONE
 **返    回: 发送池对象
 **实现描述:
 **注意事项: 因创建时已经初始化相关数据, 因此此过程不用再初始化相关值.
 **作    者: # Qifeng.zou # 2015.04.11 #
 ******************************************************************************/
sdsd_pool_t *sdsd_pool_attach(const char *fpath)
{
    int idx;
    void *addr;
    sdsd_pool_t *pool;

    /* > 附着共享内存 */
    addr = shm_attach(fpath, 0);
    if (NULL == addr) {
        return NULL;
    }

    /* > 创建全局对象 */
    pool = (sdsd_pool_t *)calloc(1, sizeof(sdsd_pool_t));
    if (NULL == pool) {
        return NULL;
    }

    pool->head = (sdsd_pool_head_t *)addr;
    for (idx=0; idx<SDSD_POOL_PAGE_NUM; ++idx) {
        pool->addr[idx] = addr + pool->head->page[idx].begin;
    }

    return pool;
}

/******************************************************************************
 **函数名称: sdsd_pool_push
 **描    述: 将数据放入发送池
 **输入参数:
 **     pool: 发送池
 **     type: 数据类型
 **     data: 需要发送的数据
 **     len: 数据长度
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述:
 **注意事项:
 **作    者: # Qifeng.zou # 2015.04.11 #
 ******************************************************************************/
int sdsd_pool_push(sdsd_pool_t *pool, int type, int nid, const void *data, size_t len)
{
    int idx, num;
    sdtp_header_t *head;
    sdsd_pool_page_t *page;

    idx = rand() % SDSD_POOL_PAGE_NUM;

    for (num=0; num<SDSD_POOL_PAGE_NUM; ++num, ++idx) {
        idx = idx % SDSD_POOL_PAGE_NUM;

        page  = &pool->head->page[idx];

        if (page->off > 1 * GB) {
            assert(0);
        }

        ticket_lock(&page->lock);     /* 加锁 */

        if (SDSD_MOD_WR != page->mode) {
            ticket_unlock(&page->lock);
            continue; /* 无写入权限 */
        }

        if ((page->size - page->off) < (sizeof(sdtp_header_t) + len)) {
            ticket_unlock(&page->lock);
            continue; /* 空间不足 */
        }

        /* > 设置报头信息 */
        head = (sdtp_header_t *)(pool->addr[idx] + page->off);

        head->type = htons(type);
        head->nid = htonl(nid);
        head->length = htonl(len);
        head->flag = SDTP_EXP_MESG;  /* 外部数据 */
        head->chksum = htonl(SDTP_CHKSUM_VAL);

        /* > 设置报体信息 */
        page->off += sizeof(sdtp_header_t);
        memcpy(pool->addr[idx] + page->off, data, len);
        page->off += len;
        ++page->num;

        if (page->off > 1 * GB) {
            assert(0);
        }

        ticket_unlock(&page->lock);   /* 加锁 */
        return SDTP_OK;
    }

    return SDTP_ERR;
}

/******************************************************************************
 **函数名称: sdsd_pool_switch
 **描    述: 切换发送池
 **输入参数:
 **     pool: 发送池
 **输出参数: NONE
 **返    回: 发送页
 **实现描述: 当发送页超时或容量超过50%时, 便可切换发送池
 **注意事项:
 **作    者: # Qifeng.zou # 2015.04.11 #
 ******************************************************************************/
sdsd_pool_page_t *sdsd_pool_switch(sdsd_pool_t *pool)
{
    int idx;
    time_t ctm = time(NULL);
    sdsd_pool_page_t *page;

    /* > 改变状态 */
    for (idx=0; idx<SDSD_POOL_PAGE_NUM; ++idx) {
        page = &pool->head->page[idx];

        if (page->off > page->size) {
            assert(0);
        }

        if (SDSD_MOD_RD == page->mode) {
            page->off = 0;
            page->num = 0;
            page->send_tm = ctm;
            page->mode = SDSD_MOD_WR; /* 发送完全 */
            continue;
        }
    }

    /* > 选择发送缓存 */
    for (idx=0; idx<SDSD_POOL_PAGE_NUM; ++idx) {
        page = &pool->head->page[idx];

        if (page->off > 1 * GB) {
            assert(0);
        }

        /* 当缓存分配超过50%或超时5s时 则可发送缓存内容 */
        if ((page->off > (page->size >> 1))
            || ((ctm - page->send_tm >= 5) && (page->off > 0)))
        {
            ticket_lock(&page->lock);
            page->mode = SDSD_MOD_RD;
            ticket_unlock(&page->lock);
            return page;
        }
    }

    return NULL;
}
