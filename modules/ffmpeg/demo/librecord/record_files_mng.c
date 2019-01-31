#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <libgen.h>
#include <errno.h>
#include "record_files_mng.h"
#include "crc32.h"


static int _RecordFile_Create(const char *name)
{
	return close(open(name, O_CREAT|O_RDWR|O_SYNC, 0775));
}

static int _RecordFile_MKDIR(const char *path)
{
	mode_t mode = 0755;
	
    if (!path) {
        return -1;
    }
    char *temp = strdup(path);
    char *pos = temp;
    int ret = 0;

    if (strncmp(temp, "/", 1) == 0) {
        pos += 1;
    } else if (strncmp(temp, "./", 2) == 0) {
        pos += 2;
    }
    for ( ; *pos != '\0'; ++ pos) {
        if (*pos == '/') {
            *pos = '\0';
            if (-1 == (ret = mkdir(temp, mode))) {
                if (errno == EEXIST) {
                    ret = 0;
                } else {
                    fprintf(stderr, "failed to mkdir %s: %d:%s\n", temp, errno, strerror(errno));
                    break;
                }
            }
            *pos = '/';
        }
    }
    if (*(pos - 1) != '/') {
        if (-1 == (ret = mkdir(temp, mode))) {
            if (errno == EEXIST) {
                ret = 0;
            } else {
                fprintf(stderr, "failed to mkdir %s: %d:%s\n", temp, errno, strerror(errno));
            }
        }
    }
    free(temp);
    return ret;
}



/**
 * 根据当前系统时间创建录像文件，如文件夹不存在则创建文件夹
 * 文件命名根据当前时间命名
 * input: nType, 0=Video, 1:Picture
 * output: pOutFullPath, 创建完的文件全路径
 */
int RecordFile_Create(int nType, char *pOutFullPath)
{
	char cFolderName[RECORD_FOLDERNAME_MAX_LEN] = {0};
	char cFileName[RECORD_FILENAME_MAX_LEN] = {0};
	//char cFullPath[RECORD_FILEPATH_MAX_LEN] = {0};
	char cDayTimeStr[12] = {0};	// 年月日
	char cHourTimeStr[16] = {0};	// 年月日天
	char cSecTimeStr[24] = {0};	// 年月日天时分秒
	int nRet = 0;
	time_t t;
	struct tm *tm;

	if (pOutFullPath == NULL) {
		printf("%s:%d Error!\n", __FUNCTION__, __LINE__);
		return -1;
	}

	t = time(NULL);
	tm = localtime(&t);
	strftime(cDayTimeStr, 12, "%Y-%m-%d", tm);
	strftime(cHourTimeStr, 16, "%Y-%m-%d-%H", tm);
	strftime(cSecTimeStr, 24, "%Y-%m-%d-%H-%M-%S", tm);

	if (0 == nType) {
		// 录像文件路径
		sprintf(cFolderName, RECORD_TOP_FOLDERNAME"/%s/%s_V", cDayTimeStr, cHourTimeStr);
		sprintf(cFileName, "%s.mp4", cSecTimeStr);
	} else {
		// 图片文件路径
		sprintf(cFolderName, RECORD_TOP_FOLDERNAME"/%s/%s_P", cDayTimeStr, cHourTimeStr);
		sprintf(cFileName, "%s.jpg", cSecTimeStr);
	}

	sprintf(pOutFullPath, "%s/%s", cFolderName, cFileName);
	//printf("%s:%d pOutFullPath = %s\n", __FUNCTION__, __LINE__, pOutFullPath);

	nRet = _RecordFile_MKDIR(cFolderName);
	if (nRet < 0) {
		return nRet;
	}
	nRet = _RecordFile_Create(pOutFullPath);

	return nRet;
}


/**
 * 索引文件完整性检查
 * input: pName, 索引文件全路径名
 * result: 0 = success, <0 = fail
 */
int RecordIndexFile_CRCCheck(char *pName)
{
	char cIndexBuf[RECORD_INDEXFILE_MAX_SIZE] = {0};
	RecordFile_IndexData_T *pRFID = (RecordFile_IndexData_T *)cIndexBuf;
	int fd = 0, nRet = 0;

	fd = open(pName, O_RDWR|O_SYNC, 0775);
	if (fd < 0) {
		printf("%s:%d %s open error!\n", __FUNCTION__, __LINE__, pName);
		return -1;
	}

	if (sizeof(RecordFile_IndexData_T) != read(fd, (void *)pRFID, sizeof(RecordFile_IndexData_T))) {
		printf("%s:%d read Error!\n", __FUNCTION__, __LINE__);
		close(fd);
		return -1;
	}
	if (pRFID->unSize > 0) {
		if (pRFID->unSize != read(fd, (void *)pRFID->ucData, pRFID->unSize)) {
			printf("%s:%d read Error!\n", __FUNCTION__, __LINE__);
			close(fd);
			return -1;
		}
	}
	close(fd);

	if (pRFID->unCrc32 != (nRet = crc32((const char*)pRFID->ucData, pRFID->unSize))) {
		printf("%s:%d check %s crc32(%u -> %u) error!\n", __FUNCTION__, __LINE__, pName, pRFID->unCrc32, nRet);
		return -1;
	}

	return 0;
}


