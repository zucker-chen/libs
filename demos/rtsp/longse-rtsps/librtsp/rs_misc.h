/******************************************************************************
 * Copyright (C), 2008-2011, Grandstream Co., Ltd.        
 ******************************************************************************
 File Name     : gs_sensor.h
 Version       : Initial Draft
 Author        : Grandstream video software group
 Created       : 2009/11/19
 Last Modified :
 Description   : 服务程序
 Function List :
 Note		   : created 2009/12/11
 History       :
 1.Date        : 2009/12/11
   Author      : lwx
   Modification: 
 ******************************************************************************/
#ifndef __GS_MISC_H__
#define __GS_MISC_H__


#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/mount.h>
#include <sys/un.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <sys/msg.h>
#include <sys/time.h>
#include <sys/ipc.h>
#include <linux/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/sockios.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <ctype.h>
#include <time.h>
#include "rs_type.h"

#define NOW_TEST				(0)		//* 有文件，则为测试
#define NO_TEST					(-1)	//* 无文件，则不测试

#define TZ_FILE_PATH	"/etc/TZ"

#define 	SIP_CONF_FILE					"/data/etc/network/sip.conf"

#define TEST_TMP_FILE			"/tmp/testTmpFile"
#define ALM_SVR_PATH			"/tmp/alm_svr"
#define JPEG_ALM_PATH			"/tmp/jpeg_alm"


#define	UMOUNT_USB_FLAG_FILE		"/tmp/U_Usb" 
#define	UMOUNT_SD_FLAG_FILE			"/tmp/U_Sd"

#define 		DRV_USB			"/proc/scsi/usb-storage"
#define 		DRV_SD			"/dev/mmcblk0p1"

#define USB_MOUNT_PATH	"/opt/dm365/web/www/mnt/usb"
#define SD_MOUNT_PATH	"/opt/dm365/web/www/mnt/sd"
#define DISK_MOUNT_PATH	"/opt/dm365/web/www/mnt/disk"


#define PN_FILE			"/proc/gxvboard/dev_info/PN"
#define LANG_FILE		"/proc/gxvboard/dev_info/language"
#define _PATH_PROCNET_ROUTE		"/proc/net/route"

#define PN_GXV3651_HD		"96700016"
#define PN_CL3651_HD		"96701016"
#define PN_GXV3651_FHD		"96700022"
#define PN_CL3651_FHD		"96701022"

/* 还未定的P/N值 */
#define PN_GXV3662			"96700020"
#define PN_CL3662			"96701020"
#define PN_GXV3670			"96700021"	/* 乱写的 */
#define PN_CL3670			"96701021"

#define PN_GXV3500			"96500007"
#define PN_CL3500			"96501007"

#define PN_IP5150			"80100180"
#define PN_IP1200			"80100182"

#define PN_IP5150_N			"96500008"
#define PN_IP1200_N			"96700024"
#define PN_IP1200_IRC		"96700023"

#define MAIN_AUTO_FRAME						0x01
#define SUB_AUTO_FRAME						0x02

#define USB_DETECT_BIT						0x01		//flags bit udisk
#define SD_DETECT_BIT						0x02		//flags bit sd card
#define HARDDISK_DET_BIT					0x04		// flags bit hard disk
#define	NO_MASK								(-5)
#define MSG_MAX_LEN							32

#define MSG_PATH					'/tmp'
#define MSG_AUDIO_KEY				'e'


#define TZINFO_PATH			"/usr/share/zoneinfo/Etc/"
#define CMD_PATH			"/usr/share/zoneinfo/Etc/GMT"
#define TZ_DST				"/data/zone/tz"
#define LANGUAGE_CONF		"/data/etc/web_language.conf"
#define ENC_DEC_CONF		"/data/etc/encode_decode_type.conf"

#define ADD 		1		/* + 号 */
#define SUB			2		/* - 号 */
#define UNDEFINED	3

enum {
	MSG_TYPE_AUDIO = 0x01,
	MSG_TYPE_MAX
};

enum UsbSdUmountStat{
	USB_SD_IS_UMOUNT = 0x01,
	USB_SD_IS_MOUNT
}UsbSdUmountStat_e;

enum UmountErr{
	EN_UMOUNT_ERR_BUSY = 0x10,
	EN_UMOUNT_ERR_NOPOINT = 0x20,
	EN_UMOUNT_ERR_UNDEFINED 
}UmountErr_e;

