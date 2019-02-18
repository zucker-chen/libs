#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "media_demux.h"

#include "libavutil/channel_layout.h"
#include "libavutil/mathematics.h"
#include "libavutil/opt.h"
#include "libavformat/avformat.h"



typedef struct _MEDIA_DEMUX_CONTEXT_T
{
	AVFormatContext *pFmtCtx;
} MEDIA_DEMUX_CONTEXT_T;






MEDIA_DEMUX_HANDLE MediaDemux_Open(char *pFileName, MEDIA_DEMUX_STREAM_INFO_T *pStreamInfo)
{
	MEDIA_DEMUX_CONTEXT_T *pContext = NULL;
	AVCodecParameters *pCodecpar = NULL;
	enum AVPixelFormat pix_fmt;
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


    avcodec_register_all();
	av_register_all();

	av_log_set_level(AV_LOG_TRACE);
	printf("%s:%d \n", __FUNCTION__, __LINE__);


    AVDictionary *format_opts = NULL;
	//format_opts = av_mallocz(8);
	av_dict_set_int(&format_opts, "sample_rate", 8000, 0);
	
	av_dict_set_int(&format_opts, "channels", 2, 0);
	
	av_dict_set(&format_opts, "scan_all_pmts", "1", AV_DICT_DONT_OVERWRITE);

	AVInputFormat * ifmt = av_find_input_format("mp4");
	#if 0
	
    if (!av_dict_get(format_opts, "scan_all_pmts", NULL, AV_DICT_MATCH_CASE)) {
        av_dict_set(&format_opts, "scan_all_pmts", "1", AV_DICT_DONT_OVERWRITE);
        //scan_all_pmts_set = 1;
    }
	#endif


	pContext->pFmtCtx = avformat_alloc_context();

    pContext->pFmtCtx->flags |= AVFMT_FLAG_KEEP_SIDE_DATA;

    pContext->pFmtCtx->flags |= AVFMT_FLAG_NONBLOCK;
	pContext->pFmtCtx->video_codec_id = AV_CODEC_ID_H264;
	pContext->pFmtCtx->audio_codec_id = AV_CODEC_ID_AAC;
	
	av_format_set_video_codec(pContext->pFmtCtx, avcodec_find_decoder(AV_CODEC_ID_H264));
	av_format_set_audio_codec(pContext->pFmtCtx, avcodec_find_decoder(AV_CODEC_ID_AAC));

    /* open input file, and allocate format context */
    //if ((nRet = avformat_open_input(&pContext->pFmtCtx , pFileName, NULL, NULL)) < 0) {
	if ((nRet = avformat_open_input(&pContext->pFmtCtx , pFileName, ifmt, &format_opts)) < 0) {
        fprintf(stderr, "Could not open source file %s :%s\n", pFileName, av_err2str(nRet));
        return NULL;
    }
	printf("%s:%d \n", __FUNCTION__, __LINE__);
	printf("%s:%d fmtctx->nb_streams = %d\n", __FUNCTION__, __LINE__, pContext->pFmtCtx->nb_streams);

    /* retrieve stream information */
    if (avformat_find_stream_info(pContext->pFmtCtx, NULL) < 0) {
        fprintf(stderr, "Could not find stream information\n");
        return NULL;
    }
	printf("%s:%d \n", __FUNCTION__, __LINE__);

	printf("%s:%d nb_streams = %d\n", __FUNCTION__, __LINE__, pContext->pFmtCtx->nb_streams);
    for (i = 0; i < pContext->pFmtCtx->nb_streams; i++) {
		printf("%s:%d \n", __FUNCTION__, __LINE__);
		pCodecpar = pContext->pFmtCtx->streams[i]->codecpar;
		if (pCodecpar->codec_type != AVMEDIA_TYPE_AUDIO &&
			pCodecpar->codec_type != AVMEDIA_TYPE_VIDEO &&
			pCodecpar->codec_type != AVMEDIA_TYPE_SUBTITLE) {
			continue;
		}


	}




    /* dump input information to stderr */
    av_dump_format(pContext->pFmtCtx, 0, pFileName, 0);




	return (MEDIA_DEMUX_HANDLE)pContext;
}


int MediaMux_ReadFrame(MEDIA_DEMUX_HANDLE hHandle,  MEDIA_DEMUX_FRAME_T *pFrame)
{


	return 0;
}

int MediaMux_Close(MEDIA_DEMUX_HANDLE hHandle)
{

	return 0;
}


