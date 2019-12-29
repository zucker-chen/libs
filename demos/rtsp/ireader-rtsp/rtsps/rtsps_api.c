
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

static void* tcp_handle = NULL;
static void* udp_handle = NULL;
static rtsps_context_t *rtsps_cxt = NULL;


extern int ip_route_get(const char* destination, char ip[40]);
extern int sockpair_create(const char* ip, socket_t pair[2], unsigned short port[2]);
extern uint32_t rtp_ssrc(void);

static int rtsps_get_media_sdp(char *channel_name, char *sdp /* out */, int sdp_maxsize)
{
	rtsps_stream_info_t strem_info;
	int sdp_size = 0, ret;

	if (rtsps_cxt == NULL || rtsps_cxt->media_handler.get_stream_info == NULL) {
		return -1;
	}

	ret = rtsps_cxt->media_handler.get_stream_info(channel_name, &strem_info);
	if (ret != 0) {
		//assert(0);
		return -1;
	}

	/// video
	if (strem_info.video_payload == RTP_PAYLOAD_H264) {			// H264, ref:sdp-h264.c
		const char* pattern =
		"m=video %hu RTP/AVP %d\n"
		"a=rtpmap:%d H264/90000\n"
		"a=fmtp:%d profile-level-id=245;packetization-mode=1;"; // sprop-parameter-sets= ...(sps pps)
		sdp_size += snprintf((char*)sdp, sdp_maxsize, pattern, 0, RTP_PAYLOAD_H264, RTP_PAYLOAD_H264, RTP_PAYLOAD_H264);
		sdp_size += snprintf((char*)sdp + sdp_size, sdp_maxsize - sdp_size, "a=control:track%d\n", 0);	// video
	} else if (strem_info.video_payload == RTP_PAYLOAD_H265) {		// H265, ref:sdp-h265.c
		const char* pattern =
		"m=video %hu RTP/AVP %d\n"
		"a=rtpmap:%d H265/90000\n"
		"a=fmtp:%d";
		sdp_size += snprintf((char*)sdp, sdp_maxsize, pattern, 0, RTP_PAYLOAD_H265, RTP_PAYLOAD_H265, RTP_PAYLOAD_H265);
		sdp_size += snprintf((char*)sdp + sdp_size, sdp_maxsize - sdp_size, "a=control:track%d\n", 0);	// video
	} else {
		return -1;
	}

	/// audio



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
		usleep(2000);
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

static void rtsps_rtp_paket(void* param, const void *packet, int bytes, uint32_t timestamp, int flags)
{
	rtsps_media_t *m = (rtsps_media_t *)param;
	rtsps_session_t *rss = (rtsps_session_t *)m->parent;
	rtsps_rtp_transport_t *transport = &rss->transport;
	int r, n;
	assert(m->packet == packet);

	// Hack: Send an initial RTCP "SR" packet, before the initial RTP packet, 
	// so that receivers will (likely) be able to get RTCP-synchronized presentation times immediately:
	rtp_onsend(m->rtp, packet, bytes/*, time*/);
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
	rtsps_stream_info_t strem_info;
	int ret;

	if (rtsps_cxt == NULL || rtsps_cxt->media_handler.get_stream_info == NULL) {
		return -1;
	}
	ret = rtsps_cxt->media_handler.get_stream_info(channel_name, &strem_info);
	if (ret != 0) {
		//assert(0);
		return -1;
	}

	// video
	m = &rss->media[0];
	m->parent = rss;
	m->track = 0;
	m->rtcp_clock = 0;
	m->ssrc = rtp_ssrc();
	m->timestamp = 0;//rtp_ssrc();
	m->bandwidth = strem_info.video_bitrate;	//4 * 1024 * 1024;
	m->dts_last = m->dts_first = -1;
	m->frequency = 90000;
	m->payload = strem_info.video_payload;	//RTP_PAYLOAD_H264;
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

	event.on_rtcp = rtsps_rtp_onrtcp;
	m->rtp = rtp_create(&event, NULL, m->ssrc, m->timestamp, m->frequency, m->bandwidth, 1);	// 1 = RTP_SENDER

	// audio ...

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
		//assert(0);
		return -1;
	}
	rss->rb_reader = rtsps_cxt->media_handler.add_rb_reader(channel_name);
	if (rss->rb_reader == NULL) {
		assert(rss->rb_reader);
		return -1;
	}
	snprintf(rss->session_code, sizeof(rss->session_code), "%p", &rss->transport);		// set a session code.
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

	if (rss->rb_reader != NULL) {
		rtsps_cxt->media_handler.del_rb_reader(rss->rb_reader);
	}
	list_remove(&rss->head);
	free(rss);
	rss = NULL;
	locker_unlock(&rtsps_cxt->locker);
	
	return 0;
}


