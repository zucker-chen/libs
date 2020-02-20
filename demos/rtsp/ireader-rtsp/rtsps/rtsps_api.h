#ifndef _rtsps_api_h_
#define _rtsps_api_h_

//#include "sys/sock.h"
//#include <stddef.h>
//#include <inttypes.h>
//#include <stdint.h>
#include <sys/socket.h>
#include <pthread.h>
//#include "sys/locker.h"
#include "list.h"
//#include "rtsp-server.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef pthread_mutex_t		locker_t;
typedef struct list_head list_head_t;
typedef struct rtsp_server_t rtsp_server_t;


// RTSP SERVER SESSION STATUS
enum {
	RTSPS_SESSION_STATUS_SETUP = 0,		// session onsetup
	RTSPS_SESSION_STATUS_PLAY,
	RTSPS_SESSION_STATUS_PAUSE,
	RTSPS_SESSION_STATUS_ERROR,
	RTSPS_SESSION_STATUS_TEARDOWN,		// session onteardown
	RTSPS_SESSION_STATUS_STOPED,		// session thread stoped.
};


typedef struct rtsps_frame_info_s
{
	int 				stream_type;	// RTP_PAYLOAD_H264 = 97, RTP_PAYLOAD_H265 = 98, RTP_PAYLOAD_PCMU = 0, RTP_PAYLOAD_PCMA = 8, RTP_PAYLOAD_MP4A(AAC) = 100
	int 				data_len;
	unsigned long long	pts;			// Note: Align for 64bit machines
	char				data[0];
} rtsps_frame_info_t;


typedef struct rtsps_stream_info_s {
	int		is_have_video;
	int 	video_payload;		// RTP_PAYLOAD_H264 = 97, RTP_PAYLOAD_H265 = 98
	int 	video_fps; 			// 25000 is 25fps
	int 	video_bitrate;
	int 	video_width;
	int 	video_height;
	char	video_extra[1024];	// extra data, sps/pps...
	int 	video_extra_size;
	int 	is_have_audio;
	int 	audio_payload;		// RTP_PAYLOAD_PCMU = 0, RTP_PAYLOAD_PCMA = 8, RTP_PAYLOAD_MP4A(AAC) = 100
	int 	audio_samplerate;
	int 	audio_chnum;
	int 	audio_bitrate;
	char	audio_extra[1024];	// extra data, atds
	int 	audio_extra_size;
} rtsps_stream_info_t;


typedef struct rtsps_rtp_transport_s {
	unsigned char			 	type;						// 1=udp, 2=tcp
	unsigned char 				rtp;						// for tcp
	unsigned char 				rtcp;						// for tcp
	rtsp_server_t				*rtsp;						// for tcp
	// unsigned char			packet[4 + (1 << 16)];		// for tcp
	int	 						udp_socket[2]; 				// for udp
	socklen_t 					udp_addrlen[2];				// for udp
	struct sockaddr_storage 	udp_addr[2];				// for udp
    int (*send)(void* param, int rtcp, const void* data, int bytes);	// tcp send or udp send
} rtsps_rtp_transport_t;

typedef struct rtsps_media_s {
	void					*parent;		// (rtsps_session_t *)

	void					*rtp;
	long long 				dts_first;		// first frame dts
	long long 				dts_last;		// last frame dts
	unsigned long long 		timestamp; 		// rtp timestamp
	unsigned long long 		rtcp_clock;
	unsigned int 			ssrc;
	int 					bandwidth;
	int 					frequency;
	char 					name[64];
	int 					payload;
	void* 					packer; 		// rtp encoder
	unsigned char	 		packet[2048];
	rtsps_rtp_transport_t 	transport;
	int 					track;			// mp4 track, 0 = video, 1 = audio
} rtsps_media_t;

typedef struct rtsps_session_s {
    list_head_t 			head;         		// list node
    char					session_code[128];	// url session code
	rtsps_media_t 			media[2]; 			// 0-video, 1-audio
	int 					status; 			// 0-setup-init, 1-play, 2-pause, 3-error, 4-teardown, 5-closing
	void					*rb_reader;			// ringbuf reader handler
} rtsps_session_t;

typedef struct rtsps_media_handle_s {
	int (*get_stream_info)(char *channel_name, rtsps_stream_info_t *stream_info);	// on discribe, channel_name(like: /live/0)
	void* (*add_rb_reader)(char *channel_name);										// on setup, add ringbuf reader
	int (*del_rb_reader)(void *rb_reader);											// on setup, add ringbuf reader
	int (*get_rb_stream)(void *rb_reader, rtsps_frame_info_t **pkg);				// play send
	int (*release_rb_stream)(void *rb_reader);										// play send
} rtsps_media_handle_t;

typedef struct rtsps_context_s {
	list_head_t				session_list; 		// internal used, list node
	locker_t 				locker; 			// internal used, locker
	void					*tcp_handle;		// internal used
	//void					*udp_handle;		// internal used
	int						port;				// 554
	int						auth_enable;
	rtsps_media_handle_t	media_handler;
} rtsps_context_t;


int rtsps_init(rtsps_context_t *ctx);
int rtsps_deinit();


#ifdef __cplusplus
}
#endif
#endif /* !_ringbuf_h_ */
