/******************************************************************************
 * Copyright (C) 2018-2020
 * file:    libmsgq.h
 * author:  zucker.chen
 * created: 2018-12-10 18:00
 * updated: 2018-12-10 18:00
 ******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <pthread.h>
#include "libmsgq.h"

#define MQ_CMD_BIND         (-1)
#define MQ_CMD_UNBIND       (-2)
#define MQ_CMD_QUIT         (-3)


typedef struct {
    int cmd;
    char buf[1];
} mq_msg_t;

static int msg_send(int msgid, int code, const void *buf, int size)
{
    mq_msg_t *msg = (mq_msg_t *)calloc(1, sizeof(mq_msg_t)-1+size);
    if (msg == NULL) {
        printf("malloc mq_msg_t failed\n");
        return -1;
    }
    msg->cmd = code;
    if (size > 0) {
        memcpy((void *)msg->buf, buf, size);
    }
    if (0 != msgsnd(msgid, (const void *)msg, sizeof(mq_msg_t)-1+size, 0)) {
        printf("msgsnd failed, error:%s\n", strerror(errno));
        size = -1;
    }
    free((void *)msg);
    return size;
}

static int msg_recv(int msgid, int *code, void *buf, int len)
{
    int size;
    mq_msg_t *msg = (mq_msg_t *)calloc(1, sizeof(mq_msg_t)-1+len);
    if (msg == NULL) {
        printf("malloc mq_msg_t failed\n");
        return -1;
    }

    if (-1 == (size = msgrcv(msgid, (void *)msg, len+sizeof(mq_msg_t)-1, 0, 0))) {
        printf("msgrcv failed, error:%s\n", strerror(errno));
        goto end;
    }
    *code = msg->cmd;
    size -= sizeof(msg);
    if (size > 0) {
        memcpy(buf, msg->buf, size);
    }
end:
    free((void *)msg);
    return size;
}

static void *server_thread(void *arg)
{
    int cmd, msg_key_c;
    char buf[1024];
    int size;
    mq_sysv_ctx_t *c = (mq_sysv_ctx_t *)arg;
    while (c->run) {
        memset(buf, 0, sizeof(buf));
        size = msg_recv(c->msgid_s, &cmd, buf, sizeof(buf));
        if (size < 0) {
            printf(("msg_recv failed\n"));
            continue;
        }
        switch (cmd) {
        case MQ_CMD_BIND:
            msg_key_c = *(pid_t *)buf;
            c->msgid_c = msgget((key_t)msg_key_c, 0);
            if (c->msgid_c == -1) {
                printf("msgget to open client msgQ failed, error:%s\n", strerror(errno));
                continue;
            }
            if (-1 == msg_send(c->msgid_c, MQ_CMD_BIND,
                        (const void *)&msg_key_c, sizeof(pid_t))) {
                printf("msg_send(MQ_CMD_BIND) failed\n");
                continue;
            }
            break;
        case MQ_CMD_UNBIND:
            if(c->msgid_c == 0) {
                continue;
            }
            c->msgid_c = 0;
            break;
        case MQ_CMD_QUIT:
            break;
        default:
            if (c->cb) {
                c->cb(buf, size);
            } else {
                printf("_mq_recv_cb is NULL!\n");
            }
            break;
        }
    }
    return NULL;
}

static void *client_thread(void *arg)
{
    char buf[1024];
    int size;
    int cmd;
    mq_sysv_ctx_t *c = (mq_sysv_ctx_t *)arg;
    while (c->run) {
        memset(buf, 0, sizeof(buf));
        size = msg_recv(c->msgid_c, &cmd, buf, sizeof(buf));
        if (size == -1) {
            printf("msg_recv failed!\n");
            continue;
        }
        switch (cmd) {
        case MQ_CMD_QUIT:
        case MQ_CMD_BIND:
        case MQ_CMD_UNBIND:
            break;
        default:
            if (c->cb) {
                c->cb(buf, size);
            } else {
                printf("_mq_recv_cb is NULL!\n");
            }
            break;
        }
    }
    return NULL;
}

mq_sysv_ctx_t *mq_init_client(int msg_key_s, int msg_key_c, mq_recv_cb_t cb)
{
    int ret;
    mq_sysv_ctx_t *ctx = calloc(1, sizeof(mq_sysv_ctx_t));
    if (!ctx) {
        printf("malloc failed!\n");
        return NULL;
    }
    ctx->msgid_s = msgget((key_t)msg_key_s, 0);
    if (ctx->msgid_s == -1) {
        printf("ipc server not run, error:%s\n", strerror(errno));
        goto failed;
    }
    if ((ctx->msgid_c = msgget((key_t)msg_key_c, 0)) != -1) {
        printf("%s:%d, Warning: msqkey = %d is exist, will be delete it first.\n", __FUNCTION__, __LINE__, msg_key_c);
        msgctl(ctx->msgid_c, IPC_RMID, NULL);
    }
    ctx->msgid_c = msgget((key_t)msg_key_c, IPC_CREAT|IPC_EXCL|S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP);
    if (ctx->msgid_c == -1) {
        printf("msgget failed, error:%s\n", strerror(errno));
        goto failed;
    }
    if (-1 == msg_send(ctx->msgid_s, MQ_CMD_BIND, (const void *)&msg_key_c, sizeof(int))) {
        printf("msg_send failed, error:%s\n", strerror(errno));
    }

    int code = 0;
    if (-1 == msg_recv(ctx->msgid_c, &code, (void *)&ret, sizeof(pid_t))) {
        printf("msg_recv failed, error:%s\n", strerror(errno));
    }
    ctx->run = true;
    ctx->cb = cb;
    if (0 != pthread_create(&ctx->tid, NULL, client_thread, ctx)) {
        printf("pthread_create failed, error:%s\n", strerror(errno));
        goto failed;
    }

    return ctx;

failed:
    if (ctx->msgid_c != -1) {
        msgctl(ctx->msgid_c, IPC_RMID, NULL);
    }
    free(ctx);
    return NULL;
}

void mq_deinit_client(mq_sysv_ctx_t *ctx)
{
    ctx->run = false;
    msg_send(ctx->msgid_c, MQ_CMD_QUIT, "a", 1);
    pthread_join(ctx->tid, NULL);
    msg_send(ctx->msgid_s, MQ_CMD_UNBIND, (const void *)&ctx->run, sizeof(pid_t));  // ctx->run can be anything
    msgctl(ctx->msgid_c, IPC_RMID, NULL);
    free(ctx);
}

mq_sysv_ctx_t *mq_init_server(int msg_key_s, mq_recv_cb_t cb)
{
    mq_sysv_ctx_t *ctx = calloc(1, sizeof(mq_sysv_ctx_t));
    if (!ctx) {
        printf("malloc failed!\n");
        return NULL;
    }
    if ((ctx->msgid_s = msgget((key_t)msg_key_s, 0)) != -1) {
        printf("%s:%d, Warning: msqkey = %d is exist, will be delete it first.\n", __FUNCTION__, __LINE__, msg_key_s);
        msgctl(ctx->msgid_s, IPC_RMID, NULL);
    }
    ctx->msgid_s = msgget((key_t)msg_key_s, IPC_CREAT|IPC_EXCL|S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP);
    if (ctx->msgid_s == -1) {
        printf("msgget failed: error:%s\n", strerror(errno));
        msgctl(ctx->msgid_s, IPC_RMID, NULL);
        goto failed;
    }
    ctx->run = true;
    ctx->cb = cb;
    if (0 != pthread_create(&ctx->tid, NULL, server_thread, ctx)) {
        printf("pthread_create failed!\n");
        goto failed;
    }
    return ctx;

failed:
    printf("init server failed!\n");
    if (ctx->msgid_s != -1) {
        msgctl(ctx->msgid_s, IPC_RMID, NULL);
    }
    free(ctx);
    return NULL;
}

void mq_deinit_server(mq_sysv_ctx_t *ctx)
{
    ctx->run = false;
    msg_send(ctx->msgid_s, MQ_CMD_QUIT, "a", 1);
    pthread_join(ctx->tid, NULL);
    msgctl(ctx->msgid_s, IPC_RMID, NULL);
    free(ctx);
}

int mq_send(int msgid, const void *buf, size_t len)
{
    int ret = msg_send(msgid, 1, buf, len);
    if (ret == -1) {
        printf("msg_send failed\n");
        return -1;
    }
    return len;
}

int mq_recv(int msgid, void *buf, size_t len)
{
    int code;
    int ret = msg_recv(msgid, &code, buf, len);
    if (ret == -1) {
        printf("msgrcv failed\n");
    }
    return ret;
}

