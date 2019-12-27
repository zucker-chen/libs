/******************************************************************************
 * Copyright (C), 2008-2011, Grandstream Co., Ltd.
 ******************************************************************************
 File Name     : gs_rtp.c
 Version       : Initial Draft
 Author        : Grandstream video software group
 Created       : 2009/11/19
 Last Modified :
 Description   : rtsp传输协议
 Function List :
 Note		   : created 2009/11/19
 History       :
 1.Date        : 2009/11/19
   Author      : lwx
   Modification:
 ******************************************************************************/

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#include <errno.h>


#include "rs_rtsp_common.h"
#include "rs_rtsp_base64.h"
#include "rs_type.h"
#include "rs_rtp.h"
#include "rs_rtsp.h"
#include "ringbuf.h"
//#include "CSVSDK.h"

//#include <avserver_ui.h>


#define MODLE_NAME		"RTP "

Base64Stat_t		gBase64Stat[MAX_ENC_NUM];
extern RtspSvr_t	gRtspVodSvr[VOD_SVR_TYPE_MAX];

#define __one_copy__

int RTP_Base64Init(int chn)
{
	gBase64Stat[chn].isActive = False;
	gBase64Stat[chn].base64Stat = BASE64_STAT_READING;
	RTSP_Media_Paras_Init(chn);
	return GS_SUCCESS;
}


int RTP_CheckBase64Stat(int chn)
{
	if(gBase64Stat[chn].base64Stat == BASE64_STAT_READED){
		return True;
	}
	return False;
}


int RTP_GetJpegQTable(JpegQTable_t *pQtab, char *data, int len)
{
	int hLen = 0, n =0, i=0, id = 0;
	char *pSrc = data;

	while(pSrc < len+data){

        /* 一个或者多个量化表DQT(Difine Quantization Table)
            0xFFDB+量化表长度(2字节)+量化表
        */
		if(((*pSrc) == 0xff) && ((*(pSrc+1)) == 0xdb)){
			pSrc += 2;
			hLen = 0;
			hLen = (*pSrc)<< 8;	/* Highbyte */
			pSrc ++;
			hLen |= (*pSrc);
			pSrc ++;
			n = (hLen - 2)/(QUANT_TABLE_LEN+1);
			for(i=0; i<n; i++){
				id = *pSrc;
				pSrc ++;
				if(id >= 4){
					rtsp_dbg(Err, DbgNoPerror, "Get qtable id error \n");
					return GS_FAIL;
				}
				pQtab->table[id].len = QUANT_TABLE_LEN;
				pQtab->table[id].src = pSrc;
				pSrc += QUANT_TABLE_LEN;
			}
		}

        /* 重新开始间隔DRI--Difine Restart Interval
            0xFFDD+0x0004(长度、固定)+复位间隔(为零时就不存在重开始间隔和重开始标记)
        */
        if ((*pSrc == 0xFF) &&(*(pSrc+1) == 0xDD)){
            pSrc += 4;
			pQtab->interval[0]= *pSrc;
			pSrc++;
			pQtab->interval[1]= *pSrc;
        }

		/* 扫描开始SOS(Start of Scan)
            0xFFDA+参数长度(2字节)+参数+JPEG码流
		*/
        if ((*pSrc == 0xFF) &&(*(pSrc+1) == 0xDA)){
			pSrc += 2;
			hLen = 0;
			hLen =  *pSrc++ << 8;	/* Highbyte */
            hLen |= *pSrc++;		/* Lowbyte */
			pQtab->scan_data = pSrc + hLen -2;
			pQtab->scan_data_len = len - (pQtab->scan_data - data);
			break;
        }

		pSrc ++;
	}
	return GS_SUCCESS;
}


int RTP_WaitIFrame(int chn, int nalType, VodSvrType_e svrType)
{
	RtpTargetHost_t		*pVTarget = NULL;
	RtpTargetHost_t		*pATarget = NULL;
	struct list_head 	*pos = NULL, *q = NULL;
	RtpUdpSender_t 		*pSender = VOD_GetSenderPtr(svrType, chn, RTP_STREAM_VIDEO);
	
	list_for_each_safe(pos, q, &pSender->targetHost.rtpTargetList){
		pVTarget = list_entry(pos, RtpTargetHost_t, rtpTargetList);
		if(pVTarget != NULL){
			if(pVTarget->isActive){
				if(pVTarget->hostState == RTP_TARGETHOST_STATE_REQ_IFRAME){
					if(nalType == NAL_TYPE_SPS || nalType == NAL_TYPE_IDR || nalType == NAL_TYPE_SEI){
						pVTarget->hostState = RTP_TARGETHOST_STATE_SENDING;
						pATarget = ((RtspSession_t *)(pVTarget->pRtspSess))->rtpTargets[RTP_STREAM_AUDIO];
						if(pATarget != NULL){
							pATarget->hostState = RTP_TARGETHOST_STATE_SENDING;
							rtsp_dbg(Dbg, DbgNoPerror, "chn: %d, client: %s:%d"
								" start sending audio data \n", chn,
								pATarget->remoteIpAddr,
								pATarget->remotePort);
						}
					}
				}
			}
		}
	}
	return GS_SUCCESS;
}

int RTP_OpenUdpSocket(int port)
{
	int	sockFd;
	int	socketOptVal = 1;
	struct sockaddr_in addr;

	sockFd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sockFd <= 0){
		rtsp_dbg(Err, DbgPerror, "socket error \n");
		return GS_FAIL;
	}

	addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);
    memset(&(addr.sin_zero), '\0', 8);
	if (-1 == setsockopt(sockFd, SOL_SOCKET, SO_REUSEADDR,
									&socketOptVal, sizeof(int))) {
		rtsp_dbg(Err, DbgPerror,"setsockopt failed!\n");
		sock_close(sockFd);
		return GS_FAIL;
	}
    if (bind (sockFd, (struct sockaddr *)&addr, sizeof (addr)))
    {
        sock_close(sockFd);
        rtsp_dbg(Err, DbgPerror, "UDP Socket Open error.\n");
        return GS_FAIL;
    }
	return sockFd;
}

