#include "sockutil.h"
#include "sock.h"
#include "byte-order.h"
#include "sys/locker.h"
#include "sys/atomic.h"
#include "sys/thread.h"
#include "sys/system.h"
#include "aio-worker.h"
#include "flv-reader.h"
#include "flv-proto.h"
#include "flv-muxer.h"
#include "aio-tcp-transport.h"
#include "urlcodec.h"
#include "uri-parse.h"
#include "httpflv_api.h"
#include <string.h>
#include <assert.h>



#define N_AIO_THREAD 8

static httpflv_context_t *httpflv_cxt = NULL;

extern struct uri_t* uri_parse(const char* uri, int len);
extern void uri_free(struct uri_t* uri);


static int httpflv_tcp_send(void* ptr, const void* data, size_t bytes)
{
	http_session_t	*session;
	socket_t socket;
	int r, times = 50;
	
	session = (http_session_t *)ptr;
	
	//aio_tcp_transport_t* transport = NULL;
	//transport = *(aio_tcp_transport_t **)((char *)session + sizeof(void *));			// session->transport
	//return aio_tcp_transport_send(session->transport, data, bytes);

	// TODO: send multiple rtp packet once time
	socket = *(socket_t *)((char *)session + sizeof(void *) + sizeof(void *));			// session->socket
	do {
		assert(session);
		r = socket_send(socket, data, bytes, 0);
		if (r != bytes) {
			usleep(2000);
		}
	} while (times-- > 0 && r != bytes);
	
	return bytes == r ? 0 : -1;
}




static int on_flv_packet(void* param, int type, const void* data, size_t bytes, uint32_t timestamp)
{
	//printf("func = %s, line = %d:  \n", __FUNCTION__, __LINE__);
	httpflv_session_t *rss = (httpflv_session_t *)param;
	unsigned char chunk[32] = {0};
	unsigned char  *tag;
	int r, size;

	// 1. chunk size
	size = sprintf(chunk, "%x\r\n", bytes+11+4);

	// 2. flv tag packet
	// 2.1 TagType
	tag = &chunk[size];
	tag[0] = type & 0x1F;
	// 2.2 DataSize
	tag[1] = (bytes >> 16) & 0xFF;
	tag[2] = (bytes >> 8) & 0xFF;
	tag[3] = bytes & 0xFF;
	// 2.3 Timestamp
	tag[4] = (timestamp >> 16) & 0xFF;
	tag[5] = (timestamp >> 8) & 0xFF;
	tag[6] = (timestamp >> 0) & 0xFF;
	tag[7] = (timestamp >> 24) & 0xFF; // Timestamp Extended
	// 2.4 StreamID(Always 0)
	tag[8] = 0;
	tag[9] = 0;
	tag[10] = 0;
	size += 11;
	r = httpflv_tcp_send(rss->session, chunk, size);
	if (r != 0) goto SEND_ERR;
	
	// 2.5 Data
	r = httpflv_tcp_send(rss->session, data, bytes);
	//printf("func = %s, line = %d: size %d %x \n", __FUNCTION__, __LINE__, r, bytes);
	if (r != 0) goto SEND_ERR;
	
	// 2.6 tag size and \r\n
	be_write_uint32(chunk, (unsigned int)bytes + 11);
	memcpy(chunk+4, "\r\n", 2);
	size = 6;
	r = httpflv_tcp_send(rss->session, chunk, size);
	//printf("func = %s, line = %d: size %d %x \n", __FUNCTION__, __LINE__, r, size);
	if (r != 0) goto SEND_ERR;

	return 0;
	
SEND_ERR:
	printf("func = %s, line = %d: Warnning, socket send error \n", __FUNCTION__, __LINE__);
	return -1;
}


static int httpflv_session_create(httpflv_session_t *rss, char *channel_name)
{
	int ret;
	
	if (rss == NULL || channel_name == NULL) {
		assert(0);
		return -1;
	}

	rss->rb_reader = httpflv_cxt->media_handler.add_rb_reader(channel_name);
	if (rss->rb_reader == NULL) {
		assert(rss->rb_reader);
		return -1;
	}
	snprintf(rss->session_code, sizeof(rss->session_code), "%p", &rss->session);		// set a session code.
	list_insert_after(&rss->head, &httpflv_cxt->session_list);

	rss->flv_muxer = flv_muxer_create(on_flv_packet, rss);
	printf("func = %s, line = %d: %s \n", __FUNCTION__, __LINE__, rss->session_code);
	
	return 0;
}