enum {
	EN_TYPE_ENCODE	= 0x90,
	EN_TYPE_DECODE,
	EN_TYPE_UNKNOWN
};

typedef enum TimeType_{
	enIsDayLightUnknown = 0,
	enIsDayLightTime,
	enIsNormailTime,
}TimeType_e;

typedef struct UsbSdStat_s{
	int		usb;
	int 	sd;
}UsbSdStat_t;

typedef struct MsgData_s{
	long			msgType;
	int				id;
	char			buf[MSG_MAX_LEN];
}MsgData_t;

typedef struct device_msg_s{
	int	 iActive;
	char f_name[256];
	char mount_cmd[256];
	char umount_cmd[256];
}device_msg_t;

typedef struct sd_mmc_device_s{
	int				mountDevice;
	int				device_n;
	device_msg_t	device[12];
}sd_mmc_device_t;

typedef enum TimeZoneType_{
	enTimeZoneUserDefined	= 1,
	enTimeZoneNormail,
	enTimeZoneTypeMax
}TimeZoneType_e;

typedef struct TimeZontGmtOffset_s{
	int			iSign;
	int			iGmtOffsetHour;
	int			iGmtOffsetMin;
	char		*pStandardTZName;
}TimeZontGmtOffset_t;

typedef struct TimeZoneInfo_s{
	int		timeZoneOffset;				/* 时区偏移值 */
	int		offsetSign;					/* 主偏移方向 */
	int		isDayLightTime;				/* 是否设置夏令时标志 */
	int		dayLightTimeZoneOffset;		/* 夏令时时区偏移值 */
	int 	dayLightTimeZoneSign;	/* 夏令时时区偏移方向 */
	int 	startDayLightTimeMon;		/* 夏令时开始的月 		*/
	int		startDayLightTimeWeek;		/* 夏令时开始的周 		*/
	int		startDayLightTimeWeekDay;	/* 夏令时开始的星期几 	*/
	int 	endDayLightTimeMon;			/* 夏令时结束的月 		*/
	int		endDayLightTimeWeek;		/* 夏令时结束的周  		*/
	int		endDayLightTimeWeekDay;		/* 夏令时结束的星期几 	*/
}TimeZoneInfo_t;



int  Net_TcpRecvN(int sockFd, char *buf, int len);
int  Net_TcpSendN(int sockFd, char *buf, int size);
int  LocalSocketCreate(char *path);
int  LocalTcpSocketCreate (const char *filename);
int  Net_UdpRecvN(int sock, struct sockaddr* sockAddr, 
							size_t *addrLen, int recvLen,  void *recvBuf);
int  GetLocalIpAddr(signed   char* pIfName,signed   char* pIPAddr);
int  LocalSocketSend(char *path, int len, void *buf);
int  USB_SD_DeviceLockInit(void);
void USB_SD_DeviceLock(void);
void USB_SD_DeviceUnlock(void);
int  USB_SD_DeviceUmount(int device);
int  USB_SD_DeviceDetect(void);
int  USB_SD_SaveFile(int inMask, char *fileName, int fileLen, char *context);
int  LocalSocketClientCreate(char *path, struct sockaddr_un *svr);
int  GetMaxFrameRate(int width, int height);
int  GetFrameSkipBit(int mainFrameRate, int subFrameRate);


int _Msg_Init(char *path, char cKey);
int _Msg_Recv(int msgId, MsgData_t *msgData, int flag);
int _Msg_Send(int msgId, MsgData_t *msgData, int flag);
int _Msg_Del(int msgId);

u32	Time_GetMsec(void);
struct tm *Time_GetLocalTime(struct tm *pstTm);
struct tm* Time_GetTzTime( struct tm* pstTm, time_t *pTargetTime);

int Proc_GetProductId(void);
int Proc_GetOsdTextLang(void);
int Get_LanguageConf(void);
int ADC_CheckTest(void);
void LocalTimeLock(void);
void LocalTimeUnlock(void);
int update_tzenv_buf(void);
int update_dn(void);
int Device_GetIdName(char *name);
int Net_GetDeviceName(char *dev);
int Get_EncodeDecodeConf(void);
int UTIL_GetTagIntByName(char *pcTagName, unsigned int *pulTagVal);
unsigned int GetSipStreamNo(void);
int sock_close(int sock_fd);
int Thread_SetAttr(pthread_attr_t *pAttr, int pri);
int update_system_timezone(void);



#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __GS_MISC_H__ */

