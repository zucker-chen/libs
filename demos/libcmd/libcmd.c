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


// used for cmd server
static cmd_ctx_t *g_cmd_ctx;


/*
 * ======================parting line =======================
 * ===>>> cmd client(input and result ack)
 */
int cmd_args_proc(int mq_key, int argc, char **argv, cmd_cb_t func)
{
    char buf[MQ_MAX_BUF_LEN] = {0};
    char *pstr = NULL;
    cmd_data_t cmd_data = {0};
	mq_handle_t *ctx = NULL;
    int i = 0, size = 0;
    
    pstr = rindex(argv[0],'/');
    if (pstr == NULL) {
        pstr = argv[0];
    } else {
        pstr++;
    }

    strncpy(cmd_data.cmd,  pstr, CMD_NAME_MAX_LEN);
    cmd_data.argc = argc > CMD_ARGC_MAX_NUM ? CMD_ARGC_MAX_NUM+1 : argc-1;
    for (i = 0; i < cmd_data.argc && i < CMD_ARGC_MAX_NUM; i++)
    {
        strncpy(cmd_data.argv[i], argv[i+1], CMD_ARGS_MAX_LEN);
        printf("%s(%d): cmd_data.argv[%d] = %s\n", __FUNCTION__, __LINE__, i, cmd_data.argv[i]);
    }
	if (isatty(STDOUT_FILENO)) {
		strncpy(cmd_data.argv[i], ttyname(STDOUT_FILENO), CMD_ARGS_MAX_LEN);
	}

    ctx = mq_init_client(mq_key, NULL); 
    if (ctx == NULL) {
        printf("%s(%d): mq_init_client error!\n", __FUNCTION__, __LINE__);
        return -1;
    }
    if (mq_send(ctx, &cmd_data, (int)sizeof(cmd_data_t)) < 0) {   // to cmd_mq_recv
        printf("%s(%d): mq_send error!\n", __FUNCTION__, __LINE__);
        return -1;
    }
    
    if ((size = mq_recv(ctx, buf, MQ_MAX_BUF_LEN)) <= 0) {
        printf("%s(%d): mq_recv error, size = %d\n", __FUNCTION__, __LINE__, size);
        return -1;
    } else if (func != NULL) {
        func(size, (char **)&buf, NULL);
    } else {
		printf("%s:%d %s\n", __FUNCTION__, __LINE__, (char *)buf);
	}
    
    mq_deinit_client(ctx);
    
    return 0;
}




/*
 * ======================parting line =======================
 * ===>>> cmd server(access callback function of register cmd)
 */
/* cmd: arguments test */ 
static int cmd_args_test(int argc, char **argv, char *ack)
{
    printf("%s:%d\n", __FUNCTION__, __LINE__);
    int i = 0, len = 0;
    char (*args)[CMD_ARGS_MAX_LEN] = (char (*)[CMD_ARGS_MAX_LEN])argv;

    for (i = 0; i < argc; i++) {
        len += sprintf(ack + len, "%s:%d args[%d] = %s\n", __FUNCTION__, __LINE__, i, args[i]);
    }
    
    return 0;
}

/* cmd: arguments test */ 
static int cmd_show_all(int argc, char **argv, char *ack)
{
    printf("%s:%d\n", __FUNCTION__, __LINE__);
    list_head_t *node, *next;
    cmd_t *cmd;
    int cnt = 0, len = 0;
    
    len += sprintf(ack, "All cmd list:\n");
    list_for_each_safe(node, next, &g_cmd_ctx->cmd_list) {
        cmd = list_entry(node, cmd_t, node);
        cnt++;
        len += sprintf(ack + len, "%d\t%s\t\t%s\n", cnt, cmd->str, cmd->help);
    }
    len += sprintf(ack + len, "There are %d cmds.\n", cnt);
    
    return 0;
}

/* cmd: dump print info for debug */ 
static int cmd_tty_dump(int argc, char **argv, char *ack)
{
    printf("%s:%d\n", __FUNCTION__, __LINE__);
    char (*args)[CMD_ARGS_MAX_LEN] = (char (*)[CMD_ARGS_MAX_LEN])argv;
    
    if (0 == strcmp(args[0], "1") && argc == 1) {
		if (strlen(args[argc]) <= 0) {
			sprintf(ack, "%s(%d): tty name is NULL\n", __FUNCTION__, __LINE__);
			return -1;
		}
        int fd = open(args[argc], O_RDWR | S_IREAD | S_IWRITE);
		if (fd > 0) {
	        dup2(fd, STDOUT_FILENO);
	        dup2(fd, STDERR_FILENO);
	        ioctl(fd, TIOCCONS);
			stdout = fdopen(STDOUT_FILENO, "w+");	// must
	        close(fd);
		}
        sprintf(ack, "%s(%d): ON-> %s redirect to %s\n", __FUNCTION__, __LINE__, g_cmd_ctx->tty_name, args[argc]);
    } else if (0 == strcmp(args[0], "0") && argc == 1) {
		// reset tty
        int fd = open(g_cmd_ctx->tty_name, O_RDWR | S_IREAD | S_IWRITE);
		if (fd > 0) {
	        dup2(fd, STDOUT_FILENO);
	        dup2(fd, STDERR_FILENO);
	        close(fd);
		}
		// reset sys console
		ioctl(open("/dev/console", O_RDWR), TIOCCONS);
		sprintf(ack, "%s(%d): OFF-> %s redirect to %s\n", __FUNCTION__, __LINE__, args[argc], g_cmd_ctx->tty_name);
    } else {
        sprintf(ack, "v-cmd-tty-dump [param]; param: 1=ON; 0=OFF;\n");
    }
    
    return 0;
}
 