int	RTP_CreateUdpSender(RtpUdpSender_t **ppRtpUdpSender, int channel, int	maxTarget, RtpPt_e pt)
{
	int sockFd ;
	int	socketOptVal = 1;
	static int	sPort = DEFAULT_RTP_UDP_PORT;
	RtpUdpSender_t *pSender = NULL;

	*ppRtpUdpSender = NULL;
	pSender = (RtpUdpSender_t *)malloc(sizeof(RtpUdpSender_t));
	if(pSender == NULL){
		rtsp_dbg(Err, DbgPerror, "malloc psender error \n");
		return GS_FAIL;
	}
	memset(pSender, 0, sizeof(RtpUdpSender_t));
	INIT_LIST_HEAD(&(pSender->targetHost.rtpTargetList));

	sockFd = RTP_OpenUdpSocket(sPort + 2);
	if(sockFd < 0){
		rtsp_dbg(Err, DbgNoPerror, "RTP_OpenUdpSocket error \n");
		free(pSender);
		pSender = NULL;
		return GS_FAIL;
	}
	if (-1 == setsockopt(sockFd, SOL_SOCKET, SO_REUSEADDR, &socketOptVal, sizeof(int))) {
        rtsp_dbg(Err, DbgPerror, "setsockopt error \n");
		free(pSender);
		pSender = NULL;
		return GS_FAIL;
    }

	pSender->bActive 	= False;
	pSender->lastTs  	= 3600;
	pSender->pt  	 	= pt;
	/* fixed by ljh */
	if((RTP_PT_H264 == pt)||(RTP_PT_JPEG == pt)||(RTP_PT_MPEG4 == pt)){
		pSender->ssrc = RTP_DEFAULT_SSRC + channel;
		pSender->lastSn = 3600;
	}else{
		pSender->ssrc = RTP_DEFAULT_SSRC + channel + 128;
		pSender->lastSn = 1200;
	}
	pSender->rtpPort = sPort + 2;
	pSender->rtcpPort = 0;
	pSender->rtpSockFd = sockFd;
	pSender->channel = channel;
	pSender->pktType = PACKET_TYPE_RAW;
	if(0 == maxTarget){
		maxTarget = RTP_MAX_SENDER;
	}
	pSender->maxTargets = maxTarget;
	*ppRtpUdpSender = pSender;
	sPort += 2;
	return GS_SUCCESS;
}



RtpTargetHost_t *RTP_FindUdpSender(char *ip, int port, RtpUdpSender_t *pSender)
{
	struct list_head *pos;
	RtpTargetHost_t *pTarget = NULL;

	list_for_each(pos, &(pSender->targetHost.rtpTargetList)){
		pTarget = list_entry(pos, RtpTargetHost_t, rtpTargetList);
		if(pTarget != NULL){
			if((0 == strcmp(ip, pTarget->remoteIpAddr)) &&
				(port == pTarget->remotePort)){
				return pTarget;
			}
		}
	}
	return NULL;
}

int RTP_DelUdpSender(char *ip, int port, RtpUdpSender_t *pSender)
{
	RtpTargetHost_t *pTarget = NULL;

	pTarget = RTP_FindUdpSender(ip, port, pSender);
	if(pTarget != NULL){
		pTarget->isActive = False;
		list_del(&(pTarget->rtpTargetList));
		free(pTarget);
		pTarget = NULL;
		pSender->targetCnt --;
	}
	return GS_SUCCESS;
}

RtpTargetHost_t *RTP_AddUdpSender(char *ip, int port, RtpUdpSender_t *pSender)
{
	RtpTargetHost_t *pTarget = NULL;

	if(pSender == NULL){
		return NULL;
	}

	if(pSender->targetCnt >= pSender->maxTargets){
		rtsp_dbg(Err, DbgNoPerror, "rtp chn = %d reached the max sender "
			"target host: %d\n", pSender->channel, pSender->maxTargets);
		return NULL;
	}

	pTarget = (RtpTargetHost_t *)malloc(sizeof(RtpTargetHost_t));
	if(pTarget == NULL){
		rtsp_dbg(Err, DbgPerror, "malloc targethost error \n");
		return NULL;
	}
	memset(pTarget, 0, sizeof(RtpTargetHost_t));
	strcpy(pTarget->remoteIpAddr, ip);
	pTarget->remotePort = port;

	memset(&pTarget->remoteAddr, 0, sizeof(struct sockaddr_in));
	pTarget->remoteAddr.sin_family 		= AF_INET;
	pTarget->remoteAddr.sin_port 		= htons(pTarget->remotePort);
	pTarget->remoteAddr.sin_addr.s_addr 	= inet_addr(pTarget->remoteIpAddr);

	list_add(&pTarget->rtpTargetList, &pSender->targetHost.rtpTargetList);
	pSender->targetCnt ++;
	pTarget->hostState = RTP_TARGETHOST_STATE_REQ_IFRAME;
	pTarget->isActive = True;

	return pTarget;
}

int RTP_UdpPacket(RtpUdpSender_t *pSender, u32 pts, int marker,
													int len, char *data)
{
	RtpHdr_t *pRtpHdr = NULL;

	pSender->sendLen = 0;
	pRtpHdr = (RtpHdr_t *)pSender->sendBuf;

	RTP_HDR_SET_VERSION(pRtpHdr, RTP_VERSION);
    RTP_HDR_SET_P(pRtpHdr, 0);
    RTP_HDR_SET_X(pRtpHdr, 0);
    RTP_HDR_SET_CC(pRtpHdr, 0);

    RTP_HDR_SET_M(pRtpHdr, marker);
    RTP_HDR_SET_PT(pRtpHdr, pSender->pt);

    RTP_HDR_SET_SEQNO(pRtpHdr, htons(pSender->lastSn));

    RTP_HDR_SET_TS(pRtpHdr, htonl(pts));

    RTP_HDR_SET_SSRC(pRtpHdr, htonl(pSender->ssrc));

    pSender->lastSn ++;
    pSender->lastTs = pts;

    memcpy(pSender->sendBuf+ RTP_HDR_LEN, data, len);
    pSender->sendLen = RTP_HDR_LEN + len;

	return GS_SUCCESS;
}

int RTP_UdpSend(RtpUdpSender_t  *pSender)
{
	int ret;
	struct list_head *pos, *q;
	RtpTargetHost_t *pHost = NULL;

	list_for_each_safe(pos, q, &(pSender->targetHost.rtpTargetList) )
    {
		pHost= list_entry(pos, RtpTargetHost_t , rtpTargetList);
		if(True == RTSP_GetUdpSendStat(pSender->channel)){
			if ((pHost != NULL) && (pHost->isActive == True) && (pSender->sendLen > 0)
					&& ((pHost->hostState == RTP_TARGETHOST_STATE_SENDING)||(RTP_PT_H264 != pSender->pt)))
			{
				//printf("udp send ......\n");
				ret = sendto(pSender->rtpSockFd , pSender->sendBuf ,
						pSender->sendLen, 0, (struct sockaddr*)&pHost->remoteAddr,
						sizeof(struct sockaddr));

				if (ret != pSender->sendLen)
				{
					pSender->stats.sendErr ++;
				}
				else
				{
					pSender->stats.sendByte += pSender->sendLen;
					pSender->stats.sendPkt ++;
				}
			}
		}
    }
    return GS_SUCCESS;

}

void RTP_UdpSendVideoPacket(int chn, u32 pts, int market,
													char *data, int len)
{
	RtpUdpSender_t *pSender = NULL;

	pSender = VOD_GetSenderPtr(VOD_SVR_TYPE_RTSP, chn, RTP_STREAM_VIDEO);
	if(pSender != NULL){
		RTP_UdpPacket(pSender, pts, market, len, data);
		RTP_UdpSend(pSender);
	}
}

