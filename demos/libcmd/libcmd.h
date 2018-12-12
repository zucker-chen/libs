/******************************************************************************
 * Copyright (C) 2018-2020
 * file:    libcmd.h
 * author:  zucker.chen<timeontheway@163.com>
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

#if 0
#define CMD_NAME_MAX_LEN    32      // 命令字符串最大字符长度
#define CMD_ARGC_MAX_NUM    8       // 命令参数支持最大个数
#define CMD_ARGS_MAX_LEN    16      // 命令参数最大字符长度

typedef struct cmd_data {
    char cmd[CMD_NAME_MAX_LEN];
    int  argc;
    char argv[CMD_ARGC_MAX_NUM][CMD_ARGS_MAX_LEN];
} cmd_data_t;
#endif

typedef int (*cmd_cb_t)(int, char **);
typedef struct cmd cmd_t;

int cmd_init(void);
void cmd_deinit(void);
int cmd_register(const char *name, cmd_cb_t func);
// name == NULL, unregister all
int cmd_unregister(const char *name);


#ifdef __cplusplus
}
#endif
#endif