/*
 *   func: find and access cmd cb(callback funcion) after receive mq from client
 */
static int cmd_mq_recv(mq_handle_t *ctx, char *buf, int size)
{
    cmd_data_t *cmd_data = (cmd_data_t *)buf;
    char ack[MQ_MAX_BUF_LEN] = {0};
    
    list_head_t *node, *next;
    cmd_t *cmd;
    int ret = 0;
    list_for_each_safe(node, next, &g_cmd_ctx->cmd_list) {
        cmd = list_entry(node, cmd_t, node);
        if (strcmp(cmd->str, cmd_data->cmd) == 0) {
            ret = cmd->func((int)cmd_data->argc, (char **)cmd_data->argv, &ack[0]);
            if (ret < 0) {
                sprintf(ack + strlen(ack), "ACK: cmd(%s) access error!\n", cmd->str);
            }
            break;
        }
    }
    if (node == &g_cmd_ctx->cmd_list) {
        sprintf(ack + strlen(ack), "ACK: cmd(%s) cannot find!\n", cmd_data->cmd);
        ret = -1;
    }
    
    if (ret >= 0) {
        sprintf(ack + strlen(ack), "ACK: cmd(%s) Access OK !\n", cmd_data->cmd);
    }
    
    if (mq_send(ctx, ack, strlen(ack)) < 0) {      // to cmd_args_ack
        printf("%s(%d): mq_send error!\n", __FUNCTION__, __LINE__);
        return -1;
    }
    
    return 0;
}

int cmd_init(int mq_key, char *filename)
{
    g_cmd_ctx = calloc(1, sizeof(cmd_ctx_t));
    if (g_cmd_ctx == NULL) {
        printf("%s(%d): cmd_init malloc error!\n", __FUNCTION__, __LINE__);
    }
    
    g_cmd_ctx->mq_key = mq_key;
    strncpy(g_cmd_ctx->client_dist_file, filename, CMD_FILENAME_MAX_LEN);
    
    LIST_INIT_HEAD(&g_cmd_ctx->cmd_list);
  
	g_cmd_ctx->mq_ctx = mq_init_server(g_cmd_ctx->mq_key, cmd_mq_recv);
    if (g_cmd_ctx->mq_ctx == NULL) {
        printf("%s(%d): mq_init_server error!\n", __FUNCTION__, __LINE__);
    }

    if (cmd_register("v-cmd-args-test", cmd_args_test, "test cmd arguments") < 0) {
        printf("%s(%d): v-cmd-args-test cmd_register error!\n", __FUNCTION__, __LINE__);
        return -1;
    }

	if (isatty(STDOUT_FILENO)) {
	    strncpy(g_cmd_ctx->tty_name, ttyname(STDOUT_FILENO), CMD_NAME_MAX_LEN);
	}
    if (cmd_register("v-cmd-tty-dump", cmd_tty_dump, "dump all tty info for debug") < 0) {
        printf("%s(%d): v-cmd-tty-dump cmd_register error!\n", __FUNCTION__, __LINE__);
        return -1;
    }

    if (cmd_register("v-cmd-show-all", cmd_show_all, "show all cmd info list") < 0) {
        printf("%s(%d): v-cmd-show-all cmd_register error!\n", __FUNCTION__, __LINE__);
        return -1;
    }
    
    return 0;
}


int cmd_deinit(void)
{    
    if (g_cmd_ctx->mq_ctx != NULL) {
        mq_deinit_server(g_cmd_ctx->mq_ctx);
        g_cmd_ctx->mq_ctx = NULL;
    }
    
    cmd_unregister(NULL);
    
    if (g_cmd_ctx) {
        free(g_cmd_ctx);
		g_cmd_ctx = NULL;
    }
    
    return 0;
}

/*
 *   func: register cmd by cmd name and callback function
 */
int cmd_register(const char *name, cmd_cb_t func, const char *help)
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
    new_cmd->help = (char *)help;
    
    list_insert_after(&new_cmd->node, &g_cmd_ctx->cmd_list);

    // create soft link
    char target[CMD_FILENAME_MAX_LEN];
	sprintf(target, CMD_LINK_DIR"/%s", new_cmd->str);
	if (symlink(g_cmd_ctx->client_dist_file, target) < 0) {
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
    list_for_each_safe(node, next, &g_cmd_ctx->cmd_list) {
        cmd = list_entry(node, cmd_t, node);
        if (name != NULL) {
            if (strcmp(cmd->str, name) == 0) {
                // remove soft link
                char target[CMD_FILENAME_MAX_LEN];
                sprintf(target, CMD_LINK_DIR"/%s",cmd->str);
                if (unlink(target) < 0) {
                    printf("%s(%d): unlink(%s) error: %s\n", __FUNCTION__, __LINE__, target, strerror(errno));
                }
                // remove cmd node
                list_remove(&cmd->node);
                free(cmd);
                break;
            }
        } else {
            // remove soft link
            char target[CMD_FILENAME_MAX_LEN];
            sprintf(target, CMD_LINK_DIR"/%s",cmd->str);
            if (unlink(target) < 0) {
                printf("%s(%d): unlink(%s) error: %s\n", __FUNCTION__, __LINE__, target, strerror(errno));
            }
            // remove cmd node
            list_remove(&cmd->node);
            free(cmd);
        }
    }

    return 0;
}


