
#include <unistd.h>
#include "libavutil/audio_fifo.h"
#include "libswresample/swresample.h"
#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>
#include "media_mux.h"
#include <pthread.h>
#include "audio_transcode.h"




MEDEA_MUX_HANDLE hHandle;
AVFormatContext *v_ifmt_ctx = NULL, *a_ifmt_ctx = NULL, *ofmt_ctx = NULL;
static pthread_t vtid;
static pthread_t atid;

void *video_frames_write_thd(void *arg)
{
    AVPacket pkt;
	MEDIA_MUX_FRAME_T stPacket;
    int unFrameCount=0;
    int ret;

	usleep(50000);
	pthread_detach(pthread_self());
	while (1) 
	{
		ret = av_read_frame(v_ifmt_ctx, &pkt);
		if (ret < 0)
			break;
		stPacket.eStreamType = (pkt.flags & AV_PKT_FLAG_KEY) ? MEDEA_MUX_STREAM_TYPE_VIDEO_I : MEDEA_MUX_STREAM_TYPE_VIDEO;
		stPacket.pData = (char *)pkt.data;
		stPacket.nLen = pkt.size;
		stPacket.ullFrameIndex = unFrameCount++;
		MediaMux_WriteFrame(hHandle,  &stPacket);
		usleep(10000);
		//printf("%s:%d MediaMux_WriteFrame = %d\n", __FUNCTION__, __LINE__, unFrameCount);

	}

	return NULL;
}

void *audio_frames_write_thd(void *arg)
{
	ATC_HANDLE hAHandle;
	ATC_INFO_T ATInfo;
    AVPacket pkt;
	MEDIA_MUX_FRAME_T stPacket;
    int unFrameCount=0;
    int ret;

	pthread_detach(pthread_self());
	ATInfo.eSrcAudioType = ATC_CODEC_G711U;
	ATInfo.eDstAudioType = ATC_CODEC_G711U;//ATC_CODEC_AAC;
	ATInfo.nABitrate = 128000;
	ATInfo.nASamplerate = 8000;
	ATInfo.nAChannelNum = 2;
	if (ATInfo.eSrcAudioType == ATInfo.eDstAudioType) {
		while (1) 
		{
			ret = av_read_frame(a_ifmt_ctx, &pkt);
			if (ret < 0 || ret == 1) 
				break;
			stPacket.eStreamType = MEDEA_MUX_STREAM_TYPE_AUDIO;
			stPacket.pData = pkt.data;
			stPacket.nLen = pkt.size;
			printf("av_read_frame: stPacket.nLen = %d\n", stPacket.nLen);
			stPacket.ullFrameIndex = unFrameCount++ * (1024);		// frame_size units increase
			MediaMux_WriteFrame(hHandle,  &stPacket);
		}
	} else {
		hAHandle = ATC_Init(&ATInfo);
		if (hAHandle == NULL) {
			printf("%s:%d ATC_Init error\n", __FUNCTION__, __LINE__);
		}

		while (1) 
		{
			ret = av_read_frame(a_ifmt_ctx, &pkt);
			if (ret < 0 || ret == 1) 
				break;

			ret = ATC_DecodeFrame(hAHandle, pkt.data, pkt.size);
			if (ret < 0) {
				printf("%s:%d ATC_DecodeFrame error\n", __FUNCTION__, __LINE__);
				continue;
			}

		encode_continue:
			ret = ATC_EncodeFrame(hAHandle, (unsigned char **)&stPacket.pData, &stPacket.nLen);
			if (ret < 0) {
				printf("%s:%d ATC_EncodeFrame error\n", __FUNCTION__, __LINE__);
				continue;
			} 
			if (ret == 1) {
				printf("%s:%d encode try again\n", __FUNCTION__, __LINE__);
				goto encode_continue;
			}
			
			stPacket.eStreamType = MEDEA_MUX_STREAM_TYPE_AUDIO;
			//stPacket.pData = pkt.data;
			//stPacket.nLen = pkt.size;
			printf("av_read_frame: stPacket.nLen = %d\n", stPacket.nLen);
			stPacket.ullFrameIndex = unFrameCount++ * (1024);		// frame_size units increase
			MediaMux_WriteFrame(hHandle,  &stPacket);
			if (ret == 3) {
				printf("%s:%d audio encode need continue\n", __FUNCTION__, __LINE__);
				goto encode_continue;
			}
		}
		ATC_Uninit(hAHandle);
	}
	
	return NULL;
}


