/****************************************************************************
*              Copyright 2008 - 2012, grandstream Tech. Co., Ltd.
*                           ALL RIGHTS RESERVED
* FileName: chnencode.c
* Description: chnencode functions
*
* History:
* Version   Date         Author			DefectNum    Description
* 1.1       2008-09-03   liaoshiliang        NULL         Create this file.
*****************************************************************************/

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

#include <stdlib.h>
#include <stdio.h>
#include<string.h>

#include "rs_type.h"
#include "rs_rtsp_base64.h"
#include "rs_rtsp.h"

#define MODLE_NAME	"RtspBase64 "

static RTSP_Media_Para  l_RTSP_Media_Para[MAX_ENC_NUM];

void* MP4Malloc(u32 size) 
{
    if(size==0)
        return NULL;
	void* p = (void*)malloc(size);
	if (p == NULL)/* malloc mem failedm */
	{
		rtsp_dbg(Err, DbgPerror,"MP4Malloc failed!");
		return NULL;
	}
	return memset(p, 0, size);
}


char* MP4ToBase64(const u8* pData, u32 dataSize)
{
	int len; 
    if(pData==NULL||dataSize==0)
    {
        rtsp_dbg(Err, DbgNoPerror,"MP4ToBase64 return NULL" );
        return NULL;
    }

	static const char encoding[64] = {
		'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P',
		'Q','R','S','T','U','V','W','X','Y','Z','a','b','c','d','e','f',
		'g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v',
		'w','x','y','z','0','1','2','3','4','5','6','7','8','9','+','/'
	};

	len = (((dataSize + 2) * 4) / 3) + 1;
	char* s = (char*)MP4Malloc(len);

	const u8* src = pData;
	char* dest = s;
	u32 numGroups = dataSize / 3;

    u32 i;
	for (i = 0; i < numGroups; i++) {
		*dest++ = encoding[src[0] >> 2];
		*dest++ = encoding[((src[0] & 0x03) << 4) | (src[1] >> 4)];
		*dest++ = encoding[((src[1] & 0x0F) << 2) | (src[2] >> 6)];
		*dest++ = encoding[src[2] & 0x3F];
		src += 3;
	}

	if (dataSize % 3 == 1) {
		*dest++ = encoding[src[0] >> 2];
		*dest++ = encoding[((src[0] & 0x03) << 4)];
		*dest++ = '=';
		*dest++ = '=';
	} else if (dataSize % 3 == 2) {
		*dest++ = encoding[src[0] >> 2];
		*dest++ = encoding[((src[0] & 0x03) << 4) | (src[1] >> 4)];
		*dest++ = encoding[((src[1] & 0x0F) << 2)];
		*dest++ = '=';
	}
	*dest = '\0';
	return s;	/* N.B. caller is responsible for free'ing s */
}

void printMsg(int chno)
{
#ifdef DBG
	int chn = chno;
	printf("------------- chn %d ----------------- \n", chn);
	printf("l_RTSP_Media_Para[%d].bSetBase64 = %d\n"
			"l_RTSP_Media_Para[%d].bSetSeq  = %d \n"
			"l_RTSP_Media_Para[%d].bSetPict = %d \n", 
			chn, l_RTSP_Media_Para[chn].bSetBase64, 
			chn, l_RTSP_Media_Para[chn].bSetSeq, 
			chn, l_RTSP_Media_Para[chn].bSetPict);

	if(chno == 0){
		chn = 1;
	
		printf("------------- chn %d ----------------- \n", chn);
		printf("l_RTSP_Media_Para[%d].bSetBase64 = %d\n"
			"l_RTSP_Media_Para[%d].bSetSeq  = %d \n"
			"l_RTSP_Media_Para[%d].bSetPict = %d \n", 
			chn, l_RTSP_Media_Para[chn].bSetBase64, 
			chn, l_RTSP_Media_Para[chn].bSetSeq, 
			chn, l_RTSP_Media_Para[chn].bSetPict);
	}
	printf("-------------------------------------- \n\n\n");
#endif 
}

int RTSP_Media_Paras_Init(int chn)
{
	if (GS_SUCCESS != RTSP_CheckChn(chn)){
		rtsp_dbg(Err, DbgNoPerror, "RTSP_CheckChn %d, error \n", chn);
        return GS_FAIL;
    }
	memset(&l_RTSP_Media_Para[chn], 0, sizeof(l_RTSP_Media_Para[chn]));
    l_RTSP_Media_Para[chn].bSetSeq = False;
    l_RTSP_Media_Para[chn].bSetPict = False;
    l_RTSP_Media_Para[chn].bSetBase64 = False;
	
	return(1);
}

