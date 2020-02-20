#ifndef _rtmps_api_h_
#define _rtmps_api_h_

//#include "sys/sock.h"
//#include "sys/locker.h"
#include "list.h"
//#include "aio-rtmp-server.h"
//#include "flv-muxer.h"
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct list_head list_head_t;
typedef pthread_mutex_t		locker_t;
typedef struct aio_rtmp_session_t aio_rtmp_session_t;
typedef struct flv_muxer_t flv_muxer_t;
typedef struct aio_rtmp_server_t aio_rtmp_server_t;


// RTSP SERVER SESSION STATUS
enum {
	RTMPS_SESSION_STATUS_SETUP = 0,		// session onsetup
	RTMPS_SESSION_STATUS_PLAY,
	RTMPS_SESSION_STATUS_PAUSE,
	RTMPS_SESSION_STATUS_ERROR,
	RTMPS_SESSION_STATUS_TEARDOWN,		// session onteardown
	RTMPS_SESSION_STATUS_STOPED,		// session thread stoped.
};


typedef struct rtmps_frame_info_s
{
	int 		stream_type;	// FLV_VIDEO_H264/FLV_VIDEO_H265/PSI_STREAM_AAC
	int 		data_len;
	uint64_t	pts;
	char		data[0];
} rtmps_frame_info_t;


typedef struct rtmps_stream_info_s {
	int is_have_video;
	int video_payload;		// FLV_VIDEO_H264 = 0x7, FLV_VIDEO_H265 = 0x12
	int video_fps; 			// 25000 is 25fps
	int video_bitrate;
	int video_width;
	int video_height;
	int is_have_audio;
	int audio_payload;		// FLV_AUDIO_AAC = 0xa0, FLV_AUDIO_MP3 = 0x20
	int audio_samplerate;
	int audio_chnum;
	int audio_bitrate;
} rtmps_stream_info_t;

typedef struct rtmps_session_s {
    list_head_t 			head;         		// list node
    aio_rtmp_session_t		*session;
    char					session_code[128];	// url session code
    uint8_t 				packet[4 * 1024 * 1024];
	int 					status; 			// 0-setup-init, 1-play, 2-pause, 3-error, 4-teardown, 5-closing
	void					*rb_reader;			// ringbuf reader handler
	flv_muxer_t				*flv_muxer;
} rtmps_session_t;

typedef struct rtmps_media_handle_s {
	int (*get_stream_info)(char *channel_name, rtmps_stream_info_t *strem_info);	// on discribe
	void* (*add_rb_reader)(char *channel_name);										// on setup, add ringbuf reader
	int (*del_rb_reader)(void *rb_reader);											// on setup, add ringbuf reader
	int (*get_rb_stream)(void *rb_reader, rtmps_frame_info_t **pkg);				// play send
	int (*release_rb_stream)(void *rb_reader);										// play send
} rtmps_media_handle_t;

typedef struct rtmps_context_s {
	list_head_t				session_list; 		// internal used, list node
	locker_t 				locker; 			// internal used, locker
	aio_rtmp_server_t		*rtmp;				// internal used
	int						port;				// 554
	rtmps_media_handle_t	media_handler;
} rtmps_context_t;


int rtmps_init(rtmps_context_t *ctx);
int rtmps_deinit();


#ifdef __cplusplus
}
#endif
#endif /* !_ringbuf_h_ */
