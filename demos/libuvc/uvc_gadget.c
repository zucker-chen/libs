

#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>

#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

#include "usb_video.h"
#include "uvc_gadget.h"
#include "uvc_video.h"






/* ---------------------------------------------------------------------------
 * Request processing
 */

struct uvc_frame_info
{
    unsigned int width;
    unsigned int height;
    unsigned int intervals[8];
};

struct uvc_format_info
{
    unsigned int                 fcc;
    const struct uvc_frame_info* frames;
};

static const struct uvc_frame_info uvc_frames_yuyv[] =
{
    {  640,  360, {333333,       0 }, },
    { 1280,  720, {333333,		0  }, },
    { 1920, 1080, {333333,       0 }, },
    { 3840, 2160, {333333,       0 }, },
    {    0,    0, {		0,         }, },
};

static const struct uvc_frame_info uvc_frames_mjpeg[] =
{
    {  640,  360, {333333,       0 }, },
    { 1280,  720, {333333,		0  }, },
    { 1920, 1080, {333333,       0 }, },
    { 3840, 2160, {333333,       0 }, },
    {    0,    0, {		0,         }, },
};

static const struct uvc_frame_info uvc_frames_h264[] =
{
    {  640,  360, {333333,       0 }, },
    { 1280,  720, {333333,		0  }, },
    { 1920, 1080, {333333,       0 }, },
    { 3840, 2160, {333333,       0 }, },
    {    0,    0, {		0,         }, },
};

static const struct uvc_frame_info uvc_frames_h265[] =
{
    {  640,  360, {333333,       0 }, },
    { 1280,  720, {333333,		0  }, },
    { 1920, 1080, {333333,       0 }, },
    { 3840, 2160, {333333,       0 }, },
    {    0,    0, {		0,         }, },
};

static const struct uvc_format_info uvc_formats[] =
{ //yuv first
    { V4L2_PIX_FMT_YUYV,  uvc_frames_yuyv  },
    { V4L2_PIX_FMT_MJPEG, uvc_frames_mjpeg },
    { V4L2_PIX_FMT_H264,  uvc_frames_h264  },
    { V4L2_PIX_FMT_H265,  uvc_frames_h265  },
};


static void uvc_events_process(struct uvc_device* dev)
{
    struct v4l2_event v4l2_event;
    struct uvc_event* uvc_event = (struct uvc_event*)(void*)&v4l2_event.u.data;
    struct uvc_request_data resp;
    int ret;

	printf("func = %s, line = %d\n", __FUNCTION__, __LINE__);
    ret = ioctl(dev->fd, VIDIOC_DQEVENT, &v4l2_event);
    if (ret < 0)
    {
        printf("VIDIOC_DQEVENT failed: %s (%d)\n", strerror(errno), errno);
        return;
    }

    memset(&resp, 0, sizeof resp);
    resp.length = -EL2HLT;

    switch (v4l2_event.type)
    {
			//0x08000000
		case UVC_EVENT_CONNECT:
			printf("handle connect event \n");
			return;
			//0x08000001
		case UVC_EVENT_DISCONNECT:
			printf("handle disconnect event\n");
			return;

			//0x08000004   UVC class
		case UVC_EVENT_SETUP:
			 printf("handle setup event\n");
			//uvc_events_process_setup(dev, &uvc_event->req, &resp);
			break;

			//0x08000005
		case UVC_EVENT_DATA:
			 printf("handle data event\n");
			//uvc_events_process_data(dev, &uvc_event->data);
			return;

			//0x08000002
		case UVC_EVENT_STREAMON:
			printf("UVC_EVENT_STREAMON\n");
			//enable_uvc_video(dev);
			return;

			//0x08000003
		case UVC_EVENT_STREAMOFF:
			printf("UVC_EVENT_STREAMOFF\n");
			//disable_uvc_video(dev);

        return;
    }

    ret = ioctl(dev->fd, UVCIOC_SEND_RESPONSE, &resp);
    if (ret < 0)
    {
        printf("UVCIOC_S_EVENT failed: %s (%d)\n", strerror(errno), errno);
        return;
    }
}

