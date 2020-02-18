
#include <unistd.h>
#include "libavutil/audio_fifo.h"
#include "libswresample/swresample.h"
#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>
#include "media_demux.h"
#include <pthread.h>


static int aac_adts_frq_index(unsigned int uSamspleFrq)
{
	switch (uSamspleFrq) 
	{
		case 96000: return 0;
		case 88200: return 1;
		case 64000: return 2;
		case 48000: return 3;
		case 44100: return 4;
		case 32000: return 5;
		case 24000: return 6;
		case 22050: return 7;
		case 16000: return 8;
		case 12000: return 9;
		case 11025: return 10;
		case 8000:  return 11;
		case 7350:  return 12;
		default:    return 0;
	}

	return -1;
}

#define ADTS_HEADER_SIZE 7
//size = audio frame data size
static int aac_set_adts_head(MEDIA_DEMUX_STREAM_INFO_T *pStreamInfo, unsigned char *buf, int size)  
{         
      unsigned char byte;    
      if (size < ADTS_HEADER_SIZE) {
            return -1;  
      }       
      buf[0] = 0xff;  
      buf[1] = 0xf1;  
      byte = 0;  
      byte |= (2 & 0x03) << 6;  			// ADTSContext->objecttype profile
      byte |= ((aac_adts_frq_index(pStreamInfo->nASamplerate)) & 0x0f) << 2;  	// ADTSContext->sample_rate_index
      byte |= (pStreamInfo->nAChannelNum & 0x07) >> 2; 		// ADTSContext->channel_conf
      buf[2] = byte;  
      byte = 0;  
      byte |= (pStreamInfo->nAChannelNum & 0x07) << 6;  		// ADTSContext->channel_conf
      byte |= (ADTS_HEADER_SIZE + size) >> 11;  
      buf[3] = byte;  
      byte = 0;  
      byte |= (ADTS_HEADER_SIZE + size) >> 3;  
      buf[4] = byte;  
      byte = 0;  
      byte |= ((ADTS_HEADER_SIZE + size) & 0x7) << 5;  
      byte |= (0x7ff >> 6) & 0x1f;  
      buf[5] = byte;  
      byte = 0;  
      byte |= (0x7ff & 0x3f) << 2;  
      buf[6] = byte;     
	  
      return 0;  
}  



int main(int argc, char **argv)
{
	MEDIA_DEMUX_HANDLE hHandle;
	MEDIA_DEMUX_FRAME_T stMDFrame;
	MEDIA_DEMUX_STREAM_INFO_T stStreamInfo;
	unsigned char frame_buf[1*1024*1024];	// 1MB
	unsigned char adts_buf[7];

	FILE *pVFile, *pAFile;
    char *v_filename, *a_filename,*input_filename;
    int ret;


    if (argc < 4) {
        printf("usage: %s outfile videofile audiofile\n", argv[0]);
        return -1;
    }

    input_filename  = argv[1];
    v_filename = argv[2];
    a_filename = argv[3];
	//av_log_set_level(56);

	av_register_all();
	hHandle = MediaDemux_Open(input_filename, &stStreamInfo);
	printf("%s:%d stStreamInfo.nAChannelNum = %d, stStreamInfo.nASamplerate = %d\n", __FUNCTION__, __LINE__, stStreamInfo.nAChannelNum, stStreamInfo.nASamplerate);

	//MediaDemux_SeekTime(hHandle, 3000);
	//MediaDemux_SetDuration(hHandle, 180000);
	printf("%s:%d fps = %d\n", __FUNCTION__, __LINE__, MediaDemux_GetFrameRate(hHandle));

	pVFile = fopen(v_filename, "wb");
	pAFile = fopen(a_filename, "wb");
	
	while (1) {
		stMDFrame.pData = &frame_buf[0];
		ret = MediaDemux_ReadFrame(hHandle, &stMDFrame);
		if (ret < 0) {
			break;
		}

		printf("%s:%d stMDFrame.eStreamType = %d, stMDFrame.nLen = %d !\n", __FUNCTION__, __LINE__, stMDFrame.eStreamType, stMDFrame.nLen);

		if (stMDFrame.eStreamType == MEDIA_DEMUX_CODEC_H264 || stMDFrame.eStreamType == MEDIA_DEMUX_CODEC_H265) {
			fwrite(stMDFrame.pData, 1, stMDFrame.nLen, pVFile); 
			//printf("Video Data Head: %x %x %x %x %x\n", stMDFrame.pData[0], stMDFrame.pData[1], stMDFrame.pData[2], stMDFrame.pData[3], stMDFrame.pData[4]);
		} else if (stMDFrame.eStreamType == MEDIA_DEMUX_CODEC_G711A || stMDFrame.eStreamType == MEDIA_DEMUX_CODEC_G711U || stMDFrame.eStreamType == MEDIA_DEMUX_CODEC_AAC) {
            if (stStreamInfo.eAudioCodecType == MEDIA_DEMUX_CODEC_AAC) {
                aac_set_adts_head(&stStreamInfo, adts_buf, stMDFrame.nLen);
                //printf("Audio Data Head: %x %x %x %x %x\n", adts_buf[0], adts_buf[1], adts_buf[2], adts_buf[3], adts_buf[4]);
                fwrite(adts_buf, 1, ADTS_HEADER_SIZE, pAFile); 
            }
			fwrite(stMDFrame.pData, 1, stMDFrame.nLen, pAFile); 
			//printf("Audio Data Head: %x %x %x %x %x\n", stMDFrame.pData[0], stMDFrame.pData[1], stMDFrame.pData[2], stMDFrame.pData[3], stMDFrame.pData[4]);
		}
		
	}

	
	fclose(pVFile);
	fclose(pAFile);
	MediaDemux_Close(hHandle);


    return 0;
}

