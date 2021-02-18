/*
 *	uvc_gadget.h  --  USB Video Class Gadget driver
 *
 *	Copyright (C) 2009-2010
 *	    Laurent Pinchart (laurent.pinchart@ideasonboard.com)
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 */

#ifndef _UVC_GADGET_H_
#define _UVC_GADGET_H_

#include <linux/ioctl.h>
#include <linux/types.h>
#include <linux/usb/ch9.h>
#include "uvc_defines.h"


#ifdef __cplusplus
#if __cplusplus
extern "C"
{
#endif
#endif /* __cplusplus */


#define UVC_EVENT_FIRST (V4L2_EVENT_PRIVATE_START + 0)
#define UVC_EVENT_CONNECT (V4L2_EVENT_PRIVATE_START + 0)
#define UVC_EVENT_DISCONNECT (V4L2_EVENT_PRIVATE_START + 1)
#define UVC_EVENT_STREAMON (V4L2_EVENT_PRIVATE_START + 2)
#define UVC_EVENT_STREAMOFF (V4L2_EVENT_PRIVATE_START + 3)
#define UVC_EVENT_SETUP (V4L2_EVENT_PRIVATE_START + 4)
#define UVC_EVENT_DATA (V4L2_EVENT_PRIVATE_START + 5)
#define UVC_EVENT_LAST (V4L2_EVENT_PRIVATE_START + 5)

struct uvc_request_data
{
    __s32 length;
    __u8  data[60];
};

struct uvc_event
{
    union
    {
        enum usb_device_speed   speed;
        struct usb_ctrlrequest  req;
        struct uvc_request_data data;
    };
};

#define UVCIOC_SEND_RESPONSE _IOW('U', 1, struct uvc_request_data)

#define UVC_INTF_CONTROL 0
#define UVC_INTF_STREAMING 1


/* ============ V4L2 <linux/videodev.h> ==================*/
 #define v4l2_fourcc(a, b, c, d) \
    ((__u32)(a) | ((__u32)(b) << 8) | ((__u32)(c) << 16) | ((__u32)(d) << 24))

 #define VIDIOC_G_FMT		    _IOWR('V', 4, struct v4l2_format)
 #define VIDIOC_S_FMT           _IOWR('V', 5, struct v4l2_format)
 #define VIDIOC_REQBUFS         _IOWR('V', 8, struct v4l2_requestbuffers)
 #define VIDIOC_QUERYBUF        _IOWR('V', 9, struct v4l2_buffer)
 #define VIDIOC_QUERYCAP        _IOR('V', 0, struct v4l2_capability)
 #define VIDIOC_STREAMON        _IOW('V', 18, int)
 #define VIDIOC_STREAMOFF       _IOW('V', 19, int)
 #define VIDIOC_QBUF            _IOWR('V', 15, struct v4l2_buffer)
 #define VIDIOC_DQBUF           _IOWR('V', 17, struct v4l2_buffer)
 #define VIDIOC_G_CTRL		    _IOWR('V', 27, struct v4l2_control)
 #define VIDIOC_S_CTRL          _IOWR('V', 28, struct v4l2_control)
 #define VIDIOC_QUERYCTRL	    _IOWR('V', 36, struct v4l2_queryctrl)
 #define VIDIOC_S_DV_TIMINGS    _IOWR('V', 87, struct v4l2_dv_timings)
 #define VIDIOC_G_DV_TIMINGS    _IOWR('V', 88, struct v4l2_dv_timings)
 #define VIDIOC_DQEVENT         _IOR('V', 89, struct v4l2_event)
 #define VIDIOC_SUBSCRIBE_EVENT     _IOW('V', 90, struct v4l2_event_subscription)
 #define VIDIOC_UNSUBSCRIBE_EVENT   _IOW('V', 91, struct v4l2_event_subscription)

 #define V4L2_PIX_FMT_YUYV v4l2_fourcc('Y', 'U', 'Y', 'V')/* 16  YUV 4:2:2     */
 #define V4L2_PIX_FMT_YUV420 v4l2_fourcc('Y', 'U', '1', '2')/* 16  YUV 4:2:0     */
 #define V4L2_PIX_FMT_NV12 v4l2_fourcc('N', 'V', '1', '2')/* 16  YUV 4:2:0     */
 #define V4L2_PIX_FMT_MJPEG v4l2_fourcc('M', 'J', 'P', 'G')/* Motion-JPEG   */
 #define V4L2_PIX_FMT_JPEG v4l2_fourcc('J', 'P', 'E', 'G')/* JFIF JPEG     */
 #define V4L2_PIX_FMT_DV v4l2_fourcc('d', 'v', 's', 'd')/* 1394          */
 #define V4L2_PIX_FMT_MPEG v4l2_fourcc('M', 'P', 'E', 'G')/* MPEG-1/2/4 Multiplexed */
 #define V4L2_PIX_FMT_H264 v4l2_fourcc('H', '2', '6', '4')/* H264 with start codes */
 #define V4L2_PIX_FMT_H264_NO_SC v4l2_fourcc('A', 'V', 'C', '1')/* H264 without start codes */
 #define V4L2_PIX_FMT_H264_MVC v4l2_fourcc('M', '2', '6', '4')/* H264 MVC */
 #define V4L2_PIX_FMT_H265     v4l2_fourcc('H', '2', '6', '5') /* H265 with start codes */
 #define V4L2_PIX_FMT_H263 v4l2_fourcc('H', '2', '6', '3')/* H263          */
 #define V4L2_PIX_FMT_MPEG1 v4l2_fourcc('M', 'P', 'G', '1')/* MPEG-1 ES     */
 #define V4L2_PIX_FMT_MPEG2 v4l2_fourcc('M', 'P', 'G', '2')/* MPEG-2 ES     */
 #define V4L2_PIX_FMT_MPEG4 v4l2_fourcc('M', 'P', 'G', '4')/* MPEG-4 ES     */
 #define V4L2_PIX_FMT_XVID v4l2_fourcc('X', 'V', 'I', 'D')/* Xvid           */
 #define V4L2_PIX_FMT_VC1_ANNEX_G v4l2_fourcc('V', 'C', '1', 'G')/* SMPTE 421M Annex G compliant stream */
 #define V4L2_PIX_FMT_VC1_ANNEX_L v4l2_fourcc('V', 'C', '1', 'L')/* SMPTE 421M Annex L compliant stream */
 #define V4L2_PIX_FMT_VP8 v4l2_fourcc('V', 'P', '8', '0')/* VP8 */

 #define V4L2_EVENT_PRIVATE_START 0x08000000
 #define UVC_EVENT_FIRST (V4L2_EVENT_PRIVATE_START + 0)
 #define UVC_EVENT_CONNECT (V4L2_EVENT_PRIVATE_START + 0)
 #define UVC_EVENT_DISCONNECT (V4L2_EVENT_PRIVATE_START + 1)
 #define UVC_EVENT_STREAMON (V4L2_EVENT_PRIVATE_START + 2)
 #define UVC_EVENT_STREAMOFF (V4L2_EVENT_PRIVATE_START + 3)
 #define UVC_EVENT_SETUP (V4L2_EVENT_PRIVATE_START + 4)
 #define UVC_EVENT_DATA (V4L2_EVENT_PRIVATE_START + 5)
 #define UVC_EVENT_LAST (V4L2_EVENT_PRIVATE_START + 5)

/*
struct uvc_streaming_control
{
    __u16 bmHint;
    __u8  bFormatIndex; // Video format index
    __u8  bFrameIndex; // Video frame index
    __u32 dwFrameInterval; // Video frame interval
    __u16 wKeyFrameRate;
    __u16 wPFrameRate;
    __u16 wCompQuality;
    __u16 wCompWindowSize;
    __u16 wDelay;
    __u32 dwMaxVideoFrameSize;
    __u32 dwMaxPayloadTransferSize;
    __u32 dwClockFrequency;
    __u8  bmFramingInfo;
    __u8  bPreferedVersion;
    __u8  bMinVersion;
    __u8  bMaxVersion;
} __attribute__((__packed__));
*/

struct v4l2_control 
{
	__u32		     id;
	__s32		     value;
};

/*  Used in the VIDIOC_QUERYCTRL ioctl for querying controls */
struct v4l2_queryctrl {
	__u32		     id;
	__u32		     type;	/* enum v4l2_ctrl_type */
	__u8		     name[32];	/* Whatever */
	__s32		     minimum;	/* Note signedness */
	__s32		     maximum;
	__s32		     step;
	__s32		     default_value;
	__u32                flags;
	__u32		     reserved[2];
};

struct v4l2_event_subscription
{
    __u32 type;
    __u32 id;
    __u32 flags;
    __u32 reserved[5];
};

/* Payload for V4L2_EVENT_VSYNC */
struct v4l2_event_vsync
{
    /* Can be V4L2_FIELD_ANY, _NONE, _TOP or _BOTTOM */
    __u8 field;
} __attribute__ ((packed));

/* Payload for V4L2_EVENT_CTRL */
 #define V4L2_EVENT_CTRL_CH_VALUE (1 << 0)
 #define V4L2_EVENT_CTRL_CH_FLAGS (1 << 1)
 #define V4L2_EVENT_CTRL_CH_RANGE (1 << 2)

struct v4l2_event_ctrl
{
    __u32 changes;
    __u32 type;
    union
    {
        __s32 value;
        __s64 value64;
    };
    __u32 flags;
    __s32 minimum;
    __s32 maximum;
    __s32 step;
    __s32 default_value;
};

struct v4l2_event_frame_sync
{
    __u32 frame_sequence;
};

struct v4l2_event
{
    __u32 type;
    union
    {
        struct v4l2_event_vsync      vsync;
        struct v4l2_event_ctrl       ctrl;
        struct v4l2_event_frame_sync frame_sync;
        __u8                         data[64];
    }               u;
    __u32           pending;
    __u32           sequence;
    struct timespec timestamp;
    __u32           id;
    __u32           reserved[8];
};
enum v4l2_buf_type
{
    V4L2_BUF_TYPE_VIDEO_CAPTURE = 1,
    V4L2_BUF_TYPE_VIDEO_OUTPUT  = 2,
    V4L2_BUF_TYPE_VIDEO_OVERLAY = 3,
    V4L2_BUF_TYPE_VBI_CAPTURE = 4,
    V4L2_BUF_TYPE_VBI_OUTPUT = 5,
    V4L2_BUF_TYPE_SLICED_VBI_CAPTURE = 6,
    V4L2_BUF_TYPE_SLICED_VBI_OUTPUT = 7,
 #if 1
    /* Experimental */
    V4L2_BUF_TYPE_VIDEO_OUTPUT_OVERLAY = 8,
 #endif
    V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE = 9,
    V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE = 10,

    /* Deprecated, do not use */
    V4L2_BUF_TYPE_PRIVATE = 0x80,
};

enum v4l2_memory
{
    V4L2_MEMORY_MMAP = 1,
    V4L2_MEMORY_USERPTR = 2,
    V4L2_MEMORY_OVERLAY = 3,
    V4L2_MEMORY_DMABUF = 4,
};

enum v4l2_field
{
    V4L2_FIELD_ANY  = 0,          /* driver can choose from none,
                                  top, bottom, interlaced
                                  depending on whatever it thinks
                                  is approximate ... */
    V4L2_FIELD_NONE = 1, /* this device has no fields ... */
    V4L2_FIELD_TOP = 2, /* top field only */
    V4L2_FIELD_BOTTOM = 3, /* bottom field only */
    V4L2_FIELD_INTERLACED = 4, /* both fields interlaced */
    V4L2_FIELD_SEQ_TB = 5,        /* both fields sequential into one
                                  buffer, top-bottom order */
    V4L2_FIELD_SEQ_BT = 6, /* same as above + bottom-top order */
    V4L2_FIELD_ALTERNATE = 7,     /* both fields alternating into
                                  separate buffers */
    V4L2_FIELD_INTERLACED_TB = 8, /* both fields interlaced, top field
                                  first and the top field is
                                  transmitted first */
    V4L2_FIELD_INTERLACED_BT = 9, /* both fields interlaced, top field
                                  first and the bottom field is
                                  transmitted first */
};
struct v4l2_capability
{
    __u8  driver[16];
    __u8  card[32];
    __u8  bus_info[32];
    __u32 version;
    __u32 capabilities;
    __u32 device_caps;
    __u32 reserved[3];
};
struct v4l2_timecode
{
    __u32 type;
    __u32 flags;
    __u8  frames;
    __u8  seconds;
    __u8  minutes;
    __u8  hours;
    __u8  userbits[4];
};
struct v4l2_buffer
{
    __u32                index;
    __u32                type;
    __u32                bytesused;
    __u32                flags;
    __u32                field;
    struct timeval       timestamp;
    struct v4l2_timecode timecode;
    __u32                sequence;

    /* memory location */
    __u32 memory;
    union
    {
        __u32              offset;
        unsigned long      userptr;
        struct v4l2_plane* planes;
        __s32              fd;
    }     m;
    __u32 length;
    __u32 reserved2;
    __u32 reserved;
};
struct v4l2_requestbuffers
{
    __u32 count;
    __u32 type;           /* enum v4l2_buf_type */
    __u32 memory;         /* enum v4l2_memory */
    __u32 reserved[2];
};
struct v4l2_pix_format
{
    __u32 width;
    __u32 height;
    __u32 pixelformat;
    __u32 field;          /* enum v4l2_field */
    __u32 bytesperline;   /* for padding, zero if unused */
    __u32 sizeimage;
    __u32 colorspace;     /* enum v4l2_colorspace */
    __u32 priv;           /* private data, depends on pixelformat */
};
 #define VIDEO_MAX_PLANES 8
struct v4l2_plane_pix_format
{
    __u32 sizeimage;
    __u16 bytesperline;
    __u16 reserved[7];
} __attribute__ ((packed));
struct v4l2_pix_format_mplane
{
    __u32 width;
    __u32 height;
    __u32 pixelformat;
    __u32 field;
    __u32 colorspace;

    struct v4l2_plane_pix_format plane_fmt[VIDEO_MAX_PLANES];
    __u8                         num_planes;
    __u8                         reserved[11];
} __attribute__ ((packed));
struct v4l2_rect
{
    __s32 left;
    __s32 top;
    __s32 width;
    __s32 height;
};
 #define __user
struct v4l2_clip
{
    struct v4l2_rect c;
    struct v4l2_clip __user* next;
};
struct v4l2_window
{
    struct v4l2_rect w;
    __u32            field;          /* enum v4l2_field */
    __u32            chromakey;
    struct v4l2_clip __user* clips;
    __u32            clipcount;
    void __user*     bitmap;
    __u8             global_alpha;
};
struct v4l2_vbi_format
{
    __u32 sampling_rate;          /* in 1 Hz */
    __u32 offset;
    __u32 samples_per_line;
    __u32 sample_format;          /* V4L2_PIX_FMT_* */
    __s32 start[2];
    __u32 count[2];
    __u32 flags;                  /* V4L2_VBI_* */
    __u32 reserved[2];            /* must be zero */
};
struct v4l2_sliced_vbi_format
{
    __u16 service_set;

    /* service_lines[0][...] specifies lines 0-23 (1-23 used) of the first field
       service_lines[1][...] specifies lines 0-23 (1-23 used) of the second field
       (equals frame lines 313-336 for 625 line video
       standards, 263-286 for 525 line standards) */
    __u16 service_lines[2][24];
    __u32 io_size;
    __u32 reserved[2];            /* must be zero */
};
struct v4l2_format
{
    __u32 type;
    union
    {
        struct v4l2_pix_format        pix;       /* V4L2_BUF_TYPE_VIDEO_CAPTURE */
        struct v4l2_pix_format_mplane pix_mp;  /* V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE */
        struct v4l2_window            win;       /* V4L2_BUF_TYPE_VIDEO_OVERLAY */
        struct v4l2_vbi_format        vbi;       /* V4L2_BUF_TYPE_VBI_CAPTURE */
        struct v4l2_sliced_vbi_format sliced;  /* V4L2_BUF_TYPE_SLICED_VBI_CAPTURE */
        __u8                          raw_data[200]; /* user-defined */
    } fmt;
};


#define clamp(val, min, max) ({                 \
                                  typeof(val)__val = (val);              \
                                  typeof(min)__min = (min);              \
                                  typeof(max)__max = (max);              \
                                  (void) (&__val == &__min);              \
                                  (void) (&__val == &__max);              \
                                  __val = __val < __min ? __min : __val;   \
                                  __val > __max ? __max : __val; })

