/******************************************************************************
 * Copyright (C), 2008-2011, Grandstream Co., Ltd.
 ******************************************************************************
 File Name     : gs_rtsp.c
 Version       : Initial Draft
 Author        : RaySharp video software group
 Created       : 2009/11/19
 Last Modified :
 Description   : rtsp传输协议
 Function List :
 Note		  : created 2009/11/19
 History       :
 1.Date        : 2009/11/19
   Author      : 
   Modification:
 ******************************************************************************/

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

#include <pthread.h>

#include "rs_rtsp_common.h"
#include "rs_rtsp_base64.h"
#include "rs_rtsp.h"
#include "rs_rtp.h"
#include "rs_type.h"
//#include "rs_cfg.h"
//#include "rs_uctrl.h"
#include "md5.h"

#include <pthread.h>
//#include <avserver_ui.h>
#include <g711.h>

#include <ringbuf.h>

//#include "audioLib.h"


#include "rtcp.h"

#define MODLE_NAME		"RTSP "

//#define PRINT_RTSP		1
#define MAX_CLIENTS		20	/* RTSP允许的最大连接用户数 */

RtspSvr_t	gRtspVodSvr[VOD_SVR_TYPE_MAX];
RtspMethod_t gRtspMethod[] = {
	{RTSP_METHOD_PLAY, 			RTSP_REQ_METHOD_PLAY},
   	{RTSP_METHOD_PAUSE, 		RTSP_REQ_METHOD_PAUSE},
   	{RTSP_METHOD_DESCRIBE, 		RTSP_REQ_METHOD_DESCRIBE},
   	{RTSP_METHOD_SETUP, 		RTSP_REQ_METHOD_SETUP},
   	{RTSP_METHOD_REDIRECT, 		RTSP_REQ_METHOD_REDIRECT},
   	{RTSP_METHOD_SESSION, 		RTSP_REQ_METHOD_SESSION},
   	{RTSP_METHOD_OPTIONS, 		RTSP_REQ_METHOD_OPTIONS},
   	{RTSP_METHOD_TEARDOWN, 		RTSP_REQ_METHOD_TEARDOWN},
   	{RTSP_METHOD_RECORD, 		RTSP_REQ_METHOD_RECORD},
   	{RTSP_METHOD_GET_PARAM, 	RTSP_REQ_METHOD_GET_PARAM},
   	{RTSP_METHOD_SET_PARAM, 	RTSP_REQ_METHOD_SET_PARAM},
   	{RTSP_METHOD_EXT_METHOD, 	RTSP_REQ_METHOD_EXTENSION},
   	{0, 						RTSP_PARSE_INVALID_OPCODE}
};

RtspMethod_t gRtspStatu[] = {
	{"Continue", 100},
	{"OK", 200},
	{"Created", 201},
	{"Accepted", 202},
	{"Non-Authoritative Information", 203},
	{"No Content", 204},
	{"Reset Content", 205},
	{"Partial Content", 206},
	{"Multiple Choices", 300},
	{"Moved Permanently", 301},
	{"Moved Temporarily", 302},
	{"Bad Request", 400},
	{"Unauthorized", 401},
	{"Payment Required", 402},
	{"Forbidden", 403},
	{"Not Found", 404},
	{"Method Not Allowed", 405},
	{"Not Acceptable", 406},
	{"Proxy Authentication Required", 407},
	{"Request Time-out", 408},
	{"Conflict", 409},
	{"Gone", 410},
	{"Length Required", 411},
	{"Precondition Failed", 412},
	{"Request Entity Too Large", 413},
	{"Request-URI Too Large", 414},
	{"Unsupported Media Type", 415},
	{"Bad Extension", 420},
	{"Invalid Parameter", 450},
	{"Parameter Not Understood", 451},
	{"Conference Not Found", 452},
	{"Not Enough Bandwidth", 453},
	{"Session Not Found", 454},
	{"Method Not Valid In This State", 455},
	{"Header Field Not Valid for Resource", 456},
	{"Invalid Range", 457},
	{"Parameter Is Read-Only", 458},
	{"Internal Server Error", 500},
	{"Not Implemented", 501},
	{"Bad Gateway", 502},
	{"Service Unavailable", 503},
	{"Gateway Time-out", 504},
	{"RTSP Version Not Supported", 505},
	{"Extended Error:", 911},
	{0, RTSP_PARSE_INVALID_OPCODE}
};

char *gRtspInvalidMethod = "Invalid Method";

#define SND_MSG_KEY2	((key_t)0x31)
#define AUD_STREAM_NEWFRAME2 0x800
#define AUD_STREAM_STOP2     0x801

typedef struct TalkHead_s{
	uLong		len:10;
	uLong		reserved:22;
	uLong		timestamp;
}TalkHead_t;

typedef struct {
         long    mtype;
         int     msg;
         int 	 sampleRate;
} message_buf2;

static pthread_t g_talkThrId;
static int g_talkInUse=0;
static int g_talkClientFd=0;

int snd_msg_qid;


static int snd_msg_qid2;
static message_buf2  sbuf2;
static message_buf2  rbuf2;
unsigned char sharedAudioData[4096];

message_buf2  sbuf;
message_buf2  rbuf;
//;shared_audio_dat* sharedAudioData = NULL;


static int g_rtspSvrPort = 554;
static int g_rtspNeedAuthentication = 0;
static int g_rtspNumOfStream = 3;

typedef struct STREAM_INFO
{
	int imageWidth;
	int imageHeight;
	int frameRate;
	int bitrate;
	
}STREAM_INFO;

static STREAM_INFO g_streamInfo[4];

int RTSP_SetStreamInfo( int chn, int w, int h, int framerate, int bitrate )
{
	if ( chn < 0 || chn >= 3 ) {
		fprintf(stderr, "RTSP_SetStreamInfo invalid chn - [%d] \n", chn);
		return -1;
	}

	g_streamInfo[chn].bitrate = bitrate;
	g_streamInfo[chn].frameRate = framerate;
	g_streamInfo[chn].imageWidth = w;
	g_streamInfo[chn].imageHeight = h;
	
	return 1;
}



int RTSP_SetServerPort( int port )
{

	if ( port < 0 || port > 65535 ) {
		fprintf(stderr, "RTSP_SetServerPort port -[%d] \n", port);
		return -1;
	}
	g_rtspSvrPort = port;
	
}

int RTSP_SetTotalStreamNum( int totalStream )
{

	if ( totalStream < 0 || totalStream > 3 ) {
		fprintf(stderr, "RTSP_SetStreamNum invalid totalStream - [%d] \n", totalStream);
		return -1;
	}

	g_rtspNumOfStream = totalStream;

}

int RTSP_SetAuthentication( int need )
{

	g_rtspNeedAuthentication = need;

}

int RTSP_GetClientNum(int chn)
{
	int i;
	int cnt = 0;
	if(chn == 0){
		cnt = gRtspVodSvr[VOD_SVR_TYPE_RTSP].clientCnt[chn];
	}else{
		for(i=1; i<5; i++){
			cnt += gRtspVodSvr[VOD_SVR_TYPE_RTSP].clientCnt[i];
		}
	}
	return cnt;
}

int RTSP_GetUdpSendStat(int chn)
{
	//return gRtspVodSvr[VOD_SVR_TYPE_RTSP].udpSendActive;
	return gRtspVodSvr[VOD_SVR_TYPE_RTSP].pRtpUdpSender[chn][RTP_STREAM_VIDEO]->bActive;
}

void RTSP_SetUdpSendActive(int chn, int isActive)
{
	if (gRtspVodSvr[VOD_SVR_TYPE_RTSP].pRtpUdpSender[chn][RTP_STREAM_AUDIO] ) {
		gRtspVodSvr[VOD_SVR_TYPE_RTSP].pRtpUdpSender[chn][RTP_STREAM_AUDIO]->bActive = isActive;
	}

	if (gRtspVodSvr[VOD_SVR_TYPE_RTSP].pRtpUdpSender[chn][RTP_STREAM_VIDEO]  ) {		
		gRtspVodSvr[VOD_SVR_TYPE_RTSP].pRtpUdpSender[chn][RTP_STREAM_VIDEO]->bActive = isActive;
	}	
}

RtspSession_t *RTSP_GetSessPtrBySessId(char *id)
{
	struct list_head *pos, *q;
	RtspSession_t *pSess;

	list_for_each_safe(pos, q,
		&gRtspVodSvr[VOD_SVR_TYPE_RTSP].rtspSessWorkList.rtspSessList){
		pSess = list_entry(pos, RtspSession_t, rtspSessList);
		if(pSess != NULL){
			if(0 == strcmp(pSess->sessId, id)){
				return pSess;
			}
		}
	}
	return NULL;
}

static int RTSP_GetPeerInfo(int sockFd, char *ipAddr, int *port)
{
	socklen_t addrLen;
	struct sockaddr_in cliAddr;

	addrLen = sizeof(cliAddr);
	if(-1 == getpeername(sockFd, (struct sockaddr *)&cliAddr, &addrLen)){
		rtsp_dbg(Err, DbgPerror, "getpeername error \n");
		return GS_FAIL;
	}
	strncpy(ipAddr, (const char *)inet_ntoa(cliAddr.sin_addr), 64);
	*port = (int)ntohs(cliAddr.sin_port);

	return GS_SUCCESS;
}

static int RTSP_GetHostInfo(int sockFd, char *ipAddr)
{
	socklen_t addrLen;
	struct sockaddr_in hostAddr;

	addrLen = sizeof(hostAddr);
	if( -1 == getsockname(sockFd, (struct sockaddr *)&hostAddr, &addrLen)){
		rtsp_dbg(Err, DbgPerror, "getsockname error \n");
		return GS_FAIL;
	}
	strncpy(ipAddr, (const char *)inet_ntoa(hostAddr.sin_addr), 64);
	return GS_SUCCESS;
}

void RTSP_GetSessionId(char *sessId, int len)
{
	int i;
	for(i=0; i<len; i++){
		sessId[i] = (char )((random()%10) + '0');
	}
	sessId[len] = 0;
}

void RTSP_SetAllSockFd(fd_set *sockFds, int *maxFd,
												RtspSvr_t *pRtspSvr)
{
	RtspSession_t *pSess = NULL;
	struct list_head *pos, *n;

	list_for_each_safe(pos, n, &(pRtspSvr->rtspSessWorkList.rtspSessList)){
		pSess = list_entry(pos, RtspSession_t, rtspSessList);
		if(pSess != NULL){
			if(pSess->rtspSockFd > 0){
				FD_SET(pSess->rtspSockFd, sockFds);
				*maxFd = (*maxFd >= pSess->rtspSockFd) ? *maxFd :
													pSess->rtspSockFd;
			}
		}
	}
}

int RTSP_GetConnectType(const char *recvBuf)
{
	if(NULL != strstr(recvBuf, "rtsp")){
		return CON_TYPE_RTSP;
	}else if(NULL != strstr(recvBuf, UCTRL_HEAD)){
		return CON_TYPE_UCTRL;
	}
	
	return CON_TYPE_UNDEFINED;
}

int RTSP_GetReq(char *buf)
{
	int seq;
	int cnt;
	char method[32];
	char object[256];
	char ver[32];
	RtspMethod_t *m;

	*method = *object = '\0';
	seq = 0;

	cnt = sscanf(buf, " %31s %255s %31s", method, object, ver);
	if(cnt != 3){
		rtsp_dbg(Warn, DbgNoPerror, "\n---------------------\n""buf: %s\n----------------------\n""is not a valid req message \n", buf);
	}

	for(m=gRtspMethod; m->opcode != -1; m++){
		if(!strcmp(m->describe, method)){
			break;
		}
	}
	return (m->opcode);
}

