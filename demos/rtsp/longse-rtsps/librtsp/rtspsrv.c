#include "rtspsrv.h"

#include "rs_rtsp.h"

static pthread_t g_rtspSvrThrId = -1;
static  int g_rtspSvrStart = 0;
static int g_rtspSvrPort = 554;

int rtspSLib_ServerStart(int port, void *rbHandle)
{
	if (g_rtspSvrStart) {
		fprintf(stderr, "rtspSLib_ServerStart server aready start.......\n");
		return -1;
	}

	fprintf(stderr, "rtspSLib server starting .............\n");

	RTSP_ParamsInit();	

	RTSP_SetServerPort( port );
	g_rtspSvrPort = port;
	
	RTSP_ThrStart(&g_rtspSvrThrId, rbHandle);

	g_rtspSvrStart = 1;

	fprintf(stderr, "rtspSLib server running port -[%d]..............\n", port);

	return 1;
}

int rtspSLib_ServerStop( )
{

	RTSP_ThrStop();

	if ( -1 != g_rtspSvrThrId ) {		
		//pthread_join(g_rtspSvrThrId, NULL);	
		g_rtspSvrThrId = -1;
	}	

	g_rtspSvrStart = 0;

	return 1;
}

int rtspSLib_ServerRestart( )
{
	rtspSLib_ServerStop();

	rtspSLib_ServerStart(g_rtspSvrPort, NULL);

	return 1;

}


int rtspSLib_SetStreamInfo( int chn, int imageWidth, int imageHeight, int frameRate, int bitrate )
{

	return RTSP_SetStreamInfo( chn , imageWidth, imageHeight, frameRate, bitrate);

}
	
int rtspSLib_NeedAuthentication( int need )
{


}


