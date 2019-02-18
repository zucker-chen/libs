#ifndef __MEDIA_MUX_H
#define __MEDIA_MUX_H

typedef enum _MEDEA_MUX_STREAM_TYPE_E
{
	MEDEA_MUX_STREAM_TYPE_NONE = 0,
	MEDEA_MUX_STREAM_TYPE_VIDEO,
	MEDEA_MUX_STREAM_TYPE_VIDEO_I,
	MEDEA_MUX_STREAM_TYPE_AUDIO,
} MEDEA_MUX_STREAM_TYPE_E;

typedef enum _MEDIA_MUX_CODEC_E
{
	MEDIA_MUX_CODEC_NONE = 0,
	MEDIA_MUX_CODEC_MPEG4,
	MEDIA_MUX_CODEC_H264,
	MEDIA_MUX_CODEC_H265,
	MEDIA_MUX_CODEC_G711A = 100,
	MEDIA_MUX_CODEC_G711U,
	MEDIA_MUX_CODEC_AAC,
} MEDIA_MUX_CODEC_E;

typedef struct _MEDIA_MUX_STREAM_INFO_T
{
	int nHaveVideo;
	MEDIA_MUX_CODEC_E eVideoCodecType;
	int nVFramerate; // 25000 for 25fps
	int nVBitrate;
	int nVGop;
	int nVWidth;
	int nVHeight;
	int nHaveAudio;
	MEDIA_MUX_CODEC_E eAudioCodecType;
	int nASamplerate;
	int nABitrate;
	int nAChannelNum;
} MEDIA_MUX_STREAM_INFO_T;

typedef struct _MEDIA_MUX_FRAME_T
{
	MEDEA_MUX_STREAM_TYPE_E eStreamType;
	char *pData;
	int nLen;
	unsigned long long ullFrameIndex;
	unsigned long ulTsSec;
	unsigned long ulTsUSec;
} MEDIA_MUX_FRAME_T;

typedef void* MEDEA_MUX_HANDLE;

MEDEA_MUX_HANDLE MediaMux_Open(char *pFileName, MEDIA_MUX_STREAM_INFO_T *pStreamInfo);
int MediaMux_Close(MEDEA_MUX_HANDLE hHandle);
int MediaMux_WriteFrame(MEDEA_MUX_HANDLE hHandle,  MEDIA_MUX_FRAME_T *pFrame);
#endif