void RTP_UdpSendAudioPacket(int chn, u32 pts, int market,
												int len ,char *data )
{
	RtpUdpSender_t *pSender = NULL;
	//SendAudioStreamToCSV(chn, len, (unsigned char *)data);
	/* 音频DISABLE状态时则不发送音频 */
	//if( AUDIO_DISABLE != CFG_GetAudioEncType() ){
		pSender = VOD_GetSenderPtr(VOD_SVR_TYPE_RTSP, chn, RTP_STREAM_AUDIO);
		if(pSender != NULL){
			RTP_UdpPacket(pSender, pts, market, len, data);
			RTP_UdpSend(pSender);
		}
	//}
}
int firstFrame = 0;
int RTP_UdpSendH264Pkt(int chn, u32 pts, int frameType, time_t wTime,int len, char *data)
{
#if 0
	char *pvide_data = NULL;
	char fua_buf[MAX_RTP_LEN];
	u8 nal_type;
	u8 s_token;
	int data_len, left_len, pos, iFlag;
	int spsLen = 0, ppsLen = 0, seiLen;
	//int productId = CFG_GetProductId();
	int send_len[5] = {0};
	char * psend_data[5] = {NULL};
	u8 send_count = 0;
	u8 uloop=0;
	if(chn != 0 && chn != 1 && chn != 2){
		return 0;
	}
	pvide_data = data + 4;
	data_len   = len - 4;
	if(data == NULL){
		return GS_SUCCESS;
	}
	
	#if 0
	if( productId == ID_GXV3500 || productId == ID_CL3500 ||
	productId == ID_IP5150 ){
	if(len <= 0 || len > ENC_SIZE_SD){
		dbg(Err, DbgNoPerror, "Len = %d \n", len);
		return GS_SUCCESS;
	}
	}else{
	if(len <= 0 || len > ENC_OUTBUFF_SIZE){
		dbg(Err, DbgNoPerror, "Len = %d \n", len);
		return GS_SUCCESS;
		}
	}
	#endif
	
#if 1 //头加密
	if(gBase64Stat[chn].base64Stat == BASE64_STAT_READING){
	nal_type = H264_Get_NalType(*pvide_data);
	if(NAL_TYPE_SPS == nal_type){
		seiLen = RINGBUF_GetSpsPpsSeiLen(&spsLen, &ppsLen, len, data);
		RTSP_Media_Para_SetSeqBase64( chn, pvide_data, spsLen-4);
		
		pvide_data = data + 4 + spsLen;
		data_len   = len - 4 - spsLen;
		nal_type = H264_Get_NalType(*pvide_data);
		if(nal_type == NAL_TYPE_PPS){
			RTSP_Media_Para_SetPictBase64( chn, pvide_data, ppsLen-4);
		}
	#if 0
		if(GS_SUCCESS == RTSP_Media_Para_SetBase64( chn)){
		gBase64Stat[chn].base64Stat = BASE64_STAT_READED;
		}else{
			dbg(Err, DbgNoPerror, "RTSP_Media_Para_SetBase64"" chn = %d error \n", chn);
		}
	#endif
		gBase64Stat[chn].base64Stat = BASE64_STAT_READED;
	}
	return GS_SUCCESS;
	}
#endif
	nal_type = H264_Get_NalType(*pvide_data);

	if(frameType == VENC_I_FRAME ){
		iFlag = 1;
	}else{
		iFlag = 0;
	}
	//SendVideoStreamToCSV(chn, len, iFlag, (unsigned char *)data);


	RTP_WaitIFrame(chn, nal_type, VOD_SVR_TYPE_RTSP);
	if (frameType == VENC_I_FRAME)
	{
		send_count = 0;
		pvide_data = data + 4;
		seiLen = RINGBUF_GetSpsPpsSeiLen(&spsLen, &ppsLen, len, data);
		send_len[0] = spsLen - 4;
		psend_data[0] = pvide_data;
		send_count++;
		pvide_data = pvide_data + spsLen;
		send_len[1] = ppsLen - 4;
		psend_data[1] = pvide_data;
		send_count++;
		pvide_data = pvide_data + ppsLen;
		if (seiLen > 0)
		{
		    send_len[send_count] = seiLen - 4;
		    psend_data[send_count] = pvide_data;
		    send_count++;
		    pvide_data = pvide_data + seiLen;
		    send_len[send_count] = len - spsLen - ppsLen - seiLen - 4;
		}
		else
		{
		    send_len[send_count] = len - spsLen - ppsLen - 4;
		}
		psend_data[send_count] = pvide_data;
		send_count++;
	}
	else
	{
		send_count = 1;
		psend_data[0] = data + 4;
		send_len[0] = len - 4;
	}
	for(uloop=0; uloop<send_count; uloop++)
	{
		/* 数据长度小于1024则不分包 */
		if(send_len[uloop] <= NAL_FRAGMENTATION_SIZE){
			RTP_UdpSendVideoPacket(chn, pts, 1, psend_data[uloop], send_len[uloop]);
		}else{
			/* 数据长度大于1024，要分包 */
			nal_type = H264_Get_NalType(*psend_data[uloop]);
			//printf("nal_type = %d \n", nal_type);
			/*根据RFC3984  FU-A的RTP荷载格式*/
			/*+---------------+
			|0|1|2|3|4|5|6|7|
			+-+-+-+-+-+-+-+-+
			|F|NRI|Type = 1c|
			+---------------+*/
			fua_buf[0] = 0x1c | (*psend_data[uloop] & ~0x1F); /*fua_buf[0] filled the FU indicator*/
			s_token = 1;
			left_len = send_len[uloop];
			pos = 0;
			while(left_len > NAL_FRAGMENTATION_SIZE)
			{
				/*+---------------+
				|0|1|2|3|4|5|6|7|
				+-+-+-+-+-+-+-+-+
				|S|E|R| NalType |
				+---------------+*/
				fua_buf[1] = (s_token<<7) | nal_type; /*fua_buf[0] filled the FU header*/
				/*第一个切包，不要第一个字节*/
				memcpy(fua_buf+2, psend_data[uloop] + pos + s_token, NAL_FRAGMENTATION_SIZE-s_token);
				RTP_UdpSendVideoPacket(chn, pts, 0, fua_buf, NAL_FRAGMENTATION_SIZE+2-s_token);
				s_token = 0;
				left_len  -= NAL_FRAGMENTATION_SIZE;
				pos  += NAL_FRAGMENTATION_SIZE;
			}
			if (s_token)
			{
			 nal_type |= 128;
			}
			fua_buf[1] = 64 | nal_type; /*fua_buf[0] filled the FU header*/
			memcpy(fua_buf+2, psend_data[uloop] + pos + s_token, left_len-s_token);
			RTP_UdpSendVideoPacket(chn, pts, 1, fua_buf, left_len+2-s_token);
		}
	}
#endif	
	return GS_SUCCESS;
}
 
