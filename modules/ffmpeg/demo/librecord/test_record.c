#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "record_files_mng.h"



int main (int argc, char *argv[])
{
	
	char ucFullPath[RECORD_FILEPATH_MAX_LEN] = {0};
	
	RecordFile_Create(0, ucFullPath);
	//printf("%s:%d ucFullPath = %s\n", __FUNCTION__, __LINE__, ucFullPath);

	RecordFile_ConfData_T stRFCD;
	strcpy(stRFCD.cFileName, ucFullPath);
	stRFCD.stTimeInfo[0].ulStartTime = (unsigned long)time(NULL);
	stRFCD.stTimeInfo[0].ulEndTime = stRFCD.stTimeInfo[0].ulStartTime + 10*60;
	RecordIndexFile_Update(0, &stRFCD);

	
	RecordFile_Create(1, ucFullPath);
	//printf("%s:%d ucFullPath = %s\n", __FUNCTION__, __LINE__, ucFullPath);
	


	return 0; 
}




