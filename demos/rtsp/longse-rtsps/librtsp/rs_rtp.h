/******************************************************************************
 * Copyright (C), 2008-2011, Grandstream Co., Ltd.
 ******************************************************************************
 File Name     : gs_rtp.h
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

#ifndef __GS_RTP_H__
#define __GS_RTP_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

#include "rs_type.h"
#include "rs_rtsp_common.h"
#include "rs_misc.h"

#define 	DEFAULT_RTP_UDP_PORT		32767
#define 	RTP_DEFAULT_SSRC 			41030
#define 	RTP_MAX_SENDER				16
#define 	RTP_VERSION				2

#define RTP_HDR_SET_VERSION(pHDR, val)  ((pHDR)->version = val)
#define RTP_HDR_SET_P(pHDR, val)        ((pHDR)->p       = val)
#define RTP_HDR_SET_X(pHDR, val)        ((pHDR)->x       = val)
#define RTP_HDR_SET_CC(pHDR, val)       ((pHDR)->cc      = val)

#define RTP_HDR_SET_M(pHDR, val)        ((pHDR)->marker  = val)
#define RTP_HDR_SET_PT(pHDR, val)       ((pHDR)->pt      = val)

#define RTP_HDR_SET_SEQNO(pHDR, _sn)    ((pHDR)->seqno  = (_sn))
#define RTP_HDR_SET_TS(pHDR, _ts)       ((pHDR)->ts     = (_ts))

#define RTP_HDR_SET_SSRC(pHDR, _ssrc)    ((pHDR)->ssrc  = _ssrc)

#define RTP_HDR_LEN  					sizeof(RtpHdr_t)



typedef enum RtpPt_
{
    RTP_PT_ULAW             = 0,        /* mu-law */
    RTP_PT_GSM              = 3,        /* GSM */
    RTP_PT_G723             = 4,        /* G.723 */
    RTP_PT_ALAW             = 8,        /* a-law */
    RTP_PT_G722             = 9,        /* G.722 */
    RTP_PT_S16BE_STEREO     = 10,       /* linear 16, 44.1khz, 2 channel */
    RTP_PT_S16BE_MONO       = 11,       /* linear 16, 44.1khz, 1 channel */
    RTP_PT_MPEGAUDIO        = 14,       /* mpeg audio */
    RTP_PT_JPEG             = 26,       /* jpeg */
    RTP_PT_H261             = 31,       /* h.261 */
    RTP_PT_MPEGVIDEO        = 32,       /* mpeg video */
    RTP_PT_MPEG2TS          = 33,       /* mpeg2 TS stream */
    RTP_PT_H263             = 34,       /* old H263 encapsulation */
    //RTP_PT_PRIVATE          = 96,
    RTP_PT_H264             = 96,       /* hisilicon define as h.264 */
    RTP_PT_H265             = 96,       /* hisilicon define as h.265 */
    RTP_PT_G726             = 97,       /* hisilicon define as G.726 */
    RTP_PT_ADPCM            = 98,       /* hisilicon define as ADPCM */
    RTP_PT_MPEG4		= 99,	/*xsf define as Mpeg4*/
    RTP_PT_INVALID          = 127
}RtpPt_e;


#ifndef BYTE_ORDER
#define BYTE_ORDER LITTLE_ENDIAN
#endif

/* total 12Bytes */
typedef struct RtpHdr_s
{

#if (BYTE_ORDER == LITTLE_ENDIAN)
    /* byte 0 */
    u16 cc      :4;   /* CSRC count */
    u16 x       :1;   /* header extension flag */
    u16 p       :1;   /* padding flag */
    u16 version :2;   /* protocol version */

    /* byte 1 */
    u16 pt      :7;   /* payload type */
    u16 marker  :1;   /* marker bit */
#elif (BYTE_ORDER == BIG_ENDIAN)
    /* byte 0 */
    u16 version :2;   /* protocol version */
    u16 p       :1;   /* padding flag */
    u16 x       :1;   /* header extension flag */
    u16 cc      :4;   /* CSRC count */
    /*byte 1*/
    u16 marker  :1;   /* marker bit */
    u16 pt      :7;   /* payload type */
#else
    #error YOU MUST DEFINE BYTE_ORDER == LITTLE_ENDIAN OR BIG_ENDIAN !
#endif


    /* bytes 2, 3 */
    u16 seqno  :16;   /* sequence number */

    /* bytes 4-7 */
    int ts;            /* timestamp in ms */

    /* bytes 8-11 */
    int ssrc;          /* synchronization source */
} RtpHdr_t;

