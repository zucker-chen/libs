#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "media_demux.h"
#include "libavutil/channel_layout.h"
#include "libavutil/mathematics.h"
#include "libavutil/opt.h"
#include "libavutil/avutil.h"
#include "libavformat/avformat.h"



typedef struct _MEDIA_DEMUX_CONTEXT_T
{
	AVFormatContext *pFmtCtx;
	long long llSeekPts;		// 解封装seek跳转时间, time_base为单位
	long long llDurationPts;	// 解封装时长, time_base为单位
} MEDIA_DEMUX_CONTEXT_T;


/**
 * 解封装输入文件打开，获取输入文件信息
 * input: 	pFileName, 输入文件名
 * output: 	pStreamInfo, 分析后的音视频流信息
 * result: 	!NULL = success, NULL = fail
 */
MEDIA_DEMUX_HANDLE MediaDemux_Open(char *pFileName, MEDIA_DEMUX_STREAM_INFO_T *pStreamInfo)
{
	MEDIA_DEMUX_CONTEXT_T *pContext = NULL;
	AVCodecParameters *pCodecpar = NULL;
	int nRet, i = 0;

	if(pFileName == NULL)
	{
		printf("error: file name is null\n");
		return NULL;
	}
	if(pStreamInfo == NULL)
	{
		printf("error:streaminfo is null\n");
		return NULL;
	}

	pContext = (MEDIA_DEMUX_CONTEXT_T*)malloc(sizeof(MEDIA_DEMUX_CONTEXT_T));
	if(pContext == NULL)
	{
		printf("error:can not alloc memory for media mux context\n");
		return NULL;
	}	
	memset(pContext, 0, sizeof(MEDIA_DEMUX_CONTEXT_T));

	av_register_all();
	//av_log_set_level(AV_LOG_TRACE);

    /* open input file, and allocate format context */
    if ((nRet = avformat_open_input(&pContext->pFmtCtx , pFileName, NULL, NULL)) < 0) {
        fprintf(stderr, "Could not open source file %s :%s\n", pFileName, av_err2str(nRet));
        return NULL;
    }
    /* retrieve stream information */
    if (avformat_find_stream_info(pContext->pFmtCtx, NULL) < 0) {
        fprintf(stderr, "Could not find stream information\n");
        return NULL;
    }

	printf("%s:%d nb_streams = %d\n", __FUNCTION__, __LINE__, pContext->pFmtCtx->nb_streams);
    for (i = 0; i < pContext->pFmtCtx->nb_streams; i++) {
		pCodecpar = pContext->pFmtCtx->streams[i]->codecpar;

		if (pCodecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
			pStreamInfo->nHaveVideo = 1;
			pStreamInfo->eVideoCodecType = pCodecpar->codec_id != AV_CODEC_ID_HEVC ? MEDIA_DEMUX_CODEC_H264 : MEDIA_DEMUX_CODEC_H265;
			pStreamInfo->nVBitrate = pCodecpar->bit_rate;
			pStreamInfo->nVHeight = pCodecpar->height;
			pStreamInfo->nVWidth = pCodecpar->width;
			
			pContext->pFmtCtx->duration = 10/av_q2d(pContext->pFmtCtx->streams[i]->time_base);
		} else if (pCodecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
			pStreamInfo->nHaveAudio = 1;
			pStreamInfo->eAudioCodecType = pCodecpar->codec_id != AV_CODEC_ID_AAC ? MEDIA_DEMUX_CODEC_G711U : MEDIA_DEMUX_CODEC_AAC;
			pStreamInfo->nAChannelNum = pCodecpar->channels;
			pStreamInfo->nABitrate = pCodecpar->bit_rate;
			pStreamInfo->nASamplerate = pCodecpar->sample_rate;
		}
	}
	//printf("%s:%d \n", __FUNCTION__, __LINE__);

    /* dump input information to stderr */
    av_dump_format(pContext->pFmtCtx, 0, pFileName, 0);

	return (MEDIA_DEMUX_HANDLE)pContext;
}

