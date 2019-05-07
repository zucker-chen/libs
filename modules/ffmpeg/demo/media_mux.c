#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "media_mux.h"

#include "libavutil/channel_layout.h"
#include "libavutil/mathematics.h"
#include "libavutil/opt.h"
#include "libavformat/avformat.h"


typedef struct _MEDEA_MUX_CONTEXT_T
{
	char abyFileName[64];
	AVFormatContext *pFmtCtx;
	AVStream *pVideoStream;
	AVStream *pAudioStream;
	AVRational VCodecTimeBase;
	AVRational ACodecTimeBase;
} MEDEA_MUX_CONTEXT_T;

/** 视频extradata说明
 * 	第2~4byte 为SPS的2~4byte数据
 *	第8byte 为SPS数据大小
 *	0x67开始是SPS数据
 *	0x68前一个byte是PPS数据大小，前两个byte 0x01 0x00 固定
 * 	0x68开始是PPS数据
 */
static const unsigned char h264_extradata[] = {
	0x01, 0x4d, 0x40, 0x33, 0xff, 0xe1, 0x00, 0x16, 0x67, 0x4d, 0x40, 0x33, 0x92, 0x54, 0xc, 0x4, 0xb4, 0x20, 0x0, 0x0, 0x3, 0x0, 0x40, 0x0, 0x0, 0xc, 0xd1, 0xe3, 0x6, 0x54, 0x01, 0x00, 0x04, 0x68, 0xee, 0x3c, 0x80
};
static const  unsigned char aac_extradata[] = {
	0x12, 0x10
};

static enum AVCodecID _MediaMux_CodecIDToLibav(MEDIA_MUX_CODEC_E eCodecType)
{
	enum AVCodecID eLibavCodecID = AV_CODEC_ID_NONE;

	switch(eCodecType)
	{
		case MEDIA_MUX_CODEC_MPEG4:
			{
				eLibavCodecID = AV_CODEC_ID_MPEG4;
				break;
			}
		case MEDIA_MUX_CODEC_H264:
			{
				eLibavCodecID = AV_CODEC_ID_H264;
				break;
			}
		case MEDIA_MUX_CODEC_H265:
			{
				eLibavCodecID = AV_CODEC_ID_HEVC;
				break;
			}
		case MEDIA_MUX_CODEC_G711A:
			{
				eLibavCodecID = AV_CODEC_ID_PCM_ALAW;
				break;
			}
		case MEDIA_MUX_CODEC_G711U:
			{
				eLibavCodecID = AV_CODEC_ID_PCM_MULAW;
				break;
			}
		case MEDIA_MUX_CODEC_AAC:
			{
				eLibavCodecID = AV_CODEC_ID_AAC;
				break;
			}
		default:
			{
				printf("error:unknow codectype:%d\n", eCodecType);
				eLibavCodecID = AV_CODEC_ID_NONE;
				break;
			}
	}

	return eLibavCodecID;
}


static int _MediaMux_ADTS_FrqIndexGet(unsigned int uSampleFrq)
{
	switch (uSampleFrq) 
	{
		case 96000: return 0;
		case 88200: return 1;
		case 64000: return 2;
		case 48000: return 3;
		case 44100: return 4;
		case 32000: return 5;
		case 24000: return 6;
		case 22050: return 7;
		case 16000: return 8;
		case 12000: return 9;
		case 11025: return 10;
		case 8000:  return 11;
		case 7350:  return 12;
		default:    return 0;
	}

	return -1;
}


static void _MediaMux_MakeDSI( unsigned int uSampleFrq, unsigned int uChConfig, unsigned char* pDsi)
{
	unsigned int uObjectType = 2; // AAC LC by default
	pDsi[0] = (uObjectType << 3) | (uSampleFrq >> 1);
	pDsi[1] = ((uSampleFrq & 1) << 7) | (uChConfig << 3);
}


