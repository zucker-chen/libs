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
#include <sys/ioctl.h>

#include "libmsgq.h"
#include "list.h"
#include "libcmd.h"



typedef struct list_head list_head_t;

typedef struct cmd {
    list_head_t node;               // list node
    char str[CMD_NAME_MAX_LEN];     // cmd name
    cmd_cb_t func;                  // cmd callback point
} cmd_t;

static list_head_t cmd_list;
static mq_sysv_ctx_t *mq_ctx;
static char tty_name[CMD_NAME_MAX_LEN];


/*
 * ======================parting line =======================
 * ===>>> cmd client(input and result ack)
 */

int cmd_args_proc(int argc, char **argv, cmd_cb_t func)
{
    int i = 0;
    int size;
    char buf[MQ_MAX_BUF_LEN];
    char *pstr = NULL;
    cmd_data_t cmd_data = {0};
    
    pstr = rindex(argv[0],'/');
    if (pstr == NULL) {
        pstr = argv[0];
    } else {
        pstr++;
    }

    strncpy(cmd_data.cmd,  pstr, CMD_NAME_MAX_LEN);
    cmd_data.argc = argc > CMD_ARGC_MAX_NUM ? CMD_ARGC_MAX_NUM+1 : argc;
    for (i = 0; i < cmd_data.argc-1 && i < CMD_ARGC_MAX_NUM; i++)
    {
        strncpy(cmd_data.argv[i], argv[i+1], CMD_ARGS_MAX_LEN);
        //printf("%s(%d): cmd_data.argv[%d] = %s\n", __FUNCTION__, __LINE__, i, cmd_data.argv[i]);
    }
	if (isatty(STDOUT_FILENO)) {
		strncpy(cmd_data.argv[i], ttyname(STDOUT_FILENO), CMD_ARGS_MAX_LEN);
	}

    mq_sysv_ctx_t *ctx = mq_init_client(MSGQ_KEY, (int)getpid(), NULL);  // mq_init_client(MSGQ_KEY, MSGQ_KEY_C, cmd_args_ack)
    if (ctx == NULL) {
        printf("%s(%d): mq_init_client error!\n", __FUNCTION__, __LINE__);
        return -1;
    }
    if (mq_send(ctx->msgid_s, &cmd_data, (int)sizeof(cmd_data_t)) < 0) {   // to cmd_mq_recv
        printf("%s(%d): mq_send error!\n", __FUNCTION__, __LINE__);
        return -1;
    }
    
    if ((size = mq_recv(ctx->msgid_c, buf, MQ_MAX_BUF_LEN)) <= 0) {
        printf("%s(%d): mq_recv error!\n", __FUNCTION__, __LINE__);
        return -1;
    } else {
        func(size, (char **)&buf);
    }
    
    mq_deinit_client(ctx);
    
    return 0;
}




/*
 * ======================parting line =======================
 * ===>>> cmd server(access callback function of register cmd)
 */

/* cmd: arguments test */ 
static int cmd_args_test(int argc, char **argv)
{
    int i = 0;
    char (*args)[CMD_ARGS_MAX_LEN] = (char (*)[CMD_ARGS_MAX_LEN])argv;

    for (i = 0; i < argc; i++) {
        printf("%s(%d): args[%d] = %s\n", __FUNCTION__, __LINE__, i, args[i]);
    }
    
    return 0;
}

/* cmd: arguments test */ 
static int cmd_show_all(int argc, char **argv)
{
    list_head_t *node, *next;
    cmd_t *cmd;
    int cnt = 0;
    
    printf("All cmd list:\n");
    list_for_each_safe(node, next, &cmd_list) {
        cmd = list_entry(node, cmd_t, node);
        printf("%s\t", cmd->str);
        cnt++;
    }
    printf("\nThere are %d cmds.\n", cnt);
    
    return 0;
}