static void uvc_fill_streaming_control(struct uvc_device* dev,
                                       struct uvc_streaming_control* ctrl,
                                       int iframe, int iformat)
{
    const struct uvc_format_info* format;
    const struct uvc_frame_info* frame;
    unsigned int nframes;

    if (iformat < 0)
    {
        iformat = ARRAY_SIZE(uvc_formats) + iformat;
    }

    if ((iformat < 0) || (iformat >= (int)ARRAY_SIZE(uvc_formats)))
    {
        return;
    }

   // INFO("iformat = %d\n", iformat);
    format = &uvc_formats[iformat];

    nframes = 0;

    while (format->frames[nframes].width != 0)
    {
        ++nframes;
    }

    if (iframe < 0)
    {
        iframe = nframes + iframe;
    }

    if ((iframe < 0) || (iframe >= (int)nframes))
    {
        return;
    }

    frame = &format->frames[iframe];

    memset(ctrl, 0, sizeof * ctrl);

    ctrl->bmHint = 1;
    ctrl->bFormatIndex = iformat + 1; // 1 is yuv ,2 is mjpeg
    ctrl->bFrameIndex = iframe + 1; //360 1 720 2
    ctrl->dwFrameInterval = frame->intervals[0]; //dui ying di ji ge zhenlv

    switch (format->fcc)
    {
    case V4L2_PIX_FMT_YUYV:
    case V4L2_PIX_FMT_YUV420:
        ctrl->dwMaxVideoFrameSize = frame->width * frame->height * 2;
        break;

    case V4L2_PIX_FMT_MJPEG:
    case V4L2_PIX_FMT_H264:
    case V4L2_PIX_FMT_H265:
        ctrl->dwMaxVideoFrameSize = frame->width * frame->height * 2;
        break;
    }

	ctrl->dwMaxPayloadTransferSize = 3072;
    ctrl->bmFramingInfo = 3;
    ctrl->bPreferedVersion = 1;
    ctrl->bMaxVersion = 1;

}

static void uvc_events_init(struct uvc_device* dev)
{
    struct v4l2_event_subscription sub;

    uvc_fill_streaming_control(dev, &dev->probe, 0, 0);
    uvc_fill_streaming_control(dev, &dev->commit, 0, 0);

    memset(&sub, 0, sizeof sub);
    sub.type = UVC_EVENT_SETUP;
    ioctl(dev->fd, VIDIOC_SUBSCRIBE_EVENT, &sub);
    sub.type = UVC_EVENT_DATA;
    ioctl(dev->fd, VIDIOC_SUBSCRIBE_EVENT, &sub);
    sub.type = UVC_EVENT_STREAMON;
    ioctl(dev->fd, VIDIOC_SUBSCRIBE_EVENT, &sub);
    sub.type = UVC_EVENT_STREAMOFF;
    ioctl(dev->fd, VIDIOC_SUBSCRIBE_EVENT, &sub);
}


/* request buffers */
static int uvc_reqbufs(struct uvc_device* dev)
{
    struct v4l2_requestbuffers rb;
    int ret;

    memset(&rb, 0, sizeof(struct v4l2_requestbuffers));
    rb.count = dev->nbufs;
    rb.type   = dev->type;
    rb.memory = (dev->type == V4L2_BUF_TYPE_VIDEO_CAPTURE) ? V4L2_MEMORY_MMAP : V4L2_MEMORY_USERPTR;
    ret = ioctl(dev->fd, VIDIOC_REQBUFS, &rb);
    if (ret < 0) {
        printf("Unable to allocate buffers: %s (%d).\n", strerror(errno), errno);
        return ret;
    }

    dev->nbufs = rb.count;
    printf("%u buffers allocated.\n", rb.count);

    return 0;
}

/* map the buffers */
static int uvc_mmapbuf(struct uvc_device* dev)
{
    struct v4l2_buffer buf;
    int ret, i;

    for (i = 0; i < dev->nbufs; i++) 
	{
        memset (&buf, 0, sizeof(struct v4l2_buffer));
        buf.index = i;
        buf.type = dev->type;
        buf.memory = V4L2_MEMORY_MMAP;
        ret = ioctl (dev->fd, VIDIOC_QUERYBUF, &buf);
        if (ret < 0) {
            printf("Unable to query buffer (%d).\n", errno);
            return -1;
        }
        
        printf("length: %u offset: %u\n", buf.length, buf.m.offset);
        dev->mem[i] = mmap (0, buf.length, PROT_READ, MAP_SHARED, dev->fd, buf.m.offset);
        if (dev->mem[i] == MAP_FAILED) {
            printf("Unable to map buffer (%d)\n", errno);
            return -1;
        }
        dev->length[i] = dev->width * dev->height * 2;
        printf("Buffer mapped at address %p.\n", dev->mem[i]);
    }

    return 0;
}


