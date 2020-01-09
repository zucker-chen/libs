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
#include "lensTimer.h"

static void Lens_TimerHandler(unsigned long data);
static struct lens_timer Lens_Timer;


#define LENS_PHASE_NUM				4				// 4拍1一个周期
#define LENS_PHASE_SPEED_HZ			300				// Hz 	//(1000000/300)	// us, 300~800Hz
#define LENS_ZOOM_STEP_MAXLEN		2318			// ZOOM最大步数
#define LENS_FOCUS_STEP_MAXLEN		3009			// FOCUS最大步数

volatile void *pGPIORegBase = 	NULL;
#define LENS_GPIO_BASE			0x120d0000
#define LENS_GPIO_DATA			(pGPIORegBase + 0x0)
#define LENS_GPIO_DIR			(pGPIORegBase + 0x400)
#define LENS_WRITE_REG(Addr, Value) ((*(volatile unsigned int *)(Addr)) = (Value))
#define LENS_READ_REG(Addr)         (*(volatile unsigned int *)(Addr))


/*
8拍方式的时序如下：
		A     B     C     D     E     F     G     H（时序）
A       1     1     0     0     0     0     0     1
A-      0     0     0     1     1     1     0     0
B       0     1     1     1     0     0     0     0
B-      0     0     0     0     0     1     1     1
*/

// bit0=A+, bit1=A-, bit2=B+, bit3=B-, 'A'为>0 'A`'为<0
static unsigned char LensStep_8P[] = {0x1, 0x5, 0x4, 0x6, 0x2, 0xA, 0x8, 0x9};	// 8拍正向	A -> AB -> B -> A`B -> A` -> A`B` -> B` -> AB`
static unsigned char LensStep_4P[] = {0x9, 0x5, 0x6, 0xA};	// 双8拍正向 	AB` -> AB -> A`B -> A`B`
static unsigned char Lens_Zoom_LastPhase = 0, Lens_Focus_LastPhase = 0;	// 0~(LENS_PHASE_NUM-1), 记录上一次停留的电机节拍位置
static struct completion sigLensComp;
static LENS_CTRL_ST stCtrl;
LENS_STATE_E eLensState = LENS_MOTORSTOP;



static int LENS_ZoomCtrl_OneStep(LENS_CTRL_E eFocusType)
{
	int nGpioGrp = 0, nValue = 0, i = 0;
	unsigned char *pLensStep_Tab = NULL;

	pLensStep_Tab = (LENS_PHASE_NUM == 8) ? &LensStep_8P[0] : &LensStep_4P[0];
	nGpioGrp = 8;
	switch (eFocusType)
	{
		case LENS_ZOOMWIDE:
		{
			i = Lens_Zoom_LastPhase = (Lens_Zoom_LastPhase+1)%LENS_PHASE_NUM;
			nValue = ((pLensStep_Tab[i] & 0x1) << 2) | (pLensStep_Tab[i] & 0x2) | ((pLensStep_Tab[i] & 0x4) >> 2) | (pLensStep_Tab[i] & 0x8);
			LENS_WRITE_REG(LENS_GPIO_DATA + (nGpioGrp << 12) + 0x3c, nValue);
			//printk("fun = %s, line = %d, nValue = %d,  i = %d, Lens_Zoom_LastPhase = %d\n",__FUNCTION__,__LINE__, nValue, i, Lens_Zoom_LastPhase);
			break;
		}
		case LENS_ZOOMTELE:
		{
			i = Lens_Zoom_LastPhase = (Lens_Zoom_LastPhase+(LENS_PHASE_NUM-1))%LENS_PHASE_NUM;
			nValue = ((pLensStep_Tab[i] & 0x1) << 2) | (pLensStep_Tab[i] & 0x2) | ((pLensStep_Tab[i] & 0x4) >> 2) | (pLensStep_Tab[i] & 0x8);
			LENS_WRITE_REG(LENS_GPIO_DATA + (nGpioGrp << 12) + 0x3c, nValue);
			//printk("fun = %s, line = %d, nValue = %d,  i = %d, Lens_Zoom_LastPhase = %d\n",__FUNCTION__,__LINE__, nValue, i, Lens_Zoom_LastPhase);
			break;
		}
		default:
		{
			printk("fun = %s line = %d %d unknown type!!!\n",__FUNCTION__,__LINE__,eFocusType);
			break;
		}
	}

	return 0;
}

