

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

#include "uvc_defines.h"
#include "uvc_gadget.h"
#include "uvc_video.h"






/* --------------------------------------------------------------------------- */
static void uvc_fill_streaming_control(struct uvc_device* dev, struct uvc_streaming_control* ctrl, int iframe, int iformat);
static int uvc_data_process_userptr(struct uvc_device *dev);
int uvc_streamon(struct uvc_device *dev);
int uvc_streamoff(struct uvc_device *dev);
static int uvc_set_format(struct uvc_device* dev);
static int uvc_get_format(struct uvc_device* dev, struct v4l2_format *fmt);
static int uvc_reqbufs(struct uvc_device* dev);
static int uvc_qbuf(struct uvc_device* dev);
static int uvc_event_streamon(struct uvc_device *dev);



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
    //{ 3840, 2160, {333333,       0 }, },
    {    0,    0, {		0,         }, },
};

static const struct uvc_frame_info uvc_frames_mjpeg[] =
{
    {  640,  360, {333333,       0 }, },
    { 1280,  720, {333333,		0  }, },
    { 1920, 1080, {333333,       0 }, },
    //{ 3840, 2160, {333333,       0 }, },
    {    0,    0, {		0,         }, },
};

static const struct uvc_frame_info uvc_frames_h264[] =
{
    //{ 3840, 2160, {333333,       0 }, },
    { 1920, 1080, {333333,       0 }, },
    { 1280,  720, {333333,		0  }, },
    {  640,  480, {333333,       0 }, },
    {    0,    0, {		0,         }, },
};

static const struct uvc_frame_info uvc_frames_h265[] =
{
    {  640,  360, {333333,       0 }, },
    { 1280,  720, {333333,		0  }, },
    { 1920, 1080, {333333,       0 }, },
    //{ 3840, 2160, {333333,       0 }, },
    {    0,    0, {		0,         }, },
};

static const struct uvc_format_info uvc_formats[] =
{ //yuv first
    { V4L2_PIX_FMT_YUYV,  uvc_frames_yuyv  },
    { V4L2_PIX_FMT_MJPEG, uvc_frames_mjpeg },
    { V4L2_PIX_FMT_H264,  uvc_frames_h264  },
    { V4L2_PIX_FMT_H265,  uvc_frames_h265  },
};



static void uvc_event_undefined_control(struct uvc_device *dev, uint8_t req, uint8_t cs, struct uvc_request_data *resp)
{
	printf("func = %s, line = %d, cs = %d, req = %d\n", __FUNCTION__, __LINE__, cs, req);
    switch (cs)
    {
	    case UVC_VC_REQUEST_ERROR_CODE_CONTROL:
	        resp->length = dev->request_error_code.length;
	        resp->data[0] = dev->request_error_code.data[0];
	        printf("dev->request_error_code.data[0] = %d\n",dev->request_error_code.data[0]);
	        break;
	    default:
	        dev->request_error_code.length = 1;
	        dev->request_error_code.data[0] = 0x06;
	        break;
    }
}

static void uvc_event_it_control(struct uvc_device *dev, uint8_t req, uint8_t unit_id, uint8_t cs, struct uvc_request_data *resp)
{
	printf("func = %s, line = %d, cs = %d, req = %d\n", __FUNCTION__, __LINE__, cs, req);
	switch (cs) 
	{
		case UVC_CT_AE_MODE_CONTROL:
			switch (req)
			{
				case UVC_SET_CUR:
					/* Incase of auto exposure, attempts to
					 * programmatically set the auto-adjusted
					 * controls are ignored.
					 */
					resp->data[0] = 0x01;
					resp->length = 1;
					/*
					 * For every successfully handled control
					 * request set the request error code to no
					 * error.
					 */
					dev->request_error_code.data[0] = 0x00;
					dev->request_error_code.length = 1;
					break;
			
				case UVC_GET_INFO:
					/*
					 * TODO: We support Set and Get requests, but
					 * don't support async updates on an video
					 * status (interrupt) endpoint as of
					 * now.
					 */
					resp->data[0] = 0x03;
					resp->length = 1;
					/*
					 * For every successfully handled control
					 * request set the request error code to no
					 * error.
					 */
					dev->request_error_code.data[0] = 0x00;
					dev->request_error_code.length = 1;
					break;
			
				case UVC_GET_CUR:
				case UVC_GET_DEF:
				case UVC_GET_RES:
					/* Auto Mode Ã¢?? auto Exposure Time, auto Iris. */
					resp->data[0] = 0x02;
					resp->length = 1;
					/*
					 * For every successfully handled control
					 * request set the request error code to no
					 * error.
					 */
					dev->request_error_code.data[0] = 0x00;
					dev->request_error_code.length = 1;
					break;
				default:
					/*
					 * We don't support this control, so STALL the
					 * control ep.
					 */
					resp->length = -EL2HLT;
					/*
					 * For every unsupported control request
					 * set the request error code to appropriate
					 * value.
					 */
					dev->request_error_code.data[0] = 0x07;
					dev->request_error_code.length = 1;
					break;
			}
			break;
		case UVC_CT_IRIS_ABSOLUTE_CONTROL:
			switch (req)
			{
				case UVC_GET_INFO:
				case UVC_GET_MIN:
				case UVC_GET_MAX:
				case UVC_GET_CUR:
				case UVC_GET_DEF:
				case UVC_GET_RES:
					resp->data[0] = 1;
					resp->length = 1;
			
					dev->request_error_code.data[0] = 0x00;
					dev->request_error_code.length = 1;
					break;
				default:
					/*
					 * We don't support this control, so STALL the
					 * control ep.
					 */
					resp->length = -EL2HLT;
					/*
					 * For every unsupported control request
					 * set the request error code to appropriate
					 * value.
					 */
					dev->request_error_code.data[0] = 0x07;
					dev->request_error_code.length = 1;
			}
			break;		
		case UVC_CT_EXPOSURE_TIME_ABSOLUTE_CONTROL:
			switch (req)
			{
				case UVC_SET_CUR:
					resp->length = 4;
					resp->data[0] = 0x0;
					break;
				case UVC_GET_INFO:
					resp->data[0] = 0xf;
					resp->length = 1;
					break;
				case UVC_GET_MIN:
					resp->length = 4;
					resp->data[0] = 0x0a;
					resp->data[1] = 0x0;
					resp->data[2] = 0x0;
					resp->data[3] = 0x0;
					break;
				case UVC_GET_MAX:
					resp->length = 4;
					resp->data[0] = (2000)&0xff;
					resp->data[1] = (2000>>8)&0xff;
					resp->data[2] = 0x0;
					resp->data[3] = 0x0;
					break;
				case UVC_GET_CUR:
					resp->length = 4;
					resp->data[0] = (2000 & 0xff);
					resp->data[1] = ((2000 >> 8) & 0xff);
					resp->data[2] = ((2000 >> 16) & 0xff);
					resp->data[3] = ((2000 >> 24) & 0xff);
					break;
				case UVC_GET_DEF:
					resp->data[0] = 0x64;
					resp->length = 4;
					break;
				case UVC_GET_RES:
					resp->data[0] = 0x0a;
					resp->length = 4;
					break;
				default:
					/*
					 * We don't support this control, so STALL the
					 * control ep.
					 */
					resp->length = -EL2HLT;
					/*
					 * For every unsupported control request
					 * set the request error code to appropriate
					 * value.
					 */
					dev->request_error_code.data[0] = 0x07;
					dev->request_error_code.length = 1;
			}
			break;
		default:
			/*
			 * We don't support this control, so STALL the control
			 * ep.
			 */
			resp->length = -EL2HLT;
			/*
			 * If we were not supposed to handle this
			 * 'cs', prepare a Request Error Code response.
			 */
			dev->request_error_code.data[0] = 0x06;
			dev->request_error_code.length = 1;
			break;
	}

}