#define ARRAY_SIZE(a) ((sizeof(a) / sizeof(a[0])))

#define CLEAR(x) memset (&(x), 0, sizeof (x))


enum uvc_ext_cmd_u
{
    CMD_GET_CAMERA_VERSION = 0x01,
    CMD_SET_CAMERA_IP,
    CMD_START_CAMERA,
    CMD_SHUTDOWN_CAMERA,
    CMD_RESET_CAMERA,
    CMD_SET_MOTOR_RATE = 0x06,
    CMD_SET_MOTOR_BY_STEPS = 0x07,
    CMD_SET_MOTOR_BY_USER = 0x08,
    CMD_STOP_MOTOR_BY_USER = 0x09,
    CMD_SET_EPTZ = 0x0a,
    CMD_SET_H265 = 0x0b,
    CMD_MAX_NUM = CMD_SET_H265,
};



struct uvc_stream_attr
{
    int             pix_fmt;    // V4L2_PIX_FMT_YUYV/V4L2_PIX_FMT_YUV420/V4L2_PIX_FMT_MJPEG/V4L2_PIX_FMT_H264
    unsigned int    width;
    unsigned int    height;
    unsigned int    length;     // frame data length
    void            *data;      // frame data
};


struct uvc_devattr  // output only
{
    int             type;       // V4L2_BUF_TYPE_VIDEO_CAPTURE=1,V4L2_BUF_TYPE_VIDEO_OUTPUT=2
    int             pix_fmt;    // V4L2_PIX_FMT_YUYV/V4L2_PIX_FMT_YUV420/V4L2_PIX_FMT_MJPEG/V4L2_PIX_FMT_H264
    unsigned int    width;      
    unsigned int    height;     
};

