/******************************************************************************
 * Copyright (C) 2018-2020
 * file:    libcmd.c
 * author:  zucker.chen<timeontheway@163.com>
 * created: 2018-12-10 18:00
 * updated: 2018-12-10 18:00
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

#define CMD_NAME_MAX_LEN    32      // Max character length of a command string
#define CMD_ARGC_MAX_NUM    8       // Max number of command parameters supported
#define CMD_ARGS_MAX_LEN    16      // Max character length of a command parameter

#define CMD_LINK_DIR   "./"
#define CMD_LINK_BIN   "./test_libcmd"


typedef struct cmd_data {
    char cmd[CMD_NAME_MAX_LEN];                     // cmd name
    int  argc;                                      // cmd number
    char argv[CMD_ARGC_MAX_NUM][CMD_ARGS_MAX_LEN];  // cmd parameters content
} cmd_data_t;

typedef struct list_head list_head_t;
typedef int (*cmd_cb_t)(int, char **);

typedef struct cmd {
    list_head_t node;               // list node
    char str[CMD_NAME_MAX_LEN];     // cmd name
    cmd_cb_t func;                  // cmd callback point
} cmd_t;

list_head_t cmd_list;
static mq_sysv_ctx_t *mq_ctx;



/*
 * ===>>> cmd input and result ack
 */

static int cmd_args_ack(char *buf, int size)
{
    //printf("%s:%d, sizeof(cmd_data_t) = %d, data size = %d\n", __FUNCTION__, __LINE__, (int)sizeof(cmd_data_t), size);
    printf("%s:%d, buf = %s\n", __FUNCTION__, __LINE__, buf);
    
    return 0;
}

int cmd_args_proc(int argc, char **argv)
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
    
    mq_sysv_ctx_t *ctx = mq_init_client(MSGQ_KEY_S, MSGQ_KEY_C, NULL);  //cmd_args_ack
    if (ctx == NULL) {
        printf("%s:%d, mq_init_client error!\n", __FUNCTION__, __LINE__);
        return -1;
    }
    if (mq_send(ctx->msgid_s, &cmd_data, (int)sizeof(cmd_data_t)) < 0) {   // to cmd_mq_recv
        printf("%s:%d, mq_send error!\n", __FUNCTION__, __LINE__);
        return -1;
    }
    
    char buf[1024];
    int size;
    if ((size = mq_recv(ctx->msgid_c, buf, 128)) <= 0) {
        printf("%s:%d, mq_recv error!\n", __FUNCTION__, __LINE__);
        return -1;
    } else {
        cmd_args_ack(buf, size);
    }
    
    
    //sleep(3);
    mq_deinit_client(ctx);
    
    return 0;
}




/*
 * ===>>> cmd service
 */

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
            ret = cmd->func((int)cmd_data->argc, (char **)cmd_data->argv);
            if (ret < 0) {
                printf("%s:%d, %s access error!\n", __FUNCTION__, __LINE__, cmd->str);
            }
            break;
        }
    }
    if (node == &cmd_list) {
        printf("%s:%d, cannot find cmd = %s!\n", __FUNCTION__, __LINE__, cmd_data->cmd);
        return -1;
    }
    
    if (mq_send(mq_ctx->msgid_c, "Access OK !", 32) < 0) {      // to cmd_args_ack
        printf("%s:%d, mq_send error!\n", __FUNCTION__, __LINE__);
        return -1;
    }
    
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

int cmd_deinit(void)
{
    if (mq_ctx != NULL) {
        mq_deinit_server(mq_ctx);
        mq_ctx = NULL;
    }
    
    cmd_unregister(NULL);
    
    return 0;
}

int cmd_register(const char *name, cmd_cb_t func)
{
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

    // create soft link
    char target[128];
	sprintf(target, CMD_LINK_DIR"%s",new_cmd->str);
	if (symlink(CMD_LINK_BIN,target) < 0) {
        printf("%s:%d, symlink(%s) error: %s\n", __FUNCTION__, __LINE__, target, strerror(errno));
    }
    
    return 0;
}

// name == NULL, unregister all
int cmd_unregister(const char *name)
{
    list_head_t *node, *next;
    cmd_t *cmd;
    list_for_each_safe(node, next, &cmd_list) {
        cmd = list_entry(node, cmd_t, node);
        if (name != NULL) {
            if (strcmp(cmd->str, name) == 0) {
                list_remove(&cmd->node);
                free(cmd);
                break;
            }
        } else {
            list_remove(&cmd->node);
            free(cmd);
        }
    }

    return 0;
}