static int httpflv_session_destroy(httpflv_session_t *rss)
{
	if (rss == NULL) {
		assert(0);
		return -1;
	}

	locker_lock(&httpflv_cxt->locker);

	if (rss->rb_reader != NULL) {
		httpflv_cxt->media_handler.del_rb_reader(rss->rb_reader);
	}
	if (rss->flv_muxer != NULL) {
		flv_muxer_destroy(rss->flv_muxer);
	}
	
	printf("func = %s, line = %d: %s \n", __FUNCTION__, __LINE__, rss->session_code);
	list_remove(&rss->head);
	free(rss);
	rss = NULL;
	locker_unlock(&httpflv_cxt->locker);
	
	return 0;
}


static int httpflv_session_setup(httpflv_session_t *rss, char *channel_name)
{
	int ret = 0;
	httpflv_stream_info_t stream_info = {0};

	ret = httpflv_cxt->media_handler.get_stream_info(channel_name, &stream_info);
	if (ret != 0) {
		assert(0);
		return -1;
	}

	// ===================== send flv head  =============================
	unsigned char buf[64] = {0};
	int size = 0;
	unsigned char header[13] = {0};
	
	header[0] = 'F'; // FLV signature
	header[1] = 'L';
	header[2] = 'V';
	header[3] = 0x01; // File version
	//header[4] = 0x05; // 0x05 = Type flags (audio & video)
	if (stream_info.is_have_video == 1) header[4] |= 0x1;
	if (stream_info.is_have_audio == 1) header[4] |= 0x4;
	be_write_uint32(header + 5, 9); // Data offset
	be_write_uint32(header + 9, 0); // PreviousTagSize0(Always 0)

	size += sprintf(buf, "%x\r\n", sizeof(header));
	memcpy(buf+size, header, sizeof(header));
	size += sizeof(header);
	memcpy(buf+size, "\r\n", 2);
	size += 2;
	ret = httpflv_tcp_send(rss->session, buf, size);
	// ===================== send flv head end =========================

	// ===================== send flv scrtip start =====================
	struct flv_metadata_t metadata;
	metadata.videocodecid = stream_info.is_have_video == 1 ? stream_info.video_payload : 0;
	metadata.videodatarate = stream_info.video_bitrate;
	metadata.framerate = stream_info.video_fps;
	metadata.width = stream_info.video_width;
	metadata.height = stream_info.video_height;
	metadata.audiocodecid = stream_info.is_have_audio == 1 ? stream_info.audio_payload>>4 : 0;
	metadata.audiodatarate = stream_info.audio_bitrate;
	metadata.audiosamplerate = stream_info.audio_samplerate;
	metadata.audiosamplesize = 32;
	metadata.stereo = stream_info.audio_chnum > 1 ? 1 : 0;
	flv_muxer_metadata(rss->flv_muxer, &metadata);
	// ===================== send flv scrtip ===========================

	// SPS PPS
	//usleep(10000);
	//ret = flv_muxer_avc(rss->flv_muxer, stream_info.video_extra, stream_info.video_extra_size, 0, 0 );

	return 0;
}

