#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <libgen.h>
#include "libcmd.h"


static int do_ls(int argc, char **argv, char *ack)
{
    printf("%s:%d\n", __FUNCTION__, __LINE__);
    int i = 0, len = 0;
    char (*args)[CMD_ARGS_MAX_LEN] = (char (*)[CMD_ARGS_MAX_LEN])argv;

    for (i = 0; i < argc; i++) {
        len += sprintf(ack + len, "%s:%d args[%d] = %s\n", __FUNCTION__, __LINE__, i, args[i]);
    }
    
    return 0;
}

static int do_sleep(int argc, char **argv, char *ack)
{
    printf("%s:%d\n", __FUNCTION__, __LINE__);
	char (*args)[CMD_ARGS_MAX_LEN] = (char (*)[CMD_ARGS_MAX_LEN])argv;

	sleep(atoi(args[0]));
    
    return 0;
}


static void signal_handle(int signo)
{
    printf("%s:%d\n", __FUNCTION__, __LINE__);
    cmd_deinit();
    exit(0);
}





int main(int argc, char **argv)
{
    signal(SIGINT, signal_handle);
 
	// libcmd 模块处理
	char procname[128] = {0};
	int	cmd_key = 0x12345678;
	if (-1 == readlink("/proc/self/exe", procname, sizeof(procname))) {
		printf("%s:%d, readlink error.\n", __FUNCTION__, __LINE__);
		return -1;
	}
	if (0 == strcmp(basename(procname), basename(argv[0]))) {
		printf("%s:%d, procname find it.\n", __FUNCTION__, __LINE__);
		int ret = 0;
		ret = cmd_init(cmd_key, procname);
		if (ret < 0) {
			printf("cmd_init failed!\n");
			return -1;
		}
		cmd_register("v-ls", do_ls, "show all arguments for the cmd");
		cmd_register("v-sleep", do_sleep, "show all arguments for the cmd");
	} else {
		printf("%s:%d, procname not find it, %s ? %s.\n", __FUNCTION__, __LINE__, procname, basename(argv[0]));
		return cmd_args_proc(cmd_key, argc, argv, NULL);
	}

	
    while(1) {
        // printf(". ");fflush(stdout);     // for test
        sleep(1);
    }
    
    return 0;
}
