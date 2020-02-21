
#include "sys/sock.h"
#include "sys/locker.h"
#include "sys/system.h"
#include "aio-worker.h"
#include "ctypedef.h"
#include "ntp-time.h"
#include "rtp.h"
#include "rtp-over-rtsp.h"
#include "rtp-profile.h"
#include "rtp-payload.h"
#include "rtsp-server.h"
#include "rtsp-server-aio.h"
#include "uri-parse.h"
#include "urlcodec.h"
#include "thread.h"
#include "http-server.h"
#include "http-header-auth.h"
#include "rtsps_api.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>


#define N_AIO_THREAD 4

static rtsps_context_t *rtsps_cxt = NULL;


extern int ip_route_get(const char* destination, char ip[40]);
extern int sockpair_create(const char* ip, socket_t pair[2], unsigned short port[2]);
extern uint32_t rtp_ssrc(void);
extern int rtsp_server_reply2(struct rtsp_server_t *rtsp, int code, const char* header, const void* data, int bytes);


static int rtsps_authenticate(rtsp_server_t* rtsp, char *method)
{
	struct http_header_www_authenticate_t auth;
	const char *www_auth = NULL;
	char buffer[1024] = {0};
	char nonce[128] = {0};
	char *user = "admin";
	char *pwd = "admin";
	int n = 0;

	if (rtsps_cxt != NULL && rtsps_cxt->auth_enable == 0) {
		return 0;
	}

	if (NULL == rtsp) {
		assert(0);
		goto Unauthorized;
	}

	www_auth = rtsp_server_get_header(rtsp, "Authorization");
	if (NULL == www_auth) {
		printf("func = %s, line = %d:  \n", __FUNCTION__, __LINE__);
		//assert(0);
		goto Unauthorized;
	}
	
	memset(&auth, 0, sizeof(auth));
	//http_header_www_authenticate(www_auth, &auth);		// for client
	http_header_authorization(www_auth, &auth);			// for server
	//printf("func = %s, line = %d: username = %s response = %s\n", __FUNCTION__, __LINE__, auth.username, auth.response);
	strcpy(auth.username, user);
	snprintf(nonce, sizeof(nonce), "%p", rtsp);
	strcpy(auth.nonce, nonce);
	http_header_auth(&auth, pwd, method, NULL, 0, buffer, sizeof(buffer));
	printf("func = %s, line = %d: buffer = %s \n", __FUNCTION__, __LINE__, buffer);
	if (NULL == strstr(buffer, auth.response)) {
		goto Unauthorized;
	}
	printf("func = %s, line = %d: SUCESS! \n", __FUNCTION__, __LINE__);

	return 0;

Unauthorized:
	snprintf(nonce, sizeof(nonce), "%p", rtsp);
	n += snprintf(buffer, sizeof(buffer), "WWW-Authenticate: Digest "
		"realm=\"RTSP Server\", nonce=\"%s\"\r\n", nonce);
	rtsp_server_reply2(rtsp, 401, buffer, NULL, 0);
	printf("func = %s, line = %d: ERROR! \n", __FUNCTION__, __LINE__);

	return -1;
}

