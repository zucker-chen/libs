#ifndef _UVC_VIDEO_H_
#define _UVC_VIDEO_H_




#ifdef __cplusplus
#if __cplusplus
extern "C"
{
#endif
#endif /* __cplusplus */




int uvc_video_fill_buffer_userptr(struct uvc_device* dev, struct v4l2_buffer* buf);
int uvc_video_init(struct uvc_device* dev);
int uvc_video_deinit(struct uvc_device* dev);









#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /*_UVC_VIDEO_H_ */

