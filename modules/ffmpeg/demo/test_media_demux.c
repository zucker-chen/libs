
#include <unistd.h>
#include "libavutil/audio_fifo.h"
#include "libswresample/swresample.h"
#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>
#include "media_demux.h"
#include <pthread.h>




MEDIA_DEMUX_HANDLE hHandle;



int main(int argc, char **argv)
{
    char *v_filename, *a_filename,*input_filename;
    int ret;

	//MEDEA_MUX_HANDLE hHandle;
	MEDIA_DEMUX_STREAM_INFO_T stStreamInfo;

    if (argc < 4) {
        printf("usage: %s outfile videofile audiofile\n", argv[0]);
        return -1;
    }

    input_filename  = argv[1];
    v_filename = argv[2];
    a_filename = argv[3];
	av_log_set_level(56);

	hHandle = MediaDemux_Open(input_filename, &stStreamInfo);


    return 0;
}

