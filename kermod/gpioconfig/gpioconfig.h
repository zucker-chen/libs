/*	filename : gpioconfig.h
*		for driver gpioconfig.c
*/
#ifndef _GPIOCONFIG_H_
#define _GPIOCONFIG_H_

//#include <linux/ioctl.h>

#define DEVICE_GPIOCONFIG "gpioconfig"

/*
*	传递的GPIO 信息
*	gio_id：IO编号
*	value：IO 的状态值
*/
struct gpio_info
{
	unsigned short	 gio_id;
	unsigned int	 value;
};

#define GPIOCONFIG_IOCSET			0x0
#define GPIOCONFIG_IOCGET			0x1
#define GPIOCONFIG_IOCDIR			0x2
#define GPIOCONFIG_IOCPWM_FREQ		0x3		// set pwm freq (need set first), value>>>  0:disable, >0: pwm freq(Hz)
#define GPIOCONFIG_IOCPWM_DUTY		0x4		// set pwm duty


#define DIR_NON		0
#define DIR_IN		11
#define DIR_OUT		22

#define VALUE_HIGH 	1
#define VALUE_LOW	0


#endif
