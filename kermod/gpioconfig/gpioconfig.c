/*****************************************************************
** 文件名: gpioconfig.c
** Copyright (c) 2013 高新兴科技集团股份有限公司嵌入式软件部
 
** 创建人: 
** 日  期: 
** 描  述: gpio驱动模块
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
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include "gpioconfig.h"
#include <linux/device.h>
#include <linux/slab.h>

#include <linux/kernel.h>
#include <linux/fcntl.h>
#include <linux/miscdevice.h>
#include <linux/proc_fs.h>
#include <linux/gpio.h>
#include <linux/timer.h>



/*****************************************************************
* 定时器 ---------------------------------------------------------
******************************************************************/
typedef struct usr_timer{
    void *timer;
    void (*function)(unsigned long);
    unsigned long data;
}usr_timer_t;

int usr_timer_init(usr_timer_t *timer)
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
EXPORT_SYMBOL(usr_timer_init);

// interval (us)
int usr_set_timer(usr_timer_t *timer, unsigned long interval)
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
EXPORT_SYMBOL(usr_set_timer);

int usr_del_timer(usr_timer_t *timer)
{
    struct timer_list *t = NULL;
    if(timer == NULL || timer->timer == NULL || timer->function == NULL){
        printk("%s - parameter invalid!\n", __FUNCTION__);
        return -1;
    }
    t = timer->timer;
    return del_timer(t);
}
EXPORT_SYMBOL(usr_del_timer);

int usr_timer_destory(usr_timer_t *timer)
{
    struct timer_list *t = timer->timer;
    del_timer(t);
    kfree(t);
    timer->timer=NULL;
    return 0;
}
EXPORT_SYMBOL(usr_timer_destory);

/*****************************************************************
* 定时器 ---------------------------------------------------------
******************************************************************/



/*************宏定义********************/
#define DRV_VERSION "1.0.0"

#define GPIO_GROUP_END	15 //最多16组
#define GPIO_ID_END		7 //最多8个

/************************结构体定义*************************/

struct gpio_cdev{
	struct cdev cdev;
	struct gpio_info info;
};

/**************************静态变量定义******************************/
#define DEVICE_MAJOR 0
//static int gpio_major = DEVICE_MAJOR;		//主设备号
struct gpio_cdev *gpio_cdevp;

/****************************函数定义**************************************/

int set_gpio_request(unsigned int group, unsigned int id)
{
	unsigned int gpio_num = 0;
	int ret = -1;

	gpio_num = group * 8 + id;
	ret = gpio_request(gpio_num, NULL);

	return ret;
}
EXPORT_SYMBOL(set_gpio_request);

int set_gpio_free(unsigned int group, unsigned int id)
{
	unsigned int gpio_num = 0;
	int ret = 0;

	gpio_num = group * 8 + id;
	gpio_free(gpio_num);

	return ret;
}
EXPORT_SYMBOL(set_gpio_free);

int set_gpio_dir(unsigned int group, unsigned int id, unsigned int dir)
{
	unsigned int gpio_num = 0;
	int ret = -1;

	gpio_num = group * 8 + id;
	if(dir == DIR_IN)
	{
		ret = gpio_direction_input(gpio_num);
	}
	else if(dir == DIR_OUT)
	{
		ret = gpio_direction_output(gpio_num, 0);
	}
	
	return ret;
}
EXPORT_SYMBOL(set_gpio_dir);

int set_gpio_value(unsigned int group, unsigned int id, unsigned int value)
{
	unsigned int gpio_num = 0;
	int ret = 0;

	gpio_num = group * 8 + id;
	gpio_set_value(gpio_num, ~~value);
	
	return ret;
}
EXPORT_SYMBOL(set_gpio_value);

int get_gpio_value(unsigned int group, unsigned int id)
{
	unsigned int gpio_num = 0;
	int ret = -1;

	gpio_num = group * 8 + id;
	ret = gpio_get_value(gpio_num);
	
	return ret;
}
EXPORT_SYMBOL(get_gpio_value);

static int gpioconfig_open( struct inode *node, struct file *filp )
{
	//filp->private_data = gpio_cdevp;
	return 0;
}

static int gpioconfig_release( struct inode *node, struct file *filp )
{
	return 0;
}


// GPIO模拟PWM
#define GPIO2PWM_MAX_NUM	4
typedef struct gpio2pwm_ctx_s
{
	usr_timer_t 	usr_timer;
	unsigned short	group;
	unsigned short	id;
	unsigned int 	hz;
	unsigned int 	duty;
}gpio2pwm_ctx_t;

static gpio2pwm_ctx_t gpio2pwm_ctx[GPIO2PWM_MAX_NUM] = {0};

static void gpio2pwm_timer_handle(unsigned long data)
{
	static int flag = 0;
	//int ret = 0;
	gpio2pwm_ctx_t *pctx = NULL;

	pctx = (gpio2pwm_ctx_t *)data;
	if (pctx->usr_timer.timer == NULL)
	{
		return;
	}
	
	usr_del_timer(&pctx->usr_timer);
	if (flag == 0)
	{
		set_gpio_value(pctx->group, pctx->id, 1);
		usr_set_timer(&pctx->usr_timer, (1000000/pctx->hz) * pctx->duty / 100);
		flag = 1;
		//printk(KERN_WARNING " gpio2pwm_timer_handle flag = %d\n", flag);
	}
	else
	{
		set_gpio_value(pctx->group, pctx->id, 0);
		usr_set_timer(&pctx->usr_timer, (1000000/pctx->hz) * (100 - pctx->duty) / 100);
		flag = 0;
	}

}