MEDEA_MUX_HANDLE MediaMux_Open(char *pFileName, MEDIA_MUX_STREAM_INFO_T *pStreamInfo)
{
	AVOutputFormat *ofmt;
	MEDEA_MUX_CONTEXT_T *pContext = NULL;
	AVFormatContext *fmtctx;
	enum AVCodecID eCodecID;
	int ret = 0;
	char averror[128];

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

	if(pStreamInfo->nHaveAudio == 0 && pStreamInfo->nHaveVideo == 0)
	{
		printf("warn:nHaveAudio and nHaveVideo = 0\n");
		return NULL;
	}

	pContext = (MEDEA_MUX_CONTEXT_T*)malloc(sizeof(MEDEA_MUX_CONTEXT_T));
	if(pContext == NULL)
	{
		printf("error:can not alloc memory for media mux context\n");
		return NULL;
	}	
	memset(pContext, 0, sizeof(MEDEA_MUX_CONTEXT_T));

	av_register_all();
	av_log_set_level(AV_LOG_TRACE);

	ofmt = av_guess_format(NULL, pFileName, NULL);
	if(!ofmt)
	{
		printf("error: Could not deduce output format from file extension\n");
		goto _error;
	}

	fmtctx = avformat_alloc_context();
	if(!fmtctx)
	{
		printf("error: can not alloc context\n");
		goto _error;
	}
	fmtctx->oformat = ofmt;
	snprintf(fmtctx->filename, sizeof(fmtctx->filename), "%s", pFileName);

	if(pStreamInfo->nHaveVideo)
	{
		eCodecID = _MediaMux_CodecIDToLibav(pStreamInfo->eVideoCodecType);
		if(eCodecID == AV_CODEC_ID_NONE)
		{
			printf("error: unknow codeid\n");
			goto _error;
		}

		fmtctx->oformat->video_codec = eCodecID;
		pContext->pVideoStream = avformat_new_stream(fmtctx, NULL);
		if(!pContext->pVideoStream)
		{
			printf("error: can not new stream for video\n");
			goto _error;
		}
		pContext->pVideoStream->time_base.num = 1;
		pContext->pVideoStream->time_base.den = 90000;//pStreamInfo->nVFramerate / 1000;	//  Avi is forced to modify 600
		pContext->pVideoStream->avg_frame_rate.num = 1;
		pContext->pVideoStream->avg_frame_rate.den = pStreamInfo->nVFramerate / 1000;;
		pContext->pVideoStream->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
		pContext->pVideoStream->codecpar->format = AV_PIX_FMT_YUV420P;
		pContext->pVideoStream->codecpar->codec_id = eCodecID;
		if (eCodecID == AV_CODEC_ID_HEVC) pContext->pVideoStream->codecpar->codec_tag = 0x31766568;	// Must be set for AVI，"H265"=0x35363248, "hev1"=0x31766568, "HEVC"=0x43564548
		if(!strcmp(fmtctx->oformat->name, "mp4" ) || !strcmp (fmtctx->oformat->name, "mov" ) || !strcmp (fmtctx->oformat->name, "3gp" )) 
		{
			pContext->pVideoStream->codecpar->codec_tag = 0;		// MP4 must be 0
		}
		pContext->pVideoStream->codecpar->width = pStreamInfo->nVWidth;
		pContext->pVideoStream->codecpar->height = pStreamInfo->nVHeight;
		//pContext->pVideoStream->duration = pStreamInfo->nVFramerate/90;//10*25;
		#if 0	// 需要完善：根据SPS/PPS数据自动封装extradata
		if(!strcmp(fmtctx->oformat->name, "mp4" ) || !strcmp (fmtctx->oformat->name, "mov" ) || !strcmp (fmtctx->oformat->name, "3gp" )) 
		{
			pContext->pVideoStream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER ;
		}
		pContext->pVideoStream->codecpar->extradata_size = sizeof(h264_extradata);
		pContext->pVideoStream->codecpar->extradata = av_mallocz(pContext->pVideoStream->codecpar->extradata_size + AV_INPUT_BUFFER_PADDING_SIZE);
		if (!pContext->pVideoStream->codecpar->extradata)
		{
			printf("error: can not alloc memory for extradata\n");
			goto _error;
		}
		memcpy(pContext->pVideoStream->codecpar->extradata, h264_extradata, sizeof(h264_extradata));	
		#endif	

		pContext->VCodecTimeBase.num = 1;
		pContext->VCodecTimeBase.den = pStreamInfo->nVFramerate / 1000;
	}
	
	if(pStreamInfo->nHaveAudio)
	{
		eCodecID = _MediaMux_CodecIDToLibav(pStreamInfo->eAudioCodecType);
		if(eCodecID == AV_CODEC_ID_NONE)
		{
			printf("error: unknow codeid\n");
			goto _error;
		}
		pContext->pAudioStream = avformat_new_stream(fmtctx, NULL);
		if(!pContext->pAudioStream)
		{
			printf("error: can not new stream for audio\n");
			goto _error;
		}
		pContext->pAudioStream->time_base.num = 1;
		pContext->pAudioStream->time_base.den = pStreamInfo->nASamplerate;
		pContext->pAudioStream->codecpar->codec_type = AVMEDIA_TYPE_AUDIO;
		pContext->pAudioStream->codecpar->codec_id = eCodecID;
		pContext->pAudioStream->codecpar->sample_rate = pStreamInfo->nASamplerate;
		pContext->pAudioStream->codecpar->channels = pStreamInfo->nAChannelNum;
		pContext->pAudioStream->time_base.num = 1;
		pContext->pAudioStream->time_base.den = pStreamInfo->nASamplerate;

		if (eCodecID == AV_CODEC_ID_AAC)
		{
			pContext->pAudioStream->codecpar->frame_size = 1024;
			#if 0
			if(!strcmp(fmtctx->oformat->name, "mp4" ) || !strcmp (fmtctx->oformat->name, "mov" ) || !strcmp (fmtctx->oformat->name, "3gp" )) 
			{
				pContext->pAudioStream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;	// warning: ‘codec’ is deprecated
				pContext->pAudioStream->codecpar->codec_tag = 0;		// MP4 must be 0
			}
			pContext->pAudioStream->codecpar->extradata_size = 2;
			unsigned char * pDsi = pContext->pAudioStream->codecpar->extradata = (uint8_t *)(char *)av_malloc(pContext->pAudioStream->codecpar->extradata_size + AV_INPUT_BUFFER_PADDING_SIZE);
			_MediaMux_MakeDSI(_MediaMux_ADTS_FrqIndexGet(pStreamInfo->nASamplerate), pStreamInfo->nAChannelNum, pDsi);
			printf("%s:%d pDsi[0] = 0x%x, pDsi[1] = 0x%x\n", __FUNCTION__, __LINE__, pDsi[0], pDsi[1]);
			#endif
		}

		pContext->ACodecTimeBase.num = 1;
		pContext->ACodecTimeBase.den = pStreamInfo->nASamplerate;

	}

	av_dump_format(fmtctx, 0, pFileName, 1);

	/* open the output file, if needed */
	if (!(fmtctx->oformat->flags & AVFMT_NOFILE)) 
	{
		printf("debug:avio_open()\n");
		ret = avio_open(&fmtctx->pb, pFileName, AVIO_FLAG_WRITE);
		if (ret < 0) 
		{
			printf("%s:%d avio_open error: %s\n", __FUNCTION__, __LINE__, av_err2str(ret));
			goto _error;
		}
	}

	/* Write the stream header, if any. */
	ret = avformat_write_header(fmtctx, NULL);	
	if(ret < 0)
	{
		printf("%s:%d avformat_write_header error: %s\n", __FUNCTION__, __LINE__, av_err2str(ret));
		goto _error;
	}

	pContext->pFmtCtx = fmtctx;

	printf("open media mux successfully!\n");

	strcpy(pContext->abyFileName, pFileName);

	return (MEDEA_MUX_HANDLE)pContext;

_error:
	if(pContext)
	{
		if(fmtctx->pb)
		{
			avio_close(fmtctx->pb);
		}
		if(pContext->pFmtCtx)
		{
			avformat_free_context(pContext->pFmtCtx);
		}
		free(pContext);
	}
	return NULL;
}


