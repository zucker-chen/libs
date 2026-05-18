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
#include <pthread.h>

#include "libmsgq.h"
#include "list.h"
#include "libcmd.h"


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

    pstr = strrchr(argv[0], '/');
    if (pstr == NULL) {
        pstr = argv[0];
    } else {
        pstr++;
    }

    snprintf(cmd_data.cmd, CMD_NAME_MAX_LEN, "%s", pstr);
    cmd_data.argc = argc > CMD_ARGC_MAX_NUM ? CMD_ARGC_MAX_NUM : argc - 1;
    for (i = 0; i < cmd_data.argc && i < CMD_ARGC_MAX_NUM; i++) {
        snprintf(cmd_data.argv[i], CMD_ARGS_MAX_LEN, "%s", argv[i + 1]);
        printf("%s(%d): cmd_data.argv[%d] = %s\n", __FUNCTION__, __LINE__, i, cmd_data.argv[i]);
    }
    if (isatty(STDOUT_FILENO)) {
        snprintf(cmd_data.argv[i], CMD_ARGS_MAX_LEN, "%s", ttyname(STDOUT_FILENO));
    }

    ctx = mq_init_client(mq_key, NULL);
    if (ctx == NULL) {
        printf("%s(%d): mq_init_client error!\n", __FUNCTION__, __LINE__);
        return -1;
    }
    if (mq_send(ctx, &cmd_data, (int)sizeof(cmd_data_t)) < 0) {
        printf("%s(%d): mq_send error!\n", __FUNCTION__, __LINE__);
        mq_deinit_client(ctx);
        return -1;
    }

    if ((size = mq_recv(ctx, buf, MQ_MAX_BUF_LEN)) <= 0) {
        printf("%s(%d): mq_recv error, size = %d\n", __FUNCTION__, __LINE__, size);
        mq_deinit_client(ctx);
        return -1;
    } else if (func != NULL) {
        func(size, (char **)buf, NULL);
    } else {
        printf("%s:%d %s\n", __FUNCTION__, __LINE__, buf);
    }

    mq_deinit_client(ctx);

    return 0;
}




/*
 * ======================parting line =======================
 * ===>>> cmd server(access callback function of register cmd)
 */
static int cmd_args_test(int argc, char **argv, char *ack)
{
    printf("%s:%d\n", __FUNCTION__, __LINE__);
    int i = 0, len = 0;
    int remain = MQ_MAX_BUF_LEN;
    char (*args)[CMD_ARGS_MAX_LEN] = (char (*)[CMD_ARGS_MAX_LEN])argv;

    for (i = 0; i < argc && remain > 0; i++) {
        int n = snprintf(ack + len, remain, "%s:%d args[%d] = %s\n", __FUNCTION__, __LINE__, i, args[i]);
        if (n > 0) { len += n; remain -= n; }
    }

    return 0;
}

static int cmd_show_all(int argc, char **argv, char *ack)
{
    printf("%s:%d\n", __FUNCTION__, __LINE__);
    list_head_t *node, *next;
    cmd_t *cmd;
    int cnt = 0, len = 0;
    int remain = MQ_MAX_BUF_LEN;
    int n;

    n = snprintf(ack, remain, "All cmd list:\n");
    if (n > 0) { len += n; remain -= n; }

    pthread_mutex_lock(&g_cmd_ctx->list_mutex);
    list_for_each_safe(node, next, &g_cmd_ctx->cmd_list) {
        cmd = list_entry(node, cmd_t, node);
        cnt++;
        n = snprintf(ack + len, remain, "%d\t%s\t\t%s\n", cnt, cmd->str, cmd->help);
        if (n > 0) { len += n; remain -= n; }
        if (remain <= 0) break;
    }
    pthread_mutex_unlock(&g_cmd_ctx->list_mutex);

    snprintf(ack + len, remain > 0 ? remain : 0, "There are %d cmds.\n", cnt);

    return 0;
}

