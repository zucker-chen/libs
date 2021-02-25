#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include "uvc_gadget.h"
#include "uvc_video.h"


//#define UVC_BUILD_TEST	1
#ifndef UVC_BUILD_TEST
	#include "hal_stream_api.h"
	#include "hal_venc.h"
	#include "ringbuf.h"
#endif


#if defined(RTSP_SUPPORT)
extern void rtsp_stop();
int rtsp_start();
#endif


// calc max frame size, init for dev->image_size
int uvc_video_max_frame_size_get(struct uvc_device* dev)
{
    switch (dev->pix_fmt)
    {
	    case V4L2_PIX_FMT_YUYV:
			dev->image_size = dev->max_width * dev->max_height * 2;
			break;
	    case V4L2_PIX_FMT_YUV420:
			dev->image_size = dev->max_width * dev->max_height * 1.5;
			break;
		case V4L2_PIX_FMT_MJPEG:
			dev->image_size = dev->max_width * dev->max_height / 3;
			break;
	    case V4L2_PIX_FMT_H264:
	    case V4L2_PIX_FMT_H265:
	        dev->image_size = dev->max_width * dev->max_height / 3;
	        break;
    }

	return dev->image_size;
}

int uvc_video_fill_buffer_userptr(struct uvc_device* dev, struct v4l2_buffer* buf)
{
    switch (dev->pix_fmt)
    {
		case V4L2_PIX_FMT_MJPEG:
		case V4L2_PIX_FMT_H264:
		case V4L2_PIX_FMT_H265:
		case V4L2_PIX_FMT_YUYV:
		case V4L2_PIX_FMT_YUV420:
		{
			// TODO
			#ifndef UVC_BUILD_TEST
			int retry_count = 0;
			int i, ret;
			
			buf->bytesused = 0;
			hal_stream_rhandle_t reader_handle;
			hal_stream_frame_info_t *pkg;
			
			//printf("func = %s, line = %d\n", __FUNCTION__, __LINE__);
			reader_handle = (hal_stream_rhandle_t)dev->video_handle;
			retry_count = 5;
			
			for (i = 0; i < retry_count; i++)
			{
				ret = hal_stream_get_frame(reader_handle, &pkg);
				if (ret != 0) {
					usleep(10000);
					continue;
				}

				buf->bytesused = pkg->data_len;
				memcpy(dev->mem[buf->index], pkg->data, pkg->data_len);
				//buf->m.userptr = (unsigned long)pkg->data;
				buf->m.userptr = (unsigned long)dev->mem[buf->index];
				
				hal_stream_release_frame(reader_handle);
				break;
			}
			#endif
			// END
		} 
			break;
		default:
			printf("what up pix_fmt %d.\n", dev->pix_fmt);
			break;
    }	
	
	return 0;
}

// 修改编码参数
int uvc_video_format_set(struct uvc_device* dev)
{
	// TODO
	#ifndef UVC_BUILD_TEST
	hal_venc_t venc = {0};
	int chn, ret = 0;

	chn = 0;
	hal_venc_cfg_get(chn, &venc);
    switch (dev->pix_fmt)
    {
        case V4L2_PIX_FMT_YUYV:
        case V4L2_PIX_FMT_YUV420:
			printf("func = %s, line = %d: yuv can not support now.\n", __FUNCTION__, __LINE__);
            break;
        case V4L2_PIX_FMT_MJPEG:
			venc.venc_attr.payload = HAL_STREAM_TYPE_MJPEG;
			break;
        case V4L2_PIX_FMT_H264:
            venc.venc_attr.payload = HAL_STREAM_TYPE_H264;
            break;
    }
	venc.venc_attr.res_size.width = dev->width;
	venc.venc_attr.res_size.height = dev->height;
	ret = hal_venc_restart(chn, &venc);
	if (ret < 0) {
		printf("%s, %d: venc%d restart error!\n", __FUNCTION__, __LINE__, chn);
	} else if (ret > 0) {
		printf("%s, %d: venc%d no changed!\n", __FUNCTION__, __LINE__, chn);
		return 1;
	}

	hal_stream_read_deinit((hal_stream_rhandle_t)dev->video_handle);
	dev->video_handle = (void *)hal_stream_read_init(0);

	// restart rtsp
	#if defined(RTSP_SUPPORT)
	rtsp_stop();
	rtsp_start();
	#endif

	#endif
	// END
	
	return 0;
}


int uvc_video_init(struct uvc_device* dev)
{
	// TODO
	printf("func = %s, line = %d\n", __FUNCTION__, __LINE__);
	#ifndef UVC_BUILD_TEST
	hal_stream_rhandle_t reader_handle;
    unsigned int page_size, buffer_size;
	int i;

	if (dev->type == V4L2_BUF_TYPE_VIDEO_OUTPUT) {
		// 取流句柄初始化
		reader_handle = hal_stream_read_init(0);
		if (reader_handle == NULL) {
			return -1;
		}
		dev->video_handle = (void *)reader_handle;
	}	

	if (dev->mem_type == V4L2_MEMORY_USERPTR) {
		// 初始化V4L2_MEMORY_USERPTR视频缓冲区
	    page_size = getpagesize();
	    buffer_size = (dev->image_size + page_size - 1) & ~(page_size - 1);
	    for (i = 0; i < dev->nbufs; i++) 
		{
			dev->mem[i] = (void *)memalign(page_size, buffer_size);
	        if (NULL == dev->mem[i])
	        {
	            printf("failed to malloc dev->mem field\n");
				return -1;
	        }
			dev->length[i] = buffer_size;
	    }
	}
	
	#endif
	// END
	
	return 0;
}


int uvc_video_deinit(struct uvc_device* dev)
{
	// TODO
	#ifndef UVC_BUILD_TEST
	int i;

	if (dev->type == V4L2_BUF_TYPE_VIDEO_OUTPUT) {
		hal_stream_read_deinit((hal_stream_rhandle_t)dev->video_handle);
	}
	
	if (dev->mem_type == V4L2_MEMORY_USERPTR) {
	    for (i = 0; i < dev->nbufs; i++) 
		{
			if (dev->mem[i]) {
				free(dev->mem[i]);
				dev->mem[i] = NULL;
			}
	    }
	}
	#endif
	// END
	
	return 0;
}


