#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <signal.h>
#include <sys/time.h>

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
	strem_info->video_payload = 98;
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


#define RB_SIZE	(5*1024*1024)

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
	thd_running[ch] = 0;
	pthread_create(&tid[ch], NULL, get_stream_thdcb, &ch);
	usleep(200000);
	ch = 1;
	thd_running[ch] = 0;
	pthread_create(&tid[ch], NULL, get_stream_thdcb, &ch);
	sleep(1);

	ctx.port = 554;
	ctx.auth_enable = 0;
	ctx.media_handler.get_stream_info = get_stream_info;
	ctx.media_handler.add_rb_reader = add_rb_reader;
	ctx.media_handler.del_rb_reader = del_rb_reader;
	ctx.media_handler.get_rb_stream = get_rb_stream;
	ctx.media_handler.release_rb_stream = release_rb_stream;
	rtsps_init(&ctx);
	
	while (1) {sleep(10);}

	return 0;
}