/* Queue the buffers. */
static int uvc_qbuf(struct uvc_device* dev)
{
    struct v4l2_buffer buf;
    int ret, i;

    for (i = 0; i < dev->nbufs; i++)
	{
        memset (&buf, 0, sizeof(struct v4l2_buffer));
        buf.index = i;
        buf.type = dev->type;
        buf.memory = (dev->type == V4L2_BUF_TYPE_VIDEO_CAPTURE) ? V4L2_MEMORY_MMAP : V4L2_MEMORY_USERPTR;
		
		if (dev->type == V4L2_BUF_TYPE_VIDEO_OUTPUT) {
			uvc_video_fill_buffer_userptr(dev, &buf);
		}
		printf("func = %s, line = %d: %d, %d, %d, %d, %p, %d %d\n", __FUNCTION__, __LINE__, dev->type, V4L2_BUF_TYPE_VIDEO_OUTPUT, buf.index, buf.memory, buf.m.userptr, buf.bytesused, buf.length);
		buf.length = dev->width * dev->height * 2;
        ret = ioctl (dev->fd, VIDIOC_QBUF, &buf);
        if (ret < 0) {
            printf("Unable to Queue buffer (%d).\n", errno);
            return -1;
        }
		printf("func = %s, line = %d: %d, %d, %d, %d, %p, %d %d\n", __FUNCTION__, __LINE__, dev->type, V4L2_BUF_TYPE_VIDEO_OUTPUT, buf.index, buf.memory, buf.m.userptr, buf.bytesused, buf.length);
        dev->length[i] = buf.length;
        printf("Buffer mapped at address %p, length = %d\n", dev->mem[i], dev->length[i]);
    }

    return 0;
}


static int uvc_set_format(struct uvc_device* dev)
{
    struct v4l2_format fmt;
    int ret;

    memset(&fmt, 0, sizeof(struct v4l2_format));
    fmt.type = dev->type;
    fmt.fmt.pix.width  = dev->width;
    fmt.fmt.pix.height = dev->height;
    fmt.fmt.pix.pixelformat = dev->pix_fmt;
    fmt.fmt.pix.field = V4L2_FIELD_NONE;    // V4L2_FIELD_NONE/V4L2_FIELD_ANY

    if ((dev->pix_fmt == V4L2_PIX_FMT_MJPEG) || (dev->pix_fmt == V4L2_PIX_FMT_H264) || (dev->pix_fmt == V4L2_PIX_FMT_H265)) {
        fmt.fmt.pix.sizeimage = dev->width * dev->height * 2;
    }

    if ((ret = ioctl(dev->fd, VIDIOC_S_FMT, &fmt)) < 0) {
        printf("Unable to set format: %s (%d).\n", strerror(errno), errno);
        return -1;
    }

    return ret;
}

/* get format info, output-> fmt */
static int uvc_get_format(struct uvc_device* dev, struct v4l2_format *fmt)
{
    int ret;

    memset(fmt, 0, sizeof(struct v4l2_format));
    fmt->type = dev->type;
    // VIDIOC_G_FMT need assign the fmt.type.
    if ((ret = ioctl(dev->fd, VIDIOC_G_FMT, fmt)) < 0) {
        printf("Unable to get format: %s (%d).\n", strerror(errno), errno);
        return -1;
    }

    return ret;
}

