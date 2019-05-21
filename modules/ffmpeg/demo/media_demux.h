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
	long long llPts;	// pts for timebase
	long long llMsPts;	// pts for ms
} MEDIA_DEMUX_FRAME_T;

typedef void* MEDIA_DEMUX_HANDLE;


/**
 * 解封装输入文件打开，获取输入文件信息
 * input: 	pFileName, 输入文件名
 * output: 	pStreamInfo, 分析后的音视频流信息
 * result: 	!NULL = success, NULL = fail
 */
MEDIA_DEMUX_HANDLE MediaDemux_Open(char *pFileName, MEDIA_DEMUX_STREAM_INFO_T *pStreamInfo);

/**
 * 解封装句柄关闭，资源回收
 * input: 	hHandle, 句柄
 * output: 	无
 * result: 	0 = success, <0 = fail
 */
int MediaDemux_Close(MEDIA_DEMUX_HANDLE hHandle);

/**
 * 解封装时间跳转
 * input: 	hHandle, 句柄; nTimeMs, 跳转的时间,单位ms,相对于文件开始时间的偏移
 * output: 	无
 * result: 	0 = success, <0 = fail
 */
int MediaDemux_SeekTime(MEDIA_DEMUX_HANDLE hHandle,  int nTimeMs);

/**
 * 解封装时长设置，设置解封装数据的持续时间
 * input: 	hHandle, 句柄; nTimeMs, 解封装时长,单位ms
 * output: 	无
 * result: 	0 = success, <0 = fail
 */
int MediaDemux_SetDuration(MEDIA_DEMUX_HANDLE hHandle,  int nTimeMs);

/**
 * 获取解封装视频数据帧率
 * input: 	hHandle, 句柄
 * result: 	fps
 */
int MediaDemux_GetFrameRate(MEDIA_DEMUX_HANDLE hHandle);

/**
 * 解封装帧数据读取
 * input: 	hHandle, 句柄
 * output: 	pFrame, 读出的帧数据信息
 * result: 	0 = success, <0 = fail
 */
int MediaDemux_ReadFrame(MEDIA_DEMUX_HANDLE hHandle,  MEDIA_DEMUX_FRAME_T *pFrame);


#endif