typedef enum RtpSenderType{
	RTP_SENDER_UNICAST		= 0, 	/* 单播 */
	RTP_SENDER_MULTICAST	= 1,	/* 组播 */
	RTP_SENDER_BROADCAST	= 3,	/* 广播 */
}RtpSenderType_e;

typedef enum PacketType{
	PACKET_TYPE_RAW 	= 0,	/* 特定媒包头类型，头长8个字节,用于TCP传输 */
	PACKET_TYPE_RTP,			/* 普通RTP打包方式，头12个字节 */
	PACKET_TYPE_RTP_STAP,		/* STAP-A打包方式，加了3个字节的净荷头，头长15个字节 */
	PACKET_TYPE_BUTT
}PacketType_e;

typedef enum AVType{
	AV_TYPE_VIDEO 		= 0,
	AV_TYPE_AUDIO,
	AV_TYPE_AV,
	AV_TYPE_BUTT
}AVType_e;

typedef enum TcpSyncState{
	SYNC_WAIT = 2,
	SYNC_OK
}TcpSyncStat_e;


typedef struct RtpStat_s{
	u64				sendPkt;		/* 已发送的包个数 */
	u64				sendByte;		/* 已发送的字节数 */
	u64				sendErr;		/* 发送失败的次数 */
	u64				recvPkt;		/* 接收的包个数 */
	u64				recvByte;		/* 接收的字节数 */
	u64				unavailable;	/*  */
	u64				bad;			/* packets that did not appear to be RTP */
	u64				discarded;		/* 丢弃的包 */
	u64				timeoutCnt;		/* 超时的次数 */
}RtpStat_t;


typedef struct RtpUdpSender_s{
	int 				maxTargets;
	int 				targetCnt;
	RtpTargetHost_t		targetHost;
	RtpPt_e				pt;
	u32					ssrc;
	u32					lastSn;
	u32					resv;
	u32					lastTs;
	RtpStat_t			stats;
	int					bActive;
	int					rtpPort;
	int					rtcpPort;
	int					channel;
	int					rtpSockFd;
	RtpSenderType_e		senderType;
	RtpTransportType_e	transportType;
	PacketType_e		pktType;
	AVType_e			avType;
	u8					sendBuf[MAX_RTP_LEN];
	u32					sendLen;
}RtpUdpSender_t;

#define QUANT_TABLE_LEN			64
#define MJPEG_RTP_LEN			888//(892-4)   //ljh 复位标志占4字节

typedef struct JpegHdr_s{
	u32			tspec:8;		/* 类型特定 */
	u32			off:24;			/* 分段偏移 */
	u8			type;			/* 类型 */
	u8			q;				/* Q值 */
	u8			width;			/* 宽 */
	u8			height;			/* 高 */
}JpegHdr_t;

typedef struct JpegHdrQTable_s{
	u8			mbz;
	u8			precision;
	u8			length[2];
}JpegHdrQTable_t;

typedef struct QTable_s{
	int		len;
	char 	*src;
}QTalbe_t;

typedef struct JpegQTable_s{
	QTalbe_t	table[4];
	char        *scan_data;         /* JPEG码流 */
	int         scan_data_len;      /* JPEG码流长度 */
	u8          interval[2];        /* 复位间隔 */
}JpegQTable_t;

typedef struct JpegRestart_s{
	u8			interval[2];        /* 复位间隔 */
	u16			f:1;                /* F帧 */
	u16			l:1;                /* I帧 */
	u16			count:14;           /* 复位计数 */
}JpegRestart_t;



int  RTP_DelUdpSender(char *ip, int port, RtpUdpSender_t *pSender);
RtpTargetHost_t *RTP_AddUdpSender(char *ip, int port, RtpUdpSender_t *pSender);
int	RTP_CreateUdpSender(RtpUdpSender_t **ppRtpUdpSender, int channel, int	maxTarget, RtpPt_e pt);
void RTP_UdpSendVideoPacket(int chn, u32 pts, int market, char *data, int len);
void RTP_UdpSendAudioPacket(int chn, u32 pts, int market, int len ,char *data );
int RTP_TcpStartSendMediaData(RtspSession_t *pSess);
int RTP_CheckBase64Stat(int chn);
int RTP_Base64Init(int chn);
void RTP_UdpSendMediaPkt(int type, int chn, u32 pts, int frameType, time_t wTime, int len, char *data);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __GS_RTP_H__ */