static int LENS_FocusCtrl_OneStep(LENS_CTRL_E eFocusType)
{
	int nGpioGrp = 0, nValue = 0, i = 0;
	unsigned char *pLensStep_Tab = NULL;

	pLensStep_Tab = (LENS_PHASE_NUM == 8) ? &LensStep_8P[0] : &LensStep_4P[0];
	nGpioGrp = 6;
	switch (eFocusType)
	{
		case LENS_FOCUSNEAR:
		{
			i = Lens_Focus_LastPhase = (Lens_Focus_LastPhase+1)%LENS_PHASE_NUM;
			#if 1 // Focus飞线硬件用
			// A+ = gpio9_4, A- = gpio9_5, B+ = gpio10_0, B- = gpio10_1;
			nGpioGrp = 9;
			nValue = (pLensStep_Tab[i] & 0x3) << 4;
			LENS_WRITE_REG(LENS_GPIO_DATA + (nGpioGrp << 12) + 0xc0, nValue);
			nGpioGrp = 10;
			nValue = (pLensStep_Tab[i] & 0xc) >> 2;
			LENS_WRITE_REG(LENS_GPIO_DATA + (nGpioGrp << 12) + 0x0c, nValue);
			#else
			nValue = ((pLensStep_Tab[i] & 0x1) << 1) | ((pLensStep_Tab[i] & 0x2) >> 1) | (pLensStep_Tab[i] & 0xc);
			LENS_WRITE_REG(LENS_GPIO_DATA + (nGpioGrp << 12) + 0x3c, nValue);
			#endif
			//printk("fun = %s, line = %d, nValue = %d,  i = %d, Lens_Focus_LastPhase = %d\n",__FUNCTION__,__LINE__, nValue, i, Lens_Focus_LastPhase);
			break;
		}
		case LENS_FOCUSFAR:
		{
			i = Lens_Focus_LastPhase = (Lens_Focus_LastPhase+(LENS_PHASE_NUM-1))%LENS_PHASE_NUM;
			#if 1 // Focus飞线硬件用
			// A+ = gpio9_4, A- = gpio9_5, B+ = gpio10_0, B- = gpio10_1;
			nGpioGrp = 9;
			nValue = (pLensStep_Tab[i] & 0x3) << 4;
			LENS_WRITE_REG(LENS_GPIO_DATA + (nGpioGrp << 12) + 0xc0, nValue);
			nGpioGrp = 10;
			nValue = (pLensStep_Tab[i] & 0xc) >> 2;
			LENS_WRITE_REG(LENS_GPIO_DATA + (nGpioGrp << 12) + 0x0c, nValue);
			#else
			nValue = ((pLensStep_Tab[i] & 0x1) << 1) | ((pLensStep_Tab[i] & 0x2) >> 1) | (pLensStep_Tab[i] & 0xc);
			LENS_WRITE_REG(LENS_GPIO_DATA + (nGpioGrp << 12) + 0x3c, nValue);
			#endif
			//printk("fun = %s, line = %d, nValue = %d,  i = %d, Lens_Focus_LastPhase = %d\n",__FUNCTION__,__LINE__, nValue, i, Lens_Focus_LastPhase);
			break;
		}
		default:
		{
			printk("fun = %s line = %d %d unknown type!!!\n",__FUNCTION__,__LINE__,eFocusType);
			break;
		}
	}
	
	return 0;
}


static void LENS_ZoomFocus_Stop(void)
{
	int nGpioGrp = 0;

	// ========= zoom ===========
	nGpioGrp = 8;
	LENS_WRITE_REG(LENS_GPIO_DATA + (nGpioGrp << 12) + 0x3c, 0x0);

	// ========= focus ===========
	#if 1 // Focus飞线硬件用
	nGpioGrp = 9;
	LENS_WRITE_REG(LENS_GPIO_DATA + (nGpioGrp << 12) + 0xc0, 0x0);
	nGpioGrp = 10;
	LENS_WRITE_REG(LENS_GPIO_DATA + (nGpioGrp << 12) + 0x0c, 0x0);
	#else
	nGpioGrp = 6;
	LENS_WRITE_REG(LENS_GPIO_DATA + (nGpioGrp << 12) + 0x3c, 0x0);
	#endif
}

