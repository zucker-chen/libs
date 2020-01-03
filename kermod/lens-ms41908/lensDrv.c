#include <linux/init.h>
#include <linux/module.h>
#include <linux/ioctl.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/compat.h>
#include <linux/delay.h>
#include <linux/spi/spi.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>
#include <linux/mm.h>
#include <linux/completion.h>
#include "lensDrv.h"
#include "lensMs41908.h"
#include "lensTimer.h"
#include "lensGpio.h"

/*类型为struct timeval的时间相减，结果单位为ms*/
#define LENS_TIME_DIFF_MS(tv_new,tv_old) ((tv_new.tv_sec-tv_old.tv_sec)*1000+((tv_new.tv_usec-tv_old.tv_usec)/1000))

#define LEDA_ON	(1<<11)
#define LEDB_ON	(1<<11)
//DECLARE_COMPLETION(comp);  
struct completion comp;  
LENS_STATE_E g_eLensState = LENS_MOTORSTOP;

unsigned char ucRegisterAddr [] = {0x00,0x01,0x02,0x03,0x04,0x05,
								   0x0A,0x0B,0x0E,0x20,0x21,0x22,
								   0x23,0x24,0x25,0x27,0x28,0x29,
								   0x2A};
void *pTimerBase = NULL;
static struct fasync_struct *fasync_queue;
static unsigned char g_ucStartFlag =1;
struct timer_list g_stTimerList;
unsigned int g_unVdFlag = 0;
spinlock_t lens_lock;
struct mem_dev *mem_devp;
unsigned char g_eLensWriteFlag=0;
LENS_CTRL_ST stCtrlWrite;
int nLensPrintfFlag=0;
int nPILedOnFlag = 1;
int nZoomPILedOnStatus = 1;
int nFocusPILedOnStatus = 1;


struct timeval TimeIrqStartValue;
#if 0
static LENS_CTRL_ST stZoomCtrl = 
{
	.eCtrlType = LENS_ZOOMTELE,
	.unStep = 0,
};
static LENS_CTRL_ST stFocusCtrl = 
{
	.eCtrlType = LENS_FOCUSNEAR,
	.unStep = 0,
};
#endif

static LENS_CTRL_ST stCtrl = 
{
	.eZoomCtrlType = LENS_ZOOMTELE,
	.eFocusCtrlType = LENS_FOCUSNEAR,
	.unZoomStep = 0,
	.unFocusStep = 0,
};
struct mutex lensMutex;

DEFINE_MUTEX(lensMutex);

void LENS_Print(const char *pcFormat, ...)
{
	va_list ap;

	if(nLensPrintfFlag)
	{
		va_start(ap, pcFormat);
		vprintk(pcFormat, ap);
		va_end(ap);
	}
	
	return;
}

int LENS_GetInfo(PROCLENSINFO_OBJ *pLensInfo)
{
	if(pLensInfo == NULL)
	{
		return -1;
	}
	
	int i = 0;
	int nRet = 0;
	
	for(i = 0;i < sizeof(ucRegisterAddr);i++)
	{
		pLensInfo->ucRegisterInfo[i][0] = ucRegisterAddr[i];
		
		nRet = LENS_MS41908Read(ucRegisterAddr[i]);
		if(nRet >= 0)
		{
			pLensInfo->ucRegisterInfo[i][1] = nRet;
		}
		else
		{
			printk("%x register read error!!\n",ucRegisterAddr[i]);
			pLensInfo->ucRegisterInfo[i][1] = -1;
		}
	}
	
	return 0;
}

static int LENS_mmap(struct file*filp, struct vm_area_struct *vma)
{
      struct mem_dev *dev = filp->private_data; /*获得设备结构体指针*/

      vma->vm_flags |= VM_IO;
      vma->vm_flags |= (VM_DONTEXPAND);// | VM_DONTDUMP);
	  //vma->vm_page_prot=pgprot_noncached(vma->vm_page_prot);
      if (remap_pfn_range(vma,vma->vm_start,virt_to_phys(dev->data)>>PAGE_SHIFT, vma->vm_end - vma->vm_start, vma->vm_page_prot))
          return  -EAGAIN;

      return 0;
}