int RTSP_GetCseq(char *buf)
{
	int cseq = -1;
	char  trash[255];
	char  *pTmp = NULL;

	pTmp = strstr(buf, RTSP_HDR_CSEQ);
	if(pTmp == NULL){
		rtsp_dbg(Err, DbgNoPerror, "not found 'Cseq'. \n""----------------------------------\n""%s\n------------------------------\n", buf);
	}else{
		if(2 != sscanf(pTmp, "%254s %d", trash, &cseq)){
			rtsp_dbg(Err, DbgNoPerror, "not found 'Cseq'. \n""----------------------------------\n""%s\n------------------------------\n", pTmp);
		}
	}
	return cseq;
}

char *RTSP_GetMethodDescrib(u32 code)
{
	RtspMethod_t *pMethod;

	for(pMethod = gRtspStatu; pMethod->opcode != RTSP_PARSE_INVALID_OPCODE;
															pMethod ++){
		if(pMethod->opcode == code){
			return (pMethod->describe);
		}
	}
	return gRtspInvalidMethod;
}

int RTSP_GetHead(int err, RtspSession_t *pSess)
{
	char *pTmp = NULL;

	RTSP_CLEAR_SENDBUF(pSess);
	pTmp = pSess->sendBuf;
	pTmp += sprintf(pTmp, "%s %d %s\r\n", RTSP_VERSION, err,
											RTSP_GetMethodDescrib( err ));
    	pTmp += sprintf(pTmp,"CSeq: %d\r\n", pSess->lastRecvSeq);
    	pTmp += sprintf(pTmp,"Server: Rtsp Server \r\n");

	return (strlen(pSess->sendBuf));
}

int RTSP_GetCurClientCnt(void)
{
	int n;
	int clients = 0;

	for(n=0; n<MAX_VOD_CHN; n++){
		clients += gRtspVodSvr[VOD_SVR_TYPE_RTSP].clientCnt[n];
	}

	return clients;
}


int RTSP_CheckChn(int chn)
{
	int curClientCnt = 0;


	if(chn<0 || chn> MAX_VOD_CHN){
		return GS_FAIL;
	}

	curClientCnt = RTSP_GetCurClientCnt();
	if(curClientCnt >= MAX_CLIENTS){
		rtsp_dbg(Warn, DbgNoPerror, "too more clients MAX[%d] > %d!! \n",curClientCnt, MAX_CLIENTS);
		return GS_FAIL;
	}

	return GS_SUCCESS;
}


int RTSP_GetChannel(const char *szChn)
{
	int chn = -1;

	if(szChn == NULL){
		return GS_FAIL;
	}else{
		chn = atoi(szChn);
		if(chn<0 || chn >= MAX_VOD_CHN){
			return GS_FAIL;
		}
	}
	return chn;
}

int RTSP_SendReply(int err, int simple, char *addon, RtspSession_t *pSess)
{
	int n;
	char *pTmp = pSess->sendBuf;

	if(simple == 1){
		pTmp += RTSP_GetHead(err, pSess);
	}

	if(addon){
		pTmp += sprintf(pTmp, "%s", addon);
	}

	if(simple){
		strcat(pSess->sendBuf, RTSP_LRLF);
	}

	pSess->readToSend = True;

	n = send(pSess->rtspSockFd, pSess->sendBuf, strlen(pSess->sendBuf), MSG_NOSIGNAL);
	if(n != strlen(pSess->sendBuf)){
		rtsp_dbg(Err, DbgPerror, "only send %d byts, we need send %d bytes!\n",n, strlen(pSess->sendBuf));
		return GS_FAIL;
	}

	#if PRINT_RTSP
	rtsp_dbg(Err, DbgPerror, "\n===================== S->C =====================\n");
	rtsp_dbg(Err, DbgPerror, "rtsp send: %s \n", pSess->sendBuf);
	rtsp_dbg(Err, DbgPerror, "\n===================== S->C =====================\n");
	#endif 

	return GS_SUCCESS;
}


int RTSP_RTCP_Parse( char* buf, int inLen )
{

	rtcp_common_t *rtcp_common_Head = NULL;
	rtcp_t *rtcpHead = NULL;
	int msgLeft = inLen;

	if ( inLen < sizeof( rtcp_common_t) ) { 
		return -1;
	}

	rtcpHead = (rtcp_t*)buf;	

	if ( rtcpHead->common.version != 2 ) { 		
		return -1;
	}

	if ( RTCP_RR == rtcpHead->common.pt ) {
		fprintf(stderr," rr ssrc %d fraction %d lost %d last_seq %d jitter %d lsr %d dlsr %d\n",
		rtcpHead->r.rr.rr[0].ssrc, rtcpHead->r.rr.rr[0].fraction, 
		rtcpHead->r.rr.rr[0].lost, rtcpHead->r.rr.rr[0].jitter, 
		rtcpHead->r.rr.rr[0].lsr, rtcpHead->r.rr.rr[0].dlsr);	

		msgLeft -= ((ntohs(rtcpHead->common.length) + 1) * 4 ) ;
	}	

	if ( RTCP_SDES == rtcpHead->common.pt ) { 		

		if (  RTCP_SDES_CNAME == rtcpHead->r.sdes.item[0].type  ) {
			fprintf(stderr, " sdes src %d type %d length %d data %s \n", rtcpHead->r.sdes.src, 
				rtcpHead->r.sdes.item[0].type, rtcpHead->r.sdes.item[0].length, rtcpHead->r.sdes.item[0].data);
		}
		msgLeft -= ((ntohs(rtcpHead->common.length) + 1) * 4 ) ;
	}

	if ( RTCP_BYE == rtcpHead->common.pt ) {
		fprintf(stderr, "rtcp bye \n");		
		msgLeft -= ntohs(rtcpHead->common.length);
	}	

	//fprintf(stderr, "inLen %d msgLest %d rtcpHead->common.length %d\n", inLen, msgLeft, ntohs(rtcpHead->common.length));

	int useLen = (ntohs(rtcpHead->common.length) + 1) * 4;
	return  RTSP_RTCP_Parse( buf+useLen, msgLeft);


}


int RTSP_RTCP_SendReport( RtspSession_t *pSess )
{
		
	char sendBuf[1024];
	sendBuf[0] = '$';		
	int sendLen = 0;

	sendLen = 4;
	// sr
	rtcp_t *pRtcp = (rtcp_t*)(sendBuf + sendLen);

	pRtcp->common.version = 2;
	pRtcp->common.length = 7;
	pRtcp->common.p = 0;
	pRtcp->common.pt = RTCP_SR;
	pRtcp->common.count = 1;

	sendLen += sizeof(rtcp_common_t);

	memset(&pRtcp->r.sr, 0, sizeof(pRtcp->r.sr));

	sendLen += sizeof(pRtcp->r.sr);

	// sdes
	pRtcp = (rtcp_t*)(sendBuf + sendLen);

	pRtcp->common.version = 2;
	pRtcp->common.length = 3;
	pRtcp->common.p = 0;
	pRtcp->common.pt = RTCP_SDES;
	pRtcp->common.count = 1;

	sendLen += sizeof(rtcp_common_t);

	pRtcp->r.sdes.src = 0;
	
	pRtcp->r.sdes.item[0].type = RTCP_SDES_CNAME;
	pRtcp->r.sdes.item[0].length = 5;
	strcpy(pRtcp->r.sdes.item[0].data, "dahuc");
	
	sendLen += 16;

	if (  pSess->reqStreamFlag[RTP_STREAM_AUDIO] ) {
		sendBuf[1] = 0x3;	
	}	

	if (  pSess->reqStreamFlag[RTP_STREAM_VIDEO] ) {
		sendBuf[1] = 0x1;		
	}	

	u16 *pSendLen = (u16*)&sendBuf[2];
	*pSendLen  = htons(sendLen - 4 );

	int sendret = send(pSess->rtspSockFd, sendBuf, sendLen,  MSG_NOSIGNAL );
	
}

int RTSP_RecvMsgParse(int *headLen, int *bodyLen,
													RtspSession_t *pSess)
{
	int end;
	int msgEnd;
	int msgLen;
	int msgBodyExist;
	int msgBodyLen;
	int endCount;
	int writeSpace;
	short *pLen ;
	char  c;
	char  *p = NULL;

	if(pSess->recvBuf[0] == '$'){
		pLen = (short *)&pSess->recvBuf[2];
		msgBodyLen = ntohs(*pLen);
		
		if(msgBodyLen <= pSess->inSize){
			if(headLen){
				*headLen = 4;
			}
			if(bodyLen){
				*bodyLen = msgBodyLen;
			}
		}		

		//RTSP_RTCP_Parse( pSess->recvBuf+4, msgBodyLen );		

		//RTSP_RTCP_SendReport(pSess);

		//rtsp_dbg(Err, DbgPerror, "\n111111111111 msgBodyLen %d pSess->inSize %d %s\n", msgBodyLen, pSess->inSize, pSess->recvBuf+4);		
		
		return GS_FAIL;
	}
	msgEnd = msgBodyExist = msgLen = msgBodyLen = 0;

	while(msgLen <= pSess->inSize){
		end = strcspn(&pSess->recvBuf[msgLen], "\r\n");
		if(end > 0){
			msgLen += end;
		}else{
			rtsp_dbg(Err, DbgPerror, "\n222222222222\n");
			return GS_FAIL;
		}

		if(msgLen > pSess->inSize){
			rtsp_dbg(Err, DbgPerror, "\n33333333333 msgLen %d pSess->inSize %d\n", msgLen, pSess->inSize);
			return GS_FAIL;
		}

		endCount = writeSpace = 0;
		while(!msgEnd && ((msgLen + endCount + writeSpace) < pSess->inSize)){
			c = pSess->recvBuf[msgLen + endCount + writeSpace];
			if(c == '\r' || c == '\n'){
				endCount ++;
			}else if ((endCount < 3) && (c == ' ' || c == '\t')){
				writeSpace ++;
			}else{
				break;
			}
		}

		if(endCount > 2 || (endCount == 2 &&
			(pSess->recvBuf[msgLen] == pSess->recvBuf[msgLen + 1]))){
			msgEnd = 1;
		}
		msgLen += endCount + writeSpace;
		if(msgEnd){
			msgLen += msgBodyLen;
			if(msgLen <= pSess->inSize){
				break;
			}
		}

		if(msgLen >= pSess->inSize){

			rtsp_dbg(Err, DbgPerror, "\n33333333333 msgLen %d pSess->inSize %d\n", msgLen, pSess->inSize);
			
			return GS_FAIL;
		}

		if(!msgBodyExist){
			if(!strncasecmp(&pSess->recvBuf[msgLen], RTSP_HDR_CONTENTLENGTH,
											strlen(RTSP_HDR_CONTENTLENGTH))){
				msgBodyExist = 1;
				msgLen += strlen(RTSP_HDR_CONTENTLENGTH);
				while(msgLen < pSess->inSize){
					c = pSess->recvBuf[msgLen];
					if(c == ':' || c == ' '){
						msgLen ++;
					}else{
						break;
					}

					if( -1 == sscanf(&pSess->recvBuf[msgLen], "%d",
															&msgBodyLen)){
						rtsp_dbg(Dbg, DbgNoPerror, "Invalid ContentLength ""encountered in messag");
						return GS_FAIL;
					}
				}
			}
		}

	}

	if(headLen){
		*headLen = msgLen - msgBodyLen;
	}

	if(bodyLen){
		for(endCount = pSess->inSize - msgLen, p = &pSess->recvBuf[msgLen];
			endCount && (*p == '\0'); p++, msgBodyLen ++, endCount --);
		*bodyLen = msgBodyLen;
	}

	return GS_SUCCESS;
}

int RTSP_ResponseMsgCheck(int *stat, char *buf)
{
	u32 state;
	char  version[32];
	char  trash[256];
	u32 parameterNum;

	parameterNum = sscanf(buf, " %31s %u %255s ", version, &state, trash);
	if(strncasecmp(version, "RTSP/", 5)){
		return RTSP_PARSE_ISNOT_RESP;
	}

	if(parameterNum < 3 || state == 0){
		return RTSP_PARSE_ISNOT_RESP;
	}

	*stat = state;
	return RTSP_PARSE_IS_RESP;
}

