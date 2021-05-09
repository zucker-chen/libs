#include "twtimer.h"
#include <time.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

#define TIMER 1000
#define TOTAL 10000000
#define TIMER_RESOLUTION 64

static uint64_t gtime;
time_wheel_t* wheel;
static twtimer_t* s_timer;
static int timeout_flag = 0;

static void ontimer(void* param)
{
    uint64_t time = 0, diff = 0;
    twtimer_t* timer;
    
    timer = (twtimer_t*)param;
    time = twtimer_get_systime();
    diff = (time - gtime > timer->expire) ? time - gtime - timer->expire : timer->expire - (time - gtime);
    printf("### src = %lums, dst = %lums, diff = %lums\n", timer->expire, time - gtime, diff);
	timer->param = NULL;
    timeout_flag = 1;
}

static void ontimer_continue(void* param)
{
    uint64_t time = 0, diff = 0;
    twtimer_t* timer;
    static uint64_t last_time = 0;
    
    timer = (twtimer_t*)param;
    time = twtimer_get_systime();
    if (last_time == 0) {
        last_time = gtime;
    }
    
    diff = (time > (last_time + timer->expire)) ? time - last_time - timer->expire : last_time + timer->expire - time;
    printf("### src = %lums, dst = %lums, diff = %lums\n", timer->expire, time - last_time, diff);
    last_time = time;
}

static void signal_handle(int sig)
{
    if(sig == SIGINT)    // ctrl+c
    {
        printf("SIGINT: CTRL+C\n");
    }
    else if(sig == SIGQUIT)  // ctrl+/
    {
        printf("SIGQUIT: CTRL+/\n");
    }
    else if(sig == SIGALRM)  // ctrl+/
    {
        printf("SIGALRM: alarm\n");
    }
    else
    {
        printf("signal others\n");
    }    
    
}

// API 1 TEST
static void test1()
{
    int i = 0, cnt = 0;
    uint64_t time = 0;
    
    timeout_flag = 0;
    gtime = time = twtimer_get_systime();
	wheel = time_wheel_create(time);
    
    s_timer[i].ontimeout = ontimer;
    s_timer[i].param = &s_timer[i];
    s_timer[i].expire = 2900;    // ms
    s_timer[i].type = TIMER_ONESHOT;
    time_wheel_start(wheel, &s_timer[i]);
    while (cnt++ < 100 && timeout_flag == 0) {
        time_wheel_process(wheel, twtimer_get_systime());
        twtimer_msleep(32);
    }

    time_wheel_stop(wheel, &s_timer[i]);
    time_wheel_destroy(wheel);
}

// API 2 TEST
static void test2()
{
    int i = 0, cnt = 0;
    tw_handle_t *wheel;
    
    gtime = twtimer_get_systime();
    
    i = 0;
    timeout_flag = 0;
    s_timer[i].ontimeout = ontimer;
    s_timer[i].param = &s_timer[i];
    s_timer[i].expire = 2600;    // ms
    s_timer[i].type = TIMER_ONESHOT;
    wheel = twtimer_start(&s_timer[i]);
    
    while (cnt++ < 100 && timeout_flag == 0) {
        twtimer_msleep(1000);
    }
    
    twtimer_stop(wheel);
}

// SIGNAL TEST
static void test3()
{
    int left = 0;
    uint64_t t1, t2;
    
    //signal(SIGINT, signal_handle);
    signal(SIGALRM, signal_handle);

    alarm(2);
    left = sleep(10);
    printf("sleep(10) left = %d\n", left);

    t1 = twtimer_get_systime();
    alarm(3);
    twtimer_msleep(5000);
    t2 = twtimer_get_systime();
    printf("twtimer_msleep(5000), dist = %lu\n", t2 - t1);
}

// API 2 continue timer TEST
static void test4()
{
    int i = 0, cnt = 0;
    tw_handle_t *wheel;
    
    gtime = twtimer_get_systime();
    
    i = 0;
    s_timer[i].ontimeout = ontimer_continue;
    s_timer[i].param = &s_timer[i];
    s_timer[i].expire = 500;    // ms
    s_timer[i].type = TIMER_CONTINUS;
    wheel = twtimer_start(&s_timer[i]);
    while (cnt++ < 10) {
        twtimer_msleep(1000);
    }
    
    twtimer_stop(wheel);
}


int main (int argc, char *argv[])
{

	s_timer = (twtimer_t*)calloc(TIMER, sizeof(*s_timer));
    
    printf("\n============= test1 (API 1) start ==============\n");
    test1();
    printf("\n============= test2 (API 2) start ==============\n");
    test2();
    printf("\n===== test3 (SIGNAL twtimer_msleep) start ======\n");
    test3();
    printf("\n====== test4 (continue timer test) start =======\n");
    test4();
    
    printf("================== TEST  END ===================\n");
    
    sleep(5);
    free(s_timer);

	return 0; 
}
