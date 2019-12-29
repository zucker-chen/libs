#ifndef _rtsps_api_h_
#define _rtsps_api_h_

#include "thread-pool.h"
#include "sys/sock.h"
#include "sys/locker.h"
#include "list.h"
#include "rtsp-server.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct list_head list_head_t;

typedef struct rtsps_frame_info_s
{
	int 		stream_type;	// RTP_PAYLOAD_H264/RTP_PAYLOAD_PCMU
	int			key_frame;		// 0: P, 1:IDR, 2:SPS, 3:PPS
	int 		data_len;
	uint64_t	pts;
	char		data[0];
} rtsps_frame_info_t;


typedef struct rtsps_stream_info_s {
	int is_have_video;
	int video_payload;		// RTP_PAYLOAD_H264 = 97, RTP_PAYLOAD_H265 = 98
	int video_fps; 			// 25000 is 25fps
	int video_bitrate;
	int video_width;
	int video_height;
	int is_have_audio;
	int audio_payload;		// RTP_PAYLOAD_PCMU = 0, RTP_PAYLOAD_PCMA = 8
	int audio_samplerate;
	int audio_chnum;
	int audio_bitrate;
} rtsps_stream_info_t;


typedef struct rtsps_rtp_transport_s {
	uint8_t type;						// 1=udp, 2=tcp
	uint8_t rtp;						// for tcp
	uint8_t rtcp;						// for tcp
	rtsp_server_t* rtsp;				// for tcp
	// uint8_t packet[4 + (1 << 16)];		// for tcp
	socket_t udp_socket[2]; 			// for udp
	socklen_t udp_addrlen[2];			// for udp
	struct sockaddr_storage udp_addr[2];// for udp
    int (*send)(void* param, int rtcp, const void* data, int bytes);	// tcp send or udp send
} rtsps_rtp_transport_t;

typedef struct rtsps_media_s {
	void		*parent;		// (rtsps_session_t *)

	void		*rtp;
	int64_t 	dts_first;		// first frame dts
	int64_t 	dts_last;		// last frame dts
	uint64_t 	timestamp; 		// rtp timestamp
	uint64_t 	rtcp_clock;

	uint32_t 	ssrc;
	int 		bandwidth;
	int 		frequency;
	char 		name[64];
	int 		payload;
	void* 		packer; // rtp encoder
	uint8_t 	packet[2048];
	int 		track;								// mp4 track
} rtsps_media_t;

typedef struct rtsps_session_s {
    list_head_t 			head;         		// list node
    char					session_code[128];	// url session code
	rtsps_media_t 			media[2]; 			// 0-video, 1-audio
	rtsps_rtp_transport_t 	transport;
    
	int 					status; 			// 0-setup-init, 1-play, 2-pause, 3-error, 4-close
	void					*rb_reader;			// ringbuf reader handler
} rtsps_session_t;

typedef struct rtsps_media_handle_s {
	int (*get_stream_info)(char *channel_name, rtsps_stream_info_t *strem_info);	// on discribe
	void* (*add_rb_reader)(char *channel_name);										// on setup, add ringbuf reader
	int (*del_rb_reader)(void *rb_reader);											// on setup, add ringbuf reader
	int (*get_rb_stream)(void *rb_reader, rtsps_frame_info_t **pkg);				// play send
	int (*release_rb_stream)(void *rb_reader);										// play send
} rtsps_media_handle_t;

typedef struct rtsps_context_s {
	list_head_t				session_list; 		// internal used, list node
	locker_t 				locker; 			// internal used, locker
	thread_pool_t			thread_pool;		// internal used
	void					*tcp_handle;		// internal used
	void					*udp_handle;		// internal used
	int						port;				// 554
	rtsps_media_handle_t	media_handler;
} rtsps_context_t;


int rtsps_init(rtsps_context_t *ctx);
int rtsps_deinit();


#ifdef __cplusplus
}
#endif
#endif /* !_ringbuf_h_ */