int RTSP_Media_Paras_Exit(int chn)
{
	if (GS_SUCCESS != RTSP_CheckChn(chn)){
		rtsp_dbg(Err, DbgNoPerror, "RTSP_CheckChn %d, error \n", chn);
        return GS_FAIL;
    }

	if(l_RTSP_Media_Para[chn].pPictBase64 != NULL){
		free(l_RTSP_Media_Para[chn].pPictBase64);
		l_RTSP_Media_Para[chn].pPictBase64 = NULL; 
	}	

	if(l_RTSP_Media_Para[chn].pSeqBase64 != NULL){
		free(l_RTSP_Media_Para[chn].pSeqBase64);
		l_RTSP_Media_Para[chn].pSeqBase64 = NULL;
	}
	return GS_SUCCESS;
}

char* RTSP_Media_Para_GetBase64(int chn)
{
    if (GS_SUCCESS != RTSP_CheckChn(chn)){
		rtsp_dbg(Err, DbgNoPerror, "RTSP_CheckChn %d, error \n", chn);
        return NULL;
    }
    if(l_RTSP_Media_Para[chn].bSetBase64 == True)
    {
        return l_RTSP_Media_Para[chn].Base64;
    }
    else
    {
        return NULL;
    }
}



/*pBuff is nal (not include sync head (4byte)*/
int RTSP_Media_Para_SetSeqBase64(int chn, char* pBuff, int len)
{
    if (GS_SUCCESS != RTSP_CheckChn(chn))
    {
    	rtsp_dbg(Err, DbgNoPerror, "RTSP_CheckChn error\n");
        return GS_FAIL;
    }
    //TODO 可能会有内存泄漏。
    if ( l_RTSP_Media_Para[chn].bSetSeq == False )
    {
        if (l_RTSP_Media_Para[chn].pSeqBase64 != NULL)
        {
            free(l_RTSP_Media_Para[chn].pSeqBase64);
            l_RTSP_Media_Para[chn].pSeqBase64 = NULL;
        }
        
        l_RTSP_Media_Para[chn].pSeqBase64 = MP4ToBase64((const u8 *)pBuff, len);
        l_RTSP_Media_Para[chn].bSetSeq = True;
        rtsp_dbg(Dbg, DbgNoPerror,"chn:%d pSeqBase64 = %s", chn,l_RTSP_Media_Para[chn].pSeqBase64);
    }
    return GS_SUCCESS;
    
}

int RTSP_Media_Para_SetPictBase64(int chn, char* pBuff, int len)
{
    if (GS_SUCCESS != RTSP_CheckChn(chn))
    {
       rtsp_dbg(Err, DbgNoPerror,"Set PictBase64 chn error");
        return GS_FAIL;
    }

    if (l_RTSP_Media_Para[chn].bSetPict == False)
    {
        if (l_RTSP_Media_Para[chn].pPictBase64 != NULL)
        {
            free(l_RTSP_Media_Para[chn].pPictBase64);
            l_RTSP_Media_Para[chn].pPictBase64 = NULL;
        }
      
        l_RTSP_Media_Para[chn].pPictBase64 = MP4ToBase64((const u8 *)pBuff, len);
        l_RTSP_Media_Para[chn].bSetPict = True;
        rtsp_dbg(Dbg, DbgNoPerror,"chn:%d pPictBase64 = %s\n",chn, l_RTSP_Media_Para[chn].pPictBase64);
    }
    return GS_SUCCESS;
}

int RTSP_Media_Para_SetBase64(int chn)
{
    if (GS_SUCCESS != RTSP_CheckChn(chn))
    {
    	rtsp_dbg(Err, DbgNoPerror, "RTSP_CheckChn error\n");
        return GS_FAIL;
    }
	
    if (l_RTSP_Media_Para[chn].bSetBase64 == False)
    {
        if (l_RTSP_Media_Para[chn].bSetSeq == True &&
            l_RTSP_Media_Para[chn].bSetPict == True )
        {

            char* pBase64 = l_RTSP_Media_Para[chn].Base64;
            memset(pBase64, 0, MAX_BASE64_LEN);

            sprintf(pBase64, "%s,%s", l_RTSP_Media_Para[chn].pSeqBase64, 
								l_RTSP_Media_Para[chn].pPictBase64);

            l_RTSP_Media_Para[chn].bSetBase64 = True;   
            free(l_RTSP_Media_Para[chn].pSeqBase64);
            l_RTSP_Media_Para[chn].pSeqBase64 = NULL;
            free(l_RTSP_Media_Para[chn].pPictBase64);
            l_RTSP_Media_Para[chn].pPictBase64 = NULL;
            l_RTSP_Media_Para[chn].bSetBase64 = True;
			rtsp_dbg(Dbg, DbgNoPerror,"chn %d, RTSP_GetPropParam:%s\n",chn, pBase64);	   
            return GS_SUCCESS;
        }
        else
        {
        	
            //WRITE_LOG_DEBUG("chn:%d SetBase64 error. something not prepare.\n",chn);            
        }
        
    }
    return GS_FAIL;
}


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