static int rtsps_get_media_sdp(char *channel_name, char *sdp /* out */, int sdp_maxsize)
{
	// m=<media><port><transport><format>
	rtsps_stream_info_t stream_info = {0};
	int sdp_size = 0, ret, port = 0;			// port: 0/96 is dynamic port

	if (rtsps_cxt == NULL || rtsps_cxt->media_handler.get_stream_info == NULL) {
		assert(0);
		return -1;
	}

	ret = rtsps_cxt->media_handler.get_stream_info(channel_name, &stream_info);
	if (ret != 0) {
		assert(0);
		return -1;
	}

	/// video
	if (stream_info.is_have_video != 0) {
		if (stream_info.video_payload == RTP_PAYLOAD_H264) {			// H264, ref:sdp-h264.c
			const char* pattern =
							"m=video %hu RTP/AVP %d\n"
							"a=rtpmap:%d H264/90000\n"
							"a=fmtp:%d profile-level-id=245;packetization-mode=1;"; // sprop-parameter-sets= ...(sps pps)
			sdp_size += snprintf((char*)sdp + sdp_size, sdp_maxsize - sdp_size, pattern, port, RTP_PAYLOAD_H264, RTP_PAYLOAD_H264, RTP_PAYLOAD_H264);
		} else if (stream_info.video_payload == RTP_PAYLOAD_H265) {		// H265, ref:sdp-h265.c
			const char* pattern =
							"m=video %hu RTP/AVP %d\n"
							"a=rtpmap:%d H265/90000\n"
							"a=fmtp:%d";
			sdp_size += snprintf((char*)sdp + sdp_size, sdp_maxsize - sdp_size, pattern, port, RTP_PAYLOAD_H265, RTP_PAYLOAD_H265, RTP_PAYLOAD_H265);
		} else {
			assert(0);
			return -1;
		}
		sdp_size += snprintf((char*)sdp + sdp_size, sdp_maxsize - sdp_size, "a=control:trackID=%d\n", 0);	// video
	}

	/// audio
	if (stream_info.is_have_audio != 0) {
		if (stream_info.audio_payload == RTP_PAYLOAD_PCMU) {			// g711a/u, ref:sdp-g7xx.c
			const char* pattern = 
							"m=audio %hu RTP/AVP %d\n"
							"a=rtpmap:%d PCMU/%d/%d\n";
			sdp_size += snprintf((char*)sdp + sdp_size, sdp_maxsize - sdp_size, pattern, 
							port, stream_info.audio_payload, stream_info.audio_payload, stream_info.audio_samplerate, stream_info.audio_chnum);
		} else if (stream_info.audio_payload == RTP_PAYLOAD_PCMA) {		// g711a/u, ref:sdp-g7xx.c
			const char* pattern = 
							"m=audio %hu RTP/AVP %d\n"
							"a=rtpmap:%d PCMA/%d/%d\n";
			sdp_size += snprintf((char*)sdp + sdp_size, sdp_maxsize - sdp_size, pattern, 
							port, stream_info.audio_payload, stream_info.audio_payload, stream_info.audio_samplerate, stream_info.audio_chnum);
		} else if (stream_info.audio_payload == RTP_PAYLOAD_MP4A) {		// aac, ref:sdp-aac.c -> sdp_aac_generic()
			static const char* pattern =
							"m=audio %hu RTP/AVP %d\n"
							"a=rtpmap:%d MPEG4-GENERIC/%d/%d\n"
							"a=fmtp:%d streamType=5;profile-level-id=1;mode=AAC-hbr;sizelength=13;indexlength=3;indexdeltalength=3;";
			sdp_size += snprintf((char*)sdp + sdp_size, sdp_maxsize - sdp_size, pattern, 
							port, RTP_PAYLOAD_MP4A, RTP_PAYLOAD_MP4A, stream_info.audio_samplerate, stream_info.audio_chnum, RTP_PAYLOAD_MP4A);
			if (stream_info.audio_extra_size > 0 && sdp_size + stream_info.audio_extra_size * 2 + 1 > sdp_maxsize) {
				// For MPEG-4 Audio streams, config is the audio object type specific
				// decoder configuration data AudioSpecificConfig()
				sdp_size += snprintf((char*)sdp + sdp_size, sdp_maxsize - sdp_size, "config=");
				sdp_size += base64_encode((char*)sdp + sdp_size, (void *)stream_info.audio_extra, stream_info.audio_extra_size);
			}
			sdp[sdp_size++] = '\n';
		} else {
			assert(0);
			return -1;
		}
		sdp_size += snprintf((char*)sdp + sdp_size, sdp_maxsize - sdp_size, "a=control:trackID=%d\n", 1);	// audio
	}

	return 0;
}


static int rtsps_udp_sockpair_create(rtsps_rtp_transport_t *rtp_s, const char* ip, unsigned short port[2])
{
	char local[SOCKET_ADDRLEN];
	int r1 = socket_addr_from(&rtp_s->udp_addr[0], &rtp_s->udp_addrlen[0], ip, port[0]);
	int r2 = socket_addr_from(&rtp_s->udp_addr[1], &rtp_s->udp_addrlen[1], ip, port[1]);
	
	if (0 != r1 || 0 != r2)
		return 0 != r1 ? r1 : r2;

	r1 = ip_route_get(ip, local);
	return sockpair_create(0==r1 ? local : NULL, rtp_s->udp_socket, port);
}

static int rtsps_udp_sockpair_destroy(rtsps_rtp_transport_t *rtp_s)
{
	int i, ret;
	
	for (i = 0; i < 2; i++)
	{
		if (rtp_s->udp_socket[i] > 0) {
			ret = socket_close(rtp_s->udp_socket[i]);
			rtp_s->udp_socket[i] = 0;
		}
	}

	return ret;
}

static int rtsps_rtp_udpsend(void* param, int rtcp, const void* data, int bytes)
{
	rtsps_rtp_transport_t *t = (rtsps_rtp_transport_t *)param;
	int i = rtcp ? 1 : 0;
	
	return socket_sendto(t->udp_socket[i], data, bytes, 0, (struct sockaddr*)&t->udp_addr[i], t->udp_addrlen[i]);
}

static int rtsps_rtp_tcpsend(void* param, int rtcp, const void* data, int bytes) 
{
	rtsps_rtp_transport_t *t = (rtsps_rtp_transport_t *)param;
	int times = 5;
	int r = 0;
	
	assert(bytes < (1 << 16));
	if (bytes >= (1 << 16))
		return -1;

	uint8_t packet[4 + (1 << 16)];
	packet[0] = '$';
	packet[1] = rtcp ? t->rtcp : t->rtp;
	packet[2] = (bytes >> 8) & 0xFF;
	packet[3] = bytes & 0xff;
	memcpy(packet + 4, data, bytes);
	do {
		assert(t->rtsp);
		r = rtsp_server_send_interleaved_data(t->rtsp, packet, bytes + 4);
		if (r != 0) {
			usleep(2000);
		}
	} while (times-- > 0 && r != 0);
	
	return 0 == r ? bytes : r;
}


static void *rtsps_rtp_alloc(void* param, int bytes)
{
	rtsps_media_t *m = (rtsps_media_t *)param;
	assert(bytes <= sizeof(m->packet));
	return m->packet;
}

