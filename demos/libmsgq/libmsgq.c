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
#include <time.h>
#include <pthread.h>
#include "libmsgq.h"

#define MQ_CMD_BIND         (10)
#define MQ_CMD_UNBIND       (MQ_CMD_BIND+1)
#define MQ_CMD_DATA         (MQ_CMD_UNBIND+1)


/* 消息队列的头 */
typedef struct {
    long cmd;
    char buf[1];
} mq_msg_t;

/* 会话数据的头 */
typedef struct {
    int session_cmd;
    char session_data[0];
} mq_data_t;


// timeout: ms
static int msg_send(int msgid, int msg_type, const void *buf, int size, int timeout)
{
    char data[MQ_MAX_BUF_LEN+16] = {0};
    mq_msg_t *msg = (mq_msg_t *)&data[0];
	int count = 0;
	
    msg->cmd = (long)msg_type;
    if (size > 0) {
        memcpy((void *)&msg->buf[0], buf, size);
    }
	
	if (timeout == 0) {
		if (0 != msgsnd(msgid, (const void *)msg, sizeof(msg->cmd)+size, 0)) {
			printf("%s:%d msgsnd failed, error(%d, %d):%s\n", __FUNCTION__, __LINE__, msg_type, errno, strerror(errno));
			return -1;
		}
		return 0;
	} else {
		count = timeout / 100;
		do
		{
			if (0 != msgsnd(msgid, (const void *)msg, sizeof(msg->cmd)+size, IPC_NOWAIT)) {
				// printf("%s:%d msgsnd failed, error(%d, %d):%s\n", __FUNCTION__, __LINE__, msg_type, errno, strerror(errno));
				usleep(100*1000);
			} else {
				return 0;
			}
		} while (count-- > 0);
	}

    return -1;
}

// timeout: ms
static int msg_recv(int msgid, int msg_type, void *buf, int len, int timeout)
{
    char data[MQ_MAX_BUF_LEN+16] = {0};
    mq_msg_t *msg = (mq_msg_t *)&data[0];
    int size = 0;
	int count = 0;

    msg->cmd = (long)msg_type;

	if (timeout == 0) {
		if (-1 == (size = msgrcv(msgid, (void *)msg, sizeof(msg->cmd)+len, msg_type, 0))) {
			printf("%s:%d msgrcv failed, error:%s\n", __FUNCTION__, __LINE__, strerror(errno));
			return -1;
		}
	} else {
		count = timeout / 100;
		do {
			usleep(100*1000);
			size = msgrcv(msgid, (void *)msg, sizeof(msg->cmd)+len, msg_type, IPC_NOWAIT);
			if (size > 0) {
				break;
			}
		} while (count-- > 0);
	}

    size -= sizeof(msg->cmd);
    if (size > 0 && size <= len) {
        memcpy(buf, &msg->buf[0], size);
		return size;
    } else {
		if (size > 0) {
			printf("%s:%d error revc size = %d > len = %d.\n", __FUNCTION__, __LINE__, size, len);
		}
		return -1;
	}
}


static void *mq_session_thread(void *arg)
{
	pthread_detach(pthread_self());
    mq_sysv_ctx_t tmp_ctx = {0,};
	mq_sysv_ctx_t *ctx = &tmp_ctx;
    char buf[MQ_MAX_BUF_LEN];
	mq_data_t *mq_data = NULL;
	int size = 0;

	/* 第一时间copy, 防止外部被篡改 */
	memcpy((void *)ctx, (void *)arg, sizeof(mq_sysv_ctx_t));
	
    while (ctx->run) {
        memset(buf, 0, sizeof(buf));
        size = msg_recv(ctx->msg_id, ctx->type_rx, buf, sizeof(buf), 100);
        if (size < 0) {
			//printf("%s:%d msg_recv failed!\n", __FUNCTION__, __LINE__);
            continue;
        }
		
		mq_data = (mq_data_t *)buf;
		if (mq_data->session_cmd == MQ_CMD_UNBIND) {
			ctx->run = false;
			break;
		}

		if (mq_data->session_cmd == MQ_CMD_DATA && ctx->cb) {
			ctx->cb(ctx, mq_data->session_data, size-sizeof(mq_data_t));
		}
    }
	printf("%s:%d exit msg type = %d.\n", __FUNCTION__, __LINE__, ctx->type_tx);
	
    return NULL;
}


