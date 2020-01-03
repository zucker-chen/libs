#include <stdio.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <signal.h>
#include <sys/select.h>
#include <pthread.h>
#include <sys/mman.h> 

#define LENS_START		0x1 //开始电机控制处理
#define LENS_MOTORCTRL			0x3 //	电机控制
#define LENS_REGSET			0x4	//寄存器值设置
#define LENS_REGGET			0x5	//寄存器值获取

#define LOW 0
#define HIGH 1
#define IRISINSPIREPIN	7,5 /*41908光圈激励引脚*/
#define VDFZINSPIREPIN  7,6/*41908VD_FZ激励引脚*/
#define RESETPIN        7,4/*41908芯片复位引脚*/
#define SPICSPIN        1,5/*41908片选引脚*/

#define GPIOCONFIG_IOCSET		0x0
#define GPIOCONFIG_IOCGET		0x1
#define GPIOCONFIG_IOCDIR		0x2

typedef enum
{
	LENS_ZOOMTELE  = 1,
	LENS_ZOOMWIDE = 2,
	LENS_FOCUSNEAR = 3,
	LENS_FOCUSFAR  = 4,
	LENS_CTRLBUTT,
} LENS_CTRL_E;

typedef enum 
{
	LENS_MOTORRUN = 1,
	LENS_MOTORSTOP = 2,
	LENS_DISABLE = 3,
	LENS_MOTORBUTT,
}LENS_STATE_E;

typedef struct lensCtrl
{
	LENS_CTRL_E eZoomCtrlType;
	LENS_CTRL_E eFocusCtrlType;
	unsigned int unZoomStep;
	unsigned int unFocusStep;
	unsigned short usRegAddr;
	unsigned short usRegValue;
}LENS_CTRL_ST;

/*
*	传递的GPIO 信息
*	gio_id：IO编号
*	value：IO 的状态值
*/
struct gpio_info
{
	unsigned short	 gio_id;
	char value;
};

void input_handler(int sig)
{
	static struct timeval now,before;
	gettimeofday(&now, NULL);//获取当前时间，秒/ 微秒两部分
	
	unsigned int nDiff = now.tv_sec*1000+ now.tv_usec/1000 - before.tv_sec*1000-before.tv_usec/1000;
	
	if(nDiff <39 || nDiff > 42)
	printf("before = %ld -----now = %ld ms-------nDiff = %d\n",before.tv_sec*1000+ before.tv_usec/1000,now.tv_sec*1000+ now.tv_usec/1000,nDiff); 
	
	before = now;
}

/*****************************************************************
  Function: 	HAL_LENS_GpioSet
  Description:	gpio状态设置函数
  Input:		cValue:设置的值 cGroup：gpio所属组 cId：gpio引脚号 
  Output:		无
  Return:		操作成功，则返回0，无则返回<0			
******************************************************************/
int HAL_LENS_GpioSet(char cValue,char cGroup,char cId)
{	
	int nGpioFd = -1;
	int nRet;
	struct gpio_info gpio;
	
	gpio.gio_id = (cGroup<<8)|(cId);
	gpio.value = cValue;	

	nGpioFd = open("/dev/gpioconfig", O_RDWR, 0);
	if(nGpioFd < 0)
	{
		perror("#open dev");
		return -1;
	}
	
	nRet = ioctl(nGpioFd, GPIOCONFIG_IOCSET, &gpio);
	if(nRet < 0)
	{
		perror("HAL_LENS_GpioSet #ioctl");
	}

	close(nGpioFd);
	
	return 0;
	
}

/*****************************************************************
  Function: 	HAL_LENS_DelayMs
  Description:	ms延时函数
  Input:		ms 延时时间  
  Output:		无
  Return:				
******************************************************************/

static void HAL_LENS_DelayMs(int ms) 
{ 
	usleep(ms*1000);
}

/*****************************************************************
  Function: 	HAL_LENS_IrisInspire
  Description:	 MS41908光圈激励信号
  Input:		无
  Output:		无
  Return:		操作成功，则返回0，无则返回<0			
******************************************************************/
static int HAL_LENS_IrisInspire(void)
{
	HAL_LENS_GpioSet(LOW,IRISINSPIREPIN);
	HAL_LENS_DelayMs(1);
	HAL_LENS_GpioSet(HIGH,IRISINSPIREPIN);
	HAL_LENS_DelayMs(1);
	HAL_LENS_GpioSet(LOW,IRISINSPIREPIN);
	HAL_LENS_DelayMs(20);
 
	return 0;
}
#ifdef 1
int main()
{
		
	LENS_CTRL_ST stCtrl;
	
	int fd = 0,nRet;
	
	fd = open("/dev/LensDrv", O_RDWR, 0);
	if(fd < 0)
	{
		perror("#open dev");
		return ;
	}
	
	while(1)
	{
		stCtrl.usRegAddr = 0x00;
		nRet = ioctl(fd, LENS_REGGET, &stCtrl);
		if(nRet < 0)
		{
			perror("LENS_REGGET #ioctl");
			break;
		}
		printf("LENS_REGGET,usRegAddr:%d,usRegValue:%d\n",stCtrl.usRegAddr,stCtrl.usRegValue);
		sleep(1);

		stCtrl.usRegAddr = 0x00;
		if (stCtrl.usRegValue)
			stCtrl.usRegValue = 0;
		else
			stCtrl.usRegValue = 300;

		nRet = ioctl(fd, LENS_REGSET, &stCtrl);
		if(nRet < 0)
		{
			perror("LENS_REGSET #ioctl");
			break;
		}
		HAL_LENS_IrisInspire();
		printf("LENS_REGSET,usRegAddr:%d,usRegValue:%d\n",stCtrl.usRegAddr,stCtrl.usRegValue);
		sleep(1);
	}
	

	close(fd);

	return 0;
}
#else
int main()
{
		
	int oflags;
	
	int fd = 0;
	
	fd = open("/dev/LensDrv", O_RDWR, 0);
	if(fd < 0)
	{
		perror("#open dev");
		return ;
	}

	
	//启动信号驱动机制,将SIGIO信号同input_handler函数关联起来,一旦产生SIGIO信号,就会执行input_handler 
	signal(SIGIO, input_handler);	
	//STDIN_FILENO是打开的设备文件描述符,F_SETOWN用来决定操作是干什么的,getpid()是个系统调用，
	
	//功能是返回当前进程的进程号,整个函数的功能是STDIN_FILENO设置这个设备文件的拥有者为当前进程。
	fcntl(fd, F_SETOWN, getpid());    

	//得到打开文件描述符的状态
	oflags = fcntl(fd, F_GETFL);
	//设置文件描述符的状态为oflags | FASYNC属性,一旦文件描述符被设置成具有FASYNC属性的状态，
	//也就是将设备文件切换到异步操作模式。这时系统就会自动调用驱动程序的fasync方法。
	fcntl(fd, F_SETFL, oflags | FASYNC);  
	
	
	while(1)
	{
		usleep(1000);
	}
	

	close(fd);

	return 0;
}
#endif