int main(int argc, char **argv)
{
    char *v_filename, *a_filename,*out_filename;
    int ret;

	//MEDEA_MUX_HANDLE hHandle;
	MEDIA_MUX_STREAM_INFO_T stStreamInfo;

    if (argc < 4) {
        printf("usage: %s outfile videofile audiofile\n", argv[0]);
        return -1;
    }

    out_filename  = argv[1];
    v_filename = argv[2];
    a_filename = argv[3];

    av_register_all();

    if ((ret = avformat_open_input(&v_ifmt_ctx, v_filename, 0, 0)) < 0) {
        fprintf(stderr, "Could not open input file '%s'", v_filename);
        return -1;
    }
    if ((ret = avformat_find_stream_info(v_ifmt_ctx, 0)) < 0) {
        fprintf(stderr, "Failed to retrieve input stream information");
        return -1;
    }
    av_dump_format(v_ifmt_ctx, 0, v_filename, 0);

    if ((ret = avformat_open_input(&a_ifmt_ctx, a_filename, 0, 0)) < 0) {
        fprintf(stderr, "Could not open input file '%s', buf = %s\n", a_filename, av_err2str(ret));
        return -1;
    }
    if ((ret = avformat_find_stream_info(a_ifmt_ctx, 0)) < 0) {
        fprintf(stderr, "Failed to retrieve input stream information");
        return -1;
    }
    av_dump_format(a_ifmt_ctx, 0, a_filename, 0);


	stStreamInfo.nHaveVideo = 1;
	stStreamInfo.eVideoCodecType = v_ifmt_ctx->streams[0]->codecpar->codec_id == AV_CODEC_ID_HEVC ? MEDIA_MUX_CODEC_H265 : MEDIA_MUX_CODEC_H264;
	stStreamInfo.nVBitrate = v_ifmt_ctx->streams[0]->codecpar->bit_rate;
	AVRational gr = av_guess_frame_rate(v_ifmt_ctx, v_ifmt_ctx->streams[0], NULL); // ctx->time_base与ctx->framerate转换用av_inv_q(ctx->framerate)
	stStreamInfo.nVFramerate = (int)av_q2d(gr) * 1000; //gr.num/gr.den;	// 25
	stStreamInfo.nVGop = stStreamInfo.nVFramerate;
	stStreamInfo.nVWidth = v_ifmt_ctx->streams[0]->codecpar->width;
	stStreamInfo.nVHeight = v_ifmt_ctx->streams[0]->codecpar->height;

	stStreamInfo.nHaveAudio = 1;
	stStreamInfo.eAudioCodecType = MEDIA_MUX_CODEC_G711U;//MEDIA_MUX_CODEC_AAC;	//MEDIA_MUX_CODEC_G711U
	stStreamInfo.nASamplerate = 8000;
	stStreamInfo.nABitrate = 128000;
	stStreamInfo.nAChannelNum = 2;
	
	int i;
	printf("SPS:PPS:\n");
	for (i = 0; i < v_ifmt_ctx->streams[0]->codecpar->extradata_size; i++)
	{
		printf("%x ",v_ifmt_ctx->streams[0]->codecpar->extradata[i]);
	}
	printf("\n");
	
	hHandle = MediaMux_Open(out_filename, &stStreamInfo);

	pthread_create(&vtid, NULL, video_frames_write_thd, NULL);
	if (stStreamInfo.nHaveAudio != 0) {
		pthread_create(&atid, NULL, audio_frames_write_thd, NULL);
	}

	sleep(3);
	MediaMux_Close(hHandle);

    return 0;
}

