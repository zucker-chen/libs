/*****************************************************************
** 文件名: user-timer.c
** Copyright (c) 2020 zucker-chen
 
** 创建人: 
** 日  期: 
** 描  述: 定时器
** 版  本: V1.0

** 修改人:
** 日  期:
** 修改描述:
** 版  本: 
******************************************************************/

/*****************************************************************
* 包含头文件
******************************************************************/
#include <linux/module.h>
#include <linux/types.h>
#include <linux/init.h>
#include "utimer.h"



static usr_timer_t utimer;
static int utimer_demo_handle(void *param)
{

    printk("%s - - - \n", __FUNCTION__);
    //utimer_restart(&utimer, 1000000);
    
    return 0;
}

static int utimer_demo_init(void)
{
    utimer.type = USER_TIMER_HIGH;
    utimer.function = utimer_demo_handle;
    utimer.data = (void *)&utimer;
    utimer_init(&utimer);
    
    utimer_start(&utimer, 1000000);

    return 0;
}

static int utimer_demo_deinit(void)
{
    return utimer_destory(&utimer);
}


static uint64_t get_systime_ms(void)
{
	uint64_t time_ms = 0;

	struct timeval tv;
	do_gettimeofday(&tv);
	time_ms = (uint64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;

	return time_ms;
}

static int utimer_usleep_test(void)
{
	int time_us = 0;
	printk("%s : %llu, t = %d.\n", __FUNCTION__, get_systime_ms(), time_us/1000);
	time_us = 10*1000;
	utimer_usleep(time_us);
	printk("%s : %llu, t = %d.\n", __FUNCTION__, get_systime_ms(), time_us/1000);

	time_us = 100*1000;
	utimer_usleep(time_us);
	printk("%s : %llu, t = %d.\n", __FUNCTION__, get_systime_ms(), time_us/1000);

	time_us = 3000*1000;
	utimer_usleep(time_us);
	printk("%s : %llu, t = %d.\n", __FUNCTION__, get_systime_ms(), time_us/1000);

	return 0;
}


static int __init utimer_mod_init(void)
{
    printk("%s - hello - \n", __FUNCTION__);
    utimer_demo_init();
	
    printk("%s - utimer_usleep 5s start - \n", __FUNCTION__);
	utimer_usleep(5000000);
    printk("%s - utimer_usleep 5s end - \n", __FUNCTION__);
    
	utimer_usleep_test();
	
    return 0;
}

static void __exit utimer_mod_exit(void)
{
    utimer_demo_deinit();
    printk("%s - bye - \n", __FUNCTION__);
}


MODULE_AUTHOR("zucker-chen <timeontheway@163.com>");
MODULE_DESCRIPTION("user timer");
MODULE_LICENSE("GPL");

module_init(utimer_mod_init);
module_exit(utimer_mod_exit);




