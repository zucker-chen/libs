

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

#include "libuvc.h"





/* Queue the buffers. */
static int uvc_qbuf(struct uvc_device* dev)
{
    struct v4l2_buffer buf;
    int ret, i;

    for (i = 0; i < dev->nbufs; i++) {
        memset (&buf, 0, sizeof(struct v4l2_buffer));
        buf.index = i;
        buf.type = dev->type;
        buf.memory = (dev->type == V4L2_BUF_TYPE_VIDEO_CAPTURE) ? V4L2_MEMORY_MMAP : V4L2_MEMORY_USERPTR;;
        ret = ioctl (dev->fd, VIDIOC_QBUF, &buf);
        if (ret < 0) {
            printf("Unable to Queue buffer (%d).\n", errno);
            return -1;
        }
    }

    return 0;
}

/* map the buffers */
static int uvc_mmapbuf(struct uvc_device* dev)
{
    struct v4l2_buffer buf;
    int ret, i;

    for (i = 0; i < dev->nbufs; i++) {
        memset (&buf, 0, sizeof(struct v4l2_buffer));
        buf.index = i;
        buf.type = dev->type;
        buf.memory = (dev->type == V4L2_BUF_TYPE_VIDEO_CAPTURE) ? V4L2_MEMORY_MMAP : V4L2_MEMORY_USERPTR;;
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
        dev->length[i] = buf.length;
        printf("Buffer mapped at address %p.\n", dev->mem[i]);
    }

    return 0;
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

    if ((dev->pix_fmt == V4L2_PIX_FMT_MJPEG) || (dev->pix_fmt == V4L2_PIX_FMT_H264))
    {
        fmt.fmt.pix.sizeimage = dev->width * dev->height * 2;
    }

    if ((ret = ioctl(dev->fd, VIDIOC_S_FMT, &fmt)) < 0)
    {
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
    if ((ret = ioctl(dev->fd, VIDIOC_G_FMT, fmt)) < 0)
    {
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

    if (dev->streaming == 0) {
        uvc_streamoff(dev);
    }
    
    for (i = 0; i < dev->nbufs; i++) {
        munmap(dev->mem[i], dev->length[i]);
    }

    close(dev->fd);
    
    return 0;
}


struct uvc_device *uvc_open(const char *devpath, struct uvc_devattr *devattr)
{
    struct uvc_device *dev = NULL;
    
    dev = (struct uvc_device*)malloc(sizeof(struct uvc_device));
    if (dev == NULL) {
        return NULL;
    }

    if (NULL == devattr) {
        printf("NULL == devattr error!\n");
        return NULL;
    }

    dev->fd = uvc_open_device(devpath);
    if (dev->fd < 0) {
        printf("uvc_open_device error!\n");
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
    } else {    // V4L2_BUF_TYPE_VIDEO_OUTPUT
        dev->pix_fmt = devattr->pix_fmt;
        dev->width = devattr->width;
        dev->height = devattr->height;
        uvc_set_format(dev);
    }
    
    dev->nbufs = 6;
    if (uvc_reqbufs(dev) < 0) {
        printf("uvc_reqbufs error!\n");
        return NULL;
    } 
    if (uvc_mmapbuf(dev) < 0) {
        printf("uvc_mmapbuf error!\n");
        return NULL;
    } 
    if (uvc_qbuf(dev) < 0) {
        printf("uvc_qbuf error!\n");
        return NULL;
    } 
    
    return dev;
}