static int LENS_ZoomCtrl(LENS_CTRL_E eZoomType,unsigned int unZoomStep,unsigned char ucEnable)
{
	if(ucEnable)
	{
		switch (eZoomType)
		{
			case LENS_ZOOMTELE:
			{
				if(nPILedOnFlag)
				{
					LENS_MS41908Write(0x24, (0x0500 | unZoomStep | LEDB_ON));
					nZoomPILedOnStatus = 1;
				}
				else
				{
					LENS_MS41908Write(0x24, (0x0500 | unZoomStep));
					nZoomPILedOnStatus = 0;
				}
				break;
			}
			case LENS_ZOOMWIDE:
			{
				if(nPILedOnFlag)
				{
					LENS_MS41908Write(0x24,(0x0400 | unZoomStep | LEDB_ON));
					nZoomPILedOnStatus = 1;
				}
				else
				{
					LENS_MS41908Write(0x24,(0x0400 | unZoomStep));
					nZoomPILedOnStatus = 0;
				}
				
				break;
			}
			default:
			{
				printk("fun = %s line = %d %d unknown type!!!\n",__FUNCTION__,__LINE__,eZoomType);
				break;
			}
		}
	}
	else
	{
		if(nPILedOnFlag)
		{
			LENS_MS41908Write(0x24,0 | LEDB_ON);
			nZoomPILedOnStatus = 1;
		}
		else
		{
			LENS_MS41908Write(0x24,0);
			nZoomPILedOnStatus = 0;
		}
	}
		
	if(unZoomStep)
	{
		LENS_MS41908Write(0x25,39375/unZoomStep);
	}
	else
	{
		LENS_MS41908Write(0x25,0);
	}

	return 0;
}



static int LENS_FocusCtrl(LENS_CTRL_E eFocusType,unsigned int unFocusStep,unsigned char ucEnable)
{
	if(ucEnable)
	{
		switch (eFocusType)
		{
			case LENS_FOCUSNEAR:
			{
				if(nPILedOnFlag)
				{
					LENS_MS41908Write(0x29, (0x0500 | unFocusStep |LEDA_ON));
					nFocusPILedOnStatus = 1;
				}
				else
				{
					LENS_MS41908Write(0x29, (0x0500 | unFocusStep));
					nFocusPILedOnStatus = 0;
				}
				break;
			}
			case LENS_FOCUSFAR:
			{
				if(nPILedOnFlag)
				{
					LENS_MS41908Write(0x29, (0x0400 | unFocusStep|LEDA_ON));	 
					nFocusPILedOnStatus = 1;
				}
				else
				{
					LENS_MS41908Write(0x29, (0x0400 | unFocusStep));	 
					nFocusPILedOnStatus = 0;
				}
				break;
			}
			default:
			{
				printk("fun = %s line = %d %d unknown type!!!\n",__FUNCTION__,__LINE__,eFocusType);
				break;
			}
		}
		
	}
	else
	{
		if(nPILedOnFlag)
		{
			LENS_MS41908Write(0x29, 0|LEDA_ON);
			nFocusPILedOnStatus = 1;
		}
		else
		{
			LENS_MS41908Write(0x29, 0);
			nFocusPILedOnStatus = 0;
		}
	}
	
	if(unFocusStep)
	{
		LENS_MS41908Write(0x2a,39375/unFocusStep);
	}
	else
	{
		LENS_MS41908Write(0x2a,0);
	}
	
	
	return 0;
}



#if 0
//irqreturn_t LENS_TimerIrqHandle(int irq, void *dev_id)
void LENS_InspireTimer(unsigned long ulData)
{
	static int nTimeNow = 0,nTimeBefore = 0;
	nTimeNow = LENS_GetTime();
	printk("fun = %s line = %d now = %d before = %d diff = %d\n",__FUNCTION__,__LINE__,nTimeNow,nTimeBefore,nTimeNow-nTimeBefore);
	nTimeBefore = nTimeNow;
	
	if(1 == g_ucStartFlag)
	{
		//mutex_lock(&lensMutex);
		
		LENS_FocusCtrl(stFocusCtrl.eCtrlType,stFocusCtrl.unStep);
		LENS_ZoomCtrl(stZoomCtrl.eCtrlType,stZoomCtrl.unStep);
		
		if(g_unVdFlag == 1)
		{
			LENS_MS41908FZInspire();
		}
		
		if (fasync_queue)
		{
			printk("fun = %s line = %d !!!\n",__FUNCTION__,__LINE__);
			kill_fasync(&fasync_queue, SIGIO, POLL_IN);
		}

		g_unVdFlag = 0;
		//stZoomCtrl.unStep = 0;
		//stFocusCtrl.unStep = 0;
		//mutex_unlock(&lensMutex);
	}

	return ;
}