typedef struct uvc_probe_t
{
    unsigned char set;
    unsigned char get;
    unsigned char max;
    unsigned char min;
} uvc_probe_t;


struct uvc_device
{
    int                 			fd;
    struct uvc_streaming_control 	probe;
    struct uvc_streaming_control 	commit;
    enum v4l2_buf_type  			type;                   // V4L2_BUF_TYPE_VIDEO_CAPTURE/V4L2_BUF_TYPE_VIDEO_OUTPUT
    int                 			pix_fmt;                // fcc, V4L2_PIX_FMT_YUYV/V4L2_PIX_FMT_YUV420/V4L2_PIX_FMT_MJPEG/V4L2_PIX_FMT_H264/V4L2_PIX_FMT_H265
    #define MAX_NB_BUFFER 16
    void                			*mem[MAX_NB_BUFFER];    // mmap buf, CAPTURE only
    unsigned int        			length[MAX_NB_BUFFER];  // mmap buf length, CAPTURE only
    unsigned int        			nbufs;                  // set for CAPTURE, get from OUTPUT
    struct v4l2_buffer  			buf;                    // video buf used for get/relase video frame
    unsigned int 					bulk;
    unsigned int        			width;
    unsigned int        			height;
    unsigned int        			max_width;
    unsigned int        			max_height;
    unsigned int        			image_size;				// default max_width * max_height * 2
    unsigned int        			streaming;              // 0:off, 1:on
	void							*video_handle;			// inter ringbuf used
	// event
    int control;
    int unit_id;
    int interface_id;
    uvc_probe_t  					probe_status;
    struct uvc_request_data 		request_error_code;
};





/* auto to get /dev/video id num */
int uvc_video_id_get(void);
struct uvc_device *uvc_open(const char *devpath, struct uvc_devattr *devattr);
int uvc_close(struct uvc_device *dev);
int uvc_streamon(struct uvc_device *dev);
int uvc_streamoff(struct uvc_device *dev);
int uvc_stream_get(struct uvc_device *dev, struct uvc_stream_attr *stream);		// for V4L2_BUF_TYPE_VIDEO_CAPTURE
int uvc_stream_release(struct uvc_device *dev);



#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* _UVC_GADGET_H_ */
