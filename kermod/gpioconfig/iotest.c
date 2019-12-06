#include <stdio.h>
#include <sys/ioctl.h>
#include "gpioconfig.h"
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(int argc, char **argv)
{

	int sel = 0;		// 0: get, 1: set, 2: pwm
	int id;
	int value;
	int fd = -1;
	int ret = -1;
	
	struct gpio_info gpio;

	sel = (unsigned int)strtoul(argv[1], NULL, 0);
	id = (unsigned int)strtoul(argv[2], NULL, 0);
	value = (unsigned int)strtoul(argv[3], NULL, 0);
	gpio.gio_id = id;
	gpio.value = value;	
	
	fd = open("/dev/gpioconfig", O_RDWR, 0);
	if(fd < 0)
	{
		perror("#open dev");
		return -1;
	}
	//printf("cmd=0x%x\n", GPIOCONFIG_IOCSET);
	if (sel == 0)
	{
		ret = ioctl(fd, GPIOCONFIG_IOCGET, &gpio);
	}
	else if (sel == 1)
	{
		ret = ioctl(fd, GPIOCONFIG_IOCSET, &gpio);
	}
	else if (sel == 2)
	{
		ret = ioctl(fd, GPIOCONFIG_IOCPWM_FREQ, &gpio);
	}
	else if (sel == 3)
	{
		ret = ioctl(fd, GPIOCONFIG_IOCPWM_DUTY, &gpio);
	}
	if(ret < 0)
	{
		perror("#ioctl");
	}
	
	close(fd);
	
	return 0;
	
}
