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
    int i, pindex;
    char name[32] = "\0";
    char arg[500] = "\0";
    
    threadpool_init(10);
    
    //sleep(5);
    
    for (i = 0; i < 15; i++)
    {
        arg[i] = i;
        sprintf(name, "thread_%d", i);
        threadpool_run(&pindex, thread_cb, &arg[i], name);
        threadpool_bind_cpu(pindex, 0);
        threadpool_set_sched_rr_priority(pindex, i < 100 ? i : 0);
        sleep(1);
    }
    
    threadpool_destroy();
    printf("threadpool destroyed\n");
    
	return 0;	
}




