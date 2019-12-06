#include "lensTimer.h"

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/slab.h>




int lens_timer_init(lens_timer_t *timer)
{
    struct timer_list *t = NULL;

    if(timer == NULL){
        printk("%s - parameter invalid!\n", __FUNCTION__);
        return -1;
    }

    t = (struct timer_list *)kmalloc(sizeof(struct timer_list), GFP_KERNEL);
    if(t == NULL){
        printk("%s - kmalloc error!\n", __FUNCTION__);
        return -1;
    }

    init_timer(t);
    timer->timer = t;
    return 0;
}
//EXPORT_SYMBOL(lens_timer_init);

// interval (us)
int lens_set_timer(lens_timer_t *timer, unsigned long interval)
{
    struct timer_list *t = NULL;
    if(timer == NULL || timer->timer == NULL || timer->function == NULL || interval == 0){
        printk("%s - parameter invalid!\n", __FUNCTION__);
        return -1;
    }
    t = timer->timer;
    t->function = timer->function;
    t->data = timer->data;
    return mod_timer(t, jiffies + usecs_to_jiffies(interval) - 1);
}
//EXPORT_SYMBOL(lens_set_timer);

int lens_del_timer(lens_timer_t *timer)
{
    struct timer_list *t = NULL;
    if(timer == NULL || timer->timer == NULL || timer->function == NULL){
        printk("%s - parameter invalid!\n", __FUNCTION__);
        return -1;
    }
    t = timer->timer;
    return del_timer(t);
}
//EXPORT_SYMBOL(lens_del_timer);

int lens_timer_destory(lens_timer_t *timer)
{
    struct timer_list *t = timer->timer;
    del_timer(t);
    kfree(t);
    timer->timer=NULL;
    return 0;
}
//EXPORT_SYMBOL(lens_timer_destory);

unsigned long lens_msleep(unsigned int msecs)
{
    return  msleep_interruptible(msecs);
}
//EXPORT_SYMBOL(lens_msleep);

void lens_udelay(unsigned int usecs)
{
    udelay(usecs);
}
//EXPORT_SYMBOL(lens_udelay);

void lens_mdelay(unsigned int msecs)
{
    mdelay(msecs);
}
//EXPORT_SYMBOL(lens_mdelay);

unsigned int lens_get_tickcount(void)
{
    return jiffies_to_usecs(jiffies);
}
//EXPORT_SYMBOL(lens_get_tickcount);

unsigned long long lens_sched_clock(void)
{
    return sched_clock();
}
//EXPORT_SYMBOL(lens_sched_clock);

void lens_getjiffies(unsigned long long *pjiffies)
{
    *pjiffies = jiffies;
}
//EXPORT_SYMBOL(lens_getjiffies);