/// @return timer interval(us)
static int LENS_SpeedUpDown(LENS_CTRL_ST *pstCtrl, unsigned int unCurStepZoom, unsigned int unCurStepFocus)
{
	static int nSpeedMin = LENS_PHASE_SPEED_HZ;			// Hz
	static int nSpeedMax = LENS_PHASE_SPEED_HZ + 500;
	static int nIncGop = 500 / 32;
	static int nResultSpeed = 0;
	static int nValidMotor = 0;							// 1 = zoom, 2 = focus
	static int nValidStep = 0;

	if (pstCtrl != NULL)	// init by ioctl
	{
		nValidMotor = 0;
		nValidStep = 0;
		if (pstCtrl->unZoomStep > 0 && pstCtrl->unFocusStep == 0) 	// only zoom
		{
			nValidMotor = 1;
			nValidStep = pstCtrl->unZoomStep;
		}
		else if (pstCtrl->unZoomStep == 0 && pstCtrl->unFocusStep > 0) 	// only focus
		{
			nValidMotor = 2;
			nValidStep = pstCtrl->unZoomStep;
		}
		else if (pstCtrl->unZoomStep > 0 && pstCtrl->unFocusStep > 0) 	// zoom and focus
		{
			nValidMotor = pstCtrl->unZoomStep > pstCtrl->unFocusStep ? 2 : 1;
			nValidStep = pstCtrl->unZoomStep > pstCtrl->unFocusStep ? pstCtrl->unFocusStep : pstCtrl->unZoomStep;	// MIN step
		}
		
		return 0;
	}
	else
	{
		static unsigned int unCurStep = 0;

		if (nValidMotor == 0)
		{
			return 1000000 / nSpeedMin;
		}
		
		unCurStep = nValidMotor == 1 ? unCurStepZoom : unCurStepFocus;
		if (unCurStep <= nValidStep>>1)  // speed up
		{
			nResultSpeed = (nSpeedMin + unCurStep * nIncGop) >= nSpeedMax ? nSpeedMax : (nSpeedMin + unCurStep * nIncGop);
		}
		else							// speed down
		{
			nResultSpeed = (nSpeedMin + (nValidStep - unCurStep) * nIncGop) >= nSpeedMax ? nSpeedMax : (nSpeedMin + (nValidStep - unCurStep) * nIncGop);
		}

		return 1000000 / nResultSpeed;
	}

	return nSpeedMin;
}

static int LENS_Release( struct inode *node, struct file *filp )
{
	//chardev_fasync(-1, filp, 0);
	return 0;
}

static int LENS_Open( struct inode *node, struct file *filp )
{
	//printk(KERN_INFO "fun = %s line = %d\n",__FUNCTION__,__LINE__);
	//filp->private_data = mem_devp;
	return 0;
}

static long LENS_Ioctl (struct file *filp, unsigned int cmd, unsigned long arg)
{
	 int nRet = 0;
	 //LENS_CTRL_ST stCtrl;
	 //printk(KERN_INFO "fun = %s line = %d cmd = %d\n",__FUNCTION__,__LINE__,cmd);
	 switch(cmd) 
	 {
		 case LENS_START:
								break;
		 case LENS_MOTORCTRL:
								if (eLensState == LENS_MOTORRUN)
								{
									printk(KERN_WARNING " LENS_MOTORCTRL is running!!!\n\n");
									return -EINVAL;
								}
								nRet = copy_from_user(&stCtrl, (const void *)arg, sizeof(LENS_CTRL_ST));
								//LENS_FocusCtrl_OneStep(stCtrl.eFocusCtrlType);
								//LENS_ZoomCtrl_OneStep(stCtrl.eZoomCtrlType);
								LENS_SpeedUpDown(&stCtrl, 0, 0);
								lens_set_timer(&Lens_Timer, 1000000 / LENS_PHASE_SPEED_HZ);
								wait_for_completion(&sigLensComp);
								eLensState = LENS_MOTORSTOP;
								stCtrl.unZoomStep = 0;
								stCtrl.unFocusStep = 0;
								break;
		 case LENS_REGSET:
							  	break;
		 case LENS_REGGET:
								break;
		 case LENS_PRINTF:
								break;
		 case LENS_PI_LED_ON:
								break;
		 case LENS_PI_LED_OFF:
								break; 							 
		 default:
			 printk(KERN_WARNING " LENS_Ioctl unknown cmd!!!\n\n");
			 return -EINVAL;
	 }
	//printk("####done = %d\n",comp.done);
	 return nRet;
}

static int LENS_Fasync(int fd, struct file * filp, int on) 
{

    return 0;
}


static const struct file_operations LENS_cdev_fops =
{
	.owner 		= THIS_MODULE,
	.open 		= LENS_Open,
	.release 	= LENS_Release,
	.unlocked_ioctl		= LENS_Ioctl,
	.fasync = LENS_Fasync,
	//.mmap = LENS_mmap,
};

static struct miscdevice LENS_dev = {
	MISC_DYNAMIC_MINOR,
	"LensDrv",
	&LENS_cdev_fops,
};


