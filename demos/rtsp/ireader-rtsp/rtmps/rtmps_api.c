#include "sockutil.h"
#include "sys/locker.h"
#include "sys/atomic.h"
#include "sys/thread.h"
#include "sys/system.h"
#include "aio-worker.h"
#include "flv-reader.h"
#include "flv-proto.h"
#include "flv-muxer.h"
#include "aio-rtmp-server.h"
#include "rtmps_api.h"
#include <string.h>
#include <assert.h>



#define N_AIO_THREAD 8

static rtmps_context_t *rtmps_cxt = NULL;


static int on_flv_packet(void* flv, int type, const void* data, size_t bytes, uint32_t timestamp)
{
	//printf("func = %s, line = %d:  \n", __FUNCTION__, __LINE__);
	rtmps_session_t *rss = (rtmps_session_t *)flv;
	int r;


	if (FLV_TYPE_AUDIO == type)
	{
		r =  aio_rtmp_server_send_audio(rss->session, data, bytes, timestamp);
	}
	else if (FLV_TYPE_VIDEO == type)
	{
		r =  aio_rtmp_server_send_video(rss->session, data, bytes, timestamp);
	}
	else if (FLV_TYPE_SCRIPT == type)
	{
		r =  aio_rtmp_server_send_script(rss->session, data, bytes, timestamp);
	}
	else
	{
		//assert(0);
		r = 0;
	}

	return r;
}


static int rtmps_session_create(rtmps_session_t *rss, char *channel_name)
{
	int ret;
	
	if (rss == NULL || channel_name == NULL) {
		assert(0);
		return -1;
	}

	rss->rb_reader = rtmps_cxt->media_handler.add_rb_reader(channel_name);
	if (rss->rb_reader == NULL) {
		assert(rss->rb_reader);
		return -1;
	}
	snprintf(rss->session_code, sizeof(rss->session_code), "%p", &rss->session);		// set a session code.
	list_insert_after(&rss->head, &rtmps_cxt->session_list);

	rss->flv_muxer = flv_muxer_create(on_flv_packet, rss);

	
	return 0;
}

static int rtmps_session_destroy(rtmps_session_t *rss)
{
	if (rss == NULL) {
		assert(0);
		return -1;
	}

	locker_lock(&rtmps_cxt->locker);

	if (rss->rb_reader != NULL) {
		rtmps_cxt->media_handler.del_rb_reader(rss->rb_reader);
	}
	if (rss->flv_muxer != NULL) {
		flv_muxer_destroy(rss->flv_muxer);
	}
	
	list_remove(&rss->head);
	free(rss);
	rss = NULL;
	locker_unlock(&rtmps_cxt->locker);
	
	return 0;
}

static int STDCALL rtmps_worker(void* param)
{
	int ret;
	rtmps_frame_info_t *pkg;
	rtmps_session_t* rss = (rtmps_session_t*)param;
	rss->status = RTMPS_SESSION_STATUS_PLAY;
	usleep(200000);

	while (1)
	{
		usleep(10000);
		if (RTMPS_SESSION_STATUS_PLAY == rss->status) {
			if (rss->rb_reader == NULL || rtmps_cxt->media_handler.get_rb_stream == NULL || rtmps_cxt->media_handler.release_rb_stream == NULL) {
				rtmps_session_destroy(rss);
				printf("func = %s, line = %d:  \n", __FUNCTION__, __LINE__);
				continue;
			}

			if (aio_rtmp_server_get_unsend(rss->session) > 8 * 1024 * 1024) {
				printf("func = %s, line = %d:  \n", __FUNCTION__, __LINE__);
				continue;
			}
			
			ret = rtmps_cxt->media_handler.get_rb_stream(rss->rb_reader, &pkg);
			if (ret == -1) {
				//printf("func = %s, line = %d:  \n", __FUNCTION__, __LINE__);
				continue;
			}

			//printf("func = %s, line = %d: pkg->stream_type = %d \n", __FUNCTION__, __LINE__, pkg->stream_type);
			if (pkg->stream_type == FLV_VIDEO_H264) {
				ret = flv_muxer_avc(rss->flv_muxer, pkg->data, pkg->data_len, (uint32_t)(pkg->pts / 90), (uint32_t)(pkg->pts / 90));
				//printf("func = %s, line = %d: ret = %d \n", __FUNCTION__, __LINE__, ret);
			} else if (pkg->stream_type == FLV_VIDEO_H265) {
				ret = flv_muxer_hevc(rss->flv_muxer, pkg->data, pkg->data_len, (uint32_t)(pkg->pts / 90), (uint32_t)(pkg->pts / 90));
			} else if (pkg->stream_type == FLV_AUDIO_AAC) {
				ret = flv_muxer_aac(rss->flv_muxer, pkg->data, pkg->data_len, (uint32_t)(pkg->pts / 90), (uint32_t)(pkg->pts / 90));
			} else {

			}
			rtmps_cxt->media_handler.release_rb_stream(rss->rb_reader);
		}else if (RTMPS_SESSION_STATUS_ERROR == rss->status || RTMPS_SESSION_STATUS_TEARDOWN == rss->status) {
			printf("func = %s, line = %d:  \n", __FUNCTION__, __LINE__);
			locker_lock(&rtmps_cxt->locker);
			rss->status = RTMPS_SESSION_STATUS_STOPED;
			locker_unlock(&rtmps_cxt->locker);
			break;
		} else {
			printf("func = %s, line = %d:  \n", __FUNCTION__, __LINE__);
			continue;
		}
	}

	return 0;
}

