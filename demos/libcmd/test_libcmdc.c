#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include "libcmd.h"



/*
 *   note: "char **buf", it should be used as "char *buf", just for common cmd_cb_t(callback) define
 */
static int cmd_ack(int size, char **buf)
{
    //printf("%s(%d): sizeof(cmd_data_t) = %d, data size = %d\n", __FUNCTION__, __LINE__, (int)sizeof(cmd_data_t), size);
    printf("%s\n", (char *)buf);
    
    return 0;
}


int main(int argc, char **argv)
{
    if (cmd_args_proc(argc, argv, cmd_ack) < 0) {
        return -1;
    }

    return 0;
}