static int rtsps_uri_parse(const char *uri, char *path)
{
	char path1[256];
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
	int i, ret;

	rtsps_uri_parse(uri, channel_name);

	if(session) {
    	list_for_each_safe(node, next, &rtsps_cxt->session_list) {
			rss = list_entry(node, rtsps_session_t, head);
			if (0 == strncmp(session, rss->session_code, strlen(session))) {
				break;
			}
    	}
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

		rss->transport.type = RTSP_TRANSPORT_RTP_TCP;
		rss->transport.rtsp = rtsp;
		rss->transport.send = rtsps_rtp_tcpsend;
		
		// RTP/AVP/TCP;interleaved=0-1
		snprintf(rtsp_transport, sizeof(rtsp_transport), "RTP/AVP/TCP;interleaved=%d-%d", interleaved[0], interleaved[1]);	
		
	} else if (RTSP_TRANSPORT_RTP_UDP == transport->transport) {
		printf("func = %s, line = %d: RTSP_TRANSPORT_RTP_UDP %p \n", __FUNCTION__, __LINE__, rtsp);
		assert(transport->rtp.u.client_port1 && transport->rtp.u.client_port2);
		
		unsigned short port[2] = { transport->rtp.u.client_port1, transport->rtp.u.client_port2 };
		const char *ip = transport->destination[0] ? transport->destination : rtsp_server_get_client(rtsp, NULL);
		
		rss->transport.type = RTSP_TRANSPORT_RTP_UDP;
		if(0 != (rtsps_udp_sockpair_create(&rss->transport, ip, port))) {
			// 500 Internal Server Error
			return rtsp_server_reply_setup(rtsp, 500, NULL, NULL);
		}
		
		rss->transport.type = RTSP_TRANSPORT_RTP_UDP;
		rss->transport.rtsp = rtsp;
		rss->transport.send = rtsps_rtp_udpsend;
		// RTP/AVP;unicast;client_port=4588-4589;server_port=6256-6257;destination=xxxx
		snprintf(rtsp_transport, sizeof(rtsp_transport), 
			"RTP/AVP;unicast;client_port=%hu-%hu;server_port=%hu-%hu%s%s", 
			transport->rtp.u.client_port1, transport->rtp.u.client_port2,
			port[0], port[1],
			transport->destination[0] ? ";destination=" : "",
			transport->destination[0] ? transport->destination : "");
	} else {
		// RFC 2326 1.6 Overall Operation p12
		// Multicast, client chooses address
		// Multicast, server chooses address
		assert(0);
		// 461 Unsupported Transport
		return rtsp_server_reply_setup(rtsp, 461, NULL, NULL);
	}

	rss->status = 0;
    return rtsp_server_reply_setup(rtsp, 200, rss->session_code, rtsp_transport);
}

static int rtsps_onplay(void* ptr, rtsp_server_t* rtsp, const char* uri, const char* session, const int64_t *npt, const double *scale)
{
	printf("func = %s, line = %d:  \n", __FUNCTION__, __LINE__);
	rtsps_session_t *rss = NULL;
    list_head_t *node, *next;

	if(session) {
    	list_for_each_safe(node, next, &rtsps_cxt->session_list) {
			rss = list_entry(node, rtsps_session_t, head);
			if (0 == strncmp(session, rss->session_code, strlen(session))) {
				break;
			}
    	}
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
		n += snprintf(rtpinfo + n, sizeof(rtpinfo) - n, "url=%s/track%d;seq=%hu;rtptime=%u", uri, m->track, seq, (unsigned int)(m->timestamp * (m->frequency / 1000) /*kHz*/));
	}

	rss->status = 1;
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
    	list_for_each_safe(node, next, &rtsps_cxt->session_list) {
			rss = list_entry(node, rtsps_session_t, head);
			if (0 == strncmp(session, rss->session_code, strlen(session))) {
				break;
			}
    	}
		if (rss == NULL || 0 != strncmp(session, rss->session_code, strlen(session))) {
			return rtsp_server_reply_setup(rtsp, 454, NULL, NULL);
		}
	} else {
		// 406 Not Acceptable
		return rtsp_server_reply_play(rtsp, 406, NULL, NULL, NULL);
	}

	rss->status = 4;
	usleep(500000);
	printf("func = %s, line = %d:  %p\n", __FUNCTION__, __LINE__, rss->transport.rtsp);
	rtsps_session_destroy(rss);
	
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
	printf("func = %s, line = %d:  \n", __FUNCTION__, __LINE__);
	//const char* require = rtsp_server_get_header(rtsp, "Require");
	return rtsp_server_reply_options(rtsp, 200);
}

