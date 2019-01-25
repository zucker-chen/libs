
/**
 * @file
 * libavformat/libavcodec demuxing and muxing API example.
 *
 * Remux streams from one container format to another.
 * @example audio_transcode.c
 */

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
//#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include "libavutil/audio_fifo.h"
#include "audio_transcode.h"






static enum AVCodecID _ATC_CodecIDGet(ATC_CODEC_TYPE_E eATC_Type)
{
	enum AVCodecID eATC_CodecID = AV_CODEC_ID_NONE;

	switch(eATC_Type)
	{
		case ATC_CODEC_G711A:
			{
				eATC_CodecID = AV_CODEC_ID_PCM_ALAW;
				break;
			}
		case ATC_CODEC_G711U:
			{
				eATC_CodecID = AV_CODEC_ID_PCM_MULAW;
				break;
			}
		case ATC_CODEC_AAC:
			{
				eATC_CodecID = AV_CODEC_ID_AAC;
				break;
			}
		default:
			{
				printf("error:unknow codectype:%d\n", eATC_Type);
				eATC_CodecID = AV_CODEC_ID_NONE;
				break;
			}
	}

	return eATC_CodecID;
}





/**
 * Initialize the audio resampler based on the input and output codec settings.
 * If the input and output sample formats differ, a conversion is required
 * libswresample takes care of this, but requires initialization.
 */
static int init_resampler(AVCodecContext *input_codec_context,
                          AVCodecContext *output_codec_context,
                          SwrContext **resample_context)
{
    int error;

    /**
     * Create a resampler context for the conversion.
     * Set the conversion parameters.
     * Default channel layouts based on the number of channels
     * are assumed for simplicity (they are sometimes not detected
     * properly by the demuxer and/or decoder).
     */
    *resample_context = swr_alloc_set_opts(NULL,
                                          av_get_default_channel_layout(output_codec_context->channels),
                                          output_codec_context->sample_fmt,
                                          output_codec_context->sample_rate,
                                          av_get_default_channel_layout(input_codec_context->channels),
                                          input_codec_context->sample_fmt,
                                          input_codec_context->sample_rate,
                                          0, NULL);
    if (!*resample_context) {
        fprintf(stderr, "Could not allocate resample context\n");
        return AVERROR(ENOMEM);
    }
    /**
    * Perform a sanity check so that the number of converted samples is
    * not greater than the number of samples to be converted.
    * If the sample rates differ, this case has to be handled differently
    */
    //av_assert0(output_codec_context->sample_rate == input_codec_context->sample_rate);

    /** Open the resampler with the specified parameters. */
    if ((error = swr_init(*resample_context)) < 0) {
        fprintf(stderr, "Could not open resample context\n");
        swr_free(resample_context);
        return error;
    }
	
    return 0;
}

/** Initialize a FIFO buffer for the audio samples to be encoded. */
static int init_fifo(AVAudioFifo **fifo, AVCodecContext *output_codec_context)
{
    /** Create the FIFO buffer based on the specified output sample format. */
    if (!(*fifo = av_audio_fifo_alloc(output_codec_context->sample_fmt,
                                      output_codec_context->channels, 1))) {
        fprintf(stderr, "Could not allocate FIFO\n");
        return AVERROR(ENOMEM);
    }
    return 0;
}




/**
 * Initialize a temporary storage for the specified number of audio samples.
 * The conversion requires temporary storage due to the different format.
 * The number of audio samples to be allocated is specified in frame_size.
 */
static int init_converted_samples(uint8_t ***converted_input_samples, AVCodecContext *output_codec_context, int frame_size)
{
    int error;

    /**
     * Allocate as many pointers as there are audio channels.
     * Each pointer will later point to the audio samples of the corresponding
     * channels (although it may be NULL for interleaved formats).
     */
    if (!(*converted_input_samples = calloc(output_codec_context->channels,
                                            sizeof(**converted_input_samples)))) {
        fprintf(stderr, "Could not allocate converted input sample pointers\n");
        return AVERROR(ENOMEM);
    }

    /**
     * Allocate memory for the samples of all channels in one consecutive
     * block for convenience.
     */
    if ((error = av_samples_alloc(*converted_input_samples, NULL, output_codec_context->channels, frame_size, output_codec_context->sample_fmt, 0)) < 0) {
        fprintf(stderr, "Could not allocate converted input samples (error '%s')\n", av_err2str(error));
        av_freep(&(*converted_input_samples)[0]);
        free(*converted_input_samples);
        return error;
    }
    return 0;
}

