#ifndef _httpflv_api_h_
#define _httpflv_api_h_

#include "list.h"
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct list_head list_head_t;
typedef pthread_mutex_t		locker_t;
typedef struct flv_muxer_t flv_muxer_t;
typedef struct http_server_t http_server_t;
typedef struct http_session_t http_session_t;


// HTTPFLV SESSION STATUS
enum {
	HTTPFLV_SESSION_STATUS_SETUP = 0,		// session onsetup
	HTTPFLV_SESSION_STATUS_PLAY,
	HTTPFLV_SESSION_STATUS_PAUSE,
	HTTPFLV_SESSION_STATUS_ERROR,
	HTTPFLV_SESSION_STATUS_TEARDOWN,		// session onteardown
	HTTPFLV_SESSION_STATUS_STOPED,		// session thread stoped.
};


typedef struct httpflv_frame_info_s
{
	int 		stream_type;	// FLV_VIDEO_H264 = 0x7, FLV_VIDEO_H265 = 0xc, PSI_STREAM_AAC = 0xa0 FLV_AUDIO_G711 = 0x70
	int 		data_len;
	uint64_t	pts;			// unit: ms
	char		data[0];
} httpflv_frame_info_t;


typedef struct httpflv_stream_info_s {
	int 	is_have_video;
	int 	video_payload;		// FLV_VIDEO_H264 = 0x7, FLV_VIDEO_H265 = 0xc
	int 	video_fps; 			// 25000 is 25fps
	int 	video_bitrate;
	int 	video_width;
	int 	video_height;
	char	video_extra[1024];	// extra data, sps/pps...
	int 	video_extra_size;
	int 	is_have_audio;
	int 	audio_payload;		// FLV_AUDIO_AAC = 0xa0, FLV_AUDIO_MP3 = 0x20
	int 	audio_samplerate;
	int 	audio_chnum;
	int 	audio_bitrate;
	char	audio_extra[1024];	// extra data, atds
	int 	audio_extra_size;
} httpflv_stream_info_t;

typedef struct httpflv_session_s {
    list_head_t 			head;         		// list node
	http_session_t			*session;
	void* 					param;
    char					session_code[128];	// url session code
	int 					status; 			// 0-setup-init, 1-play, 2-pause, 3-error, 4-teardown, 5-closing
	void					*rb_reader;			// ringbuf reader handler
	flv_muxer_t				*flv_muxer;
} httpflv_session_t;

typedef struct httpflv_media_handle_s {
	int (*get_stream_info)(char *channel_name, httpflv_stream_info_t *strem_info);	// on discribe
	void* (*add_rb_reader)(char *channel_name);										// on setup, add ringbuf reader
	int (*del_rb_reader)(void *rb_reader);											// on setup, add ringbuf reader
	int (*get_rb_stream)(void *rb_reader, httpflv_frame_info_t **pkg);				// play send
	int (*release_rb_stream)(void *rb_reader);										// play send
} httpflv_media_handle_t;

typedef struct httpflv_context_s {
	list_head_t				session_list; 		// internal used, list node
	http_server_t			*http;				// internal used
	locker_t 				locker; 			// internal used, locker
	httpflv_media_handle_t	media_handler;
	int						auth_enable;
	int						port;				// 80
} httpflv_context_t;


int httpflv_init(httpflv_context_t *ctx);
int httpflv_deinit();


#ifdef __cplusplus
}
#endif
#endif /* !_ringbuf_h_ */
