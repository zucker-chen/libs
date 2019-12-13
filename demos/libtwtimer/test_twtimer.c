#include "twtimer.h"
#include <time.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

#define TIMER 1000
#define TOTAL 10000000
#define TIMER_RESOLUTION 64

static uint64_t now;
static time_wheel_t* wheel;
static struct twtimer_t* s_timer;
static int timeout_flag = 0;

static void ontimer(void* param)
{
    uint64_t time = 0, diff = 0;
    struct twtimer_t* timer;
    
    timer = (struct twtimer_t*)param;
    time = twtimer_sysclock();
    diff = (time > timer->expire) ? time - timer->expire : timer->expire - time;
    printf("### src = %lums, dst = %lums, diff = %lums\n", timer->expire - now, time - now, diff);
	timer->param = NULL;
    timeout_flag = 1;
}


static void test1()
{
    int i = 0, cnt = 0;
    uint64_t time = 0;
    
    timeout_flag = 0;
    now = time = twtimer_sysclock();
	wheel = time_wheel_create(time);
    
    s_timer[i].ontimeout = ontimer;
    s_timer[i].param = &s_timer[i];
    s_timer[i].expire = time + 2900;    // + ms
    twtimer_start(wheel, &s_timer[i]);
    
    while (cnt++ < 100 && timeout_flag == 0) {
        twtimer_process(wheel, twtimer_sysclock());
        twtimer_msleep(32);
    }

    twtimer_stop(wheel, &s_timer[i]);
    time_wheel_destroy(wheel);
}


static void test2()
{
    int i = 0, cnt = 0;
    
    now = twtimer_sysclock();
    twtimer_init();
    
    i = 1;
    timeout_flag = 0;
    s_timer[i].ontimeout = ontimer;
    s_timer[i].param = &s_timer[i];
    s_timer[i].expire = 2600;    // ms
    
    twtimer_add(&s_timer[i]);
    
    while (cnt++ < 100 && timeout_flag == 0) {
        twtimer_msleep(1000);
    }
    
    twtimer_del(&s_timer[i]);
    twtimer_deinit();
}




int main (int argc, char *argv[])
{

	s_timer = (struct twtimer_t*)calloc(TIMER, sizeof(*s_timer));
    
    test1();
    test2();

	return 0; 
}
