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

#include "libmsgq.h"
#include "list.h"
#include "libcmd.h"


#define CMD_LINK_FIFO            "libcmd_shell"
#define MSGQ_KEY            12345678


typedef struct list_head list_head_t;
typedef struct cmd {
    list_head_t node;
    char str[CMD_NAME_MAX_LEN];
    cmd_cb func;
} cmd_t;

list_head_t cmd_list;
static mq_sysv_ctx_t *mq_ctx;




int cmd_get_input(char *buf, int size)
{
    printf("%s:%d, sizeof(cmd_data_t) = %d, data size = %d\n", __FUNCTION__, __LINE__, sizeof(cmd_data_t), size);
    cmd_data_t *cmd_data = buf;
    
    list_head_t *node, *next;
    cmd_t *cmd;
    list_for_each_safe(node, next, &cmd_list) {
        cmd = list_entry(node, cmd_t, node);
        if (strcmp(cmd->str, cmd_data->cmd) == 0) {
            cmd->cb((int)cmd_data->argc, (char **)cmd_data->argv);
            break;
        }
    }
    if (node == &cmd_list) {
        printf("%s:%d, cannot find cmd = %s!\n", __FUNCTION__, __LINE__, cmd_data->cmd);
        return -1;
    }
    
    return 0;
}

int cmd_init(void)
{
    LIST_INIT_HEAD(&cmd_list);

	mq_ctx = mq_init_server(MSGQ_KEY, cmd_get_input);
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

int cmd_register(cmd_t *cmd, const char *name, cmd_cb func)
{
    cmd_t *new_cmd = NULL;
    new_cmd = calloc(0, sizeof(cmd_t));
    if (new_cmd == NULL) {
        printf("%s:%d, calloc error!\n", __FUNCTION__, __LINE__);
        return -1;
    }
    
    if (strncpy(new_cmd->str, name, CMD_NAME_MAX_LEN) == NULL) {
        printf("%s:%d, strncpy error!\n", __FUNCTION__, __LINE__);
       return -1;
    }
    new_cmd->func = func;
    
    list_insert_after(&new_cmd->node, &cmd->node);

    return 0;
}