static void Lens_TimerHandler(unsigned long data)
{
	//printk("============= Lens_TimerHandler!\n");
	lens_del_timer(&Lens_Timer);

	//printk("fun = %s line = %d  stCtrl.unZoomStep = %d, stCtrl.unFocusStep = %d\n",__FUNCTION__,__LINE__,stCtrl.unZoomStep,stCtrl.unFocusStep);
	if (stCtrl.unZoomStep > 0 || stCtrl.unFocusStep > 0)
	{
		static unsigned long interval = 0;
		eLensState = LENS_MOTORRUN;
		interval = LENS_SpeedUpDown(NULL, stCtrl.unZoomStep, stCtrl.unFocusStep);
		lens_set_timer(&Lens_Timer, interval);
		//lens_set_timer(&Lens_Timer, LENS_PHASE_SPEED);
	}
	else
	{
		complete(&sigLensComp);
		LENS_ZoomFocus_Stop();	
	}

	if (stCtrl.unZoomStep > 0)
	{
		LENS_ZoomCtrl_OneStep(stCtrl.eZoomCtrlType);
		stCtrl.unZoomStep--;
	}
	if (stCtrl.unFocusStep > 0)
	{
		LENS_FocusCtrl_OneStep(stCtrl.eFocusCtrlType);
		stCtrl.unFocusStep--;
	}
	
}


static int __init LENS_Init(void)
{
	int nStatus = 0;
	int nGpioGrp = 0, nValue = 0;
	printk(KERN_INFO "fun = %s line = %d\n",__FUNCTION__,__LINE__);
	
	nStatus = misc_register(&LENS_dev);
    if(nStatus)
    {
        printk("could not register alarm devices. \n");
        return nStatus;
    } 

    Lens_Timer.function = Lens_TimerHandler;
    nStatus = lens_timer_init(&Lens_Timer);
    if(nStatus)
    {
        printk("could not register alarm devices. \n");
        return nStatus;
    } 

	//lens_set_timer(&Lens_Timer, 2*1000*1000);

	init_completion(&sigLensComp);

    if (!pGPIORegBase)
    {
        pGPIORegBase = (volatile void *)ioremap_wc(LENS_GPIO_BASE, 0xc000);
        if (!pGPIORegBase)
        {
            printk("osal_ioremap_nocache err. \n");
            return -1;
        }
    }
	nGpioGrp = 6;
    nValue = LENS_READ_REG(LENS_GPIO_DIR + (nGpioGrp << 12));
    LENS_WRITE_REG(LENS_GPIO_DIR + (nGpioGrp << 12), (nValue&0xf0) | 0x0f);	// set gpio6_0~3 out
	LENS_WRITE_REG(LENS_GPIO_DATA + (nGpioGrp << 12) + 0x3c, 0x0);
	nGpioGrp = 8;
    nValue = LENS_READ_REG(LENS_GPIO_DIR + (nGpioGrp << 12));
    LENS_WRITE_REG(LENS_GPIO_DIR + (nGpioGrp << 12), (nValue&0xf0) | 0x0f);	// set gpio8_0~3 out
	LENS_WRITE_REG(LENS_GPIO_DATA + (nGpioGrp << 12) + 0x3c, 0x0);
#if 1 // Focus飞线硬件用
	nGpioGrp = 9;
    nValue = LENS_READ_REG(LENS_GPIO_DIR + (nGpioGrp << 12));
    LENS_WRITE_REG(LENS_GPIO_DIR + (nGpioGrp << 12), (nValue&0xcf) | 0x30);	// set gpio9_4~5 out
	LENS_WRITE_REG(LENS_GPIO_DATA + (nGpioGrp << 12) + 0xc0, 0x0);
	nGpioGrp = 10;
    nValue = LENS_READ_REG(LENS_GPIO_DIR + (nGpioGrp << 12));
    LENS_WRITE_REG(LENS_GPIO_DIR + (nGpioGrp << 12), (nValue&0xfc) | 0x03);	// set gpio8_0~1 out
	LENS_WRITE_REG(LENS_GPIO_DATA + (nGpioGrp << 12) + 0xc, 0x0);
#endif
	LENS_ZoomFocus_Stop();	

	printk("lens driver init successfully!\n");
	
	return nStatus;
}

static void __exit LENS_Exit(void)
{
	//kfree(mem_devp); 

    lens_del_timer(&Lens_Timer);
    lens_timer_destory(&Lens_Timer);

	misc_deregister(&LENS_dev);
	LENS_ZoomFocus_Stop();	
	printk("lens driver exit successfully!\n");
}

module_init(LENS_Init);
module_exit(LENS_Exit);
MODULE_AUTHOR("CZQ");
MODULE_LICENSE("GPL");