static void rtsps_rtp_free(void* param, void *packet)
{
	rtsps_media_t *m = (rtsps_media_t *)param;
	assert(m->packet == packet);
}

// rtp paket send
static void rtsps_rtp_paket(void* param, const void *packet, int bytes, uint32_t timestamp, int flags)
{
	rtsps_media_t *m = (rtsps_media_t *)param;
	rtsps_session_t *rss = (rtsps_session_t *)m->parent;
	rtsps_rtp_transport_t *transport = NULL;
	int r, n;
	assert(m->packet == packet);

	// Hack: Send an initial RTCP "SR" packet, before the initial RTP packet, 
	// so that receivers will (likely) be able to get RTCP-synchronized presentation times immediately:
	rtp_onsend(m->rtp, packet, bytes/*, time*/);
	transport = &rss->media[m->track].transport;
	//printf("func = %s, line = %d: timestamp = %u \n", __FUNCTION__, __LINE__, timestamp);
	{
		// make sure have sent RTP packet
		int interval = rtp_rtcp_interval(m->rtp);
		uint64_t clock = system_clock();
		if (0 == m->rtcp_clock || m->rtcp_clock + interval < clock)
		{
			char rtcp[1024] = { 0 };
			n = rtp_rtcp_report(m->rtp, rtcp, sizeof(rtcp));

			// send RTCP packet
			assert(transport->send);
			if (transport->send) {
				r = transport->send(transport, 1, rtcp, n);
				if (r != (int)n) {
					//assert(0);
					printf("func = %s, line = %d: Warnning, socket send error \n", __FUNCTION__, __LINE__);
				}
			}
			m->rtcp_clock = clock;
		}
	}

	// rtp pkg send
	r = transport->send(transport, 0, packet, bytes);

	if (r != (int)bytes) {
		//assert(0);
		printf("func = %s, line = %d: Warnning, socket send error \n", __FUNCTION__, __LINE__);
	}
}

static void rtsps_rtp_onrtcp(void* param, const struct rtcp_msg_t* msg)
{
	return;
}

static int rtsps_rtp_init(rtsps_session_t *rss, char *channel_name)
{
	struct rtp_payload_t rtpfunc;
	struct rtp_event_t event;
	rtsps_media_t *m;
	rtsps_stream_info_t stream_info = {0};
	int ret;

	if (rtsps_cxt == NULL || rtsps_cxt->media_handler.get_stream_info == NULL) {
		return -1;
	}
	ret = rtsps_cxt->media_handler.get_stream_info(channel_name, &stream_info);
	if (ret != 0) {
		assert(0);
		return -1;
	}

	/// video
	if (stream_info.is_have_video != 0) {
		m = &rss->media[0];
		m->parent = rss;
		m->track = 0;
		m->rtcp_clock = 0;
		m->ssrc = rtp_ssrc();
		m->timestamp = 0;//rtp_ssrc();
		m->bandwidth = stream_info.video_bitrate;	// 4 * 1024 * 1024;
		m->dts_last = m->dts_first = -1;
		m->frequency = 90000;
		m->payload = stream_info.video_payload;		// RTP_PAYLOAD_H264;
		if (m->payload == RTP_PAYLOAD_H264) {
			snprintf(m->name, sizeof(m->name), "%s", "H264");
		} else if (m->payload == RTP_PAYLOAD_H265) {
			snprintf(m->name, sizeof(m->name), "%s", "H265");
		} else {
			assert(0);
			return -1;
		}
		
		rtpfunc.alloc = rtsps_rtp_alloc;
		rtpfunc.free = rtsps_rtp_free;
		rtpfunc.packet = rtsps_rtp_paket;
		m->packer = rtp_payload_encode_create(m->payload, m->name, (uint16_t)m->ssrc, m->ssrc, &rtpfunc, (void *)m);
		assert(m->packer != NULL);
		event.on_rtcp = rtsps_rtp_onrtcp;
		m->rtp = rtp_create(&event, NULL, m->ssrc, m->timestamp, m->frequency, m->bandwidth, 1);	// 1 = RTP_SENDER
		rtp_set_info(m->rtp, "RTSPServer", channel_name);
	}
	
	/// audio ...
	if (stream_info.is_have_audio != 0) {
		m = &rss->media[1];
		m->parent = rss;
		m->track = 1;
		m->rtcp_clock = 0;
		m->ssrc = rtp_ssrc();
		m->timestamp = 0;//rtp_ssrc();
		m->bandwidth = stream_info.audio_bitrate;	// 128 * 1024;
		m->dts_last = m->dts_first = -1;
		m->frequency = stream_info.audio_samplerate;
		m->payload = stream_info.audio_payload;		// RTP_PAYLOAD_PCMU;
		if (m->payload == RTP_PAYLOAD_PCMA) {
			snprintf(m->name, sizeof(m->name), "%s", "PCMA");	// PCMA	// ref: rtp-payload.c -> rtp_payload_find
		} else if (m->payload == RTP_PAYLOAD_PCMU) {
			snprintf(m->name, sizeof(m->name), "%s", "PCMU");	// PCMU
		} else if (m->payload == RTP_PAYLOAD_MP4A) {
			snprintf(m->name, sizeof(m->name), "%s", "MPEG4-GENERIC");
		} else {
			assert(0);
			return -1;
		}
		
		rtpfunc.alloc = rtsps_rtp_alloc;
		rtpfunc.free = rtsps_rtp_free;
		rtpfunc.packet = rtsps_rtp_paket;
		m->packer = rtp_payload_encode_create(m->payload, m->name, (uint16_t)m->ssrc, m->ssrc, &rtpfunc, (void *)m);
		assert(m->packer != NULL);
		event.on_rtcp = rtsps_rtp_onrtcp;
		m->rtp = rtp_create(&event, NULL, m->ssrc, m->timestamp, m->frequency, m->bandwidth, 1);	// 1 = RTP_SENDER
		rtp_set_info(m->rtp, "RTSPServer", channel_name);
	}

	return 0;
}


