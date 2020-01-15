/*	filename : user-timer.h
 *	
 */
#ifndef __USER_TIMER_H__
#define __USER_TIMER_H__

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
    int                    (*function)(void *);    // USER_TIMER_NORMAL: void * = unsigned long, USER_TIMER_HIGH void * = struct hrtimer *
} usr_timer_t;



#endif