int RTSP_ParseUrl(int *port, char *server, char *fileName, const char *url)
{
	int ret = GS_FAIL;
	int havePort = 0;
	int len, n;
	char *pTmp = NULL;
	char *pStr = NULL;
	char *pEnd, *pPort;
	char *full = (char *)malloc(strlen(url) + 1);


	*port = DEFAULT_RTSP_PORT;
	strcpy(full, url);

	if(0 == strncmp(full, "rtsp://", 7)){
		pStr = (char *)malloc(strlen(url) + 1);
		if(pStr != NULL){
			strcpy(pStr, &full[7]);
			if(strchr(pStr, '/')){
				len = 0;
				pEnd = strchr(pStr, '/');
				len = pEnd - pStr;
				for(n=0; n<strlen(url); n++){
					pStr[n] = 0;
				}
				strncpy(pStr, &full[7], len);
			}

			if(strchr(pStr, ':')){
				havePort = 1;
			}
			free(pStr);
			pStr = NULL;
			pTmp = strtok(&full[7], " :/\t\n");
			if(pTmp != NULL){
				strcpy(server, pTmp);
				if(havePort){
					pPort = strtok(&full[strlen(server) + 7 + 1], " /\t\n");
					if(pPort != NULL){
						*port = atol(pPort);
					}
				}
				pTmp = strtok(NULL, " ");
				if(pTmp ){
					strcpy(fileName, pTmp);
				}else{
					fileName[0] = '\0';
				}
				ret = GS_SUCCESS;
			}
		}else{
			rtsp_dbg(Err, DbgPerror, "malloc error \n");
			ret = GS_FAIL;
		}
	}else{
		pTmp = strtok(full, "\t\n");
		if(pTmp){
			strncpy(fileName, pTmp, 16);
			server[0] = '\0';
			ret = GS_SUCCESS;
		}
	}
	free(full);
	full = NULL;

	return ret;
}

int RTSP_PraseUserPwd(char *buff, char *name, char *passwd, char *uri)
{
	char *p,*q,*r = NULL;
	char *outer_ptr = NULL;
	char *inner_ptr = NULL;
	p = strstr(buff, RTSP_HDR_AUTHORIZATION);
	p = p+22;//p 指向username
	r = p;

	while ((p = strtok_r(p,",",&outer_ptr)) != NULL)
	{
			q = strtok_r(p,"=",&inner_ptr);
			if(0 == strcmp(q,"username"))
			{
				if(NULL != (q = strtok_r(NULL,"=",&inner_ptr)))
				{
					q = q+1;
					strncpy(name,q,LEN_32-1);
					name[LEN_32-1] = 0;
					q = strrchr(name,'"');
					if(q)
						*q = 0;
				}
				else
				{
					memset(name,0,LEN_32);
					return GS_FAIL;
				}
				p = NULL;
			}
			else if(0 == strcmp(q+1,"uri"))
			{
				if(NULL != (q = strtok_r(NULL,"=",&inner_ptr)))
				{
					q = q+1;
					strncpy(uri,q,127);
					uri[127-1] = 0;
					q = strrchr(uri,'"');
					if(q)
						*q = 0;

				}
				else
				{
					memset(uri,0,sizeof(uri));
					return GS_FAIL;
				}
				p = NULL;
			}
			else if(0 == strcmp(q+1,"response"))//在分割时 response前有一个空格
			{
				if(NULL != (q = strtok_r(NULL,"=",&inner_ptr)))
				{
					q = q+1;
					strncpy(passwd,q,LEN_64-1);
					passwd[LEN_64-1]=0;
					q = strrchr(passwd,'"');
					if(q)
						*q = 0;
				}
				else
				{
					memset(passwd,0,LEN_64);
					return GS_FAIL;
				}
				p = NULL;
			}

			p = NULL;
	}


	return GS_SUCCESS;
}

void RTSP_SessListInit(RtspSvr_t *pRtspSvr)
{
	INIT_LIST_HEAD(&pRtspSvr->rtspSessWorkList.rtspSessList);
}

void RTSP_SessListLock(void )
{
	pthread_mutex_lock(&gRtspVodSvr[VOD_SVR_TYPE_RTSP].rtspListMutex);
}

void RTSP_SessListUnlock(void)
{
	pthread_mutex_unlock(&gRtspVodSvr[VOD_SVR_TYPE_RTSP].rtspListMutex);
}

void RTSP_SessListAdd(RtspSession_t *pSess)
{
	if(pSess != NULL){
		//dbg(Err, DbgNoPerror, "RTSP_SessListLock ...... \n");
		RTSP_SessListLock();
		//dbg(Err, DbgNoPerror, "RTSP_SessListLock ......ok \n");
		list_add_tail(&pSess->rtspSessList,
				&gRtspVodSvr[pSess->svrType].rtspSessWorkList.rtspSessList);
		//dbg(Err, DbgNoPerror, "RTSP_SessListUnlock ...... \n");
		RTSP_SessListUnlock();
		//dbg(Err, DbgNoPerror, "RTSP_SessListUnlock ...... ok\n");
	}
}

void RTSP_SessListDel(RtspSession_t *pSess)
{
	if(pSess != NULL){
		RTSP_SessListLock();
		if((pSess->channel >= 0) && (pSess->sessStat >= RTSP_STATE_PLAY)){
			gRtspVodSvr[pSess->svrType].clientCnt[pSess->channel] --;
		}
		list_del(&pSess->rtspSessList);
		free(pSess);
		pSess = NULL;
		RTSP_SessListUnlock();
	}
}

void RTSP_SessListPrint(RtspSvr_t *pRtsp)
{
	RtspSession_t *pSess;
	struct list_head *pos, *n;
	int ch;

	//dbg(Dbg, DbgNoPerror, "RtspClientList:\n");
	for(ch=0; ch<MAX_VOD_CHN; ch++){
		if(pRtsp->clientCnt[ch] > 0){
			printf("[Rtsp: %d]: %d clients \n", ch, pRtsp->clientCnt[ch]);
		}
	}
	//dbg(Dbg, DbgNoPerror, "Clients ip: \n");
	list_for_each_safe(pos, n, &(pRtsp->rtspSessWorkList.rtspSessList)){
		pSess = list_entry(pos, RtspSession_t, rtspSessList);
		if(pSess != NULL){
			printf("[Rtsp: %d]: [%s: %s] \n", pSess->channel,
				pSess->transportType == RTP_TRANSPORT_TYPE_UDP ? "Udp": "Tcp",
				pSess->remoteIp);
		}
	}
}

RtspSession_t *RTSP_SessionCreate(RtspSvr_t *pRtsp, int sockFd)
{
	RtspSession_t *pSess = NULL;

	if( sockFd <= 0){
		rtsp_dbg(Err, DbgNoPerror, "sock = %d , error!!!\n", sockFd);
		return NULL;
	}

	pSess = (RtspSession_t *)malloc(sizeof(RtspSession_t));
	if(pSess == NULL){
		rtsp_dbg(Err, DbgPerror, "malloc pSess error\n");
		return NULL;
	}
	memset(pSess, 0, sizeof(RtspSession_t));
	pSess->setupFlag[0] = 0;
	pSess->setupFlag[1] = 0;
	pSess->rtspSockFd 	= sockFd;
	pSess->svrType 		= pRtsp->svrType;
	pSess->channel		= -1;
	pSess->sessStat		= RTSP_STATE_INIT;
	if( GS_SUCCESS != RTSP_GetPeerInfo(sockFd, pSess->remoteIp,
												&pSess->remotePort)){
		rtsp_dbg(Err, DbgNoPerror, "RTSP_GetPeerInfo error\n ");
		free(pSess);
		pSess = NULL;
	}

	if( GS_SUCCESS != RTSP_GetHostInfo(sockFd, pSess->hostIp)){
		rtsp_dbg(Err, DbgNoPerror, "RTSP_GetHostInfo error\n ");
		free(pSess);
		pSess = NULL;
	}

	pSess->readrbHandle = malloc(sizeof(ringbuf_rlink_t));
	if( GS_SUCCESS != ringbuf_read_add(pRtsp->ringbufHandle, (ringbuf_rlink_t *)pSess->readrbHandle)){
		rtsp_dbg(Err, DbgNoPerror, "ringbuf_read_add error\n ");
		free(pSess);
		pSess = NULL;
	}
	
	RTSP_GetSessionId(pSess->sessId, 8);
	RTSP_SessListAdd(pSess);
	return pSess;
}

int RTSP_SessionClose(RtspSession_t *pSess)
{
	RtpUdpSender_t *pSender = NULL;
	if(pSess == NULL){
		//dbg(Err, DbgNoPerror, "pSess == NULL \n");
		return GS_FAIL;
	}

	if(pSess->sessStat < RTSP_STATE_PLAY){
		if(pSess->rtspSockFd > 0){
			sock_close(pSess->rtspSockFd);
			pSess->rtspSockFd = -1;
		}
		RTSP_SessListDel(pSess);
		return GS_SUCCESS;
	}

	if(RTP_TRANSPORT_TYPE_TCP == pSess->transportType){
		pSess->sessStat = RTSP_STATE_STOP;		

		rtsp_dbg(Dbg, DbgNoPerror, "pSess->sessStat = RTSP_STATE_STOP \n");
		if(pSess->tcpThrId > 0){			
			rtsp_dbg(Dbg, DbgNoPerror, "pthread_join tcp thread id = %d\n",(int)pSess->tcpThrId);
			pthread_join(pSess->tcpThrId, NULL);			
			pSess->tcpThrId = -1;
			rtsp_dbg(Dbg, DbgNoPerror, "pthread_join ok\n");
		}

		if(pSess->pRtpTcpSender != NULL){
			//dbg(Dbg, DbgNoPerror, "free(pSess->pRtpTcpSender) \n");
			free(pSess->pRtpTcpSender);
			pSess->pRtpTcpSender = NULL;
		}
		if(pSess->rtspSockFd > 0){
			sock_close(pSess->rtspSockFd);
			pSess->rtspSockFd = -1;
		}
		if(gRtspVodSvr[VOD_SVR_TYPE_RTSP].clientCnt[pSess->channel] <= 0){
			 /* stop sub video stream encode  */
		}
	} else {
		/* 先停音频 */
		pSender = VOD_GetSenderPtr(pSess->svrType, pSess->channel, RTP_STREAM_AUDIO);
		RTSP_SetUdpSendActive(pSess->channel, False);
		if(pSender != NULL){
			if(GS_SUCCESS != RTP_DelUdpSender(pSess->remoteIp,
						pSess->remoteRtpPort[RTP_STREAM_AUDIO], pSender)){
				rtsp_dbg(Err, DbgNoPerror, "RTP_DelUdpSender audio error \n");
				RTSP_SetUdpSendActive(pSess->channel, True);
				return GS_FAIL;
			}
		}

		/* 再停视频 */
		pSender = VOD_GetSenderPtr(pSess->svrType, pSess->channel, RTP_STREAM_VIDEO);
		if(pSender != NULL){
			if(GS_SUCCESS != RTP_DelUdpSender(pSess->remoteIp,
						pSess->remoteRtpPort[RTP_STREAM_VIDEO], pSender)){
				rtsp_dbg(Err, DbgNoPerror, "RTP_DelUdpSender video error \n");
				RTSP_SetUdpSendActive(pSess->channel, True);
				return GS_FAIL;
			}
		}
		RTSP_SetUdpSendActive(pSess->channel, True);
	}
	if (NULL != pSess->readrbHandle) {
		ringbuf_read_del(pSess->readrbHandle);
		free(pSess->readrbHandle);
		pSess->readrbHandle = NULL;
	}
	RTSP_SessListDel(pSess);
	return GS_SUCCESS;
}

