/**
* Copyright (C) 2008 Seapeak.Xu / xvhfeng@gmail.com
*
* FastLib may be copied only under the terms of the GNU General
* Public License V3, which may be found in the FastLib source kit.
* Please visit the FastLib Home Page http://www.csource.org/ for more detail.
**/

#include "pthread_pool.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <errno.h>
#include <signal.h>


/*
 *the thread pool
 */
 // global varalibale declared
static threadpool_info_t *pool;

/*
 * the thread callback function proxy
 * parameters:
 * 				arg:the thread callback function parameter
 */
static void *callback_proxy(void *arg);


// proxy the thread running, use pthread_cond_wait to wait for arg to be update by
// other users that need to use this thread
static void *callback_proxy(void *arg)
{
	thread_info_t *thread = (thread_info_t *)arg;
    thread->pid = syscall(SYS_gettid);
    
	while(TP_TRUE == thread->enable)
	{
        thread->activate = TP_FALSE;
        prctl(PR_SET_NAME, thread->name);
        
		pthread_mutex_lock(&thread->mutex_locker);
        pthread_cond_wait(&thread->run_locker, &thread->mutex_locker);
        pthread_mutex_unlock(&thread->mutex_locker);
       
        if (TP_TRUE == thread->activate && NULL != thread->func) {
            prctl(PR_SET_NAME, thread->name);
            thread->func(thread->arg);
            
            pthread_mutex_lock(&thread->mutex_locker);
            strcpy(thread->name, TP_THREAD_DEFAULT_NAME);
            pthread_mutex_unlock(&thread->mutex_locker);
        }
	}

    pthread_mutex_lock(&thread->mutex_locker);
    thread->enable = TP_FALSE;  // pthread exit.
    thread->activate = TP_FALSE;
    pthread_mutex_unlock(&thread->mutex_locker);
    printf("Warnning: callback_proxy thread[%d] is exited.\n", thread->index);
    
	return NULL;
}


// initialize a thread pool of [size] for later use
int threadpool_init(int size)
{
    int index = 0;
    thread_info_t *thread = NULL;
    pthread_attr_t attr;
    
	if (0 >= size) {
		return -1;
	}

	pool = (threadpool_info_t *) malloc(sizeof(threadpool_info_t));
	if (NULL == pool) {
		return -2;
	}
	memset(pool, 0, sizeof(threadpool_info_t));
	pool->inited = TP_INITED;
	pool->total_size = size;
    
	// initialize sync data structures
	pthread_mutex_init(&pool->mutex_locker, NULL);
	pthread_cond_init(&pool->run_locker, NULL);
    
	// initialize a list of thread_info_t structs
	pool->list = (thread_info_t **) malloc(sizeof(thread_info_t *) * size);
	if (NULL == pool->list) {
		pthread_cond_destroy(&pool->run_locker);
		pthread_mutex_destroy(&pool->mutex_locker);
		// free the memory pointed by pool pointer
		free(pool);
		return -2;
	}

    pthread_attr_init(&attr);
    for (index = 0; index < size; index++)
    {
        pool->list[index] = (thread_info_t *) malloc(sizeof(thread_info_t));
        if (NULL == pool->list[index]) {
            printf("ERR: threadpool_init malloc error.\n");
        }
        
        thread = pool->list[index];
        memset(thread, 0, sizeof(thread_info_t));
        pthread_mutex_init(&thread->mutex_locker, NULL);
        pthread_cond_init(&thread->run_locker, NULL);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        thread->enable = TP_TRUE;
        strcpy(thread->name, TP_THREAD_DEFAULT_NAME);
        if(0 > pthread_create(&thread->tid, &attr, callback_proxy, thread)) {
            printf("Warnning: pool pthread_create error, index = %d\n", index);
            continue;
        }
        //threadpool_set_index_sched_priority(index, SCHED_RR, 22);
    }
    usleep(5000);   // 5ms, Wait for all thread already.
    
	pool->inited = TP_INITED;
	return 0;
}

// run the callback within the thread pool, arg is its var
int threadpool_run(int *pindex, callback func, void *arg, const char *name)
{
    int index = 0;
    thread_info_t *thread = NULL;
    
	if (NULL == pool) {
		return -1;
	}
    
    if (TP_INITED != pool->inited) { //the pool cannot use
		return -2;
    }

    pthread_mutex_lock(&pool->mutex_locker);
    for (index = 0; index < pool->total_size; index++)
    {
        thread = pool->list[index];
        if (TP_TRUE == thread->enable && TP_FALSE == thread->activate) {
			thread->arg = arg;
			thread->func = func;
			thread->index = index;
            if (NULL != name) {
                strncpy(thread->name, name, sizeof(thread->name));
            } else {
                thread->name[0] = '\0';
            }
            if (NULL != pindex) {
                *pindex = index;
            }
            thread->activate = TP_TRUE;
			pthread_cond_signal(&thread->run_locker) ;
            break;
        }
    }
	pthread_mutex_unlock(&pool->mutex_locker);
    
    if (index >= pool->total_size) {
        printf("ERR: threadpool is full, pls try again later.\n");
        return -3;
    }

	return 0;
}