#else

static void LENS_MotorRun(void)
{
	static struct timeval FZValue,FZValueLast;
	time_t TimeValueTemp;
	#if 1
	//printk("@@@@@@####:g_eLensState = %d  zoom:%d focus:%d\n",g_eLensState,stCtrl.unZoomStep,stCtrl.unFocusStep);
	switch (g_eLensState)
	{
		case LENS_MOTORRUN:
					//printk("fun = %s line = %d %d LENS_MOTORRUN type!!!\n",__FUNCTION__,__LINE__,g_eLensState);
					g_eLensState = LENS_DISABLE;
					LENS_FocusCtrl(stCtrl.eFocusCtrlType,stCtrl.unFocusStep,1);
					LENS_ZoomCtrl(stCtrl.eZoomCtrlType,stCtrl.unZoomStep,1);
					//printk("@@@@@@####: zoom:%d focus:%d\n",stCtrl.unZoomStep,stCtrl.unFocusStep);
					LENS_MS41908FZInspire();
					do_gettimeofday(&FZValue);
					TimeValueTemp = LENS_TIME_DIFF_MS(FZValue,TimeIrqStartValue);
//					if (TimeValueTemp >= 5)
//						printk("FZStartValue:%lu\n",TimeValueTemp);
					TimeValueTemp = LENS_TIME_DIFF_MS(FZValue,FZValueLast);
					if (TimeValueTemp <= 39)
					{
						LENS_Print("FZValueDiff:%lu,new:%lu.%lu,old:%lu.%lu\n",TimeValueTemp,FZValue.tv_sec,
						FZValue.tv_usec,FZValueLast.tv_sec,FZValueLast.tv_usec);
					}			
					FZValueLast.tv_sec = FZValue.tv_sec;
					FZValueLast.tv_usec = FZValue.tv_usec;
					complete(&comp); 
					break;
		case LENS_MOTORSTOP:
					g_eLensState = LENS_IDLE;
					//printk("fun = %s line = %d %d LENS_MOTORSTOP type!!!\n",__FUNCTION__,__LINE__,g_eLensState);
					LENS_FocusCtrl(stCtrl.eFocusCtrlType,0,0);
					LENS_ZoomCtrl(stCtrl.eZoomCtrlType,0,0);
					break;
		case LENS_DISABLE:
					//printk("fun = %s line = %d %d LENS_DISABLE type!!!\n",__FUNCTION__,__LINE__,g_eLensState);
					g_eLensState = LENS_MOTORSTOP;
					LENS_FocusCtrl(stCtrl.eFocusCtrlType,0,1);
					LENS_ZoomCtrl(stCtrl.eZoomCtrlType,0,1);
					break;
		case LENS_IDLE:
					if(nZoomPILedOnStatus != nPILedOnFlag)
					{
						LENS_ZoomCtrl(stCtrl.eZoomCtrlType,0,0);
					}
					if(nFocusPILedOnStatus != nPILedOnFlag)
					{
						LENS_FocusCtrl(stCtrl.eFocusCtrlType,0,0);
					}
					break;
		default:
					printk("fun = %s line = %d %d unknown type!!!\n",__FUNCTION__,__LINE__,g_eLensState);
					break;
	}

	if (g_eLensWriteFlag)
	{
		g_eLensWriteFlag = 0;
		LENS_MS41908Write(stCtrlWrite.usRegAddr, stCtrlWrite.usRegValue);
		if (0x00 == stCtrlWrite.usRegAddr)
			LENS_MS41908IrisInspire();
	}
	#else
	LENS_MS41908Write(0x01,0X3E00);
	LENS_MS41908Read(0x01);
	#endif
	return;
}

