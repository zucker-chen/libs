/*	filename : user-timer.h
 *	
 */
#ifndef __USER_TIMER_H__
#define __USER_TIMER_H__

#include <linux/hrtimer.h>

enum user_timer_type {
	USER_TIMER_NORMAL = 0,	/* normal(mod_timer) */
	USER_TIMER_HIGH,	    /* high timer(hrtimer) */
};


typedef struct usr_timer {
    int                     type;
    union {
        struct timer_list   t;
        struct hrtimer      ht;
    }                       timer;
    int                    (*function)(void *);     // USER_TIMER_NORMAL: void * = unsigned long, USER_TIMER_HIGH void * = struct hrtimer *
    void                    *data;                  // function param
	struct task_struct		*task;					// used for sleep
	int						timer_expire;			// used for sleep
} usr_timer_t;


int utimer_init(usr_timer_t *timer);
int utimer_destory(usr_timer_t *timer);
int utimer_start(usr_timer_t *timer, unsigned long interval);   // interval (us)
int utimer_restart(usr_timer_t *timer, unsigned long interval); // interval (us)
int utimer_usleep(int us);

#endif