#if 0
int RTP_UdpSendH264Pkt(int chn, u32 pts, int frameType, time_t wTime,
												int len, char *data)
{
	char *pvide_data = NULL;
	char fua_buf[MAX_RTP_LEN];
	u8 nal_type;
	u8 s_token;
	int data_len, left_len, pos;
	int	spsLen = 0, ppsLen = 0, sei;
	int	productId = CFG_GetProductId();

	if(chn != 0 && chn != 1){
		return 0;
	}

	pvide_data = data + 4;
	data_len   = len - 4;

	if(data == NULL){
		return GS_SUCCESS;
	}

	if( productId == ID_GXV3500 || productId == ID_CL3500 ||
		productId == ID_IP5150 ){
		if(len <= 0 || len > ENC_SIZE_SD){
			dbg(Err, DbgNoPerror, "Len = %d \n", len);
			return GS_SUCCESS;
		}

	}else{
		if(len <= 0 || len > ENC_OUTBUFF_SIZE){
			dbg(Err, DbgNoPerror, "Len = %d \n", len);
			return GS_SUCCESS;
		}
	}
#if 1	//头加密
	if(gBase64Stat[chn].base64Stat == BASE64_STAT_READING){
		nal_type = H264_Get_NalType(*pvide_data);
		if(NAL_TYPE_SPS == nal_type){
			sei = RINGBUF_GetSpsPpsSeiLen(&spsLen, &ppsLen, len, data);
			RTSP_Media_Para_SetSeqBase64( chn, pvide_data, spsLen-4);
			pvide_data = data + 4 + spsLen;
			data_len   = len - 4 - spsLen;
			nal_type = H264_Get_NalType(*pvide_data);
			if(nal_type == NAL_TYPE_PPS){
				RTSP_Media_Para_SetPictBase64( chn, pvide_data, ppsLen-4);
			}
			if(GS_SUCCESS == RTSP_Media_Para_SetBase64( chn)){
				gBase64Stat[chn].base64Stat = BASE64_STAT_READED;
			}else{
				dbg(Err, DbgNoPerror, "RTSP_Media_Para_SetBase64"
								" chn = %d error \n", chn);
			}
		}
		return GS_SUCCESS;
	}
#endif
	nal_type = H264_Get_NalType(*pvide_data);
	RTP_WaitIFrame(chn, nal_type, VOD_SVR_TYPE_RTSP);

	pvide_data = data + 4;
	data_len   = len - 4;

	/* 数据长度小于1024则不分包 */
	if(data_len <= NAL_FRAGMENTATION_SIZE){
		RTP_UdpSendVideoPacket(chn, pts, 1, pvide_data, data_len);
	}else{
	/* 数据长度大于1024，要分包 */
		nal_type = H264_Get_NalType(*pvide_data);
		//printf("nal_type = %d \n", nal_type);
		/*根据RFC3984  FU-A的RTP荷载格式*/
		/*+---------------+
		|0|1|2|3|4|5|6|7|
		+-+-+-+-+-+-+-+-+
		|F|NRI|Type = 1c|
		+---------------+*/
		fua_buf[0] = 0x1c | (*pvide_data & ~0x1F);			/*fua_buf[0] filled the FU indicator*/
		s_token = 1;
		left_len = data_len;
		pos = 0;

		while(left_len > NAL_FRAGMENTATION_SIZE)
		{
			/*+---------------+
			  |0|1|2|3|4|5|6|7|
			  +-+-+-+-+-+-+-+-+
			  |S|E|R| NalType |
			  +---------------+*/
			fua_buf[1] = (s_token<<7) | nal_type;			/*fua_buf[0] filled the FU header*/

			/*第一个切包，不要第一个字节*/
			memcpy(fua_buf+2, pvide_data + pos + s_token, NAL_FRAGMENTATION_SIZE-s_token);
			RTP_UdpSendVideoPacket(chn, pts, 0, fua_buf, NAL_FRAGMENTATION_SIZE+2-s_token);
			s_token = 0;
			left_len 	-= NAL_FRAGMENTATION_SIZE;
			pos 		+= NAL_FRAGMENTATION_SIZE;
		}

		if (s_token)
		{
		    nal_type |= 128;
		}
		fua_buf[1] = 64 | nal_type;							/*fua_buf[0] filled the FU header*/
		memcpy(fua_buf+2, pvide_data + pos + s_token, left_len-s_token);
		RTP_UdpSendVideoPacket(chn, pts, 1, fua_buf, left_len+2-s_token);
	}
	return GS_SUCCESS;
}
#endif
/*added by xsf for MPEG4*/
int RTP_UdpSendMpeg4Pkt(int chn, u32 pts, int frameType, time_t wTime,
											int len, char *data)
{
#if 0
	char *pvide_data = NULL;
	char fua_buf[MAX_RTP_LEN];
	int data_len, left_len, pos;
	//int	productId = CFG_GetProductId();

	if(chn != 0 && chn != 1){
		return 0;
	}

	if(data == NULL){
		return GS_SUCCESS;
	}
#if 0
	if( productId == ID_GXV3500 || productId == ID_CL3500 ||
		productId == ID_IP5150 ){
		if(len <= 0 || len > ENC_SIZE_SD){
			dbg(Err, DbgNoPerror, "Len = %d \n", len);
			return GS_SUCCESS;
		}

	}else{
		if(len <= 0 || len > ENC_OUTBUFF_SIZE){
			dbg(Err, DbgNoPerror, "Len = %d \n", len);
			return GS_SUCCESS;
		}
	}
#endif
	pvide_data = gMpeg4Stream[chn].vbuf;
	data_len   = gMpeg4Stream[chn].vlen;

	left_len = data_len;
	pos = 0;

	//SendVideoStreamToCSV(chn, len, 1, (unsigned char *)pvide_data);
	/*mpeg4 不需要组AU-head 包头，直接通过udp发送*/
	//printf("RTP_UdpSendMpeg4Pkt : chn = %d \n", chn);
	if(left_len <= NAL_FRAGMENTATION_SIZE){
		RTP_UdpSendVideoPacket(chn, pts, 1, pvide_data, data_len);
	}else{
		/* 数据长度大于1024，要分包 */
		while(left_len > NAL_FRAGMENTATION_SIZE){
			/*第一个切包，不要第一个字节*/
			memcpy(fua_buf, pvide_data + pos , NAL_FRAGMENTATION_SIZE );
			RTP_UdpSendVideoPacket(chn, pts, 0, fua_buf, NAL_FRAGMENTATION_SIZE );
			left_len 	-= NAL_FRAGMENTATION_SIZE;
			pos 		+= NAL_FRAGMENTATION_SIZE;
		}
		memcpy(fua_buf, pvide_data + pos , left_len);
		RTP_UdpSendVideoPacket(chn, pts, 1, fua_buf, left_len);

	}
	return GS_SUCCESS;
#endif
}

