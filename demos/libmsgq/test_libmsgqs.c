#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include "libmsgq.h"


#define MSGQ_KEY            12345678


static mq_sysv_ctx_t *ctx = NULL;


int msg_recv_cb(char *buf, int size)
{
    printf("%s(%d): buf = %s, size = %d\n", __FUNCTION__, __LINE__, buf, size);
    
    
    return 0;
}

static void ctrl_c_op(int signo)
{
    printf("%s:%d\n", __FUNCTION__, __LINE__);
    mq_deinit_server(ctx);
    exit(0);
}

int main(int argc, char **argv)
{
    signal(SIGINT, ctrl_c_op);

    ctx = mq_init_server(MSGQ_KEY, msg_recv_cb);
    
    sleep(100000);

    return 0;
}