atomic_t timerCnt;
irqreturn_t LENS_TimerIrqHandle(int irq, void *dev_id)
{	
	static struct timeval TimeIrqStartValueLast;
	time_t TimeValueTemp;
	do_gettimeofday(&TimeIrqStartValue);
	TimeValueTemp = LENS_TIME_DIFF_MS(TimeIrqStartValue,TimeIrqStartValueLast);
	if (TimeValueTemp <= 38 || TimeValueTemp >= 60)
	{
		LENS_Print("TimeIrqValue:%lu,new:%lu.%lu,old:%lu.%lu\n",TimeValueTemp,TimeIrqStartValue.tv_sec,
		TimeIrqStartValue.tv_usec,TimeIrqStartValueLast.tv_sec,TimeIrqStartValueLast.tv_usec);
	}
	TimeIrqStartValueLast.tv_sec = TimeIrqStartValue.tv_sec;
	TimeIrqStartValueLast.tv_usec = TimeIrqStartValue.tv_usec;
	
	unsigned long		flags;
	if(pTimerBase == NULL)
	{
		//printk("fun = %s pTimerBase == NULL!!\n",__FUNCTION__);
		return IRQ_HANDLED;
	}


	writel(0x64,pTimerBase+0x08); 
	writel(0xff,pTimerBase+0x0c);

	//LENS_Print("***************************PID: %d --->LENS_TimerIrqHandle\n", getpid());
	//if(1 == g_ucStartFlag)
	{
		#if 0
		static int nTimeNow = 0,nTimeBefore = 0;
		nTimeNow = LENS_GetTime();
		printk("fun = %s line = %d now = %d before = %d diff = %d\n",__FUNCTION__,__LINE__,nTimeNow,nTimeBefore,nTimeNow-nTimeBefore);
		nTimeBefore = nTimeNow;
		#endif
		
		spin_lock_irqsave(&lens_lock,flags);
		LENS_MotorRun();
		spin_unlock_irqrestore(&lens_lock,flags);
		
		if (fasync_queue)
		{
			//printk("fun = %s line = %d !!!\n",__FUNCTION__,__LINE__);
			kill_fasync(&fasync_queue, SIGIO, POLL_IN);
		}		
	}
	
	writel(7500,pTimerBase);
	writel(0xe4,pTimerBase+0x08); 
	
	return IRQ_HANDLED;

}

#endif
static int LENS_Fasync(int fd, struct file * filp, int on) 
{
	
    int retval;  
    //将该设备登记到fasync_queue队列中去
    retval=fasync_helper(fd,filp,on,&fasync_queue);  
    if(retval<0)    
	{
        return retval;    
	}

    return 0;
}

static int LENS_Release( struct inode *node, struct file *filp )
{
	//chardev_fasync(-1, filp, 0);
	return 0;
}