static void uvc_event_pu_control(struct uvc_device *dev, uint8_t req, uint8_t unit_id, uint8_t cs, struct uvc_request_data *resp)
{
	printf("func = %s, line = %d, cs = %d, req = %d\n", __FUNCTION__, __LINE__, cs, req);
	switch (cs)
	{
		case UVC_PU_BRIGHTNESS_CONTROL:
			switch (req)
			{
				case UVC_SET_CUR:
					resp->data[0] = 0x0;
					resp->length = 2;
					/*
					 * For every successfully handled control
					 * request set the request error code to no
					 * error
					 */
					dev->request_error_code.data[0] = 0x00;
					dev->request_error_code.length = 1;
					break;
				case UVC_GET_MIN:
					resp->data[0] = 0;
					resp->length = 2;
					/*
					 * For every successfully handled control
					 * request set the request error code to no
					 * error
					 */
					dev->request_error_code.data[0] = 0x00;
					dev->request_error_code.length = 1;
					break;
				case UVC_GET_MAX:
					resp->data[0] = 0x64;
					resp->length = 2;
					/*
					 * For every successfully handled control
					 * request set the request error code to no
					 * error
					 */
					dev->request_error_code.data[0] = 0x00;
					dev->request_error_code.length = 1;
					break;
				case UVC_GET_CUR:
					resp->data[0] = 0x32;
					resp->length = 2;
					/*
					 * For every successfully handled control
					 * request set the request error code to no
					 * error
					 */
					dev->request_error_code.data[0] = 0x00;
					dev->request_error_code.length = 1;
					break;
				case UVC_GET_INFO:
					/*
					 * We support Set and Get requests and don't
					 * support async updates on an interrupt endpt
					 */
					resp->data[0] = 0x03;
					resp->length = 1;
					/*
					 * For every successfully handled control
					 * request, set the request error code to no
					 * error.
					 */
					dev->request_error_code.data[0] = 0x00;
					dev->request_error_code.length = 1;
					break;
				case UVC_GET_DEF:
					resp->data[0] = 0x32;
					resp->length = 2;
					/*
					 * For every successfully handled control
					 * request, set the request error code to no
					 * error.
					 */
					dev->request_error_code.data[0] = 0x00;
					dev->request_error_code.length = 1;
					break;
				case UVC_GET_RES:
					resp->data[0] = 0x1;
					resp->length = 2;
					/*
					 * For every successfully handled control
					 * request, set the request error code to no
					 * error.
					 */
					dev->request_error_code.data[0] = 0x00;
					dev->request_error_code.length = 1;
					break;
				default:
					/*
					 * We don't support this control, so STALL the
					 * default control ep.
					 */
					resp->length = -EL2HLT;
					/*
					 * For every unsupported control request
					 * set the request error code to appropriate
					 * code.
					 */
					dev->request_error_code.data[0] = 0x07;
					dev->request_error_code.length = 1;
					break;
			}
			break;
        case UVC_PU_CONTRAST_CONTROL:
            switch (req)
            {
	            case UVC_SET_CUR:
	                resp->data[0] = 0x0;
	                resp->length = 2;
	                dev->request_error_code.data[0] = 0x00;
	                dev->request_error_code.length = 1;
	                break;
	            case UVC_GET_MIN:
	                resp->data[0] = 0x0;
	                resp->length = 2;
	                dev->request_error_code.data[0] = 0x00;
	                dev->request_error_code.length = 1;
	                break;
	            case UVC_GET_MAX:
	                resp->data[0] = 0x64;
	                resp->length = 2;
	                dev->request_error_code.data[0] = 0x00;
	                dev->request_error_code.length = 1;
	                break;
	            case UVC_GET_CUR:
	                resp->length = 2;
					resp->data[0] = 0x32;
	                dev->request_error_code.data[0] = 0x00;
	                dev->request_error_code.length = 1;
	                break;
	            case UVC_GET_INFO:
	                resp->data[0] = 0x03;
	                resp->length = 1;
	                dev->request_error_code.data[0] = 0x00;
	                dev->request_error_code.length = 1;
	                break;
	            case UVC_GET_DEF:
					resp->data[0] = 0x32;
	                resp->length = 2;
	                dev->request_error_code.data[0] = 0x00;
	                dev->request_error_code.length = 1;
	                break;
	            case UVC_GET_RES:
	                resp->data[0] = 0x1;
	                resp->length = 2;
	                dev->request_error_code.data[0] = 0x00;
	                dev->request_error_code.length = 1;
	                break;
	            default:
	                resp->length = -EL2HLT;
	                dev->request_error_code.data[0] = 0x07;
	                dev->request_error_code.length = 1;
	                break;
            }
            break;
        case UVC_PU_HUE_CONTROL:
            switch (req)
            {
            case UVC_SET_CUR:
                resp->data[0] = 0x0;
                resp->length = 2;
                dev->request_error_code.data[0] = 0x00;
                dev->request_error_code.length = 1;
                break;
            case UVC_GET_MIN:
                resp->data[0] = 0x0;
                resp->length = 2;
                dev->request_error_code.data[0] = 0x00;
                dev->request_error_code.length = 1;
                break;
            case UVC_GET_MAX:
                resp->data[0] = 0x64;
                resp->length = 2;
                dev->request_error_code.data[0] = 0x00;
                dev->request_error_code.length = 1;
                break;
            case UVC_GET_CUR:
                resp->length = 2;
				resp->data[0] = 0x32;
                dev->request_error_code.data[0] = 0x00;
                dev->request_error_code.length = 1;
                break;
            case UVC_GET_INFO:
                resp->data[0] = 0x03;
                resp->length = 1;
                dev->request_error_code.data[0] = 0x00;
                dev->request_error_code.length = 1;
                break;
            case UVC_GET_DEF:
                resp->data[0] = 0x0;
                resp->length = 2;
                dev->request_error_code.data[0] = 0x00;
                dev->request_error_code.length = 1;
                break;
            case UVC_GET_RES:
                resp->data[0] = 0x1;
                resp->length = 2;
                dev->request_error_code.data[0] = 0x00;
                dev->request_error_code.length = 1;
                break;
            default:
                resp->length = -EL2HLT;
                dev->request_error_code.data[0] = 0x07;
                dev->request_error_code.length = 1;
                break;
            }
            break;
        case UVC_PU_SATURATION_CONTROL:
            switch (req)
            {
	            case UVC_SET_CUR:
	                resp->data[0] = 0x0;
	                resp->length = 2;
	                dev->request_error_code.data[0] = 0x00;
	                dev->request_error_code.length = 1;
	                break;
	            case UVC_GET_MIN:
	                resp->data[0] = 0x0;
	                resp->length = 2;
	                dev->request_error_code.data[0] = 0x00;
	                dev->request_error_code.length = 1;
	                break;
	            case UVC_GET_MAX:
	                resp->data[0] = 0x64;
	                resp->length = 2;
	                dev->request_error_code.data[0] = 0x00;
	                dev->request_error_code.length = 1;
	                break;
	            case UVC_GET_CUR:
	                resp->length = 2;
					resp->data[0] = 0x32;
	                dev->request_error_code.data[0] = 0x00;
	                dev->request_error_code.length = 1;
	                break;
	            case UVC_GET_INFO:
	                resp->data[0] = 0x03;
	                resp->length = 1;
	                dev->request_error_code.data[0] = 0x00;
	                dev->request_error_code.length = 1;
	                break;
	            case UVC_GET_DEF:
	                resp->data[0] = 0x32;
	                resp->length = 2;
	                dev->request_error_code.data[0] = 0x00;
	                dev->request_error_code.length = 1;
	                break;
	            case UVC_GET_RES:
	                resp->data[0] = 0x1;
	                resp->length = 2;
	                dev->request_error_code.data[0] = 0x00;
	                dev->request_error_code.length = 1;
	                break;
	            default:
	                resp->length = -EL2HLT;
	                dev->request_error_code.data[0] = 0x07;
	                dev->request_error_code.length = 1;
	                break;
            }
            break;
        case UVC_PU_SHARPNESS_CONTROL:
            switch (req)
            {
	            case UVC_SET_CUR:
	                resp->data[0] = 0x0;
	                resp->length = 2;
	                dev->request_error_code.data[0] = 0x00;
	                dev->request_error_code.length = 1;
	                break;
	            case UVC_GET_MIN:
	                resp->data[0] = 0x0;
	                resp->length = 2;
	                dev->request_error_code.data[0] = 0x00;
	                dev->request_error_code.length = 1;
	                break;
	            case UVC_GET_MAX:
	                resp->data[0] = 0x64;
	                resp->length = 2;
	                dev->request_error_code.data[0] = 0x00;
	                dev->request_error_code.length = 1;
	                break;
	            case UVC_GET_CUR:
	                resp->length = 2;
					resp->data[0] = 0x32;
	                dev->request_error_code.data[0] = 0x00;
	                dev->request_error_code.length = 1;
	                break;
	            case UVC_GET_INFO:
	                resp->data[0] = 0x03;
	                resp->length = 1;
	                dev->request_error_code.data[0] = 0x00;
	                dev->request_error_code.length = 1;
	                break;
	            case UVC_GET_DEF:
					resp->data[0] = 0x32;
	                resp->length = 2;
	                dev->request_error_code.data[0] = 0x00;
	                dev->request_error_code.length = 1;
	                break;
	            case UVC_GET_RES:
	                resp->data[0] = 0x1;
	                resp->length = 2;
	                dev->request_error_code.data[0] = 0x00;
	                dev->request_error_code.length = 1;
	                break;
	            default:
	                resp->length = -EL2HLT;
	                dev->request_error_code.data[0] = 0x07;
	                dev->request_error_code.length = 1;
	                break;
            }
            break;
        case UVC_PU_WHITE_BALANCE_COMPONENT_AUTO_CONTROL:
            //break;
        case UVC_PU_WHITE_BALANCE_COMPONENT_CONTROL:
            //break;
        case UVC_PU_WHITE_BALANCE_TEMPERATURE_CONTROL:
            //break;
        case UVC_PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL:
            //break;
        case UVC_PU_POWER_LINE_FREQUENCY_CONTROL:
            //break;
        default:
            resp->length = -EL2HLT;
            dev->request_error_code.data[0] = 0x06;
            dev->request_error_code.length = 1;
            break;
    }

}