/**
 * Convert the input audio samples into the output sample format.
 * The conversion happens on a per-frame basis, the size of which is specified
 * by frame_size.
 */
static int convert_samples(const uint8_t **input_data, uint8_t **converted_data, const int frame_size, SwrContext *resample_context)
{
    int error;

    /** Convert the samples using the resampler. */
    if ((error = swr_convert(resample_context, converted_data, frame_size, input_data , frame_size)) < 0) {
        fprintf(stderr, "Could not convert input samples (error '%s')\n", av_err2str(error));
        return error;
    }

    return 0;
}

/** Add converted input audio samples to the FIFO buffer for later processing. */
static int add_samples_to_fifo(AVAudioFifo *fifo,
                               uint8_t **converted_input_samples,
                               const int frame_size)
{
    int error;

    /**
     * Make the FIFO as large as it needs to be to hold both,
     * the old and the new samples.
     */
    if ((error = av_audio_fifo_realloc(fifo, av_audio_fifo_size(fifo) + frame_size)) < 0) {
        fprintf(stderr, "Could not reallocate FIFO\n");
        return error;
    }

    /** Store the new samples in the FIFO buffer. */
    if (av_audio_fifo_write(fifo, (void **)converted_input_samples, frame_size) < frame_size) {
        fprintf(stderr, "Could not write data to FIFO\n");
        return AVERROR_EXIT;
    }
    return 0;
}






ATC_HANDLE ATC_Init(ATC_INFO_T *pATInfo)
{
	ATC_CONTEXT_T *pATCtx;
	int nRet;

	pATCtx = (ATC_CONTEXT_T*)malloc(sizeof(ATC_CONTEXT_T));

	// input
	pATCtx->pSrcCodec = avcodec_find_decoder(_ATC_CodecIDGet(pATInfo->eSrcAudioType));//AV_CODEC_ID_PCM_MULAW
	if (pATCtx->pSrcCodec == NULL){
		printf("%s:%d avcodec_find_decoder error\n", __FUNCTION__, __LINE__);
		return NULL;
	}
	pATCtx->pSrcCodecCtx = avcodec_alloc_context3(pATCtx->pSrcCodec);
	if (pATCtx->pSrcCodecCtx == NULL){
		printf("%s:%d avcodec_alloc_context3 error\n", __FUNCTION__, __LINE__);
		return NULL;
	}
	pATCtx->pSrcCodecCtx->channels 	  		= pATInfo->nAChannelNum;
	pATCtx->pSrcCodecCtx->channel_layout 	= av_get_default_channel_layout(pATInfo->nAChannelNum);
	pATCtx->pSrcCodecCtx->sample_rate	  	= pATInfo->nASamplerate;
	pATCtx->pSrcCodecCtx->sample_fmt	  	= pATCtx->pSrcCodec->sample_fmts[0];
	pATCtx->pSrcCodecCtx->bit_rate 	  		= pATInfo->nABitrate;
	nRet = avcodec_open2(pATCtx->pSrcCodecCtx, pATCtx->pSrcCodec, NULL);
	if (nRet < 0){
		printf("%s:%d avcodec_alloc_context3 error: %s\n", __FUNCTION__, __LINE__, av_err2str(nRet));
		return NULL;
	}
	
	// output
	pATCtx->pDstCodec = avcodec_find_encoder(_ATC_CodecIDGet(pATInfo->eDstAudioType));//AV_CODEC_ID_PCM_MULAW
	if (pATCtx->pDstCodec == NULL){
		printf("%s:%d avcodec_find_decoder error\n", __FUNCTION__, __LINE__);
		return NULL;
	}
	pATCtx->pDstCodecCtx = avcodec_alloc_context3(pATCtx->pDstCodec);
	if (pATCtx->pDstCodecCtx == NULL){
		printf("%s:%d avcodec_alloc_context3 error\n", __FUNCTION__, __LINE__);
		return NULL;
	}
	pATCtx->pDstCodecCtx->channels 	  		= pATInfo->nAChannelNum;
	pATCtx->pDstCodecCtx->channel_layout 	= av_get_default_channel_layout(pATInfo->nAChannelNum);
	pATCtx->pDstCodecCtx->sample_rate	  	= pATInfo->nASamplerate;
	pATCtx->pDstCodecCtx->sample_fmt	  	= pATCtx->pDstCodec->sample_fmts[0];
	pATCtx->pDstCodecCtx->bit_rate 	  		= pATInfo->nABitrate;
	nRet = avcodec_open2(pATCtx->pDstCodecCtx, pATCtx->pDstCodec, NULL);
	if (nRet < 0){
		printf("%s:%d avcodec_open2 error: %s\n", __FUNCTION__, __LINE__, av_err2str(nRet));
		return NULL;
	}


    nRet = init_fifo(&pATCtx->pFIFO, pATCtx->pDstCodecCtx);
	if (nRet < 0){
		printf("%s:%d init_fifo error\n", __FUNCTION__, __LINE__);
		return NULL;
	}

    nRet = init_resampler(pATCtx->pSrcCodecCtx, pATCtx->pDstCodecCtx, &pATCtx->pSwrCtx);
	if (nRet < 0){
		printf("%s:%d init_resampler error\n", __FUNCTION__, __LINE__);
		return NULL;
	}

	return pATCtx;

}