static int gpio2pwm_freq(unsigned int group, unsigned int id, int hz)
{
	int ret = 0, i = 0;
	gpio2pwm_ctx_t *pctx = NULL;
	
	if (hz <= 0)	// del timer
	{
		for (i = 0; i < GPIO2PWM_MAX_NUM; i++)
		{
			pctx = (gpio2pwm_ctx_t *)&gpio2pwm_ctx[i];
			if (pctx->usr_timer.timer != NULL && pctx->group == group && pctx->id == id)
			{
				usr_timer_destory(&pctx->usr_timer);
				return 0;
			}
		}
	}
	else
	{
		for (i = 0; i < GPIO2PWM_MAX_NUM; i++)
		{
			pctx = (gpio2pwm_ctx_t *)&gpio2pwm_ctx[i];
			if (pctx->usr_timer.timer == NULL)
			{
				pctx->group = group;
				pctx->id = id;
				pctx->hz = hz;				
				pctx->usr_timer.function = gpio2pwm_timer_handle;
				pctx->usr_timer.data = (unsigned long)pctx;
				ret = usr_timer_init(&pctx->usr_timer);
				if(ret)
				{
					printk("could not register lens_timer_init. \n");
					return ret;
				} 
				return 0;
			}
			else if (pctx->group == group && pctx->id == id)
			{
				pctx->hz = hz;				
				return 0;
			}
		}
	}
	
	return -1;
}

static int gpio2pwm_duty(unsigned int group, unsigned int id, int duty)
{
	int ret = 0, i = 0;
	gpio2pwm_ctx_t *pctx = NULL;

	for (i = 0; i < GPIO2PWM_MAX_NUM; i++)
	{
		pctx = (gpio2pwm_ctx_t *)&gpio2pwm_ctx[i];
		if (pctx->usr_timer.timer != NULL && pctx->group == group && pctx->id == id)
		{
			usr_del_timer(&pctx->usr_timer);
			pctx->duty = duty;
			ret = set_gpio_dir(pctx->group, pctx->id, DIR_OUT);
			if (pctx->duty >= 100)
			{
				ret = set_gpio_value(pctx->group, pctx->id, 1);
			}
			else if (pctx->duty <= 0)
			{
				ret = set_gpio_value(pctx->group, pctx->id, 0);
			}
			else
			{
				ret = usr_set_timer(&pctx->usr_timer, 1000000/pctx->hz);
			}
			printk(KERN_WARNING "gpio2pwm_set ret=%d,group=0x%x,id=0x%x,value=0x%x\n", ret, group, id, duty);
			
			return ret;
		}
	}

	return -1;
}


static long gpio_ioctl (struct file *filp, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	struct gpio_info ioarg;
	unsigned short group,id;
	
	ret = copy_from_user(&ioarg, (const void *)arg, sizeof(struct gpio_info));
	group = ioarg.gio_id>>8;
	id = ioarg.gio_id&0xff;
	//printk(KERN_WARNING "kernel set gpio cmd=0x%x,group=0x%x,id=0x%x,value=0x%x\n", cmd, group, id, ioarg.value);
	
	if((group > GPIO_GROUP_END) || (id > GPIO_ID_END))
	{
		return -EFAULT;
	}

	set_gpio_request(group, id);	
	switch(cmd) 
	{
		case GPIOCONFIG_IOCGET:			
			ret = set_gpio_dir(group, id, DIR_IN);
			ioarg.value = get_gpio_value(group, id);
			ret = copy_to_user((void *)arg, &ioarg, sizeof(struct gpio_info));			
			break;
		
		case GPIOCONFIG_IOCSET:
			ret = set_gpio_dir(group, id, DIR_OUT);
			ret = set_gpio_value(group, id, ioarg.value);
			break;
			
		case GPIOCONFIG_IOCDIR:
			ret = set_gpio_dir(group, id, ioarg.value);
			break;
		
		case GPIOCONFIG_IOCPWM_FREQ:
			ret = gpio2pwm_freq(group, id, ioarg.value);
			break;
		
		case GPIOCONFIG_IOCPWM_DUTY:
			ret = gpio2pwm_duty(group, id, ioarg.value);
			break;
		
		default:
			printk(KERN_WARNING " DEFAULT #############\n\n");
			ret = -EINVAL;
	}
	set_gpio_free(group, id);	

	return ret;
	
}

static const struct file_operations gpio_cdev_fops =
{
	.owner 		= THIS_MODULE,
	.open 		= gpioconfig_open,
	.release 	= gpioconfig_release,
	.unlocked_ioctl		= gpio_ioctl,
};

static struct miscdevice gpioconfig_dev = {
  MISC_DYNAMIC_MINOR,
  "gpioconfig",
   &gpio_cdev_fops,
};


static int __init gpioconfig_init(void)
{  
    int ret = 0; 

    /*注册设备*/
    ret = misc_register(&gpioconfig_dev);
    if(ret)
    {
        printk("could not register alarm devices. \n");
        return ret;
    }
    
    printk("gpioconfig driver init successful.\n\n");
    
    return ret;
}

static void __exit gpioconfig_exit(void)
{
	int i = 0;
	gpio2pwm_ctx_t *pctx = NULL;
	
    misc_deregister(&gpioconfig_dev);
	
	for (i = 0; i < GPIO2PWM_MAX_NUM; i++)
	{
		pctx = (gpio2pwm_ctx_t *)&gpio2pwm_ctx[i];
		if (pctx->usr_timer.timer != NULL)
		{
			usr_del_timer(&pctx->usr_timer);
			usr_timer_destory(&pctx->usr_timer);
		}
	}
}

MODULE_AUTHOR("Alessandro Zummo <a.zummo@towertech.it>");
MODULE_DESCRIPTION("GPIO driver");
MODULE_LICENSE("GPL");
MODULE_VERSION(DRV_VERSION);

module_init(gpioconfig_init);
module_exit(gpioconfig_exit);




