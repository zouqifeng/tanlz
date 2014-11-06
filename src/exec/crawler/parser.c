/******************************************************************************
 ** Coypright(C) 2014-2024 Xundao technology Co., Ltd
 **
 ** 文件名: crwl_parser.c
 ** 版本号: 1.0
 ** 描  述: 超链接的提取程序
 **         从爬取的网页中提取超链接
 ** 作  者: # Qifeng.zou # 2014.10.17 #
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <libgen.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "list.h"
#include "http.h"
#include "redis.h"
#include "xd_str.h"
#include "common.h"
#include "syscall.h"
#include "crawler.h"
#include "xml_tree.h"
#include "crwl_conf.h"
#include "crwl_parser.h"

int main(int argc, char *argv[])
{
    log_cycle_t *log;
    crwl_conf_t *conf;
    crwl_parser_t *parser;

    /* 1. 初始化日志模块 */
    log = crwl_init_log(argv[0]);
    if (NULL == log)
    {
        return CRWL_ERR;
    }

    /* 2. 加载配置信息 */
    conf = crwl_conf_creat("../conf/crawler.xml", log);
    if (NULL == conf)
    {
        log_error(log, "Initialize log failed!");
        return CRWL_ERR;
    }

#if !defined(__MEM_LEAK_CHECK__)
    daemon(1, 0);
#endif /*__MEM_LEAK_CHECK__*/

    /* 3. 初始化Parser对象 */
    parser = crwl_parser_init(conf, log);
    if (NULL == parser)
    {
        log_error(log, "Init parser failed!");
        return CRWL_ERR;
    }

    /* 4. 处理网页信息 */
    crwl_parser_work(parser);

    /* 5. 释放GUMBO对象 */
    crwl_parser_destroy(parser);

    return CRWL_OK;
}