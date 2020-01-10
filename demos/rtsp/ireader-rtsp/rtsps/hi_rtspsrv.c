#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <signal.h>
#include <sys/time.h>
#include "mpi_venc.h"
#include "hi_comm_venc.h"
#include "hi_comm_video.h"

#include "ringbuf.h"
#include "rtsps_api.h"


#define RTSP_SERVER_DEBUG
#ifdef RTSP_SERVER_DEBUG
#define pri_dbg(format, args...) fprintf(stderr,"%s %d %s() " format, __FILE__, __LINE__, __func__, ## args)
#else
#define pri_dbg(format, args...) do{}while(0)
#endif


char *url_prefix = "/live/";
ringbuf_t *rb[2];
rtsps_stream_info_t stream_info[2];
static int thd_running[2];


static inline uint64_t system_mstime(void)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (uint64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

static void * get_stream_thdcb(void * arg)
{
	pthread_detach(pthread_self());

	VENC_STREAM_S stHiVencStream;
	VENC_CHN_STATUS_S stHiVencStat;
	#define VENC_DEFAULT_PACK_NUM (20)
	int snVencPackNum = 0;
	VENC_PACK_S *pVencPack = NULL;
	rtsps_frame_info_t stPkg = {0};
	int nFrameSize = 0, i = 0;
	char *p;
    int nRet, nCh;

	nCh = arg != NULL ? *(int *)arg : 0;
	
	pVencPack = malloc(VENC_DEFAULT_PACK_NUM*sizeof(VENC_PACK_S));
	snVencPackNum = VENC_DEFAULT_PACK_NUM;

	printf("func = %s, line = %d:  ch = %d\n", __FUNCTION__, __LINE__, nCh);
	while (thd_running[nCh] == 1) 
	{
		memset(&stHiVencStat, 0, sizeof(stHiVencStat));
		nRet = HI_MPI_VENC_QueryStatus(nCh, &stHiVencStat);
		if (HI_SUCCESS != nRet || stHiVencStat.u32CurPacks == 0 || stHiVencStat.u32LeftStreamFrames == 0)
		{
			//printf("func = %s, line = %d:  ch = %d\n", __FUNCTION__, __LINE__, nCh);
			usleep(10000);
			continue;
		}

		//printf("func = %s, line = %d:  ch = %d\n", __FUNCTION__, __LINE__, nCh);
		if (snVencPackNum < stHiVencStat.u32CurPacks) {
			if (pVencPack) {
				free(pVencPack);
			}
			snVencPackNum = stHiVencStat.u32CurPacks;
			pVencPack = malloc(snVencPackNum*sizeof(VENC_PACK_S));
			if (pVencPack == NULL) {
				snVencPackNum = 0;
				return NULL;
			}
		}

		stHiVencStream.pstPack = pVencPack; 	
		stHiVencStream.u32PackCount = stHiVencStat.u32CurPacks;
		nRet = HI_MPI_VENC_GetStream(nCh, &stHiVencStream, 50);//HAL_CODEC_GET_VENC_TIMEOUT_MS);
		if(nRet != 0)
		{
			continue;
		}

		nFrameSize = 0;
		for(i = 0; i < stHiVencStream.u32PackCount; i++)
		{
			nFrameSize += stHiVencStream.pstPack[i].u32Len - stHiVencStream.pstPack[i].u32Offset;
		}
		stPkg.data_len = nFrameSize;
		stPkg.stream_type = nCh == 0 ? 98 : 97; //RTP_PAYLOAD_H265;
		stPkg.key_frame = 0;
		stPkg.pts = stHiVencStream.pstPack[0].u64PTS / 1000 * 90;	// us -> 1/90000
		nRet = ringbuf_write_get_unit(rb[nCh], (unsigned char **)&p, stPkg.data_len);
		if (nRet != 0) {
			printf("func = %s, line = %d:  ch = %d, nFrameSize = %d\n", __FUNCTION__, __LINE__, nCh, nFrameSize);
			continue; 
		}
		memcpy(p, &stPkg, sizeof(rtsps_frame_info_t));
		p += sizeof(rtsps_frame_info_t);

		for(i = 0; i < stHiVencStream.u32PackCount; i++)
		{
			memcpy(p, stHiVencStream.pstPack[i].pu8Addr + stHiVencStream.pstPack[i].u32Offset, stHiVencStream.pstPack[i].u32Len - stHiVencStream.pstPack[i].u32Offset);
			//(nCh == 1) && printf("u32PackCount = %d, Video Data Head: %x %x %x %x %x\n", stHiVencStream.u32PackCount, p[0], p[1], p[2], p[3], p[4]);
			p += stHiVencStream.pstPack[i].u32Len - stHiVencStream.pstPack[i].u32Offset;
		}
		
		ringbuf_write_put_unit(rb[nCh], sizeof(rtsps_frame_info_t) + stPkg.data_len);

		#if 0 // test
		if (nCh == 1 && stHiVencStream.pstPack[0].DataType.enH265EType != 1) {
			p = stHiVencStream.pstPack[0].pu8Addr + stHiVencStream.pstPack[0].u32Offset;
			printf("u32PackCount = %d, size = %d, Video Data Head: %x %x %x %x %x\n", stHiVencStream.u32PackCount, nFrameSize, p[0], p[1], p[2], p[3], p[4]);
		}
		#endif 
		//(nCh == 1) && printf("func = %s, line = %d:  ch = %d, type = %d, pts = %llu\n", __FUNCTION__, __LINE__, nCh, stHiVencStream.pstPack[0].DataType.enH265EType, stPkg.pts);
		
		HI_MPI_VENC_ReleaseStream(nCh, &stHiVencStream);	

		usleep(10000);
	}

	printf("func = %s, line = %d:  ch = %d\n", __FUNCTION__, __LINE__, nCh);
	free(pVencPack);
	pVencPack = NULL;

	return NULL;
}


static int get_stream_info(char *channel_name, rtsps_stream_info_t *strem_info)
{
	if (channel_name == NULL || strem_info == NULL) {
		return -1;
	}

	int ch = 0;
	char *p;

	if (0 != strncmp(channel_name, url_prefix, strlen(url_prefix))) {
		printf("func = %s, line = %d:  \n", __FUNCTION__, __LINE__);
		return -1;
	}
	p = &channel_name[0] + strlen(url_prefix);
	sscanf(p, "%2d", &ch);
	printf("func = %s, line = %d:  ch = %d\n", __FUNCTION__, __LINE__, ch);
	ch = ch == 0 ? 0 : 1;
	
	strem_info->is_have_video = 1;
	strem_info->video_payload = ch == 0 ? 98 : 97;
	strem_info->video_fps = 30;
	strem_info->video_bitrate = 2048000;
	strem_info->video_width = 1920;
	strem_info->video_height = 1080;
	strem_info->is_have_audio = 0;
	strem_info->audio_payload = 0;
	strem_info->audio_samplerate = 8000;
	strem_info->audio_chnum = 1;
	strem_info->audio_bitrate = 1024;

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

static int get_rb_stream(void *rb_reader, rtsps_frame_info_t **pkg)
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
		rtsps_deinit();
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
	rtsps_context_t ctx;
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

	ctx.port = 554;
	ctx.auth_enable = 1;
	ctx.media_handler.get_stream_info = get_stream_info;
	ctx.media_handler.add_rb_reader = add_rb_reader;
	ctx.media_handler.del_rb_reader = del_rb_reader;
	ctx.media_handler.get_rb_stream = get_rb_stream;
	ctx.media_handler.release_rb_stream = release_rb_stream;
	rtsps_init(&ctx);
	
	while (1) {sleep(10);}

	return 0;
}


