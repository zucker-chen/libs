#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#include "pthread_pool.h"


void thread_cb(void *arg)
{
    int t = rand() % 10;
    
    sleep(t);
    printf("arg = %d, sleep(%d)\n", *(char *)arg, t);
    
}


int main (int argc, char *argv[])
{
    int ret, i, pindex;
    char name[32] = "\0";
    char arg[500] = "\0";
    char dump[2*1024] = "\0";
    int  dump_len = 0;
    int policy, priority;
    
    threadpool_init(10);
    
    for (i = 0; i < 15; i++)
    {
        arg[i] = i;
        sprintf(name, "thread_%d", i);
        ret = threadpool_run(&pindex, thread_cb, &arg[i], name);
        if (ret < 0) {
            printf("threadpool_run error, i = %d\n", i);
            usleep(1000000);
            continue;
        }
        threadpool_bind_cpu(pindex, 0);
        threadpool_set_index_sched_priority(pindex, SCHED_RR, i < 50 ? 50 + i : 0);
    }
    threadpool_get_index_sched_priority(5, &policy, &priority);
    
    threadpool_dump_info(dump, &dump_len);
    printf("dump_len = %d\n%s\n", dump_len, dump);
    
    sleep(5);
    threadpool_dump_info(dump, &dump_len);
    printf("dump_len = %d\n%s\n", dump_len, dump);
    
    threadpool_destroy();
    printf("threadpool destroyed\n");
    
	return 0;	
}