static int rtsps_session_create(rtsps_session_t *rss, char *channel_name)
{
	int ret;
	
	if (rss == NULL || channel_name == NULL) {
		assert(0);
		return -1;
	}

	ret = rtsps_rtp_init(rss, channel_name);
	if (ret != 0) {
		assert(0);
		return -1;
	}
	rss->rb_reader = rtsps_cxt->media_handler.add_rb_reader(channel_name);
	if (rss->rb_reader == NULL) {
		assert(rss->rb_reader);
		return -1;
	}
	snprintf(rss->session_code, sizeof(rss->session_code), "%p", &rss->session_code);		// set a session code.
	list_insert_after(&rss->head, &rtsps_cxt->session_list);
	
	return 0;
}

static int rtsps_session_destroy(rtsps_session_t *rss)
{
	if (rss == NULL) {
		assert(0);
		return -1;
	}

	locker_lock(&rtsps_cxt->locker);
	if (rss->media[0].rtp) {
		rtp_destroy(rss->media[0].rtp);
		rss->media[0].rtp = NULL;
	}
	if (rss->media[1].rtp) {
		rtp_destroy(rss->media[1].rtp);
		rss->media[1].rtp = NULL;
	}
	if (rss->media[0].packer) {
		rtp_payload_encode_destroy(rss->media[0].packer);
		rss->media[0].packer = NULL;
	}
	if (rss->media[1].packer) {
		rtp_payload_encode_destroy(rss->media[1].packer);
		rss->media[1].packer = NULL;
	}

	if (rss->rb_reader != NULL) {
		rtsps_cxt->media_handler.del_rb_reader(rss->rb_reader);
	}

	if (rss->media[0].transport.type == RTSP_TRANSPORT_RTP_UDP) {
		rtsps_udp_sockpair_destroy(&rss->media[0].transport);
	} else if (rss->media[1].transport.type == RTSP_TRANSPORT_RTP_UDP) {
		rtsps_udp_sockpair_destroy(&rss->media[1].transport);
	}
	
	list_remove(&rss->head);
	free(rss);
	rss = NULL;
	locker_unlock(&rtsps_cxt->locker);
	
	return 0;
}


// session paly thread
static int rtsps_play_proc(void* param)
{
	rtsps_session_t *rss = (rtsps_session_t *)param;
	rtsps_frame_info_t *pkg;
	rtsps_media_t *m = NULL;
	uint32_t timestamp;
	int ret = -1;
	
	usleep(200000);
	while (1)
	{
		if (RTSPS_SESSION_STATUS_PLAY == rss->status) {
			if (rss->rb_reader == NULL || rtsps_cxt->media_handler.get_rb_stream == NULL || rtsps_cxt->media_handler.release_rb_stream == NULL) {
				rtsps_session_destroy(rss);
				usleep(10000);
				continue;
			}
			
			ret = rtsps_cxt->media_handler.get_rb_stream(rss->rb_reader, &pkg);
			if (ret == -1) {
				usleep(10000);
				continue;
			}
			assert(ret == 0 && pkg != NULL);
			if (pkg->stream_type == RTP_PAYLOAD_H264 || pkg->stream_type == RTP_PAYLOAD_H265) {
				m = &rss->media[0];
			} else if (pkg->stream_type == RTP_PAYLOAD_PCMU || pkg->stream_type == RTP_PAYLOAD_PCMA || pkg->stream_type == RTP_PAYLOAD_MP4A) {
				m = &rss->media[1];
			} else {
				assert(0);
				usleep(10000);
				rtsps_cxt->media_handler.release_rb_stream(rss->rb_reader);
				continue;
			}
			
			if (-1 == m->dts_first) {
				m->dts_first = pkg->pts;
			}
			m->dts_last = pkg->pts;
			timestamp = (uint32_t)m->timestamp + m->dts_last - m->dts_first;	// ms
			//printf("func = %s, line = %d: pkg->data_len = %d, pts = %lu, timestamp = %u\n", __FUNCTION__, __LINE__, pkg->data_len, pkg->pts, timestamp);
			if (m->packer != NULL) {
				rtp_payload_encode_input(m->packer, pkg->data, pkg->data_len, (uint32_t)(timestamp * (m->frequency / 1000) /*kHz*/));
			}
			rtsps_cxt->media_handler.release_rb_stream(rss->rb_reader);
		} else if (RTSPS_SESSION_STATUS_ERROR == rss->status || RTSPS_SESSION_STATUS_TEARDOWN == rss->status) {
			//usleep(100000);
			//rtsps_session_destroy(rss);
			locker_lock(&rtsps_cxt->locker);
			rss->status = RTSPS_SESSION_STATUS_STOPED;
			locker_unlock(&rtsps_cxt->locker);
			break;
		} else {
			usleep(10000);
			continue;
		}

	}

	printf("func = %s, line = %d:  \n", __FUNCTION__, __LINE__);
}