void RTSP_SessionStatCheck(RtspSvr_t *pRtspSvr)
{
	struct list_head *pos, *n;
	RtspSession_t *pRtspSess = NULL;
	list_for_each_safe(pos, n, &pRtspSvr->rtspSessWorkList.rtspSessList){
		pRtspSess = list_entry(pos, RtspSession_t, rtspSessList);
		if(pRtspSess != NULL){
			if(pRtspSess->sessStat == RTSP_STATE_STOP){
				rtsp_dbg(Dbg, DbgNoPerror, "close session \n");
				if(GS_SUCCESS != RTSP_SessionClose(pRtspSess)){
					rtsp_dbg(Err, DbgNoPerror, "RTSP_SessionClose error. \n");
				}
				rtsp_dbg(Dbg, DbgNoPerror, "RTSP_SessListPrint \n");
				RTSP_SessListPrint(pRtspSvr);
			}
		}
	}
}


int RTSP_EventHandleTeamdown(RtspSession_t *pSess)
{
	int ret;
	char *pTmp = pSess->sendBuf;

	pTmp += RTSP_GetHead( RTSP_STATUS_OK, pSess);
	pTmp += sprintf(pTmp,"Session: %s\r\n", pSess->sessId);
	pTmp += sprintf(pTmp,"Connection: Close\r\n\r\n");
	ret = RTSP_SendReply( RTSP_STATUS_OK, 0, NULL, pSess);
	if(ret == GS_SUCCESS){
		pSess->sessStat = RTSP_STATE_STOP;
	}
	return ret;
}

int RTSP_EventHandlePause(RtspSession_t *pSess)
{
	return RTSP_SendReply(455, 1, "Accept: OPTIONS, SETUP, "
												"PLAY, TEARDOWN\r\n", pSess);
}

int RTSP_EventHandleUnknown(RtspSession_t *pSess)
{
	return RTSP_SendReply(501, 1, "Accept: OPTIONS, SETUP, "
												"PLAY, TEARDOWN\r\n", pSess);
}

int RTSP_EventHandleOptions(RtspSession_t *pSess)
{
	int station;

	station = RTSP_GetHead(RTSP_STATUS_OK, pSess);
	sprintf(pSess->sendBuf + station,
						"Public: OPTIONS, DESCRIBE, SETUP, PLAY, TEARDOWN, SET_PARAMETER\r\n\r\n");
	return RTSP_SendReply(RTSP_STATUS_OK, 0, NULL, pSess);
}

int RTSP_EventHandleSetParam(RtspSession_t *pSess)
{
	char *pTmp = pSess->sendBuf;

	pTmp += RTSP_GetHead( RTSP_STATUS_OK, pSess);
	/*末尾表示结束的回车换行符不能省掉，否则观看视频会出错*/
	pTmp += sprintf(pTmp,"Session: %s\r\n\r\n", pSess->sessId);

	return RTSP_SendReply(RTSP_STATUS_OK, 0, NULL, pSess);
}

