/******************************************************************************
 * Copyright (C), 2008-2011, Grandstream Co., Ltd.        
 ******************************************************************************
 File Name     : gs_rtsp_base64.h
 Version       : Initial Draft
 Author        : Grandstream video software group
 Created       : 2010/02/08
 Last Modified :
 Description   : rtsp base64 ±àÂë
 Function List :
 Note		   : created 2010/02/08
 History       :
 1.Date        : 2010/02/08
   Author      : lwx
   Modification: 
 ******************************************************************************/
#ifndef __GS_RTSP_BASE64_H__
#define __GS_RTSP_BASE64_H__


#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

#include "rs_type.h"

#define TEST_VIU_WIDTH      352
#define TEST_VIU_HEIGHT     288

#define TEST_VOU_WIDTH      704
#define TEST_VOU_HEIGHT     288

#define SHOW_NONE           		0
#define SHOW_VIU            		1
#define SHOW_RECON_FRAME    	2

#define MAX_BASE64_LEN      	128
#define MAX_ENC_NUM			3

typedef struct hiRTSP_Media_Para
{
    Bools bSetSeq;
    char* pSeqBase64;
    Bools bSetPict;
    char* pPictBase64;
    Bools  bSetBase64;
    char Base64[MAX_BASE64_LEN];
}RTSP_Media_Para;

int RTSP_Media_Paras_Init(int chn);
char* RTSP_Media_Para_GetBase64(int chn);
int RTSP_Media_Para_SetSeqBase64(int chn, char* pBuff, int len);
int RTSP_Media_Para_SetPictBase64(int chn, char* pBuff, int len);
int RTSP_Media_Para_SetBase64(int chn);
int RTSP_Media_Paras_Exit(int chn);


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif

