#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/types.h>

#include "shm_opt.h"
#include "syscall.h"

/******************************************************************************
 **函数名称: shm_ftok
 **功    能: 通过文件产生共享内存KEY值
 **输入参数: 
 **     path: 文件路径
 **     id: 编号
 **输出参数: NONE
 **返    回: 共享内存KEY值
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2015.01.19 #
 ******************************************************************************/
key_t shm_ftok(const char *path, int id)
{
    FILE *fp;

    if (access(path, F_OK))
    {
        Mkdir2(path, 0777);

        fp = fopen(path, "w");
        if (NULL == fp)
        {
            return -1;
        }

        fClose(fp);
    }
    
    return ftok(path, id);
}

/******************************************************************************
 **函数名称: shm_creat
 **功    能: 创建共享内存
 **输入参数: 
 **     key: 共享内存KEY
 **     size: 空间SIZE
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述: 
 **注意事项: 
 **     如果创建共享内存时出现错误，可能是因为SHMMAX限制了共享内存的大小.
 **     SHMMAX的默认值为32MB, 可在/proc/sys/kernel/shmmax中查看到SHMMAX的值
 **作    者: # Qifeng.zou # 2014.09.08 #
 ******************************************************************************/
void *shm_creat(int key, size_t size)
{
    int shmid;
    void *addr;

    /* 1 判断是否已经创建 */
    shmid = shmget(key, 0, 0);
    if(shmid >= 0)
    {
        return NULL;  /* 已创建 */
    }

    /* 2 异常，则退出处理 */
    if(ENOENT != errno)
    {
        return NULL;
    }

    /* 3 创建共享内存 */
    shmid = shmget(key, size, IPC_CREAT|0666);
    if(shmid < 0)
    {
        return NULL;
    }

    /* 4. ATTACH共享内存 */
    addr = (void *)shmat(shmid, NULL, 0);
    if((void *)-1 == addr)
    {
        return NULL;
    }

    return addr;
}

/******************************************************************************
 **函数名称: shm_attach
 **功    能: 附着共享内存
 **输入参数: 
 **     key: 共享内存KEY
 **     size: 空间SIZE
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2014.12.22 #
 ******************************************************************************/
void *shm_attach(int key, size_t size)
{
    int shmid;

    /* 1 判断是否已经创建 */
    shmid = shmget(key, 0, 0666);
    if(shmid >= 0)
    {
        return shmat(shmid, NULL, 0);  /* 已创建 */
    }

    return NULL;
}

/******************************************************************************
 **函数名称: shm_creat_and_attach
 **功    能: 创建或附着共享内存
 **输入参数: 
 **     key: 共享内存KEY
 **     size: 空间SIZE
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2014.09.08 #
 ******************************************************************************/
void *shm_creat_and_attach(int key, size_t size)
{
    int shmid;
    void *addr;

    /* 1 判断是否已经创建 */
    shmid = shmget(key, 0, 0666);
    if(shmid >= 0)
    {
        return shmat(shmid, NULL, 0);  /* 已创建 */
    }

    /* 2 异常，则退出处理 */
    if(ENOENT != errno)
    {
        return NULL;
    }

    /* 3 创建共享内存 */
    shmid = shmget(key, size, IPC_CREAT|0666);
    if(shmid < 0)
    {
        return NULL;
    }

    /* 4. ATTACH共享内存 */
    addr = (void *)shmat(shmid, NULL, 0);
    if((void *)-1 == addr)
    {
        return NULL;
    }

    return addr;
}

