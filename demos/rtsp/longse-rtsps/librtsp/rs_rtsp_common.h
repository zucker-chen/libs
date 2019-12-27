/******************************************************************************
 * Copyright (C), 2008-2011, Grandstream Co., Ltd.        
 ******************************************************************************
 File Name     : gs_rtsp_common.h
 Version       : Initial Draft
 Author        : Grandstream video software group
 Created       : 2009/11/19
 Last Modified :
 Description   : 
 Function List :
 Note		   : created 2009/11/19
 History       :
 1.Date        : 2009/11/19
   Author      : lwx
   Modification: 
 ******************************************************************************/

#ifndef __GS_RTSP_COMMON_H__
#define __GS_RTSP_COMMON_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>  
#include <errno.h>

#include "rs_type.h"
#include "linux_list.h"
//#include "rs_cfg.h"
#include "rs_type.h"

#define 	DEFAULT_RTSP_PORT			554
#define 	RTSP_PORT_OFFSET			2000
#define 	MAX_RTP_LEN					1400 //(1400)
#define 	MAX_RTSP_BUF_LEN			4096
#define 	MAX_VOD_CHN					5	/* 最大点播通道，一路主码流，一路次码流 */

#define RTSP_PARSE_OK 					0
#define RTSP_PARSE_IS_RESP  			-4
#define RTSP_PARSE_INVALID_OPCODE	 	-1
#define RTSP_PARSE_INVALID 				-2
#define RTSP_PARSE_ISNOT_RESP 			-3
#define RTSP_BAD_STATUS_BEGIN 			202

#define RTSP_STATUS_CONTINUE            100
#define RTSP_STATUS_OK                  200
#define RTSP_STATUS_ACCEPTED            202
#define RTSP_STATUS_BAD_REQUEST         400
#define RTSP_STATUS_METHOD_NOT_ALLOWED  405

#define NAL_TYPE_SLICE      	1
#define NAL_TYPE_IDR        	5
#define NAL_TYPE_SEI        	6
#define NAL_TYPE_SPS        	7
#define NAL_TYPE_PPS        	8
#define NAL_TYPE_SEQ_END    	9
#define NAL_TYPE_STREAM_END 10
#define H264_STARTCODE_LEN    4 /* 00 00 00 01 */
#define	NAL_FRAGMENTATION_SIZE		1024 //1024	/*H264码流的分包单位长度为1024*/

#define MPEG4_STARTCODE_LEN  3 /*00 00 01*/
#define MPEG4_VO 		0x01
#define MPEG4_VOL       0X20
#define MPEG4_FRAME  0XB6

#define RTSP_VERSION  			"RTSP/1.0"
#define RTSP_LRLF 				"\r\n"


#define RTSP_METHOD_SETUP      "SETUP"
#define RTSP_METHOD_REDIRECT   "REDIRECT"
#define RTSP_METHOD_PLAY       "PLAY"
#define RTSP_METHOD_PAUSE      "PAUSE"
#define RTSP_METHOD_SESSION    "SESSION"
#define RTSP_METHOD_RECORD     "RECORD"
#define RTSP_METHOD_EXT_METHOD "EXT-"
#define RTSP_METHOD_OPTIONS    "OPTIONS"
#define RTSP_METHOD_DESCRIBE   "DESCRIBE"
#define RTSP_METHOD_GET_PARAM  "GET_PARAMETER"
#define RTSP_METHOD_SET_PARAM  "SET_PARAMETER"
#define RTSP_METHOD_TEARDOWN   "TEARDOWN"
#define RTSP_METHOD_INVALID	   "Invalid Method"

/* message header keywords */
#define RTSP_HDR_CONTENTLENGTH 		"Content-Length"
#define RTSP_HDR_ACCEPT 			"Accept"
#define RTSP_HDR_ALLOW 				"Allow"
#define RTSP_HDR_BLOCKSIZE 			"Blocksize"
#define RTSP_HDR_CONTENTTYPE 		"Content-Type"
#define RTSP_HDR_DATE 				"Date"
#define RTSP_HDR_REQUIRE 			"Require"
#define RTSP_HDR_TRANSPORTREQUIRE 	"Transport-Require"
#define RTSP_HDR_SEQUENCENO 		"SequenceNo"
#define RTSP_HDR_CSEQ 				"CSeq"
#define RTSP_HDR_STREAM 			"Stream"
#define RTSP_HDR_SESSION 			"Session"
#define RTSP_HDR_TRANSPORT 			"Transport"
#define RTSP_HDR_RANGE 				"Range"	
#define RTSP_HDR_USER_AGENT 		"User-Agent"	
#define RTSP_HDR_AUTHORIZATION		"Authorization"

#define RTSP_SDP					"application/sdp"

#define RTSP_MAKE_RESP_CMD(req)		(req+100)

/* vodSvrType点播类型，chn 媒体数据通道号， avType 音频还是视频 */
#define VOD_GetSenderPtr(vodSvrType, chn, avType) \
						(gRtspVodSvr[vodSvrType].pRtpUdpSender[chn][avType])

#define H264_Get_NalType(c)  ( (c) & 0x1F )  

#define UE_IS_IFRAME		0x88

#define H264_GetUe(c)		((c)&0xff)
#define H264_SetToIFrame(C)	(((C) & (~0x1F)) | 0x05)

#define MPEG4_Get_NalType(c)  ( (c) & 0xC0 )  	//added by xsf


/* pSess 的发送缓冲清0 */
#define RTSP_CLEAR_SENDBUF(pSess) 	\
{\
	memset(pSess->sendBuf, 0, MAX_RTSP_BUF_LEN);\
}

