


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
    int count = 10;
    int ret;
    FILE *file = fopen ("./stream.yuv", "wb");

    struct uvc_device *dev = NULL;
    struct uvc_devattr  devattr;
    struct uvc_stream_attr stream;
    
    devattr.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    devattr.width = 256;
    devattr.height = 192;
    devattr.pix_fmt = V4L2_PIX_FMT_YUYV;
    dev = uvc_open("/dev/video0", &devattr);

    
    uvc_streamon(dev);
  
    while(count-- > 0) {
        fd_set fds;
        struct timeval tv;
        FD_ZERO(&fds);
        FD_SET(dev->fd, &fds);
        tv.tv_sec = 2;
        tv.tv_usec = 0;
        ret = select(dev->fd + 1, &fds, NULL, NULL, &tv);
        if (-1 == ret) {
            if (EINTR == errno)
                continue;
        }
        if (0 == ret) {
            printf("select timeout/n");
        }

        uvc_stream_get(dev, &stream);
        printf("stream info: width = %d, height = %d, length = %d\n", stream.width, stream.height, stream.length);
        fwrite(stream.data, stream.length, 1, file);
        uvc_stream_release(dev);
            
        
    }
  
    fclose (file);
    uvc_close(dev);
    
	return 0;	
}