static int httpflv_worker(void* param)
{
	int ret, delay_optimize_ms, first_pts_ms, pts_ms;
	httpflv_frame_info_t *pkg;
	httpflv_session_t* rss = (httpflv_session_t*)param;
	
	rss->status = HTTPFLV_SESSION_STATUS_PLAY;
	usleep(20000);
	delay_optimize_ms = 7000;	// 7s
	first_pts_ms = 0;

	while (1)
	{
		usleep(10000);
		if (HTTPFLV_SESSION_STATUS_PLAY == rss->status) {
			if (rss->rb_reader == NULL || httpflv_cxt->media_handler.get_rb_stream == NULL || httpflv_cxt->media_handler.release_rb_stream == NULL) {
				httpflv_session_destroy(rss);
				printf("func = %s, line = %d:  \n", __FUNCTION__, __LINE__);
				continue;
			}

			ret = httpflv_cxt->media_handler.get_rb_stream(rss->rb_reader, &pkg);
			if (ret == -1) {
				//printf("func = %s, line = %d:  \n", __FUNCTION__, __LINE__);
				continue;
			}

			pts_ms = (uint32_t)pkg->pts;
			
			#if 1	// Optimize client playback delay
				if (first_pts_ms == 0) first_pts_ms = pts_ms;
				if (pts_ms - first_pts_ms < delay_optimize_ms) {
					pts_ms = (delay_optimize_ms - 1000) + first_pts_ms + (pts_ms - first_pts_ms)*1000/delay_optimize_ms;
				}
			#endif

			//printf("func = %s, line = %d: pkg->stream_type = %d \n", __FUNCTION__, __LINE__, pkg->stream_type);
			if (pkg->stream_type == FLV_VIDEO_H264) {
				ret = flv_muxer_avc(rss->flv_muxer, pkg->data, pkg->data_len, pts_ms, pts_ms);
				//printf("func = %s, line = %d: ret = %d \n", __FUNCTION__, __LINE__, ret);
			} else if (pkg->stream_type == FLV_VIDEO_H265) {
				ret = flv_muxer_hevc(rss->flv_muxer, pkg->data, pkg->data_len, pts_ms, pts_ms);
				printf("func = %s, line = %d:  \n", __FUNCTION__, __LINE__);
			} else if (pkg->stream_type == FLV_AUDIO_AAC) {
				//printf("func = %s, line = %d:  \n", __FUNCTION__, __LINE__);
				ret = flv_muxer_aac(rss->flv_muxer, pkg->data, pkg->data_len, pts_ms, pts_ms);
			} else {

			}
			httpflv_cxt->media_handler.release_rb_stream(rss->rb_reader);
			if (ret < 0) rss->status = HTTPFLV_SESSION_STATUS_ERROR;
		}else if (HTTPFLV_SESSION_STATUS_ERROR == rss->status || HTTPFLV_SESSION_STATUS_TEARDOWN == rss->status) {
			printf("func = %s, line = %d:  \n", __FUNCTION__, __LINE__);
			locker_lock(&httpflv_cxt->locker);
			rss->status = HTTPFLV_SESSION_STATUS_STOPED;
			locker_unlock(&httpflv_cxt->locker);
			break;
		} else {
			printf("func = %s, line = %d:  \n", __FUNCTION__, __LINE__);
			break;
		}
	}
	httpflv_session_destroy(rss);


	return 0;
}

static int httpflv_server_route(void* http, http_session_t* session, const char* method, const char* path)
{
	httpflv_session_t* rss = NULL;
	struct uri_t* uri = NULL;
	char reqpath[1024];//[PATH_MAX];
	int ret = 0;

	uri = uri_parse(path, strlen(path));
	url_decode(uri->path, -1, reqpath, sizeof(reqpath));
	uri_free(uri);

	rss = calloc(1, sizeof(httpflv_session_t));
	if (rss == NULL) {
		assert(0);
		return http_server_send(session, 404, "", 0, NULL, NULL);
	} 
	
	ret = httpflv_session_create(rss, reqpath);
	if (ret < 0) {
		return http_server_send(session, 404, "", 0, NULL, NULL);
	}
	rss->session = session;

	http_server_set_header(session, "Server", "http-flv/1.0.2");
	http_server_set_header(session, "Content-Type", "video/x-flv");
	http_server_set_header(session, "Transfer-Encoding", "chunked");
	http_server_set_header(session, "Connection", "keep-alive");
	//http_server_set_header(session, "Connection", "close");
	http_server_set_header(session, "Expires", "-1");
	http_server_set_header(session, "Cache-Control", "no-cache");
	http_server_set_header(session, "Access-Control-Allow-Origin", "*");
	http_server_set_header(session, "Access-Control-Allow-Credentials", "true");

	http_server_send(session, 200, "", 0, NULL, NULL);

	usleep(20000);		// must be sleep
	httpflv_session_setup(rss, reqpath);
	httpflv_worker((void *)rss);

	return 0;
}


int httpflv_init(httpflv_context_t *ctx)
{
	if (ctx == NULL) {
		return -1;
	}
	
	httpflv_cxt = ctx;

	aio_worker_init(N_AIO_THREAD);

	LIST_INIT_HEAD(&httpflv_cxt->session_list);
	if(0 != locker_create(&httpflv_cxt->locker)) {
		return -1;
	}
	
	httpflv_cxt->http = http_server_create(NULL, httpflv_cxt->port);
	http_server_set_handler(httpflv_cxt->http, httpflv_server_route, httpflv_cxt->http);

	return 0;
}

int httpflv_deinit()
{
	http_server_destroy(httpflv_cxt->http);
	aio_worker_clean(N_AIO_THREAD);
	
	return 0;
}