/**
 * 索引文件还原
 * input: pName, 备份索引文件全路径名
 * result: 0 = success, <0 = fail
 */
int RecordIndexFile_Restore(char *pName)
{
	char cIndexBuf[RECORD_INDEXFILE_MAX_SIZE] = {0};
	char cIndexFullPath[RECORD_FILEPATH_MAX_LEN] = {0};
	RecordFile_IndexData_T *pRFID = (RecordFile_IndexData_T *)cIndexBuf;
	int nRet = 0, fd = 0;

	// 读取备份文件数据
	sprintf(cIndexFullPath, "%s/%s", dirname(strcpy(cIndexFullPath, pName)), RECORD_INDEXFILE_NAME);
	fd = open(pName, O_RDWR|O_SYNC, 0775);
	if (fd < 0) {
		printf("%s:%d %s open error!\n", __FUNCTION__, __LINE__, cIndexFullPath);
		return -1;
	}
	if (sizeof(RecordFile_IndexData_T) != read(fd, (void *)pRFID, sizeof(RecordFile_IndexData_T))) {
		printf("%s:%d read Error!\n", __FUNCTION__, __LINE__);
		close(fd);
		return -1;
	}
	if (pRFID->unSize > 0) {
		if (pRFID->unSize != read(fd, (void *)pRFID->ucData, pRFID->unSize)) {
			printf("%s:%d read Error!\n", __FUNCTION__, __LINE__);
			close(fd);
			return -1;
		}
	}
	close(fd);
	// 数据写入索引文件
	fd = open(pName, O_CREAT|O_RDWR|O_SYNC, 0775);
	nRet = write(fd, pRFID, sizeof(RecordFile_IndexData_T) + pRFID->unSize);
	if (nRet < sizeof(RecordFile_IndexData_T) + pRFID->unSize) {
		printf("%s:%d write %s error\n", __FUNCTION__, __LINE__, RECORD_INDEXFILE_BAKUP);
		close(fd);
		return -1;
	}
	close(fd);

	return 0;
}




/**
 * 索引文件备份，如果当前文件校验失败则还原
 * input: pName, 索引文件全路径名
 * result: 0 = success, <0 = fail
 */
int RecordIndexFile_Bakup(char *pName)
{
	char cIndexBuf[RECORD_INDEXFILE_MAX_SIZE] = {0};
	char cIndexFullPath[RECORD_FILEPATH_MAX_LEN] = {0};
	RecordFile_IndexData_T *pRFID = (RecordFile_IndexData_T *)cIndexBuf;
	int nRet = 0, fd = 0;

	// step1: 检查当前索引文件完整性
	nRet = RecordIndexFile_CRCCheck(pName);
	if (nRet < 0) {
		printf("%s:%d %s crc check error, restoring...!\n", __FUNCTION__, __LINE__, pName);
		// step1-1: 还原索引文件(从备份文件)
		RecordIndexFile_Restore(pName);
		return -1;
	}
	
	// step2-1: 读出当前索引文件数据
	fd = open(pName, O_RDWR|O_SYNC, 0775);
	if (fd < 0) {
		printf("%s:%d %s open error!\n", __FUNCTION__, __LINE__, pName);
		return -1;
	}
	if (sizeof(RecordFile_IndexData_T) != read(fd, (void *)pRFID, sizeof(RecordFile_IndexData_T))) {
		printf("%s:%d read Error!\n", __FUNCTION__, __LINE__);
		close(fd);
		return -1;
	}
	if (pRFID->unSize > 0) {
		if (pRFID->unSize != read(fd, (void *)pRFID->ucData, pRFID->unSize)) {
			printf("%s:%d read Error!\n", __FUNCTION__, __LINE__);
			close(fd);
			return -1;
		}
	}
	close(fd);
	// step2-2: 数据写入到备份索引文件中
	char cFileName[RECORD_FILENAME_MAX_LEN] = {0};	// dirname会修改输入字符串
	sprintf(cIndexFullPath, "%s/"RECORD_INDEXFILE_BAKUP, dirname(strcpy(cFileName, pName)));
	fd = open(cIndexFullPath, O_CREAT|O_RDWR|O_SYNC, 0775);
	nRet = write(fd, pRFID, sizeof(RecordFile_IndexData_T) + pRFID->unSize);
	if (nRet < sizeof(RecordFile_IndexData_T) + pRFID->unSize) {
		printf("%s:%d write %s error\n", __FUNCTION__, __LINE__, RECORD_INDEXFILE_BAKUP);
		close(fd);
		return -1;
	}
	close(fd);

	return 0;
}