/* cmd: dump print info for debug */ 
static int cmd_tty_dump(int argc, char **argv)
{
    char (*args)[CMD_ARGS_MAX_LEN] = (char (*)[CMD_ARGS_MAX_LEN])argv;
    
    if (0 == strcmp(args[0], "1") && argc == 2) {
		if (strlen(args[argc-1]) <= 0) {
			printf("%s(%d): tty name is NULL\n", __FUNCTION__, __LINE__);
			return -1;
		}
        int fd = open(args[argc-1], O_RDWR | S_IREAD | S_IWRITE);
        dup2(fd, STDOUT_FILENO);
        printf("%s(%d): ON-> %s redirect to %s\n", __FUNCTION__, __LINE__, tty_name, args[argc-1]);
        close(fd);
    } else if (0 == strcmp(args[0], "0") && argc == 2) {
        int fd = open(tty_name, O_RDWR | S_IREAD | S_IWRITE);
        dup2(fd, STDOUT_FILENO);
        printf("%s(%d): OFF-> %s redirect to %s\n", __FUNCTION__, __LINE__, args[argc-1], tty_name);
        close(fd);
    } else if (0 == strcmp(args[0], "2") && argc == 2) {
		if (strlen(args[argc-1]) <= 0) {
			printf("%s(%d): tty name is NULL\n", __FUNCTION__, __LINE__);
			return -1;
		}
        int fd = open(args[argc-1], O_RDWR | S_IREAD | S_IWRITE);
        ioctl(fd, TIOCCONS);
        printf("%s(%d): UART-> %s redirect to %s\n", __FUNCTION__, __LINE__, "UART", args[argc-1]);
        close(fd);
    } else {
        printf("v-cmd-tty-dump [param]; param: 0=ON; 1=OFF; 2=CONSOLE(UART)\n");
    }
    
    return 0;
}
 
/*
 *   func: find and access cmd cb(callback funcion) after receive mq from client
 */
static int cmd_mq_recv(char *buf, int size)
{
    cmd_data_t *cmd_data = (cmd_data_t *)buf;
    char ack[MQ_MAX_BUF_LEN];
    
    list_head_t *node, *next;
    cmd_t *cmd;
    int ret = 0;
    list_for_each_safe(node, next, &cmd_list) {
        cmd = list_entry(node, cmd_t, node);
        if (strcmp(cmd->str, cmd_data->cmd) == 0) {
            ret = cmd->func((int)cmd_data->argc, (char **)cmd_data->argv);
            if (ret < 0) {
                sprintf(ack, "ACK: cmd(%s) access error!\n", cmd->str);
            }
            break;
        }
    }
    if (node == &cmd_list) {
        sprintf(ack, "ACK: cmd(%s) cannot find!\n", cmd_data->cmd);
        ret = -1;
    }
    
    if (ret >= 0) {
        sprintf(ack, "ACK: cmd(%s) Access OK !\n", cmd_data->cmd);
    }
    
    if (mq_send(mq_ctx->msgid_c, ack, MQ_MAX_BUF_LEN) < 0) {      // to cmd_args_ack
        printf("%s(%d): mq_send error!\n", __FUNCTION__, __LINE__);
        return -1;
    }
    
    return 0;
}

int cmd_init(void)
{
    LIST_INIT_HEAD(&cmd_list);

	mq_ctx = mq_init_server(MSGQ_KEY, cmd_mq_recv);
    if (mq_ctx == NULL) {
        printf("%s(%d): mq_init_server error!\n", __FUNCTION__, __LINE__);
    }

    if (cmd_register("v-cmd-args-test", cmd_args_test) < 0) {
        printf("%s(%d): v-cmd-args-test cmd_register error!\n", __FUNCTION__, __LINE__);
        return -1;
    }

    strncpy(tty_name, ttyname(STDOUT_FILENO), CMD_NAME_MAX_LEN);
    if (cmd_register("v-cmd-tty-dump", cmd_tty_dump) < 0) {
        printf("%s(%d): v-cmd-tty-dump cmd_register error!\n", __FUNCTION__, __LINE__);
        return -1;
    }

    if (cmd_register("v-cmd-show-all", cmd_show_all) < 0) {
        printf("%s(%d): v-cmd-show-all cmd_register error!\n", __FUNCTION__, __LINE__);
        return -1;
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

/*
 *   func: register cmd by cmd name and callback function
 */
int cmd_register(const char *name, cmd_cb_t func)
{
    cmd_t *new_cmd = NULL;

    if (func == NULL) {
        printf("%s(%d): func(callback) is NULL!\n", __FUNCTION__, __LINE__);
    }
    
    new_cmd = calloc(1, sizeof(cmd_t));
    if (new_cmd == NULL) {
        printf("%s(%d): calloc error: %s\n", __FUNCTION__, __LINE__, strerror(errno));
        return -1;
    }
    if (strncpy(new_cmd->str, name, CMD_NAME_MAX_LEN) == NULL) {
        printf("%s(%d): strncpy error: %s\n", __FUNCTION__, __LINE__, strerror(errno));
       return -1;
    }
    new_cmd->func = func;
    
    list_insert_after(&new_cmd->node, &cmd_list);

    // create soft link
    char target[128];
	sprintf(target, CMD_LINK_DIR"%s",new_cmd->str);
	if (symlink(CMD_LINK_BIN,target) < 0) {
        printf("%s(%d): symlink(%s) error: %s\n", __FUNCTION__, __LINE__, target, strerror(errno));
    }
    
    return 0;
}

/*
 *   func: unregister cmd by cmd name
 *   note: name == NULL, unregister all
 */
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