static int rtsps_ongetparameter(void* ptr, rtsp_server_t* rtsp, const char* uri, const char* session, const void* content, int bytes)
{
	printf("func = %s, line = %d:  \n", __FUNCTION__, __LINE__);
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

#if 0
/* rtsp-server-tcp.c struct rtsp_session_t */
struct rtsp_session_t
{
	socket_t socket;
	void* aio;
	struct rtp_over_rtsp_t rtp;
	int rtsp_need_more_data;
	uint8_t buffer[4 * 1024];

	void *rtsp;
	struct sockaddr_storage addr;
	socklen_t addrlen;

	void (*onerror)(void* param, rtsp_server_t* rtsp, int code);
	void (*onrtp)(void* param, uint8_t channel, const void* data, uint16_t bytes);
	void* param;
};
#endif

static int rtsps_onclose(void* ptr2)
{
	printf("func = %s, line = %d:  \n", __FUNCTION__, __LINE__);
	// TODO: notify rtsp connection lost
	//       start a timer to check rtp/rtcp activity
	//       close rtsp media session on expired
	#if 0
	struct rtsp_session_t *rs = (struct rtsp_session_t *)ptr2;
	rtsps_session_t *rss = NULL;
    list_head_t *node, *next;

	list_for_each_safe(node, next, &rtsps_cxt->session_list) {
		rss = list_entry(node, rtsps_session_t, head);
		printf("func = %s, line = %d: 0x%p  0x%p \n", __FUNCTION__, __LINE__, rs->rtsp, rss->transport.rtsp);
		printf("func = %s, line = %d: 0x%x  0x%x \n", __FUNCTION__, __LINE__, (uint32_t)rs->rtsp, (uint32_t)rss->transport.rtsp);
		if ((uint32_t)rs->rtsp == (uint32_t)rss->transport.rtsp) {
			printf("func = %s, line = %d:  if\n", __FUNCTION__, __LINE__);
			break;
		} else {
			rss = NULL;
			printf("func = %s, line = %d:  else\n", __FUNCTION__, __LINE__);
		}
	}
	if (rss != NULL) {
		rss->status = 3;
		usleep(500000);
		printf("func = %s, line = %d:  \n", __FUNCTION__, __LINE__);
	}
	#endif

	
	printf("rtsp close\n");
	return 0;
}

static void rtsps_onerror(void* param, rtsp_server_t* rtsp, int code)
{
	printf("rtsp_onerror code=%d, rtsp=%p\n", code, rtsp);
	rtsps_session_t *rss = NULL;
    list_head_t *node, *next;

	list_for_each_safe(node, next, &rtsps_cxt->session_list) {
		rss = list_entry(node, rtsps_session_t, head);
		printf("func = %s, line = %d: %p  %p \n", __FUNCTION__, __LINE__, rtsp, rss->transport.rtsp);
		if ((uint32_t)rtsp == (uint32_t)rss->transport.rtsp) {
			printf("func = %s, line = %d:  if\n", __FUNCTION__, __LINE__);
			break;
		} else {
			rss = NULL;
			printf("func = %s, line = %d:  else\n", __FUNCTION__, __LINE__);
		}
	}
	if (rss != NULL) {
		rss->status = 3;
		usleep(500000);
		printf("func = %s, line = %d:  \n", __FUNCTION__, __LINE__);
	}
}



static int rtsps_play_proc(void* param)
{
	thread_detach(thread_self());

	rtsps_session_t *rss = NULL;
    list_head_t *node, *next;
	rtsps_frame_info_t *pkg;
	rtsps_media_t *m = NULL;
	uint32_t timestamp;
	int ret = -1;
	
	while (1)
	{
		locker_lock(&rtsps_cxt->locker);  // when session list remove
		list_for_each_safe(node, next, &rtsps_cxt->session_list) {
			rss = list_entry(node, rtsps_session_t, head);
			if (1 == rss->status) {
				if (rss->rb_reader == NULL || rtsps_cxt->media_handler.get_rb_stream == NULL || rtsps_cxt->media_handler.release_rb_stream == NULL) {
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
				} else if (pkg->stream_type == RTP_PAYLOAD_PCMU || pkg->stream_type == RTP_PAYLOAD_PCMA) {
					m = &rss->media[1];
				} else {
					assert(0);
					continue;
				}
				
				if (-1 == m->dts_first)
					m->dts_first = pkg->pts;
				m->dts_last = pkg->pts;
				timestamp = (uint32_t)m->timestamp + m->dts_last - m->dts_first;
				//printf("func = %s, line = %d: pkg->data_len = %d, pts = %lu, timestamp = %u\n", __FUNCTION__, __LINE__, pkg->data_len, pkg->pts, timestamp);
				rtp_payload_encode_input(m->packer, pkg->data, pkg->data_len, (uint32_t)timestamp);

				rtsps_cxt->media_handler.release_rb_stream(rss->rb_reader);
			} else if (3 == rss->status) {
				rss->status = 4;
				rtsps_session_destroy(rss);
				continue;
			} else {
				continue;
			}
		}
		locker_unlock(&rtsps_cxt->locker);

		usleep(1000);
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

	pthread_t pid;
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

	tcp_handle = rtsp_server_listen(NULL, port, &handler, NULL); assert(tcp_handle);
	udp_handle = rtsp_transport_udp_create(NULL, port, &handler.base, NULL); assert(udp_handle);

    LIST_INIT_HEAD(&rtsps_cxt->session_list);
	if(0 != locker_create(&rtsps_cxt->locker)) {
		return -1;
	}

	thread_create(&pid, rtsps_play_proc, NULL);
	
    return 0;
}


int rtsps_deinit()
{
	aio_worker_clean(N_AIO_THREAD);
	rtsp_server_unlisten(tcp_handle);
	rtsp_transport_udp_destroy(udp_handle);


	return 0;
}