#if 0
/*added by xsf for Mjpeg*/
int RTP_UdpSendMJpegPkt(int chn, u32 pts, int frameType, time_t wTime,
												int len, char *data)
{
	int 				tmp;
	int 				leftLen = 0;
	int				market = 0, dataLen = 0 ,pos = 0;
	char				sendBuf[MAX_RTP_LEN];
	JpegHdrQTable_t	tabHdr;
	JpegQTable_t		qTable;
	JpegHdr_t		JpgHdr ;
	JpegRestart_t   restart;
	char				*pvide_data = NULL;
	char				*pSend = NULL;

	if(chn != 0 && chn != 1){
		return 0;
	}

	if(data == NULL){
		return GS_SUCCESS;
	}

	JpgHdr.tspec	= 0;			//类型特定
	JpgHdr.off	= 0;			//分段偏移
	JpgHdr.type	= 65;			/* (带复位标记:64 -- yuv4:2:2, 65 -- yuv4:2:0)(不带复位标记:0 -- yuv4:2:2, 1 -- yuv4:2:0) */
	JpgHdr.q		= 255;		///* Q, value 255 -> dynamic quant tables */
	JpgHdr.width	= CFG_GetVideoWidth(chn)/8;
	JpgHdr.height	= CFG_GetVideoHeight(chn)/8;

	tabHdr.mbz		= 0;
	tabHdr.precision	= 0;
	tabHdr.length[0]	= 0x00;
	tabHdr.length[1]	= 0x80;

	restart.count    = 0x3fff;
	restart.f           = 1;
	restart.l           = 1;

	if(GS_SUCCESS != RTP_GetJpegQTable(&qTable, data, len)){
		dbg(Err, DbgNoPerror, "RTP_GetJpegQTable error \n");
		return GS_FAIL;
	}

	pvide_data = data;
	//leftLen = len;
    leftLen = qTable.scan_data_len;  //ljh
    restart.interval[0] = qTable.interval[0];
    restart.interval[1] = qTable.interval[1];
	pos = 0;

	if(leftLen <= MJPEG_RTP_LEN){
		pSend = sendBuf;
		memcpy(pSend, &JpgHdr, sizeof(JpegHdr_t));
		pSend += sizeof(JpegHdr_t);
		memcpy(pSend, &restart, sizeof(JpegRestart_t));
		pSend += sizeof(JpegRestart_t);
		memcpy(pSend, &tabHdr, sizeof(JpegHdrQTable_t));
		pSend += sizeof(JpegHdrQTable_t);
		memcpy(pSend, qTable.table[0].src, QUANT_TABLE_LEN);
		pSend += QUANT_TABLE_LEN;
		memcpy(pSend, qTable.table[1].src, QUANT_TABLE_LEN);
		pSend += QUANT_TABLE_LEN;

		//memcpy(pSend, pvide_data, MJPEG_RTP_LEN);
		memcpy(pSend, qTable.scan_data, MJPEG_RTP_LEN);
		RTP_UdpSendVideoPacket(chn, pts, 1, sendBuf, leftLen + pSend - sendBuf);
	}
	else{
		pos = 0;
		leftLen = len;
		while(leftLen > MJPEG_RTP_LEN){
			pSend = sendBuf;
			memcpy(pSend, &JpgHdr, sizeof(JpegHdr_t));
			pSend += sizeof(JpegHdr_t);
            memcpy(pSend, &restart, sizeof(JpegRestart_t));
            pSend += sizeof(JpegRestart_t);
			if(JpgHdr.off == 0){
				memcpy(pSend, &tabHdr, sizeof(JpegHdrQTable_t));
				pSend += sizeof(JpegHdrQTable_t);
				memcpy(pSend, qTable.table[0].src, QUANT_TABLE_LEN);
				pSend += QUANT_TABLE_LEN;
				memcpy(pSend, qTable.table[1].src, QUANT_TABLE_LEN);
				pSend += QUANT_TABLE_LEN;
			}
			//memcpy(pSend, pvide_data + pos, MJPEG_RTP_LEN);
			memcpy(pSend, qTable.scan_data + pos, MJPEG_RTP_LEN);
			RTP_UdpSendVideoPacket(chn, pts, 0, sendBuf, MJPEG_RTP_LEN + pSend - sendBuf);

			pos +=  MJPEG_RTP_LEN;
			leftLen -= MJPEG_RTP_LEN;
			JpgHdr.off  +=  MJPEG_RTP_LEN;

		}
		pSend = sendBuf;
		memcpy(pSend, &JpgHdr, sizeof(JpegHdr_t));
		pSend += sizeof(JpegHdr_t);
        memcpy(pSend, &restart, sizeof(JpegRestart_t));
        pSend += sizeof(JpegRestart_t);

		//memcpy(pSend, pvide_data + pos, leftLen);
		memcpy(pSend, qTable.scan_data + pos, MJPEG_RTP_LEN);

		RTP_UdpSendVideoPacket(chn, pts, 1, sendBuf, leftLen + pSend - sendBuf);
	}

	return GS_SUCCESS;
}
#else
int RTP_UdpSendMJpegPkt(int chn, u32 pts, int frameType, time_t wTime,
												int len, char *data)
{
	int					isGrandstreamClinet = 0;
	int 				leftLen = 0;
	int					market = 0, dataLen = 0;
	char				sendBuf[MAX_RTP_LEN] = {0};;
	JpegHdrQTable_t		tabHdr;
	JpegQTable_t		qTable;
	JpegHdr_t		    JpgHdr ;
	JpegRestart_t       restart;     /* 复位标记头 */
	char				*pSend = NULL;
	//int i = 0;

	JpgHdr.tspec	= 0;			//类型特定
	JpgHdr.off	= 0;				//分段偏移
	JpgHdr.type	= 65;			/* (带复位标记:64 -- yuv4:2:2, 65 -- yuv4:2:0)(不带复位标记:0 -- yuv4:2:2, 1 -- yuv4:2:0) */
	JpgHdr.q		= 255;		///* Q, value 255 -> dynamic quant tables */
	JpgHdr.width	= 1280/8;  // gAVSERVER_UI_ctrl.avserverConfig.encodeConfig[chn].cropWidth/8;
	JpgHdr.height	= 720/8; // gAVSERVER_UI_ctrl.avserverConfig.encodeConfig[chn].cropHeight/8;

	tabHdr.mbz		= 0;
	tabHdr.precision	= 0;
	tabHdr.length[0]	= 0x00;
	tabHdr.length[1]	= 0x80;

	restart.count    = 0x3fff;   /* 复位计数必须设为0x3fff */
	restart.f           = 1;
	restart.l           = 1;

	if(GS_SUCCESS != RTP_GetJpegQTable(&qTable, data, len)){
		rtsp_dbg(Err, DbgNoPerror, "RTP_GetJpegQTable error \n");
		return GS_FAIL;
	}

	if(isGrandstreamClinet == 1){
		leftLen = len;
	}else{
	leftLen = qTable.scan_data_len;                      /* JPEG码流数据长度 */
	}
    restart.interval[0] = qTable.interval[0];            /* 复位间隔 */
    restart.interval[1] = qTable.interval[1];

	while(leftLen > 0){
		pSend = sendBuf;
		memcpy(pSend, &JpgHdr, sizeof(JpgHdr));
		pSend += sizeof(JpgHdr);
		if( isGrandstreamClinet != 1){
        memcpy(pSend, &restart, sizeof(JpegRestart_t));   /* 复位标记头紧跟在JPEG头之后 */
        pSend += sizeof(JpegRestart_t);
		}

		if(JpgHdr.off == 0){
			memcpy(pSend, &tabHdr, sizeof(tabHdr));
			pSend += sizeof(tabHdr);
			memcpy(pSend, qTable.table[0].src, QUANT_TABLE_LEN);
			pSend += QUANT_TABLE_LEN;
			memcpy(pSend, qTable.table[1].src, QUANT_TABLE_LEN);
			pSend += QUANT_TABLE_LEN;
		}
		if(leftLen > MJPEG_RTP_LEN){
			market = 0;
			dataLen = MJPEG_RTP_LEN;
		}else{
			market = 1;
			dataLen = leftLen;
		}
		if( isGrandstreamClinet == 1 ){
			memcpy(pSend, data+JpgHdr.off, dataLen);                      /* 发送APPO段+JPEG码流数据 */
		}else{
        memcpy(pSend, qTable.scan_data + JpgHdr.off, MJPEG_RTP_LEN);    /* 发送JPEG码流数据 */

		}

		RTP_UdpSendVideoPacket(chn, pts, market, sendBuf,  pSend + dataLen - sendBuf);
		JpgHdr.off  += dataLen;
		leftLen -= dataLen;
	}
	//SendVideoStreamToCSV(chn, len, 1,(unsigned char *)data);

	return GS_SUCCESS;
}
#endif

