/******************************************************************************
 * Copyright (C), 2008-2011, Grandstream Co., Ltd.        
 ******************************************************************************
 File Name     : gs_audio.h
 Version       : Initial Draft
 Author        : Grandstream video software group
 Created       : 2009/11/19
 Last Modified :
 Description   : 系统共用的数据类型定义
 Function List :
 Note		   : created 2009/12/10
 History       :
 1.Date        : 2009/12/10
   Author      : lwx
   Modification: 
 ******************************************************************************/
#ifndef __GS_TYPE_H__
#define __GS_TYPE_H__

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
//#include "rs_syslog.h"

/* h264编码器版本 */
//#define 	CODEC_01_00_00_18				1
//#define 	CODEC_01_02_00_05				1
//#define 	CODEC_02_00_00_04				1	//* 1080p@30fps
#define 	CODEC_02_10_00_05				1	//* h264enc codec 2.1.0.5

//#define 	OPEN_SYSLOG				
#define 	MAX_VOD_CHN	5

#define 	GS_FAIL					(-1)
#define 	GS_SUCCESS				(0)

typedef unsigned long			uLong;
typedef unsigned int 			u32;
typedef unsigned short 			u16;
typedef unsigned char 			u8;


#ifndef _M_IX86
typedef unsigned long long 		u64;
typedef long long				s64;
#else
typedef __init64				u64;
typedef __init64				s64;
#endif 

#define STOP_HERE(msg)	{printf("[%s]run here now\n",msg);while(1){sleep(1);}}

#define __printf		printf
//#define P_THR_MSG

enum {
	LEN_32   = 32,
	LEN_64   = 64,
	LEN_128  = 128,
	LEN_256  = 256,
	LEN_512  = 512,
	LEN_1K	 = 0x400,
	LEN_2K	 = 0x800,
	LEN_16K  = 0x4000,
	LEN_32K  = 0x8000,
	LEN_64K  = 0x10000,
	LEN_128K = 0x20000,
	LEN_256K = 0x40000,
	LEN_512K = 0x80000,
	LEN_1M	 = 0x100000,
	LEN_3M   = 0x300000,
	LEN_8M   = 0x800000
};


typedef enum DbgLevel{
	None = 0,
	Err ,
	Warn,
	Dbg ,
	Info
}DbgLevel_e;

typedef enum DbgType{
	DbgPerror = 1,
	DbgNoPerror
}DbgType_e;


//#ifndef OPEN_SYSLOG

#define OPEN_SYSLOG

#ifdef OPEN_SYSLOG

#if 0
#define rtsp_dbg( levels, type, fmt...) { \
	if(levels <= Info || levels > None) { \
		fprintf(stderr, "[File: %-15s, Func: %-20s, Line: %05d] :", __FILE__, \
							__FUNCTION__, __LINE__ ); \
		if(levels == Err){ \
			fprintf(stderr, "Err: \n");	\
		}else if(levels == Warn){	\
			fprintf(stderr, "Warn: \n");	\
		}else if (levels == Dbg){ \
			fprintf(stderr, "Dbg: \n"); \
		}else if (levels == Info){ \
			fprintf(stderr, "Info: \n"); \
		} \
		fprintf(stderr, "MODLE: "); \
		fprintf(stderr, MODLE_NAME); \
		fprintf(stderr, fmt); \
		fprintf(stderr, "\n"); \
		if(type == DbgPerror){ perror("Perror: ");} \
	} \
}

#endif