static int LENS_Open( struct inode *node, struct file *filp )
{
	printk(KERN_INFO "fun = %s line = %d\n",__FUNCTION__,__LINE__);
	filp->private_data = mem_devp;
	return 0;
}
static int LENS_Ioctl (struct file *filp, unsigned int cmd, unsigned long arg)
{
	unsigned long		flags;
	int nRet = 0;
	int nZoom = 0;
	int nFocus = 0;
	LENS_CTRL_ST stCtrlTemp;
	//printk(KERN_INFO "fun = %s line = %d cmd = %d\n",__FUNCTION__,__LINE__,cmd);
	switch(cmd) 
	{
		case LENS_START:
								g_ucStartFlag = 1;
								break;
		case LENS_MOTORCTRL:
								spin_lock_irqsave(&lens_lock,flags);
								//copy_from_user(&stZoomCtrl, (const void *)arg, sizeof(LENS_CTRL_ST));
								copy_from_user(&stCtrl, (const void *)arg, sizeof(LENS_CTRL_ST));
								g_eLensState = LENS_MOTORRUN;
								spin_unlock_irqrestore(&lens_lock,flags);
								wait_for_completion(&comp); 
								stCtrl.unZoomStep = 0;
								stCtrl.unFocusStep = 0;
								break;
		case LENS_REGSET:
								spin_lock_irqsave(&lens_lock,flags);
								copy_from_user(&stCtrlTemp, (const void *)arg, sizeof(LENS_CTRL_ST));
								stCtrlWrite.usRegAddr = stCtrlTemp.usRegAddr;
								stCtrlWrite.usRegValue = stCtrlTemp.usRegValue;
								g_eLensWriteFlag = 1;
								spin_unlock_irqrestore(&lens_lock,flags);
								//printk(KERN_WARNING " LENS_REGSET,usRegAddr:%d,usRegValue:%d\n",stCtrlTemp.usRegAddr,stCtrlTemp.usRegValue);
								break;
		case LENS_REGGET:
								if(copy_from_user(&stCtrlTemp, (const void *)arg, sizeof(LENS_CTRL_ST)))
									return - EFAULT;
								nRet = LENS_MS41908Read(stCtrlTemp.usRegAddr);
								if(nRet < 0)
								{
									printk(KERN_WARNING "LENS_REGGET error!\n");
									return -EINVAL;
								}
								stCtrlTemp.usRegValue = nRet;
								if(copy_to_user((int *)arg, &stCtrlTemp, sizeof(LENS_CTRL_ST)))
									return - EFAULT;
								//printk(KERN_WARNING " LENS_REGGET,usRegAddr:%d,usRegValue:%d\n",stCtrlTemp.usRegAddr,stCtrlTemp.usRegValue);
								break;
		case LENS_PRINTF:
								if(copy_from_user(&stCtrlTemp, (const void *)arg, sizeof(LENS_CTRL_ST)))
									return - EFAULT;
								nLensPrintfFlag = stCtrlTemp.usRegValue;
								printk("nLensPrintfFlag=%d\n",nLensPrintfFlag);
								break;
		case LENS_PI_LED_ON:
								nPILedOnFlag = 1;
								break;
		case LENS_PI_LED_OFF:
								nPILedOnFlag = 0;
								break;								
		default:
			printk(KERN_WARNING " LENS_Ioctl unknown cmd!!!\n\n");
			return -EINVAL;
	}
   //printk("####done = %d\n",comp.done);
	return nRet;
	
}
static const struct file_operations LENS_cdev_fops =
{
	.owner 		= THIS_MODULE,
	.open 		= LENS_Open,
	.release 	= LENS_Release,
	.unlocked_ioctl		= LENS_Ioctl,
	.fasync = LENS_Fasync,
	.mmap = LENS_mmap,
};

static struct miscdevice LENS_dev = {
  MISC_DYNAMIC_MINOR,
  "LensDrv",
   &LENS_cdev_fops,
};

static int __init LENS_Init(void)
{
	int nStatus = 0;
	printk(KERN_INFO "fun = %s line = %d\n",__FUNCTION__,__LINE__);
	spin_lock_init (&lens_lock);  
	init_completion(&comp); 
	nStatus = misc_register(&LENS_dev);
	
    if(nStatus)
    {
        printk("could not register alarm devices. \n");
        return nStatus;
    } 
	nStatus = LENS_MS41908Init();
	if(nStatus < 0)
	{
		printk("LENS_MS41908Init failed !!\n");
		
		return -1;
	}
#if 1
	atomic_set(&timerCnt, 0);
	//Lens_ProcInit();
	printk(KERN_INFO "fun = %s line = %d\n",__FUNCTION__,__LINE__);
	nStatus = LENS_TimerInit(LENS_TimerIrqHandle,&pTimerBase);
	if(nStatus < 0)
	{
		printk("LENS_TimerInit failed !!\n");
		
		return -1;
	}
#else
	memset(&g_stTimerList,0,sizeof(g_stTimerList));
	init_timer(&g_stTimerList);
	g_stTimerList.function = &LENS_InspireTimer;
	//g_stTimerList.expires = jiffies + 2;
	LENS_TimerSet(&g_stTimerList,4);
#endif

	printk("lens driver init successfully!\n");
	
	return nStatus;
}

static void __exit LENS_Exit(void)
{
	//kfree(mem_devp); 
	#if 1
	LENS_TimerDeinit();
	#else
	LENS_TimerDestory(&g_stTimerList);
	#endif
	//Lens_ProcDeinit();
	LENS_MS41908Deinit();
	misc_deregister(&LENS_dev);
	
	printk("lens driver exit successfully!\n");
}

module_init(LENS_Init);
module_exit(LENS_Exit);
MODULE_AUTHOR("XUEMD");
MODULE_LICENSE("GPL");