static void uvc_event_ext_control_6(struct uvc_device *dev, uint8_t req, uint8_t unit_id, uint8_t cs, struct uvc_request_data *resp)
{
	printf("func = %s, line = %d, cs = %d, req = %d\n", __FUNCTION__, __LINE__, cs, req);
    switch (cs)
    {
        case CMD_GET_CAMERA_VERSION:
            switch (req)
            {
	            case UVC_GET_LEN:
	                memset(resp->data, 0, sizeof(resp->data));
	                resp->data[0] = sizeof(resp->data);
	                resp->length = 2;
	                dev->request_error_code.data[0] = 0x00;
	                dev->request_error_code.length = 1;
	                break;
	            case UVC_SET_CUR:
	            case UVC_GET_MIN:
	            case UVC_GET_MAX:
	            case UVC_GET_CUR:
	            case UVC_GET_INFO:
	            case UVC_GET_DEF:
	                memset(resp->data, 0, sizeof(resp->data));
	                resp->data[0] = 0;
	                resp->length = 2;
	                dev->request_error_code.data[0] = 0x00;
	                dev->request_error_code.length = 1;
	                break;
	            case UVC_GET_RES:
	                memset(resp->data, 0, sizeof(resp->data));
	                resp->data[0] = 1;
	                resp->length = 2;
	                dev->request_error_code.data[0] = 0x00;
	                dev->request_error_code.length = 1;
	                break;
	            default:
	                resp->length = -EL2HLT;
	                dev->request_error_code.data[0] = 0x07;
	                dev->request_error_code.length = 1;
	                break;
            }
            break;

        case CMD_SET_CAMERA_IP:
            switch (req)
            {
	            case UVC_GET_LEN:
	                memset(resp->data, 0, sizeof(resp->data));
	                resp->data[0] = sizeof(resp->data);
	                resp->length = 2;
	                dev->request_error_code.data[0] = 0x00;
	                dev->request_error_code.length = 1;
	                break;
	            case UVC_SET_CUR:
	            case UVC_GET_MIN:
	            case UVC_GET_MAX:
	            case UVC_GET_CUR:
	            case UVC_GET_INFO:
	                resp->data[0] = 0x03;
	                resp->length = 1;
	                dev->request_error_code.data[0] = 0x00;
	                dev->request_error_code.length = 1;
	                break;
	            case UVC_GET_DEF:
	                memset(resp->data, 0, sizeof(resp->data));
	                resp->data[0] = 0;
	                resp->length = 2;
	                dev->request_error_code.data[0] = 0x00;
	                dev->request_error_code.length = 1;
	                break;
	            case UVC_GET_RES:
	                memset(resp->data, 0, sizeof(resp->data));
	                resp->data[0] = 1;
	                resp->length = 2;
	                dev->request_error_code.data[0] = 0x00;
	                dev->request_error_code.length = 1;
	                break;
	            default:
	                resp->length = -EL2HLT;
	                dev->request_error_code.data[0] = 0x07;
	                dev->request_error_code.length = 1;
	                break;
            }
            break;
		case CMD_START_CAMERA:
			switch (req)
			{
	            case UVC_GET_LEN:
					//uvc_event_streamon(dev);
				case UVC_GET_INFO:
				case UVC_GET_MIN:
				case UVC_GET_MAX:
				case UVC_GET_CUR:
				case UVC_GET_DEF:
				case UVC_GET_RES:
					resp->data[0] = 1;
					resp->length = 2;
			
					dev->request_error_code.data[0] = 0x00;
					dev->request_error_code.length = 1;
					break;
				default:
					/*
					 * We don't support this control, so STALL the
					 * control ep.
					 */
					resp->length = -EL2HLT;
					/*
					 * For every unsupported control request
					 * set the request error code to appropriate
					 * value.
					 */
					dev->request_error_code.data[0] = 0x07;
					dev->request_error_code.length = 1;
			}
			break;		
        case CMD_SET_H265:
            switch (req)
            {
	            case UVC_GET_LEN:
	                memset(resp->data, 0, sizeof(resp->data));
	                resp->data[0] = sizeof(resp->data);
	                resp->length = 4;
	                dev->request_error_code.data[0] = 0x00;
	                dev->request_error_code.length = 1;
	                break;
	            case UVC_SET_CUR:
	                memset(resp->data, 0, sizeof(resp->data));
	                resp->data[0] = 0x0;
	                resp->length = 4;
	                dev->request_error_code.data[0] = 0x00;
	                dev->request_error_code.length = 1;
	                break;
	            case UVC_GET_MIN:
	                memset(resp->data, 0, sizeof(resp->data));
	                resp->length = 4;
	                dev->request_error_code.data[0] = 0x00;
	                dev->request_error_code.length = 1;
	                break;
	            case UVC_GET_MAX:
	                memset(resp->data, 0xFF, sizeof(resp->data));
	                resp->length = 4;
	                dev->request_error_code.data[0] = 0x00;
	                dev->request_error_code.length = 1;
	                break;
	            case UVC_GET_CUR:
	                resp->data[0] = 0x0;
	                resp->length = 4;
	                dev->request_error_code.data[0] = 0x00;
	                dev->request_error_code.length = 1;
	                break;
	            case UVC_GET_INFO:
	                resp->data[0] = 0x03;
	                resp->length = 1;
	                dev->request_error_code.data[0] = 0x00;
	                dev->request_error_code.length = 1;
	                break;
	            case UVC_GET_DEF:
	                memset(resp->data, 0, sizeof(resp->data));
	                resp->data[0] = 0;
	                resp->length = 4;
	                dev->request_error_code.data[0] = 0x00;
	                dev->request_error_code.length = 1;
	                break;
	            case UVC_GET_RES:
	                memset(resp->data, 0, sizeof(resp->data));
	                resp->data[0] = 1;
	                resp->length = 1;
	                dev->request_error_code.data[0] = 0x00;
	                dev->request_error_code.length = 1;
	                break;
	            default:
	                resp->length = -EL2HLT;
	                dev->request_error_code.data[0] = 0x07;
	                dev->request_error_code.length = 1;
	                break;
            }
            break;
        default:
            /*
             * We don't support this control, so STALL the control
             * ep.
             */
            resp->length = -EL2HLT;
            /*
             * If we were not supposed to handle this
             * 'cs', prepare a Request Error Code response.
             */
            dev->request_error_code.data[0] = 0x06;
            dev->request_error_code.length = 1;
            break;
    }

}