static int uvc_open_device(const char* devname)
{
    struct v4l2_capability cap;
    int ret;
    int fd;

    fd = open(devname, O_RDWR | O_NONBLOCK);
    if (fd == -1) {
        printf("v4l2 open failed(%s): %s (%d)\n",devname, strerror(errno), errno);
        return -1;
    }

    ret = ioctl(fd, VIDIOC_QUERYCAP, &cap);
    if (ret < 0) {
        printf("unable to query device: %s (%d)\n", strerror(errno), errno);
        close(fd);
        return -1;
    }

    printf("open succeeded(%s:caps=0x%04x)\n", devname, cap.capabilities);

    /*
    V4L2_CAP_VIDEO_CAPTURE 0x00000001 support video Capture interface.
    V4L2_CAP_VIDEO_OUTPUT 0x00000002 support video output interface.
    V4L2_CAP_VIDEO_OVERLAY 0x00000004 support video cover interface.
    V4L2_CAP_VBI_CAPTURE 0x00000010 Original VBI Capture interface.
    V4L2_CAP_VBI_OUTPUT 0x00000020 Original VBI Output interface.
    V4L2_CAP_SLICED_VBI_CAPTURE 0x00000040 Sliced VBI Capture interface.
    V4L2_CAP_SLICED_VBI_OUTPUT 0x00000080 Sliced VBI Output interface.
    V4L2_CAP_RDS_CAPTURE 0x00000100 undefined
    */
    if (!(cap.capabilities & 0x00000001) && !(cap.capabilities & 0x00000002)) {
        close(fd);
        return -1;
    }

    return fd;
}

int uvc_stream_get(struct uvc_device *dev, struct uvc_stream_attr *stream)
{
    int ret;
    
    if (!dev->streaming) {
        printf("error: stream is off.\n");
        return -1;
    }

    memset (&dev->buf, 0, sizeof (struct v4l2_buffer));
    dev->buf.type   = dev->type;
    dev->buf.memory = (dev->type == V4L2_BUF_TYPE_VIDEO_CAPTURE) ? V4L2_MEMORY_MMAP : V4L2_MEMORY_USERPTR;
    ret = ioctl (dev->fd, VIDIOC_DQBUF, &dev->buf);
    if (ret < 0) {
        printf("unable to dequeue device: %s (%d)\n", strerror(errno), errno);
        return -1;
    }
    
    stream->pix_fmt = dev->pix_fmt;
    stream->width = dev->width;
    stream->height = dev->height;
    stream->data = dev->mem[dev->buf.index];
    stream->length = dev->buf.bytesused;

    return 0;
}

int uvc_stream_release(struct uvc_device *dev)
{
    int ret;
    
    ret = ioctl (dev->fd, VIDIOC_QBUF, &dev->buf);
    if (ret < 0) {
        printf("Unable to requeue buffer (%d).\n", errno);
        return -1;
    }
    
    return 0;
}


static int uvc_data_process_userptr(struct uvc_device *dev)
{
    struct v4l2_buffer buf;
    int ret;

    memset(&buf, 0, sizeof buf);
    buf.type   = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    buf.memory = V4L2_MEMORY_USERPTR;
	printf("func = %s, line = %d\n", __FUNCTION__, __LINE__);

    if ((ret = ioctl(dev->fd, VIDIOC_DQBUF, &buf)) < 0)
    {
        return ret;
    }

    uvc_video_fill_buffer_userptr(dev, &buf);
	printf("func = %s, line = %d: %d, %d, %p, %d %d\n", __FUNCTION__, __LINE__, dev->type, buf.memory, buf.m.userptr, buf.bytesused, buf.length);

    if ((ret = ioctl(dev->fd, VIDIOC_QBUF, &buf)) < 0)
    {
        printf("Unable to requeue buffer: %s (%d).\n", strerror(errno), errno);
        return ret;
    }

    return 0;
}


static void *uvc_data_run_thd(void *arg)
{
	pthread_detach(pthread_self());
	
	struct uvc_device *dev;
    fd_set efds, wfds;
    struct timeval tv;
    int ret;

	dev = (struct uvc_device *)arg;
    tv.tv_sec  = 1;
    tv.tv_usec = 0;
    FD_ZERO(&efds);
    FD_ZERO(&wfds);
    FD_SET(dev->fd, &efds);
	FD_SET(dev->fd, &wfds);
	
	while (dev->streaming != 0)
	{
		ret = select(dev->fd + 1, NULL, &wfds, &efds, &tv);
		if (ret > 0)
		{
			if (FD_ISSET(dev->fd, &efds))
			{
				uvc_events_process(dev);
			}
			if (FD_ISSET(dev->fd, &wfds))
			{
				ret = uvc_data_process_userptr(dev);
				if (ret < 0) {
					printf("func = %s, line = %d, uvc_data_process_userptr error.\n", __FUNCTION__, __LINE__);
					break;
				}
			}
		} else {
			usleep(200000);
		}
	}
	
	return NULL;
}