/**
 * 索引文件更新，将最新的录像文件信息更新到索引文件中，如索引文件不存在则创建
 * 每次新写入数据到索引文件之前会进行备份，如果当前索引文件被破坏(不完整)，则会从备份索引文件恢复
 * input: nType, 0=File, 1:Folder
 *        pRFCD, 录像文件信息
 * result: 0 = success, <0 = fail
 */
int RecordIndexFile_Update(int nType, RecordFile_ConfData_T *pRFCD)
{
	char cIndexFullPath[RECORD_FILEPATH_MAX_LEN] = {0};
	char cIndexBuf[RECORD_INDEXFILE_MAX_SIZE] = {0};
	RecordFile_IndexData_T *pRFID = (RecordFile_IndexData_T *)cIndexBuf;
	RecordFile_ConfData_T *pInsertPos = NULL, *pDelPos = NULL;
	int fd;
	int i, j, nRet;

	if (pRFCD == NULL){
		printf("%s:%d Error!\n", __FUNCTION__, __LINE__);
		return -1;
	}
		
	if (nType == 0) {	// 文件列表索引
		char cFileName[RECORD_FILENAME_MAX_LEN] = {0};	// dirname会修改输入字符串
		sprintf(cIndexFullPath, "%s/%s", dirname(strcpy(cFileName, pRFCD->cFileName)), RECORD_INDEXFILE_NAME);
		if (access(cIndexFullPath, F_OK|W_OK|R_OK) == 0){	// exist
			RecordIndexFile_Bakup(cIndexFullPath);
			fd = open(cIndexFullPath, O_RDWR|O_SYNC, 0775);
			if (sizeof(RecordFile_IndexData_T) != read(fd, (void *)pRFID, sizeof(RecordFile_IndexData_T))) {
				printf("%s:%d read Error!\n", __FUNCTION__, __LINE__);
				close(fd);
				return -1;
			}
			if (pRFID->unSize > 0) {
				//printf("%s:%d pRFID->unSize = %d\n", __FUNCTION__, __LINE__, pRFID->unSize);
				if (pRFID->unSize != read(fd, (void *)pRFID->ucData, pRFID->unSize)) {
					printf("%s:%d read Error!\n", __FUNCTION__, __LINE__);
					close(fd);
					return -1;
				}
			}
		} else {	// need create
			fd = open(cIndexFullPath, O_CREAT|O_RDWR|O_SYNC, 0775);
			memset(pRFID, 0, sizeof(RecordFile_IndexData_T));
		}
		lseek(fd, 0, SEEK_SET); //文件默认追加写

		//printf("%s:%d pRFID->nNum = %d\n", __FUNCTION__, __LINE__, pRFID->nNum);
		pInsertPos = (RecordFile_ConfData_T *)pRFID->ucData;
		for (i = 0; i < pRFID->nNum; i++) {
			// 分析索引数据，根据录像文件时间比较，选择插入位置
			if (pRFCD->stTimeInfo[0].ulStartTime >= pInsertPos->stTimeInfo[0].ulEndTime) {
				printf("%s:%d File Name = %s\n", __FUNCTION__, __LINE__, pInsertPos->cFileName);
				pInsertPos++;
			} else {
				break;
			}
		}
		if (i < pRFID->nNum) {	// 新录像文件开始时间不在最后
			// 删除此时间节点之后的所有文件
			pDelPos = pInsertPos;
			for (j = i; j < pRFID->nNum; j++) {
				printf("%s:%d Del File = %s\n", __FUNCTION__, __LINE__, pDelPos->cFileName);
				remove(pDelPos->cFileName);
				pDelPos++;
			}
			// 更新索引头部信息
			pRFID->nNum = i;
			pRFID->unSize = i * sizeof(RecordFile_ConfData_T);
		}
		
		// 追加写入最新文件信息到索引文件
		pRFID->nNum++;
		pRFID->unSize += sizeof(RecordFile_ConfData_T);
		printf("%s:%d Append File = %s\n", __FUNCTION__, __LINE__, pRFCD->cFileName);
		memcpy(pInsertPos, pRFCD, sizeof(RecordFile_ConfData_T));
		pRFID->unCrc32 = crc32((const char*)pRFID->ucData, pRFID->unSize);		// 需要完善CRC
		nRet = write(fd, pRFID, sizeof(RecordFile_IndexData_T) + pRFID->unSize);
		if (nRet < sizeof(RecordFile_IndexData_T) + pRFID->unSize) {
			printf("%s:%d write error\n", __FUNCTION__, __LINE__);
			close(fd);
			return -1;
		}
	} else {	// 目录列表索引

	}

	close(fd);
	return 0;
}