static void uvc_event_ext_control_a(struct uvc_device *dev, uint8_t req, uint8_t unit_id, uint8_t cs, struct uvc_request_data *resp)
{
	printf("func = %s, line = %d, cs = %d, req = %d\n", __FUNCTION__, __LINE__, cs, req);
    switch (cs)
    {
        case 0x09:	// UVCX_PICTURE_TYPE_CONTROL: USB_Video_Payload_H_264_1.0 3.3
			switch (req)
			{
				case UVC_SET_CUR:
					resp->length = 4;
					break;
				case UVC_GET_LEN:
					resp->data[0] = 0x04;
					resp->data[1] = 0x00;
					resp->length = 2;
					break;
				case UVC_GET_INFO:
					resp->data[0] = 0x03;
					resp->length = 1;
					break;
				default:
					break;
			}
            break;
        default:
			switch (req)
			{
				case UVC_GET_MIN:
					resp->length = 4;
					break;
				case UVC_GET_LEN:
					resp->data[0] = 0x04;
					resp->data[1] = 0x00;
					resp->length = 2;
					break;
				case UVC_GET_INFO:
					resp->data[0] = 0x03;
					resp->length = 1;
					break;
				case UVC_GET_CUR:
					resp->length = 1;
					resp->data[0] = 0x01;
					break;
				default:
					resp->length = 1;
					resp->data[0] = 0x06;
					break;
			}
            break;
    }

}