/**
 * 解封装时间跳转
 * input: 	hHandle, 句柄; nTimeMs, 跳转的时间,单位ms,相对于文件开始时间的偏移
 * output: 	无
 * result: 	0 = success, <0 = fail
 */
int MediaDemux_SeekTime(MEDIA_DEMUX_HANDLE hHandle,  int nTimeMs)
{
	MEDIA_DEMUX_CONTEXT_T *pMDCtx = NULL;
	AVFormatContext *pFmtCtx = NULL;
	int nRet;

	pMDCtx = (MEDIA_DEMUX_CONTEXT_T *)hHandle;
	pFmtCtx = pMDCtx->pFmtCtx;
	if(pMDCtx == NULL || pFmtCtx == NULL)
	{
		printf("%s:%d pMDCtx = NULL error !\n", __FUNCTION__, __LINE__);
		return -1;
	}

	nRet = av_seek_frame(pFmtCtx, -1, ((double)nTimeMs/(double)1000)*AV_TIME_BASE + (double)pFmtCtx->start_time, AVSEEK_FLAG_BACKWARD);
	if (nRet < 0) {
		printf("%s:%d av_seek_frame error: %s\n", __FUNCTION__, __LINE__, av_err2str(nRet));
		return -1;
	}
	pMDCtx->llSeekPts = (nTimeMs/1000)/av_q2d(pMDCtx->pFmtCtx->streams[0]->time_base);

	return 0;
}

/**
 * 解封装时长设置，设置解封装数据的持续时间
 * input: 	hHandle, 句柄; nTimeMs, 解封装时长,单位ms
 * output: 	无
 * result: 	0 = success, <0 = fail
 */
int MediaDemux_SetDuration(MEDIA_DEMUX_HANDLE hHandle,  int nTimeMs)
{
	MEDIA_DEMUX_CONTEXT_T *pMDCtx = NULL;

	pMDCtx = (MEDIA_DEMUX_CONTEXT_T *)hHandle;
	if(pMDCtx == NULL)
	{
		printf("%s:%d pMDCtx = NULL error !\n", __FUNCTION__, __LINE__);
		return -1;
	}

	pMDCtx->llDurationPts = (nTimeMs/1000)/av_q2d(pMDCtx->pFmtCtx->streams[0]->time_base);

	return 0;
}

/**
 * 获取解封装视频数据帧率
 * input: 	hHandle, 句柄
 * result: 	fps
 */
int MediaDemux_GetFrameRate(MEDIA_DEMUX_HANDLE hHandle)
{
	int nFps = -1, i;
	MEDIA_DEMUX_CONTEXT_T *pMDCtx = NULL;

	pMDCtx = (MEDIA_DEMUX_CONTEXT_T *)hHandle;
	if(pMDCtx == NULL) {
		printf("%s:%d pMDCtx = NULL error !\n", __FUNCTION__, __LINE__);
		return -1;
	}

    for (i = 0; i < pMDCtx->pFmtCtx->nb_streams; i++) {
		if (pMDCtx->pFmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
			break;
		}
	}
	if (pMDCtx->pFmtCtx->nb_streams == i) {
		printf("%s:%d Can not find video codec type !\n", __FUNCTION__, __LINE__);
		return nFps;
	}

	AVRational gr = av_guess_frame_rate(pMDCtx->pFmtCtx, pMDCtx->pFmtCtx->streams[i], NULL);
	nFps = (int)av_q2d(gr);
	printf("%s:%d video(%d) fps = %d !\n", __FUNCTION__, __LINE__, i, nFps);

	return nFps;
}

/**
 * 解封装帧数据读取
 * input: 	hHandle, 句柄
 * output: 	pFrame, 读出的帧数据信息
 * result: 	0 = success, <0 = fail
 */
