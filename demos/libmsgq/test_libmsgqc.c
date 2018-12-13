#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include "libmsgq.h"


#define MSGQ_KEY1            12345678
#define MSGQ_KEY2            12345679


int msg_recv_cb(char *buf, int size)
{
    printf("%s(%d): buf = %s, size = %d\n", __FUNCTION__, __LINE__, buf, size);
    
    
    return 0;
}


int main(int argc, char **argv)
{
    struct mq_sysv_ctx *ctx = NULL;
    ctx = mq_init_client(MSGQ_KEY1, MSGQ_KEY2, msg_recv_cb);
    
    mq_send(ctx->msgid_s, "hello", 6);

    mq_deinit_client(ctx);

    return 0;
}
