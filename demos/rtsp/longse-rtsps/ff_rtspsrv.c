#include <unistd.h>
#include "libavutil/audio_fifo.h"
#include "libswresample/swresample.h"
#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>
#include "media_demux.h"
#include <pthread.h>

#include "ringbuf.h"
#include "rtspsrv.h"


#define MAX_ENCODE_STREAM_NUM    1
static int fd_iav[MAX_ENCODE_STREAM_NUM];
static int G_vencStream_opened = 0;
static int local_stream[MAX_ENCODE_STREAM_NUM];
static int gRingbufferProcessRun = 0;  //Ringbuffer Work

#define RTSP_SERVER_DEBUG
#ifdef RTSP_SERVER_DEBUG
#define pri_dbg(format, args...) fprintf(stderr,"%s %d %s() " format, __FILE__, __LINE__, __func__, ## args)
#else
#define pri_dbg(format, args...) do{}while(0)
#endif

//static rtsp_handle h;
struct timeval timeout_val, p_tv;
struct timespec ts; 
ringbuf_t *rb;

static void * get_stream_thdcb(void * arg)
{
	pthread_detach(pthread_self());

	MEDIA_DEMUX_HANDLE hHandle;
	MEDIA_DEMUX_FRAME_T stMDFrame;
	MEDIA_DEMUX_STREAM_INFO_T stStreamInfo;
	unsigned char frame_buf[1*1024*1024];	// 1MB
	RtspSvr_Pkg_t stPkg;
	char *p;

    char *input_filename = "../../../modules/ffmpeg/files/sample_cif.h264";
    int ret;

	av_register_all();
	hHandle = MediaDemux_Open(input_filename, &stStreamInfo);
	printf("%s:%d stStreamInfo.nAChannelNum = %d, stStreamInfo.nASamplerate = %d\n", __FUNCTION__, __LINE__, stStreamInfo.nAChannelNum, stStreamInfo.nASamplerate);

	while (1) {
		stMDFrame.pData = &frame_buf[0];
		ret = MediaDemux_ReadFrame(hHandle, &stMDFrame);
		if (ret < 0) {
			MediaDemux_Close(hHandle);
			hHandle = MediaDemux_Open(input_filename, &stStreamInfo);
			continue;
		}

		//printf("%s:%d stMDFrame.eStreamType = %d, stMDFrame.nLen = %d !\n", __FUNCTION__, __LINE__, stMDFrame.eStreamType, stMDFrame.nLen);

		if (stMDFrame.eStreamType == MEDIA_DEMUX_STREAM_TYPE_VIDEO || stMDFrame.eStreamType == MEDIA_DEMUX_STREAM_TYPE_VIDEO_I) {
			stPkg.eStreamType = RTSP_STREAM_VIDEO;
			stPkg.uKeyFrame = stMDFrame.eStreamType == MEDIA_DEMUX_STREAM_TYPE_VIDEO_I ? 1 : 0;
			stPkg.nLen = stMDFrame.nLen;
			stPkg.llPts = stMDFrame.llPts < 0 ? stPkg.llPts + 3600 : stMDFrame.llPts;
			ringbuf_write_get_unit(rb, &p, sizeof(RtspSvr_Pkg_t) + stMDFrame.nLen);
			memcpy(p, &stPkg, sizeof(RtspSvr_Pkg_t));
			memcpy(p + sizeof(RtspSvr_Pkg_t), stMDFrame.pData, stMDFrame.nLen);
			ringbuf_write_put_unit(rb, stMDFrame.nLen);

			//fwrite(stMDFrame.pData, 1, stMDFrame.nLen, pVFile); 
			//printf("Video Data Head: %x %x %x %x %x\n", stMDFrame.pData[0], stMDFrame.pData[1], stMDFrame.pData[2], stMDFrame.pData[3], stMDFrame.pData[4]);
		} else if (stMDFrame.eStreamType == MEDIA_DEMUX_STREAM_TYPE_AUDIO) {
		}

		usleep(38000);
	}

	
	MediaDemux_Close(hHandle);



	return NULL;
}





#define RB_SIZE	(10*1024*1024)

int main(void )
{

	char *rb_buf = NULL;
	pthread_t tid;
	



	rb_buf = malloc(RB_SIZE);
    ringbuf_create(&rb, rb_buf, RB_SIZE);
	
	pthread_create(&tid, NULL, get_stream_thdcb, NULL);






	//new Rtsp
	rtspSLib_ServerStart(554, rb);
	




	
	while (1) {sleep(10);}

	return 0;
}