int RTSP_EventHandleDescribe(RtspSession_t *pSess)
{
	int  port, chn, ch, vencType;
	int  isAnonymousDisable = 0, frameRate = 0;
	char surveServer[64];
	char server[128];
	char szChn[16];
	char url[256];
	char uri[128];
	char name[LEN_32];
	char pwd[LEN_64];
	char md5Pwd[LEN_64];
	MD5_CTX 	md5p;
	//UserPwd_t   userInfo;
	char *pSdp = NULL;
	char *pTmp = NULL;
	char *p = NULL;
	int width, height;
	int urlLen;

	if(pSess == NULL){
		//dbg(Err, DbgNoPerror, "Input params is NULL \n");
		return GS_FAIL;
	}

	memset(&md5p, 0, sizeof(MD5_CTX));

	if(!sscanf(pSess->recvBuf, " %*s %254s ", url)){
		RTSP_SendReply(RTSP_STATUS_BAD_REQUEST, 1, NULL, pSess);
		rtsp_dbg(Err, DbgNoPerror, "sscanf url error \n");
		return GS_FAIL;
	}

	urlLen = strlen(url);
	if(url[urlLen-2] != '/'){
		RTSP_SendReply(RTSP_STATUS_BAD_REQUEST, 1, NULL, pSess);
		rtsp_dbg(Err, DbgNoPerror, "sscanf url error \n");
		return GS_FAIL;
	}
	if(url[urlLen-1] != '0' && url[urlLen-1] != '1' && url[urlLen-1] != '2'){
		RTSP_SendReply(RTSP_STATUS_BAD_REQUEST, 1, NULL, pSess);
		rtsp_dbg(Err, DbgNoPerror, "sscanf url error \n");
		return GS_FAIL;
	}

	if( GS_SUCCESS != RTSP_ParseUrl(&port, server, szChn, url)){
		rtsp_dbg(Err, DbgNoPerror, "url is not found !\n");
		RTSP_SendReply(RTSP_STATUS_BAD_REQUEST, 1, NULL, pSess);
		return GS_FAIL;
	}

	chn = RTSP_GetChannel(szChn);
	if(chn == GS_FAIL){
		RTSP_SendReply(RTSP_STATUS_BAD_REQUEST, 1, NULL, pSess);
		rtsp_dbg(Err, DbgNoPerror, "RTSP_GetChannel error\n");
		return GS_FAIL;
	}
	if(chn == 0){
		ch = 0;
	}else if(chn == 1){
		ch = 1;
	}
	else if(chn == 2){
		ch = 2;
	}else{
		ch = 0;
	}

	if (ch + 1 > g_rtspNumOfStream) {
		RTSP_SendReply(404, 1, NULL, pSess);  //404. Not Found
				return GS_FAIL; 
	}

	//width = gAVSERVER_UI_ctrl.avserverConfig.encodeConfig[ch].cropWidth;
	//height = gAVSERVER_UI_ctrl.avserverConfig.encodeConfig[ch].cropHeight;

	width = 1280;
	height = 720;

//	width = g_streamInfo[chn].imageWidth;
//	height = g_streamInfo[chn].imageHeight;

	//codeLib_VideoGetResolution(ch, &width, &height );
	
	
	printf("width: %d, height: %d ~~~~~~~~~~~~~~~~\n", width, height);

	if(NULL != strstr(pSess->recvBuf, RTSP_HDR_ACCEPT)){
		if(NULL == strstr(pSess->recvBuf, RTSP_SDP)){
			rtsp_dbg(Err, DbgNoPerror, "only accept require. \n");
			RTSP_SendReply(551, 1, NULL, pSess);
			return GS_FAIL;
		}
	}

	pTmp = strstr(pSess->recvBuf, RTSP_HDR_USER_AGENT);
	if(pTmp != NULL){
		p = pTmp;
		while(0 != (char)(*p)){
			if((char)(*p) == 10 || (char)(*p) == 13){
				break;
			}

			if((char)(*p) == ':'){
				pTmp = p + 2;
			}
			p ++;
		}
		if(p != pTmp){
			strncpy(pSess->userAgent, pTmp, p - pTmp);
		}
	}
	//isAnonymousDisable = CFG_IsAnonymousDisable();
	if( isAnonymousDisable == IS_UNABLE ){
		#if 1   //如果要进行onvif test 就不能进行用户名密码认证
		/* 添加用户名密码认证 2010-01-20 * lwx */
		memset(surveServer, 0, 64);
		strncpy(surveServer, "Surveillance Server", 20);
		pTmp = strstr(pSess->recvBuf, RTSP_HDR_AUTHORIZATION);
		if(pTmp == NULL){ /* 匿名用户检查 */
			isAnonymousDisable = IS_ENABLE;//CFG_IsAnonymousDisable();
			/* 匿名用户被禁用 */
			//dbg(Warn, DbgNoPerror, "isAnonymousDisable = %d \n", isAnonymousDisable);
			if(isAnonymousDisable == IS_UNABLE){
				RTSP_GetSessionId(pSess->nonce, 8);
				rtsp_dbg(Dbg, DbgNoPerror, "send 401 \n");
				#if 0
				p = pSess->sendBuf + RTSP_GetHead(401, pSess);
				#else
				p = pSess->sendBuf;
				p += sprintf(p, "%s %d %s\r\n", RTSP_VERSION, 401,RTSP_GetMethodDescrib(401));
				p += sprintf(p, "Cseq: %d \r\n", pSess->lastRecvSeq);
				/* 宽，高，帧率，码率 */
				//frameRate = CFG_GetvideoFrameRate(ch);
				if(frameRate == 0){
					frameRate = 30000;
				}
				p += sprintf(p, "Server: Rtsp Server"
							" %d*%d*%d*%d\r\n", width,
												height,
												frameRate/1000,
												4096);
				#endif

				p += sprintf(p, "WWW-Authenticate: Digest "
					"realm=\"Surveillance Server\", nonce=\"%s\"\r\n\r\n",pSess->nonce);
				strcpy(pSess->uri, url);
				RTSP_SendReply(401, 0, NULL, pSess);

				//dbg(Warn, DbgNoPerror, "The anonymous is disable !!! \n");
				return GS_SUCCESS;
			}
		}else{/* 用户名密码检查 */
			/* 从接收信息中提取用户名密码及 URI 信息 */
			if( GS_SUCCESS == RTSP_PraseUserPwd(pSess->recvBuf, name, pwd, uri)){
			//dbg(Dbg, DbgNoPerror, "User = %s, pwd = %s  \n", name, pwd);
				if(0 != strcmp(name, "guest")){
					/* 从数据库获取对应用户名的密码信息,用户名存在返回 GS_SUCCESS */
					//memset(&userInfo, 0, sizeof(userInfo));
					//if(GS_SUCCESS == DB_GetUserPwd(name, &userInfo))
					{
					/* 密码MD5加密 */
					//dbg(Dbg, DbgNoPerror, "name = %s, dbpwd = %s \n",name, userInfo.pwd);
						memset(md5Pwd, 0, LEN_64);
						//MD5Auth_BuildAuthResonseWithMD5((char *)md5Pwd, LEN_64,
						//		0, name, surveServer, userInfo.pwd, pSess->nonce,
						//		NULL, NULL, NULL, "DESCRIBE", uri);
						/* 密码不正确 */
					//dbg(Dbg, DbgNoPerror, "pwd = %s, md5pwd = %s \n", pwd, md5Pwd);
						//if(0 != strcmp(pwd, md5Pwd)){
						if(1){
							//dbg(Dbg, DbgNoPerror, "send 401 \n");
							#if 0
							p = pSess->sendBuf + RTSP_GetHead(401, pSess);
							#else
							p = pSess->sendBuf;
							p += sprintf(p, "%s %d %s\r\n", RTSP_VERSION, 401,RTSP_GetMethodDescrib(401));
							p += sprintf(p, "Cseq: %d \r\n", pSess->lastRecvSeq);
							/* 宽，高，帧率，码率 */
							//frameRate = CFG_GetvideoFrameRate(ch);
							if(frameRate == 0){
								frameRate = 30000;
							}
							p += sprintf(p, "Server: Rtsp Server"
									" %d*%d*%d*%d\r\n", width,
													height,
													frameRate/1000,
													4096);
							#endif
							RTSP_GetSessionId(pSess->nonce, 8);
							p += sprintf(p, "WWW-Authenticate: Digest "
								"realm=\"Surveillance Server\", "
								"nonce=\"%s\"\r\n\r\n", pSess->nonce);
							strcpy(pSess->uri, url);
							RTSP_SendReply(401, 0, NULL, pSess);
							rtsp_dbg(Err, DbgNoPerror, "You input password is error \n");
							return GS_SUCCESS;
						}
					else{/* 用户名不存在 */
						//dbg(Dbg, DbgNoPerror, "send 401 \n");
						#if 0
						p = pSess->sendBuf + RTSP_GetHead(401, pSess);
						#else
						p = pSess->sendBuf;
						p += sprintf(p, "%s %d %s\r\n", RTSP_VERSION, 401,RTSP_GetMethodDescrib(401));
						p += sprintf(p, "Cseq: %d \r\n", pSess->lastRecvSeq);
						/* 宽，高，帧率，码率 */
						//frameRate = CFG_GetvideoFrameRate(ch);
						if(frameRate == 0){
							frameRate = 30000;
						}
						p += sprintf(p, "Server: Rtsp Server"
								" %d*%d*%d*%d\r\n", width,
												height,
												frameRate/1000,
												4096);
						#endif

						RTSP_GetSessionId(pSess->nonce, 8);
						p += sprintf(p, "WWW-Authenticate: Digest "
							"realm=\"Surveillance Server\", "
							"nonce=\"%s\"\r\n\r\n", pSess->nonce);
						strcpy(pSess->uri, url);
						RTSP_SendReply(401, 0, NULL, pSess);
						rtsp_dbg(Err, DbgNoPerror, "Can`t find you input name in db\n");
						return GS_SUCCESS;
					}
				}
			}else{
				/* 用户名或密码为空 */
			//dbg(Dbg, DbgNoPerror, "2 usr = %s, pwd = %s \n", name, pwd);
			//dbg(Dbg, DbgNoPerror, "send 401 \n");
				#if 0
				p = pSess->sendBuf + RTSP_GetHead(401, pSess);
				#else
				p = pSess->sendBuf;
				p += sprintf(p, "%s %d %s\r\n", RTSP_VERSION, 401,RTSP_GetMethodDescrib(401));
				p += sprintf(p, "Cseq: %d \r\n", pSess->lastRecvSeq);
				/* 宽，高，帧率，码率 */
				//frameRate = CFG_GetvideoFrameRate(ch);
				if(frameRate == 0){
					frameRate = 30000;
				}
				p += sprintf(p, "Server: Rtsp Server"
							" %d*%d*%d*%d\r\n", width,
									    height,
												frameRate/1000,
												4096);
				#endif
				RTSP_GetSessionId(pSess->nonce, 8);
				p += sprintf(p, "WWW-Authenticate: Digest "
					"realm=\"Surveillance Server\", nonce=\"%s\"\r\n\r\n", pSess->nonce);
				strcpy(pSess->uri, url);
				RTSP_SendReply(401, 0, NULL, pSess);
				//dbg(Err, DbgNoPerror, "your username or passworld is NULL\n");
				return GS_SUCCESS;
			}
		}
		#endif
	}
	memset(pSess->sendBuf, 0, sizeof(pSess->sendBuf));
	#if 0
	pTmp = pSess->sendBuf + RTSP_GetHead(200, pSess);
	#else
	pTmp = pSess->sendBuf;
	pTmp += sprintf(pTmp, "%s %d %s\r\n", RTSP_VERSION, 200,
										RTSP_GetMethodDescrib(200));
	pTmp += sprintf(pTmp, "Cseq: %d \r\n", pSess->lastRecvSeq);
	/* 宽，高，帧率，码率 */
	//frameRate = CFG_GetvideoFrameRate(ch);
	if(frameRate == 0){
		frameRate = 30000;
	}
	pTmp += sprintf(pTmp, "Server: Rtsp Server"
					 " %d*%d*%d*%d\r\n", width,
										height,
										frameRate/1000,
										4096);
	#endif
	pTmp += sprintf(pTmp, "Content-Type: application/sdp\r\n");

	pSdp = (char *)malloc(4096);
	if(pSdp == NULL){
		rtsp_dbg(Err, DbgPerror, "malloc pSdp error \n");
		return GS_FAIL;
	}
	p = pSdp;
	p += sprintf(p,"v=0\r\n");
    	p += sprintf(p,"o=StreamingServer 3331435948 1116907222000 IN IP4 %s\r\n",pSess->hostIp);
    	p += sprintf(p,"s=h264.mp4\r\n");
    	p += sprintf(p,"c=IN IP4 0.0.0.0\r\n");
    	p += sprintf(p,"t=0 0\r\n");
    	p += sprintf(p,"a=control:*\r\n");

	/*视频媒体描述*/
    	/*H264 TrackID=0 RTP_PT 96*/
	//vencType = VENC_TYPE_JPEG;//VENC_TYPE_H264;//CFG_GetVideoEncType(ch);

  	//if(gAVSERVER_config.encodeConfig[ch].codecType == ALG_VID_CODEC_H264)
	//	vencType = VENC_TYPE_H264;
	//else	
	//	vencType = VENC_TYPE_JPEG;

	vencType = VENC_TYPE_H264;

	switch(vencType){
		case VENC_TYPE_H264:
		    p += sprintf(p,"m=video 0 RTP/AVP 96\r\n");
		    p += sprintf(p,"a=control:trackID=0\r\n");
		    p += sprintf(p,"a=rtpmap:96 H264/90000\r\n");
		    p += sprintf(p, "a=fmtp:96 packetization-mode=1; "			
				"sprop-parameter-sets=%s\r\n", (char *)RTSP_Media_Para_GetBase64(ch));//加密
			break;

		case VENC_TYPE_JPEG:
			p += sprintf(p,"m=video 0 RTP/AVP 26\r\n");
			p += sprintf(p,"a=control:trackID=0\r\n");
			p += sprintf(p,"a=rtpmap:26 JPEG/90000\r\n");
			break;

		case VENC_TYPE_MPEG4:	//added by xsf for MPEG4
			p += sprintf(p, "m=video 0 RTP/AVP 99\r\n");
			//p += sprintf(p, "m=video 49170/2 RTP/AVP 99\r\n");
			p += sprintf(p, "b=AS:1024\r\n");
			p += sprintf(p, "a=control:trackID=0\r\n");
			p += sprintf(p, "a=rtpmap:99 MP4V-ES/90000\r\n");
			p += sprintf(p, "a=fmtp:99 profile-level-id=1\r\n");
			break;

		default:
			p += sprintf(p,"m=video 0 RTP/AVP 96\r\n");
		        p += sprintf(p,"a=control:trackID=0\r\n");
		        p += sprintf(p,"a=rtpmap:96 H264/90000\r\n");
			p += sprintf(p,"a=fmtp:96 packetization-mode=1; "
				"sprop-parameter-sets=%s\r\n",( char *)RTSP_Media_Para_GetBase64(ch));
			break;
	}
	p += sprintf(p, "a=videoinfo:%d*%d*%d*%d\r\n", width, height, frameRate/1000, 4096);


	#if 0
        p += sprintf(p,"m=video 0 RTP/AVP 96\r\n");
        p += sprintf(p,"a=control:trackID=0\r\n");
        p += sprintf(p,"a=rtpmap:96 H264/90000\r\n");
	#endif

	#if 1
	/*音频媒体描述*/
	//p += sprintf(p,"m=audio 0 RTP/AVP 8\r\n");
	/* RTP/AVP 97 --> G726  */
	/* RTP/AVP 8  --> G711-A */
	/* RTP/AVP 0  --> G711-U */
	/*主码流的声音编码通道, 类型为G711-A */

	//if(AUDIO_ENC_FMT_ALAW == CFG_GetAudioEncType()){
	//if(0){
	/* G711-A */
	p += sprintf(p,"m=audio 0 RTP/AVP 0\r\n");
	p += sprintf(p,"a=control:trackID=1\r\n");
	//p += sprintf(p,"a=rtpmap:0 L16/8000/1\r\n");
	p += sprintf(p,"a=rtpmap:0 PCMU/8000\r\n");
	//}
	/* G726-16 */
        //p += sprintf(p,"a=rtpmap:97 G726-%d/8000\r\n",16);
	//p += sprintf(p,"a=ptime:20\r\n\r\n");
	p += sprintf(p,"a=ptime:20\r\n\r\n");
	#endif

	pTmp += sprintf(pTmp,"Content-length: %d\r\n", strlen(pSdp));
    	pTmp += sprintf(pTmp,"Content-Base: rtsp://%s/%d\r\n\r\n", pSess->hostIp,chn);

	strcat(pTmp, pSdp);
	strcpy(pSess->uri, (const char *)url);

	if(pSdp != NULL){
		free(pSdp);
		pSdp = NULL;
	}
	return RTSP_SendReply(200, 0, NULL, pSess);
}
}
int RTSP_EventHandleSetup(RtspSession_t *pSess)
{
	int ret;
	int svrPort = 0;
	int trackId, chn;
	int rtpPort, rtcpPort;
	char url[255], trash[255];
	char server[255], obj[255], line[255];
	char *p = NULL, *pTmp = NULL;

	if(!sscanf(pSess->recvBuf, " %*s %254s ", url)){
		RTSP_SendReply(400, 1, NULL, pSess);
		return GS_FAIL;
	}

	if(GS_SUCCESS != RTSP_ParseUrl(&svrPort, (char *)server, (char *)obj, (const char *)url)){
		RTSP_SendReply(400, 1, NULL, pSess);
		rtsp_dbg(Err, DbgNoPerror, "RTSP_ParseUrl error \n");
		return GS_FAIL;
	}


	p = strstr(obj, "trackID");
	if(p == NULL){
		rtsp_dbg(Err, DbgNoPerror, "no track id. \n");
		RTSP_SendReply(406, 1, "Require: Transport settings " "of rtp/udp;port=nnnn. ", pSess);
		return GS_FAIL;
	}

	sscanf(p, "%8s%d", trash, &trackId);
	sscanf(obj, "%d/%s", &chn, trash);
	if(GS_FAIL == RTSP_CheckChn(chn)){
		rtsp_dbg(Err,DbgNoPerror, "chn = %d, error \n", chn);
		RTSP_SendReply(400, 1, NULL, pSess);
		return GS_FAIL;
	}

	if(chn <0 || chn > 2){
		chn = 0;
	}
	pSess->channel = chn;

	p = strstr(pSess->recvBuf, RTSP_HDR_TRANSPORT);
	if(p == NULL){
		rtsp_dbg(Err, DbgNoPerror, "get rtp transport type  error \n");
		RTSP_SendReply(406, 1, "Require: Transport settings"
										" of rtp/udp;port=nnnn. ", pSess);
		return GS_FAIL;
	}

	if(trackId == TRACK_ID_VIDEO){
		pSess->reqStreamFlag[RTP_STREAM_VIDEO] = True;
	}else if(trackId == TRACK_ID_AUDIO){
		pSess->reqStreamFlag[RTP_STREAM_AUDIO] = True;
	}else{
		rtsp_dbg(Err, DbgNoPerror, "track id = %d error \n", trackId);
		RTSP_SendReply(400, 1, NULL, pSess);
		return GS_FAIL;
	}
	/*
	 * Transport: RTP/AVP;unicast;client_port=6972-6973;source=10.71.147.222;
	 * 		   server_port=6970-6971;ssrc=00003654
	 * trash = "Transport:"
	 * line = "RTP/AVP;unicast;client_port=6972-6973;source=10.71.147.222;
	 *         server_port=6970-6971;ssrc=00003654"
	 */

	if(2 != sscanf(p, "%10s%255s", trash, line)){
		rtsp_dbg(Err, DbgNoPerror, "setup request malformed \n");
		RTSP_SendReply(400, 1, 0, pSess);
		return GS_FAIL;
	}

	p = strstr(line, "client_port");
	if(p != NULL){
		p = strstr(p, "=");
		sscanf(p+1, "%d", &rtpPort);

		p = strstr(p, "-");
		sscanf(p+1, "%d", &rtcpPort);

		pSess->remoteRtpPort[trackId]  = rtpPort;
		pSess->remoteRtcpPort[trackId] = rtcpPort;
		pSess->transportType = RTP_TRANSPORT_TYPE_UDP;

		pTmp = pSess->sendBuf;
		pTmp += RTSP_GetHead(200, pSess);
		pTmp += sprintf(pTmp,"Session: %s\r\n", pSess->sessId);
		pTmp += sprintf(pTmp,"Transport: RTP/AVP;unicast;client_port=%d-%d;"
					"source=%s;server_port=%d-%d;ssrc=00004E87\r\n\r\n",
					rtpPort, rtcpPort, pSess->hostIp,
					VOD_GetSenderPtr(VOD_SVR_TYPE_RTSP, chn,
											trackId/*RTP_STREAM_VIDEO*/)->rtpPort,
					VOD_GetSenderPtr(VOD_SVR_TYPE_RTSP, chn,
											trackId/*RTP_STREAM_VIDEO*/)->rtcpPort);
	}else{

		p = strstr(line, "/TCP");
		if(p != NULL){
			pSess->transportType = RTP_TRANSPORT_TYPE_TCP;
			pTmp = pSess->sendBuf;
			pTmp += RTSP_GetHead(200, pSess);
			pTmp += sprintf(pTmp,"Session: %s;timeout=120\r\n", pSess->sessId);

			p = strstr(line, "interleaved");
			if(p != NULL){
				if(trackId == TRACK_ID_VIDEO){
					if( pSess->setupFlag[0] == 0 ){
						p = strstr(p, "=");
						if(p != NULL){
							sscanf(p+1, "%d", &pSess->interleaved[RTP_STREAM_VIDEO].rtp);
						}

						p = strstr(p, "-");
						if(p != NULL){
							sscanf(p+1, "%d", &pSess->interleaved[RTP_STREAM_VIDEO].rtcp);
						}else{
							pSess->interleaved[RTP_STREAM_VIDEO].rtcp =
										pSess->interleaved[RTP_STREAM_VIDEO].rtp + 1;
						}

						pTmp += sprintf(pTmp, "Transport: RTP/AVP/TCP;unicast;"
										"interleaved=%d-%d\r\n\r\n",
										pSess->interleaved[RTP_STREAM_VIDEO].rtp,
										pSess->interleaved[RTP_STREAM_VIDEO].rtcp);
						pSess->setupFlag[0] = 1;
					}
				}else if(trackId == TRACK_ID_AUDIO){
					if( pSess->setupFlag[1] == 0 ){
						p = strstr(p, "=");
						if(p != NULL){
							sscanf(p+1, "%d", &pSess->interleaved[RTP_STREAM_AUDIO].rtp);
						}

						p = strstr(p, "-");
						if(p != NULL){
							sscanf(p+1, "%d", &pSess->interleaved[RTP_STREAM_AUDIO].rtcp);
						}else{
							pSess->interleaved[RTP_STREAM_AUDIO].rtcp =
										pSess->interleaved[RTP_STREAM_AUDIO].rtp + 1;
						}

						pTmp += sprintf(pTmp, "Transport: RTP/AVP/TCP;unicast;"
										"interleaved=%d-%d\r\n\r\n",
										pSess->interleaved[RTP_STREAM_AUDIO].rtp,
										pSess->interleaved[RTP_STREAM_AUDIO].rtcp);
						pSess->setupFlag[1] = 1;
					}
				}else{
					rtsp_dbg(Err, DbgNoPerror, "setup chn = %d, unsupported transport\n",chn);
					RTSP_SendReply(461, 1, "Unsupported Transport", pSess);
					return GS_FAIL;
				}
			}
		 }
	}
	ret = RTSP_SendReply(200, 0, NULL, pSess);
	if(ret == GS_SUCCESS){
		pSess->sessStat = RTSP_STATE_READY;
	}
	return ret;
}

