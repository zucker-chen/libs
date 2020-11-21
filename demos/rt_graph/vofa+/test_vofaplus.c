#include "vofaplus.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <signal.h>



#define pri_dbg(format, args...) fprintf(stderr,"%s %d %s() " format, __FILE__, __LINE__, __func__, ## args)


void ringbuf_test(void)
{
	vofaplus_t *vofa = NULL;
	float t = 0;
	float data[2] = {0};
	
	vofa = vofaplus_init("192.168.40.145", 1347);

	pri_dbg(" %d==\n", vofa->fd);

	while (1)
	{
		t += 0.2;
		data[0] = sin(t);
		data[1] = cos(4*t);
		vofaplus_data_send(vofa, data, 2);
		usleep(100000);
	}
	
	vofaplus_deinit(vofa);
  
    return;
}


static void signal_handle(int sig)
{
    if(sig == SIGINT)    // ctrl+c
    {
        printf("SIGINT: CTRL+C\n");
		exit(0);
    }
    else if(sig == SIGQUIT)  // ctrl+/
    {
        printf("SIGQUIT: CTRL+/\n");
    }
    else if(sig == SIGALRM)  // ctrl+/
    {
        printf("SIGALRM: alarm\n");
    }
    else if(sig == SIGPIPE)  // socket FIN
    {
        printf("SIGPIPE: socket FIN\n");
    }
    else
    {
        printf("signal others\n");
    }    
    
}


int main (int argc, char *argv[])
{
    signal(SIGPIPE, signal_handle);
    signal(SIGINT, signal_handle);

	ringbuf_test();

	return 0;
}