int uvc_streamon(struct uvc_device *dev)
{
    int type = dev->type;
    int ret;

    ret = ioctl (dev->fd, VIDIOC_STREAMON, &type);
    if (ret < 0) {
        printf("Unable to %s capture: %d.\n", "start", errno);
        return ret;
    }
    dev->streaming = 1;

	printf("func = %s, line = %d\n", __FUNCTION__, __LINE__);
	if (dev->type == V4L2_BUF_TYPE_VIDEO_OUTPUT) {
		pthread_t tid;
		pthread_create(&tid, 0, uvc_data_run_thd, (void *)dev);
	}
    
    return 0;
}

int uvc_streamoff(struct uvc_device *dev)
{
    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    int ret;

    ret = ioctl (dev->fd, VIDIOC_STREAMOFF, &type);
    if (ret < 0) {
        printf("Unable to %s capture: %d.\n", "stop", errno);
        return ret;
    }
    dev->streaming = 0;

    return 0;
}


int uvc_close(struct uvc_device *dev)
{
    int i;
	
	uvc_video_deinit(dev);
	
    if (dev->streaming != 0) {
        uvc_streamoff(dev);
		dev->streaming = 3;
		usleep(200000);
    }
    
    for (i = 0; i < dev->nbufs; i++) {
        munmap(dev->mem[i], dev->length[i]);
    }

    close(dev->fd);
	free(dev);
    
    return 0;
}


struct uvc_device *uvc_open(const char *devpath, struct uvc_devattr *devattr)
{
    struct uvc_device *dev = NULL;
    
    dev = (struct uvc_device*)malloc(sizeof(struct uvc_device));
    if (dev == NULL) {
		printf("func = %s, line = %d error.\n", __FUNCTION__, __LINE__);
        return NULL;
    }

    if (NULL == devattr) {
        printf("NULL == devattr error!\n");
		printf("func = %s, line = %d error.\n", __FUNCTION__, __LINE__);
        return NULL;
    }

    dev->fd = uvc_open_device(devpath);
    if (dev->fd < 0) {
        printf("uvc_open_device error!\n");
		printf("func = %s, line = %d error.\n", __FUNCTION__, __LINE__);
        return NULL;
    }
    
    /* init uvc fomat */
    dev->type = devattr->type;
    if (dev->type == V4L2_BUF_TYPE_VIDEO_CAPTURE) {
        struct v4l2_format fmt;
        uvc_get_format(dev, &fmt);
        dev->type = fmt.type;
        dev->pix_fmt = fmt.fmt.pix.pixelformat;
        dev->width = fmt.fmt.pix.width;
        dev->height = fmt.fmt.pix.height;
		printf("fmt.pix.width = %d,  fmt.pix.height = %d.\n", fmt.fmt.pix.width, fmt.fmt.pix.height);
    } else if (dev->type == V4L2_BUF_TYPE_VIDEO_OUTPUT) {    // V4L2_BUF_TYPE_VIDEO_OUTPUT
        dev->pix_fmt = devattr->pix_fmt;
        dev->width = devattr->width;
        dev->height = devattr->height;
        uvc_set_format(dev);
    } else {
		printf("func = %s, line = %d error.\n", __FUNCTION__, __LINE__);
		return NULL;
	}
    
	uvc_video_init(dev);
	
    dev->nbufs = 4;
    if (uvc_reqbufs(dev) < 0) {
        printf("uvc_reqbufs error!\n");
		printf("func = %s, line = %d error.\n", __FUNCTION__, __LINE__);
        return NULL;
    } 
    if (dev->type == V4L2_BUF_TYPE_VIDEO_CAPTURE && uvc_mmapbuf(dev) < 0) {
        printf("uvc_mmapbuf error!\n");
        //return NULL;
    } 
    if (uvc_qbuf(dev) < 0) {
        printf("uvc_qbuf error!\n");
		printf("func = %s, line = %d error.\n", __FUNCTION__, __LINE__);
        return NULL;
    } 

    return dev;
}