typedef enum VodSvrType_{
	VOD_SVR_TYPE_RTSP, 
	VOD_SVR_TYPE_HTTP,
	VOD_SVR_TYPE_MAX
}VodSvrType_e;

typedef enum RtpStreamType{
	RTP_STREAM_VIDEO = 0,
	RTP_STREAM_AUDIO,
	RTP_STREAM_MAX
}RtpStreamType_e;

typedef enum TrackId{
	TRACK_ID_VIDEO = 0,
	TRACK_ID_AUDIO,
	TRACK_ID_UNKNOWN
}TrackId_e;

typedef enum RtspReqMethod{
	RTSP_REQ_METHOD_SETUP = 0,
	RTSP_REQ_METHOD_DESCRIBE,
	RTSP_REQ_METHOD_REDIRECT,
	RTSP_REQ_METHOD_PLAY,
	RTSP_REQ_METHOD_PAUSE,
	RTSP_REQ_METHOD_SESSION,
	RTSP_REQ_METHOD_OPTIONS,
	RTSP_REQ_METHOD_RECORD,
	RTSP_REQ_METHOD_TEARDOWN,
	RTSP_REQ_METHOD_GET_PARAM,
	RTSP_REQ_METHOD_SET_PARAM,
	RTSP_REQ_METHOD_EXTENSION,
	RTSP_REQ_METHOD_MAX
}RtspReqMethod_e;

typedef enum RtpTransportType{
	RTP_TRANSPORT_TYPE_UDP, 
	RTP_TRANSPORT_TYPE_TCP, 
	RTP_TRANSPORT_TYPE_BUTT
}RtpTransportType_e;

typedef enum RtpTargetHostState_{
	RTP_TARGETHOST_STATE_INIT, 
	RTP_TARGETHOST_STATE_REQ_IFRAME, 
	RTP_TARGETHOST_STATE_SENDING,
	RTP_TARGETHOST_BUTT
}RtpTargetHostState_e;


typedef enum VodSvrState{
	VOD_SVR_STATE_INIT,
	VOD_SVR_STATE_RUNNING,
	VOD_SVR_STATE_STOP, 
	VOD_SVR_STATE_BUTT
}VodSvrState_e;



typedef enum VodSessionState{
	RTSP_STATE_INIT		= 0,
	RTSP_STATE_READY,
	RTSP_STATE_PLAY,
	RTSP_STATE_STOP,
	RTSP_TCP_EXIT,
	RTSP_STATE_BUTT
}VodSessionState_e;

typedef enum BaseStat{
	BASE64_STAT_STOP = 1,	/* 没有运行 */
	BASE64_STAT_READING,	/* 准备 */
	BASE64_STAT_READED,
	BASE64_STAT_GETSPSPPS,	/* 获取SPS， PPS */
	BASE64_STAT_RUNNING		/*  */
}Base64Stat_e;

typedef struct Base64Stat_s{
	Bools 			isActive;
	u32				videoSeq;
	Base64Stat_e	base64Stat;
}Base64Stat_t;

typedef struct RtspMethod_s{
	char   *describe;
	int		opcode;
}RtspMethod_t;

typedef struct RtpTcpTicket_s{
	u32		rtp;
	u32		rtcp;
}RtpTcpTicket_t;

typedef struct RtpTargetHost_s{
	struct list_head 		rtpTargetList;
	RtpTransportType_e		transportType;
	char						remoteIpAddr[64];
	int 					remotePort;
	struct sockaddr_in		remoteAddr;
	RtpTargetHostState_e	hostState;
	int					isActive;
	void 					*pRtspSess;
}RtpTargetHost_t;


typedef struct RtpTcpSender_s{
	RtpTcpTicket_t 		interleaved[RTP_STREAM_MAX];
	u32					audioG726Ssrc;
	u32					audioG711Ssrc;
	u32					videoH264Ssrc;
	u16					lastSn;
	u16					AudioSeq;
	u16					lastTs;
	int					channel;
	int					tcpSockFd;
	u8					sendBuf[MAX_RTP_LEN];
	u32					sendLen;
}RtpTcpSender_t;


typedef struct RtspSession_s{
	struct list_head 	rtspSessList;
	u32					channel;
	RtpTransportType_e	transportType;
	RtpTcpTicket_t		interleaved[RTP_STREAM_MAX];
	char				sessId[16];
	char				ssrc[16];
	char				userAgent[128];
	char				uri[256];
	char				recvBuf[MAX_RTSP_BUF_LEN];
	char				sendBuf[MAX_RTSP_BUF_LEN];
	char				hostIp[64];
	char				remoteIp[64];
	int					setupFlag[2];
	int					rtspSockFd;
	int					inSize;
	int					remotePort;
	int					rsv;
	int					remoteRtpPort[RTP_STREAM_MAX];
	int					remoteRtcpPort[RTP_STREAM_MAX];
	RtpTargetHost_t		*rtpTargets[RTP_STREAM_MAX];
	int					reqStreamFlag[RTP_STREAM_MAX];
	int					lastSendReq;
	u32					lastSendSeq;
	u32					lastRecvSeq;
	int					readToSend;
	pthread_t			tcpThrId;
	VodSvrState_e		svrType;
	VodSessionState_e	sessStat;
	char 				nonce[17];
	RtpTcpSender_t		*pRtpTcpSender;
	void				*readrbHandle;				// ringbuf read handle
}RtspSession_t;



#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __GS_RTSP_COMMON_H__ */