static void *mq_server_thread(void *arg)
{
	pthread_detach(pthread_self());
    char buf[MQ_MAX_BUF_LEN];
    mq_sysv_ctx_t *ctx = (mq_sysv_ctx_t *)arg;
	mq_sysv_ctx_t bind_ctx = {0,};
	struct timespec time_now = {0, 0};
	pthread_t tid;
	int ret = -1;
	
    while (ctx->run) {
        memset(buf, 0, sizeof(buf));
        ret = msg_recv(ctx->msg_id, MQ_CMD_BIND, buf, sizeof(buf), 100);
        if (ret < 0) {
			//printf("%s:%d msg_recv failed\n", __FUNCTION__, __LINE__);
            continue;
        }

		ctx->type_tx = *(int *)&buf[0];
		
		/* 对会话BIND的状态回复 */
		clock_gettime(CLOCK_REALTIME, &time_now); 
		ctx->type_rx = (ctx->type_tx & (0xffff<<16)) | ((int)(time_now.tv_nsec/1000) & 0xffff);		/* 客户端进程ID与毫秒时间组合，保证唯一性 */
		ret = msg_send(ctx->msg_id, ctx->type_tx, (const void *)&ctx->type_rx, sizeof(int), 0);
		if (ret < 0) {
			printf("%s:%d msg_send(MQ_CMD_BIND) failed\n", __FUNCTION__, __LINE__);
			return NULL;
		}
		
		memcpy((void *)&bind_ctx, ctx, sizeof(mq_sysv_ctx_t));
		if (0 != pthread_create(&tid, NULL, mq_session_thread, &bind_ctx)) {
			printf("pthread_create failed!\n");
		}
    }
	printf("%s:%d exit.\n", __FUNCTION__, __LINE__);
	
    return NULL;
}

static void *mq_client_thread(void *arg)
{
	pthread_detach(pthread_self());
    char buf[MQ_MAX_BUF_LEN];
    int size;
    mq_sysv_ctx_t *ctx = (mq_sysv_ctx_t *)arg;
    while (ctx->run) {
        memset(buf, 0, sizeof(buf));
        size = msg_recv(ctx->msg_id, ctx->type_rx, buf, sizeof(buf), 100);
		size -= sizeof(mq_data_t);
        if (size < 0) {
			//printf("%s:%d msg_recv failed!\n", __FUNCTION__, __LINE__);
            continue;
        }
		if (ctx->cb) {
			ctx->cb(ctx, &buf[sizeof(mq_data_t)], size);
		} else {
			printf("%s:%d mq_recv_cb is NULL!\n", __FUNCTION__, __LINE__);
		}
    }
	printf("%s:%d exit.\n", __FUNCTION__, __LINE__);
	
    return NULL;
}

// you need receive msg manual if cb is NULL.
mq_sysv_ctx_t *mq_init_client(int msg_key, mq_recv_cb_t cb)
{
	pthread_t tid;
	struct timespec time_now = {0, 0};
    mq_sysv_ctx_t *ctx = calloc(1, sizeof(mq_sysv_ctx_t));
    if (!ctx) {
        printf("%s:%d malloc failed!\n", __FUNCTION__, __LINE__);
        return NULL;
    }
	
    ctx->msg_id = msgget((key_t)msg_key, 0);
    if (ctx->msg_id == -1) {
        printf("%s:%d ipc server not run, error:%s\n", __FUNCTION__, __LINE__, strerror(errno));
		free(ctx);
		return NULL;
    }
	clock_gettime(CLOCK_REALTIME, &time_now); 
	ctx->type_rx = ((int)getpid() << 16) | ((int)(time_now.tv_nsec/1000) & 0xffff);		/* 客户端进程ID与毫秒时间组合，保证唯一性 */
    if (-1 == msg_send(ctx->msg_id, MQ_CMD_BIND, (const void *)&ctx->type_rx, sizeof(int), 0)) {
        printf("%s:%d msg_send failed, error:%s\n", __FUNCTION__, __LINE__, strerror(errno));
    }

	/* ctx->type_tx 是从服务端获取的msg type唯一标识符，作为客户端发送用 */
    if (-1 == msg_recv(ctx->msg_id, ctx->type_rx, (void *)&ctx->type_tx, sizeof(int), 0)) {
        printf("%s:%d msg_recv failed, error:%s\n", __FUNCTION__, __LINE__, strerror(errno));
		free(ctx);
		return NULL;
    }
	printf("%s:%d mq bind success, type_tx = %d, type_rx = %d\n", __FUNCTION__, __LINE__, ctx->type_tx, ctx->type_rx);
    
    if (cb != NULL) {
        ctx->run = true;
        ctx->cb = cb;
        if (0 != pthread_create(&tid, NULL, mq_client_thread, ctx)) {
            printf("%s:%d pthread_create failed, error:%s\n", __FUNCTION__, __LINE__, strerror(errno));
			free(ctx);
			return NULL;
        }
    }

    return ctx;
}

