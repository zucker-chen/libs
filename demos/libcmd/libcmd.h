/******************************************************************************
 * Copyright (C) 2014-2016
 * file:    libcmd.h
 * author:  gozfree <gozfree@163.com>
 * created: 2016-09-16 18:00
 * updated: 2016-09-16 18:00
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
void cmd_deinit(cmd_t *cmd);
int cmd_register(cmd_t *cmd, const char *name, cmd_cb_t func);
int cmd_execute(cmd_t *cmd, const char *name);

#ifdef __cplusplus
}
#endif
#endif
