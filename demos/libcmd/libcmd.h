/******************************************************************************
 * Copyright (C) 2018-2020
 * file:    libcmd.h
 * author:  zucker.chen<timeontheway@163.com>
 * created: 2018-12-10 18:00
 * updated: 2018-12-10 18:00
 ******************************************************************************/
#ifndef _LIBCMD_H_  /* Cannot used "LIBCMD_H" */
#define _LIBCMD_H_

#ifdef __cplusplus
extern "C" {
#endif


#define CMD_LINK_DIR   "./"
#define CMD_LINK_BIN   "./test_libcmdc"

#define MSGQ_KEY            12345678    // server mq key, client mq key instead of getpid()

#define CMD_NAME_MAX_LEN    32      // Max character length of a command string
#define CMD_ARGC_MAX_NUM    8       // Max number of command parameters supported
#define CMD_ARGS_MAX_LEN    16      // Max character length of a command parameter


typedef struct cmd_data {
    char cmd[CMD_NAME_MAX_LEN];                     // cmd name
    int  argc;                                      // cmd number
    char argv[CMD_ARGC_MAX_NUM+1][CMD_ARGS_MAX_LEN];  // cmd parameters content, argv[argc] store ttyname
} cmd_data_t;

typedef int (*cmd_cb_t)(int argc, char **argv);


int cmd_init(void);
int cmd_deinit(void);
int cmd_register(const char *name, cmd_cb_t func);
int cmd_unregister(const char *name);   // name == NULL, unregister all
int cmd_args_proc(int argc, char **argv, cmd_cb_t func);    // client cmd call


#ifdef __cplusplus
}
#endif
#endif  /* _LIBCMD_H_ */