static int rtsps_uri_parse(const char *uri, char *path)
{
	char path1[128];
	struct uri_t* r = uri_parse(uri, strlen(uri));
	if(!r) {
		return -1;
	}
	url_decode(r->path, strlen(r->path), path1, sizeof(path1));
	strncpy(path, path1, sizeof(path1));
	uri_free(r);
	
	return 0;
}

static int rtsps_ondescribe(void* ptr, rtsp_server_t* rtsp, const char* uri)
{
	printf("func = %s, line = %d:  \n", __FUNCTION__, __LINE__);
	char channel_name[128];
	char buffer[1024] = {0};
	int ret;

	static const char* pattern_vod =
		"v=0\n"
		"o=- %llu %llu IN IP4 %s\n"
		"s=%s\n"
		"c=IN IP4 0.0.0.0\n"
		"t=0 0\n"
		"a=range:npt=0-%.1f\n"
		"a=recvonly\n"
		"a=control:*\n"; // aggregate control

	static const char* pattern_live =
		"v=0\n"
		"o=- %llu %llu IN IP4 %s\n"
		"s=%s\n"
		"c=IN IP4 0.0.0.0\n"
		"t=0 0\n"
		"a=range:npt=now-\n" // live
		"a=recvonly\n"
		"a=control:*\n"; // aggregate control

	rtsps_uri_parse(uri, channel_name);

	ret = rtsps_authenticate(rtsp, "DESCRIBE");
	if (ret != 0) {
		//assert(0);
		return 0;
	}

	int offset = snprintf(buffer, sizeof(buffer), pattern_live, ntp64_now(), ntp64_now(), "0.0.0.0", uri);
	assert(offset > 0 && offset + 1 < sizeof(buffer));

	ret = rtsps_get_media_sdp(channel_name, (char *)(buffer + offset), sizeof(buffer) - offset);
	if (ret != 0) {
		//assert(0);
		return -1;
	}

    return rtsp_server_reply_describe(rtsp, 200, buffer);
}