static aio_rtmp_userptr_t rtmps_onplay(void* param, aio_rtmp_session_t* session, const char* app, const char* stream, double start, double duration, uint8_t reset)
{
	printf("func = %s, line = %d:  \n", __FUNCTION__, __LINE__);
	printf("rtmps_onplay(%s, %s, %f, %f, %d)\n", app, stream, start, duration, (int)reset);

	rtmps_session_t* rss = NULL;
	char channel_name[128] = "\0";
	
	rss = calloc(1, sizeof(rtmps_session_t));
	if (rss == NULL) {
		assert(0);
	} 
	rss->session = session;
	sprintf(channel_name, "/%s/%s", app, stream);
	rtmps_session_create(rss, channel_name);

	pthread_t t;
	thread_create(&t, rtmps_worker, rss);
	thread_detach(t);
	
	return rss;
}

static int rtmps_onpause(aio_rtmp_userptr_t ptr, int pause, uint32_t ms)
{
	printf("func = %s, line = %d:  \n", __FUNCTION__, __LINE__);
	printf("rtmps_onpause(%d, %u)\n", pause, (unsigned int)ms);
	return 0;
}

static int rtmps_onseek(aio_rtmp_userptr_t ptr, uint32_t ms)
{
	printf("func = %s, line = %d:  \n", __FUNCTION__, __LINE__);
	printf("rtmps_onseek(%u)\n", (unsigned int)ms);
	return 0;
}

static int rtmps_ongetduration(void* param, const char* app, const char* stream, double* duration)
{
	printf("func = %s, line = %d:  \n", __FUNCTION__, __LINE__);
	*duration = 30 * 60;
	return 0;
}

static void rtmps_onsend(aio_rtmp_userptr_t ptr, size_t bytes)
{
	//printf("func = %s, line = %d:  \n", __FUNCTION__, __LINE__);
}

static void rtmps_onclose(aio_rtmp_userptr_t ptr)
{
	printf("func = %s, line = %d:  \n", __FUNCTION__, __LINE__);
	// close thread
	rtmps_session_t *rss = (rtmps_session_t *)ptr;
	int count = 0;
	
	if (rss == NULL) {
		return ;
	}

	locker_lock(&rtmps_cxt->locker);
	rss->status = RTMPS_SESSION_STATUS_TEARDOWN;
	locker_unlock(&rtmps_cxt->locker);
	while (rss->status != RTMPS_SESSION_STATUS_STOPED && count++ < 100)
	{
		usleep(100000);
	}
	
	rtmps_session_destroy(rss);
}

int rtmps_init(rtmps_context_t *ctx)
{
	if (ctx == NULL) {
		return -1;
	}
	
	aio_rtmp_server_t* rtmp;
	struct aio_rtmp_server_handler_t handler;

	rtmps_cxt = ctx;
	memset(&handler, 0, sizeof(handler));
	handler.onsend = rtmps_onsend;
	handler.onplay = rtmps_onplay;
	handler.onpause = rtmps_onpause;
	handler.onseek = rtmps_onseek;
	handler.onclose = rtmps_onclose;
	handler.ongetduration = rtmps_ongetduration;

	aio_worker_init(N_AIO_THREAD);
    LIST_INIT_HEAD(&rtmps_cxt->session_list);
	if(0 != locker_create(&rtmps_cxt->locker)) {
		return -1;
	}
	rtmp =  aio_rtmp_server_create(NULL, ctx->port, &handler, NULL);
	if (ctx == NULL) {
		return -1;
	}

	return 0;
}

int rtmps_deinit()
{
	aio_rtmp_server_destroy(rtmps_cxt->rtmp);
	aio_worker_clean(N_AIO_THREAD);
	
	return 0;
}




