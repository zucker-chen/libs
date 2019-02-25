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
} MEDIA_DEMUX_CONTEXT_T;






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


int MediaDemux_SeekTime(MEDIA_DEMUX_HANDLE hHandle,  int nTimeMs)
{
	MEDIA_DEMUX_CONTEXT_T *pMDCtx = NULL;
	AVFormatContext *pFmtCtx = NULL;
	int nRet;

	pMDCtx = (MEDIA_DEMUX_CONTEXT_T *)hHandle;
	pFmtCtx = pMDCtx->pFmtCtx;

	nRet = av_seek_frame(pFmtCtx, -1, ((double)nTimeMs/(double)1000)*AV_TIME_BASE + (double)pFmtCtx->start_time, AVSEEK_FLAG_BACKWARD);
	if (nRet < 0) {
		printf("%s:%d av_seek_frame error: %s\n", __FUNCTION__, __LINE__, av_err2str(nRet));
		return -1;
	}

	return 0;
}

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

	eCodecType = pMDCtx->pFmtCtx->streams[pkt.stream_index]->codecpar->codec_type;
	//printf("%s:%d pkt.stream_index(%d), eCodecType(%d) pkt.pts = %lu!\n", __FUNCTION__, __LINE__, pkt.stream_index, eCodecType, pkt.pts);
	if (eCodecType == AVMEDIA_TYPE_VIDEO) {
		pFrame->eStreamType = (pkt.flags & AV_PKT_FLAG_KEY) != 0 ? MEDIA_DEMUX_STREAM_TYPE_VIDEO_I : MEDIA_DEMUX_STREAM_TYPE_VIDEO;
		memcpy(pFrame->pData, pkt.data, pkt.size);
		pFrame->nLen = pkt.size;
		pFrame->llPts = pkt.pts;
	} else if (eCodecType == AVMEDIA_TYPE_AUDIO) {
		pFrame->eStreamType = MEDIA_DEMUX_STREAM_TYPE_AUDIO;
		memcpy(pFrame->pData, pkt.data, pkt.size);
		pFrame->nLen = pkt.size;
		pFrame->llPts = pkt.pts;
	} else {
		printf("%s:%d eCodecType(%d) error !\n", __FUNCTION__, __LINE__, eCodecType);
		return -1;
	}
	av_packet_unref(&pkt);
	
	return 0;
}

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