static void uvc_events_process_control(struct uvc_device*       dev, uint8_t  req, uint8_t unit_id, uint8_t cs, struct uvc_request_data *resp)
{
	printf("func = %s, line = %d, unit_id = %d, cs = %d\n", __FUNCTION__, __LINE__, unit_id, cs);
    switch (unit_id)
    {
        case 0:
            uvc_event_undefined_control(dev, req, cs,  resp);
            break;
        case 1:	/* Camera terminal unit 'UVC_VC_INPUT_TERMINAL'. */
            uvc_event_it_control(dev, req, unit_id, cs, resp);
            break;
        case 2:	/* processing unit 'UVC_VC_PROCESSING_UNIT' */
            uvc_event_pu_control(dev, req, unit_id, cs, resp);
            break;
        case 6:	/* Extension unit 'UVC_VC_Extension_Unit' for RK*/
            uvc_event_ext_control_6(dev, req, unit_id, cs, resp);
            break;
        case 10:/* Extension unit 'UVC_VC_Extension_Unit' for Hisi*/
            uvc_event_ext_control_a(dev, req, unit_id, cs, resp);
            break;
        default:
            dev->request_error_code.length = 1;
            dev->request_error_code.data[0] = 0x06;
    }
}

static void uvc_events_process_streaming(struct uvc_device* dev, uint8_t req, uint8_t cs, struct uvc_request_data* resp)
{
    struct uvc_streaming_control* ctrl;

	printf("func = %s, line = %d, cs = %d, req = %d\n", __FUNCTION__, __LINE__, cs, req);
    if ((cs != UVC_VS_PROBE_CONTROL) && (cs != UVC_VS_COMMIT_CONTROL)) {
        return;
    }

    ctrl = (struct uvc_streaming_control*)&resp->data;
    resp->length = sizeof * ctrl;

    //request
    switch (req) 
	{
	        //0x01
	    case UVC_SET_CUR:
	        dev->control = cs;
	        resp->length = 34;
	        break;
	        //0x81
	    case UVC_GET_CUR:
	        if (cs == UVC_VS_PROBE_CONTROL) {
	            memcpy(ctrl, &dev->probe, sizeof * ctrl);
	        } else {
	            memcpy(ctrl, &dev->commit, sizeof * ctrl);
	        }
	        break;
	        //0x82
	    case UVC_GET_MIN:
	        //0x83
	    case UVC_GET_MAX:
	        //uvc_fill_streaming_control(dev, ctrl, 0, 0);
	        //break;
	        //0x87
	    case UVC_GET_DEF:
	        uvc_fill_streaming_control(dev, ctrl, req == UVC_GET_MAX ? -1 : 0, req == UVC_GET_MAX ? -1 : 0);
	        break;
	        //0x84
	    case UVC_GET_RES:
	        memset(ctrl, 0, sizeof * ctrl);
	        break;
	        //0x85
	    case UVC_GET_LEN:
	        resp->data[0] = 0x00;
	        resp->data[1] = 0x22;
	        resp->length = 2;
	        break;
	        //0x86
	    case UVC_GET_INFO:
	        resp->data[0] = 0x03;
	        resp->length = 1;
	        break;
    }
}
										 

static void set_probe_status(struct uvc_device* dev, int cs, int req)
{
    if (cs == 0x01) 
	{
        switch (req) 
		{
	        case 0x01:
		        {
		            dev->probe_status.set = 1;
		        }
	            break;
	        case 0x81:
		        {
		            dev->probe_status.get = 1;
		        }
	            break;
	        case 0x82:
		        {
		            dev->probe_status.min = 1;
		        }
	            break;
	        case 0x83:
		        {
		            dev->probe_status.max = 1;
		        }
	            break;
	        case 0x84:
		        {}
	            break;
	        case 0x85:
		        {}
	            break;
	        case 0x86:
		        {}
	            break;
        }
    }
}

static int check_probe_status(struct uvc_device* dev)
{
	return 1;	// ??????
    if ((dev->probe_status.get == 1) && (dev->probe_status.set == 1) && (dev->probe_status.min == 1) && (dev->probe_status.max == 1)) {
        return 1;
    }

	printf("func = %s, line = %d, the probe status is not correct...\n", __FUNCTION__, __LINE__);

    return 0;
}

static void uvc_events_process_class(struct uvc_device* dev, struct usb_ctrlrequest* ctrl,
                                     struct uvc_request_data* resp)
{
    unsigned char probe_status = 1;

	printf("func = %s, line = %d\n", __FUNCTION__, __LINE__);
    if (probe_status) {
        unsigned char type = ctrl->bRequestType & USB_RECIP_MASK;
        switch (type)
        {
			case USB_RECIP_INTERFACE:
				printf("func = %s, line = %d, request type :DEVICE\n", __FUNCTION__, __LINE__);
				set_probe_status(dev, (ctrl->wValue >> 8), ctrl->bRequest);
				break;
			case USB_RECIP_DEVICE:
				printf("func = %s, line = %d, request type :DEVICE\n", __FUNCTION__, __LINE__);
				break;
			case USB_RECIP_ENDPOINT:
				printf("func = %s, line = %d, request type :ENDPOINT\n", __FUNCTION__, __LINE__);
				break;
			case USB_RECIP_OTHER:
				printf("func = %s, line = %d, request type :OTHER\n", __FUNCTION__, __LINE__);
				break;
			}
    }

    if ((ctrl->bRequestType & USB_RECIP_MASK) != USB_RECIP_INTERFACE) {
        return;
    }

    //save unit id, interface id and control selector
    dev->control = (ctrl->wValue >> 8);
    dev->unit_id = ((ctrl->wIndex & 0xff00) >> 8);
    dev->interface_id = (ctrl->wIndex & 0xff);

    switch (ctrl->wIndex & 0xff)
    {
	        //0x0   0x100
	    case UVC_INTF_CONTROL:
	        uvc_events_process_control(dev, ctrl->bRequest, ctrl->wIndex >> 8, ctrl->wValue >> 8, resp);
	        break;

	        //0x1
	    case UVC_INTF_STREAMING:
			printf("func = %s, line = %d =========UVC_INTF_STREAMING========\n", __FUNCTION__, __LINE__);
	        uvc_events_process_streaming(dev, ctrl->bRequest, ctrl->wValue >> 8, resp);
	        break;

	    default:
	        break;
    }
}

static void uvc_events_process_standard(struct uvc_device* dev, struct usb_ctrlrequest* ctrl,
                                        struct uvc_request_data* resp)
{
	printf("func = %s, line = %d\n", __FUNCTION__, __LINE__);
    (void)dev;
    (void)ctrl;
    (void)resp;
}

static void uvc_events_process_setup(struct uvc_device* dev, struct usb_ctrlrequest* ctrl,
                                     struct uvc_request_data* resp)
{
    dev->control = 0;
    dev->unit_id = 0;
    dev->interface_id = 0;

	printf("func = %s, line = %d\n", __FUNCTION__, __LINE__);
    switch (ctrl->bRequestType & USB_TYPE_MASK)
    {
    case USB_TYPE_STANDARD:
        uvc_events_process_standard(dev, ctrl, resp);
        break;

    case USB_TYPE_CLASS:
        uvc_events_process_class(dev, ctrl, resp);
        break;

    default:
        break;
    }
}



static void uvc_event_it_data(struct uvc_device *dev, int unit_id, int control, struct uvc_request_data *data)
{
    switch(control)
    {
        case UVC_CT_AE_MODE_CONTROL:
            break;
        case UVC_CT_EXPOSURE_TIME_ABSOLUTE_CONTROL:
            break;
        default:
            break;
    }
}

static void uvc_event_pu_data(struct uvc_device *dev, int unit_id, int control, struct uvc_request_data *data)
{
    switch(control)
	{
        case UVC_PU_BRIGHTNESS_CONTROL:
            break;
        case UVC_PU_CONTRAST_CONTROL:
            break;
        case UVC_PU_HUE_CONTROL:
            break;
        case UVC_PU_SATURATION_CONTROL:
            break;
        case UVC_PU_SHARPNESS_CONTROL:
            break;
        default:
            break;
    }
}


static void uvc_events_process_control_data(struct uvc_device *dev, struct uvc_request_data *data)
{
	printf("func = %s, line = %d\n", __FUNCTION__, __LINE__);
    switch (dev->unit_id)
    {
        case 1:
            uvc_event_it_data(dev,dev->unit_id, dev->control, data);
            break;
        case 2:
            uvc_event_pu_data(dev,dev->unit_id, dev->control, data);
            break;
        case 10:
            //histream_event_eu_h264_data(dev->unit_id, dev->control, data);
            break;
        default:
            break;
    }
}

/*
 * This function is called in response to either:
 *  - A SET_ALT(interface 1, alt setting 1) command from USB host,
 *    if the UVC gadget supports an ISOCHRONOUS video streaming endpoint
 *    or,
 *
 *  - A UVC_VS_COMMIT_CONTROL command from USB host, if the UVC gadget
 *    supports a BULK type video streaming endpoint.
 */
static int uvc_event_streamon(struct uvc_device *dev)
{
	printf("func = %s, line = %d\n", __FUNCTION__, __LINE__);
    if (uvc_reqbufs(dev) < 0) {
		printf("func = %s, line = %d uvc_reqbufs error.\n", __FUNCTION__, __LINE__);
        return -1;
    } 
    if (uvc_qbuf(dev) < 0) {
		printf("func = %s, line = %d uvc_qbuf error.\n", __FUNCTION__, __LINE__);
        return -1;
    } 
	uvc_streamon(dev);

	return 0;
}

static void uvc_events_process_data(struct uvc_device* dev, struct uvc_request_data* data)
{
    struct uvc_streaming_control* target;
    struct uvc_streaming_control* ctrl;
    const struct uvc_format_info* format;
    const struct uvc_frame_info* frame;
    const unsigned int* interval;
    unsigned int iformat, iframe;
    unsigned int nframes;

	printf("func = %s, line = %d\n", __FUNCTION__, __LINE__);
    if ((dev->unit_id != 0) && (dev->interface_id == UVC_INTF_CONTROL))
    {
        return uvc_events_process_control_data(dev, data);
    }

    switch (dev->control)
    {
        case UVC_VS_PROBE_CONTROL:
		   printf("func = %s, line = %d\n", __FUNCTION__, __LINE__);
            target = &dev->probe;
            break;

        case UVC_VS_COMMIT_CONTROL:
			printf("func = %s, line = %d\n", __FUNCTION__, __LINE__);
            target = &dev->commit;
            break;

        default:
			printf("func = %s, line = %d\n", __FUNCTION__, __LINE__);
            return;
    }

    ctrl = (struct uvc_streaming_control*)&data->data;
    iformat = clamp((unsigned int)ctrl->bFormatIndex, 1U, (unsigned int)ARRAY_SIZE(uvc_formats));
    format = &uvc_formats[iformat - 1];
    nframes = 0;

    printf("format->frames[nframes].width: %d\n", format->frames[nframes].width);
    printf("format->frames[nframes].height: %d\n", format->frames[nframes].height);

    while (format->frames[nframes].width != 0)
    {
        ++nframes;
    }

    iframe = clamp((unsigned int)ctrl->bFrameIndex, 1U, nframes);
    frame = &format->frames[iframe - 1];
    interval = frame->intervals;

    while ((interval[0] < ctrl->dwFrameInterval) && interval[1])
    {
        ++interval;
    }

    target->bFormatIndex = iformat;
    target->bFrameIndex = iframe;

    switch (format->fcc)
    {
        case V4L2_PIX_FMT_YUYV:
            target->dwMaxVideoFrameSize = frame->width * frame->height * 2;
            break;
        case V4L2_PIX_FMT_YUV420:
            target->dwMaxVideoFrameSize = frame->width * frame->height * 1.5;
            break;

        case V4L2_PIX_FMT_MJPEG:
        case V4L2_PIX_FMT_H264:
            target->dwMaxVideoFrameSize = frame->width * frame->height * 2;
            break;
    }

    target->dwFrameInterval = *interval;

    #if 1
	printf("set interval=%d format=%d frame=%d dwMaxPayloadTransferSize=%d ctrl->dwMaxPayloadTransferSize = %d\n",
	target->dwFrameInterval,
	target->bFormatIndex,
	target->bFrameIndex,
	target->dwMaxPayloadTransferSize,
	ctrl->dwMaxPayloadTransferSize);
    #endif

    if ((dev->control == UVC_VS_COMMIT_CONTROL) && check_probe_status(dev)) {
        dev->pix_fmt    = format->fcc;
        dev->width  = frame->width;
        dev->height = frame->height;


		printf("func = %s, line = %d, width=%d height=%d\n", __FUNCTION__, __LINE__, dev->width, dev->height);
        uvc_set_format(dev);

        if (dev->bulk != 0)
        {
        	uvc_streamoff(dev);
            uvc_event_streamon(dev);
        }
    }

    if (dev->control == UVC_VS_COMMIT_CONTROL)
    {
        memset(&dev->probe_status, 0, sizeof (dev->probe_status));
    }
}


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
			uvc_events_process_setup(dev, &uvc_event->req, &resp);
			break;

			//0x08000005
		case UVC_EVENT_DATA:
			 printf("handle data event\n");
			uvc_events_process_data(dev, &uvc_event->data);
			return;

			//0x08000002
		case UVC_EVENT_STREAMON:
			printf("UVC_EVENT_STREAMON\n");
			uvc_event_streamon(dev);
			break;

			//0x08000003
		case UVC_EVENT_STREAMOFF:
			printf("UVC_EVENT_STREAMOFF\n");
			uvc_streamoff(dev);
			break;
        default:
			printf("func = %s, line = %d\n", __FUNCTION__, __LINE__);

        return;
    }

    ret = ioctl(dev->fd, UVCIOC_SEND_RESPONSE, &resp);
    if (ret < 0)
    {
        printf("UVCIOC_S_EVENT failed: %s (%d)\n", strerror(errno), errno);
        return;
    }
}

static void uvc_fill_streaming_control(struct uvc_device* dev, struct uvc_streaming_control* ctrl, int iframe, int iformat)
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
    ctrl->bFormatIndex = iformat;// + 1; // 1 is yuv ,2 is mjpeg
    ctrl->bFrameIndex = iframe;// + 1; //360 1 720 2
    ctrl->dwFrameInterval = frame->intervals[0]; //dui ying di ji ge zhenlv

    switch (format->fcc)
    {
	    case V4L2_PIX_FMT_YUYV:
	    case V4L2_PIX_FMT_YUV420:
			case V4L2_PIX_FMT_MJPEG:
	        ctrl->dwMaxVideoFrameSize = frame->width * frame->height * 2;
	        break;
	    case V4L2_PIX_FMT_H264:
	    case V4L2_PIX_FMT_H265:
	        ctrl->dwMaxVideoFrameSize = frame->width * frame->height * 2;
	        break;
    }

    if (dev->bulk) {
        ctrl->dwMaxPayloadTransferSize = 1843200;   /* TODO this should be filled by the driver. */
    } else {
        ctrl->dwMaxPayloadTransferSize = 3072;
    }
    ctrl->bmFramingInfo = 3;
    ctrl->bPreferedVersion = 1;
    ctrl->bMaxVersion = 1;

}

static void uvc_events_init(struct uvc_device* dev)
{
    struct v4l2_event_subscription sub;

    uvc_fill_streaming_control(dev, &dev->probe, 0, 2);
    uvc_fill_streaming_control(dev, &dev->commit, 0, 2);
    if (dev->bulk)
    {
        /* FIXME Crude hack, must be negotiated with the driver. */
        dev->probe.dwMaxPayloadTransferSize  = dev->image_size;
        dev->commit.dwMaxPayloadTransferSize = dev->image_size;
    }

    memset(&sub, 0, sizeof sub);
    sub.type = UVC_EVENT_SETUP;
    ioctl(dev->fd, VIDIOC_SUBSCRIBE_EVENT, &sub);
    sub.type = UVC_EVENT_DATA;
    ioctl(dev->fd, VIDIOC_SUBSCRIBE_EVENT, &sub);
    sub.type = UVC_EVENT_STREAMON;
    ioctl(dev->fd, VIDIOC_SUBSCRIBE_EVENT, &sub);
    sub.type = UVC_EVENT_STREAMOFF;
    ioctl(dev->fd, VIDIOC_SUBSCRIBE_EVENT, &sub);
	sub.type = UVC_EVENT_CONNECT;
	ioctl(dev->fd, VIDIOC_SUBSCRIBE_EVENT, &sub);
	sub.type = UVC_EVENT_DISCONNECT;
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
		buf.length = dev->image_size;
		if (dev->type == V4L2_BUF_TYPE_VIDEO_OUTPUT) {
			uvc_video_fill_buffer_userptr(dev, &buf);
		}
        ret = ioctl (dev->fd, VIDIOC_QBUF, &buf);
        if (ret < 0) {
            printf("Unable to Queue buffer (%d).\n", errno);
            return -1;
        }
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
	fmt.fmt.pix.sizeimage = dev->image_size;

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
	//printf("func = %s, line = %d\n", __FUNCTION__, __LINE__);

    if ((ret = ioctl(dev->fd, VIDIOC_DQBUF, &buf)) < 0)
    {
        printf("Unable to dq buffer: %s (%d).\n", strerror(errno), errno);
        return ret;
    }

    uvc_video_fill_buffer_userptr(dev, &buf);
	static n = 0; if (n++ % 100 == 0) printf("func = %s, line = %d, frame size = %d\n", __FUNCTION__, __LINE__, buf.bytesused);

    if ((ret = ioctl(dev->fd, VIDIOC_QBUF, &buf)) < 0)
    {
        printf("Unable to requeue buffer: %s (%d).\n", strerror(errno), errno);
        return ret;
    }

    return 0;
}


static void *uvc_events_run_thd(void *arg)
{
	pthread_detach(pthread_self());
	
	struct uvc_device *dev;
    fd_set efds;
    struct timeval tv;
    int ret;

	dev = (struct uvc_device *)arg;
	while (1)
	{
		tv.tv_sec  = 1;
		tv.tv_usec = 0;
		FD_ZERO(&efds);
		FD_SET(dev->fd, &efds);		// 要放while里面，不然会出现某些包收不到
		ret = select(dev->fd + 1, NULL, NULL, &efds, &tv);
		if (ret > 0) {
			//printf("func = %s, line = %d, selected\n", __FUNCTION__, __LINE__);
			if (FD_ISSET(dev->fd, &efds)) {
				uvc_events_process(dev);
			}
		} else {
			//printf("func = %s, line = %d, FD_ISSET else\n", __FUNCTION__, __LINE__);
			//usleep(200000);
		}
	}
	printf("func = %s, line = %d, exit !!!\n", __FUNCTION__, __LINE__);
	
	return NULL;
}


static void *uvc_data_run_thd(void *arg)
{
	pthread_detach(pthread_self());
	
	struct uvc_device *dev;
    fd_set wfds;
    struct timeval tv;
    int ret;

	dev = (struct uvc_device *)arg;
	while (1)
	{
		if (dev->streaming != 1) {
			usleep(200000);
			continue;
		}
	
		tv.tv_sec  = 1;
		tv.tv_usec = 0;
		FD_ZERO(&wfds);
		FD_SET(dev->fd, &wfds);
		ret = select(dev->fd + 1, NULL, &wfds, NULL, &tv);
		if (ret > 0) {
			//printf("func = %s, line = %d, selected\n", __FUNCTION__, __LINE__);
			if (FD_ISSET(dev->fd, &wfds))
			{
				ret = uvc_data_process_userptr(dev);
				if (ret < 0) {
					usleep(500000);
					printf("func = %s, line = %d, uvc_data_process_userptr error.\n", __FUNCTION__, __LINE__);
					//break;
				}
			}
		} else {
			printf("func = %s, line = %d, FD_ISSET else\n", __FUNCTION__, __LINE__);
			usleep(200000);
		}
	}
	printf("func = %s, line = %d, exit !!!\n", __FUNCTION__, __LINE__);
	
	return NULL;
}


int uvc_streamon(struct uvc_device *dev)
{
    int type, ret;

	printf("func = %s, line = %d\n", __FUNCTION__, __LINE__);
	type = dev->type;
    ret = ioctl (dev->fd, VIDIOC_STREAMON, &type);
    if (ret < 0) {
        printf("Unable to %s stream on: %d.\n", "start", errno);
        return ret;
    }
    dev->streaming = 1;

    return 0;
}

int uvc_streamoff(struct uvc_device *dev)
{
    int type, ret;

	printf("func = %s, line = %d\n", __FUNCTION__, __LINE__);
    dev->streaming = 0;
	type = dev->type;
    ret = ioctl (dev->fd, VIDIOC_STREAMOFF, &type);
    if (ret < 0) {
        printf("Unable to %s stream off: %d.\n", "stop", errno);
        return ret;
    }

    return 0;
}


int uvc_close(struct uvc_device *dev)
{
    int i;
	
    if (dev->type == V4L2_BUF_TYPE_VIDEO_OUTPUT) {
		uvc_video_deinit(dev);
	}
	
    if (dev->streaming != 0) {
        uvc_streamoff(dev);
		dev->streaming = 3;
		usleep(200000);
    }
    
    if (dev->type == V4L2_BUF_TYPE_VIDEO_CAPTURE) {
	    for (i = 0; i < dev->nbufs; i++) {
	        munmap(dev->mem[i], dev->length[i]);
	    }
	}
    close(dev->fd);
	free(dev);
    
    return 0;
}


struct uvc_device *uvc_open(const char *devpath, struct uvc_devattr *devattr)
{
    struct uvc_device *dev = NULL;
    
    dev = (struct uvc_device*)calloc(1, sizeof(struct uvc_device));
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
        //uvc_set_format(dev);
    } else {
		printf("func = %s, line = %d error.\n", __FUNCTION__, __LINE__);
		return NULL;
	}
	
	dev->max_width = 1920;
	dev->max_height = 1080;
    switch (dev->pix_fmt)
    {
	    case V4L2_PIX_FMT_YUYV:
			dev->image_size = dev->max_width * dev->max_height * 2;
			break;
	    case V4L2_PIX_FMT_YUV420:
		case V4L2_PIX_FMT_MJPEG:
			dev->image_size = dev->max_width * dev->max_height * 1.5;
			break;
	    case V4L2_PIX_FMT_H264:
	    case V4L2_PIX_FMT_H265:
	        dev->image_size = dev->max_width * dev->max_height / 3;
	        break;
    }
	
    dev->nbufs = 4;
	dev->bulk = 0;
	uvc_events_init(dev);
	if (dev->type == V4L2_BUF_TYPE_VIDEO_OUTPUT) {
		uvc_video_init(dev);
	}
    
    if (dev->type == V4L2_BUF_TYPE_VIDEO_CAPTURE && uvc_mmapbuf(dev) < 0) {
        printf("uvc_mmapbuf error!\n");
        //return NULL;
    } 


	if (dev->type == V4L2_BUF_TYPE_VIDEO_OUTPUT) {
		pthread_t tid1, tid2;
		pthread_create(&tid1, 0, uvc_events_run_thd, (void *)dev);
		pthread_t tid;
		pthread_create(&tid2, 0, uvc_data_run_thd, (void *)dev);
	}


    return dev;
}

