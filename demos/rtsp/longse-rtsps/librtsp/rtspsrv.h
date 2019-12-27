#ifndef __RTSPSLIB_H__
#define __RTSPSLIB_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "rs_rtsp.h"

int rtspSLib_ServerStart(int port, void *rbHandle);

int RTSP_SetTotalStreamNum( int totalStream );

//int rtspSLib_SetStreamInfo( int chn, int imageWidth, int imageHeight, int frameRate, int bitrate );
int rtspSLib_NeedAuthentication( int need );

int  rtspSLib_ServerStop( );

//int rtspSLib_ServerRestart( );



#ifdef __cplusplus
}
#endif


#endif