void RTP_UdpSendVideo(int type, int chn, u32 pts, int frameType,
										time_t wTime, int len, char *data)
{
	switch(type){
		case VENC_TYPE_H264:
			RTP_UdpSendH264Pkt(chn, pts, frameType, wTime, len, data);
			break;

		case VENC_TYPE_JPEG:
			RTP_UdpSendMJpegPkt(chn, pts, frameType, wTime, len, data);
			break;

		case VENC_TYPE_MPEG4:
			RTP_UdpSendMpeg4Pkt(chn, pts, frameType, wTime, len, data);
			break;

		default:
			rtsp_dbg(Err, DbgNoPerror, "venc type[%d] error  !!!\n", type);
			break;
	}
}

void RTP_UdpSendMediaPkt(int type, int chn, u32 pts, int frameType,
										time_t wTime, int len, char *data)
{
	if(frameType == AUDIO_TYPE){
		RTP_UdpSendAudioPacket(chn, pts, 1, len, data );
	}else{
		RTP_UdpSendVideo(type, chn, pts, frameType, wTime, len, data);
	}
}

int RTP_TcpPacket_Jpeg(RtpTcpSender_t *pSender, u8 *pSendbuff, RtpPt_e payloadType, u32 pts, u8 marker)
{
	RtpHdr_t *pRtpHdr = NULL;
    //u8 *psendbuf = NULL;
    pRtpHdr = (RtpHdr_t *)(pSendbuff);

	RTP_HDR_SET_VERSION(pRtpHdr, RTP_VERSION);
    RTP_HDR_SET_P(pRtpHdr, 0);
    RTP_HDR_SET_X(pRtpHdr, 0);
    RTP_HDR_SET_CC(pRtpHdr, 0);
    RTP_HDR_SET_M(pRtpHdr, marker);
    RTP_HDR_SET_PT(pRtpHdr, payloadType);
	RTP_HDR_SET_SEQNO(pRtpHdr, htons(pSender->lastSn));
	pSender->lastSn ++;
    RTP_HDR_SET_TS(pRtpHdr, htonl(pts));
	RTP_HDR_SET_SSRC(pRtpHdr,htonl(pSender->videoH264Ssrc));
    pSender->lastTs = pts;

    return GS_SUCCESS;

}


int RTP_TcpPacket(RtpTcpSender_t *pSender, RtpPt_e payloadType, u32 pts,
									int marker, int len, char *data)
{
	//printf("[TEST]nalType :%d\n",H264_Get_NalType(*(data+8)));
	RtpHdr_t *pRtpHdr = NULL;
	unsigned short *intlvd_ch = (unsigned short *)(pSender->sendBuf+2);

	pSender->sendLen = 0;
	pRtpHdr = (RtpHdr_t *)(pSender->sendBuf + 4);
	RTP_HDR_SET_VERSION(pRtpHdr, RTP_VERSION);
    RTP_HDR_SET_P(pRtpHdr, 0);
    RTP_HDR_SET_X(pRtpHdr, 0);
    RTP_HDR_SET_CC(pRtpHdr, 0);
    RTP_HDR_SET_M(pRtpHdr, marker);
    RTP_HDR_SET_PT(pRtpHdr, payloadType);
	if( payloadType == RTP_PT_ALAW || RTP_PT_ULAW == payloadType || 
		RTP_PT_G726 == payloadType){
		RTP_HDR_SET_SEQNO(pRtpHdr, htons(pSender->AudioSeq));
		RTP_HDR_SET_SSRC(pRtpHdr, htonl(pSender->audioG711Ssrc));
		pSender->AudioSeq ++;
	}else{
    		RTP_HDR_SET_SEQNO(pRtpHdr, htons(pSender->lastSn));
		RTP_HDR_SET_SSRC(pRtpHdr, htonl(pSender->videoH264Ssrc));
		pSender->lastSn ++;
	}

	RTP_HDR_SET_TS(pRtpHdr, htonl(pts));


	(payloadType == RTP_PT_G726 )? RTP_HDR_SET_SSRC(pRtpHdr,
			htonl(pSender->audioG711Ssrc)):RTP_HDR_SET_SSRC(pRtpHdr,
			htonl(pSender->videoH264Ssrc));
	
	
    pSender->lastTs = pts;
	#ifndef __one_copy__
    memcpy(pSender->sendBuf + RTP_HDR_LEN + 4 , data, len);
	#endif 
    pSender->sendLen = RTP_HDR_LEN + len ;

 	pSender->sendBuf[0] = '$';	
	
	if((payloadType == RTP_PT_H264)
		|| (payloadType == RTP_PT_MPEG4)
    	|| (payloadType == RTP_PT_JPEG)){//fixed by xsf
		pSender->sendBuf[1] = pSender->interleaved[RTP_STREAM_VIDEO].rtp;

		
		
	}else{
		pSender->sendBuf[1] = pSender->interleaved[RTP_STREAM_AUDIO].rtp;
	}
	*intlvd_ch = htons((unsigned short) pSender->sendLen);

    return GS_SUCCESS;

}


void chunk(char *pBuf, int len)
{
	int size = len;
	unsigned char *p = pBuf;	
	unsigned char *pB = p;
	

	for( size = 0; size < len - 5; size++ ) {		
		if ( *p == 0x24 && *(p+1) == 0x0 ) {
			printf("len %d %02x %02x %02x %02x \n", len,  *p, *(p+1), *(p+2), *(p+3) );
		}

		p++;
	}

}

