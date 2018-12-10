/******************************************************************************
 * Copyright (C) 2018-2020
 * file:    libmsgq.h
 * author:  zucker.chen
 * created: 2018-12-10 18:00
 * updated: 2018-12-10 18:00
 ******************************************************************************/
#ifndef LIBCMD_H
#define LIBCMD_H

//#include "libcmd.h"
//#include "list.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*mq_recv_cb_t)(char *, int);
typedef struct mq_sysv_ctx {
    int msgid_c;
    int msgid_s;
    pthread_t tid;
    mq_recv_cb_t cb;
    char run;
} mq_sysv_ctx_t;




mq_sysv_ctx_t *mq_init_client(int msg_key_s, int msg_key_c, mq_recv_cb_t cb);
void mq_deinit_client(mq_sysv_ctx_t *ctx);
mq_sysv_ctx_t *mq_init_server(int msg_key_s, mq_recv_cb_t cb);
void mq_deinit_server(mq_sysv_ctx_t *ctx);
int mq_send(int msgid, const void *buf, size_t len);
int mq_recv(int msgid, void *buf, size_t len);


#ifdef __cplusplus
}
#endif
#endif