int ATC_DecodeFrame(ATC_HANDLE hHandle, uint8_t *pData, int nSize)
{
	ATC_CONTEXT_T *pATCtx = (ATC_CONTEXT_T *)hHandle;
    AVPacket pkt;
	AVFrame *pFrame = av_frame_alloc();
	uint8_t **pSamples_Data = NULL;
	int nRet;

	av_init_packet(&pkt);
	pkt.data = pData;
	pkt.size = nSize;
	nRet = avcodec_send_packet(pATCtx->pSrcCodecCtx, &pkt);
	if (nRet == AVERROR(EAGAIN)) {
		printf("%s:%d avcodec_send_packet error\n", __FUNCTION__, __LINE__);
	/* If the last frame has been encoded, stop encoding. */
	} else if (nRet == AVERROR_EOF) {
		printf("%s:%d avcodec_send_packet error\n", __FUNCTION__, __LINE__);
	} else if (nRet < 0) {
		printf("%s:%d avcodec_send_packet error: %s\n", __FUNCTION__, __LINE__, av_err2str(nRet));
	}
	
	nRet = avcodec_receive_frame(pATCtx->pSrcCodecCtx, pFrame);
	if (nRet == AVERROR(EAGAIN)) {
		printf("%s:%d avcodec_receive_frame error\n", __FUNCTION__, __LINE__);
	/* If the last frame has been encoded, stop encoding. */
	} else if (nRet == AVERROR_EOF) {
		printf("%s:%d avcodec_receive_frame error\n", __FUNCTION__, __LINE__);
	} else if (nRet < 0) {
		printf("%s:%d avcodec_receive_frame error: %s\n", __FUNCTION__, __LINE__, av_err2str(nRet));
	}


	nRet = init_converted_samples(&pSamples_Data, pATCtx->pDstCodecCtx, pFrame->nb_samples);
	if (nRet < 0){
		printf("%s:%d init_converted_samples error\n", __FUNCTION__, __LINE__);
		return -1;
	}
	
	/**
	 * Convert the input samples to the desired output sample format.
	 * This requires a temporary storage provided by converted_input_samples.
	 */
	nRet = convert_samples((const uint8_t**)pFrame->extended_data, pSamples_Data, pFrame->nb_samples, pATCtx->pSwrCtx);
	if (nRet < 0){
		printf("%s:%d convert_samples error\n", __FUNCTION__, __LINE__);
		return -1;
	}
	
	/** Add the converted input samples to the FIFO buffer for later processing. */
	nRet = add_samples_to_fifo(pATCtx->pFIFO, pSamples_Data, pFrame->nb_samples);
	if (nRet < 0){
		printf("%s:%d add_samples_to_fifo error\n", __FUNCTION__, __LINE__);
		return -1;
	}

	return 0;
}