int RTP_TcpSend(RtpTcpSender_t *pSender, int type)
{
	int 	ret = 0;

	/* 音频DISABLE 状态时不发送音频 */
	if( type == RTP_STREAM_AUDIO ){
		//if( AUDIO_DISABLE == CFG_GetAudioEncType() ){
		//	return GS_SUCCESS;
		//}
	}

	if(pSender->tcpSockFd > 0){

		//chunk(pSender->sendBuf+12, pSender->sendLen - 12 );
		
		ret = Net_TcpSendN(pSender->tcpSockFd, (char *)pSender->sendBuf, pSender->sendLen +4);
	
		/*sendBuf[0] = '$'
          sendBuf[1] = interleaved
          sendBuf[2~3] = sendLen
          sendBuf[4~] = data  */

		// static FILE* p = NULL;

		// if (!p ) {
		// 	p = fopen("/opt/app/mytest/send_0508.h264", "w+");
			
		// }
				
		// //printf("nalType :%d\n",H264_Get_NalType(*(pSender->sendBuf+4+pSender->sendLen)));
		// if ( p ) {
		// 	fwrite(pSender->sendBuf, pSender->sendLen+4, 1, p);
		// }
	
		if(ret == pSender->sendLen + 4 ){
			return GS_SUCCESS;
		}
		
	}

	
	rtsp_dbg(Err, DbgPerror, "send %d, sendLen = %d , sockfd = %d\n",
					ret, pSender->sendLen, pSender->tcpSockFd);

	time_t ttt = time(NULL);		
	struct tm *ptm = localtime(&ttt);			
	fprintf(stderr, "time %04d-%02d-%02dT%02d:%02d:%02dZ [File: %s, %s, Line %d] : send %d sendLen = %d , sockfd = %d errno %d %s\n", 
		ptm->tm_year+1900, ptm->tm_mon +1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec, 
		__FILE__, __FUNCTION__, __LINE__,  ret, pSender->sendLen, pSender->tcpSockFd, errno, strerror(errno)); 
	perror(" send ");
	
	return GS_FAIL;
}


//H264发送
void *RTP_TcpSendH264ThrFxn(void *pArgs)
{

	int type, nRet;
	int audioEncType ;
	int nalType, pos, leftLen, dataLen;
	int pktLen,syncStat = SYNC_WAIT;
	u32  index, syncIndex, pts;
	u8  sToken;
	char sBuf[MAX_RTP_LEN];
	struct timeval timeout;
	char	*pBuf = NULL; 
	char 	*pData = NULL;
	char 	*pSend = NULL;
	RtspSvr_Pkg_t *pPkg = NULL;

	RtspSession_t *pSess = (RtspSession_t *)pArgs;

	// pthread_detach(pthread_self());
	
	if(pSess == NULL ){
		rtsp_dbg(Err, DbgNoPerror, "Input params error \n");
		goto __RtpTcpThrFxnExit;
	}

	if(pSess->pRtpTcpSender == NULL){
		rtsp_dbg(Dbg, DbgNoPerror, "tcpSender is NULL \n");
		goto __RtpTcpThrFxnExit;
	}

	audioEncType = RTP_PT_ULAW; //RTP_PT_ULAW;

	timeout.tv_sec  = 3;  // 3
	timeout.tv_usec = 0;

	setsockopt(pSess->rtspSockFd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout));
	setsockopt(pSess->rtspSockFd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));

	//setsockopt(pSess->pRtpTcpSender->tcpSockFd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout));
	
	index = 0;

	rtsp_dbg(Dbg, DbgNoPerror, "start send data\n");


	while(RTSP_STATE_PLAY == pSess->sessStat)
	{
		pktLen = 0;
		while(syncStat == SYNC_WAIT)
		{
			nRet = ringbuf_read_get_unit(pSess->readrbHandle, &pPkg, &pktLen);
			if (nRet != 0 && nRet == EAGAIN) {
				usleep(1000);
				continue;
			} else if (nRet == 0 && pktLen > 0) {
				if(RTSP_STATE_PLAY != pSess->sessStat){
					rtsp_dbg(Err, DbgNoPerror, "pSess stat is not play \n");
					goto __RtpTcpThrFxnExit;
				}
				syncStat = SYNC_OK;
			} else {
				rtsp_dbg(Err, DbgNoPerror, "ringbuf_read_get_unit error, nRet = %d, pktLen = %d \n", nRet, pktLen);
				goto __RtpTcpThrFxnExit;
			}
		}

		if (pktLen <= 0) {
			nRet = ringbuf_read_get_unit(pSess->readrbHandle, &pData, &pktLen);
			if (nRet != 0 && nRet == EAGAIN) {
				usleep(1000);
				syncStat = SYNC_WAIT;
				continue;
			} else if (nRet != 0 || pktLen <= 0) {
				rtsp_dbg(Err, DbgNoPerror, "ringbuf_read_get_unit error, nRet = %d, pktLen = %d \n", nRet, pktLen);
				goto __RtpTcpThrFxnExit;
			}
		}

		pData 	= pPkg->pData;
		pktLen 	= pPkg->nLen;
		pBuf 	= (char *)(pSess->pRtpTcpSender->sendBuf + RTP_HDR_LEN + 4); 
		pts 	= (u32)pPkg->llPts;

		
		if (0) {	// audio

		} else {
			if (pSess->reqStreamFlag[RTP_STREAM_VIDEO]) {					
					pSend	= pData + H264_STARTCODE_LEN ;	//
					dataLen = pktLen - H264_STARTCODE_LEN;
					nalType = H264_Get_NalType(*pSend); 	
					//rtsp_dbg(Dbg, DbgNoPerror, "ringbuf_read_get_unit pktLen = %d, nalType = %d\n", pktLen, nalType);
					//pts += 3600;
					
					/* 长度小于1024不需要分包 */
					if(dataLen <= NAL_FRAGMENTATION_SIZE){
						memcpy(pBuf, pSend, dataLen);
						RTP_TcpPacket(pSess->pRtpTcpSender, RTP_PT_H264, pts, 1, dataLen, pSend);  
						if(GS_SUCCESS != RTP_TcpSend(pSess->pRtpTcpSender, RTP_STREAM_VIDEO)){
							rtsp_dbg(Err, DbgNoPerror, "RTP_TcpSend error, sockfd = %d\n", pSess->rtspSockFd);
							goto __RtpTcpThrFxnExit;
						}
					} else {
						/* 长度大于1024则进行 FU-A 分包 */
			
						pBuf[0] = 0x1c | (*pSend & (~0x1f));
						sBuf[0] = 0x1c | (*pSend & (~0x1f));
									
						leftLen = dataLen;
						sToken	= 1;
						pos 	= 0;
			
						int count = 0;				
						while(leftLen > NAL_FRAGMENTATION_SIZE){
							if(RTSP_STATE_PLAY != pSess->sessStat){
								rtsp_dbg(Err, DbgNoPerror, "pSess stat is not play \n");
								goto __RtpTcpThrFxnExit;
							}					
														
							pBuf[1] = (sToken << 7) | nalType ; 										
							memcpy(pBuf + 2, pSend + pos + sToken, NAL_FRAGMENTATION_SIZE - sToken);							
							sBuf[1] = (sToken << 7) | nalType;
							memcpy(sBuf+2, pSend+pos+sToken,NAL_FRAGMENTATION_SIZE - sToken);
							RTP_TcpPacket(pSess->pRtpTcpSender, RTP_PT_H264, pts, 0, NAL_FRAGMENTATION_SIZE + 2 - sToken, sBuf); 			
														
							if(GS_SUCCESS != RTP_TcpSend(pSess->pRtpTcpSender, RTP_STREAM_VIDEO)){ 
								rtsp_dbg(Err, DbgNoPerror, "RTP_TcpSend error, sockfd = %d\n",
												pSess->rtspSockFd);
								goto __RtpTcpThrFxnExit;
							}				
							
							sToken	= 0;
							leftLen -= NAL_FRAGMENTATION_SIZE;
							pos 	+= NAL_FRAGMENTATION_SIZE;
						}
						
						if(sToken){
							nalType |= 128;
						}
						pBuf[1] = 64 | nalType ;					
						memcpy(pBuf + 2, pSend+pos+sToken, leftLen-sToken);
						sBuf[1] = 64 | nalType;
						memcpy(sBuf + 2, pSend+pos+sToken, leftLen-sToken);
						RTP_TcpPacket(pSess->pRtpTcpSender, RTP_PT_H264, pts, 1, leftLen+2-sToken, sBuf);
						if(GS_SUCCESS != RTP_TcpSend(pSess->pRtpTcpSender,RTP_STREAM_VIDEO)){
							rtsp_dbg(Err, DbgNoPerror, "RTP_TcpSend error, sockfd = %d\n",pSess->rtspSockFd);
							goto __RtpTcpThrFxnExit;
						}		
					}
				}
		}

		ringbuf_read_put_unit(pSess->readrbHandle);
	}