// destory the thread pool
int threadpool_destroy()
{
    int index = 0, ret = 0;
    thread_info_t *thread = NULL;
    
    if(NULL == pool) return 0;

    pthread_mutex_lock(&pool->mutex_locker);
    for (index = 0; index < pool->total_size; index++)
    {
        thread = pool->list[index];
        ret = pthread_kill(thread->tid, 0);
        if (ret != ESRCH && ret != EINVAL) {
            // thread is alive.
            pthread_cancel(thread->tid);
        }
            
        pthread_cond_destroy(&thread->run_locker );
        pthread_mutex_destroy(&thread->mutex_locker);
        free(thread);
        thread = NULL;
        pool->list[index] = NULL;
    }
	pthread_mutex_unlock(&pool->mutex_locker);

    pthread_mutex_destroy(&pool->mutex_locker);
    pthread_cond_destroy(&pool->run_locker);
    pthread_cond_destroy(&pool->full_locker);
    pthread_cond_destroy(&pool->empty_locker);

    free(pool->list);
    pool->list = NULL;
    free( pool);
    pool = NULL;

    return 0;
}


// bind thread to cpu
int threadpool_bind_cpu(int index, int cpu_id)
{
	int ret;
	cpu_set_t mask;
    thread_info_t *thread = NULL;

	CPU_ZERO(&mask);
	CPU_SET(cpu_id, &mask);
	
	if (0 > index || pool->total_size <= index) {  //索引号出错 
		printf("ERR: threadpool_set_bind_cpu params invalid\n");
		return -1;
	}
	
    thread = pool->list[index];
	ret = pthread_setaffinity_np(thread->tid, sizeof(mask), &mask);
	if (0 > ret) {
		printf("ERR: threadpool_set_bind_cpu pthread_setaffinity_np: ret = %d\n", ret);
		return -1;
	}
    
    return 0;
}

// set thread priority, pid is pthread id
int threadpool_set_pid_sched_priority(pthread_t pid, int policy, int priority)
{
	int ret;
	struct sched_param param;
    
	if (!((SCHED_OTHER == policy) || (SCHED_RR == policy) || (SCHED_FIFO == policy))) {
		printf("ERR: policy = %d error.\n", policy);
		return -1;
	}

	if ((0 > priority) || (99 < priority)) {
		printf("ERR: priority = %d error.\n", priority);
		return -1;
	}

	param.sched_priority = priority;	
    pid = pid <= 0 ? pthread_self() : pid;
	ret = pthread_setschedparam(pid, policy, &param);
	if (0 > ret) {
		printf("ERR: pthread_setschedparam error. ret = %d\n", ret);
		return -1;
	}
    
    return 0;
}

int threadpool_set_index_sched_priority(int index, int policy, int priority)
{
	int ret;
    thread_info_t *thread = NULL;
	
	if (0 > index || pool->total_size <= index) {  //索引号出错 
		printf("ERR: threadpool_set_index_sched_priority params invalid\n");
		return -1;
	}

    thread = pool->list[index];
	ret = threadpool_set_pid_sched_priority(thread->tid, policy, priority);
	if (0 > ret) {
		printf("ERR: threadpool_set_pid_sched_priority error. ret = %d\n", ret);
		return -1;
	}

    pthread_mutex_lock(&thread->mutex_locker);
    thread->policy = policy;
    thread->priority = priority;
    pthread_mutex_unlock(&thread->mutex_locker);
    
    return 0;
}

// get and show thread priority
int threadpool_get_pid_sched_priority(pthread_t pid, int *policy, int *priority)
{
	int ret;
	struct sched_param param;
    
    if (NULL == policy || NULL == priority) {
		printf("ERR: threadpool_get_pid_sched_priority error. NULL == policy || NULL == priority\n");
        return -1;
    }
    
    pid = pid <= 0 ? pthread_self() : pid;
	ret = pthread_getschedparam(pid, policy, &param);
	if (0 > ret) {
		printf("ERR: pthread_getschedparam error. ret = %d\n", ret);
		return -1;
	}
    
    *priority = param.sched_priority;
    #if 0
    printf("pid = %lu, policy=%s, priority=%d\n", pid, 
            (*policy == SCHED_FIFO)  ? "SCHED_FIFO" :
            (*policy == SCHED_RR)    ? "SCHED_RR" :
            (*policy == SCHED_OTHER) ? "SCHED_OTHER" :
            "???", param.sched_priority);
    #endif        

    return 0;
}

int threadpool_get_index_sched_priority(int index, int *policy, int *priority)
{
    thread_info_t *thread = NULL;
	
	if (0 > index || pool->total_size <= index) {  //索引号出错 
		printf("ERR: threadpool_get_index_sched_priority params invalid\n");
		return -1;
	}
    
    thread = pool->list[index];
    return threadpool_get_pid_sched_priority(thread->tid, policy, priority);
}

int threadpool_dump_info(char *outstr, int *outlen)
{
    int len = 0, index = 0;
    thread_info_t *thread = NULL;
    
    if (NULL == outstr) {
		printf("ERR: threadpool_dump_info error. NULL == outstr\n");
        return -1;
    }
    
    // tile
    len += sprintf(outstr + len, "\t%-8s %-24s %-12s %-12s %-12s\n",
                    "index", "name", "thread pid", "policy", "priority");

    pthread_mutex_lock(&pool->mutex_locker);
    for (index = 0; index < pool->total_size; index++)
    {
        thread = pool->list[index];
        
        pthread_mutex_lock(&thread->mutex_locker);
        threadpool_get_index_sched_priority(index, &thread->policy, &thread->priority);
        len += sprintf(outstr + len,"\t%-8d %-24s %-12d %-12s %-12d\n",
                        index, thread->name, thread->pid, 
                        (thread->policy == SCHED_FIFO)  ? "SCHED_FIFO" :
                        (thread->policy == SCHED_RR)    ? "SCHED_RR" :
                        (thread->policy == SCHED_OTHER) ? "SCHED_OTHER" : "???", 
                        thread->priority);
        pthread_mutex_unlock(&thread->mutex_locker);
    }
	pthread_mutex_unlock(&pool->mutex_locker);
    
    *outlen = len;
                    
    return 0;
}



