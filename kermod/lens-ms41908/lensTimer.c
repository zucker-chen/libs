#include <linux/init.h>
#include "lensTimer.h"


#define TIMER3_BASE   0x20010020
#define SCCTRL_BASE   0x20050000
#define TIMER_IRQ 36

static void *pTimerBase;
static void *pScctrlBase;
static unsigned long ulDev;


int LENS_GetTime(void)
{
	struct timeval tm;
	do_gettimeofday(&tm);
	
	return  tm.tv_sec*1000+tm.tv_usec/1000;
	
}

#if 0
int LENS_TimerInit(struct timer_list *pTimer)
{
	if(pTimer == NULL)
	{
		printk("fun = %s invalid param!!\n",__FUNCTION__);
		
		return -1;
	}
	
	init_timer(pTimer);
	
	return 0;
}

int LENS_TimerSet(struct timer_list *pTimer,unsigned long ulInterval)
{
	if(pTimer == NULL)
	{
		printk("fun = %s invalid param!!\n",__FUNCTION__);
		
		return -1;
	}
	
	 return mod_timer(pTimer, jiffies + ulInterval);
}

int LENS_TimerDestory(struct timer_list *pTimer)
{
	return del_timer(pTimer);
}

#else
	
int LENS_TimerInit(irq_handler_t handler,void **ppTimerBase)
{
	int nRet = 0; 
	int nData = 0;
	
	if(handler == NULL)
	{
		printk("fun = %s invalid param!!\n",__FUNCTION__);
		
		return -1;
	}

	#if 1
	pTimerBase = ioremap(TIMER3_BASE,0x20);
	pScctrlBase = ioremap(SCCTRL_BASE,0x04);

	if((pTimerBase == NULL) || (pScctrlBase == NULL))
	{
		return -1;
	}
	
	//时钟频率�?Mhz，预分频16，计算可得要定时40ms，初值为7500
	writel(7500,pTimerBase); 
	writel(0xffffffff,pTimerBase+0x18); 
	nData = readl(pScctrlBase);
	writel(nData & (~(1<<22)),pScctrlBase);
	writel(0xe4,pTimerBase+0x08); 
	
	*ppTimerBase = pTimerBase;
	#else
 		writel(7500, IO_ADDRESS(TIMER3_BASE));
	#endif

	unsigned long irqfrag = IRQF_ONESHOT;
	nRet = request_threaded_irq(TIMER_IRQ, NULL, handler,irqfrag, "timer3", (void *)&ulDev);
	//nRet = request_irq(TIMER_IRQ, handler,0x15200, "timer3", (void *)&ulDev);	// IRQF_SHARED 0x15200
	if (nRet != 0)
	{
		 printk("could not request_irq.nRet = %d \n",nRet);
		 return -1;
	}
	
	//*ppTimerBase = pTimerBase;
	
	return 0;
}

int LENS_TimerDeinit(void)
{
	free_irq(TIMER_IRQ,(void *)&ulDev);	
}

#endif