static int rtsps_onsetup(void* ptr, rtsp_server_t* rtsp, const char* uri, const char* session, const struct rtsp_header_transport_t transports[], size_t num)
{
	printf("func = %s, line = %d:  \n", __FUNCTION__, __LINE__);
	char channel_name[128];
	rtsps_session_t *rss = NULL;
    list_head_t *node, *next;
	const struct rtsp_header_transport_t *transport = NULL;
	char rtsp_transport[128];
	const char *cseq_str;
	int cseq_num = 0;
	int trackID = 0;		// 0=video, 1=audio;	ref: sdp info
	int i, ret;

	rtsps_uri_parse(uri, channel_name);
	// find "trackID=0, and cut it."
	if (NULL != strstr(channel_name, "trackID")) {
		char* p = strrchr(channel_name, '/');
		if (NULL != p) {
			sscanf(p, "%*[^=]=%2d", &trackID);
			printf("func = %s, line = %d: trackID = %d \n", __FUNCTION__, __LINE__, trackID);
			p[0] = '\0';
		}
	}
	
	cseq_str = rtsp_server_get_header(rtsp, "CSeq");
	(cseq_str != NULL) && sscanf(cseq_str, "%4d", &cseq_num);
	printf("func = %s, line = %d: cseq_num = %d \n", __FUNCTION__, __LINE__, cseq_num);
	if (cseq_num < 2) {		// auth error, or others reasion
		// 454 Session Not Found
		return rtsp_server_reply_setup(rtsp, 454, NULL, NULL);
	}

	if(session) {
		locker_lock(&rtsps_cxt->locker);
    	list_for_each_safe(node, next, &rtsps_cxt->session_list) {
			rss = list_entry(node, rtsps_session_t, head);
			if (0 == strncmp(session, rss->session_code, strlen(session))) {
				break;
			}
    	}
		locker_unlock(&rtsps_cxt->locker);
		if (rss == NULL || 0 != strncmp(session, rss->session_code, strlen(session))) {
			// 454 Session Not Found
			return rtsp_server_reply_setup(rtsp, 454, NULL, NULL);
		}
	} else {
		rss = calloc(1, sizeof(rtsps_session_t));
		if (rss == NULL) {
			assert(0);
		} 
		ret = rtsps_session_create(rss, channel_name);
		if (ret != 0) {
			// 454 Session Not Found
			//assert(0);
			return rtsp_server_reply_setup(rtsp, 454, NULL, NULL);
		} 
	}

	for(i = 0; i < num && transport == NULL; i++)
	{
		if(RTSP_TRANSPORT_RTP_UDP == transports[i].transport) {
			// RTP/AVP/UDP
			transport = &transports[i];
		} else if(RTSP_TRANSPORT_RTP_TCP == transports[i].transport) {
			// RTP/AVP/TCP
			// 10.12 Embedded (Interleaved) Binary Data (p40)
			transport = &transports[i];
		}
	}
	if(!transport) {
		// 461 Unsupported Transport
        printf("func = %s, line = %d: : Unsupported Transport transport = NULL, trans = %d!\n", __FUNCTION__, __LINE__, transports[0].transport);
		return rtsp_server_reply_setup(rtsp, 461, NULL, NULL);
	}


	if (RTSP_TRANSPORT_RTP_TCP == transport->transport) {
		printf("func = %s, line = %d: RTSP_TRANSPORT_RTP_TCP %p \n", __FUNCTION__, __LINE__, rtsp);
		// 10.12 Embedded (Interleaved) Binary Data (p40)
		int interleaved[2];
		if (transport->interleaved1 == transport->interleaved2) {
			interleaved[0] = 0;
			interleaved[1] = 1;
		} else {
			interleaved[0] = transport->interleaved1;
			interleaved[1] = transport->interleaved2;
		}

		rss->media[trackID].transport.type = RTSP_TRANSPORT_RTP_TCP;
		rss->media[trackID].transport.rtsp = rtsp;
		rss->media[trackID].transport.rtp = interleaved[0];
		rss->media[trackID].transport.rtcp = interleaved[1];
		rss->media[trackID].transport.send = rtsps_rtp_tcpsend;
		
		// RTP/AVP/TCP;interleaved=0-1
		snprintf(rtsp_transport, sizeof(rtsp_transport), 
			"RTP/AVP/TCP;interleaved=%d-%d;ssrc=%d", 
			interleaved[0], interleaved[1], rss->media[trackID].ssrc);	
		
	} else if (RTSP_TRANSPORT_RTP_UDP == transport->transport) {
		printf("func = %s, line = %d: RTSP_TRANSPORT_RTP_UDP %p \n", __FUNCTION__, __LINE__, rtsp);
		assert(transport->rtp.u.client_port1 && transport->rtp.u.client_port2);
		
		unsigned short port[2] = { transport->rtp.u.client_port1, transport->rtp.u.client_port2 };
		const char *ip = transport->destination[0] ? transport->destination : rtsp_server_get_client(rtsp, NULL);
		
		rss->media[trackID].transport.type = RTSP_TRANSPORT_RTP_UDP;
		if(0 != (rtsps_udp_sockpair_create(&rss->media[trackID].transport, ip, port))) {
			// 500 Internal Server Error
			return rtsp_server_reply_setup(rtsp, 500, NULL, NULL);
		}
		
		rss->media[trackID].transport.type = RTSP_TRANSPORT_RTP_UDP;
		rss->media[trackID].transport.rtsp = rtsp;
		rss->media[trackID].transport.send = rtsps_rtp_udpsend;
		// RTP/AVP;unicast;client_port=4588-4589;server_port=6256-6257;destination=xxxx
		snprintf(rtsp_transport, sizeof(rtsp_transport), 
			"RTP/AVP;unicast;client_port=%hu-%hu;server_port=%hu-%hu%s%s;ssrc=%d", 
			transport->rtp.u.client_port1, transport->rtp.u.client_port2,
			port[0], port[1],
			transport->destination[0] ? ";destination=" : "",
			transport->destination[0] ? transport->destination : "",
			rss->media[trackID].ssrc);
	} else {
		// RFC 2326 1.6 Overall Operation p12
		// Multicast, client chooses address
		// Multicast, server chooses address
		assert(0);
		// 461 Unsupported Transport
		return rtsp_server_reply_setup(rtsp, 461, NULL, NULL);
	}

	// set tcp socket timeout, default: recv = 20s, send = 10s
	/* ref: rtsp-server-tcp.c struct rtsp_session_t */
	void *sendparam = *(void **)((char *)rss->media[trackID].transport.rtsp + sizeof(struct rtsp_handler_t) + sizeof(void *));			// rtsp->sendparam
	void *aoi_tansport = *(void **)((char *)sendparam + sizeof(long));	/*sizeof(socket_t) = 4, align at sizeof(long)*/	// rtsp_session_t->aio
	aio_tcp_transport_set_timeout(aoi_tansport, (4 * 60 * 1000), (2 * 60 * 1000));										// recv=4min, send=2min

	rss->status = RTSPS_SESSION_STATUS_SETUP;
    return rtsp_server_reply_setup(rtsp, 200, rss->session_code, rtsp_transport);
}