int MediaMux_WriteFrame(MEDEA_MUX_HANDLE hHandle,  MEDIA_MUX_FRAME_T *pFrame)
{
	MEDEA_MUX_CONTEXT_T *pContext = (MEDEA_MUX_CONTEXT_T*)hHandle;
	AVPacket pkt   = { 0 };
	int nRet = 0;

	if(hHandle == NULL || pFrame == NULL)
	{
		printf("error: invalid params\n");
		return -1;
	}

	av_init_packet(&pkt);

	if(pFrame->eStreamType == MEDEA_MUX_STREAM_TYPE_VIDEO
			|| pFrame->eStreamType == MEDEA_MUX_STREAM_TYPE_VIDEO_I)
	{
		if(pFrame->eStreamType == MEDEA_MUX_STREAM_TYPE_VIDEO_I)
		{
			pkt.flags |= AV_PKT_FLAG_KEY;
		}
		//int64_t calc_duration=(double)AV_TIME_BASE * av_q2d(pContext->pVideoStream->time_base);
		//pkt.pts = calc_duration * sunFrameCount;
		//av_packet_rescale_ts(&pkt, pContext->pVideoCodecCtx->time_base, pContext->pVideoStream->time_base);
		//pkt.pts = av_rescale_q(sunFrameCount, pContext->pVideoCodecCtx->time_base, pContext->pVideoStream->time_base);
		pkt.pts = av_rescale_q(pFrame->ullFrameIndex, pContext->VCodecTimeBase, pContext->pVideoStream->time_base);
		printf("frame count:%llu, pts:%lu\n", pFrame->ullFrameIndex, pkt.pts);
		printf("basetime.num = %d, .den = %d; basetime.num = %d, .den = %d\n", pContext->VCodecTimeBase.num, pContext->VCodecTimeBase.den, pContext->pVideoStream->time_base.num, pContext->pVideoStream->time_base.den);
		pkt.dts = pkt.pts;
		//pkt.pts = (90000/25) * sunFrameCount;
		//pkt.pts = pPacket->ulTsSec * 1000 + pPacket->ulTsUSec / 1000;
		//pkt.duration = 3600;
		pkt.stream_index = pContext->pVideoStream->index;
		pkt.duration = av_rescale_q(1, pContext->VCodecTimeBase, pContext->pVideoStream->time_base);
		pkt.data = (uint8_t *)pFrame->pData;
		pkt.size = pFrame->nLen;
		#if 0
		nRet = av_write_frame(pContext->pFmtCtx, &pkt);
		#else
		nRet = av_interleaved_write_frame(pContext->pFmtCtx, &pkt);
		#endif
		if(nRet != 0)
		{
			printf("%s:%d av_write_frame video error: %s\n", __FUNCTION__, __LINE__, av_err2str(nRet));
			return -1;
		}
	}
	else if(pFrame->eStreamType == MEDEA_MUX_STREAM_TYPE_AUDIO)
	{
		pkt.flags |= AV_PKT_FLAG_KEY;
		//pkt.pts = pPacket->ulTsSec * 1000 + pPacket->ulTsUSec / 1000;
		pkt.pts = av_rescale_q(pFrame->ullFrameIndex, pContext->ACodecTimeBase, pContext->pAudioStream->time_base);
		printf("audio pts:%lu, data size:%d\n", pkt.pts, pFrame->nLen);
		pkt.dts = pkt.pts;
		pkt.duration = av_rescale_q(1, pContext->ACodecTimeBase, pContext->pAudioStream->time_base);
		pkt.stream_index = pContext->pAudioStream->index;
		pkt.data = (uint8_t *)pFrame->pData;
		pkt.size = pFrame->nLen;
		#if 0
		nRet = av_write_frame(pContext->pFmtCtx, &pkt);
		#else
		nRet = av_interleaved_write_frame(pContext->pFmtCtx, &pkt);
		#endif
		if(nRet != 0)
		{
			printf("%s:%d av_write_frame audio error: %s\n", __FUNCTION__, __LINE__, av_err2str(nRet));
			return -1;
		}
	}
	else
	{
		printf("error: unknow stream type:%d\n", pFrame->eStreamType);
		return -1;
	}

	return 0;
}

int MediaMux_Close(MEDEA_MUX_HANDLE hHandle)
{
	MEDEA_MUX_CONTEXT_T *pContext = (MEDEA_MUX_CONTEXT_T*)hHandle;

	if(pContext == NULL)
	{
		printf("error: invalid param\n");
		return -1;
	}

	av_write_trailer(pContext->pFmtCtx);

	if (!(pContext->pFmtCtx->oformat->flags & AVFMT_NOFILE)) 
	{
		if (avio_close(pContext->pFmtCtx->pb) < 0) 
		{
			printf("error: Could not open '%s'\n", pContext->abyFileName);
		}
	}	

	// cleanup
	if(pContext)
	{
		if(pContext->pFmtCtx)
		{
			avformat_free_context(pContext->pFmtCtx);
		}
		free(pContext);
	}

	return 0;
}

#if 0
int main(void)
{
	MEDEA_MUX_HANDLE hHandle;
	MEDIA_MUX_STREAM_INFO_T stStreamInfo;
	MEDIA_MUX_AV_PACKET_T packet;

	hHandle = MediaMux_Open("1111.mp4", &stStreamInfo);
	MediaMux_WriteFrame(hHandle, &packet);
	MediaMux_Close(hHandle);

	return 0;
}
#endif