int RTSP_EventHandlePlay(RtspSession_t *pSess)
{
	int ret;
	int avType;
	char *pTmp = NULL;
	RtpUdpSender_t *pUdpSender = NULL;
	RtpTargetHost_t *pTarget = NULL;

	if(RTSP_STATE_READY != pSess->sessStat){
		return GS_FAIL;
	}

	/* TCP传输方式 */
	if(RTP_TRANSPORT_TYPE_TCP == pSess->transportType){
		pSess->pRtpTcpSender = (RtpTcpSender_t *)malloc(sizeof(RtpTcpSender_t));
		if(pSess->pRtpTcpSender == NULL){
			rtsp_dbg(Err, DbgPerror, "malloc tcpsender error \n");
			RTSP_SendReply(403, 1, NULL, pSess);
			return GS_FAIL;
		}
		pSess->pRtpTcpSender->interleaved[RTP_STREAM_VIDEO].rtp =
								pSess->interleaved[RTP_STREAM_VIDEO].rtp;
		pSess->pRtpTcpSender->interleaved[RTP_STREAM_VIDEO].rtcp =
								pSess->interleaved[RTP_STREAM_VIDEO].rtcp;
		pSess->pRtpTcpSender->interleaved[RTP_STREAM_AUDIO].rtp =
								pSess->interleaved[RTP_STREAM_AUDIO].rtp;
		pSess->pRtpTcpSender->interleaved[RTP_STREAM_AUDIO].rtcp =
								pSess->interleaved[RTP_STREAM_AUDIO].rtcp;
		pSess->pRtpTcpSender->tcpSockFd = pSess->rtspSockFd;
		pSess->pRtpTcpSender->lastTs = 3600;
		pSess->pRtpTcpSender->AudioSeq = 1200;
		pSess->pRtpTcpSender->audioG711Ssrc = RTP_DEFAULT_SSRC + pSess->channel;
		pSess->pRtpTcpSender->videoH264Ssrc = RTP_DEFAULT_SSRC +
														pSess->channel + 128;
		pSess->sessStat 	= RTSP_STATE_PLAY;
		/* 添加TCP传输线程在这里 */
		//进行传输
		if( GS_SUCCESS != RTP_TcpStartSendMediaData(pSess)){
			rtsp_dbg(Err, DbgNoPerror, "RTP_TcpStartSendMediaData error \n ");
			pSess->sessStat = RTSP_STATE_STOP;
			RTSP_SendReply(911, 1, "create tcp thread error \n", pSess);
		}
	}else{
	/* UDP 传输方式 */
		rtsp_dbg(Dbg, DbgNoPerror, "Add new Udp client %d, chn = %d\n",pSess->svrType, pSess->channel);
		for(avType=0; avType<RTP_STREAM_MAX; avType++){
			pUdpSender = VOD_GetSenderPtr(pSess->svrType, pSess->channel, avType);
 			if(pUdpSender != NULL){
 				if(pSess->reqStreamFlag[avType] == True && pSess->remoteRtpPort[avType] != 0 ){
					pTarget = (RtpTargetHost_t *)RTP_AddUdpSender(
							pSess->remoteIp, pSess->remoteRtpPort[avType], pUdpSender);
					if(pTarget == NULL){
						rtsp_dbg(Err, DbgNoPerror, "RTP_AddUdpSender return NULL \n");
						RTSP_SendReply(403, 1, NULL, pSess);
						return GS_FAIL;
					}
					pSess->rtpTargets[avType] = pTarget;
					pTarget->pRtspSess		  = pSess;
					//pTarget->hostState 		  = RTP_TARGETHOST_STATE_REQ_IFRAME;
					pUdpSender->bActive 	  = True;
				}
			}else{
				printf("pUdpSender is NULL, avType = %d error \n", avType);
			}
		}
	}
	memset(pSess->sendBuf, 0, MAX_RTSP_BUF_LEN);
	pTmp = pSess->sendBuf;
	pTmp += RTSP_GetHead(200, pSess);
	
	pTmp += sprintf(pTmp, "Session: %s;timeout=120\r\n\r\n", pSess->sessId);

	ret = RTSP_SendReply(200, 0, NULL, pSess);
	if(ret == GS_SUCCESS){
		pSess->sessStat = RTSP_STATE_PLAY;
		gRtspVodSvr[pSess->svrType].clientCnt[pSess->channel] ++;
		RTSP_SessListPrint(&gRtspVodSvr[pSess->svrType]);
	}
	return ret;
}

int RTSP_EventHandle(int event, int stat, RtspSession_t *pSess)
{
	int ret;

#if PRINT_RTSP
	rtsp_dbg(Err, DbgPerror, "\n================== C->S ================\n");
	rtsp_dbg(Err, DbgPerror, "rtsp recv:  %s \n", pSess->recvBuf);
	rtsp_dbg(Err, DbgPerror, "\n================ C->S ==================\n");
#endif

	//RTSP流程走一遍
	switch(event){
		case RTSP_REQ_METHOD_OPTIONS:
			ret = RTSP_EventHandleOptions(pSess);
			break;

		case RTSP_REQ_METHOD_TEARDOWN:
			ret = RTSP_EventHandleTeamdown(pSess);
			break;

		case RTSP_REQ_METHOD_SETUP:
			ret = RTSP_EventHandleSetup(pSess);
			break;

		case RTSP_REQ_METHOD_DESCRIBE:
			ret = RTSP_EventHandleDescribe(pSess);
			break;

		case RTSP_REQ_METHOD_PLAY:
			ret = RTSP_EventHandlePlay(pSess);     //传输
			break;

		case RTSP_REQ_METHOD_PAUSE:
			ret = RTSP_EventHandlePause(pSess);
			break;

		case RTSP_REQ_METHOD_SET_PARAM:
			ret = RTSP_EventHandleSetParam(pSess);
			break;

		default:
			ret = RTSP_EventHandleUnknown(pSess);
			break;
	}
	return ret;
}

int RTSP_SessionProcess(RtspSession_t *pSess)
{
	int stat, seqNum = 0;
	int headLen, bodyLen;
	int opcode;
	int cseq = -1;
	if(pSess == NULL){
		//dbg(Err, DbgNoPerror, "pSess == NULL \n");
		return GS_FAIL;
	}
	rtsp_dbg(Info, DbgNoPerror, "\n\n\n ------------- recv buf -------------\n%s"
		"\n-------------------------------------------\n\n\n", pSess->recvBuf);
	if(GS_SUCCESS != RTSP_RecvMsgParse(&headLen, &bodyLen, pSess)){
		rtsp_dbg(Err, DbgNoPerror, "RTSP_RecvMsgParse error \n");
		return GS_SUCCESS;
	}

	if(RTSP_PARSE_IS_RESP == RTSP_ResponseMsgCheck(&stat, pSess->recvBuf)){
		if(seqNum != pSess->lastSendSeq + 1){
			rtsp_dbg(Warn, DbgNoPerror, "last send sn is %d != resp seq = %d\n",pSess->lastSendSeq, seqNum);
		}
		opcode = RTSP_MAKE_RESP_CMD(pSess->lastSendReq);
		if(stat > RTSP_BAD_STATUS_BEGIN){
			rtsp_dbg(Warn, DbgNoPerror, "response had status = %d. \n", stat);
		}
	}else{
		opcode = RTSP_GetReq(pSess->recvBuf);
		if(opcode == RTSP_PARSE_INVALID_OPCODE){
			rtsp_dbg(Err, DbgNoPerror, "method request was invalid.\n""\n-------------------------------------\n"
				"%s"
				"\n-------------------------------------\n", pSess->recvBuf);
			return GS_FAIL;
		}else if(opcode == RTSP_PARSE_INVALID){
			rtsp_dbg(Err, DbgNoPerror, "Bad request line encountered.  ""Expected 4 valid tokens.  Message discarded.\n"
				"------------------------------------\n"
				"%s\n-----------------------------------\n", pSess->recvBuf);
			return GS_FAIL;
		}
		cseq = RTSP_GetCseq(pSess->recvBuf);
		if(cseq > 0){
			pSess->lastRecvSeq = cseq;
		}else{
			rtsp_dbg(Err, DbgNoPerror, "invalid cseq = %d \n", cseq);
		}
		stat = 0;
	}

	return RTSP_EventHandle(opcode, stat, pSess);
}