static int rtsps_onplay(void* ptr, rtsp_server_t* rtsp, const char* uri, const char* session, const int64_t *npt, const double *scale)
{
	printf("func = %s, line = %d:  \n", __FUNCTION__, __LINE__);
	rtsps_session_t *rss = NULL;
    list_head_t *node, *next;
	pthread_t t;

	if(session) {
		locker_lock(&rtsps_cxt->locker);
    	list_for_each_safe(node, next, &rtsps_cxt->session_list) {
			rss = list_entry(node, rtsps_session_t, head);
			if (0 == strncmp(session, rss->session_code, strlen(session))) {
				break;
			}
    	}
		locker_unlock(&rtsps_cxt->locker);
		if (rss == NULL || 0 != strncmp(session, rss->session_code, strlen(session))) {
			return rtsp_server_reply_setup(rtsp, 454, NULL, NULL);
		}
	} else {
		// 406 Not Acceptable
		return rtsp_server_reply_play(rtsp, 406, NULL, NULL, NULL);
	}

	// RFC 2326 12.33 RTP-Info (p55)
	// 1. Indicates the RTP timestamp corresponding to the time value in the Range response header.
	// 2. A mapping from RTP timestamps to NTP timestamps (wall clock) is available via RTCP.
	char rtpinfo[512] = { 0 };
	int n = 0, i = 0;
	uint16_t seq;
	uint32_t timestamp;

	// RTP-Info: url=rtsp://foo.com/bar.avi/streamid=0;seq=45102,
	//			 url=rtsp://foo.com/bar.avi/streamid=1;seq=30211
	for (i = 0; i < 2; i++)
	{
		rtsps_media_t *m = &rss->media[i];
		if (m->packer == NULL) {
			continue;
		}
		
		rtp_payload_encode_getinfo(m->packer, &seq, &timestamp);
		if (i > 0) {
			rtpinfo[n++] = ',';
		}
		n += snprintf(rtpinfo + n, sizeof(rtpinfo) - n, "url=%s/trackID=%d;seq=%hu;rtptime=%u", uri, m->track, seq, (unsigned int)(m->timestamp * (m->frequency / 1000) /*kHz*/));
	}

	thread_create(&t, rtsps_play_proc, (void *)rss);
	thread_detach(t);
	
	rss->status = RTSPS_SESSION_STATUS_PLAY;
    return rtsp_server_reply_play(rtsp, 200, npt, NULL, rtpinfo);
}

static int rtsps_onpause(void* ptr, rtsp_server_t* rtsp, const char* uri, const char* session, const int64_t* npt)
{
	printf("func = %s, line = %d:  \n", __FUNCTION__, __LINE__);
    return rtsp_server_reply_pause(rtsp, 200);
}

static int rtsps_onteardown(void* ptr, rtsp_server_t* rtsp, const char* uri, const char* session)
{
	printf("func = %s, line = %d:  \n", __FUNCTION__, __LINE__);
	rtsps_session_t *rss = NULL;
    list_head_t *node, *next;

	if(session) {
		locker_lock(&rtsps_cxt->locker);
    	list_for_each_safe(node, next, &rtsps_cxt->session_list) {
			rss = list_entry(node, rtsps_session_t, head);
			if (0 == strncmp(session, rss->session_code, strlen(session))) {
				break;
			}
    	}
		locker_unlock(&rtsps_cxt->locker);
		if (rss == NULL || 0 != strncmp(session, rss->session_code, strlen(session))) {
			return rtsp_server_reply_setup(rtsp, 454, NULL, NULL);
		}
	} else {
		// 406 Not Acceptable
		return rtsp_server_reply_play(rtsp, 406, NULL, NULL, NULL);
	}

	rss->status = RTSPS_SESSION_STATUS_TEARDOWN;	// will auto trigger onerror --> onclose after
	//usleep(100000);
	printf("func = %s, line = %d: %s\n", __FUNCTION__, __LINE__, rss->session_code);
	
	return rtsp_server_reply_teardown(rtsp, 200);
}

static int rtsps_onannounce(void* ptr, rtsp_server_t* rtsp, const char* uri, const char* sdp)
{
	printf("func = %s, line = %d:  \n", __FUNCTION__, __LINE__);
    return rtsp_server_reply_announce(rtsp, 200);
}

static int rtsps_onrecord(void* ptr, rtsp_server_t* rtsp, const char* uri, const char* session, const int64_t *npt, const double *scale)
{
	printf("func = %s, line = %d:  \n", __FUNCTION__, __LINE__);
    return rtsp_server_reply_record(rtsp, 200, NULL, NULL);
}

static int rtsps_onoptions(void* ptr, rtsp_server_t* rtsp, const char* uri)
{
	int ret;
	
	printf("func = %s, line = %d:  \n", __FUNCTION__, __LINE__);
	//const char* require = rtsp_server_get_header(rtsp, "Require");

	ret = rtsps_authenticate(rtsp, "OPTIONS");
	if (ret != 0) {
		//assert(0);
		return 0;
	}
	
	return rtsp_server_reply_options(rtsp, 200);
}

static int rtsps_ongetparameter(void* ptr, rtsp_server_t* rtsp, const char* uri, const char* session, const void* content, int bytes)
{
	printf("func = %s, line = %d: rtsp = %p \n", __FUNCTION__, __LINE__, rtsp);
	//const char* ctype = rtsp_server_get_header(rtsp, "Content-Type");
	//const char* encoding = rtsp_server_get_header(rtsp, "Content-Encoding");
	//const char* language = rtsp_server_get_header(rtsp, "Content-Language");
	return rtsp_server_reply_get_parameter(rtsp, 200, NULL, 0);
}

static int rtsps_onsetparameter(void* ptr, rtsp_server_t* rtsp, const char* uri, const char* session, const void* content, int bytes)
{
	printf("func = %s, line = %d:  \n", __FUNCTION__, __LINE__);
	//const char* ctype = rtsp_server_get_header(rtsp, "Content-Type");
	//const char* encoding = rtsp_server_get_header(rtsp, "Content-Encoding");
	//const char* language = rtsp_server_get_header(rtsp, "Content-Language");
	return rtsp_server_reply_set_parameter(rtsp, 200);
}


