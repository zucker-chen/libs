#ifndef __AUDIO_TRANSCODE_H
#define __AUDIO_TRANSCODE_H

typedef enum _ATC_CODEC_TYPE_E
{
	ATC_NONE = 0,
	ATC_CODEC_G711A,
	ATC_CODEC_G711U,
	ATC_CODEC_AAC,
} ATC_CODEC_TYPE_E;


typedef struct _ATC_INFO_T
{
	ATC_CODEC_TYPE_E eSrcAudioType;
	ATC_CODEC_TYPE_E eDstAudioType;
	int nASamplerate;
	int nABitrate;
	int nAChannelNum;
} ATC_INFO_T;


typedef struct _ATC_CONTEXT_T
{
	AVCodec *pSrcCodec;
	AVCodecContext *pSrcCodecCtx;
	AVCodec *pDstCodec;
	AVCodecContext *pDstCodecCtx;
	AVAudioFifo *pFIFO;
	SwrContext *pSwrCtx;
} ATC_CONTEXT_T;


typedef ATC_CONTEXT_T * ATC_HANDLE;



ATC_HANDLE ATC_Init(ATC_INFO_T *pATInfo);
int ATC_Uninit(ATC_HANDLE hHandle);
int ATC_DecodeFrame(ATC_HANDLE hHandle, uint8_t *pData, int nSize);
int ATC_EncodeFrame(ATC_HANDLE hHandle, uint8_t **pData, int *pSize);


#endif