void mq_deinit_client(mq_handle_t *ctx)
{
	int status = 0;

	if (ctx == NULL) {
		printf("%s:%d error, ctx == NULL\n", __FUNCTION__, __LINE__);
		return;
	}
    ctx->run = false;
	usleep(200*1000);
	/* 会话的buf第一int当指令用，比如MQ_CMD_UNBIND代表会话结束 */
	status = MQ_CMD_UNBIND;
    msg_send(ctx->msg_id, ctx->type_tx, (const void *)&status, sizeof(int), 0);
	
    free(ctx);
}

mq_sysv_ctx_t *mq_init_server(int msg_key, mq_recv_cb_t cb)
{
	pthread_t tid;
    mq_sysv_ctx_t *ctx = calloc(1, sizeof(mq_sysv_ctx_t));
    if (!ctx) {
        printf("%s:%d malloc failed!\n", __FUNCTION__, __LINE__);
        return NULL;
    }
    if ((ctx->msg_id = msgget((key_t)msg_key, 0)) != -1) {
        printf("%s(%d): Warning: msqkey = %d is exist, will be delete it first.\n", __FUNCTION__, __LINE__, msg_key);
        msgctl(ctx->msg_id, IPC_RMID, NULL);
    }
    ctx->msg_id = msgget((key_t)msg_key, IPC_CREAT|IPC_EXCL|S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP);
    if (ctx->msg_id == -1) {
        printf("%s:%d msgget failed: error:%s\n", __FUNCTION__, __LINE__, strerror(errno));
        msgctl(ctx->msg_id, IPC_RMID, NULL);
        goto failed;
    }
    ctx->run = true;
    ctx->cb = cb;
    if (0 != pthread_create(&tid, NULL, mq_server_thread, ctx)) {
        printf("%s:%d pthread_create failed!\n", __FUNCTION__, __LINE__);
        goto failed;
    }
	printf("%s:%d msgid = %d\n", __FUNCTION__, __LINE__, ctx->msg_id);
    return ctx;

failed:
    printf("init server failed!\n");
    if (ctx->msg_id > 0) {
        msgctl(ctx->msg_id, IPC_RMID, NULL);
    }
    free(ctx);
    return NULL;
}

void mq_deinit_server(mq_sysv_ctx_t *ctx)
{
	if (ctx == NULL) {
		printf("%s:%d ctx == NULL\n", __FUNCTION__, __LINE__);
		return;
	}
    ctx->run = false;
	usleep(200*1000);
    msgctl(ctx->msg_id, IPC_RMID, NULL);
    free(ctx);
}

int mq_send(mq_sysv_ctx_t *ctx, const void *buf, int len)
{
	mq_data_t *mq_data = NULL;
    char mq_buf[MQ_MAX_BUF_LEN];
    int ret = 0;

	if (ctx == NULL) {
		printf("%s:%d error, ctx == NULL\n", __FUNCTION__, __LINE__);
		return -1;
	}
	
	mq_data = (mq_data_t *)&mq_buf[0];
	mq_data->session_cmd = MQ_CMD_DATA;
	memcpy(&mq_data->session_data, buf, len);
	ret = msg_send(ctx->msg_id, ctx->type_tx, mq_data, sizeof(mq_data_t)+len, 0);
    if (ret == -1) {
        printf("%s:%d msg_send failed\n", __FUNCTION__, __LINE__);
        return -1;
    }
    return len;
}

int mq_recv(mq_sysv_ctx_t *ctx, void *buf, int len)
{
	mq_data_t *mq_data = NULL;
    char mq_buf[MQ_MAX_BUF_LEN];
	int size = 0;

	if (ctx == NULL) {
		printf("%s:%d error, ctx == NULL\n", __FUNCTION__, __LINE__);
		return -1;
	}
	
	size = msg_recv(ctx->msg_id, ctx->type_rx, mq_buf, MQ_MAX_BUF_LEN, 0);
	size -= sizeof(mq_data_t);
    if (size <= 0) {
        printf("%s:%d msgrcv failed, size = %d\n", __FUNCTION__, __LINE__, size);
		return -1;
    }
	mq_data = (mq_data_t *)&mq_buf[0];
	if (mq_data->session_cmd != MQ_CMD_DATA) {
        printf("%s:%d mq recv error, session cmd = %d\n", __FUNCTION__, __LINE__, mq_data->session_cmd);
		return -1;
	}
	if (size > len) {
        printf("%s:%d recive size of out range %d -> %d\n", __FUNCTION__, __LINE__, size, len);
		return -1;
	}
	memcpy(buf, &mq_data->session_data, size);
	
    return size;
}

