#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include "libcmd.h"



/*
 *   note: "char **buf", it should be used as "char *buf", just for common cmd_cb_t(callback) define
 */
static int cmd_ack(int size, char **buf, char *ack)
{
    //printf("%s(%d): sizeof(cmd_data_t) = %d, data size = %d\n", __FUNCTION__, __LINE__, (int)sizeof(cmd_data_t), size);
    printf("%s:%d %s\n", __FUNCTION__, __LINE__, (char *)buf);
    
    return 0;
}


int main(int argc, char **argv)
{
    #if 0   // test
	int i = 0;
	for (i = 0; i < argc; i++) {
		printf("argv[%d] = %s\n", i, argv[i]);
	}
    #endif
    
    if (cmd_args_proc(argc, argv, cmd_ack) < 0) {
        return -1;
    }

    return 0;
}