static int cmd_tty_dump(int argc, char **argv, char *ack)
{
    printf("%s:%d\n", __FUNCTION__, __LINE__);
    char (*args)[CMD_ARGS_MAX_LEN] = (char (*)[CMD_ARGS_MAX_LEN])argv;
    int remain = MQ_MAX_BUF_LEN;

    if (0 == strcmp(args[0], "1") && argc == 1) {
        if (strlen(args[argc]) <= 0) {
            snprintf(ack, remain, "%s(%d): tty name is NULL\n", __FUNCTION__, __LINE__);
            return -1;
        }
        int fd = open(args[argc], O_RDWR | S_IREAD | S_IWRITE);
        if (fd != -1) {
            dup2(fd, STDOUT_FILENO);
            dup2(fd, STDERR_FILENO);
            ioctl(fd, TIOCCONS);
            stdout = fdopen(STDOUT_FILENO, "w+");
            close(fd);
        }
        snprintf(ack, remain, "%s(%d): ON-> %s redirect to %s\n", __FUNCTION__, __LINE__, g_cmd_ctx->tty_name, args[argc]);
    } else if (0 == strcmp(args[0], "0") && argc == 1) {
        int fd = open(g_cmd_ctx->tty_name, O_RDWR | S_IREAD | S_IWRITE);
        if (fd != -1) {
            dup2(fd, STDOUT_FILENO);
            dup2(fd, STDERR_FILENO);
            close(fd);
        }
        int cfd = open("/dev/console", O_RDWR);
        if (cfd != -1) {
            ioctl(cfd, TIOCCONS);
            close(cfd);
        }
        snprintf(ack, remain, "%s(%d): OFF-> %s redirect to %s\n", __FUNCTION__, __LINE__, args[argc], g_cmd_ctx->tty_name);
    } else {
        snprintf(ack, remain, "v-cmd-tty-dump [param]; param: 1=ON; 0=OFF;\n");
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
    int ack_len = 0;
    int remain = MQ_MAX_BUF_LEN;

    list_head_t *node, *next;
    cmd_t *cmd;
    cmd_cb_t found_func = NULL;
    int ret = 0;

    pthread_mutex_lock(&g_cmd_ctx->list_mutex);
    list_for_each_safe(node, next, &g_cmd_ctx->cmd_list) {
        cmd = list_entry(node, cmd_t, node);
        if (strcmp(cmd->str, cmd_data->cmd) == 0) {
            found_func = cmd->func;
            break;
        }
    }
    if (node == &g_cmd_ctx->cmd_list) {
        snprintf(ack, remain, "ACK: cmd(%s) cannot find!\n", cmd_data->cmd);
        ret = -1;
    }
    pthread_mutex_unlock(&g_cmd_ctx->list_mutex);

    if (found_func) {
        ret = found_func((int)cmd_data->argc, (char **)cmd_data->argv, &ack[0]);
        if (ret < 0) {
            ack_len = strlen(ack);
            remain = MQ_MAX_BUF_LEN - ack_len;
            snprintf(ack + ack_len, remain > 0 ? remain : 0, "ACK: cmd(%s) access error!\n", cmd_data->cmd);
        }
    }

    if (ret >= 0) {
        ack_len = strlen(ack);
        remain = MQ_MAX_BUF_LEN - ack_len;
        snprintf(ack + ack_len, remain > 0 ? remain : 0, "ACK: cmd(%s) Access OK !\n", cmd_data->cmd);
    }

    if (mq_send(ctx, ack, strlen(ack) + 1) < 0) {
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
        return -1;
    }

    g_cmd_ctx->mq_key = mq_key;
    snprintf(g_cmd_ctx->client_dist_file, CMD_FILENAME_MAX_LEN, "%s", filename);

    LIST_INIT_HEAD(&g_cmd_ctx->cmd_list);
    pthread_mutex_init(&g_cmd_ctx->list_mutex, NULL);

    g_cmd_ctx->mq_ctx = mq_init_server(g_cmd_ctx->mq_key, cmd_mq_recv);
    if (g_cmd_ctx->mq_ctx == NULL) {
        printf("%s(%d): mq_init_server error!\n", __FUNCTION__, __LINE__);
        pthread_mutex_destroy(&g_cmd_ctx->list_mutex);
        free(g_cmd_ctx);
        g_cmd_ctx = NULL;
        return -1;
    }

    if (cmd_register("v-cmd-args-test", cmd_args_test, "test cmd arguments") < 0) {
        printf("%s(%d): v-cmd-args-test cmd_register error!\n", __FUNCTION__, __LINE__);
        return -1;
    }

    if (isatty(STDOUT_FILENO)) {
        snprintf(g_cmd_ctx->tty_name, CMD_NAME_MAX_LEN, "%s", ttyname(STDOUT_FILENO));
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
    if (g_cmd_ctx == NULL) {
        return 0;
    }

    if (g_cmd_ctx->mq_ctx != NULL) {
        mq_deinit_server(g_cmd_ctx->mq_ctx);
        g_cmd_ctx->mq_ctx = NULL;
    }

    cmd_unregister(NULL);

    pthread_mutex_destroy(&g_cmd_ctx->list_mutex);
    free(g_cmd_ctx);
    g_cmd_ctx = NULL;

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
        return -1;
    }

    new_cmd = calloc(1, sizeof(cmd_t));
    if (new_cmd == NULL) {
        printf("%s(%d): calloc error: %s\n", __FUNCTION__, __LINE__, strerror(errno));
        return -1;
    }
    snprintf(new_cmd->str, CMD_NAME_MAX_LEN, "%s", name);
    new_cmd->func = func;
    new_cmd->help = (char *)help;

    pthread_mutex_lock(&g_cmd_ctx->list_mutex);
    list_insert_after(&new_cmd->node, &g_cmd_ctx->cmd_list);
    pthread_mutex_unlock(&g_cmd_ctx->list_mutex);

    char target[CMD_FILENAME_MAX_LEN];
    snprintf(target, CMD_FILENAME_MAX_LEN, CMD_LINK_DIR "/%s", new_cmd->str);
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

    pthread_mutex_lock(&g_cmd_ctx->list_mutex);
    list_for_each_safe(node, next, &g_cmd_ctx->cmd_list) {
        cmd = list_entry(node, cmd_t, node);
        if (name != NULL) {
            if (strcmp(cmd->str, name) == 0) {
                char target[CMD_FILENAME_MAX_LEN];
                snprintf(target, CMD_FILENAME_MAX_LEN, CMD_LINK_DIR "/%s", cmd->str);
                if (unlink(target) < 0) {
                    printf("%s(%d): unlink(%s) error: %s\n", __FUNCTION__, __LINE__, target, strerror(errno));
                }
                list_remove(&cmd->node);
                free(cmd);
                break;
            }
        } else {
            char target[CMD_FILENAME_MAX_LEN];
            snprintf(target, CMD_FILENAME_MAX_LEN, CMD_LINK_DIR "/%s", cmd->str);
            if (unlink(target) < 0) {
                printf("%s(%d): unlink(%s) error: %s\n", __FUNCTION__, __LINE__, target, strerror(errno));
            }
            list_remove(&cmd->node);
            free(cmd);
        }
    }
    pthread_mutex_unlock(&g_cmd_ctx->list_mutex);

    return 0;
}