int RTSP_SvrCreate(int port, int chnNum, VodSvrType_e vodSvrType,
														RtspSvr_t *pRtspSvr)
{
	int	chn;
	int audioType;
	int videoType;
	int ptType;

	RtpUdpSender_t	*pRtpUdpVideoSender = NULL;
	RtpUdpSender_t  *pRtpUdpAudioSender = NULL;
#if 0
	if(AUDIO_ENC_FMT_ALAW == CFG_GetAudioEncType()){
		audioType = RTP_PT_ALAW;
	}else{
		audioType = RTP_PT_ULAW;
	}
#endif
        audioType = RTP_PT_ULAW;

	pRtspSvr->svrType 		= vodSvrType;
	pRtspSvr->vodSvrState 	= VOD_SVR_STATE_INIT;
	pRtspSvr->rtspSvrPort   = port;
	pRtspSvr->rtspSvrSockFd = 0;
	
	memset(pRtspSvr->pRtpUdpSender, 0, sizeof(pRtspSvr->pRtpUdpSender));

	//memset(pRtspSvr->pRtpUdpSender, 0, sizeof(RtpUdpSender_t ));

	fprintf(stderr, "--------------%s %d sizeof(pRtspSvr->pRtpUdpSender ) %d \n", __FILE__, __LINE__, sizeof(pRtspSvr->pRtpUdpSender ));

	/*added by xsf for mpeg4 jpeg*/
	for(chn=0; chn<chnNum; chn++){
		//videoType = VENC_TYPE_H264;//CFG_GetVideoEncType(chn);
  	//	if(gAVSERVER_config.encodeConfig[chn].codecType == ALG_VID_CODEC_H264)
	//		videoType = VENC_TYPE_H264;
	//	else	
	//		videoType = VENC_TYPE_JPEG;	

		videoType = VENC_TYPE_H264;
		
		if(videoType == VENC_TYPE_H264){
			ptType = RTP_PT_H264;
		}else if(videoType == VENC_TYPE_MPEG4){
			ptType = RTP_PT_MPEG4;
		}else{
			ptType = RTP_PT_JPEG;
		}

		if(GS_SUCCESS != RTP_CreateUdpSender(
						&pRtspSvr->pRtpUdpSender[chn][RTP_STREAM_VIDEO],
						chn, 0, ptType)){
			rtsp_dbg(Err, DbgNoPerror, "RTP_CreateUdpSender video error\n");
			return GS_FAIL;
		}
		pRtpUdpVideoSender = pRtspSvr->pRtpUdpSender[chn][RTP_STREAM_VIDEO];
		pRtpUdpVideoSender->avType 	= AV_TYPE_VIDEO;
		pRtpUdpVideoSender->pktType = PACKET_TYPE_RTP;
		pRtpUdpVideoSender->pt		= ptType;

 		if(GS_SUCCESS != RTP_CreateUdpSender(
						&pRtspSvr->pRtpUdpSender[chn][RTP_STREAM_AUDIO],
						chn, 0, audioType)){
			rtsp_dbg(Err, DbgNoPerror, "RTP_CreateUdpSender audio error\n");
			return GS_FAIL;
		}
		pRtpUdpAudioSender = pRtspSvr->pRtpUdpSender[chn][RTP_STREAM_AUDIO];
		pRtpUdpAudioSender->avType	= AV_TYPE_AUDIO;
		pRtpUdpAudioSender->pktType = PACKET_TYPE_RTP;
		pRtpUdpAudioSender->pt 		= audioType;
	}
	pthread_mutex_init(&pRtspSvr->rtspListMutex, NULL);
	RTSP_SessListInit(pRtspSvr);
	return GS_SUCCESS;
}

int RTSP_SvrListen(RtspSvr_t *pRtspSvr)
{
	int sockOptVal = 1;
	struct sockaddr_in svrAddr;

	pRtspSvr->rtspSvrSockFd = socket(PF_INET, SOCK_STREAM, 0);
	if(-1 == pRtspSvr->rtspSvrSockFd){
		rtsp_dbg(Err, DbgPerror, "socket rtspSvrSockFd error \n");
		return GS_FAIL;
	}

	if (-1 == setsockopt(pRtspSvr->rtspSvrSockFd , SOL_SOCKET, SO_REUSEADDR,&sockOptVal, sizeof(int))) {
		rtsp_dbg(Err, DbgPerror, "setsockopt error \n");
		sock_close(pRtspSvr->rtspSvrSockFd);
		pRtspSvr->rtspSvrSockFd = -1;
		return GS_FAIL;
	}

	memset(&svrAddr, 0, sizeof(svrAddr));
	svrAddr.sin_addr.s_addr 	= htonl(INADDR_ANY);
	svrAddr.sin_family 		= AF_INET;
    	svrAddr.sin_port  		= htons(pRtspSvr->rtspSvrPort);

	if (-1 == bind(pRtspSvr->rtspSvrSockFd, (struct sockaddr *)&svrAddr,sizeof(struct sockaddr))){
       		rtsp_dbg(Err, DbgPerror, "bind error , sockfd = %d \n",pRtspSvr->rtspSvrSockFd);
		sock_close(pRtspSvr->rtspSvrSockFd);
		pRtspSvr->rtspSvrSockFd = -1;
		return GS_FAIL;
    }

	if(-1 == listen(pRtspSvr->rtspSvrSockFd,SOMAXCONN)){
		rtsp_dbg(Err, DbgPerror, "listen error \n");
		sock_close(pRtspSvr->rtspSvrSockFd);
		pRtspSvr->rtspSvrSockFd = -1;
		return GS_FAIL;
	}
	return GS_SUCCESS;
}

//void RTSP_ParamsInit(Cfg_t *pCfg)
void RTSP_ParamsInit()
{
	RtspSvr_t *pRtsp = &gRtspVodSvr[VOD_SVR_TYPE_RTSP];
	memset(pRtsp, 0, sizeof(RtspSvr_t));
	//pRtsp->hPause	= &pCfg->thrCtl.pause;
	//pRtsp->rvInit 	= &pCfg->thrCtl.rvInit;
	//pRtsp->rvExit	= &pCfg->thrCtl.rvExit;
	RTP_Base64Init(0);
	RTP_Base64Init(1);
	RTP_Base64Init(2);
}

int RTSP_FreeUdpSender(RtspSvr_t *pRtspSvr, int vodChnNum, int streamNum)
{
	int vodChn, stream;
	for(vodChn=0; vodChn<vodChnNum; vodChn++){
		for(stream=0; stream<streamNum; stream++){
			free(pRtspSvr->pRtpUdpSender[vodChn][stream]);
			pRtspSvr->pRtpUdpSender[vodChn][stream] = NULL;
		}
	}
	return GS_SUCCESS;
}

void RTSP_CloseAllClient(void)
{
	struct list_head *pos, *n;
	RtspSession_t *pSess = NULL;
	RtspSvr_t *pRtspSvr = &gRtspVodSvr[VOD_SVR_TYPE_RTSP];
	list_for_each_safe(pos, n, &pRtspSvr->rtspSessWorkList.rtspSessList){
		pSess = list_entry(pos, RtspSession_t, rtspSessList);
		if(pSess != NULL){
			//dbg(Dbg, DbgNoPerror, "sess is not null ......\n");
			if(GS_SUCCESS != RTSP_SessionClose(pSess)){
				//dbg(Err, DbgNoPerror, "RTSP_SessionClose error. \n");
			}
		}
	}
}

int initTalk()
{
	init_all();

	return 0;
}
    
void freeTalk()
{
	closeRemoteStream();	
  	close_all();
}

int snd_msg2(int qid)
{
	int status;
	int buf_length;
	sbuf2.mtype = 1;
	buf_length = sizeof(sbuf) - sizeof(long);
	status = msgsnd(qid, &sbuf2, buf_length, IPC_NOWAIT);
	//printf("\nSVR: msg_sent :length(%d)", buf_length);

	return status;
}

int rcv_msg2(int qid)
{
	int status;

	status = msgrcv(qid, &rbuf2, sizeof(rbuf2) - sizeof(long), 2, 0);
	//printf("\nCLI: msg received");

	return status;
}

int init_all()
{
	//initialize the message queues
	snd_msg_qid2 = msgget(SND_MSG_KEY2, IPC_CREAT | 0666 );
	if(snd_msg_qid2 < 0) {
        	perror("msgget");
        	return -1;
    	}

   	return 0;
}

int close_all()
{
	return 0;
}

void closeRemoteStream() {
	int buf_length;
	sbuf2.mtype = 1;
	//strcpy(sbuf.mtext, "END");
	//buf_length = strlen(sbuf.mtext) + 1 ;

	sbuf2.msg = AUD_STREAM_STOP2;
	msgsnd(snd_msg_qid, &sbuf2, sizeof(sbuf2) - sizeof(long), 0);

	//wait for ACK
	rcv_msg2(snd_msg_qid);
}

void writeAudioData(const unsigned char* data, unsigned dataSize)
{
        // Copy the audio frame into the buffer on the sahred memory
	//memcpy(sharedAudioData, data, dataSize);

        // Sample rate for the audio frame just received
	sbuf2.sampleRate = 8000;
	sbuf2.msg        = AUD_STREAM_NEWFRAME2;

	//send message that data written
	snd_msg2(snd_msg_qid2);

	//wait for ACK
	rcv_msg2(snd_msg_qid2);
}


int g_recvNum = 0;

void * PrintRecvSampleThread( void *args )
{

	pthread_detach( pthread_self() );	

	struct timeval t1, t2;
	
	gettimeofday(&t1, NULL);

	int difftime = 0;
	int startNum = g_recvNum;
	double tmp = 0;

	int timeInMs = 2 * 60 * 1000;

	while ( g_talkInUse) {

		gettimeofday(&t2, NULL);
		difftime =  (t2.tv_sec * 1000 + t2.tv_usec / 1000) - (t1.tv_sec * 1000 + t1.tv_usec / 1000);
		
		if ( difftime >= timeInMs  ) {
			
			gettimeofday(&t1, NULL);

			tmp = g_recvNum - startNum;
			startNum = g_recvNum;
			
			fprintf(stderr, "net recv total num %f  avg %f/s difftime %d \n", tmp, (float)(tmp/120), difftime);			

			
		} else {
			usleep(5000);
		}
		
	}


}


void *AUDIO_TalkThrFxn(void *pArgs)
{
	
	int maxFd = 0;
	int ret, rLen;
	fd_set 	fds;
	fd_set	wFds;	
	struct timeval  timeout;
	TalkHead_t  head;
	unsigned char audioCompressed[2000];

	//pthread_t countThr;
	//pthread_create(&countThr, NULL, PrintRecvSampleThread, NULL);
	printf("Enter talk thread\n");

	pthread_detach( pthread_self() );	

	do {					
		memset(&head, 0, sizeof(TalkHead_t));
		rLen = Net_TcpRecvN(g_talkClientFd, (char *)&head, sizeof(TalkHead_t));
		//printf("recv head %d\n", head.len);
		if (rLen != sizeof(head)) {
			rtsp_dbg(Err, DbgPerror, "talk recv head error \n");
			goto __TalkThrFxnExit;
		}

		if (head.len < 0 || head.len > 2000){
			rtsp_dbg(Err, DbgNoPerror, "talk head len = %d errorno = %d \n", 
			head.len, errno);
			goto __TalkThrFxnExit;
		}

		rLen = Net_TcpRecvN(g_talkClientFd, audioCompressed, head.len);
		//printf("recv data: %d\n", rLen);

		if (rLen != head.len) {
			rtsp_dbg(Err, DbgPerror, "talk recv audio data error %d, "
						"errno = %d\n", rLen, errno);
			goto __TalkThrFxnExit;
		}						

		// if (-2 == audioLib_Write( audioCompressed, head.len ) ) {
		// 	break;
		// }

	}while( 1 );

__TalkThrFxnExit:
	close(g_talkClientFd);
	g_talkInUse = 0;

	return NULL;
}