// run onclose after onerror.
static int rtsps_onclose(void* ptr2)
{
	printf("func = %s, line = %d:  \n", __FUNCTION__, __LINE__);
	// TODO: notify rtsp connection lost
	//       start a timer to check rtp/rtcp activity
	//       close rtsp media session on expired
	
	printf("rtsp close\n");
	return 0;
}

// code = errno (socket-epoll recv/send errno)
static void rtsps_onerror(void* param, rtsp_server_t* rtsp, int code)
{
	printf("rtsp_onerror code=%d, rtsp=%p\n", code, rtsp);
	rtsps_session_t *rss = NULL;
    list_head_t *node, *next;
	int count = 0;

	locker_lock(&rtsps_cxt->locker);
	list_for_each_safe(node, next, &rtsps_cxt->session_list) {
		rss = list_entry(node, rtsps_session_t, head);
		printf("func = %s, line = %d: %p  %s \n", __FUNCTION__, __LINE__, rtsp, rss->session_code);
		if ((uint32_t)rtsp == (uint32_t)rss->media[0].transport.rtsp) {
			printf("func = %s, line = %d:  find it\n", __FUNCTION__, __LINE__);
			break;
		} else {
			rss = NULL;
			printf("func = %s, line = %d:  not find\n", __FUNCTION__, __LINE__);
		}
	}
	locker_unlock(&rtsps_cxt->locker);
	

	#if 0	// Not a good method, instead by aio_tcp_transport_set_timeout()
	if (code == 110 && rss != NULL && rss->transport.type == RTSP_TRANSPORT_RTP_UDP) {
		printf("func = %s, line = %d:  IS RTP/UDP CMD TIMEOUT!\n", __FUNCTION__, __LINE__);
		return;
	}
	#endif
	
	if (rss != NULL) {
		locker_lock(&rtsps_cxt->locker);
		rss->status = rss->status != RTSPS_SESSION_STATUS_STOPED ? RTSPS_SESSION_STATUS_ERROR : RTSPS_SESSION_STATUS_STOPED;
		locker_unlock(&rtsps_cxt->locker);
		while (rss->status != RTSPS_SESSION_STATUS_STOPED && count++ < 100)
		{
			usleep(100000);
		}
	
		rtsps_session_destroy(rss);
		printf("func = %s, line = %d: count = %d \n", __FUNCTION__, __LINE__, count);
	}
}


/* input:	port,		554(defaults)
 * func: rtsp server init
 */
int rtsps_init(rtsps_context_t *ctx)
{
	if (ctx == NULL) {
		return -1;
	}

	struct aio_rtsp_handler_t handler;
	int port = ctx->port == 0 ? 554 : ctx->port;

	rtsps_cxt = ctx;
	aio_worker_init(N_AIO_THREAD);
	memset(&handler, 0, sizeof(handler));
	handler.base.ondescribe = rtsps_ondescribe;
	handler.base.onsetup = rtsps_onsetup;
	handler.base.onplay = rtsps_onplay;
	handler.base.onpause = rtsps_onpause;
	handler.base.onteardown = rtsps_onteardown;
	handler.base.close = rtsps_onclose;
	handler.base.onannounce = rtsps_onannounce;
	handler.base.onrecord = rtsps_onrecord;
	handler.base.onoptions = rtsps_onoptions;
	handler.base.ongetparameter = rtsps_ongetparameter;
	handler.base.onsetparameter = rtsps_onsetparameter;
	//handler.base.send; // ignore
	handler.onerror = rtsps_onerror;

	rtsps_cxt->tcp_handle = rtsp_server_listen(NULL, port, &handler, NULL); assert(rtsps_cxt->tcp_handle);
	//rtsps_cxt->udp_handle = rtsp_transport_udp_create(NULL, port, &handler.base, NULL); assert(rtsps_cxt->udp_handle);

    LIST_INIT_HEAD(&rtsps_cxt->session_list);
	if(0 != locker_create(&rtsps_cxt->locker)) {
		return -1;
	}
	
    return 0;
}


int rtsps_deinit()
{
	rtsps_session_t *rss = NULL;
    list_head_t *node, *next;
	int count = 0;
	
	locker_lock(&rtsps_cxt->locker);
	list_for_each_safe(node, next, &rtsps_cxt->session_list) {
		rss = list_entry(node, rtsps_session_t, head);
		rss->status = rss->status != RTSPS_SESSION_STATUS_STOPED ? RTSPS_SESSION_STATUS_ERROR : RTSPS_SESSION_STATUS_STOPED;		
		while (rss->status != 5 && count++ < 100)
		{
			usleep(100000);
		}
		
		rtsps_session_destroy(rss);
	}
	locker_unlock(&rtsps_cxt->locker);


	if (rtsps_cxt->tcp_handle) {
		rtsp_server_unlisten(rtsps_cxt->tcp_handle);
	}
	//rtsp_transport_udp_destroy(rtsps_cxt->udp_handle);
	locker_destroy(&rtsps_cxt->locker);
	aio_worker_clean(N_AIO_THREAD);

	return 0;
}


