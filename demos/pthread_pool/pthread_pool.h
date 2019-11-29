/**
* Copyright (C) 2008 Seapeak.Xu / xvhfeng@gmail.com
*
* FastLib may be copied only under the terms of the GNU General
* Public License V3, which may be found in the FastLib source kit.
* Please visit the FastLib Home Page http://www.csource.org/ for more detail.
**/

#ifndef PTHREAD_POOL_H_
#define PTHREAD_POOL_H_

#define _GNU_SOURCE
#include <sched.h>
#include <pthread.h>


#define TP_TRUE     1
#define TP_FALSE    0
#define TP_INITED   1
#define TP_THREAD_DEFAULT_NAME  "thread_idle"

/*
 * define the callback function type of thread
 */
typedef void (*callback)(void *);

/*
 * define the thread type which in the pool
 * members:
 * 			id : the thread id
 * 			mutex_locker : the  mutext locker
 * 			run_locker : the locker for noticing the thread do running or waitting
 * 			func : the callback function for thread
 * 			arg : the callback parameter
 */
typedef struct thread_info
{
    unsigned char       enable;       // 1: enable, 0: disable
    unsigned char       activate;     // 1: activate, 0: deactivate
    int                 index;        // pthread pool index
	pthread_t           tid;          // pthread id
    pid_t               pid;          // pthread sys pid
	pthread_mutex_t     mutex_locker;
	pthread_cond_t      run_locker;
    int                 policy;       // scheduling policy for thread, SCHED_FIFO/SCHED_RR/SCHED_OTHER
    int                 priority;     // 0-99, The bigger the priority
	callback            func;         // call back function
	void                *arg;         // argument for func
    #define PTHREADPOOL_NAME_LEN 32
    char                name[PTHREADPOOL_NAME_LEN];
}thread_info_t;

/*
 * the structure for the thread pool
 * member:
 * 			list : the initialazed thread list
 * 			mutex_locker : the mutex locker for the thread operation.
 * 			run_locker : the locker for noticing the thread do running or waitting.
 * 			full_locker : the locker notice the thread is stoping when free the thread pool and the pool is not full .
 * 			empty_Locker : the locker notice the thread waitting for the busy thread work over,then do with the thread.
 * 			state : the pool's current state.
 *          total_size : the pool max size;
 *          current_size : the thread count for the current pool ;
 *          current_index : the busy thread in the  pool index.
 */
typedef struct threadpool_info
{
	thread_info_t   **list;
	pthread_mutex_t mutex_locker;
	pthread_cond_t  run_locker;
	pthread_cond_t  full_locker;
	pthread_cond_t  empty_locker;
	int             inited;          // 1: inited, 0:non init
	int             total_size;      // thread numbers
}threadpool_info_t;

#ifdef __cplusplus
extern "C" {
#endif

/*
 * initialize the thread pool
 * parameters:
 * 				size : thread pool max size
 * return:
 * 				0:initialize pool success;
 * 				-1:the size parameter is less 0;
 * 				-2:initialize pool is fail,malloc memory for pool or pool->list is error;
 */
int threadpool_init(int size);

/*
 * run the function with the thread from pool
 * parameter:
 * 				pindex:output thread index
 * 				func:the thread callback function
 * 				arg:the parameter of callback function
 * return:
 * 				0 : success
 * 				-1: the pool is NULL;
 * 				-2 : threadpool not init;
 * 				-3 : threadpool is full;
 * Note:        arg is pointer, so you need copy the arg to local mem as soon as possible.
 */
int threadpool_run(int *pindex, callback func, void *arg, const char *name);

/*
 * free and destroy the thread pool memory
 * return:
 * 				0 : success
 * 				less 0 : fail
 */
int threadpool_destroy();

/*
 * bind thread to cpu id
 * parameters:
 *              index: thread index
 * 				cpu_id : cpu id
 * return:
 * 				0 : success
 * 				less 0 : fail
 */
int threadpool_bind_cpu(int index, int cpu_id);

/*
 * set thread sched priority
 * parameters:
 *              pid: thread id
 *              index: thread index
 * 				policy : scheduling policy for thread, SCHED_FIFO/SCHED_RR/SCHED_OTHER
 *                       default is SCHED_OTHER, priority constant by 0 at SCHED_OTHER
 *                       SCHED_RR(priority = 0) equal SCHED_OTHER
 *              priority: 0-99, The bigger the priority
 * return:
 * 				0 : success
 * 				less 0 : fail
 */
int threadpool_set_pid_sched_priority(pthread_t pid, int policy, int priority);
int threadpool_set_index_sched_priority(int index, int policy, int priority);

/*
 * get thread sched priority
 * parameters:
 *              pid: thread id
 *              index: thread index
 * 				policy : out -> scheduling policy for thread, SCHED_FIFO/SCHED_RR/SCHED_OTHER
 *              priority: out -> 0-99, The bigger the priority
 * return:
 * 				0 : success
 * 				less 0 : fail
 */
int threadpool_get_pid_sched_priority(pthread_t pid, int *policy, int *priority);
int threadpool_get_index_sched_priority(int index, int *policy, int *priority);


/*
 * dump all thread info
 * parameters:
 *              outstr: out -> result pointer
 *              outlen: out -> result len
 * return:
 * 				0 : success
 * 				less 0 : fail
 */
int threadpool_dump_info(char *outstr, int *outlen);



#ifdef __cplusplus
}
#endif

#endif /* PTHREAD_POOL_H_ */