int ATC_EncodeFrame(ATC_HANDLE hHandle, uint8_t **pData, int *pSize)
{
	ATC_CONTEXT_T *pATCtx = (ATC_CONTEXT_T *)hHandle;
    AVPacket pkt;
	AVFrame *pFrame = NULL;
	uint8_t **pSamples_Data = NULL;
	int nFrameSize = 0;
	int nRet;


	pFrame =  av_frame_alloc();
	nFrameSize = FFMIN(av_audio_fifo_size(pATCtx->pFIFO), pATCtx->pDstCodecCtx->frame_size);
	pFrame->nb_samples	 = nFrameSize;
	pFrame->channel_layout = pATCtx->pDstCodecCtx->channel_layout;
	pFrame->format		 = pATCtx->pDstCodecCtx->sample_fmt;
	pFrame->sample_rate	 = pATCtx->pDstCodecCtx->sample_rate;
	//printf("av_audio_fifo_size(pATCtx->pFIFO) = %d, pATCtx->pDstCodecCtx->frame_size = %d\n", av_audio_fifo_size(pATCtx->pFIFO), pATCtx->pDstCodecCtx->frame_size);
	//fprintf(stderr, "ATC_EncodeFrame: %d, frame_size = %d, channel_layout = %d, format = %d, sample_rate = %d\n", __LINE__, nFrameSize, pFrame->channel_layout, pATCtx->pDstCodecCtx->sample_fmt, pFrame->sample_rate);
	nRet = av_frame_get_buffer(pFrame, 0);
	if (nRet < 0){
		printf("%s:%d av_frame_get_buffer error: %s\n", __FUNCTION__, __LINE__, av_err2str(nRet));
		av_frame_free(pFrame);
		return -1;
	}
	
	nRet = av_audio_fifo_read(pATCtx->pFIFO, (void **)pFrame->data, nFrameSize);
	if (nRet < nFrameSize) {
		printf("%s:%d convert_samples error\n", __FUNCTION__, __LINE__);
		av_frame_free(&pFrame);
		return -1;
	}



	nRet = avcodec_send_frame(pATCtx->pDstCodecCtx, pFrame);
	if (nRet == AVERROR(EAGAIN)) {
		printf("%s:%d avcodec_send_frame error\n", __FUNCTION__, __LINE__);
		return 1;
	/* If the last frame has been encoded, stop encoding. */
	} else if (nRet == AVERROR_EOF) {
		printf("%s:%d avcodec_send_frame error\n", __FUNCTION__, __LINE__);
		return 2;
	} else if (nRet < 0) {
		printf("%s:%d avcodec_send_frame error: %s\n", __FUNCTION__, __LINE__, av_err2str(nRet));
		return -1;
	}
	
	av_init_packet(&pkt);
	pkt.data = NULL;
	pkt.size = 0;
	nRet = avcodec_receive_packet(pATCtx->pDstCodecCtx, &pkt);
	if (nRet == AVERROR(EAGAIN)) {
		printf("%s:%d avcodec_receive_packet error\n", __FUNCTION__, __LINE__);
		return 1;
	/* If the last frame has been encoded, stop encoding. */
	} else if (nRet == AVERROR_EOF) {
		printf("%s:%d avcodec_send_frame error\n", __FUNCTION__, __LINE__);
		return 2;
	} else if (nRet < 0) {
		printf("%s:%d avcodec_send_frame error: %s\n", __FUNCTION__, __LINE__, av_err2str(nRet));
		return -1;
	}

	*pData = pkt.data;
	*pSize = pkt.size;

	return 0;
}






