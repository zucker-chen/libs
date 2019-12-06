#ifndef __LENS_DRV__
#define __LENS_DRV__


#define LENS_START		0x1 //开始电机控制处理
#define LENS_MOTORCTRL			0x3 //	电机控制
#define LENS_REGSET			0x4	//寄存器值设置
#define LENS_REGGET			0x5	//寄存器值获取
#define LENS_PI_LED_ON		0x6	//PI LED 打开，可以找PI
#define LENS_PI_LED_OFF		0x7	//PI LED 关闭
#define LENS_PRINTF			0X10 //打印开关

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
	LENS_IDLE = 4,
	LENS_MOTORBUTT,
}LENS_STATE_E;

typedef struct lensCtrl
{
	LENS_CTRL_E eZoomCtrlType;
	LENS_CTRL_E eFocusCtrlType;
	unsigned int unZoomStep;
	unsigned int unFocusStep;
}LENS_CTRL_ST;



#endif
