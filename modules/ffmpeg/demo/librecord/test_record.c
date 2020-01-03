#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#include "record_files_mng.h"



int main (int argc, char *argv[])
{
	
	char ucFullPath[RECORD_FILEPATH_MAX_LEN] = {0};
	char cIndex_Buf[RECORD_INDEXFILE_MAX_SIZE] = {0};
	RecordFile_IndexData_T *pSearchRFID = (RecordFile_IndexData_T *)cIndex_Buf;
	RecordFile_SearchInfo_T *RFSI = NULL;
	time_t t;
	struct timeval tv1, tv2;

	t = time(NULL);
	RecordFile_Time2FullPath(0, (unsigned long)t, ucFullPath);
	RecordFile_Create(ucFullPath);
	//printf("%s:%d ucFullPath = %s\n", __FUNCTION__, __LINE__, ucFullPath);

	RecordFile_ConfData_T stRFCD;
	strcpy(stRFCD.cFileName, ucFullPath);
	stRFCD.ucEventNum = 1;
	stRFCD.stTimeInfo[0].ulStartTime = (unsigned long)t;
	stRFCD.stTimeInfo[0].ulEndTime = stRFCD.stTimeInfo[0].ulStartTime + 10;		// 测试文件长度10s
	//RecordIndexFile_Update(&stRFCD);
	RecordIndexFile_UpdateThr(&stRFCD);

	usleep(200000);
	t = time(NULL);
	RecordFile_Time2FullPath(1, (unsigned long)t, ucFullPath);
	RecordFile_Create(ucFullPath);
	//printf("%s:%d ucFullPath = %s\n", __FUNCTION__, __LINE__, ucFullPath);
	strcpy(stRFCD.cFileName, ucFullPath);
	stRFCD.ucEventNum = 1;
	stRFCD.stTimeInfo[0].ulStartTime = (unsigned long)t;
	stRFCD.stTimeInfo[0].ulEndTime = stRFCD.stTimeInfo[0].ulStartTime;
	RecordIndexFile_Update(&stRFCD);
	//RecordIndexFile_UpdateThr(&stRFCD);

	// 搜索
	gettimeofday(&tv1, NULL);
	RecordFile_TimeSearch(0, 0, time(NULL), pSearchRFID);
	gettimeofday(&tv2, NULL);
	RFSI = (RecordFile_SearchInfo_T *)pSearchRFID->ucData + pSearchRFID->nNum - 1;
	RecordFile_Time2FullPath(0, (unsigned long)RFSI->stTimeInfo[0].ulStartTime, ucFullPath);
	printf("%s:%d %s\n", __FUNCTION__, __LINE__, ucFullPath);
	printf("%s:%d search time: %dms\n", __FUNCTION__, __LINE__, (int)(tv2.tv_sec*1000+tv2.tv_usec/1000 - tv1.tv_sec*1000+tv1.tv_usec/1000));
	
	gettimeofday(&tv1, NULL);
	RecordFile_TimeSearch(1, 0, time(NULL), pSearchRFID);
	gettimeofday(&tv2, NULL);
	RFSI = (RecordFile_SearchInfo_T *)pSearchRFID->ucData + pSearchRFID->nNum - 1;
	RecordFile_Time2FullPath(1, (unsigned long)RFSI->stTimeInfo[0].ulStartTime, ucFullPath);
	printf("%s:%d %s\n", __FUNCTION__, __LINE__, ucFullPath);
	printf("%s:%d search time: %dms\n", __FUNCTION__, __LINE__, (int)(tv2.tv_sec*1000+tv2.tv_usec/1000 - tv1.tv_sec*1000+tv1.tv_usec/1000));

	if (argc > 1)
		RecordFile_OldestFileDel();

	return 0; 
}