void *RTSP_SvrThrFxn(void *pArgs)
{
	int ret;
	int recvLen;
	int svrSockFd, cliSockFd;
	int maxFd = 0;
	int sockOptVal = 1;
	int handleStat = GS_SUCCESS;
	int uctrlConType, spareNull;
	fd_set sockFds;
	socklen_t addrLen;
	ConType_e conType;
	char 	recvBuf[MAX_RTSP_BUF_LEN];
	char    respBuf[MAX_RTSP_BUF_LEN];
	struct sockaddr_in cliAddr;
	struct linger  socklinger;
	struct timeval timeout;
	struct list_head *pos, *n;
	RtspSession_t *pSess = NULL;
	//UctrlSess_t *pSessUctrl = NULL;
	RtspSvr_t *pRtspSvr = (RtspSvr_t *)pArgs;

	if(pRtspSvr == NULL){
		//dbg(Err, DbgNoPerror, "pArgs == NULL \n");
		goto __RtspSvrThrFxnExit;
	}

	svrSockFd = pRtspSvr->rtspSvrSockFd;
	if(svrSockFd < 0){
		//dbg(Err, DbgNoPerror, "svrSockFd =%d \n", svrSockFd);
		goto __RtspSvrThrFxnExit;
	}

	//dbg(Dbg, DbgNoPerror, "Rendezvous_meet .......\n");
	#ifdef P_THR_MSG
	printf("[RTSP]: init success \n");
	#endif 
	//Rendezvous_meet(pRtspSvr->rvInit);
	//dbg(Dbg, DbgNoPerror, "Rendezvous_meet .......ok\n");
	while(1){
		//dbg(Dbg, DbgNoPerror, "rtsp run... \n");
		//Pause_test(pRtspSvr->hPause);
		RTSP_SessionStatCheck(pRtspSvr);
		FD_ZERO(&sockFds);
		FD_SET(svrSockFd, &sockFds);
		maxFd = svrSockFd;
		RTSP_SetAllSockFd(&sockFds, &maxFd, pRtspSvr);
		timeout.tv_sec  = 2;
		timeout.tv_usec = 0;	/* 2 s timeout */
		ret = select(maxFd + 1, &sockFds, NULL, NULL, &timeout);
		if(ret < 0){
			if(errno == EINTR){
				continue;
			}
			rtsp_dbg(Err, DbgPerror, "rtsp select erro \n");
			goto __RtspSvrThrFxnExit;
		}else if(ret == 0){
			//rtsp_dbg(Warn, DbgPerror, "rtsp select 2s timeout \n");
			continue;
		}else{
			if(FD_ISSET(svrSockFd, &sockFds)){
				addrLen = sizeof(cliAddr);
				cliSockFd = accept(svrSockFd, (struct sockaddr *)&cliAddr,&addrLen);
				if(cliSockFd <= 0){
					rtsp_dbg(Err, DbgPerror, "rtsp server accept error,sock = %d\n",cliSockFd);					
					continue;
				}
				//dbg(Dbg, DbgNoPerror, "accept a client\n");
				socklinger.l_onoff	= 1;
				socklinger.l_linger = 0;
				timeout.tv_sec  = 3; // 3
				timeout.tv_usec = 0;
				if( -1 == setsockopt(cliSockFd, SOL_SOCKET, SO_RCVTIMEO,(char*)&timeout, sizeof(timeout))){
					rtsp_dbg(Err, DbgPerror, "setsockopt cliSockFd = %d error\n",cliSockFd);
				}
            		if( -1 == setsockopt(cliSockFd, SOL_SOCKET, SO_LINGER,&socklinger, sizeof(socklinger))){
				//	rtsp_dbg(Err, DbgPerror, "setsockopt cliSockFd = %d error\n",cliSockFd);
				}
				if( -1 == setsockopt(cliSockFd ,IPPROTO_TCP, TCP_NODELAY,&sockOptVal, sizeof(int))){
					rtsp_dbg(Err, DbgPerror, "setsockopt cliSockFd = %d error\n",cliSockFd);
				}


				#if 0
				int nSendBufLen = 65535; //
				if( -1 == setsockopt(cliSockFd, SOL_SOCKET, SO_SNDBUF,&nSendBufLen, sizeof(nSendBufLen))){
					rtsp_dbg(Err, DbgPerror, "setsockopt cliSockFd = %d error\n", cliSockFd);
				}

				#endif

				
				memset(recvBuf, 0, MAX_RTSP_BUF_LEN);
				recvLen = recv(cliSockFd, recvBuf, MAX_RTSP_BUF_LEN, 0);

				fprintf(stderr, "\n=============================== %s %s %d  len %d recvBuf %s \n",
					__FILE__, __FUNCTION__, __LINE__, recvLen, recvBuf);
				
				if(recvLen <= 0){
					rtsp_dbg(Err, DbgPerror, "rtsp recv error\n");
					sock_close(cliSockFd);
					continue;
				}
			#if 0
				if(WORK_STAT_RUNNING != CFG_GetVencWorkStat() ||
								ID_STOP == CFG_GetRtspConnectStat()){
					dbg(Warn, DbgNoPerror, "Please reconnect after 3 seconds ... \n");
					sock_close(cliSockFd);
					cliSockFd = 0;
					continue;
				}
			#endif
			
				conType = RTSP_GetConnectType(recvBuf);
				if(conType == CON_TYPE_RTSP){
					//dbg(Dbg, DbgNoPerror, "a new rtsp client\n");
					pSess = RTSP_SessionCreate(pRtspSvr, cliSockFd);
					if(pSess == NULL){
						if(cliSockFd > 2){
							sock_close(cliSockFd);
							cliSockFd = -1;
						}
						continue;
					}
					memcpy(pSess->recvBuf, recvBuf, MAX_RTSP_BUF_LEN);
					pSess->inSize = recvLen;
					//RTSP一套
					ret = RTSP_SessionProcess(pSess);
					if(ret != GS_SUCCESS){
						//close(pSess->rtspSockFd);
						//pSess->rtspSockFd = -1;
						printf("\n##################### close\n");
						
						RTSP_SessionClose(pSess);
					}
				}else if(conType == CON_TYPE_UCTRL){
					if(strstr(recvBuf, "TALK") != NULL){
						printf("Talk now\n");
						if(g_talkInUse==1){
							sprintf((char *)respBuf, "CMD:TALK MCTP/1.0 SC\n@%d@%d@%d@\n", 0, 0, 0);	
							send(cliSockFd, (char const*) respBuf, strlen(respBuf), 0);	
							if(cliSockFd > 0){
								sock_close(cliSockFd);
								cliSockFd = -1;
							}
						}
						else{
							sprintf(respBuf, "CMD:TALK MCTP/1.0 SC\n@%d@%d@1@\n", 0, 1);   // first para @0 single, @1 dual
							send(cliSockFd, (char const*)respBuf, strlen(respBuf), 0);
							g_talkClientFd = cliSockFd;
							printf("Create talk thread\n");
							if(0 != pthread_create(&g_talkThrId, NULL, AUDIO_TalkThrFxn, NULL)){
								printf("Create failed\n");
								if(cliSockFd > 0){
									sock_close(cliSockFd);
									cliSockFd = -1;
								}
							}else{
								g_talkInUse = 1;
								pthread_detach(g_talkThrId);
							}
						}
					}
				}else{
					rtsp_dbg(Err, DbgNoPerror, "unknown connect type \n");
					if(cliSockFd > 0){
						sock_close(cliSockFd);
						cliSockFd = -1;
					}
				}
			}

			__RtspEventProcess:

			list_for_each_safe(pos, n, &(pRtspSvr->rtspSessWorkList.rtspSessList)){
				pSess = list_entry(pos, RtspSession_t, rtspSessList);
				if(pSess != NULL){
					cliSockFd = pSess->rtspSockFd;
					if(pSess->rtspSockFd <= 0){
						rtsp_dbg(Err, DbgPerror, "pSess->rtspSockFd = %d \n",pSess->rtspSockFd);
						continue;
					}
					if(svrSockFd != cliSockFd && FD_ISSET(cliSockFd, &sockFds)){
						handleStat = GS_FAIL;
						memset(pSess->recvBuf, 0, MAX_RTSP_BUF_LEN);
						recvLen = recv(pSess->rtspSockFd, pSess->recvBuf,MAX_RTSP_BUF_LEN, 0);

					//	fprintf(stderr, "\n=============================== %s %s %d  len %d pSess->recvBuf %s \n",
					//		__FILE__, __FUNCTION__, __LINE__, recvLen, pSess->recvBuf);
						
						if(recvLen <= 0){
							rtsp_dbg(Err, DbgPerror, "recv from [%s] error\n",pSess->remoteIp);
							goto __RtspSessprocessErr;
						}
						pSess->inSize = recvLen;
						//RTSP一套
						handleStat = RTSP_SessionProcess(pSess);

						__RtspSessprocessErr:
						if(handleStat != GS_SUCCESS){
							rtsp_dbg(Warn, DbgNoPerror, "close ch %d client ip : [%s] \n",pSess->channel, pSess->remoteIp);
							if(pSess->rtspSockFd > 0){
								FD_CLR(pSess->rtspSockFd, &sockFds);
								sock_close(pSess->rtspSockFd);
							}
							RTSP_SessionClose(pSess);
							RTSP_SessListPrint(pRtspSvr);
						}
					}
				}
			}
		}
	}

__RtspSvrThrFxnExit:
	__printf("[rtsp] server exit ... \n");
	//dbg(Dbg, DbgNoPerror, "__RtspSvrThrFxnExit !!!\n");
	/* close all tcp thread first */
	//dbg(Dbg, DbgNoPerror, "close all session !!!\n");
	RTSP_CloseAllClient();
	//dbg(Dbg, DbgNoPerror, "Rendezvous_force ......\n");
	//RTSP_FreeUdpSender(pRtspSvr, MAX_VOD_CHN, RTP_STREAM_MAX);
	//Rendezvous_force(pRtspSvr->rvInit);
	//dbg(Dbg, DbgNoPerror, "Rendezvous_meet rvExit ......\n");
	//CFG_PrintCnt();
	//Rendezvous_meet(pRtspSvr->rvExit);
	//dbg(Dbg, DbgNoPerror, "Rendezvous_meet exit ......ok\n");
	RTSP_FreeUdpSender(pRtspSvr, MAX_VOD_CHN, RTP_STREAM_MAX);
	RTSP_Media_Paras_Exit(0);
	RTSP_Media_Paras_Exit(1);
	RTSP_Media_Paras_Exit(2);
	//dbg(Dbg, DbgNoPerror, "exit ok  !!!\n");
	__printf("[rtsp] server exit ok \n");
	return NULL;
}


int RTSP_SvrThrStart(pthread_t *pid,RtspSvr_t *pRtspSvr)
{
	if(GS_SUCCESS != RTSP_SvrListen(pRtspSvr)){
		//dbg(Err, DbgNoPerror, "RTSP_SvrListen error \n");
		return GS_FAIL;
	}

	if(0 != pthread_create(pid, NULL, RTSP_SvrThrFxn, pRtspSvr)){
		//dbg(Err, DbgNoPerror, "pthread_create error \n");
		return GS_FAIL;
	}
	return GS_SUCCESS;
}

int	RTSP_ThrStart(pthread_t *pid, void *rbHandle)
{
	int webPort = 0;
	int	rtspPort = 0;
	RtspSvr_t *pRtspSvr = NULL;

	/* DB_GetRtspCfg() -> rtspPort */
	webPort = 80;//CFG_GetWebPort();
	if(webPort == 80){
		rtspPort = DEFAULT_RTSP_PORT;
	}else{
		rtspPort = webPort + RTSP_PORT_OFFSET;
	}

	rtspPort = g_rtspSvrPort;

	pRtspSvr = &gRtspVodSvr[VOD_SVR_TYPE_RTSP];

	rtsp_dbg(Info, DbgNoPerror, "RTSP_SvrCreate \n");
	if(GS_SUCCESS != RTSP_SvrCreate(rtspPort, MAX_VOD_CHN,VOD_SVR_TYPE_RTSP, pRtspSvr)){
		rtsp_dbg(Err, DbgNoPerror, "RTSP_SvrCreate error \n");
		return GS_FAIL;
	}
	pRtspSvr->ringbufHandle = rbHandle;
	rtsp_dbg(Info, DbgNoPerror, "RTSP_SvrThrStart \n");
	if(GS_SUCCESS != RTSP_SvrThrStart(pid, pRtspSvr)){
		rtsp_dbg(Err, DbgNoPerror, "RTSP_SvrThrStart error \n");
		return GS_FAIL;
	}
	return GS_SUCCESS;
}

int RTSP_ThrStop(void)
{
	//ThrCtl_t *pThr = CFG_GetThrCtlPtr();
	//return pthread_join(pThr->rtspThrId, NULL);

	close(gRtspVodSvr[VOD_SVR_TYPE_RTSP].rtspSvrSockFd);
	gRtspVodSvr[VOD_SVR_TYPE_RTSP].rtspSvrSockFd = -1;
	
	
	return GS_SUCCESS;
}



#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */




