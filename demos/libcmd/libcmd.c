/******************************************************************************
 * Copyright (C) 2014-2015
 * file:    libcmd.c
 * author:  gozfree <gozfree@163.com>
 * created: 2015-10-31 16:58
 * updated: 2015-10-31 16:58
 ******************************************************************************/
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stddef.h>
#include <errno.h>

#include "libmsgq.h"
#include "list.h"
#include "libcmd.h"


#define CMD_LINK_FIFO            "libcmd_shell"
#define MSGQ_KEY_S            12345678
#define MSGQ_KEY_C            12345679

#define CMD_NAME_MAX_LEN    32      // 命令字符串最大字符长度
#define CMD_ARGC_MAX_NUM    8       // 命令参数支持最大个数
#define CMD_ARGS_MAX_LEN    16      // 命令参数最大字符长度

#define CMD_LINK_DIR   "/home/zucker/Project/8.debug/libs/demos/libcmd/"
#define CMD_LINK_BIN   "/home/zucker/Project/8.debug/libs/demos/libcmd/test_libcmd"


typedef struct cmd_data {
    char cmd[CMD_NAME_MAX_LEN];
    int  argc;
    char argv[CMD_ARGC_MAX_NUM][CMD_ARGS_MAX_LEN];
} cmd_data_t;

typedef struct list_head list_head_t;
typedef int (*cmd_cb_t)(int, char **);

typedef struct cmd {
    list_head_t node;
    char str[CMD_NAME_MAX_LEN];
    cmd_cb_t func;
} cmd_t;

list_head_t cmd_list;
static mq_sysv_ctx_t *mq_ctx;




int cmd_args_ack(char *buf, int size)
{
    printf("%s:%d, sizeof(cmd_data_t) = %d, data size = %d\n", __FUNCTION__, __LINE__, (int)sizeof(cmd_data_t), size);
    printf("%s:%d, buf = %s\n", __FUNCTION__, __LINE__, buf);
    
    return 0;
}


int cmd_args_proc(int argc, char *argv[])
{
    int i = 0;
    char *pstr = NULL;
    cmd_data_t cmd_data;
    
    pstr = rindex(argv[0],'/');
    if (pstr == NULL) {
        pstr = argv[0];
    } else {
        pstr++;
    }
    printf("argc = %d, argv[0] = %s\n", argc, pstr);
    strncpy(cmd_data.cmd,  pstr, CMD_NAME_MAX_LEN);
    cmd_data.argc = argc - 1;
    for (i = 0; i < cmd_data.argc && i < CMD_ARGC_MAX_NUM; i++)
    {
        strncpy(cmd_data.argv[i], argv[i+1], CMD_ARGS_MAX_LEN);
        printf("cmd_data.argv[%d] = %s\n", i, cmd_data.argv[i]);
    }
    
    mq_sysv_ctx_t *ctx = mq_init_client(MSGQ_KEY_S, MSGQ_KEY_C, cmd_args_ack);
    mq_send(ctx->msgid_s, &cmd_data, (int)sizeof(cmd_data_t));
    
    sleep(3);
    mq_deinit_client(ctx);
    
    return 0;
}






static int cmd_mq_recv(char *buf, int size)
{
    printf("%s:%d, sizeof(cmd_data_t) = %d, data size = %d\n", __FUNCTION__, __LINE__, (int)sizeof(cmd_data_t), size);
    cmd_data_t *cmd_data = (cmd_data_t *)buf;
    
    list_head_t *node, *next;
    cmd_t *cmd;
    int ret = 0;
    list_for_each_safe(node, next, &cmd_list) {
        cmd = list_entry(node, cmd_t, node);
        if (strcmp(cmd->str, cmd_data->cmd) == 0) {
            cmd->func((int)cmd_data->argc, (char **)cmd_data->argv);
            break;
        }
    }
    if (node == &cmd_list || ret != 0) {
        printf("%s:%d, cannot find cmd = %s!\n", __FUNCTION__, __LINE__, cmd_data->cmd);
        return -1;
    }
    
    mq_send(mq_ctx->msgid_c, "Access OK !", 32);
    
    return 0;
}

int cmd_init(void)
{
    LIST_INIT_HEAD(&cmd_list);

	mq_ctx = mq_init_server(MSGQ_KEY_S, cmd_mq_recv);
    if (mq_ctx == NULL) {
        printf("%s:%d, mq_init_server error!\n", __FUNCTION__, __LINE__);
    }
    
    return 0;
}

int cmd_deinit(cmd_t *cmd)
{
    if (mq_ctx != NULL) {
        mq_deinit_server(mq_ctx);
        mq_ctx = NULL;
    }
    
    return 0;
}

int cmd_register(cmd_t *cmd, const char *name, cmd_cb_t func)
{
    printf("%s:%d\n", __FUNCTION__, __LINE__);
    cmd_t *new_cmd = NULL;
    new_cmd = calloc(1, sizeof(cmd_t));
    if (new_cmd == NULL) {
        printf("%s:%d, calloc error: %s\n", __FUNCTION__, __LINE__, strerror(errno));
        return -1;
    }
    
    if (strncpy(new_cmd->str, name, CMD_NAME_MAX_LEN) == NULL) {
        printf("%s:%d, strncpy error: %s\n", __FUNCTION__, __LINE__, strerror(errno));
       return -1;
    }
    new_cmd->func = func;
    
    list_insert_after(&new_cmd->node, &cmd_list);

    printf("%s:%d\n", __FUNCTION__, __LINE__);
    // create soft link
    char link[64];
	sprintf(link, CMD_LINK_DIR"%s",new_cmd->str);
	if (symlink(CMD_LINK_BIN,link) < 0) {
        printf("%s:%d, symlink error: %s\n", __FUNCTION__, __LINE__, strerror(errno));
    }
    printf("%s:%d, link = %s\n", __FUNCTION__, __LINE__, link);
    
    return 0;
}


