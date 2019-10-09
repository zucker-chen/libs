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
    
    printf("arg = %d, sleep(%d)\n", *(char *)arg, t);
    sleep(t);
}


int main (int argc, char *argv[])
{
    int i;
    char name[32] = "\0";
    char arg[500] = "\0";
    
    threadpool_init(20);
    
    for (i = 0; i < 128; i++)
    {
        arg[i] = i;
        sprintf(name, "thread_%d", i);
        threadpool_run(thread_cb, &arg[i], name);
    }

    
    threadpool_destroy();
    printf("threadpool destroyed\n");
    
	return 0;	
}




