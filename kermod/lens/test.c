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

typedef enum
{
	LENS_ZOOMTELE  = 1,
	LENS_ZOOMWIDE = 2,
	LENS_FOCUSNEAR = 3,
	LENS_FOCUSFAR  = 4,
	LENS_CTRLBUTT,
} LENS_CTRL_E;


typedef struct lensCtrl
{
	LENS_CTRL_E eZoomCtrlType;
	LENS_CTRL_E eFocusCtrlType;
	unsigned int unZoomStep;
	unsigned int unFocusStep;
	unsigned short usRegAddr;
	unsigned short usRegValue;
}LENS_CTRL_ST;


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


int HAL_LENS_SetZoomFocus(LENS_CTRL_E enZoomCtrlType,unsigned int unZoomStep,LENS_CTRL_E enFocusCtrlType,unsigned int unFocusStep)
{
	LENS_CTRL_ST stCtrl = {0};
	int nRet = 0;
	static int snFocusRunCnt = 0;
	static int snZoomRunCnt = 0;
    int sLensDrvFd = 0;
	stCtrl.eZoomCtrlType = enZoomCtrlType;
	stCtrl.eFocusCtrlType = enFocusCtrlType;
	stCtrl.unZoomStep = unZoomStep;
	stCtrl.unFocusStep = unFocusStep;

	if(sLensDrvFd == 0)
	{
		sLensDrvFd = open("/dev/LensDrv", O_RDWR, 777);
		if(sLensDrvFd < 0)
		{
			perror("#open dev");
			return -1;
		}
	}

	nRet = ioctl(sLensDrvFd, LENS_MOTORCTRL, &stCtrl);
	if(nRet < 0)
	{
		perror("HAL_LENS_SetZoomFocus #ioctl");
	}

	//if(g_ucPrintEn == 1)
	{
		printf("fun = %s line = %d type = %d %d unStep = %d %d\n",__FUNCTION__,__LINE__,enZoomCtrlType,enFocusCtrlType,unZoomStep,unFocusStep);
	}
	
    close(sLensDrvFd);
    
	return 0;
}

int main (int argc, char *argv[])
{
	LENS_CTRL_ST stCtrl;
	
	int fd = 0,nRet;
	
	fd = open("/dev/LensDrv", O_RDWR, 0);
	if(fd < 0)
	{
		perror("#open dev");
		return -1;
	}
	
    printf("#### argv[1] = %s, argv[2] = %s\n", argv[1], argv[2]);
    int type = atoi(argv[1]);
    switch (type)
    {
        case 1:     
        case 2:     
            //printf("=====1 2 \n");
            HAL_LENS_SetZoomFocus(atoi(argv[1]), atoi(argv[2]), 3, 0);
            break;
        case 3:     
        case 4:     
            HAL_LENS_SetZoomFocus(1, 0, atoi(argv[1]), atoi(argv[2]));
            break;
        case 5:     
        {
            int sLensDrvFd = open("/dev/LensDrv", O_RDWR, 777);
            
            stCtrl.usRegAddr = 0x00;  // iris
            stCtrl.usRegValue = atoi(argv[2]);
            nRet = ioctl(sLensDrvFd, LENS_REGSET, &stCtrl);
            close(sLensDrvFd);
            if(nRet < 0)
            {
                perror("LENS_REGSET #ioctl");
            }
            break;
        }
        default:
            printf("=====default \n");
            break;
    }
    
	close(fd);

	return 0;
}

