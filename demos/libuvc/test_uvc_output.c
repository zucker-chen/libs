


#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include "uvc_gadget.h"


int main (int argc, char *argv[])
{
    struct uvc_device *dev = NULL;
    struct uvc_devattr  devattr;
	char video_dev_name[32];
	int video_id = -1;
    
    devattr.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    devattr.width = 1920;
    devattr.height = 1080;
    devattr.pix_fmt = V4L2_PIX_FMT_MJPEG;//V4L2_PIX_FMT_MJPEG;//V4L2_PIX_FMT_H264;
    video_id = uvc_video_id_get();
	if (video_id >= 0) {
		snprintf(video_dev_name, sizeof(video_dev_name), "/dev/video%d", video_id);
	    dev = uvc_open(video_dev_name, &devattr);
		if (dev == NULL) {
			printf("func = %s, line = %d: uvc open error.\n", __FUNCTION__, __LINE__);
		}
	} else {
		printf("func = %s, line = %d: uvc_video_id_get error.\n", __FUNCTION__, __LINE__);
	}


	while(1) {sleep(100);}

	uvc_close(dev);
    
	return 0;	
}




