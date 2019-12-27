/******************************************************************************
 * Copyright (C), 2008-2011, Grandstream Co., Ltd.        
 ******************************************************************************
 File Name     : gs_rtsp.h
 Version       : Initial Draft
 Author        : Grandstream video software group
 Created       : 2009/11/19
 Last Modified :
 Description   : 
 Function List :
 Note		   : created 
 History       :
 1.Date        : 2009/11/23
   Author      : lwx
   Modification: 
 ******************************************************************************/

#ifndef __GS_RTSP_H__
#define __GS_RTSP_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */


#include "rs_rtp.h"
#include "rs_rtsp_common.h"

#define UCTRL_HEAD "MCTP/1.0"


typedef enum RtspSvr_Codec_s
{
	RTSP_CODEC_NONE = 0,
	RTSP_CODEC_MPEG4,
	RTSP_CODEC_H264,
	RTSP_CODEC_H265,
	RTSP_CODEC_G711A = 100,
	RTSP_CODEC_G711U,
	RTSP_CODEC_AAC,
} RtspSvr_Codec_t;

typedef struct RtspSvr_MediaInfo_s
{
	int nHaveVideo;
	RtspSvr_Codec_t eVideoType;
	int nVFramerate; 			// 25000 for 25fps
	int nVBitrate;
	int nVWidth;
	int nVHeight;
	int nHaveAudio;
	RtspSvr_Codec_t eAudioType;
	int nASamplerate;
	int nABitrate;
	int nAChannelNum;
} RtspSvr_MediaInfo_t;


typedef enum RtspSvr_StreamType {
	RTSP_STREAM_VIDEO = RTP_STREAM_VIDEO,
	RTSP_STREAM_AUDIO = RTP_STREAM_AUDIO,
	RTSP_STREAM_MAX = RTP_STREAM_MAX
} RtspSvr_StreamType_e;

typedef struct RtspSvr_Pkg_s
{
	RtspSvr_StreamType_e 	eStreamType;
	int						uKeyFrame;		// 0: P, 1:IDR, 2:SPS, 3:PPS
	int 		  			nLen;
	long long				llPts;
	char					pData[0];
} RtspSvr_Pkg_t;


typedef struct RtspSvr_s{
	int 				rtspSvrSockFd;
	int 				rtspSvrPort;
	VodSvrState_e		vodSvrState;
	RtpUdpSender_t		*pRtpUdpSender[MAX_VOD_CHN][RTP_STREAM_MAX];
	RtspSession_t		rtspSessWorkList;
	int					clientCnt[MAX_VOD_CHN];
	VodSvrType_e		svrType;
	int 				udpSendActive;
	void				*ringbufHandle;				// ringbuf handle
	pthread_mutex_t		rtspListMutex;
}RtspSvr_t;

void RTSP_CloseAllClient(void);
int  RTSP_GetClientNum(int chn);
int  RTSP_CheckChn(int chn);
//void RTSP_ParamsInit(Cfg_t *pCfg);
void RTSP_ParamsInit();
int	 RTSP_ThrStart(pthread_t *pid, void *rbHandle);
int  RTSP_GetUdpSendStat(int chn);
void RTSP_SetUdpSendActive(int chn, int isActive);
int  RTSP_ThrStop(void);
void RTSP_SessListLock(void );
void RTSP_SessListUnlock(void );
void RTSP_CloseAllClient(void);
RtspSession_t *RTSP_GetSessPtrBySessId(char *id);




#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __GS_RTSP_H__ */

