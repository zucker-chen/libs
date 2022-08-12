/******************************************************************************
 * Copyright (C) 2018-2020
 * file:    libmsgq.h
 * author:  zucker.chen
 * created: 2018-12-10 18:00
 * updated: 2018-12-10 18:00
 ******************************************************************************/
#ifndef LIBCMD_H
#define LIBCMD_H


#ifdef __cplusplus
extern "C" {
#endif

#define MQ_MAX_BUF_LEN  (8176)    // (<=msgmax-16) Max bufer length when read and write MQ. "cat /proc/sys/kernel/msgmax" = 8192

typedef struct mq_sysv_ctx mq_handle_t;
typedef int (*mq_recv_cb_t)(mq_handle_t *handle, char *data, int size);


mq_handle_t *mq_init_server(int msg_key, mq_recv_cb_t cb);
void mq_deinit_server(mq_handle_t *handle);

/* you need receive msg manual if cb is NULL */
mq_handle_t *mq_init_client(int msg_key, mq_recv_cb_t cb);
void mq_deinit_client(mq_handle_t *handle);

int mq_send(mq_handle_t *handle, const void *buf, int len);
/* len = input buf size, return recive size */
int mq_recv(mq_handle_t *handle, void *buf, int len);


#ifdef __cplusplus
}
#endif
#endif
