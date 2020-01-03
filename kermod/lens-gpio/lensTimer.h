#ifndef __LENS_TIMER__
#define __LENS_TIMER__



//timer
typedef struct lens_timer{
    void *timer;
    void (*function)(unsigned long);
    unsigned long data;
}lens_timer_t;


int lens_timer_init(lens_timer_t *timer);
int lens_set_timer(lens_timer_t *timer, unsigned long interval);
int lens_del_timer(lens_timer_t *timer);
int lens_timer_destory(lens_timer_t *timer);
unsigned long lens_msleep(unsigned int msecs);
void lens_udelay(unsigned int usecs);
void lens_mdelay(unsigned int msecs);
unsigned int lens_get_tickcount(void);
unsigned long long lens_sched_clock(void);
void lens_getjiffies(unsigned long long *pjiffies);




#endif