__RtpTcpThrFxnExit:
	rtsp_dbg(Dbg, DbgNoPerror, "[rtp] tcp send h264 thread exit ... \n");
	pSess->sessStat = RTSP_STATE_STOP;	
	rtsp_dbg(Dbg, DbgNoPerror, "__RtpTcpThrFxnExit !!! \n");
	rtsp_dbg(Dbg, DbgNoPerror, "[rtp] tcp send h264 thread ok \n");
	ringbuf_read_put_unit(pSess->readrbHandle);
	
	return NULL;
}





/*added by xsf for Mpeg4*/
void *RTP_TcpSendMpeg4ThrFxn(void *pArgs)
{
	RtspSession_t *pSess = (RtspSession_t *)pArgs;

	rtsp_dbg(Dbg, DbgNoPerror, "[rtp] tcp send mpeg4 thread exit now ... \n");
	pSess->sessStat = RTSP_STATE_STOP;
	rtsp_dbg(Dbg, DbgNoPerror, "__RtpTcpThrFxnExit !!! \n");
	rtsp_dbg(Dbg, DbgNoPerror, "[rtp] tcp send mpeg4 thread exit ok \n");
	
	return NULL;
}

//#define USE_VLC

void *RTP_TcpSendJpegThrFxn(void *pArgs)
{
	RtspSession_t *pSess = (RtspSession_t *)pArgs;

	rtsp_dbg(Dbg, DbgNoPerror, "[rtp] tcp send jpeg thread exit now ... \n");
	pSess->sessStat = RTSP_STATE_STOP;
	rtsp_dbg(Dbg, DbgNoPerror, "__RtpTcpSendJpegThrFxnExit !!! \n");
	rtsp_dbg(Dbg, DbgNoPerror, "[rtp] tcp send jpeg thread exit ok \n");
	
	return NULL;
}

int RTP_TcpSendH264VideoData(RtspSession_t *pSess)
{

	//ReadStreamFile();

	if( 0 != pthread_create(&pSess->tcpThrId, NULL, RTP_TcpSendH264ThrFxn, pSess)){
		rtsp_dbg(Err, DbgPerror, "pthread_create RTP_TcpSendH264ThrFxn error \n");
		return GS_FAIL;
	}

	fprintf(stderr, "=====================> tcpThrId %d \n", pSess->tcpThrId);
	fflush(stderr);
	
	return GS_SUCCESS;
}

/*added by xsf for Mpeg4*/
int RTP_TcpSendMpeg4VideoData(RtspSession_t *pSess)
{
	if( 0 != pthread_create(&pSess->tcpThrId, NULL, RTP_TcpSendMpeg4ThrFxn, pSess)){
		rtsp_dbg(Err, DbgPerror, "pthread_create RTP_TcpSendMPEG4ThrFxn error \n");
		return GS_FAIL;
	}
	return GS_SUCCESS;
}

int RTP_TcpSendJpegVideoData(RtspSession_t *pSess)
{
	if( 0 != pthread_create(&pSess->tcpThrId, NULL, RTP_TcpSendJpegThrFxn, pSess)){
		rtsp_dbg(Err, DbgPerror, "pthread_create RTP_TcpSendJpegVideoData error \n");
		return GS_FAIL;
	}
	return GS_SUCCESS;
}

int RTP_TcpStartSendMediaData(RtspSession_t *pSess)
{
	int ret = GS_FAIL;
	int type = VENC_TYPE_JPEG;//CFG_GetVideoEncType(pSess->channel);
  //	if(gAVSERVER_config.encodeConfig[pSess->channel].codecType == ALG_VID_CODEC_H264)
	//	type = VENC_TYPE_H264;
	//else	
	//	type = VENC_TYPE_JPEG;

  	type = VENC_TYPE_H264;
  
	rtsp_dbg(Dbg, DbgNoPerror, "RTP_TcpStartSendMediaData \n");

	
	//fprintf(stderr, "%s %s line %d RTP_TcpStartSendMediaData\n", __FILE__, __FUNCTION__, __LINE__);

	time_t ttt = time(NULL);		
	struct tm *ptm = localtime(&ttt);			
	fprintf(stderr, "time %04d-%02d-%02dT%02d:%02d:%02dZ [File: %s, %s, Line %d] : RTP_TcpStartSendMediaData\n", 
		ptm->tm_year+1900, ptm->tm_mon +1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec, 
		__FILE__, __FUNCTION__, __LINE__); 

	
	switch(type){
		case VENC_TYPE_H264:
			ret = RTP_TcpSendH264VideoData(pSess);
			break;

		case VENC_TYPE_JPEG:
			ret = RTP_TcpSendJpegVideoData(pSess);
			break;

		case VENC_TYPE_MPEG4:
			ret = RTP_TcpSendMpeg4VideoData(pSess);
			break;

		default:
			rtsp_dbg(Err, DbgNoPerror, "GetVideoEncType error \n");
			break;
	}
	return ret;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