#define rtsp_dbg( levels, type, fmt...) do { \
	if(levels <= Info || levels > None) { \
		time_t ttt = time(NULL);		\
		struct tm *ptm = localtime(&ttt);	\
		fprintf(stderr, "time %04d-%02d-%02dT%02d:%02d:%02dZ [File: %-15s, Func: %-20s, Line: %05d] :", \
		ptm->tm_year+1900, ptm->tm_mon +1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec, \
			__FILE__, __FUNCTION__, __LINE__ ); \
			if(levels == Err){ \
			fprintf(stderr, "Err: \n");	\
		}else if(levels == Warn){	\
			fprintf(stderr, "Warn: \n");	\
		}else if (levels == Dbg){ \
			fprintf(stderr, "Dbg: \n"); \
		}else if (levels == Info){ \
			fprintf(stderr, "Info: \n"); \
		} \
		fprintf(stderr, "MODLE: "); \
		fprintf(stderr, MODLE_NAME); \
		fprintf(stderr, fmt); \
		fprintf(stderr, "\n"); \
		if(type == DbgPerror){ perror("Perror: ");} \
	} \
} while( 0 )

#else

#define rtsp_dbg( levels, type, fmt...) {}


#endif 

#define CLR(var)	(memset(&var, 0, sizeof(var)))

typedef enum ConType{
	CON_TYPE_RTSP = 0x200,
	CON_TYPE_UCTRL,
	CON_TYPE_UNDEFINED
}ConType_e;

typedef enum BooL{
	False = 0,
	True = 1
}Bools;

enum {
	IS_UNABLE = 0,
	IS_ENABLE
};

typedef enum VencType{
	VENC_TYPE_H264 = 0, 
	VENC_TYPE_MPEG4,
	VENC_TYPE_JPEG,
	VENC_TYPE_MAX, 
	VENC_TYPE_DUAL, 
	VENC_H264_MPEG4,
	VENC_H264_JPEG,
	VENC_MPEG4_JPEG,
	VENC_MPEG4_H264,
	VENC_JPEG_H264,
	VENC_JPEG_MPEG4,
	VENC_TYPE_PUTT
}VencType_e;

#define 	RSZ_CHN_A						0	/* RSZ 输出通道0 */
#define 	RSZ_CHN_B						1	/* RSZ 输出通道1 */
#define 	RSZ_CHN_C						2	/* RSZ 输出通道2 */
#define	RSZ_CHN_MAX					2	//lyh
#define	BUF_TIMEOUT					4 	/* 4秒超时 */
#define 	AUDIO_TYPE						97
#define	VENC_BUF_NUM					3	//lyh 3

typedef enum FrameType_{
	VENC_I_FRAME = 1,
	VENC_P_FRAME = 2, 
	VENC_UNDEFINED = -1
}FrameType_e;

typedef struct EncBuffer_s{
	int 		id;
	int 		outSize;
	int 		frameType;
	int		height;
	int 		width;
	int		isMd;
	u32		pt;
	char 		*outBuf;
}EncBuffer_t;

typedef struct TransEncBuf_s{
	int				cnt;
	EncBuffer_t		stream[VENC_BUF_NUM]; 
}TransEncBuf_t;

typedef struct OneChnTransBuf_s{
	TransEncBuf_t	channel;
}OneChnTransBuf_t;
typedef struct TwoChnTransBuf_s{
	TransEncBuf_t	channel[2];
}TwoChnTransBuf_t;
typedef struct ThreeChnTransBuf_s{
	TransEncBuf_t	channel[3];
}ThreeChnTransBuf_t;

enum {
	ID_OK = 0x10, 
	ID_SKIP,
	ID_MD,
	ID_STOP,
	ID_END, 
};

enum {
	enResolutionErr_None = 0,
	enResolutionErr_Width = 0x01,
	enResolutionErr_Height = 0x02,
	enResolutionErr_Other  = 0x04,
	enEncodeTypeErr        = 0x08,
	enFrameRateErr_JPEG	   = 0x10,
};

typedef struct Params_s{
	int 	bri;
	int		con;
	int		sat;
}Params_t;

#define FREE(p)  do {\
	printf("%s:%d:%s:free(0x%lx)\n", __FILE__, __LINE__, __func__, (unsigned long)p);\
	if (p) {\
		free(p);\
		p = NULL;}\
} while (0)

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __GS_TYPE_H__ */


