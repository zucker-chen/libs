#ifndef __LENS_TIMER__
#define __LENS_TIMER__

#include <linux/timer.h>
#include <linux/interrupt.h>
#include <asm/io.h>

int LENS_GetTime(void);

#if 0
int LENS_TimerInit(struct timer_list *pTimer);
int LENS_TimerSet(struct timer_list *pTimer,unsigned long ulInterval);
int LENS_TimerDestory(struct timer_list *pTimer);
#else 
int LENS_TimerInit(irq_handler_t handler,void **pTimerBase);
int LENS_TimerDeinit(void);
#endif

#endif