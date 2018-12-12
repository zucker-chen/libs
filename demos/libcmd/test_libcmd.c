#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include "libcmd.h"


static int do_ls(int argc, char argv[8][16])
{
    printf("argc = %d\n", argc);
    if (argc > 0) {
        printf("argv = %s\n", argv[0]);
    }
    if (argc > 1) {
        printf("argv = %s\n", argv[1]);
    }

    return 0;
}


static void ctrl_c_op(int signo)
{
        cmd_deinit();
    exit(0);
}











int main(int argc, char **argv)
{
    int ret = 0;
    signal(SIGINT, ctrl_c_op);
    ret = cmd_init();
    if (ret < 0) {
        printf("cmd_init failed!\n");
        return -1;
    }
    if (cmd_register("v-ls", do_ls) < 0) {
        return -1;
    }

    printf("cmd main end!\n");
    cmd_args_proc(argc, argv);
    sleep(10);
    cmd_deinit();

    return 0;
}