int MediaDemux_ReadFrame(MEDIA_DEMUX_HANDLE hHandle,  MEDIA_DEMUX_FRAME_T *pFrame)
{
    AVPacket pkt;
	MEDIA_DEMUX_CONTEXT_T *pMDCtx = NULL;
	enum AVMediaType eCodecType = AVMEDIA_TYPE_UNKNOWN;
	int nRet;

	if(hHandle == NULL)
	{
		printf("%s:%d hHandle = NULL error !\n", __FUNCTION__, __LINE__);
		return -1;
	}
	av_init_packet(&pkt);
	pMDCtx = (MEDIA_DEMUX_CONTEXT_T *)hHandle;
	nRet = av_read_frame(pMDCtx->pFmtCtx, &pkt);
	if (nRet == AVERROR_EOF) {
		printf("%s:%d av_read_frame End of file !\n", __FUNCTION__, __LINE__);
		return -1;
	} else if (nRet < 0) {
		printf("%s:%d av_read_frame error: %s\n", __FUNCTION__, __LINE__, av_err2str(nRet));
		return -1;
	}
	if (pMDCtx->llDurationPts != 0 && pkt.dts > (pMDCtx->pFmtCtx->start_time + pMDCtx->llSeekPts + pMDCtx->llDurationPts)) {
		printf("%s:%d End of file (End early)!\n", __FUNCTION__, __LINE__);
		return -1;
	}

	eCodecType = pMDCtx->pFmtCtx->streams[pkt.stream_index]->codecpar->codec_type;
	pFrame->llMsPts = pkt.dts * av_q2d(pMDCtx->pFmtCtx->streams[pkt.stream_index]->time_base) * 1000;	// ms
	//printf("%s:%d pkt.stream_index(%d), eCodecType(%d) pkt.dts = %lld, pFrame->llMsPts = %lld!\n", __FUNCTION__, __LINE__, pkt.stream_index, eCodecType, pkt.dts, pFrame->llMsPts);
	if (eCodecType == AVMEDIA_TYPE_VIDEO) {
		pFrame->eStreamType = (pkt.flags & AV_PKT_FLAG_KEY) != 0 ? MEDIA_DEMUX_STREAM_TYPE_VIDEO_I : MEDIA_DEMUX_STREAM_TYPE_VIDEO;
		memcpy(pFrame->pData, pkt.data, pkt.size);
		pFrame->nLen = pkt.size;
		pFrame->llPts = pkt.dts;	// 解封装用pkt.dts，因为pkt.pts AVI时解析不正确
	} else if (eCodecType == AVMEDIA_TYPE_AUDIO) {
		pFrame->eStreamType = MEDIA_DEMUX_STREAM_TYPE_AUDIO;
		memcpy(pFrame->pData, pkt.data, pkt.size);
		pFrame->nLen = pkt.size;
		pFrame->llPts = pkt.dts;	// 解封装用pkt.dts，因为pkt.pts AVI时解析不正确
	} else {
		printf("%s:%d eCodecType(%d) error !\n", __FUNCTION__, __LINE__, eCodecType);
		return -1;
	}
	av_packet_unref(&pkt);
	
	return 0;
}

/**
 * 解封装句柄关闭，资源回收
 * input: 	hHandle, 句柄
 * output: 	无
 * result: 	0 = success, <0 = fail
 */
int MediaDemux_Close(MEDIA_DEMUX_HANDLE hHandle)
{
	MEDIA_DEMUX_CONTEXT_T *pMDCtx = (MEDIA_DEMUX_CONTEXT_T*)hHandle;

	if(pMDCtx == NULL)
	{
		printf("%s:%d pMDCtx = NULL error !\n", __FUNCTION__, __LINE__);
		return -1;
	}

	if(pMDCtx->pFmtCtx) {
		avformat_free_context(pMDCtx->pFmtCtx);
		pMDCtx->pFmtCtx = NULL;
	}
	free(pMDCtx);
	pMDCtx = NULL;

	return 0;
}


