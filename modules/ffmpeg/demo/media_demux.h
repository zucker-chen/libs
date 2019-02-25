#ifndef __MEDIA_DEMUX_H
#define __MEDIA_DEMUX_H

typedef enum _MEDIA_DEMUX_STREAM_TYPE_E
{
	MEDIA_DEMUX_STREAM_TYPE_NONE = 0,
	MEDIA_DEMUX_STREAM_TYPE_VIDEO,
	MEDIA_DEMUX_STREAM_TYPE_VIDEO_I,
	MEDIA_DEMUX_STREAM_TYPE_AUDIO,
} MEDIA_DEMUX_STREAM_TYPE_E;

typedef enum _MEDIA_DEMUX_CODEC_E
{
	MEDIA_DEMUX_CODEC_NONE = 0,
	MEDIA_DEMUX_CODEC_MPEG4,
	MEDIA_DEMUX_CODEC_H264,
	MEDIA_DEMUX_CODEC_H265,
	MEDIA_DEMUX_CODEC_G711A = 100,
	MEDIA_DEMUX_CODEC_G711U,
	MEDIA_DEMUX_CODEC_AAC,
} MEDIA_DEMUX_CODEC_E;

typedef struct _MEDIA_DEMUX_STREAM_INFO_T
{
	int nHaveVideo;
	MEDIA_DEMUX_CODEC_E eVideoCodecType;
	int nVFramerate; // 25000 for 25fps
	int nVBitrate;
	int nVGop;
	int nVWidth;
	int nVHeight;
	int nHaveAudio;
	MEDIA_DEMUX_CODEC_E eAudioCodecType;
	int nASamplerate;
	int nABitrate;
	int nAChannelNum;
} MEDIA_DEMUX_STREAM_INFO_T;

typedef struct _MEDIA_DEMUX_FRAME_T
{
	MEDIA_DEMUX_STREAM_TYPE_E eStreamType;
	unsigned char *pData;
	int nLen;
	long long ullPts;
} MEDIA_DEMUX_FRAME_T;

typedef void* MEDIA_DEMUX_HANDLE;


MEDIA_DEMUX_HANDLE MediaDemux_Open(char *pFileName, MEDIA_DEMUX_STREAM_INFO_T *pStreamInfo);

int MediaDemux_Close(MEDIA_DEMUX_HANDLE hHandle);

int MediaDemux_SeekTime(MEDIA_DEMUX_HANDLE hHandle,  int nTimeMs);

int MediaDemux_ReadFrame(MEDIA_DEMUX_HANDLE hHandle,  MEDIA_DEMUX_FRAME_T *pFrame);


#endif
