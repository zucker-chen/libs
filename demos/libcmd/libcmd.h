/******************************************************************************
 * Copyright (C) 2018-2020
 * file:    libcmd.h
 * author:  zucker.chen<timeontheway@163.com>
 * created: 2018-12-10 18:00
 * updated: 2018-12-10 18:00
 ******************************************************************************/
#ifndef _LIBCMD_H_  /* Cannot used "LIBCMD_H" */
#define _LIBCMD_H_

#include "list.h"
#include "libmsgq.h"

#ifdef __cplusplus
extern "C" {
#endif


#define CMD_LINK_DIR   "./"     // like: /bin/v-cmd-show-all

#define CMD_FILENAME_MAX_LEN    128     // Max character length of file name full path
#define CMD_NAME_MAX_LEN        32      // Max character length of a command string
#define CMD_ARGC_MAX_NUM        8       // Max number of command parameters supported
#define CMD_ARGS_MAX_LEN        16      // Max character length of a command parameter


typedef struct list_head list_head_t;
typedef int (*cmd_cb_t)(int argc, char **argv, char *ack);


// server cmd data info
typedef struct cmd {
    list_head_t node;               // list node
    char str[CMD_NAME_MAX_LEN];     // cmd name
    cmd_cb_t func;                  // cmd callback point
    char *help;
} cmd_t;

// client cmd data info
typedef struct cmd_data {
    char cmd[CMD_NAME_MAX_LEN];                         // cmd name
    int  argc;                                          // cmd number
    char argv[CMD_ARGC_MAX_NUM+1][CMD_ARGS_MAX_LEN];    // cmd parameters content, argv[argc] store ttyname
} cmd_data_t;

typedef struct cmd_ctx {
    int mq_key;                                     // server mq key, client mq key instead of getpid()
    char client_dist_file[CMD_FILENAME_MAX_LEN];    // cmd client file name(full path)
    list_head_t cmd_list;
    mq_sysv_ctx_t *mq_ctx;
    char tty_name[CMD_NAME_MAX_LEN];                // cmd server tty name
} cmd_ctx_t;



int cmd_init(int mq_key, char *filename);
int cmd_deinit(void);
int cmd_register(const char *name, cmd_cb_t func, const char *help);
int cmd_unregister(const char *name);   // name == NULL, unregister all

int cmd_args_proc(int mq_key, int argc, char **argv, cmd_cb_t func);    // mq_key, cmd server key


#ifdef __cplusplus
}
#endif
#endif  /* _LIBCMD_H_ */
