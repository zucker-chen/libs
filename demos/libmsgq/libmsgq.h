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

#define MQ_MAX_BUF_LEN  1024    // Max bufer length when read and write MQ.

typedef int (*mq_recv_cb_t)(char *buf, int size);
typedef struct mq_sysv_ctx {
    int msgid_c;    // used for client(ack cmd result)
    int msgid_s;    // used for server(input parse and cb access)
    pthread_t tid;
    mq_recv_cb_t cb;
    char run;
} mq_sysv_ctx_t;


mq_sysv_ctx_t *mq_init_server(int msg_key_s, mq_recv_cb_t cb);  // you need receive msg manual if cb is NULL.
void mq_deinit_server(mq_sysv_ctx_t *ctx);

mq_sysv_ctx_t *mq_init_client(int msg_key_s, int msg_key_c, mq_recv_cb_t cb);   // we suggest that msg_key_c be replaced by getpid()
void mq_deinit_client(mq_sysv_ctx_t *ctx);

int mq_send(int msgid, const void *buf, size_t len);
int mq_recv(int msgid, void *buf, size_t len);


#ifdef __cplusplus
}
#endif
#endif
