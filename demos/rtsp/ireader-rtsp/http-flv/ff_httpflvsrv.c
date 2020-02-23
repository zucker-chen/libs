#include <unistd.h>
#include "libavutil/audio_fifo.h"
#include "libswresample/swresample.h"
#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>
#include "media_demux.h"
#include <pthread.h>
#include <time.h>
#include <signal.h>
#include <sys/time.h>

#include "ringbuf.h"
#include "httpflv_api.h"


#define MAX_ENCODE_STREAM_NUM    1

#define HTTPFLV_SERVER_DEBUG
#ifdef HTTPFLV_SERVER_DEBUG
#define pri_dbg(format, args...) fprintf(stderr,"%s %d %s() " format, __FILE__, __LINE__, __func__, ## args)
#else
#define pri_dbg(format, args...) do{}while(0)
#endif


static char *input_filename[2] = {"../../../../modules/ffmpeg/files/sample_cif.h264", "../../../../modules/ffmpeg/files/sample_cif.mp4"};
static char *url_prefix = "/live/";
static ringbuf_t *rb[2];
static MEDIA_DEMUX_HANDLE hHandle[2];
static MEDIA_DEMUX_STREAM_INFO_T stStreamInfo[2];
static int thd_running[2];


static inline uint64_t system_mstime(void)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (uint64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

static int aac_adts_frq_index(unsigned int uSamspleFrq)
{
	switch (uSamspleFrq) 
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

#define ADTS_HEADER_SIZE 7
//size = audio frame data size
static int aac_set_adts_head(MEDIA_DEMUX_STREAM_INFO_T *pStreamInfo, unsigned char *buf, int size)  
{         
      unsigned char byte;    
      if (size < ADTS_HEADER_SIZE) {
            return -1;  
      }       
      buf[0] = 0xff;  
      buf[1] = 0xf1;  
      byte = 0;  
      byte |= (2 & 0x03) << 6;  			// ADTSContext->objecttype profile
      byte |= ((aac_adts_frq_index(pStreamInfo->nASamplerate)) & 0x0f) << 2;  	// ADTSContext->sample_rate_index
      byte |= (pStreamInfo->nAChannelNum & 0x07) >> 2; 		// ADTSContext->channel_conf
      buf[2] = byte;  
      byte = 0;  
      byte |= (pStreamInfo->nAChannelNum & 0x07) << 6;  		// ADTSContext->channel_conf
      byte |= (ADTS_HEADER_SIZE + size) >> 11;  
      buf[3] = byte;  
      byte = 0;  
      byte |= (ADTS_HEADER_SIZE + size) >> 3;  
      buf[4] = byte;  
      byte = 0;  
      byte |= ((ADTS_HEADER_SIZE + size) & 0x7) << 5;  
      byte |= (0x7ff >> 6) & 0x1f;  
      buf[5] = byte;  
      byte = 0;  
      byte |= (0x7ff & 0x3f) << 2;  
      buf[6] = byte;     
	  
      return 0;  
}  

static void * get_stream_thdcb(void * arg)
{
	pthread_detach(pthread_self());

	MEDIA_DEMUX_FRAME_T stMDFrame;
	unsigned char frame_buf[1*1024*1024];	// 1MB
	httpflv_frame_info_t stPkg[2] = {0};			// 0=video, 1=audio
	char *p;
	long long last_pts[2] = {0};				// 0=video, 1=audio
	unsigned char adts_buf[7];					// aac
    int ret, nCh, size;
	
	nCh = arg != NULL ? *(int *)arg : 0;

	hHandle[nCh] = MediaDemux_Open(input_filename[nCh], &stStreamInfo[nCh]);
	printf("%s:%d videotype = %d, audiotype = %d\n", __FUNCTION__, __LINE__, stStreamInfo[nCh].eVideoCodecType, stStreamInfo[nCh].eAudioCodecType);
	//FILE *pVFile = fopen("v.h264", "wb");

	while (thd_running[nCh] == 1) {
		stMDFrame.pData = &frame_buf[0];
		ret = MediaDemux_ReadFrame(hHandle[nCh], &stMDFrame);
		if (ret < 0) {
			MediaDemux_Close(hHandle[nCh]);
			hHandle[nCh] = MediaDemux_Open(input_filename[nCh], &stStreamInfo[nCh]);
			stPkg[1].pts = stPkg[0].pts;					// 音频时间戳对齐视频时间戳
			continue;
			//stMDFrame.nLen = 1024;
			//MediaDemux_GetExtradata(hHandle[nCh], 0, stMDFrame.pData, &stMDFrame.nLen);
		}

		//printf("%s:%d stMDFrame.eStreamType = %d, stMDFrame.nLen = %d !\n", __FUNCTION__, __LINE__, stMDFrame.eStreamType, stMDFrame.nLen);
		if (stMDFrame.eStreamType == MEDIA_DEMUX_CODEC_H264 || stMDFrame.eStreamType == MEDIA_DEMUX_CODEC_H265) {
			stPkg[0].stream_type = stMDFrame.eStreamType == MEDIA_DEMUX_CODEC_H264 ? 0x7 : 0xc;	//RTP_PAYLOAD_H264;
			stPkg[0].data_len = stMDFrame.nLen;
			if (stMDFrame.llPts < 0) { 			// h264 raw file
				stPkg[0].pts = stPkg[0].pts + 40;
			} else {
				if (stMDFrame.llPts == 0) {		// pts倒序，重头开始了
					last_pts[0] = 0;
				}
				stPkg[0].pts = stPkg[0].pts + stMDFrame.llMsPts - last_pts[0];
				last_pts[0] = stMDFrame.llMsPts;
			}
			ringbuf_write_get_unit(rb[nCh], (unsigned char **)&p, sizeof(httpflv_frame_info_t) + stMDFrame.nLen);
			memcpy(p, &stPkg[0], sizeof(httpflv_frame_info_t));
			memcpy(p + sizeof(httpflv_frame_info_t), stMDFrame.pData, stMDFrame.nLen);
			ringbuf_write_put_unit(rb[nCh], sizeof(httpflv_frame_info_t) + stMDFrame.nLen);

			usleep(38000);
			//if (stMDFrame.eStreamType == MEDIA_DEMUX_STREAM_TYPE_VIDEO_I)
			//	printf("func = %s, line = %d: This is Key farme pts = %luu\n", __FUNCTION__, __LINE__, stPkg.pts);
			//fwrite(stMDFrame.pData, 1, stMDFrame.nLen, pVFile); 
			//printf("Video Data Head: %x %x %x %x %x\n", stMDFrame.pData[0], stMDFrame.pData[1], stMDFrame.pData[2], stMDFrame.pData[3], stMDFrame.pData[4]);
		} else if (stMDFrame.eStreamType == MEDIA_DEMUX_CODEC_G711A || stMDFrame.eStreamType == MEDIA_DEMUX_CODEC_G711U || stMDFrame.eStreamType == MEDIA_DEMUX_CODEC_AAC) {
			if (stMDFrame.eStreamType == MEDIA_DEMUX_CODEC_G711A) {
				stPkg[1].stream_type = 0x70;
			} else if (stMDFrame.eStreamType == MEDIA_DEMUX_CODEC_G711U) {
				stPkg[1].stream_type = 0x80;
			} else if (stMDFrame.eStreamType == MEDIA_DEMUX_CODEC_AAC) {
				stPkg[1].stream_type = 0xa0;
			}
			stPkg[1].data_len = stMDFrame.nLen;
			if (stMDFrame.llPts < 0) { 			// G711U raw file
				stPkg[1].pts = stPkg[1].pts + 98;
			} else {
				if (stMDFrame.llPts == 0) {		// pts倒序，重头开始了
					last_pts[1] = 0;
				}
				stPkg[1].pts = stPkg[1].pts + stMDFrame.llMsPts - last_pts[1];
				last_pts[1] = stMDFrame.llMsPts;
			}
			size = sizeof(httpflv_frame_info_t) + stMDFrame.nLen;
			if (stMDFrame.eStreamType == MEDIA_DEMUX_CODEC_AAC) size += ADTS_HEADER_SIZE;
			ringbuf_write_get_unit(rb[nCh], (unsigned char **)&p, size);
			memcpy(p, &stPkg[1], sizeof(httpflv_frame_info_t));
			if (stMDFrame.eStreamType == MEDIA_DEMUX_CODEC_AAC) {
				aac_set_adts_head(&stStreamInfo, adts_buf, stMDFrame.nLen);
				memcpy(p + sizeof(httpflv_frame_info_t), adts_buf, ADTS_HEADER_SIZE);
			}
			memcpy(p + sizeof(httpflv_frame_info_t) + ADTS_HEADER_SIZE, stMDFrame.pData, stMDFrame.nLen);
			ringbuf_write_put_unit(rb[nCh], sizeof(httpflv_frame_info_t) + stMDFrame.nLen);
		} else {
			printf("%s:%d stMDFrame.eStreamType = %d\n", __FUNCTION__, __LINE__, stMDFrame.eStreamType);
		}
		//nCh == 0 && printf("%s:%d eCodecType(%d) pFrame->llMsPts = %lld, data size = %d!\n", __FUNCTION__, __LINE__, stMDFrame.eStreamType, stMDFrame.llMsPts, stMDFrame.nLen);
	}

	MediaDemux_Close(hHandle[nCh]);

	return NULL;
}

/*
SPS:PPS:
0 0 0 1 67 4d 40 33 92 54 c 4 b4 20 0 0 3 0 40 0 0 c d1 e3 6 54 0 0 0 1 68 ee 3c 80 
*/
static int get_stream_info(char *channel_name, httpflv_stream_info_t *stream_info)
{
	if (channel_name == NULL || stream_info == NULL) {
		return -1;
	}

	int ch = 0, size = 0, ret = -1;
	char *p;

	if (0 != strncmp(channel_name, url_prefix, strlen(url_prefix))) {
		printf("func = %s, line = %d:  \n", __FUNCTION__, __LINE__);
		return -1;
	}
	p = &channel_name[0] + strlen(url_prefix);
	sscanf(p, "%2d", &ch);
	printf("func = %s, line = %d:  ch = %d\n", __FUNCTION__, __LINE__, ch);
	ch = ch == 0 ? 0 : 1;
	
	stream_info->is_have_video = stStreamInfo[ch].nHaveVideo;
	stream_info->video_payload = stStreamInfo[ch].eVideoCodecType == MEDIA_DEMUX_CODEC_H264 ? 97 : 98;
	stream_info->video_fps = stStreamInfo[ch].nVFramerate;
	stream_info->video_bitrate = stStreamInfo[ch].nVBitrate;
	stream_info->video_width = stStreamInfo[ch].nVWidth;
	stream_info->video_height = stStreamInfo[ch].nVHeight;
	size = sizeof(stream_info->video_extra);
	ret = MediaDemux_GetExtradata(hHandle[ch], 0, stream_info->video_extra, &size);
	stream_info->video_extra_size = ret == 0 ? size : 0;
	printf("func = %s, line = %d: video extradata size = %d \n", __FUNCTION__, __LINE__, stream_info->video_extra_size);
	
	stream_info->is_have_audio = stStreamInfo[ch].nHaveAudio;
	stream_info->audio_payload = stStreamInfo[ch].eAudioCodecType;
	if (stStreamInfo[ch].eAudioCodecType == MEDIA_DEMUX_CODEC_G711A) {
		stream_info->audio_payload = 0x70;
	} else if (stStreamInfo[ch].eAudioCodecType == MEDIA_DEMUX_CODEC_G711U) {
		stream_info->audio_payload = 0x80;
	} else if (stStreamInfo[ch].eAudioCodecType == MEDIA_DEMUX_CODEC_AAC) {
		stream_info->audio_payload = 0xa0;
	} 
	stream_info->audio_samplerate = stStreamInfo[ch].nASamplerate;
	stream_info->audio_chnum = stStreamInfo[ch].nAChannelNum;
	stream_info->audio_bitrate = stStreamInfo[ch].nABitrate;
	size = sizeof(stream_info->audio_extra);
	ret = MediaDemux_GetExtradata(hHandle[ch], 1, stream_info->audio_extra, &size);
	stream_info->audio_extra_size = ret == 0 ? size : 0;
	printf("func = %s, line = %d: audio extradata size = %d \n", __FUNCTION__, __LINE__, stream_info->audio_extra_size);

	return 0;
}

static void *add_rb_reader(char *channel_name)
{
	int ret = -1;
	ringbuf_rlink_t *rb_reader = NULL;
	int ch = 0;
	char *p;

	if (0 != strncmp(channel_name, url_prefix, strlen(url_prefix))) {
		printf("func = %s, line = %d:  \n", __FUNCTION__, __LINE__);
		return NULL;
	}
	p = &channel_name[0] + strlen(url_prefix);
	sscanf(p, "%2d", &ch);
	printf("func = %s, line = %d:  ch = %d\n", __FUNCTION__, __LINE__, ch);
	ch = ch == 0 ? 0 : 1;
	
	rb_reader = malloc(sizeof(ringbuf_rlink_t));
	ret = ringbuf_read_add(rb[ch], rb_reader);
	if (ret != 0) {
		return NULL;
	}

	return rb_reader;
}

static int del_rb_reader(void *rb_reader)
{
	int ret = -1;

	if (rb_reader == NULL) {
		return -1;
	}
	
	ret = ringbuf_read_del((ringbuf_rlink_t *)rb_reader);
	if (ret != 0) {
		return -1;
	}
	free(rb_reader);
	rb_reader = NULL;

	return 0;
}

static int get_rb_stream(void *rb_reader, httpflv_frame_info_t **pkg)
{
	if (rb_reader == NULL) {
		return -1;
	}

	int ret = -1;
	int len = 0;

	ret = ringbuf_read_get_unit((ringbuf_rlink_t *)rb_reader, (unsigned char **)pkg, &len);
	if (ret != 0 || len <= 0) {
		return -1;
	}

	return 0;
}

static int release_rb_stream(void *rb_reader)
{
	if (rb_reader == NULL) {
		return -1;
	}

	int ret = -1;

	ret = ringbuf_read_put_unit((ringbuf_rlink_t *)rb_reader);
	if (ret != 0) {
		return -1;
	}

	return 0;
}




static void signal_handle(int sig)
{
    if(sig == SIGINT)    // ctrl+c
    {
        printf("SIGINT: CTRL+C\n");
		thd_running[0] = 0;
		thd_running[0] = 0;
		usleep(200000);
		httpflv_deinit();
		usleep(200000);
		exit(0);
    }
    else if(sig == SIGQUIT)  // ctrl+/
    {
        printf("SIGQUIT: CTRL+/\n");
    }
    else if(sig == SIGALRM)  // ctrl+/
    {
        printf("SIGALRM: alarm\n");
    }
    else if(sig == SIGPIPE)  // socket FIN
    {
        printf("SIGPIPE: socket FIN\n");
    }
    else
    {
        printf("signal others\n");
    }    
    
}


#define RB_SIZE	(10*1024*1024)

int main(void )
{

	char *rb_buf[2];
	pthread_t tid[2];
	httpflv_context_t ctx = {0};
	int ch;
	

    signal(SIGPIPE, signal_handle);
    signal(SIGINT, signal_handle);


	rb_buf[0] = malloc(RB_SIZE);
	rb_buf[1] = malloc(RB_SIZE);
    ringbuf_create(&rb[0], rb_buf[0], RB_SIZE);
    ringbuf_create(&rb[1], rb_buf[1], RB_SIZE);

	ch = 0;
	thd_running[ch] = 1;
	pthread_create(&tid[ch], NULL, get_stream_thdcb, &ch);
	usleep(200000);
	ch = 1;
	thd_running[ch] = 1;
	pthread_create(&tid[ch], NULL, get_stream_thdcb, &ch);
	sleep(1);

	ctx.port = 80;
	ctx.auth_enable = 0;
	ctx.media_handler.get_stream_info = get_stream_info;
	ctx.media_handler.add_rb_reader = add_rb_reader;
	ctx.media_handler.del_rb_reader = del_rb_reader;
	ctx.media_handler.get_rb_stream = get_rb_stream;
	ctx.media_handler.release_rb_stream = release_rb_stream;
	httpflv_init(&ctx);
	
	while (1) {sleep(10);}

	return 0;
}


